/*
 * Differential tests of memcard.c on a real file: the exe's own memory manager and file system are set up in
 * each setup() and the harness keeps the written file in memory (and compares it). Read cases create the
 * file with the original writer first.
 */
#include "bmx/com/action/comdata.h"
#include "bmx/com/action/memcard.h"
#include "bmx/com/action/overlay.h"
#include "compat/crt.h"
#include "dt.h"
#include "dt_gameplay.h"
#include "dt_zmem.h"
#include "zge/zfile.h"

/* The three blocks of game state the save file holds. */
typedef uint8_t SaveBlockOptions[0xc3c];
typedef uint8_t SaveBlockProfiles[0x98];
typedef uint8_t SaveBlockRecords[0x1680];
#define gBlockOptions  ORIG_VAR(0x006955e0, SaveBlockOptions, gBlockOptions)
#define gBlockProfiles ORIG_VAR(0x006952a0, SaveBlockProfiles, gBlockProfiles)
#define gBlockRecords  ORIG_VAR(0x00551fd8, SaveBlockRecords, gBlockRecords)

#define ORIG_SAVE_WRITE DT_ORIG(0x00412340, int (*)(void))

static void randomState(void)
{
    dt_fill(&gBlockOptions, sizeof gBlockOptions);
    dt_fill(&gBlockProfiles, sizeof gBlockProfiles);
    dt_fill(&gBlockRecords, sizeof gBlockRecords);
}

/* The exe's memory manager (CRT heap, small static region) and file system, as the game starts them. */
static void startFileSystem(uint32_t staticSize)
{
    dt_zmem_env();
    memset(&gZmem, 0, 0x24); /* zmemInitialize also hooks the renderer, which is not running */
    DT_ORIG(0x00485c80, int (*)(uint32_t))(staticSize);
    /* The renderer's file interface (CRT fopen and friends) lives in the engine instance. */
    *(void **)(uintptr_t)0x0067696c = dt_alloc(0x400);
    DT_ORIG(0x004e82e0, int (*)(void))();
    DT_ORIG(0x00483aa0, int (*)(void))();
}

/* Creates the save file with `size` bytes of `data` through the original file layer. */
static void createFile(const void *data, int size)
{
    void *file = DT_ORIG(0x00484700, void *(*)(const char *, int, int))(".\\save\\SAVE.DAT", 2, 0);

    if (file != NULL) {
        DT_ORIG(0x00484b90, int (*)(void *, const void *, int, int))(file, data, size, 0);
        DT_ORIG(0x00484a60, int (*)(void *, int))(file, 0);
    }
}

static void createValidFile(void)
{
    randomState();
    ORIG_SAVE_WRITE();
    randomState();
}

static void setupNone(DtCtx *c)
{
    dtgpBegin(c, 0);
}

static void setupClose(DtCtx *c)
{
    dtgpBegin(c, DTGP_DEBUG);
    startFileSystem(0x40000);
    if ((dt_u32() & 1) != 0) {
        createValidFile();
    }
}

static void setupRead(DtCtx *c)
{
    dtgpBegin(c, DTGP_DEBUG);
    startFileSystem((dt_u32() & 15) == 0 ? 0x100 : 0x40000);
    switch (dt_range(0, 5)) {
    case 0:
        break;
    case 1:
        createValidFile();
        break;
    case 2:
    case 3: {
        uint8_t *buf = dt_alloc(0x2400);
        dt_fill(buf, 0x2400);
        if (dt_u32() & 1) {
            *(uint32_t *)(buf + 4) = 0x235c; /* the magic is not checked */
        } else if (*(uint32_t *)(buf + 4) == 0x235c) {
            *(uint32_t *)(buf + 4) = 0x235d;
        }
        createFile(buf, 0x235c);
        break;
    }
    case 4: {
        uint8_t *buf = dt_alloc(0x2400);
        dt_fill(buf, 0x2400);
        *(uint32_t *)(buf + 4) = 0x235c;
        createFile(buf, dt_range(0, 0x235b));
        break;
    }
    default: {
        uint8_t *buf = dt_alloc(0x2400);
        dt_fill(buf, 0x2400);
        *(uint32_t *)(buf + 4) = 0x235c;
        createFile(buf, 0x2400);
        break;
    }
    }
    randomState();
}

static void setupWrite(DtCtx *c)
{
    dtgpBegin(c, DTGP_DEBUG);
    startFileSystem((dt_u32() & 15) == 0 ? 0x100 : 0x40000);
    if ((dt_u32() & 3) == 0) {
        uint8_t *buf = dt_alloc(0x2400);
        dt_fill(buf, 0x2400);
        createFile(buf, 0x2400);
    }
    randomState();
}

DT_CASES(memcard_cases) = {
    {"saveFileClose", 0x00412130, saveFileClose, 0, DT_RET_INT, setupClose, 100, 0},
    {"memcardFormat", 0x00412170, memcardFormat, 0, DT_RET_INT, setupNone, 1, 0},
    {"saveFileRead", 0x00412190, saveFileRead, 0, DT_RET_INT, setupRead, 300, 0},
    {"saveFileWrite", 0x00412340, saveFileWrite, 0, DT_RET_INT, setupWrite, 300, 0},
};
