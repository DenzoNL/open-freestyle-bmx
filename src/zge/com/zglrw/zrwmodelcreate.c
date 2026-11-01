/* Model loading and instancing (C:\NgEngine\Zge\Com\ZglRw\zrwModelCreate.c). */
#include "zge/com/zglrw/zrwmodelcreate.h"

#include <stddef.h>

#include "compat/crt.h"
#include "rw/rwclump.h"
#include "rw/rwcore.h"
#include "rw/rwmath.h"
#include "rw/rwplugins.h"
#include "rw/rwstream.h"
#include "rw/rwtexture.h"
#include "rw/rwworld.h"
#include "zge/com/zglrw/zrwmat.h"
#include "zge/com/zglrw/zrwmodelrender.h"
#include "zge/com/zglrw/zrwtxr.h"
#include "zge/zasset.h"
#include "zge/zdebug.h"
#include "zge/zfile.h"
#include "zge/ziff.h"
#include "zge/zlist.h"
#include "zge/zmem.h"
#include "zge/zquat.h"
#include "zge/zstring.h"

static const char kModelCreateFile[] = "C:\\NgEngine\\Zge\\Com\\ZglRw\\zrwModelCreate.c";

/* Chunk ids of a model file (four characters, read as a little endian word). */
enum {
    kChunkModl = 0x4c444f4d, /* "MODL" a model */
    kChunkMdef = 0x4645444d, /* "MDEF" its definition (ZrwMdefHeader and the entries) */
    kChunkMati = 0x4954414d, /* "MATI" a material */
    kChunkMlod = 0x444f4c4d, /* "MLOD" a level of detail */
    kChunkMldf = 0x46444c4d, /* "MLDF" the header of a level of detail (ZrwLodHeader) */
    kChunkMcol = 0x4c4f434d, /* "MCOL" the collision clump */
    kChunkMdff = 0x4646444d, /* "MDFF" a RenderWare clump stream */
};

/* The MDEF chunk's header. */
typedef struct ZrwMdefHeader {
    uint32_t options;    /* 0x00 kZrwModel* */
    char name[0x10];     /* 0x04 */
    uint32_t defaultKey; /* 0x14 */
    uint32_t pad18[4];
    uint32_t entryCount; /* 0x28 followed by that many ZrwModelEntry name / key pairs */
} ZrwMdefHeader;

/* The MLDF chunk. */
typedef struct ZrwLodHeader {
    uint32_t pad00;
    float distance; /* 0x04 */
} ZrwLodHeader;

_Static_assert(offsetof(ZrwMdefHeader, defaultKey) == 0x14, "MDEF header");
_Static_assert(sizeof(ZrwMdefHeader) == 0x2c, "MDEF header");
_Static_assert(sizeof(ZrwLodHeader) == 8, "MLDF chunk");

/* Default distances of the four levels of detail. */
typedef float ZrwLodTable[kZrwModelMaxLods];
#define gZrwLodDefaults ORIG_VAR(0x00562cc4, ZrwLodTable, gZrwLodDefaults)

typedef void *(BMX_CDECL *RwFrameCallback)(void *frame, void *data);
typedef void *(BMX_CDECL *RpAtomicCallback)(void *atomic, void *data);

/* The same function called with a stray second argument (see zrwAtomicCountGeometry). */
ORIG_FN(0x004b4720, int, RpGeometryGetNumTrianglesWithArg, (void *geometry, int vertices));

/* ---- decompression ------------------------------------------------------------------------------ */

/* TODO(file): 0x49bd10 and 0x49c080 have no asserts; their "ZCOMPRES:" messages suggest a source file of
 * their own between zrwShadow.c and zrwModelCreate.c. */
#ifdef BMX_HOOK
#define kHashDummy ((const uint8_t *)BMX_ORIG(0x00562c04))
#else
static const uint8_t kHashDummy[] = "123456789012345678";
#endif

static inline uint32_t hashBucket(uint32_t a, uint32_t b, uint32_t c)
{
    return (uint32_t)((int32_t)((((a << 4) ^ b) << 4 ^ c) * (uint32_t)-0x1a1) >> 4) & 0x1ff;
}

