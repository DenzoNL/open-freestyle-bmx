/*
 * Tests for the IFF chunk reader (ziff.c) and the animation / skeleton helpers that live next to it.
 *
 * The readers run on the real model files of the install (every Assets\*.ZMD, the level scene files) and on
 * files made up in memory (served by the RAM disk device): trees of chunks with random shapes, and
 * damaged ones (bad ids, sizes that are not a multiple of four, cut off). Each case starts from a state the
 * original code walked into with a random sequence of find / next / enter / leave / seek / read calls.
 */
#include "dt.h"
#include "dt_ngif.h"
#include "dt_realfile.h"
#include "zge/com/zglrw/zrwmat.h"
#include "zge/ziff.h"

typedef Ziff *(BMX_CDECL *OpenFn)(void *, uint32_t, uint32_t);
typedef void(BMX_CDECL *CloseFn)(void *);
typedef int(BMX_CDECL *FindFn)(void *, uint32_t);
typedef int(BMX_CDECL *ReadFn)(void *, void *, uint32_t);
typedef int(BMX_CDECL *SeekFn)(void *, int, int);
typedef int(BMX_CDECL *BeginFn)(void *);
typedef int(BMX_CDECL *ProcessFn)(void *, void *);
typedef void *(BMX_CDECL *CallbackFn)(uint32_t, int(BMX_CDECL *)(void *), int);
typedef void(BMX_CDECL *InitFn)(void);
typedef ZfileHandle *(BMX_CDECL *FileOpenFn)(const char *, int, int);

#define O_OPEN     DT_ORIG(0x0049a790, OpenFn)
#define O_FIND     DT_ORIG(0x0049ad90, FindFn)
#define O_NEXT     DT_ORIG(0x0049aed0, FindFn)
#define O_READ     DT_ORIG(0x0049ac30, ReadFn)
#define O_SEEK     DT_ORIG(0x0049b060, SeekFn)
#define O_BEGIN    DT_ORIG(0x0049b130, BeginFn)
#define O_END      DT_ORIG(0x0049b250, BeginFn)
#define O_CALLBACK DT_ORIG(0x0049aad0, CallbackFn)
#define O_INIT     DT_ORIG(0x0049a6d0, InitFn)
#define O_FILEOPEN DT_ORIG(0x00484700, FileOpenFn)

enum {
    kMODL = 0x4c444f4d,
    kMLOD = 0x444f4c4d,
    kMLDF = 0x46444c4d,
    kMDFF = 0x4646444d,
    kMDEF = 0x4645444d,
    kMATI = 0x4954414d,
    kSCEN = 0x4e454353,
    kJUNK = 0x4b4e554a,
    kAny = -1,
};

