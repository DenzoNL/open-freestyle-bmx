/*
 * RenderWare 3.1 textures, images and rasters (rw/texture.c).
 *
 * Image and raster work that touches pixels goes through the standard functions of the device driver
 * (RwGlobals.stdFunc, indexed by RwStandardType); those calls stay indirect, they are the seam for our own
 * renderer. Each module keeps its state in plugin data of the engine instance (RwTextureGlobals,
 * RwRasterGlobals, RwImageGlobals), found through the offsets the modules' open routines store.
 */
#ifndef RW_RWTEXTURE_H
#define RW_RWTEXTURE_H

#include "rw/rwcore.h"
#include "rw/rwstream.h"

/* Indices into RwGlobals.stdFunc (the device's standard functions). */
enum {
    rwSTANDARDPIXELTORGB = 3,
    rwSTANDARDRASTERCREATE = 4,
    rwSTANDARDRASTERDESTROY = 5,
    rwSTANDARDRASTERSETIMAGE = 7,
    rwSTANDARDTEXTURESETRASTER = 8,
    rwSTANDARDSETRASTERCONTEXT = 11,
    rwSTANDARDRASTERSUBRASTER = 12,
    rwSTANDARDRASTERSHOWRASTER = 20,
    rwSTANDARDRASTERLOCK = 15,
    rwSTANDARDRASTERUNLOCK = 16,
    rwSTANDARDRASTERLOCKPALETTE = 23,
    rwSTANDARDRASTERUNLOCKPALETTE = 24,
};

/* Raster types (RwRaster.type), flags and private flags. */
enum {
    rwRASTERTYPENORMAL = 0,
    rwRASTERTYPEZBUFFER = 1,
    rwRASTERTYPECAMERA = 2,
    rwRASTERTYPETEXTURE = 4,
    rwRASTERTYPECAMERATEXTURE = 5,
    rwRASTERDONTALLOCATE = 0x80,   /* RwRaster.flags: a sub raster must have this on its parent */
    rwRASTERGAMMACORRECTED = 0x01, /* RwRaster.privateFlags */
    rwRASTERPALETTELOCKED = 0x18,  /* RwRaster.privateFlags: the palette lock bits */
    rwRASTERPIXELSLOCKED = 0x06,   /* RwRaster.privateFlags: the pixel lock bits (read, write) */
};

/* An RwRaster; type, flags and format are filled in by the device's create function. */
typedef struct RwRaster {
    struct RwRaster *parent; /* 0x00 the raster itself, or the parent of a sub raster */
    uint8_t *pixels;         /* 0x04 */
    uint8_t *palette;        /* 0x08 */
    int32_t width;           /* 0x0c */
    int32_t height;          /* 0x10 */
    int32_t depth;           /* 0x14 */
    int32_t stride;          /* 0x18 */
    int16_t originX;         /* 0x1c offset inside the parent */
    int16_t originY;         /* 0x1e */
    uint8_t type;            /* 0x20 */
    uint8_t flags;           /* 0x21 */
    uint8_t privateFlags;    /* 0x22 */
    uint8_t format;          /* 0x23 */
    uint8_t stamp;           /* 0x24 incremented when the raster changes */
    uint8_t pad25[3];        /* 0x25 */
    void *pad28;             /* 0x28 */
    int32_t originalWidth;   /* 0x2c level 0 size and stride, kept by the driver while a level is locked */
    int32_t originalHeight;  /* 0x30 */
    int32_t originalStride;  /* 0x34 */
} RwRaster;

BMX_LAYOUT32_ASSERT(offsetof(RwRaster, originX) == 0x1c, "RwRaster layout");
BMX_LAYOUT32_ASSERT(offsetof(RwRaster, type) == 0x20, "RwRaster layout");
BMX_LAYOUT32_ASSERT(offsetof(RwRaster, stamp) == 0x24, "RwRaster layout");
BMX_LAYOUT32_ASSERT(offsetof(RwRaster, originalWidth) == 0x2c && sizeof(RwRaster) == 0x38, "RwRaster layout");

