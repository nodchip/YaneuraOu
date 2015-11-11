#include <gtest/gtest.h>
#include "search.hpp"
#include "usi.hpp"

using namespace std;

class UsiTest : public testing::Test {
public:
  UsiTest() {}
  virtual ~UsiTest() {}
protected:
  virtual void SetUp() { }
  virtual void TearDown() { }
private:
};

TEST_F(UsiTest, go_startSearch) {
  Position pos;
  setPosition(pos, std::istringstream("startpos moves"));
  go(pos, std::istringstream("btime 0 wtime 0 byoyomi 2000"));
  pos.searcher()->threads.waitForThinkFinished();
}

TEST_F(UsiTest, go_withoutMoves) {
  Position pos;
  setPosition(pos, std::istringstream("startpos"));
  go(pos, std::istringstream("btime 0 wtime 0 byoyomi 2000"));
  pos.searcher()->threads.waitForThinkFinished();
}