static const char *const kZmd[] = {
    "ABBike01.ZMD",
    "AmishBoy1.ZMD",
    "AmishBoy2.ZMD",
    "AmishBoy3.ZMD",
    "AmishBoy4.ZMD",
    "AmishBoy5.ZMD",
    "Amishbike01.ZMD",
    "CKbike01.ZMD",
    "CKbike02.ZMD",
    "CKbike03.ZMD",
    "CKbike04.ZMD",
    "ChadKagy1.ZMD",
    "ChadKagy2.zmd",
    "ChadKagy3.zmd",
    "ChadKagy4.zmd",
    "ChadKagy5.zmd",
    "DMbike01.zmd",
    "DMbike02.zmd",
    "DMbike03.zmd",
    "DMbike04.zmd",
    "DaveMirra.ZMD",
    "DaveMirra1.ZMD",
    "DaveMirra2.zmd",
    "DaveMirra3.zmd",
    "DaveMirra4.zmd",
    "DaveMirra5.zmd",
    "DaveMirra_shadow.zmd",
    "JGbike01.ZMD",
    "JGbike02.ZMD",
    "JGbike03.ZMD",
    "JGbike04.ZMD",
    "JoeyGarcia1.ZMD",
    "JoeyGarcia2.ZMD",
    "JoeyGarcia3.ZMD",
    "JoeyGarcia4.ZMD",
    "JoeyGarcia5.ZMD",
    "KHbike01.ZMD",
    "KHbike02.ZMD",
    "KHbike03.ZMD",
    "KHbike04.ZMD",
    "KenanHarkin1.ZMD",
    "KenanHarkin2.ZMD",
    "KenanHarkin3.ZMD",
    "KenanHarkin4.ZMD",
    "KenanHarkin5.ZMD",
    "LRbike01.ZMD",
    "LRbike02.ZMD",
    "LRbike03.ZMD",
    "LRbike04.ZMD",
    "LeighRamsdell1.ZMD",
    "LeighRamsdell2.ZMD",
    "LeighRamsdell3.ZMD",
    "LeighRamsdell4.ZMD",
    "LeighRamsdell5.ZMD",
    "MLbike01.ZMD",
    "MLbike02.ZMD",
    "MLbike03.ZMD",
    "MLbike04.ZMD",
    "MikeLaird1.ZMD",
    "MikeLaird2.ZMD",
    "MikeLaird3.ZMD",
    "MikeLaird4.ZMD",
    "MikeLaird5.ZMD",
    "RNbike01.ZMD",
    "RNbike02.ZMD",
    "RNbike03.ZMD",
    "RNbike04.ZMD",
    "RyanNyquist1.ZMD",
    "RyanNyquist2.zmd",
    "RyanNyquist3.zmd",
    "RyanNyquist4.zmd",
    "RyanNyquist5.zmd",
    "SBbike01.ZMD",
    "SBbike02.ZMD",
    "SBbike03.ZMD",
    "SBbike04.ZMD",
    "SJbike01.ZMD",
    "ShadowVolume.ZMD",
    "ShaunButler1.ZMD",
    "ShaunButler2.ZMD",
    "ShaunButler3.ZMD",
    "ShaunButler4.ZMD",
    "ShaunButler5.ZMD",
    "SlimJimGuy1.ZMD",
    "SlimJimGuy2.ZMD",
    "SlimJimGuy3.ZMD",
    "SlimJimGuy4.ZMD",
    "SlimJimGuy5.ZMD",
    "TMbike02.ZMD",
    "TMcbike01.ZMD",
    "TMcbike02.ZMD",
    "TMcbike03.ZMD",
    "TMcbike04.ZMD",
    "TMibike01.ZMD",
    "TMibike02.ZMD",
    "TMibike03.ZMD",
    "TMibike04.ZMD",
    "TimMirra1.ZMD",
    "TimMirra2.ZMD",
    "TimMirra3.ZMD",
    "TimMirra4.ZMD",
    "TimMirra5.ZMD",
    "TroyMcMurray1.ZMD",
    "TroyMcMurray1v1.ZMD",
    "TroyMcMurray2.ZMD",
    "TroyMcMurray3.ZMD",
    "TroyMcMurray4.ZMD",
    "TroyMcMurray5.ZMD",
    "XXBike01.ZMD",
    "bike.zmd",
    "bike_hires.zmd",
    "bike_shadow.zmd",
    "rider_shadow.zmd",
    "secretbike01.ZMD",
    "sfx.zmd",
};
enum { kZmdCount = sizeof kZmd / sizeof kZmd[0] };

static const struct {
    const char *path;
    uint32_t form;
} kOthers[] = {
    {"Level_-_AD\\AcclaimD.ZSD", kSCEN},     {"Level_-_AS\\AcclaimS.ZSD", kSCEN},
    {"Level_-_GD\\GrenDirt.ZSD", kSCEN},     {"Level_-_GV\\GrenVert.ZSD", kSCEN},
    {"Level_-_PS\\ProTourS.ZSD", kSCEN},     {"Level_-_WS\\WoodStreet.ZSD", kSCEN},
    {"Level_-_WV\\WoodVert.ZSD", kSCEN},     {"Level_-_AD\\Mat._sd", kMATI},
    {"Level_-_AD\\Prop._sd", 0x4c425250},    /* PRBL */
    {"Level_-_AD\\PropDef._sd", 0x4c424450}, /* PDBL */
    {"Level_-_AD\\Scene._sd", 0x54504353},   /* SCPT */
    {"Level_-_AD\\Tag._sd", 0x42474154},     /* TAGB */
};
enum {
    kOtherCount = sizeof kOthers / sizeof kOthers[0],
    kSceneCount = 7, /* the first entries are real nested files (SCEN); the others are plain chunk lists */
    kRealCount = kZmdCount + kSceneCount,
    kOpenCount = kZmdCount + kOtherCount
};

