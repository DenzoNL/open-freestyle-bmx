/*
 * Tests for the action loop (action.c). Every callee is stubbed (controllers, overlay, replays, camera,
 * sound, riders, loaders): the stubs log their arguments by value and draw their results from the
 * deterministic generator, so the order and the arguments of all calls are compared. The state of the
 * screens, the controller records and the player records are random but inside the ranges the game uses.
 */
#include <stddef.h>

#include "dt.h"
#include "dt_zmem.h"

#include "bmx/com/action/action.h"
#include "bmx/com/action/overlay.h"
#include "bmx/com/zge/zactor.h"
#include "compat/crt.h"
#include "zge/zdebug.h"

#define LOG_MAX 2048u

static uint32_t gLog[LOG_MAX];
static uint32_t gLogN;
static uint32_t gPressP;  /* contrlerPressed fires with probability gPressP / 8 */
static uint32_t gFailP;   /* one in gFailP loaders fail (0: never) */
static uint32_t gEntries; /* motions the bank enumerates */
static uint32_t gEntryAt;
static uint32_t *gPadFlags[2];
static uint8_t *gPads[2];
static uint8_t *gReplayData;
static uint32_t gReplaySize;
static uint32_t gAssetBig;

static void logU(uint32_t v)
{
    if (gLogN < LOG_MAX) {
        gLog[gLogN] = v;
    }
    gLogN++;
}

static void logS(const char *s)
{
    uint32_t h = 5381;

    while (*s) {
        h = h * 33 + (uint8_t)*s++;
    }
    logU(h);
}

static uint32_t rnd(uint32_t lo, uint32_t hi)
{
    return lo + dt_u32() % (hi - lo + 1);
}

static int fails(void)
{
    return gFailP != 0 && dt_u32() % gFailP == 0;
}

/* The byte parameters of the replay functions: the original passes stack garbage above them. */
static uint32_t b(uint32_t v)
{
    return v & 0xff;
}

/* ---- stubs -------------------------------------------------------------------------------------- */

static void BMX_CDECL s_print(const char *fmt, ...)
{
    const uint32_t *arg = (const uint32_t *)((const uint8_t *)&fmt + sizeof fmt);
    const char *p;

    logU(0xd10);
    logS(fmt);
    for (p = fmt; *p; p++) {
        if (*p == '%') {
            p++;
            if (*p == 'l') {
                p++;
            }
            if (*p == 's') {
                logS((const char *)(uintptr_t)*arg++); // NOLINT(clang-analyzer-security.ArrayBound)
            } else if (*p == 'd') {
                logU(*arg++); // NOLINT(clang-analyzer-security.ArrayBound)
            }
        }
    }
}

#define VOID0(name, tag)                                                                                     \
    static void BMX_CDECL name(void)                                                                         \
    {                                                                                                        \
        logU(tag);                                                                                           \
    }
#define VOID1(name, tag)                                                                                     \
    static void BMX_CDECL name(uint32_t a)                                                                   \
    {                                                                                                        \
        logU(tag);                                                                                           \
        logU(a);                                                                                             \
    }
#define VOID2(name, tag)                                                                                     \
    static void BMX_CDECL name(uint32_t a, uint32_t c)                                                       \
    {                                                                                                        \
        logU(tag);                                                                                           \
        logU(a);                                                                                             \
        logU(c);                                                                                             \
    }

