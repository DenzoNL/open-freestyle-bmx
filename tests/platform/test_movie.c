/*
 * Our AVI reader and Cinepak decoder (src/platform/avi.c, cinepak.c) against avifil32.dll / iccvid.dll, on
 * the game's movie files (docs/PLATFORM.md S8): per file and stream the AVIFileOpenA / AVIFileGetStream
 * results, AVIStreamInfoA, ReadFormat, Start / Length, TimeToSample at every millisecond and SampleToTime at
 * every sample (and around and far beyond the ends), AVIStreamRead in the game's pattern and at random, and
 * every frame of AVIStreamGetFrame byte for byte, in order and at random positions. Windows only (target
 * bmxmovietest).
 *
 *   bmxmovietest <folder with the .avi files>
 *   bmxmovietest --drawdib <file.avi> <frame> <shot.bmp> [left top right bottom]
 *       DrawDibDraw of the real frame into a 640x480 memory DC (as zmovieDraw calls it) against a
 * BMX_MOVIE_SHOT readback of the native exe (docs/PLATFORM.md S9); also checks DrawDib's 2x stretch against
 * plain pixel doubling.
 *
 * Exit code 0 when everything matches.
 */
#include <SDL3/SDL.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>

#include <vfw.h>

#include "platform/cinepak.h"
#include "platform/standins.h"

static long failures, checks;