BMX_ADDR(0x0049bd10)
void zassetDecompressHashed(void *work, const uint8_t *src, int srcLength, uint8_t *dst, int *outLength)
{
    const uint8_t *const end = src + srcLength;
    const uint8_t **table = (const uint8_t **)(((uintptr_t)work + 3) & ~(uintptr_t)3);
    const uint8_t *ip = src + 4;
    uint32_t ring = 0;
    uint32_t flags = 1;
    uint32_t pending = 0;
    uint8_t *op = dst;

    if (src[0] == 1) {
        const int length = srcLength - 4;
        for (int i = 0; i < length; i++) {
            dst[i] = ip[i];
        }
        *outLength = length;
        return;
    }
    for (int i = 0; i < 0x1000; i++) {
        table[i] = kHashDummy;
    }
    while (ip != end) {
        uint32_t cells;

        if (flags == 1) {
            flags = ip[0] | 0x10000u | ((uint32_t)ip[1] << 8);
            ip += 2;
        }
        cells = ((uintptr_t)(end - 0x20) < (uintptr_t)ip) ? 1 : 16;
        for (; cells != 0; cells--) {
            uint8_t *at = op;

            if ((flags & 1) == 0) {
                *at = *ip;
                op = at + 1;
                ip++;
                pending++;
                if (pending == 3) {
                    table[ring + hashBucket(at[-2], at[-1], at[0]) * 8] = at - 2;
                    ring = (ring + 1) & 7;
                    pending = 2;
                }
            } else {
                const uint8_t b0 = ip[0];
                const uint8_t b1 = ip[1];
                const uint32_t high = (uint32_t)(b0 & 0xf0) << 4;
                uint32_t extra = b0 & 0xfu;
                const uint8_t *from = table[high | b1];

                ip += 2;
                at[0] = from[0];
                at[1] = from[1];
                at[2] = from[2];
                op = at + 3;
                from += 3;
                while (extra != 0) {
                    *op++ = *from++;
                    extra--;
                }
                if (pending != 0) {
                    const uint8_t *back = at - pending;

                    table[ring + hashBucket(back[0], back[1], back[2]) * 8] = back;
                    ring = (ring + 1) & 7;
                    if (pending == 2) {
                        table[ring + hashBucket(back[1], back[2], back[3]) * 8] = back + 1;
                        ring = (ring + 1) & 7;
                    }
                    pending = 0;
                }
                table[(high | (b1 & 0xfffffff8u)) + ring] = at;
                ring = (ring + 1) & 7;
            }
            flags >>= 1;
        }
    }
    *outLength = (int)(op - dst);
}

/* The LZO1X decoder the asset files use (method 2). */
BMX_ADDR(0x0049c080)
int zassetDecompress(int kind, const void *packed, uint32_t size, void *dest, uint32_t *outSize)
{
    const int method = kind;
    const uint8_t *const src = packed;
    const int srcLength = (int)size;
    uint8_t *const dst = dest;
    int *const outLength = (int *)outSize;
    const uint8_t *ip;
    uint8_t *op;
    int afterMatch = 0;
    int afterLiteral = 0;

    if (method == 1) {
        void *work = zmemAlloc(0x4010);
        zassetDecompressHashed(work, src, srcLength, dst, outLength);
        zmemFree(work);
        return 0;
    }
    if (method != 2) {
        gZdebugPrint("ZCOMPRES: Unknown decompression method\n");
        return 1;
    }
    op = dst;
    *outLength = 0;
    ip = src;
    if (srcLength == 0) {
        return 0;
    }
    for (;;) {
        uint8_t t;
        const uint8_t *next;
        int length;
        uint32_t tail;
        int distance;

        for (;;) {
            t = *ip;
            next = ip + 1;
            if (afterLiteral) {
                break;
            }
            if (t < 0x10) {
                if (t == 0) {
                    length = 0x12;
                    t = *next;
                    next = ip + 2;
                    while (t == 0) {
                        length += 0xff;
                        t = *next++;
                    }
                    length += t;
                } else {
                    length = t + 3;
                }
                for (int i = 0; i < length; i++) {
                    op[i] = next[i];
                }
                ip = next + length;
                op += length;
                afterLiteral = 1;
                afterMatch = 0;
            } else if (next == src + 1) {
                if (t < 0x12) {
                    gZdebugPrint("Illegal starting byte");
                    gZdebugPrint("ZCOMPRES: Decompression failed!\n");
                    return 1;
                }
                length = t - 0x11;
                for (int i = 0; i < length; i++) {
                    op[i] = next[i];
                }
                ip = next + length;
                op += length;
                afterLiteral = 1;
                afterMatch = 0;
            } else {
                afterMatch = 0;
                afterLiteral = 1;
            }
        }
        if (t < 0x10) {
            tail = t & 3u;
            ip += 2;
            distance = (int)(((uint32_t)*next << 2) | ((t >> 2) & 3u));
            if (afterMatch) {
                distance += 1;
                length = 2;
            } else {
                distance += 0x801;
                length = 3;
            }
        } else if (t < 0x20) {
            const uint32_t low = t & 7u;
            uint32_t far;

            if (low == 0) {
                uint8_t b;
                length = 9;
                b = *next;
                next = ip + 2;
                while (b == 0) {
                    length += 0xff;
                    b = *next++;
                }
                length += b;
            } else {
                length = (int)low + 2;
            }
            tail = next[0] & 3u;
            ip = next + 2;
            far = ((((uint32_t)(t >> 3) & 1u) << 8 | next[1]) << 6) | (uint32_t)(next[0] >> 2);
            if (far == 0) {
                if (low == 1) {
                    *outLength = (int)(op - dst);
                    return 0;
                }
                gZdebugPrint("Faulty v value on EOF: %d", low);
                gZdebugPrint("ZCOMPRES: Decompression failed!\n");
                return 1;
            }
            distance = (int)(far + 0x4000);
        } else if (t < 0x40) {
            if ((t & 0x1f) == 0) {
                uint8_t b;
                length = 0x21;
                b = *next;
                next = ip + 2;
                while (b == 0) {
                    length += 0xff;
                    b = *next++;
                }
                length += b;
            } else {
                length = (t & 0x1f) + 2;
            }
            tail = next[0] & 3u;
            ip = next + 2;
            distance = (int)(((uint32_t)next[1] << 6 | (uint32_t)(next[0] >> 2)) + 1);
        } else {
            tail = t & 3u;
            ip += 2;
            distance = (int)((((uint32_t)*next << 3) | ((t >> 2) & 7u)) + 1);
            length = (t >> 5) + 1;
        }
        {
            const uint8_t *from = op - distance;
            for (int i = 0; i < length; i++) {
                op[i] = from[i];
            }
        }
        op += length;
        if (tail == 0) {
            afterMatch = 1;
            afterLiteral = 0;
        } else {
            for (uint32_t i = 0; i < tail; i++) {
                op[i] = ip[i];
            }
            ip += tail;
            op += tail;
            afterMatch = 1;
        }
    }
}

