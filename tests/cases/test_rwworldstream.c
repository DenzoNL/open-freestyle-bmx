/*
 * Tests for rw/clump.c, part 4: the binary world (RpWorld*, sectors, plane nodes, collision trees). The
 * sector and plane readers/writers and the size helpers take register arguments and are reached through
 * RpWorldStreamRead / RpWorldStreamWrite. Tri-strip generation stays out (world format bit 0 is clear).
 */
#include "dt_rwclump.h"

static RpMaterial *gClMats8[8];
static int32_t BMX_CDECL stMatListSize(const RpMaterialList *list);
static void *BMX_CDECL stMatListWrite(const RpMaterialList *list, void *stream);
static int32_t gTreeNumbers[8];
static int gTreeCalls;

static int32_t BMX_CDECL stTreeSizeFull(const uint8_t *tree)
{
    rwtLog(0x4bb760);
    rwtLogPtr(tree);
    return tree == NULL ? 0 : 0x180;
}

static int32_t BMX_CDECL stTreeSizeScripted(const uint8_t *tree)
{
    rwtLog(0x4bb760);
    rwtLogPtr(tree);
    return tree == NULL ? 0 : gTreeNumbers[gTreeCalls++ & 7];
}

static int32_t BMX_CDECL stRegistrySize2(const void *registry, const void *object)
{
    rwtLog(0x4e8d60);
    rwtLogPtr(registry);
    rwtLogPtr(object);
    return (int32_t)(dt_canon((uint32_t)(uintptr_t)registry) % 7) * 8;
}

/* A collision tree of 0x180 bytes whose walk reaches node 127 (so its size is the full 0x180). */
static uint8_t *clFullTree(void)
{
    uint8_t *t = dt_alloc(0x180);
    dt_fill(t, 0x180);
    for (int i = 1; i < 128; i++) {
        t[i * 3] = (uint8_t)(t[i * 3] & 0x7f);
    }
    for (int i = 1; i < 64; i = i * 2 + 1) {
        t[i * 3] |= 0x80;
    }
    return t;
}

static RpWorldSector *clSector(void)
{
    RpWorldSector *s = dt_alloc(sizeof *s + 8);
    dt_fill(s, sizeof *s);
    s->type = -1;
    s->numVertices = (uint16_t)dt_range(0, 3);
    s->numPolygons = (uint16_t)dt_range(0, 3);
    s->matListWindowBase = (uint16_t)dt_range(0, 1);
    s->vertices = dt_alloc(0x80);
    s->normals = dt_alloc(0x40);
    s->preLitLum = dt_alloc(0x40);
    s->texCoords = dt_alloc(0x80);
    dt_fill(s->vertices, 0x80);
    uint8_t *polygons = dt_alloc(0x40);
    s->polygons = (RpPolygon *)polygons;
    for (int i = 0; i < 12; i++) {
        polygons[i * 4] = (uint8_t)dt_range(0, 2);
        polygons[i * 4 + 1] = (uint8_t)dt_range(0, 5);
        polygons[i * 4 + 2] = (uint8_t)dt_range(0, 5);
        polygons[i * 4 + 3] = (uint8_t)dt_range(0, 5);
    }
    s->boundingBox.sup.x = dt_float_edgy(-5.0f, 5.0f);
    s->boundingBox.sup.y = dt_float_edgy(-5.0f, 5.0f);
    s->boundingBox.sup.z = dt_float_edgy(-5.0f, 5.0f);
    s->boundingBox.inf.x = dt_float_edgy(-5.0f, 5.0f);
    s->boundingBox.inf.y = dt_float_edgy(-5.0f, 5.0f);
    s->boundingBox.inf.z = dt_float_edgy(-5.0f, 5.0f);
    s->colSectorRoot = dt_u32() % 2 ? NULL : clFullTree();
    s->mesh = NULL;
    return s;
}

