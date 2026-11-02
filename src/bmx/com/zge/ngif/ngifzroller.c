/* Level renderer glue (C:\zaxis\Bmx\src\com\zge\NgIF\NgIFzroller.c). */
#include "bmx/com/zge/ngif/ngifzroller.h"

#include "bmx/com/zge/ngif/ngifzactor.h"
#include "bmx/com/zge/ngif/ngifzbsp.h"
#include "bmx/com/zge/ngif/ngifzcollide.h"
#include "bmx/com/zge/ngif/ngifzres.h"
#include "compat/crt.h"
#include "zge/com/zglrw/zrwcam.h"
#include "zge/zdebug.h"
#include "zge/zgemods/zobjcamera.h"
#include "zge/zlist.h"
#include "zge/zmem.h"
#include "zge/zobj.h"
#include "zge/zquat.h"
#include "zge/zscene.h"
#include "zge/zstring.h"

static const char kRollerFile[] = "C:\\zaxis\\Bmx\\src\\com\\zge\\NgIF\\NgIFzroller.c";

/* What every hook stub prints: "<file> <line> <name>". */
static void trace(int line, const char *what)
{
    gZdebugPrint("%s %d %s\n", kRollerFile, line, what);
}

static const char *tableName(const ZrollerNameTable *table, uint32_t index)
{
    return ((const char *const *)table)[index];
}

/* Model handle of a scene object. */
static void *modelOf(const void *object)
{
    return ((const ZObj *)object)->model;
}

/* x87 FCOM + `TEST AH,1`: true for a < b and for unordered operands. */
static int lessOrUnordered(double a, double b)
{
    return !(a >= b);
}

/* Frees the handle of every model table entry, then clears the table. */
BMX_ADDR(0x0046c1d0)
void zrollerFreeModelTable(void)
{
    for (int i = 0; i < 512; i++) {
        if (gZrollerModels[i].handle != 0) {
            gZrollerFreeHandle(gZrollerModels[i].handle);
        }
    }
    zmemFill(&gZrollerModels, 0, sizeof gZrollerModels);
}

/*
 * Sets up the shadow state, creates the face (0x88 byte) and edge (0x38 byte) pools and the edge list on
 * first use, and starts the resource tables. Returns 1 on success.
 */
BMX_ADDR(0x0046c220)
int zrollerInit(void)
{
    gZrollerShadow.shadow.kind = 2;
    gZrollerShadow.shadow.offset.x = 0.0f;
    gZrollerShadow.shadow.offset.y = -500.0f;
    gZrollerShadow.shadow.offset.z = 0.0f;
    gZrollerShadow.zero1c = 0;
    gZrollerShadow.planeB = -5.0f;
    gZrollerShadow.zero24 = 0;
    if (gZbspEdgePool == NULL) {
        gZbspEdgePool = zmemFreeListCreate(sizeof(PhysEdge), 0x200, NULL);
    }
    if (gZbspFacePool == NULL) {
        gZbspFacePool = zmemFreeListCreate(sizeof(PhysFace), 0x200, NULL);
    }
    gZrollerShadow.shadow.slots = gZrollerShadowSlots;
    if (gZrollerEdges == NULL) {
        gZrollerEdges = zlistCreate(0x80);
    }
    return ngifzresInit() != 0;
}

/* Resets the reservation counter and the draw callbacks. Returns 1 (after a debug print) if zres fails. */
BMX_ADDR(0x0046c310)
int zrollerStartup(void)
{
    zrollerResetReserve();
    if (ngifzresReset() != 0) {
        gZdebugPrint("ZROLLER: Failed to intialize ZRES\n");
        return 1;
    }
    for (int i = 0; i < 8; i++) {
        gZrollerDrawCallbacks[i] = NULL;
    }
    gZrollerSlotCount = 0;
    return 0;
}

BMX_ADDR(0x0046c360)
int zrollerShutdown(void)
{
    ngifzresShutdown();
    return 0;
}

/* zmemFreeListForEach callback: gives the payload back to its list. */
BMX_ADDR(0x0046c380)
void zrollerFreeNode(void *payload, void *list)
{
    zmemFreeListFree(list, payload);
}

BMX_ADDR(0x0046c3b0)
void zrollerFreeLists(void)
{
    if (gZrollerEdges != NULL) {
        zlistClear(gZrollerEdges, NULL, NULL);
    }
    if (gZbspFacePool != NULL) {
        zmemFreeListForEach(gZbspFacePool, BMX_FNPTR(ZrollerFreeNodeFn, 0x00401a9b, zrollerFreeNode),
                            gZbspFacePool);
    }
    if (gZbspEdgePool != NULL) {
        zmemFreeListForEach(gZbspEdgePool, BMX_FNPTR(ZrollerFreeNodeFn, 0x00401a9b, zrollerFreeNode),
                            gZbspEdgePool);
    }
}

