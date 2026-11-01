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

struct BmxHook;
/*
 * [video] full_draw_distance (drawdist.c): the jump over the ported ngiffxAdaptQuality's entry that keeps the
 * far clip at 200 when the option is 1, none (count 0) otherwise.
 */
const struct BmxHook *bmxDrawDistJumps(uint32_t *count);

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

/* BMX_RECORD / BMX_REPLAY (replay.c; the API is in platform/platform.h): closes the recording at exit. */
void bmxReplayClose(void);

/* BMX_SIM_HASH=<file> (src/native/simhash.c): a per-tick hash of the simulation state, one line per
 * controller poll (docs/WIDESCREEN.md W10.5). */
void bmxSimHashInstall(void);
/* The hash BMX_SIM_HASH writes per tick, of the current state (also used by BMX_HF_GUARD). */
uint64_t bmxSimStateHash(void);

/*
 * BMX_HF_GUARD=1|2|3 (src/native/hfguard.c, docs/HIGHFPS.md "HF0 findings"): measures what each draw changes
 * and how the loop paces. bmxHfGuardInstall returns the level (0: off); main.c then calls bmxHfGuardFrame
 * for every tick instead of onIdle + zrwDrawFrame (returns 0 when onIdle ends the program).
 */
int bmxHfGuardInstall(void);
int bmxHfGuardFrame(void);
/*
 * The guard with `[video] interpolate = 1` (hfinterp.c runs the loop): per level tick, and around every extra
 * draw. The summary counts the sub-hashes an extra draw changed (frames, ties and the shadow are render state
 * there: the draw syncs the interpolated poses; the restore before the next tick puts them back).
 */
void bmxHfGuardInterpTick(void);
void bmxHfGuardExtraBefore(void);
void bmxHfGuardExtraAfter(void);

/*
 * `[video] interpolate = 1` (src/native/hfinterp.c, docs/HIGHFPS.md HF1-HF3): bmxHfInstall reads the
 * config (and BMX_HF_ALPHA / BMX_HF_LOG) and returns 1 when the mode is on; main.c then calls bmxHfStep
 * instead of its own limiter, onIdle and zrwDrawFrame (returns 0 when onIdle ends the program). `guard`:
 * the BMX_HF_GUARD level (0: off).
 */
int bmxHfInstall(int guard);
int bmxHfStep(void);

/* Install the vectored exception handler that reports int3 traps. */
void bmxTrapInstall(void);

#endif
