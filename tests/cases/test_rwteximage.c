/*
 * Tests for the large integer functions of rw/texture.c: the three texture readers (0x4e0b10, 0x4e1470,
 * 0x4e1b50), the default mipmap generator (0x4e1140), RwImageApplyMask (0x4e3eb8) and RwImageRead (0x4e4c50).
 *
 * The texture fixture (dt_rwtex.h) gives the engine instance with logging string and memory functions. The
 * functions that call their helpers (the readers, the mipmap generator) run against stubs of all those
 * helpers, which log their arguments and return random but well-formed results, so the order of every call
 * and every failure path is compared with the original. ApplyMask and RwImageRead inline the image helpers in
 * the original; they run on real images and only the engine's allocators are stubs.
 */
#include "dt_rwtex.h"

#define REG1(addr, ret, ta)     ((ret(__attribute__((regparm(1))) *)(ta))(uintptr_t)(addr))
#define REG2(addr, ret, ta, tb) ((ret(__attribute__((regparm(2))) *)(ta, tb))(uintptr_t)(addr))
#define REG3_7(addr, ret, a, b, c)                                                                           \
    ((ret(__attribute__((regparm(3))) *)(a, b, c, int32_t, int32_t, int32_t, int32_t))(uintptr_t)(addr))

/* ---- engine string functions that the texture fixture lacks ----------------------------------------------
 */

static char *BMX_CDECL imgStrcat(char *dst, const char *src)
{
    rwtLog(0xf8);
    rwtLogPtr(dst);
    rwtLogPtr(src);
    char *end = dst;
    while (*end != 0) {
        end++;
    }
    while ((*end++ = *src++) != 0) {
    }
    return dst;
}

static char *BMX_CDECL imgStrrchr(const char *s, int c)
{
    rwtLog(0x100);
    rwtLogPtr(s);
    rwtLog((uint32_t)c);
    const char *last = NULL;
    for (;; s++) {
        if (*s == (char)c) {
            last = s;
        }
        if (*s == 0) {
            break;
        }
    }
    return (char *)(uintptr_t)last;
}

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

static RwGlobals *imgEnv(DtCtx *c)
{
    RwGlobals *g = texEnv(c);
    g->stringFuncs.rwstrcat = (void *)imgStrcat;
    g->stringFuncs.rwstrrchr = (void *)imgStrrchr;
    g->stringFuncs.rwstrchr = (void *)imgStrchr;
    return g;
}

/* ---- fixtures ------------------------------------------------------------------------------------------ */

/* An image of the given depth and size with random pixels (and a palette for depths 4 and 8). */
static RwImage *mkImg(int32_t depth, int32_t w, int32_t h)
{
    RwImage *im = dt_alloc(0x20);
    im->width = w;
    im->height = h;
    im->depth = depth;
    im->stamp = (uint8_t)dt_u32();
    const int32_t bytes = depth > 0 ? ((depth + 7) >> 3) : 0;
    im->stride = ((bytes * w) + 3) & ~3;
    const uint32_t rows = (uint32_t)(im->stride > w * 4 ? im->stride : w * 4) * (uint32_t)(h + 1);
    im->pixels = dt_alloc(rows + 1024);
    /* not dt_fill: the stubs build images during the run, and the harness serves its fill from its own state
     */
    for (uint32_t i = 0; i < (rows + 1024) / 4; i++) {
        ((uint32_t *)im->pixels)[i] = dt_u32();
    }
    im->palette = depth == 4 || depth == 8 ? im->pixels + (uint32_t)im->stride * (uint32_t)h : NULL;
    im->flags = 1;
    return im;
}

static int32_t pickDepth(void)
{
    static const int32_t depths[] = {4, 8, 32, 32, 8, 4};
    return depths[dt_u32() % 6];
}

/* A name from a pool with dots, separators and drive letters, copied into the heap. */
static const char *pickPath(void)
{
    static const char *const pool[] = {"wall.bmp", "dir/wall.BMP", "C:wall.png", "a\\b.c\\tex.tga",
                                       "noext",    "x.",           "A.B.bmp",    "sub\\x.PNG",
                                       "m",        "zz.dds"};
    return mkName(pool[dt_u32() % (sizeof pool / sizeof pool[0])]);
}

