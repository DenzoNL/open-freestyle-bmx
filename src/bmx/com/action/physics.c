/* Rider and bike physics (C:\zaxis\Bmx\src\com\action\physics.c). */
#include "bmx/com/action/physics.h"

#include "bmx/com/action/contrler.h"
#include "bmx/com/action/fx.h"
#include "bmx/com/action/prop.h"
#include "bmx/com/action/rider.h"
#include "bmx/com/zge/bmx/bmxsound.h"
#include "bmx/com/zge/bmx/zoverii.h"
#include "bmx/com/zge/ngif/ngifpsx.h"
#include "bmx/com/zge/ngif/ngifzactor.h"
#include "bmx/com/zge/ngif/ngifzbsp.h"
#include "bmx/com/zge/ngif/ngifzcollide.h"
#include "bmx/com/zge/ngif/ngifzroller.h"
#include "bmx/com/zge/zactor.h"
#include "compat/crt.h"
#include "zge/zdebug.h"
#include "zge/zgemods/zobjcamera.h"
#include "zge/zquat.h"
#include "zge/zstring.h"

/*
 * Floating point follows the original x87 code exactly: `double` is a value held in an x87
 * register (53-bit precision control), `float` is a value the original stored to memory.
 */

static const char physicsFile[] = "C:\\zaxis\\Bmx\\Src\\com\\action\\physics.c";

/* The rider's fwd/up/side/omega rows double as a ZMatrix (rotation rows, omega in the position slot). */
/*
 * _ftol: the original truncates through a 64-bit FISTP and returns the low 32 bits, so values beyond the
 * int32 range wrap instead of saturating.
 */
static int32_t ftol(double x)
{
    return (int32_t)(int64_t)x;
}

/* x87 FCOM followed by TEST AH,1: below, or unordered. */
static int fpuBelow(double a, double b)
{
    return !(a >= b);
}

static ZMatrix *orient(PhysRider *r)
{
    return (ZMatrix *)(void *)&r->fwd;
}

static ZMatrix *contactAxes(PhysRider *r)
{
    return (ZMatrix *)(void *)&r->contactAxis0;
}

static ZActor *riderActor(const PhysRider *r)
{
    return (ZActor *)r->actor;
}

static ZActor *bikeActor(const PhysRider *r)
{
    return (ZActor *)r->bikeObj;
}

/* Number of riders the per-frame loops visit: at least one. */
static unsigned riderCount(void)
{
    return gPhysPlayerCount < 1 ? 1u : gPhysPlayerCount;
}

/* Grind sound variant of an edge kind. */
static uint32_t edgeSoundKind(const PhysEdge *e)
{
    if (e->kind == 1) {
        return 7;
    }
    if (e->kind == 2) {
        return 2;
    }
    return 0;
}

/* ---- HUD and debug hooks ------------------------------------------------------------------------ */

/* TODO(file): HUD text, probably belongs to the in-game overlay code. */
BMX_ADDR(0x0041f930)
void physHudTextShow(int hide)
{
    zoverShow(gPhysHudHandle, hide);
}

/* Shows (or removes) the centred HUD text 0x10ba. TODO(file): HUD, not physics. */
BMX_ADDR(0x0041f960)
void physHudTextSet(int on)
{
    if (on) {
        const char *text = zstringGet(0x10ba);
        const uint16_t width = (uint16_t)zoverTextWidth(text, &gPhysHudFont);
        const uint32_t handle = zoverAddText(0, &gPhysHudFont, text, 0x210 - (width >> 1), 0x3c, 0xc09020, 0);
        gPhysHudHandle = (uint16_t)handle;
        zoverShow(handle, 0);
        return;
    }
    zoverRemove(gPhysHudHandle);
    gPhysHudHandle = 0xffff;
}

BMX_ADDR(0x0041fa00)
void physTick(void)
{
    gPhysTick++;
}

/* TODO(file): debug-draw registration, probably belongs to the level renderer. */
BMX_ADDR(0x0041fa20)
void physRegisterDebugDraw(void)
{
    zrollerAddDrawCallback(BMX_FNPTR(ZrollerDrawFn, 0x0040252c, physDebugDraw));
}

BMX_ADDR(0x0041fa40)
void physNop(void) {}

/* Pairs a bike part (7, 8) with the matching rider part (3, 6) as limb target `mask`. */
static void limbTarget(PhysRider *r, int bikePart, int riderPart, uint32_t mask)
{
    ZVec3 local, target, axis, base;

    zvec3Zero(&local);
    local.z = 0.0f;
    local.y = 22.0f;
    zactorGetPartPosition(r->bikeObj, bikePart, &local, &target);
    zvec3Zero(&local);
    local.x = 1.0f;
    zactorGetPartPosition(r->actor, riderPart, &local, &axis);
    local.x = 0.0f;
    zactorGetPartPosition(r->actor, riderPart, &local, &base);
    axis.x = axis.x - base.x;
    axis.y = axis.y - base.y;
    axis.z = axis.z - base.z;
    zoverSetLimbTargets(r->ik, &target, &axis, mask);
}

/* Points the rider's hands at the handlebar grips (bike parts 7 and 8). */
BMX_ADDR(0x0041fa60)
void physUpdateLimbTargets(PhysRider *r)
{
    limbTarget(r, 7, 3, 1);
    limbTarget(r, 8, 6, 2);
}

/*
 * Per-frame update of every active rider: orientation, velocity, wheel spin, movement and actor sync
 * handlers of the current state. A switch into the scripted state runs its move handler twice.
 */
BMX_ADDR(0x0041fc40)
void physUpdateRiders(void)
{
    gPhysDebugEdges = NULL;
    for (unsigned i = 0; i < riderCount(); i++) {
        PhysRider *r = &gPhysRiders[i];
        unsigned prev;
        PhysStepFn fn;

        if (!(r->flags & kPhysActive)) {
            continue;
        }
        zactorSetFlag2(riderActor(r));
        zactorClearFlag2(bikeActor(r));
        fn = gPhysStateOrient[physGetState(r)];
        if (fn) {
            fn(r);
        }
        fn = gPhysStateVelocity[physGetState(r)];
        if (fn) {
            fn(r);
        }
        for (int w = 0; w < 2; w++) {
            r->wheel[w].angle = (int16_t)(r->wheel[w].angle + (int16_t)r->wheel[w].spin);
        }
        r->flags &= 0xff3fffffu;
        prev = physGetState(r);
        fn = gPhysStateMove[physGetState(r)];
        if (fn) {
            fn(r);
        }
        if (physGetState(r) == kPhysStateScripted && prev != kPhysStateScripted) {
            gPhysStateMove[kPhysStateScripted](r);
        }
        fn = gPhysStateSync[physGetState(r)];
        if (fn) {
            fn(r);
        }
        bikeActor(r)->player->ch[0].joints[6].x = r->wheel[0].angle;
        bikeActor(r)->player->ch[0].joints[5].x = r->wheel[1].angle;
        if ((gPhysSyncFlags & 8) && gPhysPlayerCount != 0) {
            riderActor(r)->player->ch[0].joints[2].y = r->wheel[0].angle;
        }
    }
}

/* Puts the rider at rest on the respawn face nearest to its position, upright on the face normal. */
BMX_ADDR(0x0041fe10)
void physRespawn(PhysRider *r)
{
    PhysFace *face;
    ZVec3 n;
    double h;

    zvec3Zero(&r->step);
    zvec3Zero(&r->fieldBFC);
    zvec3Zero(&r->vel);
    zvec3Zero(&r->dvel);
    r->speed = 0.0f;
    r->rot.x = 0;
    r->rot.y = 0;
    r->rot.z = 0;
    r->yawTrim = 0;
    r->field58 = 0;
    r->fieldC60 = 0.0f;
    r->fieldC64 = 0.0f;
    for (int w = 0; w < 2; w++) {
        zvec3Zero(&r->wheel[w].vel);
        r->wheel[w].speed = 0.0f;
        r->wheel[w].mode = 0;
        r->wheel[w].angle = 0;
        r->wheel[w].spin = 0;
    }
    face = physFindRespawnFace(&r->pos, 0);
    if (face == NULL) {
        zdebugFail("couldn't find respawn face", physicsFile, 0x1bf, 0);
    }
    r->surface = 0;
    r->contact = face;
    physContactCopy(r);
    zbspFaceCentroid(face, &r->pos);
    /* Like the original, carries on after the assert. */
    n = face->normal; // NOLINT(clang-analyzer-core.NullDereference)
    if (__builtin_fabs(zvec3Dot(&r->fwd, &n)) > 0.99f) {
        r->fwd = r->up;
    }
    r->up = n;
    zvec3Cross3(&r->side, &r->fwd, &gZmathOrigin, &r->up);
    zvec3NormalizeScaled(&r->side, &r->side);
    zvec3Cross3(&r->fwd, &r->up, &gZmathOrigin, &r->side);
    zvec3Zero(&r->omega);
    zactorFaceMatrix(riderActor(r), orient(r));
    h = ((double)r->wheel[1].radius + r->wheel[0].radius) * 0.5f;
    r->center.x = (float)(h * r->up.x + r->pos.x);
    r->center.y = (float)(h * r->up.y + r->pos.y);
    r->center.z = (float)(h * r->up.z + r->pos.z);
    zmatrixCopyRotation(&r->basis, orient(r));
    r->state = kPhysStateRoll;
}

/*
 * Finds a face to respawn on. Unless flags bit 0 is set, probes the ground on a widening spiral around
 * pos (24 tries) for a flat, plain face with no prop on it; bit 1 limits the height difference. Falls
 * back to the nearest upward face tagged 0xfa1..0xfa4.
 */
BMX_ADDR(0x004200b0)
PhysFace *physFindRespawnFace(const ZVec3 *pos, uint32_t flags)
{
    PhysFace *found = NULL;
    PhysFace *best = NULL;
    void **list;
    PhysFace *f;
    uint32_t bestDist;

    if (!(flags & 1)) {
        float dist = 0.0f;
        ZAngle angle = 0;
        int tries = 0x19;

        do {
            PhysGroundHit hit;
            ZVec3 v;

            if (--tries == 0) {
                break;
            }
            v.x = dist;
            v.y = 0.0f;
            v.z = 0.0f;
            zvec3RotateY(&v, &v, angle);
            zvec3Add(&v, &v, pos);
            if (physProbeGround(&v, &hit) && !(hit.flags & 1)) {
                int ok = 1;
                if (flags & 2) {
                    const double dy = (double)hit.pos.y - pos->y;
                    ok = fpuBelow(dy, 400.0f) && !fpuBelow(dy, -200.0f);
                }
                if (ok && zmathAcos(-hit.normal.y) <= 0x1555) {
                    const PhysFace *face = hit.face;
                    const unsigned kind = face->tag >> 12;
                    if (!(face->flags & 0x10) && (kind == 0 || (kind > 3 && kind <= 8)) &&
                        (*(const uint32_t *)hit.material & 0xf) != 6) {
                        const uint8_t *near = zbspFindFaceNear(&v, 50.0f, (float)zmathCos(0x2aaa));
                        const int32_t *props = near ? *(const int32_t *const *)(near + 0x3c) : NULL;
                        if (props == NULL || *props == 0) {
                            best = hit.face;
                            found = hit.face;
                        }
                    }
                }
            }
            angle = (ZAngle)(angle + 0x2aaa);
            if (dist == 0.0f) {
                dist = (float)((double)dist + 100.0f);
            } else {
                dist = (float)((double)dist * 1.1f);
            }
        } while (found == NULL);
        if (tries == 0) {
            gZdebugPrint("warning: no respawn faces found by distance check\n");
        }
        if (found != NULL) {
            return found;
        }
    }
    bestDist = 0xffffffffu;
    list = zbspFindTaggedFaces(0xfa1, 0xfa4);
    if (list == NULL) {
        gZdebugPrint("warning: no tagged respawn faces found in level\n");
        return found;
    }
    f = *list++;
    if (f == NULL) {
        return found;
    }
    do {
        ZVec3 c;
        int32_t dx, dz;
        uint32_t d;

        zbspFaceCentroid(f, &c);
        dx = ftol((double)c.x - pos->x);
        dz = ftol((double)c.z - pos->z);
        d = (uint32_t)dz * (uint32_t)dz + (uint32_t)dx * (uint32_t)dx;
        if (d < bestDist && fpuBelow(f->normal.y, 0.0f)) {
            best = f;
            bestDist = d;
        }
        f = *list++;
    } while (f != NULL);
    return best;
}

BMX_ADDR(0x00420c90)
uint8_t physGetState(const PhysRider *r)
{
    return r->state;
}

/*
 * Switches state: runs the exit handler of the current state (the one of state 2 when there is none
 * yet) and the enter handler of the new one, which decides the state actually entered.
 */
