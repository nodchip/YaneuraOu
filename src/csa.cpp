#include <filesystem>
#include <fstream>
#include "csa.hpp"
#include "thread.hpp"
#include "usi.hpp"

using namespace std;

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
