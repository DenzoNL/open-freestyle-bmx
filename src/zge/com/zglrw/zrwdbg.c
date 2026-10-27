/* Renderer debug output (C:\NgEngine\Zge\Com\ZglRw\zrwDbg.c). */
#include "zge/com/zglrw/zrwdbg.h"

#include <stdarg.h>
#include <stddef.h>

#include "compat/crt.h"
#include "rw/rwcamera.h"
#include "rw/rwcore.h"
#include "rw/rwmath.h"
#include "zge/com/zglrw/zrwzgl.h"
#include "zge/zdebug.h"
#include "zge/zgemods/zobjcamera.h"
#include "zge/zmem.h"
#include "zge/zstring.h"

static const char kZrwDbgFile[] = "C:\\NgEngine\\Zge\\Com\\ZglRw\\zrwDbg.c";

/* RenderWare library calls (statically linked in the exe, not ported). */

/* Hooks into the engine (function pointers in the exe's data). */
typedef void(BMX_CDECL *ZrwDbgFrameStartFn)(void);
typedef void *(BMX_CDECL *ZrwDbgCameraFn)(void);
typedef ZObjCamera *(BMX_CDECL *ZrwDbgCameraObjectFn)(void *camera);
typedef ZrwUpdateNode *(BMX_CDECL *ZrwDbgRegisterFn)(int type, ZrwUpdateFn fn, void *arg, int append);
typedef int(BMX_CDECL *ZrwDbgUnregisterFn)(ZrwUpdateNode *node);
#define gZrwDbgRegister     ORIG_VAR(0x00561470, ZrwDbgRegisterFn, gZrwDbgRegister)
#define gZrwDbgUnregister   ORIG_VAR(0x00561474, ZrwDbgUnregisterFn, gZrwDbgUnregister)
#define gZrwDbgFrameStart   ORIG_VAR(0x0056148c, ZrwDbgFrameStartFn, gZrwDbgFrameStart)
#define gZrwDbgGetCamera    ORIG_VAR(0x00561db0, ZrwDbgCameraFn, gZrwDbgGetCamera)
#define gZrwDbgCameraObject ORIG_VAR(0x00561db4, ZrwDbgCameraObjectFn, gZrwDbgCameraObject)

/* Flags: a shape is drawn the first time with its static vertex data set up (these start as 1). */
#define gLineFresh     ORIG_VAR(0x00560080, int, gLineFresh)
#define gAxesFresh     ORIG_VAR(0x00560084, int, gAxesFresh)
#define gTriangleFresh ORIG_VAR(0x00560088, int, gTriangleFresh)
#define gBoxFresh      ORIG_VAR(0x0056008c, int, gBoxFresh)

/* The 12 edges of the box: pairs of corner indices (0x560090). */
typedef uint8_t ZrwDbgBoxEdges[24];
#define gBoxEdges ORIG_VAR(0x00560090, ZrwDbgBoxEdges, gBoxEdges)

/* The static vertex and primitive data of the shapes, in the exe's .bss from 0x650180. */
typedef struct ZrwDbgScratch {
    ZrwDbgPoly2 boxPolys[12];  /* 0x650180 */
    ZrwDbgVertex lineVerts[2]; /* 0x650210 */
    ZrwDbgVertex triVerts[3];  /* 0x650248 */
    uint32_t pad65029c;
    ZrwDbgVertex boxVerts[8];  /* 0x6502a0 */
    ZrwDbgPoly3 triPoly;       /* 0x650380 */
    ZrwDbgVertex axesVerts[6]; /* 0x650390 */
    ZrwDbgPoly2 linePoly;      /* 0x650438 */
    uint32_t pad650444;
    ZrwDbgPoly2 axesPolys[3]; /* 0x650448 */
} ZrwDbgScratch;

_Static_assert(offsetof(ZrwDbgScratch, linePoly) == 0x2b8, "scratch layout");
_Static_assert(sizeof(ZrwDbgScratch) == 0x2ec, "scratch layout");

