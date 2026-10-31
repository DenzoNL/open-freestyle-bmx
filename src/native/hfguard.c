/*
 * BMX_HF_GUARD (docs/HIGHFPS.md "HF0 findings"): a dev aid that measures, for the high frame rate design,
 * what a draw (zrwDrawFrame 0x48b690) changes and how the loop paces. Native exe only; main.c calls
 * bmxHfGuardFrame instead of onIdle + zrwDrawFrame when the variable is set, and nothing else changes.
 *
 *   1  sub-hashes of the state before and after every draw; per sub-hash the number of draws that changed it.
 *      Pacing per tick: interval, onIdle time, draw time, swapchain wait, slack. The effect quality (the far
 *      clip, ngiffxAdaptQuality) is logged whenever it changes.
 *   2  as 1, but the draw runs through a copy of zrwDrawFrame that hashes between steps and list nodes, so a
 *      change is attributed to the node (its callback address) or step that made it.
 *   3  as 2, plus in a level a second draw per tick through the copy (screen shake node skipped, HUD flush
 *      re-armed, no list 3, no present), then the identity recompute of every model (zrwModelUpdate through
 *      its slot, pool order) with a frame sync: does restore-by-recompute keep frames and sector ties?
 *      Level 3 changes the run (extra syncs re-tie atomics); compare its BMX_SIM_HASH with a level 0 run.
 *   4  as 3, but the recompute's dirty frames are synced in the order the tick queued them (recorded after
 *      onIdle): the restore that keeps RenderWare's sector tie order (HF2).
 *
 * Sub-hashes only read memory (no getters that sync or move list cursors). Summaries every 900 ticks and
 * before BMX_QUIT_FRAMES ends the process.
 */
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bmx/com/zge/ngif/ngiffx.h"
#include "bmx/com/zge/ngif/ngifpsx.h"
#include "compat/orig.h"
#include "native/native.h"
#include "rw/rwcamera.h"
#include "rw/rwcore.h"
#include "rw/rwplugins.h"
#include "rw/rwtexture.h"
#include "rw/rwworld.h"
#include "zge/com/zglrw/zrwcol.h"
#include "zge/com/zglrw/zrwmodel.h"
#include "zge/com/zglrw/zrwshadow.h"
#include "zge/com/zglrw/zrwworld.h"
#include "zge/zlist.h"
#include "zge/zmath.h"
#include "zge/zmem.h"
#include "zge/zobj.h"

int drvGpuShownFrames(void);
void drvGpuMeasureAcquire(void);
uint64_t drvGpuAcquireNs(void);
void zrwResetRenderState(void);

/* zrwzgl.h cannot be included next to zrwmodel.h; the update lists' layout (see zrwzgl.h). */
typedef struct GuardNode {
    struct GuardNode *next, *prev;
    void *list;
    int(BMX_CDECL *fn)(void *arg);
    void *arg;
} GuardNode;
typedef struct GuardList {
    GuardNode *head, *tail;
    int count;
} GuardList;
typedef GuardList GuardLists[9];
#define gLists      ORIG_VAR(0x00672768, GuardLists, gLists)
#define gRaster     ORIG_VAR(0x006976c0, void *, gRaster)
#define gDebugLight ORIG_VAR(0x006976e8, int, gDebugLight)
#define gClearOn    ORIG_VAR(0x0069772c, int, gClearOn)
#define gClearColor ORIG_VAR(0x00697730, uint32_t, gClearColor)
#define gWindow     ORIG_VAR(0x00697780, void *, gWindow)
#define gFrameLimit ORIG_VAR(0x0069771c, float, gFrameLimit)
#define gNgMode     ORIG_VAR(0x005ab3a0, uint8_t, gNgMode)
#define gNgCamera   ORIG_VAR(0x0068ff98, const ZObj *, gNgCamera)
#define gZobjPool   ORIG_VAR(0x006722d4, ZMemFreeList *, gZobjPool)
#define gFxQuality  ORIG_VAR(0x0055bd08, float, gFxQuality)
#define gFxFps      ORIG_VAR(0x0055bd0c, float, gFxFps)
ORIG_FN(0x0048b570, int, onIdle, (void));
ORIG_FN(0x0048b690, int, drawFrame, (void));
ORIG_FN(0x00492eb0, void *, zrwcamFirst, (void));
ORIG_FN(0x00492ed0, void *, zrwcamNext, (void *camera));
ORIG_FN(0x00492f00, int, zrwcamBegin, (void *camera));
ORIG_FN(0x00492fe0, int, zrwcamEnd, (void));
ORIG_FN(0x00493060, void *, zrwcamHandle, (void *camera));
ORIG_FN(0x00497620, int, zrwtxrFlush, (void));

