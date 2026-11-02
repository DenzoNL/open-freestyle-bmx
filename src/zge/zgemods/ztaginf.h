/*
 * Level tag tables (C:\NgEngine\Zge\ZgeMods\ztaginf.c): the level's triangle array (0x6c byte records) and a
 * binary tree over it whose nodes list the triangles they hold.
 */
#ifndef ZGE_ZGEMODS_ZTAGINF_H
#define ZGE_ZGEMODS_ZTAGINF_H

#include <stddef.h>
#include <stdint.h>

#include "compat/orig.h"
#include "zge/zmath.h"
#include "zge/zmem.h"

struct PhysEdge;
struct PhysFace;

/* Tag of a level edge: `id` selects the kind of edge, `kind` (non-zero) is copied into the edge record. */
typedef struct ZbspEdgeTag {
    uint16_t id;
    uint8_t kind;
    uint8_t pad03;
} ZbspEdgeTag;

/*
 * Triangle of the level mesh (0x6c bytes, the array at gZtaginfFaces). The three vertices are float points
 * padded to 16 bytes. Face and edge records (PhysFace / PhysEdge) are built from it on demand and cached
 * (ngifzbsp.c); the renderer's collision debug code walks the same triangles (ZrwRenderColTriangle).
 */
typedef struct ZbspLevelFace {
    ZVec3 v0;
    float pad0c;
    ZVec3 v1;
    float pad1c;
    ZVec3 v2;
    float pad2c;
    ZVec3 normal; /* 0x30 */
    float pad3c;
    const uint16_t *tag;           /* low 12 bits: tag id (0xfa1..0xfa4 respawn faces, ...) */
    const ZbspEdgeTag *edgeTag[3]; /* tag of the edge from vertex k to vertex k + 1 */
    const void *cornerTag[3];      /* tags of the corners (the renderer's debug drawing marks them) */
    struct PhysEdge *edge[3];      /* cached by zbspFindTaggedEdges */
    struct PhysFace *face;         /* cached by zbspFindTaggedFaces */
} ZbspLevelFace;

BMX_LAYOUT32_ASSERT(sizeof(ZbspLevelFace) == 0x6c, "ZbspLevelFace size");
_Static_assert(offsetof(ZbspLevelFace, v1) == 0x10, "ZbspLevelFace.v1");
_Static_assert(offsetof(ZbspLevelFace, tag) == 0x40, "ZbspLevelFace.tag");
BMX_LAYOUT32_ASSERT(offsetof(ZbspLevelFace, edgeTag) == 0x44, "ZbspLevelFace.edgeTag");
BMX_LAYOUT32_ASSERT(offsetof(ZbspLevelFace, edge) == 0x5c, "ZbspLevelFace.edge");
BMX_LAYOUT32_ASSERT(offsetof(ZbspLevelFace, face) == 0x68, "ZbspLevelFace.face");

/* Tree node (0x34 bytes, from a free list of 0x100 per chunk). A kind 1 node is a leaf. */
typedef struct ZtaginfNode {
    uint32_t f00;
    uint32_t f04;
    uint32_t f08;
    uint32_t f0c;
    uint8_t pad10[4];
    uint32_t f14;
    uint32_t f18;
    uint32_t f1c;
    uint8_t pad20[4];
    uint8_t kind;                 /* 0x24 */
    uint8_t pad25;                /* 0x25 */
    uint16_t faceCount;           /* 0x26 */
    struct ZtaginfNode *child[2]; /* 0x28 */
    struct ZbspLevelFace **faces; /* 0x30 array of faceCount pointers into the triangle array */
} ZtaginfNode;

BMX_LAYOUT32_ASSERT(sizeof(ZtaginfNode) == 0x34, "tag node size");

/* The level's triangle array and its length (globals 0x6728c0 / 0x6728bc). */
#define gZtaginfFaces     ORIG_VAR(0x006728c0, struct ZbspLevelFace *, gZtaginfFaces)
#define gZtaginfFaceCount ORIG_VAR(0x006728bc, uint32_t, gZtaginfFaceCount)
/* Root of the tree (0x6728c4, inline) and the block its triangle records point into (0x6728fc). */
#define gZtaginfRoot ORIG_VAR(0x006728c4, ZtaginfNode, gZtaginfRoot)
#define gZtaginfData ORIG_VAR(0x006728fc, void *, gZtaginfData)
/* Free list of tree nodes. */
#define gZtaginfNodes ORIG_VAR(0x006737b8, ZMemFreeList *, gZtaginfNodes)

/* Resets the tables and creates the node free list. Returns 1. */
int ztaginfInitialize(void);
/* Frees a node's face array and both subtrees. */
void ztaginfFreeNode(ZtaginfNode *node);
/* Frees the level tag tables. */
void ztaginfRelease(void);
struct ZbspLevelFace *ztaginfGetFaces(void);
uint32_t ztaginfGetFaceCount(void);
/* Reads a node (and recursively both subtrees) from the current 'NGAT' chunk; returns 1. */
int ztaginfReadNode(ZtaginfNode *node, void *iff);
/* Loads the tag tables from a level file (chunks 'TAGD', 'TAGI', 'TAGF', 'TAGN'). Returns 0 on a missing or
 * bad chunk. */
int ztaginfLoad(void *iff);

#endif
