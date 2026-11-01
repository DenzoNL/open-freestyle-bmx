/* RenderWare 3.1 streams: open/close/read/write/skip over files, memory and callbacks, chunk headers. */
#include "rw/rwstream.h"

#include "compat/crt.h"

/* Records an error of the stream layer in the error slot. */
static void rwStreamError(int32_t code)
{
    RwError error;
    error.pluginID = 1;
    error.errorCode = _rwerror(code);
    RwErrorSet(&error);
}

/* The free list the stream objects come from (plugin data of the engine instance). */
static RwFreeList *rwStreamFreeList(void)
{
    return *(RwFreeList **)((uint8_t *)RwEngineInstance + rwStreamOffset);
}

static void rwStreamFree(RwStream *stream)
{
    RwEngineInstance->freeListFree(rwStreamFreeList(), stream);
}

/* The engine's file function table. */
BMX_ADDR(0x004e8280)
const RwFileFunctions *RwOSGetFileInterface(void)
{
    return &RwEngineInstance->fileFuncs;
}

/* Installs the CRT's stdio as the engine's file system (the callbacks are still original code). */
BMX_ADDR(0x004e82e0)
int _rwFSOpen(void)
{
    RwFileFunctions *fs = &RwEngineInstance->fileFuncs;
    fs->rwfexist = RW_CB(int(BMX_CDECL *)(const char *), 0x004e8290);
    fs->rwfopen = RW_CB(void *(BMX_CDECL *)(const char *, const char *), 0x004a5c98);
    fs->rwfclose = RW_CB(int(BMX_CDECL *)(void *), 0x004a5cab);
    fs->rwfread = RW_CB(size_t(BMX_CDECL *)(void *, size_t, size_t, void *), 0x0051db3d);
    fs->rwfwrite = RW_CB(size_t(BMX_CDECL *)(const void *, size_t, size_t, void *), 0x0051dc25);
    fs->rwfgets = RW_CB(char *(BMX_CDECL *)(char *, int32_t, void *), 0x0051dd2f);
    fs->rwfputs = RW_CB(int(BMX_CDECL *)(const char *, void *), 0x0051d4d2);
    fs->rwfeof = RW_CB(int(BMX_CDECL *)(void *), 0x0051dd86);
    fs->rwfseek = RW_CB(int(BMX_CDECL *)(void *, int32_t, int32_t), 0x004a61ad);
    fs->rwfflush = RW_CB(int(BMX_CDECL *)(void *), 0x004a8370);
    return 1;
}

/*
 * Opens a stream. data is the file handle (FILE), the file name (FILENAME), an RwMemory (MEMORY: a block to
 * read, or ignored for an empty write; an append starts at the end of the block) or an RwStreamCustom
 * (CUSTOM). Returns NULL with an error set (null data, a file that does not open, an unknown type or access).
 * A FILENAME stream with null data fails without an error.
 */
