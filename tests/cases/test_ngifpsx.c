/*
 * Tests for the display list primitive writers of NgIFpsx.c (clip, sprite, rectangle, line) and the
 * callbacks of its function table. The writers fill a primitive buffer pre-filled with random bytes, so
 * bytes the original leaves alone must stay alone, and link it into an ordering table.
 */
#include "bmx/com/zge/ngif/ngifpsx.h"
#include "dt.h"
#include "dt_ngif.h"

enum { kPrimBytes = 0x100, kOtWords = 8 };

/* Primitive buffer, ordering table and the colour state with edge values mixed in. */
static void setup_display_list(DtCtx *c)
{
    uint32_t *prim = dt_alloc(kPrimBytes);
    uint32_t *ot = dt_alloc(kOtWords * 4);

    ngBegin(c, 0);
    dt_fill(prim, kPrimBytes);
    dt_fill(ot, kOtWords * 4);
    gNgifPrim = prim;
    gNgifOrderTable = ot + kOtWords - 2;
    gNgifColor = (dt_u32() & 3) ? dt_u32() : 0;
    gNgifBrightness = (dt_u32() & 3) ? dt_u32() : (dt_u32() & 1 ? 0 : 0xff);
    gNgifScreenW = (dt_u32() & 3) ? 640.0f : dt_float_edgy(0.0f, 2000.0f);
    gNgifScreenH = (dt_u32() & 3) ? 480.0f : dt_float_edgy(0.0f, 2000.0f);
}

/* A coordinate with junk in the upper bits (the original only reads 16) and edge values. */
static uint32_t coord(void)
{
    const uint32_t v = dt_u32();

    return (v & 3) == 0 ? (v & 0xffffu) : ((v >> 4) & 0x3ff) | ((v & 4) ? (v & 0xffff0000u) : 0);
}

static void setup_clip(DtCtx *c)
{
    uint32_t x0, y0, x1, y1;

    setup_display_list(c);
    x0 = coord();
    y0 = coord();
    x1 = coord();
    y1 = coord();
    if (dt_u32() % 3 == 0) {
        /* the full-screen rectangle (any junk in the upper halves) */
        const int flip = dt_u32() & 1;

        x1 = flip ? x0 - 0x27f : x0 + 0x27f;
        y1 = (dt_u32() & 1) ? y0 - 0x1df : y0 + 0x1df;
        if (dt_u32() & 1) {
            x1 ^= 0x10000u * (dt_u32() & 0xf);
        }
    }
    if (dt_u32() % 8 == 0) {
        gNgifPrim = NULL;
    }
    DT_ARG_INT(c, 0, x0);
    DT_ARG_INT(c, 1, y0);
    DT_ARG_INT(c, 2, x1);
    DT_ARG_INT(c, 3, y1);
}

static void setup_sprite(DtCtx *c)
{
    const uint32_t frames = (uint32_t)dt_range(1, 12);
    uint8_t *set = dt_alloc((frames + 2) * 0x10);

    setup_display_list(c);
    dt_fill(set, (frames + 2) * 0x10);
    DT_ARG_PTR(c, 0, set);
    DT_ARG_INT(c, 1, (dt_u32() & 0xffff0000u) | (uint32_t)dt_range(0, (int)frames - 1));
    DT_ARG_INT(c, 2, dt_u32());
    DT_ARG_INT(c, 3, (dt_u32() & 1) ? (dt_u32() & 0xffffff00u) | (uint32_t)(dt_u32() & 1) : dt_u32());
    DT_ARG_INT(c, 4, dt_u32());
    DT_ARG_INT(c, 5, dt_u32());
    DT_ARG_INT(c, 6, dt_u32());
    DT_ARG_INT(c, 7, coord());
    DT_ARG_INT(c, 8, coord());
    DT_ARG_INT(c, 9, coord());
    DT_ARG_INT(c, 10, coord());
    DT_ARG_INT(c, 11, dt_u32());
    DT_ARG_INT(c, 12, dt_u32());
}

static void setup_rect(DtCtx *c)
{
    setup_display_list(c);
    DT_ARG_INT(c, 0, dt_u32());
    DT_ARG_INT(c, 1, (dt_u32() & 1) ? 1u + (dt_u32() & 0xffffff00u) : dt_u32());
    DT_ARG_INT(c, 2, coord());
    DT_ARG_INT(c, 3, coord());
    DT_ARG_INT(c, 4, coord());
    DT_ARG_INT(c, 5, coord());
}

static void setup_nop(DtCtx *c)
{
    ngBegin(c, 0);
}

