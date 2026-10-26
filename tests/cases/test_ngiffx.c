/*
 * Tests for NgIFfx.c: particle spawners, renderers, the emitter updater, effect creators and the
 * registration code. The renderer's draw calls, camera, texture lookups and the exe's effect-by-name call
 * are stubbed: every call is logged by value (vertex data included), so the effects are compared through
 * what they would draw. Particle memory and the emitter lists are real (built with the original allocators)
 * and compared as memory.
 */
#include "bmx/com/zge/ngif/ngiffx.h"
#include "compat/crt.h"
#include "dt.h"
#include "dt_ngif.h"
#include "dt_zmem.h"
#include "zge/com/zglrw/zrwzgl.h"
#include "zge/zgemods/zobjcamera.h"
#include "zge/zobj.h"
#include "zge/zstring.h"

#define NOW 1001u /* the first timeGetTime of an iteration */

/* ---- fixtures ------------------------------------------------------------------------------------ */

typedef ZList *(*ListCreateFn)(int);
typedef void (*ListInsertFn)(ZList *, void *);
typedef ZMemFreeList *(*PoolCreateFn)(uint32_t, uint32_t, ZMemDestroyFn);
typedef void *(*PoolAllocFn)(ZMemFreeList *);

static ZMatrix gMatrix;

static float rf(void)
{
    return dt_float_edgy(-100.0f, 100.0f);
}

static float rpos(void)
{
    return dt_float_edgy(0.0f, 20.0f);
}

static void fillWords(float *p, int n)
{
    for (int i = 0; i < n; i++) {
        p[i] = rf();
    }
}

/* Remaining life targets that hit every threshold of the renderers. */
static uint32_t remaining(void)
{
    static const uint32_t kRem[] = {0,     1,     5,     100,    255,    256,    700,        1023,
                                    1024,  1500,  2047,  2048,   5000,   29999,  30000,      30001,
                                    39999, 40000, 40001, 100000, 0x7530, 0x7531, 0xffffffffu};
    return kRem[dt_u32() % (sizeof kRem / sizeof kRem[0])];
}

static ZfxParticle *newParticle(ZfxEmitter *e, int moving)
{
    ZfxParticle *p = DT_ORIG(0x00486050, PoolAllocFn)(e->pool);
    const uint32_t birth = (uint32_t)dt_range(800, 1001);

    dt_fill(p, sizeof *p);
    p->kind = moving ? dt_range(0, 3) : dt_range(0, 1) * 2;
    p->pos = (ZVec3){rf(), rf(), rf()};
    p->prev = (ZVec3){rf(), rf(), rf()};
    p->vel = (ZVec3){rf(), rf(), rf()};
    p->axis = (ZVec3){rf(), rf(), rf()};
    p->spin = (ZAngle3){(ZAngle)dt_u32(), (ZAngle)dt_u32(), (ZAngle)dt_u32()};
    p->size = rpos();
    p->birth = birth;
    p->life = remaining() + NOW - birth;
    for (int i = 0; i < 4; i++) {
        p->corner[i][0] = rf();
        p->corner[i][1] = rf();
        p->corner[i][2] = rf();
    }
    DT_ORIG(0x0048c460, ListInsertFn)(e->particles, p);
    return p;
}

static ZfxEmitter *newEmitter(int particles, int moving)
{
    ZfxEmitter *e = dt_alloc(sizeof *e);
    const int capacity = dt_range(1, 6);

    dt_fill(e, sizeof *e);
    e->particles = DT_ORIG(0x0048c260, ListCreateFn)(0x20);
    e->pool = DT_ORIG(0x00485e80, PoolCreateFn)(sizeof(ZfxParticle), (uint32_t)capacity, NULL);
    for (int i = 0; i < particles; i++) {
        newParticle(e, moving);
    }
    e->lastUpdate = NOW - (uint32_t)(dt_u32() % 3 == 0 ? dt_range(0, 100000) : dt_range(0, 40));
    return e;
}

