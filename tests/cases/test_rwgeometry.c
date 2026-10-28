/*
 * Tests for rw/clump.c, part 2: geometries (accessors, lock/unlock, reference counting, creation through the
 * stream reader, destruction) and the mesh building they use. RpGeometryCreate takes register arguments and
 * is reached through RpGeometryStreamRead. Tri-strip generation (0x4c6490 and friends) is original code and
 * stays out: geometries here are triangle lists.
 */
#include "dt_rwclump.h"

static void clModuleOpen(void)
{
    rpGeometryModuleCount = dt_u32() % 8 == 0 ? 0 : 1;
}

/* A geometry, NULL one time in ten; the module is closed one time in eight. */
static RpGeometry *clMaybeGeometry(void)
{
    clModuleOpen();
    if (dt_u32() % 10 == 0) {
        return NULL;
    }
    RpGeometry *g = clGeometry(dt_range(0, 5), dt_range(0, 4), dt_u32() & 0x1e, dt_range(0, 3));
    return g;
}

static void setup_geometry(DtCtx *c)
{
    clEnv(c);
    DT_ARG_PTR(c, 0, clMaybeGeometry());
    rwtEnd();
}

static void setup_geometry_index(DtCtx *c)
{
    clEnv(c);
    RpGeometry *g = clMaybeGeometry();
    DT_ARG_PTR(c, 0, g);
    DT_ARG_INT(c, 1, dt_range(-2, 6));
    rwtEnd();
}

static void setup_morph_vertices(DtCtx *c)
{
    clEnv(c);
    clModuleOpen();
    RpGeometry *g = clGeometry(2, 1, 0, 0);
    DT_ARG_PTR(c, 0, dt_u32() % 8 == 0 || g->numMorphTargets == 0 ? NULL : &g->morphTarget[0]);
    rwtEnd();
}

static void setup_lock(DtCtx *c)
{
    clEnv(c);
    RpGeometry *g = clMaybeGeometry();
    if (g != NULL && dt_u32() % 2) {
        RpMeshHeader *header = dt_alloc(0x40);
        header->flags = dt_u32() % 2;
        header->numMeshes = (uint16_t)(dt_u32() % 2);
        g->mesh = header;
    }
    DT_ARG_PTR(c, 0, g);
    DT_ARG_INT(c, 1, (int32_t)(dt_u32() % 4 == 0 ? dt_u32() : dt_u32() & 0x3f));
    rwtEnd();
}

static void setup_unlock(DtCtx *c)
{
    clEnv(c);
    clModuleOpen();
    RpGeometry *g = dt_u32() % 10 == 0
                        ? NULL
                        : clGeometry(dt_range(0, 5), dt_range(0, 4), dt_u32() & 0x1e, dt_range(1, 3));
    if (g != NULL) {
        if (dt_u32() % 3 == 0) {
            g->mesh = dt_alloc(0x20);
        }
        for (int i = 0; i < g->numTriangles; i++) {
            if (g->triangles[i].matIndex < 0 || g->triangles[i].matIndex >= g->matList.numMaterials) {
                g->triangles[i].matIndex = (int16_t)(g->matList.numMaterials - 1);
            }
        }
    }
    gClAllocFailAt = dt_u32() % 8 == 0 ? dt_range(1, 2) : 0;
    gClMallocFailAt = dt_u32() % 8 == 0 ? dt_range(1, 3) : 0;
    DT_ARG_PTR(c, 0, g);
    rwtEnd();
}

static void setup_set_flags(DtCtx *c)
{
    clEnv(c);
    DT_ARG_PTR(c, 0, clMaybeGeometry());
    DT_ARG_INT(c, 1, dt_u32());
    rwtEnd();
}

