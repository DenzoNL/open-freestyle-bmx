/*
 * x87 instruction sequences of the original that plain C cannot reproduce bit for bit: the transcendental
 * instructions its lookup tables and a few run-time paths use (FSIN, FCOS, FPTAN, FPATAN, and the CRT's asin
 * built on FPATAN), and two arithmetic routines whose results depend on the x87 control word (the camera
 * update's 24-bit, round-down mode) and the x87 NaN rules. Every helper covers a whole FILD..FSTP sequence so
 * no intermediate is rounded to a narrower type than in the original.
 *
 * On x86 the hook and native builds run the real instructions (bmxX87Hw*). Elsewhere (arm64), and on x86
 * with -DBMX_X87_SOFT=1, the portable bit-exact emulation of compat/x87soft.c runs instead (bmxX87Soft*,
 * docs/PORTING.md "X5 part 2 (portable x87)").
 */
#ifndef BMX_COMPAT_X87_H
#define BMX_COMPAT_X87_H

#include <stdint.h>

#include "compat/x87soft.h"

#ifndef BMX_X87_SOFT
#define BMX_X87_SOFT 0
#endif

#if defined(__i386__) || defined(__x86_64__)

/* (float)fsin((double)n * k). */
static inline float bmxX87HwSinScaled(int32_t n, float k)
{
    float r;
    __asm__("fildl %1\n\tfmuls %2\n\tfsin\n\tfstps %0" : "=m"(r) : "m"(n), "m"(k) : "st");
    return r;
}

/* fcos((double)n * k), rounded to double. */
static inline double bmxX87HwCosScaled(int32_t n, double k)
{
    double r;
    __asm__("fildl %1\n\tfmull %2\n\tfcos\n\tfstpl %0" : "=m"(r) : "m"(n), "m"(k) : "st");
    return r;
}

/* FSIN / FCOS of a value held in a register (a double). */
static inline double bmxX87HwFsin(double a)
{
    double r;
    __asm__("fldl %1\n\tfsin\n\tfstpl %0" : "=m"(r) : "m"(a) : "st");
    return r;
}

static inline double bmxX87HwFcos(double a)
{
    double r;
    __asm__("fldl %1\n\tfcos\n\tfstpl %0" : "=m"(r) : "m"(a) : "st");
    return r;
}

/* asin(n * step) * scale as the CRT computes it for |n * step| < 1: x / sqrt((1 + x) * (1 - x)) via FPATAN.
 */
static inline double bmxX87HwAsinScaled(int32_t n, double step, float scale)
{
    double r;
    __asm__("fildl %1\n\tfmull %2\n\tfld1\n\tfadd %%st(1), %%st\n\tfld1\n\tfsub %%st(2), %%st\n\t"
            "fmulp %%st, %%st(1)\n\tfsqrt\n\tfpatan\n\tfmuls %3\n\tfstpl %0"
            : "=m"(r)
            : "m"(n), "m"(step), "m"(scale)
            : "st", "st(1)", "st(2)");
    return r;
}

/* (float)atan2(sqrt((1 + c) * (1 - c)), c) by FSQRT and FPATAN: the angle of a cosine (the slerp set-up). */
static inline float bmxX87HwAngleOfCosine(double c)
{
    float r;
    __asm__("fldl %1\n\tfld1\n\tfadd %%st(1), %%st\n\tfld1\n\tfsub %%st(2), %%st\n\t"
            "fmulp %%st, %%st(1)\n\tfsqrt\n\tfxch\n\tfpatan\n\tfstps %0"
            : "=m"(r)
            : "m"(c)
            : "st", "st(1)", "st(2)");
    return r;
}

/* (float)tan(a): FLD, FPTAN, drop the 1.0 it pushes, FSTP to float (the light cone angle). */
static inline float bmxX87HwTanFloat(float a)
{
    float r;
    __asm__("flds %1\n\tfptan\n\tfstp %%st(0)\n\tfstps %0" : "=m"(r) : "m"(a) : "st", "st(1)");
    return r;
}

/* The CRT's asin(+-1): +-pi/2 loaded as an 80 bit constant, times scale. */
static inline double bmxX87HwHalfPiScaled(int negative, float scale)
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
static inline double bmxX87HwAtanScaled(int32_t n, double step, float scale)
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
static inline void bmxX87HwRotate(float *r1, float *r2, float a, float b, const float *c, const float *s)
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

/*
 * The core's x87 matrix product (_rwcseg 0x54b038) instruction by instruction: three rows of the rotation
 * part (the loop), then the position row with the second matrix's position added. EAX = out, EDX = a,
 * EBX = b, ECX = the row counter, as the original uses them. The result depends on the x87 control word
 * (the camera update runs it at 24 bit precision, rounding down). On x86_64 the same registers in their
 * 64-bit form (the pointers are 64-bit; addressing through EAX would drop the upper half).
 */
