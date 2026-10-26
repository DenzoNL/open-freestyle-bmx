/*
 * Tests for the sound event dispatcher bmxSoundPlayAt (bmxsound.c). Every callee that touches the mixer,
 * sequencer, 3-D sources, loader or stream player is a stub that logs its (masked) arguments and answers
 * from a per-iteration script; zdebugFail logs and returns (the code carries on, as the fixtures stay
 * valid). zmathRandom, zstrFormat and zfileBuildFileName run for real. Each iteration sends one event with
 * arguments shaped for it (or random ones) from a random sound state.
 *
 * zdebugFail ends the run on both sides, so an assert is the last thing compared (the NULL position
 * asserts of events 4001 .. 4003 included).
 */
#include <stddef.h>

#include "dt.h"
#include "dt_zmem.h"

#include "bmx/com/zge/bmx/bmxsound.h"
#include "bmx/com/zge/zsndseq.h"
#include "bmx/com/zge/zsndsrc.h"
#include "bmx/pc/zge/sound/winchan.h"
#include "bmx/pc/zge/sound/winload.h"
#include "bmx/pc/zge/sound/winstrm.h"
#include "compat/crt.h"
#include "zge/zmath.h"

static uint32_t gLog[2048];
static uint32_t gLogN;

static void logU(uint32_t v)
{
    if (gLogN < sizeof gLog / sizeof gLog[0]) {
        gLog[gLogN] = v;
    }
    gLogN++;
}

static void logF(float f)
{
    uint32_t u;

    __builtin_memcpy(&u, &f, 4);
    logU(u);
}

static void logVec(const ZVec3 *v)
{
    if (v == NULL) {
        logU(0xdead);
        return;
    }
    logF(v->x);
    logF(v->y);
    logF(v->z);
}

static uint32_t hash_str(const char *s)
{
    uint32_t h = 0;

    while (*s != '\0') {
        h = h * 31 + (uint8_t)*s++;
    }
    return h;
}

/* ---- script ------------------------------------------------------------------------------------- */

static struct {
    uint32_t trackCount;  /* winloadGetTrackDefCount */
    uint32_t voiceCount;  /* winloadGetVoiceCount */
    uint32_t sampleCount; /* winloadGetSampleFileCount */
    uint32_t loadFails;   /* bit per winloadLoad call */
    uint32_t loadCalls;
    uint16_t nextHandle;
    int createFails;
    uint8_t allocResult;
    uint16_t sourceResult;
    uint8_t measuredVolume;
    int8_t measuredPan;
    int measureResult;
    uint32_t calls; /* running counter for results that change per call */
} gScript;

static ZsdVoice *fake_voice(uint16_t file, uint32_t index)
{
    return (ZsdVoice *)(uintptr_t)(0x30000000u + ((uint32_t)file << 12) + index * 0x14);
}

static uint32_t BMX_CDECL s_setEffect(const WinchanEffect *effect)
{
    logU(0x101);
    logU((uint32_t)effect->kind);
    logU((uint32_t)effect->level & 0xffff);
    return effect->kind == 0 || effect->kind == 1 ? 0 : 0x10d;
}

static int BMX_CDECL s_seqStart(uint8_t track)
{
    logU(0x102);
    logU(track);
    return 0;
}

static int BMX_CDECL s_strmInit(void)
{
    logU(0x103);
    return 0;
}

static uint32_t BMX_CDECL s_sampleFileCount(uint16_t file)
{
    logU(0x104);
    logU(file);
    return gScript.sampleCount;
}

static int BMX_CDECL s_measure(const struct ZVec3 *position, uint8_t volume, uint8_t *outVolume,
                               int8_t *outPan)
{
    logU(0x105);
    logVec((const ZVec3 *)position);
    logU(volume);
    *outVolume = gScript.measuredVolume;
    *outPan = gScript.measuredPan;
    return gScript.measureResult;
}

