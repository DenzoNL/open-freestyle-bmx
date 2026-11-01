/* Renderer world and effect object (see zrwworld.h). */
#include "zge/com/zglrw/zrwworld.h"

#include "compat/crt.h"
#include "rw/rwcamera.h"
#include "rw/rwclump.h"
#include "rw/rwcore.h"
#include "rw/rwlight.h"
#include "rw/rwmath.h"
#include "rw/rwstream.h"
#include "rw/rwtexture.h"
#include "rw/rwworld.h"
#include "zge/com/zglrw/zrwcam.h"
#include "zge/com/zglrw/zrwmat.h"
#include "zge/com/zglrw/zrwtxr.h"
#include "zge/zdebug.h"
#include "zge/zfile.h"
#include "zge/zgemods/zobjcamera.h"
#include "zge/ziff.h"
#include "zge/zlist.h"
#include "zge/zmem.h"
#include "zge/zobj.h"
#include "zge/zstring.h"

static const char kZrwWorldFile[] = "C:\\NgEngine\\Zge\\Com\\ZglRw\\zrwWorld.c";

/* zrwzgl.h cannot be included next to zrwmodel.h (it names 0x697720 / 0x697724 the other way round). */
typedef int(BMX_CDECL *ZrwUpdateFn)(void *arg);
struct ZrwUpdateNode *zrwRegisterUpdateFunction(int type, ZrwUpdateFn fn, void *arg, int append);
void zrwRunUpdateList(int type);
void zrwShowRaster(int vsync);
void zrwDbgPrintDirect(const char *text, int x, int y);

typedef void *(BMX_CDECL *ZrwForAllFn)(void *item, void *arg);
ORIG_FN(0x00492eb0, ZrwCamera *, zrwGetCurrentCamera, (void));
ORIG_FN(0x00492f00, int, zrwcamBegin, (void *camera));
ORIG_FN(0x00492fe0, int, zrwcamEnd, (void));
ORIG_FN(0x00493060, void *, zrwCameraGetRwCamera, (const ZrwCamera *camera));

/* Interface pointers in the exe's data. */
typedef void(BMX_CDECL *ZrwSetBoundsFn)(int mode, const float *bounds, int flags);
typedef void *(BMX_CDECL *ZrwSetHookFn)(void *hook);
typedef void(BMX_CDECL *ZrwReleaseFn)(void *object);
typedef const char *(BMX_CDECL *ZrwTextureNameFn)(void *texture);
#define gZrwWorldSetBounds  ORIG_VAR(0x00561484, ZrwSetBoundsFn, gZrwWorldSetBounds)
#define gZrwWorldSetHook    ORIG_VAR(0x00561044, ZrwSetHookFn, gZrwWorldSetHook)
#define gZrwWorldTexDrop    ORIG_VAR(0x00562400, ZrwReleaseFn, gZrwWorldTexDrop)
#define gZrwWorldMatRelease ORIG_VAR(0x00561024, ZrwReleaseFn, gZrwWorldMatRelease)
#define gZrwWorldTexName    ORIG_VAR(0x00562410, ZrwTextureNameFn, gZrwWorldTexName)
#define gZrwWorldEffectOps  ORIG_VAR(0x00562098, uint32_t, gZrwWorldEffectOps)
#define gZrwWorldPathArgs   ORIG_VAR(0x00562128, uint32_t, gZrwWorldPathArgs)
typedef const char *ZrwLightKindTable[7];
#define gZrwLightKinds    ORIG_VAR(0x0056210c, ZrwLightKindTable, gZrwLightKinds)
#define gZrwClearColorRec ORIG_VAR(0x00697730, uint32_t, gZrwClearColorRec)
#define gZrwWorldName     ORIG_VAR(0x00674208, char, gZrwWorldName)
#define kSeparator        "--------------------------------------------------------------\n"

