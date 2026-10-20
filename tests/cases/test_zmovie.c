/*
 * Tests for the AVI movie player (zmovie.c). The VFW thunks, the waveOut / user32 imports and the
 * DirectDraw object are all stubbed: every call is logged by value, results are derived from the stream
 * handle and the iteration. The movie structure is built from random but self-consistent fixtures.
 */
#include <stddef.h>

#include "dt.h"
#include "dt_zmem.h"

#include "compat/crt.h"
#include "zge/zmovie.h"

#define LOG_MAX 1024u
#define TICKS   100000u

static uint32_t gLog[LOG_MAX];
static uint32_t gLogN;

/* Stream fixtures: handle = 0x1000 + index. */
static uint32_t gType[16];
static int32_t gSampSize[16];
static int32_t gStart[16];
static int32_t gLen[16];
static int32_t gNumStreams;
static int32_t gOpenFail;
static int32_t gFrameOpenFail;
static int32_t gFrameFail;
static int32_t gReadFail;
static int32_t gWaveOpenFails;
static int32_t gPrepareFailAt;
static int32_t gPrepareCalls;
static int32_t gWriteFail;
static uint32_t gDummyVtable[64];
static uint32_t *gFakeObject;

static void logU(uint32_t v)
{
    if (gLogN < LOG_MAX) {
        gLog[gLogN] = v;
    }
    gLogN++;
}

static void logP(const void *p)
{
    logU(dt_canon((uint32_t)(uintptr_t)p));
}

static uint32_t idx(const void *stream)
{
    return ((uint32_t)(uintptr_t)stream - 0x1000u) & 15u;
}

/* ---- VFW thunk stubs --------------------------------------------------------------------------- */

static int32_t BMX_STDCALL s_fileInit(void)
{
    logU(1);
    return 0;
}

static int32_t BMX_STDCALL s_fileExit(void)
{
    logU(2);
    return 0;
}

static int32_t BMX_STDCALL s_readFormat(void *stream, int32_t pos, void *format, int32_t *size)
{
    logU(3);
    logU(idx(stream));
    logU((uint32_t)pos);
    logP(format);
    if (format == NULL) {
        *size = (int32_t)(16 + idx(stream));
        return 0;
    }
    logU((uint32_t)*size);
    for (int i = 0; i < *size; i++) {
        ((uint8_t *)format)[i] = (uint8_t)(i * 7 + idx(stream) * 3 + 1);
    }
    return 0;
}

static int32_t BMX_STDCALL s_info(void *stream, void *info, int32_t size)
{
    uint8_t *p = info;
    const uint32_t i = idx(stream);

    logU(4);
    logU(i);
    logU((uint32_t)size);
    memset(p, 0, 0x8c);
    memcpy(p, &gType[i], 4);
    memcpy(p + 0x30, &gSampSize[i], 4);
    return 0;
}

static int32_t BMX_STDCALL s_read(void *stream, int32_t start, int32_t samples, void *buffer,
                                  int32_t bufferBytes, uint32_t *bytes, int32_t *samplesRead)
{
    const uint32_t i = idx(stream);

    logU(5);
    logU(i);
    logU((uint32_t)start);
    logU((uint32_t)samples);
    logP(buffer);
    logU((uint32_t)bufferBytes);
    *bytes = (uint32_t)(samples * gSampSize[i]);
    *samplesRead = gReadFail ? samples - 1 : samples;
    memset(buffer, (int)(start & 0xff), 8);
    return 0;
}

static int32_t BMX_STDCALL s_start(void *stream)
{
    return gStart[idx(stream)];
}

static int32_t BMX_STDCALL s_length(void *stream)
{
    return gLen[idx(stream)];
}

static int32_t BMX_STDCALL s_timeToSample(void *stream, int32_t time)
{
    return time / (int32_t)(idx(stream) % 3 + 2);
}

static int32_t BMX_STDCALL s_sampleToTime(void *stream, int32_t sample)
{
    return sample * (int32_t)(idx(stream) % 3 + 2);
}

static void *BMX_STDCALL s_drawDibOpen(void)
{
    logU(6);
    return (void *)0x2000;
}

static void *BMX_STDCALL s_frameOpen(void *stream, void *format)
{
    logU(7);
    logU(idx(stream));
    logP(format);
    return gFrameOpenFail ? NULL : (void *)(0x3000u + idx(stream));
}

