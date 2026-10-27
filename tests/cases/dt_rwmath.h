/*
 * Shared fixtures of the RenderWare maths, frame, camera and charset tests (rw/math.c, rw/camera.c,
 * rw/charset.c): an engine instance with the matrix and vector module data and logging free list stubs,
 * matrix and frame tree builders.
 *
 * Everything the stubs record goes through dt_canon() and is compared between the original and the port.
 */
#ifndef DT_RWMATH_H
#define DT_RWMATH_H

#include "dt_rw.h"
#include "rw/rwcamera.h"
#include "rw/rwmath.h"

enum {
    kMathMatrixOffset = kRwtMatrixOffset,      /* RwMatrixModule, 0x10 bytes */
    kMathV3dOffset = kRwtSqrtOffset,           /* RwV3dModule */
    kMathFrameOffset = kRwtSqrtOffset + 0x10,  /* the frame free list pointer */
    kMathCameraOffset = kRwtSqrtOffset + 0x14, /* the camera free list pointer */
    kMathEngineBytes = kRwtSqrtOffset + 0x20,
};

static int gMathAllocCalls, gMathAllocFailAt;

/* The free list entry points of the engine: blocks of 0xb0 bytes (a frame is the largest object), zeroed. */
RWT_FN void *BMX_CDECL mathAlloc(RwFreeList *list)
{
    rwtLog(0x4dbc30);
    rwtLogPtr(list);
    gMathAllocCalls++;
    if (gMathAllocFailAt != 0 && gMathAllocCalls == gMathAllocFailAt) {
        return NULL;
    }
    return dt_alloc(0xb0);
}

RWT_FN void BMX_CDECL mathFree(RwFreeList *list, void *entry)
{
    rwtLog(0x4dbd90);
    rwtLogPtr(list);
    rwtLogPtr(entry);
}

/* The matrix product behind the module's function pointer: a deterministic mix of both operands. */
RWT_FN void BMX_CDECL mathMultiply(RwMatrix *out, const RwMatrix *a, const RwMatrix *b)
{
    rwtLog(0x4d0500);
    rwtLogFloatBits(a->right.x);
    rwtLogFloatBits(a->pos.z);
    rwtLogFloatBits(b->right.x);
    rwtLogFloatBits(b->pos.z);
    for (int i = 0; i < 16; i++) {
        if (i % 4 == 3) {
            ((uint32_t *)out)[i] = ((const uint32_t *)a)[i] ^ (((const uint32_t *)b)[i] << 1);
        } else {
            ((float *)out)[i] = ((const float *)a)[i] * 0.5f + ((const float *)b)[i];
        }
    }
}

/* The vector transforms behind the V3d module's function pointers. */
RWT_FN RwV3d *BMX_CDECL mathTransformPoints(RwV3d *out, const RwV3d *in, int32_t count,
                                            const RwMatrix *matrix)
{
    rwtLog(0x4d3a00);
    rwtLogPtr(out);
    rwtLogPtr(in);
    rwtLog((uint32_t)count);
    rwtLogPtr(matrix);
    return out;
}

RWT_FN RwV3d *BMX_CDECL mathTransformVectors(RwV3d *out, const RwV3d *in, int32_t count,
                                             const RwMatrix *matrix)
{
    rwtLog(0x4d3a01);
    rwtLogPtr(out);
    rwtLogPtr(in);
    rwtLog((uint32_t)count);
    rwtLogPtr(matrix);
    return (RwV3d *)(uintptr_t)(dt_u32() | 1u);
}

/*
 * A fresh engine instance for the maths tests: free lists that log, the matrix module with the identity bit
 * and the multiply stub, the V3d module with a random reciprocal square root table and the transform stubs,
 * an empty dirty frame list, and a random device near/far z.
 */
