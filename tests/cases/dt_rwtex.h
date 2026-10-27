/*
 * Shared fixtures of the texture and light tests (rw/texture.c, rw/light.c): an engine instance that carries
 * the module data (textures, rasters, images, light free list, stream free list), free list entry points
 * that hand out zeroed blocks of 0x80 bytes (or fail on demand), a realloc, string functions and device
 * standard functions that log their arguments, on top of the call log of dt_rw.h.
 */
#ifndef DT_RWTEX_H
#define DT_RWTEX_H

#include "dt_rw.h"
#include "rw/rwlight.h"
#include "rw/rwstream.h"
#include "rw/rwtexture.h"

#define TEX_FN __attribute__((unused)) static

/* Plugin data after the engine's own: texture 0x20, raster 0x70, image 0x230, light 8, stream 8. */
enum {
    kTexOffTexture = kRwtEngineBytes,
    kTexOffRaster = kTexOffTexture + 0x20,
    kTexOffImage = kTexOffRaster + 0x70,
    kTexOffLight = kTexOffImage + 0x230,
    kTexOffStream = kTexOffLight + 8,
    kTexEngineBytes = kTexOffStream + 8,
};

static int gTexAllocCalls, gTexAllocFailAt; /* the n-th free list allocation fails (1-based) */
static uint32_t gTexOldSize;                /* bytes realloc copies */
static int gTexReallocFail;
static uint32_t gStdOk; /* bit n: the n-th standard function call succeeds */
static int gStdCalls;
static void *gTexPalette; /* what the lock palette function hands out */

TEX_FN void *BMX_CDECL texEntryAlloc(RwFreeList *freeList)
{
    rwtLog(0x4dbc30);
    rwtLogPtr(freeList);
    gTexAllocCalls++;
    if (gTexAllocFailAt != 0 && gTexAllocCalls == gTexAllocFailAt) {
        return NULL;
    }
    return dt_alloc(0x80);
}

TEX_FN void BMX_CDECL texEntryFree(RwFreeList *freeList, void *entry)
{
    rwtLog(0x4dbd90);
    rwtLogPtr(freeList);
    rwtLogPtr(entry);
}

TEX_FN void *BMX_CDECL texRealloc(void *block, size_t size)
{
    rwtLog(0x130);
    rwtLogPtr(block);
    rwtLog((uint32_t)size);
    if (gTexReallocFail) {
        return NULL;
    }
    uint8_t *fresh = dt_alloc((uint32_t)size);
    __builtin_memcpy(fresh, block, size < gTexOldSize ? size : gTexOldSize);
    return fresh;
}

/* ---- the engine's string functions ------------------------------------------------------------------ */

TEX_FN size_t BMX_CDECL texStrlen(const char *s)
{
    rwtLog(0x114);
    rwtLogPtr(s);
    size_t n = 0;
    while (s[n] != 0) {
        n++;
    }
    return n;
}

TEX_FN char *BMX_CDECL texStrcpy(char *dst, const char *src)
{
    rwtLog(0xf0);
    rwtLogPtr(dst);
    rwtLogPtr(src);
    size_t i = 0;
    do {
        dst[i] = src[i];
    } while (src[i++] != 0);
    return dst;
}

TEX_FN char *BMX_CDECL texStrncpy(char *dst, const char *src, size_t n)
{
    rwtLog(0xf4);
    rwtLogPtr(dst);
    rwtLogPtr(src);
    rwtLog((uint32_t)n);
    size_t i = 0;
    for (; i < n && src[i] != 0; i++) {
        dst[i] = src[i];
    }
    for (; i < n; i++) {
        dst[i] = 0;
    }
    return dst;
}

TEX_FN int BMX_CDECL texStrcmp(const char *a, const char *b)
{
    rwtLog(0x10c);
    rwtLogPtr(a);
    rwtLogPtr(b);
    size_t i = 0;
    while (a[i] != 0 && a[i] == b[i]) {
        i++;
    }
    return (int)(unsigned char)a[i] - (int)(unsigned char)b[i];
}

TEX_FN char *BMX_CDECL texStrupr(char *s)
{
    rwtLog(0x118);
    rwtLogPtr(s);
    for (char *p = s; *p != 0; p++) {
        if (*p >= 'a' && *p <= 'z') {
            *p = (char)(*p - 0x20);
        }
    }
    return s;
}

TEX_FN char *BMX_CDECL texStrlwr(char *s)
{
    rwtLog(0x11c);
    rwtLogPtr(s);
    for (char *p = s; *p != 0; p++) {
        if (*p >= 'A' && *p <= 'Z') {
            *p = (char)(*p + 0x20);
        }
    }
    return s;
}

/* ---- the device's standard functions ---------------------------------------------------------------- */

