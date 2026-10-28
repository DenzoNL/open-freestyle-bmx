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
/* Rotation from a unit axis, sine and 1 - cosine: EAX = matrix, EDX = axis, ECX = combine, ST0 = 1 - cos and
 * the float sine as the fourth stack word (asm entry; NULL for a bad combine type). */
RwMatrix *RwMatrixRotateOneMinusCosineSine(uint32_t unused0, uint32_t unused1, uint32_t unused2, float sine);
RwMatrix *RwMatrixScale(RwMatrix *matrix, const RwV3d *scale, RwOpCombineType combine);
RwMatrix *RwMatrixTranslate(RwMatrix *matrix, const RwV3d *translation, RwOpCombineType combine);
RwMatrix *RwMatrixInvert(RwMatrix *out, const RwMatrix *in);

/* Reciprocal square root of a float (given as its bits) from the engine's table; 0 for 0. */
double _rwFastInvSqrt(int32_t floatBits);
/* Square root of a float (given as its bits) from the engine's table; 0 for 0. */
double _rwFastSqrt(int32_t floatBits);
/* Normalise in into out (they may be the same): the fast one returns 1 / length, RwV3dNormalize the length
 * (and reports an error for a zero vector). */
double _rwV3dNormalize(RwV3d *out, const RwV3d *in);
double RwV3dNormalize(RwV3d *out, const RwV3d *in);
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
/* Whether the frame's hierarchy waits for a sync. */
int RwFrameDirty(const RwFrame *frame);
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

/* Module open/close routines (rw/math.c) and the engine's default transforms. */
void *_rwFrameOpen(void *engine, int32_t offset);
void *_rwFrameClose(void *engine);
void rwMatrixMultiplyDefault(RwMatrix *out, const RwMatrix *a, const RwMatrix *b);
RwV3d *rwV3dTransformPointsDefault(RwV3d *out, const RwV3d *in, int32_t count, const RwMatrix *matrix);
RwV3d *rwV3dTransformVectorsDefault(RwV3d *out, const RwV3d *in, int32_t count, const RwMatrix *matrix);
/* Installs the point and vector transforms (EAX, EDX; NULL = the defaults). */
__attribute__((regparm(2))) int rwV3dSetTransformFuncs(RwV3dTransformFunc points, RwV3dTransformFunc vectors);
void rwV3dFreeSqrtTable(void);
void rwV3dFreeInvSqrtTable(void);
void *rwVectorOpen(void *engine, int32_t offset);
void *rwVectorClose(void *engine);
/* Stores a frame hierarchy into an array (EAX = cursor, EDX = frame) and builds the frame list of a clump. */
__attribute__((regparm(2))) int rwFrameListFill(RwFrame ***cursor, RwFrame *frame);
RwFrameList *_rwFrameListInitialize(RwFrameList *frameList, RwFrame *root);

/* In rw/clump.c: bounding boxes, matrix error measures (returned on the x87 stack), frame clones. */
void rwBBoxInitialize(RwBBox *bbox, const ZVec3 *point);
RwBBox *rwBBoxAddPoint(RwBBox *bbox, const ZVec3 *point);
double rwMatrixDeterminant(const RwMatrix *matrix);
double rwMatrixNormalError(const RwMatrix *matrix);
double rwMatrixOrthogonalError(const RwMatrix *matrix);
/* Destroys a frame and, recursively, everything below it (EAX = frame). */
__attribute__((regparm(1))) void rwFrameDestroyRecursive(RwFrame *frame);
__attribute__((regparm(2))) RwFrame *rwFrameCloneRecursive(RwFrame *frame, RwFrame *root);
RwFrame *RwFrameCloneHierarchy(RwFrame *frame);
void rwFrameResetRoot(RwFrame *frame);
RwFrame *RwFrameAddChild(RwFrame *parent, RwFrame *child);

/* In rw/clump.c (also declared in rwclump.h). */
RpClump *RpClumpForAllAtomics(RpClump *clump, RwObjectCallBack callback, void *data);

#endif
