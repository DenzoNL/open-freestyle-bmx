/*
 * Tests for the stream player (winstrm.c). DirectSound is replaced by fake COM buffers (one per stream)
 * whose Lock hands out a heap region, whose cursor comes from a script, and which log every call. The file
 * layer (CreateFile, SetFilePointer, ReadFile, CloseHandle) and the critical section are fake imports that
 * log their arguments; ReadFile serves a pattern derived from the handle and file position (sometimes a
 * short read), so the ADPCM decoder gets varied input. The CRT assert is stubbed (it logs and the function
 * carries on, which is safe with these fixtures).
 *
 * Not covered: real DirectSound timing, and winstrmDestroy / winstrmShutdown on a stream whose buffer was
 * never created (the original dereferences it).
 */
#include <stddef.h>

#include "dt.h"
#include "dt_zmem.h"

#include "bmx/pc/zge/sound/winstrm.h"
#include "compat/crt.h"

/* ---- stub log ------------------------------------------------------------------------------------ */

static uint32_t gLog[4096];
static uint32_t gLogN;

static void logU(uint32_t v)
{
    if (gLogN < sizeof gLog / sizeof gLog[0]) {
        gLog[gLogN] = v;
    }
    gLogN++;
}

static uint32_t garbage_high(uint32_t low)
{
    return low | (dt_u32() & 0xffffff00u);
}

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

/* ---- fake DirectSound buffers -------------------------------------------------------------------- */

typedef struct FakeBuffer {
    const DsBufferVtbl *vtbl;
    uint32_t id;
} FakeBuffer;

static struct {
    uint32_t play[8];
    uint8_t *dest[8];
    uint32_t destBytes[8];
    DsResult lockHr[2];
    uint32_t lockCalls;
    uint32_t bytes2;
    DsResult otherHr; /* result of volume, pan, stop, play, ... */
    uint32_t createNext;
    DsResult createHr;
    uint32_t shortReads; /* every n-th ReadFile returns fewer bytes */
    uint32_t readCalls;
    uint32_t createFails; /* CreateFile fails when the name starts with 'x' */
    uint32_t seekOffset;  /* SetFilePointer returns this much too much when nonzero */
} gScript;

static FakeBuffer gBuffers[8];

#define SELF(p) (((FakeBuffer *)(p))->id)

static uint32_t BMX_STDCALL fb_release(DsBuffer *self)
{
    logU(0x201);
    logU(SELF(self));
    return (uint32_t)gScript.otherHr;
}

static DsResult BMX_STDCALL fb_getCurrentPosition(DsBuffer *self, uint32_t *play, uint32_t *write)
{
    logU(0x202);
    logU(SELF(self));
    *play = gScript.play[SELF(self)];
    *write = 0x1234;
    return gScript.otherHr == 7 ? 7 : 0;
}

static DsResult BMX_STDCALL fb_lock(DsBuffer *self, uint32_t offset, uint32_t bytes, void **ptr1,
                                    uint32_t *bytes1, void **ptr2, uint32_t *bytes2, uint32_t flags)
{
    const uint32_t id = SELF(self);
    const DsResult hr = gScript.lockHr[gScript.lockCalls > 1 ? 1 : gScript.lockCalls];

    logU(0x204);
    logU(id);
    logU(offset);
    logU(bytes);
    logU(flags);
    gScript.lockCalls++;
    /* Also on failure: the original carries on after its assert and writes through the pointer. */
    *ptr1 = gScript.dest[id] + (offset + bytes <= gScript.destBytes[id] ? offset : 0);
    *bytes1 = bytes;
    *ptr2 = 0;
    *bytes2 = gScript.bytes2;
    return hr;
}

static DsResult BMX_STDCALL fb_play(DsBuffer *self, uint32_t reserved, uint32_t priority, uint32_t flags)
{
    logU(0x205);
    logU(SELF(self));
    logU(reserved);
    logU(priority);
    logU(flags);
    return gScript.otherHr;
}

static DsResult BMX_STDCALL fb_setCurrentPosition(DsBuffer *self, uint32_t position)
{
    logU(0x206);
    logU(SELF(self));
    logU(position);
    return gScript.otherHr;
}

static DsResult BMX_STDCALL fb_setVolume(DsBuffer *self, int32_t volume)
{
    logU(0x207);
    logU(SELF(self));
    logU((uint32_t)volume);
    return gScript.otherHr;
}

static DsResult BMX_STDCALL fb_setPan(DsBuffer *self, int32_t pan)
{
    logU(0x208);
    logU(SELF(self));
    logU((uint32_t)pan);
    return gScript.otherHr;
}

static DsResult BMX_STDCALL fb_stop(DsBuffer *self)
{
    logU(0x209);
    logU(SELF(self));
    return gScript.otherHr;
}

