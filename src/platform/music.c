/*
 * The music override without game state (music.h, docs/MUSIC.md "Override"): the edit plan, the renderer, the
 * FLAC (dr_flac) and ADPCM sources, file matching, and the render thread with its cache, which the mixer
 * reads without ever waiting.
 *
 * Memory: the first kHeadFrames of every overridden track are rendered when the override opens (2 s, 345 KB
 * per track), the whole track (16-bit stereo, ~23 MB) only while it plays, one track at a time: a track
 * change frees the previous render and renders the new one from its start, far faster than real time, so the
 * head covers the moment the thread needs to begin.
 */
#include "platform/music.h"

#include <SDL3/SDL.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "dr_flac.h"
#include "platform/platform.h"

enum {
    kChunk = 4096,                         /* frames rendered per step */
    kHeadFrames = 2 * kBmxMusicSampleRate, /* rendered for every track at open */
    kSincPhases = 512,
    kAdpcmCheckpoint = 32768, /* ADPCM samples between decoder checkpoints */
    kMaxTracks = 16,
    kMaxZsd = 64,
    kPathBytes = 1024,
};

/* ---- the plan ---- */

float bmxMusicSegmentGain(const BmxMusicEdit *edit, const BmxMusicSegment *segment)
{
    const double db = 20.0 * log10((double)segment->gain / (double)edit->gain);
    const int inFade = edit->fadeEnd != 0 && segment->gameEnd > edit->fadeStart;

    return fabs(db) > 1.0 && !inFade ? segment->gain : edit->gain;
}

/* The FLAC frame a FLAC piece reads for game sample t. */
static double pieceSource(const BmxMusicPiece *p, double t)
{
    return p->rate == 0.0 ? t + p->offset : p->origin + (t - p->start) * p->rate;
}

/* The first game sample a piece may start at so that its crossfade reaches only frames >= lo, and the end
 * so that it reaches only frames < hi. */
static void validRange(const BmxMusicPiece *p, double lo, double hi, double *first, double *last)
{
    const double reach = kBmxMusicXfade;

    if (p->rate == 0.0) {
        *first = lo - p->offset + reach;
        *last = hi - p->offset - reach;
    } else {
        *first = p->start + (lo - p->origin) / p->rate + reach;
        *last = p->start + (hi - p->origin) / p->rate - reach;
    }
}

static int addPiece(BmxMusicPiece *out, int count, int max, BmxMusicPiece piece)
{
    if (piece.end <= piece.start || count >= max) {
        return count;
    }
    /* Neighbouring game pieces are one piece. */
    if (piece.kind == kBmxMusicGame && count > 0 && out[count - 1].kind == kBmxMusicGame &&
        out[count - 1].end == piece.start) {
        out[count - 1].end = piece.end;
        return count;
    }
    out[count] = piece;
    return count + 1;
}

static BmxMusicPiece gamePiece(uint32_t start, uint32_t end)
{
    BmxMusicPiece p = {0};

    p.start = start;
    p.end = end;
    p.kind = kBmxMusicGame;
    p.gain = 1.0f;
    return p;
}

/* A segment's FLAC piece over [start, end), cut back to where the FLAC (and the crossfade's reach) exists;
 * the parts cut off are game pieces. */
static int addFlacPiece(const BmxMusicEdit *edit, const BmxMusicSegment *s, uint32_t start, uint32_t end,
                        uint64_t flacFrames, BmxMusicPiece *out, int count, int max)
{
    const double r = (double)(s->flacEnd - s->flacStart) / (double)(s->gameEnd - s->gameStart);
    BmxMusicPiece p = {0};
    double lo = 0.0, hi = (double)flacFrames, first, last;
    uint32_t a, b;

    p.start = start;
    p.end = end;
    p.kind = kBmxMusicFlac;
    p.gain = bmxMusicSegmentGain(edit, s);
    if (r < 0.5 || r > 1.5) {
        /* Not a plausible speed (a broken edit list): the game's own data. */
        return addPiece(out, count, max, gamePiece(start, end));
    }
    if (fabs(r - 1.0) < 1e-3) {
        /* The CD's own samples, the offset centred on the segment (the game runs ~14 ppm off the CD). */
        p.offset =
            (int32_t)lround(((double)s->flacStart - s->gameStart + (double)s->flacEnd - s->gameEnd) / 2.0);
    } else {
        p.rate = r;
        p.origin = s->flacStart + ((double)start - s->gameStart) * r;
        lo += kBmxMusicSincHalf;
        hi -= kBmxMusicSincHalf + 1;
    }
    validRange(&p, lo, hi, &first, &last);
    a = first <= (double)start ? start : (first >= (double)end ? end : (uint32_t)ceil(first));
    b = last >= (double)end ? end : (last <= (double)a ? a : (uint32_t)floor(last));
    count = addPiece(out, count, max, gamePiece(start, a));
    if (p.rate != 0.0) {
        p.origin += ((double)a - start) * p.rate;
    }
    p.start = a;
    p.end = b;
    count = addPiece(out, count, max, p);
    return addPiece(out, count, max, gamePiece(b, end));
}

