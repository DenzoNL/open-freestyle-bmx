/*
 * Rebuilds the original NgBMX.exe address space at its link base without its code. See docs/NATIVE.md.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>

#include <bcrypt.h>

#include "driver/driver.h"
#include "hook/hook.h"
#include "native/native.h"

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
 * AVISaveOptionsFree (AVIFIL32): zmovieClose frees each stream's lpFormat with the CRT (zmemFree) and then
 * hands the AVICOMPRESSOPTIONS to this function, which GlobalFrees lpFormat and lpParms and clears them. In
 * the original the block comes from MSVC6's own heap and that GlobalFree fails harmlessly; with our CRT the
 * block is the process heap's and the second free is fatal (heap corruption). Only the clearing remains.
 */
static LONG WINAPI shimAviSaveOptionsFree(int count, void **options)
{
    for (int i = 0; i < count; i++) {
        uint8_t *const o = options[i];

        memset(o + 0x18, 0, 4); /* lpFormat */
        memset(o + 0x20, 0, 4); /* lpParms */
    }
    return 0;
}

static const struct {
    const char *name;
    FARPROC fn;
} kImportShims[] = {
    {"AVISaveOptionsFree", (FARPROC)(void *)shimAviSaveOptionsFree},
};

/* Fills every import slot with the real Windows function (or its shim); reports the slot range for the thunk
 * scan. */
static int fillImports(const IMAGE_NT_HEADERS32 *nt, uint32_t *iatLo, uint32_t *iatHi)
{
    const IMAGE_DATA_DIRECTORY dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    const IMAGE_IMPORT_DESCRIPTOR *imp =
        (const IMAGE_IMPORT_DESCRIPTOR *)(kBmxImageBase + dir.VirtualAddress);
    int count = 0;

    *iatLo = 0xffffffffu;
    *iatHi = 0;
    for (; imp->Name; imp++) {
        const char *dllName = (const char *)(kBmxImageBase + imp->Name);
        const HMODULE dll = LoadLibraryA(dllName);
        const uint32_t *names =
            (const uint32_t *)(kBmxImageBase +
                               (imp->OriginalFirstThunk ? imp->OriginalFirstThunk : imp->FirstThunk));
        FARPROC *slots = (FARPROC *)(kBmxImageBase + imp->FirstThunk);

        if (!dll) {
            bmxLog("import: cannot load %s\n", dllName);
            return -1;
        }
        for (; *names; names++, slots++) {
            const int byOrdinal = (*names & 0x80000000u) != 0;
            const char *name = byOrdinal ? (const char *)(uintptr_t)(*names & 0xffff)
                                         : (const char *)(kBmxImageBase + *names + 2);
            FARPROC fn = GetProcAddress(dll, name);
            const uint32_t slot = (uint32_t)(uintptr_t)slots;

            for (size_t s = 0; !byOrdinal && s < sizeof kImportShims / sizeof kImportShims[0]; s++) {
                if (strcmp(name, kImportShims[s].name) == 0) {
                    fn = kImportShims[s].fn;
                }
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
    }
    bmxLog("imports: %d slots filled\n", count);
    return 0;
}

/* Parts of the original's .text that are kept as they are instead of int3 (start, end). */
static const struct {
    uint32_t start, end;
} kTextKeep[] = {
    /* The incremental linker's thunk table: 1130 `jmp <function>`. Function pointers in the data point at
     * these thunks; the jump then lands on the function's own address (our port or a trap). */
    {0x00401005, 0x00402617},
    /* Data the linker put into .text: dinput.lib's object tables of c_dfDIJoystick (0x2c entries at 0x4afc60)
     * and c_dfDIKeyboard (0x100 entries at 0x4aff20), which the data formats in .rdata point at. */
    {0x004afc60, 0x004b0f20},
};

static void writeJump(uint32_t from, const void *to)
{
    uint8_t *p = (uint8_t *)(uintptr_t)from;
    const int32_t rel = (int32_t)((uintptr_t)to - (from + 5));

    p[0] = 0xE9;
    memcpy(p + 1, &rel, 4);
}

int bmxLoadImage(const char *exePath)
{
    size_t size;
    uint8_t *file = readFile(exePath, &size);
    const IMAGE_DOS_HEADER *dos;
    const IMAGE_NT_HEADERS32 *nt;
    const IMAGE_SECTION_HEADER *sec;
    char hex[65];
    uint32_t iatLo, iatHi;
    DWORD oldProt;
    int thunks = 0;

    if (!file) {
        bmxLog("cannot read %s (run from the game folder)\n", exePath);
        return -1;
    }
    if (sha256Hex(file, size, hex) != 0 || strcmp(hex, kKnownSha256) != 0) {
        bmxLog("%s is not the supported NgBMX.exe (sha256 %s)\n", exePath, hex);
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
            for (size_t k = 0; k < sizeof kTextKeep / sizeof kTextKeep[0]; k++) {
                if (kTextKeep[k].start >= start && kTextKeep[k].end <= start + raw) {
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
    bmxCrtInstall();
    FlushInstructionCache(GetCurrentProcess(), NULL, 0);
    free(file);
    return 0;
}
