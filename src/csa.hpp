#ifndef CSA_HPP
#define CSA_HPP

#include <filesystem>
#include <string>
#include <vector>

#include "position.hpp"
#include "game_record.hpp"

namespace csa {
  extern const std::tr2::sys::path DEFAULT_INPUT_CSA1_FILE_PATH;
  extern const std::tr2::sys::path DEFAULT_OUTPUT_SFEN_FILE_PATH;

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
    const std::tr2::sys::path& inputFilePath = DEFAULT_INPUT_CSA1_FILE_PATH,
    const std::tr2::sys::path& outputFilePath = DEFAULT_OUTPUT_SFEN_FILE_PATH);

  // CSAファイルを読み込む
  bool readCsa(const std::tr2::sys::path& filepath, GameRecord& gameRecord);

  // サブディレクトリも含めてCSAファイルを読み込む
  // filterがtrueとなるファイルのみ処理する
  bool readCsas(
    const std::tr2::sys::path& directory,
    const std::function<bool(const std::tr2::sys::path&)>& pathFilter,
    const std::function<bool(const GameRecord&)>& gameRecordFilter,
    std::vector<GameRecord>& gameRecords);

  // CSA1ファイルを読み込む
  bool readCsa1(
    const std::tr2::sys::path& filepath,
    std::vector<GameRecord>& gameRecords);

  // CSA1ファイルを保存する
  bool writeCsa1(
    const std::tr2::sys::path& filepath,
    const std::vector<GameRecord>& gameRecords);

  // CSA1ファイルをマージする
  bool mergeCsa1s(
    const std::vector<std::tr2::sys::path>& inputFilepaths,
    const std::tr2::sys::path& outputFilepath);
}

#endif
