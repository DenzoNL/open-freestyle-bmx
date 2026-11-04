/* Renderer side of the RAM disk (C:\NgEngine\Zge\Com\ZglRw\zrwRamDisk.c). */
#include "zge/com/zglrw/zrwramdisk.h"

#include "compat/crt.h"
#include "zge/zdebug.h"
#include "zge/zfile.h"
#include "zge/zgemods/zobjpemit.h"
#include "zge/zstring.h"

static const char kZrwRamFile[] = "C:\\NgEngine\\Zge\\Com\\ZglRw\\zrwRamDisk.c";

/* x87 compares of the original: a point passes when it is not below `lo` (NaN fails) and not above `hi`. */
static int inRange(float v, float lo, float hi)
{
    return (double)v >= (double)lo && !((double)v > (double)hi);
}

/* Both bounds tested together, for the box corners at box[0..2] and box[4..6]. */
static int inBox(const float *box, const float *p)
{
    return inRange(p[0], box[0], box[4]) && inRange(p[1], box[1], box[5]) && inRange(p[2], box[2], box[6]);
}

BMX_ADDR(0x004a2f50)
int zrwBBoxContainsPoint(const float *box, const float *point)
{
    return inBox(box, point);
}

/* The six faces of a box: directions for the clipping planes (the first three use the min corner). */
typedef float ZrwBBoxFaces[6][4];
#define kZrwBBoxFaces ORIG_VAR(0x00563770, ZrwBBoxFaces, kZrwBBoxFaces)

BMX_ADDR(0x004a3010)
int zrwBBoxClipSegment(float *a, float *b, const float *box)
{
    float p0[3], p1[3], plane[5];

    if (inBox(box, a) && inBox(box, b)) {
        return 1;
    }
    memcpy(p0, a, sizeof p0);
    memcpy(p1, b, sizeof p1);
    for (int face = 0; face < 6; face++) {
        zobjPlaneFromFace(plane, face < 3 ? box : box + 4, kZrwBBoxFaces[face]);
        if (zobjClipSegmentToPlane(p0, p1, plane) == 0) {
            return 0;
        }
    }
    memcpy(a, p0, sizeof p0);
    memcpy(b, p1, sizeof p1);
    return 1;
}

BMX_ADDR(0x004a3200)
int zrwRamDiskMatches(const char *path)
{
    char ext[64];

    if (gZrwRamDiskInfo->extension != NULL) {
        zfileGetExtension(path, ext);
        zstrupr(ext);
        if (zstrcmp(ext, gZrwRamDiskInfo->extension) != 0) {
            return 0;
        }
    }
    return 1;
}

BMX_ADDR(0x004a3260)
void *zrwfopen(const char *path, const char *mode)
{
    (void)path; /* the RAM disk serves the file set with ImageBufferSizeType */
    (void)mode;
    const char *keys[2];
    uintptr_t values[2];
    const char *name;

    if (gZrwRamDiskInfo == gZrwRamDiskNoInfo) {
        gZdebugPrint("zrwFileRamDiskSetInf() not called\n");
        return NULL;
    }
    if (gZrwRamDiskFile != NULL) {
        zdebugFail("Too many RAM disk files in use", kZrwRamFile, 0x7b, 0);
    }
    if (gZrwRamDiskDevice == -1) {
        ZfileDeviceTable *table = zfileGetDevices();

        if (table == NULL) {
            zdebugFail("No devices!", kZrwRamFile, 0x83, 0);
        }
        for (uint32_t i = 0; table->count != 0 && i < table->count; i++) {
            if (zstrcmp(zfileDeviceName(table->device[i]), "rd00") == 0) {
                gZrwRamDiskDevice = (intptr_t)table->device[i];
                break;
            }
        }
        if (gZrwRamDiskDevice == -1) {
            zdebugFail("Could not locate a RAM disk device on this system!", kZrwRamFile, 0x91, 0);
            return NULL;
        }
    }
    keys[0] = "ADDRESS";
    keys[1] = "SIZE";
    values[0] = (uintptr_t)gZrwRamDiskInfo->address;
    values[1] = gZrwRamDiskInfo->size;
    name = zfileDeviceControl((ZfileDevice *)gZrwRamDiskDevice, 2, keys, values);
    if (name == NULL) {
        return NULL;
    }
    gZrwRamDiskFile = zfileOpen(name, 1, 0);
    if (gZrwRamDiskFile == NULL) {
        zdebugFail("Error opening RAM disk file.", kZrwRamFile, 0xa7, 0);
    }
    return (void *)&gZrwRamDiskFile;
}
