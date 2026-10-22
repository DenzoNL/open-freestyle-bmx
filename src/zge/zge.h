/* Engine start-up (C:\NgEngine\Zge\zge.c). */
#ifndef ZGE_ZGE_H
#define ZGE_ZGE_H

#include "compat/orig.h"

#define gZgeInitialized ORIG_VAR(0x00675378, int, gZgeInitialized)

/*
 * Starts every engine subsystem in order (memory, tables, files, strings, IFF, renderer, input, debug,
 * objects, collision, scene at `scenePath`, timer, profiler); stops at the first one that fails.
 * Returns 1 when all started.
 */
int zgeInitializeSubsystems(const char *scenePath);

#endif
