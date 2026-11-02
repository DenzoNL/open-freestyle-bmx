/*
 * Differential tests of prop.c. The prop definitions, records and node list are filled with random data
 * (ids in a small range so the tags match); scene objects, world object signals, goals, sounds and shards
 * are stubbed and logged.
 */
#include "bmx/com/action/overlay.h"
#include "bmx/com/action/physics.h"
#include "bmx/com/action/prop.h"
#include "bmx/com/zge/bmx/zoverii.h"
#include "bmx/com/zge/ngif/ngifzroller.h"
#include "dt.h"
#include "dt_gameplay.h"
#include "zge/zgemods/zobjcamera.h"
#include "zge/zobj.h"

enum { kMaxNodes = 8 };

static uint16_t pickId(void)
{
    return (uint16_t)dt_range(0, 12);
}

static void randomDefs(void)
{
    dt_fill(&gPropDefs, sizeof gPropDefs);
    for (int l = 0; l < 12; l++) {
        for (int i = 0; i < 16; i++) {
            PropDef *d = &gPropDefs[l][i];
            d->id = pickId();
            d->type =
                (dt_u32() & 7) < 4 ? 0 : (uint8_t)((dt_u32() & 1) != 0 ? dt_range(3, 5) : (int32_t)dt_u32());
            d->kind = (dt_u32() & 15) == 0 ? (uint8_t)dt_u32() : (uint8_t)dt_range(0, 17);
            d->sound = (dt_u32() & 1) != 0 ? 0xff : (uint8_t)dt_u32();
            d->soundStop = (dt_u32() & 1) != 0 ? 0xff : (uint8_t)dt_u32();
        }
    }
    gCurrentLevel = (uint8_t)dt_range(0, 11);
}

static void randomRecord(PropRecord *r)
{
    static const uint32_t states[] = {0, 1, 1, 1, 3, 3};

    dt_fill(r, sizeof *r);
    r->state = (dt_u32() & 7) == 0 ? (uint8_t)dt_u32() : (uint8_t)states[dt_u32() % 6];
    r->flags = (uint8_t)dt_u32();
    r->index = (uint8_t)dt_range(0, 15);
    r->mode = (dt_u32() & 3) == 0 ? dt_u32() : (uint32_t)dt_range(0, 4);
    r->timer = (dt_u32() & 3) == 0 ? dt_u32() : (dt_u32() & 1);
    r->object = (PhysPropObject *)(uintptr_t)(0x00340000u + dt_range(0, 0xff) * 0x200u);
    r->pos.x = dt_float_edgy(-10000.0f, 10000.0f);
    r->pos.y = dt_float_edgy(-10000.0f, 10000.0f);
    r->pos.z = dt_float_edgy(-10000.0f, 10000.0f);
    r->vel.x = dt_float_edgy(-100.0f, 100.0f);
    r->vel.y = dt_float_edgy(-100.0f, 100.0f);
    r->vel.z = dt_float_edgy(-100.0f, 100.0f);
    if ((dt_u32() & 1) != 0) {
        r->pos.z = dt_float_edgy(-9000.0f, -7000.0f);
    }
    r->angles.x = (dt_u32() & 3) == 0 ? 0 : (ZAngle)dt_u32();
    r->angles.y = (dt_u32() & 3) == 0 ? 0 : (ZAngle)dt_u32();
    r->angles.z = (dt_u32() & 3) == 0 ? 0 : (ZAngle)dt_u32();
    {
        /* A rotation (sometimes pitched straight up) plus a random position row. */
        ZAngle3 a = {(ZAngle)dt_u32(),
                     (ZAngle)((dt_u32() & 3) == 0 ? 0x4000 * dt_range(-1, 1) : (int32_t)dt_u32()),
                     (ZAngle)dt_u32()};
        zmatrixIdentity(&r->orient);
        zmatrixRotateEuler(&r->orient, &r->orient, &a);
        r->orient.pos.x = dt_float_edgy(-1000.0f, 1000.0f);
        r->orient.pos.y = dt_float_edgy(-1000.0f, 1000.0f);
        r->orient.pos.z = dt_float_edgy(-1000.0f, 1000.0f);
        if ((dt_u32() & 7) == 0) {
            r->orient.right.x = dt_float_edgy(-0.001f, 0.001f);
            r->orient.right.y = dt_float_edgy(-0.001f, 0.001f);
        }
    }
}

static void randomRecords(void)
{
    for (int i = 0; i < 16; i++) {
        randomRecord(&gPropRecords[i]);
    }
}

