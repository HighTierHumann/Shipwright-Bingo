#include "soh/OTRGlobals.h"
#include "soh/ResourceManagerHelpers.h"
#include "soh/Enhancements/enhancementTypes.h"
#include "soh/Enhancements/custom-message/CustomMessageTypes.h"
#include "soh/Enhancements/randomizer/randomizerTypes.h"
#include "soh/Enhancements/randomizer/dungeon.h"
#include "soh/Enhancements/randomizer/static_data.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/SohGui/ImGuiUtils.h"
#include "soh/Notification/Notification.h"
#include "soh/SaveManager.h"
#include "soh/ShipInit.hpp"
#include "soh/ObjectExtension/ObjectExtension.h"
#include "soh/Enhancements/randomizer/randomizer.h"
#include "soh/Enhancements/randomizer/randomizer_check_tracker.h"
#include "soh/Enhancements/randomizer/RCToRandInf.h"
#include "soh/Bingo/BingoSquareMap.h"

#include <algorithm>
#include <bitset>

extern "C" {
#include "macros.h"
#include "functions.h"
#include "variables.h"
#include "src/overlays/actors/ovl_En_Dodongo/z_en_dodongo.h"
#include "src/overlays/actors/ovl_En_Firefly/z_en_firefly.h"
#include "src/overlays/actors/ovl_En_Tubo_Trap/z_en_tubo_trap.h"
#include "src/overlays/actors/ovl_En_Wallmas/z_en_wallmas.h"
#include "src/overlays/actors/ovl_En_Ru1/z_en_ru1.h"
#include "src/overlays/actors/ovl_Obj_Switch/z_obj_switch.h"
extern PlayState* gPlayState;
extern SaveContext gSaveContext;
}

static std::queue<RandomizerCheck> randomizerQueuedChecks;
static RandomizerCheck randomizerQueuedCheck = RC_UNKNOWN_CHECK;
static GetItemEntry randomizerQueuedItemEntry = GET_ITEM_NONE;
static std::vector<int16_t> previousDungeonScenes;

constexpr SceneID dungeonScenes[] = {
    SCENE_DEKU_TREE,
    SCENE_DODONGOS_CAVERN,
    SCENE_JABU_JABU,
    SCENE_FOREST_TEMPLE,
    SCENE_FIRE_TEMPLE,
    SCENE_WATER_TEMPLE,
    SCENE_SPIRIT_TEMPLE,
    SCENE_SHADOW_TEMPLE,
    SCENE_INSIDE_GANONS_CASTLE,
    SCENE_GANONS_TOWER,
    SCENE_ICE_CAVERN,
    SCENE_GERUDO_TRAINING_GROUND,
    SCENE_BOTTOM_OF_THE_WELL,
};

constexpr SceneID mainDungeonScenes[] = {
    SCENE_DEKU_TREE,
    SCENE_DODONGOS_CAVERN,
    SCENE_JABU_JABU,
    SCENE_FOREST_TEMPLE,
    SCENE_FIRE_TEMPLE,
    SCENE_WATER_TEMPLE,
    SCENE_SPIRIT_TEMPLE,
    SCENE_SHADOW_TEMPLE,
    SCENE_INSIDE_GANONS_CASTLE,
    SCENE_GANONS_TOWER,
};

constexpr SceneID miniDungeonScenes[] = {
    SCENE_ICE_CAVERN,
    SCENE_GERUDO_TRAINING_GROUND,
    SCENE_BOTTOM_OF_THE_WELL,
};

constexpr SceneID bossScenes[] = {
    SCENE_DEKU_TREE_BOSS,
    SCENE_DODONGOS_CAVERN_BOSS,
    SCENE_JABU_JABU_BOSS,
    SCENE_FOREST_TEMPLE_BOSS,
    SCENE_FIRE_TEMPLE_BOSS,
    SCENE_WATER_TEMPLE_BOSS,
    SCENE_SPIRIT_TEMPLE_BOSS,
    SCENE_SHADOW_TEMPLE_BOSS,
    SCENE_GANONDORF_BOSS,
};

bool IsDungeonScene(int16_t scene) {
    for (SceneID dungeonScene : dungeonScenes) {
        if (scene == dungeonScene) {
            return true;
        }
    }
    return false;
}

bool IsMainDungeonScene(int16_t scene) {
    for (SceneID mainDungeonScene : mainDungeonScenes) {
        if (scene == mainDungeonScene) {
            return true;
        }
    }
    return false;
}

bool IsMiniDungeonScene(int16_t scene) {
    for (SceneID miniDungeonScene : miniDungeonScenes) {
        if (scene == miniDungeonScene) {
            return true;
        }
    }
    return false;
}

bool IsBossScene(int16_t scene) {
    for (SceneID bossScene : bossScenes) {
        if (scene == bossScene) {
            return true;
        }
    }
    return false;
}

std::ptrdiff_t CountFreedGorons() {
    constexpr int goronCageSwitchFlags[] = { 0x1, 0x2, 0x3, 0x4, 0x5, 0x8, 0x9, 0xA, 0xB };
    return std::count_if(std::begin(goronCageSwitchFlags), std::end(goronCageSwitchFlags),
                         [](int flag) { return Flags_GetSwitch(gPlayState, flag) != 0; });
}

static uint64_t GetEyeSwitchMask(int16_t sceneNum) {
    switch (sceneNum) {
        case SCENE_DEKU_TREE:
            return 1ULL << 0x4;
        case SCENE_DODONGOS_CAVERN:
            return (1ULL << 0x38) | (1ULL << 0x39) | (1ULL << 0x3A);
        case SCENE_FOREST_TEMPLE:
            return (1ULL << 0x25) | (1ULL << 0xF) | (1ULL << 0x20) | (1ULL << 0x21);
        case SCENE_FIRE_TEMPLE:
            return 1ULL << 0x3C;
        case SCENE_WATER_TEMPLE:
            return (1ULL << 0x13) | (1ULL << 0x3A);
        case SCENE_GERUDO_TRAINING_GROUND:
            return 1ULL << 0x1A;
        default:
            return 0;
    }
}

