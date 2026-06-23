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

extern "C" {
#include "src/overlays/actors/ovl_Obj_Bean/z_obj_bean.h"
}

// Cache the bean patch's CheckIdentity at actor init time using its params & 0x3F switch flag.
// The skulltula actor (ACTOR_EN_SW) does not exist yet at this point; the identity comes
// entirely from the static (sceneNum, switchFlag) -> RandomizerCheck table.
static void SoilSkulltula_CacheIdentity(void* refActor) {
    ObjBean* beanActor = static_cast<ObjBean*>(refActor);
    auto identity =
        OTRGlobals::Instance->gRandomizer->IdentifySoilSkulltula(gPlayState->sceneNum, beanActor->dyna.actor.params);
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

    if (!RAND_GET_OPTION(RSK_SKULLTULA_PARTICLES)) {
        return;
    } else if (beanActor->dyna.actor.xzDistToPlayer > 1000.0f) {
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
    bool shouldRegister = IS_RANDO && RAND_GET_OPTION(RSK_SKULLTULA_PARTICLES);

    COND_ID_HOOK(OnActorInit, ACTOR_OBJ_BEAN, shouldRegister, SoilSkulltula_CacheIdentity);
    COND_ID_HOOK(OnActorUpdate, ACTOR_OBJ_BEAN, shouldRegister, SoilSkulltula_RandomizerDrawSetup);
}

static RegisterShipInitFunc registerSoilSkulls(RegisterSoilSkulls, { "IS_RANDO" });