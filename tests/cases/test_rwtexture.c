/*
 * Tests for rw/texture.c: rasters, textures and dictionaries, images, the gamma tables and the image format
 * registry. (The BMP and PNG readers are in test_rwimage.c.) The engine instance is the texture fixture of
 * dt_rwtex.h: free lists, string functions and the device's standard functions log their arguments.
 */
#include "dt_rwtex.h"

static void BMX_CDECL stResourcesPurge(void)
{
    rwtLog(0x4dc3f0);
}

static void stubsPurge(void)
{
    dt_stub(0x004dc3f0, NULL, (void *)stResourcesPurge);
}

/* ---- rasters ---------------------------------------------------------------------------------------- */

static void setup_raster_create(DtCtx *c)
{
    texEnv(c);
    gTexAllocFailAt = dt_u32() % 6 == 0 ? 1 : 0;
    DT_ARG_INT(c, 0, dt_range(0, 512));
    DT_ARG_INT(c, 1, dt_range(0, 512));
    DT_ARG_INT(c, 2, dt_range(0, 32));
    DT_ARG_INT(c, 3, (int32_t)dt_u32());
    rwtEnd();
}

static void setup_raster(DtCtx *c)
{
    texEnv(c);
    DT_ARG_PTR(c, 0, mkRaster());
    rwtEnd();
}

/* A sub raster inside (or just outside) the parent, to reach every check of the rectangle. */
static void setup_subraster(DtCtx *c)
{
    texEnv(c);
    RwRaster *sub = mkRaster();
    RwRaster *parent = mkRaster();
    parent->width = dt_range(1, 200);
    parent->height = dt_range(1, 200);
    int32_t *rect = dt_alloc(16);
    rect[0] = dt_range(-3, parent->width + 3);
    rect[1] = dt_range(-3, parent->height + 3);
    rect[2] = dt_range(0, parent->width - rect[0] + 3);
    rect[3] = dt_range(0, parent->height - rect[1] + 3);
    if (dt_u32() % 5 == 0) {
        rect[dt_u32() % 4] = (int32_t)dt_u32();
    }
    if (dt_u32() % 5 == 0) { /* an empty rectangle on the far edge: the bound checks are strict */
        rect[0] = parent->width - dt_range(0, 1);
        rect[1] = parent->height - dt_range(0, 1);
        rect[2] = 0;
        rect[3] = dt_range(0, 1);
    }
    DT_ARG_PTR(c, 0, sub);
    DT_ARG_PTR(c, 1, parent);
    DT_ARG_PTR(c, 2, rect);
    rwtEnd();
}

static void setup_push(DtCtx *c)
{
    texEnv(c);
    RwRasterGlobals *g = rasterGlobals();
    g->depth = dt_range(0, 10);
    for (int i = 0; i < 10; i++) {
        g->stack[i] = dt_u32() % 2 ? mkRaster() : NULL;
    }
    DT_ARG_PTR(c, 0, mkRaster());
    rwtEnd();
}

static void setup_pop(DtCtx *c)
{
    texEnv(c);
    RwRasterGlobals *g = rasterGlobals();
    g->depth = dt_range(-1, 9);
    for (int i = 0; i < 10; i++) {
        g->stack[i] = dt_u32() % 2 ? mkRaster() : NULL;
    }
    rwtEnd();
}

static void setup_show(DtCtx *c)
{
    texEnv(c);
    stubsPurge();
    DT_ARG_PTR(c, 0, mkRaster());
    DT_ARG_PTR(c, 1, dt_alloc(8));
    DT_ARG_INT(c, 2, (int32_t)dt_u32());
    rwtEnd();
}

static void setup_lock_palette(DtCtx *c)
{
    texEnv(c);
    RwRaster *r = mkRaster();
    r->privateFlags = (uint8_t)(dt_u32() % 2 ? dt_u32() & ~0x18u : dt_u32());
    DT_ARG_PTR(c, 0, r);
    DT_ARG_INT(c, 1, dt_range(0, 3));
    rwtEnd();
}

static void setup_unlock_palette(DtCtx *c)
{
    texEnv(c);
    RwRaster *r = mkRaster();
    r->privateFlags = (uint8_t)(dt_u32() % 2 ? dt_u32() | 0x08u : dt_u32());
    DT_ARG_PTR(c, 0, r);
    rwtEnd();
}

/* ---- textures and dictionaries ---------------------------------------------------------------------- */

