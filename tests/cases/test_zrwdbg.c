/*
 * Tests for zrwDbg.c (renderer debug output). The renderer's draw hooks, the charset functions and the
 * metrics query are stubbed; they log what they are given (matrices are logged by value, because the
 * original builds them on the stack). The static vertex data of the shapes lives in .bss and is compared as
 * memory. Assert paths end the harness run, which compares the abort.
 */
#include <stdarg.h>
#include <stddef.h>

#include "dt.h"
#include "dt_ngif.h"
#include "dt_zmem.h"
#include "zge/com/zglrw/zrwdbg.h"
#include "zge/zgemods/zobjcamera.h"

#define gTextEndT    ORIG_VAR(0x00650a94, uint8_t *, gTextEndT)
#define gTextQueueT  ORIG_VAR(0x00650694, uint8_t, gTextQueueT)
#define gIndentT     ORIG_VAR(0x0065e29c, int, gIndentT)
#define gSphereVertT ORIG_VAR(0x0065e2a0, void *, gSphereVertT)
#define gSpherePtrT  ORIG_VAR(0x0065e2a4, void **, gSpherePtrT)
#define gLineFreshT  ORIG_VAR(0x00560080, int, gLineFreshT)
#define gAxesFreshT  ORIG_VAR(0x00560084, int, gAxesFreshT)
#define gTriFreshT   ORIG_VAR(0x00560088, int, gTriFreshT)
#define gBoxFreshT   ORIG_VAR(0x0056008c, int, gBoxFreshT)
#define gFrameStart  ORIG_VAR(0x0056148c, void *, gFrameStart)
#define gGetCamera   ORIG_VAR(0x00561db0, void *, gGetCamera)
#define gCameraObj   ORIG_VAR(0x00561db4, void *, gCameraObj)
#define gRegisterT   ORIG_VAR(0x00561470, void *, gRegisterT)
#define gUnregisterT ORIG_VAR(0x00561474, void *, gUnregisterT)

/* ---- stubs ---------------------------------------------------------------------------------------- */

static int32_t gMetrics[8];
static int gMetricsAnswer;
static void *gCharsetAnswer;
static ZObjCamera *gCameraAnswer;

static void BMX_STDCALL stubOutput(const char *text)
{
    ngLog(0x48a6e0);
    ngLog(ngHash(text));
}

static void BMX_CDECL stubPrintCharset(void *charset, const char *text, int x, int y)
{
    ngLog(0x4cbef0);
    ngLogPtr(charset);
    ngLog(ngHash(text));
    ngLog((uint32_t)x);
    ngLog((uint32_t)y);
}

static const int32_t *BMX_CDECL stubMetrics(void)
{
    ngLog(0x4d0120);
    return gMetricsAnswer ? gMetrics : NULL;
}

static void *BMX_CDECL stubCharsetCreate(const uint32_t *foreground, const uint32_t *background)
{
    ngLog(0x4cbfb0);
    ngLogPtr(foreground);
    ngLogPtr(background);
    return gCharsetAnswer;
}

static void BMX_CDECL stubCharsetDesc(void *charset, int32_t *desc)
{
    ngLog(0x4cb3f0);
    ngLogPtr(charset);
    /* All 5 words of RtCharsetDesc, like the real function: a too-small caller buffer must break here too. */
    desc[0] = dt_range(1, 20);
    desc[1] = dt_range(1, 30);
    desc[2] = dt_range(1, 20);
    desc[3] = dt_range(1, 30);
    desc[4] = dt_range(1, 256);
}

static void BMX_CDECL stubCharsetDestroy(void *charset)
{
    ngLog(0x4cbf60);
    ngLogPtr(charset);
}

static void logMatrix(const ZMatrix *m)
{
    if (m == NULL) {
        ngLog(0);
        return;
    }
    ngLogVec(&m->right);
    ngLogVec(&m->up);
    ngLogVec(&m->at);
    ngLogVec(&m->pos);
}

static void BMX_CDECL stubDrawPolys(uint32_t flags, const void *prims, int count, const ZMatrix *matrix,
                                    int zero)
{
    ngLog(0x561728);
    ngLog(flags);
    ngLogPtr(prims);
    ngLog((uint32_t)count);
    logMatrix(matrix);
    ngLog((uint32_t)zero);
}