/* ---- frames and clumps -------------------------------------------------------------------------- */

/*
 * RpClumpClone does not copy the frames' user data (the part hashes), so zrwModelCreateInstance parks it in
 * each frame's matrix (at.x: low half, at.y: high half) for the clone and reads it back afterwards.
 */
BMX_ADDR(0x0049c6e0)
void *zrwFrameUserDataToMatrix(void *frame, void *arg)
{
    ZMatrix *matrix = RwFrameGetMatrix(frame);
    const uint32_t data = RwFrameGetUserData(frame);

    matrix->at.x = (float)(data & 0xffff);
    matrix->at.y = (float)(data >> 16);
    RwFrameForAllChildren(frame, BMX_FNPTR(RwFrameCallback, 0x0049c6e0, zrwFrameUserDataToMatrix), arg);
    return frame;
}

BMX_ADDR(0x0049c7a0)
void *zrwFrameMatrixToUserData(void *frame, void *arg)
{
    ZMatrix *matrix = RwFrameGetMatrix(frame);
    const uint16_t high = (uint16_t)bmxFtol((double)matrix->at.y);
    const uint32_t low = (uint32_t)bmxFtol((double)matrix->at.x);

    RwFrameSetUserData(frame, ((uint32_t)high << 16) | (low & 0xffff));
    RwFrameForAllChildren(frame, BMX_FNPTR(RwFrameCallback, 0x0049c7a0, zrwFrameMatrixToUserData), arg);
    return frame;
}

/* Materials are matched by texture: the loader gave the model's ZrwMaterial and the RpMaterial the same one.
 */
BMX_ADDR(0x0049c880)
void zrwModelSetRwInfo(ZrwModel *model)
{
    RpGeometry *geometry;

    zrwMdlSetUserData(model->clump, (uint32_t)(uintptr_t)model);
    if (model->collisionClump != NULL) {
        zrwMdlSetUserData(model->collisionClump, (uint32_t)(uintptr_t)model);
    }
    geometry = model->def->geometry;
    for (void *material = zlistFirst(model->materials); material != NULL;
         material = zlistNext(model->materials)) {
        void *texture = zrwmatGetTexture(material, 0);
        void *rwTexture = zrwTextureGetRwTexture(texture);
        const char *name = zrwTextureGetName(texture);
        int index = RpGeometryGetNumMaterials(geometry) - 1;
        int matched = 0;

        for (; index >= 0; index--) {
            RpMaterial *rpMaterial = RpGeometryGetMaterial(geometry, index);

            if (RpMaterialGetTexture(rpMaterial) == rwTexture) {
                zrwmatSetTexture(material, rpMaterial);
                matched = 1;
                break;
            }
        }
        if (!matched) {
            gZdebugPrint("zrwModelSetRwInfo: Failed to match RpMaterial:%s\n", name);
        }
    }
}

BMX_ADDR(0x0049c970)
void *zrwAtomicCountGeometry(void *atomic, int *counts)
{
    RpGeometry *geometry = ((const RpAtomic *)atomic)->geometry;

    counts[0] += RpGeometryGetNumTriangles(geometry);
    counts[1] += RpGeometryGetNumVertices(geometry);
    const int vertices = RpGeometryGetNumVertices(geometry);
    /* The original passes the vertex count as a second argument and prints one value for two specifiers. */
    gZdebugPrint(" [F:%d, V:%d]", RpGeometryGetNumTrianglesWithArg(geometry, vertices));
    return atomic;
}

BMX_ADDR(0x0049cc60)
void *zrwFrameCollectFirst(void *object, void *out)
{
    *(void **)out = object;
    return NULL;
}

