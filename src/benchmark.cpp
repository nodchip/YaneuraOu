#include "benchmark.hpp"
#include "common.hpp"
#include "generateMoves.hpp"
#include "position.hpp"
#include "search.hpp"
#include "usi.hpp"
#include "tt.hpp"

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

// ベンチマークを行う。特定の局面で特定の深さを探索窓の大きさを変えて比較する。
void benchmarkSearchWindow(Position& pos) {
  std::string token;
  LimitsType limits;

  std::string options[] = {
    "name Threads value 1",
    "name MultiPV value 1",
    "name OwnBook value false",
    "name Max_Random_Score_Diff value 0",
    "name USI_Hash value " USI_HASH_FOR_BENCHMARK,
    "name Use_Sleeping_Threads value true",
  };
  for (const auto& option : options) {
    pos.searcher()->setOption(option);
  }

  std::ifstream ifs("benchmark.sfen");
  std::string sfen;
  std::getline(ifs, sfen);
  std::cout << sfen << std::endl;

  Score expected = Score(-2655);

  for (Score delta = Score(16); delta <= 1024; delta += 16) {
    setPosition(pos, sfen);

    Score alpha = expected - delta;
    Score beta = expected + delta;

    SearchStack searchStack[MaxPlyPlus2];
    memset(searchStack, 0, sizeof(searchStack));
    searchStack[0].currentMove = Move::moveNull(); // skip update gains
    searchStack[0].staticEvalRaw.p[0][0] = ScoreNotEvaluated;
    searchStack[1].currentMove = Move::moveNull(); // skip update gains
    searchStack[1].staticEvalRaw.p[0][0] = ScoreNotEvaluated;
    searchStack[2].staticEvalRaw.p[0][0] = ScoreNotEvaluated;

    pos.searcher()->threads.waitForThinkFinished();
    pos.searcher()->signals.stopOnPonderHit = pos.searcher()->signals.firstRootMove = false;
    pos.searcher()->signals.stop = pos.searcher()->signals.failedLowAtRoot = false;
    pos.searcher()->rootPosition = pos;
    pos.searcher()->limits.set(limits);
    pos.searcher()->rootMoves.clear();

    const MoveType MT = Legal;
    for (MoveList<MT> ml(pos); !ml.end(); ++ml) {
        pos.searcher()->rootMoves.push_back(RootMove(ml.move()));
    }

    //pos.searcher()->tt.clear();

    Time time = Time::currentTime();
    
    Score actual = pos.searcher()->search<Root>(pos, &searchStack[2], alpha, beta, static_cast<Depth>(17 * OnePly), false);
    printf("delta=%d alpha=%d beta=%d expected=%d actual=%d elapsed=%d %s\n", delta, alpha, beta, expected, actual, time.elapsed(),
      (alpha < actual && actual < beta) ? "*" : "");
  }
}

// 指し手生成の速度を計測
void benchmarkGenerateMoves(Position& pos) {
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

  std::ifstream ifs("benchmark.sfen");
  std::string sfen;
  u64 sumOfSearchedNodes = 0;
  int sumOfSeaerchTimeMs = 0;
  int depthIndex = 0;
  while (std::getline(ifs, sfen)) {
    //std::cout << sfen << std::endl;
    setPosition(pos, sfen);
    //pos.print();

    MoveStack legalMoves[1024];
    for (int i = 0; i < sizeof(legalMoves) / sizeof(MoveStack); ++i) legalMoves[i].move = moveNone();
    MoveStack* pms = &legalMoves[0];
    const u64 num = 10000000;
    Time t = Time::currentTime();
    if (pos.inCheck()) {
      for (u64 i = 0; i < num; ++i) {
        pms = &legalMoves[0];
        pms = generateMoves<Evasion>(pms, pos);
      }
    }
    else {
      for (u64 i = 0; i < num; ++i) {
        pms = &legalMoves[0];
        //pms = generateMoves<CapturePlusPro>(pms, pos);
        //pms = generateMoves<NonCaptureMinusPro>(pms, pos);
        pms = generateMoves<Drop>(pms, pos);
        //			pms = generateMoves<PseudoLegal>(pms, pos);
        //			pms = generateMoves<Legal>(pms, pos);
      }
    }
    const int elapsed = t.elapsed();
    //std::cout << "elapsed = " << elapsed << " [msec]" << std::endl;
    if (elapsed != 0) {
      std::cout << "times/s = " << num * 1000 / elapsed << " [times/sec]" << std::endl;
    }
    //const ptrdiff_t count = pms - &legalMoves[0];
    //std::cout << "num of moves = " << count << std::endl;
    //for (int i = 0; i < count; ++i) {
    //  std::cout << legalMoves[i].move.toCSA() << ", ";
    //}
    //std::cout << std::endl;
  }
}