/* An object whose transform pointer (+0x30) leads to a block with the position at +8. */
static void *newObject(void)
{
    uint8_t *object = dt_alloc(0x40);
    float *xf = dt_alloc(0x40);

    fillWords(xf, 16);
    *(float **)(object + 0x30) = xf;
    return object;
}

/* A random parameter block of `words` words; the raw words in `raw` (a bit mask) stay integers. */
static float *newParams(int words)
{
    float *p = dt_alloc((uint32_t)words * 4 + 0x20);

    for (int i = 0; i < words; i++) {
        p[i] = rf();
    }
    return p;
}

static void rawWord(float *p, int index, uint32_t value)
{
    memcpy(&p[index], &value, 4);
}

/* ---- stubs ---------------------------------------------------------------------------------------- */

static void BMX_CDECL stubDraw(uint32_t flags, const uint32_t *desc, int count, int a, int b)
{
    const int vertices = flags == 0x6502 ? 3 : 4;

    ngLog(0x48d000);
    ngLog(flags);
    ngLog((uint32_t)count);
    ngLog((uint32_t)a);
    ngLog((uint32_t)b);
    for (int i = 0; i < count && i < 4; i++) {
        const uint32_t *primitive = desc + i * (1 + vertices);

        ngLog(primitive[0]);
        for (int k = 0; k < vertices; k++) {
            const ZfxVertex *v = (const ZfxVertex *)(uintptr_t)primitive[1 + k];

            ngLogFloat(v->x);
            ngLogFloat(v->y);
            ngLogFloat(v->z);
            ngLogFloat(v->u);
            ngLogFloat(v->v);
            ngLog(v->color);
        }
    }
}

static void *BMX_CDECL stubGetCamera(void)
{
    ngLog(0x493070);
    return (void *)0xca00;
}

static void *BMX_CDECL stubCameraObject(void *camera)
{
    ngLog(0x493090);
    ngLogPtr(camera);
    return (void *)0xca10;
}

static ZMatrix *BMX_CDECL stubCameraMatrix(void *camera, uint32_t kind)
{
    ngLog(0x482310);
    ngLogPtr(camera);
    ngLog(kind);
    return &gMatrix;
}

static void stubRenderer(void)
{
    gMatrix = (ZMatrix){.right = {rf(), rf(), rf()},
                        .up = {rf(), rf(), rf()},
                        .at = {rf(), rf(), rf()},
                        .pos = {rf(), rf(), rf()},
                        .flags = dt_u32(),
                        .pad1 = dt_u32(),
                        .pad2 = dt_u32(),
                        .pad3 = dt_u32()};
    dt_stub(0x0048d120, NULL, (void *)stubDraw);
    dt_stub(0x0048d660, NULL, (void *)stubDraw);
    dt_stub(0x00493070, NULL, (void *)stubGetCamera);
    dt_stub(0x00493090, NULL, (void *)stubCameraObject);
    dt_stub(0x00482310, NULL, (void *)stubCameraMatrix);
}

/* ---- small functions ----------------------------------------------------------------------------- */

static void *BMX_CDECL stubFindAnim(int flags, const char *name)
{
    ngLog(0x491720);
    ngLog((uint32_t)flags);
    ngLog(ngHash(name));
    return (dt_u32() & 1) ? (void *)0x4000 : NULL;
}

static int BMX_CDECL stubAnimTable(void *library, uint32_t *table, int size)
{
    ngLog(0x491a80);
    ngLogPtr(library);
    ngLogPtr(table);
    ngLog((uint32_t)size);
    return 0;
}

static void setup_load_anim(DtCtx *c)
{
    ngBegin(c, 0);
    dt_stub(0x00491720, NULL, (void *)stubFindAnim);
    dt_stub(0x00491a80, NULL, (void *)stubAnimTable);
    DT_ARG_PTR(c, 0, dt_rand_str(12, 0));
    DT_ARG_PTR(c, 1, dt_alloc(16));
    DT_ARG_INT(c, 2, dt_u32());
}

static void setup_debug_float(DtCtx *c)
{
    ngBegin(c, 0);
    ORIG_VAR(0x005ab454, float, gDebugFloat) = rf();
}

