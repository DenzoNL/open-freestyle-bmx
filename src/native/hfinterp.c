/*
 * High frame rate: fixed tick, interpolated rendering (docs/HIGHFPS.md, slices HF1-HF3). Native exe only.
 *
 * With `[video] interpolate = 1` main.c hands its loop to bmxHfStep. Outside a level (gNgMode != 2 or a limit
 * other than 30: the frontend, loading) the step is the original loop: the same limiter, a busy wait, onIdle
 * and zrwDrawFrame as a pair. In a level:
 *
 * - **Ticks** keep the original condition (hfLimiterTick, the same clock and arithmetic as main.c). Before a
 *   tick the poses of the last draw are restored (below); after it the tick is captured: for every model
 *   object zobjUpdateAll updated (its walk), the ZObj pos / rot, the ZrwModel position / rotation / scale and
 *   the active LOD's part offsets and rotations; for the camera the view the tick handed to the renderer
 *   camera. Also recorded: the dirty frame list (head to tail) and the HUD's two flush flags.
 * - **Draws.** The first draw after a tick is the real zrwDrawFrame (counted, every node once per tick as in
 *   vanilla). Extra draws run while they cannot delay the next tick (hfSchedNext: the worst of the recent
 *   draw times plus a margin must end before the tick is due) through hfDrawFrame, a copy of zrwDrawFrame
 *   (0x48b690) that skips the screen shake node, re-arms the flush flags, wraps the shadow probes in a save /
 *   restore of the collision state they touch, skips the debug list 4 and presents without counting a frame.
 *   Before every draw the poses are applied at alpha = (now - tick) / tick length: one tick behind, lerp and
 *   nlerp between the last two captures (hfmath.c), written raw into the fields, then zrwModelUpdate through
 *   its slot per model, and the interpolated view handed to the renderer camera (gZrwCamSetView). Objects
 *   and the camera that break a skip rule (H6) keep the tick pose.
 * - **Restore** (before the next tick): the tick values written back, every captured model recomputed through
 *   the slot, the tick's view handed to the renderer camera again, the dirty list relinked in the recorded
 *   order (frames not in the record after them) and rwFrameSyncDirtyList: the frames, skin and collision
 *   clump matrices and the world sectors' tie order are the tick's again (HF0 findings, Q3).
 * - **Waiting** sleeps (SDL_DelayNS) up to 2 ms before the tick is due, then spins on the limiter.
 *
 * Dev aids: BMX_HF_ALPHA=<a> draws every frame at alpha a (1: always the tick pose, for byte identity with
 * the option off); BMX_HF_LOG=1 logs pacing, skips and per-tick motion maxima (the H6 thresholds); with
 * BMX_HF_GUARD set, the guard (hfguard.c) hashes the state around every extra draw, and the restore is
 * checked against the capture.
 */
#include <SDL3/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bmx/com/action/camera.h"
#include "bmx/com/zge/ngif/ngifpsx.h"
#include "compat/orig.h"
#include "native/hfmath.h"
#include "native/native.h"
#include "platform/platform.h"
#include "rw/rwcamera.h"
#include "rw/rwcore.h"
#include "rw/rwmath.h"
#include "rw/rwplugins.h"
#include "rw/rwtexture.h"
#include "zge/com/zglrw/zrwcam.h"
#include "zge/com/zglrw/zrwcol.h"
#include "zge/com/zglrw/zrwmodel.h"
#include "zge/zgemods/zobjcamera.h"
#include "zge/zmem.h"
#include "zge/zobj.h"
#include "zge/zquat.h"

int drvGpuShownFrames(void);
void drvGpuSetUncounted(int uncounted);
void zrwResetRenderState(void);

/* zrwzgl.h cannot be included next to zrwmodel.h; the update lists' layout (see zrwzgl.h). */
typedef struct HfNode {
    struct HfNode *next, *prev;
    void *list;
    int(BMX_CDECL *fn)(void *arg);
    void *arg;
} HfNode;
typedef struct HfList {
    HfNode *head, *tail;
    int count;
} HfList;
typedef HfList HfLists[9];
#define gLists      ORIG_VAR(0x00672768, HfLists, gLists)
#define gRaster     ORIG_VAR(0x006976c0, void *, gRaster)
#define gDebugLight ORIG_VAR(0x006976e8, int, gDebugLight)
#define gClearOn    ORIG_VAR(0x0069772c, int, gClearOn)
#define gClearColor ORIG_VAR(0x00697730, uint32_t, gClearColor)
#define gWindow     ORIG_VAR(0x00697780, void *, gWindow)
#define gNgMode     ORIG_VAR(0x005ab3a0, uint8_t, gNgMode)
#define gNgCamera   ORIG_VAR(0x0068ff98, ZObj *, gNgCamera)
#define gZobjPool   ORIG_VAR(0x006722d4, ZMemFreeList *, gZobjPool)
#define gLastTick   ORIG_VAR(0x0067275c, uint32_t, gLastTick)
#define gRateA      ORIG_VAR(0x0069771c, float, gRateA)
#define gRateB      ORIG_VAR(0x0054d05c, float, gRateB)
#define gRateC      ORIG_VAR(0x0054dbb8, float, gRateC)
#define gFxQuality  ORIG_VAR(0x0055bd08, float, gFxQuality)
ORIG_FN(0x0048b570, int, onIdle, (void));
ORIG_FN(0x0048b690, int, drawFrame, (void));
ORIG_FN(0x00492eb0, void *, zrwcamFirst, (void));
ORIG_FN(0x00492ed0, void *, zrwcamNext, (void *camera));
ORIG_FN(0x00492f00, int, zrwcamBegin, (void *camera));
ORIG_FN(0x00492fe0, int, zrwcamEnd, (void));
ORIG_FN(0x00493060, void *, zrwcamHandle, (void *camera));
ORIG_FN(0x00497620, int, zrwtxrFlush, (void));