#define gScratch ORIG_VAR(0x00650180, ZrwDbgScratch, gScratch)

/* The sphere mesh (allocated on first use): 8 x 9 pairs of vertices and the pointers to them. */
#define gSphereVerts    ORIG_VAR(0x0065e2a0, ZrwDbgVertex *, gSphereVerts)
#define gSpherePointers ORIG_VAR(0x0065e2a4, ZrwDbgVertex **, gSpherePointers)

enum { kSphereBands = 8, kSphereSlices = 9, kSphereVerts = kSphereBands * kSphereSlices * 2 };

/* Text queue: x, y (16 bit) and the text, each entry padded to 2 bytes; 0x650694 .. 0x650a94. */
typedef char ZrwDbgFormatBuffer[0x200];
typedef uint8_t ZrwDbgTextQueue[0x400];
#define gFormatBuffer ORIG_VAR(0x00650494, ZrwDbgFormatBuffer, gFormatBuffer)
#define gTextQueue    ORIG_VAR(0x00650694, ZrwDbgTextQueue, gTextQueue)
#define gTextEnd      ORIG_VAR(0x00650a94, uint8_t *, gTextEnd)
typedef ZrwDbgItem ZrwDbgItems[kZrwDbgMaxItems];
#define gItems       ORIG_VAR(0x00650a9c, ZrwDbgItems, gItems)
#define gDebugIndent ORIG_VAR(0x0065e29c, int, gDebugIndent)

#define gZrwPvsSavedA gZrwLightParamA
#define gZrwPvsSavedB gZrwLightParamB

static uint32_t drawFlags(uint16_t flags)
{
    uint32_t result = (flags & 1) != 0 ? 1 : 0;

    if ((flags & 2) != 0) {
        result |= 0x6502;
    }
    return result;
}

static uint32_t read32(const uint8_t *data, uint32_t offset)
{
    return *(const uint32_t *)(data + offset);
}

static uint16_t read16(const uint8_t *data, uint32_t offset)
{
    return *(const uint16_t *)(data + offset);
}

/* ---- text ----------------------------------------------------------------------------------------- */

BMX_ADDR(0x00480240)
void BMX_CDECL DebugString(const char *format, ...)
{
    va_list args;

    if (gDebugIndent != 0) {
        for (int i = 0; i < gDebugIndent; i++) {
            gFormatBuffer[i] = ' ';
        }
    }
    va_start(args, format);
    vsprintf(gFormatBuffer + gDebugIndent, format, args);
    va_end(args);
    zrwDebugOutput(gFormatBuffer);
}

BMX_ADDR(0x0047f940)
int BMX_CDECL ZDebugPrintBlya(int16_t x, int16_t y, const char *format, ...)
{
    va_list args;
    int length;

    va_start(args, format);
    vsprintf(gFormatBuffer, format, args);
    va_end(args);
    length = zstrlen(gFormatBuffer);
    if ((uintptr_t)gTextEnd + (uintptr_t)length + 6 >= (uintptr_t)gTextQueue + sizeof(ZrwDbgTextQueue)) {
        gZdebugPrint("zdebugPrint: Out of buffer space printing:'%s'\n", format);
        return 0;
    }
    *(int16_t *)gTextEnd = x;
    gTextEnd += 2;
    *(int16_t *)gTextEnd = y;
    gTextEnd += 2;
    zstrcpy((char *)gTextEnd, gFormatBuffer);
    gTextEnd = (uint8_t *)(((uintptr_t)gTextEnd + (uintptr_t)length + 2) & ~(uintptr_t)1);
    return length;
}

BMX_ADDR(0x0047f640)
void zrwDbgPrintDirect(const char *text, int x, int y)
{
    RtCharsetPrint(gZrwDbgCharset, text, x, y);
}

