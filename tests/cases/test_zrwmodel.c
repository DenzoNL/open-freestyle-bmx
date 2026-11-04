/*
 * Tests for the renderer model glue (zrwmodel.c). Everything RenderWare does is stubbed: the stubs log
 * their arguments (so call order and values are compared) and answer from the fake clumps, frames and
 * matrices of the fixtures, which are memory the harness compares after the run.
 */
#include <stdarg.h>

#include "dt.h"
#include "dt_ngif.h"
#include "zge/com/zglrw/zrwmodel.h"
#include "zge/zquat.h"

/* ---- stubs for the renderer ------------------------------------------------------------------------ */

enum {
    kStubForAll = 0x4b1990,
    kStubGeomGet = 0x4b6140,
    kStubGeomSet = 0x4b61b0,
    kStubGetWorld = 0x4b8870,
    kStubAdd = 0x4b86a0,
    kStubRemove = 0x4b8770,
    kStubGetMatrix = 0x4d2b00,
    kStubGetLtm = 0x4d2b10,
    kStubUpdateObjects = 0x4d2a80,
    kStubXformVec = 0x4d3ae0,
    kStubXformPts = 0x4d3a90,
    kStubMatUpdate = 0x4d04d0,
    kStubMatMul = 0x4d04f0,
    kStubMatScale = 0x4d1400,
    kStubMatCreate = 0x4d1fe0,
    kStubMatDestroy = 0x4d1fb0,
    kStubPlugin = 0x4b31e0,
    kStubNumMat = 0x4b4ce0,
    kStubGetMat = 0x4b47a0,
    kStubSetColor = 0x4b9310,
};

/* Answers of the init/shutdown calls and of the plugin registration, set per iteration. */
static int gCreateInit, gRenderInit, gCreateDown, gRenderDown, gPluginResult, gMatCreates;
static ZMatrix gMatPool[8];

static void BMX_CDECL zrwPrint(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    ngLog(0x480240);
    ngLog(ngHash(fmt));
    for (const char *p = fmt; *p; p++) {
        if (p[0] == '%' && p[1] == 's') {
            ngLog(ngHash(va_arg(ap, const char *)));
        } else if (p[0] == '%' && (p[1] == 'd' || p[1] == 'x')) {
            ngLog((uint32_t)va_arg(ap, int));
        }
    }
    va_end(ap);
}

static void *BMX_CDECL stubForAll(void *clump, void *cb, uint32_t *data)
{
    ngLog(kStubForAll);
    ngLogPtr(clump);
    ngLogPtr(cb);
    ngLog(data[0]);
    ngLog(data[1]);
    return clump;
}

static uint32_t BMX_CDECL stubGeomGet(void *geometry)
{
    ngLog(kStubGeomGet);
    ngLogPtr(geometry);
    return *(uint32_t *)geometry;
}

static void *BMX_CDECL stubGeomSet(void *geometry, uint32_t flags)
{
    ngLog(kStubGeomSet);
    ngLogPtr(geometry);
    ngLog(flags);
    *(uint32_t *)geometry = flags;
    return geometry;
}

/* A fake clump: +4 its frame, +0x10 the world it is in (0 for none), +0x14 a counter. */
static void *BMX_CDECL stubGetWorld(void *clump)
{
    ngLog(kStubGetWorld);
    ngLogPtr(clump);
    return *(void **)((uint8_t *)clump + 0x10);
}

static void *BMX_CDECL stubAdd(void *world, void *clump)
{
    ngLog(kStubAdd);
    ngLogPtr(world);
    ngLogPtr(clump);
    *(void **)((uint8_t *)clump + 0x10) = world;
    return clump;
}

static void *BMX_CDECL stubRemove(void *world, void *clump)
{
    ngLog(kStubRemove);
    ngLogPtr(world);
    ngLogPtr(clump);
    *(void **)((uint8_t *)clump + 0x10) = NULL;
    return clump;
}

/* A fake frame is 0x80 bytes: its matrix at +0, a second (world) matrix at +0x40. */
static void *BMX_CDECL stubGetMatrix(void *frame)
{
    ngLog(kStubGetMatrix);
    ngLogPtr(frame);
    return frame;
}

static void *BMX_CDECL stubGetLtm(void *frame)
{
    ngLog(kStubGetLtm);
    ngLogPtr(frame);
    return (uint8_t *)frame + 0x40;
}

static void *BMX_CDECL stubUpdateObjects(void *frame)
{
    ngLog(kStubUpdateObjects);
    ngLogPtr(frame);
    return frame;
}

static void logMatrix(const ZMatrix *m)
{
    const uint32_t *w = (const uint32_t *)m;

    for (int i = 0; i < 16; i++) {
        ngLog(w[i]);
    }
}

static void *BMX_CDECL stubXformVec(float *out, const float *in, int count, const float *m)
{
    ngLog(kStubXformVec);
    ngLogPtr(out);
    ngLogPtr(in);
    ngLog((uint32_t)count);
    ngLogPtr(m);
    for (int i = 0; i < 3; i++) {
        const float v = in[i] * m[i] + m[4 + i];
        ngLogFloat(in[i]);
        out[i] = v;
    }
    return out;
}

