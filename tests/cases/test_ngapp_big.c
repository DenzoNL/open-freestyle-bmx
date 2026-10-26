/*
 * Tests for the two renderer callbacks of ngapp.c: the collision debug overlay and the loading screen.
 * The draw hooks and queries the code calls through the exe's function pointers are replaced in setup by
 * stubs that log what they get (vertex positions and colours; the vertex words the original leaves
 * uninitialised are not logged). Faces, roller edges and props are random fixtures around the camera so
 * that both sides of every distance test are taken; zvec3Length runs for real.
 */
#include <stddef.h>

#include "dt.h"
#include "dt_zmem.h"

#include "bmx/com/action/physics_hit.h"
#include "bmx/com/zge/ngif/ngapp.h"
#include "bmx/com/zge/ngif/ngifzbsp.h"
#include "compat/crt.h"
#include "zge/com/zglrw/zrwdbg.h"
#include "zge/com/zglrw/zrwprim.h"
#include "zge/zlist.h"

static uint32_t gLog[4096];
static uint32_t gLogN;

static void logU(uint32_t v)
{
    if (gLogN < sizeof gLog / sizeof gLog[0]) {
        gLog[gLogN] = v;
    }
    gLogN++;
}

static void logF(float f)
{
    uint32_t u;

    memcpy(&u, &f, 4);
    logU(u);
}

/* Position and colour of a debug / primitive vertex (both 0x1c bytes with the same layout). */
static void logVertex(const void *p)
{
    const ZrwDbgVertex *v = p;
    uint32_t color;

    logF(v->x);
    logF(v->y);
    logF(v->z);
    memcpy(&color, v->color, 4);
    logU(color);
}

/* Function pointers of the exe the code calls through. */
typedef void(BMX_CDECL *NgQueryFn)(int what, void *out, int arg);
typedef void(BMX_CDECL *NgFlagsFn)(void *model, int a, int b);
#define gTestQuery     ORIG_VAR(0x00561488, NgQueryFn, gTestQuery)
#define gTestDrawPolys ORIG_VAR(0x00561728, ZrwDrawFn, gTestDrawPolys)
#define gTestDrawTris  ORIG_VAR(0x0056172c, ZrwDrawFn, gTestDrawTris)
#define gTestDrawQuads ORIG_VAR(0x00561738, ZrwDrawFn, gTestDrawQuads)
#define gTestSetFlags  ORIG_VAR(0x00560ed8, NgFlagsFn, gTestSetFlags)
#define gTestFlags     ORIG_VAR(0x0068ffa0, NgFlags, gTestFlags)
#define gTestPlayer    ORIG_VAR(0x0056b615, uint8_t, gTestPlayer)
#define gTestLevel     ORIG_VAR(0x0056b614, uint8_t, gTestLevel)
#define gTestLoadCount ORIG_VAR(0x005ab480, uint16_t, gTestLoadCount)
#define gTestRoller    ORIG_VAR(0x00678494, ZList *, gTestRoller)
typedef uint8_t NgFlags[4];

/* ---- debug overlay ------------------------------------------------------------------------------- */

enum { kMaxFaces = 8, kMaxEdges = 6, kMaxProps = 4 };

static struct {
    ZbspLevelFace *faces;
    uint32_t faceCount;
    PhysEdge *edges;
    uint32_t edgeCount;
    uint32_t edgeAt;
    PhysPropNode *props;
} gFix;

static void BMX_CDECL s_drawTris(uint32_t flags, const void *prims, int count, const ZMatrix *matrix,
                                 int zero)
{
    const ZrwDbgPoly3 *p = prims;
    int k;

    logU(0x101);
    logU(flags);
    logU((uint32_t)count);
    logU((uint32_t)(uintptr_t)matrix);
    logU((uint32_t)zero);
    logU(p->head);
    for (k = 0; k < 3; k++) {
        logVertex(p->verts[k]);
    }
}

static void BMX_CDECL s_drawPolys(uint32_t flags, const void *prims, int count, const ZMatrix *matrix,
                                  int zero)
{
    const ZrwDbgPoly2 *p = prims;

    logU(0x102);
    logU(flags);
    logU((uint32_t)count);
    logU((uint32_t)(uintptr_t)matrix);
    logU((uint32_t)zero);
    logU(p->head);
    logVertex(p->verts[0]);
    logVertex(p->verts[1]);
}

