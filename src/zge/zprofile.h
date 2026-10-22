/*
 * Profiler and the renderer's file callbacks (C:\NgEngine\Zge\zprofile.c). The file callbacks replace
 * RenderWare's file interface while a texture is read from memory (see zrwramdisk.h): every call goes to the
 * zfile layer through the handle slot RenderWare passes in.
 */
#ifndef ZGE_ZPROFILE_H
#define ZGE_ZPROFILE_H

#include <stdint.h>

#include "compat/orig.h"
#include "zge/com/zglrw/zrwramdisk.h"
#include "zge/zfile.h"

/* RenderWare's file interface: ten function pointers (open, close, read, write, gets, puts, eof, seek,
 * flush). */
typedef void *ZrwFileInterface[10];
#define gZrwSavedFileInterface ORIG_VAR(0x00676410, ZrwFileInterface, gZrwSavedFileInterface)
/* Storage the RAM disk info pointers point into while the callbacks are installed. */
typedef ZrwRamDiskInfo ZrwRamDiskInfoSlots[2];
#define gZrwRamDiskInfoSlots ORIG_VAR(0x006763ec, ZrwRamDiskInfoSlots, gZrwRamDiskInfoSlots)

/* State of the profiler (0x676438, 0x68 bytes, cleared at initialisation). */
typedef struct ZProfileState {
    int initialized;     /* 0x00 */
    void *entries;       /* 0x04 free list of 0x24 byte entries */
    void *updateNode;    /* 0x08 per-frame update registration */
    int32_t limit0c;     /* 0x0c 10 */
    uint32_t pad10;      /* 0x10 */
    int32_t limit14;     /* 0x14 copy of limit60 */
    float scale18;       /* 0x18 32.0 */
    float scale1c;       /* 0x1c 32.0 */
    float scale20;       /* 0x20 2.0 */
    uint8_t pad24[0x14]; /* 0x24 */
    int32_t limit38;     /* 0x38 0x20 */
    int32_t zero3c;      /* 0x3c */
    int32_t zero40;      /* 0x40 */
    int32_t limit44;     /* 0x44 8 */
    int32_t limit48;     /* 0x48 6 */
    int32_t limit4c;     /* 0x4c 2 */
    int32_t limit50;     /* 0x50 4 */
    int32_t limit54;     /* 0x54 8 */
    int32_t limit58;     /* 0x58 4 */
    int32_t limit5c;     /* 0x5c 2 */
    int32_t limit60;     /* 0x60 0x78 */
    int32_t limit64;     /* 0x64 0x3c */
} ZProfileState;

_Static_assert(sizeof(ZProfileState) == 0x68, "profile state");

#define gZProfile ORIG_VAR(0x00676438, ZProfileState, gZProfile)

/* Callbacks of the file interface; `file` is the slot RenderWare keeps the handle in. */
int zrwfclose(ZfileHandle **file);
int zrwfread(void *buffer, int size, int count, ZfileHandle **file);
int zrwfwrite(const void *buffer, int size, int count, ZfileHandle **file);
char *zrwfgets(char *buffer, int size, ZfileHandle **file);
int zrwfflush(void);
int zrwfputs(void);
int zrwfeof(ZfileHandle **file);
uint32_t zrwfseek(ZfileHandle **file, uint32_t a, uint32_t b);

/* Installs the callbacks into RenderWare's interface (keeping a copy of the old one), resets the RAM disk
 * state. */
void RegisterCallBacksForFileSystem(void);
/* Appends a memory block to the RAM disk info list. */
void ImageBufferSizeType(const void *address, uint32_t size, const char *extension);
/* Restores RenderWare's own file interface. */
void zrwFileRamDiskClear(void);

/* Per-frame profiler update and the entry destructor (0x4a4080, pcprofile.c): not ported. */
ORIG_FN(0x004a35f0, int, zprofileUpdate, (void *arg));
ORIG_FN(0x004a4080, void, pcProfileDestroyEntry, (void *entry));
ORIG_FN(0x004a45e0, int, pcProfileInitialize, (void));
/* RenderWare's file interface table. */
ORIG_FN(0x004e8280, void **, RwOSGetFileInterface, (void));

int zprofileInitialize(void);

#endif
