/*
 * BMX_SOUND_CHECK=1 (docs/PLATFORM.md S7): the real dsound.dll opened alongside the stand-in. Every call the
 * game makes on our IDirectSound / IDirectSoundBuffer (dsound.c) is repeated on a real object created the
 * same way, and the results are compared.
 *
 * Strict (counted as differences): every HRESULT and Release count, the created buffer's caps and format, the
 * status, the lock regions (offset and sizes of both parts), the volume / pan / frequency read back after a
 * Set, the primary format after SetFormat, and the steady write lead (write - play once both play cursors
 * move). Informational: how far our play cursor is from the real one (the two mixers run 10 ms quanta on
 * their own phase) and the sizes of the cursor steps between two reads.
 *
 * The real buffers never get the game's data (the Lock / Unlock calls are mirrored, the bytes are not
 * copied), so they play silence: a check run is not heard twice.
 */
#include <SDL3/SDL.h>
#include <stdlib.h>
#include <string.h>

#include "platform/sound.h"

enum { kMaxLines = 300, kSteps = 8, kLocks = 4 };

typedef struct StepCount {
    uint32_t bytes;
    uint64_t count;
} StepCount;

typedef struct RealLock {
    int used;
    uint32_t ourOffset;
    void *ptr1, *ptr2;
    uint32_t bytes1, bytes2;
} RealLock;

struct BmxSoundCheckBuffer {
    BmxDsBuffer *real;
    const uint8_t *realBase;
    int primary, index;
    uint32_t size, align, rate;
    RealLock locks[kLocks];
    /* cursors */
    uint32_t lastOur, lastReal;
    int haveLast, ourMoved, realMoved;
    uint32_t startOur, startReal;
    uint64_t reads, leadSame, leadDiff;
    int64_t maxAhead, maxBehind; /* bytes our play cursor was ahead of / behind the real one */
    double sumAbs;
    StepCount ourSteps[kSteps], realSteps[kSteps];
};

static int gEnabled = -1, gLines, gBufferIndex;
static uint64_t gCalls, gDifferences;
static BmxSoundCheckBuffer *gAll[256];
static int gAllCount;

static void line(const char *fmt, ...) SDL_PRINTF_VARARG_FUNC(1);

static void line(const char *fmt, ...)
{
    char text[512];
    va_list ap;

    if (gLines++ >= kMaxLines) {
        if (gLines == kMaxLines + 1) {
            bmxLog("soundcheck: more than %d lines, quiet until the summary\n", kMaxLines);
        }
        return;
    }
    va_start(ap, fmt);
    SDL_vsnprintf(text, sizeof text, fmt, ap);
    va_end(ap);
    bmxLog("soundcheck: [%7.3f s] %s\n", (double)SDL_GetTicks() / 1000.0, text);
}

static void differ(const char *fmt, ...) SDL_PRINTF_VARARG_FUNC(1);

static void differ(const char *fmt, ...)
{
    char text[400];
    va_list ap;

    gDifferences++;
    va_start(ap, fmt);
    SDL_vsnprintf(text, sizeof text, fmt, ap);
    va_end(ap);
    line("DIFFERENCE %s", text);
}

int bmxSoundCheckEnabled(void)
{
    if (gEnabled < 0) {
        const char *e = getenv("BMX_SOUND_CHECK");

        gEnabled = e != NULL && *e != 0 && *e != '0';
    }
    return gEnabled;
}

uint64_t bmxSoundCheckDifferences(void)
{
    return gDifferences;
}

BmxDs *bmxSoundCheckCreate(void)
{
    const BmxDirectSoundCreateFn create =
        (BmxDirectSoundCreateFn)bmxRealProc("DSOUND.dll", "DirectSoundCreate");
    BmxDs *real = NULL;

    if (create == NULL || create(NULL, &real, NULL) < 0) {
        bmxLog("soundcheck: the real DirectSoundCreate is not available; no check\n");
        return NULL;
    }
    line("real IDirectSound created");
    return real;
}

void bmxSoundCheckReleaseDevice(BmxDs *real)
{
    if (real != NULL) {
        const uint32_t refs = real->vtbl->release(real);

        if (refs != 0) {
            differ("IDirectSound Release: real count %u, ours 0", (unsigned)refs);
        }
    }
}

