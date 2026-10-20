/* Collision query results shared by the physics code (C:\zaxis\Bmx\src\com\action\physics.c). */
#ifndef BMX_COM_ACTION_PHYSICS_HIT_H
#define BMX_COM_ACTION_PHYSICS_HIT_H

#include <stddef.h>
#include <stdint.h>

#include "zge/zmath.h"

/* Terrain face record: the high nibble of `info` is the face type, the low 12 bits a type parameter. */
typedef struct PhysFace {
    uint8_t pad00[0x16];
    uint16_t info;
} PhysFace;

_Static_assert(sizeof(PhysFace) == 0x18, "PhysFace size");

/* Dynamic collision object (props). */
typedef struct PhysCollObject {
    uint8_t pad000[0x1ec];
    int32_t propId; /* 0 when the object is not a prop */
} PhysCollObject;

_Static_assert(sizeof(PhysCollObject) == 0x1f0, "PhysCollObject size");

/*
 * Line test result (0x44 bytes, global at 0x56c128). Bit 0 of the first word: terrain face hit, bit 1:
 * object hit. Some callers test the byte, others compare the whole dword, hence the union.
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
    PhysCollObject *objectData;
} PhysRayHit;

_Static_assert(sizeof(PhysRayHit) == 0x44, "PhysRayHit size");
_Static_assert(offsetof(PhysRayHit, normal) == 0x1c, "PhysRayHit.normal");
_Static_assert(offsetof(PhysRayHit, face) == 0x30, "PhysRayHit.face");
_Static_assert(offsetof(PhysRayHit, objectData) == 0x40, "PhysRayHit.objectData");

#endif