static uint32_t BMX_STDCALL s_fileRelease(void *file)
{
    logU(8);
    logP(file);
    return 0;
}

static int32_t BMX_STDCALL s_fileGetStream(void *file, void **stream, uint32_t type, int32_t index)
{
    logU(9);
    logP(file);
    logU(type);
    logU((uint32_t)index);
    if (index >= gNumStreams) {
        return 1;
    }
    *stream = (void *)(0x1000u + (uint32_t)index);
    return 0;
}

static int32_t BMX_STDCALL s_fileOpen(void **file, const char *path, uint32_t mode, void *handler)
{
    logU(10);
    logU((uint32_t)strlen(path));
    logU(mode);
    logP(handler);
    *file = (void *)0x4000;
    return gOpenFail;
}

static uint32_t BMX_STDCALL s_streamRelease(void *stream)
{
    logU(11);
    logU(idx(stream));
    return 0;
}

static int32_t BMX_STDCALL s_saveOptionsFree(int32_t count, void *options)
{
    logU(12);
    logU((uint32_t)count);
    logP(options);
    return 0;
}

static int32_t BMX_STDCALL s_drawDibClose(void *drawDib)
{
    logU(13);
    logP(drawDib);
    return 0;
}

static int32_t BMX_STDCALL s_frameClose(void *getFrame)
{
    logU(14);
    logP(getFrame);
    return 0;
}

static int32_t BMX_STDCALL s_drawDibDraw(void *drawDib, void *hdc, int32_t x, int32_t y, int32_t w, int32_t h,
                                         void *bi, void *bits, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
                                         uint32_t flags)
{
    logU(15);
    logP(drawDib);
    logP(hdc);
    logU((uint32_t)x);
    logU((uint32_t)y);
    logU((uint32_t)w);
    logU((uint32_t)h);
    logP(bi);
    logP(bits);
    logU((uint32_t)sx);
    logU((uint32_t)sy);
    logU((uint32_t)sw);
    logU((uint32_t)sh);
    logU(flags);
    return 1;
}

static void *BMX_STDCALL s_getFrame(void *getFrame, int32_t position)
{
    logU(16);
    logP(getFrame);
    logU((uint32_t)position);
    return gFrameFail ? NULL : (void *)0x5000;
}

/* ---- waveOut, user32 and DirectDraw (import table / COM slots) ------------------------------------ */

static uint32_t BMX_STDCALL s_waveOpen(uint32_t *hwo, int32_t device, const void *format, uint32_t callback,
                                       uint32_t instance, uint32_t flags)
{
    logU(20);
    logU((uint32_t)device);
    logP(format);
    logU(callback);
    logU(instance);
    logU(flags);
    if (gWaveOpenFails > 0) {
        gWaveOpenFails--;
        return 1;
    }
    *hwo = 0x77;
    return 0;
}

static uint32_t BMX_STDCALL s_waveWrite(uint32_t hwo, ZMovieWaveHeader *h, uint32_t size)
{
    logU(21);
    logU(hwo);
    logP(h);
    logU(size);
    logU(h->length);
    return gWriteFail ? 1 : 0;
}

static uint32_t BMX_STDCALL s_wavePrepare(uint32_t hwo, ZMovieWaveHeader *h, uint32_t size)
{
    logU(22);
    logU(hwo);
    logP(h);
    logU(size);
    logU(h->flags);
    logU(h->length);
    logP(h->data);
    return ++gPrepareCalls == gPrepareFailAt ? 1 : 0;
}

static uint32_t BMX_STDCALL s_waveUnprepare(uint32_t hwo, ZMovieWaveHeader *h, uint32_t size)
{
    logU(23);
    logU(hwo);
    logP(h);
    logU(size);
    return 0;
}

static uint32_t BMX_STDCALL s_waveHandle(uint32_t hwo)
{
    logU(24);
    logU(hwo);
    return 0;
}

static uint32_t BMX_STDCALL s_waveReset(uint32_t hwo)
{
    logU(25);
    logU(hwo);
    return 0;
}

static uint32_t BMX_STDCALL s_wavePause(uint32_t hwo)
{
    logU(26);
    logU(hwo);
    return 0;
}

static uint32_t BMX_STDCALL s_waveRestart(uint32_t hwo)
{
    logU(27);
    logU(hwo);
    return 0;
}

