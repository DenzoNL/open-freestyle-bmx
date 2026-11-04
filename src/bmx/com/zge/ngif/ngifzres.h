/*
 * Resource tables (C:\zaxis\Bmx\src\com\zge\NgIF\NgIFzres.c): the game's fixed data libraries
 * (Assets\Tables.ZAL with the level and player tables, MiscData, Replay and Material), the small IFF
 * tables read out of them, and the overlay material list.
 */
#ifndef BMX_COM_ZGE_NGIF_NGIFZRES_H
#define BMX_COM_ZGE_NGIF_NGIFZRES_H

#include <stdint.h>

#include "compat/orig.h"
#include "zge/zfile.h"

enum { kZresLevelInfoBytes = 100, kZresRiderRecordBytes = 0x18, kZresFixedDefsSize = 0x1000 };

/* Everything NgIFzres.c keeps at 0x5f8f60. */
typedef struct ZresState {
    uint8_t levelInfo[kZresLevelInfoBytes]; /* 0x00 record returned by ngifzresLoadLevelInfo */
    uint8_t bufferA[kZresFixedDefsSize]; /* 0x64 fixed buffer for assets loaded with the "alternate" flag */
    struct ZassetLibrary *libs[2];       /* 0x1064 Material and MiscData libraries (overlay sources) */
    uint32_t spare;                      /* 0x106c */
    uint8_t *mapData;                    /* 0x1070 Tables asset 0 (level table), loaded once */
    uint8_t pad1074[4];
    uint8_t *playerData; /* 0x1078 Tables asset 1 (player table), loaded once */
    uint8_t pad107c[4];
    uint8_t playerInfo[8];     /* 0x1080 */
    uint16_t playerCount;      /* 0x1088 header of the player table ("PLHD") */
    uint16_t playerRecordSize; /* 0x108a */
    uint8_t pad108c[4];
    ZfileHandle *mapFile;                       /* 0x1090 RAM disk file over mapData */
    void *playerIff;                            /* 0x1094 */
    uint8_t bufferB[kZresFixedDefsSize];        /* 0x1098 fixed buffer for the other assets */
    void *mapIff;                               /* 0x2098 */
    int overlayCounter;                         /* 0x209c numbers the overlay textures */
    uint8_t riderRecord[kZresRiderRecordBytes]; /* 0x20a0 */
    ZfileHandle *playerFile;                    /* 0x20b8 */
    uint8_t pad20bc[0xc];
    uint32_t levelCount;      /* 0x20c8 header of the level table ("ZMHD") */
    uint32_t levelRecordSize; /* 0x20cc must be 100 */
    int overlayInit;          /* 0x20d0 */
} ZresState;

BMX_LAYOUT32_ASSERT(__builtin_offsetof(ZresState, libs) == 0x1064, "zres layout");
BMX_LAYOUT32_ASSERT(__builtin_offsetof(ZresState, playerCount) == 0x1088, "zres layout");
BMX_LAYOUT32_ASSERT(__builtin_offsetof(ZresState, bufferB) == 0x1098, "zres layout");
BMX_LAYOUT32_ASSERT(__builtin_offsetof(ZresState, riderRecord) == 0x20a0, "zres layout");
BMX_LAYOUT32_ASSERT(__builtin_offsetof(ZresState, levelCount) == 0x20c8, "zres layout");
BMX_LAYOUT32_ASSERT(sizeof(ZresState) == 0x20d4, "zres size");

#define gZres ORIG_VAR(0x005f8f60, ZresState, gZres)

/* Library handles (zasset). */
#define gZresTablesLib   ORIG_VAR(0x0067847c, struct ZassetLibrary *, gZresTablesLib)
#define gZresMaterialLib ORIG_VAR(0x0067848c, struct ZassetLibrary *, gZresMaterialLib)
#define gZresMiscLib     ORIG_VAR(0x00678490, struct ZassetLibrary *, gZresMiscLib)
#define gZresReplayLib   ORIG_VAR(0x0068ffa8, struct ZassetLibrary *, gZresReplayLib)
/* Free lists the resource system owns (destroyed by ngifzresUnload). */
#define gZresListA ORIG_VAR(0x005f34ac, struct ZMemFreeList *, gZresListA)
#define gZresListB ORIG_VAR(0x005f34b0, struct ZMemFreeList *, gZresListB)