DT_CASES(ngifpsx_cases) = {
    {"ngifSetClip", 0x00464030, ngifSetClip, 4, DT_RET_VOID, setup_clip, 2000, 0},
    {"ngifNop", 0x00464200, ngifNop, 0, DT_RET_VOID, setup_nop, 2, 0},
    {"ngifDrawSprite", 0x00464220, ngifDrawSprite, 13, DT_RET_VOID, setup_sprite, 1000, 0},
    {"ngifFillRect", 0x00464420, ngifFillRect, 6, DT_RET_VOID, setup_rect, 1000, 0},
    {"ngifDrawLine", 0x004645d0, ngifDrawLine, 6, DT_RET_VOID, setup_rect, 1000, 0},
    {"ngifReturnOneA", 0x004646b0, ngifReturnOneA, 0, DT_RET_INT, setup_nop, 2, 0},
    {"ngifReturnOneB", 0x004646d0, ngifReturnOneB, 0, DT_RET_INT, setup_nop, 2, 0},
};

/* ---- ordering table, polygons and frame control (stand-ins for the engine hooks) ---------------------- */

static void *gCamera;

static void logVertex(const uint32_t *v)
{
    ngLog(v[0]);
    ngLog(v[1]);
    ngLog(v[2]);
    ngLog(v[4]);
    ngLog(v[5]);
    ngLog(v[6]);
}

static int BMX_CDECL stubRegister(int type, void *cb, void *arg, int append)
{
    ngLog(0x48ba30);
    ngLog((uint32_t)type);
    ngLog((uint32_t)(uintptr_t)cb);
    ngLogPtr(arg);
    ngLog((uint32_t)append);
    return (int)(dt_u32() | 1);
}

static void BMX_CDECL stubUnregister(int node)
{
    ngLog(0x48bad0);
    ngLog((uint32_t)node);
}

static void BMX_CDECL stubQuery(int what, void *out, int arg)
{
    ngLog(0x48ad20);
    ngLog((uint32_t)what);
    ngLog((uint32_t)arg);
    ngLog(*(uint32_t *)out);
    *(float *)out = dt_float(100.0f, 1000.0f);
}

static void *BMX_CDECL stubGetCamera(void)
{
    ngLog(0x493070);
    return gCamera;
}

static void BMX_CDECL stubSetViewport(void *cam, int flags, const uint32_t *rect)
{
    ngLog(0x492de0);
    ngLogPtr(cam);
    ngLog((uint32_t)flags);
    ngLog(rect != NULL);
    for (int i = 0; rect != NULL && i < 4; i++) {
        ngLog(rect[i]);
    }
}

static void BMX_CDECL stubSetOffset(void *cam, uint32_t x, uint32_t y)
{
    ngLog(0x493130);
    ngLogPtr(cam);
    ngLog(x);
    ngLog(y);
}

static void drawLog(uint32_t tag, uint32_t flags, const uint32_t *prims, int n, int count, const void *m,
                    int z)
{
    ngLog(tag);
    ngLog(flags);
    ngLog((uint32_t)count);
    ngLogPtr(m);
    ngLog((uint32_t)z);
    ngLog(prims[0]);
    for (int i = 0; i < n; i++) {
        logVertex((const uint32_t *)(uintptr_t)prims[1 + i]);
    }
}

static void BMX_CDECL stubDraw2(uint32_t f, const uint32_t *p, int count, const void *m, int z)
{
    drawLog(0x48ccf0, f, p, 2, count, m, z);
}

static void BMX_CDECL stubDraw3(uint32_t f, const uint32_t *p, int count, const void *m, int z)
{
    drawLog(0x48d120, f, p, 3, count, m, z);
}

static void BMX_CDECL stubDraw4(uint32_t f, const uint32_t *p, int count, const void *m, int z)
{
    drawLog(0x48d660, f, p, 4, count, m, z);
}

static void BMX_CDECL stubKind(const uint8_t *prim, int blend)
{
    ngLog(0x4611d0);
    ngLogPtr(prim);
    ngLog((uint32_t)blend);
}

static void BMX_CDECL stubDrawPrim(const uint8_t *prim)
{
    ngLog(0x462050);
    ngLogPtr(prim);
    ngLog(prim[7]);
}

static uint32_t BMX_CDECL stubTexture(uint32_t w, uint32_t h, uint32_t bpp, const uint8_t *px,
                                      const uint8_t *pal)
{
    ngLog(0x47c030);
    ngLog(w);
    ngLog(h);
    ngLog(bpp);
    ngLogPtr(px);
    for (int i = 0; i < 4 + 0x20; i++) {
        ngLog(pal[i]);
    }
    return dt_u32() | 1;
}