/* More interface pointers and data the moved functions use. */
typedef uint32_t(BMX_CDECL *ZrwTimeFn)(void);
#define gZrwTimeGetTime     ORIG_VAR(0x00561480, ZrwTimeFn, gZrwTimeGetTime)
#define gZrwSetBounds       gZrwWorldSetBounds
#define gZrwMaterialRelease gZrwWorldMatRelease
#define gZrwTextureRelease  gZrwWorldTexDrop
/* Where the PVS data is read from: a 0 terminated list of file names and the buffer it fills. */
#define gZrwPvsKeys   ORIG_VAR(0x0056212c, const char *, gZrwPvsKeys)
#define gZrwPvsBuffer ORIG_VAR(0x00674208, uint8_t, gZrwPvsBuffer)
/* Defaults of the particle steps: gravity, and floor height with bounce factor. */
#define gZrwDefaultGravity ORIG_VAR(0x005623c0, ZVec3, gZrwDefaultGravity)
typedef float ZrwBounceParams[2];
#define gZrwDefaultBounce ORIG_VAR(0x005623d0, ZrwBounceParams, gZrwDefaultBounce)
/* Address only: the PVS progress callbacks RenderWare is given. */
ORIG_FN(0x004cd510, int, rwWorldPvsDefaultCallback, (int stage, float progress));

static ZrwEffectHost *effectHost(void *object)
{
    return ((ZObj *)object)->data;
}

BMX_ADDR(0x00495bd0)
int zrwworldUpdateEffects(void *object)
{
    ZList *items = effectHost(object)->items;
    ZrwEffectItem *item = zlistFirst(items);

    while (item != NULL) {
        ZrwEffectType *type = item->type;

        type->update(type, item, 0);
        item = zlistNext(effectHost(object)->items);
    }
    return 1;
}

BMX_ADDR(0x00495c10)
void *zrwworldCreateObject(void)
{
    ZObj *object = zobjAlloc();
    ZrwEffectHost *host;

    if (object == NULL) {
        return NULL;
    }
    zobjInit(object, 2);
    object->ops = (const ZObjOps *)&gZrwWorldEffectOps;
    host = zmemAlloc(sizeof *host);
    object->data = host;
    host->items = zlistCreate(4);
    effectHost(object)->updateNode = zrwRegisterUpdateFunction(
        2, BMX_FNPTR(ZrwUpdateFn, 0x00495bd0, (ZrwUpdateFn)zrwworldUpdateEffects), object, 1);
    effectHost(object)->pad18[0] = 0;
    effectHost(object)->pad18[1] = 0;
    effectHost(object)->pad18[2] = 0;
    effectHost(object)->one24 = 1.0f;
    effectHost(object)->pad08[0] = 0;
    effectHost(object)->pad08[1] = 0;
    effectHost(object)->pad08[2] = 0;
    effectHost(object)->one28 = 1.0f;
    effectHost(object)->one2c = 1.0f;
    effectHost(object)->one30 = 1.0f;
    effectHost(object)->active = 1;
    return object;
}

BMX_ADDR(0x00495d50)
uint32_t zrwworldApplyEffect(void *object, const char *name, uint32_t flags, void *a, void *b)
{
    ZrwEffectType *type = zstrNameTable0Find(name);
    ZList *items;
    ZrwEffectItem *item;

    if (type == NULL) {
        return 0;
    }
    items = effectHost(object)->items;
    item = zlistFirst(items);
    while (item != NULL) {
        if (item->type == type) {
            break;
        }
        item = zlistNext(items);
    }
    if (item == NULL) {
        item = type->create(type, 0x100);
        zlistInsert(items, (void *)item);
        if (item == NULL) {
            return 0;
        }
    }
    type->apply(type, object, (void *)item, flags, a, b);
    return flags & 0xff;
}

BMX_ADDR(0x00495df0)
void *zrwworldCountSector(void *sector)
{
    gZrwWorldStats.polygons += (uint32_t)RpWorldSectorGetNumPolygons(sector);
    gZrwWorldStats.vertices += (uint32_t)RpWorldSectorGetNumVertices(sector);
    return sector;
}

BMX_ADDR(0x00495e30)
void *zrwworldCountAtomic(void *atomic)
{
    RpGeometry *geometry = ((const RpAtomic *)atomic)->geometry;

    gZrwWorldStats.clumpPolygons += (uint32_t)RpGeometryGetNumTriangles(geometry);
    gZrwWorldStats.clumpVertices += (uint32_t)RpGeometryGetNumVertices(geometry);
    return atomic;
}

BMX_ADDR(0x00495e70)
void *zrwworldCountClump(void *clump, void *arg)
{
    RpClumpForAllAtomics(clump, BMX_FNPTR(ZrwForAllFn, 0x00495e30, (ZrwForAllFn)zrwworldCountAtomic), arg);
    return clump;
}