static void BingoRecordEyeSwitch(int16_t sceneNum, int16_t flag) {
    if (flag < 0 || flag > 0x3F || !(GetEyeSwitchMask(sceneNum) & (1ULL << flag))) {
        return;
    }

    uint64_t& scenesBits = gSaveContext.ship.bingo.eyeSwitchesActivated[sceneNum];
    if (scenesBits & (1ULL << flag)) {
        return;
    }
    scenesBits |= (1ULL << flag);

    int totalSwitches = 0;
    int scenesWithSwitch = 0;
    for (int i = 0; i < BINGO_SCENE_INFO_COUNT; i++) {
        uint64_t bits = gSaveContext.ship.bingo.eyeSwitchesActivated[i];
        if (bits) {
            scenesWithSwitch++;
            totalSwitches += static_cast<int>(std::bitset<64>(bits).count());
        }
    }

    if (totalSwitches >= 5) {
        Flags_SetBingoInf(BINGO_INF_5_DIFFERENT_EYE_SWITCHES);
    }
    if (scenesWithSwitch >= 4) {
        Flags_SetBingoInf(BINGO_INF_4_EYE_SWITCHES_IN_DIFFERENT_DUNGEONS);
    }
}

void BingoOnFlagSetHandler(int16_t flagType, int16_t flag) {
    Player* player = GET_PLAYER(gPlayState);

    if (flagType == FLAG_EVENT_CHECK_INF) {
        switch (flag) {
            case EVENTCHKINF_LEARNED_PRELUDE_OF_LIGHT:
                Flags_SetBingoInf(BINGO_INF_PRELUDE_CHECK);
                break;
            case EVENTCHKINF_BONGO_BONGO_ESCAPED_FROM_WELL:
                Flags_SetBingoInf(BINGO_INF_NOCTURNE_CHECK);
                break;
        }
    }
    if (flagType == FLAG_RANDOMIZER_INF) {
        if (Flags_GetRandomizerInf(RAND_INF_GANONS_CASTLE_FOREST_TRIAL_POT_1) &&
            Flags_GetRandomizerInf(RAND_INF_GANONS_CASTLE_FOREST_TRIAL_POT_2)) {
                Flags_SetBingoInf(BINGO_INF_BREAK_FOREST_TRIAL_POTS);
        }
        if (Flags_GetRandomizerInf(RAND_INF_GANONS_CASTLE_FIRE_TRIAL_POT_1) &&
            Flags_GetRandomizerInf(RAND_INF_GANONS_CASTLE_FIRE_TRIAL_POT_2)) {
                Flags_SetBingoInf(BINGO_INF_BREAK_FIRE_TRIAL_POTS);
        }
        if (Flags_GetRandomizerInf(RAND_INF_GANONS_CASTLE_WATER_TRIAL_POT_1) &&
            Flags_GetRandomizerInf(RAND_INF_GANONS_CASTLE_WATER_TRIAL_POT_2)) {
                Flags_SetBingoInf(BINGO_INF_BREAK_WATER_TRIAL_POTS);
        }
        if (Flags_GetRandomizerInf(RAND_INF_GANONS_CASTLE_SPIRIT_TRIAL_POT_1) &&
            Flags_GetRandomizerInf(RAND_INF_GANONS_CASTLE_SPIRIT_TRIAL_POT_2)) {
                Flags_SetBingoInf(BINGO_INF_BREAK_SPIRIT_TRIAL_POTS);
        }
        if (Flags_GetRandomizerInf(RAND_INF_GANONS_CASTLE_SHADOW_TRIAL_POT_1) &&
            Flags_GetRandomizerInf(RAND_INF_GANONS_CASTLE_SHADOW_TRIAL_POT_2)) {
                Flags_SetBingoInf(BINGO_INF_BREAK_SHADOW_TRIAL_POTS);
        }
        if (Flags_GetRandomizerInf(RAND_INF_GANONS_CASTLE_LIGHT_TRIAL_POT_1) &&
            Flags_GetRandomizerInf(RAND_INF_GANONS_CASTLE_LIGHT_TRIAL_POT_2)) {
                Flags_SetBingoInf(BINGO_INF_BREAK_LIGHT_TRIAL_POTS);
        }
    }
    if (flagType == FLAG_RANDOMIZER_INF && (flag >= RAND_INF_SPIRIT_TEMPLE_BOULDER_SILVER_RUPEE_1 &&
                                            flag <= RAND_INF_BOTTOM_OF_THE_WELL_BASEMENT_SILVER_RUPEE_5)) {
        int shadowSilverItemsCollected = 0;
        int trialSilverItemsCollected = 0;
        int iceSilverItemsCollected = 0;

        for (int i = RAND_INF_SPIRIT_TEMPLE_BOULDER_SILVER_RUPEE_1; i <= RAND_INF_BOTTOM_OF_THE_WELL_BASEMENT_SILVER_RUPEE_5; i++) {
            if (Flags_GetRandomizerInf(static_cast<RandomizerInf>(i))) {
                if (i >= RAND_INF_SHADOW_TEMPLE_SCYTHE_SILVER_RUPEE_1 && i <= RAND_INF_SHADOW_TEMPLE_REDEAD_SILVER_RUPEE_5) {
                    ++shadowSilverItemsCollected;
                } else if (i >= RAND_INF_GANONS_CASTLE_FOREST_TRIAL_SILVER_RUPEE_1 && i <= RAND_INF_GANONS_CASTLE_LIGHT_TRIAL_SILVER_RUPEE_5) {
                    ++trialSilverItemsCollected;
                } else if (i >= RAND_INF_ICE_CAVERN_SCYTHE_SILVER_RUPEE_1 && i <= RAND_INF_ICE_CAVERN_PUSH_BLOCK_SILVER_RUPEE_5) {
                    ++iceSilverItemsCollected;
                }
            }
        }

        if (shadowSilverItemsCollected == 15) {
            Flags_SetBingoInf(BINGO_INF_COLLECT_SHADOW_TEMPLE_SILVER_ITEMS);
        }
        if (trialSilverItemsCollected == 20) {
            Flags_SetBingoInf(BINGO_INF_COLLECT_GANONS_CASTLE_SILVER_ITEMS);
        }
        if (iceSilverItemsCollected == 10) {
            Flags_SetBingoInf(BINGO_INF_COLLECT_ICE_CAVERN_SILVER_ITEMS);
        }
    }
}