BMX_ADDR(0x00420cb0)
uint8_t physSetState(PhysRider *r, int state)
{
    const int cur = physGetState(r);

    if (cur != state) {
        if (cur == kPhysStateNone) {
            gPhysStateExit[2](r, kPhysStateNone, state);
        } else if (gPhysStateExit[cur] != NULL) {
            gPhysStateExit[cur](r, cur, state);
        }
        r->state = (uint8_t)gPhysStateEnter[state](r, cur, state);
    }
    return r->state;
}

BMX_ADDR(0x00420d30)
void physAddVelocity(PhysRider *r, const ZVec3 *dv)
{
    zvec3Add(&r->vel, &r->vel, dv);
    r->speed = (float)zvec3Length(&r->vel);
}

/* Casts a vertical ray through the whole world at p's x/z and reports the face or edge hit from above. */
BMX_ADDR(0x00420d70)
int physProbeGround(const ZVec3 *p, PhysGroundHit *out)
{
    ZVec3 to, from;
    const float *bounds;

    out->flags = 0;
    from = *p;
    to = *p;
    bounds = zrollerGetWorldBounds();
    to.y = (float)((double)bounds[1] + 1.0f);
    from.y = (float)((double)bounds[5] - 1.0f);
    out->face = NULL;
    out->edge = NULL;
    out->material = NULL;
    if (zcollideRay(&from, &to, &gPhysRay) && fpuBelow(gPhysRay.normal.y, 0.0f)) {
        out->pos = gPhysRay.point;
        out->normal = gPhysRay.normal;
        out->d = gPhysRay.normalW;
        if ((gPhysRay.flags & 1) && gPhysRay.face != NULL) {
            out->face = gPhysRay.face;
            out->material = (void *)(uintptr_t)gPhysRay.material;
            return 1;
        }
        if ((gPhysRay.flags & 2) && gPhysRay.edge != NULL) {
            out->flags |= 1;
            out->edge = gPhysRay.edge;
            return 1;
        }
    }
    return 0;
}

/* Exit handler of the grind state. */
BMX_ADDR(0x00420f00)
int physExitGrind(PhysRider *r, int from, int to)
{
    r->rot.x = 0;
    r->rot.y = 0;
    r->rot.z = 0;
    return from;
}

/*
 * Exit handler of the air and line-up states. Landing (to 0) takes the step from the actor's root
 * pose, a switch to the scripted state resets it. Either way the contact is snapshotted into snapB.
 */
BMX_ADDR(0x00420f30)
int physExitAir(PhysRider *r, int from, int to)
{
    r->flags &= ~(uint32_t)kPhysLanding;
    if (to == kPhysStateRoll) {
        const ZActorRootPose *root = zactorGetRootPose(riderActor(r));
        ZVec3 t;

        r->step = root->pos;
        zmatrixTransformPoint(&t, &r->basis, &r->step);
        r->step = t;
        zvec3Negate(&r->step, &r->step);
        r->pos = r->center;
    } else if (to == kPhysStateScripted) {
        zactorSetHeight(riderActor(r), 0.0f);
        zvec3Zero(&r->step);
    }
    r->snapBPos = r->pos;
    r->snapB.contact = r->contact;
    r->snapB.surface = r->surface;
    physContactSnapshotFix(&r->snapB);
    return from;
}

/* Exit handler of the scripted state. */
BMX_ADDR(0x004210a0)
int physExitScripted(PhysRider *r, int from, int to)
{
    return from;
}

/* Enter handler of the roll state. Coming off a grind decides whether the rider rolls backwards. */
BMX_ADDR(0x004210c0)
int physEnterRoll(PhysRider *r, int from, int to)
{
    if (from == kPhysStateGrind) {
        zmatrixCopyRotation(&r->basis, orient(r));
        if (fpuBelow(zvec3Dot(&r->fwd, &r->vel), 0.0f)) {
            r->flags |= kPhysFakie;
        } else {
            r->flags &= ~(uint32_t)kPhysFakie;
        }
    }
    r->rot.x = 0;
    r->rot.y = 0;
    r->rot.z = 0;
    r->groundGap = 0.0f;
    zoverLimbsBegin(r->actor, r->ik);
    zoverResetLimbs(r->ik);
    return to;
}

/* Squared length of an integer offset, with the original's 32-bit wrap-around. */
static int32_t dist2i(int32_t dx, int32_t dy, int32_t dz)
{
    return (int32_t)((uint32_t)dz * (uint32_t)dz + (uint32_t)dy * (uint32_t)dy + (uint32_t)dx * (uint32_t)dx);
}

/* Contact normal of a prop: the horizontal facing of its scene rotation, reversed. */
static void propNormal(const PhysPropObject *obj, ZVec3 *normal)
{
    ZMatrix m;
    ZAngle3 ang;
    ZQuat q;
    const PhysPropFrame *f = obj->node->frame;

    zmatrixIdentity(&m);
    ang.x = obj->rot.x;
    ang.y = (ZAngle)-obj->rot.y;
    ang.z = obj->rot.z;
    q.x = f->rot.x;
    q.y = -f->rot.y;
    q.z = f->rot.z;
    q.w = f->rot.w;
    zquatToMatrix(&m, &q);
    zmatrixRotateEuler(&m, &m, &ang);
    zmatrixRotateEuler(&m, &m, &obj->rot);
    m.at.y = 0.0f;
    zvec3NormalizeScaled(&m.at, &m.at);
    normal->x = -m.at.x;
    normal->y = -m.at.y;
    normal->z = -m.at.z;
}

/*
 * Enter handler of the grind-in and stall-in states (3, 5). Follows the rider's flight path (gravity
 * applied, gPhysGrindInSteps or gPhysStallInSteps steps) looking for something to land on: the nearest
 * grind edge (surface 9), or for lip tricks (surface 11) a steep lip face or a prop (surface 12). Sets
 * up the contact frame, the frame count and the air velocity that reaches the target in that many
 * frames. Returns `from` when nothing is in reach; a slow grind becomes a stall-in (5).
 */
BMX_ADDR(0x00421190)
int physEnterGrindIn(PhysRider *r, int from, int to)
{
    uint32_t trick;
    uint8_t kind = 9;
    int32_t radius2, bestD2, dx, dy, dz;
    uint32_t steps, frames;
    float radius, step = 10.0f, bestY = 32767.0f;
    ZVec3 probe, vel, tmp, bestPoint, alignedPoint, propN;
    PhysEdge *best = NULL, *aligned = NULL;
    const ZVec3 *contact = NULL; /* lip face record, or the prop normal */
    PhysPropObject *propHit = NULL;
    ZAngle bestAngle = 0x7fff;
    void **list;
    ZMatrix m, rot;

    if (to == kPhysStateGrindIn) {
        trick = riderGetTrickA(r);
    } else {
        trick = riderGetTrickKind(r);
        if (trick >= 0x2a && trick <= 0x2b) {
            kind = 11;
        }
    }
    radius2 = bmxFtol(gPhysGrindInRadius);
    bestD2 = (int32_t)((uint32_t)radius2 * (uint32_t)radius2);
    probe = r->center;
    vel = r->vel;
    if (kind == 11) {
        step = 30.0f;
    }
    radius = physGetState(r) == kPhysStateAir ? gPhysEdgeSearchRadius : 50.0f;
    steps = to == kPhysStateGrindIn ? gPhysGrindInSteps : gPhysStallInSteps;
    list = kind == 9 ? zbspFindEdgesNear(&r->center, 400.0f) : zbspFindPropsNear(10, 13);
    gPhysDebugEdges = (PhysEdge **)list;

    for (uint32_t i = 0; i < steps && propHit == NULL; i++) {
        const int32_t reach = bmxFtol(radius);
        const int32_t reach2 = (int32_t)((uint32_t)reach * (uint32_t)reach);

        if (kind == 9) {
            for (void **p = list; p != NULL && *p != NULL; p++) {
                PhysEdge *e = *p;
                ZVec3 cp;
                int32_t d2;
                ZAngle a;

                physClosestPointOnEdge(&probe, e, &cp);
                tmp.x = (float)((double)cp.x - r->center.x);
                tmp.y = (float)((double)cp.y - r->center.y);
                tmp.z = (float)((double)cp.z - r->center.z);
                if (fpuBelow(zvec3Dot(&tmp, &r->vel), 0.0f)) {
                    continue;
                }
                dx = bmxFtol((double)cp.x - probe.x);
                dy = bmxFtol((double)cp.y - probe.y);
                dz = bmxFtol((double)cp.z - probe.z);
                d2 = dist2i(dx, dy, dz);
                if ((uint32_t)d2 > (uint32_t)reach2) {
                    continue;
                }
                if (d2 < bestD2) {
                    zmemCopy(&gPhysGrindEdge, e, sizeof(PhysEdge));
                    e = &gPhysGrindEdge;
                    best = e;
                    bestD2 = d2;
                    bestPoint = cp;
                }
                a = zmathAcos((float)__builtin_fabs(zvec3Dot(&e->dir, &r->fwd)));
                if (a < bestAngle) {
                    zmemCopy(&gPhysAlignEdge, e, sizeof(PhysEdge));
                    aligned = &gPhysAlignEdge;
                    bestAngle = a;
                    alignedPoint = cp;
                }
            }
        } else if (kind > 9 && kind <= 11 && (i & 3) == 0) {
            const ZbspFaceQuery *q;

            for (void **p = list; p != NULL && *p != NULL; p++) {
                PhysPropObject *obj = *p;
                int32_t d2;

                dx = obj->pos[0] - bmxFtol(probe.x);
                dy = obj->pos[1] - bmxFtol(probe.y);
                dz = obj->pos[2] - bmxFtol(probe.z);
                d2 = dist2i(dx, dy, dz);
                if (d2 > reach2) {
                    continue;
                }
                tmp.x = (float)((double)obj->pos[0] - probe.x);
                tmp.y = (float)((double)obj->pos[1] - probe.y);
                tmp.z = (float)((double)obj->pos[2] - probe.z);
                if (obj->prop == NULL || zvec3Dot(&obj->prop->orient.up, &tmp) > 0.0f) {
                    continue;
                }
                tmp.x = (float)(int16_t)dx;
                tmp.y = 0.0f;
                tmp.z = (float)(int16_t)dz;
                if (fpuBelow(zvec3Dot(&tmp, &vel), 0.0f)) {
                    continue;
                }
                contact = &propN;
                bestPoint.x = (float)obj->pos[0];
                bestPoint.y = (float)obj->pos[1];
                bestPoint.z = (float)obj->pos[2];
                bestY = bestPoint.y;
                bestD2 = d2;
                propHit = obj;
                propNormal(obj, &propN);
                kind = 12;
                break;
            }
            q = propHit == NULL ? zbspFindFaceNear(&probe, radius, 1.0f) : NULL;
            for (const ZbspFaceHit *h = q != NULL ? q->hits : NULL; h != NULL && h->face != NULL; h++) {
                const PhysFace *f = h->face;
                int32_t d2;

                if (kind == 10) {
                    if (zmathAcos(__builtin_fabsf(f->normal.y)) >= 0x18e3) {
                        continue;
                    }
                } else if (kind == 11) {
                    tmp = f->normal;
                    if (h->object != NULL || (f->flags & 0x10) ||
                        zmathAcos(__builtin_fabsf(f->normal.y)) <= 0x3555 || zvec3Dot(&tmp, &vel) > 0.0f ||
                        !(h->dist > 0.0f) || (f->flags & 0x30)) {
                        continue;
                    }
                } else {
                    continue;
                }
                dx = bmxFtol((double)h->point.x - probe.x);
                dy = bmxFtol((double)h->point.y - probe.y);
                dz = bmxFtol((double)h->point.z - probe.z);
                d2 = dist2i(dx, dy, dz);
                if (d2 >= bestD2 || !fpuBelow(h->point.y, bestY)) {
                    continue;
                }
                zmemCopy(&gPhysLipFace, f, sizeof(PhysFace));
                contact = &gPhysLipFace.normal;
                bestPoint = h->point;
                bestD2 = d2;
                bestY = h->point.y;
            }
        }
        vel.y = (float)((double)gPhysGravity + vel.y);
        zvec3Add(&probe, &probe, &vel);
        radius = (float)((double)radius + step);
    }

    /* A slightly farther edge that lines up better with the bike wins. */
    if (kind == 9 && aligned != NULL && best != NULL) {
        const int32_t near2 =
            dist2i(bmxFtol((double)bestPoint.x - r->center.x), bmxFtol((double)bestPoint.y - r->center.y),
                   bmxFtol((double)bestPoint.z - r->center.z));
        const int32_t far2 = dist2i(bmxFtol((double)alignedPoint.x - r->center.x),
                                    bmxFtol((double)alignedPoint.y - r->center.y),
                                    bmxFtol((double)alignedPoint.z - r->center.z));
        const uint32_t diff = (uint32_t)near2 - (uint32_t)far2;
        const uint32_t sign = (uint32_t)((int32_t)diff >> 31);

        if ((int32_t)((diff ^ sign) - sign) < 0x9c4) {
            best = aligned;
            bestPoint = alignedPoint;
        }
    }
    if (best == NULL && contact == NULL) {
        return from;
    }

    /* Frames to the target: from the vertical speed when the flight is steep, else horizontally. */
    dx = bmxFtol((double)bestPoint.x - r->center.x);
    dy = bmxFtol((double)bestPoint.y - r->center.y);
    dz = bmxFtol((double)bestPoint.z - r->center.z);
    {
        const float vy = r->vel.y;
        float hlen;

        tmp.x = r->vel.x;
        tmp.y = 0.0f;
        tmp.z = r->vel.z;
        hlen = (float)zvec3Length(&tmp);
        if (!fpuBelow(hlen, __builtin_fabsf(vy))) {
            if (!(hlen < 0.0f || hlen > 0.0f)) {
                frames = steps;
            } else {
                const int32_t h2 = (int32_t)((uint32_t)dz * (uint32_t)dz + (uint32_t)dx * (uint32_t)dx);
                frames = (uint32_t)bmxFtol((double)(int16_t)bmxFtol(__builtin_sqrt((double)h2)) / hlen);
            }
        } else {
            const double disc = (((double)bestPoint.y - r->center.y) * gPhysGravity) * 2.0 + (double)vy * vy;

            if (fpuBelow(disc, 0.0f)) {
                frames = steps;
            } else {
                const double s = __builtin_sqrt(disc);
                const double rise = -(double)vy;

                if (s > rise) {
                    frames = (uint32_t)bmxFtol((s - r->vel.y) / gPhysGravity);
                } else {
                    frames = (uint32_t)bmxFtol((rise - s) / gPhysGravity);
                }
            }
        }
    }
    if (frames > steps) {
        frames = steps;
    }
    if (frames < 6) {
        frames = 6;
    }

    if (kind >= 10 && kind <= 12) {
        /*
         * Lip face or prop: up is the surface normal, right runs along it (the original first picks a
         * heading from side/fwd here and then overwrites it).
         */
        m.up = *contact;
        zvec3Cross3(&m.right, &m.up, &gZmathOrigin, &gPhysDown);
        zvec3NormalizeScaled(&m.right, &m.right);
        if (fpuBelow(zvec3Dot(&m.right, &r->fwd), 0.0f)) {
            zvec3Negate(&m.right, &m.right);
        }
        zvec3Cross3(&m.at, &m.right, &gZmathOrigin, &m.up);
        zvec3NormalizeScaled(&m.at, &m.at);
        zvec3Zero(&m.pos);
        if (kind != 10) {
            zmatrixIdentity(&rot);
            zmatrixRotateX(&rot, &rot, m.at.y > 0.0f ? -0x2000 : 0x2000);
            zmatrixMul(&m, &m, &rot);
        }
        r->surface = kind;
        r->contact = kind == 12 ? (void *)propHit : (void *)contact;
        physContactCopy(r);
        r->contactPos = bestPoint;
        zmatrixCopyRotation(contactAxes(r), &m);
        if (fpuBelow(m.at.y, 0.0f)) {
            r->flags |= kPhysGrindReverse;
        } else {
            r->flags &= ~(uint32_t)kPhysGrindReverse;
        }
        if (kind != 12 && trick == 0x2b) {
            tmp.x = (float)((double)m.right.x * 30.0f + bestPoint.x);
            tmp.y = (float)((double)m.right.y * 30.0f + bestPoint.y);
            tmp.z = (float)((double)m.right.z * 30.0f + bestPoint.z);
            fxSkidMark(&bestPoint, &tmp, 0, contact);
        }
    } else {
        uint32_t move = trick;
        int reverse;

        if (fpuBelow(__builtin_fabs(zvec3Dot(&best->dir, &r->vel)), gPhysGrindMinSpeed)) {
            to = kPhysStateStallIn;
            move = riderGetTrickB(r);
        }
        if (move >= 0x27 && move <= 0x29) {
            /* Tail tricks: the bike points across the edge. */
            m.at = best->dir;
            if (fpuBelow(zvec3Dot(&r->side, &m.at), 0.0f)) {
                zvec3Negate(&m.at, &m.at);
            }
            zvec3Cross3(&m.right, &gPhysDown, &gZmathOrigin, &m.at);
            zvec3NormalizeScaled(&m.right, &m.right);
            zvec3Cross3(&m.up, &m.at, &gZmathOrigin, &m.right);
            zvec3Zero(&m.pos);
            reverse = 0;
        } else {
            /* Along the edge; which way round the rider sits depends on the solid side of the edge. */
            const int flip = fpuBelow(zvec3Dot(&r->fwd, &best->dir), 0.0f);
            const uint32_t held = (r->fieldBA4 >> 3) & 1;
            int solid, ahead;
            float across;

            m.right = best->dir;
            if (flip) {
                zvec3Negate(&m.right, &m.right);
            }
            zvec3Cross3(&m.at, &m.right, &gZmathOrigin, &gPhysDown);
            zvec3NormalizeScaled(&m.at, &m.at);
            zvec3Cross3(&m.up, &m.at, &gZmathOrigin, &m.right);
            zvec3Zero(&m.pos);
            tmp.x = (float)(int16_t)dx;
            tmp.y = (float)(int16_t)dy;
            tmp.z = (float)(int16_t)dz;
            across = (float)zvec3Dot(&m.at, &tmp);
            if (to == kPhysStateGrindIn && from == kPhysStateRoll) {
                zmatrixIdentity(&rot);
                zmatrixRotateX(&rot, &rot, fpuBelow(across, 0.0f) ? 0x1555 : -0x1555);
                zmatrixMul(&m, &m, &rot);
            }
            ahead = !fpuBelow(across, 0.0f);
            solid = zbspEdgeSide(best);
            if (solid == 0) {
                reverse = (int)(held ^ (uint32_t)ahead);
            } else {
                ZVec3 at = m.at;

                if ((solid < 0) == flip) {
                    zvec3Negate(&at, &at);
                }
                reverse = (int)(held ^ (uint32_t)ahead ^ (uint32_t)!fpuBelow(zvec3Dot(&at, &tmp), 0.0f));
            }
        }
        if (reverse) {
            r->flags |= kPhysGrindReverse;
        } else {
            r->flags &= ~(uint32_t)kPhysGrindReverse;
        }
        r->surface = 9;
        r->contact = best;
        physContactCopy(r);
        r->contactPos = bestPoint;
        zmatrixCopyRotation(contactAxes(r), &m);
    }

    if (to != from) {
        double n;

        r->timer = (int16_t)frames;
        r->pos.x = (float)((double)r->pos.x + r->step.x);
        r->pos.y = (float)((double)r->step.y + r->pos.y);
        r->pos.z = (float)((double)r->step.z + r->pos.z);
        zvec3Zero(&r->step);
        n = (double)(int32_t)frames;
        r->airVel.x = (float)(((double)bestPoint.x - r->pos.x) / n);
        r->airVel.y = (float)(((double)bestPoint.y - r->pos.y) / n - (double)gPhysGravity * 0.5f * n);
        r->airVel.z = (float)(((double)bestPoint.z - r->pos.z) / n);
    }
    return to;
}

