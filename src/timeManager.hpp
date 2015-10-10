#ifndef APERY_TIMEMANAGER_HPP
#define APERY_TIMEMANAGER_HPP

#include "evaluate.hpp"
#include "limits_type.hpp"
#include "thread.hpp"

struct LimitsType;

class TimeManager {
public:
  TimeManager(const LimitsType& limits, Ply currentPly, Color us, Searcher* searcher);
  void update();
  int getSoftTimeLimitMs() const {
    return softTimeLimitMs_ + unstablePVExtraTime_;
  }
  int getHardTimeLimitMs() const { return hardTimeLimitMs_; }
  void setPvInstability(int currChanges, int prevChanges);
  // 持ち時間が残っている場合は true
  // そうでない場合は false
  bool isTimeLeft() const {
    return limits_.time[us_] != 0;
  }
  // 持ち時間を使いきって秒読みに入った場合は true
  // そうでない場合は false
  bool isInByoyomi() const {
    return limits_.time[us_] == 0 &&
      limits_.byoyomi != 0;
  }

private:

  const LimitsType& limits_;
  const Ply currentPly_;
  const Color us_;
  const Searcher* searcher_;
  std::atomic<int> softTimeLimitMs_;
  std::atomic<int> hardTimeLimitMs_;
  std::atomic<int> unstablePVExtraTime_;
};

#endif // #ifndef APERY_TIMEMANAGER_HPP
