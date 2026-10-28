/*
 * Tests for rw/camera.c and rw/charset.c: the camera object and its settings, the calls through the device
 * table, and the charset raster. The engine instance is the maths fixture (dt_rwmath.h); the raster and image
 * routines the charset calls are stubs that behave like the real ones (they hand out objects, log their
 * arguments and can fail).
 *
 * CameraBuildPerspClipPlanes / CameraBuildParallelClipPlanes (EAX = camera) are called through asm wrappers.
 * CameraUpdateZShiftScale (EAX = camera) and RtCharsetSetColors (EAX, EDX, ECX) take register arguments and
 * are exercised through the setters, RwCameraCreate and RtCharsetCreate. RwCameraClone (0x4ce6d0) takes the
 * far clip distance on the x87 stack and stays original code.
 */
#include "dt.h"
#include "dt_rwmath.h"
#include "rw/rwtexture.h"

/* ---- cameras ---------------------------------------------------------------------------------------- */

static int gUpdateCalls;

static RwCamera *BMX_CDECL stub_update(RwCamera *camera)
{
    rwtLog(0x4de690);
    rwtLogPtr(camera);
    gUpdateCalls++;
    return (RwCamera *)(uintptr_t)(dt_u32() % 3 == 0 ? 0u : (uint32_t)(uintptr_t)camera);
}

static int BMX_CDECL stub_clear(RwCamera *camera, const void *color, int32_t flags)
{
    rwtLog(0x4de8b0);
    rwtLogPtr(camera);
    rwtLogPtr(color);
    rwtLog((uint32_t)flags);
    return (int)(dt_u32() & 1);
}

/* A camera with random settings; optionally attached to a frame of a random tree. */
static RwCamera *make_camera(int attach)
{
    RwCamera *camera = dt_alloc(0xa8);
    dt_fill(camera, 0xa0);
    camera->object.object.parent = NULL;
    camera->projection = dt_u32() % 5 == 0 ? dt_range(0, 3) : dt_range(1, 2);
    camera->beginUpdate = stub_update;
    camera->endUpdate = stub_update;
    camera->nearPlane = mathFloat();
    camera->farPlane = mathFloat();
    camera->viewWindow.x = mathFloat();
    camera->viewWindow.y = mathFloat();
    if (attach) {
        RwFrame *frames[4];
        const int n = mathFrameTree(frames, 4);
        RwFrame *frame = frames[dt_range(0, n - 1)];
        camera->object.object.parent = frame;
        camera->object.lFrame.prev = &frame->objectList;
        camera->object.lFrame.next = frame->objectList.next;
        frame->objectList.next->prev = &camera->object.lFrame;
        frame->objectList.next = &camera->object.lFrame;
    }
    return camera;
}

static void camera_env(DtCtx *c)
{
    dt_seed(c);
    RwGlobals *g = mathEnv();
    g->stdFunc[rwSTANDARDCAMERACLEAR] = (uint32_t)(uintptr_t)stub_clear;
    gUpdateCalls = 0;
}

static void setup_camera_get(DtCtx *c)
{
    camera_env(c);
    DT_ARG_PTR(c, 0, make_camera(dt_u32() % 2));
    rwtEnd();
}

static void setup_camera_ptr(DtCtx *c)
{
    camera_env(c);
    DT_ARG_PTR(c, 0, make_camera(dt_u32() % 2));
    DT_ARG_PTR(c, 1, dt_u32() % 5 == 0 ? NULL : dt_alloc(0x40));
    rwtEnd();
}

static void setup_camera_float(DtCtx *c)
{
    camera_env(c);
    DT_ARG_PTR(c, 0, make_camera(dt_u32() % 2));
    DT_ARG_FLOAT(c, 1, mathFloat());
    rwtEnd();
}

static void setup_camera_vec2(DtCtx *c)
{
    camera_env(c);
    DT_ARG_PTR(c, 0, make_camera(dt_u32() % 2));
    RwV2d *v = dt_alloc(sizeof *v);
    v->x = mathFloat();
    v->y = mathFloat();
    DT_ARG_PTR(c, 1, v);
    rwtEnd();
}

static void setup_camera_projection(DtCtx *c)
{
    camera_env(c);
    DT_ARG_PTR(c, 0, make_camera(dt_u32() % 2));
    DT_ARG_INT(c, 1, dt_u32() % 4 == 0 ? dt_range(-1, 4) : dt_range(1, 2));
    rwtEnd();
}

static void setup_camera_frame(DtCtx *c)
{
    camera_env(c);
    DT_ARG_PTR(c, 0, make_camera(dt_u32() % 2));
    RwFrame *frames[4];
    const int n = mathFrameTree(frames, 4);
    DT_ARG_PTR(c, 1, dt_u32() % 6 == 0 ? NULL : frames[dt_range(0, n - 1)]);
    rwtEnd();
}

static void setup_camera_clear(DtCtx *c)
{
    camera_env(c);
    DT_ARG_PTR(c, 0, make_camera(0));
    DT_ARG_PTR(c, 1, dt_alloc(8));
    DT_ARG_INT(c, 2, dt_u32());
    rwtEnd();
}

static void setup_camera_create(DtCtx *c)
{
    camera_env(c);
    RwPluginRegistry *r = rwtRegistry(dt_range(0, 2), 0x140);
    __builtin_memcpy(&rwCameraRegistry, r, sizeof *r);
    gMathAllocFailAt = dt_u32() % 6 == 0 ? 1 : 0;
    gRwtCtorFailAt = dt_u32() % 8 == 0 ? 1 : 0;
    rwtEnd();
}

