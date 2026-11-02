/* RenderWare 3.1 immediate mode: render state queries, Im3D drawing entry points and the Im3D pipeline stack.
 */
#include "rw/rwim.h"

static RwIm3DGlobals *rwIm3DGlobals(void)
{
    return (RwIm3DGlobals *)((uint8_t *)RwEngineInstance + rwIm3DOffset);
}

/* Asks the device for a render state; it does not need a camera (unlike RwRenderStateSet). */
BMX_ADDR(0x004db870)
int RwRenderStateGet(int32_t state, void *value)
{
    return RwEngineInstance->dOpenDevice.fpRenderStateGet(state, value);
}

/* Remembers whether the shade mode is Gouraud, then draws the vertices of the last transform. */
BMX_ADDR(0x004def70)
int RwIm3DRenderPrimitive(int32_t primType)
{
    int32_t shadeMode = 0;
    RwRenderStateGet(rwRENDERSTATESHADEMODE, &shadeMode);
    rwIm3DGouraud = shadeMode == rwSHADEMODEGOURAUD;
    return RwEngineInstance->dOpenDevice.fpIm3DRenderPrimitive(primType);
}

BMX_ADDR(0x004defc0)
int RwIm3DRenderLine(int32_t from, int32_t to)
{
    int32_t shadeMode = 0;
    RwRenderStateGet(rwRENDERSTATESHADEMODE, &shadeMode);
    rwIm3DGouraud = shadeMode == rwSHADEMODEGOURAUD;
    return RwEngineInstance->dOpenDevice.fpIm3DRenderLine(from, to);
}

BMX_ADDR(0x004df020)
int RwIm3DRenderIndexedPrimitive(int32_t primType, const uint16_t *indices, int32_t numIndices)
{
    int32_t shadeMode = 0;
    RwRenderStateGet(rwRENDERSTATESHADEMODE, &shadeMode);
    rwIm3DGouraud = shadeMode == rwSHADEMODEGOURAUD;
    return RwEngineInstance->dOpenDevice.fpIm3DRenderIndexedPrimitive(primType, indices, numIndices);
}

/* Ends the Im3D transform started by RwIm3DTransform; 0 if the pipeline could not be popped. */
BMX_ADDR(0x004df090)
int RwIm3DEnd(void)
{
    return rwIm3DPipelinePop() != NULL;
}

/*
 * Transforms vertices through the engine's Im3D pipeline (optionally by a matrix); returns the vertices,
 * or NULL if the pipeline refused. A successful transform must be closed with RwIm3DEnd.
 */
BMX_ADDR(0x004df0b0)
void *RwIm3DTransform(void *vertices, int32_t numVertices, const RwMatrix *ltm, int32_t flags)
{
    void *result = NULL;
    RwIm3DPipeline *pipeline = *(RwIm3DPipeline **)((uint8_t *)RwEngineInstance + rwIm3DPipelineOffset);
    if (rwIm3DPipelinePush(pipeline) != NULL) {
        RwIm3DTransformInfo info;
        info.vertices = vertices;
        info.numVertices = numVertices;
        info.flags = flags;
        info.ltm = ltm;
        if (rwIm3DPipelineRun(&info) != NULL) {
            result = vertices;
        } else {
            rwIm3DPipelinePop();
        }
    }
    return result;
}

/* Pushes a pipeline on the stack and lets the device begin it; returns the pipeline or NULL. */
BMX_ADDR(0x004e85b0)
RwIm3DPipeline *rwIm3DPipelinePush(RwIm3DPipeline *pipeline)
{
    RwIm3DGlobals *globals = rwIm3DGlobals();
    globals->stack[globals->depth] = pipeline;
    rwIm3DGlobals()->depth++;
    return rwIm3DGlobals()->begin(pipeline) != 0 ? pipeline : NULL;
}

/* Runs the transform on the pipeline on top of the stack: instance the work, then each stage on it. */
BMX_ADDR(0x004e8600)
RwIm3DTransformInfo *rwIm3DPipelineRun(RwIm3DTransformInfo *info)
{
    if (info == NULL) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(rwERR_NULLP);
        RwErrorSet(&error);
        return NULL;
    }
    const RwIm3DGlobals *globals = rwIm3DGlobals();
    const RwIm3DPipeline *pipeline = globals->stack[globals->depth - 1];
    void *work = pipeline->instance(info);
    if (work != NULL) {
        pipeline->stage1(work);
        pipeline->stage2(work);
        pipeline->stage4(work, pipeline->stageData);
        pipeline->stage5(work);
        pipeline->stage6(work);
    }
    return info;
}

/* Lets the device end the pipeline on top of the stack and pops it; returns it, or NULL if the end failed. */
BMX_ADDR(0x004e8690)
RwIm3DPipeline *rwIm3DPipelinePop(void)
{
    if (rwIm3DGlobals()->end() == 0) {
        return NULL;
    }
    rwIm3DGlobals()->depth--;
    return rwIm3DGlobals()->stack[rwIm3DGlobals()->depth];
}

/* ---- modules: the Im3D pipeline stack and its default stages ---------------------------------------- */

#define rwRenderPipelineModuleCount ORIG_VAR(0x00676d74, int32_t, rwRenderPipelineModuleCount)
#define rwIm3DModuleCount           ORIG_VAR(0x00676d34, int32_t, rwIm3DModuleCount)

/* The default entries of the Im3D module: nothing to begin or end, 0 for "failed". */
BMX_ADDR(0x004e8390)
int rwIm3DDefaultBegin(RwIm3DPipeline *pipeline)
{
    return 0;
}

BMX_ADDR(0x004e83a0)
int rwIm3DDefaultEnd(void)
{
    return 0;
}

