/*
 * Asset library files, .ZAL (C:\NgEngine\Zge\zasset.c). A library is a directory of groups of assets; a
 * group is {id, then `assetsPerGroup` entries of {offset, size, [stored size]}}. Entries are either plain
 * data, compressed (stored size set, method 0 or 1), or names of loose files ("linked" libraries).
 * A library is handled as an int (its address); one cursor selects the asset zassetSize / zassetRead use.
 */
#ifndef ZGE_ZASSET_H
#define ZGE_ZASSET_H

#include <stdint.h>

#include "compat/orig.h"
#include "zge/zfile.h"
#include "zge/zmem.h"

/* An open library (0x128 bytes from a free list). */
typedef struct ZassetLibrary {
    ZfileHandle *file; /* 0x00 */
    uint32_t pad04;
    uint16_t groups;         /* 0x08 */
    uint16_t perGroup;       /* 0x0a assets per group */
    uint8_t *cursor;         /* 0x0c selected entry (inside the directory) */
    uint8_t *directory;      /* 0x10 */
    uint32_t directoryBytes; /* 0x14 */
    uint16_t entryBytes;     /* 0x18 8, or 12 with stored sizes */
    uint16_t stored;         /* 0x1a entries carry a stored size (compressed) */
    uint16_t method;         /* 0x1c decompression method */
    uint8_t linked;          /* 0x1e entries name loose files */
    char linkName[0x101];    /* 0x1f file name read for a linked entry */
    uint8_t *cachedEntry;    /* 0x120 the entry whose file size is cached below */
    uint32_t cachedSize;     /* 0x124 */
} ZassetLibrary;

_Static_assert(__builtin_offsetof(ZassetLibrary, cursor) == 0xc, "zasset layout");
_Static_assert(__builtin_offsetof(ZassetLibrary, entryBytes) == 0x18, "zasset layout");
_Static_assert(__builtin_offsetof(ZassetLibrary, linkName) == 0x1f, "zasset layout");
_Static_assert(__builtin_offsetof(ZassetLibrary, cachedEntry) == 0x120, "zasset layout");
_Static_assert(sizeof(ZassetLibrary) == 0x128, "zasset size");

/* Free list of open libraries (0x65e2b0), created with the first library and destroyed with the last. */
#define gZassetLibraries ORIG_VAR(0x0065e2b0, ZMemFreeList *, gZassetLibraries)

/* Opens a library file; returns a handle or 0. */
int zassetOpenLibrary(const char *name);
void zassetCloseLibrary(int lib);
/* Selects asset `asset` of group `group`; returns 1 when it exists. */
int zassetFind(int lib, int group, int asset);
/* Prints the group and asset sizes of the library. */
void zassetDump(int lib);
/* Size in bytes of the selected resource. */
uint32_t zassetSize(int lib);
/* Reads the selected resource into dst and steps to the next asset of the group; returns 1 on success. */
int zassetRead(int lib, void *dst);

/* Number of groups in the library, and a step to the next one (the selection of the loop in zscene.c). */
int zassetCount(int lib);
int zassetNext(int lib);

/* Decompressor of the renderer (kind 1 or 2): source, size, destination, bytes written. */
int zassetDecompress(int kind, const void *packed, uint32_t size, void *dest, uint32_t *outSize);

#endif
