/*
 * Renderer world (C:\NgEngine\Zge\Com\ZglRw\zrwWorld.c): the level geometry (an RpWorld read from a .rws
 * stream next to the scene file), the effect object that hosts particle effects, and the statistics logged
 * when a world is loaded. The world and particle callbacks of zrwtxr.c (0x497200..0x497780) belong here too.
 */
#ifndef ZGE_COM_ZGLRW_ZRWWORLD_H
#define ZGE_COM_ZGLRW_ZRWWORLD_H

#include <stddef.h>
#include <stdint.h>

#include "compat/orig.h"
#include "zge/zgemods/zobjpemit.h"
#include "zge/zlist.h"
#include "zge/zmath.h"

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

/* The effect types are ZfxEffect tables (ngiffx.h); the effect object is an emitter object (zobjpemit.h). */

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

/* ---- world reset, PVS, material list and particle steps (0x497200..0x497700) ------------------------ */

#define gZrwWorldActive       ORIG_VAR(0x006741e4, int, gZrwWorldActive)
#define gZrwWorldMaterials    ORIG_VAR(0x006741e8, ZList *, gZrwWorldMaterials)
#define gZrwWorldPvsState     ORIG_VAR(0x00674204, uint32_t, gZrwWorldPvsState)
#define gZrwPvsLoaded         ORIG_VAR(0x006976e4, int, gZrwPvsLoaded)
#define gZrwPvsEnabled        ORIG_VAR(0x006976e8, int, gZrwPvsEnabled)
#define gZrwWorldCenterScaled ORIG_VAR(0x006976d8, ZVec3, gZrwWorldCenterScaled)

/* Releases the world: its materials and the PVS data, then creates the empty replacement world. */
int zrwWorldReset(void);
void *zrwWorldFindMaterial(int hash);
int zrwWorldMaterialCount(void);
void *zrwWorldGetMaterial(int index);
int zrwWorldHasPvs(void);
/* Camera dependent PVS setup; the second entry point (0x4974b0) is the tail of this function. */
int zrwWorldSetupPvs(void);
int zrwWorldSetPvsEnabled(int enabled);
int zrwWorldLoadPvs(void);
int zrwWorldRender(void);
void zrwWorldNop(void);

/* The particle steps, the apply functions of gFxGravity (arg: a ZVec3 acceleration) and gFxFloorColl (arg:
 * the floor y and the bounce factor); nothing moves without a modifier or an emitter. */
void zrwParticleAccelerate(ZfxModifier *self, ZfxEmitter *emitter, void *arg);
void zrwParticleNop(void);
void zrwParticleFloorBounce(ZfxModifier *self, ZfxEmitter *emitter, void *arg);

#endif
