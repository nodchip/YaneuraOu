#ifndef APERY_LIMITS_TYPE_HPP
#define APERY_LIMITS_TYPE_HPP

#include <atomic>

#include "color.hpp"
#include "common.hpp"
#include "score.hpp"

// 時間や探索深さの制限を格納する為の構造体
struct LimitsType {
  // コマンド受け取りスレッドから変更され
  // メインスレッドで読まれるため std::atomic<> をつける
  std::atomic<int> time[ColorNum];
  std::atomic<int> increment[ColorNum];
  std::atomic<int> movesToGo;
  std::atomic<Ply> depth;
  std::atomic<u32> nodes;
  std::atomic<int> byoyomi;
  std::atomic<int> ponderTime;
  std::atomic<bool> infinite;
  std::atomic<bool> ponder;

  LimitsType();
  void set(const LimitsType& rh);
  std::string outputInfoString() const;
};

#endif
