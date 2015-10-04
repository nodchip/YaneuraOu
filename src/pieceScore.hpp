#ifndef APERY_PIECESCORE_HPP
#define APERY_PIECESCORE_HPP

#include "score.hpp"
#include "piece.hpp"

constexpr Score PawnScore = static_cast<Score>(100 * 9 / 10);
constexpr Score LanceScore = static_cast<Score>(350 * 9 / 10);
constexpr Score KnightScore = static_cast<Score>(450 * 9 / 10);
constexpr Score SilverScore = static_cast<Score>(550 * 9 / 10);
constexpr Score GoldScore = static_cast<Score>(600 * 9 / 10);
constexpr Score BishopScore = static_cast<Score>(950 * 9 / 10);
constexpr Score RookScore = static_cast<Score>(1100 * 9 / 10);
constexpr Score ProPawnScore = static_cast<Score>(600 * 9 / 10);
constexpr Score ProLanceScore = static_cast<Score>(600 * 9 / 10);
constexpr Score ProKnightScore = static_cast<Score>(600 * 9 / 10);
constexpr Score ProSilverScore = static_cast<Score>(600 * 9 / 10);
constexpr Score HorseScore = static_cast<Score>(1050 * 9 / 10);
constexpr Score DragonScore = static_cast<Score>(1550 * 9 / 10);

constexpr Score KingScore = static_cast<Score>(15000);

constexpr Score CapturePawnScore = PawnScore * 2;
constexpr Score CaptureLanceScore = LanceScore * 2;
constexpr Score CaptureKnightScore = KnightScore * 2;
constexpr Score CaptureSilverScore = SilverScore * 2;
constexpr Score CaptureGoldScore = GoldScore * 2;
constexpr Score CaptureBishopScore = BishopScore * 2;
constexpr Score CaptureRookScore = RookScore * 2;
constexpr Score CaptureProPawnScore = ProPawnScore + PawnScore;
constexpr Score CaptureProLanceScore = ProLanceScore + LanceScore;
constexpr Score CaptureProKnightScore = ProKnightScore + KnightScore;
constexpr Score CaptureProSilverScore = ProSilverScore + SilverScore;
constexpr Score CaptureHorseScore = HorseScore + BishopScore;
constexpr Score CaptureDragonScore = DragonScore + RookScore;
constexpr Score CaptureKingScore = KingScore * 2;

constexpr Score PromotePawnScore = ProPawnScore - PawnScore;
constexpr Score PromoteLanceScore = ProLanceScore - LanceScore;
constexpr Score PromoteKnightScore = ProKnightScore - KnightScore;
constexpr Score PromoteSilverScore = ProSilverScore - SilverScore;
constexpr Score PromoteBishopScore = HorseScore - BishopScore;
constexpr Score PromoteRookScore = DragonScore - RookScore;

constexpr Score ScoreKnownWin = KingScore;

constexpr Score PieceScore[PieceNone] = {
  ScoreZero,
  PawnScore, LanceScore, KnightScore, SilverScore, BishopScore, RookScore, GoldScore,
  ScoreZero, // King
  ProPawnScore, ProLanceScore, ProKnightScore, ProSilverScore, HorseScore, DragonScore,
  ScoreZero, ScoreZero,
  PawnScore, LanceScore, KnightScore, SilverScore, BishopScore, RookScore, GoldScore,
  ScoreZero, // King
  ProPawnScore, ProLanceScore, ProKnightScore, ProSilverScore, HorseScore, DragonScore,
};
constexpr Score CapturePieceScore[PieceNone] = {
  ScoreZero,
  CapturePawnScore, CaptureLanceScore, CaptureKnightScore, CaptureSilverScore, CaptureBishopScore, CaptureRookScore, CaptureGoldScore,
  ScoreZero, // King
  CaptureProPawnScore, CaptureProLanceScore, CaptureProKnightScore, CaptureProSilverScore, CaptureHorseScore, CaptureDragonScore,
  ScoreZero, ScoreZero,
  CapturePawnScore, CaptureLanceScore, CaptureKnightScore, CaptureSilverScore, CaptureBishopScore, CaptureRookScore, CaptureGoldScore,
  ScoreZero, // King
  CaptureProPawnScore, CaptureProLanceScore, CaptureProKnightScore, CaptureProSilverScore, CaptureHorseScore, CaptureDragonScore,
};
constexpr Score PromotePieceScore[7] = {
  ScoreZero,
  PromotePawnScore, PromoteLanceScore, PromoteKnightScore,
  PromoteSilverScore, PromoteBishopScore, PromoteRookScore
};

#endif // #ifndef APERY_PIECESCORE_HPP
