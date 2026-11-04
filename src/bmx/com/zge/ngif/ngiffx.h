/*
 * Particle effects of the level renderer (C:\zaxis\Bmx\src\com\zge\NgIF\NgIFfx.c). An effect ("spark",
 * "shard", "splash", "dust", "poster", "ripple", "lightning") is a table of handlers in the exe's data
 * (ZfxEffect); each emitter owns a ZList of particles drawn from a free list.
 */
#ifndef BMX_COM_ZGE_NGIF_NGIFFX_H
#define BMX_COM_ZGE_NGIF_NGIFFX_H

#include <stdint.h>

#include "compat/orig.h"
#include "zge/zlist.h"
#include "zge/zmath.h"
#include "zge/zmem.h"

/* A live particle (0xa0 bytes, from the emitter's free list). Positions are in world space. */
typedef struct ZfxParticle {
    int32_t kind; /* 0x00 bit 0: moves, bit 1: its axis rotates (1 spark, 3 shard, 0 static) */
    ZVec3 pos;    /* 0x04 */
    uint32_t pad10;
    ZVec3 prev; /* 0x14 position before the last update */
    uint32_t pad20;
    ZVec3 vel; /* 0x24 */
    uint32_t pad30;
    ZVec3 axis; /* 0x34 shard axis */
    uint32_t pad40;
    ZAngle3 spin;    /* 0x44 per update axis rotation */
    uint8_t rgba[4]; /* 0x4a (not aligned) */
    uint8_t pad4e[2];
    float size;         /* 0x50 */
    uint32_t texture;   /* 0x54 */
    uint32_t life;      /* 0x58 ms */
    uint32_t birth;     /* 0x5c timeGetTime at creation */
    float corner[4][4]; /* 0x60 poster corners (xyz + padding) */
} ZfxParticle;

_Static_assert(__builtin_offsetof(ZfxParticle, rgba) == 0x4a, "particle layout");
_Static_assert(__builtin_offsetof(ZfxParticle, birth) == 0x5c, "particle layout");
_Static_assert(sizeof(ZfxParticle) == 0xa0, "particle layout");

struct ZfxEmitter;
/* A modifier object (gFxGravity, gFxFloorColl): +4 is its function, called once per update. */
typedef struct ZfxModifier {
    void(BMX_CDECL *reset)(void); /* 0x00 a no-op in both of the engine's modifiers */
    void(BMX_CDECL *apply)(struct ZfxModifier *self, struct ZfxEmitter *emitter, void *arg); /* 0x04 */
} ZfxModifier;

typedef struct ZfxModifierNode {
    struct ZfxModifierNode *next;
    uint32_t pad4;
    ZfxModifier *modifier; /* 0x08 */
    void *arg;             /* 0x0c */
} ZfxModifierNode;

struct ZfxEffect;
typedef struct ZfxEffect ZfxEffect;

/*
 * Particle emitter of one effect instance, made by the effect's create handler: the record an effect object
 * (zobjpemit.c, zrwworld.c) keeps per effect.
 */
typedef struct ZfxEmitter {
    ZList *particles;   /* 0x00 */
    ZMemFreeList *pool; /* 0x04 particle allocator */
    ZVec3 min;          /* 0x08 bounds of the live particles, rebuilt by every update */
    uint32_t pad14;
    ZVec3 max; /* 0x18 */
    uint32_t pad24;
    uint32_t lastUpdate;      /* 0x28 timeGetTime of the last update */
    struct ZfxEffect *effect; /* 0x2c the effect that created it */
} ZfxEmitter;

BMX_LAYOUT32_ASSERT(sizeof(ZfxEmitter) == 0x30, "emitter layout");
BMX_LAYOUT32_ASSERT(__builtin_offsetof(ZfxEmitter, lastUpdate) == 0x28, "emitter layout");

/* Handler table of an effect, in the exe's data (0x55c278 + 0x38 * n). */
struct ZfxEffect {
    void(BMX_CDECL *destroy)(ZfxEffect *effect);                                       /* 0x00 */
    void(BMX_CDECL *addModifier)(ZfxEffect *effect, ZfxModifier *modifier, void *arg); /* 0x04 */
    void(BMX_CDECL *attach)(ZfxEffect *effect, const char *modifierName, void *arg);   /* 0x08 */
    void(BMX_CDECL *clearModifiers)(ZfxEffect *effect);                                /* 0x0c */
    ZfxEmitter *(BMX_CDECL *create)(ZfxEffect *effect, uint16_t capacity);             /* 0x10 */
    void(BMX_CDECL *destroyEmitter)(ZfxEffect *effect, ZfxEmitter *emitter);           /* 0x14 */
    void(BMX_CDECL *spawn)(const ZfxEffect *effect, const void *object, ZfxEmitter *emitter, uint32_t count,
                           void *unused, const float *params);      /* 0x18 */
    int(BMX_CDECL *update)(ZfxEffect *effect, ZfxEmitter *emitter); /* 0x1c */
    /* 0x20 the world's effect object passes a third argument, 0 */
    int(BMX_CDECL *render)(const ZfxEffect *effect, ZfxEmitter *emitter, int unused);
    ZfxModifierNode *modifiers;    /* 0x24 the modifier list: head, tail, count (zobjpemit.c) */
    ZfxModifierNode *modifierTail; /* 0x28 */
    int modifierCount;             /* 0x2c */
    uint8_t pad30[0x8];            /* 0x30 */
};

BMX_LAYOUT32_ASSERT(sizeof(ZfxEffect) == 0x38, "effect table layout");
BMX_LAYOUT32_ASSERT(__builtin_offsetof(ZfxEffect, render) == 0x20, "effect table layout");
BMX_LAYOUT32_ASSERT(__builtin_offsetof(ZfxEffect, modifiers) == 0x24, "effect table layout");

