/* Cameras (C:\zaxis\Bmx\Src\com\action\camera.c). */
#include "bmx/com/action/camera.h"

#include "bmx/com/action/action.h"
#include "bmx/com/action/comdata.h"
#include "bmx/com/action/overlay.h"
#include "bmx/com/action/physics.h"
#include "bmx/com/zge/bmx/bmxsound.h"
#include "bmx/com/zge/ngif/ngifpsx.h"
#include "bmx/com/zge/ngif/ngifzbsp.h"
#include "bmx/com/zge/ngif/ngifzcollide.h"
#include "bmx/com/zge/ngif/ngifzroller.h"
#include "bmx/com/zge/zactor.h"
#include "bmx/com/zge/zsndsrc.h"
#include "compat/crt.h"
#include "zge/zdebug.h"
#include "zge/zgemods/zobjcamera.h"

static const char cameraFile[] = "C:\\zaxis\\Bmx\\Src\\com\\action\\camera.c";

/* The renderer's camera object. */
#define gCameraObject ORIG_VAR(0x0068ff98, void *, gCameraObject)

/* Mode of the action loop (riding, pause, replay, demo); non-zero while something plays back. */
BMX_ADDR(0x004056a0)
uint32_t cameraPlaybackActive(void)
{
    return gActionMode;
}

/* Resets the cameras of the active players, selects the first and registers the letterbox callback. */
BMX_ADDR(0x004056c0)
void cameraInit(void)
{
    for (uint32_t i = 0; (gPlayerCount < 1 ? 1u : gPlayerCount) > i; i++) {
        gCameras[i].mode = 0xff;
        comdataSetViewMode((int)i, gProfiles[i].option);
        gCameras[i].type = 0xff;
        comdataResetView((int)i, 0);
        gCameras[i].view = 0;
        gCameras[i].changed = 1;
        gCameras[i].flags = 0;
    }
    gCameraPlayer = 0;
    zrollerAddDrawCallback(BMX_FNPTR(ZrollerDrawFn, 0x004074c0, cameraDrawLetterbox));
}

BMX_ADDR(0x00405760)
void cameraNop(void) {}

/* ---- per frame update --------------------------------------------------------------------------------- */

/* x87 FCOMP + TEST AH,0x40: equal or unordered. */
static int x87Equal(double a, double b)
{
    return !(a < b || a > b);
}

/* x87 FCOMP + TEST AH,1: below or unordered. */
static int x87Below(double a, double b)
{
    return !(a >= b);
}

static ZVec3 riderBase(const PhysRider *r)
{
    return (ZVec3){(float)((double)r->step.x + r->pos.x), (float)((double)r->step.y + r->pos.y),
                   (float)((double)r->step.z + r->pos.z)};
}

/*
 * Eye target of a camera whose rider is in the air: 300 units out from the rider towards the old target
 * (500 along x when it was straight above or below on an axis), 700 up.
 */
static void airTarget(CameraState *cam, const PhysRider *r)
{
    ZVec3 t = r->pos;

    zvec3Negate(&t, &t);
    zvec3Add(&t, &t, &cam->target);
    t.y = 0.0f;
    if (x87Equal(t.x, 0.0f) || x87Equal(t.z, 0.0f)) {
        t.x = 500.0f;
    } else {
        zvec3NormalizeScaled(&t, &t);
        zvec3Scale(&t, &t, 300.0f);
    }
    t.y = -700.0f;
    zvec3Add(&t, &t, &r->pos);
    cam->target = t;
}

/* The rider's velocity, or (standing still) facing direction. */
static ZVec3 riderHeading(const PhysRider *r)
{
    return x87Equal(r->speed, 0.0f) ? r->fwd : r->vel;
}