BMX_ADDR(0x0047f9e0)
void zrwDbgPrintFloat(int x, int y, const char *label, float value)
{
    if (label == NULL) {
        ZDebugPrintBlya((int16_t)x, (int16_t)y, "a:% 10.4f", (double)value);
        return;
    }
    ZDebugPrintBlya((int16_t)x, (int16_t)y, "%s% 10.4f", label, (double)value);
}

BMX_ADDR(0x0047fa30)
void zrwDbgPrintMatrix(int x, int y, const char *label, const float *m)
{
    int nextX = x;

    if (label == NULL) {
        ZDebugPrintBlya((int16_t)x, (int16_t)y, "% 10.4f,% 10.4f,% 10.4f", (double)m[0], (double)m[4],
                        (double)m[8]);
    } else {
        nextX = zstrlen(label) * gZrwDbgCharWidth + x;
        ZDebugPrintBlya((int16_t)x, (int16_t)y, "%s% 10.4f,% 10.4f,% 10.4f", label, (double)m[0],
                        (double)m[4], (double)m[8]);
    }
    ZDebugPrintBlya((int16_t)nextX, -1, "% 10.4f,% 10.4f,% 10.4f", (double)m[1], (double)m[5], (double)m[9]);
    ZDebugPrintBlya((int16_t)nextX, -1, "% 10.4f,% 10.4f,% 10.4f", (double)m[2], (double)m[6], (double)m[10]);
    ZDebugPrintBlya((int16_t)nextX, -1, "% 10.4f,% 10.4f,% 10.4f", (double)m[12], (double)m[13],
                    (double)m[14]);
}

BMX_ADDR(0x0047fb40)
void zrwDbgPrintVec4(int x, int y, const char *label, const float *v)
{
    if (label == NULL) {
        /* The original pushes a stray 0 in front of the values, which shifts every printed number. */
        ZDebugPrintBlya((int16_t)x, (int16_t)y, "% 10.4f,% 10.4f,% 10.4f,% 10.4f", 0, (double)v[0],
                        (double)v[1], (double)v[2], (double)v[3]);
        return;
    }
    ZDebugPrintBlya((int16_t)x, (int16_t)y, "%s% 10.4f,% 10.4f,% 10.4f,% 10.4f", label, (double)v[0],
                    (double)v[1], (double)v[2], (double)v[3]);
}

BMX_ADDR(0x0047fbd0)
void zrwDbgPrintVec3(int x, int y, const char *label, const float *v)
{
    if (label == NULL) {
        ZDebugPrintBlya((int16_t)x, (int16_t)y, "% 10.4f,% 10.4f,% 10.4f", (double)v[0], (double)v[1],
                        (double)v[2]);
        return;
    }
    ZDebugPrintBlya((int16_t)x, (int16_t)y, "%s% 10.4f,% 10.4f,% 10.4f", label, (double)v[0], (double)v[1],
                    (double)v[2]);
}

/* Angles print in degrees (65536 units are a full turn). */
#define kDegreesPerUnit 0x1.68p-8f

BMX_ADDR(0x0047fc40)
void zrwDbgPrintAngle(int x, int y, const char *label, int16_t angle)
{
    const double degrees = (double)(int)angle * (double)kDegreesPerUnit;

    if (label == NULL) {
        ZDebugPrintBlya((int16_t)x, (int16_t)y, "a:% 10.4f", degrees);
        return;
    }
    ZDebugPrintBlya((int16_t)x, (int16_t)y, "%s% 10.4f", label, degrees);
}

BMX_ADDR(0x0047fcb0)
void zrwDbgPrintAngles(int x, int y, const char *label, const int16_t *angles)
{
    if (label == NULL) {
        ZDebugPrintBlya((int16_t)x, (int16_t)y, "a:% 10.4f,% 10.4f,% 10.4f",
                        (double)(int)angles[0] * (double)kDegreesPerUnit,
                        (double)(int)angles[1] * (double)kDegreesPerUnit,
                        (double)(int)angles[2] * (double)kDegreesPerUnit);
        return;
    }
    ZDebugPrintBlya((int16_t)x, (int16_t)y, "%s% 10.4f,% 10.4f,% 10.4f", label,
                    (double)(int)angles[0] * (double)kDegreesPerUnit,
                    (double)(int)angles[1] * (double)kDegreesPerUnit,
                    (double)(int)angles[2] * (double)kDegreesPerUnit);
}

