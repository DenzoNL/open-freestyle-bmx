/* Effect nodes of the level renderer (C:\zaxis\Bmx\src\com\zge\NgIF\NgIFfx.c). Not ported yet; callees only.
 */
#ifndef BMX_COM_ZGE_NGIF_NGIFFX_H
#define BMX_COM_ZGE_NGIF_NGIFFX_H

#include <stdint.h>

#include "compat/orig.h"

/* The effect node `index` attached to an object (NULL if none); +0x30 points at its transform. */
ORIG_FN(0x0045f480, const uint8_t *, ngiffxGetNode, (const void *obj, int index));

#endif
