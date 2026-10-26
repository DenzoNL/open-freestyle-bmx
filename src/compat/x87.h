/*
 * x87 transcendental instructions the original uses for its lookup tables (FSIN, FPATAN, and the CRT's
 * asin built on FPATAN). Plain C cannot reproduce them bit for bit, so the hook build runs the real
 * instructions; every helper covers a whole FILD..FSTP sequence so no intermediate is rounded to a
 * narrower type than in the original.
 */
#ifndef BMX_COMPAT_X87_H
#define BMX_COMPAT_X87_H

#include <stdint.h>

#if defined(BMX_HOOK)

/* (float)fsin((double)n * k). */
static inline float bmxX87SinScaled(int32_t n, float k)
{
    float r;
    __asm__("fildl %1\n\tfmuls %2\n\tfsin\n\tfstps %0" : "=m"(r) : "m"(n), "m"(k) : "st");
    return r;
}

/* fcos((double)n * k), rounded to double. */
static inline double bmxX87CosScaled(int32_t n, double k)
{
    double r;
    __asm__("fildl %1\n\tfmull %2\n\tfcos\n\tfstpl %0" : "=m"(r) : "m"(n), "m"(k) : "st");
    return r;
}

/* asin(n * step) * scale as the CRT computes it for |n * step| < 1: x / sqrt((1 + x) * (1 - x)) via FPATAN.
 */
static inline double bmxX87AsinScaled(int32_t n, double step, float scale)
{
    double r;
    __asm__("fildl %1\n\tfmull %2\n\tfld1\n\tfadd %%st(1), %%st\n\tfld1\n\tfsub %%st(2), %%st\n\t"
            "fmulp %%st, %%st(1)\n\tfsqrt\n\tfpatan\n\tfmuls %3\n\tfstpl %0"
            : "=m"(r)
            : "m"(n), "m"(step), "m"(scale)
            : "st", "st(1)", "st(2)");
    return r;
}

/* The CRT's asin(+-1): +-pi/2 loaded as an 80 bit constant, times scale. */
static inline double bmxX87HalfPiScaled(int negative, float scale)
{
    static const uint8_t halfPi[10] = {0x35, 0xc2, 0x68, 0x21, 0xa2, 0xda, 0x0f, 0xc9, 0xff, 0x3f};
    double r;
    if (negative) { // NOLINT(bugprone-branch-clone)
        __asm__("fldt %1\n\tfchs\n\tfmuls %2\n\tfstpl %0" : "=m"(r) : "m"(halfPi), "m"(scale) : "st");
    } else {
        __asm__("fldt %1\n\tfmuls %2\n\tfstpl %0" : "=m"(r) : "m"(halfPi), "m"(scale) : "st");
    }
    return r;
}

/* atan(n * step) * scale. */
static inline double bmxX87AtanScaled(int32_t n, double step, float scale)
{
    double r;
    __asm__("fildl %1\n\tfmull %2\n\tfld1\n\tfpatan\n\tfmuls %3\n\tfstpl %0"
            : "=m"(r)
            : "m"(n), "m"(step), "m"(scale)
            : "st", "st(1)");
    return r;
}

/*
 * One 2D rotation as the original's FPU sequence computes it: *r1 = c*a - s*b, *r2 = c*b + s*a with
 * 53 bit intermediates. Run on the real instructions so NaN operands propagate exactly like the original
 * (x87 keeps the NaN with the larger significand, SSE keeps the first operand).
 */
// NOLINTBEGIN(readability-non-const-parameter)
static inline void bmxX87Rotate(float *r1, float *r2, float a, float b, const float *c, const float *s)
{
    __asm__("flds %2\n\tflds %3\n\tflds %4\n\tflds %5\n\t"
            "fld %%st(1)\n\tfmul %%st(4), %%st\n\tfld %%st(1)\n\tfmul %%st(4), %%st\n\t"
            ".byte 0xde, 0xe9\n\tfstps %0\n\t"
            "fxch\n\tfmul %%st(2), %%st\n\tfxch\n\tfmul %%st(3), %%st\n\t"
            ".byte 0xde, 0xc1\n\tfstps %1\n\tfstp %%st(0)\n\tfstp %%st(0)"
            : "=m"(*r1), "=m"(*r2)
            : "m"(a), "m"(b), "m"(*c), "m"(*s)
            : "st", "st(1)", "st(2)", "st(3)", "st(4)", "st(5)");
}
// NOLINTEND(readability-non-const-parameter)

#else
#include <math.h>

static inline float bmxX87SinScaled(int32_t n, float k)
{
    return (float)sin((double)n * k);
}
static inline double bmxX87CosScaled(int32_t n, double k)
{
    return cos((double)n * k);
}
static inline double bmxX87AsinScaled(int32_t n, double step, float scale)
{
    return asin((double)n * step) * scale;
}
static inline double bmxX87HalfPiScaled(int negative, float scale)
{
    return (negative ? -1.5707963267948966 : 1.5707963267948966) * scale;
}
static inline double bmxX87AtanScaled(int32_t n, double step, float scale)
{
    return atan((double)n * step) * scale;
}
static inline void bmxX87Rotate(float *r1, float *r2, float a, float b, const float *c, const float *s)
{
    const double cd = *c, sd = *s;

    *r1 = (float)(cd * a - sd * b);
    *r2 = (float)(cd * b + sd * a);
}
#endif

#endif