BMX_ADDR(0x0047ff80)
void zrwDbgSetFpsMode(int8_t mode)
{
    gZrwDbgFpsMode = (uint8_t)(mode + 1);
}

/* ---- queue ---------------------------------------------------------------------------------------- */

/* The next free queue entry with its type set, or NULL (with a debug message) when the queue is full. */
static ZrwDbgItem *queueItem(uint32_t type)
{
    ZrwDbgItem *item;

    if (gZrwDbgItemCount == kZrwDbgMaxItems) {
        gZdebugPrint("ZrwDbg: Out of buffer space\n");
        return NULL;
    }
    item = &gItems[gZrwDbgItemCount++];
    item->type = type;
    return item;
}

static void putWord(ZrwDbgItem *item, uint32_t offset, uint32_t value)
{
    *(uint32_t *)((uint8_t *)item->data + offset) = value;
}

static void putVec(ZrwDbgItem *item, uint32_t offset, const ZVec3 *v)
{
    putWord(item, offset, ((const uint32_t *)v)[0]);
    putWord(item, offset + 4, ((const uint32_t *)v)[1]);
    putWord(item, offset + 8, ((const uint32_t *)v)[2]);
}

static void putHalf(ZrwDbgItem *item, uint32_t offset, uint16_t value)
{
    *(uint16_t *)((uint8_t *)item->data + offset) = value;
}

BMX_ADDR(0x0047ff90)
void zrwDbgAddLine(const ZVec3 *a, const ZVec3 *b, const uint32_t *color, uint16_t flags)
{
    ZrwDbgItem *const item = queueItem(kZrwDbgLine);

    if (item == NULL) {
        return;
    }
    putVec(item, 0, a);
    putVec(item, 0x10, b);
    putWord(item, 0x20, *color);
    putHalf(item, 0x24, flags);
}

BMX_ADDR(0x00480010)
void zrwDbgAddAxes(const ZMatrix *matrix, uint16_t mode, uint16_t flags)
{
    ZrwDbgItem *const item = queueItem(kZrwDbgAxes);

    if (item == NULL) {
        return;
    }
    putHalf(item, 0x40, mode);
    putHalf(item, 0x42, flags);
    if (matrix != NULL) {
        zmatrixCopyRotation((ZMatrix *)item->data, matrix);
    }
}

BMX_ADDR(0x00480070)
void zrwDbgAddTriangle(const ZVec3 *a, const ZVec3 *b, const ZVec3 *c, const uint32_t *color, uint16_t flags)
{
    ZrwDbgItem *const item = queueItem(kZrwDbgTriangle);

    if (item == NULL) {
        return;
    }
    putVec(item, 0, a);
    putVec(item, 0x10, b);
    putVec(item, 0x20, c);
    putWord(item, 0x30, *color);
    putHalf(item, 0x34, flags);
}

BMX_ADDR(0x00480100)
void zrwDbgAddSphere(const ZVec3 *center, float radius, const uint32_t *color, uint16_t flags)
{
    ZrwDbgItem *const item = queueItem(kZrwDbgSphere);
    uint32_t radiusBits;

    if (item == NULL) {
        return;
    }
    memcpy(&radiusBits, &radius, 4);
    putVec(item, 0, center);
    putWord(item, 0x10, radiusBits);
    putWord(item, 0x14, *color);
    putHalf(item, 0x18, flags);
}

