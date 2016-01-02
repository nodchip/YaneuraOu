#include "square.hpp"

Direction SquareRelation[SquareNum][SquareNum];

// 何かの駒で一手で行ける位置関係についての距離のテーブル。桂馬の位置は距離1とする。
int SquareDistance[SquareNum][SquareNum];

Rank makeRank(const Square s) {
  assert(isInSquare(s));
  return SquareToRank[s];
}

File makeFile(const Square s) {
  assert(isInSquare(s));
  return SquareToFile[s];
}
