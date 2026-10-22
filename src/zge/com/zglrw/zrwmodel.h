/*
 * Renderer models (C:\NgEngine\Zge\Com\ZglRw\zrwModel.c): a model is one or two RenderWare clumps with a
 * set of body parts (frames that follow a skeleton), a material list and a position, scale and rotation
 * given in game coordinates. Everything the model hands to RenderWare goes through the game-to-RenderWare
 * coordinate conversion (scale gZrwScale, y and z mirrored, origin gZrwWorldCenter).
 */
#ifndef ZGE_COM_ZGLRW_ZRWMODEL_H
#define ZGE_COM_ZGLRW_ZRWMODEL_H

#include <stdint.h>

#include "compat/orig.h"
#include "zge/zlist.h"
#include "zge/zmath.h"

/* Entry of a model's table of 0x14 byte records, found by the key stored at +0x10. */
typedef struct ZrwModelEntry {
    uint8_t pad00[0x10];
    int32_t key;
} ZrwModelEntry;

/* Definition of one body part of the skeleton: where it sits and how it is rotated. 0x44 bytes. */
typedef struct ZrwPartDef {
    uint8_t pad00[4];
    uint32_t hash;   /* 0x04 name hash */
    float offset[3]; /* 0x08 */
    uint8_t pad14[4];
    ZQuat rotation; /* 0x18 */
    uint8_t pad28[0x1c];
} ZrwPartDef;

/* The part definitions of a model: a count, then the array at +8. */
typedef struct ZrwPartDefTable {
    uint32_t count;
    uint32_t pad04;
    ZrwPartDef *defs;
} ZrwPartDefTable;

/* Body part of a model instance. 0x28 bytes. */
typedef struct ZrwModelPart {
    uint32_t hash; /* 0x00 matches ZrwPartDef.hash */
    uint32_t pad04;
    ZrwPartDef *def;   /* 0x08 NULL until zrwModelMatchParts found its definition */
    void *frame;       /* 0x0c RwFrame the part drives */
    float offset[3];   /* 0x10 */
    uint32_t userData; /* 0x1c */
    uint8_t pad20[8];
} ZrwModelPart;

/* A set of parts (the model keeps one per pose set). 12 bytes. */
typedef struct ZrwPartSet {
    uint32_t count;
    ZrwModelPart *parts;
    uint32_t pad08;
} ZrwPartSet;

/* Shared description of a model. */
typedef struct ZrwModelDef {
    uint32_t flags; /* 0x00 bit 0: set by zrwModelSetFlags(.., 1) */
    uint8_t pad04[4];
    uint32_t flags2;       /* 0x08 bit 1: parts need the post-update hook (0x504490) */
    uint16_t partSetCount; /* 0x0c */
    uint8_t pad0e[2];
    uint32_t defaultKey; /* 0x10 */
    char name[0x90];     /* 0x14 */
    uint16_t entryCount; /* 0xa4 */
    uint8_t padA6[2];
    ZrwModelEntry *entries; /* 0xa8 */
    void *atomic;           /* 0xac first atomic of the first clump */
    void *geometry;         /* 0xb0 its geometry */
    void *atomicInfo;       /* 0xb4 the pointer at +0x4c of the atomic */
    float *vertices;        /* 0xb8 locked vertices of the first morph target (zrwModelLockGeometry) */
} ZrwModelDef;

typedef struct ZrwModel ZrwModel;
typedef void(BMX_CDECL *ZrwModelUpdateFn)(ZrwModel *model);

struct ZrwModel {
    uint8_t pad00[8];
    uint32_t flags;     /* 0x08 bit 0: scale differs from 1, bit 1: parts matched */
    void *clump;        /* 0x0c RpClump (its frame is at +4) */
    void *clump2;       /* 0x10 optional second clump that is kept in step */
    uint32_t userValue; /* 0x14 */
    ZVec3 position;     /* 0x18 */
    uint32_t pad24;
    ZVec3 scale; /* 0x28 */
    uint32_t pad34;
    ZQuat rotation;   /* 0x38 */
    uint16_t partSet; /* 0x48 index of the active part set */
    uint8_t pad4a[2];
    ZList *materials; /* 0x4c */
    uint32_t pad50;
    ZrwPartSet partSets[3]; /* 0x54 */
    uint8_t pad78[8];
    ZrwModelUpdateFn onUpdate; /* 0x80 called at the end of zrwModelUpdate */
    uint32_t onUpdateArg;      /* 0x84 */
    ZrwModelDef *def;          /* 0x88 */
};

