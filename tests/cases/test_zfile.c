/*
 * Tests for the file layer (zfile.c). The devices are fakes defined here: their callbacks log what they
 * are given and answer from per-iteration settings, so every path of the layer (and its asserts, which
 * end the run as an abort on both sides) can be reached. A separate case runs the layer against the
 * real RenderWare file devices on real game files.
 */
#include <stddef.h>

#include "dt.h"
#include "dt_zmem.h"

#include "compat/crt.h"
#include "zge/com/zglrw/zrwzgl.h"
#include "zge/zdebug.h"
#include "zge/zfile.h"

/* ---- log and fake devices ------------------------------------------------------------------------ */

static uint32_t gLog[1024];
static uint32_t gLogN;

static void logU(uint32_t v)
{
    if (gLogN < sizeof gLog / sizeof gLog[0]) {
        gLog[gLogN] = v;
    }
    gLogN++;
}

static void logStr(const char *s)
{
    uint32_t h = 5381;
    while (*s) {
        h = h * 33 + (uint8_t)*s++;
    }
    logU(h);
}

typedef struct FakeDevice {
    ZfileDevice d; /* first, so a device pointer is a FakeDevice pointer */
    const char *name;
    uint32_t id;
} FakeDevice;

static FakeDevice gDevs[4];
static int gOpenResult, gCloseResult, gSizeResult, gFakeSize;
static uint32_t gControlResult, gOpAnswer;
static uint8_t gShortRead; /* reads return fewer bytes than asked */
static uint32_t gFillSeed;
static const char kRamName[] = "rd00:ram0001";

static uint32_t devId(const ZfileDevice *d)
{
    return ((const FakeDevice *)d)->id;
}

static const char *BMX_CDECL dev_name(ZfileDevice *d)
{
    logU(0x200 + devId(d));
    return ((FakeDevice *)d)->name;
}

static const char *BMX_CDECL dev_control(ZfileDevice *d, uint32_t cmd, const char *const *keys,
                                         const void *block)
{
    const uint32_t a = (uint32_t)(uintptr_t)keys;
    const uint32_t b = (uint32_t)(uintptr_t)block;

    logU(0x300 + devId(d));
    logU(cmd);
    if (cmd == 2) {
        const uint32_t *values = block;
        logStr(keys[0]);
        logStr(keys[1]);
        logU(values[0]);
        logU(values[1]);
    } else {
        logU(dt_canon(a));
        logU(dt_canon(b));
    }
    return (const char *)(uintptr_t)gControlResult;
}

static int BMX_CDECL dev_open(ZfileHandle *f, const char *path, int mode, int flags)
{
    logU(0x400 + devId(f->device));
    logU(dt_canon((uint32_t)(uintptr_t)f));
    logStr(path);
    logU((uint32_t)mode);
    logU((uint32_t)flags);
    return gOpenResult;
}

static int BMX_CDECL dev_close(ZfileHandle *f, int flags)
{
    logU(0x500 + devId(f->device));
    logU((uint32_t)flags);
    return gCloseResult;
}

static int BMX_CDECL dev_read(ZfileHandle *f, void *buf, int count, int flags)
{
    uint8_t *p = buf;
    int got = gShortRead && count > 1 ? count - 1 : count, i;
    logU(0x600 + devId(f->device));
    logU((uint32_t)count);
    logU((uint32_t)flags);
    for (i = 0; i < got; i++) {
        p[i] = (uint8_t)(gFillSeed++ * 7 + 3);
    }
    return got;
}

static int BMX_CDECL dev_write(ZfileHandle *f, const void *buf, int count, int flags)
{
    logU(0x700 + devId(f->device));
    logU((uint32_t)count);
    logU(*(const uint8_t *)buf);
    logU((uint32_t)flags);
    return count - 1;
}

static uint32_t BMX_CDECL dev_op30(ZfileHandle *f, uint32_t a, uint32_t b, const void *c)
{
    logU(0x800 + devId(f->device));
    logU(a);
    logU(b);
    logU((uint32_t)(uintptr_t)c);
    return gOpAnswer;
}

