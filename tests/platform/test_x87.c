/*
 * The portable x87 (compat/x87soft.c) against the real instructions (compat/x87.h's bmxX87Hw*),
 * docs/PORTING.md "X5 part 2 (portable x87)". On x86 every case runs both and counts the differences;
 * elsewhere (arm64) only the soft side runs. Each domain also prints a hash of its results: the soft hash on
 * arm64 must equal the hw hash printed on x86.
 *
 *   bmxx87test                       finite domains exhaustively, the others on random + edge inputs
 *   bmxx87test --random N            N random cases per continuous domain (default 2000000)
 *   bmxx87test --all-floats NAME     every one of the 2^32 floats through one helper (fsin, fcos, rotate,
 *                                    tan, acos); --threads N (default 8)
 * Exit status 0 when nothing differs.
 */
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "compat/x87.h"

#if defined(__i386__) || defined(__x86_64__)
#define HAVE_HW 1
#else
#define HAVE_HW 0
#endif

#define CW_DEFAULT 0x027f /* Windows: 53 bit, nearest */
#define CW_CAMERA  0x047f /* the camera update: 24 bit, down */

static const uint16_t kAllCws[] = {0x007f, 0x047f, 0x087f, 0x0c7f, 0x027f, 0x067f,
                                   0x0a7f, 0x0e7f, 0x037f, 0x077f, 0x0b7f, 0x0f7f};

/* ---- results, hashes, counts ---- */

typedef struct Domain {
    const char *name;
    uint64_t cases, diffs, hwHash, softHash;
    int shown;
} Domain;

static void hashIn(uint64_t *h, const void *p, size_t n)
{
    const unsigned char *b = p;
    for (size_t i = 0; i < n; i++) {
        *h = (*h ^ b[i]) * 0x100000001b3ull;
    }
}

static void domainStart(Domain *d, const char *name)
{
    memset(d, 0, sizeof *d);
    d->name = name;
    d->hwHash = d->softHash = 0xcbf29ce484222325ull;
}

/* One case: the input (for the report), the hw and soft result bytes. */
static void domainCase(Domain *d, const void *in, size_t inSize, const void *hw, const void *soft,
                       size_t size)
{
    d->cases++;
    hashIn(&d->softHash, soft, size);
#if HAVE_HW
    hashIn(&d->hwHash, hw, size);
    if (memcmp(hw, soft, size) != 0) {
        d->diffs++;
        if (d->shown++ < 8) {
            printf("  %s differs: in", d->name);
            for (size_t i = 0; i < inSize; i++) {
                printf("%s%02x", i % 4 == 0 ? " " : "", ((const unsigned char *)in)[i]);
            }
            printf(" hw");
            for (size_t i = 0; i < size; i++) {
                printf("%s%02x", i % 4 == 0 ? " " : "", ((const unsigned char *)hw)[i]);
            }
            printf(" soft");
            for (size_t i = 0; i < size; i++) {
                printf("%s%02x", i % 4 == 0 ? " " : "", ((const unsigned char *)soft)[i]);
            }
            printf("\n");
        }
    }
#else
    (void)in;
    (void)inSize;
    (void)hw;
#endif
}

static int gFailed;

static void domainEnd(const Domain *d)
{
#if HAVE_HW
    printf("%-34s %12" PRIu64 " cases %10" PRIu64 " differ   hw %016" PRIx64 " soft %016" PRIx64 "\n",
           d->name, d->cases, d->diffs, d->hwHash, d->softHash);
    gFailed |= d->diffs != 0;
#else
    printf("%-34s %12" PRIu64 " cases   soft %016" PRIx64 "\n", d->name, d->cases, d->softHash);
#endif
}

/* ---- inputs ---- */

static uint64_t gRng = 0x2545f4914f6cdd1dull;

static uint64_t rnd64(void)
{
    gRng ^= gRng << 13;
    gRng ^= gRng >> 7;
    gRng ^= gRng << 17;
    return gRng;
}

static float floatOf(uint32_t b)
{
    float f;
    memcpy(&f, &b, 4);
    return f;
}

static double doubleOf(uint64_t b)
{
    double d;
    memcpy(&d, &b, 8);
    return d;
}

