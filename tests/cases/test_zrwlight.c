/*
 * Tests for zrwlight.c: the renderer light interface. The RenderWare calls are stubs that log their
 * arguments (floats by their bits) and hand out fixed handles.
 */
#include "dt.h"
#include "dt_s5.h"
#include "dt_zmem.h"
#include "zge/com/zglrw/zrwlight.h"
#include "zge/com/zglrw/zrwmodel.h"

static ZMatrix gFrameMatrix;
static int gCreateFails;

static void *handle(void)
{
    return (void *)(uintptr_t)(0x34000000u + (dt_u32() & 0xfffffu) * 16u);
}

static void *BMX_CDECL stubLightCreate(uint32_t type)
{
    s5Log(0x4c2400);
    s5Log(type);
    return gCreateFails ? NULL : (void *)0x34100000u;
}

static void BMX_CDECL stubSetRadius(void *light, float radius)
{
    s5Log(0x4c2530);
    s5LogPtr(light);
    s5LogFloat(radius);
}

static void BMX_CDECL stubSetCone(void *light, float angle)
{
    s5Log(0x4c25e0);
    s5LogPtr(light);
    s5LogFloat(angle);
}

static void BMX_CDECL stubSetColor(void *light, const float *color)
{
    s5Log(0x4c2570);
    s5LogPtr(light);
    s5LogWords(color, 4);
}

static void BMX_CDECL stubSetFrame(void *light, void *frame)
{
    s5Log(0x4c2bb0);
    s5LogPtr(light);
    s5LogPtr(frame);
}

static void *BMX_CDECL stubGetFrame(void *light)
{
    s5Log(0x4c2be0);
    s5LogPtr(light);
    return (void *)0x34200000u;
}

static void BMX_CDECL stubSetFlags(void *light, uint32_t flags)
{
    s5Log(0x4c2c00);
    s5LogPtr(light);
    s5Log(flags);
}

static void *BMX_CDECL stubWorldAdd(void *world, void *light)
{
    s5Log(0x4b8af0);
    s5LogPtr(world);
    s5LogPtr(light);
    return light;
}

static void *BMX_CDECL stubWorldRemove(void *world, void *light)
{
    s5Log(0x4b8b80);
    s5LogPtr(world);
    s5LogPtr(light);
    return light;
}

static int BMX_CDECL stubLightDestroy(void *light)
{
    s5Log(0x4c24c0);
    s5LogPtr(light);
    return 1;
}

static void *BMX_CDECL stubFrameCreate(void)
{
    s5Log(0x4d2870);
    return (void *)0x34300000u;
}

static int BMX_CDECL stubFrameDestroy(void *frame)
{
    s5Log(0x4d2920);
    s5LogPtr(frame);
    return 1;
}

static void *BMX_CDECL stubTranslate(void *frame, const ZVec3 *offset, int combine)
{
    s5Log(0x4d2f40);
    s5LogPtr(frame);
    s5LogWords(offset, 3);
    s5Log((uint32_t)combine);
    return frame;
}

static ZMatrix *BMX_CDECL stubGetMatrix(void *frame)
{
    s5Log(0x4d2b00);
    s5LogPtr(frame);
    return &gFrameMatrix;
}

static void *BMX_CDECL stubSetIdentity(void *frame)
{
    s5Log(0x4d3200);
    s5LogPtr(frame);
    return frame;
}

static void *BMX_CDECL stubMatrixUpdate(ZMatrix *m)
{
    s5Log(0x4d04d0);
    s5LogPtr(m);
    s5LogWords(m, 16);
    return m;
}

