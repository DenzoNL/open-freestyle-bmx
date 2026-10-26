# PhysRider and friends: layout recovered from physics.c (offsets verified against the disassembly).
@PhysContactSnap 0x1fc
0x000 uint8_t surface  /* PhysSurface at the time of the snapshot */
0x004 void_ptr contact /* points at data[] after a PhysContactSnapshotFix */
0x008 uint8_t data[500]

@PhysWheel 0x84
0x000 ZVec3 anchor      /* attachment point in bike space */
0x010 float radius
0x014 ZVec3 vel
0x024 float speed
0x028 ZVec3 dir         /* rolling direction */
0x038 ZVec3 normal      /* contact normal */
0x048 ZVec3 side
0x068 uint8_t mode      /* 0 rolling, 1 skidding, 2 airborne */
0x069 uint8_t touch
0x06a int16_t angle /* rolling angle, advanced by spin each frame */
0x06c int32_t spin
0x070 float travel      /* suspension compression */
0x074 float travelVel
0x080 int32_t material

# One scored trick of the current run (rider.c / game.c): stride 0x10 in RiderCombo.
@RiderTrick 0x10
0x00 uint8_t flags /* bits 0..2: bonus multipliers */
0x01 uint8_t trick /* 0xff: none */
0x02 uint8_t style
0x03 uint8_t spin
0x04 uint8_t tier
0x05 uint8_t barspin /* barspin turns of the trick: (|field54| + 0x8000) >> 16, at most 3 */
0x06 int16_t count
0x08 uint16_t ticks /* frames since the trick was entered (game.c) */
0x0a uint16_t points
0x0c float duration /* summed per trick family for the HUD */

# The run record at +0x5c of PhysRider: the tricks of the current combo and the run state.
@RiderCombo 0x94
0x00 uint32_t score /* run score (game.c) */
0x04 int32_t total
0x08 int32_t points
0x0c int32_t landed
0x10 RiderTrick trick[8]
0x90 uint8_t runState /* 0 riding, 1 combo open, 2 ended, 3 landing */
0x91 uint8_t count
0x92 uint8_t timer

# Run statistics of one sixteenth of the level (game.c): what the judges count.
@RiderAreaStats 0xe
0x00 uint16_t tricks
0x02 uint16_t modifiers
0x04 uint16_t modified /* tricks with a modifier */
0x06 uint16_t grinds
0x08 uint16_t stalls
0x0a uint16_t manuals
0x0c uint16_t falls

# One competition run as the judges saw it (game.c).
@RiderJudgeRun 0xc
0x00 uint32_t score
0x04 uint8_t judge[5]
0x09 uint8_t average

