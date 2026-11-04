/*
 * Tests for zrwZgl.c (renderer glue). Every RenderWare / engine callee is stubbed on both sides: stubs log
 * their arguments by value and return deterministic, mostly successful results, so the frame loop,
 * initialisation chains and parameter plumbing are compared call for call. The Win32 imports the code
 * reaches through the import slots (message pump, cursor, dialog) are replaced by test functions in
 * setup. Not covered: the message-quit path of zrwPlayMovie (it ends in _exit) and the NULL result of
 * the node allocator is only reached through a stub.
 */
#include <stdarg.h>
#include <stddef.h>

#include "dt.h"
#include "dt_zmem.h"

#include "compat/crt.h"
#include "rw/rwworld.h"
#include "zge/com/zglrw/zrwzgl.h"
#include "zge/zdebug.h"

/* ---- log ------------------------------------------------------------------------------------------ */

enum { kLogMax = 4096 };
static uint32_t gLog[kLogMax];
static uint32_t gLogN;

static void lg(uint32_t v)
{
    if (gLogN < kLogMax) {
        gLog[gLogN] = v;
    }
    gLogN++;
}

static void lgp(const void *p)
{
    lg(dt_canon((uint32_t)(uintptr_t)p));
}

static void lgf(float f)
{
    uint32_t u;

    memcpy(&u, &f, 4);
    lg(u);
}

static uint32_t hashStr(const char *s)
{
    uint32_t h = 5381;

    while (*s) {
        h = h * 33 + (uint8_t)*s++;
    }
    return h;
}

static int ok(void)
{
    return dt_u32() % 6 != 0;
}

/* printf stand-in: logs the format and the first argument. */
static void BMX_CDECL stubPrint(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    lg(0x480240);
    lg(hashStr(fmt));
    if (strchr(fmt, '%') != NULL) {
        lg(va_arg(ap, uint32_t));
    }
    va_end(ap);
}

static void BMX_CDECL stubFail(const char *why, const char *file, int line, int flags)
{
    lg(0x47ed10);
    lg(hashStr(why));
    lg(hashStr(file));
    lg((uint32_t)line);
    lg((uint32_t)flags);
}

static void begin(const DtCtx *c, int stubAssert)
{
    dt_zmem_env();
    gLogN = 0;
    gZdebugPrint = stubPrint;
    if (stubAssert) {
        dt_stub(0x0047ed10, NULL, (void *)stubFail);
    }
}

/* ---- stubs ---------------------------------------------------------------------------------------- */

#define S0(name, id)                                                                                         \
    static int BMX_CDECL name(void)                                                                          \
    {                                                                                                        \
        lg(id);                                                                                              \
        return ok();                                                                                         \
    }
#define S1(name, id)                                                                                         \
    static int BMX_CDECL name(uint32_t a)                                                                    \
    {                                                                                                        \
        lg(id);                                                                                              \
        lgp((void *)(uintptr_t)a);                                                                           \
        return ok();                                                                                         \
    }
#define S2(name, id)                                                                                         \
    static int BMX_CDECL name(uint32_t a, uint32_t b)                                                        \
    {                                                                                                        \
        lg(id);                                                                                              \
        lgp((void *)(uintptr_t)a);                                                                           \
        lgp((void *)(uintptr_t)b);                                                                           \
        return ok();                                                                                         \
    }
#define S3(name, id)                                                                                         \
    static int BMX_CDECL name(uint32_t a, uint32_t b, uint32_t c)                                            \
    {                                                                                                        \
        lg(id);                                                                                              \
        lgp((void *)(uintptr_t)a);                                                                           \
        lgp((void *)(uintptr_t)b);                                                                           \
        lgp((void *)(uintptr_t)c);                                                                           \
        return ok();                                                                                         \
    }

S2(sRenderState, 0x4db820)
S2(sViewport, 0x4930a0)
S2(sViewWindow, 0x4930b0)
S2(sCameraPlane, 0x4930c0)
S2(sCameraOffset, 0x4930e0)
S2(sCameraFlags, 0x493100)
S1(sMaterialParam, 0x498b70)
S1(sMaterialValue, 0x498b90)

static void BMX_CDECL sClipPlane(const void *plane, int zero, float value)
{
    lg(0x4929f0);
    lgp(plane);
    lg((uint32_t)zero);
    lgf(value);
}

static float BMX_CDECL sRenderStateFloat(void)
{
    lg(0x4db800);
    return dt_float_edgy(-100.0f, 100.0f);
}

/* ---- list fixtures --------------------------------------------------------------------------------- */

typedef struct Node {
    struct Node *next;
    struct Node *prev;
    uint32_t pad[3];
} Node;

static ZrwList *makeList(int n, Node **nodes)
{
    ZrwList *l = dt_alloc(sizeof *l);

    for (int i = 0; i < n; i++) {
        nodes[i] = dt_alloc(sizeof(Node));
        dt_fill(nodes[i], sizeof(Node));
    }
    for (int i = 0; i < n; i++) {
        nodes[i]->next = i + 1 < n ? nodes[i + 1] : NULL;
        nodes[i]->prev = i > 0 ? nodes[i - 1] : NULL;
    }
    l->head = n > 0 ? nodes[0] : NULL;
    l->tail = n > 0 ? nodes[n - 1] : NULL;
    l->count = n;
    return l;
}

static void setup_list_init(DtCtx *c)
{
    dt_seed(c);
    ZrwList *l = dt_alloc(sizeof *l);

    dt_fill(l, sizeof *l);
    DT_ARG_PTR(c, 0, l);
}

static void setup_list_add(DtCtx *c)
{
    dt_seed(c);
    Node *nodes[5];
    ZrwList *l;
    Node *fresh;
    const int n = dt_range(0, 5);

    l = makeList(n, nodes);
    fresh = dt_alloc(sizeof *fresh);
    dt_fill(fresh, sizeof *fresh);
    DT_ARG_PTR(c, 0, l);
    DT_ARG_PTR(c, 1, fresh);
}

static void setup_list_remove(DtCtx *c)
{
    dt_seed(c);
    Node *nodes[5];
    ZrwList *l;
    const int n = dt_range(1, 5);

    l = makeList(n, nodes);
    DT_ARG_PTR(c, 0, l);
    DT_ARG_PTR(c, 1, nodes[dt_range(0, n - 1)]);
}

/* ---- small functions ------------------------------------------------------------------------------- */

static void setup_none(DtCtx *c)
{
    dt_seed(c);
    begin(c, 0);
}

static void setup_material_usage(DtCtx *c)
{
    dt_seed(c);
    ZMemFreeList *list;

    begin(c, 0);
    list = DT_ORIG(0x00485e80, ZMemFreeList * (*)(uint32_t, uint32_t, ZMemDestroyFn))(
        (uint32_t)dt_range(4, 40), (uint32_t)dt_range(1, 6), NULL);
    for (int i = dt_range(0, 9); i > 0; i--) {
        DT_ORIG(0x00486050, void *(*)(ZMemFreeList *))(list);
    }
    ORIG_VAR(0x006723b0, ZMemFreeList *, gMaterials) = list;
}

static void setup_debug_output(DtCtx *c)
{
    dt_seed(c);
    char *s = dt_rand_str(20, 0);

    begin(c, 0);
    DT_ARG_PTR(c, 0, s);
}

static int sFileInitResult;

static int BMX_CDECL sFileInit(void)
{
    lg(0x49ea10);
    return sFileInitResult;
}

static int BMX_CDECL sFileShutdown(void)
{
    lg(0x49ea60);
    return 0;
}

