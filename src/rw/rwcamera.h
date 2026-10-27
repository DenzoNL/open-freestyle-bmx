/*
 * RenderWare 3.1 camera (rw/camera.c) and the charset utility (rw/charset.c).
 *
 * Camera work that reaches the device goes through function pointers: the camera's own begin/end update
 * callbacks and the engine's standard function table. Those stay indirect (the seam for another renderer).
 */
#ifndef RW_RWCAMERA_H
#define RW_RWCAMERA_H

#include "rw/rwcore.h"

typedef struct RwRaster RwRaster;
typedef struct RwCamera RwCamera;

typedef RwCamera *(BMX_CDECL *RwCameraUpdateFunc)(RwCamera *camera);

enum {
    rwPERSPECTIVE = 1,
    rwPARALLEL = 2,
};

/* Entry of the engine's standard function table that clears a camera's rasters. */
enum { rwSTANDARDCAMERACLEAR = 0x15 };

typedef struct RwV2d {
    float x, y;
} RwV2d;

/* RwCamera: an object with a frame, a view matrix, two rasters, the view window and the clip planes. */
struct RwCamera {
    RwObjectHasFrame object;        /* 0x00 type 4; sync callback at 0x10 */
    int32_t projection;             /* 0x14 */
    RwCameraUpdateFunc beginUpdate; /* 0x18 */
    RwCameraUpdateFunc endUpdate;   /* 0x1c */
    RwMatrix viewMatrix;            /* 0x20 */
    RwRaster *frameBuffer;          /* 0x60 */
    RwRaster *zBuffer;              /* 0x64 */
    RwV2d viewWindow;               /* 0x68 */
    RwV2d recipViewWindow;          /* 0x70 */
    RwV2d viewOffset;               /* 0x78 */
    float nearPlane;                /* 0x80 */
    float farPlane;                 /* 0x84 */
    float fogPlane;                 /* 0x88 */
    float zScale;                   /* 0x8c */
    float zShift;                   /* 0x90 */
    uint8_t pad94[2];               /* 0x94 */
    uint8_t version;                /* 0x96 bumped whenever a setting changes */
};

_Static_assert(offsetof(RwCamera, projection) == 0x14, "RwCamera layout");
_Static_assert(offsetof(RwCamera, viewMatrix) == 0x20, "RwCamera layout");
_Static_assert(offsetof(RwCamera, frameBuffer) == 0x60, "RwCamera layout");
_Static_assert(offsetof(RwCamera, viewWindow) == 0x68, "RwCamera layout");
_Static_assert(offsetof(RwCamera, nearPlane) == 0x80, "RwCamera layout");
_Static_assert(offsetof(RwCamera, zShift) == 0x90, "RwCamera layout");
_Static_assert(offsetof(RwCamera, version) == 0x96, "RwCamera layout");

#define rwCameraOffset ORIG_VAR(0x00676d10, int32_t, rwCameraOffset)

RwCamera *RwCameraCreate(void);
int RwCameraDestroy(RwCamera *camera);
RwCamera *RwCameraSetFrame(RwCamera *camera, RwFrame *frame);
RwFrame *RwCameraGetFrame(const RwCamera *camera);
RwCamera *RwCameraSetViewOffset(RwCamera *camera, const RwV2d *offset);
RwCamera *RwCameraSetRaster(RwCamera *camera, RwRaster *raster);
RwRaster *RwCameraGetRaster(const RwCamera *camera);
RwCamera *RwCameraSetZRaster(RwCamera *camera, RwRaster *raster);
RwRaster *RwCameraGetZRaster(const RwCamera *camera);
RwCamera *RwCameraSetNearClipPlane(RwCamera *camera, float nearClip);
float RwCameraGetNearClipPlane(const RwCamera *camera);
RwCamera *RwCameraSetFarClipPlane(RwCamera *camera, float farClip);
float RwCameraGetFarClipPlane(const RwCamera *camera);
RwCamera *RwCameraSetFogDistance(RwCamera *camera, float fogDistance);
RwCamera *RwCameraSetProjection(RwCamera *camera, int32_t projection);
RwCamera *RwCameraSetViewWindow(RwCamera *camera, const RwV2d *viewWindow);
const RwV2d *RwCameraGetViewWindow(const RwCamera *camera);
/* Creates the PVS camera and image (asm entry: EAX = world, EDX / ECX = parent camera / z raster or NULL, ST0
 * = far clip plane). */
void rwPvsCameraCreate(void);
RwCamera *RwCameraBeginUpdate(RwCamera *camera);
RwCamera *RwCameraEndUpdate(RwCamera *camera);
RwCamera *RwCameraGetCurrentCamera(void);
RwCamera *RwCameraClear(RwCamera *camera, const void *color, int32_t flags);
float RwIm2DGetFarScreenZ(void);

/* Recomputes zScale and zShift from the clip planes and the device's screen z range (EAX = camera). */
__attribute__((regparm(1))) void CameraUpdateZShiftScale(RwCamera *camera);

/* ---- charset (a raster holding the debug font) ------------------------------------------------------- */

typedef struct RwRGBA {
    uint8_t red, green, blue, alpha;
} RwRGBA;

typedef RwRaster RtCharset;

/* Glyph size and count, and how many glyphs fit a row / how many rows the raster needs. */
typedef struct RtCharsetDesc {
    int32_t width;
    int32_t height;
    int32_t count;
    int32_t glyphsPerRow;
    int32_t rows;
} RtCharsetDesc;

RtCharset *RtCharsetCreate(const RwRGBA *foreground, const RwRGBA *background);
/* RtCharsetPrint's worker: EAX = charset, EDX = text, ECX = x, y is the fourth stack word. */
__attribute__((regparm(3))) int rtCharsetPrintText(RtCharset *charset, const char *text, int32_t x,
                                                   int32_t unused0, int32_t unused1, int32_t unused2,
                                                   int32_t y);
int RtCharsetDestroy(RtCharset *charset);
RtCharset *RtCharsetGetDesc(RtCharset *charset, RtCharsetDesc *desc);
/* EAX = charset, EDX = foreground, ECX = background. */
__attribute__((regparm(3))) RtCharset *RtCharsetSetColors(RtCharset *charset, const RwRGBA *foreground,
                                                          const RwRGBA *background);
int RtCharsetPrint(RtCharset *charset, const char *text, int32_t x, int32_t y);

int RwIm2DRenderIndexedPrimitive(int32_t primType, void *vertices, int32_t numVertices, void *indices,
                                 int32_t numIndices);

#endif
