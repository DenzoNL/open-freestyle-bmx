/*
 * Fixtures for the overlay tests. Every callee outside the slice (overlay layer, pad, game state, save
 * file, ...) is replaced per iteration with dt_stub() by a stub that logs its arguments by value into
 * DLL data (compared like any other memory) and returns scripted values. Arguments the callee only reads
 * 16 bits of are logged as 16 bits, because the original leaves garbage in the upper half.
 */
#include "dt_overlay.h"

#include "bmx/com/action/action.h"
#include "bmx/com/action/camera.h"
#include "bmx/com/action/comdata.h"
#include "bmx/com/action/contrler.h"
#include "bmx/com/action/game.h"
#include "bmx/com/action/memcard.h"
#include "bmx/com/action/overlay.h"
#include "bmx/com/action/overlymp.h"
#include "bmx/com/action/replay.h"
#include "bmx/com/action/rider.h"
#include "bmx/com/zge/bmx/bmxsound.h"
#include "bmx/com/zge/bmx/zoverii.h"
#include "bmx/com/zge/ngif/missingzge.h"
#include "bmx/com/zge/ngif/ngifpsx.h"
#include "bmx/com/zge/ngif/ngifzres.h"
#include "bmx/com/zge/ngif/ngifzroller.h"
#include "bmx/pc/zge/sound/winchan.h"
#include "compat/crt.h"
#include "zge/zasset.h"
#include "zge/zfile.h"
#include "zge/zmem.h"
#include "zge/zstring.h"

/* ---- log ---------------------------------------------------------------------------------------- */

#define LOG_SIZE 4096

/* Exported so the compiler keeps the stores although nothing in the DLL reads the log. */
__declspec(dllexport) uint32_t dtOvlLog[LOG_SIZE];
__declspec(dllexport) uint32_t dtOvlLogN;
#define logBuf dtOvlLog
#define logN   dtOvlLogN

static void lg(uint32_t v)
{
    if (logN < LOG_SIZE) {
        logBuf[logN] = v;
    }
    logN++;
}

static uint32_t hashText(const char *s)
{
    uint32_t h = 2166136261u;
    while (*s != '\0') {
        h = (h ^ (uint8_t)*s++) * 16777619u;
    }
    return h;
}

/* ---- scripted answers --------------------------------------------------------------------------- */

typedef struct Record {
    uint32_t value;
    uint8_t rank;
    uint8_t flags;
    uint8_t pad[2];
    char name[16];
} Record;

static struct {
    uint32_t mode;
    uint32_t subMode;
    uint32_t runType;
    uint32_t runCount;
    uint32_t modeByte;
    uint32_t options;
    uint32_t pad[8];
    uint32_t pendingUnlock;
    uint32_t writeOk;
    uint32_t closeResult;
    uint32_t queryResult;
    uint32_t winQuery;
    uint32_t fileHandle;
    uint32_t fileWriteOk;
    uint32_t assetFailAt; /* 1-based call number of the failing asset lookup, 0: none */
    uint32_t assetCalls;
    uint32_t allocFailAt;
    uint32_t allocCalls;
    uint32_t parseFailAt;
    uint32_t parseCalls;
    uint32_t recordNull;
    uint32_t recordNullBit; /* 1: only keys with 0x1000 set, 2: only keys without */
    uint32_t handleNext;
    uint32_t reserveNext;
    uint32_t soundResult;
    uint32_t updateResult;
    uint32_t strIndex;
    uint32_t strMode;
} script;

static Record records[16];
static uint8_t levelRecords[16][32];
static uint8_t goalBlocks[4][0x70];
static uint8_t riderRecords[8][0x20];
static uint8_t rollerRecords[8][0x10];
static uint8_t controllers[2][16];
static char nameTable[16];
static uint8_t replayBuf[64];
static uint8_t poolMem[16][0x100];
static char textPool[64][48];

void *dtOvlRandomBytes(uint32_t size)
{
    uint8_t *p = dt_alloc(size);
    for (uint32_t i = 0; i < size; i++) {
        p[i] = (uint8_t)dt_u32();
    }
    return p;
}

char *dtOvlRandomText(int maxLen)
{
    const int len = dt_range(0, maxLen);
    char *s = dt_alloc((uint32_t)len + 1);
    for (int i = 0; i < len; i++) {
        s[i] = (char)('a' + dt_range(0, 25));
    }
    return s;
}

void dtOvlNoNullRecords(void)
{
    script.recordNull = 0;
}

void dtOvlNullRecord(uint32_t slot, uint32_t keyBit)
{
    script.recordNull = slot & 15;
    script.recordNullBit = keyBit;
}

void dtOvlNoInput(void)
{
    memset(script.pad, 0, sizeof script.pad);
}

void dtOvlMaybeNoScreen(void)
{
    if (dt_u32() % 5 == 0) {
        gOvlScreen = NULL;
    }
}

void dtOvlSetPad(int player, uint32_t buttons)
{
    script.pad[player & 7] = buttons;
}

void dtOvlSetMode(uint32_t mode, uint32_t subMode)
{
    script.mode = mode;
    script.subMode = subMode;
}

/* ---- stubs -------------------------------------------------------------------------------------- */

/* Strings: a fixed function of the id; some are empty, some carry capital letters or a %d. */
static const char *stubString(uint32_t id)
{
    static const char alphabet[] = "abcdefghijklmnopqrstuvwxyz ,.-ABCDEFGH";
    const uint16_t sid = (uint16_t)id;
    lg(0x57120000u | sid);
    char *s = textPool[script.strIndex++ & 63];
    const uint32_t h = sid * 2654435761u + script.strMode * 40503u;
    const unsigned kind = (h >> 21) & 15;
    unsigned len = 1 + (h >> 8) % 12;
    if (kind == 0) {
        s[0] = '\0';
        return s;
    }
    for (unsigned i = 0; i < len; i++) {
        const unsigned pick = (h >> (i % 20)) + i * 7;
        s[i] = kind < 4 ? alphabet[pick % (sizeof alphabet - 1)] : alphabet[pick % 27];
    }
    if (kind == 1 || kind == 2) { /* a format with an integer */
        s[len++] = '%';
        s[len++] = 'd';
    }
    s[len] = '\0';
    return s;
}

