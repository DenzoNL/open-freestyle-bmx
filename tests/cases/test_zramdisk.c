/*
 * Tests for the RAM disk device (zramdisk.c) and its renderer-side hook (zrwramdisk.c). The device entry
 * points are called directly, on handles the original open call made, with and without completion
 * callbacks (which log the events they get), with and without the file system initialised, and with the
 * broken names, modes and handles the asserts guard against.
 */
#include "dt.h"
#include "dt_ngif.h"
#include "dt_realfile.h"
#include "zge/com/zglrw/zrwramdisk.h"
#include "zge/zgemods/zramdisk.h"

typedef int(BMX_CDECL *OpenFn)(ZfileHandle *, const char *, int, const ZramdiskCallback *);
typedef int(BMX_CDECL *InitFn)(void);

#define O_OPEN DT_ORIG(0x0049ee80, OpenFn)
#define O_INIT DT_ORIG(0x0049f450, InitFn)

/* ---- callbacks ------------------------------------------------------------------------------------- */

static void BMX_CDECL cb_with_value(void *event)
{
    const ZramdiskEvent *e = event;

    ngLog(0xcb0001);
    ngLog(e->arg);
    ngLogPtr(e->file);
    ngLog(e->ok);
    ngLog(e->value);
}

/* Open, close and the unsupported slots leave the fourth word of the event unset. */
static void BMX_CDECL cb_no_value(void *event)
{
    const ZramdiskEvent *e = event;

    ngLog(0xcb0002);
    ngLog(e->arg);
    ngLogPtr(e->file);
    ngLog(e->ok);
}

static ZramdiskCallback *make_callback(int withValue)
{
    ZramdiskCallback *cb = dt_alloc(sizeof *cb);

    cb->fn = withValue ? cb_with_value : cb_no_value;
    cb->arg = dt_u32();
    return cb;
}

static ZramdiskCallback *maybe_callback(int withValue)
{
    return (dt_u32() % 3 == 0) ? NULL : make_callback(withValue);
}

/* ---- fixtures ------------------------------------------------------------------------------------- */

static void begin(DtCtx *c, int initialised)
{
    ngBegin(c, 0);
    dt_zmem_env();
    if (initialised) {
        O_INIT();
    }
}

/* An open file over a fresh memory block, made by the original code; *size gets the block size. */
static ZfileHandle *open_file(uint32_t *size, int mode)
{
    ZfileHandle *file = dt_alloc(sizeof *file);
    char *name = dt_alloc(48);
    uint8_t *block;
    uint32_t n = (uint32_t)dt_range(1, 300);
    uint32_t len = 0;
    static const char kHex[] = "0123456789abcdef";

    file->data = NULL;
    *size = 0;
    if (gZramdiskFiles == NULL) {
        return file; /* the file system is not up: opening would assert in the setup */
    }
    block = dt_alloc(n + 16);
    dt_fill(block, n + 16);
    /* "<address>:<size>" in hex, as zfileOpen hands it to the device after the "rd00:" prefix */
    for (int shift = 28; shift >= 0; shift -= 4) {
        name[len++] = kHex[((uint32_t)(uintptr_t)block >> shift) & 0xf];
    }
    name[len++] = ':';
    for (int shift = 28; shift >= 0; shift -= 4) {
        name[len++] = kHex[(n >> shift) & 0xf];
    }
    name[len] = '\0';
    O_OPEN(file, name, mode, NULL);
    *size = n;
    return file;
}

static int random_mode(void)
{
    return dt_u32() % 5 == 0 ? dt_range(0, 3) : 3;
}

/* ---- setups --------------------------------------------------------------------------------------- */

static void setup_name(DtCtx *c)
{
    begin(c, c->seed % 5 != 0);
    DT_ARG_PTR(c, 0, NULL);
}

static void setup_control(DtCtx *c)
{
    const char **keys = (const char **)dt_alloc(8);
    uint32_t *values = dt_alloc(8);
    const uint32_t k = dt_u32() % 8;

    begin(c, c->seed % 7 != 0);
    keys[0] = k == 1 ? "address" : "ADDRESS";
    keys[1] = k == 2 ? "SIZ" : "SIZE";
    if (k == 3) {
        keys[0] = "SIZE";
        keys[1] = "ADDRESS";
    }
    values[0] = dt_u32();
    values[1] = dt_u32();
    DT_ARG_PTR(c, 0, NULL);
    DT_ARG_INT(c, 1, k == 4 ? dt_range(0, 5) : 2);
    DT_ARG_PTR(c, 2, keys);
    DT_ARG_PTR(c, 3, values);
}