static void setup_none(DtCtx *c)
{
    ngBegin(c, 0);
}

static void BMX_CDECL stubSetQuality(float quality)
{
    ngLog(0x45c920);
    ngLogFloat(quality);
}

static void setup_adapt_quality(DtCtx *c)
{
    static const uint32_t kElapsed[] = {0,    1,     50,    99,    100,   101,   150,
                                        2000, 40000, 65535, 65536, 70000, 100100};

    ngBegin(c, 0);
    ORIG_VAR(0x0055bd08, float, gQuality) = dt_float_edgy(20.0f, 260.0f);
    ORIG_VAR(0x0055bd0c, float, gFps) = rf();
    ORIG_VAR(0x005ab49c, uint32_t, gTime) =
        (dt_u32() % 6 == 0) ? 0 : NOW - kElapsed[dt_u32() % (sizeof kElapsed / sizeof kElapsed[0])];
    ORIG_VAR(0x005ab4a0, int32_t, gFrames) =
        (dt_u32() & 1) ? dt_range(0, 60) : (int32_t)(dt_u32() & 0x3ffff) - 0x10000;
    ORIG_VAR(0x005ab4a4, int32_t, gAdjusting) = dt_range(0, 1);
    ORIG_VAR(0x005ab4a8, float, gStep) = rf();
    dt_stub(0x0045c920, NULL, (void *)stubSetQuality);
    if (dt_u32() % 4 != 0) {
        /* a measured rate around the 26 / 28 fps thresholds */
        static const float kRate[] = {5.0f,   24.0f, 25.0f, 25.5f, 25.99f, 26.0f,
                                      26.01f, 27.0f, 28.0f, 30.0f, 60.0f};
        const uint32_t elapsed = (uint32_t)dt_range(100, 1500);
        const float rate = kRate[dt_u32() % (sizeof kRate / sizeof kRate[0])];
        int frames = (int)(rate * (float)elapsed / 1000.0f + 0.5f) + dt_range(-1, 1);

        if (frames < 1) {
            frames = 1;
        }
        ORIG_VAR(0x005ab49c, uint32_t, gTimeB) = NOW - elapsed;
        ORIG_VAR(0x005ab4a0, int32_t, gFramesB) = frames - 1; /* the function counts this frame first */
        ORIG_VAR(0x0055bd08, float, gQualityB) =
            (dt_u32() & 1) ? 200.0f + dt_float(-15.0f, 15.0f) : dt_float(30.0f, 260.0f);
    }
}

static void *BMX_CDECL stubObjectModel(const void *object)
{
    ngLog(0x487440);
    ngLogPtr(object);
    return (void *)0x7700;
}

static int BMX_CDECL stubSetParam(int id, void *model, const float *value)
{
    ngLog(0x48ab10);
    ngLog((uint32_t)id);
    ngLogPtr(model);
    ngLogFloat(*value);
    return 1;
}

static void setup_set_quality(DtCtx *c)
{
    ngBegin(c, 0);
    ORIG_VAR(0x0068ff98, int32_t, gSceneObject) = (int32_t)dt_u32();
    dt_stub(0x00487440, NULL, (void *)stubObjectModel);
    dt_stub(0x0048ab10, NULL, (void *)stubSetParam);
    DT_ARG_FLOAT(c, 0, dt_float_edgy(0.0f, 300.0f));
}

/* ---- spawners -------------------------------------------------------------------------------------- */

static uint32_t spawnCount(void)
{
    return (dt_u32() % 6) | (dt_u32() & 0xffffff00u);
}

static void setupSpawn(DtCtx *c, int words)
{
    float *params = NULL;

    ngBegin(c, 0);
    if (dt_u32() % 4 != 0) {
        params = newParams(words);
    }
    DT_ARG_INT(c, 0, dt_u32() % 5 != 0 ? (dt_u32() | 1) : 0);
    DT_ARG_PTR(c, 1, newObject());
    DT_ARG_PTR(c, 2, dt_u32() % 8 == 0 ? NULL : newEmitter(dt_range(0, 3), 1));
    DT_ARG_INT(c, 3, spawnCount());
    DT_ARG_INT(c, 4, dt_u32());
    DT_ARG_PTR(c, 5, params);
}

