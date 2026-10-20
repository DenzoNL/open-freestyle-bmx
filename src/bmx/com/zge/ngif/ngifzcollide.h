/* Collision queries (C:\zaxis\Bmx\src\com\zge\NgIF\NgIFzcollide.c). Not ported yet; callees only. */
#ifndef BMX_COM_ZGE_NGIF_NGIFZCOLLIDE_H
#define BMX_COM_ZGE_NGIF_NGIFZCOLLIDE_H

#include "compat/orig.h"
#include "zge/zmath.h"

/* Casts a ray from -> to and fills a 0xd8 byte hit record; returns nonzero on a hit. */
ORIG_FN(0x00469b00, int, zcollideRay, (const ZVec3 *from, const ZVec3 *to, void *hit));

#endif
