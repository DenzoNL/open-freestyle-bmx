/*
 * Tests for the sound channels (winchan.c). The mixer state (24 channels, 64 handles, the loaded sound
 * files the voices come from) is built from random but consistent fixtures. DirectSound is replaced by
 * fake COM objects: vtables of stdcall stubs that log their arguments and answer from a script (status,
 * cursor positions, lock results), exactly as the real objects would be called. The mixer proper
 * (winchanMix) is stubbed and logs its range. The CRT assert is stubbed too: it logs the expression and
 * line and the function carries on (the fixtures stay valid, so that is safe).
 *
 * Not covered: winchanVoiceAlloc with every handle taken (the CRT assert would be followed by an endless
 * search), and the real DSOUND.DLL.
 */
#include <stddef.h>

#include "dt.h"
#include "dt_zmem.h"

#include "bmx/pc/zge/sound/winapi.h"
#include "bmx/pc/zge/sound/winchan.h"
#include "bmx/pc/zge/sound/winload.h"
#include "compat/crt.h"

/* ---- stub log ------------------------------------------------------------------------------------ */

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

static uint32_t garbage_high(uint32_t low)
{
    return low | (dt_u32() & 0xffffff00u);
}

static uint8_t rand_byte(void)
{
    return (uint8_t)dt_u32();
}

/* ---- fake DirectSound ---------------------------------------------------------------------------- */

typedef struct FakeBuffer {
    const DsBufferVtbl *vtbl;
    uint32_t id;
} FakeBuffer;

typedef struct FakeDevice {
    const DsDeviceVtbl *vtbl;
    uint32_t id;
} FakeDevice;

/* Script for the buffer calls. */
static struct {
    DsResult statusHr;
    uint32_t status;
    uint32_t play, write;
    DsResult lockHr[2];
    uint32_t lockCalls;
    uint32_t lockBytes1;
    uint32_t lockBytes2;
    uint8_t *dest;
    DsResult coopHr;
    DsResult createHr;
    int loadFails;
    int procFails;
} gScript;

static uint32_t BMX_STDCALL fb_release(DsBuffer *self)
{
    logU(0x201);
    logU(((FakeBuffer *)self)->id);
    return 0;
}

static DsResult BMX_STDCALL fb_getCurrentPosition(DsBuffer *self, uint32_t *play, uint32_t *write)
{
    logU(0x202);
    logU(((FakeBuffer *)self)->id);
    *play = gScript.play;
    *write = gScript.write;
    return 0;
}

static DsResult BMX_STDCALL fb_getStatus(DsBuffer *self, uint32_t *status)
{
    logU(0x203);
    logU(((FakeBuffer *)self)->id);
    if (gScript.statusHr == 0) {
        *status = gScript.status;
    }
    return gScript.statusHr;
}

static DsResult BMX_STDCALL fb_lock(DsBuffer *self, uint32_t offset, uint32_t bytes, void **ptr1,
                                    uint32_t *bytes1, void **ptr2, uint32_t *bytes2, uint32_t flags)
{
    const DsResult hr = gScript.lockHr[gScript.lockCalls > 1 ? 1 : gScript.lockCalls];

    logU(0x204);
    logU(((FakeBuffer *)self)->id);
    logU(offset);
    logU(bytes);
    logU(flags);
    gScript.lockCalls++;
    if (hr == 0) {
        *ptr1 = gScript.dest;
        *bytes1 = gScript.lockBytes1;
        *ptr2 = gScript.dest + 0x3000;
        *bytes2 = gScript.lockBytes2;
    }
    return hr;
}

static DsResult BMX_STDCALL fb_stop(DsBuffer *self)
{
    logU(0x205);
    logU(((FakeBuffer *)self)->id);
    return 0;
}

static DsResult BMX_STDCALL fb_unlock(DsBuffer *self, void *ptr1, uint32_t bytes1, void *ptr2,
                                      uint32_t bytes2)
{
    logU(0x206);
    logU(((FakeBuffer *)self)->id);
    logP(ptr1);
    logU(bytes1);
    logP(ptr2);
    logU(bytes2);
    return 0;
}

static DsResult BMX_STDCALL fb_restore(DsBuffer *self)
{
    logU(0x207);
    logU(((FakeBuffer *)self)->id);
    return 0;
}

