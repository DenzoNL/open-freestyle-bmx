/* Resource tables (C:\zaxis\Bmx\src\com\zge\NgIF\NgIFzres.c). */
#include "bmx/com/zge/ngif/ngifzres.h"

#include "compat/crt.h"
#include "zge/zasset.h"
#include "zge/zdebug.h"
#include "zge/zfile.h"
#include "zge/ziff.h"
#include "zge/zmem.h"
#include "zge/zstring.h"

static const char kResFile[] = "C:\\zaxis\\Bmx\\src\\com\\zge\\NgIF\\NgIFzres.c";

/* Chunk ids of the table files. */
enum {
    kIdForm = 0x4d524f46,          /* 'FORM' */
    kIdMapHeader = 0x44484d5a,     /* 'ZMHD' */
    kIdMapRecords = 0x50414d5a,    /* 'ZMAP' */
    kIdPlayerHeader = 0x44484c50,  /* 'PLHD' */
    kIdPlayerRecords = 0x52594c50, /* 'PLYR' */
};

BMX_ADDR(0x0046b8b0)
uint8_t *ngifzresLoadAsset(int lib, uint32_t group, uint32_t asset, uint32_t *sizeOut, int alternate)
{
    uint32_t size;
    uint8_t *buffer;

    if (zassetFind(lib, (int)group, (int)asset) == 0) {
        zassetDump(lib);
        return NULL;
    }
    size = zassetSize(lib);
    buffer = alternate != 0 ? gZres.bufferA : gZres.bufferB;
    if (size >= kZresFixedDefsSize) {
        zdebugFail("Memory overflow in zres()!!! Increase FIXED_DEFS_SIZE.", kResFile, 0x65, 0);
    }
    zassetRead(lib, buffer);
    if (sizeOut != NULL) {
        *sizeOut = size;
    }
    return buffer;
}

BMX_ADDR(0x0046b7c0)
int ngifzresInitLevels(void)
{
    if (gZres.mapData == NULL) {
        uint32_t size;

        gZres.mapData = ngifzresLoadAsset(gZresTablesLib, 0, 0, &size, 0);
        gZres.mapFile = zfileOpenRamDisk(gZres.mapData, size);
        gZres.mapIff = ziffOpen(gZres.mapFile, kIdForm, 0);
        ziffFindChunk(gZres.mapIff, kIdMapHeader);
        ziffRead(gZres.mapIff, &gZres.levelCount, 8);
        if (ziffChunkSize(gZres.mapIff) != 8) {
            zdebugFail("Out of sync map data", kResFile, 0x95, 0);
        }
        if (gZres.levelRecordSize != 100) {
            zdebugFail("Out of sync map data", kResFile, 0x96, 0);
        }
    }
    return 0;
}

BMX_ADDR(0x0046b950)
uint32_t ngifzresGetSpare(void)
{
    return gZres.spare;
}

BMX_ADDR(0x0046b970)
uint32_t ngifzresLevelCount(void)
{
    return gZres.levelCount;
}

BMX_ADDR(0x0046b990)
const uint8_t *ngifzresLoadLevelInfo(uint32_t index)
{
    if (gZres.levelCount <= index) {
        index = 0;
    }
    ziffFindChunk(gZres.mapIff, kIdMapRecords);
    ziffSeek(gZres.mapIff, 1, (int)(index * 100));
    ziffRead(gZres.mapIff, gZres.levelInfo, 100);
    return gZres.levelInfo;
}

BMX_ADDR(0x0046ba10)
int ngifzresInitPlayers(void)
{
    if (gZres.playerData == NULL) {
        uint32_t size;

        gZres.playerData = ngifzresLoadAsset(gZresMiscLib, 1, 0, &size, 1);
        gZres.playerFile = zfileOpenRamDisk(gZres.playerData, size);
        gZres.playerIff = ziffOpen(gZres.playerFile, kIdForm, 0);
        ziffFindChunk(gZres.playerIff, kIdPlayerHeader);
        ziffRead(gZres.playerIff, &gZres.playerCount, 8);
        if (ziffChunkSize(gZres.playerIff) != 8) {
            zdebugFail("Out of sync player data", kResFile, 0x117, 0);
        }
        if (gZres.playerRecordSize != 0x18) {
            zdebugFail("Out of sync player data", kResFile, 0x118, 0);
        }
    }
    return 0;
}

BMX_ADDR(0x0046bb00)
const uint8_t *ngifzresGetRiderRecord(uint32_t index)
{
    index &= 0xffff;
    if (gZres.playerCount <= index) {
        zdebugFail("Invalid player", kResFile, 0x135, 0);
        return NULL;
    }
    ziffFindChunk(gZres.playerIff, kIdPlayerRecords);
    ziffSeek(gZres.playerIff, 1, (int)(index * 0x18));
    ziffRead(gZres.playerIff, gZres.riderRecord, 0x18);
    return gZres.riderRecord;
}

BMX_ADDR(0x0046bba0)
uint32_t ngifzresPlayerCount(void)
{
    return gZres.playerCount;
}

BMX_ADDR(0x0046bbc0)
uint8_t *ngifzresGetPlayerInfo(void)
{
    return gZres.playerInfo;
}

BMX_ADDR(0x0046bbe0)
uint32_t *ngifzresGetBikeEntry(uint32_t index)
{
    if (index > 0x2b) {
        zdebugFail("bike number out of range", kResFile, 0x199, 0);
    }
    return (uint32_t *)BMX_ORIG(0x0055d8c8) + index;
}

