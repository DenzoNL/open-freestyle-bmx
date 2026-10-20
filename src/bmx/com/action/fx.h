/* Particle effects (C:\zaxis\Bmx\src\com\action\fx.c). Not ported yet; callees only. */
#ifndef BMX_COM_ACTION_FX_H
#define BMX_COM_ACTION_FX_H

#include <stdint.h>

#include "compat/orig.h"
#include "zge/zmath.h"

/* Spawns effect `kind` at a world position for an actor. */
ORIG_FN(0x00408aa0, void, fxSpawn, (void *actor, int kind, const ZVec3 *pos, uint32_t seed));
ORIG_FN(0x00409250, void, fxSkidMark, (const ZVec3 *from, const ZVec3 *to, int flags, const ZVec3 *normal));

#endif
