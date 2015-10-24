#ifndef APERY_USI_HPP
#define APERY_USI_HPP

#include "common.hpp"
#include "move.hpp"
#include "scanner.hpp"

const std::string DefaultStartPositionSFEN = "lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL b - 1";

struct OptionsMap;

namespace OptionNames
{
  constexpr char* USI_HASH = "USI_Hash";
  constexpr char* CLEAR_HASH = "Clear_Hash";
  constexpr char* BOOK_FILE = "Book_File";
  constexpr char* BEST_BOOK_MOVE = "Best_Book_Move";
  constexpr char* OWNBOOK = "OwnBook";
  constexpr char* MIN_BOOK_PLY = "Min_Book_Ply";
  constexpr char* MAX_BOOK_PLY = "Max_Book_Ply";
  constexpr char* MIN_BOOK_SCORE = "Min_Book_Score";
  constexpr char* EVAL_DIR = "Eval_Dir";
  constexpr char* WRITE_SYNTHESIZED_EVAL = "Write_Synthesized_Eval";
  constexpr char* USI_PONDER = "USI_Ponder";
  constexpr char* BYOYOMI_MARGIN = "Byoyomi_Margin";
  constexpr char* PONDER_TIME_MARGIN = "Ponder_Time_Margin";
  constexpr char* MULTIPV = "MultiPV";
  constexpr char* SKILL_LEVEL = "Skill_Level";
  constexpr char* MAX_RANDOM_SCORE_DIFF = "Max_Random_Score_Diff";
  constexpr char* MAX_RANDOM_SCORE_DIFF_PLY = "Max_Random_Score_Diff_Ply";
  constexpr char* EMERGENCY_MOVE_HORIZON = "Emergency_Move_Horizon";
  constexpr char* EMERGENCY_BASE_TIME = "Emergency_Base_Time";
  constexpr char* EMERGENCY_MOVE_TIME = "Emergency_Move_Time";
  constexpr char* SLOW_MOVER = "Slow_Mover";
  constexpr char* MINIMUM_THINKING_TIME = "Minimum_Thinking_Time";
  constexpr char* MAX_THREADS_PER_SPLIT_POINT = "Max_Threads_per_Split_Point";
  constexpr char* THREADS = "Threads";
  constexpr char* USE_SLEEPING_THREADS = "Use_Sleeping_Threads";
  constexpr char* DANGER_DEMERIT_SCORE = "Danger_Demerit_Score";
}

class USIOption {
  using Fn = void(Searcher*, const USIOption&);
public:
  USIOption(Fn* = nullptr, Searcher* s = nullptr);
  USIOption(const char* v, Fn* = nullptr, Searcher* s = nullptr);
  USIOption(const bool v, Fn* = nullptr, Searcher* s = nullptr);
  USIOption(const int v, const int min, const int max, Fn* = nullptr, Searcher* s = nullptr);

  USIOption& operator = (const std::string& v);

  operator int() const {
    assert(type_ == "check" || type_ == "spin");
    return (type_ == "spin" ? atoi(currentValue_.c_str()) : currentValue_ == "true");
  }

  operator std::string() const {
    assert(type_ == "string");
    return currentValue_;
  }

private:
  friend std::ostream& operator << (std::ostream&, const OptionsMap&);

  std::string defaultValue_;
  std::string currentValue_;
  std::string type_;
  int min_;
  int max_;
  Fn* onChange_;
  Searcher* searcher_;
};

struct CaseInsensitiveLess {
  bool operator() (const std::string&, const std::string&) const;
};

struct OptionsMap : public std::map<std::string, USIOption, CaseInsensitiveLess> {
public:
  void init(Searcher* s);
  bool isLegalOption(const std::string name) {
    return this->find(name) != std::end(*this);
  }
};

void go(const Position& pos, Scanner command);
void setPosition(Position& pos, Scanner command);
Move csaToMove(const Position& pos, const std::string& moveStr);
Move usiToMove(const Position& pos, const std::string& moveStr);

#endif // #ifndef APERY_USI_HPP
