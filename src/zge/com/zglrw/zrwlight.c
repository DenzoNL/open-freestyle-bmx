/* Renderer lights (see zrwlight.h). */
#include "zge/com/zglrw/zrwlight.h"

#include "compat/crt.h"
#include "rw/rwcamera.h"
#include "rw/rwcore.h"
#include "rw/rwmath.h"
#include "zge/com/zglrw/zrwmodel.h"
#include "zge/zdebug.h"
#include "zge/zmem.h"

static const char kZrwLightFile[] = "C:\\NgEngine\\Zge\\Com\\ZglRw\\zrwLight.c";

ORIG_FN(0x004c2400, void *, RpLightCreate, (uint32_t type));
ORIG_FN(0x004c2530, void, RpLightSetRadius, (void *light, float radius));
ORIG_FN(0x004c25e0, void, RpLightSetConeAngle, (void *light, float angle));
ORIG_FN(0x004c2570, void, RpLightSetColor, (void *light, const float *color));
ORIG_FN(0x004c2bb0, void, RpLightSetFrame, (void *light, void *frame));
ORIG_FN(0x004c2be0, void *, RpLightGetFrame, (void *light));
ORIG_FN(0x004c2c00, void, RpLightSetFlags, (void *light, uint32_t flags));
ORIG_FN(0x004b8af0, void *, RpWorldAddLight, (void *world, void *light));
ORIG_FN(0x004b8b80, void *, RpWorldRemoveLight, (void *world, void *light));
ORIG_FN(0x004c24c0, int, RpLightDestroy, (void *light));

/* RpLightType of the four kinds the scene creates. */
typedef uint32_t ZrwLightTypeTable[4];
#define gZrwLightTypes ORIG_VAR(0x00563408, ZrwLightTypeTable, gZrwLightTypes)

#define kByteToUnit 0x1.010102p-8f /* 1 / 255 as the original stores it */

/* Colour as RenderWare wants it: floats 0..1. Every product is rounded once, to float. */
static void unitColor(float *out, const uint8_t *rgba)
{
    out[0] = (float)((double)rgba[0] * (double)kByteToUnit);
    out[1] = (float)((double)rgba[1] * (double)kByteToUnit);
    out[2] = (float)((double)rgba[2] * (double)kByteToUnit);
    out[3] = (float)((double)rgba[3] * (double)kByteToUnit);
}

BMX_ADDR(0x0049f520)
int zrwLightShutdown(void)
{
    if (gZrwLightInitialized != 0) {
        zmemFreeListDestroy(gZrwLightPool);
        gZrwLightInitialized = 0;
    }
    return 1;
}

BMX_ADDR(0x0049f550)
ZrwLight *zrwLightCreate(int kind, const uint8_t *rgba)
{
    ZrwLight *record = zmemFreeListAlloc(gZrwLightPool);
    float color[4];
    ZVec3 position;
    void *frame;

    memset(record, 0, sizeof *record);
    record->flags = (record->flags & 0xffffff02u) | 2;
    record->light = RpLightCreate(gZrwLightTypes[kind]);
    if (record->light == NULL) {
        zmemFreeListFree(gZrwLightPool, record);
        return NULL;
    }
    RpLightSetRadius(record->light, (float)((double)gZrwScale * (double)0x1.47ae14p-6f));
    RpLightSetConeAngle(record->light, 0x1.921fb6p-1f);
    unitColor(color, rgba);
    RpLightSetColor(record->light, color);
    frame = RwFrameCreate();
    RwFrameSetIdentity(frame);
    RpLightSetFrame(record->light, frame);
    position.x = (float)((double)0.0f + (double)gZrwWorldCenter.x);
    position.y = (float)((double)gZrwWorldCenter.y - (double)0.0f);
    position.z = (float)((double)gZrwWorldCenter.z - (double)0.0f);
    RwFrameTranslate(RpLightGetFrame(record->light), &position, 0);
    RpLightSetFlags(record->light, 3);
    RpWorldAddLight(gZrwWorld, record->light);
    return record;
}

BMX_ADDR(0x0049f6d0)
int zrwLightDestroy(ZrwLight *light)
{
    RpWorldRemoveLight(gZrwWorld, light->light);
    RwFrameDestroy(RpLightGetFrame(light->light));
    RpLightDestroy(light->light);
    zmemFreeListFree(gZrwLightPool, light);
    return 1;
}

BMX_ADDR(0x0049f720)
ZrwLight *zrwLightSetColor(ZrwLight *light, const uint8_t *rgba)
{
    float color[4];

    unitColor(color, rgba);
    RpLightSetColor(light->light, color);
    return light;
}

BMX_ADDR(0x0049f7a0)
ZrwLight *zrwLightSetFlags(ZrwLight *light, uint8_t flags)
{
    uint32_t bits = (flags & 1) != 0 ? 1u : 0u;

    if ((flags & 2) != 0) {
        bits |= 2;
    }
    RpLightSetFlags(light->light, bits);
    return light;
}

BMX_ADDR(0x0049f7d0)
ZrwLight *zrwLightSetPosition(ZrwLight *light, const float *position)
{
    ZVec3 world;

    world.x = (float)((double)gZrwScale * (double)position[0] + (double)gZrwWorldCenter.x);
    world.y = (float)((double)gZrwWorldCenter.y - (double)position[1] * (double)gZrwScale);
    world.z = (float)((double)gZrwWorldCenter.z - (double)position[2] * (double)gZrwScale);
    RwFrameTranslate(RpLightGetFrame(light->light), &world, 0);
    return light;
}

BMX_ADDR(0x0049f840)
ZrwLight *zrwLightSetDirection(ZrwLight *light, const float *direction)
{
    ZMatrix *matrix;
    const float y = -direction[1];
    const float z = -direction[2];

    matrix = RwFrameGetMatrix(RpLightGetFrame(light->light));
    matrix->at.x = direction[0];
    matrix->at.y = y;
    matrix->at.z = z;
    RwMatrixUpdate(matrix);
    return light;
}

BMX_ADDR(0x0049f8a0)
ZrwLight *zrwLightSetConeAngle(ZrwLight *light, int16_t angle)
{
    RpLightSetConeAngle(light->light, (float)((double)angle * (double)0x1.921fb6p-14f));
    return light;
}

BMX_ADDR(0x0049f8d0)
ZrwLight *zrwLightSetRadius(ZrwLight *light, float radius)
{
    RpLightSetRadius(light->light, (float)((double)gZrwScale * (double)radius));
    return light;
}

BMX_ADDR(0x0049f4d0)
int zrwLightSystemInitialize(void)
{
    if (gZrwLightInitialized != 0) {
        zdebugFail("Already initialized", kZrwLightFile, 0xa9, 0);
    }
    gZrwLightInitialized = 1;
    gZrwLightPool =
        zmemFreeListCreate(sizeof(ZrwLight), 2, BMX_FNPTR(ZMemDestroyFn, 0x0049f6d0, zrwLightDestroy));
    return 1;
}
