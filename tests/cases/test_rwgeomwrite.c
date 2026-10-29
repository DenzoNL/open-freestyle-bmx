/*
 * Test for the geometry chunk writer (rw/clump.c, 0x4b6350). The stream functions, the material list and the
 * plugin registry are logging stubs; any write can be made to fail so every early exit is compared.
 */
#include "dt_rw.h"
#include "rw/rwclump.h"
#include "rw/rwplugins.h"
#include "rw/rwstream.h"

static int gWriteCalls, gWriteFailAt;

static int BMX_CDECL stMaterialListSize(const RpMaterialList *list)
{
    rwtLog(0x4c50e0);
    rwtLogPtr(list);
    return dt_range(0, 900);
}

static int BMX_CDECL stRegistrySize(const RwPluginRegistry *registry, const void *object)
{
    rwtLog(0x4e8d60);
    rwtLogPtr(registry);
    rwtLogPtr(object);
    return dt_range(0, 300);
}

static void *BMX_CDECL stNative16(void *mem, uint32_t size)
{
    rwtLog(0x4e2de0);
    rwtLogPtr(mem);
    rwtLog(size);
    return mem;
}

static void *BMX_CDECL stLittleEndian32(void *mem, uint32_t size)
{
    rwtLog(0x4e2da0);
    rwtLogPtr(mem);
    rwtLog(size);
    rwtLogWords(mem, (int)(size / 4));
    return mem;
}

static RwStream *BMX_CDECL stWrite(RwStream *stream, const void *buffer, uint32_t length)
{
    rwtLog(0x4e29d0);
    rwtLogPtr(stream);
    rwtLogPtr(buffer);
    rwtLog(length);
    rwtLogWords(buffer, (int)(length / 4 > 8 ? 8 : length / 4));
    gWriteCalls++;
    return gWriteFailAt != 0 && gWriteCalls == gWriteFailAt ? NULL : stream;
}

static RwStream *BMX_CDECL stWriteReal(RwStream *stream, const void *reals, uint32_t size)
{
    rwtLog(0x4e3040);
    rwtLogPtr(stream);
    rwtLogPtr(reals);
    rwtLog(size);
    gWriteCalls++;
    return gWriteFailAt != 0 && gWriteCalls == gWriteFailAt ? NULL : stream;
}

static RpMaterialList *BMX_CDECL stListWrite(const RpMaterialList *list, RwStream *stream)
{
    rwtLog(0x4c5160);
    rwtLogPtr(list);
    rwtLogPtr(stream);
    gWriteCalls++;
    return gWriteFailAt != 0 && gWriteCalls == gWriteFailAt ? NULL : (RpMaterialList *)list;
}

static RwPluginRegistry *BMX_CDECL stRegistryWrite(RwPluginRegistry *registry, void *stream, void *object)
{
    rwtLog(0x4e8db0);
    rwtLogPtr(registry);
    rwtLogPtr(stream);
    rwtLogPtr(object);
    gWriteCalls++;
    return gWriteFailAt != 0 && gWriteCalls == gWriteFailAt ? NULL : registry;
}

static void setup_write(DtCtx *c)
{
    dt_seed(c);
    rwtEnv();
    rwtStreamStubs();
    dt_stub(0x004c50e0, NULL, (void *)stMaterialListSize);
    dt_stub(0x004e8d60, NULL, (void *)stRegistrySize);
    dt_stub(0x004e2de0, NULL, (void *)stNative16);
    dt_stub(0x004e2da0, NULL, (void *)stLittleEndian32);
    dt_stub(0x004e29d0, NULL, (void *)stWrite);
    dt_stub(0x004e3040, NULL, (void *)stWriteReal);
    dt_stub(0x004c5160, NULL, (void *)stListWrite);
    dt_stub(0x004e8db0, NULL, (void *)stRegistryWrite);
    gWriteCalls = 0;
    gWriteFailAt = dt_u32() % 3 == 0 ? dt_range(1, 40) : 0;
    gRwtHeaderFailAt = dt_u32() % 8 == 0 ? dt_range(1, 2) : 0;
    rpGeometryModuleCount = dt_u32() % 10 == 0 ? 0 : 1;
    RpGeometry *g = dt_alloc(sizeof(RpGeometry));
    g->flags = (uint8_t)dt_u32();
    g->numVertices = dt_u32() % 5 == 0 ? 0 : dt_range(0, 50);
    g->numTriangles = dt_u32() % 5 == 0 ? 0 : dt_range(1, 20);
    g->numMorphTargets = dt_range(0, 4);
    g->triangles = dt_alloc(8 * 32);
    for (int i = 0; i < 128; i++) {
        ((uint16_t *)g->triangles)[i] = (uint16_t)dt_u32();
    }
    g->preLitLum = dt_alloc(8);
    g->texCoords = dt_alloc(8);
    ((uint32_t *)&g->surfaceProps)[0] = dt_u32();
    ((uint32_t *)&g->surfaceProps)[1] = dt_u32();
    ((uint32_t *)&g->surfaceProps)[2] = dt_u32();
    g->morphTarget = dt_alloc(sizeof(RpMorphTarget) * 4);
    for (int i = 0; i < 4; i++) {
        uint32_t *sphere = (uint32_t *)&g->morphTarget[i].boundingSphere;
        for (int k = 0; k < 4; k++) {
            sphere[k] = dt_u32();
        }
        g->morphTarget[i].verts = dt_u32() % 2 ? NULL : dt_alloc(16);
        g->morphTarget[i].normals = dt_u32() % 2 ? NULL : dt_alloc(16);
    }
    DT_ARG_PTR(c, 0, dt_u32() % 12 == 0 ? NULL : g);
    DT_ARG_PTR(c, 1, dt_u32() % 12 == 0 ? NULL : dt_alloc(16));
    rwtEnd();
}

DT_CASES(rwgeomwrite_cases) = {
    {"RpGeometryStreamWrite", 0x004b6350, RpGeometryStreamWrite, 2, DT_RET_INT, setup_write, 600, 0},
};
