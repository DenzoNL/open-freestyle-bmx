/*
 * Tests for rw/stream.c: the stream layer over a stdio file (the engine's file function table is a logging
 * script), memory blocks and custom callbacks, the chunk header helpers and the endian helpers.
 *
 * The engine instance is the fixture from dt_rw.h plus a stream free list slot, a realloc and a file table.
 */
#include "dt.h"
#include "dt_rw.h"
#include "rw/rwstream.h"

/* ---- the fixture ------------------------------------------------------------------------------------ */

static void *gFile;               /* the handle the file stubs know */
static int gFopenOk;              /* fopen succeeds */
static uint32_t gAvail;           /* bytes fread delivers at most */
static int gFcloseRet, gFseekRet; /* results of fclose and fseek */
static int gFeofRet;
static uint32_t gFwriteMax; /* bytes fwrite accepts */
static uint32_t gOldSize;   /* size of the block realloc is asked to grow */
static int gReallocFail;
static uint32_t gCustRead; /* what a custom read returns: bit 0 = full, else length / 2 */
static int gCustWriteOk, gCustSkipOk;
static int gCustWriteCalls, gCustWriteFailAt, gCustReadCalls;

static void *BMX_CDECL stRealloc(void *block, size_t size)
{
    rwtLog(0x130);
    rwtLogPtr(block);
    rwtLog((uint32_t)size);
    if (gReallocFail) {
        return NULL;
    }
    uint8_t *fresh = dt_alloc((uint32_t)size);
    __builtin_memcpy(fresh, block, size < gOldSize ? size : gOldSize);
    return fresh;
}

static void *BMX_CDECL stFopen(const char *name, const char *mode)
{
    rwtLog(0xc4);
    rwtLogPtr(name);
    for (int i = 0; i < 3; i++) {
        rwtLog((uint8_t)mode[i]);
    }
    return gFopenOk ? gFile : NULL;
}

static int BMX_CDECL stFclose(void *file)
{
    rwtLog(0xc8);
    rwtLogPtr(file);
    return gFcloseRet;
}

static size_t BMX_CDECL stFread(void *buf, size_t size, size_t count, void *file)
{
    rwtLog(0xcc);
    rwtLog((uint32_t)size);
    rwtLog((uint32_t)count);
    rwtLogPtr(file);
    const size_t n = count < gAvail ? count : gAvail;
    for (size_t i = 0; i < n; i++) {
        ((uint8_t *)buf)[i] = (uint8_t)(i * 7 + 1);
    }
    return n;
}

static size_t BMX_CDECL stFwrite(const void *buf, size_t size, size_t count, void *file)
{
    rwtLog(0xd0);
    rwtLog((uint32_t)size);
    rwtLog((uint32_t)count);
    rwtLogPtr(file);
    for (size_t i = 0; i < count && i < 24; i++) {
        rwtLog(((const uint8_t *)buf)[i]);
    }
    return count < gFwriteMax ? count : gFwriteMax;
}

static int BMX_CDECL stFeof(void *file)
{
    rwtLog(0xdc);
    rwtLogPtr(file);
    return gFeofRet;
}

static int BMX_CDECL stFseek(void *file, int32_t offset, int32_t origin)
{
    rwtLog(0xe0);
    rwtLogPtr(file);
    rwtLog((uint32_t)offset);
    rwtLog((uint32_t)origin);
    return gFseekRet;
}

static int BMX_CDECL stCustClose(void *data)
{
    rwtLog(0xc1);
    rwtLogPtr(data);
    return 1;
}

static uint32_t BMX_CDECL stCustRead(void *data, void *buffer, uint32_t length)
{
    rwtLog(0xc2);
    rwtLogPtr(data);
    rwtLog(length);
    for (uint32_t i = 0; i < length; i++) {
        ((uint8_t *)buffer)[i] = (uint8_t)(i * 5 + 3);
    }
    if (length == 12) { /* a chunk header: types count up, so a search ends */
        ((uint32_t *)buffer)[0] = 0x10u + (uint32_t)gCustReadCalls;
        ((uint32_t *)buffer)[1] = (uint32_t)gCustReadCalls * 3u;
    }
    gCustReadCalls++;
    if (gCustReadCalls > 6) {
        return 0;
    }
    return (gCustRead & 1) ? length : length / 2;
}