void BingoOnSceneFlagSetHandler(int16_t sceneNum, int16_t flagType, int16_t flag) {
    constexpr int bossKeyScenes[] = { // I dont think this is actually necessary (as I don't believe mini-dungeons and
        SCENE_FOREST_TEMPLE,          // child dungeons use 0x14) but better safe than sorry?
        SCENE_FIRE_TEMPLE,
        SCENE_WATER_TEMPLE,
        SCENE_SPIRIT_TEMPLE,
        SCENE_SHADOW_TEMPLE,
        SCENE_GANONS_TOWER
    };

    if (flagType == FLAG_SCENE_SWITCH) {
        BingoRecordEyeSwitch(sceneNum, flag);

        switch (sceneNum) {
            case SCENE_FOREST_TEMPLE:
                if (flag == 0x20 || flag == 0x21) Flags_SetBingoInf(BINGO_INF_TWIST_1_FOREST_HALLWAY);
                if (flag == 0x15) Flags_SetBingoInf(BINGO_INF_DEFEAT_ALL_FOREST_STALFOS);
                // FOrest poes existence is tied to switches anyways (leaving before it's set respawns them)
                // so we consider them "dead" only when the appropriate switches are activated
                if (flag == 0x1C) Flags_SetBingoInf(BINGO_INF_DEFEAT_MEG);
                if (flag == 0x1F) Flags_SetBingoInf(BINGO_INF_DEFEAT_AMY);
                if (Flags_GetSwitch(gPlayState, 0x1D) && Flags_GetSwitch(gPlayState, 0x1E)) {
                    Flags_SetBingoInf(BINGO_INF_DEFEAT_JOELLE_AND_BETH);
                }
                break;
            case SCENE_FIRE_TEMPLE:
                if (Flags_GetSwitch(gPlayState, 0x9) && Flags_GetSwitch(gPlayState, 0xB) &&
                    Flags_GetRandomizerInf(RAND_INF_FIRE_TEMPLE_TEMP_HAMMER_SWITCH)) {
                    Flags_SetBingoInf(BINGO_INF_FIRE_RUSTY_HAMMER_SWITCHES);
                }
                if (CountFreedGorons() >= 5) {
                    Flags_SetBingoInf(BINGO_INF_FREE_5_GORONS);
                }
                if (CountFreedGorons() >= 9) {
                    Flags_SetBingoInf(BINGO_INF_FREE_ALL_9_GORONS);
                }
                break;
            case SCENE_WATER_TEMPLE: {
                if (flag == 0x1D) Flags_SetBingoInf(BINGO_INF_WATER_TEMPLE_MIDDLE_LEVEL);

                // Indices 1 and 2 share switch flag 0x1B,
                // so it's being handled in the ShouldActorUpdate function instead.
                static constexpr struct { int16_t flag; uint8_t index; } waterCrystals[] = {
                    { 0x08, 0 }, { 0x27, 3 }, { 0x31, 4 }, { 0x3B, 5 }, { 0x3C, 6 }, { 0x3D, 7 },
                };
                for (const auto& crystal : waterCrystals) {
                    if (flag == crystal.flag) {
                        gSaveContext.ship.bingo.waterCrystalsActivated[crystal.index] = true;
                        break;
                    }
                }
                break;
            }
            case SCENE_SPIRIT_TEMPLE:
                if (Flags_GetSwitch(gPlayState, 0x7) && Flags_GetSwitch(gPlayState, 0xB)) {
                    Flags_SetBingoInf(BINGO_INF_SPIRIT_RUSTY_HAMMER_SWITCHES);
                }
                break;
            default:
                break;
        }

        // Boss key used flag is consistent across all dungeons
        if (flag == 0x14 && std::find(std::begin(bossKeyScenes), std::end(bossKeyScenes), sceneNum) != std::end(bossKeyScenes)) {
            int bksUsed = 1; // If we're even running this code, then it's safe to assume we used a BK anyways
            switch (sceneNum) {
                case SCENE_FOREST_TEMPLE:
                    Flags_SetBingoInf(BINGO_INF_UNLOCK_FOREST_BOSS_DOOR);
                    [[fallthrough]];
                case SCENE_FIRE_TEMPLE:
                case SCENE_WATER_TEMPLE:
                case SCENE_SPIRIT_TEMPLE:
                case SCENE_SHADOW_TEMPLE:
                case SCENE_GANONS_TOWER:
                    for (int i = 0; i < 6; i++) {
                        int scene = bossKeyScenes[i];
                        if (gSaveContext.sceneFlags[scene].swch & (1 << flag)) {
                            ++bksUsed;
                        }
                    }

                    if (bksUsed >= 4) {
                        Flags_SetBingoInf(BINGO_INF_UNLOCK_4_BOSS_DOORS);
                    }
                    break;
                default:
                    break;
            }
        }
    }

    if (flagType == FLAG_SCENE_TREASURE) {
        switch (sceneNum) {
            case SCENE_FIRE_TEMPLE:
                if (Flags_GetTreasure(gPlayState, 0x5) && Flags_GetTreasure(gPlayState, 0xD)) {
                    Flags_SetBingoInf(BINGO_INF_FIRE_TEMPLE_TIMER_CHALLENGE_CHESTS);
                }
                break;
            default:
                break;
        }
    }

    if (flagType == FLAG_SCENE_CLEAR) {
        switch (sceneNum) {
            case SCENE_DEKU_TREE:
                if (flag == 0x9) {
                    Flags_SetBingoInf(BINGO_INF_SCARE_OFF_ALL_DEKU_SCRUBS_IN_TREE);
                }
                break;
            case SCENE_FIRE_TEMPLE:
                if (Flags_GetClear(gPlayState, 0x3) && Flags_GetClear(gPlayState, 0x18)) {
                    Flags_SetBingoInf(BINGO_INF_DEFEAT_BOTH_FLARE_DANCERS);
                }
                break;
            case SCENE_WATER_TEMPLE:
                if (flag == 0xD) Flags_SetBingoInf(BINGO_INF_DEFEAT_DARK_LINK);
            default:
                break;
        }

        if (flag == 0x1 && IsBossScene(sceneNum)) {
            if (!previousDungeonScenes.empty()) {
                bool beatTemple = false;
                for (int16_t previousDungeonsList : previousDungeonScenes) {
                    switch (previousDungeonsList) {
                        case SCENE_DEKU_TREE:
                            Flags_SetBingoInf(BINGO_INF_BEAT_DEKU_TREE);
                            break;
                        case SCENE_DODONGOS_CAVERN:
                            Flags_SetBingoInf(BINGO_INF_BEAT_DODONGOS_CAVERN);
                            break;
                        case SCENE_JABU_JABU:
                            Flags_SetBingoInf(BINGO_INF_BEAT_JABU_JABUS_BELLY);
                            break;
                        case SCENE_FOREST_TEMPLE:
                            Flags_SetBingoInf(BINGO_INF_BEAT_FOREST_TEMPLE);
                            beatTemple = true;
                            break;
                        case SCENE_FIRE_TEMPLE:
                            Flags_SetBingoInf(BINGO_INF_BEAT_FIRE_TEMPLE);
                            beatTemple = true;
                            break;
                        case SCENE_WATER_TEMPLE:
                            Flags_SetBingoInf(BINGO_INF_BEAT_WATER_TEMPLE);
                            beatTemple = true;
                            break;
                        case SCENE_SPIRIT_TEMPLE:
                            Flags_SetBingoInf(BINGO_INF_BEAT_SPIRIT_TEMPLE);
                            beatTemple = true;
                            break;
                        case SCENE_SHADOW_TEMPLE:
                            Flags_SetBingoInf(BINGO_INF_BEAT_SHADOW_TEMPLE);
                            beatTemple = true;
                            break;
                        default:
                            break;
                    }
                }

                if (beatTemple) {
                    constexpr BingoInf templeBeatFlags[] = {
                        BINGO_INF_BEAT_FOREST_TEMPLE,
                        BINGO_INF_BEAT_FIRE_TEMPLE,
                        BINGO_INF_BEAT_WATER_TEMPLE,
                        BINGO_INF_BEAT_SPIRIT_TEMPLE,
                        BINGO_INF_BEAT_SHADOW_TEMPLE,
                    };
                    int templesBeaten = 0;
                    for (BingoInf templeBeatFlag : templeBeatFlags) {
                        if (Flags_GetBingoInf(templeBeatFlag)) {
                            ++templesBeaten;
                        }
                    }
                    if (templesBeaten >= 2) {
                        Flags_SetBingoInf(BINGO_INF_BEAT_2_TEMPLES);
                    }
                    if (templesBeaten >= 4) {
                        Flags_SetBingoInf(BINGO_INF_BEAT_4_TEMPLES);
                    }
                }
            } else {
                // Defeat boss squares not yet added
                return;
            }
        }
    }
}

