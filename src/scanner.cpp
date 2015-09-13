#include <cassert>
#include "scanner.hpp"
#include "string_util.hpp"

using namespace std;

Scanner::Scanner() {
}

Scanner::Scanner(const char* input)
{
  SetInput(input);
}

Scanner::Scanner(const std::string& input) {
  SetInput(input);
}

Scanner::Scanner(const std::vector<std::string>& input) {
  SetInput(input);
}

void Scanner::SetInput(const std::string& input) {
  SetInput(string_util::split(input));
}

void Scanner::SetInput(const std::vector<std::string>& input) {
  q = deque<string>(input.begin(), input.end());
}

bool Scanner::hasNext() const {
  return !q.empty();
}

char Scanner::hasNextChar() const {
  if (q.empty()) {
    return false;
  }

  if (q.size() == 1 && q.front() == "") {
    return false;
  }

  return true;
}

bool Scanner::hasNextInt() const {
  if (q.empty()) {
    return false;
  }

  const string& word = q.front();
  if (word == "0") {
    return true;
  }
  return atoi(word.c_str()) != 0;
}

std::string Scanner::next() {
  assert(hasNext());
  string word = q.front();
  q.pop_front();
  return word;
}

int Scanner::nextInt() {
  assert(hasNextInt());
  int i = atoi(q.front().c_str());
  q.pop_front();
  return i;
}

char Scanner::nextChar() {
  assert(hasNextChar());
  char ch;
  if (q.front().empty()) {
    ch = ' ';
    q.pop_front();
  }
  else {
    ch = q.front().front();
    q.front().erase(0, 1);
  }
  return ch;
}
