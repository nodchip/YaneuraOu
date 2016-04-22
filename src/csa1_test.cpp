#include "timeManager.hpp"
#include <filesystem>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "csa1.hpp"
#include "thread.hpp"

using namespace std;
using namespace std::tr2::sys;

static const path TEMP_OUTPUT_DIRECTORY_PATH = "../temp";

class Csa1Test : public testing::Test {
public:
  Csa1Test() {}
  virtual ~Csa1Test() {}
protected:
  virtual void SetUp() {
    remove_all(TEMP_OUTPUT_DIRECTORY_PATH);
    create_directories(TEMP_OUTPUT_DIRECTORY_PATH);
  }
  virtual void TearDown() {
    remove_all(TEMP_OUTPUT_DIRECTORY_PATH);
  }
private:
};

TEST_F(Csa1Test, readCsa1_convert) {
  path inputFilePath = "../src/testdata/csa1line/utf82chkifu.csa";
  std::vector<GameRecord> gameRecords;

  Position pos;
  EXPECT_TRUE(csa::readCsa1(inputFilePath.string(), pos, gameRecords));

  EXPECT_EQ(1, gameRecords[0].gameRecordIndex);
  EXPECT_EQ("2003/09/08", gameRecords[0].date);
  EXPECT_EQ("âHê∂ëPé°", gameRecords[0].blackPlayerName);
  EXPECT_EQ("íJêÏç_éi", gameRecords[0].whitePlayerName);
  EXPECT_EQ(2, gameRecords[0].winner);
  EXPECT_EQ(126, gameRecords[0].numberOfPlays);
  EXPECT_EQ("â§à êÌ", gameRecords[0].leagueName);
  EXPECT_EQ("ÇªÇÃëºÇÃêÌå^", gameRecords[0].strategy);
  EXPECT_EQ(126, gameRecords[0].moves.size());
  EXPECT_EQ(3, gameRecords.size());
}

TEST_F(Csa1Test, writeCsa1_convert) {
  path outputFilePath = TEMP_OUTPUT_DIRECTORY_PATH / "temp.csa1";
  GameRecord gameRecord;
  gameRecord.gameRecordIndex = 1;
  gameRecord.date = "2015/11/02";
  gameRecord.blackPlayerName = "hoge";
  gameRecord.whitePlayerName = "fuga";
  gameRecord.winner = 1;
  gameRecord.numberOfPlays = 1;
  gameRecord.leagueName = "foo";
  gameRecord.strategy = "bar";
  gameRecord.moves.push_back(Move(73275));
  std::vector<GameRecord> gameRecords = { gameRecord };

  EXPECT_TRUE(csa::writeCsa1(outputFilePath.string(), gameRecords));

  ifstream ifs(outputFilePath);
  string line0, line1, line2;
  EXPECT_TRUE(getline(ifs, line0));
  EXPECT_TRUE(getline(ifs, line1));
  EXPECT_FALSE(getline(ifs, line2));
  EXPECT_NE(string::npos, line0.find("2015/11/02"));
  EXPECT_NE(string::npos, line0.find("hoge"));
  EXPECT_NE(string::npos, line0.find("fuga"));
  EXPECT_NE(string::npos, line0.find("foo"));
  EXPECT_NE(string::npos, line0.find("bar"));
  EXPECT_EQ("7776FU", line1);
}

TEST_F(Csa1Test, mergeCsa1s_convert) {
  path outputFilePath = TEMP_OUTPUT_DIRECTORY_PATH / "temp.csa1";
  GameRecord gameRecord;
  gameRecord.gameRecordIndex = 1;
  gameRecord.date = "2015/11/02";
  gameRecord.blackPlayerName = "hoge";
  gameRecord.whitePlayerName = "fuga";
  gameRecord.winner = 1;
  gameRecord.numberOfPlays = 1;
  gameRecord.leagueName = "foo";
  gameRecord.strategy = "bar";
  gameRecord.moves.push_back(Move(73275));
  std::vector<GameRecord> gameRecords = { gameRecord };

  EXPECT_TRUE(csa::writeCsa1(outputFilePath.string(), gameRecords));

  ifstream ifs(outputFilePath);
  string line0, line1, line2;
  EXPECT_TRUE(getline(ifs, line0));
  EXPECT_TRUE(getline(ifs, line1));
  EXPECT_FALSE(getline(ifs, line2));
  EXPECT_NE(string::npos, line0.find("2015/11/02"));
  EXPECT_NE(string::npos, line0.find("hoge"));
  EXPECT_NE(string::npos, line0.find("fuga"));
  EXPECT_NE(string::npos, line0.find("foo"));
  EXPECT_NE(string::npos, line0.find("bar"));
  EXPECT_EQ("7776FU", line1);
}

TEST_F(Csa1Test, mergeCsa1s_merge) {
  path inputFilePath = "../src/testdata/csa1line/utf82chkifu.csa";
  path outputFilePath = TEMP_OUTPUT_DIRECTORY_PATH / "temp.csa1";

  Position pos;
  EXPECT_TRUE(csa::mergeCsa1s(
  { inputFilePath.string(), inputFilePath.string() },
    outputFilePath.string(),
    pos));

  ifstream ifs(outputFilePath);
  for (int loop = 0; loop < 12; ++loop) {
    string line;
    EXPECT_TRUE(getline(ifs, line));
  }
  string line;
  EXPECT_FALSE(getline(ifs, line));
}
