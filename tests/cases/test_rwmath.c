/*
 * Tests for rw/math.c: matrices, vector transforms, bounding boxes, the frame hierarchy and frame lists.
 * The engine instance is a fixture (dt_rwmath.h) with logging free list stubs and module data.
 *
 * The register-argument helpers (RwFrameRemoveChild, rwFrameSetRoot, rwFrameSyncLTM) are exercised through
 * their standard-ABI callers (RwFrameDestroy, RwFrameDestroyHierarchy, RwFrameGetLTM).
 * An invalid combine type crashes the original (it dereferences the null matrix it just returned), so the
 * Scale / Translate / Transform error paths are not run.
 */
#include "dt.h"
#include "dt_rwmath.h"

/* ---- RpClumpForAllAtomics --------------------------------------------------------------------------- */

static int gAtomicCalls, gAtomicStopAt;

static void *BMX_CDECL atomic_cb(void *atomic, void *data)
{
    rwtLogPtr(atomic);
    rwtLogPtr(data);
    gAtomicCalls++;
    return gAtomicStopAt != 0 && gAtomicCalls == gAtomicStopAt ? NULL : atomic;
}

static void setup_clump_atomics(DtCtx *c)
{
    dt_seed(c);
    rwtEnv();
    RpClump *clump = dt_alloc(sizeof(RpClump));
    clump->atomicList.next = clump->atomicList.prev = &clump->atomicList;
    const int n = dt_range(0, 4);
    for (int i = 0; i < n; i++) {
        RpAtomic *atomic = dt_alloc(sizeof(RpAtomic));
        dt_fill(atomic, sizeof(RpAtomic));
        atomic->inClumpLink.prev = clump->atomicList.prev;
        atomic->inClumpLink.next = &clump->atomicList;
        clump->atomicList.prev->next = &atomic->inClumpLink;
        clump->atomicList.prev = &atomic->inClumpLink;
    }
    gAtomicCalls = 0;
    gAtomicStopAt = dt_u32() % 3 == 0 ? dt_range(1, 4) : 0;
    DT_ARG_PTR(c, 0, clump);
    DT_ARG_PTR(c, 1, atomic_cb);
    DT_ARG_PTR(c, 2, dt_alloc(8));
    rwtEnd();
}

/* ---- matrices --------------------------------------------------------------------------------------- */

static void setup_matrix_create(DtCtx *c)
{
    dt_seed(c);
    mathEnv();
    gMathAllocFailAt = dt_u32() % 5 == 0 ? 1 : 0;
    rwtEnd();
}

static void setup_matrix_destroy(DtCtx *c)
{
    dt_seed(c);
    mathEnv();
    DT_ARG_PTR(c, 0, dt_alloc(sizeof(RwMatrix)));
    rwtEnd();
}

/* A matrix, a vector (zero length now and then) and a combine type. */
static void matrix_op_setup(DtCtx *c, int combines, int withAngle)
{
    dt_seed(c);
    mathEnv();
    RwMatrix *m = dt_alloc(sizeof *m);
    mathFillMatrix(m);
    RwV3d *v = dt_alloc(sizeof *v);
    v->x = mathFloat();
    v->y = mathFloat();
    v->z = mathFloat();
    if (dt_u32() % 8 == 0) {
        v->x = v->y = v->z = 0.0f;
    }
    DT_ARG_PTR(c, 0, m);
    DT_ARG_PTR(c, 1, v);
    if (withAngle) {
        DT_ARG_FLOAT(c, 2, dt_u32() % 6 == 0 ? mathFloat() : dt_float_edgy(-720.0f, 720.0f));
        DT_ARG_INT(c, 3, dt_range(0, combines));
    } else {
        DT_ARG_INT(c, 2, dt_range(0, combines));
    }
    rwtEnd();
}

/*
 * RwMatrixRotateOneMinusCosineSine (original code) copies stale words of its own stack frame into the padding
 * of the result matrix. Zeroing the stack below the setup frame makes those words the same for both runs.
 */
static void clear_stack(void)
{
    uint8_t scratch[8192];
    memset(scratch, 0, sizeof scratch);
    __asm__ volatile("" : : "r"(scratch) : "memory");
}

static void setup_matrix_rotate(DtCtx *c)
{
    matrix_op_setup(c, 2, 1);
    clear_stack();
}

