/* Scene points (C:\NgEngine\Zge\ZgeMods\zscnpts.c). Not ported yet; callees only. */
#ifndef ZGE_ZGEMODS_ZSCNPTS_H
#define ZGE_ZGEMODS_ZSCNPTS_H

#include "compat/orig.h"

/* Releases the data of one scene point (list payload). */
ORIG_FN(0x00491f10, void, zscnptsFreePoint, (void *point, void *arg));

#endif