static RpSector *clTree(int depth)
{
    if (depth == 0) {
        return (RpSector *)clSector();
    }
    RpPlaneSector *n = dt_alloc(sizeof *n + 8);
    n->type = dt_range(0, 2);
    n->value = dt_float(-5.0f, 5.0f);
    n->leftValue = dt_float(-5.0f, 5.0f);
    n->rightValue = dt_float(-5.0f, 5.0f);
    n->leftSubTree = clTree(dt_u32() % 2 ? depth - 1 : 0);
    n->rightSubTree = clTree(dt_u32() % 2 ? depth - 1 : 0);
    return (RpSector *)n;
}

static RpWorld *clWorld(int depth)
{
    RpWorld *w = dt_alloc(sizeof *w + 8);
    dt_fill(w, sizeof *w);
    w->object.type = 7;
    w->object.flags = (uint8_t)(dt_u32() & 0x1e);
    w->worldOrigin.x = dt_float(-9.0f, 9.0f);
    w->worldOrigin.y = dt_float(-9.0f, 9.0f);
    w->worldOrigin.z = dt_float(-9.0f, 9.0f);
    w->surfaceProps.ambient = dt_float_edgy(0.0f, 2.0f);
    w->surfaceProps.specular = dt_float_edgy(0.0f, 2.0f);
    w->surfaceProps.diffuse = dt_float_edgy(0.0f, 2.0f);
    clMaterialList(&w->matList, 5);
    w->rootSector = clTree(depth);
    return w;
}

static void clWorldOpen(void)
{
    rpWorldModuleCount = dt_u32() % 8 == 0 ? 0 : 1;
}

/* ---- list and field accessors ----------------------------------------------------------------------- */

static void setup_world_register(DtCtx *c)
{
    clEnv(c);
    DT_ARG_PTR(c, 0, dt_alloc(0x80));
    DT_ARG_INT(c, 1, dt_u32());
    gClAllocFailAt = dt_u32() % 5 == 0 ? 1 : 0;
    rwtEnd();
}

static void setup_world_unregister(DtCtx *c)
{
    clEnv(c);
    RwLLLink *head = (RwLLLink *)((uint8_t *)RwEngineInstance + kClOffWorld + 8);
    RpWorld *target = dt_alloc(0x80);
    for (int i = dt_range(0, 3); i > 0; i--) {
        RpWorldListEntry *r = dt_alloc(sizeof *r + 8);
        r->world = dt_u32() % 3 == 0 ? target : dt_alloc(0x80);
        clInsertLink(head, &r->link);
    }
    DT_ARG_PTR(c, 0, dt_u32() % 4 == 0 ? dt_alloc(0x80) : target);
    rwtEnd();
}

static void setup_world(DtCtx *c)
{
    clEnv(c);
    clWorldOpen();
    DT_ARG_PTR(c, 0, dt_u32() % 8 == 0 ? NULL : clWorld(0));
    rwtEnd();
}

static void setup_world_surface(DtCtx *c)
{
    clEnv(c);
    clWorldOpen();
    RwSurfaceProperties *o = dt_alloc(sizeof *o);
    o->ambient = dt_float_edgy(-9.0f, 9.0f);
    o->specular = dt_float_edgy(-9.0f, 9.0f);
    o->diffuse = dt_float_edgy(-9.0f, 9.0f);
    DT_ARG_PTR(c, 0, dt_u32() % 8 == 0 ? NULL : clWorld(0));
    DT_ARG_PTR(c, 1, dt_u32() % 8 == 0 ? NULL : o);
    rwtEnd();
}

static void setup_world_value(DtCtx *c)
{
    clEnv(c);
    clWorldOpen();
    DT_ARG_PTR(c, 0, dt_u32() % 8 == 0 ? NULL : clWorld(0));
    DT_ARG_INT(c, 1, dt_u32() % 8 == 0 ? 0 : dt_u32());
    rwtEnd();
}

static void setup_calc_bbox(DtCtx *c)
{
    clEnv(c);
    RpWorld *w = clWorld(dt_range(0, 3));
    DT_ARG_PTR(c, 0, w);
    DT_ARG_PTR(c, 1, dt_alloc(sizeof(RwBBox)));
    rwtEnd();
}