/* A list of `count` prop nodes (frames with a model, tags with ids from the small pool); installs it. */
static int randomNodes(PhysPropNode **nodes)
{
    const int count = dt_range(0, kMaxNodes);
    PhysPropNode *prev = NULL;

    for (int i = count - 1; i >= 0; i--) {
        PhysPropNode *n = dt_alloc(sizeof *n);
        PhysPropTag *tag = dt_alloc(sizeof *tag);
        uint8_t *frame = dt_alloc(0x40);

        dt_fill(frame, 0x40);
        *(void **)(frame + 0x2c) = (void *)(uintptr_t)(0x00350000u + dt_range(0, 0xff) * 0x10u);
        tag->range = (dt_u32() & 7) == 0 ? (uint16_t)dt_u32() : (uint16_t)(pickId() + (dt_u32() & 1));
        n->next = prev;
        n->frame = (PhysPropFrame *)frame;
        n->tag = (dt_u32() & 7) == 0 ? NULL : tag;
        nodes[i] = n;
        prev = n;
    }
    *zobjPropList() = count > 0 ? nodes[0] : NULL;
    return count;
}

/* Node state table: k entries naming nodes of the list, a terminating NULL, random flags. */
static void randomNodeStates(PropNodeState *states, PhysPropNode **nodes, int count, int full)
{
    const int k = (full || count == 0) ? (count == 0 ? 0 : 64) : dt_range(0, 12);

    for (int i = 0; i < 64; i++) {
        states[i].flags = dt_u32();
        states[i].pad08 = dt_u32();
        states[i].node = (i < k && count > 0) ? nodes[dt_u32() % (uint32_t)count] : NULL;
    }
    if (count > 0 && !full && k < 64) {
        states[k].node = NULL;
    }
}

static PropState *randomState(PhysPropNode **nodes, int count)
{
    PropState *s = dt_alloc(sizeof *s);

    for (int i = 0; i < 16; i++) {
        randomRecord(&s->record[i]);
    }
    s->pad840 = dt_u32();
    randomNodeStates((PropNodeState *)s->nodes, nodes, count, (dt_u32() & 7) == 0);
    return s;
}

static void setupFindNode(DtCtx *c)
{
    PhysPropNode *nodes[kMaxNodes];

    dtgpBegin(c, 0);
    randomNodes(nodes);
    DT_ARG_INT(c, 0, (dt_u32() & 7) == 0 ? dt_u32() : (uint32_t)pickId() + (dt_u32() & 1));
}

static void setupStopSounds(DtCtx *c)
{
    dtgpBegin(c, DTGP_SOUND);
    randomDefs();
    randomRecords();
}

static void setupNone(DtCtx *c)
{
    dtgpBegin(c, 0);
}

static void setupLoad(DtCtx *c)
{
    PhysPropNode *nodes[kMaxNodes];

    dtgpBegin(c, DTGP_PROP);
    randomDefs();
    dt_fill(&gPropState, sizeof gPropState);
    const int count = randomNodes(nodes);
    DT_ARG_PTR(c, 0, randomState(nodes, count));
}

static void setupReset(DtCtx *c)
{
    PhysPropNode *nodes[kMaxNodes];

    dtgpBegin(c, DTGP_PROP);
    randomDefs();
    dt_fill(&gPropState, sizeof gPropState);
    const int count = randomNodes(nodes);
    PropState *s = randomState(nodes, count);
    __builtin_memcpy(&gPropDefaults, s, kPropStateSaved);
}

static void setupFind(DtCtx *c)
{
    dtgpBegin(c, DTGP_DEBUG);
    randomDefs();
    randomRecords();
    DT_ARG_INT(c, 0, (dt_u32() & 7) == 0 ? dt_u32() : (uint32_t)pickId());
}

static void setupCollect(DtCtx *c)
{
    PhysPropNode *nodes[kMaxNodes];

    dtgpBegin(c, DTGP_PROP | DTGP_SOUND);
    randomDefs();
    randomRecords();
    randomNodes(nodes);
    DT_ARG_INT(c, 0, dt_u32() & 7);
    DT_ARG_PTR(c, 1, &gPropRecords[dt_range(0, 15)]);
}

static void setupTouch(DtCtx *c)
{
    PhysPropNode *nodes[kMaxNodes];

    dtgpBegin(c, DTGP_PROP | DTGP_SOUND);
    randomDefs();
    randomRecords();
    randomNodes(nodes);
    dtgp.goalResult = dt_u32() & 1;
    DT_ARG_INT(c, 0, dt_range(0, 3));
    DT_ARG_PTR(c, 1, &gPropRecords[dt_range(0, 15)]);
}

