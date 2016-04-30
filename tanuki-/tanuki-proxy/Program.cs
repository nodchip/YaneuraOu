using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Diagnostics;
using System.Text.RegularExpressions;
using System.Threading;
using System.Collections.Concurrent;
using System.IO;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Json;

namespace tanuki_proxy
{
    public class Program
    {
        public enum UpstreamState
        {
            Stopped,
            Thinking,
        }

        public static object upstreamLockObject = new object();
        public static UpstreamState upstreamState = UpstreamState.Stopped;
        public static int depth = 0;
        public const string bestmoveNone = "None";
        public static string bestmoveBestMove = bestmoveNone;
        public static string bestmovePonder = null;
        public static int upstreamGoIndex = 0;
        public static int numberOfReadyoks = 0;

        [DataContract]
        public struct Option
        {
            [DataMember]
            public string name;
            [DataMember]
            public string value;

            public Option(string name, string value)
            {
                this.name = name;
                this.value = value;
            }
        }

        [DataContract]
        public class EngineOption
        {
            [DataMember]
            public string engineName { get; set; }
            [DataMember]
            public string fileName { get; set; }
            [DataMember]
            public string arguments { get; set; }
            [DataMember]
            public string workingDirectory { get; set; }
            [DataMember]
            public Option[] optionOverrides { get; set; }

            public EngineOption(string engineName, string fileName, string arguments, string workingDirectory, Option[] optionOverrides)
            {
                this.engineName = engineName;
                this.fileName = fileName;
                this.arguments = arguments;
                this.workingDirectory = workingDirectory;
                this.optionOverrides = optionOverrides;
            }

            public EngineOption()
            {
                //empty constructor for serialization
            }
        }

        [DataContract]
        public class ProxySetting
        {
            [DataMember]
            public EngineOption[] engines { get; set; }
            [DataMember]
            public string logDirectory { get; set; }
        }

        class Engine
        {
            private Process process = new Process();
            private Option[] optionOverrides;
            private int downstreamGoIndex = 0;
            private object downstreamLockObject = new object();
            private BlockingCollection<string> commandQueue = new BlockingCollection<string>();
            private Thread thread;
            public string name { get; }

            public Engine(string engineName, string fileName, string arguments, string workingDirectory, Option[] optionOverrides)
            {
                this.name = engineName;
                this.process.StartInfo.FileName = fileName;
                this.process.StartInfo.Arguments = arguments;
                this.process.StartInfo.WorkingDirectory = workingDirectory;
                this.process.StartInfo.UseShellExecute = false;
                this.process.StartInfo.RedirectStandardInput = true;
                this.process.StartInfo.RedirectStandardOutput = true;
                this.process.StartInfo.RedirectStandardError = true;
                this.process.OutputDataReceived += HandleStdout;
                this.process.ErrorDataReceived += HandleStderr;
                this.optionOverrides = optionOverrides;
            }

            public Engine(EngineOption opt) : this(opt.engineName, opt.fileName, opt.arguments, opt.workingDirectory, opt.optionOverrides)
            {

            }

            public void RunAsync()
            {
                thread = new Thread(ThreadRun);
                thread.Start();
            }

            private void ThreadRun()
            {
                process.Start();
                process.BeginOutputReadLine();
                process.BeginErrorReadLine();

                while (!commandQueue.IsCompleted)
                {
                    string input = null;
                    try
                    {
                        input = commandQueue.Take();
                    }
                    catch (InvalidOperationException)
                    {
                        continue;
                    }

                    if (string.IsNullOrEmpty(input))
                    {
                        continue;
                    }

                    string[] split = Split(input);
                    if (split.Length == 0)
                    {
                        continue;
                    }

                    // 将棋所：USIプロトコルとは http://www.geocities.jp/shogidokoro/usi.html
                    if (HandleSetoption(input))
                    {
                        continue;
                    }

                    Debug.WriteLine("     >> engine={0} command={1}", name, Concat(split));
                    process.StandardInput.WriteLine(input);
                    process.StandardInput.Flush();
                }
            }

            public void Close()
            {
                commandQueue.CompleteAdding();
                thread.Join();
                process.Close();
            }

            /// <summary>
            /// エンジンにコマンドを書き込む。
            /// </summary>
            /// <param name="input"></param>
            public void Write(string input)
            {
                // コマンドをBlockingCollectionに入れる。
                // 入れられたコマンドは別のスレッドで処理される。
                commandQueue.Add(input);
            }

