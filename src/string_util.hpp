#ifndef STRING_UTIL_HPP
#define STRING_UTIL_HPP

namespace string_util {

  // 文字列をスペースで区切って文字列の配列に変換する
  void split(const std::string& in, std::vector<std::string>& out);

  // 文字列の配列をスペース区切りで結合する
  void concat(const std::vector<std::string>& words, std::string& out);

}

#endif
