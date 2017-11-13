using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Threading;
using System.Threading.Tasks;

namespace TanukiColiseum
{
    class Engine
    {
        private const int MaxMoves = 256;
        private Process Process = new Process();
        private Coliseum Coliseum;
        private SemaphoreSlim ReadyokSemaphoreSlim = new SemaphoreSlim(0);
        private int ProcessIndex;
        private int GameIndex;
        private int EngineIndex;
        private Dictionary<string, string> OverriddenOptions;

        public Engine(string fileName, Coliseum coliseum, int processIndex, int gameIndex, int engineIndex, int numaNode, Dictionary<string, string> overriddenOptions)
        {
            this.Process.StartInfo.FileName = "cmd.exe";
            if (Path.IsPathRooted(fileName))
            {
                this.Process.StartInfo.WorkingDirectory = Path.GetDirectoryName(fileName);
            }
            this.Process.StartInfo.Arguments = string.Format("/c start /B /WAIT /NODE {0} {1}", numaNode, fileName);
            this.Process.StartInfo.UseShellExecute = false;
            this.Process.StartInfo.RedirectStandardInput = true;
            this.Process.StartInfo.RedirectStandardOutput = true;
            this.Process.StartInfo.RedirectStandardError = true;
            this.Process.OutputDataReceived += HandleStdout;
            this.Process.ErrorDataReceived += HandleStderr;
            this.Coliseum = coliseum;
            this.ProcessIndex = processIndex;
            this.GameIndex = gameIndex;
            this.EngineIndex = engineIndex;
            this.OverriddenOptions = overriddenOptions;
        }

        /// <summary>
        /// 思考エンジンを開始し、isreadyを送信し、readyokが返るのを待つ
        /// </summary>
        /// <returns></returns>
        public Task StartAsync()
        {
            //BeginOutputReadLine()/BeginErrorReadLine()が呼び出されたあと
            //読み込みスレッドが動かないので
            // スレッド経由で実行する
            // TODO(nodchip): async/await構文に書き換える
            Process.Start();
            Process.BeginOutputReadLine();
            Process.BeginErrorReadLine();
            Send("usi");
            return ReadyokSemaphoreSlim.WaitAsync();
        }

        public void Finish()
        {
            Send("quit");
            Process.WaitForExit();
            Process.Dispose();
        }

        /// <summary>
        /// エンジンにコマンドを送信する
        /// </summary>
        /// <param name="command"></param>
        public void Send(string command)
        {
            //Debug.WriteLine("    > [{0}] {1}", ProcessIndex, command);
            Process.StandardInput.WriteLine(command);
            Process.StandardInput.Flush();
        }

        /// <summary>
        /// 思考エンジンの標準出力を処理する
        /// </summary>
        /// <param name="sender">出力を送ってきた思考エンジンのプロセス</param>
        /// <param name="e">思考エンジンの出力</param>
        private void HandleStdout(object sender, DataReceivedEventArgs e)
        {
            if (e.Data == null)
            {
                return;
            }

            //Debug.WriteLine("    < [{0}] {1}", ProcessIndex, e.Data);

            List<string> command = Util.Split(e.Data);
            if (command.Contains("usiok"))
            {
                HandleUsiok(command);
            }
            else if (command.Contains("option"))
            {
                HandleOption(command);
            }
            else if (command.Contains("readyok"))
            {
                HandleReadyok(command);
            }
            else if (command.Contains("bestmove"))
            {
                HandleBestmove(command);
            }
        }

        /// <summary>
        /// 思考エンジンの標準エラー出力を処理する
        /// </summary>
        /// <param name="sender">出力を送ってきた思考エンジンのプロセス</param>
        /// <param name="e">思考エンジンの出力</param>
        private void HandleStderr(object sender, DataReceivedEventArgs e)
        {
            Debug.WriteLine("    ! [{0}] {1}", ProcessIndex, e.Data);
        }

        private void HandleUsiok(List<string> command)
        {
            Send("isready");
        }
        private void HandleOption(List<string> command)
        {
            int nameIndex = command.IndexOf("name");
            int defaultIndex = command.IndexOf("default");
            if (nameIndex == -1 || nameIndex + 1 >= command.Count ||
                defaultIndex == -1 || defaultIndex + 1 >= command.Count)
            {
                return;
            }

            string name = command[nameIndex + 1];
            string value = command[defaultIndex + 1];
            if (OverriddenOptions.ContainsKey(name))
            {
                value = OverriddenOptions[name];
            }
            Send(string.Format("setoption name {0} value {1}", name, value));
        }

        private void HandleReadyok(List<string> command)
        {
            // readyokが返ってきたのでセマフォを開放してStart()関数から抜けさせる
            ReadyokSemaphoreSlim.Release();
        }

        private void HandleBestmove(List<string> command)
        {
            var game = Coliseum.Games[GameIndex];
            if (command[1] == "resign" || command[1] == "win" || game.Moves.Count >= MaxMoves)
            {
                int engineWin;
                int blackWhiteWin;
                bool draw = false;
                bool declaration = false;
                if (command[1] == "resign")
                {
                    // 相手側の勝数を上げる
                    engineWin = game.Turn ^ 1;
                    blackWhiteWin = (game.Moves.Count + 1) & 1;
                }
                else if (command[1] == "win")
                {
                    // 自分側の勝数を上げる
                    engineWin = game.Turn;
                    blackWhiteWin = game.Moves.Count & 1;
                    declaration = true;
                }
                else
                {
                    // 引き分け
                    engineWin = 0;
                    blackWhiteWin = 0;
                    draw = true;
                }

                // 次の対局を開始する
                // 先にGame.OnGameFinished()を読んでゲームの状態を停止状態に移行する
                game.OnGameFinished();
                Coliseum.OnGameFinished(engineWin, blackWhiteWin, draw, declaration);
            }
            else
            {
                game.OnMove(command[1]);
                game.Go();
            }
        }

        public void Stop()
        {
            Send("stop");
        }
    }
}