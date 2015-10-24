#include "common.hpp"
#include "search.hpp"
#include "usi.hpp"
#include "timeManager.hpp"

namespace {
#if 1
  static constexpr int MoveHorizon = 47; // 15分切れ負け用。
  //static constexpr double MaxRatio = 3.0; // 15分切れ負け用。
  static constexpr double MaxRatio = 5.0; // 15分 秒読み10秒用。
#else
  static constexpr int MoveHorizon = 35; // 2時間切れ負け用。(todo: もう少し時間使っても良いかも知れない。)
  static constexpr double MaxRatio = 5.0; // 2時間切れ負け用。
#endif
  static constexpr double StealRatio = 0.33;

  // Stockfish とは異なる。
  static constexpr int MoveImportance[512] = {
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

  double sigmoidFunction(double x, double left, double right, double bottom, double top) {
    x = (x - left) / (right - left);
    x = x * 12.0 - 6.0;
    return standardSigmoidFunction(x) * (top - bottom) + bottom;
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

TimeManager::TimeManager(const LimitsType& limits, Ply currentPly, Color us, Searcher* searcher) :
  limits_(limits),
  currentPly_(currentPly),
  us_(us),
  searcher_(searcher),
  softTimeLimitMs_(0),
  hardTimeLimitMs_(0),
  unstablePVExtraTime_(0),
  backfootExtraTime_(0)
{
  update();
}

void TimeManager::update()
{
  int emergencyMoveHorizon = searcher_->options["Emergency_Move_Horizon"];
  int emergencyBaseTime = searcher_->options["Emergency_Base_Time"];
  int emergencyMoveTime = searcher_->options["Emergency_Move_Time"];
  int minThinkingTime = searcher_->options["Minimum_Thinking_Time"];
  int slowMover = searcher_->options["Slow_Mover"];
  int byoyomiMargin = searcher_->options["Byoyomi_Margin"];
  int byoyomi = limits_.byoyomi;
  int byoyomiWithMargin = std::max(0, limits_.byoyomi - byoyomiMargin);
  int ponderTime = limits_.ponderTime;
  int ponderTimeMargin = searcher_->options["Ponder_Time_Margin"];
  int ponderTimeWithMargin = std::max(0, ponderTime - ponderTimeMargin);
  // 将棋所上で双方が秒読みに入った時に ponderTime > byoyomi となる場合があるので
  // 秒読み時間以下に設定する
  if (limits_.time[Black] == 0 && limits_.time[White] == 0 && limits_.byoyomi > 0) {
    ponderTimeWithMargin = std::min(ponderTimeWithMargin, byoyomiWithMargin);
  }
  int myTime = limits_.time[us_];

  // 持ち時間+秒読み+ponder時間で初期化する
  int softTimeLimitMs = myTime + byoyomiWithMargin + ponderTimeWithMargin;
  int hardTimeLimitMs = myTime + byoyomiWithMargin + ponderTimeWithMargin;

  for (int hypMTG = 1; hypMTG <= (limits_.movesToGo ? std::min((int)limits_.movesToGo, MoveHorizon) : MoveHorizon); ++hypMTG) {
    int hypMyTime =
      myTime
      + limits_.increment[us_] * (hypMTG - 1)
      + byoyomiWithMargin
      + ponderTimeWithMargin
      - emergencyBaseTime
      - emergencyMoveTime + std::min(hypMTG, emergencyMoveHorizon);

    hypMyTime = std::max(hypMyTime, 0);

    int t1 = minThinkingTime + remaining<OptimumTime>(hypMyTime, hypMTG, currentPly_, slowMover);
    int t2 = minThinkingTime + remaining<MaxTime>(hypMyTime, hypMTG, currentPly_, slowMover);

    softTimeLimitMs = std::min(softTimeLimitMs, t1);
    hardTimeLimitMs = std::min(hardTimeLimitMs, t2);
  }

  if (searcher_->options["USI_Ponder"]) {
    softTimeLimitMs += softTimeLimitMs / 4;
  }

  // 後半で3～4秒程度しか読まなくなるのを防ぐため
  // 秒読みの時間以上に設定する
  softTimeLimitMs = std::max(softTimeLimitMs, byoyomiWithMargin);
  hardTimeLimitMs = std::max(hardTimeLimitMs, byoyomiWithMargin);

  // 序盤に時間を使わないようにする
  // 20手目: 本来の時間 * OPENING_GAME_SEARCH_TIME_COMPRESSION_RATIO
  // 20～44手目: シグモイド関数で補間
  // 44手目: 本来の時間
  double left = 20;
  double right = 44;
  double bottom = 1.0 / 3.0;
  double top = byoyomi != 0 ? 1.75 : 1.0;
  double sig = sigmoidFunction(currentPly_, left, right, bottom, top);
  softTimeLimitMs = (int)(softTimeLimitMs * sig);
  hardTimeLimitMs = (int)(hardTimeLimitMs * sig);

  // 持ち時間を使いきっている場合は
  // 秒読みギリギリまで利用する
  if (softTimeLimitMs >= myTime) {
    softTimeLimitMs = myTime + byoyomiWithMargin + ponderTimeWithMargin;
  }
  if (hardTimeLimitMs >= myTime) {
    hardTimeLimitMs = myTime + byoyomiWithMargin + ponderTimeWithMargin;
  }

  //// 秒節約のため hard time limit を ??500 ms に合わせる
  //// 探索のiterationがいつ終わるかわからないので soft hard limit は合わせない
  //hardTimeLimitMs = hardTimeLimitMs / 1000 * 1000 + 500;

  // soft > hard にならないようにする
  softTimeLimitMs = std::min(softTimeLimitMs, hardTimeLimitMs);

  // こちらも minThinkingTime 以上にする。
  softTimeLimitMs = std::max(softTimeLimitMs, minThinkingTime);
  hardTimeLimitMs = std::max(hardTimeLimitMs, minThinkingTime);

  // 時間切れにならないよう思考時間を制限する
  softTimeLimitMs = std::min(softTimeLimitMs, myTime + byoyomiWithMargin + ponderTimeWithMargin);
  hardTimeLimitMs = std::min(hardTimeLimitMs, myTime + byoyomiWithMargin + ponderTimeWithMargin);

  // 切れ負けで残り時間が足りない場合は最低思考時間とする
  if (byoyomi == 0 && (256 - currentPly_) / 2 * 1000 + 10 * 1000 > myTime) {
    softTimeLimitMs = minThinkingTime;
    hardTimeLimitMs = minThinkingTime;
  }

  if (Searcher::outputInfo) {
    char buffer[1024];
    sprintf(buffer, "info string soft_time_limit_ms=%d hard_time_limit_ms=%d btime=%d wtime=%d byoyomi=%d ponderTime=%d",
      softTimeLimitMs, hardTimeLimitMs, limits_.time[Black].load(), limits_.time[White].load(), byoyomi, ponderTime);
    SYNCCOUT << buffer << SYNCENDL;
    //SYNCCOUT << limits_.outputInfoString() << SYNCENDL;
  }
  softTimeLimitMs_ = softTimeLimitMs;
  hardTimeLimitMs_ = hardTimeLimitMs;
}

void TimeManager::setSearchStatus(int currentChanges, int previousChanges, Score score) {
  unstablePVExtraTime_ = currentChanges * (softTimeLimitMs_ / 2)
    + previousChanges * (softTimeLimitMs_ / 3);

  double left = 0.0;
  double right = -800.0;
  double bottom = 0.0;
  double top = 1.0;
  double sig = sigmoidFunction(score, left, right, bottom, top);
  backfootExtraTime_ = int(softTimeLimitMs_ * sig);

  //if (Searcher::outputInfo) {
  //  char buffer[1024];
  //  sprintf(buffer, "info string soft_time_limit_ms=%d hard_time_limit_ms=%d unstable_pv_extra_time=%d backfoot_extra_time=%d",
  //    softTimeLimitMs_.load(),
  //    hardTimeLimitMs_.load(),
  //    unstablePVExtraTime_.load(),
  //    backfootExtraTime_.load());
  //  SYNCCOUT << buffer << SYNCENDL;
  //  //SYNCCOUT << limits_.outputInfoString() << SYNCENDL;
  //}
}
