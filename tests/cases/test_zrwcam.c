/*
 * Tests for the renderer camera (zrwcam.c). The RenderWare library and the scene hooks are replaced by
 * logging stand-ins whose answers (null camera, missing rasters, failing updates) come from the generator.
 */
#include "dt.h"
#include "dt_ngif.h"
#include "zge/com/zglrw/zrwcam.h"

typedef struct ZMemFreeList *(BMX_CDECL *ListCreateFn)(uint32_t, uint32_t, void *);
typedef void *(BMX_CDECL *ListAllocFn)(struct ZMemFreeList *);

#define G32(addr) (*(uint32_t *)(uintptr_t)(addr))
#define GF(addr)  (*(float *)(uintptr_t)(addr))
#define kPool     0x0067401c
#define kCurrent  0x00674020
#define kActiveRw 0x006976f4
#define kInUpdate 0x006976f8
#define kRaster   0x006976c0
#define kZRaster  0x006976c4
#define kWidth    0x00697770
#define kHeight   0x00697774

static void *gRw, *gFrame, *gMatrix, *gRasterObj, *gZRasterObj, *gWindow;
static int gRwNull, gFrameNull, gRasterNull, gZRasterNull, gParentNull, gBeginOk, gPushOk;

static void logF(float f)
{
    ngLogFloat(f);
}

static void *BMX_CDECL stubRwCameraCreate(void)
{
    ngLog(0x4deae0);
    return gRwNull ? NULL : gRw;
}

static void *BMX_CDECL stubFrameCreate(void)
{
    ngLog(0x4d2870);
    return gFrame;
}

static void BMX_CDECL stubSetFrame(void *cam, void *frame)
{
    ngLog(0x4dea60);
    ngLogPtr(cam);
    ngLogPtr(frame);
}

static void *BMX_CDECL stubGetFrame(void *cam)
{
    ngLog(0x4dea80);
    ngLogPtr(cam);
    return gFrameNull ? NULL : gFrame;
}

static void BMX_CDECL stubSetIdentity(void *frame)
{
    ngLog(0x4d3200);
    ngLogPtr(frame);
}

static void *BMX_CDECL stubGetMatrix(void *frame)
{
    ngLog(0x4d2b00);
    ngLogPtr(frame);
    return gMatrix;
}

static void BMX_CDECL stubMatrixUpdate(float *m)
{
    ngLog(0x4d04d0);
    for (int i = 0; i < 16; i++) {
        logF(m[i]);
    }
}

static void BMX_CDECL stubProjection(void *cam, int p)
{
    ngLog(0x4de920);
    ngLogPtr(cam);
    ngLog((uint32_t)p);
}

static void BMX_CDECL stubSubRaster(void *r, void *parent, const int32_t *rect)
{
    ngLog(0x4dcc80);
    ngLogPtr(r);
    ngLogPtr(parent);
    for (int i = 0; i < 4; i++) {
        ngLog((uint32_t)rect[i]);
    }
}

static void *BMX_CDECL stubGetParent(void *r)
{
    ngLog(0x4dcd70);
    ngLogPtr(r);
    return gParentNull ? NULL : (uint8_t *)r + 0x100;
}

static void BMX_CDECL stubSetRaster(void *cam, void *r)
{
    ngLog(0x4de580);
    ngLogPtr(cam);
    ngLogPtr(r);
}

static void BMX_CDECL stubSetZRaster(void *cam, void *r)
{
    ngLog(0x4de5b0);
    ngLogPtr(cam);
    ngLogPtr(r);
}

static void *BMX_CDECL stubGetRaster(void *cam)
{
    ngLog(0x4de5a0);
    return gRasterNull ? NULL : gRasterObj;
}

static void *BMX_CDECL stubGetZRaster(void *cam)
{
    ngLog(0x4de5d0);
    return gZRasterNull ? NULL : gZRasterObj;
}

static void BMX_CDECL stubSetWindow(void *cam, const float *w)
{
    ngLog(0x4de9b0);
    ngLogPtr(cam);
    logF(w[0]);
    logF(w[1]);
}

