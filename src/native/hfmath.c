/* The pure parts of the high frame rate mode (hfmath.h, docs/HIGHFPS.md). */
#include "native/hfmath.h"

#include <math.h>
#include <string.h>

/* Measured in HF4 (docs/HIGHFPS.md "HF4"): well above normal riding, well below respawns and cuts. */
const HfLimits kHfLimits = {
    .objectJump =
        500.0, /* riding <= 64 per tick, the Vail Trails train 40; respawns and the train wrap >= 2048 */
    .objectTurn = 90.0,
    .cameraJump = 256.0, /* chase and replay cameras <= 128 per tick; view changes >= 512 */
    .cameraTurn = 60.0,
    .cameraCutMove = 0.001,
    .fovRatio = 1.5, /* HF5: the chase camera's FOV eases (camera.c) */
};

int hfLimiterTick(uint32_t now, uint32_t *lastTick, float rateA, float rateB, float rateC)
{
    if (rateA == rateB) {
        return 1;
    }
    if (*lastTick == 0) {
        *lastTick = now;
        return 1;
    }
    {
        const uint32_t elapsed = now - *lastTick;
        /* The original divides on the x87 at 53-bit precision: double arithmetic on the floats. */
        const double limit = (double)rateC / (double)rateA;

        if (!(limit > (double)elapsed)) {
            *lastTick += elapsed;
            return 1;
        }
    }
    return 0;
}

uint32_t hfLimiterStepMs(float rateA, float rateC)
{
    const double limit = (double)rateC / (double)rateA;

    if (!(limit > 0.0)) {
        return 0;
    }
    if (limit >= 4294967295.0) {
        return 0xffffffffu;
    }
    return (uint32_t)ceil(limit);
}

uint64_t hfTickDueNs(uint64_t nowNs, uint32_t nowMs, uint32_t lastTick, uint32_t stepMs)
{
    const int32_t remaining = (int32_t)(lastTick + stepMs - nowMs);

    if (remaining <= 0) {
        return nowNs;
    }
    return nowNs - nowNs % 1000000u + (uint64_t)remaining * 1000000u;
}

void hfSchedInit(HfSched *s, uint32_t fps, uint64_t marginNs)
{
    memset(s, 0, sizeof *s);
    s->periodNs = 1000000000u / (fps != 0 ? fps : 60u);
    s->marginNs = marginNs;
}

uint64_t hfSchedWorst(const HfSched *s)
{
    uint64_t worst = 0;

    for (int i = 0; i < kHfCostSlots; i++) {
        worst = s->cost[i] > worst ? s->cost[i] : worst;
    }
    return worst;
}

void hfSchedDrawn(HfSched *s, uint64_t startNs, uint64_t endNs)
{
    s->lastDrawNs = startNs;
    s->cost[s->costNext] = endNs > startNs ? endNs - startNs : 0;
    s->costNext = (s->costNext + 1) % kHfCostSlots;
}

int hfSchedNext(const HfSched *s, uint64_t nowNs, uint64_t tickDueNs, uint64_t *wakeNs)
{
    const uint64_t slot = s->lastDrawNs != 0 ? s->lastDrawNs + s->periodNs : nowNs;
    const uint64_t start = slot > nowNs ? slot : nowNs;
    const uint64_t need = hfSchedWorst(s) + s->marginNs;

    if (start >= tickDueNs || tickDueNs - start < need) {
        *wakeNs = tickDueNs;
        return kHfWait;
    }
    if (start > nowNs) {
        *wakeNs = start;
        return kHfWait;
    }
    *wakeNs = nowNs;
    return kHfDraw;
}

double hfAlpha(uint64_t nowNs, uint64_t tickNs, uint64_t lengthNs)
{
    double alpha;

    if (nowNs <= tickNs || lengthNs == 0) {
        return lengthNs == 0 ? 1.0 : 0.0;
    }
    alpha = (double)(nowNs - tickNs) / (double)lengthNs;
    return alpha > 1.0 ? 1.0 : alpha;
}

void hfLerpN(float *out, const float *a, const float *b, int n, double t)
{
    if (t <= 0.0) {
        memmove(out, a, (size_t)n * sizeof *out);
        return;
    }
    if (t >= 1.0) {
        memmove(out, b, (size_t)n * sizeof *out);
        return;
    }
    for (int i = 0; i < n; i++) {
        out[i] = (float)((double)a[i] + ((double)b[i] - (double)a[i]) * t);
    }
}

void hfLerp3(float out[3], const float a[3], const float b[3], double t)
{
    hfLerpN(out, a, b, 3, t);
}

