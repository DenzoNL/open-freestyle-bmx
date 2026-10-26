/* Model loading and instancing (C:\NgEngine\Zge\Com\ZglRw\zrwModelCreate.c). */
#include "zge/com/zglrw/zrwmodelcreate.h"

#include <stddef.h>

#include "compat/crt.h"
#include "rw/rwcore.h"
#include "rw/rwplugins.h"
#include "zge/com/zglrw/zrwcol.h"
#include "zge/com/zglrw/zrwmat.h"
#include "zge/com/zglrw/zrwmodelrender.h"
#include "zge/com/zglrw/zrwprim.h"
#include "zge/com/zglrw/zrwtxr.h"
#include "zge/zasset.h"
#include "zge/zdebug.h"
#include "zge/zfile.h"
#include "zge/zgemods/zobjcamera.h"
#include "zge/zgemods/zobjpemit.h"
#include "zge/ziff.h"
#include "zge/zlist.h"
#include "zge/zmem.h"
#include "zge/zobj.h"
#include "zge/zquat.h"
#include "zge/zstring.h"

/* Declared by zrwzgl.h, which clashes with zrwmodel.h over the names of the scale globals. */
int zrwGetParameter(int id, void *out, const void *extra);
int zrwUnregisterUpdateFunction(struct ZrwUpdateNode *node);

static const char kModelCreateFile[] = "C:\\NgEngine\\Zge\\Com\\ZglRw\\zrwModelCreate.c";

/* Default distances of the four levels of detail. */
typedef float ZrwLodTable[4];
#define gZrwLodDefaults ORIG_VAR(0x00562cc4, ZrwLodTable, gZrwLodDefaults)

typedef void *(BMX_CDECL *RwFrameCallback)(void *frame, void *data);
typedef void *(BMX_CDECL *RpAtomicCallback)(void *atomic, void *data);

ORIG_FN(0x004d2b00, ZMatrix *, RwFrameGetMatrix, (void *frame));
ORIG_FN(0x004d2eb0, void *, RwFrameForAllChildren, (void *frame, RwFrameCallback callback, void *data));
ORIG_FN(0x004d2f00, int, RwFrameCount, (void *frame));
ORIG_FN(0x004d3200, void *, RwFrameSetIdentity, (void *frame));
ORIG_FN(0x004b1990, void *, RpClumpForAllAtomics, (void *clump, RpAtomicCallback callback, void *data));
ORIG_FN(0x004b1440, int, RpClumpGetNumAtomics, (void *clump));
ORIG_FN(0x004b1ee0, void *, RpClumpClone, (void *clump));
ORIG_FN(0x004b21e0, int, RpClumpDestroy, (void *clump));
ORIG_FN(0x004b2790, void *, RpClumpStreamRead, (void *stream));
ORIG_FN(0x004b4720, int, RpGeometryGetNumTriangles, (void *geometry));
/* The same function called with a stray second argument (see zrwAtomicCountGeometry). */
ORIG_FN(0x004b4720, int, RpGeometryGetNumTrianglesWithArg, (void *geometry, int vertices));
ORIG_FN(0x004b4550, int, RpGeometryGetNumVertices, (void *geometry));
ORIG_FN(0x004b4ce0, int, RpGeometryGetNumMaterials, (void *geometry));
ORIG_FN(0x004b47a0, void *, RpGeometryGetMaterial, (void *geometry, int index));
ORIG_FN(0x004b6140, uint32_t, RpGeometryGetFlags, (void *geometry));
ORIG_FN(0x004b93d0, void *, RpMaterialGetTexture, (void *material));
ORIG_FN(0x004e4420, void, RwImageSetPath, (const char *path));
ORIG_FN(0x004e23e0, void *, RwStreamOpen, (int type, int access, const void *data));
ORIG_FN(0x004e2ec0, int, RwStreamFindChunk,
        (void *stream, uint32_t type, uint32_t *length, uint32_t *version));
