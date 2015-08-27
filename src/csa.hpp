#ifndef CSA_HPP
#define CSA_HPP

#include <filesystem>
#include <string>
#include <vector>
#include "position.hpp"

namespace csa {
  // CSAファイルをsfen形式へ変換する
  bool toSfen(const std::tr2::sys::path& filepath, std::vector<std::string>& sfen);

  // CSAファイルが勝負が終わっているかどうかを返す
  bool isFinished(const std::tr2::sys::path& filepath);

  // CSAファイル中でtanuki-が先手かどうかを返す
  bool isTanukiBlack(const std::tr2::sys::path& filepath);

  // CSAファイル中でどちらが勝ったかを返す
  // 引き分けの場合はColorNumを返す
  Color getWinner(const std::tr2::sys::path& filepath);

  // floodgateのCSAファイルをSFEN形式へ変換する
  bool convertCsaToSfen(
    const std::tr2::sys::path& inputDirectoryPath,
    const std::tr2::sys::path& outputFilePath);

  // 2chkifu.csa1をSFEN形式へ変換する
  bool convertCsa1LineToSfen(
    const std::tr2::sys::path& inputFilePath,
    const std::tr2::sys::path& outputFilePath);
}

#endif