/* ======================================================================================================== */
/* RwImageApplyMask (0x4e3eb8)                                                                              */
/* ======================================================================================================== */

static void setup_applymask(DtCtx *c)
{
    imgEnv(c);
    const int32_t w = dt_range(1, 9);
    const int32_t h = dt_range(1, 9);
    static const int32_t imageDepths[] = {4, 8, 32, 32, 16, 0, 4, 8};
    const int32_t depth = imageDepths[dt_u32() % 8];
    RwImage *image = mkImg(depth == 0 ? 32 : depth, w, h);
    image->depth = depth;
    image->flags = dt_u32() % 5 == 0 ? 0 : 1;
    RwImage *mask = mkImg(pickDepth(), w + dt_range(0, 3), h + dt_range(0, 3));
    if (dt_u32() % 6 == 0) {
        mask->depth = dt_u32() % 2 ? 16 : 1;
    }
    /* Only the allocation of the temporary copy may fail: a failed one for the image would crash both. */
    gTexAllocFailAt = dt_u32() % 5 == 0 ? 1 : 0;
    gRwtMallocFailAt = dt_u32() % 5 == 0 ? 1 : 0;
    DT_ARG_PTR(c, 0, image);
    DT_ARG_PTR(c, 1, mask);
    rwtEnd();
}

static RwImage *applymask_orig(RwImage *image, RwImage *mask)
{
    return REG2(0x004e3eb8, RwImage *, RwImage *, RwImage *)(image, mask);
}

static RwImage *applymask_impl(RwImage *image, RwImage *mask)
{
    return RwImageApplyMask(image, mask);
}

/* ======================================================================================================== */
/* RwImageRead (0x4e4c50)                                                                                   */
/* ======================================================================================================== */

static RwImage *gReadImage[2];
static const char *gReadMaskName;

/* The path walk: hands the attempt a prepared image (or none) instead of trying files. */
__attribute__((regparm(3))) static const char *stPathWalk(const char *name, int32_t extra,
                                                          RwImagePathCallback callback, int32_t u0,
                                                          int32_t u1, int32_t u2, void *data)
{
    (void)u0;
    (void)u1;
    (void)u2;
    rwtLog(0x4e44f0);
    rwtLogPtr(name);
    rwtLog((uint32_t)extra);
    rwtLog((uint32_t)(uintptr_t)callback);
    rwtLogPtr(data);
    RwImageAttempt *attempt = data;
    rwtLog((uint32_t)(uintptr_t)attempt->read);
    attempt->image = gReadImage[name == gReadMaskName ? 1 : 0];
    return name;
}

static RwImage *BMX_CDECL stFormatRead(const char *name)
{
    (void)name;
    return NULL;
}

static void setup_read(DtCtx *c)
{
    imgEnv(c);
    dt_stub(0x004e44f0, NULL, (void *)stPathWalk);
    static const char *const kExt[] = {".bmp", ".png", ".tga", ".dds"};
    RwImageGlobals *ig = imageGlobals();
    ig->formats = NULL;
    const int formats = dt_range(0, 4);
    for (int i = 0; i < formats; i++) {
        RwImageFormat *f = dt_alloc(sizeof *f);
        const char *ext = kExt[dt_u32() % 4];
        for (int k = 0; ext[k] != 0; k++) {
            f->extension[k] = ext[k];
            f->extensionUpper[k] = (char)(ext[k] >= 'a' && ext[k] <= 'z' ? ext[k] - 0x20 : ext[k]);
        }
        f->read = dt_u32() % 5 == 0 ? NULL : stFormatRead;
        f->next = ig->formats;
        ig->formats = f;
    }
    const int32_t w = dt_range(1, 8);
    const int32_t h = dt_range(1, 8);
    for (int i = 0; i < 2; i++) {
        gReadImage[i] = dt_u32() % 6 == 0 ? NULL : mkImg(pickDepth(), w, h);
    }
    /* An image that claims to own pixels it lacks reaches the error branch of the destroy (the mask failed).
     */
    if (gReadImage[0] != NULL && gReadImage[1] == NULL && dt_u32() % 2 != 0) {
        gReadImage[0]->pixels = NULL;
        gReadImage[0]->palette = NULL;
    }
    if (gReadImage[1] != NULL && dt_u32() % 6 == 0) {
        gReadImage[1]->depth = 16;
    }
    gTexAllocFailAt = dt_u32() % 6 == 0 ? 1 : 0;
    gRwtMallocFailAt = dt_u32() % 6 == 0 ? 1 : 0;
    DT_ARG_PTR(c, 0, pickPath());
    const uint32_t r = dt_u32() % 5;
    gReadMaskName = r == 0 ? NULL : (r == 1 ? mkName("") : pickPath());
    DT_ARG_PTR(c, 1, gReadMaskName);
    rwtEnd();
}