BMX_ADDR(0x00480170)
void zrwDbgAddBox(const uint32_t *corners, const ZMatrix *matrix, const uint32_t *color, uint16_t flags)
{
    ZrwDbgItem *const item = queueItem(kZrwDbgBox);

    if (item == NULL) {
        return;
    }
    for (uint32_t i = 0; i < 8; i++) {
        putWord(item, i * 4, corners[i]);
    }
    if (matrix == NULL) {
        zmatrixIdentity((ZMatrix *)((uint8_t *)item->data + 0x20));
    } else {
        for (uint32_t i = 0; i < 16; i++) {
            putWord(item, 0x20 + i * 4, ((const uint32_t *)matrix)[i]);
        }
    }
    putWord(item, 0x60, *color);
    putHalf(item, 0x64, flags);
}

/* ---- shapes --------------------------------------------------------------------------------------- */

/* A line from data[0] to data[0x10] in the colour at +0x20. */
BMX_ADDR(0x0047ee80)
void zrwDbgDrawLine(const uint8_t *data)
{
    if (gLineFresh != 0) {
        gLineFresh = 0;
        for (int i = 0; i < 2; i++) {
            gScratch.lineVerts[i].u = 0;
            gScratch.lineVerts[i].v = 0;
        }
        gScratch.linePoly.head = 0;
        for (int i = 0; i < 2; i++) {
            gScratch.linePoly.verts[i] = &gScratch.lineVerts[i];
        }
    }
    for (int i = 0; i < 2; i++) {
        ZrwDbgVertex *const vertex = &gScratch.lineVerts[i];

        for (int c = 0; c < 4; c++) {
            vertex->color[c] = data[0x20 + c];
        }
        *(uint32_t *)&vertex->x = read32(data, (uint32_t)i * 0x10);
        *(uint32_t *)&vertex->y = read32(data, (uint32_t)i * 0x10 + 4);
        *(uint32_t *)&vertex->z = read32(data, (uint32_t)i * 0x10 + 8);
    }
    gZrwDrawPolys(drawFlags(read16(data, 0x24)), &gScratch.linePoly, 1, NULL, 0);
}

/* Three coloured lines: the matrix axes (or the camera's) drawn from a point. */
BMX_ADDR(0x0047ef40)
void zrwDbgDrawAxes(const uint8_t *data)
{
    static const uint8_t axisColors[3][4] = {{0xff, 0, 0, 0xff}, {0, 0xff, 0, 0xff}, {0, 0, 0xff, 0xff}};
    const uint32_t *rows;
    ZVec3 position;
    uint32_t flags = 0;

    if (gAxesFresh != 0) {
        gAxesFresh = 0;
        for (int i = 0; i < 6; i++) {
            for (int c = 0; c < 4; c++) {
                gScratch.axesVerts[i].color[c] = axisColors[i / 2][c];
            }
            gScratch.axesVerts[i].u = 0;
            gScratch.axesVerts[i].v = 0;
        }
        for (int i = 0; i < 3; i++) {
            gScratch.axesPolys[i].head = 0;
            gScratch.axesPolys[i].verts[0] = &gScratch.axesVerts[i * 2];
            gScratch.axesPolys[i].verts[1] = &gScratch.axesVerts[i * 2 + 1];
        }
    }
    for (int i = 0; i < 6; i += 2) {
        gScratch.axesVerts[i].x = 0;
        gScratch.axesVerts[i].y = 0;
        gScratch.axesVerts[i].z = 0;
    }
    if ((data[0x40] & 1) == 0) {
        rows = (const uint32_t *)data;
        memcpy(&position, data + 0x30, sizeof position);
    } else {
        rows = (const uint32_t *)zobjCameraGetMatrix(gZrwDbgCameraObject(gZrwDbgGetCamera()), 0);
        position.x = 0;
        position.y = 0;
        position.z = 5.0f;
        flags = 0x1000000;
    }
    for (int axis = 0; axis < 3; axis++) {
        ZrwDbgVertex *const tip = &gScratch.axesVerts[axis * 2 + 1];

        *(uint32_t *)&tip->x = rows[axis * 4];
        *(uint32_t *)&tip->y = rows[axis * 4 + 1];
        *(uint32_t *)&tip->z = rows[axis * 4 + 2];
    }
    for (int i = 0; i < 6; i++) {
        ZrwDbgVertex *const vertex = &gScratch.axesVerts[i];

        zvec3Scale((ZVec3 *)vertex, (const ZVec3 *)vertex, 0.5f);
        zvec3Add((ZVec3 *)vertex, (const ZVec3 *)vertex, &position);
    }
    flags |= drawFlags(read16(data, 0x42));
    gZrwDrawPolys(flags, &gScratch.axesPolys, 3, NULL, 0);
}

