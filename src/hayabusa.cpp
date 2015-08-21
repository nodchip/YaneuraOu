#include "csa.hpp"
#include "hayabusa.hpp"
#include "thread.hpp"
#include "usi.hpp"
#include "search.hpp"
#include <filesystem>
#include <iostream>

using namespace std;
using namespace std::tr2::sys;

const std::tr2::sys::path hayabusa::DEFAULT_INPUT_DIRECTORY_PATH("../../../wdoor2015/2015");
const std::tr2::sys::path hayabusa::DEFAULT_OUTPUT_DIRECTORY_PATH("../../../cache/hayabusa/evaluation");

void setPosition(Position& pos, std::istringstream& ssCmd);
void go(const Position& pos, std::istringstream& ssCmd);

static void concat(const vector<string>& words, string& out) {
  out.clear();
  for (const auto& word : words) {
    if (!out.empty()) {
      out += " ";
    }
    out += word;
  }
}

bool hayabusa::createEvaluationCache(
  const std::tr2::sys::path& inputDirectoryPath,
  const std::tr2::sys::path& outputDirectoryPath,
  int maxNumberOfPlays) {
  create_directories(DEFAULT_OUTPUT_DIRECTORY_PATH);

  int plays = 0;
  for (auto it = directory_iterator(inputDirectoryPath); it != directory_iterator(); ++it) {
    const auto& inputFilePath = *it;
    cout << inputFilePath << endl;

    path outputFilePath = outputDirectoryPath / inputFilePath.path().filename();
    // 出力ファイルが既に存在していたらスキップ
    if (is_regular_file(outputFilePath)) {
      continue;
    }

    vector<string> sfen;
    if (!csa::toSfen(inputFilePath, sfen)) {
      cout << "!!! Failed to create an evaluation cache: inputFilePath=" << inputFilePath << endl;
      return false;
    }

    ofstream ofs(outputFilePath);
    if (!ofs.is_open()) {
      cout << "!!! Failed to create an output file: outputFilPath=" << outputFilePath << endl;
      return false;
    }

    int numberOfPlays = sfen.size() - 2;
    for (int play = 1; play <= numberOfPlays; ++play) {
      string subSfen;
      concat(vector<string>(sfen.begin(), sfen.begin() + play + 2), subSfen);

      std::istringstream ss_sfen(subSfen);
      Position pos(DefaultStartPositionSFEN, g_threads.mainThread());
      setPosition(pos, ss_sfen);

      SearchStack searchStack[MaxPlyPlus2];
      memset(searchStack, 0, sizeof(searchStack));
      searchStack[0].currentMove = Move::moveNull(); // skip update gains
      searchStack[0].staticEvalRaw = (Score)INT_MAX;
      searchStack[1].staticEvalRaw = (Score)INT_MAX;

      Score score = evaluate(pos, &searchStack[1]);

      ofs << score << endl;

      if (++plays >= maxNumberOfPlays) {
        break;
      }
    }
  }
  return true;
}
