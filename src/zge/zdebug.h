#ifndef ZGE_ZDEBUG_H
#define ZGE_ZDEBUG_H

#include "compat/orig.h"

/*
 * Fatal assertion handler (dispatches through a function pointer at 0x56147c). Ported code passes the
 * original file path and line number so messages match the original exactly.
 */
ORIG_FN(0x0047ed10, void, zdebugFail, (const char *why, const char *file, int line, int flags));

#endif