static int stubTextWidth(const char *text, const OvlFont *font)
{
    lg(0x71d70000u);
    lg(hashText(text));
    lg((uint32_t)(uintptr_t)font);
    return (int)(strlen(text) * 9 + (hashText(text) & 7));
}

static uint32_t stubAddText(int layer, const OvlFont *font, const char *text, int x, int y, uint32_t color,
                            int flags)
{
    lg(0xadd70000u);
    lg((uint32_t)layer);
    lg((uint32_t)(uintptr_t)font);
    lg(hashText(text));
    lg((uint16_t)x);
    lg((uint16_t)y);
    lg(color);
    lg((uint16_t)flags);
    return 0x1000 + (script.handleNext++) * 3;
}

static void stubRemove(uint32_t handle)
{
    lg(0x4e3f0000u);
    lg((uint16_t)handle);
}

static int stubShow(uint32_t handle, int show)
{
    lg(0x540d0000u);
    lg((uint16_t)handle);
    lg((uint32_t)show);
    return 0;
}

static int stubSetColor(uint32_t handle, const OvlFont *font, uint32_t color, int flags)
{
    lg(0x5c010000u);
    lg((uint16_t)handle);
    lg((uint32_t)(uintptr_t)font);
    lg(color);
    lg((uint16_t)flags);
    return 0;
}

static void stubDrawText(const OvlFont *font, const char *text, int x, int y, uint32_t color, int flags)
{
    lg(0xd7a70000u);
    lg((uint32_t)(uintptr_t)font);
    lg(hashText(text));
    lg((uint16_t)x);
    lg((uint16_t)y);
    lg(color);
    lg((uint16_t)flags);
}

static void stubDrawRect(int x, int y, int w, int h, uint32_t color)
{
    lg(0xd7ec0000u);
    lg((uint16_t)x);
    lg((uint16_t)y);
    lg((uint16_t)w);
    lg((uint16_t)h);
    lg(color);
}

static int stubSound(int id, const void *pos, int a, int volume, int b)
{
    lg(0x50d00000u);
    lg((uint32_t)id);
    lg((uint32_t)(uintptr_t)pos);
    lg((uint32_t)a);
    lg((uint32_t)volume);
    lg((uint32_t)b);
    return (int)script.soundResult;
}

static int stubPressed(int player, uint32_t mask, uint32_t repeat)
{
    lg(0x9e550000u);
    lg((uint32_t)player);
    lg(mask);
    lg(repeat);
    return (script.pad[player & 7] & mask) != 0;
}

static uint32_t stubAddRect(int layer, int x, int y, int w, int h, uint32_t color)
{
    lg(0xadd90000u);
    lg((uint32_t)layer);
    lg((uint16_t)x);
    lg((uint16_t)y);
    lg((uint16_t)w);
    lg((uint16_t)h);
    lg(color);
    return 0x1000 + (script.handleNext++) * 3;
}

static int stubSetRect(uint32_t handle, int x, int y, int w, int h)
{
    lg(0x5e7c0000u);
    lg((uint16_t)handle);
    lg((uint16_t)x);
    lg((uint16_t)y);
    lg((uint16_t)w);
    lg((uint16_t)h);
    return 0;
}

static uint32_t stubAddGradient(int layer, int x, int y, int w, int h, uint32_t top, uint32_t bottom)
{
    lg(0x62ad0000u);
    lg((uint32_t)layer);
    lg((uint16_t)x);
    lg((uint16_t)y);
    lg((uint16_t)w);
    lg((uint16_t)h);
    lg(top);
    lg(bottom);
    return 0x1000 + (script.handleNext++) * 3;
}

static uint32_t stubAddSprite(int layer, uint32_t id, int x, int y, uint32_t color)
{
    lg(0x5b2e0000u);
    lg((uint32_t)layer);
    lg((uint16_t)id);
    lg((uint16_t)x);
    lg((uint16_t)y);
    lg(color);
    return 0x1000 + (script.handleNext++) * 3;
}

static int stubFlush(void)
{
    lg(0xf1a50000u);
    return 0;
}

static int stubLayerFlag(uint32_t flag)
{
    lg(0x1a7e0000u);
    lg(flag & 0xff);
    return 0;
}

static int stubParseFont(const void *data, void *font)
{
    lg(0xfa2e0000u);
    lg((uint32_t)(uintptr_t)data);
    lg((uint32_t)(uintptr_t)font);
    script.parseCalls++;
    return script.parseFailAt == script.parseCalls;
}

static int stubTestOption(uint32_t mask)
{
    return (script.options & (mask & 0xff)) != 0;
}

static uint32_t stubGetMode(void)
{
    return script.mode;
}

static uint32_t stubGetSubMode(void)
{
    return script.subMode;
}

static uint32_t stubRunType(void)
{
    return script.runType;
}

static uint32_t stubRunCount(void)
{
    return script.runCount;
}

static uint8_t *stubRecord(uint32_t key)
{
    lg(0x4ec00000u);
    lg(key);
    if (script.recordNull != 0 && (key >> 16 & 15) == script.recordNull &&
        (script.recordNullBit == 0 || ((key & 0x1000) != 0) == (script.recordNullBit == 1))) {
        return NULL;
    }
    return (uint8_t *)&records[(key >> 16 ^ key >> 22) & 15];
}

static uint32_t stubStartMode(int mode)
{
    lg(0x57a70000u);
    lg((uint32_t)mode);
    return (uint32_t)mode;
}

static uint32_t stubSetSubMode(uint32_t sub)
{
    lg(0x5e750000u);
    lg(sub & 0xff);
    return sub;
}

static uint32_t stubBigGame(int state)
{
    lg(0xb1670000u);
    lg((uint32_t)state);
    return 0;
}

static const char *stubNameTable(void)
{
    return nameTable;
}

