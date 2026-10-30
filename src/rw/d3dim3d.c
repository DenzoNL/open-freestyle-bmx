/*
 * RenderWare 3.1's Direct3D 7 driver, Im3D side (lib/d3d7driver): the core extension's engine plugin, the
 * Im3D pipeline stages that transform RwIm3DVertex arrays into camera space and TL vertices with clip flags,
 * and the draw entries that turn primitives into index lists for the device's Im2D indexed primitive.
 * Nothing here talks to Direct3D; clipped primitives go to the clipper (rwD3DClipTri / rwD3DClipLine).
 */
#include "rw/rwd3d.h"
#include "rw/rwd3dclip.h"
#include "rw/rwtexture.h"

/* The Im3D device entries and pipeline stages of the driver (stored as the original's addresses). */
#define D3D_FN(type, addr, fn) BMX_FNPTR(type, addr, fn)

/* ---- the Im3D transform stack ------------------------------------------------------------------------- */

/* 0x4ea960 (Im3D begin): opens a level on the stack; its vertices follow the previous level's count. */
BMX_ADDR(0x004ea960)
int rwD3DIm3DBegin(RwIm3DPipeline *pipeline)
{
    const uint32_t depth = rwD3DIm3DDepth;
    RwD3DIm3DLevel *level;

    if (pipeline == NULL || depth >= 10) {
        return 0;
    }
    level = &rwD3DIm3DLevels[depth];
    level->base = rwD3DIm3DLevel != NULL ? rwD3DIm3DLevel->numVertices : 0;
    level->numTL = 0;
    level->numVertices = 0;
    rwD3DIm3DLevel = level;
    rwD3DTLBase = rwD3DTLBuffer + level->base;
    rwD3DIm3DDepth = depth + 1;
    rwD3DCamBase = rwD3DCamBuffer + level->base;
    return 1;
}

/* 0x4eaa10 (Im3D end): closes the top level. */
BMX_ADDR(0x004eaa10)
int rwD3DIm3DEnd(void)
{
    if (rwD3DIm3DDepth == 0) {
        return 0;
    }
    rwD3DIm3DDepth--;
    if (rwD3DIm3DDepth == 0) {
        rwD3DCamBase = NULL;
        rwD3DTLBase = NULL;
        rwD3DIm3DLevel = NULL;
    } else {
        RwD3DIm3DLevel *level = &rwD3DIm3DLevels[rwD3DIm3DDepth - 1];
        rwD3DIm3DLevel = level;
        rwD3DTLBase = rwD3DTLBuffer + level->base;
        rwD3DCamBase = rwD3DCamBuffer + level->base;
    }
    return 1;
}

/* 0x4eacc0: grows both vertex buffers to the top level's base + extra vertices; 0 if either fails. */
BMX_ADDR(0x004eacc0)
int rwD3DIm3DGrow(uint32_t extra)
{
    const uint32_t size = extra + (uint32_t)rwD3DIm3DLevel->base;
    RwMemoryFunctions *mem = &RwEngineInstance->memoryFuncs;
    RwD3DTLVertex *tl;
    RwD3DCamVertex *cam;

    tl = rwD3DTLBuffer == NULL ? mem->rwmalloc(size * sizeof *tl)
                               : mem->rwrealloc(rwD3DTLBuffer, size * sizeof *tl);
    cam = rwD3DCamBuffer == NULL ? mem->rwmalloc(size * sizeof *cam)
                                 : mem->rwrealloc(rwD3DCamBuffer, size * sizeof *cam);
    if (tl != NULL) {
        rwD3DTLBase = tl + rwD3DIm3DLevel->base;
        rwD3DTLBuffer = tl;
    }
    if (cam != NULL) {
        rwD3DCamBase = cam + rwD3DIm3DLevel->base;
        rwD3DCamBuffer = cam;
    }
    if (tl == NULL || cam == NULL) {
        return 0;
    }
    rwD3DBufferSize = size;
    return 1;
}

/* ---- index lists ----------------------------------------------------------------------------------------
 */

/* 0x4eadc8 (list in EAX, capacity in EDX): reallocates an index list. */
BMX_ADDR(0x004eadc8)
__attribute__((regparm(2))) int rwD3DIndexListGrow(RwD3DIndexList *list, uint32_t capacity)
{
    uint16_t *indices = RwEngineInstance->memoryFuncs.rwrealloc(list->indices, capacity * 2);

    if (indices == NULL) {
        return 0;
    }
    list->indices = indices;
    list->capacity = capacity;
    return 1;
}