static void setup_camera_destroy(DtCtx *c)
{
    camera_env(c);
    RwPluginRegistry *r = rwtRegistry(dt_range(0, 2), 0x140);
    __builtin_memcpy(&rwCameraRegistry, r, sizeof *r);
    DT_ARG_PTR(c, 0, make_camera(dt_u32() % 2));
    rwtEnd();
}

static void setup_current_camera(DtCtx *c)
{
    camera_env(c);
    RwEngineInstance->curCamera = dt_u32() % 3 == 0 ? NULL : dt_alloc(0x10);
    rwtEnd();
}

static void setup_far_screen_z(DtCtx *c)
{
    camera_env(c);
    RwEngineInstance->dOpenDevice.zBufferFar = mathFloat();
    rwtEnd();
}

static int BMX_CDECL stub_indexed(int32_t primType, void *vertices, int32_t numVertices, void *indices,
                                  int32_t numIndices)
{
    rwtLog(0x4de100);
    rwtLog((uint32_t)primType);
    rwtLogPtr(vertices);
    rwtLog((uint32_t)numVertices);
    rwtLogPtr(indices);
    rwtLog((uint32_t)numIndices);
    return (int)(dt_u32() & 3);
}

static void setup_indexed(DtCtx *c)
{
    camera_env(c);
    RwEngineInstance->dOpenDevice.fpIm2DRenderIndexedPrimitive = (void *)stub_indexed;
    DT_ARG_INT(c, 0, dt_range(0, 6));
    DT_ARG_PTR(c, 1, dt_alloc(0x40));
    DT_ARG_INT(c, 2, dt_range(0, 30));
    DT_ARG_PTR(c, 3, dt_alloc(0x40));
    DT_ARG_INT(c, 4, dt_range(0, 60));
    rwtEnd();
}

/* ---- charset ---------------------------------------------------------------------------------------- */

/* Partial RwImage of the stubs: flags, width, height, depth, stride, pixels, palette. */
typedef struct StubImage {
    int32_t flags, width, height, depth, stride;
    uint8_t *pixels;
    RwRGBA *palette;
} StubImage;

static int gCharCalls, gCharFailAt;

static int char_fails(void)
{
    gCharCalls++;
    return gCharFailAt != 0 && gCharCalls == gCharFailAt;
}

static RwRaster *BMX_CDECL stub_raster_create(int32_t width, int32_t height, int32_t depth, int32_t flags)
{
    rwtLog(0x4dcb10);
    rwtLog((uint32_t)width);
    rwtLog((uint32_t)height);
    rwtLog((uint32_t)depth);
    rwtLog((uint32_t)flags);
    return char_fails() ? NULL : dt_alloc(0x40);
}

static int BMX_CDECL stub_raster_destroy(RwRaster *raster)
{
    rwtLog(0x4dcbd0);
    rwtLogPtr(raster);
    return 1;
}

static StubImage *BMX_CDECL stub_image_create(int32_t width, int32_t height, int32_t depth)
{
    rwtLog(0x4e38e0);
    rwtLog((uint32_t)width);
    rwtLog((uint32_t)height);
    rwtLog((uint32_t)depth);
    if (char_fails()) {
        return NULL;
    }
    StubImage *image = dt_alloc(sizeof *image);
    image->width = width;
    image->height = height;
    image->depth = depth;
    image->stride = width;
    return image;
}

static int BMX_CDECL stub_image_destroy(StubImage *image)
{
    rwtLog(0x4e3960);
    rwtLogPtr(image);
    return 1;
}

static StubImage *BMX_CDECL stub_image_alloc_pixels(StubImage *image)
{
    rwtLog(0x4e3ab0);
    rwtLogPtr(image);
    if (char_fails()) {
        return NULL;
    }
    image->pixels = dt_alloc((uint32_t)(image->stride * image->height) + 16);
    image->palette = dt_alloc(256 * 4);
    return image;
}

static uint8_t *BMX_CDECL stub_image_pixels(const StubImage *image)
{
    return image->pixels;
}

static RwRGBA *BMX_CDECL stub_image_palette(const StubImage *image)
{
    return image->palette;
}

/* Loading the raster logs the palette and a checksum of the pixels: the drawn font is compared that way. */
static RwRaster *BMX_CDECL stub_raster_from_image(RwRaster *raster, StubImage *image)
{
    rwtLog(0x4e7f70);
    rwtLogPtr(raster);
    rwtLogPtr(image);
    rwtLogWords(image->palette, 2);
    uint32_t sum = 0;
    for (int32_t i = 0; i < image->stride * image->height; i++) {
        sum = sum * 31u + image->pixels[i];
    }
    rwtLog(sum);
    return raster;
}

__attribute__((regparm(3))) static int stub_print(RtCharset *charset, const char *text, int32_t x,
                                                  int32_t unused0, int32_t unused1, int32_t unused2,
                                                  int32_t y)
{
    rwtLog(0x4cb860);
    rwtLogPtr(charset);
    rwtLogWords(text, 2);
    rwtLog((uint32_t)x);
    rwtLog((uint32_t)y);
    return (int)(dt_u32() & 7);
}

