/* Actor rendering glue (C:\zaxis\Bmx\src\com\zge\NgIF\NgIFzactor.c). Not ported yet; callees only. */
#ifndef BMX_COM_ZGE_NGIF_NGIFZACTOR_H
#define BMX_COM_ZGE_NGIF_NGIFZACTOR_H

#include "compat/orig.h"
#include "zge/zmath.h"

/* World position of `local` in the frame of model part `part` of an actor. */
ORIG_FN(0x00464b50, void, zactorGetPartPosition, (void *actor, int part, const ZVec3 *local, ZVec3 *out));

/* Applies the pose of a rider / bike actor (limb joints) to its model. */
ORIG_FN(0x004654b0, void, zactorSyncPose, (void *actor));

/* Clears the limb hash tables (0x5da1e8..) the pose sync fills. */
ORIG_FN(0x004646f0, void, zactorClearPoseTables, (void));
/* Finishes a freshly created rider / bike actor (limb setup). */
ORIG_FN(0x00465130, void, zactorSetupRider, (void *actor));
ORIG_FN(0x00465340, void, zactorSetupBike, (void *actor));
/* Length of model part `part` of an actor (a vector length, returned unrounded on the x87 stack). */
ORIG_FN(0x00464ea0, double, zactorGetPartLength, (void *actor, int part));

#endif