/* 0x4eadc0: the same with stack arguments (the instancing pipelines call it). */
BMX_ADDR(0x004eadc0)
int rwD3DIndexListResize(RwD3DIndexList *list, uint32_t capacity)
{
    uint16_t *indices = RwEngineInstance->memoryFuncs.rwrealloc(list->indices, capacity * 2);

    if (indices == NULL) {
        return 0;
    }
    list->indices = indices;
    list->capacity = capacity;
    return 1;
}

/* 0x4eb270: an empty list with room for one index. */
BMX_ADDR(0x004eb270)
void rwD3DIndexListCreate(RwD3DIndexList *list)
{
    list->count = 0;
    list->capacity = 1;
    list->indices = RwEngineInstance->memoryFuncs.rwmalloc(2);
}

/* 0x4eb240 */
BMX_ADDR(0x004eb240)
void rwD3DIndexListDestroy(RwD3DIndexList *list)
{
    if (list->indices != NULL) {
        RwEngineInstance->memoryFuncs.rwfree(list->indices);
        list->indices = NULL;
    }
}

/* 0x4eae10: a draw call starts: the TL count restarts at the transformed vertices, both lists empty. */
BMX_ADDR(0x004eae10)
void rwD3DIndexStartDefault(void)
{
    rwD3DIm3DLevel->numTL = rwD3DIm3DLevel->numVertices;
    rwD3DTriIndices.count = 0;
    rwD3DLineIndices.count = 0;
}

/* 0x4eaeb0: appends a triangle (growing by 0x42 indices when fewer than 6 are left). */
BMX_ADDR(0x004eaeb0)
void rwD3DIndexTriDefault(int32_t a, int32_t b, int32_t c)
{
    uint16_t *indices;

    if (rwD3DTriIndices.count + 6 > rwD3DTriIndices.capacity) {
        rwD3DIndexListGrow(&rwD3DTriIndices, rwD3DTriIndices.count + 0x42);
    }
    indices = rwD3DTriIndices.indices + rwD3DTriIndices.count;
    indices[2] = (uint16_t)c;
    indices[1] = (uint16_t)b;
    indices[0] = (uint16_t)a;
    rwD3DTriIndices.count += 3;
}

/* 0x4eaf30: appends a line (growing by 0x40 indices when fewer than 4 are left). */
BMX_ADDR(0x004eaf30)
void rwD3DIndexLineDefault(int32_t a, int32_t b)
{
    uint16_t *indices;

    if (rwD3DLineIndices.count + 4 > rwD3DLineIndices.capacity) {
        rwD3DIndexListGrow(&rwD3DLineIndices, rwD3DLineIndices.count + 0x40);
    }
    indices = rwD3DLineIndices.indices + rwD3DLineIndices.count;
    indices[1] = (uint16_t)b;
    indices[0] = (uint16_t)a;
    rwD3DLineIndices.count += 2;
}

/* 0x4eae30: draws the lists through the device's Im2D indexed primitive (triangle list, then line list). */
BMX_ADDR(0x004eae30)
void rwD3DIndexFlushDefault(void)
{
    if (rwD3DTriIndices.count != 0) {
        RwIm2DRenderIndexedPrimitive(3, rwD3DTLBase, rwD3DIm3DLevel->numTL, rwD3DTriIndices.indices,
                                     (int32_t)rwD3DTriIndices.count);
    }
    if (rwD3DLineIndices.count != 0) {
        RwIm2DRenderIndexedPrimitive(1, rwD3DTLBase, rwD3DIm3DLevel->numTL, rwD3DLineIndices.indices,
                                     (int32_t)rwD3DLineIndices.count);
    }
}

/* ---- texture coordinates --------------------------------------------------------------------------------
 */

/* 0x4eb030: transforms without rwIM3D_VERTEXUV keep whatever the TL vertices hold. */
BMX_ADDR(0x004eb030)
void rwD3DUVNone(RwD3DIm3DWork *work) {}

/* 0x4eb040: copies the texture coordinates into the camera vertices and, where they are inside, the TL ones.
 */
