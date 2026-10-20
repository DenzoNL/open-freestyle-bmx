/* Scene (C:\NgEngine\Zge\zScene.c). Not ported yet; callees only. */
#ifndef ZGE_ZSCENE_H
#define ZGE_ZSCENE_H

#include "compat/orig.h"

/* Finds (or creates) the scene object of a model name; NULL when it is not in the scene. */
ORIG_FN(0x00491410, void *, zsceneFindModel, (int flags, const char *name));

#endif
