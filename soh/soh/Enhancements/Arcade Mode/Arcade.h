#pragma once

#include <libultraship/libultra/types.h>

#ifdef __cplusplus
extern "C" {
#endif
struct GameState;
struct FileChooseContext;

void FileChoose_UpdateArcadeMenu(struct GameState* gameState);
void FileChoose_DrawArcadeMenuWindowContents(struct FileChooseContext* fileChooseContext);
const char* Arcade_GetSettingName(u8 optionIndex, u8 language);
const char* Arcade_GetSettingChoiceName(u8 optionIndex, u8 choiceIndex, u8 language);
void Arcade_DrawCurrentScore(struct PlayState* play);
u8 Arcade_GetSettingOptionsAmount(u8 optionIndex);
#ifdef __cplusplus
};
#endif

#define ARCADE_MAX_OPTIONS_ON_SCREEN 6

typedef enum {
    AM_OPTIONS_BOSSES,
    AM_OPTIONS_HEARTS,
    AM_OPTIONS_AMMO,
    AM_OPTIONS_HEAL,
    AM_OPTIONS_HYPERBOSSES,
    AM_OPTIONS_MAGIC,
    AM_OPTIONS_BGS,
    AM_OPTIONS_BOTTLE,
    AM_OPTIONS_LONGSHOT,
    AM_OPTIONS_HOVERBOOTS,
    AM_OPTIONS_BUNNYHOOD,
    AM_OPTIONS_TIMER,
    AM_OPTIONS_MAX,
} ArcadeOptionEnums;

typedef enum {
    AM_CHOICE_HYPERBOSSES_NO,
    AM_CHOICE_HYPERBOSSES_YES,
} ArcadeHyperBossesChoices;

typedef enum {
    REQ_NONE,

    // Inventory
    REQ_STICKS,    REQ_NUTS,    REQ_BOMBS,    REQ_BOW,      REQ_FIRE_ARROWS,  REQ_DINS,
    REQ_SLINGSHOT, REQ_OCARINA, REQ_BOMBCHUS, REQ_HOOKSHOT, REQ_ICE_ARROWS, //REQ_FARORES // Will never be required or obtainable, but here for ordering
    REQ_BOOMERANG, REQ_LENS,    REQ_BEANS,    REQ_HAMMER,   REQ_LIGHT_ARROWS, REQ_NAYRUS,

    //Equipment
    REQ_BGS,
    REQ_GORON_TUNIC, REQ_ZORA_TUNIC,
    REQ_SHIELD,      REQ_MIRROR_SHIELD,
    REQ_IRON_BOOTS,  REQ_HOVER_BOOTS,
    REQ_STRENGTH,
} Arcade_SpawnLogicRequirements;

typedef enum {
    AGE_NONE,
    AGE_ADULT,
    AGE_CHILD
} Arcade_AgeRequirements;

typedef enum {
    TRICK_NONE,
    TRICK_GROUNDJUMP,
    TRICK_WEIRDSHOT,
    TRICK_QPA,
    TRICK_ISG,
} Arcade_TrickRequirements;