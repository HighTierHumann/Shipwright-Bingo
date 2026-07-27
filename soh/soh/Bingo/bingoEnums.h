// Default expansion: real enums

#if !defined(BINGO_ENUM_BEGIN) && !defined(BINGO_ENUM_ITEM) && !defined(BINGO_ENUM_END)
// clang-format off
#define BINGO_ENUM_BEGIN(EnumName) typedef enum EnumName {
#define BINGO_ENUM_ITEM(name, ...) name __VA_OPT__(=) __VA_ARGS__,
#define BINGO_ENUM_END(EnumName) } EnumName;
#define BINGO_ENUM__CLEANUP
// clang-format on
#endif

#include "BingoSquares.h"


// Clean up only the defaults we defined.
#ifdef BINGO_ENUM__CLEANUP
#undef BINGO_ENUM_BEGIN
#undef BINGO_ENUM_ITEM
#undef BINGO_ENUM_END
#undef BINGO_ENUM__CLEANUP
#endif