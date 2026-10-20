/* Light objects (C:\NgEngine\Zge\ZgeMods\zobjLight.c). */
#ifndef ZGE_ZGEMODS_ZOBJLIGHT_H
#define ZGE_ZGEMODS_ZOBJLIGHT_H

#include <stdint.h>

#include "compat/orig.h"
#include "zge/zmath.h"

/* The renderer's light interface: function pointers in the exe's data (0x563420 table). */
typedef void *(BMX_CDECL *ZrwLightCreateFn)(int type, int param);
typedef void *(BMX_CDECL *ZrwLightColorFn)(void *light, const uint8_t *rgba);
typedef void *(BMX_CDECL *ZrwLightFlagsFn)(void *light, uint8_t flags);
typedef void *(BMX_CDECL *ZrwLightDirectionFn)(void *light, const ZVec3 *direction);
#define gZrwLightCreate    ORIG_VAR(0x00563428, ZrwLightCreateFn, gZrwLightCreate)
#define gZrwLightColor     ORIG_VAR(0x00563430, ZrwLightColorFn, gZrwLightColor)
#define gZrwLightFlags     ORIG_VAR(0x00563434, ZrwLightFlagsFn, gZrwLightFlags)
#define gZrwLightDirection ORIG_VAR(0x0056343c, ZrwLightDirectionFn, gZrwLightDirection)

/* Creates a light object of renderer type 0..3; NULL on failure. */
ORIG_FN(0x00490010, void *, zobjLightCreate, (int type, int param));

/* The light's renderer handle is at +0x2c of the object; these forward to the renderer (0x563420 table). */
void zobjLightSetColor(void *light, const uint8_t *rgba);
void zobjLightSetFlags(void *light, uint8_t flags);
void zobjLightSetDirection(void *light, const ZVec3 *direction);

#endif
