/*
 * Effects (C:\zaxis\Bmx\src\com\action\fx.c): thin glue between the game and the particle system of
 * the engine (NgIFfx). Each effect fills a parameter block, in the stack or in a block of .data, and
 * hands it to the system or to a handler registered by name ("spark", "ripple", "splash", ...).
 */
#ifndef BMX_COM_ACTION_FX_H
#define BMX_COM_ACTION_FX_H

#include <stdint.h>

#include "bmx/com/action/physics_types.h"
#include "compat/orig.h"
#include "zge/zmath.h"

/* Engine side: the callbacks the effect nodes of a level call (stored at 0x5f34b4 / 0x5f34b8). */
typedef int(BMX_CDECL *FxLightFn)(const uint8_t *obj, int event);
typedef int(BMX_CDECL *FxListFn)(const uint8_t *list, uint32_t count);
#define gFxLightCallback ORIG_VAR(0x005f34b4, FxLightFn, gFxLightCallback)
#define gFxListCallback  ORIG_VAR(0x005f34b8, FxListFn, gFxListCallback)

/* State flags and the rider whose actor the impact sparks attach to. */
#define gFxActive ORIG_VAR(0x0056b6c4, uint32_t, gFxActive)
#define gFxUnused ORIG_VAR(0x0056b6c8, uint32_t, gFxUnused)
#define gFxRider  ORIG_VAR(0x0056b6cc, PhysRider *, gFxRider)

void fxNop1(void);
void fxNop2(void);
/* Remembers the rider whose actor the impact sparks attach to. */
void fxSetRider(PhysRider *rider);
/* Registers the engine callbacks and the effect handlers; returns 0. */
uint32_t fxInit(void);
void fxShutdown(void);
/* Stops all effects. */
void fxStopAll(void);
void fxStopAllMode(int mode);
/* Burst of sparkles that rewards a collected goal. */
void fxBurst(void);
/* A spark flying out of an object's effect node `index` at a world position (the callers pass a seed that is
 * not used). */
void fxSpawn(const void *obj, int index, const ZVec3 *pos, uint32_t seed);
/* Puff at a world position (parameter block 0x554738). */
void fxPuff(const ZVec3 *pos);
/* Sparks where a hard impact happened (position, velocity of the sparks). */
void fxImpactSparks(const ZVec3 *pos, const ZVec3 *vel);
/* Scatters the sprites of table 0x554798 around `offset`, turned by `angle`, as effect `kind`. */
void fxScatter(const uint32_t *sprites, int kind, const ZVec3 *offset, int angle, const ZVec3 *base);

/* Emitter of effect `type` (0..5) at a position (asserts type < 6). */
void fxEmit(uint32_t a, uint32_t b, const ZVec3 *pos, uint32_t unused, uint32_t type);
/* Creates effect `type` (0..5) at a position with an extra parameter block. */
void fxCreate(uint32_t type, const ZVec3 *pos, const void *arg);

/* Callbacks of the level's effect nodes. */
void fxWaterSplash(const uint8_t *obj, int unused, const float *offset);
void fxNop(void);
void fxCallbackEmit(const uint8_t *obj, uint32_t type);
int fxCallbackLight(const uint8_t *obj, int event);
int fxCreateFromList(const uint8_t *list, uint32_t count);

/* A burst of shards at a position, flying along `vel` (relative to the first effect node). */
void fxShards(const ZVec3 *pos, const ZVec3 *vel);
/* Skid mark quad from `from` to `to` on a surface with `normal`; `flags` is not read. */
void fxSkidMark(const ZVec3 *from, const ZVec3 *to, int flags, const ZVec3 *normal);

#endif
