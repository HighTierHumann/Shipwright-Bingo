#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/ShipInit.hpp"
#include "soh/cvar_prefixes.h"

extern "C" {
#include "z64.h"
#include "functions.h"
#include "macros.h"
#include "variables.h"
extern PlayState* gPlayState;
}

#define CVAR_HEAVY_BOOT_PHYSICS_DEFAULT 0
#define CVAR_HEAVY_BOOT_PHYSICS_NAME CVAR_ENHANCEMENT("SprainedLegMode")
#define CVAR_HEAVY_BOOT_PHYSICS_VALUE CVarGetInteger(CVAR_HEAVY_BOOT_PHYSICS_NAME, CVAR_HEAVY_BOOT_PHYSICS_DEFAULT)

void OnPlayerUpdateHeavyBootPhysics() {
    bool gravityModifierActive = CVAR_HEAVY_BOOT_PHYSICS_VALUE != CVAR_HEAVY_BOOT_PHYSICS_DEFAULT;

    Player* player = GET_PLAYER(gPlayState);

    if (player != nullptr && player->currentBoots != PLAYER_BOOTS_IRON && (player->actor.bgCheckFlags & BGCHECKFLAG_GROUND_LEAVE) && 
        player->linearVelocity >= 3.0f) {

        R_RUN_SPEED_LIMIT = 600;
        player->linearVelocity = 6.0f;
        player->actor.velocity.y = 7.5f;
    }

    if (player != nullptr && player->currentBoots != PLAYER_BOOTS_IRON && player->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
        R_RUN_SPEED_LIMIT = 300;
        //REG(27) = 500; // Turn Speed  (also somehow alters siderolls/downrolls, so do not change)
        REG(38) = 0; // Animation Speed
        REG(48) = 100; // Something related to linearVelocity
    }
}

void RegisterHeavyBootPhysics() {
    COND_HOOK(OnPlayerUpdate, CVAR_HEAVY_BOOT_PHYSICS_VALUE != CVAR_HEAVY_BOOT_PHYSICS_DEFAULT, OnPlayerUpdateHeavyBootPhysics);
}

static RegisterShipInitFunc initFunc(RegisterHeavyBootPhysics, { CVAR_HEAVY_BOOT_PHYSICS_NAME });
