/*
 * Checks compat/x87soft.c's FSIN / FCOS / FPATAN register results against an independent MPFR model of the
 * same definition (docs/PORTING.md "X5 part 2 (portable x87)"): the operand reduced exactly by the
 * reduction's pi (66-bit Intel pi, or pi to 128 bits), the function evaluated at 400 bits, rounded once to
 * the register's 64 bits. Also counts how often a correctly rounded binary64 library (CORE-MATH, or MPFR at
 * 53 bits, which by definition give the same results) followed by the helper's own rounding would differ.
 *
 * Not part of the CMake build (needs MPFR):
 *   nix shell nixpkgs#mpfr.dev nixpkgs#gmp.dev -c sh -c 'cc -O2 -Isrc tests/platform/x87_mpfr.c \
 *     src/compat/x87soft.c -lmpfr -lgmp -o build/x87_mpfr' && build/x87_mpfr [count]
 * (with nix, add -I/-L for the mpfr / gmp store paths if the compiler does not find them).
 */
#include <mpfr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compat/x87soft.h"

#define PREC 400

static uint64_t gRng = 0x9e3779b97f4a7c15ull;

static uint64_t rnd64(void)
{
    gRng ^= gRng << 13;
    gRng ^= gRng >> 7;
    gRng ^= gRng << 17;
    return gRng;
}

static void regToMpfr(mpfr_t out, BmxX87Reg r)
{
    mpfr_set_ui_2exp(out, (unsigned long)(r.m >> 32), 32, MPFR_RNDN);
    mpfr_add_ui(out, out, (unsigned long)(uint32_t)r.m, MPFR_RNDN);
    mpfr_mul_2si(out, out, (long)(r.se & 0x7fff) - 16383 - 63, MPFR_RNDN);
    if (r.se & 0x8000) {
        mpfr_neg(out, out, MPFR_RNDN);
    }
}

/* The 64-bit register value of an MPFR number (finite, nonzero, normal range). */
static BmxX87Reg mpfrToReg(mpfr_t v)
{
    BmxX87Reg r = {0, 0};
    if (mpfr_zero_p(v)) {
        r.se = mpfr_signbit(v) ? 0x8000 : 0;
        return r;
    }
    mpfr_t t;
    mpfr_init2(t, 64);
    mpfr_set(t, v, MPFR_RNDN);
    const long e = mpfr_get_exp(t); /* t = 0.1xxx * 2^e */
    mpfr_abs(t, t, MPFR_RNDN);
    mpfr_mul_2si(t, t, 64 - e, MPFR_RNDN);
    mpz_t z;
    mpz_init(z);
    mpfr_get_z(z, t, MPFR_RNDN);
    uint64_t m = 0;
    mpz_export(&m, NULL, -1, 8, 0, 0, z);
    r.m = m;
    r.se = (uint16_t)((mpfr_signbit(v) ? 0x8000 : 0) | (e - 1 + 16383));
    mpz_clear(z);
    mpfr_clear(t);
    return r;
}

/* sin / cos / tan of x by the model: x = k * P/2 + r exactly, then the function of r. */
static void model(mpfr_t out, BmxX87Reg x, int fn, int precisePi)
{
    mpfr_t X, P, K, R, s;
    mpfr_inits2(PREC, X, P, K, R, s, (mpfr_ptr)0);
    regToMpfr(X, x);
    if (precisePi) {
        mpfr_set_str(P, "c90fdaa22168c234c4c6628b80dc1cd1", 16, MPFR_RNDN);
        mpfr_mul_2si(P, P, -126, MPFR_RNDN);
    } else {
        mpfr_set_str(P, "3243f6a8885a308d3", 16, MPFR_RNDN);
        mpfr_mul_2si(P, P, -64, MPFR_RNDN);
    }
    mpfr_div_2ui(P, P, 1, MPFR_RNDN);
    mpfr_div(K, X, P, MPFR_RNDN);
    mpfr_rint(K, K, MPFR_RNDN);
    mpfr_mul(s, K, P, MPFR_RNDN);
    mpfr_sub(R, X, s, MPFR_RNDN);
    /* (with |x| < 2^63 and these widths both steps are exact) */
    const long q = (long)(mpfr_get_si(K, MPFR_RNDN) & 3);
    if (fn == 0) {
        if (q & 1) {
            mpfr_cos(out, R, MPFR_RNDN);
        } else {
            mpfr_sin(out, R, MPFR_RNDN);
        }
        if (q & 2) {
            mpfr_neg(out, out, MPFR_RNDN);
        }
    } else if (fn == 1) {
        if (q & 1) {
            mpfr_sin(out, R, MPFR_RNDN);
        } else {
            mpfr_cos(out, R, MPFR_RNDN);
        }
        if (q == 1 || q == 2) {
            mpfr_neg(out, out, MPFR_RNDN);
        }
    } else {
        if (q & 1) {
            mpfr_cot(out, R, MPFR_RNDN);
            mpfr_neg(out, out, MPFR_RNDN);
        } else {
            mpfr_tan(out, R, MPFR_RNDN);
        }
    }
    mpfr_clears(X, P, K, R, s, (mpfr_ptr)0);
}