static int BMX_CDECL sRamdiskInit(void)
{
    lg(0x49f450);
    return dt_range(0, 2);
}

static void setup_filesystems(DtCtx *c)
{
    dt_seed(c);
    uint32_t *table = dt_alloc(16);

    begin(c, 0);
    dt_fill(table, 16);
    sFileInitResult = dt_range(0, 2);
    dt_stub(0x0049ea10, NULL, (void *)sFileInit);
    dt_stub(0x0049ea60, NULL, (void *)sFileShutdown);
    dt_stub(0x0049f450, NULL, (void *)sRamdiskInit);
    DT_ARG_PTR(c, 0, table);
}

/* ---- parameters ------------------------------------------------------------------------------------ */

static void stubParamCallees(void)
{
    dt_stub(0x004930a0, NULL, (void *)sViewport);
    dt_stub(0x004930b0, NULL, (void *)sViewWindow);
    dt_stub(0x004930c0, NULL, (void *)sCameraPlane);
    dt_stub(0x004930e0, NULL, (void *)sCameraOffset);
    dt_stub(0x00493100, NULL, (void *)sCameraFlags);
    dt_stub(0x00498b70, NULL, (void *)sMaterialParam);
    dt_stub(0x00498b90, NULL, (void *)sMaterialValue);
    dt_stub(0x004929f0, NULL, (void *)sClipPlane);
    dt_stub(0x004db800, NULL, (void *)sRenderStateFloat);
}

static int randomParamId(void)
{
    return dt_u32() % 8 == 0 ? dt_range(-2, 0x1a) : dt_range(0, 0x15);
}

static void setup_set_parameter(DtCtx *c)
{
    dt_seed(c);
    const int id = randomParamId();
    float *value = dt_alloc(0x40);
    uint32_t *extra = dt_alloc(0x20);
    uint32_t pointerValue;

    begin(c, 1);
    stubParamCallees();
    dt_fill(value, 0x40);
    dt_fill(extra, 0x20);
    ((float *)value)[0] = dt_float_edgy(-100.0f, 100.0f);
    ((float *)extra)[0] = dt_float_edgy(-100.0f, 100.0f);
    gZrwFlags = dt_u32();
    gZrwKeysLocked = (int)(dt_u32() & 1);
    pointerValue = (id == 0xc || id == 0xd || id == 0x10) ? dt_u32() : (uint32_t)(uintptr_t)value;
    DT_ARG_INT(c, 0, id);
    DT_ARG_INT(c, 1, pointerValue);
    DT_ARG_PTR(c, 2, extra);
}

static void setup_get_parameter(DtCtx *c)
{
    dt_seed(c);
    const int id = randomParamId();
    uint32_t *out = dt_alloc(0x40);
    float *extra = dt_alloc(0x10);

    begin(c, 1);
    stubParamCallees();
    dt_fill(out, 0x40);
    gZrwFrameLimit = dt_float_edgy(-100.0f, 100.0f);
    gZrwScale = dt_float_edgy(-100.0f, 100.0f);
    gZrwInvScale = dt_float_edgy(-100.0f, 100.0f);
    gZrwClearOn = (int)dt_u32();
    gZrwClearColor = dt_u32();
    gZrwScreenW = dt_u32();
    gZrwScreenH = dt_u32();
    dt_fill(&gZrwName, 0x20);
    for (int i = 0; i < 3; i++) {
        extra[i] = dt_float_edgy(-100.0f, 100.0f);
    }
    DT_ARG_INT(c, 0, id);
    DT_ARG_PTR(c, 1, out);
    DT_ARG_PTR(c, 2, (id == 6 && (dt_u32() & 1)) ? NULL : (void *)extra);
}

/* ---- initialisation chains ------------------------------------------------------------------------- */

S0(sInitTexture, 0x5623f0)
S0(sInitMaterial, 0x561018)
S0(sInitModel, 0x560e78)
S0(sInitLight, 0x563420)
S0(sInitCol, 0x562060)
S0(sInitPrim, 0x561720)
S0(sExitPrim, 0x561724)
S0(sExitCol, 0x562064)
S0(sExitLight, 0x563424)
S0(sExitModel, 0x560e7c)
S0(sExitMaterial, 0x56101c)
S0(sExitTexture, 0x5623f4)
S0(sWorldShutdown, 0x496de0)

static void *BMX_CDECL sCreateWorld(void)
{
    lg(0x496c40);
    return ok() ? (void *)0x1234 : NULL;
}

static void BMX_CDECL sApplicationError(const char *text)
{
    lg(0x489cc0);
    lg(hashStr(text));
}

static void stubSlot(uint32_t slot, void *fn)
{
    dt_stub(*(uint32_t *)(uintptr_t)slot, NULL, fn);
}

static void setup_graphic_init(DtCtx *c)
{
    dt_seed(c);
    begin(c, 0);
    gZrwFlags = dt_u32();
    gZrwWantW = dt_range(0, 2000);
    gZrwWantH = dt_range(0, 2000);
    dt_fill(&gZrwScreenW, 0x10);
    gZrwNodePool = NULL;
    stubSlot(0x5623f0, (void *)sInitTexture);
    stubSlot(0x561018, (void *)sInitMaterial);
    stubSlot(0x560e78, (void *)sInitModel);
    stubSlot(0x563420, (void *)sInitLight);
    stubSlot(0x562060, (void *)sInitCol);
    stubSlot(0x561720, (void *)sInitPrim);
    dt_stub(0x00496c40, NULL, (void *)sCreateWorld);
    dt_stub(0x00489cc0, NULL, (void *)sApplicationError);
}

static void setup_graphic_exit(DtCtx *c)
{
    dt_seed(c);
    begin(c, 1);
    gZrwInitialized = (int)(dt_u32() % 4 != 0);
    gZrwNodePool =
        DT_ORIG(0x00485e80, ZMemFreeList * (*)(uint32_t, uint32_t, ZMemDestroyFn))(0x14, 0x20, NULL);
    stubSlot(0x561724, (void *)sExitPrim);
    stubSlot(0x562064, (void *)sExitCol);
    stubSlot(0x563424, (void *)sExitLight);
    stubSlot(0x560e7c, (void *)sExitModel);
    stubSlot(0x56101c, (void *)sExitMaterial);
    stubSlot(0x5623f4, (void *)sExitTexture);
    dt_stub(0x00496de0, NULL, (void *)sWorldShutdown);
}

S2(sEngineInit, 0x4d01f0)
S0(sWorldPlugin, 0x4bc1c0)
S0(sAnimPlugin, 0x500250)
S0(sBonePlugin, 0x505870)
S0(sLodPlugin, 0x5062b0)
S0(sPvsPlugin, 0x4ccac0)
S0(sMatPlugin, 0x498b10)
S0(sMdlPlugin, 0x488890)
S0(sEngineStart, 0x4cfd50)
S0(sSelectDevice, 0x489d70)
S0(sSelectDefault, 0x489ef0)

static int BMX_CDECL sEngineOpen(const uint32_t *params)
{
    lg(0x4d0020);
    lg(*params);
    return 1;
}

