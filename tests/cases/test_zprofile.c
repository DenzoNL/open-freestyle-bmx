/*
 * Tests for zprofile.c: the file callbacks RenderWare calls while a texture is read from memory, their
 * installation, and the profiler initialisation. The zfile layer and the platform side are stubbed; the
 * stubs log what they get and answer from the seeded generator.
 */
#include "dt.h"
#include "dt_ngif.h"
#include "zge/zprofile.h"

static uint32_t BMX_CDECL stubClose(ZfileHandle *file, int flags)
{
    ngLog(0x484a60);
    ngLogPtr(file);
    ngLog((uint32_t)flags);
    return dt_u32();
}

static int BMX_CDECL stubRead(ZfileHandle *file, void *buffer, int count, int flags)
{
    ngLog(0x484b20);
    ngLogPtr(file);
    ngLogPtr(buffer);
    ngLog((uint32_t)count);
    ngLog((uint32_t)flags);
    return (int)dt_u32();
}

static int BMX_CDECL stubWrite(ZfileHandle *file, const void *buffer, int count, int flags)
{
    ngLog(0x484b90);
    ngLogPtr(file);
    ngLogPtr(buffer);
    ngLog((uint32_t)count);
    ngLog((uint32_t)flags);
    return (int)dt_u32();
}

static char *BMX_CDECL stubGets(ZfileHandle *file, char *buffer, int size)
{
    ngLog(0x4853c0);
    ngLogPtr(file);
    ngLogPtr(buffer);
    ngLog((uint32_t)size);
    return (char *)(uintptr_t)dt_u32();
}

/* The position and size answers repeat sometimes, so that both sides of the comparison are reached. */
static uint32_t gSamePos;

static uint32_t BMX_CDECL stubOp34(ZfileHandle *file, uint32_t a, uint32_t b)
{
    ngLog(0x484c70);
    ngLogPtr(file);
    ngLog(dt_canon(a));
    ngLog(b);
    gSamePos = (dt_u32() % 3 == 0) ? 0x1000 : dt_u32();
    *(uint32_t *)(uintptr_t)a = gSamePos;
    return dt_u32();
}

static uint32_t BMX_CDECL stubGetSize(ZfileHandle *file, int *size, uint32_t flags)
{
    ngLog(0x484cd0);
    ngLogPtr(file);
    ngLog(dt_canon((uint32_t)(uintptr_t)size));
    ngLog(flags);
    *(uint32_t *)size = (dt_u32() % 3 == 0) ? gSamePos : dt_u32();
    return dt_u32();
}

static uint32_t BMX_CDECL stubOp30(ZfileHandle *file, uint32_t a, uint32_t b, uint32_t c)
{
    ngLog(0x484c00);
    ngLogPtr(file);
    ngLog(a);
    ngLog(b);
    ngLog(c);
    return dt_u32();
}

static void install_file_stubs(void)
{
    dt_stub(0x00484a60, NULL, (void *)stubClose);
    dt_stub(0x00484b20, NULL, (void *)stubRead);
    dt_stub(0x00484b90, NULL, (void *)stubWrite);
    dt_stub(0x004853c0, NULL, (void *)stubGets);
    dt_stub(0x00484c70, NULL, (void *)stubOp34);
    dt_stub(0x00484cd0, NULL, (void *)stubGetSize);
    dt_stub(0x00484c00, NULL, (void *)stubOp30);
}

static ZfileHandle **make_slot(void)
{
    ZfileHandle **slot = (ZfileHandle **)dt_alloc(sizeof *slot);

    *slot = (ZfileHandle *)(uintptr_t)dt_u32();
    return slot;
}

static void begin(DtCtx *c)
{
    ngBegin(c, 0);
    install_file_stubs();
}

static void setup_close(DtCtx *c)
{
    begin(c);
    gZrwRamDiskInfo = (ZrwRamDiskInfo *)(uintptr_t)(dt_u32() & ~3u);
    DT_ARG_PTR(c, 0, make_slot());
}

