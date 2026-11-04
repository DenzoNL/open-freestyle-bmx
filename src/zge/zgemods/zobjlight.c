/* Light objects (C:\NgEngine\Zge\ZgeMods\zobjLight.c). */
#include <stddef.h>

#include "zge/zgemods/zobjlight.h"

#include "zge/zdebug.h"
#include "zge/zobj.h"
#include "zge/zquat.h"

static const char kZobjLightFile[] = "C:\\NgEngine\\Zge\\ZgeMods\\zobjLight.c";

#define gZobjLightInitialized ORIG_VAR(0x00673f68, int, gZobjLightInitialized)
#define gZobjLightOps         ORIG_VAR(0x00561858, void *, gZobjLightOps)

BMX_ADDR(0x0048ff50)
int zobjLightDestroy(ZObjLight *light)
{
    gZrwLightDestroy(light->handle);
    return 1;
}

BMX_ADDR(0x0048ff70)
int zobjLightSetParam(ZObjLight *light, const ZVec3 *position)
{
    gZrwLightSetParam(light->handle, position);
    return 1;
}

BMX_ADDR(0x0048ff90)
int zobjLightSetRotation(ZObjLight *light, const ZQuat *rotation)
{
    ZVec3 axis;

    zquatToAxisAngle(&axis, rotation);
    gZrwLightDirection(light->handle, &axis);
    return 1;
}

/* Returns 1: the original ends with MOV EAX,1 (Ghidra shows void) and its caller tests EAX. */
BMX_ADDR(0x0048ffc0)
int zobjLightInitialize(void)
{
    if (gZobjLightInitialized != 0) {
        zdebugFail("Already initialized", kZobjLightFile, 0x99, 0);
    }
    gZobjLightInitialized = 1;
    return 1;
}

BMX_ADDR(0x00490010)
void *zobjLightCreate(int type, const uint8_t *rgba)
{
    ZObjLight *light = zobjAlloc();

    if (light == NULL) {
        return NULL;
    }
    zobjInit(light, 1);
    light->ops = (const void *)&gZobjLightOps;
    if ((uint32_t)type <= 3) {
        light->handle = gZrwLightCreate(type, rgba);
        return light;
    }
    zdebugFail("Unknown type", kZobjLightFile, 0xda, 0);
    return light;
}
