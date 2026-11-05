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

static ZpadDiRecord *BMX_CDECL stubSetup(int kind, void *device)
{
    ngLog(0x493310);
    ngLog((uint32_t)kind);
    ngLogPtr(device);
    return gSetupAnswer ? (ZpadDiRecord *)(uintptr_t)(0x7100 + (dt_u32() & 0xff0)) : NULL;
}

static int BMX_CDECL stubRelease(void *device)
{
    ngLog(0x4937f0);
    ngLogPtr(device);
    return 1;
}

static int BMX_CDECL stubPoll(void)
{
    ngLog(0x493910);
    return 1; /* the real one always returns 1: the callers are optimised on that */
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

/* ---- DirectInput side ------------------------------------------------------------------------------ */

/* Stand-in DirectInput device: every method logs, answers from the generator and fills state buffers. */
static int32_t answer(void)
{
    switch (dt_u32() % 6) {
    case 0:
        return (int32_t)0x80004005;
    case 1:
        return (int32_t)0x8007001e;
    default:
        return 0;
    }
}

static int32_t BMX_STDCALL diAcquire(ZpadDiDevice *self)
{
    ngLog(0xd10001);
    ngLogPtr(self);
    return answer();
}

static int32_t BMX_STDCALL diUnacquire(ZpadDiDevice *self)
{
    ngLog(0xd10002);
    ngLogPtr(self);
    return 0;
}

static uint32_t BMX_STDCALL diRelease(ZpadDiDevice *self)
{
    ngLog(0xd10003);
    ngLogPtr(self);
    return 0;
}

static int32_t BMX_STDCALL diPoll(ZpadDiDevice *self)
{
    ngLog(0xd10004);
    ngLogPtr(self);
    return (dt_u32() % 6 == 0) ? (int32_t)0x80004005 : 0;
}

/* dt_fill is not usable from code that runs under test: its generator is not part of the snapshot. */
static void fill_words(void *p, uint32_t n)
{
    uint32_t *w = p;

    for (uint32_t i = 0; i < n / 4; i++) {
        w[i] = dt_u32();
    }
}

static int32_t BMX_STDCALL diGetState(ZpadDiDevice *self, uint32_t size, void *state)
{
    int32_t *words = state;

    ngLog(0xd10005);
    ngLogPtr(self);
    ngLog(size);
    if (size == 0x50) {
        for (int i = 0; i < 12; i++) {
            words[i] = (dt_u32() % 3 == 0) ? 0 : dt_range(-130, 130);
        }
        fill_words(&words[12], 0x20);
    } else {
        fill_words(state, size);
    }
    return answer();
}

static const ZpadDiVtbl kDiVtbl = {
    .release = diRelease,
    .acquire = diAcquire,
    .unacquire = diUnacquire,
    .getDeviceState = diGetState,
    .poll = diPoll,
};

static ZpadDiDevice *make_di(void)
{
    ZpadDiDevice *di = dt_alloc(sizeof *di);

    di->vtbl = &kDiVtbl;
    return di;
}

/* Byte pairs {index, slot} ended by `end`; `header` bytes of noise come first. */
static const uint8_t *make_map(int header, uint8_t end)
{
    uint8_t *map = dt_alloc(64);
    int n = dt_range(0, 8);
    int at = header;

    dt_fill(map, (uint32_t)header);
    for (int i = 0; i < n; i++) {
        map[at++] = (uint8_t)((end == 0xff && dt_u32() % 8 == 0) ? 0xfe : dt_range(0, 31));
        map[at++] = (uint8_t)dt_range(0, 21);
    }
    map[at] = end;
    return map;
}

static ZMemFreeList *make_records(int count)
{
    ZMemFreeList *list = DT_ORIG(0x00485e80, ListCreateFn)(sizeof(ZpadDiRecord), 3, NULL);

    for (int i = 0; i < count; i++) {
        ZpadDiRecord *r = DT_ORIG(0x00486050, ListAllocFn)(list);
        ZpadDevice *d = dt_alloc(sizeof *d);

        dt_fill(d, sizeof *d);
        d->record = r;
        r->device = d;
        r->di = (dt_u32() % 5 == 0) ? NULL : make_di();
        r->flags = dt_u32();
        r->keyMap = (dt_u32() % 3 == 0) ? NULL : make_map(0, 0xff);
        r->buttonMap = (const ZpadProfile *)make_map(8, 0xff);
    }
    return list;
}

static void begin_di(DtCtx *c, int count)
{
    ngBegin(c, 0);
    dt_zmem_env();
    gZpadDiRecords = make_records(count);
    gZpadDiActive = (dt_u32() % 5 != 0);
    gZpadKeyboard = (dt_u32() % 4 == 0) ? NULL : make_di();
    gZpadKeyboardFlags = dt_u32();
    gZpadKeyboardOk = (uint16_t)dt_u32();
    dt_fill(&gZpadKeyState, sizeof gZpadKeyState);
}

static void setup_di_poll(DtCtx *c)
{
    begin_di(c, dt_range(0, 4));
    if (gZpadKeyboard == NULL) {
        gZpadKeyboard = make_di();
    }
}

static void setup_di_activate(DtCtx *c)
{
    begin_di(c, dt_range(0, 4));
}

static void setup_di_release(DtCtx *c)
{
    ZpadDiRecord *record;
    ZMemFreeNode *node;

    begin_di(c, dt_range(1, 4));
    node = zmemFreeListFirst(gZpadDiRecords);
    for (int i = dt_range(0, 2); i > 0 && zmemFreeNodeNext(node) != NULL; i--) {
        node = zmemFreeNodeNext(node);
    }
    record = zmemFreeNodePayload(node);
    DT_ARG_PTR(c, 0, record->device);
}

static void setup_di_keymap(DtCtx *c)
{
    ZpadDevice *d = dt_alloc(sizeof *d);

    begin_di(c, 1);
    dt_fill(d, sizeof *d);
    d->record = (dt_u32() % 4 == 0) ? NULL : zmemFreeNodePayload(zmemFreeListFirst(gZpadDiRecords));
    DT_ARG_PTR(c, 0, d);
    DT_ARG_PTR(c, 1, make_map(0, 0xff));
}

DT_CASES(pcinput_cases) = {
    {"zpadReleaseDirectInput", 0x004937f0, zpadReleaseDirectInput, 1, DT_RET_INT, setup_di_release, 100, 0},
    {"zrwOnActivate", 0x00493830, zrwOnActivate, 0, DT_RET_INT, setup_di_activate, 200, 0},
    {"zrwOnDeactivate", 0x00493890, zrwOnDeactivate, 0, DT_RET_INT, setup_di_activate, 200, 0},
    {"zpadPollDevices", 0x00493910, zpadPollDevices, 0, DT_RET_INT, setup_di_poll, 1000, 0},
    {"zpadSetKeyMap", 0x00493cc0, zpadSetKeyMap, 2, DT_RET_INT, setup_di_keymap, 50, 0},
    {"zpadCreateDevice", 0x00480370, zpadCreateDevice, 1, DT_RET_INT, setup_create, 200, 0},
    {"zpadDestroyDevice", 0x004803c0, zpadDestroyDevice, 1, DT_RET_INT, setup_destroy, 100, 0},
    {"zpadPoll", 0x004803f0, zpadPoll, 0, DT_RET_INT, setup_poll, 20, 0},
    {"zpadGetDevice", 0x00480400, zpadGetDevice, 1, DT_RET_INT, setup_get, 200, 0},
};

/* ---- DirectInput start-up and device creation -----------------------------------------------------------
 */

static int gDiHr; /* answers of the stand-in DirectInput calls */
static int gFmtHr, gCoopHr;
static void *gFakeDevice;
static void *gFakeDirectInput;

static int BMX_CDECL stubQuery(int kind, void **out, int flags)
{
    ngLog(0x48ad20);
    ngLog((uint32_t)kind);
    ngLog((uint32_t)flags);
    *out = dt_alloc(8);
    *(uint32_t *)*out = 0x00400000u + (dt_u32() & 0xfff0);
    return 1;
}

static intptr_t BMX_STDCALL stubGetWindowLong(void *window, int index)
{
    ngLog(0x699794);
    ngLog((uint32_t)(uintptr_t)window);
    ngLog((uint32_t)index);
    return (intptr_t)(0x00400000 + (dt_u32() & 0xff0));
}

static int32_t BMX_STDCALL stubEnumDevices(ZpadDirectInput *self, uint32_t type,
                                           uint32_t(BMX_STDCALL *cb)(const void *, void *), void *ref,
                                           uint32_t flags)
{
    ngLog(0xd1e001);
    ngLogPtr(self);
    ngLog(type);
    ngLog((uint32_t)(uintptr_t)cb);
    ngLog((uint32_t)(uintptr_t)ref);
    ngLog(flags);
    return 0;
}

static int32_t BMX_STDCALL stubCreateDeviceEx(ZpadDirectInput *self, const ZpadGuid *guid,
                                              const ZpadGuid *iid, ZpadDiDevice **out, void *outer)
{
    ngLog(0xd1e002);
    ngLogPtr(self);
    ngLog(guid->data1);
    ngLog(guid->data2);
    ngLog(guid->data3);
    ngLog(iid->data1);
    ngLog((uint32_t)(uintptr_t)iid);
    ngLogPtr((const void *)out);
    ngLog((uint32_t)(uintptr_t)outer);
    if (gDiHr >= 0) {
        *out = gFakeDevice;
    }
    return gDiHr;
}

static int32_t BMX_STDCALL stubDirectInputCreate(void *instance, uint32_t version, const ZpadGuid *iid,
                                                 ZpadDirectInput **out, void *outer)
{
    ngLog(0x4afc58);
    ngLog((uint32_t)(uintptr_t)instance);
    ngLog(version);
    ngLog(iid->data1);
    ngLog((uint32_t)(uintptr_t)out);
    ngLog((uint32_t)(uintptr_t)outer);
    if (gDiHr >= 0) {
        *out = gFakeDirectInput;
    }
    return gDiHr;
}

static int32_t BMX_STDCALL stubSetDataFormat(ZpadDiDevice *self, const void *format)
{
    ngLog(0xd1e003);
    ngLogPtr(self);
    ngLogPtr(format);
    return gFmtHr;
}

static int32_t BMX_STDCALL stubSetCoop(ZpadDiDevice *self, void *window, uint32_t flags)
{
    ngLog(0xd1e004);
    ngLogPtr(self);
    ngLog((uint32_t)(uintptr_t)window);
    ngLog(flags);
    return gCoopHr;
}

static int32_t hrAnswer(void)
{
    return (dt_u32() & 3) == 0 ? (int32_t)0x80004005 : 0;
}

static void di_begin(DtCtx *c)
{
    ZpadDirectInputVtbl *dvt = dt_alloc(sizeof *dvt);
    ZpadDiVtbl *devt = dt_alloc(sizeof *devt);
    ZpadDirectInput *di = dt_alloc(sizeof *di);
    ZpadDiDevice *dev = dt_alloc(sizeof *dev);

    ngBegin(c, 1);
    memset(dvt, 0, sizeof *dvt);
    memset(devt, 0, sizeof *devt);
    dvt->enumDevices = stubEnumDevices;
    dvt->createDeviceEx = stubCreateDeviceEx;
    devt->setDataFormat = stubSetDataFormat;
    devt->setCooperativeLevel = stubSetCoop;
    di->vtbl = dvt;
    dev->vtbl = devt;
    gFakeDevice = dev;
    gFakeDirectInput = di;
    gDiHr = hrAnswer();
    gFmtHr = hrAnswer();
    gCoopHr = hrAnswer();
    gZpadDirectInput = di;
    *(void **)(uintptr_t)0x00699794 = (void *)stubGetWindowLong;
    dt_stub(0x0048ad20, NULL, (void *)stubQuery);
    dt_stub(0x004afc58, NULL, (void *)stubDirectInputCreate);
}

static void setup_di_init(DtCtx *c)
{
    di_begin(c);
    gZpadDiActive = (dt_u32() & 3) == 0;
    gZpadDiRecords = NULL;
    gZpadDirectInput = NULL;
}

static void setup_di_enum(DtCtx *c)
{
    uint8_t *inst = dt_alloc(0x20);
    uint32_t *guid = dt_alloc(16);

    ngBegin(c, 0);
    dt_fill(inst, 0x20);
    dt_fill(guid, 16);
    DT_ARG_PTR(c, 0, inst);
    DT_ARG_PTR(c, 1, guid);
}

static void setup_di_device(DtCtx *c)
{
    di_begin(c);
    DT_ARG_PTR(c, 0, dt_alloc(8));
    for (int i = 1; i <= 4; i++) {
        DT_ARG_INT(c, i, dt_u32());
    }
    DT_ARG_PTR(c, 5, dt_alloc(16));
    DT_ARG_INT(c, 6, dt_u32());
}

/* ---- device setup (profile lookup) ------------------------------------------------------------------- */

static int32_t BMX_STDCALL stubSetProperty(ZpadDiDevice *self, uintptr_t property, const void *header)
{
    const uint32_t *w = header;

    ngLog(0xd1e005);
    ngLogPtr(self);
    ngLog((uint32_t)property);
    for (uint32_t i = 0; i < w[0] / 4; i++) {
        ngLog(w[i]);
    }
    return hrAnswer();
}

static uint32_t nameCount(const ZpadProfile *p)
{
    uint32_t n = 0;

    while (p->names[n] != NULL) {
        n++;
    }
    return n;
}

static uint32_t guidCount(const ZpadProfile *p)
{
    uint32_t n = 0;

    while (p->guids != NULL && p->guids[n] != NULL) {
        n++;
    }
    return n;
}

/* A product the profile table knows by name, by GUID, by both of different profiles, or not at all. */
static int32_t BMX_STDCALL stubGetDeviceInfo(ZpadDiDevice *self, void *infop)
{
    uint8_t *info = infop;
    uint32_t profiles = 0;
    const uint32_t mode = dt_u32() % 5;

    ngLog(0xd1e006);
    ngLogPtr(self);
    ngLog(*(uint32_t *)info);
    while (gZpadProfiles[profiles] != NULL) {
        profiles++;
    }
    fill_words(info + 0x14, 0x10);
    for (int i = 0; i < 12; i++) {
        info[0x12c + i] = (char)dt_range('a', 'z');
    }
    info[0x12c + 12] = 0;
    if (mode >= 1 && mode <= 3) {
        const ZpadProfile *p = gZpadProfiles[dt_u32() % profiles];
        const uint32_t n = nameCount(p);

        if (n != 0) {
            const char *name = p->names[dt_u32() % n];
            uint32_t len = 0;

            while (name[len] != 0) {
                info[0x12c + len] = (uint8_t)name[len];
                len++;
            }
            info[0x12c + len] = 0;
        }
    }
    if (mode >= 3) {
        const ZpadProfile *p = gZpadProfiles[dt_u32() % profiles];
        const uint32_t n = guidCount(p);

        if (n != 0) {
            memcpy(info + 0x14, p->guids[dt_u32() % n], sizeof(ZpadGuid));
        }
    }
    return 0;
}

static void setup_setup_di(DtCtx *c)
{
    ZpadDirectInputVtbl *dvt = dt_alloc(sizeof *dvt);
    ZpadDiVtbl *devt = dt_alloc(sizeof *devt);
    ZpadDirectInput *di = dt_alloc(sizeof *di);
    ZpadDiDevice *dev = dt_alloc(sizeof *dev);
    ZpadDevice *device = dt_alloc(sizeof *device);

    ngBegin(c, 0);
    dt_zmem_env();
    memset(dvt, 0, sizeof *dvt);
    memset(devt, 0, sizeof *devt);
    dvt->createDeviceEx = stubCreateDeviceEx;
    devt->setDataFormat = stubSetDataFormat;
    devt->setCooperativeLevel = stubSetCoop;
    devt->setProperty = stubSetProperty;
    devt->getDeviceInfo = stubGetDeviceInfo;
    di->vtbl = dvt;
    dev->vtbl = devt;
    gFakeDevice = dev;
    gDiHr = (dt_u32() & 3) == 0 ? (int32_t)0x80004005 : 0;
    gFmtHr = (dt_u32() & 15) == 0 ? (int32_t)0x80004005 : 0;
    gCoopHr = (dt_u32() & 15) == 0 ? (int32_t)0x80004005 : 0;
    gZpadDirectInput = di;
    gZpadWindow = (void *)(uintptr_t)(dt_u32() | 1);
    fill_words(&gZpadJoystickGuid, sizeof gZpadJoystickGuid);
    gZpadDiRecords = make_records(dt_range(0, 2));
    gZpadKeyboard = (dt_u32() & 1) ? NULL : make_di();
    gZpadKeyboardFlags = dt_u32();
    dt_fill(device, sizeof *device);
    DT_ARG_INT(c, 0, (dt_u32() & 15) == 0 ? dt_range(-1, 4) : dt_range(0, 2));
    DT_ARG_PTR(c, 1, device);
}

DT_CASES(pcinput_cases2) = {
    {"zpadDirectInputInitialize", 0x00493160, zpadDirectInputInitialize, 0, DT_RET_INT, setup_di_init, 200,
     0},
    {"zpadEnumJoysticksCallback", 0x00493220, (void *)zpadEnumJoysticksCallback, 2, DT_RET_INT, setup_di_enum,
     20, 0},
    {"zpadCreateDiDevice", 0x00493760, (void *)zpadCreateDiDevice, 7, DT_RET_INT, setup_di_device, 300, 0},
    {"zpadSetupDirectInput", 0x00493310, zpadSetupDirectInput, 2, DT_RET_INT, setup_setup_di, 1500, 0},
};
