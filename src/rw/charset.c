/* RenderWare 3.1 charset: a raster holding an 8x14 pixel debug font of 91 glyphs. */
#include "compat/crt.h"
#include "rw/rwcamera.h"

/* Partial RwImage: the pixel stride, the rest is read through the image calls. */
typedef struct RwImage {
    uint8_t pad00[0x10];
    int32_t stride; /* 0x10 */
} RwImage;

_Static_assert(offsetof(RwImage, stride) == 0x10, "RwImage layout");

/* Callees in other RenderWare groups (texture, image, raster), still original code. */
ORIG_FN(0x004dcb10, RwRaster *, RwRasterCreate,
        (int32_t width, int32_t height, int32_t depth, int32_t flags));
ORIG_FN(0x004dcbd0, int, RwRasterDestroy, (RwRaster * raster));
ORIG_FN(0x004e38e0, RwImage *, RwImageCreate, (int32_t width, int32_t height, int32_t depth));
ORIG_FN(0x004e3960, int, RwImageDestroy, (RwImage * image));
ORIG_FN(0x004e3ab0, RwImage *, RwImageAllocatePixels, (RwImage * image));
ORIG_FN(0x004e3a90, uint8_t *, RwImageGetPixels, (const RwImage *image));
ORIG_FN(0x004e3aa0, RwRGBA *, RwImageGetPalette, (const RwImage *image));
ORIG_FN(0x004e7f70, RwRaster *, RwRasterSetFromImage, (RwRaster * raster, RwImage *image));
/* The CRT's ceil (status skip). */
ORIG_FN(0x004a552a, double, crtCeil, (double x));
/*
 * RtCharsetPrint's worker (0x4cb860, not ported): EAX = charset, EDX = text, ECX = x, and the fourth word
 * of its stack arguments is y (it reads nothing else from the stack).
 */
ORIG_FN_REGPARM(3, 0x004cb860, int, rtCharsetPrintText,
                (RtCharset * charset, const char *text, int32_t x, int32_t unused0, int32_t unused1,
                 int32_t unused2, int32_t y));

typedef uint8_t RtCharsetGlyphs[91 * 14];
/* One byte per glyph row, the low bit is the leftmost pixel. */
#define rtCharsetGlyphs ORIG_VAR(0x005668c0, RtCharsetGlyphs, rtCharsetGlyphs)

static void rwReportNullError(void)
{
    RwError error;
    error.pluginID = 1;
    error.errorCode = _rwerror(rwERR_NULLP);
    RwErrorSet(&error);
}

/* The original's inline double to int conversion: it truncates, values below 1 give 0. */
static int32_t truncateToInt(double x)
{
    uint64_t bits;
    memcpy(&bits, &x, sizeof bits);
    const uint32_t high = (uint32_t)(bits >> 32);
    const uint32_t low = (uint32_t)bits;
    const uint32_t exponent = (high >> 20) & 0x7ff;
    if (exponent <= 0x3fe) {
        return 0;
    }
    const uint32_t value = (((high << 11) | (low >> 21)) | 0x80000000u) >> ((0x41e - exponent) & 31);
    return (int32_t)((high & 0x80000000u) != 0 ? 0u - value : value);
}

/*
 * Chooses the glyph grid and the image side: the smallest power of two (from 32) in which the glyphs, at
 * side / 8 per row, fit in side / 14 rows. Returns the side.
 */
static int32_t charsetLayout(RtCharsetDesc *desc)
{
    desc->width = 8;
    desc->height = 14;
    desc->count = 91;
    int32_t side = 16;
    int32_t rows;
    do {
        side *= 2;
        const int32_t perRow = (int32_t)(((uint32_t)(side >> 2) >> 29) + (uint32_t)side) >> 3;
        desc->glyphsPerRow = perRow;
        rows = truncateToInt(crtCeil((double)0x1.6cp6f / (double)perRow));
        desc->rows = rows;
    } while (side < rows * 14);
    return side;
}

