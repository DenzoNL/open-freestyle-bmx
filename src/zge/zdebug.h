#ifndef ZGE_ZDEBUG_H
#define ZGE_ZDEBUG_H

#include "compat/orig.h"

/*
 * Fatal assertion handler (dispatches through a function pointer at 0x56147c). Ported code passes the
 * original file path and line number so messages match the original exactly.
 */
ORIG_FN(0x0047ed10, void, zdebugFail, (const char *why, const char *file, int line, int flags));

/* printf-style debug output: the exe calls it through a function pointer stored at 0x560074. */
typedef void(BMX_CDECL *ZDebugPrintFn)(const char *fmt, ...);
#define gZdebugPrint ORIG_VAR(0x00560074, ZDebugPrintFn, gZdebugPrint)

/* Indents (positive) or unindents (negative) the following debug output: a function pointer at 0x560078. */
typedef void(BMX_CDECL *ZDebugIndentFn)(int delta);
#define gZdebugIndent ORIG_VAR(0x00560078, ZDebugIndentFn, gZdebugIndent)

/* Sets the sound timer's ticks per second (0x55ffc0); returns the value. */
int zsndSetTickRate(int ticks);
/* Returns 0. */
int zsndNop(void);
/* Starts the debug back end; asserts when called twice. Returns 0 when the back end fails to start. */
int zdebugCreate(void);
/* Sets the debug indent level, clamped to 0..32; zdebugIndent changes it by a delta (the function behind
 * gZdebugIndent). */
void zdebugSetIndent(int level);
void zdebugIndent(int delta);

#endif
