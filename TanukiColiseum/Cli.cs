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
            int engine1Win = status.Win[0, 0] + status.Win[0, 1];
            int engine2Win = status.Win[1, 0] + status.Win[1, 1];
            int draw = status.NumDraw;
            int blackWin = status.Win[0, 0] + status.Win[1, 0];
            int whiteWin = status.Win[0, 1] + status.Win[1, 1];
            int engine1DeclarationWin = status.DeclarationWin[0];
            int engine2DeclarationWin = status.DeclarationWin[1];

            // T1,b10000,433 - 54 - 503(46.26% R-26.03) win black : white = 52.42% : 47.58%
            double winRate = engine1Win / (double)(engine1Win + engine2Win);
            double rating = 0.0;
            if (1e-8 < winRate && winRate < 1.0 - 1e-8)
            {
                rating = -400.0 * Math.Log10((1.0 - winRate) / winRate);
            }
            double black = blackWin / (double)(blackWin + whiteWin);
            double white = whiteWin / (double)(blackWin + whiteWin);
            Console.WriteLine("T1,b{0},{1} - {2} - {3}({4:0.00%} R{5:0.00}) win black: white = {6:0.00%} : {7:0.00%} declaration win engine1={8} engine2={9}",
                status.TimeMs, engine1Win, draw, engine2Win, winRate, rating, black, white,
                engine1DeclarationWin, engine2DeclarationWin);
            Console.Out.Flush();
        }
    }
}
