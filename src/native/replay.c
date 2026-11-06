/*
 * BMX_RECORD=<file> / BMX_REPLAY=<file>: record a session's input and replay it on another build
 * (docs/PORTING.md X0). Together with BMX_SIM_HASH (simhash.c) this is the cross-build check: the same
 * recording on two builds must give the same per-tick hash.
 *
 * What is recorded is the input as the game receives it from the DirectInput stand-in (src/platform/input.c):
 * the result and state of GetDeviceState, after the d-pad / 8-way conversions, at the shown frame
 * (drvGpuShownFrames: one per tick in a level, as BMX_KEYS counts) and the call's index within that frame.
 * The game's random seed is never taken from the clock (only zmathRandom changes it) and the simulation steps
 * per tick, so the input decides the run.
 *
 * Only changes are stored: some loops poll a device thousands of times per frame, as often as the wall clock
 * lets them, so the number of calls in a frame differs from run to run. A record is the device's new result
 * and state from (frame, index) on; a replay answers every call with the latest record at or before its own
 * (frame, index), per device. A call before any record of its device keeps the stand-in's own answer (no pad
 * in a replay, no keys).
 *
 * File: "BMXR", u32 version (2), the pad the game saw (u8 present, u8 family, u16 vendor, u16 product,
 * char[128] name), u32 size and the bytes of save/SAVE.DAT at the start (the replay warns when the file
 * differs; the replay script puts it in place), then records: u32 frame, u32 index, u8 keyboard, i32 result,
 * u16 size, the state. Little-endian, no padding. `keyboard` 2 is a frame mark (size 0), written about once a
 * second, so that a replay runs as long as the recorded session did (it ends 60 frames after the last
 * record).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "native/native.h"
#include "platform/platform.h"

int drvGpuShownFrames(void);

enum { kVersion = 2, kMaxState = 256, kSaveMax = 1 << 20, kFlushEvery = 64 };

typedef struct Record {
    uint32_t frame, index;
    uint8_t keyboard;
    int32_t result;
    uint16_t size;
    uint8_t data[kMaxState];
} Record;

static int gMode;
static FILE *gFile;
static const char *gPath;
static uint32_t gFrame = 0xffffffffu, gIndex, gWritten;
/* Per device (keyboard 1, pad 0): recording, the last state written; replaying, the state in force. */
static Record gCur[2];
static int gHaveCur[2];
/* Replaying: the next record of the file. */
static Record gNext;
static int gHaveNext, gApplied;

static void put(const void *p, size_t n)
{
    if (gFile != NULL && fwrite(p, 1, n, gFile) != n) {
        bmxLog("replay: cannot write %s\n", gPath);
        fclose(gFile);
        gFile = NULL;
    }
}

static int get(void *p, size_t n)
{
    return gFile != NULL && fread(p, 1, n, gFile) == n;
}

static uint8_t *loadSave(uint32_t *size)
{
    FILE *f = fopen(bmxPathResolve("save\\SAVE.DAT"), "rb"); /* the folder's own case on Linux */
    uint8_t *bytes = malloc(kSaveMax);
    size_t n = 0;

    if (f != NULL && bytes != NULL) {
        n = fread(bytes, 1, kSaveMax, f);
    }
    if (f != NULL) {
        fclose(f);
    }
    *size = (uint32_t)n;
    return bytes;
}

static void readNext(void)
{
    gHaveNext = get(&gNext.frame, 4) && get(&gNext.index, 4) && get(&gNext.keyboard, 1) &&
                get(&gNext.result, 4) && get(&gNext.size, 2) && gNext.size <= kMaxState &&
                gNext.keyboard <= 2 && get(gNext.data, gNext.size);
}

int bmxReplayMode(void)
{
    static int checked;

    if (!checked) {
        const char *rec = getenv("BMX_RECORD"), *rep = getenv("BMX_REPLAY");

        checked = 1;
        if (rep != NULL && *rep != 0) {
            gMode = 2;
            gPath = rep;
        } else if (rec != NULL && *rec != 0) {
            gMode = 1;
            gPath = rec;
        }
    }
    return gMode;
}

static void startRecording(const BmxReplayPad *pad)
{
    static const char kMagic[4] = {'B', 'M', 'X', 'R'};
    const uint32_t version = kVersion;
    const uint8_t present = (uint8_t)pad->present, family = (uint8_t)pad->family;
    uint32_t saveSize = 0;
    uint8_t *save;

    gFile = fopen(gPath, "wb");
    if (gFile == NULL) {
        bmxLog("replay: cannot create %s, not recording\n", gPath);
        gMode = 0;
        return;
    }
    save = loadSave(&saveSize);
    put(kMagic, 4);
    put(&version, 4);
    put(&present, 1);
    put(&family, 1);
    put(&pad->vendor, 2);
    put(&pad->product, 2);
    put(pad->name, sizeof pad->name);
    put(&saveSize, 4);
    put(save, saveSize);
    free(save);
    bmxLog("replay: recording into %s (pad %s, save %u bytes)\n", gPath, pad->present ? pad->name : "none",
           (unsigned)saveSize);
}

