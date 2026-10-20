/* Files (C:\NgEngine\Zge\zfile.c). */
#include "zge/zfile.h"

#include "compat/crt.h"
#include "zge/com/zglrw/zrwzgl.h"
#include "zge/zdebug.h"
#include "zge/zmem.h"
#include "zge/zstring.h"

static const char zfileFile[] = "C:\\NgEngine\\Zge\\zfile.c";

static const char kNotInitialized[] = "File system has not been intialized.";
static const char kInvalidFile[] = "Invalid file handle";
static const char kInvalidDevice[] = "An invalid device identifier was given";

/* Fires the "not initialized" assert shared by all entry points (the line differs per caller). */
static void requireInitialized(int line)
{
    if (gZfileInitialized == 0) {
        zdebugFail(kNotInitialized, zfileFile, line, 0);
    }
}

/* Asserts that the handle has a device. */
static void requireDevice(const ZfileHandle *file, int line)
{
    if (file->device == 0) {
        zdebugFail(kInvalidFile, zfileFile, line, 0);
    }
}

BMX_ADDR(0x00483a60)
void zfileRegisterErrorMessage(const char *msg)
{
    if (gZfileErrorMessage[0] != '\0' && (uint32_t)zstrlen(msg) < kZfileErrorBytes - 1) {
        zstrcpy(gZfileErrorMessage, msg);
    }
    gZdebugPrint("zfileRegisterErrorMessage(): %s\n", msg);
}

BMX_ADDR(0x00483aa0)
int zfileInitialize(void)
{
    ZfileDevice *first;

    if (zrwFileSystemsInitialize(&gZfileDevices) == 0) {
        zdebugFail("Error initializing platform file system", zfileFile, 237, 0);
        return 0;
    }
    if (gZfileDevices.count == 0) {
        zdebugFail("There are no devices connected to the system.", zfileFile, 240, 0);
    }
    if (gZfileHandles != 0) {
        zdebugFail("File system should not be initialized", zfileFile, 242, 0);
    }
    gZfileHandles =
        zmemFreeListCreate(sizeof(ZfileHandle), 8, BMX_FNPTR(ZMemDestroyFn, 0x00483b80, zfileDestroyHandle));
    first = gZfileDevices.device[0];
    if (gZfileHandles == 0) {
        zdebugFail("Failed to create file handle allocator", zfileFile, 250, 0);
        return 0;
    }
    gZfileInitialized = 1;
    if (first == 0) {
        zdebugFail(kInvalidDevice, zfileFile, 408, 0);
    }
    gZfileDefaultDevice = first;
    return 1;
}

BMX_ADDR(0x00483b80)
int zfileDestroyHandle(ZfileHandle *file)
{
    requireInitialized(640);
    requireDevice(file, 641);
    if (file->device->close(file, 0) == 0) {
        zfileRegisterErrorMessage("Error when trying to close file through device.");
        return 1;
    }
    zmemFreeListFree(gZfileHandles, file);
    return 1;
}

BMX_ADDR(0x00483cb0)
ZfileDeviceTable *zfileGetDevices(void)
{
    requireInitialized(291);
    return &gZfileDevices;
}

BMX_ADDR(0x00483ce0)
const char *zfileDeviceName(ZfileDevice *device)
{
    requireInitialized(307);
    if (device == 0) {
        zdebugFail(kInvalidDevice, zfileFile, 308, 0);
    }
    return device->name(device);
}

BMX_ADDR(0x00483d30)
uint32_t zfileDeviceControl(ZfileDevice *device, uint32_t command, uint32_t a, uint32_t b)
{
    requireInitialized(333);
    if (device == 0) {
        zdebugFail(kInvalidDevice, zfileFile, 334, 0);
    }
    return device->control(device, command, a, b);
}

/*
 * Default device lookup used when a path names none; the original keeps asserting when the default
 * device is missing and carries on with whatever is stored.
 */
static ZfileDevice *defaultDevice(void)
{
    requireInitialized(390);
    if (gZfileDefaultDevice == 0) {
        zdebugFail("The default device is invalid", zfileFile, 391, 0);
    }
    return gZfileDefaultDevice;
}