static void psxBegin(DtCtx *c)
{
    ngBegin(c, 1);
    gCamera = dt_alloc(16);
    dt_stub(0x0048ba30, NULL, (void *)stubRegister);
    dt_stub(0x0048bad0, NULL, (void *)stubUnregister);
    dt_stub(0x0048ad20, NULL, (void *)stubQuery);
    dt_stub(0x00493070, NULL, (void *)stubGetCamera);
    dt_stub(0x00492de0, NULL, (void *)stubSetViewport);
    dt_stub(0x00493130, NULL, (void *)stubSetOffset);
    dt_stub(0x0048ccf0, NULL, (void *)stubDraw2);
    dt_stub(0x0048d120, NULL, (void *)stubDraw3);
    dt_stub(0x0048d660, NULL, (void *)stubDraw4);
    dt_stub(0x004611d0, NULL, (void *)stubKind);
    dt_stub(0x00460b40, NULL, (void *)stubKind);
    dt_stub(0x0045ffd0, NULL, (void *)stubKind);
    dt_stub(0x0047c030, NULL, (void *)stubTexture);
    gNgifScreenW = (dt_u32() & 3) ? 640.0f : dt_float_edgy(0.0f, 2000.0f);
    gNgifScreenH = (dt_u32() & 3) ? 480.0f : dt_float_edgy(0.0f, 2000.0f);
}

static void setupScriptStep(DtCtx *c)
{
    int8_t *script = dt_alloc(8);

    psxBegin(c);
    dt_fill(script, 8);
    if ((dt_u32() & 3) == 0) {
        script[0] = -0x80;
    }
    gNgifScript = (dt_u32() & 3) ? script : NULL;
    gNgifScriptNode = (void *)(uintptr_t)dt_u32();
}

static void setupScriptSet(DtCtx *c)
{
    psxBegin(c);
    gNgifScriptNode = (dt_u32() & 1) ? (void *)(uintptr_t)dt_u32() : NULL;
    DT_ARG_PTR(c, 0, dt_alloc(16));
}

static void setupDrawArea(DtCtx *c)
{
    psxBegin(c);
    DT_ARG_PTR(c, 0, dt_alloc(0x40));
    DT_ARG_PTR(c, 1, dt_alloc(8));
    dt_fill((void *)(uintptr_t)c->args[0], 0x40);
    dt_fill((void *)(uintptr_t)c->args[1], 8);
}

static void setupAreaPrim(DtCtx *c)
{
    uint32_t *prim = dt_alloc(0x40);

    psxBegin(c);
    for (int i = 0; i < 16; i++) {
        prim[i] = (dt_u32() & 3) ? (dt_u32() & 0xffff) : dt_u32();
    }
    if (dt_u32() % 3 == 0) {
        prim[4] = prim[5] = 0;
    }
    DT_ARG_PTR(c, 0, prim);
}

static void setupInitOt(DtCtx *c)
{
    uint32_t *buf = dt_alloc(70 * 4);
    const int n = dt_range(-2, 40);

    psxBegin(c);
    dt_fill(buf, 70 * 4);
    DT_ARG_PTR(c, 0, buf + 8);
    DT_ARG_INT(c, 1, (dt_u32() & 1) ? (uint32_t)n : (dt_u32() & 0xffff0000u) | (uint32_t)(n & 0xffff));
}

/* A random primitive of 0x24 words; the interesting flag words and bytes get edge values. */
/* Links in a display list are 28 bit: such primitives live in the exe's data (0x678560, the 3D buffer). */
static uint8_t *lowPrim(int index, uint8_t code);

static uint8_t *randomPrim(uint8_t code)
{
    uint8_t *p = dt_alloc(0x94);

    dt_fill(p, 0x94);
    for (int i = 0x3c; i < 0x94; i += 4) {
        const float f = dt_float_edgy(-500.0f, 500.0f);
        memcpy(p + i, &f, 4);
    }
    for (int i = 0x14; i < 0x30; i += 4) {
        const uint32_t v = (dt_u32() & 1) ? dt_u32() : (dt_u32() & 0xfe) << (dt_u32() & 7);
        memcpy(p + i, &v, 4);
    }
    p[7] = code;
    return p;
}

static const uint8_t kCodes[] = {0x20, 0x22, 0x24, 0x28, 0x2a, 0x2c, 0x2e, 0x30, 0x34, 0x38,
                                 0x3a, 0x3c, 0x3e, 0x40, 0x42, 0x48, 0x4c, 0x50, 0x52, 0x58,
                                 0x5c, 0x64, 0x74, 0x7c, 0xf8, 0x00, 0x04, 0x60, 0xff};

static uint8_t *lowPrim(int index, uint8_t code)
{
    uint8_t *p = (uint8_t *)(uintptr_t)(0x00678570u + (uint32_t)index * 0x94u);

    memcpy(p, randomPrim(code), 0x94);
    return p;
}

static void setupPolyF3(DtCtx *c)
{
    psxBegin(c);
    DT_ARG_PTR(c, 0, randomPrim(0x20));
    DT_ARG_INT(c, 1, (dt_u32() & 1) ? 2 : (dt_u32() & 3) ? 0 : dt_u32());
}

static void setupDrawPrim(DtCtx *c)
{
    psxBegin(c);
    DT_ARG_PTR(c, 0, randomPrim(kCodes[dt_u32() % sizeof kCodes]));
}

