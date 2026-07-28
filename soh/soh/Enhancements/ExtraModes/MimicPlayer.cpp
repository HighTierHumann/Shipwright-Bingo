#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/ShipInit.hpp"
#include "soh/ObjectExtension/ObjectExtension.h"
#include "soh/Notification/Notification.h"
#include <vector>
#include <spdlog/spdlog.h>
#include "include/functions.h"

extern "C" {
#include "include/macros.h"
#include "include/variables.h"
extern PlayState* gPlayState;

void Player_UseItem(PlayState* play, Player* player, s32 item);
void Player_Draw(Actor* actor, PlayState* play);
}

static constexpr int32_t CVAR_MIMIC_PLAYER_DEFAULT = 0;
#define CVAR_MIMIC_PLAYER_NAME CVAR_ENHANCEMENT("MimicMode")
#define CVAR_MIMIC_PLAYER_VALUE CVarGetInteger(CVAR_MIMIC_PLAYER_NAME, CVAR_MIMIC_PLAYER_DEFAULT)

#define CVAR_MIMIC_DELAY_NAME CVAR_ENHANCEMENT("MimicDelay")
#define CVAR_MIMIC_DELAY_VALUE CVarGetInteger(CVAR_MIMIC_DELAY_NAME, 5)

#define CVAR_MIMIC_SPAWN_INTERVAL_NAME CVAR_ENHANCEMENT("MimicSpawnInterval")
#define CVAR_MIMIC_SPAWN_INTERVAL_VALUE CVarGetInteger(CVAR_MIMIC_SPAWN_INTERVAL_NAME, 10)

#define CVAR_MIMIC_COUNT_NAME CVAR_ENHANCEMENT("MimicCount")
#define CVAR_MIMIC_COUNT_VALUE CVarGetInteger(CVAR_MIMIC_COUNT_NAME, 1)

#define CVAR_MIMIC_DAMAGE_NAME CVAR_ENHANCEMENT("MimicDamage")
#define CVAR_MIMIC_DAMAGE_VALUE CVarGetInteger(CVAR_MIMIC_DAMAGE_NAME, 40)

// Guards against the ShouldActorInit hook hijacking the real player's own ACTOR_PLAYER spawn.
// Only set to true for the duration of our own deliberate Actor_Spawn call below.
static bool sSpawningMimicPlayer = false;
// If true, the initial mimic spawn was deferred until we have enough recorded history
// (see OnSceneSpawnActors). When set, the OnPlayerUpdate spawn hook will perform
// the actual spawn once `sMimicRecording.size()` >= delayTicks.
static bool sPendingSpawnFirstMimic = false;

// The base game's actor/collision logic always runs at a fixed 20 ticks per second,
// independent of the render frame rate, so this converts delays to tick counts.
static constexpr int32_t MIMIC_TICKS_PER_SECOND = 20;

// Number of mimics already spawned this scene, and ticks remaining until the next one spawns.
// Both reset whenever a new scene is entered.
static int32_t sMimicsSpawnedThisScene = 0;
static int32_t sMimicSpawnCooldownTicks = 0;

static constexpr f32 MIMIC_COLLIDER_RADIUS = 13.0f;
static constexpr f32 MIMIC_COLLIDER_HEIGHT = 40.0f;

// A single recorded frame of the player's state, deep-copied so it can be replayed later.
struct MimicSnapshot {
    Vec3f pos;
    Vec3s rot;
    Vec3s upperLimbRot;
    Vec3s jointTable[PLAYER_LIMB_BUF_COUNT];
    u8 movementFlags;
    Vec3s prevTransl;
    s8 currentBoots;
    s8 currentShield;
    u8 heldItemId;
    s8 currentTunic;
    u32 stateFlags1;
    u32 stateFlags2;
    s8 itemAction;
    s8 heldItemAction;
    s8 invincibilityTimer;
    s16 unk_862;
    f32 unk_85C;
    s8 actionVar1;
};

// A single shared, ever-growing timeline of the player's recorded state since the scene began.
// All mimics replay from this SAME timeline (each tracking its own read position via
// MimicPlaybackData below), rather than each mimic recording its own live feed. This is what lets
// a newly-spawned mimic replay the player's history from the very beginning instead of
// immediately overlapping whatever an already-spawned mimic is doing.
static std::vector<MimicSnapshot> sMimicRecording;

