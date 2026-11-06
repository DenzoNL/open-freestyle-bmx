/*
 * Start-up of the native exe's original-side state: the import slots of the original's data
 * (src/data/imports.c) get their functions, original code addresses that RenderWare passes around become
 * functions, and the opt-in jumps over our own functions are written. Nothing is read from NgBMX.exe: its
 * data is in the source (src/data, docs/NATIVE.md "The original's data in the source").
 *
 * macOS / Linux (docs/PORTING.md "X5 part 1"): no real Windows function exists, and on arm64 no code can be
 * written at run time (macOS signs every code page). The stubs are C functions there (BMX_STUBS below), and
 * the opt-in replacements go through the redirect slots of compat/orig.h (BMX_REDIRECT_VOID) instead of
 * jumps.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#endif

#include "driver/wide.h"
#include "hook/hook.h"
#include "native/native.h"
#include "platform/music.h"
#include "platform/platform.h"
#include "platform/smoothmix.h"

#if defined(_WIN32) && (defined(__i386__) || defined(__x86_64__))
#define BMX_MACHINE_STUBS 1 /* stubs and jumps written as x86 machine code (BMX_REDIRECTS 0) */
#else
#define BMX_MACHINE_STUBS 0
#endif

/*
 * Dead slots (docs/PLATFORM.md 1b): the original code that calls them is not part of this exe. Each gets a
 * stub in place of the real function, so the DLLs are not loaded for them; if one is called anyway the stub
 * reports which function and from where, and the process stops. A stub is `push <slot index>; call deadSlot`:
 * deadSlot then sees (index, caller's return address) as its two cdecl arguments.
 */
enum { kMaxDeadSlots = 160, kDeadStubSize = 16 };

static char deadNames[kMaxDeadSlots][64];
static int deadCount;

static void __attribute__((noreturn)) BMX_TRAP_CDECL deadSlot(uint32_t index, uint32_t callerRet)
{
    bmxLog("DEAD IMPORT called: %s (the native exe should never reach it), caller %08x\n",
           index < kMaxDeadSlots ? deadNames[index] : "?", (unsigned)callerRet);
    bmxExitProcess(3);
}

#if BMX_MACHINE_STUBS
static void *makeDeadStub(const char *dll, const char *name)
{
    static uint8_t *page;
    uint8_t *stub;
    int32_t rel;
    const uint32_t index = (uint32_t)deadCount;

    if (page == NULL) {
        page = VirtualAlloc(NULL, kMaxDeadSlots * kDeadStubSize, MEM_COMMIT | MEM_RESERVE,
                            PAGE_EXECUTE_READWRITE);
    }
    if (page == NULL || deadCount == kMaxDeadSlots) {
        return NULL;
    }
    snprintf(deadNames[index], sizeof deadNames[index], "%s!%s", dll, name);
    stub = page + index * kDeadStubSize;
    rel = (int32_t)((uintptr_t)deadSlot - ((uintptr_t)stub + 10));
    stub[0] = 0x68; /* push imm32 */
    memcpy(stub + 1, &index, 4);
    stub[5] = 0xE8; /* call rel32 */
    memcpy(stub + 6, &rel, 4);
    deadCount++;
    return stub;
}
#else
/* S(d, k) for the indices 10 d + k: the C stubs of the dead slots and the trap addresses. */
#define BMX_STUBS_DEF10(S, d) S(d, 0) S(d, 1) S(d, 2) S(d, 3) S(d, 4) S(d, 5) S(d, 6) S(d, 7) S(d, 8) S(d, 9)
#define BMX_STUBS_DEF4(S, d)  S(d, 0) S(d, 1) S(d, 2) S(d, 3)
#define BMX_STUBS_REF4(S, d)  S(d, 0), S(d, 1), S(d, 2), S(d, 3)
#define BMX_STUBS_REF10(S, d)                                                                                \
    S(d, 0), S(d, 1), S(d, 2), S(d, 3), S(d, 4), S(d, 5), S(d, 6), S(d, 7), S(d, 8), S(d, 9)

