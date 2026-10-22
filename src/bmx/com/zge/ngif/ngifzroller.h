/* Level renderer glue (C:\zaxis\Bmx\src\com\zge\NgIF\NgIFzroller.c). */
#ifndef BMX_COM_ZGE_NGIF_NGIFZROLLER_H
#define BMX_COM_ZGE_NGIF_NGIFZROLLER_H

#include <stdint.h>

#include "bmx/com/zge/zactor.h"
#include "compat/orig.h"
#include "zge/zlist.h"
#include "zge/zmath.h"

typedef int(BMX_CDECL *ZrollerDrawFn)(void);

/* The eight debug draw callbacks (0x5fb6c8..0x5fb6e8) and the resource counter behind zrollerReserve. */
typedef ZrollerDrawFn ZrollerDrawTable[8];
#define gZrollerDrawCallbacks ORIG_VAR(0x005fb6c8, ZrollerDrawTable, gZrollerDrawCallbacks)
#define gZrollerReserved      ORIG_VAR(0x005fb6e8, int, gZrollerReserved)

/* Collision edges of all props (a ZList of PhysEdge pointers, capacity 0x80). */
#define gZrollerEdges ORIG_VAR(0x00678494, ZList *, gZrollerEdges)

/* Player slot: the 4 byte definition {rider id, ?, skin} is copied to +4. At most two slots. */
typedef struct ZrollerSlot {
    uint8_t pad00[0x4];
    uint8_t def[4];
    uint8_t pad08[0x8];
} ZrollerSlot;
typedef ZrollerSlot ZrollerSlotTable[2];
#define gZrollerSlots     ORIG_VAR(0x005fb6f0, ZrollerSlotTable, gZrollerSlots)
#define gZrollerSlotCount ORIG_VAR(0x005fb738, int, gZrollerSlotCount)

/* State shared with the shadow update callback (0x49b520), at 0x678440. */
typedef struct ZrollerShadow {
    int32_t kind;   /* 2 */
    int32_t *slots; /* model handles: [0] rider shadow, [1] bike shadow (0x5fb710) */
    int32_t bikeModel;
    uint32_t zero0c;
    float planeA; /* -500 */
    uint32_t zero14;
    uint32_t pad18;
    uint32_t zero1c;
    float planeB; /* -5 */
    uint32_t zero24;
} ZrollerShadow;

#define gZrollerShadow       ORIG_VAR(0x00678440, ZrollerShadow, gZrollerShadow)
#define gZrollerShadowSlots  ORIG_VAR(0x005fb710, int32_t, gZrollerShadowSlots)
#define gZrollerUpdateHandle ORIG_VAR(0x005fb740, int, gZrollerUpdateHandle)

/* Renderer hooks: free a model table handle, query world data, (un)register update callbacks. */
typedef void(BMX_CDECL *ZrollerFreeHandleFn)(int handle);
typedef void(BMX_CDECL *ZrollerQueryFn)(int what, float *out, int arg);
typedef int(BMX_CDECL *ZrollerRegisterFn)(int priority, void *callback, void *arg, int flag);
typedef void(BMX_CDECL *ZrollerUnregisterFn)(int handle);
#define gZrollerFreeHandle ORIG_VAR(0x00561024, ZrollerFreeHandleFn, gZrollerFreeHandle)
#define gZrollerQuery      ORIG_VAR(0x00561488, ZrollerQueryFn, gZrollerQuery)
#define gZrollerRegister   ORIG_VAR(0x00561470, ZrollerRegisterFn, gZrollerRegister)
#define gZrollerUnregister ORIG_VAR(0x00561474, ZrollerUnregisterFn, gZrollerUnregister)
typedef void(BMX_CDECL *ZrollerFreeNodeFn)(void *payload, void *list);
/* Update callback of the shadow state (zrwmodelcreate.c by address). */
void zrollerUpdateShadow(void *state);

/* World bounds record (0x5fb718) returned by zrollerGetWorldBounds. */
typedef float ZrollerBounds[7];
#define gZrollerBounds ORIG_VAR(0x005fb718, ZrollerBounds, gZrollerBounds)

/* Rider and bike model names (tables of const char *: 12 riders at 0x55dc50, bikes at 0x55dc80). */
typedef const char *ZrollerNameTable[1];
#define gZrollerRiderNames ORIG_VAR(0x0055dc50, ZrollerNameTable, gZrollerRiderNames)
#define gZrollerBikeNames  ORIG_VAR(0x0055dc80, ZrollerNameTable, gZrollerBikeNames)

/* Table of the renderer models the roller loaded: 512 entries of 0x14 bytes, the handle at +0x10. */
typedef struct ZrollerModelEntry {
    uint8_t pad00[0x10];
    int32_t handle;
} ZrollerModelEntry;
typedef ZrollerModelEntry ZrollerModelTable[512];
#define gZrollerModels ORIG_VAR(0x0068c3e0, ZrollerModelTable, gZrollerModels)

