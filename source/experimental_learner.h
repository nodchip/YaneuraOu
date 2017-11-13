#ifndef _LEARNER_H_
#define _LEARNER_H_

#include <sstream>

#include "position.h"
#include "shogi.h"

namespace Learner {
struct Record {
    PackedSfen packed;
    int16_t value;
    int16_t win_color;
};
static_assert(sizeof(Record) == 36, "Size of Record is not 36");

#ifdef USE_EXPERIMENTAL_LEARNER
void InitializeLearner(USI::OptionsMap& o);
void Learn(std::istringstream& iss);
void CalculateTestDataEntropy(std::istringstream& iss);
void BreedEval(std::istringstream& iss);
#endif
}

#endif
