#include "generateMoves.hpp"
#include "timeManager.hpp"
#include "thread.hpp"
#include "usi.hpp"

using namespace Search;

// Global object
ThreadPool Threads;

namespace
{
  Position rootPositions[MaxThreads];
}

/// Thread constructor launch the thread and then wait until it goes to sleep
/// in idle_loop().

Thread::Thread(Position& rootPos) : rootPos(rootPos) {

  resetCalls = exit = false;
  maxPly = callsCnt = 0;
  history.clear();
  gains.clear();
  idx = Threads.size(); // Start from 0

  std::unique_lock<Mutex> lk(mutex);
  searching = true;
  nativeThread = std::thread(&Thread::idle_loop, this);
  sleepCondition.wait(lk, [&] { return !searching; });
}


/// Thread destructor wait for thread termination before returning

Thread::~Thread() {

  mutex.lock();
  exit = true;
  sleepCondition.notify_one();
  mutex.unlock();
  nativeThread.join();
}


/// Thread::wait_for_search_finished() wait on sleep condition until not searching

void Thread::wait_for_search_finished() {

  std::unique_lock<Mutex> lk(mutex);
  sleepCondition.wait(lk, [&] { return !searching; });
}


/// Thread::wait() wait on sleep condition until condition is true

void Thread::wait(std::atomic_bool& condition) {

  std::unique_lock<Mutex> lk(mutex);
  sleepCondition.wait(lk, [&] { return bool(condition); });
}


/// Thread::start_searching() wake up the thread that will start the search

void Thread::start_searching(bool resume) {

  std::unique_lock<Mutex> lk(mutex);

  if (!resume)
    searching = true;

  sleepCondition.notify_one();
}


/// Thread::idle_loop() is where the thread is parked when it has no work to do

void Thread::idle_loop() {

  while (!exit)
  {
    std::unique_lock<Mutex> lk(mutex);

    searching = false;

    while (!searching && !exit)
    {
      sleepCondition.notify_one(); // Wake up any waiting thread
      sleepCondition.wait(lk);
    }

    lk.unlock();

    if (!exit)
      search();
  }
}


/// ThreadPool::init() create and launch requested threads, that will go
/// immediately to sleep. We cannot use a constructor because Threads is a
/// static object and we need a fully initialized engine at this point due to
/// allocation of Endgames in the Thread constructor.

void ThreadPool::init() {

  push_back(new MainThread(rootPositions[0]));
  read_usi_options();
}


/// ThreadPool::exit() terminate threads before the program exits. Cannot be
/// done in destructor because threads must be terminated before deleting any
/// static objects, so while still in main().

void ThreadPool::exit() {

  while (size())
    delete back(), pop_back();
}


/// ThreadPool::read_uci_options() updates internal threads parameters from the
/// corresponding UCI options and creates/destroys threads to match requested
/// number. Thread objects are dynamically allocated.

void ThreadPool::read_usi_options() {

  size_t requested = Options[USI::OptionNames::THREADS];

  assert(requested > 0);

  while (size() < requested)
    push_back(new Thread(rootPositions[size()]));

  while (size() > requested)
    delete back(), pop_back();
}


/// ThreadPool::nodes_searched() return the number of nodes searched

int64_t ThreadPool::nodes_searched() {

  int64_t nodes = 0;
  for (Thread* th : *this)
    nodes += th->rootPos.nodesSearched();
  return nodes;
}


/// ThreadPool::start_thinking() wake up the main thread sleeping in idle_loop()
/// and start a new search, then return immediately.

void ThreadPool::start_thinking(const Position& pos, const LimitsType& limits,
  StateStackPtr& states) {
  assert(limits.startTime != 0);

  main()->wait_for_search_finished();

  Signals.stopOnPonderhit = Signals.stop = false;

  main()->rootMoves.clear();
  main()->rootPos = pos;
  Limits = limits;
  if (states.get()) // If we don't set a new position, preserve current state
  {
    SetupStates = std::move(states); // Ownership transfer here
    //assert(!states.get());
  }

  const MoveType MT = Legal;
  for (MoveList<MT> ml(pos); !ml.end(); ++ml) {
    if (limits.searchmoves.empty()
      || std::find(limits.searchmoves.begin(), limits.searchmoves.end(), ml.move()) != limits.searchmoves.end())
    {
      main()->rootMoves.push_back(RootMove(ml.move()));
    }
  }

  main()->start_searching();
}