#define DEAD_STUB_DEF(d, k)                                                                                  \
    static void deadStub_##d##_##k(void)                                                                     \
    {                                                                                                        \
        deadSlot((d) * 10 + (k), (uint32_t)(uintptr_t)__builtin_return_address(0));                          \
    }
#define DEAD_STUB_REF(d, k) deadStub_##d##_##k
BMX_STUBS_DEF10(DEAD_STUB_DEF, 0)
BMX_STUBS_DEF10(DEAD_STUB_DEF, 1)
BMX_STUBS_DEF10(DEAD_STUB_DEF, 2)
BMX_STUBS_DEF10(DEAD_STUB_DEF, 3)
BMX_STUBS_DEF10(DEAD_STUB_DEF, 4)
BMX_STUBS_DEF10(DEAD_STUB_DEF, 5)
BMX_STUBS_DEF10(DEAD_STUB_DEF, 6)
BMX_STUBS_DEF10(DEAD_STUB_DEF, 7)
BMX_STUBS_DEF10(DEAD_STUB_DEF, 8)
BMX_STUBS_DEF10(DEAD_STUB_DEF, 9)
BMX_STUBS_DEF10(DEAD_STUB_DEF, 10)
BMX_STUBS_DEF10(DEAD_STUB_DEF, 11)
BMX_STUBS_DEF10(DEAD_STUB_DEF, 12)
BMX_STUBS_DEF10(DEAD_STUB_DEF, 13)
BMX_STUBS_DEF10(DEAD_STUB_DEF, 14)
BMX_STUBS_DEF10(DEAD_STUB_DEF, 15)
static void (*const kDeadStubs[])(void) = {
    BMX_STUBS_REF10(DEAD_STUB_REF, 0),  BMX_STUBS_REF10(DEAD_STUB_REF, 1),
    BMX_STUBS_REF10(DEAD_STUB_REF, 2),  BMX_STUBS_REF10(DEAD_STUB_REF, 3),
    BMX_STUBS_REF10(DEAD_STUB_REF, 4),  BMX_STUBS_REF10(DEAD_STUB_REF, 5),
    BMX_STUBS_REF10(DEAD_STUB_REF, 6),  BMX_STUBS_REF10(DEAD_STUB_REF, 7),
    BMX_STUBS_REF10(DEAD_STUB_REF, 8),  BMX_STUBS_REF10(DEAD_STUB_REF, 9),
    BMX_STUBS_REF10(DEAD_STUB_REF, 10), BMX_STUBS_REF10(DEAD_STUB_REF, 11),
    BMX_STUBS_REF10(DEAD_STUB_REF, 12), BMX_STUBS_REF10(DEAD_STUB_REF, 13),
    BMX_STUBS_REF10(DEAD_STUB_REF, 14), BMX_STUBS_REF10(DEAD_STUB_REF, 15)};
_Static_assert(sizeof kDeadStubs / sizeof kDeadStubs[0] == kMaxDeadSlots, "a C stub per dead slot");

static void *makeDeadStub(const char *dll, const char *name)
{
    const uint32_t index = (uint32_t)deadCount;

    if (deadCount == kMaxDeadSlots) {
        return NULL;
    }
    snprintf(deadNames[index], sizeof deadNames[index], "%s!%s", dll, name);
    deadCount++;
    return (void *)(uintptr_t)kDeadStubs[index];
}
#endif

#if defined(_WIN32)
void *bmxRealProc(const char *dll, const char *name)
{
    const HMODULE module = LoadLibraryA(dll);

    return module != NULL ? (void *)GetProcAddress(module, name) : NULL;
}
#else
/* The real Windows functions exist only on Windows: the checks that compare with them (BMX_INPUT_CHECK,
 * BMX_SOUND_CHECK, ...) report that they are not available. */