            /// <summary>
            /// setoptionコマンドを処理する
            /// </summary>
            /// <param name="input">UIまたは上流proxyからのコマンド文字列</param>
            /// <returns>コマンドをこの関数で処理した場合はtrue、そうでない場合はfalse</returns>
            private bool HandleSetoption(string input)
            {
                string[] split = Split(input);
                if (split[0] != "setoption")
                {
                    return false;
                }

                // エンジンに対して値を設定する時に送ります。
                Debug.Assert(split.Length == 5);
                Debug.Assert(split[1] == "name");
                Debug.Assert(split[3] == "value");

                // オプションをオーバーライドする
                foreach (var optionOverride in optionOverrides)
                {
                    if (split[2] == optionOverride.name)
                    {
                        split[4] = optionOverride.value;
                    }
                }

                Debug.WriteLine("     >> engine={0} command={1}", name, Concat(split));
                process.StandardInput.WriteLine(Concat(split));
                process.StandardInput.Flush();
                return true;
            }

            /// <summary>
            /// 思考エンジンの標準出力を処理する
            /// </summary>
            /// <param name="sender">出力を送ってきた思考エンジンのプロセス</param>
            /// <param name="e">思考エンジンの出力</param>
            private void HandleStdout(object sender, DataReceivedEventArgs e)
            {
                if (string.IsNullOrEmpty(e.Data))
                {
                    return;
                }

                Debug.WriteLine("     << engine={0} command={1}", name, e.Data);

                string[] split = Split(e.Data);

                if (HandleReadyok(e.Data))
                {
                    return;
                }

                if (HandleStarted(e.Data))
                {
                    return;
                }

                if (HandlePv(e.Data))
                {
                    return;
                }

                if (HandleBestmove(e.Data))
                {
                    return;
                }

                Debug.WriteLine("  <<    engine={0} command={1}", name, e.Data);
                Console.WriteLine(e.Data);
                Console.Out.Flush();
            }

            /// <summary>
            /// 思考エンジンの標準エラー出力を処理する
            /// </summary>
            /// <param name="sender">出力を送ってきた思考エンジンのプロセス</param>
            /// <param name="e">思考エンジンの出力</param>
            private void HandleStderr(object sender, DataReceivedEventArgs e)
            {
                if (string.IsNullOrEmpty(e.Data))
                {
                    return;
                }
                Debug.WriteLine("!!!!! engine={0} command={1}", name, e.Data);
            }

            private int getNumberOfRunningEngines()
            {
                int numberOfRunningEngines = 0;
                foreach (var engine in engines)
                {
                    if (!engine.process.HasExited)
                    {
                        ++numberOfRunningEngines;
                    }
                }
                return numberOfRunningEngines;
            }

            private bool HandleReadyok(string output)
            {
                string[] split = Split(output);
                if (!Contains(split, "readyok"))
                {
                    return false;
                }

                // goコマンドが受理されたのでpvの受信を開始する
                lock (upstreamLockObject)
                {
                    if (getNumberOfRunningEngines() == ++numberOfReadyoks)
                    {
                        Debug.WriteLine("  <<    engine={0} command={1}", name, output);
                        Console.WriteLine(output);
                    }
                }

                return true;
            }

            /// <summary>
            /// info string startedを受信し、goコマンドが受理されたときの処理を行う
            /// </summary>
            /// <param name="output">思考エンジンの出力文字列</param>
            /// <returns>コマンドをこの関数で処理した場合はtrue、そうでない場合はfalse</returns>
            private bool HandleStarted(string output)
            {
                string[] split = Split(output);
                if (!Contains(split, "started"))
                {
                    return false;
                }

                // goコマンドが受理されたのでpvの受信を開始する
                lock (downstreamLockObject)
                {
                    ++downstreamGoIndex;
                }

                return true;
            }