// Tracks how far into sMimicRecording a single mimic has replayed so far. Attached per-mimic via
// ObjectExtension (keyed by the mimic's own Actor pointer) so each mimic can progress
// independently through the shared recording.
struct MimicPlaybackData {
    size_t readIndex = 0;
};
static ObjectExtension::Register<MimicPlaybackData> MimicPlaybackDataRegister;

void Math_Vec3s_Copy_Mimic(Vec3s* dest, Vec3s* src) {
    dest->x = src->x;
    dest->y = src->y;
    dest->z = src->z;
}

static void Mimic_CaptureSnapshot(Player* actualPlayer, MimicSnapshot& snapshot) {
    snapshot.pos = actualPlayer->actor.world.pos;
    snapshot.rot = actualPlayer->actor.shape.rot;
    snapshot.upperLimbRot = actualPlayer->upperLimbRot;

    for (size_t i = 0; i < PLAYER_LIMB_BUF_COUNT; i++) {
        snapshot.jointTable[i] = actualPlayer->skelAnime.jointTable[i];
    }

    snapshot.movementFlags = actualPlayer->skelAnime.movementFlags;
    snapshot.prevTransl = actualPlayer->skelAnime.prevTransl;
    snapshot.currentBoots = actualPlayer->currentBoots;
    snapshot.currentShield = actualPlayer->currentShield;
    snapshot.heldItemId = actualPlayer->heldItemId;
    snapshot.currentTunic = actualPlayer->currentTunic;
    snapshot.stateFlags1 = actualPlayer->stateFlags1;
    snapshot.stateFlags2 = actualPlayer->stateFlags2;
    snapshot.itemAction = actualPlayer->itemAction;
    snapshot.heldItemAction = actualPlayer->heldItemAction;
    snapshot.invincibilityTimer = actualPlayer->invincibilityTimer;
    snapshot.unk_862 =
        (actualPlayer->unk_862 > (s16)GID_MAXIMUM) ? (s16)GID_STONE_OF_AGONY : actualPlayer->unk_862; // prevent OOB
    snapshot.unk_85C = actualPlayer->unk_85C;
    snapshot.actionVar1 = actualPlayer->av1.actionVar1;
}

static void Mimic_ApplySnapshot(Player* player, Actor* actor, MimicSnapshot& snapshot) {
    Math_Vec3s_Copy_Mimic(&player->upperLimbRot, &snapshot.upperLimbRot);
    Math_Vec3s_Copy_Mimic(&actor->shape.rot, &snapshot.rot);
    Math_Vec3f_Copy(&actor->world.pos, &snapshot.pos);

    for (size_t i = 0; i < PLAYER_LIMB_BUF_COUNT; i++) {
        player->jointTable[i] = snapshot.jointTable[i];
    }

    player->skelAnime.movementFlags = snapshot.movementFlags;
    Math_Vec3s_Copy_Mimic(&player->skelAnime.prevTransl, &snapshot.prevTransl);
    player->currentBoots = snapshot.currentBoots;
    player->currentShield = snapshot.currentShield;
    player->heldItemId = snapshot.heldItemId;
    player->currentTunic = snapshot.currentTunic;
    player->stateFlags1 = snapshot.stateFlags1;
    player->stateFlags2 = snapshot.stateFlags2;
    player->itemAction = snapshot.itemAction;
    player->heldItemAction = snapshot.heldItemAction;
    player->invincibilityTimer = snapshot.invincibilityTimer;
    player->unk_862 = snapshot.unk_862;
    player->unk_85C = snapshot.unk_85C;
    player->av1.actionVar1 = snapshot.actionVar1;
}

