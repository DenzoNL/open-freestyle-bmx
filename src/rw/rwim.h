/*
 * RenderWare 3.1 immediate mode (rw/im.c): the Im3D entry points and the pipeline stack behind them. Drawing
 * goes through the device driver's function table (RwDevice); the stack of Im3D pipelines is plugin data of
 * the engine instance.
 */
#ifndef RW_RWIM_H
#define RW_RWIM_H

#include "rw/rwcore.h"

/* What RwIm3DTransform hands the pipeline: the vertices to transform and how. */
typedef struct RwIm3DTransformInfo {
    void *vertices;      /* 0x00 */
    int32_t numVertices; /* 0x04 */
    int32_t flags;       /* 0x08 */
    const RwMatrix *ltm; /* 0x0c or NULL */
} RwIm3DTransformInfo;

/* An Im3D pipeline: seven entries called in this order on a transform (instance, then the stages). */
typedef struct RwIm3DPipeline {
    void *(BMX_CDECL *instance)(RwIm3DTransformInfo *info); /* 0x00 returns the work object or NULL */
    void(BMX_CDECL *stage1)(void *work);                    /* 0x04 */
    void(BMX_CDECL *stage2)(void *work);                    /* 0x08 */
    void *stageData;                                        /* 0x0c second argument of stage4 */
    void(BMX_CDECL *stage4)(void *work, void *data);        /* 0x10 */
    void(BMX_CDECL *stage5)(void *work);                    /* 0x14 */
    void(BMX_CDECL *stage6)(void *work);                    /* 0x18 */
} RwIm3DPipeline;

_Static_assert(sizeof(RwIm3DPipeline) == 0x1c, "RwIm3DPipeline layout");

/* The Im3D plugin's data in the engine instance: the device's begin/end entries and the pipeline stack. */
typedef struct RwIm3DGlobals {
    uint32_t pad00;                                  /* 0x00 */
    int(BMX_CDECL *begin)(RwIm3DPipeline *pipeline); /* 0x04 */
    int(BMX_CDECL *end)(void);                       /* 0x08 */
    int32_t depth;                                   /* 0x0c number of pipelines on the stack */
    RwIm3DPipeline *stack[1];                        /* 0x10 grows with depth */
} RwIm3DGlobals;

_Static_assert(offsetof(RwIm3DGlobals, stack) == 0x10, "RwIm3DGlobals layout");

/* Offsets of plugin data inside the engine instance. */
#define rwIm3DOffset         ORIG_VAR(0x00676d70, int32_t, rwIm3DOffset)
#define rwIm3DPipelineOffset ORIG_VAR(0x00676d30, int32_t, rwIm3DPipelineOffset)
/* 1 when the device's shade mode is Gouraud, refreshed by every Im3D draw call. */
#define rwIm3DGouraud ORIG_VAR(0x00696960, int32_t, rwIm3DGouraud)

enum { rwRENDERSTATESHADEMODE = 5, rwSHADEMODEGOURAUD = 2 };

int RwRenderStateGet(int32_t state, void *value);
int RwIm3DRenderPrimitive(int32_t primType);
int RwIm3DRenderLine(int32_t from, int32_t to);
int RwIm3DRenderIndexedPrimitive(int32_t primType, const uint16_t *indices, int32_t numIndices);
int RwIm3DEnd(void);
void *RwIm3DTransform(void *vertices, int32_t numVertices, const RwMatrix *ltm, int32_t flags);

RwIm3DPipeline *rwIm3DPipelinePush(RwIm3DPipeline *pipeline);
RwIm3DTransformInfo *rwIm3DPipelineRun(RwIm3DTransformInfo *info);
RwIm3DPipeline *rwIm3DPipelinePop(void);

#endif
