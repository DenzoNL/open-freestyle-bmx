/*
 * The music override (src/platform/music.c, docs/MUSIC.md "Override") on synthetic audio only: a small FLAC
 * encoder (VERBATIM subframes) writes test files, so no music is involved. Checks the FLAC and ADPCM sources,
 * the plan (splits, overlaps, gaps, low confidence, lead-in), the renderer (bit exact at gain 1, gains, the
 * crossfade, the fade out, game pieces, the resampler, the soft clip), the track position the DirectSound
 * stand-in hands an override (through Play / Stop / SetCurrentPosition and the ring's wrap, volume, the
 * fallback to the buffer's own data), and the service (file matching by name, title and song name, the
 * fallback when a file is missing or of another length, the renders the mixer reads).
 */
#include <SDL3/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/music.h"
#include "platform/platform.h"
#include "platform/sound.h"
#include "platform/standins.h"

int testMusic(int *checks);

static int gFailures, gChecks;

#define CHECK(cond, ...)                                                                                     \
    do {                                                                                                     \
        gChecks++;                                                                                           \
        if (!(cond)) {                                                                                       \
            gFailures++;                                                                                     \
            printf("FAIL %s:%d: %s  ", __FILE__, __LINE__, #cond);                                           \
            printf(__VA_ARGS__);                                                                             \
            printf("\n");                                                                                    \
        }                                                                                                    \
    } while (0)

/* ---- a FLAC writer (STREAMINFO + fixed-size frames of VERBATIM subframes, 16 bit) ---- */

typedef struct Bits {
    uint8_t *data;
    size_t size, cap;
    uint32_t acc;
    int count;
} Bits;

static void putBits(Bits *b, uint32_t value, int n)
{
    for (int i = n - 1; i >= 0; i--) {
        b->acc = b->acc << 1 | ((value >> i) & 1);
        if (++b->count == 8) {
            if (b->size == b->cap) {
                const size_t cap = b->cap != 0 ? b->cap * 2 : 4096;
                uint8_t *const data = realloc(b->data, cap);

                if (data == NULL) {
                    abort();
                }
                b->data = data;
                b->cap = cap;
            }
            b->data[b->size++] = (uint8_t)b->acc;
            b->acc = 0;
            b->count = 0;
        }
    }
}

static uint8_t crc8(const uint8_t *p, size_t n)
{
    uint8_t crc = 0;

    for (size_t i = 0; i < n; i++) {
        crc ^= p[i];
        for (int k = 0; k < 8; k++) {
            crc = (uint8_t)((crc & 0x80) != 0 ? (crc << 1) ^ 0x07 : crc << 1);
        }
    }
    return crc;
}

static uint16_t crc16(const uint8_t *p, size_t n)
{
    uint16_t crc = 0;

    for (size_t i = 0; i < n; i++) {
        crc ^= (uint16_t)(p[i] << 8);
        for (int k = 0; k < 8; k++) {
            crc = (uint16_t)((crc & 0x8000) != 0 ? (crc << 1) ^ 0x8005 : crc << 1);
        }
    }
    return crc;
}

typedef int16_t (*Signal)(uint32_t frame, int channel);

/* Writes `frames` frames of `channels` channels at 44.1 kHz. Returns 1 on success. */
static int writeFlac(const char *path, uint32_t frames, int channels, Signal signal)
{
    const uint32_t block = 4096;
    Bits b = {0};
    SDL_IOStream *io;
    int ok;

    putBits(&b, 0x664c6143, 32); /* "fLaC" */
    putBits(&b, 0x80, 8);        /* last metadata block, STREAMINFO */
    putBits(&b, 34, 24);
    putBits(&b, block, 16);
    putBits(&b, block, 16);
    putBits(&b, 0, 24);
    putBits(&b, 0, 24);
    putBits(&b, 44100, 20);
    putBits(&b, (uint32_t)channels - 1, 3);
    putBits(&b, 15, 5);
    putBits(&b, 0, 4); /* total samples, 36 bits */
    putBits(&b, frames, 32);
    for (int i = 0; i < 4; i++) {
        putBits(&b, 0, 32); /* MD5 unknown */
    }
    for (uint32_t f = 0, number = 0; f < frames; f += block, number++) {
        const uint32_t n = frames - f < block ? frames - f : block;
        const size_t start = b.size;

        putBits(&b, 0xfff8, 16);                                /* sync, fixed block size */
        putBits(&b, 7 << 4 | 9, 8);                             /* 16-bit block size at the end, 44.1 kHz */
        putBits(&b, (uint32_t)(channels - 1) << 4 | 4 << 1, 8); /* independent channels, 16 bit */
        if (number < 0x80) {                                    /* the frame number, UTF-8 coded */
            putBits(&b, number, 8);
        } else {
            putBits(&b, 0xc0 | number >> 6, 8);
            putBits(&b, 0x80 | (number & 0x3f), 8);
        }
        putBits(&b, n - 1, 16);
        putBits(&b, crc8(b.data + start, b.size - start), 8);
        for (int c = 0; c < channels; c++) {
            putBits(&b, 0x02, 8); /* VERBATIM, no wasted bits */
            for (uint32_t i = 0; i < n; i++) {
                putBits(&b, (uint16_t)signal(f + i, c), 16);
            }
        }
        putBits(&b, crc16(b.data + start, b.size - start), 16);
    }
    io = SDL_IOFromFile(path, "wb");
    ok = io != NULL && SDL_WriteIO(io, b.data, b.size) == b.size;
    if (io != NULL) {
        SDL_CloseIO(io);
    }
    free(b.data);
    return ok;
}

/* A ramp that names its position (unique within 32768 frames); right = -left - 1. */
static int16_t ramp(uint32_t frame, int channel)
{
    const int16_t v = (int16_t)((int32_t)(frame & 0x7fff) - 16384);

    return channel == 0 ? v : (int16_t)(-v - 1);
}

/* 1 kHz at 10000 on the left, 2 kHz at 5000 on the right. */
static int16_t sine(uint32_t frame, int channel)
{
    const double f = channel == 0 ? 1000.0 : 2000.0, a = channel == 0 ? 10000.0 : 5000.0;

    return (int16_t)lrint(a * sin(2.0 * SDL_PI_D * f * frame / 44100.0));
}

static char gDir[512];

static void tempPath(char *out, size_t size, const char *name)
{
    SDL_snprintf(out, size, "%s/%s", gDir, name);
}

/* ---- sources ---- */

static void testFlacSource(void)
{
    char path[600];
    BmxMusicFlac *f;
    BmxMusicSource s;
    int16_t got[2 * 2000];
    int bad = 0;

    tempPath(path, sizeof path, "ramp.flac");
    CHECK(writeFlac(path, 100000, 2, ramp), "write %s", path);
    f = bmxMusicFlacOpen(path);
    CHECK(f != NULL, "dr_flac opens our FLAC");
    if (f == NULL) {
        return;
    }
    CHECK(bmxMusicFlacFrames(f) == 100000 && bmxMusicFlacRate(f) == 44100 && bmxMusicFlacChannels(f) == 2,
          "%llu frames %u Hz %u channels", (unsigned long long)bmxMusicFlacFrames(f),
          (unsigned)bmxMusicFlacRate(f), (unsigned)bmxMusicFlacChannels(f));
    s = bmxMusicFlacSource(f);
    /* forward, a seek ahead, a seek back, overlapping steps (the resampler's pattern), both ends */
    {
        static const int64_t starts[] = {0, 1000, 50000, 10, 51990, 52000, 53985, 99990, -5, 4090, 4096};

        for (size_t k = 0; k < sizeof starts / sizeof starts[0]; k++) {
            s.fetch(s.user, starts[k], 2000, got);
            for (int i = 0; i < 2000; i++) {
                const int64_t at = starts[k] + i;
                const int in = at >= 0 && at < 100000;
                const int16_t l = in ? ramp((uint32_t)at, 0) : 0, r = in ? ramp((uint32_t)at, 1) : 0;

                if (got[2 * i] != l || got[2 * i + 1] != r) {
                    if (bad++ < 3) {
                        printf("  at %lld: %d %d, expected %d %d\n", (long long)at, got[2 * i],
                               got[2 * i + 1], l, r);
                    }
                }
            }
        }
    }
    CHECK(bad == 0, "FLAC fetches: %d wrong frames", bad);
    bmxMusicFlacClose(f);
    tempPath(path, sizeof path, "mono.flac");
    CHECK(writeFlac(path, 5000, 1, ramp), "write mono");
    f = bmxMusicFlacOpen(path);
    CHECK(f != NULL && bmxMusicFlacChannels(f) == 1, "mono FLAC");
    if (f != NULL) {
        s = bmxMusicFlacSource(f);
        s.fetch(s.user, 100, 10, got);
        CHECK(got[0] == ramp(100, 0) && got[1] == ramp(100, 0) && got[19] == ramp(109, 0),
              "mono on both sides");
        bmxMusicFlacClose(f);
    }
    tempPath(path, sizeof path, "nothing.flac");
    CHECK(bmxMusicFlacOpen(path) == NULL, "a missing file: NULL");
}

/* winstrmDecodeAdpcm as docs/MUSIC.md's dec.py writes it. */
static void referenceAdpcm(const uint8_t *bytes, uint32_t size, int16_t *out)
{
    static const int idx[16] = {-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8};
    static const int step[89] = {
        7,    8,     9,     10,    11,    12,    13,    14,    16,    17,    19,    21,    23,    25,   28,
        31,   34,    37,    41,    45,    50,    55,    60,    66,    73,    80,    88,    97,    107,  118,
        130,  143,   157,   173,   190,   209,   230,   253,   279,   307,   337,   371,   408,   449,  494,
        544,  598,   658,   724,   796,   876,   963,   1060,  1166,  1282,  1411,  1552,  1707,  1878, 2066,
        2272, 2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,  5894,  6484,  7132,  7845, 8630,
        9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767};
    int pred = 0, s = 7, x = 0;

    for (uint32_t i = 0; i < 2 * size; i++) {
        const int nib = (i & 1) == 0 ? bytes[i / 2] & 15 : bytes[i / 2] >> 4;
        int d = s >> 3;

        d += (nib & 4) != 0 ? s : 0;
        d += (nib & 2) != 0 ? s >> 1 : 0;
        d += (nib & 1) != 0 ? s >> 2 : 0;
        pred += (nib & 8) != 0 ? -d : d;
        pred = pred > 32767 ? 32767 : (pred < -32768 ? -32768 : pred);
        out[i] = (int16_t)pred;
        x += idx[nib];
        x = x > 88 ? 88 : (x < 0 ? 0 : x);
        s = step[x];
    }
}

static void testAdpcmSource(void)
{
    enum { kBytes = 50000 };
    uint8_t *bytes = malloc(kBytes);
    int16_t *ref = malloc(sizeof(int16_t) * 2 * kBytes);
    int16_t got[2 * 3000];
    BmxMusicAdpcm *a;
    BmxMusicSource s;
    int bad = 0;

    SDL_srand(7);
    for (int i = 0; i < kBytes; i++) {
        bytes[i] = (uint8_t)SDL_rand(256);
    }
    referenceAdpcm(bytes, kBytes, ref);
    a = bmxMusicAdpcmCreate(bytes, kBytes);
    s = bmxMusicAdpcmSource(a);
    for (int k = 0; k < 40; k++) {
        const int64_t at = k == 0 ? -10 : (k == 1 ? 2 * kBytes - 100 : (int64_t)SDL_rand(2 * kBytes));

        s.fetch(s.user, at, 3000, got);
        for (int i = 0; i < 3000; i++) {
            const int64_t p = at + i;
            const int16_t want = p >= 0 && p < 2 * kBytes ? ref[p] : 0;

            bad += got[2 * i] != want || got[2 * i + 1] != want;
        }
    }
    CHECK(bad == 0, "ADPCM random access equals a sequential decode: %d wrong", bad);
    bmxMusicAdpcmDestroy(a);
    free(bytes);
    free(ref);
}

/* ---- plan and renderer ---- */

/* A synthetic edit (gameSamples 60000, FLAC 100000 frames): a lead-in before the FLAC's start, an overlap,
 * a segment 6 dB above the track's level, a low-confidence one after a gap, a fade, a tail. */
static const BmxMusicSegment kSegs[] = {
    {0, 20000, -500, 19500, 0.5f, kBmxMusicHigh},
    {19900, 30000, 50000, 60100, 1.0f, kBmxMusicHigh},
    {31000, 40000, 1000, 10000, 0.5f, kBmxMusicLow},
    {40000, 55000, 70000, 85000, 0.5f, kBmxMusicMedium},
};
static const BmxMusicEdit kEdit = {
    0, "Alpha - Song One", "Alpha - Song One.flac", 100000, 60000, 0.5f, 52000, 55000, 0.1f, kSegs, 4};

static void testPlan(void)
{
    BmxMusicPiece p[16];
    const int n = bmxMusicPlan(&kEdit, 100000, p, 16);
    static const struct {
        uint32_t start, end;
        int kind;
        int32_t offset;
    } want[] = {{0, 628, kBmxMusicGame, 0},           {628, 19950, kBmxMusicFlac, -500},
                {19950, 30000, kBmxMusicFlac, 30100}, {30000, 40000, kBmxMusicGame, 0},
                {40000, 55000, kBmxMusicFlac, 30000}, {55000, 60000, kBmxMusicGame, 0}};
    int same = n == 6;

    for (int i = 0; same && i < n; i++) {
        same = p[i].start == want[i].start && p[i].end == want[i].end && p[i].kind == want[i].kind &&
               (p[i].kind == kBmxMusicGame || (p[i].offset == want[i].offset && p[i].rate == 0.0));
    }
    CHECK(same,
          "pieces: lead-in (to the first frame + the crossfade), overlap split at 19950, gap and low "
          "confidence one game piece, tail (%d pieces)",
          n);
    for (int i = 0; i < n; i++) {
        printf("  piece %d: [%u, %u) %s offset %d gain %.3f xfade %u/%u\n", i, (unsigned)p[i].start,
               (unsigned)p[i].end, p[i].kind == kBmxMusicFlac ? "FLAC" : "game", (int)p[i].offset,
               (double)p[i].gain, (unsigned)p[i].xfadeIn, (unsigned)p[i].xfadeOut);
    }
    CHECK(n == 6 && p[1].gain == 0.5f && p[2].gain == 1.0f && p[4].gain == 0.5f && p[0].gain == 1.0f,
          "gains: the track's, the segment's own when > 1 dB off, the track's in the fade");
    CHECK(n == 6 && p[0].xfadeIn == 0 && p[0].xfadeOut == 128 && p[2].xfadeIn == 128 && p[5].xfadeOut == 0,
          "crossfades of 2 x 128 frames, none at the track's ends");
    {
        /* A short piece narrows its crossfades; the speed of a segment is honoured off 1. */
        static const BmxMusicSegment segs[] = {{0, 1000, 5000, 6000, 1.0f, kBmxMusicHigh},
                                               {1000, 1100, 9000, 9100, 1.0f, kBmxMusicHigh},
                                               {1100, 21100, 20000, 39900, 1.0f, kBmxMusicHigh}};
        const BmxMusicEdit e = {0, "t", "t.flac", 100000, 21100, 1.0f, 0, 0, 1.0f, segs, 3};
        const int m = bmxMusicPlan(&e, 100000, p, 16);

        CHECK(m == 3 && p[1].xfadeIn == 50 && p[1].xfadeOut == 50, "100-frame piece: crossfades of 50 (%d)",
              m);
        CHECK(m == 3 && fabs(p[2].rate - 0.995) < 1e-12 && p[2].origin == 20000.0, "speed 0.995: %f from %f",
              p[2].rate, p[2].origin);
    }
}

/* Test sources: the game's audio a constant, the FLAC split in two levels. */
static void gameConst(void *user, int64_t pos, uint32_t frames, int16_t *out)
{
    (void)user;
    for (uint32_t i = 0; i < frames; i++) {
        out[2 * i] = (pos + i >= 0 && pos + i < 60000) ? 1000 : 0;
        out[2 * i + 1] = (pos + i >= 0 && pos + i < 60000) ? -1000 : 0;
    }
}

static void flacTwoLevels(void *user, int64_t pos, uint32_t frames, int16_t *out)
{
    (void)user;
    for (uint32_t i = 0; i < frames; i++) {
        const int16_t v = pos + i < 40000 ? 8000 : -8000;

        out[2 * i] = v;
        out[2 * i + 1] = v;
    }
}

static void testRender(void)
{
    char path[600];
    BmxMusicPiece p[16];
    const int n = bmxMusicPlan(&kEdit, 100000, p, 16);
    BmxMusicFlac *f;
    BmxMusicSource fs, gs = {NULL, gameConst};
    int16_t *out = malloc(sizeof(int16_t) * 2 * 60000);
    uint64_t clipped = 0;
    int bad = 0;

    tempPath(path, sizeof path, "ramp.flac");
    f = bmxMusicFlacOpen(path);
    if (f == NULL || out == NULL) {
        CHECK(0, "no ramp.flac");
        bmxMusicFlacClose(f);
        free(out);
        return;
    }
    fs = bmxMusicFlacSource(f);
    bmxMusicRender(&kEdit, p, n, &fs, &gs, 0, 60000, out, &clipped);
    /* the lead-in and the game pieces: the game's audio as it is */
    CHECK(out[2 * 100] == 1000 && out[2 * 100 + 1] == -1000 && out[2 * 35000] == 1000 &&
              out[2 * 59999] == 1000,
          "game pieces: %d %d %d", out[200], out[70000], out[119998]);
    /* inside FLAC pieces: the CD at the gain (the right channel kept apart) */
    for (uint32_t t = 1000; t < 19000; t++) {
        bad += out[2 * t] != (int16_t)lrintf(ramp(t - 500, 0) * 0.5f) ||
               out[2 * t + 1] != (int16_t)lrintf(ramp(t - 500, 1) * 0.5f);
    }
    for (uint32_t t = 20200; t < 29800; t++) {
        bad += out[2 * t] != ramp(t + 30100, 0) || out[2 * t + 1] != ramp(t + 30100, 1);
    }
    CHECK(bad == 0, "FLAC pieces at their gain (0.5, and 1.0 bit exact): %d wrong", bad);
    /* the fade: 1 at its start, 0.55 halfway, 0.1 at the end */
    {
        const float g0 = (float)out[2 * 52000] / (ramp(52000 + 30000, 0) * 0.5f);
        const float g1 = (float)out[2 * 53500] / (ramp(53500 + 30000, 0) * 0.5f);
        const float g2 = (float)out[2 * 54800] / (ramp(54800 + 30000, 0) * 0.5f);

        CHECK(fabsf(g0 - 1.0f) < 0.01f && fabsf(g1 - 0.55f) < 0.01f && fabsf(g2 - 0.16f) < 0.01f,
              "fade: %.3f %.3f %.3f", (double)g0, (double)g1, (double)g2);
    }
    CHECK(clipped == 0, "nothing over full scale");
    bmxMusicFlacClose(f);

    /* The crossfade at 19950: two levels, a smooth raised cosine from one to the other. */
    {
        BmxMusicSource two = {NULL, flacTwoLevels};
        int16_t xf[2 * 600];
        int monotonic = 1, maxStep = 0;

        bmxMusicRender(&kEdit, p, n, &two, &gs, 19950 - 300, 600, xf, NULL);
        for (int i = 1; i < 600; i++) {
            const int step = xf[2 * i] - xf[2 * (i - 1)];

            monotonic &= step <= 0;
            maxStep = abs(step) > maxStep ? abs(step) : maxStep;
        }
        CHECK(xf[0] == 4000 && xf[2 * 171] == 4000 && xf[2 * 428] == -8000 && xf[2 * 599] == -8000,
              "pure before 19950 - 128, pure after 19950 + 128: %d %d %d %d", xf[0], xf[2 * 171], xf[2 * 428],
              xf[2 * 599]);
        CHECK(abs(xf[2 * 300] + 2000) <= 80 && abs(xf[2 * 299] + 2000) <= 80, "halfway at the splice: %d %d",
              xf[2 * 299], xf[2 * 300]);
        /* 4000 to -8000 (gains 0.5 and 1) over 256 frames as a raised cosine: steps of at most 12000 * pi /
         * 512 */
        CHECK(monotonic && maxStep <= 74, "no click: monotonic, largest step %d", maxStep);
    }

    /* The resampler: a segment at speed 0.995 of a 1 kHz / 2 kHz sine. */
    {
        static const BmxMusicSegment segs[] = {{1000, 41000, 2000, 41800, 1.0f, kBmxMusicHigh}};
        const BmxMusicEdit e = {0, "s", "s.flac", 100000, 42000, 1.0f, 0, 0, 1.0f, segs, 1};
        const int m = bmxMusicPlan(&e, 100000, p, 16);
        double err = 0, peak = 0;

        tempPath(path, sizeof path, "sine.flac");
        CHECK(writeFlac(path, 100000, 2, sine), "write sine");
        f = bmxMusicFlacOpen(path);
        if (f != NULL) {
            fs = bmxMusicFlacSource(f);
            bmxMusicRender(&e, p, m, &fs, NULL, 0, 42000, out, NULL);
            for (uint32_t t = 2000; t < 40000; t++) {
                const double src = 2000.0 + (t - 1000.0) * 0.995;
                const double l = 10000.0 * sin(2.0 * SDL_PI_D * 1000.0 * src / 44100.0);
                const double r = 5000.0 * sin(2.0 * SDL_PI_D * 2000.0 * src / 44100.0);

                err = fmax(err, fmax(fabs(out[2 * t] - l), fabs(out[2 * t + 1] - r)));
                peak = fmax(peak, fabs((double)out[2 * t]));
            }
            CHECK(err < 4.0 && peak > 9990,
                  "speed 0.995 through the windowed sinc: max error %.2f, peak %.0f", err, peak);
            CHECK(out[2 * 500] == 0 && out[2 * 41500] == 0, "no game source: silence outside the segment");
            bmxMusicFlacClose(f);
        }
    }

    /* The soft clip: linear to -1 dB, then saturating; the renderer counts what went over full scale. */
    CHECK(bmxMusicSoftClip(0.5f) == 0.5f && bmxMusicSoftClip(0.89f) == 0.89f &&
              bmxMusicSoftClip(1.0f) < 0.99f && bmxMusicSoftClip(3.0f) <= 1.0f &&
              bmxMusicSoftClip(3.0f) > 0.999f && bmxMusicSoftClip(-2.0f) == -bmxMusicSoftClip(2.0f) &&
              bmxMusicSoftClip(1.2f) > bmxMusicSoftClip(1.1f),
          "soft clip %f %f", (double)bmxMusicSoftClip(1.0f), (double)bmxMusicSoftClip(3.0f));
    {
        static const BmxMusicSegment segs[] = {{0, 1000, 0, 1000, 8.0f, kBmxMusicHigh}};
        const BmxMusicEdit e = {0, "c", "c.flac", 100000, 1000, 8.0f, 0, 0, 1.0f, segs, 1};
        BmxMusicSource two = {NULL, flacTwoLevels};
        const int m = bmxMusicPlan(&e, 100000, p, 16);

        clipped = 0;
        bmxMusicRender(&e, p, m, &two, NULL, 0, 1000, out, &clipped);
        CHECK(clipped > 2 * (1000 - 192) && clipped <= 2 * (1000 - 128) && out[0] == 0 &&
                  out[2 * 500] > 32700,
              "8 x 8000 after a 128-frame lead-in: clipped %llu, %d", (unsigned long long)clipped,
              out[2 * 500]);
    }
    free(out);
}

/* ---- the position the DirectSound stand-in hands an override ---- */

static const BmxDsBuffer *gOvrBuffer;
static int gOvrAttach, gOvrAttaches;
static uint64_t gOvrNext, gOvrFirst;
static int gOvrGaps, gOvrReads;

static int ovrAttach(const BmxDsBuffer *buffer, uint64_t pos)
{
    (void)pos;
    gOvrAttaches++;
    gOvrReads = 0;
    return buffer == gOvrBuffer ? gOvrAttach : -1;
}

static void ovrRead(int token, uint64_t pos, uint32_t frames, int16_t *out)
{
    (void)token;
    if (gOvrReads++ == 0) {
        gOvrFirst = pos;
    } else if (pos != gOvrNext) {
        gOvrGaps++;
    }
    gOvrNext = pos + frames;
    for (uint32_t i = 0; i < frames; i++) {
        out[2 * i] = 8192;
        out[2 * i + 1] = -8192;
    }
}

static const BmxSoundOverride kTestOverride = {ovrAttach, ovrRead, NULL};

static double meanOf(const float *out, int channel)
{
    double s = 0;

    for (uint32_t i = 0; i < bmxSoundQuantum(); i++) {
        s += out[2 * i + channel];
    }
    return s / bmxSoundQuantum();
}

static void testOverridePosition(void)
{
    BmxDs *ds = NULL;
    BmxDsBuffer *b = NULL;
    const BmxWaveFormat f = {1, 1, 44100, 88200, 2, 16, 0};
    BmxDsBufferDesc d = {sizeof d,      kDsbCtrlPan | kDsbCtrlVolume | kDsbCtrlFrequency, 0x40000, 0, &f,
                         {0, 0, 0, {0}}};
    float *out;
    uint32_t play, write;
    void *p1, *p2;
    uint32_t n1, n2;

    bmxSoundReset();
    bmxSoundUseOffline(48000);
    bmxStandDirectSoundCreate(NULL, (void **)&ds, NULL);
    ds->vtbl->setCooperativeLevel(ds, (void *)ds, 1);
    ds->vtbl->createSoundBuffer(ds, &d, &b, NULL);
    out = calloc(2 * (size_t)bmxSoundQuantum(), sizeof(float));
    bmxSoundForceFocus(1);
    /* the ring holds 0.5 everywhere: what plays without the override */
    b->vtbl->lock(b, 0, 0, &p1, &n1, &p2, &n2, kDsbLockEntireBuffer);
    for (uint32_t i = 0; i < n1 / 2; i++) {
        ((int16_t *)p1)[i] = 16384;
    }
    b->vtbl->unlock(b, p1, n1, p2, n2);
    gOvrBuffer = b;
    gOvrAttach = 0;
    bmxSoundSetOverride(&kTestOverride);

    b->vtbl->setCurrentPosition(b, 0);
    b->vtbl->play(b, 0, 0, kDsbPlayLooping);
    for (int i = 0; i < 100; i++) {
        bmxSoundMixOffline(out);
    }
    b->vtbl->getCurrentPosition(b, &play, &write);
    CHECK(gOvrAttaches == 1 && gOvrFirst == 0 && gOvrGaps == 0 && gOvrNext == write / 2,
          "1 s from position 0: reads contiguous from 0 up to the write cursor (%llu, %u)",
          (unsigned long long)gOvrNext, (unsigned)(write / 2));
    CHECK(fabs(meanOf(out, 0) - 0.25) < 1e-3 && fabs(meanOf(out, 1) + 0.25) < 1e-3,
          "the override's stereo plays, not the ring: %f %f", meanOf(out, 0), meanOf(out, 1));
    {
        float l, r;

        b->vtbl->setVolume(b, -600);
        bmxDsGains(-600, 0, &l, &r);
        bmxSoundMixOffline(out);
        CHECK(fabs(meanOf(out, 0) - 0.25 * l) < 1e-4 && fabs(meanOf(out, 1) + 0.25 * r) < 1e-4,
              "the buffer's volume applies: %f", meanOf(out, 0));
        b->vtbl->setVolume(b, 0);
    }

    /* pause: Stop, then Play resumes at the play cursor's track position */
    b->vtbl->stop(b);
    b->vtbl->getCurrentPosition(b, &play, &write);
    b->vtbl->play(b, 0, 0, kDsbPlayLooping);
    bmxSoundMixOffline(out);
    CHECK(gOvrFirst == play / 2 && gOvrAttaches == 2, "resume at the play cursor: %llu, %u",
          (unsigned long long)gOvrFirst, (unsigned)(play / 2));

    /* past the ring's wrap (131072 frames, 2.97 s): the track position keeps counting */
    for (int i = 0; i < 400; i++) {
        bmxSoundMixOffline(out);
    }
    b->vtbl->stop(b);
    b->vtbl->getCurrentPosition(b, &play, &write);
    b->vtbl->play(b, 0, 0, kDsbPlayLooping);
    bmxSoundMixOffline(out);
    CHECK(gOvrFirst == play / 2 + 131072 && gOvrGaps == 0, "after the wrap: %llu = %u + 131072",
          (unsigned long long)gOvrFirst, (unsigned)(play / 2));

    /* a new track: Stop, SetCurrentPosition(0), Play from 0 */
    b->vtbl->stop(b);
    b->vtbl->setCurrentPosition(b, 0);
    b->vtbl->play(b, 0, 0, kDsbPlayLooping);
    bmxSoundMixOffline(out);
    CHECK(gOvrFirst == 0, "SetCurrentPosition(0) restarts the track: %llu", (unsigned long long)gOvrFirst);
    /* SetCurrentPosition while playing restarts the count there */
    b->vtbl->setCurrentPosition(b, 2000);
    gOvrReads = 0;
    bmxSoundMixOffline(out);
    CHECK(gOvrFirst == 1000, "SetCurrentPosition(2000 bytes) while playing: frame %llu",
          (unsigned long long)gOvrFirst);

    /* attach declines (no FLAC for this track): the buffer's own data */
    b->vtbl->stop(b);
    gOvrAttach = -1;
    b->vtbl->play(b, 0, 0, kDsbPlayLooping);
    bmxSoundMixOffline(out);
    bmxSoundMixOffline(out);
    CHECK(fabs(meanOf(out, 0) - 0.5) < 1e-3 && fabs(meanOf(out, 1) - 0.5) < 1e-3,
          "declined: the ring plays (mono on both sides): %f %f", meanOf(out, 0), meanOf(out, 1));
    b->vtbl->stop(b);
    bmxSoundSetOverride(NULL);
    b->vtbl->release(b);
    ds->vtbl->release(ds);
    free(out);
    bmxSoundReset();
}

/* ---- the service ---- */

static uint32_t le(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
    return v;
}

/* Five tracks of kGame game samples (longer than the 2 s head) from FLACs of kFlac frames. */
enum { kGame = 120000, kFlac = 200000, kBytes = kGame / 2 };

static void renderReference(const BmxMusicEdit *e, const char *flacName, const uint8_t *adpcmBytes,
                            int16_t *out)
{
    char path[600];
    BmxMusicPiece p[16];
    BmxMusicFlac *f;
    BmxMusicAdpcm *a = bmxMusicAdpcmCreate(adpcmBytes, kBytes);
    BmxMusicSource fs, gs = bmxMusicAdpcmSource(a);
    int n;

    tempPath(path, sizeof path, flacName);
    f = bmxMusicFlacOpen(path);
    if (f != NULL) {
        fs = bmxMusicFlacSource(f);
        n = bmxMusicPlan(e, kFlac, p, 16);
        bmxMusicRender(e, p, n, &fs, &gs, 0, kGame, out, NULL);
        bmxMusicFlacClose(f);
    }
    bmxMusicAdpcmDestroy(a);
}

static void testService(void)
{
    static const BmxMusicSegment segs[] = {{0, 20000, -500, 19500, 0.5f, kBmxMusicHigh},
                                           {19900, kGame, 50000, kGame + 30100, 0.5f, kBmxMusicHigh}};
    const BmxMusicEdit edits[] = {
        {0, "Alpha - Song One", "Alpha - Song One.flac", kFlac, kGame, 0.5f, 0, 0, 1.0f, segs, 2},
        {1, "Beta's - Two", "Beta\xe2\x80\x99s - Two.flac", kFlac, kGame, 0.5f, 0, 0, 1.0f, segs, 2},
        {2, "Gamma - Three", "Gamma - Three.flac", kFlac, kGame, 0.5f, 0, 0, 1.0f, segs, 2},
        {3, "Delta - Four", "Delta - Four.flac", kFlac, kGame, 0.5f, 0, 0, 1.0f, segs, 2},
        {4, "Epsilon - Five", "Epsilon - Five.flac", kFlac, kGame, 0.5f, 0, 0, 1.0f, segs, 2},
    };
    char path[600], zsdPath[600], found[600];
    uint8_t *zsd = calloc(1, 0x100 + 5 * kBytes);
    int16_t *got = malloc(sizeof(int16_t) * 2 * kGame), *want = malloc(sizeof(int16_t) * 2 * kGame);
    SDL_IOStream *io;

    tempPath(path, sizeof path, "Alpha - Song One.flac");
    writeFlac(path, kFlac, 2, ramp);
    tempPath(path, sizeof path, "Beta's - Two.flac"); /* ' for the edit's U+2019: found by title */
    writeFlac(path, kFlac, 2, sine);
    tempPath(path, sizeof path, "03 three.FLAC"); /* found by the song's name */
    writeFlac(path, kFlac, 2, ramp);
    tempPath(path, sizeof path, "Epsilon - Five.flac"); /* another length: the game's music */
    writeFlac(path, kFlac - 1, 2, ramp);
    CHECK(bmxMusicFindFile(gDir, &edits[0], found, sizeof found) == 1, "exact name");
    CHECK(bmxMusicFindFile(gDir, &edits[1], found, sizeof found) == 2 && strstr(found, "Beta's") != NULL,
          "by title: %s", found);
    CHECK(bmxMusicFindFile(gDir, &edits[2], found, sizeof found) == 3 && strstr(found, "03 three") != NULL,
          "by song name: %s", found);
    CHECK(bmxMusicFindFile(gDir, &edits[3], found, sizeof found) == 0, "none");

    /* MUSIC.ZSD: "ZSND", the sample file table at 0x40, kBytes of ADPCM per track */
    zsd[0] = (uint8_t)'Z';
    zsd[1] = (uint8_t)'S';
    zsd[2] = (uint8_t)'N';
    zsd[3] = (uint8_t)'D';
    le(zsd + 0x20, 5);
    le(zsd + 0x24, 0x40);
    SDL_srand(3);
    for (int i = 0; i < 5; i++) {
        le(zsd + 0x40 + 8 * i, 0x100 + kBytes * (uint32_t)i);
        le(zsd + 0x44 + 8 * i, kBytes);
    }
    for (int i = 0x100; i < 0x100 + 5 * kBytes; i++) {
        zsd[i] = (uint8_t)SDL_rand(256);
    }
    tempPath(zsdPath, sizeof zsdPath, "MUSIC.ZSD");
    io = SDL_IOFromFile(zsdPath, "wb");
    SDL_WriteIO(io, zsd, 0x100 + 5 * kBytes);
    SDL_CloseIO(io);

    CHECK(bmxMusicOpen(edits, 5, gDir, zsdPath) == 3, "three tracks overridden");
    CHECK(bmxMusicTrackOfSample(0x100 + kBytes * 2, kBytes) == 2 &&
              bmxMusicTrackOfSample(0x100, kBytes - 1) == -1,
          "tracks by their sample file");
    CHECK(bmxMusicAttach(3, 0) == 0 && bmxMusicAttach(4, 0) == 0,
          "missing / wrong length: the game's own data");
    CHECK(bmxMusicAttach(0, 0) == 1 && bmxMusicWaitRendered(0, 10000), "track 0 rendered");
    /* what the mixer reads equals the renderer's output, with the game's ADPCM in the lead-in */
    renderReference(&edits[0], "Alpha - Song One.flac", zsd + 0x100, want);
    for (uint32_t at = 0; at < kGame; at += 441) {
        bmxMusicRead(0, at, at + 441 <= kGame ? 441 : kGame - at, got + 2 * at);
    }
    CHECK(memcmp(got, want, sizeof(int16_t) * 2 * kGame) == 0 && got[2 * 10] != 0,
          "reads equal the render (lead-in from the game's data)");
    bmxMusicRead(0, kGame - 100, 441, got);
    CHECK(got[2 * 99] == want[2 * (kGame - 1)] && got[2 * 100] == 0 && got[2 * 440] == 0 &&
              bmxMusicMisses() == 0,
          "past the end: silence, not a miss");
    /* another track: its render replaces track 0's; track 0's first 2 s stay (the head) */
    CHECK(bmxMusicAttach(1, 0) == 1 && bmxMusicWaitRendered(1, 10000), "track 1 rendered");
    bmxMusicRead(0, 1000, 441, got);
    CHECK(bmxMusicMisses() == 0 && memcmp(got, want + 2 * 1000, 441 * 4) == 0, "track 0's head still reads");
    {
        /* past the head, track 0 is gone (track 1's render took its place): silence, counted */
        const uint64_t before = bmxMusicMisses();

        bmxMusicRead(0, 100000, 441, got);
        CHECK(bmxMusicMisses() == before + 441 && got[0] == 0 && got[881] == 0,
              "a render not there: silence, %llu missed", (unsigned long long)(bmxMusicMisses() - before));
    }
    renderReference(&edits[1], "Beta's - Two.flac", zsd + 0x100 + kBytes, want);
    bmxMusicRead(1, 0, kGame, got);
    CHECK(memcmp(got, want, sizeof(int16_t) * 2 * kGame) == 0, "track 1 (by title) reads equal its render");
    bmxMusicClose();
    CHECK(bmxMusicAttach(0, 0) == 0, "closed: nothing overridden");
    free(zsd);
    free(got);
    free(want);
}

/* [audio] music_dir: a text value, long paths and UTF-8 kept whole. */
static void testConfigPath(void)
{
    char text[600], path[400];
    const char *got;

    SDL_snprintf(path, sizeof path, "C:\\Users\\Someone\\Music\\%s\\Dave Mirra \xe2\x80\x99 FLAC",
                 "a-rather-long-folder-name-to-pass-the-old-64-character-line-limit-of-the-parser");
    SDL_snprintf(text, sizeof text, "[audio]\r\nmusic_dir = %s\r\nsmooth_resample = 2\r\n", path);
    bmxConfigLoadText(text, "test");
    got = bmxConfigString("audio.music_dir");
    CHECK(got != NULL && strcmp(got, path) == 0, "music_dir: %s", got != NULL ? got : "(null)");
    CHECK(bmxConfigInt("audio.smooth_resample", 0) == 2 && bmxConfigString("audio.nothing") == NULL,
          "other keys unchanged");
    bmxConfigLoadText("", "test");
    CHECK(bmxConfigString("audio.music_dir") == NULL, "unset: NULL (the game's music)");
}

int testMusic(int *checks)
{
    char **files;
    int count = 0;
    const char *const temp = SDL_getenv("TEMP");

    testConfigPath();
    SDL_snprintf(gDir, sizeof gDir, "%s/bmxmusictest-%u", temp != NULL ? temp : ".",
                 (unsigned)SDL_GetCurrentThreadID());
    SDL_CreateDirectory(gDir);
    testFlacSource();
    testAdpcmSource();
    testPlan();
    testRender();
    testOverridePosition();
    testService();
    files = SDL_GlobDirectory(gDir, "*", 0, &count);
    for (int i = 0; i < count; i++) {
        char path[600];

        tempPath(path, sizeof path, files[i]);
        SDL_RemovePath(path);
    }
    SDL_free((void *)files);
    SDL_RemovePath(gDir);
    printf("music: %d checks, %d failures\n", gChecks, gFailures);
    *checks += gChecks;
    return gFailures;
}