BMX_ADDR(0x004eb040)
void rwD3DUVCopy(RwD3DIm3DWork *work)
{
    const uint8_t *src = work->vertices;
    RwD3DCamVertex *cam = rwD3DCamBase;
    RwD3DTLVertex *tl = rwD3DTLBase;

    if (rwD3DIm3DLevel->orClip == 0) {
        for (int32_t n = work->numVertices; n != 0; n--) {
            const RwD3DIm3DVertex *v = (const RwD3DIm3DVertex *)src;
            cam->u = v->u;
            cam->v = v->v;
            tl->u = v->u;
            tl->v = cam->v;
            src += work->stride;
            tl++;
            cam++;
        }
    } else {
        for (int32_t n = work->numVertices; n != 0; n--) {
            const RwD3DIm3DVertex *v = (const RwD3DIm3DVertex *)src;
            cam->u = v->u;
            cam->v = v->v;
            if (cam->clip == 0) {
                tl->u = cam->u;
                tl->v = cam->v;
            }
            src += work->stride;
            tl++;
            cam++;
        }
    }
}

/* 0x4eb0f0: picks the texture coordinate stage of the top level by the transform flags (2: rwIM3D_VERTEXUV).
 */
BMX_ADDR(0x004eb0f0)
void rwD3DSetUVStageDefault(uint32_t flags)
{
    rwD3DIm3DLevel->uvStage = (flags & 2) == 0 ? D3D_FN(RwD3DUVStageFunc, 0x004eb030, rwD3DUVNone)
                                               : D3D_FN(RwD3DUVStageFunc, 0x004eb040, rwD3DUVCopy);
}

/* 0x4eb0d0 */
BMX_ADDR(0x004eb0d0)
void rwD3DSetUVStage0(uint32_t flags)
{
    rwD3DSetUVStage(flags);
}

/* ---- the engine plugin of the core extension (0x40e) ------------------------------------------------- */

/*
 * 0x4eaaa0: installs the Im3D side: projection and texture coordinate hooks, empty index lists, the default
 * index functions, the Im3D begin/end and draw entries of the device, and the work object. NULL on no memory.
 */
BMX_ADDR(0x004eaaa0)
void *rwD3DIm3DOpen(void *engine)
{
    RwDevice *device;
    RwD3DIm3DWork *work;

    rwD3DSetProjection = D3D_FN(RwD3DProjectionFunc, 0x004f69f0, rwD3DSetProjectionDefault);
    rwD3DTLBuffer = NULL;
    rwD3DCamBuffer = NULL;
    rwD3DBufferSize = 0;
    rwD3DSetUVStage = D3D_FN(RwD3DUVSetFunc, 0x004eb0f0, rwD3DSetUVStageDefault);
    rwD3DIndexListCreate(&rwD3DTriIndices);
    rwD3DIndexListCreate(&rwD3DLineIndices);
    rwD3DIm3DLevel = NULL;
    rwD3DIm3DDepth = 0;
    rwD3DIm3DSpare1 = 0;
    rwD3DIm3DSpare0 = NULL;
    rwIm3DGouraud = 1;
    rwD3DIndexStart = D3D_FN(RwD3DVoidFunc, 0x004eae10, rwD3DIndexStartDefault);
    rwD3DIndexTri = D3D_FN(RwD3DTriVoidFunc, 0x004eaeb0, rwD3DIndexTriDefault);
    rwD3DIndexLine = D3D_FN(RwD3DLineFunc, 0x004eaf30, rwD3DIndexLineDefault);
    rwD3DIndexFlush = D3D_FN(RwD3DVoidFunc, 0x004eae30, rwD3DIndexFlushDefault);
    rwIm3DSetEntries(D3D_FN(RwIm3DBeginFunc, 0x004ea960, rwD3DIm3DBegin),
                     D3D_FN(RwIm3DEndFunc, 0x004eaa10, rwD3DIm3DEnd));
    device = &RwEngineInstance->dOpenDevice;
    device->fpIm3DRenderLine = D3D_FN(RwIm3DRenderLineFunc, 0x004f69b0, rwD3DIm3DRenderLine);
    device->fpIm3DRenderTriangle = D3D_FN(void *, 0x004f6050, rwD3DIm3DRenderTriangle);
    device->fpIm3DRenderPrimitive = D3D_FN(RwIm3DRenderPrimitiveFunc, 0x004f6090, rwD3DIm3DRenderPrimitive);
    device->fpIm3DRenderIndexedPrimitive =
        D3D_FN(RwIm3DRenderIndexedPrimitiveFunc, 0x004f64e0, rwD3DIm3DRenderIndexedPrimitive);
    work = RwEngineInstance->memoryFuncs.rwmalloc(sizeof *work);
    rwD3DIm3DWork = work;
    if (work == NULL) {
        rwD3DIndexListDestroy(&rwD3DTriIndices);
        rwD3DIndexListDestroy(&rwD3DLineIndices);
        return NULL;
    }
    work->stride = sizeof(RwD3DIm3DVertex);
    *(int32_t *)(work->pad2c + 0xc) = 0;
    *(int32_t *)(work->pad2c + 0x0) = 0;
    *(int32_t *)(work->pad2c + 0x4) = 0;
    *(int32_t *)(work->pad2c + 0x8) = 0;
    work->pad40 = 0;
    return engine;
}