BMX_ADDR(0x0049c9c0)
void zrwClumpDump(void *clump)
{
    RpAtomic *first = NULL;
    int counts[2];
    RpGeometry *geometry;

    RpClumpForAllAtomics(clump, BMX_FNPTR(RpAtomicCallback, 0x0049cc60, zrwFrameCollectFirst),
                         (void *)&first);
    geometry = first->geometry;
    counts[0] = 0;
    counts[1] = 0;
    gZdebugPrint("Num atomics:%d (", RpClumpGetNumAtomics(clump));
    RpClumpForAllAtomics(clump, BMX_FNPTR(RpAtomicCallback, 0x0049c970, zrwAtomicCountGeometry), counts);
    gZdebugPrint(" )\n");
    gZdebugPrint("Num triangles:%d\n", counts[0]);
    gZdebugPrint("Num vertices :%d\n", counts[1]);
    gZdebugPrint("Flags (Atomic0):");
    if ((RpGeometryGetFlags(geometry) & rpGEOMETRYTEXTURED) != 0) {
        gZdebugPrint(" rpGEOMETRYTEXTURED");
    }
    if ((RpGeometryGetFlags(geometry) & rpGEOMETRYPRELIT) != 0) {
        gZdebugPrint(" rpGEOMETRYPRELIT");
    }
    if ((RpGeometryGetFlags(geometry) & rpGEOMETRYNORMALS) != 0) {
        gZdebugPrint(" rpGEOMETRYNORMALS");
    }
    if ((RpGeometryGetFlags(geometry) & rpGEOMETRYLIGHT) != 0) {
        gZdebugPrint(" rpGEOMETRYLIGHT");
    }
    if ((RpGeometryGetFlags(geometry) & rpGEOMETRYTRISTRIP) != 0) {
        gZdebugPrint(" rpGEOMETRYTRISTRIP");
    }
    gZdebugPrint("\n");
}

BMX_ADDR(0x0049caf0)
void *zrwFrameDumpHierarchy(void *frame, void *model)
{
    const uint32_t key = RwFrameGetUserData(frame);
    const ZrwModelEntry *entry = zrwModelFindEntry(model, (int)key);

    if (entry == NULL) {
        gZdebugPrint("0x%x\n", key);
    } else {
        gZdebugPrint("0x%x (%s)\n", key, entry->name);
    }
    gZdebugIndent(1);
    RwFrameForAllChildren(frame, BMX_FNPTR(RwFrameCallback, 0x0049caf0, zrwFrameDumpHierarchy),
                          (void *)model);
    gZdebugIndent(-1);
    return frame;
}

/* The root frame's step of zrwFrameDumpHierarchy is inlined (as in the original). */
BMX_ADDR(0x0049cb60)
void zrwModelDumpHierarchy(const ZrwModel *model)
{
    RwFrame *frame = RpClumpGetFrame(model->clump);
    uint32_t key;
    const ZrwModelEntry *entry;

    gZdebugPrint("HIERARCHY:-\n");
    gZdebugIndent(1);
    key = RwFrameGetUserData(frame);
    entry = zrwModelFindEntry(model, (int)key);
    if (entry == NULL) {
        gZdebugPrint("0x%x\n", key);
    } else {
        gZdebugPrint("0x%x (%s)\n", key, entry->name);
    }
    gZdebugIndent(1);
    RwFrameForAllChildren(frame, BMX_FNPTR(RwFrameCallback, 0x0049caf0, zrwFrameDumpHierarchy),
                          (void *)model);
    gZdebugIndent(-1);
    gZdebugIndent(-1);
}

BMX_ADDR(0x0049cc70)
void *zrwModelCreateFirstAtomic(void *clump)
{
    void *first = NULL;

    RpClumpForAllAtomics(clump, BMX_FNPTR(RpAtomicCallback, 0x0049cc60, zrwFrameCollectFirst),
                         (void *)&first);
    return first;
}

/* Fills a part from its frame; the hierarchy links are left to the caller. */
static void initPart(ZrwModelPart *part, RwFrame *frame)
{
    const ZMatrix *matrix = RwFrameGetMatrix(frame);

    part->hash = RwFrameGetUserData(frame);
    part->def = NULL;
    part->frame = frame;
    part->restPos[0] = matrix->pos.x;
    part->restPos[1] = matrix->pos.y;
    part->restPos[2] = matrix->pos.z;
}

BMX_ADDR(0x0049cce0)
void *zrwFrameBuildPartNode(void *frame, void *parentNode)
{
    ZrwModelPart *part = gZrwModelCreate.cursor;
    ZrwModelPart *parent = parentNode;

    gZrwModelCreate.cursor = part + 1;
    initPart(part, frame);
    part->parent = parent;
    if (parent != NULL) {
        part->sibling = parent->child;
        parent->child = part;
    }
    RwFrameForAllChildren(frame, BMX_FNPTR(RwFrameCallback, 0x0049cce0, zrwFrameBuildPartNode), part);
    return frame;
}

/* Allocates the `count` parts of a frame hierarchy (count = RwFrameCount(frame)), fills them, returns the
 * root. */
static ZrwModelPart *newPartTree(RwFrame *frame, int count)
{
    const uint32_t size = (uint32_t)count * (uint32_t)sizeof(ZrwModelPart);
    ZrwModelPart *root = zmemFill(zmemAlloc(size), 0, size);

    gZrwModelCreate.cursor = root + 1;
    initPart(root, frame);
    root->parent = NULL;
    RwFrameForAllChildren(frame, BMX_FNPTR(RwFrameCallback, 0x0049cce0, zrwFrameBuildPartNode), root);
    return root;
}

/*
 * Only the first clump is cloned, and every level of detail of the instance gets the parts of that clone.
 * The root frame's steps of zrwFrameUserDataToMatrix / zrwFrameMatrixToUserData are inlined (as in the
 * original); the source model's child frames keep the user data in their matrices.
 */
