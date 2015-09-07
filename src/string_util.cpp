#include "string_util.hpp"

using namespace std;

void string_util::split(const std::string& in, std::vector<std::string>& out) {
  out.clear();
  istringstream iss(in);
  string word;
  while (iss >> word) {
    out.push_back(word);
  }
}

void string_util::concat(const std::vector<std::string>& words, std::string& out) {
  out.clear();
  for (const auto& word : words) {
    if (!out.empty()) {
      out += " ";
    }
    out += word;
  }
}
