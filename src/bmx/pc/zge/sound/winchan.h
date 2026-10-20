/* Windows sound channels (C:\zaxis\Bmx\pc\zge\sound\winchan.c). Empty stubs in the PC build; callees only. */
#ifndef BMX_PC_ZGE_SOUND_WINCHAN_H
#define BMX_PC_ZGE_SOUND_WINCHAN_H

#include <stdint.h>

#include "compat/orig.h"

typedef void(BMX_CDECL *WinchanCallback)(int a, int b);

ORIG_FN(0x0046d720, void, winchanStart, (uint32_t slot));
/* Registers the callback that reports the progress of the save thread. */
ORIG_FN(0x0046d7a0, int, winchanSetCallback, (WinchanCallback callback));
ORIG_FN(0x0046d8a0, int, winchanQuery, (void));
ORIG_FN(0x0046d8c0, void, winchanBegin, (void));
ORIG_FN(0x0046d8e0, void, winchanEnd, (void));

/*
 * Voice channels of the software mixer ("zsndc"). A handle is a byte; 0xff means none. Handles and
 * small values are passed as full words with undefined upper bits.
 */
ORIG_FN(0x0046e410, uint8_t, winchanVoiceAlloc, (const void *voice, int priority));
ORIG_FN(0x0046e890, int, winchanVoiceFree, (uint8_t channel));
ORIG_FN(0x0046e9a0, int, winchanVoiceStart, (uint8_t channel));
/* flags: 1 stops immediately instead of releasing, 2 also frees the channel. */
ORIG_FN(0x0046eb60, int, winchanVoiceStop, (uint8_t channel, uint8_t flags));
ORIG_FN(0x0046ed20, int, winchanVoiceSetVolume, (uint8_t channel, uint8_t volume));
ORIG_FN(0x0046ee80, int, winchanVoiceSetPan, (uint8_t channel, int8_t pan));
/* Pitch offset in cents. */
ORIG_FN(0x0046efb0, int, winchanVoiceSetPitch, (uint8_t channel, int16_t cents));

#endif