static void *BMX_CDECL stubXformPts(float *out, const float *in, int count, const float *m)
{
    ngLog(kStubXformPts);
    ngLogPtr(out);
    ngLogPtr(in);
    ngLog((uint32_t)count);
    ngLogPtr(m);
    for (int i = 0; i < 3; i++) {
        ngLogFloat(in[i]);
        out[i] = in[i] + m[12 + i];
    }
    return out;
}

static void *BMX_CDECL stubMatUpdate(ZMatrix *m)
{
    ngLog(kStubMatUpdate);
    ngLogPtr(m);
    logMatrix(m);
    return m;
}

static void *BMX_CDECL stubMatMul(float *out, const float *a, const float *b)
{
    ngLog(kStubMatMul);
    ngLogPtr(out);
    ngLogPtr(a);
    ngLogPtr(b);
    for (int i = 0; i < 16; i++) {
        out[i] = a[i] * 0.5f + b[i];
    }
    return out;
}

static void *BMX_CDECL stubMatScale(float *m, const float *scale, int combine)
{
    ngLog(kStubMatScale);
    ngLogPtr(m);
    ngLogVec((const ZVec3 *)scale);
    ngLog((uint32_t)combine);
    m[0] += scale[0];
    m[5] += scale[1];
    m[10] += scale[2];
    return m;
}

static ZMatrix *BMX_CDECL stubMatCreate(void)
{
    ngLog(kStubMatCreate);
    return &gMatPool[(gMatCreates++) & 7];
}

static int BMX_CDECL stubMatDestroy(ZMatrix *m)
{
    ngLog(kStubMatDestroy);
    ngLogPtr(m);
    return 1;
}

static int BMX_CDECL stubPlugin(int size, uint32_t id, void *ctor, void *dtor, void *copy)
{
    ngLog(kStubPlugin);
    ngLog((uint32_t)size);
    ngLog(id);
    ngLogPtr(ctor);
    ngLogPtr(dtor);
    ngLogPtr(copy);
    return gPluginResult;
}

/* A fake geometry: +0 flags, +4 the number of materials. */
static int BMX_CDECL stubNumMat(void *geometry)
{
    ngLog(kStubNumMat);
    ngLogPtr(geometry);
    return *(int *)((uint8_t *)geometry + 4);
}

static void *BMX_CDECL stubGetMat(void *geometry, int index)
{
    ngLog(kStubGetMat);
    ngLogPtr(geometry);
    ngLog((uint32_t)index);
    return (uint8_t *)geometry + 0x20 + index * 4;
}

static void *BMX_CDECL stubSetColor(void *material, const uint32_t *color)
{
    ngLog(kStubSetColor);
    ngLogPtr(material);
    ngLog(*color);
    *(uint32_t *)material = *color;
    return material;
}

/* Other renderer modules. */
static int BMX_CDECL stubCreateInit(void)
{
    ngLog(0x49d9d0);
    return gCreateInit;
}

static int BMX_CDECL stubCreateDown(void)
{
    ngLog(0x49da60);
    return gCreateDown;
}

static int BMX_CDECL stubRenderInit(void)
{
    ngLog(0x48fc50);
    return gRenderInit;
}

static int BMX_CDECL stubRenderDown(void)
{
    ngLog(0x48fd00);
    return gRenderDown;
}

/* The first atomic of a clump: a fake atomic whose +0x1c is the geometry (kept at +0x20 of the clump). */
static void *BMX_CDECL stubFirstAtomic(void *clump)
{
    ngLog(0x49cc70);
    ngLogPtr(clump);
    return *(void **)((uint8_t *)clump + 0x20);
}

static void BMX_CDECL stubMatrixToGame(float *out, const float *m)
{
    ngLog(0x48be30);
    ngLogPtr(out);
    ngLogPtr(m);
    for (int i = 0; i < 16; i++) {
        out[i] = m[i] + 1.0f;
    }
}

static void BMX_CDECL stubSetRenderCallback(void *model, void *cb)
{
    ngLog(0x48fec0);
    ngLogPtr(model);
    ngLogPtr(cb);
}

static void BMX_CDECL stubPostUpdate(void *clump)
{
    ngLog(0x504490);
    ngLogPtr(clump);
}

static void BMX_CDECL stubQuatToMatrix(const ZQuat *q, float *out)
{
    ngLog(0x487e70);
    ngLogPtr(out);
    ngLogFloat(q->x);
    ngLogFloat(q->y);
    ngLogFloat(q->z);
    ngLogFloat(q->w);
    for (int i = 0; i < 12; i++) {
        out[i] = q->x * (float)(i + 1) + q->w;
    }
}