static void compareHr(const char *what, int index, BmxHresult ours, BmxHresult real)
{
    gCalls++;
    if (ours != real) {
        differ("buffer %d %s: ours %08x, DirectSound %08x", index, what, (unsigned)ours, (unsigned)real);
    }
}

void bmxSoundCheckCooperative(BmxDs *real, void *window, uint32_t level, BmxHresult ours)
{
    if (real != NULL) {
        compareHr("SetCooperativeLevel", -1, ours, real->vtbl->setCooperativeLevel(real, window, level));
    }
}

static void compareFormat(BmxSoundCheckBuffer *c, BmxDsBuffer *ours)
{
    BmxWaveFormat a, b;
    uint32_t na = 0, nb = 0;

    memset(&a, 0, sizeof a);
    memset(&b, 0, sizeof b);
    ours->vtbl->getFormat(ours, &a, sizeof a, &na);
    c->real->vtbl->getFormat(c->real, &b, sizeof b, &nb);
    if (memcmp(&a, &b, 16) != 0) {
        differ("buffer %d format: ours %u Hz %u bit %u ch, DirectSound %u Hz %u bit %u ch", c->index,
               (unsigned)a.samplesPerSec, a.bitsPerSample, a.channels, (unsigned)b.samplesPerSec,
               b.bitsPerSample, b.channels);
    }
}

static BmxSoundCheckBuffer *track(BmxDsBuffer *real, BmxDsBuffer *ours, const char *how)
{
    BmxSoundCheckBuffer *const c = calloc(1, sizeof *c);
    BmxDsbCaps a = {sizeof a, 0, 0, 0, 0}, b = {sizeof b, 0, 0, 0, 0};
    BmxWaveFormat f;
    void *p1 = NULL, *p2 = NULL;
    uint32_t n1 = 0, n2 = 0;

    if (c == NULL) {
        real->vtbl->release(real);
        return NULL;
    }
    c->real = real;
    c->index = gBufferIndex++;
    ours->vtbl->getCaps(ours, &a);
    real->vtbl->getCaps(real, &b);
    if (a.flags != b.flags || a.bufferBytes != b.bufferBytes) {
        differ("buffer %d caps: ours flags %#x, %u bytes; DirectSound flags %#x, %u bytes", c->index,
               (unsigned)a.flags, (unsigned)a.bufferBytes, (unsigned)b.flags, (unsigned)b.bufferBytes);
    }
    c->primary = (b.flags & kDsbPrimary) != 0;
    c->size = b.bufferBytes;
    memset(&f, 0, sizeof f);
    real->vtbl->getFormat(real, &f, sizeof f, NULL);
    c->align = f.blockAlign != 0 ? f.blockAlign : 1;
    c->rate = f.samplesPerSec != 0 ? f.samplesPerSec : 1;
    if (!c->primary) {
        compareFormat(c, ours);
        if (real->vtbl->lock(real, 0, 0, &p1, &n1, &p2, &n2, kDsbLockEntireBuffer) == kDsOk) {
            c->realBase = p1;
            real->vtbl->unlock(real, p1, n1, p2, n2);
        }
    }
    line("buffer %d %s: %s%u bytes, %u Hz, %u bit, %u ch, flags %#x", c->index, how,
         c->primary ? "primary, " : "", (unsigned)c->size, (unsigned)f.samplesPerSec, f.bitsPerSample,
         f.channels, (unsigned)b.flags);
    if (gAllCount < (int)(sizeof gAll / sizeof gAll[0])) {
        gAll[gAllCount++] = c;
    }
    return c;
}

BmxSoundCheckBuffer *bmxSoundCheckCreateBuffer(BmxDs *real, const BmxDsBufferDesc *desc, BmxHresult ours,
                                               BmxDsBuffer *buffer)
{
    BmxDsBuffer *rb = NULL;
    const BmxHresult hr = real->vtbl->createSoundBuffer(real, desc, &rb, NULL);

    compareHr("CreateSoundBuffer", gBufferIndex, ours, hr);
    if (rb == NULL) {
        return NULL;
    }
    if (buffer == NULL) {
        rb->vtbl->release(rb);
        return NULL;
    }
    /* The primary buffer is one object: a second CreateSoundBuffer returns it again. */
    for (int i = 0; i < gAllCount; i++) {
        if (gAll[i] != NULL && gAll[i]->real == rb) {
            return gAll[i];
        }
    }
    return track(rb, buffer, "created");
}

