/*
 * Render states of the SDL3 driver (RwDevice.fpRenderStateSet 0x4d7320 / fpRenderStateGet 0x4d7c10).
 *
 * Step 1 keeps the values only. Like the original, a set outside a camera update fails (0); inside one every
 * known state is accepted. The original also refuses some values by the D3D device's caps (filter, blend,
 * fog modes); that and the GPU pipeline state they select come with Im2D (docs/SDL3-DRIVER.md, step 2).
 */
#include "driver/driver.h"

enum { kDrvNumStates = 0x20 };

static int32_t drvStates[kDrvNumStates];

int drvRenderStateSet(int32_t state, int32_t value)
{
    DRV_TRACE("drvRenderStateSet", (uint32_t)state, (uint32_t)value, 0);
    if (!drvCameraUpdating() || state <= 0 || state >= kDrvNumStates) {
        return 0;
    }
    drvStates[state] = value;
    return 1;
}

int drvRenderStateGet(int32_t state, void *value)
{
    DRV_TRACE("drvRenderStateGet", (uint32_t)state, (uint32_t)(uintptr_t)value, 0);
    if (state <= 0 || state >= kDrvNumStates) {
        return 0;
    }
    *(int32_t *)value = drvStates[state];
    return 1;
}

/* A destroyed raster stops being the texture raster state (state 1), as in 0x4d5660. */
void drvRenderStateForgetRaster(const void *raster)
{
    if (drvStates[1] == (int32_t)(uintptr_t)raster) {
        drvStates[1] = 0;
    }
}
