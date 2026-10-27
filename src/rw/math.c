/* RenderWare 3.1 maths: matrices, the frame hierarchy, vector transforms, bounding boxes, frame lists. */
#include "compat/crt.h"
#include "compat/x87.h"
#include "rw/rwmath.h"

/* Callees in other RenderWare groups, still original code (or another slice's). */
ORIG_FN(0x004e2800, int32_t, RwStreamRead, (void *stream, void *buffer, uint32_t length));
ORIG_FN(0x004e2dc0, void *, RwMemNative32, (void *mem, int32_t size));
ORIG_FN(0x004e2e00, void *, RwMemFloat32ToReal, (void *mem, int32_t size));
ORIG_FN(0x004e2ec0, int, RwStreamFindChunk,
        (void *stream, uint32_t type, uint32_t *length, uint32_t *version));
/* Error measures of a matrix (they return on the x87 stack): the rows' length, their skew, the determinant.
 */
ORIG_FN(0x004d2190, double, rwMatrixNormalError, (const RwMatrix *matrix));
ORIG_FN(0x004d2210, double, rwMatrixOrthogonalError, (const RwMatrix *matrix));
ORIG_FN(0x004d0470, double, rwMatrixDeterminant, (const RwMatrix *matrix));
ORIG_FN(0x004d2b50, RwFrame *, RwFrameAddChild, (RwFrame * parent, RwFrame *child));
/* Destroys a frame and, recursively, everything below it (EAX = frame). */
ORIG_FN_REGPARM(1, 0x004d2530, void, rwFrameDestroyRecursive, (RwFrame * frame));

enum {
    rwERR_NOPARENT = 0x11,
    rwMATRIXTYPEMASK = 0x00000003,
    rwMATRIXINTERNALIDENTITY = 0x00020000,
};

static RwMatrixModule *rwMatrixModule(void)
{
    return (RwMatrixModule *)((uint8_t *)RwEngineInstance + rwMatrixOffset);
}

static RwV3dModule *rwV3dModule(void)
{
    return (RwV3dModule *)((uint8_t *)RwEngineInstance + rwFastSqrtOffset);
}

static RwFreeList *rwFrameFreeList(void)
{
    return *(RwFreeList **)((uint8_t *)RwEngineInstance + rwFrameOffset);
}

static void rwReportError(int32_t code, const char *message)
{
    RwError error;
    error.pluginID = 1;
    error.errorCode = _rwerror(code, message);
    RwErrorSet(&error);
}

/* The exact product of two floats (a double holds it without rounding). */
static double rwMul(float a, float b)
{
    return (double)a * (double)b;
}

/* ---- matrices --------------------------------------------------------------------------------------- */

/* Allocates an identity matrix from the matrix free list (NULL if it is exhausted). */
BMX_ADDR(0x004d1fe0)
RwMatrix *RwMatrixCreate(void)
{
    RwMatrix *matrix = RwEngineInstance->freeListAlloc(rwMatrixModule()->freeList);
    if (matrix != NULL) {
        matrix->right.x = 1.0f;
        matrix->up.y = 1.0f;
        matrix->at.z = 1.0f;
        matrix->up.x = 0.0f;
        matrix->right.z = 0.0f;
        matrix->right.y = 0.0f;
        matrix->at.y = 0.0f;
        matrix->at.x = 0.0f;
        matrix->up.z = 0.0f;
        matrix->pos.z = 0.0f;
        matrix->pos.y = 0.0f;
        matrix->pos.x = 0.0f;
        matrix->flags = 0x20003;
        ((uint8_t *)&matrix->pad1)[0] = 0;
    }
    return matrix;
}

BMX_ADDR(0x004d1fb0)
int RwMatrixDestroy(RwMatrix *matrix)
{
    RwEngineInstance->freeListFree(rwMatrixModule()->freeList, matrix);
    return 1;
}

/* Sets the matrix to the identity (flags: identity, orthonormal) with all other padding left alone. */
static void rwMatrixSetIdentity(RwMatrix *m)
{
    m->pos.z = 0.0f;
    m->flags |= 0x20003;
    m->pos.x = 0.0f;
    m->pos.y = 0.0f;
    m->up.z = 0.0f;
    m->at.x = 0.0f;
    m->at.y = 0.0f;
    m->right.y = 0.0f;
    m->right.z = 0.0f;
    m->up.x = 0.0f;
    m->right.x = 1.0f;
    m->up.y = 1.0f;
    m->at.z = 1.0f;
}

