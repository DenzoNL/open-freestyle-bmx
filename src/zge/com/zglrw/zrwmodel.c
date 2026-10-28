/* Renderer models (C:\NgEngine\Zge\Com\ZglRw\zrwModel.c). */
#include "zge/com/zglrw/zrwmodel.h"

#include <stddef.h>

#include "compat/crt.h"
#include "rw/rwcamera.h"
#include "rw/rwclump.h"
#include "rw/rwcore.h"
#include "rw/rwmath.h"
#include "rw/rwplugins.h"
#include "rw/rwworld.h"
#include "zge/com/zglrw/zrwmodelrender.h"
#include "zge/zdebug.h"
#include "zge/ziff.h"
#include "zge/zlist.h"
#include "zge/zquat.h"

static const char kZrwModelFile[] = "C:\\NgEngine\\Zge\\Com\\ZglRw\\zrwModel.c";

/* RenderWare library calls (statically linked in the exe, not ported). */
typedef void *(BMX_CDECL *RpAtomicCallback)(void *atomic, void *data);

/* Other renderer modules (zrwmodelcreate.c, zrwmodelrender.c, zrwzgl.c, zobjlight.c, ...). */
int zrwModelCreateInitialize(void);
int zrwModelCreateShutdown(void);
void *zrwModelCreateFirstAtomic(void *clump);
ORIG_FN(0x0048be30, void, zrwMatrixToGame, (void *out, const ZMatrix *matrix));

/* Part sets are 12 bytes apart from +0x54; the index is a 16 bit field of the model, not range checked. */
static ZrwPartSet *partSet(const ZrwModel *model, uint32_t index)
{
    return (ZrwPartSet *)((uint8_t *)model + offsetof(ZrwModel, partSets)) + index;
}

BMX_ADDR(0x00487ad0)
ZrwModelEntry *zrwModelFindEntry(const ZrwModel *model, int key)
{
    const ZrwModelDef *def = model->def;

    for (int i = 0; i < def->entryCount; i++) {
        if (key == def->entries[i].key) {
            return &def->entries[i];
        }
    }
    return NULL;
}

BMX_ADDR(0x00487b60)
void *zrwAtomicMaskFlags(void *atomic, const uint32_t *mask)
{
    uint8_t *bytes = atomic;

    bytes[2] = (uint8_t)(((uint8_t)mask[0] & bytes[2]) | (uint8_t)mask[1]);
    bytes[0x14]++;
    return atomic;
}

BMX_ADDR(0x00487b90)
void zrwClumpMaskAtomicFlags(void *clump, uint32_t clear, uint32_t set)
{
    uint32_t mask[2] = {~clear, set};

    RpClumpForAllAtomics(clump, BMX_FNPTR(RpAtomicCallback, 0x00487b60, zrwAtomicMaskFlags), mask);
}

BMX_ADDR(0x00487bc0)
void *zrwAtomicMaskGeometryFlags(void *atomic, const uint32_t *mask)
{
    void *geometry = *(void **)((uint8_t *)atomic + 0x1c);
    const uint32_t flags = RpGeometryGetFlags(geometry);

    RpGeometrySetFlags(geometry, (mask[0] & flags) | mask[1]);
    return atomic;
}

BMX_ADDR(0x00487bf0)
void zrwClumpMaskGeometryFlags(void *clump, uint32_t clear, uint32_t set)
{
    uint32_t mask[2] = {~clear, set};

    RpClumpForAllAtomics(clump, BMX_FNPTR(RpAtomicCallback, 0x00487bc0, zrwAtomicMaskGeometryFlags), mask);
}

/* Masks the atomic flags of a clump with the given (keep, set) words. */
static void maskAtomics(void *clump, uint32_t keep, uint32_t set)
{
    uint32_t mask[2] = {keep, set};

    RpClumpForAllAtomics(clump, BMX_FNPTR(RpAtomicCallback, 0x00487b60, zrwAtomicMaskFlags), mask);
}