static void setup_attach(DtCtx *c)
{
    dt_seed(c);
    begin(c, 0);
    gZrwSkipDevice = (int)(dt_u32() & 1);
    gZrwEngineInit = dt_u32();
    gZrwEngineOpen = dt_u32();
    dt_stub(0x004d01f0, NULL, (void *)sEngineInit);
    dt_stub(0x004bc1c0, NULL, (void *)sWorldPlugin);
    dt_stub(0x00500250, NULL, (void *)sAnimPlugin);
    dt_stub(0x00505870, NULL, (void *)sBonePlugin);
    dt_stub(0x005062b0, NULL, (void *)sLodPlugin);
    dt_stub(0x004ccac0, NULL, (void *)sPvsPlugin);
    dt_stub(0x00498b10, NULL, (void *)sMatPlugin);
    dt_stub(0x00488890, NULL, (void *)sMdlPlugin);
    dt_stub(0x004d0020, NULL, (void *)sEngineOpen);
    dt_stub(0x004cfd50, NULL, (void *)sEngineStart);
    dt_stub(0x00489d70, NULL, (void *)sSelectDevice);
    dt_stub(0x00489ef0, NULL, (void *)sSelectDefault);
    dt_stub(0x00489cc0, NULL, (void *)sApplicationError);
    DT_ARG_INT(c, 0, dt_u32());
}

static uint32_t sMemoryInterface[4];

static const uint32_t *BMX_CDECL sMemInterface(void)
{
    lg(0x4dba80);
    return sMemoryInterface;
}

static int BMX_CDECL sZgeInit(const char *argument)
{
    lg(0x49f900);
    lg(argument != NULL ? hashStr(argument) : 0);
    lg(argument == &gZrwArgBuffer);
    return ok();
}

S0(sNgifInit, 0x45b270)

static void setup_init_app(DtCtx *c)
{
    dt_seed(c);
    static const char kAlphabet[] = "ab  c";
    char *cmd = NULL;

    begin(c, 1);
    for (int i = 0; i < 4; i++) {
        sMemoryInterface[i] = dt_u32();
    }
    gZrwInitialized = (int)(dt_u32() % 4 == 0);
    dt_fill(&gZrwArgBuffer, 0x20);
    dt_stub(0x004dba80, NULL, (void *)sMemInterface);
    dt_stub(0x0049f900, NULL, (void *)sZgeInit);
    dt_stub(0x0045b270, NULL, (void *)sNgifInit);
    if (dt_u32() % 8 != 0) {
        const int len = dt_range(0, 18);

        cmd = dt_alloc((uint32_t)len + 1);
        for (int i = 0; i < len; i++) {
            cmd[i] = kAlphabet[dt_u32() % 5];
        }
    }
    DT_ARG_PTR(c, 0, cmd);
}

/* ---- update lists ---------------------------------------------------------------------------------- */

static int BMX_CDECL cbA(void *arg)
{
    lg(0xa);
    lgp(arg);
    return ok();
}

static int BMX_CDECL cbB(void *arg)
{
    lg(0xb);
    lgp(arg);
    return ok();
}

/* Fills list `l` with n callback nodes. */
static void fillCallbacks(ZrwList *l, int n)
{
    ZrwUpdateNode *prev = NULL;

    zrwListInit(l);
    for (int i = 0; i < n; i++) {
        ZrwUpdateNode *node = dt_alloc(sizeof *node);

        node->fn = (dt_u32() & 1) ? cbA : cbB;
        node->arg = (void *)(uintptr_t)(0x100 + i);
        node->list = l;
        node->prev = prev;
        if (prev != NULL) {
            prev->next = node;
        } else {
            l->head = node;
        }
        prev = node;
    }
    l->tail = prev;
    l->count = n;
}

static void fillAllLists(void)
{
    for (int i = 0; i < kZrwUpdateListCount; i++) {
        fillCallbacks(&gZrwUpdateLists[i], dt_range(0, 3));
    }
}

static void setup_run_callbacks(DtCtx *c)
{
    dt_seed(c);
    ZrwList *l = dt_alloc(sizeof *l);

    begin(c, 0);
    fillCallbacks(l, dt_range(0, 4));
    DT_ARG_PTR(c, 0, l);
}

static void setup_run_update_list(DtCtx *c)
{
    dt_seed(c);
    begin(c, 0);
    fillAllLists();
    DT_ARG_INT(c, 0, dt_range(0, 8));
}

static int BMX_CDECL sNgifFrame(float dt)
{
    lg(0x45b8a0);
    lgf(dt);
    return ok();
}

static void setup_on_idle(DtCtx *c)
{
    dt_seed(c);
    begin(c, 0);
    fillAllLists();
    gZrwLastTime = (uint32_t)dt_range(0, 1100) + (dt_u32() % 8 == 0 ? 0xfffff000u : 0);
    gZrwFpsTime = (uint32_t)dt_range(0, 1100);
    gZrwFps = dt_range(0, 100);
    gZrwFrames = dt_range(0, 100);
    gZrwQuit = 0;
    dt_stub(0x0045b8a0, NULL, (void *)sNgifFrame);
}

/* ---- frame ----------------------------------------------------------------------------------------- */

typedef struct Cam {
    uint8_t pad0[8];
    uint8_t flags;
    uint8_t pad9[7];
    struct Cam *next;
} Cam;

static Cam *gCams;

static Cam *BMX_CDECL sCamFirst(void)
{
    lg(0x492eb0);
    return gCams;
}

static Cam *BMX_CDECL sCamNext(Cam *cam)
{
    lg(0x492ed0);
    lgp(cam);
    return cam->next;
}

S1(sCamBegin, 0x492f00)
S0(sCamEnd, 0x492fe0)
S0(sTxrFlush, 0x497620)

static void *BMX_CDECL sCamHandle(Cam *cam)
{
    lg(0x493060);
    lgp(cam);
    return (uint8_t *)cam + 0x40;
}

static int BMX_CDECL sCameraClear(void *camera, const uint32_t *color, int flags)
{
    lg(0x4de8a0);
    lgp(camera);
    lg(*color);
    lg((uint32_t)flags);
    return 1;
}

static void BMX_CDECL sErrorGet(uint32_t *error)
{
    lg(0x4ded90);
    error[0] = 7;
    error[1] = dt_u32() % 5 != 0 ? 0x80000000u : dt_u32();
}

static void *BMX_CDECL sGetFrame(void *camera)
{
    lg(0x4dea80);
    lgp(camera);
    return (uint8_t *)camera + 0x10;
}

static uint8_t *BMX_CDECL sFrameMatrix(void *frame)
{
    lg(0x4d2b00);
    lgp(frame);
    return (uint8_t *)0x7000;
}

S3(sShowRaster, 0x4dcfd0)
S1(sLightBegin, 0x4cc270)
S2(sLightMatrix, 0x4cc510)
S3(sLightParams, 0x4cc670)
S1(sLightEnd, 0x4cc2c0)

static void stubFrame(void)
{
    dt_stub(0x00492eb0, NULL, (void *)sCamFirst);
    dt_stub(0x00492ed0, NULL, (void *)sCamNext);
    dt_stub(0x00492f00, NULL, (void *)sCamBegin);
    dt_stub(0x00492fe0, NULL, (void *)sCamEnd);
    dt_stub(0x00493060, NULL, (void *)sCamHandle);
    dt_stub(0x00497620, NULL, (void *)sTxrFlush);
    dt_stub(0x004de8a0, NULL, (void *)sCameraClear);
    dt_stub(0x004ded90, NULL, (void *)sErrorGet);
    dt_stub(0x004dea80, NULL, (void *)sGetFrame);
    dt_stub(0x004d2b00, NULL, (void *)sFrameMatrix);
    dt_stub(0x004dcfd0, NULL, (void *)sShowRaster);
    dt_stub(0x004cc270, NULL, (void *)sLightBegin);
    dt_stub(0x004cc510, NULL, (void *)sLightMatrix);
    dt_stub(0x004cc670, NULL, (void *)sLightParams);
    dt_stub(0x004cc2c0, (void *)RpPVSUnhook, (void *)sLightEnd);
    dt_stub(0x004db820, NULL, (void *)sRenderState);
}

