/* Profiler and the renderer's file callbacks (C:\NgEngine\Zge\zprofile.c). */
#include "zge/zprofile.h"

#include "compat/crt.h"
#include "zge/com/zglrw/zrwprim.h"
#include "zge/com/zglrw/zrwzgl.h"
#include "zge/pc/pcprofile.h"
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

/* The renderer's quad and triangle drawing hooks and the debug text hook (print at x, y). */
typedef void(BMX_CDECL *ZProfileDrawFn)(uint32_t flags, const void *prims, int count, const void *matrix,
                                        int zero);
typedef int(BMX_CDECL *ZProfilePrintAtFn)(int x, int y, const char *text);
#define gZProfileDrawTriangles ORIG_VAR(0x0056172c, ZProfileDrawFn, gZProfileDrawTriangles)
#define gZProfileDrawQuads     ORIG_VAR(0x00561738, ZProfileDrawFn, gZProfileDrawQuads)
#define gZProfilePrintAt       ORIG_VAR(0x00560038, ZProfilePrintAtFn, gZProfilePrintAt)
/* Colours of the peak bar and of the ticks at whole units, tenths, hundredths and the rest. */
typedef uint32_t ZProfileColors[5];
#define gZProfileColors ORIG_VAR(0x00563904, ZProfileColors, gZProfileColors)

enum { kProfileScreenSpace = 0x2000000 };

static void setColor(ZrwPrimVertex *v, uint32_t color)
{
    __builtin_memcpy(v->color, &color, 4);
}

/* Whether v is within 5e-5 of the integer _ftol gives for v + 5e-5. */
static int nearWhole(double v)
{
    return !(__builtin_fabs(v - (double)bmxFtol(v + (double)0x1.a36e2ep-15f)) >= 5e-05);
}

/* A bar from x to `right`, `height` high from `top`, as a screen space quad. */
static void drawBar(ZrwPrimVertex *v, const ZrwPrimQuad *quad, float x, float top, uint32_t height,
                    double right, uint32_t color)
{
    v[0].x = x;
    v[0].y = top;
    v[1].x = x;
    v[1].y = (float)((double)height + (double)top);
    v[2].y = v[1].y;
    v[2].x = (float)right;
    v[3].x = (float)right;
    v[3].y = top;
    for (int i = 0; i < 4; i++) {
        setColor(&v[i], color);
    }
    gZProfileDrawQuads(kProfileScreenSpace, quad, 1, NULL, 0);
}

/*
 * The vertices handed to the hooks only get x, y and colour (z, u and v are left as they are). The ticks
 * run from 0 to the budget in steps of 1, 0.1, 0.01 or 0.001 (depending on the budget); a budget below 0
 * draws none.
 */
