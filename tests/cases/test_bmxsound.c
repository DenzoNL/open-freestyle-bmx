/*
 * Tests for the game sound front end (bmxsound.c). Everything it calls is a stub that logs its (masked)
 * arguments and answers from the test's random stream: the mixer, sequencer, sources, loader and stream
 * player, the GUI pad functions, and the winmm timer imports (through the exe's import table slots). The
 * failure paths of bmxSoundOpen (each initialisation step failing in turn) end the run in zdebugFail on both
 * sides. bmxSoundPlayAt (0x0044e300) is not part of this module's port.
 */
#include <stddef.h>

#include "dt.h"
#include "dt_zmem.h"

#include "bmx/com/menu/gui/guiinput.h"
#include "bmx/com/zge/bmx/bmxsound.h"
#include "bmx/com/zge/zsndseq.h"
#include "bmx/com/zge/zsndsrc.h"
#include "bmx/pc/zge/sound/winapi.h"
#include "bmx/pc/zge/sound/winchan.h"
#include "bmx/pc/zge/sound/winstrm.h"
#include "compat/crt.h"

static uint32_t gLog[1024];
static uint32_t gLogN;

static void logU(uint32_t v)
{
    if (gLogN < sizeof gLog / sizeof gLog[0]) {
        gLog[gLogN] = v;
    }
    gLogN++;
}

static void logP(const void *p)
{
    logU(dt_canon((uint32_t)(uintptr_t)p));
}

/* Hash of a C string (the literals live at different addresses in the exe and the DLL). */
static uint32_t hash_str(const char *s)
{
    uint32_t h = 0;

    while (*s != '\0') {
        h = h * 31 + (uint8_t)*s++;
    }
    return h;
}

/* ---- scripted results ---------------------------------------------------------------------------- */

static struct {
    int failStep; /* bmxSoundOpen: which step fails (0: none) */
    uint32_t capsMin, capsMax;
    uint32_t timerId;
    uint32_t state;
    uint32_t fileCount;
    int guiUpdateResult;
    uint16_t handle;
    uint32_t platformWindow;
} gScript;

static int step(int n)
{
    return gScript.failStep == n;
}

/* ---- stubs --------------------------------------------------------------------------------------- */

static void BMX_CDECL stub_appLog(const char *message)
{
    logU(0x100);
    logU(hash_str(message));
}

static int BMX_CDECL stub_playEvent(int id, int a, int b, int c, int d)
{
    logU(0x101);
    logU((uint32_t)id);
    logU((uint32_t)a);
    logU((uint32_t)b);
    logU((uint32_t)c);
    logU((uint32_t)d);
    return 0;
}

static int BMX_CDECL stub_zrollerShutdown(void)
{
    logU(0x102);
    return 0;
}

static void BMX_CDECL stub_modeShutdown(void)
{
    logU(0x103);
}

static void BMX_CDECL stub_padShutdown(void)
{
    logU(0x104);
}

static GuiPad *gFakePad;

static uint32_t BMX_CDECL stub_padPoll(void)
{
    logU(0x105);
    return 0;
}

static GuiPad *BMX_CDECL stub_padGet(uint32_t index)
{
    logU(0x106);
    logU(index);
    return gFakePad;
}

static void BMX_CDECL stub_appShowError(void)
{
    logU(0x107);
}

static int BMX_CDECL stub_guiUpdate(void)
{
    logU(0x108);
    return gScript.guiUpdateResult;
}

static int BMX_CDECL stub_setTickRate(int ticks)
{
    logU(0x109);
    logU((uint32_t)ticks);
    return ticks ^ 0x55;
}

static uint32_t BMX_CDECL stub_probe(void *window)
{
    logU(0x10a);
    logU((uint32_t)(uintptr_t)window);
    return 0x105;
}

static int BMX_CDECL stub_nop(void)
{
    logU(0x10b);
    return 0;
}

/* open */
static int BMX_CDECL stub_getObject(int kind, void **object, int flags)
{
    static uint32_t platform[2];

    logU(0x110);
    logU((uint32_t)kind);
    logU((uint32_t)flags);
    platform[0] = gScript.platformWindow;
    *object = platform;
    return 1;
}

static int BMX_CDECL stub_winchanInit(const WinchanInitDesc *desc)
{
    logU(0x111);
    logU((uint32_t)(uintptr_t)desc->window);
    logU(desc->format);
    logU(desc->rate);
    logU(desc->chunkMillis | (uint32_t)(desc->echoSeconds != 0.0f));
    return step(1);
}