static void setup_tree_size(DtCtx *c)
{
    clEnv(c);
    uint8_t *t = dt_alloc(0x180);
    dt_fill(t, 0x180);
    /* a random shape: internal nodes (bit 7) with a decreasing chance per level, so every walk ends */
    for (int i = 1; i < 128; i++) {
        t[i * 3] = (uint8_t)(t[i * 3] & 0x7f);
        if (i < 64 && dt_u32() % (1u + (uint32_t)(i / 8)) == 0) {
            t[i * 3] |= 0x80;
        }
    }
    DT_ARG_PTR(c, 0, dt_u32() % 8 == 0 ? NULL : t);
    rwtEnd();
}

static void setup_generate_meshes(DtCtx *c)
{
    clEnv(c);
    clWorldOpen();
    gClAllocFailAt = dt_u32() % 8 == 0 ? dt_range(1, 3) : 0;
    gClMallocFailAt = dt_u32() % 8 == 0 ? dt_range(1, 3) : 0;
    RpWorld *w = clWorld(dt_range(0, 2));
    w->object.flags = (uint8_t)(w->object.flags & ~1);
    DT_ARG_PTR(c, 0, dt_u32() % 12 == 0 ? NULL : w);
    rwtEnd();
}

/* ---- writing a world -------------------------------------------------------------------------------- */

static void setup_world_write(DtCtx *c)
{
    clEnv(c);
    clRegistryStubs();
    clWriteStubs();
    rpWorldModuleCount = 1; /* a closed module makes the original dereference NULL */
    dt_stub(0x004e8d60, NULL, (void *)stRegistrySize2);
    dt_stub(0x004bb760, NULL, (void *)stTreeSizeFull);
    dt_stub(0x004c5160, NULL, (void *)stMatListWrite);
    dt_stub(0x004c50e0, NULL, (void *)stMatListSize);
    DT_ARG_PTR(c, 0, clWorld(dt_range(0, 2)));
    DT_ARG_PTR(c, 1, dt_alloc(8));
    rwtEnd();
}

/* ---- reading a world -------------------------------------------------------------------------------- */

static int32_t BMX_CDECL stMatListSize(const RpMaterialList *list)
{
    rwtLog(0x4c50e0);
    rwtLogPtr(list);
    return list->numMaterials * 8;
}

static void *BMX_CDECL stMatListWrite(const RpMaterialList *list, void *stream)
{
    rwtLog(0x4c5160);
    rwtLogPtr(list);
    rwtLogPtr(stream);
    return clFails(kClStubOther) ? NULL : (void *)list;
}

static RwTexture *BMX_CDECL stGetTexture(RpMaterial *m)
{
    rwtLog(0x4b93d0);
    rwtLogPtr(m);
    return m != NULL && clFails(kClStubOther) ? NULL : (RwTexture *)m;
}

static void *BMX_CDECL stSetTexture(RpMaterial *m, RwTexture *t)
{
    rwtLog(0x4b9380);
    rwtLogPtr(m);
    rwtLogPtr(t);
    return m;
}

static int BMX_CDECL stWorldDestroy(RpWorld *w)
{
    rwtLog(0x4ba9c0);
    rwtLogPtr(w);
    return 1;
}

static RpMaterialList *BMX_CDECL stWorldMatListRead(void *stream, RpMaterialList *list)
{
    rwtLog(0x4c5330);
    rwtLogPtr(stream);
    rwtLogPtr(list);
    if (clFails(kClStubMesh)) {
        return NULL;
    }
    list->materials = gClMats8;
    list->numMaterials = 8;
    list->space = 8;
    return list;
}

/* Writes a well-formed tree into the stream script from dword gStreamDword on: planes (6 dwords: type, value,
 * left is a sector, right is a sector, left value, right value) and sectors (11 dwords, small vertex and
 * polygon counts, no collision tree, then the per-vertex arrays the world format (dword 12) asks for), at
 * most 8 planes so the block (30 planes, 30 sectors) holds it. */
