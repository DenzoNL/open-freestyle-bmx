/*
 * Direct access to a model's geometry (C:\NgEngine\Zge\Com\ZglRw\zrwModelGeom.c): lock the vertices of the
 * first morph target and read or write them in game coordinates (scaled, y and z mirrored). Entries of the
 * scene's model interface.
 */
#ifndef ZGE_COM_ZGLRW_ZRWMODELGEOM_H
#define ZGE_COM_ZGLRW_ZRWMODELGEOM_H

#include <stdint.h>

#include "zge/com/zglrw/zrwmodel.h"

/* Sets the callback called at the end of zrwModelUpdate. */
void zrwModelSetOnUpdate(ZrwModel *model, ZrwModelUpdateFn fn);
/* "Not coded": asserts and returns 0. */
int zrwModelGeomNotCoded(void);
/* Returns index + 1. */
int zrwModelNextIndex(int index);
/* Locks the geometry when bit 0 of flags is set (asserts when RenderWare refuses). Returns 1, or 0 on
 * failure. */
int zrwModelLockGeometry(ZrwModel *model, uint8_t flags);
/* Unlocks the geometry. Returns 1, or 0 on failure. */
int zrwModelUnlockGeometry(ZrwModel *model);
/* Vertex number `index` (1 based) of the locked geometry as a game position. Returns 1. */
int zrwModelGetVertex(ZrwModel *model, int index, float *out);
int zrwModelSetVertex(ZrwModel *model, int index, const float *position);

#endif