VOID0(s_ovlInit, 0x101)
VOID0(s_ovlShutdown, 0x102)
VOID0(s_ovlLoadFonts, 0x103)
VOID1(s_ovlLoadStrings, 0x104)
VOID1(s_ovlSetScreen, 0x105)
VOID2(s_ovlVisible, 0x106)
VOID0(s_comdataInit, 0x108)
VOID0(s_contrlerCreate, 0x109)
VOID0(s_contrlerFree, 0x10a)
VOID1(s_contrlerSetMode, 0x10b)
VOID0(s_fxShutdown, 0x10d)
VOID0(s_gameSetup, 0x10e)
VOID0(s_gameFreeReplays, 0x10f)
VOID0(s_gameUpdateRun, 0x110)
VOID0(s_physInit, 0x113)
VOID0(s_physShutdown, 0x114)
VOID0(s_physUpdateRiders, 0x115)
VOID0(s_propInit, 0x116)
VOID0(s_propStopSounds, 0x117)
VOID0(s_propUpdate, 0x118)
VOID0(s_replayInit, 0x119)
VOID0(s_replayDestroyAll, 0x11a)
VOID0(s_replayUpdate, 0x11b)
VOID0(s_replayUpdateRiders, 0x11c)
VOID0(s_riderUpdateAll, 0x11d)
VOID0(s_zsystemMenu, 0x11e)
VOID0(s_zsystemEnd, 0x11f)
VOID0(s_stepPlayers, 0x120)
VOID0(s_checkPlayers, 0x121)
VOID0(s_bmxSoundStopAll, 0x122)
VOID0(s_zmotionClose, 0x123)
VOID0(s_zrollerSetLocal, 0x124)
VOID0(s_zrollerLoadCode, 0x125)
VOID0(s_zrollerInitLoading, 0x126)
VOID0(s_zrollerShutLoading, 0x127)
VOID0(s_zrollerSetMap, 0x128)
VOID1(s_cameraSelect, 0x129)
VOID1(s_gameSetSubMode, 0x12a)
VOID1(s_gameStartMode, 0x12b)
VOID2(s_comdataResetView, 0x12c)
VOID2(s_gameRestart, 0x12e)

static void BMX_CDECL s_comdataSetView(uint32_t player, uint32_t view)
{
    logU(0x131);
    logU(player);
    logU(view);
}

static void BMX_CDECL s_replaySetMode(uint32_t h, uint32_t mode)
{
    logU(0x132);
    logU(b(h));
    logU(mode);
}

static void BMX_CDECL s_actionLog(const char *msg)
{
    logU(0x133);
    logS(msg);
}

static void BMX_CDECL s_zpadSelect(int index)
{
    logU(0x134);
    logU((uint32_t)index);
}

static void BMX_CDECL s_ovlSetElementA(int index, const void *value)
{
    const uint8_t *p = value;

    logU(0x135);
    logU((uint32_t)index);
    if (index == 0x10) {
        logU(p[0]);
        logU(p[1]);
        logU(p[2]);
        logU(p[3]);
        logU((uint32_t)p[4] | (uint32_t)p[5] << 8);
    } else {
        logU(p[0]);
    }
}

static uint32_t BMX_CDECL s_actionSetMode(uint32_t mode)
{
    logU(0x136);
    logU(mode);
    return mode;
}

static uint32_t BMX_CDECL s_ovlUpdate(void)
{
    logU(0x140);
    return dt_u32() % 4 == 0 ? rnd(1, 3) : 0;
}

static uint32_t BMX_CDECL s_ovlGetScreenId(void)
{
    uint32_t v;

    logU(0x141);
    switch (dt_u32() % 6) {
    case 0:
        v = rnd(0, 0x1c);
        break;
    case 1:
        v = 0x19;
        break;
    default:
        v = rnd(8, 12);
        break;
    }
    return v;
}

static uint8_t *BMX_CDECL s_contrlerGet(uint32_t index)
{
    logU(0x142);
    logU(index);
    return gPads[index & 1];
}

static uint32_t BMX_CDECL s_contrlerPoll(void)
{
    logU(0x143);
    return dt_u32();
}

static int BMX_CDECL s_contrlerPressed(int player, uint32_t mask, uint32_t repeat)
{
    int pressed = dt_u32() % 8 < gPressP;

    logU(0x144);
    logU((uint32_t)player);
    logU(mask);
    logU(repeat);
    return pressed;
}

static uint32_t BMX_CDECL s_comdataGetView(uint32_t player)
{
    logU(0x145);
    logU(player);
    return rnd(0, 11);
}

static uint32_t BMX_CDECL s_fxInit(void)
{
    logU(0x146);
    return 0;
}

static uint32_t BMX_CDECL s_gameGetMode(void)
{
    logU(0x147);
    return rnd(0, 3);
}

static uint32_t BMX_CDECL s_gameGetModeByte(void)
{
    logU(0x148);
    return rnd(0, 4) | (dt_u32() & 0xffffff00u);
}