static void begin(DtCtx *c)
{
    s5Begin(c, 0);
    gCreateFails = 0;
    dt_stub(0x004c2400, NULL, (void *)stubLightCreate);
    dt_stub(0x004c2530, NULL, (void *)stubSetRadius);
    dt_stub(0x004c25e0, NULL, (void *)stubSetCone);
    dt_stub(0x004c2570, NULL, (void *)stubSetColor);
    dt_stub(0x004c2bb0, NULL, (void *)stubSetFrame);
    dt_stub(0x004c2be0, NULL, (void *)stubGetFrame);
    dt_stub(0x004c2c00, NULL, (void *)stubSetFlags);
    dt_stub(0x004b8af0, NULL, (void *)stubWorldAdd);
    dt_stub(0x004b8b80, NULL, (void *)stubWorldRemove);
    dt_stub(0x004c24c0, NULL, (void *)stubLightDestroy);
    dt_stub(0x004d2870, NULL, (void *)stubFrameCreate);
    dt_stub(0x004d2920, NULL, (void *)stubFrameDestroy);
    dt_stub(0x004d2f40, NULL, (void *)stubTranslate);
    dt_stub(0x004d2b00, NULL, (void *)stubGetMatrix);
    dt_stub(0x004d3200, NULL, (void *)stubSetIdentity);
    dt_stub(0x004d04d0, NULL, (void *)stubMatrixUpdate);
    gZrwScale = dt_float_edgy(-5.0f, 5.0f);
    s5RandVec(&gZrwWorldCenter);
    gZrwWorld = handle();
    gZrwLightPool = S5_POOL_CREATE(12, 4, NULL);
    gZrwLightInitialized = dt_u32() % 2;
    {
        float *m = (float *)&gFrameMatrix;
        for (int i = 0; i < 16; i++) {
            m[i] = dt_float(-1.0f, 1.0f);
        }
    }
}

static ZrwLight *newLight(void)
{
    ZrwLight *l = dt_alloc(sizeof *l);

    dt_fill(l, sizeof *l);
    l->light = handle();
    return l;
}

static uint8_t *randColor(void)
{
    uint8_t *rgba = dt_alloc(4);

    dt_fill(rgba, 4);
    if (dt_u32() % 5 == 0) {
        rgba[0] = rgba[1] = rgba[2] = rgba[3] = 0xff;
    }
    return rgba;
}

static void setup_shutdown(DtCtx *c)
{
    begin(c);
}

static void setup_create(DtCtx *c)
{
    begin(c);
    gCreateFails = dt_u32() % 6 == 0;
    DT_ARG_INT(c, 0, dt_range(0, 3));
    DT_ARG_PTR(c, 1, randColor());
}

static void setup_destroy(DtCtx *c)
{
    ZrwLight *l;

    begin(c);
    l = S5_POOL_ALLOC(gZrwLightPool);
    l->light = handle();
    DT_ARG_PTR(c, 0, l);
}

static void setup_color(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, newLight());
    DT_ARG_PTR(c, 1, randColor());
}

static void setup_flags(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, newLight());
    DT_ARG_INT(c, 1, dt_u32());
}

static void setup_vector(DtCtx *c)
{
    float *v = dt_alloc(12);

    begin(c);
    for (int i = 0; i < 3; i++) {
        v[i] = s5Rf();
    }
    DT_ARG_PTR(c, 0, newLight());
    DT_ARG_PTR(c, 1, v);
}

static void setup_cone(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, newLight());
    DT_ARG_INT(c, 1, dt_u32());
}

static void setup_radius(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, newLight());
    DT_ARG_FLOAT(c, 1, dt_float_edgy(-100.0f, 100.0f));
}

DT_CASES(zrwlight_cases) = {
    {"zrwLightShutdown", 0x0049f520, zrwLightShutdown, 0, DT_RET_INT, setup_shutdown, 10, 0},
    {"zrwLightCreate", 0x0049f550, zrwLightCreate, 2, DT_RET_INT, setup_create, 300, 0},
    {"zrwLightDestroy", 0x0049f6d0, zrwLightDestroy, 1, DT_RET_INT, setup_destroy, 20, 0},
    {"zrwLightSetColor", 0x0049f720, zrwLightSetColor, 2, DT_RET_INT, setup_color, 300, 0},
    {"zrwLightSetFlags", 0x0049f7a0, zrwLightSetFlags, 2, DT_RET_INT, setup_flags, 100, 0},
    {"zrwLightSetPosition", 0x0049f7d0, zrwLightSetPosition, 2, DT_RET_INT, setup_vector, 300, 0},
    {"zrwLightSetDirection", 0x0049f840, zrwLightSetDirection, 2, DT_RET_INT, setup_vector, 300, 0},
    {"zrwLightSetConeAngle", 0x0049f8a0, zrwLightSetConeAngle, 2, DT_RET_INT, setup_cone, 300, 0},
    {"zrwLightSetRadius", 0x0049f8d0, zrwLightSetRadius, 2, DT_RET_INT, setup_radius, 300, 0},
};
