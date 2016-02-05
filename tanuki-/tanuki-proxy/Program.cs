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
        static void Main(string[] args)
        {
            // 子プロセスの標準入出力 (System.Diagnostics.Process) - Programming/.NET Framework/標準入出力 - 総武ソフトウェア推進所 http://smdn.jp/programming/netfx/standard_streams/1_process/
            using (Process child = new Process())
            {
                child.StartInfo.FileName = "C:\\home\\develop\\tanuki-\\tanuki-\\x64\\Release\\tanuki-.exe";
                child.StartInfo.WorkingDirectory = "C:\\home\\develop\\tanuki-\\bin";
                child.StartInfo.UseShellExecute = false;
                child.StartInfo.RedirectStandardInput = true;
                child.StartInfo.RedirectStandardOutput = true;
                child.OutputDataReceived += HandleStdout;

                if (!child.Start())
                {
                    return;
                }

                child.BeginOutputReadLine();

                string line;
                while ((line = Console.ReadLine()) != null)
                {
                    child.StandardInput.WriteLine(line);
                    child.StandardInput.Flush();

                    if (line == "quit")
                    {
                        break;
                    }
                }
                child.StandardInput.Close();
                child.WaitForExit();
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
