/* Generated once by tools/structs/gen_struct.py from tools/structs/physics.spec; maintained by hand since
 * docs/PORTING.md X2c (the 32-bit-only layout asserts). */
#ifndef BMX_COM_ACTION_PHYSICS_TYPES_H
#define BMX_COM_ACTION_PHYSICS_TYPES_H

#include <stddef.h>
#include <stdint.h>

#include "zge/zmath.h"

typedef struct PhysEdge {
    uint8_t side; /* bit 0 / bit 1: the solid side of the edge (see zbspEdgeSide) */
    uint8_t pad001[0x3];
    int8_t kind;
    uint8_t pad005[0x3];
    int16_t a[3];
    int16_t b[3];
    int16_t length;
    uint16_t tag;
    ZVec3 dir;
    ZVec3 normal;
    float d;
    void *prop;
} PhysEdge;

BMX_LAYOUT32_ASSERT(sizeof(PhysEdge) == 0x38, "PhysEdge size");
_Static_assert(offsetof(PhysEdge, side) == 0x0, "PhysEdge.side");
_Static_assert(offsetof(PhysEdge, kind) == 0x4, "PhysEdge.kind");
_Static_assert(offsetof(PhysEdge, a) == 0x8, "PhysEdge.a");
_Static_assert(offsetof(PhysEdge, b) == 0xe, "PhysEdge.b");
_Static_assert(offsetof(PhysEdge, length) == 0x14, "PhysEdge.length");
_Static_assert(offsetof(PhysEdge, tag) == 0x16, "PhysEdge.tag");
_Static_assert(offsetof(PhysEdge, dir) == 0x18, "PhysEdge.dir");
_Static_assert(offsetof(PhysEdge, normal) == 0x24, "PhysEdge.normal");
_Static_assert(offsetof(PhysEdge, d) == 0x30, "PhysEdge.d");
BMX_LAYOUT32_ASSERT(offsetof(PhysEdge, prop) == 0x34, "PhysEdge.prop");

typedef struct PhysFace {
    ZVec3 normal;
    float d;
    uint8_t pad010[0x4];
    uint8_t kind; /* always 0 in records built by zbspBuildFace */
    uint8_t flags;
    uint16_t tag;
    uint8_t pad018[0x4];
    void *
        vert[4]; /* vert[0..2] point at q0..q2; for triangles slot 3 holds the level face it was built from */
    struct PhysPropObject *object; /* owning prop's collision object, NULL for level geometry */
    ZVec3 p0;                      /* the three corners in world units */
    uint8_t pad03c[0x4];
    ZVec3 p1;
    uint8_t pad04c[0x4];
    ZVec3 p2;
    uint8_t pad05c[0x4];
    int16_t q0[3];
    uint8_t pad066[0x6];
    int16_t q1[3];
    uint8_t pad072[0x6];
    int16_t q2[3];
    uint8_t pad07e[0x6];
    const struct ZrwColHit *owner; /* the renderer hit the face was built from (a corner, the material) */
} PhysFace;

BMX_LAYOUT32_ASSERT(sizeof(PhysFace) == 0x88, "PhysFace size");
_Static_assert(offsetof(PhysFace, normal) == 0x0, "PhysFace.normal");
_Static_assert(offsetof(PhysFace, d) == 0xc, "PhysFace.d");
_Static_assert(offsetof(PhysFace, kind) == 0x14, "PhysFace.kind");
_Static_assert(offsetof(PhysFace, flags) == 0x15, "PhysFace.flags");
_Static_assert(offsetof(PhysFace, tag) == 0x16, "PhysFace.tag");
BMX_LAYOUT32_ASSERT(offsetof(PhysFace, vert) == 0x1c, "PhysFace.vert");
BMX_LAYOUT32_ASSERT(offsetof(PhysFace, object) == 0x2c, "PhysFace.object");
BMX_LAYOUT32_ASSERT(offsetof(PhysFace, p0) == 0x30, "PhysFace.p0");
BMX_LAYOUT32_ASSERT(offsetof(PhysFace, p1) == 0x40, "PhysFace.p1");
BMX_LAYOUT32_ASSERT(offsetof(PhysFace, p2) == 0x50, "PhysFace.p2");
BMX_LAYOUT32_ASSERT(offsetof(PhysFace, q0) == 0x60, "PhysFace.q0");
BMX_LAYOUT32_ASSERT(offsetof(PhysFace, q1) == 0x6c, "PhysFace.q1");
BMX_LAYOUT32_ASSERT(offsetof(PhysFace, q2) == 0x78, "PhysFace.q2");
BMX_LAYOUT32_ASSERT(offsetof(PhysFace, owner) == 0x84, "PhysFace.owner");

