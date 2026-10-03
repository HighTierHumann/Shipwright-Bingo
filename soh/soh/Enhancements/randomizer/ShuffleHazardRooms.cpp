#include "ShuffleHazardRooms.h"
#include <soh/OTRGlobals.h>
#include "soh/Enhancements/randomizer/randomizer.h"
#include "soh/Enhancements/randomizer/rng.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/ShipInit.hpp"

#include <vector>
#include <set>
#include <utility>
#include <algorithm>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

extern "C" {
#include "functions.h"
#include "variables.h"
extern PlayState* gPlayState;
extern SaveContext gSaveContext;
}

namespace {

// Number of rooms (room_0..room_{N-1}) that exist for each real, non-debug scene in the game.
// Derived directly from the DMA room segment lists in segment_symbols.h, matched to their
// SceneID via the scene name table in scene_table.h.
constexpr std::pair<int16_t, int8_t> kSceneRoomCounts[] = {
    { SCENE_DEKU_TREE, 12 },
    { SCENE_DODONGOS_CAVERN, 17 },
    { SCENE_JABU_JABU, 16 },
    { SCENE_FOREST_TEMPLE, 23 },
    { SCENE_FIRE_TEMPLE, 27 },
    { SCENE_WATER_TEMPLE, 23 },
    { SCENE_SPIRIT_TEMPLE, 29 },
    { SCENE_SHADOW_TEMPLE, 23 },
    { SCENE_BOTTOM_OF_THE_WELL, 7 },
    { SCENE_ICE_CAVERN, 12 },
    { SCENE_GANONS_TOWER, 10 },
    { SCENE_GERUDO_TRAINING_GROUND, 11 },
    { SCENE_THIEVES_HIDEOUT, 6 },
    { SCENE_INSIDE_GANONS_CASTLE, 20 },
    { SCENE_TREASURE_BOX_SHOP, 7 },
    { SCENE_DEKU_TREE_BOSS, 2 },
    { SCENE_DODONGOS_CAVERN_BOSS, 2 },
    { SCENE_JABU_JABU_BOSS, 2 },
    { SCENE_FOREST_TEMPLE_BOSS, 2 },
    { SCENE_FIRE_TEMPLE_BOSS, 2 },
    { SCENE_WATER_TEMPLE_BOSS, 2 },
    { SCENE_SPIRIT_TEMPLE_BOSS, 4 },
    { SCENE_SHADOW_TEMPLE_BOSS, 2 },
    { SCENE_GANONDORF_BOSS, 1 },
    { SCENE_GANONS_TOWER_COLLAPSE_EXTERIOR, 1 },
    { SCENE_MARKET_ENTRANCE_DAY, 1 },
    { SCENE_MARKET_ENTRANCE_NIGHT, 1 },
    { SCENE_MARKET_ENTRANCE_RUINS, 1 },
    { SCENE_BACK_ALLEY_DAY, 1 },
    { SCENE_BACK_ALLEY_NIGHT, 1 },
    { SCENE_MARKET_DAY, 1 },
    { SCENE_MARKET_NIGHT, 1 },
    { SCENE_MARKET_RUINS, 1 },
    { SCENE_TEMPLE_OF_TIME_EXTERIOR_DAY, 1 },
    { SCENE_TEMPLE_OF_TIME_EXTERIOR_NIGHT, 1 },
    { SCENE_TEMPLE_OF_TIME_EXTERIOR_RUINS, 1 },
    { SCENE_KNOW_IT_ALL_BROS_HOUSE, 1 },
    { SCENE_TWINS_HOUSE, 1 },
    { SCENE_MIDOS_HOUSE, 1 },
    { SCENE_SARIAS_HOUSE, 1 },
    { SCENE_KAKARIKO_CENTER_GUEST_HOUSE, 1 },
    { SCENE_BACK_ALLEY_HOUSE, 1 },
    { SCENE_BAZAAR, 1 },
    { SCENE_KOKIRI_SHOP, 1 },
    { SCENE_GORON_SHOP, 1 },
    { SCENE_ZORA_SHOP, 1 },
    { SCENE_POTION_SHOP_KAKARIKO, 1 },
    { SCENE_POTION_SHOP_MARKET, 1 },
    { SCENE_BOMBCHU_SHOP, 1 },
    { SCENE_HAPPY_MASK_SHOP, 1 },
    { SCENE_LINKS_HOUSE, 1 },
    { SCENE_DOG_LADY_HOUSE, 1 },
    { SCENE_STABLE, 1 },
    { SCENE_IMPAS_HOUSE, 1 },
    { SCENE_LAKESIDE_LABORATORY, 1 },
    { SCENE_CARPENTERS_TENT, 1 },
    { SCENE_GRAVEKEEPERS_HUT, 1 },
    { SCENE_GREAT_FAIRYS_FOUNTAIN_MAGIC, 1 },
    { SCENE_FAIRYS_FOUNTAIN, 1 },
    { SCENE_GREAT_FAIRYS_FOUNTAIN_SPELLS, 1 },
    { SCENE_GROTTOS, 14 },
    { SCENE_REDEAD_GRAVE, 1 },
    { SCENE_GRAVE_WITH_FAIRYS_FOUNTAIN, 1 },
    { SCENE_ROYAL_FAMILYS_TOMB, 3 },
    { SCENE_SHOOTING_GALLERY, 1 },
    { SCENE_TEMPLE_OF_TIME, 2 },
    { SCENE_CHAMBER_OF_THE_SAGES, 1 },
    { SCENE_CASTLE_COURTYARD_GUARDS_DAY, 1 },
    { SCENE_CASTLE_COURTYARD_GUARDS_NIGHT, 1 },
    { SCENE_WINDMILL_AND_DAMPES_GRAVE, 7 },
    { SCENE_FISHING_POND, 1 },
    { SCENE_CASTLE_COURTYARD_ZELDA, 1 },
    { SCENE_BOMBCHU_BOWLING_ALLEY, 1 },
    { SCENE_LON_LON_BUILDINGS, 3 },
    { SCENE_MARKET_GUARD_HOUSE, 1 },
    { SCENE_POTION_SHOP_GRANNY, 1 },
    { SCENE_GANON_BOSS, 1 },
    { SCENE_HOUSE_OF_SKULLTULA, 1 },
    { SCENE_HYRULE_FIELD, 1 },
    { SCENE_KAKARIKO_VILLAGE, 1 },
    { SCENE_GRAVEYARD, 2 },
    { SCENE_ZORAS_RIVER, 2 },
    { SCENE_KOKIRI_FOREST, 3 },
    { SCENE_SACRED_FOREST_MEADOW, 1 },
    { SCENE_LAKE_HYLIA, 1 },
    { SCENE_ZORAS_DOMAIN, 2 },
    { SCENE_ZORAS_FOUNTAIN, 1 },
    { SCENE_GERUDO_VALLEY, 1 },
    { SCENE_LOST_WOODS, 10 },
    { SCENE_DESERT_COLOSSUS, 1 },
    { SCENE_GERUDOS_FORTRESS, 2 },
    { SCENE_HAUNTED_WASTELAND, 2 },
    { SCENE_HYRULE_CASTLE, 1 },
    { SCENE_DEATH_MOUNTAIN_TRAIL, 1 },
    { SCENE_DEATH_MOUNTAIN_CRATER, 2 },
    { SCENE_GORON_CITY, 4 },
    { SCENE_LON_LON_RANCH, 1 },
    { SCENE_OUTSIDE_GANONS_CASTLE, 1 },
};

// Same dungeon/overworld classification already used elsewhere for scene-based shuffles
// (see ShuffleWonderItems.cpp). Boss rooms and Thieves' Hideout are treated as overworld here.
bool IsDungeonScene(int16_t scene) {
    return (scene >= SCENE_DEKU_TREE && scene <= SCENE_GERUDO_TRAINING_GROUND) ||
           scene == SCENE_INSIDE_GANONS_CASTLE;
}

std::set<std::tuple<int16_t, int8_t, bool>>& GetHazardRoomSet() {
    static std::set<std::tuple<int16_t, int8_t, bool>> hazardRooms;
    return hazardRooms;
}

} // namespace

