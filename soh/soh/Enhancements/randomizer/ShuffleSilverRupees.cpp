#include <soh/OTRGlobals.h>
#include "soh/Enhancements/randomizer/randomizer.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "ShuffleSilverRupees.h"

extern "C" {
#include "variables.h"
#include "src/overlays/actors/ovl_En_G_Switch/z_en_g_switch.h"
extern PlayState* gPlayState;
}

namespace Rando {
SilverRupeeAreaInfo::SilverRupeeAreaInfo(std::string name_, const RandomizerHintTextKey hintKey_,
                                         const RandomizerSettingKey settingKey_, const RandomizerGet rupeeItem_,
                                         const RandomizerGet bagItem_)
    : name(std::move(name_)), hintKey(hintKey_), settingKey(settingKey_), rupeeItem(rupeeItem_), bagItem(bagItem_) {
}
SilverRupeeAreaInfo::SilverRupeeAreaInfo() = default;
SilverRupeeAreaInfo::~SilverRupeeAreaInfo() = default;

const std::string& SilverRupeeAreaInfo::GetName() const {
    return name;
}

RandomizerHintTextKey SilverRupeeAreaInfo::GetHintKey() const {
    return hintKey;
}

RandomizerSettingKey SilverRupeeAreaInfo::GetSettingKey() const {
    return settingKey;
}

RandomizerGet SilverRupeeAreaInfo::GetRupeeItem() const {
    return rupeeItem;
}

RandomizerGet SilverRupeeAreaInfo::GetBagItem() const {
    return bagItem;
}

void SilverRupeeAreaInfo::SetShuffled(const bool asBag) {
    shuffled = true;
    bag = asBag;
}

void SilverRupeeAreaInfo::ClearShuffled() {
    shuffled = false;
    bag = false;
}

bool SilverRupeeAreaInfo::IsShuffled() const {
    return shuffled;
}

bool SilverRupeeAreaInfo::IsBag() const {
    return bag;
}

SilverRupeeAreas::SilverRupeeAreas() {
    mAreas[SRA_SPIRIT_TEMPLE_BOULDERS] =
        SilverRupeeAreaInfo("Spirit Temple Boulder Room", RHT_SPIRIT_TEMPLE_BOULDER_SILVER_RUPEES,
                            RSK_SILVERS_SPIRIT_TEMPLE_BOULDERS, RG_SPIRIT_TEMPLE_BOULDER_SILVER_RUPEE,
                            RG_SPIRIT_TEMPLE_BOULDER_SILVER_RUPEE_BAG);
    mAreas[SRA_SPIRIT_TEMPLE_CHILD] =
        SilverRupeeAreaInfo("Spirit Temple Child Torches Room", RHT_SPIRIT_TEMPLE_CHILD_SILVER_RUPEES,
                            RSK_SILVERS_SPIRIT_TEMPLE_CHILD, RG_SPIRIT_TEMPLE_CHILD_SILVER_RUPEE,
                            RG_SPIRIT_TEMPLE_CHILD_SILVER_RUPEE_BAG);
    mAreas[SRA_SPIRIT_TEMPLE_SUN_BLOCK] =
        SilverRupeeAreaInfo("Spirit Temple Sun Block Room", RHT_SPIRIT_TEMPLE_SUN_BLOCK_SILVER_RUPEES,
                            RSK_SILVERS_SPIRIT_TEMPLE_SUN_BLOCK, RG_SPIRIT_TEMPLE_SUN_BLOCK_SILVER_RUPEE,
                            RG_SPIRIT_TEMPLE_SUN_BLOCK_SILVER_RUPEE_BAG);
    mAreas[SRA_SHADOW_TEMPLE_SCYTHE] =
        SilverRupeeAreaInfo("Shadow Temple Scythe Room", RHT_SHADOW_TEMPLE_SCYTHE_SILVER_RUPEES,
                            RSK_SILVERS_SHADOW_TEMPLE_SCYTHE, RG_SHADOW_TEMPLE_SCYTHE_SILVER_RUPEE,
                            RG_SHADOW_TEMPLE_SCYTHE_SILVER_RUPEE_BAG);
    mAreas[SRA_SHADOW_TEMPLE_BEAMOS] =
        SilverRupeeAreaInfo("Shadow Temple Beamos Room", RHT_SHADOW_TEMPLE_BEAMOS_SILVER_RUPEES,
                            RSK_SILVERS_SHADOW_TEMPLE_BEAMOS, RG_SHADOW_TEMPLE_BEAMOS_SILVER_RUPEE,
                            RG_SHADOW_TEMPLE_BEAMOS_SILVER_RUPEE_BAG);
    mAreas[SRA_SHADOW_TEMPLE_REDEAD] =
        SilverRupeeAreaInfo("Shadow Temple ReDead Room", RHT_SHADOW_TEMPLE_REDEAD_SILVER_RUPEES,
                            RSK_SILVERS_SHADOW_TEMPLE_REDEAD, RG_SHADOW_TEMPLE_REDEAD_SILVER_RUPEE,
                            RG_SHADOW_TEMPLE_REDEAD_SILVER_RUPEE_BAG);
    mAreas[SRA_GANONS_CASTLE_FOREST_TRIAL] =
        SilverRupeeAreaInfo("Ganon's Castle Forest Trial", RHT_GANONS_CASTLE_FOREST_TRIAL_SILVER_RUPEES,
                            RSK_SILVERS_FOREST_TRIAL, RG_GANONS_CASTLE_FOREST_TRIAL_SILVER_RUPEE,
                            RG_GANONS_CASTLE_FOREST_TRIAL_SILVER_RUPEE_BAG);
    mAreas[SRA_GANONS_CASTLE_FIRE_TRIAL] =
        SilverRupeeAreaInfo("Ganon's Castle Fire Trial", RHT_GANONS_CASTLE_FIRE_TRIAL_SILVER_RUPEES,
                            RSK_SILVERS_FIRE_TRIAL, RG_GANONS_CASTLE_FIRE_TRIAL_SILVER_RUPEE,
                            RG_GANONS_CASTLE_FIRE_TRIAL_SILVER_RUPEE_BAG);
    mAreas[SRA_GANONS_CASTLE_SPIRIT_TRIAL] =
        SilverRupeeAreaInfo("Ganon's Castle Spirit Trial", RHT_GANONS_CASTLE_SPIRIT_TRIAL_SILVER_RUPEES,
                            RSK_SILVERS_SPIRIT_TRIAL, RG_GANONS_CASTLE_SPIRIT_TRIAL_SILVER_RUPEE,
                            RG_GANONS_CASTLE_SPIRIT_TRIAL_SILVER_RUPEE_BAG);
    mAreas[SRA_GANONS_CASTLE_LIGHT_TRIAL] =
        SilverRupeeAreaInfo("Ganon's Castle Light Trial", RHT_GANONS_CASTLE_LIGHT_TRIAL_SILVER_RUPEES,
                            RSK_SILVERS_LIGHT_TRIAL, RG_GANONS_CASTLE_LIGHT_TRIAL_SILVER_RUPEE,
                            RG_GANONS_CASTLE_LIGHT_TRIAL_SILVER_RUPEE_BAG);
    mAreas[SRA_ICE_CAVERN_SCYTHE] =
        SilverRupeeAreaInfo("Ice Cavern Scythe Room", RHT_ICE_CAVERN_SCYTHE_SILVER_RUPEES,
                            RSK_SILVERS_ICE_CAVERN_SCYTHE, RG_ICE_CAVERN_SCYTHE_SILVER_RUPEE,
                            RG_ICE_CAVERN_SCYTHE_SILVER_RUPEE_BAG);
    mAreas[SRA_ICE_CAVERN_PUSH_BLOCK] =
        SilverRupeeAreaInfo("Ice Cavern Push Block Room", RHT_ICE_CAVERN_PUSH_BLOCK_SILVER_RUPEES,
                            RSK_SILVERS_ICE_CAVERN_PUSH_BLOCK, RG_ICE_CAVERN_PUSH_BLOCK_SILVER_RUPEE,
                            RG_ICE_CAVERN_PUSH_BLOCK_SILVER_RUPEE_BAG);
    mAreas[SRA_GERUDO_TRAINING_GROUND_BOULDERS] =
        SilverRupeeAreaInfo("Gerudo Training Ground Boulder Room", RHT_GERUDO_TRAINING_GROUND_BOULDER_SILVER_RUPEES,
                            RSK_SILVERS_GTG_BOULDERS, RG_GERUDO_TRAINING_GROUND_BOULDER_SILVER_RUPEE,
                            RG_GERUDO_TRAINING_GROUND_BOULDER_SILVER_RUPEE_BAG);
    mAreas[SRA_GERUDO_TRAINING_GROUND_LAVA] =
        SilverRupeeAreaInfo("Gerudo Training Ground Lava Room", RHT_GERUDO_TRAINING_GROUND_LAVA_SILVER_RUPEES,
                            RSK_SILVERS_GTG_LAVA, RG_GERUDO_TRAINING_GROUND_LAVA_SILVER_RUPEE,
                            RG_GERUDO_TRAINING_GROUND_LAVA_SILVER_RUPEE_BAG);
    mAreas[SRA_GERUDO_TRAINING_GROUND_TOILET] =
        SilverRupeeAreaInfo("Gerudo Training Ground Toilet Room", RHT_GERUDO_TRAINING_GROUND_TOILET_SILVER_RUPEES,
                            RSK_SILVERS_GTG_TOILET, RG_GERUDO_TRAINING_GROUND_TOILET_SILVER_RUPEE,
                            RG_GERUDO_TRAINING_GROUND_TOILET_SILVER_RUPEE_BAG);
    mAreas[SRA_BOTTOM_OF_THE_WELL_BASEMENT] =
        SilverRupeeAreaInfo("Bottom of the Well Basement", RHT_BOTTOM_OF_THE_WELL_BASEMENT_SILVER_RUPEES,
                            RSK_SILVERS_BOTW, RG_BOTTOM_OF_THE_WELL_BASEMENT_SILVER_RUPEE, RG_BOTTOM_OF_THE_WELL_BASEMENT_SILVER_RUPEE_BAG);
}
SilverRupeeAreas::~SilverRupeeAreas() = default;

SilverRupeeAreaInfo* SilverRupeeAreas::GetArea(const SilverRupeeAreaKey key) {
    return &mAreas[key];
}

SilverRupeeAreaInfo* SilverRupeeAreas::GetAreaForHintKey(const RandomizerHintTextKey hintKey) {
    for (auto& area : mAreas) {
        if (area.GetHintKey() == hintKey) {
            return &area;
        }
    }
    return nullptr;
}

void SilverRupeeAreas::ClearAllShuffled() {
    for (auto& area : mAreas) {
        area.ClearShuffled();
    }
}

std::array<SilverRupeeAreaInfo*, SRA_MAX> SilverRupeeAreas::GetAreaList() {
    std::array<SilverRupeeAreaInfo*, SRA_MAX> areaList{};
    for (size_t i = 0; i < mAreas.size(); i++) {
        areaList[i] = &mAreas[i];
    }
    return areaList;
}

size_t SilverRupeeAreas::GetAreaListSize() const {
    return mAreas.size();
}
} // namespace Rando