static const uint32_t kEdgeFloats[] = {0x00000000, 0x80000000, 0x7f800000, 0xff800000, 0x7fc00000, 0xffc00000,
                                       0x7fa00000, 0xff812345, 0x00000001, 0x80000001, 0x007fffff, 0x00800000,
                                       0x7f7fffff, 0xff7fffff, 0x3f800000, 0xbf800000, 0x3fc90fdb, 0x40490fdb,
                                       0x5f000000, 0xdf000000, 0x5effffff, 0x33800000, 0x2f800000, 0x3f7fffff,
                                       0x3f800001, 0x4b000000};

static const uint64_t kEdgeDoubles[] = {
    0x0000000000000000ull, 0x8000000000000000ull, 0x7ff0000000000000ull, 0xfff0000000000000ull,
    0x7ff8000000000000ull, 0xfff8000000000000ull, 0x7ff4000000000000ull, 0x0000000000000001ull,
    0x800fffffffffffffull, 0x0010000000000000ull, 0x7fefffffffffffffull, 0x3ff0000000000000ull,
    0xbff0000000000000ull, 0x3ff921fb54442d18ull, 0x400921fb54442d18ull, 0x401921fb54442d18ull,
    0x43e0000000000000ull, 0xc3e0000000000000ull, 0x43dfffffffffffffull, 0x3e40000000000000ull,
    0x3fefffffffffffffull, 0x3ff0000000000001ull, 0x3c90000000000000ull};

/* A float: mostly ordinary magnitudes, some raw bit patterns (NaNs, denormals, huge). */
static float randomFloat(void)
{
    const uint64_t r = rnd64();
    if ((r & 7) == 0) {
        return floatOf((uint32_t)(r >> 32));
    }
    const int e = (int)((r >> 3) % 60) - 30;
    return floatOf((uint32_t)(((r >> 32) & 0x807fffff) | (uint32_t)(127 + e) << 23));
}

static double randomDouble(void)
{
    const uint64_t r = rnd64();
    if ((r & 7) == 0) {
        return doubleOf(rnd64());
    }
    const int e = (int)((r >> 3) % 80) - 40;
    return doubleOf((rnd64() & 0x800fffffffffffffull) | (uint64_t)(1023 + e) << 52);
}

/* ---- the comparisons ---- */

#if HAVE_HW
#define HW(expr) (expr)
#else
#define HW(expr) 0
#endif

static void setCw(uint16_t cw)
{
    bmxX87SetControlWord(cw);
}

static void tablesExhaustive(uint16_t cw, const char *suffix)
{
    char name[64];
    Domain d;
    setCw(cw);
    snprintf(name, sizeof name, "SinScaled table%s", suffix);
    domainStart(&d, name);
    for (int32_t n = -32768; n < 32768; n++) {
        const float hw = HW(bmxX87HwSinScaled(n, 0x1.921fb6p-14f)),
                    soft = bmxX87SoftSinScaled(n, 0x1.921fb6p-14f);
        domainCase(&d, &n, 4, &hw, &soft, 4);
    }
    domainEnd(&d);
    snprintf(name, sizeof name, "CosScaled LFO table%s", suffix);
    domainStart(&d, name);
    for (int32_t n = 0; n < 4096; n++) {
        const double hw = HW(bmxX87HwCosScaled(n, 0x1.921fb54411744p-10)),
                     soft = bmxX87SoftCosScaled(n, 0x1.921fb54411744p-10);
        domainCase(&d, &n, 4, &hw, &soft, 8);
    }
    domainEnd(&d);
    snprintf(name, sizeof name, "AsinScaled table%s", suffix);
    domainStart(&d, name);
    for (int32_t n = -0xfff; n <= 0xffe; n++) {
        const double hw = HW(bmxX87HwAsinScaled(n, 0x1p-12, 0x1.45f306p+13f)),
                     soft = bmxX87SoftAsinScaled(n, 0x1p-12, 0x1.45f306p+13f);
        domainCase(&d, &n, 4, &hw, &soft, 8);
    }
    domainEnd(&d);
    snprintf(name, sizeof name, "AtanScaled table%s", suffix);
    domainStart(&d, name);
    for (int32_t n = -0x1000; n <= 0xfff; n++) {
        const double hw = HW(bmxX87HwAtanScaled(n, 0x1p-12, 0x1.45f306p+13f)),
                     soft = bmxX87SoftAtanScaled(n, 0x1p-12, 0x1.45f306p+13f);
        domainCase(&d, &n, 4, &hw, &soft, 8);
    }
    domainEnd(&d);
    snprintf(name, sizeof name, "HalfPiScaled%s", suffix);
    domainStart(&d, name);
    for (int neg = 0; neg < 2; neg++) {
        const double hw = HW(bmxX87HwHalfPiScaled(neg, 0x1.45f306p+13f)),
                     soft = bmxX87SoftHalfPiScaled(neg, 0x1.45f306p+13f);
        domainCase(&d, &neg, 4, &hw, &soft, 8);
    }
    domainEnd(&d);
    setCw(CW_DEFAULT);
}