//#include "generateMoves.hpp"
//#include "search.hpp"
//#include "thread.hpp"
//#include "usi.hpp"
//
//namespace {
//  template <typename T> T* newThread(Searcher* s) {
//    T* th = new T(s);
//    th->handle = std::thread(&Thread::idleLoop, th); // move constructor
//    return th;
//  }
//  void deleteThread(Thread* th) {
//    th->exit = true;
//    th->notifyOne();
//    th->handle.join(); // Wait for thread termination
//    delete th;
//  }
//}
//
/////////////////////////////////////////////////////////////////////////////////
//// Thread
/////////////////////////////////////////////////////////////////////////////////
//Thread::Thread(Searcher* s) /*: splitPoints()*/ {
//  searcher = s;
//  exit = false;
//  searching = false;
//  splitPointsSize = 0;
//  maxPly = 0;
//  activeSplitPoint = nullptr;
//  activePosition = nullptr;
//  idx = s->threads.size();
//}
//
//void Thread::notifyOne() {
//  std::unique_lock<Mutex> lock(sleepLock);
//  sleepCond.notify_one();
//}
//
//bool Thread::cutoffOccurred() const {
//  for (SplitPoint* sp = activeSplitPoint; sp != nullptr; sp = sp->parentSplitPoint) {
//    if (sp->cutoff) {
//      return true;
//    }
//  }
//  return false;
//}
//
//// master と同じ thread であるかを判定
//bool Thread::isAvailableTo(Thread* master) const {
//  if (searching) {
//    return false;
//  }
//
//  // ローカルコピーし、途中で値が変わらないようにする。
//  const int spCount = splitPointsSize;
//  return !spCount || (splitPoints[spCount - 1].slavesMask & (UINT64_C(1) << master->idx));
//}
//
//void Thread::waitFor(const std::atomic<bool>& b) {
//  std::unique_lock<Mutex> lock(sleepLock);
//  sleepCond.wait(lock, [&]() -> bool { return b; });
//}
//
//void ThreadPool::init(Searcher* s) {
//  sleepWhileIdle_ = true;
//#if defined LEARN
//#else
//  timer_ = newThread<TimerThread>(s);
//#endif
//  push_back(newThread<MainThread>(s));
//  readUSIOptions(s);
//}
//
//void ThreadPool::exit() {
//#if defined LEARN
//#else
//  // checkTime() がデータにアクセスしないよう、先に timer_ を delete
//  deleteThread(timer_);
//#endif
//
//  for (auto elem : *this)
//    deleteThread(elem);
//}
//
//void ThreadPool::readUSIOptions(Searcher* s) {
//  maxThreadsPerSplitPoint_ = USI::Options[OptionNames::MAX_THREADS_PER_SPLIT_POINT];
//  const size_t requested = USI::Options[OptionNames::THREADS];
//  minimumSplitDepth_ = (requested < 6 ? 4 : (requested < 8 ? 5 : 7)) * OnePly;
//
//  assert(0 < requested);
//
//  while (size() < requested) {
//    push_back(newThread<Thread>(s));
//  }
//
//  while (requested < size()) {
//    deleteThread(back());
//    pop_back();
//  }
//}
//
//Thread* ThreadPool::availableSlave(Thread* master) const {
//  for (auto elem : *this) {
//    if (elem->isAvailableTo(master)) {
//      return elem;
//    }
//  }
//  return nullptr;
//}
//
//void ThreadPool::setTimer(const int msec) {
//  timer_->maxPly = msec;
//  timer_->notifyOne(); // Wake up and restart the timer
//}
//
//void ThreadPool::waitForThinkFinished() {
//  MainThread* t = mainThread();
//  std::unique_lock<Mutex> lock(t->sleepLock);
//  sleepCond_.wait(lock, [&] { return !(t->thinking); });
//}
//
//void ThreadPool::startThinking(
//  const Position& pos,
//  const LimitsType& limits,
//  const std::vector<Move>& searchMoves,
//  const std::chrono::time_point<std::chrono::system_clock>& goReceivedTime)
//{
//#if defined LEARN
//#else
//  waitForThinkFinished();
//#endif
//  pos.searcher()->searchTimer.set(goReceivedTime);
//  pos.searcher()->signals.stopOnPonderHit = pos.searcher()->signals.firstRootMove = false;
//  pos.searcher()->signals.stop = pos.searcher()->signals.failedLowAtRoot = false;
//  pos.searcher()->signals.skipMainThreadCurrentDepth = false;
//  pos.searcher()->broadcastedPvDepth = -1;
//  pos.searcher()->broadcastedPvInfo.clear();
//  pos.searcher()->mainThreadCurrentSearchDepth = 0;
//
//  pos.searcher()->rootPosition = pos;
//  pos.searcher()->limits.set(limits);
//  pos.searcher()->rootMoves.clear();
//
//#if defined LEARN
//  // searchMoves を直接使う。
//  pos.searcher()->rootMoves.push_back(RootMove(searchMoves[0]));
//#else
//  const MoveType MT = Legal;
//  for (MoveList<MT> ml(pos); !ml.end(); ++ml) {
//    if (searchMoves.empty()
//      || std::find(searchMoves.begin(), searchMoves.end(), ml.move()) != searchMoves.end())
//    {
//      pos.searcher()->rootMoves.push_back(RootMove(ml.move()));
//    }
//  }
//#endif
//
//#if defined LEARN
//  // 浅い探索なので、thread 生成、破棄のコストが高い。余分な thread を生成せずに直接探索を呼び出す。
//  pos.searcher()->think();
//#else
//  mainThread()->thinking = true;
//  mainThread()->notifyOne();
//#endif
//}
//
//template <bool Fake>
//void Thread::split(Position& pos, SearchStack* ss, const Score alpha, const Score beta, Score& bestScore,
//  Move& bestMove, const Depth depth, const Move threatMove, const int moveCount,
//  MovePicker& mp, const NodeType nodeType, const bool cutNode)
//{
//  assert(pos.isOK());
//  assert(bestScore <= alpha && alpha < beta && beta <= ScoreInfinite);
//  assert(-ScoreInfinite < bestScore);
//  assert(searcher->threads.minSplitDepth() <= depth);
//
//  assert(searching);
//  assert(splitPointsSize < MaxSplitPointsPerThread);
//
//  SplitPoint& sp = splitPoints[splitPointsSize];
//
//  sp.masterThread = this;
//  sp.parentSplitPoint = activeSplitPoint;
//  sp.slavesMask = UINT64_C(1) << idx;
//  sp.depth = depth;
//  sp.bestMove = bestMove;
//  sp.threatMove = threatMove;
//  sp.alpha = alpha;
//  sp.beta = beta;
//  sp.nodeType = nodeType;
//  sp.cutNode = cutNode;
//  sp.bestScore = bestScore;
//  sp.movePicker = &mp;
//  sp.moveCount = moveCount;
//  sp.pos = &pos;
//  sp.nodes = 0;
//  sp.cutoff = false;
//  sp.ss = ss;
//
//  searcher->threads.mutex_.lock();
//  sp.mutex.lock();
//
//  ++splitPointsSize;
//  activeSplitPoint = &sp;
//  activePosition = nullptr;
//
//  // thisThread が常に含まれるので 1
//  size_t slavesCount = 1;
//  Thread* slave;
//
//  while ((slave = searcher->threads.availableSlave(this)) != nullptr
//    && ++slavesCount <= searcher->threads.maxThreadsPerSplitPoint_ && !Fake)
//  {
//    sp.slavesMask |= UINT64_C(1) << slave->idx;
//    slave->activeSplitPoint = &sp;
//    slave->searching = true;
//    slave->notifyOne();
//  }
//
//  if (1 < slavesCount || Fake) {
//    sp.mutex.unlock();
//    searcher->threads.mutex_.unlock();
//    Thread::idleLoop();
//    assert(!searching);
//    assert(!activePosition);
//    searcher->threads.mutex_.lock();
//    sp.mutex.lock();
//  }
//
//  searching = true;
//  --splitPointsSize;
//  activeSplitPoint = sp.parentSplitPoint;
//  activePosition = &pos;
//  pos.setNodesSearched(pos.nodesSearched() + sp.nodes);
//  bestMove = sp.bestMove;
//  bestScore = sp.bestScore;
//
//  searcher->threads.mutex_.unlock();
//  sp.mutex.unlock();
//}
//
//template void Thread::split<true >(Position& pos, SearchStack* ss, const Score alpha, const Score beta, Score& bestScore,
//  Move& bestMove, const Depth depth, const Move threatMove, const int moveCount,
//  MovePicker& mp, const NodeType nodeType, const bool cutNode);
//template void Thread::split<false>(Position& pos, SearchStack* ss, const Score alpha, const Score beta, Score& bestScore,
//  Move& bestMove, const Depth depth, const Move threatMove, const int moveCount,
//  MovePicker& mp, const NodeType nodeType, const bool cutNode);
//
/////////////////////////////////////////////////////////////////////////////////
//// TimerThread
/////////////////////////////////////////////////////////////////////////////////
//TimerThread::TimerThread(Searcher* s) :
//  Thread(s),
//  timerPeriodFirstMs(FOREVER),
//  timerPeriodAfterMs(FOREVER),
//  first(true)
//{
//}
//
//void TimerThread::idleLoop() {
//  while (!exit) {
//    int timerPeriodMs = first ? timerPeriodFirstMs : timerPeriodAfterMs;
//    first = false;
//    {
//      std::unique_lock<Mutex> lock(sleepLock);
//      if (!exit) {
//        sleepCond.wait_for(lock, std::chrono::milliseconds(timerPeriodMs));
//      }
//    }
//    if (timerPeriodMs != FOREVER) {
//      searcher->checkTime();
//    }
//  }
//}
//
//void TimerThread::restartTimer(int firstMs, int afterMs)
//{
//  // TODO(nodchip): スレッド競合に対処する
//  this->timerPeriodFirstMs = firstMs;
//  this->timerPeriodAfterMs = afterMs;
//  first = true;
//  notifyOne();
//}
//
/////////////////////////////////////////////////////////////////////////////////
//// MainThread
/////////////////////////////////////////////////////////////////////////////////
//void MainThread::idleLoop() {
//  while (true) {
//    {
//      std::unique_lock<Mutex> lock(sleepLock);
//      thinking = false;
//      while (!thinking && !exit) {
//        // UI 関連だから要らないのかも。
//        searcher->threads.sleepCond_.notify_one();
//        sleepCond.wait(lock);
//      }
//    }
//
//    if (exit) {
//      return;
//    }
//
//    searching = true;
//    searcher->think();
//    assert(searching);
//    searching = false;
//  }
//}
