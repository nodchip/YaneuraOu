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
        public static object lockObject = new object();
        public static bool thinking = false;
        public static int depth = 0;
        public static string bestmoveBestMove = null;
        public static string bestmovePonder = null;
        // Ponder中にbestmoveを返してしまう場合があるバグへの対処
        public static bool pondering = false;

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

        struct Engine
        {
            public Process process;
            public Option[] optionOverrides;

            public Engine(string fileName, string arguments, string workingDirectory, Option[] optionOverrides)
            {
                this.process = new Process();
                this.process.StartInfo.FileName = fileName;
                this.process.StartInfo.Arguments = arguments;
                this.process.StartInfo.WorkingDirectory = workingDirectory;
                this.process.StartInfo.UseShellExecute = false;
                this.process.StartInfo.RedirectStandardInput = true;
                this.process.StartInfo.RedirectStandardOutput = true;
                this.process.OutputDataReceived += HandleStdout;
                this.optionOverrides = optionOverrides;
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
                    new Option("Threads", "2"),
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
            engines.Add(new Engine(
                "ssh",
                "nue ./tanuki.sh",
                "C:\\home\\develop\\tanuki-\\bin",
                new[] {
                    new Option("USI_Hash", "4096"),
                    new Option("Book_File", "../bin/book-2016-02-01.bin"),
                    new Option("Best_Book_Move", "true"),
                    new Option("Max_Random_Score_Diff", "0"),
                    new Option("Max_Random_Score_Diff_Ply", "0"),
                    new Option("Threads", "4"),
                }));

            // 子プロセスの標準入出力 (System.Diagnostics.Process) - Programming/.NET Framework/標準入出力 - 総武ソフトウェア推進所 http://smdn.jp/programming/netfx/standard_streams/1_process/
            try
            {
                foreach (var engine in engines)
                {
                    if (!engine.process.Start())
                    {
                        return;
                    }

                    engine.process.BeginOutputReadLine();
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
                            thinking = true;
                            bestmoveBestMove = null;
                            bestmovePonder = null;
                            depth = 0;
                            pondering = input.Contains("ponder");
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
                            pondering = false;
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
                    engine.process.Close();
                }
            }
        }

        /// <summary>
        /// 各思考エンジンに対して出力する
        /// </summary>
        /// <param name="input">親ソフトウェアからの入力。USIプロトコルサーバーまたは親tanuki-proxy</param>
        private static void WriteToEachEngine(string input)
        {
            foreach (var engine in engines)
            {
                string[] split = Split(input);
                if (split.Length == 0)
                {
                    continue;
                }

                // 将棋所：USIプロトコルとは http://www.geocities.jp/shogidokoro/usi.html
                if (split[0] == "setoption")
                {
                    // エンジンに対して値を設定する時に送ります。
                    Debug.Assert(split.Length == 5);
                    Debug.Assert(split[1] == "name");
                    Debug.Assert(split[3] == "value");

                    // オプションをオーバーライドする
                    foreach (var optionOverride in engine.optionOverrides)
                    {
                        if (split[2] == optionOverride.name)
                        {
                            split[4] = optionOverride.value;
                        }
                    }
                }

                engine.process.StandardInput.WriteLine(Concat(split));
                engine.process.StandardInput.Flush();

                // usiコマンドは1回だけ処理する
                if (split[0] == "usi")
                {
                    break;
                }
            }
        }

        /// <summary>
        /// 思考エンジンの出力を処理する
        /// </summary>
        /// <param name="sender">出力を送ってきた思考エンジンのプロセス</param>
        /// <param name="e">思考エンジンの出力</param>
        private static void HandleStdout(object sender, DataReceivedEventArgs e)
        {
            string output = e.Data;
            if (string.IsNullOrEmpty(output))
            {
                return;
            }

            // bestmoveは直接親に返さず、OutputBestMove()の中で返すようにする
            if (output.Contains("bestmove"))
            {
                if (thinking && !pondering)
                {
                    OutputBestMove();
                }
                WriteToEachEngine("stop");
                return;
            }

            // info depthは直接返さず、HandleInfo()の中で返すようにする
            if (output.Contains("depth"))
            {
                HandleInfo(output);
                return;
            }

            //Console.Error.WriteLine(output);
            Console.WriteLine(output);
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

                thinking = false;
                depth = 0;
                bestmoveBestMove = null;
                bestmovePonder = null;
                pondering = false;
            }
        }

        static string[] Split(string s)
        {
            return new Regex("\\s+").Split(s);
        }
    }
}