/* Type 0: the chase camera. Returns 1 when the orientation was taken from the rider (mode 3). */
static int chaseGoals(CameraState *cam, PhysRider *r, uint32_t state, float *params)
{
    ZVec3 v;
    ZVec3 e;
    double y;

    if (state == 2 && cam->mode != 3) {
        cam->lookGoal.x = r->pos.x;
        cam->lookGoal.y = (float)((double)r->pos.y - 25.0f);
        cam->lookGoal.z = r->pos.z;
        airTarget(cam, r);
        return 0;
    }
    cam->lookGoal = riderBase(r);
    if (physGetState(r) != 0 || !x87Equal(r->speed, 0.0f)) {
        cam->lookGoal.x =
            (float)(((double)r->center.x - ((double)r->step.x + r->pos.x)) * 0.33f + cam->lookGoal.x);
        cam->lookGoal.y =
            (float)(((double)r->center.y - ((double)r->step.y + r->pos.y)) * 0.33f + cam->lookGoal.y);
        cam->lookGoal.z =
            (float)(((double)r->center.z - ((double)r->step.z + r->pos.z)) * 0.33f + cam->lookGoal.z);
    }
    switch (cam->mode) {
    case 0:
    case 1: {
        const uint32_t s = physGetState(r);

        v = r->vel;
        if ((s == 1 || s == 3 || s == 5) && r->vel.y > 15.0f) {
            v.y = (float)((double)v.y - 15.0f);
        } else {
            v.y = 0.0f;
        }
        zvec3Scale(&v, &v, gCameraTuning.lookAhead);
        zvec3Add(&cam->lookGoal, &cam->lookGoal, &v);
        break;
    }
    case 2:
        v = riderHeading(r);
        v.y = 0.0f;
        zvec3Scale(&v, &v, (float)((double)gCameraTuning.lookAhead + gCameraTuning.lookAhead));
        zvec3Add(&cam->lookGoal, &cam->lookGoal, &v);
        break;
    case 3:
        cam->lookGoal.x = (float)(((double)r->up.x * 80.0f + (double)r->fwd.x * 500.0f) + cam->lookGoal.x);
        cam->lookGoal.y = (float)(((double)r->up.y * 80.0f + (double)r->fwd.y * 500.0f) + cam->lookGoal.y);
        cam->lookGoal.z = (float)(((double)r->up.z * 80.0f + (double)r->fwd.z * 500.0f) + cam->lookGoal.z);
        zmatrixCopyRotation(&cam->orient, (const ZMatrix *)(const void *)&r->fwd);
        break;
    default:
        break;
    }
    switch (cam->mode) {
    case 2:
        v = riderHeading(r);
        v.y = 0.0f;
        zvec3NormalizeScaled(&v, &v);
        cam->target.x = (float)(((double)r->step.x + r->pos.x) - (double)v.x * 150.0f);
        cam->target.y = (float)(((double)r->step.y + r->pos.y) - 800.0f);
        cam->target.z = (float)(((double)r->step.z + r->pos.z) - (double)v.z * 150.0f);
        return 0;
    case 3:
        cam->target.x = (float)((double)r->up.x * 100.0f + r->step.x + r->pos.x);
        cam->target.y = (float)((double)r->up.y * 100.0f + r->step.y + r->pos.y);
        cam->target.z = (float)((double)r->up.z * 100.0f + r->step.z + r->pos.z);
        zactorHide(r->actor);
        zactorHide(r->bikeObj);
        return 1;
    default:
        break;
    }
    /* behind the heading (level while on the ground), raised by the state's height */
    v = x87Equal(r->speed, 0.0f) ? r->fwd : r->vel;
    if (state == 1 || state == 3 || state == 5) {
        v.y = 0.0f;
    }
    if (!(x87Below(__builtin_fabsf(v.x), 0x1.4f8b58p-17f) &&
          x87Below(__builtin_fabsf(v.y), 0x1.4f8b58p-17f) &&
          x87Below(__builtin_fabsf(v.z), 0x1.4f8b58p-17f))) {
        zvec3NormalizeScaled(&v, &v);
    }
    {
        const double dist = gCameraTuning.distance;

        e.x = (float)(-(dist * v.x) + r->step.x + r->pos.x);
        e.y = (float)(-(dist * v.y) + r->step.y + r->pos.y);
        e.z = (float)(-(dist * v.z) + r->step.z + r->pos.z);
    }
    switch (physGetState(r)) {
    case 0:
        y = x87Equal(r->speed, 0.0f) ? (double)e.y - 70.0f : (double)e.y - gCameraTuning.height;
        break;
    case 1:
        if ((r->surface == 1 || r->surface == 3) && x87Below(r->vel.y, 0.0f)) {
            params[3] = 0.75f;
            y = ((double)e.y - 150.0f) - gCameraTuning.heightLow;
        } else {
            y = ((double)e.y - gCameraTuning.heightLow) - gCameraTuning.heightLow;
        }
        break;
    case 3:
    case 5:
        y = (double)e.y - gCameraTuning.heightLow;
        break;
    default:
        y = (double)e.y - gCameraTuning.height;
        break;
    }
    cam->target.x = e.x;
    cam->target.y = (float)y;
    cam->target.z = e.z;
    return 0;
}

