/*
 * Tests for zrwmodelcreate.c: the roller's shadow sampler, the two decompressors, the frame and clump helpers
 * and the model loader. The RenderWare library and the IFF reader are stubs that log their arguments.
 */
#include "dt.h"
#include "dt_s5.h"
#include "dt_zmem.h"
#include "zge/com/zglrw/zrwcol.h"
#include "zge/com/zglrw/zrwmodelcreate.h"
#include "zge/com/zglrw/zrwprim.h"
#include "zge/com/zglrw/zrwshadow.h"
#include "zge/zasset.h"
#include "zge/zlist.h"
#include "zge/zmem.h"
#include "zge/zobj.h"

typedef void *(BMX_CDECL *FrameCb)(void *frame, void *data);

/* ---- generic stubs --------------------------------------------------------------------------------- */

S5_STUB1(stubUnregister, 0x48bad0)
S5_STUB1(stubFrameCount, 0x4d2f00)
S5_STUB1(stubSetIdentity, 0x4d3200)
S5_STUB2(stubSetUserData, 0x500230)
S5_STUB1(stubNumAtomics, 0x4b1440)
S5_STUB1(stubClumpDestroy, 0x4b21e0)
S5_STUB1(stubStreamRead, 0x4b2790)
S5_STUB1(stubImageSetPath, 0x4e4420)
S5_STUB3(stubStreamOpen, 0x4e23e0)
S5_STUB2(stubStreamClose, 0x4e2660)
S5_STUB2(stubWorldAdd, 0x4b86a0)
S5_STUB2(stubWorldRemove, 0x4b8770)
S5_STUB1(stubClumpWorld, 0x4b8870)
S5_STUB1(stubSkin, 0x505950)
S5_STUB2(stubMdlUserData, 0x487dc0)
S5_STUB2(stubSetTexture, 0x489b80)
S5_STUB1(stubRetain, 0x489af0)
S5_STUB2(stubSetInWorld, 0x487c20)
S5_STUB2(stubSetActive, 0x487cd0)
S5_STUB3(stubMaskGeometry, 0x487bf0)
S5_STUB3(stubMaskAtomic, 0x487b90)
S5_STUB1(stubModelUpdate, 0x488650)
S5_STUB1(stubDefaultRender, 0x48fee0)
S5_STUB1(stubRenderRegister, 0x48ff00)
S5_STUB0(stubRenderUnregister, 0x48ff30)
S5_STUB1(stubReleaseTexture, 0x489530)
S5_STUB1(stubTextureRelease, 0x498810)
S5_STUB1(stubMaterialHook, 0x4896c0)
S5_STUB3(stubSetupCallback, 0x49aad0)
S5_STUB2(stubOpenRamDisk, 0x485580)
S5_STUB3(stubIffOpen, 0x49a790)
S5_STUB1(stubIffClose, 0x49a8a0)
S5_STUB2(stubFileClose, 0x484a60)
S5_STUB1(stubFreeBuffer, 0x485370)
S5_STUB1(stubModelSetRwInfo, 0x49c880)
S5_STUB1(stubDumpClump, 0x49c9c0)
S5_STUB1(stubFileLoadModel, 0x49db90)
S5_STUB0(stubMemoryInterface, 0x4dba80)
S5_STUB4(stubFindChunk, 0x4e2ec0)

/* ---- shadow sampler -------------------------------------------------------------------------------- */

static void BMX_CDECL stubDrawQuads(uint32_t flags, const ZrwPrimQuad *quads, int count, uint32_t arg)
{
    s5Log(0x48d660);
    s5Log(flags);
    s5Log((uint32_t)count);
    s5Log(arg);
    for (int i = 0; i < count; i++) {
        s5LogWords(&quads[i], 5);
    }
}

static void setup_shadow_draw(DtCtx *c)
{
    s5Begin(c, 0);
    S5_ON(0x0048d660, stubDrawQuads);
    dt_fill(&gZrwShadowQuads, sizeof(ZrwShadowQuadList));
    dt_fill(&gZrwShadowVertices, sizeof(ZrwShadowVertexList));
    gZrwShadowReady = dt_u32() % 6 != 0;
    gZrwShadowTexture = dt_u32();
    for (int i = 0; i < 36; i++) {
        gZrwShadowMiss[i] = dt_u32() % 7 == 0;
    }
    if (dt_u32() % 5 == 0) {
        for (int i = 0; i < 36; i++) {
            gZrwShadowMiss[i] = 1;
        }
    }
}

static void setup_shadow_stop(DtCtx *c)
{
    s5Begin(c, 0);
    S5_ON(0x0048bad0, stubUnregister);
    gZrwShadowActive = dt_u32() % 3 != 0;
    gZrwShadowNode =
        dt_u32() % 4 != 0 ? (struct ZrwUpdateNode *)(uintptr_t)(0x38000000u + (dt_u32() & 0xfff0u)) : NULL;
}

static int BMX_CDECL stubGetParameter(int id, void *out, const void *extra)
{
    s5Log(0x48ad20);
    s5Log((uint32_t)id);
    s5LogPtr(extra);
    *(float *)out = dt_float(0.2f, 4.0f);
    return 1;
}

static int BMX_CDECL stubLineQuery(const float *a, const float *b, int mode, int flags, float *out)
{
    s5Log(0x493e20);
    s5LogWords(a, 3);
    s5LogWords(b, 3);
    s5Log((uint32_t)mode);
    s5Log((uint32_t)flags);
    for (int i = 0; i < 19; i++) {
        out[i] = dt_float(-100.0f, 100.0f);
    }
    return dt_u32() % 3 != 0;
}