/* Chain of 0..4 primitives behind an entry word; some have no size nibble (skipped). */
static void setupDrawList(DtCtx *c)
{
    const int n = dt_range(0, 4);
    uint32_t *entry = (uint32_t *)(uintptr_t)0x00678560u;
    uint32_t *prev = entry;

    psxBegin(c);
    dt_stub(0x00462050, NULL, (void *)stubDrawPrim);
    for (int i = 0; i < n; i++) {
        uint32_t *p = (uint32_t *)lowPrim(i + 1, kCodes[dt_u32() % sizeof kCodes]);

        *prev = (*prev & 0xf0000000u) | ((uint32_t)(uintptr_t)p & 0x0fffffffu);
        p[0] = (dt_u32() & 3) ? 0x90000000u : 0;
        prev = p;
    }
    *prev = (*prev & 0xf0000000u) | 0x0fffffffu;
    DT_ARG_PTR(c, 0, entry);
}

static void setupLine3D(DtCtx *c)
{
    uint8_t *prim = dt_alloc(0x80);
    uint32_t *ot = dt_alloc(0x40);
    NgifVert3 *v = dt_alloc(2 * sizeof *v);

    psxBegin(c);
    dt_fill(prim, 0x80);
    dt_fill(ot, 0x40);
    dt_fill(v, 2 * sizeof *v);
    gNgifPrim3D = (uint32_t *)prim;
    gNgifOtBase = ot;
    DT_ARG_INT(c, 0, dt_u32());
    DT_ARG_PTR(c, 1, v);
    DT_ARG_PTR(c, 2, v + 1);
}

static void setupFrame(DtCtx *c)
{
    uint32_t *ot = dt_alloc(0x400 * 4);

    psxBegin(c);
    dt_stub(0x00462050, NULL, (void *)stubDrawPrim);
    dt_fill(ot, 0x400 * 4);
    gNgifOtBase = ot;
    gNgifPrim3D = (uint32_t *)(uintptr_t)dt_u32();
    gNgifFlushPending = (int)(dt_u32() & 1);
    gNgifFlushPending2 = (int)(dt_u32() & 1);
    gNgifFlushNode = (dt_u32() & 1) ? (void *)(uintptr_t)dt_u32() : NULL;
    gNgifFlushNode2 = (dt_u32() & 1) ? (void *)(uintptr_t)dt_u32() : NULL;
    gNgifPrimStart = (dt_u32() & 1) ? (uint32_t *)(uintptr_t)dt_u32() : NULL;
    gNgifPrim = (uint32_t *)(uintptr_t)dt_u32();
    gNgifOrderTable = (uint32_t *)(uintptr_t)dt_u32();
    if (dt_u32() & 1) {
        /* a short chain from the start entry */
        uint32_t *p = (uint32_t *)lowPrim(1, 0x20);

        p[0] = 0x90000000u | 0x0fffffffu;
        ot[0x3ff] = (uint32_t)(uintptr_t)p & 0x0fffffffu;
    } else {
        ot[0x3ff] = 0x0fffffffu;
    }
    DT_ARG_PTR(c, 0, ot);
    DT_ARG_INT(c, 1, dt_u32());
}

static void setupPicture(DtCtx *c)
{
    uint32_t *pic = dt_alloc(0x30);

    setup_display_list(c);
    dt_fill(pic, 0x30);
    DT_ARG_PTR(c, 0, pic);
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_INT(c, 2, dt_u32());
}

static void setupBlend(DtCtx *c)
{
    psxBegin(c);
    DT_ARG_INT(c, 0, dt_u32());
    DT_ARG_INT(c, 1, dt_u32());
}

static void setupTableAccess(DtCtx *c)
{
    psxBegin(c);
    gNgifOrderTable = (uint32_t *)(uintptr_t)dt_u32();
    DT_ARG_INT(c, 0, dt_u32());
}

typedef struct ZMemFreeList *(BMX_CDECL *PsxListCreateFn)(uint32_t, uint32_t, void *);