static void worldRemove(const ZrwModel *model)
{
    if (RpClumpGetWorld(model->clump) != NULL) {
        RpWorldRemoveClump(gZrwWorld, model->clump);
    }
    if (model->clump2 != NULL) {
        if (RpClumpGetWorld(model->clump2) != NULL) {
            RpWorldRemoveClump(gZrwWorld, model->clump2);
        }
    }
}

static void worldAdd(const ZrwModel *model)
{
    if (RpClumpGetWorld(model->clump) == NULL) {
        RpWorldAddClump(gZrwWorld, model->clump);
    }
    if (model->clump2 != NULL) {
        if (RpClumpGetWorld(model->clump2) == NULL) {
            RpWorldAddClump(gZrwWorld, model->clump2);
        }
    }
}

BMX_ADDR(0x00487c20)
void zrwModelSetInWorld(const ZrwModel *model, int add)
{
    if (add == 0) {
        worldRemove(model);
    } else {
        worldAdd(model);
    }
}

BMX_ADDR(0x00487cd0)
void zrwModelSetActive(const ZrwModel *model, int on)
{
    if (on == 0) {
        if (model->clump2 == NULL) {
            maskAtomics(model->clump, 0xfffffffe, 0);
        } else if (RpClumpGetWorld(model->clump2) != NULL) {
            RpWorldRemoveClump(gZrwWorld, model->clump2);
        }
    } else {
        if (model->clump2 == NULL) {
            maskAtomics(model->clump, 0xffffffff, 1);
        } else if (RpClumpGetWorld(model->clump2) == NULL) {
            RpWorldAddClump(gZrwWorld, model->clump2);
        }
    }
}

BMX_ADDR(0x00487d90)
void *zrwMdlConstructor(void *clump)
{
    if (gZrwMdlPluginOffset > 0) {
        *(uint32_t *)((uint8_t *)clump + gZrwMdlPluginOffset) = 0;
    }
    return clump; /* the plugin registry treats NULL as failure */
}

BMX_ADDR(0x00487db0)
void *zrwMdlDestructor(void *clump)
{
    return clump;
}

BMX_ADDR(0x00487dc0)
int zrwMdlSetUserData(void *clump, uint32_t value)
{
    if (gZrwMdlPluginOffset < 1) {
        return 0;
    }
    *(uint32_t *)((uint8_t *)clump + gZrwMdlPluginOffset) = value;
    return 1;
}

/* Locals shared by the two matrix builders: the quaternion (x, -y, -z, w), normalised, and its products. */
typedef struct QuatTerms {
    ZQuat q;
    double s;
    float xs, ys, xx, xy, yy, yz, xw, yw, zw, xz;
    double zs, zz;
} QuatTerms;

/* s = 2 / |q| (0 for a zero or NaN length); xs and ys are stored as floats, zs and zz stay in registers. */
static void quatTerms(const ZQuat *quat, QuatTerms *t)
{
    float len;

    t->q.x = quat->x;
    t->q.y = -quat->y;
    t->q.z = -quat->z;
    t->q.w = quat->w;
    len = zquatNormalize(&t->q);
    t->s = (double)len > 0.0 ? 2.0 / (double)len : 0.0;
    t->xs = (float)(t->s * t->q.x);
    t->ys = (float)(t->s * t->q.y);
    t->zs = t->s * t->q.z;
    t->xw = (float)((double)t->xs * t->q.w);
    t->yw = (float)((double)t->ys * t->q.w);
    t->zw = (float)(t->zs * t->q.w);
    t->xx = (float)((double)t->xs * t->q.x);
    t->xy = (float)((double)t->ys * t->q.x);
    t->xz = (float)(t->zs * t->q.x);
    t->yy = (float)((double)t->ys * t->q.y);
    t->yz = (float)(t->zs * t->q.y);
    t->zz = t->zs * t->q.z;
}

