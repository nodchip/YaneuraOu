#include <gtest/gtest.h>
#include "scanner.hpp"

using namespace std;

class ScannerTest : public testing::Test {
public:
  ScannerTest() {}
  virtual ~ScannerTest() {}
protected:
  virtual void SetUp() { }
  virtual void TearDown() { }
private:
};

TEST_F(ScannerTest, hasNext_returnFalseIfQIsEmpty) {
  Scanner scanner;

  EXPECT_FALSE(scanner.hasNext());
}

TEST_F(ScannerTest, hasNext_returnTrueIfQIsNotEmpty) {
  Scanner scanner;
  scanner.SetInput("a b c");

  EXPECT_TRUE(scanner.hasNext());
}

TEST_F(ScannerTest, hasNextInt_returnFalseIfQIsEmpty) {
  Scanner scanner;

  EXPECT_FALSE(scanner.hasNextInt());
}

TEST_F(ScannerTest, hasNextInt_returnTrueIfFrontIsZero) {
  Scanner scanner;
  scanner.SetInput("0 1 2");

  EXPECT_TRUE(scanner.hasNextInt());
}

TEST_F(ScannerTest, hasNextInt_returnTrueIfForntIsNotZero) {
  Scanner scanner;
  scanner.SetInput("1 2 3");

  EXPECT_TRUE(scanner.hasNextInt());
}

TEST_F(ScannerTest, hasNextChar_returnTrueIfQIsNotEmpty) {
  Scanner scanner;
  scanner.SetInput("1 2 3");

  EXPECT_TRUE(scanner.hasNextChar());
}

TEST_F(ScannerTest, hasNextChar_returnTrueIfSpace) {
  Scanner scanner;
  scanner.SetInput("1 2 3");

  ASSERT_TRUE(scanner.hasNextChar());
  EXPECT_TRUE(scanner.hasNextChar());
}

TEST_F(ScannerTest, hasNextChar_returnFalseIfEndOfLine) {
  Scanner scanner;
  scanner.SetInput("1");

  ASSERT_TRUE(scanner.hasNextChar());
  ASSERT_EQ('1', scanner.nextChar());
  EXPECT_FALSE(scanner.hasNextChar());
}

TEST_F(ScannerTest, next_returnFromFront) {
  Scanner scanner;
  scanner.SetInput("hoge fuga");

  EXPECT_EQ("hoge", scanner.next());
  EXPECT_EQ("fuga", scanner.next());
}

TEST_F(ScannerTest, nextChar_returnFromFront) {
  Scanner scanner;
  scanner.SetInput("hoge fuga");

  EXPECT_EQ('h', scanner.nextChar());
  EXPECT_EQ('o', scanner.nextChar());
}

TEST_F(ScannerTest, nextChar_returnSpaceAfterWord) {
  Scanner scanner;
  scanner.SetInput("a b");

  ASSERT_EQ('a', scanner.nextChar());
  EXPECT_EQ(' ', scanner.nextChar());
}

TEST_F(ScannerTest, nextInt_returnFromFront) {
  Scanner scanner;
  scanner.SetInput("1 2 3");

  EXPECT_EQ(1, scanner.nextInt());
  EXPECT_EQ(2, scanner.nextInt());
}