/* ---- per-state velocity --------------------------------------------------------------------------- */

BMX_ADDR(0x00425740)
void physVelocityRoll(PhysRider *r) {}

/* Gravity and air drag. */
BMX_ADDR(0x00425760)
void physVelocityAir(PhysRider *r)
{
    r->dvel.x = 0.0f;
    r->dvel.y = gPhysGravity;
    r->dvel.z = 0.0f;
    r->vel.y = (float)((double)gPhysGravity + r->vel.y);
    r->speed = (float)zvec3Length(&r->vel);
    zvec3Scale(&r->vel, &r->vel, (float)(1.0f - (double)gPhysAirDrag));
}

/* Gravity along the grind edge (pointing the way the rider faces) and grind drag. */
BMX_ADDR(0x004257e0)
void physVelocityGrind(PhysRider *r)
{
    const PhysEdge *e = r->contact;
    ZVec3 g = e->dir;
    double pull;

    if (fpuBelow(zvec3Dot(&r->fwd, &e->dir), 0.0f)) {
        zvec3Negate(&g, &g);
    }
    pull = (double)gPhysGravity * g.y;
    g.x = (float)(pull * g.x);
    r->dvel.x = g.x;
    g.y = (float)(pull * g.y);
    g.z = (float)(pull * g.z);
    r->dvel.z = g.z;
    r->dvel.y = g.y;
    zvec3Add(&r->vel, &r->vel, &g);
    zvec3Scale(&r->vel, &r->vel,
               (float)(1.0f - ((1.0f - (double)r->stat[5]) * 10.0f + 1.0f) * gPhysGrindDrag));
    r->speed = (float)zvec3Length(&r->vel);
}

/* Gravity on the line-up drift. */
BMX_ADDR(0x00425910)
void physVelocityGrindIn(PhysRider *r)
{
    r->airVel.y = (float)((double)gPhysGravity + r->airVel.y);
}

/* ---- orientation --------------------------------------------------------------------------------- */

/* Eases fwd/up towards the contact axes and re-orthonormalises. */
BMX_ADDR(0x00425610)
void physOrientToContact(PhysRider *r)
{
    r->fwd.x = (float)(((double)r->contactAxis0.x - r->fwd.x) * gPhysAlignRate + r->fwd.x);
    r->fwd.y = (float)(((double)r->contactAxis0.y - r->fwd.y) * gPhysAlignRate + r->fwd.y);
    r->fwd.z = (float)(((double)r->contactAxis0.z - r->fwd.z) * gPhysAlignRate + r->fwd.z);
    r->up.x = (float)(((double)r->contactAxis1.x - r->up.x) * gPhysAlignRate + r->up.x);
    r->up.y = (float)(((double)r->contactAxis1.y - r->up.y) * gPhysAlignRate + r->up.y);
    r->up.z = (float)(((double)r->contactAxis1.z - r->up.z) * gPhysAlignRate + r->up.z);
    zvec3Cross3(&r->side, &r->fwd, &gZmathOrigin, &r->up);
    zvec3NormalizeScaled(&r->side, &r->side);
    zvec3Cross3(&r->fwd, &r->up, &gZmathOrigin, &r->side);
}

/* ---- edges and contacts --------------------------------------------------------------------------- */

BMX_ADDR(0x0042b570)
int physEdgeEqual(const PhysEdge *a, const PhysEdge *b)
{
    return a->a[0] == b->a[0] && a->a[1] == b->a[1] && a->a[2] == b->a[2] && a->b[0] == b->b[0] &&
           a->b[1] == b->b[1] && a->b[2] == b->b[2];
}

BMX_ADDR(0x0042cca0)
void physClosestPointOnEdge(const ZVec3 *p, const PhysEdge *e, ZVec3 *out)
{
    zbspClosestPointOnEdge(e->a, e->b, &e->dir, p, out);
}

/* Signed distance of p (truncated to integers) from plane {nx, ny, nz, d}. */
BMX_ADDR(0x0042d670)
double physPlaneDistance(const ZVec3 *p, const float *plane)
{
    const int32_t z = ftol(p->z);
    const int32_t y = ftol(p->y);
    const int32_t x = ftol(p->x);

    return (((double)z * plane[2] + (double)y * plane[1]) + (double)x * plane[0]) + plane[3];
}

/* Point of segment a-b (int16 coordinates) closest to p. */
BMX_ADDR(0x0042d6e0)
void physClosestPointOnSegment(const int16_t *a, const int16_t *b, const int16_t *p, ZVec3 *out)
{
    ZVec3 d;
    double s;

    d.x = (float)(int16_t)(b[0] - a[0]);
    d.y = (float)(int16_t)(b[1] - a[1]);
    d.z = (float)(int16_t)(b[2] - a[2]);
    zvec3NormalizeScaled(&d, &d);
    s = ((double)(int16_t)(a[2] - p[2]) * d.z + (double)(int16_t)(a[1] - p[1]) * d.y) +
        (double)(int16_t)(a[0] - p[0]) * d.x;
    if (!fpuBelow(s, 0.0f)) {
        out->x = (float)a[0];
        out->y = (float)a[1];
        out->z = (float)a[2];
        return;
    }
    s = ((double)(int16_t)(b[2] - p[2]) * d.z + (double)(int16_t)(b[1] - p[1]) * d.y) +
        (double)(int16_t)(b[0] - p[0]) * d.x;
    if (s > 0.0f) {
        out->x = (float)((double)b[0] - s * d.x);
        out->y = (float)((double)b[1] - s * d.y);
        out->z = (float)((double)b[2] - s * d.z);
        return;
    }
    out->x = (float)b[0];
    out->y = (float)b[1];
    out->z = (float)b[2];
}

/*
 * Of all edges tagged `id`, returns the one closest to p as a copy in a static record (NULL when there
 * is none).
 */