static int BMX_CDECL stub_seqReset(void)
{
    logU(0x112);
    logU(gZsndTicksPerSec);
    return step(2);
}

static int BMX_CDECL stub_srcInit(uint32_t range, int lifetime)
{
    logU(0x113);
    logU(range);
    logU((uint32_t)lifetime);
    return step(3);
}

static int BMX_CDECL stub_load(const char *path, int resident, uint16_t *handle)
{
    static int calls;

    logU(0x114);
    logU(hash_str(path));
    logU((uint32_t)resident);
    *handle = (uint16_t)(0x1100 + calls);
    logP(handle);
    calls++;
    return step(4 + (resident != 0 ? 0 : (path[10] == 'M' ? 1 : 2)));
}

static uint32_t BMX_STDCALL imp_getDevCaps(WinTimeCaps *caps, uint32_t size)
{
    logU(0x120);
    logU(size);
    caps->periodMin = gScript.capsMin;
    caps->periodMax = gScript.capsMax;
    return step(7) ? 1 : 0;
}

static uint32_t BMX_STDCALL imp_beginPeriod(uint32_t period)
{
    logU(0x121);
    logU(period);
    return 0;
}

static uint32_t BMX_STDCALL imp_setEvent(uint32_t delay, uint32_t resolution, WinTimeCallback callback,
                                         uint32_t user, uint32_t flags)
{
    logU(0x122);
    logU(delay);
    logU(resolution);
    logU((uint32_t)(uintptr_t)callback);
    logU(user);
    logU(flags);
    return step(8) ? 0 : gScript.timerId;
}

static uint32_t BMX_STDCALL imp_endPeriod(uint32_t period)
{
    logU(0x123);
    logU(period);
    return 0;
}

static uint32_t BMX_STDCALL imp_killEvent(uint32_t id)
{
    logU(0x124);
    logU(id);
    return 0;
}

/* close */
static int BMX_CDECL stub_unload(uint16_t file)
{
    logU(0x130);
    logU(file);
    return 0;
}

static void BMX_CDECL stub_srcShutdown(void)
{
    logU(0x131);
}

static int BMX_CDECL stub_seqStopAll(void)
{
    logU(0x132);
    return 0;
}

static void BMX_CDECL stub_winchanShutdown(void)
{
    logU(0x133);
}

/* update */
static int BMX_CDECL stub_strmGetState(const WinstrmStream *stream, uint32_t *state)
{
    logU(0x140);
    logP(stream);
    *state = gScript.state;
    return 0;
}

static uint32_t BMX_CDECL stub_sampleFileCount(uint16_t file)
{
    logU(0x141);
    logU(file);
    return gScript.fileCount;
}

static WinloadSampleFile gFakeSampleFile;

static WinloadSampleFile *BMX_CDECL stub_sampleFile(uint16_t file, uint32_t index)
{
    logU(0x142);
    logU(file);
    logU(index);
    return &gFakeSampleFile;
}

static int BMX_CDECL stub_replace(WinstrmStream *stream, const WinloadSampleFile *file, uint32_t flags)
{
    logU(0x143);
    logP(stream);
    logP(file);
    logU(flags);
    return 0;
}

static int BMX_CDECL stub_volumePan(WinstrmStream *stream, uint8_t volume, int8_t pan)
{
    logU(0x144);
    logP(stream);
    logU(volume);
    logU((uint8_t)pan);
    return 0;
}

static int BMX_CDECL stub_strmStart(WinstrmStream *stream)
{
    logU(0x145);
    logP(stream);
    return 0;
}

static int BMX_CDECL stub_seqUpdate(void)
{
    logU(0x146);
    return 0;
}

static int BMX_CDECL stub_srcUpdate(void)
{
    logU(0x147);
    return 0;
}

static int BMX_CDECL stub_updateFades(void)
{
    logU(0x148);
    return 0;
}

static int BMX_CDECL stub_strmUpdate(void)
{
    logU(0x149);
    return 0;
}

static int BMX_CDECL stub_measure(const ZVec3 *position, uint8_t *volume, int8_t *pan)
{
    logU(0x150);
    logP(position);
    *volume = (uint8_t)dt_u32();
    *pan = (int8_t)dt_u32();
    return (int)(dt_u32() & 1);
}

