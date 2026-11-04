/* Rider and bike physics (C:\zaxis\Bmx\src\com\action\physics.c). */
#ifndef BMX_COM_ACTION_PHYSICS_H
#define BMX_COM_ACTION_PHYSICS_H

#include <stdint.h>

#include "bmx/com/action/physics_hit.h"
#include "bmx/com/action/physics_types.h"
#include "compat/orig.h"
#include "zge/zmath.h"

/* PhysRider.state */
enum {
    kPhysStateRoll = 0,
    kPhysStateAir = 1,
    kPhysStateScripted = 2, /* position and orientation follow a canned motion (bails) */
    kPhysStateGrindIn = 3,  /* lining up with a grind edge, then kPhysStateGrind */
    kPhysStateGrind = 4,
    kPhysStateStallIn = 5, /* lining up with a lip, then kPhysStateStall */
    kPhysStateStall = 6,
    kPhysStateCount = 7,
    kPhysStateNone = 0xff,
};

/* PhysRider.flags */
enum {
    kPhysActive = 0x1,
    kPhysAirSpin = 0x800,  /* orientB/orientBRot are applied every air frame */
    kPhysAirFlip = 0x1000, /* orientARot is applied as a plain rotation */
    kPhysQuarterPipe = 0x2000,
    kPhysNoPipeAlign = 0x4000,
    kPhysLanding = 0x8000,
    kPhysGrindRequest = 0x20000, /* leaving the ground over a lip or rail tries the grind state first */
    kPhysFakie = 0x40000,        /* rolling backwards */
    kPhysGrindReverse = 0x80000,
    kPhysAirBackwards = 0x10000000, /* took off rolling backwards */
};

/* Per-state handlers: exit/enter get (rider, from, to); enter returns the state actually entered. */
typedef int(BMX_CDECL *PhysTransitionFn)(PhysRider *r, int from, int to);
typedef void(BMX_CDECL *PhysStepFn)(PhysRider *r);
typedef PhysTransitionFn PhysTransitionTable[kPhysStateCount];
typedef PhysStepFn PhysStepTable[kPhysStateCount];

#define gPhysStateExit     ORIG_VAR(0x00557898, PhysTransitionTable, gPhysStateExit)
#define gPhysStateEnter    ORIG_VAR(0x005578b4, PhysTransitionTable, gPhysStateEnter)
#define gPhysStateOrient   ORIG_VAR(0x005578d0, PhysStepTable, gPhysStateOrient)
#define gPhysStateVelocity ORIG_VAR(0x005578ec, PhysStepTable, gPhysStateVelocity)
#define gPhysStateMove     ORIG_VAR(0x00557908, PhysStepTable, gPhysStateMove)
#define gPhysStateSync     ORIG_VAR(0x00557924, PhysStepTable, gPhysStateSync)

typedef PhysRider PhysRiderArray[4];
#define gPhysRiders      ORIG_VAR(0x00691f40, PhysRiderArray, gPhysRiders)
#define gPhysPlayerCount ORIG_VAR(0x0069621c, uint8_t, gPhysPlayerCount)
/* Game option bits; 8 also drives the rider's wheel joint. */
#define gPhysSyncFlags ORIG_VAR(0x006955e2, uint8_t, gPhysSyncFlags)

/* Tunables (.data). */
#define gPhysGrindDrag        ORIG_VAR(0x005577f0, float, gPhysGrindDrag)
#define gPhysAirDrag          ORIG_VAR(0x00557800, float, gPhysAirDrag)
#define gPhysRollYawDamp      ORIG_VAR(0x00557804, float, gPhysRollYawDamp)
#define gPhysGrindYawDamp     ORIG_VAR(0x0055780c, float, gPhysGrindYawDamp)
#define gPhysGravity          ORIG_VAR(0x00557810, float, gPhysGravity)
#define gPhysAirSpeedCap      ORIG_VAR(0x0055782c, float, gPhysAirSpeedCap)
#define gPhysPipeLaunchScale  ORIG_VAR(0x00557834, float, gPhysPipeLaunchScale)
#define gPhysRollSpinSpeed    ORIG_VAR(0x00557828, float, gPhysRollSpinSpeed)
#define gPhysGrindMinSpeed    ORIG_VAR(0x00557838, float, gPhysGrindMinSpeed)
#define gPhysRailAngleLimit   ORIG_VAR(0x0055784e, int16_t, gPhysRailAngleLimit)
#define gPhysPipeAngleLimit   ORIG_VAR(0x00557856, int16_t, gPhysPipeAngleLimit)
#define gPhysPipeTiltLimit    ORIG_VAR(0x00557858, int16_t, gPhysPipeTiltLimit)
#define gPhysGrindInSteps     ORIG_VAR(0x0055785c, uint32_t, gPhysGrindInSteps)
#define gPhysStallInSteps     ORIG_VAR(0x00557860, uint32_t, gPhysStallInSteps)
#define gPhysAirAlignFrames   ORIG_VAR(0x00557864, int32_t, gPhysAirAlignFrames)
#define gPhysGrindInRadius    ORIG_VAR(0x00557868, float, gPhysGrindInRadius)
#define gPhysEdgeSearchRadius ORIG_VAR(0x00557870, float, gPhysEdgeSearchRadius)
#define gPhysGrindBalance     ORIG_VAR(0x00557874, float, gPhysGrindBalance)
#define gPhysRollSteer        ORIG_VAR(0x0055787c, float, gPhysRollSteer)
#define gPhysAlignRate        ORIG_VAR(0x00557880, float, gPhysAlignRate)
#define gPhysDown             ORIG_VAR(0x00557888, ZVec3, gPhysDown)

