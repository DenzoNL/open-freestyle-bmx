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

/* A clip plane of the view frustum: unit normal (pointing out), distance from the origin along it, and per
 * axis 1 when the normal component is not negative (sign bit clear), else 0. */
typedef struct RwFrustumPlane {
    ZVec3 normal;       /* 0x00 */
    float distance;     /* 0x0c */
    uint8_t closest[3]; /* 0x10 x, y, z */
    uint8_t pad13;      /* 0x13 */
} RwFrustumPlane;

_Static_assert(sizeof(RwFrustumPlane) == 0x14, "RwFrustumPlane layout");

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
    uint8_t pad97;                  /* 0x97 */
    /* 0x98 far, near, left, top, right, bottom (CameraSync rebuilds them in world space) */
    RwFrustumPlane frustumPlanes[6];
    float frustumBoundBox[6]; /* 0x110 RwBBox of the corners (sup, inf) */
    ZVec3 frustumCorners[8];  /* 0x128 near corners 0..3, far corners 4..7 */
};

_Static_assert(offsetof(RwCamera, projection) == 0x14, "RwCamera layout");
_Static_assert(offsetof(RwCamera, viewMatrix) == 0x20, "RwCamera layout");
_Static_assert(offsetof(RwCamera, frameBuffer) == 0x60, "RwCamera layout");
_Static_assert(offsetof(RwCamera, viewWindow) == 0x68, "RwCamera layout");
_Static_assert(offsetof(RwCamera, nearPlane) == 0x80, "RwCamera layout");
_Static_assert(offsetof(RwCamera, zShift) == 0x90, "RwCamera layout");
_Static_assert(offsetof(RwCamera, version) == 0x96, "RwCamera layout");
_Static_assert(offsetof(RwCamera, frustumPlanes) == 0x98, "RwCamera layout");
_Static_assert(offsetof(RwCamera, frustumCorners) == 0x128, "RwCamera layout");
_Static_assert(sizeof(RwCamera) == 0x188, "RwCamera layout");

#define rwCameraOffset ORIG_VAR(0x00676d10, int32_t, rwCameraOffset)

void *_rwCameraOpen(void *engine, int32_t offset);
void *_rwCameraClose(void *engine);
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
/* The camera's own begin / end update callbacks (stored in new cameras) and the sphere test: 0 outside, 1
 * crossing, 2 inside (sphere = x, y, z, radius). */
RwCamera *CameraBeginUpdate(RwCamera *camera);
RwCamera *CameraEndUpdate(RwCamera *camera);
int RwCameraFrustumTestSphere(const RwCamera *camera, const float *sphere);
RwCamera *RwCameraClear(RwCamera *camera, const void *color, int32_t flags);
float RwIm2DGetFarScreenZ(void);

/* Rebuild the frustum corners and clip planes in world space from the camera's frame (EAX = camera). */
__attribute__((regparm(1))) void CameraBuildPerspClipPlanes(RwCamera *camera);
__attribute__((regparm(1))) void CameraBuildParallelClipPlanes(RwCamera *camera);
/* The view matrix builders (EAX = camera, which they return) and the camera sync callback. */
__attribute__((regparm(1))) RwCamera *CameraBuildPerspViewMatrix(RwCamera *camera);
__attribute__((regparm(1))) RwCamera *CameraBuildParallelViewMatrix(RwCamera *camera);
RwCamera *CameraSync(RwCamera *camera);

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