void BingoOnSceneInitHandler(int16_t sceneNum) {
    auto ctx = Rando::Context::GetInstance();

    if (IsDungeonScene(sceneNum)) {
        if (std::find(previousDungeonScenes.begin(), previousDungeonScenes.end(), sceneNum) != previousDungeonScenes.end()) {
            return;
        } else {
            previousDungeonScenes.push_back(sceneNum);
            LUSLOG_DEBUG("%s added to previousDungeonScenes", SohUtils::GetSceneName(previousDungeonScenes.back()).c_str());
            for (int16_t scene : previousDungeonScenes) {
                LUSLOG_DEBUG("Previous dungeon scene list contains %s", SohUtils::GetSceneName(scene).c_str());
            }
        }
    } else if (!IsBossScene(sceneNum) && !previousDungeonScenes.empty()) {
        previousDungeonScenes.clear();
        LUSLOG_DEBUG("Previous dungeon scene reset to: %d", previousDungeonScenes.empty() ? -1 : previousDungeonScenes.back());
    }

    if (IsDungeonScene(sceneNum) && sceneNum != SCENE_GANONS_TOWER) {
        int enteredDungeonCount = 0;
        gSaveContext.ship.bingo.enteredDungeons[sceneNum] = true;

        for (int i = 0; i <= SCENE_INSIDE_GANONS_CASTLE; ++i) {
            if (gSaveContext.ship.bingo.enteredDungeons[i]) {
                ++enteredDungeonCount;
                LUSLOG_DEBUG("Has entered %s, enteredDungeonCount: %d", SohUtils::GetSceneName(i).c_str(), enteredDungeonCount);
            }
        }

        if (enteredDungeonCount == 12) {
            Flags_SetBingoInf(BINGO_INF_ENTER_EVERY_DUNGEON);
            LUSLOG_DEBUG("All dungeons entered");
        }
    }

    // If we have mixed entracnes and the player enters a mini-dungeon from a main dungeon,
    // we will consider the main dungeons as being beaten, since mini-dungeons lack bosses to kill.
    // Currently lacks the ability to detect entering a boss door and exiting to overworld, but I don't
    // think that's something ship does anyways? Cross that bridge when we get there.
    if (IsMiniDungeonScene(sceneNum) && !previousDungeonScenes.empty() &&
        ctx->GetOption(RSK_MIX_DUNGEON_ENTRANCES) && ctx->GetOption(RSK_MIX_BOSS_ENTRANCES)) {
            bool beatTemple = false;
            for (int16_t previousDungeonsList : previousDungeonScenes) {
                switch (previousDungeonsList) {
                    case SCENE_DEKU_TREE:
                        Flags_SetBingoInf(BINGO_INF_BEAT_DEKU_TREE);
                        break;
                    case SCENE_DODONGOS_CAVERN:
                        Flags_SetBingoInf(BINGO_INF_BEAT_DODONGOS_CAVERN);
                        break;
                    case SCENE_JABU_JABU:
                        Flags_SetBingoInf(BINGO_INF_BEAT_JABU_JABUS_BELLY);
                        break;
                    case SCENE_FOREST_TEMPLE:
                        Flags_SetBingoInf(BINGO_INF_BEAT_FOREST_TEMPLE);
                        beatTemple = true;
                        break;
                    case SCENE_FIRE_TEMPLE:
                        Flags_SetBingoInf(BINGO_INF_BEAT_FIRE_TEMPLE);
                        beatTemple = true;
                        break;
                    case SCENE_WATER_TEMPLE:
                        Flags_SetBingoInf(BINGO_INF_BEAT_WATER_TEMPLE);
                        beatTemple = true;
                        break;
                    case SCENE_SPIRIT_TEMPLE:
                        Flags_SetBingoInf(BINGO_INF_BEAT_SPIRIT_TEMPLE);
                        beatTemple = true;
                        break;
                    case SCENE_SHADOW_TEMPLE:
                        Flags_SetBingoInf(BINGO_INF_BEAT_SHADOW_TEMPLE);
                        beatTemple = true;
                        break;
                    default:
                        break;
                }
            }

            if (beatTemple) {
                constexpr BingoInf templeBeatFlags[] = {
                    BINGO_INF_BEAT_FOREST_TEMPLE,
                    BINGO_INF_BEAT_FIRE_TEMPLE,
                    BINGO_INF_BEAT_WATER_TEMPLE,
                    BINGO_INF_BEAT_SPIRIT_TEMPLE,
                    BINGO_INF_BEAT_SHADOW_TEMPLE,
                };
                int templesBeaten = 0;
                for (BingoInf templeBeatFlag : templeBeatFlags) {
                    if (Flags_GetBingoInf(templeBeatFlag)) {
                        ++templesBeaten;
                    }
                }
                if (templesBeaten >= 2) {
                    Flags_SetBingoInf(BINGO_INF_BEAT_2_TEMPLES);
                }
                if (templesBeaten >= 4) {
                    Flags_SetBingoInf(BINGO_INF_BEAT_4_TEMPLES);
                }
            }
    }
}

