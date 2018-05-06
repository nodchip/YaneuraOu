using System;

namespace TanukiColiseum
{
    public class Cli
    {
        public void Run(Options options)
        {
            var coliseum = new Coliseum();
            coliseum.OnStatusChanged += ShowResult;
            coliseum.Run(options);
        }

        private void ShowResult(Status status)
        {
            string engine1 = "engine1";
            string engine2 = "engine2";
            int numGames = status.NumGames;
            int blackWin = status.Win[0, 0] + status.Win[1, 0];
            int whiteWin = status.Win[0, 1] + status.Win[1, 1];
            int blackWinRatio = 100 * blackWin / (blackWin + whiteWin);
            int whiteWinRatio = 100 * whiteWin / (blackWin + whiteWin);
            int numDraw = status.NumDraw;
            int engine1Win = status.Win[0, 0] + status.Win[0, 1];
            int engine1BlackWin = status.Win[0, 0];
            int engine1WhiteWin = status.Win[0, 1];
            int engine2Win = status.Win[1, 0] + status.Win[1, 1];
            int engine2BlackWin = status.Win[1, 0];
            int engine2WhiteWin = status.Win[1, 1];
            int engine1WinRatio = 100 * engine1Win / (engine1Win + engine2Win);
            int engine2WinRatio = 100 * engine2Win / (engine1Win + engine2Win);
            int engine1BlackWinRatio = 100 * engine1BlackWin / (engine1Win + engine2Win);
            int engine1WhiteWinRatio = 100 * engine1WhiteWin / (engine1Win + engine2Win);
            int engine2BlackWinRatio = 100 * engine2BlackWin / (engine1Win + engine2Win);
            int engine2WhiteWinRatio = 100 * engine2WhiteWin / (engine1Win + engine2Win);
            int numFinishedGames = engine1Win + engine2Win + numDraw;
            int engine1DeclarationWin = status.DeclarationWin[0];
            int engine2DeclarationWin = status.DeclarationWin[1];

            double winRate = engine1Win / (double)(engine1Win + engine2Win);
            double rating = 0.0;
            if (1e-8 < winRate && winRate < 1.0 - 1e-8)
            {
                rating = -400.0 * Math.Log10((1.0 - winRate) / winRate);
            }

            Console.WriteLine(
                @"対局数{0} 先手勝ち{1}({2}%) 後手勝ち{3}({4}%) 引き分け{5}
{6}
勝ち{7}({8}% R{22:0.00}) 先手勝ち{9}({10}%) 後手勝ち{11}({12}%) 宣言勝ち{20}
{13}
勝ち{14}({15}%) 先手勝ち{16}({17}%) 後手勝ち{18}({19}%) 宣言勝ち{21}",
                numFinishedGames, blackWin, blackWinRatio, whiteWin, whiteWinRatio, numDraw,
                engine1,
                engine1Win, engine1WinRatio, engine1BlackWin, engine1BlackWinRatio, engine1WhiteWin, engine1WhiteWinRatio,
                engine2,
                engine2Win, engine2WinRatio, engine2BlackWin, engine2BlackWinRatio, engine2WhiteWin, engine2WhiteWinRatio,
                engine1DeclarationWin, engine2DeclarationWin, rating);
            Console.Out.Flush();
        }
    }
}