static uint32_t BMX_CDECL s_replayGetState(uint32_t h)
{
    logU(0x149);
    logU(b(h));
    return dt_u32() % 5 == 0 ? rnd(0, 5) : rnd(1, 2) * 2 % 6;
}

static uint32_t BMX_CDECL s_replayGetRider(uint32_t h)
{
    logU(0x14a);
    logU(b(h));
    return rnd(0, 1);
}

static uint8_t BMX_CDECL s_replayCreate(const void *desc)
{
    const uint8_t *p = desc;
    uint32_t size;

    memcpy(&size, p + 4, 4);
    logU(0x14b);
    logU(p[0]);
    logU(p[1]);
    logU(size);
    return dt_u32() % 8 == 0 ? 0xff : (uint8_t)rnd(0, 4);
}

static void BMX_CDECL s_replayGetBuffer(uint32_t source, uint8_t **data, int *size)
{
    logU(0x14c);
    logU(b(source));
    *data = gReplayData;
    *size = (int)gReplaySize;
}

static void BMX_CDECL s_replayDestroy(uint32_t h)
{
    logU(0x14d);
    logU(b(h));
}

static void *BMX_CDECL s_riderCreate(uint32_t index, uint32_t rider, uint32_t look, uint32_t bike)
{
    logU(0x14e);
    logU(index);
    logU(rider);
    logU(look);
    logU(bike);
    return fails() ? NULL : (void *)0x1234;
}

static uint32_t BMX_CDECL s_zactorResetCache(void)
{
    logU(0x14f);
    return 0;
}

static int BMX_CDECL s_bmxSoundPlayAt(int id, const void *position, int a, int volume, int c)
{
    logU(0x150);
    logU((uint32_t)id);
    logU((uint32_t)(uintptr_t)position);
    logU((uint32_t)a);
    logU((uint32_t)volume);
    logU((uint32_t)c);
    return (int)rnd(0, 255);
}

static int BMX_CDECL s_zrollerInit(void)
{
    logU(0x151);
    return fails();
}

static int BMX_CDECL s_zrollerStartup(void)
{
    logU(0x152);
    return fails();
}

static int BMX_CDECL s_zrollerLoadAssets(void)
{
    logU(0x153);
    return fails();
}

static int BMX_CDECL s_zrollerRunDraw(void)
{
    logU(0x154);
    return 0;
}

static int BMX_CDECL s_zrollerShutdown(void)
{
    logU(0x155);
    return (int)rnd(0, 3);
}

static int BMX_CDECL s_zmotionOpen(const char *name)
{
    logU(0x156);
    logS(name);
    return fails();
}

static ZMotionBank *BMX_CDECL s_zmotionLoadBank(uint32_t id)
{
    logU(0x157);
    logU(id);
    gEntryAt = 0;
    return fails() ? NULL : (ZMotionBank *)(uintptr_t)0x7000;
}

static ZMotionEntry *BMX_CDECL s_zmotionBankNext(ZMotionBank *bank)
{
    static ZMotionEntry entry;

    logU(0x158);
    logU((uint32_t)(uintptr_t)bank);
    if (gEntryAt >= gEntries) {
        return NULL;
    }
    entry.id = gEntryAt;
    entry.motion = (const ZMotion *)(uintptr_t)(0x4000u + gEntryAt * 0x10u);
    gEntryAt++;
    return &entry;
}

static void BMX_CDECL s_zactorAngle(uint16_t base, int flip)
{
    logU(0x159);
    logU(base);
    logU((uint32_t)flip);
}

static int BMX_CDECL s_zassetFind(int lib, int id, int flags)
{
    logU(0x15a);
    logU((uint32_t)lib);
    logU((uint32_t)id);
    logU((uint32_t)flags);
    return fails() ? 0 : 1;
}

static uint32_t BMX_CDECL s_zassetSize(int lib)
{
    logU(0x15b);
    logU((uint32_t)lib);
    return gAssetBig ? gReplaySize + 1 : gReplaySize / 2;
}

static int BMX_CDECL s_zassetRead(int lib, void *dst)
{
    logU(0x15c);
    logU((uint32_t)lib);
    logU(dst == gReplayData);
    return 1;
}

