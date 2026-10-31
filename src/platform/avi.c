/* AVIFIL32 stand-ins. The movie reader itself comes in slice S8 (docs/PLATFORM.md); until then the real DLL
 * runs. */
#include <string.h>

#include "platform/standins.h"

/*
 * AVISaveOptionsFree (AVIFIL32): zmovieClose frees each stream's lpFormat with the CRT (zmemFree) and then
 * hands the AVICOMPRESSOPTIONS to this function, which GlobalFrees lpFormat and lpParms and clears them. In
 * the original the block comes from MSVC6's own heap and that GlobalFree fails harmlessly; with our CRT the
 * block is the process heap's and the second free is fatal (heap corruption). Only the clearing remains.
 */
int32_t BMX_WINAPI bmxStandAVISaveOptionsFree(int count, void **options)
{
    for (int i = 0; i < count; i++) {
        uint8_t *const o = options[i];

        memset(o + 0x18, 0, 4); /* lpFormat */
        memset(o + 0x20, 0, 4); /* lpParms */
    }
    return 0;
}
