/*
 * Cinepak ('cvid') decoder (docs/PLATFORM.md P12): decodes into a bottom-up 24-bit DIB like the frames
 * AVIStreamGetFrame hands out through iccvid.dll. Hosted C, no dependencies.
 */
#ifndef BMX_PLATFORM_CINEPAK_H
#define BMX_PLATFORM_CINEPAK_H

#include <stdint.h>

enum { kCinepakMaxStrips = 32 };

/* One codebook entry: four luma values (top-left, top-right, bottom-left, bottom-right) and the chroma. */
typedef struct CinepakEntry {
    uint8_t y[4];
    int8_t u, v;
} CinepakEntry;

typedef struct CinepakStrip {
    CinepakEntry v1[256];
    CinepakEntry v4[256];
} CinepakStrip;

/* Decoder state. The codebooks persist from frame to frame (inter frames update them). */
typedef struct CinepakDecoder {
    int32_t width, height;
    CinepakStrip strips[kCinepakMaxStrips];
} CinepakDecoder;

void cinepakInit(CinepakDecoder *d, int32_t width, int32_t height);
/*
 * Decodes one frame into `bgr`: bottom-up rows of `stride` bytes, B, G, R per pixel. Pixels a frame does not
 * code (skipped blocks of an inter frame) keep their values. Returns 0 when the data is malformed (what was
 * decoded up to that point stays).
 */
int cinepakDecode(CinepakDecoder *d, const uint8_t *data, uint32_t size, uint8_t *bgr, int32_t stride);

#endif
