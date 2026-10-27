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