static const DsBufferVtbl gBufferVtbl = {
    .release = fb_release,
    .getCurrentPosition = fb_getCurrentPosition,
    .getStatus = fb_getStatus,
    .lock = fb_lock,
    .stop = fb_stop,
    .unlock = fb_unlock,
    .restore = fb_restore,
};

static uint32_t BMX_STDCALL fd_release(DsDevice *self)
{
    logU(0x211);
    logU(((FakeDevice *)self)->id);
    return 0;
}

static DsResult BMX_STDCALL fd_setCooperativeLevel(DsDevice *self, void *window, uint32_t level)
{
    logU(0x212);
    logU(((FakeDevice *)self)->id);
    logU((uint32_t)(uintptr_t)window);
    logU(level);
    return gScript.coopHr;
}

static const DsDeviceVtbl gDeviceVtbl = {
    .release = fd_release,
    .setCooperativeLevel = fd_setCooperativeLevel,
};

static FakeBuffer gBufferA = {&gBufferVtbl, 0xa1};
static FakeBuffer gBufferB = {&gBufferVtbl, 0xb2};
static FakeDevice gDevice = {&gDeviceVtbl, 0xd3};

/* kernel32 imports used by the probe and the shutdown. */
static WinHandle BMX_STDCALL s_loadLibrary(const char *name)
{
    logU(0x221);
    logU((uint32_t)name[0]);
    return (WinHandle)(uintptr_t)(gScript.loadFails ? 0 : 0x1234);
}

static int32_t BMX_STDCALL s_freeLibrary(WinHandle module)
{
    logU(0x222);
    logU((uint32_t)(uintptr_t)module);
    return 1;
}

static int32_t BMX_STDCALL s_directSoundCreate(void *guid, DsDevice **device, void *outer)
{
    logU(0x223);
    logU((uint32_t)(uintptr_t)guid);
    logU((uint32_t)(uintptr_t)outer);
    if (gScript.createHr == 0) {
        *device = (DsDevice *)&gDevice;
    }
    return gScript.createHr;
}

static void *BMX_STDCALL s_getProcAddress(WinHandle module, const char *name)
{
    logU(0x224);
    logU((uint32_t)(uintptr_t)module);
    logU((uint32_t)name[0]);
    return gScript.procFails ? NULL : (void *)s_directSoundCreate;
}

static void BMX_CDECL stub_mix(uint8_t *ring, uint32_t bytes)
{
    logU(0x230);
    logU((uint32_t)(ring - gWinchanRing));
    logU(bytes);
}

/* The CRT assert: logs the expression (hashed, the strings live at different addresses) and the line. */
static void BMX_CDECL stub_assert(const char *expr, const char *file, unsigned line)
{
    uint32_t h = 0;

    while (*expr != '\0') {
        h = h * 31 + (uint8_t)*expr++;
    }
    logU(0x240);
    logU(h);
    logU(line);
}

static void install_ds(void)
{
    gLogN = 0;
    dt_stub(0x004a4de8, NULL, (void *)stub_assert);
    dt_stub(0x0046f5f0, NULL, (void *)stub_mix);
    gWinLoadLibrary = s_loadLibrary;
    gWinFreeLibrary = s_freeLibrary;
    gWinGetProcAddress = s_getProcAddress;
}

/* ---- fixtures ------------------------------------------------------------------------------------ */

enum { kVoiceCount = 6, kFilesLoaded = 2 };

static uint8_t rand_time(void)
{
    switch (dt_u32() % 5) {
    case 0:
        return 0;
    case 1:
        return 0x7f;
    case 2:
        return (uint8_t)dt_range(1, 0x7e);
    default:
        return (uint8_t)dt_range(0, 0x7f);
    }
}

static ZsdVoice *gVoices[kFilesLoaded][kVoiceCount];