static uint32_t stubModeByte(void)
{
    return script.modeByte;
}

static void stubLeaveRun(void)
{
    lg(0x1ea50000u);
}

static void stubResetGoals(int player)
{
    lg(0x4e5e0000u);
    lg((uint32_t)player);
}

static int stubPendingUnlock(int player)
{
    lg(0x9e0d0000u);
    lg((uint32_t)player);
    return (int)script.pendingUnlock;
}

static uint8_t *stubGoalInfo(int player, uint32_t level)
{
    lg(0x60a10000u);
    lg((uint32_t)player);
    lg(level);
    return goalBlocks[(player * 3 + (int)level) & 3];
}

static uint32_t *stubLevelRecord(uint32_t key)
{
    lg(0x1e7e0000u);
    lg(key);
    return (uint32_t *)levelRecords[(key >> 16) & 15];
}

static void stubSetViewMode(int player, uint32_t mode)
{
    lg(0x5e7a0000u);
    lg((uint32_t)player);
    lg(mode);
}

static void stubResetView(int player, int mode)
{
    lg(0x4e5a0000u);
    lg((uint32_t)player);
    lg((uint32_t)mode);
}

static uint8_t *stubController(uint32_t index)
{
    lg(0xc0770000u);
    lg(index);
    return controllers[index & 1];
}

static void stubCamera(uint32_t player)
{
    lg(0xca3e0000u);
    lg(player);
}

static void stubRiderReset(int player, int a)
{
    lg(0x41de0000u);
    lg((uint32_t)player);
    lg((uint32_t)a);
}

static uint32_t stubActionMode(uint32_t mode)
{
    lg(0xac710000u);
    lg(mode);
    return 0;
}

static int stubSaveWrite(void)
{
    lg(0x5a7e0000u);
    return (int)script.writeOk;
}

static int stubSaveClose(void)
{
    lg(0x5ac10000u);
    return (int)script.closeResult;
}

static int stubSaveQuery(void)
{
    lg(0x5a9e0000u);
    return (int)script.queryResult;
}

static void stubHeader(const char *title)
{
    lg(0x4ead0000u);
    lg(hashText(title));
}

static void stubBody(int y)
{
    lg(0xb0d70000u);
    lg((uint32_t)y);
}

static void stubReplayBuffer(uint32_t source, uint8_t **data, int *size)
{
    lg(0x4e9a0000u);
    lg(source & 0xff);
    *data = replayBuf;
    *size = 12 + (int)(source & 7);
}

static void *stubFileOpen(const char *path, int mode, int flags)
{
    lg(0xf11e0000u);
    lg(hashText(path));
    lg((uint32_t)mode);
    lg((uint32_t)flags);
    return script.fileHandle != 0 ? (void *)&replayBuf[8] : NULL;
}

static int stubFileWrite(void *file, const void *data, int size, int flags)
{
    lg(0xf1170000u);
    lg((uint32_t)(uintptr_t)file);
    lg((uint32_t)(uintptr_t)data);
    lg((uint32_t)size);
    lg((uint32_t)flags);
    return script.fileWriteOk != 0 ? size : size - 1;
}

static void stubFillBar(uint32_t color, uint32_t x, int y, uint32_t width, const void *ot)
{
    lg(0xf11b0000u);
    lg(color);
    lg(x);
    lg((uint32_t)y);
    lg(width);
    lg((uint32_t)(uintptr_t)ot);
}

static const uint8_t *stubRiderRecord(uint32_t index)
{
    lg(0x41de1000u);
    lg(index);
    return riderRecords[index & 7];
}

static void stubRegister(int id, uint32_t a, uint32_t index)
{
    lg(0x4e960000u);
    lg((uint32_t)id);
    lg(a);
    lg(index & 0xffff);
}

static const uint8_t *stubRollerRecord(uint32_t index)
{
    lg(0x4011e000u);
    lg(index);
    return rollerRecords[index & 7];
}

static int stubReserve(int count)
{
    lg(0x4e5e4000u);
    lg((uint32_t)count);
    script.reserveNext += (uint32_t)count;
    return (int)script.reserveNext - count;
}

static void stubDrawCallback(ZrollerDrawFn fn)
{
    lg(0xdca10000u);
    lg((uint32_t)(uintptr_t)fn);
}

static void stubWinStart(uint32_t slot)
{
    lg(0x31150000u);
    lg(slot & 0xff);
}

static int stubWinCallback(WinchanCallback fn)
{
    lg(0x31ca0000u);
    lg((uint32_t)(uintptr_t)fn);
    return 0;
}

static int stubWinQuery(void)
{
    lg(0x31900000u);
    return (int)script.winQuery;
}

static void stubWinBegin(void)
{
    lg(0x31be0000u);
}

static void stubWinEnd(void)
{
    lg(0x31e40000u);
}

static void stubMenuMode(void)
{
    lg(0x5e450000u);
}

static int stubAssetFind(int lib, int id, int flags)
{
    lg(0xa55e0000u);
    lg((uint32_t)lib);
    lg((uint32_t)id);
    lg((uint32_t)flags);
    script.assetCalls++;
    return script.assetFailAt != script.assetCalls;
}

static uint32_t stubAssetSize(int lib)
{
    lg(0xa55a0000u);
    lg((uint32_t)lib);
    return 0x40;
}

static int stubAssetRead(int lib, void *dst)
{
    lg(0xa55d0000u);
    lg((uint32_t)lib);
    lg((uint32_t)(uintptr_t)dst);
    return 1;
}

static void *stubAlloc(uint32_t size)
{
    lg(0x5a110000u);
    lg(size);
    script.allocCalls++;
    if (script.allocFailAt == script.allocCalls) {
        return NULL;
    }
    return poolMem[script.reserveNext++ & 15];
}

static int stubStringRegister(const ZStringSet *set)
{
    lg(0x57e60000u);
    lg((uint32_t)(uintptr_t)set);
    return 0;
}

