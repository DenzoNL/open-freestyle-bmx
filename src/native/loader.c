/*
 * Start-up of the native exe's original-side state: the import slots of the original's data
 * (src/data/imports.c) get their functions, original code addresses that RenderWare passes around become
 * functions, and the opt-in jumps over our own functions are written. Nothing is read from NgBMX.exe: its
 * data is in the source (src/data, docs/NATIVE.md "The original's data in the source").
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>

#include "driver/wide.h"
#include "hook/hook.h"
#include "native/native.h"
#include "platform/music.h"
#include "platform/platform.h"
#include "platform/smoothmix.h"

/*
 * Dead slots (docs/PLATFORM.md 1b): the original code that calls them is not part of this exe. Each gets a
 * stub in place of the real function, so the DLLs are not loaded for them; if one is called anyway the stub
 * reports which function and from where, and the process stops. A stub is `push <slot index>; call deadSlot`:
 * deadSlot then sees (index, caller's return address) as its two cdecl arguments.
 */
enum { kMaxDeadSlots = 160, kDeadStubSize = 16 };

static char deadNames[kMaxDeadSlots][64];
static int deadCount;

static void __attribute__((noreturn, cdecl)) deadSlot(uint32_t index, uint32_t callerRet)
{
    bmxLog("DEAD IMPORT called: %s (the native exe should never reach it), caller %08x\n",
           index < kMaxDeadSlots ? deadNames[index] : "?", (unsigned)callerRet);
    ExitProcess(3);
}

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

void *bmxRealProc(const char *dll, const char *name)
{
    const HMODULE module = LoadLibraryA(dll);

    return module != NULL ? (void *)GetProcAddress(module, name) : NULL;
}

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

static void *trapStub(uint32_t orig)
{
    static uint8_t *page;
    uint8_t *stub;
    int32_t rel;

    for (int i = 0; i < gTrapStubCount; i++) {
        if (gTrapStubs[i].orig == orig) {
            return gTrapStubs[i].stub;
        }
    }
    if (page == NULL) {
        page = VirtualAlloc(NULL, kMaxTrapStubs * kTrapStubSize, MEM_COMMIT | MEM_RESERVE,
                            PAGE_EXECUTE_READWRITE);
    }
    if (page == NULL || gTrapStubCount == kMaxTrapStubs) {
        bmxLog("loader: out of trap stubs at %08x\n", (unsigned)orig);
        ExitProcess(3);
    }
    stub = page + gTrapStubCount * kTrapStubSize;
    rel = (int32_t)((uintptr_t)bmxOrigTrap - ((uintptr_t)stub + 10));
    stub[0] = 0x68; /* push imm32 */
    memcpy(stub + 1, &orig, 4);
    stub[5] = 0xE8; /* call rel32 */
    memcpy(stub + 6, &rel, 4);
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
    ExitProcess(3);
}

/* ---- the import slots
 * ------------------------------------------------------------------------------------------ */

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
        HMODULE dll = NULL;
        int dllStandIns = 0, dllDead = 0, dllReal = 0;

        if (bmxImportKeepsDll(dllName)) {
            dll = LoadLibraryA(dllName);
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
                    dll = LoadLibraryA(dllName);
                }
                if (!dll) {
                    bmxLog("import: cannot load %s\n", dllName);
                    return -1;
                }
                fn = (void *)GetProcAddress(dll, name);
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
     * calls it). */
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
        }
    }
    /* [video] widescreen: the hor+ wrapper in the zgl camera slot 0x561d98 (data, no jump; driver/wide.c). */
    drvWideInstall();
    bmxSimHashInstall();
    /* [audio] music_dir: the music override attaches at the DirectSound stand-in (no jump; musicgame.c). */
    bmxMusicInstall();
    FlushInstructionCache(GetCurrentProcess(), NULL, 0);
    return 0;
}
