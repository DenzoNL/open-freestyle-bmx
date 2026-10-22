/* Direct access to a model's geometry (see zrwmodelgeom.h). */
#include "zge/com/zglrw/zrwmodelgeom.h"

#include "zge/zdebug.h"
#include "zge/zstring.h"

ORIG_FN(0x004b43e0, void *, RpGeometryGetMorphTarget, (void *geometry, int index));
ORIG_FN(0x004b4f20, int, RpGeometryLock, (void *geometry, int lockMode));
ORIG_FN(0x004b45d0, float *, RpMorphTargetGetVertices, (void *morphTarget));
ORIG_FN(0x004b4fd0, int, RpGeometryUnlock, (void *geometry));

static const char kZrwModelGeomFile[] = "C:\\NgEngine\\Zge\\Com\\ZglRw\\zrwModelGeom.c";

enum { kRpGeometryLockVertices = 2 };

BMX_ADDR(0x0049dfe0)
void zrwModelSetOnUpdate(ZrwModel *model, ZrwModelUpdateFn fn)
{
    model->onUpdate = fn;
}

BMX_ADDR(0x0049dff0)
int zrwModelGeomNotCoded(void)
{
    zdebugFail("Not coded", kZrwModelGeomFile, 0x86, 0);
    return 0;
}

BMX_ADDR(0x0049e010)
int zrwModelNextIndex(int index)
{
    return (int)((uint32_t)index + 1u);
}

BMX_ADDR(0x0049e020)
int zrwModelLockGeometry(ZrwModel *model, uint8_t flags)
{
    if ((flags & 1) != 0) {
        void *const geometry = model->def->geometry;
        void *const target = RpGeometryGetMorphTarget(geometry, 0);

        if (RpGeometryLock(geometry, kRpGeometryLockVertices) == 0) {
            zdebugFail(zstrFormatTemp("Could not lock model geometry for '%s'", model->def->name),
                       kZrwModelGeomFile, 0xba, 0);
            return 0;
        }
        model->def->vertices = RpMorphTargetGetVertices(target);
        model->flags |= 4;
    }
    return 1;
}

BMX_ADDR(0x0049e0b0)
int zrwModelUnlockGeometry(ZrwModel *model)
{
    if (RpGeometryUnlock(model->def->geometry) == 0) {
        zdebugFail(zstrFormatTemp("Could not unlock model geometry for '%s'", model->def->name),
                   kZrwModelGeomFile, 0xdc, 0);
        return 0;
    }
    model->flags &= ~4u;
    return 1;
}

BMX_ADDR(0x0049e110)
int zrwModelGetVertex(ZrwModel *model, int index, float *out)
{
    const uint32_t offset = ((uint32_t)index * 3u - 3u) * 4u;
    const float *const v = (const float *)((const uint8_t *)model->def->vertices + offset);

    out[0] = (float)((double)v[0] * (double)gZrwInvScale);
    out[1] = -(float)((double)v[1] * (double)gZrwInvScale);
    out[2] = -(float)((double)v[2] * (double)gZrwInvScale);
    return 1;
}

BMX_ADDR(0x0049e180)
int zrwModelSetVertex(ZrwModel *model, int index, const float *position)
{
    const uint32_t offset = ((uint32_t)index * 3u - 3u) * 4u;
    float *const v = (float *)((uint8_t *)model->def->vertices + offset);

    v[0] = (float)((double)gZrwScale * (double)position[0]);
    v[1] = -(float)((double)position[1] * (double)gZrwScale);
    v[2] = -(float)((double)position[2] * (double)gZrwScale);
    return 1;
}
