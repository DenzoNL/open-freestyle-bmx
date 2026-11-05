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

/*
 * Combines a transformation into the matrix: replace it (keeping its flags), apply the transformation first
 * (pre-concatenation) or last (post-concatenation). An identity operand turns the product into a copy of the
 * other one; otherwise it goes through the module's multiply routine. The matrix keeps its own flags minus
 * the identity and type bits, and its version byte goes up (twice when combined).
 */
BMX_ADDR(0x004d16f0)
RwMatrix *RwMatrixTransform(RwMatrix *matrix, const RwMatrix *transform, RwOpCombineType combine)
{
    if (combine == rwCOMBINEREPLACE) {
        const uint32_t flags = matrix->flags;
        *matrix = *transform;
        matrix->flags = flags;
    } else if (combine == rwCOMBINEPRECONCAT || combine == rwCOMBINEPOSTCONCAT) {
        const RwMatrix *first = combine == rwCOMBINEPRECONCAT ? transform : matrix;
        const RwMatrix *second = combine == rwCOMBINEPRECONCAT ? matrix : transform;
        const uint32_t identity = rwMatrixModule()->flagMask & rwMATRIXINTERNALIDENTITY;
        RwMatrix result;
        if ((first->flags & identity) != 0) {
            result = *second;
        } else if ((second->flags & identity) != 0) {
            result = *first;
        } else {
            typedef void(BMX_CDECL * MultiplyFunc)(RwMatrix * out, const RwMatrix *a, const RwMatrix *b);
            ((MultiplyFunc)rwMatrixModule()->multiply)(&result, first, second);
        }
        ((uint8_t *)&result.pad1)[0]++;
        const uint32_t flags = matrix->flags;
        *matrix = result;
        matrix->flags = flags & 0xfffdfffc;
    } else {
        /* The original goes on to touch the (now null) matrix and crashes; here it returns. */
        rwReportError(rwERR_INVALIDSIZE, "Invalid combination type specified");
        return NULL;
    }
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
 * The rotation by the unit axis with the sine and 1 - cosine of the angle, combined with the matrix (replace,
 * rotation first or last; the product goes through the module's multiply routine unless the matrix is the
 * identity). The matrix keeps its own flags minus the identity bit and its version byte goes up (twice when
 * combined). The original leaves the padding words of its rotation matrix uninitialised; they are 0 here.
 */
__attribute__((used, noinline)) static RwMatrix *
rwMatrixRotateImpl(RwMatrix *matrix, const RwV3d *axis, int32_t combine, double oneMinusCosine, float sine)
{
    const double c = oneMinusCosine;
    const double xyc = rwMul(axis->x, axis->y) * c;
    const double xzc = rwMul(axis->x, axis->z) * c;
    const double yzc = rwMul(axis->z, axis->y) * c;
    const double sx = rwMul(sine, axis->x), sy = rwMul(sine, axis->y), sz = rwMul(sine, axis->z);
    RwMatrix rotation;
    memset(&rotation, 0, sizeof rotation);
    rotation.right.x = (float)(1.0 - (1.0 - rwMul(axis->x, axis->x)) * c);
    rotation.right.y = (float)(sz + xyc);
    rotation.right.z = (float)(xzc - sy);
    rotation.flags = 3;
    rotation.up.x = (float)(xyc - sz);
    rotation.up.y = (float)(1.0 - (1.0 - rwMul(axis->y, axis->y)) * c);
    rotation.up.z = (float)(yzc + sx);
    rotation.at.x = (float)(sy + xzc);
    rotation.at.y = (float)(yzc - sx);
    rotation.at.z = (float)(1.0 - c * (1.0 - rwMul(axis->z, axis->z)));
    RwMatrix result;
    memset(&result, 0, sizeof result);
    if (combine == rwCOMBINEREPLACE) {
        result = rotation;
    } else if (combine == rwCOMBINEPRECONCAT || combine == rwCOMBINEPOSTCONCAT) {
        const uint32_t identity = rwMatrixModule()->flagMask & rwMATRIXINTERNALIDENTITY;
        if ((matrix->flags & identity) != 0) {
            result = rotation;
        } else {
            typedef void(BMX_CDECL * MultiplyFunc)(RwMatrix * out, const RwMatrix *a, const RwMatrix *b);
            const MultiplyFunc multiply = (MultiplyFunc)rwMatrixModule()->multiply;
            if (combine == rwCOMBINEPRECONCAT) {
                multiply(&result, &rotation, matrix);
            } else {
                multiply(&result, matrix, &rotation);
            }
        }
        ((uint8_t *)&result.pad1)[0]++;
    } else {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(rwERR_INVALIDSIZE, "Invalid combination type");
        RwErrorSet(&error);
        return NULL;
    }
    const uint32_t flags = matrix->flags & ~(uint32_t)rwMATRIXINTERNALIDENTITY;
    *matrix = result;
    matrix->flags = flags;
    ((uint8_t *)&matrix->pad1)[0]++;
    return matrix;
}

/* The original's entry: EAX = matrix, EDX = unit axis, ECX = combine, ST0 = 1 - cos (popped), the float sine
 * as the fourth stack word. */
BMX_ADDR(0x004d0a00)
__attribute__((naked)) RwMatrix *RwMatrixRotateOneMinusCosineSine(uint32_t unused0, uint32_t unused1,
                                                                  uint32_t unused2, float sine)
{
    BMX_X86_ENTRY("pushl 0x10(%esp)\n\t"
                  "subl $8, %esp\n\t"
                  "fstpl (%esp)\n\t"
                  "pushl %ecx\n\t"
                  "pushl %edx\n\t"
                  "pushl %eax\n\t"
                  "call _rwMatrixRotateImpl\n\t"
                  "addl $24, %esp\n\t"
                  "ret");
}

/*
 * Rotates the matrix by angle degrees around axis: the axis scaled by the table based 1 / length (floats, as
 * the original stores them), sin(angle) as a float and 1 - cos(angle) as a double (the x87 FSIN / FCOS).
 */
BMX_ADDR(0x004d1040)
RwMatrix *RwMatrixRotate(RwMatrix *matrix, const RwV3d *axis, float angle, RwOpCombineType combine)
{
    const double radians = rwMul(0x1.1df46ap-6f, angle);
    const float lengthSquared =
        (float)((rwMul(axis->z, axis->z) + rwMul(axis->y, axis->y)) + rwMul(axis->x, axis->x));
    uint32_t bits;
    memcpy(&bits, &lengthSquared, sizeof bits);
    const double scale = _rwFastInvSqrt((int32_t)bits);
    RwV3d unit;
    unit.x = (float)((double)axis->x * scale);
    unit.y = (float)((double)axis->y * scale);
    unit.z = (float)((double)axis->z * scale);
    const float sine = (float)bmxX87Fsin(radians);
    const double cosine = bmxX87Fcos(radians);
    rwMatrixRotateImpl(matrix, &unit, combine, (double)1.0f - cosine, sine);
    return matrix;
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

/* sqrt(x) for a float given as its bits, from the engine's square root table; 0 for 0. */
BMX_ADDR(0x004d3880)
double _rwFastSqrt(int32_t floatBits)
{
    uint32_t bits = (uint32_t)floatBits;
    if (bits != 0) {
        const uint32_t *table = rwV3dModule()->sqrtTable;
        bits += 0x800;
        bits = table[(bits & 0xffffff) >> 12] + ((bits & 0x7f800000) >> 1);
    }
    float result;
    memcpy(&result, &bits, sizeof result);
    return result;
}

/* Squared length of a vector as the original sums it: (z*z + y*y) + x*x. */
static double rwV3dLengthSquared(const RwV3d *v)
{
    return (rwMul(v->z, v->z) + rwMul(v->y, v->y)) + rwMul(v->x, v->x);
}

static int32_t rwFloatBits(float f)
{
    int32_t bits;
    memcpy(&bits, &f, sizeof bits);
    return bits;
}

/* out = in scaled by the float s, x first (out may be in). */
static void rwV3dScale(RwV3d *out, const RwV3d *in, float s)
{
    out->x = (float)rwMul(in->x, s);
    out->y = (float)rwMul(s, in->y);
    out->z = (float)rwMul(s, in->z);
}

/* Normalises by the table based 1 / length (no error for a zero vector, which stays zero); returns it. */
BMX_ADDR(0x004d3650)
double _rwV3dNormalize(RwV3d *out, const RwV3d *in)
{
    const float scale = (float)_rwFastInvSqrt(rwFloatBits((float)rwV3dLengthSquared(in)));
    rwV3dScale(out, in, scale);
    return scale;
}

/* Normalises a vector and returns its length (both from the tables); a zero vector reports an error. */
BMX_ADDR(0x004d36e0)
double RwV3dNormalize(RwV3d *out, const RwV3d *in)
{
    const int32_t bits = rwFloatBits((float)rwV3dLengthSquared(in));
    const double length = (float)_rwFastSqrt(bits);
    rwV3dScale(out, in, (float)_rwFastInvSqrt(bits));
    if (length == 0.0 || __builtin_isnan(length)) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x10);
        RwErrorSet(&error);
    }
    return length;
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
/* Whether the frame's hierarchy waits for a sync (its root is queued). */
BMX_ADDR(0x004d2840)
int RwFrameDirty(const RwFrame *frame)
{
    return (frame->root->object.privateFlags & 3) != 0;
}

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

/* The per object callback of the sync below: calls the object's sync function; returns the object. */
BMX_ADDR(0x004ee410)
void *rwFrameSyncObjectCB(void *object, void *data)
{
    (void)data;
    ((void(BMX_CDECL *)(void *))((RwObjectHasFrame *)object)->sync)(object);
    return object;
}

/*
 * Lets the objects attached to a frame list and its subtrees follow their frames (EAX = frame, EDX = force):
 * a frame that is marked (bits 2 and 3) or is forced updates its world matrix (bit 2 or a forced bit 2),
 * clears its flags and calls every object's sync function; children take the combined bits, siblings the
 * caller's.
 */
BMX_ADDR(0x004ee430)
__attribute__((regparm(2))) void rwFrameSyncObjects(RwFrame *frame, uint32_t force)
{
    if (frame == NULL) {
        return;
    }
    const uint32_t dirty = (frame->object.privateFlags & 0xcu) | force;
    if (dirty != 0) {
        if ((dirty & 4) != 0) {
            RwMatrixMultiply(&frame->ltm, &frame->modelling, &((RwFrame *)frame->object.parent)->ltm);
        }
        frame->object.privateFlags = 0;
        RwFrameForAllObjects(frame, BMX_FNPTR(RwObjectCallBack, 0x004ee410, rwFrameSyncObjectCB), NULL);
    }
    rwFrameSyncObjects(frame->child, dirty);
    rwFrameSyncObjects(frame->next, force);
}

/*
 * Syncs every hierarchy in the engine's dirty list (the camera's begin update does this): a marked root
 * copies its modelling matrix to its world matrix, then it and its subtrees sync their objects. The list is
 * empty afterwards. Returns 1.
 */
BMX_ADDR(0x004ee5d0)
int rwFrameSyncDirtyList(void)
{
    const RwLLLink *head = &RwEngineInstance->dirtyFrameList;
    for (RwLLLink *link = RwEngineInstance->dirtyFrameList.next; link != head; link = link->next) {
        RwFrame *frame = (RwFrame *)((uint8_t *)link - offsetof(RwFrame, dirtyLink));
        const uint8_t flags = frame->object.privateFlags;
        frame->object.privateFlags = 0;
        const uint32_t changed = flags & 0xcu;
        if (changed != 0) {
            if ((changed & 4) != 0) {
                frame->ltm = frame->modelling;
            }
            RwFrameForAllObjects(frame, BMX_FNPTR(RwObjectCallBack, 0x004ee410, rwFrameSyncObjectCB), NULL);
        }
        rwFrameSyncObjects(frame->child, changed);
    }
    RwEngineInstance->dirtyFrameList.next = &RwEngineInstance->dirtyFrameList;
    RwEngineInstance->dirtyFrameList.prev = &RwEngineInstance->dirtyFrameList;
    return 1;
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
    frameList->frames = (RwFrame **)RwEngineInstance->memoryFuncs.rwmalloc((size_t)count * sizeof(RwFrame *));
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

/* ---- modules: frames, matrices and the square root tables ------------------------------------------- */

#define rwFrameModuleCount  ORIG_VAR(0x00676984, int32_t, rwFrameModuleCount)
#define rwVectorModuleCount ORIG_VAR(0x00676994, int32_t, rwVectorModuleCount)
/* Numerators of the reciprocal square root tables (both 1.0 in the exe's data). */
#define rwInvSqrtScaleOdd  ORIG_VAR(0x0069b950, double, rwInvSqrtScaleOdd)
#define rwInvSqrtScaleEven ORIG_VAR(0x0069b958, double, rwInvSqrtScaleEven)

static void rwReportNoMemory(int32_t bytes)
{
    RwError error;
    error.pluginID = 1;
    error.errorCode = _rwerror(rwERR_NOMEM, bytes);
    RwErrorSet(&error);
}

/* Frame module: the free list of frames (the registry's size, entries 4 aligned) and an empty dirty list. */
BMX_ADDR(0x004d2280)
void *_rwFrameOpen(void *engine, int32_t offset)
{
    rwFrameOffset = offset;
    RwFreeList *freeList = RwFreeListCreate(rwFrameRegistry.sizeOfStruct, 0x28, 4);
    *(RwFreeList **)((uint8_t *)RwEngineInstance + offset) = freeList;
    if (freeList == NULL) {
        return NULL;
    }
    RwEngineInstance->dirtyFrameList.next = &RwEngineInstance->dirtyFrameList;
    RwEngineInstance->dirtyFrameList.prev = &RwEngineInstance->dirtyFrameList;
    rwFrameModuleCount++;
    return engine;
}

BMX_ADDR(0x004d2310)
void *_rwFrameClose(void *engine)
{
    if (rwFrameFreeList() != NULL) {
        RwFreeListDestroy(rwFrameFreeList());
        *(RwFreeList **)((uint8_t *)RwEngineInstance + rwFrameOffset) = NULL;
    }
    rwFrameModuleCount--;
    return engine;
}

/* The engine's matrix product: out = a * b for matrices without perspective (rotation rows then position). */
BMX_ADDR(0x004d02c0)
void rwMatrixMultiplyDefault(RwMatrix *out, const RwMatrix *a, const RwMatrix *b)
{
    float *o = (float *)out;
    const float *x = (const float *)a;
    const float *y = (const float *)b;
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            o[row * 4 + col] =
                (float)((rwMul(x[row * 4 + 2], y[8 + col]) + rwMul(y[4 + col], x[row * 4 + 1])) +
                        rwMul(y[col], x[row * 4]));
        }
    }
    for (int col = 0; col < 3; col++) {
        o[12 + col] = (float)(((rwMul(x[14], y[8 + col]) + rwMul(y[4 + col], x[13])) + rwMul(y[col], x[12])) +
                              y[12 + col]);
    }
}

