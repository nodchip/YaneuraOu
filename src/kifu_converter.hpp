#ifndef KIFU_CONVERTER_HPP
#define KIFU_CONVERTER_HPP

#include <sstream>
#include <string>

class Position;

namespace KifuConverter {
    bool ConvertKifuToText(Position& pos, std::istringstream& ssCmd);
    bool ConvertKifuToBinary(Position& pos, std::istringstream& ssCmd);
}

#endif