static DsResult BMX_STDCALL fb_unlock(DsBuffer *self, void *ptr1, uint32_t bytes1, void *ptr2,
                                      uint32_t bytes2)
{
    const uint32_t id = SELF(self);

    logU(0x20a);
    logU(id);
    logU((uint32_t)((uint8_t *)ptr1 - gScript.dest[id]));
    logU(bytes1);
    logU((uint32_t)(uintptr_t)ptr2);
    logU(bytes2);
    return 0;
}

static DsResult BMX_STDCALL fb_restore(DsBuffer *self)
{
    logU(0x20b);
    logU(SELF(self));
    return 0;
}

static const DsBufferVtbl gBufferVtbl = {
    .release = fb_release,
    .getCurrentPosition = fb_getCurrentPosition,
    .lock = fb_lock,
    .play = fb_play,
    .setCurrentPosition = fb_setCurrentPosition,
    .setVolume = fb_setVolume,
    .setPan = fb_setPan,
    .stop = fb_stop,
    .unlock = fb_unlock,
    .restore = fb_restore,
};

typedef struct FakeDevice {
    const DsDeviceVtbl *vtbl;
} FakeDevice;

static DsResult BMX_STDCALL fd_createSoundBuffer(DsDevice *self, const DsBufferDesc *desc, DsBuffer **buffer,
                                                 void *outer)
{
    logU(0x211);
    logU(desc->size);
    logU(desc->flags);
    logU(desc->bufferBytes);
    logU(desc->reserved);
    logU(desc->format->formatTag);
    logU(desc->format->channels);
    logU(desc->format->samplesPerSec);
    logU(desc->format->avgBytesPerSec);
    logU(desc->format->blockAlign);
    logU(desc->format->bitsPerSample);
    logU((uint32_t)(uintptr_t)outer);
    {
        /* Also on failure: the original carries on after its assert and uses the buffer. */
        const uint32_t id = gScript.createNext++ & 7;

        *buffer = (DsBuffer *)&gBuffers[id];
        gScript.destBytes[id] = desc->bufferBytes + 64;
        gScript.dest[id] = dt_alloc(desc->bufferBytes + 64);
    }
    return gScript.createHr;
}

static const DsDeviceVtbl gDeviceVtbl = {.createSoundBuffer = fd_createSoundBuffer};
static FakeDevice gDevice = {&gDeviceVtbl};

/* ---- fake file layer ----------------------------------------------------------------------------- */

static uint32_t gFilePos[16];

static WinHandle BMX_STDCALL f_createFile(const char *name, uint32_t access, uint32_t share, void *security,
                                          uint32_t disposition, uint32_t flags, void *templateFile)
{
    logU(0x221);
    logU((uint32_t)(uint8_t)name[0]);
    logU(access);
    logU(share);
    logU((uint32_t)(uintptr_t)security);
    logU(disposition);
    logU(flags);
    logU((uint32_t)(uintptr_t)templateFile);
    if (gScript.createFails && name[0] == 'x') {
        return kWinInvalidHandle;
    }
    return (WinHandle)(uintptr_t)(0x500 + ((name[1] + flags) & 7));
}

static uint32_t BMX_STDCALL f_setFilePointer(WinHandle file, int32_t distance, int32_t *high, uint32_t method)
{
    logU(0x222);
    logU((uint32_t)(uintptr_t)file);
    logU((uint32_t)distance);
    logU((uint32_t)(uintptr_t)high);
    logU(method);
    gFilePos[(uintptr_t)file & 7] = (uint32_t)distance;
    return (uint32_t)distance + gScript.seekOffset;
}

static int32_t BMX_STDCALL f_readFile(WinHandle file, void *buffer, uint32_t bytes, uint32_t *read, void *ov)
{
    const uint32_t h = (uint32_t)(uintptr_t)file & 7;
    uint32_t n = bytes, i;
    uint8_t *p = buffer;

    gScript.readCalls++;
    if (gScript.shortReads != 0 && gScript.readCalls % gScript.shortReads == 0) {
        n = bytes / 3;
    }
    for (i = 0; i < n; i++) {
        uint32_t x = (gFilePos[h] + i) * 2654435761u + h * 40503u;

        p[i] = (uint8_t)(x >> 13);
    }
    gFilePos[h] += n;
    logU(0x223);
    logU((uint32_t)(uintptr_t)file);
    logU(bytes);
    logU((uint32_t)(uintptr_t)ov);
    *read = n;
    return 1;
}

static int32_t BMX_STDCALL f_closeHandle(WinHandle file)
{
    logU(0x224);
    logU((uint32_t)(uintptr_t)file);
    return 1;
}

static void BMX_STDCALL f_enter(void *section)
{
    logU(0x231);
    logU(section == (void *)&gWinstrmLock);
}