/* Two loaded sound files with random voices, samples and sample files. */
static void make_files(void)
{
    int k, j;

    memset(&gWinloadFiles, 0, sizeof gWinloadFiles);
    for (k = 0; k < kFilesLoaded; k++) {
        ZsdHeader *h = dt_alloc(sizeof *h);

        gWinloadFiles[k].flags = kWinloadLoaded | kWinloadSamplesResident;
        gWinloadFiles[k].header = h;
        h->voiceCount = kVoiceCount;
        h->voices = dt_alloc(sizeof(ZsdVoice) * kVoiceCount);
        h->sampleCount = kVoiceCount;
        h->samples = dt_alloc(sizeof(ZsdSample) * kVoiceCount);
        h->sampleFileCount = kVoiceCount;
        h->sampleFiles = dt_alloc(sizeof(ZsdSampleFileEntry) * kVoiceCount);
        for (j = 0; j < kVoiceCount; j++) {
            ZsdVoice *v = &h->voices[j];
            ZsdSample *s = &h->samples[j];

            v->sample = dt_u32() % 7 == 0 ? 0xffff : (uint16_t)j;
            v->pitch = (uint16_t)dt_range(0x100, 0x4000);
            v->volume = dt_u32() % 6 == 0 ? 0 : (dt_u32() % 5 == 0 ? rand_byte() : (uint8_t)dt_range(1, 127));
            v->pan = dt_u32() % 4 == 0 ? (int8_t)dt_range(-1, 1) : (int8_t)dt_range(-127, 127);
            v->attack = rand_time();
            v->decay = rand_time();
            v->fade = rand_time();
            v->release = rand_time();
            v->sustain = dt_u32() % 5 == 0 ? 0x7f : (uint8_t)dt_range(0, 127);
            v->file = (uint16_t)k;
            gVoices[k][j] = v;
            s->sampleFile = dt_u32() % 7 == 0 ? 0xffff : (uint16_t)j;
            s->format = (uint16_t)(dt_u32() & 6);
            s->rate = (uint32_t)dt_range(8000, 48000);
            s->file = (uint16_t)k;
            h->sampleFiles[j].offset = (uint32_t)dt_range(0, 1000);
            h->sampleFiles[j].size = (uint32_t)dt_range(1, 400000);
        }
    }
}

static ZsdVoice *rand_voice(void)
{
    return gVoices[dt_range(0, kFilesLoaded - 1)][dt_range(0, kVoiceCount - 1)];
}

/* A voice that can be allocated (it has a sample and a sample file). */
static ZsdVoice *playable_voice(void)
{
    int tries;

    for (tries = 0; tries < 100; tries++) {
        ZsdVoice *v = rand_voice();
        const ZsdSample *s;

        if (v->sample == 0xffff) {
            continue;
        }
        s = &gWinloadFiles[v->file].header->samples[v->sample];
        if (s->sampleFile != 0xffff) {
            return v;
        }
    }
    {
        ZsdVoice *v = gVoices[0][0];

        v->sample = 0;
        gWinloadFiles[0].header->samples[0].sampleFile = 0;
        return v;
    }
}

static float rand_float(void)
{
    return dt_float_edgy(-1.0f, 2.0f);
}

/* Mixer state: channels and handles in a consistent random configuration. */
static void make_mixer(void)
{
    int used[kWinchanNumHandles] = {0};
    int c, h;

    make_files();
    memset(&gWinchanChannels, 0, sizeof gWinchanChannels);
    memset(&gWinchanHandles, 0, sizeof gWinchanHandles);
    gWinchanInitialized = 1;
    gWinchanLock = dt_u32() % 8 == 0 ? 1 : 0;
    gWinchanRate = (uint32_t)dt_range(8000, 48000);
    gWinchanNextChannel = dt_range(0, kWinchanNumChannels - 1);
    gWinchanNextHandle = dt_range(0, kWinchanNumHandles - 1);
    gWinchanActive = dt_range(0, 24);
    for (c = 0; c < kWinchanNumChannels; c++) {
        WinchanChannel *ch = &gWinchanChannels[c];

        ch->state = dt_u32() % 3 == 0 ? kWinchanFree : (uint8_t)dt_range(1, 3);
        ch->flags = (uint8_t)(dt_u32() & 3);
        ch->priority = dt_u32() % 4 == 0 ? (uint8_t)dt_range(0, 2) : rand_byte();
        ch->voice = rand_voice();
        ch->sample = &gWinloadFiles[ch->voice->file].header->samples[0];
        ch->length = dt_u32() % 5000;
        ch->step = (uint32_t)dt_range(0, 0x4000);
        ch->position = dt_u32() % 5000;
        ch->fraction = dt_u32() % 4096;
        ch->volume = (uint8_t)dt_range(0, 255);
        ch->pan = (int8_t)dt_range(-127, 127);
        ch->phase = (uint8_t)dt_range(0, 3);
        ch->gainLeft = rand_float();
        ch->gainRight = rand_float();
        ch->phaseFrames = dt_u32() % 100000;
        ch->level = rand_float();
        ch->levelStep = rand_float();
        ch->attackStep = rand_float();
        ch->decayStep = rand_float();
        ch->sustainLevel = rand_float();
        ch->fadeStep = rand_float();
        if (ch->state != kWinchanFree) {
            do {
                h = dt_range(0, kWinchanNumHandles - 1);
            } while (used[h]);
            used[h] = 1;
            ch->handle = (uint8_t)h;
            gWinchanHandles[h].allocated = 1;
            gWinchanHandles[h].channelNum = (uint8_t)c;
        } else {
            ch->handle = rand_byte();
        }
    }
    /* Handles that are allocated without a channel. */
    for (h = 0; h < kWinchanNumHandles; h++) {
        if (!used[h] && dt_u32() % 4 == 0) {
            gWinchanHandles[h].allocated = 1;
            gWinchanHandles[h].channelNum = kWinchanNoChannel;
        }
    }
}

