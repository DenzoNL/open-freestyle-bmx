/*
 * Tests for the small start-up pieces of zinput.c, the debug indent level (zdebug.c) and the model object
 * hooks (zobjModel.c). Callees are replaced by logging stand-ins.
 */
#include "dt.h"
#include "dt_ngif.h"
#include "zge/pc/pcinput.h"
#include "zge/zdebug.h"
#include "zge/zgemods/zobjmodel.h"
#include "zge/zinput.h"

#define gTestZinputReady    (*(int *)(uintptr_t)0x0065e2a8)
#define gTestIndent         (*(int *)(uintptr_t)0x0065e29c)
#define gTestZobjModelReady (*(int *)(uintptr_t)0x00672308)

static int BMX_CDECL stubDirectInit(void)
{
    ngLog(0x493160);
    return 1;
}

static void setup_create(DtCtx *c)
{
    ngBegin(c, 1);
    gTestZinputReady = (dt_u32() & 1) != 0;
    gZpadDevices = (void *)(uintptr_t)dt_u32();
    dt_stub(0x00493160, NULL, (void *)stubDirectInit);
}

static void setup_indent_set(DtCtx *c)
{
    ngBegin(c, 0);
    gTestIndent = (int)dt_u32();
    DT_ARG_INT(c, 0, (dt_u32() & 1) ? (uint32_t)dt_range(-5, 40) : dt_u32());
}

static void setup_indent(DtCtx *c)
{
    ngBegin(c, 0);
    gTestIndent = dt_range(0, 0x20);
    DT_ARG_INT(c, 0, (dt_u32() & 1) ? (uint32_t)dt_range(-40, 40) : dt_u32());
}

static int BMX_CDECL stubFreeModel(void *model)
{
    ngLog(0x49dcd0);
    ngLogPtr(model);
    return 1;
}

static void setup_model_free(DtCtx *c)
{
    uint8_t *obj = dt_alloc(0x40);

    ngBegin(c, 0);
    dt_fill(obj, 0x40);
    *(uint32_t *)(obj + 0x2c) = (dt_u32() & 3) ? (uint32_t)(uintptr_t)dt_alloc(8) : 0;
    dt_stub(0x0049dcd0, NULL, (void *)stubFreeModel);
    DT_ARG_PTR(c, 0, obj);
}

static int BMX_CDECL stubAnimInit(void)
{
    ngLog(0x499330);
    return (int)(dt_u32() & 1);
}

static void setup_model_init(DtCtx *c)
{
    ngBegin(c, 1);
    gTestZobjModelReady = (dt_u32() & 3) == 0;
    dt_stub(0x00499330, NULL, (void *)stubAnimInit);
}

DT_CASES(zinput_cases) = {
    {"zinputCreate", 0x004802f0, zinputCreate, 0, DT_RET_INT, setup_create, 40, 0},
    {"zdebugSetIndent", 0x00480290, zdebugSetIndent, 1, DT_RET_VOID, setup_indent_set, 100, 0},
    {"zdebugIndent", 0x004802c0, zdebugIndent, 1, DT_RET_VOID, setup_indent, 100, 0},
    {"zobjModelFree", 0x00488ea0, zobjModelFree, 1, DT_RET_INT, setup_model_free, 40, 0},
    {"zobjModelInitialize", 0x00488f40, zobjModelInitialize, 0, DT_RET_INT, setup_model_init, 40, 0},
};
