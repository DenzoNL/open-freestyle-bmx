/* RenderWare 3.1 lights: creation, destruction and the property setters (RpLight*). */
#include "compat/x87.h"
#include "rw/rwlight.h"
#include "rw/rwplugins.h"

/* Callees in other RenderWare groups, still original code. */
ORIG_FN(0x004d2a80, void *, RwFrameUpdateObjects, (RwFrame * frame));

/* The light's address in the original: its sync callback, which only returns the object. */
#define RP_LIGHT_SYNC_CALLBACK 0x004c2510

static RwFreeList *lightFreeList(void)
{
    return *(RwFreeList **)((uint8_t *)RwEngineInstance + rpLightOffset);
}

/* Tells the world extension module that a light exists (its callback is entry 3 of the module table). */
BMX_ADDR(0x004c46f0)
void rpWorldExtLightInit(RpLight *light)
{
    ((void(BMX_CDECL *)(RpLight *))rpWorldExtGetFuncs()[3])(light);
}

/* A new light of the given type: white, no radius or cone, lighting atomics and the world. */
BMX_ADDR(0x004c2400)
RpLight *RpLightCreate(uint32_t type)
{
    RpLight *light = RwEngineInstance->freeListAlloc(lightFreeList());
    if (light == NULL) {
        return NULL;
    }
    light->inWorld.next = NULL;
    light->inWorld.prev = NULL;
    light->sectors.prev = &light->sectors;
    light->stamp = 0;
    light->sectors.next = &light->sectors;
    light->object.object.privateFlags = 1;
    for (int i = 0; i < 4; i++) {
        light->color[i] = 1.0F;
    }
    light->coneAngle = 0.0F;
    light->radius = 0.0F;
    light->object.object.parent = NULL;
    light->object.sync = RW_CB(void *, RP_LIGHT_SYNC_CALLBACK);
    light->object.object.flags = 0;
    light->object.object.subType = (uint8_t)type;
    light->object.object.type = 3;
    const uint16_t lightFrame =
        *(const uint16_t *)((const uint8_t *)RwEngineInstance + offsetof(RwGlobals, pad0c));
    light->stamp = 1;
    light->object.object.flags = rpLIGHTLIGHTATOMICS | rpLIGHTLIGHTWORLD;
    light->lightFrame = (uint16_t)(lightFrame - 1);
    rpWorldExtLightInit(light);
    _rwPluginRegistryInitObject(&rpLightRegistry, light);
    return light;
}

/*
 * Destroys a light: plugin data, its frame link and the memory. The first four bytes of the original are the
 * load of the argument for the register-entry tail at 0x4c24c4, which no other code enters.
 */
BMX_ADDR(0x004c24c0)
int RpLightDestroy(RpLight *light)
{
    _rwPluginRegistryDeInitObject(&rpLightRegistry, light);
    _rwObjectHasFrameReleaseFrame(&light->object);
    RwEngineInstance->freeListFree(lightFreeList(), light);
    return 1;
}

/* Sets the radius and lets the frame know its object changed. Returns the light. */
BMX_ADDR(0x004c2530)
RpLight *RpLightSetRadius(RpLight *light, float radius)
{
    light->radius = radius;
    if (light->object.object.parent != NULL) {
        RwFrameUpdateObjects(light->object.object.parent);
    }
    light->stamp++;
    return light;
}

/* The light's colour (red, green, blue, alpha). */
BMX_ADDR(0x004c2560)
const float *RpLightGetColor(const RpLight *light)
{
    return light->color;
}

/*
 * Sets the colour and flags the light as grey (private flag 1) when red == green == blue. The original
 * tests with FCOMP/SAHF/JZ, which also takes unordered operands (NaN) as equal; so does this.
 */
BMX_ADDR(0x004c2570)
RpLight *RpLightSetColor(RpLight *light, const float *color)
{
    for (int i = 0; i < 4; i++) {
        light->color[i] = color[i];
    }
    const double red = light->color[0];
    const int greenSame = !(light->color[1] < red) && !(light->color[1] > red);
    const int blueSame = !(red < light->color[2]) && !(red > light->color[2]);
    light->object.object.privateFlags = greenSame && blueSame ? 1 : 0;
    light->stamp++;
    return light;
}

/* Stores the tangent of the cone angle (x87 FPTAN, rounded to float). Returns the light. */
BMX_ADDR(0x004c25e0)
RpLight *RpLightSetConeAngle(RpLight *light, float angle)
{
    light->coneAngle = bmxX87TanFloat(angle);
    light->stamp++;
    return light;
}

BMX_ADDR(0x004c2bb0)
RpLight *RpLightSetFrame(RpLight *light, RwFrame *frame)
{
    _rwObjectHasFrameSetFrame(&light->object, frame);
    light->stamp++;
    return light;
}

BMX_ADDR(0x004c2be0)
RwFrame *RpLightGetFrame(const RpLight *light)
{
    return light->object.object.parent;
}

BMX_ADDR(0x004c2bf0)
uint32_t RpLightGetType(const RpLight *light)
{
    return light->object.object.subType;
}

BMX_ADDR(0x004c2c00)
RpLight *RpLightSetFlags(RpLight *light, uint32_t flags)
{
    light->object.object.flags = (uint8_t)flags;
    light->stamp++;
    return light;
}

BMX_ADDR(0x004c2c20)
uint32_t RpLightGetFlags(const RpLight *light)
{
    return light->object.object.flags;
}
