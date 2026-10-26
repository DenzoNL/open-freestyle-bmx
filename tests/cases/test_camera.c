/*
 * Differential tests of camera.c. The engine calls (camera object, sound, letterbox fill, actors) are
 * stubbed and logged; comdata.c runs natively on both sides.
 */
#include "bmx/com/action/action.h"
#include "bmx/com/action/camera.h"
#include "bmx/com/action/overlay.h"
#include "bmx/com/action/physics.h"
#include "bmx/com/zge/ngif/ngifzbsp.h"
#include "bmx/com/zge/ngif/ngifzroller.h"
#include "dt.h"
#include "dt_gameplay.h"

/* A camera in a state the view code branches on. */
static void randomCamera(CameraState *c)
{
    static const uint8_t types[] = {0, 0, 0, 1, 3, 4, 0xff, 2};
    static const uint8_t modes[] = {0, 1, 2, 3, 3, 0xff, 5};

    dt_fill(c, sizeof *c);
    c->type = (dt_u32() & 7) == 0 ? (uint8_t)dt_u32() : types[dt_u32() % sizeof types];
    c->mode = (dt_u32() & 7) == 0 ? (uint8_t)dt_u32() : modes[dt_u32() % sizeof modes];
    c->flags = (uint8_t)dt_u32();
}

static void randomRiders(void)
{
    for (int p = 0; p < 2; p++) {
        gPhysRiders[p].flags = dt_u32();
        gPhysRiders[p].actor = (void *)(uintptr_t)(0x00300000u + dt_range(0, 0xff) * 0x10u);
        gPhysRiders[p].bikeObj = (void *)(uintptr_t)(0x00310000u + dt_range(0, 0xff) * 0x10u);
    }
}

static void setupPlayback(DtCtx *c)
{
    dtgpBegin(c, 0);
    gActionMode = (uint8_t)dt_u32();
}

static void setupNone(DtCtx *c)
{
    dtgpBegin(c, 0);
}

static void setupInit(DtCtx *c)
{
    dtgpBegin(c, DTGP_DEBUG | DTGP_ACTOR);
    gPlayerCount = (uint8_t)dt_range(0, 2);
    randomCamera(&gCameras[0]);
    randomCamera(&gCameras[1]);
    gCameraPlayer = (uint8_t)dt_u32();
    randomRiders();
    for (int i = 0; i < 2; i++) {
        gProfiles[i].option = (dt_u32() & 3) == 0 ? (uint8_t)dt_u32() : (uint8_t)dt_range(0, 4);
    }
    for (int i = 0; i < 8; i++) {
        gZrollerDrawCallbacks[i] =
            (dt_u32() & 1) ? NULL : (ZrollerDrawFn)(uintptr_t)(0x00401000u + (uint32_t)i);
    }
    if ((dt_u32() & 7) == 0) {
        for (int i = 0; i < 8; i++) {
            gZrollerDrawCallbacks[i] = (ZrollerDrawFn)(uintptr_t)(0x00401000u + (uint32_t)i);
        }
    }
}

static void setupApply(DtCtx *c)
{
    dtgpBegin(c, DTGP_CAMERA | DTGP_SOUND);
    CameraView *v = dt_alloc(sizeof *v);
    dt_fill(v, sizeof *v);
    v->camera = (void *)(uintptr_t)(0x00320000u + dt_range(0, 0xff) * 0x10u);
    v->eye.x = dt_float_edgy(-1000.0f, 1000.0f);
    v->eye.y = dt_float_edgy(-1000.0f, 1000.0f);
    v->eye.z = dt_float_edgy(-1000.0f, 1000.0f);
    v->target.x = dt_float_edgy(-1000.0f, 1000.0f);
    v->target.y = dt_float_edgy(-1000.0f, 1000.0f);
    v->target.z = dt_float_edgy(-1000.0f, 1000.0f);
    DT_ARG_PTR(c, 0, v);
}

