/*
 * IFF chunk files (C:\NgEngine\Zge\ziff.c). A reader walks a file of nested chunks (an id, a size, the
 * data); `Ziff.frames` is the stack of the containers entered so far. Chunk ids are four character codes
 * read as little endian words ('FORM' = 0x4d524f46). Callbacks registered per chunk id handle a chunk when
 * ziffProcessChunks meets it.
 */
#ifndef ZGE_ZIFF_H
#define ZGE_ZIFF_H

#include <stdint.h>

#include "compat/orig.h"
#include "zge/zfile.h"
#include "zge/zmem.h"

/* One level of nesting: the container being read and the chunk the reader is positioned on in it. */
typedef struct ZiffFrame {
    uint32_t id;        /* 0x00 container id */
    uint32_t size;      /* 0x04 container data size */
    int32_t start;      /* 0x08 file offset of the container's header */
    int32_t end;        /* 0x0c file offset one past its data */
    int32_t pos;        /* 0x10 file position of the reader */
    uint32_t chunkId;   /* 0x14 id of the current chunk */
    uint32_t chunkSize; /* 0x18 its data size */
    int32_t chunkStart; /* 0x1c file offset of its header */
} ZiffFrame;

enum { kZiffFrames = 8 };

typedef struct Ziff {
    ZfileHandle *file;             /* 0x00 */
    ZiffFrame *frame;              /* 0x04 innermost container, NULL until the first frame is entered */
    ZiffFrame frames[kZiffFrames]; /* 0x08 */
} Ziff;

_Static_assert(sizeof(ZiffFrame) == 0x20, "ziff frame");
_Static_assert(sizeof(Ziff) == 0x108, "ziff reader");

/* Chunk handler: returns 0 to stop processing. The original passes a user argument as second parameter. */
typedef int(BMX_CDECL *ZiffChunkFn)(void *iff, void *arg);

typedef struct ZiffCallbackNode {
    uint32_t id;
    ZiffChunkFn fn;
    struct ZiffCallbackNode *next;
    uint32_t pad0c;
} ZiffCallbackNode;

typedef struct ZiffState {
    int initialized;                /* 0x6746d0 */
    ZMemFreeList *readers;          /* 0x6746d4 readers (Ziff) */
    ZMemFreeList *callbacks;        /* 0x6746d8 callback nodes */
    ZiffCallbackNode *callbackHead; /* 0x6746dc */
    uint32_t headId;                /* 0x6746e0 header of the container ziffOpen is about to enter */
    uint32_t headSize;              /* 0x6746e4 */
    int32_t headOffset;             /* 0x6746e8 */
} ZiffState;

_Static_assert(sizeof(ZiffState) == 0x1c, "ziff state");

#define gZiff ORIG_VAR(0x006746d0, ZiffState, gZiff)

/* Sets up the allocators; fatal when called twice. */
int ziffInitialize(void);
/*
 * Registers (or replaces) the handler for chunk `id`; returns the registration. The original takes only
 * the id and the handler (flags is never read); handlers get a second, user argument.
 */
ZiffCallbackNode *ziffSetupCallback(uint32_t id, int(BMX_CDECL *callback)(void *iff), int flags);
/* Opens an IFF reader on a file handle; formId -1 accepts any form. Returns NULL on failure. */
void *ziffOpen(void *file, uint32_t formId, uint32_t size);
void ziffClose(void *iff);
/* Positions on the first / next chunk with the given id (-1: any); returns nonzero when there is one. */
int ziffFindChunk(void *iff, uint32_t id);
int ziffNextChunk(void *iff, uint32_t id);
/* Reads the header of the next chunk (after the current one has been skipped or read). */
void ziffReadChunkHeader(void *iff);
/* Size of the current chunk, and a read of `size` bytes of it. */
uint32_t ziffChunkSize(void *iff);
int ziffRead(void *iff, void *dst, uint32_t size);
/* Reads the whole current chunk into dst (fatal unless positioned at its start). */
int ziffReadChunk(void *iff, void *dst);
/* Reads four bytes. */
void ziffReadWordA(void *iff, void *dst);
void ziffReadWordB(void *iff, void *dst);
/* Moves within the container: mode 0 from the start of its data, 1 from the reader; -1 at the end or on
 * error. */
int ziffSeek(void *iff, int mode, int offset);
/* Descends into the current chunk (a container) / leaves it again. */
int ziffBeginModel(void *iff);
int ziffEndModel(void *iff);
/* Runs the registered handlers over the chunks of the current container. */
int ziffProcessChunks(void *iff, void *arg);
/* Runs the handler of the current chunk only. */
int ziffDispatchChunk(void *iff, void *arg);

/* Reads the animation in the current chunk; its name hash; frees it. */
void *ziffReadAnim(void *iff);
uint32_t ziffAnimName(void *anim);
void ziffFreeAnim(void *anim);

/* TODO(file): the rest belong to the renderer's model / skeleton code (zrwmat.c). */
/* A pending item hanging off a skeleton node (its first 0x38 bytes are not used here). */
typedef struct ZSkeletonItem {
    uint8_t pad00[0x38];
    struct ZSkeletonItem *next;
} ZSkeletonItem;

/* Pose of one named part of a model; skeletons keep them sorted by name hash. */
typedef struct ZSkeletonNode {
    uint32_t flags; /* bit 0: rotation was set */
    uint32_t hash;
    uint8_t pad08[0x10];
    float quat[4];  /* 0x18 */
    float scale[3]; /* 0x28 */
    uint8_t pad34[8];
    ZSkeletonItem *items; /* 0x3c */
    uint8_t pad40[4];
} ZSkeletonNode;

typedef struct ZSkeleton {
    uint32_t count;
    uint32_t pad04;
    ZSkeletonNode *nodes; /* points at data */
    ZSkeletonNode data[];
} ZSkeleton;

/* The part of a renderer object the skeleton code touches. */
typedef struct ZSkeletonOwner {
    uint8_t pad00[0x30];
    ZSkeleton *skeleton;
} ZSkeletonOwner;

_Static_assert(sizeof(ZSkeletonNode) == 0x44, "skeleton node");

/* Recycled ZSkeletonItems, and the animation being read by ziffReadAnim (its handlers fill it). */
#define gZiffItemPool ORIG_VAR(0x006746c4, ZSkeletonItem *, gZiffItemPool)
#define gZiffAnim     ORIG_VAR(0x006746c8, void *, gZiffAnim)

/* Skeleton node whose name hash is `hash` (NULL if none); sets a node's rotation quaternion. */
void *zrwmatFindSkeletonNode(void *skeleton, uint32_t hash);
void zrwmatSetNodeRotation(void *node, const float *quat);
/* Builds the skeleton (nodes sorted by name hash) of a model object; frees it again. */
void *zrwmatBuildSkeleton(void *object);
void zrwmatFreeSkeleton(void *object);

#endif
