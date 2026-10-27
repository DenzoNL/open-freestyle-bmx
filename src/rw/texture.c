/*
 * RenderWare 3.1 textures, texture dictionaries, images and rasters: the object bookkeeping, the BMP reader
 * and the PNG reader's wrapper. Pixel work on rasters goes through the device driver's standard functions
 * (RwGlobals.stdFunc), libpng stays original code.
 */
#include "rw/rwtexture.h"

#include "compat/crt.h"
#include "rw/rwplugins.h"
#include "rw/rwstream.h"

/* The CRT's pow (it returns its result on the x87 stack). */
ORIG_FN(0x0051d939, double, crtPow, (double base, double exponent));

/* The libpng 1.0.5 linked into the exe, called as it is. */
ORIG_FN(0x005070d0, void *, png_create_read_struct_2,
        (const char *version, void *errorPtr, void *errorFn, void *warnFn, void *memPtr, void *mallocFn,
         void *freeFn));
ORIG_FN(0x00508e40, void *, png_create_info_struct, (void *png));
/* The CRT setjmp (MSVC _setjmp3(buf, count, ...)): the original reads more stack slots than count names. */
ORIG_FN(0x0051e008, int, _setjmp3, (void *jmpBuf, int count, ...));
ORIG_FN(0x005090f0, void, png_set_read_fn, (void *png, void *ioPtr, void *readFn));
ORIG_FN(0x00507290, void, png_read_info, (void *png, void *info));
ORIG_FN(0x00509600, uint32_t, png_get_IHDR,
        (void *png, void *info, uint32_t *width, uint32_t *height, int *bitDepth, int *colorType,
         int *interlace, int *compression, int *filter));
ORIG_FN(0x00509b90, void, png_set_strip_16, (void *png));
ORIG_FN(0x00510bb0, void, png_set_packing, (void *png));
ORIG_FN(0x0050a610, void, png_set_gray_1_2_4_to_8, (void *png));
ORIG_FN(0x0050a650, void, png_set_gray_to_rgb, (void *png));
ORIG_FN(0x00510c50, void, png_set_filler, (void *png, uint32_t filler, int where));
ORIG_FN(0x00509150, uint32_t, png_get_valid, (void *png, void *info, uint32_t flag));
ORIG_FN(0x00509890, uint32_t, png_get_PLTE, (void *png, void *info, uint8_t **palette, int *numPalette));
ORIG_FN(0x00509970, uint32_t, png_get_tRNS,
        (void *png, void *info, uint8_t **trans, int *numTrans, void **transValues));
ORIG_FN(0x00507f80, void, png_read_image, (void *png, uint8_t **rows));
ORIG_FN(0x00508000, void, png_read_end, (void *png, void *info));
ORIG_FN(0x00508550, void, png_destroy_read_struct, (void **png, void **info, void **endInfo));

/* Small callbacks of the PNG reader that are not functions of their own in the exe (they only forward). */
#define PNG_MALLOC_CALLBACK 0x00506820
#define PNG_FREE_CALLBACK   0x00506840
#define PNG_READ_CALLBACK   0x00506860

/* ---- shared helpers --------------------------------------------------------------------------------- */

static RwRasterGlobals *rasterGlobals(void)
{
    return (RwRasterGlobals *)((uint8_t *)RwEngineInstance + rwRasterOffset);
}

static RwTextureGlobals *textureGlobals(void)
{
    return (RwTextureGlobals *)((uint8_t *)RwEngineInstance + rwTextureOffset);
}

static RwImageGlobals *imageGlobals(void)
{
    return (RwImageGlobals *)((uint8_t *)RwEngineInstance + rwImageOffset);
}

/* One of the device's standard functions: (out, in, flags), non-zero on success. */
typedef int(BMX_CDECL *RwStandardFunc)(void *out, void *in, int32_t flags);

static RwStandardFunc standardFunc(int index)
{
    return (RwStandardFunc)(uintptr_t)RwEngineInstance->stdFunc[index];
}

/* Records an error with no arguments in the error slot. */
static void reportError(int32_t code)
{
    RwError error;
    error.pluginID = 1;
    error.errorCode = _rwerror(code);
    RwErrorSet(&error);
}

/* Records "out of memory" for a block of the given size. */
static void reportNoMemory(int32_t size)
{
    RwError error;
    error.pluginID = 1;
    error.errorCode = _rwerror(rwERR_NOMEM, size);
    RwErrorSet(&error);
}

/* The engine's C string functions (filled in by _rwStringOpen). */
static size_t rwStrlen(const char *s)
{
    return ((size_t(BMX_CDECL *)(const char *))RwEngineInstance->stringFuncs.rwstrlen)(s);
}

static char *rwStrcpy(char *dst, const char *src)
{
    return RwEngineInstance->stringFuncs.rwstrcpy(dst, src);
}

static char *rwStrncpy(char *dst, const char *src, size_t n)
{
    return ((char *(BMX_CDECL *)(char *, const char *, size_t))RwEngineInstance->stringFuncs.rwstrncpy)(
        dst, src, n);
}

static int rwStrcmp(const char *a, const char *b)
{
    return ((int(BMX_CDECL *)(const char *, const char *))RwEngineInstance->stringFuncs.rwstrcmp)(a, b);
}

static char *rwStrupr(char *s)
{
    return ((char *(BMX_CDECL *)(char *))RwEngineInstance->stringFuncs.rwstrupr)(s);
}

static char *rwStrlwr(char *s)
{
    return ((char *(BMX_CDECL *)(char *))RwEngineInstance->stringFuncs.rwstrlwr)(s);
}

/* ---- rasters ---------------------------------------------------------------------------------------- */

/*
 * A raster of the given size and depth; the device's create function sets its type, format and pixels from
 * `flags`. NULL (and the raster freed again) if the device refuses.
 */