static int gStreamDword, gTreePlanes;

static void clPut(uint32_t value)
{
    const int at = (gStreamDword++ * 4) % kClStreamBytes;
    for (int i = 0; i < 4; i++) {
        gClStream[at + i] = (uint8_t)(value >> (i * 8));
    }
}

static void clPutFloat(float value)
{
    union {
        float f;
        uint32_t u;
    } bits = {value};
    clPut(bits.u);
}

static void clPutSector(void)
{
    const uint32_t numPolygons = (uint32_t)dt_range(0, 2);
    const uint32_t numVertices = (uint32_t)dt_range(0, 2);
    clPut((uint32_t)dt_range(0, 1));
    clPut(numPolygons);
    clPut(numVertices);
    for (int i = 0; i < 6; i++) {
        clPutFloat(dt_float_edgy(-50.0f, 50.0f));
    }
    clPut(0); /* no collision tree */
    clPut(dt_u32());
    const uint32_t format = gClStream[12 * 4];
    const uint32_t perVertex = 3 + ((format & 0x10) != 0) + ((format & 8) != 0) + ((format & 4) != 0) * 2;
    for (uint32_t i = 0; i < numVertices * perVertex; i++) { /* vertices, normals, prelight, uvs */
        clPut(dt_u32());
    }
    for (uint32_t i = 0; i < numPolygons; i++) { /* material 0..3, vertex indices 0 */
        clPut(dt_u32() % 4);
    }
}

static void clPutPlane(int depth)
{
    gTreePlanes++;
    const int leftPlane = depth > 0 && gTreePlanes < 8 && dt_u32() % 3 != 0;
    const int rightPlane = depth > 0 && gTreePlanes < 8 && dt_u32() % 3 != 0;
    clPut((uint32_t)dt_range(0, 2) * 4);
    clPutFloat(dt_float_edgy(-50.0f, 50.0f));
    clPut(leftPlane ? 0 : (uint32_t)dt_range(1, 3));
    clPut(rightPlane ? 0 : 1);
    clPutFloat(dt_float_edgy(-50.0f, 50.0f));
    clPutFloat(dt_float_edgy(-50.0f, 50.0f));
    if (leftPlane) {
        clPutPlane(depth - 1);
    } else {
        clPutSector();
    }
    if (rightPlane) {
        clPutPlane(depth - 1);
    } else {
        clPutSector();
    }
}

/* nested: plane nodes may have plane children (the stream decides), as in real levels */
static void worldReadSetup(DtCtx *c, int nested)
{
    clEnv(c);
    clRegistryStubs();
    clStreamScript();

    for (int i = 0; i < 8; i++) {
        gClFailAt[i] = dt_u32() % 6 == 0 ? dt_range(1, 3) : 0;
        gClFailCalls[i] = 0;
    }
    for (int i = 0; i < 1024; i += 4) {
        gClStream[i] = (uint8_t)(dt_u32() % 4);
        gClStream[i + 1] = gClStream[i + 2] = gClStream[i + 3] = 0;
    }
    gClStream[0] = (uint8_t)(dt_u32() % 2);         /* the root is a sector */
    gClStream[12 * 4] = (uint8_t)(dt_u32() & 0x1e); /* format flags (no strips) */
    if (nested) {
        gClStream[0] = 0;
        gClStream[12 * 4 + 3] =
            (uint8_t)(dt_u32() % 2 ? 0x40 : 0); /* plane nodes keep their own left/right values */
        gStreamDword = 13;
        gTreePlanes = 0;
        clPutPlane(3);
        gClStreamFailAt = 0; /* a failed read does not advance the script, and the prelight read ignores it */
    } else {
        gClStream[15 * 4] = 1; /* a plane root with sector children */
        gClStream[16 * 4] = 1;
    }
    gClStream[7 * 4] = 60; /* triangles, vertices, planes, sectors, collision bytes: */
    gClStream[8 * 4] = 60; /* generous, so the reader stays inside its block */
    gClStream[9 * 4] = 30;
    gClStream[10 * 4] = 30;
    gClStream[11 * 4] = 0x80;
    for (int i = 0; i < 16; i++) {
        gClLengths[i] = i == 0 ? 0x34 : 0x18;
    }
    for (int i = 0; i < 8; i++) {
        gTreeNumbers[i] = dt_range(0, 0x180);
    }
    gTreeCalls = 0;
    for (int i = 0; i < 8; i++) {
        gClMats8[i] = clMaterial();
    }
    gClAllocFailAt = dt_u32() % 10 == 0 ? 1 : 0;
    gClMallocFailAt = dt_u32() % 8 == 0 ? dt_range(1, 3) : 0;
    dt_stub(0x004bb760, NULL, (void *)stTreeSizeScripted);
    dt_stub(0x004c5330, NULL, (void *)stWorldMatListRead);
    dt_stub(0x004b93d0, NULL, (void *)stGetTexture);
    dt_stub(0x004b9380, NULL, (void *)stSetTexture);
    dt_stub(0x004ba9c0, NULL, (void *)stWorldDestroy);
    rpWorldModuleCount = dt_u32() % 8 == 0 ? 0 : 1;
    DT_ARG_PTR(c, 0, dt_alloc(8));
    rwtEnd();
}

