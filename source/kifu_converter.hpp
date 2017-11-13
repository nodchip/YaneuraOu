#ifndef KIFU_CONVERTER_HPP
#define KIFU_CONVERTER_HPP

#include <sstream>
#include <string>

struct Position;

namespace KifuConverter {
#ifdef USE_SFEN_PACKER
    bool ConvertKifuToText(Position& pos, std::istringstream& ssCmd);
    bool ConvertKifuToBinary(Position& pos, std::istringstream& ssCmd);
#endif
}

#endif
