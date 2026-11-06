/*
 * The SSE rounding mode's counterpart on arm64 (docs/PORTING.md "X5 part 1"): FPCR.
 *
 * Inside a camera update the original runs the x87 rounding down; the port's float code (SSE on x86) gets
 * that mode copied into MXCSR by rw/rwd3d.h. On arm64 the same copy goes into FPCR's RMode, from the x87
 * control word the camera mode sets (bmxX87ControlWord, compat/x87soft.h: a variable there), and the driver's
 * DRV_FPU_GUARDED saves and restores FPCR around GPU driver calls like it does MXCSR.
 */
#ifndef BMX_COMPAT_FPU_H
#define BMX_COMPAT_FPU_H

#include <stdint.h>

#if defined(__aarch64__)
static inline uint64_t bmxFpcrGet(void)
{
    uint64_t fpcr;
    __asm__ volatile("mrs %0, fpcr" : "=r"(fpcr)::"memory");
    return fpcr;
}

static inline void bmxFpcrSet(uint64_t fpcr)
{
    __asm__ volatile("msr fpcr, %0" ::"r"(fpcr) : "memory");
}

/* FPCR with the rounding mode (RMode, bits 22-23) of an x87 control word (RC, bits 10-11): x87 nearest /
 * down / up / chop are RMode nearest (0) / minus (2) / plus (1) / zero (3). */
static inline uint64_t bmxFpcrWithX87Rounding(uint64_t fpcr, uint16_t cw)
{
    static const uint8_t kRMode[4] = {0, 2, 1, 3};
    return (fpcr & ~(UINT64_C(3) << 22)) | ((uint64_t)kRMode[(cw >> 10) & 3] << 22);
}
#endif

#endif
