#include "common.hpp"
#include "search.hpp"
#include "usi.hpp"
#include "timeManager.hpp"

namespace {
#if 1
  static const int MoveHorizon = 47; // 15分切れ負け用。
  static const double MaxRatio = 3.0; // 15分切れ負け用。
                              //const double MaxRatio = 5.0; // 15分 秒読み10秒用。
#else
  static const int MoveHorizon = 35; // 2時間切れ負け用。(todo: もう少し時間使っても良いかも知れない。)
  static const double MaxRatio = 5.0; // 2時間切れ負け用。
#endif
  static const double StealRatio = 0.33;
  // 序盤で本来の思考時間に対する割合
  static const double OPENING_GAME_SEARCH_TIME_COMPRESSION_RATIO = 0.2;

  // Stockfish とは異なる。
  static const int MoveImportance[512] = {
    7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780,
    7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780,
    7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780,
    7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780,
    7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780,
    7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780,
    7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780,
    7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780,
    7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780,
    7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780,
    7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780,
    7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780,
    7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780,
    7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780,
    7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780,
    7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780,
    7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780, 7780,
    7780, 7780, 7780, 7780, 7778, 7778, 7776, 7776, 7776, 7773, 7770, 7768, 7766, 7763, 7757, 7751,
    7743, 7735, 7724, 7713, 7696, 7689, 7670, 7656, 7627, 7605, 7571, 7549, 7522, 7493, 7462, 7425,
    7385, 7350, 7308, 7272, 7230, 7180, 7139, 7094, 7055, 7010, 6959, 6902, 6841, 6778, 6705, 6651,
    6569, 6508, 6435, 6378, 6323, 6253, 6152, 6085, 5995, 5931, 5859, 5794, 5717, 5646, 5544, 5462,
    5364, 5282, 5172, 5078, 4988, 4901, 4831, 4764, 4688, 4609, 4536, 4443, 4365, 4293, 4225, 4155,
    4085, 4005, 3927, 3844, 3765, 3693, 3634, 3560, 3479, 3404, 3331, 3268, 3207, 3146, 3077, 3011,
    2947, 2894, 2828, 2776, 2727, 2676, 2626, 2589, 2538, 2490, 2442, 2394, 2345, 2302, 2243, 2192,
    2156, 2115, 2078, 2043, 2004, 1967, 1922, 1893, 1845, 1809, 1772, 1736, 1702, 1674, 1640, 1605,
    1566, 1536, 1509, 1479, 1452, 1423, 1388, 1362, 1332, 1304, 1289, 1266, 1250, 1228, 1206, 1180,
    1160, 1134, 1118, 1100, 1080, 1068, 1051, 1034, 1012, 1001, 980 , 960 , 945 , 934 , 916 , 900 ,
    888 , 878 , 865 , 852 , 828 , 807 , 787 , 770 , 753 , 744 , 731 , 722 , 706 , 700 , 683 , 676 ,
    671 , 664 , 652 , 641 , 634 , 627 , 613 , 604 , 591 , 582 , 568 , 560 , 552 , 540 , 534 , 529 ,
    519 , 509 , 495 , 484 , 474 , 467 , 460 , 450 , 438 , 427 , 419 , 410 , 406 , 399 , 394 , 387 ,
    382 , 377 , 372 , 366 , 359 , 353 , 348 , 343 , 337 , 333 , 328 , 321 , 315 , 309 , 303 , 298 ,
    293 , 287 , 284 , 281 , 277 , 273 , 265 , 261 , 255 , 251 , 247 , 241 , 240 , 235 , 229 , 218
  };

  int moveImportance(const Ply ply) {
    return MoveImportance[std::min(ply, 511)];
  }

  enum TimeType {
    OptimumTime,
    MaxTime
  };

  double standardSigmoidFunction(double x) {
    return 1.0 / (1.0 + exp(-x));
  }

  template <TimeType T> int remaining(const int myTime, int movesToGo, Ply currentPly, int slowMover) {
    double TMaxRatio = (T == OptimumTime ? 1 : MaxRatio);
    double TStealRatio = (T == OptimumTime ? 0 : StealRatio);

    double thisMoveImportance = moveImportance(currentPly) * slowMover / 100;
    double otherMoveImportance = 0;

    for (int i = 1; i < movesToGo; ++i) {
      otherMoveImportance += moveImportance(currentPly + 2 * i);
    }

    double ratio1 =
      (TMaxRatio * thisMoveImportance) / static_cast<double>(TMaxRatio * thisMoveImportance + otherMoveImportance);
    double ratio2 =
      (thisMoveImportance + TStealRatio * otherMoveImportance) / static_cast<double>(thisMoveImportance + otherMoveImportance);

    return static_cast<int>(myTime * std::min(ratio1, ratio2));
  }
}