/* ======================================================================================================== */
/* RwImageRead on a real game file (the BMP reader, the file functions and the stream layer are real)      */
/* ======================================================================================================== */

static void setup_read_real(DtCtx *c)
{
    static const uint32_t kRwFile[10] = {0x4e8290, 0x4a5c98, 0x4a5cab, 0x51db3d, 0x51dc25,
                                         0x51dd2f, 0x51d4d2, 0x51dd86, 0x4a61ad, 0x4a8370};
    RwGlobals *g = imgEnv(c);
    __builtin_memcpy(&g->fileFuncs, kRwFile, sizeof kRwFile);
    RwImageGlobals *ig = imageGlobals();
    RwImageFormat *f = dt_alloc(sizeof *f);
    __builtin_memcpy(f->extension, ".bmp", 5);
    __builtin_memcpy(f->extensionUpper, ".BMP", 5);
    f->read = (RwImageCallBackRead)(uintptr_t)0x004e6d90;
    ig->formats = f;
    static const char *const kNames[] = {"Assets\\shadowmap.bmp", "Assets\\ShadowMap.BMP",
                                         "Assets\\nofile.bmp", "Assets\\shadowmap"};
    DT_ARG_PTR(c, 0, mkName(kNames[dt_u32() % 4]));
    const uint32_t r = dt_u32() % 4;
    DT_ARG_PTR(c, 1, r == 0 ? NULL : mkName(kNames[dt_u32() % 4]));
    rwtEnd();
}

/* ======================================================================================================== */
/* Stubs of the image, raster and device helpers the readers call                                           */
/* ======================================================================================================== */

static int gLockLeft;
static char *gExtA, *gExtB;
static int gGenerateCalls;

static int32_t BMX_CDECL stImgWidth(const RwImage *im)
{
    rwtLog(0x4e3a60);
    rwtLogPtr(im);
    return im->width;
}

static int32_t BMX_CDECL stImgHeight(const RwImage *im)
{
    rwtLog(0x4e3a70);
    rwtLogPtr(im);
    return im->height;
}

static int32_t BMX_CDECL stImgDepth(const RwImage *im)
{
    rwtLog(0x4e3a80);
    rwtLogPtr(im);
    return im->depth;
}

static uint8_t *BMX_CDECL stImgPixels(const RwImage *im)
{
    rwtLog(0x4e3a90);
    rwtLogPtr(im);
    return im->pixels;
}

static uint8_t *BMX_CDECL stImgPalette(const RwImage *im)
{
    rwtLog(0x4e3aa0);
    rwtLogPtr(im);
    return im->palette;
}

static RwImage *BMX_CDECL stImgCreate(int32_t w, int32_t h, int32_t d)
{
    rwtLog(0x4e38e0);
    rwtLog((uint32_t)w);
    rwtLog((uint32_t)h);
    rwtLog((uint32_t)d);
    if (dt_u32() % 7 == 0) {
        return NULL;
    }
    RwImage *im = dt_alloc(0x20);
    im->width = w;
    im->height = h;
    im->depth = d;
    im->stride = 16;
    im->pixels = dt_alloc(8);
    im->palette = dt_alloc(8);
    return im;
}

