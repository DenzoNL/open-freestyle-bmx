/*
 * Tests for zanim.c (animation chunk readers) and zobjlight.c (light objects).
 *
 * The chunk reader is stubbed: the stubs answer the chunk size from the fixture and fill the destinations
 * with random words, and log their arguments. The renderer's light functions and the object allocator are
 * stubbed too. Assert paths end the run in the harness (the abort is compared), so the fixtures aim at
 * valid input most of the time and at a bad size or type now and then.
 */
#include "dt.h"
#include "dt_ngif.h"
#include "dt_zmem.h"
#include "zge/zanim.h"
#include "zge/zgemods/zobjlight.h"
#include "zge/ziff.h"

/* ---- chunk reader stubs --------------------------------------------------------------------------- */

static uint32_t gChunkSize;
static uint32_t gFirstWord; /* what the first 4 byte read answers (the bone index) */
static int gReads;

static uint32_t BMX_CDECL stubChunkSize(void *iff)
{
    ngLog(0x49abc0);
    ngLogPtr(iff);
    return gChunkSize;
}

static int BMX_CDECL stubIffRead(void *iff, void *dst, uint32_t size)
{
    uint32_t *words = dst;

    ngLog(0x49ac30);
    ngLogPtr(iff);
    ngLog(size);
    if (size == 0) {
        return 0;
    }
    for (uint32_t i = 0; i < size / 4; i++) {
        words[i] = dt_u32();
    }
    if (gReads == 0 && size == 4) {
        words[0] = gFirstWord;
    }
    gReads++;
    return (int)size;
}

static void begin(const DtCtx *c)
{
    ngBegin(c, 0);
    gReads = 0;
    dt_stub(0x0049abc0, NULL, (void *)stubChunkSize);
    dt_stub(0x0049ac30, NULL, (void *)stubIffRead);
}

/* A name table of `count` entries; the bone hash of each (+0x20) is random. */
static ZanimNames *make_names(uint32_t count, ZanimNames *next)
{
    ZanimNames *names = dt_alloc(sizeof *names);

    names->count = count;
    names->names = dt_alloc(count * 0x24 + 0x40);
    dt_fill(names->names, count * 0x24 + 0x40);
    names->next = next;
    return names;
}

static ZanimChannel *make_channel(ZanimChannel *next)
{
    ZanimChannel *channel = dt_alloc(sizeof *channel);

    channel->count = (uint32_t)dt_range(0, 3);
    channel->boneHash = dt_u32();
    channel->keys = dt_alloc(0x40);
    channel->next = next;
    return channel;
}

/* The animation being read: a record of {name, names, rotations, translations, unused}. */
static uint32_t *make_anim(uint32_t nameCount)
{
    uint32_t *anim = dt_alloc(0x14);

    anim[0] = dt_u32();
    anim[1] = (uint32_t)(uintptr_t)make_names(nameCount, dt_u32() & 1 ? make_names(2, NULL) : NULL);
    anim[2] = (dt_u32() & 1) ? (uint32_t)(uintptr_t)make_channel(NULL) : 0;
    anim[3] = (dt_u32() & 1) ? (uint32_t)(uintptr_t)make_channel(make_channel(NULL)) : 0;
    gZiffAnim = anim;
    return anim;
}

static void setup_defi(DtCtx *c)
{
    const uint32_t count = (uint32_t)dt_range(0, 5);
    const uint32_t extra = dt_u32() % 6 == 0 ? (uint32_t)dt_range(1, 3) : 0;

    begin(c);
    make_anim(3);
    gChunkSize = 4 + count * 0x24 + extra;
    DT_ARG_PTR(c, 0, dt_alloc(8));
}

static void setup_channel(DtCtx *c, uint32_t unit)
{
    const uint32_t names = (uint32_t)dt_range(1, 4);
    const uint32_t count = (uint32_t)dt_range(0, 5);

    begin(c);
    make_anim(names);
    gFirstWord = (uint32_t)dt_range(0, (int)names); /* the last one is one past the table */
    gChunkSize = 4 + count * unit + (uint32_t)dt_range(0, 3);
    DT_ARG_PTR(c, 0, dt_alloc(8));
}

static void setup_rota(DtCtx *c)
{
    setup_channel(c, 20);
}

static void setup_atra(DtCtx *c)
{
    setup_channel(c, 16);
}

static void setup_nop(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, dt_alloc(8));
}

#define gZanimInitialized ORIG_VAR(0x006746c0, int, gZanimInitialized)

static int BMX_CDECL otherHandler(void *iff)
{
    (void)iff;
    return 2;
}

static void setup_init(DtCtx *c)
{
    typedef void *(BMX_CDECL * CallbackFn)(uint32_t, int(BMX_CDECL *)(void *), int);
    typedef void(BMX_CDECL * InitFn)(void);
    static const uint32_t ids[4] = {0x4d494e41, 0x49464544, 0x544f5241, 0x41525441};
    static const uint32_t handlers[4] = {0x498fb0, 0x498fc0, 0x4990b0, 0x499200};

    ngBegin(c, 0);
    DT_ORIG(0x0049a6d0, InitFn)();
    gZanimInitialized = dt_u32() % 8 == 0;
    for (int i = 0; i < 4; i++) {
        if (dt_u32() % 4 == 0) {
            /* already registered: with the same handler or another one */
            DT_ORIG(0x0049aad0, CallbackFn)(
                ids[i], dt_u32() & 1 ? DT_ORIG(handlers[i], int(BMX_CDECL *)(void *)) : otherHandler, 0);
        }
    }
}