/* Scales the matrix (or replaces it by a scale); the identity and orthonormal flags are dropped. */
BMX_ADDR(0x004d1400)
RwMatrix *RwMatrixScale(RwMatrix *matrix, const RwV3d *scale, RwOpCombineType combine)
{
    if (combine == rwCOMBINEREPLACE) {
        rwMatrixSetIdentity(matrix);
        matrix->right.x = scale->x;
        matrix->up.y = scale->y;
        matrix->at.z = scale->z;
    } else if (combine == rwCOMBINEPRECONCAT) {
        matrix->right.x = (float)rwMul(matrix->right.x, scale->x);
        matrix->right.y = (float)rwMul(matrix->right.y, scale->x);
        matrix->right.z = (float)rwMul(matrix->right.z, scale->x);
        matrix->up.x = (float)rwMul(matrix->up.x, scale->y);
        matrix->up.y = (float)rwMul(matrix->up.y, scale->y);
        matrix->up.z = (float)rwMul(matrix->up.z, scale->y);
        matrix->at.x = (float)rwMul(matrix->at.x, scale->z);
        matrix->at.y = (float)rwMul(matrix->at.y, scale->z);
        matrix->at.z = (float)rwMul(matrix->at.z, scale->z);
    } else if (combine == rwCOMBINEPOSTCONCAT) {
        matrix->right.x = (float)rwMul(matrix->right.x, scale->x);
        matrix->right.y = (float)rwMul(matrix->right.y, scale->y);
        matrix->right.z = (float)rwMul(matrix->right.z, scale->z);
        matrix->up.x = (float)rwMul(matrix->up.x, scale->x);
        matrix->up.y = (float)rwMul(matrix->up.y, scale->y);
        matrix->up.z = (float)rwMul(matrix->up.z, scale->z);
        matrix->at.x = (float)rwMul(matrix->at.x, scale->x);
        matrix->at.y = (float)rwMul(matrix->at.y, scale->y);
        matrix->at.z = (float)rwMul(matrix->at.z, scale->z);
        matrix->pos.x = (float)rwMul(matrix->pos.x, scale->x);
        matrix->pos.y = (float)rwMul(matrix->pos.y, scale->y);
        matrix->pos.z = (float)rwMul(matrix->pos.z, scale->z);
    } else {
        /* The original goes on to touch the (now null) matrix and crashes; here it returns. */
        rwReportError(rwERR_INVALIDSIZE, "Invalid combination type specified");
        return NULL;
    }
    matrix->flags &= 0xfffdfffc;
    ((uint8_t *)&matrix->pad1)[0]++;
    return matrix;
}

/*
 * Translates the matrix (or replaces it by a translation). Pre-concatenation adds the translation
 * transformed by the matrix's rotation; the sums are accumulated in the original's order, in double
 * precision.
 */
BMX_ADDR(0x004d15a0)
RwMatrix *RwMatrixTranslate(RwMatrix *matrix, const RwV3d *translation, RwOpCombineType combine)
{
    if (combine == rwCOMBINEREPLACE) {
        rwMatrixSetIdentity(matrix);
        matrix->pos.x = translation->x;
        matrix->pos.y = translation->y;
        matrix->pos.z = translation->z;
    } else if (combine == rwCOMBINEPRECONCAT) {
        const RwV3d t = *translation;
        const double x =
            ((rwMul(t.z, matrix->at.x) + rwMul(matrix->up.x, t.y)) + rwMul(matrix->right.x, t.x)) +
            (double)matrix->pos.x;
        matrix->pos.x = (float)x;
        const double y =
            ((rwMul(matrix->at.y, t.z) + rwMul(matrix->up.y, t.y)) + rwMul(matrix->right.y, t.x)) +
            (double)matrix->pos.y;
        matrix->pos.y = (float)y;
        const double z =
            ((rwMul(matrix->at.z, t.z) + rwMul(matrix->up.z, t.y)) + rwMul(matrix->right.z, t.x)) +
            (double)matrix->pos.z;
        matrix->pos.z = (float)z;
    } else if (combine == rwCOMBINEPOSTCONCAT) {
        matrix->pos.x = (float)((double)matrix->pos.x + (double)translation->x);
        matrix->pos.y = (float)((double)matrix->pos.y + (double)translation->y);
        matrix->pos.z = (float)((double)matrix->pos.z + (double)translation->z);
    } else {
        rwReportError(rwERR_INVALIDSIZE, "Invalid combination type specified");
        return NULL;
    }
    matrix->flags &= 0xfffdffff;
    ((uint8_t *)&matrix->pad1)[0]++;
    return matrix;
}