/* Adds a debug draw callback (asserts when all eight slots are taken). */
void zrollerAddDrawCallback(ZrollerDrawFn fn);
/* World bounding box {min x, y, z, pad, max x, y, z} in a static buffer. */
const float *zrollerGetWorldBounds(void);

/* Pointer to a 0x10 byte record of the roller table, or NULL. */
const uint8_t *zrollerGetRecord(uint32_t index);
/* Reserves `count` slots of the roller resource counter and returns the first. */
int zrollerReserve(int count);

/* Resets the model table: frees every handle and clears the 0x2800 bytes. */
void zrollerFreeModelTable(void);
/* Initialises the shadow state, the face / edge pools and the edge list; returns 1 on success. */
int zrollerInit(void);
/* Resets the counter and the player slots, installs no draw callback; returns 1 when zres fails. */
int zrollerStartup(void);
int zrollerShutdown(void);
/* Pool callback: gives a payload back to its free list (arg). */
void zrollerFreeNode(void *payload, void *list);
/* Releases the edge list and the face / edge pools. */
void zrollerFreeLists(void);
/* Calls the registered draw callbacks. Always returns 0. */
int zrollerRunDrawCallbacks(void);
/* Lists the collision edges of every prop with a loadable shape in the edge list. */
int zrollerLoadAssets(void);
/* Builds the rider actor for a player definition {rider id, skin} (the original's CreateActor). */
ZActor *zrollerCreatePlayerActor(const uint16_t *def);
ZActor *zrollerCreateBikeActor(const uint32_t *def);
void zrollerResetReserve(void);
/* Start point and orientation of player `player` (from the level's Player / PlayerT markers). */
void zrollerGetPlayerStartPos(int player, ZVec3 *pos, ZAngle3 *rot);
int zrollerReturnZero(void);
void zrollerNop1(void);
void zrollerNop2(void);

/* ---- debug-print stubs ---------------------------------------------------------------------------
 * Empty hooks of the original that only print their name.
 */
int zrollerUpdateZGL(void);
int zrollerCreateChaser(void);
int zrollerCreateProp(void);
int zrollerSetEngineState(void);
int zrollerGetNumPlayerDefs(void);
int zrollerLoadMdlPlayerCB(void);
int zrollerIsPlayerClose(void);
void zrollerChangePlayerModeLook(void);
void zrollerAddPlayerMatsToLoadList(void);
void zrollerChangePlayerSponsor(void);
void zrollerChangeBoardSponsor(void);
int zrollerLoadMdlGenericCB(void);
int zrollerLoadMdlCommon(void);
void zrollerLoadMdlCommonMat(void);
void zrollerProcessModelFlags(void);
void zrollerProcessActorDrawFlags(void);
void zrollerSetAnimMaterialPalette(void);
void zrollerInitializeLoadingScreen(void);
void zrollerShutdownLoadingScreen(void);
void zrollerDisplayLoadingScreen(void);
void zrollerSetLocal(void);
int zrollerLoadModule(void);
int zrollerLoadCodeModule(void);
void zrollerAllocMaterials(void);
int zrollerAddGenericMaterialsToLoad(void);
void zrollerInitializeMaps(void);
void zrollerSetMap(void);
int zrollerLoadMaps(void);
int zrollerLoadMatTerrainFirstCB(void);
int zrollerLoadMatTerrainLastCB(void);
void zrollerAssociateMapMaterials(void);
void zrollerAssociateMaterial(void);
void zrollerAssociateMaterialPalAnimation(void);
int zrollerGetPlayerRestartPos(void);
void zrollerInitPropLoadList(void);
void zrollerAddPropToLoadList(void);
void zrollerCreateProps(void);
void zrollerLoadPropMaterials(void);
int zrollerCreatePropActor(void);
int zrollerDestroyActor(void);
int zrollerGetAsyncKeyState(void);
int zrollerSelectMap(void);
int zrollerSelectPlayer(void);
int zrollerSelectBikeNum(void);
void zrollerSetFogDistance(void);
void zrollerGetFogDistance(void);
void zrollerDisableFog(void);
void zrollerSetFogColor(void);
void zrollerReplaceMaterial(void);
void zrollerSetShadeMode(void);
void zrollerStartCrossFade(void);
int zrollerGetSceneState(void *scene, int *state);
void zrollerFreeSceneState(void);
void zrollerSetSceneState(void);
int zrollerSelectLook(void);

/* Start position and orientation of player `index` (HARDCODED fallback when the level lacks one). */
ORIG_FN(0x0046cf30, void, zrollerGetStart, (int index, ZVec3 *pos, ZAngle3 *rot));
/* Creates the bike object for look `*lookId` (asserts <= 0x30). */
ORIG_FN(0x0046c740, void *, zrollerCreateBike, (const int32_t *lookId));

#endif
