/*
 * Renderer side of the RAM disk (C:\NgEngine\Zge\Com\ZglRw\zrwRamDisk.c): the renderer's file open hook
 * turns the memory block set with zrwFileRamDiskSetInf into a RAM disk file.
 */
#ifndef ZGE_COM_ZGLRW_ZRWRAMDISK_H
#define ZGE_COM_ZGLRW_ZRWRAMDISK_H

#include <stdint.h>

#include "compat/orig.h"
#include "zge/zfile.h"

/* The memory block to open (set by zrwFileRamDiskSetInf) and the file extension the hook accepts. */
typedef struct ZrwRamDiskInfo {
    uint32_t address;
    uint32_t size;
    const char *extension; /* NULL accepts every path */
} ZrwRamDiskInfo;

#define gZrwRamDiskInfo ORIG_VAR(0x00676408, ZrwRamDiskInfo *, gZrwRamDiskInfo)
/* Value gZrwRamDiskInfo has when no block was set. */
#define gZrwRamDiskNoInfo ORIG_VAR(0x00676404, ZrwRamDiskInfo *, gZrwRamDiskNoInfo)
/* The one open RAM disk file (0 when none) and the cached RAM disk device (-1 until found). */
#define gZrwRamDiskFile   ORIG_VAR(0x006763e8, ZfileHandle *, gZrwRamDiskFile)
#define gZrwRamDiskDevice ORIG_VAR(0x00563874, intptr_t, gZrwRamDiskDevice)

/* Opens the set memory block as a RAM disk file; returns the address of gZrwRamDiskFile or NULL. */
ZfileHandle **zrwfopen(void);
/* 1 when the extension of `path` matches the one set (or none is set). */
int zrwRamDiskMatches(const char *path);

/* TODO(file): bounding box helpers that probably belong to the renderer's collision code. */
/* Box = min corner (x, y, z), pad, max corner. 1 when the point is inside (NaN corners count as inside). */
int zrwBBoxContainsPoint(const float *box, const float *point);
/* Clips the segment a-b to the box in place; returns 0 when it misses it. */
int zrwBBoxClipSegment(float *a, float *b, const float *box);

#endif