static void BMX_CDECL stubDrawTriangle(uint32_t flags, const void *prims, int count, const ZMatrix *matrix,
                                       int zero)
{
    ngLog(0x56172c);
    ngLog(flags);
    ngLogPtr(prims);
    ngLog((uint32_t)count);
    logMatrix(matrix);
    ngLog((uint32_t)zero);
}

static void BMX_CDECL stubDrawSphere(uint32_t flags, const void *prims, int count, const ZMatrix *matrix,
                                     int zero)
{
    const uint32_t *poly = prims;

    ngLog(0x561730);
    ngLog(flags);
    ngLog(poly[0]);
    ngLogPtr((const void *)(uintptr_t)poly[1]);
    ngLog(poly[2]);
    ngLog((uint32_t)count);
    logMatrix(matrix);
    ngLog((uint32_t)zero);
}

static void BMX_CDECL stubFrameStart(void)
{
    ngLog(0x56148c);
}

static void *BMX_CDECL stubGetCamera(void)
{
    ngLog(0x561db0);
    return (void *)(uintptr_t)0x7777;
}

static ZObjCamera *BMX_CDECL stubCameraObject(void *camera)
{
    ngLog(0x561db4);
    ngLogPtr(camera);
    return gCameraAnswer;
}

static void *BMX_CDECL stubRegister(int type, void *fn, void *arg, int append)
{
    ngLog(0x561470);
    ngLog((uint32_t)type);
    ngLogPtr(fn);
    ngLogPtr(arg);
    ngLog((uint32_t)append);
    return (void *)(uintptr_t)0x6100;
}

static void BMX_CDECL stubUnregister(void *node)
{
    ngLog(0x561474);
    ngLogPtr(node);
}

static void *BMX_CDECL stubAddCallback(void *list, void *fn, void *arg, int append)
{
    ngLog(0x48b990);
    ngLogPtr(list);
    ngLogPtr(fn);
    ngLogPtr(arg);
    ngLog((uint32_t)append);
    return (void *)(uintptr_t)0x6200;
}

static int BMX_CDECL stubUnregisterNode(void *node)
{
    ngLog(0x48bad0);
    ngLogPtr(node);
    return 1;
}

static int BMX_CDECL stubRunCallbacks(void *list)
{
    ngLog(0x48b9e0);
    ngLogPtr(list);
    return 1;
}

/* ---- fixtures ------------------------------------------------------------------------------------- */

static void begin(const DtCtx *c)
{
    ngBegin(c, 0);
    dt_zmem_env();
    gOutputDebugString = stubOutput;
    gTextEndT = &gTextQueueT;
    gIndentT = 0;
    gZrwDbgCharWidth = dt_range(1, 16);
    gZrwDbgCharHeight = dt_range(1, 24);
    gZrwDbgCharset = dt_u32() % 5 == 0 ? NULL : (void *)(uintptr_t)0x7000;
    gZrwDbgFpsMode = (uint8_t)dt_range(0, 3);
    gZrwFps = dt_range(0, 99);
    gZrwDebugLightOn = (int)(dt_u32() & 1);
    gZrwLightParamA = dt_u32() % 200;
    gZrwLightParamB = dt_u32() % 100;
    gMetricsAnswer = (int)(dt_u32() & 1);
    for (int i = 0; i < 8; i++) {
        gMetrics[i] = dt_range(0, 99999);
    }
    gZrwDrawPolys = stubDrawPolys;
    gZrwDrawTriangle = stubDrawTriangle;
    gZrwDrawSphere = stubDrawSphere;
    gFrameStart = (void *)stubFrameStart;
    gGetCamera = (void *)stubGetCamera;
    gCameraObj = (void *)stubCameraObject;
    gRegisterT = (void *)stubRegister;
    gUnregisterT = (void *)stubUnregister;
    gLineFreshT = (int)(dt_u32() % 3 != 0);
    gAxesFreshT = (int)(dt_u32() % 3 != 0);
    gTriFreshT = (int)(dt_u32() % 3 != 0);
    gBoxFreshT = (int)(dt_u32() % 3 != 0);
    dt_stub(0x004d0120, NULL, (void *)stubMetrics);
    dt_stub(0x004cbef0, NULL, (void *)stubPrintCharset);
    dt_stub(0x004cbfb0, NULL, (void *)stubCharsetCreate);
    dt_stub(0x004cb3f0, NULL, (void *)stubCharsetDesc);
    dt_stub(0x004cbf60, NULL, (void *)stubCharsetDestroy);
    dt_stub(0x0048b9e0, NULL, (void *)stubRunCallbacks);
}

