/*
 * Tests for the asset library loader (zasset.c). The real libraries of the install (Assets\*.zal) are opened
 * and every one of their assets is selected, sized and read (the iteration number walks through them
 * all, compressed ones included); libraries made up in memory (served by the RAM disk device) cover the
 * corrupt, truncated, old format, compressed and linked ("ref mode") layouts.
 */
#include "dt.h"
#include "dt_ngif.h"
#include "dt_realfile.h"
#include "zge/zasset.h"

typedef int(BMX_CDECL *OpenFn)(const char *);
typedef void(BMX_CDECL *CloseFn)(int);
typedef int(BMX_CDECL *FindFn)(int, int, int);
typedef uint32_t(BMX_CDECL *SizeFn)(int);
typedef int(BMX_CDECL *ReadFn)(int, void *);

#define ORIG_OPEN  DT_ORIG(0x004804d0, OpenFn)
#define ORIG_CLOSE DT_ORIG(0x004806c0, CloseFn)
#define ORIG_FIND  DT_ORIG(0x00480780, FindFn)
#define ORIG_SIZE  DT_ORIG(0x00480850, SizeFn)

static const struct {
    const char *path;
    uint32_t groups; /* one asset per group in all of them */
} kLibs[] = {
    {"Assets\\Intwrap.zal", 5}, {"Assets\\Material.zal", 1262}, {"Assets\\MiscData.ZAL", 21},
    {"Assets\\Motion.zal", 1},  {"Assets\\Replay.ZAL", 24},     {"Assets\\Tables.ZAL", 1},
};
enum { kLibCount = sizeof kLibs / sizeof kLibs[0], kTotalAssets = 5 + 1262 + 21 + 1 + 24 + 1 };

/* The library and group of the n-th asset of all libraries (n wraps around). */
static void locate(uint32_t n, int *lib, uint32_t *group)
{
    n %= kTotalAssets;
    for (int i = 0; i < kLibCount; i++) {
        if (n < kLibs[i].groups) {
            *lib = i;
            *group = n;
            return;
        }
        n -= kLibs[i].groups;
    }
}

/* The id of the group at `index` of an open library (0 when the library is empty). */
static uint32_t group_id(int lib, uint32_t index)
{
    const ZassetLibrary *l = (const ZassetLibrary *)(uintptr_t)lib;
    const uint32_t stride = (uint32_t)l->perGroup * l->entryBytes + 4;

    return *(const uint32_t *)(l->directory + stride * index);
}

/* ---- opening and closing -------------------------------------------------------------------------- */

static void setup_open_real(DtCtx *c)
{
    const uint32_t k = c->seed % 12;

    dt_real_env(c, 1);
    if (c->seed % 3 == 0) {
        ORIG_OPEN("Assets\\Tables.ZAL"); /* a library is open already: the list exists */
    }
    if (k < kLibCount) {
        DT_ARG_PTR(c, 0, kLibs[k].path);
    } else if (k == 6) {
        DT_ARG_PTR(c, 0, "Assets\\NoSuchLibrary.zal");
    } else if (k == 7) {
        DT_ARG_PTR(c, 0, "Assets\\Tables.zal"); /* same file, other spelling */
    } else {
        DT_ARG_PTR(c, 0, "bin\\nowhere\\x.zal");
    }
}

static void setup_close_real(DtCtx *c)
{
    dt_real_env(c, 1);
    if (c->seed % 2 == 0) {
        ORIG_OPEN("Assets\\Tables.ZAL"); /* another library keeps the list alive */
    }
    DT_ARG_INT(c, 0, ORIG_OPEN(kLibs[c->seed % kLibCount].path));
}

/* ---- walking all real assets -------------------------------------------------------------------- */

/* Opens the library of the n-th asset and selects it. Returns the library; the group goes to *group. */
static int open_asset(uint32_t n, uint32_t *group)
{
    int idx;
    int lib;

    locate(n, &idx, group);
    lib = ORIG_OPEN(kLibs[idx].path);
    return lib;
}