static void BMX_CDECL stubQuatToMatrixMirrored(const ZQuat *q, float *out)
{
    ngLog(0x487fc0);
    ngLogPtr(out);
    ngLogFloat(q->x);
    ngLogFloat(q->y);
    ngLogFloat(q->z);
    ngLogFloat(q->w);
    for (int i = 0; i < 12; i++) {
        out[i] = q->y * (float)(i + 1) - q->w;
    }
}

static void *BMX_CDECL stubMaterialTexture(void *material, int index)
{
    ngLog(0x489a40);
    ngLogPtr(material);
    ngLog((uint32_t)index);
    return material;
}

static int BMX_CDECL stubTextureHash(void *texture)
{
    ngLog(0x4988b0);
    ngLogPtr(texture);
    return (int)(uintptr_t)texture & 0xff;
}

/* The model callbacks of the object wrappers. */
static int BMX_CDECL stubVt1(void *model)
{
    ngLog(0xa0001);
    ngLogPtr(model);
    return (int)(uintptr_t)model + 7;
}

static int BMX_CDECL stubVt2(void *model, const void *arg)
{
    ngLog(0xa0002);
    ngLogPtr(model);
    ngLogPtr(arg);
    return 3;
}

static void *BMX_CDECL stubVtLookup(void *model, const void *key)
{
    ngLog(0xa0003);
    ngLogPtr(model);
    ngLogPtr(key);
    return (uint8_t *)model + 0x30;
}

static int BMX_CDECL stubVtFlags(void *model, int clear, int set)
{
    ngLog(0xa0004);
    ngLogPtr(model);
    ngLog((uint32_t)clear);
    ngLog((uint32_t)set);
    return 5;
}

static void BMX_CDECL stubOnUpdate(ZrwModel *model)
{
    ngLog(0xa0005);
    ngLogPtr(model);
    ngLog(model->flags);
}

/* Finite floats: random bits would make NaNs, whose x87 and SSE propagation differs (and never occurs). */
static void fill_floats(void *p, uint32_t bytes)
{
    float *f = p;

    for (uint32_t i = 0; i < bytes / 4; i++) {
        f[i] = dt_float_edgy(-100.0f, 100.0f);
    }
}

static void zrwEnv(const DtCtx *c, int stubFail)
{
    ngBegin(c, stubFail);
    gZdebugPrint = zrwPrint;
    gCreateInit = (dt_u32() % 5) != 0;
    gRenderInit = (dt_u32() % 5) != 0;
    gCreateDown = (dt_u32() % 5) != 0;
    gRenderDown = (dt_u32() % 5) != 0;
    gMatCreates = 0;
    gPluginResult = (dt_u32() % 4 == 0) ? -1 : dt_range(0, 12);
    for (int i = 0; i < 8; i++) {
        fill_floats(&gMatPool[i], sizeof gMatPool[i]);
    }
    dt_stub(kStubForAll, NULL, (void *)stubForAll);
    dt_stub(kStubGeomGet, NULL, (void *)stubGeomGet);
    dt_stub(kStubGeomSet, NULL, (void *)stubGeomSet);
    dt_stub(kStubGetWorld, NULL, (void *)stubGetWorld);
    dt_stub(kStubAdd, NULL, (void *)stubAdd);
    dt_stub(kStubRemove, NULL, (void *)stubRemove);
    dt_stub(kStubGetMatrix, NULL, (void *)stubGetMatrix);
    dt_stub(kStubGetLtm, NULL, (void *)stubGetLtm);
    dt_stub(kStubUpdateObjects, NULL, (void *)stubUpdateObjects);
    dt_stub(kStubXformVec, NULL, (void *)stubXformVec);
    dt_stub(kStubXformPts, NULL, (void *)stubXformPts);
    dt_stub(kStubMatUpdate, NULL, (void *)stubMatUpdate);
    dt_stub(kStubMatMul, NULL, (void *)stubMatMul);
    dt_stub(kStubMatScale, NULL, (void *)stubMatScale);
    dt_stub(kStubMatCreate, NULL, (void *)stubMatCreate);
    dt_stub(kStubMatDestroy, NULL, (void *)stubMatDestroy);
    dt_stub(kStubPlugin, NULL, (void *)stubPlugin);
    dt_stub(kStubNumMat, NULL, (void *)stubNumMat);
    dt_stub(kStubGetMat, NULL, (void *)stubGetMat);
    dt_stub(kStubSetColor, NULL, (void *)stubSetColor);
    dt_stub(0x49d9d0, NULL, (void *)stubCreateInit);
    dt_stub(0x49da60, NULL, (void *)stubCreateDown);
    dt_stub(0x48fc50, NULL, (void *)stubRenderInit);
    dt_stub(0x48fd00, NULL, (void *)stubRenderDown);
    dt_stub(0x49cc70, NULL, (void *)stubFirstAtomic);
    dt_stub(0x48be30, NULL, (void *)stubMatrixToGame);
    dt_stub(0x48fec0, NULL, (void *)stubSetRenderCallback);
    dt_stub(0x504490, NULL, (void *)stubPostUpdate);
    gZrwMaterialTexture = stubMaterialTexture;
    gZrwTextureHash = stubTextureHash;
    gZrwVtDestroy = stubVt1;
    gZrwVtUpdate = stubVt1;
    gZrwVtSetPos = stubVt2;
    gZrwVtSetRot = stubVt2;
    gZrwVtSetScale = stubVt2;
    gZrwVtKey = (ZrwModelKeyFn)(void *)stubVtLookup;
    gZrwVtEntry = (ZrwModelLookup)stubVtLookup;
    gZrwVtSetFlags = stubVtFlags;
    gZrwVtClump = (ZrwModelClumpFn)(void *)stubVt1;
}

