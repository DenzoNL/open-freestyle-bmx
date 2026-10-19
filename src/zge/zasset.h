/* Asset library files (C:\NgEngine\Zge\zasset.c). Not ported yet. */
#ifndef ZGE_ZASSET_H
#define ZGE_ZASSET_H

#include "compat/orig.h"

/* Opens a library file; returns a handle or 0. */
ORIG_FN(0x004804d0, int, zassetOpenLibrary, (const char *name));
ORIG_FN(0x004806c0, void, zassetCloseLibrary, (int lib));
/* Selects resource `id` in the library; returns 1 when it exists. */
ORIG_FN(0x00480780, int, zassetFind, (int lib, int id, int flags));
/* Size in bytes of the selected resource. */
ORIG_FN(0x00480850, uint32_t, zassetSize, (int lib));
/* Reads the selected resource into dst; returns 1 on success. */
ORIG_FN(0x00480930, int, zassetRead, (int lib, void *dst));

#endif
