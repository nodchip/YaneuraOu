using System;

namespace TanukiColiseum
{
    public class Options
    {
        public enum UserInterface
        {
            Cli,
            Gui,
        }

        public string Engine1FilePath { get; set; } = null;
        public string Engine2FilePath { get; set; } = null;
        public string Eval1FolderPath { get; set; } = null;
        public string Eval2FolderPath { get; set; } = null;
        public int NumConcurrentGames { get; set; } = 0;
        public int NumGames { get; set; } = 0;
        public int HashMb { get; set; } = 0;
        public int NumBookMoves1 { get; set; } = 0;
        public int NumBookMoves2 { get; set; } = 0;
        public string BookFileName1 { get; set; } = "no_book";
        public string BookFileName2 { get; set; } = "no_book";
        public int NumBookMoves { get; set; } = 0;
        public string SfenFilePath { get; set; } = "records_2017-05-19.sfen";
        public int TimeMs { get; set; } = 0;
        public int NumNumaNodes { get; set; } = 1;
        public UserInterface Interface { get; set; } = UserInterface.Gui;
        public int ProgressIntervalMs { get; set; } = 60 * 1000;
        public int NumThreads1 { get; set; } = 1;
        public int NumThreads2 { get; set; } = 1;
        public int BookEvalDiff1 { get; set; } = 30;
        public int BookEvalDiff2 { get; set; } = 30;
        public string ConsiderBookMoveCount1 { get; set; } = "false";
        public string ConsiderBookMoveCount2 { get; set; } = "false";

        public void Parse(string[] args)
        {
            for (int i = 0; i < args.Length; ++i)
            {
                switch (args[i])
                {
                    case "TanukiColiseum.exe":
                        break;
                    case "--engine1":
                        Engine1FilePath = args[++i];
                        break;
                    case "--engine2":
                        Engine2FilePath = args[++i];
                        break;
                    case "--eval1":
                        Eval1FolderPath = args[++i];
                        break;
                    case "--eval2":
                        Eval2FolderPath = args[++i];
                        break;
                    case "--num_concurrent_games":
                        NumConcurrentGames = int.Parse(args[++i]);
                        break;
                    case "--num_games":
                        NumGames = int.Parse(args[++i]);
                        break;
                    case "--hash":
                        HashMb = int.Parse(args[++i]);
                        break;
                    case "--time":
                        TimeMs = int.Parse(args[++i]);
                        break;
                    case "--num_numa_nodes":
                        NumNumaNodes = int.Parse(args[++i]);
                        break;
                    case "--num_book_moves1":
                        NumBookMoves1 = int.Parse(args[++i]);
                        break;
                    case "--num_book_moves2":
                        NumBookMoves2 = int.Parse(args[++i]);
                        break;
                    case "--book_file_name1":
                        BookFileName1 = args[++i];
                        break;
                    case "--book_file_name2":
                        BookFileName2 = args[++i];
                        break;
                    case "--num_book_moves":
                        NumBookMoves = int.Parse(args[++i]);
                        break;
                    case "--sfen_file_name":
                        SfenFilePath = args[++i];
                        break;
                    case "--no_gui":
                        Interface = UserInterface.Cli;
                        break;
                    case "--progress_interval_ms":
                        ProgressIntervalMs = int.Parse(args[++i]);
                        break;
                    case "--num_threads1":
                        NumThreads1 = int.Parse(args[++i]);
                        break;
                    case "--num_threads2":
                        NumThreads2 = int.Parse(args[++i]);
                        break;
                    case "--book_eval_diff1":
                        BookEvalDiff1 = int.Parse(args[++i]);
                        break;
                    case "--book_eval_diff2":
                        BookEvalDiff2 = int.Parse(args[++i]);
                        break;
                    case "--consider_book_move_count1":
                        ConsiderBookMoveCount1 = args[++i];
                        break;
                    case "--consider_book_move_count2":
                        ConsiderBookMoveCount2 = args[++i];
                        break;
                    default:
                        throw new ArgumentException("Unexpected option: " + args[i]);
                }
            }
        }

        public void Validate()
        {
            if (Engine1FilePath == null)
            {
                throw new ArgumentException("--engine1 is not specified.");
            }
            else if (Engine2FilePath == null)
            {
                throw new ArgumentException("--engine2 is not specified.");
            }
            else if (Eval1FolderPath == null)
            {
                throw new ArgumentException("--eval1 is not specified.");
            }
            else if (Eval2FolderPath == null)
            {
                throw new ArgumentException("--eval2 is not specified.");
            }
            else if (NumConcurrentGames == 0)
            {
                throw new ArgumentException("--num_concurrent_games is not specified.");
            }
            else if (NumGames == 0)
            {
                throw new ArgumentException("--num_games is not specified.");
            }
            else if (HashMb == 0)
            {
                throw new ArgumentException("--hash is not specified.");
            }
            else if (TimeMs == 0)
            {
                throw new ArgumentException("--time is not specified.");
            }
        }
    }
}