BMX_ADDR(0x00487e70)
void zrwQuatToMatrix(const ZQuat *quat, ZMatrix *out)
{
    QuatTerms t;
    float *m = (float *)out;

    quatTerms(quat, &t);
    m[0] = (float)(1.0 - (t.zz + (double)t.yy));
    m[1] = (float)((double)t.xy + t.zw);
    m[2] = (float)((double)t.xz - t.yw);
    m[4] = (float)((double)t.xy - t.zw);
    m[5] = (float)(1.0 - (t.zz + (double)t.xx));
    m[6] = (float)((double)t.yz + t.xw);
    m[8] = (float)((double)t.xz + t.yw);
    m[9] = (float)((double)t.yz - t.xw);
    m[10] = (float)(1.0 - ((double)t.yy + (double)t.xx));
    RwMatrixUpdate(out);
}

BMX_ADDR(0x00487fc0)
void zrwQuatToMatrixMirrored(const ZQuat *quat, ZMatrix *out)
{
    QuatTerms t;
    float *m = (float *)out;

    quatTerms(quat, &t);
    m[0] = (float)(1.0 - (t.zz + (double)t.yy));
    m[1] = (float)(-((double)t.xy + t.zw));
    m[2] = (float)(-((double)t.xz - t.yw));
    m[4] = (float)(-((double)t.xy - t.zw));
    m[5] = (float)(1.0 - (t.zz + (double)t.xx));
    m[6] = (float)((double)t.yz + t.xw);
    m[8] = (float)(-((double)t.xz + t.yw));
    m[9] = (float)((double)t.yz - t.xw);
    m[10] = (float)(1.0 - ((double)t.yy + (double)t.xx));
    RwMatrixUpdate(out);
}

BMX_ADDR(0x00488190)
int zrwModelSystemInitialize(void)
{
    if (gZrwModelState.initialized != 0) {
        zdebugFail("Already initialized", kZrwModelFile, 0x2d1, 0);
    }
    memset(&gZrwModelState, 0, sizeof gZrwModelState);
    if (zrwModelCreateInitialize() == 0) {
        return 0;
    }
    if (zrwModelRenderInitialize() == 0) {
        return 0;
    }
    gZrwModelState.initialized = 1;
    gZrwModelState.matrix = RwMatrixCreate();
    gZrwModelState.scratch = RwMatrixCreate();
    return 1;
}

BMX_ADDR(0x00488200)
int zrwModelSystemShutdown(void)
{
    if (gZrwModelState.initialized != 0) {
        RwMatrixDestroy(gZrwModelState.matrix);
        RwMatrixDestroy(gZrwModelState.scratch);
        if (zrwModelCreateShutdown() == 0) {
            return 0;
        }
        if (zrwModelRenderShutdown() == 0) {
            return 0;
        }
        gZrwModelState.initialized = 0;
    }
    return 1;
}

BMX_ADDR(0x00488250)
void zrwModelMatchParts(ZrwModel *model, const ZrwPartDefTable *table)
{
    const uint32_t sets = model->def->partSetCount;

    for (uint32_t set = 0; set < sets; set++) {
        const ZrwPartSet *parts = partSet(model, set);

        for (uint32_t i = 0; i < parts->count; i++) {
            ZrwModelPart *part = &parts->parts[i];
            const uint32_t hash = part->hash;
            uint32_t k = 0;

            part->def = NULL;
            for (; k < table->count; k++) {
                if (hash == table->defs[k].hash) {
                    part->def = &table->defs[k];
                    break;
                }
            }
            if (k == table->count) {
                gZdebugPrint("BodyPart not matched Hash:%x\n", hash);
            }
        }
    }
    model->flags |= 2;
}

BMX_ADDR(0x00488310)
int zrwModelPartCount(const ZrwModel *model)
{
    return (int)model->partSets[0].count;
}