static void setup_draw_frame(DtCtx *c)
{
    dt_seed(c);
    const int n = dt_range(0, 3);
    Cam *prev = NULL;

    begin(c, 1);
    fillAllLists();
    stubFrame();
    gCams = NULL;
    for (int i = 0; i < n; i++) {
        Cam *cam = dt_alloc(sizeof *cam);

        cam->flags = (uint8_t)dt_u32();
        cam->next = NULL;
        if (prev != NULL) {
            prev->next = cam;
        } else {
            gCams = cam;
        }
        prev = cam;
    }
    gZrwRaster = (dt_u32() % 5 != 0) ? (void *)0x5000 : NULL;
    gZrwWindow = (void *)(uintptr_t)dt_u32();
    gZrwClearOn = (int)(dt_u32() & 1);
    gZrwClearColor = dt_u32();
    gZrwDebugLightOn = (int)(dt_u32() & 1);
    gZrwDebugLight = (void *)(uintptr_t)dt_range(0x6000, 0x7000);
    gZrwLightParamA = dt_u32();
    gZrwLightParamB = dt_u32();
    gZrwRenderFlags = dt_u32();
}

static void setup_show_raster(DtCtx *c)
{
    dt_seed(c);
    begin(c, 0);
    dt_stub(0x004dcfd0, NULL, (void *)sShowRaster);
    gZrwRaster = (void *)(uintptr_t)dt_u32();
    gZrwWindow = (void *)(uintptr_t)dt_u32();
    DT_ARG_INT(c, 0, dt_u32() & 3);
}

/* ---- registration ---------------------------------------------------------------------------------- */

static ZrwUpdateNode *gSpareNode;

static void *BMX_CDECL sPoolAlloc(ZMemFreeList *pool)
{
    lg(0x486050);
    lgp(pool);
    return gSpareNode;
}

static void BMX_CDECL sPoolFree(ZMemFreeList *pool, void *payload)
{
    lg(0x486170);
    lgp(pool);
    lgp(payload);
}

static void setup_add_callback(DtCtx *c)
{
    dt_seed(c);
    Node *nodes[4];
    ZrwList *l;

    begin(c, 1);
    l = makeList(dt_range(0, 3), nodes);
    gSpareNode = (dt_u32() % 5 != 0) ? dt_alloc(sizeof(ZrwUpdateNode)) : NULL;
    gZrwNodePool = (ZMemFreeList *)(uintptr_t)dt_range(0x100, 0x200);
    dt_stub(0x00486050, NULL, (void *)sPoolAlloc);
    DT_ARG_PTR(c, 0, l);
    DT_ARG_PTR(c, 1, cbA);
    DT_ARG_INT(c, 2, dt_u32());
    DT_ARG_INT(c, 3, dt_u32() & 1);
}

static void setup_register(DtCtx *c)
{
    dt_seed(c);
    int type;

    begin(c, 1);
    fillAllLists();
    gSpareNode = (dt_u32() % 5 != 0) ? dt_alloc(sizeof(ZrwUpdateNode)) : NULL;
    gZrwNodePool = (ZMemFreeList *)(uintptr_t)dt_range(0x100, 0x200);
    gZrwInitialized = (int)(dt_u32() % 5 != 0);
    dt_stub(0x00486050, NULL, (void *)sPoolAlloc);
    type = dt_u32() % 5 == 0 ? dt_range(-1, 10) : dt_range(0, 8);
    if (type < 0 || type > 8) {
        memset((void *)(uintptr_t)(0x672768 + type * 12), 0, 12); /* the list the original links into */
    }
    DT_ARG_INT(c, 0, type);
    DT_ARG_PTR(c, 1, cbB);
    DT_ARG_INT(c, 2, dt_u32());
    DT_ARG_INT(c, 3, dt_u32() & 1);
}

static void setup_unregister(DtCtx *c)
{
    dt_seed(c);
    ZrwUpdateNode *picked = NULL;

    begin(c, 1);
    fillAllLists();
    gZrwNodePool = (ZMemFreeList *)(uintptr_t)dt_range(0x100, 0x200);
    dt_stub(0x00486170, NULL, (void *)sPoolFree);
    for (int i = 0; i < kZrwUpdateListCount && picked == NULL; i++) {
        ZrwUpdateNode *n = gZrwUpdateLists[i].head;

        if (n != NULL && (dt_u32() & 1)) {
            picked = n;
            if (n->next != NULL && (dt_u32() & 1)) {
                picked = n->next;
            }
        }
    }
    DT_ARG_PTR(c, 0, dt_u32() % 8 == 0 ? NULL : picked);
}

/* ---- render states --------------------------------------------------------------------------------- */

typedef void(BMX_CDECL *StateHookFn)(uint32_t, int);

static void BMX_CDECL sStateHook(uint32_t flags, int extra)
{
    lg(0x561490);
    lg(flags);
    lg((uint32_t)extra);
}

static void setup_reset_state(DtCtx *c)
{
    dt_seed(c);
    begin(c, 0);
    dt_stub(0x004db820, NULL, (void *)sRenderState);
    gZrwRenderFlags = dt_u32();
    gZrwRenderExtra = (const void *)(uintptr_t)dt_u32();
}

static void setup_set_state(DtCtx *c)
{
    dt_seed(c);
    uint32_t flags;

    begin(c, 0);
    dt_stub(0x004db820, NULL, (void *)sRenderState);
    gZrwRenderFlags = dt_u32() & 0xffff;
    gZrwRenderExtra = (const void *)(uintptr_t)dt_range(0, 3);
    flags = (dt_u32() & 1) ? gZrwRenderFlags ^ (1u << dt_range(0, 15)) : dt_u32() & 0xffff;
    if ((dt_u32() & 3) == 0) {
        flags = gZrwRenderFlags;
    }
    DT_ARG_INT(c, 0, flags);
    DT_ARG_INT(c, 1, dt_range(0, 3));
}

static void setup_push_state(DtCtx *c)
{
    dt_seed(c);
    begin(c, 0);
    gZrwStateDepth = dt_range(0, 4);
    gZrwRenderFlags = dt_u32();
    gZrwRenderExtra = (const void *)(uintptr_t)dt_u32();
    dt_fill(&gZrwStateStack, sizeof(ZrwStateStack));
}

static void setup_pop_state(DtCtx *c)
{
    dt_seed(c);
    begin(c, 0);
    gZrwStateDepth = dt_range(0, 4);
    gZrwRenderFlags = dt_u32() & 0xff;
    gZrwRenderExtra = (const void *)(uintptr_t)dt_range(0, 2);
    for (int i = 0; i < kZrwStateStackSize; i++) {
        gZrwStateStack[i].flags = (dt_u32() & 1) ? gZrwRenderFlags : dt_u32() & 0xff;
        gZrwStateStack[i].raster = (dt_u32() & 1) ? gZrwRenderExtra : (const void *)(uintptr_t)dt_range(0, 2);
    }
    ORIG_VAR(0x00561490, StateHookFn, gStateHook) = sStateHook;
}

