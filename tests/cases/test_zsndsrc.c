/*
 * Tests for the positional sound sources and the stream wrappers (zsndsrc.c). The listener transform
 * (zsystemToListener), the mixer channel calls and the stream player calls are stubs that log their
 * (masked) arguments and answer from the test's random stream. Source tables are random but well formed.
 * The assert paths (module not initialised, handle 0xffff, source not allocated) end the run in
 * zdebugFail on both sides.
 */
#include <stddef.h>

#include "dt.h"
#include "dt_zmem.h"

#include "bmx/com/zge/zsndseq.h"
#include "bmx/com/zge/zsndsrc.h"
#include "bmx/pc/zge/sound/winchan.h"
#include "compat/crt.h"

static uint32_t gLog[2048];
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

static uint32_t garbage_high(uint32_t low)
{
    return low | (dt_u32() & 0xffff0000u);
}

/* ---- stubs --------------------------------------------------------------------------------------- */

static ZsdVoice gVoices[6];
static uint8_t gAllocFails; /* 1 in n allocations fail */
static ZVec3 gListener;
static float gScale;
static int gSpecialLocal;
static uint32_t gStateValue;
static WinstrmStream gFakeStream[2];
static int gWinstrmInitResult;
static int gCreateResult;

static void BMX_CDECL stub_toListener(const ZVec3 *world, ZVec3 *local)
{
    logU(0x100);
    logU(*(const uint32_t *)&world->x);
    logU(*(const uint32_t *)&world->y);
    logU(*(const uint32_t *)&world->z);
    local->x = world->x * gScale - gListener.x;
    local->y = world->y * gScale - gListener.y;
    local->z = world->z * gScale - gListener.z;
    switch (gSpecialLocal) {
    case 1:
        local->x = 0.0f;
        break;
    case 2:
        local->z = 0.0f;
        break;
    case 3:
        local->y = 0.0f;
        break;
    case 4:
        local->x = local->x / 0.0f * 0.0f; /* NaN */
        break;
    case 5:
        local->y = 0.0f;
        local->x = 0.0f;
        break;
    default:
        break;
    }
}

static uint8_t BMX_CDECL stub_alloc(const ZsdVoice *voice, int priority)
{
    const uint32_t r = dt_u32();

    logU(0x101);
    logP(voice);
    logU((uint32_t)priority);
    return gAllocFails && r % gAllocFails == 0 ? 0xff : (uint8_t)(r >> 8) % 60;
}

static int BMX_CDECL stub_free(uint8_t ch)
{
    logU(0x102);
    logU(ch);
    return 0;
}

static int BMX_CDECL stub_start(uint8_t ch)
{
    logU(0x103);
    logU(ch);
    return 0;
}

static int BMX_CDECL stub_volume(uint8_t ch, uint8_t vol)
{
    logU(0x104);
    logU(ch);
    logU(vol);
    return 0;
}

static int BMX_CDECL stub_pan(uint8_t ch, int8_t pan)
{
    logU(0x105);
    logU(ch);
    logU((uint8_t)pan);
    return 0;
}

static int BMX_CDECL stub_pitch(uint8_t ch, int16_t cents)
{
    logU(0x106);
    logU(ch);
    logU((uint16_t)cents);
    return 0;
}

static const ZsdVoice *BMX_CDECL stub_getVoice(uint8_t ch)
{
    logU(0x107);
    logU(ch);
    return &gVoices[ch % 6];
}

/* Stream player. */
static int BMX_CDECL stub_strmInit(void)
{
    logU(0x110);
    return gWinstrmInitResult;
}

static int BMX_CDECL stub_strmShutdown(void)
{
    logU(0x111);
    return 7;
}

static int BMX_CDECL stub_strmDestroy(WinstrmStream *s)
{
    logU(0x112);
    logP(s);
    return 8;
}

static int BMX_CDECL stub_strmCreate(WinstrmStream **out, const WinloadSampleFile *file, uint32_t reserved,
                                     uint32_t bytes, uint32_t flags)
{
    logU(0x113);
    logP(file);
    logU(reserved);
    logU(bytes);
    logU(flags);
    *out = gCreateResult == 0 ? &gFakeStream[1] : NULL;
    return gCreateResult;
}

