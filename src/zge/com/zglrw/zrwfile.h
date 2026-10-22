/*
 * RenderWare file system device "rw00" (C:\NgEngine\Zge\Com\ZglRw\zrwFile.c): the table at 0x563050 hands
 * these functions to RenderWare's file system layer, and they forward to the OS file interface it saved at
 * start-up. Every operation can report back through a callback record {function, argument}.
 */
#ifndef ZGE_COM_ZGLRW_ZRWFILE_H
#define ZGE_COM_ZGLRW_ZRWFILE_H

#include <stdint.h>

#include "compat/orig.h"

/* What a callback is called with (a pointer to it); operations fill the first 3 or all 4 words. */
typedef struct ZrwFileEvent {
    uint32_t arg;   /* 0x00 the callback record's argument */
    void *handle;   /* 0x04 the operation's first parameter */
    uint32_t ok;    /* 0x08 1 on success */
    uint32_t value; /* 0x0c result: a flag, a byte count, a position or a size */
} ZrwFileEvent;

typedef struct ZrwFileCallback {
    void(BMX_CDECL *fn)(ZrwFileEvent *event);
    uint32_t arg;
} ZrwFileCallback;

/* The handle a RenderWare file object holds. */
typedef struct ZrwFileHandle {
    void *file;
} ZrwFileHandle;

/* RenderWare's OS file interface as copied at initialisation (0x6752c0, ten function pointers). */
typedef struct ZrwFileOps {
    int(BMX_CDECL *exists)(const char *name);                                                  /* 0x00 */
    void *(BMX_CDECL *open)(const char *name, const char *mode);                               /* 0x04 */
    int(BMX_CDECL *close)(void *file);                                                         /* 0x08 */
    uint32_t(BMX_CDECL *read)(void *buffer, uint32_t size, uint32_t count, void *file);        /* 0x0c */
    uint32_t(BMX_CDECL *write)(const void *buffer, uint32_t size, uint32_t count, void *file); /* 0x10 */
    void *pad14[3];                                                                            /* 0x14 */
    int(BMX_CDECL *seek)(void *file, int32_t offset, int whence);                              /* 0x20 */
    void *pad24;                                                                               /* 0x24 */
} ZrwFileOps;

_Static_assert(sizeof(ZrwFileOps) == 0x28, "file ops");

#define gZrwFileOps         ORIG_VAR(0x006752c0, ZrwFileOps, gZrwFileOps)
#define gZrwFileInitialized ORIG_VAR(0x006752e8, int, gZrwFileInitialized)

/* The device name "rw00" (asserts unless the file system is initialised). */
const char *zrwFileGetDeviceName(void);
/* Resolves the attributes of a RenderWare file name: kind 1 with key "PATH" copies names[0]'s value into a
 * static buffer and returns it, anything else reports an error and returns NULL. */
char *zrwFileResolveName(void *unused, int kind, const char **keys, const char **values);
/* Operations that only succeed: set *done to 1 and call back. */
int zrwFileSucceedA(void *unused, uint32_t *done, const ZrwFileCallback *callback);
int zrwFileSucceedB(void *unused, uint32_t *done, const ZrwFileCallback *callback);
/* Operations the device does not support: report an error and return 0. */
int zrwFileNotPermitted(void);
int zrwFileNotImplementedA(void);
int zrwFileNotImplementedB(void);
int zrwFileNotImplementedC(void);
int zrwFileExists(void *unused, const char *name, uint32_t *exists, const ZrwFileCallback *callback);
int zrwFileOpen(ZrwFileHandle *handle, const char *name, int mode, const ZrwFileCallback *callback);
int zrwFileClose(ZrwFileHandle *handle, const ZrwFileCallback *callback);
int zrwFileRead(ZrwFileHandle *handle, void *buffer, int count, const ZrwFileCallback *callback);
int zrwFileWrite(ZrwFileHandle *handle, const void *buffer, int count, const ZrwFileCallback *callback);
int zrwFileSeek(ZrwFileHandle *handle, uint32_t offset, int whence, const ZrwFileCallback *callback);
int zrwFileGetSize(ZrwFileHandle *handle, uint32_t *size, const ZrwFileCallback *callback);
void zrwFileSystemInitialize(void);
int zrwFileSystemShutdown(void);

#endif