static int BMX_CDECL s_voicePlay(const ZsdVoice *voice, int16_t cents, uint8_t volume, int8_t pan,
                                 int priority)
{
    logU(0x106);
    logU((uint32_t)(uintptr_t)voice);
    logU((uint16_t)cents);
    logU(volume);
    logU((uint8_t)pan);
    logU((uint32_t)priority);
    return 0;
}

static int BMX_CDECL s_strmStop(WinstrmStream *stream)
{
    logU(0x107);
    logU((uint32_t)(uintptr_t)stream);
    return 0;
}

static uint16_t BMX_CDECL s_srcCreate(const ZsdVoice *voice, const ZVec3 *position)
{
    logU(0x108);
    logU((uint32_t)(uintptr_t)voice);
    logVec(position);
    return (uint16_t)(gScript.sourceResult + gScript.calls++);
}

static uint8_t BMX_CDECL s_seqCreate(const ZsndSeqHeader *header, uint16_t flags)
{
    logU(0x109);
    logU((uint32_t)(uintptr_t)header);
    logU(flags);
    return (uint8_t)(0x40 + gScript.calls++);
}

static int BMX_CDECL s_voiceSetVolume(uint8_t channel, uint8_t volume)
{
    logU(0x10a);
    logU(channel);
    logU(volume);
    return 0;
}

static int BMX_CDECL s_strmFade(WinstrmStream *stream, int millis, uint32_t target, uint32_t flags)
{
    logU(0x10b);
    logU((uint32_t)(uintptr_t)stream);
    logU((uint32_t)millis);
    logU(target);
    logU(flags);
    return 0;
}

static uint32_t BMX_CDECL s_voiceCount(uint16_t file)
{
    logU(0x10c);
    logU(file);
    return gScript.voiceCount;
}

static int BMX_CDECL s_voiceFree(uint8_t channel)
{
    logU(0x10d);
    logU(channel);
    return 0;
}

static WinloadSampleFile *BMX_CDECL s_getSampleFile(uint16_t file, uint32_t index)
{
    logU(0x10e);
    logU(file);
    logU(index);
    return (WinloadSampleFile *)(uintptr_t)(0x31000000u + ((uint32_t)file << 8) + index);
}

static int BMX_CDECL s_load(const char *path, int resident, uint16_t *handle)
{
    const int fails = (int)(gScript.loadFails >> (gScript.loadCalls++ & 31) & 1);

    logU(0x10f);
    logU(hash_str(path));
    logU((uint32_t)resident);
    if (!fails) {
        *handle = gScript.nextHandle++;
    }
    return fails ? 0x301 : 0;
}

static int BMX_CDECL s_strmSetVolumePan(WinstrmStream *stream, uint8_t volume, int8_t pan)
{
    logU(0x110);
    logU((uint32_t)(uintptr_t)stream);
    logU(volume);
    logU((uint8_t)pan);
    return 0;
}

static ZsdTrackDef *BMX_CDECL s_getTrackDef(uint16_t file, uint32_t index)
{
    logU(0x111);
    logU(file);
    logU(index);
    return (ZsdTrackDef *)(uintptr_t)(0x32000000u + ((uint32_t)file << 8) + index);
}

static int BMX_CDECL s_strmShutdown(void)
{
    logU(0x112);
    return 0;
}

static int BMX_CDECL s_voiceSetPan(uint8_t channel, int8_t pan)
{
    logU(0x113);
    logU(channel);
    logU((uint8_t)pan);
    return 0;
}

static int BMX_CDECL s_seqPause(uint8_t track)
{
    logU(0x114);
    logU(track);
    return 0;
}

static int BMX_CDECL s_strmPause(WinstrmStream *stream, int pause)
{
    logU(0x115);
    logU((uint32_t)(uintptr_t)stream);
    logU((uint32_t)pause);
    return 0;
}

static int BMX_CDECL s_srcSetVoice(uint16_t source, const ZsdVoice *voice)
{
    logU(0x116);
    logU(source);
    logU((uint32_t)(uintptr_t)voice);
    return 0;
}

