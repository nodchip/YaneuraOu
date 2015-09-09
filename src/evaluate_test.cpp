#include <gtest/gtest.h>
#include "position.hpp"
#include "search.hpp"
#include "string_util.hpp"
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

TEST_F(EvaluateTest, evaluate_endToEnd) {
  vector<string> sfen = string_util::split("startpos moves 7g7f 3c3d 2g2f 4a3b 6i7h 1c1d 3i4h 8c8d 6g6f 8d8e 8h7g 7a6b 4i5h 5c5d 7i8h 2b3c 5i6i 3a2b 5h6g 3c4b 7g5i 8e8f 8g8f 4b8f 8i7g 8f4b P*8g 5a4a 5g5f 2b3c 4h5g 7c7d 3g3f 4b6d 2h1h 3c4d 5g4f 6b5c 6i7i 4a3a 7i8i 9c9d 1g1f 5c4b 2i3g 8b5b 3g4e 5b5a 5i4h 5d5e 6f6e 6d7c 5f5e 4d5e 4f5e 5a5e 7g8e 7c9e S*5f 5e5a P*5c S*4d 4g4f P*5e 5f4g 9e6b 6e6d 6a7b 2f2e 6c6d 1f1e 1d1e 4h1e P*1g 1h1g P*1d 1e4b+ 3a4b 5c5b+ 4b5b P*5d 5b6c S*5c 6c5d 5c6b+ 7b6b B*8b P*8d 8b9a+ 8d8e L*8d 8a9c 9a9b B*3i 9b9c 4d4e 1g2g N*6f 6g6f 3i6f+ P*6g 6f5g N*6i 5g4h 8d8c+ 4h9c 8c9c 8e8f 4f4e N*9e B*8d S*7c 8d7c+ 6b7c S*6b 8f8g+ 6b5a 8g8h 7h8h B*6h 8i7h B*5i R*5b S*5c 6g6f P*8g");

  for (int i = 2; i <= sfen.size(); ++i) {
    string subSfen = string_util::concat(vector<string>(sfen.begin(), sfen.begin() + i));
    std::istringstream ss_sfen(subSfen);
    Position pos;
    setPosition(pos, ss_sfen);

    SearchStack searchStack[MaxPlyPlus2];
    memset(searchStack, 0, sizeof(searchStack));
    searchStack[0].currentMove = Move::moveNull(); // skip update gains
    searchStack[0].staticEvalRaw = (Score)INT_MAX;
    searchStack[1].currentMove = Move::moveNull(); // skip update gains
    searchStack[1].staticEvalRaw = (Score)INT_MAX;
    searchStack[2].staticEvalRaw = (Score)INT_MAX;

    Score actual = evaluate(pos, &searchStack[2]);

    Score expected = evaluateUnUseDiff(pos) / FVScale;
    EXPECT_EQ(expected, actual);
  }
}