BMX_ADDR(0x00488320)
void zrwModelGetPartHashes(const ZrwModel *model, uint32_t *out)
{
    const ZrwPartSet *set = &model->partSets[0];

    for (uint32_t i = 0; i < set->count; i++) {
        out[i] = set->parts[i].hash;
    }
}

BMX_ADDR(0x00488350)
ZrwModelPart *zrwModelFindPart(const ZrwModel *model, uint32_t hash)
{
    const ZrwPartSet *set = &model->partSets[0];

    for (uint32_t i = 0; i < set->count; i++) {
        if (set->parts[i].hash == hash) {
            return &set->parts[i];
        }
    }
    gZdebugPrint("Part (0x%x)not found in model:%s\n", hash, model->def->name);
    return NULL;
}

BMX_ADDR(0x004883a0)
void zrwModelPartToWorld(const ZrwModel *model, const ZrwModelPart *part, const ZVec3 *local, ZVec3 *out,
                         uint8_t flags)
{
    const ZMatrix *matrix = (flags & 1) == 0 ? RwFrameGetMatrix(part->frame) : RwFrameGetLTM(part->frame);
    ZVec3 vec, origin;
    double x;
    float y, z;

    vec.x = (float)((double)gZrwScale * local->x);
    vec.y = (float)-((double)local->y * gZrwScale);
    vec.z = (float)-((double)local->z * gZrwScale);
    RwV3dTransformVectors(&vec, &vec, 1, matrix);
    RwV3dTransformPoints(&origin, &gZrwOrigin, 1, matrix);
    x = (double)origin.x + vec.x;
    y = (float)((double)origin.y + vec.y);
    z = (float)((double)origin.z + vec.z);
    out->x = (float)((x - gZrwWorldCenter.x) * gZrwInvScale);
    out->y = (float)-(((double)y - gZrwWorldCenter.y) * gZrwInvScale);
    out->z = (float)-(((double)z - gZrwWorldCenter.z) * gZrwInvScale);
}

BMX_ADDR(0x00488480)
void zrwModelPartOffset(const ZrwModel *model, const ZrwModelPart *part, ZVec3 *out)
{
    out->x = (float)((double)part->offset[0] * gZrwInvScale);
    out->y = (float)-((double)part->offset[1] * gZrwInvScale);
    out->z = (float)-((double)part->offset[2] * gZrwInvScale);
}

BMX_ADDR(0x004884b0)
void zrwModelPartMatrix(const ZrwModel *model, const ZrwModelPart *part, void *out, uint8_t flags)
{
    const ZMatrix *matrix = (flags & 1) == 0 ? RwFrameGetMatrix(part->frame) : RwFrameGetLTM(part->frame);

    zrwMatrixToGame(out, matrix);
}

BMX_ADDR(0x004884f0)
uint32_t zrwModelPartUserData(const ZrwModel *model, const ZrwModelPart *part)
{
    return part->userData;
}

/* Key of an entry: the one given, or the model's default key. */
BMX_ADDR(0x00488500)
uint32_t zrwModelEntryKey(const ZrwModel *model, const uint32_t *key)
{
    if (key == NULL) {
        return model->def->defaultKey;
    }
    return *key;
}

/* Entry with the given key; without a key the model's name (at def + 0x14) comes back instead. */
BMX_ADDR(0x00488520)
void *zrwModelEntryLookup(const ZrwModel *model, const uint32_t *key)
{
    if (key == NULL) {
        return model->def->name;
    }
    for (int i = 0; i < model->def->entryCount; i++) {
        if ((int32_t)*key == model->def->entries[i].key) {
            return &model->def->entries[i];
        }
    }
    return NULL;
}

BMX_ADDR(0x00488580)
int zrwModelSetPosition(ZrwModel *model, const ZVec3 *position)
{
    model->position = *position;
    return 1;
}

BMX_ADDR(0x004885a0)
int zrwModelSetRotation(ZrwModel *model, const ZQuat *rotation)
{
    model->rotation = *rotation;
    return 1;
}