/* ---- fixtures ------------------------------------------------------------------------------------- */

static float rf(void)
{
    return dt_float_edgy(-1000.0f, 1000.0f);
}

static uint32_t small_hash(void)
{
    return 0x100 + dt_u32() % 5;
}

static void *make_frame(void)
{
    void *frame = dt_alloc(0x80);

    fill_floats(frame, 0x80);
    return frame;
}

/* Fake clump: +4 frame, +0x10 world (random: in the world or not), +0x20 a fake atomic. */
static void *make_clump(void)
{
    uint8_t *clump = dt_alloc(0x40);
    uint8_t *atomic = dt_alloc(0x40);
    uint8_t *geometry = dt_alloc(0x60);

    dt_fill(clump, 0x40);
    *(void **)(clump + 4) = make_frame();
    *(void **)(clump + 0x10) = (dt_u32() & 1) ? (void *)gZrwWorld : NULL;
    *(void **)(clump + 0x20) = atomic;
    dt_fill(atomic, 0x40);
    *(void **)(atomic + 0x1c) = geometry;
    dt_fill(geometry, 0x60);
    *(int *)(geometry + 4) = dt_range(0, 6);
    return clump;
}

static void random_quat(ZQuat *q)
{
    q->x = rf();
    q->y = rf();
    q->z = rf();
    q->w = rf();
    if (dt_u32() % 12 == 0) {
        q->x = q->y = q->z = q->w = 0.0f; /* asserts in zquatNormalize */
    }
}

static void random_vec(ZVec3 *v)
{
    v->x = rf();
    v->y = rf();
    v->z = rf();
}

static void random_world(void)
{
    gZrwWorld = (dt_u32() % 8 == 0) ? NULL : (void *)(uintptr_t)(0x700000 + dt_u32() % 16);
    gZrwScale = dt_float(
        0.001f, 20.0f); /* never 0 or inf: 0 * inf is a NaN whose sign depends on the order of the negation */
    gZrwInvScale = (dt_u32() & 1) ? 1.0f / gZrwScale : dt_float(0.001f, 20.0f);
    random_vec(&gZrwWorldCenter);
    gZrwFlags = (uint8_t)dt_u32();
    random_vec(&gZrwOrigin);
}

static ZrwModelDef *make_def(int sets)
{
    ZrwModelDef *def = dt_alloc(sizeof *def);
    ZrwModelEntry *entries;

    def->flags = dt_u32();
    def->options = dt_u32();
    def->lodCount = (uint16_t)sets;
    def->defaultKey = dt_u32();
    for (int i = 0; i < 20; i++) { /* runs on into the path: the name need not be terminated */
        ((char *)def + offsetof(ZrwModelDef, name))[i] = (char)dt_range('a', 'z');
    }
    def->entryCount = (uint16_t)dt_range(0, 6);
    entries = dt_alloc(sizeof *entries * 8);
    for (int i = 0; i < 8; i++) {
        dt_fill(&entries[i], sizeof entries[i]);
        entries[i].key = (int32_t)small_hash();
    }
    def->entries = entries;
    return def;
}

static ZrwModelPart *make_parts(int count)
{
    ZrwModelPart *parts = dt_alloc(sizeof *parts * (uint32_t)(count + 1));

    for (int i = 0; i < count; i++) {
        dt_fill(&parts[i], sizeof parts[i]);
        parts[i].hash = small_hash();
        parts[i].frame = make_frame();
        parts[i].def = NULL;
        random_vec((ZVec3 *)parts[i].restPos);
    }
    return parts;
}

static ZrwPartDefTable *make_defs(void)
{
    ZrwPartDefTable *table = dt_alloc(sizeof *table);
    const int count = dt_range(0, 5);

    table->count = (uint32_t)count;
    table->defs = dt_alloc(sizeof(ZrwPartDef) * 6);
    for (int i = 0; i < 6; i++) {
        ZrwPartDef *d = &table->defs[i];

        dt_fill(d, sizeof *d);
        d->hash = small_hash();
        random_vec((ZVec3 *)d->offset);
        random_quat(&d->rotation);
    }
    return table;
}

static ZList *make_materials(void)
{
    ZList *list = dt_alloc(sizeof *list);
    const int count = dt_range(0, 6);

    list->items = (void **)dt_alloc(sizeof(void *) * 8);
    for (int i = 0; i < count; i++) {
        list->items[i] = (void *)(uintptr_t)(0x100 + dt_u32() % 7);
    }
    list->count = count;
    list->capacity = 8;
    list->cursor = dt_range(-1, count);
    return list;
}

