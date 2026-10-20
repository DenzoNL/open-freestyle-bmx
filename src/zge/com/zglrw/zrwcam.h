/* Scene markers (C:\NgEngine\Zge\Com\ZglRw\zrwcam.c). Not ported yet; callees only. */
#ifndef ZGE_COM_ZGLRW_ZRWCAM_H
#define ZGE_COM_ZGLRW_ZRWCAM_H

#include <stdint.h>

#include "compat/orig.h"
#include "zge/zmath.h"

/* A named marker of the level scene (the "Player<n>" start points): its frame matrix sits at +8. */
typedef struct ZrwMarker {
    uint8_t pad00[0x8];
    ZMatrix matrix;
} ZrwMarker;

/* Looks a marker up by the hash of its base name and its number; NULL when absent. */
ORIG_FN(0x00492130, ZrwMarker *, zrwcamFindMarker, (int hash, int index));

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