/* Font with a clut block of `count` colours and a page table that is empty, filled or missing. */
static void setupGlyph(DtCtx *c)
{
    const int bad = dt_u32() % 8;
    const uint32_t count = bad == 0 ? (uint32_t)dt_range(0x7fe, 0x800) : (uint32_t)dt_range(0x120, 0x140);
    uint8_t *font = dt_alloc(0x40 + 0x1000);
    uint8_t *glyph = dt_alloc(8);
    uint32_t *table = dt_alloc(0x80);
    uint8_t *clut = font + 0x40;
    uint32_t idx = (uint32_t)dt_range(0, 15);

    setup_display_list(c);
    psxBegin(c);
    dt_stub(0x0047ed10, NULL, (void *)ngFail);
    gNgifGlyphPages = DT_ORIG(0x00485e80, PsxListCreateFn)(0x80, 2, NULL);
    gNgifPrim = dt_alloc(0xa0);
    dt_fill(gNgifPrim, 0xa0);
    gNgifOrderTable = dt_alloc(16);
    dt_fill(font, 0x40 + 0x1000);
    dt_fill(glyph, 8);
    *(uint32_t *)(font + 4) = 0x40;
    *(uint16_t *)(font + 8) = (uint16_t)dt_range(1, 64);
    *(uint16_t *)(font + 10) = (uint16_t)dt_range(1, 64);
    *(uint16_t *)(font + 0x14) = (uint16_t)dt_range(1, 8);
    *(uint32_t *)(font + 0x10) = 0x40;
    *(uint16_t *)(clut + 2) = (uint16_t)count;
    for (int i = 0; i < 16; i++) {
        table[i] = (dt_u32() & 1) ? dt_u32() : 0;
    }
    *(uint32_t *)(font + 0xc) = (dt_u32() & 3) ? (uint32_t)(uintptr_t)table : 0;
    if (dt_u32() % 8 == 0) {
        glyph[2] = glyph[3] = 0;
    }
    if (bad == 1) {
        idx = (uint32_t)dt_range(16, 17);
    }
    DT_ARG_PTR(c, 0, font);
    DT_ARG_PTR(c, 1, glyph);
    DT_ARG_INT(c, 2, (dt_u32() & 1) ? 1 : (int)dt_u32());
    DT_ARG_INT(c, 3, dt_u32());
    DT_ARG_INT(c, 4, dt_u32());
    DT_ARG_INT(c, 5, (int16_t)dt_u32());
    DT_ARG_INT(c, 6, (int16_t)dt_u32());
    DT_ARG_INT(c, 7, dt_u32());
    DT_ARG_INT(c, 8, (dt_u32() & 0xffff0000u) | idx);
}

/* ---- 3D list: screen rectangles, glyph records, polygons --------------------------------------------- */

/*
 * The 3D list buffer, its ordering table (entry words random: the links must be rebuilt from them), and
 * the texture table: 16 entries, some unused (material 0). Asserts are not stubbed: reaching one ends the
 * run and the abort is compared.
 */
static uint8_t *setupList3D(DtCtx *c, uint32_t size, uint32_t none)
{
    uint8_t *prim = dt_alloc(size);
    uint32_t *ot = dt_alloc(0x40 * 4);

    ngBegin(c, 0);
    dt_fill(prim, size);
    dt_fill(ot, 0x40 * 4);
    gNgifPrim3D = (dt_u32() & 31) == 0 ? (uint32_t *)(uintptr_t)none : (uint32_t *)prim;
    gNgifOtBase = ot;
    for (uint32_t i = 0; i < 16; i++) {
        NgifTexture *t = &(&gNgifTextures)[i];

        dt_fill(t, sizeof *t);
        t->width = (uint8_t)dt_range(0, 200);
        t->height = (uint8_t)dt_range(0, 200);
        if ((dt_u32() & 7) == 0) {
            t->material = 0;
        } else {
            t->material = dt_u32() | 1;
        }
    }
    return prim;
}

static void setupFillBar(DtCtx *c)
{
    static const uint32_t bits[] = {1,     2,     8,      0x10,   0x20,   0x40, 0x100,
                                    0x400, 0x800, 0x1000, 0x2000, 0x4000, 0x80};
    uint32_t *info = dt_alloc(0x40);
    uint32_t flags = 0;
    uint32_t color;

    setupList3D(c, 0x200, 0);
    color = dt_u32();
    dt_fill(info, 0x40);
    for (uint32_t i = 0; i < sizeof bits / sizeof bits[0]; i++) {
        if ((dt_u32() & 3) == 0) {
            flags |= bits[i];
        }
    }
    flags |= (dt_u32() & 1) ? 4 : 0;
    if ((flags & 4) == 0 && (flags & 0x80) == 0 && (dt_u32() & 15) != 0) {
        flags |= 0x200;
    }
    if ((dt_u32() & 3) == 0) {
        flags |= 0x200;
    }
    if ((flags & 4) != 0) {
        color = (uint32_t)dt_range(0, 15);
    } else if ((dt_u32() & 3) != 0) {
        color &= 0x00ffffffu | ((dt_u32() & 3) == 0 ? 0xff000000u : 0);
    }
    for (uint32_t i = 0; i < 0x10; i++) {
        info[i] = (uint32_t)dt_range(-300, 600);
    }
    info[2] = (uint32_t)dt_range(0, 4);
    info[3] = (uint32_t)dt_range(0, 4);
    info[10] = (uint32_t)dt_range(0, 40);
    info[11] = (uint32_t)dt_range(0, 15);
    if ((flags & 0x4000) != 0) {
        for (uint32_t i = 0x30; i < 0x34; i++) {
            ((uint8_t *)info)[i] = (uint8_t)dt_u32();
        }
    }
    DT_ARG_INT(c, 0, color);
    DT_ARG_INT(c, 1, (dt_u32() & 3) == 0 ? dt_u32() : (uint32_t)dt_range(-100, 700));
    DT_ARG_INT(c, 2, (dt_u32() & 3) == 0 ? (int32_t)dt_u32() : dt_range(-100, 600));
    DT_ARG_INT(c, 3, flags);
    DT_ARG_PTR(c, 4, info);
}