void MimicPlayer_Init(Actor* actor, PlayState* play) {
    Player* player = (Player*)actor;

    // Start replaying the shared recording from the very beginning.
    ObjectExtension::GetInstance().Set<MimicPlaybackData>(actor, MimicPlaybackData{});

    // #region modeled after EnTorch2_Init and Player_Init
    actor->room = -1;
    player->itemAction = player->heldItemAction = -1;
    player->heldItemId = ITEM_NONE;
    Player_UseItem(play, player, ITEM_NONE);
    Player_SetModelGroup(player, Player_ActionToModelGroup(player, player->heldItemAction));
    play->playerInit(player, play, gPlayerSkelHeaders[gSaveContext.linkAge]);

    // Prevent the mimic from holding a weapon trail effect slot, as it doesn't use it anyway
    Effect_Delete(play, player->meleeWeaponEffectIndex);
    player->meleeWeaponEffectIndex = TOTAL_EFFECT_COUNT;

    play->func_11D54(player, play);
    // #endregion

    // Make sure the mimic replays animation into its own joint buffer, not the real player's
    player->skelAnime.jointTable = player->jointTable;

    player->cylinder.base.acFlags = AC_ON | AC_TYPE_PLAYER;
    player->cylinder.base.ocFlags2 = OC2_TYPE_1;
    player->cylinder.info.bumperFlags = BUMP_ON | BUMP_HOOKABLE | BUMP_NO_HITMARK;
    player->actor.flags |= ACTOR_FLAG_HOOKSHOT_PULLS_PLAYER;
    player->cylinder.dim.radius = static_cast<int16_t>(MIMIC_COLLIDER_RADIUS);
    player->cylinder.dim.height = static_cast<int16_t>(MIMIC_COLLIDER_HEIGHT);
}

// Records the real player's current state, then replays whatever state was recorded
// `sMimicDelaySeconds` seconds ago onto the mimic actor. Kills the real player on contact.
void MimicPlayer_Update(Actor* actor, PlayState* play) {
    Player* actualPlayer = GET_PLAYER(play);
    Player* player = (Player*)actor;

    MimicPlaybackData* playbackData = ObjectExtension::GetInstance().Get<MimicPlaybackData>(actor);
    if (playbackData == nullptr) {
        return;
    }

    // Do not update while the player is in a cutscene
    if (Play_InCsMode(play)) {
        return;
    }

    if (gPlayState->sceneNum == SCENE_SHOOTING_GALLERY && gPlayState->shootingGalleryStatus != 0) {
        return;
    }

    actor->shape.shadowAlpha = 255;

    // Damage player if they touch mimic
    if (player->cylinder.base.ocFlags2 & OC2_HIT_PLAYER && actualPlayer->invincibilityTimer <= 0) {
        player->cylinder.base.ocFlags2 &= ~OC2_HIT_PLAYER;
        GameInteractor::RawAction::HealOrDamagePlayer(CVAR_MIMIC_DAMAGE_VALUE * -1);
        if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("MimicLose"), 0) && gSaveContext.health <= 0) {
            Play_PerformSave(play);
            Notification::Emit({
                .message = "Game autosaved",
            });
            gSaveContext.ship.quest.data.randomizer.loseCon = true;
        }
    }

    const size_t delayTicks = static_cast<size_t>(CVAR_MIMIC_DELAY_VALUE * MIMIC_TICKS_PER_SECOND);
    const size_t targetReadIndex = (sMimicRecording.size() > delayTicks) ? (sMimicRecording.size() - delayTicks) : 0;

    if (playbackData->readIndex < targetReadIndex) {
        Mimic_ApplySnapshot(player, actor, sMimicRecording[playbackData->readIndex]);
        playbackData->readIndex++;

        // Apply animation movement (Copied from Player_ApplyAnimMovementScaledByAge)
        Vec3f diff;
        SkelAnime_UpdateTranslation(&player->skelAnime, &diff, player->actor.shape.rot.y);

        if (player->skelAnime.movementFlags & 1) {
            if (!LINK_IS_ADULT) {
                diff.x *= 0.64f;
                diff.z *= 0.64f;
            }

            player->actor.world.pos.x += diff.x * player->actor.scale.x;
            player->actor.world.pos.z += diff.z * player->actor.scale.z;
        }

        if (player->skelAnime.movementFlags & 2) {
            if (!(player->skelAnime.movementFlags & 4)) {
                diff.y *= player->ageProperties->unk_08;
            }

            player->actor.world.pos.y += diff.y * player->actor.scale.y;
        }
    }
    // Otherwise there isn't enough recorded history yet, so the mimic simply waits in place
    // until the initial delay has elapsed.

    actor->flags |= ACTOR_FLAG_LOCK_ON_DISABLED;

    Collider_UpdateCylinder(&player->actor, &player->cylinder);

    if (!(player->stateFlags2 & PLAYER_STATE2_FROZEN)) {
        if (!(player->stateFlags1 & (PLAYER_STATE1_DEAD | PLAYER_STATE1_HANGING_OFF_LEDGE |
                                     PLAYER_STATE1_CLIMBING_LEDGE | PLAYER_STATE1_ON_HORSE))) {
            CollisionCheck_SetOC(play, &play->colChkCtx, &player->cylinder.base);
        }

        if (!(player->stateFlags1 & (PLAYER_STATE1_DEAD | PLAYER_STATE1_DAMAGED)) &&
            (player->invincibilityTimer <= 0)) {
            CollisionCheck_SetAC(play, &play->colChkCtx, &player->cylinder.base);

            if (player->invincibilityTimer < 0) {
                CollisionCheck_SetAT(play, &play->colChkCtx, &player->cylinder.base);
            }
        }
    }

    if (player->stateFlags1 & (PLAYER_STATE1_DEAD | PLAYER_STATE1_IN_ITEM_CS | PLAYER_STATE1_IN_CUTSCENE)) {
        player->actor.colChkInfo.mass = MASS_IMMOVABLE;
    } else {
        player->actor.colChkInfo.mass = 50;
    }

    Collider_ResetCylinderAC(play, &player->cylinder.base);
}

