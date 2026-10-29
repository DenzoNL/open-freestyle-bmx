/*
 * Shared fixtures of the RenderWare core tests (rw/core.c, rw/engine.c, rw/plugins.c): a call log, an engine
 * instance whose memory interface, free list entry points and device are logging stubs, plugin registry
 * builders and scripted stream functions.
 *
 * Everything the stubs record goes through dt_canon() and is compared between the original and the port.
 */
#ifndef DT_RW_H
#define DT_RW_H

#include <stdint.h>

#include "compat/crt.h"
#include "dt.h"
#include "dt_zmem.h"
#include "rw/rwcore.h"

#define RWT_FN __attribute__((unused)) static

enum { kRwtLogWords = 8192 };
static uint32_t gRwtLog[kRwtLogWords];
static uint32_t gRwtLogN;

static inline void rwtLog(uint32_t v)
{
    if (gRwtLogN < kRwtLogWords) {
        gRwtLog[gRwtLogN] = v;
    }
    gRwtLogN++;
}

static inline void rwtLogFloatBits(float f)
{
    uint32_t u;
    __builtin_memcpy(&u, &f, 4);
    rwtLog(u);
}

static inline void rwtLogWords(const void *p, int n)
{
    for (int i = 0; i < n; i++) {
        rwtLog(((const uint32_t *)p)[i]);
    }
}

static inline void rwtLogPtr(const void *p)
{
    rwtLog(dt_canon((uint32_t)(uintptr_t)p));
}

/* Failure injection: the n-th (1-based) call of the kind fails; 0 never fails. */
static int gRwtMallocCalls, gRwtMallocFailAt;
static int gRwtEntryCalls, gRwtEntryFailAt;

RWT_FN void *BMX_CDECL rwtMalloc(size_t size)
{
    rwtLog(0x4a50d2);
    rwtLog((uint32_t)size);
    gRwtMallocCalls++;
    if (gRwtMallocFailAt != 0 && gRwtMallocCalls == gRwtMallocFailAt) {
        return NULL;
    }
    return dt_alloc((uint32_t)size);
}

RWT_FN void BMX_CDECL rwtFree(void *block)
{
    rwtLog(0x4a4d7f);
    rwtLogPtr(block);
}

/* Free list entry points of the engine (0x4dbc30 / 0x4dbd90 in the exe): entries are 0x34 bytes. */
RWT_FN void *BMX_CDECL rwtEntryAlloc(RwFreeList *freeList)
{
    rwtLog(0x4dbc30);
    rwtLogPtr(freeList);
    gRwtEntryCalls++;
    if (gRwtEntryFailAt != 0 && gRwtEntryCalls == gRwtEntryFailAt) {
        return NULL;
    }
    return dt_alloc(sizeof(RwPluginRegEntry));
}

RWT_FN void BMX_CDECL rwtEntryFree(RwFreeList *freeList, void *entry)
{
    rwtLog(0x4dbd90);
    rwtLogPtr(freeList);
    rwtLogPtr(entry);
}

/* Plugin data of the engine instance, after the RwGlobals: error slot, resource arena, matrix globals. */
enum {
    kRwtErrorOffset = sizeof(RwGlobals),
    kRwtResourcesOffset = kRwtErrorOffset + sizeof(RwError),
    kRwtMatrixOffset = kRwtResourcesOffset + sizeof(RwResourcesGlobals),
    kRwtSqrtOffset = kRwtMatrixOffset + 0x10,
    kRwtEngineBytes = kRwtSqrtOffset + 8,
};

static inline RwError *rwtErrorSlot(RwGlobals *g)
{
    return (RwError *)((uint8_t *)g + kRwtErrorOffset);
}

static inline RwResourcesGlobals *rwtResources(RwGlobals *g)
{
    return (RwResourcesGlobals *)((uint8_t *)g + kRwtResourcesOffset);
}

/*
 * A fresh engine instance (becomes RwEngineInstance) with an empty error slot. Its memory interface logs and
 * allocates from the exe's CRT heap; the module counters and failure injection start cleared.
 */