/* Update list nodes the extra draws treat specially (node->fn, HF0 findings Q2). */
enum {
    kShakeNode = 0x00401a4b,  /* ngifpsxUpdate: the screen shake script advances per call */
    kShadowProbe = 0x0049b520 /* zrollerUpdateShadow: 36 line probes (gZrwColBox, gZrwCol, lightFrame) */
};

/* ZrwCamera (zrwcam.h): the RwCamera frame's matrix copy and the view (rotation, eye) zrwCameraSetView
 * keeps. */
enum { kZrwCameraWorld = 0x14, kZrwCameraView = 0x54 };

enum { kMaxModels = 0x100, kMaxParts = 0x1000, kMaxDirty = 512 };
static const uint64_t kMarginNs = 1500000; /* kept free before a tick on top of the worst draw */
static const uint64_t kSpinNs = 2000000;   /* the last stretch before a tick is spun, not slept */

typedef struct HfModel {
    ZObj *object;
    ZrwModel *model;
    void *clump;
    uint16_t lod;
    uint16_t parts; /* captured parts (defs of the active LOD's matched parts) */
    uint32_t firstPart;
    uint32_t modelFlags;
    float objPos[3], objRot[4];
    float pos[3], rot[4], scale[3];
    int prev; /* index in the previous capture, -1: none */
    int snap; /* kHfSnap*: drawn at the tick pose */
} HfModel;

typedef struct HfPart {
    ZrwPartDef *def;
    float offset[3], rot[4];
    int snap;
} HfPart;

typedef struct HfCapture {
    int valid;
    int models, parts;
    uint64_t tickNs;
    HfModel model[kMaxModels];
    HfPart part[kMaxParts];
    ZObj *camera; /* gNgCamera when zobjUpdateAll updated it, else NULL */
    /* the renderer camera's view as the tick's zobjCameraDraw set it (ZrwCamera.view: rotation and eye),
     * its eye and rotation (zquatFromMatrix), and the selected camera state's last eye move */
    ZMatrix camView;
    uint32_t camValid; /* the camera object's cache bits at the capture (BMX_HF_GUARD) */
    float camPos[3], camRot[4], eyeVel[3];
    int camSnap;
} HfCapture;

/* Histogram buckets: moves by powers of two from 1/16 unit, turns by kTurnEdges degrees. */
enum { kHfMoveBuckets = 18, kHfTurnBuckets = 10 };
static const double kTurnEdges[kHfTurnBuckets] = {1, 2, 5, 10, 20, 30, 45, 60, 90, 181};

static void countMove(uint32_t *hist, double move)
{
    int b = 0;

    while (b < kHfMoveBuckets - 1 && !(move < ldexp(1.0, b - 4))) {
        b++;
    }
    hist[b]++;
}

static void countTurn(uint32_t *hist, double turn)
{
    int b = 0;

    while (b < kHfTurnBuckets - 1 && !(turn < kTurnEdges[b])) {
        b++;
    }
    hist[b]++;
}

typedef struct HfStats {
    uint32_t ticks, draws, extra, late, overflow;
    uint32_t overruns, oversleeps; /* extra draws that ended / sleeps that woke after the tick was due */
    uint32_t partSnaps;            /* parts drawn at the tick pose (turned more than objectTurn) */
    double maxOverrunMs, maxOversleepMs;
    uint32_t snaps[kHfSnapGlobal + 1], camSnaps[kHfSnapGlobal + 1];
    uint32_t perTick[10];
    double maxLateMs, maxObjMove, maxObjTurn, maxPartTurn, maxCamMove, maxCamTurn;
    /* per-tick motion of every matched object / the camera, snapped or not (the H6 thresholds) */
    uint32_t objMove[kHfMoveBuckets], objTurn[kHfTurnBuckets], camMove[kHfMoveBuckets],
        camTurn[kHfTurnBuckets];
    uint64_t worstDraw;
} HfStats;

static struct {
    int on, log, guard;
    double alpha; /* BMX_HF_ALPHA, < 0: none */
    uint32_t fps;
    HfSched sched;
    HfCapture cap[2];
    int cur;     /* gHf.cap[cur] is tick N, the other N - 1 */
    int applied; /* poses applied since the last restore */
    int inLevel; /* the last tick ran in a level */
    int flush, flush2;
    uint64_t dueNs; /* the tick's due time as last computed (0: unknown) */
    uint32_t extraThisTick;
    RwFrame *dirty[kMaxDirty];
    int dirtyCount;
    uint64_t restoreHash[7]; /* BMX_HF_GUARD: the capture's hashes, compared after the restore */
    uint32_t restoreChecks, restoreMismatches;
    int logged;
    float quality; /* the far clip last logged */
    HfStats stats;
} gHf;

/* ---- the dirty frame list (hfguard.c's tested recordDirty / replayDirty) ---- */

