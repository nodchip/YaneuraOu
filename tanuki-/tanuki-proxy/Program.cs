using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Diagnostics;
using System.Text.RegularExpressions;
using System.Threading;

namespace tanuki_proxy
{
    class Program
    {
        public enum UpstreamState
        {
            Stopped,
            Thinking,
            Pondering,
            Stopping,
        }

        public static object lockObject = new object();
        public static UpstreamState upstreamState = UpstreamState.Stopped;
        public static int depth = 0;
        public static string bestmoveBestMove = "None";
        public static string bestmovePonder = null;

        struct Option
        {
            public string name;
            public string value;

            public Option(string name, string value)
            {
                this.name = name;
                this.value = value;
            }
        }

        class Engine
        {
            private enum DownstreamState
            {
                Stopped,
                Thinking,
                Pondering,
                Stopping,
            }

            private Process process = new Process();
            private Option[] optionOverrides;
            private DownstreamState downstreamState = DownstreamState.Stopped;
            private ManualResetEvent bestmoveReceivedEvent = new ManualResetEvent(true);
            private object ioLock = new object();

            public Engine(string fileName, string arguments, string workingDirectory, Option[] optionOverrides)
            {
                this.process.StartInfo.FileName = fileName;
                this.process.StartInfo.Arguments = arguments;
                this.process.StartInfo.WorkingDirectory = workingDirectory;
                this.process.StartInfo.UseShellExecute = false;
                this.process.StartInfo.RedirectStandardInput = true;
                this.process.StartInfo.RedirectStandardOutput = true;
                this.process.OutputDataReceived += HandleStdout;
                this.optionOverrides = optionOverrides;
            }
            public void Start()
            {
                process.Start();
                process.BeginOutputReadLine();
            }
            public void Close()
            {
                process.Close();
            }

            /// <summary>
            /// エンジンに対して出力する
            /// </summary>
            /// <param name="input">親ソフトウェアからの入力。USIプロトコルサーバーまたは親tanuki-proxy</param>
            public void Write(string input)
            {
                string[] split = Split(input);
                if (split.Length == 0)
                {
                    return;
                }

                // 将棋所：USIプロトコルとは http://www.geocities.jp/shogidokoro/usi.html
                if (split[0] == "setoption")
                {
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
                }

                switch (downstreamState)
                {
                    case DownstreamState.Stopped:
                        Debug.Assert(split[0] != "stop");
                        Debug.Assert(split[0] != "ponderhit");
                        break;

                    case DownstreamState.Thinking:
                        Debug.Assert(split[0] != "go");
                        Debug.Assert(split[0] != "stop");
                        Debug.Assert(split[0] != "ponderhit");
                        break;

                    case DownstreamState.Pondering:
                        Debug.Assert(split[0] != "go");
                        if (Contains(split, "stop"))
                        {
                            downstreamState = DownstreamState.Stopping;
                            // bestmoveを受信するまで上流からのコマンドの受信を停止する
                            bestmoveReceivedEvent.Reset();
                        }
                        else if (split[0] == "ponderhit")
                        {
                            downstreamState = DownstreamState.Thinking;
                        }
                        break;

                    case DownstreamState.Stopping:
                        bestmoveReceivedEvent.WaitOne(10 * 1000);
                        break;
                }

                if (split[0] == "go")
                {
                    downstreamState = Contains(split, "ponder") ? DownstreamState.Pondering : DownstreamState.Thinking;
                }

                process.StandardInput.WriteLine(Concat(split));
                process.StandardInput.Flush();
            }