static int BMX_CDECL stCustWrite(void *data, const void *buffer, uint32_t length)
{
    rwtLog(0xc3);
    rwtLogPtr(data);
    rwtLog(length);
    for (uint32_t i = 0; i < length && i < 16; i++) {
        rwtLog(((const uint8_t *)buffer)[i]);
    }
    gCustWriteCalls++;
    if (gCustWriteFailAt != 0 && gCustWriteCalls == gCustWriteFailAt) {
        return 0;
    }
    return gCustWriteOk;
}

static int BMX_CDECL stCustSkip(void *data, uint32_t offset)
{
    rwtLog(0xc5);
    rwtLogPtr(data);
    rwtLog(offset);
    return gCustSkipOk;
}

/* An engine instance with the stream free list slot, a realloc and the file table. */
static RwGlobals *stEnv(DtCtx *c)
{
    dt_seed(c);
    RwGlobals *g = rwtEnv();
    g->memoryFuncs.rwrealloc = stRealloc;
    g->fileFuncs.rwfopen = stFopen;
    g->fileFuncs.rwfclose = stFclose;
    g->fileFuncs.rwfread = stFread;
    g->fileFuncs.rwfwrite = stFwrite;
    g->fileFuncs.rwfeof = stFeof;
    g->fileFuncs.rwfseek = stFseek;
    void **slot = (void **)dt_alloc(8);
    *slot = dt_alloc(8);
    rwStreamOffset = (int32_t)((uintptr_t)slot - (uintptr_t)g);
    gFile = dt_alloc(8);
    gFopenOk = dt_u32() % 5 != 0;
    gAvail = (uint32_t)dt_range(0, 40);
    gFcloseRet = dt_u32() % 4 == 0;
    gFseekRet = dt_u32() % 4 == 0;
    gFeofRet = (int)(dt_u32() % 2);
    gFwriteMax = dt_u32() % 4 == 0 ? (uint32_t)dt_range(0, 20) : 0xffffffffu;
    gOldSize = 0;
    gReallocFail = 0;
    gCustRead = dt_u32();
    gCustWriteOk = dt_u32() % 5 != 0;
    gCustSkipOk = dt_u32() % 5 != 0;
    gCustWriteCalls = 0;
    gCustReadCalls = 0;
    gCustWriteFailAt = dt_u32() % 3 == 0 ? dt_range(1, 4) : 0;
    return g;
}

/* A stream with random fields for its type: a file, a block of memory (size, position) or callbacks. */
static RwStream *makeStream(int32_t type, int32_t access)
{
    RwStream *s = dt_alloc(sizeof *s);
    s->type = type;
    s->access = access;
    switch (type) {
    case rwSTREAMFILE:
    case rwSTREAMFILENAME:
        s->u.file = gFile;
        break;
    case rwSTREAMMEMORY: {
        const uint32_t size = (uint32_t)dt_range(0, 64);
        s->u.memory.block = dt_alloc(size + 8);
        dt_fill(s->u.memory.block, size);
        s->u.memory.size = size;
        s->u.memory.position = (uint32_t)dt_range(0, (int32_t)size);
        break;
    }
    case rwSTREAMCUSTOM:
        s->u.custom.close = dt_u32() % 5 == 0 ? NULL : stCustClose;
        s->u.custom.read = stCustRead;
        s->u.custom.write = stCustWrite;
        s->u.custom.skip = stCustSkip;
        s->u.custom.data = dt_alloc(8);
        break;
    default:
        break;
    }
    return s;
}

static int32_t pickType(void)
{
    return dt_u32() % 12 == 0 ? dt_range(-1, 6) : dt_range(1, 4);
}

static int32_t pickAccess(void)
{
    return dt_u32() % 12 == 0 ? dt_range(0, 4) : dt_range(1, 3);
}

/* ---- RwStreamOpen / RwStreamClose ------------------------------------------------------------------- */