void MimicPlayer_Draw(Actor* actor, PlayState* play) {
    Player* actualPlayer = GET_PLAYER(play);
    Player* player = (Player*)actor;

    u8 originalButtonItem0 = gSaveContext.equips.buttonItems[0];
    gSaveContext.equips.buttonItems[0] = player->heldItemId;

    Player_Draw((Actor*)player, play);
    gSaveContext.equips.buttonItems[0] = originalButtonItem0;
}

void MimicPlayer_Destroy(Actor* actor, PlayState* play) {
    // Mimic actors are initially spawned as ACTOR_PLAYER, but change their
    // ID shortly afterwards to ACTOR_EN_OE2. This would cause ACTOR_PLAYER's
    // ActorDB Entry's `numLoaded` to leak, which is mostly harmless but hits debug
    // asserts. Set the id back to ACTOR_PLAYER so that `numLoaded` will be decremented
    // correctly.
    actor->id = ACTOR_PLAYER;
    // Note: this mimic's MimicHistoryData is automatically freed by ObjectExtension_Free
    // (called from Actor_Delete), so no manual cleanup is needed here.
}

// Spawns a brand new ACTOR_PLAYER instance behind the real player, which the ShouldActorInit hook
// below (guarded by sSpawningMimicPlayer) converts into the mimic. This is necessary because
// ACTOR_PLAYER is also the id of the real player's own actor - without the guard flag, the hook
// would hijack the real player's spawn on every scene load instead. spawnIndex spaces additional
// mimics further back so they don't all spawn stacked on top of each other.
static void MimicPlayer_Spawn(PlayState* play, int32_t spawnIndex) {
    Player* realPlayer = GET_PLAYER(play);

    if (realPlayer == NULL) {
        return;
    }

    static constexpr f32 MIMIC_SPAWN_DISTANCE = -9999;

    sSpawningMimicPlayer = true;
    Actor_Spawn(&play->actorCtx, play, ACTOR_PLAYER, MIMIC_SPAWN_DISTANCE, MIMIC_SPAWN_DISTANCE, MIMIC_SPAWN_DISTANCE,
                realPlayer->actor.shape.rot.x, realPlayer->actor.shape.rot.y, realPlayer->actor.shape.rot.z, 0);
    sSpawningMimicPlayer = false;
    sMimicsSpawnedThisScene++;
    sMimicSpawnCooldownTicks = (CVAR_MIMIC_SPAWN_INTERVAL_VALUE * MIMIC_TICKS_PER_SECOND);
}

