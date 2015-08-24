#ifndef INIT_HPP
#define INIT_HPP

#include "ifdef.hpp"
#include "common.hpp"
#include "bitboard.hpp"

void initTable(bool initializeFv = true);
void writeTable();

#if defined FIND_MAGIC
u64 findMagic(const Square sqare, const bool isBishop);
#endif // #if defined FIND_MAGIC

#endif // #ifndef INIT_HPP