/* A triangle: corners at data[0], [0x10], [0x20], colour at +0x30. */
BMX_ADDR(0x0047f230)
void zrwDbgDrawTriangle(const uint8_t *data)
{
    if (gTriangleFresh != 0) {
        gTriangleFresh = 0;
        for (int i = 0; i < 3; i++) {
            gScratch.triVerts[i].u = 0;
            gScratch.triVerts[i].v = 0;
        }
        gScratch.triPoly.head = 0;
        for (int i = 0; i < 3; i++) {
            gScratch.triPoly.verts[i] = &gScratch.triVerts[i];
        }
    }
    for (int i = 0; i < 3; i++) {
        ZrwDbgVertex *const vertex = &gScratch.triVerts[i];

        for (int c = 0; c < 4; c++) {
            vertex->color[c] = data[0x30 + c];
        }
        *(uint32_t *)&vertex->x = read32(data, (uint32_t)i * 0x10);
        *(uint32_t *)&vertex->y = read32(data, (uint32_t)i * 0x10 + 4);
        *(uint32_t *)&vertex->z = read32(data, (uint32_t)i * 0x10 + 8);
    }
    gZrwDrawTriangle(drawFlags(read16(data, 0x34)), &gScratch.triPoly, 1, NULL, 0);
}

/*
 * A wire sphere of radius data[0x10] around data[0] (colour at +0x14): the unit mesh is built once and
 * scaled by the matrix handed to the renderer.
 */
BMX_ADDR(0x0047f2f0)
void zrwDbgDrawSphere(const uint8_t *data)
{
    ZMatrix matrix;
    ZVec3 scale;
    struct {
        uint32_t head;
        ZrwDbgVertex **verts;
        uint32_t count;
    } poly;
    ZrwDbgVertex *vertex;
    float radius;

    if (gSphereVerts == NULL) {
        gSphereVerts =
            zmemFill(zmemAlloc(kSphereVerts * sizeof(ZrwDbgVertex)), 0, kSphereVerts * sizeof(ZrwDbgVertex));
        gSpherePointers = (ZrwDbgVertex **)zmemAlloc(kSphereVerts * sizeof(ZrwDbgVertex *));
        for (int i = kSphereVerts - 1; i >= 0; i--) {
            gSpherePointers[i] = &gSphereVerts[i];
        }
    }
    vertex = gSphereVerts;
    for (int band = 0; band < kSphereBands; band++) {
        const ZAngle lat1 = (ZAngle)(int)((double)band * 4096.0f);
        const float sin1 = (float)zmathSin(lat1);
        const ZAngle lat2 = (ZAngle)(int)((double)(band + 1) * 4096.0f);
        const float sin2 = (float)zmathSin(lat2);
        const float cos1 = (float)zmathCos(lat1);
        const float cos2 = (float)zmathCos(lat2);

        for (int slice = 0; slice < kSphereSlices; slice++) {
            const ZAngle lon = (ZAngle)(int)((double)slice * 8192.0f);
            const float x1 = (float)(zmathSin(lon) * (double)sin1);
            const float z1 = (float)(zmathCos(lon) * (double)sin1);
            const float x2 = (float)(zmathSin(lon) * (double)sin2);
            const float z2 = (float)(zmathCos(lon) * (double)sin2);

            for (int c = 0; c < 4; c++) {
                vertex[0].color[c] = data[0x14 + c];
                vertex[1].color[c] = data[0x14 + c];
            }
            vertex[0].x = x1;
            vertex[0].y = cos1;
            vertex[0].z = z1;
            vertex[1].x = x2;
            vertex[1].y = cos2;
            vertex[1].z = z2;
            vertex += 2;
        }
    }
    zmatrixIdentity(&matrix);
    memcpy(&radius, data + 0x10, 4);
    scale.x = radius;
    scale.y = radius;
    scale.z = radius;
    zmatrixScaleDiagonal(&matrix, &matrix, &scale);
    matrix.pos = *(const ZVec3 *)data;
    poly.head = 0;
    poly.verts = gSpherePointers;
    poly.count = kSphereVerts;
    gZrwDrawSphere(drawFlags(read16(data, 0x18)), &poly, 1, &matrix, 0);
}

