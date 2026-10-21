/* Fixtures of the camera / comdata / controller / effect / prop / replay / save tests (see dt_gameplay.h). */
#include "dt_gameplay.h"

#include "bmx/com/action/action.h"
#include "bmx/com/action/comdata.h"
#include "bmx/com/action/contrler.h"
#include "bmx/com/action/fx.h"
#include "bmx/com/action/game.h"
#include "bmx/com/action/overlay.h"
#include "bmx/com/action/prop.h"
#include "bmx/com/menu/gui/guiinput.h"
#include "bmx/com/zge/bmx/bmxsound.h"
#include "bmx/com/zge/bmx/zoverii.h"
#include "bmx/com/zge/ngif/missingzge.h"
#include "bmx/com/zge/ngif/ngiffx.h"
#include "bmx/com/zge/ngif/ngifpsx.h"
#include "bmx/com/zge/zactor.h"
#include "compat/crt.h"
#include "zge/zdebug.h"
#include "zge/zgemods/zobjcamera.h"
#include "zge/zobj.h"

DtgpScript dtgp;

#define LOG_SIZE 4096

/* Exported so the compiler keeps the stores although nothing in the DLL reads the log. */
__declspec(dllexport) uint32_t dtgpLogBuf[LOG_SIZE];
__declspec(dllexport) uint32_t dtgpLogCount;

void dtgpLog(uint32_t v)
{
    if (dtgpLogCount < LOG_SIZE) {
        dtgpLogBuf[dtgpLogCount] = v;
    }
    dtgpLogCount++;
}

void dtgpLogF(float f)
{
    uint32_t u;
    memcpy(&u, &f, 4);
    dtgpLog(u);
}

void dtgpLogP(const void *p)
{
    dtgpLog(dt_canon((uint32_t)(uintptr_t)p));
}

void dtgpLogN(const void *p, uint32_t words)
{
    const uint32_t *w = p;
    for (uint32_t i = 0; i < words; i++) {
        dtgpLog(w[i]);
    }
}

uint32_t dtgpHash(const char *s)
{
    uint32_t h = 2166136261u;
    while (*s != '\0') {
        h = (h ^ (uint8_t)*s++) * 16777619u;
    }
    return h;
}

/* ---- stubs -------------------------------------------------------------------------------------- */

static void BMX_CDECL stubFail(const char *why, const char *file, int line, int flags)
{
    dtgpLog(0xfa110000u);
    dtgpLog(dtgpHash(why));
    dtgpLog(dtgpHash(file));
    dtgpLog((uint32_t)line);
    dtgpLog((uint32_t)flags);
}

/* Logs the arguments by value; a %s argument is logged by content (the exe and the DLL keep their own
 * literals). */
static void BMX_CDECL stubPrint(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    dtgpLog(0x9217a000u);
    for (const char *p = fmt; *p; p++) {
        if (*p == '%') {
            p++;
            while (*p != '\0' && *p != 'd' && *p != 's') {
                p++;
            }
            if (*p == '\0') {
                break;
            }
            if (*p == 's') {
                const char *str = va_arg(ap, const char *);
                dtgpLog(dt_canon((uint32_t)(uintptr_t)str) == DT_STACK_PTR ? DT_STACK_PTR : dtgpHash(str));
            } else {
                dtgpLog(dt_canon((uint32_t)va_arg(ap, int)));
            }
        }
    }
    dtgpLog(dtgpHash(fmt));
    va_end(ap);
}

static void BMX_CDECL stubActorShow(void *actor)
{
    dtgpLog(0xac700001u);
    dtgpLogP(actor);
}

static void BMX_CDECL stubActorHide(void *actor)
{
    dtgpLog(0xac700002u);
    dtgpLogP(actor);
}

static void BMX_CDECL stubHorizonOn(void)
{
    dtgpLog(0x45a1f000u);
}

static void BMX_CDECL stubHorizonOff(void)
{
    dtgpLog(0x45a22000u);
}