/* Prop collision object (500 bytes where pointers are 32 bits wide), the contact of surface 12 (listed by
 * zbspFindPropsNear). */
typedef struct PhysPropObject {
    uint16_t index; /* position of the source in the tree object list */
    uint16_t flags; /* low 5 bits: tag group; see kZbspObj* */
    ZAngle3 rot;
    int16_t pos[3];
    float scale[3];
    uint8_t pad01c[0x8];
    PhysEdge *edges; /* collision edges of the shape, allocated by zcollideBuildPropEdges */
    uint8_t pad028[0x1c4];
    const struct PropRecord *prop; /* NULL when the object is not a prop */
    struct PhysPropNode *node;
} PhysPropObject;

_Static_assert(offsetof(PhysPropObject, pos) == 0xa, "PhysPropObject.pos");
_Static_assert(offsetof(PhysPropObject, scale) == 0x10, "PhysPropObject.scale");
BMX_LAYOUT32_ASSERT(offsetof(PhysPropObject, edges) == 0x24, "PhysPropObject.edges");
BMX_LAYOUT32_ASSERT(offsetof(PhysPropObject, prop) == 0x1ec, "PhysPropObject.prop");
BMX_LAYOUT32_ASSERT(offsetof(PhysPropObject, node) == 0x1f0, "PhysPropObject.node");

/* State of a prop of the current level (0x84 bytes, 16 at 0x694460). */
typedef struct PropRecord {
    uint8_t state;          /* 0x00 0 unused, 1 in place, 3 collected */
    uint8_t flags;          /* 0x01 bit 1: set when the prop's object has the signalled flag */
    uint8_t index;          /* 0x02 index into the level's PropDefs */
    uint8_t player;         /* 0x03 who collected it */
    uint32_t timer;         /* 0x04 */
    PhysPropObject *object; /* 0x08 */
    ZVec3 pos;              /* 0x0c */
    uint8_t pad18[4];
    ZVec3 vel; /* 0x1c */
    uint8_t pad28[4];
    ZMatrix orient; /* 0x2c its `up` row is the prop's facing (physics.c) */
    ZAngle3 angles; /* 0x6c turn applied to the orientation by propOrient */
    uint8_t pad72[2];
    uint32_t sound;  /* 0x74 handle of the looping sound */
    uint32_t mode;   /* 0x78 sub state of the movement / whether it is shown (kind 5) */
    uint32_t paramA; /* 0x7c from PropDef.paramA */
    uint32_t paramB; /* 0x80 */
} PropRecord;

BMX_LAYOUT32_ASSERT(sizeof(PropRecord) == 0x84, "PropRecord size");

/* The contact record a rider or a snapshot keeps a copy of (physContactCopy): a face, an edge or a prop's
 * collision object, by surface kind (500 bytes where pointers are 32 bits wide). */
typedef union PhysContactData {
    PhysFace face;
    PhysEdge edge;
    PhysPropObject object;
} PhysContactData;

BMX_LAYOUT32_ASSERT(sizeof(PhysContactData) == 500, "PhysContactData size");
_Static_assert(offsetof(PhysFace, tag) == offsetof(PhysEdge, tag), "a contact's tag, face or edge");

typedef struct PhysContactSnap {
    uint8_t surface; /* PhysSurface at the time of the snapshot */
    uint8_t pad001[0x3];
    void *contact; /* points at data[] after a PhysContactSnapshotFix */
    PhysContactData data;
} PhysContactSnap;

BMX_LAYOUT32_ASSERT(sizeof(PhysContactSnap) == 0x1fc, "PhysContactSnap size");
_Static_assert(offsetof(PhysContactSnap, surface) == 0x0, "PhysContactSnap.surface");
BMX_LAYOUT32_ASSERT(offsetof(PhysContactSnap, contact) == 0x4, "PhysContactSnap.contact");
BMX_LAYOUT32_ASSERT(offsetof(PhysContactSnap, data) == 0x8, "PhysContactSnap.data");

