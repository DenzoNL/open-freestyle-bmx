/*
 * Renderer model wrappers and materials (C:\NgEngine\Zge\Com\ZglRw\zrwmat.c). The first half wraps the model
 * table of the renderer: a scene object keeps its model at +0x2c and every call goes through a function
 * pointer in the exe's data. The second half is the material system: records of 0x74 bytes that hold up to
 * six textures and an RpMaterial, read from MATI / MADF chunks.
 */
#ifndef ZGE_COM_ZGLRW_ZRWMAT_H
#define ZGE_COM_ZGLRW_ZRWMAT_H

#include <stddef.h>
#include <stdint.h>

#include "compat/orig.h"
#include "zge/zmem.h"

/* One of the texture slots of a material: the texture handle and two words nothing uses. */
typedef struct ZrwMatSlot {
    void *texture; /* 0x00 */
    uint32_t pad04[2];
} ZrwMatSlot;

/* Material record (0x74 bytes, from the free list in gZrwMat.pool). The last of the six texture slots (+0x68)
 * ends in the word of `rwMaterial` at +0x70, so only its handle is usable. */
typedef struct ZrwMaterial {
    uint32_t flags; /* 0x00 low two bits set while in use */
    uint32_t pad04;
    uint32_t ambient[3];   /* 0x08 from the MATI chunk (MADF +0x0c), copied as raw words */
    uint32_t color[3];     /* 0x14 from MADF +0x40 */
    uint32_t pad20;        /* 0x20 cleared when read */
    uint16_t textureCount; /* 0x24 */
    uint16_t refCount;     /* 0x26 */
    uint8_t pad28[4];
    union {
        ZrwMatSlot slots[6]; /* 0x2c */
        struct {
            uint8_t pad2c[0x44];
            void *rwMaterial; /* 0x70 RpMaterial that follows the first texture */
        };
    };
} ZrwMaterial;

BMX_LAYOUT32_ASSERT(sizeof(ZrwMatSlot) == 12, "zrw material slot");
_Static_assert(offsetof(ZrwMaterial, textureCount) == 0x24, "zrw material");
_Static_assert(offsetof(ZrwMaterial, refCount) == 0x26, "zrw material");
BMX_LAYOUT32_ASSERT(offsetof(ZrwMaterial, slots) == 0x2c, "zrw material");

BMX_LAYOUT32_ASSERT(sizeof(ZrwMaterial) == 0x74, "zrw material");
BMX_LAYOUT32_ASSERT(offsetof(ZrwMaterial, rwMaterial) == 0x70, "zrw material");

/* Material definition chunk (MADF) as the loader keeps it: the chunk body starts at +8. */
typedef struct ZrwMadf {
    uint8_t pad00[8];
    uint32_t magic; /* 0x08 0x48 */
    uint32_t flags; /* 0x0c bit 0: textures are loaded with the definition */
    uint32_t v10;   /* 0x10 */
    uint32_t v14;   /* 0x14 */
    uint8_t pad18[0x28];
    uint32_t v40;     /* 0x40 */
    uint32_t v44;     /* 0x44 */
    uint32_t v48;     /* 0x48 */
    uint8_t pad4c[4]; /* 0x4c */
    char name[0x48];  /* 0x50 texture name */
} ZrwMadf;

_Static_assert(sizeof(ZrwMadf) == 0x98, "zrw madf");

/* State of a search through the material pool (zrwmatFindByTextureHash), handed to zrwmatMatchHash. */
typedef struct ZrwMatSearch {
    uint32_t hash;      /* 0x00 texture hash to find */
    int skip;           /* 0x04 matches to skip */
    ZrwMaterial *found; /* 0x08 */
} ZrwMatSearch;

/* State of the material system (the first 0x24 bytes are cleared at initialisation). */
typedef struct ZrwMatState {
    int initialized;    /* 0x00 */
    uint32_t pad04;     /* 0x04 */
    ZMemFreeList *pool; /* 0x08 material records */
    ZrwMadf *madf;      /* 0x0c the definition chunk last read (NULL when none) */
    ZrwMaterial *last;  /* 0x10 the material created last */
    void *hook;         /* 0x14 called with (material, arg) for each material read */
    ZrwMatSearch find;  /* 0x18 search state of zrwmatFindByTextureHash */
} ZrwMatState;

_Static_assert(offsetof(ZrwMatState, pool) == 0x08, "zrw material state");
BMX_LAYOUT32_ASSERT(offsetof(ZrwMatState, madf) == 0x0c, "zrw material state");
BMX_LAYOUT32_ASSERT(offsetof(ZrwMatState, last) == 0x10, "zrw material state");
BMX_LAYOUT32_ASSERT(offsetof(ZrwMatState, hook) == 0x14, "zrw material state");
BMX_LAYOUT32_ASSERT(offsetof(ZrwMatState, find) == 0x18, "zrw material state");