BMX_ADDR(0x0042d950)
PhysEdge *physFindNearestTaggedEdge(int unused, int id, const ZVec3 *p)
{
    PhysEdge *best = NULL;
    float bestDist = 1e11f;
    const PhysEdge *e = zbspFindEdge(unused, id, 0, 0);
    int index = 0;

    if (e == NULL) {
        return NULL;
    }
    do {
        int16_t q[3];
        ZVec3 c;
        double dx, dy, dz, d;

        gPhysEdgeProbe = *e;
        q[0] = (int16_t)ftol(p->x);
        q[1] = (int16_t)ftol(p->y);
        q[2] = (int16_t)ftol(p->z);
        physClosestPointOnSegment(gPhysEdgeProbe.a, gPhysEdgeProbe.b, q, &c);
        dx = (double)c.x - q[0];
        dy = (double)c.y - q[1];
        dz = (double)c.z - q[2];
        d = (dz * dz + dy * dy) + dx * dx;
        if (fpuBelow(d, bestDist)) {
            bestDist = (float)d;
            gPhysBestEdge = gPhysEdgeProbe;
            best = &gPhysBestEdge;
        }
        index++;
        e = zbspFindEdge(unused, id, index, 0);
    } while (e != NULL);
    return best;
}

/*
 * Takes the face or edge of a ray hit as the new contact. Quarter-pipe faces (kinds 1-3) resolve to
 * their lip edge; returns 0 when the hit has neither.
 */
BMX_ADDR(0x0042dad0)
int physTakeContact(PhysRider *r, const PhysRayHit *hit)
{
    if ((hit->flags & 1) && hit->face != NULL) {
        const PhysFace *face = hit->face;
        const unsigned kind = face->tag >> 12;

        if (kind >= 1 && kind <= 3) {
            r->contact = physFindNearestTaggedEdge(hit->id, face->tag & 0xfff, &r->pos);
            r->surface = (uint8_t)(face->tag >> 12);
            physContactCopy(r);
            if (r->contact == NULL) {
                gZdebugPrint("bmxphysUpdatePositionAir: quarterpipe face with no assigned edge, ID=%d\n",
                             face->tag & 0xfff);
                r->surface = 0;
                r->contact = hit->face;
                physContactCopy(r);
            }
            if ((r->surface == 1 || r->surface == 3) && (r->flags & kPhysQuarterPipe) &&
                !(r->flags & kPhysNoPipeAlign)) {
                physAlignToQuarterPipe(r, r->contact, 0);
            }
            return 1;
        }
        if (kind >= 4 && kind <= 8) {
            r->surface = (uint8_t)(kind & 0xf);
            r->contact = hit->face;
            physContactCopy(r);
            return 1;
        }
        r->surface = 0;
        r->contact = hit->face;
        physContactCopy(r);
        r->flags &= ~(uint32_t)kPhysNoPipeAlign;
        return 1;
    }
    if ((hit->flags & 2) && hit->edge != NULL) {
        r->surface = 12;
        r->contact = hit->edge;
        physContactCopy(r);
        return 1;
    }
    return 0;
}

/* Size of the contact record of a surface kind (0 for unknown kinds): a face, an edge, a prop object. */
static uint32_t contactSize(uint8_t surface)
{
    switch (surface) {
    case 0:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 10:
    case 11:
    case 13:
    case 14:
        return sizeof(PhysFace);
    case 1:
    case 2:
    case 3:
    case 9:
        return sizeof(PhysEdge);
    case 12:
        return sizeof(PhysPropObject);
    default:
        return 0;
    }
}

/* Copies a snapshot's contact record into the snapshot itself so it survives level queries. */
BMX_ADDR(0x0042dce0)
void physContactSnapshotFix(PhysContactSnap *s)
{
    const uint32_t size = contactSize(s->surface);

    if (s->contact != NULL && size != 0) {
        zmemCopy(&s->data, s->contact, size);
        s->contact = &s->data;
    }
}

/* Copies the current contact record into the rider (the level query results are static buffers). */
BMX_ADDR(0x0042dda0)
void physContactCopy(PhysRider *r)
{
    const uint32_t size = contactSize(r->surface);

    if (r->contact != NULL && size != 0) {
        zmemCopy(&r->contactData, r->contact, size);
        r->contact = &r->contactData;
    }
}
/* Steering on the ground: leans into turns at moderate speed, then applies and damps the yaw. */
BMX_ADDR(0x00424800)
void physOrientRoll(PhysRider *r)
{
    ZMatrix m;

    zmatrixIdentity(&m);
    if (__builtin_fabs((double)zmathAcos(__builtin_fabsf(r->up.y))) > 5461.0f &&
        fpuBelow(__builtin_fabs((double)zmathAcos(__builtin_fabsf(r->side.y))), 10922.0f) &&
        fpuBelow(r->speed, 50.0f) && r->speed > 0.1f &&
        ((double)r->dvel.x * r->dvel.x + (double)r->dvel.y * r->dvel.y) + (double)r->dvel.z * r->dvel.z >
            0.1f) {
        r->rot.y =
            (ZAngle)(r->rot.y + (int16_t)ftol((((50.0f - (double)r->speed) * gPhysRollSteer) * r->side.y) *
                                              -0x1.6c16c2p+7f));
    }
    if (r->fieldB9C == 2) {
        double y;

        zmatrixRotateEuler(&m, &m, &r->rot);
        zmatrixMul(orient(r), orient(r), &m);
        zmatrixMul(&r->basis, &r->basis, &m);
        y = (double)r->rot.y;
        r->rot.y = (ZAngle)ftol(y - y * gPhysRollYawDamp);
        if (fpuBelow(__builtin_fabs((double)r->rot.y), 91.0f)) {
            r->rot.y = 0;
        }
    }
}

/* Rotates `v` by the transpose of m. */
static void transformRows(PhysRider *r, const ZMatrix *m)
{
    ZVec3 t;

    zmatrixTransformPoint(&t, m, &r->fwd);
    r->fwd = t;
    zmatrixTransformPoint(&t, m, &r->up);
    r->up = t;
    zmatrixTransformPoint(&t, m, &r->side);
    r->side = t;
}

/* Applies `angles` in the frame of `frame`: into the frame, rotate, back out. */
static void rotateInFrame(PhysRider *r, const ZMatrix *frame, const ZAngle3 *angles)
{
    ZMatrix t;

    zmatrixCopyRotation(&t, frame);
    zmatrixTranspose(&t, &t);
    transformRows(r, &t);
    zmatrixRotateEuler(orient(r), orient(r), angles);
    zmatrixTranspose(&t, &t);
    transformRows(r, &t);
}

/* Air rotation: the trick spin, the timed flip/spin about orientA and the continuous one about orientB. */
BMX_ADDR(0x00424a10)
void physOrientAir(PhysRider *r)
{
    ZMatrix m;

    zmatrixIdentity(&m);
    zmatrixRotateEuler(&m, &m, &r->rot);
    zmatrixMul(orient(r), orient(r), &m);
    if (r->timer != 0) {
        if (!(r->flags & kPhysAirFlip)) {
            rotateInFrame(r, &r->orientA, &r->orientARot);
        } else {
            zmatrixIdentity(&m);
            zmatrixRotateEuler(&m, &m, &r->orientARot);
            zmatrixMul(orient(r), orient(r), &m);
        }
        r->timer--;
    }
    if (r->flags & kPhysAirSpin) {
        rotateInFrame(r, &r->orientB, &r->orientBRot);
    }
    zvec3NormalizeScaled(&r->up, &r->up);
    zvec3Cross3(&r->side, &r->fwd, &gZmathOrigin, &r->up);
    zvec3NormalizeScaled(&r->side, &r->side);
    zvec3Cross3(&r->fwd, &r->up, &gZmathOrigin, &r->side);
    zmatrixCopyRotation(&r->basis, orient(r));
}

static int isTailTrick(const PhysRider *r)
{
    return r->anim >= 0x27 && r->anim <= 0x29;
}

/* Balance push of grinds and stalls: ((1 - f6e4) * 10 + 1) * balance. */
static double balanceGain(const PhysRider *r)
{
    return ((1.0f - (double)r->stat[5]) * 10.0f + 1.0f) * gPhysGrindBalance;
}

static ZAngle dampYaw(ZAngle a)
{
    const double v = (double)a;

    return (ZAngle)ftol(v - v * gPhysGrindYawDamp);
}

/*
 * Grind and stall balance. On a rail (surface 9) the rider wobbles about the rail and is thrown off
 * (state 1) once he leans too far; on lips (10-12) he tips over the edge.
 */
BMX_ADDR(0x00424ea0)
void physOrientGrind(PhysRider *r)
{
    ZMatrix m;

    zmatrixIdentity(&m);
    switch (r->surface) {
    case 9: {
        ZVec3 v;
        const ZVec3 *axis;
        double d;
        ZAngle a;

        if (fpuBelow(r->speed, 40.0f)) {
            double s = isTailTrick(r) ? -(double)r->fwd.y : (double)r->side.y;

            if (fpuBelow(__builtin_fabs(s), 0.1f)) {
                s = fpuBelow(s, 0.0f) ? -0.1f : 0.1f;
            }
            r->rot.x = (ZAngle)(r->rot.x + (int16_t)ftol(((balanceGain(r) * s) * (40.0f - (double)r->speed)) *
                                                         0x1.6c16c2p+7f));
        }
        if (isTailTrick(r)) {
            zmatrixRotateZ(&m, &m, r->rot.x);
        } else {
            zmatrixRotateX(&m, &m, r->rot.x);
        }
        zmatrixMul(orient(r), orient(r), &m);
        zmatrixMul(&r->basis, &r->basis, &m);
        r->rot.x = dampYaw(r->rot.x);
        v = r->up;
        axis = isTailTrick(r) ? &r->contactAxis2 : &r->contactAxis0;
        d = zvec3Dot(&v, axis);
        v.x = (float)(v.x - d * axis->x);
        v.y = (float)(v.y - d * axis->y);
        v.z = (float)(v.z - d * axis->z);
        zvec3NormalizeScaled(&v, &v);
        a = zmathAcos((float)zvec3Dot(&v, &r->contactAxis1));
        if (!fpuBelow((double)gPhysRailAngleLimit, __builtin_fabs((double)a))) {
            return;
        }
        zmatrixCopyRotation(&m, contactAxes(r));
        if (isTailTrick(r)) {
            m.right.y = 0.0f;
            zvec3NormalizeScaled(&m.right, &m.right);
            zvec3Cross3(&m.up, &m.at, &gZmathOrigin, &m.right);
            if (fpuBelow(zvec3Dot(&m.right, &r->up), 0.0f)) {
                zvec3Negate(&m.right, &m.right);
            }
            if (fpuBelow(zvec3Dot(&r->fwd, &m.up), 0.0f)) {
                r->vel.x = (float)(m.up.x * 15.0 + r->vel.x);
                r->vel.y = (float)(m.up.y * 15.0 + r->vel.y);
                r->vel.z = (float)(m.up.z * 15.0 + r->vel.z);
                physSetState(r, kPhysStateAir);
                return;
            }
            r->vel.x = (float)((double)m.right.x * 9.5f + (double)m.up.x * 17.0f);
            r->vel.y = (float)((double)m.right.y * 9.5f + (double)m.up.y * 17.0f);
            r->vel.z = (float)((double)m.right.z * 9.5f + (double)m.up.z * 17.0f);
            physSetState(r, kPhysStateAir);
            return;
        }
        m.at.y = 0.0f;
        zvec3NormalizeScaled(&m.at, &m.at);
        zvec3Cross3(&m.up, &m.at, &gZmathOrigin, &m.right);
        if (fpuBelow(zvec3Dot(&m.at, &r->up), 0.0f)) {
            zvec3Negate(&m.at, &m.at);
        }
        r->vel.x = (float)(((double)m.up.x * 7.5f + (double)m.at.x * 10.0f) + r->vel.x);
        r->vel.y = (float)(((double)m.up.y * 7.5f + (double)m.at.y * 10.0f) + r->vel.y);
        r->vel.z = (float)(((double)m.up.z * 7.5f + (double)m.at.z * 10.0f) + r->vel.z);
        physSetState(r, kPhysStateAir);
        return;
    }
    case 10:
        zmatrixRotateY(&m, &m, (ZAngle)-r->rot.x);
        r->rot.x = dampYaw(r->rot.x);
        zmatrixMul(orient(r), orient(r), &m);
        zmatrixMul(&r->basis, &r->basis, &m);
        return;
    case 11:
    case 12:
        r->rot.x =
            (ZAngle)(r->rot.x + (int16_t)ftol(((balanceGain(r) * r->side.y) * (30.0f - (double)r->speed)) *
                                              0x1.6c16c2p+7f));
        zmatrixRotateX(&m, &m, r->rot.x);
        zmatrixMul(orient(r), orient(r), &m);
        zmatrixMul(&r->basis, &r->basis, &m);
        r->rot.x = dampYaw(r->rot.x);
        if (zmathAsin(-r->up.y) < 0xaaa) {
            physSetState(r, kPhysStateAir);
        }
        return;
    default:
        return;
    }
}

