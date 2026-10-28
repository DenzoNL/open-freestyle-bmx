/*
 * Tests for the BMP writer of rw/texture.c: the header size helpers (EAX register arguments), the row writer
 * and the file writer (EAX, EDX, ECX and stack arguments, called through wrappers), and BMPImageWrite on a
 * file stream. The streams are custom ones (or the file table of the engine) whose callbacks log every byte
 * they get. Images have depth 32, 8 or 4; other depths make the original read address 0 (a fault), so they
 * are not generated.
 */
#include "dt_rwtex.h"

static int gWriteCalls, gWriteFailAt;
static int gFopenOk;

static void bmpLogBytes(const void *buffer, uint32_t length)
{
    rwtLog(length);
    for (uint32_t i = 0; i < length && i < 64; i++) {
        rwtLog(((const uint8_t *)buffer)[i]);
    }
}

static int BMX_CDECL bmpCustWrite(void *data, const void *buffer, uint32_t length)
{
    rwtLog(0xc3);
    rwtLogPtr(data);
    bmpLogBytes(buffer, length);
    gWriteCalls++;
    return gWriteFailAt != 0 && gWriteCalls == gWriteFailAt ? 0 : 1;
}

static void *BMX_CDECL bmpFopen(const char *name, const char *mode)
{
    rwtLog(0xc4);
    rwtLogPtr(name);
    for (int i = 0; i < 3; i++) {
        rwtLog((uint8_t)mode[i]);
    }
    return gFopenOk ? (void *)(uintptr_t)0x1234 : NULL;
}

static int BMX_CDECL bmpFclose(void *file)
{
    rwtLog(0xc8);
    rwtLogPtr(file);
    return 0;
}

static size_t BMX_CDECL bmpFwrite(const void *buf, size_t size, size_t count, void *file)
{
    rwtLog(0xd0);
    rwtLog((uint32_t)size);
    rwtLogPtr(file);
    bmpLogBytes(buf, (uint32_t)(size * count));
    gWriteCalls++;
    return gWriteFailAt != 0 && gWriteCalls == gWriteFailAt ? 0 : count;
}

/* A write stream over custom callbacks. */
static RwStream *bmpStream(void)
{
    RwStream *s = dt_alloc(sizeof *s);
    s->type = rwSTREAMCUSTOM;
    s->access = rwSTREAMWRITE;
    s->u.custom.close = NULL;
    s->u.custom.write = bmpCustWrite;
    s->u.custom.data = dt_alloc(8);
    return s;
}

static void bmpEnv(DtCtx *c)
{
    RwGlobals *g = texEnv(c);
    g->fileFuncs.rwfopen = bmpFopen;
    g->fileFuncs.rwfclose = bmpFclose;
    g->fileFuncs.rwfwrite = bmpFwrite;
    gWriteCalls = 0;
    gWriteFailAt = dt_u32() % 5 == 0 ? dt_range(1, 12) : 0;
    gFopenOk = dt_u32() % 6 != 0;
    gRwtMallocFailAt = dt_u32() % 6 == 0 ? 1 : 0;
}

/* A 32 bit, 8 bit or 4 bit image of up to 6 x 4 pixels, an index byte per pixel for the paletted ones. */
static RwImage *bmpImage(void)
{
    static const int32_t depths[] = {32, 8, 4};
    RwImage *image = dt_alloc(sizeof *image);
    image->depth = depths[dt_u32() % 3];
    image->width = dt_range(0, 6);
    image->height = dt_u32() % 12 == 0 ? dt_range(-2, 0) : dt_range(0, 4);
    image->stride = image->width * (image->depth == 32 ? 4 : 1) + dt_range(0, 5);
    const int32_t rows = image->height > 0 ? image->height : 0;
    const uint32_t size = (uint32_t)(image->stride * rows) + 16;
    image->pixels = dt_alloc(size);
    dt_fill(image->pixels, size);
    if (image->depth != 32) {
        image->palette = dt_alloc(1024);
        dt_fill(image->palette, 1024);
    }
    return image;
}

/* ---- the register entries ----------------------------------------------------------------------------- */

typedef int32_t(__attribute__((regparm(1))) * BmpSize1)(int32_t);
typedef uint32_t(__attribute__((regparm(3))) * BmpRowBytes)(int32_t, int32_t, int32_t);
typedef int32_t(__attribute__((regparm(3))) * BmpWriteRows)(RwStream *, uint32_t, int32_t, int32_t, int32_t,
                                                            int32_t, int32_t, const RwImage *);
typedef int32_t(__attribute__((regparm(3))) * BmpWriteImage)(RwStream *, int32_t, int32_t, int32_t, int32_t,
                                                             int32_t, int32_t, const RwImage *);

