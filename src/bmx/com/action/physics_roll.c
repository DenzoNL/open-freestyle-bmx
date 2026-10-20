/* Rolling position update (C:\zaxis\Bmx\src\com\action\physics.c). */
#include "bmx/com/action/physics_roll.h"

#include "bmx/com/zge/bmx/bmxsound.h"
#include "bmx/com/zge/zactor.h"
#include "zge/zdebug.h"
#include "zge/zgemods/zobjcamera.h"

/*
 * Floating point follows the original x87 code exactly: `double` is a value held in an x87
 * register (53-bit precision control), `float` is a value the original stored to memory.
 */

static const char physicsFile[] = "C:\\zaxis\\Bmx\\Src\\com\\action\\physics.c";

#define DEG_TO_ANGLE 0x1.6c16c2p+7f /* 0x10000 / 360 */

enum { kRollNext = 0, kRollStop = 1 };

/* Per-call scratch shared by the wheel passes. */
typedef struct RollState {
    ZVec3 pos[2];   /* wheel centres in world space */
    PhysRayHit hit; /* last line test */
    ZVec3 n;        /* normal of the last examined hit */
    float travel;   /* clamped suspension travel of the current wheel */
    uint32_t airborne;
    /*
     * The original's single local matrix, shared by rollSettle, rollAirSteer and rollFinish.
     * rollFinish's lean rotation multiplies it before setting its at row, so that row still holds
     * whatever the last wheel pass left there.
     */
    ZMatrix m;
} RollState;

/* x87 "C3 set" after FCOM: equal or unordered. */
static int x87Equal(double a, double b)
{
    return !(a < b) && !(a > b);
}

/*
 * _ftol: truncates st0 to a 64-bit integer (FISTP qword) and returns the low half, so values past
 * the int32 range wrap, and NaN or |x| >= 2^63 (integer indefinite) give 0.
 */
static int32_t physFtol(double x)
{
    if (!(x >= -0x1p63 && x < 0x1p63)) {
        return 0;
    }
    return (int32_t)(uint32_t)(uint64_t)(int64_t)x;
}

/* A wheel's dir/normal/side rows (+ unused position) laid out as a matrix, as the original treats them. */
static ZMatrix *wheelFrame(PhysWheel *wh)
{
    return (ZMatrix *)(void *)&wh->dir;
}

/* The rider's fwd/up/side/omega rows, which mirror the bike basis. */
static ZMatrix *riderFrame(PhysRider *r)
{
    return (ZMatrix *)(void *)&r->fwd;
}

static void rollDebugLine(const ZVec3 *from, const ZVec3 *to)
{
    PhysDebugVertex a = {0};
    PhysDebugVertex b = {0};

    if (!gPhysDebugDraw) {
        return;
    }
    a.pos = *from;
    a.rgb[0] = 0xc0;
    a.rgb[1] = 0xa0;
    a.rgb[2] = 0x20;
    b.pos = *to;
    b.rgb[0] = 0xc0;
    b.rgb[1] = 0xa0;
    b.rgb[2] = 0x20;
    ngifDrawLine3D(0xc, &a, &b);
}

/* Spark burst from a grind hazard: random offset (from `rnd`) in the ground plane around the hit. */
static void rollSparks(const PhysRayHit *hit, const ZVec3 *vel, float velScale, uint32_t rnd)
{
    ZVec3 off = {(float)(rnd & 0x1f), 0.0f, 0.0f};
    ZVec3 pos;
    ZVec3 v;

    zvec3RotateY(&off, &off, (ZAngle)zmathRand());
    pos.x = (float)((double)hit->point.x + off.x);
    pos.y = (float)((double)hit->point.y + off.y);
    pos.z = (float)((double)hit->point.z + off.z);
    v.x = (float)((double)vel->x * velScale);
    v.y = (float)((double)vel->y * velScale);
    v.z = (float)((double)vel->z * velScale);
    fxSparks(&pos, &v);
}

