/* IFF chunk reader (C:\NgEngine\Zge\ziff.c). */
#include "zge/ziff.h"

#include "compat/crt.h"
#include "zge/com/zglrw/zrwmat.h"
#include "zge/zdebug.h"
#include "zge/zfile.h"
#include "zge/zmem.h"
#include "zge/zstring.h"

static const char kZiffFile[] = "C:\\NgEngine\\Zge\\ziff.c";

/* The four characters of an id, low byte first. */
#define ID_CHARS(id)                                                                                         \
    (int)((id) & 0xff), (int)(((id) >> 8) & 0xff), (int)(((id) >> 16) & 0xff), (int)((id) >> 24)

static void failIdMessage(const char *fmt, uint32_t id, int line)
{
    zdebugFail(zstrFormatTemp(fmt, ID_CHARS(id)), kZiffFile, line, 0);
}

BMX_ADDR(0x0049a6d0)
void ziffInitialize(void)
{
    if (gZiff.initialized != 0) {
        zdebugFail("Already initialized", kZiffFile, 0x11a, 0);
    }
    zmemFill(&gZiff, 0, sizeof gZiff);
    gZiff.callbacks = zmemFreeListCreate(0x10, 0x40, NULL);
    gZiff.readers = zmemFreeListCreate(0x108, 0x10, NULL);
    gZiff.initialized = 1;
}

BMX_ADDR(0x0049aad0)
ZiffCallbackNode *ziffSetupCallback(uint32_t id, int(BMX_CDECL *callback)(void *iff), int flags)
{
    ZiffChunkFn fn = (ZiffChunkFn)(uintptr_t)callback;
    ZiffCallbackNode *node;

    if (gZiff.initialized == 0) {
        zdebugFail("Not initialized", kZiffFile, 0x1e0, 0);
    }
    for (node = gZiff.callbackHead; node != NULL; node = node->next) {
        if (node->id == id) {
            if (node->fn != fn) {
                node->fn = fn;
                gZdebugPrint("ZIFF: Changing function for '%c%c%c%c'\n", ID_CHARS(id));
            } else {
                gZdebugPrint("ZIFF: Not re-registering '%c%c%c%c'\n", ID_CHARS(id));
            }
            return node;
        }
    }
    node = zmemFreeListAlloc(gZiff.callbacks);
    node->id = id;
    node->fn = fn;
    node->pad0c = 0;
    node->next = gZiff.callbackHead;
    gZiff.callbackHead = node;
    return node;
}

BMX_ADDR(0x0049a790)
void *ziffOpen(void *file, uint32_t formId, uint32_t size)
{
    Ziff *iff = zmemFreeListAlloc(gZiff.readers);

    if (formId == 0xffffffffu) {
        int fileSize = (int)size;
        if (size == 0) {
            zfileGetSize(file, &fileSize, 0);
        }
        gZiff.headId = 0xffffffffu;
        gZiff.headSize = (uint32_t)fileSize;
        gZiff.headOffset = 0;
    } else {
        zfileRead(file, &gZiff.headId, 8, 0);
        gZiff.headOffset = 8;
        if (gZiff.headId != formId) {
            zmemFreeListFree(gZiff.readers, iff);
            gZdebugPrint("ZIFF: Non-matching chunkID('%c%c%c%c != %c%c%c%c')\n", ID_CHARS(formId),
                         ID_CHARS(gZiff.headId));
            zdebugFail("ZIFF: Bad IFF file", kZiffFile, 0x171, 0);
            return NULL;
        }
    }
    iff->file = file;
    iff->frame = NULL;
    ziffBeginModel(iff);
    return iff;
}

BMX_ADDR(0x0049a8a0)
void ziffClose(void *iffp)
{
    Ziff *iff = iffp;

    if (iff->frame != &iff->frames[0]) {
        zdebugFail("ZIFF: Chunks still open", kZiffFile, 0x18e, 0);
    }
    zmemFreeListFree(gZiff.readers, iff);
}

/* Moves the reader to chunk `chunkId`'s header, reading nothing (shared tail of find / next). */
static void readHeader(Ziff *iff)
{
    iff->frame->chunkStart = iff->frame->pos;
    iff->frame->pos += zfileRead(iff->file, &iff->frame->chunkId, 8, 0);
}