/* A model with up to three part sets, one or two clumps, a few entries and a material list. */
static ZrwModel *make_model(void)
{
    ZrwModel *model = dt_alloc(sizeof *model);
    const int sets = dt_range(0, 3);

    dt_fill(model, sizeof *model);
    model->flags = dt_u32() & 3;
    if (dt_u32() % 8 == 0) {
        model->flags = dt_u32();
    }
    model->clump = make_clump();
    model->collisionClump = (dt_u32() % 3 == 0) ? NULL : make_clump();
    random_vec(&model->position);
    random_vec(&model->scale);
    if (dt_u32() % 3 == 0) {
        model->scale.x = model->scale.y = model->scale.z = 1.0f;
    }
    random_quat(&model->rotation);
    model->activeLod = (uint16_t)(sets > 0 ? dt_range(0, sets - 1) : 0);
    model->materials = make_materials();
    for (int i = 0; i < 3; i++) {
        const int count = dt_range(0, 4);

        model->lods[i].partCount = (uint32_t)count;
        model->lods[i].parts = make_parts(count);
        dt_fill(&model->lods[i].distance, sizeof model->lods[i].distance);
    }
    model->onUpdate = (dt_u32() & 1) ? stubOnUpdate : NULL;
    model->def = make_def(sets);
    if (dt_u32() & 1) {
        for (int i = 0; i < 3; i++) {
            for (uint32_t k = 0; k < model->lods[i].partCount; k++) {
                ZrwPartDefTable *t = make_defs();

                model->lods[i].parts[k].def = &t->defs[dt_range(0, 5)];
            }
        }
    }
    return model;
}

static ZrwModelPart *pick_part(const ZrwModel *model)
{
    const ZrwModelLod *set = &model->lods[0];

    if (set->partCount == 0 || dt_u32() % 8 == 0) {
        return make_parts(1);
    }
    return &set->parts[dt_u32() % set->partCount];
}

static void begin(DtCtx *c, int stubFail)
{
    zrwEnv(c, stubFail);
    random_world();
    gZrwModelState.initialized = dt_u32() & 1;
    gZrwModelState.matrix = (ZMatrix *)make_frame();
    gZrwModelState.scratch = (ZMatrix *)make_frame();
    gZrwMdlPluginOffset = (dt_u32() % 6 == 0) ? dt_range(-1, 0) : dt_range(1, 8) * 4;
}

/* ---- setups --------------------------------------------------------------------------------------- */

static void setup_find_entry(DtCtx *c)
{
    ZrwModel *model;

    begin(c, 1);
    model = make_model();
    DT_ARG_PTR(c, 0, model);
    DT_ARG_INT(c, 1, small_hash());
}

static void setup_atomic_mask(DtCtx *c)
{
    uint8_t *atomic = dt_alloc(0x40);
    uint32_t *mask = dt_alloc(8);

    begin(c, 1);
    dt_fill(atomic, 0x40);
    mask[0] = dt_u32();
    mask[1] = dt_u32();
    DT_ARG_PTR(c, 0, atomic);
    DT_ARG_PTR(c, 1, mask);
}

static void setup_geom_mask(DtCtx *c)
{
    uint8_t *atomic = dt_alloc(0x40);
    uint32_t *mask = dt_alloc(8);
    uint32_t *geometry = dt_alloc(0x20);

    begin(c, 1);
    dt_fill(atomic, 0x40);
    dt_fill(geometry, 0x20);
    *(void **)(atomic + 0x1c) = geometry;
    mask[0] = dt_u32();
    mask[1] = dt_u32();
    DT_ARG_PTR(c, 0, atomic);
    DT_ARG_PTR(c, 1, mask);
}

static void setup_clump_mask(DtCtx *c)
{
    begin(c, 1);
    DT_ARG_PTR(c, 0, make_clump());
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_INT(c, 2, dt_u32());
}

static void setup_model_int(DtCtx *c)
{
    begin(c, 1);
    DT_ARG_PTR(c, 0, make_model());
    DT_ARG_INT(c, 1, dt_u32() % 3 == 0 ? 0 : dt_u32());
}

static void setup_plugin_ctor(DtCtx *c)
{
    uint8_t *clump = dt_alloc(0x60);

    begin(c, 1);
    dt_fill(clump, 0x60);
    DT_ARG_PTR(c, 0, clump);
}

static void setup_plugin_set(DtCtx *c)
{
    setup_plugin_ctor(c);
    DT_ARG_INT(c, 1, dt_u32());
}

static void setup_quat(DtCtx *c)
{
    ZQuat *q = dt_alloc(sizeof *q);

    begin(c, 1);
    random_quat(q);
    DT_ARG_PTR(c, 0, q);
    DT_ARG_PTR(c, 1, make_frame());
}

static void setup_noargs(DtCtx *c)
{
    begin(c, 1);
}

