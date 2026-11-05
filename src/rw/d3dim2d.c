/*
 * RenderWare 3.1's Direct3D 7 driver, Im2D side (lib/d3d7driver): the device's Im2D line, triangle,
 * primitive and indexed primitive (TL vertices, D3DFVF 0x1c4). With rwRENDERSTATEALPHAPRIMITIVEBUFFER on and
 * z test on, translucent primitives are not drawn at once but copied with the current state into a deferred
 * list; a flush (z test off, the buffer state off, the camera's end update) sorts their triangles and lines
 * by screen z and draws them back to front.
 */
#include "rw/rwd3d.h"

ORIG_FN(0x004ded80, int32_t, rwD3DError, (int32_t code, ...));
ORIG_FN(0x004a4aa7, void, rwD3DQsort, (void *base, uint32_t n, uint32_t size, void *compare));

enum {
    kFvfTL = 0x1c4,
    kDrawFlags = 5, /* D3DDP_DONOTUPDATEEXTENTS | D3DDP_WAIT */
    kErrNoMemory = (int32_t)0x80000002,
    kPtLineList = 2,
    kPtLineStrip = 3,
    kPtTriangleList = 4,
    kPtTriangleStrip = 5,
    kPtTriangleFan = 6,
};

typedef void(BMX_CDECL *RwD3DDeferFunc)(uint8_t *record);

/* The size of a deferred record's callback slot (4 bytes where pointers are 32 bits wide). */
#define kDeferSlot ((uint32_t)sizeof(void *))

/* Set when a flip waits for the blitter (cleared by the next camera begin update). */
#define rwD3DFlipPending ORIG_VAR(0x00676ba4, int32_t, rwD3DFlipPending)

static void noMemory(uint32_t size)
{
    RwError error;
    error.pluginID = 1;
    error.errorCode = rwD3DError(kErrNoMemory, size);
    RwErrorSet(&error);
}

/* ---- the deferred list ----------------------------------------------------------------------------------
 */

/* 0x4ee700: an empty list (from the device's memory functions). */
BMX_ADDR(0x004ee700)
RwD3DDeferList *rwD3DDeferCreate(void)
{
    RwD3DDeferList *list = rwD3DMemory->rwmalloc(sizeof *list);

    if (list == NULL) {
        noMemory(sizeof *list);
        return NULL;
    }
    list->maxUsed = 0;
    list->frames = 0;
    list->count = 0;
    list->offsetCap = 0;
    list->offsets = NULL;
    list->used = 0;
    list->arenaSize = 0;
    list->arena = NULL;
    return list;
}

/* 0x4ee7e0: room for a record of `size` bytes behind its callback word; NULL on no memory. */
BMX_ADDR(0x004ee7e0)
void *rwD3DDeferAlloc(RwD3DDeferList *list, uint32_t size, void *callback)
{
    /* The callback in a pointer-sized slot, the record after it; both kept pointer-aligned. */
    const uint32_t need = (size + kDeferSlot + kDeferSlot - 1) & ~(kDeferSlot - 1);
    const uint32_t total = need + list->used;
    uint32_t used;

    if (list->arenaSize < total) {
        uint8_t *arena =
            list->arena == NULL ? rwD3DMemory->rwmalloc(total) : rwD3DMemory->rwrealloc(list->arena, total);
        if (arena == NULL) {
            noMemory(total);
            return NULL;
        }
        list->arena = arena;
        list->arenaSize = list->used + need;
    }
    if (list->offsetCap <= list->count) {
        const uint32_t bytes = list->offsetCap * 4 + 0x14;
        uint32_t *offsets = list->offsets == NULL ? rwD3DMemory->rwmalloc(bytes)
                                                  : rwD3DMemory->rwrealloc(list->offsets, bytes);
        if (offsets == NULL) {
            noMemory(bytes);
            return NULL;
        }
        list->offsets = offsets;
        list->offsetCap += 5;
    }
    used = list->used;
    list->offsets[list->count] = used;
    *(void **)(list->arena + used) = callback;
    list->count++;
    list->used += need;
    return list->arena + used + kDeferSlot;
}

