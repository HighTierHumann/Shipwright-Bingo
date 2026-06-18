#include "Arcade.h"
#include "soh/ShipInit.hpp"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/Enhancements/cosmetics/cosmeticsTypes.h"
#include "soh_assets.h"
#include "soh/frame_interpolation.h"
#include "soh/OTRGlobals.h"
#include "include/z64camera.h"
#include "soh/Notification/Notification.h"

#include <array>
#include <string>
#include <vector>
#include <random>
#include <unordered_map>
#include <cmath>

extern "C" {
#include "functions.h"
#include "macros.h"
#include "variables.h"
#include "src/overlays/actors/ovl_Boss_Goma/z_boss_goma.h"
#include "src/overlays/actors/ovl_Boss_Mo/z_boss_mo.h"
#include "src/overlays/actors/ovl_Door_Warp1/z_door_warp1.h"
#include "src/overlays/gamestates/ovl_file_choose/file_choose.h"
#include "objects/gameplay_keep/gameplay_keep.h"
#include "textures/icon_item_nes_static/icon_item_nes_static.h"
#include "textures/icon_item_ger_static/icon_item_ger_static.h"
#include "textures/icon_item_fra_static/icon_item_fra_static.h"
#include "soh/Enhancements/randomizer/randomizer_grotto.h"

#include "src/overlays/actors/ovl_En_Door/z_en_door.h"
#include "src/overlays/actors/ovl_Door_Shutter/z_door_shutter.h"
#include "src/overlays/actors/ovl_En_Holl/z_en_holl.h"
#include "src/overlays/actors/ovl_Bg_Heavy_block/z_bg_heavy_block.h"

#include "src/overlays/actors/ovl_En_Bb/z_en_bb.h"
#include "src/overlays/actors/ovl_En_Dekubaba/z_en_dekubaba.h"
#include "src/overlays/actors/ovl_En_Mb/z_en_mb.h"
#include "src/overlays/actors/ovl_En_Tite/z_en_tite.h"
#include "src/overlays/actors/ovl_En_Zf/z_en_zf.h"
#include "src/overlays/actors/ovl_En_Wf/z_en_wf.h"
#include "src/overlays/actors/ovl_En_Reeba/z_en_reeba.h"
#include "src/overlays/actors/ovl_En_Peehat/z_en_peehat.h"
#include "src/overlays/actors/ovl_En_Po_Field/z_en_po_field.h"
#include "src/overlays/actors/ovl_En_Poh/z_en_poh.h"
#include "src/overlays/actors/ovl_En_Tp/z_en_tp.h"
#include "src/overlays/actors/ovl_En_Firefly/z_en_firefly.h"
#include "src/overlays/actors/ovl_En_Rd/z_en_rd.h"
#include "src/overlays/actors/ovl_En_Am/z_en_am.h"
#include "src/overlays/actors/ovl_En_Ik/z_en_ik.h"
#include "src/overlays/actors/ovl_En_Test/z_en_test.h"
#include "src/overlays/actors/ovl_En_Crow/z_en_crow.h"
#include "src/overlays/actors/ovl_En_Wonder_Item/z_en_wonder_item.h"
#include "src/overlays/actors/ovl_En_Bom/z_en_bom.h"
#include "src/overlays/actors/ovl_Bg_Mori_Elevator/z_bg_mori_elevator.h"

extern PlayState* gPlayState;

Gfx* KaleidoScope_QuadTextureIA8(Gfx* gfx, void* texture, s16 width, s16 height, u16 point);
void FileChoose_UpdateStickDirectionPromptAnim(GameState* thisx);
void FileChoose_DrawTextRec(GraphicsContext* gfxCtx, s32 r, s32 g, s32 b, s32 a, f32 x, f32 y, f32 z, s32 s, s32 t,
                            f32 dx, f32 dy);
}

static s32 sArcadeScore = -1;
static s16 sArcadeOverrideEntrance = -1;
static s8 sArcadeOverrideRoom = -1;
static s8 sArcadeTransitionsMade = -1;
static ActorEntry modifiedLinkActorEntry = { 0 };
static bool sArcadeLockClosestDoor = false;

// Use the existing UI variables from z_parameter.c
extern "C" {
    extern const char* digitTextures[];
    extern s16 Top_HUD_Margin;
    extern s16 Left_HUD_Margin;
    extern s16 Right_HUD_Margin;
    extern s16 Bottom_HUD_Margin;

    f32 gArcadeDamageMult;

    float OTRGetRectDimensionFromLeftEdge(float v);
    float OTRGetRectDimensionFromRightEdge(float v);
    s16 Grotto_GetEntranceValueHandlingGrottoRando(s16 nextEntranceIndex);

    void DoorShutter_SetupAction(DoorShutter*, DoorShutterActionFunc);
    void DoorShutter_SetupType(DoorShutter*, PlayState*);
    void EnHoll_Draw(Actor* thisx, PlayState* play);
}

typedef enum {
    AM_CHOICE_BOSSES_ALL,
    AM_CHOICE_BOSSES_CHILD,
    AM_CHOICE_BOSSES_ADULT,
    AM_CHOICE_BOSSES_GANONDORF_GANON
} ArcadeBossesChoices;

typedef enum {
    AM_CHOICE_HEARTS_10,
    AM_CHOICE_HEARTS_15,
    AM_CHOICE_HEARTS_20,
    AM_CHOICE_HEARTS_3,
    AM_CHOICE_HEARTS_5,
    AM_CHOICE_HEARTS_7
} ArcadeHeartsChoices;

typedef enum {
    AM_CHOICE_AMMO_LIMITED,
    AM_CHOICE_AMMO_FULL,
    AM_CHOICE_AMMO_MAXED,
} ArcadeAmmoChoices;

typedef enum {
    AM_CHOICE_HEAL_GANONDORF,
    AM_CHOICE_HEAL_EVERYBOSS,
    AM_CHOICE_HEAL_NEVER,
} ArcadeHealChoices;

typedef enum {
    AM_CHOICE_MAGIC_SINGLE,
    AM_CHOICE_MAGIC_DOUBLE,
} ArcadeMagicChoices;

typedef enum {
    AM_CHOICE_BGS_NO,
    AM_CHOICE_BGS_YES,
} ArcadeBgsChoices;

typedef enum {
    AM_CHOICE_BOTTLE_NO,
    AM_CHOICE_BOTTLE_EMPTY,
    AM_CHOICE_BOTTLE_FAIRY,
    AM_CHOICE_BOTTLE_REDPOTION,
    AM_CHOICE_BOTTLE_GREENPOTION,
    AM_CHOICE_BOTTLE_BLUEPOTION
} ArcadeBottleChoices;

typedef enum {
    AM_CHOICE_LONGSHOT_NO,
    AM_CHOICE_LONGSHOT_YES,
} ArcadeLongshotChoices;

typedef enum {
    AM_CHOICE_HOVERBOOTS_NO,
    AM_CHOICE_HOVERBOOTS_YES,
} ArcadeHoverBootsChoices;

typedef enum {
    AM_CHOICE_BUNNYHOOD_NO,
    AM_CHOICE_BUNNYHOOD_YES,
} ArcadeBunnyHoodChoices;

typedef enum {
    AM_CHOICE_TIMER_YES,
    AM_CHOICE_TIMER_NO,
} ArcadeTimerChoices;

typedef struct ArcadeSetting {
    std::array<std::string, LANGUAGE_MAX> name;
    std::vector<std::array<std::string, LANGUAGE_MAX>> choices;
} ArcadeSetting;

struct ArcadeSpawn {
    s16 x;
    s16 y;
    s16 z;
    s16 rotY;
    u16 params;
    Arcade_SpawnLogicRequirements requirement = REQ_NONE;
    Arcade_TrickRequirements trickName = TRICK_NONE;
};

struct ArcadeEnemyTable {
    s32 enemyScore;
    std::string enemyName;
    s32 enemyParams;
};

// clang-format off
static const std::vector<std::pair<s16, ArcadeEnemyTable>> sEnemyScoreTable = {
    { ACTOR_EN_ANUBICE_TAG, {30  ,     "Anubis"               ,                                }}, // Anubis
    { ACTOR_EN_AM,          {15  ,     "Armos"                ,                                }}, // Armos
    { ACTOR_EN_DODOJR,      {5   ,     "Baby Dodongo"         ,                                }}, // Baby Dodongo
    { ACTOR_EN_VALI,        {20  ,     "Bari"                 ,                                }}, // Bari (big jellyfish)
    { ACTOR_EN_VM,          {20  ,     "Beamos"               ,                                }}, // Beamos
    { ACTOR_EN_BILI,        {10  ,     "Biri"                 ,                                }}, // Biri (jellyfish)
    { ACTOR_EN_BB,          {10  ,     "Blue Bubble"          ,  ENBB_BLUE                     }}, // Bubble (flying skull enemy) (blue)
    { ACTOR_EN_BB,          {10  ,     "Red Bubble"           ,  ENBB_RED                      }}, // Bubble (flying skull enemy) (red)
    { ACTOR_EN_BB,          {10  ,     "Green Bubble"         ,  ENBB_GREEN                    }}, // Bubble (flying skull enemy) (green)
    { ACTOR_EN_BB,          {10  ,     "Big Green Bubble"     ,  ENBB_GREEN_BIG                }}, // Bubble (flying skull enemy) (green big)
    { ACTOR_EN_TORCH2,      {1000,     "Dark Link"            ,                                }}, // Dark Link
    { ACTOR_EN_DEKUBABA,    {10  ,     "Large Deku Baba"      ,  DEKUBABA_BIG                  }}, // Deku Baba (large)
    { ACTOR_EN_DEKUBABA,    {10  ,     "Small Deku Baba"      ,  DEKUBABA_NORMAL               }}, // Deku Baba (small)
    { ACTOR_EN_DODONGO,     {20  ,     "Dodongo"              ,                                }}, // Dodongo
    { ACTOR_EN_TP,          {10  ,     "Electric Tailpasaran" ,                                }}, // Electric Tailpasaran
    { ACTOR_EN_FD,          {200 ,     "Flare Dancer"         ,                                }}, // Flare Dancer
    { ACTOR_EN_FLOORMAS,    {50  ,     "Floormaster"          ,       0                        }}, // Floormaster
    { ACTOR_EN_FLOORMAS,    {10  ,     "Invisible Floormaster",      -1                        }}, // Floormaster (invisible)
    { ACTOR_EN_PEEHAT,      {30  ,     "Peahat"               ,  PEAHAT_TYPE_FLYING            }}, // Flying Peahat
    { ACTOR_EN_PEEHAT,      {10  ,     "Peahat Larva"         ,  PEAHAT_TYPE_LARVA             }}, // Flying Peahat Larva
    { ACTOR_EN_TUBO_TRAP,   {0   ,     "Flying Pot"           ,                                }}, // Flying pot
    { ACTOR_EN_FZ,          {30  ,     "Freezard"             ,                                }}, // Freezard
    { ACTOR_EN_GELDB,       {100 ,     "Gerudo Guard"         ,                                }}, // Gerudo Fighter
    { ACTOR_EN_GOMA,        {10  ,     "Gohma Larva"          ,                                }}, // Gohma Larva (Non-Gohma rooms)
    { ACTOR_EN_CROW,        {10  ,     "Guay"                 ,                                }}, // Guay
    { ACTOR_EN_IK,          {300 ,     "Black Iron Knuckle"   ,  EN_IK_BLACK                   }}, // Iron Knuckle (black, standing)
    { ACTOR_EN_IK,          {300 ,     "White Iron Knuckle"   ,  EN_IK_WHITE                   }}, // Iron Knuckle (white, standing)
    { ACTOR_EN_IK,          {300 ,     "White Iron Knuckle"   ,  EN_IK_WHITE_CHAIR             }}, // Iron Knuckle (white, chair)
    { ACTOR_EN_FIREFLY,     {10  ,     "Keese"                ,  KEESE_NORMAL_FLY              }}, // Keese
    { ACTOR_EN_FIREFLY,     {10  ,     "Keese"                ,  KEESE_NORMAL_PERCH            }}, // Keese
    { ACTOR_EN_FIREFLY,     {15  ,     "Invisible Keese"      ,  KEESE_NORMAL_INVIS            }}, // Keese (invisible)
    { ACTOR_EN_FIREFLY,     {15  ,     "Fire Keese"           ,                                }}, // Keese (Fire)
    { ACTOR_EN_FIREFLY,     {15  ,     "Fire Keese"           ,  KEESE_FIRE_PERCH              }}, // Keese (Fire)
    { ACTOR_EN_FIREFLY,     {20  ,     "Invisible Fire Keese" ,  KEESE_FIRE_INVIS              }}, // Keese (Fire invisible)
    { ACTOR_EN_FIREFLY,     {15  ,     "Ice Keese"            ,  KEESE_ICE_FLY                 }}, // Keese (Ice)
    { ACTOR_EN_FIREFLY,     {20  ,     "Invisible Ice Keese"  ,  KEESE_ICE_INVIS               }}, // Keese (Ice invisible)
    { ACTOR_EN_REEBA,       {10  ,     "Leever"               ,  LEEVER_SMALL                  }}, // Leever
    { ACTOR_EN_REEBA,       {100 ,     "Purple Leever"        ,  LEEVER_BIG                    }}, // Leever (Purple)
    { ACTOR_EN_RR,          {10  ,     "Like-Like"            ,                                }}, // Like-Like
    { ACTOR_EN_ZF,          {10  ,     "Lizalfos"             ,  ENZF_TYPE_LIZALFOS_LONE       }}, // Lizalfos (normal)
    { ACTOR_EN_ZF,          {10  ,     "Lizalfos"             ,  ENZF_TYPE_LIZALFOS_MINIBOSS_A }}, // Lizalfos (miniboss A)
    { ACTOR_EN_ZF,          {10  ,     "Lizalfos"             ,  ENZF_TYPE_LIZALFOS_MINIBOSS_B }}, // Lizalfos (miniboss A)
    { ACTOR_EN_ZF,          {100 ,     "Dinolfos"             ,  ENZF_TYPE_DINOLFOS            }}, // Lizalfos (dinolfos)
    { ACTOR_EN_DEKUNUTS,    {10  ,     "Mad Scrub"            ,                                }}, // Mad Scrub (triple attack)
    { ACTOR_EN_MB,          {50  ,     "Club Moblin"          ,  ENMB_TYPE_CLUB                }}, // Moblin Club
    { ACTOR_EN_MB,          {70  ,     "Spear Moblin"         ,  ENMB_TYPE_SPEAR_PATROL        }}, // Moblin Spear
    { ACTOR_EN_OKUTA,       {10  ,     "Octorok"              ,                                }}, // Octorok
    { ACTOR_EN_POH,         {10  ,     "Poe"                  ,  EN_POH_NORMAL                 }}, // Poe
    { ACTOR_EN_POH,         {10  ,     "Sharp"                ,  EN_POH_SHARP                  }}, // Poe (composer Sharp)
    { ACTOR_EN_POH,         {10  ,     "Flat"                 ,  EN_POH_FLAT                   }}, // Poe (composer Flat)
    { ACTOR_EN_RD,          {20  ,     "Invisible Redead"     ,  EN_RD_REDEAD_INVIS            }}, // Redead (invisible)
    { ACTOR_EN_RD,          {10  ,     "Redead"               ,  EN_RD_REDEAD_NORMAL           }}, // Redead (standing)
    { ACTOR_EN_RD,          {30  ,     "Gibdo"                ,  EN_RD_GIBDO                   }}, // Redead (Gibdo)
    { ACTOR_EN_BUBBLE,      {5   ,     "Bubble (Literal)"     ,                                }}, // Shabom (bubble)
    { ACTOR_EN_SB,          {10  ,     "Shell Blade"          ,                                }}, // Shell Blade
    { ACTOR_EN_SKJ,         {10  ,     "Skull Kid"            ,                                }}, // Skull Kid
    { ACTOR_EN_ST,          {10  ,     "Skulltula"            ,                                }}, // Skulltula
    { ACTOR_EN_ST,          {10  ,     "Large Skulltula"      ,       1                        }}, // Skulltula (big)
    { ACTOR_EN_ST,          {10  ,     "Invisible Skulltula"  ,       2                        }}, // Skulltula (invisible)
    { ACTOR_EN_SW,          {10  ,     "Wall Skulltula"       ,                                }}, // Skulltula (wall)
    { ACTOR_EN_SW,          {75  ,     "Wall Skulltula"       ,     8196                       }}, // Skulltula (gold)
    { ACTOR_EN_NY,          {10  ,     "Spike"                ,                                }}, // Spike (rolling enemy)
    { ACTOR_EN_SKB,         {10  ,     "Stalchild"            ,                                }}, // Stalchild
    { ACTOR_EN_TEST,        {100 ,     "Stalfos"              ,                                }}, // Stalfos (cannot differentiate between normal and invisible, so award same score)
    { ACTOR_EN_EIYER,       {10  ,     "Stinger"              ,                                }}, // Stinger (land) (One in formation, sink under floor and do not activate)
    { ACTOR_EN_TITE,        {10  ,     "Red Tektite"          ,  TEKTITE_RED                   }}, // Tektite (red)
    { ACTOR_EN_TITE,        {20  ,     "Blue Tektite"         ,  TEKTITE_BLUE                  }}, // Tektite (blue)
    { ACTOR_EN_BW,          {10  ,     "Torch Slug"           ,                                }}, // Torch Slug
    { ACTOR_EN_WALLMAS,     {100 ,     "Wallmaster"           ,                                }}, // Wallmaster
    { ACTOR_EN_KAREBABA,    {5   ,     "Withered Deku Baba"   ,                                }}, // Withered Deku Baba
    { ACTOR_EN_WF,          {20  ,     "Wolfos"               ,  WOLFOS_NORMAL                 }}, // Wolfos (normal)
    { ACTOR_EN_WF,          {30  ,     "White Wolfos"         ,  WOLFOS_WHITE                  }}, // Wolfos (white)
};                                                                   

