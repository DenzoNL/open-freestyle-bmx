/* Airborne rider update and actor pose sync (C:\zaxis\Bmx\src\com\action\physics.c, part). */
#include "bmx/com/action/physics_air.h"
#include "bmx/com/action/contrler.h"
#include "bmx/com/action/prop.h"

#include "bmx/com/zge/bmx/bmxsound.h"
#include "bmx/com/zge/zactor.h"
#include "zge/zdebug.h"
#include "zge/zgemods/zobjcamera.h"

/*
 * Floating point follows the original x87 code exactly: `double` is a value held in an x87
 * register (53-bit precision control), `float` is a value the original stored to memory.
 */

static const char physicsFile[] = "C:\\zaxis\\Bmx\\Src\\com\\action\\physics.c";

#define ANGLE_SCALE         65536.0f
#define SPIN_PER_UNIT       0x1.a1372ap-8f /* 1 / (50 pi): wheel travel to rolling angle */
#define SPIN_DECAY          0x1.fae148p-1f /* 0.99 */
#define IK_TILT             0x1.0a3d70p-4f /* 0.065 */
#define WHEEL_SPIN_TO_PITCH 0x1.a75554p+8f /* 423.33 */
#define GROUND_PROBE_Y      15535.0f

/* MSVC6 _ftol: truncation through a 64-bit integer, keeping the low 32 bits. */
static int32_t physFtol(double x)
{
    return (int32_t)(uint32_t)(uint64_t)(int64_t)x;
}

/* x87 "C3" test: equal or unordered (FCOMP + TEST AH,0x40). */
static int x87Equal(double a, double b)
{
    return !(a < b || a > b);
}

/* The rider's orientation rows (fwd/up/side/omega at 0x3e0) as one matrix. */
static ZMatrix *riderBasis(PhysRider *r)
{
    return (ZMatrix *)(void *)&r->fwd;
}

static unsigned faceWord(const void *face)
{
    return ((const PhysContactData *)face)->face.tag;
}

/* Game object descriptor of a world object (its prop record). */
static void *objectInfo(const void *object)
{
    return physObjectProp(object);
}

/* Aims IK limb(s) `limbs` at `target`; the pole runs from part `part` of the rider actor to its unit x
 * offset. */
static void aimLimb(PhysRider *r, const ZVec3 *target, int part, uint32_t limbs)
{
    ZVec3 off = {0.0f, 0.0f, 0.0f};
    ZVec3 pole;
    ZVec3 base;

    off.x = 1.0f;
    zactorGetPartPosition(r->actor, part, &off, &pole);
    off.x = 0.0f;
    zactorGetPartPosition(r->actor, part, &off, &base);
    pole.x = (float)((double)pole.x - base.x);
    pole.y = (float)((double)pole.y - base.y);
    pole.z = (float)((double)pole.z - base.z);
    zactorSetIkTarget(r->ik, target, &pole, limbs);
}

/* Target at part `part` of the bike actor, offset along x. */
static void bikePart(PhysRider *r, int part, float offsetX, ZVec3 *out)
{
    ZVec3 off;

    zvec3Zero(&off);
    off.x = offsetX;
    zactorGetPartPosition(r->bikeObj, part, &off, out);
}

/* Tilts bike joint 1 by `angle` about Y (plus a fixed -15 degrees about X). */
static void tiltBike(PhysRider *r, int angle)
{
    ZMatrix tilt;
    ZMatrix ref;
    ZAngle3 joint;

    zmatrixIdentity(&tilt);
    zmatrixRotateY(&tilt, &tilt, (ZAngle)angle);
    zmatrixRotateX(&tilt, &tilt, (ZAngle)-0xaaa);
    zmatrixIdentity(&ref);
    physRelativeAngles(&ref, &tilt, 1, &joint);
    zactorSetJointAngles(r->bikeObj, 1, &joint);
}

/*
 * Points the rider and bike actors along the rider basis, places the rider's limbs on the bike with IK
 * (one placement per bit of the pose's limb mask) and updates the rolling sound. The community labels
 * this function "GravityFunction".
 */