BMX_ADDR(0x0046c420)
int zrollerRunDrawCallbacks(void)
{
    for (int i = 0; i < 8; i++) {
        if (gZrollerDrawCallbacks[i] != NULL) {
            gZrollerDrawCallbacks[i]();
        }
    }
    return 0;
}

/*
 * Builds the collision objects of all props and, for those whose definition asks for it (flag 1),
 * their collision edges, which go into the edge list.
 */
BMX_ADDR(0x0046c4b0)
int zrollerLoadAssets(void)
{
    PhysEdge *edges[6];

    for (PhysPropNode *n = *zobjPropList(); n != NULL; n = n->next) {
        const ZbspObjectDef *def;
        int count;

        if (n->object == NULL) {
            zbspBuildCollObject(n);
        }
        def = n->source != NULL ? n->source->def : NULL;
        if (def == NULL || (def->flags & 1) == 0) {
            continue;
        }
        count = zcollideBuildPropEdges(n->object, edges);
        for (int i = 0; i < count; i++) {
            zlistInsert(gZrollerEdges, edges[i]);
        }
    }
    gZdebugPrint("%s %d %s\n", kRollerFile, 0x168, "zrollerLoadAssets");
    return 0;
}

/*
 * Creates the rider actor of a player definition {rider id, skin}: finds the rider model ("<name><skin+1>")
 * and its shadow in the scene, hooks the shadow update and builds the actor. At most two players.
 */
BMX_ADDR(0x0046c580)
ZActor *zrollerCreatePlayerActor(const uint16_t *def)
{
    char name[0x40];
    uint8_t color[4];
    void *riderObject;
    void *shadowObject;
    ZActor *actor;
    int len;

    if (gZrollerSlotCount >= 2) {
        zdebugFail("Out of player defs", kRollerFile, 0x19b, 0);
    }
    zmemCopy(
        ((ZrollerSlot *)((uintptr_t)&gZrollerSlots + (size_t)gZrollerSlotCount * sizeof(ZrollerSlot)))->def,
        def, 4);
    gZrollerSlotCount++;
    zactorClearPoseTables();
    zstrcpy(name, tableName(&gZrollerRiderNames, def[0]));
    {
        const char skin = (char)(((const uint8_t *)def)[2] + '1');

        len = zstrlen(name);
        name[len - 1] = skin;
    }
    riderObject = zsceneFindModel(0, name);
    shadowObject = zsceneFindModel(0, "Rider Shadow");
    gZrollerShadow.shadow.slots[0] = zobjGetModel(shadowObject);
    color[0] = color[1] = color[2] = color[3] = 0xff;
    gZobjSetColor(zobjGetModel(shadowObject), color);
    gZobjSetFlags(modelOf(riderObject), 8, 0);
    zobjSetVisible((int32_t)(uintptr_t)shadowObject, 0);
    if (gZrollerUpdateHandle != 0) {
        gZrollerUnregister(gZrollerUpdateHandle);
    }
    gZrollerUpdateHandle =
        gZrollerRegister(5, BMX_FNPTR(void *, 0x0049b520, zrollerUpdateShadow), &gZrollerShadow, 1);
    actor = zactorCreate(1);
    if (actor == NULL) {
        return NULL;
    }
    zactorSetSkeleton(actor, 0);
    actor->object = (int32_t)(uintptr_t)riderObject;
    actor->shadow = (int32_t)(uintptr_t)shadowObject;
    zactorSetupRider(actor);
    return actor;
}

/* Creates the bike actor for bike number *def (0..0x30) with its shadow. */
BMX_ADDR(0x0046c740)
ZActor *zrollerCreateBikeActor(const uint32_t *def)
{
    uint8_t color[4];
    void *bikeObject;
    void *shadowObject;
    ZActor *actor = zactorCreate(1);

    if (actor == NULL) {
        return NULL;
    }
    if (*def >= 0x31) {
        zdebugFail("Invalid bike selected!", kRollerFile, 0x233, 0);
    }
    bikeObject = zsceneFindModel(0, tableName(&gZrollerBikeNames, *def));
    shadowObject = zsceneFindModel(0, "Bike Shadow");
    gZrollerShadow.shadow.model = zobjGetModel(bikeObject);
    gZrollerShadow.shadow.slots[1] = zobjGetModel(shadowObject);
    color[0] = color[1] = color[2] = color[3] = 0xff;
    gZobjSetColor(zobjGetModel(shadowObject), color);
    gZobjSetFlags(modelOf(bikeObject), 8, 0);
    zobjSetVisible((int32_t)(uintptr_t)shadowObject, 0);
    zactorSetSkeleton(actor, 1);
    actor->object = (int32_t)(uintptr_t)bikeObject;
    actor->shadow = (int32_t)(uintptr_t)shadowObject;
    zactorSetupBike(actor);
    return actor;
}

