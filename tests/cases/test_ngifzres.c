/*
 * Tests for the resource tables of NgIFzres.c. The real libraries (Tables, MiscData, Replay, Material) are
 * opened by the original code and the level / player tables, rider records and overlay materials are
 * loaded from them; renderer calls (texture load and queries) are replaced by logging stand-ins.
 */
#include "bmx/com/zge/ngif/ngifzres.h"
#include "dt.h"
#include "dt_ngif.h"
#include "dt_realfile.h"
#include "zge/zasset.h"
#include "zge/ziff.h"

typedef struct ZassetLibrary *(BMX_CDECL *OpenFn)(const char *);
typedef int(BMX_CDECL *VoidIntFn)(void);
typedef void(BMX_CDECL *VoidFn)(void);
typedef struct ZMemFreeList *(BMX_CDECL *ListCreateFn)(uint32_t, uint32_t, void *);
typedef const uint8_t *(BMX_CDECL *InfoFn)(uint32_t);

#define O_OPEN        DT_ORIG(0x004804d0, OpenFn)
#define O_INIT_LEVELS DT_ORIG(0x0046b7c0, VoidIntFn)
#define O_INIT_PLAYER DT_ORIG(0x0046ba10, VoidIntFn)
#define O_RESET       DT_ORIG(0x0046bef0, VoidIntFn)

static void open_libs(void)
{
    gZresMiscLib = O_OPEN("Assets\\MiscData.ZAL");
    gZresTablesLib = O_OPEN("Assets\\Tables.ZAL");
    gZresReplayLib = O_OPEN("Assets\\Replay.ZAL");
    gZresMaterialLib = O_OPEN("Assets\\Material.ZAL");
}

/* The environment with the four libraries open and (optionally) the tables loaded. */
static void begin(DtCtx *c, int tables)
{
    dt_real_env(c, 1);
    DT_ORIG(0x0049a6d0, VoidFn)(); /* ziffInitialize: the table readers are IFF files */
    open_libs();
    if (tables) {
        O_INIT_LEVELS();
        O_INIT_PLAYER();
    }
}

/* ---- renderer stand-ins ------------------------------------------------------------------------------ */

static uint32_t gTextureAnswers;

static void *BMX_CDECL stubLoadTexture(const void *data, uint32_t size, const char *name, int a, int b)
{
    ngLog(0x4896d0);
    ngLogPtr(data);
    ngLog(size);
    ngLog(ngHash(name));
    ngLog((uint32_t)a);
    ngLog((uint32_t)b);
    return (gTextureAnswers & 1) ? NULL : (void *)(uintptr_t)(0x70000000u + (dt_u32() & 0xfff0));
}

static void *BMX_CDECL stubGetTexture(void *material, int index)
{
    ngLog(0x489a40);
    ngLogPtr(material);
    ngLog((uint32_t)index);
    return (void *)((uintptr_t)material + 0x100);
}

static uint32_t BMX_CDECL stubTextureWidth(void *texture)
{
    ngLog(0x498870);
    ngLogPtr(texture);
    return (gTextureAnswers & 2) ? 0x100 + dt_u32() % 0x300 : dt_u32() % 0x100;
}

static uint32_t BMX_CDECL stubTextureHeight(void *texture)
{
    ngLog(0x498880);
    ngLogPtr(texture);
    return (gTextureAnswers & 4) ? 0x100 + dt_u32() % 0x300 : dt_u32() % 0x100;
}

static void install_renderer(void)
{
    gTextureAnswers = (dt_u32() % 5 == 0) ? 1 : (dt_u32() & 6);
    gZresLoadTexture = stubLoadTexture;
    gZresGetTexture = stubGetTexture;
    gZresTextureWidth = stubTextureWidth;
    gZresTextureHeight = stubTextureHeight;
}

/* ---- setups --------------------------------------------------------------------------------------- */

static struct ZassetLibrary *random_lib(void)
{
    struct ZassetLibrary *const libs[4] = {gZresMiscLib, gZresTablesLib, gZresReplayLib, gZresMaterialLib};

    return libs[dt_u32() % 4];
}