/* The raster module's data in the engine instance: the context stack and the raster free list. */
typedef struct RwRasterGlobals {
    RwRaster *stack[10];  /* 0x00 pushed rasters; the entry for depth n is stack[n + 1] */
    int32_t depth;        /* 0x28 number of pushed contexts */
    RwRaster root;        /* 0x2c the bottom of the context stack: a pseudo raster without a parent */
    RwFreeList *freeList; /* 0x64 */
} RwRasterGlobals;

BMX_LAYOUT32_ASSERT(offsetof(RwRasterGlobals, freeList) == 0x64, "RwRasterGlobals layout");

typedef struct RwTexDictionary RwTexDictionary;
typedef struct RwTexture RwTexture;

/* A texture: a raster with a name, in at most one dictionary (lInDictionary). */
struct RwTexture {
    RwRaster *raster;       /* 0x00 */
    uint8_t stamp;          /* 0x04 incremented when the texture changes */
    uint8_t pad05[3];       /* 0x05 */
    RwTexDictionary *dict;  /* 0x08 */
    RwLLLink lInDictionary; /* 0x0c */
    char *name;             /* 0x14 */
    char *mask;             /* 0x18 */
    uint32_t refCount;      /* 0x1c */
    int32_t filterMode;     /* 0x20 */
    int32_t addressing;     /* 0x24 */
};

BMX_LAYOUT32_ASSERT(sizeof(RwTexture) == 0x28, "RwTexture layout");
BMX_LAYOUT32_ASSERT(offsetof(RwTexture, lInDictionary) == 0xc, "RwTexture layout");
BMX_LAYOUT32_ASSERT(offsetof(RwTexture, refCount) == 0x1c, "RwTexture layout");

/* A texture dictionary: its textures (texturesInDict) and its place in the list of all dictionaries. */
struct RwTexDictionary {
    RwObject object;         /* 0x00 type 6 */
    uint8_t stamp;           /* 0x08 incremented when a texture is added */
    uint8_t pad09[3];        /* 0x09 */
    RwLLLink texturesInDict; /* 0x0c */
    RwLLLink lInInstance;    /* 0x14 */
};

BMX_LAYOUT32_ASSERT(sizeof(RwTexDictionary) == 0x1c, "RwTexDictionary layout");
BMX_LAYOUT32_ASSERT(offsetof(RwTexDictionary, texturesInDict) == 0xc, "RwTexDictionary layout");

typedef void *(BMX_CDECL *RwTextureCallBackRead)(const char *name, const char *maskName);
typedef void *(BMX_CDECL *RwTextureCallBackFind)(const char *name);

/* The texture module's data in the engine instance. */
typedef struct RwTextureGlobals {
    RwLLLink dictList;                  /* 0x00 all dictionaries */
    RwFreeList *textureFreeList;        /* 0x08 */
    RwFreeList *dictFreeList;           /* 0x0c */
    RwTexDictionary *currentDict;       /* 0x10 */
    RwTextureCallBackRead readCallBack; /* 0x14 */
    RwTextureCallBackFind findCallBack; /* 0x18 */
    int32_t mipmapping;                 /* 0x1c */
    void *field20;                      /* 0x20 file name buffer of the texture readers, freed on close */
    uint16_t field24;                   /* 0x24 its size (16 bits) */
    uint8_t pad26[2];                   /* 0x26 */
    void *defaultFind; /* 0x28 the mipmap generator (0x4e1140), called by the mipmapped reader */
} RwTextureGlobals;

BMX_LAYOUT32_ASSERT(sizeof(RwTextureGlobals) == 0x2c, "RwTextureGlobals layout");

enum { rwIMAGEOWNSPIXELS = 1, rwIMAGEGAMMACORRECTED = 2 };

