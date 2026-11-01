/* Effects (C:\zaxis\Bmx\Src\com\action\fx.c). */
#include "bmx/com/action/fx.h"

#include "bmx/com/zge/ngif/missingzge.h"
#include "bmx/com/zge/ngif/ngiffx.h"
#include "bmx/com/zge/ngif/ngifpsx.h"
#include "compat/crt.h"
#include "zge/zdebug.h"
#include "zge/zgemods/zobjcamera.h"
#include "zge/zobj.h"
#include "zge/zstring.h"

/*
 * Floating point follows the original x87 code exactly: `double` is a value held in an x87
 * register (53-bit precision control), `float` is a value the original stored to memory.
 */

static const char fxFile[] = "C:\\zaxis\\Bmx\\Src\\com\\action\\fx.c";

/* Handler table registered with the engine (3 entries). */
#define gFxHandlerTable ORIG_VAR(0x00554718, uint8_t, gFxHandlerTable)

/* Parameter blocks in .data that the effects patch before handing them over. */
#define gFxBurstBlock ORIG_VAR(0x00554724, uint8_t, gFxBurstBlock)

typedef struct FxPuffBlock {
    uint8_t pad00[0xc];
    ZVec3 pos; /* 0x0c */
} FxPuffBlock;
#define gFxPuffBlock ORIG_VAR(0x00554738, FxPuffBlock, gFxPuffBlock)

/* Emitter / creation blocks (0x20 bytes each at 0x554ab8 and 0x554ad8) and the six effect presets. */
typedef struct FxBlock {
    uint8_t pad00[2];
    uint16_t value; /* 0x02 (emitter block) */
    uint32_t id;    /* 0x04 (emitter block) */
    ZVec3 pos;      /* 0x08 */
    uint8_t pad14[4];
    const void *preset; /* 0x18 */
    uint8_t pad1c[4];
} FxBlock;

_Static_assert(sizeof(FxBlock) == 0x20, "FxBlock size");
typedef const void *FxPresets[6];
#define gFxPresets     ORIG_VAR(0x00554a9c, FxPresets, gFxPresets)
#define gFxEmitBlock   ORIG_VAR(0x00554ab8, FxBlock, gFxEmitBlock)
#define gFxCreateBlock ORIG_VAR(0x00554ad8, FxBlock, gFxCreateBlock)

/* The 42 sprite slots of fxScatter: index into the sprite list, a value, and a position. */
typedef struct FxScatterSlot {
    uint16_t index;
    uint16_t value;
    int16_t x;
    int16_t y;
} FxScatterSlot;

typedef FxScatterSlot FxScatterSlots[42];
#define gFxScatterSlots ORIG_VAR(0x00554798, FxScatterSlots, gFxScatterSlots)

static int16_t rdS16(const void *base, uint32_t offset)
{
    int16_t v;
    memcpy(&v, (const uint8_t *)base + offset, sizeof v);
    return v;
}

static float rdFloat(const void *base, uint32_t offset)
{
    float v;
    memcpy(&v, (const uint8_t *)base + offset, sizeof v);
    return v;
}

BMX_ADDR(0x00408960)
void fxNop1(void) {}

BMX_ADDR(0x00408980)
void fxNop2(void) {}

BMX_ADDR(0x004089a0)
void fxSetRider(PhysRider *rider)
{
    gFxRider = rider;
}

/* Registers the node callbacks, the handler table and the effect handlers; returns 0. */
BMX_ADDR(0x004089c0)
uint32_t fxInit(void)
{
    gFxLightCallback = BMX_FNPTR(FxLightFn, 0x004098b0, fxCallbackLight);
    gFxListCallback = BMX_FNPTR(FxListFn, 0x004098f0, fxCreateFromList);
    zfxRegisterFuncTable(&gFxHandlerTable, 3);
    gFxUnused = 0;
    gFxActive = 1;
    ngiffxRegisterHandlers();
    return 0;
}

BMX_ADDR(0x00408a10)
void fxShutdown(void)
{
    zfxDestroyAll(0);
    gFxActive = 0;
    ngiffxShutdown();
}

BMX_ADDR(0x00408a40)
void fxStopAll(void)
{
    zfxDestroyAll(0);
}

BMX_ADDR(0x00408a60)
void fxStopAllMode(int mode)
{
    zfxDestroyAll(mode);
}

BMX_ADDR(0x00408a80)
void fxBurst(void)
{
    ngifpsxSetup(&gFxBurstBlock);
}

