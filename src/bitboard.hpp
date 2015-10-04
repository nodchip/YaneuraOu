#ifndef APERY_BITBOARD_HPP
#define APERY_BITBOARD_HPP

#include "common.hpp"
#include "square.hpp"
#include "color.hpp"

class Bitboard;
extern const Bitboard SetMaskBB[SquareNum];

class Bitboard {
public:
#if defined (HAVE_SSE2) || defined (HAVE_SSE4)
  Bitboard& operator = (const Bitboard& rhs) {
    _mm_store_si128(&this->m_, rhs.m_);
    return *this;
  }
  Bitboard(const Bitboard& bb) {
    _mm_store_si128(&this->m_, bb.m_);
  }
#endif
  Bitboard() {}
  Bitboard(const u64 v0, const u64 v1) {
    this->p_[0] = v0;
    this->p_[1] = v1;
  }
  u64 p(const int index) const { return p_[index]; }
  void set(const int index, const u64 val) { p_[index] = val; }
  u64 merge() const { return this->p(0) | this->p(1); }
  bool isNot0() const {
#ifdef HAVE_SSE4
    return !(_mm_testz_si128(this->m_, _mm_set1_epi8(static_cast<char>(0xffu))));
#else
    return (this->merge() ? true : false);
#endif
  }
  // これはコードが見難くなるけど仕方ない。
  bool andIsNot0(const Bitboard& bb) const {
#ifdef HAVE_SSE4
    return !(_mm_testz_si128(this->m_, bb.m_));
#else
    return (*this & bb).isNot0();
#endif
  }
  Bitboard operator ~ () const {
#if defined (HAVE_SSE2) || defined (HAVE_SSE4)
    Bitboard tmp;
    _mm_store_si128(&tmp.m_, _mm_andnot_si128(this->m_, _mm_set1_epi8(static_cast<char>(0xffu))));
    return tmp;
#else
    return Bitboard(~this->p(0), ~this->p(1));
#endif
  }
  Bitboard operator &= (const Bitboard& rhs) {
#if defined (HAVE_SSE2) || defined (HAVE_SSE4)
    _mm_store_si128(&this->m_, _mm_and_si128(this->m_, rhs.m_));
#else
    this->p_[0] &= rhs.p(0);
    this->p_[1] &= rhs.p(1);
#endif
    return *this;
  }
  Bitboard operator |= (const Bitboard& rhs) {
#if defined (HAVE_SSE2) || defined (HAVE_SSE4)
    _mm_store_si128(&this->m_, _mm_or_si128(this->m_, rhs.m_));
#else
    this->p_[0] |= rhs.p(0);
    this->p_[1] |= rhs.p(1);
#endif
    return *this;
  }
  Bitboard operator ^= (const Bitboard& rhs) {
#if defined (HAVE_SSE2) || defined (HAVE_SSE4)
    _mm_store_si128(&this->m_, _mm_xor_si128(this->m_, rhs.m_));
#else
    this->p_[0] ^= rhs.p(0);
    this->p_[1] ^= rhs.p(1);
#endif
    return *this;
  }
  Bitboard operator <<= (const int i) {
#if defined (HAVE_SSE2) || defined (HAVE_SSE4)
    _mm_store_si128(&this->m_, _mm_slli_epi64(this->m_, i));
#else
    this->p_[0] <<= i;
    this->p_[1] <<= i;
#endif
    return *this;
  }
  Bitboard operator >>= (const int i) {
#if defined (HAVE_SSE2) || defined (HAVE_SSE4)
    _mm_store_si128(&this->m_, _mm_srli_epi64(this->m_, i));
#else
    this->p_[0] >>= i;
    this->p_[1] >>= i;
#endif
    return *this;
  }
  Bitboard operator & (const Bitboard& rhs) const { return Bitboard(*this) &= rhs; }
  Bitboard operator | (const Bitboard& rhs) const { return Bitboard(*this) |= rhs; }
  Bitboard operator ^ (const Bitboard& rhs) const { return Bitboard(*this) ^= rhs; }
  Bitboard operator << (const int i) const { return Bitboard(*this) <<= i; }
  Bitboard operator >> (const int i) const { return Bitboard(*this) >>= i; }
  bool operator == (const Bitboard& rhs) const {
#ifdef HAVE_SSE4
    return (_mm_testc_si128(_mm_cmpeq_epi8(this->m_, rhs.m_), _mm_set1_epi8(static_cast<char>(0xffu))) ? true : false);
#else
    return (this->p(0) == rhs.p(0)) && (this->p(1) == rhs.p(1));
#endif
  }
  bool operator != (const Bitboard& rhs) const { return !(*this == rhs); }
  // これはコードが見難くなるけど仕方ない。
  Bitboard andEqualNot(const Bitboard& bb) {
#if defined (HAVE_SSE2) || defined (HAVE_SSE4)
    _mm_store_si128(&this->m_, _mm_andnot_si128(bb.m_, this->m_));
#else
    (*this) &= ~bb;
#endif
    return *this;
  }
  // これはコードが見難くなるけど仕方ない。
  Bitboard notThisAnd(const Bitboard& bb) const {
#if defined (HAVE_SSE2) || defined (HAVE_SSE4)
    Bitboard temp;
    _mm_store_si128(&temp.m_, _mm_andnot_si128(this->m_, bb.m_));
    return temp;
#else
    return ~(*this) & bb;
#endif
  }
  bool isSet(const Square sq) const {
    assert(isInSquare(sq));
    return andIsNot0(SetMaskBB[sq]);
  }
  void setBit(const Square sq) { *this |= SetMaskBB[sq]; }
  void clearBit(const Square sq) { andEqualNot(SetMaskBB[sq]); }
  void xorBit(const Square sq) { (*this) ^= SetMaskBB[sq]; }
  void xorBit(const Square sq1, const Square sq2) { (*this) ^= (SetMaskBB[sq1] | SetMaskBB[sq2]); }
  // Bitboard の right 側だけの要素を調べて、最初に 1 であるマスの index を返す。
  // そのマスを 0 にする。
  // Bitboard の right 側が 0 でないことを前提にしている。
  FORCE_INLINE Square firstOneRightFromI9() {
    const Square sq = static_cast<Square>(firstOneFromLSB(this->p(0)));
    // LSB 側の最初の 1 の bit を 0 にする
    this->p_[0] &= this->p(0) - 1;
    return sq;
  }
  // Bitboard の left 側だけの要素を調べて、最初に 1 であるマスの index を返す。
  // そのマスを 0 にする。
  // Bitboard の left 側が 0 でないことを前提にしている。
  FORCE_INLINE Square firstOneLeftFromB9() {
    const Square sq = static_cast<Square>(firstOneFromLSB(this->p(1)) + 63);
    // LSB 側の最初の 1 の bit を 0 にする
    this->p_[1] &= this->p(1) - 1;
    return sq;
  }
  // Bitboard を I9 から A1 まで調べて、最初に 1 であるマスの index を返す。
  // そのマスを 0 にする。
  // Bitboard が allZeroBB() でないことを前提にしている。
  // VC++ の _BitScanForward() は入力が 0 のときに 0 を返す仕様なので、
  // 最初に 0 でないか判定するのは少し損。
  FORCE_INLINE Square firstOneFromI9() {
    if (this->p(0)) {
      return firstOneRightFromI9();
    }
    return firstOneLeftFromB9();
  }
  // 返す位置を 0 にしないバージョン。
  FORCE_INLINE Square constFirstOneRightFromI9() const { return static_cast<Square>(firstOneFromLSB(this->p(0))); }
  FORCE_INLINE Square constFirstOneLeftFromB9() const { return static_cast<Square>(firstOneFromLSB(this->p(1)) + 63); }
  FORCE_INLINE Square constFirstOneFromI9() const {
    if (this->p(0)) {
      return constFirstOneRightFromI9();
    }
    return constFirstOneLeftFromB9();
  }
  // Bitboard の 1 の bit を数える。
  // Crossover は、merge() すると 1 である bit が重なる可能性があるなら true
  template <bool Crossover = true>
  int popCount() const { return (Crossover ? count1s(p(0)) + count1s(p(1)) : count1s(merge())); }
  // bit が 1 つだけ立っているかどうかを判定する。
  // Crossover は、merge() すると 1 である bit が重なる可能性があるなら true
  template <bool Crossover = true>
  bool isOneBit() const {
#if defined (HAVE_SSE42)
    return (this->popCount<Crossover>() == 1);
#else
    if (!this->isNot0()) {
      return false;
    }
    else if (this->p(0)) {
      return !((this->p(0) & (this->p(0) - 1)) | this->p(1));
    }
    return !(this->p(1) & (this->p(1) - 1));
#endif
  }

