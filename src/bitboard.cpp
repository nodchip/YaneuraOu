#include "common.hpp"
#include "bitboard.hpp"

const Bitboard SetMaskBB[SquareNum] = {
  Bitboard(UINT64_C(1) << 0,                 0),  // 0 , I9
  Bitboard(UINT64_C(1) << 1,                 0),  // 1 , I8
  Bitboard(UINT64_C(1) << 2,                 0),  // 2 , I7
  Bitboard(UINT64_C(1) << 3,                 0),  // 3 , I6
  Bitboard(UINT64_C(1) << 4,                 0),  // 4 , I5
  Bitboard(UINT64_C(1) << 5,                 0),  // 5 , I4
  Bitboard(UINT64_C(1) << 6,                 0),  // 6 , I3
  Bitboard(UINT64_C(1) << 7,                 0),  // 7 , I2
  Bitboard(UINT64_C(1) << 8,                 0),  // 8 , I1
  Bitboard(UINT64_C(1) << 9,                 0),  // 9 , H9
  Bitboard(UINT64_C(1) << 10,                 0),  // 10, H8
  Bitboard(UINT64_C(1) << 11,                 0),  // 11, H7
  Bitboard(UINT64_C(1) << 12,                 0),  // 12, H6
  Bitboard(UINT64_C(1) << 13,                 0),  // 13, H5
  Bitboard(UINT64_C(1) << 14,                 0),  // 14, H4
  Bitboard(UINT64_C(1) << 15,                 0),  // 15, H3
  Bitboard(UINT64_C(1) << 16,                 0),  // 16, H2
  Bitboard(UINT64_C(1) << 17,                 0),  // 17, H1
  Bitboard(UINT64_C(1) << 18,                 0),  // 18, G9
  Bitboard(UINT64_C(1) << 19,                 0),  // 19, G8
  Bitboard(UINT64_C(1) << 20,                 0),  // 20, G7
  Bitboard(UINT64_C(1) << 21,                 0),  // 21, G6
  Bitboard(UINT64_C(1) << 22,                 0),  // 22, G5
  Bitboard(UINT64_C(1) << 23,                 0),  // 23, G4
  Bitboard(UINT64_C(1) << 24,                 0),  // 24, G3
  Bitboard(UINT64_C(1) << 25,                 0),  // 25, G2
  Bitboard(UINT64_C(1) << 26,                 0),  // 26, G1
  Bitboard(UINT64_C(1) << 27,                 0),  // 27, F9
  Bitboard(UINT64_C(1) << 28,                 0),  // 28, F8
  Bitboard(UINT64_C(1) << 29,                 0),  // 29, F7
  Bitboard(UINT64_C(1) << 30,                 0),  // 30, F6
  Bitboard(UINT64_C(1) << 31,                 0),  // 31, F5
  Bitboard(UINT64_C(1) << 32,                 0),  // 32, F4
  Bitboard(UINT64_C(1) << 33,                 0),  // 33, F3
  Bitboard(UINT64_C(1) << 34,                 0),  // 34, F2
  Bitboard(UINT64_C(1) << 35,                 0),  // 35, F1
  Bitboard(UINT64_C(1) << 36,                 0),  // 36, E9
  Bitboard(UINT64_C(1) << 37,                 0),  // 37, E8
  Bitboard(UINT64_C(1) << 38,                 0),  // 38, E7
  Bitboard(UINT64_C(1) << 39,                 0),  // 39, E6
  Bitboard(UINT64_C(1) << 40,                 0),  // 40, E5
  Bitboard(UINT64_C(1) << 41,                 0),  // 41, E4
  Bitboard(UINT64_C(1) << 42,                 0),  // 42, E3
  Bitboard(UINT64_C(1) << 43,                 0),  // 43, E2
  Bitboard(UINT64_C(1) << 44,                 0),  // 44, E1
  Bitboard(UINT64_C(1) << 45,                 0),  // 45, D9
  Bitboard(UINT64_C(1) << 46,                 0),  // 46, D8
  Bitboard(UINT64_C(1) << 47,                 0),  // 47, D7
  Bitboard(UINT64_C(1) << 48,                 0),  // 48, D6
  Bitboard(UINT64_C(1) << 49,                 0),  // 49, D5
  Bitboard(UINT64_C(1) << 50,                 0),  // 50, D4
  Bitboard(UINT64_C(1) << 51,                 0),  // 51, D3
  Bitboard(UINT64_C(1) << 52,                 0),  // 52, D2
  Bitboard(UINT64_C(1) << 53,                 0),  // 53, D1
  Bitboard(UINT64_C(1) << 54,                 0),  // 54, C9
  Bitboard(UINT64_C(1) << 55,                 0),  // 55, C8
  Bitboard(UINT64_C(1) << 56,                 0),  // 56, C7
  Bitboard(UINT64_C(1) << 57,                 0),  // 57, C6
  Bitboard(UINT64_C(1) << 58,                 0),  // 58, C5
  Bitboard(UINT64_C(1) << 59,                 0),  // 59, C4
  Bitboard(UINT64_C(1) << 60,                 0),  // 60, C3
  Bitboard(UINT64_C(1) << 61,                 0),  // 61, C2
  Bitboard(UINT64_C(1) << 62,                 0),  // 62, C1
  Bitboard(0, UINT64_C(1) << 0),  // 63, B9
  Bitboard(0, UINT64_C(1) << 1),  // 64, B8
  Bitboard(0, UINT64_C(1) << 2),  // 65, B7
  Bitboard(0, UINT64_C(1) << 3),  // 66, B6
  Bitboard(0, UINT64_C(1) << 4),  // 67, B5
  Bitboard(0, UINT64_C(1) << 5),  // 68, B4
  Bitboard(0, UINT64_C(1) << 6),  // 69, B3
  Bitboard(0, UINT64_C(1) << 7),  // 70, B2
  Bitboard(0, UINT64_C(1) << 8),  // 71, B1
  Bitboard(0, UINT64_C(1) << 9),  // 72, A9
  Bitboard(0, UINT64_C(1) << 10),  // 73, A8
  Bitboard(0, UINT64_C(1) << 11),  // 74, A7
  Bitboard(0, UINT64_C(1) << 12),  // 75, A6
  Bitboard(0, UINT64_C(1) << 13),  // 76, A5
  Bitboard(0, UINT64_C(1) << 14),  // 77, A4
  Bitboard(0, UINT64_C(1) << 15),  // 78, A3
  Bitboard(0, UINT64_C(1) << 16),  // 79, A2
  Bitboard(0, UINT64_C(1) << 17)   // 80, A1
};