/* Debug and HUD state. */
#define gPhysHudHandle    ORIG_VAR(0x0056be94, uint16_t, gPhysHudHandle)
#define gPhysHudFont      ORIG_VAR(0x0056bf38, struct OvlFont, gPhysHudFont)
#define gPhysTick         ORIG_VAR(0x0056c07c, uint32_t, gPhysTick)
#define gPhysDebugContact ORIG_VAR(0x0056c444, int32_t, gPhysDebugContact)
#define gPhysDebugEdges   ORIG_VAR(0x0056c448, PhysEdge **, gPhysDebugEdges)
#define gPhysDebugFrame   ORIG_VAR(0x0056c44c, uint32_t, gPhysDebugFrame)
#define gPhysDebugTexture ORIG_VAR(0x006951e8, uint16_t, gPhysDebugTexture)

/* Static results. */
#define gPhysRay       ORIG_VAR(0x0056c128, PhysRayHit, gPhysRay)
#define gPhysBestEdge  ORIG_VAR(0x0056c200, PhysEdge, gPhysBestEdge)
#define gPhysNearEdge  ORIG_VAR(0x0056c2c0, PhysEdge, gPhysNearEdge)
#define gPhysEdgeProbe ORIG_VAR(0x0056c2f8, PhysEdge, gPhysEdgeProbe)
#define gPhysGrindEdge ORIG_VAR(0x0056c330, PhysEdge, gPhysGrindEdge) /* nearest edge on the flight path */
#define gPhysAlignEdge ORIG_VAR(0x0056c238, PhysEdge, gPhysAlignEdge) /* edge best aligned with the bike */
#define gPhysLipFace   ORIG_VAR(0x0056c3b8, PhysFace, gPhysLipFace)

/* Canned motions of the scripted state, indexed by PhysRider.motionIndex. */
typedef const void *PhysMotionTable[1];
#define gPhysMotions ORIG_VAR(0x006962fc, PhysMotionTable, gPhysMotions)

void physHudTextShow(int hide);
void physHudTextSet(int on);
void physTick(void);
void physRegisterDebugDraw(void);
void physNop(void);
void physUpdateLimbTargets(PhysRider *r);
void physUpdateRiders(void);
void physRespawn(PhysRider *r);
PhysFace *physFindRespawnFace(const ZVec3 *pos, uint32_t flags);
int physDebugDraw(void);
uint8_t physGetState(const PhysRider *r);
uint8_t physSetState(PhysRider *r, int state);
void physAddVelocity(PhysRider *r, const ZVec3 *dv);
int physProbeGround(const ZVec3 *p, PhysGroundHit *out);
int physExitGrind(PhysRider *r, int from, int to);
int physExitAir(PhysRider *r, int from, int to);
int physExitScripted(PhysRider *r, int from, int to);
int physEnterRoll(PhysRider *r, int from, int to);
int physEnterGrindIn(PhysRider *r, int from, int to);
int physEnterAir(PhysRider *r, int from, int to);
int physEnterGrind(PhysRider *r, int from, int to);
int physEnterStall(PhysRider *r, int from, int to);
int physEnterScripted(PhysRider *r, int from, int to);
void physOrientRoll(PhysRider *r);
void physOrientAir(PhysRider *r);
void physOrientGrind(PhysRider *r);
void physOrientToContact(PhysRider *r);
void physVelocityRoll(PhysRider *r);
void physVelocityAir(PhysRider *r);
void physVelocityGrind(PhysRider *r);
void physVelocityGrindIn(PhysRider *r);
void physMoveGrindIn(PhysRider *r);
void physMoveGrind(PhysRider *r);
PhysEdge *physFindNearestEdge(PhysRider *r, const ZVec3 *p, float radius, ZVec3 *point);
int physEdgeEqual(const PhysEdge *a, const PhysEdge *b);
void physMoveScripted(PhysRider *r);
void physSyncRoll(PhysRider *r);
void physSyncGrind(PhysRider *r);
void physSnapToEdge(PhysRider *r, const PhysEdge *e);
void physClosestPointOnEdge(const ZVec3 *p, const PhysEdge *e, ZVec3 *out);
void physPoseActorsRoll(PhysRider *r);
void physPoseActorsGrind(PhysRider *r);
void physRelativeAngles(const ZMatrix *a, const ZMatrix *b, int div, ZAngle3 *out);
void physAlignToQuarterPipe(PhysRider *r, const PhysEdge *e, int force);
double physPlaneDistance(const ZVec3 *p, const float *plane);
void physClosestPointOnSegment(const int16_t *a, const int16_t *b, const int16_t *p, ZVec3 *out);
PhysEdge *physFindNearestTaggedEdge(int unused, int id, const ZVec3 *p);
int physTakeContact(PhysRider *r, const PhysRayHit *hit);
void physContactSnapshotFix(PhysContactSnap *s);
void physContactCopy(PhysRider *r);

#endif
