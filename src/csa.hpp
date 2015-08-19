#ifndef CSA_HPP
#define CSA_HPP

#include <string>
#include <vector>
#include "position.hpp"

namespace csa {
  // CSAファイルをsfen形式へ変換する
  bool toSfen(const std::string& filepath, std::vector<std::string>& sfen);
}

#endif