static void shadow_update_impl(DtCtx *c)
{
    ZrwModel *model = dt_alloc(sizeof *model);
    ZObj *object = dt_alloc(sizeof *object);
    ZrwShadow *state = dt_alloc(sizeof *state);

    s5Begin(c, 0);
    S5_ON(0x0048ad20, stubGetParameter);
    S5_ON(0x00493e20, stubLineQuery);
    dt_fill(model, sizeof *model);
    dt_fill(object, sizeof *object);
    s5RandVec(&object->pos);
    model->userValue = (uint32_t)(uintptr_t)object;
    state->model = model;
    s5RandVec(&state->offset);
    if (dt_u32() % 3 == 0) {
        state->offset.x = 0.0f;
        state->offset.y = -500.0f;
        state->offset.z = 0.0f;
    }
    gZrwShadowInit = dt_u32() % 3 != 0;
    for (int i = 0; i < 36; i++) {
        s5RandVec(&gZrwShadowOffsets[i].pos);
        gZrwShadowMiss[i] = dt_u32();
    }
    gZrwShadowScale = dt_float(0.1f, 2.0f);
    gZrwShadowDouble = dt_float(0.1f, 2.0f);
    dt_fill(&gZrwShadowVertices, sizeof(ZrwShadowVertexList));
    DT_ARG_PTR(c, 0, state);
}

static void setup_shadow_update(DtCtx *c)
{
    shadow_update_impl(c);
    s5ClearStack();
}

/* ---- decompression ---------------------------------------------------------------------------------- */

/*
 * A hashed stream of random tokens: a header word, then control words of 16 bits (set: match, two bytes;
 * clear: literal, one byte) each followed by its tokens. The decoder stops where the data ends.
 */
static uint32_t makeHashed(uint8_t *buf)
{
    uint32_t pos = 4;
    uint32_t control = 0;
    int bits = 0;
    int tokens = dt_range(1, 70);

    dt_fill(buf, 4);
    buf[0] = 0;
    for (int t = 0; t < tokens; t++) {
        if (bits == 0) {
            control = pos;
            buf[pos] = 0;
            buf[pos + 1] = 0;
            pos += 2;
            bits = 16;
        }
        if (dt_u32() % 2 == 0) {
            buf[control + (uint32_t)(16 - bits) / 8] |= (uint8_t)(1u << ((16 - bits) % 8));
            buf[pos++] = (uint8_t)dt_u32();
            buf[pos++] = (uint8_t)dt_u32();
        } else {
            buf[pos++] = (uint8_t)dt_u32();
        }
        bits--;
    }
    return pos;
}

static void setup_hashed(DtCtx *c)
{
    uint8_t *src = dt_alloc(512);
    uint32_t length;
    uint8_t *dst = dt_alloc(512 * 18 + 64);

    s5Begin(c, 0);
    dt_fill(dst, 512 * 18 + 64);
    if (dt_u32() % 8 == 0) {
        length = (uint32_t)dt_range(5, 100);
        dt_fill(src, length);
        src[0] = 1; /* stored */
    } else {
        length = makeHashed(src);
    }
    DT_ARG_PTR(c, 0, dt_alloc(0x4010));
    DT_ARG_PTR(c, 1, src);
    DT_ARG_INT(c, 2, length);
    DT_ARG_PTR(c, 3, dst);
    DT_ARG_PTR(c, 4, dt_alloc(4));
}

/* Appends a valid LZO1X stream of random tokens to buf; returns its length. */
static uint32_t makeLzo(uint8_t *buf)
{
    uint32_t pos = 0;
    uint32_t out = 0;
    int afterLiteral;
    int afterMatch = 0;
    int tokens = dt_range(1, 10);

    if (dt_u32() % 2 == 0) {
        const uint32_t t = (uint32_t)dt_range(1, 15);
        buf[pos++] = (uint8_t)t;
        for (uint32_t i = 0; i < t + 3; i++) {
            buf[pos++] = (uint8_t)dt_u32();
        }
        out += t + 3;
    } else {
        const uint32_t n = (uint32_t)dt_range(1, 30);
        buf[pos++] = (uint8_t)(17 + n);
        for (uint32_t i = 0; i < n; i++) {
            buf[pos++] = (uint8_t)dt_u32();
        }
        out += n;
    }
    afterLiteral = 1;
    while (tokens-- > 0) {
        uint32_t tail = 0;
        uint32_t len = 0;

        if (!afterLiteral && dt_u32() % 2 == 0) {
            if (dt_u32() % 3 == 0) {
                const uint32_t extra = (uint32_t)dt_range(1, 20);
                buf[pos++] = 0;
                buf[pos++] = (uint8_t)extra;
                for (uint32_t i = 0; i < 18 + extra; i++) {
                    buf[pos++] = (uint8_t)dt_u32();
                }
                out += 18 + extra;
            } else {
                const uint32_t t = (uint32_t)dt_range(1, 15);
                buf[pos++] = (uint8_t)t;
                for (uint32_t i = 0; i < t + 3; i++) {
                    buf[pos++] = (uint8_t)dt_u32();
                }
                out += t + 3;
            }
            afterLiteral = 1;
            afterMatch = 0;
            continue;
        }
        switch (dt_u32() % 3) {
        case 0: { /* M2 */
            const uint32_t maxDist = out < 2048 ? out : 2048;
            const uint32_t dist = (uint32_t)dt_range(1, (int32_t)maxDist) - 1;
            len = (uint32_t)dt_range(2, 7) + 1;
            tail = dt_u32() % 4;
            buf[pos++] = (uint8_t)(((len - 1) << 5) | ((dist & 7) << 2) | tail);
            buf[pos++] = (uint8_t)(dist >> 3);
            break;
        }
        case 1: { /* M3 */
            const uint32_t maxDist = out < 16384 ? out : 16384;
            const uint32_t dist = (uint32_t)dt_range(1, (int32_t)maxDist) - 1;
            tail = dt_u32() % 4;
            if (dt_u32() % 4 == 0) {
                const uint32_t extra = (uint32_t)dt_range(1, 10);
                buf[pos++] = 0x20;
                buf[pos++] = (uint8_t)extra;
                len = 0x21 + extra;
            } else {
                len = (uint32_t)dt_range(1, 31) + 2;
                buf[pos++] = (uint8_t)(0x20 | (len - 2));
            }
            buf[pos++] = (uint8_t)(((dist & 63) << 2) | tail);
            buf[pos++] = (uint8_t)(dist >> 6);
            break;
        }
        default: { /* M1 after a match: short form */
            if (!afterLiteral || !afterMatch) {
                continue;
            }
            const uint32_t maxDist = out < 1024 ? out : 1024;
            const uint32_t dist = (uint32_t)dt_range(1, (int32_t)maxDist) - 1;
            len = 2;
            tail = dt_u32() % 4;
            buf[pos++] = (uint8_t)(((dist & 3) << 2) | tail);
            buf[pos++] = (uint8_t)(dist >> 2);
            break;
        }
        }
        out += len;
        for (uint32_t i = 0; i < tail; i++) {
            buf[pos++] = (uint8_t)dt_u32();
        }
        out += tail;
        afterMatch = 1;
        afterLiteral = tail != 0;
    }
    switch (dt_u32() % 8) {
    case 0: /* faulty end of stream value */
        buf[pos++] = 0x13;
        buf[pos++] = 0;
        buf[pos++] = 0;
        break;
    default:
        buf[pos++] = 0x11;
        buf[pos++] = 0;
        buf[pos++] = 0;
        break;
    }
    return pos;
}

