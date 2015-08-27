#include "csa.hpp"
#include "generateMoves.hpp"
#include "hayabusa.hpp"
#include "thread.hpp"
#include "usi.hpp"
#include "search.hpp"
#include "square.hpp"
#include "thread.hpp"
#include <filesystem>
#include <iostream>

using namespace std;
using namespace std::tr2::sys;

const std::tr2::sys::path hayabusa::DEFAULT_INPUT_CSA_DIRECTORY_PATH("../../wdoor2015/2015");
const std::tr2::sys::path hayabusa::DEFAULT_OUTPUT_TEACHER_DATA_FILE_PATH("../hayabusa.teacherdata");
const std::tr2::sys::path hayabusa::DEFAULT_INPUT_TEACHER_DATA_FILE_PATH("../hayabusa.teacherdata");
const std::tr2::sys::path hayabusa::DEFAULT_INPUT_SHOGIDOKORO_CSA_DIRECTORY_PATH("../../Shogidokoro/csa");
const std::tr2::sys::path hayabusa::DEFAULT_INPUT_SFEN_FILE_PATH("../bin/kifu.sfen");

static const float ALPHA = pow(10.0, -7.0);
extern bool showInfo;

void setPosition(Position& pos, std::istringstream& ssCmd);
void go(const Position& pos, std::istringstream& ssCmd);

