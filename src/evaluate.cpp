#include "evaluate.hpp"
#include "position.hpp"
#include "search.hpp"
#include "thread.hpp"

KPPBoardIndexStartToPiece g_kppBoardIndexStartToPiece;

std::array<s16, 2> Evaluater::KPP[SquareNum][fe_end][fe_end];
std::array<s32, 2> Evaluater::KKP[SquareNum][SquareNum][fe_end];
std::array<s32, 2> Evaluater::KK[SquareNum][SquareNum];

#if defined(OUTPUT_EVALUATE_HASH_HIT_RATE)
std::atomic<u64> Evaluater::numberOfHits;
std::atomic<u64> Evaluater::numberOfMissHits;
#elif defined(OUTPUT_EVALUATE_HASH_EXPIRATION_RATE)
std::atomic<u64> Evaluater::numberOfEvaluations;
std::atomic<u64> Evaluater::numberOfExpirations;
#endif


#if defined USE_K_FIX_OFFSET
const s32 Evaluater::K_Fix_Offset[SquareNum] = {
  2000 * FVScale, 1700 * FVScale, 1350 * FVScale, 1000 * FVScale,  650 * FVScale,  350 * FVScale,  100 * FVScale,    0 * FVScale,    0 * FVScale,
  1800 * FVScale, 1500 * FVScale, 1250 * FVScale, 1000 * FVScale,  650 * FVScale,  350 * FVScale,  100 * FVScale,    0 * FVScale,    0 * FVScale,
  1800 * FVScale, 1500 * FVScale, 1250 * FVScale, 1000 * FVScale,  650 * FVScale,  350 * FVScale,  100 * FVScale,    0 * FVScale,    0 * FVScale,
  1700 * FVScale, 1400 * FVScale, 1150 * FVScale,  900 * FVScale,  550 * FVScale,  250 * FVScale,    0 * FVScale,    0 * FVScale,    0 * FVScale,
  1600 * FVScale, 1300 * FVScale, 1050 * FVScale,  800 * FVScale,  450 * FVScale,  150 * FVScale,    0 * FVScale,    0 * FVScale,    0 * FVScale,
  1700 * FVScale, 1400 * FVScale, 1150 * FVScale,  900 * FVScale,  550 * FVScale,  250 * FVScale,    0 * FVScale,    0 * FVScale,    0 * FVScale,
  1800 * FVScale, 1500 * FVScale, 1250 * FVScale, 1000 * FVScale,  650 * FVScale,  350 * FVScale,  100 * FVScale,    0 * FVScale,    0 * FVScale,
  1900 * FVScale, 1600 * FVScale, 1350 * FVScale, 1000 * FVScale,  650 * FVScale,  350 * FVScale,  100 * FVScale,    0 * FVScale,    0 * FVScale,
  2000 * FVScale, 1700 * FVScale, 1350 * FVScale, 1000 * FVScale,  650 * FVScale,  350 * FVScale,  100 * FVScale,    0 * FVScale,    0 * FVScale
};
#endif

EvaluateHashTable g_evalTable;

using xmm = __m128i;
using ymm = __m256i;

namespace {
#ifdef HAVE_AVX2
  static const __m256i MASK[9] = {
    _mm256_setzero_si256(),
    _mm256_set_epi32(0, 0, 0, 0, 0, 0, 0, -1),
    _mm256_set_epi32(0, 0, 0, 0, 0, 0, -1, -1),
    _mm256_set_epi32(0, 0, 0, 0, 0, -1, -1, -1),
    _mm256_set_epi32(0, 0, 0, 0, -1, -1, -1, -1),
    _mm256_set_epi32(0, 0, 0, -1, -1, -1, -1, -1),
    _mm256_set_epi32(0, 0, -1, -1, -1, -1, -1, -1),
    _mm256_set_epi32(0, -1, -1, -1, -1, -1, -1, -1),
    _mm256_set_epi32(-1, -1, -1, -1, -1, -1, -1, -1),
  };
#endif

