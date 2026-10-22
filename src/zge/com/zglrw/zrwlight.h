/*
 * Renderer lights (by address 0x49f520-0x49f8d0; the source file is not named in the data file, which files
 * the functions under zge.c). Each light is a 12 byte record around a RenderWare light; the scene's light
 * interface (the table at 0x563420, used by zobjLight.c) points here.
 */
#ifndef ZGE_COM_ZGLRW_ZRWLIGHT_H
#define ZGE_COM_ZGLRW_ZRWLIGHT_H

#include <stdint.h>

#include "compat/orig.h"
#include "zge/zmath.h"
#include "zge/zmem.h"

typedef struct ZrwLight {
    uint32_t flags; /* 0x00 */
    uint32_t pad04;
    void *light; /* 0x08 RpLight */
} ZrwLight;

_Static_assert(sizeof(ZrwLight) == 12, "light record");

#define gZrwLightPool        ORIG_VAR(0x00675374, ZMemFreeList *, gZrwLightPool)
#define gZrwLightInitialized ORIG_VAR(0x00675370, int, gZrwLightInitialized)

/* Frees the pool of light records. Returns 1. */
int zrwLightShutdown(void);
/* Creates a light of renderer kind 0..3 with the given RGBA colour and adds it to the world; NULL on failure.
 */
ZrwLight *zrwLightCreate(int kind, const uint8_t *rgba);
/* Removes the light from the world and frees it. Returns 1. */
int zrwLightDestroy(ZrwLight *light);
/* The setters return the light. */
ZrwLight *zrwLightSetColor(ZrwLight *light, const uint8_t *rgba);
/* Bit 0 of the argument lights the world, bit 1 lights the atomics. */
ZrwLight *zrwLightSetFlags(ZrwLight *light, uint8_t flags);
/* Position in game coordinates. */
ZrwLight *zrwLightSetPosition(ZrwLight *light, const float *position);
/* Points the light's frame along the direction (y and z mirrored for RenderWare). */
ZrwLight *zrwLightSetDirection(ZrwLight *light, const float *direction);
/* Cone angle in ZAngle units. */
ZrwLight *zrwLightSetConeAngle(ZrwLight *light, int16_t angle);
ZrwLight *zrwLightSetRadius(ZrwLight *light, float radius);

#endif
