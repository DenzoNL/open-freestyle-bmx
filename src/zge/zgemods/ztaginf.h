/* Level tag tables (C:\NgEngine\Zge\ZgeMods\ztaginf.c). Not ported yet; callees only. */
#ifndef ZGE_ZGEMODS_ZTAGINF_H
#define ZGE_ZGEMODS_ZTAGINF_H

#include <stdint.h>

#include "compat/orig.h"

struct ZbspLevelFace;

/* The level's triangle array and its length (globals 0x6728c0 / 0x6728bc). */
#define gZtaginfFaces     ORIG_VAR(0x006728c0, struct ZbspLevelFace *, gZtaginfFaces)
#define gZtaginfFaceCount ORIG_VAR(0x006728bc, uint32_t, gZtaginfFaceCount)

/* Frees the level tag tables. */
ORIG_FN(0x0048d8d0, void, ztaginfRelease, (void));
ORIG_FN(0x0048d980, struct ZbspLevelFace *, ztaginfGetFaces, (void));
ORIG_FN(0x0048d990, uint32_t, ztaginfGetFaceCount, (void));

#endif
