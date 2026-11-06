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

/*
 * The floating-point state a Windows thread starts with, for x86 builds on other systems (docs/PORTING.md "X4
 * (Linux x64)"): x87 control word 0x027f (53-bit precision, round to nearest, exceptions masked) and MXCSR
 * 0x1f80 (round to nearest, exceptions masked, no flush-to-zero or denormals-are-zero). Linux starts the x87
 * at 0x037f, 64-bit precision, with which the x87 helpers of compat/x87.h would round differently from
 * Windows. Called first in main and in our own threads; SDL's threads copy the state of the thread that
 * creates them (Linux clone). Nothing on Windows (already its state) or without an x87 (x87soft's control
 * word starts at 0x027f).
 */
static inline void bmxFpuWindowsDefaults(void)
{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_WIN32)
    const uint16_t cw = 0x027f;
    const uint32_t mxcsr = 0x1f80;

    __asm__ volatile("fldcw %0" ::"m"(cw) : "memory");
    __asm__ volatile("ldmxcsr %0" ::"m"(mxcsr) : "memory");
#endif
}

#endif