static void setup_find_real(DtCtx *c)
{
    uint32_t group;
    int lib;
    const uint32_t k = dt_u32() % 8;

    dt_real_env(c, 1);
    lib = open_asset(c->seed, &group);
    DT_ARG_INT(c, 0, lib);
    if (k < 4) {
        DT_ARG_INT(c, 1, group_id(lib, group)); /* a group by id */
        DT_ARG_INT(c, 2, (dt_u32() & 1) ? 0 : (dt_u32() & 0xffff0000u));
    } else if (k < 6) {
        DT_ARG_INT(c, 1, (int)(dt_u32() % 40000)); /* a missing id, or one found by the scan */
        DT_ARG_INT(c, 2, dt_u32());
    } else {
        DT_ARG_INT(c, 1, (int)group_id(lib, group) + 1);
        DT_ARG_INT(c, 2, dt_range(0, 3));
    }
}

static void setup_dump_real(DtCtx *c)
{
    uint32_t group;

    dt_real_env(c, 1);
    DT_ARG_INT(c, 0, open_asset(c->seed, &group));
}

static void setup_count_real(DtCtx *c)
{
    uint32_t group;

    dt_real_env(c, 1);
    DT_ARG_INT(c, 0, open_asset(c->seed, &group));
}

static void setup_next_real(DtCtx *c)
{
    uint32_t group;
    int lib;

    dt_real_env(c, 1);
    lib = open_asset(c->seed, &group);
    ORIG_FIND(lib, (int)group_id(lib, group), 0);
    DT_ARG_INT(c, 0, lib);
}

static void setup_size_real(DtCtx *c)
{
    uint32_t group;
    int lib;

    dt_real_env(c, 1);
    lib = open_asset(c->seed, &group);
    ORIG_FIND(lib, (int)group_id(lib, group), 0);
    DT_ARG_INT(c, 0, lib);
}

static void setup_read_real(DtCtx *c)
{
    uint32_t group;
    int lib;
    uint32_t size;

    dt_real_env(c, 1);
    lib = open_asset(c->seed, &group);
    ORIG_FIND(lib, (int)group_id(lib, group), 0);
    size = ORIG_SIZE(lib);
    DT_ARG_INT(c, 0, lib);
    DT_ARG_PTR(c, 1, dt_alloc(size + 16));
}

/* ---- libraries made up in memory ---------------------------------------------------------------- */

typedef struct ZalSpec {
    int oldFormat; /* no magic: info word at +8, directory at +0xc */
    uint32_t groups;
    uint32_t perGroup;
    uint32_t stored; /* 0..3: bit 0 goes to bit 15 of the high info half, bit 1 to bit 15 of the low half */
    uint32_t method; /* bit 14 of the high info half */
    uint32_t linked; /* bit 13 of the low info half */
    uint32_t truncateTo; /* file size cap (0: keep everything) */
} ZalSpec;

enum { kLinkNames = 3 };

static const char *const kLinkTargets[kLinkNames] = {"Assets\\Scene.Txt", "Assets\\Tables.ZAL",
                                                     "Assets\\NoSuchLooseFile.bin"};

/* Expected decompressed size of the last made-up entry set up for the decompress stub. */
static uint32_t gStubOut;
static int gStubMismatch;

/* Builds the image; group ids are 10, 20, 30...; returns the image and its size. */
static uint8_t *build_zal(const ZalSpec *s, uint32_t *size)
{
    const uint32_t entry = s->stored ? 12 : 8;
    const uint32_t dirStart = s->oldFormat ? 0xc : 0x14;
    const uint32_t dirBytes = (s->perGroup * entry + 4) * s->groups;
    const uint32_t dataStart = dirStart + dirBytes;
    uint32_t pos = dataStart;
    uint8_t *img = dt_alloc(dataStart + s->groups * s->perGroup * 80 + 64);
    uint32_t info_lo = s->perGroup | (s->linked << 13) | ((s->stored & 2) ? 0x8000u : 0);
    uint32_t info_hi = s->groups | (s->method << 14) | ((s->stored & 1) ? 0x8000u : 0);

    dt_fill(img, dataStart + s->groups * s->perGroup * 80 + 64);
    if (!s->oldFormat) {
        dt_wr32(img, 0, 0x2a81511c);
    }
    dt_wr16(img, s->oldFormat ? 8 : 0x10, info_lo);
    dt_wr16(img, s->oldFormat ? 10 : 0x12, info_hi);
    for (uint32_t g = 0; g < s->groups; g++) {
        const uint32_t base = dirStart + g * (s->perGroup * entry + 4);

        dt_wr32(img, base, 10 * (g + 1));
        for (uint32_t a = 0; a < s->perGroup; a++) {
            const uint32_t e = base + 4 + a * entry;
            uint32_t len = (uint32_t)dt_range(1, 64);

            if (s->linked) {
                const char *name = kLinkTargets[dt_u32() % kLinkNames];

                len = (uint32_t)strlen(name) + 1;
                memcpy(img + pos, name, len);
            }
            dt_wr32(img, e, pos - 0x10);
            dt_wr32(img, e + 4, len | (dt_u32() & 0xff000000u));
            if (s->stored) {
                dt_wr32(img, e + 8, len); /* stored size; the unpacked size is the one above */
            }
            pos += (len + 3) & ~3u;
        }
    }
    *size = pos;
    if (s->truncateTo != 0 && s->truncateTo < pos) {
        *size = s->truncateTo;
    }
    return img;
}

