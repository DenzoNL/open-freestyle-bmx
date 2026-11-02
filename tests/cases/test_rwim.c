/*
 * Tests for rw/im.c: render state query, the Im3D draw entry points and the Im3D pipeline stack. The device
 * driver and the pipeline stages are logging stubs; the engine instance is the fixture from dt_rw.h.
 */
#include "dt.h"
#include "dt_rw.h"
#include "rw/rwim.h"

static int32_t gShade; /* what the device reports as the shade mode */
static uint32_t gOk;   /* bit n: the n-th begin/end/instance call succeeds */
static int gCalls;     /* begin/end/instance calls so far */
static void *gWork;    /* what a pipeline's instance returns when it succeeds */
static RwMatrix *gMatrix;

static int BMX_CDECL stGet(int32_t state, void *value)
{
    rwtLog(0x24);
    rwtLog((uint32_t)state);
    rwtLogPtr(value);
    *(int32_t *)value = gShade;
    return (int)(dt_u32() & 0xff);
}

static int BMX_CDECL stPrimitive(int32_t type)
{
    rwtLog(0x40);
    rwtLog((uint32_t)type);
    return (int)(dt_u32() & 0xff);
}

static int BMX_CDECL stLine(int32_t from, int32_t to)
{
    rwtLog(0x38);
    rwtLog((uint32_t)from);
    rwtLog((uint32_t)to);
    return (int)(dt_u32() & 0xff);
}

static int BMX_CDECL stIndexed(int32_t type, const uint16_t *indices, int32_t count)
{
    rwtLog(0x44);
    rwtLog((uint32_t)type);
    rwtLogPtr(indices);
    rwtLog((uint32_t)count);
    return (int)(dt_u32() & 0xff);
}

static int BMX_CDECL stBegin(RwIm3DPipeline *pipeline)
{
    rwtLog(0x04);
    rwtLogPtr(pipeline);
    rwtLog((uint32_t)rwIm3DOffset != 0);
    return (gOk >> gCalls++) & 1;
}

static int BMX_CDECL stEnd(void)
{
    rwtLog(0x08);
    return (gOk >> gCalls++) & 1;
}

static void *BMX_CDECL stInstance(RwIm3DTransformInfo *info)
{
    rwtLog(0x50);
    rwtLogPtr(info->vertices);
    rwtLog((uint32_t)info->numVertices);
    rwtLog((uint32_t)info->flags);
    rwtLogPtr(info->ltm);
    return (gOk >> gCalls++) & 1 ? gWork : NULL;
}

static void BMX_CDECL stStage1(void *work)
{
    rwtLog(0x51);
    rwtLogPtr(work);
}

static void BMX_CDECL stStage2(void *work)
{
    rwtLog(0x52);
    rwtLogPtr(work);
}

static void BMX_CDECL stStage4(void *work, void *data)
{
    rwtLog(0x54);
    rwtLogPtr(work);
    rwtLogPtr(data);
}

static void BMX_CDECL stStage5(void *work)
{
    rwtLog(0x55);
    rwtLogPtr(work);
}

static void BMX_CDECL stStage6(void *work)
{
    rwtLog(0x56);
    rwtLogPtr(work);
}

static RwIm3DPipeline *makePipeline(void)
{
    RwIm3DPipeline *p = dt_alloc(sizeof *p);
    p->instance = stInstance;
    p->stage1 = stStage1;
    p->stage2 = stStage2;
    p->stageData = dt_alloc(4);
    p->stage4 = stStage4;
    p->stage5 = stStage5;
    p->stage6 = stStage6;
    return p;
}

/* An engine with the device stubs, the Im3D plugin data (stack depth 0..max) and the default pipeline. */
static RwIm3DGlobals *imEnv(DtCtx *c, int maxDepth, int minDepth)
{
    dt_seed(c);
    RwGlobals *g = rwtEnv();
    g->dOpenDevice.fpRenderStateGet = stGet;
    g->dOpenDevice.fpIm3DRenderPrimitive = stPrimitive;
    g->dOpenDevice.fpIm3DRenderLine = stLine;
    g->dOpenDevice.fpIm3DRenderIndexedPrimitive = stIndexed;
    RwIm3DGlobals *im = dt_alloc(sizeof *im - sizeof(void *));
    im->begin = stBegin;
    im->end = stEnd;
    im->depth = dt_range(minDepth, maxDepth);
    for (int i = 0; i < 8; i++) {
        im->stack[i] = makePipeline();
    }
    rwIm3DOffset = (int32_t)((uintptr_t)im - (uintptr_t)g);
    void **slot = (void **)dt_alloc(8);
    *slot = makePipeline();
    rwIm3DPipelineOffset = (int32_t)((uintptr_t)slot - (uintptr_t)g);
    rwIm3DGouraud = (int32_t)dt_u32();
    gShade = dt_u32() % 2 ? rwSHADEMODEGOURAUD : dt_range(0, 4);
    gOk = dt_u32() % 3 == 0 ? dt_u32() : 0xffffffffu;
    gCalls = 0;
    gWork = dt_alloc(16);
    gMatrix = dt_alloc(sizeof *gMatrix);
    return im;
}