/* Describes the font of a charset (see RtCharsetDesc); returns the charset, NULL for null arguments. */
BMX_ADDR(0x004cb3f0)
RtCharset *RtCharsetGetDesc(RtCharset *charset, RtCharsetDesc *desc)
{
    if (charset == NULL || desc == NULL) {
        rwReportNullError();
        return NULL;
    }
    charsetLayout(desc);
    return charset;
}

/*
 * Draws the font into an 8 bit image (palette: background, foreground) and loads the charset raster from it.
 * Returns the charset, 0 on failure. EAX = charset, EDX = foreground, ECX = background.
 */
BMX_ADDR(0x004cb51c)
__attribute__((regparm(3))) RtCharset *RtCharsetSetColors(RtCharset *charset, const RwRGBA *foreground,
                                                          const RwRGBA *background)
{
    if (charset == NULL || foreground == NULL || background == NULL) {
        rwReportNullError();
        return NULL;
    }
    RtCharsetDesc desc;
    const int32_t side = charsetLayout(&desc);
    RwImage *image = RwImageCreate(side, side, 8);
    if (image == NULL) {
        return NULL;
    }
    RtCharset *result = NULL;
    if (RwImageAllocatePixels(image) != NULL) {
        RwRGBA *palette = RwImageGetPalette(image);
        palette[0] = *background;
        palette[1] = *foreground;
        uint8_t *pixels = RwImageGetPixels(image);
        const int32_t stride = image->stride;
        const uint8_t *glyph = rtCharsetGlyphs;
        int32_t column = 0;
        int32_t row = 0;
        for (int32_t index = 0; index < desc.count; index++) {
            uint8_t *line = pixels + column * desc.width + stride * row * desc.height;
            for (int32_t y = 0; y < desc.height; y++) {
                uint8_t *next = line + stride;
                memset(line, 0, (size_t)desc.width);
                uint32_t bits = *glyph;
                if (bits != 0) {
                    uint8_t *pixel = line;
                    do {
                        *pixel++ = (uint8_t)(bits & 1);
                        bits >>= 1;
                    } while (bits != 0);
                }
                glyph++;
                line = next;
            }
            column++;
            if (desc.glyphsPerRow == column) {
                column = 0;
                row++;
            }
        }
        RwRasterSetFromImage(charset, image);
        result = charset;
    }
    RwImageDestroy(image);
    return result;
}

/* Releases the charset raster. */
BMX_ADDR(0x004cbf60)
int RtCharsetDestroy(RtCharset *charset)
{
    if (charset == NULL) {
        rwReportNullError();
        return 0;
    }
    RwRasterDestroy(charset);
    return 1;
}

/* A raster of the layout's size with the font drawn in the given colours; NULL on failure. */
BMX_ADDR(0x004cbfb0)
RtCharset *RtCharsetCreate(const RwRGBA *foreground, const RwRGBA *background)
{
    if (foreground == NULL || background == NULL) {
        rwReportNullError();
        return NULL;
    }
    RtCharsetDesc desc;
    const int32_t side = charsetLayout(&desc);
    RtCharset *charset = RwRasterCreate(side, side, 0, 4);
    if (charset == NULL) {
        return NULL;
    }
    if (RtCharsetSetColors(charset, foreground, background) == NULL) {
        RwRasterDestroy(charset);
        return NULL;
    }
    return charset;
}

/* Draws text at (x, y) into the current camera's raster (the work is done by the unported worker). */
BMX_ADDR(0x004cbef0)
int RtCharsetPrint(RtCharset *charset, const char *text, int32_t x, int32_t y)
{
    if (charset == NULL || text == NULL) {
        rwReportNullError();
        return 0;
    }
    return rtCharsetPrintText(charset, text, x, 0, 0, 0, y);
}

/* The device's indexed 2D primitive call (entry 0x24 of its table). */
BMX_ADDR(0x004db950)
int RwIm2DRenderIndexedPrimitive(int32_t primType, void *vertices, int32_t numVertices, void *indices,
                                 int32_t numIndices)
{
    typedef int(BMX_CDECL * IndexedFunc)(int32_t, void *, int32_t, void *, int32_t);
    return ((IndexedFunc)RwEngineInstance->dOpenDevice.fpIm2DRenderIndexedPrimitive)(
        primType, vertices, numVertices, indices, numIndices);
}
