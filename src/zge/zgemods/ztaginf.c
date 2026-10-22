/* Level tag tables (C:\NgEngine\Zge\ZgeMods\ztaginf.c). */
#include "zge/zgemods/ztaginf.h"

#include <stddef.h>

#include "zge/zdebug.h"
#include "zge/ziff.h"
#include "zge/zmem.h"

static const char kZtaginfFile[] = "C:\\NgEngine\\Zge\\ZgeMods\\ztaginf.c";

/* Cleared by the initialiser, never read. */
#define gZtaginfUnused  ORIG_VAR(0x006728b8, uint32_t, gZtaginfUnused)
#define gZtaginfUnused2 ORIG_VAR(0x00672900, uint32_t, gZtaginfUnused2)

enum {
    kFaceStride = 0x6c,
    kTagChunkD = 0x44474154, /* 'TAGD' */
    kTagChunkI = 0x49474154, /* 'TAGI' */
    kTagChunkF = 0x46474154, /* 'TAGF' */
    kTagChunkN = 0x4e474154, /* 'TAGN' */
};

/* Tree node's record in the file: 0x28 bytes. */
typedef struct TagNodeRecord {
    uint8_t pad00[4];
    uint8_t kind; /* 0x04 */
    uint8_t pad05[3];
    uint32_t w08;
    uint32_t w0c;
    uint32_t w10;
    uint32_t w14;
    uint32_t w18;
    uint32_t w1c;
    uint32_t w20;
    uint32_t count; /* 0x24 (low 16 bits) */
} TagNodeRecord;

_Static_assert(sizeof(TagNodeRecord) == 0x28, "tag node record");

/* Triangle record in the file: 0x50 bytes. */
typedef struct TagFaceRecord {
    uint32_t size;  /* 0x00 must be 0x50 */
    uint32_t w[19]; /* 0x04, w[i] at 0x04 + 4 * i */
} TagFaceRecord;

_Static_assert(sizeof(TagFaceRecord) == 0x50, "tag face record");

/*
 * The original moves these floats with FLD / FSTP. The emulator (the reference for the port) leaves
 * signalling NaNs as they are, so a plain copy matches; real hardware would quiet them.
 */
static uint32_t fpuCopy(uint32_t bits)
{
    return bits;
}

BMX_ADDR(0x0048d6f0)
int ztaginfInitialize(void)
{
    gZtaginfRoot.kind = 1;
    gZtaginfUnused = 0;
    gZtaginfFaces = NULL;
    gZtaginfData = NULL;
    gZtaginfUnused2 = 0;
    gZtaginfRoot.child[1] = NULL;
    gZtaginfRoot.child[0] = NULL;
    gZtaginfRoot.faceCount = 0;
    gZtaginfNodes = zmemFreeListCreate(sizeof(ZtaginfNode), 0x100, NULL);
    return 1;
}

BMX_ADDR(0x0048d860)
void ztaginfFreeNode(ZtaginfNode *node)
{
    if (node->faces != NULL) {
        zmemFree((void *)node->faces);
        node->faces = NULL;
    }
    if (node->child[0] != NULL) {
        ztaginfFreeNode(node->child[0]);
        zmemFreeListFree(gZtaginfNodes, node->child[0]);
        node->child[0] = NULL;
    }
    if (node->child[1] != NULL) {
        ztaginfFreeNode(node->child[1]);
        zmemFreeListFree(gZtaginfNodes, node->child[1]);
        node->child[1] = NULL;
    }
}

/* The release sequence ztaginfRelease and the loader share. */
static void freeTables(void)
{
    if (gZtaginfRoot.faces != NULL) {
        zmemFree((void *)gZtaginfRoot.faces);
        gZtaginfRoot.faces = NULL;
    }
    if (gZtaginfRoot.child[0] != NULL) {
        ztaginfFreeNode(gZtaginfRoot.child[0]);
        zmemFreeListFree(gZtaginfNodes, gZtaginfRoot.child[0]);
        gZtaginfRoot.child[0] = NULL;
    }
    if (gZtaginfRoot.child[1] != NULL) {
        ztaginfFreeNode(gZtaginfRoot.child[1]);
        zmemFreeListFree(gZtaginfNodes, gZtaginfRoot.child[1]);
        gZtaginfRoot.child[1] = NULL;
    }
    if (gZtaginfFaces != NULL) {
        zmemFree(gZtaginfFaces);
        gZtaginfFaces = NULL;
        gZtaginfFaceCount = 0;
    }
    if (gZtaginfData != NULL) {
        zmemFree(gZtaginfData);
        gZtaginfData = NULL;
    }
}

BMX_ADDR(0x0048d8d0)
void ztaginfRelease(void)
{
    freeTables();
}

BMX_ADDR(0x0048d980)
struct ZbspLevelFace *ztaginfGetFaces(void)
{
    return gZtaginfFaces;
}

BMX_ADDR(0x0048d990)
uint32_t ztaginfGetFaceCount(void)
{
    return gZtaginfFaceCount;
}