static uint32_t BMX_CDECL dev_op34(ZfileHandle *f, uint32_t *a, const void *b)
{
    logU(0x900 + devId(f->device));
    logU((uint32_t)(uintptr_t)a);
    logU((uint32_t)(uintptr_t)b);
    return gOpAnswer + 1;
}

static uint32_t BMX_CDECL dev_size(ZfileHandle *f, int *size, uint32_t flags)
{
    logU(0xa00 + devId(f->device));
    logU(dt_canon((uint32_t)(uintptr_t)size));
    logU(flags);
    *size = gFakeSize;
    return (uint32_t)gSizeResult;
}

static void BMX_CDECL stub_print(const char *fmt, ...)
{
    const char *const *arg = (const char *const *)((const uint8_t *)&fmt + sizeof fmt);
    logU(0xb00);
    logStr(fmt);
    logStr(*arg); // NOLINT(clang-analyzer-security.ArrayBound)
}

/* Platform setup stub: reports the fake devices. */
static int gPlatformResult;
static int gPlatformDevices;

static int BMX_CDECL stub_platform(void *table)
{
    ZfileDeviceTable *t = table;
    int i;
    logU(0xc00);
    t->count = (uint32_t)gPlatformDevices;
    for (i = 0; i < gPlatformDevices; i++) {
        t->device[i] = &gDevs[i].d;
    }
    return gPlatformResult;
}

/* ---- fixtures ------------------------------------------------------------------------------------ */

static const char *const kNames[] = {"rw00", "aa01", "rd00", "bb02"};

static void make_devices(void)
{
    int i;
    for (i = 0; i < 4; i++) {
        FakeDevice *f = &gDevs[i];
        memset(f, 0, sizeof *f);
        f->d.name = dev_name;
        f->d.control = dev_control;
        f->d.open = dev_open;
        f->d.close = dev_close;
        f->d.read = dev_read;
        f->d.write = dev_write;
        f->d.op30 = dev_op30;
        f->d.op34 = dev_op34;
        f->d.getSize = dev_size;
        f->name = kNames[i];
        f->id = (uint32_t)i;
    }
}

static void begin(DtCtx *c, int init)
{
    int n, i;
    dt_seed(c);
    dt_zmem_env();
    gZdebugPrint = stub_print;
    gLogN = 0;
    make_devices();
    n = dt_range(1, 4);
    memset(&gZfileDevices, 0, sizeof gZfileDevices);
    gZfileDevices.count = (uint32_t)n;
    for (i = 0; i < n; i++) {
        gZfileDevices.device[i] = &gDevs[i].d;
    }
    gZfileDefaultDevice = dt_u32() % 8 == 0 ? NULL : &gDevs[dt_range(0, n - 1)].d;
    gZfileInitialized = init;
    gZfileHandles = DT_ORIG(0x00485e80, ZMemFreeList * (*)(uint32_t, uint32_t, void *))(8, 8, NULL);
    gZfileLoadPool = NULL;
    gZfileRamDisk = -1;
    memset(gZfileErrorMessage, 0, sizeof gZfileErrorMessage);
    gZfileErrorMessage[0] = dt_u32() & 1 ? 'x' : '\0';
    gOpenResult = (int)(dt_u32() % 5 != 0);
    gCloseResult = (int)(dt_u32() % 5 != 0);
    gSizeResult = dt_u32() % 5 != 0;
    gFakeSize = dt_range(0, 100);
    gControlResult = 0;
    gOpAnswer = dt_u32();
    gShortRead = dt_u32() % 5 == 0;
    gFillSeed = dt_u32();
}

static int init_flag(DtCtx *c)
{
    return c->seed % 17 != 5; /* now and then: "not initialized" */
}