void ShuffleRoomHazards_GenerateHazardRooms() {
    auto& hazardRooms = GetHazardRoomSet();
    hazardRooms.clear();

    auto shuffleSetting = RAND_GET_OPTION(RSK_SHUFFLE_HAZARD_ROOMS);
    if (shuffleSetting.Is(RO_SHUFFLE_HAZARD_ROOMS_OFF)) {
        return;
    }

    // Every room in the eligible population (restricted to dungeons/overworld/all per the setting),
    // with vanilla heat removed from all of them until reassigned below.
    std::vector<std::pair<int16_t, int8_t>> eligibleRooms;
    for (const auto& [scene, roomCount] : kSceneRoomCounts) {
        const bool isDungeon = IsDungeonScene(scene);
        if ((shuffleSetting.Is(RO_SHUFFLE_HAZARD_ROOMS_DUNGEONS) && !isDungeon) ||
            (shuffleSetting.Is(RO_SHUFFLE_HAZARD_ROOMS_OVERWORLD) && isDungeon)) {
            continue;
        }
        for (int8_t roomNum = 0; roomNum < roomCount; roomNum++) {
            eligibleRooms.emplace_back(scene, roomNum);
        }
    }

    Shuffle(eligibleRooms);

    const int hazardRoomCount =
        std::min<int>(RAND_GET_OPTION(RSK_ADDITIONAL_HAZARD_ROOMS).Get() + 7, static_cast<int>(eligibleRooms.size()));
    for (int i = 0; i < hazardRoomCount; i++) {
        hazardRooms.insert(std::make_tuple(eligibleRooms[i].first, eligibleRooms[i].second, Random(0, 2)));
    }
}

