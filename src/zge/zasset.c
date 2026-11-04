/* Asset library files (C:\NgEngine\Zge\zasset.c). */
#include "zge/zasset.h"

#include "compat/crt.h"
#include "zge/zdebug.h"
#include "zge/zfile.h"
#include "zge/zmem.h"
#include "zge/zstring.h"

static const char kAssetFile[] = "C:\\NgEngine\\Zge\\zasset.c";

enum {
    kLibraryMagic = 0x2a81511c,
    kDirectoryOffsetNew = 0x14, /* header of 0x1c bytes (with the magic) */
    kDirectoryOffsetOld = 0xc,  /* header of 0x14 bytes without it */
};

/* Little endian fields of a directory entry. */
static uint32_t entryField(const uint8_t *entry, unsigned index)
{
    return ((const uint32_t *)entry)[index];
}

/* Fatal: the original passes an empty message for failed loose file opens. */
static void failEmpty(int line)
{
    zdebugFail("", kAssetFile, line, 0);
}

BMX_ADDR(0x004804d0)
ZassetLibrary *zassetOpenLibrary(const char *name)
{
    ZassetLibrary *lib;
    ZfileHandle *file;
    int32_t header[7];
    uint32_t headerOffset;
    uint16_t info[2];
    const char *failure;

    gZdebugPrint("OPEN LIBRARY: ");
    if (gZassetLibraries == NULL) {
        gZassetLibraries = zmemFreeListCreate(sizeof(ZassetLibrary), 0x20, NULL);
    }
    lib = zmemFreeListAlloc(gZassetLibraries);
    if (lib == NULL) {
        gZdebugPrint("FAILED - Could not alocate memory for ZfileLibraryHandle\n");
        return 0;
    }
    file = zfileOpen(name, 1, 0);
    if (file == NULL) {
        zmemFreeListFree(gZassetLibraries, lib);
        return 0;
    }
    zfileOp30(file, 0, 0, 0);
    if (zfileRead(file, header, 0x1c, 0) != 0x1c) {
        failure = "FAILED - Could not load header!\n";
    } else {
        /* the 4 byte info word sits at +0x10 behind the magic, else at +8 */
        const uint32_t infoOffset = header[0] != kLibraryMagic ? 8 : 0x10;

        headerOffset = infoOffset + 4;
        zmemCopy(info, (const uint8_t *)header + infoOffset, 4);
        lib->file = file;
        lib->stored = (uint16_t)(((((uint32_t)info[1] << 16 | info[0]) >> 14) & 2) + (info[1] >> 15));
        lib->method = (info[1] >> 14) & 1;
        lib->entryBytes = lib->stored == 0 ? 8 : 0xc;
        lib->groups = info[1] & 0x3fff;
        lib->perGroup = info[0] & 0x1fff;
        lib->directoryBytes = ((uint32_t)lib->perGroup * lib->entryBytes + 4) * lib->groups;
        lib->linked = (info[0] >> 13) & 1;
        if (lib->linked != 0 && lib->stored != 0) {
            zdebugFail("Compression in ref mode", kAssetFile, 0xfb, 0);
        }
        lib->directory = zmemAlloc(lib->directoryBytes);
        if (lib->directory == NULL) {
            return 0;
        }
        zfileOp30(file, headerOffset, 0, 0);
        if (zfileRead(file, lib->directory, (int)lib->directoryBytes, 0) == (int)lib->directoryBytes) {
            lib->cursor = lib->directory + 4;
            return lib;
        }
        failure = "FAILED to load library directory\n";
    }
    gZdebugPrint(failure);
    zfileClose(file, 0);
    return 0;
}

BMX_ADDR(0x004806c0)
void zassetCloseLibrary(ZassetLibrary *lib)
{

    gZdebugPrint("CLOSE LIBRARY: ");
    zfileClose(lib->file, 0);
    zmemFree(lib->directory);
    zmemFreeListFree(gZassetLibraries, lib);
    if (zmemFreeListUsedCount(gZassetLibraries) == 0) {
        zmemFreeListDestroy(gZassetLibraries);
        gZassetLibraries = NULL;
    }
}

BMX_ADDR(0x00480720)
int zassetCount(ZassetLibrary *lib)
{
    return lib->groups;
}

BMX_ADDR(0x00480730)
int zassetNext(ZassetLibrary *lib)
{
    const uint32_t stride = (uint32_t)lib->entryBytes * lib->perGroup + 4;
    const int32_t group = (int32_t)(((uint32_t)(lib->cursor - lib->directory) - 4u) / stride) + 1;

    if (group >= (int32_t)lib->groups) {
        return 0;
    }
    lib->cursor = lib->directory + (uint32_t)group * stride + 4;
    return 1;
}

