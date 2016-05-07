#ifndef APERY_TIMEMANAGER_HPP
#define APERY_TIMEMANAGER_HPP

#include "limits_type.hpp"

/// The TimeManagement class computes the optimal time to think depending on
/// the maximum available time, the game move number and other parameters.

class TimeManagement {
public:
    void init(LimitsType& limits, Color us, int ply);
    void pv_instability(double bestMoveChanges) { unstablePvFactor = 1 + bestMoveChanges; }
    int available() const { return int(optimumTime * unstablePvFactor * 1.016); }
    int maximum() const { return maximumTime; }
    int elapsed() const { return int(now() - startTime); }

    int64_t availableNodes; // When in 'nodes as time' mode

private:
    TimePoint startTime;
    int optimumTime;
    int maximumTime;
    double unstablePvFactor;
};

extern TimeManagement Time;

#endif // #ifndef APERY_TIMEMANAGER_HPP