/* 0x4eabd0: frees the buffers, the work object and the index lists. */
BMX_ADDR(0x004eabd0)
void *rwD3DIm3DClose(void *engine)
{
    RwMemoryFunctions *mem = &RwEngineInstance->memoryFuncs;

    if (rwD3DIm3DSpare0 != NULL) {
        mem->rwfree(rwD3DIm3DSpare0);
        rwD3DIm3DSpare0 = NULL;
    }
    if (rwD3DTLBuffer != NULL) {
        RwEngineInstance->memoryFuncs.rwfree(rwD3DTLBuffer);
        rwD3DTLBuffer = NULL;
    }
    if (rwD3DCamBuffer != NULL) {
        RwEngineInstance->memoryFuncs.rwfree(rwD3DCamBuffer);
        rwD3DCamBuffer = NULL;
    }
    if (rwD3DIm3DWork != NULL) {
        RwEngineInstance->memoryFuncs.rwfree(rwD3DIm3DWork);
        rwD3DIm3DWork = NULL;
    }
    rwD3DIndexListDestroy(&rwD3DTriIndices);
    rwD3DIndexListDestroy(&rwD3DLineIndices);
    rwD3DBufferSize = 0;
    return engine;
}

/*
 * 0x4eb150 (core extension entry 1, from the camera's begin update): the screen mapping of the camera's
 * raster, its clip planes, view window and screen z range for the Im3D transform; z test and z write on.
 */
BMX_ADDR(0x004eb150)
void rwD3DIm3DCameraBegin(RwCamera *camera)
{
    const RwRaster *raster = RwCameraGetRaster(camera);

    rwD3DSetProjection(camera->projection);
    rwD3DZScale = camera->zScale;
    rwD3DZShift = camera->zShift;
    rwD3DXScale = (float)RwRasterGetWidth(raster);
    rwD3DYScale = (float)RwRasterGetHeight(raster);
    rwD3DXOffset = (float)raster->originX;
    rwD3DYOffset = (float)raster->originY;
    rwD3DNearZ = rwD3DBits(camera->nearPlane);
    rwD3DFarZ = rwD3DBits(camera->farPlane);
    rwD3DRecipWindow = camera->recipViewWindow;
    rwD3DNearScreenZ = RwIm2DGetNearScreenZ();
    rwD3DFarScreenZ = RwIm2DGetFarScreenZ();
    RwRenderStateSet(4, 1);
    RwRenderStateSet(6, 1);
}

/* 0x4eb120 (core extension entry 2): the shade mode (5) decides between Gouraud (2) and flat colours. */
BMX_ADDR(0x004eb120)
void rwD3DIm3DRenderState(int32_t state, int32_t value)
{
    if (state == rwRENDERSTATESHADEMODE) {
        rwIm3DGouraud = value == rwSHADEMODEGOURAUD;
    }
}

/* 0x4f69f0: the culling and clipping functions of a projection (2: parallel, else perspective). */
BMX_ADDR(0x004f69f0)
void rwD3DSetProjectionDefault(int32_t projection)
{
    if (projection == 2) {
        rwD3DClipLine = D3D_FN(RwD3DLineFunc, 0x004f59b0, rwD3DClipLineParallel);
        rwD3DTriCull = D3D_FN(RwD3DTriFunc, 0x004f4e30, rwD3DTriCullParallel);
        rwD3DClipTri = D3D_FN(RwD3DTriVoidFunc, 0x004f4ea0, rwD3DClipTriParallel);
        rwD3DClipTriCull = D3D_FN(RwD3DTriFunc, 0x004f4e30, rwD3DTriCullParallel);
        return;
    }
    rwD3DClipLine = D3D_FN(RwD3DLineFunc, 0x004f5450, rwD3DClipLinePersp);
    rwD3DClipTriCull = D3D_FN(RwD3DTriFunc, 0x004f3d20, rwD3DClipTriCullPersp);
    rwD3DClipTri = D3D_FN(RwD3DTriVoidFunc, 0x004f3e40, rwD3DClipTriPersp);
    rwD3DTriCull = D3D_FN(RwD3DTriFunc, 0x004f3de0, rwD3DTriCullPersp);
}

/* ---- the Im3D pipeline stages ----------------------------------------------------------------------------
 */