BMX_LAYOUT32_ASSERT(sizeof(ZrwMatState) == 0x24, "zrw material state");

#define gZrwMat     ORIG_VAR(0x006723a8, ZrwMatState, gZrwMat)
#define gZrwMadfRec ORIG_VAR(0x00672318, ZrwMadf, gZrwMadfRec)

/* Wrappers over the renderer's model table: load from a chunk, release, and look up. */
void *zrwmatCreateObject(void *model);
void *zrwmatLoadModel(void *source);
void *zrwmatLoadChunk(void *iff);
void zrwmatReleaseModel(void *model);
void *zrwmatModelFromObject(void *model);
/* Wrappers over the renderer's clump functions (they pass the clump at +0x2c of the object on). */
int zrwmatNodeCount(void *object);
void zrwmatGetNodeHashes(void *object, uint32_t *hashes);
/* Node of the object with the given name hash (0 when there is none). */
uint32_t zrwmatFindNode(void *object, uint32_t hash);
/* World position of a point given in the frame of a node (the flag selects the pose used). */
void zrwmatNodeToWorld(void *object, uint32_t node, const float *local, float *out, int flag);
/* Vector stored in a node's frame. */
void zrwmatNodeVector(void *object, uint32_t node, float *out, int flag);
/* Stores the skeleton in the object's word at +0x30 and hands it to the renderer. */
void zrwmatSetSkeleton(void *object, void *skeleton);
/* Matrix of a node of the object (the parent frame of an attached scene object). */
void zrwmatNodeMatrix(void *object, uint32_t node, void *out, int flag);
/* Refreshes the world matrices of an object. */
void zrwmatUpdate(void *object);

/* Material system. */
int zrwmatInitialize(void);
int zrwmatShutdown(void);
/* IFF chunk readers registered by zrwmatInitialize: "MATI" creates a material through the loader, "MADF"
 * keeps the definition for it. */
int zrwmatReadMATI(void *iff, void *arg);
int zrwmatReadMADF(void *iff);
/* A blank material (flags 3, one reference). */
ZrwMaterial *zrwmatCreateRecord(void);
/* Free list destroy callback: drops the textures, and frees the record at reference count 0. */
void zrwmatReleaseTexture(ZrwMaterial *material);
/* Replaces texture `index` (asserts when no texture was added for it). */
void zrwmatReplaceTexture(ZrwMaterial *material, void *texture, uint32_t index);
/* Appends a texture (at most six). */
void zrwmatAddTexture(ZrwMaterial *material, void *texture);
/* Colour of the material's RpMaterial (asserts when there is none); the middle argument is unused. */
void zrwmatSetColor(ZrwMaterial *material, void *unused, const void *color);
/* Sets the material callback of the renderer (receives each material read by a model); returns the old one.
 */
void *zrwmatSetMaterialHook(void *hook);
/* Creates a material around a texture loaded from `size` bytes of image data under the given name. */
ZrwMaterial *zrwmatLoadSpriteTexture(void *data, uint32_t size, const char *name, int type);
/* zmemFreeListForEach callback: 0 once the (skip + 1)th material whose first texture has the hash is found
 * (stored in the search). */
int zrwmatMatchHash(ZrwMaterial *material, uint32_t *state);
/* The material with the given first texture hash, skipping `skip` matches; NULL when there is none. */
ZrwMaterial *zrwmatFindByTextureHash(uint32_t hash, int skip);
/* Texture `index` of a material. */
void *zrwmatGetTexture(ZrwMaterial *material, uint32_t index);
/* A copy of a material with a reference on each of its textures. */
ZrwMaterial *zrwmatClone(ZrwMaterial *material);
/* Takes a reference on a material and on each of its textures. */
ZrwMaterial *zrwmatRetainMaterial(ZrwMaterial *material);
/* The colour triple at +0x14. */
uint32_t *zrwmatGetColor(ZrwMaterial *material);
/* Raster of texture `index` (asserts when out of range). */
void *zrwmatGetRaster(ZrwMaterial *material, uint32_t index);
void zrwmatSetTexture(ZrwMaterial *material, void *rwMaterial);
/* Flag bit 0 of the MADF chunk (asserts when none was read). */
int zrwmatTextureLoadBegin(void);
/* Wraps a loaded texture into a new material and makes it the last one created. */
void zrwmatTextureLoaded(void *texture);

#endif