static void charset_env(DtCtx *c)
{
    camera_env(c);
    gCharCalls = 0;
    gCharFailAt = dt_u32() % 3 == 0 ? dt_range(1, 3) : 0;
    dt_stub(0x004dcb10, NULL, (void *)stub_raster_create);
    dt_stub(0x004dcbd0, NULL, (void *)stub_raster_destroy);
    dt_stub(0x004e38e0, NULL, (void *)stub_image_create);
    dt_stub(0x004e3960, NULL, (void *)stub_image_destroy);
    dt_stub(0x004e3ab0, NULL, (void *)stub_image_alloc_pixels);
    dt_stub(0x004e3a90, NULL, (void *)stub_image_pixels);
    dt_stub(0x004e3aa0, NULL, (void *)stub_image_palette);
    dt_stub(0x004e7f70, NULL, (void *)stub_raster_from_image);
}

static RwRGBA *random_color(void)
{
    RwRGBA *color = dt_alloc(sizeof *color);
    dt_fill(color, sizeof *color);
    return color;
}

static void setup_charset_create(DtCtx *c)
{
    charset_env(c);
    DT_ARG_PTR(c, 0, dt_u32() % 10 == 0 ? NULL : random_color());
    DT_ARG_PTR(c, 1, dt_u32() % 10 == 0 ? NULL : random_color());
    rwtEnd();
}

static void setup_charset_destroy(DtCtx *c)
{
    charset_env(c);
    DT_ARG_PTR(c, 0, dt_u32() % 6 == 0 ? NULL : dt_alloc(0x40));
    rwtEnd();
}

static void setup_charset_desc(DtCtx *c)
{
    charset_env(c);
    DT_ARG_PTR(c, 0, dt_u32() % 8 == 0 ? NULL : dt_alloc(0x40));
    DT_ARG_PTR(c, 1, dt_u32() % 8 == 0 ? NULL : dt_alloc(sizeof(RtCharsetDesc) + 4));
    rwtEnd();
}

static void setup_charset_print(DtCtx *c)
{
    charset_env(c);
    dt_stub(0x004cb860, NULL, (void *)stub_print);
    char *text = dt_alloc(16);
    dt_fill(text, 8);
    DT_ARG_PTR(c, 0, dt_u32() % 8 == 0 ? NULL : dt_alloc(0x40));
    DT_ARG_PTR(c, 1, dt_u32() % 8 == 0 ? NULL : text);
    DT_ARG_INT(c, 2, dt_u32());
    DT_ARG_INT(c, 3, dt_u32());
    rwtEnd();
}

/* ---- charset text ------------------------------------------------------------------------------------ */

static int32_t BMX_CDECL stub_strlen(const char *s)
{
    int32_t n = 0;
    while (s[n] != 0) {
        n++;
    }
    rwtLog(0x4a4f20);
    rwtLog((uint32_t)n);
    return n;
}

/* The rasters of these tests hold their width and height in the first two words. */
static int32_t BMX_CDECL stub_raster_width(const RwRaster *raster)
{
    rwtLog(0x4dcc30);
    rwtLogPtr(raster);
    return ((const int32_t *)raster)[0];
}

static int32_t BMX_CDECL stub_raster_height(const RwRaster *raster)
{
    rwtLog(0x4dcc40);
    rwtLogPtr(raster);
    return ((const int32_t *)raster)[1];
}

static int BMX_CDECL stub_state_set(int32_t state, int32_t value)
{
    rwtLog(0x4db820);
    rwtLog((uint32_t)state);
    rwtLog(dt_canon((uint32_t)value));
    return 1;
}

static int BMX_CDECL stub_state_get(int32_t state, void *value)
{
    rwtLog(0x4db870);
    rwtLog((uint32_t)state);
    rwtLogPtr(value);
    *(int32_t *)value = (int32_t)(dt_u32() & 0xff);
    return 1;
}

static int BMX_CDECL stub_text_primitive(int32_t primType, void *vertices, int32_t numVertices, void *indices,
                                         int32_t numIndices)
{
    rwtLog(0x4de101);
    rwtLog((uint32_t)primType);
    rwtLogPtr(vertices);
    rwtLog((uint32_t)numVertices);
    rwtLogWords(vertices, numVertices * 8);
    rwtLogPtr(indices);
    rwtLog((uint32_t)numIndices);
    for (int32_t i = 0; i < numIndices; i++) {
        rwtLog(((const uint16_t *)indices)[i]);
    }
    return 1;
}

static RwRaster *text_raster(int32_t width, int32_t height)
{
    int32_t *raster = dt_alloc(0x40);
    raster[0] = width;
    raster[1] = height;
    return (RwRaster *)raster;
}

static int text_orig(RtCharset *charset, const char *text, int32_t x, int32_t y)
{
    return ((int(__attribute__((regparm(3))) *)(RtCharset *, const char *, int32_t, int32_t, int32_t, int32_t,
                                                int32_t))(uintptr_t)0x004cb860)(charset, text, x, 0, 0, 0, y);
}

static int text_impl(RtCharset *charset, const char *text, int32_t x, int32_t y)
{
    return rtCharsetPrintText(charset, text, x, 0, 0, 0, y);
}