/* ---- light objects ---------------------------------------------------------------------------------- */

static void *gNewObject;

static void BMX_CDECL stubLightDestroy(void *handle)
{
    ngLog(0x49f6d0);
    ngLogPtr(handle);
}

static void BMX_CDECL stubLightParam(void *handle, uint32_t param)
{
    ngLog(0x49f7d0);
    ngLogPtr(handle);
    ngLog(param);
}

static void BMX_CDECL stubLightDirection(void *handle, const ZVec3 *direction)
{
    ngLog(0x49f840);
    ngLogPtr(handle);
    ngLogVec(direction);
}

static void *BMX_CDECL stubLightCreate(int type, int param)
{
    ngLog(0x49f550);
    ngLog((uint32_t)type);
    ngLog((uint32_t)param);
    return (void *)(uintptr_t)(0x30000000u + dt_u32() % 64);
}

static void *BMX_CDECL stubObjAlloc(void)
{
    ngLog(0x487370);
    return gNewObject;
}

static void *BMX_CDECL stubObjInit(void *object, uint32_t type)
{
    ngLog(0x4873a0);
    ngLogPtr(object);
    ngLog(type);
    return object;
}

static ZObjLight *make_light(void)
{
    ZObjLight *light = dt_alloc(0x40);

    light->handle = (void *)(uintptr_t)(0x30000000u + dt_u32() % 64);
    return light;
}

static void setup_destroy(DtCtx *c)
{
    ngBegin(c, 0);
    dt_stub(0x0049f6d0, NULL, (void *)stubLightDestroy);
    DT_ARG_PTR(c, 0, make_light());
}

static void setup_param(DtCtx *c)
{
    ngBegin(c, 0);
    dt_stub(0x0049f7d0, NULL, (void *)stubLightParam);
    DT_ARG_PTR(c, 0, make_light());
    DT_ARG_INT(c, 1, dt_u32());
}

static void setup_rotation(DtCtx *c)
{
    ZQuat *q = dt_alloc(sizeof *q);

    ngBegin(c, 0);
    dt_stub(0x0049f840, NULL, (void *)stubLightDirection);
    q->x = dt_float_edgy(-1.0f, 1.0f);
    q->y = dt_float_edgy(-1.0f, 1.0f);
    q->z = dt_float_edgy(-1.0f, 1.0f);
    q->w = dt_float_edgy(-1.0f, 1.0f);
    DT_ARG_PTR(c, 0, make_light());
    DT_ARG_PTR(c, 1, q);
}

static void setup_light_init(DtCtx *c)
{
    ngBegin(c, 0);
    ORIG_VAR(0x00673f68, int, gLightInitialized) = dt_u32() % 5 == 0;
}

static void setup_create(DtCtx *c)
{
    ngBegin(c, 0);
    dt_stub(0x00487370, NULL, (void *)stubObjAlloc);
    dt_stub(0x004873a0, NULL, (void *)stubObjInit);
    dt_stub(0x0049f550, NULL, (void *)stubLightCreate);
    gNewObject = dt_u32() % 6 == 0 ? NULL : dt_alloc(0x40);
    DT_ARG_INT(c, 0, dt_u32() % 8 == 0 ? dt_range(-2, 7) : dt_range(0, 3));
    DT_ARG_INT(c, 1, dt_u32());
}

DT_CASES(zanim_cases) = {
    {"zanimReadAnim", 0x00498fb0, zanimReadAnim, 1, DT_RET_INT, setup_nop, 5, 0},
    {"zanimReadDefinitions", 0x00498fc0, zanimReadDefinitions, 1, DT_RET_INT, setup_defi, 400, 0},
    {"zanimReadRotations", 0x004990b0, zanimReadRotations, 1, DT_RET_INT, setup_rota, 400, 0},
    {"zanimReadTranslations", 0x00499200, zanimReadTranslations, 1, DT_RET_INT, setup_atra, 400, 0},
    {"zanimInitialize", 0x00499330, zanimInitialize, 0, DT_RET_INT, setup_init, 300, 0},
    {"zobjLightDestroy", 0x0048ff50, zobjLightDestroy, 1, DT_RET_INT, setup_destroy, 20, 0},
    {"zobjLightSetParam", 0x0048ff70, zobjLightSetParam, 2, DT_RET_INT, setup_param, 50, 0},
    {"zobjLightSetRotation", 0x0048ff90, zobjLightSetRotation, 2, DT_RET_INT, setup_rotation, 400, 0},
    {"zobjLightInitialize", 0x0048ffc0, zobjLightInitialize, 0, DT_RET_VOID, setup_light_init, 20, 0},
    {"zobjLightCreate", 0x00490010, zobjLightCreate, 2, DT_RET_INT, setup_create, 300, 0},
};
