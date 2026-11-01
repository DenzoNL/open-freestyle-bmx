/*
 * bmxhftest: the high frame rate maths (src/native/hfmath.c, docs/HIGHFPS.md H9.5) on synthetic input: the
 * limiter against a verbatim copy of the original's (main.c), the tick's due time against the millisecond
 * clock, the draw scheduler (no draw may end after the next tick is due; draws fill the tick), lerp / nlerp
 * (exact ends, unit length, the short way) and the skip rules.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "native/hfmath.h"

static int gChecks, gFailures;

#define CHECK(cond)                                                                                          \
    do {                                                                                                     \
        gChecks++;                                                                                           \
        if (!(cond)) {                                                                                       \
            gFailures++;                                                                                     \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                                           \
        }                                                                                                    \
    } while (0)

static const double kPi = 3.14159265358979323846;

static uint32_t gRandom = 12345;

static uint32_t nextRandom(void)
{
    gRandom = gRandom * 1103515245u + 12345u;
    return gRandom >> 8;
}

static int sameBits(const float *a, const float *b, int n)
{
    return memcmp(a, b, (size_t)n * sizeof *a) == 0;
}

/* main.c's limiter as it was before the high frame rate mode (the original WinMain's). */
static int referenceTick(uint32_t now, uint32_t *lastTick, float rateA, float rateB, float rateC)
{
    int idle = 0;

    if (rateA == rateB) {
        idle = 1;
    } else if (*lastTick == 0) {
        *lastTick = now;
        idle = 1;
    } else {
        const uint32_t elapsed = now - *lastTick;
        const double limit = (double)rateC / (double)rateA;

        if (!(limit > (double)elapsed)) {
            *lastTick += elapsed;
            idle = 1;
        }
    }
    return idle;
}

static void testLimiter(void)
{
    static const float kRates[] = {0.0f, 30.0f, 60.0f, 25.0f, 29.97f};

    CHECK(hfLimiterStepMs(30.0f, 1000.0f) == 34);
    CHECK(hfLimiterStepMs(60.0f, 1000.0f) == 17);
    CHECK(hfLimiterStepMs(25.0f, 1000.0f) == 40);
    CHECK(hfLimiterStepMs(0.0f, 1000.0f) == 0xffffffffu || hfLimiterStepMs(0.0f, 1000.0f) == 0);
    for (size_t r = 0; r < sizeof kRates / sizeof kRates[0]; r++) {
        uint32_t now = 0xfffff000u; /* across the 32-bit wrap */
        uint32_t a = 0, b = 0;
        int ticks = 0, same = 1;

        for (int i = 0; i < 20000; i++) {
            const int x = referenceTick(now, &a, kRates[r], 0.0f, 1000.0f);
            const int y = hfLimiterTick(now, &b, kRates[r], 0.0f, 1000.0f);

            same &= x == y && a == b;
            ticks += x;
            now += nextRandom() % 4; /* 0..3 ms per loop pass */
        }
        CHECK(same);
        CHECK(ticks > 0);
    }
    /* The step: a tick exactly when the elapsed milliseconds reach it. */
    for (uint32_t e = 0; e < 100; e++) {
        uint32_t last = 1000;

        CHECK(hfLimiterTick(1000 + e, &last, 30.0f, 0.0f, 1000.0f) == (e >= 34));
        last = 1000;
        CHECK(hfLimiterTick(1000 + e, &last, 60.0f, 0.0f, 1000.0f) == (e >= 17));
    }
}

/* The due time is the first instant (ns) at which the millisecond clock makes the limiter tick. */
static void testDue(void)
{
    const uint32_t base = 0x0036ee80;
    const uint32_t step = 34;
    int same = 1;

    for (int k = 0; k < 2000; k++) {
        const uint64_t lastNs = (uint64_t)(nextRandom() % 100000) * 1000000u + nextRandom() % 1000000u;
        const uint32_t lastTick = base + (uint32_t)(lastNs / 1000000u);
        const uint64_t now = lastNs + (uint64_t)(nextRandom() % 40000) * 1000u;
        const uint64_t due = hfTickDueNs(now, base + (uint32_t)(now / 1000000u), lastTick, step);
        uint64_t first = now;

        /* Walk the clock in 10 us steps to the first tick. */
        for (;;) {
            uint32_t last = lastTick;

            if (hfLimiterTick(base + (uint32_t)(first / 1000000u), &last, 30.0f, 0.0f, 1000.0f)) {
                break;
            }
            first += 10000u - first % 10000u;
        }
        /* `first` is the first 10 us grid point at or after the due instant (or now when already due). */
        same &= due <= first && first - due < 10000u && due >= now;
    }
    CHECK(same);
}

