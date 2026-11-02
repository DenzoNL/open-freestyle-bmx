/* Scene markers (C:\NgEngine\Zge\Com\ZglRw\zrwcam.c). Not ported yet; callees only. */
#ifndef ZGE_COM_ZGLRW_ZRWCAM_H
#define ZGE_COM_ZGLRW_ZRWCAM_H

#include <stddef.h>
#include <stdint.h>

#include "compat/orig.h"
#include "zge/zmath.h"
#include "zge/zscene.h"

/* A named marker of the level scene (the "Player<n>" start points), kept in a singly linked list. */
typedef struct ZrwMarker {
    int32_t hash;           /* 0x00 hash of the base name */
    int32_t index;          /* 0x04 */
    ZMatrix matrix;         /* 0x08 frame matrix */
    void *data;             /* 0x48 object data of the point (a block from zmemAlloc, may be NULL) */
    struct ZrwMarker *next; /* 0x4c */
} ZrwMarker;

BMX_LAYOUT32_ASSERT(__builtin_offsetof(ZrwMarker, next) == 0x4c, "marker layout");

/* The scene point state's last field (0x00674014). */
#define gZrwMarkers (gScnpts.markers)

/* Looks a marker up by the hash of its base name and its number; NULL when absent. */
ZrwMarker *zrwcamFindMarker(int hash, int index);
/* Creates the camera pool and the camera and z-buffer rasters at the current screen size, then starts the
 * roller's shadow. Returns 0 when that fails. */
int zrwCameraInitialize(void);
/* Stops the shadow and frees the rasters and the pool; returns 0 when the shadow could not be stopped. */
int zrwCameraShutdown(void);
/* Recreates the rasters at a new size (asserts when cameras exist). Returns 0 only when it asserts. */
int zrwCameraSetResolution(int width, int height);

/* The renderer's camera interface: function pointers in the exe's data (0x561d88..). */
typedef int(BMX_CDECL *ZrwCamInitFn)(void);
typedef void *(BMX_CDECL *ZrwCamCreateFn)(int flags, void *owner);
typedef int(BMX_CDECL *ZrwCamDestroyFn)(void *camera);
/* Field of view as a ZAngle; 0 leaves it alone. */
typedef void(BMX_CDECL *ZrwCamSetFovFn)(void *camera, int fov);
/* Positions the camera: view matrix (the world to camera transform) and eye position. */
typedef void(BMX_CDECL *ZrwCamSetViewFn)(void *camera, const ZMatrix *view, const ZVec3 *eye);
#define gZrwCamInit    ORIG_VAR(0x00561d88, ZrwCamInitFn, gZrwCamInit)
#define gZrwCamCreate  ORIG_VAR(0x00561d90, ZrwCamCreateFn, gZrwCamCreate)
#define gZrwCamDestroy ORIG_VAR(0x00561d94, ZrwCamDestroyFn, gZrwCamDestroy)
#define gZrwCamSetFov  ORIG_VAR(0x00561d98, ZrwCamSetFovFn, gZrwCamSetFov)
#define gZrwCamSetView ORIG_VAR(0x00561da0, ZrwCamSetViewFn, gZrwCamSetView)

/* ---- cameras ------------------------------------------------------------------------------------ */

/* Renderer camera (0xc8 bytes, from the pool at 0x67401c): the RwCamera plus the state the engine keeps. */
typedef struct ZrwCamera {
    uint32_t flags; /* 0x00 bit 0: in use */
    uint8_t pad04[4];
    uint32_t state;    /* 0x08 bit 0: viewport rectangle set, bit 1: clear on begin, bit 2: has rasters */
    void *rwCamera;    /* 0x0c */
    void *owner;       /* 0x10 */
    ZMatrix world;     /* 0x14 copy of the RwCamera frame matrix */
    ZMatrix view;      /* 0x54 view basis; its position row (0x84) is the eye */
    float viewport[4]; /* 0x94 x0, y0, x1, y1 */
    int32_t width;     /* 0xa4 */
    int32_t height;    /* 0xa8 */
    float nearClip;    /* 0xac */
    float farClip;     /* 0xb0 */
    float offsetX;     /* 0xb4 raster offset */
    float offsetY;     /* 0xb8 */
    uint16_t fov;      /* 0xbc ZAngle */
    uint8_t padbe[2];
    uint32_t clearColor; /* 0xc0 bytes r, g, b, a */
    float clipOffset;    /* 0xc4 distance between the far plane and the fog end */
} ZrwCamera;