static int BMX_CDECL s_voiceStart(uint8_t channel)
{
    logU(0x117);
    logU(channel);
    return 0;
}

static int BMX_CDECL s_srcSetVolume(uint16_t source, uint8_t volume)
{
    logU(0x118);
    logU(source);
    logU(volume);
    return 0;
}

static ZsdVoice *BMX_CDECL s_getVoice(uint16_t file, uint32_t index)
{
    logU(0x119);
    logU(file);
    logU(index);
    return fake_voice(file, index);
}

static int BMX_CDECL s_strmStart(WinstrmStream *stream)
{
    logU(0x11a);
    logU((uint32_t)(uintptr_t)stream);
    return 0;
}

static void BMX_CDECL s_unload(uint16_t file)
{
    logU(0x11b);
    logU(file);
}

static void BMX_CDECL s_srcDestroy(uint16_t source)
{
    logU(0x11c);
    logU(source);
}

static int BMX_CDECL s_seqSetVolume(uint8_t track, uint8_t volume, uint8_t pan)
{
    logU(0x11d);
    logU(track);
    logU(volume);
    logU(pan);
    return 0;
}

static uint32_t BMX_CDECL s_trackCount(uint16_t file)
{
    logU(0x11e);
    logU(file);
    return gScript.trackCount;
}

static int BMX_CDECL s_strmDestroy(WinstrmStream *stream)
{
    logU(0x11f);
    logU((uint32_t)(uintptr_t)stream);
    return 0;
}

static int BMX_CDECL s_srcSetPosition(uint16_t source, const ZVec3 *position)
{
    logU(0x120);
    logU(source);
    logVec(position);
    return 0;
}

static uint8_t BMX_CDECL s_voiceAlloc(const ZsdVoice *voice, int priority)
{
    logU(0x121);
    logU((uint32_t)(uintptr_t)voice);
    logU((uint32_t)priority);
    return gScript.allocResult == 0xff ? 0xff : (uint8_t)(gScript.allocResult + gScript.calls++);
}

static int BMX_CDECL s_srcSetPitch(uint16_t source, int16_t cents)
{
    logU(0x122);
    logU(source);
    logU((uint16_t)cents);
    return 0;
}

static int BMX_CDECL s_seqDestroy(uint8_t track)
{
    logU(0x123);
    logU(track);
    return 0;
}

static int BMX_CDECL s_strmCreate(WinstrmStream **out, const WinloadSampleFile *file, uint32_t reserved,
                                  uint32_t bufferBytes, uint32_t flags)
{
    logU(0x124);
    logU((uint32_t)(uintptr_t)file);
    logU(reserved);
    logU(bufferBytes);
    logU(flags);
    *out = (WinstrmStream *)(uintptr_t)0x33000000u;
    return gScript.createFails ? 0x205 : 0;
}

static void install_stubs(void)
{
    static const struct {
        uint32_t addr;
        void *stub;
    } kStubs[] = {
        {0x0046f250, (void *)s_setEffect},    {0x0047c940, (void *)s_seqStart},
        {0x00470aa0, (void *)s_strmInit},     {0x00470260, (void *)s_sampleFileCount},
        {0x0044ffb0, (void *)s_measure},      {0x0046e8c0, (void *)s_voicePlay},
        {0x00471a20, (void *)s_strmStop},     {0x0047e520, (void *)s_srcCreate},
        {0x0047c6b0, (void *)s_seqCreate},    {0x0046ed20, (void *)s_voiceSetVolume},
        {0x00471c40, (void *)s_strmFade},     {0x00470140, (void *)s_voiceCount},
        {0x0046e890, (void *)s_voiceFree},    {0x00470620, (void *)s_getSampleFile},
        {0x0046fb20, (void *)s_load},         {0x00471b60, (void *)s_strmSetVolumePan},
        {0x004707c0, (void *)s_getTrackDef},  {0x00470b20, (void *)s_strmShutdown},
        {0x0046ee80, (void *)s_voiceSetPan},  {0x0047cb30, (void *)s_seqPause},
        {0x00471aa0, (void *)s_strmPause},    {0x0047e720, (void *)s_srcSetVoice},
        {0x0046e9a0, (void *)s_voiceStart},   {0x0047e910, (void *)s_srcSetVolume},
        {0x004704a0, (void *)s_getVoice},     {0x00471890, (void *)s_strmStart},
        {0x004700b0, (void *)s_unload},       {0x0047e650, (void *)s_srcDestroy},
        {0x0047d1b0, (void *)s_seqSetVolume}, {0x00470380, (void *)s_trackCount},
        {0x004712f0, (void *)s_strmDestroy},  {0x0047e7e0, (void *)s_srcSetPosition},
        {0x0046e410, (void *)s_voiceAlloc},   {0x0047e880, (void *)s_srcSetPitch},
        {0x0047c870, (void *)s_seqDestroy},   {0x00470fb0, (void *)s_strmCreate},
    };
    size_t i;

    for (i = 0; i < sizeof kStubs / sizeof kStubs[0]; i++) {
        dt_stub(kStubs[i].addr, NULL, kStubs[i].stub);
    }
}

