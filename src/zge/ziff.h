/* IFF chunk files (C:\NgEngine\Zge\ziff.c). Not ported yet; callees only. */
#ifndef ZGE_ZIFF_H
#define ZGE_ZIFF_H

#include <stdint.h>

#include "compat/orig.h"

/* Registers the callback for chunk `id` of files read with ziffRead (`id` is a four character code). */
ORIG_FN(0x0049aad0, void, ziffSetupCallback, (uint32_t id, int(BMX_CDECL *callback)(void *iff), int flags));
/* Opens an IFF reader on a file handle; formId -1 accepts any form. Returns NULL on failure. */
ORIG_FN(0x0049a790, void *, ziffOpen, (void *file, uint32_t formId, uint32_t size));
ORIG_FN(0x0049a8a0, void, ziffClose, (void *iff));
/* Positions on the first / next chunk with the given id; returns nonzero when there is one. */
ORIG_FN(0x0049ad90, int, ziffFindChunk, (void *iff, uint32_t id));
ORIG_FN(0x0049aed0, int, ziffNextChunk, (void *iff, uint32_t id));
/* Size of the current chunk, and a read of `size` bytes of it. */
ORIG_FN(0x0049abc0, uint32_t, ziffChunkSize, (void *iff));
ORIG_FN(0x0049ac30, int, ziffRead, (void *iff, void *dst, uint32_t size));
/* Reads the animation in the current chunk; its name hash; frees it. */
ORIG_FN(0x0049a4e0, void *, ziffReadAnim, (void *iff));
ORIG_FN(0x0049a530, uint32_t, ziffAnimName, (void *anim));
ORIG_FN(0x0049a630, void, ziffFreeAnim, (void *anim));
/* Starts / ends reading a model chunk. */
ORIG_FN(0x0049b130, int, ziffBeginModel, (void *iff));
ORIG_FN(0x0049b250, void, ziffEndModel, (void *iff));

#endif