static void setupLetterbox(DtCtx *c)
{
    dtgpBegin(c, DTGP_CAMERA);
    gActionOptions = (uint8_t)((dt_u32() & 3) != 0 ? dt_u32() | 2 : dt_u32());
    gPlayerCount = (uint8_t)((dt_u32() & 3) != 0 ? dt_range(0, 2) : (int32_t)dt_u32());
    gCameraRect[0] = dt_u32();
    gCameraRect[1] = dt_u32();
}

static void setupSelect(DtCtx *c)
{
    dtgpBegin(c, DTGP_DEBUG);
    randomCamera(&gCameras[0]);
    randomCamera(&gCameras[1]);
    gCameraPlayer = (uint8_t)dt_range(0, 1);
    if ((dt_u32() & 1) != 0) {
        gCameras[gCameraPlayer].flags = (uint8_t)(gCameras[gCameraPlayer].flags | 0x10);
    }
    DT_ARG_INT(c, 0, (dt_u32() & 7) == 0 ? (uint32_t)dt_range(2, 3) : (uint32_t)dt_range(0, 1));
}

/* ---- cameraUpdate --------------------------------------------------------------------------------- */

static float rf(float lo, float hi)
{
    return dt_float(lo, hi);
}

static void rvec(ZVec3 *v, const ZVec3 *around, float range)
{
    v->x = around->x + rf(-range, range);
    v->y = around->y + rf(-range, range);
    v->z = around->z + rf(-range, range);
}

/* A unit vector (or, rarely, zero / tiny / NaN components). */
static void rdir(ZVec3 *v)
{
    static const ZVec3 zero = {0.0f, 0.0f, 0.0f};
    double l;

    do {
        rvec(v, &zero, 1.0f);
        l = __builtin_sqrt((double)v->x * v->x + (double)v->y * v->y + (double)v->z * v->z);
    } while (l < 0.1);
    v->x = (float)(v->x / l);
    v->y = (float)(v->y / l);
    v->z = (float)(v->z / l);
    switch (dt_u32() % 16) {
    case 0:
        *v = zero;
        break;
    case 1:
        v->x = 0.0f;
        v->z = 0.0f;
        v->y = (dt_u32() & 1) != 0 ? 1.0f : -1.0f;
        break;
    case 2:
        v->x = rf(-1e-6f, 1e-6f);
        v->y = rf(-1e-6f, 1e-6f);
        v->z = rf(-1e-6f, 1e-6f);
        break;
    default:
        break;
    }
}

static ZbspFaceQuery *camFaces;

static void *BMX_CDECL stubFacesNear(const ZVec3 *p, float radius, float cosLimit)
{
    dtgpLog(0x46807001u);
    dtgpLogN(p, 3);
    dtgpLogF(radius);
    dtgpLogF(cosLimit);
    return camFaces;
}

static int BMX_CDECL stubRay(const ZVec3 *from, const ZVec3 *to, void *hit)
{
    dtgpLog(0x469b0001u);
    dtgpLogN(from, 3);
    dtgpLogN(to, 3);
    dtgpLog(dt_canon((uint32_t)(uintptr_t)hit));
    return (int)(dt_u32() & 1);
}

static int BMX_CDECL stubProbe(const ZVec3 *p, PhysGroundHit *out)
{
    PhysFace *face = dt_alloc(sizeof *face);
    uint32_t *material = dt_alloc(4);

    dtgpLog(0x420d7001u);
    dtgpLogN(p, 3);
    /* (no dt_fill here: the harness's fill stream is not rewound between the two sides) */
    face->flags = (uint8_t)((dt_u32() & 7) == 0 ? 0x10 : 0);
    face->tag =
        (uint16_t)(((dt_u32() & 3) == 0 ? (uint32_t)dt_range(0, 15) : (uint32_t)dt_range(4, 8)) << 12 |
                   (dt_u32() & 0xfff));
    *material = (dt_u32() & 7) == 0 ? 6 : dt_u32();
    out->flags = (dt_u32() & 7) == 0 ? 1 : 0;
    out->pos.x = rf(-3000.0f, 3000.0f);
    out->pos.y = rf(-3000.0f, 3000.0f);
    out->pos.z = rf(-3000.0f, 3000.0f);
    out->face = face;
    out->material = material;
    return (dt_u32() & 7) != 0;
}