typedef struct Fixture {
    ZfileHandle *file;
    Ziff *iff;
    uint32_t form;
    int depth;
    int fresh;   /* just entered a container: the chunk fields are 8 bytes off until the next find */
    int damaged; /* a made-up file with broken chunk sizes: only open it */
} Fixture;

static uint32_t gSkeletonStubCount;

static const uint32_t kIdPool[] = {kMLOD, kMLDF, kMDFF, kMDEF, kMATI, kMODL, kSCEN, kJUNK, 0xffffffffu};

static uint32_t random_id(void)
{
    return kIdPool[dt_u32() % (sizeof kIdPool / sizeof kIdPool[0])];
}

/* ---- made up files ------------------------------------------------------------------------------ */

typedef struct MadeSpec {
    int depth;  /* nesting below the form */
    int damage; /* 0 none, 1 odd chunk size, 2 child bigger than its parent, 3 cut off, 4 wrong form id */
} MadeSpec;

/* A word of capital letters: a misparse of the data then still yields printable ids (assert messages). */
static uint32_t ascii_word(void)
{
    uint32_t w = 0;

    for (int i = 0; i < 4; i++) {
        w = w << 8 | (uint32_t)('A' + dt_u32() % 26);
    }
    return w;
}

/* Containers and plain data chunks use different ids, so the walker knows what it may enter. */
static const uint32_t kContainerIds[] = {kMLOD, kMODL, kSCEN};
static const uint32_t kDataIds[] = {kMLDF, kMDFF, kMDEF, kMATI, kJUNK};

static int is_container(uint32_t id)
{
    return id == kMLOD || id == kMODL || id == kSCEN;
}

/* Appends a chunk of random children (or data) at *pos; fills sizes in; ids come from the pool. */
static uint32_t put_chunk(uint8_t *img, uint32_t pos, uint32_t id, int depth, int damage)
{
    const uint32_t start = pos;
    uint32_t size = 0;

    dt_wr32(img, pos, id);
    pos += 8;
    if (depth > 0) {
        for (int i = dt_range(1, 3); i > 0; i--) {
            const int sub = (dt_u32() & 1) ? depth - 1 : 0;
            const uint32_t child = sub > 0 ? kContainerIds[dt_u32() % 3] : kDataIds[dt_u32() % 5];

            pos = put_chunk(img, pos, child, sub, damage);
        }
    } else {
        const uint32_t words = (uint32_t)dt_range(0, 12);

        for (uint32_t w = 0; w < words; w++) {
            dt_wr32(img, pos + w * 4, ascii_word());
        }
        pos += words * 4;
    }
    size = pos - start - 8;
    if (damage == 1 && (dt_u32() % 4) == 0) {
        size += (uint32_t)dt_range(1, 3);
    }
    if (damage == 2 && (dt_u32() % 4) == 0) {
        size += (uint32_t)dt_range(4, 64) & ~3u;
    }
    dt_wr32(img, start + 4, size);
    return pos;
}

static uint8_t *make_iff(const MadeSpec *spec, uint32_t *size, uint32_t *form)
{
    uint8_t *img = dt_alloc(4096);
    uint32_t end;

    for (int i = 0; i < 4096; i += 4) {
        dt_wr32(img, (uint32_t)i, ascii_word());
    }
    *form = kMODL;
    end = put_chunk(img, 0, *form, spec->depth, spec->damage);
    *size = end;
    if (spec->damage == 3) {
        *size = (uint32_t)dt_range(1, (int)end);
    }
    if (spec->damage == 4) {
        dt_wr32(img, 0, kJUNK);
    }
    return img;
}

static MadeSpec random_made(void)
{
    MadeSpec s;

    s.depth = dt_range(1, 3); /* the root holds chunks, not data */
    s.damage = (dt_u32() % 3 == 0) ? dt_range(1, 4) : 0;
    return s;
}

/* ---- fixtures ------------------------------------------------------------------------------------ */

static int is_real(uint32_t seed)
{
    return seed % 4 != 3;
}

/* File n of the nested ones (n < kRealCount), or of all (when `all`: for the open case only). */
static void path_of(uint32_t n, const char **path, uint32_t *form, int all)
{
    static char buf[64];

    n %= all ? kOpenCount : kRealCount;
    if (n < kZmdCount) {
        memcpy(buf, "Assets\\", 8); /* with the terminator, overwritten below */
        memcpy(buf + 7, kZmd[n], strlen(kZmd[n]) + 1);
        *path = buf;
        *form = kMODL;
    } else {
        *path = kOthers[n - kZmdCount].path;
        *form = kOthers[n - kZmdCount].form;
    }
}

