/* Renderer models and materials (C:\NgEngine\Zge\Com\ZglRw\zrwmat.c). Not ported yet; callees only. */
#ifndef ZGE_COM_ZGLRW_ZRWMAT_H
#define ZGE_COM_ZGLRW_ZRWMAT_H

#include "compat/orig.h"

/* Wrappers over the renderer's model table: load from a chunk, release, and look up. */
ORIG_FN(0x00488fb0, void *, zrwmatCreateObject, (void *model));
ORIG_FN(0x00489070, void *, zrwmatLoadModel, (void *source));
ORIG_FN(0x00489090, void *, zrwmatLoadChunk, (void *iff));
ORIG_FN(0x004890b0, void, zrwmatReleaseModel, (void *model));
ORIG_FN(0x004890c0, void *, zrwmatModelFromObject, (void *model));

#endif