static void setup_load_asset(DtCtx *c)
{
    static const uint32_t kMiscGroups[] = {1, 30001, 30002, 30004, 30005, 0, 7, 99999};
    static const uint32_t kPlain[] = {0, 1, 2, 3, 5, 9, 11, 23, 24, 100, 1261, 1262, 5000};
    struct ZassetLibrary *lib;

    begin(c, 0);
    lib = random_lib();
    DT_ARG_PTR(c, 0, lib);
    DT_ARG_INT(c, 1, lib == gZresMiscLib ? kMiscGroups[dt_u32() % 8] : kPlain[dt_u32() % 13]);
    DT_ARG_INT(c, 2,
               dt_u32() & 0xffff0000u); /* asset 0 (the only one; the loader masks the index to 16 bits) */
    DT_ARG_PTR(c, 3, (dt_u32() & 1) ? (void *)dt_alloc(8) : NULL);
    DT_ARG_INT(c, 4, (int)(dt_u32() & 3));
}

static void setup_init_levels(DtCtx *c)
{
    begin(c, 0);
    if (c->seed % 4 == 0) {
        O_INIT_LEVELS(); /* already loaded: nothing to do */
    }
}

static void setup_init_players(DtCtx *c)
{
    begin(c, 0);
    if (c->seed % 4 == 0) {
        O_INIT_PLAYER();
    }
}

static void setup_fields(DtCtx *c)
{
    dt_real_env(c, 0);
    dt_fill(&gZres, sizeof gZres);
}

static void setup_level_info(DtCtx *c)
{
    begin(c, 1);
    DT_ARG_INT(c, 0, (dt_u32() % 5 == 0) ? dt_u32() % 100 : dt_u32() % (gZres.levelCount + 1));
}

static void setup_rider_record(DtCtx *c)
{
    begin(c, 1);
    DT_ARG_INT(c, 0, (dt_u32() % 5 == 0) ? dt_u32() : dt_u32() % (gZres.playerCount + 2));
}

static void setup_bike_entry(DtCtx *c)
{
    dt_real_env(c, 0);
    DT_ARG_INT(c, 0, (dt_u32() % 6 == 0) ? (uint32_t)dt_range(0x2c, 0x40) : (uint32_t)dt_range(0, 0x2b));
}

static void setup_init_data(DtCtx *c)
{
    begin(c, 0);
}

static void setup_close_libs(DtCtx *c)
{
    dt_real_env(c, 1);
    open_libs();
    if (dt_u32() & 1) {
        gZresMaterialLib = 0;
    }
    if (dt_u32() & 1) {
        gZresMiscLib = 0;
    }
    if (dt_u32() & 1) {
        gZresTablesLib = 0;
    }
    if (dt_u32() & 1) {
        gZresReplayLib = 0;
    }
}

static void setup_open_libs(DtCtx *c)
{
    dt_real_env(c, 1);
}

static void setup_init(DtCtx *c)
{
    dt_real_env(c, 1);
    DT_ORIG(0x0049a6d0, VoidFn)(); /* ziffInitialize */
}

static int gOpenAnswers;

static int BMX_CDECL stubOpenLibrary(const char *name)
{
    const int ok = gOpenAnswers & 1;

    ngLog(0x4804d0);
    ngLog(ngHash(name));
    gOpenAnswers >>= 1;
    return ok ? (int)(0x7000 + (dt_u32() & 0xfff0)) : 0;
}

static void setup_open_libs_stub(DtCtx *c)
{
    dt_real_env(c, 1);
    gOpenAnswers = (c->seed % 3 == 0) ? 0xf : (int)dt_u32();
    dt_stub(0x004804d0, NULL, (void *)stubOpenLibrary);
}

static void setup_unload(DtCtx *c)
{
    begin(c, dt_u32() % 4 != 0);
    gZresListA = (dt_u32() & 1) ? DT_ORIG(0x00485e80, ListCreateFn)(8, 4, NULL) : NULL;
    gZresListB = (dt_u32() & 1) ? DT_ORIG(0x00485e80, ListCreateFn)(8, 4, NULL) : NULL;
    if (dt_u32() % 4 == 0) {
        gZresMaterialLib = 0;
    }
}

static void setup_reset(DtCtx *c)
{
    dt_real_env(c, 0);
    gZresMaterialLib = (struct ZassetLibrary *)(uintptr_t)dt_u32();
    gZresMiscLib = (struct ZassetLibrary *)(uintptr_t)dt_u32();
    dt_fill(&gZres, sizeof gZres);
}