/*
 * Opens a real file (the iteration number walks through all of them) or a made-up one with the original
 * code. A damaged made-up file (odd chunk sizes, children bigger than their parent, cut off) is only
 * opened, never walked: the original asserts on the first damaged chunk it meets, and an assert in the
 * setup would cut it short.
 */
static void open_fixture(Fixture *fx, DtCtx *c)
{
    const char *path;
    uint32_t form, size;

    fx->depth = 0;
    fx->fresh = 1; /* ziffOpen leaves the chunk fields 8 bytes off as well, like entering a container */
    fx->damaged = 0;
    if (is_real(c->seed)) {
        path_of(c->seed / 4 * 3 + c->seed % 4, &path, &form, 0);
        fx->file = O_FILEOPEN(path, 1, 0);
    } else {
        MadeSpec s = random_made();
        uint8_t *img;

        if (s.damage == 4) {
            s.damage = 0; /* a wrong form id is the business of the ziffOpen case */
        }
        img = make_iff(&s, &size, &form);
        if (s.damage == 3 && size < 8) {
            size = 8;
        }
        fx->damaged = s.damage != 0;
        fx->file = O_FILEOPEN(dt_ramdisk_name(img, size), 1, 0);
    }
    fx->form = form;
    fx->iff = fx->file != NULL ? O_OPEN(fx->file, form, 0) : NULL;
}

static uint32_t bytes_left(const Ziff *iff)
{
    return (uint32_t)(iff->frame->chunkStart - iff->frame->pos) + 8 + iff->frame->chunkSize;
}

/* One random call of the original reader that cannot assert in a well formed file. */
static void walk_step(Fixture *fx)
{
    Ziff *iff = fx->iff;
    const uint32_t op = dt_u32() % 7;

    switch (op) {
    case 0:
        O_FIND(iff, random_id());
        fx->fresh = 0;
        break;
    case 1:
        if (iff->frame->pos != -1 && !fx->fresh) {
            O_NEXT(iff, (dt_u32() & 1) ? 0xffffffffu : random_id());
        }
        break;
    case 2:
        if (fx->depth < 3 && !fx->fresh && iff->frame->pos == iff->frame->chunkStart + 8 &&
            is_container(iff->frame->chunkId) && iff->frame->chunkSize >= 8) {
            O_BEGIN(iff);
            fx->depth++;
            fx->fresh = 1;
        }
        break;
    case 3:
        if (fx->depth > 0) {
            O_END(iff);
            fx->depth--;
            fx->fresh = 0;
        }
        break;
    case 4: {
        const int span = iff->frame->end - iff->frame->start - 8;

        if (span > 0) {
            O_SEEK(iff, 0, dt_range(0, span));
        }
        break;
    }
    case 5: {
        const uint32_t left = bytes_left(iff);

        if (iff->frame->pos != -1 && !fx->fresh && left > 0 && left < 0x10000) {
            uint8_t buf[256];

            O_READ(iff, buf, (uint32_t)dt_range(1, (int)(left > 256 ? 256 : left)));
        }
        break;
    }
    default:
        break;
    }
}

static Fixture begin_fixture(DtCtx *c, int steps)
{
    Fixture fx;

    dt_real_env(c, 1);
    O_INIT();
    open_fixture(&fx, c);
    for (int i = 0; fx.iff != NULL && !fx.damaged && i < steps; i++) {
        walk_step(&fx);
    }
    return fx;
}

/* ---- callbacks the process / dispatch cases register ---------------------------------------------- */

static int BMX_CDECL chunk_handler(void *iffp, void *arg)
{
    const Ziff *iff = iffp;

    ngLog(0x4a0000);
    ngLogPtr(arg);
    ngLog(iff->frame->chunkId);
    ngLog(iff->frame->chunkSize);
    ngLog((uint32_t)iff->frame->pos);
    return iff->frame->chunkId != kMDEF;
}

static int BMX_CDECL chunk_handler_b(void *iffp)
{
    const Ziff *iff = iffp;

    ngLog(0x4b0000);
    ngLog(iff->frame->chunkId);
    return 1;
}