static uint32_t BMX_STDCALL s_sndPlay(const char *name, uint32_t flags)
{
    logU(28);
    logP(name);
    logU(flags);
    return 1;
}

static uint32_t BMX_STDCALL s_tick(void)
{
    return TICKS;
}

static int32_t BMX_STDCALL s_clientRect(void *window, ZMovieRect *rect)
{
    logU(30);
    logP(window);
    rect->left = 1;
    rect->top = 2;
    rect->right = 641;
    rect->bottom = 482;
    return 1;
}

static void *BMX_STDCALL s_getDc(void *window)
{
    logU(31);
    logP(window);
    return (void *)0x6000;
}

static int32_t BMX_STDCALL s_releaseDc(void *window, void *hdc)
{
    logU(32);
    logP(window);
    logP(hdc);
    return 1;
}

static int32_t BMX_STDCALL s_getGdiSurface(void *self, void *out)
{
    logU(33);
    logP(self);
    *(void **)out = gFakeObject;
    return 0;
}

static int32_t BMX_STDCALL s_release(void *self)
{
    logU(34);
    logP(self);
    return 0;
}

/* Exe import-table slots and the DirectDraw globals. */
#define IAT(addr)  (*(uint32_t *)(uintptr_t)(addr))
#define gDdObject  (*(void **)(uintptr_t)0x0069730c)
#define gDdSurface (*(void **)(uintptr_t)0x006977cc)

static void install_stubs(void)
{
    dt_stub(0x004af9d0, NULL, (void *)s_fileInit);
    dt_stub(0x004af9d6, NULL, (void *)s_fileExit);
    dt_stub(0x004af9dc, NULL, (void *)s_readFormat);
    dt_stub(0x004af9e2, NULL, (void *)s_info);
    dt_stub(0x004af9e8, NULL, (void *)s_read);
    dt_stub(0x004af9ee, NULL, (void *)s_start);
    dt_stub(0x004af9f4, NULL, (void *)s_length);
    dt_stub(0x004af9fa, NULL, (void *)s_timeToSample);
    dt_stub(0x004afa00, NULL, (void *)s_drawDibOpen);
    dt_stub(0x004afa06, NULL, (void *)s_frameOpen);
    dt_stub(0x004afa0c, NULL, (void *)s_sampleToTime);
    dt_stub(0x004afa12, NULL, (void *)s_fileRelease);
    dt_stub(0x004afa18, NULL, (void *)s_fileGetStream);
    dt_stub(0x004afa24, NULL, (void *)s_fileOpen);
    dt_stub(0x004afa2a, NULL, (void *)s_streamRelease);
    dt_stub(0x004afa30, NULL, (void *)s_saveOptionsFree);
    dt_stub(0x004afa36, NULL, (void *)s_drawDibClose);
    dt_stub(0x004afa3c, NULL, (void *)s_frameClose);
    dt_stub(0x004afa42, NULL, (void *)s_drawDibDraw);
    dt_stub(0x004afa48, NULL, (void *)s_getFrame);
    IAT(0x0069980c) = (uint32_t)(uintptr_t)s_waveOpen;
    IAT(0x00699808) = (uint32_t)(uintptr_t)s_waveWrite;
    IAT(0x0069983c) = (uint32_t)(uintptr_t)s_wavePrepare;
    IAT(0x0069982c) = (uint32_t)(uintptr_t)s_waveUnprepare;
    IAT(0x00699830) = (uint32_t)(uintptr_t)s_waveHandle;
    IAT(0x00699828) = (uint32_t)(uintptr_t)s_waveReset;
    IAT(0x00699820) = (uint32_t)(uintptr_t)s_wavePause;
    IAT(0x0069981c) = (uint32_t)(uintptr_t)s_waveRestart;
    IAT(0x00699818) = (uint32_t)(uintptr_t)s_sndPlay;
    IAT(0x006995fc) = (uint32_t)(uintptr_t)s_tick;
    IAT(0x00699798) = (uint32_t)(uintptr_t)s_clientRect;
    IAT(0x0069979c) = (uint32_t)(uintptr_t)s_releaseDc;
    IAT(0x006997a0) = (uint32_t)(uintptr_t)s_getDc;
}

/* ---- fixtures -------------------------------------------------------------------------------------- */

