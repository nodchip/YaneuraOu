#include <filesystem>
#include <fstream>
#include "csa.hpp"
#include "thread.hpp"
#include "usi.hpp"

using namespace std;
using namespace std::tr2::sys;

bool csa::toSfen(const std::tr2::sys::path& filepath, std::vector<std::string>& sfen) {
  sfen.clear();
  sfen.push_back("startpos");
  sfen.push_back("moves");

  Position position(DefaultStartPositionSFEN, g_threads.mainThread());

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
  return false;
}