static void setup_decompress(DtCtx *c)
{
    uint8_t *src = dt_alloc(2048);
    uint8_t *dst = dt_alloc(8192);
    uint32_t length;
    int method = dt_u32() % 5 == 0 ? 1 : 2;

    s5Begin(c, 0);
    dt_fill(src, 2048);
    dt_fill(dst, 8192);
    if (method == 2) {
        length = makeLzo(src);
        if (dt_u32() % 10 == 0) {
            src[0] = (uint8_t)dt_range(0x10, 0x11); /* illegal first byte */
        }
    } else {
        length = makeHashed(src);
        if (dt_u32() % 5 == 0) {
            src[0] = 1;
        }
    }
    if (dt_u32() % 12 == 0) {
        method = dt_range(3, 9);
    }
    DT_ARG_INT(c, 0, method);
    DT_ARG_PTR(c, 1, src);
    DT_ARG_INT(c, 2, length);
    DT_ARG_PTR(c, 3, dst);
    DT_ARG_PTR(c, 4, dt_alloc(4));
}

/* ---- object events ---------------------------------------------------------------------------------- */

static void BMX_CDECL handlerA(const ZObjEvent *event)
{
    s5Log(0x3001);
    s5LogPtr(event);
    s5Log(event->id);
}

static void BMX_CDECL handlerB(const ZObjEvent *event)
{
    s5Log(0x3002);
    s5LogPtr(event);
    s5Log(event->id);
}

static void setup_events(DtCtx *c)
{
    ZObj *object = dt_alloc(sizeof *object);
    ZObjEvents *events = dt_alloc(sizeof *events);

    s5Begin(c, 0);
    dt_fill(object, sizeof *object);
    events->pending = dt_u32() % 8 == 0 ? NULL : S5_LIST_CREATE(16);
    events->handlers = dt_u32() % 5 == 0 ? NULL : S5_LIST_CREATE(16);
    if (events->pending != NULL) {
        for (int i = dt_range(0, 5); i > 0; i--) {
            ZObjEvent *e = dt_alloc(sizeof *e);
            e->id = (uint32_t)dt_range(1, 3);
            S5_LIST_INSERT(events->pending, e);
        }
    }
    if (events->handlers != NULL) {
        for (int i = dt_range(0, 4); i > 0; i--) {
            ZObjHandler *h = dt_alloc(sizeof *h);
            h->kind = dt_u32() % 4 == 0 ? 2 : 1;
            h->eventId = (uint32_t)dt_range(1, 3);
            h->fn = dt_u32() % 2 == 0 ? handlerA : handlerB;
            S5_LIST_INSERT(events->handlers, h);
        }
    }
    object->events = dt_u32() % 10 == 0 ? NULL : events;
    DT_ARG_PTR(c, 0, object);
}

/* ---- frames --------------------------------------------------------------------------------------- */

static ZMatrix gMatrix[8];
static int gMatrixNext;

static ZMatrix *BMX_CDECL stubGetMatrix(void *frame)
{
    ZMatrix *m = &gMatrix[gMatrixNext++ & 7];

    s5Log(0x4d2b00);
    s5LogPtr(frame);
    return m;
}

static uint32_t BMX_CDECL stubGetUserData(void *frame)
{
    s5Log(0x500220);
    s5LogPtr(frame);
    return dt_u32() % 4 == 0 ? dt_u32() & 0xffff : dt_u32();
}

/* Enumerates up to two child frames (a fixed array), calling the callback on each. */
static uint8_t gFrames[16][16];
static int gFrameDepth;

static void *BMX_CDECL stubForAllChildren(void *frame, FrameCb callback, void *data)
{
    s5Log(0x4d2eb0);
    s5LogPtr(frame);
    s5Log(dt_canon((uint32_t)(uintptr_t)callback));
    s5LogPtr(data);
    if (gFrameDepth > 0) {
        gFrameDepth--;
        for (int i = dt_range(0, 2); i > 0; i--) {
            callback(gFrames[(dt_u32() & 0xf)], data);
        }
        gFrameDepth++;
    }
    return frame;
}

static void beginFrames(DtCtx *c, int depth)
{
    s5Begin(c, 0);
    gMatrixNext = 0;
    gFrameDepth = depth;
    dt_fill(gMatrix, sizeof gMatrix);
    for (int i = 0; i < 8; i++) {
        float *f = (float *)&gMatrix[i];

        for (int k = 0; k < 16; k++) {
            f[k] = dt_float_edgy(-100000.0f, 100000.0f);
        }
    }
    S5_ON(0x004d2b00, stubGetMatrix);
    S5_ON(0x00500220, stubGetUserData);
    S5_ON(0x00500230, stubSetUserData);
    S5_ON(0x004d2eb0, stubForAllChildren);
}

static void setup_frame_to_matrix(DtCtx *c)
{
    beginFrames(c, 2);
    DT_ARG_PTR(c, 0, gFrames[dt_u32() & 0xf]);
    DT_ARG_PTR(c, 1, (void *)(uintptr_t)(0x20000000u + dt_u32() % 100u));
}