/* 0x4ee940: calls every record's callback, in order (1) or backwards. */
BMX_ADDR(0x004ee940)
RwD3DDeferList *rwD3DDeferRun(RwD3DDeferList *list, int32_t order)
{
    if (order == 1) {
        const uint32_t *offset = list->offsets;
        for (uint32_t n = list->count; n != 0; n--, offset++) {
            (*(RwD3DDeferFunc *)(list->arena + *offset))(list->arena + kDeferSlot + *offset);
        }
    } else {
        const uint32_t *offset = list->offsets + list->count - 1;
        for (uint32_t n = list->count; n != 0; n--, offset--) {
            (*(RwD3DDeferFunc *)(list->arena + *offset))(list->arena + kDeferSlot + *offset);
        }
    }
    return list;
}

/* 0x4ee9b0: empties the list; every 64th time shrinks the arena to what the last 64 needed. */
BMX_ADDR(0x004ee9b0)
RwD3DDeferList *rwD3DDeferReset(RwD3DDeferList *list)
{
    if (list->maxUsed < list->used) {
        list->maxUsed = list->used;
    }
    list->frames = (list->frames + 1) & 0x3f;
    if (list->frames == 0) {
        const uint32_t max = list->maxUsed;
        if ((max >> 1) + max < list->arenaSize) {
            if (max == 0) {
                rwD3DMemory->rwfree(list->arena);
                list->arena = NULL;
                list->arenaSize = list->maxUsed;
            } else {
                uint8_t *arena = rwD3DMemory->rwrealloc(list->arena, max);
                if (arena != NULL) {
                    list->arena = arena;
                    list->arenaSize = list->maxUsed;
                }
            }
        }
        list->maxUsed = 0;
    }
    list->count = 0;
    list->used = 0;
    return list;
}

/*
 * 0x4dafd0 (from the device start): the deferred list and sort buffers, the device's Im2D entries, the linear
 * fog table and the fog range of no camera yet.
 */
BMX_ADDR(0x004dafd0)
int rwD3DIm2DOpen(void)
{
    rwD3DDefer = rwD3DDeferCreate();
    if (rwD3DDefer == NULL) {
        return 0;
    }
    rwD3DMergeCap = 0x10;
    rwD3DRemap = NULL;
    rwD3DEngineDevice->fpIm2DRenderTriangle = BMX_FNPTR(void *, 0x004d8c80, rwD3DIm2DTriangle);
    rwD3DRemapUsed = NULL;
    rwD3DRemapCap = 0;
    rwD3DEngineDevice->fpIm2DRenderLine = BMX_FNPTR(void *, 0x004d8ea0, rwD3DIm2DLine);
    rwD3DSortItems = NULL;
    rwD3DSortNeed = 0;
    rwD3DEngineDevice->fpIm2DRenderPrimitive =
        BMX_FNPTR(RwIm2DRenderPrimitiveFunc, 0x004d9920, rwD3DIm2DPrimitive);
    rwD3DSortCap = 0;
    rwD3DFlipPending = 0;
    rwD3DEngineDevice->fpIm2DRenderIndexedPrimitive =
        BMX_FNPTR(void *, 0x004d9b00, rwD3DIm2DIndexedPrimitive);
    rwD3DMergeBuf = rwD3DMemory->rwmalloc(0x20);
    for (int i = 0; i < 0x100; i++) {
        rwD3DFogLinear[i] = (uint8_t)i;
    }
    rwD3DFogScale = 0.0f;
    rwD3DFogStart = 0;
    rwD3DFogEnd = 1.0f;
    __builtin_memcpy(rwD3DState.fogTable, rwD3DFogLinear, sizeof rwD3DState.fogTable);
    return 1;
}

