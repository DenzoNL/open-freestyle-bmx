/* Sound file loader (C:\zaxis\Bmx\pc\zge\sound\winload.c). Not ported yet; callees only. */
#ifndef BMX_PC_ZGE_SOUND_WINLOAD_H
#define BMX_PC_ZGE_SOUND_WINLOAD_H

#include <stdint.h>

#include "compat/orig.h"

/* Voice number `index` (a 0x14 byte descriptor) of the loaded sound file `file` (0-15). */
ORIG_FN(0x004704a0, void *, winloadGetVoice, (uint16_t file, uint32_t index));
/* Key map number `index` of the loaded sound file `file`. */
ORIG_FN(0x00470880, const uint8_t *, winloadGetKeymap, (uint16_t file, uint32_t index));

#endif