/* Parameter block of the "spark" handler. */
typedef struct FxSpark {
    ZVec3 pos;
    uint32_t zero0c;
    ZVec3 vel;
    uint32_t zero1c;
    float scale;
    float scaleEnd;
    int32_t life;
    uint8_t color[4];
    uint32_t zero30;
} FxSpark;

_Static_assert(sizeof(FxSpark) == 0x34, "FxSpark size");

/* Spark with a random velocity (x and z in 1/32 steps of -128..127, y upwards) at a world position. */
BMX_ADDR(0x00408aa0)
void fxSpawn(const void *obj, int index, const ZVec3 *pos, uint32_t seed)
{
    FxSpark spark = {{0.0f, 0.0f, 0.0f},       0, {0.0f, -10.0f, 0.0f}, 0, 2.0f, 2.0f, 2000,
                     {0xff, 0xff, 0xff, 0x40}, 0};
    const uint8_t *const node = ngiffxGetNode(obj, index);

    if (node != NULL) {
        const float *const target = ((const ZObj *)node)->data;

        spark.pos.x = (float)((double)pos->x - target[2]);
        spark.pos.y = (float)((double)pos->y - target[3]);
        spark.pos.z = (float)((double)pos->z - target[4]);
        spark.vel.x = (float)((double)(int16_t)bmxFtol((double)(zmathRandom() & 0xff) - 128.0) * 0.03125f);
        spark.vel.y = (float)((double)(int16_t)(zmathRandom() & 0x7f) * -0.015625f);
        spark.vel.z = (float)((double)(int16_t)bmxFtol((double)(zmathRandom() & 0xff) - 128.0) * 0.03125f);
        zstrNameTable2Call("spark", (uint32_t)(uintptr_t)node, (uint32_t)(uintptr_t)&spark);
    }
}

BMX_ADDR(0x00408c50)
void fxPuff(const ZVec3 *pos)
{
    gFxPuffBlock.pos = *pos;
    ngiffxCreate(1, &gFxPuffBlock, 0);
}

/* Parameter block of the impact spark handler. */
typedef struct FxImpact {
    ZVec3 pos;
    uint32_t zero0c;
    ZVec3 vel;
    uint32_t zero1c;
    float size;
    int32_t life;
    uint32_t zero28;
} FxImpact;

_Static_assert(sizeof(FxImpact) == 0x2c, "FxImpact size");

/* Dust and sparks at a hard impact: position relative to the rider's effect node (and 20 up), velocity
 * halved. */
BMX_ADDR(0x00408c90)
void fxImpactSparks(const ZVec3 *pos, const ZVec3 *vel)
{
    FxImpact impact = {{0.0f, 0.0f, 0.0f}, 0, {0.0f, -10.0f, 0.0f}, 0, 20.0f, 1000, 0};
    const uint8_t *const node = ngiffxGetNode(gFxRider->actor, 0);

    if (node != NULL) {
        const float *const target = ((const ZObj *)node)->data;

        impact.pos.x = (float)((double)pos->x - target[2]);
        impact.pos.y = (float)(((double)pos->y - target[3]) - 20.0f);
        impact.pos.z = (float)((double)pos->z - target[4]);
        impact.vel.x = (float)((double)vel->x * 0.5f);
        impact.vel.y = (float)((double)vel->y * 0.5f);
        impact.vel.z = (float)((double)vel->z * 0.5f);
        zstrNameTable2Call("dust", (uint32_t)(uintptr_t)node, (uint32_t)(uintptr_t)&impact);
    }
}

/* Parameter block of effect 7 of fxScatter. */
typedef struct FxScatterBlock {
    uint16_t value;
    uint16_t color; /* 0xff */
    uint32_t zero04;
    ZVec3 dir;       /* 0x08 */
    uint32_t pad14;  /* never set: stack garbage in the original */
    uint32_t sprite; /* 0x18 */
    ZVec3 offset;    /* 0x1c */
    uint32_t pad28;  /* never set */
    uint32_t zero2c;
    uint32_t zero30;
    uint32_t zero34;
    uint32_t pad38; /* never set */
    uint32_t zero3c;
} FxScatterBlock;

_Static_assert(sizeof(FxScatterBlock) == 0x40, "FxScatterBlock size");

/*
 * Starts effect `kind` for each of the 42 slots of the scatter table, from the last: the slot's
 * position, turned by -angle around y and moved by `offset`, with a random 32 unit cube of jitter
 * around `base`.
 */
