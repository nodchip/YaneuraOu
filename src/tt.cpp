#include "tt.hpp"

void TranspositionTable::setSize(const size_t mbSize) { // Mega Byte 指定
                                                        // 確保する要素数を取得する。
  size_t newSize = (mbSize << 20) / sizeof(TTCluster);
  newSize = std::max(static_cast<size_t>(1024), newSize); // 最小値は 1024 としておく。
                                                          // 確保する要素数は 2 のべき乗である必要があるので、MSB以外を捨てる。
  const int msbIndex = 63 - firstOneFromMSB(static_cast<u64>(newSize));
  newSize = UINT64_C(1) << msbIndex;

  if (newSize == this->size()) {
    // 現在と同じサイズなら何も変更する必要がない。
    return;
  }

  size_ = newSize;
  ALIGNED_FREE(entries_);
  entries_ = (TTCluster*)ALIGNED_ALLOC(sizeof(TTCluster), newSize * sizeof(TTCluster));
  if (!entries_) {
    std::cerr << "Failed to allocate transposition table: " << mbSize << "MB";
    exit(EXIT_FAILURE);
  }
  clear();
}

void TranspositionTable::clear() {
  memset(entries_, 0, size() * sizeof(TTCluster));
}

void TranspositionTable::store(const Key& posKey, const Score score, const Bound bound, Depth depth,
  Move move, const Score evalScore)
{
#ifdef OUTPUT_TRANSPOSITION_EXPIRATION_RATE
  ++numberOfSaves;
#endif

  TTEntry* tte = firstEntry(posKey);
  TTEntry* replace = tte;

  if (depth < Depth0) {
    depth = Depth0;
  }

  for (int i = 0; i < ClusterSize; ++i, ++tte) {
    // 置換表が空か、keyが同じな古い情報が入っているとき
    if (!tte->key() || tte->key() == posKey.p[1]) {
      // move が無いなら、とりあえず古い情報でも良いので、他の指し手を保存する。
      if (move.isNone()) {
        move = tte->move();
      }

      tte->save(depth, score, move, posKey.p[1],
        bound, this->generation(), evalScore);
      return;
    }

    int c = (replace->generation() == this->generation() ? 2 : 0);
    c += (tte->generation() == this->generation() || tte->type() == BoundExact ? -2 : 0);
    c += (tte->depth() < replace->depth() ? 1 : 0);

    if (0 < c) {
      replace = tte;
    }
  }

#ifdef OUTPUT_TRANSPOSITION_EXPIRATION_RATE
  if (replace->key() != 0 && replace->key() != posKeyHigh32) {
    ++numberOfCacheExpirations;
  }
#endif

  replace->save(depth, score, move, posKey.p[1],
    bound, this->generation(), evalScore);
}

TTEntry* TranspositionTable::probe(const Key& posKey) {
  TTEntry* tte = firstEntry(posKey);

  // firstEntry() で、posKey の下位 (size() - 1) ビットを hash key に使用した。
  // ここでは posKey の上位 32bit が 保存されている hash key と同じか調べる。
  for (int i = 0; i < ClusterSize && tte[i].key(); ++i) {
    if (tte[i].key() == posKey.p[1]) {
#ifdef OUTPUT_TRANSPOSITION_HIT_RATE
      ++numberOfHits;
#endif
      return &tte[i];
    }
  }
#ifdef OUTPUT_TRANSPOSITION_HIT_RATE
  ++numberOfMissHits;
#endif
  return nullptr;
}

#if defined(OUTPUT_TRANSPOSITION_TABLE_UTILIZATION)
int TranspositionTable::getUtilizationPerMill() const
{
  long numberOfUsed = 0;
  int maxIndex = 64 * 1024;
  for (int i = 0; i < maxIndex; ++i) {
    for (int j = 0; j < ClusterSize; ++j) {
      if (entries_[i].data[j].key() != 0) {
        ++numberOfUsed;
      }
    }
  }

  return numberOfUsed * 1000 / (maxIndex * ClusterSize);
}
#elif defined(OUTPUT_TRANSPOSITION_HIT_RATE)
int TranspositionTable::getHitRatePerMill() const
{
  if (numberOfHits == 0) {
    return 0;
  }
  return numberOfHits * 1000 / (numberOfHits + numberOfMissHits);
}
#elif defined(OUTPUT_TRANSPOSITION_EXPIRATION_RATE)
int TranspositionTable::getCacheExpirationRatePerMill() const
{
  if (numberOfCacheExpirations == 0) {
    return 0;
  }
  return numberOfCacheExpirations * 1000 / numberOfSaves;
}

u64 TranspositionTable::getNumberOfSaves() const
{
  return numberOfSaves;
}

u64 TranspositionTable::getNumberOfCacheExpirations() const
{
  return numberOfCacheExpirations;
}
#endif