static ZfileHandle *new_handle(void)
{
    ZfileHandle *h = dt_alloc(sizeof *h);
    h->data = (void *)(uintptr_t)dt_u32();
    h->device = dt_u32() % 9 == 0 ? NULL : &gDevs[dt_range(0, (int)gZfileDevices.count - 1)].d;
    return h;
}

/* A path: sometimes a device prefix, sometimes long or empty. */
static char *rand_path(void)
{
    char *s = dt_alloc(160);
    int len = dt_range(0, 30), i = 0;
    switch (dt_u32() % 9) {
    case 0:
        len = dt_range(120, 140); /* long: the length assert */
        break;
    case 1:
    case 2:
    case 3: {
        const char *prefix = kNames[dt_range(0, 3)];
        memcpy(s, prefix, 4);
        s[4] = ':';
        i = 5;
        break;
    }
    case 4:
        memcpy(s, "zz99:", 6); /* unknown device name */
        i = 5;
        break;
    default:
        break;
    }
    for (; i < len + 5 && i < 159; i++) {
        s[i] = (char)dt_range('a', 'z');
        if (dt_u32() % 12 == 0) {
            s[i] = dt_u32() & 1 ? '.' : '\\';
        }
    }
    return s;
}

/* ---- setups -------------------------------------------------------------------------------------- */

static void setup_none(DtCtx *c)
{
    begin(c, init_flag(c));
}

static void setup_error_message(DtCtx *c)
{
    char *s;
    int len = dt_range(0, 80);
    int i;
    begin(c, 1);
    if (c->seed % 4 == 0) {
        len = 61 + (int)(c->seed % 4); /* around the 0x3f limit */
    }
    s = dt_alloc(96);
    for (i = 0; i < len; i++) {
        s[i] = (char)dt_range('a', 'z');
    }
    DT_ARG_PTR(c, 0, s);
}

static void setup_initialize(DtCtx *c)
{
    begin(c, 0);
    dt_stub(0x0048a940, NULL, (void *)stub_platform);
    gPlatformResult = c->seed % 9 != 4;
    gPlatformDevices = c->seed % 11 == 6 ? 0 : dt_range(1, 4);
    gZfileHandles = c->seed % 13 == 3
                        ? DT_ORIG(0x00485e80, ZMemFreeList * (*)(uint32_t, uint32_t, void *))(8, 8, NULL)
                        : NULL;
    gZfileDefaultDevice = NULL;
    memset(&gZfileDevices, 0, sizeof gZfileDevices);
}

static void setup_handle1(DtCtx *c)
{
    begin(c, init_flag(c));
    DT_ARG_PTR(c, 0, new_handle());
}

static void setup_destroy(DtCtx *c)
{
    setup_handle1(c);
    ((ZfileHandle *)(uintptr_t)c->args[0])->data = NULL;
}

static void setup_device_arg(DtCtx *c)
{
    begin(c, init_flag(c));
    DT_ARG_PTR(c, 0, c->seed % 7 == 3 ? NULL : (void *)&gDevs[dt_range(0, (int)gZfileDevices.count - 1)].d);
}

static void setup_device_control(DtCtx *c)
{
    setup_device_arg(c);
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_INT(c, 2, dt_u32());
    DT_ARG_INT(c, 3, dt_u32());
}

static void setup_path(DtCtx *c)
{
    begin(c, init_flag(c));
    DT_ARG_PTR(c, 0, c->seed % 23 == 7 ? NULL : rand_path());
}

/* A NULL path is not tested: the original measures it before checking it. */
static void setup_open(DtCtx *c)
{
    setup_path(c);
    if (c->args[0] == 0) {
        DT_ARG_PTR(c, 0, rand_path());
    }
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_INT(c, 2, dt_u32());
}

static void setup_handle_flags(DtCtx *c)
{
    setup_handle1(c);
    DT_ARG_INT(c, 1, dt_u32());
}

