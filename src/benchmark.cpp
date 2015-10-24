#include "benchmark.hpp"
#include "common.hpp"
#include "limits_type.hpp"
#include "position.hpp"
#include "scanner.hpp"
#include "search.hpp"
#include "usi.hpp"

// 今はベンチマークというより、PGO ビルドの自動化の為にある。
void benchmark(Position& pos) {
  std::string token;
  LimitsType limits;

  std::string options[] = {
    "name Threads value 4",
    "name MultiPV value 1",
    "name OwnBook value false",
    "name Max_Random_Score_Diff value 0",
    "name USI_Hash value 8192",
  };
  for (auto& str : options) {
    pos.searcher()->setOption(str);
  }

  Searcher::recordIterativeDeepningScores = false;
  std::ifstream ifs("benchmark.sfen");
  std::string sfen;
  u64 sumOfSearchedNodes = 0;
  int sumOfSeaerchTimeMs = 0;
  while (std::getline(ifs, sfen)) {
    std::cout << sfen << std::endl;
    setPosition(pos, sfen);
    go(pos, "byoyomi 10000");
    pos.searcher()->threads.waitForThinkFinished();

    sumOfSearchedNodes += Searcher::lastSearchedNodes;
    sumOfSeaerchTimeMs += Searcher::searchTimer.elapsed();
  }

  SYNCCOUT << "info nodes " << sumOfSearchedNodes * 1000 / sumOfSeaerchTimeMs
    << " time " << 1000 << SYNCENDL;
}
