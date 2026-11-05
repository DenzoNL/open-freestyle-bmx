/* Collision query results shared by the physics code (C:\zaxis\Bmx\src\com\action\physics.c). */
#ifndef BMX_COM_ACTION_PHYSICS_HIT_H
#define BMX_COM_ACTION_PHYSICS_HIT_H

#include <stddef.h>
#include <stdint.h>

#include "bmx/com/action/physics_types.h"
#include "compat/orig.h"
#include "zge/zgemods/zprop.h"
#include "zge/zmath.h"
#include "zge/zobj.h"

/* Dynamic collision object (props). */

/* Scene frame of a prop: the scene object its node carries (rotation y stored negated relative to the world;
 * the model's flags are set through the pointer at 0x560ed8). Was a separate view with the pointers as
 * padding, which put `model` in the wrong place on 64-bit. */
typedef ZObj PhysPropFrame;

/* Placement record of a prop in the level file (bytes 4-7 and the word at 8 feed the collision object). */
typedef struct PhysPropTag {
    uint8_t pad00[0x4];
    uint8_t flag4, flag5, flag6;
    uint8_t group;  /* low 5 bits go into the collision object's flags */
    uint16_t range; /* low 12 bits: id compared against zbspFindTaggedFaces ranges */
} PhysPropTag;

struct ZbspTreeObject;

/* PhysPropNode, the node of the prop list, is zge/zgemods/zprop.h's placed prop. */

/* The prop record of a collision object (a PhysPropObject); NULL when the object is not a prop. */
static inline void *physObjectProp(const void *object)
{
    return (void *)((const PhysPropObject *)object)->prop;
}

/*
 * Line test / ray result (cleared to 0xd8 bytes by zcollideRay; the global copy at 0x56c128 uses the
 * first 0x44). First word: 1 = terrain face hit (the face record at +0x30 is valid), 2 = prop hit. A prop
 * hit whose placement tag carries a surface type is reported as kind 1 so that it acts as terrain (quarter
 * pipes); the object pointer is still set. Some callers test the byte, others compare the whole dword,
 * hence the union.
 * The pointer at +0x40 is always the hit prop's collision object (PhysPropObject, 500 bytes, built by
 * zbspBuildCollObject; its first word is the handle stored at +0x3c) and NULL for terrain hits. Nothing ever
 * stores an edge record there: `edge` is the same pointer, kept for the contact code that only forwards it
 * (surface 12 keeps the object itself in the rider's contact).
 */
typedef struct PhysRayHit {
    union {
        uint32_t kind;
        uint8_t flags;
    };
    int32_t id; /* terrain section the face belongs to */
    ZVec3 point;
    uint8_t pad14[0x4];
    float distance; /* 0x18: the renderer's hit distance */
    ZVec3 normal;
    float normalW;
    uint32_t onFace; /* 0x2c: 1 when the hit acts as terrain */
    PhysFace *face;
    void *propCopy;           /* 0x34: objectData again for prop hits (zcollideRay) */
    const uint32_t *material; /* bits 0-3 surface material, bits 19-20 hazard kind */
    int32_t object;           /* collision object handle */
    union {
        PhysPropObject *objectData;
        PhysEdge *edge;
    };
    uint8_t pad44[0x4];
    PhysFace faceRecord;   /* 0x48: what `face` points at after zcollideRay */
    uint32_t materialWord; /* 0xd0: what `material` points at after zcollideRay */
    uint32_t padd4;
} PhysRayHit;

BMX_LAYOUT32_ASSERT(sizeof(PhysRayHit) == 0xd8, "PhysRayHit size");
BMX_LAYOUT32_ASSERT(offsetof(PhysRayHit, faceRecord) == 0x48, "PhysRayHit.faceRecord");
BMX_LAYOUT32_ASSERT(offsetof(PhysRayHit, materialWord) == 0xd0, "PhysRayHit.materialWord");
_Static_assert(offsetof(PhysRayHit, normal) == 0x1c, "PhysRayHit.normal");
_Static_assert(offsetof(PhysRayHit, face) == 0x30, "PhysRayHit.face");
BMX_LAYOUT32_ASSERT(offsetof(PhysRayHit, material) == 0x38, "PhysRayHit.material");
BMX_LAYOUT32_ASSERT(offsetof(PhysRayHit, objectData) == 0x40, "PhysRayHit.objectData");

#endif
