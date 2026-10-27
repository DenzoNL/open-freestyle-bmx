/*
 * Tests for rw/camera.c and rw/charset.c: the camera object and its settings, the calls through the device
 * table, and the charset raster. The engine instance is the maths fixture (dt_rwmath.h); the raster and image
 * routines the charset calls are stubs that behave like the real ones (they hand out objects, log their
 * arguments and can fail).
 *
 * CameraUpdateZShiftScale (EAX = camera) and RtCharsetSetColors (EAX, EDX, ECX) take register arguments and
 * are exercised through the setters, RwCameraCreate and RtCharsetCreate. RwCameraClone (0x4ce6d0) takes the
 * far clip distance on the x87 stack and stays original code.
 */
#include "dt.h"
#include "dt_rwmath.h"

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

DT_CASES(rwcamera_cases) = {
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
