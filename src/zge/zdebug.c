/* Debug support (C:\NgEngine\Zge\zdebug.c). */
#include "zge/zdebug.h"

static const char kZdebugFile[] = "C:\\NgEngine\\Zge\\zdebug.c";

/* Set once the debug back end (a function pointer at 0x560030) has started. */
#define gZdebugCreated ORIG_VAR(0x00650170, int, gZdebugCreated)
typedef int(BMX_CDECL *ZDebugStartFn)(void);
#define gZdebugStart ORIG_VAR(0x00560030, ZDebugStartFn, gZdebugStart)
/* The assert handler the original calls through its pointer (not through the wrapper zdebugFail). */
typedef void(BMX_CDECL *ZDebugFailFn)(const char *why, const char *file, int line, int flags);
#define gZdebugFailHandler ORIG_VAR(0x0056147c, ZDebugFailFn, gZdebugFailHandler)

/* Timer ticks per second the sound sequencer and sources count in. */
#define gZsndTickRate ORIG_VAR(0x0055ffc0, int, gZsndTickRate)

BMX_ADDR(0x0047eba0)
int zsndSetTickRate(int ticks)
{
    gZsndTickRate = ticks;
    return ticks;
}

BMX_ADDR(0x0047ebc0)
int zsndNop(void)
{
    return 0;
}

BMX_ADDR(0x0047ec40)
int zdebugCreate(void)
{
    if (gZdebugCreated != 0) {
        gZdebugFailHandler("Already initialized", kZdebugFile, 0x48, 0);
    }
    if (gZdebugStart() == 0) {
        return 0;
    }
    gZdebugCreated = 1;
    return 1;
}