typedef struct PhysWheel {
    ZVec3 anchor; /* attachment point in bike space */
    uint8_t pad00c[0x4];
    float radius;
    ZVec3 vel;
    uint8_t pad020[0x4];
    float speed;
    ZVec3 dir; /* rolling direction */
    uint8_t pad034[0x4];
    ZVec3 normal; /* contact normal */
    uint8_t pad044[0x4];
    ZVec3 side;
    uint8_t pad054[0x14];
    uint8_t mode; /* 0 rolling, 1 skidding, 2 airborne */
    uint8_t touch;
    int16_t angle; /* rolling angle, advanced by spin each frame */
    int32_t spin;
    float travel; /* suspension compression */
    float travelVel;
    uint8_t pad078[0x8];
    int32_t material;
} PhysWheel;

_Static_assert(sizeof(PhysWheel) == 0x84, "PhysWheel size");
_Static_assert(offsetof(PhysWheel, anchor) == 0x0, "PhysWheel.anchor");
_Static_assert(offsetof(PhysWheel, radius) == 0x10, "PhysWheel.radius");
_Static_assert(offsetof(PhysWheel, vel) == 0x14, "PhysWheel.vel");
_Static_assert(offsetof(PhysWheel, speed) == 0x24, "PhysWheel.speed");
_Static_assert(offsetof(PhysWheel, dir) == 0x28, "PhysWheel.dir");
_Static_assert(offsetof(PhysWheel, normal) == 0x38, "PhysWheel.normal");
_Static_assert(offsetof(PhysWheel, side) == 0x48, "PhysWheel.side");
_Static_assert(offsetof(PhysWheel, mode) == 0x68, "PhysWheel.mode");
_Static_assert(offsetof(PhysWheel, touch) == 0x69, "PhysWheel.touch");
_Static_assert(offsetof(PhysWheel, angle) == 0x6a, "PhysWheel.angle");
_Static_assert(offsetof(PhysWheel, spin) == 0x6c, "PhysWheel.spin");
_Static_assert(offsetof(PhysWheel, travel) == 0x70, "PhysWheel.travel");
_Static_assert(offsetof(PhysWheel, travelVel) == 0x74, "PhysWheel.travelVel");
_Static_assert(offsetof(PhysWheel, material) == 0x80, "PhysWheel.material");

typedef struct RiderTrick {
    uint8_t flags; /* bits 0..2: bonus multipliers */
    uint8_t trick; /* 0xff: none */
    uint8_t style;
    uint8_t spin;
    uint8_t tier;
    uint8_t barspin; /* barspin turns of the trick: (|field54| + 0x8000) >> 16, at most 3 */
    int16_t count;
    uint16_t ticks; /* frames since the trick was entered (game.c) */
    uint16_t points;
    float duration; /* summed per trick family for the HUD */
} RiderTrick;

_Static_assert(sizeof(RiderTrick) == 0x10, "RiderTrick size");
_Static_assert(offsetof(RiderTrick, flags) == 0x0, "RiderTrick.flags");
_Static_assert(offsetof(RiderTrick, trick) == 0x1, "RiderTrick.trick");
_Static_assert(offsetof(RiderTrick, style) == 0x2, "RiderTrick.style");
_Static_assert(offsetof(RiderTrick, spin) == 0x3, "RiderTrick.spin");
_Static_assert(offsetof(RiderTrick, tier) == 0x4, "RiderTrick.tier");
_Static_assert(offsetof(RiderTrick, barspin) == 0x5, "RiderTrick.barspin");
_Static_assert(offsetof(RiderTrick, count) == 0x6, "RiderTrick.count");
_Static_assert(offsetof(RiderTrick, ticks) == 0x8, "RiderTrick.ticks");
_Static_assert(offsetof(RiderTrick, points) == 0xa, "RiderTrick.points");
_Static_assert(offsetof(RiderTrick, duration) == 0xc, "RiderTrick.duration");

typedef struct RiderCombo {
    uint32_t score; /* run score (game.c) */
    int32_t total;
    int32_t points;
    int32_t landed;
    RiderTrick trick[8];
    uint8_t runState; /* 0 riding, 1 combo open, 2 ended, 3 landing */
    uint8_t count;
    uint8_t timer;
    uint8_t pad093[0x1];
} RiderCombo;

