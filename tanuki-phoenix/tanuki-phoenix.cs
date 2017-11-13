using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Json;
using System.Diagnostics;
using System.Text.RegularExpressions;

namespace tanuki_phoenix
{
    [DataContract]
    public class Settings
    {
        [DataMember]
        public string working_directory = "";
        [DataMember]
        public string exe_path = "";
        [DataMember]
        public List<string> arguments = new List<string>();
        [DataMember]
        public double retry_sleep_ms = 200;
        [DataMember]
        public int retry_count = 9999;
        // position/go関係はタイミングを間違えるとillegal moveにつながる。resend_goをfalseにすれば
        // position/goの再送を行わず、1指し手分の思考時間は無駄にすることで安全側に倒すことができる。
        [DataMember]
        public bool resend_go = false;
    }

    class Program : IDisposable
    {
        static string settings_file_name = "tanuki-phoenix.json";
        static string log_file_name = System.IO.Path.Combine(AppDomain.CurrentDomain.BaseDirectory, 
            String.Format("tanuki-phoenix-{0}-{1}.log", DateTime.Now.ToLocalTime().ToString("yyyyMMdd_HHmmss"), System.Diagnostics.Process.GetCurrentProcess().Id));

        static Settings GetSettings()
        {
            string settings_file_path = System.IO.Path.Combine(AppDomain.CurrentDomain.BaseDirectory, settings_file_name);

            using (var fs = new System.IO.FileStream(settings_file_path, System.IO.FileMode.Open, System.IO.FileAccess.Read))
            {
                var serializer = new DataContractJsonSerializer(typeof(Settings));
                Settings s = (Settings)serializer.ReadObject(fs);

                return s;
            }
        }

        static void Main(string[] args)
        {
            using (Program p = new Program())
            {
                p.Start();
            }
        }

        Settings settings = GetSettings();

        // 上流から受け取ったusi setoptionのコマンドリスト
        List<string> m_usi_set_options = new List<string>();

        // 初回のUSIシーケンスやGO, BESTMOVEが完了する前にプロセスが死んだ場合、
        // 二個目以降のプロセスでもその直前まではエミュレートする必要がある。
        // また、GOを受けて思考中に死んだ場合は、再起動後にPOSITION、GOを再送する必要がある。
        enum EUSISequence
        {
            NONE,
            USI_USIOK,
            READY_READYOK,
            USINEWGAME,
            NO_POSITION_GO, // bestmoveにより古いposition/goが無効化された状態
            POSITION,
            GO,
        }
        EUSISequence m_initial_sequence_done = EUSISequence.NONE;
        string m_last_position = null;
        string m_last_go = null;
        bool m_terminate = false;
        object m_state_lock = new object();

        // プロセス再起動回数
        int m_n_retried = 0;

        // ログ
        System.IO.StreamWriter m_log_writer = null;

        // 子プロセス
        Process m_active_process = null;

        // active_processの使用可能状態
        System.Threading.SemaphoreSlim m_process_semaphore = new System.Threading.SemaphoreSlim(0, 1);
        System.Threading.SemaphoreSlim m_process_stdout_semaphore = new System.Threading.SemaphoreSlim(0, 1);
        System.Threading.SemaphoreSlim m_process_stdin_semaphore = new System.Threading.SemaphoreSlim(0, 1);

        public void Log(string msg)
        {
            string timestr = DateTime.Now.ToLocalTime().ToString();
            lock (m_log_writer)
            {
                string line = String.Format("{0}: {1}", timestr, msg);
                m_log_writer.WriteLine(line);
                Debug.WriteLine(line);
            }
        }

        public void Log(string format, params object[] objects)
        {
            Log(String.Format(format, objects));
        }

        public void Dispose()
        {
            m_log_writer?.Close();
        }

        void Start()
        {
            if (!System.IO.Directory.Exists(settings.working_directory))
            {
                Console.WriteLine("info ERROR working directory does not exist");
                return;
            }
            if (!System.IO.File.Exists(settings.exe_path))
            {
                Console.WriteLine("info ERROR exe does not exist");
                return;
            }

            m_log_writer = new System.IO.StreamWriter(System.IO.File.Open(log_file_name, System.IO.FileMode.Append, System.IO.FileAccess.Write));
            Log("log init");

            DoTask();
        }