static const float *BMX_CDECL stubGetWindow(void *cam)
{
    ngLog(0x4dea10);
    return gWindow;
}

static void BMX_CDECL stubViewOffset(void *cam, const float *o)
{
    ngLog(0x4de540);
    ngLogPtr(cam);
    logF(o[0]);
    logF(o[1]);
}

static void BMX_CDECL stubNear(void *cam, uint32_t d)
{
    ngLog(0x4de5e0);
    ngLogPtr(cam);
    ngLog(d);
}

static void BMX_CDECL stubFar(void *cam, uint32_t d)
{
    ngLog(0x4de620);
    ngLogPtr(cam);
    ngLog(d);
}

static void BMX_CDECL stubFog(void *cam, uint32_t d)
{
    ngLog(0x4de660);
    ngLogPtr(cam);
    ngLog(d);
}

static void BMX_CDECL stubFrameDestroy(void *f)
{
    ngLog(0x4d2920);
    ngLogPtr(f);
}

static void BMX_CDECL stubCameraDestroy(void *c)
{
    ngLog(0x4dea90);
    ngLogPtr(c);
}

static int BMX_CDECL stubRasterDestroy(void *r)
{
    ngLog(0x4dcbd0);
    ngLogPtr(r);
    return 1;
}

static void *BMX_CDECL stubRasterCreate(int w, int h, int d, int f)
{
    ngLog(0x4dcb10);
    ngLog((uint32_t)w);
    ngLog((uint32_t)h);
    ngLog((uint32_t)d);
    ngLog((uint32_t)f);
    return dt_alloc(0x40);
}

static int BMX_CDECL stubBeginUpdate(void *cam)
{
    ngLog(0x4de790);
    ngLogPtr(cam);
    return gBeginOk;
}

static void BMX_CDECL stubEndUpdate(void *cam)
{
    ngLog(0x4de7f0);
    ngLogPtr(cam);
}

static int BMX_CDECL stubPush(void *r)
{
    ngLog(0x4dcd80);
    ngLogPtr(r);
    return gPushOk;
}

static void BMX_CDECL stubPop(void)
{
    ngLog(0x4dce20);
}

static int BMX_CDECL stubRenderState(int s, int v)
{
    ngLog(0x4db820);
    ngLog((uint32_t)s);
    ngLog((uint32_t)v);
    return 1;
}

static void BMX_CDECL stubSceneBegin(void *a, void *b)
{
    ngLog(0x4b81f0);
    ngLog((uint32_t)(uintptr_t)a);
    ngLogPtr(b);
}

static void BMX_CDECL stubSceneEnd(void *a)
{
    ngLog(0x5065d0);
    ngLog((uint32_t)(uintptr_t)a);
}

static void *BMX_CDECL stubSceneFind(void *a)
{
    ngLog(0x4b82a0);
    ngLogPtr(a);
    return (void *)(uintptr_t)0x1234;
}

static void BMX_CDECL stubSceneRelease(void *s, void *camera)
{
    ngLog(0x4b8240);
    ngLog((uint32_t)(uintptr_t)s);
    ngLogPtr(camera);
}

static void *randomBlock(uint32_t size)
{
    uint8_t *p = dt_alloc(size);

    dt_fill(p, size);
    return p;
}

