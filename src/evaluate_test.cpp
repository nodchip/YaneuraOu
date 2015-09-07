#include <gtest/gtest.h>
#include "evaluate.hpp"
#include "thread.hpp"
#include "usi.hpp"

using namespace std;

class EvaluateTest : public testing::Test {
public:
  EvaluateTest() {}
  virtual ~EvaluateTest() {}
protected:
  virtual void SetUp() { }
  virtual void TearDown() { }
private:
};

void setPosition(Position& pos, std::istringstream& ssCmd);

TEST_F(EvaluateTest, evaluate_withoutDiffBlack) {
  std::istringstream ss_sfen("startpos moves");
  Position pos(DefaultStartPositionSFEN, g_threads.mainThread());
  setPosition(pos, ss_sfen);

  SearchStack searchStack[MaxPlyPlus2];
  memset(searchStack, 0, sizeof(searchStack));
  searchStack[0].currentMove = Move::moveNull(); // skip update gains
  searchStack[0].staticEvalRaw = (Score)INT_MAX;
  searchStack[1].staticEvalRaw = (Score)INT_MAX;

  Score score = evaluate(pos, &searchStack[1]);

  EXPECT_EQ(0, score);
}

TEST_F(EvaluateTest, evaluate_withoutDiffWhite) {
  std::istringstream ss_sfen("startpos moves 7g7f");
  Position pos(DefaultStartPositionSFEN, g_threads.mainThread());
  setPosition(pos, ss_sfen);

  SearchStack searchStack[MaxPlyPlus2];
  memset(searchStack, 0, sizeof(searchStack));
  searchStack[0].currentMove = Move::moveNull(); // skip update gains
  searchStack[0].staticEvalRaw = (Score)INT_MAX;
  searchStack[1].staticEvalRaw = (Score)INT_MAX;

  Score score = evaluate(pos, &searchStack[1]);

  EXPECT_EQ(-76, score);
}

TEST_F(EvaluateTest, evaluate_withDiffBlack) {
  std::istringstream ss_sfen("startpos moves 7g7f");
  Position pos(DefaultStartPositionSFEN, g_threads.mainThread());
  setPosition(pos, ss_sfen);

  SearchStack searchStack[MaxPlyPlus2];
  memset(searchStack, 0, sizeof(searchStack));
  searchStack[0].currentMove = Move::moveNull(); // skip update gains
  searchStack[0].staticEvalRaw = (Score)INT_MAX;
  searchStack[1].staticEvalRaw = (Score)INT_MAX;
  searchStack[2].staticEvalRaw = (Score)INT_MAX;

  evaluate(pos, &searchStack[1]);
  StateInfo stateInfo;
  pos.doMove(usiToMove(pos, "3c3d"), stateInfo);

  Score score = evaluate(pos, &searchStack[2]);

  EXPECT_EQ(0, score);
}

TEST_F(EvaluateTest, evaluate_withDiffWhite) {
  std::istringstream ss_sfen("startpos moves 7g7f");
  Position pos(DefaultStartPositionSFEN, g_threads.mainThread());
  setPosition(pos, ss_sfen);
  StateInfo stateInfo[2];
  pos.doMove(usiToMove(pos, "3c3d"), stateInfo[0]);
  pos.doMove(usiToMove(pos, "2g2f"), stateInfo[1]);

  SearchStack searchStack[MaxPlyPlus2];
  memset(searchStack, 0, sizeof(searchStack));
  searchStack[0].currentMove = Move::moveNull(); // skip update gains
  searchStack[0].staticEvalRaw = (Score)INT_MAX;
  searchStack[1].staticEvalRaw = (Score)INT_MAX;

  Score score = evaluate(pos, &searchStack[1]);

  EXPECT_EQ(-87, score);
}
