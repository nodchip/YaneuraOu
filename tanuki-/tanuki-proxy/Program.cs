using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Diagnostics;

namespace tanuki_proxy
{
    class Program
    {
        struct Option
        {
            public string name;
            public string value;

            public Option(string key, string value)
            {
                this.name = key;
                this.value = value;
            }
        }

        struct Engine
        {
            public Process process;
            public Option[] optionOverrides;

            public Engine(string fileName, string workingDirectory, Option[] optionOverrides)
            {
                this.process = new Process();
                this.process.StartInfo.FileName = fileName;
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
                "C:\\home\\develop\\tanuki-\\bin",
                new[] {
                    new Option("USI_Hash", "2048"),
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
                    foreach (var engine in engines)
                    {
                        string[] split = input.Split();
                        if (split.Length == 0)
                        {
                            continue;
                        }

                        if (split[0] == "setoption")
                        {
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
                    }

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

        private static void HandleStdout(object sender, DataReceivedEventArgs e)
        {
            if (string.IsNullOrEmpty(e.Data))
            {
                return;
            }

            Console.WriteLine(e.Data);
        }
    }
}