BMX_ADDR(0x00408db0)
void fxScatter(const uint32_t *sprites, int kind, const ZVec3 *offset, int angle, const ZVec3 *base)
{
    FxScatterBlock block = {0};
    const ZVec3 origin = *base;

    block.color = 0xff;
    for (int i = 41; i >= 0; i--) {
        const FxScatterSlot *const slot = &gFxScatterSlots[i];
        ZVec3 jitter;
        uint32_t rnd;

        block.sprite = sprites[slot->index];
        block.value = slot->value;
        block.dir.z = -40.0f;
        block.dir.x = -(float)(int16_t)(uint16_t)(slot->x * 2);
        block.dir.y = -(float)(int16_t)(uint16_t)(slot->y * 2);
        zvec3RotateY(&block.dir, &block.dir, (ZAngle)-angle);
        zvec3Add(&block.dir, &block.dir, offset);
        rnd = zmathRandom();
        jitter.x = (float)(int16_t)((rnd & 0x1f) - 0x10);
        jitter.y = (float)(int16_t)((rnd >> 4 & 0x1f) - 0x10);
        jitter.z = (float)(int16_t)((rnd >> 8 & 0x1f) - 0x10);
        zvec3Add(&jitter, &jitter, &origin);
        block.offset = jitter;
        ngiffxCreate(kind, &block, 0);
    }
}

/* Parameter block of the "shard" handler (words 3, 7 and 11 are never set: stack garbage in the original). */
typedef struct FxShard {
    ZVec3 pos;
    uint32_t pad0c;
    ZVec3 vel;
    uint32_t pad1c;
    ZVec3 spin;
    uint32_t pad2c;
    float size;
    int32_t life;
    uint8_t color[4];
    uint32_t zero3c;
} FxShard;

_Static_assert(sizeof(FxShard) == 0x40, "FxShard size");

/*
 * Bursts 32 shards at `pos` (relative to the first effect node): each gets a position jittered by +-300,
 * the velocity `vel` slowed by a random 0..31 steps of 0.005, a random spin and size and a life of 1..3 s.
 */
BMX_ADDR(0x00408fd0)
void fxShards(const ZVec3 *pos, const ZVec3 *vel)
{
    FxShard shard = {{0.0f, 0.0f, 0.0f},       0, {0.0f, 0.0f, 0.0f}, 0, {0.0f, 0.0f, 0.0f}, 0, 0.0f, 0,
                     {0x80, 0x80, 0xd0, 0x70}, 0};
    const uint8_t *const node = ngiffxGetNode(NULL, 0);

    if (node == NULL) {
        return;
    }
    const float *const target = ((const ZObj *)node)->data;

    for (int i = 0x20; i != 0; i--) {
        shard.pos.x = (float)(((double)pos->x - target[2]) + (double)(int16_t)(zmathRandom() % 600 - 300));
        shard.pos.y = (float)(((double)pos->y - target[3]) + (double)(int16_t)(zmathRandom() % 600 - 300));
        shard.pos.z = (float)(((double)pos->z - target[4]) + (double)(int16_t)(zmathRandom() % 600 - 300));
        shard.vel.x = (float)((double)vel->x - (double)(int16_t)(zmathRandom() & 0x1f) * 0.005f);
        shard.vel.y = (float)((double)vel->y - (double)(int16_t)(zmathRandom() & 0x1f) * 0.005f);
        shard.vel.z = (float)((double)vel->z - (double)(int16_t)(zmathRandom() & 0x1f) * 0.005f);
        shard.spin.x = (float)(int16_t)(zmathRandom() % 1000);
        shard.spin.y = (float)(int16_t)(zmathRandom() % 1000);
        shard.spin.z = (float)(int16_t)(zmathRandom() % 1000);
        shard.size = (float)(int16_t)(zmathRandom() % 0x50 + 5);
        shard.life = (int32_t)(zmathRandom() % 2000 + 1000);
        zstrNameTable2Call("shard", (uint32_t)(uintptr_t)node, (uint32_t)(uintptr_t)&shard);
    }
}

/* Four corners of the skid mark quad (0x554964; the words between are used by the particle code). */
typedef struct FxSkidCorner {
    ZVec3 pos;
    uint32_t other;
} FxSkidCorner;

typedef FxSkidCorner FxSkidCorners[4];
#define gFxSkidCorners ORIG_VAR(0x00554964, FxSkidCorners, gFxSkidCorners)
/* A zero vector in .bss, the middle point of the cross product below. */
#define gFxZero ORIG_VAR(0x0056b6d0, ZVec3, gFxZero)

/* Parameter block of the "skid" handler: four corners (pad words 3, 7, 11, 15 never set), life, colour. */
typedef struct FxSkid {
    FxSkidCorner corner[4];
    int32_t life;
    uint8_t color[4];
    uint32_t zero48;
} FxSkid;