RWT_FN RwGlobals *rwtEnv(void)
{
    RwGlobals *g = dt_alloc(kRwtEngineBytes);
    g->memoryFuncs.rwmalloc = rwtMalloc;
    g->memoryFuncs.rwfree = rwtFree;
    g->freeListAlloc = rwtEntryAlloc;
    g->freeListFree = rwtEntryFree;
    rwtErrorSlot(g)->pluginID = 0;
    rwtErrorSlot(g)->errorCode = rwERRNONE;
    RwEngineInstance = g;
    rwErrorOffset = kRwtErrorOffset;
    rwResourcesOffset = kRwtResourcesOffset;
    rwMatrixOffset = kRwtMatrixOffset;
    rwFastSqrtOffset = kRwtSqrtOffset;
    rwEngineInstanceCount = 0;
    gRwtLogN = 0;
    gRwtMallocCalls = gRwtMallocFailAt = 0;
    gRwtEntryCalls = gRwtEntryFailAt = 0;
    return g;
}

/* The log is cleared at the end of a setup, so only the call under test is recorded. */
static inline void rwtEnd(void)
{
    gRwtLogN = 0;
    gRwtMallocCalls = 0;
    gRwtEntryCalls = 0;
}

/* ---- plugin callbacks ------------------------------------------------------------------------------- */

static int gRwtCtorCalls, gRwtCtorFailAt;
static int gRwtCallbackFails; /* bit n: the n-th stream callback call fails */
static int gRwtCallbackCalls;

/* The macros below define functions: their names can't be parenthesised. */
// NOLINTBEGIN(bugprone-macro-parentheses)
#define RWT_CTOR(name, id)                                                                                   \
    RWT_FN void *BMX_CDECL name(void *object, int32_t offset, int32_t size)                                  \
    {                                                                                                        \
        rwtLog(id);                                                                                          \
        rwtLogPtr(object);                                                                                   \
        rwtLog((uint32_t)offset);                                                                            \
        rwtLog((uint32_t)size);                                                                              \
        gRwtCtorCalls++;                                                                                     \
        return gRwtCtorFailAt != 0 && gRwtCtorCalls == gRwtCtorFailAt ? NULL : object;                       \
    }
#define RWT_DTOR(name, id)                                                                                   \
    RWT_FN void *BMX_CDECL name(void *object, int32_t offset, int32_t size)                                  \
    {                                                                                                        \
        rwtLog(id);                                                                                          \
        rwtLogPtr(object);                                                                                   \
        rwtLog((uint32_t)offset);                                                                            \
        rwtLog((uint32_t)size);                                                                              \
        return object;                                                                                       \
    }
#define RWT_COPY(name, id)                                                                                   \
    RWT_FN void *BMX_CDECL name(void *dst, const void *src, int32_t offset, int32_t size)                    \
    {                                                                                                        \
        rwtLog(id);                                                                                          \
        return dst;                                                                                          \
    }
#define RWT_READ(name, id)                                                                                   \
    RWT_FN void *BMX_CDECL name(void *stream, int32_t length, void *object, int32_t offset, int32_t size)    \
    {                                                                                                        \
        rwtLog(id);                                                                                          \
        rwtLogPtr(stream);                                                                                   \
        rwtLog((uint32_t)length);                                                                            \
        rwtLogPtr(object);                                                                                   \
        rwtLog((uint32_t)offset);                                                                            \
        rwtLog((uint32_t)size);                                                                              \
        return (gRwtCallbackFails >> gRwtCallbackCalls++) & 1 ? NULL : stream;                               \
    }
#define RWT_WRITE(name, id)                                                                                  \
    RWT_FN void *BMX_CDECL name(void *stream, int32_t length, const void *object, int32_t offset,            \
                                int32_t size)                                                                \
    {                                                                                                        \
        rwtLog(id);                                                                                          \
        rwtLogPtr(stream);                                                                                   \
        rwtLog((uint32_t)length);                                                                            \
        rwtLogPtr(object);                                                                                   \
        rwtLog((uint32_t)offset);                                                                            \
        rwtLog((uint32_t)size);                                                                              \
        return (gRwtCallbackFails >> gRwtCallbackCalls++) & 1 ? NULL : stream;                               \
    }

