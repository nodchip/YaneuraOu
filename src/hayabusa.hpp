#ifndef HAYABUSA_HPP
#define HAYABUSA_HPP

#include "evalList.hpp"
#include "score.hpp"

namespace hayabusa {
  struct TeacherData {
    Square squareBlackKing;
    Square squareWhiteKing;
    int list0[EvalList::ListSize];
    int list1[EvalList::ListSize];
    Score material;
    // 線形重回帰分析の教師信号
    // この値に近づくようにKPPとKKPの値を調整する
    Score teacher;
  };

  extern const std::tr2::sys::path DEFAULT_INPUT_CSA_DIRECTORY_PATH;
  extern const std::tr2::sys::path DEFAULT_OUTPUT_TEACHER_DATA_FILE_PATH;
  extern const std::tr2::sys::path DEFAULT_INPUT_TEACHER_DATA_FILE_PATH;
  extern const std::tr2::sys::path DEFAULT_INPUT_SHOGIDOKORO_CSA_DIRECTORY_PATH;

  // HAYABUSA学習メソッドで使用する教師データを作成する
  // inputCsaDirectoryPath CSAファイルが含まれたディレクトリパス
  // outputTeacherDataFilePath 教師データファイルパス
  // maxNumberOfPlays 処理する最大局面数
  bool createTeacherData(
    const std::tr2::sys::path& inputCsaDirectoryPath = DEFAULT_INPUT_CSA_DIRECTORY_PATH,
    const std::tr2::sys::path& outputTeacherDataFilePath = DEFAULT_OUTPUT_TEACHER_DATA_FILE_PATH,
    int maxNumberOfPlays = INT_MAX);

  // 将棋所の自己対戦結果を教師データに追加する
  // inputShogidokoroCsaDirectoryPath 将棋所の出力したCSAファイルが含まれたディレクトリパス
  // outputTeacherFilePath 更新される教師データファイルパス
  // maxNumberOfPlays 処理する最大局面数
  bool addTeacherData(
    const std::tr2::sys::path& inputShogidokoroCsaDirectoryPath = DEFAULT_INPUT_SHOGIDOKORO_CSA_DIRECTORY_PATH,
    const std::tr2::sys::path& outputTeacherFilePath = DEFAULT_OUTPUT_TEACHER_DATA_FILE_PATH,
    int maxNumberOfPlays = INT_MAX);

  // HAYABUSA学習メソッドで重みを調整する
  // inputTeacherFilePath 教師データファイルパス
  // numberOfIterations 
  bool adjustWeights(
    const std::tr2::sys::path& inputTeacherFilePath = DEFAULT_INPUT_TEACHER_DATA_FILE_PATH,
    int numberOfIterations = 20);
}

#endif