enum { kShakeNode = 0x00401a4b, kEffectOps = 0x00562098 };
enum {
    kSim,
    kRng,
    kFrames,
    kTies,
    kColBox,
    kColState,
    kShadow,
    kParticles,
    kHud,
    kCamera,
    kLightFrame,
    kSubs
};
static const char *const kSubName[kSubs] = {"sim",    "rng",      "frames",    "ties",
                                            "colbox", "colstate", "shadow",    "particles",
                                            "hud",    "camera",   "lightframe"};
/* Steps of the draw that are not list nodes ("where" values below 0x100). */
static const char *const kStepName[] = {"draw",   "clear", "camBegin", "txrFlush",
                                        "camEnd", "show",  "draw2",    "recompute"};
enum {
    kWhereDraw,
    kWhereClear,
    kWhereBegin,
    kWhereFlush,
    kWhereEnd,
    kWhereShow,
    kWhereDraw2,
    kWhereRecompute
};

typedef struct Hashes {
    uint64_t h[kSubs];
} Hashes;

enum { kMaxWhere = 48 };
static int gLevel;
static uint32_t gTicks, gDraws;
static uint32_t gWhere[kMaxWhere];
static uint32_t gChanges[kSubs][kMaxWhere];
static uint32_t gFirstTick[kSubs][kMaxWhere];
static int gWheres;
static Hashes gLast;

/* Pacing of the ticks in a level (limit 30) and in the frontend (60). */
typedef struct Pace {
    uint32_t n;
    double interval, idle, draw, wait, slack, minSlack, maxInterval, maxDraw;
} Pace;
static Pace gPace[2];
static uint64_t gLastStart;
static float gLastQuality;
static uint32_t gSecond, gTieMoves, gFrameMoves, gExtraDirty, gEmitters, gMaxParticles;

/* Level 4: the dirty frame list as the tick left it (head first), replayed before the recompute's sync. */
enum { kMaxDirty = 512 };
static RwFrame *gDirty[kMaxDirty];
static int gDirtyCount;

static RwFrame *dirtyFrame(RwLLLink *link)
{
    return (RwFrame *)(void *)((uint8_t *)link - offsetof(RwFrame, dirtyLink));
}

static void recordDirty(void)
{
    RwLLLink *const head = &RwEngineInstance->dirtyFrameList;

    gDirtyCount = 0;
    for (RwLLLink *link = head->next; link != head && gDirtyCount < kMaxDirty; link = link->next) {
        gDirty[gDirtyCount++] = dirtyFrame(link);
    }
}

/* Puts the queued frames in the recorded order, followed by those the tick did not queue (counted). */
static void replayDirty(void)
{
    static RwFrame *order[2 * kMaxDirty];
    RwLLLink *const head = &RwEngineInstance->dirtyFrameList;
    int n = 0;

    for (int i = 0; i < gDirtyCount; i++) {
        if ((gDirty[i]->object.privateFlags & 3) != 0) {
            order[n++] = gDirty[i];
        }
    }
    for (RwLLLink *link = head->next; link != head && n < 2 * kMaxDirty; link = link->next) {
        int known = 0;

        for (int i = 0; i < gDirtyCount && !known; i++) {
            known = gDirty[i] == dirtyFrame(link);
        }
        if (!known) {
            order[n++] = dirtyFrame(link);
            gExtraDirty++;
        }
    }
    head->next = head->prev = head;
    for (int i = 0; i < n; i++) {
        order[i]->dirtyLink.prev = head->prev;
        order[i]->dirtyLink.next = head;
        head->prev->next = &order[i]->dirtyLink;
        head->prev = &order[i]->dirtyLink;
    }
}

static uint64_t fnv(uint64_t h, const void *data, size_t size)
{
    const uint8_t *p = data;

    for (size_t i = 0; i < size; i++) {
        h = (h ^ p[i]) * 0x100000001b3ull;
    }
    return h;
}