int bmxMusicPlan(const BmxMusicEdit *edit, uint64_t flacFrames, BmxMusicPiece *out, int max)
{
    uint32_t cursor = 0;
    int count = 0;

    for (uint32_t i = 0; i < edit->segmentCount; i++) {
        const BmxMusicSegment *const s = &edit->segments[i];
        uint32_t start = s->gameStart, end = s->gameEnd;

        /* Overlapping neighbours split the overlap in the middle. */
        if (i > 0 && start < edit->segments[i - 1].gameEnd) {
            start = (edit->segments[i - 1].gameEnd + start) / 2;
        }
        if (i + 1 < edit->segmentCount && edit->segments[i + 1].gameStart < end) {
            end = (end + edit->segments[i + 1].gameStart) / 2;
        }
        start = start < cursor ? cursor : start;
        end = end > edit->gameSamples ? edit->gameSamples : end;
        if (end <= start) {
            continue;
        }
        count = addPiece(out, count, max, gamePiece(cursor, start));
        if (s->confidence == kBmxMusicLow) {
            count = addPiece(out, count, max, gamePiece(start, end));
        } else {
            count = addFlacPiece(edit, s, start, end, flacFrames, out, count, max);
        }
        cursor = end;
    }
    count = addPiece(out, count, max, gamePiece(cursor, edit->gameSamples));
    /* The crossfades: as wide as kBmxMusicXfade, at most half of either neighbour. */
    for (int i = 0; i + 1 < count; i++) {
        uint32_t h = kBmxMusicXfade;

        h = h < (out[i].end - out[i].start) / 2 ? h : (out[i].end - out[i].start) / 2;
        h = h < (out[i + 1].end - out[i + 1].start) / 2 ? h : (out[i + 1].end - out[i + 1].start) / 2;
        out[i].xfadeOut = h;
        out[i + 1].xfadeIn = h;
    }
    return count;
}

/* ---- the renderer ---- */

float bmxMusicSoftClip(float x)
{
    const float knee = 0.891251f; /* -1 dB */
    const float a = fabsf(x);

    if (a <= knee) {
        return x;
    }
    return copysignf(knee + (1.0f - knee) * tanhf((a - knee) / (1.0f - knee)), x);
}

static float gSinc[kSincPhases + 1][2 * kBmxMusicSincHalf];
static SDL_InitState gSincInit;

/* Blackman-windowed sinc, cutoff 0.97 of Nyquist, each phase normalised to unity gain. */
static void sincTable(void)
{
    if (!SDL_ShouldInit(&gSincInit)) {
        return;
    }
    for (int ph = 0; ph <= kSincPhases; ph++) {
        const double frac = (double)ph / kSincPhases;
        double sum = 0.0;

        for (int k = 0; k < 2 * kBmxMusicSincHalf; k++) {
            const double x = (double)(k - (kBmxMusicSincHalf - 1)) - frac; /* tap at i - 7 .. i + 8 */
            const double w = x / kBmxMusicSincHalf;                        /* -1 .. 1 */
            const double win =
                fabs(w) >= 1.0 ? 0.0 : 0.42 + 0.5 * cos(SDL_PI_D * w) + 0.08 * cos(2 * SDL_PI_D * w);
            const double arg = SDL_PI_D * 0.97 * x;
            const double s = x == 0.0 ? 0.97 : sin(arg) / (SDL_PI_D * x);

            gSinc[ph][k] = (float)(s * win);
            sum += s * win;
        }
        for (int k = 0; k < 2 * kBmxMusicSincHalf; k++) {
            gSinc[ph][k] = (float)(gSinc[ph][k] / sum);
        }
    }
    SDL_SetInitialized(&gSincInit, true);
}

/* Raised cosine 0 -> 1 over [0, 1]. */
static float rise(double x)
{
    return (float)(0.5 - 0.5 * cos(SDL_PI_D * x));
}

/* The piece's weight at game sample t (crossfades). */
static float pieceWeight(const BmxMusicPiece *p, uint32_t t)
{
    float w = 1.0f;

    if (p->xfadeIn != 0 && t < p->start + p->xfadeIn) {
        w *= rise(((double)t - (p->start - p->xfadeIn) + 0.5) / (2.0 * p->xfadeIn));
    }
    if (p->xfadeOut != 0 && t >= p->end - p->xfadeOut) {
        w *= 1.0f - rise(((double)t - (p->end - p->xfadeOut) + 0.5) / (2.0 * p->xfadeOut));
    }
    return w;
}

static float fadeAt(const BmxMusicEdit *edit, uint32_t t)
{
    if (edit->fadeEnd == 0 || t < edit->fadeStart) {
        return 1.0f;
    }
    if (t >= edit->fadeEnd) {
        return edit->fadeFinal;
    }
    return 1.0f -
           (1.0f - edit->fadeFinal) * (float)(t - edit->fadeStart) / (float)(edit->fadeEnd - edit->fadeStart);
}

