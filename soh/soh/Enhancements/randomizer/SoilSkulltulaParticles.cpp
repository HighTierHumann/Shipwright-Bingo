#include "soh/OTRGlobals.h"
#include "randomizer_grotto.h"
#include "draw.h"
#include "soh/cvar_prefixes.h"
#include "static_data.h"
#include "soh/Enhancements/item-tables/ItemTableTypes.h"
#include "soh/ObjectExtension/ObjectExtension.h"
#include "soh/ObjectExtension/ActorListIndex.h"
#include "item_category_adj.h"
#include "particle_cmc.h"
#include "macros.h"
#include "z64actor_enum.h"
#include "soh/Enhancements/randomizer/randomizer.h"
#include "soh/Enhancements/randomizer/RCToRandInf.h"

extern "C" {
#include "src/overlays/actors/ovl_Obj_Bean/z_obj_bean.h"
}

CheckIdentity IdentifySoilSkulltula(s32 sceneNum, s32 beanActorParams) {
    struct CheckIdentity soilSkullIdentity;
    soilSkullIdentity.randomizerInf = RAND_INF_MAX;
    soilSkullIdentity.randomizerCheck = RC_UNKNOWN_CHECK;

    // Bean-patch GS checks are uniquely identified by (sceneId, switchFlag).
    // The switch flag lives in bits [5:0] of the bean actor's params, confirmed by
    // ObjBean_WaitForBean: Flags_SetSwitch(play, this->dyna.actor.params & 0x3F).
    // Switch flags sourced from BeanPlanted() in location_access.cpp.
    const s32 switchFlag = beanActorParams & 0x3F;

    struct BeanPatchEntry {
        SceneID scene;
        s32 flag;
        RandomizerCheck rc;
    };
    static constexpr std::array<BeanPatchEntry, 10> sBeanPatchGSTable = { {
        { SCENE_GRAVEYARD, 3, RC_GRAVEYARD_GS_BEAN_PATCH },
        { SCENE_KOKIRI_FOREST, 9, RC_KF_GS_BEAN_PATCH },
        { SCENE_LOST_WOODS, 4, RC_LW_GS_BEAN_PATCH_NEAR_BRIDGE },
        { SCENE_LOST_WOODS, 18, RC_LW_GS_BEAN_PATCH_NEAR_THEATER },
        { SCENE_DEATH_MOUNTAIN_TRAIL, 6, RC_DMT_GS_BEAN_PATCH },
        { SCENE_LAKE_HYLIA, 1, RC_LH_GS_BEAN_PATCH },
        { SCENE_GERUDO_VALLEY, 3, RC_GV_GS_BEAN_PATCH },
        { SCENE_DEATH_MOUNTAIN_CRATER, 3, RC_DMC_GS_BEAN_PATCH },
        { SCENE_DESERT_COLOSSUS, 24, RC_COLOSSUS_GS_BEAN_PATCH },
    } };

    for (const auto& entry : sBeanPatchGSTable) {
        if (static_cast<s32>(entry.scene) == sceneNum && entry.flag == switchFlag) {
            soilSkullIdentity.randomizerCheck = entry.rc;
            return soilSkullIdentity;
        }
    }

    LUSLOG_WARN("IdentifySoilSkulltula: no bean-patch GS entry for sceneNum=%d switchFlag=%d.", sceneNum, switchFlag);
    return soilSkullIdentity;
}

// Cache the bean patch's CheckIdentity at actor init time using its params & 0x3F switch flag.
// The skulltula actor (ACTOR_EN_SW) does not exist yet at this point; the identity comes
// entirely from the static (sceneNum, switchFlag) -> RandomizerCheck table.
static void SoilSkulltula_CacheIdentity(void* refActor) {
    ObjBean* beanActor = static_cast<ObjBean*>(refActor);
    auto identity = IdentifySoilSkulltula(gPlayState->sceneNum, beanActor->dyna.actor.params);
    ObjectExtension::GetInstance().Set<CheckIdentity>(&beanActor->dyna.actor, std::move(identity));
}