/* ---- fixtures ------------------------------------------------------------------------------------ */

static uint8_t rand_byte(void)
{
    return (uint8_t)dt_u32();
}

static uint16_t rand_file(void)
{
    return dt_u32() % 5 == 0 ? 0xffff : (uint16_t)dt_range(0, 30);
}

static int32_t pick(const int32_t *values, int n)
{
    return values[dt_range(0, n - 1)];
}

static ZVec3 *rand_pos(void)
{
    ZVec3 *v = dt_alloc(sizeof *v);

    v->x = dt_u32() % 6 == 0 ? (dt_u32() & 1 ? 0.0f : -0.0f) : dt_float(-5000.0f, 5000.0f);
    v->y = dt_u32() % 7 == 0 ? -2000.0f : dt_float(-6000.0f, 2000.0f);
    v->z = dt_float(-5000.0f, 5000.0f);
    return v;
}

static void make_state(void)
{
    static const int32_t kStates[] = {-1, 0, 1, 2, 2, 3, 3, 4};
    uint32_t i;

    gLogN = 0;
    memset(&gScript, 0, sizeof gScript);
    gBmxSoundState = dt_u32() % 40 == 0 ? -1 : pick(kStates + 1, 7);
    gBmxMusicMode = dt_range(-1, 5);
    gBmxSfxVolume = dt_u32() % 5 == 0 ? 0x7f : rand_byte();
    gBmxMusicVolume = rand_byte();
    gBmxVolumes[2] = rand_byte();
    gBmxVolumes[3] = rand_byte();
    gBmxSfxFile = (uint16_t)dt_range(0, 30);
    gBmxMenuSfxFile = rand_file();
    gBmxGameSfxFile = rand_file();
    gBmxGunSfxFile = rand_file();
    gBmxRiderSfxFile = rand_file();
    gBmxLevelSfxFile = rand_file();
    gBmxMusicFile = (uint16_t)dt_range(0, 30);
    gBmxNextTrack = dt_u32() % 3 == 0 ? 0xff : rand_byte();
    gBmxCurrentTrack = dt_u32() % 3 == 0 ? 0xff : rand_byte();
    gBmxLevelIndex = dt_u32() % 3 == 0 ? 4 : (uint8_t)dt_range(0, 13);
    gBmxRiderIndex = rand_byte();
    gBmxAmbientSide = dt_u32() % 5 == 0 ? rand_byte() : (uint8_t)(dt_u32() & 1);
    gBmxGunChannel = dt_u32() % 2 == 0 ? 0xff : rand_byte();
    gBmxMenuChannels[0] = rand_byte();
    gBmxMenuChannels[1] = rand_byte();
    gBmxMenuSfxSet = dt_u32() % 4 == 0 ? rand_byte() : (uint8_t)(dt_u32() & 1);
    gBmxMusicStream = (WinstrmStream *)(uintptr_t)(dt_u32() % 4 == 0 ? 0 : 0x34000000u + (dt_u32() & 0xff0));
    gBmxTrackCooldown = (uint32_t)dt_range(0, 100);
    for (i = 0; i < 8; i++) {
        gBmxAmbientTracks[i] = rand_byte();
    }
    for (i = 0; i < 2; i++) {
        gBmxRiderLoopA[i] = (uint16_t)dt_u32();
        gBmxRiderLoopB[i] = (uint16_t)dt_u32();
    }
    gScript.trackCount = dt_u32() % 4 == 0 ? 0 : (uint32_t)dt_range(1, 8);
    gScript.voiceCount = (uint32_t)dt_range(0, 12);
    gScript.sampleCount = (uint32_t)dt_range(0, 20);
    gScript.loadFails = dt_u32() % 4 == 0 ? dt_u32() : 0;
    gScript.nextHandle = (uint16_t)dt_range(0, 30);
    gScript.createFails = dt_u32() % 6 == 0;
    gScript.allocResult = dt_u32() % 4 == 0 ? 0xff : (uint8_t)dt_range(0, 60);
    gScript.sourceResult = dt_u32() % 5 == 0 ? 0xffff : (uint16_t)dt_range(0, 100);
    gScript.measuredVolume = rand_byte();
    gScript.measuredPan = (int8_t)rand_byte();
    gScript.measureResult = (int)(dt_u32() & 1);
    install_stubs();
}