static void BMX_STDCALL f_leave(void *section)
{
    logU(0x232);
    logU(section == (void *)&gWinstrmLock);
}

static void BMX_STDCALL f_init(void *section)
{
    logU(0x233);
    logU(section == (void *)&gWinstrmLock);
}

static void BMX_STDCALL f_delete(void *section)
{
    logU(0x234);
    logU(section == (void *)&gWinstrmLock);
}

static void install(void)
{
    int i;

    gLogN = 0;
    memset(&gScript, 0, sizeof gScript);
    dt_stub(0x004a4de8, NULL, (void *)stub_assert);
    gWinCreateFile = f_createFile;
    gWinSetFilePointer = f_setFilePointer;
    gWinReadFile = f_readFile;
    gWinCloseHandle = f_closeHandle;
    gWinEnterCriticalSection = f_enter;
    gWinLeaveCriticalSection = f_leave;
    gWinInitCriticalSection = f_init;
    gWinDeleteCriticalSection = f_delete;
    for (i = 0; i < 8; i++) {
        gBuffers[i].vtbl = &gBufferVtbl;
        gBuffers[i].id = (uint32_t)i;
        gFilePos[i] = 0;
    }
    gWinstrmDirectSound = (DsDevice *)&gDevice;
    gWinstrmTicksPerSec = (uint32_t)dt_range(30, 120);
}

/* ---- fixtures ------------------------------------------------------------------------------------ */

static const char *const kNames[] = {"aone.zsd", "btwo.zsd", "cthree.zsd", "dfour.zsd", "xbad.zsd", "e5.zsd"};

static WinloadSampleFile *rand_file(void)
{
    WinloadSampleFile *f = dt_alloc(sizeof *f);

    f->name = kNames[dt_u32() % 4];
    f->offset = dt_u32() % 3 == 0 ? 0 : (uint32_t)dt_range(1, 5000);
    f->size = (uint32_t)dt_range(1, 120000);
    return f;
}

static uint32_t rand_format_flags(void)
{
    static const uint32_t rates[] = {0, kWinstrmFmtRate11k, kWinstrmFmtRate16k, kWinstrmFmtRate44k,
                                     kWinstrmFmtRate44k | kWinstrmFmtRate16k};
    uint32_t f = rates[dt_u32() % 5];

    if (dt_u32() & 1) {
        f |= kWinstrmFmtStereo;
    }
    if (dt_u32() % 3 == 0) {
        f |= kWinstrmFmt8Bit;
    }
    if (dt_u32() % 3 == 0) {
        f |= kWinstrmFmtPcm;
    }
    return f;
}

/* Source in the format that winstrmOpenSource derives from `flags`. */
static void set_source_format(WinstrmSource *s, uint32_t flags)
{
    s->channels = (uint8_t)(((flags & kWinstrmFmtStereo) != 0) + 1);
    s->rate = (flags & kWinstrmFmtRate44k) != 0   ? 44100
              : (flags & kWinstrmFmtRate16k) != 0 ? 16000
              : (flags & kWinstrmFmtRate11k) != 0 ? 11025
                                                  : 22050;
    s->bits = (flags & kWinstrmFmt8Bit) != 0 ? 8 : 16;
    s->flags = 0;
    if ((flags & kWinstrmFmtPcm) == 0) {
        s->channels = 1;
        s->flags = kWinstrmSourceAdpcm;
    }
}

static WinstrmSource *make_source(uint32_t formatFlags)
{
    int tries;

    for (tries = 0; tries < 64; tries++) {
        WinstrmSource *s = &gWinstrmSources[dt_range(0, kWinstrmMaxSources - 1)];

        if (s->size == 0) {
            s->file = (WinHandle)(uintptr_t)(0x500 + dt_range(0, 7));
            s->size = (uint32_t)dt_range(1, 60000);
            set_source_format(s, formatFlags);
            s->startOffset = dt_u32() % 3 == 0 ? 0 : (uint32_t)dt_range(1, 5000);
            s->position = dt_u32() % 4 == 0 ? s->size : (uint32_t)dt_range(0, (int32_t)s->size);
            s->predictor = dt_range(-32768, 32767);
            s->step = (uint32_t)dt_range(7, 32767);
            s->index = dt_range(0, 88);
            return s;
        }
    }
    return &gWinstrmSources[0];
}

/* A chain node from the pool with a random source of the format. */
static WinstrmNode *make_node(uint32_t formatFlags)
{
    int tries;

    for (tries = 0; tries < 64; tries++) {
        WinstrmNode *n = &gWinstrmNodes[dt_range(0, kWinstrmMaxNodes - 1)];

        if (n->repeat == 0) {
            n->repeat = (uint32_t)dt_range(1, 3);
            n->source = make_source(formatFlags);
            n->next = 0;
            return n;
        }
    }
    return 0;
}

