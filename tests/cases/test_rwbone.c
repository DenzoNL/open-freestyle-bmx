/*
 * Tests for the bone plugin's callbacks and frame data in rw/plugins.c (frame walk callbacks, bone data of
 * frames, the plugin constructors, destructors, copy and stream callbacks), and the animation interpolator's
 * deinit. The engine instance is the fixture of dt_rw.h; its memory interface logs. Callees outside the
 * group (frame walks, matrix calls, the stream writer) are logging stubs. The register-argument helpers are
 * called through the original's convention (EAX, EDX).
 */
#include "dt.h"
#include "dt_rw.h"
#include "rw/rwclump.h"
#include "rw/rwcore.h"
#include "rw/rwplugins.h"

#define REG1(addr, ret, ta)     ((ret(__attribute__((regparm(1))) *)(ta))(uintptr_t)(addr))
#define REG2(addr, ret, ta, tb) ((ret(__attribute__((regparm(2))) *)(ta, tb))(uintptr_t)(addr))

enum {
    kFrameBytes = 0x100,
};

static void boneEnv(DtCtx *c)
{
    dt_seed(c);
    rwtEnv();
    rpBoneFrameOffset = dt_range(0x10, 0x50) & ~3;
    rpBoneGeometryOffset = dt_range(0x20, 0x60) & ~3;
    rpAnimFrameOffset = dt_range(0x10, 0x50) & ~3;
    rpBoneEngineOffset = (int32_t)dt_u32();
    rpBoneModuleCount = dt_range(-2, 5);
    gRwtMallocFailAt = dt_u32() % 8 == 0 ? 1 : 0;
}

static uint8_t *mkFrame(void)
{
    uint8_t *frame = dt_alloc(kFrameBytes);
    for (int i = 0; i < kFrameBytes; i += 4) {
        *(uint32_t *)(frame + i) = dt_u32() % 3 == 0 ? 0 : dt_u32();
    }
    return frame;
}

static RpSkinBone *mkBone(void *frame)
{
    RpSkinBone *bone = dt_alloc(sizeof(RpSkinBone));
    uint32_t *words = (uint32_t *)bone;
    for (int i = 0; i < 0x22; i++) {
        words[i] = dt_u32();
    }
    bone->frame = frame;
    return bone;
}

/* ---- frame object callbacks ------------------------------------------------------------------------- */

static void setup_find_atomic(DtCtx *c)
{
    boneEnv(c);
    uint8_t *object = dt_alloc(0x40);
    object[0] = (uint8_t)dt_range(0, 3);
    DT_ARG_PTR(c, 0, object);
    DT_ARG_PTR(c, 1, dt_alloc(8));
    rwtEnd();
}

static void setup_find_skin_atomic(DtCtx *c)
{
    boneEnv(c);
    uint8_t *object = dt_alloc(0x40);
    object[0] = (uint8_t)dt_range(0, 3);
    uint8_t *geometry = dt_alloc(0x80);
    *(void **)(geometry + rpBoneGeometryOffset) = dt_u32() % 2 ? dt_alloc(0xc) : NULL;
    *(void **)(object + 0x1c) = geometry;
    DT_ARG_PTR(c, 0, object);
    DT_ARG_PTR(c, 1, dt_alloc(8));
    rwtEnd();
}

/* ---- bone data of frames ---------------------------------------------------------------------------- */

static void setup_bone_get(DtCtx *c)
{
    boneEnv(c);
    DT_ARG_PTR(c, 0, dt_u32() % 5 == 0 ? NULL : mkFrame());
    rwtEnd();
}

static void *boneGetOrig(void *frame)
{
    return REG1(0x00502ba0, void *, void *)(frame);
}

static void *boneGetImpl(void *frame)
{
    return rpBoneGet(frame);
}

static void setup_bone_set(DtCtx *c)
{
    boneEnv(c);
    DT_ARG_PTR(c, 0, dt_u32() % 5 == 0 ? NULL : mkFrame());
    DT_ARG_PTR(c, 1, dt_alloc(8));
    rwtEnd();
}

