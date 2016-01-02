#include <fstream>
#include "csa1.hpp"
#include "search.hpp"
#include "string_util.hpp"

using namespace std;

#define RETURN_IF_FALSE(x) if (!(x)) { return false; }

bool csa::readCsa1(
  const std::string& filepath,
  Position& pos,
  std::vector<GameRecord>& gameRecords)
{
  std::ifstream ifs(filepath);
  if (!ifs.is_open()) {
    cout
      << "!!! Failed to open the input file: filepath="
      << filepath
      << endl;
    return false;
  }

  int gameRecordIndex = 0;
  std::string line;
  while (std::getline(ifs, line)) {
    ++gameRecordIndex;
    GameRecord gameRecord;
    istringstream iss0(line);

    iss0
      >> gameRecord.gameRecordIndex
      >> gameRecord.date
      >> gameRecord.blackPlayerName
      >> gameRecord.whitePlayerName
      >> gameRecord.winner
      >> gameRecord.numberOfPlays
      >> gameRecord.leagueName
      >> gameRecord.strategy;

    if (!getline(ifs, line)) {
      cout
        << "Failed to read the second line of a game: gameRecordIndex="
        << gameRecordIndex
        << endl;
      return false;
    }

    if (line.size() != 6 * gameRecord.numberOfPlays) {
      cout
        << "Number of moves is not expected: gameRecordIndex=" << gameRecordIndex
        << " expected=" << 6 * gameRecord.numberOfPlays
        << " actual=" << line.size()
        << endl;
      continue;
    }

    pos.set(DefaultStartPositionSFEN, pos.searcher()->threads.mainThread());
    StateStackPtr stateStack = make_unique<std::stack<StateInfo>>();
    for (int play = 0; play < gameRecord.numberOfPlays; ++play) {
      string moveStr = line.substr(play * 6, 6);
      Move move = csaToMove(pos, moveStr);
      if (move.isNone()) {
        //pos.print();
        //cout
        //  << "Failed to parse a move: moveStr="
        //  << moveStr
        //  << endl;
        break;
        //return false;
      }
      gameRecord.moves.push_back(move);

      stateStack->push(StateInfo());
      pos.doMove(move, stateStack->top());
    }

    gameRecords.push_back(gameRecord);
  }

  return true;
}

bool csa::writeCsa1(
  const std::string& filepath,
  const std::vector<GameRecord>& gameRecords)
{
  ofstream ofs(filepath);
  if (!ofs.is_open()) {
    cout << "!!! Failed to create the output file: filepath="
      << filepath
      << endl;
    return false;
  }

  int gameRecordIndex = 0;
  for (const auto& gameRecord : gameRecords) {
    ofs
      << ++gameRecordIndex << " "
      << gameRecord.date << " "
      << gameRecord.blackPlayerName << " "
      << gameRecord.whitePlayerName << " "
      << gameRecord.winner << " "
      << gameRecord.numberOfPlays << " "
      << gameRecord.leagueName << " "
      << gameRecord.strategy << endl;
    for (const auto& move : gameRecord.moves) {
      ofs << move.toCSA();
    }
    ofs << endl;
  }

  return true;
}

bool csa::mergeCsa1s(
  const std::vector<std::string>& inputFilepaths,
  const std::string& outputFilepath,
  Position& pos)
{
  std::vector<GameRecord> gameRecords;
  for (const auto& p : inputFilepaths) {
    cout << p << endl;
    if (!readCsa1(p, pos, gameRecords)) {
      return false;
    }
  }

  if (!writeCsa1(outputFilepath, gameRecords)) {
    return false;
  }

  return true;
}