/* Streams: `count` random live ones. All use format `fmt`. */
static void make_world(int count, uint32_t fmt)
{
    int used[kWinstrmMaxStreams] = {0};
    int c;

    memset(&gWinstrmStreams, 0, sizeof gWinstrmStreams);
    memset(&gWinstrmSources, 0, sizeof gWinstrmSources);
    memset(&gWinstrmNodes, 0, sizeof gWinstrmNodes);
    gWinstrmInitialized = 1;
    gWinstrmPaused = dt_u32() % 9 == 0 ? 1 : 0;
    gWinstrmTick = dt_range(0, 1000);
    gWinstrmStreamHint = dt_range(0, 7);
    gWinstrmNodeHint = dt_range(0, 31);
    gWinstrmSourceHint = dt_range(0, 31);
    for (c = 0; c < count; c++) {
        int i;
        WinstrmStream *s;
        WinstrmNode *tail;
        int chain;

        do {
            i = dt_range(0, kWinstrmMaxStreams - 1);
        } while (used[i]);
        used[i] = 1;
        s = &gWinstrmStreams[i];
        s->flags = (uint16_t)(kWinstrmFlagUsed | (dt_u32() & 0x6003));
        s->state = (uint8_t)dt_range(0, 4);
        s->pauseCount = (int8_t)dt_range(-2, 3);
        s->bufferBytes = 0x100u * (uint32_t)dt_range(1, 24);
        s->threshold = (s->bufferBytes * (uint32_t)dt_range(10, 60) / 100) & ~3u;
        s->writeOffset = (dt_u32() % s->bufferBytes) & ~3u;
        s->lastPlay = dt_u32() % s->bufferBytes;
        s->endOffset = dt_u32() % s->bufferBytes;
        s->played = dt_u32() % s->bufferBytes;
        s->fadeFlags = dt_u32() & 3;
        s->fadeLevel = (int32_t)((uint32_t)dt_range(0, 127) << 16);
        s->fadeTarget = (int32_t)((uint32_t)dt_range(0, 127) << 16);
        s->fadeStep = dt_range(-0x40000, 0x40000);
        s->volume = (uint8_t)dt_range(0, 127);
        s->pan = (int8_t)dt_range(-127, 127);
        s->drain = (uint32_t)dt_range(0, 3000);
        s->buffer = (DsBuffer *)&gBuffers[i];
        gScript.destBytes[i] = s->bufferBytes + 64;
        gScript.dest[i] = dt_alloc(s->bufferBytes + 64);
        gScript.play[i] = dt_u32() % s->bufferBytes;
        s->node.source = make_source(fmt);
        s->node.repeat = (uint32_t)dt_range(0, 3);
        tail = &s->node;
        for (chain = dt_range(0, 2); chain > 0; chain--) {
            WinstrmNode *n = make_node(fmt);

            if (n == 0) {
                break;
            }
            tail->next = n;
            tail = n;
        }
        tail->next = 0;
    }
}

/* A live stream (there is always at least one). */
static WinstrmStream *live_stream(void)
{
    int i = dt_range(0, kWinstrmMaxStreams - 1), n;

    for (n = 0; n < kWinstrmMaxStreams; n++, i = (i + 1) % kWinstrmMaxStreams) {
        if ((gWinstrmStreams[i].flags & kWinstrmFlagUsed) != 0) {
            return &gWinstrmStreams[i];
        }
    }
    return &gWinstrmStreams[0];
}

static void begin(DtCtx *c, int count)
{
    dt_seed(c);
    dt_zmem_env();
    install();
    make_world(count, rand_format_flags());
}

/* ---- setups -------------------------------------------------------------------------------------- */

static void setup_world(DtCtx *c)
{
    begin(c, dt_range(0, 5));
}

static void setup_init(DtCtx *c)
{
    begin(c, dt_range(0, 4));
    gWinstrmInitialized = c->seed % 4 == 0 ? 1 : 0;
}

