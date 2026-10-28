/*
 * Tests for rw/plugins.c: the plugin registration wrappers, the attach routines, the skin helpers and the
 * RwMatrixInvert helpers. Callbacks and callees of other RenderWare groups are stubs that log their
 * arguments (dt_rw.h); the registries in .data are replaced by fixtures each iteration.
 */
#include "dt.h"
#include "dt_rw.h"
#include "rw/rwcore.h"
#include "rw/rwplugins.h"

/* ---- registration wrappers -------------------------------------------------------------------------- */

static void plugin_env(DtCtx *c)
{
    dt_seed(c);
    rwtEnv();
    rwPluginRegEntryFreeList = c->seed % 11 == 1 ? NULL : dt_alloc(sizeof(RwFreeList));
    rwEngineInstanceCount = c->seed % 9 == 2;
    gRwtEntryFailAt = c->seed % 13 == 3 ? dt_range(1, 9) : 0;
    rpGeometryModuleCount = c->seed % 10 == 4;
    rpWorldModuleCount = c->seed % 10 == 6;
}

/* Replaces a registry by a fixture with entries 0x100..; ids the register calls may already find. */
static void fill_registry(RwPluginRegistry *dst)
{
    *dst = *rwtRegistry(dt_range(0, 5), 0x100);
}

static void register_setup(DtCtx *c, RwPluginRegistry *registry, int withCopy)
{
    plugin_env(c);
    fill_registry(registry);
    DT_ARG_INT(c, 0, dt_u32() % 6 == 0 ? -dt_range(1, 9) : dt_range(0, 100));
    DT_ARG_INT(c, 1, dt_u32() % 2 ? 0x100 + dt_range(0, 5) : 0x300);
    DT_ARG_PTR(c, 2, rwtPickCtor());
    DT_ARG_PTR(c, 3, rwtPickDtor());
    if (withCopy) {
        DT_ARG_PTR(c, 4, rwtPickCopy());
    }
    rwtEnd();
}

static void stream_setup(DtCtx *c, RwPluginRegistry *registry)
{
    plugin_env(c);
    fill_registry(registry);
    DT_ARG_INT(c, 0, dt_u32() % 3 ? 0x100 + dt_range(0, 5) : 0x500);
    DT_ARG_PTR(c, 1, rwtPickRead());
    DT_ARG_PTR(c, 2, rwtPickWrite());
    DT_ARG_PTR(c, 3, rwtPickSize());
    rwtEnd();
}

#define REGISTER_SETUP(name, registry, withCopy)                                                             \
    static void setup_##name(DtCtx *c)                                                                       \
    {                                                                                                        \
        register_setup(c, &(registry), withCopy);                                                            \
    }
#define STREAM_SETUP(name, registry)                                                                         \
    static void setup_##name(DtCtx *c)                                                                       \
    {                                                                                                        \
        stream_setup(c, &(registry));                                                                        \
    }

REGISTER_SETUP(atomic_register, rpAtomicRegistry, 1)
REGISTER_SETUP(clump_register, rpClumpRegistry, 1)
REGISTER_SETUP(geometry_register, rpGeometryRegistry, 1)
REGISTER_SETUP(world_register, rpWorldRegistry, 1)
REGISTER_SETUP(sector_register, rpWorldSectorRegistry, 1)
REGISTER_SETUP(light_register, rpLightRegistry, 1)
REGISTER_SETUP(frame_register, rwFrameRegistry, 1)
REGISTER_SETUP(camera_register, rwCameraRegistry, 1)
REGISTER_SETUP(texture_register, rwTextureRegistry, 1)
REGISTER_SETUP(engine_register, rwEngineRegistry, 0)
STREAM_SETUP(atomic_stream, rpAtomicRegistry)
STREAM_SETUP(clump_stream, rpClumpRegistry)
STREAM_SETUP(geometry_stream, rpGeometryRegistry)
STREAM_SETUP(world_stream, rpWorldRegistry)
STREAM_SETUP(sector_stream, rpWorldSectorRegistry)
STREAM_SETUP(frame_stream, rwFrameRegistry)

