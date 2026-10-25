/* Motion rigs and the screen overlay layer (C:\zaxis\Bmx\src\com\zge\bmx\zoverii.c). */
#include "bmx/com/zge/bmx/zoverii.h"

#include "bmx/com/zge/ngif/ngifpsx.h"
#include "bmx/com/zge/ngif/ngifzactor.h"
#include "bmx/com/zge/ngif/ngifzbsp.h"
#include "compat/crt.h"
#include "zge/zdebug.h"
#include "zge/zgemods/zobjcamera.h"
#include "zge/zmem.h"
#include "zge/zstring.h"

/*
 * Floating point follows the original x87 code: `double` is a value held in an x87 register, `float` is a
 * value the original stored to memory.
 */

static const char zoverFile[] = "C:\\zaxis\\Bmx\\src\\com\\zge\\bmx\\zoverii.c";

#define ZERO 0.0f
#define ONE  1.0f
#define HALF 0.5f

/* ---- timer ------------------------------------------------------------------------------------ */

typedef uint32_t(BMX_STDCALL *TimeGetTimeFn)(void);
#define gTimeGetTime  ORIG_VAR(0x00699838, TimeGetTimeFn, gTimeGetTime)
#define gLastPollTime ORIG_VAR(0x005746b0, uint32_t, gLastPollTime)

/*
 * Callback that runs the input poll at most every fourth millisecond; returns the elapsed time, or the
 * new time stamp when it polled. The five stdcall parameters are ignored.
 */
BMX_ADDR(0x00450020)
uint32_t BMX_STDCALL zoverPollTimer(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e)
{
    uint32_t t = gTimeGetTime() - gLastPollTime;
    if (t > 3) {
        zoverPollInput();
        t = gTimeGetTime();
        gLastPollTime = t;
    }
    return t;
}

/* ---- rig construction ------------------------------------------------------------------------- */

/* Allocates an empty rig from the static region (the remaining fields are left as they are). */
BMX_ADDR(0x00450060)
ZRig *zoverRigCreate(void)
{
    ZRig *rig = zmemStaticAlloc(sizeof(ZRig));
    rig->pointCount = 0;
    rig->constraintCount = 0;
    rig->onHit = NULL;
    rig->hingeCount = 0;
    return rig;
}

/*
 * Adds a point; returns its index, or -1 for an invalid mode. Original quirk: the mode is stored in the
 * next free slot even when it is rejected.
 */
BMX_ADDR(0x004500a0)
int zoverRigAddPoint(ZRig *rig, const ZRigPointDef *def)
{
    ZRigPoint *p = &rig->points[rig->pointCount];
    p->mode = def->mode;
    if (def->mode > 2) {
        return -1;
    }
    p->mass = def->mass;
    p->friction = def->friction;
    p->restitution = def->restitution;
    p->pos = def->pos;
    p->flags = 0;
    zvec3Zero(&p->accum);
    zvec3Zero(&p->vel);
    return (int)rig->pointCount++;
}

/* Adds a distance stick between two points, with the current distance as its rest length. */
BMX_ADDR(0x00450140)
int zoverRigAddStick(ZRig *rig, int a, int b, float stiffness)
{
    ZRigConstraint *c = &rig->constraints[rig->constraintCount];
    ZVec3 d;

    c->stiffness = stiffness;
    c->type = kZoverStick;
    c->a = &rig->points[a];
    c->b = &rig->points[b];
    d.x = c->a->pos.x - c->b->pos.x;
    d.y = c->a->pos.y - c->b->pos.y;
    d.z = c->a->pos.z - c->b->pos.z;
    c->u.stick.length = (float)zvec3Length(&d);
    return (int)rig->constraintCount++;
}

/* A stick that also reads the alignment of points c and d (see zoverRigSolveStickLinked). */
BMX_ADDR(0x00450210)
void zoverRigAddStickLinked(ZRig *rig, int a, int b, int c, int d, float stiffness, float alignGain)
{
    ZRigConstraint *k = &rig->constraints[rig->constraintCount];
    ZVec3 v;

    k->stiffness = stiffness;
    k->type = kZoverStickLinked;
    k->a = &rig->points[a];
    k->b = &rig->points[b];
    v.x = k->a->pos.x - k->b->pos.x;
    v.y = k->a->pos.y - k->b->pos.y;
    v.z = k->a->pos.z - k->b->pos.z;
    k->u.stick.length = (float)zvec3Length(&v);
    k->u.stick.c = &rig->points[c];
    k->u.stick.alignGain = alignGain;
    k->u.stick.d = &rig->points[d];
    rig->constraintCount++;
}

/*
 * Adds a corner constraint: it stores the three frame points as indices and the angle (arccosine of the
 * dot product) between the direction a to b and the z axis of the frame built from them. Relaxing a corner
 * does nothing in the shipped game.
 */
BMX_ADDR(0x00450300)
void zoverRigAddCorner(ZRig *rig, int a, int b, int i0, int i1, int i2, float stiffness)
{
    ZRigConstraint *c = &rig->constraints[rig->constraintCount];
    ZVec3 d;
    ZMatrix frame;

    c->stiffness = stiffness;
    c->type = kZoverCorner;
    c->a = &rig->points[a];
    c->b = &rig->points[b];
    d.x = c->b->pos.x - c->a->pos.x;
    d.y = c->b->pos.y - c->a->pos.y;
    d.z = c->b->pos.z - c->a->pos.z;
    zvec3NormalizeScaled(&d, &d);
    zoverRigBuildFrame(rig, i0, i1, i2, &frame);
    c->u.corner.index[0] = (uint8_t)i0;
    c->u.corner.index[1] = (uint8_t)i1;
    c->u.corner.index[2] = (uint8_t)i2;
    c->u.corner.angle = (uint16_t)zmathAcos((float)zvec3Dot(&d, &frame.at));
    rig->constraintCount++;
}

static int addHinge(ZRig *rig, const ZRigHingeDef *def, int type)
{
    ZRigHinge *h = &rig->hinges[rig->hingeCount];
    h->type = type;
    h->a = def->a;
    h->b = def->b;
    h->c = def->c;
    h->refA = def->refA;
    h->refB = def->refB;
    h->strength = def->strength;
    h->sign = def->sign;
    h->axisA = def->axisA;
    h->axisB = def->axisB;
    return (int)rig->hingeCount++;
}

/* The hinge adders return the number of the new hinge. */
BMX_ADDR(0x00450430)
int zoverRigAddHingeSwing(ZRig *rig, const ZRigHingeDef *def)
{
    return addHinge(rig, def, kZoverHingeSwing);
}

BMX_ADDR(0x004504c0)
int zoverRigAddHingeTwist(ZRig *rig, const ZRigHingeDef *def)
{
    return addHinge(rig, def, kZoverHingeTwist);
}

/* Adds a bone: only a and b are read from the definition; the strength field is the current distance. */
BMX_ADDR(0x00450550)
void zoverRigAddBone(ZRig *rig, const ZRigHingeDef *def)
{
    ZRigHinge *h = &rig->hinges[rig->hingeCount];
    ZVec3 d;

    h->type = kZoverHingeBone;
    h->a = def->a;
    h->b = def->b;
    d.x = def->a->pos.x - def->b->pos.x;
    d.y = def->a->pos.y - def->b->pos.y;
    d.z = def->a->pos.z - def->b->pos.z;
    h->strength = (float)zvec3Length(&d);
    rig->hingeCount++;
}

/* ---- hinges ----------------------------------------------------------------------------------- */

/* Copies the axis of `ref` that a hinge names (see ZRigHingeDef); the negated kinds are scaled by -1. */
static void hingeAxis(ZVec3 *out, const ZRigPoint *ref, int32_t axis)
{
    if (axis == 1) {
        *out = ref->basis.right;
    } else if (axis == 3) {
        *out = ref->basis.at;
    } else if (axis == 2) {
        *out = ref->basis.up;
    } else {
        if (axis == 4) {
            *out = ref->basis.right;
        } else if (axis == 6) {
            *out = ref->basis.at;
        } else {
            *out = ref->basis.up;
        }
        zvec3Scale(out, out, -1.0f);
    }
}

/* True when the hinge's sign rule says to leave the points alone for this projection. */
static int hingeSkips(int32_t sign, float f)
{
    if (sign == 1 && f > ZERO) {
        return 1;
    }
    return sign == 2 && !(f >= ZERO);
}

/*
 * Pushes a along refB's axis and b along refA's axis, in proportion to how far c's offset from a lies along
 * the axis; the lighter point moves more.
 */
BMX_ADDR(0x004505f0)
void zoverRigSolveHingeSwing(ZRigHinge *h)
{
    ZVec3 axisA, axisB, d;
    float f;
    double ratio;
    float s;

    hingeAxis(&axisA, h->refA, h->axisA);
    hingeAxis(&axisB, h->refB, h->axisB);
    d.x = h->a->pos.x - h->c->pos.x;
    d.y = h->a->pos.y - h->c->pos.y;
    d.z = h->a->pos.z - h->c->pos.z;
    f = (float)-zvec3Dot(&d, &axisB);
    if (hingeSkips(h->sign, f)) {
        return;
    }
    ratio = (double)h->a->mass / ((double)h->a->mass + h->b->mass);
    s = (float)(ratio * ((double)f * h->strength));
    zvec3Scale(&axisB, &axisB, s);
    if (h->a->mode != 0) {
        zvec3Add(&h->a->pos, &h->a->pos, &axisB);
    }
    d.x = h->b->pos.x - h->c->pos.x;
    d.y = h->b->pos.y - h->c->pos.y;
    d.z = h->b->pos.z - h->c->pos.z;
    {
        const double dot = zvec3Dot(&d, &axisA);
        ratio = (double)h->b->mass / ((double)h->b->mass + h->a->mass);
        s = (float)(ratio * (-dot * h->strength));
    }
    zvec3Scale(&axisA, &axisA, s);
    if (h->b->mode != 0) {
        zvec3Add(&h->b->pos, &h->b->pos, &axisA);
    }
}

/*
 * Like the swing hinge, but only b is pushed, along refA's axis, with the push clamped to length 4 when it
 * exceeds 16 squared. refB's axis is computed and then replaced by refA's.
 */
BMX_ADDR(0x00450990)
void zoverRigSolveHingeTwist(ZRigHinge *h)
{
    ZVec3 axisA, axisB, d;
    float f, s;
    double ratio;

    hingeAxis(&axisA, h->refA, h->axisA);
    hingeAxis(&axisB, h->refB, h->axisB);
    d.x = h->b->pos.x - h->c->pos.x;
    d.y = h->b->pos.y - h->c->pos.y;
    d.z = h->b->pos.z - h->c->pos.z;
    f = (float)-zvec3Dot(&d, &axisA);
    if (hingeSkips(h->sign, f)) {
        return;
    }
    axisB = axisA;
    ratio = (double)h->a->mass / ((double)h->a->mass + h->b->mass);
    s = (float)(ratio * ((double)f * h->strength));
    zvec3Scale(&axisA, &axisA, s);
    zvec3Scale(&axisB, &axisB, s);
    if (zvec3Dot(&axisB, &axisB) > 16.0f) {
        zvec3NormalizeScaled(&axisB, &axisB);
        zvec3Scale(&axisB, &axisB, 4.0f);
    }
    if (h->a->mode != 0) {
        zvec3Add(&h->b->pos, &h->b->pos, &axisB);
    }
}

/* Pulls b to the point `strength` below a along b's own up axis (a bone of fixed length), softly. */
BMX_ADDR(0x00450d30)
void zoverRigSolveBone(ZRigHinge *h)
{
    ZVec3 v = h->b->basis.up;

    zvec3Scale(&v, &v, -h->strength);
    zvec3Add(&v, &v, &h->a->pos);
    v.x = h->b->pos.x - v.x;
    v.y = h->b->pos.y - v.y;
    v.z = h->b->pos.z - v.z;
    zvec3Scale(&v, &v, -0x1.3d70a4p-5f);
    zvec3Add(&h->b->pos, &h->b->pos, &v);
}

/* Runs every hinge of the rig, unless hingesOff is set. */
BMX_ADDR(0x00450e00)
void zoverRigSolveHinges(ZRig *rig)
{
    if (rig->hingesOff != 0) {
        return;
    }
    for (uint32_t i = 0; i < rig->hingeCount; i++) {
        ZRigHinge *h = &rig->hinges[i];
        if (h->type == kZoverHingeSwing) {
            zoverRigSolveHingeSwing(h);
        } else if (h->type == kZoverHingeTwist) {
            zoverRigSolveHingeTwist(h);
        } else if (h->type == kZoverHingeBone) {
            zoverRigSolveBone(h);
        }
    }
}

/* ---- relaxation ------------------------------------------------------------------------------- */

