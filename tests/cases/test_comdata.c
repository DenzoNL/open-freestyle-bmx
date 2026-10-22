/*
 * Differential tests of comdata.c: the camera view code (actors, horizon and the unported view setter are
 * stubbed and logged), the level record tables, the goal info lookup and the reset of the profile data.
 */
#include "bmx/com/action/action.h"
#include "bmx/com/action/camera.h"
#include "bmx/com/action/comdata.h"
#include "bmx/com/action/game.h"
#include "bmx/com/action/overlay.h"
#include "bmx/com/action/physics.h"
#include "bmx/com/menu/frontend.h"
#include "dt.h"
#include "dt_gameplay.h"
#include "zge/zmath.h"

static void randomCamera(CameraState *c)
{
    static const uint8_t types[] = {0, 0, 0, 1, 3, 4, 0xff, 2};
    static const uint8_t modes[] = {0, 1, 2, 3, 3, 0xff, 5};

    dt_fill(c, sizeof *c);
    c->type = (dt_u32() & 7) == 0 ? (uint8_t)dt_u32() : types[dt_u32() % sizeof types];
    c->mode = (dt_u32() & 7) == 0 ? (uint8_t)dt_u32() : modes[dt_u32() % sizeof modes];
    c->flags = (uint8_t)dt_u32();
}

static void randomState(void)
{
    randomCamera(&gCameras[0]);
    randomCamera(&gCameras[1]);
    gCameraPlayer = (uint8_t)dt_u32();
    gCameraReset = dt_u32();
    for (int p = 0; p < 2; p++) {
        gPhysRiders[p].flags = dt_u32();
        gPhysRiders[p].actor = (void *)(uintptr_t)(0x00300000u + dt_range(0, 0xff) * 0x10u);
        gPhysRiders[p].bikeObj = (void *)(uintptr_t)(0x00310000u + dt_range(0, 0xff) * 0x10u);
    }
}

static void setupViewMode(DtCtx *c)
{
    dtgpBegin(c, DTGP_ACTOR);
    randomState();
    const int player = dt_range(0, 1);
    /* Mostly the interesting modes; sometimes the mode the camera already has, sometimes a wide value. */
    uint32_t mode = (uint32_t)dt_range(0, 5);
    if ((dt_u32() & 7) == 0) {
        mode = gCameras[player].mode;
    } else if ((dt_u32() & 15) == 0) {
        mode = dt_u32();
    }
    DT_ARG_INT(c, 0, player);
    DT_ARG_INT(c, 1, mode);
}

static void setupPlayerOnly(DtCtx *c)
{
    dtgpBegin(c, 0);
    randomState();
    DT_ARG_INT(c, 0, dt_range(0, 1));
}

static void setupResetView(DtCtx *c)
{
    dtgpBegin(c, DTGP_ACTOR);
    randomState();
    DT_ARG_INT(c, 0, dt_range(0, 1));
    DT_ARG_INT(c, 1, (dt_u32() & 15) == 0 ? (int32_t)dt_u32() : dt_range(0, 6));
}

static void setupNone(DtCtx *c)
{
    dtgpBegin(c, 0);
    randomState();
}

static void setupInit(DtCtx *c)
{
    dtgpBegin(c, 0);
    dt_fill(&gGameRiders, sizeof gGameRiders);
    dt_fill(&gProfiles, sizeof gProfiles);
    dt_fill(&gPlayers, sizeof gPlayers);
    dt_fill(&gSoundVolumes, sizeof gSoundVolumes);
    gGameUnlockBits = (uint16_t)dt_u32();
    dt_fill(&gActionOptions, 2);
    gCurrentPlayer = (uint8_t)dt_u32();
    gPlayerCount = (uint8_t)dt_u32();
    gActionLanguage = (uint8_t)dt_u32();
}

static void setupLevelRecord(DtCtx *c)
{
    dtgpBegin(c, 0);
    gCurrentLevel = (uint8_t)((dt_u32() & 7) == 0 ? dt_u32() & 0x1f : (uint32_t)dt_range(0, 11));
    const uint32_t type = (dt_u32() & 7) == 0 ? dt_u32() & 0xff : (uint32_t)dt_range(0, 12);
    const uint32_t set = (uint32_t)dt_range(0, 3);
    const uint32_t sub = (dt_u32() & 3) == 0 ? (uint32_t)dt_range(0, 63) : (uint32_t)dt_range(0, 1);
    const uint32_t low = (dt_u32() & 3) == 0 ? dt_u32() & 0xffff : (uint32_t)dt_range(0, 40);
    DT_ARG_INT(c, 0, type << 24 | set << 22 | sub << 16 | low);
}

