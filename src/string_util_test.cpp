#include <gtest/gtest.h>
#include "string_util.hpp"

using namespace std;

class StringUtilTest : public testing::Test {
public:
  StringUtilTest() {}
  virtual ~StringUtilTest() {}
protected:
  virtual void SetUp() { }
  virtual void TearDown() { }
private:
};

TEST_F(StringUtilTest, split_simpleExample) {
  string input = "a b c";
  vector<string> expected = { "a", "b", "c" };

  vector<string> actual = string_util::split(input);
  EXPECT_EQ(expected, actual);
}

TEST_F(StringUtilTest, concat_simpleExample) {
  vector<string> input = { "a", "b", "c" };
  string expected = "a b c";

  string actual = string_util::concat(input);
  EXPECT_EQ(expected, actual);
}
