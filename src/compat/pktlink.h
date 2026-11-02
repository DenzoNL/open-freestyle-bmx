/*
 * The links of the PSX-style display lists (ngifpsx.c, zoverii.c's overlay pools, the menu polygons, wide.c):
 * the low 28 bits of a packet's first word name the next packet, the top nibble is the packet's tag, and
 * 0x0fffffff ends a chain (docs/PORTING.md "X2d").
 *
 * On 32-bit builds a link is the packet's address, as in the original. On 64-bit builds it is the packet's
 * signed distance from bmxPacketAnchor: every packet and ordering table is static data, a few MB from the
 * anchor, well inside the 28 bits.
 */
#ifndef COMPAT_PKTLINK_H
#define COMPAT_PKTLINK_H

#include <stdint.h>

enum {
    kBmxPacketLinkMask = 0x0fffffff, /* the link bits of a packet word */
    kBmxPacketEnd = 0x0fffffff,      /* the link of the last packet of a chain */
};

#if UINTPTR_MAX == 0xffffffffu

/* The link that names packet p. */
static inline uint32_t bmxPacketLink(const void *p)
{
    return (uint32_t)(uintptr_t)p & kBmxPacketLinkMask;
}

/* The packet a link names (the link bits of `word`; the tag nibble is ignored). */
static inline void *bmxPacketAt(uint32_t word)
{
    return (void *)(uintptr_t)(word & kBmxPacketLinkMask);
}

#else

extern char bmxPacketAnchor;
/* Stops the program: packet p is too far from the anchor to be linked (not static data). */
_Noreturn void bmxPacketLinkFail(const void *p);

static inline uint32_t bmxPacketLink(const void *p)
{
    const intptr_t distance = (intptr_t)p - (intptr_t)&bmxPacketAnchor;

    /* -0x08000000 .. 0x07fffffe: the end marker (-1 as a distance) is never a packet. */
    if (distance < -0x08000000 || distance >= 0x07ffffff) {
        bmxPacketLinkFail(p);
    }
    return (uint32_t)distance & kBmxPacketLinkMask;
}

static inline void *bmxPacketAt(uint32_t word)
{
    int32_t distance = (int32_t)(word & kBmxPacketLinkMask);

    if ((distance & 0x08000000) != 0) {
        distance -= 0x10000000;
    }
    return &bmxPacketAnchor + distance;
}

#endif

#endif