/*
 * The core's own x87 matrix product in _rwcseg: the routine the driver's camera begin update installs as
 * RwMatrixMultiply's function (rwMatrixGlobals()->multiply). Same maths as rwMatrixMultiplyDefault, but on
 * the x87 in the camera's precision mode and with its own order of additions (compat/x87.h).
 */
BMX_ADDR(0x0054b038)
void rwMatrixMultiplyX87(RwMatrix *out, const RwMatrix *a, const RwMatrix *b)
{
    bmxX87MatrixMultiply((float *)out, (const float *)a, (const float *)b);
}

/* Default point transform of the engine: out[i] = in[i] * matrix (rows then position). Returns out. */
BMX_ADDR(0x004d34d0)
RwV3d *rwV3dTransformPointsDefault(RwV3d *out, const RwV3d *in, int32_t count, const RwMatrix *matrix)
{
    const RwMatrix *m = matrix;
    for (int32_t i = 0; i < count; i++) {
        const double x = (rwMul(m->right.x, in[i].x) + rwMul(m->up.x, in[i].y)) + rwMul(m->at.x, in[i].z);
        const double y = (rwMul(m->right.y, in[i].x) + rwMul(m->up.y, in[i].y)) + rwMul(m->at.y, in[i].z);
        const double z = (rwMul(m->right.z, in[i].x) + rwMul(m->up.z, in[i].y)) + rwMul(m->at.z, in[i].z);
        out[i].x = (float)(x + m->pos.x);
        out[i].y = (float)(y + m->pos.y);
        out[i].z = (float)(z + m->pos.z);
    }
    return out;
}