BmxSoundCheckBuffer *bmxSoundCheckDuplicate(BmxDs *real, BmxSoundCheckBuffer *original, BmxHresult ours,
                                            BmxDsBuffer *buffer)
{
    BmxDsBuffer *rb = NULL;
    BmxHresult hr;

    if (original == NULL) {
        return NULL;
    }
    hr = real->vtbl->duplicateSoundBuffer(real, original->real, &rb);
    compareHr("DuplicateSoundBuffer", original->index, ours, hr);
    return rb != NULL ? track(rb, buffer, "duplicated") : NULL;
}

static void addStep(StepCount *steps, uint32_t bytes)
{
    for (int i = 0; i < kSteps; i++) {
        if (steps[i].count == 0 || steps[i].bytes == bytes) {
            steps[i].bytes = bytes;
            steps[i].count++;
            return;
        }
    }
}

static void restartCursors(BmxSoundCheckBuffer *c)
{
    c->haveLast = c->ourMoved = c->realMoved = 0;
}

void bmxSoundCheckSimple(BmxSoundCheckBuffer *c, const char *name, uint32_t arg, BmxHresult ours)
{
    BmxDsBuffer *rb;
    BmxHresult hr;
    int32_t a = 0, b = 0;
    uint32_t fa = 0, fb = 0;

    if (c == NULL) {
        return;
    }
    rb = c->real;
    if (strcmp(name, "Release") == 0) {
        compareHr(name, c->index, ours, (BmxHresult)rb->vtbl->release(rb));
        return;
    }
    if (strcmp(name, "Play") == 0) {
        hr = rb->vtbl->play(rb, 0, 0, arg);
        restartCursors(c);
    } else if (strcmp(name, "Stop") == 0) {
        hr = rb->vtbl->stop(rb);
        restartCursors(c);
    } else if (strcmp(name, "SetCurrentPosition") == 0) {
        hr = rb->vtbl->setCurrentPosition(rb, arg);
        restartCursors(c);
    } else if (strcmp(name, "SetVolume") == 0) {
        hr = rb->vtbl->setVolume(rb, (int32_t)arg);
        if (hr == kDsOk && ours == kDsOk) {
            rb->vtbl->getVolume(rb, &b);
            a = (int32_t)arg;
        }
    } else if (strcmp(name, "SetPan") == 0) {
        hr = rb->vtbl->setPan(rb, (int32_t)arg);
        if (hr == kDsOk && ours == kDsOk) {
            rb->vtbl->getPan(rb, &b);
            a = (int32_t)arg;
        }
    } else if (strcmp(name, "SetFrequency") == 0) {
        hr = rb->vtbl->setFrequency(rb, arg);
        if (hr == kDsOk && ours == kDsOk) {
            rb->vtbl->getFrequency(rb, &fb);
            fa = arg != 0 ? arg : fb;
        }
    } else if (strcmp(name, "Restore") == 0) {
        hr = rb->vtbl->restore(rb);
    } else {
        return;
    }
    compareHr(name, c->index, ours, hr);
    if (a != b || fa != fb) {
        differ("buffer %d %s(%d): read back ours %d / %u, DirectSound %d / %u", c->index, name, (int)arg,
               (int)a, (unsigned)fa, (int)b, (unsigned)fb);
    }
}

void bmxSoundCheckStatus(BmxSoundCheckBuffer *c, BmxHresult ours, uint32_t status)
{
    uint32_t real = 0;
    BmxHresult hr;

    if (c == NULL) {
        return;
    }
    hr = c->real->vtbl->getStatus(c->real, &real);
    compareHr("GetStatus", c->index, ours, hr);
    if (hr == kDsOk && real != status) {
        differ("buffer %d status: ours %#x, DirectSound %#x", c->index, (unsigned)status, (unsigned)real);
    }
}