static void setup_print_text(DtCtx *c)
{
    camera_env(c);
    RwGlobals *g = RwEngineInstance;
    g->stringFuncs.rwstrlen = (void *)stub_strlen;
    g->dOpenDevice.fpRenderStateSet = stub_state_set;
    g->dOpenDevice.fpRenderStateGet = stub_state_get;
    g->dOpenDevice.fpIm2DRenderIndexedPrimitive = (void *)stub_text_primitive;
    dt_stub(0x004dcc30, NULL, (void *)stub_raster_width);
    dt_stub(0x004dcc40, NULL, (void *)stub_raster_height);
    RwCamera *camera = make_camera(0);
    camera->frameBuffer = dt_u32() % 10 == 0 ? NULL : text_raster(dt_range(1, 200), dt_range(1, 120));
    g->curCamera = dt_u32() % 10 == 0 ? NULL : camera;
    gRwtMallocFailAt = dt_u32() % 8 == 0 ? dt_range(1, 2) : 0;
    const int length = dt_u32() % 10 == 0 ? 0 : dt_range(1, 12);
    char *text = dt_alloc((uint32_t)length + 4);
    for (int i = 0; i < length; i++) {
        text[i] = (char)dt_range(0x20, 0x7a);
    }
    text[length] = 0;
    DT_ARG_PTR(c, 0, text_raster(dt_u32() % 2 == 0 ? 128 : dt_range(16, 256), dt_range(16, 256)));
    DT_ARG_PTR(c, 1, text);
    DT_ARG_INT(c, 2, dt_range(-40, 220));
    DT_ARG_INT(c, 3, dt_range(-30, 140));
    rwtEnd();
}

/* ---- the PVS camera ---------------------------------------------------------------------------------- */

static RwRaster *BMX_CDECL stub_sub_raster(RwRaster *raster, RwRaster *parent, const int32_t *rect)
{
    rwtLog(0x4dcc80);
    rwtLogPtr(raster);
    rwtLogPtr(parent);
    rwtLogPtr(rect);
    rwtLogWords(rect, 4);
    return raster;
}

static void *BMX_CDECL stub_world_add_camera(void *world, RwCamera *camera)
{
    rwtLog(0x4b81f0);
    rwtLogPtr(world);
    rwtLogPtr(camera);
    rwtLogFloatBits(camera->farPlane);
    rwtLogFloatBits(camera->nearPlane);
    return world;
}

/* EAX = world, EDX / ECX = parent rasters, the far clip plane in ST0. */
static void pvs_camera_call(uint32_t target, void *world, RwRaster *raster, RwRaster *zRaster,
                            const float *farClip)
{
    uint32_t eax = (uint32_t)(uintptr_t)world, edx = (uint32_t)(uintptr_t)raster;
    uint32_t ecx = (uint32_t)(uintptr_t)zRaster;
    __asm__ volatile("flds (%3)\n\tcall *%4"
                     : "+a"(eax), "+d"(edx), "+c"(ecx)
                     : "r"(farClip), "r"(target)
                     : "memory", "cc", "st");
}

static void pvs_camera_orig(void *world, RwRaster *raster, RwRaster *zRaster, const float *farClip)
{
    pvs_camera_call(0x004ce6d0, world, raster, zRaster, farClip);
}

static void pvs_camera_impl(void *world, RwRaster *raster, RwRaster *zRaster, const float *farClip)
{
    pvs_camera_call((uint32_t)(uintptr_t)rwPvsCameraCreate, world, raster, zRaster, farClip);
}

static void setup_pvs_camera(DtCtx *c)
{
    charset_env(c);
    RwPluginRegistry *r = rwtRegistry(dt_range(0, 2), 0x140);
    __builtin_memcpy(&rwCameraRegistry, r, sizeof *r);
    mathFrameRegistry();
    gCharFailAt = 0;
    dt_stub(0x004dcc80, NULL, (void *)stub_sub_raster);
    dt_stub(0x004b81f0, NULL, (void *)stub_world_add_camera);
    float *farClip = dt_alloc(4);
    *farClip = dt_u32() % 6 == 0 ? mathFloat() : dt_float(1.0f, 5000.0f);
    DT_ARG_PTR(c, 0, dt_alloc(0x20));
    DT_ARG_PTR(c, 1, dt_u32() % 2 == 0 ? NULL : dt_alloc(0x40));
    DT_ARG_PTR(c, 2, dt_u32() % 2 == 0 ? NULL : dt_alloc(0x40));
    DT_ARG_PTR(c, 3, farClip);
    rwtEnd();
}

/* ---- clip planes (EAX = camera) ------------------------------------------------------------------------ */

static void clip_call(uint32_t target, RwCamera *camera)
{
    uint32_t eax = (uint32_t)(uintptr_t)camera;
    __asm__ volatile("call *%1"
                     : "+a"(eax)
                     : "r"(target)
                     : "memory", "cc", "ecx", "edx", "st", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5",
                       "xmm6", "xmm7");
}

static void persp_orig(RwCamera *camera)
{
    clip_call(0x004dd4d0, camera);
}

static void persp_impl(RwCamera *camera)
{
    clip_call((uint32_t)(uintptr_t)CameraBuildPerspClipPlanes, camera);
}

static void parallel_orig(RwCamera *camera)
{
    clip_call(0x004ddeb0, camera);
}

static void parallel_impl(RwCamera *camera)
{
    clip_call((uint32_t)(uintptr_t)CameraBuildParallelClipPlanes, camera);
}