/* ---- attach routines -------------------------------------------------------------------------------- */

static int BMX_CDECL stub_ext_core(void)
{
    rwtLog(0x4eac90);
    return (int)(dt_u32() % 5) - 1;
}

static int BMX_CDECL stub_ext_world(void)
{
    rwtLog(0x4c5ab0);
    return (int)(dt_u32() % 5) - 1;
}

/* Maybe pre-fills a registry with plugin ids around `base`, so some plugins are found already registered. */
static void maybe_fill(RwPluginRegistry *registry, int32_t base)
{
    if (dt_u32() % 4 == 0) {
        *registry = *rwtRegistry(dt_range(1, 6), base);
    }
}

static void attach_env(DtCtx *c)
{
    plugin_env(c);
    dt_stub(0x004eac90, NULL, (void *)stub_ext_core);
    dt_stub(0x004c5ab0, NULL, (void *)stub_ext_world);
    maybe_fill(&rwEngineRegistry, 0x401);
    maybe_fill(&rwFrameRegistry, 0x106);
    maybe_fill(&rpClumpRegistry, 0x108);
    maybe_fill(&rpAtomicRegistry, 0x10e);
    maybe_fill(&rpGeometryRegistry, 0x10e);
    maybe_fill(&rpWorldRegistry, 0x100);
    maybe_fill(&rpWorldSectorRegistry, 0x100);
    maybe_fill(&rpLightRegistry, 0x500);
    maybe_fill(&rwCameraRegistry, 0x500);
    rpBoneStreamResult = (int32_t)dt_u32();
    rpLodCamera = (void *)(uintptr_t)dt_u32();
    rpLodReserved = (int32_t)dt_u32();
    rwtEnd();
}

static void setup_attach(DtCtx *c)
{
    attach_env(c);
}

static void setup_ext_attach(DtCtx *c)
{
    dt_seed(c);
    gRwtLogN = 0;
    dt_stub(0x004c5ab0, NULL, (void *)stub_ext_world);
}

static void setup_nothing(DtCtx *c)
{
    dt_seed(c);
}

/* ---- morph targets ---------------------------------------------------------------------------------- */

static void setup_morph_sphere(DtCtx *c)
{
    plugin_env(c);
    RpMorphTarget *morph = dt_alloc(sizeof *morph);
    dt_fill(morph, sizeof *morph);
    RwSphere *sphere = dt_alloc(sizeof *sphere);
    dt_fill(sphere, sizeof *sphere);
    DT_ARG_PTR(c, 0, dt_u32() % 5 == 0 ? NULL : morph);
    DT_ARG_PTR(c, 1, dt_u32() % 5 == 0 ? NULL : sphere);
    rwtEnd();
}

static void setup_morph_normals(DtCtx *c)
{
    plugin_env(c);
    RpMorphTarget *morph = dt_alloc(sizeof *morph);
    dt_fill(morph, sizeof *morph);
    DT_ARG_PTR(c, 0, dt_u32() % 5 == 0 ? NULL : morph);
    rwtEnd();
}

/* ---- frames ----------------------------------------------------------------------------------------- */

static int gForAllStopAt, gForAllCalls;

static void *BMX_CDECL for_all_cb(void *object, void *data)
{
    rwtLogPtr(object);
    rwtLogPtr(data);
    gForAllCalls++;
    return gForAllStopAt != 0 && gForAllCalls == gForAllStopAt ? NULL : object;
}

static void setup_for_all_objects(DtCtx *c)
{
    plugin_env(c);
    RwFrame *frame = dt_alloc(0x80);
    frame->objectList.next = frame->objectList.prev = &frame->objectList;
    const int n = dt_range(0, 5);
    for (int i = 0; i < n; i++) {
        RwObjectHasFrame *o = dt_alloc(sizeof *o);
        o->lFrame.next = &frame->objectList;
        o->lFrame.prev = frame->objectList.prev;
        frame->objectList.prev->next = &o->lFrame;
        frame->objectList.prev = &o->lFrame;
    }
    gForAllCalls = 0;
    gForAllStopAt = dt_u32() % 2 ? dt_range(1, 6) : 0;
    DT_ARG_PTR(c, 0, frame);
    DT_ARG_PTR(c, 1, for_all_cb);
    DT_ARG_PTR(c, 2, dt_alloc(8));
    rwtEnd();
}

