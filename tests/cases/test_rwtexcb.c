/*
 * Tests for the texture module's default read callback and the image/raster glue (rw/texture.c, 0x4e1410,
 * 0x4e7f20, 0x4e7fc0, 0x4f2ec0, 0x4e003c). The two readers behind the callback and the device's standard
 * functions are logging stubs.
 */
#include "dt_rwtex.h"

static void *BMX_CDECL stReadMipmapped(const char *name, const char *maskName)
{
    rwtLog(0x4e1b50);
    rwtLogPtr(name);
    rwtLogPtr(maskName);
    return dt_alloc(8);
}

static void *BMX_CDECL stReadPlain(const char *name, const char *maskName)
{
    rwtLog(0x4e1470);
    rwtLogPtr(name);
    rwtLogPtr(maskName);
    return dt_alloc(8);
}

static void setup_default_read(DtCtx *c)
{
    texEnv(c);
    dt_stub(0x004e1b50, NULL, (void *)stReadMipmapped);
    dt_stub(0x004e1470, NULL, (void *)stReadPlain);
    RwTextureGlobals *tg = texGlobals();
    tg->findCallBack = dt_u32() % 2 ? NULL : (RwTextureCallBackFind)(uintptr_t)(dt_u32() | 1U);
    tg->mipmapping = dt_u32() % 2 ? 0 : (int32_t)dt_u32();
    DT_ARG_PTR(c, 0, mkName(poolName()));
    DT_ARG_PTR(c, 1, dt_u32() % 2 ? NULL : mkName(poolName()));
    rwtEnd();
}

static int BMX_CDECL stStdRasterToImage(RwImage *image, RwRaster *raster, int32_t flags)
{
    rwtLog(0x1006);
    rwtLogPtr(image);
    rwtLogPtr(raster);
    rwtLog((uint32_t)flags);
    return dt_u32() % 4 != 0;
}

static void setup_set_from_raster(DtCtx *c)
{
    RwGlobals *g = texEnv(c);
    g->stdFunc[6] = (uint32_t)(uintptr_t)stStdRasterToImage;
    RwImage *image = mkImage(1);
    image->flags = dt_u32();
    DT_ARG_PTR(c, 0, image);
    DT_ARG_PTR(c, 1, mkRaster());
    rwtEnd();
}

static int BMX_CDECL stStdFindFormat(RwRaster *info, RwImage *image, int32_t flags)
{
    rwtLog(0x1009);
    rwtLogPtr(image);
    rwtLog((uint32_t)flags);
    uint32_t *words = (uint32_t *)info;
    for (int i = 0; i < 10; i++) {
        words[i] = dt_u32();
    }
    info->width = dt_range(0, 512);
    info->height = dt_range(0, 512);
    info->depth = dt_range(0, 32);
    return dt_u32() % 4 != 0;
}

static void setup_find_format(DtCtx *c)
{
    RwGlobals *g = texEnv(c);
    g->stdFunc[9] = (uint32_t)(uintptr_t)stStdFindFormat;
    DT_ARG_PTR(c, 0, mkImage(1));
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_PTR(c, 2, dt_alloc(4));
    DT_ARG_PTR(c, 3, dt_alloc(4));
    DT_ARG_PTR(c, 4, dt_alloc(4));
    DT_ARG_PTR(c, 5, dt_alloc(4));
    rwtEnd();
}

static void setup_path(DtCtx *c)
{
    texEnv(c);
    static const char *const paths[] = {
        "C:\\\\dir\\\\file", "\\\\share", "dir\\\\file", "z:", "Z", "a:b", "9:", ":", "", "/usr", "[:", "{:"};
    char *path = mkName(paths[dt_u32() % (sizeof paths / sizeof paths[0])]);
    if (dt_u32() % 4 == 0) {
        path = dt_alloc(4);
        path[0] = (char)dt_u32();
        path[1] = dt_u32() % 2 ? ':' : (char)dt_u32();
    }
    DT_ARG_PTR(c, 0, path);
}

static void setup_no_args(DtCtx *c)
{
    texEnv(c);
}

DT_CASES(rwtexcb_cases) = {
    {"rwTextureDefaultRead", 0x004e1410, rwTextureDefaultRead, 2, DT_RET_INT, setup_default_read, 60, 0},
    {"RwImageSetFromRaster", 0x004e7f20, RwImageSetFromRaster, 2, DT_RET_INT, setup_set_from_raster, 100, 0},
    {"RwImageFindRasterFormat", 0x004e7fc0, RwImageFindRasterFormat, 6, DT_RET_INT, setup_find_format, 100,
     0},
    {"_rwpathisabsolute", 0x004f2ec0, _rwpathisabsolute, 1, DT_RET_INT, setup_path, 200, 0},
    {"rwImageFormatSucceed", 0x004e003c, rwImageFormatSucceed, 0, DT_RET_INT, setup_no_args, 3, 0},
};
