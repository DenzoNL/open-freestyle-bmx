/* Renderer model wrappers and materials (see zrwmat.h). */
#include "zge/com/zglrw/zrwmat.h"

#include "compat/crt.h"
#include "rw/rwworld.h"
#include "zge/com/zglrw/zrwmodel.h"
#include "zge/com/zglrw/zrwtxr.h"
#include "zge/zdebug.h"
#include "zge/ziff.h"
#include "zge/zmem.h"
#include "zge/zobj.h"

static const char kZrwMatFile[] = "C:\\NgEngine\\Zge\\Com\\ZglRw\\zrwMat.c";

/* The renderer's model table (pointers in the exe's data, targets in zrwmodel.c / zrwmodelcreate.c). */
typedef void *(BMX_CDECL *ZrwMatCallFn)(void *a);
typedef void *(BMX_CDECL *ZrwMatCall2Fn)(void *a, void *b);
typedef void *(BMX_CDECL *ZrwMatCall4Fn)(void *a, void *b, void *c, void *d);
typedef void *(BMX_CDECL *ZrwMatCall5Fn)(void *a, void *b, void *c, void *d, void *e);
typedef uint32_t(BMX_CDECL *ZrwMatCountFn)(void *model);
#define gZrwVtLoadModel   ORIG_VAR(0x00560e84, ZrwMatCall2Fn, gZrwVtLoadModel)
#define gZrwVtLoadChunk   ORIG_VAR(0x00560e8c, ZrwMatCall2Fn, gZrwVtLoadChunk)
#define gZrwVtInstance    ORIG_VAR(0x00560e90, ZrwMatCallFn, gZrwVtInstance)
#define gZrwVtPartCount   ORIG_VAR(0x00560ea8, ZrwMatCountFn, gZrwVtPartCount)
#define gZrwVtOwner       ORIG_VAR(0x00560edc, ZrwMatCall2Fn, gZrwVtOwner)
#define gZrwVtNodeHashes  ORIG_VAR(0x00560ec8, ZrwMatCall2Fn, gZrwVtNodeHashes)
#define gZrwVtFindNode    ORIG_VAR(0x00560eac, ZrwMatCall2Fn, gZrwVtFindNode)
#define gZrwVtNodeToWorld ORIG_VAR(0x00560eb0, ZrwMatCall5Fn, gZrwVtNodeToWorld)
#define gZrwVtNodeVector  ORIG_VAR(0x00560eb4, ZrwMatCall4Fn, gZrwVtNodeVector)
#define gZrwVtNodeMatrix  ORIG_VAR(0x00560eb8, ZrwMatCall4Fn, gZrwVtNodeMatrix)
#define gZrwVtSkeleton    ORIG_VAR(0x00560ecc, ZrwMatCall2Fn, gZrwVtSkeleton)
/* Object types of the two kinds of model (a single clump, or a skinned one). */
#define gZrwOpsSimple  ORIG_VAR(0x00560fd0, uint32_t, gZrwOpsSimple)
#define gZrwOpsSkinned ORIG_VAR(0x00560fa8, uint32_t, gZrwOpsSkinned)
/* Non-zero: models get flag 2 set (set by the viewer). */
#define gZrwModelFlag2 ORIG_VAR(0x0067230c, int, gZrwModelFlag2)

/* Texture handle interface of the renderer. */
typedef void(BMX_CDECL *ZrwMatTextureFn)(void *texture);
typedef void *(BMX_CDECL *ZrwMatLoadFn)(const void *data, uint32_t size, const char *name, int type,
                                        int zero);
#define gZrwTextureDrop   ORIG_VAR(0x00562400, ZrwMatTextureFn, gZrwTextureDrop)
#define gZrwTextureRetain ORIG_VAR(0x00562420, ZrwMatTextureFn, gZrwTextureRetain)
#define gZrwTextureLoad   ORIG_VAR(0x005623fc, ZrwMatLoadFn, gZrwTextureLoad)

typedef struct ZrwMatSearch {
    uint32_t hash;
    int skip;
    ZrwMaterial *found;
} ZrwMatSearch;

typedef void(BMX_CDECL *ZrwMatEachFn)(void *payload, void *arg);
typedef void *(BMX_CDECL *ZrwMatHookFn)(void *material, void *arg);

static void **textureSlot(ZrwMaterial *material, uint32_t index)
{
    return (void **)(material->slots + (size_t)index * 12);
}