static void setup_destroy(DtCtx *c)
{
    clEnv(c);
    clRegistryStubs();
    dt_stub(0x004b9290, NULL, (void *)stMaterialDestroy);
    RpGeometry *g = clMaybeGeometry();
    if (g != NULL) {
        g->numMorphTargets = dt_range(0, 3);
        g->refCount = (uint16_t)(dt_u32() % 3 == 0 ? 2 : 1);
        if (dt_u32() % 5 == 0) {
            g->mesh = dt_alloc(0x20);
        }
        for (int i = 0; i < g->numTriangles; i++) {
            if (g->triangles[i].matIndex >= g->matList.numMaterials) {
                g->triangles[i].matIndex = (int16_t)(g->matList.numMaterials - 1);
            }
        }
    }
    DT_ARG_PTR(c, 0, g);
    rwtEnd();
}

/* ---- reading a geometry ----------------------------------------------------------------------------- */

static RpMaterialList *BMX_CDECL stMatListRead(void *stream, RpMaterialList *list)
{
    rwtLog(0x4c5330);
    rwtLogPtr(stream);
    rwtLogPtr(list);
    if (clFails(kClStubMesh)) {
        return NULL;
    }
    list->materials = (RpMaterial **)dt_alloc(0x40);
    list->numMaterials = list->space = 4;
    return list;
}

static RpMaterial *BMX_CDECL stMaterialAddRef(RpMaterial *m)
{
    rwtLog(0x4b91f0);
    rwtLogPtr(m);
    return m;
}

static void setup_geometry_read(DtCtx *c)
{
    clEnv(c);
    clModuleOpen();
    clRegistryStubs();
    clStreamScript();
    for (int i = 0; i < 1024; i += 4) {
        gClStream[i] = (uint8_t)(dt_u32() % 4);
        gClStream[i + 1] = gClStream[i + 2] = gClStream[i + 3] = 0;
    }
    gClStream[0] = (uint8_t)(dt_u32() % 2 ? 0 : (dt_u32() & 0x1e)); /* format flags, no strips */
    gClStream[4] = (uint8_t)(dt_u32() % 4);                         /* triangles */
    gClStream[8] = (uint8_t)(dt_u32() % 4);                         /* vertices */
    gClStream[12] = (uint8_t)(dt_range(0, 3));                      /* morph targets */
    for (int i = 0; i < 16; i++) {
        gClLengths[i] = 0x1c;
    }
    for (int i = 0; i < 8; i++) {
        gClFailAt[i] = dt_u32() % 7 == 0 ? dt_range(1, 3) : 0;
        gClFailCalls[i] = 0;
    }
    gClAllocFailAt = dt_u32() % 10 == 0 ? dt_range(1, 3) : 0;
    gClMallocFailAt = dt_u32() % 8 == 0 ? dt_range(1, 8) : 0;
    dt_stub(0x004c5330, NULL, (void *)stMatListRead);
    dt_stub(0x004b91f0, NULL, (void *)stMaterialAddRef);
    dt_stub(0x004b9290, NULL, (void *)stMaterialDestroy);
    dt_stub(0x004c5640, NULL, (void *)stMaterialDestroy);
    DT_ARG_PTR(c, 0, dt_u32() % 12 == 0 ? NULL : dt_alloc(8));
    rwtEnd();
}

/* ---- mesh building ---------------------------------------------------------------------------------- */

static void setup_build_create(DtCtx *c)
{
    clEnv(c);
    gClMallocFailAt = dt_u32() % 5 == 0 ? dt_range(1, 2) : 0;
    DT_ARG_INT(c, 0, dt_u32() % 4 == 0 ? 0 : dt_range(0, 9));
    rwtEnd();
}

