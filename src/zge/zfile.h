/* Files (C:\NgEngine\Zge\zfile.c). Not ported yet; callees only. */
#ifndef ZGE_ZFILE_H
#define ZGE_ZFILE_H

#include <stdint.h>

#include "compat/orig.h"

/* Opens a file by path (the original calls it PizdecFunction); returns a handle or NULL. */
ORIG_FN(0x00484700, void *, zfileOpen, (const char *path, int mode, int flags));
/* Writes size bytes; returns the number written. */
ORIG_FN(0x00484b90, int, zfileWrite, (void *file, const void *data, int size, int flags));

#endif