static void stubs_on(void)
{
    gLogN = 0;
    dt_stub(0x0045a1c0, NULL, (void *)stub_appLog);
    dt_stub(0x0044e300, NULL, (void *)stub_playEvent);
    dt_stub(0x0046c360, NULL, (void *)stub_zrollerShutdown);
    dt_stub(0x004377d0, NULL, (void *)stub_modeShutdown);
    dt_stub(0x00437290, NULL, (void *)stub_padShutdown);
    dt_stub(0x004372b0, NULL, (void *)stub_padPoll);
    dt_stub(0x00437420, NULL, (void *)stub_padGet);
    dt_stub(0x0045a250, NULL, (void *)stub_appShowError);
    dt_stub(0x00437c40, NULL, (void *)stub_guiUpdate);
    dt_stub(0x0047eba0, NULL, (void *)stub_setTickRate);
    dt_stub(0x0046f380, NULL, (void *)stub_probe);
    dt_stub(0x0047ebc0, NULL, (void *)stub_nop);
    dt_stub(0x0046d940, NULL, (void *)stub_winchanInit);
    dt_stub(0x0047c4c0, NULL, (void *)stub_seqReset);
    dt_stub(0x0047e0b0, NULL, (void *)stub_srcInit);
    dt_stub(0x0046fb20, NULL, (void *)stub_load);
    dt_stub(0x004700b0, NULL, (void *)stub_unload);
    dt_stub(0x0047e180, NULL, (void *)stub_srcShutdown);
    dt_stub(0x0047c530, NULL, (void *)stub_seqStopAll);
    dt_stub(0x0046df20, NULL, (void *)stub_winchanShutdown);
    dt_stub(0x00471840, NULL, (void *)stub_strmGetState);
    dt_stub(0x00470260, NULL, (void *)stub_sampleFileCount);
    dt_stub(0x00470620, NULL, (void *)stub_sampleFile);
    dt_stub(0x00471450, NULL, (void *)stub_replace);
    dt_stub(0x00471b60, NULL, (void *)stub_volumePan);
    dt_stub(0x00471890, NULL, (void *)stub_strmStart);
    dt_stub(0x0047c580, NULL, (void *)stub_seqUpdate);
    dt_stub(0x0047e200, NULL, (void *)stub_srcUpdate);
    dt_stub(0x00470ba0, NULL, (void *)stub_updateFades);
    dt_stub(0x00470d10, NULL, (void *)stub_strmUpdate);
    dt_stub(0x0047ea10, NULL, (void *)stub_measure);
    gZsystemGetObject = stub_getObject;
    gWinTimeGetDevCaps = imp_getDevCaps;
    gWinTimeBeginPeriod = imp_beginPeriod;
    gWinTimeSetEvent = imp_setEvent;
    gWinTimeEndPeriod = imp_endPeriod;
    gWinTimeKillEvent = imp_killEvent;
}

static void begin(DtCtx *c)
{
    dt_seed(c);
    dt_zmem_env();
    stubs_on();
    memset(&gScript, 0, sizeof gScript);
    gScript.capsMin = (uint32_t)dt_range(0, 5);
    gScript.capsMax = (uint32_t)dt_range(0, 20);
    gScript.timerId = (uint32_t)dt_range(1, 100);
    gScript.state = (uint32_t)dt_range(0, 3);
    gScript.fileCount = (uint32_t)dt_range(0, 12);
    gScript.guiUpdateResult = (int)(dt_u32() % 3);
    gScript.platformWindow = dt_u32();
}

/* ---- setups -------------------------------------------------------------------------------------- */

static void setup_shutdown(DtCtx *c)
{
    begin(c);
}

static void setup_menu_update(DtCtx *c)
{
    begin(c);
    gFakePad = dt_alloc(sizeof *gFakePad);
    gFakePad->held = dt_u32();
    gFakePad->pressed = c->seed % 3 == 0 ? dt_u32() & 0x300 : dt_u32();
    if (c->seed % 3 == 1) {
        gFakePad->pressed = 0;
    }
    gFakePad->flags2 = (uint16_t)(c->seed % 4 == 0 ? 0 : dt_u32());
    gFakePad->flags = (uint16_t)dt_u32();
    gBmxMenuExit = (int32_t)(dt_u32() % 3 == 0);
    gBmxMenuExitResult = (int32_t)(dt_u32() & 1);
}

static void setup_exit(DtCtx *c)
{
    begin(c);
    DT_ARG_INT(c, 0, dt_u32());
}

static void setup_tick(DtCtx *c)
{
    begin(c);
    DT_ARG_INT(c, 0, dt_u32());
}

