/* Particle effects (C:\zaxis\Bmx\src\com\action\fx.c). Not ported yet; callees only. */
#ifndef BMX_COM_ACTION_FX_H
#define BMX_COM_ACTION_FX_H

#include <stdint.h>

#include "compat/orig.h"
#include "zge/zmath.h"

/* Spawns effect `kind` at a world position for an actor. */
ORIG_FN(0x00408aa0, void, fxSpawn, (void *actor, int kind, const ZVec3 *pos, uint32_t seed));
ORIG_FN(0x00409250, void, fxSkidMark, (const ZVec3 *from, const ZVec3 *to, int flags, const ZVec3 *normal));

/* Emitter of effect `type` (0..5) at a position. */
ORIG_FN(0x004095a0, void, fxEmit, (uint32_t a, uint32_t b, const ZVec3 *pos, uint32_t c, uint32_t type));
/* Creates effect `type` (0..5) at a position with an extra parameter block. */
ORIG_FN(0x00409640, void, fxCreate, (uint32_t type, const ZVec3 *pos, const void *arg));
/* Burst of sparkles that rewards a collected goal. */
ORIG_FN(0x00408a80, void, fxBurst, (void));
/* Sparks where a hard impact happened (position, velocity of the sparks). */
ORIG_FN(0x00408c90, void, fxImpactSparks, (const ZVec3 *pos, const ZVec3 *vel));

#endif