/* The vector transform behind the V3d module: an affine map with the matrix's own rows (logged). */
static RwV3d *BMX_CDECL stub_transform_points(RwV3d *out, const RwV3d *in, int32_t count, const RwMatrix *m)
{
    rwtLog(0x4d3a00);
    rwtLogPtr(out);
    rwtLogPtr(in);
    rwtLog((uint32_t)count);
    rwtLogPtr(m);
    for (int32_t i = 0; i < count; i++) {
        const RwV3d v = in[i];
        out[i].x = v.x * m->right.x + v.y * m->up.x + v.z * m->at.x + m->pos.x;
        out[i].y = v.x * m->right.y + v.y * m->up.y + v.z * m->at.y + m->pos.y;
        out[i].z = v.x * m->right.z + v.y * m->up.z + v.z * m->at.z + m->pos.z;
    }
    return out;
}

/*
 * A float: mostly moderate, now and then an edge value (zero, denormal, huge, infinite) or a tiny one. No NaN
 * inputs: where two different NaNs meet, the x87 keeps the one with the larger significand and SSE the first
 * operand, which plain C cannot follow (NaNs the arithmetic itself generates are the same on both).
 */
static float clip_float(float lo, float hi)
{
    static const uint32_t edges[] = {0x00000000, 0x80000000, 0x00000001, 0x7f7fffff,
                                     0x7f800000, 0xff800000, 0x3f800000, 0xbf800000};
    const uint32_t r = dt_u32() % 16;
    if (r == 0) {
        float f;
        const uint32_t bits = edges[dt_u32() % (sizeof edges / sizeof edges[0])];
        __builtin_memcpy(&f, &bits, 4);
        return f;
    }
    if (r == 1) {
        return dt_float(-1e-20f, 1e-20f);
    }
    return dt_float_edgy(lo, hi);
}

/* A camera with a frame: rotation-like or random LTM, random window, offset and clip distances. */
static void setup_clip_planes(DtCtx *c)
{
    camera_env(c);
    RwV3dModule *v3d = (RwV3dModule *)((uint8_t *)RwEngineInstance + rwFastSqrtOffset);
    v3d->transformPoints = stub_transform_points;
    uint32_t *invSqrt = dt_alloc(4096 * 4); /* table entries are finite (mathEnv's random ones can be NaN) */
    for (int i = 0; i < 4096; i++) {
        invSqrt[i] = (dt_u32() & 0x007fffffu) | 0x3f000000u;
    }
    v3d->invSqrtTable = invSqrt;
    RwCamera *camera = dt_alloc(sizeof(RwCamera));
    dt_fill(camera, sizeof(RwCamera));
    RwFrame *frame = dt_alloc(sizeof(RwFrame));
    dt_fill(frame, sizeof(RwFrame));
    float *f = (float *)&frame->ltm;
    const float range = dt_u32() % 4 == 0 ? 1e6f : 1.0f;
    for (int i = 0; i < 16; i++) {
        if (i % 4 != 3) {
            f[i] = i >= 12 ? clip_float(-500.0f, 500.0f) : clip_float(-range, range);
        }
    }
    camera->object.object.parent = frame;
    camera->viewWindow.x = clip_float(0.0f, 2.0f);
    camera->viewWindow.y = clip_float(0.0f, 2.0f);
    camera->viewOffset.x = dt_u32() % 3 == 0 ? 0.0f : clip_float(-1.0f, 1.0f);
    camera->viewOffset.y = dt_u32() % 3 == 0 ? 0.0f : clip_float(-1.0f, 1.0f);
    camera->nearPlane = clip_float(0.0f, 5.0f);
    camera->farPlane = clip_float(0.0f, 5000.0f);
    DT_ARG_PTR(c, 0, camera);
    rwtEnd();
}

/* ---- view matrices (EAX = camera, returned in EAX) and the sync callback ---------------------------------
 */

static uint32_t view_call(uint32_t target, RwCamera *camera)
{
    uint32_t eax = (uint32_t)(uintptr_t)camera;
    __asm__ volatile("call *%1"
                     : "+a"(eax)
                     : "r"(target)
                     : "memory", "cc", "ecx", "edx", "st", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5",
                       "xmm6", "xmm7");
    return eax;
}

static uint32_t persp_view_orig(RwCamera *camera)
{
    return view_call(0x004ddd40, camera);
}

static uint32_t persp_view_impl(RwCamera *camera)
{
    return view_call((uint32_t)(uintptr_t)CameraBuildPerspViewMatrix, camera);
}

static uint32_t parallel_view_orig(RwCamera *camera)
{
    return view_call(0x004de2a0, camera);
}

static uint32_t parallel_view_impl(RwCamera *camera)
{
    return view_call((uint32_t)(uintptr_t)CameraBuildParallelViewMatrix, camera);
}

/* A camera on a frame with a random LTM (moderate values, edges and infinities, no NaN), window and offset.
 */
