/* Model objects (C:\NgEngine\Zge\ZgeMods\zobjModel.c). */
#include "zge/zgemods/zobjmodel.h"

#include <stddef.h>

#include "compat/orig.h"
#include "zge/zanim.h"
#include "zge/zdebug.h"
#include "zge/zobj.h"

static const char kZobjModelFile[] = "C:\\NgEngine\\Zge\\ZgeMods\\zobjModel.c";

#define gZobjModelReady ORIG_VAR(0x00672308, int, gZobjModelReady)
/* Frees a renderer model handle (a function pointer in the exe data). */
typedef int(BMX_CDECL *ZobjModelFreeFn)(void *model);
#define gZobjModelFree ORIG_VAR(0x00560e94, ZobjModelFreeFn, gZobjModelFree)

BMX_ADDR(0x00488ea0)
int zobjModelFree(void *object)
{
    void *model = ((ZObj *)object)->model;

    if (model != NULL) {
        gZobjModelFree(model);
    }
    return 1;
}

BMX_ADDR(0x00488f40)
int zobjModelInitialize(void)
{
    int ok;

    if (gZobjModelReady != 0) {
        zdebugFail("Already initialized", kZobjModelFile, 0x19d, 0);
    }
    ok = zanimInitialize();
    if (ok == 0) {
        return ok;
    }
    gZobjModelReady = 1;
    return 1;
}
