/* Shared fixtures for the zprim tests (test_zprim.c): DMM pool, display list, renderer stubs, fake library.
 */
#ifndef DT_ZPRIM_H
#define DT_ZPRIM_H

#include <stdint.h>

#include "bmx/com/zge/zprim.h"
#include "dt.h"
#include "dt_zactor.h"
#include "dt_zmem.h"
#include "zge/zmem.h"

typedef int (*DtDmmInitFn)(int, uint32_t);
typedef int32_t (*DtDmmAllocFn)(uint32_t);
typedef void *(*DtDmmLockFn)(int32_t);
typedef void (*DtDmmUnlockFn)(int32_t);
typedef ZPrimAsset *(*DtAssetAllocFn)(void);
typedef int (*DtVoidIntFn)(void);
typedef void (*DtVoidFn)(void);
typedef struct ZMemFreeList *(*DtFreeListCreateFn)(uint32_t, uint32_t, void *);

#define DT_DMM_INIT   DT_ORIG(0x0046aca0, DtDmmInitFn)
#define DT_DMM_ALLOC  DT_ORIG(0x0046b030, DtDmmAllocFn)
#define DT_DMM_LOCK   DT_ORIG(0x0046b470, DtDmmLockFn)
#define DT_DMM_UNLOCK DT_ORIG(0x0046b4f0, DtDmmUnlockFn)
#define DT_ASSET_NEW  DT_ORIG(0x0047b830, DtAssetAllocFn)
#define DT_ZPRIM_INIT DT_ORIG(0x004779e0, DtVoidIntFn)
#define DT_NGIF_BEGIN DT_ORIG(0x00463ab0, DtVoidFn)

#define gNgifObject      ORIG_VAR(0x005d2508, int32_t, gNgifObject)
#define gFileSystemReady ORIG_VAR(0x00670674, int32_t, gFileSystemReady)

/* What the renderer stubs saw. Part of the DLL's data, so the harness compares it between both runs. */
typedef struct DtRwLog {
    uint32_t loads;
    uint32_t frees;
    uint32_t clones;
    uint32_t freeSum;
    uint32_t hash[8]; /* FNV-1a of the last bitmaps handed to the loader */
    uint32_t sizes[8];
    char names[8][16];
} DtRwLog;

extern DtRwLog dt_rw_log;

static inline uint32_t dt_fnv(const uint8_t *p, uint32_t n)
{
    uint32_t h = 2166136261u;
    for (uint32_t i = 0; i < n; i++) {
        h = (h ^ p[i]) * 16777619u;
    }
    return h;
}

int32_t BMX_CDECL dt_rw_load(const void *bmp, uint32_t size, const char *name, int a, int b);
void BMX_CDECL dt_rw_free(int32_t texture);
int32_t BMX_CDECL dt_rw_clone(int32_t texture);

/* A library in memory: id table plus data blobs, readable through the exe's own zasset / zfile code. */
typedef struct DtFakeLib {
    uint8_t *lib;
    uint8_t *file;
    uint32_t fileSize;
    uint32_t used;
    uint32_t count;
} DtFakeLib;

void dt_fake_lib_begin(DtFakeLib *l, uint32_t maxEntries, uint32_t maxBytes);
void dt_fake_lib_add(DtFakeLib *l, uint32_t id, const void *data, uint32_t size);

/* Brings up the DMM pool, an initialised zprim, the display list and the renderer stubs. */
static inline void dt_zprim_env(void)
{
    dt_static_region(0x80000);
    dt_zmem_env();
    gNgifObject = 1;
    gFileSystemReady = 1;
    DT_DMM_INIT(0, 0x40000);
    gZprimFontPageList = DT_ORIG(0x00485e80, DtFreeListCreateFn)(0x80, 4, NULL);
    gRwTextureFree = dt_rw_free;
    gRwTextureClone = dt_rw_clone;
    gRwTextureLoad = dt_rw_load;
    DT_ZPRIM_INIT();
    DT_NGIF_BEGIN();
}