static void setup_spark_spawn(DtCtx *c)
{
    setupSpawn(c, 16);
}

static void setup_shard_spawn(DtCtx *c)
{
    setupSpawn(c, 16);
    if (c->args[5] != 0) {
        float *p = (float *)(uintptr_t)c->args[5];

        p[8] = dt_float_edgy(-40000.0f, 40000.0f);
        p[9] = dt_float_edgy(-1.0e10f, 1.0e10f);
        p[10] = (dt_u32() % 7 == 0) ? 3.0e9f : dt_float_edgy(-70000.0f, 70000.0f);
    }
}

static void setup_dust_spawn(DtCtx *c)
{
    setupSpawn(c, 16);
    ORIG_VAR(0x0068fc20, uint32_t, gDust0) = dt_u32();
}

static void setup_poster_spawn(DtCtx *c)
{
    setupSpawn(c, 20);
}

static void setup_ripple_spawn(DtCtx *c)
{
    setupSpawn(c, 12);
}

static void setup_lightning_spawn(DtCtx *c)
{
    setupSpawn(c, 16);
    if (c->args[5] != 0) {
        float *p = (float *)(uintptr_t)c->args[5];

        rawWord(p, 8, (uint32_t)dt_range(0, 6));
        rawWord(p, 11, dt_u32());
    }
}

/* ---- renderers ------------------------------------------------------------------------------------- */

static void setupRender(DtCtx *c, int particles)
{
    ngBegin(c, 0);
    stubRenderer();
    DT_ARG_PTR(c, 0, newObject());
    DT_ARG_PTR(c, 1, newEmitter(particles, 1));
}

static void setup_render(DtCtx *c)
{
    setupRender(c, dt_range(0, 4));
}

static void setup_render_camera_dust(DtCtx *c)
{
    setupRender(c, dt_range(0, 4));
    dt_fill((void *)0x0068fc20, 64);
}

/* Line primitives (0x48ccf0): the texture and both end vertices. */
static void BMX_CDECL stubDrawLine(uint32_t flags, const uint32_t *desc, int count, int a, int b)
{
    ngLog(0x48ccf0);
    ngLog(flags);
    ngLog((uint32_t)count);
    ngLog((uint32_t)a);
    ngLog((uint32_t)b);
    ngLog(desc[0]);
    for (int k = 0; k < 2; k++) {
        const ZfxVertex *v = (const ZfxVertex *)(uintptr_t)desc[1 + k];

        ngLogFloat(v->x);
        ngLogFloat(v->y);
        ngLogFloat(v->z);
        ngLogFloat(v->u);
        ngLogFloat(v->v);
        ngLog(v->color);
    }
}

/*
 * Spark / lightning particles: some untextured (drawn as lines), some standing still or moving along a
 * single axis (zero offsets: a degenerate sweep direction, the lightning's winding test at exactly 0).
 */
static void setup_render_streak(DtCtx *c)
{
    ZfxEmitter *e;

    ngBegin(c, 0);
    stubRenderer();
    dt_stub(0x0048ccf0, NULL, (void *)stubDrawLine);
    if (dt_u32() % 4 == 0) {
        gMatrix.right = (ZVec3){1.0f, 0.0f, 0.0f};
        gMatrix.up = (ZVec3){0.0f, 1.0f, 0.0f};
        gMatrix.at = (ZVec3){0.0f, 0.0f, 1.0f};
    }
    e = newEmitter(0, 1);
    for (int i = dt_range(0, 5); i > 0; i--) {
        ZfxParticle *p = newParticle(e, 1);

        switch (dt_u32() % 6) {
        case 0:
            p->texture = 0;
            break;
        case 1:
            p->prev = p->pos;
            break;
        case 2:
            p->prev.x = p->pos.x;
            break;
        case 3:
            p->prev.y = p->pos.y;
            p->prev.z = p->pos.z;
            break;
        default:
            break;
        }
    }
    DT_ARG_PTR(c, 0, newObject());
    DT_ARG_PTR(c, 1, e);
}