        // 上流に送る
        void SendToUpstream(string line)
        {
            Log("U< {0} (sending)", line);
            Console.WriteLine(line);
            Log("U< {0} (sent)", line);
        }

        // 上流からもらう
        string RecvFromUpstream(System.IO.StreamReader stdin_reader)
        {
            Log("U> (waiting)");
            string line = stdin_reader.ReadLine();
            line = line.TrimEnd();
            Log("U> {0}", line);
            return line;
        }

        // 下流に送る
        void SendToDownstream(string line)
        {
            m_process_stdin_semaphore.Wait();
            SendToDownstreamNoLock(line);
            m_process_stdin_semaphore.Release();
        }
        void SendToDownstreamNoLock(string line)
        {
            Log(">D {0} (sending)", line);
            m_active_process.StandardInput.WriteLine(line);
            m_active_process.StandardInput.Flush();
            Log(">D {0} (sent)", line);
        }

        // 下流からもらう
        string RecvFromDownstream()
        {
            while (true)
            {
                m_process_stdout_semaphore.Wait();
                try
                {
                    string line = m_active_process.StandardOutput.ReadLine();
                    if (line == null)
                    {
                        // プロセスが死んだ場合
                        Debug.WriteLine("ERROR NULL read line");
                        continue;
                    }
                    line = line.TrimEnd();
                    Log("<D {0}", line);
                    return line;
                }
                catch
                {
                    Debug.WriteLine("CATCH");
                }
                finally
                {
                    m_process_stdout_semaphore.Release();
                }
            }
        }
        string RecvFromDownstreamNoLock()
        {
            while (true)
            {
                string line = m_active_process.StandardOutput.ReadLine();
                if (line == null)
                {
                    Debug.WriteLine("ERROR NULL read line");
                    continue;
                }
                line = line.TrimEnd();
                Log("<D {0}", line);
                return line;
            }
        }

        // 特定の文字列を待つ
        void WaitExpectedNoLock(string expect_line)
        {
            while (true)
            {
                Log(String.Format("W) waiting downstream for [{0}] {1}", expect_line, m_active_process.HasExited));
                string line = RecvFromDownstreamNoLock();
                if (line == expect_line)
                {
                    Log(String.Format("W) match"));
                    return;
                }
                Log(String.Format("W) not match"));
            }
        }


        void DoTask()
        {
            m_n_retried = 0;

            // 下流から受け取って上流に渡す
            System.Threading.Thread downstream_to_upstream_task = new System.Threading.Thread(new System.Threading.ThreadStart(() => 
            {
                var name_pattern = new Regex("^id name (.*?)$");
                Log("D2U) downstream_to_upstream_task started.");
                try
                {
                    while (true)
                    {
                        string line = RecvFromDownstream();

                        // 初期化シーケンスの完了をマーク
                        lock (m_state_lock)
                        {
                            if (line == "usiok") m_initial_sequence_done = EUSISequence.USI_USIOK;
                            if (line == "readyok") m_initial_sequence_done = EUSISequence.READY_READYOK;
                            if (line.StartsWith("bestmove")) m_initial_sequence_done = EUSISequence.NO_POSITION_GO; // POSITION, GOを巻き戻す
                        }

                        // プログラム名を書き換え
                        Match match = name_pattern.Match(line);
                        if (match.Success)
                        {
                            line = String.Format("id name {0}-phoenix", match.Groups[1].ToString());
                        }

                        // 上流に渡す
                        SendToUpstream(line);
                    }
                }
                catch (System.Threading.ThreadAbortException e)
                {
                    Log("D2U) downstream_to_upstream_task terminating: {0}", e.Message);
                }
            }));
            downstream_to_upstream_task.Start();

            // 上流から受け取って下流に渡す
            System.Threading.Thread upstream_to_downstream_task = new System.Threading.Thread(new System.Threading.ThreadStart(() => 
            {
                Log("U2D) upstream_to_downstream_task started.");
                using (var stdin = Console.OpenStandardInput())
                using (var stdin_reader = new System.IO.StreamReader(stdin))
                while (true)
                {
                    // 上流から読み込む
                    string line = RecvFromUpstream(stdin_reader);

                    lock (m_state_lock)
                    {
                        // 初期化シーケンスの完了をマーク
                        if (line == "usinewgame") m_initial_sequence_done = EUSISequence.USINEWGAME;
                        // 状態の保存
                        if (line.StartsWith("position"))
                        {
                            m_last_position = line;
                            m_initial_sequence_done = EUSISequence.POSITION;
                        }
                        if (line.StartsWith("go"))
                        {
                            m_last_go = line;
                            m_initial_sequence_done = EUSISequence.GO;
                        }
                        if (line.StartsWith("stop") || line.StartsWith("gameover"))
                        {
                            m_initial_sequence_done = EUSISequence.NO_POSITION_GO;
                        }
                        // setoptionの保存
                        if (line.StartsWith("usi setoption"))
                        {
                            Log("adding setoption.");
                            m_usi_set_options.Add(line);
                        }
                    }

                    // 下流に渡す
                    SendToDownstream(line);

                    // quitは下流に伝達してからこのプロセスを終了
                    if (line == "quit")
                    {
                        Log("quit..");
                        lock (m_state_lock)
                            m_terminate = true;
                        break;
                    }
                }

                Log("U2D) upstream_to_downstream_task terminating.");
            }));
            upstream_to_downstream_task.Start();

            // プロセスを回す
            while (true)
            {
                CreateProcessAndInitialize();
                m_active_process.WaitForExit();
                lock (m_state_lock)
                {
                    if (m_terminate || m_n_retried >= settings.retry_count)
                        break;
                }
                System.Threading.Thread.Sleep((int)settings.retry_sleep_ms);
            }
            Log("M  ) finished");
            downstream_to_upstream_task.Abort();
            Log("M  ) terminate");
        }