/* A wire box between the corners data[0] and data[0x10], transformed by the matrix at +0x20 (colour +0x60).
 */
BMX_ADDR(0x0047f530)
void zrwDbgDrawBox(const uint8_t *data)
{
    if (gBoxFresh != 0) {
        gBoxFresh = 0;
        for (int i = 0; i < 8; i++) {
            gScratch.boxVerts[i].u = 0;
            gScratch.boxVerts[i].v = 0;
        }
        gScratch.boxPolys[8].head = 0;
        for (int i = 0; i < 12; i++) {
            gScratch.boxPolys[i].verts[0] = &gScratch.boxVerts[gBoxEdges[i * 2]];
            gScratch.boxPolys[i].verts[1] = &gScratch.boxVerts[gBoxEdges[i * 2 + 1]];
        }
    }
    for (uint32_t i = 0; i < 8; i++) {
        ZrwDbgVertex *const vertex = &gScratch.boxVerts[i];

        *(uint32_t *)&vertex->x = read32(data, (i & 1) != 0 ? 0 : 0x10);
        *(uint32_t *)&vertex->y = read32(data, (i & 2) != 0 ? 4 : 0x14);
        *(uint32_t *)&vertex->z = read32(data, (i & 4) != 0 ? 8 : 0x18);
        for (int c = 0; c < 4; c++) {
            vertex->color[c] = data[0x60 + c];
        }
    }
    gZrwDrawPolys(drawFlags(read16(data, 0x64)), &gScratch.boxPolys, 12, (const ZMatrix *)(data + 0x20), 0);
}

/* ---- life cycle ----------------------------------------------------------------------------------- */