BMX_ADDR(0x004dcb10)
RwRaster *RwRasterCreate(int32_t width, int32_t height, int32_t depth, int32_t flags)
{
    RwRaster *raster = RwEngineInstance->freeListAlloc(rasterGlobals()->freeList);
    if (raster == NULL) {
        return NULL;
    }
    raster->palette = NULL;
    raster->pixels = NULL;
    raster->stamp = 0;
    raster->parent = raster;
    raster->depth = depth;
    raster->originY = 0;
    raster->originX = 0;
    raster->height = height;
    raster->width = width;
    raster->flags = 0;
    raster->privateFlags = 0;
    if (standardFunc(rwSTANDARDRASTERCREATE)(NULL, raster, flags) != 0) {
        _rwPluginRegistryInitObject(&rwRasterRegistry, raster);
        return raster;
    }
    RwEngineInstance->freeListFree(rasterGlobals()->freeList, raster);
    return NULL;
}

BMX_ADDR(0x004dcbd0)
int RwRasterDestroy(RwRaster *raster)
{
    _rwPluginRegistryDeInitObject(&rwRasterRegistry, raster);
    standardFunc(rwSTANDARDRASTERDESTROY)(NULL, raster, 0);
    RwEngineInstance->freeListFree(rasterGlobals()->freeList, raster);
    return 1;
}

BMX_ADDR(0x004dcc30)
int32_t RwRasterGetWidth(const RwRaster *raster)
{
    return raster->width;
}

BMX_ADDR(0x004dcc40)
int32_t RwRasterGetHeight(const RwRaster *raster)
{
    return raster->height;
}

BMX_ADDR(0x004dcc50)
int32_t RwRasterGetStride(const RwRaster *raster)
{
    return raster->stride;
}

BMX_ADDR(0x004dcc60)
int32_t RwRasterGetDepth(const RwRaster *raster)
{
    return raster->depth;
}

/* The format byte, shifted up to where RenderWare keeps the format flags. */
BMX_ADDR(0x004dcc70)
uint32_t RwRasterGetFormat(const RwRaster *raster)
{
    return (uint32_t)raster->format << 8;
}

/*
 * Makes `subRaster` (a raster created with rwRASTERDONTALLOCATE) a window onto a rectangle of `raster`.
 * The raster's fields are changed before the device is asked, and stay so if it refuses.
 */
BMX_ADDR(0x004dcc80)
RwRaster *RwRasterSubRaster(RwRaster *subRaster, RwRaster *raster, const int32_t *rect)
{
    if ((subRaster->flags & rwRASTERDONTALLOCATE) == 0) {
        return NULL;
    }
    const int32_t x = rect[0];
    const int32_t y = rect[1];
    if (x < 0 || x >= raster->width || y < 0 || y >= raster->height || x + rect[2] > raster->width ||
        y + rect[3] > raster->height) {
        reportError(0xf);
        return NULL;
    }
    subRaster->width = rect[2];
    subRaster->height = rect[3];
    subRaster->originX = (int16_t)(raster->originX + (int16_t)x);
    subRaster->originY = (int16_t)(raster->originY + (int16_t)y);
    if (standardFunc(rwSTANDARDRASTERSUBRASTER)(subRaster, raster, 0) == 0) {
        return NULL;
    }
    subRaster->parent = raster->parent;
    subRaster->stamp++;
    return subRaster;
}

BMX_ADDR(0x004dcd70)
RwRaster *RwRasterGetParent(const RwRaster *raster)
{
    return raster->parent;
}

/* Makes the raster the render target and pushes it on the context stack (nine deep). */
BMX_ADDR(0x004dcd80)
RwRaster *RwRasterPushContext(RwRaster *raster)
{
    if (rasterGlobals()->depth >= 9) {
        reportError(0xd);
        return NULL;
    }
    if (standardFunc(rwSTANDARDSETRASTERCONTEXT)(NULL, raster, 0) == 0) {
        return NULL;
    }
    RwRasterGlobals *globals = rasterGlobals();
    const int32_t depth = globals->depth;
    globals->depth = depth + 1;
    globals->stack[depth + 1] = raster;
    return raster;
}

/* Pops the context stack and makes the raster below the render target again; returns the one popped. */
BMX_ADDR(0x004dce20)
RwRaster *RwRasterPopContext(void)
{
    RwRasterGlobals *globals = rasterGlobals();
    const int32_t depth = globals->depth;
    if (depth <= 0) {
        reportError(0xe);
        return NULL;
    }
    globals->depth = depth - 1;
    RwRaster *popped = globals->stack[depth];
    if (standardFunc(rwSTANDARDSETRASTERCONTEXT)(NULL, globals->stack[globals->depth], 0) == 0) {
        return NULL;
    }
    return popped;
}

/* Purges the resource arena, then flips the raster to the screen. */
BMX_ADDR(0x004dcfd0)
RwRaster *RwRasterShowRaster(RwRaster *raster, void *dev, uint32_t flags)
{
    _rwResourcesPurge();
    if (standardFunc(rwSTANDARDRASTERSHOWRASTER)(raster, dev, (int32_t)flags) == 0) {
        return NULL;
    }
    return raster;
}

/* The palette of an 8 bit raster for writing; NULL if it is locked or the device refuses. */
BMX_ADDR(0x004dd160)
uint8_t *RwRasterLockPalette(RwRaster *raster, int32_t lockMode)
{
    if ((raster->privateFlags & rwRASTERPALETTELOCKED) != 0) {
        reportError((int32_t)0x80000014);
        return NULL;
    }
    uint8_t *palette = NULL;
    if (standardFunc(rwSTANDARDRASTERLOCKPALETTE)((void *)&palette, raster, lockMode) == 0) {
        return NULL;
    }
    return palette;
}

BMX_ADDR(0x004dd1d0)
RwRaster *RwRasterUnlockPalette(RwRaster *raster)
{
    if ((raster->privateFlags & rwRASTERPALETTELOCKED) == 0) {
        reportError((int32_t)0x80000015);
        return NULL;
    }
    standardFunc(rwSTANDARDRASTERUNLOCKPALETTE)(NULL, raster, 0);
    raster->privateFlags &= (uint8_t)~rwRASTERPALETTELOCKED;
    return raster;
}

/* ---- textures and dictionaries ---------------------------------------------------------------------- */

BMX_ADDR(0x004df520)
int RwTextureSetReadCallBack(RwTextureCallBackRead callBack)
{
    textureGlobals()->readCallBack = callBack;
    return 1;
}