        // 子プロセスを開始(main thread)
        void CreateProcessAndInitialize()
        {
            ProcessStartInfo info = new ProcessStartInfo(settings.exe_path, String.Join(" ", settings.arguments));
            info.CreateNoWindow = true;
            info.WorkingDirectory = settings.working_directory;
            info.RedirectStandardInput = true;
            info.RedirectStandardOutput = true;
            info.RedirectStandardError = false;
            info.ErrorDialog = false;
            info.UseShellExecute = false;
            
            Process p = new Process();
            p.StartInfo = info;

            p.EnableRaisingEvents = true;
            p.Exited += new EventHandler((object o, EventArgs e) =>
            {
                // 使用可能プロセスが減る
                Debug.WriteLine("process termination. lock the process...");
                m_process_semaphore.Wait();
                m_process_stdout_semaphore.Wait();
                m_process_stdin_semaphore.Wait();
                Debug.WriteLine("process termination. lock the process...OK");
            });

            Console.WriteLine("info string tanuki-phoenix: launching a process. {0}/{1}", m_n_retried, settings.retry_count);
            Log(String.Format("M  ) launching #{0}/{1}-th process", m_n_retried, settings.retry_count));

            m_active_process = p;
            m_active_process.Start();
            Log("M  ) launched.");

            // 他のタスクが読み書きできるようになる前に初期化シーケンスを行う
            lock (m_state_lock)
            {
                Log(String.Format("M  ) init sequence.."));
                if (m_initial_sequence_done >= EUSISequence.USI_USIOK)
                {
                    SendToDownstreamNoLock("usi");
                    WaitExpectedNoLock("usiok");
                }
                if (m_initial_sequence_done >= EUSISequence.READY_READYOK)
                {
                    foreach (string cmd in m_usi_set_options)
                    {
                        SendToDownstreamNoLock(cmd);
                    }
                    SendToDownstreamNoLock("isready");
                    WaitExpectedNoLock("readyok");
                }
                if (m_initial_sequence_done >= EUSISequence.USINEWGAME)
                {
                    SendToDownstreamNoLock("usinewgame");
                }
                if (settings.resend_go && m_initial_sequence_done >= EUSISequence.POSITION)
                {
                    SendToDownstreamNoLock(m_last_position);
                }
                if (settings.resend_go && m_initial_sequence_done >= EUSISequence.GO)
                {
                    SendToDownstreamNoLock(m_last_go);
                }
            }

            // 使用可能プロセスが増え、他のタスクで定常の読み書きを行う
            m_n_retried += 1;
            m_process_semaphore.Release();
            m_process_stdout_semaphore.Release();
            m_process_stdin_semaphore.Release();
            Log("M  ) process init OK. release the process...OK");
        }

    }
}
