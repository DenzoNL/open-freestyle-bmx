/* Input system start-up (C:\NgEngine\Zge\zinput.c). */
#include "zge/zinput.h"

#include "zge/pc/pcinput.h"
#include "zge/zdebug.h"
#include "zge/zmem.h"

static const char kZinputFile[] = "C:\\NgEngine\\Zge\\zinput.c";

#define gZinputReady ORIG_VAR(0x0065e2a8, int, gZinputReady)

BMX_ADDR(0x004802f0)
int zinputCreate(void)
{
    if (gZinputReady != 0) {
        zdebugFail("Already initialized", kZinputFile, 0x49, 0);
    }
    gZinputReady = 1;
    gZpadDevices =
        zmemFreeListCreate(sizeof(ZpadDevice), 2, BMX_FNPTR(ZMemDestroyFn, 0x004803c0, zpadDestroyDevice));
    zpadDirectInputInitialize();
    return 1;
}