static RwImage *BMX_CDECL stImgAlloc(RwImage *im)
{
    rwtLog(0x4e3ab0);
    rwtLogPtr(im);
    return dt_u32() % 7 == 0 ? NULL : im;
}

static int BMX_CDECL stImgDestroy(RwImage *im)
{
    rwtLog(0x4e3960);
    rwtLogPtr(im);
    return 1;
}

static RwImage *BMX_CDECL stImgResize(RwImage *dest, RwImage *source)
{
    rwtLog(0x4e3c00);
    rwtLogPtr(dest);
    rwtLogPtr(source);
    return dest;
}

static RwImage *BMX_CDECL stImgGamma(RwImage *im)
{
    rwtLog(0x4e53c0);
    rwtLogPtr(im);
    return im;
}

static const char *BMX_CDECL stFindFileType(const char *name)
{
    rwtLog(0x4e4a00);
    rwtLogPtr(name);
    const uint32_t r = dt_u32() % 3;
    return r == 0 ? NULL : (r == 1 ? gExtA : gExtB);
}

/* Reads "an image file": random depth and a size from a small set. */
static RwImage *BMX_CDECL stImgRead(const char *name, const char *mask)
{
    rwtLog(0x4e4c50);
    rwtLogPtr(name);
    rwtLogPtr(mask);
    if (dt_u32() % 6 == 0) {
        return NULL;
    }
    static const int32_t sizes[] = {4, 8, 16};
    RwImage *im = mkImg(pickDepth(), sizes[dt_u32() % 3], sizes[dt_u32() % 3]);
    return im;
}

/* The quantiser and resampler (never ported). */
static void BMX_CDECL stQuantResample(RwImage *dest, RwImage *source)
{
    rwtLog(0x4f25d0);
    rwtLogPtr(dest);
    rwtLogPtr(source);
}

static int BMX_CDECL stQuantOpen(void *state)
{
    rwtLog(0x4f2070);
    rwtLogPtr(state);
    return dt_u32() % 6 != 0;
}

static void BMX_CDECL stQuantHistogram(void *state, RwImage *image)
{
    rwtLog(0x4ef4e0);
    rwtLogPtr(state);
    rwtLogPtr(image);
}

static void BMX_CDECL stQuantPalette(uint8_t *palette, int32_t count, void *state)
{
    rwtLog(0x4f0da0);
    rwtLogPtr(palette);
    rwtLog((uint32_t)count);
    rwtLogPtr(state);
}

static void BMX_CDECL stQuantMap(uint8_t *pixels, int32_t stride, int32_t depth, void *state, RwImage *image)
{
    rwtLog(0x4f19e0);
    rwtLogPtr(pixels);
    rwtLog((uint32_t)stride);
    rwtLog((uint32_t)depth);
    rwtLogPtr(state);
    rwtLogPtr(image);
}

static void BMX_CDECL stQuantClose(void *state)
{
    rwtLog(0x4f1f90);
    rwtLogPtr(state);
}

static void stubQuantizer(void)
{
    dt_stub(0x004f25d0, NULL, (void *)stQuantResample);
    dt_stub(0x004f2070, NULL, (void *)stQuantOpen);
    dt_stub(0x004ef4e0, NULL, (void *)stQuantHistogram);
    dt_stub(0x004f0da0, NULL, (void *)stQuantPalette);
    dt_stub(0x004f19e0, NULL, (void *)stQuantMap);
    dt_stub(0x004f1f90, NULL, (void *)stQuantClose);
}

static void stubImages(void)
{
    dt_stub(0x004e3a60, NULL, (void *)stImgWidth);
    dt_stub(0x004e3a70, NULL, (void *)stImgHeight);
    dt_stub(0x004e3a80, NULL, (void *)stImgDepth);
    dt_stub(0x004e3a90, NULL, (void *)stImgPixels);
    dt_stub(0x004e3aa0, NULL, (void *)stImgPalette);
    dt_stub(0x004e38e0, NULL, (void *)stImgCreate);
    dt_stub(0x004e3ab0, NULL, (void *)stImgAlloc);
    dt_stub(0x004e3960, NULL, (void *)stImgDestroy);
    dt_stub(0x004e3c00, NULL, (void *)stImgResize);
    dt_stub(0x004e53c0, NULL, (void *)stImgGamma);
    stubQuantizer();
}

