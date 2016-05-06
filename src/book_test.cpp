#include "timeManager.hpp"
#include "book.hpp"

#include <gtest/gtest.h>

#include "thread.hpp"
#include "usi.hpp"

using namespace std;

class BookTest : public testing::Test {
public:
  BookTest() {}
  virtual ~BookTest() {}
protected:
  virtual void SetUp() { }
  virtual void TearDown() { }
private:
};

TEST_F(BookTest, prove_hirate) {
  Position pos;
  pos.set(USI::DefaultStartPositionSFEN, Threads.main());

  Book book;
  auto moveAndScore = book.probe(pos);
  ASSERT_NE(Move::moveNone(), moveAndScore.first);
}