BMX_ADDR(0x004e23e0)
RwStream *RwStreamOpen(int32_t type, int32_t access, const void *data)
{
    RwStream *stream = RwEngineInstance->freeListAlloc(rwStreamFreeList());
    if (stream == NULL) {
        return NULL;
    }
    stream->type = type;
    stream->access = access;
    if (type == rwSTREAMFILE) {
        if (data != NULL) {
            stream->u.file = (void *)data;
            return stream;
        }
        rwStreamError(rwERR_NULLP);
    } else if (type == rwSTREAMFILENAME) {
        if (data != NULL) {
            void *file = NULL;
            if (access == rwSTREAMREAD) {
                file = RwEngineInstance->fileFuncs.rwfopen(data, "rb");
            } else if (access == rwSTREAMWRITE) {
                file = RwEngineInstance->fileFuncs.rwfopen(data, "wb");
            } else if (access == rwSTREAMAPPEND) {
                file = RwEngineInstance->fileFuncs.rwfopen(data, "ab");
            } else {
                rwStreamError(0);
            }
            if (file != NULL) {
                stream->u.file = file;
                return stream;
            }
            RwError error;
            error.pluginID = 1;
            error.errorCode = _rwerror(rwERR_FILEOPEN, data);
            RwErrorSet(&error);
        }
    } else if (type == rwSTREAMMEMORY) {
        const RwMemory *memory = data;
        if (access == rwSTREAMREAD) {
            stream->u.memory.position = 0;
            stream->u.memory.size = memory->length;
            stream->u.memory.block = memory->start;
            return stream;
        }
        if (access == rwSTREAMWRITE) {
            stream->u.memory.block = NULL;
            stream->u.memory.size = 0;
            stream->u.memory.position = 0;
            return stream;
        }
        if (access == rwSTREAMAPPEND) {
            stream->u.memory.position = memory->length;
            stream->u.memory.size = memory->length;
            stream->u.memory.block = memory->start;
            return stream;
        }
        rwStreamError(0);
    } else if (type == rwSTREAMCUSTOM) {
        const RwStreamCustom *custom = data;
        if (custom != NULL) {
            stream->u.custom = *custom;
            return stream;
        }
        rwStreamError(rwERR_NULLP);
    } else {
        rwStreamError(1);
    }
    rwStreamFree(stream);
    return NULL;
}

/*
 * Closes a stream and frees it. A memory stream opened for writing hands its block and size to *data
 * (an RwMemory); without it that is an error. A custom stream without a close callback is not freed.
 * Returns 0 on failure.
 */
BMX_ADDR(0x004e2660)
int RwStreamClose(RwStream *stream, void *data)
{
    switch (stream->type) {
    case rwSTREAMFILE:
    case rwSTREAMFILENAME:
        if (RwEngineInstance->fileFuncs.rwfclose(stream->u.file) != 0) {
            rwStreamFree(stream);
            return 0;
        }
        rwStreamFree(stream);
        return 1;
    case rwSTREAMMEMORY:
        if (stream->access == rwSTREAMREAD) {
            rwStreamFree(stream);
        } else {
            RwMemory *memory = data;
            if (memory == NULL) {
                rwStreamFree(stream);
                rwStreamError(rwERR_NULLP);
                return 0;
            }
            memory->start = stream->u.memory.block;
            memory->length = stream->u.memory.position;
            rwStreamFree(stream);
        }
        return 1;
    case rwSTREAMCUSTOM:
        if (stream->u.custom.close != NULL) {
            stream->u.custom.close(stream->u.custom.data);
            rwStreamFree(stream);
        }
        return 1;
    default:
        rwStreamFree(stream);
        rwStreamError(1);
        return 0;
    }
}

/*
 * Reads length bytes; returns how many it got. A short file read sets "end of file" or a read error, a short
 * memory read sets "end of file" and returns what was left. Length 0 or a write-only stream is an error.
 */
BMX_ADDR(0x004e2800)
uint32_t RwStreamRead(RwStream *stream, void *buffer, uint32_t length)
{
    if (length == 0) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(rwERR_INVALIDSIZE, "Invalid length passed");
        RwErrorSet(&error);
        return 0;
    }
    if (stream->access != rwSTREAMREAD) {
        rwStreamError(0);
        return 0;
    }
    switch (stream->type) {
    case rwSTREAMFILE:
    case rwSTREAMFILENAME: {
        void *file = stream->u.file;
        const uint32_t got = (uint32_t)RwEngineInstance->fileFuncs.rwfread(buffer, 1, length, file);
        if (got != length) {
            if (RwEngineInstance->fileFuncs.rwfeof(file) == 0) {
                rwStreamError(rwERR_READ);
            } else {
                rwStreamError(2);
            }
        }
        return got;
    }
    case rwSTREAMMEMORY:
        if (stream->u.memory.size - stream->u.memory.position < length) {
            rwStreamError(2);
            length = stream->u.memory.size - stream->u.memory.position;
        }
        memcpy(buffer, stream->u.memory.block + stream->u.memory.position, length);
        stream->u.memory.position += length;
        return length;
    case rwSTREAMCUSTOM:
        return stream->u.custom.read(stream->u.custom.data, buffer, length);
    default:
        rwStreamError(1);
        return 0;
    }
}

