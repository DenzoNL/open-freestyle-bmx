/*
 * Airborne rider update and actor pose sync (C:\zaxis\Bmx\src\com\action\physics.c, part).
 * Kept apart from physics.h while the rest of physics.c is ported in parallel; the ORIG_FN callees
 * below belong to physics.c, rider.c and the world/collision modules.
 */
#ifndef BMX_COM_ACTION_PHYSICS_AIR_H
#define BMX_COM_ACTION_PHYSICS_AIR_H

#include <stdint.h>

#include "bmx/com/action/physics_types.h"
#include "compat/orig.h"
#include "zge/zmath.h"

/* Line vertex for the debug line renderer: position and RGB colour (the other bytes are unused). */
typedef struct PhysDebugVertex {
    ZVec3 pos;
    uint32_t pad0c;
    uint8_t rgb[3];
    uint8_t pad13;
} PhysDebugVertex;

_Static_assert(sizeof(PhysDebugVertex) == 0x14, "debug vertex size");

/* Rider states (PhysRider.state). */
enum {
    kPhysStateRoll = 0,
    kPhysStateBail = 2,
};

/* Draws collision probes as debug lines when non-zero. */
#define gPhysDebugDraw ORIG_VAR(0x0056c444, int32_t, gPhysDebugDraw)
/* Forces the actor step offset (PhysRider.step) on in every state. */
#define gPhysForceStep ORIG_VAR(0x0056c440, int32_t, gPhysForceStep)
/* Zero vector used as the middle point of zvec3Cross3. */
#define gPhysZero ORIG_VAR(0x006702d8, ZVec3, gPhysZero)
/* Tunables (.data). */
#define gPhysAirCrashSpeed  ORIG_VAR(0x0055781c, float, gPhysAirCrashSpeed)
#define gPhysLandCrashSpeed ORIG_VAR(0x00557820, float, gPhysLandCrashSpeed)
#define gPhysWheelSideSlip  ORIG_VAR(0x00557824, float, gPhysWheelSideSlip)
#define gPhysAirProbe       ORIG_VAR(0x0055783c, ZVec3, gPhysAirProbe)
#define gPhysMaxSlope       ORIG_VAR(0x0055784c, ZAngle, gPhysMaxSlope)
#define gPhysAirTurnSteps   ORIG_VAR(0x00557864, int32_t, gPhysAirTurnSteps)
#define gPhysLandUpBlend    ORIG_VAR(0x00557880, float, gPhysLandUpBlend)
#define gPhysAirUp          ORIG_VAR(0x00557888, ZVec3, gPhysAirUp)
/* Per-animation flags; the low three bits mark animations that may land on one wheel. */
typedef uint8_t RiderAnimFlags[256];
#define gRiderAnimFlags ORIG_VAR(0x00558fb4, RiderAnimFlags, gRiderAnimFlags)
/* IK limb masks indexed by PhysRider.ikPose; each bit selects one limb placement. */
typedef uint32_t PhysIkLimbMasks[256];
#define gPhysIkLimbMasks ORIG_VAR(0x0055907c, PhysIkLimbMasks, gPhysIkLimbMasks)

/* physics.c, not ported yet. */
ORIG_FN(0x00420c90, int, physGetState, (const PhysRider *r));
ORIG_FN(0x00420cb0, int, physSetState, (PhysRider * r, int state));
ORIG_FN(0x0042d050, void, physPlaceActors, (PhysRider * r));
ORIG_FN(0x0042d250, void, physMatrixToAngles,
        (const ZMatrix *ref, const ZMatrix *m, int divisor, ZAngle3 *out));
ORIG_FN(0x0042d950, void *, physFindQuarterpipeEdge, (int faceIndex, int id, const ZVec3 *pos));
ORIG_FN(0x0042dad0, int, physContactFromHit, (PhysRider * r, PhysRayHit *hit));
ORIG_FN(0x0042dda0, void, physSnapshotContact, (PhysRider * r));
/* Game object touches (0x42eec0 returns 0 when the object is not solid for this player). */
ORIG_FN(0x0042eec0, int, physObjectTouch, (int player, void *info));
ORIG_FN(0x0042ef60, void, physFaceTrigger, (int player, void *face));
/* rider.c. */
ORIG_FN(0x00431dd0, int, riderGetStateLevel, (const PhysRider *r, unsigned level));
ORIG_FN(0x00431e10, void, riderAddImpact, (PhysRider * r, int kind, float speed));
/* Controller rumble (an empty stub in this build). */
ORIG_FN(0x004086d0, void, padRumble, (int player, int a, int b));
/* Rider actor inverse kinematics. */
ORIG_FN(0x00456120, void, zactorApplyIk, (void *actor, void *ik));
ORIG_FN(0x00457790, void, zactorSetIkTarget,
        (void *ik, const ZVec3 *target, const ZVec3 *pole, uint32_t limbs));
ORIG_FN(0x00463710, void, debugDrawLine, (int kind, const PhysDebugVertex *a, const PhysDebugVertex *b));
ORIG_FN(0x00464b50, void, zactorGetPartPosition, (void *actor, int part, const ZVec3 *offset, ZVec3 *out));
/* World objects by id. */
ORIG_FN(0x004665c0, uint8_t *, worldObjectInfo, (int id));
ORIG_FN(0x00466610, void, worldObjectSignal, (void *object, int signal));
ORIG_FN(0x00469530, void *, worldObjectState, (int id));
/* Segment from -> to against the world; fills `hit` and returns non-zero on a hit. */
ORIG_FN(0x00469b00, int, worldRaycast, (const ZVec3 *from, const ZVec3 *to, PhysRayHit *hit));

void bmxphysUpdatePositionAir(PhysRider *r);
void bmxphysUpdatePose(PhysRider *r);

#endif