/*
 * One frame of the rig. Free points are moved by their velocity (plus gravity), the hinges and two passes
 * of the sticks pull the points together as mass weighted position sums, then every point is collided with
 * the level (calling onHit) and the velocities and the centre of mass are derived from the result.
 */
BMX_ADDR(0x00450e80)
void zoverRigRelax(ZRig *rig)
{
    ZVec3 sum;
    float total;
    ZRigPoint *p;
    uint32_t i;

    for (i = 0; i < rig->pointCount; i++) {
        p = &rig->points[i];
        p->prev = p->pos;
        if (p->mode != 0) {
            p->vel.y = (float)(1.58f + (double)p->vel.y);
            p->pos.x = (float)((double)p->vel.x + p->pos.x);
            p->pos.y = (float)((double)p->vel.y + p->pos.y);
            p->pos.z = (float)((double)p->vel.z + p->pos.z);
        }
        p->pos.x = (float)((double)p->pos.x - rig->center.x);
        p->pos.y = (float)((double)p->pos.y - rig->center.y);
        p->pos.z = (float)((double)p->pos.z - rig->center.z);
    }
    zoverRigSolveHinges(rig);
    for (int pass = 0; pass < 2; pass++) {
        for (i = 0; i < rig->pointCount; i++) {
            p = &rig->points[i];
            p->weight = p->mass;
            p->accum.x = (float)((double)p->pos.x * p->mass);
            p->accum.y = (float)((double)p->pos.y * p->mass);
            p->accum.z = (float)((double)p->pos.z * p->mass);
        }
        for (i = 0; i < rig->constraintCount; i++) {
            ZRigConstraint *c = &rig->constraints[i];
            if (c->type == kZoverStick) {
                zoverRigSolveStick(c);
            } else if (c->type == kZoverStickLinked) {
                zoverRigSolveStickLinked(c);
                zoverRigSolveStick(c);
            } else if (c->type == kZoverCorner) {
                zoverRigSolveCorner(c);
            }
        }
        for (i = 0; i < rig->pointCount; i++) {
            p = &rig->points[i];
            if (p->mode != 0) {
                const double inv = (double)ONE / p->weight;
                p->pos.x = (float)(inv * p->accum.x);
                p->pos.y = (float)(inv * p->accum.y);
                p->pos.z = (float)(inv * p->accum.z);
            }
        }
    }
    zvec3Zero(&sum);
    total = 0.0f;
    if (rig->pointCount != 0) {
        for (i = 0; i < rig->pointCount; i++) {
            p = &rig->points[i];
            if (p->mode != 0) {
                uint32_t hitId;
                uint8_t hit[0x100];

                p->pos.x = (float)((double)rig->center.x + p->pos.x);
                p->pos.y = (float)((double)p->pos.y + rig->center.y);
                p->pos.z = (float)((double)rig->center.z + p->pos.z);
                p->vel.x = (float)((double)p->pos.x - p->prev.x);
                p->vel.y = (float)((double)p->pos.y - p->prev.y);
                p->vel.z = (float)((double)p->pos.z - p->prev.z);
                if (zoverRigCollidePoint(p, &hitId, hit) != 0 && rig->onHit != NULL) {
                    rig->onHit(rig, i, hitId, hit);
                }
                p->pos.x = (float)((double)p->pos.x - rig->center.x);
                p->pos.y = (float)((double)p->pos.y - rig->center.y);
                p->pos.z = (float)((double)p->pos.z - rig->center.z);
            }
            if (p->mode != 0) {
                sum.x = (float)((double)p->mass * p->pos.x + sum.x);
                sum.y = (float)((double)p->pos.y * p->mass + sum.y);
                sum.z = (float)((double)p->mass * p->pos.z + sum.z);
                total = (float)((double)total + p->mass);
                p->pos.x = (float)((double)rig->center.x + p->pos.x);
                p->pos.y = (float)((double)rig->center.y + p->pos.y);
                p->pos.z = (float)((double)rig->center.z + p->pos.z);
            } else {
                p->pos = p->prev;
            }
            if ((p->flags & kZoverPointFrozen) == 0) {
                p->vel.x = (float)((double)p->pos.x - p->prev.x);
                p->vel.y = (float)((double)p->pos.y - p->prev.y);
                p->vel.z = (float)((double)p->pos.z - p->prev.z);
            }
        }
        if (total > ZERO) {
            zvec3Scale(&sum, &sum, (float)((double)ONE / total));
        }
    }
    zvec3Add(&rig->center, &rig->center, &sum);
}

BMX_ADDR(0x00451280)
void zoverRigSetHitCallback(ZRig *rig, ZRigHitFn fn)
{
    rig->onHit = fn;
}

/* ---- sticks ----------------------------------------------------------------------------------- */

/*
 * Frame from three points: the x axis points from the third point's neighbourhood to the first, the z axis
 * from the third to the second, y is their cross product; the position row is zero.
 */
BMX_ADDR(0x004512a0)
void zoverRigBuildFrame(ZRig *rig, int a, int b, int c, ZMatrix *out)
{
    const ZRigPoint *pa = &rig->points[a];
    const ZRigPoint *pb = &rig->points[b];
    const ZRigPoint *pc = &rig->points[c];

    out->at.x = pb->pos.x - pc->pos.x;
    out->at.y = pb->pos.y - pc->pos.y;
    out->at.z = pb->pos.z - pc->pos.z;
    out->right.x = (float)((double)pa->pos.x - ((double)out->at.x * HALF + pc->pos.x));
    out->right.y = (float)((double)pa->pos.y - ((double)out->at.y * HALF + pc->pos.y));
    out->right.z = (float)((double)pa->pos.z - ((double)out->at.z * HALF + pc->pos.z));
    zvec3NormalizeScaled(&out->right, &out->right);
    zvec3NormalizeScaled(&out->at, &out->at);
    zvec3Zero(&out->pos);
    zvec3Cross3(&out->up, &out->right, &out->pos, &out->at);
}

/*
 * Projects the segment a-b on the axis n: *outMid gets half the sum of the two projections, *outDepth
 * half the absolute projection of the segment plus `depth`.
 */
BMX_ADDR(0x00451c80)
void zoverRigProject(const ZVec3 *a, const ZVec3 *b, float depth, float *outMid, float *outDepth,
                     const ZVec3 *n)
{
    const float r1 = (float)zvec3Dot(a, n);
    const double r2 = zvec3Dot(b, n);
    const float mid = (float)((r2 + r1) * HALF);
    ZVec3 v = *a;
    double r3;

    zvec3Negate(&v, &v);
    zvec3Add(&v, &v, b);
    r3 = zvec3Dot(&v, n);
    if (!(r3 >= ZERO)) {
        r3 = -r3;
    }
    *outMid = mid;
    *outDepth = (float)(r3 * HALF + depth);
}

/* Pulls a and b towards the rest length: both get a weighted target position added to their sums. */
BMX_ADDR(0x00452370)
void zoverRigSolveStick(ZRigConstraint *c)
{
    ZRigPoint *a = c->a;
    ZRigPoint *b = c->b;
    ZVec3 d, v;

    d.x = a->pos.x - b->pos.x;
    d.y = a->pos.y - b->pos.y;
    d.z = a->pos.z - b->pos.z;
    zvec3NormalizeScaled(&d, &d);
    zvec3Scale(&d, &d, c->u.stick.length);
    v.x = (float)((double)d.x + b->pos.x);
    v.y = (float)((double)d.y + b->pos.y);
    v.z = (float)((double)d.z + b->pos.z);
    zvec3Scale(&v, &v, c->stiffness);
    zvec3Add(&a->accum, &a->accum, &v);
    v.x = (float)((double)a->pos.x - d.x);
    v.y = (float)((double)a->pos.y - d.y);
    v.z = (float)((double)a->pos.z - d.z);
    zvec3Scale(&v, &v, c->stiffness);
    zvec3Add(&b->accum, &b->accum, &v);
    a->weight = (float)((double)a->weight + c->stiffness);
    b->weight = (float)((double)b->weight + c->stiffness);
}

/*
 * Like zoverRigSolveStick, but the stiffness grows by alignGain times how much the up axis of point c
 * opposes the right axis of point d. (The stick proper is relaxed by a second zoverRigSolveStick call.)
 */
BMX_ADDR(0x004524d0)
void zoverRigSolveStickLinked(ZRigConstraint *c)
{
    ZRigPoint *a = c->a;
    ZRigPoint *b = c->b;
    float s = c->stiffness;
    const double align = -zvec3Dot(&c->u.stick.c->basis.up, &c->u.stick.d->basis.right);
    ZVec3 d, v;

    if (align > ZERO) {
        s = (float)(align * c->u.stick.alignGain + s);
    }
    d.x = a->pos.x - b->pos.x;
    d.y = a->pos.y - b->pos.y;
    d.z = a->pos.z - b->pos.z;
    zvec3NormalizeScaled(&d, &d);
    zvec3Scale(&d, &d, c->u.stick.length);
    v = d;
    zvec3Add(&v, &v, &b->pos);
    zvec3Scale(&v, &v, s);
    zvec3Add(&a->accum, &a->accum, &v);
    v.x = (float)((double)a->pos.x - d.x);
    v.y = (float)((double)a->pos.y - d.y);
    v.z = (float)((double)a->pos.z - d.z);
    zvec3Scale(&v, &v, s);
    zvec3Add(&b->accum, &b->accum, &v);
    a->weight = (float)((double)s + a->weight);
    b->weight = (float)((double)s + b->weight);
}

BMX_ADDR(0x00452680)
void zoverRigSolveCorner(ZRigConstraint *c) {}

/* A three point rig (a triangle of sticks); returns NULL if no memory. */
BMX_ADDR(0x004526a0)
ZRig *zoverRigCreateTriangle(void)
{
    ZRig *rig = zoverRigCreate();
    ZRigPointDef def;

    if (rig != NULL) {
        def.mode = 2;
        def.mass = 0.1f;
        def.friction = 0.8f;
        def.restitution = 0.4f;
        def.pos.x = 0.0f;
        def.pos.y = 40.0f;
        def.pos.z = 0.0f;
        zoverRigAddPoint(rig, &def);
        def.pos.x = 30.0f;
        def.pos.y = 0.0f;
        def.pos.z = 0.0f;
        zoverRigAddPoint(rig, &def);
        def.pos.x = -30.0f;
        def.pos.y = 0.0f;
        def.pos.z = 0.0f;
        zoverRigAddPoint(rig, &def);
        zoverRigAddStick(rig, 0, 1, 16.0f);
        zoverRigAddStick(rig, 0, 2, 16.0f);
        zoverRigAddStick(rig, 1, 2, 16.0f);
    }
    return rig;
}

/* ---- bike and rider motion -------------------------------------------------------------------- */

/*
 * Starts the bike rig: the three points are placed around the bike actor's position in its matrix
 * (offsets (0,40,0), (30,0,0), (-30,0,0)); the velocity of each is vel plus the movement a rotation by
 * `rot` would cause, halved. Points past the third reuse the last offset (the original's stack scratch).
 */
BMX_ADDR(0x004527c0)
void zoverStartBikeMotion(void *rigp, void *bike, const ZMatrix *m, const ZVec3 *vel, const ZAngle3 *rot)
{
    ZRig *rig = rigp;
    ZActor *actor = bike;
    const ZVec3 *apos = zactorGetPos(actor);
    ZVec3 v, t[kZoverRigMaxPoints], w[kZoverRigMaxPoints];

    for (uint32_t i = 0; i < rig->pointCount; i++) {
        ZRigPoint *p = &rig->points[i];

        if (i == 0) {
            v.x = ZERO;
            v.y = 40.0f;
            v.z = ZERO;
        } else if (i == 1) {
            v.x = 30.0f;
            v.y = ZERO;
            v.z = ZERO;
        } else if (i == 2) {
            v.x = -30.0f;
            v.y = ZERO;
            v.z = ZERO;
        }
        zmatrixTransformPoint(&t[i], m, &v);
        p->pos.x = (float)((double)apos->x + t[i].x);
        p->pos.y = (float)((double)apos->y + t[i].y);
        p->pos.z = (float)((double)t[i].z + apos->z);
        w[i] = t[i];
        zvec3RotateEuler(&w[i], &w[i], rot);
        p->vel.x = w[i].x - t[i].x;
        p->vel.y = w[i].y - t[i].y;
        p->vel.z = w[i].z - t[i].z;
        p->vel.x = (float)((double)vel->x + p->vel.x);
        p->vel.y = (float)((double)vel->y + p->vel.y);
        p->vel.z = (float)((double)vel->z + p->vel.z);
        zvec3Scale(&p->vel, &p->vel, HALF);
        p->prev = p->pos;
        zmatrixCopyRotation(&p->basis, m);
        p->flags = 0;
    }
    rig->center = *apos;
    actor->rot[0] = 0;
    actor->rot[1] = 0;
    actor->rot[2] = 0;
    actor->heading = 0;
}