BMX_ADDR(0x004df540)
RwTextureCallBackRead RwTextureGetReadCallBack(void)
{
    return textureGlobals()->readCallBack;
}

BMX_ADDR(0x004df550)
int RwTextureSetFindCallBack(RwTextureCallBackFind callBack)
{
    textureGlobals()->findCallBack = callBack;
    return 1;
}

BMX_ADDR(0x004df570)
RwTextureCallBackFind RwTextureGetFindCallBack(void)
{
    return textureGlobals()->findCallBack;
}

BMX_ADDR(0x004df580)
int RwTextureSetMipmapping(int32_t enable)
{
    textureGlobals()->mipmapping = enable;
    return 1;
}

/* Gives the texture a raster (any type but a plain image or z buffer); NULL clears it. */
BMX_ADDR(0x004df5f0)
RwTexture *RwTextureSetRaster(RwTexture *texture, RwRaster *raster)
{
    if (raster == NULL) {
        texture->raster = NULL;
        return texture;
    }
    if (raster->type == rwRASTERTYPETEXTURE || raster->type == rwRASTERTYPECAMERATEXTURE ||
        raster->type == rwRASTERTYPECAMERA) {
        if (standardFunc(rwSTANDARDTEXTURESETRASTER)(texture, raster, 0) != 0) {
            texture->stamp++;
            return texture;
        }
    }
    return NULL;
}

BMX_ADDR(0x004df650)
RwRaster *RwTextureGetRaster(const RwTexture *texture)
{
    return texture->raster;
}

/* A new, empty dictionary at the head of the list of all dictionaries. */
BMX_ADDR(0x004df670)
RwTexDictionary *RwTexDictionaryCreate(void)
{
    RwTextureGlobals *globals = textureGlobals();
    RwTexDictionary *dict = RwEngineInstance->freeListAlloc(globals->dictFreeList);
    if (dict == NULL) {
        return NULL;
    }
    dict->object.type = 6;
    dict->object.parent = NULL;
    dict->object.privateFlags = 0;
    dict->object.flags = 0;
    dict->object.subType = 0;
    dict->lInInstance.next = globals->dictList.next;
    dict->lInInstance.prev = &globals->dictList;
    globals->dictList.next->prev = &dict->lInInstance;
    globals->dictList.next = &dict->lInInstance;
    dict->texturesInDict.prev = &dict->texturesInDict;
    dict->texturesInDict.next = &dict->texturesInDict;
    _rwPluginRegistryInitObject(&rwTexDictionaryRegistry, dict);
    return dict;
}

static void linkOut(RwLLLink *link)
{
    link->prev->next = link->next;
    link->next->prev = link->prev;
}

/*
 * Drops one reference; at zero the texture leaves its dictionary and loses its raster, names and memory.
 * This is the body the original enters at 0x4df764 with the texture in EAX (RwTextureDestroy falls into it).
 */
static void textureRelease(RwTexture *texture)
{
    texture->refCount--;
    if (texture->refCount != 0) {
        return;
    }
    _rwPluginRegistryDeInitObject(&rwTextureRegistry, texture);
    if (texture->dict != NULL) {
        linkOut(&texture->lInDictionary);
    }
    if (texture->raster != NULL) {
        RwRasterDestroy(texture->raster);
        texture->raster = NULL;
    }
    if (texture->name != NULL) {
        RwEngineInstance->memoryFuncs.rwfree(texture->name);
    }
    if (texture->mask != NULL) {
        RwEngineInstance->memoryFuncs.rwfree(texture->mask);
    }
    RwEngineInstance->freeListFree(textureGlobals()->textureFreeList, texture);
}

/* Releases the texture until it is gone (a reference count of one or less takes the final release). */
BMX_ADDR(0x004df730)
RwTexture *RwTextureDestroyAll(RwTexture *texture)
{
    while (texture->refCount > 1) {
        textureRelease(texture);
    }
    textureRelease(texture);
    return texture;
}

/* Drops one reference of the texture. */
BMX_ADDR(0x004df760)
int RwTextureDestroy(RwTexture *texture)
{
    textureRelease(texture);
    return 1;
}

/* Destroys the dictionary and every texture in it, whatever their reference counts. */
BMX_ADDR(0x004df810)
int RwTexDictionaryDestroy(RwTexDictionary *dictionary)
{
    RwTextureGlobals *globals = textureGlobals();
    if (dictionary == globals->currentDict) {
        globals->currentDict = NULL;
    }
    RwLLLink *head = &dictionary->texturesInDict;
    RwLLLink *link = head->next;
    while (link != head) {
        RwTexture *texture = (RwTexture *)((uint8_t *)link - offsetof(RwTexture, lInDictionary));
        RwLLLink *next = link->next;
        if (RwTextureDestroyAll(texture) == NULL) {
            break;
        }
        link = next;
    }
    _rwPluginRegistryDeInitObject(&rwTexDictionaryRegistry, dictionary);
    linkOut(&dictionary->lInInstance);
    RwEngineInstance->freeListFree(textureGlobals()->dictFreeList, dictionary);
    return 1;
}

/* A texture around the raster with one reference, linear filter and wrap addressing, and no name. */
BMX_ADDR(0x004df910)
RwTexture *RwTextureCreate(RwRaster *raster)
{
    RwTexture *texture = RwEngineInstance->freeListAlloc(textureGlobals()->textureFreeList);
    if (texture == NULL) {
        return NULL;
    }
    texture->addressing = 1;
    texture->filterMode = 1;
    texture->refCount = 1;
    texture->raster = raster;
    texture->dict = NULL;
    texture->name = NULL;
    texture->mask = NULL;
    _rwPluginRegistryInitObject(&rwTextureRegistry, texture);
    return texture;
}

BMX_ADDR(0x004df980)
RwTexture *RwTextureAddRef(RwTexture *texture)
{
    texture->refCount++;
    return texture;
}

BMX_ADDR(0x004df990)
const char *RwTextureGetName(const RwTexture *texture)
{
    return texture->name;
}

