#ifndef STRING_UTIL_HPP
#define STRING_UTIL_HPP

namespace string_util {

  // 文字列をスペースで区切って文字列の配列に変換する
  std::vector<std::string> split(const std::string& in);

  // 文字列の配列をスペース区切りで結合する
  std::string concat(const std::vector<std::string>& words);

}

#endif