BMX_ADDR(0x0046bc20)
int ngifzresInitData(void)
{
    ngifzresInitPlayers();
    ngifzresInitLevels();
    return 0;
}

BMX_ADDR(0x0046bc40)
void ngifzresCloseLibraries(void)
{
    if (gZresMaterialLib != 0) {
        zassetCloseLibrary(gZresMaterialLib);
        gZresMaterialLib = 0;
    }
    if (gZresMiscLib != 0) {
        zassetCloseLibrary(gZresMiscLib);
        gZresMiscLib = 0;
    }
    if (gZresTablesLib != 0) {
        zassetCloseLibrary(gZresTablesLib);
        gZresTablesLib = 0;
    }
    if (gZresReplayLib != 0) {
        zassetCloseLibrary(gZresReplayLib);
        gZresReplayLib = 0;
    }
}

/* Opens one library; prints and fails when it cannot be opened. */
static int openLibrary(const char *path, int line)
{
    const int lib = zassetOpenLibrary(path);

    if (lib == 0) {
        gZdebugPrint("ZRES: Failed to open:%s\n", path);
        zdebugFail("Could not open ZAL", kResFile, line, 0);
    }
    return lib;
}

BMX_ADDR(0x0046bcd0)
void ngifzresOpenLibraries(void)
{
    gZresMiscLib = openLibrary("Assets\\MiscData.ZAL", 0x1e9);
    if (gZresMiscLib == 0) {
        return;
    }
    gZresTablesLib = openLibrary("Assets\\Tables.ZAL", 0x1f1);
    if (gZresTablesLib == 0) {
        return;
    }
    gZresReplayLib = openLibrary("Assets\\Replay.ZAL", 0x1f9);
    if (gZresReplayLib == 0) {
        return;
    }
    gZresMaterialLib = openLibrary("Assets\\Material.ZAL", 0x201);
}

BMX_ADDR(0x0046be10)
int ngifzresInit(void)
{
    ngifzresOpenLibraries();
    ngifzresInitData();
    return 0;
}

BMX_ADDR(0x0046be30)
void ngifzresUnload(void)
{
    if (gZres.playerData != NULL) {
        zfileClose(gZres.playerFile, 0);
        gZres.playerData = NULL;
    }
    if (gZres.mapData != NULL) {
        zfileClose(gZres.mapFile, 0);
        gZres.mapData = NULL;
    }
    ngifzresCloseLibraries();
    if (gZresListB != NULL) {
        zmemFreeListDestroy(gZresListB);
        gZresListB = NULL;
    }
    if (gZresListA != NULL) {
        zmemFreeListDestroy(gZresListA);
        gZresListA = NULL;
    }
}

BMX_ADDR(0x0046bed0)
void ngifzresShutdown(void) {}

BMX_ADDR(0x0046bef0)
int ngifzresReset(void)
{
    gZres.libs[0] = gZresMaterialLib;
    gZres.libs[1] = gZresMiscLib;
    gZres.overlayCounter = 0;
    return 0;
}

/* Byte size of a texture dimension reported by the renderer, capped at 255 (each query is made twice). */
static uint8_t textureByte(ZresTextureSizeFn query, void *material)
{
    void *texture = gZresGetTexture(material, 0);

    if (query(texture) > 0xff) {
        return 0xff;
    }
    texture = gZresGetTexture(material, 0);
    return (uint8_t)query(texture);
}

BMX_ADDR(0x0046bf30)
void ngifzresRegister(int id, uint32_t a, uint32_t slotIndex)
{
    const ZresOverlaySource *source = gZresOverlaySources;
    uint32_t sourceIndex = 0;
    char name[32];
    int lib;
    ZresOverlaySlot *slot;
    uint32_t *mark;
    uint32_t size;
    void *data, *material;

    if (gZres.overlayInit == 0) {
        zmemFill(gZresOverlaySlots, 0, sizeof gZresOverlaySlots);
        gZres.overlayInit = 1;
    }
    while (source->id != (uint32_t)id) {
        source++;
        sourceIndex++;
        if (sourceIndex >= kZresOverlaySources) {
            zdebugFail("zresAddMaterialToLoadList() Not found", kResFile, 0x2b0, 0);
            return;
        }
    }
    lib = gZres.libs[source->lib];
    if (zassetFind(lib, source->asset, 0) == 0) {
        gZdebugPrint("%s %d %s\n", kResFile, 0x28c, "zresAddMaterialToLoadList() Not found");
        return;
    }
    slot = &gZresOverlaySlots[slotIndex & 0xffff];
    if (slot->texture != NULL) {
        return;
    }
    mark = zmemStaticMark();
    size = zassetSize(lib);
    data = zmemStaticAlloc(size);
    zassetRead(lib, data);
    zstrFormat(name, "Overlay%d", gZres.overlayCounter);
    material = gZresLoadTexture(data, size, name, 0, 0);
    zmemStaticRelease(mark);
    if (material == NULL) {
        zdebugFail("ERROR loading overlay material", kResFile, 0x2aa, 0);
        return;
    }
    gZres.overlayCounter++;
    zmemFill(slot, 0, sizeof *slot);
    slot->texture = material;
    slot->width = textureByte(gZresTextureWidth, material);
    slot->height = textureByte(gZresTextureHeight, material);
    slot->u0 = 0;
    slot->v0 = 0;
    slot->u1 = slot->width;
    slot->v1 = slot->height;
}