/* 0x4d48f0 (EAX): saves the state a deferred primitive is drawn with. */
BMX_ADDR(0x004d48f0)
__attribute__((regparm(1))) void rwD3DSaveState(RwD3DSavedState *state)
{
    const RwD3DStateCache *s = &rwD3DState;

    state->address = s->address;
    state->borderColor = s->borderColor;
    state->magFilter = s->magFilter;
    state->minFilter = s->minFilter;
    state->mipFilter = s->mipFilter;
    state->colorOp = s->colorOp;
    state->alphaOp = s->alphaOp;
    state->textureRaster = s->textureRaster;
    state->vertexAlpha = s->vertexAlpha;
    state->perspective = s->perspective;
    state->zTest = s->zTest;
    state->zWrite = s->zWrite;
    state->shadeMode = s->shadeMode;
    state->srcBlend = s->srcBlend;
    state->destBlend = s->destBlend;
    state->fogEnable = s->fogEnable;
    state->fogColor = s->fogColor;
}

/* 0x4d9350 (EAX): sends a saved state to the device (texture raster and vertex alpha through the state set).
 */
BMX_ADDR(0x004d9350)
__attribute__((regparm(1))) void rwD3DRestoreState(const RwD3DSavedState *saved)
{
    RwD3DStateCache *s = &rwD3DState;

    if (s->borderColor != saved->borderColor) {
        rwD3DDevSetTextureStageState(0, 0xf, (uint32_t)saved->borderColor);
        s->borderColor = saved->borderColor;
    }
    if (s->address != saved->address) {
        rwD3DDevSetTextureStageState(0, 0xc, (uint32_t)saved->address);
        s->address = saved->address;
    }
    if (s->magFilter != saved->magFilter) {
        rwD3DDevSetTextureStageState(0, 0x10, (uint32_t)saved->magFilter);
        s->magFilter = saved->magFilter;
    }
    if (s->minFilter != saved->minFilter) {
        rwD3DDevSetTextureStageState(0, 0x11, (uint32_t)saved->minFilter);
        s->minFilter = saved->minFilter;
    }
    if (s->mipFilter != saved->mipFilter) {
        rwD3DDevSetTextureStageState(0, 0x12, (uint32_t)saved->mipFilter);
        s->mipFilter = saved->mipFilter;
    }
    rwD3DSetRenderState(1, (intptr_t)saved->textureRaster);
    if (s->fogColor != saved->fogColor) {
        rwD3DDevSetRenderState(0x22, (uint32_t)saved->fogColor);
        s->fogColor = saved->fogColor;
    }
    if (s->fogEnable != saved->fogEnable) {
        rwD3DDevSetRenderState(0x1c, (uint32_t)saved->fogEnable);
        s->fogEnable = saved->fogEnable;
    }
    if (s->srcBlend != saved->srcBlend) {
        rwD3DDevSetRenderState(0x13, (uint32_t)saved->srcBlend);
        s->srcBlend = saved->srcBlend;
    }
    if (s->destBlend != saved->destBlend) {
        rwD3DDevSetRenderState(0x14, (uint32_t)saved->destBlend);
        s->destBlend = saved->destBlend;
    }
    if (s->shadeMode != saved->shadeMode) {
        rwD3DDevSetRenderState(9, (uint32_t)saved->shadeMode);
        s->shadeMode = saved->shadeMode;
    }
    if (s->zTest != saved->zTest) {
        rwD3DDevSetRenderState(7, (uint32_t)saved->zTest);
        s->zTest = saved->zTest;
    }
    if (s->zWrite != saved->zWrite) {
        rwD3DDevSetRenderState(0xe, (uint32_t)saved->zWrite);
        s->zWrite = saved->zWrite;
    }
    if (s->perspective != saved->perspective) {
        rwD3DDevSetRenderState(4, (uint32_t)saved->perspective);
        s->perspective = saved->perspective;
    }
    rwD3DSetRenderState(10, saved->vertexAlpha);
}

/* 0x4d9050: qsort order of the sort items: larger key (bit pattern) first. */
BMX_ADDR(0x004d9050)
int rwD3DSortCompare(const RwD3DSortItem *a, const RwD3DSortItem *b)
{
    return rwD3DBits(b->key) - rwD3DBits(a->key);
}

/* Appends a sort item's indices to the merge buffer at `at`. */
static void mergeCopy(const RwD3DSortItem *item, uint32_t at)
{
    __builtin_memcpy(rwD3DMergeBuf + at, item->indices, (uint32_t)item->count * 2);
}

/*
 * 0x4d9070: sorts the items back to front and draws them; neighbours from the same record go out as one
 * indexed primitive (the last item never joins a run, as in the original). Stops on no memory.
 */
