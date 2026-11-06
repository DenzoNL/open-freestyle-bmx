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
ORIG_FN(0x004dcc30, int32_t, RwRasterGetWidth, (const RwRaster *raster));
ORIG_FN(0x004dcc40, int32_t, RwRasterGetHeight, (const RwRaster *raster));

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
BMX_REGPARM(3)
RtCharset *RtCharsetSetColors(RtCharset *charset, const RwRGBA *foreground, const RwRGBA *background)
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

/* A 2D immediate mode vertex as the charset fills it. */
typedef struct RtCharsetVertex {
    float x, y, z, rhw;
    uint32_t color;
    uint32_t pad14;
    float u, v;
} RtCharsetVertex;

_Static_assert(sizeof(RtCharsetVertex) == 0x20, "RtCharsetVertex layout");

static void charsetCorner(RtCharsetVertex *vertex, int32_t x, int32_t y, int32_t u, int32_t v,
                          double invWidth, double invHeight)
{
    vertex->x = (float)((double)x + 0.5);
    vertex->y = (float)((double)y + 0.5);
    vertex->u = (float)((double)u * invWidth);
    vertex->v = (float)((double)v * invHeight);
}

/*
 * RtCharsetPrint's worker: one textured quad per character (8 x 14 pixels from (x, y)), clipped to the
 * current camera's raster, drawn as one indexed triangle list with texture filtering and the raster render
 * states set and restored. EAX = charset, EDX = text, ECX = x, y is the fourth stack word. Always 0.
 */
BMX_ADDR(0x004cb860)
BMX_REGPARM(3)
int rtCharsetPrintText(RtCharset *charset, const char *text, int32_t x, int32_t unused0, int32_t unused1,
                       int32_t unused2, int32_t y)
{
    (void)unused0;
    (void)unused1;
    (void)unused2;
    typedef int32_t(BMX_CDECL * StrlenFunc)(const char *s);
    const int32_t length = ((StrlenFunc)RwEngineInstance->stringFuncs.rwstrlen)(text);
    if (length == 0) {
        return 0;
    }
    RwCamera *camera = RwCameraGetCurrentCamera();
    if (camera == NULL) {
        return 0;
    }
    const RwRaster *target = RwCameraGetRaster(camera);
    if (target == NULL) {
        return 0;
    }
    const int32_t screenHeight = RwRasterGetHeight(target);
    const int32_t screenWidth = RwRasterGetWidth(target);
    const int32_t fontHeight = RwRasterGetHeight(charset);
    const int32_t fontWidth = RwRasterGetWidth(charset);
    const double invHeight = (double)1.0f / fontHeight;
    const double invWidth = (double)1.0f / fontWidth;
    RtCharsetVertex *vertices = RwEngineInstance->memoryFuncs.rwmalloc((size_t)(length << 7));
    uint16_t *indices = RwEngineInstance->memoryFuncs.rwmalloc((size_t)(length * 12));
    if (vertices == NULL || indices == NULL) {
        if (vertices != NULL) {
            RwEngineInstance->memoryFuncs.rwfree(vertices);
        }
        if (indices != NULL) {
            RwEngineInstance->memoryFuncs.rwfree(indices);
        }
        RwError error;
        error.pluginID = 0x180;
        error.errorCode = _rwerror(rwERR_NOMEM, (length << 7) + length * 12);
        RwErrorSet(&error);
        return 0;
    }
    RtCharsetDesc desc;
    charsetLayout(&desc);
    const int32_t perRow = desc.glyphsPerRow;
    int32_t savedFilter;
    int32_t savedAddress;
    RwRenderStateGet(4, &savedFilter);
    RwRenderStateGet(6, &savedAddress);
    RwRenderStateSet(4, 0);
    RwRenderStateSet(6, 0);
    RwRenderStateSet(7, 1);
    RwRenderStateSet(1, (intptr_t)charset);
    RwRenderStateSet(8, 5);
    RwRenderStateSet(9, 6);
    for (int32_t i = 0; i < length * 4; i++) {
        vertices[i].rhw = 1.0f;
        vertices[i].z = 1.0f;
        vertices[i].color = 0xffffffffu;
    }
    uint32_t quads = 0;
    for (int32_t i = 0; i < length; i++) {
        int32_t left = x + i * 8;
        int32_t right = left + 8;
        int32_t top = y;
        int32_t bottom = y + 14;
        if (top >= screenHeight || bottom <= 0 || left >= screenWidth || right <= 0) {
            continue;
        }
        const int32_t glyph = text[i] - 0x20;
        const int32_t row = glyph / perRow;
        int32_t u0 = (glyph - perRow * row) * 8;
        int32_t u1 = u0 + 8;
        int32_t v0 = row * 14;
        int32_t v1 = v0 + 14;
        if (top < 0) {
            v0 -= top;
            top = 0;
        }
        if (bottom > screenHeight - 1) {
            bottom = screenHeight - 1;
            v1 = bottom - top + v0;
        }
        if (left < 0) {
            u0 -= left;
            left = 0;
        }
        if (right > screenWidth - 1) {
            right = screenWidth - 1;
            u1 = right - left + u0;
        }
        RtCharsetVertex *quad = &vertices[quads * 4];
        charsetCorner(&quad[0], left, top, u0, v0, invWidth, invHeight);
        charsetCorner(&quad[1], left, bottom, u0, v1, invWidth, invHeight);
        charsetCorner(&quad[2], right, bottom, u1, v1, invWidth, invHeight);
        charsetCorner(&quad[3], right, top, u1, v0, invWidth, invHeight);
        uint16_t *triangles = &indices[quads * 6];
        const uint16_t first = (uint16_t)(quads * 4);
        triangles[0] = first;
        triangles[1] = (uint16_t)(first + 1);
        triangles[2] = (uint16_t)(first + 2);
        triangles[3] = first;
        triangles[4] = (uint16_t)(first + 2);
        triangles[5] = (uint16_t)(first + 3);
        quads++;
    }
    if (quads != 0) {
        RwIm2DRenderIndexedPrimitive(3, vertices, (int32_t)(quads * 4), indices, (int32_t)(quads * 6));
    }
    RwRenderStateSet(4, savedFilter);
    RwRenderStateSet(6, savedAddress);
    RwEngineInstance->memoryFuncs.rwfree(indices);
    RwEngineInstance->memoryFuncs.rwfree(vertices);
    return 0;
}

/* Draws text at (x, y) into the current camera's raster. */
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
