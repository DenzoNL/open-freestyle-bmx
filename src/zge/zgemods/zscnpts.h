/* Scene points (C:\NgEngine\Zge\ZgeMods\zscnpts.c). Partly ported (the rest lives in zscene.c). */
#ifndef ZGE_ZGEMODS_ZSCNPTS_H
#define ZGE_ZGEMODS_ZSCNPTS_H

#include "compat/orig.h"

/* Releases the data of one scene point (list payload) and the point itself; returns 1 (0 for NULL). `arg` is
 * not read. */
int zscnptsFreePoint(void *point, void *arg);
/* Chunk handler of the point list: reads one point ("SDPT" data and an optional "OBJI" block). Returns 0 on
 * bad data. */
int zscnptsReadChunk(void *iff);

#endif