/* One part of a matrix: 0 the twelve floats, 1 the flags word, 2 the three padding words. */
static uint64_t hashMatrix(uint64_t h, const RwMatrix *m, int part)
{
    const ZVec3 *const rows[4] = {&m->right, &m->up, &m->at, &m->pos};
    const uint32_t *const pads[3] = {&m->pad1, &m->pad2, &m->pad3};

    for (int i = 0; part == 0 && i < 4; i++) {
        h = fnv(h, rows[i], sizeof *rows[i]);
    }
    for (int i = 0; part == 2 && i < 3; i++) {
        h = fnv(h, pads[i], 4);
    }
    return part == 1 ? fnv(h, &m->flags, 4) : h;
}

/* The modelling and world matrices of a frame list and its subtrees. */
/* A pointer or word, by value. */
static uint64_t fnvWord(uint64_t h, uintptr_t word)
{
    const uint32_t v = (uint32_t)word;

    return fnv(h, &v, sizeof v);
}

static uint64_t hashFrames(uint64_t h, const RwFrame *frame, int part)
{
    for (; frame != NULL; frame = frame->next) {
        h = hashMatrix(h, &frame->modelling, part);
        h = hashMatrix(h, &frame->ltm, part);
        h = hashFrames(h, frame->child, part);
    }
    return h;
}

/* A type 0 object zobjUpdateAll updates (active, not attached) with its model; NULL otherwise. */
static ZrwModel *modelOf(const ZObj *object)
{
    if ((object->flags & 7) != 0 || (object->flags & kZobjActive) == 0 ||
        (object->flags & kZobjAttached) != 0) {
        return NULL;
    }
    return object->model;
}

static uint64_t hashList(uint64_t h, const RwLLLink *head)
{
    for (const RwLLLink *link = head->next; link != head; link = link->next) {
        h = fnvWord(h, (uintptr_t)((const RpTie *)link)->apAtom);
    }
    return h;
}

static void *BMX_CDECL tieSector(void *sector, void *data)
{
    const RpWorldSector *s = sector;
    uint64_t *h = data;

    *h = hashList(*h, &s->collAtomicsInWorldSector);
    *h = hashList(*h, &s->noCollAtomicsInWorldSector);
    return sector;
}

static void hashAll(Hashes *out)
{
    const uint64_t seed = 0xcbf29ce484222325ull;

    for (int i = 0; i < kSubs; i++) {
        out->h[i] = seed;
    }
    out->h[kSim] = bmxSimStateHash();
    out->h[kRng] = fnv(seed, &gZmathTables.randomSeed, 4);
    for (ZMemFreeNode *node = zmemFreeListFirst(gZobjPool); node != NULL; node = zmemFreeNodeNext(node)) {
        const ZObj *object = zmemFreeNodePayload(node);
        const ZrwModel *model = modelOf(object);

        if (model != NULL) {
            out->h[kFrames] = fnv(out->h[kFrames], &object->pos, 0x20); /* pos, rot */
            out->h[kFrames] = fnv(out->h[kFrames], &model->position, 0x32);
            for (int part = 0; part < 2; part++) { /* floats and flags; padding words are not state */
                out->h[kFrames] =
                    hashFrames(out->h[kFrames], model->clump ? RpClumpGetFrame(model->clump) : NULL, part);
                out->h[kFrames] =
                    hashFrames(out->h[kFrames],
                               model->collisionClump ? RpClumpGetFrame(model->collisionClump) : NULL, part);
            }
        } else if (object->ops == (const ZObjOps *)(uintptr_t)kEffectOps) {
            const ZList *items = (*(ZrwEffectHost *const *)&object->pad30[0])->items;

            gEmitters = (uint32_t)items->count > gEmitters ? (uint32_t)items->count : gEmitters;
            for (int i = 0; i < items->count; i++) {
                const ZfxEmitter *e = items->items[i];

                if (e->particles != NULL && (uint32_t)e->particles->count > gMaxParticles) {
                    gMaxParticles = (uint32_t)e->particles->count;
                }
                out->h[kParticles] = fnv(out->h[kParticles], &e->min, 0x24);
                for (int j = 0; e->particles != NULL && j < e->particles->count; j++) {
                    out->h[kParticles] = fnv(out->h[kParticles], e->particles->items[j], 0x60);
                }
            }
        }
    }
    if (gZrwWorld != NULL) {
        RpWorldForAllWorldSectors(gZrwWorld, tieSector, &out->h[kTies]);
    }
    out->h[kColBox] = fnv(seed, &gZrwColBox, sizeof gZrwColBox);
    out->h[kColState] = fnv(seed, &gZrwCol, sizeof gZrwCol);
    out->h[kShadow] = fnv(seed, &gZrwShadowReady, 4);
    out->h[kShadow] = fnv(out->h[kShadow], &gZrwShadowMiss, sizeof gZrwShadowMiss);
    out->h[kShadow] = fnv(out->h[kShadow], &gZrwShadowVertices, sizeof gZrwShadowVertices);
    out->h[kHud] = fnv(seed, &gNgifFlushPending, 4);
    out->h[kHud] = fnv(out->h[kHud], &gNgifFlushPending2, 4);
    out->h[kHud] = fnvWord(out->h[kHud], (uintptr_t)gNgifPrim3D);
    out->h[kHud] = fnvWord(out->h[kHud], (uintptr_t)gNgifScript);
    if (gNgifOtBase != NULL) {
        out->h[kHud] = fnv(out->h[kHud], gNgifOtBase, 0x400 * 4);
    }
    if (gNgCamera != NULL) {
        out->h[kCamera] = fnv(seed, &gNgCamera->pos, 0x20);
    }
    out->h[kLightFrame] = fnv(seed, &RwEngineInstance->lightFrame, 2);
}

