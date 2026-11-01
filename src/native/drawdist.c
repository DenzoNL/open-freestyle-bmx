/*
 * [video] full_draw_distance = 1 (opt-in, PLAN.md Decisions): the far clip and fog at their maximum all the
 * time.
 *
 * The original adapts them to the frame rate it measures over 100 ms windows (ngiffxAdaptQuality 0x45c710,
 * docs/HIGHFPS.md "Wall-clock render state"): the level load reads as ~2.5 fps, the far clip drops to 60 and
 * climbs back to ~190 over ~20 seconds, so a level starts with its distant geometry missing. Any machine that
 * runs the native exe holds the 29.4 Hz tick, so the adapter only ever reacts to the load. With the option
 * its entry jumps to fullDrawDistance, which sets the quality the adapter would end at with nothing to adapt
 * to (200, its maximum) every tick. The far clip is the camera's (zrwSetClipPlane); the simulation does not
 * read it.
 */
#include "bmx/com/zge/ngif/ngiffx.h"
#include "compat/orig.h"
#include "hook/hook.h"
#include "native/native.h"
#include "platform/platform.h"

#define gFxQuality ORIG_VAR(0x0055bd08, float, gFxQuality)

static void fullDrawDistance(void)
{
    gFxQuality = 200.0f;
    ngiffxSetQuality(200.0f);
}

static BmxHook gJumps[1];

const BmxHook *bmxDrawDistJumps(uint32_t *count)
{
    const int on = bmxConfigInt("video.full_draw_distance", 0) == 1;

    gJumps[0].orig = (uint32_t)(uintptr_t)ngiffxAdaptQuality;
    gJumps[0].impl = (void *)fullDrawDistance;
    *count = on ? 1u : 0u;
    if (on) {
        bmxLog("video: full_draw_distance on (far clip 200)\n");
    }
    return gJumps;
}