BMX_ADDR(0x00495f60)
ZrwWorldStats *zrwworldLogInfo(void)
{
    int count;

    gZdebugPrint(kSeparator);
    gZdebugPrint("- SCENE INFO FOR:%s\n", &gZrwWorldName);
    gZdebugPrint(kSeparator);
    gZdebugIndent(1);
    gZrwWorldStats.lightIndex = 0;
    gZdebugPrint("- LIGHTS\n");
    gZdebugIndent(1);
    RpWorldForAllLights(gZrwWorld, BMX_FNPTR(ZrwForAllFn, 0x00496120, (ZrwForAllFn)zrwworldLogLight), NULL);
    gZdebugIndent(-1);
    gZdebugPrint("- MATERIALS\n");
    gZdebugIndent(1);
    count = RpWorldGetNumMaterials(gZrwWorld);
    for (int i = 0; i < count; i++) {
        void *material = RpWorldGetMaterial(gZrwWorld, i);

        if (material != NULL) {
            void *texture = RpMaterialGetTexture(material);

            if (texture != NULL) {
                gZdebugPrint("%3d: %s\n", i, RwTextureGetName(texture));
            }
        }
    }
    gZdebugIndent(-1);
    memset(&gZrwWorldStats, 0, sizeof gZrwWorldStats);
    RpWorldForAllWorldSectors(gZrwWorld, BMX_FNPTR(ZrwForAllFn, 0x00495df0, (ZrwForAllFn)zrwworldCountSector),
                              NULL);
    gZdebugPrint("- STATIC GEOM\n");
    gZdebugIndent(1);
    gZdebugPrint("Number of polygons:%d\n", gZrwWorldStats.polygons);
    gZdebugPrint("Number of vertices:%d\n", gZrwWorldStats.vertices);
    gZdebugIndent(-1);
    RpWorldForAllClumps(gZrwWorld, BMX_FNPTR(ZrwForAllFn, 0x00495e70, (ZrwForAllFn)zrwworldCountClump), NULL);
    gZdebugPrint("- CLUMPS (Props etc)\n");
    gZdebugIndent(1);
    gZdebugPrint("Number of clumps  :%d\n", RpWorldGetNumClumps(gZrwWorld));
    gZdebugPrint("Number of polygons:%d\n", gZrwWorldStats.clumpPolygons);
    gZdebugPrint("Number of vertices:%d\n", gZrwWorldStats.clumpVertices);
    gZdebugIndent(-1);
    gZdebugIndent(-1);
    gZdebugPrint(kSeparator);
    return &gZrwWorldStats;
}

BMX_ADDR(0x00496120)
void *zrwworldLogLight(void *light)
{
    uint32_t type = RpLightGetType(light);
    const float *color;
    uint32_t flags;

    if (type > 6) {
        type = 6;
    }
    gZdebugPrint("Light:%3d: %s\n", gZrwWorldStats.lightIndex, gZrwLightKinds[type]);
    gZdebugIndent(1);
    color = RpLightGetColor(light);
    gZdebugPrint("R:%f G:%f B:%f\n", (double)color[0], (double)color[1], (double)color[2]);
    flags = RpLightGetFlags(light);
    if (flags != 0) {
        if ((flags & 1) != 0) {
            gZdebugPrint("LightAtomics ");
        }
        if ((flags & 2) != 0) {
            gZdebugPrint("LightWorld ");
        }
        gZdebugPrint("\n");
    }
    /* The original also prints the radius and cone angle for types 0x80..0x82, which the clamp above
     * makes unreachable. */
    gZdebugIndent(-1);
    gZrwWorldStats.lightIndex++;
    return light;
}

BMX_ADDR(0x004962e0)
ZrwWorldRecord *zrwworldFindRecord(void *image, char kind, uint32_t *size)
{
    ZfileHandle *file = zfileOpenRamDisk((uint32_t)(uintptr_t)image, ((uint32_t *)image)[1]);
    void *iff;

    if (file != NULL) {
        iff = ziffOpen(file, 0x4e454353, 0);
        if (iff != NULL) {
            int found = ziffFindChunk(iff, 0x49464353);

            while (found != 0) {
                if (ziffChunkSize(iff) == 0x28) {
                    ziffReadChunk(iff, &gZrwWorldRecord);
                    if ((char)gZrwWorldRecord.kind == kind) {
                        ziffClose(iff);
                        zfileClose(file, 0);
                        if (size != NULL) {
                            *size = gZrwWorldRecord.size;
                        }
                        return &gZrwWorldRecord;
                    }
                } else {
                    zdebugFail("Chunk out of sync", kZrwWorldFile, 0x1d0, 0);
                }
                found = ziffNextChunk(iff, 0x49464353);
            }
            ziffClose(iff);
        }
        zfileClose(file, 0);
    }
    return NULL;
}