static void setup_user_data_get(DtCtx *c)
{
    dt_seed(c);
    rpAnimFrameOffset = dt_range(0, 0x20) & ~3;
    uint8_t *frame = dt_alloc(0x80);
    dt_fill(frame, 0x80);
    DT_ARG_PTR(c, 0, frame);
}

static void setup_user_data_set(DtCtx *c)
{
    setup_user_data_get(c);
    DT_ARG_INT(c, 1, dt_u32());
}

static void setup_lod_camera(DtCtx *c)
{
    dt_seed(c);
    rpLodCamera = (void *)(uintptr_t)dt_u32();
    DT_ARG_INT(c, 0, dt_u32());
}

/* ---- vector length ---------------------------------------------------------------------------------- */

static float edge_float(void)
{
    static const uint32_t bits[] = {0x00000000, 0x80000000, 0x00000001, 0x007fffff, 0x7f7fffff,
                                    0x7f800000, 0xff800000, 0x3f800000, 0x1e3ce508, 0x5f000000};
    switch (dt_u32() % 5) {
    case 0: {
        float f;
        const uint32_t b = bits[dt_u32() % (sizeof bits / sizeof bits[0])];
        __builtin_memcpy(&f, &b, 4);
        return f;
    }
    case 1:
        return dt_float(-1e20f, 1e20f);
    default:
        return dt_float(-100.0f, 100.0f);
    }
}

static void setup_v3d_length(DtCtx *c)
{
    dt_seed(c);
    RwGlobals *g = rwtEnv();
    uint32_t *table = dt_alloc(4096 * 4);
    dt_fill(table, 4096 * 4);
    *(uint32_t **)((uint8_t *)g + kRwtSqrtOffset) = table;
    ZVec3 *v = dt_alloc(sizeof *v);
    v->x = edge_float();
    v->y = edge_float();
    v->z = edge_float();
    if (dt_u32() % 6 == 0) {
        v->x = v->y = v->z = 0.0f;
    }
    DT_ARG_PTR(c, 0, v);
    rwtEnd();
}

/* ---- skin helpers ----------------------------------------------------------------------------------- */

static void *gFoundFrame;

static void *BMX_CDECL stub_children(void *frame, void *callback, void *data)
{
    rwtLog(0x4d2eb0);
    rwtLogPtr(frame);
    rwtLogPtr(callback);
    rwtLog(((uint32_t *)data)[1]);
    *(void **)data = gFoundFrame;
    return frame;
}

static void setup_find_bone_frames(DtCtx *c)
{
    plugin_env(c);
    dt_stub(0x004d2eb0, NULL, (void *)stub_children);
    gFoundFrame = dt_u32() % 3 ? dt_alloc(16) : NULL;
    const int n = dt_range(0, 4);
    RpSkinBoneBlock *blocks = dt_alloc(sizeof(RpSkinBoneBlock) * (uint32_t)(n + 1));
    for (int i = 0; i < n; i++) {
        blocks[i].firstVertex = dt_range(0, 50);
        blocks[i].numVertices = dt_range(1, 30);
        blocks[i].key = (int32_t)dt_u32();
        blocks[i].bone = (RpSkinBone *)(uintptr_t)dt_u32();
    }
    DT_ARG_PTR(c, 0, dt_u32() % 8 == 0 ? NULL : blocks);
    DT_ARG_PTR(c, 1, dt_u32() % 8 == 0 ? NULL : dt_alloc(0x80));
    rwtEnd();
}