void SetSilverRupeeFlag() {
    if (gPlayState == nullptr) {
        return;
    }
    
    if (Flags_GetRandomizerInf(RAND_INF_SPIRIT_TEMPLE_BOULDER_SILVERS)) {
        gSaveContext.sceneFlags[SCENE_SPIRIT_TEMPLE].swch |= (1 << 0x02);
        if (gPlayState->sceneNum == SCENE_SPIRIT_TEMPLE) {
            Flags_SetSwitch(gPlayState, 0x02);
        }
    }

    if (Flags_GetRandomizerInf(RAND_INF_SPIRIT_TEMPLE_CHILD_SILVERS)) {
        gSaveContext.sceneFlags[SCENE_SPIRIT_TEMPLE].swch |= (1 << 0x05);
        if (gPlayState->sceneNum == SCENE_SPIRIT_TEMPLE) {
            Flags_SetSwitch(gPlayState, 0x05);
        }
    }

    if (Flags_GetRandomizerInf(RAND_INF_SPIRIT_TEMPLE_SUN_BLOCK_SILVERS)) {
        gSaveContext.sceneFlags[SCENE_SPIRIT_TEMPLE].swch |= (1 << 0x0A);
        if (gPlayState->sceneNum == SCENE_SPIRIT_TEMPLE) {
            Flags_SetSwitch(gPlayState, 0x0A);
        }
    }

    if (Flags_GetRandomizerInf(RAND_INF_SHADOW_TEMPLE_SCYTHE_SILVERS)) {
        gSaveContext.sceneFlags[SCENE_SHADOW_TEMPLE].swch |= (1 << 0x01);
        if (gPlayState->sceneNum == SCENE_SHADOW_TEMPLE) {
            Flags_SetSwitch(gPlayState, 0x01);
        }
    }

    if (Flags_GetRandomizerInf(RAND_INF_SHADOW_TEMPLE_BEAMOS_SILVERS)) {
        gSaveContext.sceneFlags[SCENE_SHADOW_TEMPLE].swch |= (1 << 0x09);
        if (gPlayState->sceneNum == SCENE_SHADOW_TEMPLE) {
            Flags_SetSwitch(gPlayState, 0x09);
        }
    }

    if (Flags_GetRandomizerInf(RAND_INF_SHADOW_TEMPLE_REDEAD_SILVERS)) {
        gSaveContext.sceneFlags[SCENE_SHADOW_TEMPLE].swch |= (1 << 0x08);
        if (gPlayState->sceneNum == SCENE_SHADOW_TEMPLE) {
            Flags_SetSwitch(gPlayState, 0x08);
        }
    }

    if (Flags_GetRandomizerInf(RAND_INF_GANONS_CASTLE_FOREST_TRIAL_SILVERS)) {
        gSaveContext.sceneFlags[SCENE_INSIDE_GANONS_CASTLE].swch |= (1 << 0x0E);
        if (gPlayState->sceneNum == SCENE_INSIDE_GANONS_CASTLE) {
            Flags_SetSwitch(gPlayState, 0x0E);
        }
    }

    if (Flags_GetRandomizerInf(RAND_INF_GANONS_CASTLE_FIRE_TRIAL_SILVERS)) {
        gSaveContext.sceneFlags[SCENE_INSIDE_GANONS_CASTLE].swch |= (1 << 0x09);
        if (gPlayState->sceneNum == SCENE_INSIDE_GANONS_CASTLE) {
            Flags_SetSwitch(gPlayState, 0x09);
        }
    }

    if (Flags_GetRandomizerInf(RAND_INF_GANONS_CASTLE_SPIRIT_TRIAL_SILVERS)) {
        gSaveContext.sceneFlags[SCENE_INSIDE_GANONS_CASTLE].swch |= (1 << 0x0B);
        if (gPlayState->sceneNum == SCENE_INSIDE_GANONS_CASTLE) {
            Flags_SetSwitch(gPlayState, 0x0B);
        }
    }

    if (Flags_GetRandomizerInf(RAND_INF_GANONS_CASTLE_LIGHT_TRIAL_SILVERS)) {
        ActorContext* act = &gPlayState->actorCtx;

        gSaveContext.sceneFlags[SCENE_INSIDE_GANONS_CASTLE].swch |= (1 << 0x12);
        gSaveContext.sceneFlags[SCENE_INSIDE_GANONS_CASTLE].clear |= (1 << 0x08); // Remove room timer
        if (gPlayState->sceneNum == SCENE_INSIDE_GANONS_CASTLE) {
            Flags_SetSwitch(gPlayState, 0x12);
            Flags_SetClear(gPlayState, 0x08); // Remove room timer
            Flags_SetTempClear(gPlayState, 0x3f); // Remove room timer
        }
    }

    if (Flags_GetRandomizerInf(RAND_INF_ICE_CAVERN_SCYTHE_SILVERS)) {
        gSaveContext.sceneFlags[SCENE_ICE_CAVERN].swch |= (1 << 0x08);
        if (gPlayState->sceneNum == SCENE_ICE_CAVERN) {
            Flags_SetSwitch(gPlayState, 0x08);
        }
    }

    if (Flags_GetRandomizerInf(RAND_INF_ICE_CAVERN_PUSH_BLOCK_SILVERS)) {
        gSaveContext.sceneFlags[SCENE_ICE_CAVERN].swch |= (1 << 0x09);
        if (gPlayState->sceneNum == SCENE_ICE_CAVERN) {
            Flags_SetSwitch(gPlayState, 0x09);
        }
    }

    if (Flags_GetRandomizerInf(RAND_INF_GERUDO_TRAINING_GROUND_BOULDER_SILVERS)) {
        gSaveContext.sceneFlags[SCENE_GERUDO_TRAINING_GROUND].swch |= (1 << 0x1C);
        gSaveContext.sceneFlags[SCENE_GERUDO_TRAINING_GROUND].clear |= (1 << 0x2);
        if (gPlayState->sceneNum == SCENE_GERUDO_TRAINING_GROUND) {
            Flags_SetSwitch(gPlayState, 0x1C);
            Flags_SetClear(gPlayState, 0x2);
        }
    }

    if (Flags_GetRandomizerInf(RAND_INF_GERUDO_TRAINING_GROUND_LAVA_SILVERS)) {
        gSaveContext.sceneFlags[SCENE_GERUDO_TRAINING_GROUND].swch |= (1 << 0x0C);
        if (gPlayState->sceneNum == SCENE_GERUDO_TRAINING_GROUND) {
            Flags_SetSwitch(gPlayState, 0x0C);
        }
    }

    if (Flags_GetRandomizerInf(RAND_INF_GERUDO_TRAINING_GROUND_TOILET_SILVERS)) {
        gSaveContext.sceneFlags[SCENE_GERUDO_TRAINING_GROUND].swch |= (1 << 0x1B);
        if (gPlayState->sceneNum == SCENE_GERUDO_TRAINING_GROUND) {
            Flags_SetSwitch(gPlayState, 0x1B);
        }
    }

    if (Flags_GetRandomizerInf(RAND_INF_BOTTOM_OF_THE_WELL_BASEMENT_SILVERS)) {
        gSaveContext.sceneFlags[SCENE_BOTTOM_OF_THE_WELL].swch |= (1 << 0x1B);
        if (gPlayState->sceneNum == SCENE_BOTTOM_OF_THE_WELL) {
            Flags_SetSwitch(gPlayState, 0x1F);
        }
    }
}

extern void EnItem00_DrawRandomizedItem(EnItem00* enItem00, PlayState* play);

CheckIdentity ShuffleSilverRupees_GetSilverRupeeIdentity(int32_t params, ActorID id) {
    CheckIdentity silverRupeeIdentity;
    s16 sceneNum = gPlayState->sceneNum;
    silverRupeeIdentity.randomizerInf = RAND_INF_MAX;
    silverRupeeIdentity.randomizerCheck = RC_UNKNOWN_CHECK;

    Rando::Location* location = OTRGlobals::Instance->gRandomizer->GetCheckObjectFromActor(id, sceneNum, params);

    if (location->GetRandomizerCheck() == RC_UNKNOWN_CHECK) {
        LUSLOG_WARN("GetSilverRupeeIdentity did not receive a valid RC value (%d).", location->GetRandomizerCheck());
        //assert(false);
    } else if (!Rando::Context::GetInstance()->IsSilverRupeeAreaShuffled(location->GetHintKey())) {
        // This room's Silver Rupees were not selected for shuffling, so leave the identity
        // unresolved and let the vanilla EnGSwitch actor behave normally.
    } else {
        silverRupeeIdentity.randomizerInf = static_cast<RandomizerInf>(location->GetCollectionCheck().flag);
        silverRupeeIdentity.randomizerCheck = location->GetRandomizerCheck();
    }

    return silverRupeeIdentity;
}