void bmxSoundCheckPosition(BmxSoundCheckBuffer *c, BmxHresult ours, uint32_t play, uint32_t write)
{
    uint32_t rp = 0, rw = 0;
    BmxHresult hr;

    if (c == NULL) {
        return;
    }
    hr = c->real->vtbl->getCurrentPosition(c->real, &rp, &rw);
    compareHr("GetCurrentPosition", c->index, ours, hr);
    if (hr != kDsOk || ours != kDsOk || c->size == 0) {
        return;
    }
    c->reads++;
    if (c->haveLast) {
        if (play != c->lastOur) {
            addStep(c->ourSteps, (play + c->size - c->lastOur) % c->size);
            c->ourMoved |= play != c->startOur;
        }
        if (rp != c->lastReal) {
            addStep(c->realSteps, (rp + c->size - c->lastReal) % c->size);
            c->realMoved |= rp != c->startReal;
        }
    } else {
        c->startOur = play;
        c->startReal = rp;
    }
    c->haveLast = 1;
    c->lastOur = play;
    c->lastReal = rp;
    {
        int64_t d = (int64_t)((play + c->size - rp) % c->size);

        if (d > (int64_t)c->size / 2) {
            d -= c->size;
        }
        c->maxAhead = d > c->maxAhead ? d : c->maxAhead;
        c->maxBehind = -d > c->maxBehind ? -d : c->maxBehind;
        c->sumAbs += (double)(d < 0 ? -d : d);
    }
    if (c->ourMoved && c->realMoved) {
        const uint32_t ourLead = (write + c->size - play) % c->size, realLead = (rw + c->size - rp) % c->size;

        if (ourLead == realLead) {
            c->leadSame++;
        } else if (c->leadDiff++ < 5) {
            differ(
                "buffer %d write lead: ours %u bytes (play %u write %u), DirectSound %u (play %u write %u)",
                c->index, (unsigned)ourLead, (unsigned)play, (unsigned)write, (unsigned)realLead,
                (unsigned)rp, (unsigned)rw);
        } else {
            gDifferences++;
        }
    }
}

void bmxSoundCheckLock(BmxSoundCheckBuffer *c, uint32_t offset, uint32_t bytes, uint32_t flags,
                       BmxHresult ours, uint32_t off1, uint32_t bytes1, int hasPtr2, uint32_t bytes2)
{
    void *p1 = NULL, *p2 = NULL;
    uint32_t n1 = 0, n2 = 0;
    BmxHresult hr;

    if (c == NULL) {
        return;
    }
    hr = c->real->vtbl->lock(c->real, offset, bytes, &p1, &n1, &p2, &n2, flags);
    compareHr("Lock", c->index, ours, hr);
    if (hr != kDsOk) {
        return;
    }
    if (ours == kDsOk) {
        const uint32_t realOff = c->realBase != NULL ? (uint32_t)((uint8_t *)p1 - c->realBase) : off1;

        if ((flags & kDsbLockFromWriteCursor) == 0 && realOff != off1) {
            differ("buffer %d Lock(%u, %u): first part at %u, DirectSound %u", c->index, (unsigned)offset,
                   (unsigned)bytes, (unsigned)off1, (unsigned)realOff);
        }
        if (n1 != bytes1 || n2 != bytes2 || (p2 != NULL) != hasPtr2) {
            differ("buffer %d Lock(%u, %u): ours %u + %u bytes, DirectSound %u + %u", c->index,
                   (unsigned)offset, (unsigned)bytes, (unsigned)bytes1, (unsigned)bytes2, (unsigned)n1,
                   (unsigned)n2);
        }
    }
    for (int i = 0; i < kLocks; i++) {
        if (!c->locks[i].used) {
            c->locks[i] = (RealLock){1, ours == kDsOk ? off1 : UINT32_MAX - 1, p1, p2, n1, n2};
            return;
        }
    }
    c->real->vtbl->unlock(c->real, p1, n1, p2, n2);
}