/* ---- state transitions --------------------------------------------------------------------------- */

/*
 * Enter handler of the grind state. Coming from the ground onto a lip or rail face (surfaces 1-3) it
 * checks the rider is close enough, picks the grind frame, and switches the contact to the edge
 * (surface 9); slow grinds become stalls (state 6). Returns 0 when the grind is refused.
 */
BMX_ADDR(0x00422830)
int physEnterGrind(PhysRider *r, int from, int to)
{
    const PhysEdge *contact;
    uint32_t kind;
    const uint8_t *prop;

    if (from == kPhysStateRoll && r->surface != 0 && r->surface <= 3) {
        const PhysEdge *e = r->contact;
        ZVec3 p, n;
        float plane[4];
        int32_t dx, dy, dz, d2;
        uint32_t trick;
        ZMatrix m;

        physClosestPointOnEdge(&r->pos, e, &p);
        dx = ftol((double)r->center.x - p.x);
        dy = ftol((double)r->center.y - p.y);
        dz = ftol((double)r->center.z - p.z);
        d2 = (int32_t)((uint32_t)dz * (uint32_t)dz + (uint32_t)dy * (uint32_t)dy +
                       (uint32_t)dx * (uint32_t)dx);
        if (!((dy < 0 && d2 < 250000) || d2 < 10000) || e->kind == 0) {
            return 0;
        }
        r->contactPos = p;
        n = e->normal;
        plane[0] = e->normal.x;
        plane[1] = e->normal.y;
        plane[2] = e->normal.z;
        plane[3] = e->d;
        if (fpuBelow(physPlaneDistance(&r->pos, plane), 0.0f)) {
            zvec3Negate(&n, &n);
        }
        if (fpuBelow(__builtin_fabs(zvec3Dot(&e->dir, &r->vel)), gPhysGrindMinSpeed)) {
            to = kPhysStateStall;
            trick = riderGetTrickB(r);
        } else {
            trick = riderGetTrickA(r);
        }
        if (trick >= 0x27 && trick <= 0x29) {
            /* Tail tricks: the bike points across the edge. */
            m.at = e->dir;
            if (fpuBelow(zvec3Dot(&r->side, &m.at), 0.0f)) {
                zvec3Negate(&m.at, &m.at);
            }
            zvec3Cross3(&m.right, &gPhysDown, &gZmathOrigin, &m.at);
            zvec3NormalizeScaled(&m.right, &m.right);
            zvec3Cross3(&m.up, &m.at, &gZmathOrigin, &m.right);
        } else {
            m.right = e->dir;
            m.at = n;
            zvec3Cross3(&m.up, &m.at, &gZmathOrigin, &m.right);
            if (m.up.y > 0.0f) {
                zvec3Negate(&m.right, &m.right);
                zvec3Negate(&m.up, &m.up);
            }
            if (fpuBelow(zvec3Dot(&r->fwd, &m.right), 0.0f)) {
                zvec3Negate(&m.right, &m.right);
                zvec3Negate(&m.at, &m.at);
            }
            if (r->surface != 2) {
                ZMatrix tilt;

                zmatrixIdentity(&tilt);
                zmatrixRotateX(&tilt, &tilt, fpuBelow(zvec3Dot(&n, &m.at), 0.0f) ? -0xe38 : 0xe38);
                zmatrixMul(&m, &m, &tilt);
            }
        }
        zvec3Zero(&m.pos);
        zmatrixCopyRotation(orient(r), &m);
        if (fpuBelow(zvec3Dot(&m.at, &n), 0.0f)) {
            r->flags |= kPhysGrindReverse;
        } else {
            r->flags &= ~(uint32_t)kPhysGrindReverse;
        }
        r->contact = (void *)e;
        r->surface = 9;
        physContactCopy(r);
        zmatrixCopyRotation(contactAxes(r), &m);
    }
    contact = r->contact;
    r->rot.x = 0;
    r->rot.y = 0;
    r->rot.z = 0;
    if (to == kPhysStateGrind) {
        physSnapToEdge(r, contact);
    } else {
        zvec3Zero(&r->vel);
        zvec3Zero(&r->dvel);
        r->speed = 0.0f;
        r->pos = r->contactPos;
        r->center = r->contactPos;
    }
    kind = edgeSoundKind(r->contact);
    prop = contact->prop;
    if (prop != NULL && physObjectProp(prop) != NULL) {
        propTouch(r->player, physObjectProp(prop));
    }
    r->groundGap = 0.0f;
    bmxSoundPlayAt(0xbba, &r->pos, (int)((uint32_t)r->player << 28 | kind | 0x200), 0x7f, 0);
    return to;
}

/* Leaving a grind or stall onto a lip face: lean the frame into the lip over 6 frames. */
static void leanIntoLip(PhysRider *r)
{
    int32_t a;

    zmatrixCopyRotation(&r->orientA, orient(r));
    a = zmathAcos(-r->up.y) / 6;
    r->orientARot.y = (ZAngle)a;
    if (r->side.y > 0.0f) {
        r->orientARot.y = (ZAngle)-a;
    }
    r->timer = 6;
}

/* Moves the rider out along its up axis by the front wheel radius. */
static void liftByWheel(PhysRider *r)
{
    ZVec3 d;

    d.x = (float)((double)r->up.x * r->wheel[0].radius);
    d.y = (float)((double)r->up.y * r->wheel[0].radius);
    d.z = (float)((double)r->up.z * r->wheel[0].radius);
    zvec3Add(&r->pos, &r->pos, &d);
    zvec3Add(&r->center, &r->center, &d);
}

/*
 * Launch off a quarter-pipe lip (surfaces 1, 3) while rolling up it: aligns to the pipe and, when the
 * flight is long enough, plans a flip about the lip (kPhysAirFlip) that lands facing back down.
 */
static void launchFromPipe(PhysRider *r, const ZVec3 *n)
{
    uint32_t frames;
    ZMatrix f;
    ZVec3 t;
    float d1;
    double d2;
    int32_t q;

    physAlignToQuarterPipe(r, r->contact, 1);
    if (r->surface == 3) {
        r->vel.x = 0.0f;
        r->vel.z = 0.0f;
    }
    if (zmathAcos((float)zvec3Dot(&r->up, n)) < gPhysPipeAngleLimit &&
        !fpuBelow(zvec3Dot(&r->vel, &r->fwd), 0.0f)) {
        frames = (uint32_t)bmxFtol((double)r->vel.y * -2.0f / gPhysGravity);
        if (frames > 15) {
            frames -= 7;
            zmatrixCopyRotation(&r->orientA, orient(r));
            r->orientARot.x = 0;
            r->orientARot.z = 0;
            zmatrixCopyRotation(&f, orient(r));
            zvec3Cross3(&f.right, &gPhysDown, &gZmathOrigin, &f.up);
            zvec3NormalizeScaled(&f.right, &f.right);
            zvec3Cross3(&f.at, &f.right, &gZmathOrigin, &f.up);
            d1 = (float)zvec3Dot(&r->fwd, &f.right);
            d2 = zvec3Dot(&r->fwd, &f.at);
            t.x = (float)(-((double)f.at.x * d2) + (double)f.right.x * d1);
            t.y = (float)(-((double)f.at.y * d2) + (double)f.right.y * d1);
            t.z = (float)(-((double)f.at.z * d2) + (double)f.right.z * d1);
            zvec3NormalizeScaled(&t, &t);
            q = zmathAcos((float)zvec3Dot(&t, &r->fwd)) / (int32_t)frames;
            r->orientARot.y = (ZAngle)q;
            if (r->side.y > 0.0f) {
                r->orientARot.y = (ZAngle)-q;
            }
            r->timer = (int16_t)frames;
            r->flags |= kPhysAirFlip;
        }
    }
    r->flags |= kPhysQuarterPipe;
}

/*
 * Launch off a lip face (surface 2): lifts the rider to just above the lip, pushes it out to 120 units
 * from the edge and aims a jump straight back down the face, with a half turn over the flight.
 */
static void launchFromLip(PhysRider *r, const ZVec3 *n, float dist, double lipY, ZVec3 *down)
{
    uint32_t frames;
    double k, f;
    ZVec3 t;
    int32_t q;

    if (lipY < r->pos.y) {
        r->pos.y = (float)lipY;
    }
    down->y = -3.0f;
    k = 120.0f - (double)dist;
    r->pos.x = (float)(k * n->x + r->pos.x);
    r->pos.z = (float)(k * n->z + r->pos.z);
    r->center.z = r->pos.z;
    down->x = -n->x;
    down->z = -n->z;
    r->center.x = r->pos.x;
    r->center.y = r->pos.y;
    zvec3NormalizeScaled(down, down);
    frames = (uint32_t)bmxFtol((double)gPhysPipeLaunchScale * r->vel.y * -2.0f / gPhysGravity);
    if (frames < 15) {
        frames = 15;
    }
    k = (double)frames;
    r->vel.y = (float)-(k * gPhysGravity * 0.5f);
    f = ((double)dist + 150.0f) / k;
    r->vel.x = (float)-((double)n->x * f);
    r->vel.z = (float)-((double)n->z * f);
    r->speed = (float)zvec3Length(&r->vel);
    t.x = r->fwd.x;
    t.y = -r->fwd.y;
    t.z = r->fwd.z;
    r->orientARot.y = 0;
    r->orientARot.x = 0;
    zmatrixCopyRotation(&r->orientA, orient(r));
    q = zmathAcos((float)zvec3Dot(&t, &r->fwd)) / (int32_t)(frames - 3);
    r->orientARot.z = (ZAngle)q;
    if (!fpuBelow(zvec3Dot(&r->vel, &r->fwd), 0.0f)) {
        r->orientARot.z = (ZAngle)-r->orientARot.z;
    }
    r->timer = (int16_t)(frames - 3);
}

/*
 * Leaving a quarter-pipe lip with kPhysNoPipeAlign: keeps the rider moving away from the lip plane at
 * 10+ units/frame and falling at 25+, and holds it at least 120 units in front of the edge.
 */
static void driftOffPipe(PhysRider *r, const PhysEdge *e, const ZVec3 *n)
{
    const double d = -zvec3Dot(&r->vel, n);
    ZVec3 p;

    if (fpuBelow(d, 10.0f)) {
        const double k = 10.0f - d;

        r->vel.x = (float)((double)r->vel.x - k * n->x);
        r->vel.y = (float)((double)r->vel.y - k * n->y);
        r->vel.z = (float)((double)r->vel.z - k * n->z);
    }
    r->vel.y = fpuBelow(r->vel.y, -25.0f) ? r->vel.y : -25.0f;
    r->speed = (float)zvec3Length(&r->vel);
    physClosestPointOnEdge(&r->pos, e, &p);
    p.x = (float)((double)n->x * 120.0f + p.x);
    p.y = (float)((double)n->y * 120.0f + p.y);
    p.z = (float)((double)n->z * 120.0f + p.z);
    if (r->pos.y > p.y) {
        r->pos = p;
        r->center = p;
    }
}

/*
 * Enter handler of the air state (bmxphysStateInitAir). Snapshots the contact, measures the drop below
 * the hips, applies the pending step and sets up the flight: lip and quarter-pipe launches, a capped
 * take-off speed, or a hop off a grind or stall. Leaving the ground over a lip or rail with
 * kPhysGrindRequest tries the grind state first and returns its state when it sticks.
 */
