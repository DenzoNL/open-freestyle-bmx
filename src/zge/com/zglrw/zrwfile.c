/* RenderWare file system device "rw00" (see zrwfile.h). */
#include "zge/com/zglrw/zrwfile.h"

#include "compat/crt.h"
#include "zge/zdebug.h"
#include "zge/zfile.h"
#include "zge/zprofile.h"
#include "zge/zstring.h"

/* The C library's fseek / ftell on RenderWare file objects (statically linked in the exe). */
ORIG_FN(0x004a61ad, int, rwfseek, (void *file, int32_t offset, int whence));
ORIG_FN(0x004a6239, int32_t, rwftell, (void *file));

static const char kZrwFileFile[] = "C:\\NgEngine\\Zge\\Com\\ZglRw\\zrwFile.c";
static const char kNotInitialized[] = "File system has not been intialized.";
static const char kNeedResultOrCallback[] = "Must specify at least either a return value or a call back";
static const char kInvalidHandle[] = "Invalid file handle";

typedef char ZrwDeviceName[8];
#define gZrwDeviceName ORIG_VAR(0x00563050, ZrwDeviceName, gZrwDeviceName)
/* Key of the file name attribute ("PATH"). */
typedef char ZrwKeyName[8];
#define gZrwPathKey ORIG_VAR(0x00562130, ZrwKeyName, gZrwPathKey)
/* Name returned by zrwFileResolveName. */
typedef char ZrwNameBuffer[0x80];
#define gZrwNameBuffer ORIG_VAR(0x00675240, ZrwNameBuffer, gZrwNameBuffer)
/* fopen mode strings by access kind, and the seek origins by mode. */
typedef const char *ZrwOpenModes[4];
#define gZrwOpenModes ORIG_VAR(0x00563038, ZrwOpenModes, gZrwOpenModes)
typedef uint32_t ZrwSeekOrigins[3];
#define gZrwSeekOrigins ORIG_VAR(0x0054e130, ZrwSeekOrigins, gZrwSeekOrigins)

static void requireInitialized(int line)
{
    if (gZrwFileInitialized == 0) {
        zdebugFail(kNotInitialized, kZrwFileFile, line, 0);
    }
}

BMX_ADDR(0x0049e1f0)
const char *zrwFileGetDeviceName(void)
{
    requireInitialized(0x7c);
    return gZrwDeviceName;
}

BMX_ADDR(0x0049e220)
char *zrwFileResolveName(void *unused, int kind, const char **keys, const char **values)
{
    requireInitialized(0x91);
    if (kind == 1 && zstrcmp(keys[0], gZrwPathKey) == 0) {
        const char *name = values[0];

        if (name == NULL) {
            zdebugFail("Invalid file name given", kZrwFileFile, 0x98, 0);
        }
        zstrcpy(gZrwNameBuffer, name);
        return gZrwNameBuffer;
    }
    zfileRegisterErrorMessage("Invalid attributes for RenderWare file name");
    return NULL;
}

/* Sets *done and calls back; the two operations differ only in the assert lines. */
static int succeed(void *handle, uint32_t *done, const ZrwFileCallback *callback, int lineInit, int lineArgs)
{
    ZrwFileEvent event;

    requireInitialized(lineInit);
    if (done == NULL) {
        if (callback == NULL) {
            zdebugFail(kNeedResultOrCallback, kZrwFileFile, lineArgs, 0);
            return 1;
        }
    } else {
        *done = 1;
        if (callback == NULL) {
            return 1;
        }
    }
    event.arg = callback->arg;
    event.handle = handle;
    event.ok = 1;
    event.value = 1;
    callback->fn(&event);
    return 1;
}

BMX_ADDR(0x0049e2b0)
int zrwFileSucceedA(void *unused, uint32_t *done, const ZrwFileCallback *callback)
{
    return succeed(unused, done, callback, 0xab, 0xae);
}

BMX_ADDR(0x0049e390)
int zrwFileSucceedB(void *unused, uint32_t *done, const ZrwFileCallback *callback)
{
    return succeed(unused, done, callback, 0xdd, 0xe0);
}

BMX_ADDR(0x0049e350)
int zrwFileNotPermitted(void)
{
    requireInitialized(0xcd);
    zfileRegisterErrorMessage("Function not permitted on this device");
    return 0;
}

BMX_ADDR(0x0049e430)
int zrwFileNotImplementedA(void)
{
    requireInitialized(0xff);
    zfileRegisterErrorMessage("Function not implemented for this device");
    return 0;
}

BMX_ADDR(0x0049e510)
int zrwFileNotImplementedB(void)
{
    requireInitialized(0x133);
    zfileRegisterErrorMessage("Function not implemented for this device");
    return 0;
}

BMX_ADDR(0x0049e910)
int zrwFileNotImplementedC(void)
{
    requireInitialized(0x208);
    zfileRegisterErrorMessage("Function not implemented for this device");
    return 0;
}

BMX_ADDR(0x0049e470)
int zrwFileExists(void *unused, const char *name, uint32_t *exists, const ZrwFileCallback *callback)
{
    ZrwFileEvent event;

    requireInitialized(0x110);
    if (exists == NULL && callback == NULL) {
        zdebugFail(kNeedResultOrCallback, kZrwFileFile, 0x113, 0);
    }
    event.value = gZrwFileOps.rwfexist(name) != 0;
    if (exists != NULL) {
        *exists = event.value;
    }
    if (callback != NULL) {
        event.arg = callback->arg;
        event.handle = unused;
        event.ok = 1;
        callback->fn(&event);
    }
    return 1;
}