static void SpawnSilverRupeeItem(f32 posX, f32 posY, f32 posZ, const CheckIdentity& silverRupeeIdentity) {
    Vec3f spawnPos = { posX, posY, posZ };
    EnItem00* item00 = (EnItem00*)Item_DropCollectible2(gPlayState, &spawnPos, ITEM00_SOH_DUMMY);
    item00->randoInf = silverRupeeIdentity.randomizerInf;
    item00->itemEntry =
        Rando::Context::GetInstance()->GetFinalGIEntry(silverRupeeIdentity.randomizerCheck, true, GI_NONE);
    item00->actor.draw = (ActorFunc)EnItem00_DrawRandomizedItem;
    item00->actor.gravity = 0.0f;
    item00->actor.velocity.y = 0.0f;
    item00->actor.speedXZ = 0.0f;
}

void RegisterShuffleSilverRupees() {
    bool shouldRegister = IS_RANDO && RAND_GET_OPTION(RSK_SILVER_RUPEES);

    COND_HOOK(OnGameFrameUpdate, shouldRegister, SetSilverRupeeFlag);

    COND_VB_SHOULD(VB_EN_G_SWITCH_SPAWN_ITEM, shouldRegister, {
        EnGSwitch* enGSwitch = va_arg(args, EnGSwitch*);

        CheckIdentity silverRupeeIdentity = ShuffleSilverRupees_GetSilverRupeeIdentity(
            TWO_ACTOR_PARAMS((int32_t)enGSwitch->actor.world.pos.x, (int32_t)enGSwitch->actor.world.pos.z),
            ACTOR_EN_G_SWITCH);

        if (silverRupeeIdentity.randomizerCheck != RC_UNKNOWN_CHECK) {
            *should = false;
            if (!Flags_GetRandomizerInf(silverRupeeIdentity.randomizerInf)) {
                SpawnSilverRupeeItem(enGSwitch->actor.world.pos.x, enGSwitch->actor.world.pos.y,
                                     enGSwitch->actor.world.pos.z, silverRupeeIdentity);
            }
        }
    });
}

