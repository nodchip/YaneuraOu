#include "csa.hpp"
#include "hayabusa.hpp"
#include "thread.hpp"
#include "usi.hpp"
#include "search.hpp"
#include <filesystem>
#include <iostream>

using namespace std;
using namespace std::tr2::sys;

const std::tr2::sys::path hayabusa::DEFAULT_INPUT_CSA_DIRECTORY_PATH("../../../wdoor2015/2015");
const std::tr2::sys::path hayabusa::DEFAULT_OUTPUT_TEACHER_FILE_PATH("../../hayabusa.teacherdata");

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

bool hayabusa::createTeacherData(
  const std::tr2::sys::path& inputCsaDirectoryPath,
  const std::tr2::sys::path& outputTeacherFilePath,
  int maxNumberOfPlays) {
  FILE* file = fopen(outputTeacherFilePath.string().c_str(), "wb");
  if (!file) {
    cout << "!!! Failed to create an output file: outputTeacherFilePath="
      << outputTeacherFilePath
      << endl;
    return false;
  }
  setvbuf(file, nullptr, _IOFBF, 1024 * 1024);

  int numberOfFiles = distance(directory_iterator(inputCsaDirectoryPath),
    directory_iterator());

  int plays = 0;
  int fileIndex = 0;
  for (auto it = directory_iterator(inputCsaDirectoryPath); it != directory_iterator(); ++it) {
    const auto& inputFilePath = *it;
    cout << "(" << fileIndex++ << "/" << numberOfFiles << ") "
      << inputFilePath << endl;

    vector<string> sfen;
    if (!csa::toSfen(inputFilePath, sfen)) {
      cout << "!!! Failed to create an evaluation cache: inputFilePath=" << inputFilePath << endl;
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
      if (pos.turn() == White) {
        score = -score;
      }

      TeacherData teacherData;
      memcpy(teacherData.list0, pos.cplist0(), sizeof(teacherData.list0));
      memcpy(teacherData.list1, pos.cplist1(), sizeof(teacherData.list1));
      teacherData.teacher = score;

      int writeSize = fwrite(&teacherData, sizeof(TeacherData), 1, file);
      assert(writeSize == 1);

      if (++plays >= maxNumberOfPlays) {
        break;
      }
    }
  }

  fclose(file);
  file = nullptr;

  return true;
}