typedef struct SkinWorld {
    RpClump *clump;
    RwFrame *frame;
    RpAtomic *atomic;
    uint8_t *geometry;
    RpMorphTarget *morph;
    ZVec3 *verts;
    ZVec3 *normals;
    int32_t numVertices;
    int found; /* the atomic search finds an atomic */
} SkinWorld;

static SkinWorld gSkin;

static void *BMX_CDECL stub_find_atomic(void *object, void *data)
{
    rwtLog(0x502b00);
    rwtLogPtr(object);
    *(void **)data = gSkin.found ? (void *)gSkin.atomic : NULL;
    return NULL;
}

static void *BMX_CDECL stub_find_update_atomic(void *object, void *data)
{
    rwtLog(0x502b50);
    rwtLogPtr(object);
    *(void **)data = gSkin.found ? (void *)gSkin.atomic : NULL;
    return NULL;
}

static void *BMX_CDECL stub_get_morph(void *geometry, int32_t index)
{
    rwtLog(0x4b43e0);
    rwtLogPtr(geometry);
    rwtLog((uint32_t)index);
    return gSkin.morph;
}

static ZVec3 *BMX_CDECL stub_get_verts(const RpMorphTarget *morph)
{
    rwtLog(0x4b45d0);
    rwtLogPtr(morph);
    return morph->verts;
}

static int32_t BMX_CDECL stub_num_vertices(const void *geometry)
{
    rwtLog(0x4b4550);
    rwtLogPtr(geometry);
    return gSkin.numVertices;
}

static int gLockFails;

static void *BMX_CDECL stub_lock(void *geometry, int32_t mode)
{
    rwtLog(0x4b4f20);
    rwtLogPtr(geometry);
    rwtLog((uint32_t)mode);
    return gLockFails ? NULL : geometry;
}

static void *BMX_CDECL stub_unlock(void *geometry)
{
    rwtLog(0x4b4fd0);
    rwtLogPtr(geometry);
    return geometry;
}

static void *BMX_CDECL stub_children_update(void *frame, void *callback, void *data)
{
    const RwMatrix *m = ((RwMatrix **)data)[1];
    rwtLog(0x4d2eb0);
    rwtLogPtr(frame);
    rwtLogPtr(callback);
    rwtLogPtr(m);
    rwtLog(((uint32_t *)data)[2]);
    rwtLogWords(m, 16);
    return frame;
}

static RwMatrix *BMX_CDECL stub_invert(RwMatrix *out, const RwMatrix *in)
{
    rwtLog(0x4d1340);
    rwtLogPtr(out);
    rwtLogPtr(in);
    rwtLogWords(in, 16);
    uint32_t *w = (uint32_t *)out;
    for (int i = 0; i < 16; i++) {
        w[i] = ((const uint32_t *)in)[i] * 7u + (uint32_t)i;
    }
    return out;
}

static void *BMX_CDECL stub_transform_points(ZVec3 *out, const ZVec3 *in, int32_t count, const RwMatrix *m)
{
    rwtLog(0x4d3a90);
    rwtLogPtr(out);
    rwtLogPtr(in);
    rwtLog((uint32_t)count);
    rwtLogPtr(m);
    for (int32_t i = 0; i < count; i++) {
        out[i].x = in[i].x + m->pos.x;
        out[i].y = in[i].y + m->pos.y;
        out[i].z = in[i].z + m->pos.z;
    }
    return out;
}

static void *BMX_CDECL stub_transform_vectors(ZVec3 *out, const ZVec3 *in, int32_t count, const RwMatrix *m)
{
    rwtLog(0x4d3ae0);
    rwtLogPtr(out);
    rwtLogPtr(in);
    rwtLog((uint32_t)count);
    rwtLogPtr(m);
    for (int32_t i = 0; i < count; i++) {
        out[i].x = in[i].x * m->right.x;
        out[i].y = in[i].y * m->up.y;
        out[i].z = in[i].z * m->at.z;
    }
    return out;
}

