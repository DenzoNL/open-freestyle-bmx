/*
 * Differential tests of zrwfile.c (the "rw00" file device), zrwmodelgeom.c (vertex access) and the light
 * system initialiser. The file operations RenderWare saved (gZrwFileOps) are replaced by logging functions,
 * callbacks log the event record they get, and the renderer's geometry calls are stubbed.
 */
#include "dt.h"
#include "dt_gameplay.h"
#include "zge/com/zglrw/zrwfile.h"
#include "zge/com/zglrw/zrwlight.h"
#include "zge/com/zglrw/zrwmodelgeom.h"

/* Result the scripted file operations return. */
static struct {
    uint32_t open;
    uint32_t seek;
    uint32_t transfer; /* bytes read or written, as a delta from the request */
} ops;

static int BMX_CDECL opExists(const char *name)
{
    dtgpLog(0xe1157000u);
    dtgpLog(dtgpHash(name));
    return (int)(dt_u32() & 1);
}

static void *BMX_CDECL opOpen(const char *name, const char *mode)
{
    dtgpLog(0x09e40000u);
    dtgpLog(dtgpHash(name));
    dtgpLogP(mode);
    return (void *)(uintptr_t)ops.open;
}

static int BMX_CDECL opClose(void *file)
{
    dtgpLog(0xc1059000u);
    dtgpLogP(file);
    return (int)ops.seek;
}

static uint32_t BMX_CDECL opRead(void *buffer, uint32_t size, uint32_t count, void *file)
{
    dtgpLog(0x4ead0000u);
    dtgpLogP(buffer);
    dtgpLog(size);
    dtgpLog(count);
    dtgpLogP(file);
    return count + ops.transfer;
}

static uint32_t BMX_CDECL opWrite(const void *buffer, uint32_t size, uint32_t count, void *file)
{
    dtgpLog(0x3a1e0000u);
    dtgpLogP(buffer);
    dtgpLog(size);
    dtgpLog(count);
    dtgpLogP(file);
    return count + ops.transfer;
}

static int BMX_CDECL opSeek(void *file, int32_t offset, int whence)
{
    dtgpLog(0x5ee40000u);
    dtgpLogP(file);
    dtgpLog((uint32_t)offset);
    dtgpLog((uint32_t)whence);
    return (int)ops.seek;
}

static void BMX_CDECL cb3(ZrwFileEvent *e)
{
    const uint32_t *w = (const uint32_t *)e;

    dtgpLog(0xcb300000u);
    dtgpLog(w[0]);
    dtgpLogP((const void *)(uintptr_t)w[1]);
    dtgpLog(w[2]);
}

static void BMX_CDECL cb4(ZrwFileEvent *e)
{
    const uint32_t *w = (const uint32_t *)e;

    dtgpLog(0xcb400000u);
    dtgpLog(w[0]);
    dtgpLogP((const void *)(uintptr_t)w[1]);
    dtgpLog(w[2]);
    dtgpLog(w[3]);
}

static void BMX_CDECL stubErrorMessage(const char *msg)
{
    dtgpLog(0xe4402000u);
    dtgpLog(dtgpHash(msg));
}

static int BMX_CDECL stubFseek(void *file, int32_t offset, int whence)
{
    dtgpLog(0xf5ee4000u);
    dtgpLogP(file);
    dtgpLog((uint32_t)offset);
    dtgpLog((uint32_t)whence);
    return 0;
}

static int32_t BMX_CDECL stubFtell(void *file)
{
    dtgpLog(0xf7e11000u);
    dtgpLogP(file);
    return (int32_t)dt_u32();
}

static void *ifaceBuffer;

static void *BMX_CDECL stubGetInterface(void)
{
    dtgpLog(0x6e71f000u);
    return ifaceBuffer;
}

static void begin(const DtCtx *c)
{
    dtgpBegin(c, DTGP_DEBUG);
    gZrwFileInitialized = (dt_u32() & 7) != 0;
    gZrwFileOps.exists = opExists;
    gZrwFileOps.open = opOpen;
    gZrwFileOps.close = opClose;
    gZrwFileOps.read = opRead;
    gZrwFileOps.write = opWrite;
    gZrwFileOps.seek = opSeek;
    ops.open = (dt_u32() & 3) == 0 ? 0 : 0x00600000u + dt_range(0, 255) * 4u;
    ops.seek = (dt_u32() & 3) == 0 ? 1 : 0;
    ops.transfer = (dt_u32() & 3) == 0 ? (uint32_t)dt_range(-2, 2) : 0;
    dt_stub(0x00483a60, NULL, (void *)stubErrorMessage);
}

