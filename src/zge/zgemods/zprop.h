/*
 * Level props (C:\NgEngine\Zge\ZgeMods\zprop.c): a prop definition (PDIN chunk plus its model) and the placed
 * props (PRBL chunks) that refer to a definition by id. The definitions are the nodes of the tree object list
 * (ZbspTreeNode, list head 0x6722b8), the placed props the nodes of the prop list (PhysPropNode, 0x6722c4).
 */
#ifndef ZGE_ZGEMODS_ZPROP_H
#define ZGE_ZGEMODS_ZPROP_H

#include <stdint.h>

#include "compat/orig.h"

struct ZbspObjectDef;
struct ZbspShape;

/*
 * A prop definition's data from its id on: what the collision code calls a tree object (ZbspTreeObject;
 * placed props point at it, PhysPropNode.source).
 */
typedef struct ZbspTreeObject {
    uint32_t id;                     /* 0x00 */
    char name[0x14];                 /* 0x04 name of the model in the scene */
    void *model;                     /* 0x18 renderer model of the definition */
    uint32_t pad1c;                  /* 0x1c */
    const struct ZbspObjectDef *def; /* 0x20 OBJI block */
    uint32_t pad24;                  /* 0x24 */
    struct ZbspShape *shape;         /* 0x28 collision shape, built on first use */
    uint32_t pad2c;                  /* 0x2c */
    union {
        uint32_t params[7]; /* 0x30 the last seven words of the PDIN header */
        struct {
            float size[3]; /* 0x30 extents: width, height, depth */
            float pad3c;   /* 0x3c */
            float lift;    /* 0x40 */
            float pad44;   /* 0x44 */
            float base;    /* 0x48 */
        };
    };
} ZbspTreeObject;

/* Prop definition (0x54 bytes): a node of the tree object list (ZbspTreeNode, head at 0x6722b8). */
typedef struct ZPropDef {
    struct ZPropDef *next; /* 0x00 */
    uint32_t pad04;        /* 0x04 */
    ZbspTreeObject object; /* 0x08 */
} ZPropDef;

typedef ZPropDef ZbspTreeNode;

struct ZObj;
struct PhysPropTag;
struct PhysPropObject;
struct ZbspTreeObject;

/*
 * Placed prop (0x34 bytes, a PRBL chunk) and node of the prop list (head at 0x6722c4): zprop.c builds it, the
 * collision code walks it as PhysPropNode; its collision object is built on first use.
 */
typedef struct PhysPropNode {
    struct PhysPropNode *next;           /* 0x00 */
    uint32_t pad04;                      /* 0x04 */
    uint32_t id;                         /* 0x08 id of the definition */
    uint32_t placement[4];               /* 0x0c the PRBL header's tag words */
    struct ZObj *frame;                  /* 0x1c scene object made from the model of the definition */
    const struct ZbspTreeObject *source; /* 0x20 the definition's data (ZPropDef + 8) */
    const struct PhysPropTag *tag;       /* 0x24 OBJI block (ngapp.c reads its first word as a colour) */
    uint8_t pad28[0x4];                  /* 0x28 */
    struct PhysPropObject *object;       /* 0x2c */
    void(BMX_CDECL *destroy)(struct PhysPropObject *object); /* 0x30 frees the object (0x468d20) */
} PhysPropNode;

typedef PhysPropNode ZPropInst;

BMX_LAYOUT32_ASSERT(sizeof(ZPropDef) == 0x54, "prop definition");
BMX_LAYOUT32_ASSERT(sizeof(ZPropInst) == 0x34, "prop instance");
BMX_LAYOUT32_ASSERT(__builtin_offsetof(ZPropDef, object) == 0x08, "prop definition");
_Static_assert(__builtin_offsetof(ZbspTreeObject, name) == 0x04, "tree object");
BMX_LAYOUT32_ASSERT(__builtin_offsetof(ZbspTreeObject, def) == 0x20, "tree object");
BMX_LAYOUT32_ASSERT(__builtin_offsetof(ZbspTreeObject, shape) == 0x28, "tree object");
BMX_LAYOUT32_ASSERT(__builtin_offsetof(ZbspTreeObject, params) == 0x30, "tree object");
_Static_assert(__builtin_offsetof(ZbspTreeObject, lift) - __builtin_offsetof(ZbspTreeObject, size) == 0x10 &&
                   __builtin_offsetof(ZbspTreeObject, base) - __builtin_offsetof(ZbspTreeObject, size) ==
                       0x18,
               "tree object: size, lift and base are header words 0, 4 and 6");
BMX_LAYOUT32_ASSERT(__builtin_offsetof(ZPropInst, placement) == 0x0c, "prop instance");
BMX_LAYOUT32_ASSERT(__builtin_offsetof(ZPropInst, frame) == 0x1c, "prop instance");
BMX_LAYOUT32_ASSERT(__builtin_offsetof(ZPropInst, source) == 0x20, "prop instance");
BMX_LAYOUT32_ASSERT(__builtin_offsetof(ZPropInst, tag) == 0x24, "prop instance");
BMX_LAYOUT32_ASSERT(__builtin_offsetof(ZPropInst, object) == 0x2c, "prop instance");
BMX_LAYOUT32_ASSERT(__builtin_offsetof(ZPropInst, destroy) == 0x30, "prop instance");

/* Chunk handlers for PDIN / MODL (a definition) and PRBL (a placed prop). Each returns 0 on a malformed
 * chunk. */
int zpropLoadDef(void *iff);
int zpropLoad(void *iff);

#endif
