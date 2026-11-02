/*
 * Rider state (C:\zaxis\Bmx\src\com\action\rider.c): the rider record is a PhysRider; this module owns
 * its creation, the layered state machine (stateLevel[0..3]) and the trick bookkeeping.
 *
 * stateLevel[0] is the mode of the rider (kRiderRoll .. kRiderBail); each mode has a step function in
 * gRiderModeStep that runs the sub states selected by stateLevel[1] and stateLevel[2]. A step returns
 * 0 when the state changed and the step should run again for the new state, non-zero when it is done.
 */
#ifndef BMX_COM_ACTION_RIDER_H
#define BMX_COM_ACTION_RIDER_H

#include <stdint.h>

#include "bmx/com/action/physics_types.h"
#include "compat/orig.h"

/* PhysRider.stateLevel[0]. */
enum {
    kRiderRoll = 0,
    kRiderAir = 1,
    kRiderGrind = 2,
    kRiderLip = 3,
    kRiderManual = 4,
    kRiderMode5 = 5,
    kRiderBail = 6,
};

typedef int(BMX_CDECL *RiderStepFn)(PhysRider *r);
typedef RiderStepFn RiderModeTable[7];
typedef RiderStepFn RiderRollTable[9];
typedef RiderStepFn RiderAirTable[3];
typedef RiderStepFn RiderSingleTable[1];

/* Step functions by mode, and by sub state for the rolling, grinding, lip and manual modes (.data). */
#define gRiderModeStep ORIG_VAR(0x005588b8, RiderModeTable, gRiderModeStep)
#define gRiderRollStep ORIG_VAR(0x005588d4, RiderRollTable, gRiderRollStep)
/* The air mode's sub states overlap the last three rolling ones: riderRollSub6..8 are air sub states 0..2. */
#define gRiderAirStep    ORIG_VAR(0x005588ec, RiderAirTable, gRiderAirStep)
#define gRiderGrindStep  ORIG_VAR(0x005588f8, RiderSingleTable, gRiderGrindStep)
#define gRiderLipStep    ORIG_VAR(0x005588fc, RiderSingleTable, gRiderLipStep)
#define gRiderManualStep ORIG_VAR(0x00558900, RiderSingleTable, gRiderManualStep)

/* Creation and reset. */
PhysRider *riderCreate(int player, int look, int bikeLook, uint32_t bikeIndex);
void riderReset(int player, int full);
void riderReadPads(void);
void riderUpdateAll(void);

/* State levels and scoring. */
uint32_t riderSetStateLevel(PhysRider *r, int level, uint32_t value);
int riderGetStateLevel(const PhysRider *r, unsigned level);
void riderAddScore(PhysRider *r, int kind, float amount);

/* The tricks being entered: stick direction latch and the lookups into the trick tables. */
void riderLatchDirection(PhysRider *r, int latch);
uint32_t riderGetTrickA(const PhysRider *r);
uint32_t riderGetTrickB(const PhysRider *r);
/* Trick class of the current move (table at 0x558fa8); 0x2a/0x2b are lip tricks. */
uint32_t riderGetTrickKind(const PhysRider *r);
uint32_t riderStickDirection(const PhysRider *r);

/* The collision callback of the rider's motion object. */
struct ZRig;
struct ZRigHit;
void riderCollide(struct ZRig *rig, uint32_t index, float impact, struct ZRigHit *hit);

/* Mode step functions. */
int riderStepRoll(PhysRider *r);
int riderStepAir(PhysRider *r);
int riderStepGrind(PhysRider *r);
int riderStepLip(PhysRider *r);
int riderStepManual(PhysRider *r);
int riderStepMode5(PhysRider *r);
int riderStepBail(PhysRider *r);

/* Sub state functions of the rolling mode, by stateLevel[1]. */
int riderRollSub0(PhysRider *r);
int riderRollSub1(PhysRider *r);
int riderRollSub2(PhysRider *r);
int riderRollSub3(PhysRider *r);
int riderRollSub4(PhysRider *r);
int riderRollSub5(PhysRider *r);
int riderRollSub6(PhysRider *r);
int riderRollSub7(PhysRider *r);
int riderRollSub8(PhysRider *r);
int riderGrindSub0(PhysRider *r);
int riderLipSub0(PhysRider *r);
int riderManualSub0(PhysRider *r);

/* Empty callbacks. */
void riderNop0(void);
void riderNop1(void);
void riderNop2(void);

#endif