// 文字列をスペースで区切って文字列の配列に変換する
static void split(const string& in, vector<string>& out) {
  out.clear();
  istringstream iss(in);
  string word;
  while (iss >> word) {
    out.push_back(word);
  }
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

// SFENを教師データに変換する
// sfen SFEN形式の文字列
// maxNumberOfPlays 処理する最大局面数
// teacherDatas 教師データ
// plays 現在までに処理した局面数
static bool converSfenToTeacherData(
  const string& in,
  int maxNumberOfPlays,
  vector<hayabusa::TeacherData>& teacherDatas,
  int& plays) {
  vector<string> sfen;
  split(in, sfen);

  int numberOfPlays = sfen.size() - 2;
  for (int play = 1; play <= numberOfPlays; ++play) {
    string subSfen;
    concat(vector<string>(sfen.begin(), sfen.begin() + play + 2), subSfen);

    std::istringstream ss_sfen(subSfen);
    Position pos(DefaultStartPositionSFEN, g_threads.mainThread());
    setPosition(pos, ss_sfen);

    std::istringstream ss_go("depth 6");
    go(pos, ss_go);
    g_threads.waitForThinkFinished();

    Score score = Searcher::rootMoves[0].score_;
    if (pos.turn() == White) {
      score = -score;
    }

#ifdef _DEBUG
    cout << "play=" << play << " score=" << score << endl;
#endif

    hayabusa::TeacherData teacherData;
    memset(&teacherData, 0, sizeof(hayabusa::TeacherData));
    teacherData.squareBlackKing = pos.kingSquare(Black);
    teacherData.squareWhiteKing = pos.kingSquare(White);
    memcpy(teacherData.list0, pos.cplist0(), sizeof(teacherData.list0));
    memcpy(teacherData.list1, pos.cplist1(), sizeof(teacherData.list1));
    teacherData.material = pos.material();
    teacherData.teacher = score;
    teacherDatas.push_back(teacherData);

    if (++plays >= maxNumberOfPlays) {
      return true;
    }
  }

  return true;
}

static int getNumberOfLines(const path& filePath) {
  int numberOfLines = 0;
  string _;
  ifstream ifs(filePath);
  while (getline(ifs, _)) {
    ++numberOfLines;
  }
  return numberOfLines;
}

bool hayabusa::convertSfenToTeacherData(
  const std::tr2::sys::path& inputSfenFilePath,
  const std::tr2::sys::path& outputTeacherDataFilePath,
  int maxNumberOfPlays) {
  cout << "hayabusa::createTeacherData()" << endl;

  showInfo = false;

  int numberOfKifus = getNumberOfLines(inputSfenFilePath);

  ifstream ifs(inputSfenFilePath);
  if (!ifs.is_open()) {
    cout << "!!! Failed to open the input file: inputSfenFilePath="
      << inputSfenFilePath
      << endl;
    return false;
  }

  ofstream ofs(outputTeacherDataFilePath, std::ios::out | std::ios::binary);
  if (!ofs.is_open()) {
    cout << "!!! Failed to create an output file: outputTeacherDataFilePath="
      << outputTeacherDataFilePath
      << endl;
    return false;
  }

  int plays = 0;
  int fileIndex = 0;
  time_t startTime = time(nullptr);
  string line;
  while (getline(ifs, line)) {
    // 進捗状況表示
    if (++fileIndex % 1 == 0) {
      time_t currentTime = time(nullptr);
      int remainingSec = (currentTime - startTime) * (numberOfKifus- fileIndex) / fileIndex;
      printf("(%d/%d) %d:%02d:%02d\n", fileIndex, numberOfKifus, remainingSec / 3600, remainingSec / 60 % 60, remainingSec % 60);
    }

    vector<TeacherData> teacherDatas;
    if (!converSfenToTeacherData(line, maxNumberOfPlays, teacherDatas, plays)) {
      cout << "!!! Failed to convert SFEN to teacher data: line=" << line << endl;
      return false;
    }

    ofs.write((char*)&teacherDatas[0], sizeof(TeacherData) * teacherDatas.size());

    if (++plays >= maxNumberOfPlays) {
      break;
    }
  }

  return true;
}

//bool hayabusa::addTeacherData(
//  const std::tr2::sys::path& inputShogidokoroCsaDirectoryPath,
//  const std::tr2::sys::path& outputTeacherFilePath,
//  int maxNumberOfPlays) {
//  cout << "hayabusa::addTeacherData()" << endl;
//
//  ofstream teacherFile(outputTeacherFilePath, std::ios::app | std::ios::binary);
//  if (!teacherFile.is_open()) {
//    cout << "!!! Failed to open an output file: outputTeacherFilePath="
//      << outputTeacherFilePath
//      << endl;
//    return false;
//  }
//
//  int plays = 0;
//  int fileIndex = 0;
//  for (auto it = directory_iterator(inputShogidokoroCsaDirectoryPath); it != directory_iterator(); ++it) {
//    const auto& csaFilePath = *it;
//    if (csaFilePath.path().extension() != ".csa") {
//      continue;
//    }
//
//    cout << csaFilePath.path().filename() << endl;
//
//    if (!csa::isFinished(csaFilePath)) {
//      continue;
//    }
//
//    vector<TeacherData> teacherDatas;
//    if (!converSfenToTeacherData(csaFilePath, maxNumberOfPlays, teacherDatas, plays)) {
//      cout << "!!! Failed to create teacher data: csaFilePath=" << csaFilePath << endl;
//      return false;
//    }
//
//    bool tanukIsBlack = csa::isTanukiBlack(csaFilePath);
//    Color winner = csa::getWinner(csaFilePath);
//
//    if (tanukIsBlack && winner == White) {
//      // tanuki-が先手で負けた
//      // 先手の教師信号を下げる
//      for (int i = 0; i < teacherDatas.size(); i += 2) {
//        teacherDatas[i].teacher -= 512;
//      }
//    }
//
//    if (!tanukIsBlack && winner == White) {
//      // tanuki-が後手で負けた
//      // 後手の教師信号を下げる
//      for (int i = 1; i < teacherDatas.size(); i += 2) {
//        teacherDatas[i].teacher -= 512;
//      }
//    }
//
//    teacherFile.write((char*)&teacherDatas[0], sizeof(TeacherData) * teacherDatas.size());
//
//    if (++plays >= maxNumberOfPlays) {
//      break;
//    }
//  }
//
//  return true;
//}

bool hayabusa::adjustWeights(
  const std::tr2::sys::path& inputTeacherFilePath,
  int numberOfIterations) {
  cout << "hayabusa::adjustWeights()" << endl;

  int numberOfTeacherData = file_size(inputTeacherFilePath) / sizeof(TeacherData);
  float alpha = ALPHA / numberOfTeacherData;
  double prevEps2 = 1e100;
  for (int iteration = 0; iteration < numberOfIterations; ++iteration) {
    cout << "iteration " << (iteration + 1) << "/" << numberOfIterations << endl;

    // 最急降下法を使用して重みを調整する
    static float k00sum[SquareNum][SquareNum];
    static float kpp[SquareNum][Apery::fe_end][Apery::fe_end];
    static float kkp[SquareNum][SquareNum][Apery::fe_end];
    memset(k00sum, 0, sizeof(k00sum));
    memset(kpp, 0, sizeof(kpp));
    memset(kkp, 0, sizeof(kkp));

    ifstream teacherFile(inputTeacherFilePath, std::ios::in | std::ios::binary);
    if (!teacherFile.is_open()) {
      cout << "!!! Failed to open a teacher file: inputTeacherFilePath=" << inputTeacherFilePath << endl;
      return false;
    }

    vector<TeacherData> teacherDatas(numberOfTeacherData);
    teacherFile.read((char*)&teacherDatas[0], sizeof(TeacherData) * numberOfTeacherData);

    double eps2 = 0.0;
    int teacherDataIndex = 0;
    for (const auto& teacherData : teacherDatas) {
      if (++teacherDataIndex % 1000000 == 0) {
        printf("(%d/%d)\n", teacherDataIndex, numberOfTeacherData);
      }

      Square sq_bk = teacherData.squareBlackKing;
      Square sq_wk = teacherData.squareWhiteKing;
      const int* list0 = teacherData.list0;
      const int* list1 = teacherData.list1;
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
      float diff = alpha * (int)delta / (iteration + 1);
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

    //if (prevEps2 < eps2) {
    //  alpha *= 0.5;
    //}
    prevEps2 = eps2;
    cout << setprecision(10) << "eps2=" << eps2 << " alpha=" << alpha << endl << endl;
  }

  return true;
}