void hfNlerp(float out[4], const float a[4], const float b[4], double t)
{
    double q[4], dot = 0.0, length = 0.0, sign;

    if (t <= 0.0) {
        memcpy(out, a, 4 * sizeof *out);
        return;
    }
    if (t >= 1.0) {
        memcpy(out, b, 4 * sizeof *out);
        return;
    }
    for (int i = 0; i < 4; i++) {
        dot += (double)a[i] * (double)b[i];
    }
    sign = dot < 0.0 ? -1.0 : 1.0;
    for (int i = 0; i < 4; i++) {
        q[i] = (double)a[i] * (1.0 - t) + sign * (double)b[i] * t;
        length += q[i] * q[i];
    }
    length = sqrt(length);
    if (!(length > 1e-12)) {
        memcpy(out, b, 4 * sizeof *out);
        return;
    }
    for (int i = 0; i < 4; i++) {
        out[i] = (float)(q[i] / length);
    }
}

double hfDistance(const float a[3], const float b[3])
{
    double sum = 0.0;

    for (int i = 0; i < 3; i++) {
        const double d = (double)b[i] - (double)a[i];

        sum += d * d;
    }
    return sqrt(sum);
}

double hfQuatAngle(const float a[4], const float b[4])
{
    double dot = 0.0, la = 0.0, lb = 0.0, c;

    for (int i = 0; i < 4; i++) {
        dot += (double)a[i] * (double)b[i];
        la += (double)a[i] * (double)a[i];
        lb += (double)b[i] * (double)b[i];
    }
    if (!(la > 0.0) || !(lb > 0.0)) {
        return 180.0;
    }
    c = fabs(dot) / sqrt(la * lb);
    c = c > 1.0 ? 1.0 : c;
    return 2.0 * acos(c) * (180.0 / 3.14159265358979323846);
}

int hfSnapObject(const HfLimits *limits, const float pos0[3], const float pos1[3], const float rot0[4],
                 const float rot1[4])
{
    if (!(hfDistance(pos0, pos1) <= limits->objectJump)) {
        return kHfSnapJump;
    }
    if (!(hfQuatAngle(rot0, rot1) <= limits->objectTurn)) {
        return kHfSnapTurn;
    }
    return kHfSnapNone;
}

int hfSnapCamera(const HfLimits *limits, const float pos0[3], const float pos1[3], const float rot0[4],
                 const float rot1[4], const float eyeVel[3])
{
    const double moved = hfDistance(pos0, pos1);

    if (!(moved <= limits->cameraJump)) {
        return kHfSnapJump;
    }
    if (!(hfQuatAngle(rot0, rot1) <= limits->cameraTurn)) {
        return kHfSnapTurn;
    }
    if (eyeVel[0] == 0.0f && eyeVel[1] == 0.0f && eyeVel[2] == 0.0f && moved >= limits->cameraCutMove) {
        return kHfSnapCut;
    }
    return kHfSnapNone;
}

int hfSnapFov(const HfLimits *limits, const float window0[2], const float window1[2])
{
    for (int i = 0; i < 2; i++) {
        const double ratio = (double)window1[i] / (double)window0[i];

        if (!(window0[i] > 0.0f) || !(window1[i] > 0.0f) || !(ratio <= limits->fovRatio) ||
            !(ratio * limits->fovRatio >= 1.0)) {
            return kHfSnapJump;
        }
    }
    return kHfSnapNone;
}

int hfSnapLate(uint64_t intervalNs, uint64_t lengthNs)
{
    return intervalNs > 2 * lengthNs;
}

const char *hfSnapName(int reason)
{
    static const char *const kNames[] = {"none", "jump", "turn", "cut", "new", "global"};

    return reason >= 0 && reason <= kHfSnapGlobal ? kNames[reason] : "?";
}

void hfParticleAt(HfParticle *out, const HfParticle *old, const HfParticle *cur, int moves, double t)
{
    *out = *cur;
    if (t >= 1.0) {
        return;
    }
    if (moves && old != NULL) {
        hfLerpN(out->pos, old->pos, cur->pos, 3, t);
        hfLerpN(out->prev, old->prev, cur->prev, 3, t);
    } else if (moves) {
        hfLerpN(out->pos, cur->prev, cur->pos, 3, t);
    }
    if (old != NULL) {
        hfLerpN(&out->size, &old->size, &cur->size, 1, t);
    }
}

uint32_t hfParticleLife(uint32_t birth, uint32_t life, uint32_t now, uint32_t minRemaining)
{
    if ((int32_t)(birth + life - now) < (int32_t)minRemaining) {
        return now + minRemaining - birth;
    }
    return life;
}