/*
 * Relaxes the bike rig and sets the bike actor from it: the frame of the three points becomes the actor's
 * orientation (axes permuted: x takes the old z, y the old x, z the old y), the centre its position.
 */
BMX_ADDR(0x00452a40)
void zoverStepBikeMotion(void *rigp, void *bike)
{
    ZRig *rig = rigp;
    ZActor *actor = bike;
    ZMatrix m;
    ZVec3 right, up, at;
    ZAngle3 zero = {0, 0, 0};

    zoverRigRelax(rig);
    zoverRigBuildFrame(rig, 0, 1, 2, &m);
    right = m.right;
    up = m.up;
    at = m.at;
    m.right = at;
    m.up = right;
    m.at = up;
    zvec3Zero(&m.pos);
    for (int i = 0; i < 3; i++) {
        zmatrixCopyRotation(&rig->points[i].basis, &m);
    }
    zactorOrient(actor, &zero, &m.up, &m.right, &m);
    zactorSetPos(actor, &rig->center);
    zactorSetHeight(actor, 0.0f);
}

/* Zeroes three joint angles. */
BMX_ADDR(0x00455bf0)
void zoverZeroAngles(ZAngle3 *angles)
{
    angles->x = 0;
    angles->y = 0;
    angles->z = 0;
}

/*
 * Places the rig's 14 points at the rider's bones (hips, spine, head, limbs; the last one 25 units below
 * its bone) and clears the pose of the blend channel.
 */
BMX_ADDR(0x00452b80)
void zoverRigLoadPose(ZRig *rig, ZActor *rider)
{
    static const uint8_t bones[14] = {1, 0xc, 9, 0xd, 0xa, 0xe, 0xb, 6, 3, 7, 4, 8, 5, 2};
    ZMotionChannel *ch = &rider->player->ch[1];
    ZVec3 v;

    for (int j = 0; j < 15; j++) {
        zoverZeroAngles(&ch->joints[j]);
    }
    for (int i = 0; i < (int32_t)rig->pointCount; i++) {
        zvec3Zero(&v);
        if (i > 13) {
            continue;
        }
        if (i == 13) {
            v.y = -25.0f;
        }
        zactorGetBonePos(rider, bones[i], &v, &rig->points[i].pos, 1);
    }
}

static void defineHinge(ZRigHingeDef *d, int32_t axisA, int32_t axisB, ZRigPoint *a, ZRigPoint *b,
                        ZRigPoint *c, int32_t sign, float strength)
{
    d->axisA = axisA;
    d->axisB = axisB;
    d->a = a;
    d->b = b;
    d->c = c;
    d->refA = a;
    d->refB = b;
    d->sign = sign;
    d->strength = strength;
}

/*
 * Builds the rider's rig from the rider actor's pose: 14 points (the first one heavier), the sticks that
 * hold the body together, six hinges for the joints and the back, then lifts the whole rig so that its
 * lowest point is at height 0. The numbers of the sticks and hinges the state functions adjust are kept
 * in gZoverStickIdx, gZoverBackIdx and gZoverHingeIdx. `flags` bit 0 makes the points grip (friction 1.75
 * instead of 0.6). The rig goes into the next slot of the rig table, even when the allocation failed.
 */
BMX_ADDR(0x00452e80)
ZRig *zoverRigCreateRider(ZActor *rider, uint32_t flags)
{
    float lowest = 500.0f;
    ZRig *rig = zoverRigCreate();

    if (rig != NULL) {
        ZRigPointDef def;
        ZRigHingeDef hinge;
        ZRigPoint *const p = rig->points;

        memset(&def, 0, sizeof def);
        def.mode = 1;
        def.mass = 0.4f;
        def.friction = (flags & 1) != 0 ? 1.75f : 0.6f;
        def.restitution = 1.0f;
        zoverRigAddPoint(rig, &def);
        def.mass = 0.1f;
        for (int i = 1; i < 14; i++) {
            zoverRigAddPoint(rig, &def);
        }
        zoverRigLoadPose(rig, rider);

        zoverRigAddStick(rig, 0, 1, 24.0f);
        zoverRigAddStick(rig, 0, 2, 24.0f);
        zoverRigAddStick(rig, 1, 2, 24.0f);
        zoverRigAddStick(rig, 7, 0, 48.0f);
        zoverRigAddStick(rig, 8, 0, 48.0f);
        zoverRigAddStick(rig, 7, 8, 24.0f);
        zoverRigAddStick(rig, 7, 1, 48.0f);
        zoverRigAddStick(rig, 7, 2, 48.0f);
        zoverRigAddStick(rig, 8, 1, 48.0f);
        zoverRigAddStick(rig, 8, 2, 48.0f);
        gZoverStickIdx[5] = zoverRigAddStick(rig, 2, 4, 24.0f);
        gZoverStickIdx[7] = zoverRigAddStick(rig, 4, 6, 24.0f);
        gZoverStickIdx[4] = zoverRigAddStick(rig, 1, 3, 24.0f);
        gZoverStickIdx[6] = zoverRigAddStick(rig, 3, 5, 24.0f);
        gZoverStickIdx[1] = zoverRigAddStick(rig, 8, 10, 24.0f);
        gZoverStickIdx[3] = zoverRigAddStick(rig, 10, 12, 24.0f);
        gZoverStickIdx[0] = zoverRigAddStick(rig, 7, 9, 24.0f);
        gZoverStickIdx[2] = zoverRigAddStick(rig, 9, 11, 24.0f);
        gZoverBackIdx[0] = zoverRigAddStick(rig, 13, 0, 4.8f);
        gZoverBackIdx[0] = zoverRigAddStick(rig, 13, 7, 24.0f);
        gZoverBackIdx[0] = zoverRigAddStick(rig, 13, 8, 24.0f);

        defineHinge(&hinge, 4, 2, &p[8], &p[9], &p[7], 2, 1.2f);
        gZoverHingeIdx[4] = zoverRigAddHingeSwing(rig, &hinge);
        defineHinge(&hinge, 1, 2, &p[7], &p[10], &p[8], 2, 1.2f);
        gZoverHingeIdx[5] = zoverRigAddHingeSwing(rig, &hinge);
        defineHinge(&hinge, 4, 2, &p[2], &p[3], &p[1], 2, 0.5f);
        gZoverHingeIdx[1] = zoverRigAddHingeSwing(rig, &hinge);
        defineHinge(&hinge, 1, 2, &p[1], &p[4], &p[2], 2, 0.5f);
        gZoverHingeIdx[3] = zoverRigAddHingeSwing(rig, &hinge);
        defineHinge(&hinge, 2, 5, &p[2], &p[3], &p[1], 1, 1.0f);
        gZoverHingeIdx[0] = zoverRigAddHingeTwist(rig, &hinge);
        defineHinge(&hinge, 2, 2, &p[1], &p[4], &p[2], 1, 1.0f);
        gZoverHingeIdx[2] = zoverRigAddHingeTwist(rig, &hinge);

        for (uint32_t i = 0; i < rig->pointCount; i++) {
            if (!(p[i].pos.y >= lowest)) {
                lowest = p[i].pos.y;
            }
        }
        for (uint32_t i = 0; i < rig->pointCount; i++) {
            p[i].pos.y = (float)((double)p[i].pos.y - (double)lowest);
        }
    }
    gZoverRigs[gZoverRigNext] = rig;
    gZoverRigNext++;
    if (gZoverRigNext > 1) {
        gZoverRigNext = 0;
    }
    return rig;
}

/* ---- rider state ------------------------------------------------------------------------------ */

BMX_ADDR(0x00453460)
void zoverRigSelect(ZRig *rig)
{
    for (int i = 0; i < 2; i++) {
        if (gZoverRigs[i] == rig) {
            gZoverRigIndex = i;
            return;
        }
    }
}

/*
 * Starts the rider rig from the rider actor's pose, with every point moving at half of vel; the rig goes
 * free with the hinges off, and the limp, stiff and hand states are reset. Only the first four parameters
 * of the original are read.
 */
BMX_ADDR(0x004534a0)
void zoverStartRiderMotion(void *rigp, void *riderp, const ZMatrix *m, const ZVec3 *vel, const ZAngle3 *rot)
{
    ZRig *rig = rigp;
    ZActor *rider = riderp;

    zoverRigLoadPose(rig, rider);
    for (int i = 0; i < (int32_t)rig->pointCount; i++) {
        ZRigPoint *p = &rig->points[i];
        p->vel = *vel;
        zvec3Scale(&p->vel, &p->vel, HALF);
        p->prev = p->pos;
        p->flags = 0;
        p->mode = kZoverPointFree;
    }
    zoverRigSelect(rig);
    rig->center = rig->points[0].pos;
    rider->rot[0] = 0;
    rider->rot[1] = 0;
    rider->rot[2] = 0;
    rider->heading = 0;
    rig->hingesOff = 1;
    zoverRigEndStiff(rig);
    zoverRigReleaseHands(rig);
    zoverRigEndLimp(rig);
    gZoverHandsGrabbed[gZoverRigIndex] = 0;
}

/*
 * Hands-off update after a bail: unless the hands hold an edge, the rig drops back to its normal state
 * (the limp, stiff, pitch and yaw requests are ended) and the hands count as eased.
 */
BMX_ADDR(0x004535d0)
void zoverRigEase(ZRig *rig, ZActor *rider)
{
    zoverRigSelect(rig);
    if (gZoverHandsGrabbed[gZoverRigIndex] != 0) {
        return;
    }
    gZoverHandsEased[gZoverRigIndex] = 1;
    if (gZoverLimp[gZoverRigIndex] != 0) {
        zoverRigEndLimp(rig);
    }
    if (gZoverStiff[gZoverRigIndex] != 0) {
        zoverRigEndStiff(rig);
    }
    if (gZoverPitchOn[gZoverRigIndex] != 0) {
        zoverRigClearPitch(rig);
    }
    if (gZoverYawOn[gZoverRigIndex] != 0) {
        zoverRigClearYaw(rig);
    }
}

/*
 * Lets the two hand points (11 and 12) grab the nearest edge within 400 units: a hand moves to the
 * closest edge point (10 units up) and becomes pinned. Does nothing while the hands already hold.
 */
BMX_ADDR(0x004536a0)
void zoverRigGrabEdges(ZRig *rig)
{
    void **edges;

    if (gZoverHandsHolding[gZoverRigIndex] != 0 || gZoverHandsGrabbed[gZoverRigIndex] != 0) {
        return;
    }
    edges = zbspFindEdgesNear(&rig->points[3].pos, 400.0f);
    if (edges == NULL) {
        return;
    }
    for (int hand = 11; hand < 13; hand++) {
        ZRigPoint *p = &rig->points[hand];
        uint32_t best = 0x3840;
        ZVec3 bestPos;

        for (void **e = edges; *e != NULL; e++) {
            const uint8_t *edge = *e;
            ZVec3 q;
            int dx, dy, dz;
            uint32_t score;

            zbspClosestPointOnEdge((const int16_t *)(edge + 8), (const int16_t *)(edge + 0xe),
                                   (const ZVec3 *)(edge + 0x18), &p->pos, &q);
            dx = bmxFtol((double)q.x - p->pos.x);
            dy = bmxFtol((double)q.y - p->pos.y);
            dz = bmxFtol((double)q.z - p->pos.z);
            score = (uint32_t)(dz * dz + dy * dy + dx * dx);
            if (score < best) {
                bestPos = q;
                best = score;
            }
        }
        if (best < 0x3840) {
            p->pos.x = bestPos.x;
            p->pos.y = (float)(-10.0f + (double)bestPos.y);
            p->pos.z = bestPos.z;
            p->mode = kZoverPointPinned;
            gZoverHandsHolding[gZoverRigIndex] = 1;
            gZoverHandsGrabbed[gZoverRigIndex] = 1;
        }
    }
}

BMX_ADDR(0x00453870)
int zoverRigHandsHolding(void)
{
    return gZoverHandsHolding[gZoverRigIndex];
}

/* Frees the hand points again. */
BMX_ADDR(0x00453890)
void zoverRigReleaseHands(ZRig *rig)
{
    zoverRigSelect(rig);
    rig->points[11].mode = kZoverPointFree;
    rig->points[12].mode = kZoverPointFree;
    gZoverHandsEased[gZoverRigIndex] = 0;
    gZoverHandsHolding[gZoverRigIndex] = 0;
}

/* a = 0.8 a + 0.2 b (b is scaled in place as well). */
BMX_ADDR(0x004538e0)
void zoverBlendVec(ZVec3 *a, ZVec3 *b)
{
    zvec3Scale(b, b, 0x1.99999ap-3f);
    zvec3Scale(a, a, 0x1.99999ap-1f);
    zvec3Add(a, a, b);
}