/*
 * 0x4f6a50 (instance): the work object for a transform: vertices, count, and the matrix to camera space (the
 * camera's view matrix, or ltm x view). Grows the vertex buffers to the vertices plus a quarter for clipping.
 */
BMX_ADDR(0x004f6a50)
RwD3DIm3DWork *rwD3DIm3DInstance(RwIm3DTransformInfo *info)
{
    RwD3DIm3DWork *work;
    const RwMatrix *view;
    uint32_t n;

    rwD3DSetUVStage0((info->flags & 1) != 0 ? 2 : 0);
    work = rwD3DIm3DWork;
    n = (uint32_t)info->numVertices;
    work->pad3e = 0;
    work->numVertices = (int32_t)n;
    work->vertices = info->vertices;
    view = &((const RwCamera *)RwEngineInstance->curCamera)->viewMatrix;
    if (info->ltm == NULL) {
        work->matrix = view;
    } else {
        work->matrix = &work->combined;
        RwMatrixMultiply(&work->combined, info->ltm, view);
    }
    rwD3DIm3DLevel->numVertices = (int32_t)n;
    if (n + (uint32_t)rwD3DIm3DLevel->base + (n >> 2) > rwD3DBufferSize) {
        rwD3DIm3DGrow((n >> 2) + n);
    }
    return rwD3DIm3DWork;
}

/* 0x4f6b00 and 0x4f6b70: stages 2 and 6 do nothing. */
BMX_ADDR(0x004f6b00)
void rwD3DIm3DNothing(RwD3DIm3DWork *work) {}

BMX_ADDR(0x004f6b70)
void rwD3DIm3DNothing6(RwD3DIm3DWork *work) {}

/* 0x4f6b10 (stage 5): vertex colours (RwRGBA) into the TL vertices as D3DCOLOR. */
BMX_ADDR(0x004f6b10)
void rwD3DIm3DColors(RwD3DIm3DWork *work)
{
    const uint8_t *src = work->vertices;
    RwD3DTLVertex *tl = rwD3DTLBase;

    for (int32_t n = work->numVertices; n != 0; n--) {
        const RwD3DIm3DVertex *v = (const RwD3DIm3DVertex *)src;
        tl->color = (uint32_t)v->a << 24 | (uint32_t)v->r << 16 | (uint32_t)v->g << 8 | v->b;
        src += work->stride;
        tl++;
    }
}

/* Fog index of a camera space z: 0 before the fog plane, 255 from the far plane on. */
static uint32_t fogSpecular(const RwD3DCamVertex *cam)
{
    const int32_t z = rwD3DBits(cam->z);
    int32_t index;

    if (z <= rwD3DFogStart) {
        index = 0;
    } else if (z >= rwD3DBits(rwD3DFogEnd)) {
        index = 0xff;
    } else {
        float start;
        __builtin_memcpy(&start, &rwD3DFogStart, 4);
        rwD3DXformFog = rwD3DFistp((cam->z - start) * rwD3DFogScale);
        index = rwD3DXformFog;
    }
    return rwD3DFogTable[index];
}

/* Camera space position of a vertex: (x, y, z) x matrix, summed in the original's order. */
static void transformVertex(const RwD3DIm3DVertex *v, const RwMatrix *m, RwD3DCamVertex *cam)
{
    const float x0 = v->x * m->right.x, y0 = v->x * m->right.y, z0 = v->x * m->right.z;
    const float x1 = v->y * m->up.x, y1 = v->y * m->up.y, z1 = v->y * m->up.z;
    const float x2 = v->z * m->at.x, y2 = v->z * m->at.y, z2 = v->z * m->at.z;

    cam->y = y0 + y1 + y2 + m->pos.y;
    cam->x = x0 + x1 + x2 + m->pos.x;
    cam->z = z0 + z1 + z2 + m->pos.z;
}

/* Perspective clip flags: x and y against z, z against near and far (bit patterns compared as integers). */
static uint32_t clipPersp(const RwD3DCamVertex *cam)
{
    const int32_t x = rwD3DBits(cam->x), y = rwD3DBits(cam->y), z = rwD3DBits(cam->z);
    const uint32_t xs = (uint32_t)x >> 31, ys = (uint32_t)y >> 31;
    uint32_t code = (uint32_t)(z > rwD3DFarZ) << 2;

    if (z >= rwD3DNearZ) {
        code = (((code + (uint32_t)(z < y)) * 2 + ys) * 2 + (uint32_t)(z < x)) * 2 + xs;
    } else if (z >= 0) {
        code = (((code + 2 + (uint32_t)(z < y)) * 2 + ys) * 2 + (uint32_t)(z < x)) * 2 + xs;
    } else {
        code = (((code + 2 + ((uint32_t)(z < y) ^ ys)) * 2 + ys) * 2 + ((uint32_t)(z < x) ^ xs)) * 2 + xs;
    }
    return code;
}