static float rf(void)
{
    return dt_float_edgy(-100.0f, 100.0f);
}

static void setf(void *p, uint32_t off, float v)
{
    *(float *)((uint8_t *)p + off) = v;
}

static ZMatrix *make_matrix(void)
{
    ZMatrix *m = dt_alloc(sizeof *m);
    float *f = (float *)m;

    for (int i = 0; i < 16; i++) {
        f[i] = rf();
    }
    m->flags = dt_u32();
    return m;
}

/* A shape record (0x68 bytes of data after the type word): the fields the drawing reads are random. */
static uint8_t *make_data(void)
{
    uint8_t *d = dt_alloc(0x80);

    dt_fill(d, 0x80);
    for (uint32_t off = 0; off < 0x60; off += 4) {
        setf(d, off, rf());
    }
    return d;
}

static ZObjCamera *make_camera(void)
{
    ZObjCamera *camera = dt_alloc(sizeof *camera);
    ZCameraCache *cache = dt_alloc(sizeof *cache);

    camera->pos.x = rf();
    camera->pos.y = rf();
    camera->pos.z = rf();
    camera->rot.x = dt_float_edgy(-1.0f, 1.0f);
    camera->rot.y = dt_float_edgy(-1.0f, 1.0f);
    camera->rot.z = dt_float_edgy(-1.0f, 1.0f);
    camera->rot.w = dt_float_edgy(-1.0f, 1.0f);
    for (int i = 0; i < 16; i++) {
        ((float *)cache->matrix)[i] = rf();
    }
    cache->valid = dt_u32() & 1;
    camera->cache = cache;
    return camera;
}

static void shape_env(const DtCtx *c)
{
    begin(c);
    gCameraAnswer = make_camera();
    if (dt_u32() % 2 == 0) {
        gSphereVertT = dt_zmemalloc(0xfc0);
        gSpherePtrT = (void **)dt_zmemalloc(0x240);
    }
}

typedef int(BMX_CDECL *PrintFn)(int, int, const char *, ...);
#define O_PRINT DT_ORIG(0x0047f940, PrintFn)

/* ---- text ----------------------------------------------------------------------------------------- */

static const char *const kFormats[] = {"%d,%d", "val=%s", "% 10.4f", "plain text"};

static void setup_debug_string(DtCtx *c)
{
    begin(c);
    gIndentT = dt_range(0, 12);
    DT_ARG_PTR(c, 0, kFormats[dt_u32() % 2 == 0 ? 0 : 3]);
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_INT(c, 2, dt_u32());
    if (c->args[0] == (uint32_t)(uintptr_t)kFormats[3] && dt_u32() % 2 == 0) {
        DT_ARG_PTR(c, 0, "%s and %d");
        DT_ARG_PTR(c, 1, dt_rand_str(30, 0));
    }
}

static void setup_print(DtCtx *c)
{
    const uint32_t f = dt_u32() % 4;
    double value = rf();
    uint32_t words[2];

    begin(c);
    if (dt_u32() % 6 == 0) {
        gTextEndT = &gTextQueueT + 0x400 - dt_range(0, 40); /* nearly full */
    }
    DT_ARG_INT(c, 0, dt_u32() % 3 == 0 ? (uint32_t)dt_range(-5, 400) : dt_u32());
    DT_ARG_INT(c, 1, dt_u32() % 3 == 0 ? (uint32_t)dt_range(-5, 400) : dt_u32());
    DT_ARG_PTR(c, 2, kFormats[f]);
    memcpy(words, &value, 8);
    switch (f) {
    case 0:
        DT_ARG_INT(c, 3, dt_u32());
        DT_ARG_INT(c, 4, dt_u32());
        break;
    case 1:
        DT_ARG_PTR(c, 3, dt_rand_str(60, 0));
        break;
    case 2:
        DT_ARG_INT(c, 3, words[0]);
        DT_ARG_INT(c, 4, words[1]);
        break;
    default:
        break;
    }
}

static void setup_print_direct(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, dt_rand_str(20, 0));
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_INT(c, 2, dt_u32());
}