/* Squared horizontal distance (whole units, wrapping) from the rider to the eye target. */
static uint32_t targetDistSq(const CameraState *cam, const PhysRider *r)
{
    const uint32_t dx = (uint32_t)bmxFtol((double)cam->target.x - ((double)r->step.x + r->pos.x));
    const uint32_t dz = (uint32_t)bmxFtol((double)cam->target.z - ((double)r->step.z + r->pos.z));

    return dz * dz + dx * dx;
}

/* Type 1: scripted views (0 orbit, 2 / 4 / 6 fixed offsets, the others look from a respawn face). */
static void scriptedGoals(CameraState *cam, PhysRider *r, uint32_t player)
{
    ZVec3 v;
    const uint8_t view = cam->view;

    cam->lookGoal.x = (float)((double)r->up.x * 20.0f + r->step.x + r->pos.x);
    cam->lookGoal.y = (float)((double)r->up.y * 20.0f + r->step.y + r->pos.y);
    cam->lookGoal.z = (float)((double)r->up.z * 20.0f + r->step.z + r->pos.z);
    if (physGetState(r) != 2) {
        cam->lookGoal.x = (float)(((double)r->center.x - cam->lookGoal.x) * 0.33f + cam->lookGoal.x);
        cam->lookGoal.y = (float)(((double)r->center.y - cam->lookGoal.y) * 0.33f + cam->lookGoal.y);
        cam->lookGoal.z = (float)(((double)r->center.z - cam->lookGoal.z) * 0.33f + cam->lookGoal.z);
    }
    switch (view) {
    case 0:
        v = r->vel;
        zvec3Scale(&v, &v, 0.25f);
        zvec3Add(&cam->lookGoal, &cam->lookGoal, &v);
        cam->orbit = (ZAngle)(cam->orbit + 300);
        cam->target = (ZVec3){500.0f, -300.0f, 0.0f};
        zvec3RotateY(&cam->target, &cam->target, cam->orbit);
        zvec3Add(&cam->target, &cam->target, &r->pos);
        if (cam->timer > 0xb4) {
            cam->flags |= kCameraFlagsViewDone;
        }
        break;
    case 2:
    case 4:
    case 6:
        if (physGetState(r) != 2) {
            ZMatrix m;

            v = r->vel;
            zvec3Scale(&v, &v, cam->view == 4 ? 7.5f : 1.75f);
            zvec3Add(&cam->lookGoal, &cam->lookGoal, &v);
            zmatrixCopyRotation(&m, (const ZMatrix *)(const void *)&r->fwd);
            if (!x87Equal(r->speed, 0.0f)) {
                m.right = r->vel;
                zvec3NormalizeScaled(&m.right, &m.right);
                zvec3Cross3(&m.at, &m.right, &gZmathOrigin, &m.up);
            }
            zvec3Zero(&m.pos);
            zmatrixTransformPoint(&cam->target, &m, &gCameraTuning.viewOffset[cam->view].v);
            zvec3Add(&cam->target, &cam->target, &r->pos);
            zvec3Add(&cam->target, &cam->target, &r->step);
        } else {
            airTarget(cam, r);
        }
        if (cam->timer > 0x96) {
            cam->flags |= kCameraFlagsViewDone;
        }
        break;
    case 1:
    case 3:
    case 5:
    case 7:
    case 10: {
        const uint8_t phase = cam->phase;

        v = r->vel;
        zvec3Scale(&v, &v, 1.5f);
        zvec3Add(&cam->lookGoal, &cam->lookGoal, &v);
        switch (phase) {
        case 0:
            if (cam->timer >= 5) {
                cam->phase = (uint8_t)(phase + 1);
            }
            break;
        case 1:
            if (cam->timer > 0x3c) {
                const uint32_t lim = cam->view != 1 ? 2000u : 1300u;
                int advance = 0;

                if (targetDistSq(cam, r) < lim * lim) {
                    switch (cam->view) {
                    case 1:
                        cam->fovGoal = 60.0f;
                        break;
                    case 3:
                        cam->fovGoal = 25.0f;
                        break;
                    case 5:
                    case 10:
                        cam->fovGoal = 30.0f;
                        break;
                    case 7:
                        advance = cam->timer > 300;
                        break;
                    default:
                        break;
                    }
                } else {
                    advance = 1;
                }
                if (advance) {
                    cam->timer = 0;
                    cam->phase = (uint8_t)(phase + 1);
                }
            }
            break;
        case 2: {
            const uint8_t flags = (uint8_t)(cam->flags & ~kCameraFlagsViewDone);

            cam->flags = flags;
            if (cam->view == 1) {
                if (cam->timer >= 0x1e) {
                    cam->flags = flags | kCameraFlagsViewDone;
                }
            } else if (cam->view != 5 || cam->timer >= 10) {
                cam->flags = flags | kCameraFlagsViewDone;
            }
            if ((cam->flags & kCameraFlagsViewDone) != 0 && (cam->flags & kCameraFlagsSplit) == 0) {
                comdataSetView(player, cam->view);
            }
            break;
        }
        default:
            break;
        }
        if (cam->view == 7) {
            cam->target.y = (float)((double)cam->target.y - 5.0f);
        }
        break;
    }
    case 8:
    case 9:
        if (cam->timer > 0x3c) {
            const uint32_t limSq = view != 8 ? 0x895440u : 0x225510u;

            if (targetDistSq(cam, r) >= limSq) {
                if ((cam->flags & kCameraFlagsSplit) != 0) {
                    cam->flags |= kCameraFlagsViewDone;
                } else {
                    comdataSetView(player, view);
                }
            }
        }
        if (cam->view == 9 && gCameraReset != 0 && cam->timer != 0) {
            cam->fovGoal = 15.0f;
        }
        break;
    default:
        break;
    }
}