static void *boneSetOrig(void *frame, void *bone)
{
    return REG2(0x00502b80, void *, void *, void *)(frame, bone);
}

static void *boneSetImpl(void *frame, void *bone)
{
    return rpBoneSet(frame, bone);
}

static void setup_bone_create(DtCtx *c)
{
    boneEnv(c);
    rwtEnd();
}

static void setup_bone_destroy(DtCtx *c)
{
    boneEnv(c);
    uint8_t *frame = mkFrame();
    RpSkinBone *bone = dt_u32() % 6 == 0 ? NULL : mkBone(dt_u32() % 4 == 0 ? NULL : frame);
    *(RpSkinBone **)(frame + rpBoneFrameOffset) = bone;
    DT_ARG_PTR(c, 0, bone);
    rwtEnd();
}

static void boneDestroyOrig(void *bone)
{
    REG1(0x00502c60, void, void *)(bone);
}

static void boneDestroyImpl(void *bone)
{
    rpBoneDestroy(bone);
}

/* ---- the walks over frames -------------------------------------------------------------------------- */

static void *BMX_CDECL stChildren(void *frame, void *callback, void *data)
{
    rwtLog(0x4d2eb0);
    rwtLogPtr(frame);
    rwtLogPtr(callback);
    rwtLogPtr(data);
    return frame;
}

static void setup_find_frame(DtCtx *c)
{
    boneEnv(c);
    dt_stub(0x004d2eb0, NULL, (void *)stChildren);
    uint8_t *frame = mkFrame();
    const uint32_t key = dt_u32();
    *(uint32_t *)(frame + rpAnimFrameOffset + 8) = dt_u32() % 3 == 0 ? key : dt_u32();
    uint32_t *search = dt_alloc(8);
    search[0] = dt_u32();
    search[1] = key;
    DT_ARG_PTR(c, 0, frame);
    DT_ARG_PTR(c, 1, dt_u32() % 6 == 0 ? NULL : search);
    rwtEnd();
}

static void *BMX_CDECL stGetMatrix(void *frame)
{
    rwtLog(0x4d2b10);
    rwtLogPtr(frame);
    return (uint8_t *)frame + 0x40;
}

static void *BMX_CDECL stGetLocalMatrix(void *frame)
{
    rwtLog(0x4d2b00);
    rwtLogPtr(frame);
    return (uint8_t *)frame + 0x20;
}

static void *BMX_CDECL stInvert(uint32_t *out, const uint32_t *in)
{
    rwtLog(0x4d1340);
    rwtLogPtr(out);
    rwtLogPtr(in);
    for (int i = 0; i < 4; i++) {
        out[i] = dt_u32();
    }
    return out;
}

static void *BMX_CDECL stTransform(uint32_t *matrix, const uint32_t *other, int32_t combine)
{
    rwtLog(0x4d16f0);
    rwtLogPtr(matrix);
    rwtLogPtr(other);
    rwtLog((uint32_t)combine);
    matrix[0] ^= dt_u32();
    return matrix;
}

static void setup_attach_frames(DtCtx *c)
{
    boneEnv(c);
    gRwtMallocFailAt = 0;
    dt_stub(0x004d2eb0, NULL, (void *)stChildren);
    dt_stub(0x004d2b10, NULL, (void *)stGetMatrix);
    dt_stub(0x004d2b00, NULL, (void *)stGetLocalMatrix);
    dt_stub(0x004d1340, NULL, (void *)stInvert);
    dt_stub(0x004d16f0, NULL, (void *)stTransform);
    uint8_t *frame = mkFrame();
    *(RpSkinBone **)(frame + rpBoneFrameOffset) = dt_u32() % 3 == 0 ? mkBone(frame) : NULL;
    *(uint32_t *)(frame + rpAnimFrameOffset + 8) = dt_u32() % 3 == 0 ? 0xffffffffU : dt_u32();
    DT_ARG_PTR(c, 0, dt_u32() % 8 == 0 ? NULL : frame);
    DT_ARG_PTR(c, 1, dt_u32() % 8 == 0 ? NULL : mkFrame());
    rwtEnd();
}