  // for debug
  void printBoard() const {
    std::cout << "   A  B  C  D  E  F  G  H  I\n";
    for (Rank r = Rank9; r < RankNum; ++r) {
      std::cout << (9 - r);
      for (File f = FileA; FileI <= f; --f) {
        std::cout << (this->isSet(makeSquare(f, r)) ? "  X" : "  .");
      }
      std::cout << "\n";
    }

    std::cout << std::endl;
  }

  void printTable(const int part) const {
    for (Rank r = Rank9; r < RankNum; ++r) {
      for (File f = FileC; FileI <= f; --f) {
        std::cout << (UINT64_C(1) & (this->p(part) >> makeSquare(f, r)));
      }
      std::cout << std::endl;
    }
    std::cout << std::endl;
  }

  // 指定した位置が Bitboard のどちらの u64 変数の要素か
  static int part(const Square sq) { return static_cast<int>(C1 < sq); }

private:
#if defined (HAVE_SSE2) || defined (HAVE_SSE4)
  union {
    u64 p_[2];
    __m128i m_;
  };
#else
  u64 p_[2];	// p_[0] : 先手から見て、1一から7九までを縦に並べたbit. 63bit使用. right と呼ぶ。
              // p_[1] : 先手から見て、8一から1九までを縦に並べたbit. 18bit使用. left  と呼ぶ。
#endif
};