BMX_ADDR(0x0049b020)
void ziffReadChunkHeader(void *iffp)
{
    readHeader(iffp);
}

BMX_ADDR(0x0049b060)
int ziffSeek(void *iffp, int mode, int offset)
{
    Ziff *iff = iffp;
    int32_t pos;

    if (mode == 0) {
        pos = iff->frame->start + 8 + offset;
    } else if (mode == 1) {
        pos = iff->frame->pos + offset;
    } else {
        zdebugFail("Invalid seek mode", kZiffFile, 0x2ea, 0);
        return -1;
    }
    if (iff->frame->end < pos) {
        zdebugFail("Seeked passed the end", kZiffFile, 0x2ed, 0);
        return -1;
    }
    if (pos < iff->frame->start) {
        zdebugFail("Seeked passed the start", kZiffFile, 0x2ee, 0);
        return -1;
    }
    zfileOp30(iff->file, (uint32_t)pos, 0, 0);
    iff->frame->pos = pos;
    if (pos == iff->frame->end) {
        return -1;
    }
    return pos;
}

BMX_ADDR(0x0049b130)
int ziffBeginModel(void *iffp)
{
    Ziff *iff = iffp;
    ZiffFrame *f = iff->frame;

    if (f == NULL) {
        iff->frame = &iff->frames[0];
        iff->frame->id = gZiff.headId;
        iff->frame->size = gZiff.headSize;
        iff->frame->start = gZiff.headOffset - 8;
    } else {
        if ((uint8_t *)f >= (uint8_t *)iff + 0xe8) {
            zdebugFail("Nested too deep", kZiffFile, 0x31a, 0);
            return 0;
        }
        if (f->chunkStart + 8 != f->pos) {
            zdebugFail("Not at the start of the chunk", kZiffFile, 0x31e, 0);
            return 0;
        }
        iff->frame = f + 1;
        iff->frame->id = f->chunkId;
        iff->frame->size = f->chunkSize;
        iff->frame->start = f->pos - 8;
    }
    f = iff->frame;
    f->end = f->start + 8 + (int32_t)f->size;
    f->pos = f->start;
    f->chunkStart = f->pos;
    f = iff->frame;
    if (f->chunkStart < 0) {
        f->chunkId = f->id;
        f->chunkSize = f->size;
        iff->frame->pos += 8;
        return 1;
    }
    f->chunkStart = f->pos;
    iff->frame->pos += zfileRead(iff->file, &iff->frame->chunkId, 8, 0);
    return 1;
}

BMX_ADDR(0x0049b250)
int ziffEndModel(void *iffp)
{
    Ziff *iff = iffp;

    if ((uintptr_t)iff->frame <= (uintptr_t)iff + 8) {
        zdebugFail("Too many closes", kZiffFile, 0x348, 0);
        return 0;
    }
    iff->frame = iff->frame - 1;
    return ziffSeek(iff, 1, 0) != -1;
}

/* The chunk the reader is on has an id and a size that is not a multiple of four. */
static void failNotWordSize(uint32_t id, int line)
{
    failIdMessage("Data length not word size for header:'%c%c%c%c'", id, line);
}

/*
 * Skips chunks until one with `id` (any when -1) is found, from the chunk after the one just read.
 * Shared by ziffFindChunk and ziffNextChunk, which differ only in how they start.
 */
static int scanForChunk(Ziff *iff, uint32_t id)
{
    if (id == 0xffffffffu) {
        return iff->frame->pos - iff->frame->chunkStart != -1;
    }
    while (iff->frame->pos < iff->frame->end) {
        ZiffFrame *f = iff->frame;

        if (id == f->chunkId) {
            return f->pos - f->chunkStart != -1;
        }
        if ((f->chunkSize & 3) != 0) {
            failNotWordSize(iff->frame->chunkId, 0xe4);
            return 0;
        }
        if (ziffSeek(iff, 0, (f->chunkStart - f->start) + (int32_t)f->chunkSize) != -1) {
            readHeader(iff);
        }
    }
    return 0;
}

BMX_ADDR(0x0049ad90)
int ziffFindChunk(void *iffp, uint32_t id)
{
    Ziff *iff = iffp;

    ziffSeek(iff, 0, 0);
    readHeader(iff);
    if ((iff->frame->chunkSize & 3) != 0) {
        failNotWordSize(iff->frame->chunkId, 0xcd);
        return 0;
    }
    return scanForChunk(iff, id);
}