static int BMX_CDECL stub_strmRepeat(WinstrmStream *s, int count)
{
    logU(0x114);
    logP(s);
    logU((uint32_t)count);
    return 9;
}

static int BMX_CDECL stub_strmState(const WinstrmStream *s, uint32_t *state)
{
    logU(0x115);
    logP(s);
    *state = gStateValue;
    return 10;
}

static int BMX_CDECL stub_strmStart(WinstrmStream *s)
{
    logU(0x116);
    logP(s);
    return 11;
}

static int BMX_CDECL stub_strmStop(WinstrmStream *s)
{
    logU(0x117);
    logP(s);
    return 12;
}

static int BMX_CDECL stub_strmVolume(WinstrmStream *s, uint8_t volume, int8_t pan)
{
    logU(0x118);
    logP(s);
    logU(volume);
    logU((uint8_t)pan);
    return 13;
}

static int BMX_CDECL stub_strmFade(WinstrmStream *s, int millis, uint32_t target, uint32_t flags)
{
    logU(0x119);
    logP(s);
    logU((uint32_t)millis);
    logU(target);
    logU(flags);
    return 14;
}

static int BMX_CDECL stub_strmPauseAll(int pause)
{
    logU(0x11a);
    logU((uint32_t)pause);
    return 15;
}

static int BMX_CDECL stub_strmFlag(uint32_t *flag)
{
    logU(0x11b);
    logP(flag);
    return 16;
}

static void stubs_on(void)
{
    gLogN = 0;
    dt_stub(0x0045a340, NULL, (void *)stub_toListener);
    dt_stub(0x0046e410, NULL, (void *)stub_alloc);
    dt_stub(0x0046e890, NULL, (void *)stub_free);
    dt_stub(0x0046e9a0, NULL, (void *)stub_start);
    dt_stub(0x0046ed20, NULL, (void *)stub_volume);
    dt_stub(0x0046ee80, NULL, (void *)stub_pan);
    dt_stub(0x0046efb0, NULL, (void *)stub_pitch);
    dt_stub(0x0046f170, NULL, (void *)stub_getVoice);
    dt_stub(0x00470aa0, NULL, (void *)stub_strmInit);
    dt_stub(0x00470b20, NULL, (void *)stub_strmShutdown);
    dt_stub(0x004712f0, NULL, (void *)stub_strmDestroy);
    dt_stub(0x00470fb0, NULL, (void *)stub_strmCreate);
    dt_stub(0x00471610, NULL, (void *)stub_strmRepeat);
    dt_stub(0x00471840, NULL, (void *)stub_strmState);
    dt_stub(0x00471890, NULL, (void *)stub_strmStart);
    dt_stub(0x00471a20, NULL, (void *)stub_strmStop);
    dt_stub(0x00471b60, NULL, (void *)stub_strmVolume);
    dt_stub(0x00471c40, NULL, (void *)stub_strmFade);
    dt_stub(0x00471ce0, NULL, (void *)stub_strmPauseAll);
    dt_stub(0x00471d10, NULL, (void *)stub_strmFlag);
}

/* ---- fixtures ------------------------------------------------------------------------------------ */

static uint8_t rand_byte(void)
{
    return (uint8_t)dt_u32();
}

static float rand_coord(void)
{
    return dt_float_edgy(-3000.0f, 3000.0f);
}