/* Bit 0 of the flags says the scale differs from 1 by more than 1e-5 on some axis. */
BMX_ADDR(0x004885d0)
int zrwModelSetScale(ZrwModel *model, const ZVec3 *scale)
{
    model->scale = *scale;
    /* x87 compares: a NaN counts as "below" */
    if (!(__builtin_fabs((double)model->scale.x - 1.0) >= (double)0x1.4f8b58p-17f)) {
        if (!(__builtin_fabs((double)model->scale.y - 1.0) >= (double)0x1.4f8b58p-17f)) {
            if (!(__builtin_fabs((double)model->scale.z - 1.0) >= (double)0x1.4f8b58p-17f)) {
                model->flags &= ~1u;
                return 1;
            }
        }
    }
    model->flags |= 1;
    return 1;
}

BMX_ADDR(0x00488650)
int zrwModelUpdate(ZrwModel *model)
{
    void *clump = model->clump;
    ZMatrix *matrix;
    float x, y, z;
    uint32_t *m;

    if ((gZrwFlags & 2) == 0) {
        zrwQuatToMatrix(&model->rotation, gZrwModelState.matrix);
    } else {
        zrwQuatToMatrixMirrored(&model->rotation, gZrwModelState.matrix);
    }
    x = (float)((double)model->position.x * gZrwScale + gZrwWorldCenter.x);
    y = (float)((double)gZrwWorldCenter.y - (double)model->position.y * gZrwScale);
    z = (float)((double)gZrwWorldCenter.z - (double)model->position.z * gZrwScale);
    if (model->flags & 1) {
        ZVec3 scale = model->scale;

        RwMatrixScale(gZrwModelState.matrix, &scale, 1);
    }
    matrix = RwFrameGetMatrix(((void **)clump)[1]);
    m = (uint32_t *)matrix;
    m[10] = 0x3f800000;
    m[5] = 0x3f800000;
    m[0] = 0x3f800000;
    m[4] = 0;
    m[2] = 0;
    m[1] = 0;
    m[9] = 0;
    m[8] = 0;
    m[6] = 0;
    m[14] = 0;
    m[13] = 0;
    m[12] = 0;
    m[3] |= 0x20003;
    if (model->flags & 2) {
        const ZrwPartSet *set = partSet(model, model->partSet);
        ZrwModelPart *part = set->parts;

        for (uint32_t i = set->count; i != 0; i--, part++) {
            ZrwPartDef *def = part->def;

            if (def != NULL) {
                ZMatrix *partMatrix = RwFrameGetMatrix(part->frame);

                zrwQuatToMatrix(&def->rotation, partMatrix);
                partMatrix->pos.x = (float)((double)def->offset[0] * gZrwScale + part->offset[0]);
                partMatrix->pos.y = (float)((double)part->offset[1] - (double)def->offset[1] * gZrwScale);
                partMatrix->pos.z = (float)((double)part->offset[2] - (double)def->offset[2] * gZrwScale);
            }
        }
    }
    RwMatrixMultiply(gZrwModelState.scratch, matrix, gZrwModelState.matrix);
    gZrwModelState.scratch->pos.x = (float)((double)matrix->pos.x + x);
    gZrwModelState.scratch->pos.y = (float)((double)matrix->pos.y + y);
    gZrwModelState.scratch->pos.z = (float)((double)matrix->pos.z + z);
    memcpy(matrix, gZrwModelState.scratch, sizeof *matrix);
    RwFrameUpdateObjects(((void **)clump)[1]);
    if (model->def->flags2 & 2) {
        RpSkinClumpUpdate(clump);
    }
    if (model->onUpdate != NULL) {
        model->onUpdate(model);
    }
    if (model->clump2 != NULL) {
        void *frame = ((void **)model->clump2)[1];

        memcpy(RwFrameGetMatrix(frame), matrix, sizeof *matrix);
        RwFrameUpdateObjects(frame);
    }
    return 1;
}

