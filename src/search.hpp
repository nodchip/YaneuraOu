#ifndef APERY_SEARCH_HPP
#define APERY_SEARCH_HPP

#include <atomic>
#include "book.hpp"
#include "move.hpp"
#include "pieceScore.hpp"
#include "thread.hpp"
#include "timeManager.hpp"
#include "tt.hpp"

class Position;
struct SplitPoint;

struct SearchStack {
  SplitPoint* splitPoint;
  Ply ply;
  Move currentMove;
  Move excludedMove; // todo: これは必要？
  Move killers[2];
  Depth reduction;
  Score staticEval;
  bool skipNullMove;
  // 評価関数の差分計算用、値が入っていないときは [0] を ScoreNotEvaluated にしておく。
  // 常に Black 側から見た評価値を入れておく。
  // 0: 双玉に対する評価値, 1: 先手玉に対する評価値, 2: 後手玉に対する評価値
  EvalSum staticEvalRaw;
};

struct SignalsType {
  std::atomic<bool> stopOnPonderHit;
  std::atomic<bool> firstRootMove;
  std::atomic<bool> stop;
  std::atomic<bool> failedLowAtRoot;
  std::atomic<bool> skipMainThreadCurrentDepth;
};

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

class RootMove {
public:
  RootMove() {}
  explicit RootMove(const Move m) : score_(-ScoreInfinite), prevScore_(-ScoreInfinite) {
    pv_.push_back(m);
    pv_.push_back(Move::moveNone());
  }
  explicit RootMove(const std::tuple<Move, Score> m) : score_(std::get<1>(m)), prevScore_(-ScoreInfinite) {
    pv_.push_back(std::get<0>(m));
    pv_.push_back(Move::moveNone());
  }

  bool operator < (const RootMove& m) const {
    return score_ < m.score_;
  }
  bool operator == (const Move& m) const {
    return pv_[0] == m;
  }

  void extractPvFromTT(Position& pos);
  void insertPvInTT(Position& pos);

public:
  Score score_;
  Score prevScore_;
  std::vector<Move> pv_;
};

template <bool Gain>
class Stats {
public:
  static constexpr Score MaxScore = static_cast<Score>(2000);

  void clear() { memset(table_, 0, sizeof(table_)); }
  Score value(const bool isDrop, const Piece pc, const Square to) const {
    assert(0 < pc && pc < PieceNone);
    assert(isInSquare(to));
    return table_[isDrop][pc][to];
  }
  void update(const bool isDrop, const Piece pc, const Square to, const Score s) {
    if (Gain) {
      table_[isDrop][pc][to] = std::max(s, value(isDrop, pc, to) - 1);
    }
    else if (abs(value(isDrop, pc, to) + s) < MaxScore) {
      table_[isDrop][pc][to] += s;
    }
  }

private:
  // [isDrop][piece][square] とする。
  Score table_[2][PieceNone][SquareNum];
};

using History = Stats<false>;
using Gains = Stats<true>;

class TranspositionTable;

// 思考スレッドの監視スレッドの実行周期の最小値
constexpr int MIN_TIMER_PERIOD_MS = 5;
// 思考スレッドの監視スレッドの実行周期の最大値
constexpr int MAX_TIMER_PERIOD_MS = 100;

struct Searcher {
  // static メンバ関数からだとthis呼べないので代わりに thisptr を使う。
  // static じゃないときは this を入れることにする。
  STATIC Searcher* thisptr;
  STATIC SignalsType signals;
  STATIC std::vector<Move> searchMoves;
  STATIC u64 lastSearchedNodes;
  STATIC StateStackPtr setUpStates;
  STATIC std::vector<RootMove> rootMoves;

#if defined LEARN
  STATIC Score alpha;
  STATIC Score beta;
#endif

  STATIC size_t pvSize;
  STATIC size_t pvIdx;
  STATIC double bestMoveChanges;
  STATIC History history;
  STATIC Gains gains;
  STATIC TranspositionTable tt;

#if defined INANIWA_SHIFT
  STATIC InaniwaFlag inaniwaFlag;
#endif
#if defined BISHOP_IN_DANGER
  STATIC BishopInDangerFlag bishopInDangerFlag;
#endif
  STATIC Position rootPosition;
  STATIC ThreadPool threads;
  STATIC Book book;
  STATIC int broadcastedPvDepth;
  STATIC std::string broadcastedPvInfo;
  STATIC std::atomic<int> mainThreadCurrentSearchDepth;

  STATIC void init();
  STATIC void idLoop(Position& pos);
  STATIC void skipCurrentDepth(Position& pos, Ply& depth);
  STATIC std::string pvInfoToUSI(Position& pos, const Ply depth, const Score alpha, const Score beta);
  template <NodeType NT, bool INCHECK>
  STATIC Score qsearch(Position& pos, SearchStack* ss, Score alpha, Score beta, const Depth depth);
#if defined INANIWA_SHIFT
  STATIC void detectInaniwa(const Position& pos);
#endif
#if defined BISHOP_IN_DANGER
  STATIC void detectBishopInDanger(const Position& pos);
#endif
  template <NodeType NT>
  STATIC Score search(Position& pos, SearchStack* ss, Score alpha, Score beta, const Depth depth, const bool cutNode);
  STATIC void think();
  STATIC void checkTime();

  STATIC void doUSICommandLoop(int argc, char* argv[]);
  STATIC void setOption(const std::string& cmd);
  STATIC void setOption(std::istringstream& ssCmd);
};

void initSearchTable();

#endif // #ifndef APERY_SEARCH_HPP