inline Bitboard setMaskBB(const Square sq) { return SetMaskBB[sq]; }

// 実際に使用する部分の全て bit が立っている Bitboard
inline Bitboard allOneBB() { return Bitboard(UINT64_C(0x7fffffffffffffff), UINT64_C(0x000000000003ffff)); }
inline Bitboard allZeroBB() { return Bitboard(0, 0); }

// 各マスのrookが利きを調べる必要があるマスの数
constexpr int RookBlockBits[SquareNum] = {
  14, 13, 13, 13, 13, 13, 13, 13, 14,
  13, 12, 12, 12, 12, 12, 12, 12, 13,
  13, 12, 12, 12, 12, 12, 12, 12, 13,
  13, 12, 12, 12, 12, 12, 12, 12, 13,
  13, 12, 12, 12, 12, 12, 12, 12, 13,
  13, 12, 12, 12, 12, 12, 12, 12, 13,
  13, 12, 12, 12, 12, 12, 12, 12, 13,
  13, 12, 12, 12, 12, 12, 12, 12, 13,
  14, 13, 13, 13, 13, 13, 13, 13, 14
};

// 各マスのbishopが利きを調べる必要があるマスの数
constexpr int BishopBlockBits[SquareNum] = {
  7,  6,  6,  6,  6,  6,  6,  6,  7,
  6,  6,  6,  6,  6,  6,  6,  6,  6,
  6,  6,  8,  8,  8,  8,  8,  6,  6,
  6,  6,  8, 10, 10, 10,  8,  6,  6,
  6,  6,  8, 10, 12, 10,  8,  6,  6,
  6,  6,  8, 10, 10, 10,  8,  6,  6,
  6,  6,  8,  8,  8,  8,  8,  6,  6,
  6,  6,  6,  6,  6,  6,  6,  6,  6,
  7,  6,  6,  6,  6,  6,  6,  6,  7
};

// Magic Bitboard で利きを求める際のシフト量
// RookShiftBits[17], RookShiftBits[53] はマジックナンバーが見つからなかったため、
// シフト量を 1 つ減らす。(テーブルサイズを 2 倍にする。)
// この方法は issei_y さんに相談したところ、教えて頂いた方法。
// PEXT Bitboardを使用する際はシフト量を減らす必要が無い。
constexpr int RookShiftBits[SquareNum] = {
  50, 51, 51, 51, 51, 51, 51, 51, 50,
#if defined HAVE_BMI2
  51, 52, 52, 52, 52, 52, 52, 52, 51,
#else
  51, 52, 52, 52, 52, 52, 52, 52, 50, // [17]: 51 -> 50
#endif
  51, 52, 52, 52, 52, 52, 52, 52, 51,
  51, 52, 52, 52, 52, 52, 52, 52, 51,
  51, 52, 52, 52, 52, 52, 52, 52, 51,
#if defined HAVE_BMI2
  51, 52, 52, 52, 52, 52, 52, 52, 51,
#else
  51, 52, 52, 52, 52, 52, 52, 52, 50, // [53]: 51 -> 50
#endif
  51, 52, 52, 52, 52, 52, 52, 52, 51,
  51, 52, 52, 52, 52, 52, 52, 52, 51,
  50, 51, 51, 51, 51, 51, 51, 51, 50
};

// Magic Bitboard で利きを求める際のシフト量
constexpr int BishopShiftBits[SquareNum] = {
  57, 58, 58, 58, 58, 58, 58, 58, 57,
  58, 58, 58, 58, 58, 58, 58, 58, 58,
  58, 58, 56, 56, 56, 56, 56, 58, 58,
  58, 58, 56, 54, 54, 54, 56, 58, 58,
  58, 58, 56, 54, 52, 54, 56, 58, 58,
  58, 58, 56, 54, 54, 54, 56, 58, 58,
  58, 58, 56, 56, 56, 56, 56, 58, 58,
  58, 58, 58, 58, 58, 58, 58, 58, 58,
  57, 58, 58, 58, 58, 58, 58, 58, 57
};

