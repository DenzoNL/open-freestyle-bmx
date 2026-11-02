/* Level geometry queries (C:\zaxis\Bmx\src\com\zge\NgIF\NgIFzbsp.c). */
#ifndef BMX_COM_ZGE_NGIF_NGIFZBSP_H
#define BMX_COM_ZGE_NGIF_NGIFZBSP_H

#include <stdint.h>

#include "bmx/com/action/physics_hit.h"
#include "compat/orig.h"
#include "zge/zgemods/zprop.h"
#include "zge/zgemods/ztaginf.h"
#include "zge/zmath.h"
#include "zge/zmem.h"

/* ---- level data --------------------------------------------------------------------------------- */

/* ZbspEdgeTag and ZbspLevelFace, the level's triangles, are zge/zgemods/ztaginf.h's. */

/* Source definition of a tree object; `kind` 1..4 maps to the shape kind (see zbspBuildShape). */
typedef struct ZbspObjectDef {
    uint8_t flags; /* bit 0: the prop's collision edges are loaded with the level */
    uint8_t pad01[0x15];
    uint8_t kind;
} ZbspObjectDef;

/* Collision shape of a tree object (0x38 bytes, built lazily by zbspBuildShape). */
typedef struct ZbspShape {
    uint16_t flags; /* bits 0-8 cleared when built */
    uint8_t byte2;
    uint8_t pad03;
    uint16_t word4;
    uint8_t byte6;
    uint8_t byte7;
    uint8_t pad08[0x1c];
    uint8_t kind; /* 0 none, 2: four edges, 3 and 4: one edge (1 is unimplemented) */
    uint8_t pad25[0x3];
    float box[4];
} ZbspShape;

_Static_assert(sizeof(ZbspShape) == 0x38, "ZbspShape size");
_Static_assert(offsetof(ZbspShape, kind) == 0x24, "ZbspShape.kind");
_Static_assert(offsetof(ZbspShape, box) == 0x28, "ZbspShape.box");

/* Static object of the level file (list head at 0x6722b8); its payload starts at +8 of the list node. */
/* ZbspTreeObject and ZbspTreeNode are the prop definitions of zge/zgemods/zprop.h. */

/* Flags of a prop collision object (PhysPropObject.flags), set by zbspObjectSignal. */
enum {
    kZbspObjGroupMask = 0x1f,
    kZbspObjFlagged = 0x40,
    kZbspObjSignalled = 0x80,
    kZbspObjTouched = 0x100,
    kZbspObjHidden = 0x200,
};

/* Level mesh (ztaginf.c getters) and the pools the face / edge records come from. */
typedef void *ZbspHitList[257];
#define gZbspHits          ORIG_VAR(0x005f0c68, ZbspHitList, gZbspHits)
#define gZbspFacePool      ORIG_VAR(0x005f34ac, ZMemFreeList *, gZbspFacePool)
#define gZbspEdgePool      ORIG_VAR(0x005f34b0, ZMemFreeList *, gZbspEdgePool)
#define gZbspPropFaceCount ORIG_VAR(0x005f34bc, uint32_t, gZbspPropFaceCount)
typedef PhysFace ZbspPropFaceTable[1];
#define gZbspPropFaces   ORIG_VAR(0x005e7fe0, ZbspPropFaceTable, gZbspPropFaces)
#define gZbspEdgeRecord  ORIG_VAR(0x005f1070, PhysEdge, gZbspEdgeRecord)
#define gZbspMatRefValue ORIG_VAR(0x005e7fd8, uint32_t, gZbspMatRefValue)
#define gZbspNoShapeKind ORIG_VAR(0x005f8d68, uint8_t, gZbspNoShapeKind)
/* Renderer entry that maps a world object's user data to its scene frame (0x560ee0). */
typedef PhysPropFrame *(BMX_CDECL *ZbspObjectFrameFn)(const void *objectData);
#define gZbspObjectFrameFn ORIG_VAR(0x00560ee0, ZbspObjectFrameFn, gZbspObjectFrameFn)
/* Renderer entry used by zbspGetMatRef (0x561038). */
typedef const uint32_t *(BMX_CDECL *ZbspMaterialFn)(const void *material);
#define gZbspMaterialFn ORIG_VAR(0x00561038, ZbspMaterialFn, gZbspMaterialFn)

/* ---- queries ------------------------------------------------------------------------------------ */

/* Centroid of the three float vertices of a face record (at +0x30, stride 0x10). */
void zbspFaceCentroid(const void *face, ZVec3 *out);
/*
 * Null-terminated list of edges within `radius` of p (static buffer): tagged level edges (records cached per
 * face edge) followed by the roller's prop edges.
 */
void **zbspFindEdgesNear(const ZVec3 *p, float radius);
/*
 * The `index`-th edge tagged `id` (only the low 16 bits count), rebuilt in a static record, or NULL. The
 * first and last parameters are unused.
 */