/* Like rwV3dTransformPointsDefault, without the position: for directions. */
BMX_ADDR(0x004d3570)
RwV3d *rwV3dTransformVectorsDefault(RwV3d *out, const RwV3d *in, int32_t count, const RwMatrix *matrix)
{
    const RwMatrix *m = matrix;
    for (int32_t i = 0; i < count; i++) {
        const double x = (rwMul(m->up.x, in[i].y) + rwMul(m->right.x, in[i].x)) + rwMul(m->at.x, in[i].z);
        const double y = (rwMul(m->right.y, in[i].x) + rwMul(m->up.y, in[i].y)) + rwMul(m->at.y, in[i].z);
        const double z = (rwMul(m->right.z, in[i].x) + rwMul(m->up.z, in[i].y)) + rwMul(m->at.z, in[i].z);
        out[i].x = (float)x;
        out[i].y = (float)y;
        out[i].z = (float)z;
    }
    return out;
}

/* Installs the point and vector transforms (EAX, EDX; NULL = the defaults above). Always 1. */
BMX_ADDR(0x004d3608)
__attribute__((regparm(2))) int rwV3dSetTransformFuncs(RwV3dTransformFunc points, RwV3dTransformFunc vectors)
{
    if (points == NULL) {
        points = BMX_FNPTR(RwV3dTransformFunc, 0x004d34d0, rwV3dTransformPointsDefault);
    }
    rwV3dModule()->transformPoints = points;
    if (vectors == NULL) {
        vectors = BMX_FNPTR(RwV3dTransformFunc, 0x004d3570, rwV3dTransformVectorsDefault);
    }
    rwV3dModule()->transformVectors = vectors;
    return 1;
}