static void SoilSkulltula_RandomizerDraw(ObjBean* beanActor, Color_RGBA8* primColor, Color_RGBA8* secColor,
                                         Color_RGBA8* envColor) {
    Vec3f pos;
    static Vec3f velocity = { 0.0f, 0.0f, 0.0f };
    static Vec3f accel = { 0.0f, 0.0f, 0.0f };

    velocity.y = -0.05f;
    accel.y = -0.025f;

    pos.x = Rand_CenteredFloat(1.0f) + beanActor->dyna.actor.world.pos.x;
    pos.y = (Rand_ZeroOne() * 25.0f) + beanActor->dyna.actor.world.pos.y + 10;
    pos.z = Rand_CenteredFloat(1.0f) + beanActor->dyna.actor.world.pos.z;
    EffectSsKiraKira_SpawnFocused(gPlayState, &pos, &velocity, &accel, secColor, envColor, 2000, 100);
    EffectSsKiraKira_SpawnFocused(gPlayState, &pos, &velocity, &accel, primColor, envColor, 2000, 100);
}

void SoilSkulltula_RandomizerDrawSetup(void* refActor) {
    ObjBean* beanActor = static_cast<ObjBean*>(refActor);

    if (!CVarGetInteger(CVAR_ENHANCEMENT("CMCSoilGSParticles"), 0)) {
        return;
    } else if (beanActor->dyna.actor.xzDistToPlayer > (1000.0f * CVarGetInteger(CVAR_ENHANCEMENT("DisableDrawDistance"), 0))) {
        return;
    }

    const CheckIdentity* identity = ObjectExtension::GetInstance().Get<CheckIdentity>(&beanActor->dyna.actor);
    if (identity == nullptr || identity->randomizerCheck == RC_UNKNOWN_CHECK) {
        return;
    }

    auto ctx = Rando::Context::GetInstance();
    if (!ctx) {
        return;
    }

    Rando::ItemLocation* loc = ctx->GetItemLocation(identity->randomizerCheck);
    if (loc == nullptr || loc->HasObtained()) {
        return;
    }

    RandomizerGet placedGet = loc->GetPlacedRandomizerGet();
    if (placedGet == RG_NONE) {
        return;
    }

    Rando::Item& placedItem = Rando::StaticData::RetrieveItem(placedGet);
    auto giPtr = placedItem.GetGIEntry();
    if (!giPtr) {
        return;
    }

    if (!IS_RANDO || !CVarGetInteger(CVAR_ENHANCEMENT("ChestSizeAndTextureMatchContents"), 0) ||
        identity->randomizerCheck == RC_UNKNOWN_CHECK) {
        return;
    }

    int requiresStoneAgony = CVarGetInteger(CVAR_ENHANCEMENT("ChestSizeDependsStoneOfAgony"), 0);
    int isNotCMC = (requiresStoneAgony && !CHECK_QUEST_ITEM(QUEST_STONE_OF_AGONY));

    GetItemCategory getItemCategory = Randomizer_AdjustItemCategory(*giPtr);
    if (!isNotCMC) {
        Color_RGBA8 primColor = Randomizer_GetParticleCMCColor(getItemCategory, COLOR_PRIMARY);
        Color_RGBA8 secColor = Randomizer_GetParticleCMCColor(getItemCategory, COLOR_SECONDARY);
        Color_RGBA8 envColor = Randomizer_GetParticleCMCColor(getItemCategory, COLOR_FLARE);
        SoilSkulltula_RandomizerDraw(beanActor, &primColor, &secColor, &envColor);
    }
}

void RegisterSoilSkulls() {
    bool shouldRegister = IS_RANDO && RAND_GET_OPTION(RSK_SHUFFLE_TOKENS);

    COND_ID_HOOK(OnActorInit, ACTOR_OBJ_BEAN, shouldRegister, SoilSkulltula_CacheIdentity);
    COND_ID_HOOK(OnActorUpdate, ACTOR_OBJ_BEAN, shouldRegister, SoilSkulltula_RandomizerDrawSetup);
}

static RegisterShipInitFunc registerSoilSkulls(RegisterSoilSkulls, { "IS_RANDO" });