/* Eases the four hand points (9 to 12) towards spots in front of the elbows (points 7 and 8). */
BMX_ADDR(0x00453920)
void zoverRigAimHands(ZRig *rig)
{
    ZVec3 v, p, q;

    zoverRigSelect(rig);
    v.x = 0.0f;
    v.y = 0.0f;
    v.z = 54.0f;
    zmatrixTransformPoint(&p, &rig->points[7].basis, &v);
    q = p;
    zvec3Add(&p, &p, &rig->points[7].pos);
    zvec3Add(&q, &q, &rig->points[8].pos);
    zoverBlendVec(&rig->points[11].pos, &p);
    zoverBlendVec(&rig->points[12].pos, &q);
    v.x = 0.0f;
    v.y = 0.0f;
    v.z = 29.0f;
    zmatrixTransformPoint(&p, &rig->points[7].basis, &v);
    q = p;
    zvec3Add(&p, &p, &rig->points[7].pos);
    zvec3Add(&q, &q, &rig->points[8].pos);
    zoverBlendVec(&rig->points[9].pos, &p);
    zoverBlendVec(&rig->points[10].pos, &q);
}

/* Stiff mode: the limp and eased states end, two hinges (4 and 5 of the table) are switched off. */
BMX_ADDR(0x00453ab0)
void zoverRigGoStiff(ZRig *rig, ZActor *rider)
{
    zoverRigSelect(rig);
    if (gZoverLimp[gZoverRigIndex] != 0) {
        zoverRigEndLimp(rig);
    }
    if (gZoverHandsEased[gZoverRigIndex] != 0) {
        zoverRigReleaseHands(rig);
    }
    gZoverStiff[gZoverRigIndex] = 1;
    rig->hinges[gZoverHingeIdx[4]].strength = 0.0f;
    rig->hinges[gZoverHingeIdx[5]].strength = 0.0f;
}

/* Ends stiff mode: four sticks get their stiffness (24) and the two hinges their strength (1) back. */
BMX_ADDR(0x00453b70)
void zoverRigEndStiff(ZRig *rig)
{
    zoverRigSelect(rig);
    rig->constraints[gZoverStickIdx[0]].stiffness = 24.0f;
    rig->constraints[gZoverStickIdx[1]].stiffness = 24.0f;
    rig->constraints[gZoverStickIdx[2]].stiffness = 24.0f;
    rig->constraints[gZoverStickIdx[3]].stiffness = 24.0f;
    rig->hinges[gZoverHingeIdx[4]].strength = 1.0f;
    rig->hinges[gZoverHingeIdx[5]].strength = 1.0f;
    gZoverStiff[gZoverRigIndex] = 0;
}

/*
 * Pulls eight points towards spots in the frames of the hips and the two thighs: the hand and foot points
 * (9 to 12) relative to the hip frame (point 0, shifted to points 7 and 8), the points 3 to 6 relative to
 * points 1 and 2.
 */
BMX_ADDR(0x00453c30)
void zoverRigBlendToPose(ZRig *rig)
{
    ZRigPoint *pt = rig->points;
    ZVec3 v, p, q;

    v.x = 16.76f;
    v.y = -25.205f;
    v.z = 13.824f;
    zmatrixTransformPoint(&p, &pt[0].basis, &v);
    zvec3Add(&p, &p, &pt[7].pos);
    zoverBlendVec(&pt[9].pos, &p);
    v.x = -8.498f;
    v.y = -29.081f;
    v.z = 13.993f;
    zmatrixTransformPoint(&q, &pt[0].basis, &v);
    zvec3Add(&q, &q, &pt[8].pos);
    zoverBlendVec(&pt[10].pos, &q);
    v.x = 14.058f;
    v.y = -26.699f;
    v.z = 36.762f;
    zmatrixTransformPoint(&p, &pt[0].basis, &v);
    zvec3Add(&p, &p, &pt[7].pos);
    zoverBlendVec(&pt[11].pos, &p);
    v.x = -6.603f;
    v.y = -32.332f;
    v.z = 36.852f;
    zmatrixTransformPoint(&q, &pt[0].basis, &v);
    zvec3Add(&q, &q, &pt[8].pos);
    zoverBlendVec(&pt[12].pos, &q);
    v.x = 0.0f;
    v.y = 29.405f;
    v.z = 40.043f;
    zmatrixTransformPoint(&p, &pt[1].basis, &v);
    zvec3Add(&p, &p, &pt[1].pos);
    zoverBlendVec(&pt[3].pos, &p);
    v.x = 0.0f;
    v.y = 27.904f;
    v.z = 13.288f;
    zmatrixTransformPoint(&q, &pt[2].basis, &v);
    zvec3Add(&q, &q, &pt[2].pos);
    zoverBlendVec(&pt[4].pos, &q);
    v.x = 0.0f;
    v.y = -7.461f;
    v.z = 60.906f;
    zmatrixTransformPoint(&p, &pt[1].basis, &v);
    zvec3Add(&p, &p, &pt[1].pos);
    zoverBlendVec(&pt[5].pos, &p);
    v.x = 0.0f;
    v.y = -9.087f;
    v.z = 37.628f;
    zmatrixTransformPoint(&q, &pt[2].basis, &v);
    zvec3Add(&q, &q, &pt[2].pos);
    zoverBlendVec(&pt[6].pos, &q);
}

/*
 * Limp mode: ends the stiff and eased states; unless a point is frozen, every stick and hinge loses its
 * strength.
 */
BMX_ADDR(0x00453f80)
void zoverRigGoLimp(ZRig *rig, ZActor *rider)
{
    zoverRigSelect(rig);
    if (gZoverStiff[gZoverRigIndex] != 0) {
        zoverRigEndStiff(rig);
    }
    if (gZoverHandsEased[gZoverRigIndex] != 0) {
        zoverRigReleaseHands(rig);
    }
    gZoverLimp[gZoverRigIndex] = 1;
    for (uint32_t i = 0; i < rig->pointCount; i++) {
        if ((rig->points[i].flags & kZoverPointFrozen) != 0) {
            return;
        }
    }
    for (int i = 0; i < 8; i++) {
        rig->constraints[gZoverStickIdx[i]].stiffness = 0.0f;
    }
    rig->hinges[gZoverHingeIdx[1]].strength = 0.0f;
    rig->hinges[gZoverHingeIdx[3]].strength = 0.0f;
    rig->hinges[gZoverHingeIdx[0]].strength = 0.0f;
    rig->hinges[gZoverHingeIdx[2]].strength = 0.0f;
    rig->hinges[gZoverHingeIdx[4]].strength = 0.0f;
    rig->hinges[gZoverHingeIdx[5]].strength = 0.0f;
}

/* Ends limp mode: hinges at strength 1, the three back sticks and the eight limb sticks stiff again. */
BMX_ADDR(0x00454140)
void zoverRigEndLimp(ZRig *rig)
{
    zoverRigSelect(rig);
    rig->hinges[gZoverHingeIdx[0]].strength = 1.0f;
    rig->hinges[gZoverHingeIdx[2]].strength = 1.0f;
    rig->hinges[gZoverHingeIdx[1]].strength = 1.0f;
    rig->hinges[gZoverHingeIdx[3]].strength = 1.0f;
    rig->hinges[gZoverHingeIdx[4]].strength = 1.0f;
    rig->hinges[gZoverHingeIdx[5]].strength = 1.0f;
    rig->constraints[gZoverBackIdx[0]].stiffness = 4.8f;
    rig->constraints[gZoverBackIdx[1]].stiffness = 24.0f;
    rig->constraints[gZoverBackIdx[2]].stiffness = 24.0f;
    for (int i = 0; i < 8; i++) {
        rig->constraints[gZoverStickIdx[i]].stiffness = 24.0f;
    }
    gZoverLimp[gZoverRigIndex] = 0;
}

BMX_ADDR(0x004542d0)
void zoverRigClearHinges(ZRig *rig)
{
    rig->hingeCount = 0;
}

BMX_ADDR(0x004542f0)
void zoverRigRequestSticky(ZRig *rig)
{
    zoverRigSelect(rig);
    gZoverSticky[gZoverRigIndex] = 1;
}

/* Back to the six hinges and the default surface (friction 0.6, restitution 1) on every point. */
BMX_ADDR(0x00454320)
void zoverRigClearSticky(ZRig *rig)
{
    zoverRigSelect(rig);
    rig->hingeCount = 6;
    for (uint32_t i = 0; i < rig->pointCount; i++) {
        rig->points[i].friction = 0.6f;
        rig->points[i].restitution = 1.0f;
    }
    gZoverSticky[gZoverRigIndex] = 0;
}

/* Pins every frozen point and stops it. */
BMX_ADDR(0x004543b0)
void zoverRigFreezeFlagged(ZRig *rig)
{
    for (uint32_t i = 0; i < rig->pointCount; i++) {
        ZRigPoint *p = &rig->points[i];
        if ((p->flags & kZoverPointFrozen) != 0) {
            p->mode = kZoverPointPinned;
            p->vel.x = 0.0f;
            p->vel.y = 0.0f;
            p->vel.z = 0.0f;
        }
    }
}

BMX_ADDR(0x00454400)
void zoverRigRequestRelaxed(ZRig *rig)
{
    zoverRigSelect(rig);
    gZoverRelaxed[gZoverRigIndex] = 1;
}

/* Back to the six hinges with every point free. */
BMX_ADDR(0x00454430)
void zoverRigClearRelaxed(ZRig *rig)
{
    zoverRigSelect(rig);
    rig->hingeCount = 6;
    for (uint32_t i = 0; i < rig->pointCount; i++) {
        rig->points[i].mode = kZoverPointFree;
    }
    gZoverRelaxed[gZoverRigIndex] = 0;
}

/* out = a - b (vector). */
BMX_ADDR(0x00455b20)
void zoverVecSub(const ZVec3 *a, const ZVec3 *b, ZVec3 *out)
{
    out->x = a->x - b->x;
    out->y = a->y - b->y;
    out->z = a->z - b->z;
}

/* Out = the transpose of the 3x3 part of in (the other elements are left alone). */
BMX_ADDR(0x00455c20)
void zoverTransposeRotation(const ZMatrix *in, ZMatrix *out)
{
    out->right.x = in->right.x;
    out->right.y = in->up.x;
    out->right.z = in->at.x;
    out->up.x = in->right.y;
    out->up.y = in->up.y;
    out->up.z = in->at.y;
    out->at.x = in->right.z;
    out->at.y = in->up.z;
    out->at.z = in->at.z;
}

/*
 * Rotates the rig's limb points (1 to 12) about point 0 by rot, in the frame of point 0's basis, and
 * settles them against the level. Original quirk: the rider parameter is unused.
 */
BMX_ADDR(0x004544b0)
void zoverRigApplyRotation(ZRig *rig, ZActor *rider, const ZMatrix *rot)
{
    ZMatrix a, b;
    ZVec3 tmp[12];

    zmatrixIdentity(&a);
    zmatrixIdentity(&b);
    zoverTransposeRotation(&rig->points[0].basis, &b);
    zmatrixCopyRotation(&a, &rig->points[0].basis);
    zmatrixMul(&a, &a, rot);
    zmatrixMul(&a, &a, &b);
    for (int i = 0; i < 12; i++) {
        ZRigPoint *p = &rig->points[1 + i];

        p->prev = p->pos;
        zoverVecSub(&p->pos, &rig->points[0].pos, &tmp[i]);
        zmatrixTransformPoint(&p->pos, &a, &tmp[i]);
        zvec3Add(&p->pos, &p->pos, &rig->points[0].pos);
        zoverRigResolvePoint(p);
        p->prev = p->pos;
    }
}

/* Applies the stored pitch (a rotation about x) of the current rig. */
BMX_ADDR(0x004545e0)
void zoverRigApplyPitch(ZRig *rig, ZActor *rider)
{
    ZAngle3 angles = {gZoverPitch[gZoverRigIndex], 0, 0};
    ZMatrix m;

    zmatrixIdentity(&m);
    zmatrixRotateEuler(&m, &m, &angles);
    zoverRigApplyRotation(rig, rider, &m);
}

BMX_ADDR(0x00454660)
void zoverRigRequestPitch(ZRig *rig, ZActor *rider, int16_t angle)
{
    zoverRigSelect(rig);
    gZoverPitchOn[gZoverRigIndex] = 1;
    gZoverPitch[gZoverRigIndex] = angle;
}

BMX_ADDR(0x004546a0)
void zoverRigClearPitch(ZRig *rig)
{
    zoverRigSelect(rig);
    gZoverPitchOn[gZoverRigIndex] = 0;
}