BMX_ADDR(0x00483d90)
ZfileDevice *zfileFindDevice(const char *path)
{
    char buffer[8];

    requireInitialized(354);
    if (path == 0) {
        zdebugFail("Invalid parameter", zfileFile, 355, 0);
    }
    if (zstrlen(path) > 3) {
        uint32_t i;

        for (i = 0; i < gZfileDevices.count; i++) {
            const char *name = zfileDeviceName(gZfileDevices.device[i]);

            if (name != 0) {
                zstrcpy(buffer, name);
                zstrcat(buffer, ":");
                if (zstrncmp(path, buffer, 4) == 0) {
                    if (i == 0) {
                        zdebugFail("Device list may be corrupt", zfileFile, 370, 0);
                    }
                    return gZfileDevices.device[i];
                }
            }
        }
    }
    return defaultDevice();
}

BMX_ADDR(0x00483f10)
ZfileDevice *zfileDefaultDevice(void)
{
    return defaultDevice();
}

BMX_ADDR(0x00484700)
ZfileHandle *zfileOpen(const char *path, int mode, int flags)
{
    ZfileDevice *device;
    const char *name = path;
    const char *deviceName;
    ZfileHandle *file;

    requireInitialized(596);
    if ((uint32_t)zstrlen(path) >= 0x80) {
        zdebugFail("An invalid file name was given", zfileFile, 598, 0);
    }
    if (path == 0) {
        zdebugFail("Invalid paramters", zfileFile, 152, 0);
    }
    device = zfileFindDevice(path);
    if (device == 0) {
        zdebugFail("Could not determine device for file name.", zfileFile, 155, 0);
    }
    deviceName = zfileDeviceName(device);
    if (deviceName == 0) {
        name = 0;
    } else {
        if (zstrlen(deviceName) != 4) {
            zdebugFail("Device name is not valid", zfileFile, 165, 0);
        }
        if (zstrncmp(name, deviceName, 4) == 0 && name[4] == ':') {
            name += 5;
        }
    }
    if (name == 0) {
        zdebugFail("Failed to identify device from file name", zfileFile, 601, 0);
    }
    if (gZfileHandles == 0) {
        zdebugFail("File system should be initialized.", zfileFile, 604, 0);
    }
    file = zmemFreeListAlloc(gZfileHandles);
    file->data = 0;
    file->device = device;
    if (device->open(file, name, mode, flags) != 0) {
        return file;
    }
    zfileRegisterErrorMessage("Error opening file.");
    zmemFreeListFree(gZfileHandles, file);
    return 0;
}

BMX_ADDR(0x00484a60)
int zfileClose(ZfileHandle *file, int flags)
{
    requireInitialized(640);
    requireDevice(file, 641);
    if (file->device->close(file, flags) == 0) {
        zfileRegisterErrorMessage("Error when trying to close file through device.");
        return 0;
    }
    zmemFreeListFree(gZfileHandles, file);
    return 1;
}

BMX_ADDR(0x00484b20)
int zfileRead(ZfileHandle *file, void *buffer, int count, int flags)
{
    requireInitialized(663);
    if (count == 0) {
        return 0;
    }
    requireDevice(file, 668);
    return file->device->read(file, buffer, count, flags);
}

BMX_ADDR(0x00484b90)
int zfileWrite(ZfileHandle *file, const void *buffer, int count, int flags)
{
    requireInitialized(683);
    if (count == 0) {
        return 0;
    }
    requireDevice(file, 688);
    return file->device->write(file, buffer, count, flags);
}

BMX_ADDR(0x00484c00)
uint32_t zfileOp30(ZfileHandle *file, uint32_t a, uint32_t b, uint32_t c)
{
    requireInitialized(703);
    requireDevice(file, 704);
    return file->device->op30(file, a, b, c);
}

BMX_ADDR(0x00484c70)
uint32_t zfileOp34(ZfileHandle *file, uint32_t a, uint32_t b)
{
    requireInitialized(719);
    requireDevice(file, 720);
    return file->device->op34(file, a, b);
}

BMX_ADDR(0x00484cd0)
uint32_t zfileGetSize(ZfileHandle *file, int *size, uint32_t flags)
{
    requireInitialized(735);
    requireDevice(file, 736);
    return file->device->getSize(file, size, flags);
}

BMX_ADDR(0x00484d30)
void *zfileLoad(const char *path, int *sizeOut)
{
    void *result = 0;
    ZfileHandle *file;
    int size;

    file = zfileOpen(path, 1, 0);
    if (file == 0) {
        return 0;
    }
    if (zfileGetSize(file, &size, 0) == 1) {
        void *buffer =
            gZfileLoadPool == 0 ? zmemAlloc((uint32_t)size) : zmemPoolAlloc(gZfileLoadPool, (uint32_t)size);

        if (buffer != 0) {
            if (zfileRead(file, buffer, size, 0) == size) {
                result = buffer;
                if (sizeOut != 0) {
                    *sizeOut = size;
                }
            } else {
                zmemFree(buffer); /* even when the buffer came from the pool */
            }
        }
    }
    zfileClose(file, 0);
    return result;
}

