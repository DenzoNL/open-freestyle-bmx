/*
 * The SDL3 GPU driver for RenderWare 3.1: it replaces the original's Direct3D 7 driver (lib/d3d7driver in
 * data/functions-rw.csv) behind RW's device interface. See docs/SDL3-DRIVER.md.
 *
 * Native exe only (never the hook DLL). The original's device table (0x566fe4), standards table (0x56701c)
 * and core extension table (0x69bee0) stay as they are in the copied data; the loader writes a jump from
 * each original driver entry point we implement to our function (bmxDriverJumps). Every pointer the driver
 * hands to RenderWare is therefore the original's address, and an entry point we do not implement yet stays
 * an int3 that the trap handler reports by name.
 */
#ifndef BMX_DRIVER_DRIVER_H
#define BMX_DRIVER_DRIVER_H

#include <stdint.h>

#include "hook/hook.h"
#include "rw/rwcamera.h"
#include "rw/rwcore.h"
#include "rw/rwtexture.h"

/* (original address, our function) pairs; written as jumps by the native loader after the hook table. */
extern const BmxHook bmxDriverJumps[];
extern const uint32_t bmxDriverJumpCount;

/* RwVideoMode as RW and the device dialog read it. */
typedef struct DrvVideoMode {
    int32_t width;
    int32_t height;
    int32_t depth;
    uint32_t flags; /* bit 0: exclusive (full screen) */
} DrvVideoMode;

enum {
    kDrvModeExclusive = 1,
    kDrvMaxTextureSize = 2048, /* the texture size limit the driver reports (request 0x13) */
};

/* Raster formats (RwRaster.format << 8) the driver deals in. */
enum {
    kDrvFormat1555 = 0x100,
    kDrvFormat565 = 0x200,
    kDrvFormat4444 = 0x300,
    kDrvFormat8888 = 0x500,
    kDrvFormat888 = 0x600,
    kDrvFormat16 = 0x700,
    kDrvFormatAutoMipmap = 0x1000,
    kDrvFormatPal8 = 0x2000,
    kDrvFormatPal4 = 0x4000,
    kDrvFormatMipmap = 0x8000,
    kDrvFormatPixelMask = 0x0f00,
};

/* RW error codes the driver reports (rwerror ids, plugin 1). */
enum {
    kDrvErrInvalidRasterFormat = (int32_t)0x80000011,
    kDrvErrInvalidRasterDepth = (int32_t)0x80000012,
    kDrvErrDevice = 0xb, /* "%s": the driver's message */
};

/* Driver state shared by device.c, raster.c and gpu.c. */
typedef struct DrvState {
    void *hwnd;              /* the game's window (RwEngineOpen's parameter) */
    int32_t currentMode;     /* index into modes, -1 when none */
    int32_t numModes;        /* modes[0] is the windowed desktop mode */
    DrvVideoMode modes[128]; /* video modes of the only sub system */
    int started;             /* between rwDEVICESYSTEMSTART and STOP */
    int32_t cameraDepth;     /* camera / normal rasters: depth and format of the display */
    int32_t cameraFormat;
    int32_t zDepth; /* z rasters */
    int32_t zFormat;
} DrvState;

extern DrvState drvState;

/* The driver's raster plugin data (0x20 bytes per raster, registered like the original's). */
typedef struct DrvRasterExt {
    struct SDL_GPUTexture *texture; /* camera raster: render target; z raster: depth texture */
    uint8_t *cpu;                   /* CPU copy in the raster's own format (what RwRasterLock returns) */
    uint32_t cpuSize;
    uint8_t lockFlags; /* rwRASTERLOCK* of the current lock */
    uint8_t pad0d[0x13];
} DrvRasterExt;

_Static_assert(sizeof(DrvRasterExt) == 0x20, "the original's raster plugin size");

extern int32_t drvCameraOffset; /* the driver's camera plugin data (DrvCameraExt) */
DrvRasterExt *drvRasterExt(const RwRaster *raster);
/* Dev aid: BMX_DRIVER_TRACE=1 logs every driver entry with its first three arguments. */
void drvTrace(const char *name, uint32_t a, uint32_t b, uint32_t c);
#define DRV_TRACE(name, a, b, c) drvTrace(name, a, b, c)

/* Sets RW's error slot like the original driver: plugin 1, errorCode = _rwerror(...)'s result. */
void drvSetError(int32_t errorCode);

/* The standards and plugin callbacks of raster.c (signatures as RW calls them). */
int drvCameraBeginUpdate(void *out, void *camera, int32_t in);
int drvCameraEndUpdate(void *out, void *camera, int32_t in);
int drvCameraClear(void *camera, void *color, int32_t flags);
int drvRasterShowRaster(void *raster, void *window, int32_t flags);
int drvRasterCreate(void *out, void *raster, int32_t flags);
int drvRasterDestroy(void *out, void *raster, int32_t in);
int drvHintRenderFront2Back(void *out, void *inOut, int32_t in);
int drvCameraUpdating(void);
extern int32_t drvRenderOrderHint;
extern RwRaster *drvContextRaster;
int drvSetRasterContext(void *out, void *raster, int32_t in);
/* state.c */
int drvRenderStateSet(int32_t state, int32_t value);
int drvRenderStateGet(int32_t state, void *value);
void drvRenderStateForgetRaster(const void *raster);
/* texture.c */
RwRaster *drvTopRaster(RwRaster *raster);
int drvRasterLock(void *out, void *raster, int32_t in);
int drvRasterUnlock(void *out, void *raster, int32_t in);
int drvImageFindRasterFormat(void *out, void *image, int32_t flags);
int drvRasterSetFromImage(void *raster, void *image, int32_t in);
int drvTextureSetRaster(void *texture, void *raster, int32_t in);
int drvRasterSubRaster(void *subRaster, void *raster, int32_t in);
void *drvCameraCtor(void *camera, int32_t offset, int32_t size);
void *drvCameraDtor(void *camera, int32_t offset, int32_t size);

/* gpu.c: the SDL3 side. All return 0 on failure (after logging the SDL error). */
int drvGpuInitVideo(void);
void drvGpuQuitVideo(void);
int drvGpuDesktopMode(DrvVideoMode *mode);
int drvGpuFullscreenSizes(int32_t (*sizes)[2], int max);
int drvGpuStart(void *hwnd);
void drvGpuStop(void);
struct SDL_GPUTexture *drvGpuCreateTarget(int32_t width, int32_t height, int depth);
void drvGpuReleaseTexture(struct SDL_GPUTexture *texture);
int drvGpuClear(struct SDL_GPUTexture *color, struct SDL_GPUTexture *depth, const uint8_t rgba[4], int flags);
int drvGpuPresent(struct SDL_GPUTexture *color, int32_t x, int32_t y, int32_t width, int32_t height);

#endif