/* Replaces the texture's name by a copy; NULL (with the old name already freed) if memory runs out. */
BMX_ADDR(0x004df9b0)
RwTexture *RwTextureSetName(RwTexture *texture, const char *name)
{
    if (texture->name != NULL) {
        RwEngineInstance->memoryFuncs.rwfree(texture->name);
    }
    texture->stamp++;
    texture->name = RwEngineInstance->memoryFuncs.rwmalloc(rwStrlen(name) + 1);
    if (texture->name == NULL) {
        reportNoMemory((int32_t)(rwStrlen(name) + 1));
        return NULL;
    }
    rwStrcpy(texture->name, name);
    return texture;
}

/* Puts the texture at the head of the dictionary's list, taking it out of the dictionary it was in. */
static void dictionaryInsert(RwTexDictionary *dictionary, RwTexture *texture)
{
    if (texture->dict != NULL) {
        linkOut(&texture->lInDictionary);
    }
    texture->dict = dictionary;
    texture->lInDictionary.prev = &dictionary->texturesInDict;
    texture->lInDictionary.next = dictionary->texturesInDict.next;
    dictionary->texturesInDict.next->prev = &texture->lInDictionary;
    dictionary->texturesInDict.next = &texture->lInDictionary;
    texture->stamp++;
    dictionary->stamp++;
}

BMX_ADDR(0x004dfb20)
RwTexture *RwTexDictionaryAddTexture(RwTexDictionary *dictionary, RwTexture *texture)
{
    dictionaryInsert(dictionary, texture);
    return texture;
}

BMX_ADDR(0x004dfc80)
RwTexDictionary *RwTexDictionarySetCurrent(RwTexDictionary *dictionary)
{
    textureGlobals()->currentDict = dictionary;
    return dictionary;
}

/* The original's name comparison: equal ignoring the case of ASCII letters (chars are signed). */
static int namesMatch(const char *a, const char *b)
{
    for (;;) {
        int ca = (int)(signed char)*a;
        int cb = (int)(signed char)*b;
        if (ca == 0) {
            return cb == 0;
        }
        if (cb == 0) {
            return 0;
        }
        if (ca >= 'a' && ca <= 'z') {
            ca -= 0x20;
        }
        if (cb >= 'a' && cb <= 'z') {
            cb -= 0x20;
        }
        if (ca != cb) {
            return 0;
        }
        a++;
        b++;
    }
}

static RwTexture *findInDictionary(RwTexDictionary *dictionary, const char *name)
{
    RwLLLink *head = &dictionary->texturesInDict;
    for (RwLLLink *link = head->next; link != head; link = link->next) {
        RwTexture *texture = (RwTexture *)((uint8_t *)link - offsetof(RwTexture, lInDictionary));
        if (texture->name != NULL && namesMatch(texture->name, name)) {
            return texture;
        }
    }
    return NULL;
}

/*
 * Finds a texture by name (case-insensitive) in the current dictionary, or in every dictionary when there
 * is none, and takes a reference; otherwise the read callback loads it and it joins the current dictionary.
 */
BMX_ADDR(0x004dfcb0)
RwTexture *RwTextureRead(const char *name, const char *maskName)
{
    RwTextureGlobals *globals = textureGlobals();
    RwTexture *found = NULL;
    if (globals->currentDict != NULL) {
        found = findInDictionary(globals->currentDict, name);
    } else {
        for (RwLLLink *link = globals->dictList.next; link != &globals->dictList && found == NULL;
             link = link->next) {
            found = findInDictionary(
                (RwTexDictionary *)((uint8_t *)link - offsetof(RwTexDictionary, lInInstance)), name);
        }
    }
    if (found != NULL) {
        found->refCount++;
        return found;
    }
    RwTexture *texture = globals->readCallBack(name, maskName);
    if (texture == NULL) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(8, name, maskName != NULL ? maskName : "(null)");
        RwErrorSet(&error);
        return NULL;
    }
    RwTexDictionary *current = textureGlobals()->currentDict;
    if (current != NULL) {
        dictionaryInsert(current, texture);
    }
    return texture;
}

BMX_ADDR(0x004dff70)
RwTexture *RwTextureSetFilterMode(RwTexture *texture, int32_t mode)
{
    texture->filterMode = mode;
    texture->stamp++;
    return texture;
}

BMX_ADDR(0x004dffa0)
RwTexture *RwTextureSetAddressing(RwTexture *texture, int32_t mode)
{
    texture->addressing = mode;
    texture->stamp++;
    return texture;
}

/* ---- images ----------------------------------------------------------------------------------------- */

/* An image without pixels (RwImageAllocatePixels adds them). */
BMX_ADDR(0x004e38e0)
RwImage *RwImageCreate(int32_t width, int32_t height, int32_t depth)
{
    RwImage *image = RwEngineInstance->freeListAlloc(imageGlobals()->imageFreeList);
    if (image == NULL) {
        return NULL;
    }
    image->stamp = 0;
    image->flags = 0;
    image->palette = NULL;
    image->pixels = NULL;
    image->depth = depth;
    image->height = height;
    image->width = width;
    _rwPluginRegistryInitObject(&rwImageRegistry, image);
    return image;
}

/* Gives the image's pixels back to the heap and clears the ownership flag. */
static void releasePixels(RwImage *image)
{
    RwEngineInstance->memoryFuncs.rwfree(image->pixels);
    image->flags &= ~(uint32_t)rwIMAGEOWNSPIXELS;
    image->stamp++;
    image->pixels = NULL;
    image->palette = NULL;
}

/* Destroys the image and the pixels it owns (reporting an error if it claims to own none). */
BMX_ADDR(0x004e3960)
int RwImageDestroy(RwImage *image)
{
    if ((image->flags & rwIMAGEOWNSPIXELS) != 0) {
        if (image->pixels != NULL) {
            releasePixels(image);
        } else {
            reportError((int32_t)0x8000000c);
        }
    }
    _rwPluginRegistryDeInitObject(&rwImageRegistry, image);
    RwEngineInstance->freeListFree(imageGlobals()->imageFreeList, image);
    return 1;
}

BMX_ADDR(0x004e3a60)
int32_t RwImageGetWidth(const RwImage *image)
{
    return image->width;
}

BMX_ADDR(0x004e3a70)
int32_t RwImageGetHeight(const RwImage *image)
{
    return image->height;
}

