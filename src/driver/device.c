/*
 * Device side of the SDL3 driver: the system function behind RwDevice (0x4ee010), the core extension table
 * (0x69bee0) and the jump table that plugs it all in. The request codes, return values and the state the
 * engine reads back follow the original D3D7 driver (work/bundles-rw); DirectDraw/Direct3D internals do not.
 */
#include <stdlib.h>
#include <string.h>

#include "driver/driver.h"
#include "driver/wide.h"
#include "native/native.h"
#include "platform/standins.h"
#include "rw/rwd3d.h"
#include "rw/rwim.h"
#include "rw/rwplugins.h"

DrvState drvState = {.currentMode = -1};

/* The original's device table in .data: its function pointers stay the original addresses. */
#define rwOrigDevice ORIG_VAR(0x00566fe4, RwDevice, rwOrigDevice)

/* Offsets of the driver's raster and camera plugin data. */
static int32_t drvRasterOffset = -1;
int32_t drvCameraOffset = -1;

enum {
    kDrvPluginId = 0x40c,  /* the driver's raster and camera plugins */
    kDrvCoreExtId = 0x40e, /* the core extension's engine plugin */
};

/* The only sub system. Names ending in "S/W" are hidden by the device dialog; the original's were
 * "<driver> H/W" or "<driver> (<description>) H/W". */
static const char kSubSystemName[] = "SDL3 GPU H/W";

/* The resolutions the device dialog lists (zrwDialogInit); offered whether or not the display has them,
 * since the driver renders at the mode's size and scales to the screen (D4). */
static const int32_t kClassicSizes[][2] = {{640, 480}, {800, 600}, {1024, 768}, {1280, 1024}, {1600, 1200}};

DrvRasterExt *drvRasterExt(const RwRaster *raster)
{
    return (DrvRasterExt *)((uint8_t *)raster + drvRasterOffset);
}

void drvTrace(const char *name, uintptr_t a, uintptr_t b, uintptr_t c)
{
    static int enabled = -1;

    if (enabled < 0) {
        const char *env = getenv("BMX_DRIVER_TRACE");

        enabled = env != NULL && *env != '0';
    }
    if (enabled) {
        bmxLog("driver: %s(%p, %p, %p)\n", name, (void *)a, (void *)b, (void *)c);
    }
}

void drvSetError(int32_t errorCode)
{
    RwError error;

    error.pluginID = 1;
    error.errorCode = errorCode;
    RwErrorSet(&error);
}

/* ---- video modes ------------------------------------------------------------------------------------- */

static void addMode(int32_t width, int32_t height, int32_t depth, uint32_t flags)
{
    DrvVideoMode *mode;

    if (drvState.numModes >= (int32_t)(sizeof drvState.modes / sizeof drvState.modes[0])) {
        return;
    }
    mode = &drvState.modes[drvState.numModes++];
    mode->width = width;
    mode->height = height;
    mode->depth = depth;
    mode->flags = flags;
}

static int sizeCmp(const void *a, const void *b)
{
    const int32_t *x = a, *y = b;

    if (x[0] != y[0]) {
        return x[0] < y[0] ? -1 : 1;
    }
    return x[1] < y[1] ? -1 : x[1] > y[1];
}

/*
 * The original's PrepareGame (0x4ec100): the windowed mode (desktop size and depth, flags 0) first, then the
 * full screen modes. Ours: the display's full screen sizes plus the dialog's five, each at 16 and 32 bits.
 * The current mode becomes 0.
 */
