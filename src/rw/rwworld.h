/*
 * RenderWare 3.1 world (rw/world.c): the BSP of plane sectors and the world sectors at its leaves, as far as
 * the collision code and the game read them.
 */
#ifndef RW_RWWORLD_H
#define RW_RWWORLD_H

#include "rw/rwmath.h"

/* A node of the world BSP: type < 0 is a world sector (leaf), otherwise a plane sector. */
typedef struct RpSector {
    int32_t type; /* 0x00 plane sectors: byte offset of the split axis in an RwV3d (0, 4, 8) */
} RpSector;

/*
 * Plane sector. The children overlap: everything below leftValue is in the left subtree, everything above
 * rightValue in the right one. The collision walks compare these values with coordinates as integers (the bit
 * patterns), which orders non-negative floats correctly.
 */
typedef struct RpPlaneSector {
    int32_t type;           /* 0x00 */
    float value;            /* 0x04 */
    RpSector *leftSubTree;  /* 0x08 */
    RpSector *rightSubTree; /* 0x0c */
    float leftValue;        /* 0x10 */
    float rightValue;       /* 0x14 */
} RpPlaneSector;

/* A polygon of a world sector: material and three vertex indices, one byte each. */
typedef struct RpPolygon {
    uint8_t matIndex;
    uint8_t vertIndex[3];
} RpPolygon;

/*
 * World sector (partial). colSectorRoot is the polygon split tree of the collision code: nodes of three bytes
 * (flags/count, split vertex, first polygon), node n has the children 2n and 2n + 1, node 1 is the root.
 */
typedef struct RpWorldSector {
    int32_t type;                      /* 0x00 negative */
    int16_t matListWindowBase;         /* 0x04 */
    uint16_t numVertices;              /* 0x06 */
    uint16_t numPolygons;              /* 0x08 */
    uint8_t pad0a[2];                  /* 0x0a */
    RpPolygon *polygons;               /* 0x0c */
    RwV3d *vertices;                   /* 0x10 */
    uint8_t pad14[0x10];               /* 0x14 */
    RwLLLink collAtomicsInWorldSector; /* 0x24 RpTie links of the atomics touching the sector */
    uint8_t pad2c[0x28];               /* 0x2c */
    const uint8_t *colSectorRoot;      /* 0x54 NULL: no polygons to test */
} RpWorldSector;

_Static_assert(offsetof(RpWorldSector, polygons) == 0x0c, "RpWorldSector layout");
_Static_assert(offsetof(RpWorldSector, collAtomicsInWorldSector) == 0x24, "RpWorldSector layout");
_Static_assert(offsetof(RpWorldSector, colSectorRoot) == 0x54, "RpWorldSector layout");

/* World (partial): the root of the BSP. */
typedef struct RpWorld {
    uint8_t pad00[0x18];
    RpSector *rootSector; /* 0x18 */
} RpWorld;

/* Links an atomic and a world sector it touches (in both of their lists). */
typedef struct RpTie {
    RwLLLink lAtomicInWorldSector; /* 0x00 in RpWorldSector.collAtomicsInWorldSector */
    RpAtomic *apAtom;              /* 0x08 */
    RwLLLink lWorldSectorInAtomic; /* 0x0c in RpAtomic.llWorldSectorsInAtomic */
    RpWorldSector *worldSector;    /* 0x14 */
} RpTie;

_Static_assert(sizeof(RpTie) == 0x18, "RpTie layout");

/* Vertex and polygon counts of a world sector (zero-extended 16-bit fields). */
int32_t RpWorldSectorGetNumVertices(const RpWorldSector *sector);
int32_t RpWorldSectorGetNumPolygons(const RpWorldSector *sector);

#endif