static void BMX_CDECL stubSetView(uint32_t player, uint32_t view)
{
    dtgpLog(0x078c0000u);
    dtgpLog(player);
    dtgpLog(view);
}

static int BMX_CDECL stubSoundAt(int id, const void *pos, int a, int volume, int b)
{
    dtgpLog(0x50d00000u);
    dtgpLog((uint32_t)id);
    dtgpLog((uint32_t)a);
    dtgpLog((uint32_t)volume);
    dtgpLog((uint32_t)b);
    if (pos != NULL) {
        dtgpLogN(pos, 3);
    }
    return 0;
}

static void BMX_CDECL stubSetFov(void *camera, int fov)
{
    dtgpLog(0xca3f0001u);
    dtgpLogP(camera);
    dtgpLog((uint16_t)fov); /* the original passes the word with stale upper bits */
}

static void BMX_CDECL stubLookAt(void *camera, const ZVec3 *eye, const ZVec3 *target, int unused)
{
    dtgpLog(0xca3f0002u);
    dtgpLogP(camera);
    dtgpLogN(eye, 3);
    dtgpLogN(target, 3);
    dtgpLog((uint32_t)unused);
}

static void BMX_CDECL stubFillBar(uint32_t color, uint32_t x, int y, uint32_t width, const void *ot)
{
    dtgpLog(0xf1bb0000u);
    dtgpLog(color);
    dtgpLog(x);
    dtgpLog((uint32_t)y);
    dtgpLog(width);
    dtgpLogN(ot, 2);
}

static void *BMX_CDECL stubPadCreate(int kind)
{
    dtgpLog(0x9adc0000u);
    dtgpLog((uint32_t)kind);
    return (void *)(uintptr_t)dtgp.padResults[kind == 2 ? 0 : 1];
}

static void BMX_CDECL stubPadDestroy(void *device)
{
    dtgpLog(0x9add0000u);
    dtgpLogP(device);
}

/* ---- effect engine ------------------------------------------------------------------------------ */

static const uint8_t *BMX_CDECL stubFxNode(const void *obj, int index)
{
    dtgpLog(0x45f48000u);
    dtgpLogP(obj);
    dtgpLog((uint32_t)index);
    return dtgp.node;
}

/* Logs the words of the parameter block selected by dtgp.extra[0] (count) and dtgp.extra[1] (mask). */
static int BMX_CDECL stubFxCreate(int kind, const void *params, const void *arg)
{
    const uint32_t *w = params;
    const uint32_t mask = dtgp.extra[1] != 0 ? dtgp.extra[1] : 0xffffffffu;

    dtgpLog(0x45fcb000u);
    dtgpLog((uint32_t)kind);
    dtgpLogP(params);
    dtgpLogP(arg);
    for (uint32_t i = 0; i < dtgp.extra[0]; i++) {
        if ((mask >> i & 1) != 0) {
            dtgpLog(w[i]);
        }
    }
    return 0;
}

static void BMX_CDECL stubNameCall(const char *name, uint32_t a, uint32_t b)
{
    const uint32_t hash = dtgpHash(name);
    uint32_t words = dtgp.sparkWords;

    dtgpLog(0x482fc000u);
    dtgpLog(hash);
    dtgpLogP((const void *)(uintptr_t)a);
    if (name[0] == 'r') {
        words = 9;
    } else if (name[0] == 's' && name[1] == 'p' && name[2] == 'l') {
        words = 13;
    }
    if (name[0] == 'r') {
        /* The ripple block's word 3 is never written by the original (stack garbage). */
        const uint32_t *w = (const uint32_t *)(uintptr_t)b;
        for (uint32_t i = 0; i < words; i++) {
            if (i != 3) {
                dtgpLog(w[i]);
            }
        }
        return;
    }
    dtgpLogN((const void *)(uintptr_t)b, words);
}

static void BMX_CDECL stubPsxSetup(const void *params)
{
    dtgpLog(0x45fdc000u);
    dtgpLogP(params);
}

static int BMX_CDECL stubRegisterTable(const void *table, int count)
{
    dtgpLog(0x45a30000u);
    dtgpLogP(table);
    dtgpLog((uint32_t)count);
    return 1;
}