static void setup_convert_matrix(DtCtx *c)
{
    dt_seed(c);
    float *in = dt_alloc(16 * 4);
    float *out = (c->seed % 4 == 3) ? in : dt_alloc(16 * 4);

    begin(c, 0);
    for (int i = 0; i < 16; i++) {
        in[i] = dt_float_edgy(-1000.0f, 1000.0f);
    }
    if (out != in) {
        dt_fill(out, 64);
    }
    gZrwOriginX = dt_float_edgy(-1000.0f, 1000.0f);
    gZrwOriginY = dt_float_edgy(-1000.0f, 1000.0f);
    gZrwOriginZ = dt_float_edgy(-1000.0f, 1000.0f);
    gZrwScale = dt_float_edgy(-10.0f, 10.0f);
    DT_ARG_PTR(c, 0, out);
    DT_ARG_PTR(c, 1, in);
}

/* ---- device selection -------------------------------------------------------------------------------- */

static int sSubSystems;

static int BMX_CDECL sNumSubSystems(void)
{
    lg(0x4cf870);
    return sSubSystems;
}

static int BMX_CDECL sSubSystemInfo(uint8_t *info, int index)
{
    lg(0x4cf8b0);
    lgp(info);
    lg((uint32_t)index);
    return 1;
}

static int BMX_CDECL sCurrentSubSystem(void)
{
    lg(0x4cf940);
    return dt_range(0, 3);
}

static int BMX_CDECL sSetSubSystem(int index)
{
    lg(0x4cf980);
    lg((uint32_t)index);
    return ok();
}

static uint32_t sModes;
static uint32_t sModeData[16][4];

static uint32_t BMX_CDECL sNumVideoModes(void)
{
    lg(0x4cf9f0);
    return sModes;
}

static int BMX_CDECL sVideoModeInfo(uint32_t *info, uint32_t index)
{
    lg(0x4cfa60);
    lg(index);
    memcpy(info, sModeData[index & 15], 16);
    return 1;
}

static int BMX_CDECL sSetVideoMode(uint32_t index)
{
    lg(0x4cfb40);
    lg(index);
    return ok();
}

static int BMX_STDCALL sShowCursor(int show)
{
    lg(0x6997a8);
    lg((uint32_t)show);
    return 0;
}

static int BMX_STDCALL sDialogBox(void *instance, uintptr_t name, void *parent, void *proc, int param)
{
    lg(0x699778);
    lgp(instance);
    lg((uint32_t)name);
    lgp(parent);
    lgp(proc);
    lg((uint32_t)param);
    return (int)(dt_u32() & 1);
}

static void setupModes(void)
{
    sModes = (uint32_t)dt_range(0, 12);
    for (int i = 0; i < 16; i++) {
        sModeData[i][0] = (uint32_t)dt_range(1, 4) * 320;
        sModeData[i][1] = (uint32_t)dt_range(1, 3) * 240;
        sModeData[i][2] = dt_u32() & 1 ? 16u : 32u;
        sModeData[i][3] = dt_u32() & 0x1ff; /* bit 0 of the flags byte: full screen */
    }
}

static void stubDevice(void)
{
    dt_stub(0x004cf870, NULL, (void *)sNumSubSystems);
    dt_stub(0x004cf8b0, NULL, (void *)sSubSystemInfo);
    dt_stub(0x004cf940, NULL, (void *)sCurrentSubSystem);
    dt_stub(0x004cf980, NULL, (void *)sSetSubSystem);
    dt_stub(0x004cf9f0, NULL, (void *)sNumVideoModes);
    dt_stub(0x004cfa60, NULL, (void *)sVideoModeInfo);
    dt_stub(0x004cfb40, NULL, (void *)sSetVideoMode);
    ORIG_VAR(0x006997a8, void *, gShowCursorSlot) = (void *)sShowCursor;
    ORIG_VAR(0x00699778, void *, gDialogBoxSlot) = (void *)sDialogBox;
}

static void setup_select_device(DtCtx *c)
{
    dt_seed(c);
    begin(c, 0);
    setupModes();
    stubDevice();
    sSubSystems = dt_range(0, 11);
    ORIG_VAR(0x00672750, int, gSubCount) = (int)dt_u32();
    ORIG_VAR(0x00672754, int, gSub) = (int)dt_u32();
    ORIG_VAR(0x00672758, uint32_t, gMode) = dt_u32();
    gZrwWindow = (void *)(uintptr_t)dt_range(0x100, 0x200);
    ORIG_VAR(0x00697784, void *, gInstance) = (void *)(uintptr_t)dt_range(0x300, 0x400);
    ORIG_VAR(0x006977a0, int, gWantW2) = (dt_u32() % 3 == 0) ? 0 : dt_range(1, 4) * 320;
    ORIG_VAR(0x006977a4, int, gWantH2) = (dt_u32() % 5 == 0) ? 0 : dt_range(1, 3) * 240;
    ORIG_VAR(0x006977a8, int, gWantDepth) = (dt_u32() % 5 == 0) ? 0 : ((dt_u32() & 1) ? 16 : 32);
    gZrwFullscreen = (int)dt_u32();
    gZrwWantW = dt_range(0, 100);
    gZrwWantH = dt_range(0, 100);
}

static void setup_select_default(DtCtx *c)
{
    dt_seed(c);
    begin(c, 0);
    setupModes();
    stubDevice();
    ORIG_VAR(0x00672754, int, gSub) = dt_range(0, 3);
    ORIG_VAR(0x00672758, uint32_t, gMode) = (uint32_t)dt_range(0, 12);
    gZrwFullscreen = (int)dt_u32();
}

/* ---- movie ----------------------------------------------------------------------------------------- */

static void *BMX_CDECL sFopen(const char *path, const char *mode)
{
    lg(0x4a5c98);
    lg(hashStr(path));
    lg(hashStr(mode));
    return dt_u32() % 6 == 0 ? NULL : (void *)0x9000;
}

static int BMX_CDECL sFclose(void *file)
{
    lg(0x4a5cab);
    lgp(file);
    return 0;
}

static void BMX_CDECL sMovieInit(void *m)
{
    lg(0x49fb30);
    lgp(m);
}

static int BMX_CDECL sMovieOpen(void *m, const char *path)
{
    lg(0x4a06f0);
    lgp(m);
    lg(hashStr(path));
    return dt_u32() % 6 != 0;
}

static void BMX_CDECL sMoviePlay(void *m, void *window, int dd, int loop, const void *rect)
{
    lg(0x4a09d0);
    lgp(m);
    lgp(window);
    lg((uint32_t)dd);
    lg((uint32_t)loop);
    lgp(rect);
}

static void BMX_CDECL sMovieRefill(void *m)
{
    lg(0x4a0a70);
    lgp(m);
}

static void BMX_CDECL sMovieUpdate(void *m, void *hdc, const void *rect)
{
    lg(0x4a0a90);
    lgp(m);
    lgp(hdc);
    lgp(rect);
}

static int sPlayingLeft;

static int BMX_CDECL sMoviePlaying(const void *m)
{
    lg(0x4a1050);
    lgp(m);
    return sPlayingLeft-- > 0;
}

static void BMX_CDECL sMovieShutdown(void *m)
{
    lg(0x49fbe0);
    lgp(m);
}

static int BMX_CDECL sPadPoll(void)
{
    lg(0x4803f0);
    return 0;
}

static uint32_t sPads[2][4];

static void *BMX_CDECL sPadDevice(int index)
{
    lg(0x480400);
    lg((uint32_t)index);
    return index == 0 && sPads[0][3] == 0xdead ? NULL : sPads[index & 1];
}

typedef void(BMX_STDCALL *SleepFn)(uint32_t);
static SleepFn sRealSleep;