RWT_FN RwGlobals *mathEnv(void)
{
    RwGlobals *small = rwtEnv();
    RwGlobals *g = dt_alloc(kMathEngineBytes);
    __builtin_memcpy(g, small, kRwtEngineBytes);
    RwEngineInstance = g;
    g->freeListAlloc = mathAlloc;
    g->freeListFree = mathFree;
    g->dirtyFrameList.next = g->dirtyFrameList.prev = &g->dirtyFrameList;
    gMathAllocCalls = gMathAllocFailAt = 0;
    rwFrameOffset = kMathFrameOffset;
    rwCameraOffset = kMathCameraOffset;
    RwMatrixModule *matrixModule = (RwMatrixModule *)((uint8_t *)g + kMathMatrixOffset);
    matrixModule->freeList = dt_alloc(0x30);
    matrixModule->flagMask = dt_u32() % 8 == 0 ? dt_u32() : 0x20000;
    matrixModule->multiply = (void *)mathMultiply;
    RwV3dModule *v3d = (RwV3dModule *)((uint8_t *)g + kMathV3dOffset);
    uint32_t *table = dt_alloc(4096 * 4);
    dt_fill(table, 4096 * 4);
    v3d->sqrtTable = table;
    uint32_t *invTable = dt_alloc(4096 * 4);
    dt_fill(invTable, 4096 * 4);
    v3d->invSqrtTable = invTable;
    v3d->transformPoints = mathTransformPoints;
    v3d->transformVectors = mathTransformVectors;
    *(RwFreeList **)((uint8_t *)g + kMathFrameOffset) = dt_alloc(0x30);
    *(RwFreeList **)((uint8_t *)g + kMathCameraOffset) = dt_alloc(0x30);
    g->dOpenDevice.zBufferNear = dt_float_edgy(-2.0f, 2.0f);
    g->dOpenDevice.zBufferFar = dt_float_edgy(-2.0f, 2.0f);
    return g;
}

RWT_FN float mathFloat(void)
{
    static const uint32_t bits[] = {0x00000000, 0x80000000, 0x00000001, 0x7f7fffff, 0x7f800000,
                                    0xff800000, 0x7fc00000, 0x3f800000, 0xbf800000, 0x5f000000};
    if (dt_u32() % 9 == 0) {
        float f;
        const uint32_t b = bits[dt_u32() % (sizeof bits / sizeof bits[0])];
        __builtin_memcpy(&f, &b, 4);
        return f;
    }
    return dt_float_edgy(-50.0f, 50.0f);
}

/* A matrix with random elements, flags (identity / orthonormal bits among them) and version byte. */
RWT_FN void mathFillMatrix(RwMatrix *m)
{
    float *f = (float *)m;
    for (int i = 0; i < 16; i++) {
        f[i] = mathFloat();
    }
    m->flags = dt_u32() % 3 == 0 ? dt_u32() : (dt_u32() & 0x20003u);
    dt_fill(&m->pad1, 4);
    dt_fill(&m->pad2, 4);
    dt_fill(&m->pad3, 4);
}

/*
 * A forest of up to maxN frames with consistent child / next / root links, random matrices, versions and
 * flags. Roots whose flags say "queued" are in the engine's dirty list. Returns the frame count.
 */
RWT_FN int mathFrameTree(RwFrame **frames, int maxN)
{
    const int n = dt_range(1, maxN);
    RwGlobals *g = RwEngineInstance;
    for (int i = 0; i < n; i++) {
        RwFrame *f = dt_alloc(sizeof(RwFrame) + 0x10);
        frames[i] = f;
        mathFillMatrix(&f->modelling);
        mathFillMatrix(&f->ltm);
        f->version = (uint8_t)dt_u32();
        f->objectList.next = f->objectList.prev = &f->objectList;
        const int parent = (i == 0 || dt_u32() % 5 == 0) ? -1 : dt_range(0, i - 1);
        f->root = f;
        f->object.privateFlags = (uint8_t)(dt_u32() & 0xf);
        if (parent >= 0) {
            RwFrame *p = frames[parent];
            f->object.parent = p;
            f->root = p->root;
            f->next = p->child;
            p->child = f;
            f->object.privateFlags &= 0xc; /* only roots are queued */
        } else if ((f->object.privateFlags & 3) != 0) {
            f->dirtyLink.next = g->dirtyFrameList.next;
            f->dirtyLink.prev = &g->dirtyFrameList;
            g->dirtyFrameList.next->prev = &f->dirtyLink;
            g->dirtyFrameList.next = &f->dirtyLink;
        }
    }
    return n;
}

/* The frame registry: random plugins whose callbacks log (constructors fail on request). */
RWT_FN void mathFrameRegistry(void)
{
    RwPluginRegistry *r = rwtRegistry(dt_range(0, 2), 0x120);
    __builtin_memcpy(&rwFrameRegistry, r, sizeof *r);
}

#endif