/* Applies the stored yaw (a rotation about y) of the current rig. */
BMX_ADDR(0x004546d0)
void zoverRigApplyYaw(ZRig *rig, ZActor *rider)
{
    ZAngle3 angles = {0, gZoverYaw[gZoverRigIndex], 0};
    ZMatrix m;

    zmatrixIdentity(&m);
    zmatrixRotateEuler(&m, &m, &angles);
    zoverRigApplyRotation(rig, rider, &m);
}

BMX_ADDR(0x00454750)
void zoverRigRequestYaw(ZRig *rig, ZActor *rider, int16_t angle)
{
    zoverRigSelect(rig);
    gZoverYawOn[gZoverRigIndex] = 1;
    gZoverYaw[gZoverRigIndex] = angle;
}

BMX_ADDR(0x00454790)
void zoverRigClearYaw(ZRig *rig)
{
    zoverRigSelect(rig);
    gZoverYawOn[gZoverRigIndex] = 0;
}

/* ---- frames (helpers of the rider rig step) --------------------------------------------------- */

/* out = (0 - a) x (b - 0): the cross product of a and b, negated; b goes through a no-op scale first. */
BMX_ADDR(0x00455b60)
void zoverCrossNeg(const ZVec3 *a, const ZVec3 *b, ZVec3 *out)
{
    ZVec3 v = *b;
    ZVec3 zero;

    zero.x = 0.0f;
    zero.y = 0.0f;
    zero.z = 0.0f;
    zvec3Scale(&v, &v, 1.0f);
    zvec3Cross3(out, a, &zero, &v);
}

/*
 * Frame whose x axis follows point i - point j and whose y axis, orthogonalised, follows point k - point l;
 * the position row is zero.
 */
BMX_ADDR(0x00455080)
void zoverFrameFromPoints(const ZRig *rig, int i, int j, int k, int l, ZMatrix *out)
{
    zmatrixZeroPos(out);
    zoverVecSub(&rig->points[i].pos, &rig->points[j].pos, &out->right);
    zoverVecSub(&rig->points[k].pos, &rig->points[l].pos, &out->up);
    zvec3NormalizeScaled(&out->right, &out->right);
    zoverCrossNeg(&out->right, &out->up, &out->at);
    zvec3NormalizeScaled(&out->at, &out->at);
    zoverCrossNeg(&out->at, &out->right, &out->up);
}

/*
 * Frame with the up axis along point ia - point ib; its other axes are the reference frame's right and
 * at axes mixed by how much each reference axis lines up with it (the weights are the absolute dot
 * products of the reference axes with the new up axis). `mode` picks which sign of the first dot product
 * flips the reference's at axis.
 */
static void frameFollow(const ZRig *rig, int ia, int ib, const ZMatrix *ref, ZMatrix *out, int mode)
{
    ZVec3 v, e1, e2;
    float d0, d1, d2, b, c, t, sa, sb, sc;
    double a;

    zmatrixIdentity(out);
    zoverVecSub(&rig->points[ia].pos, &rig->points[ib].pos, &out->up);
    zvec3NormalizeScaled(&out->up, &out->up);
    d0 = (float)zvec3Dot(&ref->right, &out->up);
    d1 = (float)zvec3Dot(&ref->up, &out->up);
    d2 = (float)zvec3Dot(&ref->at, &out->up);
    {
        const int negative = mode == 0 ? !(d0 >= ZERO) : !(d0 > ZERO);
        const int flipAt = mode == 0 ? negative : !negative;

        a = negative ? -(double)d0 : (double)d0;
        if (flipAt) {
            v.x = -ref->at.x;
            v.y = -ref->at.y;
            v.z = -ref->at.z;
        } else {
            v = ref->at;
        }
    }
    b = d1 > ZERO ? d1 : -d1;
    c = d2 > ZERO ? d2 : -d2;
    e1 = ref->right;
    e2 = ref->right;
    t = (float)(((double)c + b) + a);
    sa = (float)(a / (double)t);
    zvec3Scale(&v, &v, sa);
    sb = (float)((double)b / (double)t);
    zvec3Scale(&e1, &e1, sb);
    sc = (float)((double)c / (double)t);
    zvec3Scale(&e2, &e2, sc);
    zvec3Add(&v, &v, &e1);
    zvec3Add(&v, &v, &e2);
    zvec3NormalizeScaled(&v, &v);
    out->right = v;
    zoverCrossNeg(&out->right, &out->up, &out->at);
    zvec3NormalizeScaled(&out->at, &out->at);
    zoverCrossNeg(&out->up, &out->at, &out->right);
    zvec3NormalizeScaled(&out->right, &out->right);
}

BMX_ADDR(0x00455140)
void zoverFrameFollow(const ZRig *rig, int ia, int ib, const ZMatrix *ref, ZMatrix *out)
{
    frameFollow(rig, ia, ib, ref, out, 0);
}

/* As zoverFrameFollow with the opposite sign convention for the reference's at axis. */
BMX_ADDR(0x00455590)
void zoverFrameFollowFlipped(const ZRig *rig, int ia, int ib, const ZMatrix *ref, ZMatrix *out)
{
    frameFollow(rig, ia, ib, ref, out, 1);
}

/*
 * Frame that tilts the reference by the angle between its up axis and point ia - point ib: the angle is
 * the arccosine of their (clamped) dot product, taken about x.
 */
BMX_ADDR(0x00455430)
void zoverFrameSnap(const ZRig *rig, int ia, int ib, const ZMatrix *ref, ZMatrix *out)
{
    ZMatrix tmp;
    ZVec3 d;
    double dot;
    float c;
    ZAngle angle;

    zmatrixIdentity(out);
    zmatrixIdentity(&tmp);
    zoverVecSub(&rig->points[ia].pos, &rig->points[ib].pos, &d);
    zvec3NormalizeScaled(&d, &d);
    dot = zvec3Dot(&d, &ref->up);
    c = (float)dot;
    if (!(dot >= -1.0f)) {
        c = -1.0f;
    } else if (c > ONE) {
        c = ONE;
    }
    angle = zmathAcos(c);
    if (c < ZERO) {
        angle = (ZAngle)(-0x8000 - angle);
    }
    zmatrixRotateX(out, out, angle);
    zmatrixCopyRotation(&tmp, ref);
    zmatrixMul(&tmp, &tmp, out);
    zmatrixCopyRotation(out, &tmp);
}

/*
 * Frame from an up axis d (normalised into out) and a reference: the out frame's at axis blends the
 * reference's at axis and the side axis in proportion to how much the reference's right axis lines up
 * with d. d is overwritten with a scratch vector; the rig parameter is unused.
 */
BMX_ADDR(0x004559c0)
void zoverFrameFromUp(const ZRig *rig, ZVec3 *d, const ZMatrix *ref, ZMatrix *out)
{
    ZVec3 l;
    float d0, ad;

    zmatrixIdentity(out);
    out->up = *d;
    zvec3NormalizeScaled(&out->up, &out->up);
    d0 = (float)zvec3Dot(&ref->right, &out->up);
    (void)zvec3Dot(&ref->up, &out->up);
    (void)zvec3Dot(&ref->at, &out->up);
    l = ref->at;
    zoverCrossNeg(&ref->right, &out->up, d);
    zvec3NormalizeScaled(d, d);
    ad = __builtin_fabsf(d0);
    zvec3Scale(&l, &l, ad);
    zvec3Scale(d, d, (float)((double)ONE - ad));
    zvec3Add(&l, &l, d);
    out->at = l;
    zoverCrossNeg(&out->up, &out->at, &out->right);
    zvec3NormalizeScaled(&out->right, &out->right);
    zoverCrossNeg(&out->right, &out->up, &out->at);
    zvec3NormalizeScaled(&out->at, &out->at);
}

/*
 * Frame from point ia - point ib and a reference: the reference's right axis is bent by twice its
 * (sign folded) projection on the segment, so that the up axis mirrors about the reference.
 */
BMX_ADDR(0x00455880)
void zoverFrameReflect(const ZRig *rig, int ia, int ib, const ZMatrix *ref, ZMatrix *out, int flip)
{
    ZMatrix scratch;
    ZVec3 d, v;
    double dot;

    zmatrixIdentity(out);
    zmatrixIdentity(&scratch);
    zoverVecSub(&rig->points[ia].pos, &rig->points[ib].pos, &d);
    dot = zvec3Dot(&d, &ref->right);
    v = ref->right;
    if (flip == 0) {
        if (!(dot >= ZERO)) {
            dot = -dot;
        }
    } else if (dot > ZERO) {
        dot = -dot;
    }
    zvec3Scale(&v, &v, (float)(dot + dot));
    zvec3Add(&d, &d, &v);
    zoverFrameFromUp(rig, &d, ref, out);
}

/* out = b * a for the 3x3 parts, accumulated element by element in float. */
BMX_ADDR(0x00455c60)
void zoverMul3(ZMatrix *out, const ZMatrix *a, const ZMatrix *b)
{
    for (int o = 0; o < 3; o++) {
        for (int i = 0; i < 3; i++) {
            float *dst = (float *)out + (2 - i) * 4 + (2 - o);

            *dst = 0.0f;
            for (int k = 0; k < 3; k++) {
                const float fa = ((const float *)a)[(2 - k) * 4 + (2 - o)];
                const float fb = ((const float *)b)[(2 - i) * 4 + (2 - k)];
                *dst = (float)((double)fa * fb + *dst);
            }
        }
    }
}

/* Euler angles of a rotation matrix (y from the length of the first row; gimbal lock below 0.01). */
BMX_ADDR(0x00455cf0)
void zoverMatrixToAngles(const ZMatrix *m, ZAngle3 *out)
{
    const double r = __builtin_sqrt((double)m->right.x * m->right.x + (double)m->right.y * m->right.y);
    const float rf = (float)r;

    if (r > 0x1.47ae14p-7f) {
        out->x = zmathAtan2(m->up.z, m->at.z);
        out->y = zmathAtan2(-m->right.z, rf);
        out->z = zmathAtan2(m->right.y, m->right.x);
    } else {
        out->x = zmathAtan2(-m->at.y, m->up.y);
        out->y = zmathAtan2(-m->right.z, rf);
        out->z = 0;
    }
}

/*
 * The actor's rotation for a frame (up and at axes used): lets zactorOrient derive the angles, returns them
 * in out and resets the actor's rotation to zero.
 */
BMX_ADDR(0x00455dd0)
void zoverActorAnglesFromFrame(ZActor *actor, const ZMatrix *frame, ZAngle3 *out)
{
    ZAngle3 zero = {0, 0, 0};
    ZMatrix tmp;

    zactorOrient(actor, &zero, &frame->up, &frame->at, &tmp);
    out->x = actor->rot[0];
    out->y = actor->rot[1];
    out->z = actor->rot[2];
    actor->rot[0] = 0;
    actor->rot[1] = 0;
    actor->rot[2] = 0;
}

/* ---- limbs ------------------------------------------------------------------------------------ */

BMX_ADDR(0x00455e60)
void zoverSetLimbBlend(ZLimbs *limbs, float blend, float epsilon)
{
    limbs->blend = blend;
    limbs->epsilon = epsilon;
}

/* Snappy limb blending (0.5, 0.01). */
BMX_ADDR(0x00455e90)
void zoverLimbsSnap(ZLimbs *limbs)
{
    zoverSetLimbBlend(limbs, 0.5f, 0x1.47ae14p-7f);
}

/* Default limb blending (0.1, 0.01). */
BMX_ADDR(0x00455ec0)
void zoverResetLimbs(void *limbs)
{
    zoverSetLimbBlend(limbs, 0x1.99999ap-4f, 0x1.47ae14p-7f);
}

/* Empty in the shipped build. */
BMX_ADDR(0x00455ef0)
void zoverLimbsBegin(void *actor, void *limbs) {}

/* Reads the lengths of the rider's arm and leg bones and derives the reach limits. */
BMX_ADDR(0x00455f10)
void zoverLimbsInit(ZActor *rider, ZLimbs *limbs)
{
    double last;

    limbs->length[1] = (float)zactorGetPartLength(rider, 5);
    limbs->length[0] = (float)zactorGetPartLength(rider, 4);
    limbs->length[5] = (float)zactorGetPartLength(rider, 8);
    limbs->length[4] = (float)zactorGetPartLength(rider, 7);
    limbs->length[2] = (float)zactorGetPartLength(rider, 0xb);
    limbs->length[3] = (float)zactorGetPartLength(rider, 0xa);
    limbs->length[6] = (float)zactorGetPartLength(rider, 0xe);
    last = zactorGetPartLength(rider, 0xd);
    limbs->length[7] = (float)last;
    limbs->field110 = 0;
    limbs->reachSum[2] = (float)((double)limbs->length[3] + limbs->length[2]);
    limbs->reachSum[3] = (float)(last + limbs->length[6]);
    limbs->reachSum[0] = (float)(((double)limbs->length[0] + limbs->length[1]) - ONE);
    limbs->reachSum[1] = (float)(((double)limbs->length[4] + limbs->length[5]) - ONE);
}

