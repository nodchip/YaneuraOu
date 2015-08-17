#ifndef COLOR_HPP
#define COLOR_HPP

#include "overloadEnumOperators.hpp"

enum Color {
	Black, White, ColorNum
};

OverloadEnumOperators(Color);

#if defined(_MSC_VER)
#define oppositeColor(c) ((c) == Black) ? White : Black
#else
inline constexpr Color oppositeColor(const Color c) {
	return static_cast<Color>(static_cast<int>(c) ^ 1);
}
#endif

#endif // #ifndef COLOR_HPP
