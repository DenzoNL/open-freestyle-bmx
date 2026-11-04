/*
 * Renderer models (C:\NgEngine\Zge\Com\ZglRw\zrwModel.c): a model is a RenderWare clump (one per level of
 * detail, the first one is used) with an optional collision clump, a set of body parts (frames that follow
 * a skeleton), a material list and a position, scale and rotation given in game coordinates. Everything the
 * model hands to RenderWare goes through the game-to-RenderWare coordinate conversion (scale gZrwScale, y and
 * z mirrored, origin gZrwWorldCenter).
 */
#ifndef ZGE_COM_ZGLRW_ZRWMODEL_H
#define ZGE_COM_ZGLRW_ZRWMODEL_H

#include <stddef.h>
#include <stdint.h>

#include "compat/orig.h"
#include "zge/zlist.h"
#include "zge/zmath.h"

struct RpAtomic;
struct RpClump;
struct RpGeometry;
struct RwFrame;

/* Named entry of a model (one per frame of its hierarchy), found by the frame's user data key. 0x14 bytes. */
typedef struct ZrwModelEntry {
    char name[0x10]; /* 0x00 NUL terminated by the loader */
    int32_t key;     /* 0x10 */
} ZrwModelEntry;

/* Definition of one body part of the skeleton: where it sits and how it is rotated. 0x44 bytes. */
typedef struct ZrwPartDef {
    uint8_t pad00[4];
    uint32_t hash;   /* 0x04 name hash */
    float offset[3]; /* 0x08 game units */
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

/*
 * Body part of a model instance: one node of the clump's frame hierarchy, built when the clump is loaded or
 * cloned. Children are prepended, so `child` is the child added last. 0x28 bytes.
 */
typedef struct ZrwModelPart {
    uint32_t hash; /* 0x00 the frame's user data, matches ZrwPartDef.hash */
    uint32_t pad04;
    ZrwPartDef *def;              /* 0x08 NULL until zrwModelMatchParts found its definition */
    struct RwFrame *frame;        /* 0x0c the frame the part drives */
    float restPos[3];             /* 0x10 the frame's position when the hierarchy was built */
    struct ZrwModelPart *parent;  /* 0x1c */
    struct ZrwModelPart *sibling; /* 0x20 */
    struct ZrwModelPart *child;   /* 0x24 */
} ZrwModelPart;

/* A level of detail: the distance it starts at and the parts of its clump's hierarchy. 12 bytes. */
typedef struct ZrwModelLod {
    float distance;      /* 0x00 from the MLOD chunk; 0 or NaN take the default (gZrwLodDefaults) */
    uint32_t partCount;  /* 0x04 */
    ZrwModelPart *parts; /* 0x08 */
} ZrwModelLod;

#define kZrwModelMaxLods 4

/* Options of a model definition (ZrwModelDef.options, the first word of its MDEF chunk). */
enum {
    kZrwModelSkinned = 0x02,        /* RpSkin clump: set up at load, updated with the frames */
    kZrwModelNotInWorld = 0x04,     /* not added to the world at load */
    kZrwModelUnlit = 0x08,          /* rpGEOMETRYLIGHT is cleared on every level of detail */
    kZrwModelFollowCamera = 0x10,   /* drawn at the camera's position by a per-frame callback (sky) */
    kZrwModelNoCollision = 0x40,    /* its atomics never take part in collision tests */
    kZrwModelCollisionClump = 0x80, /* has an MCOL chunk: a separate clump collides instead of the model */
};

/* Atomic render callback (RpAtomicRenderCallBack). */
typedef struct RpAtomic *(BMX_CDECL *ZrwAtomicRenderFn)(struct RpAtomic *atomic);

/* Shared description of a model; the models using it count themselves in refCount. 0xbc bytes. */
typedef struct ZrwModelDef {
    uint32_t flags;    /* 0x00 bit 0: set by zrwModelSetFlags(.., 1) */
    uint32_t refCount; /* 0x04 the last model to let go frees the definition */
    uint32_t options;  /* 0x08 kZrwModel*; bit 0: zrwModelRender supports the model */
    uint16_t lodCount; /* 0x0c levels of detail loaded so far */
    uint8_t pad0e[2];
    uint32_t defaultKey; /* 0x10 */
    char name[0x10];     /* 0x14 */
    char path[0x80];     /* 0x24 file it was loaded from (zrwModelLoadFromFile) */
    uint16_t entryCount; /* 0xa4 */
    uint8_t padA6[2];
    ZrwModelEntry *entries;         /* 0xa8 */
    struct RpAtomic *atomic;        /* 0xac first atomic of the first clump */
    struct RpGeometry *geometry;    /* 0xb0 its geometry */
    ZrwAtomicRenderFn atomicRender; /* 0xb4 its render callback before the model's own replaced it */
    float *vertices; /* 0xb8 locked vertices of the first morph target (zrwModelLockGeometry) */
} ZrwModelDef;

typedef struct ZrwModel ZrwModel;
typedef void(BMX_CDECL *ZrwModelUpdateFn)(ZrwModel *model);

/* A model instance (0x8c byte records from the loader's pool). */
struct ZrwModel {
    uint32_t pad00; /* 0x00 the loader clears its low byte */
    uint32_t pad04;
    uint32_t flags;                 /* 0x08 bit 0: scale differs from 1, bit 1: parts matched */
    struct RpClump *clump;          /* 0x0c */
    struct RpClump *collisionClump; /* 0x10 the MCOL clump, NULL when the model collides with its own */
    void *userValue;                /* 0x14 the model's object (zrwshadow.c, gZbspObjectFrameFn) */
    ZVec3 position;                 /* 0x18 game coordinates */
    uint32_t pad24;
    ZVec3 scale; /* 0x28 */
    uint32_t pad34;
    ZQuat rotation;     /* 0x38 */
    uint16_t activeLod; /* 0x48 the level of detail zrwModelUpdate poses (not range checked) */
    uint8_t pad4a[2];
    ZList *materials;                   /* 0x4c */
    ZrwModelLod lods[kZrwModelMaxLods]; /* 0x50 */
    ZrwModelUpdateFn onUpdate;          /* 0x80 called at the end of zrwModelUpdate */
    uint32_t color;                     /* 0x84 RGBA the model's render callback draws it with */
    ZrwModelDef *def;                   /* 0x88 */
};

_Static_assert(sizeof(ZrwModelEntry) == 0x14, "zrw entry");
_Static_assert(offsetof(ZrwPartDef, hash) == 0x04, "zrw part definition");
_Static_assert(offsetof(ZrwPartDef, offset) == 0x08, "zrw part definition");
_Static_assert(offsetof(ZrwPartDef, rotation) == 0x18, "zrw part definition");
_Static_assert(sizeof(ZrwPartDef) == 0x44, "zrw part definition");
_Static_assert(offsetof(ZrwPartDefTable, defs) == 0x08, "zrw part definition table");
_Static_assert(offsetof(ZrwModelPart, def) == 0x08, "zrw part");
BMX_LAYOUT32_ASSERT(offsetof(ZrwModelPart, frame) == 0x0c, "zrw part");
BMX_LAYOUT32_ASSERT(offsetof(ZrwModelPart, restPos) == 0x10, "zrw part");
BMX_LAYOUT32_ASSERT(offsetof(ZrwModelPart, parent) == 0x1c, "zrw part");
BMX_LAYOUT32_ASSERT(offsetof(ZrwModelPart, child) == 0x24, "zrw part");
BMX_LAYOUT32_ASSERT(sizeof(ZrwModelPart) == 0x28, "zrw part");
BMX_LAYOUT32_ASSERT(sizeof(ZrwModelLod) == 12, "zrw level of detail");
_Static_assert(offsetof(ZrwModelDef, refCount) == 0x04, "zrw model definition");
_Static_assert(offsetof(ZrwModelDef, options) == 0x08, "zrw model definition");
_Static_assert(offsetof(ZrwModelDef, lodCount) == 0x0c, "zrw model definition");
_Static_assert(offsetof(ZrwModelDef, defaultKey) == 0x10, "zrw model definition");
_Static_assert(offsetof(ZrwModelDef, name) == 0x14, "zrw model definition");
_Static_assert(offsetof(ZrwModelDef, path) == 0x24, "zrw model definition");
_Static_assert(offsetof(ZrwModelDef, entryCount) == 0xa4, "zrw model definition");
_Static_assert(offsetof(ZrwModelDef, entries) == 0xa8, "zrw model definition");
BMX_LAYOUT32_ASSERT(offsetof(ZrwModelDef, atomic) == 0xac, "zrw model definition");
BMX_LAYOUT32_ASSERT(offsetof(ZrwModelDef, geometry) == 0xb0, "zrw model definition");
BMX_LAYOUT32_ASSERT(offsetof(ZrwModelDef, atomicRender) == 0xb4, "zrw model definition");
BMX_LAYOUT32_ASSERT(offsetof(ZrwModelDef, vertices) == 0xb8, "zrw model definition");
BMX_LAYOUT32_ASSERT(sizeof(ZrwModelDef) == 0xbc, "zrw model definition");
_Static_assert(offsetof(ZrwModel, flags) == 0x08, "zrw model");
BMX_LAYOUT32_ASSERT(offsetof(ZrwModel, clump) == 0x0c, "zrw model");
BMX_LAYOUT32_ASSERT(offsetof(ZrwModel, collisionClump) == 0x10, "zrw model");
BMX_LAYOUT32_ASSERT(offsetof(ZrwModel, userValue) == 0x14, "zrw model");
BMX_LAYOUT32_ASSERT(offsetof(ZrwModel, position) == 0x18, "zrw model");
BMX_LAYOUT32_ASSERT(offsetof(ZrwModel, scale) == 0x28, "zrw model");
BMX_LAYOUT32_ASSERT(offsetof(ZrwModel, rotation) == 0x38, "zrw model");
BMX_LAYOUT32_ASSERT(offsetof(ZrwModel, activeLod) == 0x48, "zrw model");
BMX_LAYOUT32_ASSERT(offsetof(ZrwModel, materials) == 0x4c, "zrw model");
BMX_LAYOUT32_ASSERT(offsetof(ZrwModel, lods) == 0x50, "zrw model");
BMX_LAYOUT32_ASSERT(offsetof(ZrwModel, onUpdate) == 0x80, "zrw model");
BMX_LAYOUT32_ASSERT(offsetof(ZrwModel, color) == 0x84, "zrw model");
BMX_LAYOUT32_ASSERT(offsetof(ZrwModel, def) == 0x88, "zrw model");
BMX_LAYOUT32_ASSERT(sizeof(ZrwModel) == 0x8c, "zrw model");

/* State of the model system (memset at initialisation): the two scratch matrices of zrwModelUpdate. */
typedef struct ZrwModelState {
    int initialized;
    ZMatrix *matrix;  /* the model's rotation and scale */
    ZMatrix *scratch; /* the product with the root frame's matrix */
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
typedef void *(BMX_CDECL *ZrwModelLookup)(void *model, const uint32_t *key);
typedef uint32_t(BMX_CDECL *ZrwModelKeyFn)(void *model, const uint32_t *key);
typedef void *(BMX_CDECL *ZrwModelClumpFn)(void *model);
typedef int(BMX_CDECL *ZrwModelFlagsFn)(void *model, int clear, int set);
#define gZrwVtDestroy  ORIG_VAR(0x00560e94, ZrwModelCall, gZrwVtDestroy)
#define gZrwVtUpdate   ORIG_VAR(0x00560e98, ZrwModelCall, gZrwVtUpdate)
#define gZrwVtSetPos   ORIG_VAR(0x00560e9c, ZrwModelCall2, gZrwVtSetPos)
#define gZrwVtSetRot   ORIG_VAR(0x00560ea0, ZrwModelCall2, gZrwVtSetRot)
#define gZrwVtSetScale ORIG_VAR(0x00560ea4, ZrwModelCall2, gZrwVtSetScale)
#define gZrwVtKey      ORIG_VAR(0x00560ec0, ZrwModelKeyFn, gZrwVtKey)
#define gZrwVtEntry    ORIG_VAR(0x00560ec4, ZrwModelLookup, gZrwVtEntry)
#define gZrwVtSetFlags ORIG_VAR(0x00560ed8, ZrwModelFlagsFn, gZrwVtSetFlags)
#define gZrwVtClump    ORIG_VAR(0x00560ee4, ZrwModelClumpFn, gZrwVtClump)
#define gZrwVtMatCount ORIG_VAR(0x00560eec, ZrwModelCall, gZrwVtMatCount)

/* Material and texture accessors of the renderer (zrwmat.c, zrwtxr.c), called through pointers in the exe's
 * data. */
typedef void *(BMX_CDECL *ZrwMaterialTextureFn)(void *material, int index);
typedef int(BMX_CDECL *ZrwTextureHashFn)(void *texture);
#define gZrwMaterialTexture ORIG_VAR(0x00561034, ZrwMaterialTextureFn, gZrwMaterialTexture)
#define gZrwTextureHash     ORIG_VAR(0x00562414, ZrwTextureHashFn, gZrwTextureHash)

/* ---- RenderWare object helpers ---------------------------------------------------------------------- */

/* Entry whose key equals `key` in the model's table, or NULL. */
ZrwModelEntry *zrwModelFindEntry(const ZrwModel *model, int key);
/* RpClumpForAllAtomics callback: flags = (flags & mask[0]) | mask[1]; bumps the atomic's stamp. */
void *zrwAtomicMaskFlags(void *atomic, const uint32_t *mask);
/* Applies zrwAtomicMaskFlags to every atomic of the clump: flags = (flags & ~clear) | set. */
void zrwClumpMaskAtomicFlags(void *clump, uint32_t clear, uint32_t set);
/* Same callback for the flags of the atomic's geometry. Returns the atomic. */
void *zrwAtomicMaskGeometryFlags(void *atomic, const uint32_t *mask);
void zrwClumpMaskGeometryFlags(void *clump, uint32_t clear, uint32_t set);
/* Adds (nonzero) or removes the model's clumps to / from the renderer's world when they are not / are in it.
 */
void zrwModelSetInWorld(const ZrwModel *model, int add);
/*
 * Turns collision on or off: the collision clump joins or leaves the world, or, without one, the model's
 * atomics get or lose rpATOMICCOLLISIONTEST.
 */
void zrwModelSetCollision(const ZrwModel *model, int on);

/* Plugin slot hooks of the clump (constructor clears the slot; the destructor does nothing). */
void *zrwMdlConstructor(void *clump);
void *zrwMdlDestructor(void *clump);
int zrwMdlSetUserData(void *clump, void *value);
int zrwMdlRegisterPlugin(void);
/* The value in the plugin slot of the atomic's clump (the model that owns it). */
void *zrwMdlGetUserData(const void *atomic);

/* Rotation matrix of a quaternion in RenderWare space (y, z negated); the second variant mirrors x. */
void zrwQuatToMatrix(const ZQuat *quat, ZMatrix *out);
void zrwQuatToMatrixMirrored(const ZQuat *quat, ZMatrix *out);

int zrwModelSystemInitialize(void);
int zrwModelSystemShutdown(void);

/* ---- model parts ------------------------------------------------------------------------------------ */

/* Links every part of every level of detail to the definition with the same hash (printing the misses). */
void zrwModelMatchParts(ZrwModel *model, const ZrwPartDefTable *table);
int zrwModelPartCount(const ZrwModel *model);
void zrwModelGetPartHashes(const ZrwModel *model, uint32_t *out);
ZrwModelPart *zrwModelFindPart(const ZrwModel *model, uint32_t hash);
/* World position of a point given in the frame of a part (flags & 1: through the world matrix). */
void zrwModelPartToWorld(const ZrwModel *model, const ZrwModelPart *part, const ZVec3 *local, ZVec3 *out,
                         uint8_t flags);
/* Rest position of the part in game coordinates. */
void zrwModelPartOffset(const ZrwModel *model, const ZrwModelPart *part, ZVec3 *out);
void zrwModelPartMatrix(const ZrwModel *model, const ZrwModelPart *part, void *out, uint8_t flags);
/* The part's parent part (NULL for the root), as a word. */
uint32_t zrwModelPartParent(const ZrwModel *model, const ZrwModelPart *part);
uint32_t zrwModelEntryKey(const ZrwModel *model, const uint32_t *key);
void *zrwModelEntryLookup(const ZrwModel *model, const uint32_t *key);

/* Setters: position, rotation and scale in game coordinates (the scale flag tracks a scale other than 1). */
int zrwModelSetPosition(ZrwModel *model, const ZVec3 *position);
int zrwModelSetRotation(ZrwModel *model, const ZQuat *rotation);
int zrwModelSetScale(ZrwModel *model, const ZVec3 *scale);
/* Pushes position, scale, rotation and the parts of the active level of detail to the RenderWare frames. */
int zrwModelUpdate(ZrwModel *model);
/*
 * `clear` and `set` are masks of: 1 (the definition flag and atomic flag 0x80), 2 (world membership),
 * 4 (draw the collision clump), 8 (collision, as zrwModelSetCollision).
 */
int zrwModelSetFlags(const ZrwModel *model, uint8_t clear, uint8_t set);

void zrwModelSetUserValue(ZrwModel *model, void *value);
void *zrwModelGetUserValue(const ZrwModel *model);
/* The clump collision queries test: the collision clump when there is one. */
void *zrwModelCollisionClump(const ZrwModel *model);
/* Material with the given texture name hash, or NULL. */
void *zrwModelFindMaterial(const ZrwModel *model, int hash);
int zrwModelMaterialCount(const ZrwModel *model);
void *zrwModelGetMaterial(const ZrwModel *model, uint32_t index);
/* Sets the model's colour and installs the render callback that draws with it. */
void zrwModelSetDrawColor(ZrwModel *model, const uint32_t *color);
/* Makes the materials of the model take the vertex colours, then sets the colour of each one. */
void zrwModelSetColor(const ZrwModel *model, const void *color);

/* ---- dispatch wrappers: the hooks of a scene object (ZObj) of the model type; each passes ZObj.model on */

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
