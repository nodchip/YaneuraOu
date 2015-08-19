#include <fstream>
#include "csa.hpp"
#include "thread.hpp"
#include "usi.hpp"

using namespace std;

bool csa::toSfen(const std::string& filepath, std::vector<std::string>& sfen) {
  sfen.clear();
  sfen.push_back("startpos");
  sfen.push_back("moves");

  Position position(DefaultStartPositionSFEN, g_threads.mainThread());

  ifstream ifs(filepath.c_str());
  if (!ifs.is_open()) {
    cout << "!!! Failed to open a CSA file." << endl;
    return false;
  }

  // Position::doMove()は前回と違うアドレスに確保されたStateInfoを要求するため
  // listを使って過去のStateInfoを保持する。
  list<StateInfo> stateInfos;

  string line;
  while (getline(ifs, line)) {
    if (line.size() != 7 || (line[0] != '+' && line[0] != '-')) {
      continue;
    }

    string csaMove = line.substr(1);
    Move move = csaToMove(position, csaMove);
    if (!position.moveIsLegal(move)) {
      cout << "!!! Found an illegal move." << endl;
      break;
    }

    stateInfos.push_back(StateInfo());
    position.doMove(move, stateInfos.back());
    //position.print();

    sfen.push_back(move.toUSI());
  }

  return true;
}