/* A handle that has a channel (there is always one: forced if the random mixer has none). */
static uint8_t live_handle(void)
{
    int n = 0, pick, h;

    for (h = 0; h < kWinchanNumHandles; h++) {
        n += gWinchanHandles[h].allocated && gWinchanHandles[h].channelNum != kWinchanNoChannel;
    }
    if (n == 0) {
        WinchanChannel *ch = &gWinchanChannels[0];

        ch->state = kWinchanPlaying;
        ch->handle = 0;
        gWinchanHandles[0].allocated = 1;
        gWinchanHandles[0].channelNum = 0;
        return 0;
    }
    pick = dt_range(0, n - 1);
    for (h = 0; h < kWinchanNumHandles; h++) {
        if (gWinchanHandles[h].allocated && gWinchanHandles[h].channelNum != kWinchanNoChannel &&
            pick-- == 0) {
            return (uint8_t)h;
        }
    }
    return 0;
}

/* Any allocated handle: with or without a channel. */
static uint8_t used_handle(void)
{
    int h = dt_range(0, kWinchanNumHandles - 1), i;

    for (i = 0; i < kWinchanNumHandles; i++, h = (h + 1) % kWinchanNumHandles) {
        if (gWinchanHandles[h].allocated) {
            return (uint8_t)h;
        }
    }
    return live_handle();
}

static void begin(DtCtx *c)
{
    dt_seed(c);
    dt_zmem_env();
    install_ds();
    make_mixer();
}

/* ---- setups -------------------------------------------------------------------------------------- */

static void setup_none(DtCtx *c)
{
    begin(c);
}

static void setup_slot(DtCtx *c)
{
    begin(c);
    DT_ARG_INT(c, 0, dt_u32());
}

static void setup_callback(DtCtx *c)
{
    begin(c);
    DT_ARG_INT(c, 0, dt_u32());
}

static void setup_channel_of(DtCtx *c)
{
    begin(c);
    DT_ARG_INT(c, 0, garbage_high(used_handle()));
}

/* Handle without an allocation, or out of range: the CRT assert ends the run. */
static void setup_channel_of_bad(DtCtx *c)
{
    int h;

    begin(c);
    if (c->seed % 2 == 0) {
        h = dt_range(64, 255);
    } else {
        for (h = 0; h < kWinchanNumHandles && gWinchanHandles[h].allocated; h++) {
        }
        if (h == kWinchanNumHandles) {
            gWinchanHandles[5].allocated = 0;
            h = 5;
        }
    }
    DT_ARG_INT(c, 0, garbage_high((uint8_t)h));
}

static void setup_channel_of_corrupt(DtCtx *c)
{
    uint8_t live;

    begin(c);
    live = live_handle();
    gWinchanHandles[live].channelNum = (uint8_t)dt_range(24, 0xfe);
    DT_ARG_INT(c, 0, garbage_high(live));
}

static void setup_release(DtCtx *c)
{
    begin(c);
    {
        const uint8_t h = live_handle();
        WinchanChannel *ch = &gWinchanChannels[gWinchanHandles[h].channelNum];

        DT_ARG_PTR(c, 0, ch);
    }
}