static void setup_collect(DtCtx *c)
{
    s5Begin(c, 0);
    DT_ARG_PTR(c, 0, gFrames[dt_u32() & 0xf]);
    DT_ARG_PTR(c, 1, dt_alloc(4));
}

/* Part nodes: a cursor into an array that is big enough for a small hierarchy. */
static void setup_part_node(DtCtx *c)
{
    ZrwModelPart *nodes = dt_alloc(sizeof *nodes * 16);
    ZrwModelPart *parent = dt_alloc(sizeof *parent);

    beginFrames(c, 2);
    dt_fill(nodes, sizeof *nodes * 16);
    dt_fill(parent, sizeof *parent);
    parent->child = dt_u32() % 2 ? (ZrwModelPart *)(uintptr_t)dt_u32() : NULL;
    gZrwModelCreate.cursor = nodes;
    DT_ARG_PTR(c, 0, gFrames[dt_u32() & 0xf]);
    DT_ARG_PTR(c, 1, dt_u32() % 5 == 0 ? NULL : parent);
}

/* ---- clumps --------------------------------------------------------------------------------------- */

static uint8_t gAtomics[4][0x60];
static int gAtomicCount;
static uint32_t gGeometryFlags;

static void *BMX_CDECL stubForAllAtomics(void *clump, void *(BMX_CDECL *callback)(void *, void *), void *data)
{
    s5Log(0x4b1990);
    s5LogPtr(clump);
    s5Log(dt_canon((uint32_t)(uintptr_t)callback));
    s5LogPtr(data);
    for (int i = 0; i < gAtomicCount; i++) {
        if (callback(gAtomics[i], data) == NULL) {
            break;
        }
    }
    return clump;
}

static int BMX_CDECL stubNumTriangles(void *geometry)
{
    s5Log(0x4b4720);
    s5LogPtr(geometry);
    return dt_range(0, 5000);
}

static int BMX_CDECL stubNumVertices(void *geometry)
{
    s5Log(0x4b4550);
    s5LogPtr(geometry);
    return dt_range(0, 9000);
}

static uint32_t BMX_CDECL stubGeometryFlags(void *geometry)
{
    s5Log(0x4b6140);
    s5LogPtr(geometry);
    return gGeometryFlags;
}

static void beginAtomics(DtCtx *c)
{
    s5Begin(c, 0);
    gAtomicCount = dt_range(1, 3);
    gGeometryFlags = dt_u32();
    for (int i = 0; i < 4; i++) {
        dt_fill(gAtomics[i], sizeof gAtomics[i]);
        *(uint32_t *)&gAtomics[i][0x1c] = 0x39000000u + (uint32_t)i * 0x100u;
        *(uint32_t *)&gAtomics[i][0x4c] = 0x39100000u + (uint32_t)i * 0x100u;
    }
    S5_ON(0x004b1990, stubForAllAtomics);
    S5_ON(0x004b4720, stubNumTriangles);
    S5_ON(0x004b4550, stubNumVertices);
    S5_ON(0x004b6140, stubGeometryFlags);
    S5_ON(0x004b1440, stubNumAtomics);
    s5SetRet(0x4b1440, (uint32_t)dt_range(0, 9));
}

static void setup_count_geometry(DtCtx *c)
{
    int *counts = dt_alloc(8);

    beginAtomics(c);
    gS5PrintMax = 1;
    counts[0] = dt_range(0, 100);
    counts[1] = dt_range(0, 100);
    DT_ARG_PTR(c, 0, gAtomics[0]);
    DT_ARG_PTR(c, 1, counts);
}

static void setup_dump_clump(DtCtx *c)
{
    beginAtomics(c);
    if (dt_u32() % 3 == 0) {
        gGeometryFlags = 0xffffffffu;
    }
    DT_ARG_PTR(c, 0, dt_alloc(0x40));
}

static void setup_first_atomic(DtCtx *c)
{
    beginAtomics(c);
    gAtomicCount = dt_u32() % 5 == 0 ? 0 : gAtomicCount;
    DT_ARG_PTR(c, 0, dt_alloc(0x40));
}

/* ---- model fixtures --------------------------------------------------------------------------------- */

static uint8_t gClumps[8][16];
static int gClumpNext;
static int gMatCount;
static char gEntryName[24] = "bodypart";
static uint32_t gLoadFlags2;
static uint8_t gLoadBuffer[0x1000];
static uint32_t gMemIf[2];

static void initClumps(void)
{
    gClumpNext = 0;
    for (int i = 0; i < 8; i++) {
        dt_fill(gClumps[i], 16);
        *(void **)&gClumps[i][4] = gFrames[i];
    }
}

static void *newMaterials(int count)
{
    void *list = S5_LIST_CREATE(16);

    for (int i = 0; i < count; i++) {
        S5_LIST_INSERT(list, (void *)(uintptr_t)(0x36000000u + (uint32_t)i * 16u));
    }
    return list;
}

static ZrwModelDef *newDef(int maxSets)
{
    ZrwModelDef *def = dt_alloc(sizeof *def);

    dt_fill(def, sizeof *def);
    def->refCount = (uint32_t)dt_range(1, 2);
    def->options = dt_u32();
    def->lodCount = (uint16_t)dt_range(0, maxSets);
    memset(def->name, 0, sizeof def->name);
    memset(def->path, 0, sizeof def->path);
    for (int i = 0; i < 12; i++) {
        def->name[i] = (char)dt_range('a', 'z');
    }
    for (int i = 0; i < 20; i++) {
        def->path[i] = (char)dt_range('a', 'z');
    }
    def->geometry = (void *)0x39000000u;
    return def;
}

static ZrwModel *newModel(int maxSets, int materials)
{
    ZrwModel *m = dt_alloc(sizeof *m);

    dt_fill(m, sizeof *m);
    m->def = newDef(maxSets);
    m->clump = (void *)gClumps[dt_u32() % 4];
    m->collisionClump = dt_u32() % 3 == 0 ? NULL : (void *)gClumps[dt_u32() % 4];
    m->materials = newMaterials(materials);
    s5RandQuat(&m->rotation);
    return m;
}