@PhysRider 0xf70
0x000 uint16_t look
0x002 uint8_t bikeLook
0x004 uint8_t padIndex /* controller index */
0x005 uint8_t dirCount /* stick directions of the trick being entered, then dirList */
0x006 uint8_t dirList[4]
0x00c uint32_t inputBits /* controller snapshot: bit flags, then the two stick axes */
0x010 uint32_t inputBits2
0x014 float stick[4] /* two sticks, x then y each, as read by the controller layer */
0x024 float axis[4] /* the same scaled by 1/127 after rounding to a byte */
0x038 uint32_t flags
0x03c uint8_t player
0x03d uint8_t stateLevel[4] /* read through riderGetStateLevel (0x431dd0) */
0x042 int16_t stateTimer[4] /* frames spent in each state level; [1] doubles as the signed input that tilts the bike actor (0x42ba50) */
0x04a uint8_t anim
0x04b uint8_t ikPose /* index into the IK limb mask table at 0x55907c */
0x04c uint8_t ikFrame
0x04d uint8_t soundCooldown
0x04e uint8_t hudByte
0x04f uint8_t bailReason
0x050 int32_t spin
0x054 int32_t field54
0x058 int32_t field58
0x05c RiderCombo combo
0x2c6 RiderAreaStats area[16] /* by position on a 4 x 4 grid over the level (gGameExtent) */
0x3a6 RiderAreaStats statTotal
0x3b4 float lipHeight /* best height of a lip trick in the current combo */
0x3b8 float impactTotal
0x3bc uint32_t hintMask /* tutorial hints still to show (bit per hint, game.c) */
0x3c0 uint32_t goalMask /* goal results unlocked this run (game.c) */
0x3c4 RiderJudgeRun judgeRun[2]
0x3dc uint8_t state
0x3dd uint8_t surface
0x3de int16_t timer
0x3e0 ZVec3 fwd
0x3f0 ZVec3 up
0x400 ZVec3 side
0x410 ZVec3 omega
0x420 ZVec3 pos
0x430 ZVec3 step
0x440 float groundGap
0x444 float landQuality
0x448 ZVec3 vel
0x458 ZVec3 dvel
0x468 ZVec3 airVel
0x478 ZAngle3 rot
0x480 int16_t yawTrim
0x484 float speed
0x488 void_ptr contact
0x48c uint8_t contactData[500]
0x680 ZVec3 contactPos
0x690 ZVec3 contactAxis0
0x6a0 ZVec3 contactAxis1
0x6b0 ZVec3 contactAxis2
0x6d0 float stat[7] /* rider and bike tuning: [4] and [5] feed the crash and grind formulas */
0x6ec ZMatrix orientA
0x72c ZAngle3 orientARot
0x734 ZMatrix orientB
0x774 ZAngle3 orientBRot
0x77c PhysContactSnap snapA
0x978 ZVec3 snapAPos
0x988 PhysContactSnap snapB
0xb84 ZVec3 snapBPos
0xb94 void_ptr actor
0xb98 void_ptr actor2
0xb9c int16_t fieldB9C
0xba0 void_ptr fieldBA0 /* non-NULL while a pose is queued */
0xba4 uint32_t fieldBA4
0xba8 float animSpeed
0xbac uint16_t motionIndex /* index into gPhysMotions */
0xbb0 void_ptr motionSample /* ZMotionDelta of the last scripted-motion step */
0xbb4 uint16_t motionFlags /* the original reads and writes all 32 bits (with motionFlagsHi) */
0xbb6 uint16_t motionFlagsHi
0xbb8 float motionWeight
0xbbc float ikBlend /* weight of the IK limb targets */
0xbc0 float motionScale
0xbfc ZVec3 fieldBFC
0xc0c int32_t bikeLookId
0xc10 ZVec3 center
0xc20 ZMatrix basis
0xc60 float fieldC60
0xc64 float fieldC64
0xc68 float fieldC68
0xc6c float fieldC6C
0xc70 float fieldC70
0xc74 float travelMin
0xc78 float travelMax
0xc7c float fieldC7C
0xc80 float fieldC80
0xc84 float fieldC84
0xc88 PhysWheel wheel[2]
0xd90 void_ptr bikeObj
0xd94 void_ptr bikeObj2
0xd98 uint8_t ik[0x1d8] /* IK limb targets of the rider actor (0x457790 fills, 0x456120 applies) */

# Grind edge / lip record (contact of surfaces 1, 2, 3 and 9), 0x38 bytes. kind 1 and 2 pick different
# grind sounds; a/b are the end points (world units), dir the unit vector a -> b, normal/d the plane of
# the edge; tag: top nibble surface kind, low 12 bits id; prop: owner whose +0x1ec goes to the prop module.
@PhysEdge 0x38
0x000 uint8_t side /* bit 0 / bit 1: the solid side of the edge (see zbspEdgeSide) */
0x004 int8_t kind
0x008 int16_t a[3]
0x00e int16_t b[3]
0x014 int16_t length
0x016 uint16_t tag
0x018 ZVec3 dir
0x024 ZVec3 normal
0x030 float d
0x034 void_ptr prop

# BSP face record (contact of the 0x88 byte surface kinds). flags: 1 quad, 0x10 no respawn; tag: top
# nibble surface kind; vert: int16 x, y, z each.
@PhysFace 0x88
0x000 ZVec3 normal
0x00c float d
0x014 uint8_t kind /* always 0 in records built by zbspBuildFace */
0x015 uint8_t flags
0x016 uint16_t tag
0x01c void_ptr vert[4] /* vert[0..2] point at q0..q2; for triangles slot 3 holds the level face it was built from */
0x02c int32_t object /* owning prop's collision object, 0 for level geometry */
0x030 ZVec3 p0 /* the three corners in world units */
0x040 ZVec3 p1
0x050 ZVec3 p2
0x060 int16_t q0[3]
0x06c int16_t q1[3]
0x078 int16_t q2[3]
0x084 void_ptr owner /* geometry the face belongs to: a point of the plane at +0x10, the material at +0x48 */

# Result of physProbeGround (flags 1: edge hit).
@PhysGroundHit 0x30
0x000 uint32_t flags
0x004 ZVec3 pos
0x014 ZVec3 normal
0x020 float d
0x024 void_ptr face
0x028 void_ptr material
0x02c void_ptr edge