static int stubStringUnregister(const ZStringSet *set)
{
    lg(0x57e70000u);
    lg((uint32_t)(uintptr_t)set);
    return 0;
}

/* ---- callbacks of the element and screen tables ------------------------------------------------- */

static void elementLog(uint32_t kind, int index, uint32_t arg)
{
    lg(0xe1e00000u | kind << 8 | (uint32_t)index);
    lg(arg);
}

#define ELEMENT_CALLBACKS(n)                                                                                 \
    static void BMX_CDECL elemInit##n(int a)                                                                 \
    {                                                                                                        \
        elementLog(1, n, (uint32_t)a);                                                                       \
    }                                                                                                        \
    static void BMX_CDECL elemDraw##n(void)                                                                  \
    {                                                                                                        \
        elementLog(2, n, 0);                                                                                 \
    }                                                                                                        \
    static void BMX_CDECL elemShow##n(int a)                                                                 \
    {                                                                                                        \
        elementLog(3, n, (uint32_t)a);                                                                       \
    }                                                                                                        \
    static void BMX_CDECL elemSetA##n(const void *p)                                                         \
    {                                                                                                        \
        elementLog(4, n, *(const uint8_t *)p);                                                               \
    }                                                                                                        \
    static void BMX_CDECL elemSetB##n(void *p)                                                               \
    {                                                                                                        \
        elementLog(5, n, *(const uint8_t *)p);                                                               \
    }

ELEMENT_CALLBACKS(0)
ELEMENT_CALLBACKS(1)
ELEMENT_CALLBACKS(2)
ELEMENT_CALLBACKS(3)
ELEMENT_CALLBACKS(4)
ELEMENT_CALLBACKS(5)
ELEMENT_CALLBACKS(6)
ELEMENT_CALLBACKS(7)
ELEMENT_CALLBACKS(8)
ELEMENT_CALLBACKS(9)
ELEMENT_CALLBACKS(10)
ELEMENT_CALLBACKS(11)
ELEMENT_CALLBACKS(12)
ELEMENT_CALLBACKS(13)
ELEMENT_CALLBACKS(14)
ELEMENT_CALLBACKS(15)
ELEMENT_CALLBACKS(16)

#define ELEMENT_ROW(n) {0, elemInit##n, elemDraw##n, elemShow##n, elemSetA##n, elemSetB##n}

static const OvlElement elementTemplate[17] = {
    ELEMENT_ROW(0),  ELEMENT_ROW(1),  ELEMENT_ROW(2),  ELEMENT_ROW(3),  ELEMENT_ROW(4),  ELEMENT_ROW(5),
    ELEMENT_ROW(6),  ELEMENT_ROW(7),  ELEMENT_ROW(8),  ELEMENT_ROW(9),  ELEMENT_ROW(10), ELEMENT_ROW(11),
    ELEMENT_ROW(12), ELEMENT_ROW(13), ELEMENT_ROW(14), ELEMENT_ROW(15), ELEMENT_ROW(16),
};

static void BMX_CDECL screenEnter(int a)
{
    lg(0x5ce10000u);
    lg(gOvlScreenId);
    lg((uint32_t)a);
}

static int BMX_CDECL screenUpdate(void)
{
    lg(0x5cd10000u);
    lg(gOvlScreenId);
    return (int)script.updateResult;
}

static void BMX_CDECL screenDraw(void)
{
    lg(0x5cd20000u);
    lg(gOvlScreenId);
}

static OvlScreen fakeScreens[0x1b];

/* ---- state -------------------------------------------------------------------------------------- */

static uint32_t pick32(const uint32_t *values, unsigned n)
{
    return values[dt_u32() % n];
}

static uint16_t randHandle(void)
{
    return dt_u32() % 3 == 0 ? kOvlNoHandle : (uint16_t)(0x2000 + dt_range(0, 0x400));
}

static void randHandles(uint16_t *h, int n)
{
    for (int i = 0; i < n; i++) {
        h[i] = randHandle();
    }
}

static uint32_t randFrame(void)
{
    static const uint32_t special[] = {0,    1,    2,    3,    0x0f, 0x10, 0x11, 0x1f,
                                       0x20, 0x3b, 0x3c, 0x5f, 0x60, 0x61, 0x96, 0x97};
    return dt_u32() % 3 == 0 ? pick32(special, 16) : (uint32_t)dt_range(0, 0x120);
}

static uint16_t randTimer(void)
{
    static const uint16_t special[] = {0,    1,    2,    0x1d, 0x1e, 0x1f, 0x3b,  0x3c,
                                       0x3d, 0x59, 0x5a, 0x5b, 0x77, 0x78, 0x14a, 0x168};
    return dt_u32() % 2 == 0 ? special[dt_u32() % 16] : (uint16_t)dt_range(0, 0x170);
}

static float randFloat(void)
{
    static const float special[] = {0.0f,
                                    -0.0f,
                                    1.0f,
                                    -1.0f,
                                    15.0f,
                                    20.0f,
                                    379.0f,
                                    0.5f,
                                    -0.5f,
                                    100.0f,
                                    -100.0f,
                                    1e9f,
                                    __builtin_nanf(""),
                                    __builtin_inff()};
    return dt_u32() % 3 == 0 ? special[dt_u32() % 14] : dt_float(-3.0f, 400.0f);
}

