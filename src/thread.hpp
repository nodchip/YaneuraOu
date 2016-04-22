#ifndef APERY_THREAD_HPP
#define APERY_THREAD_HPP

#include "movePicker.hpp"
#include "search.hpp"
#include "tt.hpp"

constexpr int MaxThreads = 64;

/// Thread struct keeps together all the thread related stuff. We also use
/// per-thread pawn and material hash tables so that once we get a pointer to an
/// entry its life time is unlimited and we don't have to care about someone
/// changing the entry under our feet.

class Thread {

  std::thread nativeThread;
  Mutex mutex;
  ConditionVariable sleepCondition;
  bool exit, searching;

public:
  Thread(Position& rootPos);
  virtual ~Thread();
  virtual void search();
  void idle_loop();
  void start_searching(bool resume = false);
  void wait_for_search_finished();
  void wait(std::atomic_bool& b);

  //Pawns::Table pawnsTable;
  //Material::Table materialTable;
  //Endgames endgames;
  size_t idx, PVIdx;
  int maxPly, callsCnt;

  Position& rootPos;
  Search::RootMoveVector rootMoves;
  Depth rootDepth;
  History history;
  Gains gains;
  //HistoryStats history;
  //MovesStats counterMoves;
  Depth completedDepth;
  std::atomic_bool resetCalls;
};


/// MainThread is a derived class with a specific overload for the main thread

struct MainThread : public Thread {
  using Thread::Thread;
  virtual void search();

  bool easyMovePlayed, failedLow;
  double bestMoveChanges;
};


/// ThreadPool struct handles all the threads related stuff like init, starting,
/// parking and, most importantly, launching a thread. All the access to threads
/// data is done through this class.

struct ThreadPool : public std::vector<Thread*> {

  void init(); // No constructor and destructor, threads rely on globals that should
  void exit(); // be initialized and valid during the whole thread lifetime.

  MainThread* main() { return static_cast<MainThread*>(at(0)); }
  void start_thinking(const Position&, const Search::LimitsType&, Search::StateStackPtr&);
  void read_usi_options();
  int64_t nodes_searched();
};

extern ThreadPool Threads;

//#include <atomic>
//
//#include "common.hpp"
//#include "evaluate.hpp"
//#include "limits_type.hpp"
//#include "tt.hpp"
//#include "usi.hpp"
//
//constexpr int MaxThreads = 64;
//constexpr int MaxSplitPointsPerThread = 8;
//
//struct Thread;
//struct SearchStack;
//class MovePicker;
//
//enum NodeType {
//  Root, PV, NonPV, SplitPointRoot, SplitPointPV, SplitPointNonPV
//};
//
//struct SplitPoint {
//  const Position* pos;
//  const SearchStack* ss;
//  Thread* masterThread;
//  Depth depth;
//  Score beta;
//  NodeType nodeType;
//  Move threatMove;
//  bool cutNode;
//
//  MovePicker* movePicker;
//  SplitPoint* parentSplitPoint;
//
//  Mutex mutex;
//  std::atomic<u64> slavesMask;
//  std::atomic<s64> nodes;
//  std::atomic<Score> alpha;
//  std::atomic<Score> bestScore;
//  std::atomic<Move> bestMove;
//  std::atomic<int> moveCount;
//  std::atomic<bool> cutoff;
//};
//
//struct Thread {
//  explicit Thread(Searcher* s);
//  virtual ~Thread() {};
//
//  virtual void idleLoop();
//  void notifyOne();
//  bool cutoffOccurred() const;
//  bool isAvailableTo(Thread* master) const;
//  void waitFor(const std::atomic<bool>& b);
//
//  template <bool Fake>
//  void split(Position& pos, SearchStack* ss, const Score alpha, const Score beta, Score& bestScore,
//    Move& bestMove, const Depth depth, const Move threatMove, const int moveCount,
//    MovePicker& mp, const NodeType nodeType, const bool cutNode);
//
//  SplitPoint splitPoints[MaxSplitPointsPerThread];
//  Position* activePosition;
//  int idx;
//  int maxPly;
//  Mutex sleepLock;
//  ConditionVariable sleepCond;
//  std::thread handle;
//  std::atomic<SplitPoint*> activeSplitPoint;
//  std::atomic<int> splitPointsSize;
//  std::atomic<bool> searching;
//  std::atomic<bool> exit;
//  Searcher* searcher;
//};
//
//struct MainThread : public Thread {
//  explicit MainThread(Searcher* s) : Thread(s), thinking(true) {}
//  virtual void idleLoop();
//  std::atomic<bool> thinking;
//};
//
//class TimerThread : public Thread {
//public:
//  static constexpr int FOREVER = INT_MAX;
//  explicit TimerThread(Searcher* s);
//  // 待機時間だけ待ったのち思考時間のチェックを行う
//  virtual void idleLoop();
//  // 思考スレッドの監視を始める
//  // firstMs: 初回待機時間
//  // afterMs: 次回以降の待機時間
//  void restartTimer(int firstMs, int afterMs);
//
//private:
//  // 初回の待機時間
//  // FOREVERの場合は思考時間のチェックを行わない
//  std::atomic<int> timerPeriodFirstMs;
//  // 次回以降回の待機時間
//  // FOREVERの場合は思考時間のチェックを行わない
//  std::atomic<int> timerPeriodAfterMs;
//  // 初回の待機時間を使用する場合は true
//  // そうでない場合は false
//  std::atomic<int> first;
//};
//
//class ThreadPool : public std::vector<Thread*> {
//public:
//  void init(Searcher* s);
//  void exit();
//
//  MainThread* mainThread() { return static_cast<MainThread*>((*this)[0]); }
//  Depth minSplitDepth() const { return minimumSplitDepth_; }
//  TimerThread* timerThread() { return timer_; }
//  void wakeUp(Searcher* s);
//  void sleep();
//  void readUSIOptions(Searcher* s);
//  Thread* availableSlave(Thread* master) const;
//  void setTimer(const int msec);
//  void waitForThinkFinished();
//  void startThinking(
//    const Position& pos,
//    const LimitsType& limits,
//    const std::vector<Move>& searchMoves,
//    const std::chrono::time_point<std::chrono::system_clock>& goReceivedTime);
//
//  bool sleepWhileIdle_;
//  size_t maxThreadsPerSplitPoint_;
//  Mutex mutex_;
//  ConditionVariable sleepCond_;
//
//private:
//  TimerThread* timer_;
//  Depth minimumSplitDepth_;
//};

#endif // #ifndef APERY_THREAD_HPP