static ZrwFileCallback *randomCallback(int variant)
{
    ZrwFileCallback *cb;

    if ((dt_u32() & 3) == 0) {
        return NULL;
    }
    cb = dt_alloc(sizeof *cb);
    cb->fn = variant == 3 ? cb3 : cb4;
    cb->arg = dt_u32();
    return cb;
}

static char *randomString(int maxLen)
{
    char *s = dt_alloc((uint32_t)maxLen + 1);
    const int len = dt_range(0, maxLen);

    for (int i = 0; i < len; i++) {
        s[i] = (char)dt_range('A', 'z');
    }
    return s;
}

static ZrwFileHandle *randomHandle(int allowBad)
{
    ZrwFileHandle *h;

    if (allowBad && (dt_u32() & 15) == 0) {
        return NULL;
    }
    h = dt_alloc(sizeof *h);
    h->file =
        (allowBad && (dt_u32() & 15) == 0) ? NULL : (void *)(uintptr_t)(0x00610000u + dt_range(0, 255) * 4u);
    return h;
}

static void setupDeviceName(DtCtx *c)
{
    begin(c);
}

static void setupResolve(DtCtx *c)
{
    const char **keys = (const char **)dt_alloc(8);
    const char **values = (const char **)dt_alloc(8);

    begin(c);
    /* The key the function compares with is the exe's own "PATH" at 0x562130. */
    keys[0] = (dt_u32() & 3) != 0 ? (const char *)(uintptr_t)0x00562130u : randomString(5);
    values[0] = randomString(100);
    DT_ARG_INT(c, 0, dt_u32());
    DT_ARG_INT(c, 1, (dt_u32() & 3) != 0 ? 1 : dt_range(0, 3));
    DT_ARG_PTR(c, 2, keys);
    DT_ARG_PTR(c, 3, values);
}

static void setupSucceed(DtCtx *c)
{
    begin(c);
    DT_ARG_INT(c, 0, dt_u32());
    DT_ARG_PTR(c, 1, (dt_u32() & 3) == 0 ? NULL : dt_alloc(4));
    DT_ARG_PTR(c, 2, randomCallback(4));
}

static void setupNoArgs(DtCtx *c)
{
    begin(c);
}

static void setupExists(DtCtx *c)
{
    begin(c);
    DT_ARG_INT(c, 0, dt_u32());
    DT_ARG_PTR(c, 1, randomString(60));
    DT_ARG_PTR(c, 2, (dt_u32() & 3) == 0 ? NULL : dt_alloc(4));
    DT_ARG_PTR(c, 3, randomCallback(4));
}

static void setupOpen(DtCtx *c)
{
    ZrwFileHandle *h;

    begin(c);
    h = randomHandle(1);
    if (h == NULL) {
        ops.open = 0; /* the original would store through the null handle */
    }
    DT_ARG_PTR(c, 0, h);
    DT_ARG_PTR(c, 1, randomString(100));
    DT_ARG_INT(c, 2, (dt_u32() & 7) == 0 ? dt_range(-2, 6) : dt_range(0, 3));
    DT_ARG_PTR(c, 3, randomCallback(3));
}

/* A handle the operation may dereference: the original does not survive a null handle or one without a file.
 */
static ZrwFileHandle *usableHandle(void)
{
    ZrwFileHandle *h = randomHandle(0);

    if ((dt_u32() & 15) == 0) {
        h->file = NULL;
    }
    return h;
}

static void setupClose(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, usableHandle());
    DT_ARG_PTR(c, 1, randomCallback(3));
}

static void setupTransfer(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, usableHandle());
    DT_ARG_PTR(c, 1, dt_alloc(64));
    DT_ARG_INT(c, 2, dt_range(0, 64));
    DT_ARG_PTR(c, 3, randomCallback(4));
}

static void setupSeek(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, usableHandle());
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_INT(c, 2, (dt_u32() & 7) == 0 ? dt_range(-2, 5) : dt_range(0, 2));
    DT_ARG_PTR(c, 3, randomCallback(4));
}