static void setup_slot(DtCtx *c)
{
    int *result = dt_alloc(4);

    begin(c, c->seed % 9 != 0);
    *result = (int)dt_u32();
    DT_ARG_INT(c, 0, dt_u32());
    DT_ARG_PTR(c, 1, (dt_u32() & 1) ? (void *)result : NULL);
    DT_ARG_PTR(c, 2, maybe_callback(1));
}

static void setup_slot4(DtCtx *c)
{
    int *result = dt_alloc(4);

    begin(c, c->seed % 9 != 0);
    *result = (int)dt_u32();
    DT_ARG_INT(c, 0, dt_u32());
    DT_ARG_INT(c, 1, dt_u32());
    DT_ARG_PTR(c, 2, (dt_u32() & 1) ? (void *)result : NULL);
    DT_ARG_PTR(c, 3, maybe_callback(1));
}

static void setup_unsupported(DtCtx *c)
{
    begin(c, c->seed % 5 != 0);
}

static void setup_open(DtCtx *c)
{
    ZfileHandle *file = dt_alloc(sizeof *file);
    char *name = dt_alloc(64);
    uint8_t *block = dt_alloc(64);
    static const char *const kBad[] = {"",  "12345678", "0:0",    "zz:zz",
                                       ":", "1000:0",   "0:1000", "  0x20:  0x10"};
    const uint32_t k = dt_u32() % 8;

    begin(c, c->seed % 11 != 0);
    dt_fill(file, sizeof *file);
    if (k < 5) {
        /* "<address>:<size>" with the address of a real block */
        const uint32_t a = (uint32_t)(uintptr_t)block;
        const char *digits = "0123456789abcdef";
        uint32_t len = 0;

        for (int shift = 28; shift >= 0; shift -= 4) {
            name[len++] = digits[(a >> shift) & 0xf];
        }
        name[len++] = ':';
        name[len++] = digits[dt_u32() & 0xf];
        name[len++] = digits[1 + dt_u32() % 15];
        name[len] = '\0';
    } else {
        const char *bad = kBad[dt_u32() % 8];

        memcpy(name, bad, strlen(bad) + 1);
    }
    DT_ARG_PTR(c, 0, (dt_u32() % 12 == 0) ? NULL : (void *)file);
    DT_ARG_PTR(c, 1, (dt_u32() % 12 == 0) ? NULL : (void *)name);
    DT_ARG_INT(c, 2, (dt_u32() % 6 == 0) ? dt_range(-2, 6) : dt_range(0, 3));
    DT_ARG_PTR(c, 3, maybe_callback(0));
}

/* A file handle: open, closed (data cleared), or empty. */
static ZfileHandle *fixture_file(uint32_t *size, int mode)
{
    const uint32_t k = dt_u32() % 12;
    ZfileHandle *file;

    if (k == 0) {
        file = dt_alloc(sizeof *file);
        file->data = NULL;
        *size = 0;
        return file;
    }
    file = open_file(size, mode);
    if (k == 1 && file->data != NULL) {
        ((ZramdiskFile *)file->data)->data = NULL; /* a closed file */
    }
    return file;
}

static void setup_close(DtCtx *c)
{
    uint32_t size;

    begin(c, c->seed % 9 != 0);
    DT_ARG_PTR(c, 0, fixture_file(&size, 3));
    DT_ARG_PTR(c, 1, maybe_callback(0));
}

static void setup_transfer(DtCtx *c)
{
    uint32_t size;
    ZfileHandle *file;
    ZramdiskFile *rf;

    begin(c, c->seed % 9 != 0);
    file = fixture_file(&size, random_mode());
    rf = file->data;
    if (rf != NULL && (dt_u32() & 1)) {
        rf->pos = (uint32_t)dt_range(0, (int)size); /* somewhere in the block */
    }
    DT_ARG_PTR(c, 0, file);
    DT_ARG_PTR(c, 1, dt_alloc(size + 400));
    DT_ARG_INT(c, 2, (dt_u32() % 6 == 0) ? 0 : dt_range(0, (int)size + 40));
    DT_ARG_PTR(c, 3, maybe_callback(1));
}

static void setup_seek(DtCtx *c)
{
    uint32_t size;
    ZfileHandle *file;
    ZramdiskFile *rf;

    begin(c, c->seed % 9 != 0);
    file = fixture_file(&size, 3);
    rf = file->data;
    if (rf != NULL) {
        rf->pos = (uint32_t)dt_range(0, (int)size);
    }
    DT_ARG_PTR(c, 0, file);
    DT_ARG_INT(c, 1, (dt_u32() % 5 == 0) ? dt_u32() : (uint32_t)dt_range(0, (int)size + 20));
    DT_ARG_INT(c, 2, (dt_u32() % 8 == 0) ? dt_range(3, 5) : dt_range(0, 2));
    DT_ARG_PTR(c, 3, maybe_callback(1));
}