/*
 * Type 4: on the first frame of the view (timer low byte 0) the target jumps to a random spot of the level
 * on a 400 unit grid, 500 units above ground that is solid, not a hazard and of a usable surface; then the
 * look-at point circles it.
 */
static void randomOrbitGoals(CameraState *cam)
{
    if ((uint8_t)cam->timer == 0) {
        const float *const bounds = zrollerGetWorldBounds();
        PhysGroundHit g;
        ZVec3 spot;

        for (;;) {
            uint32_t lo = (uint32_t)bmxFtol(bounds[0]);
            uint32_t rnd = zmathRandom() & 0xff;
            const uint32_t x = ((rnd * ((uint32_t)bmxFtol(bounds[4]) - lo - 2000u)) >> 8) + lo + 1000u;
            uint32_t tag;

            lo = (uint32_t)bmxFtol(bounds[2]);
            rnd = zmathRandom() & 0xff;
            {
                const uint32_t z = ((rnd * ((uint32_t)bmxFtol(bounds[6]) - lo - 2000u)) >> 8) + lo + 1000u;

                spot.y = 0.0f;
                spot.x = (float)(int16_t)(x / 400u * 400u + 1000u);
                spot.z = (float)(int16_t)(z / 400u * 400u + 1000u);
            }
            if (physProbeGround(&spot, &g) == 0 || (g.flags & 1) != 0) {
                continue;
            }
            if ((((const PhysFace *)g.face)->flags & 0x10) != 0) {
                continue;
            }
            tag = (uint32_t)((const PhysFace *)g.face)->tag >> 12;
            if (tag != 0 && (tag <= 3 || tag > 8)) {
                continue;
            }
            if ((*(const uint32_t *)g.material & 0xf) == 6) {
                continue;
            }
            break;
        }
        cam->target.x = g.pos.x;
        cam->target.z = g.pos.z;
        cam->changed = 1;
        cam->target.y = (float)((double)g.pos.y - 500.0f);
    }
    cam->orbit = (ZAngle)(cam->orbit - 50);
    cam->lookGoal = (ZVec3){500.0f, 200.0f, 0.0f};
    zvec3RotateY(&cam->lookGoal, &cam->lookGoal, cam->orbit);
    zvec3Add(&cam->lookGoal, &cam->lookGoal, &cam->target);
}