#define CHECK(cond, ...)                                                                                     \
    do {                                                                                                     \
        checks++;                                                                                            \
        if (!(cond)) {                                                                                       \
            if (failures++ < 300) {                                                                          \
                printf("FAIL %s:%d: %s  ", __FILE__, __LINE__, #cond);                                       \
                printf(__VA_ARGS__);                                                                         \
                printf("\n");                                                                                \
            }                                                                                                \
        }                                                                                                    \
    } while (0)

void bmxLog(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}

static uint32_t rng = 12345;

static uint32_t rnd(void)
{
    rng = rng * 1103515245u + 12345u;
    return rng >> 8;
}

static int32_t rndRange(int32_t lo, int32_t hi)
{
    return lo + (int32_t)(rnd() % (uint32_t)(hi - lo + 1));
}

/* ---- per stream ---- */

static void compareTimes(const char *what, PAVISTREAM real, void *ours, int32_t endTime, int32_t length)
{
    static const int32_t special[] = {INT32_MIN, -2000000000, -100000000, -1000000, 1000000,
                                      10000000,  100000000,   1000000000, INT32_MAX};
    long bad = 0, n = 0;

    for (int32_t t = -3000; t <= endTime + 3000; t++) {
        const LONG r = AVIStreamTimeToSample(real, t);
        const int32_t o = bmxStandAVIStreamTimeToSample(ours, t);

        n++;
        if (r != o && bad++ < 5) {
            CHECK(r == o, "%s TimeToSample(%d): real %ld ours %d", what, t, r, o);
        }
    }
    for (size_t i = 0; i < sizeof special / sizeof special[0]; i++) {
        const LONG r = AVIStreamTimeToSample(real, special[i]);

        CHECK(r == bmxStandAVIStreamTimeToSample(ours, special[i]), "%s TimeToSample(%d): real %ld ours %d",
              what, special[i], r, bmxStandAVIStreamTimeToSample(ours, special[i]));
    }
    checks += n;
    failures += bad;
    printf("  %s TimeToSample: %ld times, %ld different\n", what, n, bad);
    bad = 0;
    n = 0;
    for (int32_t s = -3000; s <= length + 3000; s++) {
        const LONG r = AVIStreamSampleToTime(real, s);
        const int32_t o = bmxStandAVIStreamSampleToTime(ours, s);

        n++;
        if (r != o && bad++ < 5) {
            CHECK(r == o, "%s SampleToTime(%d): real %ld ours %d", what, s, r, o);
        }
    }
    for (size_t i = 0; i < sizeof special / sizeof special[0]; i++) {
        const LONG r = AVIStreamSampleToTime(real, special[i]);

        CHECK(r == bmxStandAVIStreamSampleToTime(ours, special[i]), "%s SampleToTime(%d): real %ld ours %d",
              what, special[i], r, bmxStandAVIStreamSampleToTime(ours, special[i]));
    }
    checks += n;
    failures += bad;
    printf("  %s SampleToTime: %ld samples, %ld different\n", what, n, bad);
}

/* One AVIStreamRead on both, buffers prefilled alike; compares the result, both counts and the whole buffer.
 */
static int compareRead(const char *what, PAVISTREAM real, void *ours, int32_t start, int32_t samples,
                       int32_t bufferBytes, int nullBuffer, uint8_t *a, uint8_t *b, uint64_t *hash)
{
    LONG rb = -7, rs = -7;
    int32_t ob = -7, os = -7;
    HRESULT rh;
    int32_t oh;
    int same;

    memset(a, 0xa5, (size_t)bufferBytes);
    memset(b, 0xa5, (size_t)bufferBytes);
    rh = AVIStreamRead(real, start, samples, nullBuffer ? NULL : a, bufferBytes, &rb, &rs);
    oh = bmxStandAVIStreamRead(ours, start, samples, nullBuffer ? NULL : b, bufferBytes, &ob, &os);
    same = rh == oh && rb == ob && rs == os && memcmp(a, b, (size_t)bufferBytes) == 0;
    CHECK(same, "%s Read(%d, %d, %s%d): real %lx %ld bytes %ld samples, ours %x %d bytes %d samples%s", what,
          start, samples, nullBuffer ? "NULL " : "", bufferBytes, (unsigned long)rh, rb, rs, (unsigned)oh, ob,
          os, memcmp(a, b, (size_t)bufferBytes) != 0 ? ", data differ" : "");
    if (hash != NULL && rh == 0) {
        for (LONG i = 0; i < rb; i++) {
            *hash = (*hash ^ a[i]) * 0x100000001b3ull;
        }
    }
    return rh == 0 ? (int)rs : -1;
}

static void compareAudio(const char *what, PAVISTREAM real, void *ours, int32_t start, int32_t length,
                         int32_t sampleSize)
{
    enum { kBuffer = 0x8000 };
    uint8_t *const a = malloc(kBuffer + 64), *const b = malloc(kBuffer + 64);
    uint64_t hash = 0xcbf29ce484222325ull;
    long reads = 0;
    const long before = failures;

    /* The game's pattern (zmovieFillAudio): 32 KB worth of samples at a time from the start to the end. */
    for (int32_t pos = start; pos < start + length;) {
        const int32_t n = SDL_min(kBuffer / sampleSize, start + length - pos);
        const int got = compareRead(what, real, ours, pos, n, kBuffer, 0, a, b, &hash);

        reads++;
        if (got <= 0) {
            break;
        }
        pos += got;
    }
    printf("  %s Read, the game's pattern: %ld reads, FNV-1a 64 %016llx\n", what, reads,
           (unsigned long long)hash);
    /* At random: starts around and beyond both ends, counts from -1 (convenient) up, buffers NULL or small.
     */
    for (int i = 0; i < 4000; i++) {
        const int edge = (int)(rnd() % 4);
        const int32_t pos = edge == 0   ? rndRange(start - 4, start + 4)
                            : edge == 1 ? rndRange(start + length - 8, start + length + 4)
                                        : rndRange(start, start + length - 1);
        const int32_t n = (rnd() % 8) == 0 ? -1 : rndRange(0, 3 * kBuffer / sampleSize);
        const int32_t size = (rnd() % 4) == 0 ? rndRange(0, 3 * sampleSize) : rndRange(0, kBuffer + 64);

        compareRead(what, real, ours, pos, n, size, (rnd() % 6) == 0, a, b, NULL);
        reads++;
    }
    printf("  %s Read: %ld reads, %s\n", what, reads, failures == before ? "all equal" : "DIFFERENCES");
    free(a);
    free(b);
}

static void compareVideoRead(const char *what, PAVISTREAM real, void *ours, int32_t start, int32_t length)
{
    enum { kBuffer = 0x40000 };
    uint8_t *const a = malloc(kBuffer), *const b = malloc(kBuffer);
    const long before = failures;

    for (int32_t pos = start - 2; pos < start + length + 2; pos++) {
        compareRead(what, real, ours, pos, 1, kBuffer, 0, a, b, NULL);
        compareRead(what, real, ours, pos, 1, 0, 1, a, b, NULL);
        compareRead(what, real, ours, pos, 1, 100, 0, a, b, NULL);
    }
    printf("  %s Read: %d frames, %s\n", what, length, failures == before ? "all equal" : "DIFFERENCES");
    free(a);
    free(b);
}

typedef struct FrameStats {
    long frames, equal, differentBytes;
    int maxDiff;
} FrameStats;

static void compareFrame(const char *what, int32_t pos, const BITMAPINFOHEADER *r, const uint8_t *o,
                         FrameStats *st)
{
    size_t size;

    CHECK((r == NULL) == (o == NULL), "%s GetFrame(%d): real %p ours %p", what, pos, (const void *)r,
          (const void *)o);
    if (r == NULL || o == NULL) {
        return;
    }
    size = r->biSize + r->biSizeImage;
    st->frames++;
    CHECK(memcmp(r, o, r->biSize) == 0, "%s GetFrame(%d): headers differ", what, pos);
    if (memcmp(r, o, size) == 0) {
        st->equal++;
        return;
    }
    for (size_t i = r->biSize; i < size; i++) {
        const int d = abs((int)((const uint8_t *)r)[i] - (int)o[i]);

        if (d != 0) {
            /* The first few differing pixels of a stream: position (top-down) and both B, G, R. */
            if (st->differentBytes < 12 && (i - r->biSize) % 3 == 0) {
                const size_t p = (i - r->biSize) / 3;
                const uint8_t *const a = (const uint8_t *)r + i;

                printf("    frame %d pixel (%d, %d): real %3d %3d %3d ours %3d %3d %3d\n", pos,
                       (int)(p % (size_t)r->biWidth), (int)(r->biHeight - 1 - (LONG)(p / (size_t)r->biWidth)),
                       a[0], a[1], a[2], o[i], o[i + 1], o[i + 2]);
            }
            st->differentBytes++;
            if (d > st->maxDiff) {
                st->maxDiff = d;
            }
        }
    }
    CHECK(0, "%s GetFrame(%d): pixels differ", what, pos);
}

static void compareFrames(const char *what, PAVISTREAM real, void *ours, int32_t start, int32_t length)
{
    PGETFRAME rg = AVIStreamGetFrameOpen(real, NULL);
    void *og = bmxStandAVIStreamGetFrameOpen(ours, NULL);
    FrameStats st = {0, 0, 0, 0};

    CHECK(rg != NULL && og != NULL, "%s GetFrameOpen: real %p ours %p", what, (void *)rg, og);
    if (rg == NULL || og == NULL) {
        return;
    }
    for (int32_t pos = start - 2; pos < start + length + 2; pos++) {
        compareFrame(what, pos, AVIStreamGetFrame(rg, pos), bmxStandAVIStreamGetFrame(og, pos), &st);
    }
    printf("  %s GetFrame in order: %ld frames, %ld identical, %ld bytes differ, max difference %d\n", what,
           st.frames, st.equal, st.differentBytes, st.maxDiff);
    memset(&st, 0, sizeof st);
    for (int i = 0; i < 400; i++) {
        const int32_t pos = (rnd() % 10) == 0 ? rndRange(start - 3, start + length + 3)
                            : (rnd() % 2) == 0
                                ? rndRange(start, start + length - 1)
                                : SDL_min(start + length - 1, rndRange(start, start + length - 1) + 1);

        compareFrame(what, pos, AVIStreamGetFrame(rg, pos), bmxStandAVIStreamGetFrame(og, pos), &st);
    }
    printf("  %s GetFrame at random: %ld frames, %ld identical, %ld bytes differ, max difference %d\n", what,
           st.frames, st.equal, st.differentBytes, st.maxDiff);
    CHECK(AVIStreamGetFrameClose(rg) == bmxStandAVIStreamGetFrameClose(og), "%s GetFrameClose", what);
}

static void compareStream(const char *file, int index, PAVISTREAM real, void *ours)
{
    char what[160];
    uint8_t a[0x100], b[0x100];
    AVISTREAMINFOA info;
    LONG rsize = -1;
    int32_t osize = -1;
    HRESULT rh;
    int32_t oh;

    snprintf(what, sizeof what, "%s stream %d", file, index);
    memset(a, 0xcc, sizeof a);
    memset(b, 0xcc, sizeof b);
    rh = AVIStreamInfoA(real, (AVISTREAMINFOA *)a, sizeof(AVISTREAMINFOA));
    oh = bmxStandAVIStreamInfoA(ours, b, sizeof(AVISTREAMINFOA));
    CHECK(rh == oh && memcmp(a, b, sizeof a) == 0, "%s Info: real %lx ours %x, bytes %s", what,
          (unsigned long)rh, (unsigned)oh, memcmp(a, b, sizeof a) == 0 ? "equal" : "differ");
    if (memcmp(a, b, sizeof a) != 0) {
        for (size_t i = 0; i < sizeof(AVISTREAMINFOA); i++) {
            if (a[i] != b[i]) {
                printf("    info +0x%02x: real %02x ours %02x\n", (unsigned)i, a[i], b[i]);
            }
        }
    }
    memcpy(&info, a, sizeof info);
    printf("  %s: '%.4s' \"%s\" start %lu length %lu scale %lu rate %lu sample size %lu\n", what,
           (const char *)&info.fccType, info.szName, info.dwStart, info.dwLength, info.dwScale, info.dwRate,
           info.dwSampleSize);
    CHECK(AVIStreamStart(real) == bmxStandAVIStreamStart(ours), "%s Start", what);
    CHECK(AVIStreamLength(real) == bmxStandAVIStreamLength(ours), "%s Length", what);
    rh = AVIStreamReadFormat(real, 0, NULL, &rsize);
    oh = bmxStandAVIStreamReadFormat(ours, 0, NULL, &osize);
    CHECK(rh == oh && rsize == osize, "%s ReadFormat size: real %lx %ld ours %x %d", what, (unsigned long)rh,
          rsize, (unsigned)oh, osize);
    for (int32_t give = 0; give <= rsize + 8; give += (give < rsize - 2 || give > rsize + 2) ? 4 : 1) {
        memset(a, 0xcc, sizeof a);
        memset(b, 0xcc, sizeof b);
        rsize = give;
        osize = give;
        rh = AVIStreamReadFormat(real, AVIStreamStart(real), a, &rsize);
        oh = bmxStandAVIStreamReadFormat(ours, bmxStandAVIStreamStart(ours), b, &osize);
        CHECK(rh == oh && rsize == osize && memcmp(a, b, sizeof a) == 0,
              "%s ReadFormat(%d bytes): real %lx %ld ours %x %d, bytes %s", what, give, (unsigned long)rh,
              rsize, (unsigned)oh, osize, memcmp(a, b, sizeof a) == 0 ? "equal" : "differ");
    }
    compareTimes(what, real, ours,
                 (int32_t)AVIStreamSampleToTime(real, AVIStreamStart(real) + AVIStreamLength(real)),
                 (int32_t)AVIStreamLength(real));
    if (info.fccType == streamtypeAUDIO) {
        compareAudio(what, real, ours, (int32_t)info.dwStart, (int32_t)info.dwLength,
                     (int32_t)info.dwSampleSize);
    } else if (info.fccType == streamtypeVIDEO) {
        compareVideoRead(what, real, ours, (int32_t)info.dwStart, (int32_t)info.dwLength);
        compareFrames(what, real, ours, (int32_t)info.dwStart, (int32_t)info.dwLength);
    }
}

static void compareFile(const char *folder, const char *name)
{
    char path[MAX_PATH];
    PAVIFILE realFile = NULL;
    void *ourFile = NULL;
    PAVISTREAM real[8];
    void *ours[8];
    int count = 0;
    HRESULT rh;
    int32_t oh;

    /* As the game spells it: backslashes, upper case (the name goes into szName). */
    snprintf(path, sizeof path, "%s\\%s", folder, name);
    CharUpperA(path + strlen(folder) + 1);
    printf("%s\n", path);
    rh = AVIFileOpenA(&realFile, path, 0, NULL);
    oh = bmxStandAVIFileOpenA(&ourFile, path, 0, NULL);
    CHECK(rh == oh, "AVIFileOpenA: real %lx ours %x", (unsigned long)rh, (unsigned)oh);
    if (rh != 0 || oh != 0) {
        return;
    }
    for (;;) {
        PAVISTREAM r = NULL;
        void *o = NULL;

        rh = AVIFileGetStream(realFile, &r, 0, count);
        oh = bmxStandAVIFileGetStream(ourFile, &o, 0, count);
        CHECK(rh == oh && (r == NULL) == (o == NULL), "GetStream(%d): real %lx ours %x", count,
              (unsigned long)rh, (unsigned)oh);
        if (rh != 0 || oh != 0 || count == 8) {
            break;
        }
        real[count] = r;
        ours[count] = o;
        count++;
    }
    /* By type: the first video and audio stream, a second one of each that is not there. */
    for (int k = 0; k < 4; k++) {
        const DWORD type = (k & 1) != 0 ? streamtypeAUDIO : streamtypeVIDEO;
        PAVISTREAM r = NULL;
        void *o = NULL;

        rh = AVIFileGetStream(realFile, &r, type, k / 2);
        oh = bmxStandAVIFileGetStream(ourFile, &o, type, k / 2);
        CHECK(rh == oh, "GetStream('%.4s', %d): real %lx ours %x", (const char *)&type, k / 2,
              (unsigned long)rh, (unsigned)oh);
        if (r != NULL && o != NULL) {
            CHECK(AVIStreamRelease(r) == bmxStandAVIStreamRelease(o), "Release of a typed stream");
        }
    }
    /* The game releases the file right after taking the streams. */
    {
        const ULONG r = AVIFileRelease(realFile);
        const uint32_t o = bmxStandAVIFileRelease(ourFile);

        CHECK(r == o, "AVIFileRelease: real %lu ours %u", r, o);
    }
    for (int i = 0; i < count; i++) {
        compareStream(path + strlen(folder) + 1, i, real[i], ours[i]);
    }
    for (int i = 0; i < count; i++) {
        const ULONG r = AVIStreamAddRef(real[i]);
        const uint32_t o = bmxStandAVIStreamAddRef(ours[i]);

        CHECK(r == o, "AVIStreamAddRef: real %lu ours %u", r, o);
        for (int k = 0; k < 2; k++) {
            const ULONG r2 = AVIStreamRelease(real[i]);
            const uint32_t o2 = bmxStandAVIStreamRelease(ours[i]);

            CHECK(r2 == o2, "AVIStreamRelease: real %lu ours %u", r2, o2);
        }
    }
}

/* ---- the decoder against iccvid on hand-made frames ---- */

typedef struct CvBuf {
    uint8_t b[16384];
    int n;
} CvBuf;

static void cvPut(CvBuf *b, uint32_t v, int bytes)
{
    for (int i = bytes - 1; i >= 0; i--) {
        b->b[b->n++] = (uint8_t)(v >> (8 * i));
    }
}

static void cvSet24(CvBuf *b, int at, uint32_t v)
{
    b->b[at] = (uint8_t)(v >> 16);
    b->b[at + 1] = (uint8_t)(v >> 8);
    b->b[at + 2] = (uint8_t)v;
}

/* A codebook chunk: full (all 256 or fewer entries) or selective (random flag words, sometimes ending in a
 * zero word, the case of iccvid's quirk), colour or greyscale; entries random bytes. */
static void cvCodebook(CvBuf *f, int v1, int selective, int grey)
{
    const int at = f->n;
    const int n = grey ? 4 : 6;

    cvPut(f, (uint32_t)(0x20 | (v1 ? 2 : 0) | (selective ? 1 : 0) | (grey ? 4 : 0)), 1);
    cvPut(f, 0, 3);
    if (selective) {
        const int words = 1 + (int)(rnd() % 8);

        for (int g = 0; g < words; g++) {
            const int kind = (int)(rnd() % 5);
            const uint32_t w = kind == 0   ? 0
                               : kind == 1 ? 0xffffffffu
                               : kind == 2 ? 0x80000000u >> (rnd() % 32)
                                           : (rnd() << 16) ^ rnd();

            cvPut(f, w, 4);
            for (int b = 0; b < 32; b++) {
                if ((w & (0x80000000u >> b)) != 0) {
                    for (int k = 0; k < n; k++) {
                        cvPut(f, rnd() & 0xff, 1);
                    }
                }
            }
        }
    } else {
        const int count = (rnd() % 3) == 0 ? (int)(rnd() % 256) : 256;

        for (int e = 0; e < count * n; e++) {
            cvPut(f, rnd() & 0xff, 1);
        }
    }
    cvSet24(f, at + 1, (uint32_t)(f->n - at));
}

/* Vectors for a strip of w x h: 0x30 (V1 / V4 per block), 0x31 (skips too) or 0x32 (all V1). */
static void cvVectors(CvBuf *f, int id, int blocks)
{
    const int at = f->n;
    uint32_t flags = 0;
    int bit = 0, flagAt = -1;

    cvPut(f, (uint32_t)id, 1);
    cvPut(f, 0, 3);
    for (int i = 0; i < blocks; i++) {
        int coded = 1, v4 = 0;

        for (int pass = 0; pass < 2; pass++) {
            if ((pass == 0 && id == 0x31) || (pass == 1 && id != 0x32 && coded)) {
                const int set = pass == 0 ? (int)(rnd() % 3) != 0 : (int)(rnd() % 2);

                if (bit == 0) {
                    if (flagAt >= 0) {
                        cvSet24(f, flagAt, flags >> 8);
                        f->b[flagAt + 3] = (uint8_t)flags;
                    }
                    flagAt = f->n;
                    cvPut(f, 0, 4);
                    flags = 0;
                    bit = 32;
                }
                bit--;
                if (set) {
                    flags |= 1u << bit;
                }
                if (pass == 0) {
                    coded = set;
                } else {
                    v4 = set;
                }
            }
        }
        if (coded) {
            if (flagAt >= 0) {
                cvSet24(f, flagAt, flags >> 8);
                f->b[flagAt + 3] = (uint8_t)flags;
            }
            for (int k = 0; k < (v4 ? 4 : 1); k++) {
                cvPut(f, rnd() & 0xff, 1);
            }
        }
    }
    if (flagAt >= 0) {
        cvSet24(f, flagAt, flags >> 8);
        f->b[flagAt + 3] = (uint8_t)flags;
    }
    cvSet24(f, at + 1, (uint32_t)(f->n - at));
}

static void compareSynthetic(void)
{
    enum { kW = 64, kH = 48, kStride = kW * 3 };
    static CinepakDecoder ours;
    static uint8_t a[kStride * kH], b[kStride * kH];
    BITMAPINFOHEADER in, out;
    HIC hic = ICOpen(ICTYPE_VIDEO, mmioFOURCC('c', 'v', 'i', 'd'), ICMODE_DECOMPRESS);
    long frames = 0, equal = 0, differentBytes = 0;
    int maxDiff = 0;

    CHECK(hic != NULL, "ICOpen(cvid)");
    if (hic == NULL) {
        return;
    }
    memset(&in, 0, sizeof in);
    in.biSize = sizeof in;
    in.biWidth = kW;
    in.biHeight = kH;
    in.biPlanes = 1;
    in.biBitCount = 24;
    in.biCompression = mmioFOURCC('c', 'v', 'i', 'd');
    out = in;
    out.biCompression = BI_RGB;
    out.biSizeImage = sizeof a;
    ICDecompressBegin(hic, &in, &out);
    cinepakInit(&ours, kW, kH);
    memset(a, 0, sizeof a);
    memset(b, 0, sizeof b);
    for (int i = 0; i < 3000; i++) {
        static CvBuf f;
        const int key = i % 25 == 0;
        const int strips = 1 + (int)(rnd() % 3);
        const int inherit = (int)(rnd() % 2);
        int y = 0;

        f.n = 0;
        cvPut(&f, (uint32_t)(inherit ? 0 : 1), 1);
        cvPut(&f, 0, 3);
        cvPut(&f, kW, 2);
        cvPut(&f, kH, 2);
        cvPut(&f, (uint32_t)strips, 2);
        for (int s = 0; s < strips; s++) {
            const int at = f.n;
            const int h = s == strips - 1 ? kH - y : 4 * (1 + (int)(rnd() % 4));
            const int grey =
                0; /* greyscale chunks are not matched in every case (cinepak.c); the movies have none */

            cvPut(&f, key && (s == 0 || (rnd() % 2) == 0) ? 0x10 : 0x11,
                  1); /* key frames: later strips 0x10 or 0x11 */
            cvPut(&f, 0, 3);
            cvPut(&f, 0, 2); /* top 0: below the previous strip */
            cvPut(&f, 0, 2);
            cvPut(&f, (uint32_t)h, 2);
            cvPut(&f, kW, 2);
            if (key || (rnd() % 2) == 0) {
                cvCodebook(&f, 0, !key && (rnd() % 2) == 0, grey);
            }
            if (key || (rnd() % 2) == 0) {
                cvCodebook(&f, 1, !key && (rnd() % 2) == 0, grey);
            }
            cvVectors(&f, key ? 0x30 + (int)(rnd() % 2) * 2 : 0x30 + (int)(rnd() % 3), (kW / 4) * (h / 4));
            cvSet24(&f, at + 1, (uint32_t)(f.n - at));
            y += h;
        }
        cvSet24(&f, 1, (uint32_t)f.n);
        in.biSizeImage = (DWORD)f.n;
        ICDecompress(hic, key ? 0 : ICDECOMPRESS_NOTKEYFRAME, &in, f.b, &out, a);
        cinepakDecode(&ours, f.b, (uint32_t)f.n, b, kStride);
        frames++;
        if (memcmp(a, b, sizeof a) == 0) {
            equal++;
            continue;
        }
        for (size_t k = 0; k < sizeof a; k++) {
            const int d = abs((int)a[k] - (int)b[k]);

            differentBytes += d != 0;
            maxDiff = SDL_max(maxDiff, d);
        }
        CHECK(0, "synthetic frame %d differs", i);
        if (getenv("BMX_CVID_DEBUG") != NULL) {
            int shown = 0;

            printf("  frame flags %02x, %d strips:", f.b[0], strips);
            for (int p = 10, s = 0; s < strips; s++) {
                const int size = (f.b[p + 1] << 16) | (f.b[p + 2] << 8) | f.b[p + 3];

                printf(" [h %d:", (f.b[p + 8] << 8) | f.b[p + 9]);
                for (int q = p + 12; q < p + size;) {
                    const int cs = (f.b[q + 1] << 16) | (f.b[q + 2] << 8) | f.b[q + 3];

                    printf(" %02x/%d", f.b[q], cs);
                    q += cs;
                }
                printf("]");
                p += size;
            }
            printf("\n    differing pixels per row block of 4 (top down):");
            for (int row = 0; row < kH; row += 4) {
                int count = 0;

                for (int r = row; r < row + 4; r++) {
                    for (int x = 0; x < kW; x++) {
                        count += memcmp(a + (kH - 1 - r) * kStride + x * 3,
                                        b + (kH - 1 - r) * kStride + x * 3, 3) != 0;
                    }
                }
                printf(" %d", count);
            }
            printf("\n");
            for (size_t k = 0; k < sizeof a && shown < 6; k += 3) {
                if (memcmp(a + k, b + k, 3) != 0) {
                    printf("    (%d, %d): iccvid %d %d %d ours %d %d %d\n", (int)(k / 3 % kW),
                           kH - 1 - (int)(k / 3 / kW), a[k], a[k + 1], a[k + 2], b[k], b[k + 1], b[k + 2]);
                    shown++;
                }
            }
        }
        memcpy(b, a, sizeof a); /* go on from iccvid's picture */
    }
    ICDecompressEnd(hic);
    ICClose(hic);
    printf("synthetic Cinepak frames against iccvid: %ld frames, %ld identical, %ld bytes differ, max "
           "difference "
           "%d\n",
           frames, equal, differentBytes, maxDiff);
}

static int runFiles(const char *folder)
{
    static const char *const kFiles[] = {"acclaim.avi", "amish.avi",   "credits.avi", "intro.avi",
                                         "mirra.avi",   "nyquist.avi", "zaxis.avi"};
    char missing[MAX_PATH];
    PAVIFILE rf = NULL;
    void *of = NULL;
    HRESULT rh;
    int32_t oh;

    compareSynthetic();
    AVIFileInit();
    bmxStandAVIFileInit();
    snprintf(missing, sizeof missing, "%s\\nothere.avi", folder);
    rh = AVIFileOpenA(&rf, missing, 0, NULL);
    oh = bmxStandAVIFileOpenA(&of, missing, 0, NULL);
    CHECK(rh == oh && (rf == NULL) == (of == NULL), "AVIFileOpenA(missing): real %lx ours %x",
          (unsigned long)rh, (unsigned)oh);
    for (size_t i = 0; i < sizeof kFiles / sizeof kFiles[0]; i++) {
        compareFile(folder, kFiles[i]);
    }
    AVIFileExit();
    bmxStandAVIFileExit();
    printf("%ld checks, %ld failures\n", checks, failures);
    return failures != 0;
}

/* ---- DrawDib against a readback of the native exe ---- */

static int runDrawDib(int argc, char **argv)
{
    const char *const file = argv[2];
    const int32_t frame = (int32_t)strtol(argv[3], NULL, 10);
    const char *const shotPath = argv[4];
    RECT rect = {0, 0, 640, 480};
    PAVIFILE f = NULL;
    PAVISTREAM s = NULL;
    PGETFRAME g;
    BITMAPINFOHEADER *bi;
    BITMAPINFO info;
    HDC dc;
    HBITMAP bitmap;
    uint8_t *pixels = NULL;
    HDRAWDIB dd;
    SDL_Surface *shot, *rgba;
    long different = 0, notDoubled = 0;
    int maxDiff = 0;

    if (argc >= 9) {
        rect.left = (int32_t)strtol(argv[5], NULL, 10);
        rect.top = (int32_t)strtol(argv[6], NULL, 10);
        rect.right = (int32_t)strtol(argv[7], NULL, 10);
        rect.bottom = (int32_t)strtol(argv[8], NULL, 10);
    }
    AVIFileInit();
    if (AVIFileOpenA(&f, file, 0, NULL) != 0 || AVIFileGetStream(f, &s, streamtypeVIDEO, 0) != 0) {
        printf("cannot open %s\n", file);
        return 2;
    }
    g = AVIStreamGetFrameOpen(s, NULL);
    bi = g != NULL ? AVIStreamGetFrame(g, frame) : NULL;
    if (bi == NULL) {
        printf("no frame %d\n", frame);
        return 2;
    }
    memset(&info, 0, sizeof info);
    info.bmiHeader.biSize = sizeof info.bmiHeader;
    info.bmiHeader.biWidth = 640;
    info.bmiHeader.biHeight = -480;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    dc = CreateCompatibleDC(NULL);
    bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, (void **)&pixels, NULL, 0);
    SelectObject(dc, bitmap);
    memset(pixels, 0, 640 * 480 * 4);
    dd = DrawDibOpen();
    /* zmovieDraw's call: the whole frame (source -1 x -1 = all of it) into the rectangle, flags 0. */
    if (!DrawDibDraw(dd, dc, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, bi, NULL, 0,
                     0, -1, -1, 0)) {
        printf("DrawDibDraw failed\n");
        return 2;
    }
    GdiFlush();
    shot = SDL_LoadBMP(shotPath);
    rgba = shot != NULL ? SDL_ConvertSurface(shot, SDL_PIXELFORMAT_XRGB8888) : NULL;
    if (rgba == NULL || rgba->w != 640 || rgba->h != 480) {
        printf("cannot read %s as 640x480: %s\n", shotPath, SDL_GetError());
        return 2;
    }
    for (int y = 0; y < 480; y++) {
        const uint8_t *const gdi = pixels + (size_t)y * 640 * 4;
        const uint8_t *const ours = (const uint8_t *)rgba->pixels + (size_t)y * (size_t)rgba->pitch;

        for (int x = 0; x < 640; x++) {
            int pixelDiffers = 0;

            for (int c = 0; c < 3; c++) {
                const int d = abs((int)gdi[x * 4 + c] - (int)ours[x * 4 + c]);

                pixelDiffers |= d != 0;
                maxDiff = SDL_max(maxDiff, d);
            }
            different += pixelDiffers;
            /* DrawDib's 2x against pixel doubling of the decoded frame (bottom-up BGR). */
            if (rect.left == 0 && rect.top == 0 && rect.right == 640 && rect.bottom == 480) {
                const uint8_t *const src =
                    (const uint8_t *)(bi + 1) + (size_t)(239 - y / 2) * 960 + (size_t)(x / 2) * 3;

                notDoubled += memcmp(src, gdi + x * 4, 3) != 0;
            }
        }
    }
    printf("DrawDibDraw of %s frame %d into (%ld, %ld, %ld, %ld) vs %s: %ld of %d pixels differ, max channel "
           "difference %d; DrawDib vs pixel doubling: %ld pixels differ\n",
           file, frame, rect.left, rect.top, rect.right, rect.bottom, shotPath, different, 640 * 480, maxDiff,
           notDoubled);
    SDL_DestroySurface(rgba);
    SDL_DestroySurface(shot);
    DrawDibClose(dd);
    AVIStreamGetFrameClose(g);
    AVIStreamRelease(s);
    AVIFileRelease(f);
    AVIFileExit();
    return different != 0;
}

int main(int argc, char **argv)
{
    if (argc >= 5 && strcmp(argv[1], "--drawdib") == 0) {
        return runDrawDib(argc, argv);
    }
    if (argc != 2) {
        printf("usage: bmxmovietest <movie folder> | --drawdib <file.avi> <frame> <shot.bmp> [l t r b]\n");
        return 2;
    }
    return runFiles(argv[1]);
}