/* RwMatrixRotateOneMinusCosineSine: EAX = matrix, EDX = axis, ECX = combine, ST0 = 1 - cos, the sine as the
 * fourth stack word. */
static RwMatrix *rotate_omcs_call(uint32_t target, RwMatrix *m, const RwV3d *axis, int32_t combine,
                                  const double *omc, const float *sine)
{
    uint32_t eax = (uint32_t)(uintptr_t)m, edx = (uint32_t)(uintptr_t)axis, ecx = (uint32_t)combine;
    const uint32_t sineBits = *(const uint32_t *)sine;
    __asm__ volatile("subl $16, %%esp\n\t"
                     "movl %5, 12(%%esp)\n\t"
                     "fldl (%3)\n\t"
                     "call *%4\n\t"
                     "addl $16, %%esp"
                     : "+a"(eax), "+d"(edx), "+c"(ecx)
                     : "r"(omc), "r"(target), "r"(sineBits)
                     : "memory", "cc", "st");
    return (RwMatrix *)(uintptr_t)eax;
}

static RwMatrix *rotate_omcs_orig(RwMatrix *m, const RwV3d *axis, int32_t combine, const double *omc,
                                  const float *sine)
{
    return rotate_omcs_call(0x004d0a00, m, axis, combine, omc, sine);
}

static RwMatrix *rotate_omcs_impl(RwMatrix *m, const RwV3d *axis, int32_t combine, const double *omc,
                                  const float *sine)
{
    return rotate_omcs_call((uint32_t)(uintptr_t)RwMatrixRotateOneMinusCosineSine, m, axis, combine, omc,
                            sine);
}

static void setup_rotate_omcs(DtCtx *c)
{
    dt_seed(c);
    mathEnv();
    RwMatrix *m = dt_alloc(sizeof *m);
    mathFillMatrix(m);
    RwV3d *axis = dt_alloc(sizeof *axis);
    axis->x = dt_float_edgy(-1.0f, 1.0f);
    axis->y = dt_float_edgy(-1.0f, 1.0f);
    axis->z = dt_float_edgy(-1.0f, 1.0f);
    double *omc = dt_alloc(8);
    *omc = dt_u32() % 6 == 0 ? 0.0 : (double)dt_float(0.0f, 2.0f) * 1.0000001;
    float *sine = dt_alloc(4);
    *sine = dt_float_edgy(-1.0f, 1.0f);
    DT_ARG_PTR(c, 0, m);
    DT_ARG_PTR(c, 1, axis);
    DT_ARG_INT(c, 2, dt_range(0, dt_u32() % 8 == 0 ? 5 : 2));
    DT_ARG_PTR(c, 3, omc);
    DT_ARG_PTR(c, 4, sine);
    rwtEnd();
    clear_stack();
}

static void setup_matrix_scale(DtCtx *c)
{
    matrix_op_setup(c, 2, 0);
}

static void setup_matrix_translate(DtCtx *c)
{
    matrix_op_setup(c, 2, 0);
}

/* RwMatrixTransform: two random matrices (sometimes the same one), either may carry the identity bit. */
static void setup_matrix_transform(DtCtx *c)
{
    dt_seed(c);
    mathEnv();
    RwMatrix *m = dt_alloc(sizeof *m);
    RwMatrix *t = dt_alloc(sizeof *t);
    mathFillMatrix(m);
    mathFillMatrix(t);
    DT_ARG_PTR(c, 0, m);
    DT_ARG_PTR(c, 1, dt_u32() % 10 == 0 ? m : t);
    DT_ARG_INT(c, 2, dt_range(0, 2));
    rwtEnd();
}

static void setup_matrix_invert(DtCtx *c)
{
    dt_seed(c);
    mathEnv();
    RwMatrix *in = dt_alloc(sizeof *in);
    RwMatrix *out = dt_alloc(sizeof *out);
    mathFillMatrix(in);
    mathFillMatrix(out);
    if (dt_u32() % 6 == 0) {
        in->flags |= 0x20000;
    }
    if (dt_u32() % 3 == 0) {
        in->flags |= 3;
    }
    DT_ARG_PTR(c, 0, dt_u32() % 8 == 0 ? in : out);
    DT_ARG_PTR(c, 1, in);
    rwtEnd();
}