static void setup_read(DtCtx *c)
{
    setup_handle1(c);
    DT_ARG_PTR(c, 1, dt_alloc(128));
    DT_ARG_INT(c, 2, c->seed % 5 == 0 ? 0 : dt_range(1, 100));
    DT_ARG_INT(c, 3, dt_u32());
}

static void setup_op30(DtCtx *c)
{
    setup_handle1(c);
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_INT(c, 2, dt_u32());
    DT_ARG_INT(c, 3, dt_u32());
}

static void setup_size(DtCtx *c)
{
    setup_handle1(c);
    DT_ARG_PTR(c, 1, dt_alloc(8));
    DT_ARG_INT(c, 2, dt_u32());
}

static void setup_load(DtCtx *c)
{
    setup_open(c);
    DT_ARG_PTR(c, 1, c->seed % 3 == 0 ? NULL : dt_alloc(8));
    gOpenResult = c->seed % 7 != 4;
}

static void setup_load_bytes(DtCtx *c)
{
    setup_open(c);
    DT_ARG_INT(c, 1, dt_range(0, 100));
    gOpenResult = c->seed % 7 != 4;
}

static void setup_free(DtCtx *c)
{
    begin(c, 1);
    DT_ARG_PTR(c, 0, DT_ORIG(0x00485aa0, void *(*)(uint32_t))(dt_range(1, 64)));
}

static void setup_gets(DtCtx *c)
{
    char *buf;
    setup_handle1(c);
    buf = dt_alloc(64);
    buf[0] = dt_u32() & 1 ? 'q' : '\0';
    DT_ARG_PTR(c, 1, buf);
    DT_ARG_INT(c, 2, c->seed % 9 == 0 ? 0 : dt_range(1, 40));
}

static void setup_build(DtCtx *c)
{
    char *a = dt_alloc(160), *b = dt_alloc(160);
    int la = dt_range(0, 60), lb = dt_range(0, 60), i;
    begin(c, 1);
    if (c->seed % 6 == 1) {
        la = 100;
        lb = 40; /* too long: the corruption assert */
    }
    for (i = 0; i < la; i++) {
        a[i] = (char)dt_range('a', 'z');
    }
    for (i = 0; i < lb; i++) {
        b[i] = (char)dt_range('a', 'z');
    }
    DT_ARG_PTR(c, 0, a);
    DT_ARG_PTR(c, 1, b);
}

/* path, out for the directory and extension helpers */
static void setup_split(DtCtx *c)
{
    char *p;
    dt_seed(c);
    begin(c, 1);
    p = rand_path();
    if (c->seed % 7 == 4) {
        p[0] = '\0';
    }
    DT_ARG_PTR(c, 0, c->seed % 19 == 6 ? NULL : p);
    DT_ARG_PTR(c, 1, dt_alloc(160));
}

static void setup_ram_disk(DtCtx *c)
{
    begin(c, init_flag(c));
    gControlResult = c->seed % 5 == 3 ? 0 : (uint32_t)(uintptr_t)kRamName;
    if (c->seed % 4 == 1) {
        gZfileRamDisk = (intptr_t)&gDevs[dt_range(0, (int)gZfileDevices.count - 1)].d;
    }
    DT_ARG_INT(c, 0, dt_u32());
    DT_ARG_INT(c, 1, dt_u32());
}

