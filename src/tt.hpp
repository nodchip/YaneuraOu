#ifndef APERY_TT_HPP
#define APERY_TT_HPP

#include "move.hpp"
#include "score.hpp"

enum Depth {
  OnePly = 2,
  Depth0 = 0,
  Depth1 = 1,
  DepthQChecks = -1 * OnePly,
  DepthQNoChecks = -2 * OnePly,
  DepthQRecaptures = -8 * OnePly,
  DepthNone = -127 * OnePly
};
OverloadEnumOperators(Depth);

/// TTEntry struct is the 10 bytes transposition table entry, defined as below:
///
/// key        16 bit
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

  void save(Key k, Score v, Bound b, Depth d, Move m, Score ev, uint8_t g) {

    // Preserve any existing move for the same position
    if (!m.isNone() || (k >> 48) != key16)
      move16 = m.value();

    // Don't overwrite more valuable entries
    if ((k >> 48) != key16
      || d > depth8 - 2
      /* || g != (genBound8 & 0xFC) // Matching non-zero keys are already refreshed by probe() */
      || b == BoundExact)
    {
      key16 = (uint16_t)(k >> 48);
      score16 = (int16_t)v;
      eval16 = (int16_t)ev;
      genBound8 = (uint8_t)(g | b);
      depth8 = (int8_t)d;
    }
  }

private:
  friend class TranspositionTable;

  uint16_t key16;
  uint16_t move16;
  int16_t  score16;
  int16_t  eval16;
  uint8_t  genBound8;
  int8_t   depth8;
};


/// A TranspositionTable consists of a power of 2 number of clusters and each
/// cluster consists of ClusterSize number of TTEntry. Each non-empty entry
/// contains information of exactly one position. The size of a cluster should
/// divide the size of a cache line size, to ensure that clusters never cross
/// cache lines. This ensures best cache performance, as the cacheline is
/// prefetched, as soon as possible.

class TranspositionTable {

  static constexpr int CacheLineSize = 64;
  static constexpr int ClusterSize = 3;

  struct Cluster {
    TTEntry entry[ClusterSize];
    char padding[2]; // Align to a divisor of the cache line size
  };

  static_assert(CacheLineSize % sizeof(Cluster) == 0, "Cluster size incorrect");

public:
  ~TranspositionTable();
  void new_search() { generation8 += 4; } // Lower 2 bits are used by Bound
  uint8_t generation() const { return generation8; }
  TTEntry* probe(const Key key, bool& found) const;
  int hashfull() const;
  void resize(size_t mbSize);
  void clear();

  // The lowest order bits of the key are used to get the index of the cluster
  TTEntry* first_entry(const Key key) const {
    return &table[(size_t)key & (clusterCount - 1)].entry[0];
  }

private:
  size_t clusterCount;
  Cluster* table;
  uint8_t generation8; // Size must be not bigger than TTEntry::genBound8
};

#endif // #ifndef APERY_TT_HPP