ORIG_FN(0x004e2660, int, RwStreamClose, (void *stream, void *data));
ORIG_FN(0x004b86a0, void *, RpWorldAddClump, (void *world, void *clump));
ORIG_FN(0x004b8770, void *, RpWorldRemoveClump, (void *world, void *clump));
ORIG_FN(0x004b8870, void *, RpClumpGetWorld, (void *clump));

/* The RenderWare memory interface: allocation at +0, free at +4. */
typedef void *(BMX_CDECL *RwAllocFn)(uint32_t size);
typedef void(BMX_CDECL *RwFreeFn)(void *block);

/* Memory descriptor of a stream: the data and its size. */
typedef struct RwMemory {
    void *start;
    uint32_t length;
} RwMemory;

/* Reference count of a model definition (+4). */
#define defRefs(def) (*(uint32_t *)((uint8_t *)(def) + 4))

static inline void *rwFrameOf(const void *clump)
{
    return *(void *const *)((const uint8_t *)clump + 4);
}

static inline ZrwPartSet *partSetOf(ZrwModel *model, uint32_t index)
{
    return (ZrwPartSet *)((uint8_t *)model + 0x54 + index * 12);
}

static inline float *lodOf(ZrwModel *model, uint32_t index)
{
    return (float *)((uint8_t *)model + 0x50 + index * 12);
}

/* ---- shadow sampler ----------------------------------------------------------------------------- */

/* TODO(file): 0x49b3d0-0x49b520 are the roller's shadow (probably ngifzroller.c); the data file puts them
 * here. */