BMX_ADDR(0x0049ce90)
ZrwModel *zrwModelCreateInstance(const ZrwModel *model)
{
    ZrwModel *instance;
    RwFrame *frame;
    ZMatrix *matrix;
    uint32_t data;
    RwFrame *cloneFrame;

    if ((model->def->options & kZrwModelSkinned) != 0) {
        gZdebugPrint("zrwModelCreateInstance: Not instancing skinned model - ReLoading\n");
        return zrwModelLoadFromFile(model->def->path);
    }
    instance = zmemFreeListAlloc(gZrwModelCreate.pool);
    zmemCopy(instance, model, sizeof *instance);
    instance->def->refCount++;
    frame = RpClumpGetFrame(model->clump);
    matrix = RwFrameGetMatrix(frame);
    data = RwFrameGetUserData(frame);
    matrix->at.x = (float)(data & 0xffff);
    matrix->at.y = (float)(data >> 16);
    RwFrameForAllChildren(frame, BMX_FNPTR(RwFrameCallback, 0x0049c6e0, zrwFrameUserDataToMatrix), NULL);
    instance->clump = RpClumpClone(model->clump);
    cloneFrame = RpClumpGetFrame(instance->clump);
    matrix = RwFrameGetMatrix(cloneFrame);
    {
        const uint16_t high = (uint16_t)bmxFtol((double)matrix->at.y);
        const uint32_t low = (uint32_t)bmxFtol((double)matrix->at.x);
        RwFrameSetUserData(cloneFrame, ((uint32_t)high << 16) | (low & 0xffff));
    }
    RwFrameForAllChildren(cloneFrame, BMX_FNPTR(RwFrameCallback, 0x0049c7a0, zrwFrameMatrixToUserData), NULL);
    RwFrameSetIdentity(RpClumpGetFrame(instance->clump));
    RwFrameSetIdentity(RpClumpGetFrame(model->clump));
    if (model->collisionClump != NULL) {
        instance->collisionClump = RpClumpClone(model->collisionClump);
    }
    zrwModelSetRwInfo(instance);
    for (uint32_t level = 0; level < instance->def->lodCount; level++) {
        ZrwModelLod *lod = &instance->lods[level];
        RwFrame *const root = RpClumpGetFrame(instance->clump);
        const int count = RwFrameCount(root);

        lod->parts = newPartTree(root, count);
        lod->partCount = (uint32_t)count;
    }
    instance->materials = zlistCreate(0x10);
    for (void *material = zlistFirst(model->materials); material != NULL;
         material = zlistNext(model->materials)) {
        zlistInsert(instance->materials, material);
        zrwmatRetainMaterial(material);
    }
    zrwModelSetInWorld(instance, 1);
    if ((instance->def->options & kZrwModelNoCollision) == 0) {
        zrwModelSetCollision(instance, 1);
    }
    return instance;
}

/*
 * The first level of detail's clump becomes the model's clump and its first atomic the definition's; every
 * level's parts are built from that first clump's hierarchy. The clump memory is freed in any case.
 */
BMX_ADDR(0x0049d120)
int zrwModelAddClump(ZrwModel *model, const uint32_t *memory, const char *imagePath, int collision)
{
    const RwMemory *const source = (const RwMemory *)memory;
    RpClump *clump = NULL;
    void *stream;

    if (imagePath != NULL) {
        RwImageSetPath(imagePath);
    }
    stream = RwStreamOpen(3, 1, memory);
    if (stream == NULL) {
        return 0;
    }
    if (RwStreamFindChunk(stream, 0x10, NULL, NULL) != 0) {
        clump = RpClumpStreamRead(stream);
    }
    RwStreamClose(stream, NULL);
    RwOsGetMemoryInterface()->rwfree(source->start);
    if (clump == NULL) {
        return 0;
    }
    if (collision == 1) {
        model->collisionClump = clump;
        return 1;
    }
    {
        ZrwModelDef *def = model->def;
        RwFrame *frame;
        int count;
        uint32_t level;

        gZrwModelCreate.lodClumps[def->lodCount] = clump;
        if (def->lodCount == 0) {
            model->clump = clump;
            RwFrameSetIdentity(RpClumpGetFrame(clump));
        }
        if (model->def->lodCount == 0) {
            RpAtomic *first = NULL;

            RpClumpForAllAtomics(clump, BMX_FNPTR(RpAtomicCallback, 0x0049cc60, zrwFrameCollectFirst),
                                 (void *)&first);
            model->def->atomic = first;
            model->def->atomicRender = model->def->atomic->renderCallBack;
            model->def->geometry = model->def->atomic->geometry;
        }
        level = model->def->lodCount;
        frame = RpClumpGetFrame(model->clump);
        count = RwFrameCount(frame);
        model->lods[level].partCount = (uint32_t)count;
        {
            ZrwModelPart *const parts = newPartTree(frame, count);

            model->lods[model->def->lodCount].parts = parts;
        }
        model->def->lodCount++;
        if ((model->def->options & kZrwModelUnlit) != 0) {
            zrwClumpMaskGeometryFlags(clump, rpGEOMETRYLIGHT, 0);
        }
        if ((model->def->options & (kZrwModelNoCollision | kZrwModelCollisionClump)) != 0) {
            zrwClumpMaskAtomicFlags(clump, rpATOMICCOLLISIONTEST, 0);
        }
        if ((model->def->options & (kZrwModelNoCollision | kZrwModelCollisionClump)) == 0) {
            zrwClumpMaskAtomicFlags(clump, 0, rpATOMICCOLLISIONTEST);
        }
        zrwClumpDump(clump);
    }
    return 1;
}