BMX_ADDR(0x004e3a80)
int32_t RwImageGetDepth(const RwImage *image)
{
    return image->depth;
}

BMX_ADDR(0x004e3a90)
uint8_t *RwImageGetPixels(const RwImage *image)
{
    return image->pixels;
}

BMX_ADDR(0x004e3aa0)
uint8_t *RwImageGetPalette(const RwImage *image)
{
    return image->palette;
}

/*
 * Allocates the pixels (rows padded to four bytes) and, for depths 4 and 8, the palette behind them. NULL
 * if the image already has pixels or memory runs out.
 */
BMX_ADDR(0x004e3ab0)
RwImage *RwImageAllocatePixels(RwImage *image)
{
    if ((image->flags & rwIMAGEOWNSPIXELS) != 0 || image->pixels != NULL) {
        return NULL;
    }
    int32_t paletteBytes = 0;
    if (image->depth == 4) {
        paletteBytes = 0x40;
    } else if (image->depth == 8) {
        paletteBytes = 0x400;
    }
    const int32_t stride = ((((image->depth + 7) >> 3) * image->width) + 3) & ~3;
    const int32_t pixelBytes = stride * image->height;
    image->stride = stride;
    const int32_t total = paletteBytes + pixelBytes;
    image->pixels = RwEngineInstance->memoryFuncs.rwmalloc((size_t)total);
    if (image->pixels == NULL) {
        reportNoMemory(total);
        return NULL;
    }
    image->palette = image->depth == 4 || image->depth == 8 ? image->pixels + pixelBytes : NULL;
    image->flags |= rwIMAGEOWNSPIXELS;
    image->stamp++;
    return image;
}

BMX_ADDR(0x004e3b90)
RwImage *RwImageFreePixels(RwImage *image)
{
    if ((image->flags & rwIMAGEOWNSPIXELS) == 0 || image->pixels == NULL) {
        reportError((int32_t)0x8000000c);
        return NULL;
    }
    releasePixels(image);
    return image;
}

/* Remembers a copy of the image search path (the buffer only ever grows). */
BMX_ADDR(0x004e4420)
const char *RwImageSetPath(const char *path)
{
    const int32_t size = (int32_t)rwStrlen(path) + 1;
    RwImageGlobals *globals = imageGlobals();
    if (size > globals->pathSize) {
        char *grown = RwEngineInstance->memoryFuncs.rwrealloc(globals->path, (size_t)size);
        if (grown == NULL) {
            reportNoMemory(size);
            return NULL;
        }
        imageGlobals()->path = grown;
        imageGlobals()->pathSize = size;
    }
    memcpy(imageGlobals()->path, path, (size_t)size);
    return path;
}

/*
 * Registers the reader and writer for files with the extension (without the dot), creating the format
 * entry the first time; a NULL callback leaves the registered one alone. 0 if memory runs out.
 */
BMX_ADDR(0x004e4a30)
int RwImageRegisterImageFormat(const char *extension, RwImageCallBackRead read, RwImageCallBackWrite write)
{
    char key[0x14];
    key[0] = '.';
    rwStrncpy(&key[1], extension, 0x12);
    key[0x13] = '\0';
    rwStrlwr(key);
    RwImageFormat *format = NULL;
    for (RwImageFormat *entry = imageGlobals()->formats; entry != NULL; entry = entry->next) {
        if (rwStrcmp(entry->extension, key) == 0) {
            format = entry;
            break;
        }
    }
    if (format == NULL) {
        format = RwEngineInstance->freeListAlloc(imageGlobals()->formatFreeList);
        if (format == NULL) {
            return 0;
        }
        memcpy(format->extension, key, sizeof key);
        format->extensionUpper[0] = '.';
        rwStrncpy(&format->extensionUpper[1], extension, 0x12);
        format->extensionUpper[0x13] = '\0';
        rwStrupr(format->extensionUpper);
        format->write = NULL;
        format->read = NULL;
        format->next = imageGlobals()->formats;
        imageGlobals()->formats = format;
    }
    if (read != NULL) {
        format->read = read;
    }
    if (write != NULL) {
        format->write = write;
    }
    return 1;
}

/* Converts a device pixel to RGBA through the device's PixelToRGB; returns the destination. */
BMX_ADDR(0x004e5330)
void *RwRGBAFromPixel(void *rgba, uint32_t pixel, int32_t format)
{
    uint32_t value = pixel;
    standardFunc(rwSTANDARDPIXELTORGB)(rgba, &value, format);
    return rgba;
}

/*
 * The double to byte conversion MSVC inlines for (uint8_t)(value): zero below 1.0, otherwise the mantissa
 * shifted by the exponent (the shift count wraps at 32, like SHR CL), negated for negative values.
 */
static uint8_t truncateToByte(double value)
{
    uint64_t bits;
    memcpy(&bits, &value, sizeof bits);
    const uint32_t high = (uint32_t)(bits >> 32);
    const uint32_t low = (uint32_t)bits;
    const uint32_t exponent = (high >> 20) & 0x7ff;
    if (exponent <= 0x3fe) {
        return 0;
    }
    const uint32_t mantissa = (high << 11) | (low >> 21) | 0x80000000U;
    uint32_t result = mantissa >> ((0x41eU - exponent) & 31);
    if ((int32_t)high < 0) {
        result = 0U - result;
    }
    return (uint8_t)result;
}

/*
 * Builds the two gamma tables (value ^ (1 / gamma) and value ^ gamma, scaled to bytes, entry 0 fixed at 0).
 * pow comes from the exe's CRT, and the exponent for the first table is the double 1 / gamma.
 */
BMX_ADDR(0x004e5570)
int RwImageSetGamma(float gamma)
{
    RwImageGlobals *globals = imageGlobals();
    const double inverse = (double)1.0F / (double)gamma;
    globals->gamma = gamma;
    globals->gammaUp[0] = 0;
    globals->gammaDown[0] = 0;
    for (int i = 1; i < 0x100; i++) {
        const double x = (double)i * (double)0x1.010102p-8F;
        const uint8_t up = truncateToByte(crtPow(x, inverse) * 255.0 + 0.5);
        imageGlobals()->gammaUp[i] = up;
        const uint8_t down = truncateToByte(crtPow(x, (double)gamma) * 255.0 + 0.5);
        imageGlobals()->gammaDown[i] = down;
    }
    return 1;
}

