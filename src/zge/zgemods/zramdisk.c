/* RAM disk file device (C:\NgEngine\Zge\ZgeMods\zramdisk.c). */
#include "zge/zgemods/zramdisk.h"

#include "compat/crt.h"
#include "zge/zdebug.h"
#include "zge/zfile.h"
#include "zge/zmem.h"
#include "zge/zstring.h"

#if UINTPTR_MAX != 0xffffffffu
#include <stdlib.h>
#endif

static const char kRamdiskFile[] = "C:\\NgEngine\\Zge\\ZgeMods\\zramdisk.c";
static const char kNotInitialized[] = "File system has not been intialized.";
static const char kInvalidHandle[] = "Invalid RAM disk file handle";
static const char kNeedResultOrCallback[] = "Must specify at least either a return value or a call back";
static const char kNotSupported[] = "Function not supported on this device";

static void requireInitialized(int line)
{
    if (gZramdiskFiles == NULL) {
        zdebugFail(kNotInitialized, kRamdiskFile, line, 0);
    }
}

/* The RAM disk file behind a handle; fatal when the handle is empty or closed (data pointer 0). */
static ZramdiskFile *fileOf(ZfileHandle *file, int line)
{
    ZramdiskFile *rf = file->data;

    if (rf == NULL || rf->data == NULL) {
        zdebugFail(kInvalidHandle, kRamdiskFile, line, 0);
    }
    return rf;
}

BMX_ADDR(0x0049eaa0)
const char *zramdiskGetName(ZfileDevice *device)
{
    requireInitialized(0x9b);
    return gZramdiskDeviceName;
}

BMX_ADDR(0x0049ead0)
const char *zramdiskControl(ZfileDevice *device, uint32_t command, const char *const *keys,
                            const void *values)
{
    requireInitialized(0xbb);
    if (command == 2 && zstrcmp(keys[0], "ADDRESS") == 0 && zstrcmp(keys[1], "SIZE") == 0) {
        const uintptr_t *block = values;

        /* the original also checks the name buffer for NULL ("Invalid file name created for RAM disk") */
#if UINTPTR_MAX == 0xffffffffu
        zstrFormat(gZramdiskName, "rd00:%#10x:%#10x", block[0], block[1]);
#else
        zstrFormat(gZramdiskName, "rd00:%#18llx:%#10x", (unsigned long long)block[0], (uint32_t)block[1]);
#endif
        return gZramdiskName;
    }
    zfileRegisterErrorMessage("Invalid RAM disk file name attributes");
    return NULL;
}

/* Shared body of the four slot calls that only report success. */
static int completeSlot(uint32_t arg, int *result, const ZramdiskCallback *callback, int firstLine)
{
    requireInitialized(firstLine);
    if (result == NULL) {
        if (callback == NULL) {
            zdebugFail(kNeedResultOrCallback, kRamdiskFile, firstLine + 3, 0);
            return 1;
        }
    } else {
        *result = 1;
        if (callback == NULL) {
            return 1;
        }
    }
    {
        ZramdiskEvent event;

        event.arg = callback->arg;
        event.file = (void *)(uintptr_t)arg;
        event.ok = 1;
        event.value = 1;
        callback->fn(&event);
    }
    return 1;
}

BMX_ADDR(0x0049eb80)
int zramdiskSlot08(uint32_t arg, int *result, const ZramdiskCallback *callback)
{
    return completeSlot(arg, result, callback, 0xe6);
}

BMX_ADDR(0x0049ec20)
int zramdiskSlot0c(void)
{
    requireInitialized(0x108);
    zfileRegisterErrorMessage(kNotSupported);
    return 0;
}

BMX_ADDR(0x0049ec60)
int zramdiskSlot10(uint32_t arg, int *result, const ZramdiskCallback *callback)
{
    return completeSlot(arg, result, callback, 0x118);
}

BMX_ADDR(0x0049ed00)
int zramdiskSlot14(uint32_t a, uint32_t b, int *result, const ZramdiskCallback *callback)
{
    return completeSlot(a, result, callback, 0x13a);
}

BMX_ADDR(0x0049eda0)
int zramdiskSlot18(uint32_t a, uint32_t b, int *result, const ZramdiskCallback *callback)
{
    return completeSlot(a, result, callback, 0x15c);
}

BMX_ADDR(0x0049ee40)
int zramdiskSlot1c(void)
{
    requireInitialized(0x17e);
    zfileRegisterErrorMessage(kNotSupported);
    return 0;
}

BMX_ADDR(0x0049ee80)
int zramdiskOpen(ZfileHandle *file, const char *name, int mode, const ZramdiskCallback *callback)
{
    ZramdiskFile *rf;
    uintptr_t address;
    uint32_t size, flags;
    const char *colon;

    requireInitialized(0x193);
    if (name == NULL || mode >= 4 || file == NULL) {
        zdebugFail("Invalid parameters", kRamdiskFile, 0x194, 0);
    }
#if UINTPTR_MAX == 0xffffffffu
    address = (uint32_t)zstrtoul(name, 16);
#else
    address = (uintptr_t)strtoull(name, NULL, 16); /* zstrtoul's unsigned long is 32 bits on Windows */
#endif
    colon = zstrchr(name, ':');
    if (colon == NULL) {
        zfileRegisterErrorMessage("Invalid file name");
        return 0;
    }
    size = (uint32_t)zstrtoul(colon + 1, 16);
    if (size == 0 || address == 0) {
        zfileRegisterErrorMessage("Invalid file name given for RAM disk");
    }
    rf = zmemFreeListAlloc(gZramdiskFiles);
    if (rf == NULL) {
        zdebugFail("Error allocating RAM disk file handle", kRamdiskFile, 0x1b7, 0);
    }
    rf->pos = 0;
    rf->data = (uint8_t *)(uintptr_t)address;
    rf->size = size;
    /* bit 1 = mode bit 0 (read), bit 0 = mode bit 1 (write); the other bits keep the recycled node's */
    flags = ((uint32_t)((mode & 1) == 1) << 1) | (rf->flags & 0xfffffffdu);
    rf->flags = ((((mode & 2) == 2) ^ flags) & 1) ^ flags;
    file->data = rf;
    if (callback != NULL) {
        ZramdiskEvent event;

        event.arg = callback->arg;
        event.file = file;
        event.ok = 1;
        callback->fn(&event);
    }
    return 1;
}