TEX_FN int texStd(int index, void *out, void *in, int32_t flags)
{
    rwtLog(0x1000 + (uint32_t)index);
    rwtLogPtr(out);
    rwtLogPtr(in);
    rwtLog((uint32_t)flags);
    switch (index) {
    case rwSTANDARDRASTERCREATE: {
        RwRaster *raster = in;
        raster->type = (uint8_t)dt_range(0, 5);
        raster->format = (uint8_t)dt_u32();
        raster->stride = dt_range(0, 512);
        break;
    }
    case rwSTANDARDRASTERLOCKPALETTE:
        *(void **)out = gTexPalette;
        break;
    case rwSTANDARDPIXELTORGB:
        rwtLog(*(uint32_t *)in);
        *(uint32_t *)out = dt_u32();
        break;
    default:
        break;
    }
    return (int)((gStdOk >> (gStdCalls++ & 31)) & 1);
}

#define TEX_STD(n)                                                                                           \
    TEX_FN int BMX_CDECL texStd##n(void *out, void *in, int32_t flags)                                       \
    {                                                                                                        \
        return texStd(n, out, in, flags);                                                                    \
    }
TEX_STD(3)
TEX_STD(4)
TEX_STD(5)
TEX_STD(7)
TEX_STD(8)
TEX_STD(11)
TEX_STD(12)
TEX_STD(20)
TEX_STD(23)
TEX_STD(24)

/* A free list stand-in: the logs only need an address to identify it by. */
TEX_FN RwFreeList *texFreeListId(void)
{
    return dt_alloc(4);
}

/* A new engine instance: the dt_rw.h one, enlarged by the module data and with the stubs installed. */
TEX_FN RwGlobals *texEnv(DtCtx *c)
{
    dt_seed(c);
    RwGlobals *base = rwtEnv();
    RwGlobals *g = dt_alloc(kTexEngineBytes);
    __builtin_memcpy(g, base, kRwtEngineBytes);
    RwEngineInstance = g;
    g->freeListAlloc = texEntryAlloc;
    g->freeListFree = texEntryFree;
    g->memoryFuncs.rwrealloc = texRealloc;
    g->stringFuncs.rwstrlen = (void *)texStrlen;
    g->stringFuncs.rwstrcpy = texStrcpy;
    g->stringFuncs.rwstrncpy = (void *)texStrncpy;
    g->stringFuncs.rwstrcmp = (void *)texStrcmp;
    g->stringFuncs.rwstrupr = (void *)texStrupr;
    g->stringFuncs.rwstrlwr = (void *)texStrlwr;
    g->stdFunc[rwSTANDARDPIXELTORGB] = (uint32_t)(uintptr_t)texStd3;
    g->stdFunc[rwSTANDARDRASTERCREATE] = (uint32_t)(uintptr_t)texStd4;
    g->stdFunc[rwSTANDARDRASTERDESTROY] = (uint32_t)(uintptr_t)texStd5;
    g->stdFunc[rwSTANDARDRASTERSETIMAGE] = (uint32_t)(uintptr_t)texStd7;
    g->stdFunc[rwSTANDARDTEXTURESETRASTER] = (uint32_t)(uintptr_t)texStd8;
    g->stdFunc[rwSTANDARDSETRASTERCONTEXT] = (uint32_t)(uintptr_t)texStd11;
    g->stdFunc[rwSTANDARDRASTERSUBRASTER] = (uint32_t)(uintptr_t)texStd12;
    g->stdFunc[rwSTANDARDRASTERSHOWRASTER] = (uint32_t)(uintptr_t)texStd20;
    g->stdFunc[rwSTANDARDRASTERLOCKPALETTE] = (uint32_t)(uintptr_t)texStd23;
    g->stdFunc[rwSTANDARDRASTERUNLOCKPALETTE] = (uint32_t)(uintptr_t)texStd24;
    rwTextureOffset = kTexOffTexture;
    rwRasterOffset = kTexOffRaster;
    rwImageOffset = kTexOffImage;
    rpLightOffset = kTexOffLight;
    rwStreamOffset = kTexOffStream;
    RwTextureGlobals *tg = (RwTextureGlobals *)((uint8_t *)g + kTexOffTexture);
    tg->dictList.next = tg->dictList.prev = &tg->dictList;
    tg->textureFreeList = texFreeListId();
    tg->dictFreeList = texFreeListId();
    RwRasterGlobals *rg = (RwRasterGlobals *)((uint8_t *)g + kTexOffRaster);
    rg->freeList = texFreeListId();
    RwImageGlobals *ig = (RwImageGlobals *)((uint8_t *)g + kTexOffImage);
    ig->imageFreeList = texFreeListId();
    ig->formatFreeList = texFreeListId();
    *(RwFreeList **)((uint8_t *)g + kTexOffLight) = texFreeListId();
    *(RwFreeList **)((uint8_t *)g + kTexOffStream) = texFreeListId();
    gTexAllocCalls = gTexAllocFailAt = 0;
    gTexOldSize = 0;
    gTexReallocFail = 0;
    gStdOk = dt_u32() % 4 == 0 ? dt_u32() : 0xffffffffu;
    gStdCalls = 0;
    gTexPalette = dt_alloc(8);
    return g;
}

TEX_FN RwTextureGlobals *texGlobals(void)
{
    return (RwTextureGlobals *)((uint8_t *)RwEngineInstance + kTexOffTexture);
}