            /// <summary>
            /// pvを含むinfoコマンドを処理する
            /// </summary>
            /// <param name="output">思考エンジンの出力文字列</param>
            /// <returns>コマンドをこの関数で処理した場合はtrue、そうでない場合はfalse</returns>
            private bool HandlePv(string output)
            {
                string[] split = Split(output);
                if (!Contains(split, "pv"))
                {
                    return false;
                }

                lock (upstreamLockObject)
                {
                    // 上流停止中はpvを含む行を処理しない
                    if (upstreamState == UpstreamState.Stopped)
                    {
                        //Debug.WriteLine("     ## process={0} upstreamState == UpstreamState.Stopped", process);
                        return true;
                    }

                    lock (downstreamLockObject)
                    {

                        if (upstreamGoIndex != downstreamGoIndex)
                        {
                            //Debug.WriteLine("     ## process={0} upstreamGoIndex != downstreamGoIndex", process);
                            return true;
                        }
                    }

                    int depthIndex = Array.FindIndex(split, x => x == "depth");
                    int pvIndex = Array.FindIndex(split, x => x == "pv");

                    // Fail-low/Fail-highした探索結果は保持しない
                    if (depthIndex == -1 || pvIndex == -1 || Contains(split, "lowerbound") || Contains(split, "upperbound"))
                    {
                        return true;
                    }

                    int tempDepth = int.Parse(split[depthIndex + 1]);

                    // 既に保持している深さ以下の場合は保持しない
                    if (depth >= tempDepth)
                    {
                        return true;
                    }

                    depth = tempDepth;
                    Debug.Assert(pvIndex + 1 < split.Length);
                    bestmoveBestMove = split[pvIndex + 1];
                    bestmovePonder = null;
                    if (pvIndex + 2 < split.Length)
                    {
                        bestmovePonder = split[pvIndex + 2];
                    }

                    Debug.WriteLine("  <<    engine={0} command={1}", name, output);
                    Console.WriteLine("info string " + name);

                    Console.WriteLine(output);
                    WriteToEachEngine("broadcast " + output);
                }

                return true;
            }

            private bool HandleBestmove(string output)
            {
                string[] split = Split(output);
                if (!Contains(split, "bestmove"))
                {
                    return false;
                }

                if (split[1] == "resign" || split[1] == "win" || bestmoveBestMove == bestmoveNone)
                {
                    bestmoveBestMove = split[1];
                    if (split.Length == 4 && split[2] == "ponder")
                    {
                        bestmovePonder = split[3];
                    }
                }

                // 手番かつ他の思考エンジンがbestmoveを返していない時のみ
                // bestmoveを返すようにする
                lock (upstreamLockObject)
                {
                    if (upstreamState == UpstreamState.Stopped)
                    {
                        //Debug.WriteLine("     ## process={0} upstreamState == UpstreamState.Stopped", process);
                        return true;
                    }

                    lock (downstreamLockObject)
                    {
                        if (upstreamGoIndex != downstreamGoIndex)
                        {
                            //Debug.WriteLine("     ## process={0} upstreamGoIndex != downstreamGoIndex", process);
                            return true;
                        }
                    }

                    Debug.Assert(!string.IsNullOrEmpty(bestmoveBestMove));

                    string command = null;
                    if (!string.IsNullOrEmpty(bestmovePonder))
                    {
                        command = string.Format("bestmove {0} ponder {1}", bestmoveBestMove, bestmovePonder);
                    }
                    else
                    {
                        command = string.Format("bestmove {0}", bestmoveBestMove);
                    }

                    Debug.WriteLine("  <<    engine={0} command={1}", name, command);
                    Console.WriteLine(command);

                    TransitUpstreamState(UpstreamState.Stopped);
                    depth = 0;
                    bestmoveBestMove = bestmoveNone;
                    bestmovePonder = null;
                    WriteToEachEngine("stop");
                }

                return true;
            }
        }

        static List<Engine> engines = new List<Engine>();

        static string Concat(string[] split)
        {
            string result = "";
            foreach (var word in split)
            {
                if (result.Length != 0)
                {
                    result += " ";
                }
                result += word;
            }
            return result;
        }

        static int GetProcessId()
        {
            using (Process process = Process.GetCurrentProcess())
            {
                return process.Id;
            }
        }