BMX_ADDR(0x0049efd0)
int zramdiskClose(ZfileHandle *file, const ZramdiskCallback *callback)
{
    ZramdiskFile *rf;

    requireInitialized(0x1dc);
    rf = fileOf(file, 0x1df);
    rf->pos = 0;
    rf->size = 0;
    rf->data = NULL;
    zmemFreeListFree(gZramdiskFiles, rf);
    if (callback != NULL) {
        ZramdiskEvent event;

        event.arg = callback->arg;
        event.file = file;
        event.ok = 1;
        callback->fn(&event);
    }
    return 1;
}

/* Shared body of read and write: clamps the count to the end of the block and copies. */
static int transfer(ZfileHandle *file, void *buffer, int count, const ZramdiskCallback *callback,
                    int toBuffer, uint32_t needFlag, int firstLine)
{
    ZramdiskFile *rf;
    int n;

    requireInitialized(firstLine);
    rf = fileOf(file, firstLine + 3);
    if ((rf->flags & needFlag) == 0) {
        zfileRegisterErrorMessage("File access violation");
        return 0;
    }
    n = count;
    if (rf->size < rf->pos + (uint32_t)count) {
        n = (int)(rf->size - rf->pos);
    }
    if (n == 0) {
        return 0;
    }
    if (toBuffer) {
        zmemCopy(buffer, rf->data + rf->pos, (uint32_t)n);
    } else {
        zmemCopy(rf->data + rf->pos, buffer, (uint32_t)n);
    }
    rf->pos += (uint32_t)n;
    if (callback != NULL) {
        ZramdiskEvent event;

        event.arg = callback->arg;
        event.file = file;
        event.ok = n == count;
        event.value = (uint32_t)n;
        callback->fn(&event);
    }
    return n;
}

BMX_ADDR(0x0049f080)
int zramdiskRead(ZfileHandle *file, void *buffer, int count, const ZramdiskCallback *callback)
{
    return transfer(file, buffer, count, callback, 1, 2, 0x1ff);
}

BMX_ADDR(0x0049f170)
int zramdiskWrite(ZfileHandle *file, const void *buffer, int count, const ZramdiskCallback *callback)
{
    return transfer(file, (void *)(uintptr_t)buffer, count, callback, 0, 1, 0x234);
}

BMX_ADDR(0x0049f260)
int zramdiskSeek(ZfileHandle *file, uint32_t offset, int mode, const ZramdiskCallback *callback)
{
    ZramdiskFile *rf;

    requireInitialized(0x26b);
    rf = fileOf(file, 0x26d);
    if (mode == 0) {
        if (rf->size < offset) {
            zfileRegisterErrorMessage("Seek past end of file");
            return 0;
        }
        rf->pos = offset;
    } else if (mode == 1) {
        offset = rf->pos + offset;
        if (rf->size < offset) {
            zfileRegisterErrorMessage("Seek past end of file");
            return 0;
        }
        rf->pos = offset;
    } else if (mode == 2) {
        rf->pos = rf->size - offset;
    } else {
        zdebugFail("Invalid seek mode", kRamdiskFile, 0x298, 0);
    }
    if (callback != NULL) {
        ZramdiskEvent event;

        event.arg = callback->arg;
        event.file = file;
        event.ok = 1;
        event.value = rf->pos;
        callback->fn(&event);
    }
    return 1;
}

BMX_ADDR(0x0049f370)
int zramdiskTell(ZfileHandle *file, uint32_t *position)
{
    requireInitialized(0x2b4);
    *position = fileOf(file, 0x2b6)->pos;
    return 1;
}

BMX_ADDR(0x0049f3d0)
int zramdiskGetSize(ZfileHandle *file, uint32_t *size)
{
    ZramdiskFile *rf = file->data;

    requireInitialized(0x2c8);
    if (size == NULL) {
        zdebugFail("Invalid pointer for file size", kRamdiskFile, 0x2ca, 0);
    }
    if (rf == NULL || rf->data == NULL) {
        zdebugFail(kInvalidHandle, kRamdiskFile, 0x2cb, 0);
    }
    *size = rf->size;
    return 1;
}

BMX_ADDR(0x0049f450)
int zramdiskInitialize(void)
{
    if (gZramdiskFiles != NULL) {
        zdebugFail("File system has already been intialized.", kRamdiskFile, 0x2df, 0);
    }
    gZramdiskFiles = zmemFreeListCreate(sizeof(ZramdiskFile), 0x10, NULL);
    return gZramdiskFiles != NULL;
}
