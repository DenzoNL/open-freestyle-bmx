/*
 * RenderWare 3.1 maths and frames (rw/math.c): matrices, the frame hierarchy, vector transforms through the
 * engine's function table, bounding boxes and the frame list of a clump stream.
 *
 * The engine keeps its matrix and vector module data after the RwGlobals (plugin offsets rwMatrixOffset,
 * rwFastSqrtOffset, rwFrameOffset): free lists, the identity flag and the transform function pointers.
 */
#ifndef RW_RWMATH_H
#define RW_RWMATH_H

#include "rw/rwplugins.h"

/* How a matrix operation combines with the matrix: replace it, put the new operation first or last. */
typedef enum RwOpCombineType {
    rwCOMBINEREPLACE = 0,
    rwCOMBINEPRECONCAT = 1,
    rwCOMBINEPOSTCONCAT = 2,
} RwOpCombineType;

typedef ZVec3 RwV3d;

typedef RwV3d *(BMX_CDECL *RwV3dTransformFunc)(RwV3d *out, const RwV3d *in, int32_t count,
                                               const RwMatrix *matrix);

/* Plugin data of the engine instance at rwMatrixOffset. */
typedef struct RwMatrixModule {
    RwFreeList *freeList; /* 0x00 */
    uint32_t flagMask;    /* 0x04 tested against the identity bit (0x20000) of a matrix */
    void *multiply;       /* 0x08 matrix product used by the rotate code */
} RwMatrixModule;

/* Plugin data of the engine instance at rwFastSqrtOffset. */
typedef struct RwV3dModule {
    const uint32_t *sqrtTable;           /* 0x00 */
    const uint32_t *invSqrtTable;        /* 0x04 */
    RwV3dTransformFunc transformPoints;  /* 0x08 */
    RwV3dTransformFunc transformVectors; /* 0x0c */
} RwV3dModule;

#define rwFrameOffset ORIG_VAR(0x00676980, int32_t, rwFrameOffset)

/* The list of frames a clump stream creates: the array and its length. */
typedef struct RwFrameList {
    RwFrame **frames;
    int32_t numFrames;
} RwFrameList;

/* ---- matrices --------------------------------------------------------------------------------------- */

RwMatrix *RwMatrixCreate(void);
int RwMatrixDestroy(RwMatrix *matrix);
RwMatrix *RwMatrixRotate(RwMatrix *matrix, const RwV3d *axis, float angle, RwOpCombineType combine);
RwMatrix *RwMatrixScale(RwMatrix *matrix, const RwV3d *scale, RwOpCombineType combine);
RwMatrix *RwMatrixTranslate(RwMatrix *matrix, const RwV3d *translation, RwOpCombineType combine);
RwMatrix *RwMatrixInvert(RwMatrix *out, const RwMatrix *in);

/* Reciprocal square root of a float (given as its bits) from the engine's table; 0 for 0. */
double _rwFastInvSqrt(int32_t floatBits);
RwV3d *RwV3dTransformPoints(RwV3d *out, const RwV3d *in, int32_t count, const RwMatrix *matrix);
RwV3d *RwV3dTransformVectors(RwV3d *out, const RwV3d *in, int32_t count, const RwMatrix *matrix);

RwBBox *RwBBoxCalculate(RwBBox *bbox, const RwV3d *points, int32_t count);

/* ---- frames ----------------------------------------------------------------------------------------- */

RwFrame *RwFrameCreate(void);
int RwFrameDestroy(RwFrame *frame);
int RwFrameDestroyHierarchy(RwFrame *frame);
RwFrame *RwFrameUpdateObjects(RwFrame *frame);
RwMatrix *RwFrameGetMatrix(RwFrame *frame);
RwMatrix *RwFrameGetLTM(RwFrame *frame);
RwFrame *RwFrameForAllChildren(RwFrame *frame, RwObjectCallBack callback, void *data);
int32_t RwFrameCount(RwFrame *frame);
RwFrame *RwFrameTranslate(RwFrame *frame, const RwV3d *translation, RwOpCombineType combine);
RwFrame *RwFrameSetIdentity(RwFrame *frame);

/* Register-argument helpers (EAX, EDX): detach a frame from its parent; set a subtree's root. */
__attribute__((regparm(1))) RwFrame *RwFrameRemoveChild(RwFrame *child);
__attribute__((regparm(2))) void rwFrameSetRoot(RwFrame *frame, RwFrame *root);
/* Recompute the LTMs below a frame (EAX = frame, EDX = parent changed), and from a root. */
__attribute__((regparm(2))) void rwFrameSyncLTM(RwFrame *frame, int32_t force);
void rwFrameSyncHierarchyLTM(RwFrame *root);

RwFrameList *_rwFrameListStreamRead(void *stream, RwFrameList *frameList);
RwFrameList *_rwFrameListDeinitialize(RwFrameList *frameList);

/* TODO(rw/clump.c): belongs to the clump module. */
RpClump *RpClumpForAllAtomics(RpClump *clump, RwObjectCallBack callback, void *data);

#endif
