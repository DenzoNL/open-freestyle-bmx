/*
 * Rebuilds the original NgBMX.exe address space at its link base without its code. See docs/NATIVE.md.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>

#include <bcrypt.h>

#include "driver/driver.h"
#include "driver/wide.h"
#include "hook/hook.h"
#include "native/native.h"
#include "platform/music.h"
#include "platform/platform.h"
#include "platform/smoothmix.h"
#include "png/png.h"

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

/* Fills every import slot (a stand-in from src/platform, a trap stub for a dead slot, else the real Windows
 * function) and reports the slot range for the thunk scan. DLLs are loaded only for slots that need the real
 * one. */
static int fillImports(const IMAGE_NT_HEADERS32 *nt, uint32_t *iatLo, uint32_t *iatHi)
{
    const IMAGE_DATA_DIRECTORY dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    const IMAGE_IMPORT_DESCRIPTOR *imp =
        (const IMAGE_IMPORT_DESCRIPTOR *)(kBmxImageBase + dir.VirtualAddress);
    int count = 0, standIns = 0, dead = 0, real = 0;
    char loaded[256] = "";

    *iatLo = 0xffffffffu;
    *iatHi = 0;
    for (; imp->Name; imp++) {
        const char *dllName = (const char *)(kBmxImageBase + imp->Name);
        HMODULE dll = NULL;
        int dllStandIns = 0, dllDead = 0, dllReal = 0;
        const uint32_t *names =
            (const uint32_t *)(kBmxImageBase +
                               (imp->OriginalFirstThunk ? imp->OriginalFirstThunk : imp->FirstThunk));
        FARPROC *slots = (FARPROC *)(kBmxImageBase + imp->FirstThunk);

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
                                         : (const char *)(kBmxImageBase + *names + 2);
            const uint32_t slot = (uint32_t)(uintptr_t)slots;
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

/* Parts of the original's .text that are kept as they are instead of int3 (start, end, only for the
 * BMX_INPUT_CHECK dev check). */
static const struct {
    uint32_t start, end;
    int inputCheckOnly;
} kTextKeep[] = {
    /* The incremental linker's thunk table: 1130 `jmp <function>`. Function pointers in the data point at
     * these thunks; the jump then lands on the function's own address (our port or a trap). */
    {0x00401005, 0x00402617, 0},
    /* Data the linker put into .text: dinput.lib's object tables of c_dfDIJoystick (0x2c entries at 0x4afc60)
     * and c_dfDIKeyboard (0x100 entries at 0x4aff20), which the data formats in .rdata point at. Only the
     * real DirectInput reads them (BMX_INPUT_CHECK passes it the game's formats); our stand-in takes just the
     * format's data size, so they are kept only for that check. */
    {0x004afc60, 0x004b0f20, 1},
    /* RpWorldDestroy: a four byte stub (`mov eax, [esp+4]`) that falls into the ported body's jump at
     * 0x4ba9c4 (rpWorldDestroyBody, rw/world.c). */
    {0x004ba9c0, 0x004ba9c4, 0},
};

static void writeJump(uint32_t from, const void *to)
{
    uint8_t *p = (uint8_t *)(uintptr_t)from;
    const int32_t rel = (int32_t)((uintptr_t)to - (from + 5));

    p[0] = 0xE9;
    memcpy(p + 1, &rel, 4);
}

/* bmxLoadImage on the file's bytes (freed by the caller). */
static int loadImage(const uint8_t *file, size_t size)
{
    const IMAGE_DOS_HEADER *dos;
    const IMAGE_NT_HEADERS32 *nt;
    const IMAGE_SECTION_HEADER *sec;
    char hex[65];
    uint32_t iatLo, iatHi;
    DWORD oldProt;
    int thunks = 0;
    const int pngCheck = getenv("BMX_PNG_CHECK") != NULL;
    const char *const inputEnv = getenv("BMX_INPUT_CHECK");
    const int inputCheck = inputEnv != NULL && *inputEnv != 0 && *inputEnv != '0';

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
    /* The range is already ours: the .orig section of this exe (src/native/range.c). */
    if (nt->OptionalHeader.SizeOfImage > kBmxOrigRangeSize ||
        !VirtualProtect((void *)kBmxImageBase, nt->OptionalHeader.SizeOfImage, PAGE_EXECUTE_READWRITE,
                        &oldProt)) {
        bmxLog("cannot take %#x..%#x (error %lu): .orig section missing or too small\n", kBmxImageBase,
               kBmxImageBase + (unsigned)nt->OptionalHeader.SizeOfImage, GetLastError());
        return -1;
    }
    memcpy((void *)kBmxImageBase, file, nt->OptionalHeader.SizeOfHeaders);

    sec = IMAGE_FIRST_SECTION(nt);
    for (int i = 0; i < nt->FileHeader.NumberOfSections; i++) {
        uint8_t *dst = (uint8_t *)(kBmxImageBase + sec[i].VirtualAddress);
        const uint32_t vsize = sec[i].Misc.VirtualSize;
        const uint32_t raw = sec[i].SizeOfRawData < vsize ? sec[i].SizeOfRawData : vsize;
        const int isCode = (sec[i].Characteristics & IMAGE_SCN_CNT_CODE) != 0;

        if (isCode) {
            const uint32_t start = kBmxImageBase + sec[i].VirtualAddress;

            memset(dst, 0xCC, vsize);
            if (pngCheck && kBmxPngCodeStart >= start && kBmxPngCodeEnd <= start + raw) {
                bmxPngCheckInstall(file + sec[i].PointerToRawData, start);
            }
            for (size_t k = 0; k < sizeof kTextKeep / sizeof kTextKeep[0]; k++) {
                if ((!kTextKeep[k].inputCheckOnly || inputCheck) && kTextKeep[k].start >= start &&
                    kTextKeep[k].end <= start + raw) {
                    memcpy((void *)(uintptr_t)kTextKeep[k].start,
                           file + sec[i].PointerToRawData + (kTextKeep[k].start - start),
                           kTextKeep[k].end - kTextKeep[k].start);
                }
            }
        } else {
            memcpy(dst, file + sec[i].PointerToRawData, raw);
        }
        bmxLog("section %-8.8s %08x size %07x %s\n", (const char *)sec[i].Name,
               (unsigned)(kBmxImageBase + sec[i].VirtualAddress), (unsigned)vsize, isCode ? "int3" : "data");
    }

    if (fillImports(nt, &iatLo, &iatHi) != 0) {
        return -1;
    }

    /* The exe's own `jmp [import slot]` stubs (e.g. its timeGetTime wrapper) are not game code: recreate
     * them. */
    for (int i = 0; i < nt->FileHeader.NumberOfSections; i++) {
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
            if (slot >= iatLo && slot < iatHi) {
                memcpy((void *)(kBmxImageBase + sec[i].VirtualAddress + off), p, 6);
                thunks++;
            }
        }
    }
    bmxLog("import thunks: %d\n", thunks);

    for (uint32_t i = 0; i < bmxHookCount; i++) {
        const uint32_t a = bmxHooks[i].orig;

        if (a < kBmxImageBase || a + 5 > kBmxImageBase + nt->OptionalHeader.SizeOfImage) {
            bmxLog("hook %#x outside the image\n", (unsigned)a);
            return -1;
        }
        writeJump(a, bmxHooks[i].impl);
    }
    bmxLog("hooks: %u jumps\n", (unsigned)bmxHookCount);
    /* The SDL3 driver replaces the original's D3D7 driver entry points (native exe only, see src/driver). */
    for (uint32_t i = 0; i < bmxDriverJumpCount; i++) {
        writeJump(bmxDriverJumps[i].orig, bmxDriverJumps[i].impl);
    }
    bmxLog("driver: %u jumps\n", (unsigned)bmxDriverJumpCount);
    /* Our PNG reader replaces libpng's entry points the same way (src/png). */
    for (uint32_t i = 0; i < bmxPngJumpCount; i++) {
        writeJump(bmxPngJumps[i].orig, bmxPngJumps[i].impl);
    }
    bmxLog("png: %u jumps\n", (unsigned)bmxPngJumpCount);
    /* [audio] smooth_resample: the opt-in interpolating mixer replaces the ported winchanMix (which
     * winchanPump calls directly: the jump goes over our own function's entry in the exe's .text). */
    {
        uint32_t smoothCount;
        const BmxHook *const smooth = bmxSmoothJumps(&smoothCount);

        for (uint32_t i = 0; i < smoothCount; i++) {
            DWORD prot;

            if (!VirtualProtect((void *)(uintptr_t)smooth[i].orig, 5, PAGE_EXECUTE_READWRITE, &prot)) {
                bmxLog("smooth: cannot patch %#x (error %lu)\n", (unsigned)smooth[i].orig, GetLastError());
                return -1;
            }
            writeJump(smooth[i].orig, smooth[i].impl);
        }
        bmxLog("smooth: %u jumps\n", (unsigned)smoothCount);
    }
    /* [video] hud = 1 (with widescreen = 1): the ngifDrawList copy that anchors the HUD (driver/wide.c), the
     * same way over the ported ngifDrawList's entry (ngifFlushFrame calls it directly). */
    {
        uint32_t wideCount;
        const BmxHook *const wide = drvWideJumps(&wideCount);

        for (uint32_t i = 0; i < wideCount; i++) {
            DWORD prot;

            if (!VirtualProtect((void *)(uintptr_t)wide[i].orig, 5, PAGE_EXECUTE_READWRITE, &prot)) {
                bmxLog("wide: cannot patch %#x (error %lu)\n", (unsigned)wide[i].orig, GetLastError());
                return -1;
            }
            writeJump(wide[i].orig, wide[i].impl);
        }
        bmxLog("wide: %u hud jumps\n", (unsigned)wideCount);
    }
    /* [video] widescreen: the hor+ wrapper in the zgl camera slot 0x561d98 (data, no jump; driver/wide.c). */
    drvWideInstall();
    bmxSimHashInstall();
    /* [audio] music_dir: the music override attaches at the DirectSound stand-in (no jump; musicgame.c). */
    bmxMusicInstall();
    if (pngCheck) {
        bmxPngCheckArm();
    }
    bmxCrtInstall();
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
