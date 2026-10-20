/* Resource tables (C:\zaxis\Bmx\src\com\zge\NgIF\NgIFzres.c). Not ported yet; callees only. */
#ifndef BMX_COM_ZGE_NGIF_NGIFZRES_H
#define BMX_COM_ZGE_NGIF_NGIFZRES_H

#include <stdint.h>

#include "compat/orig.h"

/* Reads the 0x18 byte rider record `index` into a static buffer (asserts "Invalid player"). */
ORIG_FN(0x0046bb00, const uint8_t *, ngifzresGetRiderRecord, (uint32_t index));
/* Registers resource `id` of kind `kind` (used for the font and icon sheets). */
ORIG_FN(0x0046bf30, void, ngifzresRegister, (int id, uint32_t a, uint32_t b));

/* Resource system start / reset / shutdown (zres init returns 0). */
ORIG_FN(0x0046be10, int, ngifzresInit, (void));
ORIG_FN(0x0046bef0, int, ngifzresReset, (void));
ORIG_FN(0x0046bed0, void, ngifzresShutdown, (void));

#endif