/* ---- plugin callbacks ------------------------------------------------------------------------------- */

static void setup_engine_open(DtCtx *c)
{
    boneEnv(c);
    DT_ARG_PTR(c, 0, dt_alloc(8));
    DT_ARG_INT(c, 1, dt_u32());
    rwtEnd();
}

static void setup_engine_close(DtCtx *c)
{
    boneEnv(c);
    DT_ARG_PTR(c, 0, dt_alloc(8));
    rwtEnd();
}

static uint8_t *mkGeometry(int withSkin)
{
    uint8_t *geometry = dt_alloc(0x80);
    RpSkinGeometry *skin = NULL;
    if (withSkin) {
        skin = dt_alloc(sizeof(RpSkinGeometry));
        skin->blocks = dt_u32() % 3 == 0 ? NULL : dt_alloc(0x10 * 6);
        skin->origVerts = dt_u32() % 3 == 0 ? NULL : dt_alloc(0x20);
        skin->origNormals = dt_u32() % 3 == 0 ? NULL : dt_alloc(0x20);
        const int n = dt_range(0, 4);
        for (int i = 0; skin->blocks != NULL && i < 6; i++) {
            skin->blocks[i].firstVertex = dt_range(0, 20);
            skin->blocks[i].numVertices = i < n ? dt_range(1, 20) : 0;
            skin->blocks[i].key = (int32_t)dt_u32();
            skin->blocks[i].bone = (RpSkinBone *)(uintptr_t)dt_u32();
        }
    }
    *(RpSkinGeometry **)(geometry + rpBoneGeometryOffset) = skin;
    return geometry;
}

static void setup_geometry_init(DtCtx *c)
{
    boneEnv(c);
    DT_ARG_PTR(c, 0, mkGeometry(dt_u32() % 2));
    rwtEnd();
}

static void setup_geometry_copy(DtCtx *c)
{
    boneEnv(c);
    DT_ARG_PTR(c, 0, dt_alloc(8));
    rwtEnd();
}

static int gWriteCalls, gWriteFailAt;

static void *BMX_CDECL stWriteInts(void *stream, const uint32_t *ints, uint32_t size)
{
    rwtLog(0x4e3130);
    rwtLogPtr(stream);
    rwtLogPtr(ints);
    rwtLog(size);
    rwtLog(ints[0]);
    rwtLog(ints[1]);
    rwtLog(ints[2]);
    gWriteCalls++;
    return gWriteFailAt != 0 && gWriteCalls == gWriteFailAt ? NULL : stream;
}

static void setup_geometry_write(DtCtx *c)
{
    boneEnv(c);
    dt_stub(0x004e3130, NULL, (void *)stWriteInts);
    gWriteCalls = 0;
    gWriteFailAt = dt_u32() % 4 == 0 ? dt_range(1, 5) : 0;
    DT_ARG_PTR(c, 0, dt_alloc(0x20));
    DT_ARG_INT(c, 1, dt_range(0, 100));
    DT_ARG_PTR(c, 2, mkGeometry(1));
    rwtEnd();
}

static void setup_frame_init(DtCtx *c)
{
    boneEnv(c);
    DT_ARG_PTR(c, 0, mkFrame());
    rwtEnd();
}

static void setup_frame_deinit(DtCtx *c)
{
    boneEnv(c);
    uint8_t *frame = mkFrame();
    *(RpSkinBone **)(frame + rpBoneFrameOffset) =
        dt_u32() % 3 == 0 ? NULL : mkBone(dt_u32() % 2 ? frame : NULL);
    DT_ARG_PTR(c, 0, frame);
    rwtEnd();
}