/* A synthetic level: ticks every 34 ms by the limiter, draws of a given cost scheduled in between. */
typedef struct SimResult {
    int ticks, draws, late, maxPerTick, minPerTick, wakeAfterDue;
    uint64_t maxLateNs;
} SimResult;

static SimResult simulate(uint32_t fps, uint64_t costNs, uint64_t jitterNs, uint64_t spikeNs)
{
    const uint32_t base = 0x0036ee80;
    HfSched s;
    SimResult r = {0, 0, 0, 0, 1000, 0, 0};
    uint64_t now = 5000000;
    uint32_t lastTick = base + 5;
    int perTick = 0;

    hfSchedInit(&s, fps, 1000000);
    while (r.ticks < 3000) {
        const uint32_t ms = base + (uint32_t)(now / 1000000u);
        const uint64_t due = hfTickDueNs(now, ms, lastTick, 34);
        uint64_t wake;

        if (hfLimiterTick(ms, &lastTick, 30.0f, 0.0f, 1000.0f)) {
            /* tick + the real draw, which always runs */
            const uint64_t cost = costNs + (jitterNs != 0 ? nextRandom() % jitterNs : 0);

            if (r.ticks > 0) {
                r.maxPerTick = perTick > r.maxPerTick ? perTick : r.maxPerTick;
                r.minPerTick = perTick < r.minPerTick ? perTick : r.minPerTick;
            }
            r.ticks++;
            perTick = 1;
            now += 200000; /* onIdle */
            hfSchedDrawn(&s, now, now + cost);
            now += cost;
            continue;
        }
        if (hfSchedNext(&s, now, due, &wake) == kHfDraw) {
            const uint64_t cost =
                costNs + (jitterNs != 0 ? nextRandom() % jitterNs : 0) + (r.draws % 97 == 0 ? spikeNs : 0);
            const uint64_t estimate = hfSchedWorst(&s);

            if (now + cost > due && cost <= estimate) {
                r.late++; /* a draw within the estimate ended after the tick was due */
                r.maxLateNs = now + cost - due > r.maxLateNs ? now + cost - due : r.maxLateNs;
            }
            hfSchedDrawn(&s, now, now + cost);
            now += cost;
            r.draws++;
            perTick++;
            continue;
        }
        r.wakeAfterDue += wake > due;
        /* The loop sleeps up to 1.5 ms before the tick, then spins in 50 us passes. */
        if (wake + 1500000 > due) {
            wake = due > 1500000 ? due - 1500000 : 0;
        }
        now = wake > now ? wake : now + 50000;
    }
    return r;
}

static void testScheduler(void)
{
    HfSched s;
    uint64_t wake = 0;
    SimResult r;

    hfSchedInit(&s, 144, 1000000);
    CHECK(s.periodNs == 6944444);
    CHECK(hfSchedWorst(&s) == 0);
    hfSchedDrawn(&s, 100000000, 101500000);
    hfSchedDrawn(&s, 107000000, 113000000);
    CHECK(hfSchedWorst(&s) == 6000000);
    /* the next slot is 113.94 ms; a tick due at 150 ms leaves room */
    CHECK(hfSchedNext(&s, 110000000, 150000000, &wake) == kHfWait && wake == 107000000 + 6944444);
    CHECK(hfSchedNext(&s, 114000000, 150000000, &wake) == kHfDraw);
    /* 114 + 6 + 1 > 120.5: wait for the tick instead */
    CHECK(hfSchedNext(&s, 114000000, 120500000, &wake) == kHfWait && wake == 120500000);
    CHECK(hfSchedNext(&s, 114000000, 121000000, &wake) == kHfDraw);
    /* already due */
    CHECK(hfSchedNext(&s, 130000000, 125000000, &wake) == kHfWait && wake == 125000000);
    /* the worst of the last kHfCostSlots draws */
    for (int i = 0; i < kHfCostSlots; i++) {
        hfSchedDrawn(&s, 200000000 + (uint64_t)i * 7000000, 200000000 + (uint64_t)i * 7000000 + 1000000);
    }
    CHECK(hfSchedWorst(&s) == 1000000);

    /* 144 Hz, 1.5 ms draws: 4-5 draws per 34 ms tick, none late. */
    r = simulate(144, 1500000, 0, 0);
    CHECK(r.late == 0 && r.wakeAfterDue == 0);
    CHECK(r.minPerTick >= 4 && r.maxPerTick <= 5);
    /* 120 Hz: 4 per tick (34 / 8.33). */
    r = simulate(120, 1500000, 0, 0);
    CHECK(r.late == 0 && r.wakeAfterDue == 0);
    CHECK(r.minPerTick >= 4 && r.maxPerTick <= 5);
    /* 240 Hz with jittery 1-6 ms draws and spikes of 6 ms more: never late within the estimate. */
    r = simulate(240, 1000000, 5000000, 6000000);
    CHECK(r.late == 0 && r.wakeAfterDue == 0);
    CHECK(r.minPerTick >= 2);
    /* Draws as long as a tick: only the real draw per tick. */
    r = simulate(144, 33000000, 0, 0);
    CHECK(r.late == 0 && r.maxPerTick == 1);
}