#if defined(__x86_64__)
#define X87_AX  "%%rax"
#define X87_DX  "%%rdx"
#define X87_BX  "%%rbx"
#define X87_LEA "leaq"
#else
#define X87_AX  "%%eax"
#define X87_DX  "%%edx"
#define X87_BX  "%%ebx"
#define X87_LEA "leal"
#endif
// NOLINTBEGIN(readability-non-const-parameter)
static inline void bmxX87HwMatrixMultiply(float *out, const float *a, const float *b)
{
    int rows = 3;
    __asm__ volatile("1:\n\t"
                     "flds (" X87_DX ")\n\tfmuls (" X87_BX ")\n\t"
                     "flds 4(" X87_DX ")\n\tfmuls 0x10(" X87_BX ")\n\t"
                     "flds 8(" X87_DX ")\n\tfmuls 0x20(" X87_BX ")\n\t"
                     "flds (" X87_DX ")\n\tfxch %%st(1)\n\tfaddp %%st, %%st(2)\n\t"
                     "fmuls 4(" X87_BX ")\n\tfxch %%st(1)\n\tfaddp %%st, %%st(2)\n\t"
                     "flds 4(" X87_DX ")\n\tfmuls 0x14(" X87_BX ")\n\t"
                     "flds 8(" X87_DX ")\n\tfmuls 0x24(" X87_BX ")\n\t"
                     "flds (" X87_DX ")\n\tfxch %%st(1)\n\tfaddp %%st, %%st(2)\n\t"
                     "fmuls 8(" X87_BX ")\n\tfxch %%st(1)\n\tfaddp %%st, %%st(2)\n\t"
                     "flds 4(" X87_DX ")\n\tfmuls 0x18(" X87_BX ")\n\t"
                     "flds 8(" X87_DX ")\n\tfmuls 0x28(" X87_BX ")\n\t"
                     "fxch %%st(4)\n\tfstps (" X87_AX ")\n\tfaddp %%st, %%st(3)\n\t"
                     "fxch %%st(1)\n\tfstps 4(" X87_AX ")\n\tfaddp %%st, %%st(1)\n\t" X87_LEA " 0x10(" X87_DX
                     "), " X87_DX "\n\tdecl %%ecx\n\tfstps 8(" X87_AX ")\n\t" X87_LEA " 0x10(" X87_AX
                     "), " X87_AX "\n\t"
                     "jne 1b\n\t"
                     "flds (" X87_DX ")\n\tfmuls (" X87_BX ")\n\t"
                     "flds 4(" X87_DX ")\n\tfxch %%st(1)\n\tfadds 0x30(" X87_BX
                     ")\n\tfxch %%st(1)\n\tfmuls 0x10(" X87_BX ")\n\t"
                     "flds 8(" X87_DX ")\n\tfmuls 0x20(" X87_BX ")\n\t"
                     "flds (" X87_DX ")\n\tfxch %%st(1)\n\tfaddp %%st, %%st(2)\n\t"
                     "fmuls 4(" X87_BX ")\n\tfxch %%st(1)\n\tfaddp %%st, %%st(2)\n\t"
                     "flds 4(" X87_DX ")\n\tfxch %%st(1)\n\tfadds 0x34(" X87_BX
                     ")\n\tfxch %%st(1)\n\tfmuls 0x14(" X87_BX ")\n\t"
                     "flds 8(" X87_DX ")\n\tfmuls 0x24(" X87_BX ")\n\t"
                     "flds (" X87_DX ")\n\tfxch %%st(1)\n\tfaddp %%st, %%st(2)\n\t"
                     "fmuls 8(" X87_BX ")\n\tfxch %%st(1)\n\tfaddp %%st, %%st(2)\n\t"
                     "flds 4(" X87_DX ")\n\tfxch %%st(1)\n\tfadds 0x38(" X87_BX
                     ")\n\tfxch %%st(1)\n\tfmuls 0x18(" X87_BX ")\n\t"
                     "flds 8(" X87_DX ")\n\tfmuls 0x28(" X87_BX ")\n\t"
                     "fxch %%st(4)\n\tfstps (" X87_AX ")\n\tfaddp %%st, %%st(3)\n\t"
                     "fxch %%st(1)\n\tfstps 4(" X87_AX ")\n\tfaddp %%st, %%st(1)\n\tfstps 8(" X87_AX ")"
                     : "+a"(out), "+d"(a), "+c"(rows)
                     : "b"(b)
                     : "memory", "cc", "st", "st(1)", "st(2)", "st(3)", "st(4)", "st(5)", "st(6)", "st(7)");
}
// NOLINTEND(readability-non-const-parameter)
#undef X87_AX
#undef X87_DX
#undef X87_BX
#undef X87_LEA

#endif /* x86 */

#if defined(BMX_HOOK) && (defined(__i386__) || defined(__x86_64__)) && !BMX_X87_SOFT
#define BMX_X87_IMPL(name) bmxX87Hw##name
#else
#define BMX_X87_IMPL(name) bmxX87Soft##name
#endif

static inline float bmxX87SinScaled(int32_t n, float k)
{
    return BMX_X87_IMPL(SinScaled)(n, k);
}

static inline double bmxX87CosScaled(int32_t n, double k)
{
    return BMX_X87_IMPL(CosScaled)(n, k);
}

static inline double bmxX87Fsin(double a)
{
    return BMX_X87_IMPL(Fsin)(a);
}

static inline double bmxX87Fcos(double a)
{
    return BMX_X87_IMPL(Fcos)(a);
}

static inline double bmxX87AsinScaled(int32_t n, double step, float scale)
{
    return BMX_X87_IMPL(AsinScaled)(n, step, scale);
}

static inline float bmxX87AngleOfCosine(double c)
{
    return BMX_X87_IMPL(AngleOfCosine)(c);
}

static inline float bmxX87TanFloat(float a)
{
    return BMX_X87_IMPL(TanFloat)(a);
}

static inline double bmxX87HalfPiScaled(int negative, float scale)
{
    return BMX_X87_IMPL(HalfPiScaled)(negative, scale);
}

static inline double bmxX87AtanScaled(int32_t n, double step, float scale)
{
    return BMX_X87_IMPL(AtanScaled)(n, step, scale);
}

static inline void bmxX87Rotate(float *r1, float *r2, float a, float b, const float *c, const float *s)
{
    BMX_X87_IMPL(Rotate)(r1, r2, a, b, c, s);
}

static inline void bmxX87MatrixMultiply(float *out, const float *a, const float *b)
{
    BMX_X87_IMPL(MatrixMultiply)(out, a, b);
}

#undef BMX_X87_IMPL

#endif