static void begin(DtCtx *c)
{
    ngBegin(c, 1);
    gRw = dt_alloc(16);
    gFrame = dt_alloc(16);
    gMatrix = randomBlock(0x40);
    gRasterObj = randomBlock(0x40);
    gZRasterObj = randomBlock(0x40);
    gWindow = dt_alloc(8);
    ((float *)gWindow)[0] = dt_float_edgy(-2.0f, 2.0f);
    ((float *)gWindow)[1] = dt_float_edgy(-2.0f, 2.0f);
    gRwNull = (dt_u32() & 7) == 0;
    gFrameNull = (dt_u32() & 7) == 0;
    gRasterNull = (dt_u32() & 7) == 0;
    gZRasterNull = (dt_u32() & 7) == 0;
    gParentNull = (dt_u32() & 7) == 0;
    gBeginOk = (dt_u32() & 7) != 0;
    gPushOk = (dt_u32() & 7) != 0;
    G32(kPool) = (uint32_t)(uintptr_t)DT_ORIG(0x00485e80, ListCreateFn)(200, 2, NULL);
    G32(kRaster) = (uint32_t)(uintptr_t)dt_alloc(16);
    G32(kZRaster) = (uint32_t)(uintptr_t)dt_alloc(16);
    G32(kWidth) = (dt_u32() & 3) ? 640 : dt_u32();
    G32(kHeight) = (dt_u32() & 3) ? 480 : dt_u32() & 0xffff;
    for (int i = 0; i < 3; i++) {
        GF(0x006976d8 + 4 * i) = dt_float_edgy(-100.0f, 100.0f);
    }
    GF(0x00697720) = dt_float_edgy(0.0f, 2.0f);
    dt_stub(0x004deae0, NULL, (void *)stubRwCameraCreate);
    dt_stub(0x004d2870, NULL, (void *)stubFrameCreate);
    dt_stub(0x004dea60, NULL, (void *)stubSetFrame);
    dt_stub(0x004dea80, NULL, (void *)stubGetFrame);
    dt_stub(0x004d3200, NULL, (void *)stubSetIdentity);
    dt_stub(0x004d2b00, NULL, (void *)stubGetMatrix);
    dt_stub(0x004d04d0, NULL, (void *)stubMatrixUpdate);
    dt_stub(0x004de920, NULL, (void *)stubProjection);
    dt_stub(0x004dcc80, NULL, (void *)stubSubRaster);
    dt_stub(0x004dcd70, NULL, (void *)stubGetParent);
    dt_stub(0x004de580, NULL, (void *)stubSetRaster);
    dt_stub(0x004de5b0, NULL, (void *)stubSetZRaster);
    dt_stub(0x004de5a0, NULL, (void *)stubGetRaster);
    dt_stub(0x004de5d0, NULL, (void *)stubGetZRaster);
    dt_stub(0x004de9b0, NULL, (void *)stubSetWindow);
    dt_stub(0x004dea10, NULL, (void *)stubGetWindow);
    dt_stub(0x004de540, NULL, (void *)stubViewOffset);
    dt_stub(0x004de5e0, NULL, (void *)stubNear);
    dt_stub(0x004de620, NULL, (void *)stubFar);
    dt_stub(0x004de660, NULL, (void *)stubFog);
    dt_stub(0x004d2920, NULL, (void *)stubFrameDestroy);
    dt_stub(0x004dea90, NULL, (void *)stubCameraDestroy);
    dt_stub(0x004dcbd0, NULL, (void *)stubRasterDestroy);
    dt_stub(0x004dcb10, NULL, (void *)stubRasterCreate);
    dt_stub(0x004de790, NULL, (void *)stubBeginUpdate);
    dt_stub(0x004de7f0, NULL, (void *)stubEndUpdate);
    dt_stub(0x004dcd80, NULL, (void *)stubPush);
    dt_stub(0x004dce20, NULL, (void *)stubPop);
    dt_stub(0x004db820, NULL, (void *)stubRenderState);
    dt_stub(0x004b81f0, NULL, (void *)stubSceneBegin);
    dt_stub(0x005065d0, NULL, (void *)stubSceneEnd);
    dt_stub(0x004b82a0, NULL, (void *)stubSceneFind);
    dt_stub(0x004b8240, NULL, (void *)stubSceneRelease);
}

