/*
 * RenderWare 3.1 streams (rw/stream.c): one reader/writer interface over a stdio file, a file name, a memory
 * block or user callbacks. Chunk headers are 12 bytes (type, length, version); the Int32/Real helpers
 * byte-swap on big-endian platforms, which on x86 leaves the data as it is.
 */
#ifndef RW_RWSTREAM_H
#define RW_RWSTREAM_H

#include "rw/rwcore.h"

enum {
    rwSTREAMFILE = 1,
    rwSTREAMFILENAME = 2,
    rwSTREAMMEMORY = 3,
    rwSTREAMCUSTOM = 4,
};

enum {
    rwSTREAMREAD = 1,
    rwSTREAMWRITE = 2,
    rwSTREAMAPPEND = 3,
};

/* The block a memory stream reads from or the result of writing to it (RwStreamClose hands it back). */
typedef struct RwMemory {
    uint8_t *start;
    uint32_t length;
} RwMemory;

typedef int(BMX_CDECL *RwStreamCustomClose)(void *data);
typedef uint32_t(BMX_CDECL *RwStreamCustomRead)(void *data, void *buffer, uint32_t length);
typedef int(BMX_CDECL *RwStreamCustomWrite)(void *data, const void *buffer, uint32_t length);
typedef int(BMX_CDECL *RwStreamCustomSkip)(void *data, uint32_t offset);

/* The callbacks of a custom stream, as RwStreamOpen takes them. */
typedef struct RwStreamCustom {
    RwStreamCustomClose close;
    RwStreamCustomRead read;
    RwStreamCustomWrite write;
    RwStreamCustomSkip skip;
    void *data;
} RwStreamCustom;

/* A stream object, 0x20 bytes from the stream plugin's free list. */
typedef struct RwStream {
    int32_t type;     /* 0x00 rwSTREAM* */
    int32_t access;   /* 0x04 */
    int32_t position; /* 0x08 unused */
    union {
        void *file; /* 0x0c the stdio handle (FILE and FILENAME streams) */
        struct {
            uint32_t position; /* 0x0c */
            uint32_t size;     /* 0x10 */
            uint8_t *block;    /* 0x14 */
        } memory;
        RwStreamCustom custom; /* 0x0c */
    } u;
} RwStream;

_Static_assert(sizeof(RwStream) == 0x20, "RwStream layout");
_Static_assert(offsetof(RwStream, u.memory.block) == 0x14, "RwStream layout");
_Static_assert(offsetof(RwStream, u.custom.data) == 0x1c, "RwStream layout");

RwStream *RwStreamOpen(int32_t type, int32_t access, const void *data);
int RwStreamClose(RwStream *stream, void *data);
uint32_t RwStreamRead(RwStream *stream, void *buffer, uint32_t length);
RwStream *RwStreamWrite(RwStream *stream, const void *buffer, uint32_t length);
RwStream *RwStreamSkip(RwStream *stream, uint32_t offset);

void *RwMemLittleEndian32(void *mem, uint32_t size);
void *RwMemNative32(void *mem, uint32_t size);
void *RwMemNative16(void *mem, uint32_t size);
void *RwMemFloat32ToReal(void *mem, uint32_t size);

int _rwStreamReadChunkHeader(RwStream *stream, uint32_t *type, uint32_t *length, uint32_t *version);
int RwStreamFindChunk(RwStream *stream, uint32_t type, uint32_t *length, uint32_t *version);
RwStream *_rwStreamWriteVersionedChunkHeader(RwStream *stream, uint32_t type, int32_t size, uint32_t version);
RwStream *RwStreamWriteReal(RwStream *stream, const void *reals, uint32_t size);
RwStream *RwStreamWriteInt32(RwStream *stream, const void *ints, uint32_t size);
RwStream *RwStreamReadReal(RwStream *stream, void *reals, uint32_t size);
RwStream *RwStreamReadInt32(RwStream *stream, void *ints, uint32_t size);

const RwFileFunctions *RwOSGetFileInterface(void);
int _rwFSOpen(void);

#endif