static void setupGoalInfo(DtCtx *c)
{
    dtgpBegin(c, DTGP_DEBUG);
    for (int p = 0; p < 2; p++) {
        gProfiles[p].character = (dt_u32() & 3) == 0 ? (uint8_t)dt_u32() : (uint8_t)dt_range(0, 11);
    }
    DT_ARG_INT(c, 0, dt_range(0, 1));
    DT_ARG_INT(c, 1, (dt_u32() & 7) == 0 ? (int32_t)dt_u32() : dt_range(0, 5));
}

/* comdataSetView: the respawn face lookup and the face centre are stubbed and log what they get. */
static void *BMX_CDECL stubRespawnFace(const ZVec3 *pos, uint32_t flags)
{
    dtgpLog(0x4e590000u);
    dtgpLogN(pos, 3);
    dtgpLog(flags);
    return (void *)(uintptr_t)(0x00680000u + (uint32_t)dt_range(0, 255) * 0x6c);
}

static void BMX_CDECL stubFaceCentroid(const void *face, ZVec3 *out)
{
    dtgpLog(0xce470000u);
    dtgpLogP(face);
    out->x = dt_float_edgy(-1000.0f, 1000.0f);
    out->y = dt_float_edgy(-1000.0f, 1000.0f);
    out->z = dt_float_edgy(-1000.0f, 1000.0f);
}

static void setupSetView(DtCtx *c)
{
    const int player = dt_range(0, 1);
    PhysRider *r = &gPhysRiders[player];
    const float range = (dt_u32() & 7) == 0 ? 1.0e6f : 3000.0f;

    dtgpBegin(c, DTGP_DEBUG);
    randomState();
    dt_fill(r, sizeof *r);
    r->pos.x = dt_float_edgy(-range, range);
    r->pos.y = dt_float_edgy(-range, range);
    r->pos.z = dt_float_edgy(-range, range);
    r->vel.x = dt_float_edgy(-60.0f, 60.0f);
    r->vel.z = dt_float_edgy(-60.0f, 60.0f);
    r->fwd.x = dt_float_edgy(-1.0f, 1.0f);
    r->fwd.z = dt_float_edgy(-1.0f, 1.0f);
    r->speed = (dt_u32() & 3) == 0 ? 0.0f : dt_float_edgy(-30.0f, 30.0f);
    r->state = (uint8_t)dt_range(0, 7);
    gActionMode = (uint8_t)dt_range(0, 2);
    gZmathTables.randomSeed = dt_u32();
    dt_fill(&gCameras[player], sizeof gCameras[player]);
    gCameras[player].timer = dt_u32();
    dt_stub(0x004200b0, NULL, (void *)stubRespawnFace);
    dt_stub(0x00466c50, NULL, (void *)stubFaceCentroid);
    DT_ARG_INT(c, 0, player);
    DT_ARG_INT(c, 1, (dt_u32() & 15) == 0 ? dt_u32() : (uint32_t)dt_range(0, 11));
}

DT_CASES(comdata_cases) = {
    {"comdataSetView", 0x004078c0, comdataSetView, 2, DT_RET_VOID, setupSetView, 2000, 0},
    {"comdataSetViewMode", 0x00407620, comdataSetViewMode, 2, DT_RET_VOID, setupViewMode, 600, 0},
    {"comdataGetViewMode", 0x00407730, comdataGetViewMode, 1, DT_RET_INT, setupPlayerOnly, 30, 0},
    {"comdataResetView", 0x00407760, comdataResetView, 2, DT_RET_VOID, setupResetView, 600, 0},
    {"comdataGetViewType", 0x00407890, comdataGetViewType, 1, DT_RET_INT, setupPlayerOnly, 30, 0},
    {"comdataGetViewPlayer", 0x00407600, comdataGetViewPlayer, 0, DT_RET_INT, setupNone, 30, 0},
    {"comdataGetView", 0x00407b70, comdataGetView, 1, DT_RET_INT, setupPlayerOnly, 30, 0},
    {"comdataInit", 0x00407ba0, comdataInit, 0, DT_RET_VOID, setupInit, 100, 0},
    {"comdataGetLevelRecord", 0x00407d20, comdataGetLevelRecord, 1, DT_RET_INT, setupLevelRecord, 1000, 0},
    {"comdataGetGoalInfo", 0x00407fc0, comdataGetGoalInfo, 2, DT_RET_INT, setupGoalInfo, 300, 0},
};
