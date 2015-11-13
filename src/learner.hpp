#ifndef APERY_LEARNER_HPP
#define APERY_LEARNER_HPP

#include "evaluate.hpp"
#include "position.hpp"
#include "search.hpp"
#include "thread.hpp"

#if defined LEARN

#if 0
#define PRINT_PV
#endif

struct LearnEvaluater : public EvaluaterBase<float, float, float> {
  float kpp_raw[SquareNum][fe_end][fe_end];
  float kkp_raw[SquareNum][SquareNum][fe_end];
  float kk_raw[SquareNum][SquareNum];

  void incParam(const Position& pos, const double dinc);

  // kpp_raw, kkp_raw, kk_raw の値を低次元の要素に与える。
  void lowerDimension();

  void clear();
};

LearnEvaluater& operator += (LearnEvaluater& lhs, LearnEvaluater& rhs);

struct Parse2Data {
  LearnEvaluater params;

  void clear();
};

// 以下のようなフォーマットが入力される。
// <棋譜番号> <日付> <先手名> <後手名> <0:引き分け, 1:先手勝ち, 2:後手勝ち> <総手数> <棋戦名前> <戦形>
// <CSA1行形式の指し手>
//
// (例)
// 1 2003/09/08 羽生善治 谷川浩司 2 126 王位戦 その他の戦型
// 7776FU3334FU2726FU4132KI
struct BookMoveData {
  // その手を指した人
  std::string player;
  // 対局日
  std::string date;
  // 正解のPV, その他0のPV, その他1のPV という順に並べる。
  // 間には MoveNone で区切りを入れる。
  std::vector<Move> pvBuffer;

  // 指し手
  Move move;
  // 正解の手が何番目に良い手か。0から数える。
  int recordIsNth;
  // 勝ったかどうか
  bool winner;
  // 学習に使うかどうか
  bool useLearning;
  // 棋譜の手と近い点数の手があったか。useLearning == true のときだけ有効な値が入る
  bool otherPVExist;
};

class Learner {
public:
  void learn(Position& pos, std::istringstream& ssCmd);

private:
  // 学習に使う棋譜から、手と手に対する補助的な情報を付けでデータ保持する。
  // 50000局程度に対して10秒程度で終わるからシングルコアで良い。
  void setLearnMoves(Position& pos, std::set<std::pair<Key, Move> >& dict, std::string& s0, std::string& s1);
  void readBook(Position& pos, std::istringstream& ssCmd);
  void setLearnOptions(Searcher& s);
  template <bool Dump> size_t lockingIndexIncrement();
  void learnParse1Body(Position& pos, std::mt19937& mt);
  void learnParse1(Position& pos);
  static constexpr double FVPenalty();
  template <typename T>
  void updateFV(T& v, float dv);
  void copyFromKppkkpkkToOneArray();
  void updateEval(const std::string& dirName);
  double sigmoid(const double x) const;
  double dsigmoid(const double x) const;
  void learnParse2Body(Position& pos, Parse2Data& parse2Data);
  void learnParse2(Position& pos);
  void print();

  std::mutex mutex_;
  size_t index_;
  Ply minDepth_;
  Ply maxDepth_;
  std::mt19937 mt_;
  std::vector<std::mt19937> mts_;
  std::vector<Position> positions_;
  std::vector<std::vector<BookMoveData> > bookMovesDatum_;
  Parse2Data parse2Data_;
  std::vector<Parse2Data> parse2Datum_;
  Evaluater eval_;

  static constexpr Score FVWindow = static_cast<Score>(200);
};

#endif

#endif // #ifndef APERY_LEARNER_HPP