static void stubReader(void)
{
    stubImages();
    dt_stub(0x004e4a00, NULL, (void *)stFindFileType);
    dt_stub(0x004e4c50, NULL, (void *)stImgRead);
    gExtA = mkName(".bmp");
    gExtB = mkName(".PNG");
}

/* ---- rasters ---------------------------------------------------------------------------------------------
 */

static int32_t BMX_CDECL stRasterWidth(const RwRaster *r)
{
    rwtLog(0x4dcc30);
    rwtLogPtr(r);
    return r->width;
}

static int32_t BMX_CDECL stRasterHeight(const RwRaster *r)
{
    rwtLog(0x4dcc40);
    rwtLogPtr(r);
    return r->height;
}

static int32_t BMX_CDECL stRasterDepth(const RwRaster *r)
{
    rwtLog(0x4dcc60);
    rwtLogPtr(r);
    return r->depth;
}

static RwRaster *BMX_CDECL stRasterCreate(int32_t w, int32_t h, int32_t d, int32_t flags)
{
    rwtLog(0x4dcb10);
    rwtLog((uint32_t)w);
    rwtLog((uint32_t)h);
    rwtLog((uint32_t)d);
    rwtLog((uint32_t)flags);
    if (dt_u32() % 8 == 0) {
        return NULL;
    }
    RwRaster *r = dt_alloc(0x30);
    r->width = w;
    r->height = h;
    r->depth = d;
    return r;
}

static int BMX_CDECL stRasterDestroy(RwRaster *r)
{
    rwtLog(0x4dcbd0);
    rwtLogPtr(r);
    return 1;
}

static RwRaster *BMX_CDECL stRasterSetFromImage(RwRaster *r, RwImage *im)
{
    rwtLog(0x4e7f70);
    rwtLogPtr(r);
    rwtLogPtr(im);
    return dt_u32() % 7 == 0 ? NULL : r;
}

/* The device refuses some formats, and picks sizes that sometimes match the image's. */
static RwImage *BMX_CDECL stFindRasterFormat(RwImage *im, int32_t flags, int32_t *w, int32_t *h, int32_t *d,
                                             uint32_t *fmt)
{
    rwtLog(0x4e7fc0);
    rwtLogPtr(im);
    rwtLog((uint32_t)flags);
    if (dt_u32() % 8 == 0) {
        return NULL;
    }
    static const int32_t sizes[] = {4, 8, 16};
    *w = dt_u32() % 2 ? im->width : sizes[dt_u32() % 3];
    *h = dt_u32() % 2 ? im->height : sizes[dt_u32() % 3];
    *d = pickDepth();
    uint32_t f = dt_u32() & 0xff;
    switch (dt_u32() % 4) {
    case 0:
        f |= 0x8000;
        break;
    case 1:
        f |= 0x8000 | 0x1000;
        break;
    case 2:
        f |= 0x1000;
        break;
    default:
        break;
    }
    *fmt = f;
    rwtLog(*w == im->width);
    rwtLog(*h == im->height);
    return im;
}

static uint8_t *BMX_CDECL stRasterLock(RwRaster *r, uint32_t level, uint32_t mode)
{
    rwtLog(0x4dd010);
    rwtLogPtr(r);
    rwtLog(level & 0xff);
    rwtLog(mode);
    return gLockLeft-- > 0 ? dt_alloc(8) : NULL;
}

static RwRaster *BMX_CDECL stRasterUnlock(RwRaster *r)
{
    rwtLog(0x4dd100);
    rwtLogPtr(r);
    return r;
}