/* out = a * w[0] + b * w[1] + c * w[2]. */
BMX_ADDR(0x00456030)
void zoverCombine3(const float *w, const ZVec3 *a, const ZVec3 *b, const ZVec3 *c, ZVec3 *out)
{
    ZVec3 va = *a;
    ZVec3 vb = *b;
    ZVec3 vc = *c;

    zvec3Scale(&va, &va, w[0]);
    zvec3Scale(&vb, &vb, w[1]);
    zvec3Scale(&vc, &vc, w[2]);
    *out = va;
    zvec3Add(out, out, &vb);
    zvec3Add(out, out, &vc);
}

/* Stores a limb target pair for each limb in mask (1, 2, 4, 8) and marks the limbs as targeted. */
BMX_ADDR(0x00457790)
void zoverSetLimbTargets(void *limbsp, const ZVec3 *a, const ZVec3 *b, uint32_t mask)
{
    ZLimbs *limbs = limbsp;

    limbs->mask |= mask;
    for (int i = 0; i < 4; i++) {
        if ((mask & (1u << i)) != 0) {
            limbs->target[i].a = *a;
            limbs->target[i].b = *b;
        }
    }
}

/* ---- overlay pools ---------------------------------------------------------------------------- */

#define LINK_MASK 0x0fffffffu

/* Points the link word of a packet at another packet (the tag nibble stays). */
static void setLink(uint32_t *word, uint32_t target)
{
    *word = ((target ^ *word) & LINK_MASK) ^ *word;
}

/* Chains the packets of a pool: every item but the last points at its successor. */
static void linkPool(ZOverItem *items, uint32_t count)
{
    for (uint32_t i = 0; i + 1 < count; i++) {
        setLink(&items[i].w[0], (uint32_t)(uintptr_t)&items[i + 1]);
    }
}

/* Clears the tag, the next-glyph link and the kind of every item. */
static void resetPool(ZOverItem *items, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++) {
        items[i].w[0] &= LINK_MASK;
        items[i].h[kZoverItemNext] = 0xffff;
        items[i].b[kZoverItemKind] = 0;
    }
}

/* Chains and clears all pools of both frames, and resets the frame, the font count and the busy flag. */
BMX_ADDR(0x004578f0)
int zoverInit(void)
{
    gZoverFontCount = 0;
    gZoverFrame = 0;
    gZoverBusy = 0;
    for (int f = 0; f < 2; f++) {
        linkPool(gZoverPool0[f], 16);
        resetPool(gZoverPool0[f], 16);
        linkPool(gZoverPool1[f], 128);
        resetPool(gZoverPool1[f], 128);
        linkPool(gZoverPool2[f], 28);
        resetPool(gZoverPool2[f], 28);
        linkPool(gZoverPool3[f], 400);
        resetPool(gZoverPool3[f], 400);
    }
    return 0;
}

/* Chains `last` (the terminator of a pool) in front of the ordering table's current chain, then the pool. */
static void chainPool(ZOverItem *pool, ZOverItem *last)
{
    setLink(&last->w[0], *gZoverOt);
    setLink(gZoverOt, (uint32_t)(uintptr_t)pool);
}

/*
 * Presents the frame: chains the pools of the current layer set and a draw mode packet into the ordering
 * table, swaps the frame, then brings the new frame's pools up to date: items freed meanwhile are cleared,
 * items changed are copied over from the other frame (and re-linked to their successor). Does nothing
 * while an overlay call is running.
 */
BMX_ADDR(0x00457b00)
int zoverFlush(void)
{
    ZOverTerminator *term;
    ZOverItem *base, *other;
    uint32_t count, kind;

    if (gZoverBusy != 0) {
        return 0;
    }
    if (gZoverLayer == 0) {
        chainPool(gZoverPool1[gZoverFrame], &gZoverPool1[gZoverFrame][127]);
        chainPool(gZoverPool0[gZoverFrame], &gZoverPool0[gZoverFrame][15]);
    } else if (gZoverLayer == 1) {
        chainPool(gZoverPool3[gZoverFrame], &gZoverPool3[gZoverFrame][399]);
        chainPool(gZoverPool2[gZoverFrame], &gZoverPool2[gZoverFrame][27]);
    }
    term = &gZoverTerminators[gZoverFrame];
    term->link = (term->link & LINK_MASK) | 0x10000000;
    term->command = 0xe1000200;
    setLink(&term->link, *gZoverOt);
    setLink(gZoverOt, (uint32_t)(uintptr_t)term);
    gZoverFrame ^= 1;
    if (gZoverFreeCount > 0x23b) {
        zdebugFail("free list overflow", zoverFile, 0x10c, 0);
    }
    while (gZoverFreeCount != 0) {
        const uint32_t h = (uint16_t)gZoverFreeList[gZoverFreeCount - 1];
        ZOverItem *it;

        gZoverFreeCount--;
        zoverGetPool(h >> 14 & 1, h >> 12 & 3, gZoverFrame, &base, &count, &kind);
        it = &base[h & 0xfff];
        it->h[kZoverItemNext] = 0xffff;
        it->b[kZoverItemKind] = 0;
        it->w[0] &= LINK_MASK;
    }
    if (gZoverDirtyCount > 0x23b) {
        zdebugFail("dirty list overflow", zoverFile, 0x117, 0);
    }
    while (gZoverDirtyCount != 0) {
        const uint32_t h = (uint16_t)gZoverDirtyList[gZoverDirtyCount - 1];
        const uint32_t index = h & 0xfff;

        gZoverDirtyCount--;
        zoverGetPool(h >> 14 & 1, h >> 12 & 3, gZoverFrame, &base, &count, &kind);
        zoverGetPool(h >> 14 & 1, h >> 12 & 3, gZoverFrame == 0, &other, &count, &kind);
        other[index].b[kZoverItemFlags] &= 0x7f;
        zmemCopy(&base[index], &other[index], sizeof(ZOverItem));
        if (index != count - 1) {
            setLink(&base[index].w[0], (uint32_t)(uintptr_t)&base[index + 1]);
        }
    }
    return 0;
}

BMX_ADDR(0x00457f50)
int zoverSetLayerFlag(uint32_t flag)
{
    gZoverLayer = (uint8_t)flag;
    return 0;
}

/*
 * Locates the pool of kind (hi, lo) for a frame: its first item, its item count and its kind number (0 to 3).
 * Other combinations leave the outputs alone. Only the low bytes of the parameters are read.
 */
BMX_ADDR(0x00457f70)
int zoverGetPool(uint32_t hi, uint32_t lo, uint32_t frame, ZOverItem **base, uint32_t *count, uint32_t *kind)
{
    frame &= 0xff;
    if ((hi & 0xff) == 0) {
        if ((lo & 0xff) == 0) {
            *base = gZoverPool0[frame];
            *count = 16;
            *kind = 0;
        } else if ((lo & 0xff) == 1) {
            *base = gZoverPool1[frame];
            *count = 128;
            *kind = 1;
        }
    } else if ((hi & 0xff) == 1) {
        if ((lo & 0xff) == 0) {
            *base = gZoverPool2[frame];
            *count = 28;
            *kind = 2;
        } else if ((lo & 0xff) == 1) {
            *base = gZoverPool3[frame];
            *count = 400;
            *kind = 3;
        }
    }
    return 0;
}

/* Allocates the first free item of pool (hi, lo) of the current frame; returns its handle, or 0xffff. */
BMX_ADDR(0x004580e0)
uint32_t zoverAllocItem(uint32_t hi, uint32_t lo)
{
    ZOverItem *base;
    uint32_t count, kind;

    zoverGetPool(hi, lo, gZoverFrame, &base, &count, &kind);
    for (uint32_t i = 0; i < count; i++) {
        if (base[i].b[kZoverItemKind] == 0) {
            return (i & 0xfff) | ((((hi & 1) << 2) | (lo & 3)) << 12);
        }
    }
    return 0xffff;
}

/* Removes the first entry equal to `handle` from a handle list (the entry after it is skipped, as in the
 * original). */
static void listRemove(int16_t *list, uint32_t *count, int16_t handle)
{
    uint32_t i = 0;
    int16_t *p = list;

    if (*count == 0) {
        return;
    }
    do {
        if (*p == handle) {
            (*count)--;
            if (i < *count) {
                uint32_t n = *count - i;
                int16_t *q = p;
                do {
                    *q = q[1];
                    q++;
                    n--;
                } while (n != 0);
            }
        }
        i++;
        p++;
    } while (i < *count);
}

/* Takes an item off the pending-free list (it was just (re)used). */
static void freeListRemove(int16_t handle)
{
    listRemove(gZoverFreeList, &gZoverFreeCount, handle);
}

/* Puts an item on the dirty list unless it is there already; `line` is the original assert line. */
static void markDirty(ZOverItem *it, int16_t handle, int line)
{
    if ((it->b[kZoverItemFlags] & 0x80) == 0) {
        gZoverDirtyList[gZoverDirtyCount] = handle;
        gZoverDirtyCount++;
        if (gZoverDirtyCount > 0x23b) {
            zdebugFail("dirty list overflow", zoverFile, line, 0);
        }
        it->b[kZoverItemFlags] |= 0x80;
    }
}

/* As markDirty, for items whose flags the caller has just reset (no test). */
static void pushDirty(ZOverItem *it, int16_t handle, int line)
{
    gZoverDirtyList[gZoverDirtyCount] = handle;
    gZoverDirtyCount++;
    if (gZoverDirtyCount > 0x23b) {
        zdebugFail("dirty list overflow", zoverFile, line, 0);
    }
    it->b[kZoverItemFlags] |= 0x80;
}

/*
 * Frees an overlay: the item goes on the free list (cleared by the next flush) and off the dirty list.
 * A text frees its whole glyph chain.
 */
BMX_ADDR(0x004581a0)
void zoverRemove(uint32_t handle)
{
    ZOverItem *base, *it;
    uint32_t count, kind;
    int16_t h = (int16_t)handle;

    if (h == -1) {
        return;
    }
    gZoverBusy++;
    zoverGetPool(handle >> 14 & 1, handle >> 12 & 3, gZoverFrame, &base, &count, &kind);
    if ((handle & 0xfff) < count) {
        it = &base[handle & 0xfff];
        if (it->b[kZoverItemKind] != 0) {
            if (it->b[kZoverItemKind] < 5) {
                it->b[kZoverItemKind] = 0;
                it->w[0] &= LINK_MASK;
                gZoverFreeList[gZoverFreeCount] = h;
                gZoverFreeCount++;
                if ((it->b[kZoverItemFlags] & 0x80) != 0) {
                    listRemove(gZoverDirtyList, &gZoverDirtyCount, h);
                    it->b[kZoverItemFlags] = 0;
                }
            } else if (it->b[kZoverItemKind] == 5) {
                uint16_t cur = (uint16_t)handle;
                uint16_t next;

                do {
                    it = &base[cur & 0xfff];
                    next = it->h[kZoverItemNext];
                    it->h[kZoverItemNext] = 0xffff;
                    it->b[kZoverItemKind] = 0;
                    it->w[0] &= LINK_MASK;
                    gZoverFreeList[gZoverFreeCount] = (int16_t)cur;
                    gZoverFreeCount++;
                    if ((it->b[kZoverItemFlags] & 0x80) != 0) {
                        listRemove(gZoverDirtyList, &gZoverDirtyCount, (int16_t)cur);
                        it->b[kZoverItemFlags] = 0;
                    }
                    cur = next;
                } while (next != 0xffff);
            }
        }
    }
    gZoverBusy--;
}

/* Sets the alpha mode byte of a textured packet: the command gains bit 1 and the mode byte moves. */
static void setTextured(ZOverItem *it)
{
    it->w[0] = (it->w[0] & LINK_MASK) | 0x90000000;
    it->b[7] = 0x2c;
    it->w[0xd] = (it->w[0xd] & 0xffffff80) | 0x7f80;
}

/*
 * Shows (show != 0) or hides (show == 0) an overlay: hiding clears the tag nibble of the link word (a packet
 * with length 0 is skipped); showing restores the length of its kind and the alpha mode. A text acts on its
 * whole glyph chain. Original quirk: showing a glyph stores the alpha mode at 0x2c, where the text creator
 * keeps it at 0x38.
 */