static void buildModeList(void)
{
    int32_t sizes[96][2];
    int count;
    DrvVideoMode desktop;
    const int haveDesktop = drvGpuDesktopMode(&desktop);

    drvState.numModes = 0;
    drvState.currentMode = -1;
    if (haveDesktop) {
        addMode(desktop.width, desktop.height, desktop.depth, 0);
    }
    count = drvGpuFullscreenSizes(sizes, (int)(sizeof sizes / sizeof sizes[0]) - 6);
    /* The desktop size is always a full screen size: Android lists no full screen modes at all (SDL3), so
     * `width = height = 0` (the desktop size) found no mode there and fell back to 640x480. */
    if (haveDesktop) {
        sizes[count][0] = desktop.width;
        sizes[count][1] = desktop.height;
        count++;
    }
    for (size_t i = 0; i < sizeof kClassicSizes / sizeof kClassicSizes[0]; i++) {
        sizes[count][0] = kClassicSizes[i][0];
        sizes[count][1] = kClassicSizes[i][1];
        count++;
    }
    qsort(sizes, (size_t)count, sizeof sizes[0], sizeCmp);
    for (int i = 0; i < count; i++) {
        if (i > 0 && sizes[i][0] == sizes[i - 1][0] && sizes[i][1] == sizes[i - 1][1]) {
            continue;
        }
        addMode(sizes[i][0], sizes[i][1], 16, kDrvModeExclusive);
        addMode(sizes[i][0], sizes[i][1], 32, kDrvModeExclusive);
    }
    drvState.currentMode = 0;
}

/*
 * The device dialog's replacement (docs/PLATFORM.md P16, slice S10): zrwSelectDevice (0x489d70) calls
 * DialogBoxParamA(resource 0x68) when the command line has no `-r<w>.<h>.<depth>`. Instead of the dialog,
 * this stand-in sets the mode the dialog would have returned and answers IDOK:
 * - `[video] fullscreen = 1` (default): the full screen mode `[video] width` x `height` x `depth` (default
 *   640 x 480 x 16, the dialog's own preselection, i.e. what pressing Enter in the vanilla dialog gives);
 * - `[video] fullscreen = 0`: the windowed mode (index 0; main.c made the window width x height, and the game
 *   renders at that size since it never enters full screen);
 * - a size the list does not have: the dialog's preselection (the last 640 x 480 x 16 mode), logged.
 * The dialog procedure (zrwDialogProc 0x48a330) is never run; its USER32 slots are dead.
 */
#define gZglVideoMode ORIG_VAR(0x00672758, uint32_t, gZglVideoMode)

intptr_t BMX_WINAPI bmxStandDialogBoxParamA(void *instance, const char *name, void *parent, void *proc,
                                            intptr_t param)
{
    const int fullscreen = bmxConfigInt("video.fullscreen", 1) != 0;
    int width, height;
    const int32_t depth = bmxConfigInt("video.depth", 16);
    int32_t pick = -1, preselect = -1;

    (void)instance;
    (void)name;
    (void)parent;
    (void)proc;
    (void)param;
    bmxVideoSize(&width, &height, 640, 480); /* 0: the desktop size (docs/WIDESCREEN.md W1) */
    for (int32_t i = 0; i < drvState.numModes; i++) {
        const DrvVideoMode *const m = &drvState.modes[i];

        if (m->width == 640 && m->height == 480 && m->depth == 16) {
            preselect = i; /* zrwDialogInit: the last 640x480x16, whatever its flags */
        }
        if (fullscreen && (m->flags & kDrvModeExclusive) != 0 && m->width == width && m->height == height &&
            m->depth == depth) {
            pick = i;
        }
    }
    if (!fullscreen && drvState.numModes > 0 && (drvState.modes[0].flags & kDrvModeExclusive) == 0) {
        pick = 0;
    }
    if (pick < 0) {
        bmxLog("driver: no %s mode %dx%dx%d, using the dialog's preselection\n",
               fullscreen ? "full screen" : "windowed", (int)width, (int)height, (int)depth);
        pick = preselect >= 0 ? preselect : drvState.currentMode;
    }
    gZglVideoMode = (uint32_t)pick;
    bmxLog("driver: no device dialog (P16): mode %d from the config's [video] or the default\n", (int)pick);
    return 1;
}

/* 0x4dd270: RwRasterRegisterPlugin (in the driver's part of the exe, but plain RW). */
int32_t drvRasterRegisterPlugin(int32_t size, int32_t pluginID, RwPluginObjectConstructor constructCB,
                                RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB)
{
    return _rwPluginRegistryAddPlugin(&rwRasterRegistry, size, pluginID, constructCB, destructCB, copyCB);
}

