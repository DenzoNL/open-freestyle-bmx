/*
 * The platform layer (docs/PLATFORM.md): stand-ins for the Windows functions the game imports, built on SDL3
 * and our own code. The loader (src/native/loader.c) fills the exe's import slots from the table in
 * imports.c. Everything here has the Win32 signature of the function it replaces; with no option set the game
 * sees the same values and behaviour as from Windows.
 */
#ifndef BMX_PLATFORM_PLATFORM_H
#define BMX_PLATFORM_PLATFORM_H

#include <stddef.h>
#include <stdint.h>

#if defined(__i386__)
#define BMX_WINAPI __attribute__((stdcall))
#else
#define BMX_WINAPI
#endif

/* ---- the import table (imports.c) ---- */

/* One stand-in: replaces dll!name (the DLL name compared without regard to case). */
typedef struct BmxStandIn {
    const char *dll;
    const char *name;
    void *fn;
} BmxStandIn;

extern const BmxStandIn bmxStandIns[];
extern const size_t bmxStandInCount;

/* The stand-in for dll!name or NULL. */
void *bmxStandInFind(const char *dll, const char *name);
/* 1 when the native exe never reaches dll!name (docs/PLATFORM.md 1b): the loader gives it a trap stub. */
int bmxImportIsDead(const char *dll, const char *name);
/* 1 when a stand-in of this DLL forwards to the real DLL when its option is off (loaded even if every slot of
 * it is a stand-in). */
int bmxImportKeepsDll(const char *dll);

/* ---- provided by the loader ---- */

/* The real dll!name (loads the DLL when needed), for stand-ins that forward or check against Windows. */
void *bmxRealProc(const char *dll, const char *name);
/* Log to stderr and bmxnative.log (src/native/trap.c). */
void bmxLog(const char *fmt, ...);

/* BMX_KEYS (src/native/keys.c): 1 when the keyboard is scripted; the script's DIK state at the current shown
 * frame (state is zeroed by the caller). */
int bmxKeysScripted(void);
void bmxKeysFillState(uint8_t *state, uint32_t size);

/* ---- time and misc kernel32 (kernel.c) ---- */

/* Sets the clock origin; idempotent. Call before the game runs. BMX_TIME_BASE overrides the base. */
void bmxPlatformInit(void);
/* The milliseconds the game's timeGetTime and GetTickCount see (32-bit, wrapping). */
uint32_t bmxTimeGetTime(void);
uint32_t bmxGetTickCount(void);
/* Test hook: the value both clocks show at the origin. */
void bmxTimeSetBase(uint32_t base);

/* ---- winmm timer thread (winmm.c) ---- */

/* Stops every timer (joins the threads); logs the interval statistics when BMX_TIMER_LOG is set. */
void bmxTimerShutdown(void);

/* At exit (kernel.c): stops the timers and logs what the dev checks collected (BMX_INPUT_CHECK). */
void bmxPlatformShutdown(void);

/* ---- the config file (config.c, P19) ---- */

/*
 * bmxnative.ini next to the exe (BMX_CONFIG=<path> names another), read on first use: `key = value` lines,
 * `[section]` headers prefix the keys ("input.dpad_steering"), `#` and `;` start comments. Options are off
 * by default except the input conveniences dpad_steering and stick_8way (on); unknown keys are logged.
 */
int bmxConfigInt(const char *key, int fallback);
/* Replaces the configuration by the text (tests; `origin` names it in the log). */
void bmxConfigLoadText(const char *text, const char *origin);

/* ---- paths (paths.c) ---- */

/* The host path for a path as the game spells it (relative to the install folder, backslashes). Identity
 * today; the seam for an asset folder, profile saves and case-insensitive lookup on other platforms (P18). */
const char *bmxPathResolve(const char *gamePath);

#endif