static void BMX_CDECL s_setFlags(void *model, int a, int b)
{
    logU(0x103);
    logU((uint32_t)(uintptr_t)model);
    logU((uint32_t)a);
    logU((uint32_t)b);
}

static void *BMX_CDECL s_getFaces(void)
{
    logU(0x104);
    return gFix.faces;
}

static uint32_t BMX_CDECL s_getFaceCount(void)
{
    logU(0x105);
    return gFix.faceCount;
}

static void *BMX_CDECL s_listFirst(ZList *list)
{
    logU(0x106);
    logU((uint32_t)(uintptr_t)list);
    gFix.edgeAt = 0;
    return gFix.edgeCount != 0 ? &gFix.edges[0] : NULL;
}

static void *BMX_CDECL s_listNext(ZList *list)
{
    logU(0x107);
    logU((uint32_t)(uintptr_t)list);
    gFix.edgeAt++;
    return gFix.edgeAt < gFix.edgeCount ? &gFix.edges[gFix.edgeAt] : NULL;
}

static PhysPropNode **BMX_CDECL s_propList(void)
{
    logU(0x108);
    return &gFix.props;
}

static float near_coord(float centre, float range)
{
    return dt_u32() % 50 == 0 ? __builtin_nanf("") : centre + dt_float(-range, range);
}

static void setup_debug(DtCtx *c)
{
    static ZVec3 eyeObject[8]; /* an object whose position (at 0xc) is eyeObject[1] */
    const ZVec3 eye = {dt_float(-3000.0f, 3000.0f), dt_float(-500.0f, 500.0f), dt_float(-3000.0f, 3000.0f)};
    uint32_t i;
    int k;

    dt_seed(c);
    dt_zmem_env();
    gLogN = 0;
    for (k = 0; k < 4; k++) {
        gTestFlags[k] = dt_u32() % 4 == 0 ? (uint8_t)dt_u32() : (uint8_t)dt_range(0, 2);
    }
    if (c->seed % 5 == 0) {
        gTestFlags[3] = gTestFlags[2];
    }
    gTestPlayer = (uint8_t)dt_range(0, 3);
    memset(eyeObject, 0, sizeof eyeObject);
    eyeObject[1] = eye;
    *(void **)(uintptr_t)(0x00692ad4u + 0xf70u * gTestPlayer) = (uint8_t *)eyeObject;
    gFix.faceCount = (uint32_t)dt_range(0, kMaxFaces);
    gFix.faces = dt_alloc(sizeof(ZbspLevelFace) * kMaxFaces);
    for (i = 0; i < gFix.faceCount; i++) {
        ZbspLevelFace *f = &gFix.faces[i];
        const float range = dt_u32() % 2 ? 1200.0f : 3000.0f;
        ZVec3 *corner[3] = {&f->v0, &f->v1, &f->v2};

        for (k = 0; k < 3; k++) {
            corner[k]->x = near_coord(eye.x, range);
            corner[k]->y = near_coord(eye.y, range);
            corner[k]->z = near_coord(eye.z, range);
        }
        f->normal.x = dt_float(-1.0f, 1.0f);
        f->normal.y = dt_float(-1.0f, 1.0f);
        f->normal.z = dt_float(-1.0f, 1.0f);
        if (dt_u32() % 4 != 0) {
            uint16_t *tag = dt_alloc(4);

            *tag = dt_u32() % 2 ? 0 : (uint16_t)dt_u32();
            f->tag = tag;
        }
        for (k = 0; k < 3; k++) {
            if (dt_u32() % 3 != 0) {
                ZbspEdgeTag *t = dt_alloc(sizeof *t);

                t->id = (uint16_t)dt_u32();
                t->kind = dt_u32() % 2 ? 0 : (uint8_t)dt_u32();
                f->edgeTag[k] = t;
            }
        }
    }
    gFix.edgeCount = (uint32_t)dt_range(0, kMaxEdges);
    gFix.edges = dt_alloc(sizeof(PhysEdge) * kMaxEdges);
    for (i = 0; i < gFix.edgeCount; i++) {
        PhysEdge *e = &gFix.edges[i];
        uint16_t *prop = dt_alloc(4);

        for (k = 0; k < 3; k++) {
            const float centre = k == 0 ? eye.x : k == 1 ? eye.y : eye.z;

            e->a[k] = (int16_t)(int32_t)(centre + dt_float(-2500.0f, 2500.0f));
            e->b[k] = (int16_t)(int32_t)(centre + dt_float(-2500.0f, 2500.0f));
        }
        prop[1] = dt_u32() % 2 ? 0 : (uint16_t)(dt_u32() & 0x3c3);
        e->prop = prop;
    }
    gFix.props = NULL;
    for (k = dt_range(0, kMaxProps); k > 0; k--) {
        PhysPropNode *n = dt_alloc(sizeof *n);

        n->frame = dt_alloc(sizeof *n->frame);
        n->frame->model = (void *)(uintptr_t)(0x5000u + dt_u32() % 0x1000);
        n->next = gFix.props;
        gFix.props = n;
    }
    gTestDrawTris = s_drawTris;
    gTestDrawPolys = s_drawPolys;
    gTestSetFlags = s_setFlags;
    gTestRoller = (ZList *)(uintptr_t)0x7000;
    dt_stub(0x0048d980, NULL, (void *)s_getFaces);
    dt_stub(0x0048d990, NULL, (void *)s_getFaceCount);
    dt_stub(0x0048c330, NULL, (void *)s_listFirst);
    dt_stub(0x0048c360, NULL, (void *)s_listNext);
    dt_stub(0x00486ed0, NULL, (void *)s_propList);
    DT_ARG_INT(c, 0, dt_u32());
}