static void setup_match(DtCtx *c)
{
    ZrwModel *model;

    begin(c, 1);
    model = make_model();
    DT_ARG_PTR(c, 0, model);
    DT_ARG_PTR(c, 1, make_defs());
}

static void setup_model(DtCtx *c)
{
    begin(c, 1);
    DT_ARG_PTR(c, 0, make_model());
}

static void setup_model_out(DtCtx *c)
{
    begin(c, 1);
    DT_ARG_PTR(c, 0, make_model());
    DT_ARG_PTR(c, 1, dt_alloc(0x40));
}

static void setup_find_part(DtCtx *c)
{
    ZrwModel *model;

    begin(c, 1);
    model = make_model();
    DT_ARG_PTR(c, 0, model);
    DT_ARG_INT(c, 1, small_hash());
}

static void setup_part_to_world(DtCtx *c)
{
    ZrwModel *model;
    ZVec3 *local = dt_alloc(sizeof *local);

    begin(c, 1);
    model = make_model();
    random_vec(local);
    DT_ARG_PTR(c, 0, model);
    DT_ARG_PTR(c, 1, pick_part(model));
    DT_ARG_PTR(c, 2, local);
    DT_ARG_PTR(c, 3, dt_alloc(0x20));
    DT_ARG_INT(c, 4, dt_u32());
}

static void setup_part_offset(DtCtx *c)
{
    ZrwModel *model;

    begin(c, 1);
    model = make_model();
    DT_ARG_PTR(c, 0, model);
    DT_ARG_PTR(c, 1, pick_part(model));
    DT_ARG_PTR(c, 2, dt_alloc(0x20));
}

static void setup_part_matrix(DtCtx *c)
{
    ZrwModel *model;

    begin(c, 1);
    model = make_model();
    DT_ARG_PTR(c, 0, model);
    DT_ARG_PTR(c, 1, pick_part(model));
    DT_ARG_PTR(c, 2, dt_alloc(0x80));
    DT_ARG_INT(c, 3, dt_u32());
}

static void setup_part_user(DtCtx *c)
{
    ZrwModel *model;

    begin(c, 1);
    model = make_model();
    DT_ARG_PTR(c, 0, model);
    DT_ARG_PTR(c, 1, pick_part(model));
}

static void setup_key(DtCtx *c)
{
    uint32_t *key = dt_alloc(8);

    begin(c, 1);
    *key = small_hash();
    DT_ARG_PTR(c, 0, make_model());
    DT_ARG_PTR(c, 1, (dt_u32() % 4 == 0) ? NULL : key);
}

static void setup_set_vec(DtCtx *c)
{
    float *v = dt_alloc(16);

    begin(c, 1);
    v[0] = dt_float_edgy(-2.0f, 2.0f);
    v[1] = dt_float_edgy(-2.0f, 2.0f);
    v[2] = dt_float_edgy(-2.0f, 2.0f);
    v[3] = dt_float_edgy(-2.0f, 2.0f);
    DT_ARG_PTR(c, 0, make_model());
    DT_ARG_PTR(c, 1, v);
}

static void setup_set_scale(DtCtx *c)
{
    float *v = dt_alloc(16);

    begin(c, 1);
    for (int i = 0; i < 3; i++) {
        switch (dt_u32() % 5) {
        case 0:
            v[i] = 1.0f;
            break;
        case 1:
            v[i] = 1.0f + dt_float(-3e-5f, 3e-5f);
            break;
        case 2:
            v[i] = (dt_u32() % 9 == 0) ? __builtin_nanf("") : dt_float_edgy(-5.0f, 5.0f);
            break;
        default:
            v[i] = 1.0f + dt_float(-1e-5f, 1e-5f);
            break;
        }
    }
    DT_ARG_PTR(c, 0, make_model());
    DT_ARG_PTR(c, 1, v);
}

/* The update replaces the two quaternion conversions by stubs. */
static void setup_update(DtCtx *c)
{
    begin(c, 1);
    dt_stub(0x487e70, NULL, (void *)stubQuatToMatrix);
    dt_stub(0x487fc0, NULL, (void *)stubQuatToMatrixMirrored);
    DT_ARG_PTR(c, 0, make_model());
}

static void setup_set_flags(DtCtx *c)
{
    begin(c, 1);
    DT_ARG_PTR(c, 0, make_model());
    DT_ARG_INT(c, 1, dt_u32() % 6 == 0 ? 0 : dt_u32());
    DT_ARG_INT(c, 2, dt_u32() % 6 == 0 ? 0 : dt_u32());
}

static void setup_user_data(DtCtx *c)
{
    uint8_t *object = dt_alloc(0x60);

    begin(c, 1);
    dt_fill(object, 0x60);
    *(void **)(object + 0x40) = dt_alloc(0x60);
    dt_fill(*(void **)(object + 0x40), 0x60);
    DT_ARG_PTR(c, 0, object);
}