/* ---- system requests ---------------------------------------------------------------------------------- */

/*
 * rwDEVICESYSTEMREGISTER (0x4ecbf0): copy the device table into the engine and register the raster and camera
 * plugins (0x20 bytes each, like the original, so later plugin offsets and object sizes stay the same).
 */
static int systemRegister(RwDevice *device, const RwMemoryFunctions *memory)
{
    *device = rwOrigDevice;
    rwD3DEngineDevice = device;
    rwD3DMemory = memory;
    drvRasterOffset = drvRasterRegisterPlugin(sizeof(DrvRasterExt), kDrvPluginId, NULL, NULL, NULL);
    drvCameraOffset = RwCameraRegisterPlugin(kDrvCameraExtBytes, kDrvPluginId, RW_CTOR(0x004d4ea0),
                                             RW_DTOR(0x004d4f80), NULL);
    rwD3DRasterExtOffset = drvRasterOffset;
    rwD3DCameraExtOffset = drvCameraOffset;
    return 1;
}

/*
 * The device caps the ported render states check (rwD3DCaps): what SDL3 GPU does for this driver. Dither;
 * every z compare, source and destination blend and alpha compare; flat and Gouraud colour and alpha; vertex
 * fog; perspective correct textures; nearest and linear filtering (no mip filters, like the texture formats);
 * wrap, mirror and clamp addressing (no border colour in SDL3 GPU).
 */
static uint32_t drvCaps[0x130 / 4];

static void buildCaps(void)
{
    memset(drvCaps, 0, sizeof drvCaps);
    drvCaps[rwD3DCAPSRASTER / 4] = 0x1;
    drvCaps[rwD3DCAPSZCMP / 4] = 0xff;
    drvCaps[rwD3DCAPSSRCBLEND / 4] = 0x1fff;
    drvCaps[rwD3DCAPSDESTBLEND / 4] = 0x1fff;
    drvCaps[rwD3DCAPSALPHACMP / 4] = 0xff;
    drvCaps[rwD3DCAPSSHADE / 4] = 0x2 | 0x8 | 0x1000 | 0x4000 | 0x40000 | 0x80000;
    drvCaps[rwD3DCAPSTEXTURE / 4] = 0x1;
    drvCaps[rwD3DCAPSFILTER / 4] = 0x1 | 0x2;
    drvCaps[rwD3DCAPSADDRESS / 4] = 0x1 | 0x2 | 0x4;
    rwD3DCaps = (const uint8_t *)drvCaps;
}

/*
 * 0x4d7ed0: the device's initial render states and their cache (rwD3DState), by the caps: perspective, z test
 * and write with less-equal, Gouraud, alpha test greater (or not equal) against 0 when the device can, source
 * alpha / inverse source alpha blending (off), stage 0 modulating texture and diffuse (selecting diffuse
 * while there is no texture), the best non-mip filter, no fog, no culling.
 */