_Static_assert(sizeof(RiderCombo) == 0x94, "RiderCombo size");
_Static_assert(offsetof(RiderCombo, score) == 0x0, "RiderCombo.score");
_Static_assert(offsetof(RiderCombo, total) == 0x4, "RiderCombo.total");
_Static_assert(offsetof(RiderCombo, points) == 0x8, "RiderCombo.points");
_Static_assert(offsetof(RiderCombo, landed) == 0xc, "RiderCombo.landed");
_Static_assert(offsetof(RiderCombo, trick) == 0x10, "RiderCombo.trick");
_Static_assert(offsetof(RiderCombo, runState) == 0x90, "RiderCombo.runState");
_Static_assert(offsetof(RiderCombo, count) == 0x91, "RiderCombo.count");
_Static_assert(offsetof(RiderCombo, timer) == 0x92, "RiderCombo.timer");

typedef struct RiderAreaStats {
    uint16_t tricks;
    uint16_t modifiers;
    uint16_t modified; /* tricks with a modifier */
    uint16_t grinds;
    uint16_t stalls;
    uint16_t manuals;
    uint16_t falls;
} RiderAreaStats;

_Static_assert(sizeof(RiderAreaStats) == 0xe, "RiderAreaStats size");
_Static_assert(offsetof(RiderAreaStats, tricks) == 0x0, "RiderAreaStats.tricks");
_Static_assert(offsetof(RiderAreaStats, modifiers) == 0x2, "RiderAreaStats.modifiers");
_Static_assert(offsetof(RiderAreaStats, modified) == 0x4, "RiderAreaStats.modified");
_Static_assert(offsetof(RiderAreaStats, grinds) == 0x6, "RiderAreaStats.grinds");
_Static_assert(offsetof(RiderAreaStats, stalls) == 0x8, "RiderAreaStats.stalls");
_Static_assert(offsetof(RiderAreaStats, manuals) == 0xa, "RiderAreaStats.manuals");
_Static_assert(offsetof(RiderAreaStats, falls) == 0xc, "RiderAreaStats.falls");

typedef struct RiderJudgeRun {
    uint32_t score;
    uint8_t judge[5];
    uint8_t average;
    uint8_t pad00a[0x2];
} RiderJudgeRun;

_Static_assert(sizeof(RiderJudgeRun) == 0xc, "RiderJudgeRun size");
_Static_assert(offsetof(RiderJudgeRun, score) == 0x0, "RiderJudgeRun.score");
_Static_assert(offsetof(RiderJudgeRun, judge) == 0x4, "RiderJudgeRun.judge");
_Static_assert(offsetof(RiderJudgeRun, average) == 0x9, "RiderJudgeRun.average");

