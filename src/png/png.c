/*
 * A small PNG reader in place of the libpng 1.0.5 the original links (lib/libpng and lib/zlib in
 * data/functions-rw.csv, third-party code we do not port). It provides exactly the entry points
 * RtPNGImageRead (rw/texture.c) calls, with the same arguments and the same results for the transforms it
 * requests: strip_16, packing, gray_1_2_4_to_8, gray_to_rgb, filler, the PLTE / tRNS getters, and Adam7
 * deinterlacing inside png_read_image. Native exe only: each original entry point is served by ours
 * (bmxPngJumps, read by tools/gen_origmap.py), like the SDL3 driver's; the hook DLL keeps the original
 * libpng.
 *
 * Like libpng:
 * - the png struct starts with the MSVC jmp_buf RtPNGImageRead fills with _setjmp3, and the read function
 *   and its io pointer sit at 0x50 / 0x54 (the read callback, rwPngRead 0x506860, reads the stream there);
 * - every error (bad signature, CRC of a critical chunk, corrupt or truncated deflate data, short read)
 *   goes through png_error (0x507050, also replaced), which longjmps there with the CRT's _longjmp (in the
 *   native exe our src/native/crtasm.S version, in the original MSVC6's);
 * - all memory comes from the malloc / free callbacks passed to png_create_read_struct_2;
 * - all data comes through the read callback set with png_set_read_fn.
 * The inflater follows the structure of Mark Adler's puff.c (zlib licence): canonical Huffman decoding one
 * bit at a time, small and fast enough for the game's few PNG files.
 *
 * Differences on corrupt input only: an unknown filter type is an error here (libpng 1.0.5 warns and
 * zeroes), and ancillary chunks with a bad CRC are skipped silently (libpng warns, then skips them too).
 */
#include "png/png.h"

#include <stddef.h>

#include "compat/orig.h"

/* The CRT's longjmp (MSVC layout; in the native exe it jumps to bmxCrtLongjmp). */
ORIG_FN(0x0051e084, void, crtLongjmp, (void *jmpBuf, int value));

typedef void (*PngReadFn)(void *png, uint8_t *data, uint32_t length);
typedef void *(*PngMallocFn)(void *png, uint32_t size);
typedef void (*PngFreeFn)(void *png, void *block);
typedef void (*PngErrorFn)(void *png, const char *message);

enum {
    kColorGray = 0,
    kColorRgb = 2,
    kColorPalette = 3,
    kColorGrayAlpha = 4,
    kColorRgba = 6,
};

/* info->valid bits (PNG_INFO_*). */
enum { kInfoPlte = 0x08, kInfoTrns = 0x10 };

/* Requested transforms (subset of libpng's PNG_* transformation flags). */
enum {
    kXformPack = 0x0004,
    kXformExpand = 0x1000,
    kXformGrayToRgb = 0x4000,
    kXformFiller = 0x8000,
    kXformStrip16 = 0x0400,
};

/* png_color_16 */
typedef struct PngColor16 {
    uint8_t index;
    uint16_t red;
    uint16_t green;
    uint16_t blue;
    uint16_t gray;
} PngColor16;

typedef struct PngInfo {
    uint32_t width;
    uint32_t height;
    int bitDepth;
    int colorType;
    int interlace;
    uint32_t valid;
    int numPalette;
    int numTrans;
    PngColor16 transValues;
    uint8_t palette[256 * 3];
    uint8_t trans[256];
} PngInfo;

typedef struct PngStruct {
    uint32_t jmpBuf[16]; /* 0x00: MSVC jmp_buf, filled by RtPNGImageRead's _setjmp3 */
    PngErrorFn errorFn;  /* 0x40 */
    void *warningFn;     /* 0x44 */
    void *errorPtr;      /* 0x48 */
    void *writeFn;       /* 0x4c */
    PngReadFn readFn;    /* 0x50 */
    void *ioPtr;         /* 0x54: the RwStream rwPngRead reads from */
    void *memPtr;
    PngMallocFn mallocFn;
    PngFreeFn freeFn;
    uint32_t transforms;
    uint8_t filler;
    int fillerAfter;
    PngInfo header;       /* IHDR, PLTE, tRNS as read (copied to the caller's info) */
    uint32_t chunkLength; /* the chunk png_read_info / png_read_image stopped at */
    uint8_t chunkType[4];
    uint32_t crc;
    uint8_t *data; /* the concatenated IDAT data */
    uint32_t dataLength;
    uint8_t *image; /* the inflated, still filtered scanlines of all passes */
} PngStruct;

_Static_assert(sizeof(((PngStruct *)0)->jmpBuf) >= 5 * sizeof(void *), "__builtin_setjmp's 5 slots");
#if UINTPTR_MAX == 0xffffffffu
_Static_assert(offsetof(PngStruct, readFn) == 0x50 && offsetof(PngStruct, ioPtr) == 0x54,
               "rwPngRead reads the stream at 0x54");
#endif

/* ---- errors, memory, input ----------------------------------------------------------------------------- */