static void setup_rw(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, dt_alloc(64));
    DT_ARG_INT(c, 1, (dt_u32() % 4 == 0) ? dt_u32() : (uint32_t)dt_range(0, 100));
    DT_ARG_INT(c, 2, (dt_u32() % 4 == 0) ? dt_u32() : (uint32_t)dt_range(0, 100));
    DT_ARG_PTR(c, 3, make_slot());
}

static void setup_gets(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, dt_alloc(64));
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_PTR(c, 2, make_slot());
}

static void setup_noargs(DtCtx *c)
{
    begin(c);
}

static void setup_eof(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, make_slot());
}

static void setup_seek(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, make_slot());
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_INT(c, 2, dt_u32());
}

/* ---- installation ---------------------------------------------------------------------------------- */

static void **gInterface;

static void **BMX_CDECL stubGetInterface(void)
{
    ngLog(0x4e8280);
    return gInterface;
}

static void begin_interface(DtCtx *c)
{
    ngBegin(c, 0);
    gInterface = (void **)dt_alloc(sizeof(ZrwFileInterface));
    dt_fill((void *)gInterface, sizeof(ZrwFileInterface));
    dt_fill(&gZrwSavedFileInterface, sizeof(ZrwFileInterface));
    dt_stub(0x004e8280, NULL, (void *)stubGetInterface);
    gZrwRamDiskInfo = (ZrwRamDiskInfo *)(uintptr_t)dt_u32();
    gZrwRamDiskNoInfo = (ZrwRamDiskInfo *)(uintptr_t)dt_u32();
    gZrwRamDiskFile = (ZfileHandle *)(uintptr_t)dt_u32();
}

static void setup_register(DtCtx *c)
{
    begin_interface(c);
}

static void setup_image_buffer(DtCtx *c)
{
    ngBegin(c, 0);
    gZrwRamDiskNoInfo = dt_alloc(4 * sizeof(ZrwRamDiskInfo));
    DT_ARG_PTR(c, 0, (dt_u32() & ~3u));
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_PTR(c, 2, (dt_u32() & 1) ? NULL : (const char *)"ZMD");
}

/* ---- profiler -------------------------------------------------------------------------------------- */

static int BMX_CDECL stubPcProfile(void)
{
    ngLog(0x4a45e0);
    return (dt_u32() % 4 != 0);
}

static void *BMX_CDECL stubFreeListCreate(uint32_t size, uint32_t count, void *destroy)
{
    ngLog(0x485e80);
    ngLog(size);
    ngLog(count);
    ngLog(dt_canon((uint32_t)(uintptr_t)destroy));
    return (void *)(uintptr_t)(dt_u32() | 4u);
}

static void *BMX_CDECL stubRegister(int type, void *fn, void *arg, int append)
{
    ngLog(0x48ba30);
    ngLog((uint32_t)type);
    ngLog(dt_canon((uint32_t)(uintptr_t)fn));
    ngLogPtr(arg);
    ngLog((uint32_t)append);
    return (void *)(uintptr_t)dt_u32();
}

static void setup_profile_init(DtCtx *c)
{
    ngBegin(c, 1);
    dt_fill(&gZProfile, sizeof gZProfile);
    if (dt_u32() & 1) {
        gZProfile.initialized = 0;
    }
    if (dt_u32() & 1) {
        gZProfile.updateNode = NULL;
    }
    dt_stub(0x004a45e0, NULL, (void *)stubPcProfile);
    dt_stub(0x00485e80, NULL, (void *)stubFreeListCreate);
    dt_stub(0x0048ba30, NULL, (void *)stubRegister);
}

/* ---- the per-frame update ------------------------------------------------------------------------- */

