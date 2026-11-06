/*
 * The SDL3 GPU driver for RenderWare 3.1: it replaces the original's Direct3D 7 driver (lib/d3d7driver in
 * data/functions-rw.csv) behind RW's device interface. See docs/SDL3-DRIVER.md.
 *
 * Native exe only (never the hook DLL). The original's device table (0x566fe4), standards table (0x56701c)
 * and core extension table (0x69bee0) stay as they are in the copied data; each original driver entry point
 * we implement is served by our function (bmxDriverJumps, read by tools/gen_origmap.py), so the loader
 * relocates those tables' pointers to them. An entry point we do not implement becomes a trap stub that
 * reports its address.
 */
#ifndef BMX_DRIVER_DRIVER_H
#define BMX_DRIVER_DRIVER_H

#include <stddef.h>
#include <stdint.h>

#include "compat/fpu.h"
#include "compat/layout.h"
#include "compat/x87soft.h"
#include "hook/hook.h"
#include "rw/rwcamera.h"
#include "rw/rwcore.h"
#include "rw/rwd3d.h"
#include "rw/rwtexture.h"

/*
 * The FPU state around a call into the graphics driver. The game's float code runs on SSE in the rounding the
 * port chose (round to nearest; the x87's mode only inside rwD3DRoundBegin / End, rw/rwd3d.h). A call into
 * the GPU driver can leave the calling thread's MXCSR changed: on 32-bit Windows this PC's driver, compiling
 * a pipeline, sets the SSE rounding from the x87 control word, which inside a camera update rounds down, so
 * the game logic after it rounded down too (docs/PORTING.md X3). Every SDL GPU call that runs driver code
 * goes through DRV_FPU_GUARDED, which puts both control words back as they were.
 */
typedef struct DrvFpuState {
    uint32_t mxcsr;
    uint16_t x87;
} DrvFpuState;

static inline DrvFpuState drvFpuSave(void)
{
    DrvFpuState s = {0, 0};
#if defined(__i386__) || defined(__x86_64__)
    __asm__ volatile("stmxcsr %0" : "=m"(s.mxcsr)::"memory");
    __asm__ volatile("fnstcw %0" : "=m"(s.x87)::"memory");
#elif defined(__aarch64__)
    s.mxcsr = (uint32_t)bmxFpcrGet(); /* FPCR in the MXCSR slot (compat/fpu.h) */
    s.x87 = bmxX87ControlWord();      /* the control word variable (compat/x87soft.h) */
#endif
    return s;
}

static inline void drvFpuRestore(DrvFpuState s)
{
#if defined(__i386__) || defined(__x86_64__)
    __asm__ volatile("fldcw %0" ::"m"(s.x87) : "memory");
    __asm__ volatile("ldmxcsr %0" ::"m"(s.mxcsr) : "memory");
#elif defined(__aarch64__)
    bmxX87SetControlWord(s.x87);
    bmxFpcrSet(s.mxcsr);
#else
    (void)s;
#endif
}

#define DRV_FPU_GUARDED(call)                                                                                \
    __extension__({                                                                                          \
        const DrvFpuState drvFpuSaved_ = drvFpuSave();                                                       \
        __typeof__(call) drvFpuResult_ = (call);                                                             \
        drvFpuRestore(drvFpuSaved_);                                                                         \
        drvFpuResult_;                                                                                       \
    })

/* (original address, our function) pairs: these override a port of the same address (tools/gen_origmap.py).
 */
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
    void *window;            /* the game's window (RwEngineOpen's parameter, window.c's token) */
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

/*
 * The driver's raster plugin data (0x20 bytes per raster, registered like the original's). The ported render
 * states (rw/d3dstate.c) read three fields at the original's offsets: the colour key flag, the texture handle
 * the device's SetTexture gets, and the pixel format record whose flags say whether the texture has alpha.
 */