static void setup_model_value(DtCtx *c)
{
    begin(c, 1);
    DT_ARG_PTR(c, 0, make_model());
    DT_ARG_INT(c, 1, dt_u32());
}

static void setup_find_material(DtCtx *c)
{
    begin(c, 1);
    DT_ARG_PTR(c, 0, make_model());
    DT_ARG_INT(c, 1, 0x100 + dt_u32() % 8);
}

static void setup_get_material(DtCtx *c)
{
    begin(c, 1);
    DT_ARG_PTR(c, 0, make_model());
    DT_ARG_INT(c, 1, dt_u32() % 10 == 0 ? dt_u32() : (uint32_t)dt_range(0, 7));
}

static void setup_update_hook(DtCtx *c)
{
    uint32_t *arg = dt_alloc(8);

    begin(c, 1);
    *arg = dt_u32();
    DT_ARG_PTR(c, 0, make_model());
    DT_ARG_PTR(c, 1, arg);
}

static void setup_color(DtCtx *c)
{
    uint32_t *color = dt_alloc(8);

    begin(c, 1);
    *color = dt_u32();
    DT_ARG_PTR(c, 0, make_model());
    DT_ARG_PTR(c, 1, color);
}

/* The wrappers take an object whose +0x2c is the model. */
static uint8_t *make_object(void)
{
    uint8_t *object = dt_alloc(0x60);

    dt_fill(object, 0x60);
    *(void **)(object + 0x2c) = (dt_u32() % 6 == 0) ? NULL : dt_alloc(0x60);
    return object;
}

static void setup_object(DtCtx *c)
{
    begin(c, 1);
    DT_ARG_PTR(c, 0, make_object());
}

static void setup_object_arg(DtCtx *c)
{
    begin(c, 1);
    DT_ARG_PTR(c, 0, make_object());
    DT_ARG_PTR(c, 1, dt_alloc(0x20));
}

static void setup_object_int(DtCtx *c)
{
    begin(c, 1);
    DT_ARG_PTR(c, 0, make_object());
    DT_ARG_INT(c, 1, dt_u32() % 3 == 0 ? 0 : dt_u32());
}

/* An object for the destroy wrapper needs a model (the skeleton owner slot at +0x30 is left empty). */
static void setup_destroy(DtCtx *c)
{
    uint8_t *object;

    begin(c, 1);
    object = make_object();
    *(void **)(object + 0x30) = NULL;
    DT_ARG_PTR(c, 0, object);
}

