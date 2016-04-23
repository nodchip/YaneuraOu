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

#endif // #ifndef APERY_THREAD_HPP