BMX_ADDR(0x004963c0)
int zrwworldAddMaterial(void *material)
{
    zrwRunUpdateList(6);
    zlistInsert(gZrwWorldMaterials, material);
    return 1;
}

BMX_ADDR(0x00496620)
int zrwWorldPvsCallback(int stage, float progress)
{
    if (stage == 0xc) {
        ZrwCamera *camera = zrwGetCurrentCamera();

        if (camera != NULL) {
            RwCameraClear(zrwCameraGetRwCamera(camera), &gZrwClearColorRec, 1);
            if (zrwcamBegin(camera) != 0) {
                zrwDbgPrintDirect(zstrFormatTemp("Percent complete: %f\n", (double)progress), 0, 0);
                if (zrwcamEnd() == 0) {
                    return 1;
                }
            }
            zrwShowRaster(1);
            gZdebugPrint("Percent complete: %f\n", (double)progress);
        }
    }
    return 1;
}

/* Centre, scaled centre and bounds of a new world; hands the bounds to the renderer. */
static void setWorldBounds(void *world)
{
    const float *origin = (const float *)RpWorldGetOrigin(world);
    const float *extent;
    float bounds[8];

    gZrwWorldCenter.x = origin[0];
    gZrwWorldCenter.y = origin[1];
    gZrwWorldCenterScaled.x = (float)((double)gZrwInvScale * gZrwWorldCenter.x);
    gZrwWorldCenter.z = origin[2];
    gZrwWorldCenterScaled.y = (float)((double)gZrwWorldCenter.y * gZrwInvScale);
    gZrwWorldCenterScaled.z = (float)((double)gZrwWorldCenter.z * gZrwInvScale);
    extent = (const float *)RpWorldGetBBox(gZrwWorld);
    bounds[0] = (float)(((double)extent[3] - gZrwWorldCenter.x) * gZrwInvScale);
    bounds[1] = (float)-(((double)extent[1] - gZrwWorldCenter.y) * gZrwInvScale);
    bounds[2] = (float)-(((double)extent[2] - gZrwWorldCenter.z) * gZrwInvScale);
    bounds[4] = (float)(((double)extent[0] - gZrwWorldCenter.x) * gZrwInvScale);
    bounds[5] = (float)-(((double)extent[4] - gZrwWorldCenter.y) * gZrwInvScale);
    bounds[6] = (float)-(((double)extent[5] - gZrwWorldCenter.z) * gZrwInvScale);
    gZrwWorldSetBounds(2, bounds, 0);
}

BMX_ADDR(0x004966e0)
int zrwworldReadStream(void *image, const char *name)
{
    uint8_t *record = gZrwWorldFind(image, 4, NULL);
    char path[128];
    const char *argument;
    const char *resolved;
    void *stream;
    void *world;

    if (record == NULL) {
        return 0;
    }
    zstrFormat(path, "%s%s", name, record + 8);
    argument = path;
    resolved = (const char *)(uintptr_t)zfileDeviceControl(
        zfileDefaultDevice(), 1, (uint32_t)(uintptr_t)&gZrwWorldPathArgs, (uint32_t)(uintptr_t)&argument);
    if (resolved == NULL) {
        return 0;
    }
    zstrcpy(&gZrwWorldName, resolved);
    stream = RwStreamOpen(2, 1, resolved);
    if (stream == NULL) {
        return 0;
    }
    if (RwStreamFindChunk(stream, 0xb, NULL, NULL) != 0) {
        world = RpWorldStreamRead(stream);
        RwStreamClose(stream, NULL);
        if (world != NULL) {
            if (gZrwWorld != NULL) {
                RpWorldDestroy(gZrwWorld);
            }
            gZrwWorld = world;
            setWorldBounds(world);
            if (RpPVSQuery(gZrwWorld) != 0) {
                gZrwPvsLoaded = 1;
                gZrwPvsEnabled = 1;
            }
            zrwworldLogInfo();
            return 1;
        }
    }
    RwStreamClose(stream, NULL);
    return 0;
}