/* ---- emitter ---------------------------------------------------------------------------------------- */

static void BMX_CDECL stubModifier(ZfxModifier *self, ZfxEmitter *emitter, void *arg)
{
    ngLog(0x1000 + (uint32_t)(uintptr_t)arg);
    ngLogPtr(self);
    ngLogPtr(emitter);
    ngLogFloat(emitter->min.x);
    ngLogFloat(emitter->max.z);
}

static void setup_update(DtCtx *c)
{
    ZfxEffect *effect = dt_alloc(sizeof *effect);
    ZfxModifierNode **tail = &effect->modifiers;

    ngBegin(c, 0);
    for (int i = dt_range(0, 3); i > 0; i--) {
        ZfxModifierNode *node = dt_alloc(sizeof *node);

        node->modifier = dt_alloc(sizeof(ZfxModifier));
        node->modifier->apply = stubModifier;
        node->arg = (void *)(uintptr_t)i;
        *tail = node;
        tail = &node->next;
    }
    DT_ARG_PTR(c, 0, effect);
    DT_ARG_PTR(c, 1, newEmitter(dt_range(0, 5), 1));
}

static void setup_poster_create(DtCtx *c)
{
    ngBegin(c, 0);
    DT_ARG_INT(c, 0, dt_u32());
    DT_ARG_INT(c, 1, (dt_u32() & 0xffff0000u) | (uint32_t)dt_range(0, 50));
}

/* ---- effect creators --------------------------------------------------------------------------------- */

static int sSpawnWords;

static void BMX_CDECL stubSpawn(uint32_t handle, const char *name, int flags, int unused,
                                const uint32_t *params)
{
    ngLog(0x495d50);
    ngLog(handle);
    ngLog(ngHash(name));
    ngLog((uint32_t)flags);
    ngLog((uint32_t)unused);
    ngLogPtr(params);
    for (int i = 0; params != NULL && i < sSpawnWords; i++) {
        ngLog(params[i]);
    }
}

static void setupCreate(DtCtx *c, int words, int withParams)
{
    float *params = NULL;

    ngBegin(c, 0);
    sSpawnWords = words;
    ORIG_VAR(0x005ab4e0, uint32_t, gSpark) = dt_u32();
    ORIG_VAR(0x005ab4e4, uint32_t, gSkid) = dt_u32();
    ORIG_VAR(0x005ab4e8, uint32_t, gRipple) = dt_u32();
    ORIG_VAR(0x005ab4ec, uint32_t, gSplash) = dt_u32();
    dt_stub(0x00495d50, NULL, (void *)stubSpawn);
    if (withParams && dt_u32() % 4 != 0) {
        params = newParams(words);
    }
    DT_ARG_INT(c, 0, dt_u32());
    DT_ARG_PTR(c, 1, params);
}

static void setup_create_spark(DtCtx *c)
{
    setupCreate(c, 16, 1);
}

static void setup_create_ripple(DtCtx *c)
{
    setupCreate(c, 12, 1);
}

static void setup_create_skid(DtCtx *c)
{
    setupCreate(c, 20, 1);
    if (c->args[1] != 0) {
        float *p = (float *)(uintptr_t)c->args[1];
        const uint32_t pick = dt_u32() % 4;

        rawWord(p, 0x12, pick == 0 ? 0 : (pick == 1 ? 1 : dt_u32()));
    }
}

static void setup_create_lightning(DtCtx *c)
{
    setupCreate(c, 12, 0);
}

/* ---- skid animation ---------------------------------------------------------------------------------- */

static uint32_t BMX_CDECL stubLoadTexture(int hash)
{
    ngLog(0x497400);
    ngLog((uint32_t)hash);
    return dt_u32() % 4 == 0 ? 0 : (uint32_t)hash * 7u + 1;
}

