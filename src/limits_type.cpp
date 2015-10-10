#include "limits_type.hpp"

LimitsType::LimitsType() {
  time[Black] = 0;
  time[White] = 0;
  increment[Black] = 0;
  increment[White] = 0;
  movesToGo = 0;
  depth = 0;
  nodes = 0;
  byoyomi = 0;
  ponderTime = 0;
  infinite = 0;
  ponder = 0;
}

void LimitsType::set(const LimitsType& rh) {
  time[Black].store(rh.time[Black]);
  time[White].store(rh.time[White]);
  increment[Black].store(rh.increment[Black]);
  increment[White].store(rh.increment[White]);
  movesToGo.store(rh.movesToGo);
  depth.store(rh.depth);
  nodes.store(rh.nodes);
  byoyomi.store(rh.byoyomi);
  ponderTime.store(rh.ponderTime);
  infinite.store(rh.infinite);
  ponder.store(rh.ponder);
}

std::string LimitsType::outputInfoString() const {
  char buffer[1024];
  sprintf(
    buffer,
    "info string btime=%d wtime=%d bincrement=%d wincrement=%d movesToGo=%d depth=%d nodes=%d byoyomi=%d ponderTime=%d infinite=%d ponder=%d",
    time[Black].load(),
    time[White].load(),
    increment[Black].load(),
    increment[White].load(),
    movesToGo.load(),
    depth.load(),
    nodes.load(),
    byoyomi.load(),
    ponderTime.load(),
    infinite.load(),
    ponder.load());
  return buffer;
}
