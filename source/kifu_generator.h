#ifndef _KIFU_GENERATOR_H_
#define _KIFU_GENERATOR_H_

#include "shogi.h"

namespace Learner
{
#ifdef USE_KIFU_GENERATOR
	void InitializeGenerator(USI::OptionsMap& o);
    void GenerateKifu();
    void MeasureMoveTimes();
    void ConvertSfenToLearningData();
#endif
}

#endif