static int whereSlot(uint32_t where)
{
    for (int i = 0; i < gWheres; i++) {
        if (gWhere[i] == where) {
            return i;
        }
    }
    if (gWheres == kMaxWhere) {
        return kMaxWhere - 1;
    }
    gWhere[gWheres] = where;
    return gWheres++;
}

/* Hashes the state and counts every sub-hash that differs from the last checkpoint against `where`. */
static void checkpoint(uint32_t where)
{
    Hashes now;

    hashAll(&now);
    for (int i = 0; i < kSubs; i++) {
        if (now.h[i] != gLast.h[i]) {
            const int slot = whereSlot(where);

            if (gChanges[i][slot]++ == 0) {
                gFirstTick[i][slot] = gTicks;
            }
        }
    }
    gLast = now;
}

static void whereName(char *out, size_t size, uint32_t where)
{
    if (where < sizeof kStepName / sizeof kStepName[0]) {
        snprintf(out, size, "%s", kStepName[where]);
    } else {
        snprintf(out, size, "node %08x", (unsigned)where);
    }
}

static void runList(int index, int second)
{
    for (GuardNode *node = gLists[index].head; node != NULL;) {
        GuardNode *const next = node->next;
        const uint32_t fn = (uint32_t)(uintptr_t)node->fn;

        if (!(second && fn == kShakeNode)) {
            const int ok = node->fn(node->arg);

            checkpoint(fn);
            if (ok == 0) {
                return; /* the original prints "failed call" and leaves the list */
            }
        }
        node = next;
    }
}

/* zrwDrawFrame (0x48b690) with checkpoints; `second`: the dry run (no shake, no list 3, no present). */
static int guardDraw(int second)
{
    if (gRaster == NULL) {
        return 1;
    }
    if (gClearOn != 0) {
        for (void *camera = zrwcamFirst(); camera != NULL; camera = zrwcamNext(camera)) {
            if ((((const uint8_t *)camera)[8] & 4) != 0) {
                RwCameraClear(zrwcamHandle(camera), &gClearColor, 3);
            }
        }
        checkpoint(kWhereClear);
    }
    runList(0, second);
    for (void *camera = zrwcamFirst(); camera != NULL; camera = zrwcamNext(camera)) {
        RwError error;

        if ((((const uint8_t *)camera)[8] & 4) == 0) {
            continue;
        }
        RwErrorGet(&error);
        if (zrwcamBegin(camera) == 0) {
            continue;
        }
        checkpoint(kWhereBegin);
        runList(1, second);
        if (zrwtxrFlush() == 0) {
            return 0;
        }
        checkpoint(kWhereFlush);
        zrwResetRenderState();
        runList(2, second);
        zrwResetRenderState();
        runList(5, second);
        runList(4, second);
        if (zrwcamEnd() == 0) {
            return 0;
        }
        checkpoint(kWhereEnd);
        RwErrorGet(&error);
    }
    if (!second) {
        runList(3, second);
        RwRasterShowRaster(gRaster, gWindow, 1);
        checkpoint(kWhereShow);
    }
    return 1;
}