void *bmxPngIoPtr(void *png)
{
    return ((PngStruct *)png)->ioPtr;
}

/* libpng's png_error: the error callback (none in the game), then longjmp(jmpbuf, 1). */
void pngError(void *png, const char *message)
{
    PngStruct *p = png;
    if (p->errorFn != NULL) {
        p->errorFn(png, message);
    }
#if defined(__i386__)
    crtLongjmp(p->jmpBuf, 1);
#else
    /* x86_64: the builtin pair (rw/texture.c's RtPNGImageRead sets it with __builtin_setjmp). */
    __builtin_longjmp((void **)p->jmpBuf, 1);
#endif
    __builtin_unreachable();
}

static void *pngAlloc(PngStruct *p, uint32_t size)
{
    void *block = p->mallocFn(p, size);
    if (block == NULL) {
        pngError(p, "Out of Memory!");
    }
    return block;
}

static void pngRelease(PngStruct *p, void **block)
{
    if (*block != NULL) {
        p->freeFn(p, *block);
        *block = NULL;
    }
}

static uint32_t crcTable(uint32_t c)
{
    for (int k = 0; k < 8; k++) {
        c = (c & 1) ? 0xedb88320u ^ (c >> 1) : c >> 1;
    }
    return c;
}

static uint32_t crcUpdate(uint32_t crc, const uint8_t *data, uint32_t length)
{
    for (uint32_t i = 0; i < length; i++) {
        crc = crcTable((crc ^ data[i]) & 0xff) ^ (crc >> 8);
    }
    return crc;
}

static void readRaw(PngStruct *p, uint8_t *data, uint32_t length)
{
    if (p->readFn == NULL) {
        pngError(p, "Call to NULL read function");
    }
    p->readFn(p, data, length);
}

static uint32_t be32(const uint8_t *b)
{
    return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | b[3];
}

/* Next chunk header into chunkLength / chunkType; starts the CRC. */
static void readChunkHeader(PngStruct *p)
{
    uint8_t b[8];
    readRaw(p, b, 8);
    p->chunkLength = be32(b);
    if (p->chunkLength > 0x7fffffffu) {
        pngError(p, "Invalid chunk length.");
    }
    for (int i = 0; i < 4; i++) {
        p->chunkType[i] = b[4 + i];
    }
    p->crc = crcUpdate(0xffffffffu, p->chunkType, 4);
}

static void readChunkData(PngStruct *p, uint8_t *data, uint32_t length)
{
    readRaw(p, data, length);
    p->crc = crcUpdate(p->crc, data, length);
}

/* Reads the stored CRC; nonzero when it matches. */
static int readChunkCrc(PngStruct *p)
{
    uint8_t b[4];
    readRaw(p, b, 4);
    return be32(b) == (p->crc ^ 0xffffffffu);
}

static int isChunk(const PngStruct *p, const char *name)
{
    for (int i = 0; i < 4; i++) {
        if (p->chunkType[i] != (uint8_t)name[i]) {
            return 0;
        }
    }
    return 1;
}

/* Skips the rest of the current chunk (length bytes); a critical chunk with a bad CRC is an error. */
static void finishChunk(PngStruct *p, uint32_t length)
{
    uint8_t buffer[256];
    while (length > 0) {
        const uint32_t n = length < sizeof buffer ? length : (uint32_t)sizeof buffer;
        readChunkData(p, buffer, n);
        length -= n;
    }
    if (!readChunkCrc(p) && (p->chunkType[0] & 0x20) == 0) {
        pngError(p, "CRC error");
    }
}

/* ---- chunk handlers --------------------------------------------------------------------------------------
 */

static void handleIhdr(PngStruct *p)
{
    uint8_t b[13];
    if (p->chunkLength != 13) {
        pngError(p, "Invalid IHDR chunk");
    }
    readChunkData(p, b, 13);
    if (!readChunkCrc(p)) {
        pngError(p, "CRC error");
    }
    PngInfo *h = &p->header;
    h->width = be32(b);
    h->height = be32(b + 4);
    h->bitDepth = b[8];
    h->colorType = b[9];
    h->interlace = b[12];
    if (h->width == 0 || h->height == 0 || h->width > 0x7fffffffu || h->height > 0x7fffffffu) {
        pngError(p, "Invalid image size in IHDR");
    }
    const int bd = h->bitDepth;
    if (bd != 1 && bd != 2 && bd != 4 && bd != 8 && bd != 16) {
        pngError(p, "Invalid bit depth in IHDR");
    }
    const int ct = h->colorType;
    if (ct != kColorGray && ct != kColorRgb && ct != kColorPalette && ct != kColorGrayAlpha &&
        ct != kColorRgba) {
        pngError(p, "Invalid color type in IHDR");
    }
    if ((ct == kColorPalette && bd > 8) || (ct != kColorGray && ct != kColorPalette && bd < 8)) {
        pngError(p, "Invalid color type/bit depth combination in IHDR");
    }
    if (h->interlace > 1 || b[10] != 0 || b[11] != 0) {
        pngError(p, "Unknown interlace, compression or filter method in IHDR");
    }
}