BMX_ADDR(0x00422e50)
int physEnterAir(PhysRider *r, int from, int to)
{
    PhysRayHit hit;
    ZVec3 hip, below;
    ZMatrix m; /* m.up starts as the down vector */
    ZMatrix rot;
    uint32_t kind = 0xffffffffu; /* surface kind of the face under the hips */
    void *target = NULL;

    r->snapAPos = r->pos;
    r->snapA.surface = r->surface;
    r->snapA.contact = r->contact;
    physContactSnapshotFix(&r->snapA);
    r->snapBPos = r->pos;
    r->snapB.contact = r->contact;
    r->snapB.surface = r->surface;
    physContactSnapshotFix(&r->snapB);
    r->flags &= ~(uint32_t)kPhysQuarterPipe;
    hip.x = (float)((double)r->up.x * 100.0f + r->pos.x);
    hip.y = (float)((double)r->up.y * 100.0f + r->pos.y);
    hip.z = (float)((double)r->up.z * 100.0f + r->pos.z);
    below.x = hip.x;
    below.y = (float)((double)hip.y + 10000.0f);
    below.z = hip.z;
    if (!zcollideRay(&hip, &below, &hit)) {
        gZdebugPrint("bmxphysStateInitAir: no valid terrain found under rider's hips (pos X: %d, Z: %d)\n",
                     bmxFtol(hip.x), bmxFtol(hip.z));
    } else {
        r->groundGap = (float)((double)hit.point.y - hip.y);
        if ((hit.flags & 1) && hit.face != NULL) {
            const PhysFace *face = hit.face;

            kind = (uint32_t)face->tag >> 12;
            if (kind - 1 > 7) {
                kind = 0;
                target = (void *)face;
            } else if (kind <= 3) {
                /* quarter-pipe faces resolve to their lip edge */
                target = physFindNearestTaggedEdge(hit.id, face->tag & 0xfff, &r->pos);
                kind = (uint32_t)face->tag >> 12;
                if (target == NULL) {
                    gZdebugPrint("bmxphysStateInitAir: quarterpipe face with no assigned edge, ID=%d\n",
                                 face->tag & 0xfff);
                    target = (void *)face;
                    kind = 0;
                }
            } else {
                target = (void *)face;
            }
        } else if ((hit.flags & 2) && hit.objectData != NULL) {
            r->surface = 12;
            kind = (kind & ~0xffu) | 12; /* only the low byte is written */
            target = hit.objectData;
            r->contact = target;
            physContactCopy(r);
        }
    }
    r->pos.x = (float)((double)r->step.x + r->pos.x);
    r->pos.y = (float)((double)r->step.y + r->pos.y);
    r->pos.z = (float)((double)r->step.z + r->pos.z);
    zvec3Zero(&r->step);
    r->rot.x = 0;
    r->rot.y = 0;
    r->rot.z = 0;
    r->yawTrim = 0;
    r->orientARot.x = 0;
    r->orientARot.y = 0;
    r->orientARot.z = 0;
    r->timer = 0;
    m.up = gPhysDown;
    r->flags &= ~(uint32_t)kPhysAirFlip;
    if (from == kPhysStateRoll && fpuBelow(zvec3Dot(&r->vel, &r->fwd), 0.0f)) {
        r->flags |= kPhysAirBackwards;
    } else {
        r->flags &= ~(uint32_t)kPhysAirBackwards;
    }

    switch (from) {
    case kPhysStateRoll:
        if (r->surface >= 1 && r->surface <= 3) {
            const PhysEdge *e = r->contact;
            ZVec3 n = e->normal;
            float plane[4];
            float dist, ax, ay;
            double d;
            int launch = 0;

            plane[0] = e->normal.x;
            plane[1] = e->normal.y;
            plane[2] = e->normal.z;
            plane[3] = e->d;
            d = physPlaneDistance(&r->pos, plane);
            dist = (float)d;
            if (fpuBelow(d, 0.0f)) {
                zvec3Negate(&n, &n);
                dist = -dist;
            }
            ax = (float)e->a[0];
            ay = (float)e->a[1];
            if (fpuBelow(r->vel.y, 0.0f) && zmathAsin(__builtin_fabsf(r->side.y)) < gPhysPipeTiltLimit) {
                launch = ((r->surface == 1 || r->surface == 3) &&
                          zmathAcos((float)zvec3Dot(&r->up, &n)) < gPhysPipeTiltLimit) ||
                         (r->surface == 2 && fpuBelow((double)r->pos.y - ((double)ay - 50.0f), 250.0f) &&
                          fpuBelow(dist, 300.0f));
            }
            if (!launch) {
                r->flags |= kPhysNoPipeAlign;
                goto align;
            }
            if (r->flags & kPhysGrindRequest) {
                const uint8_t s = physSetState(r, kPhysStateGrind);

                if (s == kPhysStateGrind || s == kPhysStateStall) {
                    return physGetState(r);
                }
            }
            if (r->surface == 2) {
                launchFromLip(r, &n, dist, (double)ay + 50.0f, &m.up);
                break;
            }
            if (r->flags & kPhysNoPipeAlign) {
                ZVec3 lip, out;

                lip.x = (float)((double)n.x * 50.0f + ax);
                lip.y = (float)((double)ay - 50.0f);
                lip.z = (float)((double)n.z * 50.0f + (double)e->a[2]);
                out.x = (float)((double)n.x * -100.0f + lip.x);
                out.y = lip.y;
                out.z = (float)((double)n.z * -100.0f + lip.z);
                if (zcollideRay(&lip, &out, &hit)) {
                    r->flags &= ~(uint32_t)kPhysNoPipeAlign;
                } else {
                    physClosestPointOnEdge(&r->pos, e, &lip);
                    if (!fpuBelow(((double)r->pos.y + r->vel.y) - lip.y, 150.0f)) {
                        r->flags &= ~(uint32_t)kPhysNoPipeAlign;
                    }
                }
                if (r->flags & kPhysNoPipeAlign) {
                    driftOffPipe(r, e, &n);
                    goto align;
                }
            }
            launchFromPipe(r, &n);
            break;
        }
        if (kind - 1 < 3) {
            if (r->flags & kPhysGrindRequest) {
                uint8_t s;

                r->contact = target;
                r->surface = (uint8_t)kind;
                physContactCopy(r);
                s = physSetState(r, kPhysStateGrind);
                if (s == kPhysStateGrind || s == kPhysStateStall) {
                    return physGetState(r);
                }
            }
        }
        {
            /* Cap the take-off speed: the steeper the ground, the faster the rider may leave it. */
            float cap;
            int32_t tilt;

            switch (r->surface) {
            case 4:
                cap = 11.0f;
                break;
            case 5:
                cap = 15.0f;
                break;
            case 6:
                cap = 21.0f;
                break;
            case 7:
                cap = 23.0f;
                break;
            case 8:
                cap = 35.0f;
                break;
            default:
                cap = gPhysAirSpeedCap;
                break;
            }
            tilt = (int32_t)zmathAcos(-r->up.y) - 0x2000;
            if (tilt < 0) {
                tilt = -tilt;
            }
            if ((int16_t)tilt < 0x1555) {
                ZVec3 h;
                float limit;
                double len;

                h.x = r->vel.x;
                h.y = 0.0f;
                h.z = r->vel.z;
                limit = (float)((zmathSin((ZAngle)tilt) + 1.0f) * cap);
                len = zvec3Length(&h);
                if (len > limit) {
                    const double k = limit / len;

                    r->vel.x = (float)(k * h.x);
                    r->vel.z = (float)(k * h.z);
                }
            }
        }
        if (kind - 1 < 3) {
            break;
        }
    align:
        /* Ease the frame upright over gPhysAirAlignFrames frames. */
        zvec3Cross3(&m.at, &r->fwd, &gZmathOrigin, &m.up);
        zvec3NormalizeScaled(&m.at, &m.at);
        zvec3Cross3(&m.right, &m.up, &gZmathOrigin, &m.at);
        zmatrixCopyRotation(&r->orientA, orient(r));
        physRelativeAngles(orient(r), &m, gPhysAirAlignFrames, &r->orientARot);
        r->timer = (int16_t)gPhysAirAlignFrames;
        break;
    case kPhysStateGrind:
        liftByWheel(r);
        if (kind - 1 < 3) {
            leanIntoLip(r);
        }
        break;
    case kPhysStateStall:
        liftByWheel(r);
        if (r->surface != 11 && r->surface != 12) {
            r->vel.x = (float)((double)r->fwd.x * 5.0f + r->vel.x);
            r->vel.y = (float)((double)r->fwd.y * 5.0f + r->vel.y);
            r->vel.z = (float)((double)r->fwd.z * 5.0f + r->vel.z);
            if (kind - 1 < 3 && !(r->anim >= 0x27 && r->anim <= 0x29)) {
                leanIntoLip(r);
            }
            break;
        }
        /* Hop off the top of a lip or prop: forwards and up, turned round and tilted back. */
        r->vel.x = (float)((double)r->fwd.x * 5.0f + (double)r->up.x * 15.0f);
        r->vel.y = (float)((double)r->fwd.y * 5.0f + (double)r->up.y * 15.0f);
        r->vel.z = (float)((double)r->fwd.z * 5.0f + (double)r->up.z * 15.0f);
        zvec3Cross3(&m.at, &r->fwd, &gZmathOrigin, &m.up);
        zvec3NormalizeScaled(&m.at, &m.at);
        zvec3Cross3(&m.right, &m.up, &gZmathOrigin, &m.at);
        zmatrixIdentity(&rot);
        zmatrixRotateY(&rot, &rot, fpuBelow(r->side.y, 0.0f) ? 0x2aaa : -0x2aaa);
        zmatrixMul(&m, &m, &rot);
        zmatrixIdentity(&rot);
        zmatrixRotateZ(&rot, &rot, -0x238e);
        zmatrixMul(&m, &m, &rot);
        zmatrixCopyRotation(&r->orientA, orient(r));
        physRelativeAngles(orient(r), &m, gPhysAirAlignFrames, &r->orientARot);
        r->timer = (int16_t)gPhysAirAlignFrames;
        break;
    default:
        break;
    }

    if (from == kPhysStateRoll) {
        if (r->surface >= 1 && r->surface <= 3 && zmathAcos(__builtin_fabsf(r->up.y)) > 0x2aaa &&
            zmathAcos(__builtin_fabsf(r->fwd.y)) < 0x2000) {
            bmxSoundPlayAt(0xbb9, &r->pos, (int)((uint32_t)r->player << 28 | 0x102), 0x7f, 0);
        }
    } else if ((from == kPhysStateGrind || from == kPhysStateStall) && r->surface == 9 &&
               ((const PhysEdge *)r->contact)->kind == 1) {
        bmxSoundPlayAt(0xbb9, &r->pos, (int)((uint32_t)r->player << 28 | 0x207), 0x7f, 0);
    }
    return to;
}

/*
 * Enter handler of the stall state: snaps the rider to the contact frame and position, measures the
 * drop below a lip (surfaces 11, 12) and plays the landing sound for the trick.
 */
BMX_ADDR(0x004243b0)
int physEnterStall(PhysRider *r, int from, int to)
{
    uint32_t kind = 0;
    uint32_t sound;
    void *prop = NULL;

    switch (r->surface) {
    case 9: {
        const PhysEdge *e = r->contact;
        ZVec3 dir = e->dir; /* unused, as in the original */

        if (fpuBelow(zvec3Dot(&r->fwd, &dir), 0.0f)) {
            zvec3Negate(&dir, &dir);
        }
        if (e->prop != NULL) {
            prop = physObjectProp(e->prop);
        }
        break;
    }
    case 12:
        prop = physObjectProp(r->contact);
        break;
    default:
        break;
    }
    if (prop != NULL) {
        propTouch(r->player, prop);
    }
    zmatrixCopyRotation(orient(r), contactAxes(r));
    r->rot.x = 0;
    r->rot.y = 0;
    r->rot.z = 0;
    if (r->surface >= 11 && r->surface <= 12) {
        PhysRayHit hit;
        ZVec3 from, below;
        const double x = (double)r->contactAxis1.x * 100.0f + r->contactPos.x;

        from.x = (float)x;
        from.y = (float)((double)r->contactAxis1.y * 100.0f + r->contactPos.y);
        from.z = (float)((double)r->contactAxis1.z * 100.0f + r->contactPos.z);
        below.x = (float)x;
        below.z = from.z;
        below.y = (float)((double)from.y + 10000.0f);
        if (zcollideRay(&from, &below, &hit)) {
            r->groundGap = (float)((double)hit.point.y - from.y);
        }
    } else {
        r->groundGap = 0.0f;
    }
    r->pos = r->contactPos;
    r->center = r->contactPos;
    if (r->surface == 9) {
        kind = edgeSoundKind(r->contact);
    }
    switch (r->anim) {
    case 0x25:
    case 0x26:
        sound = 3;
        break;
    case 0x27:
    case 0x28:
    case 0x2b:
        sound = 5;
        break;
    case 0x2a:
        sound = 4;
        break;
    default:
        sound = 2;
        break;
    }
    bmxSoundPlayAt(0xbba, &r->pos, (int)((((uint32_t)r->player << 20 | sound) << 8) | kind), 0x7f, 0);
    zvec3Zero(&r->vel);
    zvec3Zero(&r->dvel);
    r->speed = 0.0f;
    return to;
}

/* Enter handler of the scripted state: hands position, velocity and rotation to the motion objects. */
BMX_ADDR(0x00424740)
int physEnterScripted(PhysRider *r, int from, int to)
{
    ZAngle3 zero;

    r->vel.y = (float)((double)r->vel.y - 5.0f);
    zoverStartRiderMotion(r->actor2, r->actor, orient(r), &r->vel, &r->rot);
    zero.x = 0;
    zero.y = 0;
    zero.z = 0;
    zoverStartBikeMotion(r->bikeObj2, r->bikeObj, &r->basis, &r->vel, &zero);
    r->groundGap = 0.0f;
    r->field58 = 0;
    return to;
}

/* ---- movement ------------------------------------------------------------------------------------ */

/* Wheels keep 99% of their spin (rescaled and truncated to 16 bits, as in the original). */
static void decaySpin(PhysWheel *w)
{
    w->spin = (int16_t)ftol(((double)w->spin * 0.99f) * 65536.0f);
}