DT_CASES(zfile_cases) = {
    {"zfileRegisterErrorMessage", 0x00483a60, zfileRegisterErrorMessage, 1, DT_RET_VOID, setup_error_message,
     200, 0},
    {"zfileInitialize", 0x00483aa0, zfileInitialize, 0, DT_RET_INT, setup_initialize, 200, 0},
    {"zfileDestroyHandle", 0x00483b80, zfileDestroyHandle, 1, DT_RET_INT, setup_destroy, 100, 0},
    {"zfileGetDevices", 0x00483cb0, zfileGetDevices, 0, DT_RET_INT, setup_none, 20, 0},
    {"zfileDeviceName", 0x00483ce0, zfileDeviceName, 1, DT_RET_INT, setup_device_arg, 100, 0},
    {"zfileDeviceControl", 0x00483d30, zfileDeviceControl, 4, DT_RET_INT, setup_device_control, 100, 0},
    {"zfileFindDevice", 0x00483d90, zfileFindDevice, 1, DT_RET_INT, setup_path, 400, 0},
    {"zfileDefaultDevice", 0x00483f10, zfileDefaultDevice, 0, DT_RET_INT, setup_none, 100, 0},
    {"zfileOpen", 0x00484700, zfileOpen, 3, DT_RET_INT, setup_open, 600, 0},
    {"zfileClose", 0x00484a60, zfileClose, 2, DT_RET_INT, setup_handle_flags, 200, 0},
    {"zfileRead", 0x00484b20, zfileRead, 4, DT_RET_INT, setup_read, 200, 0},
    {"zfileWrite", 0x00484b90, zfileWrite, 4, DT_RET_INT, setup_read, 200, 0},
    {"zfileOp30", 0x00484c00, zfileOp30, 4, DT_RET_INT, setup_op30, 100, 0},
    {"zfileOp34", 0x00484c70, zfileOp34, 3, DT_RET_INT, setup_op30, 100, 0},
    {"zfileGetSize", 0x00484cd0, zfileGetSize, 3, DT_RET_INT, setup_size, 100, 0},
    {"zfileLoad", 0x00484d30, zfileLoad, 2, DT_RET_INT, setup_load, 500, 0},
    {"zfileLoadBytes", 0x00485070, zfileLoadBytes, 2, DT_RET_INT, setup_load_bytes, 500, 0},
    {"zfileFreeBuffer", 0x00485370, zfileFreeBuffer, 1, DT_RET_VOID, setup_free, 20, 0},
    {"zfileGets", 0x004853c0, zfileGets, 3, DT_RET_INT, setup_gets, 300, 0},
    {"zfileBuildFileName", 0x00485460, zfileBuildFileName, 2, DT_RET_INT, setup_build, 200, 0},
    {"zfileGetDirectory", 0x004854d0, zfileGetDirectory, 2, DT_RET_INT, setup_split, 300, 0},
    {"zfileGetExtension", 0x00485520, zfileGetExtension, 2, DT_RET_INT, setup_split, 300, 0},
    {"zfileOpenRamDisk", 0x00485580, zfileOpenRamDisk, 2, DT_RET_INT, setup_ram_disk, 400, 0},
};

/* ---- real devices on real files -------------------------------------------------------------------- */

#define gRwEngine ORIG_VAR(0x0067696c, uint8_t *, gRwEngine)

/* A minimal engine instance whose file interface (+0xc0) is the exe's CRT stdio wrappers. */
static void real_env(DtCtx *c, int initialize)
{
    static const uint32_t kRwFile[10] = {0x4e8290, 0x4a5c98, 0x4a5cab, 0x51db3d, 0x51dc25,
                                         0x51dd2f, 0x51d4d2, 0x51dd86, 0x4a61ad, 0x4a8370};
    uint8_t *inst;
    dt_seed(c);
    dt_zmem_env();
    gZdebugPrint = stub_print;
    gLogN = 0;
    inst = dt_alloc(0x400);
    memcpy(inst + 0xc0, kRwFile, sizeof kRwFile);
    gRwEngine = inst;
    if (initialize) {
        DT_ORIG(0x00483aa0, int (*)(void))();
    }
}

static const char *const kRealFiles[] = {"Assets\\Scene.Txt",      "Program_Files\\Scene.Txt",
                                         "Assets\\Intwrap.zal",    "rw00:Assets\\Scene.Txt",
                                         "Assets\\NoSuchFile.bin", "nowhere\\x.dat"};

static char *real_path(void)
{
    const char *src = kRealFiles[dt_range(0, 5)];
    char *p = dt_alloc(64);
    memcpy(p, src, strlen(src) + 1);
    return p;
}