static void setup_frame_copy(DtCtx *c)
{
    boneEnv(c);
    uint8_t *to = mkFrame();
    uint8_t *from = mkFrame();
    *(RpSkinBone **)(to + rpBoneFrameOffset) = dt_u32() % 3 == 0 ? NULL : mkBone(NULL);
    *(RpSkinBone **)(from + rpBoneFrameOffset) = dt_u32() % 3 == 0 ? NULL : mkBone(from);
    DT_ARG_PTR(c, 0, to);
    DT_ARG_PTR(c, 1, from);
    rwtEnd();
}

static void setup_interpolator_deinit(DtCtx *c)
{
    boneEnv(c);
    uint8_t *interpolator = dt_alloc(0x40);
    *(void **)(interpolator + 0x2c) = dt_u32() % 2 ? dt_alloc(8) : NULL;
    *(void **)(interpolator + 0x30) = dt_u32() % 2 ? dt_alloc(8) : NULL;
    DT_ARG_PTR(c, 0, interpolator);
    rwtEnd();
}

/* ---- the animation plugin's callbacks (frames, clumps, stream chunks) ---------------------------------- */

void *rpAnimFrameDataCreate(void);
void *rpAnimNodeCreate(void);

static int32_t BMX_CDECL stStrlen(const char *s)
{
    rwtLog(0x5e17);
    rwtLogPtr(s);
    return (int32_t)(((uintptr_t)s >> 3) % 11);
}

__attribute__((regparm(3))) static void *stAddSequence(void *clump, const char *name, void *callBack,
                                                       uint32_t u0, uint32_t u1, uint32_t u2, void *data)
{
    rwtLog(0x4fc7fc);
    rwtLogPtr(clump);
    rwtLogPtr(name);
    rwtLogPtr(callBack);
    rwtLogPtr(data);
    return clump;
}

static void *BMX_CDECL stReadInt(void *stream, uint32_t *ints, uint32_t size)
{
    rwtLog(0x4e3270);
    rwtLogPtr(stream);
    rwtLog(size);
    ints[0] = dt_u32() % 3 == 0 ? 0 : dt_u32();
    return dt_u32() % 5 == 0 ? NULL : stream;
}

static void *BMX_CDECL stWriteInt(void *stream, const uint32_t *ints, uint32_t size)
{
    rwtLog(0x4e3130);
    rwtLogPtr(stream);
    rwtLog(size);
    rwtLog(ints[0]);
    return dt_u32() % 5 == 0 ? NULL : stream;
}

static void animEnv(DtCtx *c)
{
    boneEnv(c);
    gRwtMallocFailAt = dt_u32() % 8 == 0 ? 1 : 0;
    RwEngineInstance->stringFuncs.rwstrlen = (void *)stStrlen;
    rpAnimClumpOffset = dt_range(0x10, 0x50) & ~3;
}

static void *mkSequence(void)
{
    uint32_t *s = dt_alloc(0x20);
    s[1] = (uint32_t)(uintptr_t)dt_alloc(16);
    s[2] = (uint32_t)dt_range(0, 3);
    s[3] = (uint32_t)dt_range(0, 6);
    s[5] = (uint32_t)dt_range(0, 6);
    return s;
}

static void *mkLinks(void)
{
    void **head = NULL;
    for (int n = dt_range(0, 3); n > 0; n--) {
        void **link = (void **)dt_alloc(8);
        link[0] = mkSequence();
        link[1] = (void *)head;
        head = link;
    }
    return (void *)head;
}

static uint8_t *mkAnimFrame(void)
{
    uint8_t *frame = mkFrame();
    void **data = (void **)dt_alloc(0x10);
    data[1] = mkLinks();
    data[3] = mkLinks();
    *(void **)(frame + rpAnimFrameOffset) = dt_u32() % 5 == 0 ? NULL : (void *)data;
    return frame;
}

static void setup_anim_interp_init(DtCtx *c)
{
    animEnv(c);
    DT_ARG_PTR(c, 0, mkFrame());
    rwtEnd();
}

static void setup_anim_node_create(DtCtx *c)
{
    animEnv(c);
    rwtEnd();
}

