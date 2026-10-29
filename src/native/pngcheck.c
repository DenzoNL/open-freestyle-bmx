/*
 * BMX_PNG_CHECK: differential check of our PNG reader (src/png) against the original libpng 1.0.5, inside
 * the native exe. The loader then keeps the original's libpng and zlib code (kBmxPngCodeStart..End) instead
 * of int3 and remembers the first bytes of each entry point our reader replaces. Every PNG read (the game's
 * PNGImageRead 0x497a30, or RtPNGImageRead 0x5068a0 through a pointer) goes to a checker. With
 * BMX_PNG_CHECK=<folder>, at the first one every *.png in that folder runs through the ported RtPNGImageRead
 * once with the original libpng and once with ours, and the RwImages are compared (width, height, depth,
 * stride, every pixel byte the reader writes, the palette): the synthetic set from tools/png_cases.py plus
 * the PNGs extracted from the game's model files. RtPNGImageRead calls through 0x5068a0 are compared the same
 * way; the game's own reads get our reader's image.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>

#include "native/native.h"
#include "png/png.h"
#include "rw/rwcore.h"
#include "rw/rwstream.h"
#include "rw/rwtexture.h"
#include "zge/com/zglrw/zrwtxr.h"

static uint8_t sSaved[32][5];
static uint8_t sOurs[32][5];
static int sMismatches;
static int sChecked;

/* RenderWare's engine instance pointer (0x67696c; this file is built without BMX_HOOK, so no ORIG_VAR). */
static RwGlobals *engine(void)
{
    return *(RwGlobals **)(uintptr_t)0x0067696c;
}

/* Original libpng (1) or our reader (0) at the entry points RtPNGImageRead calls. */
static void selectReader(int original)
{
    for (uint32_t i = 0; i < bmxPngJumpCount; i++) {
        memcpy((void *)(uintptr_t)bmxPngJumps[i].orig, original ? sSaved[i] : sOurs[i], 5);
    }
    FlushInstructionCache(GetCurrentProcess(), NULL, 0);
}

static int sameBytes(const uint8_t *a, const uint8_t *b, size_t n, size_t *at)
{
    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i]) {
            *at = i;
            return 0;
        }
    }
    return 1;
}

static void compareImages(const char *name, const RwImage *a, const RwImage *b)
{
    sChecked++;
    if (a == NULL || b == NULL) {
        if (a != b) {
            sMismatches++;
            bmxLog("pngcheck: %s: MISMATCH original %s, ours %s\n", name, a ? "image" : "NULL",
                   b ? "image" : "NULL");
        } else {
            bmxLog("pngcheck: %s: both NULL\n", name);
        }
        return;
    }
    if (a->width != b->width || a->height != b->height || a->depth != b->depth || a->stride != b->stride) {
        sMismatches++;
        bmxLog("pngcheck: %s: MISMATCH header %dx%d d%d s%d vs %dx%d d%d s%d\n", name, a->width, a->height,
               a->depth, a->stride, b->width, b->height, b->depth, b->stride);
        return;
    }
    /* Bytes the reader writes per row (depth 4 images get one byte per pixel and overrun the stride, like
     * the original); the rest of the row is uninitialised heap in both. */
    size_t written = (size_t)a->width * (a->depth == 32 ? 4 : 1);
    if (written > (size_t)a->stride) {
        written = (size_t)a->stride;
    }
    size_t at = 0;
    for (int32_t y = 0; y < a->height; y++) {
        if (!sameBytes(a->pixels + (size_t)y * a->stride, b->pixels + (size_t)y * b->stride, written, &at)) {
            sMismatches++;
            bmxLog("pngcheck: %s: MISMATCH pixels row %d byte %u\n", name, y, (unsigned)at);
            return;
        }
    }
    if ((a->depth == 4 || a->depth == 8) && !sameBytes(a->palette, b->palette, (size_t)4 << a->depth, &at)) {
        sMismatches++;
        bmxLog("pngcheck: %s: MISMATCH palette byte %u\n", name, (unsigned)at);
        return;
    }
    bmxLog("pngcheck: %s: same (%dx%d depth %d stride %d)\n", name, a->width, a->height, a->depth, a->stride);
}

static RwImage *checkOne(const char *name)
{
    selectReader(1);
    RwImage *original = RtPNGImageRead(name);
    selectReader(0);
    RwImage *ours = RtPNGImageRead(name);
    compareImages(name, original, ours);
    if (original != NULL) {
        RwImageDestroy(original);
    }
    return ours;
}

