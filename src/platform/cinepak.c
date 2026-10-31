/*
 * Cinepak decoder (cinepak.h). The bitstream: a frame header (flags, size, width, height, strip count), then
 * strips of 4x4 blocks, each with its own V1 / V4 codebooks (entries: four luma values plus one chroma pair)
 * and a vector chunk saying per block: skip (inter frames), V1 (one entry scaled 2x) or V4 (four entries, one
 * per 2x2 quadrant).
 */
#include "platform/cinepak.h"

#include <string.h>

static uint32_t be16(const uint8_t *p)
{
    return ((uint32_t)p[0] << 8) | p[1];
}

static uint32_t be24(const uint8_t *p)
{
    return ((uint32_t)p[0] << 16) | ((uint32_t)p[1] << 8) | p[2];
}

static uint32_t be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | be24(p + 1);
}

static uint8_t clamp255(int v)
{
    return (uint8_t)(v < 0 ? 0 : v > 255 ? 255 : v);
}

void cinepakInit(CinepakDecoder *d, int32_t width, int32_t height)
{
    memset(d, 0, sizeof *d);
    d->width = width;
    d->height = height;
}

/*
 * iccvid's quirk in a selective V1 codebook update whose last flag word is zero: one more entry is
 * overwritten, with the chunk's first four bytes (its first flag word) as luma and no chroma. Which entry:
 * the one after the last updated entry when the word before the last is not zero (or there is none), else
 * entry 32 * (G - 1) + 1 for the last word G. Measured with hand-made chunks through ICDecompress (random
 * flag words, 400 cases, all fit); it shows in the game's movies as single wrong blocks (acclaim.avi frame
 * 35, nyquist.avi frame 232). V4 updates do not do this.
 */
static void zeroTailQuirk(CinepakEntry *book, const uint8_t *body, int words, uint32_t lastWord,
                          uint32_t wordBefore, int lastUpdated)
{
    const int target = (words == 1 || wordBefore != 0) ? lastUpdated + 1 : 32 * (words - 2) + 1;

    if (words == 0 || lastWord != 0 || target < 0 || target > 255) {
        return;
    }
    memcpy(book[target].y, body, 4);
    book[target].u = 0;
    book[target].v = 0;
}

/*
 * Codebook chunk: 0x20 / 0x22 full (V4 / V1), +0x01 selective (a 32-bit mask before every 32 entries says
 * which are updated), +0x04 greyscale (4 bytes per entry, no chroma).
 */
static void decodeCodebook(CinepakEntry *book, uint32_t id, const uint8_t *p, const uint8_t *end)
{
    const uint8_t *const body = p;
    const int n = (id & 0x04) != 0 ? 4 : 6;
    uint32_t flag = 0, mask = 0, wordBefore = 0;
    int words = 0, lastUpdated = -1;

    for (int i = 0; i < 256; i++) {
        if ((id & 0x01) != 0) {
            mask >>= 1;
            if (mask == 0) {
                if (end - p < 4) {
                    break;
                }
                wordBefore = flag;
                flag = be32(p);
                p += 4;
                words++;
                mask = 0x80000000u;
            }
            if ((flag & mask) == 0) {
                continue;
            }
        }
        if (end - p < n) {
            return; /* data ends inside an entry: no quirk (not measured) */
        }
        memcpy(book[i].y, p, 4);
        if (n == 6) {
            book[i].u = (int8_t)p[4];
            book[i].v = (int8_t)p[5];
        } else {
            book[i].u = 0; /* greyscale */
            book[i].v = 0;
        }
        p += n;
        lastUpdated = i;
    }
    if ((id & 0x03) == 0x03) {
        zeroTailQuirk(book, body, words, flag, wordBefore, lastUpdated);
    }
}

/* Writes the 2x2 pixels of luma k..k+1 / k+2..k+3 of `e` at (x, y) (top-down coordinates). */
static void put2x2(const CinepakDecoder *d, uint8_t *bgr, int32_t stride, int32_t x, int32_t y,
                   const CinepakEntry *e)
{
    /* iccvid's conversion: R = Y + 2V, G = Y - U/2 - V, B = Y + 2U, clamped. */
    for (int k = 0; k < 4; k++) {
        const int32_t px = x + (k & 1), py = y + (k >> 1);
        const int luma = e->y[k];

        if (px < d->width && py < d->height) {
            uint8_t *const out = bgr + (int32_t)(d->height - 1 - py) * stride + px * 3;

            out[0] = clamp255(luma + 2 * e->u);
            out[1] = clamp255(luma - e->u / 2 - e->v);
            out[2] = clamp255(luma + 2 * e->v);
        }
    }
}

/* One V1 block: the entry's four luma values each cover a 2x2 quadrant. */
static void putV1(const CinepakDecoder *d, uint8_t *bgr, int32_t stride, int32_t x, int32_t y,
                  const CinepakEntry *e)
{
    for (int q = 0; q < 4; q++) {
        CinepakEntry quad;

        memset(quad.y, e->y[q], 4);
        quad.u = e->u;
        quad.v = e->v;
        put2x2(d, bgr, stride, x + (q & 1) * 2, y + (q >> 1) * 2, &quad);
    }
}