BMX_ADDR(0x00488890)
int zrwMdlRegisterPlugin(void)
{
    gZrwMdlPluginOffset =
        RpClumpRegisterPlugin(4, 0xff02, BMX_FNPTR(RwPluginObjectConstructor, 0x00487d90, zrwMdlConstructor),
                              BMX_FNPTR(RwPluginObjectDestructor, 0x00487db0, zrwMdlDestructor), NULL);
    return gZrwMdlPluginOffset != -1;
}

/*
 * Turns model states off (`clear`) and on (`set`). The bit 4 blocks only touch the second clump; bit 2 puts
 * the clumps in or takes them out of the world; bit 8 shows or hides them; bit 1 marks the definition and all
 * atomics with flag 0x80.
 */
BMX_ADDR(0x004888c0)
int zrwModelSetFlags(const ZrwModel *model, uint8_t clear, uint8_t set)
{
    if (model->clump2 != NULL) {
        if (clear & 4) {
            maskAtomics(model->clump2, 0xfffffffa, 1);
        }
        if (set & 4) {
            maskAtomics(model->clump2, 0xfffffffa, 5);
        }
    }
    if (model->clump2 != NULL) {
        if (clear & 4) {
            maskAtomics(model->clump2, 0xfffffffb, 0);
        }
        if (set & 4) {
            maskAtomics(model->clump2, 0xffffffff, 4);
        }
    }
    if (clear & 2) {
        worldRemove(model);
    }
    if (set & 2) {
        worldAdd(model);
    }
    if (clear & 8) {
        if (model->clump2 == NULL) {
            zrwClumpMaskAtomicFlags(model->clump, 1, 0);
        } else if (RpClumpGetWorld(model->clump2) != NULL) {
            RpWorldRemoveClump(gZrwWorld, model->clump2);
        }
    }
    if (set & 8) {
        if (model->clump2 == NULL) {
            maskAtomics(model->clump, 0xffffffff, 1);
        } else if (RpClumpGetWorld(model->clump2) == NULL) {
            RpWorldAddClump(gZrwWorld, model->clump2);
        }
    }
    if (clear & 1) {
        model->def->flags &= ~1u;
        maskAtomics(model->clump, 0xffffff7f, 0);
        if (model->clump2 != NULL) {
            maskAtomics(model->clump2, 0xffffff7f, 0);
        }
    }
    if (set & 1) {
        model->def->flags |= 1;
        maskAtomics(model->clump, 0xffffffff, 0x80);
        if (model->clump2 != NULL) {
            maskAtomics(model->clump2, 0xffffffff, 0x80);
        }
    }
    return 0;
}

BMX_ADDR(0x00488b80)
uint32_t zrwMdlGetUserData(const void *object)
{
    if (gZrwMdlPluginOffset < 1) {
        return 0;
    }
    return *(const uint32_t *)(*(const uint8_t *const *)((const uint8_t *)object + 0x40) +
                               gZrwMdlPluginOffset);
}

BMX_ADDR(0x00488ba0)
void zrwModelSetUserValue(ZrwModel *model, uint32_t value)
{
    model->userValue = value;
}

BMX_ADDR(0x00488bb0)
uint32_t zrwModelGetUserValue(const ZrwModel *model)
{
    return model->userValue;
}

BMX_ADDR(0x00488bc0)
void *zrwModelRenderClump(const ZrwModel *model)
{
    return model->clump2 != NULL ? model->clump2 : model->clump;
}

BMX_ADDR(0x00488bd0)
void *zrwModelFindMaterial(const ZrwModel *model, int hash)
{
    for (void *material = zlistFirst(model->materials); material != NULL;
         material = zlistNext(model->materials)) {
        if (hash == gZrwTextureHash(gZrwMaterialTexture(material, 0))) {
            return material;
        }
    }
    return NULL;
}