static RpBuildMesh *clBuild(int capacity, int n)
{
    RpMaterial *pool[3] = {clMaterial(), clMaterial(), clMaterial()};
    RpBuildMesh *b = dt_alloc(sizeof *b);
    b->capacity = (uint32_t)capacity;
    b->numTriangles = (uint32_t)n;
    b->triangles = dt_alloc((uint32_t)capacity * 12 + 0x40);
    for (int i = 0; i < n; i++) {
        b->triangles[i].vertIndex[0] = (uint16_t)dt_range(0, 9);
        b->triangles[i].vertIndex[1] = (uint16_t)dt_range(0, 9);
        b->triangles[i].vertIndex[2] = (uint16_t)dt_range(0, 9);
        b->triangles[i].material = pool[dt_u32() % 3];
    }
    return b;
}

static void setup_build_destroy(DtCtx *c)
{
    clEnv(c);
    RpBuildMesh *b = clBuild(3, dt_range(0, 3));
    if (dt_u32() % 3 == 0) {
        b->triangles = NULL;
    }
    DT_ARG_PTR(c, 0, b);
    rwtEnd();
}

static void setup_header_destroy(DtCtx *c)
{
    clEnv(c);
    RpMeshHeader *h = dt_alloc(sizeof *h + 8);
    h->flags = dt_u32() % 2 ? 0 : dt_u32();
    h->numMeshes = (uint16_t)(dt_u32() % 2 ? 0 : dt_u32());
    h->serialNum = (uint16_t)(dt_u32() % 2 ? 0 : dt_u32());
    h->totalIndices = dt_u32() % 2 ? 0 : dt_u32();
    h->firstMeshOffset = dt_u32() % 2 ? 0 : dt_u32();
    DT_ARG_PTR(c, 0, h);
    rwtEnd();
}

static void setup_add_triangle(DtCtx *c)
{
    clEnv(c);
    const int n = dt_range(0, 4);
    RpBuildMesh *b = clBuild(dt_u32() % 2 ? n : n + 2, n);
    if (n == 0 && dt_u32() % 2) {
        b->triangles = NULL;
        b->capacity = 0;
    }
    gClOldSize = 0x80;
    gClMallocFailAt = dt_u32() % 6 == 0 ? 1 : 0;
    DT_ARG_PTR(c, 0, b);
    DT_ARG_PTR(c, 1, (void *)(uintptr_t)(0x2000 + dt_u32() % 64));
    DT_ARG_INT(c, 2, dt_u32() & 0xffff);
    DT_ARG_INT(c, 3, dt_u32() & 0xffff);
    DT_ARG_INT(c, 4, dt_u32() & 0xffff);
    rwtEnd();
}

static int BMX_CDECL clMeshCb(RpMesh *mesh, RpMeshHeader *header, void *data)
{
    rwtLogPtr(mesh);
    rwtLogPtr(header);
    rwtLogPtr(data);
    rwtLog(mesh->numIndices);
    return (int)(dt_u32() % 5 != 0);
}

static void setup_for_all_meshes(DtCtx *c)
{
    clEnv(c);
    RpMeshHeader *h = dt_alloc(0x100);
    h->numMeshes = (uint16_t)dt_range(0, 4);
    h->firstMeshOffset = (uint32_t)dt_range(0, 2) * 4;
    dt_fill((uint8_t *)h + sizeof *h, 0x60);
    DT_ARG_PTR(c, 0, h);
    DT_ARG_PTR(c, 1, clMeshCb);
    DT_ARG_PTR(c, 2, dt_alloc(8));
    rwtEnd();
}

/* The triangle list generator: a build mesh whose materials come from a small set. */
static void setup_trilist(DtCtx *c)
{
    clEnv(c);
    RpBuildMesh *b = clBuild(5, dt_range(1, 5));
    gClMallocFailAt = dt_u32() % 7 == 0 ? dt_range(1, 2) : 0;
    DT_ARG_PTR(c, 0, b);
    DT_ARG_INT(c, 1, 0);
    rwtEnd();
}

static void setup_generate(DtCtx *c)
{
    clEnv(c);
    RpBuildMesh *b = clBuild(5, dt_range(0, 5));
    if (dt_u32() % 8 == 0) {
        b = NULL;
    }
    gClMallocFailAt = dt_u32() % 7 == 0 ? dt_range(1, 2) : 0;
    DT_ARG_PTR(c, 0, b);
    DT_ARG_INT(c, 1, 0);
    rwtEnd();
}