static int32_t file_header_orig(int32_t format)
{
    return ((BmpSize1)(uintptr_t)0x004e7860)(format);
}

static int32_t file_header_impl(int32_t format)
{
    return bmpFileHeaderSize(format);
}

static int32_t info_header_orig(int32_t format)
{
    return ((BmpSize1)(uintptr_t)0x004e7880)(format);
}

static int32_t info_header_impl(int32_t format)
{
    return bmpInfoHeaderSize(format);
}

static uint32_t row_bytes_orig(int32_t format, int32_t bits, int32_t width)
{
    return ((BmpRowBytes)(uintptr_t)0x004e78a0)(format, bits, width);
}

static uint32_t row_bytes_impl(int32_t format, int32_t bits, int32_t width)
{
    return bmpRowBytes(format, bits, width);
}

static int32_t rows_orig(RwStream *stream, uint32_t width, int32_t height, const RwImage *image)
{
    return ((BmpWriteRows)(uintptr_t)0x004e7920)(stream, width, height, 0, 0, 0, 0x18, image);
}

static int32_t rows_impl(RwStream *stream, uint32_t width, int32_t height, const RwImage *image)
{
    return bmpWriteRows(stream, width, height, 0, 0, 0, 0x18, image);
}

static int32_t image_orig(RwStream *stream, int32_t format, int32_t width, int32_t height,
                          const RwImage *image)
{
    return ((BmpWriteImage)(uintptr_t)0x004e7b60)(stream, format, width, 0, 0, 0, height, image);
}

static int32_t image_impl(RwStream *stream, int32_t format, int32_t width, int32_t height,
                          const RwImage *image)
{
    return bmpWriteImage(stream, format, width, 0, 0, 0, height, image);
}

static void setup_format(DtCtx *c)
{
    dt_seed(c);
    DT_ARG_INT(c, 0, dt_u32() % 4 == 0 ? (int32_t)dt_u32() : dt_range(0, 3));
}

static void setup_row_bytes(DtCtx *c)
{
    dt_seed(c);
    DT_ARG_INT(c, 0, dt_u32() % 4 == 0 ? (int32_t)dt_u32() : dt_range(0, 3));
    DT_ARG_INT(c, 1, dt_u32() % 4 == 0 ? (int32_t)dt_u32() : (dt_u32() % 2 ? 0x18 : dt_range(1, 32)));
    DT_ARG_INT(c, 2, dt_u32() % 4 == 0 ? (int32_t)dt_u32() : dt_range(0, 300));
}

static void setup_rows(DtCtx *c)
{
    bmpEnv(c);
    RwImage *image = bmpImage();
    DT_ARG_PTR(c, 0, bmpStream());
    DT_ARG_INT(c, 1, image->width);
    DT_ARG_INT(c, 2, image->height);
    DT_ARG_PTR(c, 3, image);
    rwtEnd();
}

static void setup_image(DtCtx *c)
{
    bmpEnv(c);
    RwImage *image = bmpImage();
    DT_ARG_PTR(c, 0, bmpStream());
    DT_ARG_INT(c, 1, dt_u32() % 5 == 0 ? dt_range(0, 3) : 1 + (int32_t)(dt_u32() % 2));
    DT_ARG_INT(c, 2, image->width);
    DT_ARG_INT(c, 3, image->height);
    DT_ARG_PTR(c, 4, image);
    rwtEnd();
}

static void setup_write(DtCtx *c)
{
    bmpEnv(c);
    RwImage *image = bmpImage();
    char *name = dt_alloc(16);
    __builtin_memcpy(name, "out.bmp", 8);
    DT_ARG_PTR(c, 0, image);
    DT_ARG_PTR(c, 1, name);
    rwtEnd();
}

DT_CASES(rwbmp_cases) = {
    {"bmpFileHeaderSize", (uint32_t)(uintptr_t)file_header_orig, file_header_impl, 1, DT_RET_INT,
     setup_format, 20, 0},
    {"bmpInfoHeaderSize", (uint32_t)(uintptr_t)info_header_orig, info_header_impl, 1, DT_RET_INT,
     setup_format, 20, 0},
    {"bmpRowBytes", (uint32_t)(uintptr_t)row_bytes_orig, row_bytes_impl, 3, DT_RET_INT, setup_row_bytes, 400,
     0},
    {"bmpWriteRows", (uint32_t)(uintptr_t)rows_orig, rows_impl, 4, DT_RET_INT, setup_rows, 600, 0},
    {"bmpWriteImage", (uint32_t)(uintptr_t)image_orig, image_impl, 5, DT_RET_INT, setup_image, 600, 0},
    {"BMPImageWrite", 0x004e7eb0, BMPImageWrite, 2, DT_RET_INT, setup_write, 400, 0},
};