static void setup_update(DtCtx *c)
{
    int i;
    uint32_t th;

    begin(c, dt_range(1, 6));
    if (c->seed % 16 == 7) {
        gWinstrmInitialized = 0;
    }
    gWinstrmPaused = c->seed % 10 == 3;
    for (i = 0; i < kWinstrmMaxStreams; i++) {
        WinstrmStream *s = &gWinstrmStreams[i];

        if ((s->flags & kWinstrmFlagUsed) == 0) {
            continue;
        }
        s->state = dt_u32() % 5 == 0 ? (uint8_t)dt_range(1, 3) : kWinstrmPlaying;
        th = s->threshold ? s->threshold : 4;
        s->threshold = th;
        /* Cursor positions that exercise: nothing to do, one refill, several, wrap. */
        s->lastPlay = dt_u32() % s->bufferBytes;
        switch (dt_u32() % 4) {
        case 0:
            gScript.play[i] = s->lastPlay;
            break;
        case 1:
            gScript.play[i] = (s->lastPlay + th / 2) % s->bufferBytes;
            break;
        case 2:
            gScript.play[i] = (s->lastPlay + 2 * th + 4) % s->bufferBytes;
            break;
        default:
            gScript.play[i] = dt_u32() % s->bufferBytes;
        }
        if (dt_u32() % 3 == 0) {
            s->writeOffset = (s->bufferBytes - (uint32_t)dt_range(0, (int32_t)th)) & ~3u;
            if (s->writeOffset >= s->bufferBytes) {
                s->writeOffset = 0;
            }
        }
        if ((s->flags & kWinstrmFlagDraining) != 0) {
            s->drain = (uint32_t)dt_range(0, 3 * (int32_t)th);
        }
        s->played = dt_u32() % (th + 1);
    }
    gScript.lockHr[0] = c->seed % 17 == 4 ? kDsErrBufferLost : 0;
    gScript.lockHr[1] = c->seed % 23 == 5 ? (DsResult)0x80004005 : 0;
    gScript.bytes2 = c->seed % 31 == 9 ? 4 : 0;
    gScript.shortReads = c->seed % 5 == 2 ? 2 : 0;
    gScript.otherHr = c->seed % 29 == 11 ? (DsResult)7 : 0;
}

static void setup_fades(DtCtx *c)
{
    int i;

    begin(c, dt_range(1, 6));
    if (c->seed % 16 == 7) {
        gWinstrmInitialized = 0;
    }
    for (i = 0; i < kWinstrmMaxStreams; i++) {
        WinstrmStream *s = &gWinstrmStreams[i];

        s->state = dt_u32() % 4 == 0 ? (uint8_t)dt_range(1, 3) : kWinstrmPlaying;
        if (dt_u32() % 3 == 0) {
            s->fadeStep = s->fadeTarget > s->fadeLevel ? 0x7fffffff : dt_range(-0x7fffffff, 0);
        }
        if (dt_u32() % 3 == 0) {
            s->fadeLevel = s->fadeTarget;
        }
    }
    gScript.otherHr = c->seed % 29 == 11 ? (DsResult)5 : 0;
}

static void setup_create(DtCtx *c)
{
    int i;

    begin(c, dt_range(0, 3));
    if (c->seed % 9 == 2) {
        for (i = 0; i < kWinstrmMaxStreams; i++) {
            gWinstrmStreams[i].flags |= kWinstrmFlagUsed;
        }
    }
    if (c->seed % 11 == 3) {
        for (i = 0; i < kWinstrmMaxSources; i++) {
            if (gWinstrmSources[i].size == 0) {
                gWinstrmSources[i].size = 1;
            }
        }
    }
    gScript.createFails = c->seed % 7 == 1;
    gScript.seekOffset = c->seed % 13 == 5 ? 3 : 0;
    gScript.createHr = c->seed % 37 == 6 ? (DsResult)0x80004005 : 0;
    gScript.lockHr[0] = c->seed % 19 == 4 ? kDsErrBufferLost : 0;
    gScript.shortReads = c->seed % 4 == 1 ? 2 : 0;
    gScript.createNext = (uint32_t)dt_range(0, 7);
    {
        WinloadSampleFile *f = rand_file();

        if (gScript.createFails) {
            f->name = "xbad.zsd";
        }
        f->size = c->seed % 5 == 0 ? (uint32_t)dt_range(1, 3000) : (uint32_t)dt_range(1000, 120000);
        DT_ARG_PTR(c, 0, dt_alloc(4));
        DT_ARG_PTR(c, 1, f);
    }
    DT_ARG_INT(c, 2, dt_u32());
    DT_ARG_INT(c, 3, c->seed % 4 == 0 ? 0 : (uint32_t)dt_range(0, 6000));
    DT_ARG_INT(c, 4, rand_format_flags() | (c->seed & 2));
}

static void setup_destroy(DtCtx *c)
{
    begin(c, dt_range(1, 5));
    gScript.otherHr = c->seed % 17 == 3 ? 9 : 0;
    DT_ARG_PTR(c, 0, live_stream());
}

static void setup_threshold(DtCtx *c)
{
    begin(c, dt_range(1, 3));
    DT_ARG_PTR(c, 0, live_stream());
    DT_ARG_INT(c, 1, dt_u32() % 5 == 0 ? (uint32_t)dt_range(-10, 300) : (uint32_t)dt_range(0, 100));
    if (c->seed % 16 == 3) {
        gWinstrmInitialized = 0;
    }
}