_Static_assert(sizeof(ZrwModelEntry) == 0x14, "zrw entry");
_Static_assert(sizeof(ZrwPartDef) == 0x44, "zrw part definition");
_Static_assert(sizeof(ZrwModelPart) == 0x28, "zrw part");
_Static_assert(sizeof(ZrwPartSet) == 12, "zrw part set");
_Static_assert(sizeof(ZrwModelDef) == 0xbc, "zrw model definition");

/* State of the model system (memset at initialisation): the two scratch matrices of zrwModelUpdate. */
typedef struct ZrwModelState {
    int initialized;
    ZMatrix *matrix;
    ZMatrix *scratch;
} ZrwModelState;

#define gZrwModelState ORIG_VAR(0x006722e8, ZrwModelState, gZrwModelState)
/* Origin of the RenderWare frame (all zero): transformed by a node matrix it gives the node position. */
#define gZrwOrigin ORIG_VAR(0x006722f8, ZVec3, gZrwOrigin)
/* The renderer's world and its centre, game to RenderWare scale and its inverse, and the coordinate flags
 * (bit 0: textures use bilinear filtering, bit 1: mirror the rotations). */
#define gZrwWorld       ORIG_VAR(0x006976c8, void *, gZrwWorld)
#define gZrwWorldCenter ORIG_VAR(0x006976cc, ZVec3, gZrwWorldCenter)
#define gZrwScale       ORIG_VAR(0x00697720, float, gZrwScale)
#define gZrwInvScale    ORIG_VAR(0x00697724, float, gZrwInvScale)
#define gZrwFlags       ORIG_VAR(0x00697728, uint8_t, gZrwFlags)
/* Offset of the model owner slot inside a clump (RpClumpRegisterPlugin result, -1 when not registered). */
#define gZrwMdlPluginOffset ORIG_VAR(0x00560e70, int, gZrwMdlPluginOffset)

/* Entry points the model object dispatches through (0x560e94..). Their original targets are in this file. */
typedef int(BMX_CDECL *ZrwModelCall)(void *model);
typedef int(BMX_CDECL *ZrwModelCall2)(void *model, const void *arg);
typedef void *(BMX_CDECL *ZrwModelLookup)(void *model, const void *key);
typedef int(BMX_CDECL *ZrwModelFlagsFn)(void *model, int clear, int set);
#define gZrwVtDestroy  ORIG_VAR(0x00560e94, ZrwModelCall, gZrwVtDestroy)
#define gZrwVtUpdate   ORIG_VAR(0x00560e98, ZrwModelCall, gZrwVtUpdate)
#define gZrwVtSetPos   ORIG_VAR(0x00560e9c, ZrwModelCall2, gZrwVtSetPos)
#define gZrwVtSetRot   ORIG_VAR(0x00560ea0, ZrwModelCall2, gZrwVtSetRot)
#define gZrwVtSetScale ORIG_VAR(0x00560ea4, ZrwModelCall2, gZrwVtSetScale)
#define gZrwVtKey      ORIG_VAR(0x00560ec0, ZrwModelLookup, gZrwVtKey)
#define gZrwVtEntry    ORIG_VAR(0x00560ec4, ZrwModelLookup, gZrwVtEntry)
#define gZrwVtSetFlags ORIG_VAR(0x00560ed8, ZrwModelFlagsFn, gZrwVtSetFlags)
#define gZrwVtClump    ORIG_VAR(0x00560ee4, ZrwModelCall, gZrwVtClump)
#define gZrwVtMatCount ORIG_VAR(0x00560eec, ZrwModelCall, gZrwVtMatCount)

/* Material and texture accessors of the renderer (zrwmat.c, zrwtxr.c), called through pointers in the exe's
 * data. */
typedef void *(BMX_CDECL *ZrwMaterialTextureFn)(void *material, int index);
typedef int(BMX_CDECL *ZrwTextureHashFn)(void *texture);
#define gZrwMaterialTexture ORIG_VAR(0x00561034, ZrwMaterialTextureFn, gZrwMaterialTexture)
#define gZrwTextureHash     ORIG_VAR(0x00562414, ZrwTextureHashFn, gZrwTextureHash)

/* ---- RenderWare object helpers ---------------------------------------------------------------------- */

/* Entry whose key (+0x10) equals `key` in the model's table, or NULL. */
ZrwModelEntry *zrwModelFindEntry(const ZrwModel *model, int key);
/* RpClumpForAllAtomics callback: flags (byte at +2) = (flags & mask[0]) | mask[1]; counts a use at +0x14. */
void *zrwAtomicMaskFlags(void *atomic, const uint32_t *mask);
/* Applies zrwAtomicMaskFlags to every atomic of the clump: flags = (flags & ~clear) | set. */
void zrwClumpMaskAtomicFlags(void *clump, uint32_t clear, uint32_t set);
/* Same callback for the flags of the atomic's geometry (the pointer at +0x1c). Returns the atomic. */
void *zrwAtomicMaskGeometryFlags(void *atomic, const uint32_t *mask);
void zrwClumpMaskGeometryFlags(void *clump, uint32_t clear, uint32_t set);
/* Adds (nonzero) or removes the model's clumps to / from the renderer's world when they are not / are in it.
 */
