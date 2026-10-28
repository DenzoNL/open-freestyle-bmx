/*
 * Tests for rw/light.c: creation, destruction and the setters of RpLight. The engine instance is the
 * texture/light fixture; the frame update and the world extension callback are logging stubs.
 */
#include "dt_rwtex.h"

static int BMX_CDECL stFrameUpdate(void *frame)
{
    rwtLog(0x4d2a80);
    rwtLogPtr(frame);
    return (int)(uintptr_t)frame;
}

static void BMX_CDECL stWorldExtLight(RpLight *light)
{
    rwtLog(0x4b0001);
    rwtLogPtr(light);
}

static void lightStubs(void)
{
    dt_stub(0x004d2a80, NULL, (void *)stFrameUpdate);
    /* entry 3 of the world extension table is original code; stub whatever it points to */
    dt_stub(RW_CB(const uint32_t *, 0x0069b520)[3], NULL, (void *)stWorldExtLight);
}

/* A frame with an empty object list. */
static RwFrame *mkFrame(void)
{
    RwFrame *f = dt_alloc(0x60);
    f->objectList.next = f->objectList.prev = &f->objectList;
    return f;
}

/* A light with random fields; in a frame's object list when `inFrame`. */
static RpLight *mkLight(int inFrame)
{
    RpLight *l = dt_alloc(sizeof *l + 8);
    l->object.object.type = 3;
    l->object.object.subType = (uint8_t)dt_u32();
    l->object.object.flags = (uint8_t)dt_u32();
    l->object.object.privateFlags = (uint8_t)dt_u32();
    l->radius = dt_float_edgy(-100.0F, 100.0F);
    for (int i = 0; i < 4; i++) {
        l->color[i] = dt_float_edgy(-2.0F, 2.0F);
    }
    l->coneAngle = dt_float_edgy(-5.0F, 5.0F);
    l->stamp = (uint8_t)dt_u32();
    l->lightFrame = (uint16_t)dt_u32();
    l->sectors.next = l->sectors.prev = &l->sectors;
    if (inFrame) {
        RwFrame *frame = mkFrame();
        l->object.object.parent = frame;
        l->object.lFrame.next = frame->objectList.next;
        l->object.lFrame.prev = &frame->objectList;
        frame->objectList.next->prev = &l->object.lFrame;
        frame->objectList.next = &l->object.lFrame;
    }
    return l;
}

static void setup_create(DtCtx *c)
{
    texEnv(c);
    lightStubs();
    gTexAllocFailAt = dt_u32() % 5 == 0 ? 1 : 0;
    *(uint16_t *)((uint8_t *)RwEngineInstance + offsetof(RwGlobals, pad0c)) = (uint16_t)dt_u32();
    DT_ARG_INT(c, 0, dt_u32() % 3 == 0 ? dt_u32() : (uint32_t)dt_range(0, 6));
    rwtEnd();
}

static void setup_destroy(DtCtx *c)
{
    texEnv(c);
    DT_ARG_PTR(c, 0, mkLight(dt_u32() % 2));
    rwtEnd();
}

/* The free list callback form: the light and the (unused) data word. */
static void setup_destroy_callback(DtCtx *c)
{
    setup_destroy(c);
    DT_ARG_PTR(c, 1, dt_alloc(8));
}

static void setup_radius(DtCtx *c)
{
    texEnv(c);
    lightStubs();
    DT_ARG_PTR(c, 0, mkLight(dt_u32() % 2));
    DT_ARG_FLOAT(c, 1, dt_float_edgy(-1000.0F, 1000.0F));
    rwtEnd();
}

static void setup_light(DtCtx *c)
{
    texEnv(c);
    DT_ARG_PTR(c, 0, mkLight(dt_u32() % 2));
    rwtEnd();
}

/* The colour tests draw equal channels often, so the grey flag goes both ways (and NaN channels). */
static void setup_color(DtCtx *c)
{
    texEnv(c);
    DT_ARG_PTR(c, 0, mkLight(0));
    float *color = dt_alloc(16);
    for (int i = 0; i < 4; i++) {
        color[i] = dt_float_edgy(-2.0F, 2.0F);
    }
    switch (dt_u32() % 5) {
    case 0:
        color[1] = color[0];
        color[2] = color[0];
        break;
    case 1:
        color[1] = color[0];
        break;
    case 2:
        color[2] = color[0];
        break;
    case 3:
        color[dt_u32() % 3] = __builtin_nanf("");
        break;
    default:
        break;
    }
    DT_ARG_PTR(c, 1, color);
    rwtEnd();
}

static void setup_cone(DtCtx *c)
{
    texEnv(c);
    DT_ARG_PTR(c, 0, mkLight(0));
    DT_ARG_FLOAT(c, 1, dt_u32() % 8 == 0 ? __builtin_nanf("") : dt_float_edgy(-4.0F, 4.0F));
    rwtEnd();
}

static void setup_frame(DtCtx *c)
{
    texEnv(c);
    lightStubs();
    DT_ARG_PTR(c, 0, mkLight(dt_u32() % 2));
    DT_ARG_PTR(c, 1, dt_u32() % 4 == 0 ? NULL : mkFrame());
    rwtEnd();
}

static void setup_flags(DtCtx *c)
{
    texEnv(c);
    DT_ARG_PTR(c, 0, mkLight(0));
    DT_ARG_INT(c, 1, dt_u32());
    rwtEnd();
}

static void setup_ext_init(DtCtx *c)
{
    texEnv(c);
    lightStubs();
    DT_ARG_PTR(c, 0, mkLight(0));
    rwtEnd();
}

DT_CASES(rwlight_cases) = {
    {"RpLightCreate", 0x004c2400, RpLightCreate, 1, DT_RET_INT, setup_create, 150, 0},
    {"RpLightDestroy", 0x004c24c0, RpLightDestroy, 1, DT_RET_INT, setup_destroy, 100, 0},
    {"rpLightFreeListDestroyCB", 0x004c2370, rpLightFreeListDestroyCB, 2, DT_RET_INT, setup_destroy_callback,
     100, 0},
    {"rpLightSyncCallback", 0x004c2510, rpLightSyncCallback, 1, DT_RET_INT, setup_light, 10, 0},
    {"RpLightSetRadius", 0x004c2530, RpLightSetRadius, 2, DT_RET_INT, setup_radius, 150, 0},
    {"RpLightGetColor", 0x004c2560, RpLightGetColor, 1, DT_RET_INT, setup_light, 30, 0},
    {"RpLightSetColor", 0x004c2570, RpLightSetColor, 2, DT_RET_INT, setup_color, 600, 0},
    {"RpLightSetConeAngle", 0x004c25e0, RpLightSetConeAngle, 2, DT_RET_INT, setup_cone, 400, 0},
    {"RpLightSetFrame", 0x004c2bb0, RpLightSetFrame, 2, DT_RET_INT, setup_frame, 200, 0},
    {"RpLightGetFrame", 0x004c2be0, RpLightGetFrame, 1, DT_RET_INT, setup_light, 30, 0},
    {"RpLightGetType", 0x004c2bf0, RpLightGetType, 1, DT_RET_INT, setup_light, 30, 0},
    {"RpLightSetFlags", 0x004c2c00, RpLightSetFlags, 2, DT_RET_INT, setup_flags, 60, 0},
    {"RpLightGetFlags", 0x004c2c20, RpLightGetFlags, 1, DT_RET_INT, setup_light, 30, 0},
    {"rpWorldExtLightInit", 0x004c46f0, rpWorldExtLightInit, 1, DT_RET_VOID, setup_ext_init, 30, 0},
};
