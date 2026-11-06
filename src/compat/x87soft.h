/*
 * Portable C versions of the x87 sequences in compat/x87.h (docs/PORTING.md "X5 part 2 (portable x87)"),
 * bit-exact with the hardware: integer-only emulation of the 80-bit registers, precision control (24 / 53 /
 * 64 bit), the rounding modes, the x87 NaN rules, and FSIN / FCOS / FPTAN / FPATAN / FSQRT evaluated to 128
 * bits and rounded to the register's 64 bits (FSIN / FCOS / FPTAN reduce with the 66-bit pi of Intel's
 * microcode). Used on CPUs without an x87 (arm64), and on x86 with BMX_X87_SOFT=1 to prove them on a replay.
 */
#ifndef BMX_COMPAT_X87SOFT_H
#define BMX_COMPAT_X87SOFT_H

#include <stdint.h>

/* An x87 register: the 64-bit significand (explicit integer bit) and sign << 15 | biased exponent. */
typedef struct BmxX87Reg {
    uint64_t m;
    uint16_t se;
} BmxX87Reg;

/*
 * The x87 control word the emulation runs with (precision control bits 8-9, rounding control bits 10-11).
 * On x86 these are the real FNSTCW / FLDCW, so the soft path follows the hardware state; elsewhere a variable
 * that starts at Windows' 0x027f (53 bit, round to nearest) and that the camera update sets like FLDCW.
 */
#if defined(__i386__) || defined(__x86_64__)
static inline uint16_t bmxX87ControlWord(void)
{
    uint16_t cw;
    __asm__ volatile("fnstcw %0" : "=m"(cw)::"memory");
    return cw;
}

static inline void bmxX87SetControlWord(uint16_t cw)
{
    __asm__ volatile("fldcw %0" ::"m"(cw) : "memory");
}
#else
extern uint16_t bmxX87SoftCw;

static inline uint16_t bmxX87ControlWord(void)
{
    return bmxX87SoftCw;
}

static inline void bmxX87SetControlWord(uint16_t cw)
{
    bmxX87SoftCw = cw;
}
#endif

float bmxX87SoftSinScaled(int32_t n, float k);
double bmxX87SoftCosScaled(int32_t n, double k);
double bmxX87SoftFsin(double a);
double bmxX87SoftFcos(double a);
double bmxX87SoftAsinScaled(int32_t n, double step, float scale);
float bmxX87SoftAngleOfCosine(double c);
float bmxX87SoftTanFloat(float a);
double bmxX87SoftHalfPiScaled(int negative, float scale);
double bmxX87SoftAtanScaled(int32_t n, double step, float scale);
void bmxX87SoftRotate(float *r1, float *r2, float a, float b, const float *c, const float *s);
void bmxX87SoftMatrixMultiply(float *out, const float *a, const float *b);

/*
 * The pi FSIN / FCOS / FPTAN reduce with: 0 (the default) Intel's 66-bit value, 1 pi to 128 bits (for
 * tests/platform/test_x87.c, to tell which one a CPU uses).
 */
void bmxX87SoftSetReductionPi(int precise);

/* The register operations, for the tests. Precision and rounding come from bmxX87ControlWord(). */
BmxX87Reg bmxX87SoftLoadDouble(double v);
double bmxX87SoftStoreDouble(BmxX87Reg r);
float bmxX87SoftStoreFloat(BmxX87Reg r);
/* op 0 FADD a + b, 1 FSUB a - b, 2 FMUL, 3 FSQRT a. */
BmxX87Reg bmxX87SoftArith(int op, BmxX87Reg a, BmxX87Reg b);
BmxX87Reg bmxX87SoftFsinReg(BmxX87Reg x);
BmxX87Reg bmxX87SoftFcosReg(BmxX87Reg x);
BmxX87Reg bmxX87SoftFpatanReg(BmxX87Reg y, BmxX87Reg x);

#endif