BMX_ADDR(0x0049aed0)
int ziffNextChunk(void *iffp, uint32_t id)
{
    Ziff *iff = iffp;
    ZiffFrame *f = iff->frame;

    if ((f->chunkSize & 3) != 0) {
        failNotWordSize(f->chunkId, 0xcd);
        return 0;
    }
    if (ziffSeek(iff, 0, (f->chunkStart - f->start) + (int32_t)f->chunkSize) == -1) {
        return 0;
    }
    ziffReadChunkHeader(iff);
    if (iff->frame->pos == -1) {
        return 0;
    }
    return scanForChunk(iff, id);
}

BMX_ADDR(0x0049abc0)
uint32_t ziffChunkSize(void *iffp)
{
    return ((Ziff *)iffp)->frame->chunkSize;
}

BMX_ADDR(0x0049ac30)
int ziffRead(void *iffp, void *dst, uint32_t size)
{
    Ziff *iff = iffp;
    const uint32_t left = (uint32_t)(iff->frame->chunkStart - iff->frame->pos) + 8 + iff->frame->chunkSize;

    if (size == 0) {
        zdebugFail("Trying to read 0 bytes", kZiffFile, 0x265, 0);
        return 0;
    }
    if (left == 0) {
        zdebugFail("All data read", kZiffFile, 0x266, 0);
        return 0;
    }
    if (left < size) {
        gZdebugPrint("ZIFF: ziffChunkReadData: Clamping size\n");
        size = left;
    }
    iff->frame->pos += zfileRead(iff->file, dst, (int)size, 0);
    return (int)size;
}

BMX_ADDR(0x0049abe0)
int ziffReadChunk(void *iffp, void *dst)
{
    Ziff *iff = iffp;

    if (iff->frame->pos - iff->frame->chunkStart != 8) {
        zdebugFail("Not at start of chunk", kZiffFile, 0x251, 0);
        return 0;
    }
    return ziffRead(iff, dst, iff->frame->chunkSize);
}

BMX_ADDR(0x0049acf0)
void ziffReadWordA(void *iff, void *dst)
{
    ziffRead(iff, dst, 4);
}

BMX_ADDR(0x0049ad10)
void ziffReadWordB(void *iff, void *dst)
{
    ziffRead(iff, dst, 4);
}

/* Registered handler of the chunk id, or NULL. */
static ZiffCallbackNode *findCallback(uint32_t id)
{
    for (ZiffCallbackNode *node = gZiff.callbackHead; node != NULL; node = node->next) {
        if (node->id == id) {
            return node;
        }
    }
    return NULL;
}

BMX_ADDR(0x0049a8e0)
int ziffProcessChunks(void *iffp, void *arg)
{
    Ziff *iff = iffp;

    ziffSeek(iff, 0, 0);
    iff->frame->chunkStart = iff->frame->pos;
    iff->frame->pos += zfileRead(iff->file, &iff->frame->chunkId, 8, 0);
    for (;;) {
        ZiffFrame *f = iff->frame;

        if (f->pos < f->end) {
            const uint32_t id = f->chunkId;
            ZiffCallbackNode *node = findCallback(id);

            if (node == NULL) {
                failIdMessage("Unregistered header='%c%c%c%c'", id, 0x373);
                return 0;
            }
            if (node->fn(iff, arg) == 0) {
                return 0;
            }
        }
        f = iff->frame;
        if (ziffSeek(iff, 0, (f->chunkStart - f->start) + (int32_t)f->chunkSize) == -1) {
            return 1;
        }
        iff->frame->chunkStart = iff->frame->pos;
        iff->frame->pos += zfileRead(iff->file, &iff->frame->chunkId, 8, 0);
        if (iff->frame->pos == -1) {
            return 1;
        }
    }
}

BMX_ADDR(0x0049b2a0)
int ziffDispatchChunk(void *iffp, void *arg)
{
    Ziff *iff = iffp;
    ZiffCallbackNode *node;

    if (iff->frame->end <= iff->frame->pos) {
        return 1;
    }
    node = findCallback(iff->frame->chunkId);
    if (node == NULL) {
        failIdMessage("Unregistered header='%c%c%c%c'", iff->frame->chunkId, 0x373);
        return 0;
    }
    return node->fn(iff, arg) != 0;
}

