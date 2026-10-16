#pragma once

#include <string>

namespace Bingo {

struct SquareDef {
    const char* name;
    int flag;

    bool IsComplete() const;
};

const SquareDef* FindSquareByName(const std::string& name);

} // namespace Bingo
