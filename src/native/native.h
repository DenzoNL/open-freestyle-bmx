#ifndef BMX_NATIVE_NATIVE_H
#define BMX_NATIVE_NATIVE_H

#include <stdint.h>

/* The original image range, rebuilt by the loader before any game code runs. */
enum { kBmxImageBase = 0x400000, kBmxOrigRangeSize = 0x2b3000 };

/* Log to stderr and bmxnative.log. */
void bmxLog(const char *fmt, ...);
void bmxLogOpen(void);

/*
 * Verify the user's NgBMX.exe, map its image at 0x400000, copy the data sections, fill the code with
 * int3, write the hook jumps and the import thunks, and fill the import table. Returns 0 on success;
 * on failure the reason has been logged.
 */
int bmxLoadImage(const char *exePath);

/* Write a jump from every reachable original CRT entry point to our C runtime layer (src/native/crt.c). */
void bmxCrtInstall(void);

/*
 * BMX_PNG_CHECK (src/native/pngcheck.c): keep the original libpng / zlib code (copied from the exe's code
 * section starting at codeStart) and remember its entry bytes; after our jumps are written, arm the checker
 * that compares both readers on every RtPNGImageRead call.
 */
void bmxPngCheckInstall(const uint8_t *code, uint32_t codeStart);
void bmxPngCheckArm(void);

/*
 * BMX_KEYS (src/native/keys.c): parse the key script; returns 1 when the keyboard is scripted. The
 * DirectInput stand-in (src/platform/input.c) then reads its keyboard from the script (bmxKeysFillState), and
 * src/platform/window.c turns its key edges into key messages and answers message boxes without a window.
 */
int bmxKeysInstall(void);

/* Install the vectored exception handler that reports int3 traps. */
void bmxTrapInstall(void);

#endif