static void make_world(int used)
{
    int i;

    memset(&gZsndSources, 0, sizeof gZsndSources);
    gZsndSrcInitialized = 1;
    gZsndSrcRange = dt_u32() % 5 == 0 ? (uint32_t)dt_range(50, 20000) : 2500;
    gZsndSrcRange2 = gZsndSrcRange * gZsndSrcRange;
    gZsndSrcLifetime = (uint32_t)dt_range(1, 12);
    gZsndSrcDopplerScale = dt_u32() % 3 == 0 ? 0.0f : dt_float(-0.01f, 0.01f);
    gZsndSrcNoDoppler = (int32_t)(dt_u32() & 1);
    gZsndSrcNext = (uint32_t)dt_range(0, kZsndSrcMax - 1);
    for (i = 0; i < 6; i++) {
        memset(&gVoices[i], (int)i, sizeof gVoices[i]);
    }
    gListener.x = dt_float(-500.0f, 500.0f);
    gListener.y = dt_float(-500.0f, 500.0f);
    gListener.z = dt_float(-500.0f, 500.0f);
    gScale = dt_u32() % 4 == 0 ? 1.0f : dt_float(0.2f, 1.5f);
    gSpecialLocal = dt_u32() % 6 == 0 ? dt_range(1, 5) : 0;
    gAllocFails = dt_u32() % 4 == 0 ? 3 : 0;
    for (i = 0; i < kZsndSrcMax; i++) {
        ZsndSource *s = &gZsndSources[i];

        if (dt_range(0, 23) >= used) {
            s->flags = (uint16_t)(dt_u32() & 0x7fff);
            continue;
        }
        s->flags = (uint16_t)(kZsndSrcUsed | (dt_u32() & 0x7fff));
        s->channel = dt_u32() % 3 == 0 ? 0xff : (uint8_t)dt_range(0, 59);
        s->voice = &gVoices[dt_range(0, 5)];
        s->lifetime = dt_u32() % 5 == 0 ? 0 : (dt_u32() % 4 == 0 ? 1 : (uint32_t)dt_range(2, 12));
        s->position.x = rand_coord();
        s->position.y = rand_coord();
        s->position.z = rand_coord();
        s->distance2 = dt_u32() % 100000;
        s->doppler = (int16_t)dt_range(-800, 800);
        s->pitch = (int16_t)dt_range(-1200, 1200);
        s->volume = rand_byte();
        s->baseVolume = (uint8_t)dt_range(0, 127);
        s->pan = (int8_t)rand_byte();
    }
}

static void begin(DtCtx *c, int used)
{
    dt_seed(c);
    dt_zmem_env();
    stubs_on();
    make_world(used);
    gZsndTicksPerSec = (uint32_t)dt_range(1, 120);
    (void)c;
}

/* Handle of a used slot (forced when the random world has none). */
static uint16_t live_handle(void)
{
    int i = dt_range(0, kZsndSrcMax - 1), n;

    for (n = 0; n < kZsndSrcMax; n++, i = (i + 1) % kZsndSrcMax) {
        if ((gZsndSources[i].flags & kZsndSrcUsed) != 0) {
            return (uint16_t)i;
        }
    }
    gZsndSources[3].flags |= kZsndSrcUsed;
    return 3;
}

static ZVec3 *rand_position(void)
{
    ZVec3 *p = dt_alloc(sizeof *p);

    p->x = rand_coord();
    p->y = rand_coord();
    p->z = rand_coord();
    return p;
}

/* ---- setups -------------------------------------------------------------------------------------- */

static void setup_init(DtCtx *c)
{
    begin(c, 4);
    gZsndSrcInitialized = c->seed % 6 == 0 ? 1 : 0;
    DT_ARG_INT(c, 0, c->seed % 7 == 3 ? 0 : (uint32_t)dt_range(1, 20000));
    DT_ARG_INT(c, 1, dt_range(0, 5000));
}

static void setup_shutdown(DtCtx *c)
{
    begin(c, dt_range(0, 24));
    gZsndSrcInitialized = c->seed % 8 != 0;
}

static void setup_update(DtCtx *c)
{
    begin(c, dt_range(1, 12));
    if (c->seed % 13 == 5) {
        gZsndSrcInitialized = 0;
    }
}

static void setup_create(DtCtx *c)
{
    int i;

    begin(c, dt_range(0, 8));
    if (c->seed % 7 == 3) {
        for (i = 0; i < kZsndSrcMax; i++) {
            gZsndSources[i].flags |= kZsndSrcUsed;
        }
    }
    if (c->seed % 11 == 4) {
        gZsndSrcInitialized = 0;
    }
    DT_ARG_PTR(c, 0, &gVoices[dt_range(0, 5)]);
    DT_ARG_PTR(c, 1, rand_position());
}

static void setup_handle(DtCtx *c)
{
    begin(c, dt_range(1, 8));
    DT_ARG_INT(c, 0, garbage_high(live_handle()));
}