static ZalSpec random_spec(void)
{
    ZalSpec s;

    s.oldFormat = (dt_u32() % 4) == 0;
    s.groups = (uint32_t)dt_range(1, 5);
    s.perGroup = (uint32_t)dt_range(1, 3);
    s.stored = 0;
    s.method = 0;
    s.linked = 0;
    s.truncateTo = 0;
    return s;
}

/* Header and directory cut short, bad magic and so on: used by the open cases. */
static ZalSpec damaged_spec(uint32_t k)
{
    ZalSpec s = random_spec();

    switch (k % 8) {
    case 0:
        s.truncateTo = (uint32_t)dt_range(1, 0x1b); /* shorter than the header */
        break;
    case 1:
        s.truncateTo = 0x1c + (uint32_t)dt_range(0, 12); /* directory cut off */
        break;
    case 2:
        s.stored = (uint32_t)dt_range(1, 3);
        s.method = (uint32_t)dt_range(0, 1);
        break;
    case 3:
        s.linked = 1; /* fine on its own... */
        if (dt_u32() & 1) {
            s.stored = 1; /* ... fatal together with compression */
        }
        break;
    case 4:
        s.groups = (uint32_t)dt_range(100, 3000); /* a big directory, cut short */
        s.truncateTo = (uint32_t)dt_range(0x20, 0x400);
        break;
    default:
        break;
    }
    return s;
}

static void BMX_CDECL stubDecompress(int kind, const void *src, uint32_t size, void *dst, uint32_t *outSize)
{
    ngLog(0x49c080);
    ngLog((uint32_t)kind);
    ngLogPtr(src);
    ngLog(size);
    ngLogPtr(dst);
    memset(dst, 0x5a, gStubOut);
    *outSize = gStubMismatch ? gStubOut + 1 : gStubOut;
}

static void setup_open_made(DtCtx *c)
{
    ZalSpec s;
    uint32_t size;
    uint8_t *img;

    dt_real_env(c, 1);
    s = damaged_spec(c->seed);
    img = build_zal(&s, &size);
    DT_ARG_PTR(c, 0, dt_ramdisk_name(img, size));
}

/* An open made-up library, with the cursor on a random entry; returns it (and the spec through *spec). */
static int open_made(ZalSpec *spec, uint32_t *size)
{
    uint8_t *img;

    *spec = random_spec();
    spec->stored = (dt_u32() % 3 == 0) ? (uint32_t)dt_range(1, 3) : 0;
    spec->method = (uint32_t)dt_range(0, 1);
    spec->linked = (spec->stored == 0 && dt_u32() % 3 == 0) ? 1 : 0;
    img = build_zal(spec, size);
    return ORIG_OPEN(dt_ramdisk_name(img, *size));
}

static void setup_find_made(DtCtx *c)
{
    ZalSpec s;
    uint32_t size;

    dt_real_env(c, 1);
    DT_ARG_INT(c, 0, open_made(&s, &size));
    DT_ARG_INT(c, 1, (dt_u32() & 1) ? (int)(10 * dt_range(1, (int)s.groups + 1)) : dt_range(0, 12));
    DT_ARG_INT(c, 2, dt_range(0, (int)s.perGroup));
}