#define gScreenWT   ORIG_VAR(0x00697770, uint32_t, gScreenWT)
#define gFrequencyT ORIG_VAR(0x006764d8, int64_t, gFrequencyT)
#define gDrawTriT   ORIG_VAR(0x0056172c, void *, gDrawTriT)
#define gDrawQuadT  ORIG_VAR(0x00561738, void *, gDrawQuadT)
#define gPrintAtT   ORIG_VAR(0x00560038, void *, gPrintAtT)
typedef uint32_t ProfileColorsT[5];
#define gProfileColorsT ORIG_VAR(0x00563904, ProfileColorsT, gProfileColorsT)

/* Logs a draw call: the record's texture word and x, y and colour of its `n` vertices (nothing else of them
 * is set). */
static void logPrims(uint32_t flags, const uint32_t *record, int count, const void *matrix, int zero, int n)
{
    ngLog(flags);
    ngLogPtr(record);
    ngLog((uint32_t)count);
    ngLogPtr(matrix);
    ngLog((uint32_t)zero);
    ngLog(record[0]);
    for (int i = 0; i < n; i++) {
        const float *v = (const float *)(uintptr_t)record[1 + i];

        ngLogPtr(v);
        ngLogFloat(v[0]);
        ngLogFloat(v[1]);
        ngLogFloat(v[6]);
    }
}

static void BMX_CDECL stubDrawQuads(uint32_t flags, const uint32_t *record, int count, const void *matrix,
                                    int zero)
{
    ngLog(0x561738);
    logPrims(flags, record, count, matrix, zero, 4);
}

static void BMX_CDECL stubDrawTriangles(uint32_t flags, const uint32_t *record, int count, const void *matrix,
                                        int zero)
{
    ngLog(0x56172c);
    logPrims(flags, record, count, matrix, zero, 3);
}

static int BMX_CDECL stubPrintAt(int x, int y, const char *text)
{
    ngLog(0x560038);
    ngLog((uint32_t)x);
    ngLog((uint32_t)y);
    ngLog(ngHash(text));
    return 1;
}

static int32_t BMX_CDECL stubElapsed(const int32_t *start)
{
    ngLog(0x4a46a0);
    ngLogPtr(start);
    return (int32_t)(dt_u32() % 50000);
}

/* A budget: whole, tenths, hundredths, thousandths, the step limits themselves, 0 and below. */
static float profile_budget(void)
{
    static const float kFixed[8] = {1.0f, 0.1f, 0.01f, 0.0f, -0.0f, -1.0f, 0.001f, 2.5f};

    switch (dt_u32() % 6) {
    case 0:
        return kFixed[dt_u32() % 8];
    case 1:
        return dt_float(1.0f, 12.0f);
    case 2:
        return dt_float(0.1f, 1.0f);
    case 3:
        return dt_float(0.01f, 0.1f);
    case 4:
        return dt_float(0.0f, 0.01f);
    default:
        return (float)dt_range(1, 6);
    }
}

