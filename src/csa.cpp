#include <filesystem>
#include <fstream>
#include "csa.hpp"
#include "search.hpp"
#include "string_util.hpp"

using namespace std;
using namespace std::tr2::sys;

namespace
{
  int getNumberOfFiles(const std::tr2::sys::path& directory)
  {
    int numberOfFiles = 0;
    for (std::tr2::sys::recursive_directory_iterator it(directory);
    it != std::tr2::sys::recursive_directory_iterator();
      ++it)
    {
      ++numberOfFiles;
    }
    return numberOfFiles;
  }
}

const std::tr2::sys::path csa::DEFAULT_INPUT_CSA1_FILE_PATH = "../2chkifu_csa/2chkifu.csa1";
const std::tr2::sys::path csa::DEFAULT_OUTPUT_SFEN_FILE_PATH = "../bin/kifu.sfen";

bool csa::toSfen(const std::tr2::sys::path& filepath, std::vector<std::string>& sfen) {
  sfen.clear();
  sfen.push_back("startpos");
  sfen.push_back("moves");

  Position position;
  setPosition(position, sfen);

  ifstream ifs(filepath);
  if (!ifs.is_open()) {
    cout << "!!! Failed to open a CSA file." << endl;
    return false;
  }

  // Position::doMove()は前回と違うアドレスに確保されたStateInfoを要求するため
  // listを使って過去のStateInfoを保持する。
  list<StateInfo> stateInfos;

  string line;
  while (getline(ifs, line)) {
    // 将棋所の出力するCSAの指し手の末尾に",T1"などとつくため
    // ","以降を削除する
    if (line.find(',') != string::npos) {
      line = line.substr(0, line.find(','));
    }

    if (line.size() != 7 || (line[0] != '+' && line[0] != '-')) {
      continue;
    }

    string csaMove = line.substr(1);
    Move move = csaToMove(position, csaMove);

#if !defined NDEBUG
    if (!position.moveIsLegal(move)) {
      cout << "!!! Found an illegal move." << endl;
      break;
    }
#endif

    stateInfos.push_back(StateInfo());
    position.doMove(move, stateInfos.back());
    //position.print();

    sfen.push_back(move.toUSI());
  }

  return true;
}

bool csa::isFinished(const std::tr2::sys::path& filepath) {
  ifstream ifs(filepath);
  if (!ifs.is_open()) {
    cout << "!!! Failed to open a CSA file." << endl;
    return false;
  }

  string line;
  while (getline(ifs, line)) {
    if (line.find("%TORYO") == 0) {
      return true;
    }
  }

  return false;
}

bool csa::isTanukiBlack(const std::tr2::sys::path& filepath) {
  ifstream ifs(filepath);
  if (!ifs.is_open()) {
    cout << "!!! Failed to open a CSA file." << endl;
    return false;
  }

  string line;
  while (getline(ifs, line)) {
    if (line.find("N+tanuki-") == 0) {
      return true;
    }
  }

  return false;
}

Color csa::getWinner(const std::tr2::sys::path& filepath) {
  assert(isFinished(filepath));

  ifstream ifs(filepath);
  if (!ifs.is_open()) {
    cout << "!!! Failed to open a CSA file." << endl;
    return ColorNum;
  }

  char turn = 0;
  string line;
  while (getline(ifs, line)) {
    if (line.empty()) {
      continue;
    }
    if (line[0] == '+' || line[0] == '-') {
      turn = line[0];
    }
    if (line.find("%TORYO") == 0) {
      return turn == '+' ? Black : White;
    }
  }

  throw exception("Failed to detect which player is win.");
}

// 文字列の配列をスペース区切りで結合する
static void concat(const vector<string>& words, string& out) {
  out.clear();
  for (const auto& word : words) {
    if (!out.empty()) {
      out += " ";
    }
    out += word;
  }
}

bool csa::convertCsaToSfen(
  const std::tr2::sys::path& inputDirectoryPath,
  const std::tr2::sys::path& outputFilePath) {
  if (!is_directory(inputDirectoryPath)) {
    cout << "!!! Failed to open the input directory: inputDirectoryPath="
      << inputDirectoryPath
      << endl;
    return false;
  }

  ofstream ofs(outputFilePath, std::ios::out);
  if (!ofs.is_open()) {
    cout << "!!! Failed to create an output file: outputTeacherFilePath="
      << outputFilePath
      << endl;
    return false;
  }

  int numberOfFiles = distance(
    directory_iterator(inputDirectoryPath),
    directory_iterator());
  int fileIndex = 0;
  for (auto it = directory_iterator(inputDirectoryPath); it != directory_iterator(); ++it) {
    if (++fileIndex % 1000 == 0) {
      printf("(%d/%d)\n", fileIndex, numberOfFiles);
    }

    const auto& inputFilePath = *it;
    vector<string> sfen;
    if (!toSfen(inputFilePath, sfen)) {
      cout << "!!! Failed to convert the input csa file to SFEN: inputFilePath="
        << inputFilePath
        << endl;
      continue;
    }

    string line;
    concat(sfen, line);
    ofs << line << endl;
  }

  return true;
}