/* Marks the raster as gamma corrected when the image is, after the device copied the pixels over. */
BMX_ADDR(0x004e7f70)
RwRaster *RwRasterSetFromImage(RwRaster *raster, RwImage *image)
{
    if (standardFunc(rwSTANDARDRASTERSETIMAGE)(raster, image, 0) == 0) {
        return NULL;
    }
    if ((image->flags & rwIMAGEGAMMACORRECTED) != 0) {
        raster->privateFlags |= rwRASTERGAMMACORRECTED;
    }
    return raster;
}

/* ---- BMP reader ------------------------------------------------------------------------------------- */

/*
 * Converts one scan line of a BMP (`source`, srcDepth bits per pixel) into row `row` of the image. The key
 * is the source depth over the image depth (1/4/8 bit sources into 4/8 bit images keep their indices or are
 * looked up in the palette for 32 bit images; 16, 24 and 32 bit sources become RGBA). Bit and nibble
 * unpackers fetch the next source byte after the last pixel of a byte, as the original does. `flags` set:
 * 24 bit data is not swapped to RGB. 0 (and an error) for an unsupported combination.
 */
BMX_ADDR(0x004e6440)
__attribute__((regparm(3))) int BMPImageSetSpan(RwImage *image, int32_t row, const uint8_t *source,
                                                int32_t reserved0, int32_t reserved1, int32_t reserved2,
                                                int32_t srcDepth, int32_t flags)
{
    (void)reserved0;
    (void)reserved1;
    (void)reserved2;
    uint8_t *dst = image->pixels + row * image->stride;
    uint32_t count = (uint32_t)image->width;
    const uint8_t *palette = image->palette;
    const uint32_t key = ((uint32_t)srcDepth << 8) | (uint32_t)image->depth;
    const uint8_t *src = source;
    switch (key) {
    case 0x104:
    case 0x108: {
        uint32_t current = *src++;
        int shift = 7;
        for (; count != 0; count--) {
            *dst++ = (uint8_t)((current >> shift) & 1);
            if (--shift < 0) {
                current = *src++;
                shift = 7;
            }
        }
        break;
    }
    case 0x404:
    case 0x408: {
        uint32_t current = *src++;
        int shift = 4;
        for (; count != 0; count--) {
            *dst++ = (uint8_t)((current >> shift) & 0xf);
            shift -= 4;
            if (shift < 0) {
                current = *src++;
                shift = 4;
            }
        }
        break;
    }
    case 0x808:
        memcpy(dst, src, count);
        break;
    case 0x120: {
        uint32_t current = *src++;
        int shift = 7;
        for (; count != 0; count--) {
            memcpy(dst, palette + ((current >> shift) & 1) * 4, 4);
            dst += 4;
            if (--shift < 0) {
                current = *src++;
                shift = 7;
            }
        }
        break;
    }
    case 0x420: {
        uint32_t current = *src++;
        int shift = 4;
        for (; count != 0; count--) {
            memcpy(dst, palette + ((current >> shift) & 0xf) * 4, 4);
            dst += 4;
            shift -= 4;
            if (shift < 0) {
                current = *src++;
                shift = 4;
            }
        }
        break;
    }
    case 0x820:
        for (; count != 0; count--) {
            memcpy(dst, palette + (uint32_t)*src++ * 4, 4);
            dst += 4;
        }
        break;
    case 0xf20:
        for (; count != 0; count--) {
            const uint32_t pixel = (uint32_t)src[0] | ((uint32_t)src[1] << 8);
            src += 2;
            dst[0] = (uint8_t)((pixel >> 7) & 0xf8);
            dst[1] = (uint8_t)((pixel >> 2) & 0xf8);
            dst[2] = (uint8_t)((pixel << 3) & 0xf8);
            dst[3] = 0xff;
            dst += 4;
        }
        break;
    case 0x1020:
        for (; count != 0; count--) {
            const uint32_t pixel = (uint32_t)src[0] | ((uint32_t)src[1] << 8);
            src += 2;
            dst[0] = (uint8_t)((pixel >> 8) & 0xf8);
            dst[1] = (uint8_t)((pixel >> 3) & 0xfc);
            dst[2] = (uint8_t)((pixel << 3) & 0xf8);
            dst[3] = 0xff;
            dst += 4;
        }
        break;
    case 0x1820:
        for (; count != 0; count--) {
            if (flags != 0) {
                dst[0] = src[0];
                dst[1] = src[1];
                dst[2] = src[2];
            } else {
                dst[2] = src[0];
                dst[1] = src[1];
                dst[0] = src[2];
            }
            dst[3] = 0xff;
            src += 3;
            dst += 4;
        }
        break;
    case 0x2020:
        for (; count != 0; count--) {
            dst[0] = src[0];
            dst[1] = src[1];
            dst[2] = src[2];
            dst[3] = 0xff;
            src += 4;
            dst += 4;
        }
        break;
    default:
        reportError((int32_t)0x8000000f);
        return 0;
    }
    return 1;
}

enum { kBmpMagic = 0x4d42, kBmpCoreHeader = 12, kBmpInfoHeader = 40, kBmpScratch = 0x408 };

static uint32_t readU32(const uint8_t *p)
{
    uint32_t v;
    memcpy(&v, p, sizeof v);
    return v;
}

static int32_t readI16(const uint8_t *p)
{
    int16_t v;
    memcpy(&v, p, sizeof v);
    return v;
}

/*
 * Reads a Windows or OS/2 BMP (1, 4, 8, 16, 24 or 32 bits; RLE8 for 8 bit) through a file stream into an
 * RwImage of depth 4, 8 or 32 (1 bit images get depth 4). The stream layer reads the 14 byte file header
 * together with the first four bytes of the info header, so the scratch buffer mirrors the original's frame:
 * "BM" at 0, the rest of the file header at 6, the info header from 4 on (overwriting the header size,
 * which is kept apart). Failures free what exists, except the paths that leak the image in the original
 * (a short palette, an unknown header size or compression). A header that does not fit the scratch buffer
 * overflows the original's frame; here it fails.
 */