/* Selects a random entry of a made-up library and primes the decompress stub. */
static int made_with_cursor(DtCtx *c)
{
    ZalSpec s;
    uint32_t size;
    int lib = open_made(&s, &size);
    const ZassetLibrary *l = (const ZassetLibrary *)(uintptr_t)lib;

    if (lib == 0) {
        return 0;
    }
    ORIG_FIND(lib, 10 * dt_range(1, (int)s.groups), dt_range(0, (int)s.perGroup - 1));
    gStubOut = *(const uint32_t *)(l->cursor + 4) & 0xffffff;
    gStubMismatch = dt_u32() % 5 == 0;
    dt_stub(0x0049c080, NULL, (void *)stubDecompress);
    (void)c;
    return lib;
}

static void setup_size_made(DtCtx *c)
{
    dt_real_env(c, 1);
    DT_ARG_INT(c, 0, made_with_cursor(c));
}

static void setup_read_made(DtCtx *c)
{
    int lib;

    dt_real_env(c, 1);
    lib = made_with_cursor(c);
    DT_ARG_INT(c, 0, lib);
    DT_ARG_PTR(c, 1, dt_alloc(0x8000)); /* room for the largest loose file (Scene.Txt, 20031 bytes) */
    if (lib != 0 && (dt_u32() & 1) && ((const ZassetLibrary *)(uintptr_t)lib)->linked) {
        ORIG_SIZE(lib); /* the size call caches the loose file: the second read path */
    }
}

static void setup_next_made(DtCtx *c)
{
    ZalSpec s;
    uint32_t size;
    int lib;

    dt_real_env(c, 1);
    lib = open_made(&s, &size);
    ORIG_FIND(lib, 10 * dt_range(1, (int)s.groups), 0);
    DT_ARG_INT(c, 0, lib);
}

static void setup_dump_made(DtCtx *c)
{
    ZalSpec s;
    uint32_t size;

    dt_real_env(c, 1);
    DT_ARG_INT(c, 0, open_made(&s, &size));
}

static void setup_close_made(DtCtx *c)
{
    ZalSpec s;
    uint32_t size;

    dt_real_env(c, 1);
    DT_ARG_INT(c, 0, open_made(&s, &size));
    if (c->seed & 1) {
        ORIG_OPEN("Assets\\Tables.ZAL");
    }
}

DT_CASES(zasset_cases) = {
    {"zassetOpenLibrary_real", 0x004804d0, zassetOpenLibrary, 1, DT_RET_INT | DT_NEEDS_ASSETS,
     setup_open_real, 240, 0},
    {"zassetOpenLibrary_made", 0x004804d0, zassetOpenLibrary, 1, DT_RET_INT, setup_open_made, 600, 0},
    {"zassetCloseLibrary_real", 0x004806c0, zassetCloseLibrary, 1, DT_RET_VOID | DT_NEEDS_ASSETS,
     setup_close_real, 60, 0},
    {"zassetCloseLibrary_made", 0x004806c0, zassetCloseLibrary, 1, DT_RET_VOID, setup_close_made, 100, 0},
    {"zassetFind_real", 0x00480780, zassetFind, 3, DT_RET_INT | DT_NEEDS_ASSETS, setup_find_real, 1400, 0},
    {"zassetFind_made", 0x00480780, zassetFind, 3, DT_RET_INT, setup_find_made, 600, 0},
    {"zassetDump_real", 0x00480450, zassetDump, 1, DT_RET_VOID | DT_NEEDS_ASSETS, setup_dump_real, 30, 0},
    {"zassetDump_made", 0x00480450, zassetDump, 1, DT_RET_VOID, setup_dump_made, 100, 0},
    {"zassetCount_real", 0x00480720, zassetCount, 1, DT_RET_INT | DT_NEEDS_ASSETS, setup_count_real, 30, 0},
    {"zassetNext_real", 0x00480730, zassetNext, 1, DT_RET_INT | DT_NEEDS_ASSETS, setup_next_real, 300, 0},
    {"zassetNext_made", 0x00480730, zassetNext, 1, DT_RET_INT, setup_next_made, 300, 0},
    {"zassetSize_real", 0x00480850, zassetSize, 1, DT_RET_INT | DT_NEEDS_ASSETS, setup_size_real, 1400, 0},
    {"zassetSize_made", 0x00480850, zassetSize, 1, DT_RET_INT, setup_size_made, 400, 0},
    {"zassetRead_real", 0x00480930, zassetRead, 2, DT_RET_INT | DT_NEEDS_ASSETS, setup_read_real, 1400, 0},
    {"zassetRead_made", 0x00480930, zassetRead, 2, DT_RET_INT, setup_read_made, 800, 0},
};