/* A side probe hit a wall the wheel moves into: impact sound, bail, or a bounce off the wall. */
static int rollWallHit(PhysRider *r, RollState *s, unsigned w, float pen)
{
    PhysWheel *wh = &r->wheel[w];
    PhysWheel *other = &r->wheel[w == 0];
    const PhysRayHit *hit = &s->hit;
    const ZVec3 *n = &s->n;
    uint32_t snd = 0;
    ZAngle ang;

    wh->touch |= 1;
    if (pen > 10.0f) {
        r->flags |= 0x800000;
        if ((((~r->fieldBA4) >> 3) & 1) != (uint32_t)(zvec3Dot(n, &r->side) >= 0.0)) {
            r->flags |= 0x80000;
        } else {
            r->flags &= ~0x80000u;
        }
    }
    if (hit->kind == 1) {
        if (hit->material) {
            switch (*hit->material & 0xf) {
            case 1:
            case 3:
                snd = 7;
                break;
            case 2:
                snd = 2;
                break;
            default:
                break;
            }
        }
    } else if (hit->kind == 2) {
        const uint8_t *info = ngifzBspGetInfo(hit->object);
        if (ngifzCollideObjectValid(hit->object)) {
            if (info[2] == 1) {
                snd = 7;
            } else if (info[2] == 2) {
                snd = 2;
            }
        }
    }
    if (pen > 10.0f) {
        int32_t vol = physFtol((double)pen + pen);
        if (vol >= 0x7f) {
            vol = 0x7f;
        }
        bmxSoundPlayAt(0xbbc, &r->pos, (int)((uint32_t)r->player << 28 | snd | 0x100), vol, 0);
    }
    if (!((double)gPhysBailImpact * r->field6E0 > pen)) {
        physSetState(r, 2);
        r->bailReason = 0;
        riderAddScore(r, 0, pen);
        if (hit->kind == 2) {
            ngifzBspObjectHit(hit->objectData, 0);
        }
        return kRollStop;
    }
    ang = zmathAcos((float)__builtin_fabs(zvec3Dot(&r->basis.right, n)));
    if (ang < 0xaaa) {
        /* Head-on: both wheels bounce back at a quarter of their speed; toggles the facing flag. */
        unsigned k;
        for (k = 0; k < 2; k++) {
            PhysWheel *x = &r->wheel[k];
            x->vel.x = (float)((double)x->vel.x * -0.25f);
            x->vel.y = (float)((double)x->vel.y * -0.25f);
            x->vel.z = (float)((double)x->vel.z * -0.25f);
            x->speed = (float)((double)x->speed * 0.25f);
        }
        if (r->flags & 0x40000) {
            r->flags &= ~0x40000u;
        } else {
            r->flags |= 0x40000;
        }
    } else {
        const double k = (double)pen * 1.25f;
        const float kf = (float)k;
        float scale;

        wh->vel.x = (float)(k * n->x + wh->vel.x);
        wh->vel.y = (float)((double)kf * n->y + wh->vel.y);
        wh->vel.z = (float)((double)kf * n->z + wh->vel.z);
        wh->dir = wh->vel;
        zvec3NormalizeScaled(&wh->dir, &wh->dir);
        zvec3Cross3(&wh->side, &wh->dir, &gZmathOrigin, &wh->normal);
        scale = (float)(1.0 - zmathCos(ang) * 0.67f);
        zvec3Scale(&wh->vel, &wh->vel, scale);
        wh->speed = (float)zvec3Length(&wh->vel);
        zvec3Scale(&other->vel, &other->vel, scale);
        /* Original quirk: the other wheel's speed lands in this wheel. */
        wh->speed = (float)zvec3Length(&other->vel);
    }
    return kRollNext;
}

/* Two probes sideways from the wheel (along its axle, offset fore and aft) for walls ahead of it. */
static int rollProbeWalls(PhysRider *r, RollState *s, unsigned w)
{
    PhysWheel *wh = &r->wheel[w];
    const PhysWheel *other = &r->wheel[w == 0];
    const ZVec3 *p = &s->pos[w];
    PhysRayHit *hit = &s->hit;
    unsigned j;

    for (j = 0; j < 2; j++) {
        const ZVec3 *axis = wh->mode == 2 ? &other->dir : &r->basis.right;
        double rad = wh->radius;
        double off = gPhysProbeOffset;
        double tz;
        double negDot;
        float tx, ty;
        float pen;
        int lands;
        ZVec3 to;

        if (w & 1) {
            rad = -rad;
        }
        tx = (float)(((double)axis->x * rad + p->x) + wh->vel.x);
        ty = (float)(((double)axis->y * rad + wh->vel.y) + p->y);
        tz = (rad * axis->z + p->z) + wh->vel.z;
        if (!(j & 1)) {
            off = -off;
        }
        to.x = (float)(off * r->basis.at.x + tx);
        to.y = (float)(off * r->basis.at.y + ty);
        to.z = (float)(off * r->basis.at.z + tz);
        if (!ngifzCollideLine(p, &to, hit)) {
            continue;
        }
        s->n = hit->normal;
        if (!x87Equal(s->n.y, 0.0) || (hit->face->info & 0xf000)) {
            /* Ignore ground: a face is a wall only if it is steep relative to the bike. */
            if (zmathAcos((float)__builtin_fabs(zvec3Dot(&r->basis.at, &s->n))) > 0x1555 &&
                zmathAcos((float)zvec3Dot(&r->basis.up, &s->n)) < gPhysWallAngle) {
                continue;
            }
        }
        negDot = -zvec3Dot(&wh->vel, &s->n);
        pen = (float)negDot;
        if (!(negDot > 0.0)) {
            continue;
        }
        lands = 1;
        if ((hit->face->info & 0xf000) == 0x9000 && pen > 0.0f) {
            propHitFace(r->player, hit->face);
            lands = 0;
        }
        if ((hit->kind & 2) && hit->objectData->propId != 0 &&
            !propTouch(r->player, hit->objectData->propId)) {
            continue;
        }
        if (!lands) {
            continue;
        }
        return rollWallHit(r, s, w, pen);
    }
    return kRollNext;
}

/* Ground test for the main line test: the face must be level enough relative to the bike. */
static int rollAcceptGround(const PhysRider *r, RollState *s)
{
    s->n = s->hit.normal;
    return zmathAcos((float)__builtin_fabs(zvec3Dot(&r->basis.at, &s->n))) > 0x1555 &&
           zmathAcos((float)zvec3Dot(&r->basis.up, &s->n)) < gPhysWallAngle;
}

/* Ground test for the follow-up line tests; `ref` is the wheel normal or the bike's up vector. */
static int rollAcceptSlope(const PhysRider *r, RollState *s, unsigned rear, const ZVec3 *ref)
{
    s->n = s->hit.normal;
    if (zmathAcos((float)__builtin_fabs(zvec3Dot(&r->basis.at, &s->n))) <= 0x1555) {
        return 0;
    }
    if (!(zvec3Dot(&r->vel, &s->n) >= 0.0)) {
        return 1;
    }
    if ((unsigned)(zvec3Dot(&r->vel, &r->basis.right) >= 0.0) != rear &&
        zmathAcos(-s->n.y) >= gPhysWallAngle) {
        return 0;
    }
    return zmathAcos((float)zvec3Dot(ref, &s->n)) < gPhysSlopeAngle;
}