static void randomState(void)
{
    static const uint32_t screenIds[] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12,
                                         13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25};
    static const uint32_t interesting[] = {4,    5,    6, 7, 0xd, 0xe, 0xf, 0x12, 0x13, 0x14,
                                           0x15, 0x16, 1, 2, 3,   8,   9,   10,   11,   12};

    /* screens and elements */
    for (int i = 0; i < 0x1b; i++) {
        OvlScreen *s = &fakeScreens[i];
        s->enter = dt_u32() % 8 == 0 ? NULL : screenEnter;
        s->update = screenUpdate;
        s->draw = dt_u32() % 2 == 0 ? NULL : screenDraw;
        s->itemCount = (uint8_t)dt_range(0, dt_u32() % 8 == 0 ? 12 : 7);
        s->selected = (uint8_t)(s->itemCount != 0 && dt_u32() % 10 != 0 ? dt_u32() % s->itemCount : dt_u32());
        gOvlScreens[i] = i == 0 ? NULL : s;
    }
    for (int i = 0; i < 17; i++) {
        gOvlElements[i] = elementTemplate[i];
        gOvlElements[i].flags = dt_u32() & 3;
        if (dt_u32() % 8 == 0) {
            gOvlElements[i].init = NULL;
        }
        if (dt_u32() % 8 == 0) {
            gOvlElements[i].show = NULL;
        }
        if (dt_u32() % 8 == 0) {
            gOvlElements[i].draw = NULL;
        }
        if (dt_u32() % 8 == 0) {
            gOvlElements[i].setA = NULL;
        }
        if (dt_u32() % 8 == 0) {
            gOvlElements[i].setB = NULL;
        }
    }
    gOvlScreen = gOvlScreens[1 + dt_u32() % 25];
    gOvlScreenId = (uint8_t)(dt_u32() % 2 == 0 ? pick32(interesting, 20) : pick32(screenIds, 26));
    gOvlReturnScreen = (uint8_t)pick32(screenIds, 26);
    gOvlLastScreen = (uint8_t)(1 + dt_u32() % 25);
    gOvlFrame = randFrame();
    gOvlPulse = dt_u32() % 4 == 0 ? dt_u32() >> 8 : (uint32_t)dt_range(0, 400);
    gOvlDrawDelay = dt_u32() % 3 == 0 ? randFrame() : (uint32_t)dt_range(0, 3);
    gOvlSharedPad = dt_u32() % 2;
    gOvlPage = (uint8_t)dt_range(0, 12);
    gOvlSaving = dt_u32() % 2;
    gOvlSaveStep = (int32_t)(dt_u32() % 2 == 0 ? (int32_t)dt_range(-1, 5) : (int32_t)dt_range(0, 12));
    gOvlSaveSlot = (uint8_t)(dt_u32() % 2);
    gOvlSavedSubMode = (int8_t)dt_range(-2, 3);
    gOvlBackdrop = (uint8_t)(dt_u32() % 3 == 0 ? dt_u32() : dt_range(0, 1));
    gOvlBackdropRect = randHandle();
    gOvlWarnLevel = (uint8_t)dt_u32();

    /* game state */
    gCurrentLevel = (uint8_t)(dt_u32() % 10 == 0 ? dt_u32() : dt_range(0, 8));
    gCurrentPlayer = (uint8_t)(dt_u32() % 12 == 0 ? dt_range(0, 3) : dt_range(0, 1));
    gPlayerCount = (uint8_t)(dt_u32() % 12 == 0 ? dt_range(0, 3) : dt_range(1, 2));
    gDemoPlayer = (uint8_t)dt_u32();
    gSoundVolumes[0] = (uint8_t)(dt_u32() % 8 == 0 ? dt_u32() : dt_range(0, 0x14));
    gSoundVolumes[1] = (uint8_t)(dt_u32() % 8 == 0 ? dt_u32() : dt_range(0, 0x14));
    gSoundVolumes[2] = (uint8_t)dt_range(0, 11);
    gSoundVolumes[3] = (uint8_t)dt_u32();
    gOptionFlags = (uint8_t)dt_u32();
    for (int p = 0; p < 2; p++) {
        OvlProfile *pr = &gProfiles[p];
        pr->flags = (uint8_t)dt_u32();
        pr->controller = (uint8_t)dt_range(0, 1);
        pr->character = (uint8_t)(dt_u32() % 10 == 0 ? dt_u32() : dt_range(0, 12));
        pr->option = (uint8_t)dt_range(0, 4);
        const int len = dt_range(0, 9);
        for (int i = 0; i < len; i++) {
            pr->name[i] = (char)('A' + dt_range(0, 25));
        }
        pr->name[len] = '\0';
        uint8_t *rec = gPlayerRecs[p];
        rec[kOvlPlayerNameLen] = (uint8_t)dt_range(0, 7);
        for (int i = 0; i < 8; i++) {
            rec[kOvlPlayerName + i] = (uint8_t)dt_range(0, 11);
        }
        *(uint32_t *)(rec + kOvlPlayerReady) = dt_u32() % 2;
        *(uint32_t *)(rec + kOvlPlayerScore) = dt_u32() % 4 == 0 ? dt_u32() : (uint32_t)dt_range(0, 5000);
        *(uint32_t *)(rec + kOvlPlayerNameCount) = (uint32_t)dt_range(0, 8);
        *(uint32_t *)(rec + kOvlPlayerUnlockMask) = dt_u32();
        for (int i = 0; i < 0x30; i++) {
            rec[kOvlPlayerRunTimes + i] = (uint8_t)(dt_u32() % 5 == 0 ? dt_u32() : dt_range(0, 120));
        }
    }
    for (int r = 0; r < 12; r++) {
        uint8_t *rd = gRiderData[r];
        *(uint32_t *)rd = dt_u32();
        rd[4] = (uint8_t)dt_range(0, 3);
        rd[5] = (uint8_t)dt_range(0, 3);
        rd[6] = (uint8_t)dt_range(0, 3);
        for (int i = 0; i < 11; i++) {
            rd[kOvlRiderTrickBits + i] = (uint8_t)dt_u32();
        }
    }
    for (int i = 0; i < 12; i++) {
        gCareer[i][2] = (uint8_t)dt_range(0, 255);
    }
    for (int i = 0; i < 3; i++) {
        gCareerUnlocked[i] = (uint8_t)dt_range(0, 11);
    }
    for (int i = 0; i < 16; i++) {
        gScoreRecordSlot[i] = (uint8_t)dt_range(0, 9);
    }

    /* fonts */
    gOvlFontSmall.height = (uint8_t)dt_range(10, 30);
    gOvlFontTiny.height = (uint8_t)dt_range(8, 24);
    gOvlFontIcons.height = (uint8_t)dt_range(8, 30);
    gOvlFontButton.height = (uint8_t)dt_range(8, 30);
    gOvlResA = randHandle();
    gOvlResB = randHandle();

    /* handles and screen data */
    randHandles(gOvlHandles, 24);
    for (int i = 0; i < 7; i++) {
        gOvlMenuItems[i].action = (uint16_t)dt_range(0, 8);
        gOvlMenuItems[i].text = (uint16_t)dt_u32();
    }
    for (int i = 0; i < 10; i++) {
        gOvlMenuItems2[i].action = (uint16_t)dt_range(0, 9);
        gOvlMenuItems2[i].text = (uint16_t)dt_u32();
    }
    randHandles(gOvlVolHandles, 7);
    gOvlTrickCount = (uint32_t)dt_range(0, 0x54);
    for (int i = 0; i < 0x54; i++) {
        static const uint8_t kinds[] = {0, 1, 0x40, 0x41, 0x80, 0x81, 0xc0, 0xc1, 0x7f};
        gOvlTrickList[i][0] = kinds[dt_u32() % 9];
        gOvlTrickList[i][1] = (uint8_t)dt_range(0, (gOvlTrickList[i][0] & 0x3f) == 1 ? 0x25 : 0x2d);
    }
    randHandles(gOvlTrickRows, 21);
    gOvlTrickDesc = randHandle();
    gOvlTrickView[1] = (uint16_t)dt_range(0, 0x52);
    gOvlTrickView[0] = (uint16_t)(gOvlTrickView[1] > 6 && dt_u32() % 2
                                      ? gOvlTrickView[1] - (dt_u32() % 4 == 0 ? 0 : dt_range(0, 9))
                                      : 0);
    if (gOvlTrickView[0] > 0x4a) {
        gOvlTrickView[0] = 0x4a;
    }
    for (int r = 0; r < 5; r++) {
        for (int k = 0; k < 4; k++) {
            gOvlGoalRows[r][k] = (uint16_t)(r < 3 || dt_u32() % 2 ? 0x2000 + r * 4 + k : kOvlNoHandle);
        }
    }
    /* The draw loops scan rows until a terminator: real tables always end within the 5 rows. Without
     * it they walk into memory the stub logs are changing, and the two sides stop at different places. */
    gOvlGoalRows[4][0] = kOvlNoHandle;
    for (int r = 0; r < 10; r++) {
        randHandles(gOvlRunTimes[r], 4);
    }
    for (int i = 0; i < 15; i++) {
        gOvlNameBuf[i] =
            dt_u32() % 6 == 0 ? ' ' : (char)(dt_u32() % 2 ? 'A' + dt_range(0, 25) : 'a' + dt_range(0, 25));
    }
    gOvlNameBuf[dt_range(0, 12)] = '\0';
    gOvlNameBuf[15] = '\0';
    gOvlNameCursor = (uint8_t)dt_range(0, 11);

    /* HUD elements */
    gOvlFade.flags = (uint8_t)dt_u32();
    gOvlFade.ticks = (uint16_t)dt_range(0, 5);
    gOvlFade.r = randFloat();
    gOvlFade.g = randFloat();
    gOvlFade.b = randFloat();
    gOvlFade.dr = randFloat();
    gOvlFade.dg = randFloat();
    gOvlFade.db = randFloat();
    gOvlScoreHandle = randHandle();
    gOvlScoreShown = dt_u32() % 4 == 0 ? dt_u32() : (uint32_t)dt_range(0, 2000);
    gOvlScoreTarget =
        dt_u32() % 3 == 0 ? gOvlScoreShown + (uint32_t)dt_range(0, 30) : (uint32_t)dt_range(0, 3000);
    gOvlScoreFlags = (uint8_t)dt_u32();
    gOvlTimerHandle = randHandle();
    for (int i = 0; i < 7; i++) {
        gOvlTimerText[i] = (char)('0' + dt_range(0, 9));
    }
    gOvlTimerText[dt_range(0, 6)] = '\0';
    gOvlTimerTicks = dt_u32() % 5 == 0 ? (uint32_t)dt_range(0, 100000) : (uint32_t)dt_range(0, 5000);
    gOvlTimerShownTicks = dt_u32();
    randHandles(gOvlHeightHandles, 6);
    gOvlHeightValue = randFloat();
    gOvlHeightTarget = randFloat();
    gOvlHeightExtra = dt_u32();
    gOvlHeightKind = (uint16_t)dt_range(0, 12);
    gOvlHeightScroll = randFloat();
    randHandles(gOvlBalanceHandles, 7);
    gOvlBalanceValue = randFloat() * 0.01f;
    gOvlCaptionHandle = randHandle();
    gOvlCaptionCurrent = (uint8_t)(dt_u32() % 3 == 0 ? 0xff : dt_range(0, 9));
    gOvlCaptionWanted = (uint8_t)(dt_u32() % 3 == 0 ? 0xff : dt_range(0, 9));
    gOvlElement8Value = (uint8_t)dt_u32();
    gOvlIndicator.id = (uint8_t)dt_range(0, 4);
    gOvlIndicator.counter = (uint16_t)dt_u32();
    gOvlHint.id = (uint8_t)(dt_u32() % 4 == 0 ? 0xff : dt_range(0, 20));
    gOvlHint.arg = (uint8_t)dt_u32();
    gOvlHint.timer = randTimer();
    randHandles(gOvlHintHandles, 7);
    gOvlGoalMsg.id = (uint8_t)(dt_u32() % 4 == 0 ? 0xff : dt_range(0, 8));
    gOvlGoalMsg.arg = (uint8_t)dt_u32();
    gOvlGoalMsg.timer = randTimer();
    randHandles(gOvlGoalHandles, 3);
    gOvlStatus.msg.id = (uint8_t)(dt_u32() % 4 == 0 ? 0xff : dt_range(0, 0x18));
    gOvlStatus.msg.arg = (uint8_t)dt_u32();
    gOvlStatus.msg.timer = randTimer();
    gOvlStatus.value = dt_u32() % 3 == 0 ? dt_u32() : (uint32_t)dt_range(0, 400);
    gOvlStatusHandle = randHandle();
    gOvlFlash.id = (uint8_t)(dt_u32() % 4 == 0 ? 0xff : dt_range(0, 5));
    gOvlFlash.arg = (uint8_t)dt_u32();
    gOvlFlash.timer = randTimer();
    randHandles(gOvlFlashHandles, 4);
    gOvlIconHandle = randHandle();

    /* font image references and string table pointers are left as they are */
}