static void setup_print_float(DtCtx *c)
{
    begin(c);
    DT_ARG_INT(c, 0, dt_u32());
    DT_ARG_INT(c, 1, dt_u32() % 2 ? (uint32_t)-1 : dt_u32());
    DT_ARG_PTR(c, 2, dt_u32() % 3 == 0 ? NULL : "lbl:");
    DT_ARG_FLOAT(c, 3, dt_float_edgy(-1000.0f, 1000.0f));
}

static float *make_floats(int n)
{
    float *v = dt_alloc((uint32_t)n * 4 + 16);

    for (int i = 0; i < n; i++) {
        v[i] = dt_float_edgy(-1000.0f, 1000.0f);
    }
    return v;
}

static void setup_print_vec(DtCtx *c, int n)
{
    begin(c);
    DT_ARG_INT(c, 0, dt_range(-4, 300));
    DT_ARG_INT(c, 1, dt_u32() % 2 ? (uint32_t)-1 : (uint32_t)dt_range(0, 300));
    DT_ARG_PTR(c, 2, dt_u32() % 3 == 0 ? NULL : "lbl:");
    DT_ARG_PTR(c, 3, make_floats(n));
}

static void setup_print_matrix(DtCtx *c)
{
    setup_print_vec(c, 16);
}

static void setup_print_vec4(DtCtx *c)
{
    setup_print_vec(c, 4);
}

static void setup_print_vec3(DtCtx *c)
{
    setup_print_vec(c, 3);
}

static void setup_print_angle(DtCtx *c)
{
    begin(c);
    DT_ARG_INT(c, 0, dt_range(-4, 300));
    DT_ARG_INT(c, 1, dt_u32() % 2 ? (uint32_t)-1 : (uint32_t)dt_range(0, 300));
    DT_ARG_PTR(c, 2, dt_u32() % 3 == 0 ? NULL : "ang:");
    DT_ARG_INT(c, 3, dt_u32());
}

static void setup_print_angles(DtCtx *c)
{
    int16_t *a = dt_alloc(8);

    setup_print_angle(c);
    for (int i = 0; i < 3; i++) {
        a[i] = (int16_t)dt_u32();
    }
    DT_ARG_PTR(c, 3, a);
}

static void setup_fps_mode(DtCtx *c)
{
    begin(c);
    DT_ARG_INT(c, 0, dt_u32());
}

/* ---- queue ---------------------------------------------------------------------------------------- */

static void queue_env(const DtCtx *c)
{
    static const int counts[] = {0, 1, 2, 5, 0x1ff, 0x200};

    begin(c);
    gZrwDbgItemCount = counts[dt_u32() % 6];
}

static uint32_t *make_color(void)
{
    uint32_t *color = dt_alloc(4);

    *color = dt_u32();
    return color;
}

static ZVec3 *make_vec(void)
{
    ZVec3 *v = dt_alloc(sizeof *v);

    v->x = rf();
    v->y = rf();
    v->z = rf();
    return v;
}

static void setup_add_line(DtCtx *c)
{
    queue_env(c);
    DT_ARG_PTR(c, 0, make_vec());
    DT_ARG_PTR(c, 1, make_vec());
    DT_ARG_PTR(c, 2, make_color());
    DT_ARG_INT(c, 3, dt_u32());
}

static void setup_add_axes(DtCtx *c)
{
    queue_env(c);
    DT_ARG_PTR(c, 0, dt_u32() % 4 == 0 ? NULL : make_matrix());
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_INT(c, 2, dt_u32());
}

static void setup_add_triangle(DtCtx *c)
{
    queue_env(c);
    DT_ARG_PTR(c, 0, make_vec());
    DT_ARG_PTR(c, 1, make_vec());
    DT_ARG_PTR(c, 2, make_vec());
    DT_ARG_PTR(c, 3, make_color());
    DT_ARG_INT(c, 4, dt_u32());
}

static void setup_add_sphere(DtCtx *c)
{
    queue_env(c);
    DT_ARG_PTR(c, 0, make_vec());
    DT_ARG_FLOAT(c, 1, dt_float_edgy(0.0f, 50.0f));
    DT_ARG_PTR(c, 2, make_color());
    DT_ARG_INT(c, 3, dt_u32());
}

