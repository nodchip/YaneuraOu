#ifndef _EXPERIMENTAL_BOOK_H_
#define _EXPERIMENTAL_BOOK_H_

#include "shogi.h"

namespace Book {
  bool Initialize(USI::OptionsMap& o);
  bool CreateRawBook();
  bool CreateScoredBook();
}

#endif