/* Sets the begin and end entries of the Im3D module (called by the driver). */
BMX_ADDR(0x004e83b0)
void rwIm3DSetEntries(int(BMX_CDECL *begin)(RwIm3DPipeline *pipeline), int(BMX_CDECL *end)(void))
{
    rwIm3DGlobals()->begin = begin;
    rwIm3DGlobals()->end = end;
}

/* Im3D module: the free list of pipelines (entries of 0x1c bytes) and the default entries. NULL on no memory.
 */
BMX_ADDR(0x004e83e0)
void *_rwRenderPipelineOpen(void *engine, int32_t offset)
{
    rwIm3DOffset = offset;
    rwIm3DGlobals()->pipelineFreeList = RwFreeListCreate(sizeof(RwIm3DPipeline), 5, 0);
    if (rwIm3DGlobals()->pipelineFreeList == NULL) {
        return NULL;
    }
    rwIm3DGlobals()->begin = BMX_FNPTR(RwIm3DBeginFunc, 0x004e8390, rwIm3DDefaultBegin);
    rwIm3DGlobals()->end = BMX_FNPTR(RwIm3DEndFunc, 0x004e83a0, rwIm3DDefaultEnd);
    rwIm3DGlobals()->depth = 0;
    rwRenderPipelineModuleCount++;
    return engine;
}

BMX_ADDR(0x004e8480)
void *_rwRenderPipelineClose(void *engine)
{
    if (rwIm3DGlobals()->pipelineFreeList != NULL) {
        RwFreeListDestroy(rwIm3DGlobals()->pipelineFreeList);
        rwIm3DGlobals()->pipelineFreeList = NULL;
    }
    rwRenderPipelineModuleCount--;
    return engine;
}

/* The default stages of a pipeline: the instance stage finds no work, the others do nothing. */
BMX_ADDR(0x004e84c0)
void *rwPipelineDefaultInstance(RwIm3DTransformInfo *info)
{
    return NULL;
}

BMX_ADDR(0x004e84d0)
void rwPipelineDefaultStage4(void *work, void *data) {}

BMX_ADDR(0x004e84e0)
void rwPipelineDefaultStageData(void) {}

BMX_ADDR(0x004e84f0)
void rwPipelineDefaultStage(void *work) {}

/* A new pipeline with the default stages, which the device then fills in; NULL if that fails. */
BMX_ADDR(0x004e8500)
RwIm3DPipeline *_rpWorldPipelineOpen(void)
{
    RwIm3DPipeline *pipeline = RwEngineInstance->freeListAlloc(rwIm3DGlobals()->pipelineFreeList);
    if (pipeline == NULL) {
        return NULL;
    }
    pipeline->instance = BMX_FNPTR(RwIm3DInstanceFunc, 0x004e84c0, rwPipelineDefaultInstance);
    pipeline->stage6 = BMX_FNPTR(RwIm3DStageFunc, 0x004e84f0, rwPipelineDefaultStage);
    pipeline->stage5 = BMX_FNPTR(RwIm3DStageFunc, 0x004e84f0, rwPipelineDefaultStage);
    pipeline->stage2 = BMX_FNPTR(RwIm3DStageFunc, 0x004e84f0, rwPipelineDefaultStage);
    pipeline->stage1 = BMX_FNPTR(RwIm3DStageFunc, 0x004e84f0, rwPipelineDefaultStage);
    pipeline->stage4 = BMX_FNPTR(RwIm3DStage4Func, 0x004e84d0, rwPipelineDefaultStage4);
    pipeline->stageData = BMX_FNPTR(void *, 0x004e84e0, rwPipelineDefaultStageData);
    if (_rwDeviceSystemRequest(&RwEngineInstance->dOpenDevice, rwDEVICESYSTEMPIPELINE, NULL, pipeline, 0) ==
        0) {
        return NULL;
    }
    return pipeline;
}

/* Gives a pipeline back to its free list; always 1. */
BMX_ADDR(0x004e8580)
int rwIm3DPipelineRelease(RwIm3DPipeline *pipeline)
{
    RwEngineInstance->freeListFree(rwIm3DGlobals()->pipelineFreeList, pipeline);
    return 1;
}

/* Calls entry 3 / entry 1 of the core extension's function table (0x69bee0) on a pipeline. */
BMX_ADDR(0x004e8700)
int rwCoreExtCallSlot3(void *pipeline)
{
    ((void(BMX_CDECL *)(void *))rwCoreExtGetFuncs()[3])(pipeline);
    return 1;
}

BMX_ADDR(0x004e8720)
void rwCoreExtCallSlot1(void *pipeline)
{
    ((void(BMX_CDECL *)(void *))rwCoreExtGetFuncs()[1])(pipeline);
}

/* Im3D pipeline module: the first pipeline, handed to the core extension. NULL if it cannot be made. */
BMX_ADDR(0x004dee70)
void *_rwIm3DOpen(void *engine, int32_t offset)
{
    rwIm3DPipelineOffset = offset;
    RwIm3DPipeline **slot = (RwIm3DPipeline **)((uint8_t *)RwEngineInstance + offset);
    *slot = _rpWorldPipelineOpen();
    if (*slot == NULL) {
        return NULL;
    }
    rwCoreExtCallSlot3(*slot);
    rwIm3DModuleCount++;
    return engine;
}

BMX_ADDR(0x004deec0)
void *_rwIm3DClose(void *engine)
{
    RwIm3DPipeline **slot = (RwIm3DPipeline **)((uint8_t *)RwEngineInstance + rwIm3DPipelineOffset);
    if (*slot != NULL) {
        rwIm3DPipelineRelease(*slot);
        *slot = NULL;
    }
    rwIm3DModuleCount--;
    return engine;
}