BMX_ADDR(0x004d3450)
void rwV3dFreeSqrtTable(void)
{
    if (rwV3dModule()->sqrtTable != NULL) {
        RwEngineInstance->memoryFuncs.rwfree((void *)rwV3dModule()->sqrtTable);
        rwV3dModule()->sqrtTable = NULL;
    }
}

BMX_ADDR(0x004d3490)
void rwV3dFreeInvSqrtTable(void)
{
    if (rwV3dModule()->invSqrtTable != NULL) {
        RwEngineInstance->memoryFuncs.rwfree((void *)rwV3dModule()->invSqrtTable);
        rwV3dModule()->invSqrtTable = NULL;
    }
}

BMX_ADDR(0x004d3b30)
void *rwVectorClose(void *engine)
{
    rwV3dFreeInvSqrtTable();
    rwV3dFreeSqrtTable();
    rwVectorModuleCount--;
    return engine;
}

static float rwFloatFromBits(uint32_t bits)
{
    float f;
    memcpy(&f, &bits, sizeof f);
    return f;
}

static uint32_t rwBitsOfFloat(float f)
{
    uint32_t bits;
    memcpy(&bits, &f, sizeof bits);
    return bits;
}

/*
 * Vector module: the default transforms and the two 8 KB... tables _rwFastSqrt and the reciprocal square
 * root
 * look up by the top mantissa bits of a float. Each holds 0x1000 entries: the bits of the root of the
 * floats
 * from 1.0 in steps of 0x1000 bits (the odd exponent half first), offset so that adding the halved
 * exponent of
 * the argument gives the root of the whole number. NULL with the error set when a table cannot
 * be allocated.
 */
