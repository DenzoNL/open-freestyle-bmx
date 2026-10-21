/*
 * Tests for the controller device list (pcinput.c). The DirectInput side (device setup, release, polling)
 * is replaced by logging stand-ins.
 */
#include "dt.h"
#include "dt_ngif.h"
#include "dt_zmem.h"
#include "zge/pc/pcinput.h"

typedef struct ZMemFreeList *(BMX_CDECL *ListCreateFn)(uint32_t, uint32_t, void *);
typedef void *(BMX_CDECL *ListAllocFn)(struct ZMemFreeList *);

static int gSetupAnswer;

static int BMX_CDECL stubSetup(int kind, void *device)
{
    ngLog(0x493310);
    ngLog((uint32_t)kind);
    ngLogPtr(device);
    return gSetupAnswer ? (int)(0x7100 + (dt_u32() & 0xff0)) : 0;
}

static void BMX_CDECL stubRelease(void *device)
{
    ngLog(0x4937f0);
    ngLogPtr(device);
}

static int BMX_CDECL stubPoll(void)
{
    ngLog(0x493910);
    return (int)dt_u32();
}

static void begin(DtCtx *c)
{
    ngBegin(c, 0);
    dt_zmem_env();
    gZpadDevices = DT_ORIG(0x00485e80, ListCreateFn)(kZpadDeviceBytes, 3, NULL);
    dt_stub(0x00493310, NULL, (void *)stubSetup);
    dt_stub(0x004937f0, NULL, (void *)stubRelease);
    dt_stub(0x00493910, NULL, (void *)stubPoll);
}

/* A list with 0..4 devices, filled with noise. */
static void *fill_devices(int count, void **last)
{
    void *first = NULL;

    for (int i = 0; i < count; i++) {
        void *d = DT_ORIG(0x00486050, ListAllocFn)(gZpadDevices);

        dt_fill(d, kZpadDeviceBytes);
        if (i == 0) {
            first = d;
        }
        *last = d;
    }
    return first;
}

static void setup_create(DtCtx *c)
{
    void *last = NULL;

    begin(c);
    gSetupAnswer = (dt_u32() % 4) != 0;
    fill_devices(dt_range(0, 3), &last);
    DT_ARG_INT(c, 0, (dt_u32() & 1) ? 2 : (int)dt_u32() % 5);
}

static void setup_destroy(DtCtx *c)
{
    void *last = NULL;

    begin(c);
    fill_devices(dt_range(1, 4), &last);
    DT_ARG_PTR(c, 0, last);
}

static void setup_get(DtCtx *c)
{
    void *last = NULL;

    begin(c);
    fill_devices(dt_range(0, 4), &last);
    DT_ARG_INT(c, 0, dt_range(0, 5));
}

static void setup_poll(DtCtx *c)
{
    begin(c);
}

DT_CASES(pcinput_cases) = {
    {"zpadCreateDevice", 0x00480370, zpadCreateDevice, 1, DT_RET_INT, setup_create, 200, 0},
    {"zpadDestroyDevice", 0x004803c0, zpadDestroyDevice, 1, DT_RET_INT, setup_destroy, 100, 0},
    {"zpadPoll", 0x004803f0, zpadPoll, 0, DT_RET_INT, setup_poll, 20, 0},
    {"zpadGetDevice", 0x00480400, zpadGetDevice, 1, DT_RET_INT, setup_get, 200, 0},
};