BMX_ADDR(0x004d9070)
void rwD3DSortDraw(void)
{
    rwD3DQsort(rwD3DSortItems, rwD3DSortCount, sizeof(RwD3DSortItem),
               BMX_FNPTR(void *, 0x004d9050, rwD3DSortCompare));
    for (uint32_t i = 0; i < rwD3DSortCount;) {
        const RwD3DSortItem *items = rwD3DSortItems;
        const RwD3DSortItem *first = &items[i];
        uint8_t *record = first->record;
        const uint16_t *indices;
        uint32_t count;

        rwD3DRestoreState((const RwD3DSavedState *)record);
        if (i < rwD3DSortCount - 1 && items[i].record == items[i + 1].record) {
            uint8_t *const same = items[i].record;
            const uint32_t a = items[i].count, b = items[i + 1].count;

            if (rwD3DMergeCap < a + b) {
                const uint32_t cap = a + 0x14 + b;
                uint16_t *buf = rwD3DMemory->rwrealloc(rwD3DMergeBuf, cap * 2);
                if (buf == NULL) {
                    return;
                }
                rwD3DMergeCap = cap;
                rwD3DMergeBuf = buf;
            }
            mergeCopy(&items[i], 0);
            count = items[i].count;
            mergeCopy(&items[i + 1], count);
            i += 2;
            count += items[i - 1].count;
            for (; i < rwD3DSortCount - 1; i++) {
                if (items[i].record != same) {
                    break;
                }
                const uint32_t total = items[i].count + count;
                if (rwD3DMergeCap < total) {
                    uint16_t *buf = rwD3DMemory->rwrealloc(rwD3DMergeBuf, total * 2 + 0x28);
                    if (buf == NULL) {
                        return;
                    }
                    rwD3DMergeCap = total + 0x14;
                    rwD3DMergeBuf = buf;
                }
                mergeCopy(&items[i], count);
                count += items[i].count;
            }
            indices = rwD3DMergeBuf;
        } else {
            count = first->count;
            i++;
            indices = first->indices;
        }
        rwD3DDevDrawIndexedPrimitive(first->type, kFvfTL, ((RwD3DDeferPrim *)record)->v,
                                     ((RwD3DDeferPrim *)record)->numVertices, indices, count, kDrawFlags);
    }
}

/* The screen z of vertex `index` of a deferred record. */
static float recordZ(const uint8_t *record, uint16_t index)
{
    return ((const RwD3DDeferPrim *)record)->v[index].sz;
}

static void addItem(uint8_t *record, uint16_t *indices, uint16_t count, uint16_t type, float key)
{
    RwD3DSortItem *item = &rwD3DSortItems[rwD3DSortCount++];
    item->record = record;
    item->indices = indices;
    item->count = count;
    item->type = type;
    item->key = key;
}

/*
 * 0x4d9680 (callback of a deferred primitive): splits it into sort items, one per line or triangle (strips
 * become separate triangles); fans are drawn at once.
 */
BMX_ADDR(0x004d9680)
void rwD3DDeferSplit(uint8_t *record)
{
    RwD3DDeferPrim *const prim = (RwD3DDeferPrim *)record;
    const int32_t type = (int32_t)prim->type;
    const uint32_t numVertices = prim->numVertices;
    int32_t n = prim->count;
    uint16_t *p = rwD3DDeferIndices(prim);
    uint32_t mxcsr;

    if (type == kPtTriangleFan) {
        rwD3DRestoreState(&prim->state);
        rwD3DDevDrawIndexedPrimitive(kPtTriangleFan, kFvfTL, prim->v, numVertices, p, (uint32_t)n,
                                     kDrawFlags);
        return;
    }
    mxcsr = rwD3DRoundBegin();
    switch (type) {
    case kPtLineList:
        for (; n > 1; n -= 2, p += 2) {
            addItem(record, p, 2, kPtLineList, recordZ(record, p[0]) + recordZ(record, p[1]));
        }
        break;
    case kPtLineStrip:
        for (; n > 1; n--, p++) {
            addItem(record, p, 2, kPtLineList, recordZ(record, p[0]) + recordZ(record, p[1]));
        }
        break;
    case kPtTriangleList:
        for (; n > 2; n -= 3, p += 3) {
            addItem(record, p, 3, kPtTriangleList,
                    recordZ(record, p[0]) + recordZ(record, p[1]) + recordZ(record, p[2]));
        }
        break;
    case kPtTriangleStrip:
        for (; n > 2; n--, p++) {
            addItem(record, p, 3, kPtTriangleList,
                    recordZ(record, p[0]) + recordZ(record, p[1]) + recordZ(record, p[2]));
        }
        break;
    default:
        break;
    }
    rwD3DRoundEnd(mxcsr);
}

