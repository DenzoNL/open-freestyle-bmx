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

/*
 * The renderer's model table (pointers in the exe's data, targets in zrwmodel.c / zrwmodelcreate.c), each
 * slot typed as its target. The loaders' NULL second argument, which the targets do not take, is left out.
 * The part calls pass the flag as the int word the original pushes (the oracle compares the pushed words; the
 * targets read its low byte), the part offset included, though zrwModelPartOffset takes no flag.
 */
typedef void *(BMX_CDECL *ZrwVtLoadFn)(void *source);                   /* zrwModelLoadFromFile / FromChunk */
typedef void *(BMX_CDECL *ZrwVtInstanceFn)(void *model);                /* zrwModelCreateInstance */
typedef int(BMX_CDECL *ZrwVtCountFn)(void *model);                      /* zrwModelPartCount */
typedef void(BMX_CDECL *ZrwVtHashesFn)(void *model, uint32_t *out);     /* zrwModelGetPartHashes */
typedef void *(BMX_CDECL *ZrwVtFindPartFn)(void *model, uint32_t hash); /* zrwModelFindPart */
typedef void(BMX_CDECL *ZrwVtPartToWorldFn)(void *model, void *part, const float *local, float *out,
                                            int flag); /* zrwModelPartToWorld */
typedef void(BMX_CDECL *ZrwVtPartOffsetFn)(void *model, void *part, float *out,
                                           int flag); /* zrwModelPartOffset */
typedef void(BMX_CDECL *ZrwVtPartMatrixFn)(void *model, void *part, void *out,
                                           int flag);          /* zrwModelPartMatrix */
typedef void(BMX_CDECL *ZrwVtSetFn)(void *model, void *value); /* zrwModelMatchParts, zrwModelSetUserValue */
#define gZrwVtLoadModel   ORIG_VAR(0x00560e84, ZrwVtLoadFn, gZrwVtLoadModel)
#define gZrwVtLoadChunk   ORIG_VAR(0x00560e8c, ZrwVtLoadFn, gZrwVtLoadChunk)
#define gZrwVtInstance    ORIG_VAR(0x00560e90, ZrwVtInstanceFn, gZrwVtInstance)
#define gZrwVtPartCount   ORIG_VAR(0x00560ea8, ZrwVtCountFn, gZrwVtPartCount)
#define gZrwVtOwner       ORIG_VAR(0x00560edc, ZrwVtSetFn, gZrwVtOwner)
#define gZrwVtNodeHashes  ORIG_VAR(0x00560ec8, ZrwVtHashesFn, gZrwVtNodeHashes)
#define gZrwVtFindNode    ORIG_VAR(0x00560eac, ZrwVtFindPartFn, gZrwVtFindNode)
#define gZrwVtNodeToWorld ORIG_VAR(0x00560eb0, ZrwVtPartToWorldFn, gZrwVtNodeToWorld)
#define gZrwVtNodeVector  ORIG_VAR(0x00560eb4, ZrwVtPartOffsetFn, gZrwVtNodeVector)
#define gZrwVtNodeMatrix  ORIG_VAR(0x00560eb8, ZrwVtPartMatrixFn, gZrwVtNodeMatrix)
#define gZrwVtSkeleton    ORIG_VAR(0x00560ecc, ZrwVtSetFn, gZrwVtSkeleton)
/* Object types of the two kinds of model (a single clump, or a skinned one). */
#define gZrwOpsSimple  ORIG_VAR(0x00560fd0, void *, gZrwOpsSimple)
#define gZrwOpsSkinned ORIG_VAR(0x00560fa8, void *, gZrwOpsSkinned)
/* Non-zero: models get flag 2 set (set by the viewer). */
#define gZrwModelFlag2 ORIG_VAR(0x0067230c, int, gZrwModelFlag2)

/* Texture handle interface of the renderer. */
typedef void(BMX_CDECL *ZrwMatTextureFn)(void *texture);
typedef void *(BMX_CDECL *ZrwMatLoadFn)(const void *data, uint32_t size, const char *name, int type,
                                        int zero);