ArcadeSetting ArcadeOptions[AM_OPTIONS_MAX] = {
    { { "BOSSES:", "BOSSE:", "BOSS:" },
      {
          { "All", "Alle", "Tous" },
          { "Child", "Kind", "Enfant" },
          { "Adult", "Erwachsener", "Adulte" },
          { "Ganondorf & Ganon", "Ganondorf & Ganon", "Ganondorf & Ganon" },
      } },
    { { "HEARTS:", "HERZEN:", "COEURS:" },
      {
          { "10", "10", "10" },
          { "15", "15", "15" },
          { "20", "20", "20" },
          { "3", "3", "3" },
          { "5", "5", "5" },
          { "7", "7", "7" },
      } },
    { { "AMMO:", "MUNITION:", "MUNITIONS:" },
      {
          { "Limited", "Limitiert", "Limitées" },
          { "Full", "Voll", "Pleines" },
          { "Maxed", "Maximum", "Maximum" },
      } },
    { { "HEAL:", "REGENERATION:", "SOIN:" },
      {
          { "Before Ganondorf", "Vor Ganondorf", "Avant Ganondorf" },
          { "Every Boss", "Bei jedem Boss", "Tous les Boss" },
          { "Never", "Niemals", "Jamais" },
      } },
    { { "HYPER BOSSES:", "HYPER-BOSSE:", "HYPER BOSS:" },
      {
          { "No", "Nein", "Non" },
          { "Yes", "Ja", "Oui" },
      } },
    { { "MAGIC:", "MAGIE:", "MAGIE:" },
      {
          { "Single", "Einzel", "Simple" },
          { "Double", "Doppel", "Double" },
      } },
    { { "BIG. SWORD:", "BIG.-SCHWERT:", "EPÉE DE BIG.:" },
      {
          { "No", "Nein", "Non" },
          { "Yes", "Ja", "Oui" },
      } },
    { { "BOTTLE:", "FLASCHEN:", "BOUTEILLE:" },
      {
          { "No", "Nein", "Non" },
          { "Empty", "Leer", "Vide" },
          { "Fairy", "Fee", "Fée" },
          { "Red Potion", "Rotes Elixier", "Potion Rouge" },
          { "Green Potion", "Grünes Elixier", "Potion Verte" },
          { "Blue Potion", "Blaues Elixier", "Potion Bleue" },
      } },
    { { "LONGSHOT:", "ENTERHAKEN:", "SUPER GRAPPIN:" },
      {
          { "No", "Nein", "Non" },
          { "Yes", "Ja", "Oui" },
      } },
    { { "HOVER BOOTS:", "GLEITSTIEFEL:", "BOTTES DES AIRS:" },
      {
          { "No", "Nein", "Non" },
          { "Yes", "Ja", "Oui" },
      } },
    { { "BUNNY HOOD:", "HASENOHREN:", "MASQUE DU LAPIN:" },
      {
          { "No", "Nein", "Non" },
          { "Yes", "Ja", "Oui" },
      } },
    { { "TIMER:", "TIMER:", "TIMER:" },
      {
          { "Yes", "Ja", "Oui" },
          { "No", "Nein", "Non" },
      } }
};

const char* Arcade_GetSettingName(u8 optionIndex, u8 language) {
    return ArcadeOptions[optionIndex].name[language].c_str();
}

const char* Arcade_GetSettingChoiceName(u8 optionIndex, u8 choiceIndex, u8 language) {
    return ArcadeOptions[optionIndex].choices[choiceIndex][language].c_str();
}

u8 Arcade_GetSettingOptionsAmount(u8 optionIndex) {
    return static_cast<u8>(ArcadeOptions[optionIndex].choices.size());
}