/* Inverse of a matrix: a copy for the identity, else by the orthonormal or the general routine. */
BMX_ADDR(0x004d1340)
RwMatrix *RwMatrixInvert(RwMatrix *out, const RwMatrix *in)
{
    const uint32_t identity = rwMatrixModule()->flagMask & rwMATRIXINTERNALIDENTITY;
    if ((in->flags & identity) != 0) {
        *out = *in;
    } else if ((in->flags & rwMATRIXTYPEMASK) == rwMATRIXTYPEMASK) {
        RwMatrixInvertOrthonormal(out, in);
    } else {
        RwMatrixInvertGeneral(out, in);
    }
    return out;
}

/*
 * The FPU work of RwMatrixRotate: the axis scaled by the table based 1 / length (floats, as the original
 * stores them), sin(angle) as a float and 1 - cos(angle) as a double (sin and cos are the x87 instructions).
 * They go into the slots of the original's stack frame, where RwMatrixRotate's asm stub finds them.
 */
__attribute__((used, noinline)) static void rotatePrepare(const RwV3d *axis, float angle, uint8_t *frame)
{
    const double radians = rwMul(0x1.1df46ap-6f, angle);
    const float lengthSquared =
        (float)((rwMul(axis->z, axis->z) + rwMul(axis->y, axis->y)) + rwMul(axis->x, axis->x));
    uint32_t bits;
    memcpy(&bits, &lengthSquared, sizeof bits);
    const double scale = _rwFastInvSqrt((int32_t)bits);
    float *unit = (float *)(frame + 0x18);
    unit[0] = (float)((double)axis->x * scale);
    unit[1] = (float)((double)axis->y * scale);
    unit[2] = (float)((double)axis->z * scale);
    const double sine = bmxX87Fsin(radians);
    const double cosine = bmxX87Fcos(radians);
    *(float *)(frame + 0xc) = (float)sine;
    *(double *)(frame + 0x28) = (double)1.0f - cosine;
}

/*
 * Rotates the matrix by angle degrees around axis. The work is done by RwMatrixRotateOneMinusCosineSine
 * (0x4d0a00, not ported yet): EAX = matrix, EDX = the normalised axis, ECX = combine, ST0 = 1 - cos(angle)
 * and the float sin(angle) as the fourth word of its stack arguments; it leaves the x87 stack empty. That
 * routine copies three words of its own, uninitialised stack frame into the matrix padding (and hands them to
 * the multiply routine); they are cleared here so the padding does not depend on stale stack contents.
 * This function keeps the original's frame (EBX saved, 0x40 bytes of locals) so the callee runs at the same
 * stack depth. TODO: plain C once 0x4d0a00 is ported.
 */
BMX_ADDR(0x004d1040)
__attribute__((naked)) RwMatrix *RwMatrixRotate(RwMatrix *matrix, const RwV3d *axis, float angle,
                                                RwOpCombineType combine)
{
    __asm__ volatile("pushl %ebx\n\t"
                     "subl $0x40, %esp\n\t"
                     "movl 0x48(%esp), %ebx\n\t"
                     "pushl %esp\n\t"
                     "pushl 0x54(%esp)\n\t"
                     "pushl 0x54(%esp)\n\t"
                     "call _rotatePrepare\n\t"
                     "addl $12, %esp\n\t"
                     "fldl 0x28(%esp)\n\t"
                     "movl 0x54(%esp), %ecx\n\t"
                     "movl %ebx, %eax\n\t"
                     "leal 0x18(%esp), %edx\n\t"
                     "movl $0, -0x94(%esp)\n\t"
                     "movl $0, -0x84(%esp)\n\t"
                     "movl $0, -0x74(%esp)\n\t"
                     "movl %esi, 0x3c(%esp)\n\t"
                     "movl $0x004d0a00, %esi\n\t"
                     "call *%esi\n\t"
                     "movl 0x3c(%esp), %esi\n\t"
                     "addl $0x40, %esp\n\t"
                     "movl %ebx, %eax\n\t"
                     "popl %ebx\n\t"
                     "ret");
}

