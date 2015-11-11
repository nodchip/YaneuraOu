#include "benchmark.hpp"
#include "common.hpp"
#include "position.hpp"
#include "search.hpp"

#ifndef USI_HASH_FOR_BENCHMARK
#define USI_HASH_FOR_BENCHMARK "8192"
#endif

// ベンチマークを行う。出力値は1秒辺りに探索した盤面の数。
void benchmark(Position& pos) {
  std::string token;
  LimitsType limits;

  std::string options[] = {
    "name Threads value 4",
    "name MultiPV value 1",
    "name OwnBook value false",
    "name Max_Random_Score_Diff value 0",
    "name USI_Hash value " USI_HASH_FOR_BENCHMARK,
    "name Use_Sleeping_Threads value true",
  };
  for (const auto& option : options) {
    pos.searcher()->setOption(option);
  }

  pos.searcher()->recordIterativeDeepningScores = false;
  std::ifstream ifs("benchmark.sfen");
  std::string sfen;
  u64 sumOfSearchedNodes = 0;
  int sumOfSeaerchTimeMs = 0;
  while (std::getline(ifs, sfen)) {
    setPosition(pos, sfen);
    go(pos, "byoyomi 10000");
    pos.searcher()->threads.waitForThinkFinished();

    sumOfSearchedNodes += pos.searcher()->lastSearchedNodes;
    sumOfSeaerchTimeMs += pos.searcher()->searchTimer.elapsed();
  }

  SYNCCOUT << "info nodes " << sumOfSearchedNodes * 1000 / sumOfSeaerchTimeMs
    << " time " << 1000 << SYNCENDL;
}

// ベンチマークを行う。出力は特定の深さまで読むのにかかった時間。
void benchmarkElapsedForDepthN(Position& pos) {
  std::string token;
  LimitsType limits;

  std::string options[] = {
    "name Threads value 4",
    "name MultiPV value 1",
    "name OwnBook value false",
    "name Max_Random_Score_Diff value 0",
    "name USI_Hash value " USI_HASH_FOR_BENCHMARK,
    "name Use_Sleeping_Threads value true",
  };
  for (const auto& option : options) {
    pos.searcher()->setOption(option);
  }

  const char* depths[] = {
    "depth 15",
    "depth 21",
    "depth 15",
    "depth 15",
    "depth 15",
  };

  pos.searcher()->recordIterativeDeepningScores = false;
  std::ifstream ifs("benchmark.sfen");
  std::string sfen;
  u64 sumOfSearchedNodes = 0;
  int sumOfSeaerchTimeMs = 0;
  int depthIndex = 0;
  while (std::getline(ifs, sfen)) {
    std::cout << sfen << std::endl;
    setPosition(pos, sfen);
    go(pos, depths[depthIndex++]);
    pos.searcher()->threads.waitForThinkFinished();

    sumOfSearchedNodes += pos.searcher()->lastSearchedNodes;
    sumOfSeaerchTimeMs += pos.searcher()->searchTimer.elapsed();
  }

  SYNCCOUT << "info nodes " << sumOfSearchedNodes
    << " time " << sumOfSeaerchTimeMs << SYNCENDL;
}