            /// <summary>
            /// 思考エンジンの出力を処理する
            /// </summary>
            /// <param name="sender">出力を送ってきた思考エンジンのプロセス</param>
            /// <param name="e">思考エンジンの出力</param>
            private void HandleStdout(object sender, DataReceivedEventArgs e)
            {
                if (string.IsNullOrEmpty(e.Data))
                {
                    return;
                }

                string[] split = Split(e.Data);

                switch (downstreamState)
                {
                    case DownstreamState.Stopped:
                        Debug.Assert(split[0] != "bestmove");
                        // 上流からのstopコマンドを受信して停止中にinfoを処理しないようにする
                        if (Contains(split, "depth"))
                        {
                            return;
                        }
                        break;

                    case DownstreamState.Thinking:
                        break;

                    case DownstreamState.Pondering:
                        Debug.Assert(split[0] != "bestmove");
                        break;

                    case DownstreamState.Stopping:
                        Debug.Assert(split[0] != "stop");
                        // 上流からのstopコマンドを受信して停止中にinfoを処理しないようにする
                        if (Contains(split, "depth"))
                        {
                            return;
                        }
                        break;
                }

                if (split[0] == "bestmove")
                {
                    if (split[1] == "resign" || split[1] == "win")
                    {
                        bestmoveBestMove = split[1];
                    }

                    // 手番かつ他の思考エンジンがbestmoveを返していない時のみ
                    // bestmoveを返すようにする
                    if (upstreamState == UpstreamState.Thinking || upstreamState == UpstreamState.Stopping)
                    {
                        // bestmoveは直接上流に送信せず、OutputBestMove()の中で送信する
                        OutputBestMove();
                        lock (lockObject)
                        {
                            upstreamState = UpstreamState.Stopped;
                        }
                    }
                    downstreamState = DownstreamState.Stopped;
                    // コマンドを送信できるようにする
                    bestmoveReceivedEvent.Set();
                    return;
                }

                // info depthは直接返さず、HandleInfo()の中で返すようにする
                if (e.Data.Contains("depth"))
                {
                    HandleInfo(e.Data);
                    return;
                }

                //Console.Error.WriteLine(output);
                Console.WriteLine(e.Data);
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

        static void Main(string[] args)
        {
            engines.Add(new Engine(
                "C:\\home\\develop\\tanuki-\\tanuki-\\x64\\Release\\tanuki-.exe",
                "",
                "C:\\home\\develop\\tanuki-\\bin",
                new[] {
                    new Option("USI_Hash", "2048"),
                    new Option("Book_File", "../bin/book-2016-02-01.bin"),
                    new Option("Best_Book_Move", "true"),
                    new Option("Max_Random_Score_Diff", "0"),
                    new Option("Max_Random_Score_Diff_Ply", "0"),
                    new Option("Threads", "1"),
                }));
            //engines.Add(new Engine(
            //    "ssh",
            //    "nighthawk ./tanuki.sh",
            //    "C:\\home\\develop\\tanuki-\\bin",
            //    new[] {
            //        new Option("USI_Hash", "16384"),
            //        new Option("Book_File", "../bin/book-2016-02-01.bin"),
            //        new Option("Best_Book_Move", "true"),
            //        new Option("Max_Random_Score_Diff", "0"),
            //        new Option("Max_Random_Score_Diff_Ply", "0"),
            //        new Option("Threads", "4"),
            //    }));
            //engines.Add(new Engine(
            //    "ssh",
            //    "nue ./tanuki.sh",
            //    "C:\\home\\develop\\tanuki-\\bin",
            //    new[] {
            //        new Option("USI_Hash", "4096"),
            //        new Option("Book_File", "../bin/book-2016-02-01.bin"),
            //        new Option("Best_Book_Move", "true"),
            //        new Option("Max_Random_Score_Diff", "0"),
            //        new Option("Max_Random_Score_Diff_Ply", "0"),
            //        new Option("Threads", "4"),
            //    }));

            // 子プロセスの標準入出力 (System.Diagnostics.Process) - Programming/.NET Framework/標準入出力 - 総武ソフトウェア推進所 http://smdn.jp/programming/netfx/standard_streams/1_process/
            try
            {
                foreach (var engine in engines)
                {
                    engine.Start();
                }

                string input;
                while ((input = Console.ReadLine()) != null)
                {
                    string[] split = Split(input);
                    if (split[0] == "go")
                    {
                        // 思考開始の合図です。エンジンはこれを受信すると思考を開始します。
                        lock (lockObject)
                        {
                            bestmoveBestMove = "None";
                            bestmovePonder = null;
                            depth = 0;
                            upstreamState = input.Contains("ponder") ? UpstreamState.Pondering : UpstreamState.Thinking;
                        }
                    }
                    else if (split[0] == "ponderhit")
                    {
                        // エンジンが先読み中、
                        // 前回のbestmoveコマンドでエンジンが予想した通りの手を相手が指した時に送ります。
                        // エンジンはこれを受信すると、
                        // 先読み思考から通常の思考に切り替わることになり、
                        // 任意の時点でbestmoveで指し手を返すことができます。
                        lock (lockObject)
                        {
                            upstreamState = UpstreamState.Thinking;
                        }
                    }
                    else if (split[0] == "stop")
                    {
                        // エンジンに対し思考停止を命令するコマンドです。
                        // エンジンはこれを受信したら、できるだけすぐ思考を中断し、bestmoveで指し手を返す必要があります。
                        // （現時点で最善と考えている手を返すようにして下さい。）
                        lock (lockObject)
                        {
                            upstreamState = UpstreamState.Stopping;
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

        /// <summary>
        /// 現在保持している探索結果より深い結果が来たら更新する
        /// </summary>
        /// <param name="output">思考エンジンによる出力</param>
        private static void HandleInfo(string output)
        {
            string[] split = Split(output);
            int depthIndex = Array.FindIndex(split, x => x == "depth");
            int pvIndex = Array.FindIndex(split, x => x == "pv");
            int lowerboundIndex = Array.FindIndex(split, x => x == "lowerbound");
            int upperboundIndex = Array.FindIndex(split, x => x == "upperbound");

            // Fail-low/Fail-highした探索結果は処理しない
            if (depthIndex == -1 || pvIndex == -1 || lowerboundIndex != -1 || upperboundIndex != -1)
            {
                return;
            }

            int tempDepth = int.Parse(split[depthIndex + 1]);

            Debug.Assert(pvIndex + 1 < split.Length);
            string tempBestmoveBestMove = split[pvIndex + 1];
            string tempBestmovePonder = null;
            if (pvIndex + 2 < split.Length)
            {
                tempBestmovePonder = split[pvIndex + 2];
            }

            lock (lockObject)
            {
                if (depth >= tempDepth)
                {
                    return;
                }

                depth = tempDepth;
                bestmoveBestMove = tempBestmoveBestMove;
                bestmovePonder = tempBestmovePonder;
            }

            Console.WriteLine(output);
        }

        /// <summary>
        /// bestmoveを出力する
        /// </summary>
        static void OutputBestMove()
        {
            lock (lockObject)
            {
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
                //Console.Error.WriteLine(command);
                Console.WriteLine(command);

                upstreamState = UpstreamState.Stopped;
                depth = 0;
                bestmoveBestMove = "None";
                bestmovePonder = null;
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
    }
}