/*
 * Writes length bytes; returns the stream, or NULL on failure. A memory stream starts with a 0x200 byte block
 * and grows by 0x200 bytes (or by the write, if that is larger) when it runs out.
 */
BMX_ADDR(0x004e29d0)
RwStream *RwStreamWrite(RwStream *stream, const void *buffer, uint32_t length)
{
    if (length == 0) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(rwERR_INVALIDSIZE, "Invalid length passed");
        RwErrorSet(&error);
        return NULL;
    }
    if (stream->access != rwSTREAMWRITE && stream->access != rwSTREAMAPPEND) {
        rwStreamError(0);
        return NULL;
    }
    switch (stream->type) {
    case rwSTREAMFILE:
    case rwSTREAMFILENAME:
        if ((uint32_t)RwEngineInstance->fileFuncs.rwfwrite(buffer, 1, length, stream->u.file) != length) {
            rwStreamError(rwERR_WRITE);
            return NULL;
        }
        return stream;
    case rwSTREAMMEMORY:
        if (stream->u.memory.block == NULL) {
            stream->u.memory.block = RwEngineInstance->memoryFuncs.rwmalloc(0x200);
            if (stream->u.memory.block == NULL) {
                RwError error;
                error.pluginID = 1;
                error.errorCode = _rwerror(rwERR_NOMEM, 0x200);
                RwErrorSet(&error);
                return NULL;
            }
            stream->u.memory.size = 0x200;
        }
        if (stream->u.memory.size - stream->u.memory.position < length) {
            uint32_t size = stream->u.memory.size;
            size += length < 0x200 ? 0x200 : length;
            uint8_t *block = RwEngineInstance->memoryFuncs.rwrealloc(stream->u.memory.block, size);
            if (block == NULL) {
                RwError error;
                error.pluginID = 1;
                error.errorCode = _rwerror(rwERR_NOMEM, size - stream->u.memory.size);
                RwErrorSet(&error);
                return NULL;
            }
            stream->u.memory.block = block;
            stream->u.memory.size = size;
        }
        memcpy(stream->u.memory.block + stream->u.memory.position, buffer, length);
        stream->u.memory.position += length;
        return stream;
    case rwSTREAMCUSTOM:
        if (stream->u.custom.write(stream->u.custom.data, buffer, length) == 0) {
            return NULL;
        }
        return stream;
    default:
        rwStreamError(1);
        return NULL;
    }
}

/* Skips offset bytes of a read stream; returns the stream, or NULL (a memory stream stops at its end). */
BMX_ADDR(0x004e2c50)
RwStream *RwStreamSkip(RwStream *stream, uint32_t offset)
{
    if (stream->access != rwSTREAMREAD) {
        rwStreamError(0);
        return NULL;
    }
    if (offset == 0) {
        return stream;
    }
    switch (stream->type) {
    case rwSTREAMFILE:
    case rwSTREAMFILENAME: {
        void *file = stream->u.file;
        if (RwEngineInstance->fileFuncs.rwfseek(file, (int32_t)offset, 1) == 0) {
            return stream;
        }
        if (RwEngineInstance->fileFuncs.rwfeof(file) != 0) {
            rwStreamError(2);
        }
        return NULL;
    }
    case rwSTREAMMEMORY:
        if (stream->u.memory.size < offset + stream->u.memory.position) {
            stream->u.memory.position = stream->u.memory.size;
            rwStreamError(2);
            return NULL;
        }
        stream->u.memory.position += offset;
        return stream;
    case rwSTREAMCUSTOM:
        if (stream->u.custom.skip(stream->u.custom.data, offset) == 0) {
            return NULL;
        }
        return stream;
    default:
        rwStreamError(1);
        return NULL;
    }
}

/* ---- endian conversion: on x86 these walk the words without changing them and return the block --------- */