static RwCamera *make_view_camera(void)
{
    RwCamera *camera = dt_alloc(sizeof(RwCamera));
    dt_fill(camera, sizeof(RwCamera));
    RwFrame *frame = dt_alloc(sizeof(RwFrame));
    dt_fill(frame, sizeof(RwFrame));
    float *f = (float *)&frame->ltm;
    const float range = dt_u32() % 4 == 0 ? 1e6f : 4.0f;
    for (int i = 0; i < 16; i++) {
        if (i % 4 != 3) {
            f[i] = clip_float(-range, range);
        }
    }
    camera->object.object.parent = frame;
    camera->recipViewWindow.x = clip_float(-3.0f, 3.0f);
    camera->recipViewWindow.y = clip_float(-3.0f, 3.0f);
    camera->viewOffset.x = dt_u32() % 3 == 0 ? 0.0f : clip_float(-1.0f, 1.0f);
    camera->viewOffset.y = dt_u32() % 3 == 0 ? 0.0f : clip_float(-1.0f, 1.0f);
    return camera;
}

static void setup_view_matrix(DtCtx *c)
{
    camera_env(c);
    DT_ARG_PTR(c, 0, make_view_camera());
    rwtEnd();
}

/* Projection 1 or 2 mostly, now and then another value; finite corners for the bounding box of the others. */
static void setup_camera_sync(DtCtx *c)
{
    setup_clip_planes(c);
    RwCamera *camera = (RwCamera *)(uintptr_t)c->args[0];
    camera->projection = dt_u32() % 5 == 0 ? dt_range(-1, 4) : dt_range(1, 2);
    camera->recipViewWindow.x = clip_float(-3.0f, 3.0f);
    camera->recipViewWindow.y = clip_float(-3.0f, 3.0f);
    for (int i = 0; i < 8; i++) {
        camera->frustumCorners[i].x = dt_float(-100.0f, 100.0f);
        camera->frustumCorners[i].y = dt_float(-100.0f, 100.0f);
        camera->frustumCorners[i].z = dt_float(-100.0f, 100.0f);
    }
    rwtEnd();
}

/* ---- begin / end update and the sphere test ---------------------------------------------------------- */

static int BMX_CDECL stub_device_update(void *out, RwCamera *camera, int32_t flags)
{
    rwtLog(0x4d1e);
    rwtLogPtr(out);
    rwtLogPtr(camera);
    rwtLog((uint32_t)flags);
    return (int)(dt_u32() % 4 != 0);
}

static void BMX_CDECL stub_core_ext(void *camera)
{
    rwtLog(0xe5107);
    rwtLogPtr(camera);
}

static void BMX_CDECL stub_frame_sync(void *object)
{
    rwtLog(0x5111c);
    rwtLogPtr(object);
}

/*
 * A forest of frames (queued roots are in the engine's dirty list) with 0..2 objects each whose sync function
 * logs; the first root gets a parent so that it can be synced on its own. Returns the frame count.
 */
static int synced_forest(RwFrame **frames, int maxN)
{
    const int n = mathFrameTree(frames, maxN);
    for (int i = 0; i < n; i++) {
        for (int k = dt_range(0, 2); k > 0; k--) {
            RwObjectHasFrame *object = dt_alloc(sizeof *object);
            object->sync = (void *)stub_frame_sync;
            object->lFrame.prev = frames[i]->objectList.prev;
            object->lFrame.next = &frames[i]->objectList;
            frames[i]->objectList.prev->next = &object->lFrame;
            frames[i]->objectList.prev = &object->lFrame;
        }
    }
    RwFrame *parent = dt_alloc(sizeof(RwFrame));
    mathFillMatrix(&parent->ltm);
    frames[0]->object.parent = parent;
    return n;
}

/* A raster object whose parent has the given size (it is its own parent for a normal raster). */
static RwRaster *make_sized_raster(int32_t width, int32_t height, int self)
{
    RwRaster *raster = dt_alloc(sizeof(RwRaster));
    dt_fill(raster, sizeof(RwRaster));
    raster->parent = raster;
    raster->width = width;
    raster->height = height;
    if (!self) {
        RwRaster *parent = dt_alloc(sizeof(RwRaster));
        dt_fill(parent, sizeof(RwRaster));
        parent->width = width;
        parent->height = height;
        raster->parent = parent;
        raster->width = dt_range(1, 3); /* a sub raster's own size is not what is compared */
    }
    return raster;
}

static void setup_begin_update(DtCtx *c)
{
    camera_env(c);
    RwGlobals *g = RwEngineInstance;
    g->stdFunc[1] = (uint32_t)(uintptr_t)stub_device_update;
    g->stdFunc[10] = (uint32_t)(uintptr_t)stub_device_update;
    ((uint32_t *)(uintptr_t)0x0069bee0)[1] = (uint32_t)(uintptr_t)stub_core_ext;
    g->curCamera = dt_alloc(8);
    RwCamera *camera = make_camera(0);
    const int32_t w = dt_range(1, 3), h = dt_range(1, 3);
    camera->frameBuffer = dt_u32() % 8 == 0 ? NULL : make_sized_raster(w, h, dt_u32() % 2);
    camera->zBuffer =
        dt_u32() % 3 == 0 ? NULL : make_sized_raster(dt_u32() % 4 == 0 ? w + 1 : w, h, dt_u32() % 2);
    if (dt_u32() % 7 == 0 && camera->zBuffer != NULL) {
        camera->zBuffer->parent->height = h + 1;
    }
    /* dirty frames with objects: the sync runs between the checks and the device call */
    RwFrame *frames[5];
    synced_forest(frames, 5);
    DT_ARG_PTR(c, 0, dt_u32() % 12 == 0 ? NULL : camera);
    rwtEnd();
}