static uint32_t BMX_CDECL stubFindTexture(void *library, int hash)
{
    ngLog(0x488bd0);
    ngLogPtr(library);
    ngLog((uint32_t)hash);
    return dt_u32() % 3 == 0 ? 0 : (uint32_t)hash * 3u + (uint32_t)(uintptr_t)library;
}

static void *BMX_CDECL stubGetRaster(uint32_t material, int index)
{
    ngLog(0x489a40);
    ngLog(material);
    ngLog((uint32_t)index);
    return (void *)(uintptr_t)(material * 5u + 0x100);
}

static void BMX_CDECL stubSetRaster(uint32_t material, void *raster, int index)
{
    ngLog(0x489590);
    ngLog(material);
    ngLogPtr(raster);
    ngLog((uint32_t)index);
}

static void stubSlot(uint32_t slot, void *fn)
{
    dt_stub(*(uint32_t *)(uintptr_t)slot, NULL, fn);
}

static void setup_find_skids(DtCtx *c)
{
    int matches = 0;
    uint32_t *head = NULL;
    const int hash = zstrHash(ORIG_VAR(0x0055c11c, const char *, gSkidName));
    int total;

    ngBegin(c, 0);
    total = dt_range(0, 12);
    dt_fill(&gFxSkids, sizeof gFxSkids);
    for (int i = 0; i < total; i++) {
        uint32_t *node = dt_alloc(0x40);

        dt_fill(node, 0x40);
        node[0] = (uint32_t)(uintptr_t)head;
        node[2] = (dt_u32() % 3 == 0 && matches < 8) ? (uint32_t)hash : dt_u32();
        matches += node[2] == (uint32_t)hash;
        head = node;
    }
    *(void **)zobjTreeObjectList() = head;
    stubSlot(0x005620ec, (void *)stubLoadTexture);
    stubSlot(0x00560ee8, (void *)stubFindTexture);
}

static void setup_animate_skids(DtCtx *c)
{
    int used;

    ngBegin(c, 0);
    used = dt_range(0, 8);
    dt_fill(&gFxSkids, sizeof gFxSkids);
    for (int i = 0; i < kFxSkidEntries; i++) {
        ZfxSkid *skid = &gFxSkids[i];

        if (i >= used) {
            skid->object = NULL;
            continue;
        }
        skid->object = (void *)(uintptr_t)(0x1000 + i);
        skid->count = (uint8_t)dt_range(1, 16);
        skid->fps = (uint8_t)dt_range(1, 255);
        skid->lastTime = NOW - (uint32_t)(dt_u32() & 1 ? dt_range(0, 40) : dt_range(0, 2000));
    }
    stubSlot(0x00561034, (void *)stubGetRaster);
    stubSlot(0x0056102c, (void *)stubSetRaster);
}

/* ---- registration ------------------------------------------------------------------------------------ */

static void *BMX_CDECL stubFindModel(int flags, const char *name)
{
    ngLog(0x491410);
    ngLog((uint32_t)flags);
    ngLog(ngHash(name));
    return (void *)0xf00d;
}

static void BMX_CDECL stubSetVisible(int32_t object, int visible)
{
    ngLog(0x487600);
    ngLog((uint32_t)object);
    ngLog((uint32_t)visible);
}

static uint32_t BMX_CDECL stubTextureByHash(int hash, int flags)
{
    ngLog(0x489a00);
    ngLog((uint32_t)hash);
    ngLog((uint32_t)flags);
    return (uint32_t)hash ^ 0x5a5a;
}

static void *BMX_CDECL stubAddWorldObject(int flags)
{
    ngLog(0x491c50);
    ngLog((uint32_t)flags);
    return (void *)0xbeef;
}

static void BMX_CDECL stubAttach(ZfxEffect *effect, const char *name, const void *params)
{
    ngLog(0x495470);
    ngLogPtr(effect);
    ngLog(ngHash(name));
    ngLogPtr(params);
}

static int sFound;

static int BMX_CDECL stubFindSkids(void)
{
    ngLog(0x45f4a0);
    sFound = dt_range(0, 1);
    return sFound;
}