BMX_ADDR(0x004e2da0)
void *RwMemLittleEndian32(void *mem, uint32_t size)
{
    (void)size;
    return mem;
}

BMX_ADDR(0x004e2dc0)
void *RwMemNative32(void *mem, uint32_t size)
{
    (void)size;
    return mem;
}

BMX_ADDR(0x004e2de0)
void *RwMemNative16(void *mem, uint32_t size)
{
    (void)size;
    return mem;
}

BMX_ADDR(0x004e2e00)
void *RwMemFloat32ToReal(void *mem, uint32_t size)
{
    (void)size;
    return mem;
}

/*
 * Reads a 12 byte chunk header; each output may be NULL. Returns 0 if the stream has less. The original's
 * code from 0x4e2e2c on (listed as a function of its own) is this body, entered by RwStreamFindChunk with
 * the arguments in registers.
 */
BMX_ADDR(0x004e2e20)
int _rwStreamReadChunkHeader(RwStream *stream, uint32_t *type, uint32_t *length, uint32_t *version)
{
    uint32_t header[3];
    if (RwStreamRead(stream, header, 12) != 12) {
        return 0;
    }
    if (type != NULL) {
        *type = header[0];
    }
    if (length != NULL) {
        *length = header[1];
    }
    if (version != NULL) {
        *version = header[2];
    }
    return 1;
}

/*
 * Skips chunks until one of the given type; returns 1 with its length and version (either may be NULL),
 * 0 if the stream ends first or a skip fails.
 */
BMX_ADDR(0x004e2ec0)
int RwStreamFindChunk(RwStream *stream, uint32_t type, uint32_t *length, uint32_t *version)
{
    uint32_t header[3];
    if (RwStreamRead(stream, header, 12) != 12) {
        return 0;
    }
    while (type != header[0]) {
        if (RwStreamSkip(stream, header[1]) == NULL) {
            return 0;
        }
        if (RwStreamRead(stream, header, 12) != 12) {
            return 0;
        }
    }
    if (length != NULL) {
        *length = header[1];
    }
    if (version != NULL) {
        *version = header[2];
    }
    return 1;
}

/* Writes a chunk header (type, size, version); returns the stream, or NULL if the write failed. */
BMX_ADDR(0x004e2fd0)
RwStream *_rwStreamWriteVersionedChunkHeader(RwStream *stream, uint32_t type, int32_t size, uint32_t version)
{
    uint32_t header[3];
    header[0] = type;
    header[1] = (uint32_t)size;
    header[2] = version;
    return RwStreamWrite(stream, header, 12);
}

/* Writes size bytes of 32-bit values in pieces of 0x100 bytes (signed compare: a size >= 2 GB is one piece).
 */
static RwStream *rwStreamWriteWords(RwStream *stream, const void *words, uint32_t size)
{
    while (size != 0) {
        uint32_t piece[64];
        const uint32_t n = (int32_t)size < 0x100 ? size : 0x100;
        memcpy(piece, words, n);
        if (RwStreamWrite(stream, piece, n) == NULL) {
            return NULL;
        }
        size -= n;
        words = (const uint8_t *)words + n;
    }
    return stream;
}

BMX_ADDR(0x004e3040)
RwStream *RwStreamWriteReal(RwStream *stream, const void *reals, uint32_t size)
{
    return rwStreamWriteWords(stream, reals, size);
}

BMX_ADDR(0x004e3130)
RwStream *RwStreamWriteInt32(RwStream *stream, const void *ints, uint32_t size)
{
    return rwStreamWriteWords(stream, ints, size);
}

/* Reads size bytes into the buffer; the stream, or NULL if fewer came. */
BMX_ADDR(0x004e3200)
RwStream *RwStreamReadReal(RwStream *stream, void *reals, uint32_t size)
{
    return RwStreamRead(stream, reals, size) == size ? stream : NULL;
}

BMX_ADDR(0x004e3270)
RwStream *RwStreamReadInt32(RwStream *stream, void *ints, uint32_t size)
{
    return RwStreamRead(stream, ints, size) == size ? stream : NULL;
}