/* Replace / queue: a file of the playing format (or not). */
static void setup_source_op(DtCtx *c)
{
    WinstrmStream *s;
    WinstrmSource *cur;
    uint32_t fmt = rand_format_flags();

    begin(c, dt_range(1, 3));
    s = live_stream();
    cur = s->node.source;
    set_source_format(cur, fmt);
    gScript.createFails = c->seed % 9 == 1;
    gScript.seekOffset = c->seed % 17 == 5 ? 3 : 0;
    gScript.lockHr[0] = c->seed % 19 == 4 ? kDsErrBufferLost : 0;
    gScript.shortReads = c->seed % 4 == 1 ? 2 : 0;
    if (c->seed % 6 == 2) {
        fmt ^= kWinstrmFmt8Bit; /* a format mismatch */
    }
    {
        WinloadSampleFile *f = rand_file();

        if (gScript.createFails) {
            f->name = "xbad.zsd";
        }
        DT_ARG_PTR(c, 0, s);
        DT_ARG_PTR(c, 1, f);
    }
    DT_ARG_INT(c, 2, (fmt & ~(uint32_t)1) | (c->seed & 1));
    if (c->seed % 13 == 7) {
        /* every source slot and node taken */
        int i;

        for (i = 0; i < kWinstrmMaxNodes; i++) {
            gWinstrmNodes[i].repeat |= 1;
        }
    }
    if (c->seed % 16 == 3) {
        gWinstrmInitialized = 0;
    }
}

static void setup_repeat(DtCtx *c)
{
    begin(c, dt_range(1, 3));
    DT_ARG_PTR(c, 0, live_stream());
    DT_ARG_INT(c, 1, c->seed % 6 == 0 ? 0 : (uint32_t)dt_range(1, 5));
    gScript.shortReads = c->seed % 4 == 1 ? 2 : 0;
    gScript.lockHr[0] = c->seed % 19 == 4 ? kDsErrBufferLost : 0;
}

static void setup_stream(DtCtx *c)
{
    begin(c, dt_range(1, 4));
    gScript.otherHr = c->seed % 17 == 3 ? 9 : 0;
    DT_ARG_PTR(c, 0, live_stream());
    if (c->seed % 16 == 3) {
        gWinstrmInitialized = 0;
    }
}

static void setup_state(DtCtx *c)
{
    setup_stream(c);
    DT_ARG_PTR(c, 1, dt_alloc(4));
}

static void setup_start(DtCtx *c)
{
    WinstrmStream *s;

    setup_stream(c);
    s = (WinstrmStream *)(uintptr_t)c->args[0];
    s->state = (uint8_t)dt_range(0, 4);
    gScript.shortReads = c->seed % 5 == 2 ? 2 : 0;
    gScript.lockHr[0] = c->seed % 19 == 4 ? kDsErrBufferLost : 0;
    if (c->seed % 3 == 0) {
        s->flags |= kWinstrmFlagPrimed;
    }
    if (c->seed % 3 == 1) {
        s->flags &= (uint16_t)~kWinstrmFlagPrimed;
    }
}

static void setup_pause(DtCtx *c)
{
    setup_stream(c);
    DT_ARG_INT(c, 1, dt_u32() & 1 ? dt_u32() : 0);
}

static void setup_volume_pan(DtCtx *c)
{
    setup_stream(c);
    DT_ARG_INT(c, 1, garbage_high((uint32_t)dt_range(0, c->seed % 5 == 0 ? 255 : 127)));
    DT_ARG_INT(c, 2, garbage_high((uint32_t)dt_range(0, 255)));
}

static void setup_fade(DtCtx *c)
{
    setup_stream(c);
    DT_ARG_INT(c, 1, c->seed % 6 == 0 ? 0 : dt_range(0, 5000));
    DT_ARG_INT(c, 2, garbage_high(rand_format_flags() & 0x7f));
    DT_ARG_INT(c, 3, dt_u32());
    ((WinstrmStream *)(uintptr_t)c->args[0])->volume = (uint8_t)dt_range(0, 255);
}

static void setup_pause_all(DtCtx *c)
{
    begin(c, 0);
    gWinstrmPaused = dt_range(-3, 3);
    DT_ARG_INT(c, 0, dt_u32() & 1 ? dt_u32() : 0);
}

static void setup_flag(DtCtx *c)
{
    begin(c, 0);
    DT_ARG_PTR(c, 0, dt_alloc(8));
}

static void setup_play_once(DtCtx *c)
{
    begin(c, dt_range(0, 3));
    gScript.createNext = (uint32_t)dt_range(0, 7);
    gScript.createFails = c->seed % 7 == 1;
    {
        WinloadSampleFile *f = rand_file();

        if (gScript.createFails) {
            f->name = "xbad.zsd";
        }
        f->size = (uint32_t)dt_range(1000, 90000);
        DT_ARG_PTR(c, 0, f);
    }
    DT_ARG_INT(c, 1, c->seed % 3 == 0 ? 0 : (uint32_t)dt_range(0x100, 5000));
    DT_ARG_INT(c, 2, garbage_high((uint32_t)dt_range(0, 127)));
    DT_ARG_INT(c, 3, garbage_high((uint32_t)dt_range(0, 255)));
    gScript.shortReads = c->seed % 4 == 1 ? 2 : 0;
}