/* An RwImage: pixels (and a palette for depths 4 and 8) in plain system memory. */
typedef struct RwImage {
    uint32_t flags;   /* 0x00 rwIMAGE* */
    int32_t width;    /* 0x04 */
    int32_t height;   /* 0x08 */
    int32_t depth;    /* 0x0c 4, 8 or 32 */
    int32_t stride;   /* 0x10 bytes per row */
    uint8_t *pixels;  /* 0x14 */
    uint8_t *palette; /* 0x18 RGBA, 1 << depth entries (depths 4 and 8) */
    uint8_t stamp;    /* 0x1c */
} RwImage;

BMX_LAYOUT32_ASSERT(offsetof(RwImage, pixels) == 0x14, "RwImage layout");
BMX_LAYOUT32_ASSERT(offsetof(RwImage, stamp) == 0x1c, "RwImage layout");

typedef RwImage *(BMX_CDECL *RwImageCallBackRead)(const char *name);
typedef RwImage *(BMX_CDECL *RwImageCallBackWrite)(RwImage *image, const char *name);

/* A registered image file format: its extension in both cases and the readers/writers. */
typedef struct RwImageFormat {
    char extension[0x14];       /* 0x00 ".ext", lower case */
    char extensionUpper[0x14];  /* 0x14 ".EXT" */
    RwImageCallBackRead read;   /* 0x28 */
    RwImageCallBackWrite write; /* 0x2c */
    struct RwImageFormat *next; /* 0x30 */
} RwImageFormat;

BMX_LAYOUT32_ASSERT(sizeof(RwImageFormat) == 0x34, "RwImageFormat layout");

/* The image module's data in the engine instance. */
typedef struct RwImageGlobals {
    RwFreeList *imageFreeList; /* 0x000 */
    char *path;                /* 0x004 the image search path */
    int32_t pathSize;          /* 0x008 bytes allocated for it */
    uint8_t gammaUp[0x100];    /* 0x00c table of the gamma applied when reading */
    uint8_t gammaDown[0x100];  /* 0x10c and its inverse */
    float gamma;               /* 0x20c */
    union {
        uint8_t pad210[8]; /* 0x210 */
        struct {
            void *nameBuffer;       /* 0x210 the buffer a candidate file name is built in (grown on demand) */
            int32_t nameBufferSize; /* 0x214 */
        };
    };
    RwFreeList *formatFreeList; /* 0x218 */
    RwImageFormat *formats;     /* 0x21c newest first */
} RwImageGlobals;

BMX_LAYOUT32_ASSERT(offsetof(RwImageGlobals, gamma) == 0x20c, "RwImageGlobals layout");
BMX_LAYOUT32_ASSERT(offsetof(RwImageGlobals, formats) == 0x21c, "RwImageGlobals layout");

/* Offsets of the module data inside the engine instance. */
#define rwTextureOffset ORIG_VAR(0x00676d40, int32_t, rwTextureOffset)
#define rwRasterOffset  ORIG_VAR(0x00676d00, int32_t, rwRasterOffset)
#define rwImageOffset   ORIG_VAR(0x00676d60, int32_t, rwImageOffset)

/* The plugin registries of the objects (in .data; the texture one is in rwplugins.h). */
#define rwRasterRegistry        ORIG_VAR(0x00566f30, RwPluginRegistry, rwRasterRegistry)
#define rwTexDictionaryRegistry ORIG_VAR(0x00566fac, RwPluginRegistry, rwTexDictionaryRegistry)
#define rwImageRegistry         ORIG_VAR(0x00566fc0, RwPluginRegistry, rwImageRegistry)

/* ---- rasters ---------------------------------------------------------------------------------------- */

RwRaster *RwRasterCreate(int32_t width, int32_t height, int32_t depth, int32_t flags);
int RwRasterDestroy(RwRaster *raster);
int32_t RwRasterGetWidth(const RwRaster *raster);
int32_t RwRasterGetHeight(const RwRaster *raster);
int32_t RwRasterGetStride(const RwRaster *raster);
int32_t RwRasterGetDepth(const RwRaster *raster);
uint32_t RwRasterGetFormat(const RwRaster *raster);
RwRaster *RwRasterSubRaster(RwRaster *subRaster, RwRaster *raster,
                            const int32_t *rect); /* rect: x, y, w, h */