/* Per pool slot: hashes of a model's frames (floats, flags, padding words: hashMatrix). */
enum { kMaxObjects = 0x100 };
static void objectHashes(uint64_t (*out)[3])
{
    int n = 0;

    for (ZMemFreeNode *node = zmemFreeListFirst(gZobjPool); node != NULL && n < kMaxObjects;
         node = zmemFreeNodeNext(node), n++) {
        const ZrwModel *model = modelOf(zmemFreeNodePayload(node));
        const RwFrame *root = model != NULL && model->clump != NULL ? RpClumpGetFrame(model->clump) : NULL;

        for (int part = 0; part < 3; part++) {
            out[n][part] = hashFrames(0, root, part);
        }
    }
}

static const ZrwModel *modelAt(int index)
{
    int n = 0;

    for (ZMemFreeNode *node = zmemFreeListFirst(gZobjPool); node != NULL; node = zmemFreeNodeNext(node)) {
        if (n++ == index) {
            return modelOf(zmemFreeNodePayload(node));
        }
    }
    return NULL;
}

/* Level 3: the identity recompute of every model in pool order plus a sync; counts what it moved. */
static void recompute(void)
{
    static uint64_t a[kMaxObjects][3], b[kMaxObjects][3];
    static int logged;
    Hashes before;

    hashAll(&before);
    objectHashes(a);
    for (ZMemFreeNode *node = zmemFreeListFirst(gZobjPool); node != NULL; node = zmemFreeNodeNext(node)) {
        ZrwModel *model = modelOf(zmemFreeNodePayload(node));

        if (model != NULL) {
            gZrwVtUpdate(model);
        }
    }
    if (gLevel >= 4) {
        replayDirty();
    }
    rwFrameSyncDirtyList();
    checkpoint(kWhereRecompute);
    objectHashes(b);
    for (int i = 0; i < kMaxObjects && logged < 12; i++) {
        if (a[i][0] != b[i][0] || a[i][1] != b[i][1] || a[i][2] != b[i][2]) {
            const ZrwModel *model = modelAt(i);

            bmxLog("hfguard: recompute tick %u slot %d (%.16s, flags %x, lod %u): floats %s, flags %s, "
                   "padding %s\n",
                   (unsigned)gTicks, i, model != NULL && model->def != NULL ? model->def->name : "?",
                   model != NULL ? (unsigned)model->flags : 0u,
                   model != NULL ? (unsigned)model->activeLod : 0u, a[i][0] != b[i][0] ? "DIFF" : "same",
                   a[i][1] != b[i][1] ? "DIFF" : "same", a[i][2] != b[i][2] ? "DIFF" : "same");
            logged++;
        }
    }
    gFrameMoves += gLast.h[kFrames] != before.h[kFrames];
    gTieMoves += gLast.h[kTies] != before.h[kTies];
}

static void summary(void)
{
    static const char *const kMode[2] = {"level", "frontend"};

    bmxLog(
        "hfguard: summary after %u ticks, %u draws (level %d); %u effect emitters, at most %u live particles "
        "in one\n",
        (unsigned)gTicks, (unsigned)gDraws, gLevel, (unsigned)gEmitters, (unsigned)gMaxParticles);
    for (int i = 0; i < kSubs; i++) {
        for (int w = 0; w < gWheres; w++) {
            char name[32];

            if (gChanges[i][w] == 0) {
                continue;
            }
            whereName(name, sizeof name, gWhere[w]);
            bmxLog("hfguard:   %-10s changed by %-14s %6u times (first at tick %u)\n", kSubName[i], name,
                   (unsigned)gChanges[i][w], (unsigned)gFirstTick[i][w]);
        }
    }
    for (int m = 0; m < 2; m++) {
        const Pace *p = &gPace[m];

        if (p->n == 0) {
            continue;
        }
        bmxLog("hfguard:   pace %-8s %u ticks: interval %.2f ms (max %.2f), idle %.2f, draw %.2f (max %.2f; "
               "swapchain wait %.2f), slack %.2f (min %.2f)\n",
               kMode[m], (unsigned)p->n, p->interval / p->n, p->maxInterval, p->idle / p->n, p->draw / p->n,
               p->maxDraw, p->wait / p->n, p->slack / p->n, p->minSlack);
    }
    if (gLevel >= 3) {
        bmxLog("hfguard:   second draws %u; recompute changed frames %u times, sector ties %u times (frames "
               "outside the tick's dirty list: %u)\n",
               (unsigned)gSecond, (unsigned)gFrameMoves, (unsigned)gTieMoves, (unsigned)gExtraDirty);
    }
}