static void setup_world_read(DtCtx *c)
{
    worldReadSetup(c, 0);
}

static void setup_world_read_nested(DtCtx *c)
{
    worldReadSetup(c, 1);
}

DT_CASES(rwworldstream_cases) = {
    {"rpWorldRegister", 0x004b9ed0, rpWorldRegister, 2, DT_RET_VOID, setup_world_register, 60, 0},
    {"rpWorldUnregister", 0x004b9f60, rpWorldUnregister, 1, DT_RET_VOID, setup_world_unregister, 100, 0},
    {"RpWorldSetSurfaceProperties", 0x004b9c30, RpWorldSetSurfaceProperties, 2, DT_RET_INT,
     setup_world_surface, 150, 0},
    {"RpWorldGetSurfaceProperties", 0x004b9cc0, RpWorldGetSurfaceProperties, 1, DT_RET_INT, setup_world, 60,
     0},
    {"RpWorldSetKind", 0x004ba270, RpWorldSetKind, 2, DT_RET_INT, setup_world_value, 80, 0},
    {"RpWorldSetRenderCallBack", 0x004baf20, RpWorldSetRenderCallBack, 2, DT_RET_INT, setup_world_value, 100,
     0},
    {"RpWorldSetFlags", 0x004bbc40, RpWorldSetFlags, 2, DT_RET_INT, setup_world_value, 80, 0},
    {"RpWorldGetFlags", 0x004bbcc0, RpWorldGetFlags, 1, DT_RET_INT8, setup_world, 60, 0},
    {"rwWorldCalcBBox", 0x004b9b90, rwWorldCalcBBox, 2, DT_RET_INT, setup_calc_bbox, 200, 0},
    {"rpCollisionTreeSize", 0x004bb760, rpCollisionTreeSize, 1, DT_RET_INT, setup_tree_size, 300, 0},
    {"_rpWorldGenerateSectorMeshes", 0x004ba5a0, _rpWorldGenerateSectorMeshes, 1, DT_RET_INT,
     setup_generate_meshes, 300, 0},
    {"RpWorldStreamWrite", 0x004c3ae0, RpWorldStreamWrite, 2, DT_RET_INT, setup_world_write, 500, 0},
    {"RpWorldStreamRead", 0x004c3e60, RpWorldStreamRead, 1, DT_RET_INT, setup_world_read, 1500, 0},
    {"RpWorldStreamRead_nested", 0x004c3e60, RpWorldStreamRead, 1, DT_RET_INT, setup_world_read_nested, 1500,
     0},
};
