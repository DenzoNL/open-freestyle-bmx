/*
 * High frame rate (docs/HIGHFPS.md, HF1-HF3): the pure parts, without SDL or game state, so that
 * tests/platform/test_hf.c can check them on synthetic input.
 *
 * - The original frame limiter (WinMain 0x48a460, main.c's copy): when a tick is due.
 * - The draw scheduler of `[video] interpolate = 1`: when an extra draw may start without delaying the next
 *   tick (H5, P9), and how long the loop may sleep.
 * - The interpolation maths (H2): positions lerped in double, quaternions nlerped (sign flip, normalised in
 *   double), each stored as float; alpha 0 and 1 give the two inputs' bits exactly.
 * - The skip rules (H6): when an object or the camera is drawn at the tick pose instead.
 */
#ifndef BMX_NATIVE_HFMATH_H
#define BMX_NATIVE_HFMATH_H

#include <stdint.h>

/* ---- the limiter ---- */

/*
 * One check of the original limiter: rateA = gZrwFrameLimit (0x69771c), rateB = 0.0f (0x54d05c), rateC =
 * 1000.0f (0x54dbb8). Returns 1 when a tick runs now (and updates *lastTick as the original does), 0 when
 * not yet.
 */
int hfLimiterTick(uint32_t now, uint32_t *lastTick, float rateA, float rateB, float rateC);
/* The smallest elapsed millisecond count at which the limiter ticks (34 at a limit of 30, 17 at 60). */
uint32_t hfLimiterStepMs(float rateA, float rateC);
/*
 * When (in ns of the clock bmxTimeNs counts) the next tick is due: the limiter ticks once the millisecond
 * clock reaches lastTick + stepMs. `nowNs` and `nowMs` are the same instant in both units (ms = base +
 * ns / 1e6, so the next millisecond starts at the next multiple of 1e6 ns).
 */
uint64_t hfTickDueNs(uint64_t nowNs, uint32_t nowMs, uint32_t lastTick, uint32_t stepMs);

/* ---- the draw scheduler ---- */

enum { kHfCostSlots = 64 };

typedef struct HfSched {
    uint64_t periodNs;           /* 1e9 / the draw rate (max_fps, or the display refresh) */
    uint64_t marginNs;           /* kept free before a tick on top of the worst draw */
    uint64_t lastDrawNs;         /* start of the last draw (0: none yet) */
    uint64_t cost[kHfCostSlots]; /* durations of the latest draws, start to end of present */
    int costNext;
} HfSched;

enum {
    kHfDraw = 1, /* draw now */
    kHfWait = 0  /* wait until *wakeNs (a draw's time, or the tick's due time) */
};

void hfSchedInit(HfSched *s, uint32_t fps, uint64_t marginNs);
/* The worst duration of the recent draws (0 when none yet). */
uint64_t hfSchedWorst(const HfSched *s);
/* Records a draw that ran from startNs to endNs. */
void hfSchedDrawn(HfSched *s, uint64_t startNs, uint64_t endNs);
/*
 * Whether an extra draw starts now. A draw starts when its slot has come (last draw + period) and it would
 * end, at the worst recent draw time plus the margin, no later than the tick's due time. Otherwise *wakeNs is
 * the next moment to look again: the draw slot, or the tick's due time when no draw fits before it.
 */
int hfSchedNext(const HfSched *s, uint64_t nowNs, uint64_t tickDueNs, uint64_t *wakeNs);

/* ---- interpolation ---- */

/* alpha = (now - tick) / length, clamped to [0, 1]. */
double hfAlpha(uint64_t nowNs, uint64_t tickNs, uint64_t lengthNs);
/* out = a + (b - a) * t per component, in double; t <= 0 gives a's bits, t >= 1 b's. */
void hfLerp3(float out[3], const float a[3], const float b[3], double t);
/* hfLerp3 for n components (out may alias a or b). */
void hfLerpN(float *out, const float *a, const float *b, int n, double t);
/*
 * Normalised linear interpolation of two quaternions (x, y, z, w): b is negated when a . b < 0 (the short
 * way), the sum is normalised in double. t <= 0 gives a's bits, t >= 1 b's (no renormalisation).
 */