/* Adds piece p's part of [a, b) to acc (stereo, relative to t0). */
static void renderPiece(const BmxMusicEdit *edit, const BmxMusicPiece *p, BmxMusicSource *flac,
                        BmxMusicSource *game, uint32_t t0, uint32_t a, uint32_t b, float *acc,
                        int16_t *scratch)
{
    const uint32_t n = b - a;

    if (p->kind == kBmxMusicGame) {
        if (game == NULL) {
            return;
        }
        game->fetch(game->user, a, n, scratch);
        for (uint32_t i = 0; i < n; i++) {
            const float w = pieceWeight(p, a + i);

            acc[2 * (a - t0 + i)] += w * scratch[2 * i];
            acc[2 * (a - t0 + i) + 1] += w * scratch[2 * i + 1];
        }
    } else if (p->rate == 0.0) {
        flac->fetch(flac->user, (int64_t)a + p->offset, n, scratch);
        for (uint32_t i = 0; i < n; i++) {
            const float w = pieceWeight(p, a + i) * p->gain * fadeAt(edit, a + i);

            acc[2 * (a - t0 + i)] += w * scratch[2 * i];
            acc[2 * (a - t0 + i) + 1] += w * scratch[2 * i + 1];
        }
    } else {
        const int64_t lo = (int64_t)floor(pieceSource(p, a)) - (kBmxMusicSincHalf - 1);
        const int64_t hi = (int64_t)floor(pieceSource(p, b - 1)) + kBmxMusicSincHalf + 1;

        flac->fetch(flac->user, lo, (uint32_t)(hi - lo), scratch);
        for (uint32_t i = 0; i < n; i++) {
            const double src = pieceSource(p, a + i);
            const double base = floor(src);
            const double phase = (src - base) * kSincPhases;
            const int ph = (int)phase;
            const float f = (float)(phase - ph);
            const int16_t *const x = scratch + 2 * ((int64_t)base - (kBmxMusicSincHalf - 1) - lo);
            float l = 0.0f, r = 0.0f;
            const float w = pieceWeight(p, a + i) * p->gain * fadeAt(edit, a + i);

            for (int k = 0; k < 2 * kBmxMusicSincHalf; k++) {
                const float c = gSinc[ph][k] + f * (gSinc[ph + 1][k] - gSinc[ph][k]);

                l += c * x[2 * k];
                r += c * x[2 * k + 1];
            }
            acc[2 * (a - t0 + i)] += w * l;
            acc[2 * (a - t0 + i) + 1] += w * r;
        }
    }
}

void bmxMusicRender(const BmxMusicEdit *edit, const BmxMusicPiece *pieces, int count, BmxMusicSource *flac,
                    BmxMusicSource *game, uint32_t t0, uint32_t frames, int16_t *out, uint64_t *clipped)
{
    /* The resampled pieces read a little more than they write (the kernel, and up to 0.1% faster). */
    const size_t scratchFrames = (size_t)kChunk * 2 + 4 * kBmxMusicSincHalf; /* rates up to 1.5 */
    float *const acc = malloc(sizeof(float) * 2 * kChunk);
    int16_t *const scratch = malloc(sizeof(int16_t) * 2 * scratchFrames);

    sincTable();
    if (acc == NULL || scratch == NULL) {
        memset(out, 0, sizeof(int16_t) * 2 * frames);
        free(acc);
        free(scratch);
        return;
    }
    for (uint32_t done = 0; done < frames;) {
        const uint32_t t1 = t0 + done, n = frames - done < kChunk ? frames - done : kChunk;

        memset(acc, 0, sizeof(float) * 2 * n);
        for (int i = 0; i < count; i++) {
            const BmxMusicPiece *const p = &pieces[i];
            const uint32_t from = p->start - p->xfadeIn, to = p->end + p->xfadeOut;
            const uint32_t a = from > t1 ? from : t1, b = to < t1 + n ? to : t1 + n;

            if (a < b) {
                renderPiece(edit, p, flac, game, t1, a, b, acc, scratch);
            }
        }
        for (uint32_t i = 0; i < 2 * n; i++) {
            const float x = acc[i] / 32768.0f;
            const float y = bmxMusicSoftClip(x);
            const long v = lrintf(y * 32768.0f);

            if (fabsf(x) > 1.0f && clipped != NULL) {
                (*clipped)++;
            }
            out[2 * (size_t)done + i] = (int16_t)(v > 32767 ? 32767 : (v < -32768 ? -32768 : v));
        }
        done += n;
    }
    free(acc);
    free(scratch);
}

/* ---- the FLAC source ---- */

struct BmxMusicFlac {
    SDL_IOStream *io;
    drflac *dr;
    uint64_t frames;
    uint32_t rate, channels;
    /* Decoded frames [bufStart, bufStart + bufLen) (stereo); the decoder sits at their end. */
    int16_t *buf;
    uint32_t cap, bufLen;
    int64_t bufStart;
    int16_t *raw; /* the decoder's interleaved output */
};

static size_t flacRead(void *user, void *out, size_t bytes)
{
    return SDL_ReadIO(user, out, bytes);
}

static drflac_bool32 flacSeek(void *user, int offset, drflac_seek_origin origin)
{
    const SDL_IOWhence whence = origin == DRFLAC_SEEK_SET
                                    ? SDL_IO_SEEK_SET
                                    : (origin == DRFLAC_SEEK_CUR ? SDL_IO_SEEK_CUR : SDL_IO_SEEK_END);

    return SDL_SeekIO(user, offset, whence) >= 0;
}

static drflac_bool32 flacTell(void *user, drflac_int64 *cursor)
{
    const Sint64 at = SDL_TellIO(user);

    *cursor = at;
    return at >= 0;
}

BmxMusicFlac *bmxMusicFlacOpen(const char *path)
{
    BmxMusicFlac *const f = calloc(1, sizeof *f);

    if (f == NULL) {
        return NULL;
    }
    f->io = SDL_IOFromFile(path, "rb");
    if (f->io != NULL) {
        f->dr = drflac_open(flacRead, flacSeek, flacTell, f->io, NULL);
    }
    if (f->dr == NULL) {
        bmxMusicFlacClose(f);
        return NULL;
    }
    f->frames = f->dr->totalPCMFrameCount;
    f->rate = f->dr->sampleRate;
    f->channels = f->dr->channels;
    f->cap = 4 * kChunk;
    f->buf = malloc(sizeof(int16_t) * 2 * f->cap);
    f->raw = malloc(sizeof(int16_t) * f->channels * kChunk);
    if (f->buf == NULL || f->raw == NULL) {
        bmxMusicFlacClose(f);
        return NULL;
    }
    return f;
}

