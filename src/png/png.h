/*
 * Our PNG reader: replaces the libpng 1.0.5 entry points RtPNGImageRead (rw/texture.c) calls, in the native
 * exe only. The hook DLL and the oracle harness keep the original libpng. See src/png/png.c.
 */
#ifndef BMX_PNG_PNG_H
#define BMX_PNG_PNG_H

#include <stdint.h>

#include "hook/hook.h"

/* (original libpng address, our function) pairs: tools/gen_origmap.py makes calls and pointers to those
 * addresses reach our functions in the native exe (over a port of the same address, if there were one). */
extern const BmxHook bmxPngJumps[];
extern const uint32_t bmxPngJumpCount;

/* libpng's png_get_io_ptr: the pointer png_set_read_fn stored (rwPngRead's stream). */
void *bmxPngIoPtr(void *png);

#endif