static void setupHitFace(DtCtx *c)
{
    PhysPropNode *nodes[kMaxNodes];

    dtgpBegin(c, DTGP_PROP | DTGP_SOUND);
    randomDefs();
    const int count = randomNodes(nodes);
    PhysFace *face = dt_alloc(sizeof *face);
    ZVec3 *position = dt_alloc(sizeof *position);

    randomNodeStates(gPropNodeStates, nodes, count, (dt_u32() & 7) == 0);
    dt_fill(face, sizeof *face);
    face->tag = (dt_u32() & 3) == 0 ? (uint16_t)dt_u32() : (uint16_t)(pickId() + (dt_u32() & 1));
    position->x = dt_float_edgy(-1000.0f, 1000.0f);
    position->y = dt_float_edgy(-1000.0f, 1000.0f);
    position->z = dt_float_edgy(-1000.0f, 1000.0f);
    dtgp.position = position;
    DT_ARG_INT(c, 0, dt_range(0, 1));
    DT_ARG_PTR(c, 1, face);
}

static void setupRecordOnly(DtCtx *c)
{
    PhysPropNode *nodes[kMaxNodes];

    dtgpBegin(c, DTGP_PROP);
    randomDefs();
    randomRecords();
    randomNodes(nodes);
    DT_ARG_PTR(c, 0, &gPropRecords[dt_range(0, 15)]);
}

static void setupUpdateMove(DtCtx *c)
{
    static const uint8_t kinds[] = {7, 8, 9, 10, 0x10, 0x10, 0x10, 0x11};
    PropRecord *rec;

    dtgpBegin(c, DTGP_PROP);
    randomDefs();
    randomRecords();
    rec = &gPropRecords[dt_range(0, 15)];
    PropDef *def = &gPropDefs[gCurrentLevel][rec->index];
    def->type = (dt_u32() & 7) == 0 ? (uint8_t)dt_range(1, 3) : 0;
    def->kind = kinds[dt_u32() % sizeof kinds];
    if ((dt_u32() & 1) != 0) {
        rec->timer = 0;
    }
    if ((dt_u32() & 1) != 0) {
        rec->mode = (uint32_t)dt_range(1, 2);
    }
    DT_ARG_PTR(c, 0, rec);
}

static void setupSetShown(DtCtx *c)
{
    PhysPropNode *nodes[kMaxNodes];

    dtgpBegin(c, DTGP_PROP);
    randomDefs();
    randomRecords();
    randomNodes(nodes);
    DT_ARG_PTR(c, 0, &gPropRecords[dt_range(0, 15)]);
    DT_ARG_INT(c, 1, (dt_u32() & 7) == 0 ? (int32_t)dt_u32() : dt_range(0, 2));
}

static void **gPropHits[16];

/* zbspFindPropsNear: the hit lists of the fixture, indexed by the (16 bit) id. */
static void **BMX_CDECL stubFindProps(int lo, int hi)
{
    dtgpLog(0x468e3000u);
    dtgpLog((uint32_t)lo & 0xffff);
    dtgpLog((uint32_t)hi & 0xffff);
    return gPropHits[(uint32_t)lo & 0xf];
}

static PhysPropObject *randomObject(void)
{
    PhysPropObject *o = dt_alloc(0x200);

    dt_fill(o, 0x200);
    o->flags = (uint16_t)dt_u32();
    return o;
}

/* A node list of up to 70 nodes (more than the 64 the state table holds). */
static void manyNodes(void)
{
    const int count = dt_u32() % 10 == 0 ? dt_range(60, 70) : dt_range(0, 12);
    PhysPropNode *head = NULL;

    for (int i = 0; i < count; i++) {
        PhysPropNode *n = dt_alloc(sizeof *n);
        PhysPropTag *tag = dt_alloc(sizeof *tag);
        uint8_t *frame = dt_alloc(0x40);

        dt_fill(frame, 0x40);
        tag->range = (dt_u32() & 7) == 0 ? 0 : (uint16_t)(pickId() + (dt_u32() & 1));
        n->next = head;
        n->frame = (PhysPropFrame *)frame;
        n->object = (struct PhysPropObject *)(uintptr_t)(0x00360000u + dt_range(0, 0xff) * 0x200u);
        n->tag = (dt_u32() & 7) == 0 ? NULL : tag;
        head = n;
    }
    *zobjPropList() = head;
}