/* Face type 10: launcher. Reflects the bike's velocity off the face and jumps. */
static int rollLaunch(PhysRider *r, RollState *s)
{
    PhysFace *face = s->hit.face;
    const ZVec3 *n = &s->n;
    const double d = zvec3Dot(&r->vel, n);
    const float vx = (float)((double)r->vel.x - d * n->x);
    float vy;
    double vz;
    double kick;

    r->vel.x = vx;
    vy = (float)((double)r->vel.y - d * n->y);
    r->vel.y = vy;
    vz = (double)r->vel.z - d * n->z;
    r->vel.z = (float)vz;
    kick = (double)(int16_t)(face->info & 0xfff);
    r->vel.x = (float)(kick * n->x + vx);
    r->vel.y = (float)(kick * n->y + vy);
    r->vel.z = (float)(kick * n->z + vz);
    r->speed = (float)zvec3Length(&r->vel);
    physSetState(r, 1);
    bmxSoundPlayAt(0xbb9, &r->pos, (int)((uint32_t)r->player << 28 | 0x109), 0x7f, 0);
    r->surface = 0;
    r->contact = face;
    physContactCopy(r);
    return kRollStop;
}

/* Front wheel (or rear, front airborne) on a non-vertical face: picks up the surface under it. */
static int rollSurface(PhysRider *r, RollState *s)
{
    const PhysRayHit *hit = &s->hit;

    if (!(__builtin_fabs((double)s->n.y) > 0.1f)) {
        return kRollNext;
    }
    if ((hit->kind & 1) && hit->face) {
        PhysFace *face = hit->face;
        void *contact = face;
        unsigned type = face->info >> 12;
        int res;

        switch (type) {
        case 1:
        case 2:
        case 3: {
            PhysQpEdge *edge = physFindQuarterpipeEdge(hit->id, face->info & 0xfff, &r->pos);
            ZVec3 edgeDir;

            type = face->info >> 12;
            if (!edge) {
                gZdebugPrint("bmxphysUpdatePositionRoll: quarterpipe face with no assigned edge, ID=%d\n",
                             face->info & 0xfff);
                type = 0;
                break;
            }
            contact = edge;
            edgeDir = edge->dir;
            if (r->surface == 0 && type != 0) {
                if (r->flags & 0x20000) {
                    r->surface = (uint8_t)type;
                    r->contact = edge;
                    physContactCopy(r);
                    res = physSetState(r, 4);
                    if (res == 4 || res == 6) {
                        return kRollStop;
                    }
                }
                if (zmathAcos((float)__builtin_fabs(zvec3Dot(&r->fwd, &edgeDir))) < 0x1555 &&
                    zmathAcos(-r->up.y) < 0xaaa && r->speed > 15.0f) {
                    bmxSoundPlayAt(0xbbb, &hit->point, (int)((uint32_t)r->player << 28 | 0x102), 0x7f, 0);
                }
            }
            break;
        }
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
            break;
        case 10:
            return rollLaunch(r, s);
        default:
            if (r->surface == 1 && (r->flags & 0x20000)) {
                res = physSetState(r, 4);
                if (res == 4 || res == 6) {
                    return kRollStop;
                }
            }
            r->flags &= ~0x4000u;
            type = 0;
            break;
        }
        r->surface = (uint8_t)type;
        r->contact = contact;
        physContactCopy(r);
    } else if ((hit->kind & 2) && hit->objectData) {
        r->surface = 12;
        r->contact = hit->objectData;
        physContactCopy(r);
    } else {
        gZdebugPrint("bmxphysUpdatePositionRoll: no valid terrain below front wheel (pos X: %d, Z: %d)\n",
                     physFtol(s->pos[0].x), physFtol(s->pos[0].z));
        r->flags &= ~0x4000u;
    }
    return kRollNext;
}

/* Rolling speed along the ground, then skid/spark effects or rolling friction. */
static void rollSettle(PhysRider *r, RollState *s, unsigned w)
{
    PhysWheel *wh = &r->wheel[w];
    const PhysRayHit *hit = &s->hit;
    const unsigned rear = w & 1;
    ZMatrix *m = &s->m;
    double sp;

    zmatrixCopyRotation(m, wheelFrame(wh));
    if (!(zvec3Dot(&wh->vel, &m->right) >= 0.0)) {
        zvec3Negate(&m->right, &m->right);
    }
    if (rear) {
        wh->speed = (float)((double)r->fieldC60 + wh->speed);
    }
    sp = ((double)m->right.y * gPhysGravity + wh->speed) * (1.0 - (double)r->fieldC64);
    wh->speed = (float)sp;
    wh->vel.x = (float)(m->right.x * sp);
    wh->vel.y = (float)((double)m->right.y * wh->speed);
    wh->vel.z = (float)((double)m->right.z * wh->speed);

    if (!rear || riderGetStateLevel(r, 0) != 0 || riderGetStateLevel(r, 1) != 2) {
        unsigned idx;
        wh->mode = 0;
        switch (wh->material) {
        case 2:
            idx = 2;
            break;
        case 5:
        case 6:
        case 10:
        case 11:
            idx = 4;
            break;
        default:
            idx = 0;
            break;
        }
        zvec3Scale(&wh->vel, &wh->vel, (float)(1.0 - gPhysRollFriction[idx]));
        return;
    }
    wh->spin = 0;
    if (wh->speed > 5.0f) {
        ZVec3 to;
        switch (wh->material) {
        case 0:
        case 1:
        case 2:
        case 4:
            to.x = (float)((double)hit->point.x + wh->vel.x);
            to.y = (float)((double)hit->point.y + wh->vel.y);
            to.z = (float)((double)hit->point.z + wh->vel.z);
            fxSkidMark(&hit->point, &to, 0, &hit->normal);
            break;
        case 5:
        case 9:
        case 10:
            rollSparks(hit, &wh->vel, 0.5f, zmathRand());
            break;
        default:
            break;
        }
        r->impactTotal = (float)((double)wh->speed + r->impactTotal);
    }
    wh->mode = 1;
}

