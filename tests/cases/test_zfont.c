/*
 * Tests for the font loader (zfont.c). Font data is generated; the IFF reader is stubbed. The loader
 * allocates through zmem, so the heap addresses of every table also check the order of allocations.
 */
#include <stddef.h>

#include "dt.h"
#include "dt_zmem.h"

#include "compat/crt.h"
#include "zge/zgemods/zfont.h"

static uint32_t gLog[64];
static uint32_t gLogN;
static const uint8_t *gChunk;
static uint32_t gChunkSize;

static void logU(uint32_t v)
{
    if (gLogN < sizeof gLog / sizeof gLog[0]) {
        gLog[gLogN] = v;
    }
    gLogN++;
}

static uint32_t BMX_CDECL stub_chunk_size(void *iff)
{
    logU(0x1000);
    return gChunkSize;
}

static int BMX_CDECL stub_read(void *iff, void *dst, uint32_t size)
{
    logU(0x1001);
    logU(size);
    memcpy(dst, gChunk, size);
    return 1;
}

static void BMX_CDECL stub_setup_callback(uint32_t id, int(BMX_CDECL *callback)(void *iff), int flags)
{
    logU(0x1002);
    logU(id);
    logU((uint32_t)(uintptr_t)callback);
    logU((uint32_t)flags);
}

static void put16(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}

/* A well formed font blob; returns its size. */
static uint32_t make_font(uint8_t *d, int version, int sets)
{
    uint8_t *p = d;
    int s;
    put16(p, (uint32_t)version);
    put16(p + 2, (uint32_t)sets);
    p += 4;
    for (s = 0; s < sets; s++) {
        const int glyphs = dt_u32() % 4 == 0 ? 0 : dt_range(1, 5);
        put16(p, (uint32_t)glyphs);
        p += 2;
        if (glyphs != 0) {
            const int ranges = dt_range(0, 3);
            int g, r, m;
            put16(p, (uint32_t)ranges);
            p += 2;
            for (g = 0; g < glyphs; g++) {
                dt_fill(p, 16);
                p += 16;
            }
            for (r = 0; r < ranges; r++) {
                const int n = dt_range(0, 4);
                put16(p, (uint32_t)n);
                p += 2;
                for (m = 0; m < n; m++) {
                    put16(p, (uint32_t)dt_range(0, glyphs - 1));
                    p += 2;
                }
            }
        }
    }
    return (uint32_t)(p - d);
}

static void begin(DtCtx *c)
{
    dt_seed(c);
    dt_zmem_env();
    gLogN = 0;
    memset(&gFont, 0, sizeof gFont);
    DT_ORIG(0x0048c110, void (*)(ZrwList *))(&gFont.fonts);
    gFont.initialized = 1;
}

static void setup_parse(DtCtx *c)
{
    uint8_t *d = dt_alloc(1024);
    int version = c->seed % 17 == 3 ? 0x100 : kZfontVersion;
    int sets = c->seed % 13 == 5 ? 0 : dt_range(1, 4);
    begin(c);
    make_font(d, version, sets);
    DT_ARG_PTR(c, 0, d);
}

static void setup_chunk(DtCtx *c)
{
    uint8_t *d = dt_alloc(1024);
    begin(c);
    dt_stub(0x0049abc0, NULL, (void *)stub_chunk_size);
    dt_stub(0x0049ac30, NULL, (void *)stub_read);
    gChunkSize = make_font(d, c->seed % 11 == 7 ? 0x102 : kZfontVersion, dt_range(1, 3));
    gChunk = d;
    DT_ARG_PTR(c, 0, (void *)0x20001234);
}

static void setup_init(DtCtx *c)
{
    begin(c);
    dt_stub(0x0049aad0, NULL, (void *)stub_setup_callback);
    dt_fill(&gFont, 16);
    gFont.initialized = c->seed % 4 == 1; /* seed 1, 5, ...: the "Already initialized" assert */
}

DT_CASES(zfont_cases) = {
    {"zfontParse", 0x004a2990, zfontParse, 1, DT_RET_INT, setup_parse, 300, 0},
    /* 0x4a29d4 is the second half of that function: a jump target, not callable on its own. */
    {"zfontParseBody", 0x004a2990, zfontParse, 1, DT_RET_INT, setup_parse, 200, 0},
    {"zfontChunkHandler", 0x004a2760, zfontChunkHandler, 1, DT_RET_INT, setup_chunk, 200, 0},
    {"zfontInitialize", 0x004a27a0, zfontInitialize, 0, DT_RET_INT, setup_init, 50, 0},
};