typedef struct DrvRasterExt {
    struct SDL_GPUTexture *texture; /* 0x00 camera: render target; z: depth texture; texture: RGBA8 copy */
    int32_t colorKey;               /* 0x04 always 0 (no colour keyed textures) */
    uint8_t *cpu;                   /* 0x08 CPU copy in the raster's own format (what RwRasterLock returns) */
    RwRaster *handle;               /* 0x0c the texture handle: the raster itself (textures only) */
    const void *info;               /* 0x10 RwD3DRasterInfo: alpha or not */
    uint32_t cpuSize;               /* 0x14 */
    uint8_t lockFlags;              /* 0x18 mip level of the current lock */
    uint8_t dirty;                  /* 0x19 the GPU texture needs an upload */
    uint8_t pad1a[6];
} DrvRasterExt;

BMX_LAYOUT32_ASSERT(sizeof(DrvRasterExt) == 0x20, "the original's raster plugin size");
_Static_assert(offsetof(DrvRasterExt, colorKey) == offsetof(RwD3DRasterExt, colorKey) &&
                   offsetof(DrvRasterExt, handle) == offsetof(RwD3DRasterExt, handle) &&
                   offsetof(DrvRasterExt, info) == offsetof(RwD3DRasterExt, info),
               "the raster plugin fields rw/d3dstate.c reads (RwD3DRasterExt)");

extern int32_t drvCameraOffset; /* the driver's camera plugin data (DrvCameraExt, no pointers) */
enum { kDrvCameraExtBytes = 0x20 };
DrvRasterExt *drvRasterExt(const RwRaster *raster);
/* Dev aid: BMX_DRIVER_TRACE=1 logs every driver entry with its first three arguments. */
void drvTrace(const char *name, uintptr_t a, uintptr_t b, uintptr_t c);
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
RwCamera *drvCameraCurrent(void); /* the camera between its begin and end update, else NULL */
extern int32_t drvRenderOrderHint;
extern RwRaster *drvContextRaster;
int drvSetRasterContext(void *out, void *raster, int32_t in);
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
int drvGpuStart(void *window);
void drvGpuStop(void);
struct SDL_GPUTexture *drvGpuCreateTarget(int32_t width, int32_t height, int depth);
void drvGpuReleaseTexture(struct SDL_GPUTexture *texture);
int drvGpuClear(struct SDL_GPUTexture *color, struct SDL_GPUTexture *depth, const uint8_t rgba[4], int flags);
int drvGpuPresent(struct SDL_GPUTexture *color, int32_t x, int32_t y, int32_t width, int32_t height);
struct SDL_GPUDevice *drvGpuDevice(void);
int drvGpuShownFrames(void);     /* frames presented so far (RwRasterShowRaster) */
void drvGpuSetUncounted(int on); /* 1: presents do not count (the high frame rate mode's extra draws) */
/* BMX_HF_GUARD: after drvGpuMeasureAcquire, the ns spent waiting for swapchain textures so far. */
void drvGpuMeasureAcquire(void);
uint64_t drvGpuAcquireNs(void);
int drvGpuPresentMovie(struct SDL_GPUTexture *color, int32_t width, int32_t height, const char *shot);
void drvGpuWindowSize(int32_t *width, int32_t *height);
/* movie.c: releases the movie textures (before the GPU device goes). */
void drvMovieRelease(void);

/* draw.c: the D3D7 device calls of the ported driver code (rw/rwd3d.h) on SDL3 GPU. */
int drvDrawStart(void);
void drvDrawStop(void);
void drvDrawFlush(void);
void drvDrawSetTarget(struct SDL_GPUTexture *color, struct SDL_GPUTexture *depth, int32_t width,
                      int32_t height, int32_t x, int32_t y, int32_t w, int32_t h);
void drvDrawSetUiMode(int mode);
void drvDrawFillRect(float x0, float y0, float x1, float y1);
void drvDrawTextureChanged(RwRaster *raster);
void drvDrawForgetRaster(RwRaster *raster);

#endif