/* Arguments for event `id` (or a random event). */
static void make_event(DtCtx *c, int32_t id)
{
    uint32_t a2 = dt_u32(), a3 = dt_u32(), a4 = dt_u32();
    const void *p1 = NULL;

    switch (id) {
    case 1000: {
        static const int32_t kTargets[] = {-1, 0, 1, 2, 2, 2, 3, 3, 4, 7};

        p1 = (const void *)(uintptr_t)(uint32_t)pick(kTargets, 10);
        a2 = (uint32_t)dt_range(0, 13);
        a3 = (uint32_t)dt_range(0, 3);
        a4 = dt_u32() % 3 == 0 ? dt_u32() : 0;
        break;
    }
    case 1001:
    case 1005:
        p1 = (const void *)(uintptr_t)(uint32_t)dt_range(-1, 21);
        break;
    case 1002:
        p1 = (const void *)(uintptr_t)(uint32_t)dt_range(0, 3);
        break;
    case 3000:
        p1 = (const void *)(uintptr_t)(dt_u32() % 4 == 0 ? 1u : 0u);
        a2 = (uint32_t)dt_range(0, 14);
        break;
    case 4000: {
        const uint32_t rider = dt_u32() % 8 == 0 ? (uint32_t)dt_range(2, 3) : dt_u32() & 1;

        p1 = rand_pos();
        a2 = rider << 28 | (dt_u32() % 3 == 0 ? dt_u32() & 0x0f000000u : 0) | (dt_u32() & 0xff0000) |
             (uint32_t)dt_range(0, 3) << 8 | (dt_u32() & 0xff);
        a3 = dt_u32() % 4 == 0 ? 0 : (uint32_t)dt_range(1, 0x40);
        a4 = dt_u32() % 4 == 0 ? 0 : (uint32_t)dt_range(1, 0x40);
        break;
    }
    case 4001:
        p1 = dt_u32() % 40 == 0 ? NULL : rand_pos();
        break;
    case 4002:
    case 4003:
        p1 = dt_u32() % 40 == 0 ? NULL : rand_pos();
        a2 = (uint32_t)dt_range(0, 5);
        a3 = dt_u32() % 5 == 0 ? 0xffff
                               : (dt_u32() % 6 == 0 ? (uint32_t)dt_range(4, 6) : (uint32_t)dt_range(0, 3));
        a4 = (uint32_t)dt_range(0, 0x80);
        break;
    default:
        if (id >= 3001 && id <= 3009) {
            p1 = dt_u32() % 6 == 0 ? NULL : rand_pos();
            a2 = dt_u32() % 3 == 0 ? dt_u32() : (uint32_t)dt_range(0, 6) << 8 | (dt_u32() & 0xff);
            a3 = dt_u32() % 3 == 0 ? dt_u32() : (uint32_t)dt_range(0, 0x80);
        } else {
            p1 = (const void *)(uintptr_t)(dt_u32() % 3 == 0 ? dt_u32() : (uint32_t)dt_range(0, 12));
            if (dt_u32() % 3 == 0) {
                p1 = (const void *)(uintptr_t)(uint32_t)dt_range(0, 1);
            }
            a2 = dt_u32() % 2 == 0 ? 0 : a2;
        }
        break;
    }
    DT_ARG_INT(c, 0, (uint32_t)id);
    DT_ARG_PTR(c, 1, p1);
    DT_ARG_INT(c, 2, a2);
    DT_ARG_INT(c, 3, a3);
    DT_ARG_INT(c, 4, a4);
}

