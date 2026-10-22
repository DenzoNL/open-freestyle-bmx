/*
 * Tests for zge.c: the start-up sequence, with every subsystem replaced by a stub that logs its call and
 * succeeds or fails depending on the step the test picked.
 */
#include "dt.h"
#include "dt_s5.h"
#include "dt_zmem.h"
#include "zge/zge.h"

static int gFailAt;

static int step(int id, uint32_t arg)
{
    s5Log(0x1000u + (uint32_t)id);
    s5Log(arg);
    return id != gFailAt;
}

static int BMX_CDECL stubMem(void)
{
    s5Log(0x1001);
    return 1;
}

static int BMX_CDECL stubMath(void)
{
    return step(2, 0);
}

static int BMX_CDECL stubFile(void)
{
    return step(3, 0);
}

static int BMX_CDECL stubString(void)
{
    return step(4, 0);
}

static int BMX_CDECL stubIff(void)
{
    s5Log(0x1005);
    return 1;
}

static int BMX_CDECL stubGraphics(void)
{
    return step(6, 0);
}

static int BMX_CDECL stubInput(void)
{
    return step(7, 0);
}

static int BMX_CDECL stubDebug(void)
{
    return step(8, 0);
}

static int BMX_CDECL stubObjects(int count)
{
    return step(9, (uint32_t)count);
}

static int BMX_CDECL stubCollision(void)
{
    return step(10, 0);
}

static int BMX_CDECL stubScene(const char *path)
{
    return step(11, dt_canon((uint32_t)(uintptr_t)path));
}

static int BMX_CDECL stubTimer(int slots)
{
    return step(12, (uint32_t)slots);
}

static int BMX_CDECL stubProfile(void)
{
    return step(13, 0);
}

static void setup_init(DtCtx *c)
{
    s5Begin(c, 1);
    gZgeInitialized = dt_u32() % 8 == 0;
    gFailAt = dt_u32() % 3 == 0 ? 0 : dt_range(2, 13);
    dt_stub(0x004859f0, NULL, (void *)stubMem);
    dt_stub(0x00481f50, NULL, (void *)stubMath);
    dt_stub(0x00483aa0, NULL, (void *)stubFile);
    dt_stub(0x00482420, NULL, (void *)stubString);
    dt_stub(0x0049a6d0, NULL, (void *)stubIff);
    dt_stub(0x0048afe0, NULL, (void *)stubGraphics);
    dt_stub(0x004802f0, NULL, (void *)stubInput);
    dt_stub(0x0047ec40, NULL, (void *)stubDebug);
    dt_stub(0x00487210, NULL, (void *)stubObjects);
    dt_stub(0x00494c20, NULL, (void *)stubCollision);
    dt_stub(0x00490ff0, NULL, (void *)stubScene);
    dt_stub(0x004a4230, NULL, (void *)stubTimer);
    dt_stub(0x004a3b60, NULL, (void *)stubProfile);
    DT_ARG_PTR(c, 0, dt_alloc(16));
}

DT_CASES(zge_cases) = {
    {"zgeInitializeSubsystems", 0x0049f900, zgeInitializeSubsystems, 1, DT_RET_INT, setup_init, 200, 0},
};
