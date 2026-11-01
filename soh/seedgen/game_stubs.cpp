// Game symbols that generation's source files link against.
#include <cstdio>
#include <cstdlib>

#include "z64.h"
#include "soh/OTRGlobals.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/item-tables/ItemTableManager.h"

// Zeroed, so the language is English. gPlayState stays null, the same as "not in a scene".
extern "C" {
SaveContext gSaveContext;
PlayState* gPlayState = nullptr;
void lusprintf(const char*, int, int, const char*, ...) {
}
void seedgen_trap(const char* name) {
    fprintf(stderr, "seedgen: game-only function was called: %s\n", name);
    abort();
}
}
// Read only by the Wonder Item gameplay hook.
Vec3f sTagPointsFree[9];
Vec3f sTagPointsOrdered[9];
GameInteractor* GameInteractor::Instance = nullptr;
ItemTableManager* ItemTableManager::Instance = nullptr;

// Gameplay and drawing functions that generation must never run. A wrong return value could silently
// change a seed, so each one stops the program and names itself instead of returning anything.
// x86-64 only. If a build reports a new undefined symbol, read what calls it before adding it here:
// generation may need the real function.
#define TRAP(symbol)                              \
    __asm__(".text\n"                             \
            ".global " #symbol "\n" #symbol ":\n" \
            "leaq 1f(%rip), %rdi\n"               \
            "call seedgen_trap@PLT\n"             \
            ".section .rodata\n"                  \
            "1: .asciz \"" #symbol "\"\n"         \
            ".text\n");

#include "game_traps.inc"