/* Sleeps a random while, so that the movie's 40 ms update interval is reached within a few loops. */
static void BMX_STDCALL sSleep(uint32_t ms)
{
    lg(0x6995f8);
    lg(ms);
    sRealSleep(ms + (uint32_t)dt_range(0, 60));
}

static int BMX_STDCALL sPeek(void *msg, void *window, uint32_t min, uint32_t max, uint32_t remove)
{
    uint32_t *m = msg;

    lg(0x6997b4);
    lgp(window);
    lg(min);
    lg(max);
    lg(remove);
    if (dt_u32() % 3 != 0) {
        return 0;
    }
    memset(m, 0, 0x1c);
    m[1] = (dt_u32() & 1) ? 0x3bd : 0x113; /* never 0x12: that path ends in _exit */
    m[2] = dt_u32();
    return 1;
}

static int BMX_STDCALL sTranslate(const uint32_t *msg)
{
    lg(0x699768);
    lg(msg[1]);
    lg(msg[2]);
    return 1;
}

static int BMX_STDCALL sDispatch(const uint32_t *msg)
{
    lg(0x699758);
    lg(msg[1]);
    lg(msg[2]);
    return 1;
}

static void setup_play_movie(DtCtx *c)
{
    dt_seed(c);
    char *name = dt_rand_str(30, 0);

    begin(c, 0);
    dt_stub(0x004a5c98, NULL, (void *)sFopen);
    dt_stub(0x004a5cab, NULL, (void *)sFclose);
    dt_stub(0x0049fb30, NULL, (void *)sMovieInit);
    dt_stub(0x004a06f0, NULL, (void *)sMovieOpen);
    dt_stub(0x004a09d0, NULL, (void *)sMoviePlay);
    dt_stub(0x004a0a70, NULL, (void *)sMovieRefill);
    dt_stub(0x004a0a90, NULL, (void *)sMovieUpdate);
    dt_stub(0x004a1050, NULL, (void *)sMoviePlaying);
    dt_stub(0x0049fbe0, NULL, (void *)sMovieShutdown);
    dt_stub(0x004803f0, NULL, (void *)sPadPoll);
    dt_stub(0x00480400, NULL, (void *)sPadDevice);
    sRealSleep = ORIG_VAR(0x006995f8, SleepFn, gSleepSlot);
    ORIG_VAR(0x006995f8, void *, gSleepSlot2) = (void *)sSleep;
    ORIG_VAR(0x006997b4, void *, gPeekSlot) = (void *)sPeek;
    ORIG_VAR(0x00699768, void *, gTranslateSlot) = (void *)sTranslate;
    ORIG_VAR(0x00699758, void *, gDispatchSlot) = (void *)sDispatch;
    ORIG_VAR(0x0067288c, int, gMovieSkipT) = (dt_u32() % 4 == 0);
    sPlayingLeft = dt_range(1, 12);
    for (int i = 0; i < 2; i++) {
        dt_fill(sPads[i], 16);
        if (dt_u32() & 1) {
            sPads[i][2] = 0;
        }
        if (dt_u32() % 3 == 0) {
            sPads[i][3] = sPads[i][2];
        }
    }
    sPads[0][3] = (dt_u32() % 7 == 0) ? 0xdead : sPads[0][3];
    gZrwWindow = (void *)(uintptr_t)dt_range(0x100, 0x200);
    gZrwFullscreen = (int)(dt_u32() & 1);
    gZrwQuit = (dt_u32() % 8 == 0);
    DT_ARG_PTR(c, 0, name);
}

/* ---- message boxes, dialog and window procedures ------------------------------------------------------- */

static int sMessageAnswer;

static int BMX_STDCALL sMessageBox(void *owner, const char *text, const char *caption, uint32_t flags)
{
    lg(0x6997bc);
    lgp(owner);
    lg(hashStr(text));
    lg(hashStr(caption));
    lg(flags);
    return sMessageAnswer;
}

static void BMX_STDCALL sDebugBreak(void)
{
    lg(0x6995f4);
}

static void BMX_STDCALL sPostQuit(int code)
{
    lg(0x699774);
    lg((uint32_t)code);
}

static int BMX_STDCALL sClipCursor(const void *rect)
{
    lg(0x699790);
    lgp(rect);
    return 1;
}

static intptr_t BMX_STDCALL sDefWindowProc(void *window, uint32_t message, uintptr_t wparam, uintptr_t lparam)
{
    lg(0x69978c);
    lgp(window);
    lg(message);
    lg((uint32_t)wparam);
    lg((uint32_t)lparam);
    return (intptr_t)(message ^ 0x1234);
}

static int BMX_STDCALL sAdjustRect(int32_t *rect, uint32_t style, int menu)
{
    lg(0x699780);
    for (int i = 0; i < 4; i++) {
        lg((uint32_t)rect[i]);
    }
    lg(style);
    lg((uint32_t)menu);
    rect[0] -= 3;
    rect[1] -= 25;
    rect[2] += 3;
    rect[3] += 3;
    return 1;
}

static void *BMX_STDCALL sGetDlgItem(void *dialog, int id)
{
    lg(0x699760);
    lgp(dialog);
    lg((uint32_t)id);
    return (void *)(uintptr_t)(0x8000 + id);
}

static uint32_t sItems;
static uint32_t sCursel;

static uintptr_t BMX_STDCALL sSendMessage(void *window, uint32_t message, uintptr_t wparam, uintptr_t lparam)
{
    lg(0x699764);
    lgp(window);
    lg(message);
    lg((uint32_t)wparam);
    if (message == 0x143) {
        lg(hashStr((const char *)lparam));
        return sItems++;
    }
    lg((uint32_t)lparam);
    if (message == 0x147) {
        return sCursel;
    }
    if (message == 0x150) {
        return (uintptr_t)(wparam & 3);
    }
    return 0;
}

static void *BMX_STDCALL sSetFocus(void *window)
{
    lg(0x69977c);
    lgp(window);
    return NULL;
}

static int BMX_STDCALL sEndDialog(void *dialog, intptr_t result)
{
    lg(0x69975c);
    lgp(dialog);
    lg((uint32_t)result);
    return 1;
}

static int BMX_CDECL sFormat(char *dst, const char *fmt, ...)
{
    va_list ap;
    int n;

    lg(0x67696c);
    va_start(ap, fmt);
    n = vsprintf(dst, fmt, ap);
    va_end(ap);
    return n;
}

typedef struct FakeStringApi {
    uint8_t pad[0xe8];
    int(BMX_CDECL *format)(char *, const char *, ...);
} FakeStringApi;

static FakeStringApi sStringApi;

static void BMX_CDECL sOnActivate(void)
{
    lg(0x493830);
}

static void BMX_CDECL sOnDeactivate(void)
{
    lg(0x493890);
}

