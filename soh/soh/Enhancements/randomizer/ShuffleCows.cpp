#include <soh/OTRGlobals.h>
#include <soh/GameVersions.h>
#include "soh/ResourceManagerHelpers.h"
#include "soh/ObjectExtension/ObjectExtension.h"
#include "soh/ObjectExtension/ActorListIndex.h"
#include "item_category_adj.h"
#include "particle_cmc.h"
#include "soh/Enhancements/randomizer/randomizer.h"
#include "soh/Enhancements/randomizer/RCToRandInf.h"

extern "C" {
#include "src/overlays/actors/ovl_En_Cow/z_en_cow.h"
extern PlayState* gPlayState;
}

void EnCow_MoveForRandomizer(EnCow* enCow, PlayState* play) {
    bool moved = false;

    // Don't reposition the tail
    if (enCow->actor.params != 0) {
        return;
    }

    if (play->sceneNum == SCENE_LON_LON_BUILDINGS && enCow->actor.world.pos.x == -108 &&
        enCow->actor.world.pos.z == -65) {
        // Move left cow in lon lon tower
        enCow->actor.world.pos.x = -229.0f;
        enCow->actor.world.pos.z = 157.0f;
        enCow->actor.shape.rot.y = 15783;
        moved = true;
    } else if (play->sceneNum == SCENE_STABLE && enCow->actor.world.pos.x == -3 && enCow->actor.world.pos.z == -254) {
        // Move right cow in lon lon stable
        enCow->actor.world.pos.x += 119.0f;
        moved = true;
    }

    if (moved) {
        // Reposition collider
        EnCow_SetColliderPos(enCow);
    }
}

static CheckIdentity IdentifyCow(s32 sceneNum, s32 posX, s32 posZ) {
    CheckIdentity cowIdentity;

    cowIdentity.randomizerInf = RAND_INF_MAX;
    cowIdentity.randomizerCheck = RC_UNKNOWN_CHECK;

    s32 actorParams = 0x00;
    // Only need to pass params if in a scene with two cows
    if (sceneNum == SCENE_GROTTOS || sceneNum == SCENE_STABLE || sceneNum == SCENE_LON_LON_BUILDINGS) {
        actorParams = TWO_ACTOR_PARAMS(posX, posZ);
    }

    Rando::Location* location =
        OTRGlobals::Instance->gRandomizer->GetCheckObjectFromActor(ACTOR_EN_COW, sceneNum, actorParams);

    if (location->GetRandomizerCheck() != RC_UNKNOWN_CHECK) {
        cowIdentity.randomizerInf = rcToRandomizerInf[location->GetRandomizerCheck()];
        cowIdentity.randomizerCheck = location->GetRandomizerCheck();
    }

    return cowIdentity;
}

static void ShuffledCow_RandomizerDraw(EnCow* enCow, Color_RGBA8* primColor, Color_RGBA8* secColor,
                                          Color_RGBA8* envColor, CheckIdentity* cowIdentity) {
    Vec3f pos;
    static Vec3f velocity = { 0.0f, 0.0f, 0.0f };
    static Vec3f accel = { 0.0f, 0.0f, 0.0f };

    velocity.y = -0.05f;
    accel.y = -0.025f;

    if (gPlayState->sceneNum == SCENE_IMPAS_HOUSE) {
        pos.x = Rand_CenteredFloat(1.0f) + enCow->actor.world.pos.x;
        pos.y = (Rand_ZeroOne() * 25.0f) + enCow->actor.world.pos.y + 65;
        pos.z = Rand_CenteredFloat(1.0f) + enCow->actor.world.pos.z + 50; // Reposition particles for better visibility
    } else {
        pos.x = Rand_CenteredFloat(1.0f) + enCow->actor.world.pos.x;
        pos.y = (Rand_ZeroOne() * 25.0f) + enCow->actor.world.pos.y + 80;
        pos.z = Rand_CenteredFloat(1.0f) + enCow->actor.world.pos.z;
    }
    EffectSsKiraKira_SpawnFocused(gPlayState, &pos, &velocity, &accel, secColor, envColor, 2000, 100);
    EffectSsKiraKira_SpawnFocused(gPlayState, &pos, &velocity, &accel, primColor, envColor, 2000, 100);
}

uint8_t ShuffledCow_RandomizerHoldsItem(EnCow* enCow, PlayState* play) {
    const CheckIdentity* cowIdentity =
        ObjectExtension::GetInstance().Get<CheckIdentity>(&enCow->actor);
    if (cowIdentity == nullptr) {
        auto newIdentity = IdentifyCow(
            gPlayState->sceneNum, static_cast<int32_t>(enCow->actor.world.pos.x),
            static_cast<int32_t>(enCow->actor.world.pos.z));

        ObjectExtension::GetInstance().Set<CheckIdentity>(enCow, std::move(newIdentity));
        cowIdentity = ObjectExtension::GetInstance().Get<CheckIdentity>(enCow);
        if (cowIdentity == nullptr) {
            return false;
        }
    }

    // For some reason Impa's House and Gerudo Valley have an invisible cow, so don't draw particles over it
    if ((gPlayState->sceneNum == SCENE_IMPAS_HOUSE && enCow->actor.world.pos.y == 57) ||
        (gPlayState->sceneNum == SCENE_GERUDO_VALLEY && enCow->actor.world.pos.y == -1994)) { // Adjust the Y position for Gerudo Valley as needed
        return false;
    }

    if (!IS_RANDO || Flags_GetRandomizerInf(cowIdentity->randomizerInf) || !CVarGetInteger(CVAR_ENHANCEMENT("ChestSizeAndTextureMatchContents"), 0) ||
        cowIdentity->randomizerCheck == RC_UNKNOWN_CHECK || (Rando::Context::GetInstance()->GetItemLocation(cowIdentity->randomizerCheck)->IsExcluded() && CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("JunkIsUnshuffled"), 0))) {
        return false;
    } else {
        return true;
    }
}