void zrwModelSetInWorld(const ZrwModel *model, int add);
/* Shows or hides the model: world membership of the second clump, or the object flag of the first. */
void zrwModelSetActive(const ZrwModel *model, int on);

/* Plugin slot hooks of the clump (constructor clears the slot; the destructor does nothing). */
void zrwMdlConstructor(void *clump);
void *zrwMdlDestructor(void *clump);
int zrwMdlSetUserData(void *clump, uint32_t value);
int zrwMdlRegisterPlugin(void);
uint32_t zrwMdlGetUserData(const void *object);

/* Rotation matrix of a quaternion in RenderWare space (y, z negated); the second variant mirrors x. */
void zrwQuatToMatrix(const ZQuat *quat, ZMatrix *out);
void zrwQuatToMatrixMirrored(const ZQuat *quat, ZMatrix *out);

int zrwModelSystemInitialize(void);
int zrwModelSystemShutdown(void);

/* ---- model parts ------------------------------------------------------------------------------------ */

/* Links every part of every set to the definition with the same hash (printing the misses). */
void zrwModelMatchParts(ZrwModel *model, const ZrwPartDefTable *table);
int zrwModelPartCount(const ZrwModel *model);
void zrwModelGetPartHashes(const ZrwModel *model, uint32_t *out);
ZrwModelPart *zrwModelFindPart(const ZrwModel *model, uint32_t hash);
/* World position of a point given in the frame of a part (flags & 1: through the world matrix). */
void zrwModelPartToWorld(const ZrwModel *model, const ZrwModelPart *part, const ZVec3 *local, ZVec3 *out,
                         uint8_t flags);
/* Offset of the part in game coordinates. */
void zrwModelPartOffset(const ZrwModel *model, const ZrwModelPart *part, ZVec3 *out);
void zrwModelPartMatrix(const ZrwModel *model, const ZrwModelPart *part, void *out, uint8_t flags);
uint32_t zrwModelPartUserData(const ZrwModel *model, const ZrwModelPart *part);
uint32_t zrwModelEntryKey(const ZrwModel *model, const uint32_t *key);
void *zrwModelEntryLookup(const ZrwModel *model, const uint32_t *key);

/* Setters: position, rotation and scale in game coordinates (the scale flag tracks a scale other than 1). */
int zrwModelSetPosition(ZrwModel *model, const ZVec3 *position);
int zrwModelSetRotation(ZrwModel *model, const ZQuat *rotation);
int zrwModelSetScale(ZrwModel *model, const ZVec3 *scale);
/* Pushes position, scale, rotation and the parts of the active set to the RenderWare frames. */
int zrwModelUpdate(ZrwModel *model);
/* `clear` and `set` are masks of the bits 1 (collision), 2 (world membership), 4, 8 (shown). */
int zrwModelSetFlags(const ZrwModel *model, uint8_t clear, uint8_t set);

void zrwModelSetUserValue(ZrwModel *model, uint32_t value);
uint32_t zrwModelGetUserValue(const ZrwModel *model);
/* The clump to draw: the second one when there is one. */
void *zrwModelRenderClump(const ZrwModel *model);
/* Material with the given texture name hash, or NULL. */
void *zrwModelFindMaterial(const ZrwModel *model, int hash);
int zrwModelMaterialCount(const ZrwModel *model);
void *zrwModelGetMaterial(const ZrwModel *model, uint32_t index);
void zrwModelSetUpdateHook(ZrwModel *model, const uint32_t *arg);
/* Makes the materials of the model take the vertex colours, then sets the colour of each one. */
void zrwModelSetColor(const ZrwModel *model, const void *color);

/* ---- dispatch wrappers: each passes the model at +0x2c of the object on --------------------------- */

int zrwObjDestroy(void *object);
int zrwObjUpdate(void *object);
void zrwObjSetPosition(void *object, const void *position);
void zrwObjSetRotation(void *object, const void *rotation);
void zrwObjSetScale(void *object, const void *scale);
int zrwObjSetShown(void *object, int shown);
uint32_t zrwObjEntryKey(void *object);
void *zrwObjClump(void *object);
void *zrwObjEntry(void *object);
int zrwObjUpdateAgain(void *object);

#endif