/* A camera record from the pool with noise, floats of the clip / size fields with edge values. */
static ZrwCamera *newCamera(void)
{
    ZrwCamera *cam = DT_ORIG(0x00486050, ListAllocFn)((struct ZMemFreeList *)(uintptr_t)G32(kPool));

    dt_fill(cam, sizeof *cam);
    cam->rwCamera = (dt_u32() & 7) ? gRw : NULL;
    cam->width = (int32_t)dt_range(0, 2000);
    cam->height = (int32_t)dt_range(0, 2000);
    cam->nearClip = dt_float_edgy(0.0f, 10.0f);
    cam->farClip = dt_float_edgy(0.0f, 300.0f);
    cam->offsetX = dt_float_edgy(-10.0f, 10.0f);
    cam->offsetY = dt_float_edgy(-10.0f, 10.0f);
    cam->clipOffset = dt_float_edgy(-5.0f, 5.0f);
    cam->fov = (dt_u32() & 3) ? (uint16_t)dt_u32() : 0;
    return cam;
}

static ZVec3 *randomVec(float range)
{
    ZVec3 *v = dt_alloc(sizeof *v);

    v->x = dt_float_edgy(-range, range);
    v->y = dt_float_edgy(-range, range);
    v->z = dt_float_edgy(-range, range);
    return v;
}

static void setup_create(DtCtx *c)
{
    begin(c);
    DT_ARG_INT(c, 0, (dt_u32() & 1) ? dt_u32() & 3 : dt_u32());
    DT_ARG_PTR(c, 1, dt_alloc(8));
}

static void setup_cam(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, newCamera());
}

static void setup_clip(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, newCamera());
    DT_ARG_FLOAT(c, 1, (dt_u32() & 3) ? dt_float_edgy(-5.0f, 100.0f) : 0.0f);
    DT_ARG_FLOAT(c, 2, (dt_u32() & 3) ? dt_float_edgy(-5.0f, 500.0f) : 0.0f);
}

static void setup_fov(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, newCamera());
    DT_ARG_INT(c, 1, (dt_u32() & 3) ? dt_u32() : dt_u32() & 0xffff0000u);
}

static void setup_view(DtCtx *c)
{
    ZMatrix *m = dt_alloc(sizeof *m);
    float *f = (float *)m;

    begin(c);
    for (int i = 0; i < 16; i++) {
        f[i] = dt_float_edgy(-1.0f, 1.0f);
    }
    DT_ARG_PTR(c, 0, newCamera());
    DT_ARG_PTR(c, 1, m);
    DT_ARG_PTR(c, 2, randomVec(500.0f));
}

static void setup_project(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, newCamera());
    DT_ARG_PTR(c, 1, randomVec(100.0f));
    DT_ARG_PTR(c, 2, dt_alloc(8));
}

static float *randomRect(void)
{
    float *r = dt_alloc(16);

    for (int i = 0; i < 4; i++) {
        r[i] = dt_float_edgy(-100.0f, 1500.0f);
    }
    return r;
}

static void setup_rect(DtCtx *c)
{
    begin(c);
    gRasterNull = 0; /* the original reads the raster's offsets without a check */
    DT_ARG_PTR(c, 0, newCamera());
    DT_ARG_PTR(c, 1, randomRect());
}

static void setup_viewport(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, newCamera());
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_PTR(c, 2, (dt_u32() & 3) ? randomRect() : NULL);
}

static void setup_list(DtCtx *c)
{
    ZrwCamera *last = NULL;

    begin(c);
    for (int i = dt_range(0, 3); i > 0; i--) {
        last = newCamera();
    }
    DT_ARG_PTR(c, 0, last != NULL ? last : newCamera());
}

static void setup_global(DtCtx *c)
{
    begin(c);
    G32(kCurrent) = dt_u32();
    G32(kActiveRw) = (dt_u32() & 3) ? (uint32_t)(uintptr_t)gRw : 0;
    G32(kInUpdate) = dt_u32() & 1;
    G32(0x006976c8) = dt_u32();
    G32(0x0069772c) = dt_u32();
    G32(0x00697730) = dt_u32();
    DT_ARG_PTR(c, 0, newCamera());
    DT_ARG_PTR(c, 1, randomBlock(16));
    DT_ARG_PTR(c, 2, randomBlock(16));
}

static void setup_flag(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, newCamera());
    DT_ARG_INT(c, 1, dt_u32());
}