const Bitboard FileMask[FileNum] = {
  FileIMask, FileHMask, FileGMask, FileFMask, FileEMask, FileDMask, FileCMask, FileBMask, FileAMask
};

const Bitboard RankMask[RankNum] = {
  Rank9Mask, Rank8Mask, Rank7Mask, Rank6Mask, Rank5Mask, Rank4Mask, Rank3Mask, Rank2Mask, Rank1Mask
};

const Bitboard InFrontMask[ColorNum][RankNum] = {
  { InFrontOfRank9Black, InFrontOfRank8Black, InFrontOfRank7Black, InFrontOfRank6Black, InFrontOfRank5Black, InFrontOfRank4Black, InFrontOfRank3Black, InFrontOfRank2Black, InFrontOfRank1Black },
  { InFrontOfRank9White, InFrontOfRank8White, InFrontOfRank7White, InFrontOfRank6White, InFrontOfRank5White, InFrontOfRank4White, InFrontOfRank3White, InFrontOfRank2White, InFrontOfRank1White }
};

// これらは一度値を設定したら二度と変更しない。
// 本当は const 化したい。
#if defined HAVE_BMI2
Bitboard RookAttack[495616];
#else
Bitboard RookAttack[512000];
#endif
int RookAttackIndex[SquareNum];
Bitboard RookBlockMask[SquareNum];
Bitboard BishopAttack[20224];
int BishopAttackIndex[SquareNum];
Bitboard BishopBlockMask[SquareNum];
Bitboard LanceAttack[ColorNum][SquareNum][128];

Bitboard KingAttack[SquareNum];
Bitboard GoldAttack[ColorNum][SquareNum];
Bitboard SilverAttack[ColorNum][SquareNum];
Bitboard KnightAttack[ColorNum][SquareNum];
Bitboard PawnAttack[ColorNum][SquareNum];

Bitboard BetweenBB[SquareNum][SquareNum];

Bitboard RookAttackToEdge[SquareNum];
Bitboard BishopAttackToEdge[SquareNum];
Bitboard LanceAttackToEdge[ColorNum][SquareNum];

Bitboard GoldCheckTable[ColorNum][SquareNum];
Bitboard SilverCheckTable[ColorNum][SquareNum];
Bitboard KnightCheckTable[ColorNum][SquareNum];
Bitboard LanceCheckTable[ColorNum][SquareNum];