static void setup_open(DtCtx *c)
{
    stEnv(c);
    gRwtEntryFailAt = dt_u32() % 8 == 0 ? 1 : 0;
    /* one block that reads as a name, a memory descriptor (start, length) and a custom table */
    uint32_t *data = dt_alloc(32);
    data[0] = (uint32_t)(uintptr_t)dt_alloc(16);
    data[1] = (uint32_t)dt_range(0, 1000);
    data[2] = (uint32_t)(uintptr_t)stCustRead;
    data[3] = dt_u32() % 2 ? (uint32_t)(uintptr_t)stCustWrite : 0;
    data[4] = (uint32_t)(uintptr_t)stCustSkip;
    data[5] = (uint32_t)(uintptr_t)dt_alloc(4);
    ((char *)data)[0] = 'a';
    DT_ARG_INT(c, 0, pickType());
    DT_ARG_INT(c, 1, pickAccess());
    /* the memory descriptor is dereferenced for any valid access, so it is never missing there */
    const int needsData = c->args[0] == rwSTREAMMEMORY && c->args[1] >= 1 && c->args[1] <= 3;
    DT_ARG_PTR(c, 2, dt_u32() % 6 == 0 && !needsData ? NULL : data);
    rwtEnd();
}

static void setup_close(DtCtx *c)
{
    stEnv(c);
    const int32_t type = pickType();
    RwStream *s = makeStream(type, pickAccess());
    RwMemory *out = dt_alloc(sizeof *out);
    out->start = (uint8_t *)(uintptr_t)dt_u32();
    out->length = dt_u32();
    DT_ARG_PTR(c, 0, s);
    DT_ARG_PTR(c, 1, dt_u32() % 4 == 0 ? NULL : out);
    rwtEnd();
}

/* ---- Read / Write / Skip ---------------------------------------------------------------------------- */

static void setup_read(DtCtx *c)
{
    stEnv(c);
    RwStream *s = makeStream(pickType(), dt_u32() % 10 == 0 ? pickAccess() : rwSTREAMREAD);
    DT_ARG_PTR(c, 0, s);
    DT_ARG_PTR(c, 1, dt_alloc(64));
    DT_ARG_INT(c, 2, dt_u32() % 10 == 0 ? 0 : dt_range(1, 48));
    rwtEnd();
}

static void setup_write(DtCtx *c)
{
    stEnv(c);
    const int32_t type = pickType();
    RwStream *s = makeStream(type, dt_u32() % 10 == 0 ? pickAccess() : dt_range(2, 3));
    if (type == rwSTREAMMEMORY) {
        if (dt_u32() % 3 == 0) {
            s->u.memory.block = NULL; /* a fresh write stream: allocates its first 0x200 bytes */
            s->u.memory.size = 0;
            s->u.memory.position = 0;
        } else {
            /* near the end of the block so that writes overflow it and grow it */
            s->u.memory.position = s->u.memory.size - (uint32_t)dt_range(0, (int32_t)s->u.memory.size);
            gOldSize = s->u.memory.size;
        }
    }
    gRwtMallocFailAt = dt_u32() % 8 == 0 ? 1 : 0;
    gReallocFail = dt_u32() % 8 == 0;
    uint8_t *src = dt_alloc(1100);
    dt_fill(src, 1100);
    DT_ARG_PTR(c, 0, s);
    DT_ARG_PTR(c, 1, src);
    DT_ARG_INT(c, 2, dt_u32() % 10 == 0 ? 0 : (dt_u32() % 4 == 0 ? dt_range(0x1f0, 0x420) : dt_range(1, 48)));
    rwtEnd();
}

static void setup_skip(DtCtx *c)
{
    stEnv(c);
    RwStream *s = makeStream(pickType(), dt_u32() % 10 == 0 ? pickAccess() : rwSTREAMREAD);
    DT_ARG_PTR(c, 0, s);
    DT_ARG_INT(c, 1, dt_u32() % 8 == 0 ? 0 : (dt_u32() % 5 == 0 ? dt_range(-8, 100000) : dt_range(1, 70)));
    rwtEnd();
}

/* ---- endian helpers --------------------------------------------------------------------------------- */

static void setup_mem(DtCtx *c)
{
    dt_seed(c);
    uint8_t *mem = dt_alloc(64);
    dt_fill(mem, 64);
    DT_ARG_PTR(c, 0, mem);
    DT_ARG_INT(c, 1, dt_range(0, 64));
}

/* ---- chunk headers ---------------------------------------------------------------------------------- */