static RwBBox *BMX_CDECL stub_bbox(RwBBox *bbox, const ZVec3 *points, int32_t count)
{
    rwtLog(0x4e9760);
    rwtLogPtr(bbox);
    rwtLogPtr(points);
    rwtLog((uint32_t)count);
    bbox->sup.x = edge_float();
    bbox->sup.y = edge_float();
    bbox->sup.z = edge_float();
    bbox->inf.x = edge_float();
    bbox->inf.y = edge_float();
    bbox->inf.z = edge_float();
    return bbox;
}

/* The clump, its frame with one object, the skinned atomic and its geometry with the bone plugin data. */
static void skin_world(DtCtx *c, int update)
{
    plugin_env(c);
    /* Finite results only: the emulator keeps a signalling NaN loaded by FLD, the hardware quiets it. */
    uint32_t *table = dt_alloc(4096 * 4);
    dt_fill(table, 4096 * 4);
    for (int i = 0; i < 4096; i++) {
        table[i] &= 0x007fffffu;
    }
    *(uint32_t **)((uint8_t *)RwEngineInstance + kRwtSqrtOffset) = table;
    rpGeometryModuleCount = dt_u32() % 8 != 0;
    memset(&gSkin, 0, sizeof gSkin);
    gSkin.clump = dt_alloc(sizeof(RpClump));
    gSkin.frame = dt_alloc(0xa0);
    gSkin.frame->objectList.next = gSkin.frame->objectList.prev = &gSkin.frame->objectList;
    gSkin.atomic = dt_alloc(sizeof(RpAtomic) + 0x20);
    dt_fill(gSkin.atomic, sizeof(RpAtomic));
    gSkin.atomic->object.object.parent = dt_alloc(0xa0);
    dt_fill(gSkin.atomic->object.object.parent, 0xa0);
    gSkin.geometry = dt_alloc(0x100);
    gSkin.atomic->geometry = (RpGeometry *)gSkin.geometry;
    gSkin.numVertices = dt_range(0, 12);
    gSkin.verts = dt_u32() % 8 == 0 ? NULL : dt_alloc(12u * 16u);
    gSkin.normals = dt_u32() % 3 == 0 ? NULL : dt_alloc(12u * 16u);
    if (gSkin.verts != NULL) {
        dt_fill(gSkin.verts, 12 * 16);
    }
    if (gSkin.normals != NULL) {
        dt_fill(gSkin.normals, 12 * 16);
    }
    gSkin.morph = dt_alloc(sizeof(RpMorphTarget));
    gSkin.morph->verts = gSkin.verts;
    gSkin.morph->normals = gSkin.normals;
    gSkin.found = dt_u32() % 8 != 0;
    if (dt_u32() % 8 != 0) { /* the frame has an object (callbacks are stubs, so which one does not matter) */
        RwObjectHasFrame *o = dt_alloc(sizeof *o);
        o->lFrame.next = &gSkin.frame->objectList;
        o->lFrame.prev = &gSkin.frame->objectList;
        gSkin.frame->objectList.next = gSkin.frame->objectList.prev = &o->lFrame;
        o->object.parent = gSkin.frame;
    }
    gSkin.clump->object.parent = dt_u32() % 10 == 0 ? NULL : gSkin.frame;
    rpBoneGeometryOffset = 0x40 + 4 * dt_range(0, 3);
    rpAnimFrameOffset = 0x20;
    gLockFails = update && dt_u32() % 6 == 0;
    dt_stub(update ? 0x00502b50 : 0x00502b00, NULL,
            (void *)(update ? stub_find_update_atomic : stub_find_atomic));
    dt_stub(0x004b43e0, NULL, (void *)stub_get_morph);
    dt_stub(0x004b45d0, NULL, (void *)stub_get_verts);
    dt_stub(0x004b4550, NULL, (void *)stub_num_vertices);
    dt_stub(0x004b4f20, NULL, (void *)stub_lock);
    dt_stub(0x004b4fd0, NULL, (void *)stub_unlock);
    gFoundFrame = dt_alloc(16);
    if (update) {
        dt_stub(0x004d2eb0, NULL, (void *)stub_children_update);
        dt_stub(0x004d1340, NULL, (void *)stub_invert);
        dt_stub(0x004d3a90, NULL, (void *)stub_transform_points);
        dt_stub(0x004d3ae0, NULL, (void *)stub_transform_vectors);
        dt_stub(0x004e9760, NULL, (void *)stub_bbox);
        /* bone plugin data: blocks of vertices with bones; the list ends with an empty block */
        RpSkinGeometry *skin = dt_alloc(sizeof *skin);
        const int blocks = dt_range(0, 3);
        RpSkinBoneBlock *list = dt_alloc(sizeof(RpSkinBoneBlock) * 5);
        skin->origNormals = dt_u32() % 3 == 0 ? NULL : dt_alloc(12u * 16u);
        /* only the pass with normals tolerates a block without a bone */
        const int bothNormals =
            skin->origNormals != NULL && gSkin.normals != NULL && rpGeometryModuleCount != 0;
        for (int i = 0; i < blocks; i++) {
            list[i].firstVertex = dt_range(0, 6);
            list[i].numVertices = dt_range(1, 6);
            list[i].bone = bothNormals && dt_u32() % 5 == 0 ? NULL : dt_alloc(sizeof(RpSkinBone));
            if (list[i].bone != NULL) {
                dt_fill(list[i].bone, sizeof(RpSkinBone));
                list[i].bone->flags = dt_u32();
            }
        }
        skin->blocks = dt_u32() % 8 == 0 ? NULL : list;
        skin->origVerts = dt_alloc(12u * 16u);
        dt_fill(skin->origVerts, 12 * 16);
        if (skin->origNormals != NULL) {
            dt_fill(skin->origNormals, 12 * 16);
        }
        *(RpSkinGeometry **)(gSkin.geometry + rpBoneGeometryOffset) = skin;
    } else {
        dt_stub(0x004d2eb0, NULL, (void *)stub_children);
        gFoundFrame = NULL;
        RpSkinGeometry *skin = dt_u32() % 3 == 0 ? NULL : dt_alloc(sizeof(RpSkinGeometry));
        if (skin != NULL) {
            skin->blocks = NULL;
        }
        *(RpSkinGeometry **)(gSkin.geometry + rpBoneGeometryOffset) = skin;
        gRwtMallocFailAt = dt_u32() % 5 == 0 ? (skin == NULL ? 2 : 1) + (int)(dt_u32() % 2) : 0;
    }
    DT_ARG_PTR(c, 0, gSkin.clump);
    rwtEnd();
}