/* Drifts towards the grind edge (or lip) for `timer` frames, then enters the grind (or stall). */
BMX_ADDR(0x0042afd0)
void physMoveGrindIn(PhysRider *r)
{
    for (int w = 0; w < 2; w++) {
        decaySpin(&r->wheel[w]);
    }
    if (--r->timer == 0) {
        const int s = physGetState(r);

        if (s == kPhysStateGrindIn) {
            physSetState(r, kPhysStateGrind);
            return;
        }
        if (s == kPhysStateStallIn) {
            physSetState(r, kPhysStateStall);
        }
        return;
    }
    zvec3Add(&r->center, &r->center, &r->airVel);
    zvec3Add(&r->pos, &r->pos, &r->airVel);
}

/*
 * Grind movement. While the rider is between the edge's end points he aligns with it; past an end he
 * hops onto the next edge in reach (if it is roughly parallel) or pops off into the air.
 */
BMX_ADDR(0x0042b0a0)
void physMoveGrind(PhysRider *r)
{
    const PhysEdge *e = r->contact;
    int32_t a0, a1, a2, b0, b1, b2;
    ZVec3 v;

    for (unsigned w = 0; w < 2; w++) {
        if (!(w & 1)) {
            decaySpin(&r->wheel[w]);
        } else {
            r->wheel[w].spin = 0;
        }
    }
    a0 = ftol((double)e->a[0] - r->pos.x);
    a1 = ftol((double)e->a[1] - r->pos.y);
    a2 = ftol((double)e->a[2] - r->pos.z);
    b0 = ftol((double)e->b[0] - r->pos.x);
    b1 = ftol((double)e->b[1] - r->pos.y);
    b2 = ftol((double)e->b[2] - r->pos.z);
    if ((int32_t)((uint32_t)b2 * (uint32_t)a2 + (uint32_t)b1 * (uint32_t)a1 + (uint32_t)b0 * (uint32_t)a0) <=
        0) {
        v = e->dir;
        if (fpuBelow(zvec3Dot(&r->fwd, &v), 0.0f)) {
            zvec3Negate(&v, &v);
        }
        r->fwd.x = (float)(((double)v.x - r->fwd.x) * gPhysAlignRate + r->fwd.x);
        r->fwd.y = (float)(((double)v.y - r->fwd.y) * gPhysAlignRate + r->fwd.y);
        r->fwd.z = (float)(((double)v.z - r->fwd.z) * gPhysAlignRate + r->fwd.z);
        zvec3Cross3(&r->side, &r->fwd, &gZmathOrigin, &r->up);
        zvec3NormalizeScaled(&r->side, &r->side);
        zvec3Cross3(&r->up, &r->side, &gZmathOrigin, &r->fwd);
    } else {
        const PhysEdge *next = physFindNearestEdge(r, &r->pos, gPhysEdgeSearchRadius, &v);

        if (next != NULL && zmathAcos((float)__builtin_fabs(
                                zvec3Dot(&next->dir, &((const PhysEdge *)r->contact)->dir))) < 0x3555) {
            physSnapToEdge(r, next);
        } else {
            v = r->up;
            zvec3Scale(&v, &v, 7.5f);
            zvec3Add(&r->vel, &r->vel, &v);
            physSetState(r, kPhysStateAir);
        }
    }
    zvec3Add(&r->pos, &r->pos, &r->vel);
    zvec3Add(&r->center, &r->center, &r->vel);
}

/*
 * Nearest edge within `radius` of p other than the one being ground, as a copy in a static record;
 * `point` receives its closest point.
 */
BMX_ADDR(0x0042b3f0)
PhysEdge *physFindNearestEdge(PhysRider *r, const ZVec3 *p, float radius, ZVec3 *point)
{
    void **list = zbspFindEdgesNear(p, radius);
    const PhysEdge *best = NULL;
    uint32_t bestDist;
    const PhysEdge *e;

    if (list == NULL) {
        return NULL;
    }
    e = *list++;
    bestDist = 0xffffffffu;
    if (e == NULL) {
        return NULL;
    }
    do {
        if (physGetState(r) != kPhysStateGrind || r->surface != 9 || !physEdgeEqual(e, r->contact)) {
            ZVec3 c;
            int32_t dx, dy, dz;
            uint32_t d;

            physClosestPointOnEdge(p, e, &c);
            dx = ftol((double)c.x - p->x);
            dy = ftol((double)c.y - p->y);
            dz = ftol((double)c.z - p->z);
            d = (uint32_t)dz * (uint32_t)dz + (uint32_t)dy * (uint32_t)dy + (uint32_t)dx * (uint32_t)dx;
            if (d < bestDist) {
                *point = c;
                bestDist = d;
                best = e;
            }
        }
        e = *list++;
    } while (e != NULL);
    if (best == NULL) {
        return NULL;
    }
    zmemCopy(&gPhysNearEdge, best, sizeof(PhysEdge));
    return &gPhysNearEdge;
}

/* Clamps a volume to 0..127. */
static int32_t clampVolume(int32_t v)
{
    if (v < 0x7f) {
        return v & -(int32_t)(v >= 0);
    }
    return 0x7f;
}

/* Rolling-wheel volume from the rear wheel's spin. */
static int32_t wheelVolume(const PhysRider *r)
{
    return clampVolume(ftol((double)r->wheel[1].spin * 0x1.a75554p+8f));
}

static int32_t speedPitch(const PhysRider *r)
{
    const int32_t p = ftol((double)r->speed * 3.0f);

    return p < 0x7f ? p : 0x7f;
}

/* Scripted state: position and frame follow the motion objects; surface 13 freezes the rider. */
BMX_ADDR(0x0042b5e0)
void physMoveScripted(PhysRider *r)
{
    const uint8_t *obj;

    for (int w = 0; w < 2; w++) {
        decaySpin(&r->wheel[w]);
    }
    if (r->surface != 13) {
        ZMotionPlayer *player;
        uint32_t *dst;
        const uint32_t *src;

        zoverStepRiderMotion(r->actor2, r->actor);
        r->motionSample = zactorSampleScaled(riderActor(r), (const ZMotion *)gPhysMotions[r->motionIndex],
                                             r->motionFlags, &r->motionScale);
        player = riderActor(r)->player;
        dst = (uint32_t *)(void *)player->ch[0].joints;
        src = (const uint32_t *)(const void *)player->ch[1].joints;
        for (int i = 0; i < 30; i++) {
            dst[i] = src[i];
        }
        obj = r->actor2;
        zmatrixCopyRotation(orient(r), (const ZMatrix *)(const void *)(obj + 8));
        obj = r->actor2;
        r->pos = *(const ZVec3 *)(const void *)(obj + 0xe70);
        zvec3Zero(&r->step);
        obj = r->actor2;
        r->vel = *(const ZVec3 *)(const void *)(obj + 0x88);
        r->speed = (float)zvec3Length(&r->vel);
    }
    zoverStepBikeMotion(r->bikeObj2, r->bikeObj);
    obj = r->bikeObj2;
    r->center = *(const ZVec3 *)(const void *)(obj + 0xe70);
    zmatrixCopyRotation(&r->basis, (const ZMatrix *)(const void *)(obj + 8));
    bmxSoundPlayAt(4000, &r->center, (int)((uint32_t)r->player << 28 | 0x103), 0, wheelVolume(r));
}

/* Rolling sound: material under the first grounded wheel, variants for hops and manuals. */
BMX_ADDR(0x0042b830)
void physSyncRoll(PhysRider *r)
{
    uint32_t surface;
    uint32_t material = 0;
    uint32_t hop = 0;
    uint32_t manual = 0;
    int32_t volume;

    physPoseActorsRoll(r);
    for (unsigned w = 0; w < 2; w++) {
        if (r->wheel[w].mode != 2) {
            material = (uint32_t)r->wheel[w].material;
            break;
        }
    }
    switch (material) {
    case 1:
    case 3:
        surface = 7;
        break;
    case 2:
        surface = 2;
        break;
    case 4:
        surface = 5;
        break;
    case 5:
        surface = 4;
        break;
    case 6:
        surface = 6;
        break;
    case 10:
        surface = 1;
        break;
    default:
        surface = 0;
        break;
    }
    volume = wheelVolume(r);
    if (riderGetStateLevel(r, 0) == 0 && riderGetStateLevel(r, 1) == 2) {
        hop = 1;
    } else if (riderGetStateLevel(r, 0) == 0 && riderGetStateLevel(r, 1) == 1) {
        manual = 1;
    } else if (riderGetStateLevel(r, 0) == 0 && riderGetStateLevel(r, 1) == 4) {
        volume = 0x6e;
    }
    bmxSoundPlayAt(4000, &r->center,
                   (int)((((((uint32_t)r->player << 4 | hop) << 8) | manual) << 16) | surface | 0x100),
                   speedPitch(r), volume);
}

/* Grinding: actor pose, sparks in proportion to speed (not on kind-2 edges) and the grind sound. */
BMX_ADDR(0x0042c7e0)
void physSyncGrind(PhysRider *r)
{
    uint32_t kind = 0;
    ZVec3 t;

    r->wheel[1].spin = 0;
    if (r->anim == 0x2b) {
        r->wheel[0].spin = 0;
    }
    zactorFaceMatrix(riderActor(r), orient(r));
    r->step = r->fieldBFC;
    zmatrixTransformPoint(&t, orient(r), &r->step);
    r->step = t;
    physPoseActorsGrind(r);
    if (physGetState(r) == kPhysStateGrind) {
        const int8_t edgeKind = ((const PhysEdge *)r->contact)->kind;

        if (r->speed > 3.0f) {
            contrlerRumble(r->player, 0, 1);
        }
        if (edgeKind == 2) {
            kind = 2;
        } else {
            kind = edgeKind == 1 ? 7 : 0;
            for (uint32_t i = 0; i < (uint32_t)ftol((double)r->speed * 0.05f); i++) {
                ZVec3 spark;
                uint32_t seed;

                spark.x = (float)(zmathRandom() & 0xf);
                spark.y = 0.0f;
                spark.z = 0.0f;
                zvec3RotateY(&spark, &spark, (ZAngle)zmathRandom());
                zvec3Add(&spark, &spark, &r->pos);
                seed = zmathRandom();
                fxSpawn(r->actor, 0, &spark, seed);
            }
        }
    }
    bmxSoundPlayAt(4000, &r->center, (int)((uint32_t)r->player << 28 | kind | 0x200), speedPitch(r),
                   wheelVolume(r));
}

/* ---- edges, actors and frames --------------------------------------------------------------------- */

/*
 * Puts the rider on edge e: position on the edge, velocity projected onto it. When grinding and not
 * yet lined up, the contact frame is rebuilt from the edge direction.
 */
BMX_ADDR(0x0042ca90)
void physSnapToEdge(PhysRider *r, const PhysEdge *e)
{
    ZVec3 p;
    ZMatrix m;
    double d;

    physClosestPointOnEdge(&r->pos, e, &p);
    r->pos = p;
    r->center = p;
    m.right = e->dir;
    if (fpuBelow(zvec3Dot(&r->fwd, &m.right), 0.0f)) {
        zvec3Negate(&m.right, &m.right);
    }
    if (physGetState(r) == kPhysStateGrind && fpuBelow(zvec3Dot(&m.right, &r->fwd), 0.99f)) {
        zvec3Cross3(&m.at, &m.right, &gZmathOrigin, &gPhysDown);
        zvec3NormalizeScaled(&m.at, &m.at);
        zvec3Cross3(&m.up, &m.at, &gZmathOrigin, &m.right);
        zvec3Zero(&m.pos);
        zmatrixCopyRotation(contactAxes(r), &m);
    }
    d = zvec3Dot(&r->vel, &m.right);
    r->contact = (void *)e;
    r->vel.y = (float)(d * m.right.y);
    r->vel.z = (float)(d * m.right.z);
    r->vel.x = (float)(d * m.right.x);
    physContactCopy(r);
}

/*
 * Poses rider and bike actors from the physics frame while rolling, including the lean of the bike
 * into slow turns (which also re-solves the rider's hands on the bars).
 */
