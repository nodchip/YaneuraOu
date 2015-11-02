#ifndef GAME_RECORD_HPP
#define GAME_RECORD_HPP

#include <string>
#include <vector>

// 以下のようなフォーマットが入力される。
// <棋譜番号> <日付> <先手名> <後手名> <0:引き分け, 1:先手勝ち, 2:後手勝ち> <総手数> <棋戦名前> <戦形>
// <CSA1行形式の指し手>
//
// (例)
// 1 2003/09/08 羽生善治 谷川浩司 2 126 王位戦 その他の戦型
// 7776FU3334FU2726FU4132KI
struct GameRecord
{
  // 棋譜番号
  int gameRecordIndex;
  // 日付
  std::string date;
  // 先手名
  std::string blackPlayerName;
  // 後手名
  std::string whitePlayerName;
  // 0:引き分け, 1:先手勝ち, 2:後手勝ち
  int winner;
  // 総手数
  int numberOfPlays;
  // 棋戦名前
  std::string leagueName;
  // 戦形
  std::string strategy;
  // 指し手
  std::vector<Move> moves;
};

#endif