BMX_ADDR(0x00488fb0)
void *zrwmatCreateObject(void *model)
{
    ZObj *object;

    if (model == NULL) {
        return NULL;
    }
    object = zobjAlloc();
    if (object == NULL) {
        return NULL;
    }
    zobjInit(object, 0);
    object->model = model;
    if (gZrwVtPartCount(model) < 2) {
        object->ops = (const ZObjOps *)&gZrwOpsSimple;
        gZrwVtSetFlags(object->model, 0, 1);
    } else {
        object->ops = (const ZObjOps *)&gZrwOpsSkinned;
        gZrwVtSetFlags(object->model, 1, 0);
        zrwmatBuildSkeleton(object);
    }
    gZrwVtOwner(object->model, object);
    if (gZrwModelFlag2 != 0) {
        gZrwVtSetFlags(object->model, 2, 0);
    }
    return object;
}

BMX_ADDR(0x00489070)
void *zrwmatLoadModel(void *source)
{
    return gZrwVtLoadModel(source, NULL);
}

BMX_ADDR(0x00489090)
void *zrwmatLoadChunk(void *iff)
{
    return gZrwVtLoadChunk(iff, NULL);
}

BMX_ADDR(0x004890b0)
void zrwmatReleaseModel(void *model)
{
    gZrwVtDestroy(model);
}

BMX_ADDR(0x004890c0)
void *zrwmatModelFromObject(void *model)
{
    return gZrwVtInstance(model);
}

BMX_ADDR(0x004890d0)
int zrwmatNodeCount(void *object)
{
    return (int)gZrwVtPartCount(((ZObj *)object)->model);
}

BMX_ADDR(0x004890f0)
void zrwmatGetNodeHashes(void *object, uint32_t *hashes)
{
    gZrwVtNodeHashes(((ZObj *)object)->model, hashes);
}

BMX_ADDR(0x00489110)
uint32_t zrwmatFindNode(void *object, uint32_t hash)
{
    return (uint32_t)(uintptr_t)gZrwVtFindNode(((ZObj *)object)->model, (void *)(uintptr_t)hash);
}

BMX_ADDR(0x00489150)
void zrwmatNodeToWorld(void *object, uint32_t node, const float *local, float *out, int flag)
{
    gZrwVtNodeToWorld(((ZObj *)object)->model, (void *)(uintptr_t)node, (void *)local, out,
                      (void *)(uintptr_t)flag);
}

BMX_ADDR(0x00489180)
void zrwmatNodeVector(void *object, uint32_t node, float *out, int flag)
{
    gZrwVtNodeVector(((ZObj *)object)->model, (void *)(uintptr_t)node, out, (void *)(uintptr_t)flag);
}

BMX_ADDR(0x004891b0)
void zrwmatNodeMatrix(void *object, uint32_t node, void *out, int flag)
{
    gZrwVtNodeMatrix(((ZObj *)object)->model, (void *)(uintptr_t)node, out, (void *)(uintptr_t)flag);
}

BMX_ADDR(0x00489210)
void zrwmatSetSkeleton(void *object, void *skeleton)
{
    ZObj *obj = object;

    *(void **)&obj->pad30[0] = skeleton;
    gZrwVtSkeleton(obj->model, skeleton);
}

BMX_ADDR(0x004892a0)
void zrwmatUpdate(void *object)
{
    gZrwVtUpdate(((ZObj *)object)->model);
}

/* "MATI" chunk: reads the material that the loader of the model system builds, then runs the hook. */
BMX_ADDR(0x004892b0)
int zrwmatReadMATI(void *iff, void *arg)
{
    ZrwMaterial *material;

    if (ziffBeginModel(iff) != 0) {
        gZrwMat.madf = NULL;
        gZrwMat.last = NULL;
        ziffProcessChunks(iff, NULL);
        if (gZrwMat.madf == NULL || gZrwMat.last == NULL) {
            gZdebugPrint("RwMat: problem loading image\n");
            ziffEndModel(iff);
            return 0;
        }
        gZrwMat.last->ambient[0] = gZrwMat.madf->flags;
        gZrwMat.last->ambient[1] = gZrwMat.madf->v10;
        gZrwMat.last->ambient[2] = gZrwMat.madf->v14;
        gZrwMat.last->color[0] = gZrwMat.madf->v40;
        gZrwMat.last->color[1] = gZrwMat.madf->v44;
        gZrwMat.last->color[2] = gZrwMat.madf->v48;
        gZrwMat.last->pad20 = 0;
        ziffEndModel(iff);
    }
    material = gZrwMat.last;
    if (gZrwMat.hook != NULL) {
        ((ZrwMatHookFn)gZrwMat.hook)(material, arg);
    }
    return (int)(uintptr_t)material;
}