static void setup_float(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, newCamera());
    DT_ARG_FLOAT(c, 1, dt_float_edgy(-100.0f, 100.0f));
    DT_ARG_FLOAT(c, 2, dt_float_edgy(-100.0f, 100.0f));
}

static void setup_color(DtCtx *c)
{
    begin(c);
    DT_ARG_PTR(c, 0, newCamera());
    DT_ARG_PTR(c, 1, randomBlock(8));
}

DT_CASES(zrwcam_cases2) = {
    {"zrwCameraCreate", 0x004925f0, zrwCameraCreate, 2, DT_RET_INT, setup_create, 600, 0},
    {"zrwCameraDestroy", 0x00492970, zrwCameraDestroy, 1, DT_RET_INT, setup_cam, 100, 0},
    {"zrwCameraSetClip", 0x004929f0, zrwCameraSetClip, 3, DT_RET_VOID, setup_clip, 300, 0},
    {"zrwCameraSetFov", 0x00492a60, zrwCameraSetFov, 2, DT_RET_VOID, setup_fov, 300, 0},
    {"zrwCameraGetFov", 0x00492ab0, zrwCameraGetFov, 1, DT_RET_INT16, setup_cam, 30, 0},
    {"zrwCameraSetView", 0x00492ac0, zrwCameraSetView, 3, DT_RET_VOID, setup_view, 600, 0},
    {"zrwCameraGetViewMatrix", 0x00492c10, zrwCameraGetViewMatrix, 1, DT_RET_INT, setup_cam, 10, 0},
    {"zrwCameraProject", 0x00492c20, zrwCameraProject, 3, DT_RET_INT, setup_project, 600, 0},
    {"zrwCameraSetRect", 0x00492cc0, zrwCameraSetRect, 2, DT_RET_VOID, setup_rect, 300, 0},
    {"zrwCameraSetViewport", 0x00492de0, zrwCameraSetViewport, 3, DT_RET_VOID, setup_viewport, 300, 0},
    {"zrwCameraFirst", 0x00492eb0, zrwCameraFirst, 0, DT_RET_INT, setup_list, 50, 0},
    {"zrwCameraNext", 0x00492ed0, zrwCameraNext, 1, DT_RET_INT, setup_list, 50, 0},
    {"zrwCameraBegin", 0x00492f00, zrwCameraBegin, 1, DT_RET_INT, setup_global, 300, 0},
    {"zrwCameraEnd", 0x00492fe0, zrwCameraEnd, 0, DT_RET_INT, setup_global, 100, 0},
    {"zrwCameraGetRw", 0x00493060, zrwCameraGetRw, 1, DT_RET_INT, setup_cam, 10, 0},
    {"zrwCameraGetCurrent", 0x00493070, zrwCameraGetCurrent, 0, DT_RET_INT, setup_global, 10, 0},
    {"zrwCameraGetWorldMatrix", 0x00493080, zrwCameraGetWorldMatrix, 1, DT_RET_INT, setup_cam, 10, 0},
    {"zrwCameraGetOwner", 0x00493090, zrwCameraGetOwner, 1, DT_RET_INT, setup_cam, 10, 0},
    {"zrwCameraSetViewportWord", 0x004930a0, zrwCameraSetViewportWord, 2, DT_RET_VOID, setup_flag, 20, 0},
    {"zrwCameraSetViewWindowWord", 0x004930b0, zrwCameraSetViewWindowWord, 2, DT_RET_VOID, setup_color, 20,
     0},
    {"zrwCameraSetClearFlag", 0x004930c0, zrwCameraSetClearFlag, 2, DT_RET_VOID, setup_flag, 50, 0},
    {"zrwCameraSetClearColor", 0x004930e0, zrwCameraSetClearColor, 2, DT_RET_VOID, setup_color, 50, 0},
    {"zrwCameraSetClipOffset", 0x00493100, zrwCameraSetClipOffset, 2, DT_RET_VOID, setup_float, 100, 0},
    {"zrwCameraSetViewOffset", 0x00493130, zrwCameraSetViewOffset, 3, DT_RET_VOID, setup_float, 100, 0},
};