bool csa::convertCsa1LineToSfen(
  const std::tr2::sys::path& inputFilePath,
  const std::tr2::sys::path& outputFilePath) {
  std::ifstream ifs(inputFilePath);
  if (!ifs.is_open()) {
    cout << "!!! Failed to open the input file: inputFilePath="
      << inputFilePath
      << endl;
    return false;
  }

  ofstream ofs(outputFilePath, std::ios::out);
  if (!ofs.is_open()) {
    cout << "!!! Failed to create an output file: outputTeacherFilePath="
      << outputFilePath
      << endl;
    return false;
  }

  std::string line;
  while (std::getline(ifs, line)) {
    Scanner scanner = line;
    int kifuIndex = scanner.nextInt();

    // 進捗状況表示
    if (kifuIndex % 1000 == 0) {
      cout << kifuIndex << endl;
    }

    std::string elem;
    scanner.next(); // 対局日を飛ばす。
    const std::string sente = scanner.next(); // 先手
    const std::string gote = scanner.next(); // 後手
    scanner.next(); // (0:引き分け,1:先手の勝ち,2:後手の勝ち)

    if (!std::getline(ifs, line)) {
      std::cout << "!!! header only !!!" << std::endl;
      return false;
    }

    ofs << "startpos moves";

    Position pos;
    pos.set(DefaultStartPositionSFEN, Searcher::threads.mainThread());
    StateStackPtr SetUpStates = StateStackPtr(new std::stack<StateInfo>());
    while (!line.empty()) {
      const std::string moveStrCSA = line.substr(0, 6);
      const Move move = csaToMove(pos, moveStrCSA);
      if (move.isNone()) {
        pos.print();
        std::cout << "!!! Illegal move = " << moveStrCSA << " !!!" << std::endl;
        break;
      }
      line.erase(0, 6); // 先頭から6文字削除

      ofs << " " << move.toUSI();

      SetUpStates->push(StateInfo());
      pos.doMove(move, SetUpStates->top());
    }

    ofs << endl;
  }

  return true;
}

bool csa::readCsa(const std::tr2::sys::path& filepath, GameRecord& gameRecord)
{
  gameRecord.gameRecordIndex = 0;
  gameRecord.date = "??/??/??";
  gameRecord.winner = 0;
  gameRecord.leagueName = "???";
  gameRecord.strategy = "???";

  Position position;
  setPosition(position, "startpos moves");

  std::ifstream ifs(filepath);
  if (!ifs.is_open()) {
    return false;
  }

  // Position::doMove()は前回と違うアドレスに確保されたStateInfoを要求するため
  // listを使って過去のStateInfoを保持する。
  list<StateInfo> stateInfos;

  string line;
  Color lastColor = Black;
  while (getline(ifs, line)) {
    // 将棋所の出力するCSAの指し手の末尾に",T1"などとつくため
    // ","以降を削除する
    if (line.find(',') != string::npos) {
      line = line.substr(0, line.find(','));
    }

    if (line.find("N+") == 0) {
      gameRecord.blackPlayerName = line.substr(2);
    }
    else if (line.find("N-") == 0) {
      gameRecord.whitePlayerName = line.substr(2);
    }
    else if (line.size() == 7 && (line[0] == '+' || line[0] == '-')) {
      string csaMove = line.substr(1);
      Move move = csaToMove(position, csaMove);

#if !defined NDEBUG
      if (!position.moveIsLegal(move)) {
        cout << "!!! Found an illegal move." << endl;
        break;
      }
#endif

      stateInfos.push_back(StateInfo());
      position.doMove(move, stateInfos.back());
      //position.print();

      gameRecord.moves.push_back(move);

      if (line[0] == '+') {
        lastColor = Black;
      }
    }
    else if (line.find("%TORYO") == 0) {
      gameRecord.winner = (lastColor == Black ? 2 : 1);
    }
  }

  gameRecord.numberOfPlays = gameRecord.moves.size();
}

bool csa::readCsas(
  const std::tr2::sys::path& directory,
  const std::function<bool(const std::tr2::sys::path&)>& filter,
  std::vector<GameRecord>& gameRecords)
{
  cout << "Listing files ..." << endl;
  int numberOfFiles = getNumberOfFiles(directory);

  double startSec = clock() / double(CLOCKS_PER_SEC);
  int fileIndex = 0;
  for (std::tr2::sys::recursive_directory_iterator it(directory);
  it != std::tr2::sys::recursive_directory_iterator();
    ++it)
  {
    if (++fileIndex % 100 == 0) {
      double currentSec = clock() / double(CLOCKS_PER_SEC);
      double secPerFile = (currentSec - startSec) / fileIndex;
      int remainedSec = (numberOfFiles - fileIndex) * secPerFile;
      int second = remainedSec % 60;
      int minute = remainedSec / 60 % 60;
      int hour = remainedSec / 3600;
      printf("%d/%d %d:%02d:%02d\n", fileIndex, numberOfFiles, hour, minute, second);
    }

    if (!filter(it->path())) {
      continue;
    }

    GameRecord gameRecord;
    if (!readCsa(it->path(), gameRecord)) {
      return false;
    }
    gameRecords.push_back(gameRecord);
  }

  return true;
}

bool csa::readCsa1(
  const std::tr2::sys::path& filepath,
  std::vector<GameRecord>& gameRecord)
{
  // TODO(nodchip): Implement.
  return false;
}

bool csa::writeCsa1(
  const std::tr2::sys::path& filepath,
  const std::vector<GameRecord>& gameRecord)
{
  // TODO(nodchip): Implement.
  return false;
}

bool csa::mergeCsa1s(
  const std::vector<std::tr2::sys::path>& inputFilepaths,
  const std::tr2::sys::path& outputFilepath)
{
  // TODO(nodchip): Implement.
  return false;
}
