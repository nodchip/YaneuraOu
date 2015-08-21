#ifndef HAYABUSA_HPP
#define HAYABUSA_HPP

#include <string>

namespace hayabusa {
  extern const std::tr2::sys::path DEFAULT_INPUT_DIRECTORY_PATH;
  extern const std::tr2::sys::path DEFAULT_OUTPUT_DIRECTORY_PATH;

  // 後の大反省のために入力の各盤面の評価値を含むキャッシュファイルを作成する
  bool createEvaluationCache(
    const std::tr2::sys::path& inputDirectoryPath = DEFAULT_INPUT_DIRECTORY_PATH,
    const std::tr2::sys::path& outputDirectoryPath = DEFAULT_OUTPUT_DIRECTORY_PATH,
    int maxNumberOfPlays = INT_MAX);
}

#endif