/* Every id a chunk of the test files can have (most of the time all of them are registered). */
static const uint32_t kRegistrable[] = {
    kMLOD,     kMLDF, kMDFF, kMDEF, kMATI, kMODL, kSCEN, kJUNK, 0x46444353 /* SCDF */, 0x49464353 /* SCFI */,
    0x54504353 /* SCPT */};

static void register_handlers(void)
{
    const int all = dt_u32() % 4 != 0;

    for (uint32_t i = 0; i < sizeof kRegistrable / sizeof kRegistrable[0]; i++) {
        if (all || dt_u32() % 4 != 0) {
            O_CALLBACK(kRegistrable[i],
                       (dt_u32() & 1) ? (int(BMX_CDECL *)(void *))(uintptr_t)chunk_handler : chunk_handler_b,
                       0);
        }
    }
}

/* ---- setups --------------------------------------------------------------------------------------- */

static void setup_initialize(DtCtx *c)
{
    dt_real_env(c, 0);
    if (c->seed & 1) {
        O_INIT(); /* already initialised: asserts */
    }
}

static void setup_setup_callback(DtCtx *c)
{
    dt_real_env(c, 0);
    if (c->seed % 8 != 0) {
        O_INIT();
    }
    if (c->seed % 3 == 0) {
        O_CALLBACK(kMLOD, (int(BMX_CDECL *)(void *))(uintptr_t)chunk_handler, 0);
    }
    DT_ARG_INT(c, 0, (c->seed % 2) ? kMLOD : random_id());
    DT_ARG_PTR(c, 1, (dt_u32() & 1) ? (void *)chunk_handler : (void *)chunk_handler_b);
    DT_ARG_INT(c, 2, dt_u32());
}

static void setup_open(DtCtx *c)
{
    const char *path;
    uint32_t form, size;

    dt_real_env(c, 1);
    O_INIT();
    if (is_real(c->seed)) {
        path_of(c->seed / 4 * 3 + c->seed % 4, &path, &form, 1);
        DT_ARG_PTR(c, 0, O_FILEOPEN(path, 1, 0));
        DT_ARG_INT(c, 1, (dt_u32() % 4 == 0) ? random_id() : form);
        DT_ARG_INT(c, 2, (dt_u32() & 1) ? 0 : dt_u32() % 100000);
    } else {
        MadeSpec s = random_made();
        uint8_t *img = make_iff(&s, &size, &form);

        DT_ARG_PTR(c, 0, O_FILEOPEN(dt_ramdisk_name(img, size), 1, 0));
        DT_ARG_INT(c, 1, (dt_u32() % 6 == 0) ? random_id() : form);
        DT_ARG_INT(c, 2, (dt_u32() & 1) ? 0 : size);
    }
}

static void setup_close(DtCtx *c)
{
    Fixture fx = begin_fixture(c, dt_range(0, 3));

    DT_ARG_PTR(c, 0, fx.iff);
}

static void setup_iff_only(DtCtx *c)
{
    Fixture fx = begin_fixture(c, dt_range(0, 8));

    DT_ARG_PTR(c, 0, fx.iff);
}

static void setup_find(DtCtx *c)
{
    Fixture fx = begin_fixture(c, dt_range(0, 8));

    DT_ARG_PTR(c, 0, fx.iff);
    DT_ARG_INT(c, 1, random_id());
}

static void setup_next(DtCtx *c)
{
    Fixture fx = begin_fixture(c, dt_range(0, 8));

    if (fx.iff != NULL && !fx.damaged) {
        O_FIND(fx.iff, 0xffffffffu); /* a current chunk to step on from */
    }
    DT_ARG_PTR(c, 0, fx.iff);
    DT_ARG_INT(c, 1, (dt_u32() & 1) ? 0xffffffffu : random_id());
}

static void setup_read(DtCtx *c)
{
    Fixture fx = begin_fixture(c, dt_range(0, 8));
    uint32_t left;

    DT_ARG_PTR(c, 0, fx.iff);
    DT_ARG_PTR(c, 1, dt_alloc(0x12000));
    left = fx.iff != NULL ? bytes_left(fx.iff) : 0;
    DT_ARG_INT(c, 2, (dt_u32() % 5 == 0) ? 0 : (uint32_t)dt_range(1, 0x2000));
    if (dt_u32() % 3 == 0 && left < 0x10000) {
        DT_ARG_INT(c, 2, left); /* exactly what is left, then more than that */
    }
}

