/*
 * Renderer world (C:\NgEngine\Zge\Com\ZglRw\zrwWorld.c): the level geometry (an RpWorld read from a .rws
 * stream next to the scene file), the effect object that hosts particle effects, and the statistics logged
 * when a world is loaded. The world and particle callbacks of zrwtxr.c (0x497200..0x497780) belong here too.
 */
#ifndef ZGE_COM_ZGLRW_ZRWWORLD_H
#define ZGE_COM_ZGLRW_ZRWWORLD_H

#include <stdint.h>

#include "compat/orig.h"

/* World interface, function pointers in the exe's data (0x5620e0..). */
typedef int(BMX_CDECL *ZrwWorldLoadFn)(void *data, const char *directory);
typedef void(BMX_CDECL *ZrwWorldCloseFn)(void);
/* A scene component of the file image: returns a record {?, hasFile, name...} and its size, or NULL. */
typedef uint8_t *(BMX_CDECL *ZrwWorldFindFn)(void *data, int kind, uint32_t *size);
#define gZrwWorldLoad  ORIG_VAR(0x005620e0, ZrwWorldLoadFn, gZrwWorldLoad)
#define gZrwWorldClose ORIG_VAR(0x005620e4, ZrwWorldCloseFn, gZrwWorldClose)
#define gZrwWorldFind  ORIG_VAR(0x005620e8, ZrwWorldFindFn, gZrwWorldFind)

/* Scene info record (SCIF chunk, 0x28 bytes) of the file image, kept in the exe after a lookup. */
typedef struct ZrwWorldRecord {
    uint32_t kind;   /* 0x00 low byte: component kind (1 = materials, 4 = world stream) */
    uint32_t size;   /* 0x04 */
    char name[0x20]; /* 0x08 file name stem */
} ZrwWorldRecord;

_Static_assert(sizeof(ZrwWorldRecord) == 0x28, "world record");

/* Counters of the statistics log (zeroed together, the polygon and vertex counts are summed by callbacks). */
typedef struct ZrwWorldStats {
    uint32_t polygons;      /* 0x00 static geometry */
    uint32_t vertices;      /* 0x04 */
    uint32_t clumpPolygons; /* 0x08 props */
    uint32_t clumpVertices; /* 0x0c */
    uint32_t lightIndex;    /* 0x10 number of the light being logged */
    uint32_t pad14[2];
} ZrwWorldStats;

_Static_assert(sizeof(ZrwWorldStats) == 0x1c, "world stats");

#define gZrwWorldRecord ORIG_VAR(0x006741b8, ZrwWorldRecord, gZrwWorldRecord)
#define gZrwWorldReady  ORIG_VAR(0x006741e0, int, gZrwWorldReady)
#define gZrwWorldStats  ORIG_VAR(0x006741ec, ZrwWorldStats, gZrwWorldStats)

/* An effect type (particle system): creates the per-object record and applies it. */
typedef struct ZrwEffectType {
    uint8_t pad00[0x10];
    void *(BMX_CDECL *create)(struct ZrwEffectType *type, int size); /* 0x10 */
    uint32_t pad14;
    void *(BMX_CDECL *apply)(struct ZrwEffectType *type, void *object, void *item, uint32_t flags, void *a,
                             void *b); /* 0x18 */
    uint32_t pad1c;
    void(BMX_CDECL *update)(struct ZrwEffectType *type, void *item, int arg); /* 0x20 */
} ZrwEffectType;

/* Per-object record of the effect object (0x3c bytes, at +0x30 of it). */
typedef struct ZrwEffectHost {
    struct ZList *items; /* 0x00 effect records, each with its type at +0x2c */
    void *updateNode;    /* 0x04 */
    uint32_t pad08[3];   /* 0x08 zero */
    uint32_t pad14;      /* 0x14 never set */
    uint32_t pad18[3];   /* 0x18 zero */
    float one24;         /* 0x24 */
    float one28;         /* 0x28 */
    float one2c;         /* 0x2c */
    float one30;         /* 0x30 */
    uint32_t pad34;      /* 0x34 never set */
    uint32_t active;     /* 0x38 */
} ZrwEffectHost;

_Static_assert(sizeof(ZrwEffectHost) == 0x3c, "effect host");

/* Creates the effect object (type 2); NULL on failure. */
void *zrwworldCreateObject(void);
/* Update callback of the effect object: runs every effect's update hook. Returns 1. */
int zrwworldUpdateEffects(void *object);
/* Applies the effect type with the given name to the object, creating its record on first use. Returns the
 * low byte of `flags`, or 0 when the type or its record does not exist. */
uint32_t zrwworldApplyEffect(void *object, const char *name, uint32_t flags, void *a, void *b);
/* Sector, atomic and clump callbacks that sum polygon and vertex counts into gZrwWorldStats. */
void *zrwworldCountSector(void *sector);
void *zrwworldCountAtomic(void *atomic);
void *zrwworldCountClump(void *clump, void *arg);
/* Logs the world's lights, materials, geometry and clump statistics; returns the counters. */
ZrwWorldStats *zrwworldLogInfo(void);
/* RpWorldForAllLights callback: logs one light. */
void *zrwworldLogLight(void *light);
/* Looks the component of the given kind up in the scene info of a file image; returns the record (in the exe)
 * and its size through *size. NULL when absent. */
ZrwWorldRecord *zrwworldFindRecord(void *image, char kind, uint32_t *size);
/* Material hook while a world loads: queues the material for texture matching. */
int zrwworldAddMaterial(void *material);
/* RpWorld progress callback while the PVS builds: draws "Percent complete" at stage 12. Returns 1. */
int zrwWorldPvsCallback(int stage, float progress);
/* Reads the world stream of a file image, sets up bounds and logs statistics. */
int zrwworldReadStream(void *image, const char *name);
/* Creates the empty startup world. Returns 0 when RenderWare cannot create it. */
int zrwCreateWorld(void);
/* Releases the world's materials and the world itself. Returns 1. */
int zrwWorldShutdown(void);
/* Loads the world named `name` from a file image: materials chunk, stream, texture matching. */
int zrwworldLoad(void *image, const char *name);

#endif