/* Out of range handle (0xffff), unallocated source, or uninitialised module. */
static void setup_handle_bad(DtCtx *c)
{
    int i;

    begin(c, dt_range(1, 8));
    switch (c->seed % 3) {
    case 0:
        DT_ARG_INT(c, 0, garbage_high(0xffff) | 0xffff0000u);
        break;
    case 1:
        for (i = 0; i < kZsndSrcMax && (gZsndSources[i].flags & kZsndSrcUsed) != 0; i++) {
        }
        if (i == kZsndSrcMax) {
            gZsndSources[2].flags = 0;
            i = 2;
        }
        DT_ARG_INT(c, 0, garbage_high((uint32_t)i));
        break;
    default:
        gZsndSrcInitialized = 0;
        DT_ARG_INT(c, 0, garbage_high(live_handle()));
    }
    DT_ARG_PTR(c, 1, rand_position());
}

static void setup_voice(DtCtx *c)
{
    setup_handle(c);
    DT_ARG_PTR(c, 1, &gVoices[dt_range(0, 5)]);
}

static void setup_position(DtCtx *c)
{
    setup_handle(c);
    DT_ARG_PTR(c, 1, rand_position());
}

static void setup_pitch(DtCtx *c)
{
    setup_handle(c);
    DT_ARG_INT(c, 1, garbage_high((uint16_t)dt_u32()));
}

static void setup_volume(DtCtx *c)
{
    setup_handle(c);
    DT_ARG_INT(c, 1, garbage_high(rand_byte()));
}

static void setup_measure(DtCtx *c)
{
    begin(c, 1);
    if (c->seed % 11 == 4) {
        gZsndSrcInitialized = 0;
    }
    DT_ARG_PTR(c, 0, rand_position());
    DT_ARG_PTR(c, 1, dt_alloc(4));
    DT_ARG_PTR(c, 2, dt_alloc(4));
}

static void setup_noargs(DtCtx *c)
{
    begin(c, 1);
}

/* ---- stream wrapper setups ---- */

static void stream_world(DtCtx *c)
{
    begin(c, 1);
    gZsndStream = c->seed % 3 == 0 ? NULL : &gFakeStream[0];
    gZsndStreamBytes = dt_u32();
    gWinstrmInitResult = c->seed % 3;
    gCreateResult = c->seed % 4 == 1 ? 0x201 : 0;
    gStateValue = (uint32_t)dt_range(0, 4);
}

static void setup_stream_init(DtCtx *c)
{
    stream_world(c);
    DT_ARG_INT(c, 0, dt_u32());
}

static void setup_stream_play(DtCtx *c)
{
    WinloadSampleFile *f = dt_alloc(sizeof *f);

    stream_world(c);
    DT_ARG_PTR(c, 0, f);
    DT_ARG_INT(c, 1, garbage_high((uint32_t)dt_range(0, 7)));
    DT_ARG_INT(c, 2, dt_range(-2, 9));
}

static void setup_stream_state(DtCtx *c)
{
    stream_world(c);
    DT_ARG_PTR(c, 0, dt_alloc(4));
}

static void setup_stream_volume(DtCtx *c)
{
    stream_world(c);
    DT_ARG_INT(c, 0, garbage_high(rand_byte()));
    DT_ARG_INT(c, 1, garbage_high(rand_byte()));
}

static void setup_stream_fade(DtCtx *c)
{
    stream_world(c);
    DT_ARG_INT(c, 0, dt_range(-5, 5000));
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_INT(c, 2, dt_u32());
}

static void setup_stream_int(DtCtx *c)
{
    stream_world(c);
    DT_ARG_INT(c, 0, dt_u32() & 1 ? dt_u32() : 0);
}

static void setup_stream_ptr(DtCtx *c)
{
    stream_world(c);
    DT_ARG_PTR(c, 0, dt_alloc(8));
}