/* A memory read stream over `chunks` chunks (type 0x10 + i, random lengths with payload), at the start. */
static RwStream *makeChunkStream(int chunks, uint32_t *types)
{
    uint32_t cap = 16u + (uint32_t)chunks * 80u;
    uint32_t *block = dt_alloc(cap);
    uint32_t pos = 0;
    for (int i = 0; i < chunks; i++) {
        const uint32_t len = (uint32_t)dt_range(0, 40);
        block[pos / 4] = 0x10u + (uint32_t)i;
        block[pos / 4 + 1] = len;
        block[pos / 4 + 2] = 0x1c020037u + (uint32_t)i;
        dt_fill((uint8_t *)block + pos + 12, len);
        pos += 12 + ((len + 3) & ~3u);
        if (types != NULL) {
            types[i] = 0x10u + (uint32_t)i;
        }
    }
    RwStream *s = dt_alloc(sizeof *s);
    s->type = rwSTREAMMEMORY;
    s->access = rwSTREAMREAD;
    s->u.memory.block = (uint8_t *)block;
    s->u.memory.size = pos - (dt_u32() % 6 == 0 && pos >= 11 ? (uint32_t)dt_range(1, 11) : 0);
    s->u.memory.position = 0;
    return s;
}

static void setup_read_header(DtCtx *c)
{
    stEnv(c);
    RwStream *s;
    if (dt_u32() % 5 == 0) {
        s = makeStream(rwSTREAMCUSTOM, rwSTREAMREAD);
    } else {
        s = makeChunkStream(dt_range(0, 2), NULL);
    }
    DT_ARG_PTR(c, 0, s);
    DT_ARG_PTR(c, 1, dt_u32() % 4 == 0 ? NULL : dt_alloc(4));
    DT_ARG_PTR(c, 2, dt_u32() % 4 == 0 ? NULL : dt_alloc(4));
    DT_ARG_PTR(c, 3, dt_u32() % 4 == 0 ? NULL : dt_alloc(4));
    rwtEnd();
}

/* The register entry 0x4e2e2c: EAX = stream, EDX = type, ECX = length, the version is the fourth stack word.
 */
static int header_reg_orig(RwStream *stream, uint32_t *type, uint32_t *length, uint32_t *version)
{
    return ((int(__attribute__((regparm(3))) *)(RwStream *, uint32_t *, uint32_t *, int32_t, int32_t, int32_t,
                                                uint32_t *))(uintptr_t)0x004e2e2c)(stream, type, length, 0, 0,
                                                                                   0, version);
}

static int header_reg_impl(RwStream *stream, uint32_t *type, uint32_t *length, uint32_t *version)
{
    return rwStreamReadChunkHeaderReg(stream, type, length, 0, 0, 0, version);
}

static void setup_find_chunk(DtCtx *c)
{
    stEnv(c);
    const int chunks = dt_range(0, 6);
    RwStream *s;
    if (dt_u32() % 6 == 0) {
        s = makeStream(rwSTREAMCUSTOM, rwSTREAMREAD);
    } else {
        s = makeChunkStream(chunks, NULL);
    }
    DT_ARG_PTR(c, 0, s);
    DT_ARG_INT(c, 1, 0x10u + (uint32_t)dt_range(0, 7));
    DT_ARG_PTR(c, 2, dt_u32() % 4 == 0 ? NULL : dt_alloc(4));
    DT_ARG_PTR(c, 3, dt_u32() % 4 == 0 ? NULL : dt_alloc(4));
    rwtEnd();
}

static void setup_write_header(DtCtx *c)
{
    stEnv(c);
    const int type = dt_u32() % 3 == 0 ? rwSTREAMCUSTOM : rwSTREAMMEMORY;
    RwStream *s = makeStream(type, rwSTREAMWRITE);
    if (type == rwSTREAMMEMORY) {
        s->u.memory.block = NULL;
        s->u.memory.size = 0;
        s->u.memory.position = 0;
    }
    gRwtMallocFailAt = dt_u32() % 8 == 0 ? 1 : 0;
    DT_ARG_PTR(c, 0, s);
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_INT(c, 2, dt_u32());
    DT_ARG_INT(c, 3, dt_u32());
    rwtEnd();
}

