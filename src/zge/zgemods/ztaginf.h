/*
 * Level tag tables (C:\NgEngine\Zge\ZgeMods\ztaginf.c): the level's triangle array (0x6c byte records) and a
 * binary tree over it whose nodes list the triangles they hold.
 */
#ifndef ZGE_ZGEMODS_ZTAGINF_H
#define ZGE_ZGEMODS_ZTAGINF_H

#include <stdint.h>

#include "compat/orig.h"
#include "zge/zmem.h"

struct ZbspLevelFace;

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