DT_CASES(zsndsrc_cases) = {
    {"zsndStreamInit", 0x0047de20, zsndStreamInit, 1, DT_RET_VOID, setup_stream_init, 20, 0},
    {"zsndStreamShutdown", 0x0047de50, zsndStreamShutdown, 0, DT_RET_INT, stream_world, 20, 0},
    {"zsndStreamNop", 0x0047de90, zsndStreamNop, 0, DT_RET_INT, setup_noargs, 2, 0},
    {"zsndStreamPlay", 0x0047deb0, zsndStreamPlay, 3, DT_RET_INT, setup_stream_play, 100, 0},
    {"zsndStreamGetState", 0x0047df40, zsndStreamGetState, 1, DT_RET_INT, setup_stream_state, 20, 0},
    {"zsndStreamStart", 0x0047df70, zsndStreamStart, 0, DT_RET_INT, stream_world, 10, 0},
    {"zsndStreamStop", 0x0047dfa0, zsndStreamStop, 0, DT_RET_INT, stream_world, 10, 0},
    {"zsndStreamStop2", 0x0047dfd0, zsndStreamStop2, 0, DT_RET_INT, stream_world, 10, 0},
    {"zsndStreamSetVolumePan", 0x0047e000, zsndStreamSetVolumePan, 2, DT_RET_INT, setup_stream_volume, 20, 0},
    {"zsndStreamFade", 0x0047e030, zsndStreamFade, 3, DT_RET_INT, setup_stream_fade, 20, 0},
    {"zsndStreamPauseAll", 0x0047e070, zsndStreamPauseAll, 1, DT_RET_VOID, setup_stream_int, 10, 0},
    {"zsndStreamSetFlag", 0x0047e090, zsndStreamSetFlag, 1, DT_RET_VOID, setup_stream_ptr, 5, 0},
    {"zsndSrcInit", 0x0047e0b0, zsndSrcInit, 2, DT_RET_INT, setup_init, 120, 0},
    {"zsndSrcShutdown", 0x0047e180, zsndSrcShutdown, 0, DT_RET_VOID, setup_shutdown, 60, 0},
    {"zsndSrcUpdate", 0x0047e200, zsndSrcUpdate, 0, DT_RET_INT, setup_update, 1500, 0},
    {"zsndSrcClearDoppler", 0x0047e4b0, zsndSrcClearDoppler, 0, DT_RET_VOID, setup_update, 100, 0},
    {"zsndSrcCreate", 0x0047e520, zsndSrcCreate, 2, DT_RET_INT16, setup_create, 400, 0},
    {"zsndSrcDestroy", 0x0047e650, zsndSrcDestroy, 1, DT_RET_VOID, setup_handle, 100, 0},
    {"zsndSrcDestroy_bad", 0x0047e650, zsndSrcDestroy, 1, DT_RET_VOID, setup_handle_bad, 12, 0},
    {"zsndSrcSetVoice", 0x0047e720, zsndSrcSetVoice, 2, DT_RET_INT, setup_voice, 100, 0},
    {"zsndSrcSetVoice_bad", 0x0047e720, zsndSrcSetVoice, 2, DT_RET_INT, setup_handle_bad, 12, 0},
    {"zsndSrcSetPosition", 0x0047e7e0, zsndSrcSetPosition, 2, DT_RET_INT, setup_position, 100, 0},
    {"zsndSrcSetPosition_bad", 0x0047e7e0, zsndSrcSetPosition, 2, DT_RET_INT, setup_handle_bad, 12, 0},
    {"zsndSrcSetPitch", 0x0047e880, zsndSrcSetPitch, 2, DT_RET_INT, setup_pitch, 100, 0},
    {"zsndSrcSetPitch_bad", 0x0047e880, zsndSrcSetPitch, 2, DT_RET_INT, setup_handle_bad, 12, 0},
    {"zsndSrcSetVolume", 0x0047e910, zsndSrcSetVolume, 2, DT_RET_INT, setup_volume, 100, 0},
    {"zsndSrcSetVolume_bad", 0x0047e910, zsndSrcSetVolume, 2, DT_RET_INT, setup_handle_bad, 12, 0},
    {"zsndSrcGetChannel", 0x0047e990, zsndSrcGetChannel, 1, DT_RET_INT8, setup_handle, 100, 0},
    {"zsndSrcGetChannel_bad", 0x0047e990, zsndSrcGetChannel, 1, DT_RET_INT8, setup_handle_bad, 12, 0},
    {"zsndSrcMeasure", 0x0047ea10, zsndSrcMeasure, 3, DT_RET_INT, setup_measure, 1000, 0},
};
