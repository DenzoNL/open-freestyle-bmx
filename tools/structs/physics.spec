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
0xbac uint16_t motionIndex /* index into gPhysMotions */
0xbb0 void_ptr motionSample /* ZMotionDelta of the last scripted-motion step */
0xbb4 uint16_t motionFlags
0xbbc float ikBlend /* weight of the IK limb targets */
0xbc0 float motionScale
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

# Result of physProbeGround (flags 1: edge hit).
@PhysGroundHit 0x30
0x000 uint32_t flags
0x004 ZVec3 pos
0x014 ZVec3 normal
0x020 float d
0x024 void_ptr face
0x028 void_ptr material
0x02c void_ptr edge