void *bmxRealProc(const char *dll, const char *name)
{
    (void)dll;
    (void)name;
    return NULL;
}
#endif

#if BMX_MACHINE_STUBS
/* Writes `jmp to` at from (one of our own functions: the opt-in replacements below). */
static void writeJump(uintptr_t from, const void *to)
{
    uint8_t *p = (uint8_t *)from;
    const intptr_t distance = (intptr_t)((uintptr_t)to - (from + 5));
    const int32_t rel = (int32_t)distance;

    if ((intptr_t)rel != distance) { /* 64-bit: both ends lie in our image, well inside +-2 GB */
        bmxLog("jump from %p to %p does not fit a rel32\n", (void *)from, to);
        abort();
    }

    p[0] = 0xE9;
    memcpy(p + 1, &rel, 4);
}
#endif

/* ---- original addresses
 * --------------------------------------------------------------------------------------- */

const void *bmxOrigFind(uint32_t orig)
{
    uint32_t lo = 0, hi = bmxOrigMapCount;

    while (lo < hi) {
        const uint32_t mid = (lo + hi) / 2;

        if (bmxOrigMap[mid].orig == orig) {
            return bmxOrigMap[mid].impl;
        }
        if (bmxOrigMap[mid].orig < orig) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    return NULL;
}

/*
 * Code addresses nothing serves get a stub each, `push <address>; call bmxOrigTrap`, so a call through such a
 * pointer reports the address and its caller. Made once per address.
 */
enum { kMaxTrapStubs = 64, kTrapStubSize = 16 };
static struct {
    uint32_t orig;
    uint8_t *stub;
} gTrapStubs[kMaxTrapStubs];
static int gTrapStubCount;

#if !BMX_MACHINE_STUBS
/* One C stub per trap slot, reporting the address recorded for its slot. */
#define TRAP_STUB_DEF(d, k)                                                                                  \
    static void trapStub_##d##_##k(void)                                                                     \
    {                                                                                                        \
        bmxOrigTrap(gTrapStubs[(d) * 10 + (k)].orig, (uint32_t)(uintptr_t)__builtin_return_address(0));      \
    }
#define TRAP_STUB_REF(d, k) trapStub_##d##_##k
BMX_STUBS_DEF10(TRAP_STUB_DEF, 0)
BMX_STUBS_DEF10(TRAP_STUB_DEF, 1)
BMX_STUBS_DEF10(TRAP_STUB_DEF, 2)
BMX_STUBS_DEF10(TRAP_STUB_DEF, 3)
BMX_STUBS_DEF10(TRAP_STUB_DEF, 4)
BMX_STUBS_DEF10(TRAP_STUB_DEF, 5)
BMX_STUBS_DEF4(TRAP_STUB_DEF, 6)
static void (*const kTrapStubs[])(void) = {
    BMX_STUBS_REF10(TRAP_STUB_REF, 0), BMX_STUBS_REF10(TRAP_STUB_REF, 1), BMX_STUBS_REF10(TRAP_STUB_REF, 2),
    BMX_STUBS_REF10(TRAP_STUB_REF, 3), BMX_STUBS_REF10(TRAP_STUB_REF, 4), BMX_STUBS_REF10(TRAP_STUB_REF, 5),
    BMX_STUBS_REF4(TRAP_STUB_REF, 6)};
_Static_assert(sizeof kTrapStubs / sizeof kTrapStubs[0] == kMaxTrapStubs, "a C stub per trap slot");
#endif

static void *trapStub(uint32_t orig)
{
    uint8_t *stub;

    for (int i = 0; i < gTrapStubCount; i++) {
        if (gTrapStubs[i].orig == orig) {
            return gTrapStubs[i].stub;
        }
    }
#if BMX_MACHINE_STUBS
    static uint8_t *page;
    int32_t rel;

    if (page == NULL) {
        page = VirtualAlloc(NULL, kMaxTrapStubs * kTrapStubSize, MEM_COMMIT | MEM_RESERVE,
                            PAGE_EXECUTE_READWRITE);
    }
    if (page == NULL || gTrapStubCount == kMaxTrapStubs) {
        bmxLog("loader: out of trap stubs at %08x\n", (unsigned)orig);
        bmxExitProcess(3);
    }
    stub = page + gTrapStubCount * kTrapStubSize;
    rel = (int32_t)((uintptr_t)bmxOrigTrap - ((uintptr_t)stub + 10));
    stub[0] = 0x68; /* push imm32 */
    memcpy(stub + 1, &orig, 4);
    stub[5] = 0xE8; /* call rel32 */
    memcpy(stub + 6, &rel, 4);
#else
    if (gTrapStubCount == kMaxTrapStubs) {
        bmxLog("loader: out of trap stubs at %08x\n", (unsigned)orig);
        bmxExitProcess(3);
    }
    stub = (uint8_t *)(uintptr_t)kTrapStubs[gTrapStubCount];
#endif
    gTrapStubs[gTrapStubCount].orig = orig;
    gTrapStubs[gTrapStubCount].stub = stub;
    gTrapStubCount++;
    return stub;
}

/* The original's code ends where its data begins (.rdata, the first data block). */
enum { kOrigCodeEnd = 0x0054D000 };

void *bmxOrigPtr(uint32_t addr)
{
    const void *fn;

    if (addr == 0) {
        return NULL;
    }
    if (addr >= kOrigCodeEnd) {
        return bmxDataFind(addr); /* a table or string in the original's data (RenderWare's plugin tables) */
    }
    fn = bmxOrigFind(addr);
    return fn != NULL ? (void *)(uintptr_t)fn : trapStub(addr);
}

void *bmxDataFind(uint32_t addr)
{
    uint32_t lo = 0, hi = bmxDataUnitCount;

    while (lo < hi) {
        const uint32_t mid = (lo + hi) / 2;

        if (addr < bmxDataUnits[mid].lo) {
            hi = mid;
        } else if (addr >= bmxDataUnits[mid].hi) {
            lo = mid + 1;
        } else {
            return (char *)(uintptr_t)bmxDataUnits[mid].at + (addr - bmxDataUnits[mid].lo);
        }
    }
    bmxLog("original data address %08x is in none of the definitions of src/data (dropped padding?)\n",
           (unsigned)addr);
    bmxExitProcess(3);
}

/* ---- the import slots
 * ------------------------------------------------------------------------------------------ */

#if defined(_WIN32)
typedef HMODULE BmxDll;
#define bmxDllLoad(name)      LoadLibraryA(name)
#define bmxDllProc(dll, name) ((void *)GetProcAddress((dll), (name)))
#else
/* No Windows DLLs: every slot must be a stand-in or dead (the Windows build has 0 real slots since S10). */
typedef void *BmxDll;
#define bmxDllLoad(name)      ((void)(name), (void *)NULL)
#define bmxDllProc(dll, name) ((void)(dll), (void)(name), (void *)NULL)
#endif

/*
 * Fills every import slot of the original's data (bmxImportSlots, src/data/imports.c): a stand-in from
 * src/platform, a trap stub for a dead slot, else the real Windows function. DLLs are loaded only for slots
 * that need the real one. The table is in the original's order, one DLL after another.
 */
static int fillImports(void)
{
    int standIns = 0, dead = 0, real = 0;
    char loaded[256] = "";

    for (uint32_t i = 0; i < bmxImportSlotCount;) {
        const char *const dllName = bmxImportSlots[i].dll;
        BmxDll dll = NULL;
        int dllStandIns = 0, dllDead = 0, dllReal = 0;

        if (bmxImportKeepsDll(dllName)) {
            dll = bmxDllLoad(dllName);
            if (!dll) {
                bmxLog("import: cannot load %s\n", dllName);
                return -1;
            }
        }
        for (; i < bmxImportSlotCount && strcmp(bmxImportSlots[i].dll, dllName) == 0; i++) {
            const char *const name = bmxImportSlots[i].name;
            void *fn = bmxStandInFind(dllName, name);

            if (fn != NULL) {
                dllStandIns++;
            } else if (bmxImportIsDead(dllName, name)) {
                fn = makeDeadStub(dllName, name);
                dllDead++;
            } else {
                if (!dll) {
                    dll = bmxDllLoad(dllName);
                }
                if (!dll) {
                    bmxLog("import: cannot load %s\n", dllName);
                    return -1;
                }
                fn = bmxDllProc(dll, name);
                dllReal++;
            }
            if (!fn) {
                bmxLog("import: %s!%s not found\n", dllName, name);
                return -1;
            }
            *bmxImportSlots[i].slot = fn;
        }
        bmxLog("imports: %-13s %3d stand-ins, %3d dead (trap stubs), %3d real\n", dllName, dllStandIns,
               dllDead, dllReal);
        if (dll != NULL && dllReal > 0) {
            snprintf(loaded + strlen(loaded), sizeof loaded - strlen(loaded), " %s", dllName);
        }
        standIns += dllStandIns;
        dead += dllDead;
        real += dllReal;
    }
    bmxLog("imports: %u slots filled: %d stand-ins, %d dead, %d real; real DLLs still used:%s\n",
           (unsigned)bmxImportSlotCount, standIns, dead, real, loaded[0] ? loaded : " none");
    return 0;
}

int bmxLoaderInit(void)
{
    if (fillImports() != 0) {
        return -1;
    }
    bmxLog("functions: %u original addresses served\n", (unsigned)bmxOrigMapCount);

    /* The opt-in replacements jump over the entries of our own ported functions, which the ported callers
     * call directly: [audio] smooth_resample, the interpolating mixer over winchanMix (winchanPump calls it);
     * [video] hud = 1 (with widescreen = 1), the ngifDrawList copy that anchors the HUD (driver/wide.c;
     * ngifFlushFrame calls it); [video] full_draw_distance over ngiffxAdaptQuality (drawdist.c; ngifRunFrame
     * calls it). Without machine-code jumps the providers have set the functions' redirect slots
     * (BMX_REDIRECT_VOID) instead. */
    {
        static const struct {
            const char *what;
            const BmxHook *(*jumps)(uint32_t *count);
        } kOptional[] = {
            {"smooth", bmxSmoothJumps},
            {"wide hud", drvWideJumps},
            {"draw distance", bmxDrawDistJumps},
        };

        for (size_t k = 0; k < sizeof kOptional / sizeof kOptional[0]; k++) {
            uint32_t count;
            const BmxHook *const jumps = kOptional[k].jumps(&count);

#if BMX_MACHINE_STUBS
            for (uint32_t i = 0; i < count; i++) {
                DWORD prot;

                if (!VirtualProtect((void *)jumps[i].orig, 5, PAGE_EXECUTE_READWRITE, &prot)) {
                    bmxLog("%s: cannot patch %p (error %lu)\n", kOptional[k].what, (void *)jumps[i].orig,
                           GetLastError());
                    return -1;
                }
                writeJump(jumps[i].orig, jumps[i].impl);
            }
            bmxLog("%s: %u jumps\n", kOptional[k].what, (unsigned)count);
#else
            (void)jumps;
            bmxLog("%s: %u redirects\n", kOptional[k].what, (unsigned)count);
#endif
        }
    }
    /* [video] widescreen: the hor+ wrapper in the zgl camera slot 0x561d98 (data, no jump; driver/wide.c). */
    drvWideInstall();
    bmxSimHashInstall();
    /* [audio] music_dir: the music override attaches at the DirectSound stand-in (no jump; musicgame.c). */
    bmxMusicInstall();
#if BMX_MACHINE_STUBS
    FlushInstructionCache(GetCurrentProcess(), NULL, 0);
#endif
    return 0;
}