#if defined HAVE_BMI2
#else
constexpr u64 RookMagic[SquareNum] = {
  UINT64_C(0x140000400809300),  UINT64_C(0x1320000902000240), UINT64_C(0x8001910c008180),
  UINT64_C(0x40020004401040),   UINT64_C(0x40010000d01120),   UINT64_C(0x80048020084050),
  UINT64_C(0x40004000080228),   UINT64_C(0x400440000a2a0a),   UINT64_C(0x40003101010102),
  UINT64_C(0x80c4200012108100), UINT64_C(0x4010c00204000c01), UINT64_C(0x220400103250002),
  UINT64_C(0x2600200004001),    UINT64_C(0x40200052400020),   UINT64_C(0xc00100020020008),
  UINT64_C(0x9080201000200004), UINT64_C(0x2200201000080004), UINT64_C(0x80804c0020200191),
  UINT64_C(0x45383000009100),   UINT64_C(0x30002800020040),   UINT64_C(0x40104000988084),
  UINT64_C(0x108001000800415),  UINT64_C(0x14005000400009),   UINT64_C(0xd21001001c00045),
  UINT64_C(0xc0003000200024),   UINT64_C(0x40003000280004),   UINT64_C(0x40021000091102),
  UINT64_C(0x2008a20408000d00), UINT64_C(0x2000100084010040), UINT64_C(0x144080008008001),
  UINT64_C(0x50102400100026a2), UINT64_C(0x1040020008001010), UINT64_C(0x1200200028005010),
  UINT64_C(0x4280030030020898), UINT64_C(0x480081410011004),  UINT64_C(0x34000040800110a),
  UINT64_C(0x101000010c0021),   UINT64_C(0x9210800080082),    UINT64_C(0x6100002000400a7),
  UINT64_C(0xa2240800900800c0), UINT64_C(0x9220082001000801), UINT64_C(0x1040008001140030),
  UINT64_C(0x40002220040008),   UINT64_C(0x28000124008010c),  UINT64_C(0x40008404940002),
  UINT64_C(0x40040800010200),   UINT64_C(0x90000809002100),   UINT64_C(0x2800080001000201),
  UINT64_C(0x1400020001000201), UINT64_C(0x180081014018004),  UINT64_C(0x1100008000400201),
  UINT64_C(0x80004000200201),   UINT64_C(0x420800010000201),  UINT64_C(0x2841c00080200209),
  UINT64_C(0x120002401040001),  UINT64_C(0x14510000101000b),  UINT64_C(0x40080000808001),
  UINT64_C(0x834000188048001),  UINT64_C(0x4001210000800205), UINT64_C(0x4889a8007400201),
  UINT64_C(0x2080044080200062), UINT64_C(0x80004002861002),   UINT64_C(0xc00842049024),
  UINT64_C(0x8040000202020011), UINT64_C(0x400404002c0100),   UINT64_C(0x2080028202000102),
  UINT64_C(0x8100040800590224), UINT64_C(0x2040009004800010), UINT64_C(0x40045000400408),
  UINT64_C(0x2200240020802008), UINT64_C(0x4080042002200204), UINT64_C(0x4000b0000a00a2),
  UINT64_C(0xa600000810100),    UINT64_C(0x1410000d001180),   UINT64_C(0x2200101001080),
  UINT64_C(0x100020014104e120), UINT64_C(0x2407200100004810), UINT64_C(0x80144000a0845050),
  UINT64_C(0x1000200060030c18), UINT64_C(0x4004200020010102), UINT64_C(0x140600021010302)
};