/* Pushes the new eye position off the faces within reach that it looks across (no prop faces). */
static void pushOffFaces(const CameraState *cam, ZVec3 *eye)
{
    const ZbspFaceQuery *const q = zbspFindFaceNear(eye, gCameraTuning.faceRadius, (float)zmathCos(0x2aaa));
    const ZbspFaceHit *hit;

    if (q == NULL) {
        return;
    }
    for (hit = q->hits; hit->face != NULL; hit++) {
        ZVec3 d;
        uint32_t ix, iy, iz, ir;

        d.x = (float)((double)hit->point.x - eye->x);
        d.y = (float)((double)hit->point.y - eye->y);
        {
            const double dz = (double)hit->point.z - eye->z;

            d.z = (float)dz;
            iz = (uint32_t)bmxFtol(dz);
        }
        iy = (uint32_t)bmxFtol(d.y);
        ix = (uint32_t)bmxFtol(d.x);
        ir = (uint32_t)bmxFtol(gCameraTuning.faceRadius);
        if (ix * ix + iy * iy + iz * iz < ir * ir) {
            ZVec3 dir = {(float)((double)cam->lookAt.x - eye->x), (float)((double)cam->lookAt.y - eye->y),
                         (float)((double)cam->lookAt.z - eye->z)};
            const ZVec3 normal = *(const ZVec3 *)hit->face;
            double dot;

            zvec3NormalizeScaled(&dir, &dir);
            dot = zvec3Dot(&dir, &normal);
            if (x87Below(dot, 0.0f) && dot > -0x1.666666p-1f && hit->object == 0) {
                const float len = (float)zvec3Length(&d);
                double push;

                zvec3Scale(&d, &d, (float)(1.0f / (double)len));
                push = (double)gCameraTuning.faceRadius - len;
                eye->x = (float)((double)eye->x - push * d.x);
                eye->y = (float)((double)eye->y - push * d.y);
                eye->z = (float)((double)eye->z - push * d.z);
            }
        }
    }
}

/*
 * Keeps the eye at least keepAway units (horizontally) from the rider: closer, it moves out along the
 * same direction (the facing direction when right above), keeping its height (in whole units).
 */
static void keepAway(const PhysRider *r, ZVec3 *eye)
{
    ZVec3 d = {(float)((double)eye->x - ((double)r->step.x + r->pos.x)),
               (float)((double)eye->y - ((double)r->step.y + r->pos.y)),
               (float)((double)eye->z - ((double)r->step.z + r->pos.z))};
    const uint32_t ix = (uint32_t)bmxFtol(d.x);
    const int32_t iy = bmxFtol(d.y);
    const uint32_t iz = (uint32_t)bmxFtol(d.z);
    const uint32_t sq = iz * iz + ix * ix;
    const uint32_t lim = (uint32_t)bmxFtol(gCameraTuning.keepAway);

    if (sq >= lim * lim) {
        return;
    }
    d.y = 0.0f;
    if (sq != 0) {
        zvec3NormalizeScaled(&d, &d);
    } else {
        d.x = r->fwd.x;
        d.z = r->fwd.z;
        d.y = 0.0f;
    }
    zvec3Scale(&d, &d, gCameraTuning.keepAway);
    d.y = (float)(int16_t)iy;
    eye->x = (float)(((double)r->step.x + r->pos.x) + d.x);
    eye->y = (float)(((double)r->step.y + r->pos.y) + d.y);
    eye->z = (float)(((double)d.z + r->step.z) + r->pos.z);
}

