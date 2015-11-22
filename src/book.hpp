#ifndef APERY_BOOK_HPP
#define APERY_BOOK_HPP

#include "position.hpp"

struct BookEntry {
  Key key;
  u16 fromToPro;
  u16 count;
  Score score;
};

class Book
{
public:
  Book() : random_(std::random_device()()) {}
  std::tuple<Move, Score> probe(const Position& pos, const std::string& fName, const bool pickBest);
  std::vector<std::pair<Move, int> > enumerateMoves(const Position& pos, const std::string& fName);
  static void init();
  static Key bookKey(const Position& pos);
  bool open(const char* fName);

private:

  static std::mt19937_64 mt64bit_; // 定跡のhash生成用なので、seedは固定でデフォルト値を使う。
  std::mt19937_64 random_; // ハードウェア乱数をseedにして色々指すようにする。
  std::string fileName_;
  std::unordered_multimap<Key, BookEntry> entries_;

  static Key ZobPiece[PieceNone][SquareNum];
  static Key ZobHand[HandPieceNum][19];
  static Key ZobTurn;
};

void makeBook(Position& pos, std::istringstream& ssCmd);

#endif // #ifndef APERY_BOOK_HPP