constexpr u64 BishopMagic[SquareNum] = {
  UINT64_C(0x20101042c8200428), UINT64_C(0x840240380102),     UINT64_C(0x800800c018108251),
  UINT64_C(0x82428010301000),   UINT64_C(0x481008201000040),  UINT64_C(0x8081020420880800),
  UINT64_C(0x804222110000),     UINT64_C(0xe28301400850),     UINT64_C(0x2010221420800810),
  UINT64_C(0x2600010028801824), UINT64_C(0x8048102102002),    UINT64_C(0x4000248100240402),
  UINT64_C(0x49200200428a2108), UINT64_C(0x460904020844),     UINT64_C(0x2001401020830200),
  UINT64_C(0x1009008120),       UINT64_C(0x4804064008208004), UINT64_C(0x4406000240300ca0),
  UINT64_C(0x222001400803220),  UINT64_C(0x226068400182094),  UINT64_C(0x95208402010d0104),
  UINT64_C(0x4000807500108102), UINT64_C(0xc000200080500500), UINT64_C(0x5211000304038020),
  UINT64_C(0x1108100180400820), UINT64_C(0x10001280a8a21040), UINT64_C(0x100004809408a210),
  UINT64_C(0x202300002041112),  UINT64_C(0x4040a8000460408),  UINT64_C(0x204020021040201),
  UINT64_C(0x8120013180404),    UINT64_C(0xa28400800d020104), UINT64_C(0x200c201000604080),
  UINT64_C(0x1082004000109408), UINT64_C(0x100021c00c410408), UINT64_C(0x880820905004c801),
  UINT64_C(0x1054064080004120), UINT64_C(0x30c0a0224001030),  UINT64_C(0x300060100040821),
  UINT64_C(0x51200801020c006),  UINT64_C(0x2100040042802801), UINT64_C(0x481000820401002),
  UINT64_C(0x40408a0450000801), UINT64_C(0x810104200000a2),   UINT64_C(0x281102102108408),
  UINT64_C(0x804020040280021),  UINT64_C(0x2420401200220040), UINT64_C(0x80010144080c402),
  UINT64_C(0x80104400800002),   UINT64_C(0x1009048080400081), UINT64_C(0x100082000201008c),
  UINT64_C(0x10001008080009),   UINT64_C(0x2a5006b80080004),  UINT64_C(0xc6288018200c2884),
  UINT64_C(0x108100104200a000), UINT64_C(0x141002030814048),  UINT64_C(0x200204080010808),
  UINT64_C(0x200004013922002),  UINT64_C(0x2200000020050815), UINT64_C(0x2011010400040800),
  UINT64_C(0x1020040004220200), UINT64_C(0x944020104840081),  UINT64_C(0x6080a080801c044a),
  UINT64_C(0x2088400811008020), UINT64_C(0xc40aa04208070),    UINT64_C(0x4100800440900220),
  UINT64_C(0x48112050),         UINT64_C(0x818200d062012a10), UINT64_C(0x402008404508302),
  UINT64_C(0x100020101002),     UINT64_C(0x20040420504912),   UINT64_C(0x2004008118814),
  UINT64_C(0x1000810650084024), UINT64_C(0x1002a03002408804), UINT64_C(0x2104294801181420),
  UINT64_C(0x841080240500812),  UINT64_C(0x4406009000004884), UINT64_C(0x80082004012412),
  UINT64_C(0x80090880808183),   UINT64_C(0x300120020400410),  UINT64_C(0x21a090100822002)
};
#endif

// 指定した位置の属する file の bit を shift し、
// index を求める為に使用する。
constexpr int Slide[SquareNum] = {
  1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 ,
  10, 10, 10, 10, 10, 10, 10, 10, 10,
  19, 19, 19, 19, 19, 19, 19, 19, 19,
  28, 28, 28, 28, 28, 28, 28, 28, 28,
  37, 37, 37, 37, 37, 37, 37, 37, 37,
  46, 46, 46, 46, 46, 46, 46, 46, 46,
  55, 55, 55, 55, 55, 55, 55, 55, 55,
  1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 ,
  10, 10, 10, 10, 10, 10, 10, 10, 10
};

const Bitboard FileIMask = Bitboard(UINT64_C(0x1ff) << (9 * 0), 0);
const Bitboard FileHMask = Bitboard(UINT64_C(0x1ff) << (9 * 1), 0);
const Bitboard FileGMask = Bitboard(UINT64_C(0x1ff) << (9 * 2), 0);
const Bitboard FileFMask = Bitboard(UINT64_C(0x1ff) << (9 * 3), 0);
const Bitboard FileEMask = Bitboard(UINT64_C(0x1ff) << (9 * 4), 0);
const Bitboard FileDMask = Bitboard(UINT64_C(0x1ff) << (9 * 5), 0);
const Bitboard FileCMask = Bitboard(UINT64_C(0x1ff) << (9 * 6), 0);
const Bitboard FileBMask = Bitboard(0, 0x1ff << (9 * 0));
const Bitboard FileAMask = Bitboard(0, 0x1ff << (9 * 1));