BMX_ADDR(0x00458400)
int zoverShow(uint32_t handle, int show)
{
    ZOverItem *base, *it;
    uint32_t count, kind;
    const int16_t h = (int16_t)handle;

    if (h == -1) {
        return 0;
    }
    gZoverBusy++;
    zoverGetPool(handle >> 14 & 1, handle >> 12 & 3, gZoverFrame, &base, &count, &kind);
    it = &base[handle & 0xfff];
    if (show == 0) {
        if (it->b[kZoverItemKind] == kZoverKindGlyph) {
            do {
                it = &base[handle & 0xfff];
                it->w[0] &= LINK_MASK;
                freeListRemove((int16_t)handle);
                markDirty(it, (int16_t)handle, 0x20d);
                handle = it->h[kZoverItemNext];
            } while (it->h[kZoverItemNext] != 0xffff);
        } else {
            it->w[0] &= LINK_MASK;
        }
    } else {
        switch (it->b[kZoverItemKind]) {
        case kZoverKindSprite:
            setTextured(it);
            if ((it->b[kZoverItemFlags] & 7) != 0) {
                it->b[7] = 0x2e;
                it->b[0x38] = it->b[kZoverItemFlags] & 6;
            }
            break;
        case kZoverKindRect:
            it->w[0] = (it->w[0] & LINK_MASK) | 0x50000000;
            it->b[7] = 0x28;
            it->w[7] = (it->w[7] & 0xffffff80) | 0x7f80;
            if ((it->b[kZoverItemFlags] & 7) != 0) {
                it->b[7] = 0x2a;
                it->b[0x20] = it->b[kZoverItemFlags] & 6;
            }
            break;
        case kZoverKindGradient:
            it->w[10] = (it->w[10] & 0xffffff80) | 0x7f80;
            it->w[0] = (it->w[0] & LINK_MASK) | 0x80000000;
            it->b[7] = 0x38;
            if ((it->b[kZoverItemFlags] & 7) != 0) {
                it->b[7] = 0x3a;
                it->b[0x2c] = it->b[kZoverItemFlags] & 6;
            }
            break;
        case kZoverKindGlyph:
            do {
                it = &base[handle & 0xfff];
                it->w[0xd] = (it->w[0xd] & 0xffffff80) | 0x7f80;
                it->w[0] = (it->w[0] & LINK_MASK) | 0x90000000;
                it->b[7] = 0x2c;
                if ((it->b[kZoverItemFlags] & 7) != 0) {
                    it->b[7] = 0x2e;
                    it->b[0x2c] = it->b[kZoverItemFlags] & 6;
                }
                freeListRemove((int16_t)handle);
                markDirty(it, (int16_t)handle, 0x1f9);
                handle = it->h[kZoverItemNext];
            } while (it->h[kZoverItemNext] != 0xffff);
            break;
        default:
            break;
        }
    }
    markDirty(it, h, 0x217);
    freeListRemove(h);
    gZoverBusy--;
    return 0;
}

/* Splits a colour word: red, green and blue in bytes 4 to 6 of the packet. */
static void setRgb(ZOverItem *it, uint32_t color)
{
    it->b[4] = (uint8_t)(color >> 16);
    it->b[5] = (uint8_t)(color >> 8);
    it->b[6] = (uint8_t)color;
}

/*
 * Adds a filled rectangle. The colour's bits 24 to 26 are the alpha mode (0 opaque), bit 28 selects the
 * variant of the draw mode word. Returns the handle, or 0xffff when the pool is full.
 */
BMX_ADDR(0x004588d0)
uint32_t zoverAddRect(int layer, int x, int y, int w, int h, uint32_t color)
{
    ZOverItem *base, *it;
    uint32_t count, kind, mode, c;
    uint32_t handle;
    int16_t sh, x0, y0, x1, y1;

    gZoverBusy++;
    handle = zoverAllocItem((uint32_t)layer, 0);
    sh = (int16_t)handle;
    if (sh != -1) {
        zoverGetPool(handle >> 14 & 1, handle >> 12 & 3, gZoverFrame, &base, &count, &kind);
        it = &base[handle & 0xfff];
        mode = it->w[7];
        it->w[0] = (it->w[0] & LINK_MASK) | 0x50000000;
        it->b[7] = 0x28;
        it->w[7] = (mode & 0xffffff80) | 0x7f80;
        c = color;
        if ((color & 0x10000000) != 0) {
            c = color & 0xefffffff;
            it->w[7] = (mode & 0xffffff80) | 0x7fc0;
        }
        setRgb(it, c);
        if ((c & 0x7000000) == 0) {
            it->b[7] = 0x28;
        } else {
            it->b[7] = 0x2a;
            it->b[0x20] = (uint8_t)(((c >> 24) & 7) - 1);
        }
        x0 = (int16_t)x;
        x1 = (int16_t)((int16_t)w + x0);
        y0 = (int16_t)y;
        y1 = (int16_t)((int16_t)h + y0);
        it->h[4] = (uint16_t)x0;
        it->h[8] = (uint16_t)x0;
        it->h[5] = (uint16_t)y0;
        it->h[6] = (uint16_t)x1;
        it->h[7] = (uint16_t)y0;
        it->h[9] = (uint16_t)y1;
        it->h[10] = (uint16_t)x1;
        it->h[11] = (uint16_t)y1;
        it->b[kZoverItemKind] = kZoverKindRect;
        it->b[kZoverItemFlags] = (uint8_t)((c >> 24) & 7);
        freeListRemove(sh);
        pushDirty(it, sh, 0x256);
    }
    gZoverBusy--;
    return handle & 0xffff;
}

/* Moves and resizes a rectangle; returns 1 for a handle outside its pool. */
BMX_ADDR(0x00458b00)
int zoverSetRect(uint32_t handle, int x, int y, int w, int h)
{
    ZOverItem *base, *it;
    uint32_t count, kind;
    const int16_t x0 = (int16_t)x, y0 = (int16_t)y;
    int16_t x1, y1;

    gZoverBusy++;
    zoverGetPool(handle >> 14 & 1, handle >> 12 & 3, gZoverFrame, &base, &count, &kind);
    if ((handle & 0xfff) < count) {
        it = &base[handle & 0xfff];
        x1 = (int16_t)((int16_t)w + x0);
        y1 = (int16_t)((int16_t)h + y0);
        it->h[4] = (uint16_t)x0;
        it->h[8] = (uint16_t)x0;
        it->h[5] = (uint16_t)y0;
        it->h[6] = (uint16_t)x1;
        it->h[7] = (uint16_t)y0;
        it->h[9] = (uint16_t)y1;
        it->h[10] = (uint16_t)x1;
        it->h[11] = (uint16_t)y1;
        freeListRemove((int16_t)handle);
        markDirty(it, (int16_t)handle, 0x27c);
        gZoverBusy--;
        return 0;
    }
    gZoverBusy--;
    return 1;
}

/* Sets the colour (and alpha mode) of a rectangle; returns 1 for a handle outside its pool. */
BMX_ADDR(0x00458cc0)
int zoverSetRectColor(uint32_t handle, uint32_t color)
{
    ZOverItem *base, *it;
    uint32_t count, kind;

    gZoverBusy++;
    zoverGetPool(handle >> 14 & 1, handle >> 12 & 3, gZoverFrame, &base, &count, &kind);
    if ((handle & 0xfff) < count) {
        const uint8_t alpha = (uint8_t)(color >> 24);

        it = &base[handle & 0xfff];
        setRgb(it, color);
        if ((color & 0x7000000) == 0) {
            it->b[7] &= 0xfd;
        } else {
            it->b[7] |= 2;
            it->b[0x20] = (uint8_t)((alpha & 7) - 1);
        }
        it->b[kZoverItemFlags] ^= (alpha ^ it->b[kZoverItemFlags]) & 7;
        freeListRemove((int16_t)handle);
        markDirty(it, (int16_t)handle, 0x2a6);
        gZoverBusy--;
        return 0;
    }
    gZoverBusy--;
    return 1;
}

/* Fills a rectangle for this frame only (a screen bar with the red and blue bytes swapped). */
BMX_ADDR(0x00458ea0)
void zoverDrawRect(int x, int y, int w, int h, uint32_t color)
{
    gZoverRectSize.w = (uint16_t)w;
    gZoverRectSize.h = (uint16_t)h;
    ngifFillScreenBar(((color >> 16) & 0xff) | ((color & 0xff) << 16) | (color & 0xff00ff00u),
                      (uint32_t)(int)(int16_t)x, (int)(int16_t)y, 0x200, &gZoverRectSize);
}

/*
 * Adds a rectangle that fades from colour `top` to colour `bottom`. The alpha mode comes from bits 24 to
 * 26 of `top`. Returns the handle, or 0xffff when the pool is full.
 */
BMX_ADDR(0x00458f20)
uint32_t zoverAddGradient(int layer, int x, int y, int w, int h, uint32_t top, uint32_t bottom)
{
    ZOverItem *base, *it;
    uint32_t count, kind;
    uint32_t handle;
    int16_t sh;

    gZoverBusy++;
    handle = zoverAllocItem((uint32_t)layer, 0);
    sh = (int16_t)handle;
    if (sh != -1) {
        const uint8_t alpha = (uint8_t)(top >> 24);
        const int16_t x0 = (int16_t)x, y0 = (int16_t)y;
        const int16_t x1 = (int16_t)((int16_t)w + x0);
        const int16_t y1 = (int16_t)((int16_t)h + y0);

        zoverGetPool(handle >> 14 & 1, handle >> 12 & 3, gZoverFrame, &base, &count, &kind);
        it = &base[handle & 0xfff];
        it->w[10] = (it->w[10] & 0xffffff80) | 0x7f80;
        it->w[0] = (it->w[0] & LINK_MASK) | 0x80000000;
        it->b[7] = 0x38;
        if ((top & 0x7000000) == 0) {
            it->b[7] = 0x38;
        } else {
            it->b[7] = 0x3a;
            it->b[0x2c] = (uint8_t)((alpha & 7) - 1);
        }
        it->h[5] = (uint16_t)y0;
        it->b[5] = (uint8_t)(top >> 8);
        it->b[0xd] = (uint8_t)(top >> 8);
        it->b[4] = (uint8_t)(top >> 16);
        it->b[0xc] = (uint8_t)(top >> 16);
        it->h[4] = (uint16_t)x0;
        it->h[12] = (uint16_t)x0;
        it->b[6] = (uint8_t)top;
        it->h[9] = (uint16_t)y0;
        it->b[0xe] = (uint8_t)top;
        it->h[8] = (uint16_t)x1;
        it->h[13] = (uint16_t)y1;
        it->b[0x14] = (uint8_t)(bottom >> 16);
        it->b[0x15] = (uint8_t)(bottom >> 8);
        it->b[0x16] = (uint8_t)bottom;
        it->h[16] = (uint16_t)x1;
        it->h[17] = (uint16_t)y1;
        it->b[0x1c] = (uint8_t)(bottom >> 16);
        it->b[0x1d] = (uint8_t)(bottom >> 8);
        it->b[0x1e] = (uint8_t)bottom;
        it->b[kZoverItemKind] = kZoverKindGradient;
        it->b[kZoverItemFlags] = alpha & 7;
        freeListRemove(sh);
        pushDirty(it, sh, 0x2ff);
    }
    gZoverBusy--;
    return (uint16_t)sh;
}

/* tpage word of a sprite for a colour word: bits 24 to 26 pick the alpha mode (1 to 3, 0: none). */
static uint16_t spriteTpage(const ZOverSprite *spr, uint32_t color)
{
    uint32_t mode;

    if ((color & 0x7000000) == 0) {
        return spr->tpage;
    }
    mode = ((color >> 24) & 7) - 1;
    if (mode > 2) {
        mode = 3;
    }
    return (uint16_t)(((mode & 3) << 5) | (spr->tpage & 0xff9f));
}

static uint16_t clamp255(uint32_t v)
{
    return (uint16_t)(v > 0xff ? 0xff : v);
}