static void setup_event(DtCtx *c)
{
    static const int32_t kEvents[] = {
        999,  1000, 1000, 1000, 1000, 1001, 1002, 1003, 1004, 1005, 1006, 1999, 2000, 2001, 2002,
        2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2999, 3000, 3001, 3002, 3003, 3004, 3005,
        3006, 3007, 3008, 3009, 3010, 4000, 4000, 4001, 4001, 4002, 4003, 4004, 4999, 5000, 5001,
        5002, 5003, 5004, 5005, 5006, 5007, 5008, 5009, 5010, 5011, 5012, 5999, 6000, 6001, -5,
    };

    dt_seed(c);
    dt_zmem_env();
    make_state();
    make_event(c, dt_u32() % 50 == 0 ? (int32_t)dt_u32()
                                     : pick(kEvents, (int)(sizeof kEvents / sizeof kEvents[0])));
}

/* Event 1000 only: state changes between every pair of states. */
static void setup_state(DtCtx *c)
{
    dt_seed(c);
    dt_zmem_env();
    make_state();
    make_event(c, 1000);
}

/* The control events 1001 .. 1005 in game and menu states. */
static void setup_control(DtCtx *c)
{
    dt_seed(c);
    dt_zmem_env();
    make_state();
    gBmxSoundState = dt_range(1, 3);
    gBmxMusicMode = dt_range(-1, 4);
    make_event(c, dt_range(1001, 1005));
}

/* The positional events 3001 .. 3009 outside the paused state. */
static void setup_positional(DtCtx *c)
{
    dt_seed(c);
    dt_zmem_env();
    make_state();
    gBmxSoundState = dt_range(0, 2);
    make_event(c, dt_range(3001, 3009));
}

/* The level 4 ambient side (event 4001) with level 4 set. */
static void setup_side(DtCtx *c)
{
    dt_seed(c);
    dt_zmem_env();
    make_state();
    gBmxLevelIndex = dt_u32() % 8 == 0 ? 3 : 4;
    if (gBmxSoundState < 0) {
        gBmxSoundState = 2;
    }
    make_event(c, 4001);
}

DT_CASES(bmxsound_big_cases) = {
    {"bmxSoundPlayAt", 0x0044e300, bmxSoundPlayAt, 5, DT_RET_INT, setup_event, 3000, 0},
    {"bmxSoundPlayAt_state", 0x0044e300, bmxSoundPlayAt, 5, DT_RET_INT, setup_state, 800, 0},
    {"bmxSoundPlayAt_control", 0x0044e300, bmxSoundPlayAt, 5, DT_RET_INT, setup_control, 600, 0},
    {"bmxSoundPlayAt_positional", 0x0044e300, bmxSoundPlayAt, 5, DT_RET_INT, setup_positional, 600, 0},
    {"bmxSoundPlayAt_side", 0x0044e300, bmxSoundPlayAt, 5, DT_RET_INT, setup_side, 300, 0},
};