static uint32_t BMX_CDECL stubMatTexture(uint32_t material, int index)
{
    s5Log(0x489a40);
    s5Log(material);
    s5Log((uint32_t)index);
    return material + 1;
}

static uint32_t BMX_CDECL stubRwTexture(uint32_t texture)
{
    s5Log(0x498ac0);
    s5Log(texture);
    return texture + 2;
}

static const char *BMX_CDECL stubTextureName(uint32_t texture)
{
    s5Log(0x4988a0);
    s5Log(texture);
    return "texture";
}

static int BMX_CDECL stubNumMaterials(void *geometry)
{
    s5Log(0x4b4ce0);
    s5LogPtr(geometry);
    return gMatCount;
}

static uint32_t BMX_CDECL stubGetMaterial(void *geometry, int index)
{
    s5Log(0x4b47a0);
    s5LogPtr(geometry);
    s5Log((uint32_t)index);
    return 0x37000000u + (uint32_t)index * 16u;
}

static uint32_t BMX_CDECL stubMaterialTexture(uint32_t material)
{
    s5Log(0x4b93d0);
    s5Log(material);
    return 0x36000003u + ((material >> 4) & 3u) * 16u;
}

static void *BMX_CDECL stubFindEntry(const ZrwModel *model, int key)
{
    s5Log(0x487ad0);
    s5LogPtr(model);
    s5Log((uint32_t)key);
    return dt_u32() % 2 == 0 ? NULL : gEntryName;
}

static void *BMX_CDECL stubClone(void *clump)
{
    s5Log(0x4b1ee0);
    s5LogPtr(clump);
    return gClumps[4 + (gClumpNext++ & 3)];
}

static void stubsMaterials(void)
{
    S5_ON(0x00489a40, stubMatTexture);
    S5_ON(0x00498ac0, stubRwTexture);
    S5_ON(0x004988a0, stubTextureName);
    S5_ON(0x004b4ce0, stubNumMaterials);
    S5_ON(0x004b47a0, stubGetMaterial);
    S5_ON(0x004b93d0, stubMaterialTexture);
    S5_ON(0x00489b80, stubSetTexture);
}

static void setup_set_rw_info(DtCtx *c)
{
    ZrwModel *model;

    beginFrames(c, 0);
    initClumps();
    gMatCount = dt_range(0, 4);
    stubsMaterials();
    S5_ON(0x00487dc0, stubMdlUserData);
    model = newModel(2, dt_range(0, 4));
    if (dt_u32() % 3 == 0) {
        model->collisionClump = NULL;
    }
    DT_ARG_PTR(c, 0, model);
}

static void setup_instance(DtCtx *c)
{
    ZrwModel *model;

    beginFrames(c, 1);
    initClumps();
    stubsMaterials();
    gZrwModelCreate.pool = S5_POOL_CREATE(0x8c, 4, NULL);
    gZrwModelCreate.initialized = 1;
    S5_ON(0x0049db90, stubFileLoadModel);
    S5_ON(0x004b1ee0, stubClone);
    S5_ON(0x004d3200, stubSetIdentity);
    S5_ON(0x0049c880, stubModelSetRwInfo);
    S5_ON(0x004d2f00, stubFrameCount);
    S5_ON(0x00489af0, stubRetain);
    S5_ON(0x00487c20, stubSetInWorld);
    S5_ON(0x00487cd0, stubSetActive);
    s5SetRet(0x49db90, 0x3a000000u);
    s5SetRet(0x4d2f00, (uint32_t)dt_range(3, 5));
    model = newModel(4, dt_range(0, 4));
    model->def->options = dt_u32() % 6 == 0 ? (model->def->options | 2u) : (model->def->options & ~2u);
    DT_ARG_PTR(c, 0, model);
}

/* ---- loading --------------------------------------------------------------------------------------- */

static void *BMX_CDECL rwAlloc(uint32_t size)
{
    s5Log(0x4dba81);
    s5Log(size);
    return gLoadBuffer;
}

static void BMX_CDECL rwFree(void *block)
{
    s5Log(0x4dba82);
    s5LogPtr(block);
}

static void stubsStream(void)
{
    gMemIf[0] = (uint32_t)(uintptr_t)rwAlloc;
    gMemIf[1] = (uint32_t)(uintptr_t)rwFree;
    S5_ON(0x004dba80, stubMemoryInterface);
    s5SetRet(0x4dba80, (uint32_t)(uintptr_t)gMemIf);
}

static void setup_add_clump(DtCtx *c)
{
    uint32_t *memory = dt_alloc(8);
    ZrwModel *model;

    beginFrames(c, 1);
    initClumps();
    stubsStream();
    S5_ON(0x004e4420, stubImageSetPath);
    S5_ON(0x004e23e0, stubStreamOpen);
    S5_ON(0x004e2ec0, stubFindChunk);
    S5_ON(0x004b2790, stubStreamRead);
    S5_ON(0x004e2660, stubStreamClose);
    S5_ON(0x004d3200, stubSetIdentity);
    S5_ON(0x004b1990, stubForAllAtomics);
    S5_ON(0x004d2f00, stubFrameCount);
    S5_ON(0x00487bf0, stubMaskGeometry);
    S5_ON(0x00487b90, stubMaskAtomic);
    S5_ON(0x0049c9c0, stubDumpClump);
    s5SetRet(0x4e23e0, dt_u32() % 5 == 0 ? 0u : 0x3b000000u);
    s5SetRet(0x4e2ec0, dt_u32() % 5 != 0);
    s5SetRet(0x4b2790, dt_u32() % 5 == 0 ? 0u : (uint32_t)(uintptr_t)gClumps[4 + dt_u32() % 4]);
    s5SetRet(0x4d2f00, (uint32_t)dt_range(3, 5));
    gAtomicCount = 1;
    for (int i = 0; i < 4; i++) {
        dt_fill(gAtomics[i], sizeof gAtomics[i]);
        *(uint32_t *)&gAtomics[i][0x1c] = 0x39000000u + (uint32_t)i * 0x100u;
        *(uint32_t *)&gAtomics[i][0x4c] = 0x39100000u + (uint32_t)i * 0x100u;
    }
    memory[0] = (uint32_t)(uintptr_t)gLoadBuffer;
    memory[1] = (uint32_t)dt_range(16, 512);
    model = newModel(3, dt_range(0, 2));
    model->clump = (void *)gClumps[0];
    DT_ARG_PTR(c, 0, model);
    DT_ARG_PTR(c, 1, memory);
    DT_ARG_PTR(c, 2, dt_u32() % 2 == 0 ? NULL : (void *)"textures\\");
    DT_ARG_INT(c, 3, dt_u32() % 3 == 0 ? 1 : (int)(dt_u32() % 3 == 0 ? 2 : 0));
}

