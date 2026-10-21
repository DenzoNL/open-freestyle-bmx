/* Console-port leftovers (C:\zaxis\Bmx\src\com\zge\NgIF\MissingZGE.c). */
#include "bmx/com/zge/ngif/missingzge.h"

#include "zge/zdebug.h"

static const char kMissingFile[] = "C:\\zaxis\\Bmx\\src\\com\\zge\\NgIF\\MissingZGE.c";

/* What every stub prints: "<file> <line> <name>". */
static void trace(int line, const char *what)
{
    gZdebugPrint("%s %d %s\n", kMissingFile, line, what);
}

BMX_ADDR(0x0045a1a0)
void zsystemEndFrame(void) {}

BMX_ADDR(0x0045a1c0)
void zconsoleLog(const char *message)
{
    trace(0x56, "zconsoleLog");
}

BMX_ADDR(0x0045a1f0)
void zglEnableHorizon(void)
{
    trace(0x5b, "Enable Horizon");
}

BMX_ADDR(0x0045a220)
void zglDisableHorizon(void)
{
    trace(0x60, "zgl disable horizon");
}

BMX_ADDR(0x0045a250)
void zsystemSetActionMode(void)
{
    gZsystemMode = 2;
    trace(0x66, "zsystemSetActionMode()");
}

BMX_ADDR(0x0045a290)
void zsystemSetMenuMode(void)
{
    gZsystemMode = 1;
    trace(0x6c, "zsystemSetMenuMode");
}

BMX_ADDR(0x0045a2d0)
void zfxDestroyAll(int mode)
{
    trace(0x7a, "zfxDestroyAll");
}

BMX_ADDR(0x0045a300)
int zfxRegisterFuncTable(const void *table, int count)
{
    trace(0x81, "zfxRegisterFuncTable");
    return 1;
}