/* A record list: up to four records of the drawable kinds, then the terminator (or a bad kind). */
static void setupTextGlyph(DtCtx *c)
{
    static const uint8_t kinds[] = {0, 0x11, 0x30, 0x31, 0x32};
    static const uint8_t sizes[] = {0xc, 0x14, 0xc, 0x14, 0x10};
    uint8_t *rec = dt_alloc(0x80);
    uint32_t off = 0;
    uint32_t n;

    setupList3D(c, 0x800, 0xffffffffu);
    n = (uint32_t)dt_range(0, 4);
    dt_fill(rec, 0x80);
    for (uint32_t i = 0; i < n; i++) {
        const uint32_t k = dt_u32() % 5;
        uint16_t *w = (uint16_t *)(void *)(rec + off);

        rec[off] = kinds[k];
        for (uint32_t j = 2; j < sizes[k]; j += 2) {
            w[j / 2] = (uint16_t)dt_range(-60, 400);
        }
        w[3] = (uint16_t)dt_range(0, 15);
        if (k >= 2) {
            w[1] = (uint16_t)(dt_u32() & 0xffff);
            w[2] = (uint16_t)dt_range(-20, 20);
        } else {
            w[1] = (uint16_t)(dt_u32() & 0xffff);
        }
        if (k == 4) {
            w[4] = (uint16_t)dt_range(1, 3);
            w[5] = (uint16_t)dt_range(1, 3);
        }
        off += sizes[k];
    }
    rec[off] = (dt_u32() & 31) == 0 ? (uint8_t)dt_range(1, 0x2f) : 0xff;
    DT_ARG_INT(c, 0, (dt_u32() & 3) == 0 ? dt_u32() : (dt_u32() & 0xffffu));
    DT_ARG_INT(c, 1, (dt_u32() & 3) == 0 ? (int32_t)dt_u32() : dt_range(-100, 600));
    DT_ARG_PTR(c, 2, (dt_u32() & 63) == 0 ? NULL : rec);
}

static int BMX_CDECL stubMaterialTexture(void *material, uint32_t index)
{
    ngLog(0x489a40);
    ngLog((uint32_t)(uintptr_t)material);
    ngLog(index);
    return 0x1234;
}

static int BMX_CDECL stubTextureWidth(const void *texture)
{
    ngLog(0x498870);
    ngLog((uint32_t)(uintptr_t)texture);
    return (int)dt_u32();
}

static int BMX_CDECL stubTextureHeight(const void *texture)
{
    ngLog(0x498880);
    ngLog((uint32_t)(uintptr_t)texture);
    return (int)dt_u32();
}

void *zrwmatGetTexture(void *material, uint32_t index);
int zrwTextureGetWidth(const void *texture);
int zrwTextureGetHeight(const void *texture);

static void setupPoly3D(DtCtx *c)
{
    NgifVert3 *v = dt_alloc(5 * sizeof *v);
    uint32_t flags;
    uint32_t count;
    uint32_t texture;

    setupList3D(c, 0x200, 0);
    dt_stub(0x00489a40, (void *)zrwmatGetTexture, (void *)stubMaterialTexture);
    dt_stub(0x00498870, (void *)zrwTextureGetWidth, (void *)stubTextureWidth);
    dt_stub(0x00498880, (void *)zrwTextureGetHeight, (void *)stubTextureHeight);
    dt_fill(v, 5 * sizeof *v);
    flags = (uint32_t)dt_range(0, 3) | ((dt_u32() & 15) == 0 ? 0 : 8) | (dt_u32() & 0xf0);
    count = dt_u32() % 6;
    texture = (dt_u32() & 3) == 0 ? 0 : (uint32_t)dt_range(1, 15);
    if (texture != 0 && (dt_u32() & 7) != 0) {
        count = 4;
    }
    if ((dt_u32() & 3) == 0) {
        texture |= dt_u32() & 0xffff0000u;
    }
    DT_ARG_INT(c, 0, flags | (dt_u32() & 0xffffff00u));
    DT_ARG_INT(c, 1, (dt_u32() & 7) == 0 ? (dt_u32() & 0xffffff00u) | count : count);
    DT_ARG_PTR(c, 2, v);
    DT_ARG_INT(c, 3, texture);
}

/* ---- quads (F4, FT4, GT4) ------------------------------------------------------------------------- */

/* Texture size the stubs report (low 16 bits used by the original; junk above). */
static uint32_t gQuadTexW, gQuadTexH;

static void *BMX_CDECL stubQuadTexture(uint32_t material, int index)
{
    ngLog(0x489a40);
    ngLog(material);
    ngLog((uint32_t)index);
    return (void *)(uintptr_t)0x5678u;
}