BMX_ADDR(0x0042cce0)
void physPoseActorsRoll(PhysRider *r)
{
    ZActorRootPose *root = zactorGetRootPose(riderActor(r));
    ZMatrix m, lean;
    ZAngle3 bar;
    ZVec3 t, p;
    int32_t tilt = 0;

    root->pos.y = -root->pos.y;
    zactorCopyRootAngles(bikeActor(r), riderActor(r));
    zmatrixCopyRotation(orient(r), &r->basis);
    zmatrixCopyRotation(&m, &r->basis);
    if (r->rot.y != 0 && r->speed > 0.0f && !(r->speed > gPhysRollSpinSpeed)) {
        const int16_t v =
            (int16_t)ftol((((double)gPhysRollSpinSpeed - r->speed) * (int32_t)r->rot.y) / gPhysRollSpinSpeed);

        if (v < -0x4000) {
            tilt = -0x4000;
        } else {
            tilt = v;
            if (tilt >= 0x4000) {
                tilt = 0x4000;
            }
        }
    }
    zactorFaceMatrix(bikeActor(r), &m);
    zactorFaceMatrix(riderActor(r), &m);
    r->pos = r->center;
    r->step = root->pos;
    zmatrixTransformPoint(&t, &m, &r->step);
    r->step = t;
    zvec3Negate(&r->step, &r->step);
    p = r->center;
    zactorSetPos(bikeActor(r), &p);
    zactorSetHeight(bikeActor(r), 0.0f);
    p.x = (float)((double)r->step.x + r->pos.x);
    p.y = r->pos.y;
    p.z = (float)((double)r->pos.z + r->step.z);
    zactorSetPos(riderActor(r), &p);
    zactorSetHeight(riderActor(r), r->step.y);
    if ((int16_t)tilt == 0) {
        bar.x = (ZAngle)0xf556;
        bar.y = 0;
        bar.z = 0;
    } else {
        zmatrixIdentity(&lean);
        zmatrixIdentity(&m);
        zmatrixRotateY(&lean, &lean, (ZAngle)tilt);
        zmatrixRotateX(&lean, &lean, (ZAngle)-0xaaa);
        physRelativeAngles(&m, &lean, 1, &bar);
    }
    zactorSetJointAngles(bikeActor(r), 1, &bar);
    if ((int16_t)tilt != 0) {
        zoverLimbsBegin(r->actor, r->ik);
        physUpdateLimbTargets(r);
        zoverSolveLimbs(r->actor, r->ik);
    }
}

/* Poses rider and bike actors from the physics frame while grinding (yaw trim about the edge). */
BMX_ADDR(0x0042d050)
void physPoseActorsGrind(PhysRider *r)
{
    ZActorRootPose *root = zactorGetRootPose(riderActor(r));
    ZMatrix m, trim;
    ZVec3 t, p;

    zactorCopyRootAngles(bikeActor(r), riderActor(r));
    zactorCopyRotation(bikeActor(r), riderActor(r));
    zmatrixCopyRotation(&r->basis, orient(r));
    zmatrixCopyRotation(&m, orient(r));
    zmatrixIdentity(&trim);
    zmatrixRotateZ(&trim, &trim, r->yawTrim);
    zmatrixMul(&m, &m, &trim);
    root->pos.y = -root->pos.y;
    zmatrixTransformPoint(&t, &m, &root->pos);
    root->pos = t;
    p.x = (float)((double)r->step.x + r->pos.x);
    p.y = r->pos.y;
    p.z = (float)((double)r->pos.z + r->step.z);
    zactorSetPos(riderActor(r), &p);
    zactorSetHeight(riderActor(r), r->step.y);
    r->center.x = (float)(((double)r->step.x + root->pos.x) + r->pos.x);
    r->center.y = (float)(((double)r->pos.y + r->step.y) + root->pos.y);
    r->center.z = (float)(((double)r->pos.z + r->step.z) + root->pos.z);
    zactorSetPos(bikeActor(r), &r->center);
    zactorSetHeight(bikeActor(r), 0.0f);
}

/* Euler angles (divided by `div`) of b relative to a: a^T * b. */
BMX_ADDR(0x0042d250)
void physRelativeAngles(const ZMatrix *a, const ZMatrix *b, int div, ZAngle3 *out)
{
    ZMatrix m;
    double h;
    float hf;

    zmatrixCopyRotation(&m, a);
    zmatrixTranspose(&m, &m);
    zmatrixMul(&m, &m, b);
    h = __builtin_sqrt((double)m.right.y * m.right.y + (double)m.right.x * m.right.x);
    hf = (float)h;
    if (h > 0.001f) {
        out->x = (ZAngle)(zmathAtan2(m.up.z, m.at.z) / div);
        out->y = (ZAngle)(zmathAtan2(-m.right.z, hf) / div);
        out->z = (ZAngle)(zmathAtan2(m.right.y, m.right.x) / div);
        return;
    }
    out->x = (ZAngle)(zmathAtan2(-m.at.y, m.up.y) / div);
    out->z = 0;
    out->y = (ZAngle)(zmathAtan2(-m.right.z, hf) / div);
}

/*
 * Quarter-pipe landing: unless `force`, only when the rider's up is within gPhysPipeAngleLimit of the
 * lip plane normal. Eases the frame towards the normal, pushes the rider out to 120 units from the
 * plane and removes the velocity along the normal.
 */
BMX_ADDR(0x0042d3c0)
void physAlignToQuarterPipe(PhysRider *r, const PhysEdge *e, int force)
{
    ZVec3 n;
    float plane[4];
    double dist;
    float distf, c;

    if (e == NULL) {
        return;
    }
    n = e->normal;
    plane[0] = e->normal.x;
    plane[1] = e->normal.y;
    plane[2] = e->normal.z;
    plane[3] = e->d;
    dist = physPlaneDistance(&r->pos, plane);
    distf = (float)dist;
    if (fpuBelow(dist, 0.0f)) {
        zvec3Negate(&n, &n);
        distf = -distf;
    }
    c = (float)zvec3Dot(&r->up, &n);
    if (force == 0 && zmathAcos(c) >= gPhysPipeAngleLimit) {
        return;
    }
    if (!(r->flags & kPhysAirSpin)) {
        ZMatrix m;

        m.right = r->fwd;
        m.up.x = (float)(((double)n.x - r->up.x) * gPhysAlignRate + r->up.x);
        m.up.y = (float)(((double)n.y - r->up.y) * gPhysAlignRate + r->up.y);
        m.up.z = (float)(((double)n.z - r->up.z) * gPhysAlignRate + r->up.z);
        zvec3Cross3(&m.at, &m.right, &gZmathOrigin, &m.up);
        zvec3NormalizeScaled(&m.at, &m.at);
        zvec3Cross3(&m.right, &m.up, &gZmathOrigin, &m.at);
        zvec3Zero(&m.pos);
        zmatrixCopyRotation(orient(r), &m);
    }
    if (fpuBelow(distf, 120.0f)) {
        const double push = 120.0f - (double)distf;

        r->pos.x = (float)(push * n.x + r->pos.x);
        r->pos.y = (float)(push * n.y + r->pos.y);
        r->pos.z = (float)(push * n.z + r->pos.z);
    }
    {
        const double d = zvec3Dot(&n, &r->vel);

        r->vel.x = (float)(r->vel.x - d * n.x);
        r->vel.y = (float)(r->vel.y - d * n.y);
        r->vel.z = (float)(r->vel.z - d * n.z);
    }
}

/* ---- debug drawing ------------------------------------------------------------------------------- */

static void setRgb(NgifVert3 *v, uint8_t r, uint8_t g, uint8_t b)
{
    v->rgb[0] = r;
    v->rgb[1] = g;
    v->rgb[2] = b;
}

static void edgeVerts(const PhysEdge *e, NgifVert3 *a, NgifVert3 *b)
{
    a->x = (float)e->a[0];
    a->y = (float)e->a[1];
    a->z = (float)e->a[2];
    b->x = (float)e->b[0];
    b->y = (float)e->b[1];
    b->z = (float)e->b[2];
}

/* Wheel contact frames, the current contact and the edges in reach (blue). */
static void drawContacts(PhysRider *r)
{
    NgifVert3 v[4];

    setRgb(&v[0], 0xb0, 0x50, 0x10);
    setRgb(&v[1], 0xb0, 0x50, 0x10);
    for (int i = 0; i < 2; i++) {
        const PhysWheel *w = &r->wheel[i];
        ZVec3 a, t;

        a = w->anchor;
        zmatrixTransformPoint(&t, &r->basis, &a);
        a = t;
        zvec3Add(&a, &a, &r->center);
        v[0].x = a.x;
        v[0].y = a.y;
        v[0].z = a.z;
        if (w->mode == 2) {
            v[1].x = (float)(-((double)r->basis.up.x * w->radius) + a.x);
            v[1].y = (float)(-((double)r->basis.up.y * w->radius) + a.y);
            v[1].z = (float)(-((double)r->basis.up.z * w->radius) + a.z);
            ngifDrawLine3D(0xc, &v[0], &v[1]);
        } else {
            v[1].x = (float)((double)w->dir.x * 30.0f + a.x);
            v[1].y = (float)((double)w->dir.y * 30.0f + a.y);
            v[1].z = (float)((double)w->dir.z * 30.0f + a.z);
            ngifDrawLine3D(0xc, &v[0], &v[1]);
            v[1].x = (float)((double)w->normal.x * 30.0f + v[0].x);
            v[1].y = (float)((double)w->normal.y * 30.0f + v[0].y);
            v[1].z = (float)((double)w->normal.z * 30.0f + v[0].z);
            ngifDrawLine3D(0xc, &v[0], &v[1]);
            v[1].x = (float)((double)w->side.x * 30.0f + v[0].x);
            v[1].y = (float)((double)w->side.y * 30.0f + v[0].y);
            v[1].z = (float)((double)w->side.z * 30.0f + v[0].z);
            ngifDrawLine3D(0xc, &v[0], &v[1]);
        }
    }
    switch (r->surface) {
    case 1:
    case 2:
    case 3:
    case 9:
        setRgb(&v[0], 0x20, 0x40, 0xc0);
        setRgb(&v[1], 0x20, 0x40, 0xc0);
        edgeVerts(r->contact, &v[0], &v[1]);
        ngifDrawLine3D(0xc, &v[0], &v[1]);
        break;
    case 10:
    case 11: {
        const PhysFace *f = r->contact;
        const unsigned n = (f->flags & 1) ? 4 : 3;

        for (unsigned i = 0; i < n; i++) {
            const int16_t *p = f->vert[i];

            v[i].x = (float)p[0];
            v[i].y = (float)p[1];
            v[i].z = (float)p[2];
            setRgb(&v[i], 0x20, 0x80, 0xc0);
        }
        ngifDrawPoly3D(10, (f->flags & 1) + 3, v, 0);
        break;
    }
    default:
        break;
    }
    if (gPhysDebugEdges != NULL) {
        for (PhysEdge **p = gPhysDebugEdges; *p != NULL; p++) {
            setRgb(&v[0], 0x20, 0x40, 0xc0);
            setRgb(&v[1], 0x20, 0x40, 0xc0);
            edgeVerts(*p, &v[0], &v[1]);
            ngifDrawLine3D(0xc, &v[0], &v[1]);
        }
    }
}

/* Seven pulsing, rotating rings of quads above the rider's head (rider flag 4). */
static void drawHalo(PhysRider *r)
{
    NgifVert3 v[4];

    for (uint32_t k = 0, j = 0; k < 0x380; k += 0x80, j++) {
        const uint32_t c = (k / 7 + gPhysDebugFrame) & 0x7f;
        ZVec3 centre, ring;
        float cf, h;
        uint8_t shade;

        centre.x = (float)(((double)r->up.x * 75.0f + r->step.x) + r->pos.x);
        cf = (float)c;
        centre.y =
            (float)(((((double)r->up.y * 75.0f - (double)cf * 0.703125f) + r->step.y) + r->pos.y) - 5.0f);
        centre.z = (float)(((double)r->up.z * 75.0f + r->step.z) + r->pos.z);
        ring.x = (float)(int16_t)ftol((zmathSin((ZAngle)(c << 11)) * 0.15625f) * (int32_t)((c >> 1) + 0x40));
        ring.y = 0.0f;
        ring.z = 0.0f;
        zvec3RotateY(&ring, &ring, (ZAngle)(j * 0x2492));
        h = (float)((double)cf * 0.0703125f + 1.0f);
        shade = (uint8_t)((0x80 - c) >> 1);
        for (int side = 0; side < 2; side++) {
            for (uint32_t q = 0; q < 4; q++) {
                ZVec3 corner;

                corner.x = ((q + 1) & 2) ? h : -h;
                if (side != 0) {
                    corner.x = -corner.x;
                }
                corner.y = (q & 2) ? h : -h;
                corner.z = 0.0f;
                zvec3RotateY(&corner, &corner, (ZAngle)(c * (uint32_t)-0x200));
                v[q].x = (float)(((double)corner.x + ring.x) + centre.x);
                v[q].y = (float)(((double)corner.y + ring.y) + centre.y);
                v[q].z = (float)(((double)corner.z + ring.z) + centre.z);
                setRgb(&v[q], shade, shade, shade);
            }
            ngifDrawPoly3D(0x1a, 4, v, gPhysDebugTexture);
        }
    }
}

/* Debug draw callback (registered by physRegisterDebugDraw). TODO(file): debug rendering, not physics. */
BMX_ADDR(0x004203a0)
int physDebugDraw(void)
{
    for (unsigned i = 0; i < riderCount(); i++) {
        PhysRider *r = &gPhysRiders[i];

        if (!(r->flags & kPhysActive)) {
            continue;
        }
        if (gPhysDebugContact != 0) {
            drawContacts(r);
        }
        if (r->flags & 4) {
            drawHalo(r);
        }
    }
    gPhysDebugFrame++;
    return 0;
}