/* Real/Int32 helpers: a memory or custom stream and a block of 0..700 bytes. */
static void setup_write_words(DtCtx *c)
{
    stEnv(c);
    const int type = dt_u32() % 3 == 0 ? rwSTREAMCUSTOM : rwSTREAMMEMORY;
    RwStream *s = makeStream(type, rwSTREAMWRITE);
    if (type == rwSTREAMMEMORY) {
        s->u.memory.block = NULL;
        s->u.memory.size = 0;
        s->u.memory.position = 0;
    }
    gRwtMallocFailAt = dt_u32() % 10 == 0 ? 1 : 0;
    uint8_t *src = dt_alloc(720);
    dt_fill(src, 720);
    DT_ARG_PTR(c, 0, s);
    DT_ARG_PTR(c, 1, src);
    DT_ARG_INT(c, 2, dt_u32() % 8 == 0 ? 0 : (uint32_t)dt_range(0, 175) * 4u);
    rwtEnd();
}

static void setup_read_words(DtCtx *c)
{
    stEnv(c);
    const int type = dt_u32() % 3 == 0 ? rwSTREAMCUSTOM : rwSTREAMMEMORY;
    RwStream *s = makeStream(type, rwSTREAMREAD);
    DT_ARG_PTR(c, 0, s);
    DT_ARG_PTR(c, 1, dt_alloc(128));
    DT_ARG_INT(c, 2, (uint32_t)dt_range(1, 20) * 4u);
    rwtEnd();
}

/* ---- file table ------------------------------------------------------------------------------------- */

static void setup_file_interface(DtCtx *c)
{
    stEnv(c);
    rwtEnd();
}

static void setup_fs_open(DtCtx *c)
{
    dt_seed(c);
    RwGlobals *g = rwtEnv();
    dt_fill(&g->fileFuncs, sizeof g->fileFuncs);
    rwtEnd();
}

DT_CASES(rwstream_cases) = {
    {"RwStreamOpen", 0x004e23e0, RwStreamOpen, 3, DT_RET_INT, setup_open, 600, 0},
    {"RwStreamClose", 0x004e2660, RwStreamClose, 2, DT_RET_INT, setup_close, 400, 0},
    {"RwStreamRead", 0x004e2800, RwStreamRead, 3, DT_RET_INT, setup_read, 800, 0},
    {"RwStreamWrite", 0x004e29d0, RwStreamWrite, 3, DT_RET_INT, setup_write, 1000, 0},
    {"RwStreamSkip", 0x004e2c50, RwStreamSkip, 2, DT_RET_INT, setup_skip, 600, 0},
    {"RwMemLittleEndian32", 0x004e2da0, RwMemLittleEndian32, 2, DT_RET_INT, setup_mem, 20, 0},
    {"RwMemNative32", 0x004e2dc0, RwMemNative32, 2, DT_RET_INT, setup_mem, 20, 0},
    {"RwMemNative16", 0x004e2de0, RwMemNative16, 2, DT_RET_INT, setup_mem, 20, 0},
    {"RwMemFloat32ToReal", 0x004e2e00, RwMemFloat32ToReal, 2, DT_RET_INT, setup_mem, 20, 0},
    {"_rwStreamReadChunkHeader", 0x004e2e20, _rwStreamReadChunkHeader, 4, DT_RET_INT, setup_read_header, 300,
     0},
    {"rwStreamReadChunkHeaderReg", (uint32_t)(uintptr_t)header_reg_orig, header_reg_impl, 4, DT_RET_INT,
     setup_read_header, 300, 0},
    {"RwStreamFindChunk", 0x004e2ec0, RwStreamFindChunk, 4, DT_RET_INT, setup_find_chunk, 600, 0},
    {"_rwStreamWriteVersionedChunkHeader", 0x004e2fd0, _rwStreamWriteVersionedChunkHeader, 4, DT_RET_INT,
     setup_write_header, 200, 0},
    {"RwStreamWriteReal", 0x004e3040, RwStreamWriteReal, 3, DT_RET_INT, setup_write_words, 500, 0},
    {"RwStreamWriteInt32", 0x004e3130, RwStreamWriteInt32, 3, DT_RET_INT, setup_write_words, 500, 0},
    {"RwStreamReadReal", 0x004e3200, RwStreamReadReal, 3, DT_RET_INT, setup_read_words, 300, 0},
    {"RwStreamReadInt32", 0x004e3270, RwStreamReadInt32, 3, DT_RET_INT, setup_read_words, 300, 0},
    {"RwOSGetFileInterface", 0x004e8280, RwOSGetFileInterface, 0, DT_RET_INT, setup_file_interface, 5, 0},
    {"_rwFSOpen", 0x004e82e0, _rwFSOpen, 0, DT_RET_INT, setup_fs_open, 5, 0},
};