TEX_FN RwRasterGlobals *rasterGlobals(void)
{
    return (RwRasterGlobals *)((uint8_t *)RwEngineInstance + kTexOffRaster);
}

TEX_FN RwImageGlobals *imageGlobals(void)
{
    return (RwImageGlobals *)((uint8_t *)RwEngineInstance + kTexOffImage);
}

/* ---- object builders -------------------------------------------------------------------------------- */

/* A raster with random fields (edge sizes included); flags hold bit 7 half of the time. */
TEX_FN RwRaster *mkRaster(void)
{
    RwRaster *r = dt_alloc(0x30);
    r->parent = dt_u32() % 3 == 0 ? NULL : r;
    r->width = dt_u32() % 5 == 0 ? (int32_t)dt_u32() : dt_range(0, 300);
    r->height = dt_u32() % 5 == 0 ? (int32_t)dt_u32() : dt_range(0, 300);
    r->depth = dt_range(0, 32);
    r->stride = dt_range(0, 1200);
    r->originX = (int16_t)(dt_u32() % 4 == 0 ? dt_u32() : (uint32_t)dt_range(-40, 40));
    r->originY = (int16_t)(dt_u32() % 4 == 0 ? dt_u32() : (uint32_t)dt_range(-40, 40));
    r->type = (uint8_t)dt_range(0, 6);
    r->flags = (uint8_t)(dt_u32() % 2 ? 0x80 | (dt_u32() & 0x7f) : dt_u32() & 0x7f);
    r->privateFlags = (uint8_t)dt_u32();
    r->format = (uint8_t)dt_u32();
    r->stamp = (uint8_t)dt_u32();
    r->pixels = dt_u32() % 2 ? dt_alloc(8) : NULL;
    r->palette = dt_u32() % 2 ? dt_alloc(8) : NULL;
    return r;
}

TEX_FN char *mkName(const char *s)
{
    char *p = dt_alloc((uint32_t)__builtin_strlen(s) + 1);
    __builtin_memcpy(p, s, __builtin_strlen(s));
    return p;
}

/* A name drawn from a small pool, spelled in different cases, so lookups find, miss and nearly match. */
TEX_FN const char *poolName(void)
{
    static const char *const pool[] = {"wall", "Wall", "WALL", "brick", "BRICK2",
                                       "",     "a",    "Zeta", "zeta_", "road"};
    return pool[dt_u32() % (sizeof pool / sizeof pool[0])];
}

/* A texture with a raster (or none), a name (or none) and random counts; not in any dictionary. */
TEX_FN RwTexture *mkTexture(void)
{
    RwTexture *t = dt_alloc(0x28);
    t->raster = dt_u32() % 4 == 0 ? NULL : mkRaster();
    t->stamp = (uint8_t)dt_u32();
    t->name = dt_u32() % 5 == 0 ? NULL : mkName(poolName());
    t->mask = dt_u32() % 3 == 0 ? NULL : mkName(poolName());
    t->refCount = dt_u32() % 3 == 0 ? (uint32_t)dt_range(0, 6) : 1;
    t->filterMode = dt_range(0, 9);
    t->addressing = dt_range(0, 9);
    return t;
}

TEX_FN void linkInsert(RwLLLink *head, RwLLLink *link)
{
    link->next = head->next;
    link->prev = head;
    head->next->prev = link;
    head->next = link;
}

/* Puts the texture into the dictionary the way RenderWare does (the list is well-formed afterwards). */
TEX_FN void dictPut(RwTexDictionary *d, RwTexture *t)
{
    t->dict = d;
    linkInsert(&d->texturesInDict, &t->lInDictionary);
}

/* A dictionary with n textures; it is also put into the global list when `registered`. */
TEX_FN RwTexDictionary *mkDict(int n, int registered)
{
    RwTexDictionary *d = dt_alloc(sizeof *d);
    d->object.type = 6;
    d->stamp = (uint8_t)dt_u32();
    d->texturesInDict.next = d->texturesInDict.prev = &d->texturesInDict;
    for (int i = 0; i < n; i++) {
        dictPut(d, mkTexture());
    }
    if (registered) {
        linkInsert(&texGlobals()->dictList, &d->lInInstance);
    }
    return d;
}

/* An image with random geometry; pixels and palette present when `withPixels`. */
TEX_FN RwImage *mkImage(int withPixels)
{
    RwImage *im = dt_alloc(0x20);
    static const int32_t depths[] = {4, 8, 32, 16, 1};
    im->width = dt_range(0, 40);
    im->height = dt_range(0, 40);
    im->depth = depths[dt_u32() % 5];
    im->stamp = (uint8_t)dt_u32();
    if (withPixels) {
        im->flags = dt_u32() % 5 == 0 ? 0 : 1;
        im->stride = dt_range(0, 200);
        im->pixels = dt_alloc(16);
        im->palette = dt_u32() % 2 ? dt_alloc(16) : NULL;
        if (dt_u32() % 6 == 0) {
            im->pixels = NULL;
        }
    } else {
        im->flags = dt_u32() % 6 == 0 ? 1 : 0;
    }
    return im;
}

#endif
