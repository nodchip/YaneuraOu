#include "timeManager.hpp"
#include "book.hpp"

#include "csa1.hpp"
#include "game_record.hpp"
#include "move.hpp"
#include "position.hpp"
#include "search.hpp"
#include "thread.hpp"
#include "time_util.hpp"
#include "usi.hpp"

std::mt19937_64 Book::mt64bit_; // 定跡のhash生成用なので、seedは固定でデフォルト値を使う。
BookKey Book::ZobPiece[PieceNone][SquareNum];
BookKey Book::ZobHand[HandPieceNum][19]; // 持ち駒の同一種類の駒の数ごと
BookKey Book::ZobTurn;

void Book::init() {
  for (Piece p = Empty; p < PieceNone; ++p) {
    for (Square sq = I9; sq < SquareNum; ++sq) {
      ZobPiece[p][sq] = mt64bit_();
    }
  }
  for (HandPiece hp = HPawn; hp < HandPieceNum; ++hp) {
    for (int num = 0; num < 19; ++num) {
      ZobHand[hp][num] = mt64bit_();
    }
  }
  ZobTurn = mt64bit_();
}

bool Book::open(const char* fName) {
  if (fileName_ == fName) {
    return true;
  }

  std::ifstream ifs(fName, std::ifstream::in | std::ifstream::binary | std::ifstream::ate);
  if (!ifs.is_open()) {
    SYNCCOUT << "info string Failed to open book file (0): " << fName << SYNCENDL;
    return false;
  }

  int numberOfEntries = ifs.tellg() / sizeof(BookEntry);
  if (!ifs.seekg(0)) {
    SYNCCOUT << "info string Failed to seek in the book file: " << fName << SYNCENDL;
    return false;
  }
  std::vector<BookEntry> entries(numberOfEntries);
  if (!ifs.read((char*)&entries[0], sizeof(BookEntry) * numberOfEntries)) {
    SYNCCOUT << "info string Failed to read the book file: " << fName << SYNCENDL;
    return false;
  }

  entries_.clear();
  for (const auto& entry : entries) {
    entries_.insert(std::make_pair(entry.key, entry));
  }

  fileName_ = fName;
  return true;
}

BookKey Book::bookKey(const Position& pos) {
  BookKey key = 0;
  Bitboard bb = pos.occupiedBB();

  while (bb.isNot0()) {
    const Square sq = bb.firstOneFromI9();
    key ^= ZobPiece[pos.piece(sq)][sq];
  }
  const Hand hand = pos.hand(pos.turn());
  for (HandPiece hp = HPawn; hp < HandPieceNum; ++hp) {
    key ^= ZobHand[hp][hand.numOf(hp)];
  }
  if (pos.turn() == White) {
    key ^= ZobTurn;
  }
  return key;
}

std::tuple<Move, Score> Book::probe(const Position& pos, const std::string& fName, const bool pickBest) {
  u16 best = 0;
  u32 sum = 0;
  Move move = Move::moveNone();
  const BookKey key = bookKey(pos);
  const Score min_book_score = static_cast<Score>(static_cast<int>(Options[USI::OptionNames::MIN_BOOK_SCORE]));
  Score score = ScoreZero;

  if (!open(fName.c_str())) {
    return std::make_tuple(Move::moveNone(), ScoreNone);
  }

  // 現在の局面における定跡手の数だけループする。
  auto range = entries_.equal_range(key);
  for (auto it = range.first; it != range.second; ++it) {
    const BookEntry& entry = it->second;
    best = std::max(best, entry.count);
    sum += entry.count;

    // 指された確率に従って手が選択される。
    // count が大きい順に並んでいる必要はない。
    if (min_book_score <= entry.score
      && ((random_() % sum < entry.count)
        || (pickBest && entry.count == best)))
    {
      const Move tmp = Move(entry.fromToPro);
      const Square to = tmp.to();
      if (tmp.isDrop()) {
        const PieceType ptDropped = tmp.pieceTypeDropped();
        move = makeDropMove(ptDropped, to);
      }
      else {
        const Square from = tmp.from();
        const PieceType ptFrom = pieceToPieceType(pos.piece(from));
        const bool promo = tmp.isPromotion() != 0;
        if (promo) {
          move = makeCapturePromoteMove(ptFrom, from, to, pos);
        }
        else {
          move = makeCaptureMove(ptFrom, from, to, pos);
        }
      }
      score = entry.score;
    }
  }

  return std::make_tuple(move, score);
}

