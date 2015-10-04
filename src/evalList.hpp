#ifndef APERY_EVALLIST_HPP
#define APERY_EVALLIST_HPP

#include "square.hpp"
#include "piece.hpp"

class Position;

struct EvalList {
  static constexpr int ListSize = 38;

  ALIGNED32(int list0[ListSize]);
  int list1[ListSize];
  Square listToSquareHand[ListSize];
  int squareHandToList[SquareHandNum];

  void set(const Position& pos);
};

constexpr Square HandPieceToSquareHand[ColorNum][HandPieceNum] = {
  { B_hand_pawn, B_hand_lance, B_hand_knight, B_hand_silver, B_hand_gold, B_hand_bishop, B_hand_rook },
  { W_hand_pawn, W_hand_lance, W_hand_knight, W_hand_silver, W_hand_gold, W_hand_bishop, W_hand_rook }
};

#endif // #ifndef APERY_EVALLIST_HPP