/* 0x4db740: callback of a deferred line. */
BMX_ADDR(0x004db740)
void rwD3DDeferLine(uint8_t *record)
{
    RwD3DDeferVerts *const line = (RwD3DDeferVerts *)record;

    rwD3DRestoreState(&line->state);
    rwD3DDevDrawPrimitive(kPtLineList, kFvfTL, line->v, 2, kDrawFlags);
}

/* 0x4db7a0: callback of a deferred triangle. */
BMX_ADDR(0x004db7a0)
void rwD3DDeferTriangle(uint8_t *record)
{
    RwD3DDeferVerts *const triangle = (RwD3DDeferVerts *)record;

    rwD3DRestoreState(&triangle->state);
    rwD3DDevDrawPrimitive(kPtTriangleList, kFvfTL, triangle->v, 3, kDrawFlags);
}

/*
 * 0x4d4990: draws the deferred primitives in the render order (rwD3DRenderOrder), sorting what they hand
 * to the sort list, then puts the state back as it was.
 */
BMX_ADDR(0x004d4990)
void rwD3DDeferFlush(void)
{
    RwD3DSavedState saved;

    if (rwD3DDefer->used == 0) {
        rwD3DDeferReset(rwD3DDefer);
        return;
    }
    rwD3DSaveState(&saved);
    if (rwD3DSortCap < rwD3DSortNeed) {
        RwD3DSortItem *items =
            rwD3DSortItems == NULL
                ? rwD3DMemory->rwmalloc(rwD3DSortNeed * sizeof(RwD3DSortItem))
                : rwD3DMemory->rwrealloc(rwD3DSortItems, rwD3DSortNeed * sizeof(RwD3DSortItem));
        if (items == NULL) {
            rwD3DDeferReset(rwD3DDefer);
            rwD3DSortNeed = 0;
            rwD3DRestoreState(&saved);
            return;
        }
        rwD3DSortCap = rwD3DSortNeed;
        rwD3DSortItems = items;
    }
    rwD3DSortCount = 0;
    rwD3DDeferRun(rwD3DDefer, rwD3DRenderOrder);
    rwD3DSortDraw();
    rwD3DDeferReset(rwD3DDefer);
    rwD3DSortNeed = 0;
    rwD3DRestoreState(&saved);
}

/* ---- the device's Im2D entries ----------------------------------------------------------------------------
 */

/* Translucent and sorted: the primitive goes to the deferred list. */
static int deferring(void)
{
    const RwD3DStateCache *s = &rwD3DState;
    return s->alphaSort != 0 && s->zTest != 0 && (s->vertexAlpha != 0 || s->textureAlpha != 0);
}

/* 0x4d8ea0: RwIm2DRenderLine. */
BMX_ADDR(0x004d8ea0)
int rwD3DIm2DLine(RwD3DTLVertex *vertices, int32_t numVertices, int32_t a, int32_t b)
{
    uint8_t *record;

    if (!deferring()) {
        const uint16_t indices[2] = {(uint16_t)a, (uint16_t)b};
        rwD3DDevDrawIndexedPrimitive(kPtLineList, kFvfTL, vertices, (uint32_t)numVertices, indices, 2,
                                     kDrawFlags);
        return 1;
    }
    record = rwD3DDeferAlloc(rwD3DDefer, (uint32_t)(sizeof(RwD3DDeferVerts) + 2 * sizeof(RwD3DTLVertex)),
                             BMX_FNPTR(void *, 0x004db740, rwD3DDeferLine));
    if (record == NULL) {
        /* Three indices, as in the original: the third is the vertex count's low half. */
        const uint16_t indices[3] = {(uint16_t)a, (uint16_t)b, (uint16_t)numVertices};
        rwD3DDevDrawIndexedPrimitive(kPtLineList, kFvfTL, vertices, (uint32_t)numVertices, indices, 3,
                                     kDrawFlags);
        return 1;
    }
    rwD3DSaveState(&((RwD3DDeferVerts *)record)->state);
    __builtin_memcpy(&((RwD3DDeferVerts *)record)->v[0], &vertices[a], sizeof *vertices);
    __builtin_memcpy(&((RwD3DDeferVerts *)record)->v[1], &vertices[b], sizeof *vertices);
    return 1;
}