static void stubs_install(void)
{
    dt_stub(0x00414970, NULL, (void *)s_ovlLoadFonts);
    dt_stub(0x004147e0, NULL, (void *)s_ovlInit);
    dt_stub(0x004148b0, NULL, (void *)s_ovlShutdown);
    dt_stub(0x00414dd0, NULL, (void *)s_ovlLoadStrings);
    dt_stub(0x00415400, NULL, (void *)s_ovlSetScreen);
    dt_stub(0x00415300, NULL, (void *)s_ovlVisible);
    dt_stub(0x00415370, NULL, (void *)s_ovlSetElementA);
    dt_stub(0x00415140, NULL, (void *)s_ovlUpdate);
    dt_stub(0x004153d0, NULL, (void *)s_ovlGetScreenId);
    dt_stub(0x00407ba0, NULL, (void *)s_comdataInit);
    dt_stub(0x00408030, NULL, (void *)s_contrlerCreate);
    dt_stub(0x004080c0, NULL, (void *)s_contrlerFree);
    dt_stub(0x004083e0, NULL, (void *)s_contrlerSetMode);
    dt_stub(0x004086f0, NULL, (void *)s_contrlerGet);
    dt_stub(0x004080f0, NULL, (void *)s_contrlerPoll);
    dt_stub(0x00408590, NULL, (void *)s_contrlerPressed);
    dt_stub(0x004089c0, NULL, (void *)s_fxInit);
    dt_stub(0x00408a10, NULL, (void *)s_fxShutdown);
    dt_stub(0x004099c0, NULL, (void *)s_gameSetup);
    dt_stub(0x00409cd0, NULL, (void *)s_gameFreeReplays);
    dt_stub(0x00409d20, NULL, (void *)s_gameUpdateRun);
    dt_stub(0x0040a9f0, NULL, (void *)s_gameGetMode);
    dt_stub(0x0040c070, NULL, (void *)s_gameGetModeByte);
    dt_stub(0x0040aa10, NULL, (void *)s_gameSetSubMode);
    dt_stub(0x0040a8c0, NULL, (void *)s_gameStartMode);
    dt_stub(0x0040c190, NULL, (void *)s_gameRestart);
    dt_stub(0x0041fa20, NULL, (void *)s_physInit);
    dt_stub(0x0041fa40, NULL, (void *)s_physShutdown);
    dt_stub(0x0041fc40, NULL, (void *)s_physUpdateRiders);
    dt_stub(0x0042deb0, NULL, (void *)s_propInit);
    dt_stub(0x0042e270, NULL, (void *)s_propStopSounds);
    dt_stub(0x0042e2f0, NULL, (void *)s_propUpdate);
    dt_stub(0x0042fce0, NULL, (void *)s_replayInit);
    dt_stub(0x0042fd10, NULL, (void *)s_replayDestroyAll);
    dt_stub(0x0042fd50, NULL, (void *)s_replayUpdate);
    dt_stub(0x00430e70, NULL, (void *)s_replayUpdateRiders);
    dt_stub(0x00431000, NULL, (void *)s_riderUpdateAll);
    dt_stub(0x00430180, NULL, (void *)s_replayCreate);
    dt_stub(0x00430d70, NULL, (void *)s_replayGetBuffer);
    dt_stub(0x00430380, NULL, (void *)s_replayDestroy);
    dt_stub(0x00430410, NULL, (void *)s_replaySetMode);
    dt_stub(0x00430b70, NULL, (void *)s_replayGetState);
    dt_stub(0x00430c70, NULL, (void *)s_replayGetRider);
    dt_stub(0x004316c0, NULL, (void *)s_riderCreate);
    dt_stub(0x004578f0, NULL, (void *)s_zactorResetCache);
    dt_stub(0x0044e2e0, NULL, (void *)s_bmxSoundStopAll);
    dt_stub(0x0044e300, NULL, (void *)s_bmxSoundPlayAt);
    dt_stub(0x0045a1a0, NULL, (void *)s_zsystemEnd);
    dt_stub(0x0045a1c0, NULL, (void *)s_actionLog);
    dt_stub(0x0045a290, NULL, (void *)s_zsystemMenu);
    dt_stub(0x00405780, NULL, (void *)s_stepPlayers);
    dt_stub(0x004056c0, NULL, (void *)s_checkPlayers);
    dt_stub(0x00407b70, NULL, (void *)s_comdataGetView);
    dt_stub(0x004078c0, NULL, (void *)s_comdataSetView);
    dt_stub(0x00407760, NULL, (void *)s_comdataResetView);
    dt_stub(0x00407530, NULL, (void *)s_cameraSelect);
    dt_stub(0x00480400, NULL, (void *)s_zpadSelect);
    dt_stub(0x0046c220, NULL, (void *)s_zrollerInit);
    dt_stub(0x0046c310, NULL, (void *)s_zrollerStartup);
    dt_stub(0x0046c360, NULL, (void *)s_zrollerShutdown);
    dt_stub(0x0046c420, NULL, (void *)s_zrollerRunDraw);
    dt_stub(0x0046c4b0, NULL, (void *)s_zrollerLoadAssets);
    dt_stub(0x0046cbf0, NULL, (void *)s_zrollerSetLocal);
    dt_stub(0x0046cc50, NULL, (void *)s_zrollerLoadCode);
    dt_stub(0x0046cb60, NULL, (void *)s_zrollerInitLoading);
    dt_stub(0x0046cb90, NULL, (void *)s_zrollerShutLoading);
    dt_stub(0x0046cd60, NULL, (void *)s_zrollerSetMap);
    dt_stub(0x00474f10, NULL, (void *)s_zmotionOpen);
    dt_stub(0x00475000, NULL, (void *)s_zmotionClose);
    dt_stub(0x00475050, NULL, (void *)s_zmotionLoadBank);
    dt_stub(0x004753c0, NULL, (void *)s_zmotionBankNext);
    dt_stub(0x004759d0, NULL, (void *)s_zactorAngle);
    dt_stub(0x00480780, NULL, (void *)s_zassetFind);
    dt_stub(0x00480850, NULL, (void *)s_zassetSize);
    dt_stub(0x00480930, NULL, (void *)s_zassetRead);
    gZdebugPrint = s_print;
}