#define rwStreamModuleCount ORIG_VAR(0x00676d54, int32_t, rwStreamModuleCount)

/* Stream module: the free list of stream objects. NULL when it cannot be created. */
BMX_ADDR(0x004e2350)
void *_rwStreamModuleOpen(void *engine, int32_t offset)
{
    rwStreamOffset = offset;
    *(RwFreeList **)((uint8_t *)RwEngineInstance + offset) = RwFreeListCreate(sizeof(RwStream), 10, 0);
    if (rwStreamFreeList() == NULL) {
        return NULL;
    }
    rwStreamModuleCount++;
    return engine;
}

/* Destroys the stream free list (if it was made) and counts the module down. */
BMX_ADDR(0x004e23b0)
void *_rwStreamModuleClose(void *engine)
{
    if (rwStreamFreeList() != NULL) {
        RwFreeListDestroy(rwStreamFreeList());
    }
    rwStreamModuleCount--;
    return engine;
}

/*
 * The register entry of the chunk header reader: EAX = stream, EDX = type, ECX = length, the version pointer
 * is the fourth stack word. Only RwStreamFindChunk's original code called it, and ours replaces that.
 */
BMX_ADDR(0x004e2e2c)
__attribute__((regparm(3))) int rwStreamReadChunkHeaderReg(RwStream *stream, uint32_t *type, uint32_t *length,
                                                           int32_t unused0, int32_t unused1, int32_t unused2,
                                                           uint32_t *version)
{
    (void)unused0;
    (void)unused1;
    (void)unused2;
    return _rwStreamReadChunkHeader(stream, type, length, version);
}

/*
 * The constant results of a table of device callbacks (a null implementation set); none reads its arguments.
 * Three of them are 3 or 4 bytes long, so their hooks spill into the next stub (patched later, the hook table
 * is in address order) and into the padding behind the last.
 */
BMX_ADDR(0x004e32f2)
int rwStubReturn0_4e32f2(void)
{
    return 0;
}

BMX_ADDR(0x004e32f9)
int rwStubReturn0_4e32f9(void)
{
    return 0;
}

BMX_ADDR(0x004e3300)
int rwStubReturn1_4e3300(void)
{
    return 1;
}

BMX_ADDR(0x004e330a)
int rwStubReturn1_4e330a(void)
{
    return 1;
}

BMX_ADDR(0x004e3311)
int rwStubReturn1_4e3311(void)
{
    return 1;
}

BMX_ADDR(0x004e3318)
int rwStubReturn1_4e3318(void)
{
    return 1;
}

BMX_ADDR(0x004e331f)
int rwStubReturn1_4e331f(void)
{
    return 1;
}

BMX_ADDR(0x004e3326)
int rwStubReturn1_4e3326(void)
{
    return 1;
}

BMX_ADDR(0x004e332d)
int rwStubReturn1_4e332d(void)
{
    return 1;
}

/* 0x4e3334: 4 bytes, directly followed by the next stub, too short for a hooking jmp (gen_hooks.py checks
 * this); the original stays in place. */
int rwStubReturn0_4e3334(void)
{
    return 0;
}

BMX_ADDR(0x004e3338)
int rwStubReturn1_4e3338(void)
{
    return 1;
}

BMX_ADDR(0x004e333f)
int rwStubReturn1_4e333f(void)
{
    return 1;
}

BMX_ADDR(0x004e3346)
int rwStubReturn1_4e3346(void)
{
    return 1;
}

BMX_ADDR(0x004e334d)
int rwStubReturn1_4e334d(void)
{
    return 1;
}

/* 0x4e3354: 4 bytes, directly followed by the next stub, too short for a hooking jmp (gen_hooks.py checks
 * this); the original stays in place. */
int rwStubReturn0_4e3354(void)
{
    return 0;
}

BMX_ADDR(0x004e3358)
int rwStubReturn1_4e3358(void)
{
    return 1;
}

BMX_ADDR(0x004e335f)
int rwStubReturn0_4e335f(void)
{
    return 0;
}