static void setup_fast_inv_sqrt(DtCtx *c)
{
    dt_seed(c);
    mathEnv();
    switch (dt_u32() % 4) {
    case 0:
        DT_ARG_INT(c, 0, 0);
        break;
    case 1:
        DT_ARG_FLOAT(c, 0, dt_float(0.0f, 100.0f));
        break;
    default:
        DT_ARG_INT(c, 0, dt_u32());
        break;
    }
    rwtEnd();
}

static void setup_transform(DtCtx *c)
{
    dt_seed(c);
    mathEnv();
    DT_ARG_PTR(c, 0, dt_alloc(0x30));
    DT_ARG_PTR(c, 1, dt_alloc(0x30));
    DT_ARG_INT(c, 2, dt_range(-1, 5));
    DT_ARG_PTR(c, 3, dt_alloc(sizeof(RwMatrix)));
    rwtEnd();
}

static void setup_bbox(DtCtx *c)
{
    dt_seed(c);
    mathEnv();
    const int n = dt_range(1, 9);
    RwV3d *points = dt_alloc(sizeof(RwV3d) * 10);
    for (int i = 0; i < n; i++) {
        points[i].x = mathFloat();
        points[i].y = mathFloat();
        points[i].z = mathFloat();
    }
    DT_ARG_PTR(c, 0, dt_alloc(sizeof(RwBBox)));
    DT_ARG_PTR(c, 1, points);
    DT_ARG_INT(c, 2, n);
    rwtEnd();
}

/* ---- frames ----------------------------------------------------------------------------------------- */

static void setup_frame_create(DtCtx *c)
{
    dt_seed(c);
    mathEnv();
    mathFrameRegistry();
    gMathAllocFailAt = dt_u32() % 6 == 0 ? 1 : 0;
    gRwtCtorFailAt = dt_u32() % 8 == 0 ? 1 : 0;
    rwtEnd();
}

/* A random frame of a random tree. */
static RwFrame *pick_frame(int maxN)
{
    RwFrame *frames[8];
    const int n = mathFrameTree(frames, maxN);
    return frames[dt_range(0, n - 1)];
}

static void setup_frame_destroy(DtCtx *c)
{
    dt_seed(c);
    mathEnv();
    mathFrameRegistry();
    DT_ARG_PTR(c, 0, pick_frame(7));
    rwtEnd();
}

static void setup_frame_plain(DtCtx *c)
{
    dt_seed(c);
    mathEnv();
    DT_ARG_PTR(c, 0, pick_frame(6));
    rwtEnd();
}

static void setup_frame_translate(DtCtx *c)
{
    dt_seed(c);
    mathEnv();
    DT_ARG_PTR(c, 0, pick_frame(6));
    RwV3d *v = dt_alloc(sizeof *v);
    v->x = mathFloat();
    v->y = mathFloat();
    v->z = mathFloat();
    DT_ARG_PTR(c, 1, v);
    DT_ARG_INT(c, 2, dt_range(0, 2));
    rwtEnd();
}

static int gChildCalls, gChildStopAt;

static void *BMX_CDECL child_cb(void *frame, void *data)
{
    rwtLogPtr(frame);
    rwtLogPtr(data);
    gChildCalls++;
    return gChildStopAt != 0 && gChildCalls == gChildStopAt ? NULL : frame;
}

static void setup_frame_children(DtCtx *c)
{
    dt_seed(c);
    mathEnv();
    DT_ARG_PTR(c, 0, pick_frame(6));
    DT_ARG_PTR(c, 1, child_cb);
    DT_ARG_PTR(c, 2, dt_alloc(8));
    gChildCalls = 0;
    gChildStopAt = dt_u32() % 3 == 0 ? dt_range(1, 3) : 0;
    rwtEnd();
}

/* ---- frame lists ------------------------------------------------------------------------------------ */

static uint8_t gStreamBytes[4 + 0x38 * 5];
static uint32_t gStreamLen, gStreamPos;
static int gStreamCalls, gStreamFailAt, gReadChunksCalls, gReadChunksFailAt;

static int32_t BMX_CDECL stub_stream_read(void *stream, void *buffer, uint32_t length)
{
    rwtLog(0x4e2800);
    rwtLogPtr(stream);
    rwtLog(length);
    gStreamCalls++;
    if (gStreamFailAt != 0 && gStreamCalls == gStreamFailAt) {
        return 0;
    }
    uint32_t n = length;
    if (gStreamPos + n > gStreamLen) {
        n = gStreamLen - gStreamPos;
    }
    memcpy(buffer, gStreamBytes + gStreamPos, n);
    gStreamPos += n;
    return (int32_t)n;
}