/* 0x4d8c80: RwIm2DRenderTriangle. */
BMX_ADDR(0x004d8c80)
int rwD3DIm2DTriangle(RwD3DTLVertex *vertices, int32_t numVertices, int32_t a, int32_t b, int32_t c)
{
    const uint16_t indices[3] = {(uint16_t)a, (uint16_t)b, (uint16_t)c};
    uint8_t *record;

    if (!deferring()) {
        rwD3DDevDrawIndexedPrimitive(kPtTriangleList, kFvfTL, vertices, (uint32_t)numVertices, indices, 3,
                                     kDrawFlags);
        return 1;
    }
    record = rwD3DDeferAlloc(rwD3DDefer, (uint32_t)(sizeof(RwD3DDeferVerts) + 3 * sizeof(RwD3DTLVertex)),
                             BMX_FNPTR(void *, 0x004db7a0, rwD3DDeferTriangle));
    if (record == NULL) {
        rwD3DDevDrawIndexedPrimitive(kPtTriangleList, kFvfTL, vertices, (uint32_t)numVertices, indices, 3,
                                     kDrawFlags);
        return 1;
    }
    rwD3DSaveState(&((RwD3DDeferVerts *)record)->state);
    __builtin_memcpy(&((RwD3DDeferVerts *)record)->v[0], &vertices[a], sizeof *vertices);
    __builtin_memcpy(&((RwD3DDeferVerts *)record)->v[1], &vertices[b], sizeof *vertices);
    __builtin_memcpy(&((RwD3DDeferVerts *)record)->v[2], &vertices[c], sizeof *vertices);
    return 1;
}

/* The D3D primitive type and primitive count of an rwPRIMTYPE* (1..5); 0 for others. */
static uint32_t primType(int32_t type, int32_t n, int32_t *prims)
{
    switch (type) {
    case 1:
        *prims = n / 2;
        return kPtLineList;
    case 2:
        *prims = n - 1;
        return kPtLineStrip;
    case 3:
        *prims = n / 3;
        return kPtTriangleList;
    case 4:
        *prims = n - 2;
        return kPtTriangleStrip;
    case 5:
        *prims = n - 2;
        return kPtTriangleFan;
    default:
        return 0;
    }
}

/* 0x4d9920: RwIm2DRenderPrimitive. */
BMX_ADDR(0x004d9920)
int rwD3DIm2DPrimitive(int32_t type, RwD3DTLVertex *vertices, int32_t numVertices)
{
    int32_t prims;
    const uint32_t d3dType = primType(type, numVertices, &prims);
    uint8_t *record;
    RwD3DDeferPrim *prim;
    uint16_t *indices;

    if (d3dType == 0) {
        return 0;
    }
    if (!deferring()) {
        rwD3DDevDrawPrimitive(d3dType, kFvfTL, vertices, (uint32_t)numVertices, kDrawFlags);
        return 1;
    }
    record = rwD3DDeferAlloc(rwD3DDefer,
                             (uint32_t)(sizeof(RwD3DDeferPrim) +
                                        (uint32_t)numVertices * (sizeof(RwD3DTLVertex) + sizeof(uint16_t))),
                             BMX_FNPTR(void *, 0x004d9680, rwD3DDeferSplit));
    if (record == NULL) {
        rwD3DDevDrawPrimitive(d3dType, kFvfTL, vertices, (uint32_t)numVertices, kDrawFlags);
        return 1;
    }
    rwD3DSortNeed += (uint32_t)prims;
    prim = (RwD3DDeferPrim *)record;
    rwD3DSaveState(&prim->state);
    prim->count = numVertices;
    prim->numVertices = (uint32_t)numVertices;
    prim->type = d3dType;
    __builtin_memcpy(prim->v, vertices, (uint32_t)numVertices * sizeof *vertices);
    indices = rwD3DDeferIndices(prim);
    for (int32_t i = 0; i < numVertices; i++) {
        indices[i] = (uint16_t)i;
    }
    return 1;
}