BMX_ADDR(0x00496c40)
int zrwCreateWorld(void)
{
    float box[6] = {100.0f, 100.0f, 100.0f, -100.0f, -100.0f, -100.0f};
    void *world;

    if (gZrwWorldReady != 0) {
        zdebugFail("Already initialized", kZrwWorldFile, 0x348, 0);
    }
    memset(&gZrwWorldReady, 0, 0xac);
    gZrwWorldMaterials = zlistCreate(0x100);
    world = RpWorldCreate((const RwBBox *)box);
    if (world == NULL) {
        return 0;
    }
    gZrwWorld = world;
    setWorldBounds(world);
    RwResourcesSetArenaSize(0x200000);
    gZrwWorldReady = 1;
    return 1;
}

BMX_ADDR(0x00496de0)
int zrwWorldShutdown(void)
{
    float box[6] = {100.0f, 100.0f, 100.0f, -100.0f, -100.0f, -100.0f};

    if (gZrwWorldReady != 0) {
        if (gZrwWorldActive != 0) {
            void *world;

            if (RpPVSQuery(gZrwWorld) != 0) {
                RpPVSUnload(gZrwWorld);
                gZrwPvsLoaded = 0;
                gZrwPvsEnabled = 0;
            }
            if (gZrwWorld != NULL) {
                RpWorldDestroy(gZrwWorld);
                gZrwWorld = NULL;
            }
            world = RpWorldCreate((const RwBBox *)box);
            if (world != NULL) {
                gZrwWorld = world;
                setWorldBounds(world);
                if (gZrwWorldMaterials != NULL) {
                    for (void *material = zlistFirst(gZrwWorldMaterials); material != NULL;
                         material = zlistFirst(gZrwWorldMaterials)) {
                        gZrwWorldTexDrop(gZrwMaterialTexture(material, 0));
                        gZrwWorldMatRelease(material);
                        zlistRemoveCurrent(gZrwWorldMaterials);
                    }
                }
                gZrwWorldActive = 0;
            }
        }
        zlistDestroy(gZrwWorldMaterials);
        gZrwWorldReady = 0;
    }
    return 1;
}

BMX_ADDR(0x00496ff0)
int zrwworldLoad(void *image, const char *name)
{
    uint32_t size;
    uint8_t *record;
    char path[128];
    void *material;

    if (gZrwWorldActive != 0) {
        zdebugFail("There is already a world open", kZrwWorldFile, 899, 0);
        return 0;
    }
    record = gZrwWorldFind(image, 1, &size);
    if (record == NULL) {
        return 0;
    }
    if (size != 0) {
        ZfileHandle *file;
        void *previous;
        void *iff;

        zstrFormat(path, "%s%s", name, record + 8);
        file = zfileOpen(path, 1, 0);
        if (file == NULL) {
            zdebugFail(zstrFormatTemp("Couldn't open:%s", path), kZrwWorldFile, 0x221, 0);
            return 0;
        }
        previous = gZrwWorldSetHook(BMX_FNPTR(void *, 0x004963c0, zrwworldAddMaterial));
        iff = ziffOpen(file, 0xffffffffu, size);
        if (iff != NULL) {
            ziffProcessChunks(iff, NULL);
            ziffClose(iff);
        }
        gZrwWorldSetHook(previous);
        zfileClose(file, 0);
    }
    if (zrwworldReadStream(image, name) == 0) {
        return 0;
    }
    if (gZrwWorldMaterials != NULL) {
        material = zlistFirst(gZrwWorldMaterials);
        while (material != NULL) {
            void *texture = gZrwMaterialTexture(material, 0);
            void *rwTexture = zrwTextureGetRwTexture(texture);
            const char *textureName = gZrwWorldTexName(texture);
            int count = RpWorldGetNumMaterials(gZrwWorld);
            int found = 0;

            for (int i = count - 1; i >= 0; i--) {
                void *rwMaterial = RpWorldGetMaterial(gZrwWorld, i);

                if (rwMaterial != NULL && RpMaterialGetTexture(rwMaterial) == rwTexture) {
                    zrwmatSetTexture(material, rwMaterial);
                    found = 1;
                    break;
                }
            }
            if (!found) {
                gZdebugPrint("zrwWorldHookRpMaterials: Failed to match RpMaterial:%s\n", textureName);
            }
            material = zlistNext(gZrwWorldMaterials);
        }
    }
    gZrwWorldActive = 1;
    return 1;
}