  EvalSum doapc(const Position& pos, int index0, int index1) {
    const Square sq_bk = pos.kingSquare(Black);
    const Square sq_wk = pos.kingSquare(White);
    const int* list0 = pos.cplist0();
    const int* list1 = pos.cplist1();

    EvalSum sum;
    sum.p[2][0] = Evaluater::KKP[sq_bk][sq_wk][index0][0];
    sum.p[2][1] = Evaluater::KKP[sq_bk][sq_wk][index0][1];
    const auto* pkppb = Evaluater::KPP[sq_bk][index0];
    const auto* pkppw = Evaluater::KPP[inverse(sq_wk)][index1];
#if defined USE_AVX2_EVAL
    ymm zero = _mm256_setzero_si256();
    ymm sum0 = zero;
    ymm sum1 = zero;
    for (int i = 0; i < pos.nlist(); i += 8) {
      ymm index0 = _mm256_load_si256((const ymm*)&list0[i]);
      ymm index1 = _mm256_load_si256((const ymm*)&list1[i]);
      ymm mask = MASK[std::min(pos.nlist() - i, 8)];
      ymm kpp0 = _mm256_mask_i32gather_epi32(zero, (const int*)pkppb, index0, mask, 4);
      ymm kpp1 = _mm256_mask_i32gather_epi32(zero, (const int*)pkppw, index1, mask, 4);
      // デバッガをアタッチした場合にymmレジスタの上位128bitがクリアされるのを回避する
      // TODO(nodchip): 上位128ビットも使用する
      ymm kpp0lo = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(kpp0, 0));
      ymm kpp0lolo = _mm256_castsi128_si256(_mm256_extracti128_si256(kpp0lo, 0));
      ymm kpp0lohi = _mm256_castsi128_si256(_mm256_extracti128_si256(kpp0lo, 1));
      sum0 = _mm256_add_epi32(sum0, kpp0lolo);
      sum0 = _mm256_add_epi32(sum0, kpp0lohi);
      ymm kpp0hi = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(kpp0, 1));
      ymm kpp0hilo = _mm256_castsi128_si256(_mm256_extracti128_si256(kpp0hi, 0));
      ymm kpp0hihi = _mm256_castsi128_si256(_mm256_extracti128_si256(kpp0hi, 1));
      sum0 = _mm256_add_epi32(sum0, kpp0hilo);
      sum0 = _mm256_add_epi32(sum0, kpp0hihi);

      ymm kpp1lo = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(kpp1, 0));
      ymm kpp1lolo = _mm256_castsi128_si256(_mm256_extracti128_si256(kpp1lo, 0));
      ymm kpp1lohi = _mm256_castsi128_si256(_mm256_extracti128_si256(kpp1lo, 1));
      sum1 = _mm256_add_epi32(sum1, kpp1lolo);
      sum1 = _mm256_add_epi32(sum1, kpp1lohi);
      ymm kpp1hi = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(kpp1, 1));
      ymm kpp1hilo = _mm256_castsi128_si256(_mm256_extracti128_si256(kpp1hi, 0));
      ymm kpp1hihi = _mm256_castsi128_si256(_mm256_extracti128_si256(kpp1hi, 1));
      sum1 = _mm256_add_epi32(sum1, kpp1hilo);
      sum1 = _mm256_add_epi32(sum1, kpp1hihi);
    }
    //sum0 = _mm256_add_epi32(sum0, _mm256_srli_si256(sum0, 16));
    sum0 = _mm256_add_epi32(sum0, _mm256_srli_si256(sum0, 8));
    //sum1 = _mm256_add_epi32(sum1, _mm256_srli_si256(sum1, 16));
    sum1 = _mm256_add_epi32(sum1, _mm256_srli_si256(sum1, 8));
    _mm_storel_epi64((xmm*)&sum.p[0], _mm256_castsi256_si128(sum0));
    _mm_storel_epi64((xmm*)&sum.p[1], _mm256_castsi256_si128(sum1));

#elif defined USE_SSE_EVAL
    sum.m[0] = _mm_set_epi32(0, 0, *reinterpret_cast<const s32*>(&pkppw[list1[0]][0]), *reinterpret_cast<const s32*>(&pkppb[list0[0]][0]));
    sum.m[0] = _mm_cvtepi16_epi32(sum.m[0]);
    for (int i = 1; i < pos.nlist(); ++i) {
      __m128i tmp;
      tmp = _mm_set_epi32(0, 0, *reinterpret_cast<const s32*>(&pkppw[list1[i]][0]), *reinterpret_cast<const s32*>(&pkppb[list0[i]][0]));
      tmp = _mm_cvtepi16_epi32(tmp);
      sum.m[0] = _mm_add_epi32(sum.m[0], tmp);
    }
#else
    sum.p[0][0] = pkppb[list0[0]][0];
    sum.p[0][1] = pkppb[list0[0]][1];
    sum.p[1][0] = pkppw[list1[0]][0];
    sum.p[1][1] = pkppw[list1[0]][1];
    for (int i = 1; i < pos.nlist(); ++i) {
      sum.p[0] += pkppb[list0[i]];
      sum.p[1] += pkppw[list1[i]];
    }
