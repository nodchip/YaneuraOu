#ifndef APERY_TT_HPP
#define APERY_TT_HPP

#include "common.hpp"
#include "move.hpp"

enum Depth {
  OnePly = 2,
  Depth0 = 0,
  Depth1 = 1,
  DepthQChecks = -1 * OnePly,
  DepthQNoChecks = -2 * OnePly,
  DepthQRecaptures = -5 * OnePly,
  DepthNone = -127 * OnePly
};
OverloadEnumOperators(Depth);

class TTEntry {
public:
  u32   key() const { return key32_; }
  Depth depth() const { return static_cast<Depth>(depth16_); }
  Score score() const { return static_cast<Score>(score16_); }
  Move  move() const { return static_cast<Move>(move16_); }
  Bound type() const { return static_cast<Bound>(bound_); }
  u8    generation() const { return generation8_; }
  Score evalScore() const { return static_cast<Score>(evalScore_); }
  void setGeneration(const u8 g) { generation8_ = g; }

  void save(const Depth depth, const Score score, const Move move,
    const u32 posKeyHigh32, const Bound bound, const u8 generation,
    const Score evalScore)
  {
    key32_ = posKeyHigh32;
    move16_ = static_cast<u16>(move.value());
    bound_ = static_cast<u8>(bound);
    generation8_ = generation;
    score16_ = static_cast<s16>(score);
    depth16_ = static_cast<s16>(depth);
    evalScore_ = static_cast<s16>(evalScore);
  }

private:
  u32 key32_;
  u16 move16_;
  u8 bound_;
  u8 generation8_;
  s16 score16_;
  s16 depth16_;
  s16 evalScore_;
};

constexpr int ClusterSize = CacheLineSize / sizeof(TTEntry);
static_assert(0 < ClusterSize, "");

struct TTCluster {
  TTEntry data[ClusterSize];
};

class TranspositionTable {
public:
  TranspositionTable();
  ~TranspositionTable();
  void setSize(const size_t mbSize); // Mega Byte 指定
  void clear();
  void store(const Key posKey, const Score score, const Bound bound, Depth depth,
    Move move, const Score evalScore);
  TTEntry* probe(const Key posKey);
  void newSearch();
  TTEntry* firstEntry(const Key posKey) const;
  void refresh(const TTEntry* tte) const;

  size_t size() const { return size_; }
  TTCluster* entries() const { return entries_; }
  u8 generation() const { return generation_; }
#ifdef OUTPUT_TRANSPOSITION_TABLE_UTILIZATION
  // ハッシュの使用率をパーミル(1/1000)単位で返す
  int getUtilizationPerMill() const;
#endif
#ifdef OUTPUT_TRANSPOSITION_HIT_RATE
  // ヒット率を返す
  double getHitRate() const;
#endif
#ifdef OUTPUT_TRANSPOSITION_CACHE_EXPIRATION_RATE
  int getCacheExpirationRatePerMill() const;
  u64 getNumberOfSaves() const;
  u64 getNumberOfCacheExpirations() const;
#endif

private:
  TranspositionTable(const TranspositionTable&);
  TranspositionTable& operator = (const TranspositionTable&);

  size_t size_; // 置換表のバイト数。2のべき乗である必要がある。
  // 置換表へのポインタ
  // メモリの確保・開放はこちらを通して行う
  TTCluster* entriesRaw_;
  // 置換表を実際に参照する際に使用するポインタ
  // CacheLineSizeにアラインメントしてある
  TTCluster* entries_;
  // iterative deepening していくとき、過去の探索で調べたものかを判定する。
  u8 generation_;
#ifdef OUTPUT_TRANSPOSITION_HIT_RATE
  std::atomic<u64> numberOfHits;
  std::atomic<u64> numberOfMissHits;
#endif
#ifdef OUTPUT_TRANSPOSITION_CACHE_EXPIRATION_RATE
  std::atomic<u64> numberOfSaves;
  std::atomic<u64> numberOfCacheExpirations;
#endif
};

inline TranspositionTable::TranspositionTable()
  : size_(0), entries_(nullptr), entriesRaw_(nullptr), generation_(0)
#ifdef OUTPUT_TRANSPOSITION_HIT_RATE
  , numberOfHits(0), numberOfMissHits(0)
#endif
#ifdef OUTPUT_TRANSPOSITION_CACHE_EXPIRATION_RATE
  , numberOfCacheExpirations(0), numberOfSaves(0)
#endif
{
}

inline TranspositionTable::~TranspositionTable() {
  delete[] entriesRaw_;
  entriesRaw_ = nullptr;
  entries_ = nullptr;
}

inline TTEntry* TranspositionTable::firstEntry(const Key posKey) const {
  // (size() - 1) は置換表で使用するバイト数のマスク
  // posKey の下位 (size() - 1) ビットを hash key として使用。
  // ここで posKey の下位ビットの一致を確認。
  // posKey の上位32ビットとの一致は probe, store 内で確認するので、
  // ここでは下位32bit 以上が確認出来れば完璧。
  // 置換表のサイズを小さく指定しているときは下位32bit の一致は確認出来ないが、
  // 仕方ない。
  return entries_[posKey & (size() - 1)].data;
}

inline void TranspositionTable::refresh(const TTEntry* tte) const {
  const_cast<TTEntry*>(tte)->setGeneration(this->generation());
}

inline void TranspositionTable::newSearch() {
  ++generation_;
}

#endif // #ifndef APERY_TT_HPP