BMX_ADDR(0x0042ba50)
void bmxphysUpdatePose(PhysRider *r)
{
    ZMatrix m;
    ZMatrix yaw;
    ZVec3 tmp;
    int state;
    int vol;
    int pitch;

    zmatrixCopyRotation(&m, riderBasis(r));
    zmatrixIdentity(&yaw);
    zmatrixRotateZ(&yaw, &yaw, r->yawTrim);
    zmatrixMul(&m, &m, &yaw);
    zactorFaceMatrix(r->actor, &m);
    state = physGetState(r);
    if (state == 3 || state == 5 || gPhysForceStep != 0) {
        r->step = r->fieldBFC;
        zmatrixTransformPoint(&tmp, riderBasis(r), &r->step);
        r->step = tmp;
    } else {
        zvec3Zero(&r->step);
    }
    physPoseActorsGrind(r);

    if ((r->flags & 0x400) != 0) {
        uint32_t mask = gPhysIkLimbMasks[r->ikPose];
        if (mask != 0) {
            for (uint32_t i = 0; i < 31; i++) {
                const uint32_t bit = (1u << i) & mask;
                ZVec3 a;
                ZVec3 c;
                ZVec3 off;

                switch (bit) {
                case 0x1:
                case 0x2:
                case 0x4:
                case 0x8: {
                    const double w = r->ikBlend;
                    int part, aimPart;
                    uint32_t limb;
                    if (bit == 0x2) {
                        a.x = (float)(w * -60.0f);
                        a.y = (float)(w * 30.0f);
                        a.z = a.x;
                        part = 8, aimPart = 6, limb = 2;
                    } else if (bit == 0x4) {
                        a.x = (float)(w * -20.0f);
                        a.y = 0.0f;
                        a.z = (float)(w * 40.0f);
                        part = 0xb, aimPart = 9, limb = 4;
                    } else if (bit == 0x8) {
                        a.x = (float)(w * -20.0f);
                        a.y = 0.0f;
                        a.z = (float)(w * -40.0f);
                        part = 0xe, aimPart = 0xc, limb = 8;
                    } else {
                        a.x = (float)(w * -60.0f);
                        a.y = (float)(w * 30.0f);
                        a.z = (float)(w * 60.0f);
                        part = 5, aimPart = 3, limb = 1;
                    }
                    zmatrixTransformPoint(&tmp, &m, &a);
                    a = tmp;
                    zvec3Zero(&off);
                    zactorGetPartPosition(r->actor, part, &off, &c);
                    zvec3Add(&c, &c, &a);
                    aimLimb(r, &c, aimPart, limb);
                    mask |= 0x80000000u;
                    break;
                }
                case 0x40:
                case 0x80:
                case 0x100:
                    tiltBike(r, physFtol((double)(bit == 0x80 ? -0x5555 : 0x5555) * r->ikBlend));
                    if (bit != 0x100) {
                        bikePart(r, 7, 0.0f, &c);
                        aimLimb(r, &c, 3, 1);
                    }
                    bikePart(r, 8, 0.0f, &c);
                    aimLimb(r, &c, 6, 2);
                    mask |= 0x80000000u;
                    break;
                case 0x200:
                case 0x400:
                    if (r->ikBlend > 0.0f) {
                        const double tilt = bit == 0x200 ? IK_TILT : -IK_TILT;
                        tiltBike(r, physFtol((double)r->stateTimer[1] * tilt * ANGLE_SCALE));
                    }
                    break;
                case 0x800:
                case 0x1000: {
                    const int part = bit == 0x800 ? 5 : 8;
                    bikePart(r, 9, 0.0f, &c);
                    zvec3Zero(&off);
                    zactorGetPartPosition(r->actor, part, &off, &a);
                    c.x = (float)(((double)c.x - a.x) * r->ikBlend + a.x);
                    c.y = (float)(((double)c.y - a.y) * r->ikBlend + a.y);
                    c.z = (float)(((double)c.z - a.z) * r->ikBlend + a.z);
                    if (bit == 0x800) {
                        aimLimb(r, &c, 3, 1);
                    } else {
                        aimLimb(r, &c, 6, 2);
                    }
                    mask |= 0x80000000u;
                    break;
                }
                case 0x2000:
                    bikePart(r, 8, 0.0f, &c);
                    aimLimb(r, &c, 3, 1);
                    mask |= 0x80000000u;
                    break;
                case 0x4000:
                    bikePart(r, 7, 0.0f, &c);
                    aimLimb(r, &c, 6, 2);
                    mask |= 0x80000000u;
                    break;
                case 0x8000:
                    bikePart(r, 6, -10.0f, &c);
                    aimLimb(r, &c, 3, 1);
                    mask |= 0x80000000u;
                    break;
                case 0x10000:
                    bikePart(r, 6, 10.0f, &c);
                    aimLimb(r, &c, 6, 2);
                    mask |= 0x80000000u;
                    break;
                case 0x20000:
                    bikePart(r, 5, 15.0f, &c);
                    aimLimb(r, &c, 9, 4);
                    mask |= 0x80000000u;
                    break;
                case 0x40000:
                    bikePart(r, 5, -15.0f, &c);
                    aimLimb(r, &c, 0xc, 8);
                    mask |= 0x80000000u;
                    break;
                default:
                    break;
                }
            }
            if ((mask & 0x80000000u) != 0) {
                zactorApplyIk(r->actor, r->ik);
            }
        }
    }

    pitch = physFtol((double)r->wheel[1].spin * WHEEL_SPIN_TO_PITCH);
    if (pitch >= 0x7f) {
        pitch = 0x7f;
    } else if (pitch < 0) {
        pitch = 0;
    }
    vol = physFtol((double)r->speed * 3.0f);
    if (vol >= 0x7f) {
        vol = 0x7f;
    }
    bmxSoundPlayAt(4000, &r->center, (int)(((uint32_t)r->player << 28) | 0x103), vol, pitch);
}