static void setupInit(DtCtx *c)
{
    dtgpBegin(c, DTGP_DEBUG | DTGP_PROP | DTGP_SOUND);
    dt_stub(0x00468e30, NULL, (void *)stubFindProps);
    randomDefs();
    if ((dt_u32() & 1) != 0) {
        gCurrentLevel = 0;
    }
    for (int i = 0; i < 16; i++) {
        PropDef *d = &gPropDefs[gCurrentLevel][i];

        if (gCurrentLevel == 0 && (dt_u32() & 3) == 0) {
            d->id = (uint16_t)dt_range(0x13, 0x1a); /* the collectables that start out collected */
        }
        if ((dt_u32() & 15) == 0) {
            d->id = 0;
        }
    }
    for (int i = 0; i < 16; i++) {
        const uint32_t kind = dt_u32() % 4;

        gPropHits[i] = NULL;
        if (kind != 0) {
            void **hits = (void **)dt_alloc(5 * sizeof(void *));
            const int extra = kind == 3 ? dt_range(1, 3) : 0;

            hits[0] = randomObject();
            for (int k = 0; k < extra; k++) {
                hits[1 + k] = randomObject();
            }
            gPropHits[i] = hits;
        }
    }
    randomRecords();
    dt_fill(&gPropState, sizeof gPropState);
    dt_fill(&gPropDefaults, sizeof gPropDefaults);
    manyNodes();
    {
        const int filled = (dt_u32() & 15) == 0 ? 8 : dt_range(0, 3);

        for (int i = 0; i < 8; i++) {
            gZrollerDrawCallbacks[i] = i < filled ? (ZrollerDrawFn)(uintptr_t)0x1234 : NULL;
        }
    }
}

/* ---- propSwitch / propUpdate ---------------------------------------------------------------------- */

/* World object callees and the props a switch or an update hands on to, logged. */
static void BMX_CDECL stubSignal(void *object, int signal)
{
    dtgpLog(0x46661001u);
    dtgpLogP(object);
    dtgpLog((uint32_t)signal);
}

static void BMX_CDECL stubSetPosition(void *object, const ZVec3 *pos)
{
    dtgpLog(0x4662b001u);
    dtgpLogP(object);
    dtgpLogN(pos, 3);
}

static void BMX_CDECL stubSetRotation(void *object, const int16_t *rot)
{
    dtgpLog(0x4661c001u);
    dtgpLogP(object);
    dtgpLog((uint16_t)rot[0]);
    dtgpLog((uint16_t)rot[1]);
    dtgpLog((uint16_t)rot[2]);
}

static void BMX_CDECL stubHandOn(uint32_t player, uint8_t *prop)
{
    dtgpLog(0x42ee4001u);
    dtgpLog(player);
    dtgpLogP(prop);
}

static void BMX_CDECL stubSwitch(PropRecord *prop, uint32_t player, int release)
{
    dtgpLog(0x42f0d001u);
    dtgpLogP(prop);
    dtgpLog(player);
    dtgpLog((uint32_t)release);
    prop->mode = dt_u32() & 1;
}

static int BMX_CDECL stubGoal(int player, void *prop)
{
    dtgpLog(0x40ca6001u);
    dtgpLog((uint32_t)player);
    dtgpLogP(prop);
    return (int)(dt_u32() & 1);
}

static float propShape[4];

static const uint8_t *BMX_CDECL stubCollideObject(uint32_t id)
{
    dtgpLog(0x46953001u);
    dtgpLog(id & 0xffff);
    return (const uint8_t *)propShape;
}

static uint8_t BMX_CDECL stubSetState(PhysRider *r, int state)
{
    dtgpLog(0x420cb001u);
    dtgpLogP(r);
    dtgpLog((uint32_t)state);
    return 2;
}

static void BMX_CDECL stubBurst(void)
{
    dtgpLog(0x408a8001u);
}

static void switchStubs(void)
{
    dt_stub(0x00466610, NULL, (void *)stubSignal);
    dt_stub(0x004662b0, NULL, (void *)stubSetPosition);
    dt_stub(0x004661c0, NULL, (void *)stubSetRotation);
}

/* A definition of any type (mostly 0 .. 5) and kind (mostly 0 .. 21). */
static void randomSwitchDef(PropDef *d)
{
    d->type = (dt_u32() & 1) != 0 ? 0 : ((dt_u32() & 7) == 0 ? (uint8_t)dt_u32() : (uint8_t)dt_range(1, 5));
    d->kind = (dt_u32() & 15) == 0 ? (uint8_t)dt_u32() : (uint8_t)dt_range(0, 21);
}