/* Wheel on the ground at s->pos[w] with normal s->n: suspension, surface effects and rolling. */
static int rollContact(PhysRider *r, RollState *s, unsigned w, double dz)
{
    PhysWheel *wh = &r->wheel[w];
    ZVec3 *p = &s->pos[w];
    const PhysRayHit *hit = &s->hit;
    const unsigned rear = w & 1;
    double dv, x, k, v;
    float h, wb, damp;

    p->z = (float)(dz + p->z);

    /* Suspension spring: fore-aft acceleration loads or unloads the wheel (clamped to +-0.5). */
    dv = zvec3Dot(&r->dvel, &r->fwd);
    h = (float)(((double)r->fieldC68 + wh->travel) + wh->radius);
    wb = (float)((double)r->wheel[0].anchor.x - r->wheel[1].anchor.x);
    x = (rear ? dv : -dv) * h / wb;
    if (x > 0.5) {
        k = 0.5;
    } else if (!(x >= -0.5)) {
        k = -0.5;
    } else {
        k = x;
    }
    v = (((double)r->travelMax - s->travel) * r->fieldC7C +
         (k + 0.5) * r->fieldC6C * r->basis.up.y * gPhysGravity) /
            ((double)r->fieldC6C * 0.5) +
        wh->travelVel;
    damp = v >= 0.0 ? r->fieldC84 : r->fieldC80;
    wh->travel = s->travel;
    wh->normal = s->n;
    wh->travelVel = (float)((1.0 - damp) * v);
    wh->dir = r->basis.right;
    zvec3Cross3(&wh->side, &wh->dir, &gZmathOrigin, &wh->normal);
    zvec3NormalizeScaled(&wh->side, &wh->side);
    zvec3Cross3(&wh->dir, &wh->normal, &gZmathOrigin, &wh->side);
    if (!rear) {
        ZMatrix yaw;
        zmatrixIdentity(&yaw);
        zmatrixRotateY(&yaw, &yaw, r->rot.y);
        zmatrixMul(wheelFrame(wh), wheelFrame(wh), &yaw);
    }

    wh->material = 0;
    if (hit->kind & 1) {
        if (hit->material) {
            wh->material = (int32_t)(*hit->material & 0xf);
        }
    } else if (hit->kind & 2) {
        const uint8_t *info;
        if (hit->objectData->propId != 0) {
            propTouch(r->player, hit->objectData->propId);
        }
        info = ngifzBspGetInfo(hit->object);
        if (ngifzCollideObjectValid(hit->object)) {
            wh->material = info[2];
        }
    }
    switch (wh->material) {
    case 5:
    case 9:
    case 10:
        if (rear) {
            uint32_t i = 0;
            if ((uint32_t)physFtol((double)wh->speed * 0.15f) != 0) {
                do {
                    const uint32_t rnd = zmathRand();
                    if ((rnd & 0xf) == 0) {
                        rollSparks(hit, &wh->vel, 0.75f, rnd);
                    }
                    i++;
                } while (i < (uint32_t)physFtol((double)wh->speed * 0.15f));
            }
        }
        break;
    case 6:
        zactorClearFlag2(r->actor);
        zactorClearFlag2(r->bikeObj);
        switch ((*hit->material >> 19) & 3) {
        case 1:
            physSetState(r, 2);
            r->bailReason = 9;
            r->surface = 0xe;
            return kRollStop;
        case 2:
            break;
        default:
            physSetState(r, 2);
            r->bailReason = 9;
            r->surface = 0xd;
            return kRollStop;
        }
        break;
    case 11: {
        int32_t strength = physFtol((double)r->speed * 1.7f);
        if (strength >= 0x40) {
            strength = 0x40;
        }
        contrlerRumble(r->player, 1, strength);
        break;
    }
    default:
        break;
    }
    if ((!rear || r->wheel[0].mode == 2) && rollSurface(r, s) == kRollStop) {
        return kRollStop;
    }
    rollSettle(r, s, w);
    return kRollNext;
}