static void setup_open(DtCtx *c)
{
    begin(c);
    gBmxSoundState = c->seed % 16 == 5 ? 0 : kBmxSoundClosed;
    gScript.failStep = c->seed % 16 < 10 ? (int)(c->seed % 16) : 0;
    if (c->seed % 16 == 9) {
        gScript.failStep = 0;
    }
    gZsndTicksPerSec = dt_u32();
    /* Timer caps: the period is the resolution clamped to [1, max]. */
    if (c->seed % 5 == 1) {
        gScript.capsMin = 0;
    }
    if (c->seed % 7 == 2) {
        gScript.capsMax = 0;
    }
}

static void setup_close(DtCtx *c)
{
    begin(c);
    gBmxSoundState = c->seed % 6 == 0 ? kBmxSoundClosed : kBmxSoundOpen;
    gBmxTimerId = dt_u32();
    gBmxTimerPeriod = dt_u32();
}

static void setup_update(DtCtx *c)
{
    int i;

    begin(c);
    gBmxSoundState = c->seed % 12 == 0 ? kBmxSoundClosed : (c->seed % 3 == 0 ? 0 : kBmxSoundInGame);
    gBmxMusicMode = c->seed % 9 == 4 ? dt_range(-2, 5) : dt_range(0, 2);
    gBmxMusicStream = (WinstrmStream *)dt_alloc(sizeof(WinstrmStream));
    gBmxMusicFile = (uint16_t)dt_range(0, 15);
    gBmxMusicVolume = (uint8_t)dt_u32();
    gBmxNextTrack = c->seed % 4 == 0 ? (uint8_t)dt_range(0, 12) : kBmxSoundNoTrack;
    gBmxTrackLoop = (uint8_t)(dt_u32() & 1);
    gBmxCurrentTrack = c->seed % 3 == 0 ? kBmxSoundNoTrack : (uint8_t)dt_range(0, 14);
    gBmxLevelIndex = (uint8_t)dt_range(0, 19);
    for (i = 0; i < 20; i++) {
        gBmxLevelTracks[i] = dt_u32() % 5 == 0 ? (uint8_t)dt_range(-3, 3) : (uint8_t)dt_range(0, 12);
    }
    gBmxTrackCooldown = c->seed % 5 == 0 ? (uint32_t)dt_range(1, 3) : 0;
    gScript.state = c->seed % 2 ? 1 : (uint32_t)dt_range(0, 3);
    if (c->seed % 11 == 3) {
        gBmxCurrentTrack = 127; /* the next track number wraps past the signed range */
        gBmxTrackLoop = 1;
        gBmxNextTrack = kBmxSoundNoTrack;
    }
}

static void setup_streams(DtCtx *c)
{
    begin(c);
    gBmxSoundState = c->seed % 3 == 0 ? kBmxSoundClosed : kBmxSoundOpen;
}

static void setup_measure(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, c->seed % 4 == 0 ? NULL : dt_alloc(12));
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_PTR(c, 2, dt_alloc(4));
    DT_ARG_PTR(c, 3, dt_alloc(4));
}

static void setup_none(DtCtx *c)
{
    begin(c);
}

DT_CASES(bmxsound_cases) = {
    {"bmxMenuShutdown", 0x0044dc30, bmxMenuShutdown, 0, DT_RET_VOID, setup_shutdown, 4, 0},
    {"bmxMenuUpdate", 0x0044dc70, bmxMenuUpdate, 0, DT_RET_INT, setup_menu_update, 300, 0},
    {"bmxMenuRequestExit", 0x0044dd10, bmxMenuRequestExit, 1, DT_RET_VOID, setup_exit, 10, 0},
    {"bmxSoundSetTickRate", 0x0044dd40, bmxSoundSetTickRate, 1, DT_RET_INT, setup_tick, 10, 0},
    {"bmxSoundProbeDevice", 0x0044dd60, bmxSoundProbeDevice, 1, DT_RET_INT, setup_tick, 10, 0},
    {"bmxSoundNop", 0x0044dd80, bmxSoundNop, 0, DT_RET_INT, setup_none, 4, 0},
    {"bmxSoundOpen", 0x0044dda0, bmxSoundOpen, 0, DT_RET_VOID, setup_open, 400, 0},
    {"bmxSoundClose", 0x0044e090, bmxSoundClose, 0, DT_RET_VOID, setup_close, 60, 0},
    {"bmxSoundUpdate", 0x0044e120, bmxSoundUpdate, 0, DT_RET_VOID, setup_update, 1500, 0},
    {"bmxSoundUpdateStreams", 0x0044e2e0, bmxSoundUpdateStreams, 0, DT_RET_VOID, setup_streams, 10, 0},
    {"bmxSoundMeasure", 0x0044ffb0, bmxSoundMeasure, 4, DT_RET_INT, setup_measure, 200, 0},
};