/* Parallel clip flags: x and y against 0 and 1, z against near and far. */
static uint32_t clipParallel(const RwD3DCamVertex *cam)
{
    const int32_t x = rwD3DBits(cam->x), y = rwD3DBits(cam->y), z = rwD3DBits(cam->z);
    const int32_t one = rwD3DBits(rwD3DOne);
    uint32_t code = (uint32_t)(z > rwD3DFarZ) * 2 | (uint32_t)(z < rwD3DNearZ);

    code = (code * 2 + (uint32_t)(one < y)) * 2 + ((uint32_t)y >> 31);
    return (code * 2 + (uint32_t)(one < x)) * 2 + ((uint32_t)x >> 31);
}

/*
 * 0x4f97c0 (stage 1, the device's pipeline stage): transforms the work's vertices into camera space with clip
 * flags and, for those inside, into TL vertices (screen position, 1/z, fog in the specular alpha). Records
 * the or'ed and and'ed clip flags in the level and, unless everything is outside, runs its texture coordinate
 * stage. Hand-written x87 in the original; the arithmetic here follows its order operation by operation.
 */
BMX_ADDR(0x004f97c0)
void rwD3DIm3DTransform(RwD3DIm3DWork *work)
{
    const uint32_t mxcsr = rwD3DRoundBegin();
    const uint8_t *src = work->vertices;
    RwD3DCamVertex *cam = rwD3DCamBase;
    const int parallel = ((const RwCamera *)RwEngineInstance->curCamera)->projection != 1;

    rwD3DAndClip = 0x3f;
    rwD3DOrClip = 0;
    rwD3DXformStride = work->stride;
    rwD3DXformMatrix = work->matrix;
    rwD3DXformOut = rwD3DTLBase;
    for (int32_t n = work->numVertices - 1; n >= 0; n--) {
        RwD3DTLVertex *tl = rwD3DXformOut;
        uint32_t code;

        transformVertex((const RwD3DIm3DVertex *)src, rwD3DXformMatrix, cam);
        src += rwD3DXformStride;
        code = parallel ? clipParallel(cam) : clipPersp(cam);
        cam->clip = (uint8_t)code;
        rwD3DOrClip |= code;
        rwD3DAndClip &= code;
        if ((uint8_t)code == 0) {
            if (parallel) {
                tl->rhw = rwD3DOne;
                tl->sz = cam->z * rwD3DZScale + rwD3DZShift;
                tl->sx = cam->x * rwD3DXScale + rwD3DXOffset;
                tl->sy = cam->y * rwD3DYScale + rwD3DYOffset;
            } else {
                const float rhw = rwD3DOne / cam->z;
                tl->rhw = rhw;
                tl->sz = rwD3DZScale * rhw + rwD3DZShift;
                tl->sx = cam->x * rhw * rwD3DXScale + rwD3DXOffset;
                tl->sy = rhw * cam->y * rwD3DYScale + rwD3DYOffset;
            }
            tl->specular = fogSpecular(cam);
        }
        cam++;
        rwD3DXformOut = tl + 1;
    }
    rwD3DIm3DLevel->orClip = rwD3DOrClip;
    rwD3DIm3DLevel->andClip = rwD3DAndClip;
    rwD3DRoundEnd(mxcsr);
    if (rwD3DIm3DLevel->andClip == 0) {
        rwD3DIm3DLevel->uvStage(work);
    }
}

/* 0x4f6b80 (core extension entry 3): the driver's stages of a new Im3D pipeline. */
BMX_ADDR(0x004f6b80)
int rwD3DIm3DPipeline(uint32_t *pipeline)
{
    if (pipeline == NULL) {
        return 0;
    }
    pipeline[0] = (uint32_t)(uintptr_t)D3D_FN(void *, 0x004f6a50, rwD3DIm3DInstance);
    pipeline[2] = (uint32_t)(uintptr_t)D3D_FN(void *, 0x004f6b00, rwD3DIm3DNothing);
    pipeline[5] = (uint32_t)(uintptr_t)D3D_FN(void *, 0x004f6b10, rwD3DIm3DColors);
    pipeline[6] = (uint32_t)(uintptr_t)D3D_FN(void *, 0x004f6b70, rwD3DIm3DNothing6);
    return 1;
}