RwRaster *RwRasterGetParent(const RwRaster *raster);
RwRaster *RwRasterPushContext(RwRaster *raster);
RwRaster *RwRasterPopContext(void);
RwRaster *RwRasterShowRaster(RwRaster *raster, void *dev, uint32_t flags);
uint8_t *RwRasterLockPalette(RwRaster *raster, int32_t lockMode);
RwRaster *RwRasterUnlockPalette(RwRaster *raster);
uint8_t *RwRasterLock(RwRaster *raster, uint32_t level, uint32_t mode);
RwRaster *RwRasterUnlock(RwRaster *raster);

/* ---- textures and dictionaries ---------------------------------------------------------------------- */

int RwTextureSetReadCallBack(RwTextureCallBackRead callBack);
RwTextureCallBackRead RwTextureGetReadCallBack(void);
int RwTextureSetFindCallBack(RwTextureCallBackFind callBack);
RwTextureCallBackFind RwTextureGetFindCallBack(void);
int RwTextureSetMipmapping(int32_t enable);
RwTexture *RwTextureSetRaster(RwTexture *texture, RwRaster *raster);
RwRaster *RwTextureGetRaster(const RwTexture *texture);
RwTexDictionary *RwTexDictionaryCreate(void);
RwTexture *RwTextureDestroyAll(RwTexture *texture);
int RwTextureDestroy(RwTexture *texture);
/* The release body of RwTextureDestroy (EAX = texture). */
__attribute__((regparm(1))) int rwTextureRelease(RwTexture *texture);

/* Module open/close routines the engine registry calls (plugin data at the given offset; NULL = failed). */
void *_rwRasterOpen(void *engine, int32_t offset);
void *_rwRasterClose(void *engine);
void *_rwTextureOpen(void *engine, int32_t offset);
void *_rwTextureClose(void *engine);
void *_rwImageOpen(void *engine, int32_t offset);
void *_rwImageClose(void *engine);
int RwTexDictionaryDestroy(RwTexDictionary *dictionary);
RwTexture *RwTextureCreate(RwRaster *raster);
RwTexture *RwTextureAddRef(RwTexture *texture);
const char *RwTextureGetName(const RwTexture *texture);
RwTexture *RwTextureSetName(RwTexture *texture, const char *name);
RwTexture *RwTexDictionaryAddTexture(RwTexDictionary *dictionary, RwTexture *texture);
RwTexDictionary *RwTexDictionarySetCurrent(RwTexDictionary *dictionary);
RwTexture *RwTextureRead(const char *name, const char *maskName);
RwTexture *RwTextureSetFilterMode(RwTexture *texture, int32_t mode);
RwTexture *RwTextureSetAddressing(RwTexture *texture, int32_t mode);

/* ---- images ----------------------------------------------------------------------------------------- */

RwImage *RwImageCreate(int32_t width, int32_t height, int32_t depth);
int RwImageDestroy(RwImage *image);
int32_t RwImageGetWidth(const RwImage *image);
int32_t RwImageGetHeight(const RwImage *image);
int32_t RwImageGetDepth(const RwImage *image);
uint8_t *RwImageGetPixels(const RwImage *image);
uint8_t *RwImageGetPalette(const RwImage *image);
RwImage *RwImageAllocatePixels(RwImage *image);
RwImage *RwImageFreePixels(RwImage *image);
const char *RwImageSetPath(const char *path);
int RwImageRegisterImageFormat(const char *extension, RwImageCallBackRead read, RwImageCallBackWrite write);

/* The BMP writer: the ".bmp" write callback and its pieces (register entries, see texture.c). */
RwImage *BMPImageWrite(RwImage *image, const char *name);
__attribute__((regparm(1))) int32_t bmpFileHeaderSize(int32_t format);
__attribute__((regparm(1))) int32_t bmpInfoHeaderSize(int32_t format);
__attribute__((regparm(3))) uint32_t bmpRowBytes(int32_t format, int32_t bits, int32_t width);
__attribute__((regparm(3))) int32_t bmpWriteRows(RwStream *stream, uint32_t width, int32_t height,
                                                 int32_t unused0, int32_t unused1, int32_t unused2,
                                                 int32_t unused3, const RwImage *image);