void FileChoose_UpdateArcadeMenu(GameState* gameState) {
    static s8 sLastArcadeOptionIndex = -1;
    static s8 sLastArcadeOptionValue = -1;

    FileChoose_UpdateStickDirectionPromptAnim(gameState);
    FileChooseContext* fileChooseContext = (FileChooseContext*)gameState;
    Input* input = &fileChooseContext->state.input[0];
    bool dpad = CVarGetInteger(CVAR_SETTING("DpadInText"), 0);

    // Fade in elements after opening Boss Rush options menu
    fileChooseContext->arcadeUIAlpha += 25;
    if (fileChooseContext->arcadeUIAlpha > 255) {
        fileChooseContext->arcadeUIAlpha = 255;
    }

    // Animate up/down arrows.
    fileChooseContext->arcadeArrowOffset += 1;
    if (fileChooseContext->arcadeArrowOffset >= 30) {
        fileChooseContext->arcadeArrowOffset = 0;
    }

    // Move menu selection up or down.
    if (ABS(fileChooseContext->stickRelY) > 30 || (dpad && CHECK_BTN_ANY(input->press.button, BTN_DDOWN | BTN_DUP))) {
        // Move down
        if (fileChooseContext->stickRelY < -30 || (dpad && CHECK_BTN_ANY(input->press.button, BTN_DDOWN))) {
            // When selecting past the last option, cycle back to the first option.
            if ((fileChooseContext->arcadeIndex + 1) > AM_OPTIONS_MAX - 1) {
                fileChooseContext->arcadeIndex = 0;
                fileChooseContext->arcadeOffset = 0;
            } else {
                fileChooseContext->arcadeIndex++;
                // When last visible option is selected when moving down, offset the list down by one.
                if (fileChooseContext->arcadeIndex - fileChooseContext->arcadeOffset >
                    ARCADE_MAX_OPTIONS_ON_SCREEN - 1) {
                    fileChooseContext->arcadeOffset++;
                }
            }
        } else if (fileChooseContext->stickRelY > 30 || (dpad && CHECK_BTN_ANY(input->press.button, BTN_DUP))) {
            // When selecting past the first option, cycle back to the last option and offset the list to view it
            // properly.
            if ((fileChooseContext->arcadeIndex - 1) < 0) {
                fileChooseContext->arcadeIndex = AM_OPTIONS_MAX - 1;
                fileChooseContext->arcadeOffset =
                    fileChooseContext->arcadeIndex - ARCADE_MAX_OPTIONS_ON_SCREEN + 1;
            } else {
                // When first visible option is selected when moving up, offset the list up by one.
                if (fileChooseContext->arcadeIndex - fileChooseContext->arcadeOffset == 0) {
                    fileChooseContext->arcadeOffset--;
                }
                fileChooseContext->arcadeIndex--;
            }
        }

        Audio_PlaySoundGeneral(NA_SE_SY_FSEL_CURSOR, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale,
                               &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
    }

    // Cycle through choices for currently selected option.
    if (ABS(fileChooseContext->stickRelX) > 30 ||
        (dpad && CHECK_BTN_ANY(input->press.button, BTN_DLEFT | BTN_DRIGHT))) {
        if (fileChooseContext->stickRelX > 30 || (dpad && CHECK_BTN_ANY(input->press.button, BTN_DRIGHT))) {
            // If exceeding the amount of choices for the selected option, cycle back to the first.
            if ((gSaveContext.ship.quest.data.arcade.options[fileChooseContext->arcadeIndex] + 1) ==
                Arcade_GetSettingOptionsAmount(fileChooseContext->arcadeIndex)) {
                gSaveContext.ship.quest.data.arcade.options[fileChooseContext->arcadeIndex] = 0;
            } else {
                gSaveContext.ship.quest.data.arcade.options[fileChooseContext->arcadeIndex]++;
            }
        } else if (fileChooseContext->stickRelX < -30 || (dpad && CHECK_BTN_ANY(input->press.button, BTN_DLEFT))) {
            // If cycling back when already at the first choice for the selected option, cycle back to the last choice.
            if ((gSaveContext.ship.quest.data.arcade.options[fileChooseContext->arcadeIndex] - 1) < 0) {
                gSaveContext.ship.quest.data.arcade.options[fileChooseContext->arcadeIndex] =
                    Arcade_GetSettingOptionsAmount(fileChooseContext->arcadeIndex) - 1;
            } else {
                gSaveContext.ship.quest.data.arcade.options[fileChooseContext->arcadeIndex]--;
            }
        }

        Audio_PlaySoundGeneral(NA_SE_SY_FSEL_CURSOR, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale,
                               &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
    }

    if (sLastArcadeOptionIndex != fileChooseContext->arcadeIndex ||
        sLastArcadeOptionValue != gSaveContext.ship.quest.data.arcade.options[fileChooseContext->arcadeIndex]) {
        GameInteractor_ExecuteOnUpdateFileArcadeOptionSelection(
            fileChooseContext->arcadeIndex,
            gSaveContext.ship.quest.data.arcade.options[fileChooseContext->arcadeIndex]);
        sLastArcadeOptionIndex = fileChooseContext->arcadeIndex;
        sLastArcadeOptionValue = gSaveContext.ship.quest.data.arcade.options[fileChooseContext->arcadeIndex];
    }

    if (CHECK_BTN_ALL(input->press.button, BTN_B)) {
        fileChooseContext->configMode = CM_ARCADE_TO_QUEST;
        return;
    }

    // Load into the game.
    if (CHECK_BTN_ALL(input->press.button, BTN_START) || CHECK_BTN_ALL(input->press.button, BTN_A)) {
        Audio_PlaySoundGeneral(NA_SE_SY_FSEL_DECIDE_L, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale,
                               &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
        fileChooseContext->buttonIndex = 0xFD;
        fileChooseContext->menuMode = FS_MENU_MODE_SELECT;
        fileChooseContext->selectMode = SM_FADE_OUT;
        fileChooseContext->prevConfigMode = fileChooseContext->configMode;
        return;
    }
}

void FileChoose_DrawArcadeMenuWindowContents(FileChooseContext* fileChooseContext) {
    OPEN_DISPS(fileChooseContext->state.gfxCtx);

    uint8_t language = (gSaveContext.language == LANGUAGE_JPN) ? LANGUAGE_ENG : gSaveContext.language;
    uint8_t listOffset = fileChooseContext->arcadeOffset;
    uint8_t textAlpha = fileChooseContext->arcadeUIAlpha;

    // Draw arrows to indicate that the list can scroll up or down.
    // Arrow up
    if (listOffset > 0) {
        uint16_t arrowUpX = 140;
        uint16_t arrowUpY = 76 - (fileChooseContext->arcadeArrowOffset / 10);
        gDPLoadTextureBlock(POLY_OPA_DISP++, gArrowUpTex, G_IM_FMT_IA, G_IM_SIZ_16b, 16, 16, 0,
                            G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
                            G_TX_NOLOD);
        gSPWideTextureRectangle(POLY_OPA_DISP++, arrowUpX << 2, arrowUpY << 2, (arrowUpX + 8) << 2, (arrowUpY + 8) << 2,
                                G_TX_RENDERTILE, 0, 0, (1 << 11), (1 << 11));
    }
    // Arrow down
    if (AM_OPTIONS_MAX - listOffset > ARCADE_MAX_OPTIONS_ON_SCREEN) {
        uint16_t arrowDownX = 140;
        uint16_t arrowDownY = 181 + (fileChooseContext->arcadeArrowOffset / 10);
        gDPLoadTextureBlock(POLY_OPA_DISP++, gArrowDownTex, G_IM_FMT_IA, G_IM_SIZ_16b, 16, 16, 0,
                            G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
                            G_TX_NOLOD);
        gSPWideTextureRectangle(POLY_OPA_DISP++, arrowDownX << 2, arrowDownY << 2, (arrowDownX + 8) << 2,
                                (arrowDownY + 8) << 2, G_TX_RENDERTILE, 0, 0, (1 << 11), (1 << 11));
    }

    // Draw options. There's more options than what fits on the screen, so the visible options
    // depend on the current offset of the list. Currently selected option pulses in
    // color and has arrows surrounding the option.
    for (uint8_t i = listOffset; i - listOffset < ARCADE_MAX_OPTIONS_ON_SCREEN; i++) {
        uint16_t textYOffset = (i - listOffset) * 16;

        // Option name.
        Interface_DrawTextLine(fileChooseContext->state.gfxCtx, (char*)Arcade_GetSettingName(i, language), 65,
                               (87 + textYOffset), 255, 255, 80, textAlpha, 0.8f, true);

        // Selected choice for option.
        uint16_t finalKerning = Interface_DrawTextLine(
            fileChooseContext->state.gfxCtx,
            (char*)Arcade_GetSettingChoiceName(i, gSaveContext.ship.quest.data.arcade.options[i], language), 165,
            (87 + textYOffset), 255, 255, 255, textAlpha, 0.8f, true);

        // Draw arrows around selected option.
        if (fileChooseContext->arcadeIndex == i) {
            Gfx_SetupDL_39Opa(fileChooseContext->state.gfxCtx);
            gDPSetCombineMode(POLY_OPA_DISP++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
            gDPLoadTextureBlock(POLY_OPA_DISP++, gArrowCursorTex, G_IM_FMT_IA, G_IM_SIZ_8b, 16, 24, 0,
                                G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, 4, G_TX_NOMASK, G_TX_NOLOD,
                                G_TX_NOLOD);
            FileChoose_DrawTextRec(fileChooseContext->state.gfxCtx, fileChooseContext->stickLeftPrompt.arrowColorR,
                                   fileChooseContext->stickLeftPrompt.arrowColorG,
                                   fileChooseContext->stickLeftPrompt.arrowColorB, textAlpha, 160, (92 + textYOffset),
                                   0.42f, 0, 0, -1.0f, 1.0f);
            FileChoose_DrawTextRec(fileChooseContext->state.gfxCtx, fileChooseContext->stickRightPrompt.arrowColorR,
                                   fileChooseContext->stickRightPrompt.arrowColorG,
                                   fileChooseContext->stickRightPrompt.arrowColorB, textAlpha, (171 + finalKerning),
                                   (92 + textYOffset), 0.42f, 0, 0, 1.0f, 1.0f);
        }
    }

    CLOSE_DISPS(fileChooseContext->state.gfxCtx);
}

void Arcade_UpdateScore(s32 scoreToAdd) {
    sArcadeScore += scoreToAdd;
    return;
}

void Arcade_SpawnBlueWarps(PlayState* play) {

    // Spawn blue warps in Chamber of Sages based on what bosses have been defeated.
    if (gSaveContext.linkAge == LINK_AGE_CHILD) {
        // Forest Medallion (Gohma)
        if (!Flags_GetEventChkInf(EVENTCHKINF_USED_DEKU_TREE_BLUE_WARP)) {
            Actor_Spawn(&play->actorCtx, play, ACTOR_DOOR_WARP1, -100, 6, -170, 0, 0, 0, -1);
        }
        // Fire Medallion (King Dodongo)
        if (!Flags_GetEventChkInf(EVENTCHKINF_USED_DODONGOS_CAVERN_BLUE_WARP)) {
            Actor_Spawn(&play->actorCtx, play, ACTOR_DOOR_WARP1, 100, 6, -170, 0, 0, 0, -1);
        }
        // Water Medallion (Barinade)
        if (!Flags_GetEventChkInf(EVENTCHKINF_USED_JABU_JABUS_BELLY_BLUE_WARP)) {
            Actor_Spawn(&play->actorCtx, play, ACTOR_DOOR_WARP1, 199, 6, 0, 0, 0, 0, -1);
        }
    } else {
        // Light Medallion (Ganondorf)
        if (CheckDungeonCount() == 8) {
            Actor_Spawn(&play->actorCtx, play, ACTOR_DOOR_WARP1, -199, 6, 0, 0, 0, 0, -1);
        }
        // Forest Medallion (Phantom Ganondorf)
        if (!Flags_GetEventChkInf(EVENTCHKINF_USED_FOREST_TEMPLE_BLUE_WARP)) {
            Actor_Spawn(&play->actorCtx, play, ACTOR_DOOR_WARP1, -100, 6, -170, 0, 0, 0, -1);
        }
        // Fire Medallion (Volvagia)
        if (!Flags_GetEventChkInf(EVENTCHKINF_USED_FIRE_TEMPLE_BLUE_WARP)) {
            Actor_Spawn(&play->actorCtx, play, ACTOR_DOOR_WARP1, 100, 6, -170, 0, 0, 0, -1);
        }
        // Water Medallion (Morpha)
        if (!Flags_GetEventChkInf(EVENTCHKINF_USED_WATER_TEMPLE_BLUE_WARP)) {
            Actor_Spawn(&play->actorCtx, play, ACTOR_DOOR_WARP1, 199, 6, 0, 0, 0, 0, -1);
        }
        // Spirit Medallion (Twinrova)
        if (!Flags_GetRandomizerInf(RAND_INF_DUNGEONS_DONE_SPIRIT_TEMPLE)) {
            Actor_Spawn(&play->actorCtx, play, ACTOR_DOOR_WARP1, 100, 6, 170, 0, 0, 0, -1);
        }
        // Shadow Medallion (Bongo Bongo)
        if (!Flags_GetRandomizerInf(RAND_INF_DUNGEONS_DONE_SHADOW_TEMPLE)) {
            Actor_Spawn(&play->actorCtx, play, ACTOR_DOOR_WARP1, -100, 6, 170, 0, 0, 0, -1);
        }
    }
}

void Arcade_SetEquipment(u8 linkAge) {
    std::array<u8, 8> amButtonItems;
    std::array<u8, 7> amCButtonSlots;

    // Set Child Equipment.
    if (linkAge == LINK_AGE_CHILD) {
        amButtonItems = {
            ITEM_SWORD_KOKIRI, ITEM_STICK, ITEM_NUT, ITEM_BOMB, ITEM_NONE, ITEM_NONE, ITEM_NONE, ITEM_NONE
        };

        amCButtonSlots = { SLOT_STICK, SLOT_NUT, SLOT_BOMB, SLOT_NONE, SLOT_NONE, SLOT_NONE, SLOT_NONE };

        Inventory_ChangeEquipment(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_KOKIRI);
        // Set Adult equipment.
    } else {
        amButtonItems = { ITEM_SWORD_MASTER, ITEM_BOW,  ITEM_HAMMER, ITEM_BOMB,
                          ITEM_NONE,         ITEM_NONE, ITEM_NONE,   ITEM_NONE };

        amCButtonSlots = { SLOT_BOW, SLOT_HAMMER, SLOT_BOMB, SLOT_NONE, SLOT_NONE, SLOT_NONE, SLOT_NONE };

        Inventory_ChangeEquipment(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_MASTER);
    }

    // Button Items
    for (int button = 0; button < ARRAY_COUNT(gSaveContext.equips.buttonItems); button++) {
        gSaveContext.equips.buttonItems[button] = amButtonItems[button];
    }

    // C buttons
    for (int button = 0; button < ARRAY_COUNT(gSaveContext.equips.cButtonSlots); button++) {
        gSaveContext.equips.cButtonSlots[button] = amCButtonSlots[button];
    }
}

void Arcade_HandleBlueWarp(PlayState* play, f32 warpPosX, f32 warpPosZ) {
    // If warping from Chamber of Sages, choose the correct boss room to teleport to.
    if (play->sceneNum == SCENE_CHAMBER_OF_THE_SAGES) {
        // Gohma & Phantom Ganon
        if (warpPosX == -100 && warpPosZ == -170) {
            if (gSaveContext.linkAge == LINK_AGE_CHILD) {
                play->nextEntranceIndex = ENTR_DEKU_TREE_BOSS_ENTRANCE;
            } else {
                play->nextEntranceIndex = ENTR_FOREST_TEMPLE_BOSS_ENTRANCE;
            }
            // King Dodongo & Volvagia
        } else if (warpPosX == 100 && warpPosZ == -170) {
            if (gSaveContext.linkAge == LINK_AGE_CHILD) {
                play->nextEntranceIndex = ENTR_DODONGOS_CAVERN_BOSS_ENTRANCE;
            } else {
                play->nextEntranceIndex = ENTR_FIRE_TEMPLE_BOSS_ENTRANCE;
            }
            // Barinade & Morb
        } else if (warpPosX == 199 && warpPosZ == 0) {
            if (gSaveContext.linkAge == LINK_AGE_CHILD) {
                play->nextEntranceIndex = ENTR_JABU_JABU_BOSS_ENTRANCE;
            } else {
                play->nextEntranceIndex = ENTR_WATER_TEMPLE_BOSS_ENTRANCE;
            }
            // Twinrova
        } else if (warpPosX == 100 && warpPosZ == 170) {
            play->nextEntranceIndex = ENTR_SPIRIT_TEMPLE_BOSS_2;
            // Bongo Bongo
        } else if (warpPosX == -100 && warpPosZ == 170) {
            play->nextEntranceIndex = ENTR_SHADOW_TEMPLE_BOSS_ENTRANCE;
            // Ganondork
        } else if (warpPosX == -199 && warpPosZ == 0) {
            play->nextEntranceIndex = ENTR_GANONDORF_BOSS_0;
        } else {
            SPDLOG_ERROR("[Arcade]: Unknown blue warp in chamber of sages at position ({}, {}). Warping back to "
                         "chamber of sages.",
                         warpPosX, warpPosZ);
            play->nextEntranceIndex = ENTR_CHAMBER_OF_THE_SAGES_0;
        }
        // If coming from a boss room, teleport back to Chamber of Sages and set flag.
    } else {
        play->nextEntranceIndex = ENTR_CHAMBER_OF_THE_SAGES_0;

        if (CheckDungeonCount() == 3) {
            play->linkAgeOnLoad = LINK_AGE_ADULT;
            gSaveContext.linkAge = LINK_AGE_ADULT;

            // Change to Adult Link.
            if (gSaveContext.ship.quest.data.arcade.options[AM_OPTIONS_BOSSES] == AM_CHOICE_BOSSES_ALL) {
                Arcade_SetEquipment(LINK_AGE_ADULT);
                // Warp to credits.
            } else if (gSaveContext.ship.quest.data.arcade.options[AM_OPTIONS_BOSSES] == AM_CHOICE_BOSSES_CHILD) {
                play->nextEntranceIndex = ENTR_CHAMBER_OF_THE_SAGES_0;
                gSaveContext.nextCutsceneIndex = 0xFFF2;
                play->transitionTrigger = TRANS_TRIGGER_START;
                play->transitionType = TRANS_TYPE_FADE_WHITE;
            }
        }
    }

    play->transitionTrigger = TRANS_TRIGGER_START;
    play->transitionType = TRANS_TYPE_FADE_WHITE;
    gSaveContext.nextTransitionType = TRANS_TYPE_FADE_WHITE_SLOW;
}

void Arcade_HandleBlueWarpHeal(PlayState* play) {

    // This function gets called multiple times per blue warp, so only heal when player isn't at max HP.
    if (gSaveContext.ship.quest.data.arcade.options[AM_OPTIONS_HEAL] == AM_CHOICE_HEAL_EVERYBOSS &&
        gSaveContext.health != gSaveContext.healthCapacity) {
        Health_ChangeBy(play, 320);
    }
}

void Arcade_HandleCompleteBoss(PlayState* play) {
    Arcade_UpdateScore(1000);
    gSaveContext.ship.quest.data.arcade.isPaused = true;
    switch (play->sceneNum) {
        case SCENE_DEKU_TREE_BOSS:
            Flags_SetEventChkInf(EVENTCHKINF_USED_DEKU_TREE_BLUE_WARP);
            break;
        case SCENE_DODONGOS_CAVERN_BOSS:
            Flags_SetEventChkInf(EVENTCHKINF_USED_DODONGOS_CAVERN_BLUE_WARP);
            break;
        case SCENE_JABU_JABU_BOSS:
            Flags_SetEventChkInf(EVENTCHKINF_USED_JABU_JABUS_BELLY_BLUE_WARP);
            break;
        case SCENE_FOREST_TEMPLE_BOSS:
            Flags_SetEventChkInf(EVENTCHKINF_USED_FOREST_TEMPLE_BLUE_WARP);
            break;
        case SCENE_FIRE_TEMPLE_BOSS:
            Flags_SetEventChkInf(EVENTCHKINF_USED_FIRE_TEMPLE_BLUE_WARP);
            break;
        case SCENE_WATER_TEMPLE_BOSS:
            Flags_SetEventChkInf(EVENTCHKINF_USED_WATER_TEMPLE_BLUE_WARP);
            break;
        case SCENE_SPIRIT_TEMPLE_BOSS:
            Flags_SetRandomizerInf(RAND_INF_DUNGEONS_DONE_SPIRIT_TEMPLE);
            break;
        case SCENE_SHADOW_TEMPLE_BOSS:
            Flags_SetRandomizerInf(RAND_INF_DUNGEONS_DONE_SHADOW_TEMPLE);
            break;
        default:
            break;
    }

    // Fully heal the player after Ganondorf
    if (gSaveContext.ship.quest.data.arcade.options[AM_OPTIONS_HEAL] == AM_CHOICE_HEAL_EVERYBOSS &&
        play->sceneNum == SCENE_GANONDORF_BOSS) {
        Health_ChangeBy(play, 320);
    }

    if ((CheckDungeonCount() == 3 &&
         gSaveContext.ship.quest.data.arcade.options[AM_OPTIONS_BOSSES] == AM_CHOICE_BOSSES_CHILD) ||
        play->sceneNum == SCENE_GANON_BOSS) {
        gSaveContext.ship.stats.playTimer += 2;
        gSaveContext.ship.stats.gameComplete = 1;
        gSaveContext.ship.stats.itemTimestamp[TIMESTAMP_ARCADE_FINISH] =
            static_cast<uint32_t>(GAMEPLAYSTAT_TOTAL_TIME);
    }
}

void Arcade_HandleEnemyDefeated(Actor* killedActor) {
    if (killedActor == nullptr) {
        return;
    }

    Player* player = GET_PLAYER(gPlayState);

    for (const auto& entry : sEnemyScoreTable) {
        int16_t tableActorId = entry.first;
        const ArcadeEnemyTable& enemyData = entry.second;

        if (tableActorId == killedActor->id) {
            if (enemyData.enemyParams == NULL || enemyData.enemyParams == killedActor->params) {
                int32_t scoreReward = enemyData.enemyScore;
                Arcade_UpdateScore(scoreReward);

                std::string messagePrefix = "+" + std::to_string(scoreReward);
                std::string messageSuffix = enemyData.enemyName;

                Notification::Emit({
                    .prefix = messagePrefix,
                    .prefixColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f),
                    .message = "points awarded for killing",
                    .suffix = messageSuffix,
                    .suffixColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
                    .mute = true
                    });
            }
        }
    }

    if (killedActor->xzDistToPlayer > 800.0f) {
        int32_t marksmanBonus = 50;
        int32_t distanceFromPlayer = killedActor->xzDistToPlayer;
        int32_t distanceFromPlayerBonus = 0;
        if (player->heldItemId == ITEM_SLINGSHOT) {
            distanceFromPlayerBonus = ((distanceFromPlayer - 800) / 10) * 10;
        }
        else if (player->heldItemId == ITEM_BOW) {
            distanceFromPlayerBonus = ((distanceFromPlayer - 800) / 20) * 10; // Currently, actors thats position is changed when struck by an arrow (Keese in particular) can award HUGE points when they finally die, so only award half the points
        }

        marksmanBonus += distanceFromPlayerBonus;
        Arcade_UpdateScore(marksmanBonus);

        std::string messagePrefix = "+" + std::to_string(marksmanBonus);
        std::string messageSuffix = std::to_string(distanceFromPlayer) + " units";

        Notification::Emit({
            .prefix = messagePrefix,
            .prefixColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f),
            .message = "point Marksman bonus! Enemy distance:",
            .suffix = messageSuffix,
            .suffixColor = ImVec4(0.0f, 0.5f, 1.0f, 1.0f),
            .mute = true,
            });
        Audio_PlaySoundGeneral(NA_SE_SY_GET_ITEM, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale,
            &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
    }
}

void Arcade_HandleWonderCollect(Actor* wonderActor) {
    EnWonderItem* wonder = (EnWonderItem*)wonderActor;

    if (wonder == nullptr) {
        return;
    }

    if (wonder->itemDrop == WONDERITEM_DROP_GREEN_RUPEE) {
        Arcade_UpdateScore(3);
    }
    else if (wonder->itemDrop == WONDERITEM_DROP_BLUE_RUPEE) {
        Arcade_UpdateScore(10);
    }
    else if (wonder->itemDrop == WONDERITEM_DROP_RED_RUPEE) {
        Arcade_UpdateScore(40);
    }
}

void Arcade_HandleNoDamage(int16_t damage) {
    bool wasDamaged = gSaveContext.ship.quest.data.arcade.takenDamage;
    if (!wasDamaged && damage < 0) {
        gSaveContext.ship.quest.data.arcade.takenDamage = true;
        SPDLOG_INFO("Player took damage");
    }
}

void Arcade_UpdateEnemyScaling() {
    int32_t timePlayed = (gSaveContext.ship.stats.playTimer / 2);
    if (timePlayed >= 30000) {
        gArcadeDamageMult = 7.0f;
    }
    else if (timePlayed >= 25000) {
        gArcadeDamageMult = 6.0f;
    }
    else if (timePlayed >= 20000) {
        gArcadeDamageMult = 5.0f;
    }
    else if (timePlayed >= 15000) {
        gArcadeDamageMult = 4.0f;
    }
    else if (timePlayed >= 10000) {
        gArcadeDamageMult = 3.0f;
    }
    else if (timePlayed >= 5000) {
        gArcadeDamageMult = 2.0f;
    }
}

static void Arcade_UpdateEnemySpeed(void* refActor) {
    int32_t timePlayed = (gSaveContext.ship.stats.playTimer / 2);
    bool updateActor = false;
    bool twiceSpeed = false;
    static float timeToAdd = 0.0f;

    Player* player = GET_PLAYER(gPlayState);
    Actor* actor = static_cast<Actor*>(refActor);

    // Some enemies are not in the ACTORCAT_ENEMY category, and some are that aren't really enemies.
    bool isEnemy = actor->category == ACTORCAT_ENEMY || actor->id == ACTOR_EN_TORCH2;
    bool isExcludedEnemy = actor->id == ACTOR_EN_FIRE_ROCK || actor->id == ACTOR_EN_ENCOUNT2;

    if (timePlayed >= 150) {
        timeToAdd += 0.5f;

        if (timeToAdd >= 1.0f) {
            updateActor = true;
            timeToAdd = 0.0f;
        }
        SPDLOG_INFO("timeToAdd, 2x Speed: {}", timeToAdd);
    } else if (timePlayed >= 50 && !twiceSpeed) { // I cannot get this to work no matter what
        timeToAdd += 0.5f;

        if (timeToAdd >= 1.0f) {
            updateActor = true;
            timeToAdd = 0.0f;
        }
        SPDLOG_INFO("timeToAdd: {}", timeToAdd);
    }

    if (timePlayed >= 100) {
        twiceSpeed = true;
    }

    // Don't apply during cutscenes because it causes weird behaviour and/or crashes on some cutscenes.
    if (isEnemy && !isExcludedEnemy && !Player_InBlockingCsMode(gPlayState, player) && twiceSpeed) {
        GameInteractor::RawAction::UpdateActor(actor);
    }
    if (isEnemy && !isExcludedEnemy && !Player_InBlockingCsMode(gPlayState, player) && updateActor) {
        GameInteractor::RawAction::UpdateActor(actor);
        updateActor = false;
    }
}

int Arcade_GetRandomEntrance() {
    static std::random_device rd;
    static std::mt19937 gen(rd());

    static const std::vector<s16> validEntrances = {
        // clang-format off
        // Index
        //ENTR_DEKU_TREE_ENTRANCE,
        //ENTR_KOKIRI_FOREST_OUTSIDE_DEKU_TREE,
        //ENTR_DODONGOS_CAVERN_ENTRANCE,
        //ENTR_DEATH_MOUNTAIN_TRAIL_OUTSIDE_DODONGOS_CAVERN,
        //ENTR_JABU_JABU_ENTRANCE,
        //ENTR_ZORAS_FOUNTAIN_OUTSIDE_JABU_JABU,
        ENTR_FOREST_TEMPLE_ENTRANCE,
        //ENTR_SACRED_FOREST_MEADOW_OUTSIDE_TEMPLE,
        //ENTR_FIRE_TEMPLE_ENTRANCE,
        //ENTR_DEATH_MOUNTAIN_CRATER_OUTSIDE_TEMPLE,
        //ENTR_WATER_TEMPLE_ENTRANCE,
        //ENTR_LAKE_HYLIA_OUTSIDE_TEMPLE,
        //ENTR_SPIRIT_TEMPLE_ENTRANCE,
        //ENTR_DESERT_COLOSSUS_OUTSIDE_TEMPLE,
        //ENTR_SHADOW_TEMPLE_ENTRANCE,
        //ENTR_GRAVEYARD_OUTSIDE_TEMPLE,
        //ENTR_BOTTOM_OF_THE_WELL_ENTRANCE,
        //ENTR_KAKARIKO_VILLAGE_OUTSIDE_BOTTOM_OF_THE_WELL,
        //ENTR_ICE_CAVERN_ENTRANCE,
        //ENTR_ZORAS_FOUNTAIN_OUTSIDE_ICE_CAVERN,
        //ENTR_GERUDO_TRAINING_GROUND_ENTRANCE,
        //ENTR_GERUDOS_FORTRESS_OUTSIDE_GERUDO_TRAINING_GROUND,
        //ENTR_INSIDE_GANONS_CASTLE_ENTRANCE,
        //ENTR_CASTLE_GROUNDS_RAINBOW_BRIDGE_EXIT,
        
        //ENTR_MIDOS_HOUSE_0,
        //ENTR_KOKIRI_FOREST_OUTSIDE_MIDOS_HOUSE,
        //ENTR_SARIAS_HOUSE_0,
        //ENTR_KOKIRI_FOREST_OUTSIDE_SARIAS_HOUSE,
        //ENTR_TWINS_HOUSE_0,
        //ENTR_KOKIRI_FOREST_OUTSIDE_TWINS_HOUSE,
        //ENTR_KNOW_IT_ALL_BROS_HOUSE_0,
        //ENTR_KOKIRI_FOREST_OUTSIDE_KNOW_IT_ALL_HOUSE,
        //ENTR_KOKIRI_SHOP_0,
        //ENTR_KOKIRI_FOREST_OUTSIDE_SHOP,
        //ENTR_LAKESIDE_LABORATORY_0,
        //ENTR_LAKE_HYLIA_OUTSIDE_LAB,
        //ENTR_FISHING_POND_0,
        //ENTR_LAKE_HYLIA_OUTSIDE_FISHING_POND,
        //ENTR_CARPENTERS_TENT_0,
        //ENTR_GERUDO_VALLEY_OUTSIDE_TENT,
        //ENTR_MARKET_GUARD_HOUSE_0,
        //ENTR_MARKET_ENTRANCE_OUTSIDE_GUARD_HOUSE,
        //ENTR_HAPPY_MASK_SHOP_0,
        //ENTR_MARKET_DAY_OUTSIDE_HAPPY_MASK_SHOP,
        //ENTR_BOMBCHU_BOWLING_ALLEY_0,
        //ENTR_MARKET_DAY_OUTSIDE_BOMBCHU_BOWLING,
        //ENTR_POTION_SHOP_MARKET_0,
        //ENTR_MARKET_DAY_OUTSIDE_POTION_SHOP,
        //ENTR_TREASURE_BOX_SHOP_0,
        //ENTR_MARKET_DAY_OUTSIDE_TREASURE_BOX_SHOP,
        //ENTR_BOMBCHU_SHOP_1,
        //ENTR_BACK_ALLEY_DAY_OUTSIDE_BOMBCHU_SHOP,
        //ENTR_BACK_ALLEY_MAN_IN_GREEN_HOUSE,
        //ENTR_BACK_ALLEY_DAY_OUTSIDE_MAN_IN_GREEN_HOUSE,
        //ENTR_KAKARIKO_CENTER_GUEST_HOUSE_0,
        //ENTR_KAKARIKO_VILLAGE_OUTSIDE_CENTER_GUEST_HOUSE,
        //ENTR_HOUSE_OF_SKULLTULA_0,
        //ENTR_KAKARIKO_VILLAGE_OUTSIDE_SKULKLTULA_HOUSE,
        //ENTR_IMPAS_HOUSE_FRONT,
        //ENTR_KAKARIKO_VILLAGE_OUTSIDE_IMPAS_HOUSE_FRONT,
        //ENTR_IMPAS_HOUSE_BACK,
        //ENTR_KAKARIKO_VILLAGE_OUTSIDE_IMPAS_HOUSE_BACK,
        //ENTR_POTION_SHOP_GRANNY_0,
        //ENTR_KAKARIKO_VILLAGE_OUTSIDE_SHOP_GRANNY,
        //ENTR_GRAVEKEEPERS_HUT_0,
        //ENTR_GRAVEYARD_OUTSIDE_DAMPES_HUT,
        //ENTR_GORON_SHOP_0,
        //ENTR_GORON_CITY_OUTSIDE_SHOP,
        //ENTR_ZORA_SHOP_0,
        //ENTR_ZORAS_DOMAIN_OUTSIDE_SHOP,
        //ENTR_LON_LON_BUILDINGS_TALONS_HOUSE,
        //ENTR_LON_LON_RANCH_OUTSIDE_TALONS_HOUSE,
        //ENTR_STABLE_0,
        //ENTR_LON_LON_RANCH_OUTSIDE_STABLES,
        //ENTR_LON_LON_BUILDINGS_TOWER,
        //ENTR_LON_LON_RANCH_OUTSIDE_TOWER,
        //ENTR_BAZAAR_1,
        //ENTR_MARKET_DAY_OUTSIDE_BAZAAR,
        //ENTR_SHOOTING_GALLERY_1,
        //ENTR_MARKET_DAY_OUTSIDE_SHOOTING_GALLERY,
        //ENTR_BAZAAR_0,
        //ENTR_KAKARIKO_VILLAGE_OUTSIDE_BAZAAR,
        //ENTR_SHOOTING_GALLERY_0,
        //ENTR_KAKARIKO_VILLAGE_OUTSIDE_SHOOTING_GALLERY,
        //ENTR_GREAT_FAIRYS_FOUNTAIN_SPELLS_NAYRUS_COLOSSUS,
        //ENTR_DESERT_COLOSSUS_GREAT_FAIRY_EXIT,
        //ENTR_GREAT_FAIRYS_FOUNTAIN_SPELLS_DINS_HC,
        //ENTR_CASTLE_GROUNDS_GREAT_FAIRY_EXIT,
        //ENTR_GREAT_FAIRYS_FOUNTAIN_MAGIC_OGC_DD,
        //// ENTR_POTION_SHOP_KAKARIKO_1 is an unused entrance index repurposed to differentiate between HC and OGC fairy fountain exits
        //// (normally both use ENTR_CASTLE_GROUNDS_GREAT_FAIRY_EXIT)
        //ENTR_POTION_SHOP_KAKARIKO_1,
        //ENTR_GREAT_FAIRYS_FOUNTAIN_MAGIC_DMC,
        //ENTR_DEATH_MOUNTAIN_CRATER_GREAT_FAIRY_EXIT,
        //ENTR_GREAT_FAIRYS_FOUNTAIN_MAGIC_DMT,
        //ENTR_DEATH_MOUNTAIN_TRAIL_GREAT_FAIRY_EXIT,
        //ENTR_GREAT_FAIRYS_FOUNTAIN_SPELLS_FARORES_ZF,
        //ENTR_ZORAS_FOUNTAIN_OUTSIDE_GREAT_FAIRY,

        //ENTR_LINKS_HOUSE_1,
        //ENTR_KOKIRI_FOREST_OUTSIDE_LINKS_HOUSE,
        //ENTR_TEMPLE_OF_TIME_ENTRANCE,
        //ENTR_TEMPLE_OF_TIME_EXTERIOR_DAY_OUTSIDE_TEMPLE,
        //ENTR_WINDMILL_AND_DAMPES_GRAVE_WINDMILL,
        //ENTR_KAKARIKO_VILLAGE_OUTSIDE_WINDMILL,
        //ENTR_POTION_SHOP_KAKARIKO_FRONT,
        //ENTR_KAKARIKO_VILLAGE_OUTSIDE_POTION_SHOP_FRONT,
        //ENTR_POTION_SHOP_KAKARIKO_BACK,
        //ENTR_KAKARIKO_VILLAGE_OUTSIDE_POTION_SHOP_BACK,

        //ENTR_THIEVES_HIDEOUT_0,
        //ENTR_GERUDOS_FORTRESS_1,
        //ENTR_THIEVES_HIDEOUT_1,
        //ENTR_GERUDOS_FORTRESS_2,
        //ENTR_THIEVES_HIDEOUT_2,
        //ENTR_GERUDOS_FORTRESS_3,
        //ENTR_THIEVES_HIDEOUT_3,
        //ENTR_GERUDOS_FORTRESS_4,
        //ENTR_THIEVES_HIDEOUT_4,
        //ENTR_GERUDOS_FORTRESS_5,
        //ENTR_THIEVES_HIDEOUT_5,
        //ENTR_GERUDOS_FORTRESS_6,
        //ENTR_THIEVES_HIDEOUT_6,
        //ENTR_GERUDOS_FORTRESS_7,
        //ENTR_THIEVES_HIDEOUT_7,
        //ENTR_GERUDOS_FORTRESS_8,
        //ENTR_THIEVES_HIDEOUT_8,
        //ENTR_GERUDOS_FORTRESS_9,
        //ENTR_THIEVES_HIDEOUT_9,
        //ENTR_GERUDOS_FORTRESS_10,
        //ENTR_THIEVES_HIDEOUT_10,
        //ENTR_GERUDOS_FORTRESS_11,
        //ENTR_THIEVES_HIDEOUT_11,
        //ENTR_GERUDOS_FORTRESS_12,
        //ENTR_THIEVES_HIDEOUT_12,
        //ENTR_GERUDOS_FORTRESS_13,

        // Grotto Loads use an entrance index of 0x0700 + their grotto id. The id is used as index for the
        // grottoLoadTable in soh/soh/Enhancements/randomizer/randomizer_grotto.c
        // Grotto Returns use an entrance index of 0x0800 + their grotto id. The id is used as index for the
        // grottoReturnTable in soh/soh/Enhancements/randomizer/randomizer_grotto.c
        //ENTRANCE_GROTTO_LOAD(GROTTO_COLOSSUS_OFFSET),
        //ENTRANCE_GROTTO_EXIT(GROTTO_COLOSSUS_OFFSET),
        //ENTRANCE_GROTTO_LOAD(GROTTO_LH_OFFSET),
        //ENTRANCE_GROTTO_EXIT(GROTTO_LH_OFFSET),
        //ENTRANCE_GROTTO_LOAD(GROTTO_ZR_STORMS_OFFSET),
        //ENTRANCE_GROTTO_EXIT(GROTTO_ZR_STORMS_OFFSET),
        //ENTRANCE_GROTTO_LOAD(GROTTO_ZR_FAIRY_OFFSET),
        //ENTRANCE_GROTTO_EXIT(GROTTO_ZR_FAIRY_OFFSET),
        //ENTRANCE_GROTTO_LOAD(GROTTO_ZR_OPEN_OFFSET),
        //ENTRANCE_GROTTO_EXIT(GROTTO_ZR_OPEN_OFFSET),
        //ENTRANCE_GROTTO_LOAD(GROTTO_DMC_HAMMER_OFFSET),
        //ENTRANCE_GROTTO_EXIT(GROTTO_DMC_HAMMER_OFFSET),
        //ENTRANCE_GROTTO_LOAD(GROTTO_DMC_UPPER_OFFSET),
        //ENTRANCE_GROTTO_EXIT(GROTTO_DMC_UPPER_OFFSET),
        //ENTRANCE_GROTTO_LOAD(GROTTO_GORON_CITY_OFFSET),
        //ENTRANCE_GROTTO_EXIT(GROTTO_GORON_CITY_OFFSET),
        //ENTRANCE_GROTTO_LOAD(GROTTO_DMT_STORMS_OFFSET),
        //ENTRANCE_GROTTO_EXIT(GROTTO_DMT_STORMS_OFFSET),
        //ENTRANCE_GROTTO_LOAD(GROTTO_DMT_COW_OFFSET),
        //ENTRANCE_GROTTO_EXIT(GROTTO_DMT_COW_OFFSET),
        //ENTRANCE_GROTTO_LOAD(GROTTO_KAK_OPEN_OFFSET),
        //ENTRANCE_GROTTO_EXIT(GROTTO_KAK_OPEN_OFFSET),
        //ENTRANCE_GROTTO_LOAD(GROTTO_KAK_REDEAD_OFFSET),
        //ENTRANCE_GROTTO_EXIT(GROTTO_KAK_REDEAD_OFFSET),
        //ENTRANCE_GROTTO_LOAD(GROTTO_HC_STORMS_OFFSET),
        //ENTRANCE_GROTTO_EXIT(GROTTO_HC_STORMS_OFFSET),
        //ENTRANCE_GROTTO_LOAD(GROTTO_HF_TEKTITE_OFFSET),
        //ENTRANCE_GROTTO_EXIT(GROTTO_HF_TEKTITE_OFFSET),
        //ENTRANCE_GROTTO_LOAD(GROTTO_HF_NEAR_KAK_OFFSET),
        //ENTRANCE_GROTTO_EXIT(GROTTO_HF_NEAR_KAK_OFFSET),
        //ENTRANCE_GROTTO_LOAD(GROTTO_HF_FAIRY_OFFSET),
        //ENTRANCE_GROTTO_EXIT(GROTTO_HF_FAIRY_OFFSET),
        //ENTRANCE_GROTTO_LOAD(GROTTO_HF_NEAR_MARKET_OFFSET),
        //ENTRANCE_GROTTO_EXIT(GROTTO_HF_NEAR_MARKET_OFFSET),
        //ENTRANCE_GROTTO_LOAD(GROTTO_HF_COW_OFFSET),
        //ENTRANCE_GROTTO_EXIT(GROTTO_HF_COW_OFFSET),
        //ENTRANCE_GROTTO_LOAD(GROTTO_HF_INSIDE_FENCE_OFFSET),
        //ENTRANCE_GROTTO_EXIT(GROTTO_HF_INSIDE_FENCE_OFFSET),
        //ENTRANCE_GROTTO_LOAD(GROTTO_HF_OPEN_OFFSET),
        //ENTRANCE_GROTTO_EXIT(GROTTO_HF_OPEN_OFFSET),
        //ENTRANCE_GROTTO_LOAD(GROTTO_HF_SOUTHEAST_OFFSET),
        //ENTRANCE_GROTTO_EXIT(GROTTO_HF_SOUTHEAST_OFFSET),
        //ENTRANCE_GROTTO_LOAD(GROTTO_LLR_OFFSET),
        //ENTRANCE_GROTTO_EXIT(GROTTO_LLR_OFFSET),
        //ENTRANCE_GROTTO_LOAD(GROTTO_SFM_WOLFOS_OFFSET),
        //ENTRANCE_GROTTO_EXIT(GROTTO_SFM_WOLFOS_OFFSET),
        //ENTRANCE_GROTTO_LOAD(GROTTO_SFM_STORMS_OFFSET),
        //ENTRANCE_GROTTO_EXIT(GROTTO_SFM_STORMS_OFFSET),
        //ENTRANCE_GROTTO_LOAD(GROTTO_SFM_FAIRY_OFFSET),
        //ENTRANCE_GROTTO_EXIT(GROTTO_SFM_FAIRY_OFFSET),
        //ENTRANCE_GROTTO_LOAD(GROTTO_LW_SCRUBS_OFFSET),
        //ENTRANCE_GROTTO_EXIT(GROTTO_LW_SCRUBS_OFFSET),
        //ENTRANCE_GROTTO_LOAD(GROTTO_LW_NEAR_SHORTCUTS_OFFSET),
        //ENTRANCE_GROTTO_EXIT(GROTTO_LW_NEAR_SHORTCUTS_OFFSET),
        //ENTRANCE_GROTTO_LOAD(GROTTO_KF_STORMS_OFFSET),
        //ENTRANCE_GROTTO_EXIT(GROTTO_KF_STORMS_OFFSET),
        //ENTRANCE_GROTTO_LOAD(GROTTO_ZD_STORMS_OFFSET),
        //ENTRANCE_GROTTO_EXIT(GROTTO_ZD_STORMS_OFFSET),
        //ENTRANCE_GROTTO_LOAD(GROTTO_GF_STORMS_OFFSET),
        //ENTRANCE_GROTTO_EXIT(GROTTO_GF_STORMS_OFFSET),
        //ENTRANCE_GROTTO_LOAD(GROTTO_GV_STORMS_OFFSET),
        //ENTRANCE_GROTTO_EXIT(GROTTO_GV_STORMS_OFFSET),
        //ENTRANCE_GROTTO_LOAD(GROTTO_GV_OCTOROK_OFFSET),
        //ENTRANCE_GROTTO_EXIT(GROTTO_GV_OCTOROK_OFFSET),
        //ENTRANCE_GROTTO_LOAD(GROTTO_LW_DEKU_THEATRE_OFFSET),
        //ENTRANCE_GROTTO_EXIT(GROTTO_LW_DEKU_THEATRE_OFFSET),
        
        // Graves have their own specified entrance indices
        //ENTR_GRAVE_WITH_FAIRYS_FOUNTAIN_0,
        //ENTR_GRAVEYARD_SHIELD_GRAVE_EXIT,
        //ENTR_REDEAD_GRAVE_0,
        //ENTR_GRAVEYARD_HEART_PIECE_GRAVE_EXIT,
        //ENTR_ROYAL_FAMILYS_TOMB_0,
        //ENTR_GRAVEYARD_ROYAL_TOMB_EXIT,
        //ENTR_WINDMILL_AND_DAMPES_GRAVE_GRAVE,
        //ENTR_GRAVEYARD_DAMPES_GRAVE_EXIT,

        //ENTR_LOST_WOODS_BRIDGE_EAST_EXIT,
        //ENTR_KOKIRI_FOREST_LOWER_EXIT,
        //ENTR_LOST_WOODS_SOUTH_EXIT,
        //ENTR_KOKIRI_FOREST_UPPER_EXIT,
        //ENTR_GORON_CITY_TUNNEL_SHORTCUT,
        //ENTR_LOST_WOODS_TUNNEL_SHORTCUT,
        //ENTR_ZORAS_RIVER_UNDERWATER_SHORTCUT,
        //ENTR_LOST_WOODS_UNDERWATER_SHORTCUT,
        //ENTR_SACRED_FOREST_MEADOW_SOUTH_EXIT,
        //ENTR_LOST_WOODS_NORTH_EXIT,
        //ENTR_HYRULE_FIELD_WOODED_EXIT,
        //ENTR_LOST_WOODS_BRIDGE_WEST_EXIT,
        //ENTR_LAKE_HYLIA_NORTH_EXIT,
        //ENTR_HYRULE_FIELD_FENCE_EXIT,
        //ENTR_GERUDO_VALLEY_EAST_EXIT,
        //ENTR_HYRULE_FIELD_ROCKY_PATH,
        //ENTR_MARKET_ENTRANCE_NEAR_GUARD_EXIT,
        //ENTR_HYRULE_FIELD_ON_BRIDGE_SPAWN,
        //ENTR_KAKARIKO_VILLAGE_FRONT_GATE,
        //ENTR_HYRULE_FIELD_STAIRS_EXIT,
        //ENTR_ZORAS_RIVER_WEST_EXIT,
        //ENTR_HYRULE_FIELD_RIVER_EXIT,
        //ENTR_LON_LON_RANCH_ENTRANCE,
        //ENTR_HYRULE_FIELD_CENTER_EXIT,
        //ENTR_ZORAS_DOMAIN_UNDERWATER_SHORTCUT,
        //ENTR_LAKE_HYLIA_UNDERWATER_SHORTCUT,
        //ENTR_GERUDOS_FORTRESS_EAST_EXIT,
        //ENTR_GERUDO_VALLEY_WEST_EXIT,
        //ENTR_HAUNTED_WASTELAND_EAST_EXIT,
        //ENTR_GERUDOS_FORTRESS_GATE_EXIT,
        //ENTR_DESERT_COLOSSUS_EAST_EXIT,
        //ENTR_HAUNTED_WASTELAND_WEST_EXIT,
        //ENTR_MARKET_SOUTH_EXIT,
        //ENTR_MARKET_ENTRANCE_NORTH_EXIT,
        //ENTR_CASTLE_GROUNDS_SOUTH_EXIT,
        //ENTR_MARKET_DAY_CASTLE_EXIT,
        //ENTR_TEMPLE_OF_TIME_EXTERIOR_DAY_GOSSIP_STONE_EXIT,
        //ENTR_MARKET_DAY_TEMPLE_EXIT,
        //ENTR_GRAVEYARD_ENTRANCE,
        //ENTR_KAKARIKO_VILLAGE_SOUTHEAST_EXIT,
        //ENTR_DEATH_MOUNTAIN_TRAIL_BOTTOM_EXIT,
        //ENTR_KAKARIKO_VILLAGE_GUARD_GATE,
        //ENTR_GORON_CITY_UPPER_EXIT,
        //ENTR_DEATH_MOUNTAIN_TRAIL_GC_EXIT,
        //ENTR_DEATH_MOUNTAIN_CRATER_GC_EXIT,
        //ENTR_GORON_CITY_DARUNIA_ROOM_EXIT,
        //ENTR_DEATH_MOUNTAIN_CRATER_UPPER_EXIT,
        //ENTR_DEATH_MOUNTAIN_TRAIL_SUMMIT_EXIT,
        //ENTR_ZORAS_DOMAIN_ENTRANCE,
        //ENTR_ZORAS_RIVER_WATERFALL_EXIT,
        //ENTR_ZORAS_FOUNTAIN_TUNNEL_EXIT,
        //ENTR_ZORAS_DOMAIN_KING_ZORA_EXIT,
        //
        //ENTR_LAKE_HYLIA_RIVER_EXIT,
        //
        //ENTR_HYRULE_FIELD_OWL_DROP,
        //ENTR_KAKARIKO_VILLAGE_OWL_DROP,
        //
        //ENTR_LINKS_HOUSE_CHILD_SPAWN,
        //ENTR_HYRULE_FIELD_10, // 0x282 is an unused entrance index repurposed to differentiate between
        //                        // Adult Spawn and prelude of light (normally they both use 0x5F4)
        //ENTR_SACRED_FOREST_MEADOW_WARP_PAD,
        //ENTR_DEATH_MOUNTAIN_CRATER_WARP_PAD,
        //ENTR_LAKE_HYLIA_WARP_PAD,
        //ENTR_DESERT_COLOSSUS_WARP_PAD,
        //ENTR_GRAVEYARD_WARP_PAD,
        //ENTR_TEMPLE_OF_TIME_WARP_PAD,
        //
        //ENTR_DEKU_TREE_BOSS_ENTRANCE,
        //ENTR_DODONGOS_CAVERN_BOSS_ENTRANCE,
        //ENTR_JABU_JABU_BOSS_ENTRANCE,
        //ENTR_FOREST_TEMPLE_BOSS_ENTRANCE,
        //ENTR_FIRE_TEMPLE_BOSS_ENTRANCE,
        //ENTR_WATER_TEMPLE_BOSS_ENTRANCE,
        //ENTR_SPIRIT_TEMPLE_BOSS_ENTRANCE,
        //ENTR_SHADOW_TEMPLE_BOSS_ENTRANCE,
        //
        //ENTR_GANONS_TOWER_0,
        //
        //ENTR_KOKIRI_FOREST_DEKU_TREE_BLUE_WARP,
        //ENTR_DEATH_MOUNTAIN_TRAIL_DODONGO_BLUE_WARP,
        //ENTR_ZORAS_FOUNTAIN_JABU_JABU_BLUE_WARP,
        //ENTR_SACRED_FOREST_MEADOW_FOREST_TEMPLE_BLUE_WARP,
        //ENTR_DEATH_MOUNTAIN_CRATER_FIRE_TEMPLE_BLUE_WARP,
        //ENTR_LAKE_HYLIA_WATER_TEMPLE_BLUE_WARP,
        //ENTR_DESERT_COLOSSUS_SPIRIT_TEMPLE_BLUE_WARP,
        //ENTR_GRAVEYARD_SHADOW_TEMPLE_BLUE_WARP,
        //ENTR_OUTSIDE_GANONS_CASTLE_1_2,
        // clang-format on
    };

    if (validEntrances.empty()) {
        // If this happens, something went VERY wrong
        return ENTR_CHAMBER_OF_THE_SAGES_0;
    }

    std::uniform_int_distribution<> distrib(0, static_cast<int>(validEntrances.size() - 1));
    return validEntrances[distrib(gen)];
}

u8 Arcade_GetSceneRoom(s16 sceneId) {
    static const std::unordered_map<s16, std::vector<s16>> sceneValidRooms = {
        { SCENE_DEKU_TREE,       { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA } },
        { SCENE_DODONGOS_CAVERN, { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF } },
        { SCENE_JABU_JABU,       { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF } },
        { SCENE_FOREST_TEMPLE,   { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8,      0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15 } }, // Well omitted
        { SCENE_FIRE_TEMPLE,     { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8,      0xA, 0xB,      0xD, 0xE, 0xF, 0x10, 0x11, 0x12, 0x13,       0x15,       0x18 } }, // Gorons & barren rooms omitted
        //{ SCENE_WATER_TEMPLE, 23 },
        //{ SCENE_SPIRIT_TEMPLE, 30 },
        //{ SCENE_SHADOW_TEMPLE, 28 },
        //{ SCENE_BOTTOM_OF_THE_WELL, 7 },
        //{ SCENE_ICE_CAVERN, 12 },
        //{ SCENE_GERUDO_TRAINING_GROUND, 11 },
        //{ SCENE_THIEVES_HIDEOUT, 14 },
        //{ SCENE_INSIDE_GANONS_CASTLE, 19 },
        //{ SCENE_GANONS_TOWER, 10 },
    };

    static std::random_device rd;
    static std::mt19937 gen(rd());

    auto it = sceneValidRooms.find(sceneId);

    if (it != sceneValidRooms.end()) {
        const std::vector<s16>& roomList = it->second;
        std::uniform_int_distribution<size_t> dist(0, roomList.size() - 1);

        return static_cast<u8>(roomList[dist(gen)]);
    }

    // UH OH, very bad!
    SPDLOG_ERROR("[Arcade]: UH OH! Returned 0 finding room!");
    return 0;
}

void Arcade_RequestEntranceOverride(s16 entrance) {
    if (IS_ARCADE_MODE) {
        sArcadeOverrideEntrance = entrance;
    }
}

void Arcade_ChooseRandomEntrance(PlayState* play) {
    int randomEntrance = Arcade_GetRandomEntrance();

    auto GetSceneFromEntrance = [](s16 entrance) -> s16 {
        // Convert grotto-special entrance IDs (0x0700+/0x0800+) to real entrance indices.
        // Grotto_GetEntranceValueHandlingGrottoRando will map grotto load/exit IDs to real entrance indices
        // when possible (it returns a concrete entrance index for grotto load/return entries).
        s16 resolved = Grotto_GetEntranceValueHandlingGrottoRando(entrance);

        // If the grotto helper returned the same special index (unmapped) then resolved may still be a special ID.
        // In that case, we can't map it to a scene here — return -1 to skip the same-scene check.
        if (resolved >= ENTRANCE_GROTTO_LOAD_START && resolved < ENTRANCE_GROTTO_EXIT_START) {
            return -1;
        }
        if (resolved >= ENTRANCE_GROTTO_EXIT_START && resolved < (ENTRANCE_GROTTO_EXIT_START + NUM_GROTTOS)) {
            return -1;
        }

        // resolved should now be a normal entrance index that indexes gEntranceTable.
        return gEntranceTable[resolved].scene;
    };

    s16 targetScene = GetSceneFromEntrance(static_cast<s16>(randomEntrance));
    s16 targetRoom = Arcade_GetSceneRoom(targetScene);

    // Set the targeted room.
    // Note: Entrance_Setup usually overwrites curRoom.num during transition,
    // which is why sArcadeOverrideRoom is exposed for your custom bounds/spawn logic.
    play->nextEntranceIndex = randomEntrance;
    sArcadeOverrideRoom = targetRoom;

    // Request a one-shot arcade-only override so the commit uses our chosen entrance.
    Arcade_RequestEntranceOverride(static_cast<s16>(randomEntrance));

    sArcadeTransitionsMade++;

    if (sArcadeTransitionsMade == 10) {
        if (play->linkAgeOnLoad == LINK_AGE_CHILD) {
            play->linkAgeOnLoad = LINK_AGE_ADULT;
        } else {
            play->linkAgeOnLoad = LINK_AGE_CHILD;
        }

        sArcadeTransitionsMade = 0;
    }

    SPDLOG_INFO("[Arcade]: Next scene: {}. Next room: {}.", play->nextEntranceIndex, sArcadeOverrideRoom);
    SPDLOG_INFO("[Arcade]: Total transitions made: {}.", sArcadeTransitionsMade);
}

void Arcade_UpdateLinkSpawn(s32 sceneNum, s8 curRoom) {
    modifiedLinkActorEntry.id = gPlayState->linkActorEntry->id;
    modifiedLinkActorEntry.pos = gPlayState->linkActorEntry->pos;
    modifiedLinkActorEntry.rot = gPlayState->linkActorEntry->rot;
    modifiedLinkActorEntry.params = gPlayState->linkActorEntry->params;

    static const std::unordered_map<s8, std::vector<ArcadeSpawn>> sDekuTreeSpawns = {
       // Room     posX   posY   posZ   rotY               Spawn Param    Requirement
        { 0x0, { { -4,    0,     554,   (s16)0x8000,       0x0DFF, },
                 { -390,  400,   389,   (s16)0x6062,       0x0DFF, },
                 { -468,  800,   -5,    (s16)0x4000,       0x0DFF, } } },

        { 0x1, { { -519,  400,   522,   (s16)0xe062,       0x0DFF, },
                 { -872,  400,   870,   (s16)0x605c,       0x0DFF, } } },

        { 0x2, { { -1005, 400,   997,   (s16)0xe05c,       0x0DFF, } } },

        { 0x3, { { -81,   -880,  488,   (s16)0x8000,       0x0DFF, } } },

        { 0x4, { { -75,   -880,  672,   (s16)0xff9a,       0x0DFF, },
                 { -243,  -880,  965,   (s16)0x4000,       0x0DFF, } } },

        { 0x5, { { -427,  -880,  967,   (s16)0xffffc000,   0x0DFF, },
                 { -1443  -760,  1070,  (s16)0x4000,       0x0DFF, } } },

        { 0x6, { { -1627, -760,  1071,  (s16)0xffffc000,   0x0DFF, },
                 { -1855, -760,  862,   (s16)0x0,   0x0DFF, } } },

        { 0x7, { { -1856, -760,  678,   (s16)0x8000,       0x0DFF, } } },

        { 0x8, { { -2486, -760,  -517,  (s16)0xffffa062,   0x0DFF, } } },

        { 0x9, { { -850,  -1880, -894,  (s16)0xfa5,        0x0DFF, } } },

        { 0xA, { { -652,  800,   1,     (s16)0xffffc000,   0x0DFF, } } },
    };

    static const std::unordered_map<s8, std::vector<ArcadeSpawn>> sDodongoCavernSpawns = {
       // Room     posX   posY   posZ   rotY               Param (0x0DFF is stationary)
        // Main Room
        { 0x0, { { -3,    0,     369,   (s16)0xffff8000,   0x0DFF },
                 { -752,  0,     -1521, (s16)0x4000,       0x0DFF },
                 { -1460, 531,   -320,  (s16)0x4000,       0x0DFF } } },
        
        // Right Scarecrow Room + Keese Pot Hallway
        { 0x1, { { 3560,  100,   -472,  (s16)0x0,   0x0DFF },
                 { 3560,  100,   -648,  (s16)0x8000,       0x0DFF },
                 { 3560,  100,   -1188, (s16)0x0,   0x0DFF } } },

        // Staircase Room
        { 0x2, { { -928,  0,     -1520, (s16)0xffffc000,   0x0DFF },
                 { -1960, 0,     -828,  (s16)0x8000,       0x0DFF },
                 { -1960, 531,   -820,  (s16)0x8000,       0x0DFF } } },

        // Lizalfos Battle Room
        { 0x3, { { 3560,  100,   -1364, (s16)0x8000,       0x0DFF },
                 { 2860,  100,   -2387, (s16)0x2af9,       0x0DFF },
                 { 4337,  531,   -688,  (s16)0x8000,       0x0DFF },
                 { 3868,  531,   -921,  (s16)0x4000,       0x0DFF } } },

        // Blue Dodongo Room
        { 0x4, { { 2710,  100,   -2474, (s16)0xffffaaf9,   0x0DFF },
                 { 1548,  80,    -922,  (s16)0x4000,       0x0DFF } } },

        // Pillar Switch Room
        { 0x5, { { -1960, 531,  -644,  (s16)0x0,   0x0DFF },
                 { -1635, 531,   -318,  (s16)0xffffc000,   0x0DFF } } },

        // Near Bombs Scrubs Room
        { 0x6, { { 1837,  411,   -1568, (s16)0x8000,       0x0DFF } } },

        // Push Block Boss Door Room
        { 0x7, { { -245,  16,    -2783, (s16)0x4000,       0x0DFF },
                 { -2,    76,    -2058, (s16)0x8000,       0x0DFF } } },

        // Fire Keese Back Room
        { 0x8, { { 1472,  16,    -3871, (s16)0x0,          0x0DFF } } },

        // Bomb Bag Chest Room
        { 0x9, { { 1840,  411,   -1230, (s16)0x0,   0x0DFF } } },

        // First Flame Pillar Room
        { 0xA, { { 4341,  531,   -512,  (s16)0x0,   0x0DFF } } },

        // Gold Skulltula Torch Room
        { 0xB, { { 2560,  0,     208,   (s16)0x0,   0x0DFF } } },

        // Second Flame Pillar Room
        { 0xC, { { 3692,  531,  -918,   (s16)0xffffc000,   0x0DFF } } },

        // Blue Dodongo Scrub Room
        { 0xD, { { 2430,  100,  -2980,  (s16)0x8000,       0x0DFF } } },

        // Back Gold Skulltula Room
        { 0xE, { { 1470,  76,   -4317,  (s16)0x8000,       0x0DFF } } },

        // Compass Chest Room
        { 0xF, { { -1958, 0,    -652,   (s16)0x0, 0x0DFF } } },
    };

    static const std::unordered_map<s8, std::vector<ArcadeSpawn>> sJabuJabuSpawns = {
        // Room     posX   posY   posZ   rotY               Param (0x0DFF is stationary)
        // Entrance
        { 0x0,
          { { -18, -244, 274, (s16)0xffff8000, 0x0DFF },
            { 0, -320, -1035, (s16)0x0, 0x0DFF } } },

        // Elevator Room
        { 0x1,
          { { 0, -320, -1218, (s16)0x8000, 0x0DFF },
            { 0, -340, -2351, (s16)0x0, 0x0DFF },
            { 528, -320, -1705, (s16)0xffffc000, 0x0DFF },
            { -528, 80, -1705, (s16)0x4000, 0x0DFF },
            { 221, -1233, -1700, (s16)0xffffc000, 0x0DFF },
          } },

        // Holes Room
        { 0x2,
          { { 0, -340, -2535, (s16)0x8000, 0x0DFF },
            { 0, -340, -3891, (s16)0x0, 0x0DFF },
          } },

        // Ruto Pickup Room
        { 0x3,
          { { -512, -1113, -2803, (s16)0x5cd5, 0x0DFF },
            { 368, -1113, -3225, (s16)0xffffc000, 0x0DFF },
            { -909, -1015, -3342, (s16)0x4000, 0x0DFF },
          } },

        // Lone Octo Room
        { 0x4,
          { { -1360, 80, -2196, (s16)0xfffffff9, 0x0DFF, REQ_BOOMERANG},
            { -712, 80, -1704, (s16)0xffffc000, 0x0DFF},
          } },

        // Boss Door Room
        { 0x5,
          { { 712, -320, -1702, (s16)0x4000, 0x0DFF },
            { 1364, -320, -1999, (s16)0x207, 0x0DFF },
          } },

        // Big Octo
        { 0x6,
          { { -1093, -1025, -3345, (s16)0xffffc000, 0x0DFF },
            { -1361, 80, -2376, (s16)0x8000, 0x0DFF },
          } },

        // Fork Room
        { 0x7,
          { { 0, -340, -4075, (s16)0x8000, 0x0DFF },
            { -968, -340, -4683, (s16)0x4000, 0x0DFF },
            { 0, -340, -5211, (s16)0x0, 0x0DFF },
            { 662, -340, -5091, (s16)0x0, 0x0DFF },
            { 968, -340, -4684, (s16)0xffffc000, 0x0DFF },
          } },

        // Middle Tentacle
        { 0x8, { { 0, -340, -5395, (s16)0x8000, 0x0DFF, REQ_BOOMERANG } } },

        // Stingray Room
        { 0x9, { { 1152, -340, -4684, (s16)0x4000, 0x0DFF } } },

        // Left Tentacle
        { 0xA, { { -1152, -340, -4682, (s16)0xffffc000, 0x0DFF } } },

        // Right Tentacle
        { 0xB, { { 663, -340, -5275, (s16)0x8000, 0x0DFF, REQ_BOOMERANG } } },

        // Bubble Room
        { 0xC, { { -659, -340, -5275, (s16)0x8000, 0x0DFF } } },

        // 5 Pot Room
        { 0xD, { { -650, -1113, -2682, (s16)0xffffdcd5, 0x0DFF } } },

        // Drown Ruto Room
        { 0xE,
          { { 552, -1114, -3225, (s16)0x4000, 0x0DFF },
            { 405, -1233, -1702, (s16)0x4000, 0x0DFF },
          } },
    };

    static const std::unordered_map<s8, std::vector<ArcadeSpawn>> sForestTempleSpawns = {
        // Room     posX   posY   posZ   rotY               Param (0x0DFF is stationary)
        // Entrance
        { 0x0, { { 110, 309, 778, (s16)0xffff8000, 0x0DFF },
                 { 120, 359, 140, (s16)0x0, 0x0DFF }
               } },

        // Entrance Skulltula
        { 0x1, { { 118, 359, -21, (s16)0xffff8000, 0x0DFF },
                 { 120, 383, -500, (s16)0x0, 0x0DFF } 
               } },

        // Lobby
        { 0x2,
          {
              { 118, 383, -661, (s16)0xffff8000, 0x0DFF },
              { 120, 467, -2436, (s16)0x0, 0x0DFF },
              { -510, 243, -2070, (s16)0x2000, 0x0DFF },
              { 847, 243, -2164, (s16)0xffffe000, 0x0DFF },
              { 919, 463, -1439, (s16)0xffffc000, 0x0DFF },
              { -681, 383, -1441, (s16)0x4000, 0x0DFF },
          } },

        // Amy Poe Hallway
        { 0x3,
          {
              { 1517, 463, -1440, (s16)0xffffc000, 0x0DFF },
              { 1080, 463, -1440, (s16)0x4000, 0x0DFF },
          } },

        // Blue Bubble Hallway
        { 0x4, { { 118, 467, -2597, (s16)0xffff8000, 0x0DFF },
                 { 118, 467, -2922, (s16)0xffff9a, 0x0DFF }
               } },

        // West Skulltula Hallway
        { 0x5, { { -1287, 383, -1441, (s16)0x4000, 0x0DFF },
                 { -842, 383, -1439, (s16)0xffffc000, 0x0DFF }
               } },

        // Stalfos Room
        { 0x6,
          {
              { 117, 467, -3098, (s16)0x8000, 0x0DFF },
              { -293, 827, -3322, (s16)0x4000, 0x0DFF },    // TODO: Set clear flag 7
              { 411, 827, -3318, (s16)0xffffc000, 0x0DFF }, // TODO: Set clear flag 7
          } },

        // East Courtyard
        { 0x7,
          {
              { 447, 800, -2808, (s16)0x4000, 0x0DFF },
              { 966, 243, -2293, (s16)0x6000, 0x0DFF },
              { 1664, 523, -2796, (s16)0x0, 0x0DFF },
          } },

        // West Courtyard
        { 0x8,
          {
              { -726, 243, -2284, (s16)0xffffa000, 0x0DFF },
              { -209, 800, -2809, (s16)0xffffc000, 0x0DFF },
              { -1642, 523, -2706, (s16)0x0, 0x0DFF },
              { -1657, 523, -2047, (s16)0x4000, 0x0DFF },
              { -1503, 523, -1944, (s16)0xffff8000, 0x0DFF },
          } },

        // Map Room
        { 0xA, { { -33, 800, -2812, (s16)0x4000, 0x0DFF },
                 { 271, 800, -2809, (s16)0xffffc000, 0x0DFF }
               } },

        // Push Block Room
        { 0xB,
          {
              { -1501, 523, -1783, (s16)0x0, 0x0DFF, REQ_STRENGTH },
              { -1448, 383, -1439, (s16)0xffffc000, 0x0DFF, REQ_STRENGTH },
              { -1761, 1228, -1732, (s16)0x0, 0x0DFF },
          } },

        // Joelle
        { 0xC, { { -1328, 1228, -3322, (s16)0x4000, 0x0DFF },
                 { -469, 827, -3318, (s16)0xffffc000, 0x0DFF }
               } },

        // Beth
        { 0xD, { { 587, 827, -3321, (s16)0x4000, 0x0DFF },
                 { 1566, 1228, -3319, (s16)0xffffc000, 0x0DFF }
               } },

        // Spinning Platforms Room
        { 0xE, { { 2001, 1228, -1732, (s16)0x0, 0x0DFF } } },

        // Falling Ceiling
        { 0xF, { { 1662, 523, -2957, (s16)0xffff8000, 0x0DFF },
                 { 1989, 403, -1902, (s16)0x8000, 0x0DFF }
               } },

        // Amy Poe
        { 0x10,
          {
              { 1994, 403, -1732, (s16)0x0, 0x0DFF },
              { 1693, 463, -1442, (s16)0x4000, 0x0DFF },
          } },

        // Basement
        { 0x11, { { 119, -779, -1665, (s16)0xffff8000, 0x0DFF } } },

        // Floormaster
        { 0x12, { { -1833, 523, -2042, (s16)0xffffc000, 0x0DFF } } },

        // Twisted Hallway 1
        { 0x13,
          {
              { -1762, 1236, -1908, (s16)0x8000, 0x0DFF },
              { -1504, 1228, -3320, (s16)0xffffc000, 0x0DFF },
          } },

        // Twisted Hallway 2
        { 0x14,
          {
              { 1742, 1228, -3321, (s16)0x4000, 0x0DFF },
              { 1999, 1228, -1908, (s16)0x8000, 0x0DFF },
          } },

        // Double Bubble
        { 0x15, { { -1646, 523, -2882, (s16)0x8000, 0x0DFF } } },
    };

    static const std::unordered_map<s8, std::vector<ArcadeSpawn>> sFireTempleSpawns = {
        // Room     posX   posY   posZ   rotY               Param (0x0DFF is stationary)
        // Entrance
        { 0x0,
          {
              { 5, 0, 980, (s16)0xffff8000, 0x0DFF },
              { 281, 0, 316, (s16)0x0, 0x0DFF },
              { -280, 0, 220, (s16)0x0, 0x0DFF },
              { -340, 200, -1, (s16)0x0, 0x0DFF },
              { 340, 200, 1, (s16)0xffffc000, 0x0DFF },
          } },

        // Big Lava Room
        { 0x1,
          {
              { 501, 200, -1, (s16)0x4000, 0x0DFF },
              { 1561, 100, -1480, (s16)0x0, 0x0DFF },
              { 1561, 240, -1458, (s16)0x0, 0x0DFF },
              { 2620, 200, 1, (s16)0xffffc000, 0x0DFF },
              { 1560, 200, 1404, (s16)0xffff8000, 0x0DFF },
          } },

        // Darunia Room
        { 0x2, { { -500, 200, 1, (s16)0xffffc000, 0x0DFF },
                 { -1478, 200, -5, (s16)0x4000, 0x0DFF }
               } },

        // BK Loop Flare Dancer
        { 0x3, { { -280, 0, -988, (s16)0xffff8000, 0x0DFF /* REQ */ },
                 { -88, 0, -1240, (s16)0xffffc000, 0x0DFF /* REQ */ }
               } },

        // Goron Shortcut
        { 0x4,
          {
              { 2990, 2060, 1, (s16)0xffffc000, 0x0DFF },
              { 2087, 2080, -628, (s16)0x0, 0x0DFF },
              { 2500, 2800, 1, (s16)0xffffc000, 0x0DFF },
          } },

        // Boulder Maze
        { 0x5,
          {
              { 2661, 2800, -1, (s16)0x4000, 0x0DFF },
              { 1781, 2800, -1420, (s16)0x0, 0x0DFF },
              { 1549, 2800, -410, (s16)0xffff8000, 0x0DFF },
              { 1540, 2940, -631, (s16)0x4000, 0x0DFF },
              { 1580, 4000, -670, (s16)0xffff8000, 0x0DFF },
          } },

        // Freestanding Hearts Room
        { 0x6,
          {
              { 1551, 2800, -250, (s16)0x0, 0x0DFF },
              { 1313, 2800, 60, (s16)0x4000, 0x0DFF },
              { 1579, 2800, 250, (s16)0xffff8000, 0x0DFF },
          } },

        // Hookshot Elevator GS
        { 0x7,
          {
              { 1581, 4000, -510, (s16)0x0, 0x0DFF },
              { 1579, 4400, -671, (s16)0xffff8000, 0x0DFF },
          } },

        // Gold Rupee Chest Room
        { 0x8, { { 1581, 4400, -510, (s16)0x0, 0x0DFF } } },

        // Flame Wall Maze
        { 0xA,
          {
              { -641, 2940, 192, (s16)0xffffc000, 0x0DFF },
              { -1779, 2800, 410, (s16)0x0, 0x0DFF },
              { -1367, 2980, 0, (s16)0x4000, 0x0DFF },
              { -1781, 2800, -411, (s16)0xffff8000, 0x0DFF, REQ_BOMBS },
              { -2514, 2840, -307, (s16)0x6000, 0x0DFF },
          } },

        // Highest Goron
        { 0xB, { { -1538, 2980, 2, (s16)0xffffc000, 0x0DFF, REQ_HAMMER } } },

        // Hammer Chest Room
        { 0xD, { { -2289, 4400, 19, (s16)0x4000, 0x0DFF } } },

        // Hammer Shortcut to Maze
        { 0xE, { { -2266, 4180, -822, (s16)0x6000, 0x0DFF, REQ_HAMMER } } },

        // BK Loop Torchslugs
        { 0xF, { { 240, 0, -788, (s16)0x0, 0x0DFF },
                 { 279, 0, 59, (s16)0xffff8000, 0x0DFF }
               } },

        // Flame Wall Chase Room
        { 0x10,
          {
              { 1581, 2800, 411, (s16)0x0, 0x0DFF },
              { 420, 2940, 189, (s16)0x4000, 0x0DFF },
              { 1380, 2940, -629, (s16)0xffffc000, 0x0DFF },
          } },

        // BK Chest
        { 0x11, { { -281, 0, 59, (s16)0xffff8000, 0x0DFF, REQ_NONE, TRICK_WEIRDSHOT },
                  { -280, 0, -812, (s16)0x0, 0x0DFF, REQ_HAMMER }
                } },

        // BK Loop GS
        { 0x12, { { 88, 0, -1242, (s16)0x4000, 0x0DFF },
                  { 238, 0, -964, (s16)0xffff8000, 0x0DFF }
                } },

        // SoT Block GS
        { 0x13, { { 1560, 240, -1601, (s16)0xffff8000, 0x0DFF } } },

        // Push Block Elevator
        { 0x15, { { 2781, 200, -1, (s16)0x4000, 0x0DFF },
                  { 3150, 2060, -1, (s16)0x4000, 0x0DFF }
                } },

        // Highest Flare Dancer
        { 0x18, { { -2731, 2840, -88, (s16)0xffffe000, 0x0DFF },
                  { -2778, 4000, 160, (s16)0xffffc000, 0x0DFF }
                } },
    };

    const std::vector<ArcadeSpawn>* availableSpawns = nullptr;

    if (sceneNum == SCENE_DEKU_TREE) {
        if (sDekuTreeSpawns.contains(curRoom)) {
            availableSpawns = &sDekuTreeSpawns.at(curRoom);
        }
    } else if (sceneNum == SCENE_DODONGOS_CAVERN) {
        if (sDodongoCavernSpawns.contains(curRoom)) {
            availableSpawns = &sDodongoCavernSpawns.at(curRoom);
        }
    } else if (sceneNum == SCENE_JABU_JABU) {
        if (sJabuJabuSpawns.contains(curRoom)) {
            availableSpawns = &sJabuJabuSpawns.at(curRoom);
        }
    } else if (sceneNum == SCENE_FOREST_TEMPLE) {
        if (sForestTempleSpawns.contains(curRoom)) {
            availableSpawns = &sForestTempleSpawns.at(curRoom);
        }
    } else if (sceneNum == SCENE_FIRE_TEMPLE) {
        if (sFireTempleSpawns.contains(curRoom)) {
            availableSpawns = &sFireTempleSpawns.at(curRoom);
        }
    }

    if (availableSpawns != nullptr && !availableSpawns->empty()) {
        s8 randomSpawnIndex = rand() % availableSpawns->size();
        const ArcadeSpawn& chosenSpawn = (*availableSpawns)[randomSpawnIndex];

        modifiedLinkActorEntry.pos.x = chosenSpawn.x;
        modifiedLinkActorEntry.pos.y = chosenSpawn.y;
        modifiedLinkActorEntry.pos.z = chosenSpawn.z;
        modifiedLinkActorEntry.rot.y = chosenSpawn.rotY;
        modifiedLinkActorEntry.params = chosenSpawn.params;

        gPlayState->linkActorEntry = &modifiedLinkActorEntry;
    }
}

extern "C" void Arcade_InitSave() {

    // Set player name to Lonk for the few textboxes that show up during Boss Rush. Player can't input their own name.
    std::array<char, 8> amPlayerName = { 21, 50, 49, 46, 62, 62, 62, 62 };
    for (int i = 0; i < ARRAY_COUNT(gSaveContext.playerName); i++) {
        gSaveContext.playerName[i] = amPlayerName[i];
    }

    sArcadeScore = -5; // Offset by -5 so that the player starts at 0 points
    sArcadeTransitionsMade = 0;
    gArcadeDamageMult = 1.0f;

    gSaveContext.ship.quest.id = QUEST_ARCADE;
    gSaveContext.ship.quest.data.arcade.isPaused = true;
    gSaveContext.entranceIndex = Arcade_GetRandomEntrance();
    gSaveContext.cutsceneIndex = 0x8000;
    gSaveContext.isMagicAcquired = 1;
    
    // Glitch Handling
    gSaveContext.ship.quest.data.arcade.canEquipSwap = false;
    gSaveContext.ship.quest.data.arcade.canISG = false;

    // Avoid awarding bonus on load
    gSaveContext.ship.quest.data.arcade.takenDamage = true;

    // Set magic
    if (gSaveContext.ship.quest.data.arcade.options[AM_OPTIONS_MAGIC] == AM_CHOICE_MAGIC_SINGLE) {
        gSaveContext.magicLevel = 1;
        gSaveContext.magic = 48;
    } else {
        gSaveContext.isDoubleMagicAcquired = 1;
        gSaveContext.magicLevel = 2;
        gSaveContext.magic = 96;
    }

    // Set health
    u16 health = 16;
    switch (gSaveContext.ship.quest.data.arcade.options[AM_OPTIONS_HEARTS]) {
        case AM_CHOICE_HEARTS_7:
            health *= 7;
            break;
        case AM_CHOICE_HEARTS_10:
            health *= 10;
            break;
        case AM_CHOICE_HEARTS_15:
            health *= 15;
            break;
        case AM_CHOICE_HEARTS_20:
            health *= 20;
            break;
        case AM_CHOICE_HEARTS_3:
            health *= 3;
            break;
        case AM_CHOICE_HEARTS_5:
            health *= 5;
            break;
        default:
            break;
    }

    gSaveContext.healthCapacity = health;
    gSaveContext.health = health;

    // Skip boss cutscenes
    gSaveContext.eventChkInf[7] |= 1;    // gohma
    gSaveContext.eventChkInf[7] |= 2;    // dodongo
    gSaveContext.eventChkInf[7] |= 4;    // phantom ganon
    gSaveContext.eventChkInf[7] |= 8;    // volvagia
    gSaveContext.eventChkInf[7] |= 0x10; // morpha
    gSaveContext.eventChkInf[7] |= 0x20; // twinrova
    gSaveContext.eventChkInf[7] |= 0x40; // barinade
    gSaveContext.eventChkInf[7] |= 0x80; // bongo bongo

    SET_EVENTCHKINF(EVENTCHKINF_ENTERED_DODONGOS_CAVERN); // Entered DC
    SET_INFTABLE(0x145); // Fight Big Octo

    // Sets all rando flags to false
    // Boss Rush currently uses 2 randomizer flags (RAND_INF_DUNGEONS_DONE_SPIRIT_TEMPLE &
    // RAND_INF_DUNGEONS_DONE_SHADOW_TEMPLE)
    for (s32 i = 0; i < ARRAY_COUNT(gSaveContext.ship.randomizerInf); i++) {
        gSaveContext.ship.randomizerInf[i] = 0;
    }

    // Set items
    std::array<u8, 24> amItems = {
        ITEM_STICK,       ITEM_NUT,      ITEM_BOMB, ITEM_BOW,  ITEM_NONE,      ITEM_NONE, ITEM_SLINGSHOT, ITEM_NONE,
        ITEM_NONE,        ITEM_HOOKSHOT, ITEM_NONE, ITEM_NONE, ITEM_BOOMERANG, ITEM_LENS, ITEM_NONE,      ITEM_HAMMER,
        ITEM_ARROW_LIGHT, ITEM_NONE,     ITEM_NONE, ITEM_NONE, ITEM_NONE,      ITEM_NONE, ITEM_NONE,      ITEM_NONE,
    };

    if (gSaveContext.ship.quest.data.arcade.options[AM_OPTIONS_LONGSHOT] == AM_CHOICE_LONGSHOT_YES) {
        amItems[9] = ITEM_LONGSHOT;
    }

    switch (gSaveContext.ship.quest.data.arcade.options[AM_OPTIONS_BOTTLE]) {
        case AM_CHOICE_BOTTLE_EMPTY:
            amItems[18] = ITEM_BOTTLE;
            break;
        case AM_CHOICE_BOTTLE_FAIRY:
            amItems[18] = ITEM_FAIRY;
            break;
        case AM_CHOICE_BOTTLE_REDPOTION:
            amItems[18] = ITEM_POTION_RED;
            break;
        case AM_CHOICE_BOTTLE_GREENPOTION:
            amItems[18] = ITEM_POTION_GREEN;
            break;
        case AM_CHOICE_BOTTLE_BLUEPOTION:
            amItems[18] = ITEM_POTION_BLUE;
            break;
        default:
            break;
    }

    if (gSaveContext.ship.quest.data.arcade.options[AM_OPTIONS_BUNNYHOOD] == AM_CHOICE_BUNNYHOOD_YES) {
        amItems[23] = ITEM_MASK_BUNNY;
    }

    for (int item = 0; item < ARRAY_COUNT(gSaveContext.inventory.items); item++) {
        gSaveContext.inventory.items[item] = amItems[item];
    }

    // Set consumable counts
    std::array<s8, 16> amAmmo = { 5, 5, 10, 10, 0, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    if (gSaveContext.ship.quest.data.arcade.options[AM_OPTIONS_AMMO] == AM_CHOICE_AMMO_FULL) {
        amAmmo = { 10, 20, 20, 30, 0, 0, 30, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    } else if (gSaveContext.ship.quest.data.arcade.options[AM_OPTIONS_AMMO] == AM_CHOICE_AMMO_MAXED) {
        amAmmo = { 30, 40, 40, 50, 0, 0, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    }

    for (int ammo = 0; ammo < ARRAY_COUNT(gSaveContext.inventory.ammo); ammo++) {
        gSaveContext.inventory.ammo[ammo] = amAmmo[ammo];
    }

    // Equipment
    gSaveContext.inventory.equipment |= 1 << 0; // Kokiri Sword
    gSaveContext.inventory.equipment |= 1 << 1; // Master Sword
    gSaveContext.inventory.equipment |= 1 << 4; // Deku Shield
    gSaveContext.inventory.equipment |= 1 << 6; // Mirror Shield
    gSaveContext.inventory.equipment |= 1 << 9; // Goron Tunic
    if (gSaveContext.ship.quest.data.arcade.options[AM_OPTIONS_BGS] == AM_CHOICE_BGS_YES) {
        gSaveContext.inventory.equipment |= 1 << 2; // Biggoron Sword
        gSaveContext.bgsFlag = 1;
    }
    if (gSaveContext.ship.quest.data.arcade.options[AM_OPTIONS_HOVERBOOTS] == AM_CHOICE_HOVERBOOTS_YES) {
        gSaveContext.inventory.equipment |= 1 << 14; // Hover Boots
    }

    // Upgrades
    u8 upgradeLevel = 1;
    if (gSaveContext.ship.quest.data.arcade.options[AM_OPTIONS_AMMO] == AM_CHOICE_AMMO_MAXED) {
        upgradeLevel = 3;
    }
    Inventory_ChangeUpgrade(UPG_QUIVER, upgradeLevel);
    Inventory_ChangeUpgrade(UPG_BOMB_BAG, upgradeLevel);
    Inventory_ChangeUpgrade(UPG_BULLET_BAG, upgradeLevel);
    Inventory_ChangeUpgrade(UPG_STICKS, upgradeLevel);
    Inventory_ChangeUpgrade(UPG_NUTS, upgradeLevel);
    Inventory_ChangeUpgrade(UPG_STRENGTH, 1);

    // Set flags and Link's age based on chosen settings.
    if (gSaveContext.ship.quest.data.arcade.options[AM_OPTIONS_BOSSES] == AM_CHOICE_BOSSES_ADULT ||
        gSaveContext.ship.quest.data.arcade.options[AM_OPTIONS_BOSSES] == AM_CHOICE_BOSSES_GANONDORF_GANON) {
        Flags_SetEventChkInf(EVENTCHKINF_USED_DEKU_TREE_BLUE_WARP);
        Flags_SetEventChkInf(EVENTCHKINF_USED_DODONGOS_CAVERN_BLUE_WARP);
        Flags_SetEventChkInf(EVENTCHKINF_USED_JABU_JABUS_BELLY_BLUE_WARP);
        if (gSaveContext.ship.quest.data.arcade.options[AM_OPTIONS_BOSSES] == AM_CHOICE_BOSSES_GANONDORF_GANON) {
            Flags_SetEventChkInf(EVENTCHKINF_USED_FOREST_TEMPLE_BLUE_WARP);
            Flags_SetEventChkInf(EVENTCHKINF_USED_FIRE_TEMPLE_BLUE_WARP);
            Flags_SetEventChkInf(EVENTCHKINF_USED_WATER_TEMPLE_BLUE_WARP);
            Flags_SetRandomizerInf(RAND_INF_DUNGEONS_DONE_SPIRIT_TEMPLE);
            Flags_SetRandomizerInf(RAND_INF_DUNGEONS_DONE_SHADOW_TEMPLE);
        }
        gSaveContext.linkAge = LINK_AGE_ADULT;
        Arcade_SetEquipment(LINK_AGE_ADULT);
    } else {
        gSaveContext.linkAge = LINK_AGE_CHILD;
        Arcade_SetEquipment(LINK_AGE_CHILD);
    }
}

static void* sSavePromptNoChoiceTexs[] = {
    (void*)gPauseNoENGTex,
    (void*)gPauseNoGERTex,
    (void*)gPauseNoFRATex,
};

void Arcade_OnVanillaBehaviorHandler(GIVanillaBehavior id, bool* should, va_list originalArgs) {
    va_list args;
    va_copy(args, originalArgs);

    switch (id) {
        // Allow not healing before ganon
        case VB_GANON_HEAL_BEFORE_FIGHT: {
            if (gSaveContext.ship.quest.data.arcade.options[AM_OPTIONS_HEAL] == AM_CHOICE_HEAL_NEVER) {
                *should = false;
            }
            break;
        }
        case VB_PLAY_BLUE_WARP_CS: {
            *should = false;
            break;
        }
        // Spawn clean blue warps (no ruto, adult animation, etc)
        case VB_SPAWN_BLUE_WARP: {
            switch (gPlayState->sceneNum) {
                case SCENE_DEKU_TREE_BOSS: {
                    BossGoma* bossGoma = va_arg(args, BossGoma*);
                    static Vec3f roomCenter = { -150.0f, 0.0f, -350.0f };
                    Vec3f childPos = roomCenter;

                    for (s32 i = 0; i < 10000; i++) {
                        if ((fabsf(childPos.x - GET_PLAYER(gPlayState)->actor.world.pos.x) < 100.0f &&
                             fabsf(childPos.z - GET_PLAYER(gPlayState)->actor.world.pos.z) < 100.0f) ||
                            (fabsf(childPos.x - bossGoma->actor.world.pos.x) < 150.0f &&
                             fabsf(childPos.z - bossGoma->actor.world.pos.z) < 150.0f)) {
                            childPos.x = Rand_CenteredFloat(400.0f) + -150.0f;
                            childPos.z = Rand_CenteredFloat(400.0f) + -350.0f;
                        } else {
                            break;
                        }
                    }
                    Actor_Spawn(&gPlayState->actorCtx, gPlayState, ACTOR_DOOR_WARP1, childPos.x,
                                bossGoma->actor.world.pos.y, childPos.z, 0, 0, 0, WARP_DUNGEON_ADULT);
                    break;
                }
                case SCENE_DODONGOS_CAVERN_BOSS: {
                    Actor_Spawn(&gPlayState->actorCtx, gPlayState, ACTOR_DOOR_WARP1, -890.0f, -1523.76f, -3304.0f, 0, 0,
                                0, WARP_DUNGEON_ADULT);
                    break;
                }
                case SCENE_JABU_JABU_BOSS: {
                    static Vec3f sWarpPos[] = {
                        { 10.0f, 0.0f, 30.0f },
                        { 260.0f, 0.0f, -470.0f },
                        { -240.0f, 0.0f, -470.0f },
                    };

                    s32 sp7C = 2;
                    for (s32 i = 2; i > 0; i -= 1) {
                        if (Math_Vec3f_DistXYZ(&sWarpPos[i], &GET_PLAYER(gPlayState)->actor.world.pos) <
                            Math_Vec3f_DistXYZ(&sWarpPos[i - 1], &GET_PLAYER(gPlayState)->actor.world.pos)) {
                            sp7C = i - 1;
                        }
                    }

                    Actor_Spawn(&gPlayState->actorCtx, gPlayState, ACTOR_DOOR_WARP1, sWarpPos[sp7C].x, sWarpPos[sp7C].y,
                                sWarpPos[sp7C].z, 0, 0, 0, WARP_DUNGEON_ADULT);
                    break;
                }
                case SCENE_FOREST_TEMPLE_BOSS: {
                    Actor_Spawn(&gPlayState->actorCtx, gPlayState, ACTOR_DOOR_WARP1, 14.0f, -33.0f, -3315.0f, 0, 0, 0,
                                WARP_DUNGEON_ADULT);
                    break;
                }
                case SCENE_FIRE_TEMPLE_BOSS: {
                    Actor_Spawn(&gPlayState->actorCtx, gPlayState, ACTOR_DOOR_WARP1, 0.0f, 100.0f, 0.0f, 0, 0, 0,
                                WARP_DUNGEON_ADULT);
                    break;
                }
                case SCENE_WATER_TEMPLE_BOSS: {
                    BossMo* bossMo = va_arg(args, BossMo*);
                    Actor_Spawn(&gPlayState->actorCtx, gPlayState, ACTOR_DOOR_WARP1, bossMo->actor.world.pos.x, -280.0f,
                                bossMo->actor.world.pos.z, 0, 0, 0, WARP_DUNGEON_ADULT);
                    break;
                }
                case SCENE_SPIRIT_TEMPLE_BOSS: {
                    Actor_Spawn(&gPlayState->actorCtx, gPlayState, ACTOR_DOOR_WARP1, 600.0f, 230.0f, 0.0f, 0, 0, 0,
                                WARP_DUNGEON_ADULT);
                    break;
                }
                case SCENE_SHADOW_TEMPLE_BOSS: {
                    Actor_Spawn(&gPlayState->actorCtx, gPlayState, ACTOR_DOOR_WARP1, -50.0f, 0.0f, 400.0f, 0, 0, 0,
                                WARP_DUNGEON_ADULT);
                    break;
                }
                default: {
                    SPDLOG_WARN("[Arcade]: Blue warp spawned in unhandled scene, ignoring");
                    return;
                }
            }
            *should = false;
            break;
        }
        // Skip past the "Save?" window when pressing B while paused and instead close the menu.
        case VB_CLOSE_PAUSE_MENU: {
            if (CHECK_BTN_ALL(gPlayState->state.input[0].press.button, BTN_B)) {
                *should = true;
            }
            break;
        }
        // Show "No" twice because the player can't continue.
        case VB_RENDER_YES_ON_CONTINUE_PROMPT: {
            Gfx** disp = va_arg(args, Gfx**);
            *disp = KaleidoScope_QuadTextureIA8(*disp, sSavePromptNoChoiceTexs[gSaveContext.language], 48, 16, 12);
            *should = false;
            break;
        }
        // Skip past the "Save?" window when dying and go to the "Continue?" screen immediately.
        case VB_TRANSITION_TO_SAVE_SCREEN_ON_DEATH: {
            PauseContext* pauseCtx = va_arg(args, PauseContext*);
            pauseCtx->state = 0xF;
            *should = false;
            break;
        }
        // Handle the heal on blue warp
        case VB_BLUE_WARP_CONSIDER_ADULT_IN_RANGE: {
            if (*should) {
                Arcade_HandleBlueWarpHeal(gPlayState);
            }
            break;
        }
        case VB_SHOW_GAMEPLAY_TIMER: {
            *should |= gSaveContext.ship.quest.data.arcade.options[AM_OPTIONS_TIMER] == AM_CHOICE_TIMER_YES;
            break;
        }
        // Prevent saving
        case VB_BE_ABLE_TO_SAVE:
        // There's no heart containers in boss rush
        case VB_SPAWN_HEART_CONTAINER:
        // Rupees are useless in boss rush
        case VB_RENDER_RUPEE_COUNTER: {
            *should = false;
            break;
        }
        case VB_RENDER_SCORE_DISPLAY: {
            *should = true;
            break;
        }
        // Prevent warning spam
        default: {
            break;
        }
    }

    va_end(args);
}

void Arcade_GlitchLogicHandling() {
    if (gPlayState == nullptr) {
        return;
    }

    // ISG handling
    Player* player = GET_PLAYER(gPlayState);
    if (player != nullptr) {
        bool isPaused = gPlayState->pauseCtx.state;
        bool swordActive = player->meleeWeaponState;
        static int32_t isgFrameCount = 0;

        if (gSaveContext.ship.quest.data.arcade.canISG == false) {
            if ((player->stateFlags3 == PLAYER_STATE3_MIDAIR) || !swordActive) {
                isgFrameCount = 0;
            } else if (!isPaused) {
                isgFrameCount++;
                if (isgFrameCount > 20) {
                    gSaveContext.ship.quest.data.arcade.canISG = true;
                }
            }
        }
    }

    // Equip Swap handling
    if (gSaveContext.ship.quest.data.arcade.canEquipSwap == false) {
        if (LINK_IS_ADULT) {
            for (int button = 1; button < ARRAY_COUNT(gSaveContext.equips.buttonItems); button++) {
                switch (gSaveContext.equips.buttonItems[button]) {
                    case ITEM_BOOMERANG:
                    case ITEM_STICK:
                    case ITEM_SLINGSHOT:
                    case ITEM_BEAN:
                        gSaveContext.ship.quest.data.arcade.canEquipSwap = true;
                        return;
                    default:
                        break;
                }
            }
        } else {
            for (int button = 1; button < ARRAY_COUNT(gSaveContext.equips.buttonItems); button++) {
                switch (gSaveContext.equips.buttonItems[button]) {
                    case ITEM_HAMMER:
                    case ITEM_HOOKSHOT:
                    case ITEM_LONGSHOT:
                    case ITEM_BOW:
                    case ITEM_ARROW_FIRE:
                    case ITEM_ARROW_ICE:
                    case ITEM_ARROW_LIGHT:
                        gSaveContext.ship.quest.data.arcade.canEquipSwap = true;
                        return;
                    default:
                        break;
                }
            }
        }
    }
}

bool Arcade_IsDungeonScene(int16_t sceneNum) {
    switch (sceneNum) {
        case SCENE_DEKU_TREE:
        case SCENE_DODONGOS_CAVERN:
        case SCENE_JABU_JABU:
        case SCENE_FOREST_TEMPLE:
        case SCENE_FIRE_TEMPLE:
        case SCENE_WATER_TEMPLE:
        case SCENE_SPIRIT_TEMPLE:
        case SCENE_SHADOW_TEMPLE:
        case SCENE_BOTTOM_OF_THE_WELL:
        case SCENE_ICE_CAVERN:
        case SCENE_GERUDO_TRAINING_GROUND:
        case SCENE_INSIDE_GANONS_CASTLE:
            return true;
        default:
            return false;
    }
}

void Arcade_OnSceneInitHandler(s16 sceneNum) {
    ActorContext* act = &gPlayState->actorCtx;
    bool canEquipSwap = gSaveContext.ship.quest.data.arcade.canEquipSwap;
    bool wasDamaged = gSaveContext.ship.quest.data.arcade.takenDamage;

    // Unpause the timer when the scene loaded isn't the Chamber of Sages.
    if (sceneNum != SCENE_CHAMBER_OF_THE_SAGES) {
        gSaveContext.ship.quest.data.arcade.isPaused = false;
    }

    //if (!canEquipSwap) Arcade_GlitchLogicHandling(gPlayState);
    Arcade_UpdateScore(5);
    
    if (!wasDamaged) {
        Arcade_UpdateScore(200);

        Notification::Emit({
            .prefix = "+200",
            .prefixColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f),
            .message = "score bonus for",
            .suffix = "No Damage Taken!",
            .suffixColor = ImVec4(0.0f, 0.5f, 1.0f, 1.0f),
        });
    }

    gSaveContext.ship.quest.data.arcade.takenDamage = false;

    // Unset all scene flags and saved flags
    if (gPlayState != nullptr) {
        ActorContext* act = &gPlayState->actorCtx;
        if (gPlayState->sceneNum == SCENE_FOREST_TEMPLE) {
            act->flags.swch = (1 << 0x1B); // Prevent the sunken elevator from resetting
        } else {
            act->flags.swch = 0;
        }
        act->flags.clear = 0;
        gSaveContext.sceneFlags[gPlayState->sceneNum].swch = act->flags.swch;
        gSaveContext.sceneFlags[gPlayState->sceneNum].clear = act->flags.clear;
    }

    if (gSaveContext.respawnFlag <= 0 && sArcadeOverrideRoom != -1) {
        int8_t origRoom = gPlayState->roomCtx.curRoom.num;

        if (origRoom != sArcadeOverrideRoom) {
            // Reset room ctx back to prev room and then load the new room
            gPlayState->roomCtx.status = 0;
            gPlayState->roomCtx.curRoom = gPlayState->roomCtx.prevRoom;
            func_8009728C(gPlayState, &gPlayState->roomCtx, sArcadeOverrideRoom);
        }
    }

    int8_t currentRoom = (sArcadeOverrideRoom != -1) ? sArcadeOverrideRoom : gPlayState->roomCtx.curRoom.num;
    Arcade_UpdateLinkSpawn(sceneNum, currentRoom);

    if (gPlayState->sceneNum == SCENE_FOREST_TEMPLE && gPlayState->roomCtx.curRoom.num == 0x6 && gPlayState->linkActorEntry->pos.y >= 827) {
        gSaveContext.sceneFlags[gPlayState->sceneNum].clear = (1 << 0x6); // Fight F2 Stalfos
    } else if (gPlayState->sceneNum == SCENE_FOREST_TEMPLE && gPlayState->roomCtx.curRoom.num == 0x11) {
        gSaveContext.sceneFlags[gPlayState->sceneNum].swch &= ~(1 << 0x1B); // Enable elevator in the basement
    }

    Arcade_ChooseRandomEntrance(gPlayState);

    sArcadeLockClosestDoor = true;
}

void Arcade_OnBossDefeatHandler(void* refActor) {
    Arcade_HandleCompleteBoss(gPlayState);
}

void Arcade_OnEnemyDefeatedHandler(void* actorPtr) {
    Actor* actor = static_cast<Actor*>(actorPtr);
    Arcade_HandleEnemyDefeated(actor);
}

void Arcade_OnWonderCollectHandler(void* actorPtr) {
    Actor* actor = static_cast<Actor*>(actorPtr);
    Arcade_HandleWonderCollect(actor);
}

void Arcade_OnBlueWarpUpdate(void* actor) {
    DoorWarp1* blueWarp = static_cast<DoorWarp1*>(actor);

    if (blueWarp->warpTimer > 160) {
        Arcade_HandleBlueWarp(gPlayState, blueWarp->actor.world.pos.x, blueWarp->actor.world.pos.z);
    }
}

void Arcade_TransitionInterceptor() {
    if (sArcadeOverrideEntrance == -1 || gPlayState == nullptr || !Arcade_IsDungeonScene(gPlayState->sceneNum)) {
        return;
    }

    bool isTransitioning = false;

    // Catch Native Room Transitions (e.g., walking through an unlocked Jabu door or hallway)
    if (gPlayState->roomCtx.status == 1) {
        gPlayState->roomCtx.status = 0; // Abort the native room transition
        isTransitioning = true;
    }
    // Catch Native Scene Transitions (e.g., doors that go between distinct scenes)
    else if (gPlayState->transitionTrigger == TRANS_TRIGGER_START) {
        // Ensure we don't accidentally override the Chamber of Sages/Credits warp
        if (gPlayState->nextEntranceIndex != ENTR_CHAMBER_OF_THE_SAGES_0 && gPlayState->nextEntranceIndex != 0xFFF2) {
            isTransitioning = true;
        }
    }

    if (isTransitioning) {
        // Force the Arcade scene transition
        gPlayState->nextEntranceIndex = sArcadeOverrideEntrance;
        gSaveContext.nextCutsceneIndex = 0x8000;
        gPlayState->transitionTrigger = TRANS_TRIGGER_START;
        gPlayState->transitionType = TRANS_TYPE_FADE_BLACK_FAST;

        SPDLOG_INFO("[Arcade] Transition intercepted. Forcing scene warp to {}", sArcadeOverrideEntrance);

        // CONSUME the override so we don't trap the player in an infinite loop
        sArcadeOverrideEntrance = -1;
    }
}

void Arcade_EnHoll_Draw(Actor* thisx, PlayState* play) {
    EnHoll* enHoll = reinterpret_cast<EnHoll*>(thisx);
    u8 originalSide = enHoll->side;

    // Extract the action type index exactly as EnHoll_ChooseAction does
    s32 action = (thisx->params >> 6) & 7;

    // Push the matrix to protect the global state
    Matrix_Push();

    // If the action type is a hole transition (1, 2, or 5), pitch it 90 degrees
    // so the visual plane lies flat on the floor or ceiling.
    if (action == 1 || action == 2 || action == 5) {
        Matrix_RotateX(M_PI / 2.0f, MTXMODE_APPLY);
        Matrix_Scale(1.5f, 1.0f, 1.5f, MTXMODE_APPLY);
    }

    // Draw both sides of actor
    enHoll->side = 1;
    EnHoll_Draw(thisx, play);
    enHoll->side = 0;
    EnHoll_Draw(thisx, play);

    // Pop the matrix to erase both our X-rotation and the Y-rotation
    // caused by the second EnHoll_Draw call.
    Matrix_Pop();

    enHoll->side = originalSide;
}

void Arcade_HandleDoors(void* actor) {
    Actor* doorActor = static_cast<Actor*>(actor);
    Player* player = GET_PLAYER(gPlayState);
    int32_t numDoors = gPlayState->actorCtx.actorLists[ACTORCAT_DOOR].length;
    int32_t numEnemies = gPlayState->actorCtx.actorLists[ACTORCAT_ENEMY].length;
    Actor* enemyName = gPlayState->actorCtx.actorLists[ACTORCAT_ENEMY].head;
    bool canClear = true;
    bool flippedDoor = false;
    int32_t curFrames = gPlayState->state.frames;
    int32_t doorLockoutTimer = 20;

    // Unkillable "enemies" that would prevent SHUTTER_FRONT_CLEAR from working
    if (enemyName != nullptr) {
        switch (enemyName->id) {
            case ACTOR_EN_BROB:
                canClear = false;
                break;
            default:
                break;
        }
    }

    if (gPlayState->sceneNum == SCENE_JABU_JABU) {
        switch (gPlayState->roomCtx.curRoom.num) {
            case 0x9:
                flippedDoor = true;
                break;
            default:
                break;
        }
    }

    if (doorActor->id == ACTOR_DOOR_SHUTTER) {
        DoorShutter* shutterDoor = static_cast<DoorShutter*>(actor);

        if (shutterDoor->doorType == SHUTTER_KEY_LOCKED) {
            shutterDoor->unlockTimer = 0;
        }

        if (shutterDoor->dyna.actor.xyzDistToPlayerSq <= SQ(200.0f) && numDoors > 1 && sArcadeLockClosestDoor) {
            shutterDoor->doorType = SHUTTER_FRONT_SWITCH_BACK_CLEAR;
            DoorShutter_SetupAction(shutterDoor, DoorShutter_SetupType);

            SPDLOG_INFO("[Arcade]: Locked door behind player dynamically at distance squared {}",
                        doorActor->xyzDistToPlayerSq);
            sArcadeLockClosestDoor = false;
        } else if (numDoors == 1 && numEnemies >= 1 && sArcadeLockClosestDoor && canClear) {
            if (flippedDoor) {
                shutterDoor->doorType = SHUTTER_FRONT_SWITCH_BACK_CLEAR;
                DoorShutter_SetupAction(shutterDoor, DoorShutter_SetupType);

                SPDLOG_INFO("[Arcade] Flipped Door: 'Back' set to clear");
                sArcadeLockClosestDoor = false;
            } else {
                shutterDoor->doorType = SHUTTER_FRONT_CLEAR;
                DoorShutter_SetupAction(shutterDoor, DoorShutter_SetupType);

                SPDLOG_INFO("[Arcade]: 1 door in room with enemies, set door to open on clear");
                sArcadeLockClosestDoor = false;
            }
        }
    } else if (doorActor->id == ACTOR_EN_HOLL) {
        EnHoll* enHoll = static_cast<EnHoll*>(actor);
        enHoll->planeAlpha = 255;
        enHoll->actor.draw = (ActorFunc)Arcade_EnHoll_Draw;

        f32 posX = enHoll->actor.world.pos.x;
        f32 posY = enHoll->actor.world.pos.y;
        f32 posZ = enHoll->actor.world.pos.z;
        int16_t rotX = enHoll->actor.shape.rot.x;
        int16_t rotY = enHoll->actor.shape.rot.y;
        int16_t rotZ = enHoll->actor.shape.rot.z;

        if (enHoll->actor.xyzDistToPlayerSq <= SQ(200.0f) && numDoors > 1 && sArcadeLockClosestDoor) {
            // Replace En_Holl load planes with gold gaunts block to force the player to go through the room
            Actor_Delete(&gPlayState->actorCtx, doorActor, gPlayState);
            BgHeavyBlock* heavyBlock = (BgHeavyBlock*)Actor_Spawn(&gPlayState->actorCtx, gPlayState, ACTOR_BG_HEAVY_BLOCK, posX, posY, posZ, rotX, rotY, rotZ, 0);
            if (heavyBlock == NULL) {
                return;
            } else {
                heavyBlock->dyna.actor.scale.x = 2.0f;
                heavyBlock->dyna.actor.scale.y = 0.5f;
            }

            SPDLOG_INFO("[Arcade]: Successfully spawned Heavy Block behind player at {}, {}, {}, rotation: {}", posX, posY, posZ, rotY);
            sArcadeLockClosestDoor = false;
        }
    } else if (doorActor->id == ACTOR_EN_DOOR) {
        EnDoor* enDoor = static_cast<EnDoor*>(actor);
        enDoor->lockTimer = 0;

        f32 posX = enDoor->actor.world.pos.x;
        f32 posY = enDoor->actor.world.pos.y;
        f32 posZ = enDoor->actor.world.pos.z;
        int16_t rotX = enDoor->actor.shape.rot.x;
        int16_t rotY = enDoor->actor.shape.rot.y;
        int16_t rotZ = enDoor->actor.shape.rot.z;

        if (enDoor->actor.xyzDistToPlayerSq <= SQ(200.0f) && numDoors > 1 && sArcadeLockClosestDoor) {
            int16_t yawDiff = (int16_t)(rotY - enDoor->actor.yawTowardsPlayer);
            if (yawDiff > 0x4000 || yawDiff < -0x4000) {
                rotY += 0x8000;
            }
            uint16_t shutterParams = enDoor->actor.params & 0xFC3F;

            Actor_Delete(&gPlayState->actorCtx, doorActor, gPlayState);
            DoorShutter* replaceShutter = (DoorShutter*)Actor_Spawn(&gPlayState->actorCtx, gPlayState, ACTOR_DOOR_SHUTTER, posX, posY, posZ, rotX, rotY, rotZ, 0);

            replaceShutter->doorType = SHUTTER_FRONT_SWITCH_BACK_CLEAR;

            SPDLOG_INFO("[Arcade]: Successfully locked door behind player at {}, {}, {}, rotation: {}", posX, posY, posZ, rotY);
            sArcadeLockClosestDoor = false;
        }
    } else if ((curFrames - doorLockoutTimer) > 0 && sArcadeLockClosestDoor) { // if no door found within a 20 game frames, then stop searching for door
        sArcadeLockClosestDoor = false;
        SPDLOG_INFO("[Arcade]: No door found to lock within 20 frames, aborting");
    }

    if (sArcadeLockClosestDoor) {
        SPDLOG_INFO("[Arcade]: Number of Doors {}", numDoors);
    }
}

void Arcade_LogDoorPos(void* actor) {
    Actor* doorPos = static_cast<Actor*>(actor);
    Player* player = GET_PLAYER(gPlayState);

    int32_t doorYaw = std::abs(doorPos->yawTowardsPlayer - doorPos->world.rot.y);
    int32_t playerYaw = std::abs(player->yaw);
    int32_t yawDifference = doorYaw - playerYaw;
    SPDLOG_INFO("Door Pos: {}, {}, {}", doorPos->world.pos.x, doorPos->world.pos.y,
                doorPos->world.pos.z);
    SPDLOG_INFO("Door YAW: {}, Player YAW: {}", doorYaw, playerYaw);
    SPDLOG_INFO("YAW Difference: {}", yawDifference);
}

void RegisterArcadeMode() {
    COND_HOOK(OnLoadGame, true, [](int32_t fileNum) {
        COND_ID_HOOK(OnActorInit, ACTOR_DEMO_SA, IS_ARCADE_MODE, [](void* actorPtr) {
            Arcade_SpawnBlueWarps(gPlayState);
            Actor_Kill((Actor*)actorPtr);
            GET_PLAYER(gPlayState)->actor.world.rot.y = 27306;
            GET_PLAYER(gPlayState)->actor.shape.rot.y = 27306;
        });

        // Remove bushes, used in Gohma's arena
        // COND_ID_HOOK(OnActorInit, ACTOR_EN_KUSA, IS_ARCADE_MODE, [](void* actorPtr) { Actor_Kill((Actor*)actorPtr); });

        // Remove pots, used in Barinade's and Ganondorf's arenas
        // COND_ID_HOOK(OnActorInit, ACTOR_OBJ_TSUBO, IS_ARCADE_MODE, [](void* actorPtr) { Actor_Kill((Actor*)actorPtr); });

        // Remove chests, mainly for the chest in King Dodongo's boss room
        // COND_ID_HOOK(OnActorInit, ACTOR_EN_BOX, IS_ARCADE_MODE, [](void* actorPtr) { Actor_Kill((Actor*)actorPtr); });

        COND_HOOK(OnVanillaBehavior, IS_ARCADE_MODE, Arcade_OnVanillaBehaviorHandler);

        COND_HOOK(OnSceneInit, IS_ARCADE_MODE, Arcade_OnSceneInitHandler);

        COND_HOOK(OnBossDefeat, IS_ARCADE_MODE, Arcade_OnBossDefeatHandler);

        COND_ID_HOOK(OnActorUpdate, ACTOR_DOOR_WARP1, IS_ARCADE_MODE, Arcade_OnBlueWarpUpdate);

        COND_HOOK(OnEnemyDefeat, IS_ARCADE_MODE, Arcade_OnEnemyDefeatedHandler);

        COND_HOOK(OnActorUpdate, IS_ARCADE_MODE, Arcade_HandleDoors);
        COND_HOOK(OnActorUpdate, IS_ARCADE_MODE, Arcade_UpdateEnemySpeed);
        COND_HOOK(OnGameFrameUpdate, IS_ARCADE_MODE, Arcade_TransitionInterceptor);
        COND_HOOK(OnGameFrameUpdate, IS_ARCADE_MODE, Arcade_GlitchLogicHandling);
        COND_HOOK(OnGameFrameUpdate, IS_ARCADE_MODE, Arcade_UpdateEnemyScaling);

        COND_ID_HOOK(OnActorDestroy, ACTOR_EN_WONDER_ITEM, IS_ARCADE_MODE, Arcade_OnWonderCollectHandler);

        COND_ID_HOOK(OnActorInit, ACTOR_DOOR_SHUTTER, IS_ARCADE_MODE, Arcade_LogDoorPos);

        COND_HOOK(OnPlayerHealthChange, IS_ARCADE_MODE, Arcade_HandleNoDamage);
    });
}

void Arcade_DrawCurrentScore(PlayState* play) {
    // Draw timer based on the Gameplay Stats total time.
    if (IS_ARCADE_MODE && VB_RENDER_SCORE_DISPLAY) {
        s32 X_Margins_Timer = 0;
        if (CVarGetInteger(CVAR_COSMETIC("HUD.Score.UseMargins"), 0) != 0) {
            if (CVarGetInteger(CVAR_COSMETIC("HUD.Score.PosType"), 0) == ORIGINAL_LOCATION) {
                X_Margins_Timer = Left_HUD_Margin;
            };
        }
        s32 rectLeftOri = OTRGetRectDimensionFromLeftEdge(24 + X_Margins_Timer);
        s32 rectTopOri = 73;
        if (CVarGetInteger(CVAR_COSMETIC("HUD.Score.PosType"), 0) != ORIGINAL_LOCATION) {
            rectTopOri = (CVarGetInteger(CVAR_COSMETIC("HUD.Score.PosY"), 0));
            if (CVarGetInteger(CVAR_COSMETIC("HUD.Score.PosType"), 0) == ANCHOR_LEFT) {
                if (CVarGetInteger(CVAR_COSMETIC("HUD.Score.UseMargins"), 0) != 0) {
                    X_Margins_Timer = Left_HUD_Margin;
                };
                rectLeftOri =
                    OTRGetRectDimensionFromLeftEdge(CVarGetInteger(CVAR_COSMETIC("HUD.Score.PosX"), 0) + X_Margins_Timer);
            } else if (CVarGetInteger(CVAR_COSMETIC("HUD.Score.PosType"), 0) == ANCHOR_RIGHT) {
                if (CVarGetInteger(CVAR_COSMETIC("HUD.Score.UseMargins"), 0) != 0) {
                    X_Margins_Timer = Right_HUD_Margin;
                };
                rectLeftOri = OTRGetRectDimensionFromRightEdge(CVarGetInteger(CVAR_COSMETIC("HUD.Score.PosX"), 0) +
                                                               X_Margins_Timer);
            } else if (CVarGetInteger(CVAR_COSMETIC("HUD.Score.PosType"), 0) == ANCHOR_NONE) {
                rectLeftOri = CVarGetInteger(CVAR_COSMETIC("HUD.Score.PosX"), 0) + 204 + X_Margins_Timer;
            } else if (CVarGetInteger(CVAR_COSMETIC("HUD.Score.PosType"), 0) == HIDDEN) {
                rectLeftOri = -9999;
            }
        }

        s32 rectLeft;
        s32 rectTop;
        s32 rectWidth = 8;
        s32 rectHeightOri = 16;
        s32 rectHeight;

        OPEN_DISPS(play->state.gfxCtx);

        gDPSetCombineLERP(OVERLAY_DISP++, 0, 0, 0, PRIMITIVE, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, PRIMITIVE, TEXEL0, 0,
                          PRIMITIVE, 0);

        gDPSetOtherMode(OVERLAY_DISP++,
                        G_AD_DISABLE | G_CD_DISABLE | G_CK_NONE | G_TC_FILT | G_TF_BILERP | G_TT_IA16 | G_TL_TILE |
                            G_TD_CLAMP | G_TP_NONE | G_CYC_1CYCLE | G_PM_NPRIMITIVE,
                        G_AC_NONE | G_ZS_PRIM | G_RM_XLU_SURF | G_RM_XLU_SURF2);

        std::string scoreStr = std::to_string(sArcadeScore);
        const char* totalScoreText = scoreStr.c_str();
        size_t textLength = strlen(totalScoreText);
        uint16_t textureIndex = 0;

        for (size_t i = 0; i < textLength; i++) {
            if (totalScoreText[i] == ':' || totalScoreText[i] == '.') {
                textureIndex = 10;
            } else {
                textureIndex = totalScoreText[i] - 48;
            }

            rectLeft = rectLeftOri + (i * 8);
            rectTop = rectTopOri;
            rectHeight = rectHeightOri;

            // Load correct digit (or : symbol)
            gDPLoadTextureBlock(OVERLAY_DISP++, ((u8*)digitTextures[textureIndex]), G_IM_FMT_I, G_IM_SIZ_8b, rectWidth,
                                rectHeight, 0, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK,
                                G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);

            // Create dot image from the colon image.
            if (totalScoreText[i] == '.') {
                rectHeight = rectHeight / 2;
                rectTop += 5;
                rectLeft -= 1;
            }

            // Draw text shadow
            gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 0, 0, 0, 255);
            gDPSetEnvColor(OVERLAY_DISP++, 255, 255, 255, 255);
            gSPWideTextureRectangle(OVERLAY_DISP++, rectLeft << 2, rectTop << 2, (rectLeft + rectWidth) << 2,
                                    (rectTop + rectHeight) << 2, G_TX_RENDERTILE, 0, 0, 1 << 10, 1 << 10);

            // Draw regular text. Change color based on if the timer is paused, running or the game is completed.
            if (gSaveContext.ship.stats.gameComplete) {
                gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 120, 255, 0, 255);
            } else if (IS_ARCADE_MODE && gSaveContext.ship.quest.data.arcade.isPaused &&
                       !gSaveContext.ship.stats.rtaTiming) {
                gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 150, 150, 150, 255);
            } else {
                gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 255, 255, 255);
            }

            // Offset text so underlaying shadow is to the bottom right of the text.
            rectLeft -= 1;
            rectTop -= 1;

            gSPWideTextureRectangle(OVERLAY_DISP++, rectLeft << 2, rectTop << 2, (rectLeft + rectWidth) << 2,
                                    (rectTop + rectHeight) << 2, G_TX_RENDERTILE, 0, 0, 1 << 10, 1 << 10);
        }
        // free(totalScoreText);

        CLOSE_DISPS(play->state.gfxCtx);
    }
}

static RegisterShipInitFunc initFunc(RegisterArcadeMode);