DT_CASES(zrwmodel_cases) = {
    {"zrwModelFindEntry", 0x00487ad0, zrwModelFindEntry, 2, DT_RET_INT, setup_find_entry, 400, 0},
    {"zrwAtomicMaskFlags", 0x00487b60, zrwAtomicMaskFlags, 2, DT_RET_INT, setup_atomic_mask, 300, 0},
    {"zrwClumpMaskAtomicFlags", 0x00487b90, zrwClumpMaskAtomicFlags, 3, DT_RET_VOID, setup_clump_mask, 300,
     0},
    {"zrwAtomicMaskGeometryFlags", 0x00487bc0, zrwAtomicMaskGeometryFlags, 2, DT_RET_INT, setup_geom_mask,
     300, 0},
    {"zrwClumpMaskGeometryFlags", 0x00487bf0, zrwClumpMaskGeometryFlags, 3, DT_RET_VOID, setup_clump_mask,
     300, 0},
    {"zrwModelSetInWorld", 0x00487c20, zrwModelSetInWorld, 2, DT_RET_VOID, setup_model_int, 600, 0},
    {"zrwModelSetCollision", 0x00487cd0, zrwModelSetCollision, 2, DT_RET_VOID, setup_model_int, 600, 0},
    {"zrwMdlConstructor", 0x00487d90, zrwMdlConstructor, 1, DT_RET_INT, setup_plugin_ctor, 200, 0},
    {"zrwMdlDestructor", 0x00487db0, zrwMdlDestructor, 1, DT_RET_INT, setup_plugin_ctor, 50, 0},
    {"zrwMdlSetUserData", 0x00487dc0, zrwMdlSetUserData, 2, DT_RET_INT, setup_plugin_set, 200, 0},
    {"zrwQuatToMatrix", 0x00487e70, zrwQuatToMatrix, 2, DT_RET_VOID, setup_quat, 2000, 0},
    {"zrwQuatToMatrixMirrored", 0x00487fc0, zrwQuatToMatrixMirrored, 2, DT_RET_VOID, setup_quat, 2000, 0},
    {"zrwModelSystemInitialize", 0x00488190, zrwModelSystemInitialize, 0, DT_RET_INT, setup_noargs, 200, 0},
    {"zrwModelSystemShutdown", 0x00488200, zrwModelSystemShutdown, 0, DT_RET_INT, setup_noargs, 200, 0},
    {"zrwModelMatchParts", 0x00488250, zrwModelMatchParts, 2, DT_RET_VOID, setup_match, 600, 0},
    {"zrwModelPartCount", 0x00488310, zrwModelPartCount, 1, DT_RET_INT, setup_model, 100, 0},
    {"zrwModelGetPartHashes", 0x00488320, zrwModelGetPartHashes, 2, DT_RET_VOID, setup_model_out, 200, 0},
    {"zrwModelFindPart", 0x00488350, zrwModelFindPart, 2, DT_RET_INT, setup_find_part, 400, 0},
    {"zrwModelPartToWorld", 0x004883a0, zrwModelPartToWorld, 5, DT_RET_VOID, setup_part_to_world, 2000, 0},
    {"zrwModelPartOffset", 0x00488480, zrwModelPartOffset, 3, DT_RET_VOID, setup_part_offset, 1000, 0},
    {"zrwModelPartMatrix", 0x004884b0, zrwModelPartMatrix, 4, DT_RET_VOID, setup_part_matrix, 300, 0},
    {"zrwModelPartParent", 0x004884f0, zrwModelPartParent, 2, DT_RET_INT, setup_part_user, 100, 0},
    {"zrwModelEntryKey", 0x00488500, zrwModelEntryKey, 2, DT_RET_INT, setup_key, 200, 0},
    {"zrwModelEntryLookup", 0x00488520, zrwModelEntryLookup, 2, DT_RET_INT, setup_key, 400, 0},
    {"zrwModelSetPosition", 0x00488580, zrwModelSetPosition, 2, DT_RET_INT, setup_set_vec, 100, 0},
    {"zrwModelSetRotation", 0x004885a0, zrwModelSetRotation, 2, DT_RET_INT, setup_set_vec, 100, 0},
    {"zrwModelSetScale", 0x004885d0, zrwModelSetScale, 2, DT_RET_INT, setup_set_scale, 1000, 0},
    {"zrwModelUpdate", 0x00488650, zrwModelUpdate, 1, DT_RET_INT, setup_update, 1500, 0},
    {"zrwMdlRegisterPlugin", 0x00488890, zrwMdlRegisterPlugin, 0, DT_RET_INT, setup_noargs, 200, 0},
    {"zrwModelSetFlags", 0x004888c0, zrwModelSetFlags, 3, DT_RET_INT, setup_set_flags, 2000, 0},
    {"zrwMdlGetUserData", 0x00488b80, zrwMdlGetUserData, 1, DT_RET_INT, setup_user_data, 200, 0},
    {"zrwModelSetUserValue", 0x00488ba0, zrwModelSetUserValue, 2, DT_RET_VOID, setup_model_value, 100, 0},
    {"zrwModelGetUserValue", 0x00488bb0, zrwModelGetUserValue, 1, DT_RET_INT, setup_model, 100, 0},
    {"zrwModelCollisionClump", 0x00488bc0, zrwModelCollisionClump, 1, DT_RET_INT, setup_model, 100, 0},
    {"zrwModelFindMaterial", 0x00488bd0, zrwModelFindMaterial, 2, DT_RET_INT, setup_find_material, 600, 0},
    {"zrwModelMaterialCount", 0x00488c30, zrwModelMaterialCount, 1, DT_RET_INT, setup_model, 100, 0},
    {"zrwModelGetMaterial", 0x00488c50, zrwModelGetMaterial, 2, DT_RET_INT, setup_get_material, 600, 0},
    {"zrwModelSetDrawColor", 0x00488ca0, zrwModelSetDrawColor, 2, DT_RET_VOID, setup_update_hook, 100, 0},
    {"zrwModelSetColor", 0x00488cc0, zrwModelSetColor, 2, DT_RET_VOID, setup_color, 400, 0},
    {"zrwObjDestroy", 0x00488d30, zrwObjDestroy, 1, DT_RET_INT, setup_destroy, 200, 0},
    {"zrwObjUpdate", 0x00488d60, zrwObjUpdate, 1, DT_RET_INT, setup_object, 100, 0},
    {"zrwObjSetPosition", 0x00488d80, zrwObjSetPosition, 2, DT_RET_VOID, setup_object_arg, 100, 0},
    {"zrwObjSetRotation", 0x00488da0, zrwObjSetRotation, 2, DT_RET_VOID, setup_object_arg, 100, 0},
    {"zrwObjSetScale", 0x00488dc0, zrwObjSetScale, 2, DT_RET_VOID, setup_object_arg, 100, 0},
    {"zrwObjSetShown", 0x00488de0, zrwObjSetShown, 2, DT_RET_INT, setup_object_int, 100, 0},
    {"zrwObjEntryKey", 0x00488e20, zrwObjEntryKey, 1, DT_RET_INT, setup_object, 100, 0},
    {"zrwObjClump", 0x00488e40, zrwObjClump, 1, DT_RET_INT, setup_object, 100, 0},
    {"zrwObjEntry", 0x00488e60, zrwObjEntry, 1, DT_RET_INT, setup_object, 100, 0},
    {"zrwObjUpdateAgain", 0x00488e80, zrwObjUpdateAgain, 1, DT_RET_INT, setup_object, 100, 0},
};