static PhysPropObject *propObject(void)
{
    PhysPropObject *o = dt_alloc(sizeof *o);

    dt_fill(o, sizeof *o);
    o->scale[0] = dt_float(0.5f, 3.0f);
    o->scale[1] = dt_float(0.5f, 3.0f);
    o->scale[2] = dt_float(0.5f, 3.0f);
    return o;
}

/* Linked prop ids (paramA / paramB) that propFind may or may not resolve. */
static void randomLinks(PropRecord *r)
{
    r->paramA = (dt_u32() & 3) == 0 ? 0 : (uint32_t)pickId();
    r->paramB = (dt_u32() & 3) == 0 ? 0 : (uint32_t)pickId();
}

static void setupSwitch(DtCtx *c)
{
    PropRecord *rec;
    const uint32_t player = (uint32_t)dt_range(0, 3);

    dtgpBegin(c, DTGP_DEBUG | DTGP_SOUND);
    switchStubs();
    dt_stub(0x0042ee40, NULL, (void *)stubHandOn);
    randomDefs();
    randomRecords();
    rec = &gPropRecords[dt_range(0, 15)];
    rec->object = propObject();
    if ((dt_u32() & 3) != 0) {
        rec->mode = dt_u32() & 1;
    }
    randomLinks(rec);
    randomSwitchDef(&gPropDefs[gCurrentLevel][rec->index]);
    gPhysRiders[player].vel.x = dt_float_edgy(-100.0f, 100.0f);
    gPhysRiders[player].vel.y = dt_float_edgy(-100.0f, 100.0f);
    gPhysRiders[player].vel.z = dt_float_edgy(-100.0f, 100.0f);
    DT_ARG_PTR(c, 0, rec);
    DT_ARG_INT(c, 1, player);
    DT_ARG_INT(c, 2, (dt_u32() & 1) != 0 ? 0 : ((dt_u32() & 3) == 0 ? dt_u32() : 1));
}

/* A rider near the prop (or far away), on it or not, in a state that is carried along or not. */
static void randomPushRider(PhysRider *r, const PropRecord *rec)
{
    static const uint8_t surfaces[] = {0xc, 0xc, 9, 9, 0, 3};
    const float range = (dt_u32() & 3) == 0 ? 2000.0f : 250.0f;

    dt_fill(r, sizeof *r);
    r->flags = dt_u32();
    if ((dt_u32() & 3) != 0) {
        r->flags |= 1;
    }
    r->player = (uint8_t)dt_range(0, 1);
    r->soundCooldown = (dt_u32() & 3) != 0 ? 0 : (uint8_t)dt_u32();
    r->state = (uint8_t)((dt_u32() & 1) != 0 ? 4 + 2 * dt_range(0, 1) : dt_range(0, 8));
    r->surface = surfaces[dt_u32() % sizeof surfaces];
    if (r->surface == 9) {
        PhysEdge *e = dt_alloc(sizeof *e);

        dt_fill(e, sizeof *e);
        e->prop = (dt_u32() & 3) != 0 ? rec->object : NULL;
        r->contact = e;
    } else {
        r->contact = (dt_u32() & 3) != 0 ? (void *)rec->object : NULL;
    }
    r->pos.x = rec->pos.x + dt_float(-range, range);
    r->pos.y = rec->pos.y + dt_float(-range * 0.5f, range * 0.5f);
    r->pos.z = rec->pos.z + dt_float(-range, range);
    r->step.x = (dt_u32() & 7) == 0 ? dt_float_edgy(-1e-4f, 1e-4f) : dt_float(-5.0f, 5.0f);
    r->step.y = (dt_u32() & 7) == 0 ? dt_float_edgy(-1e-4f, 1e-4f) : dt_float(-5.0f, 5.0f);
    r->step.z = (dt_u32() & 7) == 0 ? dt_float_edgy(-1e-4f, 1e-4f) : dt_float(-5.0f, 5.0f);
    r->up.y = dt_float(-1.0f, 1.0f);
    r->actor2 = dt_alloc(sizeof(ZRig));
}