static void setup_end_update(DtCtx *c)
{
    camera_env(c);
    RwGlobals *g = RwEngineInstance;
    g->stdFunc[1] = (uint32_t)(uintptr_t)stub_device_update;
    g->stdFunc[10] = (uint32_t)(uintptr_t)stub_device_update;
    g->curCamera = dt_alloc(8);
    DT_ARG_PTR(c, 0, make_camera(0));
    rwtEnd();
}

/*
 * Planes with small integer normals and distances and spheres on integer centres half of the time, so that
 * the distance equals the radius (or its negation) exactly; otherwise random edgy values. A NaN radius now
 * and then (a single NaN: two different ones meet differently on the x87 and SSE).
 */
static void setup_frustum_sphere(DtCtx *c)
{
    camera_env(c);
    RwCamera *camera = dt_alloc(sizeof(RwCamera));
    dt_fill(camera, sizeof(RwCamera));
    float *sphere = dt_alloc(16);
    const int integral = dt_u32() % 2;
    for (int i = 0; i < 6; i++) {
        RwFrustumPlane *plane = &camera->frustumPlanes[i];
        if (integral) {
            plane->normal.x = (float)dt_range(-2, 2);
            plane->normal.y = (float)dt_range(-2, 2);
            plane->normal.z = (float)dt_range(-2, 2);
            plane->distance = (float)dt_range(-6, 6);
        } else {
            plane->normal.x = clip_float(-2.0f, 2.0f);
            plane->normal.y = clip_float(-2.0f, 2.0f);
            plane->normal.z = clip_float(-2.0f, 2.0f);
            plane->distance = clip_float(-100.0f, 100.0f);
        }
    }
    for (int i = 0; i < 3; i++) {
        sphere[i] = integral ? (float)dt_range(-3, 3) : clip_float(-50.0f, 50.0f);
    }
    sphere[3] = integral ? (float)dt_range(-1, 5) : clip_float(-10.0f, 10.0f);
    if (dt_u32() % 12 == 0) {
        const uint32_t nan = 0x7fc00000u;
        __builtin_memcpy(&sphere[3], &nan, 4);
    }
    DT_ARG_PTR(c, 0, camera);
    DT_ARG_PTR(c, 1, sphere);
    rwtEnd();
}

/* ---- frame sync (called by the camera's begin update) ------------------------------------------------ */

static void setup_sync_dirty_list(DtCtx *c)
{
    camera_env(c);
    RwFrame *frames[6];
    synced_forest(frames, 6);
    rwtEnd();
}

/* EAX = frame, EDX = force; EAX is not an output (the original leaves the frame there or a callee result). */
static void sync_call(uint32_t target, RwFrame *frame, uint32_t force)
{
    uint32_t eax = (uint32_t)(uintptr_t)frame, edx = force;
    __asm__ volatile("call *%2"
                     : "+a"(eax), "+d"(edx)
                     : "r"(target)
                     : "memory", "cc", "ecx", "st", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6",
                       "xmm7");
}

static void sync_objects_orig(RwFrame *frame, uint32_t force)
{
    sync_call(0x004ee430, frame, force);
}

static void sync_objects_impl(RwFrame *frame, uint32_t force)
{
    sync_call((uint32_t)(uintptr_t)rwFrameSyncObjects, frame, force);
}

static void setup_sync_objects(DtCtx *c)
{
    camera_env(c);
    RwFrame *frames[6];
    synced_forest(frames, 6);
    static const uint32_t forces[] = {0, 4, 8, 0xc, 1};
    DT_ARG_PTR(c, 0, dt_u32() % 10 == 0 ? NULL : frames[0]);
    DT_ARG_INT(c, 1, dt_u32() % 8 == 0 ? dt_u32() : forces[dt_u32() % 5]);
    rwtEnd();
}

static void setup_sync_object_cb(DtCtx *c)
{
    camera_env(c);
    RwObjectHasFrame *object = dt_alloc(sizeof *object);
    object->sync = (void *)stub_frame_sync;
    DT_ARG_PTR(c, 0, object);
    DT_ARG_INT(c, 1, dt_u32());
    rwtEnd();
}