/* ---- loading screen ------------------------------------------------------------------------------ */

static float gScale[2];

static void BMX_CDECL s_query(int what, void *out, int arg)
{
    logU(0x201);
    logU((uint32_t)what);
    logU((uint32_t)arg);
    memcpy(out, &gScale[what == 11], 4);
}

static void BMX_CDECL s_drawQuads(uint32_t flags, const void *prims, int count, const ZMatrix *matrix,
                                  int zero)
{
    const ZrwPrimQuad *q = prims;
    int k;

    logU(0x202);
    logU(flags);
    logU((uint32_t)count);
    logU((uint32_t)(uintptr_t)matrix);
    logU((uint32_t)zero);
    logU(q->texture);
    for (k = 0; k < 4; k++) {
        logVertex(q->vertices[k]);
    }
}

static void *BMX_CDECL s_cameraFirst(void)
{
    logU(0x203);
    return (void *)(uintptr_t)0x6000;
}

static int BMX_CDECL s_cameraBegin(void *camera)
{
    logU(0x204);
    logU((uint32_t)(uintptr_t)camera);
    return 1;
}

static int BMX_CDECL s_cameraEnd(void)
{
    logU(0x205);
    return 1;
}

static void BMX_CDECL s_showRaster(int vsync)
{
    logU(0x206);
    logU((uint32_t)vsync);
}

static int BMX_CDECL s_drawSprite(void *sprite)
{
    logU(0x207);
    logU((uint32_t)(uintptr_t)sprite);
    return 1;
}

static void setup_loading(DtCtx *c)
{
    static const float kScales[] = {640.0f, 800.0f, 1024.0f, 1280.0f, 480.0f, 600.0f, 768.0f, 1024.0f};

    dt_seed(c);
    dt_zmem_env();
    gLogN = 0;
    gScale[0] = dt_u32() % 3 == 0 ? dt_float(1.0f, 4000.0f) : kScales[dt_range(0, 3)];
    gScale[1] = dt_u32() % 3 == 0 ? dt_float(1.0f, 4000.0f) : kScales[dt_range(4, 7)];
    gTestLevel = (uint8_t)dt_range(0, 11);
    gTestLoadCount = dt_u32() % 4 == 0 ? (uint16_t)dt_u32() : (uint16_t)dt_range(0, 0x200);
    gTestQuery = s_query;
    gTestDrawQuads = s_drawQuads;
    dt_stub(0x00492eb0, NULL, (void *)s_cameraFirst);
    dt_stub(0x00492f00, NULL, (void *)s_cameraBegin);
    dt_stub(0x00492fe0, NULL, (void *)s_cameraEnd);
    dt_stub(0x0048b950, NULL, (void *)s_showRaster);
    dt_stub(0x0045b6e0, NULL, (void *)s_drawSprite);
    DT_ARG_INT(c, 0, 0x8000u + dt_u32() % 0x1000);
}

DT_CASES(ngapp_big_cases) = {
    {"ngappDrawDebug", 0x0045a4d0, ngappDrawDebug, 1, DT_RET_INT, setup_debug, 1500, 0},
    {"ngappDrawLoading", 0x0045c150, ngappDrawLoading, 1, DT_RET_INT, setup_loading, 600, 0},
};