/* ---- world reset, PVS, material list and particle steps (0x497200..0x497700, filed under zrwTxr.c by the
 * packet; they continue zrwWorld.c) --------------------------------------------------------------- */

BMX_ADDR(0x00497200)
int zrwWorldReset(void)
{
    float box[6] = {100.0f, 100.0f, 100.0f, -100.0f, -100.0f, -100.0f};
    float bounds[8];
    const float *origin, *extent;
    void *world;

    if (gZrwWorldActive == 0) {
        return 1;
    }
    if (RpPVSQuery(gZrwWorld) != 0) {
        RpPVSUnload(gZrwWorld);
        gZrwPvsLoaded = 0;
        gZrwPvsEnabled = 0;
    }
    if (gZrwWorld != NULL) {
        RpWorldDestroy(gZrwWorld);
        gZrwWorld = NULL;
    }
    world = RpWorldCreate((const RwBBox *)box);
    if (world == NULL) {
        return 0;
    }
    gZrwWorld = world;
    origin = (const float *)RpWorldGetOrigin(world);
    gZrwWorldCenter.x = origin[0];
    gZrwWorldCenter.y = origin[1];
    gZrwWorldCenterScaled.x = (float)((double)gZrwInvScale * gZrwWorldCenter.x);
    gZrwWorldCenter.z = origin[2];
    gZrwWorldCenterScaled.y = (float)((double)gZrwWorldCenter.y * gZrwInvScale);
    gZrwWorldCenterScaled.z = (float)((double)gZrwWorldCenter.z * gZrwInvScale);
    extent = (const float *)RpWorldGetBBox(gZrwWorld);
    bounds[0] = (float)(((double)extent[3] - gZrwWorldCenter.x) * gZrwInvScale);
    bounds[1] = (float)-(((double)extent[1] - gZrwWorldCenter.y) * gZrwInvScale);
    bounds[2] = (float)-(((double)extent[2] - gZrwWorldCenter.z) * gZrwInvScale);
    bounds[4] = (float)(((double)extent[0] - gZrwWorldCenter.x) * gZrwInvScale);
    bounds[5] = (float)-(((double)extent[4] - gZrwWorldCenter.y) * gZrwInvScale);
    bounds[6] = (float)-(((double)extent[5] - gZrwWorldCenter.z) * gZrwInvScale);
    gZrwSetBounds(2, bounds, 0);
    if (gZrwWorldMaterials != NULL) {
        for (void *material = zlistFirst(gZrwWorldMaterials); material != NULL;
             material = zlistFirst(gZrwWorldMaterials)) {
            gZrwTextureRelease(gZrwMaterialTexture(material, 0));
            gZrwMaterialRelease(material);
            zlistRemoveCurrent(gZrwWorldMaterials);
        }
    }
    gZrwWorldActive = 0;
    return 1;
}

BMX_ADDR(0x00497400)
void *zrwWorldFindMaterial(int hash)
{
    for (void *material = zlistFirst(gZrwWorldMaterials); material != NULL;
         material = zlistNext(gZrwWorldMaterials)) {
        if (hash == gZrwTextureHash(gZrwMaterialTexture(material, 0))) {
            return material;
        }
    }
    return NULL;
}

BMX_ADDR(0x00497450)
int zrwWorldMaterialCount(void)
{
    return zlistCount(gZrwWorldMaterials);
}

BMX_ADDR(0x00497460)
void *zrwWorldGetMaterial(int index)
{
    return zlistGet(gZrwWorldMaterials, index);
}

BMX_ADDR(0x00497480)
int zrwWorldHasPvs(void)
{
    return RpPVSQuery(gZrwWorld) != 0;
}