static int BMX_CDECL stubAddClump(ZrwModel *model, const uint32_t *memory, const char *path, int collision)
{
    s5Log(0x49d120);
    s5LogPtr(model);
    s5Log(memory[0]);
    s5Log(memory[1]);
    s5LogPtr(path);
    s5Log((uint32_t)collision);
    return gLoadFlags2;
}

static void beginLoad(DtCtx *c, int beginOk, int endOk)
{
    s5Begin(c, 1);
    initClumps();
    stubsStream();
    s5IffBeginScript(beginOk, endOk);
    gLoadFlags2 = dt_u32() % 3 != 0;
    S5_ON(0x0049d120, stubAddClump);
    S5_ON(0x00487b90, stubMaskAtomic);
    S5_ON(0x004b86a0, stubWorldAdd);
    S5_ON(0x004896c0, stubMaterialHook);
    s5SetRet(0x4896c0, 0x3c000000u);
}

static void setup_load_lod(DtCtx *c)
{
    uint32_t *lod = dt_alloc(8);
    ZrwModel *model;

    beginLoad(c, dt_u32() % 6 != 0, dt_u32() % 6 != 0);
    lod[0] = dt_u32();
    {
        float f = dt_u32() % 4 == 0 ? 0.0f : dt_float(1.0f, 500.0f);
        memcpy(&lod[1], &f, 4);
    }
    if (dt_u32() % 6 != 0) {
        s5AddChunk(0x46444c4d, lod, 8);
    }
    if (dt_u32() % 5 != 0) {
        s5AddChunk(0x4646444d, gLoadBuffer, (uint32_t)dt_range(4, 200));
    }
    model = newModel(4, 0);
    DT_ARG_PTR(c, 0, dt_alloc(16));
    DT_ARG_PTR(c, 1, model);
}

static void setup_load_collision(DtCtx *c)
{
    ZrwModel *model;

    beginLoad(c, dt_u32() % 6 != 0, dt_u32() % 6 != 0);
    if (dt_u32() % 5 != 0) {
        s5AddChunk(0x4646444d, gLoadBuffer, (uint32_t)dt_range(4, 200));
    }
    model = newModel(3, 0);
    DT_ARG_PTR(c, 0, dt_alloc(16));
    DT_ARG_PTR(c, 1, model);
}

static void setup_read_def(DtCtx *c)
{
    uint8_t *data = dt_alloc(0x2c + 5 * 0x14 + 16);
    uint32_t *block = (uint32_t *)data;
    ZrwModel *model;

    beginLoad(c, 1, 1);
    dt_fill(data, 0x2c + 5 * 0x14 + 16);
    block[10] = ((uint32_t)dt_u32() & 0xffff0000u) | (uint32_t)dt_range(0, 5);
    if (dt_u32() % 6 != 0) {
        s5AddChunk(0x4645444d, data, 0x2c + 5 * 0x14 + 16);
    }
    model = newModel(1, 0);
    DT_ARG_PTR(c, 0, dt_alloc(16));
    DT_ARG_PTR(c, 1, model);
}

static void setup_add_material(DtCtx *c)
{
    ZrwModel *model;

    s5Begin(c, 0);
    initClumps();
    model = newModel(1, dt_range(0, 3));
    DT_ARG_PTR(c, 0, (void *)(uintptr_t)(0x3d000000u + (dt_u32() & 0xfff0u)));
    DT_ARG_PTR(c, 1, model);
}

static void setup_load_materials(DtCtx *c)
{
    ZrwModel *model;

    beginLoad(c, 1, 1);
    gS5NextLeft = dt_range(0, 3);
    for (int i = dt_range(0, 3); i > 0; i--) {
        gS5Dispatch[gS5DispatchN++] = dt_u32() % 4 != 0;
    }
    if (dt_u32() % 5 != 0) {
        s5AddChunk(0x4954414d, "", 0);
    }
    model = newModel(1, dt_range(0, 3));
    DT_ARG_PTR(c, 0, dt_alloc(16));
    DT_ARG_PTR(c, 1, model);
}

static int BMX_CDECL stubReadDef(void *iff, ZrwModel *model)
{
    s5Log(0x49d5f0);
    s5LogPtr(iff);
    s5LogPtr(model);
    model->def->options = gLoadFlags2;
    return 1;
}

static void setup_load(DtCtx *c)
{
    beginLoad(c, dt_u32() % 8 != 0, dt_u32() % 6 != 0);
    gLoadFlags2 = dt_u32();
    S5_ON(0x0049d5f0, stubReadDef);
    S5_ON(0x00505950, stubSkin);
    S5_ON(0x0048fee0, stubDefaultRender);
    S5_ON(0x0048ff00, stubRenderRegister);
    S5_ON(0x0049c880, stubModelSetRwInfo);
    S5_ON(0x00488650, stubModelUpdate);
    gZrwModelCreate.pool = S5_POOL_CREATE(0x8c, 4, NULL);
    gZrwModelCreate.initialized = 1;
    gZrwModelCreate.last = (ZrwModel *)(uintptr_t)dt_u32();
    gS5NextLeft = dt_range(0, 3);
    for (int i = dt_range(0, 4); i > 0; i--) {
        gS5Dispatch[gS5DispatchN++] = dt_u32() % 4 != 0;
    }
    if (dt_u32() % 2 != 0) {
        s5AddChunk(0x4954414d, "", 0);
    }
    if (dt_u32() % 2 != 0) {
        s5AddChunk(0x444f4c4d, "", 0);
    }
    if (dt_u32() % 2 != 0) {
        s5AddChunk(0x4c4f434d, "", 0);
    }
    DT_ARG_PTR(c, 0, dt_alloc(16));
}