/* An overlay material source: id, library (0 Material, 1 MiscData) and the asset number in it. */
typedef struct ZresOverlaySource {
    uint32_t id;
    uint32_t lib;
    uint16_t asset;
    uint8_t pad[6];
} ZresOverlaySource;

enum { kZresOverlaySources = 11, kZresOverlaySlots = 512 };

/* One slot of the loaded overlay materials. */
typedef struct ZresOverlaySlot {
    uint8_t pad00[4];
    uint8_t u0, v0, u1, v1; /* 0x04 */
    uint8_t pad08[4];
    uint8_t width, height; /* 0x0c in pixels, capped at 255 */
    uint8_t pad0e[2];
    uint32_t texture; /* 0x10 the material's handle (zrwmatHandle), 0 when the slot is free */
} ZresOverlaySlot;

_Static_assert(sizeof(ZresOverlaySlot) == 0x14, "overlay slot");

typedef ZresOverlaySource ZresOverlaySourceTable[kZresOverlaySources];
typedef ZresOverlaySlot ZresOverlaySlotTable[kZresOverlaySlots];
#define gZresOverlaySources ORIG_VAR(0x0055d978, ZresOverlaySourceTable, gZresOverlaySources)
#define gZresOverlaySlots   ORIG_VAR(0x0068c3e0, ZresOverlaySlotTable, gZresOverlaySlots)

/* Renderer entry points the overlay loader calls through pointers. */
typedef void *(BMX_CDECL *ZresLoadTextureFn)(const void *data, uint32_t size, const char *name, int a, int b);
typedef void *(BMX_CDECL *ZresGetTextureFn)(void *material, int index);
typedef uint32_t(BMX_CDECL *ZresTextureSizeFn)(void *texture);
#define gZresLoadTexture   ORIG_VAR(0x00561048, ZresLoadTextureFn, gZresLoadTexture)
#define gZresGetTexture    ORIG_VAR(0x00561034, ZresGetTextureFn, gZresGetTexture)
#define gZresTextureWidth  ORIG_VAR(0x00562404, ZresTextureSizeFn, gZresTextureWidth)
#define gZresTextureHeight ORIG_VAR(0x00562408, ZresTextureSizeFn, gZresTextureHeight)

/* Loads asset (group, asset) of a library into one of the two fixed buffers; NULL when it does not exist. */
uint8_t *ngifzresLoadAsset(struct ZassetLibrary *lib, uint32_t group, uint32_t asset, uint32_t *sizeOut,
                           int alternate);
/* Opens the level table (once). */
int ngifzresInitLevels(void);
/* Opens the player table (once). */
int ngifzresInitPlayers(void);
/* Opens both tables. */
int ngifzresInitData(void);
/* Closes the four libraries. */
void ngifzresCloseLibraries(void);
/* Opens the four libraries; fatal when one is missing. */
void ngifzresOpenLibraries(void);
/* Closes the tables and libraries and destroys the two free lists. */
void ngifzresUnload(void);
uint32_t ngifzresGetSpare(void);
uint32_t ngifzresLevelCount(void);
uint32_t ngifzresPlayerCount(void);
uint8_t *ngifzresGetPlayerInfo(void);
/* Entry `index` of the bike table at 0x55d8c8 (fatal above 43). */
uint32_t *ngifzresGetBikeEntry(uint32_t index);

/* Reads the 0x18 byte rider record `index` into a static buffer (asserts "Invalid player"). */
const uint8_t *ngifzresGetRiderRecord(uint32_t index);
/* Loads the overlay material `id` into slot `slot` (once). */
void ngifzresRegister(int id, uint32_t a, uint32_t slot);

/* Resource system start / reset / shutdown (zres init returns 0). */
int ngifzresInit(void);
int ngifzresReset(void);
void ngifzresShutdown(void);
/* Reads the 100 byte level info record `index` into a static buffer (record 0 for an index past the last). */
const uint8_t *ngifzresLoadLevelInfo(uint32_t index);

#endif