/*
 * 1 / sqrt(x) for a float given as its bits: the table is indexed by the top mantissa bits (after a rounding
 * offset) and the halved exponent is added; 0 maps to 0.
 */
BMX_ADDR(0x004d38d0)
double _rwFastInvSqrt(int32_t floatBits)
{
    uint32_t bits = (uint32_t)floatBits;
    if (bits != 0) {
        const uint32_t *table = rwV3dModule()->invSqrtTable;
        bits += 0x800;
        bits = table[(bits & 0xffffff) >> 12] + (((bits ^ 0xffffffffu) & 0x7f800000) >> 1);
    }
    float result;
    memcpy(&result, &bits, sizeof result);
    return result;
}

/* Transforms points and vectors through the engine's function table (the transform routines are the
 * module's). */
BMX_ADDR(0x004d3a90)
RwV3d *RwV3dTransformPoints(RwV3d *out, const RwV3d *in, int32_t count, const RwMatrix *matrix)
{
    return rwV3dModule()->transformPoints(out, in, count, matrix);
}

BMX_ADDR(0x004d3ae0)
RwV3d *RwV3dTransformVectors(RwV3d *out, const RwV3d *in, int32_t count, const RwMatrix *matrix)
{
    return rwV3dModule()->transformVectors(out, in, count, matrix);
}

/*
 * Bounding box of count points (count >= 1; the original loops 2^32 times for 0). Comparisons are those of
 * the x87: NaN never lowers inf but does replace sup.
 */
BMX_ADDR(0x004e9760)
RwBBox *RwBBoxCalculate(RwBBox *bbox, const RwV3d *points, int32_t count)
{
    bbox->inf = points[0];
    bbox->sup = points[0];
    const RwV3d *point = points + 1;
    for (uint32_t left = (uint32_t)count - 1; left != 0; left--, point++) {
        if (bbox->inf.x > point->x) {
            bbox->inf.x = point->x;
        }
        if (bbox->inf.y > point->y) {
            bbox->inf.y = point->y;
        }
        if (bbox->inf.z > point->z) {
            bbox->inf.z = point->z;
        }
        if (!(bbox->sup.x >= point->x)) {
            bbox->sup.x = point->x;
        }
        if (!(bbox->sup.y >= point->y)) {
            bbox->sup.y = point->y;
        }
        if (!(bbox->sup.z >= point->z)) {
            bbox->sup.z = point->z;
        }
    }
    return bbox;
}

/* ---- frames ----------------------------------------------------------------------------------------- */

/*
 * Marks the frame and its subtree as changed and queues the hierarchy's root in the engine's dirty list
 * (unless it is queued already).
 */
static void rwFrameMarkDirty(RwFrame *frame)
{
    frame->object.privateFlags |= rwFRAMEPRIVATESUBTREESYNCLTM | rwFRAMEPRIVATESUBTREESYNCOBJ;
    RwFrame *root = frame->root;
    const uint8_t flags = root->object.privateFlags;
    if ((flags & 3) == 0) {
        RwLLLink *head = &RwEngineInstance->dirtyFrameList;
        root->dirtyLink.next = head->next;
        root->dirtyLink.prev = head;
        head->next->prev = &root->dirtyLink;
        head->next = &root->dirtyLink;
    }
    root->object.privateFlags = flags | 3;
}

/* Takes the frame out of the dirty list if it is queued. */
static void rwFrameUnlinkDirty(RwFrame *frame)
{
    if ((frame->object.privateFlags & 3) != 0) {
        frame->dirtyLink.prev->next = frame->dirtyLink.next;
        frame->dirtyLink.next->prev = frame->dirtyLink.prev;
    }
}

BMX_ADDR(0x004d2870)
RwFrame *RwFrameCreate(void)
{
    RwFrame *frame = RwEngineInstance->freeListAlloc(rwFrameFreeList());
    if (frame == NULL) {
        return NULL;
    }
    frame->modelling.up.x = 0.0f;
    frame->modelling.right.z = 0.0f;
    frame->modelling.right.y = 0.0f;
    frame->modelling.at.y = 0.0f;
    frame->modelling.at.x = 0.0f;
    frame->modelling.up.z = 0.0f;
    frame->modelling.pos.z = 0.0f;
    frame->modelling.pos.y = 0.0f;
    frame->modelling.pos.x = 0.0f;
    frame->version = 0;
    frame->next = NULL;
    frame->child = NULL;
    frame->object.parent = NULL;
    frame->object.privateFlags = 0;
    frame->object.flags = 0;
    frame->object.subType = 0;
    frame->object.type = 0;
    frame->modelling.at.z = 1.0f;
    frame->modelling.up.y = 1.0f;
    frame->modelling.right.x = 1.0f;
    frame->objectList.next = &frame->objectList;
    frame->objectList.prev = &frame->objectList;
    frame->root = frame;
    frame->ltm.flags = 3;
    frame->modelling.flags = 0x20003;
    _rwPluginRegistryInitObject(&rwFrameRegistry, frame);
    return frame;
}

