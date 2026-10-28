/*
 * Tests for the image file helpers of rw/texture.c: the walk over the image search path with its two
 * callbacks, the format lookup, the attempt callback, the pixel copy between images and the gamma correction.
 * The engine is the texture fixture (dt_rwtex.h) with a strchr, a file-exists function and callbacks that log
 * the path they are handed. ImagePathForAllFullNames and the pixel copy take register arguments (wrappers).
 */
#include "dt_rwtex.h"

static int gCbCalls, gCbStopAt;
static int gExistCalls;
static uint32_t gExistMask;

static char *BMX_CDECL imgStrchr(const char *s, int c)
{
    rwtLog(0x104);
    rwtLogPtr(s);
    rwtLog((uint32_t)c);
    for (; *s != 0; s++) {
        if (*s == (char)c) {
            return (char *)(uintptr_t)s;
        }
    }
    return NULL;
}

static void imgLogString(const char *s)
{
    for (uint32_t n = 0; n < 64; n++) {
        rwtLog((uint8_t)s[n]);
        if (s[n] == 0) {
            break;
        }
    }
}

static int BMX_CDECL imgExist(const char *name)
{
    rwtLog(0xc0);
    rwtLogPtr(name);
    imgLogString(name);
    return (int)((gExistMask >> (gExistCalls++ & 31)) & 1);
}

/* The walk's callback: logs the path, asks for another directory until the n-th call. */
static const char *BMX_CDECL imgPathCb(char *path, void *data)
{
    rwtLog(0xc1);
    rwtLogPtr(path);
    imgLogString(path);
    rwtLogPtr(data);
    gCbCalls++;
    return gCbStopAt != 0 && gCbCalls >= gCbStopAt ? NULL : path;
}

static RwImage *gReadResult;

static RwImage *BMX_CDECL imgRead(const char *name)
{
    rwtLog(0xc2);
    rwtLogPtr(name);
    imgLogString(name);
    return gReadResult;
}

static void imgEnv(DtCtx *c)
{
    RwGlobals *g = texEnv(c);
    g->stringFuncs.rwstrchr = (void *)imgStrchr;
    g->fileFuncs.rwfexist = imgExist;
    gCbCalls = 0;
    gCbStopAt = dt_u32() % 3 == 0 ? 0 : dt_range(1, 3);
    gExistCalls = 0;
    gExistMask = dt_u32();
}

static const char *imgPool(const char *const *pool, int n)
{
    const char *s = pool[dt_u32() % (uint32_t)n];
    char *copy = dt_alloc((uint32_t)__builtin_strlen(s) + 1);
    __builtin_memcpy(copy, s, __builtin_strlen(s));
    return copy;
}

/* The search path, the name buffer and the formats of the image module. */
static void imgModule(void)
{
    static const char *const paths[] = {"",   "dir", "a;b",           "textures;extra;more",
                                        "x;", ";y",  "c:\\game\\tex", ";;"};
    static const char *const exts[] = {".bmp", ".png", ".dds", ".tga"};
    RwImageGlobals *images = imageGlobals();
    images->path = dt_u32() % 6 == 0 ? NULL : (char *)(uintptr_t)imgPool(paths, 8);
    images->pathSize = 256;
    if (dt_u32() % 2) {
        images->nameBufferSize = dt_range(0, 40);
        images->nameBuffer = dt_alloc((uint32_t)images->nameBufferSize + 4);
        gTexOldSize = (uint32_t)images->nameBufferSize;
    } else {
        images->nameBufferSize = dt_u32() % 4 == 0 ? dt_range(0, 5) : 0;
        images->nameBuffer = NULL;
    }
    images->formats = NULL;
    for (int n = dt_range(0, 3); n > 0; n--) {
        RwImageFormat *format = dt_alloc(sizeof *format);
        const char *ext = exts[dt_u32() % 4];
        __builtin_memcpy(format->extension, ext, 5);
        for (int i = 0; i < 5; i++) {
            format->extensionUpper[i] = (char)(ext[i] >= 'a' && ext[i] <= 'z' ? ext[i] - 0x20 : ext[i]);
        }
        format->next = images->formats;
        images->formats = format;
    }
    gRwtMallocFailAt = dt_u32() % 6 == 0 ? 1 : 0;
    gTexReallocFail = dt_u32() % 8 == 0;
}

static const char *imgName(void)
{
    static const char *const names[] = {"tex", "wall", "C:\\abs\\file", "\\root\\file", "sub\\dir\\name",
                                        "x",   ""};
    return imgPool(names, 7);
}

/* ---- ImagePathForAllFullNames (EAX, EDX, ECX, data = fourth stack word) ----------------------------- */