static void stubRasters(void)
{
    dt_stub(0x004dcc30, NULL, (void *)stRasterWidth);
    dt_stub(0x004dcc40, NULL, (void *)stRasterHeight);
    dt_stub(0x004dcc60, NULL, (void *)stRasterDepth);
    dt_stub(0x004dcb10, NULL, (void *)stRasterCreate);
    dt_stub(0x004dcbd0, NULL, (void *)stRasterDestroy);
    dt_stub(0x004e7f70, NULL, (void *)stRasterSetFromImage);
    dt_stub(0x004e7fc0, NULL, (void *)stFindRasterFormat);
    dt_stub(0x004dd010, NULL, (void *)stRasterLock);
    dt_stub(0x004dd100, NULL, (void *)stRasterUnlock);
}

/* ======================================================================================================== */
/* The texture readers                                                                                      */
/* ======================================================================================================== */

/* The mipmap generator the mipmapped reader calls through the texture globals. */
static void BMX_CDECL stGenerator(RwRaster *raster, RwImage *image)
{
    rwtLog(0x4e1140);
    rwtLogPtr(raster);
    rwtLogPtr(image);
    gGenerateCalls++;
}

/* The state the readers share: name buffer of the texture module (none, small, huge or of a negative size).
 */
static void readerGlobals(void)
{
    RwTextureGlobals *tg = texGlobals();
    tg->findCallBack = dt_u32() % 2 ? (RwTextureCallBackFind)(uintptr_t)stImgRead : NULL;
    tg->mipmapping = dt_u32() % 2;
    tg->defaultFind = dt_u32() % 5 == 0 ? NULL : (void *)stGenerator;
    switch (dt_u32() % 4) {
    case 0:
        tg->field20 = NULL;
        tg->field24 = 0;
        break;
    case 1:
        tg->field20 = dt_alloc(512);
        tg->field24 = (uint16_t)dt_range(0, 60);
        break;
    case 2:
        tg->field20 = dt_alloc(512);
        tg->field24 = (uint16_t)dt_range(100, 400);
        break;
    default:
        tg->field20 = dt_alloc(512);
        tg->field24 = (uint16_t)(0x8000 + dt_range(0, 400)); /* a negative 16 bit size */
        break;
    }
    gTexOldSize = 64;
    gGenerateCalls = 0;
    gTexAllocFailAt = dt_u32() % 8 == 0 ? 1 : 0;
    gRwtMallocFailAt = dt_u32() % 5 == 0 ? dt_range(1, 8) : 0;
    gTexReallocFail = dt_u32() % 10 == 0;
}

static const char *pickMask(void)
{
    const uint32_t r = dt_u32() % 5;
    return r == 0 ? NULL : (r == 1 ? mkName("") : pickPath());
}

static void setup_readimage(DtCtx *c)
{
    imgEnv(c);
    stubReader();
    readerGlobals();
    static const int32_t dims[] = {0, -1, 4, 8, 16, 4, 8};
    DT_ARG_PTR(c, 0, pickPath());
    DT_ARG_PTR(c, 1, pickMask());
    DT_ARG_INT(c, 2, dims[dt_u32() % 7]);
    DT_ARG_INT(c, 3, dims[dt_u32() % 7]);
    rwtEnd();
}

static RwImage *readimage_orig(const char *name, const char *mask, int32_t width, int32_t height)
{
    return REG3_7(0x004e0b10, RwImage *, const char *, const char *, int32_t)(name, mask, width, 0, 0, 0,
                                                                              height);
}

static RwImage *readimage_impl(const char *name, const char *mask, int32_t width, int32_t height)
{
    return rwTextureReadImage(name, mask, width, 0, 0, 0, height);
}

/* The reader that the plain and the mipmapped reader call (0x4e0b10) is stubbed for the plain one. */
__attribute__((regparm(3))) static RwImage *stReadImage(const char *name, const char *mask, int32_t width,
                                                        int32_t u0, int32_t u1, int32_t u2, int32_t height)
{
    (void)u0;
    (void)u1;
    (void)u2;
    rwtLog(0x4e0b10);
    rwtLogPtr(name);
    rwtLogPtr(mask);
    rwtLog((uint32_t)width);
    rwtLog((uint32_t)height);
    if (dt_u32() % 7 == 0) {
        return NULL;
    }
    static const int32_t sizes[] = {4, 8, 16};
    return mkImg(pickDepth(), width > 0 ? width : sizes[dt_u32() % 3],
                 height > 0 ? height : sizes[dt_u32() % 3]);
}

