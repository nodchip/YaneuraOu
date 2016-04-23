#ifndef APERY_SEARCH_HPP
#define APERY_SEARCH_HPP

#include "book.hpp"
#include "evaluate.hpp"
#include "move.hpp"
#include "timeManager.hpp"
#include "tt.hpp"

namespace Search {

  struct SearchStack {
    Move* pv;
    Ply ply;
    Move currentMove;
    Move excludedMove;
    Move killers[2];
    Depth reduction;
    Score staticEval;
    bool skipNullMove;
    // 評価関数の差分計算用、値が入っていないときは [0] を ScoreNotEvaluated にしておく。
    // 常に Black 側から見た評価値を入れておく。
    // 0: 双玉に対する評価値, 1: 先手玉に対する評価値, 2: 後手玉に対する評価値
    EvalSum staticEvalRaw;
    int moveCount;
  };

  /// RootMove struct is used for moves at the root of the tree. For each root move
  /// we store a score and a PV (really a refutation in the case of moves which
  /// fail low). Score is normally set at -VALUE_INFINITE for all non-pv moves.

  struct RootMove {

    explicit RootMove(Move m) : pv(1, m) {}

    bool operator<(const RootMove& m) const { return m.score < score; } // Descending sort
    bool operator==(const Move& m) const { return pv[0] == m; }
    void insert_pv_in_tt(Position& pos);
    bool extract_ponder_from_tt(Position& pos);

    Score score = -ScoreInfinite;
    Score previousScore = -ScoreInfinite;
    std::vector<Move> pv;
  };

  typedef std::vector<RootMove> RootMoveVector;

  struct LimitsType {

    LimitsType() { // Init explicitly due to broken value-initialization of non POD in MSVC
      nodes = time[White] = time[Black] = inc[White] = inc[Black] = npmsec = movestogo =
        depth = movetime = mate = infinite = ponder = byoyomi = 0;
      startTime = static_cast<TimePoint>(0);
    }

    bool use_time_management() const {
      return !(mate | movetime | depth | nodes | infinite);
    }

    std::vector<Move> searchmoves;
    int time[ColorNum], inc[ColorNum], npmsec, movestogo, depth, movetime, mate, infinite, ponder, byoyomi;
    int64_t nodes;
    TimePoint startTime;
  };

  struct SignalsType {
    std::atomic_bool stop, stopOnPonderhit;
  };

  using StateStackPtr = std::unique_ptr<std::stack<StateInfo> >;

  extern SignalsType Signals;
  extern LimitsType Limits;
  extern StateStackPtr SetupStates;
  extern std::mutex BroadcastMutex;
  extern int BroadcastPvDepth;
  extern std::string BroadcastPvInfo;

  void init();
  void clear();

  enum InaniwaFlag {
    NotInaniwa,
    InaniwaIsBlack,
    InaniwaIsWhite,
    InaniwaFlagNum
  };

  enum BishopInDangerFlag {
    NotBishopInDanger,
    BlackBishopInDangerIn28,
    WhiteBishopInDangerIn28,
    BlackBishopInDangerIn78,
    WhiteBishopInDangerIn78,
    BishopInDangerFlagNum
  };

  extern Book book;

}

#endif // #ifndef APERY_SEARCH_HPP