static void setup_profile_update(DtCtx *c)
{
    ZMemFreeList *list = dt_alloc(sizeof *list);
    const int n = dt_range(0, 4);
    ZMemFreeNode *prev = NULL;

    ngBegin(c, 0);
    dt_stub(0x004a46a0, NULL, (void *)stubElapsed);
    gDrawQuadT = (void *)stubDrawQuads;
    gDrawTriT = (void *)stubDrawTriangles;
    gPrintAtT = (void *)stubPrintAt;
    for (int i = 0; i < 5; i++) {
        gProfileColorsT[i] = dt_u32();
    }
    gScreenWT = (dt_u32() % 8 == 0) ? dt_u32() % 64 : (uint32_t)dt_range(320, 1600);
    gFrequencyT = (dt_u32() % 16 == 0) ? 0 : (int64_t)dt_range(1000, 0x7fffffff) * dt_range(1, 8);
    dt_fill(&gZProfile, sizeof gZProfile);
    gZProfile.holdFrames = (dt_u32() & 1) ? 0 : dt_range(1, 3);
    gZProfile.peakResetFrames = (dt_u32() & 1) ? 0 : dt_range(1, 4);
    gZProfile.peakResetCountdown = dt_range(-1, 2);
    gZProfile.x = dt_float_edgy(0.0f, 100.0f);
    gZProfile.y = dt_float_edgy(0.0f, 400.0f);
    gZProfile.margin = (dt_u32() % 8 == 0) ? dt_u32() : (uint32_t)dt_range(0, 64);
    gZProfile.barHeight = (uint32_t)dt_range(0, 16);
    gZProfile.timeBarHeight = (uint32_t)dt_range(0, 16);
    gZProfile.barGap = (dt_u32() % 8 == 0) ? dt_u32() : (uint32_t)dt_range(0, 4);
    gZProfile.tickAbove = (uint32_t)dt_range(0, 8);
    gZProfile.tickWidth = (dt_u32() % 8 == 0) ? dt_u32() : (uint32_t)dt_range(0, 16);
    gZProfile.tickBelow = (uint32_t)dt_range(0, 8);
    gZProfile.unitsPerSecond = (dt_u32() % 4 == 0) ? dt_u32() : 0x3c;
    dt_fill(list, sizeof *list);
    list->usedHead = NULL;
    for (int i = 0; i < n; i++) {
        ZMemFreeNode *node = dt_alloc(sizeof *node + sizeof(ZProfileEntry));
        ZProfileEntry *e = (ZProfileEntry *)(node + 1);

        dt_fill(e, sizeof *e);
        node->next = NULL;
        node->prev = prev;
        if (prev == NULL) {
            list->usedHead = node;
        } else {
            prev->next = node;
        }
        prev = node;
        e->name = (dt_u32() % 3 == 0) ? NULL : dt_rand_str(12, 0);
        e->ticks = (dt_u32() & 1) ? dt_u32() % 100000 : dt_u32();
        e->peak = (dt_u32() & 1) ? dt_u32() % 100000 : dt_u32();
        e->budget = profile_budget();
    }
    gZProfile.entries = list;
    DT_ARG_PTR(c, 0, dt_alloc(4));
}

DT_CASES(zprofile_cases) = {
    {"zrwfclose", 0x004a33c0, zrwfclose, 1, DT_RET_INT, setup_close, 100, 0},
    {"zrwfread", 0x004a33f0, zrwfread, 4, DT_RET_INT, setup_rw, 200, 0},
    {"zrwfwrite", 0x004a3420, zrwfwrite, 4, DT_RET_INT, setup_rw, 200, 0},
    {"zrwfgets", 0x004a3450, zrwfgets, 3, DT_RET_INT, setup_gets, 100, 0},
    {"zrwfflush", 0x004a3470, zrwfflush, 0, DT_RET_INT, setup_noargs, 20, 0},
    {"zrwfputs", 0x004a3480, zrwfputs, 0, DT_RET_INT, setup_noargs, 20, 0},
    {"zrwfeof", 0x004a3490, zrwfeof, 1, DT_RET_INT, setup_eof, 300, 0},
    {"zrwfseek", 0x004a34e0, zrwfseek, 3, DT_RET_INT, setup_seek, 100, 0},
    {"RegisterCallBacksForFileSystem", 0x004a3500, RegisterCallBacksForFileSystem, 0, DT_RET_VOID,
     setup_register, 50, 0},
    {"ImageBufferSizeType", 0x004a3580, ImageBufferSizeType, 3, DT_RET_VOID, setup_image_buffer, 100, 0},
    {"zrwFileRamDiskClear", 0x004a35c0, zrwFileRamDiskClear, 0, DT_RET_VOID, setup_register, 50, 0},
    {"zprofileInitialize", 0x004a3b60, zprofileInitialize, 0, DT_RET_INT, setup_profile_init, 200, 0},
    {"zprofileUpdate", 0x004a35f0, zprofileUpdate, 1, DT_RET_INT, setup_profile_update, 2000, 0},
};