static void *BMX_CDECL stubRegister(int type, void *fn, void *arg, int append)
{
    ngLog(0x48ba30);
    ngLog((uint32_t)type);
    ngLogPtr(fn);
    ngLogPtr(arg);
    ngLog((uint32_t)append);
    return (void *)0x7777;
}

static void setup_register(DtCtx *c)
{
    ngBegin(c, 0);
    dt_stub(0x00491410, NULL, (void *)stubFindModel);
    dt_stub(0x00487600, NULL, (void *)stubSetVisible);
    dt_stub(0x00491c50, NULL, (void *)stubAddWorldObject);
    dt_stub(0x00495470, NULL, (void *)stubAttach);
    dt_stub(0x0045f4a0, NULL, (void *)stubFindSkids);
    dt_stub(0x0048ba30, NULL, (void *)stubRegister);
    stubSlot(0x0056104c, (void *)stubTextureByHash);
    ORIG_VAR(0x0068fcf8, int, gAnimHandle) = (dt_u32() & 1) ? 0 : (int)dt_u32();
}

static void BMX_CDECL stubDestroy(ZfxEffect *effect)
{
    ngLog(0x495380);
    ngLogPtr(effect);
}

static void BMX_CDECL stubUnregister(void *handle)
{
    ngLog(0x48bad0);
    ngLogPtr(handle);
}

static void setup_shutdown(DtCtx *c)
{
    ngBegin(c, 0);
    DT_ORIG(0x00482bd0, void (*)(void))();
    DT_ORIG(0x00482d00, void (*)(void *, const char *))((void *)0x1111, "x");
    DT_ORIG(0x00482df0, void (*)(void *, const char *))((void *)0x2222, "y");
    DT_ORIG(0x00482f20, void (*)(void *, const char *))((void *)0x3333, "z");
    dt_stub(0x00495380, NULL, (void *)stubDestroy);
    dt_stub(0x0048bad0, NULL, (void *)stubUnregister);
    ORIG_VAR(0x0068fcf8, int, gAnimHandle) = (dt_u32() & 1) ? 0 : (int)(dt_u32() | 1);
}

static void setup_node(DtCtx *c)
{
    ngBegin(c, 0);
    ORIG_VAR(0x0068ff80, uint32_t, gNode) = dt_u32();
    DT_ARG_INT(c, 0, dt_u32());
    DT_ARG_INT(c, 1, dt_u32());
}

static void setup_create(DtCtx *c)
{
    ngBegin(c, 0);
    DT_ARG_INT(c, 0, dt_range(0, 3));
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_INT(c, 2, dt_u32());
}