__attribute__((regparm(3))) int32_t bmpWriteImage(RwStream *stream, int32_t format, int32_t width,
                                                  int32_t unused0, int32_t unused1, int32_t unused2,
                                                  int32_t height, const RwImage *image);

/* Image files: the walk over the image search path and its callbacks (rw/texture.c). */
typedef const char *(BMX_CDECL *RwImagePathCallback)(char *path, void *data);
typedef struct RwImageAttempt {
    RwImageCallBackRead read;
    RwImage *image;
} RwImageAttempt;

__attribute__((regparm(3))) const char *ImagePathForAllFullNames(const char *name, int32_t extra,
                                                                 RwImagePathCallback callback,
                                                                 int32_t unused0, int32_t unused1,
                                                                 int32_t unused2, void *data);
const char *rwImageFindFormatCallback(char *path, void *data);
const char *RwImageFindFileType(const char *name);
const char *ImageAttempRead(char *name, void *data);
/* Copy the pixels of one image into another (EAX = dest, EDX = source for the register entry). */
__attribute__((regparm(2))) RwImage *rwImageCopyPixels(RwImage *dest, RwImage *source);
RwImage *RwImageResize(RwImage *dest, RwImage *source);
RwImage *RwImageGammaCorrect(RwImage *image);
/* Shrinks a 32 bit image by block averages (EAX = source, EDX = dest). */
__attribute__((regparm(2))) int rwImageBoxShrink(const RwImage *source, RwImage *dest);
void *RwRGBAFromPixel(void *rgba, uint32_t pixel, int32_t format);
int RwImageSetGamma(float gamma);
RwImage *RtBMPImageRead(const char *name);
RwRaster *RwRasterSetFromImage(RwRaster *raster, RwImage *image);
RwImage *RtPNGImageRead(const char *name);

/*
 * One scan line of a BMP into the image (register arguments: EAX = image, EDX = row, ECX = source), as
 * file format srcDepth bits per pixel into the image's depth. `flags` non-zero: 24 bit data is already RGB
 * (RLE); the three reserved parameters stand for stack slots the original never reads. 0 on an unknown
 * combination.
 */
__attribute__((regparm(3))) int BMPImageSetSpan(RwImage *image, int32_t row, const uint8_t *source,
                                                int32_t reserved0, int32_t reserved1, int32_t reserved2,
                                                int32_t srcDepth, int32_t flags);

/* Default read callback of the texture module and the image/raster glue next to it (rw/texture.c). */
void *rwTextureDefaultRead(const char *name, const char *maskName);
RwImage *RwImageSetFromRaster(RwImage *image, RwRaster *raster);
RwImage *RwImageFindRasterFormat(RwImage *image, int32_t flags, int32_t *width, int32_t *height,
                                 int32_t *depth, uint32_t *format);
int _rwpathisabsolute(const char *path);
int rwImageFormatSucceed(void);

/* Texture readers, mip level generator and image file reading (rw/texture.c). */
/* EAX = name, EDX = mask name, ECX = width; height is the fourth stack word (three unread words before it).
 */
__attribute__((regparm(3))) RwImage *rwTextureReadImage(const char *name, const char *maskName, int32_t width,
                                                        int32_t unused0, int32_t unused1, int32_t unused2,
                                                        int32_t height);
void *rwTextureReadPlain(const char *name, const char *maskName);
void *rwTextureReadMipmapped(const char *name, const char *maskName);
RwRaster *rwRasterGenerateMipmaps(RwRaster *raster, RwImage *image);
/* EAX = image, EDX = mask. */
__attribute__((regparm(2))) RwImage *RwImageApplyMask(RwImage *image, RwImage *mask);
RwImage *RwImageRead(const char *name, const char *maskName);

#endif