static void setup_envelope_step(DtCtx *c)
{
    begin(c);
    DT_ARG_INT(c, 0, c->seed % 4 == 0 ? 0x7f : (int32_t)dt_u32() >> dt_range(0, 31));
    DT_ARG_INT(c, 1, c->seed % 5 == 0 ? 0 : (int32_t)(dt_u32() >> dt_range(0, 31)));
    if (c->seed % 7 == 0) {
        gWinchanRate = 0;
    }
}

static void setup_alloc(DtCtx *c)
{
    int i;

    begin(c);
    if (c->seed % 6 == 0) {
        /* every channel busy: steal or refuse */
        for (i = 0; i < kWinchanNumChannels; i++) {
            if (gWinchanChannels[i].state == kWinchanFree) {
                gWinchanChannels[i].state = kWinchanPlaying;
                gWinchanChannels[i].handle = (uint8_t)(40 + i % 24);
                gWinchanHandles[40 + i % 24].allocated = 1;
                gWinchanHandles[40 + i % 24].channelNum = (uint8_t)i;
            }
        }
    }
    DT_ARG_PTR(c, 0, c->seed % 11 == 4 ? NULL : playable_voice());
    DT_ARG_INT(c, 1, c->seed % 3 == 0 ? (int)dt_u32() : dt_range(0, 255));
    if (c->seed % 6 == 0) {
        /* Priorities right at the lowest one on the mixer: stealing needs priority >= that one. */
        uint8_t lowest = 0xff;

        for (i = 0; i < kWinchanNumChannels; i++) {
            if (gWinchanChannels[i].priority < lowest) {
                lowest = gWinchanChannels[i].priority;
            }
        }
        DT_ARG_INT(c, 1, garbage_high((uint8_t)(lowest + (int)(c->seed / 6 % 3) - 1)));
    }
}

/* Voice without a sample (or whose sample has no file). */
static void setup_alloc_nosample(DtCtx *c)
{
    ZsdVoice *v;

    begin(c);
    v = gVoices[0][1];
    if (c->seed % 2) {
        v->sample = 0xffff;
    } else {
        v->sample = 1;
        gWinloadFiles[0].header->samples[1].sampleFile = 0xffff;
    }
    DT_ARG_PTR(c, 0, v);
    DT_ARG_INT(c, 1, dt_u32());
}

static void setup_play(DtCtx *c)
{
    begin(c);
    gWinchanLock = 0;
    DT_ARG_PTR(c, 0, c->seed % 13 == 3 ? NULL : playable_voice());
    DT_ARG_INT(c, 1, garbage_high((uint32_t)(int16_t)dt_u32() % 5000));
    DT_ARG_INT(c, 2, garbage_high(rand_byte()));
    DT_ARG_INT(c, 3, garbage_high(rand_byte()));
    DT_ARG_INT(c, 4, dt_u32());
}

static void setup_play_uninit(DtCtx *c)
{
    setup_play(c);
    gWinchanInitialized = 0;
}

static void setup_handle(DtCtx *c)
{
    begin(c);
    DT_ARG_INT(c, 0, garbage_high(c->seed % 5 == 0 ? used_handle() : live_handle()));
}

static void setup_handle_uninit(DtCtx *c)
{
    setup_handle(c);
    gWinchanInitialized = 0;
}

static void setup_stop(DtCtx *c)
{
    setup_handle(c);
    DT_ARG_INT(c, 1, garbage_high(rand_byte()));
    if (c->seed % 3 == 0) {
        DT_ARG_INT(c, 1, garbage_high((uint32_t)dt_range(0, 3)));
    }
}

static void setup_handle_byte(DtCtx *c)
{
    setup_handle(c);
    DT_ARG_INT(c, 1, garbage_high(rand_byte()));
}

static void setup_pitch(DtCtx *c)
{
    setup_handle(c);
    DT_ARG_INT(c, 1, garbage_high(c->seed % 6 == 0 ? 0 : (uint32_t)(int16_t)dt_range(-9000, 9000)));
    if (c->seed % 7 == 1) {
        /* a rate that hits the mixer rate exactly */
        gWinchanRate = gWinloadFiles[0].header->samples[0].rate;
    }
}

static void setup_stop_voice(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, rand_voice());
}

static void setup_free_all(DtCtx *c)
{
    begin(c);
    gWinchanHandles[64].allocated = 0;
}

static void setup_free_all_extra(DtCtx *c)
{
    begin(c);
    gWinchanHandles[64].allocated = 1;
}

