/* Level renderer glue (C:\zaxis\Bmx\src\com\zge\NgIF\NgIFzroller.c). Not ported yet; callees only. */
#ifndef BMX_COM_ZGE_NGIF_NGIFZROLLER_H
#define BMX_COM_ZGE_NGIF_NGIFZROLLER_H

#include "compat/orig.h"

typedef int(BMX_CDECL *ZrollerDrawFn)(void);

/* Adds a debug draw callback (asserts when all eight slots are taken). */
ORIG_FN(0x0046d690, void, zrollerAddDrawCallback, (ZrollerDrawFn fn));
/* World bounding box {min x, y, z, pad, max x, y, z} in a static buffer. */
ORIG_FN(0x0046ceb0, const float *, zrollerGetWorldBounds, (void));

#endif