static void setup_callback_set(DtCtx *c)
{
    texEnv(c);
    texGlobals()->readCallBack = (RwTextureCallBackRead)(uintptr_t)dt_u32();
    texGlobals()->findCallBack = (RwTextureCallBackFind)(uintptr_t)dt_u32();
    texGlobals()->mipmapping = (int32_t)dt_u32();
    DT_ARG_INT(c, 0, dt_u32());
    rwtEnd();
}

static void setup_callback_get(DtCtx *c)
{
    setup_callback_set(c);
}

static void setup_texture_set_raster(DtCtx *c)
{
    texEnv(c);
    DT_ARG_PTR(c, 0, mkTexture());
    DT_ARG_PTR(c, 1, dt_u32() % 5 == 0 ? NULL : mkRaster());
    rwtEnd();
}

static void setup_texture(DtCtx *c)
{
    texEnv(c);
    DT_ARG_PTR(c, 0, mkTexture());
    rwtEnd();
}

static void setup_texture_int(DtCtx *c)
{
    texEnv(c);
    DT_ARG_PTR(c, 0, mkTexture());
    DT_ARG_INT(c, 1, (int32_t)dt_u32());
    rwtEnd();
}

static void setup_dict_create(DtCtx *c)
{
    texEnv(c);
    for (int n = dt_range(0, 3); n > 0; n--) {
        mkDict(dt_range(0, 2), 1);
    }
    gTexAllocFailAt = dt_u32() % 6 == 0 ? 1 : 0;
    rwtEnd();
}

/* A texture that is in a dictionary (half the time), with a raster, names and a reference count 0..4. */
static RwTexture *mkInDict(void)
{
    RwTexDictionary *d = mkDict(dt_range(0, 2), 1);
    RwTexture *t = mkTexture();
    if (dt_u32() % 2) {
        dictPut(d, t);
    }
    t->refCount = (uint32_t)dt_range(0, 4);
    return t;
}

static void setup_texture_destroy(DtCtx *c)
{
    texEnv(c);
    DT_ARG_PTR(c, 0, mkInDict());
    rwtEnd();
}

static void setup_texture_destroy_all(DtCtx *c)
{
    texEnv(c);
    RwTexture *t = mkInDict();
    t->refCount = (uint32_t)dt_range(1, 5);
    DT_ARG_PTR(c, 0, t);
    rwtEnd();
}

static void setup_dict_destroy(DtCtx *c)
{
    texEnv(c);
    RwTexDictionary *d = mkDict(dt_range(0, 4), 1);
    for (RwLLLink *l = d->texturesInDict.next; l != &d->texturesInDict; l = l->next) {
        ((RwTexture *)((uint8_t *)l - offsetof(RwTexture, lInDictionary)))->refCount =
            (uint32_t)dt_range(1, 3);
    }
    if (dt_u32() % 2) {
        texGlobals()->currentDict = d;
    } else if (dt_u32() % 2) {
        texGlobals()->currentDict = mkDict(1, 1);
    }
    mkDict(dt_range(0, 2), 1);
    DT_ARG_PTR(c, 0, d);
    rwtEnd();
}

static void setup_texture_create(DtCtx *c)
{
    texEnv(c);
    gTexAllocFailAt = dt_u32() % 6 == 0 ? 1 : 0;
    DT_ARG_PTR(c, 0, dt_u32() % 4 == 0 ? NULL : mkRaster());
    rwtEnd();
}

static void setup_set_name(DtCtx *c)
{
    texEnv(c);
    DT_ARG_PTR(c, 0, mkTexture());
    DT_ARG_PTR(c, 1, mkName(poolName()));
    gTexAllocFailAt = 0;
    RwEngineInstance->memoryFuncs.rwmalloc = rwtMalloc;
    gRwtMallocFailAt = dt_u32() % 6 == 0 ? 1 : 0;
    rwtEnd();
}

static void setup_dict_add(DtCtx *c)
{
    texEnv(c);
    RwTexDictionary *d = mkDict(dt_range(0, 3), 1);
    RwTexture *t = mkTexture();
    if (dt_u32() % 2) {
        dictPut(mkDict(dt_range(0, 2), 1), t);
    }
    if (dt_u32() % 4 == 0) {
        dictPut(d, t);
    }
    DT_ARG_PTR(c, 0, d);
    DT_ARG_PTR(c, 1, t);
    rwtEnd();
}

static void setup_dict_set_current(DtCtx *c)
{
    texEnv(c);
    texGlobals()->currentDict = dt_u32() % 2 ? mkDict(1, 1) : NULL;
    DT_ARG_PTR(c, 0, dt_u32() % 3 == 0 ? NULL : mkDict(1, 1));
    rwtEnd();
}