static void setup_read_chunk(DtCtx *c)
{
    Fixture fx = begin_fixture(c, dt_range(0, 3));

    DT_ARG_PTR(c, 0, fx.iff);
    DT_ARG_PTR(c, 1, dt_alloc(0x40000));
    if (fx.iff != NULL && !fx.damaged && (dt_u32() & 1)) {
        O_FIND(fx.iff, 0xffffffffu); /* at the start of a chunk: the allowed case */
        if (fx.iff->frame->chunkSize > 0x3ff00) {
            O_SEEK(fx.iff, 0, 0);
        }
    }
}

static void setup_word(DtCtx *c)
{
    Fixture fx = begin_fixture(c, dt_range(0, 8));

    DT_ARG_PTR(c, 0, fx.iff);
    DT_ARG_PTR(c, 1, dt_alloc(16));
}

static void setup_seek(DtCtx *c)
{
    Fixture fx = begin_fixture(c, dt_range(0, 8));

    DT_ARG_PTR(c, 0, fx.iff);
    DT_ARG_INT(c, 1, (dt_u32() % 8 == 0) ? dt_range(2, 4) : dt_range(0, 1));
    DT_ARG_INT(c, 2, (dt_u32() % 4 == 0) ? dt_range(-40, 40) : dt_range(0, 300));
}

static void setup_begin(DtCtx *c)
{
    Fixture fx = begin_fixture(c, dt_range(0, 6));

    if (fx.iff != NULL && !fx.damaged && (dt_u32() & 1)) {
        O_FIND(fx.iff, kMLOD);
    }
    DT_ARG_PTR(c, 0, fx.iff);
}

static void setup_end(DtCtx *c)
{
    Fixture fx = begin_fixture(c, dt_range(0, 8));

    if (fx.iff != NULL && !fx.damaged && fx.depth == 0 && (dt_u32() & 1) &&
        fx.iff->frame->pos == fx.iff->frame->chunkStart + 8) {
        O_BEGIN(fx.iff);
    }
    DT_ARG_PTR(c, 0, fx.iff);
}

static void setup_process(DtCtx *c)
{
    Fixture fx = begin_fixture(c, dt_range(0, 4));

    register_handlers();
    DT_ARG_PTR(c, 0, fx.iff);
    DT_ARG_PTR(c, 1, (dt_u32() & 1) ? (void *)dt_alloc(8) : NULL);
}

static void setup_dispatch(DtCtx *c)
{
    Fixture fx = begin_fixture(c, dt_range(0, 6));

    register_handlers();
    if (fx.iff != NULL && !fx.damaged && (dt_u32() & 1)) {
        O_FIND(fx.iff, 0xffffffffu);
    }
    DT_ARG_PTR(c, 0, fx.iff);
    DT_ARG_PTR(c, 1, dt_alloc(8));
}

static void setup_read_anim(DtCtx *c)
{
    Fixture fx = begin_fixture(c, 0);

    register_handlers();
    if (fx.iff != NULL && !fx.damaged) {
        O_FIND(fx.iff, (dt_u32() & 3) ? kMLOD : 0xffffffffu);
    }
    DT_ARG_PTR(c, 0, fx.iff);
}

/* An animation record whose four chains hold 0..3 nodes each. */
static void *make_anim(void)
{
    static const struct {
        unsigned head, data, next;
    } chains[4] = {{0xc, 8, 0xc}, {8, 8, 0xc}, {0x10, 8, 0xc}, {4, 4, 8}};
    uint8_t *anim = dt_zmemalloc(0x14);

    memset(anim, 0, 0x14);
    *(uint32_t *)anim = dt_u32();
    for (int i = 0; i < 4; i++) {
        uint8_t *prev = NULL;

        for (int n = dt_range(0, 3); n > 0; n--) {
            uint8_t *node = dt_zmemalloc(0x20);

            memset(node, 0, 0x20);
            *(void **)(node + chains[i].data) = dt_zmemalloc((uint32_t)dt_range(1, 40));
            *(uint8_t **)(node + chains[i].next) = prev;
            prev = node;
        }
        *(uint8_t **)(anim + chains[i].head) = prev;
    }
    return anim;
}