BMX_ADDR(0x004a35f0)
int zprofileUpdate(void *arg)
{
    ZProfileState *s = &gZProfile;
    ZrwPrimVertex v[4];
    const ZrwPrimQuad quad = {0, {&v[0], &v[1], &v[2], &v[3]}};
    const ZrwPrimTriangle triangle = {0, {&v[0], &v[1], &v[2]}};
    float y = s->y;
    const float width = (float)((double)(uint32_t)(gZrwScreenW - s->margin * 2) - (double)s->x);

    (void)arg;
    for (ZMemFreeNode *n = zmemFreeListFirst(s->entries); n != NULL; n = zmemFreeNodeNext(n)) {
        ZProfileEntry *e = zmemFreeNodePayload(n);
        float step;

        if ((e->flags & 1) != 0) {
            const uint32_t ticks = e->ticks + (uint32_t)pcProfileElapsed(e->start);

            e->ticks = ticks;
            if (s->holdFrames == 0 && !(e->peak > ticks)) {
                e->peak = ticks;
            }
            e->flags &= ~1u;
        }
        if (s->holdFrames == 0) {
            const double right = pcProfileTicksToUnits(e->peak, (float)(double)s->unitsPerSecond) *
                                     (double)width / (double)e->budget +
                                 (double)s->x;

            drawBar(v, &quad, s->x, y, s->barHeight, right, gZProfileColors[0]);
        }
        {
            const double length = pcProfileTicksToUnits(e->ticks, (float)(double)s->unitsPerSecond) *
                                  (double)width / (double)e->budget;
            const float top =
                (float)(__builtin_fabs((double)(int32_t)(s->barHeight - s->timeBarHeight) * 0.5) + (double)y);

            drawBar(v, &quad, s->x, top, s->timeBarHeight, length + (double)s->x, e->color);
        }
        if (e->budget >= 1.0f) {
            step = 1.0f;
        } else if (e->budget > 0.1f) {
            step = 0.1f;
        } else if (e->budget > 0.01f) {
            step = 0.01f;
        } else {
            step = 0.001f;
        }
        if (!(0.0f > e->budget)) {
            float mark = 0.0f;
            double next;

            do {
                const double at = (double)mark * (double)width / (double)e->budget + (double)s->x;
                const double half = (double)(s->tickWidth >> 1);
                uint32_t color;

                if (nearWhole((double)mark)) {
                    color = gZProfileColors[1];
                } else if (nearWhole((double)mark * 10.0)) {
                    color = gZProfileColors[2];
                } else if (nearWhole((double)mark * 100.0)) {
                    color = gZProfileColors[3];
                } else {
                    color = gZProfileColors[4];
                }
                for (int i = 0; i < 3; i++) {
                    setColor(&v[i], color);
                }
                v[0].x = (float)(half + at);
                v[0].y = (float)((double)y - (double)s->tickAbove);
                v[1].y = v[0].y;
                v[1].x = (float)(at - half);
                v[2].x = (float)at;
                v[2].y = (float)((double)s->tickBelow + (double)v[0].y);
                gZProfileDrawTriangles(kProfileScreenSpace, &triangle, 1, NULL, 0);
                next = (double)mark + (double)step;
                mark = (float)next;
            } while (!(next > (double)e->budget));
        }
        if (e->name != NULL) {
            gZProfilePrintAt(0, bmxFtol((double)y) - 3, e->name);
        }
        e->ticks = 0;
        memset(e->start, 0, sizeof e->start);
        y = (float)((double)(uint32_t)(s->barHeight + s->barGap) + (double)y);
    }
    if (s->holdFrames != 0) {
        s->holdFrames--;
    }
    if (s->peakResetFrames != 0) {
        const int32_t countdown = s->peakResetCountdown;

        s->peakResetCountdown = countdown - 1;
        if (countdown == 0) {
            for (ZMemFreeNode *n = zmemFreeListFirst(s->entries); n != NULL; n = zmemFreeNodeNext(n)) {
                ((ZProfileEntry *)zmemFreeNodePayload(n))->peak = 0;
            }
            s->peakResetCountdown = s->peakResetFrames;
        }
    }
    return 1;
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
    s->margin = 0x20;
    s->zero3c = 0;
    s->zero40 = 0;
    s->barHeight = 8;
    s->timeBarHeight = 6;
    s->barGap = 2;
    s->tickAbove = 4;
    s->tickWidth = 8;
    s->tickBelow = 4;
    s->limit5c = 2;
    s->limit60 = 0x78;
    s->unitsPerSecond = 0x3c;
    s->holdFrames = 10;
    s->x = 32.0f;
    s->y = 32.0f;
    s->scale20 = 2.0f;
    s->initialized = 1;
    s->entries = zmemFreeListCreate(0x24, 0, BMX_FNPTR(ZMemDestroyFn, 0x004a4080, pcProfileDestroyEntry));
    if (s->updateNode == NULL) {
        s->updateNode =
            zrwRegisterUpdateFunction(4, BMX_FNPTR(ZrwUpdateFn, 0x004a35f0, zprofileUpdate), NULL, 0);
    }
    s->peakResetFrames = s->limit60;
    if (s->limit60 == 0) {
        for (ZMemFreeNode *n = zmemFreeListFirst(s->entries); n != NULL; n = zmemFreeNodeNext(n)) {
            *(uint32_t *)((uint8_t *)zmemFreeNodePayload(n) + 0x10) = 0;
        }
    }
    return 1;
}