static void setupSize(DtCtx *c)
{
    begin(c);
    dt_stub(0x004a61ad, NULL, (void *)stubFseek);
    dt_stub(0x004a6239, NULL, (void *)stubFtell);
    DT_ARG_PTR(c, 0, usableHandle());
    DT_ARG_PTR(c, 1, dt_alloc(4));
    DT_ARG_PTR(c, 2, randomCallback(4));
}

static void setupFsInit(DtCtx *c)
{
    ifaceBuffer = dt_alloc(40);
    begin(c);
    dt_fill(ifaceBuffer, 40);
    dt_stub(0x004e8280, NULL, (void *)stubGetInterface);
    dt_fill(&gZrwFileOps, sizeof gZrwFileOps);
}

static void setupFsShutdown(DtCtx *c)
{
    begin(c);
}

/* ---- zrwmodelgeom ------------------------------------------------------------------------------ */

static void *stubVertices;

static void *BMX_CDECL stubMorphTarget(void *geometry, int index)
{
    dtgpLog(0x30f90000u);
    dtgpLogP(geometry);
    dtgpLog((uint32_t)index);
    return (void *)(uintptr_t)0x00620000u;
}

static int BMX_CDECL stubGeometryLock(void *geometry, int mode)
{
    dtgpLog(0x10c40000u);
    dtgpLogP(geometry);
    dtgpLog((uint32_t)mode);
    return (dt_u32() & 3) == 0 ? 0 : (int)dt_u32() | 1;
}

static float *BMX_CDECL stubMorphVertices(void *target)
{
    dtgpLog(0x3e470000u);
    dtgpLogP(target);
    return stubVertices;
}

static int BMX_CDECL stubGeometryUnlock(void *geometry)
{
    dtgpLog(0x04c10000u);
    dtgpLogP(geometry);
    return (dt_u32() & 3) != 0;
}

static ZrwModel *randomModel(void)
{
    ZrwModel *m = dt_alloc(sizeof *m);
    ZrwModelDef *d = dt_alloc(sizeof *d);

    dt_fill(m, sizeof *m);
    dt_fill(d, sizeof *d);
    {
        /* The name may run on into the path (0x90 bytes in all). */
        char *const text = (char *)d + offsetof(ZrwModelDef, name);

        for (int i = dt_range(0, 0x8f); i < 0x90; i++) {
            text[i] = 0;
        }
        for (int i = 0; i < 0x90 && text[i] != 0; i++) {
            text[i] = (char)dt_range('a', 'z');
        }
    }
    stubVertices = dt_alloc(16 * 12);
    d->vertices = (float *)stubVertices;
    m->def = d;
    gZrwScale = dt_float_edgy(0.001f, 100.0f);
    gZrwInvScale = dt_float_edgy(0.001f, 100.0f);
    return m;
}

static void setupModelOnUpdate(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, randomModel());
    DT_ARG_PTR(c, 1, (void *)(uintptr_t)dt_u32());
}

static void setupLock(DtCtx *c)
{
    ZrwModel *m;

    begin(c);
    m = randomModel();
    dt_stub(0x004b43e0, NULL, (void *)stubMorphTarget);
    dt_stub(0x004b4f20, NULL, (void *)stubGeometryLock);
    dt_stub(0x004b45d0, NULL, (void *)stubMorphVertices);
    dt_stub(0x004b4fd0, NULL, (void *)stubGeometryUnlock);
    DT_ARG_PTR(c, 0, m);
    DT_ARG_INT(c, 1, dt_u32());
}

static void setupVertex(DtCtx *c)
{
    ZrwModel *m;
    float *v;
    float *p;

    begin(c);
    m = randomModel();
    v = (float *)stubVertices;
    p = dt_alloc(16);
    for (int i = 0; i < 48; i++) {
        v[i] = dt_float_edgy(-1000.0f, 1000.0f);
    }
    for (int i = 0; i < 3; i++) {
        p[i] = dt_float_edgy(-1000.0f, 1000.0f);
    }
    DT_ARG_PTR(c, 0, m);
    DT_ARG_INT(c, 1, dt_range(1, 16));
    DT_ARG_PTR(c, 2, p);
}

static void setupMapIndex(DtCtx *c)
{
    begin(c);
    DT_ARG_INT(c, 0, (dt_u32() & 3) == 0 ? (int32_t)dt_u32() : dt_range(-5, 100));
}

/* ---- zrwlight -------------------------------------------------------------------------------- */