static void setup_create_init(DtCtx *c)
{
    s5Begin(c, 1);
    S5_ON(0x0049aad0, stubSetupCallback);
    gZrwModelCreate.initialized = dt_u32() % 5 == 0;
    gZrwModelCreate.pool = NULL;
}

static void setup_create_shutdown(DtCtx *c)
{
    s5Begin(c, 0);
    gZrwModelCreate.initialized = dt_u32() % 3 != 0;
    gZrwModelCreate.pool = S5_POOL_CREATE(0x8c, 4, NULL);
}

static void setup_from_chunk(DtCtx *c)
{
    beginLoad(c, 1, 1);
    gZrwModelCreate.last = (ZrwModel *)(uintptr_t)(0x3e000000u + (dt_u32() & 0xfff0u));
    for (int i = dt_range(0, 1); i > 0; i--) {
        gS5Dispatch[gS5DispatchN++] = (int)(dt_u32() & 1);
    }
    DT_ARG_PTR(c, 0, dt_alloc(16));
}

/* Stubs for the memory file and the IFF reader the two loaders open. */
static void beginOpenModel(DtCtx *c)
{
    beginLoad(c, 1, 1);
    S5_ON(0x00485580, stubOpenRamDisk);
    S5_ON(0x0049a790, stubIffOpen);
    S5_ON(0x0049a8a0, stubIffClose);
    S5_ON(0x00484a60, stubFileClose);
    s5SetRet(0x485580, dt_u32() % 6 == 0 ? 0u : 0x3f000000u);
    s5SetRet(0x49a790, dt_u32() % 6 == 0 ? 0u : 0x3f100000u);
    if (dt_u32() % 5 != 0) {
        s5AddChunk(0x4c444f4d, "", 0);
    }
    gZrwModelCreate.last = newModel(1, 0);
}

static void setup_from_buffer(DtCtx *c)
{
    uint32_t *buffer = dt_alloc(32);

    beginOpenModel(c);
    buffer[0] = dt_u32() % 6 == 0 ? dt_u32() : 0x4c444f4du;
    buffer[1] = (uint32_t)dt_range(0, 100);
    DT_ARG_PTR(c, 0, buffer);
}

static uint32_t gFileBuffer[16];

static void *BMX_CDECL stubFileLoad(const char *path, int *size)
{
    s5Log(0x484d30);
    s5Log(s5Hash(path));
    *size = 64;
    return gFileBuffer[2] == 0 ? NULL : gFileBuffer;
}

static void setup_from_file(DtCtx *c)
{
    char *path = dt_alloc(0x200);

    beginOpenModel(c);
    S5_ON(0x00484d30, stubFileLoad);
    S5_ON(0x00485370, stubFreeBuffer);
    memset(gFileBuffer, 0, sizeof gFileBuffer);
    gFileBuffer[0] = dt_u32() % 6 == 0 ? dt_u32() : 0x4c444f4du;
    gFileBuffer[1] = (uint32_t)dt_range(0, 100);
    gFileBuffer[2] = dt_u32() % 7 != 0;
    for (int i = 0; i < 12; i++) {
        path[i] = (char)dt_range('a', 'z');
    }
    path[dt_range(1, 12)] = 0;
    *(void **)(path + 0x88) = dt_alloc(0x100);
    DT_ARG_PTR(c, 0, path);
}

static void setup_not_coded(DtCtx *c)
{
    s5Begin(c, 1);
}

static void setup_destroy(DtCtx *c)
{
    ZrwModel *model;
    ZrwModelDef *def;

    beginFrames(c, 0);
    initClumps();
    stubsMaterials();
    S5_ON(0x004b8870, stubClumpWorld);
    S5_ON(0x004b8770, stubWorldRemove);
    S5_ON(0x004b21e0, stubClumpDestroy);
    S5_ON(0x0048ff30, stubRenderUnregister);
    S5_ON(0x00498810, stubTextureRelease);
    S5_ON(0x00489530, stubReleaseTexture);
    s5SetRet(0x4b8870, dt_u32() % 2 ? 0x3a100000u : 0u);
    gZrwModelCreate.pool = S5_POOL_CREATE(0x8c, 4, NULL);
    gZrwModelCreate.initialized = 1;
    model = S5_POOL_ALLOC(gZrwModelCreate.pool);
    dt_fill(model, sizeof *model);
    def = dt_zmemalloc(0xbc);
    dt_fill(def, 0xbc);
    def->refCount = (uint32_t)dt_range(1, 2);
    def->options = dt_u32();
    def->lodCount = (uint16_t)dt_range(0, 3);
    def->entries = dt_zmemalloc(0x28);
    model->def = def;
    model->clump = (void *)gClumps[0];
    model->collisionClump = dt_u32() % 2 ? (void *)gClumps[1] : NULL;
    model->materials = newMaterials(dt_range(0, 4));
    for (uint32_t i = 0; i < def->lodCount; i++) {
        model->lods[i].parts = dt_zmemalloc(0x28);
    }
    DT_ARG_PTR(c, 0, model);
}

static void setup_print_usage(DtCtx *c)
{
    s5Begin(c, 0);
    gZrwModelCreate.pool = S5_POOL_CREATE(0x8c, 4, NULL);
    for (int i = dt_range(0, 6); i > 0; i--) {
        S5_POOL_ALLOC(gZrwModelCreate.pool);
    }
}

static void setup_dump_info(DtCtx *c)
{
    ZrwModel *model;

    beginFrames(c, 2);
    initClumps();
    beginAtomics(c);
    S5_ON(0x004d2b00, stubGetMatrix);
    S5_ON(0x00500220, stubGetUserData);
    S5_ON(0x004d2eb0, stubForAllChildren);
    S5_ON(0x00487ad0, stubFindEntry);
    S5_ON(0x0049c9c0, stubDumpClump);
    stubsMaterials();
    model = newModel(1, dt_range(0, 4));
    DT_ARG_PTR(c, 0, model);
}