/* ---- fixtures -------------------------------------------------------------------------------------- */

/* A counter value near one of the thresholds the steps compare against. */
static uint32_t edgy_timer(void)
{
    static const uint32_t near[] = {0, 1, 2, 0x1d, 0x1e, 0xee, 0xef, 0xf0, 0xd79, 0xd7a, 0xf3b, 0xf3c};

    return dt_u32() % 3 == 0 ? rnd(0, 0x1000) : near[dt_u32() % (sizeof near / sizeof near[0])];
}

static void begin(DtCtx *c)
{
    dt_seed(c);
    dt_zmem_env();
    gLogN = 0;
    gPressP = c->seed % 3 == 0 ? 6 : 2;
    gFailP = c->seed % 2 ? 0 : 10;
    gEntries = dt_u32() % 20 == 0 ? rnd(0xed, 0xf2) : rnd(0, 6);
    gEntryAt = 0;
    gAssetBig = dt_u32() % 6 == 0;
    gReplaySize = (uint32_t)rnd(16, 0x3000);
    gReplayData = dt_alloc(32);
    for (int i = 0; i < 2; i++) {
        gPadFlags[i] = dt_alloc(16);
        dt_fill(gPadFlags[i], 16);
        gPads[i] = dt_alloc(0x20);
        dt_fill(gPads[i], 0x20);
        *(uint32_t **)(void *)gPads[i] = gPadFlags[i];
        *(uint16_t *)(void *)(gPads[i] + 0xe) = (uint16_t)(dt_u32() % 2 ? 0 : rnd(0, 4));
        ((uint32_t *)(void *)gPads[i])[5] = dt_u32() % 2 ? (dt_u32() & 0x300u) : dt_u32();
        if (dt_u32() % 2) {
            ((uint32_t *)(void *)gPads[i])[4] = 0;
        }
    }
    stubs_install();
    gActionMode = (uint8_t)(dt_u32() % 12 == 0 ? rnd(5, 9) : rnd(0, 4));
    gAction.step = (uint8_t)(dt_u32() % 12 == 0 ? rnd(8, 12) : rnd(0, 7));
    gAction.replay = (uint8_t)rnd(0, 4);
    gAction.player = (uint8_t)dt_u32();
    gAction.field04 = dt_u32();
    gAction.field08 = (uint8_t)dt_u32();
    gAction.savedView = (uint8_t)rnd(0, 11);
    gAction.timer = edgy_timer();
    if (gActionMode == 4) {
        gAction.step = (uint8_t)(dt_u32() % 10 == 0 ? rnd(4, 6) : rnd(0, 3));
        if (gAction.step == 1 && dt_u32() % 2) {
            static const uint32_t near[] = {0x1d, 0xd79, 0xf3b};

            gAction.timer = near[dt_u32() % 3];
        }
    }
    gAction.field10 = dt_u32();
    gAction.hudShown = dt_u32() % 2;
    gAction.random = (uint8_t)dt_u32();
    gAction.holdCount = (uint16_t)(dt_u32() % 3 == 0 ? rnd(0x3a, 0x3d) : rnd(0, 5));
    gActionFrameReload = dt_u32() % 5 == 0 ? -1 : (int32_t)rnd(1, 4);
    gActionFrameCount = dt_u32() % 4 ? 1 : (int32_t)rnd(0, 3);
    gActionQuit = dt_u32();
    gActionSubMode = (uint8_t)dt_u32();
    gActionLanguage = (uint8_t)rnd(0, 5);
    gActionOptions = (uint8_t)dt_u32();
    gActionReplayLib = (int32_t)dt_u32();
    gPlayerCount = (uint8_t)(dt_u32() % 8 == 0 ? rnd(2, 3) : rnd(0, 2));
    gDemoPlayer = (uint8_t)(dt_u32() % 2 ? 0xff : rnd(0, 2));
    gCurrentPlayer = (uint8_t)rnd(0, 1);
    gCurrentLevel = (uint8_t)rnd(0, 11);
    for (int i = 0; i < 2; i++) {
        gPlayerRecs[i][0x4d] = (uint8_t)rnd(0, 3);
        gProfiles[i].character = (uint8_t)rnd(0, 11);
        gProfiles[i].pad04[0] = (uint8_t)dt_u32();
        gProfiles[i].pad04[1] = (uint8_t)dt_u32();
    }
}