static void BMX_CDECL stubDestroyAll(int mode)
{
    dtgpLog(0x45a2d000u);
    dtgpLog((uint32_t)mode);
}

static void BMX_CDECL stubFxShutdown(void)
{
    dtgpLog(0x45fc1000u);
}

static void BMX_CDECL stubFxRegisterHandlers(void)
{
    dtgpLog(0x45f78000u);
}

/* ---- static region, game state ------------------------------------------------------------------ */

static uint32_t *BMX_CDECL stubStaticMark(void)
{
    dtgpLog(0x485e4000u);
    return (uint32_t *)(uintptr_t)dtgp.extra[2];
}

static void *BMX_CDECL stubStaticAlloc(uint32_t size)
{
    const uint32_t call = dtgp.regionUsed++;

    dtgpLog(0x485d8000u);
    dtgpLog(size);
    return (dtgp.staticFail >> call & 1) != 0 ? NULL : dtgp.region;
}

static void BMX_CDECL stubStaticRelease(const uint32_t *mark)
{
    dtgpLog(0x485e1000u);
    dtgpLog((uint32_t)(uintptr_t)mark);
}

static uint32_t BMX_CDECL stubGetMode(void)
{
    dtgpLog(0x40a9f000u);
    return dtgp.mode;
}

static uint32_t BMX_CDECL stubGetSubMode(void)
{
    dtgpLog(0x40aa3000u);
    return dtgp.subMode;
}

static uint8_t *BMX_CDECL stubGoalInfo(int player, uint32_t level)
{
    dtgpLog(0x407fc000u);
    dtgpLog((uint32_t)player);
    dtgpLog(level);
    return dtgp.goalInfo;
}

/* ---- props -------------------------------------------------------------------------------------- */

static void BMX_CDECL stubSetVisible(int32_t object, int visible)
{
    dtgpLog(0x48760000u);
    dtgpLogP((const void *)(uintptr_t)object);
    dtgpLog((uint32_t)visible);
}

static const ZVec3 *BMX_CDECL stubGetPosition(const void *object)
{
    dtgpLog(0x4874a000u);
    dtgpLogP(object);
    return dtgp.position;
}

static void BMX_CDECL stubSetFlags(void *model, int set, int clear)
{
    dtgpLog(0x560ed800u);
    dtgpLogP(model);
    dtgpLog((uint32_t)set);
    dtgpLog((uint32_t)clear);
}

static void BMX_CDECL stubObjectSignal(void *object, int signal)
{
    dtgpLog(0x46661000u);
    dtgpLogP(object);
    dtgpLog((uint32_t)signal);
}

static void BMX_CDECL stubPropPosition(void *object, const ZVec3 *pos)
{
    dtgpLog(0x4662b000u);
    dtgpLogP(object);
    dtgpLogN(pos, 3);
}

static void BMX_CDECL stubPropRotation(void *object, const int16_t *rot)
{
    dtgpLog(0x4661c000u);
    dtgpLogP(object);
    dtgpLog((uint16_t)rot[0]);
    dtgpLog((uint16_t)rot[1]);
    dtgpLog((uint16_t)rot[2]);
}

static void BMX_CDECL stubPropSwitch(void *prop, uint32_t player, int release)
{
    dtgpLog(0x42f0d000u);
    dtgpLogP(prop);
    dtgpLog(player);
    dtgpLog((uint32_t)release);
}

static int BMX_CDECL stubPropGoal(int player, void *prop)
{
    dtgpLog(0x40ca6000u);
    dtgpLog((uint32_t)player);
    dtgpLogP(prop);
    return (int)dtgp.goalResult;
}

static int BMX_CDECL stubGrindGoal(int player, const void *edge)
{
    dtgpLog(0x40cbe000u);
    dtgpLog((uint32_t)player);
    dtgpLogP(edge);
    return 0;
}

static void BMX_CDECL stubShards(const ZVec3 *pos, const void *rider)
{
    dtgpLog(0x408fd000u);
    dtgpLogN(pos, 3);
    dtgpLogP(rider);
}

