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
        private List<string> Options;
        private SemaphoreSlim ReadyokSemaphoreSlim = new SemaphoreSlim(0);
        private int ProcessIndex;
        private int GameIndex;
        private int EngineIndex;

        public Engine(string fileName, List<string> options, Coliseum coliseum, int processIndex, int gameIndex, int engineIndex, int numaNode)
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
            this.Options = options;
            this.ProcessIndex = processIndex;
            this.GameIndex = gameIndex;
            this.EngineIndex = engineIndex;
        }

        /// <summary>
        /// 思考エンジンを開始し、isreadyを送信し、readyokが返るのを待つ
        /// </summary>
        /// <returns></returns>
        public async Task StartAsync()
        {
            //BeginOutputReadLine()/BeginErrorReadLine()が呼び出されたあと
            //読み込みスレッドが動かないので
            // スレッド経由で実行する
            // TODO(nodchip): async/await構文に書き換える
            Process.Start();
            Process.BeginOutputReadLine();
            Process.BeginErrorReadLine();
            Send("usi");
            foreach (var option in Options)
            {
                Send(option);
            }
            Send("isready");
            await ReadyokSemaphoreSlim.WaitAsync().ConfigureAwait(false);
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
            if (command.Contains("readyok"))
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
                bool draw;
                if (command[1] == "resign")
                {
                    // 相手側の勝数を上げる
                    engineWin = game.Turn ^ 1;
                    blackWhiteWin = (game.Moves.Count + 1) & 1;
                    draw = false;
                }
                else if (command[1] == "win")
                {
                    // 自分側の勝数を上げる
                    engineWin = game.Turn;
                    blackWhiteWin = game.Moves.Count & 1;
                    draw = false;
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
                Coliseum.OnGameFinished(engineWin, blackWhiteWin, draw);
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