#endif

    return sum;
  }
  std::array<s32, 2> doablack(const Position& pos, int index0, int index1) {
    const Square sq_bk = pos.kingSquare(Black);
    const int* list0 = pos.cplist0();

    const auto* pkppb = Evaluater::KPP[sq_bk][index0];
    std::array<s32, 2> sum = { {pkppb[list0[0]][0], pkppb[list0[0]][1]} };
    for (int i = 1; i < pos.nlist(); ++i) {
      sum[0] += pkppb[list0[i]][0];
      sum[1] += pkppb[list0[i]][1];
    }
    return sum;
  }
  std::array<s32, 2> doawhite(const Position& pos, int index0, int index1) {
    const Square sq_wk = pos.kingSquare(White);
    const int* list1 = pos.cplist1();

    const auto* pkppw = Evaluater::KPP[inverse(sq_wk)][index1];
    std::array<s32, 2> sum = { {pkppw[list1[0]][0], pkppw[list1[0]][1]} };
    for (int i = 1; i < pos.nlist(); ++i) {
      sum[0] += pkppw[list1[i]][0];
      sum[1] += pkppw[list1[i]][1];
    }
    return sum;
  }

#if defined INANIWA_SHIFT
  Score inaniwaScoreBody(const Position& pos) {
    Score score = ScoreZero;
    if (pos.csearcher()->inaniwaFlag == InaniwaIsBlack) {
      if (pos.piece(B9) == WKnight) { score += 700 * FVScale; }
      if (pos.piece(H9) == WKnight) { score += 700 * FVScale; }
      if (pos.piece(A7) == WKnight) { score += 700 * FVScale; }
      if (pos.piece(I7) == WKnight) { score += 700 * FVScale; }
      if (pos.piece(C7) == WKnight) { score += 400 * FVScale; }
      if (pos.piece(G7) == WKnight) { score += 400 * FVScale; }
      if (pos.piece(B5) == WKnight) { score += 700 * FVScale; }
      if (pos.piece(H5) == WKnight) { score += 700 * FVScale; }
      if (pos.piece(D5) == WKnight) { score += 100 * FVScale; }
      if (pos.piece(F5) == WKnight) { score += 100 * FVScale; }
      if (pos.piece(E3) == BPawn) { score += 200 * FVScale; }
      if (pos.piece(E4) == BPawn) { score += 200 * FVScale; }
      if (pos.piece(E5) == BPawn) { score += 200 * FVScale; }
    }
    else {
      assert(pos.csearcher()->inaniwaFlag == InaniwaIsWhite);
      if (pos.piece(B1) == BKnight) { score -= 700 * FVScale; }
      if (pos.piece(H1) == BKnight) { score -= 700 * FVScale; }
      if (pos.piece(A3) == BKnight) { score -= 700 * FVScale; }
      if (pos.piece(I3) == BKnight) { score -= 700 * FVScale; }
      if (pos.piece(C3) == BKnight) { score -= 400 * FVScale; }
      if (pos.piece(G3) == BKnight) { score -= 400 * FVScale; }
      if (pos.piece(B5) == BKnight) { score -= 700 * FVScale; }
      if (pos.piece(H5) == BKnight) { score -= 700 * FVScale; }
      if (pos.piece(D5) == BKnight) { score -= 100 * FVScale; }
      if (pos.piece(F5) == BKnight) { score -= 100 * FVScale; }
      if (pos.piece(E7) == WPawn) { score -= 200 * FVScale; }
      if (pos.piece(E6) == WPawn) { score -= 200 * FVScale; }
      if (pos.piece(E5) == WPawn) { score -= 200 * FVScale; }
    }
    return score;
  }
  inline Score inaniwaScore(const Position& pos) {
    if (pos.csearcher()->inaniwaFlag == NotInaniwa) return ScoreZero;
    return inaniwaScoreBody(pos);
  }
