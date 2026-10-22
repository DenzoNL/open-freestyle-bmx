/*
 * Renderer debug output (C:\NgEngine\Zge\Com\ZglRw\zrwDbg.c): text lines and wire shapes queued during a
 * frame and drawn by an update callback. Text goes through a charset the renderer prints with, shapes (lines,
 * triangles, spheres, boxes, matrix axes) through the immediate mode draw hooks.
 */
#ifndef ZGE_COM_ZGLRW_ZRWDBG_H
#define ZGE_COM_ZGLRW_ZRWDBG_H

#include <stdint.h>

#include "compat/orig.h"
#include "zge/com/zglrw/zrwzgl.h"
#include "zge/zmath.h"

/* Kinds of queued shapes (ZrwDbgItem.type). */
enum {
    kZrwDbgAxes = 0,
    kZrwDbgLine = 1,
    kZrwDbgTriangle = 2,
    kZrwDbgSphere = 3,
    kZrwDbgBox = 4,
    kZrwDbgMaxItems = 0x200,
};

/* Queued shape: type word and 0x68 bytes of shape specific data (see the zrwDbgAdd functions). */
typedef struct ZrwDbgItem {
    uint32_t type;
    uint32_t data[26];
} ZrwDbgItem;

_Static_assert(sizeof(ZrwDbgItem) == 0x6c, "debug item");

/* Vertex of the immediate mode primitives (0x1c bytes). */
typedef struct ZrwDbgVertex {
    float x, y, z; /* 0x00 */
    uint32_t pad0c;
    uint32_t u;       /* 0x10 */
    uint32_t v;       /* 0x14 */
    uint8_t color[4]; /* 0x18 */
} ZrwDbgVertex;

_Static_assert(sizeof(ZrwDbgVertex) == 0x1c, "debug vertex");

/* A primitive handed to the draw hook: a head word and the vertex pointers. */
typedef struct ZrwDbgPoly2 {
    uint32_t head;
    ZrwDbgVertex *verts[2];
} ZrwDbgPoly2;

typedef struct ZrwDbgPoly3 {
    uint32_t head;
    ZrwDbgVertex *verts[3];
} ZrwDbgPoly3;

/* Renderer draw hooks: (flags, primitives, count, matrix, 0). */
typedef void(BMX_CDECL *ZrwDrawFn)(uint32_t flags, const void *prims, int count, const ZMatrix *matrix,
                                   int zero);
#define gZrwDrawPolys    ORIG_VAR(0x00561728, ZrwDrawFn, gZrwDrawPolys)
#define gZrwDrawTriangle ORIG_VAR(0x0056172c, ZrwDrawFn, gZrwDrawTriangle)
#define gZrwDrawSphere   ORIG_VAR(0x00561730, ZrwDrawFn, gZrwDrawSphere)

/* Queue hooks the rest of the engine calls through function pointers (0x560064.. in the exe's data). */
typedef void(BMX_CDECL *ZrwDbgLineFn)(const ZVec3 *a, const ZVec3 *b, const uint32_t *color, int flags);
typedef void(BMX_CDECL *ZrwDbgTriangleFn)(const ZVec3 *a, const ZVec3 *b, const ZVec3 *c,
                                          const uint32_t *color, int flags);
typedef void(BMX_CDECL *ZrwDbgSphereFn)(const ZVec3 *center, float radius, const uint32_t *color, int flags);
#define gZrwDbgLineHook     ORIG_VAR(0x00560064, ZrwDbgLineFn, gZrwDbgLineHook)
#define gZrwDbgTriangleHook ORIG_VAR(0x00560068, ZrwDbgTriangleFn, gZrwDbgTriangleHook)
#define gZrwDbgSphereHook   ORIG_VAR(0x0056006c, ZrwDbgSphereFn, gZrwDbgSphereHook)

