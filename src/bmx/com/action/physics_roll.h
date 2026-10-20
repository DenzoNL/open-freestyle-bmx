/*
 * Rolling (ground contact) position update of C:\zaxis\Bmx\src\com\action\physics.c, kept in its own
 * file while the rest of physics.c is ported in parallel. Callees that are not ported yet are declared
 * here; most belong to other modules (see the TODO(file) notes) and move there when those are ported.
 */
#ifndef BMX_COM_ACTION_PHYSICS_ROLL_H
#define BMX_COM_ACTION_PHYSICS_ROLL_H

#include <stdint.h>

#include "bmx/com/action/physics.h"
#include "bmx/com/action/physics_hit.h"
#include "bmx/com/action/physics_types.h"
#include "compat/orig.h"
#include "zge/zgemods/zobjcamera.h"
#include "zge/zmath.h"

/* Vertex of a debug line: position and an RGB colour; the other bytes are left unset. */
typedef struct PhysDebugVertex {
    ZVec3 pos;
    uint32_t unused;
    uint8_t rgb[3];
    uint8_t unusedA;
} PhysDebugVertex;

_Static_assert(sizeof(PhysDebugVertex) == 0x14, "PhysDebugVertex size");

/* TODO(file): rider.c. */
ORIG_FN(0x00431dd0, int, riderGetStateLevel, (PhysRider * r, unsigned level));
ORIG_FN(0x00431e10, void, riderAddScore, (PhysRider * r, int kind, float amount));

/* TODO(file): prop.c. */
ORIG_FN(0x0042eec0, int, propTouch, (int player, int propId));
ORIG_FN(0x0042ef60, void, propHitFace, (int player, PhysFace *face));

/* TODO(file): fx.c and contrler.c. */
ORIG_FN(0x004089a0, void, fxSetRider, (PhysRider * r));
ORIG_FN(0x00408c90, void, fxSparks, (const ZVec3 *pos, const ZVec3 *vel));
ORIG_FN(0x00409250, void, fxSkidMark, (const ZVec3 *from, const ZVec3 *to, int flags, const ZVec3 *normal));
ORIG_FN(0x004086d0, void, contrlerRumble, (int player, int a, int strength));

/* TODO(file): zge/ngif (ngifzcollide.c, ngifzbsp.c, ngifpsx.c). */
ORIG_FN(0x00469b00, int, ngifzCollideLine, (const ZVec3 *from, const ZVec3 *to, PhysRayHit *hit));
ORIG_FN(0x004665c0, const uint8_t *, ngifzBspGetInfo, (int object));
ORIG_FN(0x00469530, int, ngifzCollideObjectValid, (int object));
ORIG_FN(0x00466610, void, ngifzBspObjectHit, (PhysCollObject * object, int a));
ORIG_FN(0x00463710, void, ngifDrawLine3D, (int mode, const PhysDebugVertex *a, const PhysDebugVertex *b));

/* TODO(file): zge/zgemods/zobjcamera.h (vector helpers). */
ORIG_FN(0x00481f80, uint32_t, zmathRand, (void));

/* Tunables (.data). */
typedef float PhysFrictionTable[5]; /* indexed 0, 2 and 4 by surface class */
#define gPhysDebugDraw        ORIG_VAR(0x0056c444, int32_t, gPhysDebugDraw)
#define gPhysRollFriction     ORIG_VAR(0x005577ec, PhysFrictionTable, gPhysRollFriction)
#define gPhysRestSpeed        ORIG_VAR(0x00557814, float, gPhysRestSpeed)
#define gPhysLeanSpeedMax     ORIG_VAR(0x00557818, float, gPhysLeanSpeedMax)
#define gPhysBailImpact       ORIG_VAR(0x0055781c, float, gPhysBailImpact)
#define gPhysLeanSpeedMin     ORIG_VAR(0x00557828, float, gPhysLeanSpeedMin)
#define gPhysProbeOffset      ORIG_VAR(0x00557844, float, gPhysProbeOffset)
#define gPhysWallAngle        ORIG_VAR(0x00557848, int16_t, gPhysWallAngle)
#define gPhysSlopeAngle       ORIG_VAR(0x0055784a, int16_t, gPhysSlopeAngle)
#define gPhysSuspGravityScale ORIG_VAR(0x00557878, float, gPhysSuspGravityScale)
#define gPhysUpBlend          ORIG_VAR(0x00557880, float, gPhysUpBlend)
#define gPhysLeanMax          ORIG_VAR(0x00557884, float, gPhysLeanMax)
#define gPhysLeanRange        ORIG_VAR(0x00558890, float, gPhysLeanRange)

void bmxphysUpdatePositionRoll(PhysRider *r);

#endif
