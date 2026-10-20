/* Actor rendering glue (C:\zaxis\Bmx\src\com\zge\NgIF\NgIFzactor.c). Not ported yet; callees only. */
#ifndef BMX_COM_ZGE_NGIF_NGIFZACTOR_H
#define BMX_COM_ZGE_NGIF_NGIFZACTOR_H

#include "compat/orig.h"
#include "zge/zmath.h"

/* World position of `local` in the frame of model part `part` of an actor. */
ORIG_FN(0x00464b50, void, zactorGetPartPosition, (void *actor, int part, const ZVec3 *local, ZVec3 *out));

#endif