static void initRenderStates(void)
{
    RwD3DStateCache *s = &rwD3DState;
    const uint32_t filter = rwD3DCap(rwD3DCAPSFILTER);
    uint32_t func;

    s->perspective = (rwD3DCap(rwD3DCAPSTEXTURE) & 1) != 0;
    rwD3DDevSetRenderState(4, (uint32_t)s->perspective);
    rwD3DDevSetRenderState(0x80, 0);
    s->zWrite = 1;
    s->zTest = 1;
    rwD3DDevSetRenderState(7, 1);
    rwD3DDevSetRenderState(0xe, 1);
    rwD3DDevSetRenderState(0x17, 4);
    s->shadeMode = 2;
    rwD3DDevSetRenderState(9, 2);
    if ((rwD3DCap(rwD3DCAPSALPHACMP) & 0x30) == 0) {
        rwD3DAlphaTestCaps = 0;
        func = 8;
    } else {
        rwD3DAlphaTestCaps = 1;
        func = (rwD3DCap(rwD3DCAPSALPHACMP) & 0x10) == 0 ? 6 : 5;
    }
    rwD3DDevSetRenderState(0x18, 0);
    rwD3DDevSetRenderState(0xf, 0);
    rwD3DDevSetRenderState(0x19, func);
    s->srcBlend = 5;
    s->destBlend = 6;
    rwD3DDevSetRenderState(0x13, 5);
    rwD3DDevSetRenderState(0x14, 6);
    s->alphaBlend = 0;
    rwD3DDevSetRenderState(0x1b, 0);
    s->colorKey = 0;
    s->vertexAlpha = 0;
    rwD3DDevSetRenderState(0x29, 0);
    s->address = 1;
    rwD3DDevSetTextureStageState(0, 0xc, 1);
    rwD3DDevSetTextureStageState(0, 2, 2);
    rwD3DDevSetTextureStageState(0, 3, 0);
    rwD3DDevSetTextureStageState(0, 5, 2);
    rwD3DDevSetTextureStageState(0, 6, 0);
    s->alphaOp = 3;
    rwD3DDevSetTextureStageState(0, 4, 3);
    s->colorOp = 3;
    rwD3DDevSetTextureStageState(0, 1, 3);
    s->textureAlpha = 0;
    s->textureRaster = NULL;
    rwD3DDevSetTexture(0, NULL);
    /* The best filter: linear / point, with mip linear / point / none (the caps' bits 0x20 down to 2). */
    if ((filter & 0x20) != 0) {
        s->mipFilter = 3, s->minFilter = s->magFilter = 2;
    } else if ((filter & 0x10) != 0) {
        s->mipFilter = 3, s->minFilter = s->magFilter = 1;
    } else if ((filter & 0x8) != 0) {
        s->mipFilter = 2, s->minFilter = s->magFilter = 2;
    } else if ((filter & 0x4) != 0) {
        s->mipFilter = 2, s->minFilter = s->magFilter = 1;
    } else {
        s->mipFilter = 1, s->minFilter = s->magFilter = (filter & 0x2) != 0 ? 2 : 1;
    }
    rwD3DDevSetTextureStageState(0, 0x10, (uint32_t)s->magFilter);
    rwD3DDevSetTextureStageState(0, 0x11, (uint32_t)s->minFilter);
    rwD3DDevSetTextureStageState(0, 0x12, (uint32_t)s->mipFilter);
    rwD3DDevSetRenderState(0x1a, (rwD3DCap(rwD3DCAPSRASTER) & 1) != 0);
    s->fogColor = 0;
    s->fogEnable = 0;
    rwD3DDevSetRenderState(0x1c, 0);
    rwD3DDevSetRenderState(0x22, 0);
    rwD3DDevSetRenderState(0x23, 0);
    rwD3DDevSetRenderState(0x27, 0);
    rwD3DDevSetRenderState(0x16, 1);
    rwD3DDevSetRenderState(8, 3);
    rwD3DDevSetRenderState(0x1d, 0);
    rwD3DDevSetRenderState(2, 0);
    s->alphaSort = 0;
}

/* rwDEVICESYSTEMOPEN (0x4ecce0): the window handle, the sub systems and the mode list. */
static int systemOpen(void *const *openParams)
{
    drvState.window = *openParams;
    if (!drvGpuInitVideo()) {
        return 0;
    }
    buildModeList();
    return 1;
}

/* rwDEVICESYSTEMCLOSE (0x4ecd60). */
static void systemClose(void)
{
    drvState.numModes = 0;
    drvState.currentMode = -1;
    drvGpuQuitVideo();
}