/* "MADF" chunk: the definition (at most 0x8f bytes, starting with 0x48) of the material being read. */
BMX_ADDR(0x004893b0)
int zrwmatReadMADF(void *iff)
{
    if (ziffChunkSize(iff) > 0x8f) {
        zdebugFail("Data too big", kZrwMatFile, 0xf0, 0);
        return 0;
    }
    ziffReadChunk(iff, &gZrwMadfRec.magic);
    if (gZrwMadfRec.magic == 0x48) {
        gZrwMat.madf = &gZrwMadfRec;
        zrwTextureSetLoadName(gZrwMadfRec.name);
        return 1;
    }
    zdebugFail("MADF Mismatch", kZrwMatFile, 0xf6, 0);
    return 0;
}

BMX_ADDR(0x00489440)
int zrwmatInitialize(void)
{
    if (gZrwMat.initialized != 0) {
        zdebugFail("Already initialized", kZrwMatFile, 0x114, 0);
    }
    memset(&gZrwMat, 0, 0x24);
    gZrwMat.initialized = 1;
    gZrwMat.pool = zmemFreeListCreate(
        0x74, 0x100, BMX_FNPTR(ZMemDestroyFn, 0x00489530, (ZMemDestroyFn)zrwmatReleaseTexture));
    gZrwMat.pad04 = 0;
    ziffSetupCallback(
        0x4954414d, BMX_FNPTR(int(BMX_CDECL *)(void *), 0x004892b0, (int(BMX_CDECL *)(void *))zrwmatReadMATI),
        0);
    ziffSetupCallback(
        0x4644414d, BMX_FNPTR(int(BMX_CDECL *)(void *), 0x004893b0, (int(BMX_CDECL *)(void *))zrwmatReadMADF),
        0);
    return 1;
}

BMX_ADDR(0x004894d0)
int zrwmatShutdown(void)
{
    if (gZrwMat.initialized != 0) {
        zmemFreeListDestroy(gZrwMat.pool);
        gZrwMat.initialized = 0;
    }
    return 1;
}

/* A zeroed pool record in use by one reference. */
static ZrwMaterial *newMaterial(void)
{
    ZrwMaterial *material = zmemFreeListAlloc(gZrwMat.pool);

    memset(material, 0, sizeof *material);
    material->refCount = 1;
    material->flags = (material->flags & 0xffffff03u) | 3;
    return material;
}

/* Appends a texture, takes a reference on it and points the RpMaterial at it. */
static void appendTexture(ZrwMaterial *material, void *texture)
{
    if (material->textureCount > 5) {
        zdebugFail("Too many textures added", kZrwMatFile, 0x1a5, 0);
        return;
    }
    *textureSlot(material, material->textureCount) = texture;
    material->textureCount++;
    gZrwTextureRetain(texture);
    if (material->rwMaterial != NULL) {
        RpMaterialSetTexture(material->rwMaterial, zrwTextureGetRwTexture(texture));
    }
}

BMX_ADDR(0x00489500)
ZrwMaterial *zrwmatCreateRecord(void)
{
    return newMaterial();
}

BMX_ADDR(0x00489530)
void zrwmatReleaseTexture(ZrwMaterial *material)
{
    int i = 0;

    if (material->textureCount != 0) {
        void **slot = (void **)&material->slots[0];
        do {
            gZrwTextureDrop(*slot);
            i++;
            slot += 3;
        } while (i < (int)material->textureCount);
    }
    material->refCount--;
    if (material->refCount == 0) {
        material->textureCount = 0;
        zmemFreeListFree(gZrwMat.pool, material);
    }
}

BMX_ADDR(0x00489590)
void zrwmatReplaceTexture(ZrwMaterial *material, void *texture, uint32_t index)
{
    void **slot;

    if (index >= material->textureCount) {
        zdebugFail("No texture has been added for this render-pass", kZrwMatFile, 0x184, 0);
        return;
    }
    slot = textureSlot(material, index);
    if (*slot != NULL) {
        gZrwTextureDrop(*slot);
    }
    *slot = texture;
    gZrwTextureRetain(texture);
    if (material->rwMaterial != NULL) {
        RpMaterialSetTexture(material->rwMaterial, zrwTextureGetRwTexture(texture));
    }
}

BMX_ADDR(0x00489610)
void zrwmatAddTexture(ZrwMaterial *material, void *texture)
{
    appendTexture(material, texture);
}

BMX_ADDR(0x00489680)
void zrwmatSetColor(ZrwMaterial *material, void *unused, const void *color)
{
    (void)unused;
    if (material->rwMaterial == NULL) {
        zdebugFail("pMat->pRwMat not set", kZrwMatFile, 0x1c6, 0);
        return;
    }
    RpMaterialSetColor(material->rwMaterial, color);
}

