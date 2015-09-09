#include "string_util.hpp"

using namespace std;

std::vector<std::string> string_util::split(const std::string& in) {
  std::vector<std::string> out;
  istringstream iss(in);
  string word;
  while (iss >> word) {
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