/* Airborne wheel while the other is down and steering locks are on: swing it back into line. */
static void rollAirSteer(PhysRider *r, RollState *s, unsigned w)
{
    PhysWheel *wh = &r->wheel[w];
    const PhysWheel *other = &r->wheel[w == 0];
    const unsigned rear = w & 1;
    ZMatrix *m = &s->m;
    ZMatrix l;
    ZVec3 t, u, tmp, out;
    ZAngle a0;
    int32_t c, roll, roty;
    int16_t diff;
    double d, q, sn, x, kg;
    float wb;

    zmatrixCopyRotation(m, (const ZMatrix *)(const void *)&other->dir);
    zvec3Cross3(&m->at, &r->basis.right, &gZmathOrigin, &m->up);
    zvec3NormalizeScaled(&m->at, &m->at);
    zvec3Cross3(&m->right, &m->up, &gZmathOrigin, &m->at);
    a0 = zmathAcos((float)zvec3Dot(&m->right, &r->basis.right));
    d = zvec3Dot(&wh->vel, &m->up);
    wh->vel.x = (float)((double)wh->vel.x - d * m->up.x);
    wh->vel.y = (float)((double)wh->vel.y - d * m->up.y);
    wh->vel.z = (float)((double)wh->vel.z - d * m->up.z);
    zvec3Cross3(&t, &m->right, &gZmathOrigin, &gPhysDown);
    zvec3NormalizeScaled(&t, &t);
    zvec3Cross3(&u, &t, &gZmathOrigin, &m->right);

    /* Target pitch from the slope, limited to 60 degrees; only the low 16 bits matter. */
    c = rear ? 0x1c71 : 0x11c7;
    if (zmathAcos(-u.y) + c > 0x2aaa) {
        c = 0x2aaa;
    } else {
        c += zmathAcos(-u.y);
    }
    if ((unsigned)!rear != (unsigned)!(m->right.y >= 0.0f)) {
        c = physFtol(-((double)(int16_t)c * m->up.y));
    }
    diff = (int16_t)(c - a0);
    q = (double)diff * 0x1.68p-8f * -0x1.6c16c2p-6f;
    if (1.0 >= q && -1.0 > q) {
        q = -1.0;
    } else if (!(1.0 >= q)) {
        q = 1.0;
    }
    r->landQuality = (float)q;

    wb = (float)((double)r->wheel[0].anchor.x - r->wheel[1].anchor.x);
    sn = zmathSin(diff) * wb * -0x1.333334p-4f;
    wh->vel.x = (float)(sn * m->up.x + wh->vel.x);
    wh->vel.y = (float)(sn * m->up.y + wh->vel.y);
    wh->vel.z = (float)(sn * m->up.z + wh->vel.z);
    roll = rear ? -r->rot.z : r->rot.z;
    sn = zmathSin((ZAngle)roll) * wb;
    wh->vel.x = (float)(sn * m->up.x + wh->vel.x);
    wh->vel.y = (float)(sn * m->up.y + wh->vel.y);
    wh->vel.z = (float)(sn * m->up.z + wh->vel.z);

    /* Yaw the velocity in bike space by the steering angle, limited to +-910 (5 degrees). */
    zmatrixCopyRotation(&l, &r->basis);
    tmp = wh->vel;
    zmatrixTranspose(&l, &l);
    zmatrixTransformPoint(&out, &l, &tmp);
    tmp = out;
    roty = rear ? r->rot.y : -r->rot.y;
    x = (double)roty * 0.15f;
    if (x >= 910.0f) {
        x = 910.0f;
    } else if (-910.0f > x) {
        x = -910.0f;
    }
    zvec3RotateY(&tmp, &tmp, (ZAngle)physFtol(x));
    zmatrixTranspose(&l, &l);
    zmatrixTransformPoint(&out, &l, &tmp);
    kg = (double)m->at.y * gPhysGravity;
    wh->vel.x = (float)(kg * m->at.x + out.x);
    wh->vel.y = (float)(kg * m->at.y + out.y);
    wh->vel.z = (float)(kg * m->at.z + out.z);
}

/* No ground under the wheel: gravity (or steering), suspension extends, spin decays. */
static void rollAir(PhysRider *r, RollState *s, unsigned w)
{
    PhysWheel *wh = &r->wheel[w];
    const ZVec3 *up = &r->basis.up;
    double v;
    float damp;

    if (!(r->flags & 0x300000)) {
        const double k = (double)gPhysGravity * up->y;
        wh->vel.x = (float)(k * up->x + wh->vel.x);
        wh->vel.y = (float)(k * up->y + wh->vel.y);
        wh->vel.z = (float)(k * up->z + wh->vel.z);
    } else if (r->wheel[w == 0].mode != 2) {
        rollAirSteer(r, s, w);
    }
    v = ((double)r->travelMax - s->travel) * r->fieldC7C / r->fieldC70 + wh->travelVel;
    damp = v >= 0.0 ? r->fieldC84 : r->fieldC80;
    wh->mode = 2;
    wh->travelVel = (float)((1.0 - damp) * v);
    wh->spin = (int16_t)physFtol((double)wh->spin * 0.99f * 65536.0f);
    s->airborne++;
}

