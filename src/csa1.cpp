#include <fstream>
#include <memory>
#include "csa1.hpp"
#include "search.hpp"
#include "string_util.hpp"

#define RETURN_IF_FALSE(x) if (!(x)) { return false; }

bool csa::readCsa1(
  const std::string& filepath,
  Position& pos,
  std::vector<GameRecord>& gameRecords)
{
  std::ifstream ifs(filepath);
  if (!ifs.is_open()) {
    std::cout
      << "!!! Failed to open the input file: filepath="
      << filepath
      << std::endl;
    return false;
  }

  int gameRecordIndex = 0;
  std::string line;
  while (std::getline(ifs, line)) {
    ++gameRecordIndex;
    GameRecord gameRecord;
    std::istringstream iss0(line);

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
      std::cout
        << "Failed to read the second line of a game: gameRecordIndex="
        << gameRecordIndex
        << std::endl;
      return false;
    }

    if (static_cast<int>(line.size()) != 6 * gameRecord.numberOfPlays) {
      std::cout
        << "Number of moves is not expected: gameRecordIndex=" << gameRecordIndex
        << " expected=" << 6 * gameRecord.numberOfPlays
        << " actual=" << line.size()
        << std::endl;
      continue;
    }

    pos.set(DefaultStartPositionSFEN, pos.searcher()->threads.mainThread());
    std::stack<StateInfo> stateStack;
    for (int play = 0; play < gameRecord.numberOfPlays; ++play) {
      std::string moveStr = line.substr(play * 6, 6);
      Move move = csaToMove(pos, moveStr);
      if (move.isNone()) {
        //pos.print();
        //std::cout
        //  << "Failed to parse a move: moveStr="
        //  << moveStr
        //  << std::endl;
        break;
        //return false;
      }
      gameRecord.moves.push_back(move);

      stateStack.push(StateInfo());
      pos.doMove(move, stateStack.top());
    }

    gameRecords.push_back(gameRecord);
  }

  return true;
}

bool csa::writeCsa1(
  const std::string& filepath,
  const std::vector<GameRecord>& gameRecords)
{
  std::ofstream ofs(filepath);
  if (!ofs.is_open()) {
    std::cout << "!!! Failed to create the output file: filepath="
      << filepath
      << std::endl;
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
      << gameRecord.strategy << std::endl;
    for (const auto& move : gameRecord.moves) {
      ofs << move.toCSA();
    }
    ofs << std::endl;
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
    std::cout << p << std::endl;
    if (!readCsa1(p, pos, gameRecords)) {
      return false;
    }
  }

  if (!writeCsa1(outputFilepath, gameRecords)) {
    return false;
  }

  return true;
}