DT_CASES(ngiffxCases) = {
    {"ngiffxLoadAnimTable", 0x0045c690, ngiffxLoadAnimTable, 3, DT_RET_VOID, setup_load_anim, 100, 0},
    {"ngiffxGetDebugFloat", 0x0045c6d0, ngiffxGetDebugFloat, 0, DT_RET_FLOAT, setup_debug_float, 50, 0},
    {"ngiffxNop", 0x0045c6f0, ngiffxNop, 0, DT_RET_VOID, setup_none, 5, 0},
    {"ngiffxAdaptQuality", 0x0045c710, ngiffxAdaptQuality, 0, DT_RET_VOID, setup_adapt_quality, 1000, 0},
    {"ngiffxSetQuality", 0x0045c920, ngiffxSetQuality, 1, DT_RET_VOID, setup_set_quality, 100, 0},
    {"ngiffxSparkSpawn", 0x0045c950, ngiffxSparkSpawn, 6, DT_RET_VOID, setup_spark_spawn, 600, 0},
    {"ngiffxSparkRender", 0x0045cc50, ngiffxSparkRender, 2, DT_RET_INT, setup_render_streak, 1000, 0},
    {"ngiffxShardSpawn", 0x0045d260, ngiffxShardSpawn, 6, DT_RET_VOID, setup_shard_spawn, 600, 0},
    {"ngiffxShardRender", 0x0045d420, ngiffxShardRender, 2, DT_RET_INT, setup_render, 600, 0},
    {"ngiffxSplashRender", 0x0045d730, ngiffxSplashRender, 2, DT_RET_INT, setup_render, 600, 0},
    {"ngiffxDustSpawn", 0x0045da40, ngiffxDustSpawn, 6, DT_RET_VOID, setup_dust_spawn, 300, 0},
    {"ngiffxDustRender", 0x0045db20, ngiffxDustRender, 2, DT_RET_INT, setup_render_camera_dust, 600, 0},
    {"ngiffxPosterCreate", 0x0045de30, ngiffxPosterCreate, 2, DT_RET_INT, setup_poster_create, 100, 0},
    {"ngiffxPosterSpawn", 0x0045dea0, ngiffxPosterSpawn, 6, DT_RET_VOID, setup_poster_spawn, 300, 0},
    {"ngiffxPosterRender", 0x0045e010, ngiffxPosterRender, 2, DT_RET_INT, setup_render, 600, 0},
    {"ngiffxRippleSpawn", 0x0045e210, ngiffxRippleSpawn, 6, DT_RET_VOID, setup_ripple_spawn, 300, 0},
    {"ngiffxRippleRender", 0x0045e2d0, ngiffxRippleRender, 2, DT_RET_INT, setup_render, 600, 0},
    {"ngiffxUpdateEmitter", 0x0045e4e0, ngiffxUpdateEmitter, 2, DT_RET_INT, setup_update, 1000, 0},
    {"ngiffxLightningSpawn", 0x0045e750, ngiffxLightningSpawn, 6, DT_RET_VOID, setup_lightning_spawn, 600, 0},
    {"ngiffxLightningRender", 0x0045eaa0, ngiffxLightningRender, 2, DT_RET_INT, setup_render_streak, 1000, 0},
    {"ngiffxCreateSpark", 0x0045f030, ngiffxCreateSpark, 2, DT_RET_VOID, setup_create_spark, 100, 0},
    {"ngiffxCreateShard", 0x0045f0a0, ngiffxCreateShard, 2, DT_RET_VOID, setup_create_spark, 100, 0},
    {"ngiffxCreateSplash", 0x0045f0d0, ngiffxCreateSplash, 2, DT_RET_VOID, setup_create_spark, 100, 0},
    {"ngiffxCreateDust", 0x0045f140, ngiffxCreateDust, 2, DT_RET_VOID, setup_create_spark, 100, 0},
    {"ngiffxCreatePoster", 0x0045f190, ngiffxCreatePoster, 2, DT_RET_VOID, setup_create_spark, 100, 0},
    {"ngiffxCreateRipple", 0x0045f1e0, ngiffxCreateRipple, 2, DT_RET_VOID, setup_create_ripple, 100, 0},
    {"ngiffxCreateSkid", 0x0045f2d0, ngiffxCreateSkid, 2, DT_RET_VOID, setup_create_skid, 200, 0},
    {"ngiffxCreateLightning", 0x0045f340, ngiffxCreateLightning, 1, DT_RET_VOID, setup_create_lightning, 100,
     0},
    {"ngiffxGetNode", 0x0045f480, ngiffxGetNode, 2, DT_RET_INT, setup_node, 20, 0},
    {"ngiffxFindSkidObjects", 0x0045f4a0, ngiffxFindSkidObjects, 0, DT_RET_INT, setup_find_skids, 300, 0},
    {"ngiffxAnimateSkids", 0x0045f650, ngiffxAnimateSkids, 0, DT_RET_INT, setup_animate_skids, 400, 0},
    {"ngiffxRegisterHandlers", 0x0045f780, ngiffxRegisterHandlers, 0, DT_RET_VOID, setup_register, 100, 0},
    {"ngiffxShutdown", 0x0045fc10, ngiffxShutdown, 0, DT_RET_VOID, setup_shutdown, 100, 0},
    {"ngiffxCreate", 0x0045fcb0, ngiffxCreate, 3, DT_RET_INT, setup_create, 50, 0},
};
