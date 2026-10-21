/* Effect nodes of the level renderer (C:\zaxis\Bmx\src\com\zge\NgIF\NgIFfx.c). Not ported yet; callees only.
 */
#ifndef BMX_COM_ZGE_NGIF_NGIFFX_H
#define BMX_COM_ZGE_NGIF_NGIFFX_H

#include <stdint.h>

#include "compat/orig.h"

/* The effect node `index` attached to an object (NULL if none); +0x30 points at its transform. */
ORIG_FN(0x0045f480, const uint8_t *, ngiffxGetNode, (const void *obj, int index));

/* Starts effect `kind` (a debug print for kinds other than 0 in this build); returns 0. */
ORIG_FN(0x0045fcb0, int, ngiffxCreate, (int kind, const void *params, const void *arg));
/* Destroys the effect list and unregisters the handlers. */
ORIG_FN(0x0045fc10, void, ngiffxShutdown, (void));
/* Registers the effect handlers (spark, shard, ripple, splash, ...) by name. */
ORIG_FN(0x0045f780, void, ngiffxRegisterHandlers, (void));

#endif