static void setup_tell(DtCtx *c)
{
    uint32_t size;
    ZfileHandle *file;
    ZramdiskFile *rf;

    begin(c, c->seed % 9 != 0);
    file = fixture_file(&size, 3);
    rf = file->data;
    if (rf != NULL) {
        rf->pos = dt_u32();
    }
    DT_ARG_PTR(c, 0, file);
    DT_ARG_PTR(c, 1, dt_alloc(8));
}

static void setup_size(DtCtx *c)
{
    uint32_t size;

    begin(c, c->seed % 9 != 0);
    DT_ARG_PTR(c, 0, fixture_file(&size, 3));
    DT_ARG_PTR(c, 1, (dt_u32() % 8 == 0) ? NULL : dt_alloc(8));
}

static void setup_initialize(DtCtx *c)
{
    begin(c, c->seed & 1); /* already initialised: asserts */
}

/* ---- the renderer side ---------------------------------------------------------------------------- */

static void setup_zrw_matches(DtCtx *c)
{
    static const char *const kPaths[] = {"models\\a.zmd", "x.TXT", "dir.v2\\file.bmp",
                                         "a.b.c",         "p.q",   "noext"};
    static const char *const kExts[] = {"ZMD", "TXT", "BMP", "", "C", "QQ"};
    ZrwRamDiskInfo *info = dt_alloc(sizeof *info);

    begin(c, 1);
    info->address = dt_u32();
    info->size = dt_u32();
    info->extension = (dt_u32() % 4 == 0) ? NULL : kExts[dt_u32() % 6];
    gZrwRamDiskInfo = info;
    DT_ARG_PTR(c, 0, kPaths[dt_u32() % 5]); /* paths with a short extension ("noext" would leave it unset) */
}

static void setup_zrwfopen(DtCtx *c)
{
    ZrwRamDiskInfo *info = dt_alloc(sizeof *info);
    uint8_t *block = dt_alloc(128);

    dt_real_env(c, 1);
    info->address = (uint32_t)(uintptr_t)block;
    info->size = 128;
    info->extension = NULL;
    gZrwRamDiskInfo = (dt_u32() % 6 == 0) ? gZrwRamDiskNoInfo : info;
    gZrwRamDiskFile = (dt_u32() % 8 == 0) ? (ZfileHandle *)(uintptr_t)dt_u32() : NULL;
    gZrwRamDiskDevice = (dt_u32() % 3 == 0) ? -1 : gZrwRamDiskDevice;
}

/* ---- bounding boxes -------------------------------------------------------------------------------- */

static float box_coord(float lo, float hi)
{
    return dt_float_edgy(lo, hi);
}

static void random_box(float *box)
{
    for (int i = 0; i < 3; i++) {
        box[i] = box_coord(-10.0f, 10.0f);
        box[4 + i] = box[i] + dt_float_edgy(0.0f, 10.0f);
    }
    box[3] = dt_float(-1.0f, 1.0f);
    box[7] = dt_float(-1.0f, 1.0f);
    if (dt_u32() % 40 == 0) {
        box[dt_range(0, 6)] = (dt_u32() & 1) ? __builtin_nanf("") : __builtin_inff();
    }
}

static void random_point(float *p, const float *box)
{
    for (int i = 0; i < 3; i++) {
        switch (dt_u32() % 6) {
        case 0:
            p[i] = box[i]; /* on the faces */
            break;
        case 1:
            p[i] = box[4 + i];
            break;
        case 2:
            p[i] = box_coord(-12.0f, 22.0f);
            break;
        default:
            p[i] = box[i] + (box[4 + i] - box[i]) * dt_float(0.0f, 1.0f);
            break;
        }
    }
    if (dt_u32() % 50 == 0) {
        p[dt_range(0, 2)] = __builtin_nanf("");
    }
}

static void setup_bbox_point(DtCtx *c)
{
    float *box = dt_alloc(32);
    float *p = dt_alloc(12);

    ngBegin(c, 0);
    random_box(box);
    random_point(p, box);
    DT_ARG_PTR(c, 0, box);
    DT_ARG_PTR(c, 1, p);
}