static void setup_plain(DtCtx *c)
{
    imgEnv(c);
    stubImages();
    stubRasters();
    dt_stub(0x004e0b10, NULL, (void *)stReadImage);
    readerGlobals();
    gLockLeft = dt_range(0, 4);
    DT_ARG_PTR(c, 0, pickPath());
    DT_ARG_PTR(c, 1, pickMask());
    rwtEnd();
}

static void setup_mipmapped(DtCtx *c)
{
    imgEnv(c);
    stubReader();
    stubRasters();
    readerGlobals();
    DT_ARG_PTR(c, 0, pickPath());
    DT_ARG_PTR(c, 1, pickMask());
    rwtEnd();
}

/* ======================================================================================================== */
/* The mipmap generator (0x4e1140)                                                                          */
/* ======================================================================================================== */

static RwImage *BMX_CDECL stSetFromRaster(RwImage *im, RwRaster *r)
{
    rwtLog(0x4e7f20);
    rwtLogPtr(im);
    rwtLogPtr(r);
    return dt_u32() % 7 != 0 ? im : NULL;
}

__attribute__((regparm(2))) static int stBoxShrink(const RwImage *source, RwImage *dest)
{
    rwtLog(0x4e0f10);
    rwtLogPtr(source);
    rwtLogPtr(dest);
    return 1;
}

__attribute__((regparm(2))) static void stMipmapPaletted(RwImage *image, RwRaster *raster)
{
    rwtLog(0x4e0160);
    rwtLogPtr(image);
    rwtLogPtr(raster);
}

static void setup_generate(DtCtx *c)
{
    imgEnv(c);
    stubImages();
    stubRasters();
    dt_stub(0x004e7f20, NULL, (void *)stSetFromRaster);
    dt_stub(0x004e0f10, NULL, (void *)stBoxShrink);
    dt_stub(0x004e0160, NULL, (void *)stMipmapPaletted);
    gLockLeft = dt_range(0, 4);
    RwRaster *r = mkRaster();
    r->format = (uint8_t)(dt_u32() % 5 == 0 ? dt_u32() & 0x7f : (0x80 | (dt_u32() & 0x7f)));
    r->width = dt_range(1, 20);
    r->height = dt_range(1, 20);
    RwImage *image = NULL;
    if (dt_u32() % 2 != 0) {
        const int32_t depth = dt_u32() % 4 == 0 ? 8 : 32;
        image = mkImg(depth, 8, 8);
    }
    DT_ARG_PTR(c, 0, r);
    DT_ARG_PTR(c, 1, image);
    rwtEnd();
}

DT_CASES(rwteximage_cases) = {
    {"RwImageApplyMask", (uint32_t)(uintptr_t)applymask_orig, applymask_impl, 2, DT_RET_INT, setup_applymask,
     1500, 0},
    {"RwImageRead", 0x004e4c50, RwImageRead, 2, DT_RET_INT, setup_read, 1500, 0},
    {"RwImageRead_realfile", 0x004e4c50, RwImageRead, 2, DT_RET_INT | DT_NEEDS_ASSETS, setup_read_real, 60,
     0},
    {"rwTextureReadImage", (uint32_t)(uintptr_t)readimage_orig, readimage_impl, 4, DT_RET_INT,
     setup_readimage, 2000, 0},
    {"rwTextureReadPlain", 0x004e1470, rwTextureReadPlain, 2, DT_RET_INT, setup_plain, 2000, 0},
    {"rwTextureReadMipmapped", 0x004e1b50, rwTextureReadMipmapped, 2, DT_RET_INT, setup_mipmapped, 2000, 0},
    {"rwRasterGenerateMipmaps", 0x004e1140, rwRasterGenerateMipmaps, 2, DT_RET_INT, setup_generate, 2000, 0},
};