void BingoOnItemReceiveHandler(GetItemEntry receivedItemEntry) {
    if (receivedItemEntry.getItemId >= RG_DEKU_TREE_MAP && receivedItemEntry.getItemId <= RG_GANONS_CASTLE_BOSS_KEY) {
        int mapsHeld = 0;
        int compassesHeld = 0;
        int bossKeysHeld = 0;
        for (int i = 0; i < 11; i++) {
            if (CHECK_DUNGEON_ITEM(DUNGEON_MAP, i) && CHECK_DUNGEON_ITEM(DUNGEON_COMPASS, i)) {
                Flags_SetBingoInf(static_cast<BingoInf>(i + BINGO_INF_DEKU_MAP_COMPASS));
            }
            if (CHECK_DUNGEON_ITEM(DUNGEON_MAP, i)) mapsHeld++;
            if (CHECK_DUNGEON_ITEM(DUNGEON_COMPASS, i)) compassesHeld++;
            if (CHECK_DUNGEON_ITEM(DUNGEON_KEY_BOSS, i)) {
                if (i == SCENE_GANONS_TOWER) {
                    Flags_SetBingoInf(BINGO_INF_GANON_BOSS_KEY);
                } else if (i >= SCENE_FOREST_TEMPLE && i <= SCENE_SHADOW_TEMPLE) {
                    Flags_SetBingoInf(static_cast<BingoInf>(i + (BINGO_INF_FOREST_BOSS_KEY - 3)));
                }
                bossKeysHeld++;
            }
        }

        if (mapsHeld >= 4) {
            Flags_SetBingoInf(BINGO_INF_4_MAPS);
        }
        if (mapsHeld >= 6) {
            Flags_SetBingoInf(BINGO_INF_6_MAPS);
        }
        if (compassesHeld >= 4) {
            Flags_SetBingoInf(BINGO_INF_4_COMPASSES);
        }
        if (compassesHeld >= 6) {
            Flags_SetBingoInf(BINGO_INF_6_COMPASSES);
        }
        if (bossKeysHeld >= 3) {
            Flags_SetBingoInf(BINGO_INF_3_BOSS_KEYS);
        }
        if (bossKeysHeld >= 5) {
            Flags_SetBingoInf(BINGO_INF_5_BOSS_KEYS);
        }
    }

    if (receivedItemEntry.getItemId >= RG_KOKIRI_EMERALD && receivedItemEntry.getItemId <= RG_LIGHT_MEDALLION) {
        int medallionsHeld = 0;
        int stonesHeld = 0;
        for (int i = 0; i < 6; i++) {
            if (CHECK_QUEST_ITEM(QUEST_MEDALLION_FOREST + i)) {
                medallionsHeld++;
            }
        }
        if (medallionsHeld >= 5) {
            Flags_SetBingoInf(BINGO_INF_5_MEDALLIONS);
        }
        if (CHECK_QUEST_ITEM(QUEST_KOKIRI_EMERALD)) stonesHeld++;
        if (CHECK_QUEST_ITEM(QUEST_GORON_RUBY)) stonesHeld++;
        if (CHECK_QUEST_ITEM(QUEST_ZORA_SAPPHIRE)) stonesHeld++;
        if (stonesHeld >= 2) {
            Flags_SetBingoInf(BINGO_INF_2_STONES);
        }
        if (CHECK_QUEST_ITEM(QUEST_KOKIRI_EMERALD) && CHECK_QUEST_ITEM(QUEST_MEDALLION_SHADOW) &&
            CHECK_QUEST_ITEM(QUEST_MEDALLION_FIRE)) {
                Flags_SetBingoInf(BINGO_INF_SHADOW_FIRE_MEDAL_EMERALD);
        }
        if (CHECK_QUEST_ITEM(QUEST_GORON_RUBY) && CHECK_QUEST_ITEM(QUEST_MEDALLION_FOREST) &&
            CHECK_QUEST_ITEM(QUEST_MEDALLION_SPIRIT)) {
                Flags_SetBingoInf(BINGO_INF_FOREST_SPIRIT_MEDAL_RUBY);
        }
        if (CHECK_QUEST_ITEM(QUEST_ZORA_SAPPHIRE) && CHECK_QUEST_ITEM(QUEST_MEDALLION_LIGHT) &&
            CHECK_QUEST_ITEM(QUEST_MEDALLION_WATER)) {
                Flags_SetBingoInf(BINGO_INF_LIGHT_WATER_MEDAL_SAPPHIRE);
        }

        LUSLOG_DEBUG("medallionsHeld: %d, stonesHeld: %d", medallionsHeld, stonesHeld);
    }
}