static void stubWin32(void)
{
    ORIG_VAR(0x006997bc, void *, gMessageBoxSlot) = (void *)sMessageBox;
    ORIG_VAR(0x006995f4, void *, gDebugBreakSlot) = (void *)sDebugBreak;
    ORIG_VAR(0x00699774, void *, gPostQuitSlot) = (void *)sPostQuit;
    ORIG_VAR(0x00699790, void *, gClipSlot) = (void *)sClipCursor;
    ORIG_VAR(0x0069978c, void *, gDefProcSlot) = (void *)sDefWindowProc;
    ORIG_VAR(0x00699780, void *, gAdjustSlot) = (void *)sAdjustRect;
    ORIG_VAR(0x00699760, void *, gGetItemSlot) = (void *)sGetDlgItem;
    ORIG_VAR(0x00699764, void *, gSendSlot) = (void *)sSendMessage;
    ORIG_VAR(0x0069977c, void *, gFocusSlot) = (void *)sSetFocus;
    ORIG_VAR(0x0069975c, void *, gEndSlot) = (void *)sEndDialog;
    sStringApi.format = sFormat;
    ORIG_VAR(0x0067696c, FakeStringApi *, gStringApiPtr) = &sStringApi;
    dt_stub(0x00493830, NULL, (void *)sOnActivate);
    dt_stub(0x00493890, NULL, (void *)sOnDeactivate);
    sMessageAnswer = dt_u32() % 4 == 0 ? 6 : (dt_u32() & 1 ? 2 : 7);
    sItems = (uint32_t)dt_range(0, 5);
    sCursel = (uint32_t)dt_range(0, 5);
}

static void setup_application_error(DtCtx *c)
{
    dt_seed(c);
    begin(c, 0);
    stubWin32();
    DT_ARG_PTR(c, 0, dt_rand_str(30, 0));
}

static void setup_epic_fail(DtCtx *c)
{
    dt_seed(c);
    begin(c, 0);
    stubWin32();
    DT_ARG_PTR(c, 0, dt_rand_str(20, 0));
    DT_ARG_PTR(c, 1, dt_rand_str(20, 0));
    DT_ARG_INT(c, 2, dt_range(-5, 5000));
    DT_ARG_PTR(c, 3, dt_u32() & 1 ? dt_rand_str(12, 0) : NULL);
}

typedef struct FakeSurface {
    const struct FakeSurfaceVtbl *vtbl;
} FakeSurface;
typedef struct FakeSurfaceVtbl {
    uint8_t pad[0x28];
    uint32_t(BMX_STDCALL *release)(void *);
} FakeSurfaceVtbl;
typedef struct FakeDdVtbl {
    uint8_t pad[0x90];
    int(BMX_STDCALL *getGdiSurface)(void *, void **);
} FakeDdVtbl;
typedef struct FakeDd {
    const FakeDdVtbl *vtbl;
} FakeDd;

static FakeSurface sSurface;
static FakeSurfaceVtbl sSurfaceVtbl;
static FakeDd sDd;
static FakeDdVtbl sDdVtbl;

static uint32_t BMX_STDCALL sSurfaceRelease(void *self)
{
    lg(0x28);
    lgp(self == &sSurface ? (void *)1 : self);
    return 0;
}

static int BMX_STDCALL sGetGdiSurface(void *self, void **surface)
{
    lg(0x90);
    lgp(self == &sDd ? (void *)1 : self);
    lg(surface == &ORIG_VAR(0x006977cc, void *, gSurfaceVar));
    *surface = &sSurface;
    return 0;
}

static void setup_confirm_quit(DtCtx *c)
{
    dt_seed(c);
    begin(c, 0);
    stubWin32();
    sSurfaceVtbl.release = sSurfaceRelease;
    sDdVtbl.getGdiSurface = sGetGdiSurface;
    sSurface.vtbl = &sSurfaceVtbl;
    sDd.vtbl = &sDdVtbl;
    ORIG_VAR(0x0069730c, FakeDd *, gDdPtr) = dt_u32() % 4 == 0 ? NULL : &sDd;
    ORIG_VAR(0x006977cc, void *, gSurfacePtr) = NULL;
    gZrwFullscreen = (int)(dt_u32() & 1);
    gZrwWindow = (void *)(uintptr_t)dt_u32();
}

/* Video modes of the dialog tests: the listed resolutions, odd ones and the 640x480x16 default. */
static void setupDialogModes(void)
{
    static const uint32_t kSizes[][2] = {{640, 480}, {800, 600}, {1024, 768},  {1280, 1024}, {1600, 1200},
                                         {800, 480}, {640, 600}, {1024, 1024}, {320, 240},   {1920, 1080}};
    static const uint32_t kDepths[] = {16, 32, 24, 8, 16, 32};

    sModes = (uint32_t)dt_range(1, 12);
    for (int i = 0; i < 16; i++) {
        const uint32_t pick = dt_u32() % 10;

        sModeData[i][0] = kSizes[pick][0];
        sModeData[i][1] = kSizes[pick][1];
        sModeData[i][2] = kDepths[dt_u32() % 6];
        sModeData[i][3] = dt_u32() & 0x1ff;
    }
    {
        const int at = dt_range(0, (int)sModes - 1);

        sModeData[at][0] = 640;
        sModeData[at][1] = 480;
        sModeData[at][2] = 16;
    }
}

static int sSelectedMode;

static int BMX_CDECL sCurrentVideoMode(void)
{
    lg(0x4cfad0);
    return sSelectedMode;
}

static void setupDialog(void)
{
    stubWin32();
    stubDevice();
    dt_stub(0x004cfad0, NULL, (void *)sCurrentVideoMode);
    sSelectedMode = dt_range(0, 5);
    setupDialogModes();
    ORIG_VAR(0x00672750, int, gSubCountD) = dt_range(1, 8);
    ORIG_VAR(0x00672754, int, gSubD) = dt_range(0, 9);
    ORIG_VAR(0x00672758, uint32_t, gModeD) = dt_u32();
    for (int i = 0; i < 8; i++) {
        char *name = (char *)0x6724d0 + i * 0x50;
        const int len = dt_range(0, 10);

        for (int k = 0; k < len; k++) {
            name[k] = (char)dt_range('A', 'Z');
        }
        name[len] = '\0';
        if (len >= 3 && dt_u32() % 3 == 0) {
            memcpy(name + len - 3, "S/W", 3);
        }
    }
    /* The original leaves its selection undefined (stack garbage) when the current sub system is not in
       the list or no 640x480x16 mode exists, so the fixtures always provide both. */
    ORIG_VAR(0x00672754, int, gSubE) = dt_range(0, ORIG_VAR(0x00672750, int, gSubCountE) - 1);
}

static void setup_dialog_init(DtCtx *c)
{
    dt_seed(c);
    begin(c, 0);
    setupDialog();
    DT_ARG_PTR(c, 0, (void *)0x9000);
}

static void setup_dialog_select(DtCtx *c)
{
    dt_seed(c);
    begin(c, 0);
    setupDialog();
    sCursel = (uint32_t)dt_range(0, 3);
    ORIG_VAR(0x00672754, int, gSubJ) = dt_range(0, 3);
    DT_ARG_PTR(c, 0, (void *)0x9100);
}

static void setup_dialog_proc(DtCtx *c)
{
    dt_seed(c);
    static const uint32_t kIds[] = {1, 0x3e8, 0x3e9, 0x3ea, 0, 2, 0x3e7, 0x3eb, 0xffff};
    static const uint32_t kMessages[] = {0x10, 0x110, 0x111, 0x111, 0x111, 0x112, 0x0};
    const uint32_t message = kMessages[dt_u32() % 7];

    begin(c, 0);
    setupDialog();
    DT_ARG_PTR(c, 0, (void *)0x9200);
    DT_ARG_INT(c, 1, message);
    DT_ARG_INT(c, 2, kIds[dt_u32() % 9] | ((uint32_t)dt_range(0, 2) << 16) | (dt_u32() & 1 ? 0 : 0x70000));
    DT_ARG_INT(c, 3, dt_u32());
    sCursel = (uint32_t)dt_range(0, 3);
}