/* Vector chunk: 0x30 intra (one bit per block: V4 or V1), 0x31 inter (skip bit, then V4 / V1), 0x32 all V1.
 */
static int decodeVectors(const CinepakDecoder *d, const CinepakStrip *s, uint32_t id, const uint8_t *p,
                         const uint8_t *end, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint8_t *bgr,
                         int32_t stride)
{
    uint32_t flag = 0, mask = 0;

    for (int32_t y = y1; y < y2; y += 4) {
        for (int32_t x = x1; x < x2; x += 4) {
            if ((id & 0x01) != 0) {
                mask >>= 1;
                if (mask == 0) {
                    if (end - p < 4) {
                        return 0;
                    }
                    flag = be32(p);
                    p += 4;
                    mask = 0x80000000u;
                }
                if ((flag & mask) == 0) {
                    continue; /* skipped: the block keeps the previous frame's pixels */
                }
            }
            if ((id & 0x02) == 0) {
                mask >>= 1;
                if (mask == 0) {
                    if (end - p < 4) {
                        return 0;
                    }
                    flag = be32(p);
                    p += 4;
                    mask = 0x80000000u;
                }
            }
            if ((id & 0x02) != 0 || (flag & mask) == 0) {
                if (end - p < 1) {
                    return 0;
                }
                putV1(d, bgr, stride, x, y, &s->v1[*p++]);
            } else {
                if (end - p < 4) {
                    return 0;
                }
                put2x2(d, bgr, stride, x, y, &s->v4[p[0]]);
                put2x2(d, bgr, stride, x + 2, y, &s->v4[p[1]]);
                put2x2(d, bgr, stride, x, y + 2, &s->v4[p[2]]);
                put2x2(d, bgr, stride, x + 2, y + 2, &s->v4[p[3]]);
                p += 4;
            }
        }
    }
    return 1;
}

static int decodeStrip(const CinepakDecoder *d, CinepakStrip *s, const uint8_t *p, const uint8_t *end,
                       int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint8_t *bgr, int32_t stride)
{
    while (end - p >= 4) {
        const uint32_t id = p[0];
        uint32_t size = be24(p + 1);

        if (size < 4) {
            return 0;
        }
        size -= 4;
        p += 4;
        if (size > (uint32_t)(end - p)) {
            size = (uint32_t)(end - p);
        }
        switch (id) {
        case 0x20:
        case 0x21:
        case 0x24:
        case 0x25:
            decodeCodebook(s->v4, id, p, p + size);
            break;
        case 0x22:
        case 0x23:
        case 0x26:
        case 0x27:
            decodeCodebook(s->v1, id, p, p + size);
            break;
        case 0x30:
        case 0x31:
        case 0x32:
            return decodeVectors(d, s, id, p, p + size, x1, y1, x2, y2, bgr, stride);
        default:
            break;
        }
        p += size;
    }
    return 1;
}

int cinepakDecode(CinepakDecoder *d, const uint8_t *data, uint32_t size, uint8_t *bgr, int32_t stride)
{
    const uint8_t *p = data;
    const uint8_t *const end = data + size;
    uint32_t flags, strips;
    int32_t y0 = 0;

    if (size < 10) {
        return 0;
    }
    flags = data[0];
    strips = be16(data + 8);
    if (strips > kCinepakMaxStrips) {
        strips = kCinepakMaxStrips;
    }
    p += 10;
    for (uint32_t i = 0; i < strips; i++) {
        int32_t y1, y2, x1, x2;
        uint32_t stripSize, id;

        if (end - p < 12) {
            return 0;
        }
        /* Top 0 means "below the previous strip", the bottom field then being the height. */
        y1 = (int32_t)be16(p + 4);
        if (y1 == 0) {
            y1 = y0;
            y2 = y1 + (int32_t)be16(p + 8);
        } else {
            y2 = (int32_t)be16(p + 8);
        }
        x1 = (int32_t)be16(p + 6);
        x2 = (int32_t)be16(p + 10);
        stripSize = be24(p + 1);
        if (stripSize < 12) {
            return 0;
        }
        stripSize -= 12;
        id = p[0];
        p += 12;
        if (stripSize > (uint32_t)(end - p)) {
            stripSize = (uint32_t)(end - p);
        }
        /* Without flag bit 0 an inter strip (0x11) starts from the codebooks of the strip above it; iccvid
         * does not do this for a key strip (0x10), which keeps its own (measured; ffmpeg copies for both). */
        if (i > 0 && (flags & 0x01) == 0 && id == 0x11) {
            d->strips[i] = d->strips[i - 1];
        }
        if (!decodeStrip(d, &d->strips[i], p, p + stripSize, x1, y1, x2, y2, bgr, stride)) {
            return 0;
        }
        p += stripSize;
        y0 = y2;
    }
    return 1;
}
