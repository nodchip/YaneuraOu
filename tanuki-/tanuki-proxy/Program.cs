using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Diagnostics;
using System.Text.RegularExpressions;
using System.Threading;
using System.Collections.Concurrent;

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

        public static object upstreamLockObject = new object();
        public static UpstreamState upstreamState = UpstreamState.Stopped;
        public static int depth = 0;
        public static string bestmoveBestMove = "None";
        public static string bestmovePonder = null;
        public static ManualResetEvent bestmoveSentEvent = new ManualResetEvent(true);
        public static int upstreamGoIndex = 0;

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
            private int downstreamGoIndex = 0;
            private object downstreamLockObject = new object();
            private ManualResetEvent pvReceivedEvent = new ManualResetEvent(true);
            private BlockingCollection<string> commandQueue = new BlockingCollection<string>();
            private Thread thread;

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

            public void RunAsync()
            {
                thread = new Thread(ThreadRun);
                thread.Start();
            }

            private void ThreadRun()
            {
                process.Start();
                process.BeginOutputReadLine();

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

                    if (input == null)
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

                    if (HandleGo(input))
                    {
                        continue;
                    }

                    if (HandleStop(input))
                    {
                        continue;
                    }

                    if (HandlePonderhit(input))
                    {
                        continue;
                    }

                    Debug.WriteLine("     >> process={0} command={1}", process, Concat(split));
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
            /// エンジンに対して出力する
            /// </summary>
            /// <param name="input">親ソフトウェアからの入力。USIプロトコルサーバーまたは親tanuki-proxy</param>
            public void Write(string input)
            {
                commandQueue.Add(input);
            }

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

                Debug.WriteLine("     >> process={0} command={1}", process, Concat(split));
                process.StandardInput.WriteLine(Concat(split));
                process.StandardInput.Flush();
                return true;
            }

            private bool HandleGo(string input)
            {
                string[] split = Split(input);
                if (split[0] != "go")
                {
                    return false;
                }

                bestmoveReceivedEvent.WaitOne();

                Debug.Assert(downstreamState != DownstreamState.Thinking);
                Debug.Assert(downstreamState != DownstreamState.Pondering);
                Debug.Assert(downstreamState != DownstreamState.Stopping);

                ++downstreamGoIndex;
                if (Contains(split, "ponder"))
                {
                    TransitDownstreamState(DownstreamState.Pondering);
                }
                else
                {
                    TransitDownstreamState(DownstreamState.Thinking);
                }
                pvReceivedEvent.Reset();

                Debug.WriteLine("     >> process={0} command={1}", process, Concat(split));
                process.StandardInput.WriteLine(input);
                process.StandardInput.Flush();
                return true;
            }

            private bool HandleStop(string input)
            {
                string[] split = Split(input);
                if (split[0] != "stop")
                {
                    return false;
                }

                Debug.Assert(downstreamState != DownstreamState.Stopped);
                Debug.Assert(downstreamState != DownstreamState.Stopping);
                Debug.Assert(downstreamState != DownstreamState.Thinking);

                TransitDownstreamState(DownstreamState.Stopping);
                // bestmoveを受信するまで上流からのコマンドの受信を停止する
                bestmoveReceivedEvent.Reset();
                // pvを受信するまで待機する
                //Debug.WriteLine("     !! process={0} pvReceivedEvent.WaitOne()", process);
                pvReceivedEvent.WaitOne();
                //Debug.WriteLine("     !! process={0} passed", process);

                Debug.WriteLine("     >> process={0} command={1}", process, Concat(split));
                process.StandardInput.WriteLine(input);
                process.StandardInput.Flush();
                return true;
            }

            private bool HandlePonderhit(string input)
            {
                string[] split = Split(input);
                if (split[0] != "ponderhit")
                {
                    return false;
                }

                Debug.Assert(downstreamState != DownstreamState.Stopped);
                Debug.Assert(downstreamState != DownstreamState.Stopping);
                Debug.Assert(downstreamState != DownstreamState.Thinking);

                TransitDownstreamState(DownstreamState.Thinking);

                Debug.WriteLine("     >> process={0} command={1}", process, Concat(split));
                process.StandardInput.WriteLine(input);
                process.StandardInput.Flush();
                return true;
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

                Debug.WriteLine("     << process={0} command={1}", process, e.Data);

                string[] split = Split(e.Data);

                if (HandlePv(e.Data))
                {
                    return;
                }

                if (HandleBestmove(e.Data))
                {
                    return;
                }

                Debug.WriteLine("  <<    process={0} command={1}", process, e.Data);
                Console.WriteLine(e.Data);
                Console.Out.Flush();
            }

            private void TransitDownstreamState(DownstreamState newDownstreamState)
            {
                Debug.WriteLine("     || downstream({0}) {1} > {2}", process.StartInfo.FileName, downstreamState, newDownstreamState);
                downstreamState = newDownstreamState;
            }

            /// <summary>
            /// 
            /// </summary>
            /// <param name="split"></param>
            /// <returns>コマンドをこの関数で処理した場合はtrue、そうでない場合はfalse</returns>
            private bool HandlePv(string output)
            {
                string[] split = Split(output);
                if (!Contains(split, "pv"))
                {
                    return false;
                }

                Debug.Assert(downstreamState != DownstreamState.Stopped);

                // pvを受信したら次のコマンドを下流に送信できるようにする
                //Debug.WriteLine("     !! process={0} pvReceivedEvent.Set()", process);
                pvReceivedEvent.Set();
                //Debug.WriteLine("     !! process={0} passed", process);

                lock (upstreamLockObject)
                {
                    lock (downstreamLockObject)
                    {
                        // 停止中・停止後に受信したpvは保持しない
                        if (downstreamState == DownstreamState.Stopping)
                        {
                            //Debug.WriteLine("     ## process={0} downstreamState == DownstreamState.Stopping", process);
                            return true;
                        }

                        if (upstreamState == UpstreamState.Stopped)
                        {
                            //Debug.WriteLine("     ## process={0} upstreamState == UpstreamState.Stopped", process);
                            return true;
                        }

                        if (upstreamState == UpstreamState.Stopping)
                        {
                            //Debug.WriteLine("     ## process={0} upstreamState == UpstreamState.Stopping", process);
                            return true;
                        }

                        if (upstreamGoIndex != downstreamGoIndex)
                        {
                            //Debug.WriteLine("     ## process={0} upstreamGoIndex != downstreamGoIndex", process);
                            return true;
                        }

                        int depthIndex = Array.FindIndex(split, x => x == "depth");
                        int pvIndex = Array.FindIndex(split, x => x == "pv");

                        // Fail-low/Fail-highした探索結果は保持しない
                        if (depthIndex == -1 || pvIndex == -1 || Contains(split, "lowerbound") || Contains(split, "upperbound"))
                        {
                            return true;
                        }

                        int tempDepth = int.Parse(split[depthIndex + 1]);

                        Debug.Assert(pvIndex + 1 < split.Length);
                        string tempBestmoveBestMove = split[pvIndex + 1];
                        string tempBestmovePonder = null;
                        if (pvIndex + 2 < split.Length)
                        {
                            tempBestmovePonder = split[pvIndex + 2];
                        }

                        if (depth >= tempDepth)
                        {
                            return true;
                        }

                        depth = tempDepth;
                        bestmoveBestMove = tempBestmoveBestMove;
                        bestmovePonder = tempBestmovePonder;

                        Debug.WriteLine("  <<    process={0} command={1}", "-", output);
                        Console.WriteLine(output);
                    }
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

                Debug.Assert(downstreamState != DownstreamState.Pondering);
                Debug.Assert(downstreamState != DownstreamState.Stopped);

                TransitDownstreamState(DownstreamState.Stopped);
                // コマンドを送信できるようにする
                // DownstreamStateをStoppedに変更してから行うこと
                //Debug.WriteLine("     !! process={0} bestmoveReceivedEvent.Set()", process);
                bestmoveReceivedEvent.Set();
                //Debug.WriteLine("     !! process={0} passed", process);

                //Debug.WriteLine("     << process={0} command={1}", process, e.Data);
                if (split[1] == "resign" || split[1] == "win")
                {
                    bestmoveBestMove = split[1];
                }

                // 手番かつ他の思考エンジンがbestmoveを返していない時のみ
                // bestmoveを返すようにする
                lock (upstreamLockObject)
                {
                    lock (downstreamLockObject)
                    {
                        if (upstreamState == UpstreamState.Stopped)
                        {
                            //Debug.WriteLine("     ## process={0} upstreamState == UpstreamState.Stopped", process);
                            return true;
                        }

                        if (upstreamGoIndex != downstreamGoIndex)
                        {
                            //Debug.WriteLine("     ## process={0} upstreamGoIndex != downstreamGoIndex", process);
                            return true;
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

                        Debug.WriteLine("  <<    process={0} command={1}", "-", command);
                        Console.WriteLine(command);

                        TransitUpstreamState(UpstreamState.Stopped);
                        depth = 0;
                        bestmoveBestMove = "None";
                        bestmovePonder = null;
                        bestmoveSentEvent.Set();
                    }
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

        static void Main(string[] args)
        {
            Debug.Listeners.Add(new TextWriterTraceListener(Console.Error));

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
            engines.Add(new Engine(
                "ssh",
                "nighthawk ./tanuki.sh",
                "C:\\home\\develop\\tanuki-\\bin",
                new[] {
                    new Option("USI_Hash", "16384"),
                    new Option("Book_File", "../bin/book-2016-02-01.bin"),
                    new Option("Best_Book_Move", "true"),
                    new Option("Max_Random_Score_Diff", "0"),
                    new Option("Max_Random_Score_Diff_Ply", "0"),
                    new Option("Threads", "4"),
                }));
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
                    engine.RunAsync();
                }

                string input;
                while ((input = Console.ReadLine()) != null)
                {
                    string[] split = Split(input);

                    switch (upstreamState)
                    {
                        case UpstreamState.Stopped:
                            Debug.Assert(split[0] != "stop");
                            Debug.Assert(split[0] != "ponderhit");
                            break;

                        case UpstreamState.Thinking:
                            Debug.Assert(split[0] != "go");
                            Debug.Assert(split[0] != "stop");
                            Debug.Assert(split[0] != "ponderhit");
                            break;

                        case UpstreamState.Pondering:
                            Debug.Assert(split[0] != "go");
                            break;

                        case UpstreamState.Stopping:
                            bestmoveSentEvent.WaitOne();
                            break;
                    }

                    if (split[0] == "go")
                    {
                        // 思考開始の合図です。エンジンはこれを受信すると思考を開始します。
                        lock (upstreamLockObject)
                        {
                            bestmoveBestMove = "None";
                            bestmovePonder = null;
                            depth = 0;
                            ++upstreamGoIndex;
                            TransitUpstreamState(input.Contains("ponder") ? UpstreamState.Pondering : UpstreamState.Thinking);
                        }
                    }
                    else if (split[0] == "ponderhit")
                    {
                        // エンジンが先読み中、
                        // 前回のbestmoveコマンドでエンジンが予想した通りの手を相手が指した時に送ります。
                        // エンジンはこれを受信すると、
                        // 先読み思考から通常の思考に切り替わることになり、
                        // 任意の時点でbestmoveで指し手を返すことができます。
                        lock (upstreamLockObject)
                        {
                            TransitUpstreamState(UpstreamState.Thinking);
                        }
                    }
                    else if (split[0] == "stop")
                    {
                        // エンジンに対し思考停止を命令するコマンドです。
                        // エンジンはこれを受信したら、できるだけすぐ思考を中断し、bestmoveで指し手を返す必要があります。
                        // （現時点で最善と考えている手を返すようにして下さい。）
                        lock (upstreamLockObject)
                        {
                            TransitUpstreamState(UpstreamState.Stopping);
                        }
                        bestmoveSentEvent.Reset();
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
    }
}