/* Reads the MDFF clump chunk the reader is on into RenderWare memory and loads it into the model. */
static int loadClumpChunk(void *iff, ZrwModel *model, int collision, int line)
{
    RwMemory memory;
    const uint32_t size = ziffChunkSize(iff);

    memory.start = RwOsGetMemoryInterface()->rwmalloc(size);
    memory.length = size;
    ziffRead(iff, memory.start, size);
    if (zrwModelAddClump(model, (const uint32_t *)&memory, NULL, collision) == 0) {
        zdebugFail("Clump failed to load", kModelCreateFile, line, 0);
        gZdebugPrint("Clump failed to load\n");
        return 0;
    }
    return 1;
}

BMX_ADDR(0x0049d350)
int zrwModelLoadLod(void *iff, ZrwModel *model)
{
    if (model->def->lodCount == kZrwModelMaxLods) {
        gZdebugPrint("Ignoring extra LOD in %s\n", model->def->name);
        return 0;
    }
    if (ziffBeginModel(iff) != 0) {
        if (ziffFindChunk(iff, kChunkMldf) != 0) {
            ZrwLodHeader header;
            uint32_t level;

            ziffRead(iff, &header, sizeof header);
            model->lods[model->def->lodCount].distance = header.distance;
            level = model->def->lodCount;
            /* Zero or NaN (what the x87 compare against 0 reports as equal). */
            if (!(model->lods[level].distance < 0.0f || model->lods[level].distance > 0.0f)) {
                model->lods[level].distance = gZrwLodDefaults[level];
            }
        }
        if (ziffFindChunk(iff, kChunkMdff) != 0) {
            loadClumpChunk(iff, model, 0, 0x3fc);
        }
        ziffEndModel(iff);
    }
    return 1;
}

/* The collision clump is never drawn but always tested, and it is in the world from the start. */
BMX_ADDR(0x0049d4d0)
int zrwModelLoadCollision(void *iff, ZrwModel *model)
{
    if (ziffBeginModel(iff) != 0) {
        if (ziffFindChunk(iff, kChunkMdff) != 0) {
            loadClumpChunk(iff, model, 1, 0x441);
            zrwClumpMaskAtomicFlags(model->collisionClump, rpATOMICRENDER, rpATOMICCOLLISIONTEST);
            RpWorldAddClump(gZrwWorld, model->collisionClump);
        }
        ziffEndModel(iff);
    }
    return 1;
}

BMX_ADDR(0x0049d5f0)
int zrwModelReadDef(void *iff, ZrwModel *model)
{
    ZrwMdefHeader header;

    if (ziffFindChunk(iff, kChunkMdef) == 0) {
        zdebugFail("Missing model def block", kModelCreateFile, 0x495, 0);
        return 0;
    }
    ziffRead(iff, &header, sizeof header);
    zstrncpy(model->def->name, header.name, sizeof model->def->name);
    model->def->defaultKey = header.defaultKey;
    model->def->options = header.options;
    model->def->entryCount = (uint16_t)header.entryCount;
    model->def->entries = zmemAlloc((header.entryCount & 0xffff) * (uint32_t)sizeof(ZrwModelEntry));
    for (int i = 0; i < model->def->entryCount; i++) {
        ZrwModelEntry *entry = &model->def->entries[i];

        ziffRead(iff, entry->name, sizeof entry->name);
        ziffReadWordA(iff, &entry->key);
        entry->name[sizeof entry->name - 1] = 0;
    }
    gZdebugPrint("-------------------------------------------------\n");
    gZdebugPrint("Loading model:%s\n", model->def->name);
    gZdebugIndent(1);
    return 1;
}

BMX_ADDR(0x0049d750)
int zrwModelAddMaterial(void *material, ZrwModel *model)
{
    zlistInsert(model->materials, material);
    return 1;
}

BMX_ADDR(0x0049d770)
int zrwModelLoadMaterials(void *iff, ZrwModel *model)
{
    void *previous = zrwmatSetMaterialHook(BMX_FNPTR(void *, 0x0049d750, zrwModelAddMaterial));
    int found = ziffFindChunk(iff, kChunkMati);

    while (found != 0) {
        if (ziffDispatchChunk(iff, model) == 0) {
            zrwmatSetMaterialHook(previous);
            return 0;
        }
        found = ziffNextChunk(iff, kChunkMati);
    }
    zrwmatSetMaterialHook(previous);
    return 1;
}

/*
 * The MODL chunk handler. The code from 0x49d895 on is entered by falling through (the saved registers of its
 * prologue are the arguments of the first call), so the data file lists it as a function of its own; it is
 * kept apart here for the same reason.
 */
