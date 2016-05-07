#include "book.hpp"
#include "csa1.hpp"
#include "game_record.hpp"
#include "move.hpp"
#include "position.hpp"
#include "search.hpp"
#include "thread.hpp"
#include "time_util.hpp"
#include "usi.hpp"
#include "generateMoves.hpp"

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

namespace
{
  // moveの上位16bitを補完する
  Move complementMove(u16 fromToPro, const Position& pos) {
    Move raw = Move(fromToPro);
    Square to = raw.to();
    Move move = Move::moveNone();
    if (raw.isDrop()) {
      PieceType ptDropped = raw.pieceTypeDropped();
      return makeDropMove(ptDropped, to);
    }

    Square from = raw.from();
    PieceType ptFrom = pieceToPieceType(pos.piece(from));
    bool promo = raw.isPromotion() != 0;
    if (promo) {
      return makeCapturePromoteMove(ptFrom, from, to, pos);
    }
    else {
      return makeCaptureMove(ptFrom, from, to, pos);
    }
  }
}

std::pair<Move, Score> Book::probe(const Position& pos) {
  std::string bookFilePath = USI::Options[OptionNames::BOOK_FILE];
  bool bestBookMove = USI::Options[OptionNames::BEST_BOOK_MOVE] != 0;
  bool ownBook = USI::Options[OptionNames::OWNBOOK] != 0;
  int minBookPly = USI::Options[OptionNames::MIN_BOOK_PLY];
  int maxBookPly = USI::Options[OptionNames::MAX_BOOK_PLY];
  int minBookSscore = USI::Options[OptionNames::MIN_BOOK_SCORE];
  int maxRandomScoreDiff = USI::Options[OptionNames::MAX_RANDOM_SCORE_DIFF];
  int maxRandomScoreDiffPly = USI::Options[OptionNames::MAX_RANDOM_SCORE_DIFF_PLY];

  // 定跡データベースを使用しない場合はmoveNoneを返す
  if (!ownBook) {
    return std::make_pair(Move::moveNone(), ScoreNone);
  }

  // 定跡データベースを開けない場合もmoveNoneを返す
  if (!open(bookFilePath.c_str())) {
    return std::make_pair(Move::moveNone(), ScoreNone);
  }

  // 定跡データベースを使用しない手数の場合もmoveNoneを返す
  std::uniform_int_distribution<int> dist(minBookPly, maxBookPly);
  Ply bookPly = dist(g_randomTimeSeed);
  if (pos.gamePly() > bookPly) {
    return std::make_pair(Move::moveNone(), ScoreNone);
  }

  int scoreDiff;
  if (bestBookMove) {
    // 最も良いスコアの手を指す場合
    scoreDiff = 0;
  }
  else if (maxRandomScoreDiffPly < pos.gamePly()) {
    // 第一候補手以外を指す手数を過ぎた場合
    scoreDiff = 0;
  }
  else {
    // それ以外の場合
    scoreDiff = maxRandomScoreDiffPly;
  }

  std::vector<BookEntry> entries;
  BookKey key = bookKey(pos);
  Score bestScore = -ScoreInfinite;

  // 現在の局面における定跡手の数だけループする。
  auto range = entries_.equal_range(key);
  for (auto it = range.first; it != range.second; ++it) {
    const BookEntry& entry = it->second;
    if (entry.score < minBookSscore) {
      continue;
    }
    if (!MoveList<LegalAll>(pos).contains(complementMove(entry.fromToPro, pos))) {
      continue;
    }
    entries.push_back(entry);
    bestScore = std::max(bestScore, entry.score);
  }

  // スコアがbestScore - scoreDiff以下のものは取り除く
  entries.erase(std::remove_if(
    entries.begin(),
    entries.end(),
    [bestScore, scoreDiff](const BookEntry& entry) {
    return entry.score < bestScore - scoreDiff;
  }), entries.end());

  // 定跡が見つからなかった場合はmoveNone()を返す
  if (entries.empty()) {
    return std::make_pair(Move::moveNone(), ScoreNone);
  }

  // countに比例する確率で1手選ぶ
  int64_t sumCount = 0;
  const BookEntry* selectedEntry = nullptr;
  for (const auto& entry : entries) {
    sumCount += entry.count;
    if (g_randomTimeSeed() % sumCount < entry.count) {
      selectedEntry = &entry;
    }
  }
  assert(selectedEntry);

  return std::make_pair(complementMove(selectedEntry->fromToPro, pos), selectedEntry->score);
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

    if (USI::Options[OptionNames::MIN_BOOK_SCORE]) {
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
    pos.searcher()->setOption(option);
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

    pos.set(DefaultStartPositionSFEN, pos.searcher()->threads.mainThread());
    StateStackPtr SetUpStates = StateStackPtr(new std::stack<StateInfo>());
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
    pos.set(DefaultStartPositionSFEN, pos.searcher()->threads.mainThread());
    StateStackPtr SetUpStates = StateStackPtr(new std::stack<StateInfo>());
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
        go(pos, "depth 3");
        pos.searcher()->threads.waitForThinkFinished();
        pos.undoMove(entryMove);
        SetUpStates->pop();
        // doMove してから search してるので点数が反転しているので直す。
        entry.score = -pos.csearcher()->rootMoves[0].score_;
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