static RwFrame *dirtyFrame(RwLLLink *link)
{
    return (RwFrame *)(void *)((uint8_t *)link - offsetof(RwFrame, dirtyLink));
}

static void recordDirty(void)
{
    RwLLLink *const head = &RwEngineInstance->dirtyFrameList;

    gHf.dirtyCount = 0;
    for (RwLLLink *link = head->next; link != head && gHf.dirtyCount < kMaxDirty; link = link->next) {
        gHf.dirty[gHf.dirtyCount++] = dirtyFrame(link);
    }
}

/* Puts the queued frames in the recorded order, followed by those the tick did not queue. */
static void replayDirty(void)
{
    static RwFrame *order[2 * kMaxDirty];
    RwLLLink *const head = &RwEngineInstance->dirtyFrameList;
    int n = 0;

    for (int i = 0; i < gHf.dirtyCount; i++) {
        if ((gHf.dirty[i]->object.privateFlags & 3) != 0) {
            order[n++] = gHf.dirty[i];
        }
    }
    for (RwLLLink *link = head->next; link != head && n < 2 * kMaxDirty; link = link->next) {
        int known = 0;

        for (int i = 0; i < gHf.dirtyCount && !known; i++) {
            known = gHf.dirty[i] == dirtyFrame(link);
        }
        if (!known) {
            order[n++] = dirtyFrame(link);
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

/* ---- capture ---- */

static void addModel(HfCapture *c, ZObj *object)
{
    ZrwModel *const model = object->model;
    HfModel *m;

    if ((object->flags & 7) != 0 || model == NULL || object->ops == NULL || object->ops->update == NULL) {
        if (object == gNgCamera && (object->flags & 7) == 3 && object->ops != NULL &&
            object->ops->update != NULL) {
            c->camera = object;
        }
        return;
    }
    if (c->models == kMaxModels) {
        gHf.stats.overflow++;
        return;
    }
    m = &c->model[c->models++];
    m->object = object;
    m->model = model;
    m->clump = model->clump;
    m->lod = model->activeLod;
    m->modelFlags = model->flags;
    m->parts = 0;
    m->firstPart = (uint32_t)c->parts;
    memcpy(m->objPos, &object->pos, sizeof m->objPos);
    memcpy(m->objRot, &object->rot, sizeof m->objRot);
    memcpy(m->pos, &model->position, sizeof m->pos);
    memcpy(m->rot, &model->rotation, sizeof m->rot);
    memcpy(m->scale, &model->scale, sizeof m->scale);
    m->prev = -1;
    m->snap = kHfSnapNew;
    if ((model->flags & 2) == 0 || model->activeLod >= kZrwModelMaxLods) {
        return;
    }
    for (uint32_t i = 0; i < model->lods[model->activeLod].partCount; i++) {
        ZrwPartDef *const def = model->lods[model->activeLod].parts[i].def;
        HfPart *p;

        if (def == NULL) {
            continue;
        }
        if (c->parts == kMaxParts) {
            gHf.stats.overflow++;
            return;
        }
        p = &c->part[c->parts++];
        p->def = def;
        memcpy(p->offset, def->offset, sizeof p->offset);
        memcpy(p->rot, &def->rotation, sizeof p->rot);
        p->snap = 0;
        m->parts++;
    }
}

/* zobjRefresh's walk: an active object and, depth first, its children. */
static void addTree(HfCapture *c, ZObj *object)
{
    if ((object->flags & kZobjActive) == 0) {
        return;
    }
    addModel(c, object);
    if (object->links == NULL) {
        return;
    }
    for (ZObj *child = object->links->firstChild; child != NULL; child = child->links->next) {
        addTree(c, child);
        if (child->links == NULL) {
            return;
        }
    }
}

/* The objects zobjUpdateAll (0x487020) updated, in its order. */
static void walkPool(HfCapture *c)
{
    for (ZMemFreeNode *node = zmemFreeListFirst(gZobjPool); node != NULL; node = zmemFreeNodeNext(node)) {
        ZObj *const object = zmemFreeNodePayload(node);

        if ((object->flags & kZobjActive) == 0 || (object->flags & kZobjAttached) != 0) {
            continue;
        }
        addModel(c, object);
        if (object->links == NULL) {
            continue;
        }
        for (ZObj *child = object->links->firstChild; child != NULL;) {
            if ((child->flags & kZobjActive) != 0) {
                addModel(c, child);
                if (child->links != NULL) {
                    for (ZObj *sub = child->links->firstChild; sub != NULL; sub = sub->links->next) {
                        addTree(c, sub);
                        if (sub->links == NULL) {
                            break;
                        }
                    }
                }
            }
            if (child->links == NULL) {
                break;
            }
            child = child->links->next;
        }
    }
}

static int samePartDefs(const HfCapture *a, const HfModel *ma, const HfCapture *b, const HfModel *mb)
{
    if (ma->parts != mb->parts) {
        return 0;
    }
    for (uint32_t i = 0; i < ma->parts; i++) {
        if (a->part[ma->firstPart + i].def != b->part[mb->firstPart + i].def) {
            return 0;
        }
    }
    return 1;
}

static int findPrev(const HfCapture *prev, const HfCapture *c, int index)
{
    const HfModel *const m = &c->model[index];

    for (int k = 0; k < prev->models; k++) {
        const int j = (index + k) % prev->models;
        const HfModel *const p = &prev->model[j];

        if (p->object == m->object && p->model == m->model && p->clump == m->clump && p->lod == m->lod &&
            p->modelFlags == m->modelFlags && samePartDefs(prev, p, c, m)) {
            return j;
        }
    }
    return -1;
}

static void logSnap(const char *what, int index, const char *name, int reason, double move, double turn)
{
    if (gHf.log && gHf.logged < 400) {
        gHf.logged++;
        bmxLog("hf: tick %u %s %d (%.16s) %s: move %.4f turn %.2f\n", (unsigned)gHf.stats.ticks, what, index,
               name, hfSnapName(reason), move, turn);
    }
}

/* Matches tick N against N - 1 and applies the skip rules (H6). `global`: the previous capture is unusable.
 */
static void classify(HfCapture *c, const HfCapture *prev, int global)
{
    for (int i = 0; i < c->models; i++) {
        HfModel *const m = &c->model[i];
        const HfModel *p;
        double move, turn;

        m->prev = global || !prev->valid ? -1 : findPrev(prev, c, i);
        if (m->prev < 0) {
            m->snap = global ? kHfSnapGlobal : kHfSnapNew;
            gHf.stats.snaps[m->snap]++;
            continue;
        }
        p = &prev->model[m->prev];
        move = hfDistance(p->pos, m->pos);
        turn = hfQuatAngle(p->rot, m->rot);
        countMove(gHf.stats.objMove, move);
        countTurn(gHf.stats.objTurn, turn);
        m->snap = hfSnapObject(&kHfLimits, p->pos, m->pos, p->rot, m->rot);
        if (m->snap == kHfSnapNone) {
            m->snap = hfSnapObject(&kHfLimits, p->objPos, m->objPos, p->objRot, m->objRot);
        }
        if (m->snap != kHfSnapNone) {
            gHf.stats.snaps[m->snap]++;
            logSnap("object", i, m->model->def != NULL ? m->model->def->name : "?", m->snap, move, turn);
            continue;
        }
        gHf.stats.maxObjMove = move > gHf.stats.maxObjMove ? move : gHf.stats.maxObjMove;
        gHf.stats.maxObjTurn = turn > gHf.stats.maxObjTurn ? turn : gHf.stats.maxObjTurn;
        for (uint32_t k = 0; k < m->parts; k++) {
            HfPart *const part = &c->part[m->firstPart + k];
            const HfPart *const old = &prev->part[p->firstPart + k];
            const double partTurn = hfQuatAngle(old->rot, part->rot);

            part->snap = !(partTurn <= kHfLimits.objectTurn);
            gHf.stats.partSnaps += (uint32_t)part->snap;
            if (!part->snap && partTurn > gHf.stats.maxPartTurn) {
                gHf.stats.maxPartTurn = partTurn;
            }
        }
    }
    c->camSnap = kHfSnapNew;
    if (c->camera != NULL && !global && prev->valid && prev->camera == c->camera) {
        const double move = hfDistance(prev->camPos, c->camPos);
        const double turn = hfQuatAngle(prev->camRot, c->camRot);

        countMove(gHf.stats.camMove, move);
        countTurn(gHf.stats.camTurn, turn);
        c->camSnap = hfSnapCamera(&kHfLimits, prev->camPos, c->camPos, prev->camRot, c->camRot, c->eyeVel);
        if (c->camSnap == kHfSnapNone) {
            gHf.stats.maxCamMove = move > gHf.stats.maxCamMove ? move : gHf.stats.maxCamMove;
            gHf.stats.maxCamTurn = turn > gHf.stats.maxCamTurn ? turn : gHf.stats.maxCamTurn;
        } else {
            logSnap("camera", 0, "camera", c->camSnap, move, turn);
        }
    }
    if (c->camera != NULL) {
        gHf.stats.camSnaps[global ? kHfSnapGlobal : c->camSnap]++;
    }
}

static void capture(uint64_t tickNs)
{
    const HfCapture *const prev = &gHf.cap[gHf.cur];
    HfCapture *const c = &gHf.cap[gHf.cur ^ 1];
    const uint64_t length = (uint64_t)hfLimiterStepMs(gRateA, gRateC) * 1000000u;
    const int global = !prev->valid || hfSnapLate(tickNs - prev->tickNs, length);

    c->models = c->parts = 0;
    c->camera = NULL;
    c->tickNs = tickNs;
    walkPool(c);
    if (c->camera != NULL) {
        const ZObjCamera *const cam = (const ZObjCamera *)(void *)c->camera;
        const CameraState *const state = &gCameras[gCameraPlayer < 4 ? gCameraPlayer : 0];
        ZQuat rot;

        memcpy(&c->camView, (const uint8_t *)cam->camera + kZrwCameraView, sizeof c->camView);
        memcpy(c->camPos, &c->camView.pos, sizeof c->camPos);
        zquatFromMatrix(&rot, &c->camView);
        memcpy(c->camRot, &rot, sizeof c->camRot);
        memcpy(c->eyeVel, &state->eyeVel, sizeof c->eyeVel);
        c->camValid = cam->cache->valid;
    }
    classify(c, prev, global);
    c->valid = 1;
    gHf.cur ^= 1;
}

/* ---- apply / restore ---- */

/* Writes a model's pose at alpha between `prev` (tick N - 1, NULL: none) and tick N into its fields. */
static void writeModel(const HfCapture *c, const HfModel *m, const HfCapture *prev, double alpha)
{
    const HfModel *p = m;
    float v3[3], v4[4];

    if (prev != NULL && prev->valid && m->prev >= 0 && m->snap == kHfSnapNone) {
        p = &prev->model[m->prev];
    } else {
        alpha = 1.0;
        prev = c;
    }
    hfLerp3(v3, p->objPos, m->objPos, alpha);
    memcpy(&m->object->pos, v3, sizeof v3);
    hfNlerp(v4, p->objRot, m->objRot, alpha);
    memcpy(&m->object->rot, v4, sizeof v4);
    hfLerp3(v3, p->pos, m->pos, alpha);
    memcpy(&m->model->position, v3, sizeof v3);
    hfNlerp(v4, p->rot, m->rot, alpha);
    memcpy(&m->model->rotation, v4, sizeof v4);
    hfLerp3(v3, p->scale, m->scale, alpha);
    memcpy(&m->model->scale, v3, sizeof v3);
    for (uint32_t k = 0; k < m->parts; k++) {
        const HfPart *const part = &c->part[m->firstPart + k];
        const HfPart *const old = p != m ? &prev->part[p->firstPart + k] : part;
        const double a = part->snap ? 1.0 : alpha;

        hfLerp3(part->def->offset, old->offset, part->offset, a);
        hfNlerp(v4, old->rot, part->rot, a);
        memcpy(&part->def->rotation, v4, sizeof v4);
    }
}

/*
 * Hands the view at alpha to the renderer camera (gZrwCamSetView, what zobjCameraDraw calls). The camera
 * object itself (pos, rot, matrix cache) is not touched: the tick reads it (the sound listener), and the view
 * the tick handed over is captured from the renderer camera, so a cache the tick left stale is no matter.
 * At alpha 1, a skip or the restore the captured view goes back as it was.
 */
static void setCamera(const HfCapture *c, const HfCapture *prev, double alpha)
{
    void *const renderer = ((ZObjCamera *)(void *)c->camera)->camera;
    ZMatrix view = c->camView;
    ZVec3 eye = c->camView.pos;

    if (prev != NULL && prev->valid && c->camSnap == kHfSnapNone && alpha < 1.0) {
        float v3[3], v4[4];
        ZQuat rot;

        hfLerp3(v3, prev->camPos, c->camPos, alpha);
        hfNlerp(v4, prev->camRot, c->camRot, alpha);
        memcpy(&rot, v4, sizeof rot);
        zquatToMatrix(&view, &rot);
        memcpy(&eye, v3, sizeof eye);
        view.pos = eye;
    }
    gZrwCamSetView(renderer, &view, &eye);
}

/* The poses at alpha written into the fields, the frames rebuilt through the game's own slots. */
static void apply(double alpha)
{
    const HfCapture *const c = &gHf.cap[gHf.cur];
    const HfCapture *const prev = &gHf.cap[gHf.cur ^ 1];

    if (!c->valid) {
        return;
    }
    if (gHf.alpha >= 0.0) {
        alpha = gHf.alpha;
    }
    for (int i = 0; i < c->models; i++) {
        writeModel(c, &c->model[i], prev, alpha);
        gZrwVtUpdate(c->model[i].model);
    }
    if (c->camera != NULL) {
        setCamera(c, prev, alpha);
    }
    gHf.applied = 1;
}

/* FNV-1a 64 over the modelling matrices (floats and flags) of a frame tree. */
static uint64_t fnv(uint64_t h, const void *data, size_t size)
{
    const uint8_t *p = data;

    for (size_t i = 0; i < size; i++) {
        h = (h ^ p[i]) * 0x100000001b3ull;
    }
    return h;
}

static uint64_t hashMatrix(uint64_t h, const void *matrix);

static uint64_t hashTree(uint64_t h, const RwFrame *frame)
{
    for (; frame != NULL; frame = frame->next) {
        h = hashMatrix(h, &frame->modelling);
        h = hashTree(h, frame->child);
    }
    return h;
}

/* BMX_HF_GUARD: what the restore must give back bit for bit, per part (kRestoreParts). Matrices count
 * with their floats and flags; their padding words and the camera cache's are not state. */
enum { kRestoreParts = 7 };
static const char *const kRestoreName[kRestoreParts] = {"object pos/rot",   "model fields", "clump frames",
                                                        "collision frames", "part defs",    "camera pos/rot",
                                                        "camera matrices"};

static uint64_t hashMatrix(uint64_t h, const void *matrix)
{
    const RwMatrix *const m = matrix;

    h = fnv(h, &m->right, sizeof m->right);
    h = fnv(h, &m->flags, sizeof m->flags);
    h = fnv(h, &m->up, sizeof m->up);
    h = fnv(h, &m->at, sizeof m->at);
    return fnv(h, &m->pos, sizeof m->pos);
}

static void restoreHash(uint64_t out[kRestoreParts])
{
    const HfCapture *const c = &gHf.cap[gHf.cur];

    for (int i = 0; i < kRestoreParts; i++) {
        out[i] = 0xcbf29ce484222325ull;
    }
    for (int i = 0; i < c->models; i++) {
        const HfModel *const m = &c->model[i];

        out[0] = fnv(out[0], &m->object->pos, sizeof m->object->pos);
        out[0] = fnv(out[0], &m->object->rot, sizeof m->object->rot);
        out[1] = fnv(out[1], &m->model->position, sizeof m->model->position);
        out[1] = fnv(out[1], &m->model->rotation, sizeof m->model->rotation);
        out[1] = fnv(out[1], &m->model->scale, sizeof m->model->scale);
        out[2] = hashTree(out[2], RpClumpGetFrame(m->model->clump));
        if (m->model->collisionClump != NULL) {
            out[3] = hashTree(out[3], RpClumpGetFrame(m->model->collisionClump));
        }
        for (uint32_t k = 0; k < m->parts; k++) {
            const ZrwPartDef *const def = c->part[m->firstPart + k].def;

            out[4] = fnv(out[4], def->offset, sizeof def->offset);
            out[4] = fnv(out[4], &def->rotation, sizeof def->rotation);
        }
    }
    if (c->camera != NULL) {
        const ZObjCamera *const cam = (const ZObjCamera *)(const void *)c->camera;
        const uint8_t *const zrw = cam->camera;

        out[5] = fnv(out[5], &cam->pos, sizeof cam->pos);
        out[5] = fnv(out[5], &cam->rot, sizeof cam->rot);
        for (int k = 0; k < 4; k++) {
            if ((cam->cache->valid & c->camValid & (1u << k)) != 0) { /* the draw may fill more */
                out[6] = hashMatrix(out[6], &cam->cache->matrix[k]);
            }
        }
        out[6] = hashMatrix(out[6], zrw + kZrwCameraWorld);
        out[6] = hashMatrix(out[6], zrw + kZrwCameraView);
        {
            /* the RwCamera frame: floats only (the sync after the restore normalises its flags, as the
             * draw's sync does in vanilla) */
            const RwMatrix *const m = RwFrameGetMatrix(RwCameraGetFrame(*(void *const *)(zrw + 0x0c)));

            out[6] = fnv(out[6], &m->right, sizeof m->right);
            out[6] = fnv(out[6], &m->up, sizeof m->up);
            out[6] = fnv(out[6], &m->at, sizeof m->at);
            out[6] = fnv(out[6], &m->pos, sizeof m->pos);
        }
    }
}

/* The tick's values back, frames recomputed, the dirty list in the tick's order, synced (HF0 Q3). */
static void restore(void)
{
    const HfCapture *const c = &gHf.cap[gHf.cur];

    gHf.applied = 0;
    for (int i = 0; i < c->models; i++) {
        writeModel(c, &c->model[i], NULL, 1.0);
        gZrwVtUpdate(c->model[i].model);
    }
    if (c->camera != NULL) {
        setCamera(c, NULL, 1.0);
    }
    replayDirty();
    rwFrameSyncDirtyList();
    if (gHf.guard) {
        uint64_t h[kRestoreParts];
        int differ = 0;

        restoreHash(h);
        gHf.restoreChecks++;
        for (int i = 0; i < kRestoreParts; i++) {
            if (h[i] != gHf.restoreHash[i]) {
                differ = 1;
                if (gHf.restoreMismatches < 10) {
                    bmxLog("hf: guard: restore at tick %u: %s differ from the capture\n",
                           (unsigned)gHf.stats.ticks, kRestoreName[i]);
                }
            }
        }
        gHf.restoreMismatches += (uint32_t)differ;
    }
}

/* ---- the draws ---- */

/* An update list as zrwDrawFrame runs it, with the extra draw's node filter. */
static void runList(int index)
{
    for (HfNode *node = gLists[index].head; node != NULL;) {
        HfNode *const next = node->next;
        const uint32_t fn = (uint32_t)(uintptr_t)node->fn;
        int ok;

        if (fn == kShakeNode) {
            node = next;
            continue;
        }
        if (fn == kShadowProbe) {
            /* The probes follow the drawn rider, but the simulation must see one probe set per tick. */
            ZrwColBox box;
            ZrwColState col;
            const uint16_t tag = RwEngineInstance->lightFrame;

            memcpy(box, gZrwColBox, sizeof box);
            memcpy(&col, &gZrwCol, sizeof col);
            ok = node->fn(node->arg);
            memcpy(gZrwColBox, box, sizeof box);
            memcpy(&gZrwCol, &col, sizeof col);
            RwEngineInstance->lightFrame = tag;
        } else {
            ok = node->fn(node->arg);
        }
        if (ok == 0) {
            return; /* the original prints "failed call" and leaves the list */
        }
        node = next;
    }
}

/* zrwDrawFrame (0x48b690) for an extra draw: the filtered lists, no debug list 4, an uncounted present. */
static int hfDrawFrame(void)
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
    }
    gNgifFlushPending = gHf.flush; /* the HUD's ordering table is drawn again */
    gNgifFlushPending2 = gHf.flush2;
    runList(0);
    for (void *camera = zrwcamFirst(); camera != NULL; camera = zrwcamNext(camera)) {
        RwError error;

        if ((((const uint8_t *)camera)[8] & 4) == 0) {
            continue;
        }
        RwErrorGet(&error);
        if (zrwcamBegin(camera) == 0) {
            continue;
        }
        runList(1);
        if (zrwtxrFlush() == 0) {
            return 0;
        }
        zrwResetRenderState();
        runList(2);
        zrwResetRenderState();
        runList(5);
        if (zrwcamEnd() == 0) {
            return 0;
        }
        RwErrorGet(&error);
        if (error.errorCode != rwERRNONE) {
            return 0;
        }
    }
    runList(3);
    drvGpuSetUncounted(1);
    RwRasterShowRaster(gRaster, gWindow, 1);
    drvGpuSetUncounted(0);
    return 1;
}

static void extraDraw(void)
{
    const uint64_t start = bmxTimeNs();
    const HfCapture *const c = &gHf.cap[gHf.cur];

    apply(hfAlpha(start, c->tickNs, (uint64_t)hfLimiterStepMs(gRateA, gRateC) * 1000000u));
    if (gHf.guard) {
        bmxHfGuardExtraBefore();
    }
    hfDrawFrame();
    if (gHf.guard) {
        bmxHfGuardExtraAfter();
    }
    const uint64_t end = bmxTimeNs();

    hfSchedDrawn(&gHf.sched, start, end);
    if (end > gHf.dueNs) {
        const double over = (double)(end - gHf.dueNs) * 1e-6;

        gHf.stats.overruns++;
        gHf.stats.maxOverrunMs = over > gHf.stats.maxOverrunMs ? over : gHf.stats.maxOverrunMs;
    }
    gHf.extraThisTick++;
    gHf.stats.extra++;
    gHf.stats.draws++;
}

/* ---- the loop ---- */

static int inLevel(void)
{
    return gNgMode == 2 && gRateA == 30.0f && gDebugLight == 0;
}

/* One line: moves "<1/16:n <1/8:n ... <4096:n more:n", turns "<1:n <2:n ... <181:n". */
static void histogram(const char *name, const uint32_t *hist, int buckets)
{
    char line[512];
    size_t n = 0;

    for (int b = 0; b < buckets && n < sizeof line - 32; b++) {
        int w;

        if (hist[b] == 0) {
            continue;
        }
        if (buckets == kHfMoveBuckets) {
            w = b == buckets - 1
                    ? snprintf(line + n, sizeof line - n, " more:%u", (unsigned)hist[b])
                    : snprintf(line + n, sizeof line - n, " <%g:%u", ldexp(1.0, b - 4), (unsigned)hist[b]);
        } else {
            w = snprintf(line + n, sizeof line - n, " <%g:%u", kTurnEdges[b], (unsigned)hist[b]);
        }
        n += w > 0 ? (size_t)w : 0;
    }
    line[n] = 0;
    bmxLog("hf:   %s per tick:%s\n", name, line);
}

static void summary(void)
{
    const HfStats *const s = &gHf.stats;

    if (!gHf.log) {
        return;
    }
    bmxLog(
        "hf: %u level ticks, %u draws (%u extra; worst %.2f ms), late ticks %u (max %.2f ms), overflow %u\n",
        (unsigned)s->ticks, (unsigned)s->draws, (unsigned)s->extra, (double)s->worstDraw * 1e-6,
        (unsigned)s->late, s->maxLateMs, (unsigned)s->overflow);
    bmxLog("hf:   extra draws past the tick's due time %u (max %.2f ms), sleeps past it %u (max %.2f ms)\n",
           (unsigned)s->overruns, s->maxOverrunMs, (unsigned)s->oversleeps, s->maxOversleepMs);
    bmxLog("hf:   draws per tick 1:%u 2:%u 3:%u 4:%u 5:%u 6:%u 7:%u 8:%u 9+:%u\n", (unsigned)s->perTick[1],
           (unsigned)s->perTick[2], (unsigned)s->perTick[3], (unsigned)s->perTick[4], (unsigned)s->perTick[5],
           (unsigned)s->perTick[6], (unsigned)s->perTick[7], (unsigned)s->perTick[8],
           (unsigned)s->perTick[9]);
    bmxLog(
        "hf:   object snaps: new %u jump %u turn %u global %u; camera: none %u new %u jump %u turn %u cut %u "
        "global %u\n",
        (unsigned)s->snaps[kHfSnapNew], (unsigned)s->snaps[kHfSnapJump], (unsigned)s->snaps[kHfSnapTurn],
        (unsigned)s->snaps[kHfSnapGlobal], (unsigned)s->camSnaps[kHfSnapNone],
        (unsigned)s->camSnaps[kHfSnapNew], (unsigned)s->camSnaps[kHfSnapJump],
        (unsigned)s->camSnaps[kHfSnapTurn], (unsigned)s->camSnaps[kHfSnapCut],
        (unsigned)s->camSnaps[kHfSnapGlobal]);
    bmxLog(
        "hf:   interpolated maxima per tick: object move %.4f turn %.2f (parts %.2f; %u part turns skipped), "
        "camera move %.4f turn %.2f\n",
        s->maxObjMove, s->maxObjTurn, s->maxPartTurn, (unsigned)s->partSnaps, s->maxCamMove, s->maxCamTurn);
    histogram("object move", s->objMove, kHfMoveBuckets);
    histogram("object turn", s->objTurn, kHfTurnBuckets);
    histogram("camera move", s->camMove, kHfMoveBuckets);
    histogram("camera turn", s->camTurn, kHfTurnBuckets);
    if (gHf.guard) {
        bmxLog("hf:   guard: %u restores checked, %u differ from the capture\n", (unsigned)gHf.restoreChecks,
               (unsigned)gHf.restoreMismatches);
    }
}

/* A tick: restore, onIdle, capture, then the real draw. */
static int tick(uint64_t nowNs)
{
    const char *const quit = getenv("BMX_QUIT_FRAMES");
    uint64_t start;

    if (gHf.inLevel) {
        gHf.stats.perTick[gHf.extraThisTick + 1 < 9 ? gHf.extraThisTick + 1 : 9]++;
        if (gHf.dueNs != 0 && nowNs > gHf.dueNs + 1000000u) {
            const double late = (double)(nowNs - gHf.dueNs) * 1e-6;

            gHf.stats.late++;
            gHf.stats.maxLateMs = late > gHf.stats.maxLateMs ? late : gHf.stats.maxLateMs;
        }
    }
    gHf.extraThisTick = 0;
    gHf.dueNs = 0;
    if (gHf.applied) {
        restore();
    }
    if (!onIdle()) {
        return 0;
    }
    gHf.inLevel = inLevel();
    if (gHf.inLevel) {
        recordDirty();
        gHf.flush = gNgifFlushPending; /* as the tick left them (HF0: captured after onIdle) */
        gHf.flush2 = gNgifFlushPending2;
        capture(nowNs);
        if (gHf.log && gFxQuality != gHf.quality) {
            bmxLog("hf: tick %u far clip (ngiffxAdaptQuality) %.4f\n", (unsigned)gHf.stats.ticks,
                   (double)gFxQuality);
            gHf.quality = gFxQuality;
        }
        gHf.stats.ticks++;
        if (gHf.guard) {
            restoreHash(gHf.restoreHash);
            bmxHfGuardInterpTick();
        }
        if ((quit != NULL && drvGpuShownFrames() + 1 >= strtol(quit, NULL, 10)) ||
            gHf.stats.ticks % 900 == 0) {
            summary();
        }
    } else {
        gHf.cap[0].valid = gHf.cap[1].valid = 0;
    }
    start = bmxTimeNs();
    if (gHf.inLevel) {
        const HfCapture *const c = &gHf.cap[gHf.cur];

        apply(hfAlpha(start, c->tickNs, (uint64_t)hfLimiterStepMs(gRateA, gRateC) * 1000000u));
    }
    drawFrame();
    if (gHf.inLevel) {
        const uint64_t end = bmxTimeNs();

        hfSchedDrawn(&gHf.sched, start, end);
        gHf.stats.draws++;
        if (end - start > gHf.stats.worstDraw) {
            gHf.stats.worstDraw = end - start;
        }
    }
    return 1;
}

int bmxHfStep(void)
{
    uint64_t now, due, wake;
    uint32_t ms;

    if (!gHf.inLevel || !inLevel()) {
        /* The original loop: the same limiter, busy waiting. */
        now = bmxTimeNs();
        if (!hfLimiterTick(bmxTimeMsAt(now), &gLastTick, gRateA, gRateB, gRateC)) {
            return 1;
        }
        return tick(now);
    }
    now = bmxTimeNs();
    ms = bmxTimeMsAt(now);
    if (hfLimiterTick(ms, &gLastTick, gRateA, gRateB, gRateC)) {
        return tick(now);
    }
    due = hfTickDueNs(now, ms, gLastTick, hfLimiterStepMs(gRateA, gRateC));
    gHf.dueNs = due;
    if (gHf.cap[gHf.cur].valid && hfSchedNext(&gHf.sched, now, due, &wake) == kHfDraw) {
        extraDraw();
        return 1;
    }
    if (!gHf.cap[gHf.cur].valid) {
        wake = due;
    }
    if (wake + kSpinNs > due) {
        wake = due > kSpinNs ? due - kSpinNs : 0;
    }
    if (wake > now + 100000u) {
        uint64_t woke;

        SDL_DelayNS(wake - now);
        woke = bmxTimeNs();
        if (woke > due) {
            const double over = (double)(woke - due) * 1e-6;

            gHf.stats.oversleeps++;
            gHf.stats.maxOversleepMs = over > gHf.stats.maxOversleepMs ? over : gHf.stats.maxOversleepMs;
        }
    }
    return 1;
}

int bmxHfInstall(int guard)
{
    const char *const alpha = getenv("BMX_HF_ALPHA");
    const char *const log = getenv("BMX_HF_LOG");
    int fps = bmxConfigInt("video.max_fps", 0);

    gHf.on = bmxConfigInt("video.interpolate", 0) == 1;
    if (!gHf.on) {
        bmxLog("hf: off\n");
        return 0;
    }
    gHf.log = log != NULL && *log != 0 && *log != '0';
    gHf.guard = guard;
    gHf.alpha = alpha != NULL && *alpha != 0 ? strtod(alpha, NULL) : -1.0;
    if (fps <= 0) {
        const SDL_DisplayMode *const mode =
            SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(bmxWindowSdl(gWindow)));

        fps = mode != NULL && mode->refresh_rate > 0.0f ? (int)(mode->refresh_rate + 0.5f) : 60;
    }
    gHf.fps = (uint32_t)(fps > 1000 ? 1000 : fps);
    hfSchedInit(&gHf.sched, gHf.fps, kMarginNs);
    bmxLog("hf: interpolate on, %u draws per second at most (max_fps %d), vsync %d%s%s\n", (unsigned)gHf.fps,
           bmxConfigInt("video.max_fps", 0), bmxConfigInt("video.vsync", 1),
           gHf.alpha >= 0.0 ? ", alpha forced" : "", gHf.guard ? ", guard" : "");
    return 1;
}
