#include "csa.hpp"
#include "hayabusa.hpp"
#include "thread.hpp"
#include "usi.hpp"
#include "search.hpp"
#include "square.hpp"
#include <filesystem>
#include <iostream>

using namespace std;
using namespace std::tr2::sys;

const std::tr2::sys::path hayabusa::DEFAULT_INPUT_CSA_DIRECTORY_PATH("../../wdoor2015/2015");
const std::tr2::sys::path hayabusa::DEFAULT_OUTPUT_TEACHER_DATA_FILE_PATH("../hayabusa.teacherdata");
const std::tr2::sys::path hayabusa::DEFAULT_INPUT_TEACHER_DATA_FILE_PATH("../hayabusa.teacherdata");

static const float ALPHA = pow(10.0, -6.5);

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

  int numberOfFiles = distance(
    directory_iterator(inputCsaDirectoryPath),
    directory_iterator());

  int plays = 0;
  int fileIndex = 0;
  for (auto it = directory_iterator(inputCsaDirectoryPath); it != directory_iterator(); ++it) {
    if (++fileIndex % 1000 == 0) {
      printf("(%d/%d)\n", fileIndex, numberOfFiles);
    }
    const auto& inputFilePath = *it;

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
      teacherData.squareBlackKing = pos.kingSquare(Black);
      teacherData.squareWhiteKing = pos.kingSquare(White);
      memcpy(teacherData.list0, pos.cplist0(), sizeof(teacherData.list0));
      memcpy(teacherData.list1, pos.cplist1(), sizeof(teacherData.list1));
      teacherData.material = pos.material();
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

bool hayabusa::adjustWeights(
  const std::tr2::sys::path& inputTeacherFilePath,
  int numberOfIterations) {
  int numberOfTeacherData = file_size(inputTeacherFilePath) / sizeof(TeacherData);
  float alpha = ALPHA / numberOfTeacherData;
  double prevEps2 = 1e100;
  for (int iteration = 0; iteration < numberOfIterations; ++iteration) {
    // 最急降下法を使用して重みを調整する
    static float k00sum[SquareNum][SquareNum];
    static float kpp[SquareNum][Apery::fe_end][Apery::fe_end];
    static float kkp[SquareNum][SquareNum][Apery::fe_end];
    memset(k00sum, 0, sizeof(k00sum));
    memset(kpp, 0, sizeof(kpp));
    memset(kkp, 0, sizeof(kkp));

    FILE* file = fopen(inputTeacherFilePath.string().c_str(), "rb");
    assert(file);
    setvbuf(file, nullptr, _IOFBF, 1024 * 1024);

    TeacherData teacherData;
    double eps2 = 0.0;
    int teacherDataIndex = 0;
    while (fread(&teacherData, sizeof(teacherData), 1, file) == 1) {
      if (++teacherDataIndex % 1000000 == 0) {
        printf("(%d/%d)\n", teacherDataIndex, numberOfTeacherData);
      }

      Square sq_bk = teacherData.squareBlackKing;
      Square sq_wk = teacherData.squareWhiteKing;
      int* list0 = teacherData.list0;
      int* list1 = teacherData.list1;
      int material = teacherData.material;
      Score teacher = teacherData.teacher * Apery::FVScale;

      // 実際の信号を計算する
      Score y = static_cast<Score>(K00Sum[sq_bk][sq_wk]);
      for (int i = 0; i < EvalList::ListSize; ++i) {
        const int k0 = list0[i];
        const int k1 = list1[i];
        for (int j = 0; j < i; ++j) {
          const int l0 = list0[j];
          const int l1 = list1[j];
          y += KPP[sq_bk][k0][l0];
          y -= KPP[inverse(sq_wk)][k1][l1];
        }
        y += KKP[sq_bk][sq_wk][k0];
      }
      y += material * Apery::FVScale;

      // 教師信号と実際の信号から重みの増減分を計算する
      // TODO(nodchip): materialを更新する必要があるかどうか考える
      Score delta = teacher - y;
      float diff = alpha * (int)delta;
      k00sum[sq_bk][sq_wk] += diff;
      for (int i = 0; i < EvalList::ListSize; ++i) {
        const int k0 = list0[i];
        const int k1 = list1[i];
        for (int j = 0; j < i; ++j) {
          const int l0 = list0[j];
          const int l1 = list1[j];
          kpp[sq_bk][k0][l0] += diff;
          kpp[sq_bk][l0][k0] += diff;
          kpp[inverse(sq_wk)][k1][l1] -= diff;
          kpp[inverse(sq_wk)][l1][k1] -= diff;
        }
        kkp[sq_bk][sq_wk][k0] += diff;
      }

      eps2 += delta * delta;
    }

    fclose(file);
    file = nullptr;

    // 重みを更新する
    for (int i = 0; i < SquareNum; ++i) {
      for (int j = 0; j < SquareNum; ++j) {
        K00Sum[i][j] += k00sum[i][j];
      }
    }
    for (int i = 0; i < SquareNum; ++i) {
      for (int j = 0; j < Apery::fe_end; ++j) {
        for (int k = 0; k < Apery::fe_end; ++k) {
          KPP[i][j][k] += kpp[i][j][k];
        }
      }
    }
    for (int i = 0; i < SquareNum; ++i) {
      for (int j = 0; j < SquareNum; ++j) {
        for (int k = 0; k < Apery::fe_end; ++k) {
          KKP[i][j][k] += kkp[i][j][k];
        }
      }
    }

    if (prevEps2 < eps2) {
      alpha *= 0.5;
    }
    prevEps2 = eps2;
    cout << "eps2=" << eps2 << " alpha=" << alpha << endl;
  }

  return true;
}