static void begin(DtCtx *c)
{
    dt_seed(c);
    dt_zmem_env();
    gLogN = 0;
    gNumStreams = dt_range(0, 5);
    gOpenFail = c->seed % 9 == 4;
    gFrameOpenFail = c->seed % 5 == 2;
    gFrameFail = c->seed % 7 == 3;
    gReadFail = c->seed % 11 == 6;
    gWaveOpenFails = c->seed % 6 == 1 ? 1 + (int32_t)(c->seed / 6) % 2 : 0;
    gPrepareFailAt = c->seed % 4 == 3 ? dt_range(1, 4) : 0;
    gPrepareCalls = 0;
    gWriteFail = c->seed % 13 == 8;
    for (int i = 0; i < 16; i++) {
        gType[i] = dt_u32() % 2 ? (uint32_t)kZmovieStreamVideo : (uint32_t)kZmovieStreamAudio;
        if (dt_u32() % 8 == 0) {
            gType[i] = dt_u32();
        }
        gSampSize[i] = dt_u32() % 6 == 0 ? dt_range(-2, 0x9000) : dt_range(1, 8);
        gStart[i] = dt_range(0, 50);
        gLen[i] = dt_range(0, 2000);
    }
    gFakeObject = dt_alloc(16);
    gDummyVtable[0x90 / 4] = (uint32_t)(uintptr_t)s_getGdiSurface;
    gDummyVtable[0x28 / 4] = (uint32_t)(uintptr_t)s_release;
    gFakeObject[0] = (uint32_t)(uintptr_t)gDummyVtable;
    gDdObject = gFakeObject;
    gDdSurface = gFakeObject;
    install_stubs();
}

static ZMovie *make_movie(DtCtx *c)
{
    ZMovie *m = dt_alloc(sizeof *m);
    const int n = (int)dt_range(0, 4);

    dt_fill(m, sizeof *m);
    m->streamCount = n;
    for (int i = 0; i < kZmovieMaxStreams; i++) {
        m->stream[i] = (void *)(0x1000u + (uint32_t)i);
        m->infoPtr[i] = &m->info[i];
        memset(&m->info[i], 0, sizeof m->info[i]);
        m->info[i].format = dt_u32() % 3 == 0 ? dt_zmemalloc(16) : NULL;
        m->getFrame[i] = dt_u32() % 2 ? (void *)(0x3000u + (uint32_t)i) : NULL;
        m->drawDib[i] = dt_u32() % 2 ? (void *)0x2000 : NULL;
    }
    m->videoStream = dt_u32() % 3 == 0 ? -1 : dt_range(0, 3);
    m->audioStream = dt_u32() % 3 == 0 ? -1 : dt_range(0, 3);
    m->startTime = (uint32_t)dt_range(0, 100);
    m->endTime = m->startTime + (uint32_t)dt_range(0, 5000);
    m->length = dt_range(1, 3000);
    m->position = dt_range(0, 3000);
    m->playing = c->seed % 3 == 0;
    m->startTick = TICKS - (uint32_t)dt_range(0, 2 * m->length);
    m->window = (void *)0x1234;
    m->rect.left = dt_range(0, 30);
    m->rect.top = dt_range(0, 30);
    m->rect.right = m->rect.left + dt_range(0, 400);
    m->rect.bottom = m->rect.top + dt_range(0, 300);
    m->loop = dt_u32() % 2;
    m->useDirectDraw = dt_u32() % 2;
    m->waveFormat = dt_zmemalloc(18);
    m->sampleSize = dt_range(1, 4);
    m->waveOut = dt_u32() % 3 ? 0x77u : 0u;
    m->bufferCount = (uint16_t)dt_range(0, 4);
    m->queued = (uint16_t)dt_range(0, m->bufferCount);
    m->head = (uint16_t)(m->bufferCount ? dt_range(0, m->bufferCount - 1) : 0);
    for (int i = 0; i < kZmovieMaxBuffers; i++) {
        m->buffers[i] = NULL;
    }
    for (int i = 0; i < m->bufferCount; i++) {
        m->buffers[i] = dt_zmemalloc(0x20 + 0x40);
        memset(m->buffers[i], 0, 0x20);
        m->buffers[i]->data = (uint8_t *)m->buffers[i] + 0x20;
        m->buffers[i]->length = 0x40;
    }
    m->audioStart = dt_range(0, 100);
    m->audioPos = m->audioStart + dt_range(0, 400);
    m->audioEnd = dt_u32() % 3 == 0 ? m->audioPos - dt_range(0, 3) : m->audioPos + dt_range(0, 100000);
    m->audioOn = dt_u32() % 4 != 0;
    m->audioEnabled = dt_u32() % 5 != 0;
    return m;
}

