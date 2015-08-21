#ifndef CSA_HPP
#define CSA_HPP

#include <filesystem>
#include <string>
#include <vector>
#include "position.hpp"

namespace csa {
  // CSAファイルをsfen形式へ変換する
  bool toSfen(const std::tr2::sys::path& filepath, std::vector<std::string>& sfen);
}

#endif