void bmxMusicFlacClose(BmxMusicFlac *flac)
{
    if (flac == NULL) {
        return;
    }
    if (flac->dr != NULL) {
        drflac_close(flac->dr);
    }
    if (flac->io != NULL) {
        SDL_CloseIO(flac->io);
    }
    free(flac->buf);
    free(flac->raw);
    free(flac);
}

uint64_t bmxMusicFlacFrames(const BmxMusicFlac *flac)
{
    return flac->frames;
}

uint32_t bmxMusicFlacRate(const BmxMusicFlac *flac)
{
    return flac->rate;
}

uint32_t bmxMusicFlacChannels(const BmxMusicFlac *flac)
{
    return flac->channels;
}

/* Decodes `n` more frames onto the end of the buffer (zeros once the decoder fails or ends). */
static void flacDecode(BmxMusicFlac *f, uint32_t n)
{
    if (f->bufLen + n > f->cap) {
        const uint32_t cap = (f->bufLen + n) * 2;
        int16_t *const buf = realloc(f->buf, sizeof(int16_t) * 2 * cap);

        if (buf == NULL) {
            return;
        }
        f->buf = buf;
        f->cap = cap;
    }
    while (n > 0) {
        const uint32_t want = n < kChunk ? n : kChunk;
        const uint32_t got = (uint32_t)drflac_read_pcm_frames_s16(f->dr, want, f->raw);
        int16_t *const dst = f->buf + 2 * (size_t)f->bufLen;

        for (uint32_t i = 0; i < got; i++) {
            dst[2 * i] = f->raw[i * f->channels];
            dst[2 * i + 1] = f->raw[i * f->channels + (f->channels > 1 ? 1 : 0)];
        }
        memset(dst + 2 * (size_t)got, 0, sizeof(int16_t) * 2 * (want - got));
        f->bufLen += want;
        n -= want;
    }
}

static void flacFetch(void *user, int64_t pos, uint32_t frames, int16_t *out)
{
    BmxMusicFlac *const f = user;
    const int64_t a = pos < 0 ? 0 : pos;
    const int64_t b = pos + frames > (int64_t)f->frames ? (int64_t)f->frames : pos + frames;

    memset(out, 0, sizeof(int16_t) * 2 * frames);
    if (a >= b) {
        return;
    }
    if (a < f->bufStart || a > f->bufStart + f->bufLen) {
        /* Not in or right after what is decoded: seek. */
        f->bufStart = a;
        f->bufLen = 0;
        if (!drflac_seek_to_pcm_frame(f->dr, (drflac_uint64)a)) {
            f->bufStart = INT64_MAX / 2; /* seek again next time */
            return;
        }
    } else if (a - f->bufStart > (int64_t)kChunk) {
        /* Keep a little history (the resampler's kernel steps back), drop the rest. */
        const uint32_t drop = (uint32_t)(a - f->bufStart) - kBmxMusicSincHalf * 2;

        memmove(f->buf, f->buf + 2 * (size_t)drop, sizeof(int16_t) * 2 * (f->bufLen - drop));
        f->bufStart += drop;
        f->bufLen -= drop;
    }
    if (b > f->bufStart + f->bufLen) {
        flacDecode(f, (uint32_t)(b - (f->bufStart + f->bufLen)));
    }
    if (b <= f->bufStart + f->bufLen) {
        memcpy(out + 2 * (a - pos), f->buf + 2 * (a - f->bufStart), sizeof(int16_t) * 2 * (size_t)(b - a));
    }
}

BmxMusicSource bmxMusicFlacSource(BmxMusicFlac *flac)
{
    const BmxMusicSource s = {flac, flacFetch};

    return s;
}

/* ---- the ADPCM source ---- */

/* The IMA ADPCM tables (the exe's: docs/MUSIC.md). */
static const int8_t kAdpcmIndex[16] = {-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8};
static const int16_t kAdpcmStep[89] = {
    7,    8,     9,     10,    11,    12,    13,    14,    16,    17,    19,    21,    23,    25,   28,
    31,   34,    37,    41,    45,    50,    55,    60,    66,    73,    80,    88,    97,    107,  118,
    130,  143,   157,   173,   190,   209,   230,   253,   279,   307,   337,   371,   408,   449,  494,
    544,  598,   658,   724,   796,   876,   963,   1060,  1166,  1282,  1411,  1552,  1707,  1878, 2066,
    2272, 2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,  5894,  6484,  7132,  7845, 8630,
    9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767};

typedef struct AdpcmState {
    int32_t pred, index;
} AdpcmState;

struct BmxMusicAdpcm {
    uint8_t *bytes;
    uint32_t samples;
    AdpcmState *checkpoints; /* the state before sample k * kAdpcmCheckpoint */
    AdpcmState cursorState;
    uint32_t cursor; /* the sample cursorState decodes next */
};