const Bitboard Rank9Mask = Bitboard(UINT64_C(0x40201008040201) << 0, 0x201 << 0);
const Bitboard Rank8Mask = Bitboard(UINT64_C(0x40201008040201) << 1, 0x201 << 1);
const Bitboard Rank7Mask = Bitboard(UINT64_C(0x40201008040201) << 2, 0x201 << 2);
const Bitboard Rank6Mask = Bitboard(UINT64_C(0x40201008040201) << 3, 0x201 << 3);
const Bitboard Rank5Mask = Bitboard(UINT64_C(0x40201008040201) << 4, 0x201 << 4);
const Bitboard Rank4Mask = Bitboard(UINT64_C(0x40201008040201) << 5, 0x201 << 5);
const Bitboard Rank3Mask = Bitboard(UINT64_C(0x40201008040201) << 6, 0x201 << 6);
const Bitboard Rank2Mask = Bitboard(UINT64_C(0x40201008040201) << 7, 0x201 << 7);
const Bitboard Rank1Mask = Bitboard(UINT64_C(0x40201008040201) << 8, 0x201 << 8);

extern const Bitboard FileMask[FileNum];
extern const Bitboard RankMask[RankNum];
extern const Bitboard InFrontMask[ColorNum][RankNum];

inline Bitboard fileMask(const File f) { return FileMask[f]; }
template <File F> inline Bitboard fileMask() {
  static_assert(FileI <= F && F <= FileA, "");
  return (F == FileI ? FileIMask
    : F == FileH ? FileHMask
    : F == FileG ? FileGMask
    : F == FileF ? FileFMask
    : F == FileE ? FileEMask
    : F == FileD ? FileDMask
    : F == FileC ? FileCMask
    : F == FileB ? FileBMask
    : /*F == FileA ?*/ FileAMask);
}

inline Bitboard rankMask(const Rank r) { return RankMask[r]; }
template <Rank R> inline Bitboard rankMask() {
  static_assert(Rank9 <= R && R <= Rank1, "");
  return (R == Rank9 ? Rank9Mask
    : R == Rank8 ? Rank8Mask
    : R == Rank7 ? Rank7Mask
    : R == Rank6 ? Rank6Mask
    : R == Rank5 ? Rank5Mask
    : R == Rank4 ? Rank4Mask
    : R == Rank3 ? Rank3Mask
    : R == Rank2 ? Rank2Mask
    : /*R == Rank1 ?*/ Rank1Mask);
}

// 直接テーブル引きすべきだと思う。
inline Bitboard squareFileMask(const Square sq) {
  const File f = makeFile(sq);
  return fileMask(f);
}

// 直接テーブル引きすべきだと思う。
inline Bitboard squareRankMask(const Square sq) {
  const Rank r = makeRank(sq);
  return rankMask(r);
}

const Bitboard InFrontOfRank9Black = allZeroBB();
const Bitboard InFrontOfRank8Black = rankMask<Rank9>();
const Bitboard InFrontOfRank7Black = InFrontOfRank8Black | rankMask<Rank8>();
const Bitboard InFrontOfRank6Black = InFrontOfRank7Black | rankMask<Rank7>();
const Bitboard InFrontOfRank5Black = InFrontOfRank6Black | rankMask<Rank6>();
const Bitboard InFrontOfRank4Black = InFrontOfRank5Black | rankMask<Rank5>();
const Bitboard InFrontOfRank3Black = InFrontOfRank4Black | rankMask<Rank4>();
const Bitboard InFrontOfRank2Black = InFrontOfRank3Black | rankMask<Rank3>();
const Bitboard InFrontOfRank1Black = InFrontOfRank2Black | rankMask<Rank2>();

const Bitboard InFrontOfRank1White = allZeroBB();
const Bitboard InFrontOfRank2White = rankMask<Rank1>();
const Bitboard InFrontOfRank3White = InFrontOfRank2White | rankMask<Rank2>();
const Bitboard InFrontOfRank4White = InFrontOfRank3White | rankMask<Rank3>();
const Bitboard InFrontOfRank5White = InFrontOfRank4White | rankMask<Rank4>();
const Bitboard InFrontOfRank6White = InFrontOfRank5White | rankMask<Rank5>();
const Bitboard InFrontOfRank7White = InFrontOfRank6White | rankMask<Rank6>();
const Bitboard InFrontOfRank8White = InFrontOfRank7White | rankMask<Rank7>();
const Bitboard InFrontOfRank9White = InFrontOfRank8White | rankMask<Rank8>();

