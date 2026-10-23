/* Scene markers (C:\NgEngine\Zge\Com\ZglRw\zrwcam.c). Not ported yet; callees only. */
#ifndef ZGE_COM_ZGLRW_ZRWCAM_H
#define ZGE_COM_ZGLRW_ZRWCAM_H

#include <stdint.h>

#include "compat/orig.h"
#include "zge/zmath.h"

/* A named marker of the level scene (the "Player<n>" start points), kept in a singly linked list. */
typedef struct ZrwMarker {
    int32_t hash;   /* 0x00 hash of the base name */
    int32_t index;  /* 0x04 */
    ZMatrix matrix; /* 0x08 frame matrix */
    uint8_t pad48[4];
    struct ZrwMarker *next; /* 0x4c */
} ZrwMarker;

_Static_assert(__builtin_offsetof(ZrwMarker, next) == 0x4c, "marker layout");

#define gZrwMarkers ORIG_VAR(0x00674014, ZrwMarker *, gZrwMarkers)

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

#endif
