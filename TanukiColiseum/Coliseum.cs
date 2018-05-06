using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

namespace TanukiColiseum
{
    class Coliseum
    {
        private SemaphoreSlim GameSemaphoreSlim;
        private SemaphoreSlim FinishSemaphoreSlim = new SemaphoreSlim(0);
        public List<Game> Games { get; } = new List<Game>();
        private DateTime LastOutput = DateTime.Now;
        private int ProgressIntervalMs;
        private Status Status { get; } = new Status();
        public delegate void StatusHandler(Status status);
        public event StatusHandler OnStatusChanged;

        public void Run(Options options)
        {
            Status.NumGames = options.NumGames;
            Status.TimeMs = options.TimeMs;
            ProgressIntervalMs = options.ProgressIntervalMs;

            // 開始局面集を読み込む
            string[] openings = File.ReadAllLines(options.SfenFilePath);

            Console.WriteLine("Initializing engines...");
            Console.Out.Flush();

            List<Task> startAsyncTasks = new List<Task>();

            for (int gameIndex = 0; gameIndex < options.NumConcurrentGames; ++gameIndex)
            {
                int numaNode = gameIndex * options.NumNumaNodes / options.NumConcurrentGames;

                // エンジン1初期化
                Dictionary<string, string> overriddenOptions1 = new Dictionary<string, string>(){
                    {"EvalDir", options.Eval1FolderPath},
                    {"Hash", options.HashMb.ToString()},
                    {"MinimumThinkingTime", "1000"},
                    {"NetworkDelay", "0"},
                    {"NetworkDelay2", "0"},
                    {"EvalShare", "true"},
                    {"BookMoves", options.NumBookMoves1.ToString()},
                    {"BookFile", options.BookFileName1},
                    {"Threads", options.NumThreads1.ToString()},
                    {"BookEvalDiff", options.BookEvalDiff1.ToString()},
                    {"ConsiderBookMoveCount", options.ConsiderBookMoveCount1},
                    {"MaxMovesToDraw", "256"},
                };
                Console.WriteLine("Starting the engine process " + (gameIndex * 2));
                Console.Out.Flush();
                var engine1 = new Engine(options.Engine1FilePath, this, gameIndex * 2, gameIndex, 0, numaNode, overriddenOptions1);
                startAsyncTasks.Add(engine1.StartAsync());

                // エンジン2初期化
                Dictionary<string, string> overriddenOptions2 = new Dictionary<string, string>()
                {
                    {"EvalDir", options.Eval2FolderPath},
                    {"Hash", options.HashMb.ToString()},
                    {"MinimumThinkingTime", "1000"},
                    {"NetworkDelay", "0"},
                    {"NetworkDelay2", "0"},
                    {"EvalShare", "true"},
                    {"BookMoves", options.NumBookMoves2.ToString()},
                    {"BookFile", options.BookFileName2},
                    {"Threads", options.NumThreads2.ToString()},
                    {"BookEvalDiff", options.BookEvalDiff2.ToString()},
                    {"ConsiderBookMoveCount", options.ConsiderBookMoveCount2},
                    {"MaxMovesToDraw", "256"},
                };
                Console.WriteLine("Starting the engine process " + (gameIndex * 2 + 1));
                Console.Out.Flush();
                var engine2 = new Engine(options.Engine2FilePath, this, gameIndex * 2 + 1, gameIndex, 1, numaNode, overriddenOptions2);
                startAsyncTasks.Add(engine2.StartAsync());

                // ゲーム初期化
                // 偶数番目はengine1が先手、奇数番目はengine2が先手
                Games.Add(new Game(gameIndex & 1, options.TimeMs, engine1, engine2,
                    options.NumBookMoves, openings));
            }

            foreach (var startAsyncTask in startAsyncTasks)
            {
                startAsyncTask.Wait();
            }

            Console.WriteLine("Initialized engines...");
            Console.WriteLine("Started games...");
            Console.Out.Flush();

            // numConcurrentGames局同時に対局できるようにする
            GameSemaphoreSlim = new SemaphoreSlim(options.NumConcurrentGames, options.NumConcurrentGames);
            var random = new Random();
            for (int i = 0; i < options.NumGames; ++i)
            {
                GameSemaphoreSlim.Wait();

                // 空いているゲームインスタンスを探す
                Game game = Games.Find(x => !x.Running);
                game.OnNewGame();
                game.Go();
            }

            while (Games.Count(game => game.Running) > 0)
            {
                FinishSemaphoreSlim.Wait();
            }

            foreach (var game in Games)
            {
                foreach (var engine in game.Engines)
                {
                    engine.Finish();
                }
            }

            Console.WriteLine("engine1={0} eval1={1}", options.Engine1FilePath, options.Eval1FolderPath);
            Console.WriteLine("engine2={0} eval2={1}", options.Engine2FilePath, options.Eval2FolderPath);
            OnStatusChanged(new Status(Status));
        }

        public void OnGameFinished(int engineWin, int blackWhiteWin, bool draw, bool declarationWin)
        {
            if (!draw)
            {
                Interlocked.Increment(ref Status.Win[engineWin, blackWhiteWin]);
            }
            else
            {
                Interlocked.Increment(ref Status.NumDraw);
            }

            if (declarationWin)
            {
                Interlocked.Increment(ref Status.DeclarationWin[engineWin]);
            }

            if (LastOutput.AddMilliseconds(ProgressIntervalMs) <= DateTime.Now)
            {
                OnStatusChanged(new Status(Status));
                LastOutput = DateTime.Now;
            }
            GameSemaphoreSlim.Release();
            FinishSemaphoreSlim.Release();
        }
    }
}
