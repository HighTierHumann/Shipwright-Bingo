#pragma once

#include "bingoEnums.h"
#include <string>

namespace Bingo {

struct SquareDef {
    BingoSquares square;
    // Must match the "name" field the website uses for this square exactly.
    const char* name;
    // Returns true once the square should be marked. nullptr means the square is known but not auto-tracked yet.
    bool (*isComplete)();
};

const SquareDef* FindSquareByName(const std::string& name);

} // namespace Bingo