void *zbspFindEdge(int unused, int id, int index, int unused2);
/* Point of the segment a-b (int16 coordinates, direction dir) closest to p. */
void zbspClosestPointOnEdge(const int16_t *a, const int16_t *b, const ZVec3 *dir, const ZVec3 *p, ZVec3 *out);
/* Null-terminated list of faces whose tag id lies in [lo, hi] (static buffer), plus prop faces. */
void **zbspFindTaggedFaces(int lo, int hi);
/*
 * Faces near p (within `radius`, |normal.y| within `cosLimit`) as a ZbspFaceQuery in a static record, or
 * NULL.
 */
void *zbspFindFaceNear(const ZVec3 *p, float radius, float cosLimit);
/* Null-terminated list of the collision objects of props whose class lies in [lo, hi] (static buffer). */
void **zbspFindPropsNear(int lo, int hi);
/* Which side of an edge is solid: 1 (flag 1), -1 (flag 2) or 0. */
int zbspEdgeSide(const void *edge);

/* Null-terminated list of edge records whose tag id lies in [lo, hi] (static buffer). */
void **zbspFindTaggedEdges(int lo, int hi);
/* Fills `out` with the face record of a level face (corners, plane, int16 copies). */
void zbspBuildFace(ZbspLevelFace *src, const uint16_t *tag, PhysFace *out);
/* Point (x, y, z) inside the face's outline, tested in the plane's dominant axis. */
int zbspPointInFace(const PhysFace *face, float x, float y, float z);
/* Point of the segment a-b closest to p. */
void zbspClosestPointOnSegment(const ZVec3 *a, const ZVec3 *b, const ZVec3 *p, ZVec3 *out);

/* ---- props ---------------------------------------------------------------------------------------- */

void zbspBuildShape(ZbspTreeObject *obj);
int zbspTreeObjectIndex(const ZbspTreeObject *obj);
ZbspTreeObject *zbspTreeObjectAt(int index);
/* Shape of the tree object `index` (low 16 bits), or NULL. */
ZbspShape *zbspGetShape(int index);
/* Pointer to the shape kind byte of tree object `index`; a static zero byte when it has no shape. */
uint8_t *zbspGetShapeKind(int index);
/* Builds the collision object of a prop node (on first use). */
void zbspBuildCollObject(PhysPropNode *node);
void zbspFreeCollObject(PhysPropObject *object);
void zbspPropSetRotation(PhysPropObject *object, const ZAngle3 *rot);
void zbspPropRotate(PhysPropObject *object, const ZAngle3 *delta);
void zbspPropSetPosition(PhysPropObject *object, const ZVec3 *pos);
/* Reacts to a prop event 0..5 (set flags, show / hide). */
void zbspObjectSignal(PhysPropObject *object, int signal);
const uint32_t *zbspGetMatRef(const void *obj);
int zbspReturnZero(void);

/* One face of a zbspFindFaceNear result; the list ends with a NULL face. */
typedef struct ZbspFaceHit {
    void *face;  /* PhysFace */
    float dist;  /* distance of p above the face plane */
    ZVec3 point; /* closest point of the face */
    uint32_t pad14;
    int32_t object; /* owning prop's collision object, 0 for level geometry */
} ZbspFaceHit;

typedef struct ZbspFaceQuery {
    uint8_t pad00[0x3c];
    ZbspFaceHit *hits;
} ZbspFaceQuery;

BMX_LAYOUT32_ASSERT(sizeof(ZbspFaceHit) == 0x1c, "ZbspFaceHit size");
BMX_LAYOUT32_ASSERT(sizeof(ZbspFaceQuery) == 0x40, "ZbspFaceQuery size");

/* zbspFindFaceNear's buffers: the renderer's sphere test result, face records and their int16 corners. */
typedef struct ZbspSphereResult {
    struct ZrwColHit *hits;
    uint32_t count;
} ZbspSphereResult;
/* Corners of one zbspFindFaceNear face record: three int16 points, 12 bytes apart. */
typedef struct ZbspNearCorners {
    int16_t v[3][6];
} ZbspNearCorners;
enum { kZbspNearFaceMax = 255 };
typedef PhysFace ZbspNearFaceTable[kZbspNearFaceMax];
typedef ZbspNearCorners ZbspNearCornerTable[kZbspNearFaceMax];
typedef ZbspFaceHit ZbspNearHitTable[kZbspNearFaceMax + 1];
#define gZbspSphere      ORIG_VAR(0x005f0c20, ZbspSphereResult, gZbspSphere)
#define gZbspFaceQuery   ORIG_VAR(0x005f0c28, ZbspFaceQuery, gZbspFaceQuery)
#define gZbspNearFaces   ORIG_VAR(0x005e8420, ZbspNearFaceTable, gZbspNearFaces)
#define gZbspNearCorners ORIG_VAR(0x005f10a8, ZbspNearCornerTable, gZbspNearCorners)
#define gZbspNearHits    ORIG_VAR(0x005e63d8, ZbspNearHitTable, gZbspNearHits)