static BmxX87Reg randomReg(int minExp, int maxExp)
{
    BmxX87Reg r;
    r.m = rnd64() | 0x8000000000000000ull;
    const int e = minExp + (int)(rnd64() % (uint64_t)(maxExp - minExp + 1));
    r.se = (uint16_t)((e + 16383) | ((rnd64() & 1) ? 0x8000 : 0));
    return r;
}

static int same(BmxX87Reg a, BmxX87Reg b)
{
    return a.m == b.m && a.se == b.se;
}

static long gFails;

static void report(const char *what, BmxX87Reg x, BmxX87Reg got, BmxX87Reg want)
{
    if (gFails++ < 20) {
        printf("FAIL %s x=%04x:%016llx got %04x:%016llx want %04x:%016llx\n", what, x.se,
               (unsigned long long)x.m, got.se, (unsigned long long)got.m, want.se,
               (unsigned long long)want.m);
    }
}

static void checkTrig(BmxX87Reg x, int precisePi)
{
    mpfr_t v;
    mpfr_init2(v, PREC);
    model(v, x, 0, precisePi);
    BmxX87Reg want = mpfrToReg(v), got = bmxX87SoftFsinReg(x);
    if (!same(got, want)) {
        report("fsin", x, got, want);
    }
    model(v, x, 1, precisePi);
    want = mpfrToReg(v);
    got = bmxX87SoftFcosReg(x);
    if (!same(got, want)) {
        report("fcos", x, got, want);
    }
    mpfr_clear(v);
}

static void checkAtan(BmxX87Reg y, BmxX87Reg x)
{
    mpfr_t Y, X, v;
    mpfr_inits2(PREC, Y, X, v, (mpfr_ptr)0);
    regToMpfr(Y, y);
    regToMpfr(X, x);
    mpfr_atan2(v, Y, X, MPFR_RNDN);
    const BmxX87Reg want = mpfrToReg(v), got = bmxX87SoftFpatanReg(y, x);
    if (!same(got, want)) {
        report("fpatan", y, got, want);
    }
    mpfr_clears(Y, X, v, (mpfr_ptr)0);
}

/* A correctly rounded library result (CORE-MATH's, by definition the same as MPFR's) as double / float. */
static double crD(int (*f)(mpfr_ptr, mpfr_srcptr, mpfr_rnd_t), double a)
{
    mpfr_t X, v;
    mpfr_init2(X, 64);
    mpfr_init2(v, 53);
    mpfr_set_d(X, a, MPFR_RNDN);
    f(v, X, MPFR_RNDN);
    const double r = mpfr_get_d(v, MPFR_RNDN);
    mpfr_clears(X, v, (mpfr_ptr)0);
    return r;
}

static float crF(int (*f)(mpfr_ptr, mpfr_srcptr, mpfr_rnd_t), double a)
{
    mpfr_t X, v;
    mpfr_init2(X, 64);
    mpfr_init2(v, 24);
    mpfr_set_d(X, a, MPFR_RNDN);
    f(v, X, MPFR_RNDN);
    const float r = mpfr_get_flt(v, MPFR_RNDN);
    mpfr_clears(X, v, (mpfr_ptr)0);
    return r;
}

static float randomFloat(void)
{
    const uint32_t bits = (uint32_t)rnd64();
    float f;
    memcpy(&f, &bits, 4);
    return f;
}

/*
 * Per helper and real domain: how often "correctly rounded binary64 / binary32 library, then the helper's own
 * rounding steps" differs from the x87 model, and how often the 66-bit and 128-bit pi models differ.
 */
