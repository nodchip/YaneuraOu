#include "timeManager.hpp"
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
  USI::setPosition(pos, std::istringstream("startpos moves"));
  //USI::go(pos, std::istringstream("btime 0 wtime 0 byoyomi 2000"));
  USI::go(pos, std::istringstream("btime 0 wtime 0 infinite"));
  Threads.main()->wait_for_search_finished();
}

TEST_F(UsiTest, go_withoutMoves) {
  Position pos;
  USI::setPosition(pos, std::istringstream("startpos"));
  USI::go(pos, std::istringstream("btime 0 wtime 0 byoyomi 2000"));
  Threads.main()->wait_for_search_finished();
}
