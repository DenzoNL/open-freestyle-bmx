/*
 * Tests for the scene point chunk reader and point release (zscnpts.c). The IFF reader is replaced by a
 * script: which chunks exist, and the bytes the reads deliver.
 */
#include "dt.h"
#include "dt_ngif.h"
#include "zge/com/zglrw/zrwcam.h"
#include "zge/zgemods/zscnpts.h"
#include "zge/zscene.h"

typedef struct ZMemFreeList *(BMX_CDECL *ListCreateFn)(uint32_t, uint32_t, void *);
typedef void *(BMX_CDECL *ListAllocFn)(struct ZMemFreeList *);
typedef void *(BMX_CDECL *AllocFn)(uint32_t);

static struct {
    int begin, hasPoint, hasObject;
    uint32_t chunk[0x13];
    uint8_t object[0x40];
    uint32_t objectSize;
} gScript;

static int BMX_CDECL stubBegin(void *iff)
{
    ngLog(0x49b130);
    return gScript.begin;
}

static int BMX_CDECL stubEnd(void *iff)
{
    ngLog(0x49b250);
    return 1;
}

static int BMX_CDECL stubFind(void *iff, uint32_t id)
{
    ngLog(0x49ad90);
    ngLog(id);
    return id == 0x54504453 ? gScript.hasPoint : gScript.hasObject;
}

static int BMX_CDECL stubRead(void *iff, void *dst, uint32_t size)
{
    ngLog(0x49ac30);
    ngLog(size);
    if (size == 0x4c) {
        memcpy(dst, gScript.chunk, 0x4c);
    } else if (size == 8) {
        uint32_t header[2] = {0x1234, gScript.objectSize};
        memcpy(dst, header, 8);
    } else {
        memcpy(dst, gScript.object, size > 0x40 ? 0x40 : size);
    }
    return 1;
}

static void BMX_CDECL stubQuery(int what, float *out, int arg)
{
    ngLog(0x48ad20);
    ngLog((uint32_t)what);
    *out = dt_float_edgy(0.0f, 4.0f);
}

static void begin(DtCtx *c)
{
    ngBegin(c, 1);
    gScnpts.points = DT_ORIG(0x00485e80, ListCreateFn)(0x50, 2, NULL);
    dt_stub(0x0049b130, NULL, (void *)stubBegin);
    dt_stub(0x0049b250, NULL, (void *)stubEnd);
    dt_stub(0x0049ad90, NULL, (void *)stubFind);
    dt_stub(0x0049ac30, NULL, (void *)stubRead);
    dt_stub(0x0048ad20, NULL, (void *)stubQuery);
}

static void setup_read(DtCtx *c)
{
    begin(c);
    gScript.begin = (dt_u32() & 7) != 0;
    gScript.hasPoint = (dt_u32() & 7) != 0;
    gScript.hasObject = (dt_u32() & 1) != 0;
    dt_fill(gScript.chunk, sizeof gScript.chunk);
    gScript.chunk[0] = (dt_u32() & 7) ? 0x4c : dt_u32();
    for (int i = 7; i < 0x13; i++) {
        const float f = dt_float_edgy(-500.0f, 500.0f);
        memcpy(&gScript.chunk[i], &f, 4);
    }
    dt_fill(gScript.object, sizeof gScript.object);
    gScript.objectSize = (uint32_t)dt_range(1, 0x40);
    gZrwMarkers = (dt_u32() & 1) ? DT_ORIG(0x00486050, ListAllocFn)(gScnpts.points) : NULL;
    DT_ARG_PTR(c, 0, dt_alloc(16));
}

static void setup_free(DtCtx *c)
{
    ZrwMarker *marker;

    begin(c);
    marker = DT_ORIG(0x00486050, ListAllocFn)(gScnpts.points);
    dt_fill(marker, 0x50);
    marker->data = (dt_u32() & 1) ? DT_ORIG(0x00485aa0, AllocFn)(16) : NULL;
    DT_ARG_PTR(c, 0, marker);
    DT_ARG_INT(c, 1, dt_u32());
}

DT_CASES(zscnpts_cases) = {
    {"zscnptsFreePoint", 0x00491f10, zscnptsFreePoint, 2, DT_RET_INT, setup_free, 100, 0},
    {"zscnptsReadChunk", 0x00491f50, zscnptsReadChunk, 1, DT_RET_INT, setup_read, 600, 0},
};