static float camBounds[8];

static const float *BMX_CDECL stubBounds(void)
{
    dtgpLog(0x46ceb001u);
    return camBounds;
}

static void BMX_CDECL stubDoppler(void)
{
    dtgpLog(0x47e4b001u);
}

static void BMX_CDECL stubCrossFade(void)
{
    dtgpLog(0x46d59001u);
}

/* Faces near the camera: up to four, some facing the look direction, some belonging to props. */
static void camFaceQuery(const ZVec3 *eye)
{
    const int n = dt_range(0, 4);
    ZbspFaceHit *hits = dt_alloc((uint32_t)(n + 1) * sizeof *hits);

    camFaces = (dt_u32() % 5 == 0) ? NULL : dt_alloc(sizeof *camFaces);
    if (camFaces != NULL) {
        dt_fill(camFaces, sizeof *camFaces);
        camFaces->hits = hits;
    }
    for (int i = 0; i < n; i++) {
        PhysFace *f = dt_alloc(sizeof *f);

        dt_fill(f, sizeof *f);
        rdir(&f->normal);
        dt_fill(&hits[i], sizeof hits[i]);
        hits[i].face = f;
        rvec(&hits[i].point, eye, (dt_u32() & 3) == 0 ? 400.0f : 150.0f);
        hits[i].object = (dt_u32() & 3) == 0 ? (int32_t)dt_u32() : 0;
    }
    hits[n].face = NULL;
}

static void camRider(PhysRider *r)
{
    static const ZVec3 zero = {0.0f, 0.0f, 0.0f};

    r->state = (uint8_t)((dt_u32() & 7) == 0 ? dt_u32() : (uint32_t)dt_range(0, 6));
    r->surface = (uint8_t)dt_range(0, 3);
    rvec(&r->pos, &zero, 2000.0f);
    rvec(&r->step, &zero, 5.0f);
    rvec(&r->center, &r->pos, 60.0f);
    rdir(&r->fwd);
    rdir(&r->up);
    rdir(&r->side);
    switch (dt_u32() % 4) {
    case 0:
        r->vel = zero;
        break;
    case 1:
        rvec(&r->vel, &zero, 1.0f);
        break;
    default:
        rvec(&r->vel, &zero, 40.0f);
        break;
    }
    if ((dt_u32() & 3) == 0) {
        r->vel.y = rf(10.0f, 30.0f);
    }
    r->speed = (dt_u32() & 3) == 0 ? 0.0f : rf(-50.0f, 50.0f);
    r->actor = (void *)(uintptr_t)(0x00300000u + dt_range(0, 0xff) * 0x10u);
    r->bikeObj = (void *)(uintptr_t)(0x00310000u + dt_range(0, 0xff) * 0x10u);
}

