/* The anchor of the 64-bit packet links (compat/pktlink.h); 32-bit builds link by address and need nothing.
 */
#include "compat/pktlink.h"

#if UINTPTR_MAX != 0xffffffffu

#include <stdio.h>
#include <stdlib.h>

char bmxPacketAnchor;

_Noreturn void bmxPacketLinkFail(const void *p)
{
    fprintf(stderr, "packet %p is too far from the link anchor %p to be linked\n", p,
            (void *)&bmxPacketAnchor);
    abort();
}

#else

typedef int BmxPktlinkUnused; /* keeps the unit non-empty */

#endif