static void debugLine(const ZVec3 *from, const ZVec3 *to)
{
    PhysDebugVertex a = {*from, 0, {0xc0, 0xa0, 0x20}, 0};
    PhysDebugVertex b = {*to, 0, {0xc0, 0xa0, 0x20}, 0};
    debugDrawLine(0xc, &a, &b);
}

/* Damped suspension speed: fieldC80 applies while compressing (v < 0), fieldC84 while extending. */
static float travelSpeed(const PhysRider *r, double v)
{
    const double damp = v >= 0.0 ? r->fieldC84 : r->fieldC80;
    return (float)((1.0f - damp) * v);
}

/*
 * Airborne step. Each wheel probes along the rider's up axis (suspension travel plus radius); a wheel that
 * reaches acceptable ground is snapped onto it and gets a new contact frame, otherwise it stays airborne.
 * With both wheels in the air the body probes eight box corners against the world and the ground below
 * the hips. Otherwise the rider lands: the basis is rebuilt from the wheel positions (bailing if that
 * tilts too far) and the state returns to rolling.
 */
BMX_ADDR(0x00428df0)
void bmxphysUpdatePositionAir(PhysRider *r)
{
    ZMatrix basis;
    ZMatrix yaw;
    ZMatrix m;
    ZVec3 wpos[2];
    ZVec3 tmp;
    ZVec3 to;
    ZVec3 n;
    PhysRayHit hit;
    uint32_t nAir = 0;
    double dot;
    float speed = 0.0f;

    zmatrixCopyRotation(&basis, riderBasis(r));
    zmatrixIdentity(&yaw);
    zmatrixRotateZ(&yaw, &yaw, r->yawTrim);
    zmatrixMul(&basis, &basis, &yaw);

    for (uint32_t i = 0; i < 2; i++) {
        PhysWheel *w = &r->wheel[i];
        ZVec3 *p = &wpos[i];
        double sum;
        double reach;
        double ax;
        float t;
        float tv;
        unsigned material;
        unsigned snd;

        w->vel = r->vel;
        *p = w->anchor;
        zmatrixTransformPoint(&tmp, &basis, p);
        *p = tmp;
        zvec3Add(p, p, &r->center);
        sum = (double)w->travel + w->travelVel;
        t = (float)sum;
        if (!(sum >= r->travelMin)) {
            tv = (float)((double)r->travelMin - w->travel);
            w->travelVel = 0.0f;
            t = r->travelMin;
        } else if (t > r->travelMax) {
            tv = (float)((double)r->travelMax - w->travel);
            w->travelVel = 0.0f;
            t = r->travelMax;
        } else {
            tv = w->travelVel;
        }
        reach = (double)t + w->radius;
        ax = (double)p->x + w->vel.x;
        w->travel = t;
        to.x = (float)(ax - basis.up.x * reach);
        to.y = (float)(((double)p->y + w->vel.y) - basis.up.y * reach);
        to.z = (float)(((double)p->z + w->vel.z) - basis.up.z * reach);
        if (gPhysDebugDraw != 0) {
            debugLine(p, &to);
        }
        if (worldRaycast(p, &to, &hit) == 0) {
            goto airborne;
        }
        n = hit.normal;
        if (x87Equal(hit.normal.y, 0.0f)) {
            goto airborne;
        }
        if (zmathAcos((float)zvec3Dot(&basis.up, &n)) >= gPhysMaxSlope) {
            goto airborne;
        }

        /* Ground in reach. Closing speed along the normal, including the suspension motion. */
        w->normal = n;
        to.x = (float)((double)w->vel.x - basis.up.x * (double)w->travelVel);
        to.y = (float)((double)w->vel.y - basis.up.y * (double)w->travelVel);
        to.z = (float)((double)w->vel.z - basis.up.z * (double)w->travelVel);
        dot = -zvec3Dot(&to, &n);
        speed = (float)dot;
        if (!(dot > 0.0)) {
            continue;
        }
        if ((hit.flags & 1) != 0) {
            const unsigned kind = faceWord(hit.face) >> 12;
            if (kind == 9) {
                propHitFace(r->player, hit.face);
                goto airborne;
            }
            if (kind == 10) {
                /* Spring face: drop the velocity along the normal and kick out by the face's strength. */
                const double vd = zvec3Dot(&r->vel, &n);
                const float vx = (float)((double)r->vel.x - vd * n.x);
                const float vy = (float)((double)r->vel.y - vd * n.y);
                const double vz = (double)r->vel.z - vd * n.z;
                r->vel.x = vx;
                r->vel.y = vy;
                r->vel.z = (float)vz;
                r->vel.x = (float)((double)(int16_t)(faceWord(hit.face) & 0xfff) * n.x + vx);
                r->vel.y = (float)((double)(int16_t)(faceWord(hit.face) & 0xfff) * n.y + vy);
                r->vel.z = (float)((double)(int16_t)(faceWord(hit.face) & 0xfff) * n.z + vz);
                r->speed = (float)zvec3Length(&r->vel);
                bmxSoundPlayAt(0xbb9, &r->pos, (int)(((uint32_t)r->player << 28) | 0x109), 0x7f, 0);
                return;
            }
        } else if ((hit.flags & 2) != 0 && objectInfo(hit.objectData) != NULL) {
            if (propTouch(r->player, objectInfo(hit.objectData)) == 0) {
                goto airborne;
            }
        }

        /* Landing on this wheel. Only the front wheel takes over the contact surface here. */
        if ((i & 1) == 0) {
            physTakeContact(r, &hit);
        }
        material = 0;
        if ((hit.flags & 1) != 0) {
            if (hit.material != NULL) {
                material = *(const uint32_t *)hit.material & 0xf;
            }
        } else if ((hit.flags & 2) != 0) {
            const uint8_t *info = worldObjectInfo(hit.object);
            if (worldObjectState(hit.object) != NULL) {
                material = info[2];
            }
        }
        switch (material) {
        case 1:
        case 3:
            snd = 7;
            break;
        case 2:
            snd = 2;
            break;
        case 4:
            snd = 5;
            break;
        case 5:
        case 9:
            snd = 4;
            break;
        case 6:
            goto deadly;
        default:
            snd = 0;
            break;
        }
        if (speed > 10.0f) {
            int vol = physFtol((double)speed + speed);
            if (vol >= 0x7f) {
                vol = 0x7f;
            }
            bmxSoundPlayAt(0xbba, &hit.point, (int)(((uint32_t)r->player << 28) | snd | 0x100), vol, 0);
        }
        if (!((double)gPhysLandCrashSpeed * r->stat[4] > speed)) {
            physSetState(r, kPhysStateScripted);
            r->bailReason = 2;
            riderAddScore(r, 0, speed);
            goto signalObject;
        }
        if (speed > 20.0f) {
            r->flags |= 0x10000;
        } else {
            r->flags &= ~0x10000u;
        }
        {
            ZVec3 dv;
            double px;
            double dd;
            double v;
            float py;
            float pz;
            float k;

            dv.x = (float)((double)hit.point.x - p->x);
            dv.y = (float)((double)hit.point.y - p->y);
            dv.z = (float)((double)hit.point.z - p->z);
            t = (float)(-zvec3Dot(&dv, &basis.up) - w->radius);
            tv = w->travelVel;
            if (t > r->travelMax) {
                t = r->travelMax;
            } else if (!(t >= r->travelMin)) {
                t = r->travelMin;
            }
            k = (float)((double)r->fieldC6C * 0.5f);
            v = (zvec3Dot(&n, &basis.up) * speed * k + ((double)r->travelMax - t) * r->fieldC7C) / k + tv;
            w->travelVel = travelSpeed(r, v);
            px = (double)p->x + w->vel.x;
            py = (float)((double)p->y + w->vel.y);
            pz = (float)((double)p->z + w->vel.z);
            w->travel = t;
            reach = (double)t + w->radius;
            p->x = (float)(reach * basis.up.x + hit.point.x);
            p->y = (float)(reach * basis.up.y + hit.point.y);
            p->z = (float)(reach * basis.up.z + hit.point.z);
            dv.x = (float)(px - p->x);
            dv.y = (float)((double)py - p->y);
            dv.z = (float)((double)pz - p->z);
            dd = zvec3Dot(&dv, &n);
            w->normal = n;
            w->dir = basis.right;
            p->x = (float)(((double)dv.x - dd * n.x) + p->x);
            p->y = (float)(((double)dv.y - dd * n.y) + p->y);
            p->z = (float)(((double)dv.z - dd * n.z) + p->z);
        }
        zvec3Cross3(&w->side, &w->dir, &gPhysZero, &w->normal);
        zvec3NormalizeScaled(&w->side, &w->side);
        zvec3Cross3(&w->dir, &w->normal, &gPhysZero, &w->side);
        {
            const double slip = zvec3Dot(&w->side, &w->vel);
            if ((slip < 0 ? -slip : slip) >= gPhysWheelSideSlip) {
                physSetState(r, kPhysStateScripted);
                r->bailReason = 4;
                return;
            }
        }
        {
            double dv;
            double len;

            w->mode = 0;
            dv = zvec3Dot(&w->dir, &w->vel);
            w->vel.x = (float)(dv * w->dir.x);
            w->vel.y = (float)(dv * w->dir.y);
            w->vel.z = (float)(dv * w->dir.z);
            len = zvec3Length(&w->vel);
            w->speed = (float)len;
            w->spin = (int16_t)physFtol(len * SPIN_PER_UNIT * ANGLE_SCALE);
            if (!(zvec3Dot(&w->vel, &w->dir) >= 0.0)) {
                w->spin = -w->spin;
            }
        }
        if ((hit.flags & 1) != 0 && hit.face != NULL) {
            const unsigned kind = faceWord(hit.face) >> 12;
            void *contact;
            unsigned surface;
            if (kind == 0 || kind > 3) {
                continue;
            }
            contact = physFindNearestTaggedEdge(hit.id, (int)(faceWord(hit.face) & 0xfff), &r->pos);
            surface = faceWord(hit.face) >> 12;
            if (contact == NULL) {
                gZdebugPrint("bmxphysUpdatePositionAir: quarterpipe face with no assigned edge, ID=%d\n",
                             faceWord(hit.face) & 0xfff);
                contact = hit.face;
                surface = 0;
            }
            r->surface = (uint8_t)surface;
            r->contact = contact;
            physContactCopy(r);
        } else if ((hit.flags & 2) != 0 && hit.objectData != NULL) {
            r->surface = 0xc;
            r->contact = hit.objectData;
            physContactCopy(r);
        }
        continue;

    airborne:
        p->x = (float)((double)p->x + w->vel.x);
        p->y = (float)((double)p->y + w->vel.y);
        p->z = (float)((double)p->z + w->vel.z);
        {
            const double v = ((double)r->travelMax - t) * r->fieldC7C / r->fieldC70 + tv;
            w->mode = 2;
            w->travelVel = travelSpeed(r, v);
        }
        w->spin = (int16_t)physFtol((double)w->spin * SPIN_DECAY * ANGLE_SCALE);
        nAir++;
    }

    if (nAir >= 2) {
        goto bodyProbe;
    }

    /* Landing on one or both wheels. Landing on one wheel bails unless the trick allows it. */
    if (riderGetStateLevel(r, 0) == 1) {
        const int level1 = riderGetStateLevel(r, 1);
        int bail = 0;
        if (level1 == 1) {
            bail = (unsigned)riderGetStateLevel(r, 2) < 3 && (gRiderAnimFlags[r->anim] & 7) == 0;
        } else if (level1 == 2) {
            bail = (unsigned)riderGetStateLevel(r, 2) < 3;
        }
        if (bail) {
            physSetState(r, kPhysStateScripted);
            if (r->combo.count <= 1 && (riderGetStateLevel(r, 2) != 2 || (r->flags & 0x100) == 0)) {
                r->bailReason = 7;
            } else {
                r->bailReason = 8;
            }
            return;
        }
    }
    {
        ZVec3 q;
        ZVec3 e;
        ZVec3 f;
        ZVec3 quad[2];
        double rx;
        double upz;
        double cx;
        int slope;

        /* Basis from the wheel line and the (blended) ground normal. */
        m.up = r->up; /* the original reads uninitialised stack here if both wheels were airborne before */
        zvec3Zero(&m.pos);
        rx = (double)wpos[0].x - wpos[1].x;
        m.right.x = (float)rx;
        m.right.y = (float)((double)wpos[0].y - wpos[1].y);
        m.right.z = (float)((double)wpos[0].z - wpos[1].z);
        if (x87Equal(rx, 0.0f) && x87Equal(m.right.y, 0.0f) && x87Equal(m.right.z, 0.0f)) {
            zdebugFail("all components of X orientation are zero", physicsFile, 0xf82, 0);
        }
        zvec3NormalizeScaled(&m.right, &m.right);
        if (nAir == 0) {
            const ZVec3 *n0 = &r->wheel[0].normal;
            const ZVec3 *n1 = &r->wheel[1].normal;
            m.up.x = (float)(((double)n1->x - n0->x) * 0.5f + n0->x);
            m.up.y = (float)(((double)n1->y - n0->y) * 0.5f + n0->y);
            upz = ((double)n1->z - n0->z) * 0.5f + n0->z;
        } else {
            for (int k = 0; k < 2; k++) {
                const PhysWheel *w = &r->wheel[k];
                if (w->mode != 2) {
                    m.up = w->normal;
                    if (zmathAcos(-w->normal.y) < 0x238e) {
                        r->flags |= 0x400000;
                    } else {
                        r->flags &= ~0x400000u;
                    }
                }
            }
            upz = m.up.z;
        }
        m.up.x = (float)(((double)m.up.x - basis.up.x) * gPhysLandUpBlend + basis.up.x);
        m.up.y = (float)(((double)m.up.y - basis.up.y) * gPhysLandUpBlend + basis.up.y);
        m.up.z = (float)((upz - basis.up.z) * gPhysLandUpBlend + basis.up.z);
        zvec3Cross3(&m.at, &m.right, &gPhysZero, &m.up);
        if (x87Equal(m.at.x, 0.0f) && x87Equal(m.at.y, 0.0f) && x87Equal(m.at.z, 0.0f)) {
            zdebugFail("all components of Z orientation are zero", physicsFile, 0xf9a, 0);
        }
        zvec3NormalizeScaled(&m.at, &m.at);
        zvec3Cross3(&m.up, &m.at, &gPhysZero, &m.right);
        if (x87Equal(m.up.x, 0.0f) && x87Equal(m.up.y, 0.0f) && x87Equal(m.up.z, 0.0f)) {
            zdebugFail("all components of Y orientation are zero", physicsFile, 0xf9e, 0);
        }
        zvec3NormalizeScaled(&m.up, &m.up);
        slope = zmathAcos((float)zvec3Dot(&m.up, &basis.up));
        if (!(gPhysMaxSlope > (slope < 0 ? -slope : slope))) {
            goto tooSteep;
        }
        zmatrixCopyRotation(&r->basis, &m);
        zmatrixCopyRotation(riderBasis(r), &m);

        /* Body centre: midway between the positions the two wheel contacts imply. */
        q = r->wheel[0].anchor;
        zmatrixTransformPoint(&e, &m, &q);
        q = r->wheel[1].anchor;
        r->center.x = (float)((double)wpos[0].x - e.x);
        r->center.y = (float)((double)wpos[0].y - e.y);
        r->center.z = (float)((double)wpos[0].z - e.z);
        zmatrixTransformPoint(&e, &m, &q);
        q.x = (float)((double)wpos[1].x - e.x);
        q.y = (float)((double)wpos[1].y - e.y);
        q.z = (float)((double)wpos[1].z - e.z);
        cx = ((double)q.x - r->center.x) * 0.5f + r->center.x;
        r->center.x = (float)cx;
        r->center.y = (float)(((double)q.y - r->center.y) * 0.5f + r->center.y);
        r->center.z = (float)(((double)q.z - r->center.z) * 0.5f + r->center.z);
        r->pos.x = (float)cx;
        r->pos.y = r->center.y;
        r->pos.z = r->center.z;

        /* Same again one step ahead; the difference becomes the new velocity. */
        for (int k = 0; k < 2; k++) {
            quad[k].x = (float)((double)wpos[k].x + r->wheel[k].vel.x);
            quad[k].y = (float)((double)wpos[k].y + r->wheel[k].vel.y);
            quad[k].z = (float)((double)wpos[k].z + r->wheel[k].vel.z);
        }
        m.right.x = (float)((double)quad[0].x - quad[1].x);
        m.right.y = (float)((double)quad[0].y - quad[1].y);
        m.right.z = (float)((double)quad[0].z - quad[1].z);
        zvec3NormalizeScaled(&m.right, &m.right);
        zvec3Cross3(&m.at, &m.right, &gPhysZero, &m.up);
        zvec3NormalizeScaled(&m.at, &m.at);
        zvec3Cross3(&m.up, &m.at, &gPhysZero, &m.right);
        q = r->wheel[0].anchor;
        zmatrixTransformPoint(&tmp, &m, &q);
        e.x = (float)((double)quad[0].x - tmp.x);
        e.y = (float)((double)quad[0].y - tmp.y);
        e.z = (float)((double)quad[0].z - tmp.z);
        q = r->wheel[1].anchor;
        zmatrixTransformPoint(&tmp, &m, &q);
        f.x = (float)((double)quad[1].x - tmp.x);
        f.y = (float)((double)quad[1].y - tmp.y);
        f.z = (float)((double)quad[1].z - tmp.z);
        r->dvel = r->vel;
        e.x = (float)(((double)f.x - e.x) * 0.5f + e.x);
        e.y = (float)(((double)f.y - e.y) * 0.5f + e.y);
        e.z = (float)(((double)f.z - e.z) * 0.5f + e.z);
        zvec3Zero(&r->vel);
        for (int k = 0; k < 2; k++) {
            PhysWheel *w = &r->wheel[k];
            double qz;

            quad[k] = w->anchor;
            zmatrixTransformPoint(&tmp, &m, &quad[k]);
            quad[k] = tmp;
            quad[k].x = (float)((double)e.x + quad[k].x);
            quad[k].y = (float)((double)e.y + quad[k].y);
            qz = (double)e.z + quad[k].z;
            quad[k].z = (float)qz;
            w->vel.x = (float)((double)quad[k].x - wpos[k].x);
            w->vel.y = (float)((double)quad[k].y - wpos[k].y);
            w->vel.z = (float)(qz - wpos[k].z);
            if (w->mode != 2) {
                const double dv = zvec3Dot(&w->vel, &w->normal);
                w->vel.x = (float)((double)w->vel.x - dv * w->normal.x);
                w->vel.y = (float)((double)w->vel.y - dv * w->normal.y);
                w->vel.z = (float)((double)w->vel.z - dv * w->normal.z);
            }
            zvec3Add(&r->vel, &r->vel, &w->vel);
        }
        zvec3Scale(&r->vel, &r->vel, 0.5f);
        r->speed = (float)zvec3Length(&r->vel);
        r->dvel.x = (float)((double)r->vel.x - r->dvel.x);
        r->dvel.y = (float)((double)r->vel.y - r->dvel.y);
        r->dvel.z = (float)((double)r->vel.z - r->dvel.z);
        if (zvec3Dot(&r->vel, &r->basis.right) >= 0.0) {
            r->flags &= ~0x40000u;
        } else {
            r->flags |= 0x40000;
        }
        physSetState(r, kPhysStateRoll);
        return;
    }

bodyProbe: {
    ZVec3 d;
    ZVec3 h;
    ZVec3 c;
    ZVec3 probe;
    double kd;

    /* Both wheels airborne: push the body out of whatever the box corners hit. */
    d.x = (float)((double)r->center.x - ((double)r->step.x + r->pos.x));
    d.y = (float)((double)r->center.y - ((double)r->step.y + r->pos.y));
    d.z = (float)((double)r->center.z - ((double)r->pos.z + r->step.z));
    kd = zvec3Dot(&d, &basis.right);
    h.x = (float)((basis.right.x * kd + r->step.x) + r->pos.x);
    h.y = (float)((basis.right.y * kd + r->step.y) + r->pos.y);
    h.z = (float)((basis.right.z * kd + r->pos.z) + r->step.z);
    for (uint32_t k = 0; k < 8; k++) {
        c = gPhysAirProbe;
        if ((k & 1) != 0) {
            c.z = -c.z;
        }
        if ((k & 2) != 0) {
            c.x = -c.x;
        }
        if ((k & 4) != 0) {
            c.y = -c.y;
        }
        zmatrixTransformPoint(&tmp, &basis, &c);
        c = tmp;
        probe = tmp;
        if (!(zvec3Dot(&c, &r->vel) >= 0.0)) {
            continue;
        }
        zvec3Add(&c, &c, &h);
        zvec3Add(&c, &c, &r->vel);
        if (gPhysDebugDraw != 0) {
            debugLine(&h, &c);
        }
        if (worldRaycast(&h, &c, &hit) == 0) {
            continue;
        }
        n = hit.normal;
        dot = -zvec3Dot(&r->vel, &n);
        speed = (float)dot;
        if (!(dot > 0.0)) {
            continue;
        }
        if ((hit.flags & 1) != 0 && hit.face != NULL) {
            if ((faceWord(hit.face) & 0xf000) == 0x9000) {
                if (speed > 0.0f) {
                    propHitFace(r->player, hit.face);
                    continue;
                }
            } else if (hit.material != NULL && (*(const uint32_t *)hit.material & 0xf) == 6) {
                goto deadly;
            }
        } else if ((hit.flags & 2) != 0) {
            if (objectInfo(hit.objectData) != NULL && propTouch(r->player, objectInfo(hit.objectData)) == 0) {
                continue;
            }
        }
        if (!((double)gPhysAirCrashSpeed * r->stat[4] > speed)) {
            physSetState(r, kPhysStateScripted);
            r->bailReason = (uint8_t)((zmathAcos(-n.y) < 0x2aaa) + 1);
            riderAddScore(r, 0, speed);
            goto signalObject;
        }
        if (zmathAcos(-n.y) <= 0x2aaa) {
            goto tooSteep;
        }
        {
            double push;
            double px;
            double py;
            double pz;
            float pxf;
            float pyf;
            float g;

            d.x = (float)((double)hit.point.x - h.x);
            d.y = (float)((double)hit.point.y - h.y);
            d.z = (float)((double)hit.point.z - h.z);
            g = (float)(10.0f - zvec3Dot(&probe, &n));
            push = zvec3Dot(&d, &n) + g;
            px = n.x * push;
            pxf = (float)px;
            r->pos.x = (float)(px + r->pos.x);
            py = n.y * push;
            pyf = (float)py;
            r->pos.y = (float)(py + r->pos.y);
            pz = push * n.z;
            r->pos.z = (float)(pz + r->pos.z);
            r->center.x = (float)((double)pxf + r->center.x);
            r->center.y = (float)((double)pyf + r->center.y);
            r->center.z = (float)(pz + r->center.z);
        }
        if (speed > 0.0f) {
            const double s = (double)speed * 1.25f;
            double d3;
            float d3f;

            r->dvel.x = (float)(n.x * s);
            r->dvel.y = (float)(n.y * s);
            r->dvel.z = (float)(n.z * s);
            zvec3Add(&r->vel, &r->vel, &r->dvel);
            r->speed = (float)zvec3Length(&r->vel);
            d3 = zvec3Dot(&r->fwd, &n);
            d3f = (float)d3;
            if (zmathAcos((float)(d3 < 0 ? -d3 : d3)) >= 0x1555) {
                /* Glancing hit: turn the rider's forward axis away from the surface over a few frames. */
                const float kf = (float)((double)d3f * 1.25f);
                m.right.x = (float)((double)r->fwd.x - n.x * (double)kf);
                m.right.y = (float)((double)r->fwd.y - n.y * (double)kf);
                m.right.z = (float)((double)r->fwd.z - n.z * (double)kf);
                m.up = r->up;
                if (r->timer != 0 && (r->flags & 0x1000) == 0) {
                    m.up = gPhysAirUp;
                }
                zvec3Zero(&m.pos);
                zvec3Cross3(&m.at, &m.right, &gPhysZero, &m.up);
                zvec3NormalizeScaled(&m.at, &m.at);
                zvec3Cross3(&m.right, &m.up, &gPhysZero, &m.at);
                physRelativeAngles(riderBasis(r), &m, gPhysAirTurnSteps, &r->orientARot);
                zmatrixCopyRotation(&r->orientA, riderBasis(r));
                r->timer = (int16_t)gPhysAirTurnSteps;
                r->flags &= ~0x1000u;
            }
        }
        break;
    }

    /* Move, then find the ground below the hips. */
    zvec3Add(&r->pos, &r->pos, &r->vel);
    h.x = (float)((double)r->step.x + r->pos.x);
    h.y = (float)((double)r->step.y + r->pos.y);
    h.z = (float)((double)r->pos.z + r->step.z);
    to.x = h.x;
    to.y = GROUND_PROBE_Y;
    to.z = h.z;
    if (worldRaycast(&h, &to, &hit) != 0 && physTakeContact(r, &hit) != 0) {
        r->groundGap = (float)((double)hit.point.y - h.y);
        return;
    }
    gZdebugPrint("bmxphysUpdatePositionAir: no valid terrain found under rider's hips (pos X: %d, Z: %d)\n",
                 physFtol(to.x), physFtol(to.z));
    r->flags &= ~0x4000u;
    return;
}

tooSteep:
    physSetState(r, kPhysStateScripted);
    r->bailReason = 5;
    return;

deadly:
    physSetState(r, kPhysStateScripted);
    r->bailReason = 10;
    r->surface = 0xd;
    contrlerRumble(r->player, 1, 0xff);
    return;

signalObject:
    if ((hit.flags & 2) != 0) {
        worldObjectSignal(hit.objectData, 0);
    }
}