/* Asset node with `size` bytes of data in the DMM pool (random content); id and flags as given. */
static inline ZPrimAsset *dt_new_asset(uint16_t id, uint16_t flags, uint32_t size)
{
    ZPrimAsset *a = DT_ASSET_NEW();
    a->id = id;
    a->flags = flags;
    a->dmm = DT_DMM_ALLOC(size);
    a->offset = 0;
    uint8_t *d = DT_DMM_LOCK(a->dmm);
    dt_fill(d, size);
    DT_DMM_UNLOCK(a->dmm);
    return a;
}

/* Data of an asset node for the test to fill in; the caller unlocks with dt_asset_unlock. */
static inline uint8_t *dt_asset_data(const ZPrimAsset *a)
{
    return (uint8_t *)DT_DMM_LOCK(a->dmm) + a->offset;
}

static inline void dt_asset_unlock(const ZPrimAsset *a)
{
    DT_DMM_UNLOCK(a->dmm);
}

/* ---- asset blobs -------------------------------------------------------------------------------- */

typedef struct DtBlob {
    uint8_t *data;
    uint32_t size;
} DtBlob;

static inline uint16_t dt_rand_color(void)
{
    return (dt_u32() % 5 == 0) ? 0x7c1f : (uint16_t)(dt_u32() & 0x7fff); /* magenta is the colour key */
}

/* Palette block {flags, count, colours}. */
static inline uint8_t *dt_put_palette(uint8_t *p, uint32_t count)
{
    *(uint16_t *)p = 0;
    *(uint16_t *)(p + 2) = (uint16_t)count;
    uint16_t *c = (uint16_t *)(p + 4);
    for (uint32_t i = 0; i < count; i++) {
        c[i] = dt_rand_color();
    }
    return p + 4 + count * 2;
}

/* Picture asset: header, palette, pixels and a second pixel set (the "VRAM" copy). */
static inline DtBlob dt_gen_picture(uint32_t w, uint32_t h, uint32_t bpp, uint32_t colors)
{
    const uint32_t rowBytes = (uint32_t)((uint64_t)w / (8 / (uint64_t)bpp));
    const uint32_t pixelBytes = rowBytes * h + 4;
    const uint32_t palBytes = (4 + colors * 2 + 3) & ~3u;
    DtBlob b = {dt_alloc(0x24 + palBytes + 2 * pixelBytes + 16), 0x24 + palBytes + 2 * pixelBytes + 16};
    uint8_t *d = b.data;
    dt_fill(d, 0x24);
    *(uint16_t *)(d + 4) = (uint16_t)w;
    *(uint16_t *)(d + 6) = (uint16_t)h;
    *(uint16_t *)(d + 0x10) = (uint16_t)bpp;
    *(int32_t *)(d + 0xc) = 0x24;
    *(int32_t *)(d + 8) = (int32_t)(0x24 + palBytes);
    *(int32_t *)(d + 0x18) = (int32_t)(0x24 + palBytes + pixelBytes);
    *(int32_t *)(d + 0x20) = 0;
    dt_put_palette(d + 0x24, colors);
    dt_fill(d + 0x24 + palBytes, 2 * pixelBytes);
    return b;
}