BMX_ADDR(0x0049d7e0)
int zrwModelLoad(void *iff)
{
    ZrwModel *model;

    gZrwModelCreate.last = NULL;
    model = zmemFreeListAlloc(gZrwModelCreate.pool);
    memset(model, 0, sizeof *model);
    model->pad00 &= 0xffffff00u; /* the original clears this byte again after the memset */
    model->def = zmemAlloc(sizeof *model->def);
    memset(model->def, 0, sizeof *model->def);
    model->def->refCount = 1;
    model->materials = zlistCreate(0x10);
    zquatIdentity(&model->rotation);
    model->scale.x = 1.0f;
    model->scale.y = 1.0f;
    model->scale.z = 1.0f;
    model->color = 0xffffffffu;
    if (ziffBeginModel(iff) == 0) {
        return 0;
    }
    return zrwModelLoadTail(iff, model);
}

/* The MLOD and MCOL chunks reach zrwModelLoadLod / zrwModelLoadCollision through ziffDispatchChunk. */
BMX_ADDR(0x0049d895)
int zrwModelLoadTail(void *iff, ZrwModel *model)
{
    zrwModelReadDef(iff, model);
    zrwModelLoadMaterials(iff, model);
    {
        int found = ziffFindChunk(iff, kChunkMlod);

        while (found != 0 && ziffDispatchChunk(iff, model) != 0) {
            found = ziffNextChunk(iff, kChunkMlod);
        }
    }
    if ((model->def->options & kZrwModelCollisionClump) != 0 && ziffFindChunk(iff, kChunkMcol) != 0) {
        ziffDispatchChunk(iff, model);
    }
    if (ziffEndModel(iff) == 0) {
        return 0;
    }
    if ((model->def->options & kZrwModelSkinned) != 0) {
        RpSkinClumpSetup(model->clump);
    }
    zrwModelSetDefaultRenderCallback(model);
    if ((model->def->options & kZrwModelFollowCamera) != 0) {
        zrwModelRenderRegister(model);
    } else if ((model->def->options & kZrwModelNotInWorld) == 0) {
        RpWorldAddClump(gZrwWorld, model->clump);
    }
    zrwModelSetRwInfo(model);
    zrwModelUpdate(model);
    gZdebugIndent(-1);
    gZrwModelCreate.last = model;
    return 1;
}

BMX_ADDR(0x0049d9d0)
int zrwModelCreateInitialize(void)
{
    if (gZrwModelCreate.initialized != 0) {
        zdebugFail("Already initialized", kModelCreateFile, 0x52d, 0);
    }
    memset(&gZrwModelCreate, 0, sizeof gZrwModelCreate);
    gZrwModelCreate.pool = zmemFreeListCreate(
        sizeof(ZrwModel), 0x100, BMX_FNPTR(ZMemDestroyFn, 0x0049dcd0, (ZMemDestroyFn)zrwModelDestroy));
    gZrwModelCreate.initialized = 1;
    ziffSetupCallback(kChunkModl, BMX_FNPTR(int(BMX_CDECL *)(void *), 0x0049d7e0, zrwModelLoad), 0);
    ziffSetupCallback(
        kChunkMlod,
        BMX_FNPTR(int(BMX_CDECL *)(void *), 0x0049d350, (int(BMX_CDECL *)(void *))zrwModelLoadLod), 0);
    ziffSetupCallback(
        kChunkMcol,
        BMX_FNPTR(int(BMX_CDECL *)(void *), 0x0049d4d0, (int(BMX_CDECL *)(void *))zrwModelLoadCollision), 0);
    return 1;
}

BMX_ADDR(0x0049da60)
int zrwModelCreateShutdown(void)
{
    if (gZrwModelCreate.initialized != 0) {
        zmemFreeListDestroy(gZrwModelCreate.pool);
        gZrwModelCreate.initialized = 0;
    }
    return 1;
}

BMX_ADDR(0x0049da90)
ZrwModel *zrwModelLoadFromChunk(void *iff)
{
    ziffDispatchChunk(iff, NULL);
    return gZrwModelCreate.last;
}

/* Original quirk: without a MODL chunk inside the block, the buffer itself comes back as the model. */
BMX_ADDR(0x0049dab0)
ZrwModel *zrwModelLoadFromBuffer(const uint32_t *buffer)
{
    const uint32_t size = buffer[1];
    ZfileHandle *file;
    void *iff;
    const void *result = buffer;

    if (buffer[0] != kChunkModl) {
        zdebugFail("Not a MODL", kModelCreateFile, 0x577, 0);
        return NULL;
    }
    file = zfileOpenRamDisk((uint32_t)(uintptr_t)buffer, size + 8);
    if (file == NULL) {
        zdebugFail("Couldn't open MODL", kModelCreateFile, 0x57b, 0);
        return NULL;
    }
    iff = ziffOpen(file, 0xffffffffu, size + 8);
    if (iff == NULL) {
        zdebugFail("Couldn't open MODL", kModelCreateFile, 0x57e, 0);
        return NULL;
    }
    if (ziffFindChunk(iff, kChunkModl) != 0) {
        ziffDispatchChunk(iff, NULL);
        result = gZrwModelCreate.last;
    }
    ziffClose(iff);
    zfileClose(file, 0);
    return (ZrwModel *)(uintptr_t)result;
}

/*
 * zrwModelLoadFromBuffer on the file's contents, then the path is stored in the definition. Original quirk:
 * without a MODL chunk inside, the path string comes back as the model (and gets the path written into it).
 */