BMX_ADDR(0x0049e550)
int zrwFileOpen(ZrwFileHandle *handle, const char *name, int mode, const ZrwFileCallback *callback)
{
    ZrwFileEvent event;
    char path[128];
    void *file;

    requireInitialized(0x14c);
    if (mode > 3 || handle == NULL || name == NULL) {
        zdebugFail("Invalid parameters", kZrwFileFile, 0x14e, 0);
    }
    zstrcpy(path, name);
    file = gZrwFileOps.rwfopen(path, gZrwOpenModes[mode]);
    if (file == NULL) {
        zfileRegisterErrorMessage("rwfopen() failed.");
        return 0;
    }
    handle->file = file;
    if (callback != NULL) {
        event.arg = callback->arg;
        event.handle = handle;
        event.ok = 1;
        callback->fn(&event);
    }
    return 1;
}

BMX_ADDR(0x0049e630)
int zrwFileClose(ZrwFileHandle *handle, const ZrwFileCallback *callback)
{
    ZrwFileEvent event;

    requireInitialized(0x171);
    if (handle == NULL || handle->file == NULL) {
        zdebugFail(kInvalidHandle, kZrwFileFile, 0x173, 0);
    }
    if (gZrwFileOps.rwfclose(handle->file) != 0) {
        zfileRegisterErrorMessage("rwfclose() failed");
        return 0;
    }
    if (callback != NULL) {
        event.arg = callback->arg;
        event.handle = handle;
        event.ok = 1;
        callback->fn(&event);
    }
    return 1;
}

BMX_ADDR(0x0049e6d0)
int zrwFileRead(ZrwFileHandle *handle, void *buffer, int count, const ZrwFileCallback *callback)
{
    ZrwFileEvent event;
    int done;

    requireInitialized(400);
    if (handle == NULL || handle->file == NULL) {
        zdebugFail(kInvalidHandle, kZrwFileFile, 0x192, 0);
    }
    done = (int)gZrwFileOps.rwfread(buffer, 1, (size_t)count, handle->file);
    if (done != count) {
        zfileRegisterErrorMessage("rwfread() failed.");
    }
    if (callback != NULL) {
        event.arg = callback->arg;
        event.handle = handle;
        event.ok = done == count;
        event.value = (uint32_t)done;
        callback->fn(&event);
    }
    return done;
}

BMX_ADDR(0x0049e780)
int zrwFileWrite(ZrwFileHandle *handle, const void *buffer, int count, const ZrwFileCallback *callback)
{
    ZrwFileEvent event;
    int done;

    requireInitialized(0x1b4);
    if (handle == NULL || handle->file == NULL) {
        zdebugFail(kInvalidHandle, kZrwFileFile, 0x1b6, 0);
    }
    done = (int)gZrwFileOps.rwfwrite(buffer, 1, (size_t)count, handle->file);
    if (done != count) {
        zfileRegisterErrorMessage("rwfwrite() failed.");
    }
    if (callback != NULL) {
        event.arg = callback->arg;
        event.handle = handle;
        event.ok = done == count;
        event.value = (uint32_t)done;
        callback->fn(&event);
    }
    return done;
}

BMX_ADDR(0x0049e830)
int zrwFileSeek(ZrwFileHandle *handle, uint32_t offset, int whence, const ZrwFileCallback *callback)
{
    ZrwFileEvent event;
    const uint32_t *origins = gZrwSeekOrigins;

    requireInitialized(0x1dd);
    if (whence > 2) {
        zdebugFail("Invalid seek mode", kZrwFileFile, 0x1de, 0);
    }
    if (handle == NULL || handle->file == NULL) {
        zdebugFail(kInvalidHandle, kZrwFileFile, 0x1e0, 0);
    }
    if (gZrwFileOps.rwfseek(handle->file, (int32_t)offset, (int32_t)origins[whence]) != 0) {
        zfileRegisterErrorMessage("rwfseek() failed");
        return 0;
    }
    if (callback != NULL) {
        event.arg = callback->arg;
        /* The position reported is the offset for an absolute seek and 0 otherwise. */
        event.value = whence != 0 ? 0 : offset;
        event.handle = handle;
        event.ok = 1;
        callback->fn(&event);
    }
    return 1;
}

BMX_ADDR(0x0049e950)
int zrwFileGetSize(ZrwFileHandle *handle, uint32_t *size, const ZrwFileCallback *callback)
{
    ZrwFileEvent event;

    requireInitialized(0x21a);
    if (size == NULL) {
        zdebugFail("Invalid pointer to file size.", kZrwFileFile, 0x21b, 0);
    }
    const int32_t position = rwftell(handle->file);
    *size = (uint32_t)rwftell(handle->file);
    rwfseek(handle->file, 0, 2);
    *size = (uint32_t)rwftell(handle->file);
    rwfseek(handle->file, position, 0);
    if (callback != NULL) {
        event.value = *size;
        event.arg = callback->arg;
        event.handle = handle;
        event.ok = 1;
        callback->fn(&event);
    }
    return 1;
}

/* Returns 1: the original leaves 1 in EAX (Ghidra shows void), and zrwFileSystemsInitialize checks it. */
BMX_ADDR(0x0049ea10)
int zrwFileSystemInitialize(void)
{
    if (gZrwFileInitialized != 0) {
        zdebugFail("File system has already been intialized.", kZrwFileFile, 0x24b, 0);
    }
    memcpy(&gZrwFileOps, RwOSGetFileInterface(), sizeof gZrwFileOps);
    gZrwFileInitialized = 1;
    return 1;
}

BMX_ADDR(0x0049ea60)
int zrwFileSystemShutdown(void)
{
    if (gZrwFileInitialized == 0) {
        zdebugFail("File system has already been shut down.", kZrwFileFile, 0x25e, 0);
    }
    gZrwFileInitialized = 0;
    return 1;
}
