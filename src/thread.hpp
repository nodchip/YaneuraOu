#ifndef APERY_THREAD_HPP
#define APERY_THREAD_HPP

#include "common.hpp"
#include "evaluate.hpp"
#include "usi.hpp"
#include "tt.hpp"

const int MaxThreads = 64;
const int MaxSplitPointsPerThread = 8;

struct Thread;
struct SearchStack;
class MovePicker;

enum NodeType {
  Root, PV, NonPV, SplitPointRoot, SplitPointPV, SplitPointNonPV
};

// 時間や探索深さの制限を格納する為の構造体
struct LimitsType {
  LimitsType() { memset(this, 0, sizeof(LimitsType)); }

  // コマンド受け取りスレッドから変更され
  // メインスレッドで読まれるため volatile をつける
  volatile int time[ColorNum];
  volatile int increment[ColorNum];
  volatile int movesToGo;
  volatile Ply depth;
  volatile u32 nodes;
  volatile int byoyomi;
  volatile int ponderTime;
  volatile bool infinite;
  volatile bool ponder;
};

struct SplitPoint {
  const Position* pos;
  const SearchStack* ss;
  Thread* masterThread;
  Depth depth;
  Score beta;
  NodeType nodeType;
  Move threatMove;
  bool cutNode;

  MovePicker* movePicker;
  SplitPoint* parentSplitPoint;

  std::mutex mutex;
  volatile u64 slavesMask;
  volatile s64 nodes;
  volatile Score alpha;
  volatile Score bestScore;
  volatile Move bestMove;
  volatile int moveCount;
  volatile bool cutoff;
};

struct Thread {
  explicit Thread(Searcher* s);
  virtual ~Thread() {};

  virtual void idleLoop();
  void notifyOne();
  bool cutoffOccurred() const;
  bool isAvailableTo(Thread* master) const;
  void waitFor(volatile const bool& b);

  template <bool Fake>
  void split(Position& pos, SearchStack* ss, const Score alpha, const Score beta, Score& bestScore,
    Move& bestMove, const Depth depth, const Move threatMove, const int moveCount,
    MovePicker& mp, const NodeType nodeType, const bool cutNode);

  SplitPoint splitPoints[MaxSplitPointsPerThread];
  Position* activePosition;
  int idx;
  int maxPly;
  std::mutex sleepLock;
  std::condition_variable sleepCond;
  std::thread handle;
  SplitPoint* volatile activeSplitPoint;
  volatile int splitPointsSize;
  volatile bool searching;
  volatile bool exit;
  Searcher* searcher;
};

struct MainThread : public Thread {
  explicit MainThread(Searcher* s) : Thread(s), thinking(true) {}
  virtual void idleLoop();
  volatile bool thinking;
};

struct TimerThread : public Thread {
  static const int FOREVER = INT_MAX;
  explicit TimerThread(Searcher* s) :
    Thread(s),
    timerPeriodFirstMs(FOREVER),
    timerPeriodAfterMs(FOREVER),
    first(true) { }
  // 待機時間だけ待ったのち思考時間のチェックを行う
  virtual void idleLoop();
  // 初回の待機時間
  // FOREVERの場合は思考時間のチェックを行わない
  volatile int timerPeriodFirstMs;
  // 次回以降回の待機時間
  // FOREVERの場合は思考時間のチェックを行わない
  volatile int timerPeriodAfterMs;
  // 初回の待機時間を使用する場合は true
  // そうでない場合は false
  volatile int first;
};

class ThreadPool : public std::vector<Thread*> {
public:
  void init(Searcher* s);
  void exit();

  MainThread* mainThread() { return static_cast<MainThread*>((*this)[0]); }
  Depth minSplitDepth() const { return minimumSplitDepth_; }
  TimerThread* timerThread() { return timer_; }
  void wakeUp(Searcher* s);
  void sleep();
  void readUSIOptions(Searcher* s);
  Thread* availableSlave(Thread* master) const;
  void setTimer(const int msec);
  void waitForThinkFinished();
  void startThinking(
    const Position& pos,
    const LimitsType& limits,
    const std::vector<Move>& searchMoves,
    const std::chrono::time_point<std::chrono::system_clock>& goReceivedTime);

  bool sleepWhileIdle_;
  size_t maxThreadsPerSplitPoint_;
  std::mutex mutex_;
  std::condition_variable sleepCond_;

private:
  TimerThread* timer_;
  Depth minimumSplitDepth_;
};

#endif // #ifndef APERY_THREAD_HPP