static void testLerp(void)
{
    const float a[3] = {1.0f, -2.5f, 1e6f}, b[3] = {3.0f, 0.1f, -7.0f};
    float out[3];

    hfLerp3(out, a, b, 0.0);
    CHECK(sameBits(out, a, 3));
    hfLerp3(out, a, b, -0.5);
    CHECK(sameBits(out, a, 3));
    hfLerp3(out, a, b, 1.0);
    CHECK(sameBits(out, b, 3));
    hfLerp3(out, a, b, 1.5);
    CHECK(sameBits(out, b, 3));
    hfLerp3(out, a, b, 0.5);
    CHECK(out[0] == 2.0f && fabsf(out[1] - -1.2f) < 1e-6f && out[2] == (float)((1e6 - 7.0) * 0.5));
    hfLerp3(out, a, a, 0.37);
    CHECK(sameBits(out, a, 3));
}

static void axisAngle(float q[4], double x, double y, double z, double degrees)
{
    const double h = degrees * kPi / 360.0, l = sqrt(x * x + y * y + z * z);

    q[0] = (float)(x / l * sin(h));
    q[1] = (float)(y / l * sin(h));
    q[2] = (float)(z / l * sin(h));
    q[3] = (float)cos(h);
}

static void testNlerp(void)
{
    float a[4], b[4], nb[4], out[4], out2[4];

    axisAngle(a, 0, 0, 1, 10);
    axisAngle(b, 1, 2, 3, 80);
    for (int i = 0; i < 4; i++) {
        nb[i] = -b[i];
    }
    hfNlerp(out, a, b, 0.0);
    CHECK(sameBits(out, a, 4));
    hfNlerp(out, a, b, 1.0);
    CHECK(sameBits(out, b, 4));
    hfNlerp(out, a, nb, 1.0);
    CHECK(sameBits(out, nb, 4)); /* t = 1 is the input as stored, sign included */
    for (int k = 1; k < 10; k++) {
        const double t = k / 10.0;
        double length = 0.0;

        hfNlerp(out, a, b, t);
        hfNlerp(out2, a, nb, t);
        for (int i = 0; i < 4; i++) {
            length += (double)out[i] * out[i];
        }
        CHECK(fabs(length - 1.0) < 1e-6);
        CHECK(sameBits(out, out2, 4)); /* -b is the same orientation: the short way either way */
        /* on the path: the angles to both ends add up to the whole */
        CHECK(fabs(hfQuatAngle(a, out) + hfQuatAngle(out, b) - hfQuatAngle(a, b)) < 1e-3);
        CHECK(hfQuatAngle(a, out) < hfQuatAngle(a, b));
    }
    /* The midpoint of a 90 degree turn about one axis is the 45 degree turn (nlerp is exact at 0.5). */
    axisAngle(a, 0, 1, 0, 0);
    axisAngle(b, 0, 1, 0, 90);
    hfNlerp(out, a, b, 0.5);
    axisAngle(out2, 0, 1, 0, 45);
    CHECK(hfQuatAngle(out, out2) < 1e-3);
    CHECK(fabs(hfQuatAngle(a, b) - 90.0) < 1e-4);
    CHECK(fabs(hfQuatAngle(b, a) - 90.0) < 1e-4);
    CHECK(hfQuatAngle(b, b) < 1e-2);
    /* Degenerate: a = -b at t = 0.5 would cancel without the sign flip. */
    hfNlerp(out, a, (const float[4]){-a[0], -a[1], -a[2], -a[3]}, 0.5);
    CHECK(hfQuatAngle(out, a) < 1e-3);
}

