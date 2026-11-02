/*
 * Scene database (C:\NgEngine\Zge\zScene.c). Scene.Txt describes a tree of scenes, models and
 * animation libraries; opening a scene loads its component files and registers the loaded objects
 * (entries) so they can be found by name and released together.
 */
#ifndef ZGE_ZSCENE_H
#define ZGE_ZSCENE_H

#include <stdint.h>

#include "compat/orig.h"
#include "zge/zmem.h"

/* ZsceneNode.type */
enum {
    kZsceneNodeScene = 0,
    kZsceneNodeModel = 1,
    kZsceneNodeAnim = 2,
};

/* A node of the Scene.Txt tree (0x78 bytes, from a free list). */
typedef struct ZsceneNode {
    char name[0x20];            /* 0x00 */
    char file[0x40];            /* 0x20 */
    int32_t type;               /* 0x60 */
    uint32_t nameHash;          /* 0x64 zstrHash(name) */
    int32_t token68;            /* 0x68 TYPE value */
    int32_t subtype;            /* 0x6c SUBTYPE value */
    struct ZsceneNode *sibling; /* 0x70 */
    struct ZsceneNode *child;   /* 0x74 */
} ZsceneNode;

BMX_LAYOUT32_ASSERT(sizeof(ZsceneNode) == 0x78, "scene node layout");

/* ZsceneEntry.flags: byte 0 is the object type, byte 1 a sub kind. */
enum {
    kZsceneEntryModel = 1,
    kZsceneEntryAnimLib = 2,
    kZsceneEntryLight = 3,
    kZsceneEntryType4 = 4,
    kZsceneEntryWorld = 5,
    kZsceneEntryScoped = 0x10000, /* registered while a scene was open: released with it */
};

/* A registered object (0x10 bytes, from a free list). */
typedef struct ZsceneEntry {
    ZsceneNode *node; /* 0x00 */
    uint32_t id;      /* 0x04 name hash */
    uint32_t flags;   /* 0x08 */
    void *object;     /* 0x0c */
} ZsceneEntry;

BMX_LAYOUT32_ASSERT(sizeof(ZsceneEntry) == 0x10, "scene entry layout");

/* An animation of an animation library (ZsceneEntry.object of a kZsceneEntryAnimLib entry), newest first. */
typedef struct ZsceneAnim {
    uint32_t name;           /* 0x00 ziffAnimName */
    void *anim;              /* 0x04 ziffReadAnim */
    struct ZsceneAnim *next; /* 0x08 */
} ZsceneAnim;

BMX_LAYOUT32_ASSERT(sizeof(ZsceneAnim) == 0xc, "scene animation layout");

/* The globals from 0x673f70 on (cleared as one block by zsceneInitialize). */
typedef struct ZsceneState {
    int32_t initialized;   /* 0x673f70 */
    ZMemFreeList *entries; /* 0x673f74 */
    ZMemFreeList *nodes;   /* 0x673f78 */
    ZsceneNode *current;   /* 0x673f7c the last SCENE of the file */
    ZsceneNode *open;      /* 0x673f80 the scene opened by zsceneOpen */
    ZsceneNode common;     /* 0x673f84 SCENE_COMMON */
} ZsceneState;

BMX_LAYOUT32_ASSERT(sizeof(ZsceneState) == 0x8c, "scene state layout");

#define gScene ORIG_VAR(0x00673f70, ZsceneState, gScene)

/* Scene point state (0x674000..0x674014), shared with the scene point loader. */
typedef struct ZscnptsState {
    uint32_t first; /* 0x674000 */
    uint8_t pad04[4];
    uint16_t count; /* 0x674008 */
    uint8_t pad0a[2];
    uint32_t field0c;          /* 0x67400c */
    ZMemFreeList *points;      /* 0x674010 */
    struct ZrwMarker *markers; /* 0x674014 the level's markers (gZrwMarkers, zge/com/zglrw/zrwcam.h) */
} ZscnptsState;

#define gScnpts ORIG_VAR(0x00674000, ZscnptsState, gScnpts)

/* Parses a "{ NAME .. FILE .. }" block into node (recursing for MODEL / ANIM children); 1 on success. */
int zsceneParseNode(ZsceneNode *node);
/* Reads Scene.Txt (or another database): SCENE_COMMON, then SCENE blocks up to END. */
int zsceneLoadDatabase(const char *path);
/* Depth first search below node for query {type, name hash (0: first of type; set to -1), -, result}. */
int zsceneFindNode(ZsceneNode *node, int32_t *query);
/* Free list callback: entry matches query {type, id, result}; returns 0 when it does (stops the scan). */
int zsceneEntryMatch(ZsceneEntry *entry, int32_t *query);
/* Registers an object in the open scene (asserts when none is open). */
ZsceneEntry *zsceneAddEntry(ZsceneNode *node, uint32_t id, uint32_t type, uint8_t kind, void *object);
/* Loads the chunks of the scene component `kind` of a file image through a table of {chunk id, handler}. */
int zsceneLoadChunks(void *data, uint32_t size, const char *directory, int kind, int tableCount,
                     const void *table);
void zsceneReleaseEntries(int all);
int zsceneForgetEntry(int type, int kind, void *object, ZsceneEntry *entry);
int zsceneLoadModelFile(uint32_t *request);
int zsceneLoadAnimFile(uint32_t *request);
int zsceneLoadAnimLibrary(uint32_t *request);
/* Creates the databases; path NULL means "Scene.Txt". */
int zsceneInitialize(const char *path);
/* Opens the scene of that name: loads its component files. Asserts when one is already open. */
int zsceneOpen(const char *name);
int zsceneClose(void);
/* Finds (or creates) the scene object of a model name; NULL when it is not in the scene. */
void *zsceneFindModel(int flags, const char *name);
void *zsceneFindAnim(int flags, const char *name);
/* Copies the animations of a library into a table of `size` entries (padding with the first). */
int zsceneAnimTable(void *library, uint32_t *table, int size);
void *zsceneAddLight(int flags, int type, int param);
void *zsceneAddWorldObject(int flags);

/* Free list destructor of the scene nodes. */
int zsceneFreeNode(ZsceneNode *node);

/* TODO(file): the next ones probably belong to zscnpts.c (scene points and the panorama model). */
int zscnptsReset(void);
int zscnptsFreeNode(void *node);
int zscnptsInitialize(void);
void zscnptsShutdown(void);
#define gPanorama ORIG_VAR(0x0067537c, void *, gPanorama)
int zpanoramaInitialize(void);
void zpanoramaShutdown(void);
/* Chunk handler: loads the panorama model of a scene point file. */
int zpanoramaLoadChunk(void *iff);

#endif
