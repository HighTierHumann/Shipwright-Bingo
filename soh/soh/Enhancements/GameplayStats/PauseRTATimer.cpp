#include <libultraship/libultraship.h>
#include "soh/ShipInit.hpp"
#include "soh/cvar_prefixes.h"
#include "soh/Enhancements/gameplaystats.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"

extern "C" {
#include "variables.h"
#include "macros.h"
#include "functions.h"

extern PlayState* gPlayState;
}

void RtaSetPaused(int paused) {
    // Only update if rtaTiming is enabled
    if (!gSaveContext.ship.stats.rtaTiming) {
        return;
    }

    if (paused) {
        if (!gSaveContext.ship.stats.rtaPaused) {
            gSaveContext.ship.stats.rtaPaused = true;
            gSaveContext.ship.stats.rtaPauseStart = GetUnixTimestamp();

            Sfx_PlaySfxCentered(NA_SE_SY_LOCK_ON_HUMAN);
        }
    }
    else {
        if (gSaveContext.ship.stats.rtaPaused) {
            uint64_t now = GetUnixTimestamp();
            if (now > gSaveContext.ship.stats.rtaPauseStart) {
                gSaveContext.ship.stats.rtaPausedTime += (now - gSaveContext.ship.stats.rtaPauseStart);
            }
            gSaveContext.ship.stats.rtaPauseStart = 0;
            gSaveContext.ship.stats.rtaPaused = false;

            Sfx_PlaySfxCentered(NA_SE_SY_DECIDE);
        }
    }
}

void RegisterRtaPauseToggle() {
    COND_HOOK(OnGameStateMainStart, 1, []() {
        if (gPlayState == nullptr) {
            return;
        }

        int comboMask = CVarGetInteger(CVAR_GAMEPLAY_STATS("RTAPauseBtn"), (BTN_R | BTN_DDOWN));
        if (comboMask == 0) {
            return;
        }

        Input* input = &gPlayState->state.input[0];

        if (CHECK_BTN_ALL(input->cur.button, comboMask) && (input->press.button & comboMask)) {
            RtaSetPaused(!gSaveContext.ship.stats.rtaPaused);
        }
        });
}

// Register to run on startup (and allow re-init if the CVAR changes)
static RegisterShipInitFunc s_registerRtaPause(RegisterRtaPauseToggle, { CVAR_GAMEPLAY_STATS("RTAPauseBtn") });