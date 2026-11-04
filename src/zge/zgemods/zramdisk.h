/*
 * RAM disk file device (C:\NgEngine\Zge\ZgeMods\zramdisk.c). Memory blocks are opened as files named
 * "rd00:<address>:<size>"; the functions below fill the device table at 0x563240 (name, control, six
 * slots, open, close, read, write, seek, tell, size). Calls may carry a completion callback, which is
 * invoked at once with an event block.
 */
#ifndef ZGE_ZGEMODS_ZRAMDISK_H
#define ZGE_ZGEMODS_ZRAMDISK_H

#include <stdint.h>

#include "compat/orig.h"
#include "zge/zfile.h"
#include "zge/zmem.h"

/* Completion callback of a device call: the handler and the argument echoed in the event. */
typedef struct ZramdiskCallback {
    void(BMX_CDECL *fn)(void *event);
    uint32_t arg;
} ZramdiskCallback;

typedef struct ZramdiskEvent {
    uint32_t arg;
    void *file; /* the handle (or the first argument of a slot call) */
    uint32_t ok;
    uint32_t value; /* bytes moved / new position (slot calls: 1); only set by read, write and seek */
} ZramdiskEvent;

/* Open RAM disk file (0x10 bytes from a free list). */
typedef struct ZramdiskFile {
    uint32_t pos;
    uint32_t size;
    uint32_t flags; /* bit 1: readable, bit 0: writable */
    uint8_t *data;
} ZramdiskFile;

BMX_LAYOUT32_ASSERT(sizeof(ZramdiskFile) == 0x10, "RAM disk file");

enum { kZramdiskOpenRead = 1, kZramdiskOpenWrite = 2 };

/* Free list of open files; NULL until zramdiskInitialize ran. */
#define gZramdiskFiles ORIG_VAR(0x0067536c, ZMemFreeList *, gZramdiskFiles)
/* File name built by zramdiskControl, and the device name "rd00". */
typedef char ZramdiskNameBuffer[0x80];
typedef char ZramdiskDeviceName[8];
#define gZramdiskName       ORIG_VAR(0x006752ec, ZramdiskNameBuffer, gZramdiskName)
#define gZramdiskDeviceName ORIG_VAR(0x0055bf38, ZramdiskDeviceName, gZramdiskDeviceName)

/* Device name ("rd00"). */
const char *zramdiskGetName(ZfileDevice *device);
/*
 * Command 2 with the attributes ADDRESS and SIZE (`values` is a uintptr_t[2]): the file name for a memory
 * block, else NULL. 32-bit builds write both as %#10x like the original; 64-bit builds need 16 digits for the
 * address (docs/PORTING.md "X2e").
 */
const char *zramdiskControl(ZfileDevice *device, uint32_t command, const char *const *keys,
                            const void *values);
/* Slots 0x08, 0x10, 0x14 and 0x18 complete at once (a result and/or callback is required). */
int zramdiskSlot08(uint32_t arg, int *result, const ZramdiskCallback *callback);
int zramdiskSlot10(uint32_t arg, int *result, const ZramdiskCallback *callback);
int zramdiskSlot14(uint32_t a, uint32_t b, int *result, const ZramdiskCallback *callback);
int zramdiskSlot18(uint32_t a, uint32_t b, int *result, const ZramdiskCallback *callback);
/* Slots 0x0c and 0x1c: "Function not supported on this device". */
int zramdiskSlot0c(void);
int zramdiskSlot1c(void);
int zramdiskOpen(ZfileHandle *file, const char *name, int mode, const ZramdiskCallback *callback);
int zramdiskClose(ZfileHandle *file, const ZramdiskCallback *callback);
/* Copies from the memory block into buffer; returns the bytes copied (clamped to the end). */
int zramdiskRead(ZfileHandle *file, void *buffer, int count, const ZramdiskCallback *callback);
int zramdiskWrite(ZfileHandle *file, const void *buffer, int count, const ZramdiskCallback *callback);
/* mode 0: from the start, 1: from the position, 2: back from the end. */
int zramdiskSeek(ZfileHandle *file, uint32_t offset, int mode, const ZramdiskCallback *callback);
int zramdiskTell(ZfileHandle *file, uint32_t *position);
int zramdiskGetSize(ZfileHandle *file, uint32_t *size);
/* Creates the file list; fatal when called twice. */
int zramdiskInitialize(void);

#endif