static void setup_fill(DtCtx *c)
{
    WinstrmStream *s;
    uint32_t bytes;

    begin(c, dt_range(1, 3));
    s = live_stream();
    gScript.lockHr[0] = c->seed % 11 == 4 ? kDsErrBufferLost : 0;
    gScript.lockHr[1] = c->seed % 29 == 8 ? (DsResult)0x80004005 : 0;
    gScript.bytes2 = c->seed % 31 == 9 ? 4 : 0;
    gScript.shortReads = c->seed % 4 == 1 ? 2 : 0;
    s->node.repeat = (uint32_t)dt_range(0, 3);
    bytes = (uint32_t)dt_range(1, 20) * 4u * (uint32_t)dt_range(1, 60);
    if (c->seed % 12 == 5) {
        bytes = 0;
    }
    if (bytes > s->bufferBytes - s->writeOffset) {
        bytes = s->bufferBytes - s->writeOffset;
    }
    if (c->seed % 3 == 0 && s->node.source->size > 40) {
        s->node.source->position = s->node.source->size - (uint32_t)dt_range(0, 40);
    }
    DT_ARG_PTR(c, 0, s);
    DT_ARG_INT(c, 1, bytes);
    DT_ARG_PTR(c, 2, dt_alloc(4));
    if (c->seed % 16 == 3) {
        gWinstrmInitialized = 0;
    }
}

static void setup_open(DtCtx *c)
{
    int i;

    begin(c, dt_range(0, 2));
    if (c->seed % 9 == 1) {
        for (i = 0; i < kWinstrmMaxSources; i++) {
            if (gWinstrmSources[i].size == 0) {
                gWinstrmSources[i].size = 1;
            }
        }
    }
    gScript.createFails = c->seed % 5 == 1;
    gScript.seekOffset = c->seed % 11 == 5 ? 3 : 0;
    {
        WinloadSampleFile *f = rand_file();

        if (gScript.createFails) {
            f->name = "xbad.zsd";
        }
        DT_ARG_PTR(c, 0, f);
    }
    DT_ARG_INT(c, 1, rand_format_flags() | (dt_u32() & 0x600e0));
}

static void setup_close(DtCtx *c)
{
    begin(c, dt_range(0, 2));
    DT_ARG_PTR(c, 0,
               c->seed % 7 == 3 ? (WinstrmSource *)dt_alloc(sizeof(WinstrmSource))
                                : make_source(rand_format_flags()));
}

static void setup_read(DtCtx *c)
{
    WinstrmSource *s;
    uint32_t bytes;

    begin(c, 0);
    s = make_source(rand_format_flags());
    if (c->seed % 3 == 0) {
        s->position = (uint32_t)dt_range(0, (int32_t)s->size);
    } else {
        s->position = 0;
    }
    bytes = (uint32_t)dt_range(0, 3) == 0 ? (uint32_t)dt_range(0, 200000) : 4u * (uint32_t)dt_range(0, 20000);
    gScript.shortReads = c->seed % 5 == 2 ? 3 : 0;
    DT_ARG_PTR(c, 0, s);
    DT_ARG_PTR(c, 1, dt_alloc(bytes + 64));
    DT_ARG_INT(c, 2, bytes);
}

static void setup_rewind(DtCtx *c)
{
    begin(c, 0);
    DT_ARG_PTR(c, 0, make_source(rand_format_flags()));
}

static void setup_node_alloc(DtCtx *c)
{
    int i;

    begin(c, 0);
    for (i = 0; i < kWinstrmMaxNodes; i++) {
        gWinstrmNodes[i].repeat = dt_u32() % 3 == 0 ? 0 : 1;
    }
    if (c->seed % 7 == 3) {
        for (i = 0; i < kWinstrmMaxNodes; i++) {
            gWinstrmNodes[i].repeat = 1;
        }
    }
    if (c->seed % 7 == 4) {
        for (i = 0; i < kWinstrmMaxNodes; i++) {
            gWinstrmNodes[i].repeat = 1;
        }
        gWinstrmNodes[kWinstrmMaxNodes - 1].repeat = 0;
        gWinstrmNodeHint = kWinstrmMaxNodes - 1;
    }
}

static void setup_node_free(DtCtx *c)
{
    setup_node_alloc(c);
    DT_ARG_PTR(c, 0,
               c->seed % 5 == 2 ? (WinstrmNode *)dt_alloc(sizeof(WinstrmNode))
                                : &gWinstrmNodes[dt_range(0, kWinstrmMaxNodes - 1)]);
}

