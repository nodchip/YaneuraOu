#include <filesystem>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "csa.hpp"
#include "thread.hpp"

using namespace std;
using namespace std::tr2::sys;

static const path TEMP_OUTPUT_DIRECTORY_PATH = "../temp";

class CsaTest : public testing::Test {
public:
  CsaTest() {}
  virtual ~CsaTest() {}
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

TEST_F(CsaTest, toPositions_convertCsaFileToPositions) {
  path filepath = "../src/testdata/csa/wdoor+floodgate-600-10+01WishBlue_07+Apery_i5-4670+20150415003002.csa";

  vector<string> sfen;
  EXPECT_TRUE(csa::toSfen(filepath, sfen));

  EXPECT_EQ(136, sfen.size());
}

TEST_F(CsaTest, toPositions_convertShogidokoroCsaFileToPositions) {
  path filepath = "../src/testdata/shogidokoro/csa/20150823_214751 tanuki- sse4.2 msvc  vs Apery sse4.2 msvc .csa";

  vector<string> sfen;
  EXPECT_TRUE(csa::toSfen(filepath, sfen));

  EXPECT_EQ(134, sfen.size());
}

TEST_F(CsaTest, isFinished_returnTrueIfFinished) {
  path filepath = "../src/testdata/shogidokoro/csa/20150823_214751 tanuki- sse4.2 msvc  vs Apery sse4.2 msvc .csa";

  EXPECT_TRUE(csa::isFinished(filepath));
}

TEST_F(CsaTest, isFinished_returnFalseIfNotFinished) {
  path filepath = "../src/testdata/shogidokoro/csa/not_finished.csa";

  EXPECT_FALSE(csa::isFinished(filepath));
}

TEST_F(CsaTest, isTanukiBlack_returnTrueIfTanikiIsBlack) {
  path filepath = "../src/testdata/shogidokoro/csa/20150823_214751 tanuki- sse4.2 msvc  vs Apery sse4.2 msvc .csa";

  EXPECT_TRUE(csa::isTanukiBlack(filepath));
}

TEST_F(CsaTest, isTanukiBlack_returnFalseIfTanikiIsWhite) {
  path filepath = "../src/testdata/shogidokoro/csa/20150823_215301 Apery sse4.2 msvc  vs tanuki- sse4.2 msvc .csa";

  EXPECT_FALSE(csa::isTanukiBlack(filepath));
}

TEST_F(CsaTest, isBlackWin_returnTrueIfBlackIsWin) {
  path filepath = "../src/testdata/shogidokoro/csa/20150823_215301 Apery sse4.2 msvc  vs tanuki- sse4.2 msvc .csa";

  EXPECT_EQ(Black, csa::getWinner(filepath));
}

TEST_F(CsaTest, isBlackWin_returnTrueIfWhiteIsWin) {
  path filepath = "../src/testdata/shogidokoro/csa/20150823_214751 tanuki- sse4.2 msvc  vs Apery sse4.2 msvc .csa";

  EXPECT_EQ(White, csa::getWinner(filepath));
}

TEST_F(CsaTest, convertCsaToSfen_convertCsaToSfen) {
  path inputDirectoryPath = "../src/testdata/csa";
  path outputFilePath = TEMP_OUTPUT_DIRECTORY_PATH / "temp.sfen";

  EXPECT_TRUE(csa::convertCsaToSfen(
    inputDirectoryPath,
    outputFilePath));

  ifstream ifs(outputFilePath);
  EXPECT_TRUE(ifs.is_open());
  string line;
  EXPECT_TRUE(getline(ifs, line));
  EXPECT_FALSE(getline(ifs, line));
}

TEST_F(CsaTest, convertCsa1LineToSfen_convert) {
  path inputFilePath = "../src/testdata/csa1line/utf82chkifu.csa";
  path outputFilePath = TEMP_OUTPUT_DIRECTORY_PATH / "temp.sfen";

  EXPECT_TRUE(csa::convertCsa1LineToSfen(
    inputFilePath,
    outputFilePath));

  ifstream ifs(outputFilePath);
  EXPECT_TRUE(ifs.is_open());
  string line;
  EXPECT_TRUE(getline(ifs, line));
  EXPECT_TRUE(getline(ifs, line));
  EXPECT_TRUE(getline(ifs, line));
  EXPECT_FALSE(getline(ifs, line));
}

TEST_F(CsaTest, readCsa_convert) {
  path inputFilePath = "../src/testdata/csa/wdoor+floodgate-600-10+01WishBlue_07+Apery_i5-4670+20150415003002.csa";
  GameRecord gameRecord;

  EXPECT_TRUE(csa::readCsa(inputFilePath, gameRecord));

  EXPECT_EQ(0, gameRecord.gameRecordIndex);
  EXPECT_EQ("??/??/??", gameRecord.date);
  EXPECT_EQ("01WishBlue_07", gameRecord.blackPlayerName);
  EXPECT_EQ("Apery_i5-4670", gameRecord.whitePlayerName);
  EXPECT_EQ(2, gameRecord.winner);
  EXPECT_EQ(134, gameRecord.numberOfPlays);
  EXPECT_EQ("???", gameRecord.leagueName);
  EXPECT_EQ("???", gameRecord.strategy);
  EXPECT_EQ(134, gameRecord.moves.size());
}

TEST_F(CsaTest, readCsas_filterReturnsTrue) {
  // wdoor+floodgate-600-10+01WishBlue_07+Apery_i5-4670+20150415003002.csa
  path inputDirectoryPath = "../src/testdata/csa";
  std::vector<GameRecord> gameRecords;

  EXPECT_TRUE(csa::readCsas(inputDirectoryPath, [](const path& p) {
    return p.string().find("+Apery_i5-4670+") != std::string::npos;
  }, gameRecords));

  EXPECT_EQ(1, gameRecords.size());
}

TEST_F(CsaTest, readCsas_filterReturnsFalse) {
  // wdoor+floodgate-600-10+01WishBlue_07+Apery_i5-4670+20150415003002.csa
  path inputDirectoryPath = "../src/testdata/csa";
  std::vector<GameRecord> gameRecords;

  EXPECT_TRUE(csa::readCsas(inputDirectoryPath, [](const path& p) {
    return false;
  }, gameRecords));

  EXPECT_EQ(0, gameRecords.size());
}

TEST_F(CsaTest, readCsa1_convert) {
  path inputFilePath = "../src/testdata/csa1line/utf82chkifu.csa";
  std::vector<GameRecord> gameRecords;

  EXPECT_TRUE(csa::readCsa1(inputFilePath, gameRecords));

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

TEST_F(CsaTest, writeCsa1_convert) {
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

  EXPECT_TRUE(csa::writeCsa1(outputFilePath, gameRecords));

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

TEST_F(CsaTest, mergeCsa1s_convert) {
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

  EXPECT_TRUE(csa::writeCsa1(outputFilePath, gameRecords));

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

TEST_F(CsaTest, mergeCsa1s_merge) {
  path inputFilePath = "../src/testdata/csa1line/utf82chkifu.csa";
  path outputFilePath = TEMP_OUTPUT_DIRECTORY_PATH / "temp.csa1";

  EXPECT_TRUE(csa::mergeCsa1s({ inputFilePath, inputFilePath }, outputFilePath));

  ifstream ifs(outputFilePath);
  for (int loop = 0; loop < 12; ++loop) {
    string line;
    EXPECT_TRUE(getline(ifs, line));
  }
  string line;
  EXPECT_FALSE(getline(ifs, line));
}
