/*
 * Tests for the mesh stream plugin (rw/world.c): _rpMeshRead/_rpMeshWrite/_rpMeshSize, the material list
 * lookup, and the stream callbacks of geometries and world sectors that call them. The stream functions
 * (RwStreamReadInt32 / RwStreamWriteInt32) are scripted stubs that log what the code reads or writes; the
 * sector callbacks find their world through the engine's list of worlds (a world that is one block of
 * memory, so the sector is found by address).
 */
#include "dt_rw.h"
#include "rw/rwclump.h"
#include "rw/rwworld.h"

enum {
    kEngineBytes = 0x400,
    kMeshOffset = 0x200,
    kWorldGlobalsOffset = 0x210,
    kWorldBlock = 0x200,
    kSectorAt = 0x100,
};

static uint32_t gScript[96];
static int gScriptN, gScriptAt;
static int gStreamCalls, gStreamFailAt; /* the n-th stream call fails (1-based); 0 never */

static void *BMX_CDECL stReadInt32(RwStream *stream, void *buffer, uint32_t size)
{
    rwtLog(0x4e3270);
    rwtLogPtr(stream);
    rwtLog(size);
    gStreamCalls++;
    if (gStreamFailAt != 0 && gStreamCalls == gStreamFailAt) {
        return NULL;
    }
    uint32_t *out = buffer;
    for (uint32_t i = 0; i < size / 4; i++) {
        out[i] = gScriptAt < gScriptN ? gScript[gScriptAt++] : 0;
    }
    return stream;
}

static void *BMX_CDECL stWriteInt32(RwStream *stream, const void *buffer, uint32_t size)
{
    rwtLog(0x4e3130);
    rwtLogPtr(stream);
    rwtLog(size);
    rwtLogWords(buffer, (int)(size / 4));
    gStreamCalls++;
    if (gStreamFailAt != 0 && gStreamCalls == gStreamFailAt) {
        return NULL;
    }
    return stream;
}

static void streamStubs(void)
{
    dt_stub(0x004e3270, NULL, (void *)stReadInt32);
    dt_stub(0x004e3130, NULL, (void *)stWriteInt32);
    gStreamCalls = 0;
    gStreamFailAt = dt_u32() % 3 == 0 ? dt_range(1, 12) : 0;
}

static RwGlobals *meshEnv(const DtCtx *c)
{
    dt_seed(c);
    RwGlobals *small = rwtEnv();
    RwGlobals *g = dt_alloc(kEngineBytes);
    __builtin_memcpy(g, small, kRwtEngineBytes);
    RwEngineInstance = g;
    rpMeshOffset = kMeshOffset;
    *(uint16_t *)((uint8_t *)g + kMeshOffset) = (uint16_t)(dt_u32() % 4 == 0 ? 0xffff : dt_u32());
    rpWorldGlobalsOffset = kWorldGlobalsOffset;
    rpWorldModuleCount = 1;
    RpWorldGlobals *worlds = (RpWorldGlobals *)((uint8_t *)g + kWorldGlobalsOffset);
    worlds->worlds.next = worlds->worlds.prev = &worlds->worlds;
    return g;
}

/* A material list of n pointers (index 0..n-1 valid) in a block of eight. */
static RpMaterialList *mkMaterials(int n)
{
    RpMaterialList *list = dt_alloc(sizeof(RpMaterialList));
    list->materials = n == 0 && dt_u32() % 2 ? NULL : (RpMaterial **)dt_alloc(8 * sizeof(void *));
    for (int i = 0; i < 8 && list->materials != NULL; i++) {
        list->materials[i] = dt_alloc(0x18);
    }
    if (list->materials != NULL && n > 1 && dt_u32() % 3 == 0) {
        list->materials[n - 1] = list->materials[0]; /* a repeated material: the first index wins */
    }
    list->numMaterials = n;
    list->space = 8;
    return list;
}

static void setup_find_material(DtCtx *c)
{
    meshEnv(c);
    const int n = dt_range(0, 6);
    RpMaterialList *list = mkMaterials(n);
    RpMaterial *material = dt_alloc(0x18);
    if (n > 0 && dt_u32() % 4 != 0) {
        material = list->materials[dt_u32() % (uint32_t)n];
    } else if (dt_u32() % 5 == 0) {
        material = NULL;
    }
    DT_ARG_PTR(c, 0, list);
    DT_ARG_PTR(c, 1, material);
}