/* winstrmDecodeAdpcm for one sample (the low nibble first). */
static int16_t adpcmNext(const uint8_t *bytes, uint32_t i, AdpcmState *s)
{
    const uint32_t nibble = (i & 1) == 0 ? bytes[i >> 1] & 0xfu : (uint32_t)bytes[i >> 1] >> 4;
    const int32_t step = kAdpcmStep[s->index];
    int32_t diff = step >> 3;

    if ((nibble & 4) != 0) {
        diff += step;
    }
    if ((nibble & 2) != 0) {
        diff += step >> 1;
    }
    if ((nibble & 1) != 0) {
        diff += step >> 2;
    }
    if ((nibble & 8) != 0) {
        diff = -diff;
    }
    s->pred += diff;
    s->pred = s->pred > 0x7fff ? 0x7fff : (s->pred < -0x8000 ? -0x8000 : s->pred);
    s->index += kAdpcmIndex[nibble];
    s->index = s->index > 88 ? 88 : (s->index < 0 ? 0 : s->index);
    return (int16_t)s->pred;
}

BmxMusicAdpcm *bmxMusicAdpcmCreate(const uint8_t *bytes, uint32_t size)
{
    BmxMusicAdpcm *const a = calloc(1, sizeof *a);
    AdpcmState s = {0, 0};

    if (a == NULL) {
        return NULL;
    }
    a->samples = size * 2;
    a->bytes = malloc(size != 0 ? size : 1);
    a->checkpoints = malloc(sizeof(AdpcmState) * (a->samples / kAdpcmCheckpoint + 1));
    if (a->bytes == NULL || a->checkpoints == NULL) {
        bmxMusicAdpcmDestroy(a);
        return NULL;
    }
    memcpy(a->bytes, bytes, size);
    for (uint32_t i = 0; i < a->samples; i++) {
        if (i % kAdpcmCheckpoint == 0) {
            a->checkpoints[i / kAdpcmCheckpoint] = s;
        }
        (void)adpcmNext(a->bytes, i, &s);
    }
    return a;
}

void bmxMusicAdpcmDestroy(BmxMusicAdpcm *adpcm)
{
    if (adpcm != NULL) {
        free(adpcm->bytes);
        free(adpcm->checkpoints);
        free(adpcm);
    }
}

static void adpcmFetch(void *user, int64_t pos, uint32_t frames, int16_t *out)
{
    BmxMusicAdpcm *const a = user;

    memset(out, 0, sizeof(int16_t) * 2 * frames);
    for (uint32_t i = 0; i < frames; i++) {
        const int64_t at = pos + i;
        int16_t v;

        if (at < 0 || at >= a->samples) {
            continue;
        }
        if ((uint32_t)at < a->cursor || (uint32_t)at - a->cursor > kAdpcmCheckpoint) {
            a->cursor = (uint32_t)at / kAdpcmCheckpoint * kAdpcmCheckpoint;
            a->cursorState = a->checkpoints[at / kAdpcmCheckpoint];
        }
        while (a->cursor < (uint32_t)at) {
            (void)adpcmNext(a->bytes, a->cursor++, &a->cursorState);
        }
        v = adpcmNext(a->bytes, a->cursor++, &a->cursorState);
        out[2 * i] = v;
        out[2 * i + 1] = v;
    }
}

BmxMusicSource bmxMusicAdpcmSource(BmxMusicAdpcm *adpcm)
{
    const BmxMusicSource s = {adpcm, adpcmFetch};

    return s;
}

/* ---- files ---- */

static uint32_t le32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

int bmxMusicZsdRead(const char *path, BmxMusicZsdEntry *out, int max)
{
    SDL_IOStream *const io = SDL_IOFromFile(path, "rb");
    uint8_t head[0x28];
    uint32_t count, table;
    int n = -1;

    if (io == NULL) {
        return -1;
    }
    if (SDL_ReadIO(io, head, sizeof head) == sizeof head && memcmp(head, "ZSND", 4) == 0) {
        count = le32(head + 0x20);
        table = le32(head + 0x24);
        if (count <= (uint32_t)max && SDL_SeekIO(io, table, SDL_IO_SEEK_SET) >= 0) {
            n = 0;
            for (uint32_t i = 0; i < count; i++) {
                uint8_t e[8];

                if (SDL_ReadIO(io, e, sizeof e) != sizeof e) {
                    n = -1;
                    break;
                }
                out[i].offset = le32(e);
                out[i].size = le32(e + 4);
                n++;
            }
        }
    }
    SDL_CloseIO(io);
    return n;
}

/* Lower-case letters and digits of `in` (other characters, also UTF-8 ones, dropped). */
static void normalise(const char *in, size_t len, char *out, size_t size)
{
    size_t n = 0;

    for (size_t i = 0; i < len && n + 1 < size; i++) {
        const char c = in[i];

        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
            out[n++] = c;
        } else if (c >= 'A' && c <= 'Z') {
            out[n++] = (char)(c - 'A' + 'a');
        }
    }
    out[n] = 0;
}

static void joinPath(const char *dir, const char *name, char *out, size_t size)
{
    const size_t len = strlen(dir);
    const int slash = len > 0 && (dir[len - 1] == '/' || dir[len - 1] == '\\');

    SDL_snprintf(out, size, "%s%s%s", dir, slash ? "" : "/", name);
}

