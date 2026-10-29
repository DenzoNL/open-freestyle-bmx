/*
 * Our PNG reader: replaces the libpng 1.0.5 entry points RtPNGImageRead (rw/texture.c) calls, in the native
 * exe only. The hook DLL and the oracle harness keep the original libpng. See src/png/png.c.
 */
#ifndef BMX_PNG_PNG_H
#define BMX_PNG_PNG_H

#include <stdint.h>

#include "hook/hook.h"

/* (original libpng address, our function) pairs; the native loader writes them as jumps after the hooks. */
extern const BmxHook bmxPngJumps[];
extern const uint32_t bmxPngJumpCount;

/* The original's libpng and zlib code (lib/libpng, lib/zlib; up to the next ported function 0x51ce10). */
enum { kBmxPngCodeStart = 0x00506e40, kBmxPngCodeEnd = 0x0051ce10 };

#endif