/* rwDEVICESYSTEMSTART (0x4eceb0): the GPU device on the game's window and the display formats. */
static int systemStart(void)
{
    const DrvVideoMode *mode;

    if (drvState.currentMode < 0 || !drvGpuStart(drvState.window)) {
        drvSetError(_rwerror(kDrvErrDevice, "Failed to create the SDL3 GPU device"));
        return 0;
    }
    mode = &drvState.modes[drvState.currentMode];
    bmxLog("driver: mode %d of %d: %dx%dx%d%s\n", (int)drvState.currentMode, (int)drvState.numModes,
           (int)mode->width, (int)mode->height, (int)mode->depth,
           (mode->flags & kDrvModeExclusive) != 0 ? " full screen" : " windowed");
    drvState.cameraDepth = mode->depth == 16 ? 16 : 32;
    drvState.cameraFormat = mode->depth == 16 ? kDrvFormat565 : kDrvFormat888;
    drvState.zDepth = 16;
    drvState.zFormat = kDrvFormat16;
    buildCaps();
    if (!rwD3DIm2DOpen()) {
        drvGpuStop();
        return 0;
    }
    drvWideInstall2D(); /* [video] widescreen: the 2D funnel (docs/WIDESCREEN.md W8-A2) */
    initRenderStates();
    rwD3DContext = NULL;
    drvState.started = 1;
    return 1;
}

/* rwDEVICESYSTEMSTOP (0x4ee170). */
static void systemStop(void)
{
    drvGpuStop();
    drvState.started = 0;
}

/* rwDEVICESYSTEMUSEMODE (0x4eca10): only before the device is started. */
static int systemUseMode(int32_t index)
{
    if (drvState.started || index < 0 || index >= drvState.numModes) {
        return 0;
    }
    drvState.currentMode = index;
    return 1;
}

static int systemGetModeInfo(DrvVideoMode *out, int32_t index)
{
    if (index < 0 || index >= drvState.numModes) {
        return 0;
    }
    *out = drvState.modes[index];
    return 1;
}

static int systemGetSubSystemInfo(char *out, int32_t index)
{
    if (index != 0) {
        drvSetError(_rwerror(kDrvErrDevice, "Invalid subsystem number"));
        return 0;
    }
    RwEngineInstance->stringFuncs.rwstrcpy(out, kSubSystemName);
    return 1;
}

static int systemSetSubSystem(int32_t index)
{
    if (index != 0) {
        drvSetError(_rwerror(kDrvErrDevice, "Invalid Subsystem number"));
        return 0;
    }
    buildModeList();
    return 1;
}

int drvNullStandard(void *out, void *inOut, int32_t in);

/* One of the original's standards (src/data, 0x56701c): the slot and the function. */
typedef struct DrvStandard {
    int32_t id;
    void *fn;
} DrvStandard;

BMX_LAYOUT32_ASSERT(sizeof(DrvStandard) == 8, "standard pair");

/* rwDEVICESYSTEMSTANDARDS (0x4ede90): the null standard everywhere, then the original's (id, fn) pairs. */
static void systemStandards(void **out, int32_t count)
{
    const DrvStandard *table = (const DrvStandard *)BMX_ORIG(0x0056701c);

    for (int32_t i = 0; i < count; i++) {
        out[i] = BMX_FNPTR(void *, 0x004ebfc0, drvNullStandard); /* D3D3NullStandard */
    }
    for (int i = 0; i < 0x1b; i++) {
        const int32_t id = table[i].id;

        if (id < count && id >= 0) {
            out[id] = table[i].fn;
        }
    }
}

/* 0x4ebfc0: the standard of every slot the driver does not fill. */
int drvNullStandard(void *out, void *inOut, int32_t in)
{
    return 0;
}

/*
 * 0x4eb2c0, 0x4eb2d0, 0x4eb2e0, 0x4eb2f0: the device table's Im2D line, triangle, primitive and indexed
 * primitive. In the original D3D7 driver all four are `mov eax, 1; ret` and nothing replaces them, so Im2D
 * draws nothing in the original game either (zrwprimDrawPolys calls the primitive one).
 */
int drvIm2DNothing(void)
{
    return 1;
}

