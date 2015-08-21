#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include "hayabusa.hpp"

using namespace std;
using namespace std::tr2::sys;

static const path INPUT_DIRECTORY_PATH = "../../testdata";
static const path OUTPUT_DIRECTORY_PATH = "../../temp";

class HayabusaTest : public testing::Test {
public:
  HayabusaTest() {}
  virtual ~HayabusaTest() {}
protected:
  virtual void SetUp() {
    remove_all(OUTPUT_DIRECTORY_PATH);
    ASSERT_TRUE(create_directories(OUTPUT_DIRECTORY_PATH));
  }
  virtual void TearDown() {
    ASSERT_TRUE(remove_all(OUTPUT_DIRECTORY_PATH));
  }
private:
};

TEST_F(HayabusaTest, createEvaluationCache_evaluatesPositions) {
  EXPECT_TRUE(hayabusa::createEvaluationCache(
    INPUT_DIRECTORY_PATH, OUTPUT_DIRECTORY_PATH, 1000));

  path outputFilePath = OUTPUT_DIRECTORY_PATH
    / "wdoor+floodgate-600-10+01WishBlue_07+Apery_i5-4670+20150415003002.csa";
  ifstream ifs(outputFilePath);
  EXPECT_TRUE(ifs.is_open());

  int score;
  EXPECT_TRUE(ifs >> score);
  EXPECT_TRUE(ifs >> score);
  EXPECT_TRUE(ifs >> score);
}
