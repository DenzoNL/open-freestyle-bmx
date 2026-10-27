/* Renderer world and effect object (see zrwworld.h). */
#include "zge/com/zglrw/zrwworld.h"

#include "compat/crt.h"
#include "rw/rwcamera.h"
#include "rw/rwcore.h"
#include "rw/rwlight.h"
#include "rw/rwmath.h"
#include "rw/rwstream.h"
#include "rw/rwtexture.h"
#include "rw/rwworld.h"
#include "zge/com/zglrw/zrwmat.h"
#include "zge/com/zglrw/zrwtxr.h"
#include "zge/zdebug.h"
#include "zge/zfile.h"
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
ORIG_FN(0x004b4720, int, RpGeometryGetNumTriangles, (void *geometry));
ORIG_FN(0x004b4550, int, RpGeometryGetNumVertices, (void *geometry));
ORIG_FN(0x004bb080, void *, RpWorldCreate, (const float *bbox));
ORIG_FN(0x004c3e60, void *, RpWorldStreamRead, (void *stream));
ORIG_FN(0x00492eb0, uint8_t *, zrwGetCurrentCamera, (void));
ORIG_FN(0x00492f00, int, zrwcamBegin, (void *camera));
ORIG_FN(0x00492fe0, int, zrwcamEnd, (void));
ORIG_FN(0x00493060, void *, zrwCameraGetRwCamera, (const uint8_t *camera));

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

static ZrwEffectHost *effectHost(void *object)
{
    return *(ZrwEffectHost **)&((ZObj *)object)->pad30[0];
}

BMX_ADDR(0x00495bd0)
int zrwworldUpdateEffects(void *object)
{
    ZList *items = effectHost(object)->items;
    void *item = zlistFirst(items);

    while (item != NULL) {
        ZrwEffectType *type = ((ZrwEffectType **)item)[0x2c / 4];

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
    host = zmemAlloc(0x3c);
    *(ZrwEffectHost **)&object->pad30[0] = host;
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
    ZrwEffectType **item;

    if (type == NULL) {
        return 0;
    }
    items = effectHost(object)->items;
    item = (ZrwEffectType **)zlistFirst(items);
    while (item != NULL) {
        if (item[0x2c / 4] == type) {
            break;
        }
        item = (ZrwEffectType **)zlistNext(items);
    }
    if (item == NULL) {
        item = (ZrwEffectType **)type->create(type, 0x100);
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
    void *geometry = ((void **)atomic)[0x1c / 4];

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
        uint8_t *camera = zrwGetCurrentCamera();

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
    world = RpWorldCreate(box);
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
            world = RpWorldCreate(box);
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