BMX_ADDR(0x0049b3d0)
int zrollerShadowDraw(void)
{
    if (gZrwShadowReady != 0) {
        static const int8_t corner[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
        uint32_t count = 0;

        for (uint32_t i = 0; i < kZrwShadowCells; i++) {
            for (uint32_t j = 0; j < kZrwShadowCells; j++) {
                ZrwShadowQuad *quad = &gZrwShadowQuads[count];
                int complete = 1;

                quad->head = gZrwShadowHead;
                for (int c = 0; c < 4; c++) {
                    const uint32_t index = (uint32_t)(corner[c][1] + (int)j) +
                                           (uint32_t)(corner[c][0] + (int)i) * kZrwShadowGrid;
                    if (gZrwShadowMiss[index] != 0) {
                        complete = 0;
                        break;
                    }
                    quad->corner[c] = &gZrwShadowVertices[index];
                }
                if (complete) {
                    count++;
                }
            }
        }
        if (count != 0) {
            zrwprimDrawQuads(0x4103, (const ZrwPrimQuad *)&gZrwShadowQuads[0], (int)count, 0);
        }
        gZrwShadowReady = 0;
    }
    return 1;
}

BMX_ADDR(0x0049b4e0)
int zrollerShadowStop(void)
{
    if (gZrwShadowActive != 0) {
        if (gZrwShadowNode != NULL) {
            zrwUnregisterUpdateFunction(gZrwShadowNode);
        }
        gZrwShadowActive = 0;
    }
    return 1;
}

/*
 * Original quirk: the vertex position adds the hit point to a vector that is never set (a stack slot
 * right behind the hit record); it is taken as zero here.
 */
BMX_ADDR(0x0049b520)
void zrollerUpdateShadow(void *state)
{
    ZrwShadow *shadow = state;
    ZVec3 position;
    ZVec3 eye;
    ZVec3 zero;
    ZVec3 unset = {0.0f, 0.0f, 0.0f};
    ZMatrix look;
    ZMatrix frame;

    if (gZrwShadowInit == 0) {
        float world;
        zrwGetParameter(1, &world, NULL);
        const double doubled = (double)world + (double)world;
        const float a = (float)(doubled * (double)0.5f);
        const float b = (float)(doubled * (double)0.5f);
        const float c = (float)(doubled * (double)0.2f);
        const double step = doubled * (double)0.2f;

        for (uint32_t i = 0; i < kZrwShadowGrid; i++) {
            for (uint32_t j = 0; j < kZrwShadowGrid; j++) {
                ZrwShadowPoint *p = &gZrwShadowOffsets[i * kZrwShadowGrid + j];
                p->pos.x = (float)((double)i * (double)c - (double)a);
                p->pos.z = 0.0f;
                p->pos.y = (float)(((double)j * step - (double)b) * (double)-1.0f);
            }
        }
        gZrwShadowScale = (float)((double)world * (double)0.1f);
        gZrwShadowInit = 1;
        gZrwShadowDouble = (float)doubled;
    }

    position = *zobjGetPosition((const void *)(uintptr_t)shadow->model->userValue);
    zvec3Add(&eye, &position, &shadow->offset);
    zobjLookAtMatrix(&look, &eye, &position);
    zmatrixInvertRigid(&look, &look);
    zero.x = 0.0f;
    zero.y = 0.0f;
    zero.z = 0.0f;
    zmatrixIdentity(&frame);
    frame.pos = position;
    zmatrixCopyBasis(&frame, &look);
    zmatrixTransformVector(&zero, &frame, &zero);
    zmatrixTranslate(&frame, &frame, &zero);

    for (uint32_t i = 0; i < kZrwShadowGrid; i++) {
        for (uint32_t j = 0; j < kZrwShadowGrid; j++) {
            const uint32_t k = i * kZrwShadowGrid + j;
            ZVec3 probe = eye;
            ZVec3 end;
            ZrwColLineHit hit;

            gZrwShadowMiss[k] = 1;
            zmatrixTransformPoint(&gZrwShadowPoints[k].pos, &frame, &gZrwShadowOffsets[k].pos);
            zvec3Lerp(&end, &probe, &gZrwShadowPoints[k].pos, 10.0f);
            if (zrwcolQueryLine(&probe.x, &end.x, 1, NULL, &hit) != 0) {
                ZrwShadowVertex *vertex = &gZrwShadowVertices[k];
                double alpha;

                zvec3Scale((ZVec3 *)hit.hit.normal, (ZVec3 *)hit.hit.normal, gZrwShadowScale);
                zvec3Add(&vertex->pos, &unset, (ZVec3 *)hit.hit.normal);
                vertex->u = (float)((double)i * (double)0.2f);
                vertex->v = (float)((double)(5 - j) * (double)0.2f);
                alpha = (double)probe.y - (double)unset.y;
                alpha = alpha + (double)1450.0f;
                alpha = alpha * (double)0.001f;
                if (!(alpha >= (double)0.0f)) {
                    alpha = (double)0.0f;
                }
                alpha = alpha * (double)128.0f;
                const uint8_t level = (uint8_t)bmxFtol(alpha);
                vertex->color[0] = level;
                vertex->color[1] = level;
                vertex->color[2] = level;
                vertex->color[3] = level;
                gZrwShadowMiss[k] = 0;
            }
        }
    }
    gZrwShadowReady = 1;
}

/* ---- decompression ------------------------------------------------------------------------------ */

#ifdef BMX_HOOK
#define kHashDummy ((const uint8_t *)(uintptr_t)0x00562c04)
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

BMX_ADDR(0x0049c570)
void zobjDispatchEvents(ZObj *object)
{
    if (object->events == NULL || object->events->pending == NULL) {
        return;
    }
    ZObjEvent *event = zlistFirst(object->events->pending);

    while (event != NULL) {
        if (object->events->handlers != NULL) {
            ZObjHandler *handler = zlistFirst(object->events->handlers);

            while (handler != NULL) {
                if (handler->eventId == event->id && handler->kind == 1) {
                    handler->fn(event);
                }
                handler = zlistNext(object->events->handlers);
            }
        }
        zlistRemoveCurrent(object->events->pending);
        event = zlistCurrent(object->events->pending);
    }
}

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

BMX_ADDR(0x0049c880)
void zrwModelSetRwInfo(ZrwModel *model)
{
    void *geometry;

    zrwMdlSetUserData(model->clump, (uint32_t)(uintptr_t)model);
    if (model->clump2 != NULL) {
        zrwMdlSetUserData(model->clump2, (uint32_t)(uintptr_t)model);
    }
    geometry = *(void **)((uint8_t *)model->def + 0xb0);
    for (void *material = zlistFirst(model->materials); material != NULL;
         material = zlistNext(model->materials)) {
        void *texture = zrwmatGetTexture(material, 0);
        void *rwTexture = zrwTextureGetRwTexture(texture);
        const char *name = zrwTextureGetName(texture);
        int index = RpGeometryGetNumMaterials(geometry) - 1;
        int matched = 0;

        for (; index >= 0; index--) {
            void *rpMaterial = RpGeometryGetMaterial(geometry, index);

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
    void *geometry = *(void **)((uint8_t *)atomic + 0x1c);

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
    void *first = NULL;
    int counts[2];
    void *geometry;

    RpClumpForAllAtomics(clump, BMX_FNPTR(RpAtomicCallback, 0x0049cc60, zrwFrameCollectFirst),
                         (void *)&first);
    geometry = *(void **)((uint8_t *)first + 0x1c);
    counts[0] = 0;
    counts[1] = 0;
    gZdebugPrint("Num atomics:%d (", RpClumpGetNumAtomics(clump));
    RpClumpForAllAtomics(clump, BMX_FNPTR(RpAtomicCallback, 0x0049c970, zrwAtomicCountGeometry), counts);
    gZdebugPrint(" )\n");
    gZdebugPrint("Num triangles:%d\n", counts[0]);
    gZdebugPrint("Num vertices :%d\n", counts[1]);
    gZdebugPrint("Flags (Atomic0):");
    if ((RpGeometryGetFlags(geometry) & 4) != 0) {
        gZdebugPrint(" rpGEOMETRYTEXTURED");
    }
    if ((RpGeometryGetFlags(geometry) & 8) != 0) {
        gZdebugPrint(" rpGEOMETRYPRELIT");
    }
    if ((RpGeometryGetFlags(geometry) & 0x10) != 0) {
        gZdebugPrint(" rpGEOMETRYNORMALS");
    }
    if ((RpGeometryGetFlags(geometry) & 0x20) != 0) {
        gZdebugPrint(" rpGEOMETRYLIGHT");
    }
    if ((RpGeometryGetFlags(geometry) & 1) != 0) {
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
        gZdebugPrint("0x%x (%s)\n", key, (const char *)entry);
    }
    gZdebugIndent(1);
    RwFrameForAllChildren(frame, BMX_FNPTR(RwFrameCallback, 0x0049caf0, zrwFrameDumpHierarchy),
                          (void *)model);
    gZdebugIndent(-1);
    return frame;
}

BMX_ADDR(0x0049cb60)
void zrwModelDumpHierarchy(const ZrwModel *model)
{
    void *frame = rwFrameOf(model->clump);
    uint32_t key;
    const ZrwModelEntry *entry;

    gZdebugPrint("HIERARCHY:-\n");
    gZdebugIndent(1);
    key = RwFrameGetUserData(frame);
    entry = zrwModelFindEntry(model, (int)key);
    if (entry == NULL) {
        gZdebugPrint("0x%x\n", key);
    } else {
        gZdebugPrint("0x%x (%s)\n", key, (const char *)entry);
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

BMX_ADDR(0x0049cce0)
void *zrwFrameBuildPartNode(void *frame, void *parentNode)
{
    ZrwPartNode *node = gZrwModelCreate.cursor;
    ZrwPartNode *parent = parentNode;
    ZMatrix *matrix;

    gZrwModelCreate.cursor = node + 1;
    matrix = RwFrameGetMatrix(frame);
    node->hash = RwFrameGetUserData(frame);
    node->def = NULL;
    node->frame = frame;
    node->pos[0] = matrix->pos.x;
    node->pos[1] = matrix->pos.y;
    node->pos[2] = matrix->pos.z;
    node->parent = parent;
    if (parent != NULL) {
        node->sibling = parent->child;
        parent->child = node;
    }
    RwFrameForAllChildren(frame, BMX_FNPTR(RwFrameCallback, 0x0049cce0, zrwFrameBuildPartNode), node);
    return frame;
}

/* Allocates the part nodes of a frame hierarchy, fills them and returns the root. */
static ZrwPartNode *buildPartTree(void *frame, int *count)
{
    ZrwPartNode *root;
    ZMatrix *matrix;
    const int n = RwFrameCount(frame);

    *count = n;
    root = zmemFill(zmemAlloc((uint32_t)n * 0x28), 0, (uint32_t)n * 0x28);
    gZrwModelCreate.cursor = root + 1;
    matrix = RwFrameGetMatrix(frame);
    root->hash = RwFrameGetUserData(frame);
    root->def = NULL;
    root->frame = frame;
    root->pos[0] = matrix->pos.x;
    root->pos[1] = matrix->pos.y;
    root->pos[2] = matrix->pos.z;
    root->parent = NULL;
    RwFrameForAllChildren(frame, BMX_FNPTR(RwFrameCallback, 0x0049cce0, zrwFrameBuildPartNode), root);
    return root;
}

BMX_ADDR(0x0049ce90)
ZrwModel *zrwModelCreateInstance(const ZrwModel *model)
{
    ZrwModel *instance;
    void *frame;
    ZMatrix *matrix;
    uint32_t data;
    void *cloneFrame;

    if ((model->def->flags2 & 2) != 0) {
        gZdebugPrint("zrwModelCreateInstance: Not instancing skinned model - ReLoading\n");
        return zrwModelLoadFromFile(&model->def->name[0x10]);
    }
    instance = zmemFreeListAlloc(gZrwModelCreate.pool);
    zmemCopy(instance, model, 0x8c);
    defRefs(instance->def)++;
    frame = rwFrameOf(model->clump);
    matrix = RwFrameGetMatrix(frame);
    data = RwFrameGetUserData(frame);
    matrix->at.x = (float)(data & 0xffff);
    matrix->at.y = (float)(data >> 16);
    RwFrameForAllChildren(frame, BMX_FNPTR(RwFrameCallback, 0x0049c6e0, zrwFrameUserDataToMatrix), NULL);
    instance->clump = RpClumpClone(model->clump);
    cloneFrame = rwFrameOf(instance->clump);
    matrix = RwFrameGetMatrix(cloneFrame);
    {
        const uint16_t high = (uint16_t)bmxFtol((double)matrix->at.y);
        const uint32_t low = (uint32_t)bmxFtol((double)matrix->at.x);
        RwFrameSetUserData(cloneFrame, ((uint32_t)high << 16) | (low & 0xffff));
    }
    RwFrameForAllChildren(cloneFrame, BMX_FNPTR(RwFrameCallback, 0x0049c7a0, zrwFrameMatrixToUserData), NULL);
    RwFrameSetIdentity(rwFrameOf(instance->clump));
    RwFrameSetIdentity(rwFrameOf(model->clump));
    if (model->clump2 != NULL) {
        instance->clump2 = RpClumpClone(model->clump2);
    }
    zrwModelSetRwInfo(instance);
    for (uint32_t i = 0; i < instance->def->partSetCount; i++) {
        int count;
        ZrwPartSet *set = partSetOf(instance, i);

        set->parts = (ZrwModelPart *)buildPartTree(rwFrameOf(instance->clump), &count);
        set->count = (uint32_t)count;
    }
    instance->materials = zlistCreate(0x10);
    for (void *material = zlistFirst(model->materials); material != NULL;
         material = zlistNext(model->materials)) {
        zlistInsert(instance->materials, material);
        zrwmatRetainMaterial(material);
    }
    zrwModelSetInWorld(instance, 1);
    if ((instance->def->flags2 & 0x40) == 0) {
        zrwModelSetActive(instance, 1);
    }
    return instance;
}

BMX_ADDR(0x0049d120)
int zrwModelAddClump(ZrwModel *model, const uint32_t *memory, const char *imagePath, int collision)
{
    void *clump = NULL;
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
    ((RwFreeFn)(uintptr_t)RwOsGetMemoryInterface()->rwfree)((void *)(uintptr_t)memory[0]);
    if (clump == NULL) {
        return 0;
    }
    if (collision == 1) {
        model->clump2 = clump;
        return 1;
    }
    {
        ZrwModelDef *def = model->def;
        void *frame;
        int count;
        ZrwPartSet *set;
        uint32_t level;

        gZrwModelCreate.lodClumps[def->partSetCount] = clump;
        if (def->partSetCount == 0) {
            model->clump = clump;
            RwFrameSetIdentity(rwFrameOf(clump));
        }
        if (model->def->partSetCount == 0) {
            void *first = NULL;

            RpClumpForAllAtomics(clump, BMX_FNPTR(RpAtomicCallback, 0x0049cc60, zrwFrameCollectFirst),
                                 (void *)&first);
            model->def->atomic = first;
            model->def->atomicInfo = *(void **)((uint8_t *)model->def->atomic + 0x4c);
            model->def->geometry = *(void **)((uint8_t *)model->def->atomic + 0x1c);
        }
        level = model->def->partSetCount;
        frame = rwFrameOf(model->clump);
        count = RwFrameCount(frame);
        partSetOf(model, level)->count = (uint32_t)count;
        {
            ZrwPartNode *root;
            ZMatrix *matrix;

            root = zmemFill(zmemAlloc((uint32_t)count * 0x28), 0, (uint32_t)count * 0x28);
            gZrwModelCreate.cursor = root + 1;
            matrix = RwFrameGetMatrix(frame);
            root->hash = RwFrameGetUserData(frame);
            root->def = NULL;
            root->frame = frame;
            root->pos[0] = matrix->pos.x;
            root->pos[1] = matrix->pos.y;
            root->pos[2] = matrix->pos.z;
            root->parent = NULL;
            RwFrameForAllChildren(frame, BMX_FNPTR(RwFrameCallback, 0x0049cce0, zrwFrameBuildPartNode), root);
            set = partSetOf(model, model->def->partSetCount);
            set->parts = (ZrwModelPart *)root;
        }
        model->def->partSetCount++;
        if ((model->def->flags2 & 8) != 0) {
            zrwClumpMaskGeometryFlags(clump, 0x20, 0);
        }
        if ((model->def->flags2 & 0xc0) != 0) {
            zrwClumpMaskAtomicFlags(clump, 1, 0);
        }
        if ((model->def->flags2 & 0xc0) == 0) {
            zrwClumpMaskAtomicFlags(clump, 0, 1);
        }
        zrwClumpDump(clump);
    }
    return 1;
}

/* Reads the MDFF clump chunk the reader is on into memory and loads it into the model. */
static int loadClumpChunk(void *iff, ZrwModel *model, int collision, int line)
{
    uint32_t memory[2];
    const uint32_t size = ziffChunkSize(iff);

    memory[0] = (uint32_t)(uintptr_t)((RwAllocFn)(uintptr_t)RwOsGetMemoryInterface()->rwmalloc)(size);
    memory[1] = size;
    ziffRead(iff, (void *)(uintptr_t)memory[0], size);
    if (zrwModelAddClump(model, memory, NULL, collision) == 0) {
        zdebugFail("Clump failed to load", kModelCreateFile, line, 0);
        gZdebugPrint("Clump failed to load\n");
        return 0;
    }
    return 1;
}

BMX_ADDR(0x0049d350)
int zrwModelLoadLod(void *iff, ZrwModel *model)
{
    if (model->def->partSetCount == 4) {
        gZdebugPrint("Ignoring extra LOD in %s\n", model->def->name);
        return 0;
    }
    if (ziffBeginModel(iff) != 0) {
        if (ziffFindChunk(iff, 0x46444c4d) != 0) {
            uint32_t header[2];
            uint32_t level;

            ziffRead(iff, header, 8);
            *lodOf(model, model->def->partSetCount) = *(const float *)&header[1];
            level = model->def->partSetCount;
            /* Zero or NaN (what the x87 compare against 0 reports as equal). */
            if (!(*lodOf(model, level) < 0.0f || *lodOf(model, level) > 0.0f)) {
                *lodOf(model, level) = gZrwLodDefaults[level];
            }
        }
        if (ziffFindChunk(iff, 0x4646444d) != 0) {
            loadClumpChunk(iff, model, 0, 0x3fc);
        }
        ziffEndModel(iff);
    }
    return 1;
}

BMX_ADDR(0x0049d4d0)
int zrwModelLoadCollision(void *iff, ZrwModel *model)
{
    if (ziffBeginModel(iff) != 0) {
        if (ziffFindChunk(iff, 0x4646444d) != 0) {
            loadClumpChunk(iff, model, 1, 0x441);
            zrwClumpMaskAtomicFlags(model->clump2, 4, 1);
            RpWorldAddClump(gZrwWorld, model->clump2);
        }
        ziffEndModel(iff);
    }
    return 1;
}

BMX_ADDR(0x0049d5f0)
int zrwModelReadDef(void *iff, ZrwModel *model)
{
    uint32_t block[11];

    if (ziffFindChunk(iff, 0x4645444d) == 0) {
        zdebugFail("Missing model def block", kModelCreateFile, 0x495, 0);
        return 0;
    }
    ziffRead(iff, block, 0x2c);
    zstrncpy(model->def->name, (const char *)&block[1], 0x10);
    model->def->defaultKey = block[5];
    model->def->flags2 = block[0];
    model->def->entryCount = (uint16_t)block[10];
    model->def->entries = zmemAlloc((block[10] & 0xffff) * 0x14);
    for (int i = 0; i < model->def->entryCount; i++) {
        uint8_t *entry = (uint8_t *)model->def->entries + i * 0x14;

        ziffRead(iff, entry, 0x10);
        ziffReadWordA(iff, entry + 0x10);
        entry[0xf] = 0;
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
    int found = ziffFindChunk(iff, 0x4954414d);

    while (found != 0) {
        if (ziffDispatchChunk(iff, model) == 0) {
            zrwmatSetMaterialHook(previous);
            return 0;
        }
        found = ziffNextChunk(iff, 0x4954414d);
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
    memset(model, 0, 0x8c);
    *(uint32_t *)model &= 0xffffff00u;
    model->def = zmemAlloc(0xbc);
    memset(model->def, 0, 0xbc);
    defRefs(model->def) = 1;
    model->materials = zlistCreate(0x10);
    zquatIdentity(&model->rotation);
    model->scale.x = 1.0f;
    model->scale.y = 1.0f;
    model->scale.z = 1.0f;
    model->onUpdateArg = 0xffffffffu;
    if (ziffBeginModel(iff) == 0) {
        return 0;
    }
    return zrwModelLoadTail(iff, model);
}

BMX_ADDR(0x0049d895)
int zrwModelLoadTail(void *iff, ZrwModel *model)
{
    zrwModelReadDef(iff, model);
    zrwModelLoadMaterials(iff, model);
    {
        int found = ziffFindChunk(iff, 0x444f4c4d);

        while (found != 0 && ziffDispatchChunk(iff, model) != 0) {
            found = ziffNextChunk(iff, 0x444f4c4d);
        }
    }
    if ((model->def->flags2 & 0x80) != 0 && ziffFindChunk(iff, 0x4c4f434d) != 0) {
        ziffDispatchChunk(iff, model);
    }
    if (ziffEndModel(iff) == 0) {
        return 0;
    }
    if ((model->def->flags2 & 2) != 0) {
        RpSkinClumpSetup(model->clump);
    }
    zrwModelSetDefaultRenderCallback(model);
    if ((model->def->flags2 & 0x10) != 0) {
        zrwModelRenderRegister(model);
    } else if ((model->def->flags2 & 4) == 0) {
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
    gZrwModelCreate.pool =
        zmemFreeListCreate(0x8c, 0x100, BMX_FNPTR(ZMemDestroyFn, 0x0049dcd0, (ZMemDestroyFn)zrwModelDestroy));
    gZrwModelCreate.initialized = 1;
    ziffSetupCallback(0x4c444f4d, BMX_FNPTR(int(BMX_CDECL *)(void *), 0x0049d7e0, zrwModelLoad), 0);
    ziffSetupCallback(
        0x444f4c4d,
        BMX_FNPTR(int(BMX_CDECL *)(void *), 0x0049d350, (int(BMX_CDECL *)(void *))zrwModelLoadLod), 0);
    ziffSetupCallback(
        0x4c4f434d,
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

BMX_ADDR(0x0049dab0)
ZrwModel *zrwModelLoadFromBuffer(const uint32_t *buffer)
{
    const uint32_t size = buffer[1];
    ZfileHandle *file;
    void *iff;
    const void *result = buffer;

    if (buffer[0] != 0x4c444f4d) {
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
    if (ziffFindChunk(iff, 0x4c444f4d) != 0) {
        ziffDispatchChunk(iff, NULL);
        result = gZrwModelCreate.last;
    }
    ziffClose(iff);
    zfileClose(file, 0);
    return (ZrwModel *)(uintptr_t)result;
}

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
    if (buffer[0] == 0x4c444f4d) {
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
                if (ziffFindChunk(iff, 0x4c444f4d) != 0) {
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
        zstrncpy(&((const ZrwModel *)result)->def->name[0x10], path, 0x80);
    }
    return (ZrwModel *)(uintptr_t)result;
}

BMX_ADDR(0x0049dcb0)
int zrwModelNotCoded(void)
{
    zdebugFail("Not coded", kModelCreateFile, 0x5ba, 0);
    return 0;
}

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
    if (model->clump2 != NULL) {
        if (RpClumpGetWorld(model->clump2) != NULL) {
            RpWorldRemoveClump(gZrwWorld, model->clump2);
        }
        RpClumpDestroy(model->clump2);
    }
    if ((model->def->flags2 & 0x10) != 0) {
        zrwModelRenderUnregister();
    }
    references = defRefs(model->def);
    for (void *material = zlistFirst(model->materials); material != NULL;
         material = zlistFirst(model->materials)) {
        if (references == 1) {
            zrwTextureRelease(zrwmatGetTexture(material, 0));
        }
        zrwmatReleaseTexture(material);
        zlistRemoveCurrent(model->materials);
    }
    zlistDestroy(model->materials);
    for (uint32_t i = 0; i < model->def->partSetCount; i++) {
        zmemFree(partSetOf(model, i)->parts);
    }
    defRefs(model->def)--;
    if (defRefs(model->def) == 0) {
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

BMX_ADDR(0x0049de60)
void zrwModelDumpInfo(const ZrwModel *model)
{
    void *clump = model->clump;
    void *frame;
    uint32_t key;
    const ZrwModelEntry *entry;

    gZdebugPrint("-------------------------------------------------\n");
    gZdebugPrint("Info for model:%s\n", model->def->name);
    gZdebugPrint("-------------------------------------------------\n");
    gZdebugIndent(1);
    zrwClumpDump(clump);
    frame = rwFrameOf(model->clump);
    gZdebugPrint("HIERARCHY:-\n");
    gZdebugIndent(1);
    key = RwFrameGetUserData(frame);
    entry = zrwModelFindEntry(model, (int)key);
    if (entry == NULL) {
        gZdebugPrint("0x%x\n", key);
    } else {
        gZdebugPrint("0x%x (%s)\n", key, (const char *)entry);
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