static void setup_skin_setup(DtCtx *c)
{
    skin_world(c, 0);
}

static void setup_skin_update(DtCtx *c)
{
    skin_world(c, 1);
}

/* ---- RwMatrixInvert helpers ------------------------------------------------------------------------- */

/*
 * The two helpers take their arguments in registers, so they are tested through RwMatrixInvert (original
 * code): the shim below is its dispatcher, calling our helpers, the original calls the original ones.
 */
static RwMatrix *shim_matrix_invert(RwMatrix *out, const RwMatrix *in)
{
    const uint32_t identity = *(uint32_t *)((uint8_t *)RwEngineInstance + rwMatrixOffset + 4) & 0x20000;
    if ((in->flags & identity) != 0) {
        *out = *in;
    } else if ((in->flags & 3) == 3) {
        RwMatrixInvertOrthonormal(out, in);
    } else {
        RwMatrixInvertGeneral(out, in);
    }
    return out;
}

static float matrix_float(void)
{
    return dt_u32() % 7 == 0 ? edge_float() : dt_float_edgy(-50.0f, 50.0f);
}

static void invert_setup(DtCtx *c, uint32_t type)
{
    dt_seed(c);
    RwGlobals *g = rwtEnv();
    ((uint32_t *)((uint8_t *)g + kRwtMatrixOffset))[1] = 0x20000;
    RwMatrix *in = dt_alloc(sizeof *in);
    RwMatrix *out = dt_alloc(sizeof *out);
    float *f = (float *)in;
    for (int i = 0; i < 16; i++) {
        f[i] = matrix_float();
    }
    dt_fill(&in->pad1, 4);
    dt_fill(&out->pad1, 4);
    dt_fill(&out->flags, 4);
    in->flags = (dt_u32() & ~0x20003u) | type | (dt_u32() % 9 == 0 ? 0x20000u : 0);
    switch (dt_u32() % 8) {
    case 0: /* singular: two equal rows */
        in->at = in->up;
        break;
    case 1: /* singular: a zero row */
        in->up.x = in->up.y = in->up.z = 0.0f;
        break;
    case 2: /* a pure rotation about z, translated */
    {
        const float s = dt_float(-1.0f, 1.0f);
        const float co = dt_float(-1.0f, 1.0f);
        in->right.x = co;
        in->right.y = s;
        in->right.z = 0.0f;
        in->up.x = -s;
        in->up.y = co;
        in->up.z = 0.0f;
        in->at.x = 0.0f;
        in->at.y = 0.0f;
        in->at.z = 1.0f;
        break;
    }
    default:
        break;
    }
    DT_ARG_PTR(c, 0, dt_u32() % 6 == 0 ? in : out);
    DT_ARG_PTR(c, 1, in);
    rwtEnd();
}