BMX_ADDR(0x0048d9a0)
int ztaginfReadNode(ZtaginfNode *node, void *iff)
{
    TagNodeRecord rec;

    ziffRead(iff, &rec, sizeof rec);
    node->kind = rec.kind;
    node->f04 = rec.w18;
    node->f00 = rec.w08;
    node->f0c = rec.w20;
    node->f08 = rec.w1c;
    node->f18 = rec.w10;
    node->f14 = rec.w0c;
    node->faceCount = (uint16_t)rec.count;
    node->f1c = rec.w14;
    if (node->faceCount == 0) {
        node->faces = NULL;
    } else {
        const uint32_t bytes = (uint32_t)node->faceCount << 2;
        uint32_t *indices = zmemAlloc(bytes);

        ziffRead(iff, indices, bytes);
        node->faces = (struct ZbspLevelFace **)zmemAlloc((uint32_t)node->faceCount << 2);
        for (uint32_t i = 0; i < node->faceCount; i++) {
            node->faces[i] = (struct ZbspLevelFace *)((uint8_t *)gZtaginfFaces + indices[i] * kFaceStride);
        }
        zmemFree(indices);
    }
    if (node->kind == 1) {
        node->child[1] = NULL;
        node->child[0] = NULL;
        return 1;
    }
    node->child[0] = zmemFreeListAlloc(gZtaginfNodes);
    ziffNextChunk(iff, kTagChunkN);
    ztaginfReadNode(node->child[0], iff);
    node->child[1] = zmemFreeListAlloc(gZtaginfNodes);
    ziffNextChunk(iff, kTagChunkN);
    ztaginfReadNode(node->child[1], iff);
    return 1;
}

/* An offset into the file's data block, or 0 for "none" (-1). */
static uint32_t dataPointer(uint32_t offset)
{
    return offset == 0xffffffffu ? 0 : offset + (uint32_t)(uintptr_t)gZtaginfData;
}

BMX_ADDR(0x0048dae0)
int ztaginfLoad(void *iff)
{
    uint32_t header[2];
    TagFaceRecord rec;

    freeTables();
    if (ziffBeginModel(iff) == 0) {
        return 1;
    }
    if (ziffFindChunk(iff, kTagChunkD) == 0) {
        zdebugFail("Tag data not found", kZtaginfFile, 0x157, 0);
        return 0;
    }
    ziffRead(iff, header, sizeof header);
    if (header[0] != 8) {
        zdebugFail("Chunk sync error", kZtaginfFile, 0x15d, 0);
        return 0;
    }
    gZtaginfFaceCount = header[1];
    if (header[1] != 0) {
        if (ziffFindChunk(iff, kTagChunkI) != 0) {
            ziffRead(iff, header, sizeof header);
            const uint32_t size = ziffChunkSize(iff) - 8;
            if (size != 0) {
                gZtaginfData = zmemAlloc(size);
                ziffRead(iff, gZtaginfData, size);
            }
        }
        gZtaginfFaces = zmemAlloc(gZtaginfFaceCount * kFaceStride);
        if (ziffFindChunk(iff, kTagChunkF) != 0) {
            uint32_t offset = 0;
            do {
                ziffRead(iff, &rec, sizeof rec);
                if (rec.size != 0x50) {
                    zdebugFail("ztaginf : Tagging info face structure mismatch", kZtaginfFile, 0x17a, 0);
                }
                uint32_t *face = (uint32_t *)((uint8_t *)gZtaginfFaces + offset);
                face[0x30 / 4] = fpuCopy(rec.w[9]);
                face[0x34 / 4] = fpuCopy(rec.w[10]);
                face[0x38 / 4] = fpuCopy(rec.w[11]);
                face[0x00 / 4] = fpuCopy(rec.w[0]);
                face[0x04 / 4] = fpuCopy(rec.w[1]);
                face[0x08 / 4] = fpuCopy(rec.w[2]);
                face[0x10 / 4] = fpuCopy(rec.w[3]);
                face[0x14 / 4] = fpuCopy(rec.w[4]);
                face[0x18 / 4] = fpuCopy(rec.w[5]);
                face[0x20 / 4] = fpuCopy(rec.w[6]);
                face[0x24 / 4] = fpuCopy(rec.w[7]);
                face[0x28 / 4] = fpuCopy(rec.w[8]);
                face[0x44 / 4] = dataPointer(rec.w[13]);
                face[0x48 / 4] = dataPointer(rec.w[14]);
                face[0x4c / 4] = dataPointer(rec.w[15]);
                face[0x50 / 4] = dataPointer(rec.w[16]);
                face[0x54 / 4] = dataPointer(rec.w[17]);
                face[0x58 / 4] = dataPointer(rec.w[18]);
                face[0x40 / 4] = dataPointer(rec.w[12]);
                face[0x5c / 4] = 0;
                face[0x60 / 4] = 0;
                face[0x64 / 4] = 0;
                face[0x68 / 4] = 0;
                offset += kFaceStride;
            } while (ziffNextChunk(iff, kTagChunkF) != 0);
        }
    }
    if (ziffFindChunk(iff, kTagChunkN) != 0) {
        ztaginfReadNode(&gZtaginfRoot, iff);
        if (ziffNextChunk(iff, kTagChunkN) != 0) {
            zdebugFail("ztaginf : Extra nodes in tree???", kZtaginfFile, 0x1a0, 0);
        }
    }
    ziffEndModel(iff);
    return 1;
}