static void *BMX_CDECL stubListCreate(uint32_t size, uint32_t count, void *destroy)
{
    dtgpLog(0xc4ea0000u);
    dtgpLog(size);
    dtgpLog(count);
    dtgpLogP(destroy);
    return (void *)(uintptr_t)(0x00630000u + dt_range(0, 255) * 0x10u);
}

static void setupLightInit(DtCtx *c)
{
    begin(c);
    gZrwLightInitialized = (dt_u32() & 3) == 0;
    gZrwLightPool = NULL;
    dt_stub(0x00485e80, NULL, (void *)stubListCreate);
}

DT_CASES(zrwfile_cases) = {
    {"zrwFileGetDeviceName", 0x0049e1f0, zrwFileGetDeviceName, 0, DT_RET_INT, setupDeviceName, 20, 0},
    {"zrwFileResolveName", 0x0049e220, zrwFileResolveName, 4, DT_RET_INT, setupResolve, 400, 0},
    {"zrwFileSucceedA", 0x0049e2b0, zrwFileSucceedA, 3, DT_RET_INT, setupSucceed, 300, 0},
    {"zrwFileNotPermitted", 0x0049e350, zrwFileNotPermitted, 0, DT_RET_INT, setupNoArgs, 20, 0},
    {"zrwFileSucceedB", 0x0049e390, zrwFileSucceedB, 3, DT_RET_INT, setupSucceed, 300, 0},
    {"zrwFileNotImplementedA", 0x0049e430, zrwFileNotImplementedA, 0, DT_RET_INT, setupNoArgs, 20, 0},
    {"zrwFileExists", 0x0049e470, zrwFileExists, 4, DT_RET_INT, setupExists, 300, 0},
    {"zrwFileNotImplementedB", 0x0049e510, zrwFileNotImplementedB, 0, DT_RET_INT, setupNoArgs, 20, 0},
    {"zrwFileOpen", 0x0049e550, zrwFileOpen, 4, DT_RET_INT, setupOpen, 500, 0},
    {"zrwFileClose", 0x0049e630, zrwFileClose, 2, DT_RET_INT, setupClose, 300, 0},
    {"zrwFileRead", 0x0049e6d0, zrwFileRead, 4, DT_RET_INT, setupTransfer, 300, 0},
    {"zrwFileWrite", 0x0049e780, zrwFileWrite, 4, DT_RET_INT, setupTransfer, 300, 0},
    {"zrwFileSeek", 0x0049e830, zrwFileSeek, 4, DT_RET_INT, setupSeek, 400, 0},
    {"zrwFileNotImplementedC", 0x0049e910, zrwFileNotImplementedC, 0, DT_RET_INT, setupNoArgs, 20, 0},
    {"zrwFileGetSize", 0x0049e950, zrwFileGetSize, 3, DT_RET_INT, setupSize, 300, 0},
    {"zrwFileSystemInitialize", 0x0049ea10, zrwFileSystemInitialize, 0, DT_RET_INT, setupFsInit, 50, 0},
    {"zrwFileSystemShutdown", 0x0049ea60, zrwFileSystemShutdown, 0, DT_RET_INT, setupFsShutdown, 20, 0},
    {"zrwModelSetOnUpdate", 0x0049dfe0, zrwModelSetOnUpdate, 2, DT_RET_VOID, setupModelOnUpdate, 20, 0},
    {"zrwModelGeomNotCoded", 0x0049dff0, zrwModelGeomNotCoded, 0, DT_RET_INT, setupNoArgs, 20, 0},
    {"zrwModelNextIndex", 0x0049e010, zrwModelNextIndex, 1, DT_RET_INT, setupMapIndex, 50, 0},
    {"zrwModelLockGeometry", 0x0049e020, zrwModelLockGeometry, 2, DT_RET_INT, setupLock, 300, 0},
    {"zrwModelUnlockGeometry", 0x0049e0b0, zrwModelUnlockGeometry, 1, DT_RET_INT, setupLock, 300, 0},
    {"zrwModelGetVertex", 0x0049e110, zrwModelGetVertex, 3, DT_RET_INT, setupVertex, 500, 0},
    {"zrwModelSetVertex", 0x0049e180, zrwModelSetVertex, 3, DT_RET_INT, setupVertex, 500, 0},
    {"zrwLightSystemInitialize", 0x0049f4d0, zrwLightSystemInitialize, 0, DT_RET_INT, setupLightInit, 40, 0},
};
