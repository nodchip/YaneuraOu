#ifndef APERY_SCORE_HPP
#define APERY_SCORE_HPP

#include "overloadEnumOperators.hpp"
#include "common.hpp"

using Ply = int;

constexpr Ply MaxPly = 128;
constexpr Ply MaxPlyPlus2 = MaxPly + 2;

enum Bound {
  BoundNone = 0,
  BoundUpper = Binary< 1>::value, // fail low  で正しい score が分からない。alpha 以下が確定という意味。
  BoundLower = Binary<10>::value, // fail high で正しい score が分からない。beta 以上が確定という意味。
  BoundExact = Binary<11>::value  // alpha と beta の間に score がある。
};

inline bool exactOrLower(const Bound st) {
  return (st & BoundLower ? true : false);
}
inline bool exactOrUpper(const Bound st) {
  return (st & BoundUpper ? true : false);
}

// 評価値
enum Score {
  ScoreZero = 0,
  ScoreDraw = 0,
  ScoreMaxEvaluate = 30000,
  ScoreMateLong = 30002,
  ScoreMate1Ply = 32599,
  ScoreMate0Ply = 32600,
  ScoreMateInMaxPly = ScoreMate0Ply - MaxPly,
  ScoreMated0Ply = -ScoreMate0Ply,
  ScoreMatedInMaxPly = -ScoreMateInMaxPly,
  ScoreSuperior0Ply = ScoreMateInMaxPly - 1,
  ScoreSuperiorMaxPly = ScoreSuperior0Ply - MaxPly,
  ScoreInferior0Ply = -ScoreSuperior0Ply,
  ScoreInferiorMaxPly = -ScoreSuperiorMaxPly,
  ScoreInfinite = 32601,
  ScoreNotEvaluated = INT_MAX,
  ScoreNone = 32602
};
OverloadEnumOperators(Score);

inline Score mateIn(Ply ply) {
  return ScoreMate0Ply - static_cast<Score>(ply);
}
inline Score matedIn(Ply ply) {
  return -ScoreMate0Ply + static_cast<Score>(ply);
}
inline Score superiorIn(Ply ply) {
  return ScoreSuperior0Ply - static_cast<Score>(ply);
}
inline Score inferiorIn(Ply ply) {
  return ScoreInferior0Ply + static_cast<Score>(ply);
}
inline bool isMate(Score score) {
  return ScoreMateInMaxPly <= score && score <= ScoreMate0Ply;
}
inline bool isMated(Score score) {
  return ScoreMated0Ply <= score && score <= ScoreMatedInMaxPly;
}
inline bool isSuperior(Score score) {
  return ScoreSuperiorMaxPly <= score && score <= ScoreSuperior0Ply;
}
inline bool isInferior(Score score) {
  return ScoreInferior0Ply <= score && score <= ScoreInferiorMaxPly;
}


#endif // #ifndef APERY_SCORE_HPP