static RwPluginRegistry *BMX_CDECL stub_read_chunks(RwPluginRegistry *registry, void *stream, void *object)
{
    rwtLog(0x4e8bf0);
    rwtLogPtr(registry);
    rwtLogPtr(stream);
    rwtLogPtr(object);
    gReadChunksCalls++;
    return gReadChunksFailAt != 0 && gReadChunksCalls == gReadChunksFailAt ? NULL : registry;
}

/* A matrix of a stream record: nearly orthonormal, scaled, skewed, mirrored or random. */
static void stream_matrix(float *e)
{
    static const float axes[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
    memcpy(e, axes, sizeof axes);
    const uint32_t kind = dt_u32() % 6;
    if (kind == 1) {
        for (int i = 0; i < 9; i++) {
            e[i] *= dt_float(0.5f, 2.0f);
        }
    } else if (kind == 2) {
        e[4] += dt_float(-0.2f, 0.2f);
        e[1] += dt_float(-0.2f, 0.2f);
    } else if (kind == 3) {
        e[0] = -1.0f;
    } else if (kind == 4) {
        for (int i = 0; i < 9; i++) {
            e[i] = dt_float(-3.0f, 3.0f);
        }
    } else if (kind == 0) {
        for (int i = 0; i < 9; i++) {
            e[i] += dt_float(-0.012f, 0.012f);
        }
    }
    for (int i = 9; i < 12; i++) {
        e[i] = dt_float(-100.0f, 100.0f);
    }
}

static void setup_frame_list_read(DtCtx *c)
{
    dt_seed(c);
    mathEnv();
    mathFrameRegistry();
    const int count = dt_range(0, 4);
    memset(gStreamBytes, 0, sizeof gStreamBytes);
    memcpy(gStreamBytes, &count, 4);
    for (int i = 0; i < count; i++) {
        float *e = (float *)(gStreamBytes + 4 + 0x38 * i);
        stream_matrix(e);
        const int32_t parent = i == 0 || dt_u32() % 3 == 0 ? -1 : dt_range(0, i - 1);
        memcpy(gStreamBytes + 4 + 0x38 * i + 0x30, &parent, 4);
        dt_fill(gStreamBytes + 4 + 0x38 * i + 0x34, 4);
    }
    gStreamLen = 4 + 0x38u * (uint32_t)count;
    if (dt_u32() % 9 == 0) {
        gStreamLen -= (uint32_t)dt_range(1, 8);
    }
    gStreamPos = 0;
    gStreamCalls = 0;
    gStreamFailAt = dt_u32() % 8 == 0 ? dt_range(1, 3) : 0;
    gReadChunksCalls = 0;
    gReadChunksFailAt = dt_u32() % 8 == 0 ? dt_range(1, 3) : 0;
    gRwtFindOk = dt_u32() % 10 != 0;
    static const uint32_t versions[] = {0x300, 0x301, 0x302, 0x303, 0x303, 0x303, 0x304};
    gRwtFindVersion = versions[dt_u32() % 7];
    gRwtFindLength = (uint32_t)(4 + 0x38 * count);
    gMathAllocFailAt = dt_u32() % 10 == 0 ? dt_range(1, 3) : 0;
    gRwtMallocFailAt = dt_u32() % 10 == 0 ? 1 : 0;
    dt_stub(0x004e2ec0, NULL, (void *)rwtStreamFindChunk);
    dt_stub(0x004e2800, NULL, (void *)stub_stream_read);
    dt_stub(0x004e8bf0, NULL, (void *)stub_read_chunks);
    RwFrameList *list = dt_alloc(sizeof(RwFrameList));
    DT_ARG_PTR(c, 0, dt_alloc(8));
    DT_ARG_PTR(c, 1, list);
    rwtEnd();
}

static void setup_frame_list_deinit(DtCtx *c)
{
    dt_seed(c);
    rwtEnv();
    RwFrameList *list = dt_alloc(sizeof(RwFrameList));
    list->frames = (RwFrame **)dt_alloc(16);
    list->numFrames = dt_u32() % 3 == 0 ? 0 : dt_range(1, 4);
    DT_ARG_PTR(c, 0, list);
    rwtEnd();
}

DT_CASES(rwmath_cases) = {
    {"RpClumpForAllAtomics", 0x004b1990, RpClumpForAllAtomics, 3, DT_RET_INT, setup_clump_atomics, 200, 0},
    {"RwMatrixRotateOneMinusCosineSine", (uint32_t)(uintptr_t)rotate_omcs_orig, rotate_omcs_impl, 5,
     DT_RET_INT, setup_rotate_omcs, 1500, 0},
    {"RwMatrixRotate", 0x004d1040, RwMatrixRotate, 4, DT_RET_INT, setup_matrix_rotate, 1500, 0},
    {"RwMatrixInvert", 0x004d1340, RwMatrixInvert, 2, DT_RET_INT, setup_matrix_invert, 400, 0},
    {"RwMatrixTransform", 0x004d16f0, RwMatrixTransform, 3, DT_RET_INT, setup_matrix_transform, 800, 0},
    {"RwMatrixScale", 0x004d1400, RwMatrixScale, 3, DT_RET_INT, setup_matrix_scale, 600, 0},
    {"RwMatrixTranslate", 0x004d15a0, RwMatrixTranslate, 3, DT_RET_INT, setup_matrix_translate, 800, 0},
    {"RwMatrixDestroy", 0x004d1fb0, RwMatrixDestroy, 1, DT_RET_INT, setup_matrix_destroy, 20, 0},
    {"RwMatrixCreate", 0x004d1fe0, RwMatrixCreate, 0, DT_RET_INT, setup_matrix_create, 40, 0},
    {"RwFrameCreate", 0x004d2870, RwFrameCreate, 0, DT_RET_INT, setup_frame_create, 150, 0},
    {"RwFrameDestroy", 0x004d2920, RwFrameDestroy, 1, DT_RET_INT, setup_frame_destroy, 400, 0},
    {"RwFrameDestroyHierarchy", 0x004d29c0, RwFrameDestroyHierarchy, 1, DT_RET_INT, setup_frame_destroy, 400,
     0},
    {"RwFrameUpdateObjects", 0x004d2a80, RwFrameUpdateObjects, 1, DT_RET_INT, setup_frame_plain, 200, 0},
    {"RwFrameGetMatrix", 0x004d2b00, RwFrameGetMatrix, 1, DT_RET_INT, setup_frame_plain, 10, 0},
    {"RwFrameGetLTM", 0x004d2b10, RwFrameGetLTM, 1, DT_RET_INT, setup_frame_plain, 400, 0},
    {"RwFrameForAllChildren", 0x004d2eb0, RwFrameForAllChildren, 3, DT_RET_INT, setup_frame_children, 200, 0},
    {"RwFrameCount", 0x004d2f00, RwFrameCount, 1, DT_RET_INT, setup_frame_plain, 100, 0},
    {"RwFrameTranslate", 0x004d2f40, RwFrameTranslate, 3, DT_RET_INT, setup_frame_translate, 600, 0},
    {"RwFrameSetIdentity", 0x004d3200, RwFrameSetIdentity, 1, DT_RET_INT, setup_frame_plain, 200, 0},
    {"_rwFastInvSqrt", 0x004d38d0, _rwFastInvSqrt, 1, DT_RET_DOUBLE, setup_fast_inv_sqrt, 1000, 0},
    {"RwV3dTransformPoints", 0x004d3a90, RwV3dTransformPoints, 4, DT_RET_INT, setup_transform, 30, 0},
    {"RwV3dTransformVectors", 0x004d3ae0, RwV3dTransformVectors, 4, DT_RET_INT, setup_transform, 30, 0},
    {"_rwFrameListStreamRead", 0x004e9360, _rwFrameListStreamRead, 2, DT_RET_INT, setup_frame_list_read, 1500,
     0},
    {"_rwFrameListDeinitialize", 0x004e96a0, _rwFrameListDeinitialize, 1, DT_RET_INT, setup_frame_list_deinit,
     30, 0},
    {"RwBBoxCalculate", 0x004e9760, RwBBoxCalculate, 3, DT_RET_INT, setup_bbox, 500, 0},
    {"rwFrameSyncHierarchyLTM", 0x004ee520, rwFrameSyncHierarchyLTM, 1, DT_RET_VOID, setup_frame_plain, 400,
     0},
};