static void setup_free_anim(DtCtx *c)
{
    dt_real_env(c, 0);
    DT_ARG_PTR(c, 0, make_anim());
}

static void setup_anim_name(DtCtx *c)
{
    uint32_t *anim = dt_alloc(0x14);

    dt_real_env(c, 0);
    dt_fill(anim, 0x14);
    DT_ARG_PTR(c, 0, anim);
}

/* ---- skeletons ------------------------------------------------------------------------------------ */

static ZSkeleton *make_skeleton(uint32_t count)
{
    ZSkeleton *s = dt_alloc(sizeof *s + count * sizeof(ZSkeletonNode));

    dt_fill(s, sizeof *s + count * sizeof(ZSkeletonNode));
    s->count = count;
    s->nodes = s->data;
    for (uint32_t i = 0; i < count; i++) {
        s->data[i].hash = (dt_u32() % 3 == 0) ? 0x1000 + dt_u32() % 8 : dt_u32();
        s->data[i].items = NULL;
    }
    return s;
}

static void setup_find_node(DtCtx *c)
{
    const uint32_t count = (uint32_t)dt_range(0, 8);
    ZSkeleton *s = make_skeleton(count);

    dt_real_env(c, 0);
    DT_ARG_PTR(c, 0, s);
    DT_ARG_INT(c, 1, (count != 0 && (dt_u32() & 1)) ? s->data[dt_u32() % count].hash : 0x1000 + dt_u32() % 8);
}

static void setup_set_rotation(DtCtx *c)
{
    ZSkeletonNode *node = dt_alloc(sizeof *node);
    float *q = dt_alloc(16);

    dt_real_env(c, 0);
    dt_fill(node, sizeof *node);
    for (int i = 0; i < 4; i++) {
        q[i] = dt_float_edgy(-1.0f, 1.0f);
    }
    DT_ARG_PTR(c, 0, node);
    DT_ARG_PTR(c, 1, q);
}

static int BMX_CDECL stubNodeCount(void *object)
{
    ngLogPtr(object);
    return (int)gSkeletonStubCount;
}

static void BMX_CDECL stubNodeHashes(void *object, uint32_t *hashes)
{
    ngLogPtr(object);
    ngLogPtr(hashes);
    for (uint32_t i = 0; i < gSkeletonStubCount; i++) {
        hashes[i] = (dt_u32() % 3 == 0) ? 0x2000 + dt_u32() % 5 : dt_u32();
    }
}

static void BMX_CDECL stubSetSkeleton(void *object, void *skeleton)
{
    ngLogPtr(object);
    ngLogPtr(skeleton);
    ((ZSkeletonOwner *)object)->skeleton = skeleton;
}

static void setup_build_skeleton(DtCtx *c)
{
    ZSkeletonOwner *object = dt_alloc(0x40);

    dt_real_env(c, 0);
    dt_fill(object, 0x40);
    gSkeletonStubCount = (uint32_t)dt_range(0, 12);
    dt_stub(0x004890d0, NULL, (void *)stubNodeCount);
    dt_stub(0x004890f0, NULL, (void *)stubNodeHashes);
    dt_stub(0x00489210, NULL, (void *)stubSetSkeleton);
    DT_ARG_PTR(c, 0, object);
}

static void setup_free_skeleton(DtCtx *c)
{
    ZSkeletonOwner *object;
    uint32_t count;
    ZSkeleton *skel;

    dt_real_env(c, 0);
    object = dt_alloc(0x40);
    count = (uint32_t)dt_range(0, 6);
    skel = dt_zmemalloc(sizeof(ZSkeleton) + count * sizeof(ZSkeletonNode));
    memset(skel, 0, sizeof(ZSkeleton) + count * sizeof(ZSkeletonNode));
    skel->count = count;
    skel->nodes = skel->data;
    for (uint32_t i = 0; i < count; i++) {
        for (int n = dt_range(0, 3); n > 0; n--) {
            ZSkeletonItem *item = dt_alloc(sizeof *item);

            dt_fill(item, sizeof *item);
            item->next = skel->data[i].items;
            skel->data[i].items = item;
        }
    }
    gZiffItemPool = (dt_u32() & 1) ? NULL : dt_alloc(sizeof(ZSkeletonItem));
    dt_fill(object, 0x40);
    object->skeleton = (dt_u32() % 6 == 0) ? NULL : skel;
    DT_ARG_PTR(c, 0, object);
}