std::vector<std::pair<Move, int> > Book::enumerateMoves(const Position& pos, const std::string& fName)
{
  if (!open(fName.c_str())) {
    return{};
  }

  const BookKey key = bookKey(pos);

  // 現在の局面における定跡手の数だけループする。
  std::vector<std::pair<Move, int> > moves;
  auto range = entries_.equal_range(key);
  for (auto it = range.first; it != range.second; ++it) {
    const BookEntry& entry = it->second;
    const Move tmp = Move(entry.fromToPro);
    const Square to = tmp.to();
    Move move;
    if (tmp.isDrop()) {
      const PieceType ptDropped = tmp.pieceTypeDropped();
      move = makeDropMove(ptDropped, to);
    }
    else {
      const Square from = tmp.from();
      const PieceType ptFrom = pieceToPieceType(pos.piece(from));
      const bool promo = tmp.isPromotion() != 0;
      if (promo) {
        move = makeCapturePromoteMove(ptFrom, from, to, pos);
      }
      else {
        move = makeCaptureMove(ptFrom, from, to, pos);
      }
    }

    if (Options[USI::OptionNames::MIN_BOOK_SCORE]) {
      if (moves.empty()) {
        moves.push_back(std::make_pair(move, it->second.score));
      }
      else if (it->second.score > moves.front().second) {
        moves[0] = std::make_pair(move, it->second.score);
      }
    }
    else {
      moves.push_back(std::make_pair(move, it->second.count));
    }
  }

  return moves;
}

inline bool countCompare(const BookEntry& b1, const BookEntry& b2) {
  return b1.count < b2.count;
}

