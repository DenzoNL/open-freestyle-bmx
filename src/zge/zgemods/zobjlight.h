/* Light objects (C:\NgEngine\Zge\ZgeMods\zobjLight.c). */
#ifndef ZGE_ZGEMODS_ZOBJLIGHT_H
#define ZGE_ZGEMODS_ZOBJLIGHT_H

#include <stdint.h>

#include "compat/orig.h"
#include "zge/zmath.h"
#include "zge/zquat.h"

/* The renderer's light interface: function pointers in the exe's data (0x563420 table). */
typedef void *(BMX_CDECL *ZrwLightCreateFn)(int type, int param);
typedef void *(BMX_CDECL *ZrwLightColorFn)(void *light, const uint8_t *rgba);
typedef void *(BMX_CDECL *ZrwLightFlagsFn)(void *light, uint8_t flags);
typedef void *(BMX_CDECL *ZrwLightDirectionFn)(void *light, const ZVec3 *direction);
#define gZrwLightCreate    ORIG_VAR(0x00563428, ZrwLightCreateFn, gZrwLightCreate)
#define gZrwLightColor     ORIG_VAR(0x00563430, ZrwLightColorFn, gZrwLightColor)
#define gZrwLightFlags     ORIG_VAR(0x00563434, ZrwLightFlagsFn, gZrwLightFlags)
#define gZrwLightDirection ORIG_VAR(0x0056343c, ZrwLightDirectionFn, gZrwLightDirection)

typedef void(BMX_CDECL *ZrwLightDestroyFn)(void *light);
typedef void(BMX_CDECL *ZrwLightParamFn)(void *light, uint32_t param);
#define gZrwLightDestroy  ORIG_VAR(0x0056342c, ZrwLightDestroyFn, gZrwLightDestroy)
#define gZrwLightSetParam ORIG_VAR(0x00563438, ZrwLightParamFn, gZrwLightSetParam)

/* Scene object of a light (the part used here): the ops table is at +0x28, the renderer handle at +0x2c. */
typedef struct ZObjLight {
    uint8_t pad00[0x28];
    const void *ops;
    void *handle;
} ZObjLight;

_Static_assert(__builtin_offsetof(ZObjLight, handle) == 0x2c, "light object layout");

/* Creates a light object of renderer type 0..3; NULL on failure. */
void *zobjLightCreate(int type, int param);
/* Object operations (the ops table at 0x561858). They forward to the renderer and return 1. */
int zobjLightDestroy(ZObjLight *light);
int zobjLightSetParam(ZObjLight *light, uint32_t param);
/* Points the light along the axis of a rotation quaternion. */
int zobjLightSetRotation(ZObjLight *light, const ZQuat *rotation);
/* Fatal when called twice. */
int zobjLightInitialize(void);

/* The light's renderer handle is at +0x2c of the object; these forward to the renderer (0x563420 table). */
void zobjLightSetColor(void *light, const uint8_t *rgba);
void zobjLightSetFlags(void *light, uint8_t flags);
void zobjLightSetDirection(void *light, const ZVec3 *direction);

#endif