static void handlePlte(PngStruct *p)
{
    PngInfo *h = &p->header;
    const uint32_t length = p->chunkLength;
    if ((h->valid & kInfoPlte) != 0 || length % 3 != 0 || length > 256 * 3) {
        pngError(p, "Invalid palette chunk");
    }
    readChunkData(p, h->palette, length);
    if (!readChunkCrc(p)) {
        pngError(p, "CRC error");
    }
    /* Kept only where RtPNGImageRead can use it (libpng also keeps it for RGB, unused there). */
    if (h->colorType == kColorPalette || h->colorType == kColorRgb || h->colorType == kColorRgba) {
        h->numPalette = (int)(length / 3);
        h->valid |= kInfoPlte;
    }
}

static void handleTrns(PngStruct *p)
{
    PngInfo *h = &p->header;
    const uint32_t length = p->chunkLength;
    uint8_t b[256];
    int ok = (h->valid & kInfoTrns) == 0;
    if (h->colorType == kColorPalette) {
        ok = ok && (h->valid & kInfoPlte) != 0 && length <= (uint32_t)h->numPalette && length > 0;
    } else if (h->colorType == kColorGray) {
        ok = ok && length == 2;
    } else if (h->colorType == kColorRgb) {
        ok = ok && length == 6;
    } else {
        ok = 0;
    }
    if (!ok || length > sizeof b) {
        finishChunk(p, length);
        return;
    }
    readChunkData(p, b, length);
    if (!readChunkCrc(p)) {
        return; /* ancillary: dropped */
    }
    if (h->colorType == kColorPalette) {
        for (uint32_t i = 0; i < length; i++) {
            h->trans[i] = b[i];
        }
        h->numTrans = (int)length;
    } else if (h->colorType == kColorGray) {
        h->transValues.gray = (uint16_t)((b[0] << 8) | b[1]);
        h->numTrans = 1;
    } else {
        h->transValues.red = (uint16_t)((b[0] << 8) | b[1]);
        h->transValues.green = (uint16_t)((b[2] << 8) | b[3]);
        h->transValues.blue = (uint16_t)((b[4] << 8) | b[5]);
        h->numTrans = 1;
    }
    h->valid |= kInfoTrns;
}

/* ---- inflate (RFC 1950 / 1951)
 * ----------------------------------------------------------------------------- */

typedef struct Inflate {
    const uint8_t *in;
    uint32_t inLength;
    uint32_t inPos;
    uint32_t bitBuf;
    int bitCount;
    uint8_t *out;
    uint32_t outLength;
    uint32_t outPos;
} Inflate;

typedef struct Huffman {
    uint16_t count[16];
    uint16_t symbol[288];
} Huffman;

enum { kInflateOk = 0, kInflateFull = 1, kInflateError = -1 };

static int needBits(Inflate *s, int need, uint32_t *value)
{
    while (s->bitCount < need) {
        if (s->inPos >= s->inLength) {
            return kInflateError;
        }
        s->bitBuf |= (uint32_t)s->in[s->inPos++] << s->bitCount;
        s->bitCount += 8;
    }
    *value = s->bitBuf & ((1u << need) - 1u);
    s->bitBuf >>= need;
    s->bitCount -= need;
    return kInflateOk;
}

/* Canonical code from code lengths; over-subscribed sets are an error, incomplete ones are allowed. */
static int buildHuffman(Huffman *h, const uint8_t *length, int n)
{
    uint16_t offset[16];
    for (int len = 0; len < 16; len++) {
        h->count[len] = 0;
    }
    for (int i = 0; i < n; i++) {
        h->count[length[i]]++;
    }
    int left = 1;
    for (int len = 1; len < 16; len++) {
        left = (left << 1) - h->count[len];
        if (left < 0) {
            return kInflateError;
        }
    }
    offset[1] = 0;
    for (int len = 1; len < 15; len++) {
        offset[len + 1] = (uint16_t)(offset[len] + h->count[len]);
    }
    for (int i = 0; i < n; i++) {
        if (length[i] != 0) {
            h->symbol[offset[length[i]]++] = (uint16_t)i;
        }
    }
    return kInflateOk;
}

static int decodeSymbol(Inflate *s, const Huffman *h)
{
    int code = 0;
    int first = 0;
    int index = 0;
    for (int len = 1; len < 16; len++) {
        uint32_t bit = 0;
        if (needBits(s, 1, &bit) != kInflateOk) {
            return kInflateError;
        }
        code |= (int)bit;
        const int count = h->count[len];
        if (code - first < count) {
            return h->symbol[index + (code - first)];
        }
        index += count;
        first = (first + count) << 1;
        code <<= 1;
    }
    return kInflateError;
}

static int putByte(Inflate *s, uint8_t b)
{
    if (s->outPos >= s->outLength) {
        return kInflateFull;
    }
    s->out[s->outPos++] = b;
    return kInflateOk;
}