static void testAlpha(void)
{
    CHECK(hfAlpha(100, 100, 34) == 0.0);
    CHECK(hfAlpha(50, 100, 34) == 0.0);
    CHECK(hfAlpha(117, 100, 34) == 0.5);
    CHECK(hfAlpha(134, 100, 34) == 1.0);
    CHECK(hfAlpha(500, 100, 34) == 1.0);
    CHECK(hfAlpha(500, 100, 0) == 1.0);
}

static void testSnap(void)
{
    const HfLimits limits = {
        .objectJump = 2.0, .objectTurn = 90.0, .cameraJump = 3.0, .cameraTurn = 60.0, .cameraCutMove = 0.001};
    const float p0[3] = {0, 0, 0}, near[3] = {1.0f, 1.0f, 1.0f}, far[3] = {2.0f, 1.0f, 0.0f};
    const float moving[3] = {0.5f, 0, 0}, still[3] = {0, 0, 0};
    float r0[4], r80[4], r100[4], r50[4], r70[4];

    axisAngle(r0, 0, 0, 1, 0);
    axisAngle(r80, 0, 1, 0, 80);
    axisAngle(r100, 1, 0, 0, 100);
    axisAngle(r50, 0, 0, 1, 50);
    axisAngle(r70, 0, 0, 1, 70);
    CHECK(hfSnapObject(&limits, p0, near, r0, r80) == kHfSnapNone); /* 1.73 units, 80 degrees */
    CHECK(hfSnapObject(&limits, p0, far, r0, r0) == kHfSnapJump);   /* 2.24 units */
    CHECK(hfSnapObject(&limits, p0, near, r0, r100) == kHfSnapTurn);
    CHECK(hfSnapObject(&limits, p0, p0, r100, r100) == kHfSnapNone);
    /* spins: 720 degrees per second is 24 per tick */
    {
        float a[4], b[4];

        axisAngle(a, 0, 0, 1, 350);
        axisAngle(b, 0, 0, 1, 14); /* across the wrap: 24 degrees */
        CHECK(fabs(hfQuatAngle(a, b) - 24.0) < 1e-3);
        CHECK(hfSnapObject(&limits, p0, p0, a, b) == kHfSnapNone);
    }
    CHECK(hfSnapCamera(&limits, p0, near, r0, r50, moving) == kHfSnapNone);
    CHECK(hfSnapCamera(&limits, p0, near, r0, r70, moving) == kHfSnapTurn);
    CHECK(hfSnapCamera(&limits, p0, (const float[3]){4, 0, 0}, r0, r0, moving) == kHfSnapJump);
    CHECK(hfSnapCamera(&limits, p0, near, r0, r0, still) == kHfSnapCut); /* moved with eyeVel = 0 */
    CHECK(hfSnapCamera(&limits, p0, p0, r0, r50, still) == kHfSnapNone); /* a pure turn is no cut */
    CHECK(hfSnapLate(68000000, 34000000) == 0);
    CHECK(hfSnapLate(68000001, 34000000) == 1);
    CHECK(strcmp(hfSnapName(kHfSnapCut), "cut") == 0);
    /* The shipped constants keep their order of magnitude. */
    CHECK(kHfLimits.objectTurn == 90.0 && kHfLimits.cameraTurn == 60.0);
    CHECK(kHfLimits.objectJump > 0.0 && kHfLimits.cameraJump > 0.0);
}