static void setup_anim_sequence(DtCtx *c)
{
    animEnv(c);
    DT_ARG_PTR(c, 0, mkSequence());
    rwtEnd();
}

static const char *seqNameOrig(void *sequence)
{
    return REG1(0x00500044, const char *, void *)(sequence);
}

static const char *seqNameImpl(void *sequence)
{
    return rpAnimSequenceName(sequence);
}

static int32_t seqSizeOrig(void *sequence)
{
    return REG1(0x004fd710, int32_t, void *)(sequence);
}

static int32_t seqSizeImpl(void *sequence)
{
    return rpAnimSequenceSize(sequence);
}

static void setup_anim_frame_size(DtCtx *c)
{
    animEnv(c);
    DT_ARG_PTR(c, 0, mkAnimFrame());
    rwtEnd();
}

static void setup_anim_add_frame_seq(DtCtx *c)
{
    animEnv(c);
    dt_stub(0x004fc7fc, NULL, (void *)stAddSequence);
    DT_ARG_PTR(c, 0, mkSequence());
    DT_ARG_PTR(c, 1, dt_alloc(0x40));
    rwtEnd();
}

static void *BMX_CDECL stAnimChildren(void *frame, void *callback, void *data)
{
    rwtLog(0x4d2eb0);
    rwtLogPtr(frame);
    rwtLogPtr(callback);
    rwtLogPtr(data);
    return frame;
}

static void setup_anim_add_frame_seqs(DtCtx *c)
{
    animEnv(c);
    dt_stub(0x004fc7fc, NULL, (void *)stAddSequence);
    dt_stub(0x004d2eb0, NULL, (void *)stAnimChildren);
    DT_ARG_PTR(c, 0, mkAnimFrame());
    DT_ARG_PTR(c, 1, dt_alloc(0x40));
    rwtEnd();
}

static void *addFrameSeqsOrig(void *frame, void *clump)
{
    return REG2(0x004fd7b8, void *, void *, void *)(frame, clump);
}

static void *addFrameSeqsImpl(void *frame, void *clump)
{
    return rpAnimClumpAddFrameSequences(frame, clump);
}

static void *addAllOrig(void *clump)
{
    return REG1(0x004fd840, void *, void *)(clump);
}

static void *addAllImpl(void *clump)
{
    return rpAnimClumpAddAllSequences(clump);
}

static void setup_anim_add_all(DtCtx *c)
{
    animEnv(c);
    dt_stub(0x004fc7fc, NULL, (void *)stAddSequence);
    dt_stub(0x004d2eb0, NULL, (void *)stAnimChildren);
    uint8_t *clump = dt_alloc(0x80);
    *(void **)(clump + 4) = dt_u32() % 4 == 0 ? NULL : mkAnimFrame();
    DT_ARG_PTR(c, 0, dt_u32() % 8 == 0 ? NULL : clump);
    rwtEnd();
}

static void setup_anim_chunk_read(DtCtx *c)
{
    animEnv(c);
    dt_stub(0x004fc7fc, NULL, (void *)stAddSequence);
    dt_stub(0x004d2eb0, NULL, (void *)stAnimChildren);
    dt_stub(0x004e3270, NULL, (void *)stReadInt);
    uint8_t *clump = dt_alloc(0x80);
    *(void **)(clump + 4) = dt_u32() % 4 == 0 ? NULL : mkAnimFrame();
    DT_ARG_PTR(c, 0, dt_alloc(0x20));
    DT_ARG_INT(c, 1, dt_range(0, 100));
    DT_ARG_PTR(c, 2, clump);
    rwtEnd();
}

static void setup_anim_chunk_write(DtCtx *c)
{
    animEnv(c);
    dt_stub(0x004e3130, NULL, (void *)stWriteInt);
    uint8_t *clump = dt_alloc(0x80);
    *(int32_t *)(clump + rpAnimClumpOffset) = dt_range(-2, 3);
    DT_ARG_PTR(c, 0, dt_alloc(0x20));
    DT_ARG_INT(c, 1, dt_range(0, 100));
    DT_ARG_PTR(c, 2, clump);
    rwtEnd();
}

