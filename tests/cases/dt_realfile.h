/*
 * Environment for tests of file based code on the real game files (the harness serves the install
 * read-only: "Assets\Tables.ZAL", level data, ...) and on files made up in memory (RAM disk names).
 */
#ifndef DT_REALFILE_H
#define DT_REALFILE_H

#include <stdint.h>

#include "dt.h"
#include "dt_ngif.h"
#include "dt_zactor.h"
#include "dt_zmem.h"
#include "zge/zfile.h"

#define gRwEngine ORIG_VAR(0x0067696c, uint8_t *, gRwEngine)

/*
 * Debug print and (optionally) assert handler of ngBegin, a static region for zmemStaticAlloc, an engine
 * instance whose file interface is the exe's CRT stdio wrappers, and the original file layer (devices rw00
 * and rd00) on top of it.
 */
static inline void dt_real_env(const DtCtx *c, int initFiles)
{
    static const uint32_t kRwFile[10] = {0x4e8290, 0x4a5c98, 0x4a5cab, 0x51db3d, 0x51dc25,
                                         0x51dd2f, 0x51d4d2, 0x51dd86, 0x4a61ad, 0x4a8370};
    uint8_t *inst;

    ngBegin(c, 0);
    dt_static_region(0x40000);
    inst = dt_alloc(0x400);
    memcpy(inst + 0xc0, kRwFile, sizeof kRwFile);
    gRwEngine = inst;
    if (initFiles) {
        DT_ORIG(0x00483aa0, int (*)(void))();
    }
}

/*
 * The RAM disk file name "rd00:<address>:<size>" of a memory block, as the original RAM disk device builds
 * it, copied into a fresh buffer.
 */
static inline char *dt_ramdisk_name(const void *data, uint32_t size)
{
    const char *keys[2] = {"ADDRESS", "SIZE"};
    uint32_t values[2];
    const char *name;
    char *copy = dt_alloc(64);

    values[0] = (uint32_t)(uintptr_t)data;
    values[1] = size;
    name =
        DT_ORIG(0x0049ead0, const char *(*)(void *, int, const char **, uint32_t *))(NULL, 2, keys, values);
    memcpy(copy, name, strlen(name) + 1);
    return copy;
}

/* Little endian writers for the files made up by tests. */
static inline void dt_wr16(uint8_t *p, uint32_t off, uint32_t v)
{
    p[off] = (uint8_t)v;
    p[off + 1] = (uint8_t)(v >> 8);
}

static inline void dt_wr32(uint8_t *p, uint32_t off, uint32_t v)
{
    dt_wr16(p, off, v);
    dt_wr16(p, off + 2, v >> 16);
}

#endif