BMX_ADDR(0x004896c0)
void *zrwmatSetMaterialHook(void *hook)
{
    void *previous = gZrwMat.hook;

    gZrwMat.hook = hook;
    return previous;
}

BMX_ADDR(0x004896d0)
ZrwMaterial *zrwmatLoadSpriteTexture(void *data, uint32_t size, const char *name, int type)
{
    void *texture;
    ZrwMaterial *material;

    gZrwMat.last = NULL;
    texture = gZrwTextureLoad(data, size, name, type, 0);
    if (texture == NULL) {
        gZdebugPrint("zrwMatCreateFromImage: Error loading:%s\n", name);
        return NULL;
    }
    material = newMaterial();
    appendTexture(material, texture);
    zrwTextureSetUserData(texture, (uint32_t)(uintptr_t)material);
    gZrwMat.last = material;
    gZrwTextureDrop(texture);
    return gZrwMat.last;
}

BMX_ADDR(0x004899a0)
int zrwmatMatchHash(ZrwMaterial *material, uint32_t *state)
{
    void *texture;
    int skip;

    if (material->textureCount == 0) {
        zdebugFail("Out of range", kZrwMatFile, 0x27b, 0);
        texture = NULL;
    } else {
        texture = *textureSlot(material, 0);
    }
    if ((uint32_t)gZrwTextureHash(texture) == state[0]) {
        skip = (int)state[1];
        state[1] = (uint32_t)(skip - 1);
        if (skip == 0) {
            ((ZrwMaterial **)state)[2] = material;
            return 0;
        }
    }
    return 1;
}

BMX_ADDR(0x00489a00)
ZrwMaterial *zrwmatFindByTextureHash(uint32_t hash, int skip)
{
    gZrwMat.findHash = hash;
    gZrwMat.findSkip = skip;
    gZrwMat.found = NULL;
    zmemFreeListForEach(gZrwMat.pool, BMX_FNPTR(ZrwMatEachFn, 0x004899a0, (ZrwMatEachFn)zrwmatMatchHash),
                        &gZrwMat.findHash);
    return gZrwMat.found;
}

BMX_ADDR(0x00489a40)
void *zrwmatGetTexture(ZrwMaterial *material, uint32_t index)
{
    if (index < material->textureCount) {
        return *textureSlot(material, index);
    }
    zdebugFail("Out of range", kZrwMatFile, 0x27b, 0);
    return NULL;
}

BMX_ADDR(0x00489a80)
ZrwMaterial *zrwmatClone(ZrwMaterial *material)
{
    ZrwMaterial *copy = newMaterial();
    int i;

    if (copy == NULL) {
        return NULL;
    }
    memcpy(copy, material, sizeof *copy);
    i = 0;
    if (material->textureCount != 0) {
        do {
            gZrwTextureRetain(gZrwMaterialTexture(copy, i));
            i++;
        } while (i < (int)material->textureCount);
    }
    return copy;
}

BMX_ADDR(0x00489af0)
ZrwMaterial *zrwmatRetainMaterial(ZrwMaterial *material)
{
    int i = 0;

    if (material->textureCount != 0) {
        do {
            gZrwTextureRetain(gZrwMaterialTexture(material, i));
            i++;
        } while (i < (int)material->textureCount);
    }
    material->refCount++;
    return material;
}

BMX_ADDR(0x00489b30)
uint32_t *zrwmatGetColor(ZrwMaterial *material)
{
    return material->color;
}

BMX_ADDR(0x00489b40)
void *zrwmatGetRaster(ZrwMaterial *material, uint32_t index)
{
    if (index < material->textureCount) {
        return zrwTextureGetRaster(*textureSlot(material, index));
    }
    zdebugFail("Out of range", kZrwMatFile, 0x2de, 0);
    return NULL;
}

BMX_ADDR(0x00489b80)
void zrwmatSetTexture(ZrwMaterial *material, void *rwMaterial)
{
    material->rwMaterial = rwMaterial;
}

BMX_ADDR(0x00489ba0)
int zrwmatTextureLoadBegin(void)
{
    if (gZrwMat.madf == NULL) {
        zdebugFail("Not loadedMADF", kZrwMatFile, 0x315, 0);
    }
    return (gZrwMat.madf->flags & 1) != 0;
}

BMX_ADDR(0x00489be0)
void zrwmatTextureLoaded(void *texture)
{
    ZrwMaterial *material = newMaterial();

    appendTexture(material, texture);
    zrwTextureSetUserData(texture, (uint32_t)(uintptr_t)material);
    gZrwMat.last = material;
}
