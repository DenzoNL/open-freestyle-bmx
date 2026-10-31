/*
 * Level props (C:\NgEngine\Zge\ZgeMods\zprop.c): a prop definition (PDIN chunk plus its model) and the placed
 * props (PRBL chunks) that refer to a definition by id. The definitions are the nodes of the tree object list
 * (ZbspTreeNode, list head 0x6722b8), the placed props the nodes of the prop list (PhysPropNode, 0x6722c4).
 */
#ifndef ZGE_ZGEMODS_ZPROP_H
#define ZGE_ZGEMODS_ZPROP_H

#include <stdint.h>

#include "compat/orig.h"

/* Prop definition (0x54 bytes). */
typedef struct ZPropDef {
    struct ZPropDef *next; /* 0x00 */
    uint32_t pad04;
    uint32_t id;     /* 0x08 */
    char name[0x14]; /* 0x0c name of the model in the scene */
    void *model;     /* 0x20 renderer model of the definition */
    uint32_t pad24;
    void *extra;        /* 0x28 OBJI block */
    uint32_t pad2c[3];  /* 0x2c */
    uint32_t params[7]; /* 0x38 the last seven words of the PDIN header */
} ZPropDef;

/* Placed prop (0x34 bytes). */
typedef struct ZPropInst {
    struct ZPropInst *next; /* 0x00 */
    uint32_t pad04;
    uint32_t id;        /* 0x08 id of the definition */
    uint32_t tag[4];    /* 0x0c */
    void *object;       /* 0x1c scene object made from the model of the definition */
    const void *source; /* 0x20 the definition's data (ZPropDef + 8) */
    void *extra;        /* 0x24 OBJI block */
    uint32_t pad28[3];  /* 0x28 */
} ZPropInst;

_Static_assert(sizeof(ZPropDef) == 0x54, "prop definition");
_Static_assert(sizeof(ZPropInst) == 0x34, "prop instance");
_Static_assert(__builtin_offsetof(ZPropDef, id) == 0x08, "prop definition");
_Static_assert(__builtin_offsetof(ZPropDef, name) == 0x0c, "prop definition");
_Static_assert(__builtin_offsetof(ZPropDef, model) == 0x20, "prop definition");
_Static_assert(__builtin_offsetof(ZPropDef, extra) == 0x28, "prop definition");
_Static_assert(__builtin_offsetof(ZPropDef, params) == 0x38, "prop definition");
_Static_assert(__builtin_offsetof(ZPropInst, tag) == 0x0c, "prop instance");
_Static_assert(__builtin_offsetof(ZPropInst, object) == 0x1c, "prop instance");
_Static_assert(__builtin_offsetof(ZPropInst, source) == 0x20, "prop instance");
_Static_assert(__builtin_offsetof(ZPropInst, extra) == 0x24, "prop instance");

/* Chunk handlers for PDIN / MODL (a definition) and PRBL (a placed prop). Each returns 0 on a malformed
 * chunk. */
int zpropLoadDef(void *iff);
int zpropLoad(void *iff);

#endif