_Static_assert(sizeof(FxSkid) == 0x4c, "FxSkid size");

/*
 * Leaves a skid mark from `from` to `to` on a surface with `normal`: a quad 1 unit either side of the
 * segment (the normal's cross product with the direction), lifted 0.5 along the normal, handed to the
 * "skid" handler relative to the first effect node. Segments shorter than 4 (L1) are ignored; flags unused.
 */
BMX_ADDR(0x00409250)
void fxSkidMark(const ZVec3 *from, const ZVec3 *to, int flags, const ZVec3 *normal)
{
    const double dz = (double)from->z - to->z;
    const double dy = (double)from->y - to->y;
    const double dx = (double)from->x - to->x;
    ZVec3 dir, side, up;
    FxSkid skid = {{{{0.0f, 0.0f, 0.0f}, 0}}, 5000, {0x7f, 0x7f, 0x7f, 0x7f}, 0};

    (void)flags;
    if (!((__builtin_fabs(dy) + __builtin_fabs(dz)) + __builtin_fabs(dx) >= 4.0)) {
        return;
    }
    dir.x = (float)dx;
    dir.y = (float)dy;
    dir.z = (float)dz;
    zvec3NormalizeScaled(&dir, &dir);
    up = *normal;
    zvec3Cross3(&side, &gFxZero, &dir, &up);
    gFxSkidCorners[0].pos.x = (float)((double)side.x + from->x);
    gFxSkidCorners[0].pos.y = (float)((double)side.y + from->y);
    gFxSkidCorners[0].pos.z = (float)((double)side.z + from->z);
    gFxSkidCorners[1].pos.x = (float)((double)from->x - side.x);
    gFxSkidCorners[1].pos.y = (float)((double)from->y - side.y);
    gFxSkidCorners[1].pos.z = (float)((double)from->z - side.z);
    gFxSkidCorners[2].pos.x = (float)((double)to->x - side.x);
    gFxSkidCorners[2].pos.y = (float)((double)to->y - side.y);
    gFxSkidCorners[2].pos.z = (float)((double)to->z - side.z);
    gFxSkidCorners[3].pos.x = (float)((double)side.x + to->x);
    gFxSkidCorners[3].pos.y = (float)((double)side.y + to->y);
    gFxSkidCorners[3].pos.z = (float)((double)side.z + to->z);
    const uint8_t *const node = ngiffxGetNode(gFxRider->actor, 0);

    if (node == NULL) {
        return;
    }
    const float *const target = ((const ZObj *)node)->data;
    const double hx = (double)up.x * 0.5f;
    const double hy = (double)up.y * 0.5f;
    const double hz = (double)up.z * 0.5f;
    const double hyf = (float)hy;
    const double hzf = (float)hz;

    skid.corner[0].pos.x = (float)((hx + gFxSkidCorners[0].pos.x) - target[2]);
    skid.corner[0].pos.y = (float)((hy + gFxSkidCorners[0].pos.y) - target[3]);
    skid.corner[0].pos.z = (float)((hz + gFxSkidCorners[0].pos.z) - target[4]);
    skid.corner[1].pos.x = (float)((hx + gFxSkidCorners[1].pos.x) - target[2]);
    skid.corner[1].pos.y = (float)((hyf + gFxSkidCorners[1].pos.y) - target[3]);
    skid.corner[1].pos.z = (float)((hzf + gFxSkidCorners[1].pos.z) - target[4]);
    skid.corner[2].pos.x = (float)((hx + gFxSkidCorners[2].pos.x) - target[2]);
    skid.corner[2].pos.y = (float)((hyf + gFxSkidCorners[2].pos.y) - target[3]);
    skid.corner[2].pos.z = (float)((hzf + gFxSkidCorners[2].pos.z) - target[4]);
    skid.corner[3].pos.x = (float)((hx + gFxSkidCorners[3].pos.x) - target[2]);
    skid.corner[3].pos.y = (float)((hyf + gFxSkidCorners[3].pos.y) - target[3]);
    skid.corner[3].pos.z = (float)((hzf + gFxSkidCorners[3].pos.z) - target[4]);
    zstrNameTable2Call("skid", (uint32_t)(uintptr_t)node, (uint32_t)(uintptr_t)&skid);
}