int bmxMusicFindFile(const char *dir, const BmxMusicEdit *edit, char *out, size_t size)
{
    char title[256], song[256], name[256];
    const char *dash = strstr(edit->title, " - ");
    char **files;
    int count = 0, found = 0;
    SDL_PathInfo info;

    joinPath(dir, edit->flacFile, out, size);
    if (SDL_GetPathInfo(out, &info) && info.type == SDL_PATHTYPE_FILE) {
        return 1;
    }
    normalise(edit->title, strlen(edit->title), title, sizeof title);
    normalise(dash != NULL ? dash + 3 : edit->title, strlen(dash != NULL ? dash + 3 : edit->title), song,
              sizeof song);
    files = SDL_GlobDirectory(dir, "*.flac", SDL_GLOB_CASEINSENSITIVE, &count);
    if (files == NULL) {
        return 0;
    }
    for (int rule = 2; rule <= 3 && found == 0; rule++) {
        for (int i = 0; i < count && found == 0; i++) {
            const char *const base =
                SDL_strrchr(files[i], '/') != NULL ? SDL_strrchr(files[i], '/') + 1 : files[i];

            normalise(base, strlen(base) - 5, name, sizeof name); /* without ".flac" */
            if ((rule == 2 && strcmp(name, title) == 0) ||
                (rule == 3 && song[0] != 0 && strstr(name, song) != NULL)) {
                joinPath(dir, files[i], out, size);
                found = rule;
            }
        }
    }
    SDL_free((void *)files);
    return found;
}

/* ---- the service ---- */

typedef struct Track {
    BmxMusicPiece pieces[kBmxMusicMaxPieces];
    const BmxMusicEdit *edit;
    int ok, failed, pieceCount;
    int16_t *head; /* the first kHeadFrames, rendered up to headFrames */
    SDL_AtomicInt headFrames;
    BmxMusicZsdEntry zsd; /* the game's ADPCM data (size 0: none) */
    char path[kPathBytes];
} Track;

static Track gTracks[kMaxTracks];
static uint32_t gTrackCount;
static BmxMusicZsdEntry gZsd[kMaxZsd];
static int gZsdCount;
static char gZsdPath[kPathBytes];
static SDL_Mutex *gLock;
static SDL_Condition *gWake;
static SDL_Thread *gThread;
static SDL_AtomicInt gWanted, gQuit, gFullDone;
static int gFullTrack = -1, gFullComplete;
static int16_t *gFull;
static uint64_t gReads, gMisses, gClipped;
static int gHeadNext;

static Track *trackOf(int index)
{
    for (uint32_t i = 0; i < gTrackCount; i++) {
        if (gTracks[i].edit->index == index) {
            return &gTracks[i];
        }
    }
    return NULL;
}

/* The game's ADPCM bytes of a track (NULL: none). */
static BmxMusicAdpcm *loadAdpcm(const Track *t)
{
    SDL_IOStream *io;
    uint8_t *bytes;
    BmxMusicAdpcm *a = NULL;

    if (t->zsd.size == 0) {
        return NULL;
    }
    io = SDL_IOFromFile(gZsdPath, "rb");
    if (io == NULL) {
        return NULL;
    }
    bytes = malloc(t->zsd.size);
    if (bytes != NULL && SDL_SeekIO(io, t->zsd.offset, SDL_IO_SEEK_SET) >= 0 &&
        SDL_ReadIO(io, bytes, t->zsd.size) == t->zsd.size) {
        a = bmxMusicAdpcmCreate(bytes, t->zsd.size);
    }
    free(bytes);
    SDL_CloseIO(io);
    return a;
}

/*
 * Renders game samples [from, to) of a track into dst (frame 0 = game sample 0), publishing the progress
 * after every chunk. Stops early when `abort` returns 1. Returns 0 when the FLAC cannot be read.
 */
static int renderTrack(Track *t, int16_t *dst, uint32_t from, uint32_t to, SDL_AtomicInt *progress,
                       int (*abort)(int track))
{
    BmxMusicFlac *const flac = bmxMusicFlacOpen(t->path);
    BmxMusicAdpcm *const adpcm = loadAdpcm(t);
    BmxMusicSource fs, gs;
    uint64_t clipped = 0;

    if (flac == NULL) {
        bmxMusicAdpcmDestroy(adpcm);
        return 0;
    }
    fs = bmxMusicFlacSource(flac);
    if (adpcm != NULL) {
        gs = bmxMusicAdpcmSource(adpcm);
    }
    for (uint32_t at = from; at < to;) {
        const uint32_t n = to - at < kChunk ? to - at : kChunk;

        if (abort(t->edit->index)) {
            break;
        }
        bmxMusicRender(t->edit, t->pieces, t->pieceCount, &fs, adpcm != NULL ? &gs : NULL, at, n,
                       dst + 2 * (size_t)at, &clipped);
        at += n;
        SDL_SetAtomicInt(progress, (int)at);
    }
    bmxMusicFlacClose(flac);
    bmxMusicAdpcmDestroy(adpcm);
    SDL_LockMutex(gLock);
    gClipped += clipped;
    SDL_UnlockMutex(gLock);
    return 1;
}

static int abortHead(int track)
{
    (void)track;
    return SDL_GetAtomicInt(&gQuit);
}

static int abortFull(int track)
{
    return SDL_GetAtomicInt(&gQuit) || SDL_GetAtomicInt(&gWanted) != track;
}