void Rando::StaticData::RegisterSilverRupeeLocations() {
    static bool registered = false;
    if (registered)
        return;
    registered = true;
    // clang-format off
    locationTable[RC_SPIRIT_TEMPLE_BOULDER_SILVER_RUPEE_1] =                  Location::SilverRupee(RC_SPIRIT_TEMPLE_BOULDER_SILVER_RUPEE_1,              RCQUEST_VANILLA, RCAREA_SPIRIT_TEMPLE,                 SCENE_SPIRIT_TEMPLE,              TWO_ACTOR_PARAMS(1573, -920),        "Boulder Room Silver Rupee 1",           RHT_SPIRIT_TEMPLE_BOULDER_SILVER_RUPEES,           SpoilerCollectionCheck::RandomizerInf(RAND_INF_SPIRIT_TEMPLE_BOULDER_SILVER_RUPEE_1));
    locationTable[RC_SPIRIT_TEMPLE_BOULDER_SILVER_RUPEE_2] =                  Location::SilverRupee(RC_SPIRIT_TEMPLE_BOULDER_SILVER_RUPEE_2,              RCQUEST_VANILLA, RCAREA_SPIRIT_TEMPLE,                 SCENE_SPIRIT_TEMPLE,              TWO_ACTOR_PARAMS(1856, -1219),       "Boulder Room Silver Rupee 2",           RHT_SPIRIT_TEMPLE_BOULDER_SILVER_RUPEES,           SpoilerCollectionCheck::RandomizerInf(RAND_INF_SPIRIT_TEMPLE_BOULDER_SILVER_RUPEE_2));
    locationTable[RC_SPIRIT_TEMPLE_BOULDER_SILVER_RUPEE_3] =                  Location::SilverRupee(RC_SPIRIT_TEMPLE_BOULDER_SILVER_RUPEE_3,              RCQUEST_VANILLA, RCAREA_SPIRIT_TEMPLE,                 SCENE_SPIRIT_TEMPLE,              TWO_ACTOR_PARAMS(1856, -944),        "Boulder Room Silver Rupee 3",           RHT_SPIRIT_TEMPLE_BOULDER_SILVER_RUPEES,           SpoilerCollectionCheck::RandomizerInf(RAND_INF_SPIRIT_TEMPLE_BOULDER_SILVER_RUPEE_3));
    locationTable[RC_SPIRIT_TEMPLE_BOULDER_SILVER_RUPEE_4] =                  Location::SilverRupee(RC_SPIRIT_TEMPLE_BOULDER_SILVER_RUPEE_4,              RCQUEST_VANILLA, RCAREA_SPIRIT_TEMPLE,                 SCENE_SPIRIT_TEMPLE,              TWO_ACTOR_PARAMS(1284, -813),        "Boulder Room Silver Rupee 4",           RHT_SPIRIT_TEMPLE_BOULDER_SILVER_RUPEES,           SpoilerCollectionCheck::RandomizerInf(RAND_INF_SPIRIT_TEMPLE_BOULDER_SILVER_RUPEE_4));
    locationTable[RC_SPIRIT_TEMPLE_BOULDER_SILVER_RUPEE_5] =                  Location::SilverRupee(RC_SPIRIT_TEMPLE_BOULDER_SILVER_RUPEE_5,              RCQUEST_VANILLA, RCAREA_SPIRIT_TEMPLE,                 SCENE_SPIRIT_TEMPLE,              TWO_ACTOR_PARAMS(1284, -1355),       "Boulder Room Silver Rupee 5",           RHT_SPIRIT_TEMPLE_BOULDER_SILVER_RUPEES,           SpoilerCollectionCheck::RandomizerInf(RAND_INF_SPIRIT_TEMPLE_BOULDER_SILVER_RUPEE_5));
    locationTable[RC_SPIRIT_TEMPLE_CHILD_SILVER_RUPEE_1] =                    Location::SilverRupee(RC_SPIRIT_TEMPLE_CHILD_SILVER_RUPEE_1,                RCQUEST_VANILLA, RCAREA_SPIRIT_TEMPLE,                 SCENE_SPIRIT_TEMPLE,              TWO_ACTOR_PARAMS(-335, -1409),       "Child Torches Silver Rupee 1",          RHT_SPIRIT_TEMPLE_CHILD_SILVER_RUPEES,             SpoilerCollectionCheck::RandomizerInf(RAND_INF_SPIRIT_TEMPLE_CHILD_SILVER_RUPEE_1));
    locationTable[RC_SPIRIT_TEMPLE_CHILD_SILVER_RUPEE_2] =                    Location::SilverRupee(RC_SPIRIT_TEMPLE_CHILD_SILVER_RUPEE_2,                RCQUEST_VANILLA, RCAREA_SPIRIT_TEMPLE,                 SCENE_SPIRIT_TEMPLE,              TWO_ACTOR_PARAMS(-347, -1075),       "Child Torches Silver Rupee 2",          RHT_SPIRIT_TEMPLE_CHILD_SILVER_RUPEES,             SpoilerCollectionCheck::RandomizerInf(RAND_INF_SPIRIT_TEMPLE_CHILD_SILVER_RUPEE_2));
    locationTable[RC_SPIRIT_TEMPLE_CHILD_SILVER_RUPEE_3] =                    Location::SilverRupee(RC_SPIRIT_TEMPLE_CHILD_SILVER_RUPEE_3,                RCQUEST_VANILLA, RCAREA_SPIRIT_TEMPLE,                 SCENE_SPIRIT_TEMPLE,              TWO_ACTOR_PARAMS(-511, -1075),       "Child Torches Silver Rupee 3",          RHT_SPIRIT_TEMPLE_CHILD_SILVER_RUPEES,             SpoilerCollectionCheck::RandomizerInf(RAND_INF_SPIRIT_TEMPLE_CHILD_SILVER_RUPEE_3));
    locationTable[RC_SPIRIT_TEMPLE_CHILD_SILVER_RUPEE_4] =                    Location::SilverRupee(RC_SPIRIT_TEMPLE_CHILD_SILVER_RUPEE_4,                RCQUEST_VANILLA, RCAREA_SPIRIT_TEMPLE,                 SCENE_SPIRIT_TEMPLE,              TWO_ACTOR_PARAMS(-672, -1075),       "Child Torches Silver Rupee 4",          RHT_SPIRIT_TEMPLE_CHILD_SILVER_RUPEES,             SpoilerCollectionCheck::RandomizerInf(RAND_INF_SPIRIT_TEMPLE_CHILD_SILVER_RUPEE_4));
    locationTable[RC_SPIRIT_TEMPLE_CHILD_SILVER_RUPEE_5] =                    Location::SilverRupee(RC_SPIRIT_TEMPLE_CHILD_SILVER_RUPEE_5,                RCQUEST_VANILLA, RCAREA_SPIRIT_TEMPLE,                 SCENE_SPIRIT_TEMPLE,              TWO_ACTOR_PARAMS(-766, -1075),       "Child Torches Silver Rupee 5",          RHT_SPIRIT_TEMPLE_CHILD_SILVER_RUPEES,             SpoilerCollectionCheck::RandomizerInf(RAND_INF_SPIRIT_TEMPLE_CHILD_SILVER_RUPEE_5));
    locationTable[RC_SPIRIT_TEMPLE_SUN_BLOCK_SILVER_RUPEE_1] =                Location::SilverRupee(RC_SPIRIT_TEMPLE_SUN_BLOCK_SILVER_RUPEE_1,            RCQUEST_VANILLA, RCAREA_SPIRIT_TEMPLE,                 SCENE_SPIRIT_TEMPLE,              TWO_ACTOR_PARAMS(-984, -450),        "Sun Block Silver Rupee 1",              RHT_SPIRIT_TEMPLE_SUN_BLOCK_SILVER_RUPEES,         SpoilerCollectionCheck::RandomizerInf(RAND_INF_SPIRIT_TEMPLE_SUN_BLOCK_SILVER_RUPEE_1));
    locationTable[RC_SPIRIT_TEMPLE_SUN_BLOCK_SILVER_RUPEE_2] =                Location::SilverRupee(RC_SPIRIT_TEMPLE_SUN_BLOCK_SILVER_RUPEE_2,            RCQUEST_VANILLA, RCAREA_SPIRIT_TEMPLE,                 SCENE_SPIRIT_TEMPLE,              TWO_ACTOR_PARAMS(-1123, 428),       "Sun Block Silver Rupee 2",              RHT_SPIRIT_TEMPLE_SUN_BLOCK_SILVER_RUPEES,         SpoilerCollectionCheck::RandomizerInf(RAND_INF_SPIRIT_TEMPLE_SUN_BLOCK_SILVER_RUPEE_2));
    locationTable[RC_SPIRIT_TEMPLE_SUN_BLOCK_SILVER_RUPEE_3] =                Location::SilverRupee(RC_SPIRIT_TEMPLE_SUN_BLOCK_SILVER_RUPEE_3,            RCQUEST_VANILLA, RCAREA_SPIRIT_TEMPLE,                 SCENE_SPIRIT_TEMPLE,              TWO_ACTOR_PARAMS(-1275, -247),       "Sun Block Silver Rupee 3",              RHT_SPIRIT_TEMPLE_SUN_BLOCK_SILVER_RUPEES,         SpoilerCollectionCheck::RandomizerInf(RAND_INF_SPIRIT_TEMPLE_SUN_BLOCK_SILVER_RUPEE_3));
    locationTable[RC_SPIRIT_TEMPLE_SUN_BLOCK_SILVER_RUPEE_4] =                Location::SilverRupee(RC_SPIRIT_TEMPLE_SUN_BLOCK_SILVER_RUPEE_4,            RCQUEST_VANILLA, RCAREA_SPIRIT_TEMPLE,                 SCENE_SPIRIT_TEMPLE,              TWO_ACTOR_PARAMS(-1433, -283),       "Sun Block Silver Rupee 4",              RHT_SPIRIT_TEMPLE_SUN_BLOCK_SILVER_RUPEES,         SpoilerCollectionCheck::RandomizerInf(RAND_INF_SPIRIT_TEMPLE_SUN_BLOCK_SILVER_RUPEE_4));
    locationTable[RC_SPIRIT_TEMPLE_SUN_BLOCK_SILVER_RUPEE_5] =                Location::SilverRupee(RC_SPIRIT_TEMPLE_SUN_BLOCK_SILVER_RUPEE_5,            RCQUEST_VANILLA, RCAREA_SPIRIT_TEMPLE,                 SCENE_SPIRIT_TEMPLE,              TWO_ACTOR_PARAMS(-1836, -446),       "Sun Block Silver Rupee 5",              RHT_SPIRIT_TEMPLE_SUN_BLOCK_SILVER_RUPEES,         SpoilerCollectionCheck::RandomizerInf(RAND_INF_SPIRIT_TEMPLE_SUN_BLOCK_SILVER_RUPEE_5));

    locationTable[RC_SHADOW_TEMPLE_SCYTHE_SILVER_RUPEE_1] =                   Location::SilverRupee(RC_SHADOW_TEMPLE_SCYTHE_SILVER_RUPEE_1,               RCQUEST_VANILLA, RCAREA_SHADOW_TEMPLE,                 SCENE_SHADOW_TEMPLE,              TWO_ACTOR_PARAMS(3399, -838),        "Scythe Room Silver Rupee 1",            RHT_SHADOW_TEMPLE_SCYTHE_SILVER_RUPEES,            SpoilerCollectionCheck::RandomizerInf(RAND_INF_SHADOW_TEMPLE_SCYTHE_SILVER_RUPEE_1));
    locationTable[RC_SHADOW_TEMPLE_SCYTHE_SILVER_RUPEE_2] =                   Location::SilverRupee(RC_SHADOW_TEMPLE_SCYTHE_SILVER_RUPEE_2,               RCQUEST_VANILLA, RCAREA_SHADOW_TEMPLE,                 SCENE_SHADOW_TEMPLE,              TWO_ACTOR_PARAMS(2869, -948),        "Scythe Room Silver Rupee 2",            RHT_SHADOW_TEMPLE_SCYTHE_SILVER_RUPEES,            SpoilerCollectionCheck::RandomizerInf(RAND_INF_SHADOW_TEMPLE_SCYTHE_SILVER_RUPEE_2));
    locationTable[RC_SHADOW_TEMPLE_SCYTHE_SILVER_RUPEE_3] =                   Location::SilverRupee(RC_SHADOW_TEMPLE_SCYTHE_SILVER_RUPEE_3,               RCQUEST_VANILLA, RCAREA_SHADOW_TEMPLE,                 SCENE_SHADOW_TEMPLE,              TWO_ACTOR_PARAMS(3554, -1432),       "Scythe Room Silver Rupee 3",            RHT_SHADOW_TEMPLE_SCYTHE_SILVER_RUPEES,            SpoilerCollectionCheck::RandomizerInf(RAND_INF_SHADOW_TEMPLE_SCYTHE_SILVER_RUPEE_3));
    locationTable[RC_SHADOW_TEMPLE_SCYTHE_SILVER_RUPEE_4] =                   Location::SilverRupee(RC_SHADOW_TEMPLE_SCYTHE_SILVER_RUPEE_4,               RCQUEST_VANILLA, RCAREA_SHADOW_TEMPLE,                 SCENE_SHADOW_TEMPLE,              TWO_ACTOR_PARAMS(3007, -1222),       "Scythe Room Silver Rupee 4",            RHT_SHADOW_TEMPLE_SCYTHE_SILVER_RUPEES,            SpoilerCollectionCheck::RandomizerInf(RAND_INF_SHADOW_TEMPLE_SCYTHE_SILVER_RUPEE_4));
    locationTable[RC_SHADOW_TEMPLE_SCYTHE_SILVER_RUPEE_5] =                   Location::SilverRupee(RC_SHADOW_TEMPLE_SCYTHE_SILVER_RUPEE_5,               RCQUEST_VANILLA, RCAREA_SHADOW_TEMPLE,                 SCENE_SHADOW_TEMPLE,              TWO_ACTOR_PARAMS(3243, -1061),       "Scythe Room Silver Rupee 5",            RHT_SHADOW_TEMPLE_SCYTHE_SILVER_RUPEES,            SpoilerCollectionCheck::RandomizerInf(RAND_INF_SHADOW_TEMPLE_SCYTHE_SILVER_RUPEE_5));
    locationTable[RC_SHADOW_TEMPLE_BEAMOS_SILVER_RUPEE_1] =                   Location::SilverRupee(RC_SHADOW_TEMPLE_BEAMOS_SILVER_RUPEE_1,               RCQUEST_VANILLA, RCAREA_SHADOW_TEMPLE,                 SCENE_SHADOW_TEMPLE,              TWO_ACTOR_PARAMS(1998, 3358),        "Beamos Silver Rupee 1",                 RHT_SHADOW_TEMPLE_BEAMOS_SILVER_RUPEES,            SpoilerCollectionCheck::RandomizerInf(RAND_INF_SHADOW_TEMPLE_BEAMOS_SILVER_RUPEE_1));
    locationTable[RC_SHADOW_TEMPLE_BEAMOS_SILVER_RUPEE_2] =                   Location::SilverRupee(RC_SHADOW_TEMPLE_BEAMOS_SILVER_RUPEE_2,               RCQUEST_VANILLA, RCAREA_SHADOW_TEMPLE,                 SCENE_SHADOW_TEMPLE,              TWO_ACTOR_PARAMS(2243, 3361),        "Beamos Silver Rupee 2",                 RHT_SHADOW_TEMPLE_BEAMOS_SILVER_RUPEES,            SpoilerCollectionCheck::RandomizerInf(RAND_INF_SHADOW_TEMPLE_BEAMOS_SILVER_RUPEE_2));
    locationTable[RC_SHADOW_TEMPLE_BEAMOS_SILVER_RUPEE_3] =                   Location::SilverRupee(RC_SHADOW_TEMPLE_BEAMOS_SILVER_RUPEE_3,               RCQUEST_VANILLA, RCAREA_SHADOW_TEMPLE,                 SCENE_SHADOW_TEMPLE,              TWO_ACTOR_PARAMS(2110, 3368),        "Beamos Silver Rupee 3",                 RHT_SHADOW_TEMPLE_BEAMOS_SILVER_RUPEES,            SpoilerCollectionCheck::RandomizerInf(RAND_INF_SHADOW_TEMPLE_BEAMOS_SILVER_RUPEE_3));
    locationTable[RC_SHADOW_TEMPLE_BEAMOS_SILVER_RUPEE_4] =                   Location::SilverRupee(RC_SHADOW_TEMPLE_BEAMOS_SILVER_RUPEE_4,               RCQUEST_VANILLA, RCAREA_SHADOW_TEMPLE,                 SCENE_SHADOW_TEMPLE,              TWO_ACTOR_PARAMS(2115, 3738),        "Beamos Silver Rupee 4",                 RHT_SHADOW_TEMPLE_BEAMOS_SILVER_RUPEES,            SpoilerCollectionCheck::RandomizerInf(RAND_INF_SHADOW_TEMPLE_BEAMOS_SILVER_RUPEE_4));
    locationTable[RC_SHADOW_TEMPLE_BEAMOS_SILVER_RUPEE_5] =                   Location::SilverRupee(RC_SHADOW_TEMPLE_BEAMOS_SILVER_RUPEE_5,               RCQUEST_VANILLA, RCAREA_SHADOW_TEMPLE,                 SCENE_SHADOW_TEMPLE,              TWO_ACTOR_PARAMS(2131, 3030),        "Beamos Silver Rupee 5",                 RHT_SHADOW_TEMPLE_BEAMOS_SILVER_RUPEES,            SpoilerCollectionCheck::RandomizerInf(RAND_INF_SHADOW_TEMPLE_BEAMOS_SILVER_RUPEE_5));
    locationTable[RC_SHADOW_TEMPLE_REDEAD_SILVER_RUPEE_1] =                   Location::SilverRupee(RC_SHADOW_TEMPLE_REDEAD_SILVER_RUPEE_1,               RCQUEST_VANILLA, RCAREA_SHADOW_TEMPLE,                 SCENE_SHADOW_TEMPLE,              TWO_ACTOR_PARAMS(2254, 988),         "ReDead Silver Rupee 1",                 RHT_SHADOW_TEMPLE_REDEAD_SILVER_RUPEES,            SpoilerCollectionCheck::RandomizerInf(RAND_INF_SHADOW_TEMPLE_REDEAD_SILVER_RUPEE_1));
    locationTable[RC_SHADOW_TEMPLE_REDEAD_SILVER_RUPEE_2] =                   Location::SilverRupee(RC_SHADOW_TEMPLE_REDEAD_SILVER_RUPEE_2,               RCQUEST_VANILLA, RCAREA_SHADOW_TEMPLE,                 SCENE_SHADOW_TEMPLE,              TWO_ACTOR_PARAMS(2135, 1297),        "ReDead Silver Rupee 2",                 RHT_SHADOW_TEMPLE_REDEAD_SILVER_RUPEES,            SpoilerCollectionCheck::RandomizerInf(RAND_INF_SHADOW_TEMPLE_REDEAD_SILVER_RUPEE_2));
    locationTable[RC_SHADOW_TEMPLE_REDEAD_SILVER_RUPEE_3] =                   Location::SilverRupee(RC_SHADOW_TEMPLE_REDEAD_SILVER_RUPEE_3,               RCQUEST_VANILLA, RCAREA_SHADOW_TEMPLE,                 SCENE_SHADOW_TEMPLE,              TWO_ACTOR_PARAMS(2478, 1208),        "ReDead Silver Rupee 3",                 RHT_SHADOW_TEMPLE_REDEAD_SILVER_RUPEES,            SpoilerCollectionCheck::RandomizerInf(RAND_INF_SHADOW_TEMPLE_REDEAD_SILVER_RUPEE_3));
    locationTable[RC_SHADOW_TEMPLE_REDEAD_SILVER_RUPEE_4] =                   Location::SilverRupee(RC_SHADOW_TEMPLE_REDEAD_SILVER_RUPEE_4,               RCQUEST_VANILLA, RCAREA_SHADOW_TEMPLE,                 SCENE_SHADOW_TEMPLE,              TWO_ACTOR_PARAMS(2940, 1069),        "ReDead Silver Rupee 4",                 RHT_SHADOW_TEMPLE_REDEAD_SILVER_RUPEES,            SpoilerCollectionCheck::RandomizerInf(RAND_INF_SHADOW_TEMPLE_REDEAD_SILVER_RUPEE_4));
    locationTable[RC_SHADOW_TEMPLE_REDEAD_SILVER_RUPEE_5] =                   Location::SilverRupee(RC_SHADOW_TEMPLE_REDEAD_SILVER_RUPEE_5,               RCQUEST_VANILLA, RCAREA_SHADOW_TEMPLE,                 SCENE_SHADOW_TEMPLE,              TWO_ACTOR_PARAMS(2042, 849),         "ReDead Silver Rupee 5",                 RHT_SHADOW_TEMPLE_REDEAD_SILVER_RUPEES,            SpoilerCollectionCheck::RandomizerInf(RAND_INF_SHADOW_TEMPLE_REDEAD_SILVER_RUPEE_5));

    locationTable[RC_GANONS_CASTLE_FOREST_TRIAL_SILVER_RUPEE_1] =             Location::SilverRupee(RC_GANONS_CASTLE_FOREST_TRIAL_SILVER_RUPEE_1,         RCQUEST_VANILLA, RCAREA_GANONS_CASTLE,                 SCENE_INSIDE_GANONS_CASTLE,       TWO_ACTOR_PARAMS(1634, 1550),        "Forest Trial Silver Rupee 1",           RHT_GANONS_CASTLE_FOREST_TRIAL_SILVER_RUPEES,      SpoilerCollectionCheck::RandomizerInf(RAND_INF_GANONS_CASTLE_FOREST_TRIAL_SILVER_RUPEE_1));
    locationTable[RC_GANONS_CASTLE_FOREST_TRIAL_SILVER_RUPEE_2] =             Location::SilverRupee(RC_GANONS_CASTLE_FOREST_TRIAL_SILVER_RUPEE_2,         RCQUEST_VANILLA, RCAREA_GANONS_CASTLE,                 SCENE_INSIDE_GANONS_CASTLE,       TWO_ACTOR_PARAMS(1651, 2021),        "Forest Trial Silver Rupee 2",           RHT_GANONS_CASTLE_FOREST_TRIAL_SILVER_RUPEES,      SpoilerCollectionCheck::RandomizerInf(RAND_INF_GANONS_CASTLE_FOREST_TRIAL_SILVER_RUPEE_2));
    locationTable[RC_GANONS_CASTLE_FOREST_TRIAL_SILVER_RUPEE_3] =             Location::SilverRupee(RC_GANONS_CASTLE_FOREST_TRIAL_SILVER_RUPEE_3,         RCQUEST_VANILLA, RCAREA_GANONS_CASTLE,                 SCENE_INSIDE_GANONS_CASTLE,       TWO_ACTOR_PARAMS(1538, 2225),        "Forest Trial Silver Rupee 3",           RHT_GANONS_CASTLE_FOREST_TRIAL_SILVER_RUPEES,      SpoilerCollectionCheck::RandomizerInf(RAND_INF_GANONS_CASTLE_FOREST_TRIAL_SILVER_RUPEE_3));
    locationTable[RC_GANONS_CASTLE_FOREST_TRIAL_SILVER_RUPEE_4] =             Location::SilverRupee(RC_GANONS_CASTLE_FOREST_TRIAL_SILVER_RUPEE_4,         RCQUEST_VANILLA, RCAREA_GANONS_CASTLE,                 SCENE_INSIDE_GANONS_CASTLE,       TWO_ACTOR_PARAMS(1361, 1222),        "Forest Trial Silver Rupee 4",           RHT_GANONS_CASTLE_FOREST_TRIAL_SILVER_RUPEES,      SpoilerCollectionCheck::RandomizerInf(RAND_INF_GANONS_CASTLE_FOREST_TRIAL_SILVER_RUPEE_4));
    locationTable[RC_GANONS_CASTLE_FOREST_TRIAL_SILVER_RUPEE_5] =             Location::SilverRupee(RC_GANONS_CASTLE_FOREST_TRIAL_SILVER_RUPEE_5,         RCQUEST_VANILLA, RCAREA_GANONS_CASTLE,                 SCENE_INSIDE_GANONS_CASTLE,       TWO_ACTOR_PARAMS(1247, 1787),        "Forest Trial Silver Rupee 5",           RHT_GANONS_CASTLE_FOREST_TRIAL_SILVER_RUPEES,      SpoilerCollectionCheck::RandomizerInf(RAND_INF_GANONS_CASTLE_FOREST_TRIAL_SILVER_RUPEE_5));
    locationTable[RC_GANONS_CASTLE_FIRE_TRIAL_SILVER_RUPEE_1] =               Location::SilverRupee(RC_GANONS_CASTLE_FIRE_TRIAL_SILVER_RUPEE_1,           RCQUEST_VANILLA, RCAREA_GANONS_CASTLE,                 SCENE_INSIDE_GANONS_CASTLE,       TWO_ACTOR_PARAMS(-1867, -2754),      "Fire Trial Silver Rupee 1",             RHT_GANONS_CASTLE_FIRE_TRIAL_SILVER_RUPEES,        SpoilerCollectionCheck::RandomizerInf(RAND_INF_GANONS_CASTLE_FIRE_TRIAL_SILVER_RUPEE_1));
    locationTable[RC_GANONS_CASTLE_FIRE_TRIAL_SILVER_RUPEE_2] =               Location::SilverRupee(RC_GANONS_CASTLE_FIRE_TRIAL_SILVER_RUPEE_2,           RCQUEST_VANILLA, RCAREA_GANONS_CASTLE,                 SCENE_INSIDE_GANONS_CASTLE,       TWO_ACTOR_PARAMS(-604, -2592),       "Fire Trial Silver Rupee 2",             RHT_GANONS_CASTLE_FIRE_TRIAL_SILVER_RUPEES,        SpoilerCollectionCheck::RandomizerInf(RAND_INF_GANONS_CASTLE_FIRE_TRIAL_SILVER_RUPEE_2));
    locationTable[RC_GANONS_CASTLE_FIRE_TRIAL_SILVER_RUPEE_3] =               Location::SilverRupee(RC_GANONS_CASTLE_FIRE_TRIAL_SILVER_RUPEE_3,           RCQUEST_VANILLA, RCAREA_GANONS_CASTLE,                 SCENE_INSIDE_GANONS_CASTLE,       TWO_ACTOR_PARAMS(-515, -3253),       "Fire Trial Silver Rupee 3",             RHT_GANONS_CASTLE_FIRE_TRIAL_SILVER_RUPEES,        SpoilerCollectionCheck::RandomizerInf(RAND_INF_GANONS_CASTLE_FIRE_TRIAL_SILVER_RUPEE_3));
    locationTable[RC_GANONS_CASTLE_FIRE_TRIAL_SILVER_RUPEE_4] =               Location::SilverRupee(RC_GANONS_CASTLE_FIRE_TRIAL_SILVER_RUPEE_4,           RCQUEST_VANILLA, RCAREA_GANONS_CASTLE,                 SCENE_INSIDE_GANONS_CASTLE,       TWO_ACTOR_PARAMS(-1555, -2317),      "Fire Trial Silver Rupee 4",             RHT_GANONS_CASTLE_FIRE_TRIAL_SILVER_RUPEES,        SpoilerCollectionCheck::RandomizerInf(RAND_INF_GANONS_CASTLE_FIRE_TRIAL_SILVER_RUPEE_4));
    locationTable[RC_GANONS_CASTLE_FIRE_TRIAL_SILVER_RUPEE_5] =               Location::SilverRupee(RC_GANONS_CASTLE_FIRE_TRIAL_SILVER_RUPEE_5,           RCQUEST_VANILLA, RCAREA_GANONS_CASTLE,                 SCENE_INSIDE_GANONS_CASTLE,       TWO_ACTOR_PARAMS(-854, -3679),       "Fire Trial Silver Rupee 5",             RHT_GANONS_CASTLE_FIRE_TRIAL_SILVER_RUPEES,        SpoilerCollectionCheck::RandomizerInf(RAND_INF_GANONS_CASTLE_FIRE_TRIAL_SILVER_RUPEE_5));
    locationTable[RC_GANONS_CASTLE_SPIRIT_TRIAL_SILVER_RUPEE_1] =             Location::SilverRupee(RC_GANONS_CASTLE_SPIRIT_TRIAL_SILVER_RUPEE_1,         RCQUEST_VANILLA, RCAREA_GANONS_CASTLE,                 SCENE_INSIDE_GANONS_CASTLE,       TWO_ACTOR_PARAMS(-1142, 639),        "Spirit Trial Silver Rupee 1",           RHT_GANONS_CASTLE_SPIRIT_TRIAL_SILVER_RUPEES,      SpoilerCollectionCheck::RandomizerInf(RAND_INF_GANONS_CASTLE_SPIRIT_TRIAL_SILVER_RUPEE_1));
    locationTable[RC_GANONS_CASTLE_SPIRIT_TRIAL_SILVER_RUPEE_2] =             Location::SilverRupee(RC_GANONS_CASTLE_SPIRIT_TRIAL_SILVER_RUPEE_2,         RCQUEST_VANILLA, RCAREA_GANONS_CASTLE,                 SCENE_INSIDE_GANONS_CASTLE,       TWO_ACTOR_PARAMS(-725, 942),         "Spirit Trial Silver Rupee 2",           RHT_GANONS_CASTLE_SPIRIT_TRIAL_SILVER_RUPEES,      SpoilerCollectionCheck::RandomizerInf(RAND_INF_GANONS_CASTLE_SPIRIT_TRIAL_SILVER_RUPEE_2));
    locationTable[RC_GANONS_CASTLE_SPIRIT_TRIAL_SILVER_RUPEE_3] =             Location::SilverRupee(RC_GANONS_CASTLE_SPIRIT_TRIAL_SILVER_RUPEE_3,         RCQUEST_VANILLA, RCAREA_GANONS_CASTLE,                 SCENE_INSIDE_GANONS_CASTLE,       TWO_ACTOR_PARAMS(-865, 656),         "Spirit Trial Silver Rupee 3",           RHT_GANONS_CASTLE_SPIRIT_TRIAL_SILVER_RUPEES,      SpoilerCollectionCheck::RandomizerInf(RAND_INF_GANONS_CASTLE_SPIRIT_TRIAL_SILVER_RUPEE_3));
    locationTable[RC_GANONS_CASTLE_SPIRIT_TRIAL_SILVER_RUPEE_4] =             Location::SilverRupee(RC_GANONS_CASTLE_SPIRIT_TRIAL_SILVER_RUPEE_4,         RCQUEST_VANILLA, RCAREA_GANONS_CASTLE,                 SCENE_INSIDE_GANONS_CASTLE,       TWO_ACTOR_PARAMS(-940, 270),         "Spirit Trial Silver Rupee 4",           RHT_GANONS_CASTLE_SPIRIT_TRIAL_SILVER_RUPEES,      SpoilerCollectionCheck::RandomizerInf(RAND_INF_GANONS_CASTLE_SPIRIT_TRIAL_SILVER_RUPEE_4));
    locationTable[RC_GANONS_CASTLE_SPIRIT_TRIAL_SILVER_RUPEE_5] =             Location::SilverRupee(RC_GANONS_CASTLE_SPIRIT_TRIAL_SILVER_RUPEE_5,         RCQUEST_VANILLA, RCAREA_GANONS_CASTLE,                 SCENE_INSIDE_GANONS_CASTLE,       TWO_ACTOR_PARAMS(-829, 591),         "Spirit Trial Silver Rupee 5",           RHT_GANONS_CASTLE_SPIRIT_TRIAL_SILVER_RUPEES,      SpoilerCollectionCheck::RandomizerInf(RAND_INF_GANONS_CASTLE_SPIRIT_TRIAL_SILVER_RUPEE_5));
    locationTable[RC_GANONS_CASTLE_LIGHT_TRIAL_SILVER_RUPEE_1] =              Location::SilverRupee(RC_GANONS_CASTLE_LIGHT_TRIAL_SILVER_RUPEE_1,          RCQUEST_VANILLA, RCAREA_GANONS_CASTLE,                 SCENE_INSIDE_GANONS_CASTLE,       TWO_ACTOR_PARAMS(-2674, -777),       "Light Trial Silver Rupee 1",            RHT_GANONS_CASTLE_LIGHT_TRIAL_SILVER_RUPEES,       SpoilerCollectionCheck::RandomizerInf(RAND_INF_GANONS_CASTLE_LIGHT_TRIAL_SILVER_RUPEE_1));
    locationTable[RC_GANONS_CASTLE_LIGHT_TRIAL_SILVER_RUPEE_2] =              Location::SilverRupee(RC_GANONS_CASTLE_LIGHT_TRIAL_SILVER_RUPEE_2,          RCQUEST_VANILLA, RCAREA_GANONS_CASTLE,                 SCENE_INSIDE_GANONS_CASTLE,       TWO_ACTOR_PARAMS(-2680, -893),       "Light Trial Silver Rupee 2",            RHT_GANONS_CASTLE_LIGHT_TRIAL_SILVER_RUPEES,       SpoilerCollectionCheck::RandomizerInf(RAND_INF_GANONS_CASTLE_LIGHT_TRIAL_SILVER_RUPEE_2));
    locationTable[RC_GANONS_CASTLE_LIGHT_TRIAL_SILVER_RUPEE_3] =              Location::SilverRupee(RC_GANONS_CASTLE_LIGHT_TRIAL_SILVER_RUPEE_3,          RCQUEST_VANILLA, RCAREA_GANONS_CASTLE,                 SCENE_INSIDE_GANONS_CASTLE,       TWO_ACTOR_PARAMS(-2646, -839),       "Light Trial Silver Rupee 3",            RHT_GANONS_CASTLE_LIGHT_TRIAL_SILVER_RUPEES,       SpoilerCollectionCheck::RandomizerInf(RAND_INF_GANONS_CASTLE_LIGHT_TRIAL_SILVER_RUPEE_3));
    locationTable[RC_GANONS_CASTLE_LIGHT_TRIAL_SILVER_RUPEE_4] =              Location::SilverRupee(RC_GANONS_CASTLE_LIGHT_TRIAL_SILVER_RUPEE_4,          RCQUEST_VANILLA, RCAREA_GANONS_CASTLE,                 SCENE_INSIDE_GANONS_CASTLE,       TWO_ACTOR_PARAMS(-2649, -545),       "Light Trial Silver Rupee 4",            RHT_GANONS_CASTLE_LIGHT_TRIAL_SILVER_RUPEES,       SpoilerCollectionCheck::RandomizerInf(RAND_INF_GANONS_CASTLE_LIGHT_TRIAL_SILVER_RUPEE_4));
    locationTable[RC_GANONS_CASTLE_LIGHT_TRIAL_SILVER_RUPEE_5] =              Location::SilverRupee(RC_GANONS_CASTLE_LIGHT_TRIAL_SILVER_RUPEE_5,          RCQUEST_VANILLA, RCAREA_GANONS_CASTLE,                 SCENE_INSIDE_GANONS_CASTLE,       TWO_ACTOR_PARAMS(-2509, -1091),      "Light Trial Silver Rupee 5",            RHT_GANONS_CASTLE_LIGHT_TRIAL_SILVER_RUPEES,       SpoilerCollectionCheck::RandomizerInf(RAND_INF_GANONS_CASTLE_LIGHT_TRIAL_SILVER_RUPEE_5));

    locationTable[RC_ICE_CAVERN_SCYTHE_SILVER_RUPEE_1] =                      Location::SilverRupee(RC_ICE_CAVERN_SCYTHE_SILVER_RUPEE_1,                  RCQUEST_VANILLA, RCAREA_ICE_CAVERN,                    SCENE_ICE_CAVERN,                 TWO_ACTOR_PARAMS(414, -579),         "Ice Scythe Room Silver Rupee 1",        RHT_ICE_CAVERN_SCYTHE_SILVER_RUPEES,               SpoilerCollectionCheck::RandomizerInf(RAND_INF_ICE_CAVERN_SCYTHE_SILVER_RUPEE_1));
    locationTable[RC_ICE_CAVERN_SCYTHE_SILVER_RUPEE_2] =                      Location::SilverRupee(RC_ICE_CAVERN_SCYTHE_SILVER_RUPEE_2,                  RCQUEST_VANILLA, RCAREA_ICE_CAVERN,                    SCENE_ICE_CAVERN,                 TWO_ACTOR_PARAMS(389, -382),         "Ice Scythe Room Silver Rupee 2",        RHT_ICE_CAVERN_SCYTHE_SILVER_RUPEES,               SpoilerCollectionCheck::RandomizerInf(RAND_INF_ICE_CAVERN_SCYTHE_SILVER_RUPEE_2));
    locationTable[RC_ICE_CAVERN_SCYTHE_SILVER_RUPEE_3] =                      Location::SilverRupee(RC_ICE_CAVERN_SCYTHE_SILVER_RUPEE_3,                  RCQUEST_VANILLA, RCAREA_ICE_CAVERN,                    SCENE_ICE_CAVERN,                 TWO_ACTOR_PARAMS(278, -637),         "Ice Scythe Room Silver Rupee 3",        RHT_ICE_CAVERN_SCYTHE_SILVER_RUPEES,               SpoilerCollectionCheck::RandomizerInf(RAND_INF_ICE_CAVERN_SCYTHE_SILVER_RUPEE_3));
    locationTable[RC_ICE_CAVERN_SCYTHE_SILVER_RUPEE_4] =                      Location::SilverRupee(RC_ICE_CAVERN_SCYTHE_SILVER_RUPEE_4,                  RCQUEST_VANILLA, RCAREA_ICE_CAVERN,                    SCENE_ICE_CAVERN,                 TWO_ACTOR_PARAMS(198, -388),         "Ice Scythe Room Silver Rupee 4",        RHT_ICE_CAVERN_SCYTHE_SILVER_RUPEES,               SpoilerCollectionCheck::RandomizerInf(RAND_INF_ICE_CAVERN_SCYTHE_SILVER_RUPEE_4));
    locationTable[RC_ICE_CAVERN_SCYTHE_SILVER_RUPEE_5] =                      Location::SilverRupee(RC_ICE_CAVERN_SCYTHE_SILVER_RUPEE_5,                  RCQUEST_VANILLA, RCAREA_ICE_CAVERN,                    SCENE_ICE_CAVERN,                 TWO_ACTOR_PARAMS(1,   -143),         "Ice Scythe Room Silver Rupee 5",        RHT_ICE_CAVERN_SCYTHE_SILVER_RUPEES,               SpoilerCollectionCheck::RandomizerInf(RAND_INF_ICE_CAVERN_SCYTHE_SILVER_RUPEE_5));
    locationTable[RC_ICE_CAVERN_PUSH_BLOCK_SILVER_RUPEE_1] =                  Location::SilverRupee(RC_ICE_CAVERN_PUSH_BLOCK_SILVER_RUPEE_1,              RCQUEST_VANILLA, RCAREA_ICE_CAVERN,                    SCENE_ICE_CAVERN,                 TWO_ACTOR_PARAMS(-1040, -485),       "Push Block Room Silver Rupee 1",        RHT_ICE_CAVERN_PUSH_BLOCK_SILVER_RUPEES,           SpoilerCollectionCheck::RandomizerInf(RAND_INF_ICE_CAVERN_PUSH_BLOCK_SILVER_RUPEE_1));
    locationTable[RC_ICE_CAVERN_PUSH_BLOCK_SILVER_RUPEE_2] =                  Location::SilverRupee(RC_ICE_CAVERN_PUSH_BLOCK_SILVER_RUPEE_2,              RCQUEST_VANILLA, RCAREA_ICE_CAVERN,                    SCENE_ICE_CAVERN,                 TWO_ACTOR_PARAMS(-1120, -1577),      "Push Block Room Silver Rupee 2",        RHT_ICE_CAVERN_PUSH_BLOCK_SILVER_RUPEES,           SpoilerCollectionCheck::RandomizerInf(RAND_INF_ICE_CAVERN_PUSH_BLOCK_SILVER_RUPEE_2));
    locationTable[RC_ICE_CAVERN_PUSH_BLOCK_SILVER_RUPEE_3] =                  Location::SilverRupee(RC_ICE_CAVERN_PUSH_BLOCK_SILVER_RUPEE_3,              RCQUEST_VANILLA, RCAREA_ICE_CAVERN,                    SCENE_ICE_CAVERN,                 TWO_ACTOR_PARAMS(-1294, -899),       "Push Block Room Silver Rupee 3",        RHT_ICE_CAVERN_PUSH_BLOCK_SILVER_RUPEES,           SpoilerCollectionCheck::RandomizerInf(RAND_INF_ICE_CAVERN_PUSH_BLOCK_SILVER_RUPEE_3));
    locationTable[RC_ICE_CAVERN_PUSH_BLOCK_SILVER_RUPEE_4] =                  Location::SilverRupee(RC_ICE_CAVERN_PUSH_BLOCK_SILVER_RUPEE_4,              RCQUEST_VANILLA, RCAREA_ICE_CAVERN,                    SCENE_ICE_CAVERN,                 TWO_ACTOR_PARAMS(-1558, -951),       "Push Block Room Silver Rupee 4",        RHT_ICE_CAVERN_PUSH_BLOCK_SILVER_RUPEES,           SpoilerCollectionCheck::RandomizerInf(RAND_INF_ICE_CAVERN_PUSH_BLOCK_SILVER_RUPEE_4));
    locationTable[RC_ICE_CAVERN_PUSH_BLOCK_SILVER_RUPEE_5] =                  Location::SilverRupee(RC_ICE_CAVERN_PUSH_BLOCK_SILVER_RUPEE_5,              RCQUEST_VANILLA, RCAREA_ICE_CAVERN,                    SCENE_ICE_CAVERN,                 TWO_ACTOR_PARAMS(-1676, -552),       "Push Block Room Silver Rupee 5",        RHT_ICE_CAVERN_PUSH_BLOCK_SILVER_RUPEES,           SpoilerCollectionCheck::RandomizerInf(RAND_INF_ICE_CAVERN_PUSH_BLOCK_SILVER_RUPEE_5));

    locationTable[RC_GERUDO_TRAINING_GROUND_BOULDER_SILVER_RUPEE_1] =         Location::SilverRupee(RC_GERUDO_TRAINING_GROUND_BOULDER_SILVER_RUPEE_1,     RCQUEST_VANILLA, RCAREA_GERUDO_TRAINING_GROUND,        SCENE_GERUDO_TRAINING_GROUND,     TWO_ACTOR_PARAMS(-1887, -2134),      "GTG Boulder Silver Rupee 1",            RHT_GERUDO_TRAINING_GROUND_BOULDER_SILVER_RUPEES,  SpoilerCollectionCheck::RandomizerInf(RAND_INF_GERUDO_TRAINING_GROUND_BOULDER_SILVER_RUPEE_1));
    locationTable[RC_GERUDO_TRAINING_GROUND_BOULDER_SILVER_RUPEE_2] =         Location::SilverRupee(RC_GERUDO_TRAINING_GROUND_BOULDER_SILVER_RUPEE_2,     RCQUEST_VANILLA, RCAREA_GERUDO_TRAINING_GROUND,        SCENE_GERUDO_TRAINING_GROUND,     TWO_ACTOR_PARAMS(-1627, -1462),      "GTG Boulder Silver Rupee 2",            RHT_GERUDO_TRAINING_GROUND_BOULDER_SILVER_RUPEES,  SpoilerCollectionCheck::RandomizerInf(RAND_INF_GERUDO_TRAINING_GROUND_BOULDER_SILVER_RUPEE_2));
    locationTable[RC_GERUDO_TRAINING_GROUND_BOULDER_SILVER_RUPEE_3] =         Location::SilverRupee(RC_GERUDO_TRAINING_GROUND_BOULDER_SILVER_RUPEE_3,     RCQUEST_VANILLA, RCAREA_GERUDO_TRAINING_GROUND,        SCENE_GERUDO_TRAINING_GROUND,     TWO_ACTOR_PARAMS(-1579, -999),       "GTG Boulder Silver Rupee 3",            RHT_GERUDO_TRAINING_GROUND_BOULDER_SILVER_RUPEES,  SpoilerCollectionCheck::RandomizerInf(RAND_INF_GERUDO_TRAINING_GROUND_BOULDER_SILVER_RUPEE_3));
    locationTable[RC_GERUDO_TRAINING_GROUND_BOULDER_SILVER_RUPEE_4] =         Location::SilverRupee(RC_GERUDO_TRAINING_GROUND_BOULDER_SILVER_RUPEE_4,     RCQUEST_VANILLA, RCAREA_GERUDO_TRAINING_GROUND,        SCENE_GERUDO_TRAINING_GROUND,     TWO_ACTOR_PARAMS(-1886, -956),       "GTG Boulder Silver Rupee 4",            RHT_GERUDO_TRAINING_GROUND_BOULDER_SILVER_RUPEES,  SpoilerCollectionCheck::RandomizerInf(RAND_INF_GERUDO_TRAINING_GROUND_BOULDER_SILVER_RUPEE_4));
    locationTable[RC_GERUDO_TRAINING_GROUND_BOULDER_SILVER_RUPEE_5] =         Location::SilverRupee(RC_GERUDO_TRAINING_GROUND_BOULDER_SILVER_RUPEE_5,     RCQUEST_VANILLA, RCAREA_GERUDO_TRAINING_GROUND,        SCENE_GERUDO_TRAINING_GROUND,     TWO_ACTOR_PARAMS(-1332, -992),       "GTG Boulder Silver Rupee 5",            RHT_GERUDO_TRAINING_GROUND_BOULDER_SILVER_RUPEES,  SpoilerCollectionCheck::RandomizerInf(RAND_INF_GERUDO_TRAINING_GROUND_BOULDER_SILVER_RUPEE_5));
    locationTable[RC_GERUDO_TRAINING_GROUND_LAVA_SILVER_RUPEE_1] =            Location::SilverRupee(RC_GERUDO_TRAINING_GROUND_LAVA_SILVER_RUPEE_1,        RCQUEST_VANILLA, RCAREA_GERUDO_TRAINING_GROUND,        SCENE_GERUDO_TRAINING_GROUND,     TWO_ACTOR_PARAMS(1560, -1861),       "Lava Silver Rupee 1",                   RHT_GERUDO_TRAINING_GROUND_LAVA_SILVER_RUPEES,     SpoilerCollectionCheck::RandomizerInf(RAND_INF_GERUDO_TRAINING_GROUND_LAVA_SILVER_RUPEE_1));
    locationTable[RC_GERUDO_TRAINING_GROUND_LAVA_SILVER_RUPEE_2] =            Location::SilverRupee(RC_GERUDO_TRAINING_GROUND_LAVA_SILVER_RUPEE_2,        RCQUEST_VANILLA, RCAREA_GERUDO_TRAINING_GROUND,        SCENE_GERUDO_TRAINING_GROUND,     TWO_ACTOR_PARAMS(1437, -2193),       "Lava Silver Rupee 2",                   RHT_GERUDO_TRAINING_GROUND_LAVA_SILVER_RUPEES,     SpoilerCollectionCheck::RandomizerInf(RAND_INF_GERUDO_TRAINING_GROUND_LAVA_SILVER_RUPEE_2));
    locationTable[RC_GERUDO_TRAINING_GROUND_LAVA_SILVER_RUPEE_3] =            Location::SilverRupee(RC_GERUDO_TRAINING_GROUND_LAVA_SILVER_RUPEE_3,        RCQUEST_VANILLA, RCAREA_GERUDO_TRAINING_GROUND,        SCENE_GERUDO_TRAINING_GROUND,     TWO_ACTOR_PARAMS(1134, -1841),       "Lava Silver Rupee 3",                   RHT_GERUDO_TRAINING_GROUND_LAVA_SILVER_RUPEES,     SpoilerCollectionCheck::RandomizerInf(RAND_INF_GERUDO_TRAINING_GROUND_LAVA_SILVER_RUPEE_3));
    locationTable[RC_GERUDO_TRAINING_GROUND_LAVA_SILVER_RUPEE_4] =            Location::SilverRupee(RC_GERUDO_TRAINING_GROUND_LAVA_SILVER_RUPEE_4,        RCQUEST_VANILLA, RCAREA_GERUDO_TRAINING_GROUND,        SCENE_GERUDO_TRAINING_GROUND,     TWO_ACTOR_PARAMS(1558, -1370),       "Lava Silver Rupee 4",                   RHT_GERUDO_TRAINING_GROUND_LAVA_SILVER_RUPEES,     SpoilerCollectionCheck::RandomizerInf(RAND_INF_GERUDO_TRAINING_GROUND_LAVA_SILVER_RUPEE_4));
    locationTable[RC_GERUDO_TRAINING_GROUND_LAVA_SILVER_RUPEE_5] =            Location::SilverRupee(RC_GERUDO_TRAINING_GROUND_LAVA_SILVER_RUPEE_5,        RCQUEST_VANILLA, RCAREA_GERUDO_TRAINING_GROUND,        SCENE_GERUDO_TRAINING_GROUND,     TWO_ACTOR_PARAMS(1320, -1248),       "Lava Silver Rupee 5",                   RHT_GERUDO_TRAINING_GROUND_LAVA_SILVER_RUPEES,     SpoilerCollectionCheck::RandomizerInf(RAND_INF_GERUDO_TRAINING_GROUND_LAVA_SILVER_RUPEE_5));
    locationTable[RC_GERUDO_TRAINING_GROUND_TOILET_SILVER_RUPEE_1] =          Location::SilverRupee(RC_GERUDO_TRAINING_GROUND_TOILET_SILVER_RUPEE_1,      RCQUEST_VANILLA, RCAREA_GERUDO_TRAINING_GROUND,        SCENE_GERUDO_TRAINING_GROUND,     TWO_ACTOR_PARAMS(2160, -1315),       "Toilet Silver Rupee 1",                 RHT_GERUDO_TRAINING_GROUND_TOILET_SILVER_RUPEES,   SpoilerCollectionCheck::RandomizerInf(RAND_INF_GERUDO_TRAINING_GROUND_TOILET_SILVER_RUPEE_1));
    locationTable[RC_GERUDO_TRAINING_GROUND_TOILET_SILVER_RUPEE_2] =          Location::SilverRupee(RC_GERUDO_TRAINING_GROUND_TOILET_SILVER_RUPEE_2,      RCQUEST_VANILLA, RCAREA_GERUDO_TRAINING_GROUND,        SCENE_GERUDO_TRAINING_GROUND,     TWO_ACTOR_PARAMS(2078, -1458),       "Toilet Silver Rupee 2",                 RHT_GERUDO_TRAINING_GROUND_TOILET_SILVER_RUPEES,   SpoilerCollectionCheck::RandomizerInf(RAND_INF_GERUDO_TRAINING_GROUND_TOILET_SILVER_RUPEE_2));
    locationTable[RC_GERUDO_TRAINING_GROUND_TOILET_SILVER_RUPEE_3] =          Location::SilverRupee(RC_GERUDO_TRAINING_GROUND_TOILET_SILVER_RUPEE_3,      RCQUEST_VANILLA, RCAREA_GERUDO_TRAINING_GROUND,        SCENE_GERUDO_TRAINING_GROUND,     TWO_ACTOR_PARAMS(2453, -1612),       "Toilet Silver Rupee 3",                 RHT_GERUDO_TRAINING_GROUND_TOILET_SILVER_RUPEES,   SpoilerCollectionCheck::RandomizerInf(RAND_INF_GERUDO_TRAINING_GROUND_TOILET_SILVER_RUPEE_3));
    locationTable[RC_GERUDO_TRAINING_GROUND_TOILET_SILVER_RUPEE_4] =          Location::SilverRupee(RC_GERUDO_TRAINING_GROUND_TOILET_SILVER_RUPEE_4,      RCQUEST_VANILLA, RCAREA_GERUDO_TRAINING_GROUND,        SCENE_GERUDO_TRAINING_GROUND,     TWO_ACTOR_PARAMS(2497, -1465),       "Toilet Silver Rupee 4",                 RHT_GERUDO_TRAINING_GROUND_TOILET_SILVER_RUPEES,   SpoilerCollectionCheck::RandomizerInf(RAND_INF_GERUDO_TRAINING_GROUND_TOILET_SILVER_RUPEE_4));
    locationTable[RC_GERUDO_TRAINING_GROUND_TOILET_SILVER_RUPEE_5] =          Location::SilverRupee(RC_GERUDO_TRAINING_GROUND_TOILET_SILVER_RUPEE_5,      RCQUEST_VANILLA, RCAREA_GERUDO_TRAINING_GROUND,        SCENE_GERUDO_TRAINING_GROUND,     TWO_ACTOR_PARAMS(2308, -1464),       "Toilet Silver Rupee 5",                 RHT_GERUDO_TRAINING_GROUND_TOILET_SILVER_RUPEES,   SpoilerCollectionCheck::RandomizerInf(RAND_INF_GERUDO_TRAINING_GROUND_TOILET_SILVER_RUPEE_5));

    locationTable[RC_BOTTOM_OF_THE_WELL_BASEMENT_SILVER_RUPEE_1] =            Location::SilverRupee(RC_BOTTOM_OF_THE_WELL_BASEMENT_SILVER_RUPEE_1,       RCQUEST_VANILLA, RCAREA_BOTTOM_OF_THE_WELL,             SCENE_BOTTOM_OF_THE_WELL,         TWO_ACTOR_PARAMS(-259, -234),        "Basement Silver Rupee 1",               RHT_BOTTOM_OF_THE_WELL_BASEMENT_SILVER_RUPEES,     SpoilerCollectionCheck::RandomizerInf(RAND_INF_BOTTOM_OF_THE_WELL_BASEMENT_SILVER_RUPEE_1));
    locationTable[RC_BOTTOM_OF_THE_WELL_BASEMENT_SILVER_RUPEE_2] =            Location::SilverRupee(RC_BOTTOM_OF_THE_WELL_BASEMENT_SILVER_RUPEE_2,       RCQUEST_VANILLA, RCAREA_BOTTOM_OF_THE_WELL,             SCENE_BOTTOM_OF_THE_WELL,         TWO_ACTOR_PARAMS(-402, -401),        "Basement Silver Rupee 2",               RHT_BOTTOM_OF_THE_WELL_BASEMENT_SILVER_RUPEES,     SpoilerCollectionCheck::RandomizerInf(RAND_INF_BOTTOM_OF_THE_WELL_BASEMENT_SILVER_RUPEE_2));
    locationTable[RC_BOTTOM_OF_THE_WELL_BASEMENT_SILVER_RUPEE_3] =            Location::SilverRupee(RC_BOTTOM_OF_THE_WELL_BASEMENT_SILVER_RUPEE_3,       RCQUEST_VANILLA, RCAREA_BOTTOM_OF_THE_WELL,             SCENE_BOTTOM_OF_THE_WELL,         TWO_ACTOR_PARAMS(-560, -291),        "Basement Silver Rupee 3",               RHT_BOTTOM_OF_THE_WELL_BASEMENT_SILVER_RUPEES,     SpoilerCollectionCheck::RandomizerInf(RAND_INF_BOTTOM_OF_THE_WELL_BASEMENT_SILVER_RUPEE_3));
    locationTable[RC_BOTTOM_OF_THE_WELL_BASEMENT_SILVER_RUPEE_4] =            Location::SilverRupee(RC_BOTTOM_OF_THE_WELL_BASEMENT_SILVER_RUPEE_4,       RCQUEST_VANILLA, RCAREA_BOTTOM_OF_THE_WELL,             SCENE_BOTTOM_OF_THE_WELL,         TWO_ACTOR_PARAMS(-614, -297),        "Basement Silver Rupee 4",               RHT_BOTTOM_OF_THE_WELL_BASEMENT_SILVER_RUPEES,     SpoilerCollectionCheck::RandomizerInf(RAND_INF_BOTTOM_OF_THE_WELL_BASEMENT_SILVER_RUPEE_4));
    locationTable[RC_BOTTOM_OF_THE_WELL_BASEMENT_SILVER_RUPEE_5] =            Location::SilverRupee(RC_BOTTOM_OF_THE_WELL_BASEMENT_SILVER_RUPEE_5,       RCQUEST_VANILLA, RCAREA_BOTTOM_OF_THE_WELL,             SCENE_BOTTOM_OF_THE_WELL,         TWO_ACTOR_PARAMS(-796, -150),        "Basement Silver Rupee 5",               RHT_BOTTOM_OF_THE_WELL_BASEMENT_SILVER_RUPEES,     SpoilerCollectionCheck::RandomizerInf(RAND_INF_BOTTOM_OF_THE_WELL_BASEMENT_SILVER_RUPEE_5));

    // clang-format on
}

static RegisterShipInitFunc registerShuffleSilverRupees(RegisterShuffleSilverRupees, { "IS_RANDO" });
static RegisterShipInitFunc registerSilverRupeeLocations(Rando::StaticData::RegisterSilverRupeeLocations);