#define gZrwTextureDrop   ORIG_VAR(0x00562400, ZrwMatTextureFn, gZrwTextureDrop)
#define gZrwTextureRetain ORIG_VAR(0x00562420, ZrwMatTextureFn, gZrwTextureRetain)
#define gZrwTextureLoad   ORIG_VAR(0x005623fc, ZrwMatLoadFn, gZrwTextureLoad)

typedef void(BMX_CDECL *ZrwMatEachFn)(void *payload, void *arg);
typedef void *(BMX_CDECL *ZrwMatHookFn)(void *material, void *arg);

/* Unchecked: callers test the index against textureCount (and that is at most six). */
static void **textureSlot(ZrwMaterial *material, uint32_t index)
{
    return &material->slots[index].texture;
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
    return gZrwVtLoadModel(source);
}

BMX_ADDR(0x00489090)
void *zrwmatLoadChunk(void *iff)
{
    return gZrwVtLoadChunk(iff);
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
void *zrwmatFindNode(void *object, uint32_t hash)
{
    return gZrwVtFindNode(((ZObj *)object)->model, hash);
}

BMX_ADDR(0x00489150)
void zrwmatNodeToWorld(void *object, void *node, const float *local, float *out, int flag)
{
    gZrwVtNodeToWorld(((ZObj *)object)->model, node, local, out, flag);
}

BMX_ADDR(0x00489180)
void zrwmatNodeVector(void *object, void *node, float *out, int flag)
{
    gZrwVtNodeVector(((ZObj *)object)->model, node, out, flag);
}

BMX_ADDR(0x004891b0)
void zrwmatNodeMatrix(void *object, void *node, void *out, int flag)
{
    gZrwVtNodeMatrix(((ZObj *)object)->model, node, out, flag);
}

BMX_ADDR(0x00489210)
void zrwmatSetSkeleton(void *object, void *skeleton)
{
    ZObj *obj = object;

    obj->data = skeleton;
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
    return bmxPtrResult(material);
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
    memset(&gZrwMat, 0, sizeof gZrwMat);
    gZrwMat.initialized = 1;
    gZrwMat.pool =
        zmemFreeListCreate(sizeof(ZrwMaterial), 0x100,
                           BMX_FNPTR(ZMemDestroyFn, 0x00489530, (ZMemDestroyFn)zrwmatReleaseTexture));
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
        ZrwMatSlot *slot = material->slots;
        do {
            gZrwTextureDrop(slot->texture);
            i++;
            slot++;
        } while (i < (int)material->textureCount);
    }
    material->refCount--;
    if (material->refCount == 0) {
#if !BMX_LAYOUT32
        bmxSlot32Drop(material->handle);
        material->handle = 0;
#endif
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
    zrwTextureSetUserData(texture, material);
    gZrwMat.last = material;
    gZrwTextureDrop(texture);
    return gZrwMat.last;
}

BMX_ADDR(0x004899a0)
int zrwmatMatchHash(ZrwMaterial *material, uint32_t *state)
{
    ZrwMatSearch *search = (ZrwMatSearch *)state;
    void *texture;
    int skip;

    if (material->textureCount == 0) {
        zdebugFail("Out of range", kZrwMatFile, 0x27b, 0);
        texture = NULL;
    } else {
        texture = *textureSlot(material, 0);
    }
    if ((uint32_t)gZrwTextureHash(texture) == search->hash) {
        skip = search->skip;
        search->skip = (int)((uint32_t)skip - 1u);
        if (skip == 0) {
            search->found = material;
            return 0;
        }
    }
    return 1;
}

BMX_ADDR(0x00489a00)
ZrwMaterial *zrwmatFindByTextureHash(uint32_t hash, int skip)
{
    gZrwMat.find.hash = hash;
    gZrwMat.find.skip = skip;
    gZrwMat.find.found = NULL;
    zmemFreeListForEach(gZrwMat.pool, BMX_FNPTR(ZrwMatEachFn, 0x004899a0, (ZrwMatEachFn)zrwmatMatchHash),
                        &gZrwMat.find);
    return gZrwMat.find.found;
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
    zrwTextureSetUserData(texture, material);
    gZrwMat.last = material;
}
