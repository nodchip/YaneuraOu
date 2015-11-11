#include <filesystem>
#include <iostream>

#include "csa.hpp"
#include "generateMoves.hpp"
#include "hayabusa.hpp"
#include "thread.hpp"
#include "usi.hpp"
#include "search.hpp"
#include "square.hpp"
#include "thread.hpp"
#include "string_util.hpp"

using namespace std;
using namespace std::tr2::sys;

const std::tr2::sys::path hayabusa::DEFAULT_INPUT_CSA_DIRECTORY_PATH("../../wdoor2015/2015");
const std::tr2::sys::path hayabusa::DEFAULT_OUTPUT_TEACHER_DATA_FILE_PATH("../bin/hayabusa.teacherdata");
const std::tr2::sys::path hayabusa::DEFAULT_INPUT_TEACHER_DATA_FILE_PATH("../bin/hayabusa.teacherdata");
const std::tr2::sys::path hayabusa::DEFAULT_INPUT_SHOGIDOKORO_CSA_DIRECTORY_PATH("../../Shogidokoro/csa");
const std::tr2::sys::path hayabusa::DEFAULT_INPUT_SFEN_FILE_PATH("../bin/kifu.sfen");

static const double ALPHA = pow(2.0, -14.0);

static const Score LOSE_PENARTY = PawnScore * 1000;