static void BMX_CDECL stubPlaneFromFace(float *plane, const float *point, const float *dir)
{
    ngLogPtr(plane);
    ngLogVec((const ZVec3 *)point);
    for (int i = 0; i < 4; i++) {
        ngLogFloat(dir[i]);
    }
    for (int i = 0; i < 5; i++) {
        plane[i] = dir[i % 4] + point[i % 3];
    }
}

static int gClipAnswers;

static int BMX_CDECL stubClipSegment(float *a, float *b, const float *plane)
{
    const int pass = (gClipAnswers >> 0) & 1;

    gClipAnswers >>= 1;
    ngLogPtr(a);
    ngLogPtr(b);
    ngLogVec((const ZVec3 *)a);
    ngLogVec((const ZVec3 *)b);
    ngLogFloat(plane[0]);
    a[0] += 0.25f;
    b[2] -= 0.5f;
    return pass;
}

static void setup_bbox_clip(DtCtx *c)
{
    float *box = dt_alloc(32);
    float *a = dt_alloc(12);
    float *b = dt_alloc(12);

    ngBegin(c, 0);
    random_box(box);
    random_point(a, box);
    random_point(b, box);
    gClipAnswers = (dt_u32() % 3 == 0) ? 0x3f : (int)dt_u32();
    if (dt_u32() % 2 == 0) {
        gClipAnswers |= 0x3f; /* all planes pass */
    }
    dt_stub(0x004956f0, NULL, (void *)stubPlaneFromFace);
    dt_stub(0x00495730, NULL, (void *)stubClipSegment);
    DT_ARG_PTR(c, 0, a);
    DT_ARG_PTR(c, 1, b);
    DT_ARG_PTR(c, 2, box);
}

DT_CASES(zramdisk_cases) = {
    {"zramdiskGetName", 0x0049eaa0, zramdiskGetName, 1, DT_RET_INT, setup_name, 40, 0},
    {"zramdiskControl", 0x0049ead0, zramdiskControl, 4, DT_RET_INT, setup_control, 300, 0},
    {"zramdiskSlot08", 0x0049eb80, zramdiskSlot08, 3, DT_RET_INT, setup_slot, 300, 0},
    {"zramdiskSlot0c", 0x0049ec20, zramdiskSlot0c, 0, DT_RET_INT, setup_unsupported, 40, 0},
    {"zramdiskSlot10", 0x0049ec60, zramdiskSlot10, 3, DT_RET_INT, setup_slot, 300, 0},
    {"zramdiskSlot14", 0x0049ed00, zramdiskSlot14, 4, DT_RET_INT, setup_slot4, 300, 0},
    {"zramdiskSlot18", 0x0049eda0, zramdiskSlot18, 4, DT_RET_INT, setup_slot4, 300, 0},
    {"zramdiskSlot1c", 0x0049ee40, zramdiskSlot1c, 0, DT_RET_INT, setup_unsupported, 40, 0},
    {"zramdiskOpen", 0x0049ee80, zramdiskOpen, 4, DT_RET_INT, setup_open, 1000, 0},
    {"zramdiskClose", 0x0049efd0, zramdiskClose, 2, DT_RET_INT, setup_close, 400, 0},
    {"zramdiskRead", 0x0049f080, zramdiskRead, 4, DT_RET_INT, setup_transfer, 1500, 0},
    {"zramdiskWrite", 0x0049f170, zramdiskWrite, 4, DT_RET_INT, setup_transfer, 1500, 0},
    {"zramdiskSeek", 0x0049f260, zramdiskSeek, 4, DT_RET_INT, setup_seek, 1500, 0},
    {"zramdiskTell", 0x0049f370, zramdiskTell, 2, DT_RET_INT, setup_tell, 300, 0},
    {"zramdiskGetSize", 0x0049f3d0, zramdiskGetSize, 2, DT_RET_INT, setup_size, 300, 0},
    {"zramdiskInitialize", 0x0049f450, zramdiskInitialize, 0, DT_RET_INT, setup_initialize, 20, 0},
    {"zrwRamDiskMatches", 0x004a3200, zrwRamDiskMatches, 1, DT_RET_INT, setup_zrw_matches, 200, 0},
    {"zrwfopen", 0x004a3260, zrwfopen, 0, DT_RET_INT, setup_zrwfopen, 300, 0},
    {"zrwBBoxContainsPoint", 0x004a2f50, zrwBBoxContainsPoint, 2, DT_RET_INT, setup_bbox_point, 2000, 0},
    {"zrwBBoxClipSegment", 0x004a3010, zrwBBoxClipSegment, 3, DT_RET_INT, setup_bbox_clip, 2000, 0},
};
