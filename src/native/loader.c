/*
 * Loads the original NgBMX.exe's data (not its code) into an allocation of ours and relocates it: the
 * pointers in it to data move with it, the ones to code become the functions serving those addresses. See
 * docs/NATIVE.md "Relocated data image".
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>

#include <bcrypt.h>

#include "driver/wide.h"
#include "hook/hook.h"
#include "native/native.h"
#include "platform/music.h"
#include "platform/platform.h"
#include "platform/smoothmix.h"

/* Same value as tools/known_exe.txt. */
static const char kKnownSha256[] = "97aa9077501f264a1f96fc2ff1ff8d4326be094dd98f2a647c913de9d60d09a2";

static uint8_t *readFile(const char *path, size_t *size)
{
    FILE *f = fopen(path, "rb");
    uint8_t *data;

    if (!f) {
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    *size = (size_t)ftell(f);
    fseek(f, 0, SEEK_SET);
    data = malloc(*size);
    if (data && fread(data, 1, *size, f) != *size) {
        free(data);
        data = NULL;
    }
    fclose(f);
    return data;
}

static int sha256Hex(const uint8_t *data, size_t size, char out[65])
{
    uint8_t digest[32];

    if (BCryptHash(BCRYPT_SHA256_ALG_HANDLE, NULL, 0, (PUCHAR)(uintptr_t)data, (ULONG)size, digest, 32) !=
        0) {
        return -1;
    }
    for (int i = 0; i < 32; i++) {
        snprintf(out + i * 2, 3, "%02x", digest[i]);
    }
    return 0;
}

/*
 * Dead slots (docs/PLATFORM.md 1b): the original code that calls them is int3 in this exe. Each gets a stub
 * in place of the real function, so the DLLs are not loaded for them; if one is called anyway the stub
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
static void writeJump(uint32_t from, const void *to)
{
    uint8_t *p = (uint8_t *)(uintptr_t)from;
    const int32_t rel = (int32_t)((uintptr_t)to - (from + 5));

    p[0] = 0xE9;
    memcpy(p + 1, &rel, 4);
}

/* ---- the relocated data image ------------------------------------------------------------------------- */

uintptr_t gBmxImageDelta;

/* The copied range (original addresses): .rdata, .data with its zero tail, .idata, .data1, _rwdseg, .rsrc. */
static uint32_t gImageLo, gImageHi;

/* Original address -> its place in the copy (inside the image only). */
static void *imagePtr(uint32_t orig)
{
    return (void *)((uintptr_t)orig + gBmxImageDelta);
}

/*
 * The incremental linker's thunk table in .text: 1130 `jmp <function>` (E9 rel32). Function pointers in the
 * data point at these thunks; a pointer to one stands for the function the thunk jumps to.
 */
enum { kThunkLo = 0x00401005, kThunkHi = 0x00402617 };
static uint8_t gThunks[kThunkHi - kThunkLo];

/*
 * Data the linker put into .text: dinput.lib's object tables of c_dfDIJoystick (0x2c entries at 0x4afc60) and
 * c_dfDIKeyboard (0x100 entries at 0x4aff20), which the data formats in .rdata point at. Only the real
 * DirectInput reads them (BMX_INPUT_CHECK passes it the game's formats); our stand-in takes just the format's
 * data size. The formats' pointers are relocated into this copy.
 */
enum { kInputTablesLo = 0x004afc60, kInputTablesHi = 0x004b0f20 };
static uint8_t gInputTables[kInputTablesHi - kInputTablesLo];

/* The exe's own `jmp [import slot]` thunks in its code (FF 25 <slot>): thunk address and slot, original. */
enum { kMaxImportThunks = 256 };
static struct {
    uint32_t thunk, slot;
} gImportThunks[kMaxImportThunks];
static int gImportThunkCount;

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
 * pointer reports the address and its caller, as the int3 traps did. Made once per address.
 */
enum { kMaxTrapStubs = 256, kTrapStubSize = 16 };
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

/* How an original address was translated (for the loader's counts). */
enum { kXlatData, kXlatCode, kXlatThunk, kXlatImport, kXlatInputTables, kXlatTrap, kXlatCount };

/* Where an original pointer value points now: into the copy for data, at our function for code. */
static void *translate(uint32_t orig, int *how)
{
    const void *fn;

    if (orig >= gImageLo && orig <= gImageHi) { /* one past the end is still a data pointer */
        *how = kXlatData;
        return imagePtr(orig);
    }
    fn = bmxOrigFind(orig);
    if (fn != NULL) {
        *how = kXlatCode;
        return (void *)(uintptr_t)fn;
    }
    if (orig >= kThunkLo && orig + 5 <= kThunkHi && gThunks[orig - kThunkLo] == 0xE9) {
        int32_t rel;

        memcpy(&rel, gThunks + (orig - kThunkLo) + 1, 4);
        fn = bmxOrigFind(orig + 5 + (uint32_t)rel);
        if (fn != NULL) {
            *how = kXlatThunk;
            return (void *)(uintptr_t)fn;
        }
        orig = orig + 5 + (uint32_t)rel; /* name the function in the report, not the thunk */
    }
    for (int i = 0; i < gImportThunkCount; i++) {
        if (gImportThunks[i].thunk == orig) {
            *how = kXlatImport;
            return *(void **)imagePtr(gImportThunks[i].slot);
        }
    }
    if (orig >= kInputTablesLo && orig < kInputTablesHi) {
        *how = kXlatInputTables;
        return gInputTables + (orig - kInputTablesLo);
    }
    *how = kXlatTrap;
    return trapStub(orig);
}

void *bmxOrigPtr(uint32_t addr)
{
    int how;

    return addr != 0 ? translate(addr, &how) : NULL;
}

/* Fills every import slot (a stand-in from src/platform, a trap stub for a dead slot, else the real Windows
 * function) and reports the slot range (original addresses) for the thunk scan. DLLs are loaded only for
 * slots that need the real one. */
static int fillImports(const IMAGE_NT_HEADERS32 *nt, uint32_t *iatLo, uint32_t *iatHi)
{
    const IMAGE_DATA_DIRECTORY dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    const IMAGE_IMPORT_DESCRIPTOR *imp = imagePtr(kBmxImageBase + dir.VirtualAddress);
    int count = 0, standIns = 0, dead = 0, real = 0;
    char loaded[256] = "";

    *iatLo = 0xffffffffu;
    *iatHi = 0;
    for (; imp->Name; imp++) {
        const char *dllName = imagePtr(kBmxImageBase + imp->Name);
        HMODULE dll = NULL;
        int dllStandIns = 0, dllDead = 0, dllReal = 0;
        const uint32_t *names =
            imagePtr(kBmxImageBase + (imp->OriginalFirstThunk ? imp->OriginalFirstThunk : imp->FirstThunk));
        FARPROC *slots = (FARPROC *)imagePtr(kBmxImageBase + imp->FirstThunk);

        if (bmxImportKeepsDll(dllName)) {
            dll = LoadLibraryA(dllName);
            if (!dll) {
                bmxLog("import: cannot load %s\n", dllName);
                return -1;
            }
        }
        for (; *names; names++, slots++) {
            const int byOrdinal = (*names & 0x80000000u) != 0;
            const char *name = byOrdinal ? (const char *)(uintptr_t)(*names & 0xffff)
                                         : (const char *)imagePtr(kBmxImageBase + *names + 2);
            const uint32_t slot = (uint32_t)((uintptr_t)slots - gBmxImageDelta);
            FARPROC fn = NULL;

            if (!byOrdinal && bmxStandInFind(dllName, name) != NULL) {
                fn = (FARPROC)bmxStandInFind(dllName, name);
                dllStandIns++;
            } else if (!byOrdinal && bmxImportIsDead(dllName, name)) {
                fn = (FARPROC)makeDeadStub(dllName, name);
                dllDead++;
            } else {
                if (!dll) {
                    dll = LoadLibraryA(dllName);
                }
                if (!dll) {
                    bmxLog("import: cannot load %s\n", dllName);
                    return -1;
                }
                fn = GetProcAddress(dll, name);
                dllReal++;
            }
            if (!fn) {
                bmxLog("import: %s!%s not found\n", dllName, byOrdinal ? "(ordinal)" : name);
                return -1;
            }
            *slots = fn;
            *iatLo = slot < *iatLo ? slot : *iatLo;
            *iatHi = slot + 4 > *iatHi ? slot + 4 : *iatHi;
            count++;
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
    bmxLog("imports: %d slots filled: %d stand-ins, %d dead, %d real; real DLLs still used:%s\n", count,
           standIns, dead, real, loaded[0] ? loaded : " none");
    return 0;
}

/*
 * The exe's own `jmp [import slot]` stubs in its code (e.g. its timeGetTime wrapper, the AVIFile calls): the
 * generated stubs of the ones ported code calls (bmxOrigImports) jump through the same slot in the copy; a
 * pointer to one in the data becomes the slot's function.
 */
static int findImportThunks(const uint8_t *file, const IMAGE_SECTION_HEADER *sec, int sections,
                            uint32_t iatLo, uint32_t iatHi)
{
    for (int i = 0; i < sections; i++) {
        if (!(sec[i].Characteristics & IMAGE_SCN_CNT_CODE)) {
            continue;
        }
        for (uint32_t off = 0; off + 6 <= sec[i].SizeOfRawData; off++) {
            const uint8_t *p = file + sec[i].PointerToRawData + off;
            uint32_t slot;

            if (p[0] != 0xFF || p[1] != 0x25) {
                continue;
            }
            memcpy(&slot, p + 2, 4);
            if (slot >= iatLo && slot < iatHi && gImportThunkCount < kMaxImportThunks) {
                gImportThunks[gImportThunkCount].thunk = kBmxImageBase + sec[i].VirtualAddress + off;
                gImportThunks[gImportThunkCount].slot = slot;
                gImportThunkCount++;
            }
        }
    }
    for (uint32_t k = 0; k < bmxOrigImportCount; k++) {
        int found = -1;

        for (int i = 0; i < gImportThunkCount && found < 0; i++) {
            found = gImportThunks[i].thunk == bmxOrigImports[k].orig ? i : -1;
        }
        if (found < 0) {
            bmxLog("loader: %08x (%s) is not an import thunk in this exe\n", (unsigned)bmxOrigImports[k].orig,
                   bmxOrigImports[k].name);
            return -1;
        }
        *bmxOrigImports[k].slotRef = (void **)imagePtr(gImportThunks[found].slot);
    }
    bmxLog("import thunks: %d in the exe, %u called by ported code\n", gImportThunkCount,
           (unsigned)bmxOrigImportCount);
    return 0;
}

/*
 * Applies the exe's base relocations (type 3, HIGHLOW) to every location inside the copied data: each is a
 * pointer the linker wrote as an absolute original address. Data pointers move with the copy; code pointers
 * become our functions (translate). Locations in the code sections are the original's code: irrelevant.
 */
static int relocate(const uint8_t *file, const IMAGE_NT_HEADERS32 *nt, const IMAGE_SECTION_HEADER *sec)
{
    const IMAGE_DATA_DIRECTORY dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    const uint8_t *block = NULL, *end;
    int counts[kXlatCount] = {0}, total = 0, other = 0;
    const int verbose = getenv("BMX_RELOC_LOG") != NULL;

    /* The directory's RVA in the file: .reloc is not part of the copy. */
    for (int i = 0; i < nt->FileHeader.NumberOfSections; i++) {
        if (dir.VirtualAddress >= sec[i].VirtualAddress &&
            dir.VirtualAddress + dir.Size <= sec[i].VirtualAddress + sec[i].SizeOfRawData) {
            block = file + sec[i].PointerToRawData + (dir.VirtualAddress - sec[i].VirtualAddress);
        }
    }
    if (block == NULL) {
        bmxLog("loader: no relocations in the exe\n");
        return -1;
    }
    end = block + dir.Size;
    while (block + 8 <= end) {
        const IMAGE_BASE_RELOCATION *br = (const IMAGE_BASE_RELOCATION *)block;
        const uint16_t *entry = (const uint16_t *)(block + 8);
        const uint32_t n = (br->SizeOfBlock - 8) / 2;

        if (br->SizeOfBlock < 8) {
            break;
        }
        for (uint32_t i = 0; i < n; i++) {
            const uint32_t at = kBmxImageBase + br->VirtualAddress + (entry[i] & 0xfff);
            int how;
            uint32_t value;
            void *now;

            if (entry[i] >> 12 == IMAGE_REL_BASED_ABSOLUTE) {
                continue; /* padding */
            }
            if (entry[i] >> 12 != IMAGE_REL_BASED_HIGHLOW) {
                bmxLog("loader: relocation type %u at %08x not supported\n", entry[i] >> 12, (unsigned)at);
                return -1;
            }
            if (at < gImageLo || at + 4 > gImageHi) {
                other++;
                continue;
            }
            memcpy(&value, imagePtr(at), 4);
            now = translate(value, &how);
            if (how == kXlatTrap && verbose) {
                char name[160];

                bmxDescribeAddress(value, name, sizeof name);
                bmxLog("relocation: pointer at %08x to %08x %s: nothing serves it (trap stub)\n",
                       (unsigned)at, (unsigned)value, name);
            }
            memcpy(imagePtr(at), (const void *)&now, 4);
            counts[how]++;
            total++;
        }
        block += br->SizeOfBlock;
    }
    /* The trap stubs are mostly switch tables of unported code (CRT internals, the D3D7 driver's raster
     * functions) and the D3D7 driver's own entry points: never reached. BMX_RELOC_LOG=1 lists them. */
    bmxLog(
        "relocations: %d in the data (%d data, %d code, %d via thunks, %d imports, %d input tables, %d trap "
        "stubs for %d addresses), %d in the code (ignored)\n",
        total, counts[kXlatData], counts[kXlatCode], counts[kXlatThunk], counts[kXlatImport],
        counts[kXlatInputTables], counts[kXlatTrap], gTrapStubCount, other);
    return 0;
}

/* bmxLoadImage on the file's bytes (freed by the caller). */
static int loadImage(const uint8_t *file, size_t size)
{
    const IMAGE_DOS_HEADER *dos;
    const IMAGE_NT_HEADERS32 *nt;
    const IMAGE_SECTION_HEADER *sec;
    char hex[65];
    uint32_t iatLo, iatHi;
    uint8_t *image;

    if (sha256Hex(file, size, hex) != 0 || strcmp(hex, kKnownSha256) != 0) {
        bmxLog("NgBMX.exe is not the supported one (sha256 %s)\n", hex);
        return -1;
    }
    dos = (const IMAGE_DOS_HEADER *)file;
    nt = (const IMAGE_NT_HEADERS32 *)(file + dos->e_lfanew);
    if (nt->OptionalHeader.ImageBase != kBmxImageBase) {
        bmxLog("unexpected image base %#lx\n", (unsigned long)nt->OptionalHeader.ImageBase);
        return -1;
    }

    /* The data sections (all but the code and the relocations), contiguous from .rdata to .rsrc. */
    sec = IMAGE_FIRST_SECTION(nt);
    gImageLo = 0xffffffffu;
    gImageHi = 0;
    for (int i = 0; i < nt->FileHeader.NumberOfSections; i++) {
        const uint32_t start = kBmxImageBase + sec[i].VirtualAddress;
        const uint32_t vend = start + ((sec[i].Misc.VirtualSize + 0xfff) & ~0xfffu);

        if ((sec[i].Characteristics & IMAGE_SCN_CNT_CODE) ||
            sec[i].VirtualAddress ==
                nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress) {
            continue;
        }
        gImageLo = start < gImageLo ? start : gImageLo;
        gImageHi = vend > gImageHi ? vend : gImageHi;
    }
    image = VirtualAlloc(NULL, gImageHi - gImageLo, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (image == NULL) {
        bmxLog("cannot allocate the data image (%#x bytes, error %lu)\n", (unsigned)(gImageHi - gImageLo),
               GetLastError());
        return -1;
    }
    gBmxImageDelta = (uintptr_t)image - gImageLo;
    for (int i = 0; i < nt->FileHeader.NumberOfSections; i++) {
        const uint32_t start = kBmxImageBase + sec[i].VirtualAddress;
        const uint32_t vsize = sec[i].Misc.VirtualSize;
        const uint32_t raw = sec[i].SizeOfRawData < vsize ? sec[i].SizeOfRawData : vsize;
        const int isData = start >= gImageLo && start < gImageHi;

        if (isData) {
            memcpy(imagePtr(start), file + sec[i].PointerToRawData, raw); /* the tail stays zero */
        } else if (sec[i].Characteristics & IMAGE_SCN_CNT_CODE) {
            /* Kept from the code: the incremental link thunks and the input object tables (see above). */
            const uint32_t keep[2][2] = {{kThunkLo, kThunkHi}, {kInputTablesLo, kInputTablesHi}};
            uint8_t *const copy[2] = {gThunks, gInputTables};

            for (int k = 0; k < 2; k++) {
                if (keep[k][0] >= start && keep[k][1] <= start + raw) {
                    memcpy(copy[k], file + sec[i].PointerToRawData + (keep[k][0] - start),
                           keep[k][1] - keep[k][0]);
                }
            }
        }
        bmxLog("section %-8.8s %08x size %07x %s\n", (const char *)sec[i].Name, (unsigned)start,
               (unsigned)vsize, isData ? "data, copied" : "not loaded");
    }
    bmxLog("image: %08x..%08x at %p (delta %#lx), no code\n", (unsigned)gImageLo, (unsigned)gImageHi,
           (void *)image, (unsigned long)gBmxImageDelta);

    if (fillImports(nt, &iatLo, &iatHi) != 0 ||
        findImportThunks(file, sec, nt->FileHeader.NumberOfSections, iatLo, iatHi) != 0 ||
        relocate(file, nt, sec) != 0) {
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

                if (!VirtualProtect((void *)(uintptr_t)jumps[i].orig, 5, PAGE_EXECUTE_READWRITE, &prot)) {
                    bmxLog("%s: cannot patch %#x (error %lu)\n", kOptional[k].what, (unsigned)jumps[i].orig,
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

int bmxLoadImage(const char *exePath)
{
    size_t size;
    uint8_t *file = readFile(exePath, &size);
    int result;

    if (!file) {
        bmxLog("cannot read %s (run from the game folder)\n", exePath);
        return -1;
    }
    result = loadImage(file, size);
    free(file);
    return result;
}
