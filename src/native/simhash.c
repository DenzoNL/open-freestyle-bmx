/*
 * Dev aid for the visual-only check of the widescreen seams (docs/WIDESCREEN.md W10.5): BMX_SIM_HASH=<file>
 * writes one line per controller poll (one per game tick, menus included) with an FNV-1a 64 hash of the
 * simulation state:
 *
 *   <tick> <hash>
 *
 * The hash covers the golden-trace recorder's state fields (src/hook/trace.c: the random seed, both pads, the
 * selected camera's eye and look-at, every rider's state, surface, state levels, flags, timer, position,
 * velocity, forward, up, rotation and spin), plus each rider's run score (combo.score) and the run timer
 * gGameTimer. (A hash of the whole rider array was tried and dropped: it differs between two runs of the same
 * build from the first tick, so the struct holds process-specific bytes.)
 *
 * Two runs of the same BMX_KEYS script with the option off and on must give the same file. Native exe only;
 * it uses the trace hook point in contrlerPoll (compat/trace.h), which the hook DLL's recorder also uses.
 */
#include <stdio.h>
#include <stdlib.h>

#include "bmx/com/action/camera.h"
#include "bmx/com/action/contrler.h"
#include "bmx/com/action/game.h"
#include "bmx/com/action/physics.h"
#include "compat/trace.h"
#include "native/native.h"
#include "zge/zmath.h"

static FILE *gOut;
static uint32_t gTick;
static uint64_t gHash;

static void mix(const void *data, size_t size)
{
    const uint8_t *p = data;

    for (size_t i = 0; i < size; i++) {
        gHash = (gHash ^ p[i]) * 0x100000001b3ull;
    }
}

static void mix32(uint32_t v)
{
    mix(&v, sizeof v);
}

uint64_t bmxSimStateHash(void)
{
    const CameraState *const cam = &gCameras[gCameraPlayer < 4 ? gCameraPlayer : 0];

    gHash = 0xcbf29ce484222325ull;
    mix32(gZmathTables.randomSeed);
    for (int i = 0; i < 2; i++) {
        const ContrlerPad *const pad = &gContrlerPads[i];
        const ContrlerDevice *const dev = pad->device;

        mix32(pad->flags);
        mix32(dev != NULL ? dev->status : 0);
        mix32(dev != NULL ? dev->held : 0);
        for (int k = 0; k < 4; k++) {
            mix32(dev != NULL ? (uint32_t)dev->axis[k] : 0);
        }
    }
    mix32(gCameraPlayer);
    mix(&cam->eye, sizeof cam->eye);
    mix(&cam->lookAt, sizeof cam->lookAt);
    for (int i = 0; i < 4; i++) {
        const PhysRider *const r = &gPhysRiders[i];

        mix32(r->state);
        mix32(r->surface);
        mix(r->stateLevel, sizeof r->stateLevel);
        mix32(r->flags);
        mix32((uint32_t)r->timer);
        mix(&r->pos, sizeof r->pos);
        mix(&r->vel, sizeof r->vel);
        mix(&r->fwd, sizeof r->fwd);
        mix(&r->up, sizeof r->up);
        mix(&r->rot, sizeof r->rot);
        mix32((uint32_t)r->spin);
        mix32(r->combo.score);
    }
    mix32(gGameTimer);
    return gHash;
}

static void simPoll(void)
{
    fprintf(gOut, "%u %016llx\n", (unsigned)gTick++, (unsigned long long)bmxSimStateHash());
    fflush(gOut); /* BMX_QUIT_FRAMES ends the process with _Exit */
}

void bmxSimHashInstall(void)
{
    const char *const path = getenv("BMX_SIM_HASH");

    if (path == NULL || *path == 0) {
        return;
    }
    gOut = fopen(path, "w");
    if (gOut == NULL) {
        bmxLog("simhash: cannot write %s\n", path);
        return;
    }
    gBmxTracePoll = simPoll;
    bmxLog("simhash: per-tick simulation hash into %s\n", path);
}