static void setup_add_box(DtCtx *c)
{
    queue_env(c);
    DT_ARG_PTR(c, 0, make_floats(8));
    DT_ARG_PTR(c, 1, dt_u32() % 3 == 0 ? NULL : make_matrix());
    DT_ARG_PTR(c, 2, make_color());
    DT_ARG_INT(c, 3, dt_u32());
}

/* ---- shapes --------------------------------------------------------------------------------------- */

static void setup_draw_line(DtCtx *c)
{
    shape_env(c);
    DT_ARG_PTR(c, 0, make_data());
}

static void setup_draw_axes(DtCtx *c)
{
    uint8_t *d = make_data();

    shape_env(c);
    d[0x40] = (uint8_t)(dt_u32() & 1);
    DT_ARG_PTR(c, 0, d);
}

static void setup_draw_radius(DtCtx *c)
{
    uint8_t *d = make_data();

    shape_env(c);
    setf(d, 0x10, dt_float_edgy(0.0f, 20.0f));
    DT_ARG_PTR(c, 0, d);
}

/* ---- the update callback and life cycle ----------------------------------------------------------- */

static void fill_text_queue(void)
{
    for (int i = dt_range(0, 6); i > 0; i--) {
        const int y = dt_u32() % 2 ? -1 - (int)(dt_u32() % 3) : dt_range(0, 300);

        O_PRINT(dt_range(-3, 200), y, "%s", dt_rand_str(14, 0));
    }
}

static void fill_items(void)
{
    typedef void(BMX_CDECL * LineFn)(const ZVec3 *, const ZVec3 *, const uint32_t *, uint32_t);
    typedef void(BMX_CDECL * AxesFn)(const ZMatrix *, uint32_t, uint32_t);
    typedef void(BMX_CDECL * TriFn)(const ZVec3 *, const ZVec3 *, const ZVec3 *, const uint32_t *, uint32_t);
    typedef void(BMX_CDECL * SphereFn)(const ZVec3 *, float, const uint32_t *, uint32_t);
    typedef void(BMX_CDECL * BoxFn)(const float *, const ZMatrix *, const uint32_t *, uint32_t);

    for (int i = dt_range(0, 6); i > 0; i--) {
        switch (dt_u32() % 5) {
        case 0:
            DT_ORIG(0x0047ff90, LineFn)(make_vec(), make_vec(), make_color(), dt_u32());
            break;
        case 1:
            DT_ORIG(0x00480010, AxesFn)(dt_u32() % 4 == 0 ? NULL : make_matrix(), dt_u32(), dt_u32());
            break;
        case 2:
            DT_ORIG(0x00480070, TriFn)(make_vec(), make_vec(), make_vec(), make_color(), dt_u32());
            break;
        case 3:
            DT_ORIG(0x00480100, SphereFn)(make_vec(), dt_float_edgy(0.0f, 20.0f), make_color(), dt_u32());
            break;
        default:
            DT_ORIG(0x00480170, BoxFn)(make_floats(8), dt_u32() % 3 == 0 ? NULL : make_matrix(), make_color(),
                                       dt_u32());
        }
    }
}

static void setup_update(DtCtx *c)
{
    shape_env(c);
    gZrwDbgItemCount = 0;
    fill_text_queue();
    fill_items();
    DT_ARG_INT(c, 0, 0);
    if (dt_u32() % 40 == 0) {
        /* an item of an unknown type */
        ((uint32_t *)(uintptr_t)0x650a9c)[0] = 9;
        gZrwDbgItemCount = 1;
    }
}

static void setup_init(DtCtx *c)
{
    begin(c);
    gCharsetAnswer = dt_u32() % 5 == 0 ? NULL : (void *)(uintptr_t)0x7000;
    gZrwDbgCharset = NULL;
    gZrwDbgInitialized = 0;
}

static void setup_shutdown(DtCtx *c)
{
    begin(c);
    gZrwDbgUpdateNode = dt_u32() & 1 ? (ZrwUpdateNode *)(uintptr_t)0x6100 : NULL;
    gZrwDbgInitialized = 1;
}

static void setup_add_callback(DtCtx *c)
{
    begin(c);
    gZrwDbgInitialized = dt_u32() % 5 != 0;
    dt_stub(0x0048b990, NULL, (void *)stubAddCallback);
    DT_ARG_INT(c, 0, dt_u32());
    DT_ARG_PTR(c, 1, (void *)0x5e00);
    DT_ARG_PTR(c, 2, (void *)0x5e04);
}

