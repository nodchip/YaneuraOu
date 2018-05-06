#ifndef _EXPERIMENTAL_BOOK_H_
#define _EXPERIMENTAL_BOOK_H_

#include "shogi.h"

#ifdef EVAL_LEARN

namespace Book {
  bool Initialize(USI::OptionsMap& o);
  bool CreateRawBook();
  bool CreateScoredBook();
  bool AddScoreToBook();
  bool MergeBook();
  bool ExtendBook();
}

#endif

#endif