static void startReplay(BmxReplayPad *pad)
{
    char magic[4];
    uint32_t version, saveSize, nowSize;
    uint8_t present, family;
    uint8_t *recorded, *now;

    memset(pad, 0, sizeof *pad);
    gFile = fopen(gPath, "rb");
    if (gFile == NULL || !get(magic, 4) || memcmp(magic, "BMXR", 4) != 0 || !get(&version, 4) ||
        version != kVersion || !get(&present, 1) || !get(&family, 1) || !get(&pad->vendor, 2) ||
        !get(&pad->product, 2) || !get(pad->name, sizeof pad->name) || !get(&saveSize, 4) ||
        saveSize > kSaveMax) {
        bmxLog("replay: %s is not a version %d recording\n", gPath, kVersion);
        exit(1);
    }
    pad->name[sizeof pad->name - 1] = 0;
    pad->present = present;
    pad->family = family;
    recorded = malloc(kSaveMax);
    if (recorded == NULL || !get(recorded, saveSize)) {
        bmxLog("replay: %s is cut short\n", gPath);
        exit(1);
    }
    now = loadSave(&nowSize);
    if (nowSize != saveSize || memcmp(now, recorded, saveSize) != 0) {
        bmxLog("replay: warning: save/SAVE.DAT differs from the recording's (%u vs %u bytes); the run may "
               "differ\n",
               (unsigned)nowSize, (unsigned)saveSize);
    }
    free(now);
    free(recorded);
    readNext();
    bmxLog("replay: replaying %s (pad %s)\n", gPath, pad->present ? pad->name : "none");
}

void bmxReplayStart(BmxReplayPad *pad)
{
    if (bmxReplayMode() == 1) {
        startRecording(pad);
    } else if (bmxReplayMode() == 2) {
        startReplay(pad);
    }
}

static void record(int dev, uint32_t frame, const int32_t *result, const void *data, uint32_t size)
{
    Record *const c = &gCur[dev];
    const uint8_t kb = (uint8_t)dev;
    const uint16_t n = (uint16_t)size;

    if (gHaveCur[dev] && c->result == *result && c->size == size && memcmp(c->data, data, size) == 0) {
        return;
    }
    gHaveCur[dev] = 1;
    c->result = *result;
    c->size = n;
    memcpy(c->data, data, size);
    put(&frame, 4);
    put(&gIndex, 4);
    put(&kb, 1);
    put(result, 4);
    put(&n, 2);
    put(data, size);
    if (++gWritten % kFlushEvery == 0 && gFile != NULL) {
        fflush(gFile);
    }
}

static void replay(int dev, uint32_t frame, int32_t *result, void *data, uint32_t size)
{
    static uint32_t endFrame;

    while (gHaveNext && (gNext.frame < frame || (gNext.frame == frame && gNext.index <= gIndex))) {
        if (gNext.keyboard < 2) {
            gCur[gNext.keyboard] = gNext;
            gHaveCur[gNext.keyboard] = 1;
            gApplied++;
        }
        readNext();
    }
    if (gHaveCur[dev] && gCur[dev].size == size) {
        *result = gCur[dev].result;
        memcpy(data, gCur[dev].data, size);
    }
    if (!gHaveNext) {
        if (endFrame == 0) {
            endFrame = frame;
            bmxLog("replay: end of the recording at frame %u (%d changes replayed)\n", (unsigned)frame,
                   gApplied);
        } else if (frame > endFrame + 60) { /* the recorded session ended without quitting the game */
            bmxLog("replay: done\n");
            fflush(NULL);
            _Exit(0);
        }
    }
}

void bmxReplayDeviceState(int keyboard, int32_t *result, void *data, uint32_t size)
{
    const uint32_t frame = (uint32_t)drvGpuShownFrames();
    const int dev = keyboard != 0;

    if (gMode == 0 || gFile == NULL || size > kMaxState) {
        return;
    }
    if (frame != gFrame) {
        gFrame = frame;
        gIndex = 0;
        if (gMode == 1 && frame % 30 == 0) {
            const uint8_t mark = 2;
            const int32_t zero = 0;
            const uint16_t none = 0;

            put(&frame, 4);
            put(&gIndex, 4);
            put(&mark, 1);
            put(&zero, 4);
            put(&none, 2);
        }
    }
    if (gMode == 1) {
        record(dev, frame, result, data, size);
    } else {
        replay(dev, frame, result, data, size);
    }
    gIndex++;
}

void bmxReplayClose(void)
{
    if (gFile != NULL) {
        fclose(gFile);
        gFile = NULL;
    }
}