void TimeManager::pvInstability(int currChanges, int prevChanges) {
  unstablePVExtraTime_ =
    currChanges * (optimumSearchTime_ / 2) + prevChanges * (optimumSearchTime_ / 3);
}

void TimeManager::init(LimitsType& limits, Ply currentPly, Color us, Searcher* s) {
  int emergencyMoveHorizon = s->options["Emergency_Move_Horizon"];
  int emergencyBaseTime = s->options["Emergency_Base_Time"];
  int emergencyMoveTime = s->options["Emergency_Move_Time"];
  int minThinkingTime = s->options["Minimum_Thinking_Time"];
  int slowMover = s->options["Slow_Mover"];

  unstablePVExtraTime_ = 0;
  optimumSearchTime_ = maximumSearchTime_ = limits.time[us];

  for (int hypMTG = 1; hypMTG <= (limits.movesToGo ? std::min(limits.movesToGo, MoveHorizon) : MoveHorizon); ++hypMTG) {
    int hypMyTime =
      limits.time[us]
      + limits.increment[us] * (hypMTG - 1)
      - emergencyBaseTime
      - emergencyMoveTime + std::min(hypMTG, emergencyMoveHorizon);

    hypMyTime = std::max(hypMyTime, 0);

    int t1 = minThinkingTime + remaining<OptimumTime>(hypMyTime, hypMTG, currentPly, slowMover);
    int t2 = minThinkingTime + remaining<MaxTime>(hypMyTime, hypMTG, currentPly, slowMover);

    optimumSearchTime_ = std::min(optimumSearchTime_, t1);
    maximumSearchTime_ = std::min(maximumSearchTime_, t2);
  }

  if (s->options["USI_Ponder"]) {
    optimumSearchTime_ += optimumSearchTime_ / 4;
  }

  // こちらも minThinkingTime 以上にする。
  optimumSearchTime_ = std::max(optimumSearchTime_, minThinkingTime);
  optimumSearchTime_ = std::min(optimumSearchTime_, maximumSearchTime_);

  if (limits.moveTime != 0) {
    if (optimumSearchTime_ < limits.moveTime) {
      optimumSearchTime_ = std::min(limits.time[us], limits.moveTime);
    }
    if (maximumSearchTime_ < limits.moveTime) {
      maximumSearchTime_ = std::min(limits.time[us], limits.moveTime);
    }
    // TODO(nodchip): なぜ秒読み分を足しているのか？
    optimumSearchTime_ += limits.moveTime;
    maximumSearchTime_ += limits.moveTime;
    if (limits.time[us] != 0) {
      limits.moveTime = 0;
    }
  }

  // 序盤に時間を使わないようにする
  // 20手目: 本来の時間 * OPENING_GAME_SEARCH_TIME_COMPRESSION_RATIO
  // 20～44手目: シグモイド関数で補間
  // 44手目: 本来の時間
  double ratio = OPENING_GAME_SEARCH_TIME_COMPRESSION_RATIO;
  optimumSearchTime_ = (int)(optimumSearchTime_ * (standardSigmoidFunction((currentPly - 32) * 0.5) * (1.0 - ratio) + ratio));
  maximumSearchTime_ = (int)(maximumSearchTime_ * (standardSigmoidFunction((currentPly - 32) * 0.5) * (1.0 - ratio) + ratio));
  // ??500 ms に合わせる
  optimumSearchTime_ = (optimumSearchTime_ + 500) / 1000 * 1000 + 500;
  maximumSearchTime_ = (maximumSearchTime_ + 500) / 1000 * 1000 + 500;
  // こちらも minThinkingTime 以上にする。
  optimumSearchTime_ = std::max(optimumSearchTime_, minThinkingTime);
  maximumSearchTime_ = std::max(maximumSearchTime_, minThinkingTime);

  if (Searcher::outputInfo) {
    SYNCCOUT << "info string optimum_search_time = " << optimumSearchTime_ << SYNCENDL;
    SYNCCOUT << "info string maximum_search_time = " << maximumSearchTime_ << SYNCENDL;
  }
}