static void continuousRandom(long count)
{
    Domain d;
    setCw(CW_DEFAULT);

    domainStart(&d, "Fsin/Fcos RwMatrixRotate");
    for (long i = 0; i < count + (long)(sizeof kEdgeFloats / 4); i++) {
        const float angle = i < count ? randomFloat() : floatOf(kEdgeFloats[i - count]);
        const double a = (double)0x1.1df46ap-6f * angle;
        const double hw[2] = {HW(bmxX87HwFsin(a)), HW(bmxX87HwFcos(a))};
        const double soft[2] = {bmxX87SoftFsin(a), bmxX87SoftFcos(a)};
        domainCase(&d, &a, 8, hw, soft, 16);
    }
    domainEnd(&d);

    domainStart(&d, "Fsin slerp/anim [0, pi/2]");
    for (long i = 0; i < count; i++) {
        const double a = (double)(rnd64() >> 11) * 0x1p-53 * 1.5707963705062866;
        const double hw = HW(bmxX87HwFsin(a)), soft = bmxX87SoftFsin(a);
        domainCase(&d, &a, 8, &hw, &soft, 8);
    }
    domainEnd(&d);

    domainStart(&d, "Fsin/Fcos any double");
    for (long i = 0; i < count + (long)(sizeof kEdgeDoubles / 8); i++) {
        const double a = i < count ? randomDouble() : doubleOf(kEdgeDoubles[i - count]);
        const double hw[2] = {HW(bmxX87HwFsin(a)), HW(bmxX87HwFcos(a))};
        const double soft[2] = {bmxX87SoftFsin(a), bmxX87SoftFcos(a)};
        domainCase(&d, &a, 8, hw, soft, 16);
    }
    domainEnd(&d);

    domainStart(&d, "AngleOfCosine [0, 1]");
    for (long i = 0; i < count; i++) {
        const double c =
            (i & 1) ? (double)(rnd64() >> 11) * 0x1p-53 : 1.0 - (double)(rnd64() >> 11) * 0x1p-70;
        const float hw = HW(bmxX87HwAngleOfCosine(c)), soft = bmxX87SoftAngleOfCosine(c);
        domainCase(&d, &c, 8, &hw, &soft, 4);
    }
    domainEnd(&d);

    domainStart(&d, "AngleOfCosine any double");
    for (long i = 0; i < count / 4 + (long)(sizeof kEdgeDoubles / 8); i++) {
        const double c = i < count / 4 ? randomDouble() : doubleOf(kEdgeDoubles[i - count / 4]);
        const float hw = HW(bmxX87HwAngleOfCosine(c)), soft = bmxX87SoftAngleOfCosine(c);
        domainCase(&d, &c, 8, &hw, &soft, 4);
    }
    domainEnd(&d);

    domainStart(&d, "TanFloat");
    for (long i = 0; i < count + (long)(sizeof kEdgeFloats / 4); i++) {
        const float a = i < count ? randomFloat() : floatOf(kEdgeFloats[i - count]);
        const float hw = HW(bmxX87HwTanFloat(a)), soft = bmxX87SoftTanFloat(a);
        domainCase(&d, &a, 4, &hw, &soft, 4);
    }
    domainEnd(&d);

    /* the arithmetic helpers under every precision / rounding control */
    for (int pass = 0; pass < 2; pass++) {
        domainStart(&d, pass ? "Rotate/MatrixMultiply all CWs" : "Rotate/MatrixMultiply default+camera");
        for (long i = 0; i < count / 8; i++) {
            const uint16_t cw = pass ? kAllCws[i % 12] : ((i & 1) ? CW_CAMERA : CW_DEFAULT);
            float in[32];
            for (int k = 0; k < 32; k++) {
                in[k] = (rnd64() & 63) == 0 ? floatOf(kEdgeFloats[rnd64() % (sizeof kEdgeFloats / 4)])
                                            : randomFloat();
            }
            float hw[18] = {0}, soft[18] = {0};
            setCw(cw);
#if HAVE_HW
            bmxX87HwRotate(&hw[0], &hw[1], in[0], in[1], &in[2], &in[3]);
            bmxX87HwMatrixMultiply(&hw[2], in, in + 16);
#endif
            bmxX87SoftRotate(&soft[0], &soft[1], in[0], in[1], &in[2], &in[3]);
            bmxX87SoftMatrixMultiply(&soft[2], in, in + 16);
            setCw(CW_DEFAULT);
            domainCase(&d, in, 16, hw, soft, sizeof hw);
        }
        domainEnd(&d);
    }

    domainStart(&d, "HalfPi/Asin/AtanScaled any scale");
    for (long i = 0; i < count / 8; i++) {
        const float scale = randomFloat();
        const double step = randomDouble();
        const int32_t n = (int32_t)rnd64(), m = (int32_t)(rnd64() % 8191) - 4095;
        setCw(kAllCws[i % 12]);
        const double hw[3] = {HW(bmxX87HwHalfPiScaled((int)(i & 1), scale)),
                              HW(bmxX87HwAtanScaled(n, step, scale)),
                              HW(bmxX87HwAsinScaled(m, 0x1p-12, scale))};
        const double soft[3] = {bmxX87SoftHalfPiScaled((int)(i & 1), scale),
                                bmxX87SoftAtanScaled(n, step, scale),
                                bmxX87SoftAsinScaled(m, 0x1p-12, scale)};
        setCw(CW_DEFAULT);
        domainCase(&d, &scale, 4, hw, soft, 24);
    }
    domainEnd(&d);
}