static void setup_remove_callback(DtCtx *c)
{
    begin(c);
    dt_stub(0x0048bad0, NULL, (void *)stubUnregisterNode);
    DT_ARG_PTR(c, 0, (void *)(uintptr_t)(0x6000 + dt_u32() % 8));
}

DT_CASES(zrwdbg_cases) = {
    {"DebugString", 0x00480240, DebugString, 3, DT_RET_VOID, setup_debug_string, 300, 0},
    {"ZDebugPrintBlya", 0x0047f940, ZDebugPrintBlya, 5, DT_RET_INT, setup_print, 800, 0},
    {"zrwDbgPrintDirect", 0x0047f640, zrwDbgPrintDirect, 3, DT_RET_VOID, setup_print_direct, 50, 0},
    {"zrwDbgPrintFloat", 0x0047f9e0, zrwDbgPrintFloat, 4, DT_RET_VOID, setup_print_float, 300, 0},
    {"zrwDbgPrintMatrix", 0x0047fa30, zrwDbgPrintMatrix, 4, DT_RET_VOID, setup_print_matrix, 300, 0},
    {"zrwDbgPrintVec4", 0x0047fb40, zrwDbgPrintVec4, 4, DT_RET_VOID, setup_print_vec4, 300, 0},
    {"zrwDbgPrintVec3", 0x0047fbd0, zrwDbgPrintVec3, 4, DT_RET_VOID, setup_print_vec3, 300, 0},
    {"zrwDbgPrintAngle", 0x0047fc40, zrwDbgPrintAngle, 4, DT_RET_VOID, setup_print_angle, 300, 0},
    {"zrwDbgPrintAngles", 0x0047fcb0, zrwDbgPrintAngles, 4, DT_RET_VOID, setup_print_angles, 300, 0},
    {"zrwDbgSetFpsMode", 0x0047ff80, zrwDbgSetFpsMode, 1, DT_RET_VOID, setup_fps_mode, 30, 0},
    {"zrwDbgAddLine", 0x0047ff90, zrwDbgAddLine, 4, DT_RET_VOID, setup_add_line, 200, 0},
    {"zrwDbgAddAxes", 0x00480010, zrwDbgAddAxes, 3, DT_RET_VOID, setup_add_axes, 200, 0},
    {"zrwDbgAddTriangle", 0x00480070, zrwDbgAddTriangle, 5, DT_RET_VOID, setup_add_triangle, 200, 0},
    {"zrwDbgAddSphere", 0x00480100, zrwDbgAddSphere, 4, DT_RET_VOID, setup_add_sphere, 200, 0},
    {"zrwDbgAddBox", 0x00480170, zrwDbgAddBox, 4, DT_RET_VOID, setup_add_box, 200, 0},
    {"zrwDbgDrawLine", 0x0047ee80, zrwDbgDrawLine, 1, DT_RET_VOID, setup_draw_line, 200, 0},
    {"zrwDbgDrawAxes", 0x0047ef40, zrwDbgDrawAxes, 1, DT_RET_VOID, setup_draw_axes, 300, 0},
    {"zrwDbgDrawTriangle", 0x0047f230, zrwDbgDrawTriangle, 1, DT_RET_VOID, setup_draw_line, 200, 0},
    {"zrwDbgDrawSphere", 0x0047f2f0, zrwDbgDrawSphere, 1, DT_RET_VOID, setup_draw_radius, 100, 0},
    {"zrwDbgDrawBox", 0x0047f530, zrwDbgDrawBox, 1, DT_RET_VOID, setup_draw_line, 200, 0},
    {"DebugUpdateFunction", 0x0047f660, DebugUpdateFunction, 1, DT_RET_INT, setup_update, 400, 0},
    {"zrwDbgInitialize", 0x0047f850, zrwDbgInitialize, 0, DT_RET_INT, setup_init, 100, 0},
    {"zrwDbgShutdown", 0x0047f900, zrwDbgShutdown, 0, DT_RET_INT, setup_shutdown, 50, 0},
    {"zrwDbgAddCallback", 0x004801f0, zrwDbgAddCallback, 3, DT_RET_INT, setup_add_callback, 50, 0},
    {"zrwDbgRemoveCallback", 0x00480230, zrwDbgRemoveCallback, 1, DT_RET_INT, setup_remove_callback, 20, 0},
};
