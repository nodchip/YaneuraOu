namespace TanukiColiseum
{
    public class Status
    {
        public int NumGames { get; set; }
        public int NumThreads { get; set; }
        public int TimeMs { get; set; }
        // [エンジン][先手・後手]
        public int[,] Win { get; } = { { 0, 0 }, { 0, 0 }, };
        public int NumDraw;

        public Status() { }

        public Status(Status status)
        {
            NumGames = status.NumGames;
            NumThreads = status.NumThreads;
            TimeMs = status.TimeMs;
            NumDraw = status.NumDraw;
            for (int engine = 0; engine < 2; ++engine)
            {
                for (int blackWhite = 0; blackWhite < 2; ++blackWhite)
                {
                    Win[engine, blackWhite] = status.Win[engine, blackWhite];
                }
            }
        }
    }
}
