/*
 * Differential tests of camera.c. The engine calls (camera object, sound, letterbox fill, actors) are
 * stubbed and logged; comdata.c runs natively on both sides.
 */
#include "bmx/com/action/action.h"
#include "bmx/com/action/camera.h"
#include "bmx/com/action/overlay.h"
#include "bmx/com/action/physics.h"
#include "bmx/com/zge/ngif/ngifzroller.h"
#include "dt.h"
#include "dt_gameplay.h"

/* A camera in a state the view code branches on. */
static void randomCamera(CameraState *c)
{
    static const uint8_t types[] = {0, 0, 0, 1, 3, 4, 0xff, 2};
    static const uint8_t modes[] = {0, 1, 2, 3, 3, 0xff, 5};

    dt_fill(c, sizeof *c);
    c->type = (dt_u32() & 7) == 0 ? (uint8_t)dt_u32() : types[dt_u32() % sizeof types];
    c->mode = (dt_u32() & 7) == 0 ? (uint8_t)dt_u32() : modes[dt_u32() % sizeof modes];
    c->flags = (uint8_t)dt_u32();
}

static void randomRiders(void)
{
    for (int p = 0; p < 2; p++) {
        gPhysRiders[p].flags = dt_u32();
        gPhysRiders[p].actor = (void *)(uintptr_t)(0x00300000u + dt_range(0, 0xff) * 0x10u);
        gPhysRiders[p].bikeObj = (void *)(uintptr_t)(0x00310000u + dt_range(0, 0xff) * 0x10u);
    }
}

static void setupPlayback(DtCtx *c)
{
    dtgpBegin(c, 0);
    gActionMode = (uint8_t)dt_u32();
}

static void setupNone(DtCtx *c)
{
    dtgpBegin(c, 0);
}

static void setupInit(DtCtx *c)
{
    dtgpBegin(c, DTGP_DEBUG | DTGP_ACTOR);
    gPlayerCount = (uint8_t)dt_range(0, 2);
    randomCamera(&gCameras[0]);
    randomCamera(&gCameras[1]);
    gCameraPlayer = (uint8_t)dt_u32();
    randomRiders();
    for (int i = 0; i < 2; i++) {
        gProfiles[i].option = (dt_u32() & 3) == 0 ? (uint8_t)dt_u32() : (uint8_t)dt_range(0, 4);
    }
    for (int i = 0; i < 8; i++) {
        gZrollerDrawCallbacks[i] =
            (dt_u32() & 1) ? NULL : (ZrollerDrawFn)(uintptr_t)(0x00401000u + (uint32_t)i);
    }
    if ((dt_u32() & 7) == 0) {
        for (int i = 0; i < 8; i++) {
            gZrollerDrawCallbacks[i] = (ZrollerDrawFn)(uintptr_t)(0x00401000u + (uint32_t)i);
        }
    }
}

static void setupApply(DtCtx *c)
{
    dtgpBegin(c, DTGP_CAMERA | DTGP_SOUND);
    CameraView *v = dt_alloc(sizeof *v);
    dt_fill(v, sizeof *v);
    v->camera = (void *)(uintptr_t)(0x00320000u + dt_range(0, 0xff) * 0x10u);
    v->eye.x = dt_float_edgy(-1000.0f, 1000.0f);
    v->eye.y = dt_float_edgy(-1000.0f, 1000.0f);
    v->eye.z = dt_float_edgy(-1000.0f, 1000.0f);
    v->target.x = dt_float_edgy(-1000.0f, 1000.0f);
    v->target.y = dt_float_edgy(-1000.0f, 1000.0f);
    v->target.z = dt_float_edgy(-1000.0f, 1000.0f);
    DT_ARG_PTR(c, 0, v);
}

static void setupLetterbox(DtCtx *c)
{
    dtgpBegin(c, DTGP_CAMERA);
    gActionOptions = (uint8_t)((dt_u32() & 3) != 0 ? dt_u32() | 2 : dt_u32());
    gPlayerCount = (uint8_t)((dt_u32() & 3) != 0 ? dt_range(0, 2) : (int32_t)dt_u32());
    gCameraRect[0] = dt_u32();
    gCameraRect[1] = dt_u32();
}

static void setupSelect(DtCtx *c)
{
    dtgpBegin(c, DTGP_DEBUG);
    randomCamera(&gCameras[0]);
    randomCamera(&gCameras[1]);
    gCameraPlayer = (uint8_t)dt_range(0, 1);
    if ((dt_u32() & 1) != 0) {
        gCameras[gCameraPlayer].flags = (uint8_t)(gCameras[gCameraPlayer].flags | 0x10);
    }
    DT_ARG_INT(c, 0, (dt_u32() & 7) == 0 ? (uint32_t)dt_range(2, 3) : (uint32_t)dt_range(0, 1));
}

DT_CASES(camera_cases) = {
    {"cameraPlaybackActive", 0x004056a0, cameraPlaybackActive, 0, DT_RET_INT, setupPlayback, 30, 0},
    {"cameraInit", 0x004056c0, cameraInit, 0, DT_RET_VOID, setupInit, 300, 0},
    {"cameraNop", 0x00405760, cameraNop, 0, DT_RET_VOID, setupNone, 1, 0},
    {"cameraApply", 0x00407470, cameraApply, 1, DT_RET_VOID, setupApply, 100, 0},
    {"cameraDrawLetterbox", 0x004074c0, cameraDrawLetterbox, 0, DT_RET_INT, setupLetterbox, 100, 0},
    {"cameraSelectPlayer", 0x00407530, cameraSelectPlayer, 1, DT_RET_VOID, setupSelect, 300, 0},
};