/* Rebuilds the world's PVS for the size of the current camera. Returns 0 without a camera. */
BMX_ADDR(0x004974a0)
int zrwWorldSetupPvs(void)
{
    ZrwCamera *camera = zrwGetCurrentCamera();
    void *rwCamera;
    RwRaster *raster;
    RwRaster *zRaster;
    float farClip;

    if (camera == NULL) {
        return 0;
    }
    rwCamera = zrwCameraGetRwCamera(camera);
    /* The original fetches both rasters first, before the PVS calls. */
    raster = RwCameraGetRaster(rwCamera);
    zRaster = RwCameraGetZRaster(rwCamera);
    farClip = camera->farClip;
    gZrwWorldPvsState = rpPVSCountSectors(gZrwWorld);
    RpPVSSetProgressCallBack(
        gZrwWorld, BMX_FNPTR(RpPVSProgressCallBack, 0x00496620, (RpPVSProgressCallBack)zrwWorldPvsCallback));
    if (RpPVSQuery(gZrwWorld) != 0) {
        RpPVSUnload(gZrwWorld);
        gZrwPvsLoaded = 0;
        gZrwPvsEnabled = 0;
    }
    if (RpPVSCreate(gZrwWorld, raster, zRaster, farClip, 0, (void *)(uintptr_t)rwWorldPvsDefaultCallback,
                    NULL) != NULL) {
        gZrwPvsLoaded = 1;
        gZrwPvsEnabled = 1;
    }
    return 1;
}

BMX_ADDR(0x00497570)
int zrwWorldSetPvsEnabled(int enabled)
{
    if (gZrwPvsLoaded != 0) {
        gZrwPvsEnabled = enabled;
        return 1;
    }
    if (enabled == 0) {
        gZrwPvsEnabled = 0;
        return 1;
    }
    gZdebugPrint("ZRWWORLD: No PVS data\n");
    return 0;
}

BMX_ADDR(0x004975b0)
int zrwWorldLoadPvs(void)
{
    uint8_t *values[1] = {&gZrwPvsBuffer};
    uint32_t data = zfileDeviceControl(zfileDefaultDevice(), 1, (uint32_t)(uintptr_t)&gZrwPvsKeys,
                                       (uint32_t)(uintptr_t)values);
    void *stream;
    int ok;

    if (data == 0) {
        return 0;
    }
    stream = RwStreamOpen(2, 2, (void *)(uintptr_t)data);
    if (stream == NULL) {
        return 0;
    }
    ok = RpWorldStreamWrite(gZrwWorld, stream) != NULL;
    RwStreamClose(stream, NULL);
    return ok != 0;
}

BMX_ADDR(0x00497620)
int zrwWorldRender(void)
{
    if (gZrwWorld != NULL) {
        RpWorldRender(gZrwWorld);
    }
    return 1;
}

BMX_ADDR(0x00497640)
void zrwWorldNop(void) {}

/* The particle steps keep the original signatures: `system` is not const although they only read it. */
// NOLINTBEGIN(readability-non-const-parameter)
/* Moves every particle's velocity by the acceleration times the time since the last step (which is not
 * updated here). The timestamp is read before `system` is checked. */
BMX_ADDR(0x00497650)
void zrwParticleAccelerate(int enabled, uint32_t *system, const ZVec3 *acceleration)
{
    const uint32_t now = gZrwTimeGetTime();
    const double elapsed = (double)(uint32_t)(now - ((const ZrwParticleSystem *)system)->lastStep);
    ZVec3 delta;
    const ZVec3 *accel = acceleration != NULL ? acceleration : &gZrwDefaultGravity;

    delta.x = (float)(elapsed * accel->x * 0x1.0624dep-10f);
    delta.y = (float)(elapsed * accel->y * 0x1.0624dep-10f);
    delta.z = (float)(elapsed * accel->z * 0x1.0624dep-10f);
    if (enabled != 0 && system != NULL) {
        ZList *list = ((ZrwParticleSystem *)system)->particles;

        for (ZrwParticle *particle = zlistFirst(list); particle != NULL; particle = zlistNext(list)) {
            zvec3Add(&particle->velocity, &particle->velocity, &delta);
        }
    }
}

BMX_ADDR(0x004976f0)
void zrwParticleNop(void) {}

/* Particles whose y reaches the floor value are set onto it and have their y velocity reflected. */
BMX_ADDR(0x00497700)
void zrwParticleFloorBounce(int enabled, uint32_t *system, const float *params)
{
    const float *p = params != NULL ? params : gZrwDefaultBounce;
    const float floor = p[0];
    const float bounce = p[1];

    if (enabled != 0 && system != NULL) {
        ZList *list = ((ZrwParticleSystem *)system)->particles;

        for (ZrwParticle *particle = zlistFirst(list); particle != NULL; particle = zlistNext(list)) {
            float *y = &particle->position.y;

            if ((double)*y >= (double)floor) {
                float *velocity = &particle->velocity.y;

                *y = floor;
                *velocity = (float)-((double)bounce * *velocity);
            }
        }
    }
}
// NOLINTEND(readability-non-const-parameter)