void BingoOnDialogMessageHandler() {
    MessageContext* msgCtx = &gPlayState->msgCtx;
    Actor* actor = msgCtx->talkActor;

    if (gPlayState->sceneNum == SCENE_DODONGOS_CAVERN) {
        if (actor && actor->id == ACTOR_EN_DNS) {
            static constexpr struct { int16_t params; uint8_t index; } dcScrubs[] = {
                    { 0x0, 0 }, { 0x1, 1 }, { 0x3, 2 }, { 0x4, 3 }
                };
                for (const auto& scrub : dcScrubs) {
                    if ((actor->params & 0xFF) == scrub.params) {
                        gSaveContext.ship.bingo.dcScrubsSpoken[scrub.index] = true;
                        break;
                    }
                }
        }

        auto& spoken = gSaveContext.ship.bingo.dcScrubsSpoken;
        if (std::all_of(std::begin(spoken), std::end(spoken), [](u8 playerSpokeTo) { return playerSpokeTo != 0; })) {
            Flags_SetBingoInf(BINGO_INF_SPEAK_WITH_4_DC_BUSINESS_SCRUBS);
        }
    }
}

void BingoOnActorUpdateHandler(void* refActor) {
    Actor* actor = static_cast<Actor*>(refActor);

    if (actor->id == ACTOR_EN_DODONGO) {
        EnDodongo* dodongo = reinterpret_cast<EnDodongo*>(actor);
        if (dodongo->actionState == 1) {
            Flags_SetBingoInf(BINGO_INF_FEED_A_DODONGO_A_BOMB_OR_FLOWER);
        }
    }

    if (actor->id == ACTOR_EN_WALLMAS) {
        EnWallmas* wallmaster = reinterpret_cast<EnWallmas*>(actor);
        if (wallmaster->timer == -0x1E &&
            !gSaveContext.ship.bingo.wallmasterVoids[gPlayState->sceneNum]) {
            gSaveContext.ship.bingo.wallmasterVoids[gPlayState->sceneNum] = true;

            int uniqueWallmasterVoids = 0;
            for (int i = 0; i <= SCENE_OUTSIDE_GANONS_CASTLE; i++) {
                if (gSaveContext.ship.bingo.wallmasterVoids[i]) {
                    uniqueWallmasterVoids++;
                }
            }
            
            if (uniqueWallmasterVoids >= 3) {
                Flags_SetBingoInf(BINGO_INF_VOIDED_BY_3_DIFFERENT_DUNGEON_WALLMASTERS);
            }
        }
    }

    if (actor->id == ACTOR_EN_RU1) {
        EnRu1* ruto = reinterpret_cast<EnRu1*>(actor);
        if (ruto->waterState != 0) {
            Flags_SetBingoInf(BINGO_INF_DROWN_RUTO);
        }
    }
}