/* ---- every float ---- */

typedef struct AllFloats {
    int helper; /* 0 fsin, 1 fcos, 2 rotate domain, 3 tan, 4 acos */
    uint32_t from, to;
    uint64_t diffs, hwHash, softHash;
} AllFloats;

static void *allFloatsRun(void *arg)
{
    AllFloats *w = arg;
    uint64_t hwHash = 0xcbf29ce484222325ull, softHash = hwHash, diffs = 0;
    uint32_t b = w->from;
    setCw(CW_DEFAULT);
    for (;;) {
        const float f = floatOf(b);
        uint64_t hw = 0, soft = 0;
        switch (w->helper) {
        case 0: {
            const double h = HW(bmxX87HwFsin(f)), s = bmxX87SoftFsin(f);
            memcpy(&hw, &h, 8);
            memcpy(&soft, &s, 8);
            break;
        }
        case 1: {
            const double h = HW(bmxX87HwFcos(f)), s = bmxX87SoftFcos(f);
            memcpy(&hw, &h, 8);
            memcpy(&soft, &s, 8);
            break;
        }
        case 2: {
            /* both of RwMatrixRotate's results folded into one word */
            const double a = (double)0x1.1df46ap-6f * f;
            const double h[2] = {HW(bmxX87HwFsin(a)), HW(bmxX87HwFcos(a))};
            const double s[2] = {bmxX87SoftFsin(a), bmxX87SoftFcos(a)};
            uint64_t t[2];
            memcpy(t, h, 16);
            hw = t[0] ^ t[1] * 0x9e3779b97f4a7c15ull;
            memcpy(t, s, 16);
            soft = t[0] ^ t[1] * 0x9e3779b97f4a7c15ull;
            break;
        }
        case 3: {
            const float h = HW(bmxX87HwTanFloat(f)), s = bmxX87SoftTanFloat(f);
            memcpy(&hw, &h, 4);
            memcpy(&soft, &s, 4);
            break;
        }
        default: {
            const float h = HW(bmxX87HwAngleOfCosine(f)), s = bmxX87SoftAngleOfCosine(f);
            memcpy(&hw, &h, 4);
            memcpy(&soft, &s, 4);
            break;
        }
        }
        hashIn(&softHash, &soft, 8);
#if HAVE_HW
        hashIn(&hwHash, &hw, 8);
        if (hw != soft) {
            if (diffs < 4) {
                printf("  differs: in %08" PRIx32 " hw %016" PRIx64 " soft %016" PRIx64 "\n", b, hw, soft);
            }
            diffs++;
        }
#endif
        if (b == w->to) {
            break;
        }
        b++;
    }
    w->diffs = diffs;
    w->hwHash = hwHash;
    w->softHash = softHash;
    return NULL;
}