static void setup_main_wnd_proc(DtCtx *c)
{
    dt_seed(c);
    static const uint32_t kMessages[] = {1, 2, 3, 4, 5, 6, 6, 0x24, 0x24, 0x100, 0x100, 0x100, 0x200, 0x0};
    const uint32_t message = kMessages[dt_u32() % 14];
    uint32_t lparam;

    begin(c, 0);
    stubWin32();
    gZrwWantW = dt_range(1, 2000);
    gZrwWantH = dt_range(1, 1500);
    gZrwFullscreen = (int)(dt_u32() & 1);
    gZrwKeysLocked = (int)(dt_u32() % 3 == 0);
    gZrwQuit = 0;
    {
        /* the key table is constant data: pick a scan code that maps to the quit action, or one that does not
         */
        const uint32_t *table = (const uint32_t *)0x0054dce8;
        const int wantQuit = (int)(dt_u32() & 1);
        uint32_t key = dt_u32() & 0xff;

        while ((table[key] == 0x80) != wantQuit) {
            key = (key + 1) & 0xff;
        }
        lparam = (key & 0x7f) << 16 | (key & 0x80 ? 0x1000000u : 0) | (dt_u32() & 0xe000ffffu);
    }
    if (message == 0x24) {
        void *info = dt_alloc(0x28);

        dt_fill(info, 0x28);
        lparam = (uint32_t)(uintptr_t)info;
    }
    DT_ARG_PTR(c, 0, (void *)0x9300);
    DT_ARG_INT(c, 1, message);
    DT_ARG_INT(c, 2, dt_u32() % 3 == 0 ? 1 : dt_u32());
    DT_ARG_INT(c, 3, lparam);
}

DT_CASES(zrwzglCases) = {
    {"zrwListInit", 0x0048c110, zrwListInit, 1, DT_RET_VOID, setup_list_init, 50, 0},
    {"zrwListPrepend", 0x0048c120, zrwListPrepend, 2, DT_RET_VOID, setup_list_add, 300, 0},
    {"zrwListAdd", 0x0048c150, zrwListAdd, 2, DT_RET_VOID, setup_list_add, 300, 0},
    {"zrwListRemove", 0x0048c1f0, zrwListRemove, 2, DT_RET_VOID, setup_list_remove, 300, 0},
    {"zrwPrintMaterialUsage", 0x00489c90, zrwPrintMaterialUsage, 0, DT_RET_VOID, setup_material_usage, 50, 0},
    {"zrwTimeGetTime", 0x00489cb0, zrwTimeGetTime, 0, DT_RET_INT, setup_none, 20, 0},
    {"zrwDebugOutput", 0x00489ce0, zrwDebugOutput, 1, DT_RET_VOID, setup_debug_output, 30, 0},
    {"zrwFileSystemsInitialize", 0x0048a940, zrwFileSystemsInitialize, 1, DT_RET_INT, setup_filesystems, 100,
     0},
    {"zrwReturnZero", 0x0048a9a0, zrwReturnZero, 0, DT_RET_INT, setup_none, 5, 0},
    {"zrwNop", 0x0048af80, zrwNop, 0, DT_RET_VOID, setup_none, 5, 0},
    {"zrwSetParameter", 0x0048ab10, zrwSetParameter, 3, DT_RET_INT, setup_set_parameter, 800, 0},
    {"zrwGetParameter", 0x0048ad20, zrwGetParameter, 3, DT_RET_INT, setup_get_parameter, 800, 0},
    {"zrwGraphicLibraryInitialize", 0x0048afe0, zrwGraphicLibraryInitialize, 0, DT_RET_INT,
     setup_graphic_init, 300, 0},
    {"zrwGraphicLibraryShutdown", 0x0048b110, zrwGraphicLibraryShutdown, 0, DT_RET_INT, setup_graphic_exit,
     300, 0},
    {"zrwAttachPlugins", 0x0048b1a0, zrwAttachPlugins, 1, DT_RET_INT, setup_attach, 500, 0},
    {"zrwInitApp", 0x0048b450, zrwInitApp, 1, DT_RET_INT, setup_init_app, 500, 0},
    {"zrwRunCallbacks", 0x0048b9e0, zrwRunCallbacks, 1, DT_RET_INT, setup_run_callbacks, 300, 0},
    {"zrwOnIdle", 0x0048b570, zrwOnIdle, 0, DT_RET_INT, setup_on_idle, 400, 0},
    {"zrwDrawFrame", 0x0048b690, zrwDrawFrame, 0, DT_RET_INT, setup_draw_frame, 800, 0},
    {"zrwShowRaster", 0x0048b950, zrwShowRaster, 1, DT_RET_VOID, setup_show_raster, 50, 0},
    {"zrwAddCallback", 0x0048b990, zrwAddCallback, 4, DT_RET_INT, setup_add_callback, 300, 0},
    {"zrwRegisterUpdateFunction", 0x0048ba30, zrwRegisterUpdateFunction, 4, DT_RET_INT, setup_register, 500,
     0},
    {"zrwUnregisterUpdateFunction", 0x0048bad0, zrwUnregisterUpdateFunction, 1, DT_RET_INT, setup_unregister,
     300, 0},
    {"zrwRunUpdateList", 0x0048bb20, zrwRunUpdateList, 1, DT_RET_VOID, setup_run_update_list, 300, 0},
    {"zrwResetRenderState", 0x0048bb70, zrwResetRenderState, 0, DT_RET_VOID, setup_reset_state, 50, 0},
    {"zrwSetRenderState", 0x0048bc60, zrwSetRenderState, 2, DT_RET_INT, setup_set_state, 1000, 0},
    {"zrwPushRenderState", 0x0048bd80, zrwPushRenderState, 0, DT_RET_INT, setup_push_state, 100, 0},
    {"zrwPopRenderState", 0x0048bdd0, zrwPopRenderState, 0, DT_RET_INT, setup_pop_state, 300, 0},
    {"zrwConvertMatrix", 0x0048be30, zrwConvertMatrix, 2, DT_RET_VOID, setup_convert_matrix, 300, 0},
    {"zrwPlayMovie", 0x0048bed0, zrwPlayMovie, 1, DT_RET_INT, setup_play_movie, 500, 0},
    {"zrwSelectDevice", 0x00489d70, zrwSelectDevice, 0, DT_RET_INT, setup_select_device, 800, 0},
    {"zrwSelectDefaultDevice", 0x00489ef0, zrwSelectDefaultDevice, 0, DT_RET_INT, setup_select_default, 300,
     0},
    {"zrwApplicationError", 0x00489cc0, zrwApplicationError, 1, DT_RET_VOID, setup_application_error, 30, 0},
    {"zrwEpicFail", 0x00489cf0, zrwEpicFail, 4, DT_RET_VOID, setup_epic_fail, 200, 0},
    {"zrwConfirmQuit", 0x0048a9b0, zrwConfirmQuit, 0, DT_RET_INT, setup_confirm_quit, 100, 0},
    {"zrwDialogInit", 0x00489f70, zrwDialogInit, 1, DT_RET_VOID, setup_dialog_init, 500, 0},
    {"zrwDialogSelectDevice", 0x0048a1a0, zrwDialogSelectDevice, 1, DT_RET_VOID, setup_dialog_select, 500, 0},
    {"zrwDialogProc", 0x0048a330, zrwDialogProc, 4, DT_RET_INT, setup_dialog_proc, 800, 0},
    {"zrwMainWndProc", 0x0048a800, zrwMainWndProc, 4, DT_RET_INT, setup_main_wnd_proc, 800, 0},
};