static void setup_decode(DtCtx *c)
{
    const uint32_t count = (uint32_t)dt_range(0, 200);
    uint8_t *src = dt_alloc(count + 8);
    int32_t *pred = dt_alloc(4);
    uint32_t *step = dt_alloc(4);
    int32_t *index = dt_alloc(4);

    begin(c, 0);
    dt_fill(src, count + 8);
    *pred = dt_range(-32768, 32767);
    *step = (uint32_t)dt_range(7, 32767);
    *index = dt_range(0, 88);
    DT_ARG_PTR(c, 0, dt_alloc(count * 2 + 8));
    DT_ARG_PTR(c, 1, src);
    DT_ARG_INT(c, 2, count);
    DT_ARG_PTR(c, 3, pred);
    DT_ARG_PTR(c, 4, step);
    DT_ARG_PTR(c, 5, index);
}

DT_CASES(winstrm_cases) = {
    {"winstrmInit", 0x00470aa0, winstrmInit, 0, DT_RET_INT, setup_init, 12, 0},
    {"winstrmShutdown", 0x00470b20, winstrmShutdown, 0, DT_RET_INT, setup_world, 100, 0},
    {"winstrmUpdateFades", 0x00470ba0, winstrmUpdateFades, 0, DT_RET_INT, setup_fades, 400, 0},
    {"winstrmUpdate", 0x00470d10, winstrmUpdate, 0, DT_RET_INT, setup_update, 2000, 0},
    {"winstrmCreate", 0x00470fb0, winstrmCreate, 5, DT_RET_INT, setup_create, 800, 0},
    {"winstrmDestroy", 0x004712f0, winstrmDestroy, 1, DT_RET_INT, setup_destroy, 300, 0},
    {"winstrmSetThreshold", 0x00471400, winstrmSetThreshold, 2, DT_RET_INT, setup_threshold, 200, 0},
    {"winstrmReplaceSource", 0x00471450, winstrmReplaceSource, 3, DT_RET_INT, setup_source_op, 600, 0},
    {"winstrmSetRepeat", 0x00471610, winstrmSetRepeat, 2, DT_RET_INT, setup_repeat, 400, 0},
    {"winstrmQueueSource", 0x004716b0, winstrmQueueSource, 3, DT_RET_INT, setup_source_op, 600, 0},
    {"winstrmGetState", 0x00471840, winstrmGetState, 2, DT_RET_INT, setup_state, 100, 0},
    {"winstrmStart", 0x00471890, winstrmStart, 1, DT_RET_INT, setup_start, 800, 0},
    {"winstrmStop", 0x00471a20, winstrmStop, 1, DT_RET_INT, setup_stream, 200, 0},
    {"winstrmPause", 0x00471aa0, winstrmPause, 2, DT_RET_INT, setup_pause, 300, 0},
    {"winstrmSetVolumePan", 0x00471b60, winstrmSetVolumePan, 3, DT_RET_INT, setup_volume_pan, 400, 0},
    {"winstrmFade", 0x00471c40, winstrmFade, 4, DT_RET_INT, setup_fade, 400, 0},
    {"winstrmPauseAll", 0x00471ce0, winstrmPauseAll, 1, DT_RET_INT, setup_pause_all, 50, 0},
    {"winstrmSetFlag", 0x00471d10, winstrmSetFlag, 1, DT_RET_INT, setup_flag, 5, 0},
    {"winstrmPlayOnce", 0x00471d30, winstrmPlayOnce, 4, DT_RET_VOID, setup_play_once, 500, 0},
    {"winstrmFill", 0x00471dc0, winstrmFill, 3, DT_RET_INT, setup_fill, 1500, 0},
    {"winstrmOpenSource", 0x00472030, winstrmOpenSource, 2, DT_RET_INT, setup_open, 600, 0},
    {"winstrmCloseSource", 0x00472200, winstrmCloseSource, 1, DT_RET_VOID, setup_close, 100, 0},
    {"winstrmRead", 0x00472250, winstrmRead, 3, DT_RET_INT, setup_read, 400, 0},
    {"winstrmRewind", 0x004723b0, winstrmRewind, 1, DT_RET_INT, setup_rewind, 100, 0},
    {"winstrmNodeAlloc", 0x00472400, winstrmNodeAlloc, 0, DT_RET_INT, setup_node_alloc, 100, 0},
    {"winstrmNodeFree", 0x00472480, winstrmNodeFree, 1, DT_RET_VOID, setup_node_free, 100, 0},
    {"winstrmDecodeAdpcm", 0x004724c0, winstrmDecodeAdpcm, 6, DT_RET_VOID, setup_decode, 300, 0},
};