/* The size callbacks return a function of the plugin's offset, so a run covers sizes <= 0 too. */
#define RWT_GETSIZE(name, id)                                                                                \
    RWT_FN int32_t BMX_CDECL name(const void *object, int32_t offset, int32_t size)                          \
    {                                                                                                        \
        rwtLog(id);                                                                                          \
        rwtLogPtr(object);                                                                                   \
        rwtLog((uint32_t)offset);                                                                            \
        rwtLog((uint32_t)size);                                                                              \
        return (offset % 5 == 0) ? 0 : (offset % 7 == 0 ? -3 : size + offset % 11);                          \
    }
// NOLINTEND(bugprone-macro-parentheses)

RWT_CTOR(rwtCtorA, 0xc0a)
RWT_CTOR(rwtCtorB, 0xc0b)
RWT_DTOR(rwtDtorA, 0xd0a)
RWT_DTOR(rwtDtorB, 0xd0b)
RWT_COPY(rwtCopyA, 0xe0a)
RWT_READ(rwtReadA, 0xf0a)
RWT_READ(rwtReadB, 0xf0b)
RWT_WRITE(rwtWriteA, 0xa0a)
RWT_WRITE(rwtWriteB, 0xa0b)
RWT_GETSIZE(rwtSizeA, 0xb0a)
RWT_GETSIZE(rwtSizeB, 0xb0b)

RWT_FN RwPluginObjectConstructor rwtPickCtor(void)
{
    switch (dt_u32() % 3) {
    case 0:
        return NULL;
    case 1:
        return rwtCtorA;
    default:
        return rwtCtorB;
    }
}

RWT_FN RwPluginObjectDestructor rwtPickDtor(void)
{
    switch (dt_u32() % 3) {
    case 0:
        return NULL;
    case 1:
        return rwtDtorA;
    default:
        return rwtDtorB;
    }
}

RWT_FN RwPluginObjectCopy rwtPickCopy(void)
{
    return dt_u32() % 2 ? rwtCopyA : NULL;
}

RWT_FN RwPluginDataChunkRead rwtPickRead(void)
{
    switch (dt_u32() % 3) {
    case 0:
        return NULL;
    case 1:
        return rwtReadA;
    default:
        return rwtReadB;
    }
}

RWT_FN RwPluginDataChunkWrite rwtPickWrite(void)
{
    switch (dt_u32() % 3) {
    case 0:
        return NULL;
    case 1:
        return rwtWriteA;
    default:
        return rwtWriteB;
    }
}

RWT_FN RwPluginDataChunkGetSize rwtPickSize(void)
{
    switch (dt_u32() % 3) {
    case 0:
        return NULL;
    case 1:
        return rwtSizeA;
    default:
        return rwtSizeB;
    }
}

/*
 * A plugin registry with n entries (ids base, base + 1, ...) with random callbacks, chained in both
 * directions, and the sizes consistent with the offsets. maxSizeOfStruct is 0 or near the current size.
 */
RWT_FN RwPluginRegistry *rwtRegistry(int n, int32_t baseId)
{
    RwPluginRegistry *r = dt_alloc(sizeof *r);
    RwPluginRegEntry *prev = NULL;
    int32_t offset = (int32_t)(dt_u32() % 16u) * 4;
    r->origSizeOfStruct = offset;
    for (int i = 0; i < n; i++) {
        RwPluginRegEntry *e = dt_alloc(sizeof *e);
        e->offset = offset;
        e->size = dt_range(0, 40);
        offset += (e->size + 3) & ~3;
        e->pluginID = baseId + i;
        e->readCB = rwtPickRead();
        e->writeCB = rwtPickWrite();
        e->getSizeCB = rwtPickSize();
        /* a registered entry always has these three (AddPlugin installs defaults) */
        e->constructCB = dt_u32() % 2 ? rwtCtorA : (dt_u32() % 2 ? rwtCtorB : rwPluginDefaultConstructor);
        e->destructCB = dt_u32() % 2 ? rwtDtorA : (dt_u32() % 2 ? rwtDtorB : rwPluginDefaultDestructor);
        e->copyCB = dt_u32() % 2 ? rwtCopyA : rwPluginDefaultCopy;
        e->parentRegistry = r;
        e->prevRegEntry = prev;
        if (prev != NULL) {
            prev->nextRegEntry = e;
        } else {
            r->firstRegEntry = e;
        }
        prev = e;
    }
    r->lastRegEntry = prev;
    r->sizeOfStruct = offset;
    r->maxSizeOfStruct = dt_u32() % 3 == 0 ? offset + dt_range(-8, 64) : 0;
    return r;
}

