#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "csa.hpp"
#include "thread.hpp"

using namespace std;

class CsaTest : public testing::Test {
public:
  CsaTest() {}
  virtual ~CsaTest() {}
protected:
  virtual void SetUp() {}
  virtual void TearDown() {}
private:
};

TEST_F(CsaTest, toPositions_convertCsaFileToPositions) {
  string filepath = "../src/testdata/csa/wdoor+floodgate-600-10+01WishBlue_07+Apery_i5-4670+20150415003002.csa";

  vector<string> sfen;
  EXPECT_TRUE(csa::toSfen(filepath, sfen));

  EXPECT_EQ(136, sfen.size());
}

TEST_F(CsaTest, toPositions_convertShogidokoroCsaFileToPositions) {
  string filepath = "../src/testdata/shogidokoro/csa/20150823_214751 tanuki- sse4.2 msvc  vs Apery sse4.2 msvc .csa";

  vector<string> sfen;
  EXPECT_TRUE(csa::toSfen(filepath, sfen));

  EXPECT_EQ(134, sfen.size());
}

TEST_F(CsaTest, isFinished_returnTrueIfFinished) {
  string filepath = "../src/testdata/shogidokoro/csa/20150823_214751 tanuki- sse4.2 msvc  vs Apery sse4.2 msvc .csa";

  EXPECT_TRUE(csa::isFinished(filepath));
}

TEST_F(CsaTest, isFinished_returnFalseIfNotFinished) {
  string filepath = "../src/testdata/shogidokoro/csa/not_finished.csa";

  EXPECT_FALSE(csa::isFinished(filepath));
}

TEST_F(CsaTest, isTanukiBlack_returnTrueIfTanikiIsBlack) {
  string filepath = "../src/testdata/shogidokoro/csa/20150823_214751 tanuki- sse4.2 msvc  vs Apery sse4.2 msvc .csa";

  EXPECT_TRUE(csa::isTanukiBlack(filepath));
}

TEST_F(CsaTest, isTanukiBlack_returnFalseIfTanikiIsWhite) {
  string filepath = "../src/testdata/shogidokoro/csa/20150823_215301 Apery sse4.2 msvc  vs tanuki- sse4.2 msvc .csa";

  EXPECT_FALSE(csa::isTanukiBlack(filepath));
}

TEST_F(CsaTest, isBlackWin_returnTrueIfBlackIsWin) {
  string filepath = "../src/testdata/shogidokoro/csa/20150823_215301 Apery sse4.2 msvc  vs tanuki- sse4.2 msvc .csa";

  EXPECT_TRUE(csa::isBlackWin(filepath));
}

TEST_F(CsaTest, isBlackWin_returnTrueIfWhiteIsWin) {
  string filepath = "../src/testdata/shogidokoro/csa/20150823_214751 tanuki- sse4.2 msvc  vs Apery sse4.2 msvc .csa";

  EXPECT_FALSE(csa::isBlackWin(filepath));
}