/* State of the debug output. */
#define gZrwDbgInitialized ORIG_VAR(0x00650470, int, gZrwDbgInitialized)
#define gZrwDbgFpsMode     ORIG_VAR(0x00650474, uint8_t, gZrwDbgFpsMode)
#define gZrwDbgUpdateNode  ORIG_VAR(0x00650478, ZrwUpdateNode *, gZrwDbgUpdateNode)
#define gZrwDbgCallbacks   ORIG_VAR(0x0065047c, ZrwList, gZrwDbgCallbacks)
#define gZrwDbgCharset     ORIG_VAR(0x00650488, void *, gZrwDbgCharset)
#define gZrwDbgCharWidth   ORIG_VAR(0x0065048c, int, gZrwDbgCharWidth)
#define gZrwDbgCharHeight  ORIG_VAR(0x00650490, int, gZrwDbgCharHeight)
#define gZrwDbgItemCount   ORIG_VAR(0x00650a98, int, gZrwDbgItemCount)

/* ---- text ----------------------------------------------------------------------------------------- */

/* printf-style debug output into a buffer (0x650494) that the shared string functions also use. */
void BMX_CDECL DebugString(const char *format, ...);
/* Queues a text line at pixel (x, y); a negative y stacks it below the previous line. Returns its length. */
int BMX_CDECL ZDebugPrintBlya(int16_t x, int16_t y, const char *format, ...);
/* Prints text directly with the charset. */
void zrwDbgPrintDirect(const char *text, int x, int y);
/* Formatted values: floats, vectors, 4x3 matrices (column by column), angles. `label` may be NULL. */
void zrwDbgPrintFloat(int x, int y, const char *label, float value);
void zrwDbgPrintMatrix(int x, int y, const char *label, const float *m);
void zrwDbgPrintVec4(int x, int y, const char *label, const float *v);
void zrwDbgPrintVec3(int x, int y, const char *label, const float *v);
void zrwDbgPrintAngle(int x, int y, const char *label, int16_t angle);
void zrwDbgPrintAngles(int x, int y, const char *label, const int16_t *angles);
/* Shows the frame statistics line next frame (0: off by value + 1). */
void zrwDbgSetFpsMode(int8_t mode);

/* ---- shapes --------------------------------------------------------------------------------------- */

void zrwDbgDrawLine(const uint8_t *data);
void zrwDbgDrawAxes(const uint8_t *data);
void zrwDbgDrawTriangle(const uint8_t *data);
void zrwDbgDrawSphere(const uint8_t *data);
void zrwDbgDrawBox(const uint8_t *data);

void zrwDbgAddLine(const ZVec3 *a, const ZVec3 *b, const uint32_t *color, uint16_t flags);
/* Matrix axes: copies the rotation of `matrix` (when not NULL); `mode` bit 0 uses the camera instead. */
void zrwDbgAddAxes(const ZMatrix *matrix, uint16_t mode, uint16_t flags);
void zrwDbgAddTriangle(const ZVec3 *a, const ZVec3 *b, const ZVec3 *c, const uint32_t *color, uint16_t flags);
void zrwDbgAddSphere(const ZVec3 *center, float radius, const uint32_t *color, uint16_t flags);
/* Box from 8 words (min, max corners) and an optional matrix. */
void zrwDbgAddBox(const uint32_t *corners, const ZMatrix *matrix, const uint32_t *color, uint16_t flags);

/* ---- life cycle ----------------------------------------------------------------------------------- */

/* Update callback: prints the queued text and draws the queued shapes, then empties both queues. */
int BMX_CDECL DebugUpdateFunction(void *arg);
int zrwDbgInitialize(void);
int zrwDbgShutdown(void);
/* Adds / removes a callback of the debug output's own list. */
ZrwUpdateNode *zrwDbgAddCallback(int unused, ZrwUpdateFn fn, void *arg);
int zrwDbgRemoveCallback(ZrwUpdateNode *node);

#endif