BMX_ADDR(0x0047f660)
int BMX_CDECL DebugUpdateFunction(void *arg)
{
    const uint8_t *cursor = gTextQueue;
    int nextY = 0;

    (void)arg;
    if (gZrwDbgCharset != NULL) {
        if (gZrwDbgFpsMode != 0) {
            const int32_t *metrics = RwEngineGetMetrics();

            if (metrics == NULL) {
                ZDebugPrintBlya(0, -1, "FPS:%02d", gZrwFps);
                if (gZrwDebugLightOn != 0) {
                    ZDebugPrintBlya(0, -1, "PVS:Saved=%02d", (int)(gZrwPvsSavedA - gZrwPvsSavedB));
                }
            } else {
                ZDebugPrintBlya(0, -1,
                                "FPS:%02d numTri:%05d (Proc:%05d) Verts:%05d ResAlloc:%02d TxrUpload:%02d",
                                gZrwFps, metrics[0], metrics[1], metrics[2], metrics[5], metrics[3]);
            }
            if (gZrwDbgFpsMode > 1) {
                zrwRunCallbacks(&gZrwDbgCallbacks);
            }
            gZrwDbgFpsMode = 0;
        }
        if (gTextEnd > cursor) {
            do {
                const int x = *(const int16_t *)cursor;
                int y = *(const int16_t *)(cursor + 2);
                const int height = gZrwDbgCharHeight;
                const char *text = (const char *)cursor + 4;

                if (y < 0) {
                    y = nextY - (y + 1) * height;
                }
                y = y > 0 ? y : 0;
                nextY = y + height;
                RtCharsetPrint(gZrwDbgCharset, text, x > 0 ? x : 0, y);
                cursor += 4 + (uint32_t)zstrlen(text);
                cursor = (const uint8_t *)(((uintptr_t)cursor + 2) & ~(uintptr_t)1);
            } while (cursor < gTextEnd);
        }
        gTextEnd = gTextQueue;
    }
    gZrwDbgFrameStart();
    if (gZrwDbgItemCount != 0) {
        const ZrwDbgItem *item = &gItems[0];

        for (int left = gZrwDbgItemCount; left != 0; left--, item++) {
            const uint8_t *const data = (const uint8_t *)item->data;

            switch (item->type) {
            case kZrwDbgAxes:
                zrwDbgDrawAxes(data);
                break;
            case kZrwDbgLine:
                zrwDbgDrawLine(data);
                break;
            case kZrwDbgTriangle:
                zrwDbgDrawTriangle(data);
                break;
            case kZrwDbgSphere:
                zrwDbgDrawSphere(data);
                break;
            case kZrwDbgBox:
                zrwDbgDrawBox(data);
                break;
            default:
                gZdebugPrint("zrwDbg: Unkown geometry type");
                break;
            }
        }
    }
    gZrwDbgItemCount = 0;
    return 1;
}

BMX_ADDR(0x0047f850)
int zrwDbgInitialize(void)
{
    /* RtCharsetDesc is 5 words (the original reserves 0x14 bytes); RtCharsetGetDesc writes all of them.
     * A 2-word buffer let it overwrite the saved registers and return address in the real game. */
    int32_t desc[5];

    gZrwDbgInitialized = 1;
    zrwListInit(&gZrwDbgCallbacks);
    gZrwDbgCharset = RtCharsetCreate((const RwRGBA *)&gZrwClearColor2, (const RwRGBA *)&gZrwClearColor);
    if (gZrwDbgCharset == NULL) {
        zdebugFail("Cannot create raster chraset.", kZrwDbgFile, 0x33b, 0);
        return 0;
    }
    RtCharsetGetDesc(gZrwDbgCharset, (RtCharsetDesc *)desc);
    gZrwDbgCharWidth = desc[0];
    gZrwDbgCharHeight = desc[1];
    gZrwDbgUpdateNode = gZrwDbgRegister(4, BMX_FNPTR(ZrwUpdateFn, 0x0047f660, DebugUpdateFunction), NULL, 1);
    gTextEnd = gTextQueue;
    gZrwDbgItemCount = 0;
    gDebugIndent = 0;
    return 1;
}

BMX_ADDR(0x0047f900)
int zrwDbgShutdown(void)
{
    if (gZrwDbgUpdateNode != NULL) {
        gZrwDbgUnregister(gZrwDbgUpdateNode);
    }
    if (gZrwDbgCharset != NULL) {
        RtCharsetDestroy(gZrwDbgCharset);
    }
    gZrwDbgCharset = NULL;
    gZrwDbgInitialized = 0;
    return 1;
}

BMX_ADDR(0x004801f0)
ZrwUpdateNode *zrwDbgAddCallback(int unused, ZrwUpdateFn fn, void *arg)
{
    (void)unused;
    if (gZrwDbgInitialized == 0) {
        zdebugFail("Not initialized", kZrwDbgFile, 0x513, 0);
    }
    return zrwAddCallback(&gZrwDbgCallbacks, fn, arg, 0);
}

BMX_ADDR(0x00480230)
int zrwDbgRemoveCallback(ZrwUpdateNode *node)
{
    return zrwUnregisterUpdateFunction(node);
}