static void setup_mesh_size(DtCtx *c)
{
    meshEnv(c);
    RpMeshHeader *mesh = dt_alloc(sizeof(RpMeshHeader));
    mesh->numMeshes = (uint16_t)(dt_u32() % 5 == 0 ? dt_u32() : (uint32_t)dt_range(0, 9));
    mesh->totalIndices = dt_u32() % 5 == 0 ? dt_u32() : (uint32_t)dt_range(0, 300);
    DT_ARG_PTR(c, 0, mesh);
}

/* A mesh header with n meshes of 0..4 indices; materials from the list, now and then a stranger. */
static RpMeshHeader *mkMesh(const RpMaterialList *materials, int n)
{
    RpMeshHeader *mesh = dt_alloc(sizeof(RpMeshHeader) + sizeof(RpMesh) * (uint32_t)n);
    mesh->flags = dt_u32() % 2;
    mesh->numMeshes = (uint16_t)n;
    mesh->serialNum = (uint16_t)dt_u32();
    RpMesh *entries = (RpMesh *)(mesh + 1);
    uint32_t total = 0;
    for (int i = 0; i < n; i++) {
        entries[i].numIndices = (uint32_t)dt_range(0, 4);
        total += entries[i].numIndices;
        entries[i].indices = dt_alloc(2 * 4 + 2);
        for (int k = 0; k < 4; k++) {
            entries[i].indices[k] = (uint16_t)dt_u32();
        }
        entries[i].material = materials->numMaterials > 0 && dt_u32() % 5 != 0
                                  ? materials->materials[dt_u32() % (uint32_t)materials->numMaterials]
                                  : dt_alloc(0x18);
    }
    mesh->totalIndices = total;
    return mesh;
}

static void setup_mesh_write(DtCtx *c)
{
    meshEnv(c);
    streamStubs();
    RpMaterialList *materials = mkMaterials(dt_range(0, 5));
    DT_ARG_PTR(c, 0, mkMesh(materials, dt_range(0, 3)));
    DT_ARG_PTR(c, 1, dt_alloc(8));
    DT_ARG_PTR(c, 2, dt_alloc(0x20));
    DT_ARG_PTR(c, 3, materials);
    rwtEnd();
}

/* The words the stream would deliver for a mesh of n meshes (consistent counts); the header words first. */
static void scriptMesh(void)
{
    const int n = dt_range(0, 3);
    gScriptN = gScriptAt = 0;
    uint32_t counts[3];
    uint32_t total = 0;
    for (int i = 0; i < n; i++) {
        counts[i] = (uint32_t)dt_range(0, 5);
        total += counts[i];
    }
    gScript[gScriptN++] = dt_u32() % 2;
    gScript[gScriptN++] = (uint32_t)n;
    gScript[gScriptN++] = total;
    for (int i = 0; i < n; i++) {
        gScript[gScriptN++] = counts[i];
        gScript[gScriptN++] = (uint32_t)dt_range(0, 7);
        for (uint32_t k = 0; k < counts[i]; k++) {
            gScript[gScriptN++] = dt_u32();
        }
    }
}

static void setup_mesh_read(DtCtx *c)
{
    meshEnv(c);
    streamStubs();
    scriptMesh();
    gRwtMallocFailAt = dt_u32() % 5 == 0 ? 1 : 0;
    DT_ARG_PTR(c, 0, dt_alloc(0x20));
    DT_ARG_PTR(c, 1, dt_alloc(8));
    DT_ARG_PTR(c, 2, mkMaterials(8));
    rwtEnd();
}

/* A geometry: the mesh at 0x40 and the material list at 0x10. */
static RpGeometry *mkGeometry(void)
{
    RpGeometry *g = dt_alloc(sizeof(RpGeometry));
    RpMaterialList *materials = mkMaterials(dt_range(0, 5));
    g->matList = *materials;
    g->mesh = mkMesh(materials, dt_range(0, 3));
    return g;
}

static void setup_geometry_write(DtCtx *c)
{
    meshEnv(c);
    streamStubs();
    DT_ARG_PTR(c, 0, dt_alloc(0x20));
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_PTR(c, 2, mkGeometry());
    DT_ARG_INT(c, 3, dt_u32());
    DT_ARG_INT(c, 4, dt_u32());
    rwtEnd();
}

static void setup_geometry_read(DtCtx *c)
{
    meshEnv(c);
    streamStubs();
    scriptMesh();
    gRwtMallocFailAt = dt_u32() % 5 == 0 ? 1 : 0;
    RpGeometry *g = dt_alloc(sizeof(RpGeometry));
    RpMaterialList *materials = mkMaterials(8);
    g->matList = *materials;
    g->mesh = dt_alloc(0x10);
    DT_ARG_PTR(c, 0, dt_alloc(0x20));
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_PTR(c, 2, g);
    DT_ARG_INT(c, 3, dt_u32());
    DT_ARG_INT(c, 4, dt_u32());
    rwtEnd();
}

