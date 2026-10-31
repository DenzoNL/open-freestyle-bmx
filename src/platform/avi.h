/* The AVI reader's error codes (vfw.h's AVIERR_*, MAKE_AVIERR(n) = 0x80044000 + n). */
#ifndef BMX_PLATFORM_AVI_H
#define BMX_PLATFORM_AVI_H

#include <stdint.h>

#define BMX_AVIERR(n) ((int32_t)(0x80044000u + (n)))

enum {
    kAvierrBadFormat = BMX_AVIERR(102),
    kAvierrMemory = BMX_AVIERR(103),
    kAvierrBadParam = BMX_AVIERR(106),
    kAvierrBadHandle = BMX_AVIERR(108),
    kAvierrFileRead = BMX_AVIERR(109),
    kAvierrFileOpen = BMX_AVIERR(111),
    kAvierrReadOnly = BMX_AVIERR(114),
    kAvierrNoData = BMX_AVIERR(115),
    kAvierrBufferTooSmall = BMX_AVIERR(116),
    kAvierrError = BMX_AVIERR(199),
};

#endif
