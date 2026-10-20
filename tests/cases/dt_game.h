/*
 * Shared fixtures for the game and rider tests (test_game.c, test_rider.c). Every callee outside the
 * function under test (controller, overlay, physics, effects, props, replay, level data, vector maths) is
 * replaced per iteration with dt_stub() by a stub that logs its arguments by value into DLL data (compared
 * like any other memory) and returns scripted values.
 */
#ifndef DT_GAME_H
#define DT_GAME_H

#include <stdint.h>

#include "bmx/com/action/game.h"
#include "bmx/com/action/physics.h"
#include "bmx/com/action/rider.h"
#include "dt.h"

/* Groups of stubs a case installs (what the function under test calls). */
enum {
    DTG_DEBUG = 1u << 0,    /* zdebugFail, gZdebugPrint */
    DTG_SOUND = 1u << 1,    /* bmxSoundPlayAt / PlayEvent */
    DTG_OVL = 1u << 2,      /* ovlSetElementA, ovlSetElementB, ovlSetScreen */
    DTG_GAMEFN = 1u << 3,   /* the unported game.c functions */
    DTG_COMDATA = 1u << 4,  /* comdata and camera */
    DTG_PROP = 1u << 5,     /* props */
    DTG_REPLAY = 1u << 6,   /* replay recorders */
    DTG_ROLLER = 1u << 7,   /* level start, bounds, records, level info */
    DTG_MATH = 1u << 8,     /* vector and matrix helpers, asin/acos, quaternion */
    DTG_DRAW = 1u << 9,     /* 3D debug primitives, collide / bsp object records */
    DTG_PHYS = 1u << 10,    /* physGetState, physSetState, physAddVelocity, physRespawn, physUpdateRiders */
    DTG_PAD = 1u << 11,     /* controller */
    DTG_ZOVER = 1u << 12,   /* motion objects and actors */
    DTG_FX = 1u << 13,      /* effects, fx nodes, name table handlers */
    DTG_RIDERFN = 1u << 14, /* riderReset (ported), riderUpdateInput */
    DTG_ALL = 0x7fffu,
};

/* Scripted answers of the stubs, drawn afresh each iteration by dtGameBegin. */
typedef struct DtGameScript {
    uint8_t pressed[0x15][3]; /* contrlerAction(action, status) */
    float axes[2][2];         /* contrlerAxes(state, stick) */
    uint32_t playback;
    uint32_t viewPlayer;
    uint32_t physState;     /* physGetState */
    uint32_t physSetResult; /* physSetState: 0 = the requested state */
    uint32_t replayFailAt;  /* 1-based replayCreate call that fails, 0: none */
    uint32_t replayCalls;
    uint8_t propMissing;   /* propFind returns NULL for ids whose low bit is set */
    uint8_t recordMissing; /* comdataGetLevelRecord returns NULL */
    uint8_t pad[2];
    float asinScale;
    float acosScale;
    uint32_t asinResult; /* 0: computed from the argument, else forced */
    uint32_t acosResult;
    uint32_t actorNull;          /* ngifzCreateActor / zrollerCreateBike: 1 and 2 fail, 3 both */
    void *node;                  /* ngiffxGetNode result (or NULL) */
    void *object;                /* ngifzCollideGetObject result */
    void *objectInfo;            /* ngifzbspGetObjectInfo result */
    float *bounds;               /* zrollerGetWorldBounds */
    uint8_t *rollerRecord;       /* zrollerGetRecord */
    void *goalInfo[4][12];       /* comdataGetGoalInfo(player, level) */
    uint32_t *levelRecord;       /* comdataGetLevelRecord */
    uint32_t statusMsg[2];       /* what ovlSetElementB(0xc) returns */
    uint8_t padRecords[2][0x40]; /* contrlerGet */
    uint32_t
        stepScript; /* three bits per call of the step handler stub: result, change mode, change sub state */
    uint32_t stepCalls;
    ZVec3 startPos;
    ZAngle3 startRot;
} DtGameScript;

extern DtGameScript dtGame;

/* Seeds from the case seed, clears the log, randomises the game state and installs the stub groups. */
void dtGameBegin(const DtCtx *ctx, uint32_t groups);

/* Random rider record: filled with random bytes, then the fields the code under test branches on. */
void dtGameRandomRider(PhysRider *r);

/* Points the mode and sub state step tables at one logging stub (so no original state code runs). */
void dtGameInstallStepTables(void);

/* Setup shorthand for the common case. */
#define DTGAME_SETUP(name, groups)                                                                           \
    static void name(DtCtx *c)                                                                               \
    {                                                                                                        \
        dtGameBegin(c, groups);                                                                              \
    }

#endif
