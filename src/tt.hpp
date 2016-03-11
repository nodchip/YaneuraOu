#ifndef APERY_TT_HPP
#define APERY_TT_HPP

#include "move.hpp"
#include "parameters.hpp"
#include "score.hpp"

enum Depth {
  OnePly = 2,
  Depth0 = 0,
  Depth1 = 1,
  DepthQChecks = -1 * OnePly,
  DepthQNoChecks = -2 * OnePly,
  DepthQRecaptures = -MOVE_PICKER_QSEARCH_RECAPTURES_DEPTH * OnePly,
  DepthNone = -127 * OnePly
};
OverloadEnumOperators(Depth);

/// TTEntry struct is the 16 bytes transposition table entry, defined as below:
///
/// key        64 bit
/// move       16 bit
/// score      16 bit
/// eval score 16 bit
/// generation  6 bit
/// bound type  2 bit
/// depth       8 bit

struct TTEntry {

  Move  move()  const { return (Move)move16; }
  Score score() const { return (Score)score16; }
  Score eval()  const { return (Score)eval16; }
  Depth depth() const { return (Depth)depth8; }
  Bound bound() const { return (Bound)(genBound8 & 0x3); }

  void save(const Key& k, Score v, Bound b, Depth d, Move m, Score ev, uint8_t g) {

    // Preserve any existing move for the same position
    if (!m.isNone() || k.p[1] != key64)
      move16 = m.value();

    // Don't overwrite more valuable entries
    if (k.p[1] != key64
      || d > depth8 - TTENTRY_DEPTH_MARGIN
      /* || g != (genBound8 & 0xFC) // Matching non-zero keys are already refreshed by probe() */
      || b == BoundExact)
    {
      key64 = k.p[1];
      score16 = (int16_t)v;
      eval16 = (int16_t)ev;
      genBound8 = (uint8_t)(g | b);
      depth8 = (int8_t)d;
    }
  }

private:
  friend class TranspositionTable;

  u64 key64;
  u16 move16;
  s16 score16;
  s16 eval16;
  u8 genBound8;
  s8 depth8;
};
static_assert(sizeof(TTEntry) == 16, "Size of TTEntry is not 16");


/// A TranspositionTable consists of a power of 2 number of clusters and each
/// cluster consists of ClusterSize number of TTEntry. Each non-empty entry
/// contains information of exactly one position. The size of a cluster should
/// divide the size of a cache line size, to ensure that clusters never cross
/// cache lines. This ensures best cache performance, as the cacheline is
/// prefetched, as soon as possible.

class TranspositionTable {

  static constexpr int CacheLineSize = 64;
  static constexpr int ClusterSize = 1 << TTCLUSTER_SIZE_SHIFT;

  struct Cluster {
    TTEntry entry[ClusterSize];
  };

  static_assert((CacheLineSize * 4) % sizeof(Cluster) == 0, "Cluster size incorrect");

public:
  ~TranspositionTable();
  void new_search() { generation8 += 4; } // Lower 2 bits are used by Bound
  uint8_t generation() const { return generation8; }
  TTEntry* probe(const Key& key, bool& found) const;
  int hashfull() const;
  void resize(size_t mbSize);
  void clear();

  // The lowest order bits of the key are used to get the index of the cluster
  TTEntry* first_entry(const Key& key) const {
    return &table[key.p[0] & (clusterCount - 1)].entry[0];
  }

private:
  size_t clusterCount;
  Cluster* table;
  uint8_t generation8; // Size must be not bigger than TTEntry::genBound8
};

#endif // #ifndef APERY_TT_HPP