void ShuffledCow_RandomizerDrawSetup(void* refActor) {
    GetItemCategory getItemCategory;
    EnCow* enCow = static_cast<EnCow*>(refActor);

    // If not a randomized item or too far, don't draw.
    if (!ShuffledCow_RandomizerHoldsItem(enCow, gPlayState) ||
        !CVarGetInteger(CVAR_ENHANCEMENT("CMCCowParticles"), 0)) {
        return;
    } else if (enCow->actor.xzDistToPlayer > (1000.0f * CVarGetInteger(CVAR_ENHANCEMENT("DisableDrawDistance"), 0))) {
        return;
    }

    int requiresStoneAgony = CVarGetInteger(CVAR_ENHANCEMENT("ChestSizeDependsStoneOfAgony"), 0);
    int isNotCMC = (requiresStoneAgony && !CHECK_QUEST_ITEM(QUEST_STONE_OF_AGONY));

    Color_RGBA8 primColor;
    Color_RGBA8 secColor;
    Color_RGBA8 envColor;

    const auto cowIdentity = ObjectExtension::GetInstance().Get<CheckIdentity>(refActor);
    if (cowIdentity == nullptr) {
        return;
    }

    GetItemEntry enCowItem =
        Rando::Context::GetInstance()->GetFinalGIEntry(cowIdentity->randomizerCheck, true, GI_NONE);
    getItemCategory = Randomizer_AdjustItemCategory(enCowItem);

    if (!isNotCMC) {
        primColor = Randomizer_GetParticleCMCColor(getItemCategory, COLOR_PRIMARY);
        secColor = Randomizer_GetParticleCMCColor(getItemCategory, COLOR_SECONDARY);
        envColor = Randomizer_GetParticleCMCColor(getItemCategory, COLOR_FLARE);
        ShuffledCow_RandomizerDraw(enCow, &primColor, &secColor, &envColor, cowIdentity);
    }
}

void RegisterShuffleCows() {
    bool shouldRegister = IS_RANDO && RAND_GET_OPTION(RSK_SHUFFLE_COWS);

    // Draw particle effect on Cow to indicate a randomized item
    COND_ID_HOOK(OnActorUpdate, ACTOR_EN_COW, shouldRegister, ShuffledCow_RandomizerDrawSetup);

    COND_VB_SHOULD(VB_GIVE_ITEM_FROM_COW, shouldRegister, {
        EnCow* enCow = va_arg(args, EnCow*);
        CheckIdentity cowIdentity = IdentifyCow(gPlayState->sceneNum, static_cast<int32_t>(enCow->actor.world.pos.x),
                                                static_cast<int32_t>(enCow->actor.world.pos.z));
        // Has this cow already rewarded an item?
        if (!Flags_GetRandomizerInf(cowIdentity.randomizerInf) && !(Rando::Context::GetInstance()->GetItemLocation(cowIdentity.randomizerCheck)->IsExcluded() && CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("JunkIsUnshuffled"), 0))) {
            Flags_SetRandomizerInf(cowIdentity.randomizerInf);
            // setting the ocarina mode here prevents intermittent issues
            // with the item get not triggering until walking away
            gPlayState->msgCtx.ocarinaMode = OCARINA_MODE_00;
            *should = false;
        }
    });

    COND_VB_SHOULD(VB_DESPAWN_HORSE_RACE_COW, shouldRegister, {
        EnCow* enCow = va_arg(args, EnCow*);
        // If this is a cow we have to move, then move it now.
        EnCow_MoveForRandomizer(enCow, gPlayState);
    });
}

static RegisterShipInitFunc registerShuffleCows(RegisterShuffleCows, { "IS_RANDO" });