BMX_ADDR(0x004d3b50)
void *rwVectorOpen(void *engine, int32_t offset)
{
    rwFastSqrtOffset = offset;
    rwV3dSetTransformFuncs(NULL, NULL);
    uint32_t *table = RwEngineInstance->memoryFuncs.rwmalloc(0x4000);
    if (table == NULL) {
        rwReportNoMemory(0x4000);
        return NULL;
    }
    uint32_t x = 0x3f800000;
    for (int i = 0; i < 0x800; i++, x += 0x1000) {
        table[0x800 + i] = rwBitsOfFloat((float)__builtin_sqrt((double)rwFloatFromBits(x))) + 0xe0400000u;
    }
    for (int i = 0; i < 0x800; i++, x += 0x1000) {
        table[i] = rwBitsOfFloat((float)__builtin_sqrt((double)rwFloatFromBits(x))) + 0xe0000000u;
    }
    rwV3dModule()->sqrtTable = table;
    uint32_t *inverse = RwEngineInstance->memoryFuncs.rwmalloc(0x4000);
    if (inverse == NULL) {
        rwReportNoMemory(0x4000);
        return NULL;
    }
    x = 0x3f800000;
    for (int i = 0; i < 0x800; i++, x += 0x1000) {
        inverse[0x800 + i] =
            rwBitsOfFloat((float)(rwInvSqrtScaleOdd / __builtin_sqrt((double)rwFloatFromBits(x)))) +
            0xe0000000u;
    }
    for (int i = 0; i < 0x800; i++, x += 0x1000) {
        inverse[i] = rwBitsOfFloat((float)(rwInvSqrtScaleEven / __builtin_sqrt((double)rwFloatFromBits(x)))) +
                     0xe0400000u;
    }
    rwVectorModuleCount++;
    rwV3dModule()->invSqrtTable = inverse;
    return engine;
}

/* ---- frame lists ------------------------------------------------------------------------------------ */

/* Stores the frame and then its siblings and children, in that order, at *cursor (EAX = cursor, EDX = frame).
 */
BMX_ADDR(0x004e8fc0)
__attribute__((regparm(2))) int rwFrameListFill(RwFrame ***cursor, RwFrame *frame)
{
    if (frame != NULL) {
        **cursor = frame;
        (*cursor)++;
        rwFrameListFill(cursor, frame->next);
        rwFrameListFill(cursor, frame->child);
    }
    return 1;
}

/* Fills a frame list with every frame of the hierarchy of root. NULL, with the error set, on no memory. */
BMX_ADDR(0x004e96d0)
RwFrameList *_rwFrameListInitialize(RwFrameList *frameList, RwFrame *root)
{
    const int32_t count = RwFrameCount(root);
    RwFrame **frames = (RwFrame **)RwEngineInstance->memoryFuncs.rwmalloc((size_t)count * sizeof(RwFrame *));
    if (frames == NULL) {
        rwReportNoMemory(count << 2);
        return NULL;
    }
    RwFrame **cursor = frames;
    rwFrameListFill(&cursor, root);
    frameList->numFrames = count;
    frameList->frames = frames;
    return frameList;
}