static void setup_anim_chunk_size(DtCtx *c)
{
    animEnv(c);
    uint8_t *clump = dt_alloc(0x80);
    *(int32_t *)(clump + rpAnimClumpOffset) = dt_range(-2, 3);
    DT_ARG_PTR(c, 0, clump);
    rwtEnd();
}

static void *mkClumpSequences(void)
{
    void **head = (void **)dt_alloc(0x24);
    void **tail = head;
    for (int n = dt_range(0, 3); n > 0; n--) {
        void **s = (void **)dt_alloc(0x24);
        s[0] = dt_alloc(8);
        s[6] = (void *)(uintptr_t)dt_u32();
        s[7] = (void *)(uintptr_t)dt_u32();
        tail[8] = (void *)s;
        tail = s;
    }
    return (void *)head;
}

static void setup_anim_clump_copy(DtCtx *c)
{
    animEnv(c);
    dt_stub(0x004fc7fc, NULL, (void *)stAddSequence);
    uint8_t *from = dt_alloc(0x80);
    *(void **)(from + rpAnimClumpOffset + 0xc) = mkClumpSequences();
    *(void **)(from + rpAnimClumpOffset + 0x10) = mkClumpSequences();
    DT_ARG_PTR(c, 0, dt_alloc(0x80));
    DT_ARG_PTR(c, 1, from);
    rwtEnd();
}

static void setup_error_close(DtCtx *c)
{
    dt_seed(c);
    rwtEnv();
    *(int32_t *)(uintptr_t)0x00676d24 = dt_range(-2, 5);
    DT_ARG_PTR(c, 0, dt_alloc(8));
    rwtEnd();
}

