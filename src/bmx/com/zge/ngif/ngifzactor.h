/*
 * Actor part lookup (C:\zaxis\Bmx\src\com\zge\NgIF\NgIFzactor.c). Riders (skeleton 0) and bikes are
 * models whose parts (limbs, wheels, grips, ...) are found by name hash once per player and cached in
 * the tables below; actors only keep the hashes of their own parts.
 */
#ifndef BMX_COM_ZGE_NGIF_NGIFZACTOR_H
#define BMX_COM_ZGE_NGIF_NGIFZACTOR_H

#include <stdint.h>

#include "compat/orig.h"
#include "zge/zmath.h"

enum { kZactorRiderParts = 15, kZactorBikeParts = 10 };

/* Part cache of two players. The *Ready words say whether a player's row has been looked up. */
typedef struct ZactorPoseTables {
    uint32_t bikeReady[2];
    void *bikeNodes[2][kZactorBikeParts]; /* skeleton nodes (zrwmatFindNode), NULL when missing */
    uint32_t riderReady[2];
    void *riderNodes[2][kZactorRiderParts];
} ZactorPoseTables;

BMX_LAYOUT32_ASSERT(sizeof(ZactorPoseTables) == 0xd8, "pose tables");

#define gZactorPose ORIG_VAR(0x005da1e8, ZactorPoseTables, gZactorPose)

/* The player whose view is drawn (the byte read by comdataGetViewPlayer). */
uint32_t zactorViewPlayer(void);
/* Looks up all parts of a rider / bike model for `player` and marks the row ready. */
void zactorInitRiderParts(void *actor, int player);
void zactorInitBikeParts(void *actor, int player);

/* World position of `local` in the frame of model part `part` of an actor (the pose is synced first). */
void zactorGetPartPosition(void *actor, int part, const ZVec3 *local, ZVec3 *out);
/* The same without the pose sync. Prints "Unknown part" and returns zeros for a part that is not present. */
void zactorGetPartPositionNoSync(void *actor, int part, const ZVec3 *local, ZVec3 *out);
/* Length of model part `part` of an actor (a vector length, returned unrounded on the x87 stack). */
double zactorGetPartLength(void *actor, int part);
/* Cached model node of a part for `player` (0 when the part does not exist). */
void *zactorGetPartNode(void *actor, int part, int player);
/* Vector of a part (as zactorGetPartLength measures it) for `player`. */
void zactorGetPartVector(void *actor, int part, ZVec3 *out, int player);

/* Clears the part caches (0x5da1e8..). */
void zactorClearPoseTables(void);
/* Finishes a freshly created rider / bike actor: part hashes and drawing order. */
void zactorSetupRider(void *actor);
void zactorSetupBike(void *actor);
/* Syncs the pose of the current rider's actor and of its bike. */
void zactorSyncRiderActors(void);

/* Applies the pose of a rider / bike actor (limb joints) to its model and places it at the actor. */
void zactorSyncPose(void *actor);

/* Name hashes of the six rider arm parts (left up, low, hand, right up, low, hand), looked up on first use.
 */
typedef uint32_t ZactorArmHashes[6];
#define gZactorArmHashes ORIG_VAR(0x005da2c8, ZactorArmHashes, gZactorArmHashes)

#endif
