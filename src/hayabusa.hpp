#ifndef HAYABUSA_HPP
#define HAYABUSA_HPP

#include "evalList.hpp"

namespace hayabusa {
  struct TeacherData {
    int list0[EvalList::ListSize];
    int list1[EvalList::ListSize];
    // 線形重回帰分析の教師信号
    // この値に近づくようにKPPとKKPの値を調整する
    int teacher;
  };

  extern const std::tr2::sys::path DEFAULT_INPUT_CSA_DIRECTORY_PATH;
  extern const std::tr2::sys::path DEFAULT_OUTPUT_TEACHER_FILE_PATH;

  // HAYABUSA学習メソッドで使用する教師データを作成する
  bool createTeacherData(
    const std::tr2::sys::path& inputCsaDirectoryPath = DEFAULT_INPUT_CSA_DIRECTORY_PATH,
    const std::tr2::sys::path& outputTeacherFilePath = DEFAULT_OUTPUT_TEACHER_FILE_PATH,
    int maxNumberOfPlays = INT_MAX);
}

#endif