/*
 * Detaches the frame from its parent (a frame without a parent is a no-op for the hierarchy), unqueues it
 * and releases it. Its children become parentless; they are not destroyed.
 */
BMX_ADDR(0x004d2920)
int RwFrameDestroy(RwFrame *frame)
{
    _rwPluginRegistryDeInitObject(&rwFrameRegistry, frame);
    if (frame->object.parent != NULL) {
        RwFrameRemoveChild(frame);
    }
    rwFrameUnlinkDirty(frame);
    for (RwFrame *child = frame->child; child != NULL;) {
        RwFrame *next = child->next;
        child->object.parent = NULL;
        child = next;
    }
    RwEngineInstance->freeListFree(rwFrameFreeList(), frame);
    return 1;
}

/* RwFrameDestroy for a frame and everything below it. */
BMX_ADDR(0x004d29c0)
int RwFrameDestroyHierarchy(RwFrame *frame)
{
    for (RwFrame *child = frame->child; child != NULL;) {
        RwFrame *next = child->next;
        rwFrameDestroyRecursive(child);
        child = next;
    }
    _rwPluginRegistryDeInitObject(&rwFrameRegistry, frame);
    if (frame->object.parent != NULL) {
        RwFrameRemoveChild(frame);
    }
    rwFrameUnlinkDirty(frame);
    for (RwFrame *child = frame->child; child != NULL;) {
        RwFrame *next = child->next;
        child->object.parent = NULL;
        child = next;
    }
    RwEngineInstance->freeListFree(rwFrameFreeList(), frame);
    return 1;
}

/* Marks a frame changed (see rwFrameMarkDirty); returns it. */
BMX_ADDR(0x004d2a80)
RwFrame *RwFrameUpdateObjects(RwFrame *frame)
{
    rwFrameMarkDirty(frame);
    return frame;
}

BMX_ADDR(0x004d2b00)
RwMatrix *RwFrameGetMatrix(RwFrame *frame)
{
    return &frame->modelling;
}

/* The frame's world matrix; the hierarchy is synchronised first if its root says so. */
BMX_ADDR(0x004d2b10)
RwMatrix *RwFrameGetLTM(RwFrame *frame)
{
    RwFrame *root = frame->root;
    if ((root->object.privateFlags & rwFRAMEPRIVATEHIERARCHYSYNCLTM) != 0) {
        rwFrameSyncHierarchyLTM(root);
    }
    return &frame->ltm;
}

/* Sets the root of a subtree and bumps the versions on the way (EAX = frame, EDX = root). */
BMX_ADDR(0x004d2700)
__attribute__((regparm(2))) void rwFrameSetRoot(RwFrame *frame, RwFrame *root)
{
    frame->root = root;
    frame->version++;
    for (RwFrame *child = frame->child; child != NULL; child = child->next) {
        rwFrameSetRoot(child, root);
    }
}

/*
 * Detaches a frame from its parent: it becomes the root of its own hierarchy, both hierarchies are queued
 * for a sync. Returns the frame, or NULL (with an error) if it has no parent. EAX = frame.
 */
BMX_ADDR(0x004d2c64)
__attribute__((regparm(1))) RwFrame *RwFrameRemoveChild(RwFrame *child)
{
    RwFrame *parent = child->object.parent;
    if (parent == NULL) {
        rwReportError(rwERR_NOPARENT, NULL);
        return NULL;
    }
    parent->version++;
    const uint8_t version = child->version;
    RwFrame *oldRoot = child->root;
    child->root = child;
    child->version = (uint8_t)(version + 2);
    for (RwFrame *grandChild = child->child; grandChild != NULL; grandChild = grandChild->next) {
        rwFrameSetRoot(grandChild, child);
    }
    rwFrameMarkDirty(child);
    RwFrame *first = parent->child;
    if (first == child) {
        parent->child = child->next;
        child->object.parent = NULL;
        rwFrameMarkDirty(oldRoot);
        child->next = NULL;
        return child;
    }
    RwFrame *previous = first;
    while (child != previous->next) {
        previous = previous->next;
    }
    previous->next = child->next;
    child->next = NULL;
    child->object.parent = NULL;
    rwFrameMarkDirty(oldRoot);
    return child;
}