static void setupUpdate(DtCtx *c)
{
    static const uint8_t timers[] = {0, 1, 1, 2, 5};

    dtgpBegin(c, DTGP_DEBUG | DTGP_SOUND);
    switchStubs();
    dt_stub(0x0042f0d0, NULL, (void *)stubSwitch);
    dt_stub(0x0042ed80, NULL, (void *)stubHandOn);
    dt_stub(0x0040ca60, NULL, (void *)stubGoal);
    dt_stub(0x00469530, NULL, (void *)stubCollideObject);
    dt_stub(0x00420cb0, NULL, (void *)stubSetState);
    dt_stub(0x00408a80, NULL, (void *)stubBurst);
    randomDefs();
    randomRecords();
    for (int i = 0; i < 16; i++) {
        PropRecord *r = &gPropRecords[i];
        PropDef *d = &gPropDefs[gCurrentLevel][i];

        randomSwitchDef(d);
        if ((dt_u32() & 1) != 0) {
            d->kind = (uint8_t)dt_range(8, 12);
        }
        r->object = (dt_u32() & 7) == 0 ? NULL : propObject();
        r->state = (dt_u32() & 7) == 0 ? (uint8_t)dt_u32() : (uint8_t)dt_range(1, 5);
        r->timer = (dt_u32() & 7) == 0 ? (dt_u32() & 1) - 1u : timers[dt_u32() % sizeof timers];
        r->pos.x = dt_float(-500.0f, 500.0f);
        r->pos.y = dt_float(-500.0f, 500.0f);
        r->pos.z = dt_float(-500.0f, 500.0f);
        r->paramA = (dt_u32() & 7) == 0 ? dt_u32() : (uint32_t)dt_range(0, 3);
        if ((dt_u32() & 1) != 0) {
            randomLinks(r);
        }
    }
    {
        const PropRecord *rec = &gPropRecords[dt_range(0, 15)];

        if (rec->object == NULL) {
            rec = &gPropRecords[0];
        }
        if (rec->object != NULL) {
            randomPushRider(&gPhysRiders[0], rec);
            randomPushRider(&gPhysRiders[1], rec);
        }
    }
    gPhysPlayerCount = (uint8_t)dt_range(0, 2);
    propShape[0] = dt_float(-1.0f, 1.0f);
    propShape[1] = dt_float(20.0f, 200.0f);
    propShape[2] = dt_float(20.0f, 200.0f);
    propShape[3] = dt_float(20.0f, 200.0f);
}

DT_CASES(prop_cases) = {
    {"propInit", 0x0042deb0, propInit, 0, DT_RET_VOID, setupInit, 800, 0},
    {"propFindNode", 0x0042de70, propFindNode, 1, DT_RET_INT, setupFindNode, 300, 0},
    {"propStopSounds", 0x0042e270, propStopSounds, 0, DT_RET_VOID, setupStopSounds, 300, 0},
    {"propUpdate", 0x0042e2f0, propUpdate, 0, DT_RET_VOID, setupUpdate, 1000, 0},
    {"propReturnZero", 0x0042eb50, propReturnZero, 0, DT_RET_INT, setupNone, 1, 0},
    {"propLoadState", 0x0042eb70, propLoadState, 1, DT_RET_VOID, setupLoad, 600, 0},
    {"propsReset", 0x0042ecf0, propsReset, 0, DT_RET_VOID, setupReset, 300, 0},
    {"propFind", 0x0042ed10, propFind, 1, DT_RET_INT, setupFind, 400, 0},
    {"propCollect", 0x0042ed80, propCollect, 2, DT_RET_VOID, setupCollect, 800, 0},
    {"propRelease", 0x0042ee40, propRelease, 2, DT_RET_VOID, setupCollect, 800, 0},
    {"propTouch", 0x0042eec0, propTouch, 2, DT_RET_INT, setupTouch, 800, 0},
    {"propHitFace", 0x0042ef60, propHitFace, 2, DT_RET_VOID, setupHitFace, 800, 0},
    {"propSwitch", 0x0042f0d0, propSwitch, 3, DT_RET_VOID, setupSwitch, 1500, 0},
    {"propApplyTurn", 0x0042f8f0, propApplyTurn, 1, DT_RET_VOID, setupRecordOnly, 400, 0},
    {"propOrient", 0x0042f930, propOrient, 1, DT_RET_VOID, setupRecordOnly, 1000, 0},
    {"propNop", 0x0042fab0, propNop, 0, DT_RET_VOID, setupNone, 1, 0},
    {"propUpdateMove", 0x0042fad0, propUpdateMove, 1, DT_RET_VOID, setupUpdateMove, 1000, 0},
    {"propSetShown", 0x0042fc00, propSetShown, 2, DT_RET_VOID, setupSetShown, 600, 0},
};