DT_CASES(rwcamera_cases) = {
    {"rwFrameSyncDirtyList", 0x004ee5d0, rwFrameSyncDirtyList, 0, DT_RET_INT, setup_sync_dirty_list, 600, 0},
    {"rwFrameSyncObjects", (uint32_t)(uintptr_t)sync_objects_orig, sync_objects_impl, 2, DT_RET_VOID,
     setup_sync_objects, 600, 0},
    {"rwFrameSyncObjectCB", 0x004ee410, rwFrameSyncObjectCB, 2, DT_RET_INT, setup_sync_object_cb, 30, 0},
    {"CameraBuildPerspViewMatrix", (uint32_t)(uintptr_t)persp_view_orig, persp_view_impl, 1, DT_RET_INT,
     setup_view_matrix, 2000, 0},
    {"CameraBuildParallelViewMatrix", (uint32_t)(uintptr_t)parallel_view_orig, parallel_view_impl, 1,
     DT_RET_INT, setup_view_matrix, 2000, 0},
    {"CameraSync", 0x004de400, CameraSync, 1, DT_RET_INT, setup_camera_sync, 1500, 0},
    {"CameraBeginUpdate", 0x004de690, CameraBeginUpdate, 1, DT_RET_INT, setup_begin_update, 800, 0},
    {"CameraEndUpdate", 0x004de7b0, CameraEndUpdate, 1, DT_RET_INT, setup_end_update, 100, 0},
    {"RwCameraFrustumTestSphere", 0x004de820, RwCameraFrustumTestSphere, 2, DT_RET_INT, setup_frustum_sphere,
     2000, 0},
    {"CameraBuildPerspClipPlanes", (uint32_t)(uintptr_t)persp_orig, persp_impl, 1, DT_RET_VOID,
     setup_clip_planes, 2000, 0},
    {"CameraBuildParallelClipPlanes", (uint32_t)(uintptr_t)parallel_orig, parallel_impl, 1, DT_RET_VOID,
     setup_clip_planes, 1000, 0},
    {"rtCharsetPrintText", (uint32_t)(uintptr_t)text_orig, text_impl, 4, DT_RET_INT, setup_print_text, 800,
     0},
    {"rwPvsCameraCreate", (uint32_t)(uintptr_t)pvs_camera_orig, pvs_camera_impl, 4, DT_RET_VOID,
     setup_pvs_camera, 200, 0},
    {"RwIm2DGetFarScreenZ", 0x004db810, RwIm2DGetFarScreenZ, 0, DT_RET_FLOAT, setup_far_screen_z, 30, 0},
    {"RwIm2DRenderIndexedPrimitive", 0x004db950, RwIm2DRenderIndexedPrimitive, 5, DT_RET_INT, setup_indexed,
     30, 0},
    {"RtCharsetGetDesc", 0x004cb3f0, RtCharsetGetDesc, 2, DT_RET_INT, setup_charset_desc, 60, 0},
    {"RtCharsetPrint", 0x004cbef0, RtCharsetPrint, 4, DT_RET_INT, setup_charset_print, 100, 0},
    {"RtCharsetDestroy", 0x004cbf60, RtCharsetDestroy, 1, DT_RET_INT, setup_charset_destroy, 30, 0},
    {"RtCharsetCreate", 0x004cbfb0, RtCharsetCreate, 2, DT_RET_INT, setup_charset_create, 300, 0},
    {"RwCameraSetViewOffset", 0x004de540, RwCameraSetViewOffset, 2, DT_RET_INT, setup_camera_vec2, 200, 0},
    {"RwCameraSetRaster", 0x004de580, RwCameraSetRaster, 2, DT_RET_INT, setup_camera_ptr, 50, 0},
    {"RwCameraGetRaster", 0x004de5a0, RwCameraGetRaster, 1, DT_RET_INT, setup_camera_get, 30, 0},
    {"RwCameraSetZRaster", 0x004de5b0, RwCameraSetZRaster, 2, DT_RET_INT, setup_camera_ptr, 50, 0},
    {"RwCameraGetZRaster", 0x004de5d0, RwCameraGetZRaster, 1, DT_RET_INT, setup_camera_get, 30, 0},
    {"RwCameraSetNearClipPlane", 0x004de5e0, RwCameraSetNearClipPlane, 2, DT_RET_INT, setup_camera_float, 800,
     0},
    {"RwCameraGetNearClipPlane", 0x004de610, RwCameraGetNearClipPlane, 1, DT_RET_FLOAT, setup_camera_get, 30,
     0},
    {"RwCameraSetFarClipPlane", 0x004de620, RwCameraSetFarClipPlane, 2, DT_RET_INT, setup_camera_float, 800,
     0},
    {"RwCameraGetFarClipPlane", 0x004de650, RwCameraGetFarClipPlane, 1, DT_RET_FLOAT, setup_camera_get, 30,
     0},
    {"RwCameraSetFogDistance", 0x004de660, RwCameraSetFogDistance, 2, DT_RET_INT, setup_camera_float, 50, 0},
    {"RwCameraBeginUpdate", 0x004de790, RwCameraBeginUpdate, 1, DT_RET_INT, setup_camera_get, 50, 0},
    {"RwCameraEndUpdate", 0x004de7f0, RwCameraEndUpdate, 1, DT_RET_INT, setup_camera_get, 50, 0},
    {"RwCameraGetCurrentCamera", 0x004de810, RwCameraGetCurrentCamera, 0, DT_RET_INT, setup_current_camera,
     20, 0},
    {"RwCameraClear", 0x004de8a0, RwCameraClear, 3, DT_RET_INT, setup_camera_clear, 100, 0},
    {"RwCameraSetProjection", 0x004de920, RwCameraSetProjection, 2, DT_RET_INT, setup_camera_projection, 600,
     0},
    {"RwCameraSetViewWindow", 0x004de9b0, RwCameraSetViewWindow, 2, DT_RET_INT, setup_camera_vec2, 600, 0},
    {"RwCameraGetViewWindow", 0x004dea10, RwCameraGetViewWindow, 1, DT_RET_INT, setup_camera_get, 20, 0},
    {"RwCameraSetFrame", 0x004dea60, RwCameraSetFrame, 2, DT_RET_INT, setup_camera_frame, 300, 0},
    {"RwCameraGetFrame", 0x004dea80, RwCameraGetFrame, 1, DT_RET_INT, setup_camera_get, 30, 0},
    {"RwCameraDestroy", 0x004dea90, RwCameraDestroy, 1, DT_RET_INT, setup_camera_destroy, 150, 0},
    {"RwCameraCreate", 0x004deae0, RwCameraCreate, 0, DT_RET_INT, setup_camera_create, 400, 0},
};