/* ---- culling ----------------------------------------------------------------------------------------------
 */

/* Twice the signed screen area of a TL triangle, as the original computes it. */
static float screenArea(const RwD3DTLVertex *a, const RwD3DTLVertex *b, const RwD3DTLVertex *c)
{
    const float cy = c->sy - b->sy;
    const float ax = a->sx - b->sx;
    const float cx = c->sx - b->sx;
    const float ay = a->sy - b->sy;

    return ax * cy - cx * ay;
}

/* 0x4f3de0: a perspective triangle with all vertices inside is culled (1) unless its screen area is positive.
 */
BMX_ADDR(0x004f3de0)
int rwD3DTriCullPersp(int32_t a, int32_t b, int32_t c)
{
    const uint32_t mxcsr = rwD3DRoundBegin();
    const RwD3DTLVertex *tl = rwD3DTLBase;
    const float area = screenArea(&tl[a], &tl[b], &tl[c]);

    rwD3DRoundEnd(mxcsr);
    return rwD3DBits(area) > 0 ? 0 : 1;
}

/* 0x4f4e30: the same for parallel projections in camera space; zero area is kept. */
BMX_ADDR(0x004f4e30)
int rwD3DTriCullParallel(int32_t a, int32_t b, int32_t c)
{
    const uint32_t mxcsr = rwD3DRoundBegin();
    const RwD3DCamVertex *v = rwD3DCamBase;
    const float cy = v[c].y - v[b].y;
    const float ax = v[a].x - v[b].x;
    const float cx = v[c].x - v[b].x;
    const float ay = v[a].y - v[b].y;
    const float area = ax * cy - cx * ay;

    rwD3DRoundEnd(mxcsr);
    return rwD3DBits(area) >= 0 ? 0 : 1;
}

/* 0x4f3d20: a clipped perspective triangle: culled unless the camera space triple product is positive. */
BMX_ADDR(0x004f3d20)
int rwD3DClipTriCullPersp(int32_t a, int32_t b, int32_t c)
{
    const uint32_t mxcsr = rwD3DRoundBegin();
    const RwD3DCamVertex *v = rwD3DCamBase;
    const RwD3DCamVertex *pa = &v[a], *pb = &v[b], *pc = &v[c];
    const float cy = pc->y - pb->y;
    const float ax = pa->x - pb->x;
    const float cx = pc->x - pb->x;
    const float ay = pa->y - pb->y;
    const float az = pa->z - pb->z;
    const float cz = pc->z - pb->z;
    /* a . ((c - b) x (a - b)), summed y, z, x like the original. */
    const float ty = pa->y * (cx * az - ax * cz);
    const float tz = (cy * ax - cx * ay) * pa->z;
    const float tx = (cz * ay - cy * az) * pa->x;
    const float result = ty + tz + tx;

    rwD3DRoundEnd(mxcsr);
    return rwD3DBits(result) > 0 ? 0 : 1;
}

/* ---- the Im3D draw entries --------------------------------------------------------------------------------
 */

/* Grows the vertex buffers before a clip call when fewer than 0x14 vertices are left above the TL vertices.
 */
static void growForClip(void)
{
    if ((uint32_t)(rwD3DIm3DLevel->numTL + 0x14 + rwD3DIm3DLevel->base) > rwD3DBufferSize) {
        rwD3DIm3DGrow((uint32_t)rwD3DIm3DLevel->numTL + 0x32);
    }
}

/* 0x4f5f00 (EAX, EDX): one line: dropped when both ends are outside the same plane, clipped when one is out.
 */
BMX_ADDR(0x004f5f00)
__attribute__((regparm(2))) void rwD3DRenderLineIndices(int32_t a, int32_t b)
{
    const uint8_t ca = rwD3DCamBase[a].clip, cb = rwD3DCamBase[b].clip;

    if ((ca & cb) != 0) {
        return;
    }
    if ((ca | cb) == 0) {
        rwD3DIndexLine(a, b);
        return;
    }
    growForClip();
    rwD3DClipLine(a, b);
}

/* One triangle: dropped when outside one plane or culled, clipped when partly outside. */
static void renderTri(int32_t a, int32_t b, int32_t c)
{
    const uint8_t ca = rwD3DCamBase[a].clip, cb = rwD3DCamBase[b].clip, cc = rwD3DCamBase[c].clip;

    if ((cc & ca & cb) != 0) {
        return;
    }
    if (ca == 0 && cb == 0 && cc == 0) {
        if (rwD3DTriCull(a, b, c) != 1) {
            rwD3DIndexTri(a, b, c);
        }
        return;
    }
    if (rwD3DClipTriCull(a, b, c) == 1) {
        return;
    }
    growForClip();
    rwD3DClipTri(a, b, c);
}