/* Eases the eye (a damped spring towards the smoothed target), the look-at point and the field of view. */
static void cameraFollow(CameraState *cam, PhysRider *r, uint32_t player, const float *params)
{
    ZVec3 d;
    ZVec3 eye;
    double sx;
    float sy, sz;

    cam->eyeVel.x = (float)((double)cam->eyeVel.x - (double)params[1] * cam->eyeVel.x);
    cam->eyeVel.y = (float)((double)cam->eyeVel.y - (double)params[1] * cam->eyeVel.y);
    cam->eyeVel.z = (float)((double)cam->eyeVel.z - (double)params[1] * cam->eyeVel.z);
    cam->lookAt.x = (float)(((double)cam->lookGoal.x - cam->lookAt.x) * params[3] + cam->lookAt.x);
    cam->lookAt.y = (float)(((double)cam->lookGoal.y - cam->lookAt.y) * params[3] + cam->lookAt.y);
    cam->lookAt.z = (float)(((double)cam->lookGoal.z - cam->lookAt.z) * params[3] + cam->lookAt.z);
    /* the x difference uses the smoothed value before it is rounded to float */
    sx = ((double)cam->target.x - cam->targetSmooth.x) * params[2] + cam->targetSmooth.x;
    cam->targetSmooth.x = (float)sx;
    sy = (float)(((double)cam->target.y - cam->targetSmooth.y) * params[2] + cam->targetSmooth.y);
    cam->targetSmooth.y = sy;
    sz = (float)(((double)cam->target.z - cam->targetSmooth.z) * params[2] + cam->targetSmooth.z);
    cam->targetSmooth.z = sz;
    d.x = (float)(sx - cam->eye.x);
    d.y = (float)((double)sy - cam->eye.y);
    d.z = (float)((double)sz - cam->eye.z);
    {
        const double gain = zvec3Length(&d) * params[0];
        const float gainF = (float)gain;

        if (gain > 0.0f) {
            zvec3NormalizeScaled(&d, &d);
            cam->eyeVel.x = (float)((double)gainF * d.x + cam->eyeVel.x);
            cam->eyeVel.y = (float)((double)gainF * d.y + cam->eyeVel.y);
            cam->eyeVel.z = (float)((double)gainF * d.z + cam->eyeVel.z);
        }
    }
    eye.x = (float)((double)cam->eye.x + cam->eyeVel.x);
    eye.y = (float)((double)cam->eyeVel.y + cam->eye.y);
    eye.z = (float)((double)cam->eye.z + cam->eyeVel.z);
    if ((cam->flags & kCameraFlagsNoFaces) == 0) {
        pushOffFaces(cam, &eye);
    }
    if ((cam->flags & kCameraFlagsNoKeepAway) == 0) {
        keepAway(r, &eye);
    }
    cam->eyeVel.x = (float)((double)eye.x - cam->eye.x);
    cam->eye.x = eye.x;
    cam->eyeVel.y = (float)((double)eye.y - cam->eye.y);
    cam->eye.y = eye.y;
    cam->eyeVel.z = (float)((double)eye.z - cam->eye.z);
    cam->eye.z = eye.z;
    if (cam->type != 4) {
        /* the rider out of sight for more than 20 frames: cut (chase camera moving) or next view (type 1) */
        const ZVec3 base = riderBase(r);
        PhysRayHit hit;

        if (zcollideRay(&cam->eye, &base, &hit) != 0 && ++cam->blocked > 0x14) {
            if (cam->type == 0) {
                if (!x87Equal(r->speed, 0.0f) && physGetState(r) != 2) {
                    cam->changed = 1;
                }
            } else if (cam->type == 1) {
                if ((cam->flags & kCameraFlagsSplit) == 0) {
                    comdataSetView(player, cam->view);
                } else {
                    cam->flags |= kCameraFlagsViewDone;
                }
            }
        }
    }
    if (!x87Equal(cam->fov, cam->fovGoal)) {
        const double diff = (double)cam->fovGoal - cam->fov;

        if (__builtin_fabs(diff) > 0x1.99999ap-4f) {
            cam->fov = (float)(diff * params[4] + cam->fov);
        } else {
            cam->fov = cam->fovGoal;
        }
    }
}