void RegisterMimicPlayerHooks() {
    COND_ID_HOOK(ShouldActorInit, ACTOR_PLAYER, CVAR_MIMIC_PLAYER_VALUE, [&](void* actorRef, bool* should) {
        if (!sSpawningMimicPlayer) {
            // This is the real player's own spawn (or some unrelated ACTOR_PLAYER spawn) - leave it alone.
            return;
        }

        Actor* actor = (Actor*)actorRef;

        if (gPlayState != NULL) {
            // By the time we get here, the actor was already added to the ACTORCAT_PLAYER list, so we need to move it
            Actor_ChangeCategory(gPlayState, &gPlayState->actorCtx, actor, ACTORCAT_NPC);
            actor->id = ACTOR_EN_OE2;
            actor->category = ACTORCAT_NPC;
            actor->init = MimicPlayer_Init;
            actor->update = MimicPlayer_Update;
            actor->draw = MimicPlayer_Draw;
            actor->destroy = MimicPlayer_Destroy;
        }
    });

    COND_HOOK(OnSceneInit, CVAR_MIMIC_PLAYER_VALUE, [](int16_t sceneNum) {
        sMimicsSpawnedThisScene = 0;
        sMimicSpawnCooldownTicks = 0;
        sMimicRecording.clear();
        sPendingSpawnFirstMimic = false;
    });

    // Records the real player's current state every tick into the single shared timeline that all
    // mimics replay from. Runs regardless of whether any mimics currently exist, so there's already
    // history available for a mimic to replay from the beginning as soon as it spawns.
    COND_HOOK(OnPlayerUpdate, CVAR_MIMIC_PLAYER_VALUE, [&]() {
        if (gPlayState == NULL) {
            return;
        }

        Player* actualPlayer = GET_PLAYER(gPlayState);
        if (actualPlayer == NULL) {
            return;
        }

        // Don't record cutscenes
        if (Play_InCsMode(gPlayState)) {
            return;
        }

        MimicSnapshot snapshot;
        Mimic_CaptureSnapshot(actualPlayer, snapshot);
        sMimicRecording.push_back(snapshot);
    });

    // Spawns the first mimic as soon as the scene's actors are set up. If we don't yet
    // have `CVAR_MIMIC_DELAY_VALUE` seconds worth of recorded history, defer the initial
    // spawn until the history buffer is long enough so the mimic can immediately
    // replay from the configured delayed offset.
    COND_HOOK(OnSceneSpawnActors, CVAR_MIMIC_PLAYER_VALUE, [&]() {
        if (gPlayState == NULL || CVAR_MIMIC_COUNT_VALUE <= 0) {
            return;
        }

        const size_t delayTicks = static_cast<size_t>(CVAR_MIMIC_DELAY_VALUE * MIMIC_TICKS_PER_SECOND);

        if (sMimicRecording.size() < delayTicks) {
            // Not enough history yet; mark that we should spawn the first mimic
            // once the recording buffer reaches the required length.
            sPendingSpawnFirstMimic = true;
            return;
        }

        MimicPlayer_Spawn(gPlayState, sMimicsSpawnedThisScene);
    });

    // Spawns each additional mimic (beyond the first) MIMIC_SPAWN_INTERVAL_SECONDS apart, until
    // CVAR_MIMIC_COUNT_VALUE have been spawned for this scene. Also services a deferred
    // initial spawn if OnSceneSpawnActors deferred it due to insufficient recording history.
    COND_HOOK(OnPlayerUpdate, CVAR_MIMIC_PLAYER_VALUE, [&]() {
        if (gPlayState == NULL || sMimicsSpawnedThisScene >= CVAR_MIMIC_COUNT_VALUE) {
            return;
        }

        const size_t delayTicks = static_cast<size_t>(CVAR_MIMIC_DELAY_VALUE * MIMIC_TICKS_PER_SECOND);

        // If the initial spawn was deferred, spawn it as soon as we have enough history.
        if (sPendingSpawnFirstMimic) {
            if (sMimicRecording.size() >= delayTicks) {
                MimicPlayer_Spawn(gPlayState, sMimicsSpawnedThisScene);
                sPendingSpawnFirstMimic = false;
            }
            return;
        }

        // If there are no mimics yet and we aren't pending the first, do nothing.
        if (sMimicsSpawnedThisScene <= 0) {
            return;
        }

        if (--sMimicSpawnCooldownTicks > 0) {
            return;
        }

        // Ensure there's enough recorded history for the configured delay before spawning.
        if (sMimicRecording.size() < delayTicks) {
            return;
        }

        MimicPlayer_Spawn(gPlayState, sMimicsSpawnedThisScene);
    });
}

// Register hooks to run at startup
static RegisterShipInitFunc initFunc(RegisterMimicPlayerHooks, { CVAR_MIMIC_PLAYER_NAME });