void Rando::StaticData::RegisterCowLocations() {
    static bool registered = false;
    if (registered)
        return;
    registered = true;
    // clang-format off
    locationTable[RC_KF_LINKS_HOUSE_COW] =      Location::Base(RC_KF_LINKS_HOUSE_COW,      RCQUEST_BOTH, RCTYPE_COW,                              ACTOR_EN_COW, SCENE_LINKS_HOUSE,       0x00,                               "Links House Cow",   RHT_KF_LINKS_HOUSE_COW,      RG_MILK, SpoilerCollectionCheck::RandomizerInf(RAND_INF_COWS_MILKED_KF_LINKS_HOUSE_COW));
    locationTable[RC_HF_COW_GROTTO_COW] =       Location::Base(RC_HF_COW_GROTTO_COW,       RCQUEST_BOTH, RCTYPE_COW, RCAREA_HYRULE_FIELD,         ACTOR_EN_COW, SCENE_GROTTOS,           TWO_ACTOR_PARAMS(3485, -291),       "Cow Grotto Cow",    RHT_HF_COW_GROTTO_COW,       RG_MILK, SpoilerCollectionCheck::RandomizerInf(RAND_INF_COWS_MILKED_HF_COW_GROTTO_COW));
    locationTable[RC_LLR_STABLES_LEFT_COW] =    Location::Base(RC_LLR_STABLES_LEFT_COW,    RCQUEST_BOTH, RCTYPE_COW,                              ACTOR_EN_COW, SCENE_STABLE,            TWO_ACTOR_PARAMS(-122, -254),       "Stables Left Cow",  RHT_LLR_STABLES_LEFT_COW,    RG_MILK, SpoilerCollectionCheck::RandomizerInf(RAND_INF_COWS_MILKED_LLR_STABLES_LEFT_COW));
    locationTable[RC_LLR_STABLES_RIGHT_COW] =   Location::Base(RC_LLR_STABLES_RIGHT_COW,   RCQUEST_BOTH, RCTYPE_COW,                              ACTOR_EN_COW, SCENE_STABLE,            TWO_ACTOR_PARAMS(116, -254),        "Stables Right Cow", RHT_LLR_STABLES_RIGHT_COW,   RG_MILK, SpoilerCollectionCheck::RandomizerInf(RAND_INF_COWS_MILKED_LLR_STABLES_RIGHT_COW));
    locationTable[RC_LLR_TOWER_LEFT_COW] =      Location::Base(RC_LLR_TOWER_LEFT_COW,      RCQUEST_BOTH, RCTYPE_COW,                              ACTOR_EN_COW, SCENE_LON_LON_BUILDINGS, TWO_ACTOR_PARAMS(-229, 157),        "Tower Left Cow",    RHT_LLR_TOWER_LEFT_COW,      RG_MILK, SpoilerCollectionCheck::RandomizerInf(RAND_INF_COWS_MILKED_LLR_TOWER_LEFT_COW));
    locationTable[RC_LLR_TOWER_RIGHT_COW] =     Location::Base(RC_LLR_TOWER_RIGHT_COW,     RCQUEST_BOTH, RCTYPE_COW,                              ACTOR_EN_COW, SCENE_LON_LON_BUILDINGS, TWO_ACTOR_PARAMS(-142, -140),       "Tower Right Cow",   RHT_LLR_TOWER_RIGHT_COW,     RG_MILK, SpoilerCollectionCheck::RandomizerInf(RAND_INF_COWS_MILKED_LLR_TOWER_RIGHT_COW));
    locationTable[RC_KAK_IMPAS_HOUSE_COW] =     Location::Base(RC_KAK_IMPAS_HOUSE_COW,     RCQUEST_BOTH, RCTYPE_COW,                              ACTOR_EN_COW, SCENE_IMPAS_HOUSE,       0x00,                               "Impas House Cow",   RHT_KAK_IMPAS_HOUSE_COW,     RG_MILK, SpoilerCollectionCheck::RandomizerInf(RAND_INF_COWS_MILKED_KAK_IMPAS_HOUSE_COW));
    locationTable[RC_DMT_COW_GROTTO_COW] =      Location::Base(RC_DMT_COW_GROTTO_COW,      RCQUEST_BOTH, RCTYPE_COW, RCAREA_DEATH_MOUNTAIN_TRAIL, ACTOR_EN_COW, SCENE_GROTTOS,           TWO_ACTOR_PARAMS(2444, -471),       "Cow Grotto Cow",    RHT_DMT_COW_GROTTO_COW,      RG_MILK, SpoilerCollectionCheck::RandomizerInf(RAND_INF_COWS_MILKED_DMT_COW_GROTTO_COW));
    locationTable[RC_GV_COW] =                  Location::Base(RC_GV_COW,                  RCQUEST_BOTH, RCTYPE_COW,                              ACTOR_EN_COW, SCENE_GERUDO_VALLEY,     0x00,                               "Cow",               RHT_GV_COW,                  RG_MILK, SpoilerCollectionCheck::RandomizerInf(RAND_INF_COWS_MILKED_GV_COW));
    locationTable[RC_JABU_JABUS_BELLY_MQ_COW] = Location::Base(RC_JABU_JABUS_BELLY_MQ_COW, RCQUEST_MQ,   RCTYPE_COW,                              ACTOR_EN_COW, SCENE_JABU_JABU,         0x00,                               "MQ Cow",            RHT_JABU_JABUS_BELLY_MQ_COW, RG_MILK, SpoilerCollectionCheck::RandomizerInf(RAND_INF_COWS_MILKED_JABU_JABUS_BELLY_MQ_COW));
    // clang-format-on
}

static RegisterShipInitFunc registerCowLocations(Rando::StaticData::RegisterCowLocations);