static void setup_effect(DtCtx *c)
{
    WinchanEffect *e;

    begin(c);
    e = dt_alloc(sizeof *e);
    e->kind = c->seed % 8 == 5 ? dt_range(2, 9) : (int32_t)(c->seed & 1);
    e->level = (int32_t)dt_u32();
    e->millis = c->seed % 4 == 0 ? (uint32_t)dt_range(0, 100) : (uint32_t)dt_range(0, 100000);
    e->param.raw = e->kind == 0 ? dt_u32() : 0;
    if (e->kind == 1) {
        e->param.scale = dt_float_edgy(-100.0f, 100.0f);
    }
    e->tail = dt_float_edgy(-1000.0f, 1000.0f);
    gWinchanEffectMaxFrames = (uint32_t)dt_range(0, 50000);
    DT_ARG_PTR(c, 0, e);
}

static void setup_probe(DtCtx *c)
{
    begin(c);
    gScript.loadFails = c->seed % 8 == 1;
    gScript.procFails = c->seed % 8 == 2;
    gScript.createHr = c->seed % 4 == 3 ? (DsResult)0x8878000a : 0;
    gScript.coopHr = c->seed % 3 == 0 ? (DsResult)0x80004005 : 0;
    gWinchanDirectSound = NULL;
    DT_ARG_PTR(c, 0, c->seed % 5 == 0 ? NULL : (void *)(uintptr_t)(0x1000 + c->seed));
}

static void setup_shutdown(DtCtx *c)
{
    begin(c);
    gBufferA.vtbl = &gBufferVtbl;
    gWinchanBuffer = (DsBuffer *)&gBufferA;
    gWinchanPrimary = (DsBuffer *)&gBufferB;
    gWinchanDirectSound = (DsDevice *)&gDevice;
    gWinchanDsoundModule = (WinHandle)(uintptr_t)0x1234;
    gWinchanRing = dt_alloc(0x100);
    gWinchanHandles[64].allocated = 0;
    gWinchanLock = 0;
}

static void setup_shutdown_uninit(DtCtx *c)
{
    setup_shutdown(c);
    gWinchanInitialized = 0;
}

static void setup_pump(DtCtx *c)
{
    uint32_t total, chunk;

    begin(c);
    gWinchanBuffer = (DsBuffer *)&gBufferA;
    total = 0x2000u + 0x100u * (uint32_t)dt_range(0, 32);
    chunk = 0x100u + 0x40u * (uint32_t)dt_range(0, 20);
    gWinchanRingBytes = total;
    gWinchanChunkBytes = chunk;
    gWinchanRing = dt_alloc(total);
    dt_fill(gWinchanRing, total);
    gWinchanCopyOffset = dt_u32() % total;
    if (c->seed % 4 == 0) {
        gWinchanCopyOffset = total - (uint32_t)dt_range(0, (int32_t)chunk);
    }
    gWinchanWriteOffset = dt_u32() % total;
    gWinchanMixOffset = dt_u32() % total;
    if (c->seed % 5 == 0) {
        gWinchanMixOffset = total - (uint32_t)dt_range(1, 0x400);
    }
    gWinchanLastPlay = dt_u32() % total;
    gScript.play = dt_u32() % total;
    if (c->seed % 3 == 0) {
        /* the play cursor right behind the write offset */
        gScript.play = (gWinchanWriteOffset + total - (uint32_t)dt_range(1, (int32_t)chunk)) % total;
    }
    if (c->seed % 6 == 1) {
        gScript.play = gWinchanLastPlay + (uint32_t)dt_range(0, 3000);
        if (gScript.play >= total) {
            gScript.play = total - 1;
        }
    }
    switch (c->seed % 9) {
    case 2: /* cursor exactly at the edges of the refill window */
        gScript.play = gWinchanWriteOffset + chunk < total ? gWinchanWriteOffset + chunk : gScript.play;
        break;
    case 3:
        gScript.play =
            gWinchanWriteOffset + chunk + 1 < total ? gWinchanWriteOffset + chunk + 1 : gScript.play;
        break;
    case 4:
        gScript.play = gWinchanWriteOffset + chunk >= 1 && gWinchanWriteOffset + chunk - 1 < total
                           ? gWinchanWriteOffset + chunk - 1
                           : gScript.play;
        break;
    case 5:
        gScript.play = gWinchanWriteOffset;
        break;
    case 6:
        gScript.play = gWinchanWriteOffset >= chunk ? gWinchanWriteOffset - chunk : gScript.play;
        break;
    default:
        break;
    }
    gScript.write = dt_u32() % total;
    gScript.status = c->seed % 9 == 4 ? 0 : (uint32_t)(dt_u32() | 1);
    gScript.statusHr = c->seed % 13 == 6 ? (DsResult)0x80004005 : 0;
    gScript.lockHr[0] = c->seed % 5 == 2 ? kDsErrBufferLost : (c->seed % 11 == 7 ? (DsResult)0x80004005 : 0);
    gScript.lockHr[1] = c->seed % 7 == 3 ? (DsResult)0x80004005 : 0;
    gScript.lockCalls = 0;
    gScript.dest = dt_alloc(0x8000);
    gScript.lockBytes1 = c->seed % 4 == 1 ? (uint32_t)dt_range(0, (int32_t)chunk) : chunk;
    gScript.lockBytes2 = (uint32_t)dt_range(0, 4);
    gWinchanPumpCount = dt_range(0, 100);
    if (c->seed % 17 == 8) {
        gWinchanLock = 1;
    } else {
        gWinchanLock = 0;
    }
}