static void setup_real_init(DtCtx *c)
{
    real_env(c, 0);
}

static void setup_real_path(DtCtx *c)
{
    real_env(c, 1);
    DT_ARG_PTR(c, 0, real_path());
}

static void setup_real_open(DtCtx *c)
{
    setup_real_path(c);
    DT_ARG_INT(c, 1, 1);
    DT_ARG_INT(c, 2, 0);
}

static void setup_real_load(DtCtx *c)
{
    setup_real_path(c);
    DT_ARG_PTR(c, 1, dt_alloc(8));
}

static void setup_real_load_bytes(DtCtx *c)
{
    setup_real_path(c);
    DT_ARG_INT(c, 1, dt_range(0, 3000));
}

/* An open handle of a real file, made by the original code. */
static ZfileHandle *real_handle(void)
{
    return DT_ORIG(0x00484700, ZfileHandle * (*)(const char *, int, int))("Assets\\Scene.Txt", 1, 0);
}

static void setup_real_gets(DtCtx *c)
{
    real_env(c, 1);
    DT_ARG_PTR(c, 0, real_handle());
    DT_ARG_PTR(c, 1, dt_alloc(256));
    DT_ARG_INT(c, 2, dt_range(1, 200));
}

static void setup_real_read(DtCtx *c)
{
    real_env(c, 1);
    DT_ARG_PTR(c, 0, real_handle());
    DT_ARG_PTR(c, 1, dt_alloc(4096));
    DT_ARG_INT(c, 2, dt_range(0, 4096));
    DT_ARG_INT(c, 3, 0);
}

static void setup_real_close(DtCtx *c)
{
    real_env(c, 1);
    DT_ARG_PTR(c, 0, real_handle());
    DT_ARG_INT(c, 1, 0);
}

static void setup_real_size(DtCtx *c)
{
    real_env(c, 1);
    DT_ARG_PTR(c, 0, real_handle());
    DT_ARG_PTR(c, 1, dt_alloc(8));
    DT_ARG_INT(c, 2, 0);
}

static void setup_real_ram(DtCtx *c)
{
    real_env(c, 1);
    DT_ARG_PTR(c, 0, dt_alloc(256));
    DT_ARG_INT(c, 1, 256);
}

DT_CASES(zfile_real_cases) = {
    {"zfileInitialize_real", 0x00483aa0, zfileInitialize, 0, DT_RET_INT, setup_real_init, 5, 0},
    {"zfileFindDevice_real", 0x00483d90, zfileFindDevice, 1, DT_RET_INT, setup_real_path, 50, 0},
    {"zfileOpen_real", 0x00484700, zfileOpen, 3, DT_RET_INT | DT_NEEDS_ASSETS, setup_real_open, 50, 0},
    {"zfileLoad_real", 0x00484d30, zfileLoad, 2, DT_RET_INT | DT_NEEDS_ASSETS, setup_real_load, 50, 0},
    {"zfileLoadBytes_real", 0x00485070, zfileLoadBytes, 2, DT_RET_INT | DT_NEEDS_ASSETS,
     setup_real_load_bytes, 50, 0},
    {"zfileGets_real", 0x004853c0, zfileGets, 3, DT_RET_INT | DT_NEEDS_ASSETS, setup_real_gets, 50, 0},
    {"zfileRead_real", 0x00484b20, zfileRead, 4, DT_RET_INT | DT_NEEDS_ASSETS, setup_real_read, 50, 0},
    {"zfileClose_real", 0x00484a60, zfileClose, 2, DT_RET_INT | DT_NEEDS_ASSETS, setup_real_close, 20, 0},
    {"zfileGetSize_real", 0x00484cd0, zfileGetSize, 3, DT_RET_INT | DT_NEEDS_ASSETS, setup_real_size, 20, 0},
    {"zfileOpenRamDisk_real", 0x00485580, zfileOpenRamDisk, 2, DT_RET_INT, setup_real_ram, 20, 0},
};