int bmxHfGuardFrame(void)
{
    const char *const quit = getenv("BMX_QUIT_FRAMES");
    const uint64_t start = SDL_GetTicksNS();
    const uint64_t waited = drvGpuAcquireNs();
    const int level = gNgMode == 2 && gFrameLimit == 30.0f;
    uint64_t idleEnd, drawStart, drawEnd;
    int flush, flush2;
    int ok;

    if (!onIdle()) {
        return 0;
    }
    flush = gNgifFlushPending; /* as the tick left them, for the second draw */
    flush2 = gNgifFlushPending2;
    idleEnd = SDL_GetTicksNS();
    if (gLevel >= 4) {
        recordDirty();
    }
    gTicks++;
    if (gFxQuality != gLastQuality) {
        int skids = 0;

        while (skids < kFxSkidEntries && gFxSkids[skids].object != NULL) {
            skids++;
        }
        bmxLog("hfguard: tick %u quality (far clip) %.4f fps %.2f (mode %d, %d ANIM objects)\n",
               (unsigned)gTicks, (double)gFxQuality, (double)gFxFps, gNgMode, skids);
        gLastQuality = gFxQuality;
    }
    if (quit != NULL && drvGpuShownFrames() + 1 >= strtol(quit, NULL, 10)) {
        summary();
    }
    hashAll(&gLast);
    drawStart = SDL_GetTicksNS(); /* level 1 times the draw alone; levels 2.. include the checkpoints */
    if (gLevel >= 2 && gDebugLight == 0) {
        ok = guardDraw(0);
        drawEnd = SDL_GetTicksNS();
    } else {
        ok = drawFrame();
        drawEnd = SDL_GetTicksNS();
        checkpoint(kWhereDraw);
    }
    gDraws++;
    if (ok && gLevel >= 3 && level && gDebugLight == 0) {
        gNgifFlushPending = flush; /* re-armed as HF3 would */
        gNgifFlushPending2 = flush2;
        guardDraw(1);
        gSecond++;
        recompute();
    }
    if (gLastStart != 0 && start - gLastStart < 200000000u) {
        Pace *p = &gPace[level ? 0 : 1];
        const double interval = (double)(start - gLastStart) * 1e-6;
        const double idle = (double)(idleEnd - start) * 1e-6;
        const double draw = (double)(drawEnd - drawStart) * 1e-6;
        const double slack = interval - idle - draw;

        p->interval += interval;
        p->idle += idle;
        p->draw += draw;
        p->wait += (double)(drvGpuAcquireNs() - waited) * 1e-6;
        p->slack += slack;
        p->minSlack = p->n == 0 || slack < p->minSlack ? slack : p->minSlack;
        p->maxInterval = interval > p->maxInterval ? interval : p->maxInterval;
        p->maxDraw = draw > p->maxDraw ? draw : p->maxDraw;
        p->n++;
    }
    gLastStart = start;
    if (gTicks % 900 == 0) {
        summary();
    }
    return ok;
}

int bmxHfGuardInstall(void)
{
    const char *const value = getenv("BMX_HF_GUARD");
    const SDL_DisplayMode *mode;

    if (value == NULL || *value == 0 || *value == '0') {
        return 0;
    }
    gLevel = (int)strtol(value, NULL, 10);
    gLevel = gLevel < 1 ? 1 : (gLevel > 4 ? 4 : gLevel);
    drvGpuMeasureAcquire();
    mode = SDL_GetCurrentDisplayMode(SDL_GetPrimaryDisplay());
    bmxLog("hfguard: level %d, display refresh %.2f Hz\n", gLevel,
           mode != NULL ? (double)mode->refresh_rate : 0.0);
    return gLevel;
}