/* ---- scripted stream functions ---------------------------------------------------------------------- */

typedef struct RwtChunk {
    uint32_t type;
    uint32_t length;
} RwtChunk;

static uint32_t gRwtFindLength, gRwtFindVersion;
static int gRwtFindOk;
static RwtChunk gRwtChunks[8];
static int gRwtChunkN, gRwtChunkAt;
static int gRwtHeaderFailAt; /* the n-th (1-based) header read or write fails */
static int gRwtHeaderCalls;
static int gRwtSkipFailAt, gRwtSkipCalls;

RWT_FN int BMX_CDECL rwtStreamFindChunk(void *stream, uint32_t type, uint32_t *length, uint32_t *version)
{
    rwtLog(0x4e2ec0);
    rwtLogPtr(stream);
    rwtLog(type);
    rwtLog(length != NULL);
    rwtLog(version != NULL);
    if (length != NULL) {
        *length = gRwtFindLength;
    }
    if (version != NULL) {
        *version = gRwtFindVersion;
    }
    return gRwtFindOk;
}

RWT_FN int BMX_CDECL rwtStreamReadChunkHeader(void *stream, uint32_t *type, uint32_t *length,
                                              uint32_t *version)
{
    rwtLog(0x4e2e20);
    rwtLogPtr(stream);
    rwtLog(type != NULL);
    rwtLog(length != NULL);
    rwtLog(version != NULL);
    gRwtHeaderCalls++;
    if (gRwtHeaderFailAt != 0 && gRwtHeaderCalls == gRwtHeaderFailAt) {
        return 0;
    }
    if (gRwtChunkAt >= gRwtChunkN) {
        return 0; /* the script ran out: a stream that is shorter than its extension chunk claims */
    }
    const RwtChunk chunk = gRwtChunks[gRwtChunkAt++];
    if (type != NULL) {
        *type = chunk.type;
    }
    if (length != NULL) {
        *length = chunk.length;
    }
    if (version != NULL) {
        *version = 0x303;
    }
    return 1;
}

RWT_FN void *BMX_CDECL rwtStreamSkip(void *stream, uint32_t count)
{
    rwtLog(0x4e2c50);
    rwtLogPtr(stream);
    rwtLog(count);
    gRwtSkipCalls++;
    return gRwtSkipFailAt != 0 && gRwtSkipCalls == gRwtSkipFailAt ? NULL : stream;
}

RWT_FN int BMX_CDECL rwtStreamWriteHeader(void *stream, uint32_t type, int32_t size, uint32_t version)
{
    rwtLog(0x4e2fd0);
    rwtLogPtr(stream);
    rwtLog(type);
    rwtLog((uint32_t)size);
    rwtLog(version);
    gRwtHeaderCalls++;
    return !(gRwtHeaderFailAt != 0 && gRwtHeaderCalls == gRwtHeaderFailAt);
}

RWT_FN void rwtStreamStubs(void)
{
    dt_stub(0x004e2ec0, NULL, (void *)rwtStreamFindChunk);
    dt_stub(0x004e2e20, NULL, (void *)rwtStreamReadChunkHeader);
    dt_stub(0x004e2c50, NULL, (void *)rwtStreamSkip);
    dt_stub(0x004e2fd0, NULL, (void *)rwtStreamWriteHeader);
    gRwtCtorCalls = gRwtCtorFailAt = 0;
    gRwtCallbackFails = gRwtCallbackCalls = 0;
    gRwtFindLength = gRwtFindVersion = 0;
    gRwtFindOk = 1;
    gRwtChunkN = gRwtChunkAt = 0;
    gRwtHeaderFailAt = gRwtHeaderCalls = 0;
    gRwtSkipFailAt = gRwtSkipCalls = 0;
}

#endif