static void setup_geometry_size(DtCtx *c)
{
    meshEnv(c);
    RpGeometry *g = mkGeometry();
    g->mesh->totalIndices = (uint32_t)dt_range(0, 300);
    DT_ARG_PTR(c, 0, g);
}

/* A world that is one block of memory in the engine's list, with the sector inside it. */
static RpWorldSector *mkSectorInWorld(RwGlobals *g, const RpMaterialList *materials)
{
    uint8_t *block = dt_alloc(kWorldBlock);
    RpWorld *world = (RpWorld *)block;
    world->object.privateFlags = (uint8_t)(1 | dt_u32());
    world->matList = *materials;
    RpWorldGlobals *globals = (RpWorldGlobals *)((uint8_t *)g + kWorldGlobalsOffset);
    RpWorldListEntry *entry = dt_alloc(sizeof(RpWorldListEntry));
    entry->world = world;
    entry->size = kWorldBlock;
    entry->link.next = &globals->worlds;
    entry->link.prev = globals->worlds.prev;
    globals->worlds.prev->next = &entry->link;
    globals->worlds.prev = &entry->link;
    return (RpWorldSector *)(block + kSectorAt);
}

static void setup_sector_write(DtCtx *c)
{
    RwGlobals *g = meshEnv(c);
    streamStubs();
    RpMaterialList *materials = mkMaterials(dt_range(0, 5));
    RpWorldSector *sector = mkSectorInWorld(g, materials);
    sector->mesh = mkMesh(materials, dt_range(0, 3));
    DT_ARG_PTR(c, 0, dt_alloc(0x20));
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_PTR(c, 2, sector);
    DT_ARG_INT(c, 3, dt_u32());
    DT_ARG_INT(c, 4, dt_u32());
    rwtEnd();
}

static void setup_sector_read(DtCtx *c)
{
    RwGlobals *g = meshEnv(c);
    streamStubs();
    scriptMesh();
    gRwtMallocFailAt = dt_u32() % 5 == 0 ? 1 : 0;
    RpMaterialList *materials = mkMaterials(8);
    RpWorldSector *sector = mkSectorInWorld(g, materials);
    sector->mesh = dt_alloc(0x10);
    DT_ARG_PTR(c, 0, dt_alloc(0x20));
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_PTR(c, 2, sector);
    DT_ARG_INT(c, 3, dt_u32());
    DT_ARG_INT(c, 4, dt_u32());
    rwtEnd();
}

static void setup_sector_size(DtCtx *c)
{
    RwGlobals *g = meshEnv(c);
    RpMaterialList *materials = mkMaterials(2);
    RpWorldSector *sector = mkSectorInWorld(g, materials);
    sector->mesh = dt_alloc(sizeof(RpMeshHeader));
    sector->mesh->numMeshes = (uint16_t)dt_range(0, 9);
    sector->mesh->totalIndices = (uint32_t)dt_range(0, 300);
    DT_ARG_PTR(c, 0, sector);
}

DT_CASES(rwmesh_cases) = {
    {"_rpMaterialListFindMaterialIndex", 0x004c50a0, _rpMaterialListFindMaterialIndex, 2, DT_RET_INT,
     setup_find_material, 200, 0},
    {"_rpMeshSize", 0x004c4ea0, _rpMeshSize, 1, DT_RET_INT, setup_mesh_size, 60, 0},
    {"_rpMeshWrite", 0x004c4bc0, _rpMeshWrite, 4, DT_RET_INT, setup_mesh_write, 300, 0},
    {"_rpMeshRead", 0x004c4d00, _rpMeshRead, 3, DT_RET_INT, setup_mesh_read, 300, 0},
    {"writeGeometryMesh", 0x004b8cc0, writeGeometryMesh, 5, DT_RET_INT, setup_geometry_write, 200, 0},
    {"readGeometryMesh", 0x004b8d00, readGeometryMesh, 5, DT_RET_INT, setup_geometry_read, 200, 0},
    {"sizeGeometryMesh", 0x004b8d40, sizeGeometryMesh, 3, DT_RET_INT, setup_geometry_size, 40, 0},
    {"writeSectorMesh", 0x004b8d60, writeSectorMesh, 5, DT_RET_INT, setup_sector_write, 200, 0},
    {"readSectorMesh", 0x004b8da0, readSectorMesh, 5, DT_RET_INT, setup_sector_read, 200, 0},
    {"sizeSectorMesh", 0x004b8de0, sizeSectorMesh, 3, DT_RET_INT, setup_sector_size, 40, 0},
};
