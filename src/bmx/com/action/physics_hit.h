/* Collision query results shared by the physics code (C:\zaxis\Bmx\src\com\action\physics.c). */
#ifndef BMX_COM_ACTION_PHYSICS_HIT_H
#define BMX_COM_ACTION_PHYSICS_HIT_H

#include <stddef.h>
#include <stdint.h>

#include "bmx/com/action/physics_types.h"
#include "compat/orig.h"
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

/* Scene frame of a prop: position at +4, rotation quaternion at +0x14 (y stored negated relative to the
 * world). */
typedef struct PhysPropFrame {
    uint8_t pad00[0x4];
    ZVec3 pos;
    uint8_t pad10[0x4];
    ZQuat rot;
    uint8_t pad24[0x8];
    void *model; /* renderer handle (flags are set through the pointer at 0x560ed8) */
} PhysPropFrame;

_Static_assert(offsetof(PhysPropFrame, rot) == 0x14, "PhysPropFrame.rot");
_Static_assert(offsetof(PhysPropFrame, model) == 0x2c, "PhysPropFrame.model");

/* Placement record of a prop in the level file (bytes 4-7 and the word at 8 feed the collision object). */
typedef struct PhysPropTag {
    uint8_t pad00[0x4];
    uint8_t flag4, flag5, flag6;
    uint8_t group;  /* low 5 bits go into the collision object's flags */
    uint16_t range; /* low 12 bits: id compared against zbspFindTaggedFaces ranges */
} PhysPropTag;

struct ZbspTreeObject;

/* Node of the prop list (head at 0x6722c4); its collision object is built on first use. */
typedef struct PhysPropNode {
    struct PhysPropNode *next;
    uint8_t pad04[0x18];
    PhysPropFrame *frame;
    const struct ZbspTreeObject *source;
    const PhysPropTag *tag;
    uint8_t pad28[0x4];
    struct PhysPropObject *object;
    void(BMX_CDECL *destroy)(struct PhysPropObject *object); /* frees the object (0x468d20) */
} PhysPropNode;

_Static_assert(offsetof(PhysPropNode, frame) == 0x1c, "PhysPropNode.frame");
_Static_assert(offsetof(PhysPropNode, destroy) == 0x30, "PhysPropNode.destroy");

/* Prop collision object (500 bytes), the contact of surface 12 (listed by zbspFindPropsNear). */
typedef struct PhysPropObject {
    uint16_t index; /* position of the source in the tree object list */
    uint16_t flags; /* low 5 bits: tag group; see kZbspObj* */
    ZAngle3 rot;
    int16_t pos[3];
    float scale[3];
    uint8_t pad01c[0x8];
    PhysEdge *edges; /* collision edges of the shape, allocated by zcollideBuildPropEdges */
    uint8_t pad028[0x1c4];
    const PhysPropInfo *prop; /* NULL when the object is not a prop */
    PhysPropNode *node;
} PhysPropObject;

_Static_assert(offsetof(PhysPropObject, pos) == 0xa, "PhysPropObject.pos");
_Static_assert(offsetof(PhysPropObject, scale) == 0x10, "PhysPropObject.scale");
_Static_assert(offsetof(PhysPropObject, edges) == 0x24, "PhysPropObject.edges");
_Static_assert(offsetof(PhysPropObject, prop) == 0x1ec, "PhysPropObject.prop");
_Static_assert(offsetof(PhysPropObject, node) == 0x1f0, "PhysPropObject.node");

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
        PhysCollObject *objectData;
        PhysEdge *edge;
    };
    uint8_t pad44[0x4];
    PhysFace faceRecord;   /* 0x48: what `face` points at after zcollideRay */
    uint32_t materialWord; /* 0xd0: what `material` points at after zcollideRay */
    uint32_t padd4;
} PhysRayHit;

_Static_assert(sizeof(PhysRayHit) == 0xd8, "PhysRayHit size");
_Static_assert(offsetof(PhysRayHit, faceRecord) == 0x48, "PhysRayHit.faceRecord");
_Static_assert(offsetof(PhysRayHit, materialWord) == 0xd0, "PhysRayHit.materialWord");
_Static_assert(offsetof(PhysRayHit, normal) == 0x1c, "PhysRayHit.normal");
_Static_assert(offsetof(PhysRayHit, face) == 0x30, "PhysRayHit.face");
_Static_assert(offsetof(PhysRayHit, material) == 0x38, "PhysRayHit.material");
_Static_assert(offsetof(PhysRayHit, objectData) == 0x40, "PhysRayHit.objectData");

#endif
