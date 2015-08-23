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
