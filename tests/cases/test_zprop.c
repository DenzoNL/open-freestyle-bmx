/*
 * Tests for zprop.c: the prop definition and placed prop chunk handlers, run against a scripted IFF reader
 * (which chunks exist, what they contain) and stubs for the model loader and the scene.
 */
#include "dt.h"
#include "dt_s5.h"
#include "dt_zmem.h"
#include "zge/com/zglrw/zrwzgl.h"
#include "zge/zgemods/zprop.h"
#include "zge/zobj.h"
#include "zge/zscene.h"

#define gPropDefs  ORIG_VAR(0x006722b8, ZrwList, gPropDefs)
#define gPropInsts ORIG_VAR(0x006722c4, ZrwList, gPropInsts)

#define ID_PDIN 0x4e494450u
#define ID_MODL 0x4c444f4du
#define ID_OBJI 0x494a424fu
#define ID_PRBL 0x4c425250u

static void *BMX_CDECL stubLoadModel(void *iff)
{
    s5Log(0x489090);
    s5LogPtr(iff);
    return (void *)(uintptr_t)(0x33000000u + (dt_u32() & 0xffff0u));
}

static int BMX_CDECL stubSetFlags(const void *model, uint8_t clear, uint8_t set)
{
    s5Log(0x4888c0);
    s5LogPtr(model);
    s5Log(clear);
    s5Log(set);
    return 1;
}

static void *BMX_CDECL stubAddEntry(void *node, uint32_t id, uint32_t type, uint8_t kind, void *object)
{
    s5Log(0x490710);
    s5LogPtr(node);
    s5Log(id);
    s5Log(type);
    s5Log(kind);
    s5LogPtr(object);
    return NULL;
}

static int BMX_CDECL stubGetParameter(int id, void *out, const void *extra)
{
    s5Log(0x48ad20);
    s5Log((uint32_t)id);
    s5LogPtr(extra);
    *(float *)out = dt_float(0.1f, 3.0f);
    return 1;
}

/* A scene object with logging hooks: what the model find returns. */
static int BMX_CDECL opMoved(ZObj *o, const ZVec3 *p)
{
    s5Log(0x2003);
    s5LogPtr(o);
    s5LogWords(p, 3);
    return 1;
}

static int BMX_CDECL opRotated(ZObj *o, const ZQuat *q)
{
    s5Log(0x2004);
    s5LogPtr(o);
    s5LogWords(q, 4);
    return 1;
}

static const ZObjOps gOps = {NULL, NULL, opMoved, opRotated, NULL, NULL};
static ZObj gSceneObject;

static void *BMX_CDECL stubFindModel(int flags, const char *name)
{
    s5Log(0x491410);
    s5Log((uint32_t)flags);
    s5Log(s5Hash(name));
    if (dt_u32() % 5 == 0) {
        return NULL;
    }
    gSceneObject.ops = &gOps;
    return &gSceneObject;
}

static void stubs(void)
{
    dt_stub(0x00489090, NULL, (void *)stubLoadModel);
    dt_stub(0x004888c0, NULL, (void *)stubSetFlags);
    dt_stub(0x00490710, NULL, (void *)stubAddEntry);
    dt_stub(0x0048ad20, NULL, (void *)stubGetParameter);
    dt_stub(0x00491410, NULL, (void *)stubFindModel);
}

static void clearLists(void)
{
    memset(&gPropDefs, 0, sizeof gPropDefs);
    memset(&gPropInsts, 0, sizeof gPropInsts);
}

static void setup_def(DtCtx *c)
{
    uint32_t *header = dt_alloc(0x38);
    uint32_t *block = dt_alloc(8);
    char *name = (char *)&header[2];

    s5Begin(c, 1);
    stubs();
    clearLists();
    s5IffBeginScript(dt_u32() % 6 != 0, 1);
    dt_fill(header, 0x38);
    header[0] = dt_u32() % 5 != 0 ? 0x38 : dt_u32();
    for (int i = 0; i < 19; i++) {
        name[i] = (char)dt_range('a', 'z');
    }
    name[dt_range(0, 19)] = 0;
    name[19] = 0;
    block[0] = 8;
    block[1] = (uint32_t)dt_range(0, 40);
    if (dt_u32() % 7 != 0) {
        s5AddChunk(ID_PDIN, header, 0x38);
    }
    if (dt_u32() % 5 != 0) {
        s5AddChunk(ID_MODL, "", 0);
    }
    if (dt_u32() % 2 == 0) {
        uint8_t *obji = dt_alloc(64);
        dt_fill(obji, 64);
        memcpy(obji, block, 8);
        s5AddChunk(ID_OBJI, obji, 64);
    }
    DT_ARG_PTR(c, 0, dt_alloc(0x110));
}

static void setup_prop(DtCtx *c)
{
    uint32_t *header = dt_alloc(0x48);
    ZPropDef *prev = NULL;
    uint32_t wanted = 0;
    const int defs = dt_range(0, 4);

    s5Begin(c, 1);
    stubs();
    clearLists();
    s5IffBeginScript(dt_u32() % 6 != 0, 1);
    for (int i = 0; i < defs; i++) {
        ZPropDef *d = dt_alloc(sizeof *d);

        dt_fill(d, sizeof *d);
        d->next = NULL;
        d->object.id = 100 + (uint32_t)i;
        for (int k = 0; k < 8; k++) {
            d->object.name[k] = (char)dt_range('a', 'z');
        }
        d->object.name[dt_range(1, 8)] = 0;
        if (prev != NULL) {
            prev->next = d;
        } else {
            gPropDefs.head = (void *)d;
        }
        prev = d;
        wanted = d->object.id;
    }
    gPropDefs.tail = (void *)prev;
    gPropDefs.count = (uint32_t)defs;
    dt_fill(header, 0x48);
    header[0] = dt_u32() % 5 != 0 ? 0x48 : dt_u32();
    header[1] = dt_u32() % 4 != 0 ? wanted - (uint32_t)dt_range(0, defs > 0 ? defs - 1 : 0) : dt_u32();
    for (int i = 6; i < 18; i++) {
        float f = dt_float_edgy(-1.0f, 1.0f);
        memcpy(&header[i], &f, 4);
    }
    if (dt_u32() % 7 != 0) {
        s5AddChunk(ID_PRBL, header, 0x48);
    }
    if (dt_u32() % 2 == 0) {
        uint8_t *obji = dt_alloc(64);
        uint32_t block[2] = {8, (uint32_t)dt_range(0, 40)};

        dt_fill(obji, 64);
        memcpy(obji, block, 8);
        s5AddChunk(ID_OBJI, obji, 64);
    }
    DT_ARG_PTR(c, 0, dt_alloc(0x110));
}

DT_CASES(zprop_cases) = {
    {"zpropLoadDef", 0x00486a20, zpropLoadDef, 1, DT_RET_INT, setup_def, 400, 0},
    {"zpropLoad", 0x00486be0, zpropLoad, 1, DT_RET_INT, setup_prop, 600, 0},
};
