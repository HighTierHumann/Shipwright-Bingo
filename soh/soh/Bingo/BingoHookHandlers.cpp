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
#include "soh/ObjectExtension/ActorListIndex.h"
#include "soh/Enhancements/randomizer/randomizer_grotto.h"

#include <algorithm>
#include <bitset>

extern "C" {
#include "macros.h"
#include "functions.h"
#include "variables.h"
#include "src/overlays/actors/ovl_En_Dns/z_en_dns.h"
#include "src/overlays/actors/ovl_En_Dodongo/z_en_dodongo.h"
#include "src/overlays/actors/ovl_En_Firefly/z_en_firefly.h"
#include "src/overlays/actors/ovl_En_Tubo_Trap/z_en_tubo_trap.h"
#include "src/overlays/actors/ovl_En_Wallmas/z_en_wallmas.h"
#include "src/overlays/actors/ovl_En_Ru1/z_en_ru1.h"
#include "src/overlays/actors/ovl_En_Wonder_Item/z_en_wonder_item.h"
#include "src/overlays/actors/ovl_Obj_Switch/z_obj_switch.h"
#include "src/overlays/actors/ovl_Obj_Bean/z_obj_bean.h"
#include "src/overlays/actors/ovl_En_Bb/z_en_bb.h"
#include "src/overlays/actors/ovl_Door_Killer/z_door_killer.h"
#include "src/overlays/actors/ovl_En_Honotrap/z_en_honotrap.h"
#include "src/overlays/actors/ovl_En_Wf/z_en_wf.h"
#include "src/overlays/actors/ovl_En_Zf/z_en_zf.h"
#include "src/overlays/actors/ovl_En_Reeba/z_en_reeba.h"
#include "src/overlays/actors/ovl_En_Gs/z_en_gs.h"
#include "src/overlays/actors/ovl_En_Go2/z_en_go2.h"
#include "src/overlays/actors/ovl_En_Ge2/z_en_ge2.h"
#include "src/overlays/actors/ovl_En_Niw/z_en_niw.h"
#include "src/overlays/actors/ovl_En_Horse/z_en_horse.h"
#include "src/overlays/actors/ovl_En_Diving_Game/z_en_diving_game.h"
#include "src/overlays/actors/ovl_En_Bom_Bowl_Man/z_en_bom_bowl_man.h"
#include "src/overlays/actors/ovl_Bg_Haka_Tubo/z_bg_haka_tubo.h"
#include "src/overlays/actors/ovl_Bg_Haka_Trap/z_bg_haka_trap.h"
#include "src/overlays/actors/ovl_Bg_Spot11_Oasis/z_bg_spot11_oasis.h"
extern PlayState* gPlayState;
extern SaveContext gSaveContext;

extern void func_808B29F0(BgSpot11Oasis* bgOasis, PlayState* play);
extern void EnGe2_CaptureTurn(EnGe2* enGe2, PlayState* play);
extern void ObjBean_Fly(ObjBean* objBean, PlayState* play);
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

typedef enum {
    KOKIRI_SHOP,
    GORON_SHOP,
    ZORA_SHOP,
    KAK_BAZAAR,
    KAK_POTION,
    MKT_BAZAAR,
    MKT_POTION,
    MKT_CHU,
} ShopTypes;

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
    if (!(GetEyeSwitchMask(sceneNum) & (1ULL << flag))) {
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

// Actor Index is a safe metric to go by for tracking this; index is set when the actor was
// placed in the scene during development, not by load-in order like I initially suspected!
static void BingoRecordBlueBubbleDeath(int16_t actorIndex, int16_t curRoom) {
    if (Flags_GetBingoInf(BINGO_INF_DEFEAT_ALL_FOREST_BLUE_BUBBLES)) {
        return;
    }

    static constexpr struct { int16_t actorIndex; int16_t actorRoom; uint8_t arrayIndex; } blueBubbles[] = {
        { 0, 0x4, 0 }, { 0, 0x15, 1 }, { 1, 0x15, 2 }, { 0, 0xB, 3 },
        { 1, 0xB, 4 }, { 2, 0xB, 5 },  { 0, 0xA, 6 }
    };
    for (const auto& bubble : blueBubbles) {
        if (curRoom == bubble.actorRoom && actorIndex == bubble.actorIndex) {
            gSaveContext.ship.bingo.blueBubblesDefeated[bubble.arrayIndex] = true;
            LUSLOG_DEBUG("Blue bubble killed in room: %d, actor index: %d", curRoom, actorIndex);
            break;
        }
    }

    auto& defeated = gSaveContext.ship.bingo.blueBubblesDefeated;
    if (std::all_of(std::begin(defeated), std::end(defeated), [](u8 bubbleDefeated) { return bubbleDefeated != 0; })) {
        Flags_SetBingoInf(BINGO_INF_DEFEAT_ALL_FOREST_BLUE_BUBBLES);
    }
}

static void BingoRecordRedBubbleDeath(int16_t sceneNum, int16_t params) {
    if (Flags_GetBingoInf(BINGO_INF_DEFEAT_2_DIFFERENT_RED_BUBBLES)) {
        return;
    }

    static constexpr struct { int16_t scene; uint8_t arrayIndex; } redBubbles[] = {
        { SCENE_DEATH_MOUNTAIN_CRATER, 0 }, { SCENE_FIRE_TEMPLE, 1 }, { SCENE_SHADOW_TEMPLE, 2 },
        { SCENE_GERUDO_TRAINING_GROUND, 3 }, { SCENE_INSIDE_GANONS_CASTLE, 4 }
    };
    for (const auto& bubble : redBubbles) {
        if (sceneNum == bubble.scene) {
            gSaveContext.ship.bingo.redBubblesDefeated[bubble.arrayIndex] = true;
            LUSLOG_DEBUG("redBubblesDefeated: %d", bubble.arrayIndex);
            break;
        }
    }

    auto& defeated = gSaveContext.ship.bingo.redBubblesDefeated;
    if (std::count_if(std::begin(defeated), std::end(defeated), [](u8 bubbleDefeated) { return bubbleDefeated != 0; }) >= 2) {
        Flags_SetBingoInf(BINGO_INF_DEFEAT_2_DIFFERENT_RED_BUBBLES);
    }
}

static void BingoRecordGreenBubbleDeath(int16_t sceneNum, int16_t actorIndex = -1) {
    if (Flags_GetBingoInf(BINGO_INF_DEFEAT_3_DIFFERENT_GREEN_BUBBLES)) {
        return;
    }
    
    static constexpr struct { int16_t scene; uint8_t arrayIndex; int16_t actorIndex; } greenBubbles[] = {
        { SCENE_SPIRIT_TEMPLE, 0 }, { SCENE_BOTTOM_OF_THE_WELL, 1 },
        { SCENE_FOREST_TEMPLE, 2, 0 }, { SCENE_FOREST_TEMPLE, 3, 1 },
        { SCENE_INSIDE_GANONS_CASTLE, 4 }
    };
    for (const auto& bubble : greenBubbles) {
        if (sceneNum == bubble.scene) {
            if (sceneNum == SCENE_FOREST_TEMPLE && actorIndex != bubble.actorIndex) {
                continue;
            }

            gSaveContext.ship.bingo.greenBubblesDefeated[bubble.arrayIndex] = true;
            LUSLOG_DEBUG("greenBubblesDefeated: %d", bubble.arrayIndex);
            break;
        }
    }

    auto& defeated = gSaveContext.ship.bingo.greenBubblesDefeated;
    if (std::count_if(std::begin(defeated), std::end(defeated), [](u8 bubbleDefeated) { return bubbleDefeated != 0; }) >= 3) {
        Flags_SetBingoInf(BINGO_INF_DEFEAT_3_DIFFERENT_GREEN_BUBBLES);
    }
}

static void BingoHandleSilverRupeeData(int16_t flagType, int16_t sceneNum, int16_t flag, class std::shared_ptr<Rando::Context> ctx) {
    auto checkSilversFlags = []() {
        auto& tracked = gSaveContext.ship.bingo.silverRupeeTracking;
        if (std::count_if(std::begin(tracked), std::begin(tracked) + 3,
                        [](u8 silverTracked) { return silverTracked != 0; }) == 3) {
            Flags_SetBingoInf(BINGO_INF_COLLECT_SHADOW_TEMPLE_SILVER_ITEMS);
        }
        if (std::count_if(std::begin(tracked) + 3, std::begin(tracked) + 7,
                        [](u8 silverTracked) { return silverTracked != 0; }) == 4) {
            Flags_SetBingoInf(BINGO_INF_COLLECT_GANONS_CASTLE_SILVER_ITEMS);
        }
        if (std::count_if(std::begin(tracked) + 7, std::begin(tracked) + 9,
                        [](u8 silverTracked) { return silverTracked != 0; }) == 2) {
            Flags_SetBingoInf(BINGO_INF_COLLECT_ICE_CAVERN_SILVER_ITEMS);
        }
    };

    if (flagType == FLAG_RANDOMIZER_INF) {
        static constexpr struct {
            int firstRandInf;
            int lastRandInf;
            uint8_t arrayIndex;
        } silverRupeeFlags[] = {
            { RAND_INF_SHADOW_TEMPLE_SCYTHE_SILVER_RUPEE_1, RAND_INF_SHADOW_TEMPLE_SCYTHE_SILVER_RUPEE_5, 0 },
            { RAND_INF_SHADOW_TEMPLE_BEAMOS_SILVER_RUPEE_1, RAND_INF_SHADOW_TEMPLE_BEAMOS_SILVER_RUPEE_5, 1 },
            { RAND_INF_SHADOW_TEMPLE_REDEAD_SILVER_RUPEE_1, RAND_INF_SHADOW_TEMPLE_REDEAD_SILVER_RUPEE_5, 2 },
            { RAND_INF_GANONS_CASTLE_FOREST_TRIAL_SILVER_RUPEE_1, RAND_INF_GANONS_CASTLE_FOREST_TRIAL_SILVER_RUPEE_5, 3 },
            { RAND_INF_GANONS_CASTLE_FIRE_TRIAL_SILVER_RUPEE_1, RAND_INF_GANONS_CASTLE_FIRE_TRIAL_SILVER_RUPEE_5, 4 },
            { RAND_INF_GANONS_CASTLE_SPIRIT_TRIAL_SILVER_RUPEE_1, RAND_INF_GANONS_CASTLE_SPIRIT_TRIAL_SILVER_RUPEE_5, 5 },
            { RAND_INF_GANONS_CASTLE_LIGHT_TRIAL_SILVER_RUPEE_1, RAND_INF_GANONS_CASTLE_LIGHT_TRIAL_SILVER_RUPEE_5, 6 },
            { RAND_INF_ICE_CAVERN_SCYTHE_SILVER_RUPEE_1, RAND_INF_ICE_CAVERN_SCYTHE_SILVER_RUPEE_5, 7 },
            { RAND_INF_ICE_CAVERN_PUSH_BLOCK_SILVER_RUPEE_1, RAND_INF_ICE_CAVERN_PUSH_BLOCK_SILVER_RUPEE_5, 8 },
        };

        for (const auto& flag : silverRupeeFlags) {
            int silverItemsCollected = 0;
            for (int i = flag.firstRandInf; i <= flag.lastRandInf; ++i) {
                if (Flags_GetRandomizerInf(static_cast<RandomizerInf>(i))) {
                    ++silverItemsCollected;
                }
            }

            if (silverItemsCollected == 5) {
                gSaveContext.ship.bingo.silverRupeeTracking[flag.arrayIndex] = true;
                LUSLOG_DEBUG("silverRupeeTracking: %d", flag.arrayIndex);
            }
        }
        checkSilversFlags();
        return;
    }

    static constexpr struct { int16_t scene; RandomizerSettingKey settingKey; uint8_t switchflag; uint8_t arrayIndex; } silversData[] = {
        { SCENE_SHADOW_TEMPLE, RSK_SILVERS_SHADOW_TEMPLE_SCYTHE, 0x1, 0 },
        { SCENE_SHADOW_TEMPLE, RSK_SILVERS_SHADOW_TEMPLE_BEAMOS, 0x9, 1 },
        { SCENE_SHADOW_TEMPLE, RSK_SILVERS_SHADOW_TEMPLE_REDEAD, 0x8, 2 },
        { SCENE_INSIDE_GANONS_CASTLE, RSK_SILVERS_FOREST_TRIAL, 0xE, 3 },
        { SCENE_INSIDE_GANONS_CASTLE, RSK_SILVERS_FIRE_TRIAL, 0x9, 4 },
        { SCENE_INSIDE_GANONS_CASTLE, RSK_SILVERS_SPIRIT_TRIAL, 0xB, 5 },
        { SCENE_INSIDE_GANONS_CASTLE, RSK_SILVERS_LIGHT_TRIAL, 0x12, 6 },
        { SCENE_ICE_CAVERN, RSK_SILVERS_ICE_CAVERN_SCYTHE, 0x8, 7 },
        { SCENE_ICE_CAVERN, RSK_SILVERS_ICE_CAVERN_PUSH_BLOCK, 0x9, 8 },
    };
    for (const auto& silver : silversData) {
        if (sceneNum == silver.scene && flag == silver.switchflag && ctx->GetOption(silver.settingKey).Is(RO_GENERIC_OFF)) {
            gSaveContext.ship.bingo.silverRupeeTracking[silver.arrayIndex] = true;
            LUSLOG_DEBUG("silverRupeeTracking: %d", silver.arrayIndex);
            break;
        }
    }
    checkSilversFlags();
}

static void BingoRecordLockedDoors(int16_t sceneNum, int16_t flag) {
    auto checkDoorFlags = []() {
        auto& tracked = gSaveContext.ship.bingo.lockedDungeonDoors;
        if (std::count_if(std::begin(tracked), std::begin(tracked) + 5,
                        [](u8 waterDoors) { return waterDoors != 0; }) == 5) {
            Flags_SetBingoInf(BINGO_INF_UNLOCK_ALL_5_LOCKED_WATER_DOORS);
        }
        if (std::count_if(std::begin(tracked) + 5, std::begin(tracked) + 10,
                        [](u8 spiritDoors) { return spiritDoors != 0; }) == 5) {
            Flags_SetBingoInf(BINGO_INF_UNLOCK_ALL_5_LOCKED_SPIRIT_DOORS);
        }
    };
    
    static constexpr struct { int16_t scene; uint8_t doorFlag; int16_t arrayIndex; } lockedDoors[] = {
        { SCENE_WATER_TEMPLE, 0x1, 0 }, { SCENE_WATER_TEMPLE, 0x2, 1 },
        { SCENE_WATER_TEMPLE, 0x5, 2 }, { SCENE_WATER_TEMPLE, 0x6, 3 },
        { SCENE_WATER_TEMPLE, 0x9, 4 },
        { SCENE_SPIRIT_TEMPLE, 0xD, 5 }, { SCENE_SPIRIT_TEMPLE, 0x15, 6 },
        { SCENE_SPIRIT_TEMPLE, 0x1B, 7 }, { SCENE_SPIRIT_TEMPLE, 0x1C, 8 },
        { SCENE_SPIRIT_TEMPLE, 0x1E, 9 }
    };
    for (const auto& door : lockedDoors) {
        if (sceneNum == door.scene && flag == door.doorFlag) {
            gSaveContext.ship.bingo.lockedDungeonDoors[door.arrayIndex] = true;
            LUSLOG_DEBUG("lockedDungeonDoors: %d", door.arrayIndex);
            break;
        }
    }
    checkDoorFlags();
}

static void BingoHandleMerchants(int16_t flag = -1, GetItemID getItemId = GI_NONE) {
    switch(flag) {
        case RAND_INF_MERCHANTS_MEDIGORON:
            gSaveContext.ship.bingo.merchantsBoughtFrom[0] = true;
            break;
        case RAND_INF_MERCHANTS_MAGIC_BEAN_SALESMAN:
            Flags_SetBingoInf(BINGO_INF_BEAN_GUY_ITEM);
            gSaveContext.ship.bingo.merchantsBoughtFrom[1] = true;
            break;
        case RAND_INF_MERCHANTS_CARPET_SALESMAN:
            Flags_SetBingoInf(BINGO_INF_CARPET_SALESMAN_ITEM);
            gSaveContext.ship.bingo.merchantsBoughtFrom[3] = true;
            break;
        default:
            break;
    }

    switch(getItemId) {
        case GI_SWORD_KNIFE:
            gSaveContext.ship.bingo.merchantsBoughtFrom[0] = true;
            break;
        case GI_BEAN:
            gSaveContext.ship.bingo.merchantsBoughtFrom[1] = true;
            break;
        case GI_POTION_BLUE:
            gSaveContext.ship.bingo.merchantsBoughtFrom[2] = true;
            break;
        case GI_BOMBCHUS_10:
            gSaveContext.ship.bingo.merchantsBoughtFrom[3] = true;
            break;
        default:
            break;
    }

    auto& tracked = gSaveContext.ship.bingo.merchantsBoughtFrom;
    if (std::count_if(std::begin(tracked), std::end(tracked),
                    [](u8 merchants) { return merchants != 0; }) >= 3) {
        Flags_SetBingoInf(BINGO_INF_BUY_FROM_3_MERCHANTS);
    }
}

static void BingoRecordScrubPurchase(EnDns* enDns) {
    s32 params = enDns->actor.params;
    // Scrubs that are 0x06 are loaded as 0x03 when child, switching from selling arrows to seeds
    if (params == 0x06) {
        params = 0x03;
    }
    if (gPlayState->sceneNum == SCENE_GROTTOS) {
        s16 respawnData = gSaveContext.respawn[RESPAWN_MODE_RETURN].data & ((1 << 8) - 1);
        params = TWO_ACTOR_PARAMS(params, respawnData);
    }

    Rando::Location* location =
        OTRGlobals::Instance->gRandomizer->GetCheckObjectFromActor(ACTOR_EN_DNS, gPlayState->sceneNum, params);
    auto it = rcToRandomizerInf.find(location->GetRandomizerCheck());
    if (it == rcToRandomizerInf.end() || it->second < 13 || it->second > 58) {
        return;
    }

    gSaveContext.ship.bingo.scrubsPurchasedFrom[it->second - 13] = true;

    auto& spoken = gSaveContext.ship.bingo.dcScrubsSpoken;
    if (std::count_if(std::begin(spoken), std::end(spoken), [](u8 playerSpokeTo) { return playerSpokeTo != 0; }) >= 10) {
        Flags_SetBingoInf(BINGO_INF_BUY_THE_WARES_OF_10_SCRUBS);
    }
}

static void BingoHandleShops(int16_t flag) {
    constexpr int kSlotsPerShop = 8;
    const int shopStart = RAND_INF_SHOP_ITEMS_KF_SHOP_ITEM_1 +
                          ((flag - RAND_INF_SHOP_ITEMS_KF_SHOP_ITEM_1) / kSlotsPerShop) * kSlotsPerShop;

    int itemsBought = 0;
    int leftSideShop = 0;
    for (int slot = 0; slot < kSlotsPerShop; slot++) {
        if (Flags_GetRandomizerInf(static_cast<RandomizerInf>(shopStart + slot))) {
            itemsBought++;
            if (slot >= 4) {
                leftSideShop++;
            }
        }
    }
    if (leftSideShop == 4) {
        Flags_SetBingoInf(BINGO_INF_BUY_LEFT_SIDE_OF_SHOP);
    }
    if (itemsBought >= 4) {
        Flags_SetBingoInf(BINGO_INF_BUY_4_ITEMS_IN_ONE_SHOP);
    }
}

static void BingoHandleSariaActors() {
    if (Flags_GetRandomizerInf(RAND_INF_DARUNIAS_JOY) &&
        Flags_GetEventChkInf(EVENTCHKINF_PLAYED_SARIAS_SONG_FOR_MIDO_AS_ADULT) &&
        Flags_GetItemGetInf(22)) { // 22 is Skull Kid check
        Flags_SetBingoInf(BINGO_INF_3_SARIAS_SONG_ACTORS);
    }
}

static void BingoHandleSuperFairies(int16_t flag) {
    static constexpr struct { RandomizerInf randInf; int16_t arrayIndex; } superFairies[] = {
        { RAND_INF_LH_ISLAND_SUN_FAIRY, 0 }, { RAND_INF_HF_POND_STORMS_FAIRY, 1 },
        { RAND_INF_DMT_FLAG_SUN_FAIRY, 2 }, { RAND_INF_DMT_COW_GROTTO_STORMS_FAIRY, 3 },
        { RAND_INF_HF_FENCE_GROTTO_STORMS_FAIRY, 4 }, { RAND_INF_LW_SHORTCUT_STORMS_FAIRY, 5 },
        { RAND_INF_TH_KITCHEN_SUN_FAIRY, 6 }, { RAND_INF_LW_DEKU_SCRUB_GROTTO_SUN_FAIRY, 7 },
        { RAND_INF_GRAVEYARD_ROYAL_FAMILYS_TOMB_SUN_FAIRY, 8 },

        { RAND_INF_SPIRIT_TEMPLE_BOULDER_ROOM_SUN_FAIRY, 9 }, { RAND_INF_SPIRIT_TEMPLE_ARMOS_ROOM_SUN_FAIRY, 10 },
        { RAND_INF_SHADOW_TEMPLE_BEAMOS_STORM_FAIRY, 11 }, { RAND_INF_SHADOW_TEMPLE_PIT_STORM_FAIRY, 12 },
        { RAND_INF_SHADOW_TEMPLE_WIND_HINT_SUN_FAIRY, 13 }, { RAND_INF_BOTTOM_OF_THE_WELL_BASEMENT_SUN_FAIRY, 14 },
        { RAND_INF_ICE_CAVERN_ENTRANCE_STORMS_FAIRY, 15 }, { RAND_INF_GERUDO_TRAINING_GROUND_ENTRANCE_STORMS_FAIRY, 16 },
        { RAND_INF_GANONS_CASTLE_SPIRIT_TRIAL_SUN_FAIRY, 17 }
    };

    for (const auto& fairy : superFairies) {
        if (flag == fairy.randInf) {
            gSaveContext.ship.bingo.superFairies[fairy.arrayIndex] = true;
        }
    }

    auto& superFairy = gSaveContext.ship.bingo.superFairies;
    if (std::count_if(std::begin(superFairy), std::end(superFairy), [](u8 fairyCollected) { return fairyCollected != 0; }) >= 3) {
        Flags_SetBingoInf(BINGO_INF_3_SUPER_FAIRIES_COLLECTED);
    }
    if (std::count_if(std::begin(superFairy) + 9, std::end(superFairy), [](u8 fairyCollected) { return fairyCollected != 0; }) >= 2) {
        Flags_SetBingoInf(BINGO_INF_2_DIFFERENT_DUNGEON_SUPER_FAIRIES);
    }
    if (Flags_GetRandomizerInf(RAND_INF_HF_POND_STORMS_FAIRY) &&
        Flags_GetRandomizerInf(RAND_INF_HF_FENCE_GROTTO_STORMS_FAIRY)) {
        Flags_SetBingoInf(BINGO_INF_BOTH_FIELD_SUPER_FAIRIES);
    }
}

static void BingoHandleBeanSpots(int16_t sceneNum, int16_t roomNum = -1, bool beanJourney = false) {
    static constexpr struct {
        int16_t scene;
        int16_t room;
        int16_t switchFlag;
        int16_t arrayIndex;
    } beanSpots[] = {
        { SCENE_GRAVEYARD,             -1,  3, 0 },
        { SCENE_ZORAS_RIVER,           -1,  3, 1 },
        { SCENE_KOKIRI_FOREST,         -1,  9, 2 },
        { SCENE_LAKE_HYLIA,            -1,  1, 3 },
        { SCENE_GERUDO_VALLEY,         -1,  3, 4 },
        { SCENE_LOST_WOODS,             5,  4, 5 },
        { SCENE_LOST_WOODS,             6, 18, 6 },
        { SCENE_DESERT_COLOSSUS,       -1, 24, 7 },
        { SCENE_DEATH_MOUNTAIN_TRAIL,  -1,  6, 8 },
        { SCENE_DEATH_MOUNTAIN_CRATER, -1,  3, 9 },
    };

    if (std::none_of(std::begin(beanSpots), std::end(beanSpots),
        [sceneNum](const auto& beanSpot) { return beanSpot.scene == sceneNum; })) {
        return;
    }

    int beansPlanted = 0;
    for (const auto& beanSpot : beanSpots) {
        if (gSaveContext.sceneFlags[beanSpot.scene].swch & (1 << beanSpot.switchFlag)) {
            beansPlanted++;
        }
    }

    if (beansPlanted >= 5) Flags_SetBingoInf(BINGO_INF_PLANT_5_BEANS);
    if (gSaveContext.sceneFlags[SCENE_ZORAS_RIVER].swch & (1 << 3)) Flags_SetBingoInf(BINGO_INF_PLANT_A_BEAN_NEAR_BEAN_GUY);
    if (gSaveContext.sceneFlags[SCENE_DEATH_MOUNTAIN_CRATER].swch & (1 << 3)) Flags_SetBingoInf(BINGO_INF_PLANT_THE_DMC_BEAN);

    if (!beanJourney) return;

    for (const auto& beanSpot : beanSpots) {
        if (sceneNum == beanSpot.scene && roomNum == beanSpot.room) {
            gSaveContext.ship.bingo.beanJourneysCompleted[beanSpot.arrayIndex] = true;
        }
    }

    auto& journeysComplete = gSaveContext.ship.bingo.beanJourneysCompleted;
    if (std::count_if(std::begin(journeysComplete), std::end(journeysComplete),
        [](u8 journeyCompleted) { return journeyCompleted != 0; }) >= 5) {
        Flags_SetBingoInf(BINGO_INF_ACTIVATE_5_BEANFLOWERS);
    }
}

static void BingoHandleAdultTrades(int16_t flag) {
    static constexpr struct { int16_t itemGetInf; int16_t randomizerInf; int16_t arrayIndex; } tradeQuests[] = {
        { 0x2E, -1, 0 }, // Trade Pocket Cucco
        { 0x30, -1, 1 }, // Trade Odd Mushroom
        { 0x31, -1, 2 }, // Trade Odd Potion
        { -1, RAND_INF_ADULT_TRADES_LW_TRADE_COJIRO, 3 },
        { -1, RAND_INF_ADULT_TRADES_GV_TRADE_SAW, 4 },
        { -1, RAND_INF_ADULT_TRADES_DMT_TRADE_BROKEN_SWORD, 5 },
        { -1, RAND_INF_ADULT_TRADES_ZD_TRADE_PRESCRIPTION, 6 },
        { -1, RAND_INF_ADULT_TRADES_LH_TRADE_FROG, 7 },
        { -1, RAND_INF_ADULT_TRADES_DMT_TRADE_EYEDROPS, 8 },
        { -1, RAND_INF_ADULT_TRADES_DMT_TRADE_CLAIM_CHECK, 9 },
    };

    int tradesComplete = 0;
    for (const auto& tradeQuest : tradeQuests) {
        if (tradeQuest.itemGetInf != -1) {
            if (Flags_GetItemGetInf(tradeQuest.itemGetInf)) {
                tradesComplete++;
            }
        }
        if (tradeQuest.randomizerInf != -1) {
            if (Flags_GetRandomizerInf(static_cast<RandomizerInf>(tradeQuest.randomizerInf))) {
                tradesComplete++;
            }
        }
    }

    if (Flags_GetRandomizerInf(RAND_INF_ADULT_TRADES_DMT_TRADE_BROKEN_SWORD) &&
        Flags_GetRandomizerInf(RAND_INF_ADULT_TRADES_DMT_TRADE_EYEDROPS) &&
        Flags_GetRandomizerInf(RAND_INF_ADULT_TRADES_DMT_TRADE_CLAIM_CHECK)) {
            Flags_SetBingoInf(BINGO_INF_ALL_BIGGORON_TRADES);
    }
    if (tradesComplete >= 3) Flags_SetBingoInf(BINGO_INF_3_ADULT_TRADE_CHECKS);
}

static void BingoHandleGrottoEntry(int16_t sceneNum) {
    static constexpr int8_t GRAVE_FAIRY_FOUNTAIN_ID = 33;
    static constexpr int8_t stormsGrottos[] = { 2, 8, 12, 27, 28, 29, 30 };
    static constexpr int8_t fairyFountains[] = { 3, 15, 24, 28, 29, GRAVE_FAIRY_FOUNTAIN_ID };

    if (sceneNum != SCENE_GROTTOS && sceneNum != SCENE_FAIRYS_FOUNTAIN &&
        sceneNum != SCENE_GRAVE_WITH_FAIRYS_FOUNTAIN) {
        return;
    }

    auto& grottosEntered = gSaveContext.ship.bingo.grottosEntered;
    int8_t grottoID = sceneNum == SCENE_GRAVE_WITH_FAIRYS_FOUNTAIN ? GRAVE_FAIRY_FOUNTAIN_ID : Grotto_CurrentGrotto();
    if (grottoID >= 0 && grottoID < ARRAY_COUNT(grottosEntered)) {
        grottosEntered[grottoID] = true;
    }

    auto countEntered = [&](const auto& ids) {
        int count = 0;
        for (int8_t id : ids) {
            if (grottosEntered[id]) {
                count++;
            }
        }
        return count;
    };

    if (countEntered(stormsGrottos) >= 3) {
        Flags_SetBingoInf(BINGO_INF_ENTER_3_STORMS_GROTTOS);
    }
    if (countEntered(fairyFountains) >= 4) {
        Flags_SetBingoInf(BINGO_INF_ENTER_4_FAIRY_FOUNTAIN_GROTTOS);
    }
    if (grottosEntered[10] && grottosEntered[11]) {
        Flags_SetBingoInf(BINGO_INF_ENTER_BOTH_KAKARIKO_GROTTOS);
    }
}

// Skulltula flags live in gSaveContext.gsFlags: one byte per area, one bit per skulltula
// (area = (actor params & 0x1F00) >> 8, bit = actor params & 0xFF).
static void BingoHandleSkulltulaTokens() {
    enum GsArea : uint8_t {
        GS_AREA_DEKU_TREE = 0,
        GS_AREA_DODONGOS_CAVERN = 1,
        GS_AREA_JABU_JABU = 2,
        GS_AREA_FOREST_TEMPLE = 3,
        GS_AREA_FIRE_TEMPLE = 4,
        GS_AREA_WATER_TEMPLE = 5,
        GS_AREA_SPIRIT_TEMPLE = 6,
        GS_AREA_SHADOW_TEMPLE = 7,
        GS_AREA_ICE_CAVERN = 9,
        GS_AREA_HYRULE_FIELD = 10,
        GS_AREA_LON_LON_RANCH = 11,
        GS_AREA_KOKIRI_FOREST = 12,
        GS_AREA_LOST_WOODS = 13,
        GS_AREA_MARKET_CASTLE = 14,
        GS_AREA_DEATH_MOUNTAIN = 15,
        GS_AREA_KAKARIKO = 16,
        GS_AREA_ZORAS = 17,
        GS_AREA_LAKE_HYLIA = 18,
        GS_AREA_GERUDO_VALLEY = 19,
        GS_AREA_GERUDO_FORTRESS = 20,
        GS_AREA_DESERT_COLOSSUS = 21,
    };

    // Every bit in mask must be set
    static constexpr struct { BingoInf bingoInf; GsArea area; uint8_t mask; } groups[] = {
        { BINGO_INF_ALL_4_LOST_WOODS_AREA_SKULLS, GS_AREA_LOST_WOODS, 0x0F },
        { BINGO_INF_ALL_8_KAKARIKO_AREA_SKULLS, GS_AREA_KAKARIKO, 0xFF },
        { BINGO_INF_ALL_5_LAKE_HYLIA_SKULLS, GS_AREA_LAKE_HYLIA, 0x1F },
        { BINGO_INF_ALL_4_RANCH_SKULLS, GS_AREA_LON_LON_RANCH, 0x0F },
        { BINGO_INF_ALL_8_ZORA_AREA_SKULLS, GS_AREA_ZORAS, 0xFF },
        { BINGO_INF_ALL_3_KOKIRI_FOREST_SKULLS, GS_AREA_KOKIRI_FOREST, 0x07 },
        { BINGO_INF_ALL_4_DEKU_TREE_SKULLS, GS_AREA_DEKU_TREE, 0x0F },
        { BINGO_INF_ALL_5_DODONGO_CAVERN_SKULLS, GS_AREA_DODONGOS_CAVERN, 0x1F },
        { BINGO_INF_ALL_4_JABU_JABU_SKULLS, GS_AREA_JABU_JABU, 0x0F },
        { BINGO_INF_ALL_5_FOREST_TEMPLE_SKULLS, GS_AREA_FOREST_TEMPLE, 0x1F },
        { BINGO_INF_ALL_5_FIRE_TEMPLE_SKULLS, GS_AREA_FIRE_TEMPLE, 0x1F },
        { BINGO_INF_ALL_3_ICE_CAVERN_SKULLS, GS_AREA_ICE_CAVERN, 0x07 },
        { BINGO_INF_ALL_5_WATER_TEMPLE_SKULLS, GS_AREA_WATER_TEMPLE, 0x1F },
        { BINGO_INF_ALL_5_SPIRIT_TEMPLE_SKULLS, GS_AREA_SPIRIT_TEMPLE, 0x1F },
        { BINGO_INF_ALL_5_SHADOW_TEMPLE_SKULLS, GS_AREA_SHADOW_TEMPLE, 0x1F },
        { BINGO_INF_BOTH_DMC_SKULLS, GS_AREA_DEATH_MOUNTAIN, 0x81 }, // bean patch + crate
        { BINGO_INF_BOTH_SCARECROW_SKULLS_IN_FIRE_TEMPLE, GS_AREA_FIRE_TEMPLE, 0x18 },
        { BINGO_INF_BOTH_FORTRESS_SKULLS, GS_AREA_GERUDO_FORTRESS, 0x03 },
        { BINGO_INF_BOTH_FIELD_SKULLS, GS_AREA_HYRULE_FIELD, 0x03 },
        { BINGO_INF_BOTH_CHILD_CASTLE_SKULLS, GS_AREA_MARKET_CASTLE, 0x06 }, // storms grotto + tree
    };

    for (const auto& group : groups) {
        if ((GET_GS_FLAGS(group.area) & group.mask) == group.mask) {
            Flags_SetBingoInf(group.bingoInf);
        }
    }

    static constexpr struct { GsArea area; uint8_t mask; } softSoilSkulls[] = {
        { GS_AREA_KOKIRI_FOREST, 0x01 },
        { GS_AREA_LOST_WOODS, 0x01 },
        { GS_AREA_LOST_WOODS, 0x02 },
        { GS_AREA_KAKARIKO, 0x01 }, // graveyard
        { GS_AREA_DEATH_MOUNTAIN, 0x01 }, // crater
        { GS_AREA_DEATH_MOUNTAIN, 0x02 }, // trail
        { GS_AREA_LAKE_HYLIA, 0x01 },
        { GS_AREA_GERUDO_VALLEY, 0x01 },
        { GS_AREA_DESERT_COLOSSUS, 0x01 },
    };

    int softSoilCollected = 0;
    for (const auto& skull : softSoilSkulls) {
        if (GET_GS_FLAGS(skull.area) & skull.mask) {
            softSoilCollected++;
        }
    }
    if (softSoilCollected >= 4) {
        Flags_SetBingoInf(BINGO_INF_4_SOFT_SOIL_SKULLS);
    }
}

static bool BingoIsCheckCollected(RandomizerCheck rc) {
    const Rando::SpoilerCollectionCheck check = Rando::StaticData::GetLocation(rc)->GetCollectionCheck();
    switch (check.type) {
        case SPOILER_CHK_ITEM_GET_INF:
            return Flags_GetItemGetInf(check.flag);
        case SPOILER_CHK_EVENT_CHK_INF:
            return Flags_GetEventChkInf(check.flag);
        case SPOILER_CHK_RANDOMIZER_INF:
            return Flags_GetRandomizerInf(static_cast<RandomizerInf>(check.flag));
        case SPOILER_CHK_CHEST:
            return (gSaveContext.sceneFlags[check.scene].chest & (1 << check.flag)) != 0;
        case SPOILER_CHK_GOLD_SKULLTULA:
            return (GET_GS_FLAGS(check.scene) & check.flag) != 0;
        default:
            return false;
    }
}

static void BingoHandleCollectChecksSquares() {
    struct AreaCheck {
        RandomizerCheck rc;
        bool alwaysShuffled;
    };

    static constexpr AreaCheck labChecks[] = {
        { RC_LH_LAB_DIVE, true },
        { RC_LH_TRADE_FROG, true },
        { RC_LH_GS_LAB_CRATE, true },
        { RC_LH_LAB_FRONT_RUPEE, false },
        { RC_LH_LAB_LEFT_RUPEE, false },
        { RC_LH_LAB_RIGHT_RUPEE, false },
        { RC_LH_LAB_CRATE, false },
    };
    static constexpr AreaCheck tombChecks[] = {
        { RC_SONG_FROM_ROYAL_FAMILYS_TOMB, true },
        { RC_GRAVEYARD_ROYAL_FAMILYS_TOMB_CHEST, true },
        { RC_GRAVEYARD_ROYAL_FAMILYS_TOMB_SUN_FAIRY, false },
    };
    static constexpr AreaCheck goronCityChecks[] = {
        { RC_GC_GS_BOULDER_MAZE, true },
        { RC_GC_MAZE_LEFT_CHEST, true },
        { RC_GC_MAZE_RIGHT_CHEST, true },
        { RC_GC_MAZE_CENTER_CHEST, true },
        { RC_GC_MAZE_CRATE, false },
        { RC_GC_MAZE_GOSSIP_STONE_FAIRY, false },
        { RC_GC_MAZE_GOSSIP_STONE_FAIRY_BIG, false },
        { RC_GC_MAZE_SILVER_BOULDER_1, false },
        { RC_GC_MAZE_SILVER_BOULDER_2, false },
        { RC_GC_MAZE_SILVER_BOULDER_3, false },
        { RC_GC_MAZE_SILVER_BOULDER_4, false },
        { RC_GC_MAZE_SILVER_BOULDER_5, false },
        { RC_GC_MAZE_SILVER_BOULDER_6, false },
        { RC_GC_MAZE_SILVER_BOULDER_7, false },
        { RC_GC_MAZE_SILVER_BOULDER_8, false },
        { RC_GC_MAZE_SILVER_BOULDER_9, false },
        { RC_GC_MAZE_SILVER_BOULDER_10, false },
        { RC_GC_MAZE_SILVER_BOULDER_11, false },
        { RC_GC_MAZE_SILVER_BOULDER_12, false },
        { RC_GC_MAZE_SILVER_BOULDER_13, false },
        { RC_GC_MAZE_SILVER_BOULDER_14, false },
        { RC_GC_MAZE_SILVER_BOULDER_15, false },
        { RC_GC_MAZE_SILVER_BOULDER_16, false },
        { RC_GC_MAZE_SILVER_BOULDER_17, false },
        { RC_GC_MAZE_SILVER_BOULDER_18, false },
        { RC_GC_MAZE_SILVER_BOULDER_19, false },
        { RC_GC_MAZE_SILVER_BOULDER_20, false },
        { RC_GC_MAZE_SILVER_BOULDER_21, false },
        { RC_GC_MAZE_SILVER_BOULDER_22, false },
        { RC_GC_MAZE_SILVER_BOULDER_23, false },
        { RC_GC_MAZE_SILVER_BOULDER_24, false },
        { RC_GC_MAZE_SILVER_BOULDER_25, false },
        { RC_GC_MAZE_SILVER_BOULDER_26, false },
        { RC_GC_MAZE_SILVER_BOULDER_27, false },
        { RC_GC_MAZE_SILVER_BOULDER_28, false },
        { RC_GC_MAZE_SILVER_BOULDER_29, false },
        { RC_GC_MAZE_BOULDER_1, false },
        { RC_GC_MAZE_BOULDER_2, false },
        { RC_GC_MAZE_BOULDER_3, false },
        { RC_GC_MAZE_BOULDER_4, false },
        { RC_GC_MAZE_BOULDER_5, false },
        { RC_GC_MAZE_BOULDER_6, false },
        { RC_GC_MAZE_BOULDER_7, false },
        { RC_GC_MAZE_BOULDER_8, false },
        { RC_GC_MAZE_BOULDER_9, false },
        { RC_GC_MAZE_BOULDER_10, false },
        { RC_GC_MAZE_BRONZE_BOULDER_1, false },
        { RC_GC_MAZE_BRONZE_BOULDER_2, false },
        { RC_GC_MAZE_BRONZE_BOULDER_3, false },
        { RC_GC_MAZE_BRONZE_BOULDER_4, false },
        { RC_GC_MAZE_BRONZE_BOULDER_5, false },
        { RC_GC_MAZE_ROCK, false },
    };

    const auto allCollected = [](const auto& checks) {
        for (const AreaCheck& check : checks) {
            if (!check.alwaysShuffled && !CheckTracker::IsCheckShuffled(check.rc)) {
                continue;
            }
            if (!BingoIsCheckCollected(check.rc)) {
                return false;
            }
        }
        return true;
    };

    if (allCollected(labChecks)) {
        Flags_SetBingoInf(BINGO_INF_COLLECT_ALL_LAKESIDE_LAB_CHECKS);
    }
    if (allCollected(tombChecks)) {
        Flags_SetBingoInf(BINGO_INF_COLLECT_ALL_ROYAL_TOMB_CHECKS);
    }
    if (allCollected(goronCityChecks)) {
        Flags_SetBingoInf(BINGO_INF_COLLECT_ALL_GORON_MAZE_CHECKS);
    }
}

void BingoOnFlagSetHandler(int16_t flagType, int16_t flag) {
    if (gPlayState == nullptr) return;

    if (gPlayState->sceneNum == SCENE_LAKESIDE_LABORATORY || gPlayState->sceneNum == SCENE_ROYAL_FAMILYS_TOMB ||
        gPlayState->sceneNum == SCENE_GORON_CITY) {
        BingoHandleCollectChecksSquares();
    }

    Player* player = GET_PLAYER(gPlayState);
    auto ctx = Rando::Context::GetInstance();

    if (flagType == FLAG_EVENT_CHECK_INF) {
        switch (flag) {
            case EVENTCHKINF_LEARNED_PRELUDE_OF_LIGHT:
                Flags_SetBingoInf(BINGO_INF_PRELUDE_CHECK);
                break;
            case EVENTCHKINF_BONGO_BONGO_ESCAPED_FROM_WELL:
                Flags_SetBingoInf(BINGO_INF_NOCTURNE_CHECK);
                break;
            case EVENTCHKINF_DISPELLED_GANONS_TOWER_BARRIER:
                Flags_SetBingoInf(BINGO_INF_DISABLE_THE_BARRIER);
                break;
        }
    }
    if (flagType == FLAG_GS_TOKEN) {
        BingoHandleSkulltulaTokens();
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
        if (Flags_GetRandomizerInf(RAND_INF_COWS_MILKED_GV_COW) &&
            Flags_GetRandomizerInf(RAND_INF_COWS_MILKED_DMT_COW_GROTTO_COW)) {
            Flags_SetBingoInf(BINGO_INF_VALLEY_AND_TRAIL_COW_CHECKS);
        }
        if (Flags_GetRandomizerInf(RAND_INF_COWS_MILKED_HF_COW_GROTTO_COW) &&
            Flags_GetRandomizerInf(RAND_INF_COWS_MILKED_KAK_IMPAS_HOUSE_COW)) {
            Flags_SetBingoInf(BINGO_INF_FIELD_AND_KAKARIKO_COW_CHECKS);
        }

        if (flag >= RAND_INF_SPIRIT_TEMPLE_BOULDER_SILVER_RUPEE_1 &&
            flag <= RAND_INF_BOTTOM_OF_THE_WELL_BASEMENT_SILVER_RUPEE_5) {
            BingoHandleSilverRupeeData(flagType, gPlayState->sceneNum, flag, ctx);
        }

        if (Flags_GetRandomizerInf(RAND_INF_HAS_SKELETON_KEY)) {
            Flags_SetBingoInf(BINGO_INF_UNLOCK_ALL_5_LOCKED_WATER_DOORS);
            Flags_SetBingoInf(BINGO_INF_UNLOCK_ALL_5_LOCKED_SPIRIT_DOORS);
        }

        if (flag >= RAND_INF_SHOP_ITEMS_KF_SHOP_ITEM_1 && flag <= RAND_INF_SHOP_ITEMS_MARKET_BOMBCHU_SHOP_ITEM_8) {
            BingoHandleShops(flag);
        }

        if ((flag >= RAND_INF_LH_ISLAND_SUN_FAIRY && flag <= RAND_INF_GRAVEYARD_ROYAL_FAMILYS_TOMB_SUN_FAIRY) ||
            (flag >= RAND_INF_SPIRIT_TEMPLE_BOULDER_ROOM_SUN_FAIRY && flag <= RAND_INF_GANONS_CASTLE_SPIRIT_TRIAL_SUN_FAIRY)) {
            BingoHandleSuperFairies(flag);
        }

        if (flag >= RAND_INF_COWS_MILKED_KF_LINKS_HOUSE_COW &&
            flag <= RAND_INF_COWS_MILKED_GV_COW) {
            int cowsMilked = 0;
            for (int i = RAND_INF_COWS_MILKED_KF_LINKS_HOUSE_COW; i <= RAND_INF_COWS_MILKED_GV_COW; i++) {
                if (Flags_GetRandomizerInf(static_cast<RandomizerInf>(i))) {
                    cowsMilked++;
                }
            }
            if (cowsMilked >= 6) {
                Flags_SetBingoInf(BINGO_INF_6_DIFFERENT_COW_CHECKS);
            }
        }

        int greatFairies = 0;
        switch(flag) {
            case RAND_INF_10_BIG_POES:
                Flags_SetBingoInf(BINGO_INF_COLLECT_POE_COLLECTOR_CHECK);
                break;
            case RAND_INF_MERCHANTS_CARPET_SALESMAN:
            case RAND_INF_MERCHANTS_MAGIC_BEAN_SALESMAN:
            case RAND_INF_MERCHANTS_MEDIGORON:
                BingoHandleMerchants(flag);
                break;
            case RAND_INF_COWS_MILKED_KF_LINKS_HOUSE_COW:
                Flags_SetBingoInf(BINGO_INF_COW_IN_HOUSE_CHECK);
                break;
            case RAND_INF_DARUNIAS_JOY:
                BingoHandleSariaActors();
                break;
            case RAND_INF_ZF_GREAT_FAIRY_REWARD:
            case RAND_INF_COLOSSUS_GREAT_FAIRY_REWARD:
            case RAND_INF_DMT_GREAT_FAIRY_REWARD:
            case RAND_INF_DMC_GREAT_FAIRY_REWARD:
            case RAND_INF_HC_GREAT_FAIRY_REWARD:
            case RAND_INF_OGC_GREAT_FAIRY_REWARD:
                if (Flags_GetRandomizerInf(RAND_INF_HC_GREAT_FAIRY_REWARD) &&
                    Flags_GetRandomizerInf(RAND_INF_OGC_GREAT_FAIRY_REWARD)) {
                    Flags_SetBingoInf(BINGO_INF_LULLABY_FOR_OGC_AND_HC_FAIRIES);
                }
                for (int i = RAND_INF_HC_GREAT_FAIRY_REWARD; i <= RAND_INF_OGC_GREAT_FAIRY_REWARD; i++) {
                    if (Flags_GetRandomizerInf(static_cast<RandomizerInf>(i))) {
                        greatFairies++;
                    }
                }
                if (greatFairies >= 4) {
                    Flags_SetBingoInf(BINGO_INF_LULLABY_FOR_4_GREAT_FAIRIES);
                }
                break;
            case RAND_INF_ADULT_TRADES_LW_TRADE_COJIRO:
            case RAND_INF_ADULT_TRADES_GV_TRADE_SAW:
            case RAND_INF_ADULT_TRADES_DMT_TRADE_BROKEN_SWORD:
            case RAND_INF_ADULT_TRADES_ZD_TRADE_PRESCRIPTION:
            case RAND_INF_ADULT_TRADES_LH_TRADE_FROG:
            case RAND_INF_ADULT_TRADES_DMT_TRADE_EYEDROPS:
            case RAND_INF_ADULT_TRADES_DMT_TRADE_CLAIM_CHECK:
                BingoHandleAdultTrades(flag);
                break;
            default:
                break;
        }
    }

    if (flagType == FLAG_BINGO_INF) {
        int bossesDefeated = 0;
        int templeBosses = 0;
        switch (flag) {
            case BINGO_INF_DEFEAT_DEAD_HAND_BOTTOM_OF_THE_WELL:
            case BINGO_INF_DEFEAT_DEAD_HAND_SHADOW_TEMPLE:
                if (Flags_GetBingoInf(BINGO_INF_DEFEAT_DEAD_HAND_BOTTOM_OF_THE_WELL) &&
                    Flags_GetBingoInf(BINGO_INF_DEFEAT_DEAD_HAND_SHADOW_TEMPLE)) {
                    Flags_SetBingoInf(BINGO_INF_DEFEAT_BOTH_DEAD_HANDS);
                }
                break;
            case BINGO_INF_DEFEAT_PHANTOM_GANON:
            case BINGO_INF_DEFEAT_VOLVAGIA:
            case BINGO_INF_DEFEAT_MORPHA:
            case BINGO_INF_DEFEAT_TWINROVA:
            case BINGO_INF_DEFEAT_BONGO_BONGO:
                for (int i = BINGO_INF_DEFEAT_GOHMA; i <= BINGO_INF_DEFEAT_BONGO_BONGO; i++) {
                    if (i >= BINGO_INF_DEFEAT_PHANTOM_GANON) {
                        templeBosses++;
                    }
                    if (Flags_GetBingoInf(static_cast<BingoInf>(i))) {
                        bossesDefeated++;
                    }
                }
                if (bossesDefeated >= 3) {
                    Flags_SetBingoInf(BINGO_INF_DEFEAT_3_BOSSES);
                }
                if (bossesDefeated >= 5) {
                    Flags_SetBingoInf(BINGO_INF_DEFEAT_5_BOSSES);
                }
                if (templeBosses >= 3) {
                    Flags_SetBingoInf(BINGO_INF_DEFEAT_3_TEMPLE_BOSSES);
                }
                break;
            default:
                break;
        }
    }

    if (flagType == FLAG_INF_TABLE) {
        if (Flags_GetInfTable(INFTABLE_11E) && Flags_GetInfTable(INFTABLE_GORON_CITY_DOORS_UNLOCKED)) {
            Flags_SetBingoInf(BINGO_INF_BOTH_ROLLING_GORON_CHECKS);
        }

        if (flag == INFTABLE_138) Flags_SetBingoInf(BINGO_INF_THAW_KING_ZORA);
    }

    if (flagType == FLAG_ITEM_GET_INF) {
        switch (flag) {
            case 27:
                Flags_SetBingoInf(BINGO_INF_WIN_CHEST_MINIGAME);
                break;
            case 0x2E: // Trade Pocket Cucco
            case 0x30: // Trade Odd Mushroom
            case 0x31: // Trade Odd Potion
                BingoHandleAdultTrades(flag);
                break;
            default:
                break;
        }
    }

    if (flagType == FLAG_EVENT_CHECK_INF) {
        if (flag >= EVENTCHKINF_SONGS_FOR_FROGS_ZL && flag <= EVENTCHKINF_SONGS_FOR_FROGS_STORMS) {
            int frogsEnlarged = 0;
            for (int i = EVENTCHKINF_SONGS_FOR_FROGS_ZL; i < EVENTCHKINF_SONGS_FOR_FROGS_STORMS; ++i) {
                if (Flags_GetEventChkInf(i)) {
                    ++frogsEnlarged;
                }
            }
            if (frogsEnlarged >= 4) {
                Flags_SetBingoInf(BINGO_INF_ENLARGE_4_FROGS);
            }
            if (Flags_GetEventChkInf(EVENTCHKINF_SONGS_FOR_FROGS_STORMS)) {
                Flags_SetBingoInf(BINGO_INF_FROGS_STORMS_CHECK);
            }
        }
        if (flag == EVENTCHKINF_OFFERED_FISH_TO_JABU_JABU) Flags_SetBingoInf(BINGO_INF_FEED_JABU_A_FISH);
        if (flag == EVENTCHKINF_TALON_WOKEN_IN_KAKARIKO) Flags_SetBingoInf(BINGO_INF_WAKE_UP_TALON);

        if ((flag == EVENTCHKINF_CARPENTERS_FREE(0) || flag == EVENTCHKINF_CARPENTERS_FREE(1) ||
             flag == EVENTCHKINF_CARPENTERS_FREE(2) || flag == EVENTCHKINF_CARPENTERS_FREE(3)) &&
             GET_EVENTCHKINF_CARPENTERS_FREE_ALL()) {
            Flags_SetBingoInf(BINGO_INF_REPAIR_THE_VALLEY_BRIDGE);
        }
        if (flag >= 0xDA && flag <= 0xDE) Flags_SetBingoInf(BINGO_INF_OBTAIN_ANY_SKULLTULA_HOUSE_CHECK);
    }

    if (flagType == FLAG_GS_TOKEN) {
        LUSLOG_DEBUG("GS Flag: %d", flag);
        switch (gPlayState->sceneNum) {
            case SCENE_ZORAS_FOUNTAIN:
                if (flag == 0x5120) Flags_SetBingoInf(BINGO_INF_COMPLETE_HIDDEN_CAVE_GS);
                break;
            case SCENE_HAUNTED_WASTELAND:
                if (flag == 0x3502 && Flags_GetTreasure(gPlayState, 0x0)) Flags_SetBingoInf(BINGO_INF_COLLECT_WASTELAND_CHEST_AND_SKULL);
                break;
            default:
                break;
        }
    }
}

void BingoOnSceneFlagSetHandler(int16_t sceneNum, int16_t flagType, int16_t flag) {
    auto ctx = Rando::Context::GetInstance();

    if ((sceneNum == SCENE_ROYAL_FAMILYS_TOMB || sceneNum == SCENE_GORON_CITY) && flagType == FLAG_SCENE_TREASURE) {
        BingoHandleCollectChecksSquares();
    }

    constexpr int bossKeyScenes[] = { // I dont think this is actually necessary (as I don't believe mini-dungeons and
        SCENE_FOREST_TEMPLE,          // child dungeons use 0x14) but better safe than sorry?
        SCENE_FIRE_TEMPLE,
        SCENE_WATER_TEMPLE,
        SCENE_SPIRIT_TEMPLE,
        SCENE_SHADOW_TEMPLE,
        SCENE_GANONS_TOWER
    };

    if ((flagType == FLAG_EVENT_CHECK_INF && flag == EVENTCHKINF_PLAYED_SARIAS_SONG_FOR_MIDO_AS_ADULT) ||
        (flagType == FLAG_ITEM_GET_INF && flag == 22)) {
        BingoHandleSariaActors();
    }

    if (flagType == FLAG_SCENE_SWITCH) {
        BingoRecordEyeSwitch(sceneNum, flag);
        BingoHandleBeanSpots(sceneNum);

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

                if (flag == 0x1 || flag == 0x2 || flag == 0x5 || flag == 0x6 || flag == 0x9) {
                    BingoRecordLockedDoors(sceneNum, flag);
                }

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
                if (Flags_GetSwitch(gPlayState, 0x31) && Flags_GetSwitch(gPlayState, 0x33) &&
                    Flags_GetSwitch(gPlayState, 0x35)) {
                    Flags_SetBingoInf(BINGO_INF_PLAYED_FOR_ALL_3_TRIFORCES_IN_SPIRIT);
                }
                if (flag == 0xD || flag == 0x15 || flag == 0x1B || flag == 0x1C || flag == 0x1E) {
                    BingoRecordLockedDoors(sceneNum, flag);
                }

                break;
            case SCENE_SHADOW_TEMPLE:
                if (flag == 0x1|| flag == 0x9 || flag == 0x8) {
                    BingoHandleSilverRupeeData(flagType, sceneNum, flag, ctx);
                }
                break;
            case SCENE_INSIDE_GANONS_CASTLE:
                if (flag == 0xE || flag == 0x9 || flag == 0xB || flag == 0x12) {
                    BingoHandleSilverRupeeData(flagType, sceneNum, flag, ctx);
                }
                break;
            case SCENE_ICE_CAVERN:
                if (flag == 0x8 || flag == 0x9) {
                    BingoHandleSilverRupeeData(flagType, sceneNum, flag, ctx);
                }
                break;
            case SCENE_BOTTOM_OF_THE_WELL:
                if (flag == 0x3) {
                    Flags_SetBingoInf(BINGO_INF_LOWER_THE_WATER_IN_BOTTOM_OF_THE_WELL);
                }
                break;
            case SCENE_GERUDOS_FORTRESS:
                if (flag == 0x3A) Flags_SetBingoInf(BINGO_INF_OPEN_THE_DOOR_TO_GTG);
                break;
            case SCENE_ZORAS_DOMAIN:
                if (flag == 0x4) Flags_SetBingoInf(BINGO_INF_THAW_ZORA_SHOP);
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
        int dekuChestsOpened = 0;
        switch (sceneNum) {
            case SCENE_DEKU_TREE:
                for (int i = 1; i < 7; i++) {
                    if (Flags_GetTreasure(gPlayState, i)) {
                        ++dekuChestsOpened;
                    }
                }
                if (dekuChestsOpened >= 6) {
                    Flags_SetBingoInf(BINGO_INF_ALL_DEKU_TREE_CHESTS);
                }
                break;
            case SCENE_JABU_JABU:
                if (Flags_GetTreasure(gPlayState, 0x1) && Flags_GetTreasure(gPlayState, 0x2) && Flags_GetTreasure(gPlayState, 0x4)) {
                    Flags_SetBingoInf(BINGO_INF_ALL_JABU_JABU_CHESTS);
                }
                break;
            case SCENE_FIRE_TEMPLE:
                if (Flags_GetTreasure(gPlayState, 0x5) && Flags_GetTreasure(gPlayState, 0xD)) {
                    Flags_SetBingoInf(BINGO_INF_FIRE_TEMPLE_TIMER_CHALLENGE_CHESTS);
                }
                break;
            case SCENE_GERUDO_TRAINING_GROUND:
                if (flag == 0xC) Flags_SetBingoInf(BINGO_INF_ICE_ARROW_CHEST);
                if (flag == 0xD) Flags_SetBingoInf(BINGO_INF_TOILET_ROOM_CHEST);
                break;
            case SCENE_INSIDE_GANONS_CASTLE:
                if (flag == 0x5) Flags_SetBingoInf(BINGO_INF_GOLD_GAUNTLETS_CHEST);
                break;
            case SCENE_GANONS_TOWER:
                if (flag == 0xB) Flags_SetBingoInf(BINGO_INF_GANON_BOSS_KEY_CHEST);
                break;
            case SCENE_DESERT_COLOSSUS:
                if (Flags_GetTreasure(gPlayState, 0x9) && Flags_GetTreasure(gPlayState, 0xB)) Flags_SetBingoInf(BINGO_INF_DESERT_COLOSSUS_HAND_CHESTS);
                break;
            case SCENE_WINDMILL_AND_DAMPES_GRAVE:
                if (flag == 0x00 && gSaveContext.sceneFlags[SCENE_GRAVEYARD].collect & (1 << 0x19)) {
                    Flags_SetBingoInf(BINGO_INF_COLLECT_BOTH_DAMPE_CHECKS);
                } 
                break;
            case SCENE_HAUNTED_WASTELAND:
                if (flag == 0x0 && (GET_GS_FLAGS((0x3502 & 0x1F00) >> 8) & (0x3502 & 0xFF))) {
                    Flags_SetBingoInf(BINGO_INF_COLLECT_WASTELAND_CHEST_AND_SKULL);
                } 
                break;
            default:
                break;
        }
    }

    if (flagType == FLAG_SCENE_COLLECTIBLE) {
        switch (sceneNum) {
            case SCENE_GORON_CITY:
                if (flag == 0x1F) Flags_SetBingoInf(BINGO_INF_GORON_CITY_BIG_POT_CHECK);
                break;
            case SCENE_ZORAS_FOUNTAIN:
                if (Flags_GetCollectible(gPlayState, 0x1) && Flags_GetCollectible(gPlayState, 0x14)) {
                    Flags_SetBingoInf(BINGO_INF_BOTH_ZF_HEART_PIECE_CHECKS);
                }
                break;
            case SCENE_GRAVEYARD:
                if (flag == 0x19 && gSaveContext.sceneFlags[SCENE_WINDMILL_AND_DAMPES_GRAVE].chest & (1 << 0x00)) {
                    Flags_SetBingoInf(BINGO_INF_COLLECT_BOTH_DAMPE_CHECKS);
                } 
                break;
            case SCENE_GROTTOS:
                if (flag == 0x01) Flags_SetBingoInf(BINGO_INF_COLLECT_TEKTITE_GROTTO_BOTTOM_ITEM);
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
            case SCENE_JABU_JABU:
                if (flag == 0x6) {
                    Flags_SetBingoInf(BINGO_INF_DEFEAT_BIG_OCTO);
                }
                break;
            case SCENE_FIRE_TEMPLE:
                if (Flags_GetClear(gPlayState, 0x3) && Flags_GetClear(gPlayState, 0x18)) {
                    Flags_SetBingoInf(BINGO_INF_DEFEAT_BOTH_FLARE_DANCERS);
                }
                break;
            case SCENE_WATER_TEMPLE:
                if (flag == 0xD) Flags_SetBingoInf(BINGO_INF_DEFEAT_DARK_LINK);
                break;
            case SCENE_SHADOW_TEMPLE:
                if (flag == 0x4) Flags_SetBingoInf(BINGO_INF_DEFEAT_DEAD_HAND_SHADOW_TEMPLE);
                break;
            case SCENE_GERUDO_TRAINING_GROUND:
                if (Flags_GetClear(gPlayState, 0x3) && Flags_GetClear(gPlayState, 0xA)) {
                    Flags_SetBingoInf(BINGO_INF_DEFEAT_ALL_WOLFOS_AND_LIKE_LIKES_IN_GTG);
                }
                break;
            case SCENE_BOTTOM_OF_THE_WELL:
                if (flag == 0x4) {
                    Flags_SetBingoInf(BINGO_INF_DEFEAT_DEAD_HAND_BOTTOM_OF_THE_WELL);
                }
                break;
            default:
                break;
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

    BingoHandleGrottoEntry(sceneNum);
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

    if (receivedItemEntry.getItemId == GI_BOMBCHUS_10 && gPlayState->sceneNum == SCENE_HAUNTED_WASTELAND) {
        Flags_SetBingoInf(BINGO_INF_CARPET_SALESMAN_ITEM);
        BingoHandleMerchants(-1, GI_BOMBCHUS_10);
    }
    if (receivedItemEntry.getItemId == GI_POTION_BLUE && gPlayState->sceneNum == SCENE_POTION_SHOP_GRANNY) {
        Flags_SetBingoInf(BINGO_INF_GRANNYS_POTION);
        BingoHandleMerchants(-1, GI_POTION_BLUE);
    }
    if (receivedItemEntry.getItemId == GI_BEAN && gPlayState->sceneNum == SCENE_ZORAS_RIVER) {
        Flags_SetBingoInf(BINGO_INF_BEAN_GUY_ITEM);
        BingoHandleMerchants(-1, GI_BEAN);
    }
    if (receivedItemEntry.getItemId == GI_SWORD_KNIFE && gPlayState->sceneNum == SCENE_GORON_CITY) {
        BingoHandleMerchants(-1, GI_SWORD_KNIFE);
    }

    switch(receivedItemEntry.getItemId) {
        case GI_ARROW_LIGHT:
        case GI_KEY_BOSS:
        case GI_BOW:
        case RG_BIGGEST_QUIVER:
        case RG_BIG_QUIVER:
        case RG_MASTER_SWORD:
        case RG_GANONS_CASTLE_BOSS_KEY:
        case RG_MAGIC_SINGLE:
        case RG_MAGIC_DOUBLE:
            if (CHECK_DUNGEON_ITEM(DUNGEON_KEY_BOSS, SCENE_GANONS_TOWER) && INV_CONTENT(ITEM_BOW) == ITEM_BOW &&
                INV_CONTENT(ITEM_ARROW_LIGHT) == ITEM_ARROW_LIGHT && CHECK_OWNED_EQUIP(EQUIP_TYPE_SWORD, EQUIP_INV_SWORD_MASTER) &&
                gSaveContext.isMagicAcquired == 1) {
                Flags_SetBingoInf(BINGO_INF_ALL_GO_MODE_ITEMS);
            }
            break;
        case RG_GOLD_SKULLTULA_TOKEN:
            if (gSaveContext.inventory.gsTokens >= 20) Flags_SetBingoInf(BINGO_INF_COLLECT_20_SKULLTULA_TOKENS);
            if (gSaveContext.inventory.gsTokens >= 30) Flags_SetBingoInf(BINGO_INF_COLLECT_30_SKULLTULA_TOKENS);
            break;
        case GI_NUT_UPGRADE_40:
        case GI_STICK_UPGRADE_30:
            Flags_SetBingoInf(BINGO_INF_40_NUTS_OR_30_STICKS_UPGRADE);
            break;
        default:
            break;
    }
}

void BingoOnDialogMessageHandler() {
    MessageContext* msgCtx = &gPlayState->msgCtx;
    Actor* actor = msgCtx->talkActor;

    if (actor == nullptr)
        return;

    auto& spoken = gSaveContext.ship.bingo.dcScrubsSpoken;
    switch(gPlayState->sceneNum) {
        case SCENE_DODONGOS_CAVERN:
            if (actor->id == ACTOR_EN_DNS) {
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

            if (std::all_of(std::begin(spoken), std::end(spoken), [](u8 playerSpokeTo) { return playerSpokeTo != 0; })) {
                Flags_SetBingoInf(BINGO_INF_SPEAK_WITH_4_DC_BUSINESS_SCRUBS);
            }
            break;
        case SCENE_GORON_CITY:
            if (actor->id == ACTOR_EN_GM && (CHECK_OWNED_EQUIP(EQUIP_TYPE_SWORD, EQUIP_INV_SWORD_BIGGORON) ||
                CHECK_OWNED_EQUIP(EQUIP_TYPE_SWORD, EQUIP_INV_SWORD_BROKENGIANTKNIFE)) && CUR_UPG_VALUE(UPG_WALLET) >= 1) {
                gSaveContext.ship.bingo.merchantsBoughtFrom[0] = true; // Knife "bought" for bingo purposes in this scenario
                BingoHandleMerchants(-1, GI_NONE);
            }
            break;
        case SCENE_ZORAS_DOMAIN:
            if (actor->id == ACTOR_EN_DIVING_GAME && msgCtx->textId == 0x405D) Flags_SetBingoInf(BINGO_INF_WIN_DIVING_MINIGAME_TWICE);
            break;
        default:
            break;
    }
}

void BingoOnActorUpdateHandler(void* refActor) {
    Actor* actor = static_cast<Actor*>(refActor);

    int uniqueWallmasterVoids = 0;
    switch(actor->id) {
        case ACTOR_EN_DNS:
            // dropCollectible is only set on purchases
            if (reinterpret_cast<EnDns*>(actor)->dropCollectible) {
                BingoRecordScrubPurchase(reinterpret_cast<EnDns*>(actor));
            }
            break;
        case ACTOR_EN_DODONGO:
            if (reinterpret_cast<EnDodongo*>(actor)->actionState == 1) {
                Flags_SetBingoInf(BINGO_INF_FEED_A_DODONGO_A_BOMB_OR_FLOWER);
            }
            break;
        case ACTOR_EN_WALLMAS:
            if (reinterpret_cast<EnWallmas*>(actor)->timer == -0x1E &&
                !gSaveContext.ship.bingo.wallmasterVoids[gPlayState->sceneNum]) {
                gSaveContext.ship.bingo.wallmasterVoids[gPlayState->sceneNum] = true;

                for (int i = 0; i <= SCENE_OUTSIDE_GANONS_CASTLE; i++) {
                    if (gSaveContext.ship.bingo.wallmasterVoids[i]) {
                        uniqueWallmasterVoids++;
                    }
                }
                
                if (uniqueWallmasterVoids >= 3) {
                    Flags_SetBingoInf(BINGO_INF_VOIDED_BY_3_DIFFERENT_DUNGEON_WALLMASTERS);
                }
            }
            break;
        case ACTOR_EN_RU1:
            if (reinterpret_cast<EnRu1*>(actor)->waterState != 0) {
                Flags_SetBingoInf(BINGO_INF_DROWN_RUTO);
            }
            break;
        case ACTOR_BG_SPOT11_OASIS:
            if (reinterpret_cast<BgSpot11Oasis*>(actor)->actionFunc == func_808B29F0) {
                Flags_SetBingoInf(BINGO_INF_FILL_THE_OASIS);
            }
            break;
        case ACTOR_EN_BOM_BOWL_MAN:
            if (reinterpret_cast<EnBomBowlMan*>(actor)->gameResult == 1) {
                Flags_SetBingoInf(BINGO_INF_WIN_BOMBCHU_BOWLING_ONCE);
            }
            break;
        case ACTOR_EN_HORSE:
            if (reinterpret_cast<EnHorse*>(actor)->action == ENHORSE_ACT_BRIDGE_JUMP) {
                Flags_SetBingoInf(BINGO_INF_JUMP_THE_VALLEY_BRIDGE_WITH_EPONA);
            }
            break;
        case ACTOR_EN_GE2:
            if (reinterpret_cast<EnGe2*>(actor)->actionFunc == EnGe2_CaptureTurn &&
                CHECK_QUEST_ITEM(QUEST_GERUDO_CARD)) {
                Flags_SetBingoInf(BINGO_INF_VOLUNTARILY_ENTER_JAIL);
            }
            break;
        case ACTOR_EN_NIW:
            if (reinterpret_cast<EnNiw*>(actor)->unk_2A8 != 0 &&
                gPlayState->sceneNum == SCENE_KAKARIKO_VILLAGE && LINK_IS_ADULT) {
                Flags_SetBingoInf(BINGO_INF_ADULT_KAKARIKO_CUCCO_STORM);
            }
            break;
        case ACTOR_OBJ_BEAN:
            if (reinterpret_cast<ObjBean*>(actor)->actionFunc == ObjBean_Fly) {
                if (gPlayState->sceneNum == SCENE_LOST_WOODS) {
                    BingoHandleBeanSpots(gPlayState->sceneNum, gPlayState->roomCtx.curRoom.num, true);
                } else {
                    BingoHandleBeanSpots(gPlayState->sceneNum, -1, true);
                }
            }
            break;
    }
}

// Use this to check for when we specifically need to check AC_HIT and AT_HIT flags
void BingoShouldActorUpdateHandler(void* refActor, bool*) {
    Actor* actor = static_cast<Actor*>(refActor);

    if (actor->id == ACTOR_EN_WONDER_ITEM && gPlayState->sceneNum == SCENE_SPIRIT_TEMPLE &&
        gPlayState->roomCtx.curRoom.num == 0x5) {
        EnWonderItem* wonderItem = reinterpret_cast<EnWonderItem*>(actor);
        if (wonderItem->wonderMode == WONDERITEM_INTERACT_SWITCH && wonderItem->collider.base.acFlags & AC_HIT) {
            Flags_SetBingoInf(BINGO_INF_SPIRIT_STATUE_NIPPLE_PIERCING);
        }
    }

    if (actor->id == ACTOR_EN_FIREFLY) {
        EnFirefly* firefly = reinterpret_cast<EnFirefly*>(actor);
        if (firefly->collider.base.atFlags & AT_HIT && firefly->auraType == 1) {
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

    if (actor->id == ACTOR_BG_HAKA_TUBO) {
        BgHakaTubo* skullPot = reinterpret_cast<BgHakaTubo*>(actor);
        int actorIndex = GetActorListIndex(actor);
        if (skullPot->potCollider.base.acFlags & AC_HIT) {
            if (gPlayState->roomCtx.curRoom.num == 0xC) {
                gSaveContext.ship.bingo.skullPotsDestroyed[actorIndex - 8] = true;
            }
            if (gPlayState->roomCtx.curRoom.num == 0xE) {
                gSaveContext.ship.bingo.skullPotsDestroyed[3] = true;
            }
        }

        auto& tracked = gSaveContext.ship.bingo.skullPotsDestroyed;
        if (std::count_if(std::begin(tracked), std::end(tracked),
                        [](u8 skullPotTracked) { return skullPotTracked != 0; }) >= 4) {
            Flags_SetBingoInf(BINGO_INF_EXPLODE_4_SKULL_POTS);
        }
    }

    if (gPlayState->sceneNum == SCENE_WATER_TEMPLE && gPlayState->roomCtx.curRoom.num == 0x8 &&
        actor->id == ACTOR_OBJ_SWITCH) {
        ObjSwitch* objSwitch = reinterpret_cast<ObjSwitch*>(actor);

        /*
            Even with checks for disableAcTimer, cooldownTimer, and cooldownOn, hitting the
            switch even when it's "on" still triggers this logic. The ObjSwitch actor doesn't
            seem to have anything I can look at to prevent this so we'll keep this as a
            lil secret; even still, it's not really a big deal.
        */
        if (objSwitch->jntSph.col.base.acFlags & AC_HIT) {
            if (objSwitch->dyna.actor.world.pos.y == -212) {
                gSaveContext.ship.bingo.waterCrystalsActivated[1] = true;
            } else {
                gSaveContext.ship.bingo.waterCrystalsActivated[2] = true;
            }
        }
    }

    // ActorKill runs for the flame it spawns as well, hence why we handle this here.
    if (actor->id == ACTOR_EN_HONOTRAP && reinterpret_cast<EnHonotrap*>(actor)->collider.tris.base.acFlags & AC_HIT) {
        Flags_SetBingoInf(BINGO_INF_DEFEAT_THE_SHADOW_EYE_SWITCH);
    }

    if (actor->id == ACTOR_EN_GS && gPlayState->sceneNum == SCENE_DEATH_MOUNTAIN_CRATER) {
        EnGs* gossipStone = reinterpret_cast<EnGs*>(actor);
        // damageEffect 13 is hit by Hammer
        if (gossipStone->collider.base.acFlags & AC_HIT && gossipStone->actor.colChkInfo.damageEffect == 13) {
            Flags_SetBingoInf(BINGO_INF_FLATTEN_THE_GOSSIP_STONE_DMC);
        }
    }
}

void BingoOnEnemyDefeatHandler(void* refActor) {
    Actor* actor = static_cast<Actor*>(refActor);

    if (actor->id == ACTOR_DOOR_KILLER) {
        DoorKiller* doorKiller = reinterpret_cast<DoorKiller*>(actor);

        switch(gPlayState->sceneNum) {
            case SCENE_FIRE_TEMPLE:
                gSaveContext.ship.bingo.doorMimicsDefeated[0] = true;
                break;
            case SCENE_GERUDO_TRAINING_GROUND:
                gSaveContext.ship.bingo.doorMimicsDefeated[1] = true;
                break;
            case SCENE_SPIRIT_TEMPLE:
                gSaveContext.ship.bingo.doorMimicsDefeated[2] = true;
                break;
        }

        auto& defeated = gSaveContext.ship.bingo.doorMimicsDefeated;
        if (std::count_if(std::begin(defeated), std::end(defeated), [](u8 doorMimicDefeated)
            { return doorMimicDefeated != 0; }) >= 2) {
            Flags_SetBingoInf(BINGO_INF_BREAK_2_DOOR_MIMICS);
        }
    }

    if (actor->id == ACTOR_EN_BB) {
        int actorIndex = GetActorListIndex(actor);
        switch(actor->params) {
            case ENBB_BLUE:
                BingoRecordBlueBubbleDeath(actorIndex, gPlayState->roomCtx.curRoom.num);
                break;
            case ENBB_RED:
                BingoRecordRedBubbleDeath(gPlayState->sceneNum, actor->params);
                break;
            case ENBB_GREEN:
                BingoRecordGreenBubbleDeath(gPlayState->sceneNum, actorIndex);
                break;
            case ENBB_GREEN_BIG:
                BingoRecordGreenBubbleDeath(gPlayState->sceneNum);
                break;
            case ENBB_WHITE:
                Flags_SetBingoInf(BINGO_INF_DEFEAT_A_WHITE_BUBBLE);
                break;
            default:
                break;
        }
    }

    if (actor->id == ACTOR_EN_WF && actor->params == WOLFOS_WHITE) {
        int actorIndex = GetActorListIndex(actor);
        if (gPlayState->sceneNum == SCENE_GERUDO_TRAINING_GROUND) {
            if (actorIndex == 4) gSaveContext.ship.bingo.whiteWolfosDefeated[0] = true;
            if (actorIndex == 5) gSaveContext.ship.bingo.whiteWolfosDefeated[1] = true;
        }
        if (gPlayState->sceneNum == SCENE_ICE_CAVERN) {
            gSaveContext.ship.bingo.whiteWolfosDefeated[2] = true;
            Flags_SetBingoInf(BINGO_INF_DEFEAT_ICE_CAVERN_WOLFOS);
        }

        if (std::count_if(std::begin(gSaveContext.ship.bingo.whiteWolfosDefeated), std::end(gSaveContext.ship.bingo.whiteWolfosDefeated), [](u8 defeated)
            { return defeated != 0; }) >= 3) {
            Flags_SetBingoInf(BINGO_INF_DEFEAT_ALL_3_WHITE_WOLFOS);
        }
    }

    if (actor->id == ACTOR_EN_ZF) {
        int actorIndex = GetActorListIndex(actor);
        switch(actor->params) {
            case ENZF_TYPE_DINOLFOS:
                if (gPlayState->sceneNum == SCENE_GERUDO_TRAINING_GROUND) {
                    if (actorIndex == 0) gSaveContext.ship.bingo.dinolfosDefeated[0] = true;
                    if (actorIndex == 1) gSaveContext.ship.bingo.dinolfosDefeated[1] = true;
                }
                if (gPlayState->sceneNum == SCENE_GANONS_TOWER) {
                    if (actorIndex == 1) gSaveContext.ship.bingo.dinolfosDefeated[2] = true;
                    if (actorIndex == 2) gSaveContext.ship.bingo.dinolfosDefeated[3] = true;
                }

                if (std::count_if(std::begin(gSaveContext.ship.bingo.dinolfosDefeated), std::end(gSaveContext.ship.bingo.dinolfosDefeated), [](u8 defeated)
                    { return defeated != 0; }) >= 4) {
                    Flags_SetBingoInf(BINGO_INF_DEFEAT_4_DIFFERENT_DINOLFOS);
                }
                break;
            case ENZF_TYPE_LIZALFOS_LONE:
                if (gPlayState->sceneNum == SCENE_SPIRIT_TEMPLE) {
                    if (gPlayState->roomCtx.curRoom.num == 0x4) {
                        if (actorIndex == 4) gSaveContext.ship.bingo.spiritLizalfosDefeated[0] = true;
                        if (actorIndex == 5) gSaveContext.ship.bingo.spiritLizalfosDefeated[1] = true;
                    }
                    if (gPlayState->roomCtx.curRoom.num == 0x1A) {
                        if (actorIndex == 3) gSaveContext.ship.bingo.spiritLizalfosDefeated[2] = true;
                        if (actorIndex == 4) gSaveContext.ship.bingo.spiritLizalfosDefeated[3] = true;
                    }
                }

                if (std::count_if(std::begin(gSaveContext.ship.bingo.spiritLizalfosDefeated), std::end(gSaveContext.ship.bingo.spiritLizalfosDefeated), [](u8 defeated)
                    { return defeated != 0; }) >= 4) {
                    Flags_SetBingoInf(BINGO_INF_DEFEAT_ALL_SPIRIT_LIZALFOS);
                }
                break;
        }
    }

    if (actor->id == ACTOR_EN_IK) {
        int actorIndex = GetActorListIndex(actor);
        if (gPlayState->sceneNum == SCENE_GANONS_TOWER) {
            if (actorIndex == 1) gSaveContext.ship.bingo.ironKnucklesDefeated[0] = true;
            if (actorIndex == 2) gSaveContext.ship.bingo.ironKnucklesDefeated[1] = true;
        }
        // 2 & 3 indices are *supposed* to be handled in SCENE_FLAG_CLEAR,
        // but I'm handling them here since that's where the others are being handled.
        // Also, it kinda just doesn't matter really.
        if (gPlayState->sceneNum == SCENE_SPIRIT_TEMPLE) {
            if (gPlayState->roomCtx.curRoom.num == 0xA) gSaveContext.ship.bingo.ironKnucklesDefeated[2] = true;
            if (gPlayState->roomCtx.curRoom.num == 0x14) gSaveContext.ship.bingo.ironKnucklesDefeated[3] = true;
        }
        if (gPlayState->sceneNum == SCENE_SPIRIT_TEMPLE_BOSS) {
            gSaveContext.ship.bingo.ironKnucklesDefeated[4] = true;
        }

        if (std::count_if(std::begin(gSaveContext.ship.bingo.ironKnucklesDefeated), std::end(gSaveContext.ship.bingo.ironKnucklesDefeated), [](u8 defeated)
            { return defeated != 0; }) >= 4) {
            Flags_SetBingoInf(BINGO_INF_DEFEAT_4_DIFFERENT_IRON_KNUCKLES);
        }
    }

    if (actor->id == ACTOR_EN_RR) {
        switch(gPlayState->sceneNum) {
            case SCENE_FIRE_TEMPLE:
                gSaveContext.ship.bingo.likelikesDefeated[0] = true;
                break;
            case SCENE_WATER_TEMPLE:
                gSaveContext.ship.bingo.likelikesDefeated[1] = true;
                break;
            case SCENE_GERUDO_TRAINING_GROUND:
                gSaveContext.ship.bingo.likelikesDefeated[2] = true;
                break;
            case SCENE_SPIRIT_TEMPLE:
                gSaveContext.ship.bingo.likelikesDefeated[3] = true;
                break;
            case SCENE_BOTTOM_OF_THE_WELL:
                gSaveContext.ship.bingo.likelikesDefeated[4] = true;
                break;
            case SCENE_SHADOW_TEMPLE:
                gSaveContext.ship.bingo.likelikesDefeated[5] = true;
                break;
            case SCENE_INSIDE_GANONS_CASTLE:
                gSaveContext.ship.bingo.likelikesDefeated[6] = true;
                break;
            default:
                break;
        }

        if (std::count_if(std::begin(gSaveContext.ship.bingo.likelikesDefeated), std::end(gSaveContext.ship.bingo.likelikesDefeated), [](u8 defeated)
            { return defeated != 0; }) >= 3) {
            Flags_SetBingoInf(BINGO_INF_DEFEAT_3_DIFFERENT_DUNGEON_LIKE_LIKES);
        }
    }

    if (actor->id == ACTOR_EN_OKUTA && gPlayState->sceneNum == SCENE_JABU_JABU) {
        int actorIndex = GetActorListIndex(actor);
        switch(gPlayState->roomCtx.curRoom.num) {
            case 0x0:
                if (actorIndex == 0) gSaveContext.ship.bingo.jabuOctoroksDefeated[0] = true;
                if (actorIndex == 1) gSaveContext.ship.bingo.jabuOctoroksDefeated[1] = true;
                break;
            case 0x1:
                gSaveContext.ship.bingo.jabuOctoroksDefeated[2] = true;
                break;
            case 0x4:
                gSaveContext.ship.bingo.jabuOctoroksDefeated[3] = true;
                break;
            case 0xD:
                if (actorIndex == 0) gSaveContext.ship.bingo.jabuOctoroksDefeated[4] = true;
                if (actorIndex == 1) gSaveContext.ship.bingo.jabuOctoroksDefeated[5] = true;
                break;
            default:
                break;
        }
    }

    if (gPlayState->sceneNum == SCENE_GROTTOS) {
        s8 grottoID = Grotto_CurrentGrotto();
        auto& defeated = gSaveContext.ship.bingo.grottoEnemiesDefeated;
        if (grottoID >= 0 && grottoID < ARRAY_COUNT(defeated)) {
            defeated[grottoID] = true;

            if (std::count_if(std::begin(defeated), std::end(defeated),
                            [](u8 d) { return d != 0; }) >= 3) {
                Flags_SetBingoInf(BINGO_INF_DEFEAT_ENEMY_3_DIFFERENT_GROTTOS);
            }
        }
    }

    if (actor->id == ACTOR_EN_REEBA && actor->params == LEEVER_BIG) {
        Flags_SetBingoInf(BINGO_INF_DEFEAT_A_PURPLE_LEEVER);
    }

    if (actor->id == ACTOR_EN_CROW && actor->params == 1) {
        Flags_SetBingoInf(BINGO_INF_DEFEAT_A_LARGE_GUAY);
    }

    if (actor->id == ACTOR_EN_SKB && actor->scale.x >= 0.015f) {
        Flags_SetBingoInf(BINGO_INF_DEFEAT_A_LARGE_STALCHILD);
    }

    if (actor->id == ACTOR_EN_PEEHAT) Flags_SetBingoInf(BINGO_INF_DEFEAT_A_PEAHAT);
    if (actor->id == ACTOR_EN_SKJ) Flags_SetBingoInf(BINGO_INF_DEFEAT_A_SKULL_KID);
    if (actor->id == ACTOR_EN_GELDB) Flags_SetBingoInf(BINGO_INF_DEFEAT_A_GERUDO);

    if (actor->id == ACTOR_EN_TITE && gPlayState->sceneNum == SCENE_LAKE_HYLIA) {
        int actorIndex = GetActorListIndex(actor);
        gSaveContext.ship.bingo.lakeHyliaTektites[actorIndex - 22] = true;

        auto& defeated = gSaveContext.ship.bingo.lakeHyliaTektites;
        if (std::count_if(std::begin(defeated), std::end(defeated),
                        [](u8 d) { return d != 0; }) >= 8) {
            Flags_SetBingoInf(BINGO_INF_DEFEAT_ALL_HYLIA_TEKTITES);
        }
    }

    if (actor->id == ACTOR_EN_MB) {
        int actorIndex = GetActorListIndex(actor);
        if (actorIndex >= 5 && actorIndex <= 10) {
            gSaveContext.ship.bingo.moblinsDefeated[actorIndex - 5] = true;

            auto& defeated = gSaveContext.ship.bingo.moblinsDefeated;
            if (std::count_if(std::begin(defeated), std::end(defeated),
                            [](u8 d) { return d != 0; }) >= 6) {
                Flags_SetBingoInf(BINGO_INF_DEFEAT_EVERY_MOBLIN);
            }
        }
    }
}

void BingoOnBossDefeatHandler(void* refActor) {
    Actor* actor = static_cast<Actor*>(refActor);

    switch(actor->id) {
        case ACTOR_BOSS_GOMA:
            Flags_SetBingoInf(BINGO_INF_DEFEAT_GOHMA);
            break;
        case ACTOR_BOSS_DODONGO:
            Flags_SetBingoInf(BINGO_INF_DEFEAT_KING_DODONGO);
            break;
        case ACTOR_BOSS_VA:
            Flags_SetBingoInf(BINGO_INF_DEFEAT_BARINADE);
            break;
        case ACTOR_BOSS_GANONDROF:
            Flags_SetBingoInf(BINGO_INF_DEFEAT_PHANTOM_GANON);
            break;
        case ACTOR_BOSS_FD:
            Flags_SetBingoInf(BINGO_INF_DEFEAT_VOLVAGIA);
            break;
        case ACTOR_BOSS_MO:
            Flags_SetBingoInf(BINGO_INF_DEFEAT_MORPHA);
            break;
        case ACTOR_BOSS_TW:
            Flags_SetBingoInf(BINGO_INF_DEFEAT_TWINROVA);
            break;
        case ACTOR_BOSS_SST:
            Flags_SetBingoInf(BINGO_INF_DEFEAT_BONGO_BONGO);
            break;
        default:
            break;
    }
}

void BingoOnActorKillHandler(void* refActor) {
    Actor* actor = static_cast<Actor*>(refActor);

    if (actor->id == ACTOR_BG_HAKA_TRAP) {
        int actorIndex = GetActorListIndex(actor);
        if (actorIndex == 3) gSaveContext.ship.bingo.woodWallsBurnt[0] = true;
        if (actorIndex == 4) gSaveContext.ship.bingo.woodWallsBurnt[1] = true;

        if (gSaveContext.ship.bingo.woodWallsBurnt[0] && gSaveContext.ship.bingo.woodWallsBurnt[1]) {
            Flags_SetBingoInf(BINGO_INF_BURN_THE_WOOD_WALLS_IN_SHADOW);
        }
    }

    if (gPlayState->sceneNum == SCENE_LOST_WOODS && gPlayState->roomCtx.curRoom.num == 0x3 &&
        actor->id == ACTOR_EN_ITEM00) {
        int actorIndex = GetActorListIndex(actor);
        gSaveContext.ship.bingo.lwPoolItems[actorIndex - 4] = true;

        auto& collected = gSaveContext.ship.bingo.lwPoolItems;
        if (std::count_if(std::begin(collected), std::end(collected),
                            [](u8 d) { return d != 0; }) >= 8) {
                Flags_SetBingoInf(BINGO_INF_COLLECT_ALL_LW_POOL_ITEMS);
            }
    }
}

void BingoOnSceneSpawnActorsHandler() {
    switch(gPlayState->sceneNum) {
        case SCENE_SPIRIT_TEMPLE:
            // I kinda really hate needing to spawn actors to make the Nipple Piercing square work, but at least a wonder
            // item is minimally invasive and preserves the spirit of the game?
            if (gPlayState->roomCtx.curRoom.num == 0x5 && !Flags_GetBingoInf(BINGO_INF_SPIRIT_STATUE_NIPPLE_PIERCING)) {
                Actor_Spawn(&gPlayState->actorCtx, gPlayState, ACTOR_EN_WONDER_ITEM,
                            150.0f, 800.0f, -1440.0f, 0, 0, 6, 0x193F);
                Actor_Spawn(&gPlayState->actorCtx, gPlayState, ACTOR_EN_WONDER_ITEM,
                            -30.0f, 800.0f, -1440.0f, 0, 0, 6, 0x193F);
            }
            break;
        default:
            break;
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
    static uint32_t onEnemyDefeatHook = 0;
    static uint32_t onBossDefeatHook = 0;
    static uint32_t onActorKillHook = 0;

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
        GameInteractor::Instance->UnregisterGameHook<GameInteractor::OnEnemyDefeat>(onEnemyDefeatHook);
        GameInteractor::Instance->UnregisterGameHook<GameInteractor::OnBossDefeat>(onBossDefeatHook);
        GameInteractor::Instance->UnregisterGameHook<GameInteractor::OnActorKill>(onActorKillHook);

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
        onEnemyDefeatHook = 0;
        onBossDefeatHook = 0;
        onActorKillHook = 0;

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
        onSceneSpawnActorsHook = GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneSpawnActors>(
            BingoOnSceneSpawnActorsHandler);
        // onPlayDestroyHook =
        //     GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>(BingoOnPlayDestroyHandler);
        // onExitGameHook =
        //     GameInteractor::Instance->RegisterGameHook<GameInteractor::OnExitGame>(BingoOnExitGameHandler);
        // onKaleidoUpdateHook = GameInteractor::Instance->RegisterGameHook<GameInteractor::OnKaleidoscopeUpdate>(
        //     BingoOnKaleidoscopeUpdateHandler);
        onEnemyDefeatHook = GameInteractor::Instance->RegisterGameHook<GameInteractor::OnEnemyDefeat>(
            BingoOnEnemyDefeatHandler);
        onBossDefeatHook = GameInteractor::Instance->RegisterGameHook<GameInteractor::OnBossDefeat>(
            BingoOnBossDefeatHandler);
        onActorKillHook = GameInteractor::Instance->RegisterGameHook<GameInteractor::OnActorKill>(
            BingoOnActorKillHandler);
    });
}

static RegisterShipInitFunc initFunc_RegisterHooks(BingoRegisterHooks);