BMX_ADDR(0x0049db90)
ZrwModel *zrwModelLoadFromFile(const char *path)
{
    int size;
    uint32_t *buffer = zfileLoad(path, &size);
    const void *result;
    uint32_t length;

    if (buffer == NULL) {
        return NULL;
    }
    length = buffer[1];
    if (buffer[0] == kChunkModl) {
        ZfileHandle *file = zfileOpenRamDisk((uint32_t)(uintptr_t)buffer, length + 8);

        if (file == NULL) {
            zdebugFail("Couldn't open MODL", kModelCreateFile, 0x57b, 0);
            result = NULL;
        } else {
            void *iff = ziffOpen(file, 0xffffffffu, length + 8);

            if (iff == NULL) {
                zdebugFail("Couldn't open MODL", kModelCreateFile, 0x57e, 0);
                result = NULL;
            } else {
                if (ziffFindChunk(iff, kChunkModl) != 0) {
                    ziffDispatchChunk(iff, NULL);
                    result = gZrwModelCreate.last;
                } else {
                    result = path;
                }
                ziffClose(iff);
                zfileClose(file, 0);
            }
        }
    } else {
        zdebugFail("Not a MODL", kModelCreateFile, 0x577, 0);
        result = NULL;
    }
    zfileFreeBuffer(buffer);
    if (result != NULL) {
        ZrwModelDef *const def = ((const ZrwModel *)result)->def;

        zstrncpy(def->path, path, sizeof def->path);
    }
    return (ZrwModel *)(uintptr_t)result;
}

BMX_ADDR(0x0049dcb0)
int zrwModelNotCoded(void)
{
    zdebugFail("Not coded", kModelCreateFile, 0x5ba, 0);
    return 0;
}

/* The textures are released only with the last model that uses the definition. */
BMX_ADDR(0x0049dcd0)
int zrwModelDestroy(ZrwModel *model)
{
    uint32_t references;

    if (model->clump != NULL) {
        if (RpClumpGetWorld(model->clump) != NULL) {
            RpWorldRemoveClump(gZrwWorld, model->clump);
        }
        RpClumpDestroy(model->clump);
    }
    if (model->collisionClump != NULL) {
        if (RpClumpGetWorld(model->collisionClump) != NULL) {
            RpWorldRemoveClump(gZrwWorld, model->collisionClump);
        }
        RpClumpDestroy(model->collisionClump);
    }
    if ((model->def->options & kZrwModelFollowCamera) != 0) {
        zrwModelRenderUnregister();
    }
    references = model->def->refCount;
    for (void *material = zlistFirst(model->materials); material != NULL;
         material = zlistFirst(model->materials)) {
        if (references == 1) {
            zrwTextureRelease(zrwmatGetTexture(material, 0));
        }
        zrwmatReleaseTexture(material);
        zlistRemoveCurrent(model->materials);
    }
    zlistDestroy(model->materials);
    for (uint32_t level = 0; level < model->def->lodCount; level++) {
        zmemFree(model->lods[level].parts);
    }
    model->def->refCount--;
    if (model->def->refCount == 0) {
        zmemFree(model->def->entries);
        zmemFree(model->def);
    }
    zmemFreeListFree(gZrwModelCreate.pool, model);
    return 1;
}

BMX_ADDR(0x0049de40)
void zrwModelPrintUsage(void)
{
    gZdebugPrint("ZGLMODEL: Objects in use:%d\n", zmemFreeListUsedCount(gZrwModelCreate.pool));
}

/* zrwModelDumpHierarchy is inlined (as in the original). */
BMX_ADDR(0x0049de60)
void zrwModelDumpInfo(const ZrwModel *model)
{
    RpClump *clump = model->clump;
    RwFrame *frame;
    uint32_t key;
    const ZrwModelEntry *entry;

    gZdebugPrint("-------------------------------------------------\n");
    gZdebugPrint("Info for model:%s\n", model->def->name);
    gZdebugPrint("-------------------------------------------------\n");
    gZdebugIndent(1);
    zrwClumpDump(clump);
    frame = RpClumpGetFrame(model->clump);
    gZdebugPrint("HIERARCHY:-\n");
    gZdebugIndent(1);
    key = RwFrameGetUserData(frame);
    entry = zrwModelFindEntry(model, (int)key);
    if (entry == NULL) {
        gZdebugPrint("0x%x\n", key);
    } else {
        gZdebugPrint("0x%x (%s)\n", key, entry->name);
    }
    gZdebugIndent(1);
    RwFrameForAllChildren(frame, BMX_FNPTR(RwFrameCallback, 0x0049caf0, zrwFrameDumpHierarchy),
                          (void *)model);
    gZdebugIndent(-1);
    gZdebugIndent(-1);
    gZdebugPrint("MATERIALS:-\n");
    gZdebugIndent(1);
    for (void *material = zlistFirst(model->materials); material != NULL;
         material = zlistNext(model->materials)) {
        gZdebugPrint("%s\n", zrwTextureGetName(zrwmatGetTexture(material, 0)));
    }
    gZdebugIndent(-1);
    gZdebugIndent(-1);
    gZdebugPrint("-------------------------------------------------\n");
}