#endif

  bool calcDifference(Position& pos, SearchStack* ss) {
#if defined INANIWA_SHIFT
    if (pos.csearcher()->inaniwaFlag != NotInaniwa) return false;
#endif
    if ((ss - 1)->staticEvalRaw.p[0][0] == ScoreNotEvaluated)
      return false;

    const Move lastMove = (ss - 1)->currentMove;
    assert(lastMove != Move::moveNull());

    if (lastMove.pieceTypeFrom() == King) {
      EvalSum diff = (ss - 1)->staticEvalRaw; // 本当は diff ではないので名前が良くない。
      const Square sq_bk = pos.kingSquare(Black);
      const Square sq_wk = pos.kingSquare(White);
      diff.p[2] = Evaluater::KK[sq_bk][sq_wk];
      diff.p[2][0] += pos.material() * FVScale;
      if (pos.turn() == Black) {
        const auto* ppkppw = Evaluater::KPP[inverse(sq_wk)];
        const int* list1 = pos.plist1();

#if defined USE_AVX2_EVAL
        ymm zero = _mm256_setzero_si256();
        ymm sum1 = zero;
        for (int i = 0; i < pos.nlist(); ++i) {
          const int k1 = list1[i];
          const auto* pkppw = ppkppw[k1];
          for (int j = 0; j < i; j += 8) {
            ymm index1 = _mm256_load_si256((const ymm*)&list1[j]);
            ymm mask = MASK[std::min(i - j, 8)];
            ymm kpp1 = _mm256_mask_i32gather_epi32(zero, (const int*)pkppw, index1, mask, 4);
            // TODO(nodchip): _mm256_add_epi32()で上位128ビットがクリアされる原因を調べる
            ymm kpp1lo = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(kpp1, 0));
            ymm kpp1lolo = _mm256_castsi128_si256(_mm256_extracti128_si256(kpp1lo, 0));
            ymm kpp1lohi = _mm256_castsi128_si256(_mm256_extracti128_si256(kpp1lo, 1));
            sum1 = _mm256_add_epi32(sum1, kpp1lolo);
            sum1 = _mm256_add_epi32(sum1, kpp1lohi);
            ymm kpp1hi = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(kpp1, 1));
            ymm kpp1hilo = _mm256_castsi128_si256(_mm256_extracti128_si256(kpp1hi, 0));
            ymm kpp1hihi = _mm256_castsi128_si256(_mm256_extracti128_si256(kpp1hi, 1));
            sum1 = _mm256_add_epi32(sum1, kpp1hilo);
            sum1 = _mm256_add_epi32(sum1, kpp1hihi);
          }
          diff.p[2][0] -= Evaluater::KKP[inverse(sq_wk)][inverse(sq_bk)][k1][0];
          diff.p[2][1] += Evaluater::KKP[inverse(sq_wk)][inverse(sq_bk)][k1][1];
        }
        //sum1 = _mm256_add_epi32(sum1, _mm256_srli_si256(sum1, 16));
        sum1 = _mm256_add_epi32(sum1, _mm256_srli_si256(sum1, 8));
        _mm_storel_epi64((xmm*)&diff.p[1], _mm256_castsi256_si128(sum1));
#else
        diff.p[1][0] = 0;
        diff.p[1][1] = 0;
        for (int i = 0; i < pos.nlist(); ++i) {
          const int k1 = list1[i];
          const auto* pkppw = ppkppw[k1];
          for (int j = 0; j < i; ++j) {
            const int l1 = list1[j];
            diff.p[1] += pkppw[l1];
          }
          diff.p[2][0] -= Evaluater::KKP[inverse(sq_wk)][inverse(sq_bk)][k1][0];
          diff.p[2][1] += Evaluater::KKP[inverse(sq_wk)][inverse(sq_bk)][k1][1];
        }
#endif

        if (pos.cl().size == 2) {
          const int listIndex_cap = pos.cl().listindex1;
          diff.p[0] += doablack(pos, pos.cl().clistpair1.newlist0, pos.cl().clistpair1.newlist1);
          pos.plist0()[listIndex_cap] = pos.cl().clistpair1.oldlist0;
          diff.p[0] -= doablack(pos, pos.cl().clistpair1.oldlist0, pos.cl().clistpair1.oldlist1);
          pos.plist0()[listIndex_cap] = pos.cl().clistpair1.newlist0;
        }
      }
      else {
        const auto* ppkppb = Evaluater::KPP[sq_bk];
        const int* list0 = pos.plist0();

#if defined USE_AVX2_EVAL
        ymm zero = _mm256_setzero_si256();
        ymm sum0 = zero;
        for (int i = 0; i < pos.nlist(); ++i) {
          const int k0 = list0[i];
          const auto* pkppb = ppkppb[k0];
          for (int j = 0; j < i; j += 8) {
            ymm index1 = _mm256_load_si256((const ymm*)&list0[j]);
            ymm mask = MASK[std::min(i - j, 8)];
            ymm kpp0 = _mm256_mask_i32gather_epi32(zero, (const int*)pkppb, index1, mask, 4);
            // デバッガをアタッチした場合にymmレジスタの上位128bitがクリアされるのを回避する
            // TODO(nodchip): 上位128ビットも使用する
            ymm kpp0lo = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(kpp0, 0));
            ymm kpp0lolo = _mm256_castsi128_si256(_mm256_extracti128_si256(kpp0lo, 0));
            ymm kpp0lohi = _mm256_castsi128_si256(_mm256_extracti128_si256(kpp0lo, 1));
            sum0 = _mm256_add_epi32(sum0, kpp0lolo);
            sum0 = _mm256_add_epi32(sum0, kpp0lohi);
            ymm kpp0hi = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(kpp0, 1));
            ymm kpp0hilo = _mm256_castsi128_si256(_mm256_extracti128_si256(kpp0hi, 0));
            ymm kpp0hihi = _mm256_castsi128_si256(_mm256_extracti128_si256(kpp0hi, 1));
            sum0 = _mm256_add_epi32(sum0, kpp0hilo);
            sum0 = _mm256_add_epi32(sum0, kpp0hihi);
          }
          diff.p[2] += Evaluater::KKP[sq_bk][sq_wk][k0];
        }
        //sum0 = _mm256_add_epi32(sum0, _mm256_srli_si256(sum0, 16));
        sum0 = _mm256_add_epi32(sum0, _mm256_srli_si256(sum0, 8));
        _mm_storel_epi64((xmm*)&diff.p[0], _mm256_castsi256_si128(sum0));