static int inflateCodes(Inflate *s, const Huffman *lit, const Huffman *dist)
{
    static const uint16_t kLengthBase[29] = {3,  4,  5,  6,  7,  8,  9,  10, 11,  13,  15,  17,  19,  23, 27,
                                             31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
    static const uint8_t kLengthExtra[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2,
                                             2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
    static const uint16_t kDistBase[30] = {1,    2,    3,    4,    5,    7,    9,    13,    17,    25,
                                           33,   49,   65,   97,   129,  193,  257,  385,   513,   769,
                                           1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
    static const uint8_t kDistExtra[30] = {0, 0, 0, 0, 1, 1, 2, 2,  3,  3,  4,  4,  5,  5,  6,
                                           6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
    for (;;) {
        const int symbol = decodeSymbol(s, lit);
        if (symbol < 0) {
            return kInflateError;
        }
        if (symbol < 256) {
            const int r = putByte(s, (uint8_t)symbol);
            if (r != kInflateOk) {
                return r;
            }
            continue;
        }
        if (symbol == 256) {
            return kInflateOk;
        }
        const int li = symbol - 257;
        if (li >= 29) {
            return kInflateError;
        }
        uint32_t extra = 0;
        if (needBits(s, kLengthExtra[li], &extra) != kInflateOk) {
            return kInflateError;
        }
        const uint32_t length = kLengthBase[li] + extra;
        const int di = decodeSymbol(s, dist);
        if (di < 0 || di >= 30 || needBits(s, kDistExtra[di], &extra) != kInflateOk) {
            return kInflateError;
        }
        const uint32_t distance = kDistBase[di] + extra;
        if (distance > s->outPos) {
            return kInflateError;
        }
        for (uint32_t i = 0; i < length; i++) {
            const int r = putByte(s, s->out[s->outPos - distance]);
            if (r != kInflateOk) {
                return r;
            }
        }
    }
}

static int inflateStored(Inflate *s)
{
    s->bitBuf = 0;
    s->bitCount = 0;
    if (s->inPos + 4 > s->inLength) {
        return kInflateError;
    }
    const uint8_t *b = s->in + s->inPos;
    const uint32_t length = b[0] | ((uint32_t)b[1] << 8);
    if ((length ^ 0xffffu) != (b[2] | ((uint32_t)b[3] << 8))) {
        return kInflateError;
    }
    s->inPos += 4;
    for (uint32_t i = 0; i < length; i++) {
        if (s->inPos >= s->inLength) {
            return kInflateError;
        }
        const int r = putByte(s, s->in[s->inPos++]);
        if (r != kInflateOk) {
            return r;
        }
    }
    return kInflateOk;
}

static int inflateFixed(Inflate *s)
{
    Huffman lit;
    Huffman dist;
    uint8_t lengths[288];
    int i = 0;
    for (; i < 144; i++) {
        lengths[i] = 8;
    }
    for (; i < 256; i++) {
        lengths[i] = 9;
    }
    for (; i < 280; i++) {
        lengths[i] = 7;
    }
    for (; i < 288; i++) {
        lengths[i] = 8;
    }
    buildHuffman(&lit, lengths, 288);
    for (i = 0; i < 30; i++) {
        lengths[i] = 5;
    }
    buildHuffman(&dist, lengths, 30);
    return inflateCodes(s, &lit, &dist);
}

static int inflateDynamic(Inflate *s)
{
    static const uint8_t kOrder[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
    uint8_t lengths[320];
    Huffman lit;
    Huffman dist;
    uint32_t nlen = 0;
    uint32_t ndist = 0;
    uint32_t ncode = 0;
    if (needBits(s, 5, &nlen) != kInflateOk || needBits(s, 5, &ndist) != kInflateOk ||
        needBits(s, 4, &ncode) != kInflateOk) {
        return kInflateError;
    }
    nlen += 257;
    ndist += 1;
    ncode += 4;
    if (nlen > 286 || ndist > 30) {
        return kInflateError;
    }
    for (int i = 0; i < 19; i++) {
        uint32_t v = 0;
        if (i < (int)ncode && needBits(s, 3, &v) != kInflateOk) {
            return kInflateError;
        }
        lengths[kOrder[i]] = (uint8_t)v;
    }
    if (buildHuffman(&lit, lengths, 19) != kInflateOk) {
        return kInflateError;
    }
    uint32_t index = 0;
    while (index < nlen + ndist) {
        int symbol = decodeSymbol(s, &lit);
        if (symbol < 0) {
            return kInflateError;
        }
        if (symbol < 16) {
            lengths[index++] = (uint8_t)symbol;
            continue;
        }
        uint8_t value = 0;
        uint32_t repeat = 0;
        if (symbol == 16) {
            if (index == 0 || needBits(s, 2, &repeat) != kInflateOk) {
                return kInflateError;
            }
            value = lengths[index - 1];
            repeat += 3;
        } else if (symbol == 17) {
            if (needBits(s, 3, &repeat) != kInflateOk) {
                return kInflateError;
            }
            repeat += 3;
        } else {
            if (needBits(s, 7, &repeat) != kInflateOk) {
                return kInflateError;
            }
            repeat += 11;
        }
        if (index + repeat > nlen + ndist) {
            return kInflateError;
        }
        while (repeat-- > 0) {
            lengths[index++] = value;
        }
    }
    if (lengths[256] == 0 || buildHuffman(&lit, lengths, (int)nlen) != kInflateOk ||
        buildHuffman(&dist, lengths + nlen, (int)ndist) != kInflateOk) {
        return kInflateError;
    }
    return inflateCodes(s, &lit, &dist);
}

/*
 * zlib stream in -> out. Like libpng, decoding stops once out is full (data past the image is ignored); a
 * stream that ends before that, corrupt data and a wrong Adler-32 are errors.
 */
static int zlibInflate(Inflate *s)
{
    if (s->inLength < 2) {
        return kInflateError;
    }
    const uint32_t cmf = s->in[0];
    const uint32_t flg = s->in[1];
    if ((cmf & 0x0f) != 8 || (cmf >> 4) > 7 || ((cmf << 8) | flg) % 31 != 0 || (flg & 0x20) != 0) {
        return kInflateError;
    }
    s->inPos = 2;
    uint32_t last = 0;
    do {
        uint32_t type = 0;
        if (needBits(s, 1, &last) != kInflateOk || needBits(s, 2, &type) != kInflateOk) {
            return kInflateError;
        }
        int r = kInflateError;
        if (type == 0) {
            r = inflateStored(s);
        } else if (type == 1) {
            r = inflateFixed(s);
        } else if (type == 2) {
            r = inflateDynamic(s);
        }
        if (r != kInflateOk) {
            return r;
        }
    } while (last == 0);
    if (s->outPos != s->outLength) {
        return kInflateError;
    }
    /* Adler-32 of the output, big endian, after the last block (byte aligned). */
    s->inPos -= (uint32_t)(s->bitCount / 8);
    if (s->inPos + 4 > s->inLength) {
        return kInflateError;
    }
    uint32_t a = 1;
    uint32_t b = 0;
    for (uint32_t i = 0; i < s->outLength; i++) {
        a = (a + s->out[i]) % 65521u;
        b = (b + a) % 65521u;
    }
    return be32(s->in + s->inPos) == ((b << 16) | a) ? kInflateOk : kInflateError;
}

/* ---- scanlines
 * --------------------------------------------------------------------------------------------- */

static const uint8_t kPassX[7] = {0, 4, 0, 2, 0, 1, 0};
static const uint8_t kPassY[7] = {0, 0, 4, 0, 2, 0, 1};
static const uint8_t kPassDx[7] = {8, 8, 4, 4, 2, 2, 1};
static const uint8_t kPassDy[7] = {8, 8, 8, 4, 4, 2, 2};

static int channelCount(int colorType)
{
    switch (colorType) {
    case kColorRgb:
        return 3;
    case kColorGrayAlpha:
        return 2;
    case kColorRgba:
        return 4;
    default:
        return 1;
    }
}

static uint32_t passExtent(uint32_t size, int start, int step)
{
    return size > (uint32_t)start ? (size - (uint32_t)start + (uint32_t)step - 1) / (uint32_t)step : 0;
}

static uint32_t rowBytes(const PngInfo *h, uint32_t width)
{
    return (uint32_t)(((uint64_t)width * (uint32_t)(channelCount(h->colorType) * h->bitDepth) + 7) >> 3);
}

static int paeth(int a, int b, int c)
{
    const int p = a + b - c;
    const int pa = p > a ? p - a : a - p;
    const int pb = p > b ? p - b : b - p;
    const int pc = p > c ? p - c : c - p;
    if (pa <= pb && pa <= pc) {
        return a;
    }
    return pb <= pc ? b : c;
}

/* Undoes the filter of one scanline (row[-1] is its filter byte, prev the unfiltered row above or NULL). */
static int unfilter(uint8_t *row, const uint8_t *prev, uint32_t length, uint32_t bpp)
{
    const int filter = row[-1];
    for (uint32_t i = 0; i < length; i++) {
        const int a = i >= bpp ? row[i - bpp] : 0;
        const int b = prev != NULL ? prev[i] : 0;
        const int c = (i >= bpp && prev != NULL) ? prev[i - bpp] : 0;
        int add = 0;
        switch (filter) {
        case 0:
            break;
        case 1:
            add = a;
            break;
        case 2:
            add = b;
            break;
        case 3:
            add = (a + b) >> 1;
            break;
        case 4:
            add = paeth(a, b, c);
            break;
        default:
            return 0;
        }
        row[i] = (uint8_t)(row[i] + add);
    }
    return 1;
}

static uint32_t sampleAt(const uint8_t *row, uint32_t index, int bitDepth)
{
    if (bitDepth == 16) {
        return ((uint32_t)row[index * 2] << 8) | row[index * 2 + 1];
    }
    if (bitDepth == 8) {
        return row[index];
    }
    const uint32_t bit = index * (uint32_t)bitDepth;
    return (row[bit >> 3] >> (8 - bitDepth - (int)(bit & 7))) & ((1u << bitDepth) - 1u);
}

/*
 * One pixel through libpng 1.0.5's read transformations, in its order: expand (gray 1/2/4 bit to 8, adding
 * alpha from tRNS), strip_16, gray_to_rgb, unpack (one sample per byte), filler. Returns the byte count.
 */
static int transformPixel(const PngStruct *p, const uint8_t *row, uint32_t x, uint8_t *out)
{
    const PngInfo *h = &p->header;
    const int bd = h->bitDepth;
    int channels = channelCount(h->colorType);
    int gray = h->colorType == kColorGray || h->colorType == kColorGrayAlpha;
    uint32_t v[4];
    for (int c = 0; c < channels; c++) {
        v[c] = sampleAt(row, x * (uint32_t)channels + (uint32_t)c, bd);
    }
    if (bd < 8 && h->colorType == kColorGray && (p->transforms & kXformExpand) != 0) {
        static const uint8_t kScale[9] = {0, 0xff, 0x55, 0, 0x11, 0, 0, 0, 1};
        const uint32_t expanded = v[0] * kScale[bd];
        if (h->numTrans != 0) {
            v[1] = expanded == (uint32_t)h->transValues.gray * kScale[bd] ? 0 : 0xff;
            channels = 2;
        }
        v[0] = expanded;
    } else if (bd < 8 && (p->transforms & kXformPack) == 0) {
        return -1; /* packed output: not requested by RtPNGImageRead */
    }
    if (bd == 16) {
        if ((p->transforms & kXformStrip16) == 0) {
            return -1; /* 16-bit output: not requested by RtPNGImageRead */
        }
        for (int c = 0; c < channels; c++) {
            v[c] >>= 8;
        }
    }
    if (gray && (p->transforms & kXformGrayToRgb) != 0) {
        if (channels == 2) {
            v[3] = v[1];
        }
        v[1] = v[0];
        v[2] = v[0];
        channels += 2;
        gray = 0;
    }
    int n = 0;
    if ((p->transforms & kXformFiller) != 0 && (channels == 3 || (gray && channels == 1)) &&
        h->colorType != kColorPalette) {
        if (!p->fillerAfter) {
            out[n++] = p->filler;
        }
        for (int c = 0; c < channels; c++) {
            out[n++] = (uint8_t)v[c];
        }
        if (p->fillerAfter) {
            out[n++] = p->filler;
        }
        return n;
    }
    for (int c = 0; c < channels; c++) {
        out[n++] = (uint8_t)v[c];
    }
    return n;
}

/* ---- the libpng entry points
 * --------------------------------------------------------------------------------- */

void *bmxPngCreateReadStruct2(const char *version, void *errorPtr, void *errorFn, void *warnFn, void *memPtr,
                              void *mallocFn, void *freeFn)
{
    (void)version;
    PngStruct dummy = {0};
    dummy.memPtr = memPtr;
    PngStruct *p = ((PngMallocFn)mallocFn)(&dummy, sizeof *p);
    if (p == NULL) {
        return NULL;
    }
    uint8_t *bytes = (uint8_t *)p;
    for (uint32_t i = 0; i < sizeof *p; i++) {
        bytes[i] = 0;
    }
    p->errorPtr = errorPtr;
    p->errorFn = (PngErrorFn)errorFn;
    p->warningFn = warnFn;
    p->memPtr = memPtr;
    p->mallocFn = (PngMallocFn)mallocFn;
    p->freeFn = (PngFreeFn)freeFn;
    return p;
}

void *bmxPngCreateInfoStruct(void *png)
{
    PngStruct *p = png;
    if (p == NULL) {
        return NULL;
    }
    PngInfo *info = p->mallocFn(p, sizeof *info);
    if (info != NULL) {
        uint8_t *bytes = (uint8_t *)info;
        for (uint32_t i = 0; i < sizeof *info; i++) {
            bytes[i] = 0;
        }
    }
    return info;
}

void bmxPngSetReadFn(void *png, void *ioPtr, void *readFn)
{
    PngStruct *p = png;
    p->ioPtr = ioPtr;
    p->readFn = (PngReadFn)readFn;
}

/* Signature and chunks up to the first IDAT, whose header stays in chunkLength / chunkType. */
void bmxPngReadInfo(void *png, void *info)
{
    static const uint8_t kSignature[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    PngStruct *p = png;
    uint8_t sig[8];
    readRaw(p, sig, 8);
    for (int i = 0; i < 8; i++) {
        if (sig[i] != kSignature[i]) {
            pngError(p, "Not a PNG file");
        }
    }
    int haveHeader = 0;
    for (;;) {
        readChunkHeader(p);
        if (isChunk(p, "IHDR")) {
            if (haveHeader) {
                pngError(p, "Out of place IHDR");
            }
            handleIhdr(p);
            haveHeader = 1;
            continue;
        }
        if (!haveHeader) {
            pngError(p, "Missing IHDR before chunk");
        }
        if (isChunk(p, "IDAT")) {
            if (p->header.colorType == kColorPalette && (p->header.valid & kInfoPlte) == 0) {
                pngError(p, "Missing PLTE before IDAT");
            }
            break;
        }
        if (isChunk(p, "PLTE")) {
            handlePlte(p);
        } else if (isChunk(p, "tRNS")) {
            handleTrns(p);
        } else if (isChunk(p, "IEND")) {
            pngError(p, "No image in file");
        } else if ((p->chunkType[0] & 0x20) == 0) {
            pngError(p, "unknown critical chunk");
        } else {
            finishChunk(p, p->chunkLength);
        }
    }
    *(PngInfo *)info = p->header;
}

uint32_t bmxPngGetIhdr(void *png, void *info, uint32_t *width, uint32_t *height, int *bitDepth,
                       int *colorType, int *interlace, int *compression, int *filter)
{
    const PngInfo *h = info;
    if (png == NULL || h == NULL || width == NULL || height == NULL || bitDepth == NULL ||
        colorType == NULL) {
        return 0;
    }
    *width = h->width;
    *height = h->height;
    *bitDepth = h->bitDepth;
    *colorType = h->colorType;
    if (interlace != NULL) {
        *interlace = h->interlace;
    }
    if (compression != NULL) {
        *compression = 0;
    }
    if (filter != NULL) {
        *filter = 0;
    }
    return 1;
}

void bmxPngSetStrip16(void *png)
{
    ((PngStruct *)png)->transforms |= kXformStrip16;
}

void bmxPngSetPacking(void *png)
{
    PngStruct *p = png;
    if (p->header.bitDepth < 8) {
        p->transforms |= kXformPack;
    }
}

void bmxPngSetGray124To8(void *png)
{
    ((PngStruct *)png)->transforms |= kXformExpand;
}

void bmxPngSetGrayToRgb(void *png)
{
    ((PngStruct *)png)->transforms |= kXformGrayToRgb;
}

void bmxPngSetFiller(void *png, uint32_t filler, int where)
{
    PngStruct *p = png;
    p->transforms |= kXformFiller;
    p->filler = (uint8_t)filler;
    p->fillerAfter = where == 1;
}

uint32_t bmxPngGetValid(void *png, void *info, uint32_t flag)
{
    return (png != NULL && info != NULL) ? ((const PngInfo *)info)->valid & flag : 0;
}

uint32_t bmxPngGetPlte(void *png, void *info, uint8_t **palette, int *numPalette)
{
    PngInfo *h = info;
    if (png == NULL || h == NULL || (h->valid & kInfoPlte) == 0 || palette == NULL) {
        return 0;
    }
    *palette = h->palette;
    *numPalette = h->numPalette;
    return kInfoPlte;
}

uint32_t bmxPngGetTrns(void *png, void *info, uint8_t **trans, int *numTrans, void **transValues)
{
    PngInfo *h = info;
    if (png == NULL || h == NULL || (h->valid & kInfoTrns) == 0) {
        return 0;
    }
    if (h->colorType == kColorPalette) {
        if (trans != NULL) {
            *trans = h->trans;
        }
    } else if (trans != NULL) {
        *trans = NULL;
    }
    if (transValues != NULL) {
        *transValues = &h->transValues;
    }
    if (numTrans != NULL) {
        *numTrans = h->numTrans;
    }
    return kInfoTrns;
}

/* Reads the IDAT run (png_read_info stopped at its first header) into data; the next header stays read. */
static void readImageData(PngStruct *p)
{
    uint32_t capacity = 0;
    while (isChunk(p, "IDAT")) {
        const uint32_t length = p->chunkLength;
        if (length > 0x7fffffffu - p->dataLength) {
            pngError(p, "Image data too large");
        }
        if (p->dataLength + length > capacity) {
            capacity = (p->dataLength + length) * 2 + 0x1000;
            uint8_t *grown = pngAlloc(p, capacity);
            for (uint32_t i = 0; i < p->dataLength; i++) {
                grown[i] = p->data[i];
            }
            pngRelease(p, (void **)&p->data);
            p->data = grown;
        }
        readChunkData(p, p->data + p->dataLength, length);
        p->dataLength += length;
        if (!readChunkCrc(p)) {
            pngError(p, "CRC error");
        }
        readChunkHeader(p);
    }
}

void bmxPngReadImage(void *png, uint8_t **rows)
{
    PngStruct *p = png;
    const PngInfo *h = &p->header;
    const int passes = h->interlace ? 7 : 1;
    const int bitsPerPixel = channelCount(h->colorType) * h->bitDepth;
    const uint32_t bpp = bitsPerPixel >= 8 ? (uint32_t)bitsPerPixel / 8 : 1;

    uint64_t total = 0;
    for (int pass = 0; pass < passes; pass++) {
        const uint32_t w = h->interlace ? passExtent(h->width, kPassX[pass], kPassDx[pass]) : h->width;
        const uint32_t ht = h->interlace ? passExtent(h->height, kPassY[pass], kPassDy[pass]) : h->height;
        if (w != 0 && ht != 0) {
            total += (uint64_t)ht * (rowBytes(h, w) + 1);
        }
    }
    if (total > 0x7fffffffu) {
        pngError(p, "Image too large");
    }
    readImageData(p);
    p->image = pngAlloc(p, (uint32_t)total);
    Inflate s = {.in = p->data, .inLength = p->dataLength, .out = p->image, .outLength = (uint32_t)total};
    const int r = zlibInflate(&s);
    if (r == kInflateError) {
        pngError(p, "Decompression error");
    }

    uint8_t *line = p->image;
    for (int pass = 0; pass < passes; pass++) {
        const uint32_t x0 = h->interlace ? kPassX[pass] : 0;
        const uint32_t y0 = h->interlace ? kPassY[pass] : 0;
        const uint32_t dx = h->interlace ? kPassDx[pass] : 1;
        const uint32_t dy = h->interlace ? kPassDy[pass] : 1;
        const uint32_t w = passExtent(h->width, (int)x0, (int)dx);
        const uint32_t ht = passExtent(h->height, (int)y0, (int)dy);
        if (w == 0 || ht == 0) {
            continue;
        }
        const uint32_t length = rowBytes(h, w);
        const uint8_t *prev = NULL;
        for (uint32_t j = 0; j < ht; j++) {
            uint8_t *row = line + 1;
            if (!unfilter(row, prev, length, bpp)) {
                pngError(p, "Unknown filter type");
            }
            uint8_t *dest = rows[y0 + j * dy];
            for (uint32_t i = 0; i < w; i++) {
                uint8_t pixel[4];
                const int n = transformPixel(p, row, i, pixel);
                if (n < 0) {
                    pngError(p, "Unsupported transformation");
                }
                uint8_t *at = dest + (x0 + i * dx) * (uint32_t)n;
                for (int k = 0; k < n; k++) {
                    at[k] = pixel[k];
                }
            }
            prev = row;
            line += length + 1;
        }
    }
    pngRelease(p, (void **)&p->image);
    pngRelease(p, (void **)&p->data);
}

/* The chunks after the image data, up to IEND. */
void bmxPngReadEnd(void *png, void *info)
{
    (void)info;
    PngStruct *p = png;
    for (;;) {
        if (isChunk(p, "IEND")) {
            finishChunk(p, p->chunkLength);
            return;
        }
        if (isChunk(p, "IDAT")) {
            pngError(p, "Too many IDAT's found");
        }
        if (isChunk(p, "PLTE") || isChunk(p, "IHDR") || (p->chunkType[0] & 0x20) == 0) {
            pngError(p, "Invalid chunk after IDAT");
        }
        finishChunk(p, p->chunkLength);
        readChunkHeader(p);
    }
}

void bmxPngDestroyReadStruct(void **png, void **info, void **endInfo)
{
    PngStruct *p = png != NULL ? *png : NULL;
    if (p == NULL) {
        return;
    }
    if (info != NULL) {
        pngRelease(p, info);
    }
    if (endInfo != NULL) {
        pngRelease(p, endInfo);
    }
    pngRelease(p, (void **)&p->image);
    pngRelease(p, (void **)&p->data);
    PngFreeFn freeFn = p->freeFn;
    PngStruct dummy = {0};
    dummy.memPtr = p->memPtr;
    freeFn(&dummy, p);
    *png = NULL;
}

/* ---- the jump table ----------------------------------------------------------------------------------- */

const BmxHook bmxPngJumps[] = {
    {0x005070d0, (void *)bmxPngCreateReadStruct2},
    {0x00508e40, (void *)bmxPngCreateInfoStruct},
    {0x005090f0, (void *)bmxPngSetReadFn},
    {0x00507290, (void *)bmxPngReadInfo},
    {0x00509600, (void *)bmxPngGetIhdr},
    {0x00509b90, (void *)bmxPngSetStrip16},
    {0x00510bb0, (void *)bmxPngSetPacking},
    {0x0050a610, (void *)bmxPngSetGray124To8},
    {0x0050a650, (void *)bmxPngSetGrayToRgb},
    {0x00510c50, (void *)bmxPngSetFiller},
    {0x00509150, (void *)bmxPngGetValid},
    {0x00509890, (void *)bmxPngGetPlte},
    {0x00509970, (void *)bmxPngGetTrns},
    {0x00507f80, (void *)bmxPngReadImage},
    {0x00508000, (void *)bmxPngReadEnd},
    {0x00508550, (void *)bmxPngDestroyReadStruct},
    {0x00507050, (void *)pngError},
};

const uint32_t bmxPngJumpCount = sizeof bmxPngJumps / sizeof bmxPngJumps[0];