BMX_ADDR(0x004d2eb0)
RwFrame *RwFrameForAllChildren(RwFrame *frame, RwObjectCallBack callback, void *data)
{
    RwFrame *child = frame->child;
    while (child != NULL) {
        RwFrame *next = child->next;
        if (callback(child, data) == NULL) {
            break;
        }
        child = next;
    }
    return frame;
}

/* Number of frames in the subtree, the frame itself included. */
BMX_ADDR(0x004d2f00)
int32_t RwFrameCount(RwFrame *frame)
{
    int32_t count = 1;
    for (RwFrame *child = frame->child; child != NULL; child = child->next) {
        count += RwFrameCount(child);
    }
    return count;
}

BMX_ADDR(0x004d2f40)
RwFrame *RwFrameTranslate(RwFrame *frame, const RwV3d *translation, RwOpCombineType combine)
{
    RwMatrixTranslate(&frame->modelling, translation, combine);
    rwFrameMarkDirty(frame);
    frame->version++;
    return frame;
}

BMX_ADDR(0x004d3200)
RwFrame *RwFrameSetIdentity(RwFrame *frame)
{
    frame->object.privateFlags |= rwFRAMEPRIVATESUBTREESYNCLTM | rwFRAMEPRIVATESUBTREESYNCOBJ;
    frame->modelling.flags |= 0x20003;
    frame->modelling.pos.x = 0.0f;
    frame->modelling.pos.y = 0.0f;
    frame->modelling.pos.z = 0.0f;
    frame->modelling.up.z = 0.0f;
    frame->modelling.at.x = 0.0f;
    frame->modelling.at.y = 0.0f;
    frame->modelling.right.y = 0.0f;
    frame->modelling.right.z = 0.0f;
    frame->modelling.up.x = 0.0f;
    frame->modelling.right.x = 1.0f;
    frame->modelling.up.y = 1.0f;
    frame->modelling.at.z = 1.0f;
    rwFrameMarkDirty(frame);
    frame->version++;
    return frame;
}

/*
 * Recomputes the world matrices of a frame list and its subtrees: a frame is updated if it is marked or
 * force is non-zero (EAX = frame, EDX = force); siblings follow with the caller's force.
 */
BMX_ADDR(0x004ee4b0)
__attribute__((regparm(2))) void rwFrameSyncLTM(RwFrame *frame, int32_t force)
{
    if (frame == NULL) {
        return;
    }
    const uint32_t dirty = (frame->object.privateFlags & rwFRAMEPRIVATESUBTREESYNCLTM) | (uint32_t)force;
    if (dirty != 0) {
        RwFrame *parent = frame->object.parent;
        RwMatrixMultiply(&frame->ltm, &frame->modelling, &parent->ltm);
        frame->object.privateFlags &= 0xfb;
    }
    rwFrameSyncLTM(frame->child, (int32_t)dirty);
    rwFrameSyncLTM(frame->next, force);
}

/* Brings the world matrices of a hierarchy up to date, from its root down. */
BMX_ADDR(0x004ee520)
void rwFrameSyncHierarchyLTM(RwFrame *root)
{
    const uint8_t flags = root->object.privateFlags;
    root->object.privateFlags = flags & 0xfa;
    const uint32_t dirty = flags & rwFRAMEPRIVATESUBTREESYNCLTM;
    if (dirty != 0) {
        root->ltm = root->modelling;
    }
    rwFrameSyncLTM(root->child, (int32_t)dirty);
}

/* ---- frame lists ------------------------------------------------------------------------------------ */

/* A frame as it is stored in a stream: the matrix rows and position, the parent's index and a flags word. */
typedef struct RwStreamFrame {
    float elements[12];
    int32_t parentIndex;
    uint32_t flags;
} RwStreamFrame;

_Static_assert(sizeof(RwStreamFrame) == 0x38, "RwStreamFrame layout");