typedef struct PhysRider {
    uint16_t look;
    uint8_t bikeLook;
    uint8_t pad003[0x1];
    uint8_t padIndex; /* controller index */
    uint8_t dirCount; /* stick directions of the trick being entered, then dirList */
    uint8_t dirList[4];
    uint8_t pad00a[0x2];
    uint32_t inputBits; /* controller snapshot: bit flags, then the two stick axes */
    uint32_t inputBits2;
    float stick[4]; /* two sticks, x then y each, as read by the controller layer */
    float axis[4];  /* the same scaled by 1/127 after rounding to a byte */
    uint8_t pad034[0x4];
    uint32_t flags;
    uint8_t player;
    uint8_t stateLevel[4]; /* read through riderGetStateLevel (0x431dd0) */
    uint8_t pad041[0x1];
    int16_t stateTimer[4]; /* frames spent in each state level; [1] doubles as the signed input that tilts the
                              bike actor (0x42ba50) */
    uint8_t anim;
    uint8_t ikPose; /* index into the IK limb mask table at 0x55907c */
    uint8_t ikFrame;
    uint8_t soundCooldown;
    uint8_t hudByte;
    uint8_t bailReason;
    int32_t spin;
    int32_t field54;
    int32_t field58;
    RiderCombo combo;
    uint8_t pad0f0[0x1d6];
    RiderAreaStats area[16]; /* by position on a 4 x 4 grid over the level (gGameExtent) */
    RiderAreaStats statTotal;
    float lipHeight; /* best height of a lip trick in the current combo */
    float impactTotal;
    uint32_t hintMask; /* tutorial hints still to show (bit per hint, game.c) */
    uint32_t goalMask; /* goal results unlocked this run (game.c) */
    RiderJudgeRun judgeRun[2];
    uint8_t state;
    uint8_t surface;
    int16_t timer;
    ZVec3 fwd;
    uint8_t pad3ec[0x4];
    ZVec3 up;
    uint8_t pad3fc[0x4];
    ZVec3 side;
    uint8_t pad40c[0x4];
    ZVec3 omega;
    uint8_t pad41c[0x4];
    ZVec3 pos;
    uint8_t pad42c[0x4];
    ZVec3 step;
    uint8_t pad43c[0x4];
    float groundGap;
    float landQuality;
    ZVec3 vel;
    uint8_t pad454[0x4];
    ZVec3 dvel;
    uint8_t pad464[0x4];
    ZVec3 airVel;
    uint8_t pad474[0x4];
    ZAngle3 rot;
    uint8_t pad47e[0x2];
    int16_t yawTrim;
    uint8_t pad482[0x2];
    float speed;
    void *contact;
    PhysContactData contactData;
    ZVec3 contactPos;
    uint8_t pad68c[0x4];
    ZVec3 contactAxis0;
    uint8_t pad69c[0x4];
    ZVec3 contactAxis1;
    uint8_t pad6ac[0x4];
    ZVec3 contactAxis2;
    uint8_t pad6bc[0x14];
    float stat[7]; /* rider and bike tuning: [4] and [5] feed the crash and grind formulas */
    ZMatrix orientA;
    ZAngle3 orientARot;
    uint8_t pad732[0x2];
    ZMatrix orientB;
    ZAngle3 orientBRot;
    uint8_t pad77a[0x2];
    PhysContactSnap snapA;
    ZVec3 snapAPos;
    uint8_t pad984[0x4];
    PhysContactSnap snapB;
    ZVec3 snapBPos;
    uint8_t padb90[0x4];
    void *actor;
    void *actor2;
    int16_t fieldB9C;
    uint8_t padb9e[0x2];
    void *fieldBA0; /* non-NULL while a pose is queued */
    uint32_t fieldBA4;
    float animSpeed;
    uint16_t motionIndex; /* index into gPhysMotions */
    uint8_t padbae[0x2];
    void *motionSample;   /* ZMotionDelta of the last scripted-motion step */
    uint16_t motionFlags; /* the original reads and writes all 32 bits (with motionFlagsHi) */
    uint16_t motionFlagsHi;
    float motionWeight;
    float ikBlend; /* weight of the IK limb targets */
    float motionScale;
    uint8_t padbc4[0x38];
    ZVec3 fieldBFC;
    uint8_t padc08[0x4];
    int32_t bikeLookId;
    ZVec3 center;
    uint8_t padc1c[0x4];
    ZMatrix basis;
    float fieldC60;
    float fieldC64;
    float fieldC68;
    float fieldC6C;
    float fieldC70;
    float travelMin;
    float travelMax;
    float fieldC7C;
    float fieldC80;
    float fieldC84;
    PhysWheel wheel[2];
    void *bikeObj;
    void *bikeObj2;
    uint8_t ik[0x1d8]; /* IK limb targets of the rider actor (0x457790 fills, 0x456120 applies) */
} PhysRider;

