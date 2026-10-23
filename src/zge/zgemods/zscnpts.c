/* Scene points (C:\NgEngine\Zge\ZgeMods\zscnpts.c): the point list read from a level. */
#include "zge/zgemods/zscnpts.h"

#include "zge/com/zglrw/zrwcam.h"
#include "zge/zdebug.h"
#include "zge/ziff.h"
#include "zge/zmem.h"
#include "zge/zscene.h"

static const char kZscnptsFile[] = "C:\\NgEngine\\Zge\\ZgeMods\\zscnpts.c";

typedef void(BMX_CDECL *ZscnptsQueryFn)(int what, float *out, int arg);
#define gZscnptsQuery ORIG_VAR(0x00561488, ZscnptsQueryFn, gZscnptsQuery)

BMX_ADDR(0x00491f10)
int zscnptsFreePoint(void *point, void *arg)
{
    ZrwMarker *marker = point;
    int result = 0;

    if (marker != NULL) {
        if (marker->data != NULL) {
            zmemFree(marker->data);
            marker->data = NULL;
        }
        result = 1;
    }
    zmemFreeListFree(gScnpts.points, marker);
    return result;
}

/* The "SDPT" chunk: size (0x4c), name hash and index, then a camera-style basis and position. */
typedef struct ScnptsChunk {
    uint32_t size;
    int32_t hash;
    int32_t index;
    uint8_t pad0c[0x10];
    ZVec3 right; /* 0x1c */
    ZVec3 up;    /* 0x28, copied as is */
    ZVec3 at;    /* 0x34 */
    ZVec3 pos;   /* 0x40 */
} ScnptsChunk;

_Static_assert(sizeof(ScnptsChunk) == 0x4c, "SDPT chunk");

BMX_ADDR(0x00491f50)
int zscnptsReadChunk(void *iff)
{
    ScnptsChunk chunk;
    ZrwMarker *marker;
    float scale;

    if (ziffBeginModel(iff) != 0) {
        if (ziffFindChunk(iff, 0x54504453) == 0) {
            zdebugFail("zscnpts : Point has no data!!!", kZscnptsFile, 0xe3, 0);
            return 0;
        }
        ziffRead(iff, &chunk, 0x4c);
        if (chunk.size != 0x4c) {
            zdebugFail("zscnpts : Point data size mismatch", kZscnptsFile, 0xe7, 0);
            return 0;
        }
        marker = zmemFreeListAlloc(gScnpts.points);
        marker->hash = chunk.hash;
        marker->index = chunk.index;
        gZscnptsQuery(1, &scale, 0);
        marker->matrix.right.x = -chunk.right.x;
        marker->matrix.right.y = -chunk.right.y;
        marker->matrix.right.z = -chunk.right.z;
        marker->matrix.up = chunk.up;
        marker->matrix.at.x = -chunk.at.x;
        marker->matrix.at.y = -chunk.at.y;
        marker->matrix.at.z = -chunk.at.z;
        marker->matrix.pos.x = (float)((double)chunk.pos.x * scale);
        marker->matrix.pos.y = (float)((double)chunk.pos.y * scale);
        marker->data = NULL;
        marker->matrix.pos.z = (float)((double)chunk.pos.z * scale);
        if (ziffFindChunk(iff, 0x494a424f) != 0) {
            uint32_t header[2];

            ziffRead(iff, header, 8);
            marker->data = zmemAlloc(header[1]);
            ziffRead(iff, marker->data, header[1]);
        }
        marker->next = gZrwMarkers;
        gZrwMarkers = marker;
        ziffEndModel(iff);
    }
    return 1;
}