inline Bitboard inFrontMask(const Color c, const Rank r) { return InFrontMask[c][r]; }
template <Color C, Rank R> inline Bitboard inFrontMask() {
  static_assert(C == Black || C == White, "");
  static_assert(Rank9 <= R && R <= Rank1, "");
  return (C == Black ? (R == Rank9 ? InFrontOfRank9Black
    : R == Rank8 ? InFrontOfRank8Black
    : R == Rank7 ? InFrontOfRank7Black
    : R == Rank6 ? InFrontOfRank6Black
    : R == Rank5 ? InFrontOfRank5Black
    : R == Rank4 ? InFrontOfRank4Black
    : R == Rank3 ? InFrontOfRank3Black
    : R == Rank2 ? InFrontOfRank2Black
    : /*R == Rank1 ?*/ InFrontOfRank1Black)
    : (R == Rank9 ? InFrontOfRank9White
      : R == Rank8 ? InFrontOfRank8White
      : R == Rank7 ? InFrontOfRank7White
      : R == Rank6 ? InFrontOfRank6White
      : R == Rank5 ? InFrontOfRank5White
      : R == Rank4 ? InFrontOfRank4White
      : R == Rank3 ? InFrontOfRank3White
      : R == Rank2 ? InFrontOfRank2White
      : /*R == Rank1 ?*/ InFrontOfRank1White));
}

// メモリ節約の為、1次元配列にして無駄が無いようにしている。
#if defined HAVE_BMI2
extern Bitboard RookAttack[495616];
#else
extern Bitboard RookAttack[512000];
#endif
extern int RookAttackIndex[SquareNum];
// メモリ節約の為、1次元配列にして無駄が無いようにしている。
extern Bitboard BishopAttack[20224];
extern int BishopAttackIndex[SquareNum];
extern Bitboard RookBlockMask[SquareNum];
extern Bitboard BishopBlockMask[SquareNum];
// メモリ節約をせず、無駄なメモリを持っている。
extern Bitboard LanceAttack[ColorNum][SquareNum][128];

extern Bitboard KingAttack[SquareNum];
extern Bitboard GoldAttack[ColorNum][SquareNum];
extern Bitboard SilverAttack[ColorNum][SquareNum];
extern Bitboard KnightAttack[ColorNum][SquareNum];
extern Bitboard PawnAttack[ColorNum][SquareNum];

extern Bitboard BetweenBB[SquareNum][SquareNum];

extern Bitboard RookAttackToEdge[SquareNum];
extern Bitboard BishopAttackToEdge[SquareNum];
extern Bitboard LanceAttackToEdge[ColorNum][SquareNum];

extern Bitboard GoldCheckTable[ColorNum][SquareNum];
extern Bitboard SilverCheckTable[ColorNum][SquareNum];
extern Bitboard KnightCheckTable[ColorNum][SquareNum];
extern Bitboard LanceCheckTable[ColorNum][SquareNum];

#if defined HAVE_BMI2
// PEXT bitboard.
inline u64 occupiedToIndex(const Bitboard& block, const Bitboard& mask) {
  return _pext_u64(block.merge(), mask.merge());
}

inline Bitboard rookAttack(const Square sq, const Bitboard& occupied) {
  const Bitboard block(occupied & RookBlockMask[sq]);
  return RookAttack[RookAttackIndex[sq] + occupiedToIndex(block, RookBlockMask[sq])];
}
inline Bitboard bishopAttack(const Square sq, const Bitboard& occupied) {
  const Bitboard block(occupied & BishopBlockMask[sq]);
  return BishopAttack[BishopAttackIndex[sq] + occupiedToIndex(block, BishopBlockMask[sq])];
}
#else
// magic bitboard.
// magic number を使って block の模様から利きのテーブルへのインデックスを算出
inline u64 occupiedToIndex(const Bitboard& block, const u64 magic, const int shiftBits) {
  return (block.merge() * magic) >> shiftBits;
}

inline Bitboard rookAttack(const Square sq, const Bitboard& occupied) {
  const Bitboard block(occupied & RookBlockMask[sq]);
  return RookAttack[RookAttackIndex[sq] + occupiedToIndex(block, RookMagic[sq], RookShiftBits[sq])];
}
inline Bitboard bishopAttack(const Square sq, const Bitboard& occupied) {
  const Bitboard block(occupied & BishopBlockMask[sq]);
  return BishopAttack[BishopAttackIndex[sq] + occupiedToIndex(block, BishopMagic[sq], BishopShiftBits[sq])];
}
#endif
// todo: 香車の筋がどこにあるか先に分かっていれば、Bitboard の片方の変数だけを調べれば良くなる。
inline Bitboard lanceAttack(const Color c, const Square sq, const Bitboard& occupied) {
  const int part = Bitboard::part(sq);
  const int index = (occupied.p(part) >> Slide[sq]) & 127;
  return LanceAttack[c][sq][index];
}
// 飛車の縦だけの利き。香車の利きを使い、index を共通化することで高速化している。
inline Bitboard rookAttackFile(const Square sq, const Bitboard& occupied) {
  const int part = Bitboard::part(sq);
  const int index = (occupied.p(part) >> Slide[sq]) & 127;
  return LanceAttack[Black][sq][index] | LanceAttack[White][sq][index];
}
inline Bitboard goldAttack(const Color c, const Square sq) { return GoldAttack[c][sq]; }
inline Bitboard silverAttack(const Color c, const Square sq) { return SilverAttack[c][sq]; }
inline Bitboard knightAttack(const Color c, const Square sq) { return KnightAttack[c][sq]; }
inline Bitboard pawnAttack(const Color c, const Square sq) { return PawnAttack[c][sq]; }