/* Player slot record `index`, NULL when out of range. */
BMX_ADDR(0x0046c920)
const uint8_t *zrollerGetRecord(uint32_t index)
{
    if (index < (uint32_t)gZrollerSlotCount) {
        return (const uint8_t *)&gZrollerSlots + (size_t)index * sizeof(ZrollerSlot);
    }
    return NULL;
}

BMX_ADDR(0x0046cc80)
void zrollerResetReserve(void)
{
    gZrollerReserved = 0;
}

BMX_ADDR(0x0046ccd0)
int zrollerReserve(int count)
{
    gZrollerReserved += count;
    return gZrollerReserved - count;
}

/* Asks the renderer for the world bounds and keeps them in a static record. */
BMX_ADDR(0x0046ceb0)
const float *zrollerGetWorldBounds(void)
{
    float box[8];

    gZrollerQuery(2, box, 0);
    gZrollerBounds[0] = box[0];
    gZrollerBounds[1] = box[1];
    gZrollerBounds[2] = box[2];
    gZrollerBounds[4] = box[4];
    gZrollerBounds[5] = box[5];
    gZrollerBounds[6] = box[6];
    return gZrollerBounds;
}

/*
 * Start point and yaw of a player. The point is the "Player<n>" marker; the yaw comes from the "PlayerT<n>"
 * marker (facing it) or, without one, from the start marker's own orientation. Without a start marker the
 * original asserts and falls back to (0, -1000, 0), zero angles.
 */
BMX_ADDR(0x0046cf30)
void zrollerGetPlayerStartPos(int player, ZVec3 *pos, ZAngle3 *rot)
{
    ZrwMarker *start = zrwcamFindMarker(zstrHash("Player"), player + 1);
    ZrwMarker *target = zrwcamFindMarker(zstrHash("PlayerT"), player + 1);

    if (start == NULL) {
        zdebugFail("Player start point not found - Using default value", kRollerFile, 0x563, 0);
        gZdebugPrint("%s %d %s\n", kRollerFile, 0x564, "zrollerGetPlayerStartPos: HARDCODED");
        pos->x = 0.0f;
        pos->y = -1000.0f;
        pos->z = 0.0f;
        rot->x = 0;
        rot->y = 0;
        rot->z = 0;
        return;
    }
    *pos = start->matrix.pos;
    if (target == NULL) {
        ZQuat q;
        ZMatrix m;
        ZAngle3 euler;

        zquatFromMatrix(&q, &start->matrix);
        if (!lessOrUnordered(__builtin_fabsf(q.y), 0x1.47ae14p-7f)) {
            q.w = -q.w;
        } else {
            q.x = -q.x;
            q.z = -q.z;
        }
        zquatToMatrix(&m, &q);
        zmatrixToEuler(&euler, &m, 0);
        *rot = euler;
    } else {
        ZVec3 a = start->matrix.pos;
        ZVec3 dir = target->matrix.pos;
        const ZVec3 xAxis = {1.0f, 0.0f, 0.0f};
        ZAngle angle;

        zvec3Sub(&dir, &dir, &a);
        dir.y = 0.0f;
        zvec3NormalizeScaled(&dir, &dir);
        rot->x = 0;
        angle = zmathAcos((float)zvec3Dot(&dir, &xAxis));
        rot->z = 0;
        rot->y = lessOrUnordered(dir.z, 0.0) ? angle : (ZAngle)-angle;
    }
}

BMX_ADDR(0x0046d350)
int zrollerReturnZero(void)
{
    return 0;
}

BMX_ADDR(0x0046d3a0)
void zrollerNop1(void) {}

BMX_ADDR(0x0046d3c0)
void zrollerNop2(void) {}

/* Registers a debug draw callback in the first free of eight slots. */
BMX_ADDR(0x0046d690)
void zrollerAddDrawCallback(ZrollerDrawFn fn)
{
    for (int i = 0; i < 8; i++) {
        if (gZrollerDrawCallbacks[i] == NULL) {
            gZrollerDrawCallbacks[i] = fn;
            return;
        }
    }
    zdebugFail("registered too many draw callbacks to zroller", kRollerFile, 0x712, 0);
}

