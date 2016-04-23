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