/*
 * 0x4d9b00: RwIm2DRenderIndexedPrimitive. A deferred one keeps only the vertices its indices use, renumbered
 * in order of first use.
 */
BMX_ADDR(0x004d9b00)
int rwD3DIm2DIndexedPrimitive(int32_t type, RwD3DTLVertex *vertices, int32_t numVertices,
                              const uint16_t *indices, int32_t numIndices)
{
    int32_t prims, used = 0;
    const uint32_t d3dType = primType(type, numIndices, &prims);
    uint8_t *record;
    RwD3DDeferPrim *prim;
    uint16_t *out;

    if (d3dType == 0) {
        return 0;
    }
    if (!deferring()) {
        rwD3DDevDrawIndexedPrimitive(d3dType, kFvfTL, vertices, (uint32_t)numVertices, indices,
                                     (uint32_t)numIndices, kDrawFlags);
        return 1;
    }
    if (rwD3DRemapCap < numVertices) {
        uint16_t *remap = rwD3DRemap == NULL ? rwD3DMemory->rwmalloc((uint32_t)numVertices * 2)
                                             : rwD3DMemory->rwrealloc(rwD3DRemap, (uint32_t)numVertices * 2);
        uint16_t *list = rwD3DRemapUsed == NULL
                             ? rwD3DMemory->rwmalloc((uint32_t)numVertices * 2)
                             : rwD3DMemory->rwrealloc(rwD3DRemapUsed, (uint32_t)numVertices * 2);
        if (remap != NULL) {
            rwD3DRemap = remap;
        }
        if (list != NULL) {
            rwD3DRemapUsed = list;
        }
        if (remap == NULL || list == NULL) {
            rwD3DDevDrawIndexedPrimitive(d3dType, kFvfTL, vertices, (uint32_t)numVertices, indices,
                                         (uint32_t)numIndices, kDrawFlags);
            return 1;
        }
        rwD3DRemapCap = numVertices;
    }
    __builtin_memset(rwD3DRemap, 0xff, (uint32_t)numVertices * 2);
    for (int32_t i = 0; i < numIndices; i++) {
        if (rwD3DRemap[indices[i]] == 0xffff) {
            rwD3DRemap[indices[i]] = (uint16_t)used;
            rwD3DRemapUsed[used] = indices[i];
            used++;
        }
    }
    record = rwD3DDeferAlloc(rwD3DDefer,
                             (uint32_t)(sizeof(RwD3DDeferPrim) + (uint32_t)numIndices * sizeof(uint16_t) +
                                        (uint32_t)used * sizeof(RwD3DTLVertex)),
                             BMX_FNPTR(void *, 0x004d9680, rwD3DDeferSplit));
    if (record == NULL) {
        rwD3DDevDrawIndexedPrimitive(d3dType, kFvfTL, vertices, (uint32_t)numVertices, indices,
                                     (uint32_t)numIndices, kDrawFlags);
        return 1;
    }
    rwD3DSortNeed += (uint32_t)prims;
    prim = (RwD3DDeferPrim *)record;
    rwD3DSaveState(&prim->state);
    prim->count = numIndices;
    prim->numVertices = (uint32_t)used;
    prim->type = d3dType;
    for (int32_t i = 0; i < used; i++) {
        __builtin_memcpy(&prim->v[i], &vertices[rwD3DRemapUsed[i]], sizeof *vertices);
    }
    out = rwD3DDeferIndices(prim);
    for (int32_t i = 0; i < numIndices; i++) {
        out[i] = rwD3DRemap[indices[i]];
    }
    return 1;
}