static void libraryGap(long count)
{
    long n = 0, dSin = 0, dCos = 0, dSinSmall = 0, nSmall = 0, dPi = 0;
    for (long i = 0; i < count; i++) {
        /* RwMatrixRotate: (double)(pi/180 as float) * angle */
        const float angle = randomFloat();
        const double a = (double)0x1.1df46ap-6f * angle;
        if (!(a < 0x1p63 && a > -0x1p63)) {
            continue;
        }
        n++;
        const double s = bmxX87SoftFsin(a), c = bmxX87SoftFcos(a);
        dSin += crD(mpfr_sin, a) != s;
        dCos += crD(mpfr_cos, a) != c;
        if (a < 0.78 && a > -0.78) {
            nSmall++;
            dSinSmall += crD(mpfr_sin, a) != s;
        }
        bmxX87SoftSetReductionPi(1);
        dPi += bmxX87SoftFsin(a) != s || bmxX87SoftFcos(a) != c;
        bmxX87SoftSetReductionPi(0);
    }
    printf(
        "Fsin/Fcos, RwMatrixRotate domain (%ld floats): CR sin differs %ld, CR cos differs %ld; |a| < pi/4: "
        "%ld of %ld; 66-bit vs 128-bit pi differ %ld\n",
        n, dSin, dCos, dSinSmall, nSmall, dPi);
    /* the slerp: Fsin of a float angle in [0, pi/2] */
    long dSl = 0;
    for (long i = 0; i < count; i++) {
        const float angle = (float)((double)(rnd64() >> 11) * 0x1p-53 * 1.5707963267948966);
        dSl += crD(mpfr_sin, angle) != bmxX87SoftFsin(angle);
    }
    printf("Fsin, slerp angles in [0, pi/2] (%ld): CR sin differs %ld\n", count, dSl);
    /* TanFloat over random floats below 2^63 */
    long nT = 0, dT = 0, dT2 = 0;
    for (long i = 0; i < count; i++) {
        const float f = randomFloat();
        if (!(f < 0x1p63f && f > -0x1p63f)) {
            continue;
        }
        nT++;
        const float t = bmxX87SoftTanFloat(f);
        dT += crF(mpfr_tan, f) != t && !(t != t);
        dT2 += (float)crD(mpfr_tan, f) != t && !(t != t);
    }
    printf("TanFloat (%ld floats): CR tanf differs %ld, CR tan then (float) differs %ld\n", nT, dT, dT2);
    /* AngleOfCosine: c in [0, 1] */
    long dA = 0;
    for (long i = 0; i < count; i++) {
        const double cosine = (double)(rnd64() >> 11) * 0x1p-53;
        const float want = bmxX87SoftAngleOfCosine(cosine);
        dA += (float)crD(mpfr_acos, cosine) != want;
    }
    printf("AngleOfCosine (%ld cosines in [0, 1]): CR acos then (float) differs %ld\n", count, dA);
    /* the finite tables */
    long dS = 0;
    for (int32_t k = -32768; k < 32768; k++) {
        const double x = (double)k * (double)0x1.921fb6p-14f;
        dS += crF(mpfr_sin, x) != bmxX87SoftSinScaled(k, 0x1.921fb6p-14f);
    }
    printf("SinScaled (65536 table angles): CR sinf differs %ld\n", dS);
    long dC = 0, dCt = 0;
    for (int32_t k = 0; k < 4096; k++) {
        const double x = (double)k * 0x1.921fb54411744p-10;
        const double lib = crD(mpfr_cos, x), soft = bmxX87SoftCosScaled(k, 0x1.921fb54411744p-10);
        dC += lib != soft;
        dCt += (long)((0.5 - lib * 0.5) * 4096.0) != (long)((0.5 - soft * 0.5) * 4096.0);
    }
    printf("CosScaled (4096 LFO entries): CR cos differs %ld, table entries differ %ld\n", dC, dCt);
    long dAs = 0, dAsT = 0, dAt = 0, dAtT = 0;
    for (int32_t k = 1; k < 0x1fff; k++) {
        const double x = (double)(k - 0x1000) * 0x1p-12;
        const double scale = (double)0x1.45f306p+13f;
        const double libAs = crD(mpfr_asin, x) * scale,
                     softAs = bmxX87SoftAsinScaled(k - 0x1000, 0x1p-12, 0x1.45f306p+13f);
        dAs += libAs != softAs;
        dAsT += (int16_t)(int32_t)libAs != (int16_t)(int32_t)softAs;
        const double libAt = crD(mpfr_atan, x) * scale,
                     softAt = bmxX87SoftAtanScaled(k - 0x1000, 0x1p-12, 0x1.45f306p+13f);
        dAt += libAt != softAt;
        dAtT += (int16_t)(int32_t)libAt != (int16_t)(int32_t)softAt;
    }
    printf("AsinScaled / AtanScaled (8190 entries each): CR then * scale differs %ld / %ld, int16 entries "
           "differ %ld / "
           "%ld\n",
           dAs, dAt, dAsT, dAtT);
}

int main(int argc, char **argv)
{
    const long count = argc > 1 ? atol(argv[1]) : 20000;
    for (int precise = 0; precise < 2; precise++) {
        bmxX87SoftSetReductionPi(precise);
        long before = gFails;
        for (long i = 0; i < count; i++) {
            checkTrig(randomReg(-80, 62), precise);
            checkTrig(randomReg(-2, 6), precise);
        }
        /* next to multiples of pi/2, where the reduction's pi decides the result */
        for (long k = 1; k < 2000; k++) {
            mpfr_t t;
            mpfr_init2(t, 64);
            mpfr_const_pi(t, MPFR_RNDN);
            mpfr_mul_si(t, t, k, MPFR_RNDN);
            mpfr_div_2ui(t, t, 1, MPFR_RNDN);
            BmxX87Reg x = mpfrToReg(t);
            for (int d = -3; d <= 3; d++) {
                BmxX87Reg y = x;
                y.m += (uint64_t)(int64_t)d;
                checkTrig(y, precise);
            }
            mpfr_clear(t);
        }
        printf("fsin/fcos (%s pi): %ld failures\n", precise ? "128-bit" : "66-bit", gFails - before);
    }
    bmxX87SoftSetReductionPi(0);
    long before = gFails;
    for (long i = 0; i < count; i++) {
        BmxX87Reg y = randomReg(-70, 70), x = randomReg(-70, 70);
        checkAtan(y, x);
        y = randomReg(-3, 1);
        x = randomReg(-3, 1);
        checkAtan(y, x);
    }
    printf("fpatan: %ld failures\n", gFails - before);
    libraryGap(count * 4);
    return gFails != 0;
}