/* 0x4ee010: RwDevice.fpSystem. Requests the original does not handle return 0. */
int drvSystem(int32_t request, void *out, void *inOut, int32_t in)
{
    DRV_TRACE("drvSystem", (uintptr_t)request, (uintptr_t)out, (uintptr_t)in);
    switch (request) {
    case rwDEVICESYSTEMOPEN:
        return systemOpen((void *const *)inOut);
    case rwDEVICESYSTEMCLOSE:
        systemClose();
        return 1;
    case rwDEVICESYSTEMSTART:
        return systemStart();
    case rwDEVICESYSTEMSTOP:
        systemStop();
        return 1;
    case rwDEVICESYSTEMREGISTER:
        return systemRegister(out, inOut);
    case rwDEVICESYSTEMGETNUMMODES:
        *(int32_t *)out = drvState.numModes;
        return 1;
    case rwDEVICESYSTEMGETMODEINFO:
        return systemGetModeInfo(out, in);
    case rwDEVICESYSTEMUSEMODE:
        return systemUseMode(in);
    case 8: /* switch between exclusive and windowed (0x4ecab0): needs a started device */
        return drvState.started;
    case rwDEVICESYSTEMPIPELINE: /* 0x4ee150: the pipeline's render stage */
        ((RwIm3DPipeline *)inOut)->stage1 =
            BMX_FNPTR(void(BMX_CDECL *)(void *), 0x004f97c0, rwD3DIm3DTransform);
        return 1;
    case rwDEVICESYSTEMGETMODE:
        *(int32_t *)out = drvState.currentMode;
        return 1;
    case rwDEVICESYSTEMSTANDARDS:
        systemStandards((void **)out, in);
        return 1;
    case 12: /* free texture memory (0x4edf60) */
        *(uint32_t *)out = 64u << 20;
        return 1;
    case rwDEVICESYSTEMGETNUMSUBSYSTEMS:
        *(int32_t *)out = 1;
        return 1;
    case rwDEVICESYSTEMGETSUBSYSTEMINFO:
        return systemGetSubSystemInfo(out, in);
    case rwDEVICESYSTEMGETCURRENTSUBSYSTEM:
        *(int32_t *)out = 0;
        return 1;
    case rwDEVICESYSTEMSETSUBSYSTEM:
        return systemSetSubSystem(in);
    case 0x13: /* max texture size (0x4edf30): min(max width, max height) of the device */
        *(uint32_t *)out = kDrvMaxTextureSize;
        return 1;
    default:
        return 0;
    }
}

/* ---- core extension (0x69bee0) ------------------------------------------------------------------------ */

/* Entry 0 (0x4eac90): attach. */
int drvCoreExtAttach(void)
{
    return RwEngineRegisterPlugin(0, kDrvCoreExtId, RW_CTOR(0x004eaaa0), RW_DTOR(0x004eabd0)) >= 0;
}

/* ---- the jump table ------------------------------------------------------------------------------------ */

const BmxHook bmxDriverJumps[] = {
    {0x004ee010, (void *)drvSystem},
    {0x004ebfc0, (void *)drvNullStandard},
    {0x004eb2c0, (void *)drvIm2DNothing},
    {0x004eb2d0, (void *)drvIm2DNothing},
    {0x004eb2e0, (void *)drvIm2DNothing},
    {0x004eb2f0, (void *)drvIm2DNothing},
    {0x004dd270, (void *)drvRasterRegisterPlugin},
    {0x004eac90, (void *)drvCoreExtAttach},
    {0x004d4ea0, (void *)drvCameraCtor},
    {0x004d4f80, (void *)drvCameraDtor},
    {0x004d45a0, (void *)drvCameraBeginUpdate},
    {0x004d4a80, (void *)drvCameraEndUpdate},
    {0x004d4c10, (void *)drvCameraClear},
    {0x004d4ae0, (void *)drvRasterShowRaster},
    {0x004d5b80, (void *)drvRasterCreate},
    {0x004d5660, (void *)drvRasterDestroy},
    {0x004d4570, (void *)drvHintRenderFront2Back},
    {0x004d5150, (void *)drvRasterLock},
    {0x004d5300, (void *)drvRasterUnlock},
    {0x004f83a0, (void *)drvImageFindRasterFormat},
    {0x004f79a0, (void *)drvRasterSetFromImage},
    {0x004d6490, (void *)drvTextureSetRaster},
    {0x004d64b0, (void *)drvRasterSubRaster},
    {0x004f94b0, (void *)drvSetRasterContext},
};

const uint32_t bmxDriverJumpCount = sizeof bmxDriverJumps / sizeof bmxDriverJumps[0];
