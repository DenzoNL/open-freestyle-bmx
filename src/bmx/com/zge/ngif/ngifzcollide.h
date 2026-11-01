/* Collision queries (C:\zaxis\Bmx\src\com\zge\NgIF\NgIFzcollide.c). */
#ifndef BMX_COM_ZGE_NGIF_NGIFZCOLLIDE_H
#define BMX_COM_ZGE_NGIF_NGIFZCOLLIDE_H

#include <stdint.h>

#include "bmx/com/action/physics_hit.h"
#include "compat/orig.h"
#include "zge/com/zglrw/zrwcol.h"
#include "zge/zmath.h"

/* Casts a ray from -> to and fills a 0xd8 byte hit record (PhysRayHit); returns 1 on a hit. */
int zcollideRay(const ZVec3 *from, const ZVec3 *to, void *out);
/*
 * Builds the collision edges of a prop object into `edges` (the box of a prop: four edges) and returns
 * how many it made.
 */
int zcollideFillPropEdges(PhysPropObject *object, PhysEdge *edges);
/*
 * NULL-terminated list (static, 32 slots) of pointers to edge pointers: the level edges of the renderer's
 * collision tree near p and the roller's prop edges within `radius`, each only when one end lies behind p
 * along dir.
 */
PhysEdge ***zcollideFindEdgesBehind(const ZVec3 *p, const ZVec3 *dir, float radius);

/* Line test result of the renderer (zrwcolLineTest): the hit triangle, then the hit point. */
typedef struct ZcollideLineHit {
    ZrwColHit hit;
    ZVec3 point; /* 0x4c */
    uint32_t pad58;
} ZcollideLineHit;

_Static_assert(sizeof(ZcollideLineHit) == 0x5c, "ZcollideLineHit size");

/* End points of the prop box edges zcollideFillPropEdges builds (indices into its 8 corners). */
typedef struct ZcollideBoxEdge {
    uint8_t a;
    uint8_t b;
    uint8_t side;
} ZcollideBoxEdge;
typedef ZcollideBoxEdge ZcollideBoxEdges[4];
#define gZcollideBoxEdges ORIG_VAR(0x0055d4f8, ZcollideBoxEdges, gZcollideBoxEdges) /* shape kind 2 */
#define gZcollideBarEdge  ORIG_VAR(0x0055d504, ZcollideBoxEdge, gZcollideBarEdge)   /* shape kind 4 */

/* Result of zcollideProbeHeight: a one point "face" list over the terrain (0x58 bytes). */
typedef struct ZcollideProbe {
    uint32_t kind; /* 2 */
    uint32_t pad04;
    ZVec3 *points; /* +0x30 */
    uint32_t count;
    ZVec3 *points2; /* +0x40 */
    uint32_t pad14;
    uint32_t *material; /* 0x5f8518 with its top bits cleared */
    float height;       /* height above the probe's start */
    uint16_t word20;
    uint8_t pad22[0x2];
    uint32_t pad24, pad28, pad2c;
    ZVec3 point; /* hit point */
    uint32_t pad3c;
    ZVec3 point2; /* same point again */
    uint32_t pad4c, pad50;
    uint8_t byte54, byte55;
    uint16_t word56;
} ZcollideProbe;

BMX_LAYOUT32_ASSERT(sizeof(ZcollideProbe) == 0x58, "ZcollideProbe size");
BMX_LAYOUT32_ASSERT(offsetof(ZcollideProbe, point) == 0x30, "ZcollideProbe.point");
BMX_LAYOUT32_ASSERT(offsetof(ZcollideProbe, point2) == 0x40, "ZcollideProbe.point2");

/* Casts a vertical line through p onto the terrain; returns the hit height or 10000 on a miss. */
double zcollideProbeHeight(const ZVec3 *p, ZcollideProbe *out);
/*
 * Allocates the collision edges of a prop object (by its shape kind), lets the engine fill them and lists
 * them in `out`. Returns the edge count.
 */
int zcollideBuildPropEdges(PhysPropObject *object, PhysEdge **out);
/* Initialises an edge record from int16 end points; side > 0 / < 0 marks the solid side. */
void zcollideInitEdge(PhysEdge *edge, uint32_t kind, const int16_t *a, const int16_t *b, int side);
/* Re-derives a prop's collision edges after its orientation / position changed. */
void zcollidePropRotated(PhysPropObject *object, const void *unused);
void zcollidePropMoved(PhysPropObject *object, const void *unused);

/* ---- debug-print stubs ---------------------------------------------------------------------------
 * Empty hooks of the original that only print their name.
 */
int zcolBboxCollision(void);
int zcollideGetTerrainFace(void);
int zcollidePropHitBox(void);
double zcollideGetPlaneHeight(void);
void zcollideGenerateBoundBox(void);
void zcollideInitPropFacesList(void);
void zcollideGeneratePropBoundingVolume(void);
void zcollidePropMove(void);

/* Record of world object `id` (a type byte first), or NULL. */
ORIG_FN(0x00469530, const uint8_t *, ngifzCollideGetObject, (uint32_t id));

#endif