/* ---- debug-print stubs ----------------------------------------------------------------------------- */

BMX_ADDR(0x0046c450)
int zrollerUpdateZGL(void)
{
    trace(282, "zrollerUpdateZGL");
    return 0;
}

BMX_ADDR(0x0046c480)
int zrollerCreateChaser(void)
{
    trace(300, "zrollerCreateChaser");
    return 0;
}

BMX_ADDR(0x0046c860)
int zrollerCreateProp(void)
{
    trace(625, "zrollerCreateProp");
    return 0;
}

BMX_ADDR(0x0046c890)
int zrollerSetEngineState(void)
{
    trace(643, "zrollerSetEngineState");
    return 0;
}

BMX_ADDR(0x0046c8c0)
int zrollerGetNumPlayerDefs(void)
{
    trace(678, "zrollerGetNumPlayerDefs");
    return 0;
}

BMX_ADDR(0x0046c8f0)
int zrollerLoadMdlPlayerCB(void)
{
    trace(699, "zrollerLoadMdlPlayerCB");
    return 0;
}

BMX_ADDR(0x0046c950)
int zrollerIsPlayerClose(void)
{
    trace(739, "zrollerIsPlayerClose");
    return 0;
}

BMX_ADDR(0x0046c980)
void zrollerChangePlayerModeLook(void)
{
    trace(758, "ChangePlayerModeLook");
}

BMX_ADDR(0x0046c9b0)
void zrollerAddPlayerMatsToLoadList(void)
{
    trace(777, "zrollerAddPlayerMatsToLoadList");
}

BMX_ADDR(0x0046c9e0)
void zrollerChangePlayerSponsor(void)
{
    trace(795, "zrollerChangePlayerSponsor");
}

BMX_ADDR(0x0046ca10)
void zrollerChangeBoardSponsor(void)
{
    trace(812, "Zrollerchangeboardsponsor");
}

BMX_ADDR(0x0046ca40)
int zrollerLoadMdlGenericCB(void)
{
    trace(828, "zrollerLoadMdlGenericCB");
    return 0;
}

BMX_ADDR(0x0046ca70)
int zrollerLoadMdlCommon(void)
{
    trace(851, "zrollerLoadMdlCommon");
    return 0;
}

BMX_ADDR(0x0046caa0)
void zrollerLoadMdlCommonMat(void)
{
    trace(873, "LoadMdlCommonMat");
}

BMX_ADDR(0x0046cad0)
void zrollerProcessModelFlags(void)
{
    trace(891, "zrollerProcessModelFlags");
}

BMX_ADDR(0x0046cb00)
void zrollerProcessActorDrawFlags(void)
{
    trace(907, "zrollerProcessActorDrawFlags");
}

BMX_ADDR(0x0046cb30)
void zrollerSetAnimMaterialPalette(void)
{
    trace(944, "zrollerSetAnimMaterialPalette");
}

BMX_ADDR(0x0046cb60)
void zrollerInitializeLoadingScreen(void)
{
    trace(960, "zrollerInitializeLoadingScreen");
}

BMX_ADDR(0x0046cb90)
void zrollerShutdownLoadingScreen(void)
{
    trace(976, "zrollerShutdownLoadingScreen");
}

BMX_ADDR(0x0046cbc0)
void zrollerDisplayLoadingScreen(void)
{
    trace(992, "zrollerDisplayLoadingScreen");
}

BMX_ADDR(0x0046cbf0)
void zrollerSetLocal(void)
{
    trace(1000, "zrollerSetLocal");
}

BMX_ADDR(0x0046cc20)
int zrollerLoadModule(void)
{
    trace(1014, "zrollerLoadModule");
    return 0;
}

BMX_ADDR(0x0046cc50)
int zrollerLoadCodeModule(void)
{
    trace(1029, "LoadCodeModule");
    return 0;
}

BMX_ADDR(0x0046cca0)
void zrollerAllocMaterials(void)
{
    trace(1065, "zrollerAllocMaterials");
}

BMX_ADDR(0x0046cd00)
int zrollerAddGenericMaterialsToLoad(void)
{
    trace(1098, "zrollerAddGenericMaterialsToLoad");
    return 0;
}

BMX_ADDR(0x0046cd30)
void zrollerInitializeMaps(void)
{
    trace(1135, "zrollerInitializeMaps");
}