static void setupUpdate(DtCtx *c)
{
    static const uint8_t types[] = {0, 0, 0, 0, 1, 1, 1, 3, 4, 2};
    static const uint32_t timers[] = {0,    1,    4,    5,    9,    10,    0x1d,  0x1e, 0x3c,
                                      0x3d, 0x96, 0x97, 0xb4, 0xb5, 0x12c, 0x12d, 0x100};
    CameraState *cam;
    PhysRider *r;

    dtgpBegin(c, DTGP_DEBUG | DTGP_ACTOR | DTGP_CAMERA | DTGP_SOUND);
    dt_stub(0x00468070, NULL, (void *)stubFacesNear);
    dt_stub(0x00469b00, NULL, (void *)stubRay);
    dt_stub(0x00420d70, NULL, (void *)stubProbe);
    dt_stub(0x0046ceb0, NULL, (void *)stubBounds);
    dt_stub(0x0047e4b0, NULL, (void *)stubDoppler);
    dt_stub(0x0046d590, NULL, (void *)stubCrossFade);
    gPlayerCount = (uint8_t)dt_range(0, 2);
    gCameraPlayer = (uint8_t)((dt_u32() & 15) == 0 ? dt_range(2, 3) : dt_range(0, 1));
    gActionOptions = (uint8_t)dt_u32();
    gCameraReset = dt_u32() & 1;
    camBounds[0] = rf(-6000.0f, -2000.0f);
    camBounds[2] = rf(-6000.0f, -2000.0f);
    camBounds[4] = rf(2000.0f, 6000.0f);
    camBounds[6] = rf(2000.0f, 6000.0f);
    for (int p = 0; p < 2; p++) {
        r = &gPhysRiders[p];
        cam = &gCameras[p];
        camRider(r);
        randomCamera(cam);
        cam->type = (dt_u32() & 15) == 0 ? (uint8_t)dt_range(0, 255) : types[dt_u32() % sizeof types];
        cam->mode = (uint8_t)((dt_u32() & 15) == 0 ? dt_range(4, 6) : dt_range(0, 3));
        cam->view = (uint8_t)((dt_u32() & 15) == 0 ? dt_range(11, 12) : dt_range(0, 10));
        cam->phase = (uint8_t)dt_range(0, 3);
        cam->timer = (dt_u32() & 3) == 0 ? dt_u32() : timers[dt_u32() % (sizeof timers / sizeof timers[0])];
        cam->changed = (uint8_t)((dt_u32() & 7) == 0 ? 2 : dt_u32() % 3 == 0);
        cam->blocked = (uint16_t)dt_range(0x10, 0x18);
        rvec(&cam->eye, &r->pos, 800.0f);
        rvec(&cam->target, &r->pos, (dt_u32() & 1) != 0 ? 3000.0f : 200.0f);
        if ((dt_u32() & 7) == 0) {
            cam->target.x = r->pos.x;
        }
        rvec(&cam->targetSmooth, &cam->target, 50.0f);
        rvec(&cam->lookAt, &r->pos, 300.0f);
        rvec(&cam->lookGoal, &r->pos, 300.0f);
        rvec(&cam->eyeVel, &(ZVec3){0.0f, 0.0f, 0.0f}, 30.0f);
        cam->fov = rf(10.0f, 90.0f);
        cam->fovGoal = (dt_u32() & 3) == 0 ? cam->fov : cam->fov + rf(-0.2f, 0.2f);
        rdir(&cam->orient.right);
        rdir(&cam->orient.up);
        rdir(&cam->orient.at);
        if ((dt_u32() & 7) == 0) {
            /* eye straight above the look-at point */
            cam->lookAt = r->pos;
            cam->eye = r->pos;
            cam->eye.y -= 500.0f;
        }
    }
    camFaceQuery(&gCameras[gCameraPlayer & 1].eye);
}

DT_CASES(camera_cases) = {
    {"cameraPlaybackActive", 0x004056a0, cameraPlaybackActive, 0, DT_RET_INT, setupPlayback, 30, 0},
    {"cameraInit", 0x004056c0, cameraInit, 0, DT_RET_VOID, setupInit, 300, 0},
    {"cameraNop", 0x00405760, cameraNop, 0, DT_RET_VOID, setupNone, 1, 0},
    {"cameraUpdate", 0x00405780, cameraUpdate, 0, DT_RET_VOID, setupUpdate, 3000, 0},
    {"cameraApply", 0x00407470, cameraApply, 1, DT_RET_VOID, setupApply, 100, 0},
    {"cameraDrawLetterbox", 0x004074c0, cameraDrawLetterbox, 0, DT_RET_INT, setupLetterbox, 100, 0},
    {"cameraSelectPlayer", 0x00407530, cameraSelectPlayer, 1, DT_RET_VOID, setupSelect, 300, 0},
};
