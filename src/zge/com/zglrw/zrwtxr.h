/*
 * Renderer textures (C:\NgEngine\Zge\Com\ZglRw\zrwTxr.c): textures come from IFF chunks (TBMP, TPNG, TDDS)
 * that are handed to RenderWare through a RAM disk file, and get a small wrapper record. The packet filed a
 * few world and particle callbacks (0x497200..0x497780) here too, they belong to zrwworld.c.
 */
#ifndef ZGE_COM_ZGLRW_ZRWTXR_H
#define ZGE_COM_ZGLRW_ZRWTXR_H

#include <stdint.h>

#include "compat/orig.h"
#include "zge/com/zglrw/zrwmodel.h"
#include "zge/zlist.h"
#include "zge/zmath.h"
#include "zge/zmem.h"

/* Wrapper of a RenderWare texture (0x28 bytes, from the free list of the texture system). */
typedef struct ZrwTexture {
    uint32_t flags; /* 0x00 low byte 4 for a loaded texture */
    uint32_t pad04;
    int32_t refCount; /* 0x08 */
    int32_t nameHash; /* 0x0c zstrHash of the name */
    const char *name; /* 0x10 the RwTexture's name */
    int32_t width;    /* 0x14 */
    int32_t height;   /* 0x18 */
    int32_t depth;    /* 0x1c */
    uint32_t pad20;
    void *texture; /* 0x24 RwTexture */
} ZrwTexture;

_Static_assert(sizeof(ZrwTexture) == 0x28, "zrw texture");

/* The texture system's state (memset to 0 at initialisation). */
typedef struct ZrwTextureState {
    int initialized;        /* 0x00 */
    ZMemFreeList *textures; /* 0x04 ZrwTexture records */
    uint8_t key[4];         /* 0x08 colour key (r, g, b): pixels of this colour become transparent */
    void *dictionary;       /* 0x0c RwTexDictionary */
    void *(BMX_CDECL *readCallback)(const char *name, const char *mask); /* 0x10 RenderWare's own reader */
    const char *loadName;    /* 0x14 name of the texture being read from an IFF chunk */
    uint8_t palette[0x400];  /* 0x18 RGBA entries of the palette last read */
    int mipmapped;           /* 0x418 textures filter with mipmaps */
    int width;               /* 0x41c size of the last image read */
    int height;              /* 0x420 */
    int depth;               /* 0x424 */
    void *loadBuffer;        /* 0x428 optional fixed buffer for chunk data */
    uint32_t loadBufferSize; /* 0x42c */
} ZrwTextureState;

_Static_assert(sizeof(ZrwTextureState) == 0x430, "zrw texture state");

#define gZrwTxr ORIG_VAR(0x00674290, ZrwTextureState, gZrwTxr)

/* Texture loading: `type` is 0 (BMP), 1 (PNG) or 4 (anything); `data` holds the file. NULL on failure. */
ZrwTexture *LoadTexture(const void *data, uint32_t size, const char *name, int type);
int zrwTextureSystemInitialize(void);
int zrwTextureSystemShutdown(void);
/* A blank texture record. */
ZrwTexture *zrwTextureCreateRecord(void);
/* Free list destroy callback: drops the RenderWare texture and frees the record at reference count 0. */
int zrwTextureRelease(ZrwTexture *texture);
int zrwTextureGetWidth(const ZrwTexture *texture);
int zrwTextureGetHeight(const ZrwTexture *texture);
int zrwTextureGetDepth(const ZrwTexture *texture);
const char *zrwTextureGetName(const ZrwTexture *texture);
int zrwTextureGetNameHash(const ZrwTexture *texture);
/* Reads the palette (as RGBA) into gZrwTxr.palette and returns it, or NULL for an unpaletted texture. */
uint8_t *zrwTextureReadPalette(const ZrwTexture *texture);
int zrwTextureWritePalette(const ZrwTexture *texture);
int zrwTextureLog(const ZrwTexture *texture);
/* The RenderWare texture of a record; its raster; takes a reference. */
void *zrwTextureGetRwTexture(const ZrwTexture *texture);
void *zrwTextureGetRaster(const ZrwTexture *texture);
void *zrwTextureAddRef(ZrwTexture *texture);
/* Sets the name the texture reader is going to look for. */
void zrwTextureSetLoadName(const char *name);
/* Stores a user value in the texture's material plugin slot (when the plugin is registered). */
void zrwTextureSetUserData(const ZrwTexture *texture, uint32_t value);
/* The `mipmapped` flag of the texture system, also handed to RenderWare's mipmapping / find callback. */
void zrwSetMaterialParam(const void *value);
void zrwSetMaterialValue(int value);
/* Logs every texture record. */
void zrwTextureLogAll(void);
void zrwRasterLog(void *raster);

/* The IFF chunk readers (TBMP, TPNG, TDDS), the image readers registered with RenderWare, ... */
int ReadTBMP(void *iff);
int ReadTPNG(void *iff);
int ReadTDDS(void);
void *BMPImageRead(const char *name);
void *PNGImageRead(const char *name);
void *DDSImageRead(void);
void *zrwTextureReadCallback(const char *name, const char *mask);
/* Pixels of the colour key become fully transparent black. */
void zrwImageApplyColorKey(void *image);
void zrwPaletteEncodeEntry(void *out, uint32_t format, int index, const uint8_t *rgba);

/* Plugin slot of a material (zrwmat.c) */
#define gZrwMatPluginOffset ORIG_VAR(0x005623e8, int, gZrwMatPluginOffset)
void rwMatConstructor(void *material);
void *rwMatDestructor(void *material);
uint32_t zrwMatGetUserData(const void *material);
/* Registers the material plugin on RenderWare textures; 0 on failure. */
int zrwMatRegisterPlugin(void);

/* ---- world and particle callbacks, TODO(file): probably belong to zrwworld.c --------------------------- */

#define gZrwWorldActive       ORIG_VAR(0x006741e4, int, gZrwWorldActive)
#define gZrwWorldMaterials    ORIG_VAR(0x006741e8, ZList *, gZrwWorldMaterials)
#define gZrwWorldPvsState     ORIG_VAR(0x00674204, uint32_t, gZrwWorldPvsState)
#define gZrwPvsLoaded         ORIG_VAR(0x006976e4, int, gZrwPvsLoaded)
#define gZrwPvsEnabled        ORIG_VAR(0x006976e8, int, gZrwPvsEnabled)
#define gZrwWorldCenterScaled ORIG_VAR(0x006976d8, ZVec3, gZrwWorldCenterScaled)

/* Releases the world: its materials and the PVS data, then creates the empty replacement world. */
int zrwWorldReset(void);
void *zrwWorldFindMaterial(int hash);
int zrwWorldMaterialCount(void);
void *zrwWorldGetMaterial(int index);
int zrwWorldHasPvs(void);
/* Camera dependent PVS setup; the second entry point (0x4974b0) is the tail of this function. */
int zrwWorldSetupPvs(void);
int zrwWorldSetPvsEnabled(int enabled);
int zrwWorldLoadPvs(void);
int zrwWorldRender(void);
void zrwWorldNop(void);
/* Particle systems: a list of particles (velocity vector at +0x24) and the time of the last step at +0x28. */
void zrwParticleAccelerate(int enabled, uint32_t *system, const ZVec3 *acceleration);
void zrwParticleNop(void);
void zrwParticleFloorBounce(int enabled, uint32_t *system, const float *params);

#endif