/* 0x4f5f80 (EAX, EDX, ECX) */
BMX_ADDR(0x004f5f80)
__attribute__((regparm(3))) void rwD3DRenderTriIndices(int32_t a, int32_t b, int32_t c)
{
    renderTri(a, b, c);
}

/* 0x4f69b0 (device Im3D line). */
BMX_ADDR(0x004f69b0)
int rwD3DIm3DRenderLine(int32_t a, int32_t b)
{
    if (rwD3DIm3DLevel->andClip == 0) {
        rwD3DIndexStart();
        rwD3DRenderLineIndices(a, b);
        rwD3DIndexFlush();
    }
    return 1;
}

/* 0x4f6050 (device Im3D triangle). */
BMX_ADDR(0x004f6050)
int rwD3DIm3DRenderTriangle(int32_t a, int32_t b, int32_t c)
{
    if (rwD3DIm3DLevel->andClip == 0) {
        rwD3DIndexStart();
        rwD3DRenderTriIndices(a, b, c);
        rwD3DIndexFlush();
    }
    return 1;
}

/* One line of a primitive: like rwD3DRenderLineIndices. */
static void renderLine(int32_t a, int32_t b)
{
    const uint8_t ca = rwD3DCamBase[a].clip, cb = rwD3DCamBase[b].clip;

    if ((cb & ca) != 0) {
        return;
    }
    if (ca == 0 && cb == 0) {
        rwD3DIndexLine(a, b);
        return;
    }
    growForClip();
    rwD3DClipLine(a, b);
}

/*
 * 0x4f6090 (device Im3D primitive): the transformed vertices as line list (1), poly line (2), triangle list
 * (3), strip (4) or fan (5), through the index lists.
 */
BMX_ADDR(0x004f6090)
int rwD3DIm3DRenderPrimitive(int32_t primType)
{
    int32_t n;

    if (rwD3DIm3DLevel->andClip != 0) {
        return 1;
    }
    rwD3DIndexStart();
    n = rwD3DIm3DLevel->numVertices;
    switch (primType) {
    case 1:
        for (int32_t i = 0; n > 1; i += 2, n -= 2) {
            renderLine(i, i + 1);
        }
        break;
    case 2:
        for (int32_t i = 0; n > 1; i++, n--) {
            renderLine(i, i + 1);
        }
        break;
    case 3:
        for (int32_t i = 0; n > 2; i += 3, n -= 3) {
            renderTri(i, i + 1, i + 2);
        }
        break;
    case 4: {
        uint32_t odd = 1;
        for (int32_t i = 0; n > 2; i++, n--, odd ^= 3) {
            renderTri(i, (int32_t)odd + i, (int32_t)(odd ^ 3) + i);
        }
        break;
    }
    case 5:
        for (int32_t i = 1; n > 2; i++, n--) {
            renderTri(0, i, i + 1);
        }
        break;
    default:
        break;
    }
    rwD3DIndexFlush();
    return 1;
}

/* 0x4f64e0 (device Im3D indexed primitive): the same through an index array. */
BMX_ADDR(0x004f64e0)
int rwD3DIm3DRenderIndexedPrimitive(int32_t primType, const uint16_t *indices, int32_t numIndices)
{
    int32_t n = numIndices;

    if (rwD3DIm3DLevel->andClip != 0) {
        return 1;
    }
    rwD3DIndexStart();
    switch (primType) {
    case 1:
        for (; n > 1; indices += 2, n -= 2) {
            renderLine(indices[0], indices[1]);
        }
        break;
    case 2:
        for (; n > 1; indices++, n--) {
            renderLine(indices[0], indices[1]);
        }
        break;
    case 3:
        for (; n > 2; indices += 3, n -= 3) {
            renderTri(indices[0], indices[1], indices[2]);
        }
        break;
    case 4: {
        uint32_t odd = 1;
        for (n -= 2; n > 0; indices++, n--, odd ^= 3) {
            renderTri(indices[0], indices[odd], indices[odd ^ 3]);
        }
        break;
    }
    case 5: {
        const int32_t first = indices[0];
        for (n -= 2; n > 0; indices++, n--) {
            renderTri(first, indices[1], indices[2]);
        }
        break;
    }
    default:
        break;
    }
    rwD3DIndexFlush();
    return 1;
}
