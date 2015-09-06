#ifndef EVALLIST_HPP
#define EVALLIST_HPP

#include "common.hpp"
#include "piece.hpp"
#include "square.hpp"

class Position;

struct EvalList {
	static const int ListSize = 38;

  ALIGNED32(int list0[ListSize]);
  ALIGNED32(int list1[ListSize]);
	Square listToSquareHand[ListSize];
	int squareHandToList[SquareHandNum];

	void set(const Position& pos);
};

extern const Square HandPieceToSquareHand[ColorNum][HandPieceNum];

#endif // #ifndef EVALLIST_HPP