BMX_ADDR(0x004e6d90)
RwImage *RtBMPImageRead(const char *name)
{
    uint8_t buf[kBmpScratch] = {0};
    RwStream *stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, name);
    if (stream == NULL) {
        return NULL;
    }
    if (RwStreamRead(stream, buf, 4) != 4) {
        goto closeOnly;
    }
    RwMemNative32(buf, 4);
    if (readI16(buf) != kBmpMagic) {
        goto closeOnly;
    }
    if (RwStreamRead(stream, &buf[6], 0xe) != 0xe) {
        goto closeOnly;
    }
    const uint32_t dataOffset = readU32(&buf[0xc]);
    const uint32_t headerSize = readU32(&buf[0x10]);
    const uint32_t infoBytes = headerSize - 4;
    if (infoBytes > sizeof buf - 4 || RwStreamRead(stream, &buf[4], infoBytes) != infoBytes) {
        goto closeOnly;
    }
    uint32_t consumed = infoBytes + 0x12;
    int32_t width;
    int32_t height;
    int32_t bitCount;
    int32_t compression;
    int32_t paletteEntries;
    if (headerSize == kBmpCoreHeader) {
        width = readI16(&buf[4]);
        height = readI16(&buf[6]);
        bitCount = readI16(&buf[10]);
        compression = 0;
        paletteEntries = (int32_t)(1U << (bitCount & 31));
    } else {
        width = (int32_t)readU32(&buf[4]);
        height = (int32_t)readU32(&buf[8]);
        compression = (int32_t)readU32(&buf[0x10]);
        if (compression == 2) {
            reportError((int32_t)0x8000000c);
            goto closeOnly;
        }
        bitCount = readI16(&buf[0xe]);
        const int32_t colorsUsed = (int32_t)readU32(&buf[0x20]);
        const int32_t maxEntries = (int32_t)(1U << (bitCount & 31));
        paletteEntries = colorsUsed > 0 && colorsUsed <= maxEntries ? colorsUsed : maxEntries;
    }
    int32_t imageDepth = 0x20;
    if (bitCount == 1 || bitCount == 4) {
        imageDepth = 4;
    } else if (bitCount == 8) {
        imageDepth = 8;
    }
    RwImage *image = RwImageCreate(width, height, imageDepth);
    if (image == NULL) {
        goto closeOnly;
    }
    if (RwImageAllocatePixels(image) == NULL) {
        goto destroyImage;
    }
    if (bitCount <= 8) {
        uint8_t *palette = image->palette;
        if (palette == NULL) {
            goto destroyImage; /* a bit count the image has no palette for: the original writes through NULL
                                */
        }
        if (headerSize == kBmpCoreHeader) {
            const uint32_t bytes = (uint32_t)paletteEntries * 3;
            if (bytes > sizeof buf - 4 || RwStreamRead(stream, &buf[4], bytes) != bytes) {
                goto closeOnly; /* leaks the image */
            }
            consumed = bytes + 0x12 + infoBytes;
            for (int32_t i = 0; i < paletteEntries; i++) {
                palette[i * 4] = buf[4 + i * 3 + 2];
                palette[i * 4 + 1] = buf[4 + i * 3 + 1];
                palette[i * 4 + 2] = buf[4 + i * 3];
                palette[i * 4 + 3] = 0xff;
            }
        } else if (headerSize == kBmpInfoHeader) {
            const uint32_t bytes = (uint32_t)paletteEntries * 4;
            if (bytes > sizeof buf - 4 || RwStreamRead(stream, &buf[4], bytes) != bytes) {
                goto closeOnly; /* leaks the image */
            }
            consumed = bytes + 0x12 + infoBytes;
            for (int32_t i = 0; i < paletteEntries; i++) {
                palette[i * 4] = buf[4 + i * 4 + 2];
                palette[i * 4 + 1] = buf[4 + i * 4 + 1];
                palette[i * 4 + 2] = buf[4 + i * 4];
                palette[i * 4 + 3] = 0xff;
            }
        } else {
            reportError((int32_t)0x8000000c);
            goto closeOnly; /* leaks the image */
        }
    }
    if (RwStreamSkip(stream, dataOffset - consumed) == NULL) {
        goto destroyImage;
    }
    {
        const int32_t sourceBits = bitCount * width;
        const int32_t bias = (int32_t)((uint32_t)((sourceBits + 7) >> 2) >> 29);
        const int32_t rowBytes = (((bias + sourceBits + 7) >> 3) + 3) & ~3;
        const int32_t bufferBytes = ((width + 7) & ~7) * 3;
        uint8_t *row = RwEngineInstance->memoryFuncs.rwmalloc((size_t)bufferBytes);
        if (row == NULL) {
            reportNoMemory(bufferBytes);
            goto destroyImage;
        }
        if (compression == 0 || bitCount == 0x18) {
            for (int32_t i = 0; i < height; i++) {
                if (RwStreamRead(stream, row, (uint32_t)rowBytes) != (uint32_t)rowBytes) {
                    goto freeRow;
                }
                BMPImageSetSpan(image, height - 1 - i, row, 0, 0, 0, bitCount, 0);
            }
        } else if (compression == 1) {
            int32_t y = 0;
            while (y < height) {
                uint32_t x = 0;
                for (;;) {
                    uint8_t pair[2];
                    if (RwStreamRead(stream, pair, 2) != 2) {
                        goto freeRow;
                    }
                    if (pair[0] != 0) {
                        memset(row + x, pair[1], pair[0]);
                        x += pair[0];
                        continue;
                    }
                    const uint32_t code = pair[1];
                    if (code > 2) {
                        if (RwStreamRead(stream, row + x, code) != code) {
                            goto freeRow;
                        }
                        x += code;
                        if ((code & 1) != 0 && RwStreamRead(stream, pair, 1) != 1) {
                            goto freeRow;
                        }
                        continue;
                    }
                    if (code == 2) {
                        goto freeRow;
                    }
                    if (code == 1) {
                        y = height - 1;
                    }
                    BMPImageSetSpan(image, height - 1 - y, row, 0, 0, 0, bitCount, 1);
                    y++;
                    break;
                }
            }
        } else {
            reportError((int32_t)0x8000000c);
            goto freeRow;
        }
        RwEngineInstance->memoryFuncs.rwfree(row);
        RwStreamClose(stream, NULL);
        return image;
    freeRow:
        RwEngineInstance->memoryFuncs.rwfree(row);
    }
