#ifndef SCANNER_HPP
#define SCANNER_HPP

#include <deque>
#include <string>
#include <vector>

// 文字列1行分を受け取り、先頭の単語から順に返すユーティリティクラス。
// 入力に連続した空白が含まれている場合は
// 1 つにまとめられたあとに処理される。
// java.util.Scanner のクローン
class Scanner
{
public:
  Scanner();
  Scanner(const char* input);
  Scanner(const std::string& input);
  Scanner(const std::vector<std::string>& input);
  void SetInput(const std::string& input);
  void SetInput(const std::vector<std::string>& input);
  bool hasNext() const;
  char hasNextChar() const;
  bool hasNextInt() const;
  std::string next();
  char nextChar();
  int nextInt();
private:
  std::deque<std::string> q;
};

#endif
