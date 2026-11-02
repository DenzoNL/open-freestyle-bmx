/*
 * Files (C:\NgEngine\Zge\zfile.c): a device layer over the platform file systems. Devices (the
 * RenderWare file interface "rw00", the RAM disk "rd00") are tables of function pointers; a file handle
 * is an 8 byte slot from a free list that remembers its device. Paths may start with "<device>:".
 */
#ifndef ZGE_ZFILE_H
#define ZGE_ZFILE_H

#include <stdint.h>

#include "compat/orig.h"
#include "zge/zmem.h"

struct ZfileDevice;

typedef struct ZfileHandle {
    void *data;                 /* 0x00 device private, cleared on open */
    struct ZfileDevice *device; /* 0x04 */
} ZfileHandle;

/* Device function table. Slots 0x08..0x1f are unused by this module. */
typedef struct ZfileDevice {
    const char *(BMX_CDECL *name)(struct ZfileDevice *device); /* 0x00 */
    uint32_t(BMX_CDECL *control)(struct ZfileDevice *device, uint32_t command, uint32_t a,
                                 uint32_t b); /* 0x04 */
    uint8_t pad08[0x18];
    int(BMX_CDECL *open)(ZfileHandle *file, const char *path, int mode, int flags);     /* 0x20 */
    int(BMX_CDECL *close)(ZfileHandle *file, int flags);                                /* 0x24 */
    int(BMX_CDECL *read)(ZfileHandle *file, void *buffer, int count, int flags);        /* 0x28 */
    int(BMX_CDECL *write)(ZfileHandle *file, const void *buffer, int count, int flags); /* 0x2c */
    uint32_t(BMX_CDECL *op30)(ZfileHandle *file, uint32_t a, uint32_t b, uint32_t c);   /* 0x30 */
    uint32_t(BMX_CDECL *op34)(ZfileHandle *file, uint32_t a, uint32_t b);               /* 0x34 */
    uint32_t(BMX_CDECL *getSize)(ZfileHandle *file, int *size, uint32_t flags);         /* 0x38 */
} ZfileDevice;

BMX_LAYOUT32_ASSERT(__builtin_offsetof(ZfileDevice, open) == 0x20, "device layout");
BMX_LAYOUT32_ASSERT(__builtin_offsetof(ZfileDevice, getSize) == 0x38, "device layout");

enum { kZfileMaxDevices = 16, kZfileErrorBytes = 0x40, kZfileNameBytes = 0x80 };

/* Device count followed by the device pointers (filled in by the platform's FileSystemsInitialize). */
typedef struct ZfileDeviceTable {
    uint32_t count;
    ZfileDevice *device[kZfileMaxDevices];
} ZfileDeviceTable;

typedef char ZfileErrorBuffer[kZfileErrorBytes];
typedef char ZfileNameBuffer[kZfileNameBytes];

#define gZfileErrorMessage  ORIG_VAR(0x00670528, ZfileErrorBuffer, gZfileErrorMessage)
#define gZfileDefaultDevice ORIG_VAR(0x00670568, ZfileDevice *, gZfileDefaultDevice)
#define gZfileNameBuffer    ORIG_VAR(0x006705ac, ZfileNameBuffer, gZfileNameBuffer)
#define gZfileDevices       ORIG_VAR(0x00670630, ZfileDeviceTable, gZfileDevices)
#define gZfileInitialized   ORIG_VAR(0x00670674, int, gZfileInitialized)
#define gZfileHandles       ORIG_VAR(0x00670678, ZMemFreeList *, gZfileHandles)
/* Pool the loaders allocate their buffers from, or NULL for the main heap. */
#define gZfileLoadPool ORIG_VAR(0x0067067c, ZMemPool *, gZfileLoadPool)
/* The RAM disk device once found (-1 until then). */
#define gZfileRamDisk ORIG_VAR(0x0056062c, intptr_t, gZfileRamDisk)

/* Remembers msg as the last error (when enabled by a non-empty buffer) and prints it. */
void zfileRegisterErrorMessage(const char *msg);
/* Sets up the devices and the handle allocator; returns 1, or 0 when the platform setup fails. */
int zfileInitialize(void);
/* Free list destructor of a handle: closes it through its device. */
int zfileDestroyHandle(ZfileHandle *file);
ZfileDeviceTable *zfileGetDevices(void);
const char *zfileDeviceName(ZfileDevice *device);
uint32_t zfileDeviceControl(ZfileDevice *device, uint32_t command, uint32_t a, uint32_t b);
/* Device named by the first four characters of the path, else the default device. */
ZfileDevice *zfileFindDevice(const char *path);
ZfileDevice *zfileDefaultDevice(void);
/* Opens a file by path; returns a handle or NULL. */
ZfileHandle *zfileOpen(const char *path, int mode, int flags);
int zfileClose(ZfileHandle *file, int flags);
int zfileRead(ZfileHandle *file, void *buffer, int count, int flags);
/* Writes count bytes; returns the number written. */
int zfileWrite(ZfileHandle *file, const void *buffer, int count, int flags);
uint32_t zfileOp30(ZfileHandle *file, uint32_t a, uint32_t b, uint32_t c);
uint32_t zfileOp34(ZfileHandle *file, uint32_t a, uint32_t b);
uint32_t zfileGetSize(ZfileHandle *file, int *size, uint32_t flags);
/* Reads a whole file into a new buffer (see gZfileLoadPool); stores its size when sizeOut is set. */
void *zfileLoad(const char *path, int *sizeOut);
/* Reads the first `size` bytes of a file into a new buffer. */
void *zfileLoadBytes(const char *path, int size);
void zfileFreeBuffer(void *buffer);
/* Reads one line (up to a newline or size - 1 bytes); returns buffer, or NULL when nothing was read. */
char *zfileGets(ZfileHandle *file, char *buffer, int size);
/* dir + file in a shared 128 byte buffer. */
char *zfileBuildFileName(const char *dir, const char *file);
/* Copies the part of path up to and including the last backslash; NULL for long or missing paths. */
char *zfileGetDirectory(const char *path, char *out);
/* Copies the extension (fewer than four characters after the last dot); NULL otherwise. */
char *zfileGetExtension(const char *path, char *out);
/* Opens the RAM disk file for a memory block (address, size); returns a handle or NULL. */
ZfileHandle *zfileOpenRamDisk(uint32_t address, uint32_t size);

#endif