/* What the read callback is made to answer, and a log of its calls. */
static RwTexture *gReadResult;

static void *BMX_CDECL stReadCallback(const char *name, const char *mask)
{
    rwtLog(0x7ead);
    rwtLogPtr(name);
    rwtLogPtr(mask);
    return gReadResult;
}

static void setup_texture_read(DtCtx *c)
{
    texEnv(c);
    texGlobals()->readCallBack = stReadCallback;
    for (int n = dt_range(0, 3); n > 0; n--) {
        mkDict(dt_range(0, 3), 1);
    }
    RwTexDictionary *current = NULL;
    if (dt_u32() % 2) {
        current = mkDict(dt_range(0, 3), 1);
        texGlobals()->currentDict = current;
    }
    switch (dt_u32() % 3) {
    case 0:
        gReadResult = NULL;
        break;
    case 1:
        gReadResult = mkTexture();
        break;
    default: {
        RwTexDictionary *other = mkDict(1, 1);
        gReadResult =
            (RwTexture *)((uint8_t *)other->texturesInDict.next - offsetof(RwTexture, lInDictionary));
        break;
    }
    }
    DT_ARG_PTR(c, 0, mkName(poolName()));
    DT_ARG_PTR(c, 1, dt_u32() % 2 ? NULL : mkName(poolName()));
    rwtEnd();
}

/* ---- images ----------------------------------------------------------------------------------------- */

static void setup_image_create(DtCtx *c)
{
    texEnv(c);
    gTexAllocFailAt = dt_u32() % 6 == 0 ? 1 : 0;
    DT_ARG_INT(c, 0, dt_range(0, 64));
    DT_ARG_INT(c, 1, dt_range(0, 64));
    DT_ARG_INT(c, 2, dt_u32() % 4 == 0 ? (int32_t)dt_u32() : dt_range(0, 32));
    rwtEnd();
}

static void setup_image(DtCtx *c)
{
    texEnv(c);
    DT_ARG_PTR(c, 0, mkImage(dt_u32() % 2));
    rwtEnd();
}

static void setup_image_alloc(DtCtx *c)
{
    texEnv(c);
    RwImage *im = mkImage(0);
    if (dt_u32() % 4 == 0) {
        im->pixels = dt_alloc(8);
    }
    im->width = dt_u32() % 8 == 0 ? dt_range(0, 3000) : dt_range(0, 60);
    gRwtMallocFailAt = dt_u32() % 6 == 0 ? 1 : 0;
    DT_ARG_PTR(c, 0, im);
    rwtEnd();
}

static void setup_image_free(DtCtx *c)
{
    texEnv(c);
    DT_ARG_PTR(c, 0, mkImage(1));
    rwtEnd();
}

static void setup_set_path(DtCtx *c)
{
    texEnv(c);
    RwImageGlobals *g = imageGlobals();
    g->pathSize = dt_range(0, 20);
    g->path = dt_u32() % 5 == 0 ? NULL : dt_alloc((uint32_t)g->pathSize + 1);
    if (g->path == NULL) {
        g->pathSize = 0;
    }
    gTexOldSize = g->path == NULL ? 0 : (uint32_t)g->pathSize;
    gTexReallocFail = dt_u32() % 8 == 0;
    char *path = dt_alloc(40);
    const int len = dt_range(0, 30);
    for (int i = 0; i < len; i++) {
        path[i] = (char)('a' + dt_range(0, 25));
    }
    DT_ARG_PTR(c, 0, path);
    rwtEnd();
}

static RwImage *BMX_CDECL stFormatRead(const char *name)
{
    (void)name;
    return NULL;
}

static RwImage *BMX_CDECL stFormatWrite(RwImage *image, const char *name)
{
    (void)image;
    (void)name;
    return NULL;
}