static void setup_dump_frame(DtCtx *c)
{
    beginFrames(c, 2);
    S5_ON(0x00487ad0, stubFindEntry);
    DT_ARG_PTR(c, 0, gFrames[dt_u32() & 0xf]);
    DT_ARG_PTR(c, 1, dt_alloc(0x8c));
}

static void setup_dump_hierarchy(DtCtx *c)
{
    beginFrames(c, 2);
    initClumps();
    S5_ON(0x00487ad0, stubFindEntry);
    DT_ARG_PTR(c, 0, newModel(1, 0));
}

DT_CASES(zrwmodelcreate_cases) = {
    {"zrollerShadowDraw", 0x0049b3d0, zrollerShadowDraw, 0, DT_RET_INT, setup_shadow_draw, 300, 0},
    {"zrollerShadowStop", 0x0049b4e0, zrollerShadowStop, 0, DT_RET_INT, setup_shadow_stop, 50, 0},
    {"zrollerUpdateShadow", 0x0049b520, zrollerUpdateShadow, 1, DT_RET_VOID, setup_shadow_update, 300, 0},
    {"zassetDecompressHashed", 0x0049bd10, zassetDecompressHashed, 5, DT_RET_VOID, setup_hashed, 400, 0},
    {"zassetDecompress", 0x0049c080, zassetDecompress, 5, DT_RET_INT, setup_decompress, 800, 0},
    {"zobjDispatchEvents", 0x0049c570, zobjDispatchEvents, 1, DT_RET_VOID, setup_events, 300, 0},
    {"zrwFrameUserDataToMatrix", 0x0049c6e0, zrwFrameUserDataToMatrix, 2, DT_RET_INT, setup_frame_to_matrix,
     200, 0},
    {"zrwFrameMatrixToUserData", 0x0049c7a0, zrwFrameMatrixToUserData, 2, DT_RET_INT, setup_frame_to_matrix,
     300, 0},
    {"zrwModelSetRwInfo", 0x0049c880, zrwModelSetRwInfo, 1, DT_RET_VOID, setup_set_rw_info, 300, 0},
    {"zrwAtomicCountGeometry", 0x0049c970, zrwAtomicCountGeometry, 2, DT_RET_INT, setup_count_geometry, 50,
     0},
    {"zrwClumpDump", 0x0049c9c0, zrwClumpDump, 1, DT_RET_VOID, setup_dump_clump, 100, 0},
    {"zrwFrameDumpHierarchy", 0x0049caf0, zrwFrameDumpHierarchy, 2, DT_RET_INT, setup_dump_frame, 100, 0},
    {"zrwModelDumpHierarchy", 0x0049cb60, zrwModelDumpHierarchy, 1, DT_RET_VOID, setup_dump_hierarchy, 100,
     0},
    {"zrwFrameCollectFirst", 0x0049cc60, zrwFrameCollectFirst, 2, DT_RET_INT, setup_collect, 10, 0},
    {"zrwModelCreateFirstAtomic", 0x0049cc70, zrwModelCreateFirstAtomic, 1, DT_RET_INT, setup_first_atomic,
     20, 0},
    {"zrwFrameBuildPartNode", 0x0049cce0, zrwFrameBuildPartNode, 2, DT_RET_INT, setup_part_node, 200, 0},
    {"zrwModelCreateInstance", 0x0049ce90, zrwModelCreateInstance, 1, DT_RET_INT, setup_instance, 300, 0},
    {"zrwModelAddClump", 0x0049d120, zrwModelAddClump, 4, DT_RET_INT, setup_add_clump, 400, 0},
    {"zrwModelLoadLod", 0x0049d350, zrwModelLoadLod, 2, DT_RET_INT, setup_load_lod, 400, 0},
    {"zrwModelLoadCollision", 0x0049d4d0, zrwModelLoadCollision, 2, DT_RET_INT, setup_load_collision, 200, 0},
    {"zrwModelReadDef", 0x0049d5f0, zrwModelReadDef, 2, DT_RET_INT, setup_read_def, 300, 0},
    {"zrwModelAddMaterial", 0x0049d750, zrwModelAddMaterial, 2, DT_RET_INT, setup_add_material, 50, 0},
    {"zrwModelLoadMaterials", 0x0049d770, zrwModelLoadMaterials, 2, DT_RET_INT, setup_load_materials, 200, 0},
    {"zrwModelLoad", 0x0049d7e0, zrwModelLoad, 1, DT_RET_INT, setup_load, 500, 0},
    {"zrwModelCreateInitialize", 0x0049d9d0, zrwModelCreateInitialize, 0, DT_RET_INT, setup_create_init, 50,
     0},
    {"zrwModelCreateShutdown", 0x0049da60, zrwModelCreateShutdown, 0, DT_RET_INT, setup_create_shutdown, 20,
     0},
    {"zrwModelLoadFromChunk", 0x0049da90, zrwModelLoadFromChunk, 1, DT_RET_INT, setup_from_chunk, 50, 0},
    {"zrwModelLoadFromBuffer", 0x0049dab0, zrwModelLoadFromBuffer, 1, DT_RET_INT, setup_from_buffer, 200, 0},
    {"zrwModelLoadFromFile", 0x0049db90, zrwModelLoadFromFile, 1, DT_RET_INT, setup_from_file, 300, 0},
    {"zrwModelNotCoded", 0x0049dcb0, zrwModelNotCoded, 0, DT_RET_INT, setup_not_coded, 2, 0},
    {"zrwModelDestroy", 0x0049dcd0, zrwModelDestroy, 1, DT_RET_INT, setup_destroy, 300, 0},
    {"zrwModelPrintUsage", 0x0049de40, zrwModelPrintUsage, 0, DT_RET_VOID, setup_print_usage, 20, 0},
    {"zrwModelDumpInfo", 0x0049de60, zrwModelDumpInfo, 1, DT_RET_VOID, setup_dump_info, 100, 0},
};
