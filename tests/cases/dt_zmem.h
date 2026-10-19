/* Shared setup helpers for tests that allocate through zmem (zmem, zlist, zstring). */
#ifndef DT_ZMEM_H
#define DT_ZMEM_H

#include <stdint.h>

#include "dt.h"
#include "zge/zmem.h"

/* Calls original (unported) code at a fixed address. */
#define DT_ORIG(addr, type) ((type)(uintptr_t)(addr))

/* Original zmemAlloc / zmemFreeList* / zmemPool*, for building fixtures independently of the port. */
#define dt_zmemalloc(n) DT_ORIG(0x00485aa0, void *(*)(uint32_t))(n)

/* Points the renderer memory interface at the exe's CRT heap, which the harness serves deterministically. */
static inline void dt_zmem_env(void)
{
    gRwMalloc = DT_ORIG(0x004a50d2, ZMemAllocFn);
    gRwFree = DT_ORIG(0x004a4d7f, ZMemFreeFn);
    gRwRealloc = DT_ORIG(0x0051d67e, ZMemReallocFn);
    gZmem.pool = NULL;
}

/* NUL-terminated random string of 0..maxLen non-zero bytes (printable unless wide is set). */
static inline char *dt_rand_str(int maxLen, int wide)
{
    const int len = dt_range(0, maxLen);
    char *s = dt_alloc((uint32_t)len + 1);
    for (int i = 0; i < len; i++) {
        s[i] = wide ? (char)dt_range(1, 255) : (char)dt_range('a' - 3, 'z' + 3);
    }
    return s;
}

#endif