/* ---- screens: text, strings, pads, overlay and game calls --------------------------------------- */

static int BMX_CDECL stubTextWidth(const char *text, const void *font)
{
    dtgpLog(0x59f30000u);
    dtgpLog(dtgpHash(text));
    dtgpLogP(font);
    return (int)((uint32_t)strlen(text) * 9 + 5);
}

static void BMX_CDECL stubDrawText(const void *font, const char *text, int x, int y, uint32_t color,
                                   int flags)
{
    dtgpLog(0x59df0000u);
    dtgpLogP(font);
    dtgpLog(dtgpHash(text));
    dtgpLog((uint16_t)x); /* the original passes the width complement with stale upper bits */
    dtgpLog((uint32_t)y);
    dtgpLog(color);
    dtgpLog((uint32_t)flags);
}

/* A stable text per string id (the real tables are not loaded). */
static const char *BMX_CDECL stubStringGet(uint32_t id)
{
    static char pool[4][256][8];
    char *s = pool[(id >> 12) & 3][id & 0xff];

    dtgpLog(0x24f00000u);
    dtgpLog(id & 0xffff);
    s[0] = 'T';
    for (int i = 0; i < 4; i++) {
        s[1 + i] = "0123456789abcdef"[(id >> (12 - 4 * i)) & 15];
    }
    s[5] = '\0';
    return s;
}

static int BMX_CDECL stubPressed(int player, uint32_t mask, uint32_t status)
{
    static const uint32_t masks[] = {0x20080, 0x4000, 4, 8, 1, 2, 0x10010};
    uint32_t bit = 31;

    dtgpLog(0x08590000u);
    dtgpLog((uint32_t)player);
    dtgpLog(mask);
    dtgpLog(status);
    for (uint32_t i = 0; i < sizeof masks / sizeof masks[0]; i++) {
        if (masks[i] == mask) {
            bit = ((uint32_t)player & 1) * 8 + i;
        }
    }
    return (int)(dtgp.extra[3] >> bit & 1);
}

static void BMX_CDECL stubSetScreen(uint32_t id)
{
    dtgpLog(0x15400000u);
    dtgpLog(id);
}

static int BMX_CDECL stubPadConfirm(uint32_t minItem)
{
    dtgpLog(0x1e920000u);
    dtgpLog(minItem);
    return (int)(dtgp.extra[4] & 1);
}

static void BMX_CDECL stubMenuItems(uint32_t flags, int offset)
{
    dtgpLog(0x1e7f0000u);
    dtgpLog(flags);
    dtgpLog((uint32_t)offset);
}

static uint32_t BMX_CDECL stubPulse(void)
{
    dtgpLog(0x1e9c0000u);
    return 0x2123456u;
}

static void BMX_CDECL stubPlayerNames(int *y)
{
    dtgpLog(0x14580000u);
    dtgpLog((uint32_t)*y);
    *y += 0x35;
}

static int BMX_CDECL stubStartMode(int mode)
{
    dtgpLog(0x0a8c0000u);
    dtgpLog((uint32_t)mode);
    return mode;
}

static uint32_t BMX_CDECL stubSetSubMode(uint32_t sub)
{
    dtgpLog(0x0aa10000u);
    dtgpLog(sub);
    return sub;
}

static uint32_t BMX_CDECL stubBigGame(int state)
{
    dtgpLog(0x0aa50000u);
    dtgpLog((uint32_t)state);
    return 0;
}

static char *BMX_CDECL stubHorseWord(const char *word)
{
    dtgpLog(0x0bfd0000u);
    dtgpLog(dtgpHash(word));
    dtgpLogP(word);
    return (char *)word;
}

static uint32_t BMX_CDECL stubActionMode(uint32_t mode)
{
    dtgpLog(0x04f40000u);
    dtgpLog(mode);
    return 0;
}

typedef struct StubDef {
    uint32_t orig;
    void *stub;
    uint32_t group;
} StubDef;