/* Vertex of the renderer's immediate mode primitives. */
typedef struct ZfxVertex {
    float x, y, z;
    uint32_t pad0c;
    float u, v;
    uint32_t color;
} ZfxVertex;

/* One primitive handed to the renderer: its texture and its vertices. */
typedef struct ZfxTriangle {
    uint32_t texture;
    ZfxVertex *vertex[3];
} ZfxTriangle;
typedef struct ZfxQuad {
    uint32_t texture;
    ZfxVertex *vertex[4];
} ZfxQuad;
typedef struct ZfxLine {
    uint32_t texture;
    ZfxVertex *vertex[2];
} ZfxLine;

typedef struct ZfxSkid ZfxSkid;
/* A level object with a texture animation: texture handles rotate through the frames. */
struct ZfxSkid {
    void *object;  /* 0x00 level object (0 ends the table) */
    uint8_t frame; /* 0x04 */
    uint8_t count; /* 0x05 frames */
    uint8_t fps;   /* 0x06 */
    uint8_t pad07;
    struct ZrwMaterial *base;        /* 0x08 */
    struct ZrwMaterial *texture[16]; /* 0x0c one per frame */
    uint32_t lastTime;               /* 0x4c */
};
BMX_LAYOUT32_ASSERT(sizeof(ZfxSkid) == 0x50, "skid layout");
enum { kFxSkidEntries = 8 };
typedef ZfxSkid ZfxSkidTable[kFxSkidEntries];
#define gFxSkids ORIG_VAR(0x0068fd00, ZfxSkidTable, gFxSkids)

/* Looks an animation library up by name and fills `table` from it (the first item sits at the library). */
void ngiffxLoadAnimTable(const char *name, void **table, int size);
/* Debug value at 0x5ab454. */
float ngiffxGetDebugFloat(void);
void ngiffxNop(void);
/* Adapts the quality value (0x55bd08, 60..200) to the measured frame rate and applies it. */
void ngiffxAdaptQuality(void);
void ngiffxSetQuality(float quality);

/*
 * Spawners, as called by the effect handler tables (ZfxEffect.spawn): nothing is spawned without an effect
 * or an emitter. `params` NULL selects the effect's default block in the exe's data.
 */
void ngiffxSparkSpawn(const ZfxEffect *effect, const void *object, ZfxEmitter *emitter, uint32_t count,
                      void *unused, const float *params);
void ngiffxShardSpawn(const ZfxEffect *effect, const void *object, ZfxEmitter *emitter, uint32_t count,
                      void *unused, const float *params);
void ngiffxDustSpawn(const ZfxEffect *effect, const void *object, ZfxEmitter *emitter, uint32_t count,
                     void *unused, const float *params);
void ngiffxPosterSpawn(const ZfxEffect *effect, const void *object, ZfxEmitter *emitter, uint32_t count,
                       void *unused, const float *params);
void ngiffxRippleSpawn(const ZfxEffect *effect, const void *object, ZfxEmitter *emitter, uint32_t count,
                       void *unused, const float *params);
void ngiffxLightningSpawn(const ZfxEffect *effect, const void *object, ZfxEmitter *emitter, uint32_t count,
                          void *unused, const float *params);
/* Renderers and the generic updater: return the number of live particles. */
int ngiffxSparkRender(const ZfxEffect *effect, ZfxEmitter *emitter, int unused);
int ngiffxShardRender(const ZfxEffect *effect, ZfxEmitter *emitter, int unused);
int ngiffxLightningRender(const ZfxEffect *effect, ZfxEmitter *emitter, int unused);
int ngiffxSplashRender(const ZfxEffect *effect, ZfxEmitter *emitter, int unused);
int ngiffxDustRender(const ZfxEffect *effect, ZfxEmitter *emitter, int unused);
int ngiffxPosterRender(const ZfxEffect *effect, ZfxEmitter *emitter, int unused);
int ngiffxRippleRender(const ZfxEffect *effect, ZfxEmitter *emitter, int unused);
int ngiffxUpdateEmitter(ZfxEffect *effect, ZfxEmitter *emitter);
/* Creates an emitter for up to `capacity` particles; NULL when out of memory. */
ZfxEmitter *ngiffxPosterCreate(ZfxEffect *effect, uint16_t capacity);

/* Effect creators registered under the effect names: (handle, params or NULL). */
void ngiffxCreateSpark(const void *object, float *params);
void ngiffxCreateShard(const void *object, const void *params);
void ngiffxCreateSplash(const void *object, float *params);
void ngiffxCreateDust(const void *object, const void *params);
void ngiffxCreatePoster(const void *object, const void *params);
void ngiffxCreateRipple(const void *object, uint32_t *params);
void ngiffxCreateSkid(const void *object, uint32_t *params);
void ngiffxCreateLightning(const void *object);

/* The effect scene object (0x68ff80); the arguments are ignored. */
const uint8_t *ngiffxGetNode(const void *obj, int index);
/* Collects the "ANIM" level objects with their texture animations; 1 when there are any. */
int ngiffxFindSkidObjects(void);
/* Per frame callback: steps the texture animations of those objects. */
int ngiffxAnimateSkids(void);
/* Registers the effect handlers (spark, shard, ripple, splash, ...) by name. */
void ngiffxRegisterHandlers(void);
/* Destroys the effect list and unregisters the handlers. */
void ngiffxShutdown(void);
/* Starts effect `kind` (a debug print for kinds other than 0 in this build); returns 0. */
int ngiffxCreate(int kind, const void *params, const void *arg);

#endif