BMX_LAYOUT32_ASSERT(sizeof(PhysRider) == 0xf70, "PhysRider size");
/* No pointer before `contact`: the overlay reads the record by byte offset (kOvlPlayer*, overlay.h). */
_Static_assert(offsetof(PhysRider, contact) == 0x488, "PhysRider.contact");
_Static_assert(offsetof(PhysRider, look) == 0x0, "PhysRider.look");
_Static_assert(offsetof(PhysRider, bikeLook) == 0x2, "PhysRider.bikeLook");
_Static_assert(offsetof(PhysRider, padIndex) == 0x4, "PhysRider.padIndex");
_Static_assert(offsetof(PhysRider, dirCount) == 0x5, "PhysRider.dirCount");
_Static_assert(offsetof(PhysRider, dirList) == 0x6, "PhysRider.dirList");
_Static_assert(offsetof(PhysRider, inputBits) == 0xc, "PhysRider.inputBits");
_Static_assert(offsetof(PhysRider, inputBits2) == 0x10, "PhysRider.inputBits2");
_Static_assert(offsetof(PhysRider, stick) == 0x14, "PhysRider.stick");
_Static_assert(offsetof(PhysRider, axis) == 0x24, "PhysRider.axis");
_Static_assert(offsetof(PhysRider, flags) == 0x38, "PhysRider.flags");
_Static_assert(offsetof(PhysRider, player) == 0x3c, "PhysRider.player");
_Static_assert(offsetof(PhysRider, stateLevel) == 0x3d, "PhysRider.stateLevel");
_Static_assert(offsetof(PhysRider, stateTimer) == 0x42, "PhysRider.stateTimer");
_Static_assert(offsetof(PhysRider, anim) == 0x4a, "PhysRider.anim");
_Static_assert(offsetof(PhysRider, ikPose) == 0x4b, "PhysRider.ikPose");
_Static_assert(offsetof(PhysRider, ikFrame) == 0x4c, "PhysRider.ikFrame");
_Static_assert(offsetof(PhysRider, soundCooldown) == 0x4d, "PhysRider.soundCooldown");
_Static_assert(offsetof(PhysRider, hudByte) == 0x4e, "PhysRider.hudByte");
_Static_assert(offsetof(PhysRider, bailReason) == 0x4f, "PhysRider.bailReason");
_Static_assert(offsetof(PhysRider, spin) == 0x50, "PhysRider.spin");
_Static_assert(offsetof(PhysRider, field54) == 0x54, "PhysRider.field54");
_Static_assert(offsetof(PhysRider, field58) == 0x58, "PhysRider.field58");
_Static_assert(offsetof(PhysRider, combo) == 0x5c, "PhysRider.combo");
_Static_assert(offsetof(PhysRider, area) == 0x2c6, "PhysRider.area");
_Static_assert(offsetof(PhysRider, statTotal) == 0x3a6, "PhysRider.statTotal");
_Static_assert(offsetof(PhysRider, lipHeight) == 0x3b4, "PhysRider.lipHeight");
_Static_assert(offsetof(PhysRider, impactTotal) == 0x3b8, "PhysRider.impactTotal");
_Static_assert(offsetof(PhysRider, hintMask) == 0x3bc, "PhysRider.hintMask");
_Static_assert(offsetof(PhysRider, goalMask) == 0x3c0, "PhysRider.goalMask");
_Static_assert(offsetof(PhysRider, judgeRun) == 0x3c4, "PhysRider.judgeRun");
_Static_assert(offsetof(PhysRider, state) == 0x3dc, "PhysRider.state");
_Static_assert(offsetof(PhysRider, surface) == 0x3dd, "PhysRider.surface");
_Static_assert(offsetof(PhysRider, timer) == 0x3de, "PhysRider.timer");
_Static_assert(offsetof(PhysRider, fwd) == 0x3e0, "PhysRider.fwd");
_Static_assert(offsetof(PhysRider, up) == 0x3f0, "PhysRider.up");
_Static_assert(offsetof(PhysRider, side) == 0x400, "PhysRider.side");
_Static_assert(offsetof(PhysRider, omega) == 0x410, "PhysRider.omega");
_Static_assert(offsetof(PhysRider, pos) == 0x420, "PhysRider.pos");
_Static_assert(offsetof(PhysRider, step) == 0x430, "PhysRider.step");
_Static_assert(offsetof(PhysRider, groundGap) == 0x440, "PhysRider.groundGap");
_Static_assert(offsetof(PhysRider, landQuality) == 0x444, "PhysRider.landQuality");
_Static_assert(offsetof(PhysRider, vel) == 0x448, "PhysRider.vel");
_Static_assert(offsetof(PhysRider, dvel) == 0x458, "PhysRider.dvel");
_Static_assert(offsetof(PhysRider, airVel) == 0x468, "PhysRider.airVel");
_Static_assert(offsetof(PhysRider, rot) == 0x478, "PhysRider.rot");
_Static_assert(offsetof(PhysRider, yawTrim) == 0x480, "PhysRider.yawTrim");
_Static_assert(offsetof(PhysRider, speed) == 0x484, "PhysRider.speed");
_Static_assert(offsetof(PhysRider, contact) == 0x488, "PhysRider.contact");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, contactData) == 0x48c, "PhysRider.contactData");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, contactPos) == 0x680, "PhysRider.contactPos");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, contactAxis0) == 0x690, "PhysRider.contactAxis0");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, contactAxis1) == 0x6a0, "PhysRider.contactAxis1");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, contactAxis2) == 0x6b0, "PhysRider.contactAxis2");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, stat) == 0x6d0, "PhysRider.stat");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, orientA) == 0x6ec, "PhysRider.orientA");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, orientARot) == 0x72c, "PhysRider.orientARot");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, orientB) == 0x734, "PhysRider.orientB");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, orientBRot) == 0x774, "PhysRider.orientBRot");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, snapA) == 0x77c, "PhysRider.snapA");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, snapAPos) == 0x978, "PhysRider.snapAPos");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, snapB) == 0x988, "PhysRider.snapB");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, snapBPos) == 0xb84, "PhysRider.snapBPos");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, actor) == 0xb94, "PhysRider.actor");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, actor2) == 0xb98, "PhysRider.actor2");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, fieldB9C) == 0xb9c, "PhysRider.fieldB9C");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, fieldBA0) == 0xba0, "PhysRider.fieldBA0");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, fieldBA4) == 0xba4, "PhysRider.fieldBA4");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, animSpeed) == 0xba8, "PhysRider.animSpeed");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, motionIndex) == 0xbac, "PhysRider.motionIndex");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, motionSample) == 0xbb0, "PhysRider.motionSample");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, motionFlags) == 0xbb4, "PhysRider.motionFlags");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, motionFlagsHi) == 0xbb6, "PhysRider.motionFlagsHi");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, motionWeight) == 0xbb8, "PhysRider.motionWeight");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, ikBlend) == 0xbbc, "PhysRider.ikBlend");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, motionScale) == 0xbc0, "PhysRider.motionScale");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, fieldBFC) == 0xbfc, "PhysRider.fieldBFC");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, bikeLookId) == 0xc0c, "PhysRider.bikeLookId");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, center) == 0xc10, "PhysRider.center");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, basis) == 0xc20, "PhysRider.basis");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, fieldC60) == 0xc60, "PhysRider.fieldC60");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, fieldC64) == 0xc64, "PhysRider.fieldC64");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, fieldC68) == 0xc68, "PhysRider.fieldC68");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, fieldC6C) == 0xc6c, "PhysRider.fieldC6C");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, fieldC70) == 0xc70, "PhysRider.fieldC70");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, travelMin) == 0xc74, "PhysRider.travelMin");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, travelMax) == 0xc78, "PhysRider.travelMax");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, fieldC7C) == 0xc7c, "PhysRider.fieldC7C");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, fieldC80) == 0xc80, "PhysRider.fieldC80");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, fieldC84) == 0xc84, "PhysRider.fieldC84");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, wheel) == 0xc88, "PhysRider.wheel");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, bikeObj) == 0xd90, "PhysRider.bikeObj");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, bikeObj2) == 0xd94, "PhysRider.bikeObj2");
BMX_LAYOUT32_ASSERT(offsetof(PhysRider, ik) == 0xd98, "PhysRider.ik");

typedef struct PhysGroundHit {
    uint32_t flags;
    ZVec3 pos;
    uint8_t pad010[0x4];
    ZVec3 normal;
    float d;
    void *face;
    void *material;
    void *edge;
} PhysGroundHit;

BMX_LAYOUT32_ASSERT(sizeof(PhysGroundHit) == 0x30, "PhysGroundHit size");
_Static_assert(offsetof(PhysGroundHit, flags) == 0x0, "PhysGroundHit.flags");
_Static_assert(offsetof(PhysGroundHit, pos) == 0x4, "PhysGroundHit.pos");
_Static_assert(offsetof(PhysGroundHit, normal) == 0x14, "PhysGroundHit.normal");
_Static_assert(offsetof(PhysGroundHit, d) == 0x20, "PhysGroundHit.d");
BMX_LAYOUT32_ASSERT(offsetof(PhysGroundHit, face) == 0x24, "PhysGroundHit.face");
BMX_LAYOUT32_ASSERT(offsetof(PhysGroundHit, material) == 0x28, "PhysGroundHit.material");
BMX_LAYOUT32_ASSERT(offsetof(PhysGroundHit, edge) == 0x2c, "PhysGroundHit.edge");

#endif