/* Adds a sprite from the sprite table at (x, y); returns the handle, or 0xffff when the pool is full. */
BMX_ADDR(0x00459180)
uint32_t zoverAddSprite(int layer, uint32_t id, int x, int y, uint32_t color)
{
    ZOverItem *base, *it;
    uint32_t count, kind, mode;
    uint32_t handle;
    int16_t sh;

    gZoverBusy++;
    handle = zoverAllocItem((uint32_t)layer, 0);
    sh = (int16_t)handle;
    if (sh != -1) {
        const ZOverSprite *spr = &(&gZoverSprites)[id & 0xffff];
        const int16_t x0 = (int16_t)x, y0 = (int16_t)y;

        zoverGetPool(handle >> 14 & 1, handle >> 12 & 3, gZoverFrame, &base, &count, &kind);
        it = &base[handle & 0xfff];
        it->b[7] = 0x2c;
        it->w[0] = (it->w[0] & LINK_MASK) | 0x90000000;
        mode = (it->w[0xd] & 0xffffff80) | 0x7f80;
        it->w[0xd] = mode;
        if ((color & 0x7000000) == 0) {
            it->b[7] = 0x2c;
        } else {
            it->b[7] = 0x2e;
            it->b[0x38] = (uint8_t)(((color >> 24) & 7) - 1);
        }
        setRgb(it, color);
        it->h[8] = spr->clut;
        it->h[13] = spriteTpage(spr, color);
        it->w[0xc] = spr->mode;
        it->w[0xd] = mode;
        it->h[4] = (uint16_t)x0;
        it->h[5] = (uint16_t)y0;
        it->h[6] = spr->u0;
        it->h[7] = spr->v0;
        it->h[10] = (uint16_t)y0;
        it->h[9] = (uint16_t)(spr->width + x0);
        it->h[11] = clamp255((uint32_t)spr->u1 + 1);
        it->h[12] = spr->v0;
        it->h[14] = (uint16_t)x0;
        it->h[15] = (uint16_t)(spr->height + y);
        it->h[16] = spr->u0;
        it->h[17] = clamp255((uint32_t)spr->v1 + 1);
        it->h[19] = (uint16_t)(spr->width + x0);
        it->h[20] = (uint16_t)(spr->height + y0);
        it->h[21] = clamp255((uint32_t)spr->u1 + 1);
        it->h[22] = clamp255((uint32_t)spr->v1 + 1);
        it->b[kZoverItemKind] = kZoverKindSprite;
        it->b[kZoverItemFlags] = (uint8_t)((color >> 24) & 7);
        pushDirty(it, sh, 0x344);
        freeListRemove(sh);
    }
    gZoverBusy--;
    return handle & 0xffff;
}

/*
 * Adds a text: one packet per glyph, chained through the next field; returns the handle of the first one
 * (0xffff if empty or if the pool ran out, in which case the glyphs added so far are removed again). The
 * colour's bits 24 to 26 are the alpha mode.
 */
BMX_ADDR(0x004594c0)
uint32_t zoverAddText(int layer, const void *fontp, const char *textp, int x, int y, uint32_t color,
                      int clutOffset)
{
    const ZOverFont *font = fontp;
    const uint8_t *text = (const uint8_t *)textp;
    const ZOverSprite *spr;
    ZOverItem *base, *it;
    uint32_t count, kind;
    uint32_t first = 0xffff, prev = 0xffff;
    const uint32_t alphaBits = color & 0x7000000;
    uint16_t tpage;

    gZoverBusy++;
    if (font == NULL) {
        zdebugFail("Null font in overlay", zoverFile, 0x363, 0);
        return 0xffff;
    }
    spr = &(&gZoverSprites)[*font->image];
    tpage = spriteTpage(spr, color);
    for (; *text != 0; text++) {
        const int firstChar = (int)font->firstChar;
        const uint32_t c = *text;

        if (firstChar <= (int)c && (int)c < (int)(font->charCount + firstChar)) {
            const ZOverGlyph *g = &font->glyphs[c - firstChar];
            const uint16_t width = g->width;
            uint32_t h = zoverAllocItem((uint32_t)layer, 1);
            int16_t sh = (int16_t)h;
            int16_t u, v, bottom, x0, x1, y0, y1;

            if (sh == -1) {
                if ((int16_t)first != -1) {
                    zoverRemove(first);
                }
                gZoverBusy--;
                return 0xffff;
            }
            if ((int16_t)first == -1) {
                first = h;
            }
            zoverGetPool(h >> 14 & 1, h >> 12 & 3, gZoverFrame, &base, &count, &kind);
            it = &base[h & 0xfff];
            it->w[0] = (it->w[0] & LINK_MASK) | 0x90000000;
            it->b[7] = 0x2c;
            it->w[0xd] = (it->w[0xd] & 0xffffff80) | 0x7f80;
            it->b[kZoverItemFlags] = 0;
            if (alphaBits != 0) {
                it->b[7] = 0x2e;
                it->b[kZoverItemFlags] = (uint8_t)((color >> 24) & 7);
                it->b[0x38] = (uint8_t)(((color >> 24) & 7) - 1);
            }
            setRgb(it, color);
            it->h[13] = tpage;
            it->h[8] = (uint16_t)(spr->clut + clutOffset);
            it->w[0xc] = spr->mode;
            u = (int16_t)(spr->u0 + g->x);
            v = (int16_t)(spr->v0 + g->y);
            x0 = (int16_t)x;
            y0 = (int16_t)y;
            x1 = (int16_t)(x + width);
            bottom = (int16_t)(font->height + v);
            y1 = (int16_t)(font->height + y);
            it->h[4] = (uint16_t)x0;
            it->h[5] = (uint16_t)y0;
            it->h[7] = (uint16_t)v;
            it->h[12] = (uint16_t)v;
            it->h[14] = (uint16_t)x0;
            it->h[6] = (uint16_t)u;
            it->h[9] = (uint16_t)x1;
            it->h[10] = (uint16_t)y0;
            it->h[11] = (uint16_t)(u + width);
            it->h[16] = (uint16_t)u;
            it->h[15] = (uint16_t)y1;
            it->h[20] = (uint16_t)y1;
            it->h[17] = (uint16_t)bottom;
            it->h[19] = (uint16_t)x1;
            it->h[21] = (uint16_t)(u + width);
            it->h[22] = (uint16_t)bottom;
            it->b[kZoverItemKind] = kZoverKindGlyph;
            it->h[kZoverItemNext] = 0xffff;
            if ((int16_t)prev != -1) {
                base[prev & 0xfff].h[kZoverItemNext] = (uint16_t)sh;
            }
            freeListRemove(sh);
            gZoverDirtyList[gZoverDirtyCount] = sh;
            gZoverDirtyCount++;
            prev = h;
            if (gZoverDirtyCount > 0x23b) {
                zdebugFail("dirty list overflow", zoverFile, 0x3ac, 0);
            }
            it->b[kZoverItemFlags] |= 0x80;
            x = x + 1 + width;
        }
    }
    gZoverBusy--;
    return first & 0xffff;
}

/* Recolours a text (every glyph of the chain) and sets its clut offset and alpha mode; 1 for a bad handle. */
BMX_ADDR(0x00459940)
int zoverSetColor(uint32_t handle, const void *fontp, uint32_t color, int clutOffset)
{
    const ZOverFont *font = fontp;
    ZOverItem *base, *it;
    uint32_t count, kind;
    uint16_t cur = (uint16_t)handle;

    gZoverBusy++;
    if ((int16_t)handle != -1) {
        const ZOverSprite *spr = &(&gZoverSprites)[*font->image];
        const uint16_t tpage = spriteTpage(spr, color);

        zoverGetPool(handle >> 14 & 1, handle >> 12 & 3, gZoverFrame, &base, &count, &kind);
        if ((handle & 0xfff) < count) {
            uint32_t idx = handle & 0xfff;

            do {
                it = &base[idx];
                setRgb(it, color);
                if ((color & 0x7000000) == 0) {
                    it->b[7] &= 0xfd;
                } else {
                    it->b[7] |= 2;
                    it->b[0x38] = (uint8_t)((((color >> 24) & 7)) - 1);
                }
                it->h[13] = tpage;
                it->h[8] = (uint16_t)(spr->clut + clutOffset);
                it->w[0xc] = spr->mode;
                markDirty(it, (int16_t)cur, 0x3ef);
                freeListRemove((int16_t)cur);
                cur = it->h[kZoverItemNext];
                idx = cur & 0xfff;
            } while (it->h[kZoverItemNext] != 0xffff);
            gZoverBusy--;
            return 0;
        }
    }
    gZoverBusy--;
    return 1;
}

/* Moves a text so that its first glyph is at (x, y). Original quirk: every glyph ends up with zero height. */
BMX_ADDR(0x00459be0)
int zoverMoveText(uint32_t handle, int x, int y)
{
    ZOverItem *base, *it;
    uint32_t count, kind;

    gZoverBusy++;
    zoverGetPool(handle >> 14 & 1, handle >> 12 & 3, gZoverFrame, &base, &count, &kind);
    if ((handle & 0xfff) < count) {
        uint16_t cur = (uint16_t)handle;
        const int16_t x0 = (int16_t)x, y0 = (int16_t)y;

        while (cur != 0xffff) {
            int16_t old, x1;

            it = &base[handle & 0xfff];
            old = (int16_t)it->h[4];
            it->h[4] = (uint16_t)x0;
            x1 = (int16_t)(((int16_t)it->h[9] - old) + x0);
            it->h[5] = (uint16_t)y0;
            it->h[9] = (uint16_t)x1;
            it->h[10] = (uint16_t)y0;
            it->h[14] = (uint16_t)x0;
            it->h[15] = (uint16_t)y0;
            it->h[19] = (uint16_t)x1;
            it->h[20] = (uint16_t)y0;
            freeListRemove((int16_t)cur);
            markDirty(it, (int16_t)cur, 0x423);
            cur = it->h[kZoverItemNext];
            handle = cur;
        }
        gZoverBusy--;
        return 0;
    }
    gZoverBusy--;
    return 1;
}

/*
 * Draws a text for this frame only: each glyph is handed to the glyph renderer through the scratch record.
 * Original quirk: the x position is passed zero extended from 16 bits.
 */
BMX_ADDR(0x00459df0)
void zoverDrawText(const void *fontp, const char *textp, int x, int y, uint32_t color, int flags)
{
    const ZOverFont *font = fontp;
    const uint8_t *text = (const uint8_t *)textp;
    uint32_t cx = (uint32_t)x;

    if (font == NULL) {
        zdebugFail("Null font in overlay", zoverFile, 0x446, 0);
        return;
    }
    gZoverGlyph.image = *font->image;
    gZoverGlyph.height = font->height;
    gZoverGlyph.color = (uint16_t)(((color >> 12) & 0xff00) | ((color >> 8) & 0xf0) | ((color >> 4) & 0xf));
    gZoverGlyph.flags = (uint16_t)flags;
    for (; *text != 0; text++) {
        const int firstChar = (int)font->firstChar;
        const int c = *text;

        if (c >= firstChar && c < firstChar + font->charCount) {
            const ZOverGlyph *g = &font->glyphs[c - firstChar];

            gZoverGlyph.x = g->x;
            gZoverGlyph.y = g->y;
            gZoverGlyph.width = g->width;
            ngifDrawTextGlyph(cx & 0xffff, (int)(int16_t)y, &gZoverGlyph);
            cx += (uint16_t)(g->width + 1);
        }
    }
}

/* Width of a text in pixels, one pixel of spacing per glyph (characters the font lacks count nothing). */
BMX_ADDR(0x00459f30)
int16_t zoverTextWidth(const char *text, const void *fontp)
{
    const ZOverFont *font = fontp;
    int16_t width = 0;

    if (font == NULL) {
        zdebugFail("Null font in overlay", zoverFile, 0x469, 0);
        return 0;
    }
    for (; *text != '\0'; text++) {
        const char first = font->firstChar;

        if (first <= *text && (int)*text < (int)font->charCount + first) {
            width = (int16_t)(width + font->glyphs[*text - first].width + 1);
        }
    }
    return width;
}

/*
 * Parses a font resource (a count, the cell height at byte 4, then 13 byte glyph records from byte 12) into
 * a font record: the glyph table (8 bytes per character code in use) is allocated from the static region.
 * Returns 1 if that fails; asserts on a null record or more than eight fonts.
 */
BMX_ADDR(0x00459fc0)
int zoverParseFont(const void *datap, void *fontp)
{
    const uint8_t *data = datap;
    ZOverFont *font = fontp;
    const uint8_t *rec;
    uint32_t n;
    uint8_t lo = 0xff, hi = 0;
    ZOverGlyph *glyphs;

    if (font == NULL) {
        zdebugFail("Null font in overlay", zoverFile, 0x486, 0);
        return 1;
    }
    if (gZoverFontCount > 7) {
        zdebugFail("Overflow", zoverFile, 0x487, 0);
    }
    n = data[0];
    font->height = data[4];
    rec = data + 0xc;
    for (uint32_t i = 0; i < n; i++) {
        const uint8_t c = rec[i * 13];
        if (c <= lo) {
            lo = c;
        }
        if (hi <= c) {
            hi = c;
        }
    }
    font->firstChar = (int8_t)lo;
    font->charCount = (uint8_t)((hi - lo) + 1);
    glyphs = zmemStaticAlloc((uint32_t)font->charCount << 3);
    font->glyphs = glyphs;
    if (glyphs == NULL) {
        return 1;
    }
    zmemFill(glyphs, 0, (uint32_t)font->charCount << 3);
    for (; n != 0; n--) {
        ZOverGlyph *g = &glyphs[rec[0] - lo];

        g->x = (uint16_t)(rec[2] * 0x100 + rec[1]);
        g->y = (uint16_t)(rec[6] * 0x100 + rec[5]);
        g->width = (uint16_t)(rec[10] * 0x100 + rec[9]);
        rec += 13;
    }
    font->pad04 = 0;
    gZoverFonts[gZoverFontCount] = font;
    gZoverFontCount++;
    return 0;
}