        static void Main(string[] args)
        {
            //writeSampleSetting();
            ProxySetting setting = loadSetting();
            Debug.Listeners.Add(new TextWriterTraceListener(Console.Error));
            string logFileFormat = setting.logDirectory + "\\" + DateTime.Now.ToString("yyyy-MM-dd-HH-mm-ss") + string.Format("_pid={0}", GetProcessId()) + ".txt";
            Debug.Listeners.Add(new TextWriterTraceListener(new StreamWriter(logFileFormat, false, Encoding.UTF8)));

            foreach (var item in setting.engines)
            {
                engines.Add(new Engine(item));
            }

            // 子プロセスの標準入出力 (System.Diagnostics.Process) - Programming/.NET Framework/標準入出力 - 総武ソフトウェア推進所 http://smdn.jp/programming/netfx/standard_streams/1_process/
            try
            {
                foreach (var engine in engines)
                {
                    engine.RunAsync();
                }

                string input;
                while ((input = Console.ReadLine()) != null)
                {
                    string[] split = Split(input);

                    if (split[0] == "go")
                    {
                        // 思考開始の合図です。エンジンはこれを受信すると思考を開始します。
                        lock (upstreamLockObject)
                        {
                            bestmoveBestMove = bestmoveNone;
                            bestmovePonder = null;
                            depth = 0;
                            ++upstreamGoIndex;
                            TransitUpstreamState(UpstreamState.Thinking);
                            Console.WriteLine("info string started");
                            Console.Out.Flush();
                        }
                    }
                    else if (split[0] == "isready")
                    {
                        lock (upstreamLockObject)
                        {
                            numberOfReadyoks = 0;
                        }
                    }

                    WriteToEachEngine(input);

                    if (input == "quit")
                    {
                        break;
                    }
                }
            }
            finally
            {
                Debug.Flush();//すべての出力をファイルに書き込むのに必要
                foreach (var engine in engines)
                {
                    engine.Close();
                }
            }
        }

        /// <summary>
        /// エンジンに対して出力する
        /// </summary>
        /// <param name="input">親ソフトウェアからの入力。USIプロトコルサーバーまたは親tanuki-proxy</param>
        private static void WriteToEachEngine(string input)
        {
            foreach (var engine in engines)
            {
                engine.Write(input);
                if (input == "usi")
                {
                    break;
                }
            }
        }

        static string[] Split(string s)
        {
            return new Regex("\\s+").Split(s);
        }

        static bool Contains(string[] strings, string s)
        {
            return Array.FindIndex(strings, x => x == s) != -1;
        }

        static void TransitUpstreamState(UpstreamState newUpstreamState)
        {
            Debug.WriteLine("  || upstream {0} > {1}", upstreamState, newUpstreamState);
            upstreamState = newUpstreamState;
        }

        static void writeSampleSetting()
        {
            ProxySetting setting = new ProxySetting();
            setting.logDirectory = "C:\\home\\develop\\tanuki-";
            setting.engines = new EngineOption[]
            {
            new EngineOption(
                "doutanuki",
                "C:\\home\\develop\\tanuki-\\tanuki-\\x64\\Release\\tanuki-.exe",
                "",
                "C:\\home\\develop\\tanuki-\\bin",
                new[] {
                    new Option("USI_Hash", "1024"),
                    new Option("Book_File", "../bin/book-2016-02-01.bin"),
                    new Option("Best_Book_Move", "true"),
                    new Option("Max_Random_Score_Diff", "0"),
                    new Option("Max_Random_Score_Diff_Ply", "0"),
                    new Option("Threads", "1"),
                }),
            new EngineOption(
                "nighthawk",
                "ssh",
                "-vvv nighthawk tanuki-.bat",
                "C:\\home\\develop\\tanuki-\\bin",
                new[] {
                    new Option("USI_Hash", "1024"),
                    new Option("Book_File", "../bin/book-2016-02-01.bin"),
                    new Option("Best_Book_Move", "true"),
                    new Option("Max_Random_Score_Diff", "0"),
                    new Option("Max_Random_Score_Diff_Ply", "0"),
                    new Option("Threads", "4"),
                }),
            new EngineOption(
                "doutanuki",
                "C:\\home\\develop\\tanuki-\\tanuki-\\x64\\Release\\tanuki-.exe",
                "",
                "C:\\home\\develop\\tanuki-\\bin",
                new[] {
                    new Option("USI_Hash", "1024"),
                    new Option("Book_File", "../bin/book-2016-02-01.bin"),
                    new Option("Best_Book_Move", "true"),
                    new Option("Max_Random_Score_Diff", "0"),
                    new Option("Max_Random_Score_Diff_Ply", "0"),
                    new Option("Threads", "1"),
                })
            };

            DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(ProxySetting));
            using (FileStream f = new FileStream("proxy-setting.sample.json", FileMode.Create))
            {
                serializer.WriteObject(f, setting);
            }
        }

        static ProxySetting loadSetting()
        {
            DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(ProxySetting));
            using (FileStream f = new FileStream("proxy-setting.json", FileMode.Open, FileAccess.Read, FileShare.Read))
            {
                return (ProxySetting)serializer.ReadObject(f);
            }
        }
    }
}