/* A registry of 0..3 formats with extensions the new one may or may not match. */
static void setup_register_format(DtCtx *c)
{
    texEnv(c);
    static const char *const exts[] = {"bmp", "PNG", "Tga", "dds", "jpeg_is_too_long_for_it", ""};
    RwImageGlobals *g = imageGlobals();
    for (int n = dt_range(0, 3); n > 0; n--) {
        RwImageFormat *f = dt_alloc(sizeof *f);
        const char *e = exts[dt_u32() % 6];
        f->extension[0] = '.';
        for (int i = 0; i < 0x12 && e[i] != 0; i++) {
            f->extension[i + 1] = (char)(e[i] >= 'A' && e[i] <= 'Z' ? e[i] + 0x20 : e[i]);
        }
        f->read = dt_u32() % 2 ? stFormatRead : NULL;
        f->write = dt_u32() % 2 ? stFormatWrite : NULL;
        f->next = g->formats;
        g->formats = f;
    }
    gTexAllocFailAt = dt_u32() % 6 == 0 ? 1 : 0;
    DT_ARG_PTR(c, 0, mkName(exts[dt_u32() % 6]));
    DT_ARG_PTR(c, 1, dt_u32() % 2 ? (void *)stFormatRead : NULL);
    DT_ARG_PTR(c, 2, dt_u32() % 2 ? (void *)stFormatWrite : NULL);
    rwtEnd();
}

static void setup_pixel_to_rgba(DtCtx *c)
{
    texEnv(c);
    DT_ARG_PTR(c, 0, dt_alloc(8));
    DT_ARG_INT(c, 1, (int32_t)dt_u32());
    DT_ARG_INT(c, 2, dt_range(0, 0x3000));
    rwtEnd();
}

static void setup_gamma(DtCtx *c)
{
    texEnv(c);
    static const float gammas[] = {1.0F, 2.2F, 0.45F, 1.8F, 3.0F, 0.1F};
    DT_ARG_FLOAT(c, 0, dt_u32() % 2 ? gammas[dt_u32() % 6] : dt_float(0.2F, 4.0F));
    rwtEnd();
}

static void setup_set_from_image(DtCtx *c)
{
    texEnv(c);
    RwImage *im = mkImage(0);
    im->flags = dt_u32() & 7;
    DT_ARG_PTR(c, 0, mkRaster());
    DT_ARG_PTR(c, 1, im);
    rwtEnd();
}

