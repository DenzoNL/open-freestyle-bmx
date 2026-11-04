/*
 * Layout asserts (docs/PORTING.md X2c).
 *
 * The ported structs carry asserts of the original's offsets and sizes. Two kinds:
 * - a layout read from or written to a file (an on-disk type, fixed-width fields only): `_Static_assert`,
 *   checked on every build;
 * - an in-memory layout with pointers in it: BMX_LAYOUT32_ASSERT. It documents the original's layout and
 *   holds it where pointers are 32 bits wide (the hook build shares those structs with the original's code,
 *   and the 32-bit native build must match it); with 64-bit pointers the layout is the compiler's.
 * Code must not depend on the 32-bit layout of the second kind: sizes are `sizeof`, offsets fields or
 * `offsetof`.
 */
#ifndef BMX_COMPAT_LAYOUT_H
#define BMX_COMPAT_LAYOUT_H

#include <stdint.h>

#if UINTPTR_MAX == 0xffffffffu
#define BMX_LAYOUT32                   1
#define BMX_LAYOUT32_ASSERT(cond, msg) _Static_assert(cond, msg)
#else
#define BMX_LAYOUT32                   0
#define BMX_LAYOUT32_ASSERT(cond, msg) _Static_assert(1, msg)
#endif

/* An offset into src/data's definitions that differs with the pointer width: src/data/addrs.h (a global
 * inside a group) and the group asserts of src/data/globals.h, both widths asserted. tools/data_off64.py
 * computes o64. */
#if BMX_LAYOUT32
#define BMX_DATA_OFF(o32, o64) (o32)
#else
#define BMX_DATA_OFF(o32, o64) (o64)
#endif

/*
 * A pointer the original returns as its int result, which every caller only tests against 0: the pointer's
 * bits where they fit (the oracle compares the result), else 1 for a pointer that is set (docs/PORTING.md
 * "X2e").
 */
static inline int bmxPtrResult(const void *p)
{
    return BMX_LAYOUT32 ? (int)(intptr_t)p : p != (const void *)0;
}

#endif
