/*
 * The real x87 sequences of compat/x87.h behind pointer-only entry points, for tests/platform/x87_unicorn.py
 * (built for i386 and run under Unicorn there). cw is loaded with FLDCW first.
 */
#include "compat/x87.h"

#define ENTRY(name) __attribute__((used, noinline)) void name

ENTRY(hwRotate)(const uint16_t *cw, const float *in, float *out)
{
    bmxX87SetControlWord(*cw);
    bmxX87HwRotate(&out[0], &out[1], in[0], in[1], &in[2], &in[3]);
}

ENTRY(hwMatrix)(const uint16_t *cw, const float *in, float *out)
{
    bmxX87SetControlWord(*cw);
    bmxX87HwMatrixMultiply(out, in, in + 16);
}

ENTRY(hwHalfPi)(const uint16_t *cw, const float *in, double *out)
{
    bmxX87SetControlWord(*cw);
    out[0] = bmxX87HwHalfPiScaled(0, in[0]);
    out[1] = bmxX87HwHalfPiScaled(1, in[0]);
}

/*
 * One arithmetic instruction on two 80-bit operands (op 0 FADD, 1 FSUB a - b, 2 FMUL, 3 FSQRT of a): the
 * register result (FSTP m80, exact) and its FST to float and double.
 */
ENTRY(hwOp)(const uint16_t *cw, const uint8_t *a, const uint8_t *b, const int32_t *op, uint8_t *out)
{
    bmxX87SetControlWord(*cw);
    __asm__ volatile("fldt (%1)\n\tfldt (%0)\n\t"
                     "cmpl $0, (%2)\n\tjne 1f\n\tfadd %%st(1), %%st\n\tjmp 4f\n"
                     "1:\n\tcmpl $1, (%2)\n\tjne 2f\n\tfsub %%st(1), %%st\n\tjmp 4f\n"
                     "2:\n\tcmpl $2, (%2)\n\tjne 3f\n\tfmul %%st(1), %%st\n\tjmp 4f\n"
                     "3:\n\tfsqrt\n"
                     "4:\n\tfld %%st(0)\n\tfstpt (%3)\n\tfld %%st(0)\n\tfstps 10(%3)\n\tfstpl 14(%3)\n\t"
                     "fstp %%st(0)"
                     :
                     : "r"(a), "r"(b), "r"(op), "r"(out)
                     : "memory", "cc", "st", "st(1)");
}

ENTRY(hwSetCw)(const uint16_t *cw)
{
    bmxX87SetControlWord(*cw);
}