#else
        diff.p[0][0] = 0;
        diff.p[0][1] = 0;
        for (int i = 0; i < pos.nlist(); ++i) {
          const int k0 = list0[i];
          const auto* pkppb = ppkppb[k0];
          for (int j = 0; j < i; ++j) {
            const int l0 = list0[j];
            diff.p[0] += pkppb[l0];
          }
          diff.p[2] += Evaluater::KKP[sq_bk][sq_wk][k0];
        }
#endif

        if (pos.cl().size == 2) {
          const int listIndex_cap = pos.cl().listindex1;
          diff.p[1] += doawhite(pos, pos.cl().clistpair1.newlist0, pos.cl().clistpair1.newlist1);
          pos.plist1()[listIndex_cap] = pos.cl().clistpair1.oldlist1;
          diff.p[1] -= doawhite(pos, pos.cl().clistpair1.oldlist0, pos.cl().clistpair1.oldlist1);
          pos.plist1()[listIndex_cap] = pos.cl().clistpair1.newlist1;
        }
      }
      ss->staticEvalRaw = diff;
}
    else {
      const int listIndex = pos.cl().listindex0;
      auto diff = doapc(pos, pos.cl().clistpair0.newlist0, pos.cl().clistpair0.newlist1);
      if (pos.cl().size == 1) {
        pos.plist0()[listIndex] = pos.cl().clistpair0.oldlist0;
        pos.plist1()[listIndex] = pos.cl().clistpair0.oldlist1;
        diff -= doapc(pos, pos.cl().clistpair0.oldlist0, pos.cl().clistpair0.oldlist1);
      }
      else {
        assert(pos.cl().size == 2);
        diff += doapc(pos, pos.cl().clistpair1.newlist0, pos.cl().clistpair1.newlist1);
        diff.p[0] -= Evaluater::KPP[pos.kingSquare(Black)][pos.cl().clistpair0.newlist0][pos.cl().clistpair1.newlist0];
        diff.p[1] -= Evaluater::KPP[inverse(pos.kingSquare(White))][pos.cl().clistpair0.newlist1][pos.cl().clistpair1.newlist1];
        const int listIndex_cap = pos.cl().listindex1;
        pos.plist0()[listIndex_cap] = pos.cl().clistpair1.oldlist0;
        pos.plist1()[listIndex_cap] = pos.cl().clistpair1.oldlist1;

        pos.plist0()[listIndex] = pos.cl().clistpair0.oldlist0;
        pos.plist1()[listIndex] = pos.cl().clistpair0.oldlist1;
        diff -= doapc(pos, pos.cl().clistpair0.oldlist0, pos.cl().clistpair0.oldlist1);

        diff -= doapc(pos, pos.cl().clistpair1.oldlist0, pos.cl().clistpair1.oldlist1);
        diff.p[0] += Evaluater::KPP[pos.kingSquare(Black)][pos.cl().clistpair0.oldlist0][pos.cl().clistpair1.oldlist0];
        diff.p[1] += Evaluater::KPP[inverse(pos.kingSquare(White))][pos.cl().clistpair0.oldlist1][pos.cl().clistpair1.oldlist1];
        pos.plist0()[listIndex_cap] = pos.cl().clistpair1.newlist0;
        pos.plist1()[listIndex_cap] = pos.cl().clistpair1.newlist1;
      }
      pos.plist0()[listIndex] = pos.cl().clistpair0.newlist0;
      pos.plist1()[listIndex] = pos.cl().clistpair0.newlist1;

      diff.p[2][0] += pos.materialDiff() * FVScale;

      ss->staticEvalRaw = diff + (ss - 1)->staticEvalRaw;
    }

    return true;
  }

  int make_list_unUseDiff(const Position& pos, int list0[EvalList::ListSize], int list1[EvalList::ListSize], int nlist) {
    auto func = [&](const Bitboard& posBB, const int f_pt, const int e_pt) {
      Square sq;
      Bitboard bb;
      bb = (posBB)& pos.bbOf(Black);
      FOREACH_BB(bb, sq, {
          list0[nlist] = (f_pt)+sq;
          list1[nlist] = (e_pt)+inverse(sq);
          nlist += 1;
      });
      bb = (posBB)& pos.bbOf(White);
      FOREACH_BB(bb, sq, {
          list0[nlist] = (e_pt)+sq;
          list1[nlist] = (f_pt)+inverse(sq);
          nlist += 1;
      });
    };
    func(pos.bbOf(Pawn), f_pawn, e_pawn);
    func(pos.bbOf(Lance), f_lance, e_lance);
    func(pos.bbOf(Knight), f_knight, e_knight);
    func(pos.bbOf(Silver), f_silver, e_silver);
    const Bitboard goldsBB = pos.goldsBB();
    func(goldsBB, f_gold, e_gold);
    func(pos.bbOf(Bishop), f_bishop, e_bishop);
    func(pos.bbOf(Horse), f_horse, e_horse);
    func(pos.bbOf(Rook), f_rook, e_rook);
    func(pos.bbOf(Dragon), f_dragon, e_dragon);

    return nlist;
  }

  void evaluateBody(Position& pos, SearchStack* ss) {
    if (calcDifference(pos, ss)) {
#ifndef NDEBUG
      const auto score = ss->staticEvalRaw.sum(pos.turn());
      if (evaluateUnUseDiff(pos) != score) {
        debugOutputEvalSum(pos, ss->staticEvalRaw);
        assert(false);
      }
#endif
      return;
    }

    const Square sq_bk = pos.kingSquare(Black);
    const Square sq_wk = pos.kingSquare(White);
    const int* list0 = pos.plist0();
    const int* list1 = pos.plist1();

    const auto* ppkppb = Evaluater::KPP[sq_bk];
    const auto* ppkppw = Evaluater::KPP[inverse(sq_wk)];

    EvalSum sum;
    sum.p[2] = Evaluater::KK[sq_bk][sq_wk];
#if defined USE_AVX2_EVAL || defined USE_SSE_EVAL
    sum.m[0] = _mm_setzero_si128();
    for (int i = 0; i < pos.nlist(); ++i) {
      const int k0 = list0[i];
      const int k1 = list1[i];
      const auto* pkppb = ppkppb[k0];
      const auto* pkppw = ppkppw[k1];
      for (int j = 0; j < i; ++j) {
        const int l0 = list0[j];
        const int l1 = list1[j];
        __m128i tmp;
        tmp = _mm_set_epi32(0, 0, *reinterpret_cast<const s32*>(&pkppw[l1][0]), *reinterpret_cast<const s32*>(&pkppb[l0][0]));
        tmp = _mm_cvtepi16_epi32(tmp);
        sum.m[0] = _mm_add_epi32(sum.m[0], tmp);
      }
      sum.p[2] += Evaluater::KKP[sq_bk][sq_wk][k0];
    }
#else
    // loop 開始を i = 1 からにして、i = 0 の分のKKPを先に足す。
    sum.p[2] += Evaluater::KKP[sq_bk][sq_wk][list0[0]];
    sum.p[0][0] = 0;
    sum.p[0][1] = 0;
    sum.p[1][0] = 0;
    sum.p[1][1] = 0;
    for (int i = 1; i < pos.nlist(); ++i) {
      const int k0 = list0[i];
      const int k1 = list1[i];
      const auto* pkppb = ppkppb[k0];
      const auto* pkppw = ppkppw[k1];
      for (int j = 0; j < i; ++j) {
        const int l0 = list0[j];
        const int l1 = list1[j];
        sum.p[0] += pkppb[l0];
        sum.p[1] += pkppw[l1];
      }
      sum.p[2] += Evaluater::KKP[sq_bk][sq_wk][k0];
    }
#endif

    sum.p[2][0] += pos.material() * FVScale;
#if defined INANIWA_SHIFT
    sum.p[2][0] += inaniwaScore(pos);
#endif
    ss->staticEvalRaw = sum;

#ifndef NDEBUG
    if (evaluateUnUseDiff(pos) != sum.sum(pos.turn())) {
      debugOutputEvalSum(pos, sum);
      assert(false);
    }
#endif
  }
}