_Static_assert(offsetof(ZrwCamera, state) == 0x08, "camera layout");
BMX_LAYOUT32_ASSERT(offsetof(ZrwCamera, rwCamera) == 0x0c, "camera layout");
BMX_LAYOUT32_ASSERT(offsetof(ZrwCamera, world) == 0x14, "camera layout");
BMX_LAYOUT32_ASSERT(offsetof(ZrwCamera, view) == 0x54, "camera layout");
BMX_LAYOUT32_ASSERT(offsetof(ZrwCamera, viewport) == 0x94, "camera layout");
BMX_LAYOUT32_ASSERT(offsetof(ZrwCamera, width) == 0xa4, "camera layout");
BMX_LAYOUT32_ASSERT(offsetof(ZrwCamera, nearClip) == 0xac, "camera layout");
BMX_LAYOUT32_ASSERT(offsetof(ZrwCamera, offsetX) == 0xb4, "camera layout");
BMX_LAYOUT32_ASSERT(offsetof(ZrwCamera, fov) == 0xbc, "camera layout");
BMX_LAYOUT32_ASSERT(offsetof(ZrwCamera, clearColor) == 0xc0, "camera layout");
BMX_LAYOUT32_ASSERT(offsetof(ZrwCamera, clipOffset) == 0xc4, "camera layout");
BMX_LAYOUT32_ASSERT(sizeof(ZrwCamera) == 0xc8, "camera layout");

/* Creates a camera (flags bit 0: parallel projection, bit 1: no rasters); NULL on failure. */
ZrwCamera *zrwCameraCreate(uint32_t flags, void *owner);
/* Frees the RwCamera, its frame and rasters and the record; returns 1. */
int zrwCameraDestroy(ZrwCamera *camera);
/* Near and far planes; 0 leaves one alone. */
void zrwCameraSetClip(ZrwCamera *camera, float nearClip, float farClip);
/* Field of view (low 16 bits, a ZAngle); 0 leaves it alone. */
void zrwCameraSetFov(ZrwCamera *camera, int fov);
uint16_t zrwCameraGetFov(const ZrwCamera *camera);
/* Positions the camera from a view matrix and an eye point (the world offset and scale are applied). */
void zrwCameraSetView(ZrwCamera *camera, const ZMatrix *view, const ZVec3 *eye);
ZMatrix *zrwCameraGetViewMatrix(ZrwCamera *camera);
/* Projects a view space point to the screen; 0 when it is outside the clip planes. */
int zrwCameraProject(const ZrwCamera *camera, const ZVec3 *point, float *out);
/* Sets the raster rectangles (x, y, w, h as floats) of both rasters and the offsets; reads the fov. */
void zrwCameraSetRect(ZrwCamera *camera, const float *rect);
/* Viewport rectangle (NULL: the whole raster); with bit 0 of `relative` the rectangle is in fractions. */
void zrwCameraSetViewport(ZrwCamera *camera, uint8_t relative, const float *rect);
ZrwCamera *zrwCameraFirst(void);
ZrwCamera *zrwCameraNext(ZrwCamera *camera);
/* Starts rendering through the camera; 0 when the RwCamera or the raster context cannot be set up. */
int zrwCameraBegin(ZrwCamera *camera);
int zrwCameraEnd(void);
void *zrwCameraGetRw(const ZrwCamera *camera);
ZrwCamera *zrwCameraGetCurrent(void);
ZMatrix *zrwCameraGetWorldMatrix(ZrwCamera *camera);
void *zrwCameraGetOwner(const ZrwCamera *camera);
/* Setters of the parameter dispatch (zrwzgl.c): viewport word, view window word, clear flag and colour. */
void zrwCameraSetViewportWord(ZrwCamera *camera, uint32_t value);
void zrwCameraSetViewWindowWord(ZrwCamera *camera, const uint32_t *value);
void zrwCameraSetClearFlag(ZrwCamera *camera, uint32_t flag);
void zrwCameraSetClearColor(ZrwCamera *camera, const uint32_t *color);
void zrwCameraSetClipOffset(ZrwCamera *camera, float offset);
/* Moves the RwCamera's view offset. */
void zrwCameraSetViewOffset(ZrwCamera *camera, float x, float y);

#endif