/* ---- debug-print stubs ---------------------------------------------------------------------------
 * The original build left these hooks as empty bodies that only print their name (file, line, name).
 */
int zbspInit(void);
void zbspUpdate(void);
int zbspShutdown(void);
int zbspInitializeScene(void);
int zbspLoadScene(void);
void zbspLoadAndProcessMap(void);
int zbspAllocAndLoadMap(void);
void zbspFreeMap(void);
int zbspSetTreeVisible(void);
void *zbspConvertTreeFromLoad(void *tree);
void zbspRelativeLodZ(void);
void zbspPreProcessScene(void);
void zbspPreProcessTerrainNode(void);
void zbspPreProcessFaceList(void);
int zbspViewerInPosSide(void);
void zbspSndRegister(void);
void zbspSndSetState(void);
int zbspSndGetHandle(void);
int zbspDvliteHandle(void);
int zbspDvliteGetColor(void);
void zbspDvliteLite(void);
void zbspDvliteInitialize(void);
void zbspDvliteAttachToProps(void);
void zbspPropActivate(void);
void zbspPropDeactivate(void);
void zbspActivateTree(void);
void zbspDeactivateTree(void);
int zbspInitializeTreeRef(void);
int16_t zbspCreateTreeRef(void);
void zbspDestroyTreeRef(void);
void zbspPropAddTreeRef(void);
void zbspRemoveTreeRef(void);
void zbspUpdateTreeRef(void);
void zbspAddToTree(void);
void zbspRemoveFromTree(void);
int zbspTotallyWithinTree(void);
int zbspOverlapsTree(void);
int zbspFindTagged(void);
void zbspPropMove(void);
int zbspGetNumDefs(void);
int zbspGetTypeFromHash(void);
void zbspFxCreateTree(void);
void zbspDestroyTree(void);
void zbspDrawTerrain(void);
void zbspDrawTerrainNode(void);
void zbspDrawFaceList(void);
void zbspSetDebug(void);
void zbspDrawLineZ(void);
void zbspDrawFaceNormal(void);
void zbspDrawFaceFlat(void);
void zbspShowBoundingBox(void);
void zbspShowEdges(void);
void zbspShowPropVolume(void);
int zbspGetFaceNum(void);
void zbspTrnDumpTree(void);
void zbspShowPartition(void);
void zbspShowPartition2(void);
void zbspDrawPartition(void);
int zbspGetTerrainFaces(void);
int zbspGetTreeObject(void);
int zbspPointWithinTreeObject(void);
int zbspGetTreeObjects(void);
void zbspGetTerrainNode(void);
int zbspGetMaterialInfo(void);
int zbspGetMatRefData(void);
int zbspGetMatIdx(void);
int zbspMatRefList(void);
int zbspFindTagged2(void);
int zbspDoesVertExist(void);
int zbspFacesAtSamePos(void);
int zbspSceneSaveState(void);
void zbspSceneRestoreState(void);
void zbspGetNormalVector(void);
int zbspGetSideVector(void);
int zbspCollide(void);
void zbspCreateSpanPlanes(void);
void zbspAddToFacelist(void);
void zbspGetTerrainHeightinf(void);
void zbspTestRayCollision(void);
void zbspTest(void);
void zbspTestResetFaceColors(void);
void zbspHighlightNodeFaces(void);
void zbspSetFaceRgb(void);
void zbspPrintFaceListInf(void);
int zbspTrnTestRayCollision(void);
void zbspCreateNodesHit(void);
void zbspAddToNodeslist(void);
void zbspAddToFacesList(void);
int zbspLineToNodes(void);
int zbspLineToFaces(void);
int zbspLinePlaneIntersection(void);
int zbspClipRayToObject(void);
int zbspClipLineToProps(void);
int zbspClipLineToProp(void);
void zbspBuildRayBoundingBox(void);
uint8_t *zbspAddPropFaces(uint8_t *prop);
void zbspComputeFacePlaneEq(void);
void zbspTagInitialize(void);
void zbspTagRegisterProFunc(void);
int zbspFaceTagRegister(void);
int zbspFaceTagPreProcess(void);
int zbspFaceTagProcess(void);
int zbspFaceTagFindDef(void);
int zbspFaceTagRegRegister(void);
int zbspFaceTagPreAttachAsRef(void);
int zbspFaceTagPreAttachSmPosAsRef(void);
int zbspFaceTagProSwapWindow(void);
void zbspColSphereCreateNodesHit(void);
int zbspHitNodeslist(void);
int zbspTestWithFacelist(void);
void *zbspFaceBoundingBox(void *face, void *box);
void zbspColSphereDraw(void);
int zbspFindFaceByType(void);

#endif
