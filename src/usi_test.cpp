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
  USI::setPosition(pos, std::istringstream("startpos moves 7g7f 3c3d 2g2f 4a3b 6i7h 1c1d 3i4h 8c8d 6g6f 8d8e 8h7g 7a6b 4i5h 5c5d 7i8h 2b3c 5i6i 3a2b 5h6g 3c4b 7g5i 8e8f 8g8f 4b8f 8i7g 8f4b P*8g 5a4a 5g5f 2b3c 4h5g"));
  USI::go(pos, std::istringstream("btime 0 wtime 0 byoyomi 2000"));
  Threads.main()->wait_for_search_finished();
}

TEST_F(UsiTest, go_withoutMoves) {
  Position pos;
  USI::setPosition(pos, std::istringstream("startpos moves 7g7f 3c3d 2g2f 4a3b 6i7h 1c1d 3i4h 8c8d 6g6f 8d8e 8h7g 7a6b 4i5h 5c5d 7i8h 2b3c 5i6i 3a2b 5h6g 3c4b 7g5i 8e8f 8g8f 4b8f 8i7g 8f4b P*8g 5a4a 5g5f 2b3c 4h5g"));
  USI::go(pos, std::istringstream("btime 0 wtime 0 byoyomi 2000"));
  Threads.main()->wait_for_search_finished();
}
