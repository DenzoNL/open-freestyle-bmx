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
/* A modifier object: +4 is its function (gravity, floor collision), called once per update. */
typedef struct ZfxModifier {
    uint32_t pad0;
    void(BMX_CDECL *apply)(struct ZfxModifier *self, struct ZfxEmitter *emitter, void *arg);
} ZfxModifier;

typedef struct ZfxModifierNode {
    struct ZfxModifierNode *next;
    uint32_t pad4;
    ZfxModifier *modifier; /* 0x08 */
    void *arg;             /* 0x0c */
} ZfxModifierNode;

struct ZfxEffect;
typedef struct ZfxEffect ZfxEffect;

/* Particle emitter of one effect instance. */
typedef struct ZfxEmitter {
    ZList *particles;   /* 0x00 */
    ZMemFreeList *pool; /* 0x04 particle allocator */
    ZVec3 min;          /* 0x08 bounds of the live particles, rebuilt by every update */
    uint32_t pad14;
    ZVec3 max; /* 0x18 */
    uint32_t pad24;
    uint32_t lastUpdate; /* 0x28 timeGetTime of the last update */
    uint32_t effect;     /* 0x2c argument of ngiffxPosterCreate */
} ZfxEmitter;

_Static_assert(sizeof(ZfxEmitter) == 0x30, "emitter layout");

/* Handler table of an effect, in the exe's data (0x55c278 + 0x38 * n). */
struct ZfxEffect {
    void(BMX_CDECL *destroy)(ZfxEffect *effect); /* 0x00 */
    uint32_t pad04;
    void(BMX_CDECL *attach)(ZfxEffect *effect, const char *modifierName, const void *params); /* 0x08 */
    uint32_t pad0c[6];
    ZfxModifierNode *modifiers; /* 0x24 */
    uint8_t pad28[0x10];
};

_Static_assert(sizeof(ZfxEffect) == 0x38, "effect table layout");

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
    uint32_t base;        /* 0x08 */
    uint32_t texture[16]; /* 0x0c one per frame */
    uint32_t lastTime;    /* 0x4c */
};
_Static_assert(sizeof(ZfxSkid) == 0x50, "skid layout");
enum { kFxSkidEntries = 8 };
typedef ZfxSkid ZfxSkidTable[kFxSkidEntries];
#define gFxSkids ORIG_VAR(0x0068fd00, ZfxSkidTable, gFxSkids)

/* Looks an animation library up by name and fills `table` from it (the first item sits at the library). */
void ngiffxLoadAnimTable(const char *name, uint32_t *table, int size);
/* Debug value at 0x5ab454. */
float ngiffxGetDebugFloat(void);
void ngiffxNop(void);
/* Adapts the quality value (0x55bd08, 60..200) to the measured frame rate and applies it. */
void ngiffxAdaptQuality(void);
void ngiffxSetQuality(float quality);

/*
 * Spawners, as called by the effect handler tables: (enabled, object, emitter, count, unused, params).
 * `params` NULL selects the effect's default block in the exe's data.
 */
void ngiffxSparkSpawn(int enabled, const void *object, ZfxEmitter *emitter, uint32_t count, int unused,
                      const float *params);
void ngiffxShardSpawn(int enabled, const void *object, ZfxEmitter *emitter, uint32_t count, int unused,
                      const float *params);
void ngiffxDustSpawn(int enabled, const void *object, ZfxEmitter *emitter, uint32_t count, int unused,
                     const float *params);
void ngiffxPosterSpawn(int enabled, const void *object, ZfxEmitter *emitter, uint32_t count, int unused,
                       const float *params);
void ngiffxRippleSpawn(int enabled, const void *object, ZfxEmitter *emitter, uint32_t count, int unused,
                       const float *params);
void ngiffxLightningSpawn(int enabled, const void *object, ZfxEmitter *emitter, uint32_t count, int unused,
                          const float *params);
/* Renderers and the generic updater: return the number of live particles. */
int ngiffxSparkRender(const void *effect, ZfxEmitter *emitter);
int ngiffxShardRender(const void *effect, ZfxEmitter *emitter);
int ngiffxLightningRender(const void *effect, ZfxEmitter *emitter);
int ngiffxSplashRender(const void *effect, ZfxEmitter *emitter);
int ngiffxDustRender(const void *effect, ZfxEmitter *emitter);
int ngiffxPosterRender(const void *effect, ZfxEmitter *emitter);
int ngiffxRippleRender(const void *effect, ZfxEmitter *emitter);
int ngiffxUpdateEmitter(ZfxEffect *effect, ZfxEmitter *emitter);
/* Creates an emitter for up to `capacity` particles; NULL when out of memory. */
ZfxEmitter *ngiffxPosterCreate(uint32_t effect, uint16_t capacity);

/* Effect creators registered under the effect names: (handle, params or NULL). */
void ngiffxCreateSpark(uint32_t handle, float *params);
void ngiffxCreateShard(uint32_t handle, const void *params);
void ngiffxCreateSplash(uint32_t handle, float *params);
void ngiffxCreateDust(uint32_t handle, const void *params);
void ngiffxCreatePoster(uint32_t handle, const void *params);
void ngiffxCreateRipple(uint32_t handle, uint32_t *params);
void ngiffxCreateSkid(uint32_t handle, uint32_t *params);
void ngiffxCreateLightning(uint32_t handle);

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