/* HF5: hfLerpN, hfSnapFov, hfParticleAt, hfParticleLife. */
static void testHf5(void)
{
    const HfLimits limits = {.fovRatio = 1.5};
    const float w0[2] = {0.5f, 0.375f}, wEase[2] = {0.6f, 0.45f}, wCut[2] = {0.13f, 0.1f};
    float w[2];

    hfLerpN(w, w0, wEase, 2, 0.0);
    CHECK(sameBits(w, w0, 2));
    hfLerpN(w, w0, wEase, 2, 1.0);
    CHECK(sameBits(w, wEase, 2));
    hfLerpN(w, w0, wEase, 2, 0.5);
    CHECK(fabsf(w[0] - 0.55f) < 1e-6f && fabsf(w[1] - 0.4125f) < 1e-6f);
    memcpy(w, w0, sizeof w);
    hfLerpN(w, w, wEase, 2, 0.5); /* aliasing */
    CHECK(fabsf(w[0] - 0.55f) < 1e-6f);
    CHECK(hfSnapFov(&limits, w0, wEase) == kHfSnapNone);
    CHECK(hfSnapFov(&limits, wEase, w0) == kHfSnapNone);
    CHECK(hfSnapFov(&limits, w0, wCut) == kHfSnapJump);
    CHECK(hfSnapFov(&limits, wCut, w0) == kHfSnapJump);
    CHECK(hfSnapFov(&limits, w0, (const float[2]){0.0f, 0.375f}) == kHfSnapJump);
    CHECK(kHfLimits.fovRatio > 1.0);

    {
        const HfParticle old = {{0, 0, 0}, {-1, 0, 0}, 2.0f};
        const HfParticle cur = {{1, 2, 0}, {0, 0, 0}, 4.0f};
        const HfParticle spawned = {{3, 0, 0}, {1, 0, 0}, 1.0f};
        HfParticle out;

        /* t >= 1: the tick's bits, whatever the case */
        hfParticleAt(&out, &old, &cur, 1, 1.0);
        CHECK(memcmp(&out, &cur, sizeof out) == 0);
        hfParticleAt(&out, NULL, &spawned, 1, 1.0);
        CHECK(memcmp(&out, &spawned, sizeof out) == 0);
        /* a moving particle seen at N - 1: pos and the streak's tail both lerp, size too */
        hfParticleAt(&out, &old, &cur, 1, 0.25);
        CHECK(out.pos[0] == 0.25f && out.pos[1] == 0.5f && out.pos[2] == 0.0f);
        CHECK(out.prev[0] == -0.75f && out.prev[1] == 0.0f);
        CHECK(out.size == 2.5f);
        hfParticleAt(&out, &old, &cur, 1, 0.0);
        CHECK(sameBits(out.pos, old.pos, 3) && sameBits(out.prev, old.prev, 3) && out.size == old.size);
        /* spawned in the tick: from the spawn point (prev), the tail stays there */
        hfParticleAt(&out, NULL, &spawned, 1, 0.5);
        CHECK(out.pos[0] == 2.0f && sameBits(out.prev, spawned.prev, 3) && out.size == spawned.size);
        hfParticleAt(&out, NULL, &spawned, 1, 0.0);
        CHECK(sameBits(out.pos, spawned.prev, 3));
        /* a static particle (poster, ripple) keeps its position, grows */
        hfParticleAt(&out, &old, &cur, 0, 0.5);
        CHECK(sameBits(out.pos, cur.pos, 3) && sameBits(out.prev, cur.prev, 3) && out.size == 3.0f);
    }
    /* the fade's remaining life never wraps in a draw */
    CHECK(hfParticleLife(1000, 500, 1200, 8) == 500);
    CHECK(hfParticleLife(1000, 500, 1492, 8) == 500);
    CHECK(hfParticleLife(1000, 500, 1493, 8) == 501);          /* 7 left: drawn with 8 */
    CHECK(hfParticleLife(1000, 500, 1520, 8) == 528);          /* expired 20 ms ago */
    CHECK(hfParticleLife(0xfffffff0u, 0x40, 0x20, 8) == 0x40); /* across the clock's wrap */
    CHECK(hfParticleLife(0xfffffff0u, 0x10, 0x20, 8) == 0x38);
    {
        const uint32_t birth = 5000, life = 300, now = 5340;
        const uint32_t drawn = hfParticleLife(birth, life, now, 8);
        const uint32_t remaining = birth + drawn - now;

        CHECK(remaining == 8 && (remaining >> 1) < 0x200);
    }
}

int main(void)
{
    testLimiter();
    testDue();
    testScheduler();
    testLerp();
    testNlerp();
    testAlpha();
    testSnap();
    testHf5();
    printf("bmxhftest: %d checks, %d failures\n", gChecks, gFailures);
    return gFailures != 0;
}
