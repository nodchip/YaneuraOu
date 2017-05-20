using System;
using System.Collections.Generic;

namespace TanukiColiseum
{
    class Game
    {
        private int NumBookMoves;
        public List<string> Moves { get; } = new List<string>();
        public int Turn { get; private set; }
        public int InitialTurn { get; private set; }
        public bool Running { get; set; } = false;
        public List<Engine> Engines { get; } = new List<Engine>();
        private Random Random = new Random();
        private int TimeMs;

        public Game(int initialTurn, int timeMs, Engine engine1, Engine engine2, int numBookMoves)
        {
            this.TimeMs = timeMs;
            this.Engines.Add(engine1);
            this.Engines.Add(engine2);
            this.NumBookMoves = numBookMoves;
        }

        public void OnNewGame(string sfen)
        {
            Moves.Clear();
            int numBookMoves = Random.Next(NumBookMoves);
            foreach (var move in Util.Split(sfen))
            {
                if (move == "startpos" || move == "moves")
                {
                    continue;
                }
                Moves.Add(move);
                if (Moves.Count >= numBookMoves)
                {
                    break;
                }
            }

            Turn = InitialTurn;

            Running = true;

            foreach (var engine in Engines)
            {
                engine.Send("usinewgame");
            }
        }

        public void Go()
        {
            string command = "position startpos moves";
            foreach (var move in Moves)
            {
                command += " ";
                command += move;
            }
            Engines[Turn].Send(command);
            Engines[Turn].Send(string.Format("go byoyomi {0}", TimeMs));
        }
        public void OnMove(string move)
        {
            Moves.Add(move);
            Turn ^= 1;
        }

        public void OnGameFinished()
        {
            InitialTurn ^= 1;
            Running = false;
        }
    }
}