static int BMX_CDECL stubQuadWidth(const void *texture)
{
    ngLog(0x498870);
    ngLog((uint32_t)(uintptr_t)texture);
    return (int)gQuadTexW;
}

static int BMX_CDECL stubQuadHeight(const void *texture)
{
    ngLog(0x498880);
    ngLog((uint32_t)(uintptr_t)texture);
    return (int)gQuadTexH;
}

static uint32_t quadTexSize(void)
{
    static const uint16_t kSize[] = {0, 1, 2, 16, 64, 128, 256, 255, 0x7fff, 0x8000, 0xffff, 0xfff0};
    const uint32_t junk = (dt_u32() & 1) ? dt_u32() & 0xffff0000u : 0;

    return junk | kSize[dt_u32() % (sizeof kSize / sizeof kSize[0])];
}

/*
 * A quad primitive of 0xa0 bytes: random bytes, the flag word (at `flagOff`) mixing every tested bit, float
 * words (some raw: signalling / quiet NaNs, infinities) from 0x3c on, a material word that is sometimes 0,
 * and blend mode bytes with values above 3.
 */
static uint8_t *quadPrim(unsigned flagOff, unsigned materialOff, unsigned modeOff)
{
    static const uint32_t kRaw[] = {0x7f800001u, 0x7fa00000u, 0xffbfffffu, 0x7fc00000u, 0xffc00001u,
                                    0x7f800000u, 0xff800000u, 0x00000001u, 0x80000000u};
    uint8_t *p = dt_alloc(0xa0);
    uint32_t pf = dt_u32();

    dt_fill(p, 0xa0);
    for (unsigned i = 0x3c; i < 0xa0; i += 4) {
        const float f = dt_float_edgy(-500.0f, 500.0f);

        if (i == flagOff || i == materialOff) {
            continue;
        }
        if (dt_u32() % 10 == 0) {
            memcpy(p + i, &kRaw[dt_u32() % (sizeof kRaw / sizeof kRaw[0])], 4);
        } else if (dt_u32() % 4 != 0) {
            memcpy(p + i, &f, 4);
        }
    }
    if ((dt_u32() & 1) != 0) {
        pf &= 0xffffff00u | (dt_u32() & 0x6f);
    }
    memcpy(p + flagOff, &pf, 4);
    if (materialOff != 0 && dt_u32() % 5 == 0) {
        memset(p + materialOff, 0, 4);
    }
    p[modeOff] = (dt_u32() & 1) ? (uint8_t)dt_range(0, 3) : (uint8_t)dt_u32();
    if ((dt_u32() & 3) == 0) {
        /* texel / screen coordinates near the edges */
        static const uint16_t kEdge[] = {0, 1, 0xffff, 0x7fff, 0x8000, 0x8001};
        for (unsigned i = 8; i < 0x38; i += 2) {
            if ((dt_u32() & 3) == 0) {
                memcpy(p + i, &kEdge[dt_u32() % (sizeof kEdge / sizeof kEdge[0])], 2);
            }
        }
    }
    return p;
}

static void setupQuad(DtCtx *c, unsigned flagOff, unsigned materialOff, unsigned modeOff)
{
    /* not psxBegin(): that stubs the quad functions themselves for the dispatch tests */
    ngBegin(c, 1);
    dt_stub(0x0048d660, NULL, (void *)stubDraw4);
    gNgifScreenW = (dt_u32() & 3) ? 640.0f : dt_float_edgy(0.0f, 2000.0f);
    gNgifScreenH = (dt_u32() & 3) ? 480.0f : dt_float_edgy(0.0f, 2000.0f);
    dt_stub(0x00489a40, NULL, (void *)stubQuadTexture);
    dt_stub(0x00498870, NULL, (void *)stubQuadWidth);
    dt_stub(0x00498880, NULL, (void *)stubQuadHeight);
    gQuadTexW = quadTexSize();
    gQuadTexH = quadTexSize();
    DT_ARG_PTR(c, 0, quadPrim(flagOff, materialOff, modeOff));
    DT_ARG_INT(c, 1, (dt_u32() & 1) ? 2 : (dt_u32() & 3) ? 0 : dt_u32());
}

static void setupPolyGT4(DtCtx *c)
{
    setupQuad(c, 0x3c, 0, 0xa);
}

static void setupPolyFT4(DtCtx *c)
{
    setupQuad(c, 0x34, 0x30, 0x38);
}

static void setupPolyF4(DtCtx *c)
{
    setupQuad(c, 0x1c, 0, 0x20);
}

