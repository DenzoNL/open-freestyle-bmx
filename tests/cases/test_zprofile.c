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
};