// Bitboard で直接利きを返す関数。
// 1段目には歩は存在しないので、1bit シフトで別の筋に行くことはない。
// ただし、from に歩以外の駒の Bitboard を入れると、別の筋のビットが立ってしまうことがあるので、
// 別の筋のビットが立たないか、立っても問題ないかを確認して使用すること。
template <Color US> inline Bitboard pawnAttack(const Bitboard& from) { return (US == Black ? (from >> 1) : (from << 1)); }
inline Bitboard kingAttack(const Square sq) { return KingAttack[sq]; }
inline Bitboard dragonAttack(const Square sq, const Bitboard& occupied) { return rookAttack(sq, occupied) | kingAttack(sq); }
inline Bitboard horseAttack(const Square sq, const Bitboard& occupied) { return bishopAttack(sq, occupied) | kingAttack(sq); }
inline Bitboard queenAttack(const Square sq, const Bitboard& occupied) { return rookAttack(sq, occupied) | bishopAttack(sq, occupied); }

// sq1, sq2 の間(sq1, sq2 は含まない)のビットが立った Bitboard
inline Bitboard betweenBB(const Square sq1, const Square sq2) { return BetweenBB[sq1][sq2]; }
inline Bitboard rookAttackToEdge(const Square sq) { return RookAttackToEdge[sq]; }
inline Bitboard bishopAttackToEdge(const Square sq) { return BishopAttackToEdge[sq]; }
inline Bitboard lanceAttackToEdge(const Color c, const Square sq) { return LanceAttackToEdge[c][sq]; }
inline Bitboard dragonAttackToEdge(const Square sq) { return rookAttackToEdge(sq) | kingAttack(sq); }
inline Bitboard horseAttackToEdge(const Square sq) { return bishopAttackToEdge(sq) | kingAttack(sq); }
inline Bitboard goldCheckTable(const Color c, const Square sq) { return GoldCheckTable[c][sq]; }
inline Bitboard silverCheckTable(const Color c, const Square sq) { return SilverCheckTable[c][sq]; }
inline Bitboard knightCheckTable(const Color c, const Square sq) { return KnightCheckTable[c][sq]; }
inline Bitboard lanceCheckTable(const Color c, const Square sq) { return LanceCheckTable[c][sq]; }
// todo: テーブル引きを検討
inline Bitboard rookStepAttacks(const Square sq) { return goldAttack(Black, sq) & goldAttack(White, sq); }
// todo: テーブル引きを検討
inline Bitboard bishopStepAttacks(const Square sq) { return silverAttack(Black, sq) & silverAttack(White, sq); }
// 前方3方向の位置のBitboard
inline Bitboard goldAndSilverAttacks(const Color c, const Square sq) { return goldAttack(c, sq) & silverAttack(c, sq); }

// Bitboard の全ての bit に対して同様の処理を行う際に使用するマクロ
// xxx に処理を書く。
// xxx には template 引数を 2 つ以上持つクラスや関数は () でくくらないと使えない。
// これはマクロの制約。
// 同じ処理のコードが 2 箇所で生成されるため、コードサイズが膨れ上がる。
// その為、あまり多用すべきでないかも知れない。
#define FOREACH_BB(bb, sq, xxx)					\
	do {										\
		while (bb.p(0)) {						\
			sq = bb.firstOneRightFromI9();		\
			xxx;								\
		}										\
		while (bb.p(1)) {						\
			sq = bb.firstOneLeftFromB9();		\
			xxx;								\
		}										\
	} while (false)

template <typename T> FORCE_INLINE void foreachBB(Bitboard& bb, Square& sq, T t) {
  while (bb.p(0)) {
    sq = bb.firstOneRightFromI9();
    t(0);
  }
  while (bb.p(1)) {
    sq = bb.firstOneLeftFromB9();
    t(1);
  }
}

#endif // #ifndef APERY_BITBOARD_HPP