BMX_ADDR(0x00485070)
void *zfileLoadBytes(const char *path, int size)
{
    void *result = 0;
    ZfileHandle *file;
    void *buffer;

    file = zfileOpen(path, 1, 0);
    if (file == 0) {
        return 0;
    }
    buffer = gZfileLoadPool == 0 ? zmemAlloc((uint32_t)size) : zmemPoolAlloc(gZfileLoadPool, (uint32_t)size);
    if (buffer != 0) {
        result = buffer;
        if (zfileRead(file, buffer, size, 0) != size) {
            zmemFree(buffer);
            result = 0;
        }
    }
    zfileClose(file, 0);
    return result;
}

BMX_ADDR(0x00485370)
void zfileFreeBuffer(void *buffer)
{
    if (gZfileLoadPool != 0) {
        zmemPoolFree(gZfileLoadPool, buffer);
        return;
    }
    zmemFree(buffer);
}

BMX_ADDR(0x004853c0)
char *zfileGets(ZfileHandle *file, char *buffer, int size)
{
    char last = '\0';
    char *p = buffer;
    uint32_t left;

    if (size == 0) {
        return 0;
    }
    left = (uint32_t)size - 1;
    do {
        if (left == 0) {
            break;
        }
        if (zfileRead(file, p, 1, 0) == 1) {
            last = *p++;
            left--;
        } else {
            left = 0;
        }
    } while (last != '\n');
    *p = '\0';
    return *buffer != '\0' ? buffer : 0;
}

BMX_ADDR(0x00485460)
char *zfileBuildFileName(const char *dir, const char *file)
{
    zstrcpy(gZfileNameBuffer, dir);
    zstrcat(gZfileNameBuffer, file);
    if ((uint32_t)zstrlen(gZfileNameBuffer) >= kZfileNameBytes) {
        zdebugFail("Memory corruption when building path name", zfileFile, 992, 0);
    }
    return gZfileNameBuffer;
}

BMX_ADDR(0x004854d0)
char *zfileGetDirectory(const char *path, char *out)
{
    int i;

    if (path == 0) {
        return 0;
    }
    i = zstrlen(path);
    if (i >= 0x80) {
        return 0;
    }
    while (i >= 0 && path[i] != '\\') {
        i--;
    }
    zstrncpy(out, path, (size_t)(i + 1));
    out[i + 1] = '\0';
    return out;
}

BMX_ADDR(0x00485520)
char *zfileGetExtension(const char *path, char *out)
{
    int i;
    const char *ext;

    if (path == 0) {
        return 0;
    }
    i = zstrlen(path);
    if (i >= 0x80) {
        return 0;
    }
    while (i >= 0 && path[i] != '.') {
        i--;
    }
    ext = path + i + 1;
    if ((uint32_t)zstrlen(ext) >= 4) {
        return 0;
    }
    zstrcpy(out, ext);
    return out;
}

BMX_ADDR(0x00485580)
ZfileHandle *zfileOpenRamDisk(uint32_t address, uint32_t size)
{
    ZfileDeviceTable *table;
    uint32_t i;
    const char *keys[2];
    uint32_t values[2];
    const char *name;
    ZfileDevice *ramDisk;

    requireInitialized(291);
    table = &gZfileDevices;
    for (i = 0; i < table->count; i++) {
        if (zstrcmp(zfileDeviceName(table->device[i]), "rd00") == 0) {
            gZfileRamDisk = (intptr_t)table->device[i];
            break;
        }
    }
    if (gZfileRamDisk == -1) {
        zdebugFail("Could not locate a RAM disk device on this system!", zfileFile, 1102, 0);
    }
    ramDisk = (ZfileDevice *)gZfileRamDisk;
    values[0] = address;
    values[1] = size;
    keys[0] = "ADDRESS";
    keys[1] = "SIZE";
    name = (const char *)(uintptr_t)zfileDeviceControl(ramDisk, 2, (uint32_t)(uintptr_t)keys,
                                                       (uint32_t)(uintptr_t)values);
    if (name == 0) {
        zdebugFail("Could not generate file name for RAM disk", zfileFile, 1113, 0);
        return 0;
    }
    return zfileOpen(name, 1, 0);
}
