/* Profiler and the renderer's file callbacks (C:\NgEngine\Zge\zprofile.c). */
#include "zge/zprofile.h"

#include "zge/com/zglrw/zrwzgl.h"
#include "zge/zdebug.h"
#include "zge/zmem.h"
#include "zge/zstring.h"

static const char kProfileFile[] = "C:\\NgEngine\\Zge\\zprofile.c";

BMX_ADDR(0x004a33c0)
int zrwfclose(ZfileHandle **file)
{
    zfileClose(*file, 0);
    *file = NULL;
    gZrwRamDiskInfo++;
    return 0;
}

BMX_ADDR(0x004a33f0)
int zrwfread(void *buffer, int size, int count, ZfileHandle **file)
{
    return zfileRead(*file, buffer, (int)((uint32_t)size * (uint32_t)count), 0);
}

BMX_ADDR(0x004a3420)
int zrwfwrite(const void *buffer, int size, int count, ZfileHandle **file)
{
    return zfileWrite(*file, buffer, (int)((uint32_t)size * (uint32_t)count), 0);
}

BMX_ADDR(0x004a3450)
char *zrwfgets(char *buffer, int size, ZfileHandle **file)
{
    return zfileGets(*file, buffer, size);
}

BMX_ADDR(0x004a3470)
int zrwfflush(void)
{
    return 0;
}

BMX_ADDR(0x004a3480)
int zrwfputs(void)
{
    return 0;
}

/* True when the position (op 0x34) has reached the size. */
BMX_ADDR(0x004a3490)
int zrwfeof(ZfileHandle **file)
{
    uint32_t position = 0;
    uint32_t size = 0;

    zfileOp34(*file, (uint32_t)(uintptr_t)&position, 0);
    zfileGetSize(*file, (int *)&size, 0);
    return size <= position;
}

BMX_ADDR(0x004a34e0)
uint32_t zrwfseek(ZfileHandle **file, uint32_t a, uint32_t b)
{
    return zfileOp30(*file, a, b, 0);
}

BMX_ADDR(0x004a3500)
void RegisterCallBacksForFileSystem(void)
{
    void **rw = RwOSGetFileInterface();

    zmemCopy(&gZrwSavedFileInterface, (const void *)rw, sizeof(ZrwFileInterface));
    rw[0] = BMX_FNPTR(void *, 0x004a3200, zrwRamDiskMatches);
    rw[1] = BMX_FNPTR(void *, 0x004a3260, zrwfopen);
    rw[2] = BMX_FNPTR(void *, 0x004a33c0, zrwfclose);
    rw[3] = BMX_FNPTR(void *, 0x004a33f0, zrwfread);
    rw[4] = BMX_FNPTR(void *, 0x004a3420, zrwfwrite);
    rw[5] = BMX_FNPTR(void *, 0x004a3450, zrwfgets);
    rw[6] = BMX_FNPTR(void *, 0x004a3480, zrwfputs);
    rw[7] = BMX_FNPTR(void *, 0x004a3490, zrwfeof);
    rw[8] = BMX_FNPTR(void *, 0x004a34e0, zrwfseek);
    rw[9] = BMX_FNPTR(void *, 0x004a3470, zrwfflush);
    gZrwRamDiskNoInfo = gZrwRamDiskInfoSlots;
    gZrwRamDiskInfo = gZrwRamDiskInfoSlots;
    gZrwRamDiskFile = NULL;
}

BMX_ADDR(0x004a3580)
void ImageBufferSizeType(const void *address, uint32_t size, const char *extension)
{
    gZrwRamDiskNoInfo->address = (uint32_t)(uintptr_t)address;
    gZrwRamDiskNoInfo->size = size;
    gZrwRamDiskNoInfo->extension = extension;
    gZrwRamDiskNoInfo++;
}

BMX_ADDR(0x004a35c0)
void zrwFileRamDiskClear(void)
{
    zmemCopy((void *)RwOSGetFileInterface(), &gZrwSavedFileInterface, sizeof(ZrwFileInterface));
    gZrwRamDiskNoInfo = NULL;
    gZrwRamDiskInfo = NULL;
    gZrwRamDiskFile = NULL;
}

/* Sets up the profiler state and registers its per-frame update; 0 when the platform side fails. */
BMX_ADDR(0x004a3b60)
int zprofileInitialize(void)
{
    ZProfileState *s = &gZProfile;

    if (s->initialized != 0) {
        zdebugFail("Already initialized", kProfileFile, 0x141, 0);
    }
    zmemFill(s, 0, sizeof *s);
    if (pcProfileInitialize() == 0) {
        return 0;
    }
    s->limit38 = 0x20;
    s->zero3c = 0;
    s->zero40 = 0;
    s->limit44 = 8;
    s->limit48 = 6;
    s->limit4c = 2;
    s->limit50 = 4;
    s->limit54 = 8;
    s->limit58 = 4;
    s->limit5c = 2;
    s->limit60 = 0x78;
    s->limit64 = 0x3c;
    s->limit0c = 10;
    s->scale18 = 32.0f;
    s->scale1c = 32.0f;
    s->scale20 = 2.0f;
    s->initialized = 1;
    s->entries = zmemFreeListCreate(0x24, 0, pcProfileDestroyEntry);
    if (s->updateNode == NULL) {
        s->updateNode = zrwRegisterUpdateFunction(4, zprofileUpdate, NULL, 0);
    }
    s->limit14 = s->limit60;
    if (s->limit60 == 0) {
        for (ZMemFreeNode *n = zmemFreeListFirst(s->entries); n != NULL; n = zmemFreeNodeNext(n)) {
            *(uint32_t *)((uint8_t *)zmemFreeNodePayload(n) + 0x10) = 0;
        }
    }
    return 1;
}