// todo: 無名名前空間に入れる。
Score evaluateUnUseDiff(const Position& pos) {
  int list0[EvalList::ListSize];
  int list1[EvalList::ListSize];

  const Hand handB = pos.hand(Black);
  const Hand handW = pos.hand(White);

  const Square sq_bk = pos.kingSquare(Black);
  const Square sq_wk = pos.kingSquare(White);
  int nlist = 0;

  auto func = [&](const Hand hand, const HandPiece hp, const int list0_index, const int list1_index) {
    for (u32 i = 1; i <= hand.numOf(hp); ++i) {
      list0[nlist] = list0_index + i;
      list1[nlist] = list1_index + i;
      ++nlist;
    }
  };
  func(handB, HPawn, f_hand_pawn, e_hand_pawn);
  func(handW, HPawn, e_hand_pawn, f_hand_pawn);
  func(handB, HLance, f_hand_lance, e_hand_lance);
  func(handW, HLance, e_hand_lance, f_hand_lance);
  func(handB, HKnight, f_hand_knight, e_hand_knight);
  func(handW, HKnight, e_hand_knight, f_hand_knight);
  func(handB, HSilver, f_hand_silver, e_hand_silver);
  func(handW, HSilver, e_hand_silver, f_hand_silver);
  func(handB, HGold, f_hand_gold, e_hand_gold);
  func(handW, HGold, e_hand_gold, f_hand_gold);
  func(handB, HBishop, f_hand_bishop, e_hand_bishop);
  func(handW, HBishop, e_hand_bishop, f_hand_bishop);
  func(handB, HRook, f_hand_rook, e_hand_rook);
  func(handW, HRook, e_hand_rook, f_hand_rook);

  nlist = make_list_unUseDiff(pos, list0, list1, nlist);

  const auto* ppkppb = Evaluater::KPP[sq_bk];
  const auto* ppkppw = Evaluater::KPP[inverse(sq_wk)];

  EvalSum score;
  score.p[2] = Evaluater::KK[sq_bk][sq_wk];

  score.p[0][0] = 0;
  score.p[0][1] = 0;
  score.p[1][0] = 0;
  score.p[1][1] = 0;
  for (int i = 0; i < nlist; ++i) {
    const int k0 = list0[i];
    const int k1 = list1[i];
    const auto* pkppb = ppkppb[k0];
    const auto* pkppw = ppkppw[k1];
    for (int j = 0; j < i; ++j) {
      const int l0 = list0[j];
      const int l1 = list1[j];
      score.p[0] += pkppb[l0];
      score.p[1] += pkppw[l1];
    }
    score.p[2] += Evaluater::KKP[sq_bk][sq_wk][k0];
  }

  score.p[2][0] += pos.material() * FVScale;

#if defined INANIWA_SHIFT
  score.p[2][0] += inaniwaScore(pos);
#endif

  return static_cast<Score>(score.sum(pos.turn()));
}