#if !defined MINIMUL
// 以下のようなフォーマットが入力される。
// <棋譜番号> <日付> <先手名> <後手名> <0:引き分け, 1:先手勝ち, 2:後手勝ち> <総手数> <棋戦名前> <戦形>
// <CSA1行形式の指し手>
//
// (例)
// 1 2003/09/08 羽生善治 谷川浩司 2 126 王位戦 その他の戦型
// 7776FU3334FU2726FU4132KI
//
// 勝った方の手だけを定跡として使うこととする。
// 出現回数がそのまま定跡として使う確率となる。
// 基本的には棋譜を丁寧に選別した上で定跡を作る必要がある。
// MAKE_SEARCHED_BOOK を on にしていると、定跡生成に非常に時間が掛かる。
void makeBook(Position& pos, std::istringstream& ssCmd) {
  std::string options[] = {
    "name Threads value 4",
    "name MultiPV value 1",
    "name OwnBook value false",
    "name Max_Random_Score_Diff value 0",
    "name USI_Hash value 32",
    "name Use_Sleeping_Threads value true",
    "name Output_Info value false",
  };
  for (auto& option : options) {
    USI::setOption(option);
  }

  std::string fileName;
  ssCmd >> fileName;
  std::vector<GameRecord> gameRecords;
  if (!csa::readCsa1(fileName, pos, gameRecords)) {
    return;
  }

  // bookMap[BookKey, proFromTo] -> BookEntry
  std::map<std::pair<BookKey, u16>, BookEntry> bookMap;

  double startClockSec = clock() / double(CLOCKS_PER_SEC);
  int gameRecordIndex = 0;
  for (const auto& gameRecord : gameRecords) {
    if (++gameRecordIndex % 1000 == 0) {
      std::cout << time_util::formatRemainingTime(
        startClockSec, gameRecordIndex, gameRecords.size());
    }
    Color winner = gameRecord.winner == 1 ? Black : gameRecord.winner == 2 ? White : ColorNum;
    // 勝った方の指し手を記録していく。
    // 又は稲庭戦法側を記録していく。
    const Color saveColor = winner;

    if (winner == ColorNum) {
      continue;
    }

    //printf("winner=%s\n", winner == Black ? "Black" : "White");

    pos.set(USI::DefaultStartPositionSFEN, Threads.main());
    Search::StateStackPtr SetUpStates = Search::StateStackPtr(new std::stack<StateInfo>());
    for (const auto& move : gameRecord.moves) {
      if (move.isNone()) {
        //pos.print();
        std::cout << "!!! Illegal move = !!!" << std::endl;
        break;
      }

      if (pos.turn() != saveColor) {
        SetUpStates->push(StateInfo());
        pos.doMove(move, SetUpStates->top());
        continue;
      }

      // 先手、後手の内、片方だけを記録する。
      BookKey key = Book::bookKey(pos);
      if (bookMap.count(std::make_pair(key, move.proFromAndTo()))) {
        if (++bookMap[std::make_pair(key, move.proFromAndTo())].count == 0) {
          // 数えられる数の上限を超えたので元に戻す。
          --bookMap[std::make_pair(key, move.proFromAndTo())].count;
        }
      }
      else {
        // 未登録の手
        BookEntry be;
        be.score = static_cast<Score>(0);
        be.key = key;
        be.fromToPro = static_cast<u16>(move.proFromAndTo());
        be.count = 1;
        bookMap[std::make_pair(key, move.proFromAndTo())] = be;
      }

      SetUpStates->push(StateInfo());
      pos.doMove(move, SetUpStates->top());
    }
  }

  std::map<BookKey, std::vector<BookEntry> > bookKeyToEntries;
  for (const auto& p : bookMap) {
    bookKeyToEntries[p.first.first].push_back(p.second);
  }

  int numberOfEntries = 0;
  std::map<BookKey, std::vector<BookEntry> > bookReduced;
  for (const auto& p : bookKeyToEntries) {
    if (p.second.size() <= 1) {
      // 候補手が一手しか存在しないエントリーを削除する
      continue;
    }
    bookReduced.insert(p);
    numberOfEntries += p.second.size();
  }

  // BookEntry::count の値で降順にソート
  for (auto& elem : bookReduced) {
    std::sort(elem.second.rbegin(), elem.second.rend(), countCompare);
  }

  // Scoreをつけていく
  int entryIndex = 0;
  std::set<BookKey> recordedKeys;
  startClockSec = clock() / double(CLOCKS_PER_SEC);
  for (const auto& gameRecord : gameRecords) {
    pos.set(USI::DefaultStartPositionSFEN, Threads.main());
    Search::StateStackPtr SetUpStates = Search::StateStackPtr(new std::stack<StateInfo>());
    for (const auto& move : gameRecord.moves) {
      if (move.isNone()) {
        //pos.print();
        std::cout << "!!! Illegal move = !!!" << std::endl;
        break;
      }

      BookKey key = Book::bookKey(pos);
      if (recordedKeys.count(key) || !bookReduced.count(key)) {
        SetUpStates->push(StateInfo());
        pos.doMove(move, SetUpStates->top());
        continue;
      }

      for (auto& entry : bookReduced[key]) {
        Move entryMove = move16toMove(entry.fromToPro, pos);
        SetUpStates->push(StateInfo());
        pos.doMove(entryMove, SetUpStates->top());
        USI::go(pos, "depth 3");
        Threads.main()->wait_for_search_finished();
        pos.undoMove(entryMove);
        SetUpStates->pop();
        // doMove してから search してるので点数が反転しているので直す。
        entry.score = -Threads.main()->rootMoves[0].score;
        //pos.print();
        printf("score=%d\n", entry.score);

        ++entryIndex;
        std::cout << time_util::formatRemainingTime(
          startClockSec, entryIndex, numberOfEntries);
      }

      recordedKeys.insert(key);

      SetUpStates->push(StateInfo());
      pos.doMove(move, SetUpStates->top());
    }
  }

  std::ofstream ofs("../bin/book-2016-01-02.bin", std::ios::binary);
  for (auto& elem : bookReduced) {
    for (auto& elel : elem.second) {
      ofs.write(reinterpret_cast<char*>(&(elel)), sizeof(BookEntry));
    }
  }

  std::cout << "book making was done" << std::endl;
}
#endif