typedef const char *(__attribute__((regparm(3))) * PathWalkFn)(const char *, int32_t, RwImagePathCallback,
                                                               int32_t, int32_t, int32_t, void *);

static const char *walk_orig(const char *name, int32_t extra, RwImagePathCallback cb, void *data)
{
    return ((PathWalkFn)(uintptr_t)0x004e44f0)(name, extra, cb, 0, 0, 0, data);
}

static const char *walk_impl(const char *name, int32_t extra, RwImagePathCallback cb, void *data)
{
    return ImagePathForAllFullNames(name, extra, cb, 0, 0, 0, data);
}

static void setup_walk(DtCtx *c)
{
    imgEnv(c);
    imgModule();
    DT_ARG_PTR(c, 0, imgName());
    DT_ARG_INT(c, 1, dt_u32() % 4 == 0 ? dt_range(0, 100) : dt_range(0, 0x14));
    DT_ARG_PTR(c, 2, imgPathCb);
    DT_ARG_PTR(c, 3, dt_alloc(8));
    rwtEnd();
}

/* ---- the callbacks and the format lookup -------------------------------------------------------------- */

static void setup_format_cb(DtCtx *c)
{
    imgEnv(c);
    imgModule();
    char *path = dt_alloc(0x80);
    __builtin_memcpy(path, "dir\\file", 9);
    DT_ARG_PTR(c, 0, path);
    DT_ARG_PTR(c, 1, dt_alloc(8));
    rwtEnd();
}

static void setup_find_file_type(DtCtx *c)
{
    imgEnv(c);
    imgModule();
    DT_ARG_PTR(c, 0, imgName());
    rwtEnd();
}

static void setup_attempt(DtCtx *c)
{
    imgEnv(c);
    RwImageAttempt *attempt = dt_alloc(sizeof *attempt);
    attempt->read = imgRead;
    attempt->image = (RwImage *)(uintptr_t)dt_u32();
    gReadResult = dt_u32() % 2 ? dt_alloc(8) : NULL;
    DT_ARG_PTR(c, 0, imgName());
    DT_ARG_PTR(c, 1, attempt);
    rwtEnd();
}

/* ---- pixel copy and gamma -------------------------------------------------------------------------------
 */

/* An image of up to 5 x 4 pixels of the depth, with random pixels, a palette (or none) and a stamp. */
static RwImage *imgImage(int32_t depth, int32_t width, int32_t height)
{
    RwImage *image = dt_alloc(sizeof *image);
    image->depth = depth;
    image->width = width;
    image->height = height;
    image->stride = width * (depth == 32 ? 4 : 1) + dt_range(0, 3);
    const int32_t rows = height > 0 ? height : 0;
    const uint32_t size = (uint32_t)(image->stride * rows) + 16;
    image->pixels = dt_u32() % 12 == 0 ? NULL : dt_alloc(size);
    if (image->pixels != NULL) {
        dt_fill(image->pixels, size);
    }
    image->palette = dt_u32() % 6 == 0 ? NULL : dt_alloc(1024);
    if (image->palette != NULL) {
        dt_fill(image->palette, 1024);
    }
    image->flags = dt_u32() & 3;
    image->stamp = (uint8_t)dt_u32();
    return image;
}

static void setup_copy(DtCtx *c)
{
    imgEnv(c);
    static const int32_t depths[] = {4, 8, 32};
    const int32_t width = dt_range(0, 5);
    const int32_t height = dt_u32() % 10 == 0 ? dt_range(-1, 0) : dt_range(0, 4);
    const int32_t dd = depths[dt_u32() % 3];
    const int32_t sd = dt_u32() % 3 == 0 ? dd : depths[dt_u32() % 3];
    RwImage *dest = imgImage(dt_u32() % 40 == 0 ? 16 : dd, width, height);
    RwImage *source = imgImage(sd, width, height);
    if (dest->depth == 32 && source->depth != 32 && source->palette == NULL) {
        source->palette = dt_alloc(1024); /* the original reads a palette entry per pixel */
        dt_fill(source->palette, 1024);
    }
    if (dest->pixels == NULL || source->pixels == NULL) {
        dest->pixels = dt_alloc(256);
        source->pixels = dt_alloc(256);
        dt_fill(source->pixels, 256);
    }
    DT_ARG_PTR(c, 0, dest);
    DT_ARG_PTR(c, 1, source);
    rwtEnd();
}

static RwImage *copy_orig(RwImage *dest, RwImage *source)
{
    RwImage *result;
    __asm__ volatile("call *%3"
                     : "=a"(result), "+d"(source)
                     : "0"(dest), "r"(0x004e3c08)
                     : "memory", "cc", "ecx", "st", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6",
                       "xmm7");
    return result;
}

