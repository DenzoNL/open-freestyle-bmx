/* Collision queries (C:\zaxis\Bmx\src\com\zge\NgIF\NgIFzcollide.c). */
#include "bmx/com/zge/ngif/ngifzcollide.h"

#include "bmx/com/zge/ngif/ngifzbsp.h"
#include "compat/crt.h"
#include "zge/zdebug.h"
#include "zge/zgemods/zobjcamera.h"
#include "zge/zmem.h"
#include "zge/zstring.h"

static const char kCollideFile[] = "C:\\zaxis\\Bmx\\src\\com\\zge\\NgIF\\NgIFzcollide.c";

/* What every hook stub prints: "<file> <line> <name>". */
static void trace(int line, const char *what)
{
    gZdebugPrint("%s %d %s\n", kCollideFile, line, what);
}

/* Material word of terrain probe hits (0x5f8518, the dword after it is cleared too). */
typedef struct ZcollideMaterial {
    uint32_t flags;
    uint32_t extra;
} ZcollideMaterial;
#define gZcollideProbeMaterial ORIG_VAR(0x005f8518, ZcollideMaterial, gZcollideProbeMaterial)

/*
 * Drops a vertical line (500 units up to 500 down) through p onto the terrain. On a hit, fills the
 * one-point face record `out` and returns the height the renderer reported; on a miss returns 10000.
 */
BMX_ADDR(0x0046a160)
double zcollideProbeHeight(const ZVec3 *p, ZcollideProbe *out)
{
    float top[4] = {p->x, 500.0f, p->z, 0.0f};
    float bottom[4] = {p->x, -500.0f, p->z, 0.0f};
    float hit[0x16];

    if (zrwcolLineTest(bottom, top, 3, 0, hit) == 0) {
        return 10000.0f;
    }
    out->kind = 2;
    out->points = &out->point;
    out->pad04 = 0;
    out->point.x = hit[0];
    out->point.y = hit[1];
    out->point.z = hit[2];
    out->pad3c = 0;
    out->count = 1;
    out->byte55 = 0;
    out->point2.x = hit[0];
    out->point2.y = hit[1];
    out->byte54 = 0;
    out->word56 = 0;
    out->point2.z = hit[2];
    out->pad4c = 0;
    out->pad50 = 0;
    out->points2 = &out->point2;
    out->pad14 = 0;
    gZcollideProbeMaterial.flags &= 0x3fe00000;
    gZcollideProbeMaterial.extra = 0;
    out->word20 = 0;
    out->pad24 = 0;
    out->pad28 = 0;
    out->pad2c = 0;
    out->material = &gZcollideProbeMaterial.flags;
    out->height = hit[0x14];
    return hit[0x14];
}

/* Allocates and lists the collision edges of a prop: 4 for shape kind 2, 1 for kinds 3 and 4. */
BMX_ADDR(0x0046a3f0)
int zcollideBuildPropEdges(PhysPropObject *object, PhysEdge **out)
{
    int count;
    int edges;

    switch (*zbspGetShapeKind(object->index)) {
    case 1:
        zdebugFail("Not coded", kCollideFile, 0x4da, 0);
        edges = 0;
        break;
    case 2:
        edges = 4;
        break;
    case 3:
    case 4:
        edges = 1;
        break;
    default:
        edges = 0;
    }
    object->edges =
        zmemFill(zmemAlloc((uint32_t)edges * sizeof(PhysEdge)), 0, (uint32_t)edges * sizeof(PhysEdge));
    count = zcollideFillPropEdges(object, object->edges);
    for (int i = 0; i < count; i++) {
        out[i] = &object->edges[i];
    }
    return count;
}

/*
 * Initialises an edge of a prop's collision box: int16 end points, rounded length, unit direction and the
 * normal (direction x up). `side` > 0 marks the solid side with flag 1, < 0 with flag 2.
 */
BMX_ADDR(0x0046aa50)
void zcollideInitEdge(PhysEdge *edge, uint32_t kind, const int16_t *a, const int16_t *b, int side)
{
    const ZVec3 down = {0.0f, -1.0f, 0.0f};
    ZVec3 n;
    uint32_t sum;
    int32_t dx, dy, dz;
    int16_t length;

    edge->side = 0;
    edge->tag = 0;
    edge->kind = (int8_t)kind;
    edge->a[0] = a[0];
    edge->a[1] = a[1];
    edge->a[2] = a[2];
    edge->b[0] = b[0];
    edge->b[1] = b[1];
    edge->b[2] = b[2];
    dx = b[0] - edge->a[0];
    dy = b[1] - edge->a[1];
    dz = b[2] - edge->a[2];
    /* The squared length is summed in 32 bits (wrapping) before the square root. */
    sum = (uint32_t)dz * (uint32_t)dz + (uint32_t)dy * (uint32_t)dy + (uint32_t)dx * (uint32_t)dx;
    length = (int16_t)bmxFtol(__builtin_sqrt((double)(int32_t)sum));
    edge->length = length;
    edge->dir.x = (float)((double)(int16_t)dx / (double)length);
    edge->dir.y = (float)((double)(int16_t)dy / (double)length);
    edge->dir.z = (float)((double)(int16_t)dz / (double)length);
    zvec3Cross3(&n, &edge->dir, &gZmathOrigin, &down);
    edge->normal = n;
    edge->d = 0.0f;
    if (side > 0) {
        edge->side |= 1;
    } else if (side < 0) {
        edge->side |= 2;
    }
}

/* Refreshes the collision edges of a prop after it turned. */
BMX_ADDR(0x0046ac40)
void zcollidePropRotated(PhysPropObject *object, const void *unused)
{
    if (object->edges != NULL) {
        zcollideFillPropEdges(object, object->edges);
    }
}

/* Refreshes the collision edges of a prop after it moved. */
BMX_ADDR(0x0046ac70)
void zcollidePropMoved(PhysPropObject *object, const void *unused)
{
    if (object->edges != NULL) {
        zcollideFillPropEdges(object, object->edges);
    }
}

/* ---- debug-print stubs ----------------------------------------------------------------------------- */

BMX_ADDR(0x0046a120)
int zcolBboxCollision(void)
{
    trace(900, "zcolbboxcollision");
    return 1;
}

BMX_ADDR(0x0046a2c0)
int zcollideGetTerrainFace(void)
{
    trace(995, "Get terrain face");
    return 0;
}

BMX_ADDR(0x0046a2f0)
int zcollidePropHitBox(void)
{
    trace(1056, "prop hit box");
    return 0;
}

BMX_ADDR(0x0046a320)
double zcollideGetPlaneHeight(void)
{
    trace(1074, "GetPlaneHeight");
    return 0.0;
}

BMX_ADDR(0x0046a360)
void zcollideGenerateBoundBox(void)
{
    trace(1167, "Generate bound box");
}

BMX_ADDR(0x0046a390)
void zcollideInitPropFacesList(void)
{
    trace(1183, "Init Prop faces list");
}

BMX_ADDR(0x0046a3c0)
void zcollideGeneratePropBoundingVolume(void)
{
    trace(1199, "Generate prop bounding volume");
}

BMX_ADDR(0x0046ac10)
void zcollidePropMove(void)
{
    trace(1592, "prop move");
}