/* Orientation from the eye to the look-at point, with the world's down (or x when looking straight down). */
static void cameraOrient(CameraState *cam)
{
    const ZVec3 down = {0.0f, -1.0f, 0.0f};

    cam->orient.right.x = (float)((double)cam->lookAt.x - cam->eye.x);
    cam->orient.right.y = (float)((double)cam->lookAt.y - cam->eye.y);
    cam->orient.right.z = (float)((double)cam->lookAt.z - cam->eye.z);
    zvec3NormalizeScaled(&cam->orient.right, &cam->orient.right);
    if (x87Below(__builtin_fabs(zvec3Dot(&cam->orient.right, &down)), 0x1.ff7ceep-1f)) {
        cam->orient.up = (ZVec3){0.0f, -1.0f, 0.0f};
    } else if (__builtin_fabs(zvec3Dot(&cam->orient.right, &cam->orient.up)) > 0x1.ff7ceep-1f) {
        cam->orient.up = (ZVec3){1.0f, 0.0f, 0.0f};
    }
    zvec3Cross3(&cam->orient.at, &cam->orient.right, &gZmathOrigin, &cam->orient.up);
    zvec3NormalizeScaled(&cam->orient.at, &cam->orient.at);
    zvec3Cross3(&cam->orient.up, &cam->orient.at, &gZmathOrigin, &cam->orient.right);
}

/*
 * Per frame update of the selected player's camera (the others stand still): picks the camera's tuning,
 * sets the eye and look-at goals by camera type (type 2 keeps its goals), then eases towards them, or jumps
 * there when the view just changed; finally the cross fades of the random cuts and the view handed to the
 * renderer. Type 4 skips the line of sight check.
 */