static void setup_update(DtCtx *c)
{
    begin(c);
    dt_stub(0x00404f40, NULL, (void *)s_actionSetMode);
}

static void setup_mode(DtCtx *c)
{
    begin(c);
    DT_ARG_INT(c, 0, dt_u32() % 10 == 0 ? rnd(5, 6) : rnd(0, 4));
}

static void setup_init(DtCtx *c)
{
    begin(c);
    dt_stub(0x00404f40, NULL, (void *)s_actionSetMode);
    gPlayerCount = (uint8_t)(c->seed % 9 == 4 ? 3 : rnd(0, 2));
}

static void setup_plain(DtCtx *c)
{
    begin(c);
}

DT_CASES(action_cases) = {
    {"actionShutdownRoller", 0x0040249b, actionShutdownRoller, 0, DT_RET_INT, setup_plain, 20, 0},
    {"actionOpenMotions", 0x00403c30, actionOpenMotions, 0, DT_RET_INT, setup_plain, 100, 0},
    {"actionCloseMotions", 0x00403cc0, actionCloseMotions, 0, 0, setup_plain, 10, 0},
    {"actionPickDemo", 0x00403ce0, actionPickDemo, 0, DT_RET_INT, setup_plain, 300, 0},
    {"actionInitialize", 0x00403d10, actionInitialize, 0, DT_RET_INT, setup_init, 1000, 0},
    {"actionStopSounds", 0x004040c0, actionStopSounds, 0, DT_RET_INT, setup_plain, 10, 0},
    {"actionUpdate", 0x004040e0, actionUpdate, 0, DT_RET_INT, setup_update, 3000, 0},
    {"actionShutdown", 0x00404ed0, actionShutdown, 0, 0, setup_plain, 20, 0},
    {"actionSetMode", 0x00404f40, actionSetMode, 1, DT_RET_INT, setup_mode, 2000, 0},
};
