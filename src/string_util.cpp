#include "string_util.hpp"

using namespace std;

std::vector<std::string> string_util::split(const std::string& in) {
  std::vector<std::string> out;
  string word;
  for (unsigned char ch : in) {
    if (isspace(ch)) {
      if (!word.empty()) {
        out.push_back(word);
        word.clear();
      }
    }
    else {
      word.push_back(ch);
    }
  }

  if (!word.empty()) {
    out.push_back(word);
  }

  return out;
}

std::string string_util::concat(const std::vector<std::string>& words) {
  string out;
  for (const auto& word : words) {
    if (!out.empty()) {
      out += " ";
    }
    out += word;
  }
  return out;
}