/*
 * Reads a frame list chunk: the frames with their matrices (flags recomputed from how orthonormal each
 * matrix is) and parent links, then the plugin data of every frame. Frees the array again if the frames
 * cannot be read or created; returns NULL for any failure.
 */
BMX_ADDR(0x004e9360)
RwFrameList *_rwFrameListStreamRead(void *stream, RwFrameList *frameList)
{
    uint32_t length;
    uint32_t version;
    if (!RwStreamFindChunk(stream, 1, &length, &version)) {
        return NULL;
    }
    if (version < 0x301 || version > 0x303) {
        rwReportError(rwERR_BADVERSION, NULL);
        return NULL;
    }
    int32_t count;
    if (RwStreamRead(stream, &count, 4) != 4) {
        return NULL;
    }
    RwMemNative32(&count, 4);
    frameList->numFrames = count;
    frameList->frames = (RwFrame **)RwEngineInstance->memoryFuncs.rwmalloc((size_t)count << 2);
    if (frameList->frames == NULL) {
        rwReportError(rwERR_NOMEM, (const char *)(uintptr_t)((uint32_t)count << 2));
        return NULL;
    }
    for (int32_t i = 0; i < count; i++) {
        RwStreamFrame record;
        if (RwStreamRead(stream, &record, 0x38) != 0x38) {
            RwEngineInstance->memoryFuncs.rwfree((void *)frameList->frames);
            return NULL;
        }
        RwMemNative32(&record, 0x38);
        RwMemFloat32ToReal(&record.elements[0], 0xc);
        RwMemFloat32ToReal(&record.elements[3], 0xc);
        RwMemFloat32ToReal(&record.elements[6], 0xc);
        RwMemFloat32ToReal(&record.elements[9], 0xc);
        RwFrame *frame = RwFrameCreate();
        if (frame == NULL) {
            RwEngineInstance->memoryFuncs.rwfree((void *)frameList->frames);
            return NULL;
        }
        RwMatrix *matrix = RwFrameGetMatrix(frame);
        matrix->right.x = record.elements[0];
        matrix->right.y = record.elements[1];
        matrix->right.z = record.elements[2];
        matrix->up.x = record.elements[3];
        matrix->up.y = record.elements[4];
        matrix->up.z = record.elements[5];
        matrix->at.x = record.elements[6];
        matrix->at.y = record.elements[7];
        matrix->at.z = record.elements[8];
        matrix->pos.x = record.elements[9];
        matrix->pos.y = record.elements[10];
        matrix->pos.z = record.elements[11];
        if (rwMatrixNormalError(matrix) > (double)0x1.47ae14p-7f ||
            rwMatrixOrthogonalError(matrix) > (double)0x1.47ae14p-7f ||
            !(rwMatrixDeterminant(matrix) >= (double)0x1.fae148p-1f)) {
            matrix->flags &= 0xfffdfffc;
        } else {
            matrix->flags &= 0xfffdffff;
        }
        frameList->frames[i] = frame;
        if (record.parentIndex >= 0) {
            RwFrameAddChild(frameList->frames[record.parentIndex], frame);
        }
    }
    for (int32_t i = 0; i < count; i++) {
        if (_rwPluginRegistryReadDataChunks(&rwFrameRegistry, stream, frameList->frames[i]) == NULL) {
            return NULL;
        }
    }
    return frameList;
}

/* Frees the frame array of a list (the frames themselves are not touched). */
BMX_ADDR(0x004e96a0)
RwFrameList *_rwFrameListDeinitialize(RwFrameList *frameList)
{
    if (frameList->numFrames != 0) {
        RwEngineInstance->memoryFuncs.rwfree((void *)frameList->frames);
    }
    return frameList;
}

/* ---- clump (misassigned: belongs to rw/clump.c) ------------------------------------------------------- */

/* Calls callback for every atomic of the clump until it returns NULL (the next link is read first). */
BMX_ADDR(0x004b1990)
RpClump *RpClumpForAllAtomics(RpClump *clump, RwObjectCallBack callback, void *data)
{
    RwLLLink *link = clump->atomicList.next;
    while (link != &clump->atomicList) {
        RpAtomic *atomic = (RpAtomic *)((uint8_t *)link - offsetof(RpAtomic, inClumpLink));
        link = link->next;
        if (callback(atomic, data) == NULL) {
            break;
        }
    }
    return clump;
}
