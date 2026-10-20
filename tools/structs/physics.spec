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

@PhysRider 0xf70
0x038 uint32_t flags
0x03c uint8_t player
0x03d uint8_t stateLevel[4] /* read through riderGetStateLevel (0x431dd0) */
0x044 int16_t field44 /* signed input that tilts the bike actor (0x42ba50) */
0x04a uint8_t anim
0x04b uint8_t ikPose /* index into the IK limb mask table at 0x55907c */
0x04f uint8_t bailReason
0x058 int32_t field58
0x0ed uint8_t fieldED
0x3b8 float impactTotal
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
0x6e0 float field6E0
0x6e4 float field6E4
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
0xba4 uint32_t fieldBA4
0xbbc float ikBlend /* weight of the IK limb targets */
0xbfc ZVec3 fieldBFC
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
