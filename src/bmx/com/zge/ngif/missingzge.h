/*
 * Stand-ins for engine calls the PC build lacks (C:\zaxis\Bmx\src\com\zge\NgIF\MissingZGE.c). Most only
 * print "<file> <line> <name>" through the debug print.
 */
#ifndef BMX_COM_ZGE_NGIF_MISSINGZGE_H
#define BMX_COM_ZGE_NGIF_MISSINGZGE_H

#include <stdint.h>

#include "compat/orig.h"

/* Mode byte of the console shell: 1 = menu, 2 = action. */
#define gZsystemMode ORIG_VAR(0x005ab3a0, uint8_t, gZsystemMode)

/* Empty; called once per frame by the shells. */
void zsystemEndFrame(void);
/* Prints "zconsoleLog" (the message argument is ignored). */
void zconsoleLog(const char *message);
void zglEnableHorizon(void);
void zglDisableHorizon(void);
/* Sets the mode byte to 2 (action) and prints. */
void zsystemSetActionMode(void);
/* Sets the mode byte to 1 (menu) and prints. */
void zsystemSetMenuMode(void);
void zfxDestroyAll(int mode);
/* Prints and reports success (1). */
int zfxRegisterFuncTable(const void *table, int count);

/* Names other modules gave these entry points. */
#define appLog            zconsoleLog
#define appShowError      zsystemSetActionMode
#define guiPlatformUpdate zsystemEndFrame

#endif