void ShuffleRoomHazards_WriteSpoilerLog(nlohmann::ordered_json& spoilerFileJson) {
    for (const auto& [scene, roomNum, needZora] : GetHazardRoomSet()) {
        spoilerFileJson["shuffledHeatedRooms"].push_back({ { "scene", scene }, { "room", roomNum }, { "needZora", needZora } });
    }
}

void ShuffleRoomHazards_ParseSpoilerLog(const nlohmann::json& spoilerFileJson) {
    auto& hazardRooms = GetHazardRoomSet();
    hazardRooms.clear();

    nlohmann::json heatedRoomsJson = spoilerFileJson.value("shuffledHeatedRooms", nlohmann::json::array());
    for (const auto& roomJson : heatedRoomsJson) {
        hazardRooms.emplace(roomJson.value("scene", static_cast<int16_t>(SCENE_ID_MAX)),
                            roomJson.value("room", static_cast<int8_t>(0)),
                            roomJson.value("needZora", static_cast<bool>(false)));
    }
}

void ShuffleRoomHazards_OnAfterSceneCommands(int16_t sceneNum) {
    Room& curRoom = gPlayState->roomCtx.curRoom;
    const bool needGoron = GetHazardRoomSet().contains({ sceneNum, curRoom.num, false });
    const bool needZora = GetHazardRoomSet().contains({ sceneNum, curRoom.num, true });
    curRoom.behaviorType2 = needGoron ? ROOM_BEHAVIOR_TYPE2_3 : curRoom.behaviorType2;

    COND_VB_SHOULD(VB_ZORA_HAZARD_ROOM, needZora, {
        if (true) {
            *should = true;
        } else {
            *should = false;
        }
    });
}

void ShuffleRoomHazards_OnPlayDrawBegin() {
    Player* player = GET_PLAYER(gPlayState);

    if (player == nullptr) {
        return;
    }

    if (GameInteractor_Should(VB_ZORA_HAZARD_ROOM, false) && !(player->stateFlags1 & PLAYER_STATE1_IN_WATER)) {
        for (int i = 0; i < ARRAY_COUNT(gSaveContext.buttonStatus); i++) {
            gSaveContext.buttonStatus[i] = BTN_ENABLED;
        }

        InterfaceContext* interfaceCtx = &gPlayState->interfaceCtx;
        interfaceCtx->bAlpha = interfaceCtx->cLeftAlpha = interfaceCtx->cDownAlpha = interfaceCtx->cRightAlpha =
            interfaceCtx->dpadUpAlpha = interfaceCtx->dpadDownAlpha = interfaceCtx->dpadLeftAlpha =
                interfaceCtx->dpadRightAlpha = 255;
    }
}

