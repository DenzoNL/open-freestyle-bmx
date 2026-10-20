/* Collision query results shared by the physics code (C:\zaxis\Bmx\src\com\action\physics.c). */
#ifndef BMX_COM_ACTION_PHYSICS_HIT_H
#define BMX_COM_ACTION_PHYSICS_HIT_H

#include <stddef.h>
#include <stdint.h>

#include "bmx/com/action/physics_types.h"
#include "zge/zmath.h"

/* Dynamic collision object (props). */
typedef struct PhysCollObject {
    uint8_t pad000[0x1ec];
    int32_t propId; /* 0 when the object is not a prop */
} PhysCollObject;

_Static_assert(sizeof(PhysCollObject) == 0x1f0, "PhysCollObject size");

/* Prop record behind a collision object: facing direction at +0x3c. */
typedef struct PhysPropInfo {
    uint8_t pad00[0x3c];
    ZVec3 facing;
} PhysPropInfo;

/* Scene frame of a prop: rotation quaternion at +0x14 (y stored negated relative to the world). */
typedef struct PhysPropFrame {
    uint8_t pad00[0x14];
    ZQuat rot;
} PhysPropFrame;

typedef struct PhysPropNode {
    uint8_t pad00[0x1c];
    const PhysPropFrame *frame;
} PhysPropNode;

/* Prop collision object, the contact of surface 12 (listed by zbspFindPropsNear). */
typedef struct PhysPropObject {
    uint8_t pad000[0x4];
    ZAngle3 rot;
    int16_t pos[3];
    uint8_t pad010[0x1dc];
    const PhysPropInfo *prop; /* NULL when the object is not a prop */
    const PhysPropNode *node;
} PhysPropObject;

_Static_assert(offsetof(PhysPropObject, pos) == 0xa, "PhysPropObject.pos");
_Static_assert(offsetof(PhysPropObject, prop) == 0x1ec, "PhysPropObject.prop");
_Static_assert(offsetof(PhysPropObject, node) == 0x1f0, "PhysPropObject.node");

/*
 * Line test / ray result (cleared to 0xd8 bytes by zcollideRay; the global copy at 0x56c128 uses the
 * first 0x44). Bit 0 of the first word: terrain face hit, bit 1: object/edge hit. Some callers test the
 * byte, others compare the whole dword, hence the union.
 * TODO: the pointer at 0x40 is read as a collision object by the roll/air updates and as a grind edge by
 * the contact code; find out whether flag 2 means both.
 */
typedef struct PhysRayHit {
    union {
        uint32_t kind;
        uint8_t flags;
    };
    int32_t id; /* terrain section the face belongs to */
    ZVec3 point;
    uint8_t pad14[0x8];
    ZVec3 normal;
    float normalW;
    uint8_t pad2c[0x4];
    PhysFace *face;
    uint8_t pad34[0x4];
    const uint32_t *material; /* bits 0-3 surface material, bits 19-20 hazard kind */
    int32_t object;           /* collision object handle */
    union {
        PhysCollObject *objectData;
        PhysEdge *edge;
    };
    uint8_t pad44[0x94];
} PhysRayHit;

_Static_assert(sizeof(PhysRayHit) == 0xd8, "PhysRayHit size");
_Static_assert(offsetof(PhysRayHit, normal) == 0x1c, "PhysRayHit.normal");
_Static_assert(offsetof(PhysRayHit, face) == 0x30, "PhysRayHit.face");
_Static_assert(offsetof(PhysRayHit, material) == 0x38, "PhysRayHit.material");
_Static_assert(offsetof(PhysRayHit, objectData) == 0x40, "PhysRayHit.objectData");

#endif