void setPosition(Position& pos, std::istringstream& ssCmd);
void go(const Position& pos, std::istringstream& ssCmd);


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
  vector<string> sfen = string_util::split(in);

  int numberOfPlays = sfen.size() - 2;
  for (int play = 1; play <= numberOfPlays; ++play) {
    string subSfen = string_util::concat(vector<string>(sfen.begin(), sfen.begin() + play + 2));

    std::istringstream ss_sfen(subSfen);
    Position pos;
    setPosition(pos, ss_sfen);

    //SearchStack searchStack[MaxPlyPlus2];
    //memset(searchStack, 0, sizeof(searchStack));
    //searchStack[0].currentMove = Move::moveNull(); // skip update gains
    //searchStack[0].staticEvalRaw = (Score)INT_MAX;
    //searchStack[1].staticEvalRaw = (Score)INT_MAX;
    //Score score = evaluate(pos, &searchStack[1]);

    go(pos, std::istringstream("depth 1"));
    pos.searcher()->threads.waitForThinkFinished();
    Score score = pos.searcher()->rootMoves[0].score_;

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
  Searcher::outputInfo = false;
  cout << "hayabusa::createTeacherData()" << endl;

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

bool hayabusa::addTeacherData(
  const std::tr2::sys::path& inputShogidokoroCsaDirectoryPath,
  const std::tr2::sys::path& outputTeacherFilePath,
  int maxNumberOfPlays) {
  Searcher::outputInfo = false;
  cout << "hayabusa::addTeacherData()" << endl;

  ofstream teacherFile(outputTeacherFilePath, std::ios::app | std::ios::binary);
  if (!teacherFile.is_open()) {
    cout << "!!! Failed to open an output file: outputTeacherFilePath="
      << outputTeacherFilePath
      << endl;
    return false;
  }

  int plays = 0;
  int fileIndex = 0;
  for (auto it = directory_iterator(inputShogidokoroCsaDirectoryPath); it != directory_iterator(); ++it) {
    const auto& csaFilePath = *it;
    if (csaFilePath.path().extension() != ".csa") {
      continue;
    }

    cout << csaFilePath.path().filename() << endl;

    if (!csa::isFinished(csaFilePath)) {
      continue;
    }

    vector<string> words;
    if (!csa::toSfen(csaFilePath.path(), words)) {
      cout << "!!! Failed to convert a CSA file to SFEN: csaFilePath="
        << csaFilePath.path()
        << endl;
      continue;
    }

    string sfen = string_util::concat(words);

    vector<TeacherData> teacherDatas;
    if (!converSfenToTeacherData(sfen, maxNumberOfPlays, teacherDatas, plays)) {
      cout << "!!! Failed to create teacher data: csaFilePath=" << csaFilePath << endl;
      return false;
    }

    bool tanukIsBlack = csa::isTanukiBlack(csaFilePath);
    Color winner = csa::getWinner(csaFilePath);

    if (tanukIsBlack && winner == White) {
      // tanuki-が先手で負けた
      // 教師信号を下げる
      for (auto& teacherData : teacherDatas) {
        teacherData.teacher += LOSE_PENARTY;
      }
    }
    else if (!tanukIsBlack && winner == Black) {
      // tanuki-が後手で負けた
      // 教師信号を上げる
      for (auto& teacherData : teacherDatas) {
        teacherData.teacher -= LOSE_PENARTY;
      }
    }

    teacherFile.write((char*)&teacherDatas[0], sizeof(TeacherData) * teacherDatas.size());

    if (++plays >= maxNumberOfPlays) {
      break;
    }
  }

  return true;
}

bool hayabusa::adjustWeights(
  const std::tr2::sys::path& inputTeacherFilePath,
  int numberOfIterations) {
  Searcher::outputInfo = false;
  cout << "hayabusa::adjustWeights()" << endl;

  int numberOfTeacherData = file_size(inputTeacherFilePath) / sizeof(TeacherData);
  double alpha = ALPHA / numberOfTeacherData;

  // 重みを更新する
  vector<vector<double> > k00sum(SquareNum, vector<double>(SquareNum));
  vector<vector<vector<double> > > kpp(SquareNum, vector<vector<double> >(fe_end, vector<double>(fe_end)));
  vector<vector<vector<double> > > kkp(SquareNum, vector<vector<double> >(SquareNum, vector<double>(fe_end)));
  for (int i = 0; i < SquareNum; ++i) {
    for (int j = 0; j < SquareNum; ++j) {
      k00sum[i][j] = Evaluater::KK[i][j];
    }
  }
  for (int i = 0; i < SquareNum; ++i) {
    for (int j = 0; j < fe_end; ++j) {
      for (int k = 0; k < fe_end; ++k) {
        kpp[i][j][k] = Evaluater::KPP[i][j][k];
      }
    }
  }
  for (int i = 0; i < SquareNum; ++i) {
    for (int j = 0; j < SquareNum; ++j) {
      for (int k = 0; k < fe_end; ++k) {
        kkp[i][j][k] = Evaluater::KKP[i][j][k];
      }
    }
  }

  for (int iteration = 0; iteration < numberOfIterations; ++iteration) {
    cout << "iteration " << (iteration + 1) << "/" << numberOfIterations << endl;

    // 最急降下法を使用して重みを調整する
    // こういう書き方すると心が荒む
    vector<vector<double> > k00sumDelta(SquareNum, vector<double>(SquareNum));
    vector<vector<vector<double> > > kppDelta(SquareNum, vector<vector<double> >(fe_end, vector<double>(fe_end)));
    vector<vector<vector<double> > > kkpDelta(SquareNum, vector<vector<double> >(SquareNum, vector<double>(fe_end)));
    // TODO(nodchip): バイアスhを加える

    ifstream teacherFile(inputTeacherFilePath, std::ios::in | std::ios::binary);
    if (!teacherFile.is_open()) {
      cout << "!!! Failed to open a teacher file: inputTeacherFilePath=" << inputTeacherFilePath << endl;
      return false;
    }

    vector<TeacherData> teacherDatas(numberOfTeacherData);
    teacherFile.read((char*)&teacherDatas[0], sizeof(TeacherData) * numberOfTeacherData);

    double delta2 = 0.0;
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
      Score teacher = teacherData.teacher * FVScale;

      // 実際の信号を計算する
      double y = k00sum[sq_bk][sq_wk];
      for (int i = 0; i < EvalList::ListSize; ++i) {
        const int k0 = list0[i];
        const int k1 = list1[i];
        for (int j = 0; j < i; ++j) {
          const int l0 = list0[j];
          const int l1 = list1[j];
          y += kpp[sq_bk][k0][l0];
          y -= kpp[inverse(sq_wk)][k1][l1];
        }
        y += kkp[sq_bk][sq_wk][k0];
      }
      y += material * FVScale;

      // 教師信号と実際の信号から重みの増減分を計算する
      // TODO(nodchip): materialを更新する必要があるかどうか考える
      double delta = (double)teacher - y;
      double diff = alpha * delta / (iteration + 1);
      k00sumDelta[sq_bk][sq_wk] += diff;
      for (int i = 0; i < EvalList::ListSize; ++i) {
        const int k0 = list0[i];
        const int k1 = list1[i];
        for (int j = 0; j < i; ++j) {
          const int l0 = list0[j];
          const int l1 = list1[j];
          kppDelta[sq_bk][k0][l0] += diff;
          kppDelta[sq_bk][l0][k0] += diff;
          kppDelta[inverse(sq_wk)][k1][l1] -= diff;
          kppDelta[inverse(sq_wk)][l1][k1] -= diff;
        }
        kkpDelta[sq_bk][sq_wk][k0] += diff;
      }

      delta2 += delta * delta;
    }

    // 重みを更新する
    for (int i = 0; i < SquareNum; ++i) {
      for (int j = 0; j < SquareNum; ++j) {
        k00sum[i][j] += k00sumDelta[i][j];
      }
    }
    for (int i = 0; i < SquareNum; ++i) {
      for (int j = 0; j < fe_end; ++j) {
        for (int k = 0; k < fe_end; ++k) {
          kpp[i][j][k] += kppDelta[i][j][k];
        }
      }
    }
    for (int i = 0; i < SquareNum; ++i) {
      for (int j = 0; j < SquareNum; ++j) {
        for (int k = 0; k < fe_end; ++k) {
          kkp[i][j][k] += kkpDelta[i][j][k];
        }
      }
    }

    cout << setprecision(10) << "delta2=" << delta2 << " alpha=" << alpha << endl << endl;
  }

  // ローカル変数から重みを書き戻す
  double maxDiff = 0.0;
  double diff2 = 0.0;
  double threshold = 4.0;
  for (int i = 0; i < SquareNum; ++i) {
    for (int j = 0; j < SquareNum; ++j) {
      double diff = Evaluater::KK[i][j] - k00sum[i][j];
      maxDiff = max(maxDiff, abs(diff));
      diff2 += diff * diff;
      if (abs(diff) > threshold) {
        //printf("K00Sum[%d][%d] %6d -> %6d\n", i, j, K00Sum[i][j], (int)round(k00sum[i][j]));
      }
      Evaluater::KK[i][j] = round(k00sum[i][j]);
    }
  }
  for (int i = 0; i < SquareNum; ++i) {
    for (int j = 0; j < fe_end; ++j) {
      for (int k = 0; k < fe_end; ++k) {
        double diff = Evaluater::KPP[i][j][k] - kpp[i][j][k];
        maxDiff = max(maxDiff, abs(diff));
        diff2 += diff * diff;
        if (abs(diff) > threshold) {
          //printf("KPP[%d][%d][%d] %6d -> %6d\n", i, j, k, KPP[i][j][k], (int)round(kpp[i][j][k]));
        }
        Evaluater::KPP[i][j][k] = round(kpp[i][j][k]);
      }
    }
  }
  for (int i = 0; i < SquareNum; ++i) {
    for (int j = 0; j < SquareNum; ++j) {
      for (int k = 0; k < fe_end; ++k) {
        double diff = Evaluater::KKP[i][j][k] - kkp[i][j][k];
        maxDiff = max(maxDiff, abs(diff));
        diff2 += diff * diff;
        if (abs(diff) > threshold) {
          //printf("KKP[%d][%d][%d] %6d -> %6d\n", i, j, k, KKP[i][j][k], (int)round(kkp[i][j][k]));
        }
        Evaluater::KKP[i][j][k] = round(kkp[i][j][k]);
      }
    }
  }

  cout << "diff2=" << diff2 << " maxDiff=" << maxDiff << endl;

  return true;
}