static void checkFolder(const char *folder)
{
    char pattern[MAX_PATH];
    WIN32_FIND_DATAA found;
    snprintf(pattern, sizeof pattern, "%s\\*.png", folder);
    HANDLE h = FindFirstFileA(pattern, &found);
    if (h == INVALID_HANDLE_VALUE) {
        bmxLog("pngcheck: no *.png in %s\n", folder);
        return;
    }
    /* The game swaps the engine's file functions for its in-memory ones while it reads a texture (rwcore.h):
     * plain stdio (_rwFSOpen's set) for the folder, then back. */
    const RwFileFunctions saved = engine()->fileFuncs;
    _rwFSOpen();
    do {
        char path[MAX_PATH];
        snprintf(path, sizeof path, "%s\\%s", folder, found.cFileName);
        RwImage *image = NULL;
        if (strncmp(found.cFileName, "bad_", 4) == 0) {
            /* Corrupt input: libpng's default error handler fprintf()s to the MSVC stderr, which our CRT
             * layer cannot take, so only our reader runs; it must longjmp back and RtPNGImageRead return
             * NULL. */
            selectReader(0);
            image = RtPNGImageRead(path);
            sChecked++;
            if (image != NULL) {
                sMismatches++;
            }
            bmxLog("pngcheck: %s: ours %s (expected NULL)\n", found.cFileName, image ? "image" : "NULL");
        } else {
            image = checkOne(path);
        }
        if (image != NULL) {
            RwImageDestroy(image);
        }
    } while (FindNextFileA(h, &found));
    FindClose(h);
    engine()->fileFuncs = saved;
    bmxLog("pngcheck: folder done, %d checked, %d mismatches\n", sChecked, sMismatches);
}

static void checkFolderOnce(void)
{
    static int folderDone;
    const char *folder = getenv("BMX_PNG_CHECK");
    if (!folderDone && folder != NULL && strcmp(folder, "1") != 0) {
        folderDone = 1;
        checkFolder(folder);
    }
}

/*
 * The game's PNG format callback (zrwtxr.c, 0x497a30) calls the ported RtPNGImageRead directly, not through
 * 0x5068a0, and reads the PNGs embedded in its model files through in-memory file functions that cannot be
 * opened twice: check the folder (with copies of those PNGs), then let the game read with our reader.
 */
static void *checkedPngImageRead(const char *name)
{
    checkFolderOnce();
    RwImage *image = PNGImageRead(name);
    if (image != NULL) {
        bmxLog("pngcheck: game read %s with our reader: %dx%d depth %d\n", name, image->width, image->height,
               image->depth);
    } else {
        bmxLog("pngcheck: game read %s with our reader: NULL\n", name);
    }
    return image;
}

static RwImage *checkedPngRead(const char *name)
{
    checkFolderOnce();
    RwImage *image = checkOne(name);
    bmxLog("pngcheck: %d checked, %d mismatches\n", sChecked, sMismatches);
    return image;
}

void bmxPngCheckInstall(const uint8_t *code, uint32_t codeStart)
{
    memcpy((void *)(uintptr_t)kBmxPngCodeStart, code + (kBmxPngCodeStart - codeStart),
           kBmxPngCodeEnd - kBmxPngCodeStart);
    for (uint32_t i = 0; i < bmxPngJumpCount && i < 32; i++) {
        memcpy(sSaved[i], (const void *)(uintptr_t)bmxPngJumps[i].orig, 5);
    }
}

void bmxPngCheckArm(void)
{
    for (uint32_t i = 0; i < bmxPngJumpCount && i < 32; i++) {
        memcpy(sOurs[i], (const void *)(uintptr_t)bmxPngJumps[i].orig, 5);
    }
    /* RtPNGImageRead's and PNGImageRead's entries (written by the hook table) now go to the checker. */
    const struct {
        uint32_t at;
        void *to;
    } redirect[] = {{0x005068a0, (void *)checkedPngRead}, {0x00497a30, (void *)checkedPngImageRead}};
    for (size_t i = 0; i < sizeof redirect / sizeof redirect[0]; i++) {
        uint8_t *p = (uint8_t *)(uintptr_t)redirect[i].at;
        const int32_t rel = (int32_t)((uintptr_t)redirect[i].to - (redirect[i].at + 5));
        p[0] = 0xE9;
        memcpy(p + 1, &rel, 4);
    }
    bmxLog("pngcheck: armed (original libpng kept at %#x..%#x)\n", kBmxPngCodeStart, kBmxPngCodeEnd);
}