BMX_ADDR(0x0046cd60)
void zrollerSetMap(void)
{
    trace(1151, "zrollerSetMap");
}

BMX_ADDR(0x0046cd90)
int zrollerLoadMaps(void)
{
    trace(1168, "zrollerLoadMaps");
    return 0;
}

BMX_ADDR(0x0046cdc0)
int zrollerLoadMatTerrainFirstCB(void)
{
    trace(1186, "zrollerLoadMatTerrainFirstCB");
    return 0;
}

BMX_ADDR(0x0046cdf0)
int zrollerLoadMatTerrainLastCB(void)
{
    trace(1205, "zrollerLoadMatTerrainLastCB");
    return 0;
}

BMX_ADDR(0x0046ce20)
void zrollerAssociateMapMaterials(void)
{
    trace(1223, "zrollerAssociateMapMaterials");
}

BMX_ADDR(0x0046ce50)
void zrollerAssociateMaterial(void)
{
    trace(1242, "zroolerAssociateMaterial llljlkfsdjflsjdklf");
}

BMX_ADDR(0x0046ce80)
void zrollerAssociateMaterialPalAnimation(void)
{
    trace(1258, "Associate material pal animation");
}

BMX_ADDR(0x0046d200)
int zrollerGetPlayerRestartPos(void)
{
    trace(1411, "zrollerGetPlayerRestartPos");
    return 0;
}

BMX_ADDR(0x0046d230)
void zrollerInitPropLoadList(void)
{
    trace(1429, "zrollerInitPropLoadList");
}

BMX_ADDR(0x0046d260)
void zrollerAddPropToLoadList(void)
{
    trace(1445, "zrollerAddPropToLoadList");
}

BMX_ADDR(0x0046d290)
void zrollerCreateProps(void)
{
    trace(1461, "Create Props");
}

BMX_ADDR(0x0046d2c0)
void zrollerLoadPropMaterials(void)
{
    trace(1477, "zrollerLoadPropMaterials");
}

BMX_ADDR(0x0046d2f0)
int zrollerCreatePropActor(void)
{
    trace(1494, "zrollerCreatePropActor");
    return 0;
}

BMX_ADDR(0x0046d320)
int zrollerDestroyActor(void)
{
    trace(1512, "zrollerDestroyActor");
    return 0;
}

BMX_ADDR(0x0046d370)
int zrollerGetAsyncKeyState(void)
{
    trace(1544, "zrollerGetAsyncKeyState");
    return 0;
}

BMX_ADDR(0x0046d3e0)
int zrollerSelectMap(void)
{
    trace(1585, "zrollerSelectMap");
    return 0;
}

BMX_ADDR(0x0046d410)
int zrollerSelectPlayer(void)
{
    trace(1599, "zrollerSelectPlayer");
    return 0;
}

BMX_ADDR(0x0046d440)
int zrollerSelectBikeNum(void)
{
    trace(1614, "zrollerSelectBikeNum");
    return 0;
}

BMX_ADDR(0x0046d470)
void zrollerSetFogDistance(void)
{
    trace(1637, "set fog distance");
}

BMX_ADDR(0x0046d4a0)
void zrollerGetFogDistance(void)
{
    trace(1647, "get fog distance");
}

BMX_ADDR(0x0046d4d0)
void zrollerDisableFog(void)
{
    trace(1657, "zrollerDisableFog");
}

BMX_ADDR(0x0046d500)
void zrollerSetFogColor(void)
{
    trace(1667, "zrollerSetFogColor");
}

BMX_ADDR(0x0046d530)
void zrollerReplaceMaterial(void)
{
    trace(1677, "zrollerReplaceMaterial");
}

BMX_ADDR(0x0046d560)
void zrollerSetShadeMode(void)
{
    trace(1692, "zrollerSetShadeMode");
}

BMX_ADDR(0x0046d590)
void zrollerStartCrossFade(void)
{
    trace(1709, "zrollerStartCrossFade");
}

BMX_ADDR(0x0046d5c0)
int zrollerGetSceneState(void *scene, int *state)
{
    trace(1731, "zrollerGetSceneState: NO DATA");
    if (state != NULL) {
        *state = 0;
    }
    return 0;
}

BMX_ADDR(0x0046d600)
void zrollerFreeSceneState(void)
{
    trace(1755, "FreeSceneState");
}

BMX_ADDR(0x0046d630)
void zrollerSetSceneState(void)
{
    trace(1770, "SetSceneState");
}

BMX_ADDR(0x0046d660)
int zrollerSelectLook(void)
{
    trace(1785, "zrollerSelectLook()");
    return 0;
}