BMX_ADDR(0x00488c30)
int zrwModelMaterialCount(const ZrwModel *model)
{
    return zlistCount(model->materials);
}

BMX_ADDR(0x00488c50)
void *zrwModelGetMaterial(const ZrwModel *model, uint32_t index)
{
    if (index >= (uint32_t)gZrwVtMatCount((void *)model)) {
        zdebugFail("Material index out of range", kZrwModelFile, 0x5f0, 0);
        return NULL;
    }
    return zlistGet(model->materials, (int)index);
}

BMX_ADDR(0x00488ca0)
void zrwModelSetUpdateHook(ZrwModel *model, const uint32_t *arg)
{
    model->onUpdateArg = *arg;
    zrwModelSetRenderCallback(model, BMX_FNPTR(ZrwObjectCallback, 0x0048f530, zrwModelRenderCallback));
}

BMX_ADDR(0x00488cc0)
void zrwModelSetColor(const ZrwModel *model, const void *color)
{
    void *geometry = *(void **)((uint8_t *)zrwModelCreateFirstAtomic(model->clump) + 0x1c);
    uint32_t mask[2] = {0xffffffff, 0x40};
    int count;

    RpClumpForAllAtomics(model->clump, BMX_FNPTR(RpAtomicCallback, 0x00487bc0, zrwAtomicMaskGeometryFlags),
                         mask);
    count = RpGeometryGetNumMaterials(geometry);
    while (count != 0) {
        count--;
        RpMaterialSetColor(RpGeometryGetMaterial(geometry, count), color);
    }
}

BMX_ADDR(0x00488d30)
int zrwObjDestroy(void *object)
{
    void *model;

    zrwmatFreeSkeleton(object);
    model = *(void **)((uint8_t *)object + 0x2c);
    if (model != NULL) {
        gZrwVtDestroy(model);
    }
    return 1;
}

BMX_ADDR(0x00488d60)
int zrwObjUpdate(void *object)
{
    gZrwVtUpdate(*(void **)((uint8_t *)object + 0x2c));
    return 1;
}

BMX_ADDR(0x00488d80)
void zrwObjSetPosition(void *object, const void *position)
{
    gZrwVtSetPos(*(void **)((uint8_t *)object + 0x2c), position);
}

BMX_ADDR(0x00488da0)
void zrwObjSetRotation(void *object, const void *rotation)
{
    gZrwVtSetRot(*(void **)((uint8_t *)object + 0x2c), rotation);
}

BMX_ADDR(0x00488dc0)
void zrwObjSetScale(void *object, const void *scale)
{
    gZrwVtSetScale(*(void **)((uint8_t *)object + 0x2c), scale);
}

BMX_ADDR(0x00488de0)
int zrwObjSetShown(void *object, int shown)
{
    if (shown == 0) {
        gZrwVtSetFlags(*(void **)((uint8_t *)object + 0x2c), 2, 0);
        return 1;
    }
    gZrwVtSetFlags(*(void **)((uint8_t *)object + 0x2c), 0, 2);
    return 1;
}

BMX_ADDR(0x00488e20)
uint32_t zrwObjEntryKey(void *object)
{
    return (uint32_t)(uintptr_t)gZrwVtKey(*(void **)((uint8_t *)object + 0x2c), NULL);
}

BMX_ADDR(0x00488e40)
void *zrwObjClump(void *object)
{
    return (void *)(uintptr_t)gZrwVtClump(*(void **)((uint8_t *)object + 0x2c));
}

BMX_ADDR(0x00488e60)
void *zrwObjEntry(void *object)
{
    return gZrwVtEntry(*(void **)((uint8_t *)object + 0x2c), NULL);
}

BMX_ADDR(0x00488e80)
int zrwObjUpdateAgain(void *object)
{
    gZrwVtUpdate(*(void **)((uint8_t *)object + 0x2c));
    return 1;
}