void bmxSoundCheckUnlock(BmxSoundCheckBuffer *c, uint32_t off1, uint32_t bytes1, int hasPtr2, uint32_t bytes2,
                         BmxHresult ours)
{
    RealLock *l = NULL;
    BmxHresult hr;

    if (c == NULL) {
        return;
    }
    for (int i = 0; i < kLocks; i++) {
        if (c->locks[i].used && c->locks[i].ourOffset == off1) {
            l = &c->locks[i];
            break;
        }
    }
    if (l == NULL) {
        /* Nothing locked there: the same call with the real buffer's pointer at that offset. */
        uint8_t *const base = (uint8_t *)(uintptr_t)c->realBase;

        hr = c->real->vtbl->unlock(c->real, off1 == UINT32_MAX || base == NULL ? NULL : base + off1, bytes1,
                                   hasPtr2 ? base : NULL, bytes2);
    } else {
        hr = c->real->vtbl->unlock(c->real, l->ptr1, bytes1, hasPtr2 ? l->ptr2 : NULL, bytes2);
        if (hr == kDsOk) {
            l->used = 0;
        }
    }
    compareHr("Unlock", c->index, ours, hr);
}

void bmxSoundCheckSetFormat(BmxSoundCheckBuffer *c, const BmxWaveFormat *format, BmxHresult ours)
{
    if (c == NULL) {
        return;
    }
    compareHr("SetFormat", c->index, ours, c->real->vtbl->setFormat(c->real, format));
}

void bmxSoundCheckRelease(BmxSoundCheckBuffer *c)
{
    if (c == NULL) {
        return;
    }
    for (int i = 0; i < gAllCount; i++) {
        if (gAll[i] == c) {
            gAll[i] = NULL;
        }
    }
    if (c->reads > 0) {
        char ours[160] = "", real[160] = "";

        for (int i = 0; i < kSteps && c->ourSteps[i].count != 0; i++) {
            SDL_snprintf(ours + strlen(ours), sizeof ours - strlen(ours), " %u x%llu",
                         (unsigned)c->ourSteps[i].bytes, (unsigned long long)c->ourSteps[i].count);
        }
        for (int i = 0; i < kSteps && c->realSteps[i].count != 0; i++) {
            SDL_snprintf(real + strlen(real), sizeof real - strlen(real), " %u x%llu",
                         (unsigned)c->realSteps[i].bytes, (unsigned long long)c->realSteps[i].count);
        }
        line("buffer %d released: %llu cursor reads; our play cursor %.1f ms ahead .. %.1f ms behind the "
             "real one "
             "(mean |d| %.1f ms); steady write lead equal %llu, different %llu; steps (bytes x count) "
             "ours:%s; "
             "DirectSound:%s",
             c->index, (unsigned long long)c->reads, (double)c->maxAhead / c->align * 1000.0 / c->rate,
             (double)c->maxBehind / c->align * 1000.0 / c->rate,
             c->sumAbs / (double)c->reads / c->align * 1000.0 / c->rate, (unsigned long long)c->leadSame,
             (unsigned long long)c->leadDiff, ours, real);
    }
    free(c);
}

void bmxSoundCheckSummary(void)
{
    if (gEnabled <= 0) {
        return;
    }
    for (int i = 0; i < gAllCount; i++) {
        if (gAll[i] != NULL && gAll[i]->reads > 0) {
            BmxSoundCheckBuffer *const c = gAll[i];

            line("buffer %d (still open): %llu cursor reads, play cursor %.1f ms ahead .. %.1f ms behind, "
                 "steady "
                 "write lead equal %llu, different %llu",
                 c->index, (unsigned long long)c->reads, (double)c->maxAhead / c->align * 1000.0 / c->rate,
                 (double)c->maxBehind / c->align * 1000.0 / c->rate, (unsigned long long)c->leadSame,
                 (unsigned long long)c->leadDiff);
        }
    }
    bmxLog("soundcheck: %llu calls compared, %llu differences\n", (unsigned long long)gCalls,
           (unsigned long long)gDifferences);
    bmxLog("soundcheck: RESULT %s\n", gDifferences == 0 ? "clean" : "DIFFERENCES");
}

void bmxSoundCheckReset(void)
{
    gEnabled = -1;
    gLines = gBufferIndex = gAllCount = 0;
    gCalls = gDifferences = 0;
    memset((void *)gAll, 0, sizeof gAll);
}