static int allFloats(const char *name, int threads)
{
    static const char *const kNames[] = {"fsin", "fcos", "rotate", "tan", "acos"};
    int helper = -1;
    for (int i = 0; i < 5; i++) {
        if (strcmp(name, kNames[i]) == 0) {
            helper = i;
        }
    }
    if (helper < 0 || threads < 1 || threads > 256) {
        fprintf(stderr, "--all-floats fsin|fcos|rotate|tan|acos [--threads 1..256]\n");
        return 2;
    }
    /* A sample first, for the time estimate. */
    AllFloats probe = {helper, 0x3f000000, 0x3f000000 + 200000, 0, 0, 0};
    const clock_t t0 = clock();
    allFloatsRun(&probe);
    const double perCase = (double)(clock() - t0) / CLOCKS_PER_SEC / 200001.0;
    printf("%s: 2^32 floats, ~%.0f s on %d threads (estimate)\n", name, perCase * 4294967296.0 / threads,
           threads);
    fflush(stdout);
    AllFloats *work = calloc((size_t)threads, sizeof *work);
    pthread_t *ids = calloc((size_t)threads, sizeof *ids);
    const uint64_t chunk = 0x100000000ull / (uint64_t)threads;
    for (int i = 0; i < threads; i++) {
        work[i].helper = helper;
        work[i].from = (uint32_t)(chunk * (uint64_t)i);
        work[i].to = i == threads - 1 ? 0xffffffffu : (uint32_t)(chunk * (uint64_t)(i + 1) - 1);
        pthread_create(&ids[i], NULL, allFloatsRun, &work[i]);
    }
    uint64_t diffs = 0, hwHash = 0, softHash = 0;
    for (int i = 0; i < threads; i++) {
        pthread_join(ids[i], NULL);
        diffs += work[i].diffs;
        hwHash = hwHash * 31 + work[i].hwHash;
        softHash = softHash * 31 + work[i].softHash;
    }
#if HAVE_HW
    printf("%s: %" PRIu64 " differ   hw %016" PRIx64 " soft %016" PRIx64 " (hashes depend on --threads)\n",
           name, diffs, hwHash, softHash);
#else
    printf("%s: soft %016" PRIx64 " (hash depends on --threads)\n", name, softHash);
#endif
    free(work);
    free(ids);
    return diffs != 0;
}

int main(int argc, char **argv)
{
    long count = 2000000;
    int threads = 8;
    const char *all = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--random") == 0 && i + 1 < argc) {
            count = atol(argv[++i]);
        } else if (strcmp(argv[i], "--all-floats") == 0 && i + 1 < argc) {
            all = argv[++i];
        } else if (strcmp(argv[i], "--threads") == 0 && i + 1 < argc) {
            threads = atoi(argv[++i]);
        } else {
            fprintf(stderr, "usage: %s [--random N] [--all-floats NAME [--threads N]]\n", argv[0]);
            return 2;
        }
    }
    printf("x87 %s, reduction pi: 66-bit (Intel)\n",
           HAVE_HW ? "hardware vs soft" : "soft only (no x87 here)");
    if (all != NULL) {
        return allFloats(all, threads);
    }
    tablesExhaustive(CW_DEFAULT, "");
    tablesExhaustive(CW_CAMERA, " (camera cw)");
    continuousRandom(count);
#if HAVE_HW
    /* Which pi does this CPU reduce with? Count differences with the 128-bit one too. */
    bmxX87SoftSetReductionPi(1);
    printf("-- the same with pi to 128 bits (expect many differences on Intel):\n");
    tablesExhaustive(CW_DEFAULT, "");
    const int failed66 = gFailed;
    continuousRandom(count / 10);
    gFailed = failed66;
    bmxX87SoftSetReductionPi(0);
#endif
    printf("%s\n", gFailed   ? "DIFFERENCES"
                   : HAVE_HW ? "all equal"
                             : "done (compare the hashes with an x86 run)");
    return gFailed;
}
