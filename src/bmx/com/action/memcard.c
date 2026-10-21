/* The save file (C:\zaxis\Bmx\Src\com\action\memcard.c). */
#include "bmx/com/action/memcard.h"

#include "bmx/com/action/comdata.h"
#include "bmx/com/action/overlay.h"
#include "compat/crt.h"
#include "zge/zdebug.h"
#include "zge/zfile.h"
#include "zge/zmem.h"

static const char memcardFile[] = "C:\\zaxis\\Bmx\\Src\\com\\action\\memcard.c";
static const char kSavePath[] = ".\\save\\SAVE.DAT";

/*
 * Layout of the save file. The state blocks: the options and rider data (0x6955e0, 0xc3c bytes), the level
 * records (0x551fd8, 0x1680 bytes) and the player profiles (0x6952a0, 0x98 bytes).
 */
enum {
    kSaveOffsetOptions = 0x8,
    kSaveOffsetRecords = 0xc44,
    kSaveOffsetProfiles = 0x22c4,
    kSaveBytesOptions = 0x30f * 4,
    kSaveBytesRecords = 0x5a0 * 4,
    kSaveBytesProfiles = 0x26 * 4,
};

#define gSaveOptions ORIG_VAR(0x006955e0, uint8_t, gSaveOptions)

BMX_ADDR(0x00412130)
int saveFileClose(void)
{
    int status = kSaveNoFile;
    ZfileHandle *const file = zfileOpen(kSavePath, 1, 0);

    if (file != NULL) {
        status = kSaveExists;
        zfileClose(file, 0);
    }
    return status;
}

BMX_ADDR(0x00412170)
int memcardFormat(void)
{
    return 0;
}

/*
 * Loads the save file. Only the size word is validated, not the magic. The messages pass the address of an
 * uninitialised buffer as the file name (an original bug).
 */
BMX_ADDR(0x00412190)
int saveFileRead(void)
{
    uint32_t *const mark = zmemStaticMark();
    uint8_t *const buffer = zmemStaticAlloc(kSaveSize);
    char name[200];
    int status;
    ZfileHandle *file;

    if (buffer == NULL) {
        zdebugFail("error allocating temporary buffer for mem card load", memcardFile, 0xd7, 0);
        return -1;
    }
    file = zfileOpen(kSavePath, 1, 0);
    if (file == NULL) {
        status = kSaveNoFile;
        gZdebugPrint("Error: couldn't open file \"%s\" for reading\n", name);
    } else if (zfileRead(file, buffer, kSaveSize, 0) == kSaveSize) {
        status = kSaveOk;
        if (*(uint32_t *)(buffer + 4) == kSaveSize) {
            memcpy(&gSaveOptions, buffer + kSaveOffsetOptions, kSaveBytesOptions);
            memcpy(&gProfiles, buffer + kSaveOffsetProfiles, kSaveBytesProfiles);
            memcpy(&gComdataRecords, buffer + kSaveOffsetRecords, kSaveBytesRecords);
        } else {
            status = kSaveInvalid;
            gZdebugPrint("Error: memory card data is invalid (probably old version)\n");
        }
        zfileClose(file, 0);
    } else {
        status = kSaveIoError;
        gZdebugPrint("Error: couldn't read from file \"%s\"\n", name);
        zfileClose(file, 0);
    }
    zmemStaticRelease(mark);
    return status;
}

/* Writes the save file; returns 0, 5 (cannot create the file), 9 (short write) or -1 (out of memory). */
BMX_ADDR(0x00412340)
int saveFileWrite(void)
{
    uint32_t *const mark = zmemStaticMark();
    uint8_t *const buffer = zmemStaticAlloc(kSaveSize);
    char name[200];
    ZfileHandle *file;

    if (buffer == NULL) {
        return -1;
    }
    memcpy(buffer + kSaveOffsetOptions, &gSaveOptions, kSaveBytesOptions);
    memcpy(buffer + kSaveOffsetProfiles, &gProfiles, kSaveBytesProfiles);
    memcpy(buffer + kSaveOffsetRecords, &gComdataRecords, kSaveBytesRecords);
    *(uint32_t *)buffer = kSaveMagic;
    *(uint32_t *)(buffer + 4) = kSaveSize;
    file = zfileOpen(kSavePath, 2, 0);
    if (file == NULL) {
        gZdebugPrint("Error: couldn't open file \"%s\" for writing\n", name);
        zmemStaticRelease(mark);
        return kSaveNoFile;
    }
    if (zfileWrite(file, buffer, kSaveSize, 0) == kSaveSize) {
        zfileClose(file, 0);
        zmemStaticRelease(mark);
        return 0;
    }
    gZdebugPrint("Error: couldn't write to file \"%s\"\n", name);
    zfileClose(file, 0);
    zmemStaticRelease(mark);
    return kSaveIoError;
}