static void setup_get(DtCtx *c)
{
    imEnv(c, 0, 0);
    DT_ARG_INT(c, 0, dt_range(0, 30));
    DT_ARG_PTR(c, 1, dt_alloc(8));
    rwtEnd();
}

static void setup_primitive(DtCtx *c)
{
    imEnv(c, 0, 0);
    DT_ARG_INT(c, 0, dt_range(0, 6));
    rwtEnd();
}

static void setup_line(DtCtx *c)
{
    imEnv(c, 0, 0);
    DT_ARG_INT(c, 0, dt_range(0, 100));
    DT_ARG_INT(c, 1, dt_range(0, 100));
    rwtEnd();
}

static void setup_indexed(DtCtx *c)
{
    imEnv(c, 0, 0);
    DT_ARG_INT(c, 0, dt_range(0, 6));
    DT_ARG_PTR(c, 1, dt_alloc(32));
    DT_ARG_INT(c, 2, dt_range(0, 60));
    rwtEnd();
}

static void setup_end(DtCtx *c)
{
    imEnv(c, 4, 1);
    rwtEnd();
}

static void setup_transform(DtCtx *c)
{
    imEnv(c, 3, 0);
    DT_ARG_PTR(c, 0, dt_alloc(64));
    DT_ARG_INT(c, 1, dt_range(0, 20));
    DT_ARG_PTR(c, 2, dt_u32() % 2 ? gMatrix : NULL);
    DT_ARG_INT(c, 3, dt_u32() % 4);
    rwtEnd();
}

static void setup_push(DtCtx *c)
{
    imEnv(c, 5, 0);
    DT_ARG_PTR(c, 0, makePipeline());
    rwtEnd();
}

static void setup_run(DtCtx *c)
{
    imEnv(c, 5, 1);
    RwIm3DTransformInfo *info = dt_alloc(sizeof *info);
    info->vertices = dt_alloc(16);
    info->numVertices = dt_range(0, 9);
    info->flags = (int32_t)dt_u32();
    info->ltm = dt_u32() % 2 ? gMatrix : NULL;
    DT_ARG_PTR(c, 0, dt_u32() % 6 == 0 ? NULL : info);
    rwtEnd();
}

DT_CASES(rwim_cases) = {
    {"RwRenderStateGet", 0x004db870, RwRenderStateGet, 2, DT_RET_INT, setup_get, 50, 0},
    {"RwIm3DRenderPrimitive", 0x004def70, RwIm3DRenderPrimitive, 1, DT_RET_INT, setup_primitive, 100, 0},
    {"RwIm3DRenderLine", 0x004defc0, RwIm3DRenderLine, 2, DT_RET_INT, setup_line, 100, 0},
    {"RwIm3DRenderIndexedPrimitive", 0x004df020, RwIm3DRenderIndexedPrimitive, 3, DT_RET_INT, setup_indexed,
     100, 0},
    {"RwIm3DEnd", 0x004df090, RwIm3DEnd, 0, DT_RET_INT, setup_end, 100, 0},
    {"RwIm3DTransform", 0x004df0b0, RwIm3DTransform, 4, DT_RET_INT, setup_transform, 400, 0},
    {"rwIm3DPipelinePush", 0x004e85b0, rwIm3DPipelinePush, 1, DT_RET_INT, setup_push, 150, 0},
    {"rwIm3DPipelineRun", 0x004e8600, rwIm3DPipelineRun, 1, DT_RET_INT, setup_run, 300, 0},
    {"rwIm3DPipelinePop", 0x004e8690, rwIm3DPipelinePop, 0, DT_RET_INT, setup_end, 150, 0},
};