DT_CASES(rwgeometry_cases) = {
    {"RpGeometryGetMorphTarget", 0x004b43e0, RpGeometryGetMorphTarget, 2, DT_RET_INT, setup_geometry_index,
     200, 0},
    {"RpGeometryGetTexCoords", 0x004b44e0, RpGeometryGetTexCoords, 1, DT_RET_INT, setup_geometry, 60, 0},
    {"RpGeometryGetNumVertices", 0x004b4550, RpGeometryGetNumVertices, 1, DT_RET_INT, setup_geometry, 60, 0},
    {"RpMorphTargetGetVertices", 0x004b45d0, RpMorphTargetGetVertices, 1, DT_RET_INT, setup_morph_vertices,
     60, 0},
    {"RpGeometryGetTriangles", 0x004b46b0, RpGeometryGetTriangles, 1, DT_RET_INT, setup_geometry, 60, 0},
    {"RpGeometryGetNumTriangles", 0x004b4720, RpGeometryGetNumTriangles, 1, DT_RET_INT, setup_geometry, 60,
     0},
    {"RpGeometryGetMaterial", 0x004b47a0, RpGeometryGetMaterial, 2, DT_RET_INT, setup_geometry_index, 200, 0},
    {"RpGeometryGetNumMaterials", 0x004b4ce0, RpGeometryGetNumMaterials, 1, DT_RET_INT, setup_geometry, 60,
     0},
    {"RpGeometryLock", 0x004b4f20, RpGeometryLock, 2, DT_RET_INT, setup_lock, 200, 0},
    {"RpGeometryUnlock", 0x004b4fd0, RpGeometryUnlock, 1, DT_RET_INT, setup_unlock, 400, 0},
    {"RpGeometryGetFlags", 0x004b6140, RpGeometryGetFlags, 1, DT_RET_INT, setup_geometry, 60, 0},
    {"RpGeometrySetFlags", 0x004b61b0, RpGeometrySetFlags, 2, DT_RET_INT, setup_set_flags, 100, 0},
    {"RpGeometryAddRef", 0x004b5a30, RpGeometryAddRef, 1, DT_RET_INT, setup_geometry, 60, 0},
    {"RpGeometryDestroy", 0x004b5ab0, RpGeometryDestroy, 1, DT_RET_INT, setup_destroy, 400, 0},
    {"RpGeometryStreamRead", 0x004b68e0, RpGeometryStreamRead, 1, DT_RET_INT, setup_geometry_read, 1500, 0},
    {"RpBuildMeshCreate", 0x004c4750, RpBuildMeshCreate, 1, DT_RET_INT, setup_build_create, 80, 0},
    {"RpBuildMeshDestroy", 0x004c4830, RpBuildMeshDestroy, 1, DT_RET_INT, setup_build_destroy, 60, 0},
    {"_rpMeshHeaderDestroy", 0x004c4870, _rpMeshHeaderDestroy, 1, DT_RET_INT, setup_header_destroy, 80, 0},
    {"_rpBuildMeshAddTriangle", 0x004c48c0, _rpBuildMeshAddTriangle, 5, DT_RET_INT, setup_add_triangle, 200,
     0},
    {"_rpMeshHeaderForAllMeshes", 0x004c49d0, _rpMeshHeaderForAllMeshes, 3, DT_RET_INT, setup_for_all_meshes,
     150, 0},
    {"_rpMeshGenerateTriList", 0x004c5e30, _rpMeshGenerateTriList, 2, DT_RET_INT, setup_trilist, 250, 0},
    {"_rpMeshGenerate", 0x004c4a30, _rpMeshGenerate, 2, DT_RET_INT, setup_generate, 250, 0},
};