void BingoShouldActorUpdateHandler(void* refActor, bool*) {
    Actor* actor = static_cast<Actor*>(refActor);

    if (actor->id == ACTOR_EN_FIREFLY) {
        EnFirefly* firefly = reinterpret_cast<EnFirefly*>(actor);
        if (firefly->collider.base.atFlags & AT_HIT && firefly->auraType == 1 &&
            !gSaveContext.ship.bingo.uniqueKeeseIgnitions[gPlayState->sceneNum]) {
            // Because you can ignite ANY keese with Din's Fire for the "Be Ignited" square,
            // we will save which scenes you were ignited in to the below array.
            // Additionally, only 1 grotto has keese, so we don't need to worry about grotto indexing.
            gSaveContext.ship.bingo.uniqueKeeseIgnitions[gPlayState->sceneNum] = true;

            int uniqueKeeseIgnitionCount = 0;
            for (int i = 0; i <= SCENE_OUTSIDE_GANONS_CASTLE; i++) {
                if (gSaveContext.ship.bingo.uniqueKeeseIgnitions[i]) {
                    uniqueKeeseIgnitionCount++;
                }
            }
            
            if (uniqueKeeseIgnitionCount >= 3) {
                Flags_SetBingoInf(BINGO_INF_IGNITED_BY_3_FIRE_KEESE);
            }
        }
    }

    if (actor->id == ACTOR_EN_FIREFLY) {
        EnFirefly* firefly = reinterpret_cast<EnFirefly*>(actor);
        if (firefly->collider.base.atFlags & AT_HIT && firefly->auraType == 2) {
            Flags_SetBingoInf(BINGO_INF_FROZEN_BY_ICE_KEESE);
        }
    }

    if (actor->id == ACTOR_EN_TUBO_TRAP) {
        EnTuboTrap* flyingPot = reinterpret_cast<EnTuboTrap*>(actor);
        if (flyingPot->collider.base.atFlags & AT_HIT) {
            static constexpr struct { int16_t scene; uint8_t index; } flyingPots[] = {
                { SCENE_ICE_CAVERN, 0 }, { SCENE_GERUDO_TRAINING_GROUND, 1 },
                { SCENE_SPIRIT_TEMPLE, 2 }, { SCENE_BOTTOM_OF_THE_WELL, 3 },
                { SCENE_SHADOW_TEMPLE, 4 }
            };
            for (const auto& pot : flyingPots) {
                if (gPlayState->sceneNum == pot.scene) {
                    gSaveContext.ship.bingo.flyingPotDamaged[pot.index] = true;
                    break;
                }
            }

            int uniqueFlyingPotCount = 0;
            for (size_t i = 0; i < ARRAY_COUNT(gSaveContext.ship.bingo.flyingPotDamaged); i++) {
                if (gSaveContext.ship.bingo.flyingPotDamaged[i]) {
                    uniqueFlyingPotCount++;
                }
            }

            if (uniqueFlyingPotCount >= 3) {
                Flags_SetBingoInf(BINGO_INF_DAMAGED_BY_3_DIFFERENT_DUNGEON_POTS);
            }
        }
    }

    if (gPlayState->sceneNum == SCENE_WATER_TEMPLE && gPlayState->roomCtx.curRoom.num == 0x8 &&
        actor->id == ACTOR_OBJ_SWITCH) {
        ObjSwitch* objSwitch = reinterpret_cast<ObjSwitch*>(actor);

        /*
            Even with the below checks in place, hitting the switch even when it's "on"
            still triggers this logic. The ObjSwitch actor doesn't seem to have anything
            I can look at to prevent this, so we will assume an honor system for now; even still,
            it's not really a big deal.
        */
        if (objSwitch->jntSph.col.base.acFlags & AC_HIT) {
            if (objSwitch->dyna.actor.world.pos.y == -212) {
                gSaveContext.ship.bingo.waterCrystalsActivated[1] = true;
            } else {
                gSaveContext.ship.bingo.waterCrystalsActivated[2] = true;
            }
        }
    }
}