/* One wheel: wall probes, suspension, ground line tests, then contact or airborne handling. */
static int rollWheel(PhysRider *r, RollState *s, unsigned w)
{
    PhysWheel *wh = &r->wheel[w];
    const PhysWheel *other = &r->wheel[w == 0];
    ZVec3 *p = &s->pos[w];
    PhysRayHit *hit = &s->hit;
    const ZVec3 *up = &r->basis.up;
    const unsigned rear = w & 1;
    ZVec3 tmp, to;
    double d, len, shift;

    *p = wh->anchor;
    wh->touch = 0;
    zmatrixTransformPoint(&tmp, &r->basis, p);
    *p = tmp;
    zvec3Add(p, p, &r->center);
    if ((unsigned)(zvec3Dot(&wh->vel, &r->basis.right) >= 0.0) != rear &&
        rollProbeWalls(r, s, w) == kRollStop) {
        return kRollStop;
    }

    /* Clamp the suspension travel; a clamped wheel moves the contact point instead. */
    {
        const double t = (double)wh->travel + wh->travelVel;
        s->travel = (float)t;
        if (!(t >= r->travelMin)) {
            d = (double)r->travelMin - wh->travel;
            wh->travelVel = 0.0f;
            s->travel = r->travelMin;
        } else if (s->travel > r->travelMax) {
            d = (double)r->travelMax - wh->travel;
            wh->travelVel = 0.0f;
            s->travel = r->travelMax;
        } else {
            d = wh->travelVel;
        }
    }
    wh->travel = s->travel;
    if (wh->mode <= 1) {
        p->x = (float)(d * up->x + p->x);
        p->y = (float)(d * up->y + p->y);
        p->z = (float)(d * up->z + p->z);
    }

    /* Main ground test: from the wheel centre down through the tyre, moved by this frame's velocity. */
    len = ((double)wh->travelVel + wh->radius) + s->travel;
    to.x = (float)(((double)p->x + wh->vel.x) - len * up->x);
    to.y = (float)(((double)wh->vel.y + p->y) - len * up->y);
    to.z = (float)(((double)p->z + wh->vel.z) - len * up->z);
    rollDebugLine(p, &to);
    if (ngifzCollideLine(p, &to, hit) && rollAcceptGround(r, s)) {
        double e, ax, base;

        to.x = (float)((double)hit->point.x - p->x);
        to.y = (float)((double)hit->point.y - p->y);
        to.z = (float)((double)hit->point.z - p->z);
        e = -zvec3Dot(&to, up) - wh->radius;
        s->travel = (float)e;
        if (e > r->travelMax) {
            s->travel = r->travelMax;
        } else if (!(s->travel >= r->travelMin)) {
            s->travel = r->travelMin;
        }
        ax = (double)p->x + wh->vel.x;
        to.y = (float)((double)wh->vel.y + p->y);
        to.z = (float)((double)p->z + wh->vel.z);
        base = (double)s->travel + wh->radius;
        p->x = (float)(base * up->x + hit->point.x);
        p->y = (float)(base * up->y + hit->point.y);
        p->z = (float)(base * up->z + hit->point.z);
        /* Slide the remaining motion along the face. */
        to.x = (float)(ax - p->x);
        to.y = (float)((double)to.y - p->y);
        to.z = (float)((double)to.z - p->z);
        d = zvec3Dot(&to, &s->n);
        p->x = (float)(((double)to.x - d * s->n.x) + p->x);
        p->y = (float)(((double)to.y - d * s->n.y) + p->y);
        return rollContact(r, s, w, (double)to.z - d * s->n.z);
    }

    p->x = (float)((double)p->x + wh->vel.x);
    p->y = (float)((double)wh->vel.y + p->y);
    p->z = (float)((double)p->z + wh->vel.z);

    /* Steering locks on and the other wheel airborne: test along this wheel's old contact normal. */
    if ((r->flags & 0x300000) && wh->mode != 2 && other->mode == 2) {
        const double t = (double)wh->travel + wh->travelVel;
        const double q = -((double)gPhysGravity * up->y) * gPhysSuspGravityScale + wh->radius;
        double zz;

        to.x = (float)((double)p->x - t * up->x);
        to.y = (float)((double)p->y - t * up->y);
        zz = (double)p->z - t * up->z;
        to.x = (float)((double)to.x - q * wh->normal.x);
        to.y = (float)((double)to.y - q * wh->normal.y);
        to.z = (float)(zz - q * wh->normal.z);
        rollDebugLine(p, &to);
        if (ngifzCollideLine(p, &to, hit) && rollAcceptSlope(r, s, rear, &wh->normal)) {
            double e;
            to.x = (float)(((double)wh->normal.x * wh->radius + hit->point.x) - p->x);
            to.y = (float)(((double)wh->normal.y * wh->radius + hit->point.y) - p->y);
            to.z = (float)(((double)wh->radius * wh->normal.z + hit->point.z) - p->z);
            e = -zvec3Dot(&to, up);
            s->travel = (float)e;
            if (e > r->travelMax) {
                s->travel = r->travelMax;
            } else if (!(e >= r->travelMin)) {
                s->travel = r->travelMin;
            }
            shift = e - s->travel;
            goto shifted;
        }
    }

    /* Last chance: straight down the bike's up axis, a little further when rolling. */
    {
        double l = ((double)wh->travel + wh->travelVel) + wh->radius;
        if (wh->mode != 2) {
            l = l + -((double)gPhysGravity * up->y) * gPhysSuspGravityScale;
        }
        to.x = (float)((double)p->x - l * up->x);
        to.y = (float)((double)p->y - l * up->y);
        to.z = (float)((double)p->z - l * up->z);
        rollDebugLine(p, &to);
        if (ngifzCollideLine(p, &to, hit) && rollAcceptSlope(r, s, rear, up)) {
            double e;
            to.x = (float)((double)hit->point.x - p->x);
            to.y = (float)((double)hit->point.y - p->y);
            to.z = (float)((double)hit->point.z - p->z);
            e = -zvec3Dot(&to, up);
            s->travel = (float)(e - wh->radius);
            if (e - wh->radius > r->travelMax) {
                s->travel = r->travelMax;
            } else if (!(s->travel >= r->travelMin)) {
                s->travel = r->travelMin;
            }
            shift = e - ((double)s->travel + wh->radius);
            goto shifted;
        }
    }
    rollAir(r, s, w);
    return kRollNext;

shifted:
    p->x = (float)(-(shift * up->x) + p->x);
    p->y = (float)(-(shift * up->y) + p->y);
    return rollContact(r, s, w, -(shift * up->z));
}