static void setup_init(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, make_movie(c));
}

static void setup_cb(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, make_movie(c));
    DT_ARG_INT(c, 1, 0x9000 + c->seed);
}

static void setup_open(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, make_movie(c));
    DT_ARG_PTR(c, 1, dt_rand_str(20, 0));
}

static void setup_play(DtCtx *c)
{
    ZMovieRect *r = dt_alloc(sizeof *r);

    begin(c);
    DT_ARG_PTR(c, 0, make_movie(c));
    DT_ARG_PTR(c, 1, (void *)0x1234);
    DT_ARG_INT(c, 2, dt_u32() % 2);
    DT_ARG_INT(c, 3, dt_u32() % 2);
    r->left = dt_range(0, 20);
    r->top = dt_range(0, 20);
    r->right = r->left + dt_range(1, 640);
    r->bottom = r->top + dt_range(1, 480);
    DT_ARG_PTR(c, 4, dt_u32() % 3 ? r : NULL);
}

static void setup_update(DtCtx *c)
{
    ZMovieRect *r = dt_alloc(sizeof *r);

    begin(c);
    DT_ARG_PTR(c, 0, make_movie(c));
    DT_ARG_PTR(c, 1, dt_u32() % 2 ? (void *)0x7000 : NULL);
    r->left = dt_range(0, 20);
    r->top = dt_range(0, 20);
    r->right = r->left + dt_range(1, 640);
    r->bottom = r->top + dt_range(1, 480);
    DT_ARG_PTR(c, 2, dt_u32() % 2 ? r : NULL);
}

static void setup_draw(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, make_movie(c));
    DT_ARG_PTR(c, 1, (void *)0x7000);
    DT_ARG_INT(c, 2, dt_range(0, 20));
    DT_ARG_INT(c, 3, dt_range(0, 20));
    DT_ARG_INT(c, 4, dt_range(100, 640));
    DT_ARG_INT(c, 5, dt_range(100, 480));
    DT_ARG_INT(c, 6, dt_u32() % 5 == 0 ? -1 : dt_range(0, 12000));
}

DT_CASES(zmovie_cases) = {
    {"zmovieInit", 0x0049fb30, zmovieInit, 1, 0, setup_init, 100, 0},
    {"zmovieShutdown", 0x0049fbe0, zmovieShutdown, 1, 0, setup_init, 300, 0},
    {"zmovieOpenAudio", 0x0049fcf0, zmovieOpenAudio, 2, DT_RET_INT, setup_cb, 500, 0},
    {"zmovieFillAudio", 0x004a0040, zmovieFillAudio, 1, DT_RET_INT, setup_init, 500, 0},
    {"zmovieStartAudio", 0x004a0270, zmovieStartAudio, 2, DT_RET_INT, setup_cb, 500, 0},
    {"zmovieScanStreams", 0x004a0400, zmovieScanStreams, 1, 0, setup_init, 500, 0},
    {"zmovieOpen", 0x004a06f0, zmovieOpen, 2, DT_RET_INT, setup_open, 300, 0},
    {"zmovieClose", 0x004a0780, zmovieClose, 1, 0, setup_init, 300, 0},
    {"zmoviePlay", 0x004a09d0, zmoviePlay, 5, 0, setup_play, 300, 0},
    {"zmovieRefillAudio", 0x004a0a70, zmovieRefillAudio, 1, 0, setup_init, 300, 0},
    {"zmovieUpdate", 0x004a0a90, zmovieUpdate, 3, 0, setup_update, 500, 0},
    {"zmovieStop", 0x004a0cb0, zmovieStop, 1, 0, setup_init, 200, 0},
    {"zmovieDraw", 0x004a0db0, zmovieDraw, 7, 0, setup_draw, 500, 0},
    {"zmovieIsPlaying", 0x004a1050, zmovieIsPlaying, 1, DT_RET_INT, setup_init, 50, 0},
};