DT_CASES(ziff_cases) = {
    {"ziffInitialize", 0x0049a6d0, ziffInitialize, 0, DT_RET_VOID, setup_initialize, 20, 0},
    {"ziffSetupCallback", 0x0049aad0, ziffSetupCallback, 3, DT_RET_INT, setup_setup_callback, 400, 0},
    {"ziffOpen", 0x0049a790, ziffOpen, 3, DT_RET_INT | DT_NEEDS_ASSETS, setup_open, 600, 0},
    {"ziffClose", 0x0049a8a0, ziffClose, 1, DT_RET_VOID | DT_NEEDS_ASSETS, setup_close, 300, 0},
    {"ziffFindChunk", 0x0049ad90, ziffFindChunk, 2, DT_RET_INT | DT_NEEDS_ASSETS, setup_find, 2000, 0},
    {"ziffNextChunk", 0x0049aed0, ziffNextChunk, 2, DT_RET_INT | DT_NEEDS_ASSETS, setup_next, 2000, 0},
    {"ziffReadChunkHeader", 0x0049b020, ziffReadChunkHeader, 1, DT_RET_VOID | DT_NEEDS_ASSETS, setup_iff_only,
     600, 0},
    {"ziffChunkSize", 0x0049abc0, ziffChunkSize, 1, DT_RET_INT | DT_NEEDS_ASSETS, setup_iff_only, 300, 0},
    {"ziffRead", 0x0049ac30, ziffRead, 3, DT_RET_INT | DT_NEEDS_ASSETS, setup_read, 2000, 0},
    {"ziffReadChunk", 0x0049abe0, ziffReadChunk, 2, DT_RET_INT | DT_NEEDS_ASSETS, setup_read_chunk, 800, 0},
    {"ziffReadWordA", 0x0049acf0, ziffReadWordA, 2, DT_RET_VOID | DT_NEEDS_ASSETS, setup_word, 400, 0},
    {"ziffReadWordB", 0x0049ad10, ziffReadWordB, 2, DT_RET_VOID | DT_NEEDS_ASSETS, setup_word, 400, 0},
    {"ziffSeek", 0x0049b060, ziffSeek, 3, DT_RET_INT | DT_NEEDS_ASSETS, setup_seek, 1500, 0},
    {"ziffBeginModel", 0x0049b130, ziffBeginModel, 1, DT_RET_INT | DT_NEEDS_ASSETS, setup_begin, 1500, 0},
    {"ziffEndModel", 0x0049b250, ziffEndModel, 1, DT_RET_INT | DT_NEEDS_ASSETS, setup_end, 1500, 0},
    {"ziffProcessChunks", 0x0049a8e0, ziffProcessChunks, 2, DT_RET_INT | DT_NEEDS_ASSETS, setup_process, 1500,
     0},
    {"ziffDispatchChunk", 0x0049b2a0, ziffDispatchChunk, 2, DT_RET_INT | DT_NEEDS_ASSETS, setup_dispatch,
     1500, 0},
    {"ziffReadAnim", 0x0049a4e0, ziffReadAnim, 1, DT_RET_INT | DT_NEEDS_ASSETS, setup_read_anim, 800, 0},
    {"ziffAnimName", 0x0049a530, ziffAnimName, 1, DT_RET_INT, setup_anim_name, 20, 0},
    {"ziffFreeAnim", 0x0049a630, ziffFreeAnim, 1, DT_RET_VOID, setup_free_anim, 300, 0},
    {"zrwmatFindSkeletonNode", 0x0049a540, zrwmatFindSkeletonNode, 2, DT_RET_INT, setup_find_node, 400, 0},
    {"zrwmatSetNodeRotation", 0x0049a570, zrwmatSetNodeRotation, 2, DT_RET_VOID, setup_set_rotation, 100, 0},
    {"zrwmatBuildSkeleton", 0x004993f0, zrwmatBuildSkeleton, 1, DT_RET_INT, setup_build_skeleton, 400, 0},
    {"zrwmatFreeSkeleton", 0x004994d0, zrwmatFreeSkeleton, 1, DT_RET_VOID, setup_free_skeleton, 300, 0},
};