void debugOutputEvalSum(const Position& pos, const EvalSum& evalSum) {
  int list0[EvalList::ListSize];
  int list1[EvalList::ListSize];

  const Hand handB = pos.hand(Black);
  const Hand handW = pos.hand(White);

  const Square sq_bk = pos.kingSquare(Black);
  const Square sq_wk = pos.kingSquare(White);
  int nlist = 0;

  auto func = [&](const Hand hand, const HandPiece hp, const int list0_index, const int list1_index) {
    for (u32 i = 1; i <= hand.numOf(hp); ++i) {
      list0[nlist] = list0_index + i;
      list1[nlist] = list1_index + i;
      ++nlist;
    }
  };
  func(handB, HPawn, f_hand_pawn, e_hand_pawn);
  func(handW, HPawn, e_hand_pawn, f_hand_pawn);
  func(handB, HLance, f_hand_lance, e_hand_lance);
  func(handW, HLance, e_hand_lance, f_hand_lance);
  func(handB, HKnight, f_hand_knight, e_hand_knight);
  func(handW, HKnight, e_hand_knight, f_hand_knight);
  func(handB, HSilver, f_hand_silver, e_hand_silver);
  func(handW, HSilver, e_hand_silver, f_hand_silver);
  func(handB, HGold, f_hand_gold, e_hand_gold);
  func(handW, HGold, e_hand_gold, f_hand_gold);
  func(handB, HBishop, f_hand_bishop, e_hand_bishop);
  func(handW, HBishop, e_hand_bishop, f_hand_bishop);
  func(handB, HRook, f_hand_rook, e_hand_rook);
  func(handW, HRook, e_hand_rook, f_hand_rook);

  nlist = make_list_unUseDiff(pos, list0, list1, nlist);

  const auto* ppkppb = Evaluater::KPP[sq_bk];
  const auto* ppkppw = Evaluater::KPP[inverse(sq_wk)];

  EvalSum score;
  score.p[2] = Evaluater::KK[sq_bk][sq_wk];

  score.p[0][0] = 0;
  score.p[0][1] = 0;
  score.p[1][0] = 0;
  score.p[1][1] = 0;
  for (int i = 0; i < nlist; ++i) {
    const int k0 = list0[i];
    assert(0 <= k0);
    assert(k0 < fe_end);
    const int k1 = list1[i];
    assert(0 <= k1);
    assert(k1 < fe_end);
    const auto* pkppb = ppkppb[k0];
    const auto* pkppw = ppkppw[k1];
    for (int j = 0; j < i; ++j) {
      const int l0 = list0[j];
      const int l1 = list1[j];
      score.p[0] += pkppb[l0];
      score.p[1] += pkppw[l1];
    }
    score.p[2] += Evaluater::KKP[sq_bk][sq_wk][k0];
  }

  score.p[2][0] += pos.material() * FVScale;

#if defined INANIWA_SHIFT
  score.p[2][0] += inaniwaScore(pos);
#endif

  std::cerr << "unuseDiff=" << std::endl;
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 2; ++j) {
      std::cerr << "p[" << i << "][" << j << "]" << score.p[i][j] << std::endl;
    }
  }
  std::cerr << "sum(pos.turn())=" << score.sum(pos.turn()) << std::endl;
  std::cerr << std::endl;

  std::cerr << "diff=" << std::endl;
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 2; ++j) {
      std::cerr << "p[" << i << "][" << j << "]" << evalSum.p[i][j] << std::endl;
    }
  }
  std::cerr << "sum(pos.turn())=" << evalSum.sum(pos.turn()) << std::endl;
}