static void setup_invert_orthonormal(DtCtx *c)
{
    invert_setup(c, 3);
}

static void setup_invert_general(DtCtx *c)
{
    invert_setup(c, dt_u32() % 3);
}

DT_CASES(rwplugins_cases) = {
    {"RpAtomicRegisterPlugin", 0x004b3190, RpAtomicRegisterPlugin, 5, DT_RET_INT, setup_atomic_register, 150,
     0},
    {"RpClumpRegisterPlugin", 0x004b31e0, RpClumpRegisterPlugin, 5, DT_RET_INT, setup_clump_register, 150, 0},
    {"RpAtomicRegisterPluginStream", 0x004b3230, RpAtomicRegisterPluginStream, 4, DT_RET_INT,
     setup_atomic_stream, 150, 0},
    {"RpClumpRegisterPluginStream", 0x004b3270, RpClumpRegisterPluginStream, 4, DT_RET_INT,
     setup_clump_stream, 150, 0},
    {"RpMorphTargetSetBoundingSphere", 0x004b3e90, RpMorphTargetSetBoundingSphere, 2, DT_RET_INT,
     setup_morph_sphere, 150, 0},
    {"RpMorphTargetGetVertexNormals", 0x004b4640, RpMorphTargetGetVertexNormals, 1, DT_RET_INT,
     setup_morph_normals, 100, 0},
    {"RpGeometryRegisterPlugin", 0x004b5f60, RpGeometryRegisterPlugin, 5, DT_RET_INT, setup_geometry_register,
     300, 0},
    {"RpGeometryRegisterPluginStream", 0x004b6020, RpGeometryRegisterPluginStream, 4, DT_RET_INT,
     setup_geometry_stream, 300, 0},
    {"_rpWorldObjRegisterExtensions", 0x004b8e00, _rpWorldObjRegisterExtensions, 0, DT_RET_INT, setup_attach,
     300, 0},
    {"RpWorldRegisterPlugin", 0x004bbe40, RpWorldRegisterPlugin, 5, DT_RET_INT, setup_world_register, 300, 0},
    {"RpWorldRegisterPluginStream", 0x004bbf00, RpWorldRegisterPluginStream, 4, DT_RET_INT,
     setup_world_stream, 300, 0},
    {"RpWorldPluginAttach", 0x004bc1c0, RpWorldPluginAttach, 0, DT_RET_INT, setup_attach, 300, 0},
    {"RpWorldSectorRegisterPlugin", 0x004c2250, RpWorldSectorRegisterPlugin, 5, DT_RET_INT,
     setup_sector_register, 150, 0},
    {"RpWorldSectorRegisterPluginStream", 0x004c22a0, RpWorldSectorRegisterPluginStream, 4, DT_RET_INT,
     setup_sector_stream, 150, 0},
    {"RpLightRegisterPlugin", 0x004c2600, RpLightRegisterPlugin, 5, DT_RET_INT, setup_light_register, 150, 0},
    {"rpWorldExtPluginAttach", 0x004c46a0, rpWorldExtPluginAttach, 0, DT_RET_INT, setup_ext_attach, 20, 0},
    {"rpWorldExtGetFuncs", 0x004c5ac0, rpWorldExtGetFuncs, 0, DT_RET_INT, setup_nothing, 1, 0},
    {"RpPVSPluginAttach", 0x004ccac0, RpPVSPluginAttach, 0, DT_RET_INT, setup_attach, 300, 0},
    {"RwEngineRegisterPlugin", 0x004cf4b0, RwEngineRegisterPlugin, 4, DT_RET_INT, setup_engine_register, 150,
     0},
    {"CorePluginAttach", 0x004cf520, CorePluginAttach, 0, DT_RET_INT, setup_attach, 300, 0},
    {"RwFrameRegisterPlugin", 0x004d33d0, RwFrameRegisterPlugin, 5, DT_RET_INT, setup_frame_register, 150, 0},
    {"RwFrameForAllObjects", 0x004d3370, RwFrameForAllObjects, 3, DT_RET_INT, setup_for_all_objects, 200, 0},
    {"RwV3dLength", 0x004d3810, RwV3dLength, 1, DT_RET_FLOAT, setup_v3d_length, 600, 0},
    {"RwCameraRegisterPlugin", 0x004ded30, RwCameraRegisterPlugin, 5, DT_RET_INT, setup_camera_register, 150,
     0},
    {"RwTextureRegisterPlugin", 0x004dffd0, RwTextureRegisterPlugin, 5, DT_RET_INT, setup_texture_register,
     150, 0},
    {"RwFrameRegisterPluginStream", 0x004e8f80, RwFrameRegisterPluginStream, 4, DT_RET_INT,
     setup_frame_stream, 150, 0},
    {"RwFrameGetUserData", 0x00500220, RwFrameGetUserData, 1, DT_RET_INT, setup_user_data_get, 30, 0},
    {"RwFrameSetUserData", 0x00500230, RwFrameSetUserData, 2, DT_RET_VOID, setup_user_data_set, 30, 0},
    {"RpAnimPluginAttach", 0x00500250, RpAnimPluginAttach, 0, DT_RET_INT, setup_attach, 300, 0},
    {"RpSkinClumpUpdate", 0x00504490, RpSkinClumpUpdate, 1, DT_RET_INT, setup_skin_update, 800, 0},
    {"RpBonePluginAttach", 0x00505870, RpBonePluginAttach, 0, DT_RET_INT, setup_attach, 300, 0},
    {"RpSkinClumpSetup", 0x00505950, RpSkinClumpSetup, 1, DT_RET_INT, setup_skin_setup, 400, 0},
    {"RpSkinFindBoneFrames", 0x00505ad0, RpSkinFindBoneFrames, 2, DT_RET_VOID, setup_find_bone_frames, 200,
     0},
    {"RpLODAtomicPluginAttach", 0x005062b0, RpLODAtomicPluginAttach, 0, DT_RET_INT, setup_attach, 300, 0},
    {"RpLODAtomicSetCamera", 0x005065d0, RpLODAtomicSetCamera, 1, DT_RET_VOID, setup_lod_camera, 20, 0},
    /* The register-argument helpers, through RwMatrixInvert (see shim_matrix_invert). */
    {"RwMatrixInvertOrthonormal", 0x004d1340, shim_matrix_invert, 2, DT_RET_INT, setup_invert_orthonormal,
     600, 0},
    {"RwMatrixInvertGeneral", 0x004d1340, shim_matrix_invert, 2, DT_RET_INT, setup_invert_general, 800, 0},
};