/* The wanted track's whole render: starts it (freeing the previous one) or continues it. */
static void renderFull(int want)
{
    Track *const t = trackOf(want);
    int16_t *buf = NULL;
    uint32_t from = 0;
    Uint64 began = SDL_GetTicks();

    SDL_LockMutex(gLock);
    if (gFullTrack == want) {
        buf = gFull;
        from = (uint32_t)SDL_GetAtomicInt(&gFullDone);
    } else {
        free(gFull);
        gFull = NULL;
        gFullTrack = -1;
        gFullComplete = 0;
        SDL_SetAtomicInt(&gFullDone, 0);
    }
    SDL_UnlockMutex(gLock);
    if (buf == NULL) {
        buf = malloc(sizeof(int16_t) * 2 * (size_t)t->edit->gameSamples);
        if (buf == NULL) {
            bmxLog("music: track %d: no memory for the render; silent\n", want);
            t->failed = 1;
            SDL_CompareAndSwapAtomicInt(&gWanted, want, -1);
            return;
        }
        SDL_LockMutex(gLock);
        gFull = buf;
        gFullTrack = want;
        SDL_UnlockMutex(gLock);
    }
    if (!renderTrack(t, buf, from, t->edit->gameSamples, &gFullDone, abortFull)) {
        bmxLog("music: track %d: %s cannot be read any more; silent\n", want, t->path);
        t->failed = 1;
        SDL_CompareAndSwapAtomicInt(&gWanted, want, -1);
        return;
    }
    if ((uint32_t)SDL_GetAtomicInt(&gFullDone) == t->edit->gameSamples) {
        SDL_LockMutex(gLock);
        gFullComplete = 1;
        SDL_BroadcastCondition(gWake);
        SDL_UnlockMutex(gLock);
        bmxLog("music: track %d rendered (%u frames in %llu ms)\n", want, (unsigned)t->edit->gameSamples,
               (unsigned long long)(SDL_GetTicks() - began));
    }
}

static void renderHead(Track *t)
{
    const uint32_t n = t->edit->gameSamples < kHeadFrames ? t->edit->gameSamples : kHeadFrames;
    int16_t *const buf = malloc(sizeof(int16_t) * 2 * n);

    if (buf == NULL) {
        return;
    }
    SDL_LockMutex(gLock);
    t->head = buf;
    SDL_UnlockMutex(gLock);
    if (!renderTrack(t, buf, 0, n, &t->headFrames, abortHead)) {
        bmxLog("music: track %d: %s cannot be read\n", t->edit->index, t->path);
        t->failed = 1;
    }
}

static int SDLCALL worker(void *user)
{
    (void)user;
    for (;;) {
        int want;
        Track *head = NULL;

        SDL_LockMutex(gLock);
        for (;;) {
            want = SDL_GetAtomicInt(&gWanted);
            if (SDL_GetAtomicInt(&gQuit) || (want >= 0 && !(want == gFullTrack && gFullComplete))) {
                break;
            }
            while (gHeadNext < (int)gTrackCount && !gTracks[gHeadNext].ok) {
                gHeadNext++;
            }
            if (gHeadNext < (int)gTrackCount) {
                head = &gTracks[gHeadNext++];
                want = -1;
                break;
            }
            SDL_WaitCondition(gWake, gLock);
        }
        SDL_UnlockMutex(gLock);
        if (SDL_GetAtomicInt(&gQuit)) {
            break;
        }
        if (head != NULL) {
            renderHead(head);
        } else {
            renderFull(want);
        }
    }
    return 0;
}

int bmxMusicOpen(const BmxMusicEdit *edits, uint32_t count, const char *dir, const char *zsdPath)
{
    static const char *const kHow[] = {"", "", " (found by title)", " (found by song name)"};
    int overridden = 0;

    bmxMusicClose();
    gZsdCount = zsdPath != NULL ? bmxMusicZsdRead(zsdPath, gZsd, kMaxZsd) : -1;
    SDL_snprintf(gZsdPath, sizeof gZsdPath, "%s", zsdPath != NULL ? zsdPath : "");
    if (zsdPath != NULL && gZsdCount < 0) {
        bmxLog("music: cannot read the sample files of %s: lead-ins and gaps silent\n", zsdPath);
    }
    gTrackCount = count < kMaxTracks ? count : kMaxTracks;
    for (uint32_t i = 0; i < gTrackCount; i++) {
        Track *const t = &gTracks[i];
        const BmxMusicEdit *const e = &edits[i];
        BmxMusicFlac *flac;
        int how;

        memset(t, 0, sizeof *t);
        t->edit = e;
        if (e->index >= 0 && e->index < gZsdCount) {
            if (gZsd[e->index].size * 2 == e->gameSamples) {
                t->zsd = gZsd[e->index];
            } else {
                bmxLog(
                    "music: track %d: the game's data is %u samples, the edit %u: lead-ins and gaps silent\n",
                    e->index, (unsigned)(gZsd[e->index].size * 2), (unsigned)e->gameSamples);
            }
        }
        how = bmxMusicFindFile(dir, e, t->path, sizeof t->path);
        if (how == 0) {
            bmxLog("music: track %d %s: no \"%s\" in %s: the game's music\n", e->index, e->title, e->flacFile,
                   dir);
            continue;
        }
        flac = bmxMusicFlacOpen(t->path);
        if (flac == NULL) {
            bmxLog("music: track %d %s: %s is not a readable FLAC: the game's music\n", e->index, e->title,
                   t->path);
            continue;
        }
        if (bmxMusicFlacRate(flac) != kBmxMusicSampleRate || bmxMusicFlacChannels(flac) > 2 ||
            bmxMusicFlacFrames(flac) != e->flacSamples) {
            bmxLog(
                "music: track %d %s: %s has %llu frames at %u Hz, %u channels; the edit needs %u frames at "
                "44100 Hz (another release or rip): the game's music\n",
                e->index, e->title, t->path, (unsigned long long)bmxMusicFlacFrames(flac),
                (unsigned)bmxMusicFlacRate(flac), (unsigned)bmxMusicFlacChannels(flac),
                (unsigned)e->flacSamples);
            bmxMusicFlacClose(flac);
            continue;
        }
        t->pieceCount = bmxMusicPlan(e, bmxMusicFlacFrames(flac), t->pieces, kBmxMusicMaxPieces);
        bmxMusicFlacClose(flac);
        t->ok = 1;
        overridden++;
        {
            int flacPieces = 0, gamePieces = 0;

            for (int k = 0; k < t->pieceCount; k++) {
                flacPieces += t->pieces[k].kind == kBmxMusicFlac;
                gamePieces += t->pieces[k].kind == kBmxMusicGame;
            }
            bmxLog("music: track %d %s: %s%s, %d FLAC pieces, %d from the game's data\n", e->index, e->title,
                   t->path, kHow[how], flacPieces, gamePieces);
        }
    }
    gLock = SDL_CreateMutex();
    gWake = SDL_CreateCondition();
    SDL_SetAtomicInt(&gWanted, -1);
    SDL_SetAtomicInt(&gQuit, 0);
    SDL_SetAtomicInt(&gFullDone, 0);
    gFullTrack = -1;
    gFullComplete = 0;
    gHeadNext = 0;
    gReads = gMisses = gClipped = 0;
    if (overridden > 0) {
        gThread = SDL_CreateThread(worker, "bmx music", NULL);
    }
    bmxLog("music: %d of %u tracks from %s\n", overridden, (unsigned)gTrackCount, dir);
    return overridden;
}