void dtgpBegin(const DtCtx *ctx, uint32_t groups)
{
    static const StubDef defs[] = {
        {0x0047ed10, stubFail, DTGP_DEBUG},         {0x00475a30, stubActorShow, DTGP_ACTOR},
        {0x00475a60, stubActorHide, DTGP_ACTOR},    {0x0045a1f0, stubHorizonOn, DTGP_ACTOR},
        {0x0045a220, stubHorizonOff, DTGP_ACTOR},   {0x004078c0, stubSetView, DTGP_ACTOR},
        {0x0044e300, stubSoundAt, DTGP_SOUND},      {0x004822a0, stubSetFov, DTGP_CAMERA},
        {0x004822c0, stubLookAt, DTGP_CAMERA},      {0x004623a0, stubFillBar, DTGP_CAMERA},
        {0x0045f480, stubFxNode, DTGP_FX},          {0x0045fcb0, stubFxCreate, DTGP_FX},
        {0x00482fc0, stubNameCall, DTGP_FX},        {0x0045fdc0, stubPsxSetup, DTGP_FX},
        {0x0045a300, stubRegisterTable, DTGP_FX},   {0x0045a2d0, stubDestroyAll, DTGP_FX},
        {0x0045fc10, stubFxShutdown, DTGP_FX},      {0x0045f780, stubFxRegisterHandlers, DTGP_FX},
        {0x00485e40, stubStaticMark, DTGP_ZMEM},    {0x00485d80, stubStaticAlloc, DTGP_ZMEM},
        {0x00485e10, stubStaticRelease, DTGP_ZMEM}, {0x0040a9f0, stubGetMode, DTGP_GAME},
        {0x0040aa30, stubGetSubMode, DTGP_GAME},    {0x00407fc0, stubGoalInfo, DTGP_GAME},
        {0x00487600, stubSetVisible, DTGP_PROP},    {0x004874a0, stubGetPosition, DTGP_PROP},
        {0x00466610, stubObjectSignal, DTGP_PROP},  {0x004662b0, stubPropPosition, DTGP_PROP},
        {0x004661c0, stubPropRotation, DTGP_PROP},  {0x0042f0d0, stubPropSwitch, DTGP_PROP},
        {0x0040ca60, stubPropGoal, DTGP_PROP},      {0x0040cbe0, stubGrindGoal, DTGP_PROP},
        {0x00408fd0, stubShards, DTGP_PROP},        {0x00459f30, stubTextWidth, DTGP_UI},
        {0x00459df0, stubDrawText, DTGP_UI},        {0x004824f0, stubStringGet, DTGP_UI},
        {0x00408590, stubPressed, DTGP_UI},         {0x00415400, stubSetScreen, DTGP_UI},
        {0x0041e920, stubPadConfirm, DTGP_UI},      {0x0041e7f0, stubMenuItems, DTGP_UI},
        {0x0041e9c0, stubPulse, DTGP_UI},           {0x00414580, stubPlayerNames, DTGP_UI},
        {0x0040a8c0, stubStartMode, DTGP_UI},       {0x0040aa10, stubSetSubMode, DTGP_UI},
        {0x0040aa50, stubBigGame, DTGP_UI},         {0x0040bfd0, stubHorseWord, DTGP_UI},
        {0x00404f40, stubActionMode, DTGP_UI},      {0x00480370, stubPadCreate, DTGP_PAD},
        {0x004803c0, stubPadDestroy, DTGP_PAD},
    };

    dt_seed(ctx);
    dtgpLogCount = 0;
    memset(&dtgp, 0, sizeof dtgp);
    for (unsigned i = 0; i < sizeof defs / sizeof defs[0]; i++) {
        if ((defs[i].group & groups) != 0) {
            dt_stub(defs[i].orig, NULL, defs[i].stub);
        }
    }
    if ((groups & DTGP_DEBUG) != 0) {
        gZdebugPrint = stubPrint;
    }
    if ((groups & DTGP_PROP) != 0) {
        gZobjSetFlags = stubSetFlags;
    }
}