/* Sprite set with `frames` frames; `perFramePalette` sets flag bit 0 (a 0x204 byte palette per frame). */
static inline DtBlob dt_gen_sprite(uint32_t frames, uint32_t w, uint32_t h, uint32_t bpp, int perFramePalette)
{
    const uint32_t rowBytes = (uint32_t)((uint64_t)w / (8 / (uint64_t)bpp));
    const uint32_t pixelBytes = (rowBytes * h + 3) & ~3u;
    const uint32_t entries = 0x14 + frames * 16;
    const uint32_t palBytes = perFramePalette ? frames * 0x204 + 0x204 : 0x204;
    const uint32_t total = entries + frames * pixelBytes + palBytes + 16;
    DtBlob b = {dt_alloc(total), total};
    uint8_t *d = b.data;
    dt_fill(d, total);
    *(uint16_t *)d = (uint16_t)frames;
    *(uint16_t *)(d + 2) = (uint16_t)(perFramePalette ? 1 : 0);
    *(uint16_t *)(d + 4) = (uint16_t)w;
    *(uint16_t *)(d + 6) = (uint16_t)h;
    *(int32_t *)(d + 8) = (int32_t)entries;
    *(int32_t *)(d + 0xc) = (int32_t)(entries + frames * pixelBytes);
    *(uint16_t *)(d + 0x10) = (uint16_t)bpp;
    for (uint32_t i = 0; i < frames; i++) {
        uint8_t *e = d + 0x14 + i * 16;
        *(uint16_t *)e = (uint16_t)w;
        *(uint16_t *)(e + 2) = (uint16_t)h;
        *(int32_t *)(e + 4) = (int32_t)(entries + i * pixelBytes);
        *(uint16_t *)(e + 8) = (uint16_t)dt_range(0, 20);
        *(uint16_t *)(e + 10) = (uint16_t)dt_range(0, 20);
        *(int32_t *)(e + 0xc) = 0;
    }
    const uint32_t colors = 1u << bpp;
    uint8_t *pal = d + entries + frames * pixelBytes;
    for (uint32_t f = 0; f < (perFramePalette ? frames : 1); f++) {
        dt_put_palette(pal + f * 0x204, colors < 256 ? colors : 256);
    }
    return b;
}

/*
 * Font asset. Small fonts (fewer than 0x100 glyphs) map consecutive codes starting at `first`; big ones carry
 * an explicit code per glyph.
 */
static inline DtBlob dt_gen_font(uint32_t glyphs, uint32_t first)
{
    const uint32_t sheetW = 32;
    const uint32_t sheetH = 16;
    const uint32_t palBytes =
        4 + 64 * 2; /* 4 pages of 16 colours: a page past the end would read stale stack */
    const uint32_t total = 0x18 + glyphs * 10 + 0x20 + palBytes + sheetW * sheetH + 16;
    DtBlob b = {dt_alloc(total), total};
    uint8_t *d = b.data;
    dt_fill(d, total);
    const uint32_t tableEnd = 0x18 + glyphs * 10;
    *(uint16_t *)d = (uint16_t)glyphs;
    *(uint16_t *)(d + 2) = (uint16_t)dt_u32();
    *(int32_t *)(d + 4) = (int32_t)(tableEnd + palBytes + 4);
    *(uint16_t *)(d + 8) = (uint16_t)sheetW;
    *(uint16_t *)(d + 10) = (uint16_t)sheetH;
    *(int32_t *)(d + 0xc) = 0;
    *(int32_t *)(d + 0x10) = (int32_t)tableEnd;
    *(uint16_t *)(d + 0x14) = 4;
    for (uint32_t i = 0; i < glyphs; i++) {
        uint8_t *g = d + 0x18 + i * 10;
        g[0] = (uint8_t)dt_range(0, 20);
        g[1] = (uint8_t)dt_range(0, 8);
        g[2] = (uint8_t)dt_range(0, 8);
        g[3] = (uint8_t)dt_range(0, 8);
        g[4] = (uint8_t)dt_range(-3, 3);
        g[5] = (uint8_t)dt_range(-3, 3);
        g[6] = (uint8_t)dt_range(-2, 4);
        g[7] = (uint8_t)dt_range(-2, 4);
        *(uint16_t *)(g + 8) = (uint16_t)(glyphs < 0x100 ? first + i : 0x20 + i * 7 % 0x1000);
    }
    dt_put_palette(d + tableEnd, 64);
    uint8_t *sheet = d + tableEnd + palBytes + 4;
    for (uint32_t i = 0; i < sheetW * sheetH; i++) {
        sheet[i] = (uint8_t)(dt_u32() % 4 == 0 ? 0 : (dt_u32() % 3 == 0 ? 1 : dt_range(2, 15)));
    }
    return b;
}

#endif