void bmxMusicClose(void)
{
    if (gLock == NULL) {
        return;
    }
    SDL_LockMutex(gLock);
    SDL_SetAtomicInt(&gQuit, 1);
    SDL_BroadcastCondition(gWake);
    SDL_UnlockMutex(gLock);
    if (gThread != NULL) {
        SDL_WaitThread(gThread, NULL);
        gThread = NULL;
        bmxLog(
            "music: %llu frames played from the renders, %llu frames not ready (silence); %llu samples over "
            "full scale in the renders (soft-clipped)\n",
            (unsigned long long)gReads, (unsigned long long)gMisses, (unsigned long long)gClipped);
    }
    for (uint32_t i = 0; i < gTrackCount; i++) {
        free(gTracks[i].head);
        gTracks[i].head = NULL;
    }
    free(gFull);
    gFull = NULL;
    gFullTrack = -1;
    gTrackCount = 0;
    SDL_DestroyCondition(gWake);
    SDL_DestroyMutex(gLock);
    gWake = NULL;
    gLock = NULL;
}

int bmxMusicTrackOfSample(uint32_t offset, uint32_t size)
{
    for (int i = 0; i < gZsdCount; i++) {
        if (gZsd[i].offset == offset && gZsd[i].size == size) {
            return i;
        }
    }
    return -1;
}

int bmxMusicAttach(int track, uint64_t pos)
{
    Track *const t = trackOf(track);

    if (gLock == NULL || t == NULL || !t->ok || t->failed) {
        if (t != NULL) {
            bmxLog("music: track %d %s at %u ms: the game's music\n", track, pos == 0 ? "starts" : "resumes",
                   (unsigned)(pos * 1000 / kBmxMusicSampleRate));
        }
        return 0;
    }
    SDL_LockMutex(gLock);
    if (SDL_GetAtomicInt(&gWanted) != track) {
        SDL_SetAtomicInt(&gWanted, track);
        SDL_BroadcastCondition(gWake);
    }
    SDL_UnlockMutex(gLock);
    bmxLog("music: track %d (%s) %s at %u ms from %s\n", track, t->edit->title,
           pos == 0 ? "starts" : "resumes", (unsigned)(pos * 1000 / kBmxMusicSampleRate), t->path);
    return 1;
}

void bmxMusicRead(int track, uint64_t pos, uint32_t frames, int16_t *out)
{
    Track *const t = trackOf(track);
    uint32_t i = 0;

    memset(out, 0, sizeof(int16_t) * 2 * frames);
    if (t == NULL || gLock == NULL) {
        return;
    }
    SDL_LockMutex(gLock);
    while (i < frames) {
        const uint64_t at = pos + i;
        const uint32_t head = (uint32_t)SDL_GetAtomicInt(&t->headFrames);
        const uint32_t full = gFullTrack == track ? (uint32_t)SDL_GetAtomicInt(&gFullDone) : 0;
        const int16_t *src;
        uint64_t avail;

        if (at >= t->edit->gameSamples) {
            break; /* the end of the track: silence, as the game's data ends there */
        }
        if (at < head) {
            src = t->head;
            avail = head - at;
        } else if (at < full) {
            src = gFull;
            avail = full - at;
        } else {
            gMisses += frames - i;
            break;
        }
        avail = avail < frames - i ? avail : frames - i;
        memcpy(out + 2 * (size_t)i, src + 2 * at, sizeof(int16_t) * 2 * (size_t)avail);
        i += (uint32_t)avail;
        gReads += avail;
    }
    SDL_UnlockMutex(gLock);
}

int bmxMusicWaitRendered(int track, uint32_t timeoutMs)
{
    const Uint64 until = SDL_GetTicks() + timeoutMs;
    int done = 0;

    if (gLock == NULL) {
        return 0;
    }
    SDL_LockMutex(gLock);
    while (!(done = gFullTrack == track && gFullComplete) && SDL_GetTicks() < until) {
        SDL_WaitConditionTimeout(gWake, gLock, 50);
    }
    SDL_UnlockMutex(gLock);
    return done;
}

uint64_t bmxMusicMisses(void)
{
    return gMisses;
}