DT_CASES(winchan_cases) = {
    {"winchanCardStub6e0", 0x0046d6e0, winchanCardStub6e0, 0, DT_RET_INT, setup_none, 2, 0},
    {"winchanCardStub700", 0x0046d700, winchanCardStub700, 0, DT_RET_VOID, setup_none, 2, 0},
    {"winchanStart", 0x0046d720, winchanStart, 1, DT_RET_VOID, setup_slot, 2, 0},
    {"winchanCardStub740", 0x0046d740, winchanCardStub740, 0, DT_RET_INT, setup_none, 2, 0},
    {"winchanCardStub760", 0x0046d760, winchanCardStub760, 0, DT_RET_INT, setup_none, 2, 0},
    {"winchanCardStub780", 0x0046d780, winchanCardStub780, 0, DT_RET_INT, setup_none, 2, 0},
    {"winchanSetCallback", 0x0046d7a0, winchanSetCallback, 1, DT_RET_INT, setup_callback, 2, 0},
    {"winchanCardStub7c0", 0x0046d7c0, winchanCardStub7c0, 0, DT_RET_INT, setup_none, 2, 0},
    {"winchanCardStub7e0", 0x0046d7e0, winchanCardStub7e0, 0, DT_RET_INT, setup_none, 2, 0},
    {"winchanCardStub800", 0x0046d800, winchanCardStub800, 0, DT_RET_INT, setup_none, 2, 0},
    {"winchanCardStub820", 0x0046d820, winchanCardStub820, 0, DT_RET_INT, setup_none, 2, 0},
    {"winchanCardStub840", 0x0046d840, winchanCardStub840, 0, DT_RET_INT, setup_none, 2, 0},
    {"winchanCardStub860", 0x0046d860, winchanCardStub860, 0, DT_RET_INT, setup_none, 2, 0},
    {"winchanCardStub880", 0x0046d880, winchanCardStub880, 0, DT_RET_INT, setup_none, 2, 0},
    {"winchanQuery", 0x0046d8a0, winchanQuery, 0, DT_RET_INT, setup_none, 2, 0},
    {"winchanBegin", 0x0046d8c0, winchanBegin, 0, DT_RET_VOID, setup_none, 2, 0},
    {"winchanEnd", 0x0046d8e0, winchanEnd, 0, DT_RET_VOID, setup_none, 2, 0},
    {"winchanCardStub900", 0x0046d900, winchanCardStub900, 0, DT_RET_VOID, setup_none, 2, 0},
    {"winchanCardStub920", 0x0046d920, winchanCardStub920, 0, DT_RET_INT, setup_none, 2, 0},
    {"winchanShutdown", 0x0046df20, winchanShutdown, 0, DT_RET_VOID, setup_shutdown, 200, 0},
    {"winchanShutdown_uninit", 0x0046df20, winchanShutdown, 0, DT_RET_VOID, setup_shutdown_uninit, 4, 0},
    {"winchanPump", 0x0046e010, winchanPump, 0, DT_RET_INT, setup_pump, 1500, 0},
    {"winchanFreeAllChannels", 0x0046e3c0, winchanFreeAllChannels, 0, DT_RET_VOID, setup_free_all, 200, 0},
    {"winchanFreeAllChannels_extra", 0x0046e3c0, winchanFreeAllChannels, 0, DT_RET_VOID, setup_free_all_extra,
     4, 0},
    {"winchanVoiceAlloc", 0x0046e410, winchanVoiceAlloc, 2, DT_RET_INT8, setup_alloc, 1500, 0},
    {"winchanVoiceAlloc_nosample", 0x0046e410, winchanVoiceAlloc, 2, DT_RET_INT8, setup_alloc_nosample, 20,
     0},
    {"winchanVoiceFree", 0x0046e890, winchanVoiceFree, 1, DT_RET_INT, setup_handle, 300, 0},
    {"winchanVoicePlay", 0x0046e8c0, winchanVoicePlay, 5, DT_RET_INT, setup_play, 1000, 0},
    {"winchanVoicePlay_uninit", 0x0046e8c0, winchanVoicePlay, 5, DT_RET_INT, setup_play_uninit, 4, 0},
    {"winchanVoiceStart", 0x0046e9a0, winchanVoiceStart, 1, DT_RET_INT, setup_handle, 600, 0},
    {"winchanVoiceStart_uninit", 0x0046e9a0, winchanVoiceStart, 1, DT_RET_INT, setup_handle_uninit, 4, 0},
    {"winchanVoiceStop", 0x0046eb60, winchanVoiceStop, 2, DT_RET_INT, setup_stop, 1200, 0},
    {"winchanVoiceStop_uninit", 0x0046eb60, winchanVoiceStop, 2, DT_RET_INT, setup_handle_uninit, 4, 0},
    {"winchanVoiceStatus", 0x0046ecd0, winchanVoiceStatus, 1, DT_RET_INT, setup_handle, 200, 0},
    {"winchanVoiceSetVolume", 0x0046ed20, winchanVoiceSetVolume, 2, DT_RET_INT, setup_handle_byte, 600, 0},
    {"winchanVoiceSetVolume_uninit", 0x0046ed20, winchanVoiceSetVolume, 2, DT_RET_INT, setup_handle_uninit, 4,
     0},
    {"winchanVoiceSetPan", 0x0046ee80, winchanVoiceSetPan, 2, DT_RET_INT, setup_handle_byte, 600, 0},
    {"winchanVoiceSetPan_uninit", 0x0046ee80, winchanVoiceSetPan, 2, DT_RET_INT, setup_handle_uninit, 4, 0},
    {"winchanVoiceSetPitch", 0x0046efb0, winchanVoiceSetPitch, 2, DT_RET_INT, setup_pitch, 1200, 0},
    {"winchanVoiceSetPitch_uninit", 0x0046efb0, winchanVoiceSetPitch, 2, DT_RET_INT, setup_handle_uninit, 4,
     0},
    {"winchanVoiceGetVoice", 0x0046f170, winchanVoiceGetVoice, 1, DT_RET_INT, setup_handle, 200, 0},
    {"winchanVoiceGetVoice_uninit", 0x0046f170, winchanVoiceGetVoice, 1, DT_RET_INT, setup_handle_uninit, 4,
     0},
    {"winchanStopVoice", 0x0046f1c0, winchanStopVoice, 1, DT_RET_VOID, setup_stop_voice, 400, 0},
    {"winchanSetEffect", 0x0046f250, winchanSetEffect, 1, DT_RET_INT, setup_effect, 600, 0},
    {"winchanProbeDevice", 0x0046f380, winchanProbeDevice, 1, DT_RET_INT, setup_probe, 100, 0},
    {"winchanChannelOf", 0x0046f470, winchanChannelOf, 1, DT_RET_INT, setup_channel_of, 200, 0},
    {"winchanChannelOf_bad", 0x0046f470, winchanChannelOf, 1, DT_RET_INT, setup_channel_of_bad, 12, 0},
    {"winchanChannelOf_corrupt", 0x0046f470, winchanChannelOf, 1, DT_RET_INT, setup_channel_of_corrupt, 6, 0},
    {"winchanChannelRelease", 0x0046f530, winchanChannelRelease, 1, DT_RET_VOID, setup_release, 100, 0},
    {"winchanEnvelopeStep", 0x0046f580, winchanEnvelopeStep, 2, DT_RET_DOUBLE, setup_envelope_step, 600, 0},
};