static void randomScript(void)
{
    static const uint32_t modes[] = {0, 0, 0, 1, 2, 3, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    memset(&script, 0, sizeof script);
    script.mode = pick32(modes, 15);
    script.subMode = dt_u32() % 4;
    script.runType = dt_u32() % 8;
    script.runCount = dt_u32() % 4 == 0 ? dt_u32() % 256 : dt_u32() % 5;
    script.modeByte = dt_u32() & 0xff;
    script.options = dt_u32();
    static const uint32_t buttons[] = {1, 2, 4, 8, 0x10, 0x20, 0x80, 0x100, 0x4000, 0x10000, 0x20000};
    for (int p = 0; p < 8; p++) {
        const unsigned presses = dt_u32() % 5 == 0 ? 0 : 1 + dt_u32() % 3;
        for (unsigned i = 0; i < presses; i++) {
            script.pad[p] |= buttons[dt_u32() % 11];
        }
    }
    script.pendingUnlock = dt_u32() % 2;
    script.writeOk = dt_u32() % 2;
    script.closeResult = dt_u32() % 8;
    script.queryResult = dt_u32() % 3 == 0 ? 7 : dt_u32() % 3;
    script.winQuery = dt_u32() % 2;
    script.fileHandle = dt_u32() % 4 != 0;
    script.fileWriteOk = dt_u32() % 2;
    script.assetFailAt = dt_u32() % 3 == 0 ? 0 : 1 + dt_u32() % 5;
    script.allocFailAt = dt_u32() % 3 == 0 ? 0 : 1 + dt_u32() % 5;
    script.parseFailAt = dt_u32() % 3 == 0 ? 0 : 1 + dt_u32() % 5;
    script.recordNull = dt_u32() % 10 == 0 ? 1 + dt_u32() % 15 : 0;
    script.recordNullBit = dt_u32() % 3;
    script.handleNext = dt_u32() % 100;
    script.reserveNext = dt_u32() % 50;
    script.soundResult = dt_u32() % 256;
    script.updateResult = dt_u32() % 4 == 0;
    script.strIndex = dt_u32();
    script.strMode = dt_u32() % 5;

    for (int i = 0; i < 16; i++) {
        Record *r = &records[i];
        r->value = dt_u32() % 4 == 0 ? dt_u32() : (uint32_t)dt_range(0, 4000);
        r->rank = (uint8_t)dt_range(0, 10);
        r->flags = (uint8_t)dt_u32();
        for (int k = 0; k < 15; k++) {
            r->name[k] = (char)('A' + dt_range(0, 25));
        }
        r->name[dt_range(0, 12)] = '\0';
        r->name[15] = '\0';
        for (uint32_t k = 0; k < 7; k++) {
            levelRecords[i][k] = (uint8_t)dt_u32();
        }
        const int nameLen = dt_range(0, 10); /* the name is a short string at +7 */
        for (int k = 0; k < nameLen; k++) {
            levelRecords[i][7 + k] = (uint8_t)('A' + dt_range(0, 25));
        }
        levelRecords[i][7 + nameLen] = 0;
    }
    for (int i = 0; i < 4; i++) {
        for (int k = 0; k < 0x70; k++) {
            goalBlocks[i][k] = (uint8_t)(dt_u32() % 3 == 0 ? dt_u32() : dt_u32() % 5 * 0x40);
        }
        goalBlocks[i][1] = (uint8_t)dt_range(0, 4);
        goalBlocks[i][2] = (uint8_t)(dt_u32() % 3 == 0 ? 4 : dt_range(0, 3));
    }
    for (int i = 0; i < 8; i++) {
        for (int k = 0; k < 0x20; k++) {
            riderRecords[i][k] = (uint8_t)('a' + dt_range(0, 25));
        }
        riderRecords[i][4] = (uint8_t)dt_u32();
        riderRecords[i][6] = (uint8_t)dt_u32();
        riderRecords[i][7] = (uint8_t)dt_u32();
        riderRecords[i][8 + dt_range(0, 8)] = 0;
        riderRecords[i][0x1f] = 0;
        for (int k = 0; k < 0x10; k++) {
            rollerRecords[i][k] = (uint8_t)dt_u32();
        }
    }
    for (int i = 0; i < 2; i++) {
        for (int k = 0; k < 16; k++) {
            controllers[i][k] = (uint8_t)dt_u32();
        }
    }
    for (int k = 0; k < 15; k++) {
        nameTable[k] = (char)('A' + dt_range(0, 25));
    }
    nameTable[15] = '\0';
    for (int k = 0; k < 64; k++) {
        replayBuf[k] = (uint8_t)dt_u32();
    }
}

typedef struct StubDef {
    uint32_t orig;
    void *ours;
    void *stub;
    uint64_t bit;
} StubDef;

#define STUB(addr, ours, fn, bit) {addr, (void *)(ours), (void *)(fn), bit}

static void installStubs(uint64_t stubs)
{
    static const StubDef defs[] = {
        STUB(0x004824f0, zstringGet, stubString, DTS_STRING),
        STUB(0x00459f30, zoverTextWidth, stubTextWidth, DTS_TEXTWIDTH),
        STUB(0x004594c0, zoverAddText, stubAddText, DTS_ADDTEXT),
        STUB(0x004581a0, zoverRemove, stubRemove, DTS_REMOVE),
        STUB(0x00458400, zoverShow, stubShow, DTS_SHOW),
        STUB(0x00459940, zoverSetColor, stubSetColor, DTS_SETCOLOR),
        STUB(0x00459df0, zoverDrawText, stubDrawText, DTS_DRAWTEXT),
        STUB(0x00458ea0, zoverDrawRect, stubDrawRect, DTS_DRAWRECT),
        STUB(0x0044e300, NULL, stubSound, DTS_SOUND),
        STUB(0x00408590, NULL, stubPressed, DTS_PRESSED),
        STUB(0x004588d0, zoverAddRect, stubAddRect, DTS_ADDRECT),
        STUB(0x00458b00, zoverSetRect, stubSetRect, DTS_SETRECT),
        STUB(0x00458f20, zoverAddGradient, stubAddGradient, DTS_ADDGRADIENT),
        STUB(0x00459180, zoverAddSprite, stubAddSprite, DTS_ADDSPRITE),
        STUB(0x00457b00, zoverFlush, stubFlush, DTS_FLUSH),
        STUB(0x00457f50, zoverSetLayerFlag, stubLayerFlag, DTS_LAYERFLAG),
        STUB(0x0040a9f0, NULL, stubGetMode, DTS_GETMODE),
        STUB(0x0040aa30, NULL, stubGetSubMode, DTS_GETSUB),
        STUB(0x0040a890, NULL, stubTestOption, DTS_TESTOPT),
        STUB(0x0040be50, NULL, stubRecord, DTS_RECORD),
        STUB(0x0040bd50, NULL, stubRunType, DTS_RUNTYPE),
        STUB(0x0040bdc0, NULL, stubRunCount, DTS_RUNCOUNT),
        STUB(0x0040a8c0, NULL, stubStartMode, DTS_STARTMODE),
        STUB(0x0040aa10, NULL, stubSetSubMode, DTS_SETSUB),
        STUB(0x0040aa50, NULL, stubBigGame, DTS_BIGGAME),
        STUB(0x0040c050, NULL, stubNameTable, DTS_NAMETABLE),
        STUB(0x0040c070, NULL, stubModeByte, DTS_MODEBYTE),
        STUB(0x0040c0b0, NULL, stubLeaveRun, DTS_LEAVERUN),
        STUB(0x0040cd40, NULL, stubResetGoals, DTS_RESETGOALS),
        STUB(0x0040cf70, NULL, stubPendingUnlock, DTS_PENDING),
        STUB(0x00407fc0, NULL, stubGoalInfo, DTS_GOALINFO),
        STUB(0x00407d20, NULL, stubLevelRecord, DTS_LEVELREC),
        STUB(0x00407620, NULL, stubSetViewMode, DTS_SETVIEW),
        STUB(0x00407760, NULL, stubResetView, DTS_RESETVIEW),
        STUB(0x004086f0, NULL, stubController, DTS_CONTROLLER),
        STUB(0x00407530, NULL, stubCamera, DTS_CAMERA),
        STUB(0x00431aa0, NULL, stubRiderReset, DTS_RIDERRESET),
        STUB(0x00404f40, NULL, stubActionMode, DTS_ACTION),
        STUB(0x00412340, NULL, stubSaveWrite, DTS_SAVEWRITE),
        STUB(0x00412130, NULL, stubSaveClose, DTS_SAVECLOSE),
        STUB(0x00412170, NULL, stubSaveQuery, DTS_SAVEQUERY),
        STUB(0x00412620, NULL, stubHeader, DTS_HEADER),
        STUB(0x004134a0, NULL, stubBody, DTS_BODY),
        STUB(0x00430d70, NULL, stubReplayBuffer, DTS_REPLAY),
        STUB(0x00484700, NULL, stubFileOpen, DTS_FILEOPEN),
        STUB(0x00484b90, NULL, stubFileWrite, DTS_FILEWRITE),
        STUB(0x004623a0, NULL, stubFillBar, DTS_FILLBAR),
        STUB(0x0046bb00, NULL, stubRiderRecord, DTS_RIDERREC),
        STUB(0x0046bf30, NULL, stubRegister, DTS_REGISTER),
        STUB(0x0046c920, (void *)zrollerGetRecord, stubRollerRecord, DTS_ROLLERREC),
        STUB(0x0046ccd0, (void *)zrollerReserve, stubReserve, DTS_RESERVE),
        STUB(0x0046d690, (void *)zrollerAddDrawCallback, stubDrawCallback, DTS_DRAWCB),
        STUB(0x0045a290, NULL, stubMenuMode, DTS_MENUMODE),
        STUB(0x0046d720, NULL, stubWinStart, DTS_WINSTART),
        STUB(0x0046d7a0, NULL, stubWinCallback, DTS_WINCALLBACK),
        STUB(0x0046d8a0, NULL, stubWinQuery, DTS_WINQUERY),
        STUB(0x0046d8c0, NULL, stubWinBegin, DTS_WINBEGIN),
        STUB(0x0046d8e0, NULL, stubWinEnd, DTS_WINEND),
        STUB(0x00480780, NULL, stubAssetFind, DTS_ASSET),
        STUB(0x00480850, NULL, stubAssetSize, DTS_ASSET),
        STUB(0x00480930, NULL, stubAssetRead, DTS_ASSET),
        STUB(0x00485d80, zmemStaticAlloc, stubAlloc, DTS_ALLOC),
        STUB(0x00459fc0, zoverParseFont, stubParseFont, DTS_PARSEFONT),
        STUB(0x00482450, zstringRegister, stubStringRegister, DTS_STRSETS),
        STUB(0x004824a0, zstringUnregister, stubStringUnregister, DTS_STRSETS),
    };
    for (unsigned i = 0; i < sizeof defs / sizeof defs[0]; i++) {
        if ((defs[i].bit & stubs) == 0) {
            continue;
        }
        dt_stub(defs[i].orig, defs[i].ours, defs[i].stub);
    }
}

void dtOvlBegin(const DtCtx *ctx, uint64_t stubs)
{
    dt_seed(ctx);
    logN = 0;
    randomScript();
    if (dt_u32() % 3 == 0) {
        dtOvlNoInput();
    }
    randomState();
    installStubs(stubs);
}