static RwImage *copy_impl(RwImage *dest, RwImage *source)
{
    RwImage *result;
    __asm__ volatile("call *%3"
                     : "=a"(result), "+d"(source)
                     : "0"(dest), "r"((uint32_t)(uintptr_t)rwImageCopyPixels)
                     : "memory", "cc", "ecx", "st", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6",
                       "xmm7");
    return result;
}

/* EAX = source, EDX = dest; the result comes back in EAX. */
static int shrink_call(uint32_t target, RwImage *source, RwImage *dest)
{
    int result;
    __asm__ volatile("call *%3"
                     : "=a"(result), "+d"(dest)
                     : "0"(source), "r"(target)
                     : "memory", "cc", "ecx", "st", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6",
                       "xmm7");
    return result;
}

static int shrink_orig(RwImage *source, RwImage *dest)
{
    return shrink_call(0x004e0f10, source, dest);
}

static int shrink_impl(RwImage *source, RwImage *dest)
{
    return shrink_call((uint32_t)(uintptr_t)rwImageBoxShrink, source, dest);
}

/*
 * A source that is a whole multiple of the destination (1..4 pixels, factors 1..3); now and then sizes that
 * do not divide or a depth other than 32. Sizes are never 0 (the original divides by them).
 */
static void setup_shrink(DtCtx *c)
{
    imgEnv(c);
    RwImage *dest = imgImage(32, dt_range(1, 4), dt_range(1, 4));
    int32_t factorW = dt_range(1, 3), factorH = dt_range(1, 3);
    RwImage *source = imgImage(32, dest->width * factorW, dest->height * factorH);
    if (dt_u32() % 8 == 0) {
        source->width += 1;
    }
    if (dt_u32() % 8 == 0) {
        source->height += 1;
    }
    if (dt_u32() % 12 == 0) {
        source->depth = 8;
    }
    if (dt_u32() % 12 == 0) {
        dest->depth = 16;
    }
    /* the pixel blocks follow the (possibly changed) sizes: make them big enough for the walk */
    source->stride = source->width * 4 + dt_range(0, 3);
    source->pixels = dt_alloc((uint32_t)(source->stride * source->height) + 64);
    dt_fill(source->pixels, (uint32_t)(source->stride * source->height) + 64);
    if (dest->pixels == NULL) {
        dest->pixels = dt_alloc((uint32_t)(dest->stride * dest->height) + 64);
    }
    DT_ARG_PTR(c, 0, source);
    DT_ARG_PTR(c, 1, dest);
    rwtEnd();
}

static void setup_gamma(DtCtx *c)
{
    imgEnv(c);
    dt_fill(imageGlobals()->gammaUp, 0x100);
    static const int32_t depths[] = {4, 8, 32, 32, 16};
    RwImage *image =
        imgImage(depths[dt_u32() % 5], dt_range(0, 5), dt_u32() % 10 == 0 ? dt_range(-1, 0) : dt_range(0, 4));
    if (image->depth == 32 && image->pixels != NULL && dt_u32() % 2) {
        image->stride = image->width * 4 + dt_range(0, 3);
    }
    DT_ARG_PTR(c, 0, image);
    rwtEnd();
}

DT_CASES(rwimagefile_cases) = {
    {"ImagePathForAllFullNames", (uint32_t)(uintptr_t)walk_orig, walk_impl, 4, DT_RET_INT, setup_walk, 1200,
     0},
    {"rwImageFindFormatCallback", 0x004e4950, rwImageFindFormatCallback, 2, DT_RET_INT, setup_format_cb, 300,
     0},
    {"RwImageFindFileType", 0x004e4a00, RwImageFindFileType, 1, DT_RET_INT, setup_find_file_type, 800, 0},
    {"ImageAttempRead", 0x004e47e0, ImageAttempRead, 2, DT_RET_INT, setup_attempt, 100, 0},
    {"rwImageCopyPixels", (uint32_t)(uintptr_t)copy_orig, copy_impl, 2, DT_RET_INT, setup_copy, 800, 0},
    {"RwImageResize", 0x004e3c00, RwImageResize, 2, DT_RET_INT, setup_copy, 400, 0},
    {"rwImageBoxShrink", (uint32_t)(uintptr_t)shrink_orig, shrink_impl, 2, DT_RET_INT, setup_shrink, 1000, 0},
    {"RwImageGammaCorrect", 0x004e53c0, RwImageGammaCorrect, 1, DT_RET_INT, setup_gamma, 600, 0},
};
