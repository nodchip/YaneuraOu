#ifndef APERY_LIMITS_TYPE_HPP
#define APERY_LIMITS_TYPE_HPP

#include "color.hpp"
#include "common.hpp"
#include "move.hpp"

struct LimitsType {

    LimitsType() { // Init explicitly due to broken value-initialization of non POD in MSVC
        nodes = time[White] = time[Black] = inc[White] = inc[Black] = npmsec = movestogo =
            depth = movetime = mate = infinite = ponder = byoyomi = 0;
        startTime = static_cast<TimePoint>(0);
    }

    bool use_time_management() const {
        return !(mate | movetime | depth | nodes | infinite);
    }

    std::vector<Move> searchmoves;
    int time[ColorNum], inc[ColorNum], npmsec, movestogo, depth, movetime, mate, infinite, ponder, byoyomi;
    int64_t nodes;
    TimePoint startTime;
};

extern LimitsType Limits;

#endif