DT_CASES(ngifpsx_cases2) = {
    {"ngifpsxUpdate", 0x0045fcf0, ngifpsxUpdate, 0, DT_RET_INT, setupScriptStep, 300, 0},
    {"ngifpsxSetup", 0x0045fdc0, ngifpsxSetup, 1, DT_RET_VOID, setupScriptSet, 60, 0},
    {"ngifPrimSetDrawArea", 0x0045fe00, ngifPrimSetDrawArea, 2, DT_RET_VOID, setupDrawArea, 100, 0},
    {"ngifDrawAreaPrim", 0x0045fe50, ngifDrawAreaPrim, 1, DT_RET_VOID, setupAreaPrim, 300, 0},
    {"ngifInitOrderTable", 0x0045ff60, ngifInitOrderTable, 2, DT_RET_VOID, setupInitOt, 300, 0},
    {"ngifDrawPolyGT4", 0x0045ffd0, (void *)ngifDrawPolyGT4, 2, DT_RET_VOID, setupPolyGT4, 2000, 0},
    {"ngifDrawPolyFT4", 0x00460b40, (void *)ngifDrawPolyFT4, 2, DT_RET_VOID, setupPolyFT4, 2000, 0},
    {"ngifDrawPolyF4", 0x004611d0, (void *)ngifDrawPolyF4, 2, DT_RET_VOID, setupPolyF4, 2000, 0},
    {"ngifDrawPolyG4", 0x00460640, (void *)ngifDrawPolyG4, 2, DT_RET_VOID, setupPolyF3, 1000, 0},
    {"ngifDrawPolyF3", 0x00461700, (void *)ngifDrawPolyF3, 2, DT_RET_VOID, setupPolyF3, 1000, 0},
    {"ngifDrawLineF2", 0x00461af0, (void *)ngifDrawLineF2, 2, DT_RET_VOID, setupPolyF3, 1000, 0},
    {"ngifDrawLineG2", 0x00461da0, (void *)ngifDrawLineG2, 2, DT_RET_VOID, setupPolyF3, 1000, 0},
    {"ngifDrawPrimitive", 0x00462050, (void *)ngifDrawPrimitive, 1, DT_RET_VOID, setupDrawPrim, 600, 0},
    {"ngifDrawList", 0x00462350, ngifDrawList, 1, DT_RET_VOID, setupDrawList, 300, 0},
    {"ngifFillScreenBar", 0x004623a0, ngifFillScreenBar, 5, DT_RET_VOID, setupFillBar, 2000, 0},
    {"ngifDrawTextGlyph", 0x00462940, ngifDrawTextGlyph, 3, DT_RET_VOID, setupTextGlyph, 2000, 0},
    {"ngifDrawPoly3D", 0x00463130, ngifDrawPoly3D, 4, DT_RET_VOID, setupPoly3D, 2000, 0},
    {"ngifDrawLine3D", 0x00463710, ngifDrawLine3D, 3, DT_RET_VOID, setupLine3D, 300, 0},
    {"ngifNop2", 0x00463800, ngifNop2, 0, DT_RET_VOID, setupBlend, 2, 0},
    {"ngifResetOrderTable", 0x00463820, ngifResetOrderTable, 0, DT_RET_VOID, setupFrame, 30, 0},
    {"ngifFlushFrame", 0x00463860, ngifFlushFrame, 0, DT_RET_INT, setupFrame, 100, 0},
    {"ngifQueryScreen", 0x004638b0, ngifQueryScreen, 0, DT_RET_VOID, setupFrame, 30, 0},
    {"ngifRegisterFlush", 0x004638f0, ngifRegisterFlush, 0, DT_RET_VOID, setupFrame, 60, 0},
    {"ngifUnregisterFlush", 0x00463930, ngifUnregisterFlush, 0, DT_RET_VOID, setupFrame, 60, 0},
    {"ngifGetOrderTable", 0x00463960, ngifGetOrderTable, 0, DT_RET_INT, setupTableAccess, 20, 0},
    {"ngifSetOrderTable", 0x00463980, ngifSetOrderTable, 1, DT_RET_VOID, setupTableAccess, 20, 0},
    {"ngifFlushFrame2", 0x004639a0, ngifFlushFrame2, 0, DT_RET_INT, setupFrame, 100, 0},
    {"ngifInit", 0x004639f0, ngifInit, 0, DT_RET_INT, setupFrame, 60, 0},
    {"ngifShutdown", 0x00463a60, ngifShutdown, 0, DT_RET_VOID, setupFrame, 60, 0},
    {"ngifBegin", 0x00463ab0, ngifBegin, 0, DT_RET_VOID, setupFrame, 30, 0},
    {"ngifRequestFlush2", 0x00463b00, ngifRequestFlush2, 0, DT_RET_VOID, setupFrame, 4, 0},
    {"ngifSetPictureScreen", 0x00463b20, ngifSetPictureScreen, 3, DT_RET_VOID, setupPicture, 300, 0},
    {"ngifSetBlend", 0x00463c40, ngifSetBlend, 2, DT_RET_VOID, setupBlend, 20, 0},
    {"ngifDrawGlyph", 0x00463c70, ngifDrawGlyph, 9, DT_RET_VOID, setupGlyph, 1000, 0},
};