Score evaluate(Position& pos, SearchStack* ss) {
  if (ss->staticEvalRaw.p[0][0] != ScoreNotEvaluated) {
    const Score score = static_cast<Score>(ss->staticEvalRaw.sum(pos.turn()));
#ifndef NDEBUG
    if (evaluateUnUseDiff(pos) != score) {
      debugOutputEvalSum(pos, ss->staticEvalRaw);
      assert(false);
    }
#endif
    return score / FVScale;
  }

  const HashTableKey keyExcludeTurn = pos.getKeyExcludeTurn();
  EvaluateHashEntry entry = *g_evalTable[keyExcludeTurn]; // atomic にデータを取得する必要がある。
  entry.decode();
  if (entry.key == keyExcludeTurn) {
    ss->staticEvalRaw = entry;
#ifndef NDEBUG
    if (ss->staticEvalRaw.sum(pos.turn()) != evaluateUnUseDiff(pos)) {
      debugOutputEvalSum(pos, ss->staticEvalRaw);
      assert(false);
    }
#endif
    return static_cast<Score>(entry.sum(pos.turn())) / FVScale;
  }

  evaluateBody(pos, ss);

  ss->staticEvalRaw.key = keyExcludeTurn;
  ss->staticEvalRaw.encode();
  *g_evalTable[keyExcludeTurn] = ss->staticEvalRaw;
  return static_cast<Score>(ss->staticEvalRaw.sum(pos.turn())) / FVScale;
}

#ifdef OUTPUT_EVALUATE_HASH_TABLE_UTILIZATION
// ハッシュの使用率をパーミル(1/1000)単位で返す
int EvaluateHashTable::getUtilizationPerMill() const
{
  long long numberOfUsed = 0;
  for (const auto& entry : entries_) {
    if (entry.word != 0) {
      ++numberOfUsed;
    }
  }
  return numberOfUsed * 1000 / EvaluateTableSize;
}
#endif

std::ostream& operator<<(std::ostream& os, const Key& key)
{
  char buffer[64];
  sprintf(buffer, "%016llx%016llx", key.p[1], key.p[0]);
  return os << buffer;
}
