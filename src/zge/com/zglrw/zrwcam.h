/* Scene markers (C:\NgEngine\Zge\Com\ZglRw\zrwcam.c). Not ported yet; callees only. */
#ifndef ZGE_COM_ZGLRW_ZRWCAM_H
#define ZGE_COM_ZGLRW_ZRWCAM_H

#include <stdint.h>

#include "compat/orig.h"
#include "zge/zmath.h"

/* A named marker of the level scene (the "Player<n>" start points): its frame matrix sits at +8. */
typedef struct ZrwMarker {
    uint8_t pad00[0x8];
    ZMatrix matrix;
} ZrwMarker;

/* Looks a marker up by the hash of its base name and its number; NULL when absent. */
ORIG_FN(0x00492130, ZrwMarker *, zrwcamFindMarker, (int hash, int index));

#endif