destroyImage:
    RwImageDestroy(image);
closeOnly:
    RwStreamClose(stream, NULL);
    return NULL;
}

/* ---- PNG reader ------------------------------------------------------------------------------------- */

enum {
    kPngColorGray = 0,
    kPngColorRgb = 2,
    kPngColorPalette = 3,
    kPngColorGrayAlpha = 4,
    kPngColorRgba = 6,
    kPngInfoPlte = 0x08,
    kPngInfoTrns = 0x10,
    kPngFillerAfter = 1,
};

/*
 * Reads a PNG through a file stream into an RwImage: palette images and grey images keep depth 4 or 8 (a
 * palette plus transparency), everything else becomes 32 bit RGBA. libpng does the decoding; it reads through
 * the stream (the read callback), takes its memory from the engine's memory interface, and reports errors by
 * longjmp to the setjmp in this function, which then frees the row table and the png structs (the image is
 * leaked, as in the original). Locals the error path needs are volatile.
 */
BMX_ADDR(0x005068a0)
RwImage *RtPNGImageRead(const char *name)
{
    if (name == NULL) {
        RwError error;
        error.pluginID = 0x10d;
        error.errorCode = _rwerror(rwERR_NULLP);
        RwErrorSet(&error);
        return NULL;
    }
    RwStream *stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, name);
    if (stream == NULL) {
        return NULL;
    }
    void *volatile png = NULL;
    void *volatile info = NULL;
    uint8_t **volatile rows = NULL;
    png = png_create_read_struct_2("1.0.5", NULL, NULL, NULL, NULL, RW_CB(void *, PNG_MALLOC_CALLBACK),
                                   RW_CB(void *, PNG_FREE_CALLBACK));
    if (png == NULL) {
        RwStreamClose(stream, NULL);
        return NULL;
    }
    info = png_create_info_struct(png);
    if (info == NULL) {
        RwStreamClose(stream, NULL);
        png_destroy_read_struct((void **)&png, NULL, NULL);
        return NULL;
    }
    if (_setjmp3(png, 0) != 0) {
        if (rows != NULL) {
            RwOsGetMemoryInterface()->rwfree((void *)rows);
            rows = NULL;
        }
        RwStreamClose(stream, NULL);
        png_destroy_read_struct((void **)&png, (void **)&info, NULL);
        return NULL;
    }
    png_set_read_fn(png, stream, RW_CB(void *, PNG_READ_CALLBACK));
    png_read_info(png, info);
    uint32_t width = 0;
    uint32_t height = 0;
    int bitDepth = 0;
    int colorType = 0;
    int interlace = 0;
    png_get_IHDR(png, info, &width, &height, &bitDepth, &colorType, &interlace, NULL, NULL);
    png_set_strip_16(png);
    int32_t depth = 0;
    switch (colorType) {
    case kPngColorPalette:
        depth = 8;
        if (bitDepth < 8) {
            png_set_packing(png);
            depth = 4;
        }
        break;
    case kPngColorGray:
        depth = 8;
        if (bitDepth < 8) {
            png_set_gray_1_2_4_to_8(png);
            depth = 4;
        }
        break;
    case kPngColorGrayAlpha:
        depth = 0x20;
        png_set_gray_to_rgb(png);
        break;
    case kPngColorRgb:
        depth = 0x20;
        png_set_filler(png, 0xff, kPngFillerAfter);
        break;
    case kPngColorRgba:
        depth = 0x20;
        break;
    default:
        break;
    }
    RwImage *image = RwImageCreate((int32_t)width, (int32_t)height, depth);
    if (image != NULL) {
        if (RwImageAllocatePixels(image) == NULL) {
            RwImageDestroy(image);
            RwStreamClose(stream, NULL);
            return NULL;
        }
        uint8_t *pixels = RwImageGetPixels(image);
        uint8_t *palette = RwImageGetPalette(image);
        const int32_t stride = image->stride;
        rows = (uint8_t **)RwOsGetMemoryInterface()->rwmalloc((size_t)(height << 2));
        if (rows == NULL) {
            RwImageFreePixels(image);
            RwImageDestroy(image);
            RwStreamClose(stream, NULL);
            return NULL;
        }
        uint8_t *rowStart = pixels;
        for (int32_t i = 0; i < (int32_t)height; i++) {
            rows[i] = rowStart;
            rowStart += stride;
        }
        if ((depth == 4 || depth == 8) && palette != NULL) {
            for (int32_t i = 0; i < (1 << depth); i++) {
                palette[i * 4 + 3] = 0xff;
                palette[i * 4 + 2] = (uint8_t)i;
                palette[i * 4 + 1] = (uint8_t)i;
                palette[i * 4] = (uint8_t)i;
            }
            if (png_get_valid(png, info, kPngInfoPlte) != 0) {
                uint8_t *entries = NULL;
                int count = 0;
                png_get_PLTE(png, info, &entries, &count);
                for (int32_t i = 0; i < count; i++) {
                    palette[i * 4] = entries[i * 3];
                    palette[i * 4 + 1] = entries[i * 3 + 1];
                    palette[i * 4 + 2] = entries[i * 3 + 2];
                }
            }
            if (png_get_valid(png, info, kPngInfoTrns) != 0) {
                uint8_t *alpha = NULL;
                int count = 0;
                void *values = NULL;
                png_get_tRNS(png, info, &alpha, &count, &values);
                for (int32_t i = 0; i < count; i++) {
                    palette[i * 4 + 3] = alpha[i];
                }
            }
        }
        png_read_image(png, (uint8_t **)rows);
        png_read_end(png, info);
        png_destroy_read_struct((void **)&png, (void **)&info, NULL);
        png = NULL;
        info = NULL;
        RwOsGetMemoryInterface()->rwfree((void *)rows);
        rows = NULL;
    }
    RwStreamClose(stream, NULL);
    return image;
}