BMX_ADDR(0x00405780)
void cameraUpdate(void)
{
    for (uint32_t p = 0; p < (gPlayerCount < 1 ? 1u : gPlayerCount); p++) {
        PhysRider *const r = &gPhysRiders[p];
        CameraState *const cam = &gCameras[p];
        int keepOrient = 0;
        float params[6];
        uint32_t state;
        uint8_t type;

        if (p != gCameraPlayer) {
            continue;
        }
        state = physGetState(r);
        if ((cam->flags & kCameraFlagsSplit) != 0 && (cam->flags & kCameraFlagsViewDone) != 0) {
            comdataSetView(p, cam->view <= 7 ? cam->view + 1u : 0u);
            cam->flags |= kCameraFlagsSplit;
        }
        type = cam->type;
        for (uint32_t i = 0; i < 6; i++) {
            /* indexed like the original: an out of range mode, view or type reads the following rows */
            if (type == 0) {
                params[i] = state == 2 && cam->mode != 3 ? gCameraTuning.air[i]
                                                         : (&gCameraTuning.mode[0][0])[cam->mode * 6u + i];
            } else if (type == 1) {
                params[i] = (&gCameraTuning.view[0][0])[cam->view * 6u + i];
            } else {
                params[i] = (&gCameraTuning.type[0][0])[(type - 2u) * 6u + i];
            }
        }
        cam->fovGoal = params[5];
        switch (type) {
        case 0:
            keepOrient = chaseGoals(cam, r, state, params);
            break;
        case 1:
            scriptedGoals(cam, r, p);
            break;
        case 3:
            cam->orbit = (ZAngle)(cam->orbit + 300);
            cam->lookGoal = riderBase(r);
            cam->target = (ZVec3){500.0f, -700.0f, 0.0f};
            zvec3RotateY(&cam->target, &cam->target, cam->orbit);
            zvec3Add(&cam->target, &cam->target, &r->pos);
            break;
        case 4:
            randomOrbitGoals(cam);
            break;
        default:
            break;
        }
        if (cam->changed == 0) {
            cameraFollow(cam, r, p, params);
        } else if (cam->changed == 1) {
            cam->lookAt = cam->lookGoal;
            cam->targetSmooth = cam->target;
            cam->eye = cam->target;
            zvec3Zero(&cam->eyeVel);
            cam->fov = cam->fovGoal;
            cam->changed = 0;
            cam->blocked = 0;
            zsndSrcClearDoppler();
        }
        if ((cam->flags & kCameraFlagsActive) != 0) {
            zrollerStartCrossFade(); /* (0x80, 0, -16) */
            cam->flags &= (uint8_t)~kCameraFlagsActive;
        } else if ((cam->flags & kCameraFlagsFrozen) != 0) {
            zrollerStartCrossFade(); /* (0x80, 0, -8) */
            cam->flags &= (uint8_t)~kCameraFlagsFrozen;
        }
        if (p == gCameraPlayer) {
            struct {
                CameraView view;
                uint32_t pad24;
                ZMatrix orient;
            } apply;

            if ((gActionOptions & 0x40) != 0 && gPlayerCount != 0) {
                zactorHide(r->actor);
            }
            if (keepOrient == 0) {
                cameraOrient(cam);
            }
            apply.view.camera = gCameraObject;
            apply.view.fov = (uint16_t)bmxFtol((double)cam->fov * 0x1.6c16c2p+7f);
            apply.view.eye = cam->eye;
            apply.view.target = cam->lookAt;
            zmatrixCopyRotation(&apply.orient, &cam->orient);
            cameraApply(&apply.view);
        }
        cam->timer++;
    }
}

/* Applies a view: field of view, eye and target; the eye is also where the sound is heard from. */
BMX_ADDR(0x00407470)
void cameraApply(const CameraView *view)
{
    zobjCameraSetFov(view->camera, view->fov);
    zobjCameraLookAt(view->camera, &view->eye, &view->target, 0);
    bmxSoundPlayAt(0xfa1, &view->eye, 0, 0, 0);
}

/* Draw callback: black bar over the split screen when two players play and option bit 1 is set. */
BMX_ADDR(0x004074c0)
int cameraDrawLetterbox(void)
{
    if ((gActionOptions & 2) != 0 && gPlayerCount != 0) {
        gCameraRect[0] = 0x280;
        gCameraRect[1] = 0x1e0;
        ngifFillScreenBar(0x70007000, 0, 0, 0x200, gCameraRect);
    }
    return 0;
}

/* Hands the selection (and the active / frozen flags) from the current camera to `player`. */
BMX_ADDR(0x00407530)
void cameraSelectPlayer(uint32_t player)
{
    if (player > 1) {
        zdebugFail("player number out of range", cameraFile, 0x49f, 0);
    }
    if (player != gCameraPlayer) {
        CameraState *const from = &gCameras[gCameraPlayer];
        CameraState *const to = &gCameras[player];

        if ((from->flags & kCameraFlagsActive) != 0) {
            to->flags |= kCameraFlagsActive;
            from->flags &= (uint8_t)~kCameraFlagsActive;
            gCameraPlayer = (uint8_t)player;
            return;
        }
        if ((from->flags & kCameraFlagsFrozen) != 0) {
            to->flags |= kCameraFlagsFrozen;
            from->flags &= (uint8_t)~kCameraFlagsFrozen;
        }
        gCameraPlayer = (uint8_t)player;
    }
}
