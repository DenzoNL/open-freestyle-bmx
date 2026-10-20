/*
 * Tests for NgIFzcollide.c: terrain probe, prop collision edges and the debug-print hooks.
 */
#include "bmx/com/zge/ngif/ngifzcollide.h"
#include "dt.h"
#include "dt_ngif.h"

/* ---- terrain probe ------------------------------------------------------------------------------- */

static int BMX_CDECL stubLineTest(const float *a, const float *b, int mode, int flags, float *out)
{
    const uint32_t hit = dt_u32() % 3 != 0;

    ngLog(0x494c70);
    for (int i = 0; i < 4; i++) {
        ngLogFloat(a[i]);
        ngLogFloat(b[i]);
    }
    ngLog((uint32_t)mode);
    ngLog((uint32_t)flags);
    if (hit) {
        for (int i = 0; i < 0x17; i++) {
            out[i] = ngRf();
        }
    }
    return (int)hit;
}

static void setup_probe(DtCtx *c)
{
    ZVec3 *p = dt_alloc(sizeof *p);
    void *out = dt_alloc(sizeof(ZcollideProbe));

    ngBegin(c, 0);
    ngRf3(p);
    dt_fill(out, sizeof(ZcollideProbe));
    dt_fill((void *)0x005f8518, 8);
    dt_stub(0x00494c70, NULL, (void *)stubLineTest);
    DT_ARG_PTR(c, 0, p);
    DT_ARG_PTR(c, 1, out);
}

/* ---- prop edges ---------------------------------------------------------------------------------- */

static int BMX_CDECL stubFillEdges(PhysPropObject *object, PhysEdge *edges)
{
    ngLog(0x46a4d0);
    ngLogPtr(object);
    ngLogPtr(edges);
    return (int)(dt_u32() % 7) - 2;
}

static PhysPropObject *newObject(int treeSize)
{
    PhysPropObject *o = dt_alloc(sizeof *o);

    dt_fill(o, sizeof *o);
    o->index = (uint16_t)dt_range(0, treeSize + 1);
    o->edges = (dt_u32() & 1) ? (PhysEdge *)(uintptr_t)(dt_u32() | 0x1000) : NULL;
    return o;
}

static void setup_build_edges(DtCtx *c)
{
    int n;
    PhysEdge **out = (PhysEdge **)dt_alloc(8 * sizeof *out);

    ngBegin(c, 1);
    n = dt_range(0, 6);
    ngMakeTree(n);
    dt_stub(0x0046a4d0, NULL, (void *)stubFillEdges);
    DT_ARG_PTR(c, 0, newObject(n));
    DT_ARG_PTR(c, 1, out);
}

static void setup_prop_moved(DtCtx *c)
{
    ngBegin(c, 0);
    dt_stub(0x0046a4d0, NULL, (void *)stubFillEdges);
    DT_ARG_PTR(c, 0, newObject(3));
    DT_ARG_INT(c, 1, dt_u32());
}

static int16_t edgeCoord(void)
{
    return dt_u32() % 5 == 0 ? (int16_t)(dt_u32() & 1 ? 32767 : -32768) : (int16_t)dt_range(-300, 300);
}

static void setup_init_edge(DtCtx *c)
{
    PhysEdge *e = dt_alloc(sizeof *e);
    int16_t *a = dt_alloc(8);
    int16_t *b = dt_alloc(8);

    ngBegin(c, 0);
    dt_fill(e, sizeof *e);
    for (int i = 0; i < 3; i++) {
        a[i] = edgeCoord();
        b[i] = (c->seed % 11 == 0) ? a[i] : edgeCoord();
    }
    DT_ARG_PTR(c, 0, e);
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_PTR(c, 2, a);
    DT_ARG_PTR(c, 3, b);
    DT_ARG_INT(c, 4, dt_u32() % 3 == 0 ? dt_u32() : (uint32_t)dt_range(-1, 1));
}

/* ---- cases ------------------------------------------------------------------------------------- */

DT_CASES(collide_cases) = {
    {"zcollideProbeHeight", 0x0046a160, zcollideProbeHeight, 2, DT_RET_DOUBLE, setup_probe, 200, 0},
    {"zcollideBuildPropEdges", 0x0046a3f0, zcollideBuildPropEdges, 2, DT_RET_INT, setup_build_edges, 300, 0},
    {"zcollideInitEdge", 0x0046aa50, zcollideInitEdge, 5, DT_RET_VOID, setup_init_edge, 500, 0},
    {"zcollidePropRotated", 0x0046ac40, zcollidePropRotated, 2, DT_RET_VOID, setup_prop_moved, 60, 0},
    {"zcollidePropMoved", 0x0046ac70, zcollidePropMoved, 2, DT_RET_VOID, setup_prop_moved, 60, 0},
};

/* ---- debug-print hooks ----------------------------------------------------------------------- */

DT_CASES(zcollide_trace_cases) = {
    {"zcolBboxCollision", 0x0046a120, zcolBboxCollision, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zcollideGetTerrainFace", 0x0046a2c0, zcollideGetTerrainFace, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zcollidePropHitBox", 0x0046a2f0, zcollidePropHitBox, 0, DT_RET_INT, ngSetupTrace, 4, 0},
    {"zcollideGetPlaneHeight", 0x0046a320, zcollideGetPlaneHeight, 0, DT_RET_DOUBLE, ngSetupTrace, 4, 0},
    {"zcollideGenerateBoundBox", 0x0046a360, zcollideGenerateBoundBox, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zcollideInitPropFacesList", 0x0046a390, zcollideInitPropFacesList, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
    {"zcollideGeneratePropBoundingVolume", 0x0046a3c0, zcollideGeneratePropBoundingVolume, 0, DT_RET_VOID,
     ngSetupTrace, 4, 0},
    {"zcollidePropMove", 0x0046ac10, zcollidePropMove, 0, DT_RET_VOID, ngSetupTrace, 4, 0},
};