void hfNlerp(float out[4], const float a[4], const float b[4], double t);
/* Distance of two points (double). */
double hfDistance(const float a[3], const float b[3]);
/* The angle between two orientations in degrees (0..180): 2 acos |a . b| of the normalised quaternions. */
double hfQuatAngle(const float a[4], const float b[4]);

/* ---- skip rules (H6) ---- */

typedef struct HfLimits {
    double objectJump;    /* game units per tick */
    double objectTurn;    /* degrees per tick */
    double cameraJump;    /* game units per tick */
    double cameraTurn;    /* degrees per tick */
    double cameraCutMove; /* an eye moving at least this much while its eyeVel is zero is a cut */
    double fovRatio;      /* the view window may grow or shrink by this factor per tick (HF5) */
} HfLimits;

/* The constants of the native exe (docs/HIGHFPS.md "HF4": thresholds). */
extern const HfLimits kHfLimits;

enum {
    kHfSnapNone = 0,
    kHfSnapJump,  /* moved too far */
    kHfSnapTurn,  /* turned too far */
    kHfSnapCut,   /* camera: moved with a zero eyeVel (cameraUpdate's jump to the goals) */
    kHfSnapNew,   /* no previous capture with the same key (new or reused slot, other clump / LOD / parts) */
    kHfSnapGlobal /* mode, limit or level changed, or the tick came late */
};

/* An object's rule: position jump or root rotation over the limits. */
int hfSnapObject(const HfLimits *limits, const float pos0[3], const float pos1[3], const float rot0[4],
                 const float rot1[4]);
/* The camera's rule: `eyeVel` is the selected camera state's last eye move (zero after a cut). */
int hfSnapCamera(const HfLimits *limits, const float pos0[3], const float pos1[3], const float rot0[4],
                 const float rot1[4], const float eyeVel[3]);
/* The FOV's rule (HF5): the view window (tangents of the half angles) changed by more than fovRatio. */
int hfSnapFov(const HfLimits *limits, const float window0[2], const float window1[2]);
/* Global rule: 1 when the tick interval exceeds two tick lengths. */
int hfSnapLate(uint64_t intervalNs, uint64_t lengthNs);
/* A short name of a kHfSnap* reason, for the log. */
const char *hfSnapName(int reason);

/* ---- particles (HF5) ---- */

/* What a particle draw reads of the step: its position, the position before the step (spark and lightning
 * streaks run from there) and its size (ripples grow). */
typedef struct HfParticle {
    float pos[3], prev[3], size;
} HfParticle;

/*
 * A particle drawn at t between ticks N - 1 and N (one tick behind, like the models). `old` is its record at
 * N - 1 (NULL: spawned in tick N), `cur` at N; `moves`: kind bit 0 (the step moves it).
 * - moving, known at N - 1: pos and prev both lerp between the two records (the streak keeps its length);
 * - moving, new: pos lerps from the spawn point (`cur->prev`, where the spawners put it) to `cur->pos`, prev
 *   stays at the spawn point (a spark spawned after the step has pos == prev and does not move);
 * - not moving: pos and prev as at N.
 * The size lerps when `old` is known. t >= 1 gives `cur`'s bits.
 */
void hfParticleAt(HfParticle *out, const HfParticle *old, const HfParticle *cur, int moves, double t);
/*
 * The life a particle is drawn with between ticks, so that its renderer's fade (remaining = birth + life -
 * now, unsigned) does not wrap: a particle whose remaining life at `now` is below `minRemaining` ms (its life
 * ran out after the tick that keeps it until the next step) is drawn with `minRemaining` left, nearly faded
 * (the spark renderer would draw a wrapped remaining at full alpha). Otherwise `life` itself.
 */
uint32_t hfParticleLife(uint32_t birth, uint32_t life, uint32_t now, uint32_t minRemaining);

#endif
