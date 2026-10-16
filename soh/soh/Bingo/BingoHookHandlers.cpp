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

extern "C" {
#include "macros.h"
#include "functions.h"
#include "variables.h"
}

extern SaveContext gSaveContext;
extern PlayState* gPlayState;

static std::queue<RandomizerCheck> randomizerQueuedChecks;
static RandomizerCheck randomizerQueuedCheck = RC_UNKNOWN_CHECK;
static GetItemEntry randomizerQueuedItemEntry = GET_ITEM_NONE;

void BingoOnFlagSetHandler(int16_t flagType, int16_t flag) {
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

    if (flagType == FLAG_SCENE_SWITCH && flag == 0x14 && std::find(std::begin(bossKeyScenes), std::end(bossKeyScenes), sceneNum) != std::end(bossKeyScenes)) {
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

void BingoOnSceneInitHandler(int16_t sceneNum) {
    if (sceneNum <= SCENE_INSIDE_GANONS_CASTLE && sceneNum >= SCENE_DEKU_TREE &&
        sceneNum != SCENE_GANONS_TOWER && sceneNum != SCENE_THIEVES_HIDEOUT) {
        int dungeonIndex = sceneNum;
        if (sceneNum == SCENE_GERUDO_TRAINING_GROUND) {
            dungeonIndex = 10;
        } else if (sceneNum == SCENE_INSIDE_GANONS_CASTLE) {
            dungeonIndex = 11;
        }
        gSaveContext.ship.quest.data.bingo.dungeonsEntered[dungeonIndex] |= 1;

        int dungeonsEntered = 0;

        for (int i = 0; i < 12; i++) {
            if (gSaveContext.ship.quest.data.bingo.dungeonsEntered[i] & 1) {
                dungeonsEntered++;
            }
        }

        if (dungeonsEntered == 12) {
            Flags_SetBingoInf(BINGO_INF_ENTER_EVERY_DUNGEON);
        }
    }
}

void BingoOnItemReceiveHandler(GetItemEntry receivedItemEntry) {
    if (receivedItemEntry.getItemId >= RG_DEKU_TREE_MAP && receivedItemEntry.getItemId <= RG_GANONS_CASTLE_BOSS_KEY) {
        int mapsHeld = 0;
        int compassesHeld = 0;
        int bossKeysHeld = 0;
        for (int i = 0; i < 14; i++) {
            if (CHECK_DUNGEON_ITEM(DUNGEON_MAP, i) && CHECK_DUNGEON_ITEM(DUNGEON_COMPASS, i)) {
                Flags_SetBingoInf(static_cast<BingoInf>(i + BINGO_INF_DEKU_MAP_COMPASS));
            }
            if (CHECK_DUNGEON_ITEM(DUNGEON_MAP, i)) mapsHeld++;
            if (CHECK_DUNGEON_ITEM(DUNGEON_COMPASS, i)) compassesHeld++;
            if (CHECK_DUNGEON_ITEM(DUNGEON_KEY_BOSS, i)) {
                Flags_SetBingoInf(static_cast<BingoInf>(i + BINGO_INF_FOREST_BOSS_KEY));
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
        // onDialogMessageHook = GameInteractor::Instance->RegisterGameHook<GameInteractor::OnDialogMessage>(
        //     BingoOnDialogMessageHandler);
        // onVanillaBehaviorHook = GameInteractor::Instance->RegisterGameHook<GameInteractor::OnVanillaBehavior>(
        //     BingoOnVanillaBehaviorHandler);
        onSceneInitHook =
            GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>(BingoOnSceneInitHandler);
        // afterSceneCommandsHook = GameInteractor::Instance->RegisterGameHook<GameInteractor::AfterSceneCommands>(
        //     BingoAfterSceneCommandsHandler);
        // onActorInitHook =
        //     GameInteractor::Instance->RegisterGameHook<GameInteractor::OnActorInit>(BingoOnActorInitHandler);
        // onActorUpdateHook =
        //     GameInteractor::Instance->RegisterGameHook<GameInteractor::OnActorUpdate>(BingoOnActorUpdateHandler);
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