static void setup_register(DtCtx *c)
{
    ZresOverlaySource *src = gZresOverlaySources;
    uint32_t id;

    begin(c, 0);
    O_RESET();
    install_renderer();
    gZres.overlayInit = (c->seed % 4 == 0) ? 1 : 0;
    gZres.overlayCounter = dt_range(0, 20);
    id = (dt_u32() % 8 == 0) ? dt_u32() : src[dt_u32() % kZresOverlaySources].id;
    if (gZres.overlayInit) {
        /* some slots already loaded */
        for (int i = 0; i < kZresOverlaySlots; i++) {
            gZresOverlaySlots[i].texture = (dt_u32() % 3 == 0) ? dt_u32() : 0;
        }
    }
    if (dt_u32() % 7 == 0) {
        gZres.libs[0] = gZresMiscLib; /* sources point at the wrong library: the asset is not there */
    }
    DT_ARG_INT(c, 0, (int)id);
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_INT(c, 2,
               (dt_u32() & 1) ? (uint32_t)dt_range(0, 20)
                              : (dt_u32() & 0xffff0000u) | (uint32_t)dt_range(0, 511));
}

static void setup_nop(DtCtx *c)
{
    dt_real_env(c, 0);
}

DT_CASES(ngifzres_cases) = {
    {"ngifzresLoadAsset", 0x0046b8b0, ngifzresLoadAsset, 5, DT_RET_INT | DT_NEEDS_ASSETS, setup_load_asset,
     800, 0},
    {"ngifzresInitLevels", 0x0046b7c0, ngifzresInitLevels, 0, DT_RET_INT | DT_NEEDS_ASSETS, setup_init_levels,
     40, 0},
    {"ngifzresGetSpare", 0x0046b950, ngifzresGetSpare, 0, DT_RET_INT, setup_fields, 20, 0},
    {"ngifzresLevelCount", 0x0046b970, ngifzresLevelCount, 0, DT_RET_INT, setup_fields, 20, 0},
    {"ngifzresLoadLevelInfo", 0x0046b990, ngifzresLoadLevelInfo, 1, DT_RET_INT | DT_NEEDS_ASSETS,
     setup_level_info, 400, 0},
    {"ngifzresInitPlayers", 0x0046ba10, ngifzresInitPlayers, 0, DT_RET_INT | DT_NEEDS_ASSETS,
     setup_init_players, 40, 0},
    {"ngifzresGetRiderRecord", 0x0046bb00, ngifzresGetRiderRecord, 1, DT_RET_INT | DT_NEEDS_ASSETS,
     setup_rider_record, 400, 0},
    {"ngifzresPlayerCount", 0x0046bba0, ngifzresPlayerCount, 0, DT_RET_INT, setup_fields, 20, 0},
    {"ngifzresGetPlayerInfo", 0x0046bbc0, ngifzresGetPlayerInfo, 0, DT_RET_INT, setup_fields, 10, 0},
    {"ngifzresGetBikeEntry", 0x0046bbe0, ngifzresGetBikeEntry, 1, DT_RET_INT, setup_bike_entry, 100, 0},
    {"ngifzresInitData", 0x0046bc20, ngifzresInitData, 0, DT_RET_INT | DT_NEEDS_ASSETS, setup_init_data, 40,
     0},
    {"ngifzresCloseLibraries", 0x0046bc40, ngifzresCloseLibraries, 0, DT_RET_VOID | DT_NEEDS_ASSETS,
     setup_close_libs, 60, 0},
    {"ngifzresOpenLibraries", 0x0046bcd0, ngifzresOpenLibraries, 0, DT_RET_VOID | DT_NEEDS_ASSETS,
     setup_open_libs, 20, 0},
    {"ngifzresOpenLibraries_stub", 0x0046bcd0, ngifzresOpenLibraries, 0, DT_RET_VOID, setup_open_libs_stub,
     40, 0},
    {"ngifzresInit", 0x0046be10, ngifzresInit, 0, DT_RET_INT | DT_NEEDS_ASSETS, setup_init, 20, 0},
    {"ngifzresUnload", 0x0046be30, ngifzresUnload, 0, DT_RET_VOID | DT_NEEDS_ASSETS, setup_unload, 60, 0},
    {"ngifzresShutdown", 0x0046bed0, ngifzresShutdown, 0, DT_RET_VOID, setup_nop, 4, 0},
    {"ngifzresReset", 0x0046bef0, ngifzresReset, 0, DT_RET_INT, setup_reset, 20, 0},
    {"ngifzresRegister", 0x0046bf30, ngifzresRegister, 3, DT_RET_VOID | DT_NEEDS_ASSETS, setup_register, 600,
     0},
};
