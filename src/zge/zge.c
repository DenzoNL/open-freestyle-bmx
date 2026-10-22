/* Engine start-up (C:\NgEngine\Zge\zge.c). */
#include "zge/zge.h"

#include "zge/com/zglrw/zrwcol.h"
#include "zge/com/zglrw/zrwzgl.h"
#include "zge/zdebug.h"
#include "zge/zfile.h"
#include "zge/zgemods/zobjcamera.h"
#include "zge/ziff.h"
#include "zge/zmem.h"
#include "zge/zobj.h"
#include "zge/zprofile.h"
#include "zge/zscene.h"
#include "zge/zstring.h"

static const char kZgeFile[] = "C:\\NgEngine\\Zge\\zge.c";

/* Subsystems that are not ported yet. */
ORIG_FN(0x004802f0, int, zinputCreate, (void));
ORIG_FN(0x0047ec40, int, zdebugCreate, (void));
ORIG_FN(0x004a4230, int, ztimerInitialize, (int slots));

/* The renderer's start-up function (a pointer in the exe's data). */
typedef int(BMX_CDECL *ZrwInitFn)(void);
#define gZrwGraphicLibraryInitialize ORIG_VAR(0x00561468, ZrwInitFn, gZrwGraphicLibraryInitialize)

BMX_ADDR(0x0049f900)
int zgeInitializeSubsystems(const char *scenePath)
{
    if (gZgeInitialized != 0) {
        zdebugFail("Already initialized", kZgeFile, 0x41, 0);
    }
    gZgeInitialized = 1;
    zmemInitialize();
    if (zmathInitialize() == 0) {
        return 0;
    }
    if (zfileInitialize() == 0) {
        return 0;
    }
    if (zstringInit() == 0) {
        return 0;
    }
    ziffInitialize();
    if (gZrwGraphicLibraryInitialize() == 0) {
        return 0;
    }
    if (zinputCreate() == 0) {
        return 0;
    }
    if (zdebugCreate() == 0) {
        return 0;
    }
    if (zobjInitialize(0x32) == 0) {
        return 0;
    }
    if (zrwcolEnableIfTagInfo() == 0) {
        return 0;
    }
    if (zsceneInitialize(scenePath) == 0) {
        return 0;
    }
    if (ztimerInitialize(0x40) == 0) {
        return 0;
    }
    return zprofileInitialize() != 0;
}