BMX_ADDR(0x00480780)
int zassetFind(ZassetLibrary *lib, int group, int asset)
{
    uint32_t entryBytes, stride, left;
    const uint8_t *p;

    if (lib == NULL) {
        zdebugFail("NULL", kAssetFile, 0x18a, 0);
    }
    /* the assert returns when ignored and the original carries on into the NULL library */
    // NOLINTNEXTLINE(clang-analyzer-core.NullDereference)
    entryBytes = lib->entryBytes;
    stride = (uint32_t)lib->perGroup * entryBytes + 4;
    if ((uint32_t)group < lib->groups) {
        p = lib->directory + stride * (uint32_t)group;
        if (entryField(p, 0) == (uint32_t)group) {
            lib->cursor = (uint8_t *)(uintptr_t)(((uint32_t)asset & 0xffff) * entryBytes + (uintptr_t)p + 4);
            return 1;
        }
    }
    p = lib->directory;
    for (left = lib->groups; (int32_t)left > 0; left--) {
        if (entryField(p, 0) == (uint32_t)group) {
            lib->cursor = (uint8_t *)(uintptr_t)(((uint32_t)asset & 0xffff) * entryBytes + (uintptr_t)p + 4);
            return 1;
        }
        p += stride;
    }
    gZdebugPrint("Failed to find asset Group=%d, AssetNum=%d\n", group, asset & 0xffff);
    return 0;
}

BMX_ADDR(0x00480450)
void zassetDump(ZassetLibrary *lib)
{
    const uint8_t *p = lib->directory;
    const uint32_t stride = (uint32_t)lib->perGroup * (lib->stored != 0 ? 0xc : 8) + 4;

    gZdebugPrint("NumGroups=%d, AssetsPerGroup=%d\n", lib->groups, lib->perGroup);
    for (int i = 0; lib->groups != 0 && i < (int)lib->groups; i++) {
        if (lib->stored == 0) {
            gZdebugPrint("Asset=%d, Size=%d\n", entryField(p, 0), entryField(p, 2) & 0xffffff);
        }
        p += stride;
    }
}

BMX_ADDR(0x00480850)
uint32_t zassetSize(ZassetLibrary *lib)
{
    const uint8_t *entry = lib->cursor;
    int size = (int)(entryField(entry, 1) & 0xffffff);

    if (lib->linked != 0) {
        ZfileHandle *loose;

        zfileOp30(lib->file, entryField(entry, 0) + 0x10, 0, 0);
        if (zfileRead(lib->file, lib->linkName, size, 0) != size) {
            zdebugFail("", kAssetFile, 0x1cf, 0);
        }
        loose = zfileOpen(lib->linkName, 1, 0);
        if (loose == NULL) {
            gZdebugPrint("Not found: %s\n", lib->linkName);
            failEmpty(0x1d8);
        }
        zfileGetSize(loose, &size, 0);
        zfileClose(loose, 0);
        lib->cachedEntry = lib->cursor;
        lib->cachedSize = (uint32_t)size;
    }
    return (uint32_t)size;
}

BMX_ADDR(0x00480930)
int zassetRead(ZassetLibrary *lib, void *dst)
{
    const uint8_t *entry = lib->cursor;
    int ok = 1;
    int size;

    size = lib->stored == 0 ? (int)(entryField(entry, 1) & 0xffffff) : (int)entryField(entry, 2);
    zfileOp30(lib->file, entryField(entry, 0) + 0x10, 0, 0);
    if (lib->stored == 0) {
        if (lib->linked == 0) {
            if (zfileRead(lib->file, dst, size, 0) != size) {
                gZdebugPrint("FAILED - Could not load asset\n");
                ok = 0;
            }
        } else if (lib->cachedEntry == lib->cursor) {
            ZfileHandle *loose = zfileOpen(lib->linkName, 1, 0);

            if (loose == NULL) {
                gZdebugPrint("Not found: %s", lib->linkName);
                failEmpty(0x22d);
            }
            zfileRead(loose, dst, (int)lib->cachedSize, 0);
            zfileClose(loose, 0);
        } else {
            ZfileHandle *loose;

            if (zfileRead(lib->file, lib->linkName, size, 0) != size) {
                gZdebugPrint("FAILED - Could not load asset\n");
                ok = 0;
            }
            loose = zfileOpen(lib->linkName, 1, 0);
            if (loose == NULL) {
                gZdebugPrint("Not found: %s", lib->linkName);
                failEmpty(0x241);
            }
            zfileGetSize(loose, &size, 0);
            zfileRead(loose, dst, size, 0);
            zfileClose(loose, 0);
            lib->cachedEntry = lib->cursor;
            lib->cachedSize = (uint32_t)size;
        }
    } else {
        void *packed = zmemAlloc((uint32_t)size);
        uint32_t outSize = 0; /* the original leaves it unset for an unknown method */

        if (zfileRead(lib->file, packed, size, 0) != size) {
            gZdebugPrint("FAILED - Could not load asset\n");
            ok = 0;
        }
        if (lib->method == 0) {
            zassetDecompress(1, packed, (uint32_t)size, dst, &outSize);
        } else if (lib->method == 1) {
            zassetDecompress(2, packed, (uint32_t)size, dst, &outSize);
        } else {
            zdebugFail("Unknown decompression method", kAssetFile, 0x264, 0);
        }
        if (outSize != (entryField(lib->cursor, 1) & 0xffffff)) {
            gZdebugPrint("ERROR - Size mismatch in decompressed data\n");
            zdebugFail("ERROR - Size mismatch in decompressed data", kAssetFile, 0x269, 0);
        }
        zmemFree(packed);
    }
    lib->cursor += lib->entryBytes;
    return ok;
}