DT_CASES(rwtexture_cases) = {
    {"RwRasterCreate", 0x004dcb10, RwRasterCreate, 4, DT_RET_INT, setup_raster_create, 100, 0},
    {"RwRasterDestroy", 0x004dcbd0, RwRasterDestroy, 1, DT_RET_INT, setup_raster, 80, 0},
    {"RwRasterGetWidth", 0x004dcc30, RwRasterGetWidth, 1, DT_RET_INT, setup_raster, 30, 0},
    {"RwRasterGetHeight", 0x004dcc40, RwRasterGetHeight, 1, DT_RET_INT, setup_raster, 30, 0},
    {"RwRasterGetStride", 0x004dcc50, RwRasterGetStride, 1, DT_RET_INT, setup_raster, 30, 0},
    {"RwRasterGetDepth", 0x004dcc60, RwRasterGetDepth, 1, DT_RET_INT, setup_raster, 30, 0},
    {"RwRasterGetFormat", 0x004dcc70, RwRasterGetFormat, 1, DT_RET_INT, setup_raster, 30, 0},
    {"RwRasterSubRaster", 0x004dcc80, RwRasterSubRaster, 3, DT_RET_INT, setup_subraster, 600, 0},
    {"RwRasterGetParent", 0x004dcd70, RwRasterGetParent, 1, DT_RET_INT, setup_raster, 30, 0},
    {"RwRasterPushContext", 0x004dcd80, RwRasterPushContext, 1, DT_RET_INT, setup_push, 200, 0},
    {"RwRasterPopContext", 0x004dce20, RwRasterPopContext, 0, DT_RET_INT, setup_pop, 200, 0},
    {"RwRasterShowRaster", 0x004dcfd0, RwRasterShowRaster, 3, DT_RET_INT, setup_show, 100, 0},
    {"RwRasterLockPalette", 0x004dd160, RwRasterLockPalette, 2, DT_RET_INT, setup_lock_palette, 150, 0},
    {"RwRasterUnlockPalette", 0x004dd1d0, RwRasterUnlockPalette, 1, DT_RET_INT, setup_unlock_palette, 150, 0},
    {"RwTextureSetReadCallBack", 0x004df520, RwTextureSetReadCallBack, 1, DT_RET_INT, setup_callback_set, 30,
     0},
    {"RwTextureGetReadCallBack", 0x004df540, RwTextureGetReadCallBack, 0, DT_RET_INT, setup_callback_get, 30,
     0},
    {"RwTextureSetFindCallBack", 0x004df550, RwTextureSetFindCallBack, 1, DT_RET_INT, setup_callback_set, 30,
     0},
    {"RwTextureGetFindCallBack", 0x004df570, RwTextureGetFindCallBack, 0, DT_RET_INT, setup_callback_get, 30,
     0},
    {"RwTextureSetMipmapping", 0x004df580, RwTextureSetMipmapping, 1, DT_RET_INT, setup_callback_set, 30, 0},
    {"RwTextureSetRaster", 0x004df5f0, RwTextureSetRaster, 2, DT_RET_INT, setup_texture_set_raster, 300, 0},
    {"RwTextureGetRaster", 0x004df650, RwTextureGetRaster, 1, DT_RET_INT, setup_texture, 30, 0},
    {"RwTexDictionaryCreate", 0x004df670, RwTexDictionaryCreate, 0, DT_RET_INT, setup_dict_create, 100, 0},
    {"RwTextureDestroyAll", 0x004df730, RwTextureDestroyAll, 1, DT_RET_INT, setup_texture_destroy_all, 200,
     0},
    {"RwTextureDestroy", 0x004df760, RwTextureDestroy, 1, DT_RET_INT, setup_texture_destroy, 300, 0},
    {"RwTexDictionaryDestroy", 0x004df810, RwTexDictionaryDestroy, 1, DT_RET_INT, setup_dict_destroy, 200, 0},
    {"RwTextureCreate", 0x004df910, RwTextureCreate, 1, DT_RET_INT, setup_texture_create, 100, 0},
    {"RwTextureAddRef", 0x004df980, RwTextureAddRef, 1, DT_RET_INT, setup_texture, 30, 0},
    {"RwTextureGetName", 0x004df990, RwTextureGetName, 1, DT_RET_INT, setup_texture, 30, 0},
    {"RwTextureSetName", 0x004df9b0, RwTextureSetName, 2, DT_RET_INT, setup_set_name, 200, 0},
    {"RwTexDictionaryAddTexture", 0x004dfb20, RwTexDictionaryAddTexture, 2, DT_RET_INT, setup_dict_add, 200,
     0},
    {"RwTexDictionarySetCurrent", 0x004dfc80, RwTexDictionarySetCurrent, 1, DT_RET_INT,
     setup_dict_set_current, 60, 0},
    {"RwTextureRead", 0x004dfcb0, RwTextureRead, 2, DT_RET_INT, setup_texture_read, 600, 0},
    {"RwTextureSetFilterMode", 0x004dff70, RwTextureSetFilterMode, 2, DT_RET_INT, setup_texture_int, 60, 0},
    {"RwTextureSetAddressing", 0x004dffa0, RwTextureSetAddressing, 2, DT_RET_INT, setup_texture_int, 60, 0},
    {"RwImageCreate", 0x004e38e0, RwImageCreate, 3, DT_RET_INT, setup_image_create, 100, 0},
    {"RwImageDestroy", 0x004e3960, RwImageDestroy, 1, DT_RET_INT, setup_image, 150, 0},
    {"RwImageGetWidth", 0x004e3a60, RwImageGetWidth, 1, DT_RET_INT, setup_image, 30, 0},
    {"RwImageGetHeight", 0x004e3a70, RwImageGetHeight, 1, DT_RET_INT, setup_image, 30, 0},
    {"RwImageGetDepth", 0x004e3a80, RwImageGetDepth, 1, DT_RET_INT, setup_image, 30, 0},
    {"RwImageGetPixels", 0x004e3a90, RwImageGetPixels, 1, DT_RET_INT, setup_image, 30, 0},
    {"RwImageGetPalette", 0x004e3aa0, RwImageGetPalette, 1, DT_RET_INT, setup_image, 30, 0},
    {"RwImageAllocatePixels", 0x004e3ab0, RwImageAllocatePixels, 1, DT_RET_INT, setup_image_alloc, 300, 0},
    {"RwImageFreePixels", 0x004e3b90, RwImageFreePixels, 1, DT_RET_INT, setup_image_free, 150, 0},
    {"RwImageSetPath", 0x004e4420, RwImageSetPath, 1, DT_RET_INT, setup_set_path, 300, 0},
    {"RwImageRegisterImageFormat", 0x004e4a30, RwImageRegisterImageFormat, 3, DT_RET_INT,
     setup_register_format, 400, 0},
    {"RwRGBAFromPixel", 0x004e5330, RwRGBAFromPixel, 3, DT_RET_INT, setup_pixel_to_rgba, 60, 0},
    {"RwImageSetGamma", 0x004e5570, RwImageSetGamma, 1, DT_RET_INT, setup_gamma, 40, 0},
    {"RwRasterSetFromImage", 0x004e7f70, RwRasterSetFromImage, 2, DT_RET_INT, setup_set_from_image, 100, 0},
};