DT_CASES(rwbone_cases) = {
    {"rpBoneFindAtomic", 0x00502b00, rpBoneFindAtomic, 2, DT_RET_INT, setup_find_atomic, 60, 0},
    {"rpBoneFindSkinAtomic", 0x00502b50, rpBoneFindSkinAtomic, 2, DT_RET_INT, setup_find_skin_atomic, 100, 0},
    {"rpBoneGet", (uint32_t)(uintptr_t)boneGetOrig, boneGetImpl, 1, DT_RET_INT, setup_bone_get, 60, 0},
    {"rpBoneSet", (uint32_t)(uintptr_t)boneSetOrig, boneSetImpl, 2, DT_RET_INT, setup_bone_set, 60, 0},
    {"rpBoneCreate", 0x00502bc0, rpBoneCreate, 0, DT_RET_INT, setup_bone_create, 60, 0},
    {"rpBoneDestroy", (uint32_t)(uintptr_t)boneDestroyOrig, boneDestroyImpl, 1, DT_RET_VOID,
     setup_bone_destroy, 100, 0},
    {"rpBoneFindFrame", 0x00502c90, rpBoneFindFrame, 2, DT_RET_INT, setup_find_frame, 100, 0},
    {"rpBoneAttachFrames", 0x00502ce0, rpBoneAttachFrames, 2, DT_RET_INT, setup_attach_frames, 300, 0},
    {"rpBoneEngineOpen", 0x00505500, rpBoneEngineOpen, 2, DT_RET_INT, setup_engine_open, 30, 0},
    {"rpBoneEngineClose", 0x00505520, rpBoneEngineClose, 1, DT_RET_INT, setup_engine_close, 30, 0},
    {"rpBoneGeometryInit", 0x00505530, rpBoneGeometryInit, 1, DT_RET_INT, setup_geometry_init, 40, 0},
    {"rpBoneGeometryDeinit", 0x00505550, rpBoneGeometryDeinit, 1, DT_RET_INT, setup_geometry_init, 150, 0},
    {"rpBoneGeometryCopy", 0x005055e0, rpBoneGeometryCopy, 1, DT_RET_INT, setup_geometry_copy, 10, 0},
    {"rpBoneGeometryWrite", 0x005055f0, rpBoneGeometryWrite, 3, DT_RET_INT, setup_geometry_write, 200, 0},
    {"rpBoneGeometrySize", 0x005057b0, rpBoneGeometrySize, 1, DT_RET_INT, setup_geometry_init, 100, 0},
    {"rpBoneFrameInit", 0x005057f0, rpBoneFrameInit, 1, DT_RET_INT, setup_frame_init, 30, 0},
    {"rpBoneFrameDeinit", 0x00505810, rpBoneFrameDeinit, 1, DT_RET_INT, setup_frame_deinit, 100, 0},
    {"rpBoneFrameCopy", 0x00505830, rpBoneFrameCopy, 2, DT_RET_INT, setup_frame_copy, 100, 0},
    {"rpAnimInterpolatorDeinit", 0x005029e0, rpAnimInterpolatorDeinit, 1, DT_RET_INT,
     setup_interpolator_deinit, 80, 0},
    {"rpAnimInterpolatorInit", 0x00502a30, rpAnimInterpolatorInit, 1, DT_RET_INT, setup_anim_interp_init, 30,
     0},
    {"rpAnimNodeCreate", 0x004fc4f0, rpAnimNodeCreate, 0, DT_RET_INT, setup_anim_node_create, 40, 0},
    {"rpAnimFrameInit", 0x004fbce0, rpAnimFrameInit, 1, DT_RET_INT, setup_anim_interp_init, 30, 0},
    {"rpAnimFrameDataCreate", 0x004fbca0, rpAnimFrameDataCreate, 0, DT_RET_INT, setup_anim_node_create, 40,
     0},
    {"rpAnimSequenceName", (uint32_t)(uintptr_t)seqNameOrig, seqNameImpl, 1, DT_RET_INT, setup_anim_sequence,
     30, 0},
    {"rpAnimSequenceType", 0x00500049, rpAnimSequenceType, 1, DT_RET_INT, setup_anim_sequence, 30, 0},
    {"rpAnimSequenceSize", (uint32_t)(uintptr_t)seqSizeOrig, seqSizeImpl, 1, DT_RET_INT, setup_anim_sequence,
     200, 0},
    {"rpAnimFrameChunkSize", 0x004fd6a0, rpAnimFrameChunkSize, 1, DT_RET_INT, setup_anim_frame_size, 200, 0},
    {"rpAnimClumpAddFrameSequence", 0x004fd770, rpAnimClumpAddFrameSequence, 2, DT_RET_INT,
     setup_anim_add_frame_seq, 50, 0},
    {"rpAnimClumpAddFrameSequences", (uint32_t)(uintptr_t)addFrameSeqsOrig, addFrameSeqsImpl, 2, DT_RET_INT,
     setup_anim_add_frame_seqs, 250, 0},
    {"rpAnimClumpAddFrameSequencesCallback", 0x004fd7b0, rpAnimClumpAddFrameSequencesCallback, 2, DT_RET_INT,
     setup_anim_add_frame_seqs, 250, 0},
    {"rpAnimClumpAddAllSequences", (uint32_t)(uintptr_t)addAllOrig, addAllImpl, 1, DT_RET_INT,
     setup_anim_add_all, 250, 0},
    {"rpAnimClumpChunkRead", 0x004fd870, rpAnimClumpChunkRead, 3, DT_RET_INT, setup_anim_chunk_read, 250, 0},
    {"rpAnimClumpChunkWrite", 0x004fd8c0, rpAnimClumpChunkWrite, 3, DT_RET_INT, setup_anim_chunk_write, 100,
     0},
    {"rpAnimClumpChunkSize", 0x004fd920, rpAnimClumpChunkSize, 1, DT_RET_INT, setup_anim_chunk_size, 60, 0},
    {"rpAnimClumpCopy", 0x004fc780, rpAnimClumpCopy, 2, DT_RET_INT, setup_anim_clump_copy, 150, 0},
    {"_rwErrorClose", 0x004dee20, _rwErrorClose, 1, DT_RET_INT, setup_error_close, 20, 0},
};