static void BingoRegisterHooks() {
    static uint32_t onFlagSetHook = 0;
    static uint32_t onSceneFlagSetHook = 0;
    static uint32_t onPlayerUpdateForRCQueueHook = 0;
    static uint32_t onPlayerUpdateForItemQueueHook = 0;
    static uint32_t onItemReceiveHook = 0;
    static uint32_t onDialogMessageHook = 0;
    static uint32_t onVanillaBehaviorHook = 0;
    static uint32_t onSceneInitHook = 0;
    static uint32_t afterSceneCommandsHook = 0;
    static uint32_t onActorInitHook = 0;
    static uint32_t shouldActorUpdateHook = 0;
    static uint32_t onActorUpdateHook = 0;
    static uint32_t onPlayerUpdateHook = 0;
    static uint32_t onGameFrameUpdateHook = 0;
    static uint32_t onSceneSpawnActorsHook = 0;
    static uint32_t onPlayDestroyHook = 0;
    static uint32_t onExitGameHook = 0;
    static uint32_t onKaleidoUpdateHook = 0;

    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnLoadGame>([](int32_t fileNum) {
        ShipInit::Init("IS_RANDO");

        randomizerQueuedChecks = std::queue<RandomizerCheck>();
        randomizerQueuedCheck = RC_UNKNOWN_CHECK;
        randomizerQueuedItemEntry = GET_ITEM_NONE;

        GameInteractor::Instance->UnregisterGameHook<GameInteractor::OnFlagSet>(onFlagSetHook);
        GameInteractor::Instance->UnregisterGameHook<GameInteractor::OnSceneFlagSet>(onSceneFlagSetHook);
        GameInteractor::Instance->UnregisterGameHook<GameInteractor::OnPlayerUpdate>(onPlayerUpdateForRCQueueHook);
        GameInteractor::Instance->UnregisterGameHook<GameInteractor::OnPlayerUpdate>(onPlayerUpdateForItemQueueHook);
        GameInteractor::Instance->UnregisterGameHook<GameInteractor::OnItemReceive>(onItemReceiveHook);
        GameInteractor::Instance->UnregisterGameHook<GameInteractor::OnItemReceive>(onDialogMessageHook);
        GameInteractor::Instance->UnregisterGameHook<GameInteractor::OnVanillaBehavior>(onVanillaBehaviorHook);
        GameInteractor::Instance->UnregisterGameHook<GameInteractor::OnSceneInit>(onSceneInitHook);
        GameInteractor::Instance->UnregisterGameHook<GameInteractor::AfterSceneCommands>(afterSceneCommandsHook);
        GameInteractor::Instance->UnregisterGameHook<GameInteractor::OnActorInit>(onActorInitHook);
        GameInteractor::Instance->UnregisterGameHook<GameInteractor::ShouldActorUpdate>(shouldActorUpdateHook);
        GameInteractor::Instance->UnregisterGameHook<GameInteractor::OnActorUpdate>(onActorUpdateHook);
        GameInteractor::Instance->UnregisterGameHook<GameInteractor::OnPlayerUpdate>(onPlayerUpdateHook);
        GameInteractor::Instance->UnregisterGameHook<GameInteractor::OnGameFrameUpdate>(onGameFrameUpdateHook);
        GameInteractor::Instance->UnregisterGameHook<GameInteractor::OnSceneSpawnActors>(onSceneSpawnActorsHook);
        GameInteractor::Instance->UnregisterGameHook<GameInteractor::OnPlayDestroy>(onPlayDestroyHook);
        GameInteractor::Instance->UnregisterGameHook<GameInteractor::OnExitGame>(onExitGameHook);
        GameInteractor::Instance->UnregisterGameHook<GameInteractor::OnKaleidoscopeUpdate>(onKaleidoUpdateHook);

        onFlagSetHook = 0;
        onSceneFlagSetHook = 0;
        onPlayerUpdateForRCQueueHook = 0;
        onPlayerUpdateForItemQueueHook = 0;
        onItemReceiveHook = 0;
        onDialogMessageHook = 0;
        onVanillaBehaviorHook = 0;
        onSceneInitHook = 0;
        afterSceneCommandsHook = 0;
        onActorInitHook = 0;
        shouldActorUpdateHook = 0;
        onActorUpdateHook = 0;
        onPlayerUpdateHook = 0;
        onGameFrameUpdateHook = 0;
        onSceneSpawnActorsHook = 0;
        onPlayDestroyHook = 0;
        onExitGameHook = 0;
        onKaleidoUpdateHook = 0;

        if (!IS_RANDO)
            return;

        // ENTRTODO: Move all entrance rando handling to a dedicated file
        // Setup the modified entrance table and entrance shuffle table for rando
        Entrance_Init();

        // Handle randomized spawn positions after the save context has been setup from load
        if (RAND_GET_OPTION(RSK_SHUFFLE_ENTRANCES)) {
            Entrance_SetSavewarpEntrance();
        }

        onFlagSetHook =
            GameInteractor::Instance->RegisterGameHook<GameInteractor::OnFlagSet>(BingoOnFlagSetHandler);
        onSceneFlagSetHook =
            GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneFlagSet>(BingoOnSceneFlagSetHandler);
        // onPlayerUpdateForRCQueueHook = GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>(
        //     BingoOnPlayerUpdateForRCQueueHandler);
        // onPlayerUpdateForItemQueueHook = GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>(
        //     BingoOnPlayerUpdateForItemQueueHandler);
        onItemReceiveHook =
            GameInteractor::Instance->RegisterGameHook<GameInteractor::OnItemReceive>(BingoOnItemReceiveHandler);
        onDialogMessageHook = GameInteractor::Instance->RegisterGameHook<GameInteractor::OnDialogMessage>(
            BingoOnDialogMessageHandler);
        // onVanillaBehaviorHook = GameInteractor::Instance->RegisterGameHook<GameInteractor::OnVanillaBehavior>(
        //     BingoOnVanillaBehaviorHandler);
        onSceneInitHook =
            GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>(BingoOnSceneInitHandler);
        // afterSceneCommandsHook = GameInteractor::Instance->RegisterGameHook<GameInteractor::AfterSceneCommands>(
        //     BingoAfterSceneCommandsHandler);
        // onActorInitHook =
        //     GameInteractor::Instance->RegisterGameHook<GameInteractor::OnActorInit>(BingoOnActorInitHandler);
        shouldActorUpdateHook =
            GameInteractor::Instance->RegisterGameHook<GameInteractor::ShouldActorUpdate>(BingoShouldActorUpdateHandler);
        onActorUpdateHook =
            GameInteractor::Instance->RegisterGameHook<GameInteractor::OnActorUpdate>(BingoOnActorUpdateHandler);
        // onPlayerUpdateHook =
        //     GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>(BingoOnPlayerUpdateHandler);
        // onGameFrameUpdateHook = GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameFrameUpdate>(
        //     BingoOnGameFrameUpdateHandler);
        // onSceneSpawnActorsHook = GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneSpawnActors>(
        //     BingoOnSceneSpawnActorsHandler);
        // onPlayDestroyHook =
        //     GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>(BingoOnPlayDestroyHandler);
        // onExitGameHook =
        //     GameInteractor::Instance->RegisterGameHook<GameInteractor::OnExitGame>(BingoOnExitGameHandler);
        // onKaleidoUpdateHook = GameInteractor::Instance->RegisterGameHook<GameInteractor::OnKaleidoscopeUpdate>(
        //     BingoOnKaleidoscopeUpdateHandler);
    });
}

static RegisterShipInitFunc initFunc_RegisterHooks(BingoRegisterHooks);