/* Rebuilds the bike basis, centre and velocity from where the two wheels ended up. */
static void rollFinish(PhysRider *r, RollState *s)
{
    ZMatrix *m = &s->m;
    ZVec3 q[2];
    ZVec3 tmp, rp, mid;
    double dx, len, rest, cx;
    unsigned k;

    /* Anything solid between the two wheel centres (raised a little) is a crash. */
    {
        const double ux = (double)r->basis.up.x * 20.0f;
        const double uy = (double)r->basis.up.y * 20.0f;
        const double uz = (double)r->basis.up.z * 20.0f;
        for (k = 0; k < 2; k++) {
            q[k].x = (float)(ux + s->pos[k].x);
            q[k].y = (float)(uy + s->pos[k].y);
            q[k].z = (float)(uz + s->pos[k].z);
        }
        if (ngifzCollideLine(&q[0], &q[1], &s->hit)) {
            physSetState(r, 2);
            r->bailReason = 5;
            return;
        }
    }

    zvec3Zero(&m->pos);
    dx = (double)s->pos[0].x - s->pos[1].x;
    m->right.x = (float)dx;
    m->right.y = (float)((double)s->pos[0].y - s->pos[1].y);
    m->right.z = (float)((double)s->pos[0].z - s->pos[1].z);
    if (x87Equal(dx, 0.0) && x87Equal(m->right.y, 0.0) && x87Equal(m->right.z, 0.0)) {
        zdebugFail("all components of X orientation are zero", physicsFile, 3429, 0);
    }
    zvec3NormalizeScaled(&m->right, &m->right);

    if (s->airborne >= 2) {
        m->up = r->basis.up;
    } else {
        if (s->airborne == 0) {
            const ZVec3 *n0 = &r->wheel[0].normal;
            const ZVec3 *n1 = &r->wheel[1].normal;
            m->up.x = (float)(((double)n1->x - n0->x) * 0.5f + n0->x);
            m->up.y = (float)(((double)n1->y - n0->y) * 0.5f + n0->y);
            m->up.z = (float)(((double)n1->z - n0->z) * 0.5f + n0->z);
        } else {
            for (k = 0; k < 2; k++) {
                const PhysWheel *wh = &r->wheel[k];
                if (wh->mode != 2) {
                    m->up = wh->normal;
                    if (zmathAcos(-wh->normal.y) < 0x238e) {
                        r->flags |= 0x400000;
                    } else {
                        r->flags &= ~0x400000u;
                    }
                    break;
                }
            }
        }
        /* Lean into turns above walking speed. */
        {
            const float thr = gPhysLeanSpeedMin;
            if (r->rot.y != 0 && r->speed > thr) {
                ZMatrix lean;
                const int16_t range = (int16_t)physFtol((double)gPhysLeanRange * DEG_TO_ANGLE);
                double v;
                zmatrixIdentity(&lean);
                v = (double)r->rot.y * 4.5f / range *
                    (((double)r->speed - thr) / ((double)gPhysLeanSpeedMax - thr));
                if (!(1.0 >= v)) {
                    v = 1.0;
                }
                zmatrixRotateX(&lean, &lean,
                               (ZAngle)(0u - (uint32_t)physFtol(v * gPhysLeanMax * DEG_TO_ANGLE)));
                zmatrixMul(m, m, &lean);
            }
        }
        m->up.x = (float)(((double)m->up.x - r->basis.up.x) * gPhysUpBlend + r->basis.up.x);
        m->up.y = (float)(((double)m->up.y - r->basis.up.y) * gPhysUpBlend + r->basis.up.y);
        m->up.z = (float)(((double)m->up.z - r->basis.up.z) * gPhysUpBlend + r->basis.up.z);
    }

    zvec3Cross3(&m->at, &m->right, &gZmathOrigin, &m->up);
    if (x87Equal(m->at.x, 0.0) && x87Equal(m->at.y, 0.0) && x87Equal(m->at.z, 0.0)) {
        zdebugFail("all components of Z orientation are zero", physicsFile, 3469, 0);
    }
    zvec3NormalizeScaled(&m->at, &m->at);
    zvec3Cross3(&m->up, &m->at, &gZmathOrigin, &m->right);
    if (x87Equal(m->up.x, 0.0) && x87Equal(m->up.y, 0.0) && x87Equal(m->up.z, 0.0)) {
        zdebugFail("all components of Y orientation are zero", physicsFile, 3473, 0);
    }
    zvec3NormalizeScaled(&m->up, &m->up);
    zmatrixCopyRotation(&r->basis, m);
    zmatrixCopyRotation(riderFrame(r), &r->basis);

    /* Centre: average of what each wheel position implies. */
    tmp = r->wheel[0].anchor;
    zmatrixTransformPoint(&rp, m, &tmp);
    r->center.x = (float)((double)s->pos[0].x - rp.x);
    r->center.y = (float)((double)s->pos[0].y - rp.y);
    r->center.z = (float)((double)s->pos[0].z - rp.z);
    tmp = r->wheel[1].anchor;
    zmatrixTransformPoint(&rp, m, &tmp);
    tmp.x = (float)((double)s->pos[1].x - rp.x);
    tmp.y = (float)((double)s->pos[1].y - rp.y);
    tmp.z = (float)((double)s->pos[1].z - rp.z);
    cx = ((double)tmp.x - r->center.x) * 0.5f + r->center.x;
    r->center.x = (float)cx;
    r->center.y = (float)(((double)tmp.y - r->center.y) * 0.5f + r->center.y);
    r->center.z = (float)(((double)tmp.z - r->center.z) * 0.5f + r->center.z);
    r->pos.x = (float)cx;
    r->pos.y = r->center.y;
    r->pos.z = r->center.z;

    /* Next frame's wheel positions give the orientation the velocities are measured in. */
    for (k = 0; k < 2; k++) {
        q[k].x = (float)((double)s->pos[k].x + r->wheel[k].vel.x);
        q[k].y = (float)((double)s->pos[k].y + r->wheel[k].vel.y);
        q[k].z = (float)((double)s->pos[k].z + r->wheel[k].vel.z);
    }
    m->right.x = (float)((double)q[0].x - q[1].x);
    m->right.y = (float)((double)q[0].y - q[1].y);
    m->right.z = (float)((double)q[0].z - q[1].z);
    zvec3NormalizeScaled(&m->right, &m->right);
    zvec3Cross3(&m->at, &m->right, &gZmathOrigin, &m->up);
    zvec3NormalizeScaled(&m->at, &m->at);
    zvec3Cross3(&m->up, &m->at, &gZmathOrigin, &m->right);

    tmp = r->wheel[0].anchor;
    zmatrixTransformPoint(&rp, m, &tmp);
    mid.x = (float)((double)q[0].x - rp.x);
    mid.y = (float)((double)q[0].y - rp.y);
    mid.z = (float)((double)q[0].z - rp.z);
    tmp = r->wheel[1].anchor;
    zmatrixTransformPoint(&rp, m, &tmp);
    tmp.x = (float)((double)q[1].x - rp.x);
    tmp.y = (float)((double)q[1].y - rp.y);
    tmp.z = (float)((double)q[1].z - rp.z);
    r->dvel = r->vel;
    mid.x = (float)(((double)tmp.x - mid.x) * 0.5f + mid.x);
    mid.y = (float)(((double)tmp.y - mid.y) * 0.5f + mid.y);
    mid.z = (float)(((double)tmp.z - mid.z) * 0.5f + mid.z);
    zvec3Zero(&r->vel);

    /* Each wheel's velocity is where it will be minus where it is; the bike gets the mean. */
    for (k = 0; k < 2; k++) {
        PhysWheel *wh = &r->wheel[k];
        double qz;

        q[k] = wh->anchor;
        zmatrixTransformPoint(&rp, m, &q[k]);
        q[k] = rp;
        q[k].x = (float)((double)mid.x + q[k].x);
        q[k].y = (float)((double)mid.y + q[k].y);
        qz = (double)mid.z + q[k].z;
        q[k].z = (float)qz;
        wh->vel.x = (float)((double)q[k].x - s->pos[k].x);
        wh->vel.y = (float)((double)q[k].y - s->pos[k].y);
        wh->vel.z = (float)(qz - s->pos[k].z);
        len = zvec3Length(&wh->vel);
        wh->speed = (float)len;
        if ((double)r->fieldC64 * 30.0f + gPhysRestSpeed > len) {
            zvec3Zero(&wh->vel);
            wh->speed = 0.0f;
        }
        if (wh->mode == 0) {
            wh->spin = (int16_t)physFtol((double)wh->speed * 0x1.a1372ap-8f * 65536.0f);
            if (!(zvec3Dot(&wh->dir, &wh->vel) >= 0.0)) {
                wh->spin = -wh->spin;
            }
        }
        zvec3Add(&r->vel, &r->vel, &wh->vel);
    }
    zvec3Scale(&r->vel, &r->vel, 0.5f);
    len = zvec3Length(&r->vel);
    r->speed = (float)len;
    rest = (double)r->fieldC64 * 30.0f + gPhysRestSpeed;
    if (rest > len) {
        zvec3Zero(&r->vel);
        r->speed = 0.0f;
        for (k = 0; k < 2; k++) {
            PhysWheel *wh = &r->wheel[k];
            if (wh->mode != 2) {
                zvec3Zero(&wh->vel);
                wh->speed = 0.0f;
            }
        }
    }
    r->dvel.x = (float)((double)r->vel.x - r->dvel.x);
    r->dvel.y = (float)((double)r->vel.y - r->dvel.y);
    r->dvel.z = (float)((double)r->vel.z - r->dvel.z);
    if (!(zvec3Dot(&r->vel, &r->basis.right) >= 0.0)) {
        r->flags |= 0x40000;
    } else {
        r->flags &= ~0x40000u;
    }
    if (s->airborne == 2) {
        physSetState(r, 1);
    }
}

/*
 * bmxphysUpdatePositionRoll: one frame of the rolling (on-ground) state. Each wheel is moved by
 * its velocity, kept on the ground by line tests below it (with suspension travel), checked for
 * walls ahead, and may switch the rider into bail, launch or air states; then the bike basis,
 * centre and velocities are rebuilt from the two wheel positions.
 */
BMX_ADDR(0x00425940)
void bmxphysUpdatePositionRoll(PhysRider *r)
{
    RollState s;
    unsigned w;

    fxSetRider(r);
    s.airborne = 0;
    for (w = 0; w < 2; w++) {
        if (rollWheel(r, &s, w) == kRollStop) {
            return;
        }
    }
    rollFinish(r, &s);
}