void DrawHazardFog() {
    Player* player = GET_PLAYER(gPlayState);

    static int16_t curFogNear = gPlayState->envCtx.adjFogNear;
    static int16_t curFogFar = gPlayState->envCtx.adjFogFar;
    static int16_t curFogColor[3] = { gPlayState->envCtx.adjFogColor[0], gPlayState->envCtx.adjFogColor[1], gPlayState->envCtx.adjFogColor[2] };

    s32 i;
    f32 fogScale;
    f32 intensity = 0.2f;

    if (player->currentTunic != PLAYER_TUNIC_ZORA && GameInteractor_Should(VB_ZORA_HAZARD_ROOM, false)) {
        if (gPlayState->roomCtx.curRoom.behaviorType1 != ROOM_BEHAVIOR_TYPE1_5) {
            intensity = CLAMP_MIN(intensity, 0.0f);
            intensity = CLAMP_MAX(intensity, 1.0f);
            fogScale = intensity - 0.2f;

            if (intensity < 0.2f) {
                fogScale = 0.0f;
            }

            gPlayState->envCtx.adjFogNear = static_cast<int32_t>((850.0f - gPlayState->envCtx.lightSettings.fogNear) * fogScale);

            if (intensity == 0.0f) {
                for (i = 0; i < ARRAY_COUNT(gPlayState->envCtx.adjFogColor); i++) {
                    gPlayState->envCtx.adjFogColor[i] = 0;
                }
            } else {
                Math_StepToS(&gPlayState->envCtx.adjFogColor[0], 30, 10);
                Math_StepToS(&gPlayState->envCtx.adjFogColor[1], 85, 10);
                Math_StepToS(&gPlayState->envCtx.adjFogColor[2], 145, 10);
            }
        }
    } else if (player->currentTunic != PLAYER_TUNIC_GORON && GetHazardRoomSet().contains({ gPlayState->sceneNum, gPlayState->roomCtx.curRoom.num, false })) {
        if (gPlayState->roomCtx.curRoom.behaviorType1 != ROOM_BEHAVIOR_TYPE1_5) {
            intensity = CLAMP_MIN(intensity, 0.0f);
            intensity = CLAMP_MAX(intensity, 1.0f);
            fogScale = intensity - 0.2f;

            if (intensity < 0.2f) {
                fogScale = 0.0f;
            }

            gPlayState->envCtx.adjFogNear = static_cast<int32_t>((850.0f - gPlayState->envCtx.lightSettings.fogNear) * fogScale);

            if (intensity == 0.0f) {
                for (i = 0; i < ARRAY_COUNT(gPlayState->envCtx.adjFogColor); i++) {
                    gPlayState->envCtx.adjFogColor[i] = 0;
                }
            } else {
                Math_StepToS(&gPlayState->envCtx.adjFogColor[0], 145, 10);
                Math_StepToS(&gPlayState->envCtx.adjFogColor[1], 55, 10);
                Math_StepToS(&gPlayState->envCtx.adjFogColor[2], 30, 10);
            }
        }
    } else {
        Math_StepToS(&gPlayState->envCtx.adjFogNear, curFogNear, 10);
        Math_StepToS(&gPlayState->envCtx.adjFogFar, curFogFar, 10);
        Math_StepToS(&gPlayState->envCtx.adjFogColor[0], curFogColor[0], 10);
        Math_StepToS(&gPlayState->envCtx.adjFogColor[1], curFogColor[1], 10);
        Math_StepToS(&gPlayState->envCtx.adjFogColor[2], curFogColor[2], 10);
    }
}

void RegisterShuffleRoomHazards() {
    bool shouldRegister = IS_RANDO && RAND_GET_OPTION(RSK_SHUFFLE_HAZARD_ROOMS).IsNot(RO_SHUFFLE_HAZARD_ROOMS_OFF);
    COND_HOOK(AfterSceneCommands, shouldRegister, ShuffleRoomHazards_OnAfterSceneCommands);
    COND_HOOK(OnPlayDrawBegin, shouldRegister, ShuffleRoomHazards_OnPlayDrawBegin);
    COND_HOOK(OnPlayerUpdate, shouldRegister, DrawHazardFog);
}

static RegisterShipInitFunc registerShuffleRoomHazards(RegisterShuffleRoomHazards, { "IS_RANDO" });