BMX_ADDR(0x004095a0)
void fxEmit(uint32_t a, uint32_t b, const ZVec3 *pos, uint32_t unused, uint32_t type)
{
    if (type > 5) {
        zdebugFail("type out of range", fxFile, 0x42f, 0);
    }
    gFxEmitBlock.preset = gFxPresets[type];
    gFxEmitBlock.pos = *pos;
    gFxEmitBlock.id = a;
    gFxEmitBlock.value = (uint16_t)b;
    ngiffxCreate(7, &gFxEmitBlock, 0);
}

BMX_ADDR(0x00409640)
void fxCreate(uint32_t type, const ZVec3 *pos, const void *arg)
{
    if (type > 5) {
        zdebugFail("type out of range", fxFile, 0x44c, 0);
    }
    gFxCreateBlock.preset = gFxPresets[type];
    gFxCreateBlock.pos = *pos;
    ngiffxCreate(7, &gFxCreateBlock, arg);
}

/* ---- effect callbacks ---------------------------------------------------------------------------- */

/* One block of parameters for the "ripple" / "splash" particle handlers. */
typedef struct FxRipple {
    ZVec3 pos;
    uint32_t unused0c;
    int32_t life;
    float spread;
    float speed;
    uint8_t color[4];
    uint32_t zero;
} FxRipple;

typedef struct FxSplash {
    ZVec3 pos;
    uint32_t zero0c;
    uint32_t zero10;
    float lift;
    uint32_t zero18;
    uint32_t zero1c;
    float scale;
    float growth;
    int32_t life;
    uint8_t color[4];
    uint32_t zero30;
} FxSplash;

_Static_assert(sizeof(FxRipple) == 0x24 && sizeof(FxSplash) == 0x34, "fx parameter blocks");

/* Water splash at the object's position plus an offset: a ripple and a splash handler call. */
BMX_ADDR(0x004096d0)
void fxWaterSplash(const uint8_t *obj, int unused, const float *offset)
{
    FxRipple ripple = {0};
    FxSplash splash = {0};
    const uint8_t *node = ngiffxGetNode(obj, 0);

    if (node == NULL) {
        return;
    }
    const float *target = ((const ZObj *)node)->data;
    const float objX = rdFloat(obj, 0xc);
    const float objY = rdFloat(obj, 0x10);
    const float objZ = rdFloat(obj, 0x14);
    ripple.pos.x = (float)(((double)objX + (double)offset[0]) - (double)target[2]);
    ripple.pos.y = (float)(((double)objY + (double)offset[1]) - (double)target[3]);
    ripple.pos.z = (float)(((double)objZ + (double)offset[2]) - (double)target[4]);
    ripple.color[0] = ripple.color[1] = ripple.color[2] = ripple.color[3] = 0xff;
    ripple.life = 1000;
    ripple.spread = 30.0f;
    ripple.speed = 250.0f;
    splash.pos = ripple.pos;
    splash.lift = -7.0f;
    splash.scale = 20.0f;
    splash.growth = 4.0f;
    splash.life = 1000;
    splash.color[0] = splash.color[1] = splash.color[2] = 0xff;
    splash.color[3] = 0x40;
    zstrNameTable2Call("ripple", (uint32_t)(uintptr_t)node, (uint32_t)(uintptr_t)&ripple);
    zstrNameTable2Call("splash", (uint32_t)(uintptr_t)node, (uint32_t)(uintptr_t)&splash);
}

/* Empty (the callers pass debug draw parameters; the shipped build does nothing). */
BMX_ADDR(0x00409860)
void fxNop(void) {}

BMX_ADDR(0x00409880)
void fxCallbackEmit(const uint8_t *obj, uint32_t type)
{
    fxEmit(0, 0xff, (const ZVec3 *)(const void *)(obj + 0x10), 0, type);
}

/* Event callback: on event 1 the original calls an empty debug function; always returns 0. */
BMX_ADDR(0x004098b0)
int fxCallbackLight(const uint8_t *obj, int event)
{
    return 0;
}

/* Creates the effects of a level's effect list (entries of 16 bytes after a 4 byte header). */
BMX_ADDR(0x004098f0)
int fxCreateFromList(const uint8_t *list, uint32_t count)
{
    const uint8_t *entry = list + 4;

    for (; count != 0; count--, entry += 16) {
        ZVec3 pos;
        pos.x = (float)rdS16(entry, 0);
        pos.y = (float)rdS16(entry, 2);
        pos.z = (float)rdS16(entry, 4);
        if (entry[6] == 7) {
            fxCreate(entry[7], &pos, entry + 8);
        } else {
            gZdebugPrint("fx: Don't know how to create:%d\n", entry[6]);
        }
    }
    return 0;
}