/* ---- animations -------------------------------------------------------------------------------- */

BMX_ADDR(0x0049a4e0)
void *ziffReadAnim(void *iff)
{
    if (ziffBeginModel(iff) != 0) {
        void *anim = zmemAlloc(0x14);
        zmemFill(anim, 0, 0x14);
        gZiffAnim = anim;
        ziffProcessChunks(iff, NULL);
        ziffEndModel(iff);
        return gZiffAnim;
    }
    return NULL;
}

BMX_ADDR(0x0049a530)
uint32_t ziffAnimName(void *anim)
{
    return *(uint32_t *)anim;
}

/* Frees a chain whose nodes hold {data at +dataOff, next at +nextOff}. */
static void freeChain(uint8_t *node, unsigned dataOff, unsigned nextOff)
{
    while (node != NULL) {
        uint8_t *next = *(uint8_t **)(node + nextOff);
        zmemFree(*(void **)(node + dataOff));
        zmemFree(node);
        node = next;
    }
}

BMX_ADDR(0x0049a630)
void ziffFreeAnim(void *anim)
{
    uint8_t *a = anim;

    freeChain(*(uint8_t **)(a + 0xc), 8, 0xc);
    freeChain(*(uint8_t **)(a + 8), 8, 0xc);
    freeChain(*(uint8_t **)(a + 0x10), 8, 0xc);
    freeChain(*(uint8_t **)(a + 4), 4, 8);
    zmemFree(anim);
}

/* ---- skeletons (renderer side, see the header) ------------------------------------------------- */

BMX_ADDR(0x0049a540)
void *zrwmatFindSkeletonNode(void *skeleton, uint32_t hash)
{
    ZSkeleton *s = skeleton;
    ZSkeletonNode *node = s->nodes;

    for (uint32_t left = s->count; left != 0; left--, node++) {
        if (node->hash == hash) {
            return node;
        }
    }
    return NULL;
}

BMX_ADDR(0x0049a570)
void zrwmatSetNodeRotation(void *nodep, const float *quat)
{
    ZSkeletonNode *node = nodep;

    node->quat[0] = quat[0];
    node->quat[1] = quat[1];
    node->quat[2] = quat[2];
    node->quat[3] = quat[3];
    node->flags |= 1;
}

BMX_ADDR(0x004993f0)
void *zrwmatBuildSkeleton(void *object)
{
    const int count = zrwmatNodeCount(object);
    const size_t bytes = (size_t)count * sizeof(ZSkeletonNode) + sizeof(ZSkeleton);
    ZSkeleton *skel = zmemAlloc((uint32_t)bytes);
    uint32_t *hashes;
    uint32_t prev = 0;

    zmemFill(skel, 0, (uint32_t)bytes);
    hashes = zmemAlloc((uint32_t)count * 4);
    zrwmatGetNodeHashes(object, hashes);
    skel->count = (uint32_t)count;
    skel->nodes = skel->data;
    for (int i = 0; i < count; i++) {
        uint32_t smallest = 0xffffffffu;
        ZSkeletonNode *node = &skel->data[i];

        for (int j = 0; j < count; j++) {
            if (hashes[j] < smallest && prev < hashes[j]) {
                smallest = hashes[j];
            }
        }
        node->hash = smallest;
        node->quat[0] = 0;
        node->quat[1] = 0;
        node->quat[2] = 0;
        node->quat[3] = 1.0f;
        node->scale[0] = 1.0f;
        node->scale[1] = 1.0f;
        node->scale[2] = 1.0f;
        prev = smallest;
    }
    zmemFree(hashes);
    zrwmatSetSkeleton(object, skel);
    return skel;
}

BMX_ADDR(0x004994d0)
void zrwmatFreeSkeleton(void *object)
{
    ZSkeleton *skel = ((ZSkeletonOwner *)object)->skeleton;

    if (skel == NULL) {
        return;
    }
    for (uint32_t i = 0; i < skel->count; i++) {
        ZSkeletonNode *node = &skel->nodes[i];
        ZSkeletonItem *item;

        while ((item = node->items) != NULL) {
            node->items = item->next;
            item->next = gZiffItemPool;
            gZiffItemPool = item;
        }
    }
    zmemFree(skel);
}
