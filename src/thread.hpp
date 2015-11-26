#ifndef APERY_THREAD_HPP
#define APERY_THREAD_HPP

#include <atomic>

#include "common.hpp"
#include "evaluate.hpp"
#include "limits_type.hpp"
#include "tt.hpp"
#include "usi.hpp"

constexpr int MaxThreads = 64;
constexpr int MaxSplitPointsPerThread = 8;

struct Thread;
struct SearchStack;
class MovePicker;

enum NodeType {
  Root, PV, NonPV, SplitPointRoot, SplitPointPV, SplitPointNonPV
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

  Mutex mutex;
  std::atomic<u64> slavesMask;
  std::atomic<s64> nodes;
  std::atomic<Score> alpha;
  std::atomic<Score> bestScore;
  std::atomic<Move> bestMove;
  std::atomic<int> moveCount;
  std::atomic<bool> cutoff;
};

struct Thread {
  explicit Thread(Searcher* s);
  virtual ~Thread() {};

  virtual void idleLoop();
  void notifyOne();
  bool cutoffOccurred() const;
  bool isAvailableTo(Thread* master) const;
  void waitFor(const std::atomic<bool>& b);

  template <bool Fake>
  void split(Position& pos, SearchStack* ss, const Score alpha, const Score beta, Score& bestScore,
    Move& bestMove, const Depth depth, const Move threatMove, const int moveCount,
    MovePicker& mp, const NodeType nodeType, const bool cutNode);

  SplitPoint splitPoints[MaxSplitPointsPerThread];
  Position* activePosition;
  int idx;
  int maxPly;
  Mutex sleepLock;
  ConditionVariable sleepCond;
  std::thread handle;
  std::atomic<SplitPoint*> activeSplitPoint;
  std::atomic<int> splitPointsSize;
  std::atomic<bool> searching;
  std::atomic<bool> exit;
  Searcher* searcher;
};

struct MainThread : public Thread {
  explicit MainThread(Searcher* s) : Thread(s), thinking(true) {}
  virtual void idleLoop();
  std::atomic<bool> thinking;
};

class TimerThread : public Thread {
public:
  static constexpr int FOREVER = INT_MAX;
  explicit TimerThread(Searcher* s);
  // 待機時間だけ待ったのち思考時間のチェックを行う
  virtual void idleLoop();
  // 思考スレッドの監視を始める
  // firstMs: 初回待機時間
  // afterMs: 次回以降の待機時間
  void restartTimer(int firstMs, int afterMs);

private:
  // 初回の待機時間
  // FOREVERの場合は思考時間のチェックを行わない
  std::atomic<int> timerPeriodFirstMs;
  // 次回以降回の待機時間
  // FOREVERの場合は思考時間のチェックを行わない
  std::atomic<int> timerPeriodAfterMs;
  // 初回の待機時間を使用する場合は true
  // そうでない場合は false
  std::atomic<int> first;
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
  Mutex mutex_;
  ConditionVariable sleepCond_;

private:
  TimerThread* timer_;
  Depth minimumSplitDepth_;
};

#endif // #ifndef APERY_THREAD_HPP
