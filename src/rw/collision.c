/*
 * RenderWare 3.1 collision: lines, points, spheres, boxes and atomics against the world's triangles, the
 * atomics in the world and the triangles of one atomic.
 *
 * Arithmetic follows the original's x87 code: values kept in registers are doubles, values stored to memory
 * floats, sums in the original's order.
 */
#include "compat/crt.h"
#include "rw/rwcollision.h"

enum {
    kCollStackDepth = 64,     /* BSP walks (no overflow check in the original) */
    kCollTreeStackDepth = 16, /* polygon split trees */
    rwERR_INTERSECTIONTYPE = 1,
    rwERR_NEGATIVEBOX = 0,
};

static double rwMul(float a, float b)
{
    return (double)a * (double)b;
}

static int32_t rwBits(float f)
{
    int32_t bits;
    memcpy(&bits, &f, sizeof bits);
    return bits;
}

/* Float coordinate `offset` bytes into a vector (the BSP types are byte offsets of the axis). */
static float rwAxis(const void *v, int32_t offset)
{
    float f;
    memcpy(&f, (const uint8_t *)v + offset, sizeof f);
    return f;
}

static int32_t rwAxisBits(const void *v, int32_t offset)
{
    return rwBits(rwAxis(v, offset));
}

static void rwCollError(int32_t code)
{
    RwError error;
    error.pluginID = 2;
    error.errorCode = _rwerror(code);
    RwErrorSet(&error);
}

/* ((z*z' + y*y') + x*x') with the double operands of the original's dot products. */
static double rwDot(double ax, double ay, double az, double bx, double by, double bz)
{
    return ((az * bz) + (ay * by)) + (ax * bx);
}

static double rwDotV(const RwV3d *a, const RwV3d *b)
{
    return rwDot(a->x, a->y, a->z, b->x, b->y, b->z);
}

/* The engine's lighting frame number tags the atomics a query has visited. */
static uint16_t rwCollNextTag(void)
{
    return ++RwEngineInstance->lightFrame;
}

/* ---- atomics ---------------------------------------------------------------------------------------- */

/*
 * The atomic's bounding sphere in world space. A pending morph interpolation (interpolator flag 2) first
 * blends the morph targets' spheres; the world sphere is recomputed when that happened or the frame is dirty
 * (the radius scaled by the length of the LTM's at row).
 */
BMX_ADDR(0x004b1280)
RwSphere *RpAtomicGetWorldBoundingSphere(RpAtomic *atomic)
{
    RwFrame *frame = atomic->object.object.parent;
    RpInterpolator *interpolator = &atomic->interpolator;
    const RpGeometry *geometry = atomic->geometry;
    if ((interpolator->flags & 2) != 0 && geometry != NULL) {
        const int32_t start = interpolator->startMorphTarget;
        const int32_t end = interpolator->endMorphTarget;
        const int32_t count = geometry->numMorphTargets;
        if (start != end && start < count && end < count) {
            const double t = rwMul(interpolator->position, interpolator->recipTime);
            const RwSphere *a = &geometry->morphTarget[start].boundingSphere;
            const RwSphere *b = &geometry->morphTarget[end].boundingSphere;
            RwSphere *s = &atomic->boundingSphere;
            s->radius = (float)(((double)b->radius - (double)a->radius) * t + (double)a->radius);
            s->center.x = (float)((double)b->center.x - (double)a->center.x);
            s->center.y = (float)((double)b->center.y - (double)a->center.y);
            s->center.x = (float)((double)s->center.x * t);
            s->center.z = (float)((double)b->center.z - (double)a->center.z);
            s->center.y = (float)((double)s->center.y * t);
            s->center.z = (float)(t * (double)s->center.z);
            s->center.x = (float)((double)s->center.x + (double)a->center.x);
            s->center.y = (float)((double)s->center.y + (double)a->center.y);
            s->center.z = (float)((double)s->center.z + (double)a->center.z);
        } else if (start < count && end < count) {
            atomic->boundingSphere = geometry->morphTarget[start].boundingSphere;
        } else {
            atomic->boundingSphere = geometry->morphTarget[0].boundingSphere;
        }
        interpolator->flags &= ~2u;
        atomic->object.object.privateFlags |= 1;
    }
    if (RwFrameDirty(frame) || (atomic->object.object.privateFlags & 1) != 0) {
        const RwMatrix *ltm = RwFrameGetLTM(frame);
        RwV3dTransformPoints(&atomic->worldBoundingSphere.center, &atomic->boundingSphere.center, 1, ltm);
        const float scaleSquared = (float)((rwMul(ltm->at.z, ltm->at.z) + rwMul(ltm->at.y, ltm->at.y)) +
                                           rwMul(ltm->at.x, ltm->at.x));
        const double scale = _rwFastSqrt(rwBits(scaleSquared));
        atomic->object.object.privateFlags &= 0xfe;
        atomic->worldBoundingSphere.radius = (float)(scale * atomic->boundingSphere.radius);
    }
    return &atomic->worldBoundingSphere;
}

/* ---- triangle against the unit cube (Graphics Gems III, Voorhies) ------------------------------------- */

/* Outcodes of a point against the 12 edge bevel planes of the cube (x +- y > 1 etc.). */
BMX_ADDR(0x004bc3c0)
__attribute__((regparm(1))) uint32_t _rpCollBevel2d(const RwV3d *p)
{
    const double x = p->x, y = p->y, z = p->z;
    uint32_t outcode = 0;
    if (y + x > 1.0) {
        outcode = 1;
    }
    if (x - y > 1.0) {
        outcode |= 2;
    }
    if (y + -x > 1.0) {
        outcode |= 4;
    }
    if (-x - y > 1.0) {
        outcode |= 8;
    }
    if (z + x > 1.0) {
        outcode |= 0x10;
    }
    if (x - z > 1.0) {
        outcode |= 0x20;
    }
    if (z + -x > 1.0) {
        outcode |= 0x40;
    }
    if (-x - z > 1.0) {
        outcode |= 0x80;
    }
    if (z + y > 1.0) {
        outcode |= 0x100;
    }
    if (y - z > 1.0) {
        outcode |= 0x200;
    }
    if (z + -y > 1.0) {
        outcode |= 0x400;
    }
    if (-y - z > 1.0) {
        outcode |= 0x800;
    }
    return outcode;
}

/* Outcodes of a point against the 8 corner bevel planes (x +- y +- z > 1.5). */
BMX_ADDR(0x004bc4c0)
__attribute__((regparm(1))) uint32_t _rpCollBevel3d(const RwV3d *p)
{
    const double x = p->x, y = p->y, z = p->z;
    const double zy = z + y;
    uint32_t outcode = 0;
    if (zy + x > 1.5) {
        outcode = 1;
    }
    if ((y + x) - z > 1.5) {
        outcode |= 2;
    }
    const double xy = x - y;
    if (xy + z > 1.5) {
        outcode |= 4;
    }
    if (xy - z > 1.5) {
        outcode |= 8;
    }
    const double nx = -x;
    if (zy + nx > 1.5) {
        outcode |= 0x10;
    }
    if ((y + nx) - z > 1.5) {
        outcode |= 0x20;
    }
    const double nxy = nx - y;
    if (z + nxy > 1.5) {
        outcode |= 0x40;
    }
    if (nxy - z > 1.5) {
        outcode |= 0x80;
    }
    return outcode;
}

/* Face outcodes (1/2 x, 4/8 y, 0x10/0x20 z) of the point p1 + alpha * (p2 - p1), masked. */
__attribute__((used, noinline)) static uint32_t rpCollCheckPointImpl(const RwV3d *p1, const RwV3d *p2,
                                                                     uint32_t mask, double alpha)
{
    const double x = ((double)p2->x - (double)p1->x) * alpha + (double)p1->x;
    const double y = ((double)p2->y - (double)p1->y) * alpha + (double)p1->y;
    const double z = alpha * ((double)p2->z - (double)p1->z) + (double)p1->z;
    const uint32_t bx = !(0.5 >= x) ? 1 : (!(x >= -0.5) ? 2 : 0);
    const uint32_t by = !(0.5 >= y) ? 4 : (!(y >= -0.5) ? 8 : 0);
    const uint32_t bz = !(0.5 >= z) ? 0x10 : (!(z >= -0.5) ? 0x20 : 0);
    return (bz | by | bx) & mask;
}

/* The original's entry: EAX = p1, EDX = p2, ECX = mask and alpha in ST0 (popped). */
BMX_ADDR(0x004bc570)
__attribute__((naked)) uint32_t _rpCollCheckPoint(void)
{
    BMX_X86_ENTRY("subl $8, %esp\n\t"
                  "fstpl (%esp)\n\t"
                  "pushl %ecx\n\t"
                  "pushl %edx\n\t"
                  "pushl %eax\n\t"
                  "call _rpCollCheckPointImpl\n\t"
                  "addl $20, %esp\n\t"
                  "ret");
}

/* Where the segment crosses each face plane in `outcodes`, test the crossing point against the cube. */
BMX_ADDR(0x004bc630)
__attribute__((regparm(3))) int32_t _rpCollCheckLine(const RwV3d *p1, const RwV3d *p2, uint32_t outcodes)
{
    static const struct {
        uint32_t bit;
        int32_t axis;
        float plane;
        uint32_t mask;
    } kFaces[6] = {
        {1, 0, 0.5f, 0x3e},  {2, 0, -0.5f, 0x3d},   {4, 4, 0.5f, 0x3b},
        {8, 4, -0.5f, 0x37}, {0x10, 8, 0.5f, 0x2f}, {0x20, 8, -0.5f, 0x1f},
    };
    for (int i = 0; i < 6; i++) {
        if ((outcodes & kFaces[i].bit) != 0) {
            const double a = rwAxis(p1, kFaces[i].axis);
            const double alpha = ((double)kFaces[i].plane - a) / ((double)rwAxis(p2, kFaces[i].axis) - a);
            if (rpCollCheckPointImpl(p1, p2, kFaces[i].mask, alpha) == 0) {
                return 0;
            }
        }
    }
    return 1;
}

/* Sign code of a cross product component against +-epsilon, as the triangle test's three branches set it. */
static uint32_t rpCollSign(double x, double y, double z, int strict)
{
    const double eps = 0x1.a36e2ep-14;
    if (eps > x) {
        return 4;
    }
    if (x > -eps) {
        return 0x20;
    }
    if (strict ? eps > y : !(y >= eps)) {
        return 2;
    }
    if (y > -eps) {
        return 0x10;
    }
    if (strict ? eps > z : !(z >= eps)) {
        return 1;
    }
    if (z > -eps) {
        return 8;
    }
    return 0;
}

/* (a - b) * (c - d) - (e - f) * (g - h) on floats, in double. */
static double rpCollCross(float a, float b, float c, float d, float e, float f, float g, float h)
{
    return ((double)a - (double)b) * ((double)c - (double)d) -
           ((double)e - (double)f) * ((double)g - (double)h);
}

/*
 * Point in triangle (all in one plane): after a bounding box reject, the signs of the cross products of the
 * edges with the point; 1 when they share no sign, i.e. the point is outside.
 */
BMX_ADDR(0x004bc750)
__attribute__((regparm(3))) int32_t _rpCollPointOutsideTriangle(const RwV3d *p, const RwV3d *a,
                                                                const RwV3d *b, uint32_t unused0,
                                                                uint32_t unused1, uint32_t unused2,
                                                                const RwV3d *c)
{
    (void)unused0;
    (void)unused1;
    (void)unused2;
    float m;
    /* max x */
    if (b->x >= a->x || c->x >= a->x) {
        m = c->x >= b->x ? c->x : b->x;
    } else {
        m = a->x;
    }
    if (!(m >= p->x)) {
        return 1;
    }
    /* max y */
    if (b->y >= a->y || c->y >= a->y) {
        m = c->y >= b->y ? c->y : b->y;
    } else {
        m = a->y;
    }
    if (!(m >= p->y)) {
        return 1;
    }
    /* max z */
    {
        const float bc = b->z > c->z ? b->z : c->z;
        m = (a->z > b->z && a->z > c->z) ? a->z : bc;
    }
    if (p->z > m) {
        return 1;
    }
    /* min x */
    if (a->x >= b->x || a->x >= c->x) {
        m = b->x >= c->x ? c->x : b->x;
    } else {
        m = a->x;
    }
    if (m > p->x) {
        return 1;
    }
    /* min y */
    if (a->y >= b->y || !(c->y > a->y)) {
        m = !(c->y > b->y) ? c->y : b->y;
    } else {
        m = a->y;
    }
    if (m > p->y) {
        return 1;
    }
    /* min z */
    if (a->z >= b->z || a->z >= c->z) {
        m = b->z >= c->z ? c->z : b->z;
    } else {
        m = a->z;
    }
    if (!(p->z >= m)) {
        return 1;
    }
    const uint32_t s0 = rpCollSign(rpCollCross(a->z, p->z, a->y, b->y, a->y, p->y, a->z, b->z),
                                   rpCollCross(a->z, b->z, a->x, p->x, a->z, p->z, a->x, b->x),
                                   rpCollCross(a->y, p->y, a->x, b->x, a->x, p->x, a->y, b->y), 0);
    const uint32_t s1 = rpCollSign(rpCollCross(b->z, p->z, b->y, c->y, b->y, p->y, b->z, c->z),
                                   rpCollCross(b->x, p->x, b->z, c->z, b->z, p->z, b->x, c->x),
                                   rpCollCross(b->y, p->y, b->x, c->x, b->y, c->y, b->x, p->x), 0);
    const uint32_t s2 = rpCollSign(rpCollCross(c->z, p->z, c->y, a->y, c->y, p->y, c->z, a->z),
                                   rpCollCross(c->z, a->z, c->x, p->x, c->z, p->z, c->x, a->x),
                                   rpCollCross(c->x, a->x, c->y, p->y, c->x, p->x, c->y, a->y), 1);
    return (s0 & s1 & s2) == 0;
}

/* Face outcodes of a point against a box (sup / inf): 1 / 2 for x, 4 / 8 for y, 0x10 / 0x20 for z. */
static uint32_t rpCollBoxOutcode(const RwBBox *box, const RwV3d *v)
{
    const uint32_t bx = !(box->sup.x >= v->x) ? 1 : (!(v->x >= box->inf.x) ? 2 : 0);
    const uint32_t by = !(box->sup.y >= v->y) ? 4 : (!(v->y >= box->inf.y) ? 8 : 0);
    const uint32_t bz = !(box->sup.z >= v->z) ? 0x10 : (!(v->z >= box->inf.z) ? 0x20 : 0);
    return bz | by | bx;
}

/* |t| as the original takes it: negated when below zero. */
static double rpCollAbs(double t)
{
    return 0.0 > t ? -t : t;
}

/*
 * Triangle against an axis aligned box: trivial accept / reject by the face outcodes, then in the box's unit
 * cube space (matrix) the bevel outcodes, the edges against the faces and the four cube diagonals against the
 * triangle's plane. normal and distance are not written (the box query passes them on uninitialised).
 */
BMX_ADDR(0x004bff70)
__attribute__((regparm(3))) int32_t _rpCollTriangleBoxIntersect(const RwBBox *box, RwV3d *normal,
                                                                const RwV3d *v0, uint32_t unused0,
                                                                uint32_t unused1, uint32_t unused2,
                                                                const RwV3d *v1, const RwV3d *v2,
                                                                const float *distance, const RwMatrix *matrix)
{
    (void)normal;
    (void)unused0;
    (void)unused1;
    (void)unused2;
    (void)distance;
    uint32_t o0 = rpCollBoxOutcode(box, v0);
    if (o0 == 0) {
        return 1;
    }
    uint32_t o1 = rpCollBoxOutcode(box, v1);
    if (o1 == 0) {
        return 1;
    }
    uint32_t o2 = rpCollBoxOutcode(box, v2);
    if (o2 == 0) {
        return 1;
    }
    if ((o0 & o1 & o2) != 0) {
        return 0;
    }
    RwV3d v[3] = {*v0, *v1, *v2};
    RwV3dTransformPoints(v, v, 3, matrix);
    o0 |= _rpCollBevel2d(&v[0]) << 8;
    o1 |= _rpCollBevel2d(&v[1]) << 8;
    o2 |= _rpCollBevel2d(&v[2]) << 8;
    if ((o0 & o1 & o2) != 0) {
        return 0;
    }
    o0 |= _rpCollBevel3d(&v[0]) << 24;
    o1 |= _rpCollBevel3d(&v[1]) << 24;
    o2 |= _rpCollBevel3d(&v[2]) << 24;
    if ((o0 & o1 & o2) != 0) {
        return 0;
    }
    if ((o0 & o1) == 0 && _rpCollCheckLine(&v[0], &v[1], o1 | o0) == 0) {
        return 1;
    }
    if ((o2 & o0) == 0 && _rpCollCheckLine(&v[0], &v[2], o0 | o2) == 0) {
        return 1;
    }
    if ((o1 & o2) == 0 && _rpCollCheckLine(&v[1], &v[2], o1 | o2) == 0) {
        return 1;
    }
    const RwV3d *a = &v[0], *b = &v[1], *c = &v[2];
    RwV3d n;
    n.x = (float)rpCollCross(a->z, c->z, a->y, b->y, a->y, c->y, a->z, b->z);
    n.y = (float)rpCollCross(a->z, b->z, a->x, c->x, a->z, c->z, a->x, b->x);
    n.z = (float)rpCollCross(a->y, c->y, a->x, b->x, a->y, b->y, a->x, c->x);
    _rwV3dNormalize(&n, &n);
    const double d = rwDot(b->x, b->y, b->z, n.x, n.y, n.z);
    /* The diagonal (t, t, t): |t| from the register, the other three from the stored float. */
    const double t1 = d / (((double)n.z + (double)n.y) + (double)n.x);
    const RwV3d p1 = {(float)t1, (float)t1, (float)t1};
    if (!(rpCollAbs(t1) > 0.5) && _rpCollPointOutsideTriangle(&p1, a, b, 0, 0, 0, c) == 0) {
        return 1;
    }
    const double t2 = d / (((double)n.y + (double)n.x) - (double)n.z);
    const RwV3d p2 = {(float)t2, (float)t2, (float)-t2};
    if (!(rpCollAbs(p2.x) > 0.5) && _rpCollPointOutsideTriangle(&p2, a, b, 0, 0, 0, c) == 0) {
        return 1;
    }
    const double t3 = d / (((double)n.x - (double)n.y) + (double)n.z);
    const RwV3d p3 = {(float)t3, (float)-t3, (float)t3};
    if (!(rpCollAbs(p3.x) > 0.5) && _rpCollPointOutsideTriangle(&p3, a, b, 0, 0, 0, c) == 0) {
        return 1;
    }
    const double t4 = d / (((double)n.x - (double)n.y) - (double)n.z);
    const RwV3d p4 = {(float)t4, (float)-t4, (float)-t4};
    if (rpCollAbs(p4.x) > 0.5) {
        return 0;
    }
    return _rpCollPointOutsideTriangle(&p4, a, b, 0, 0, 0, c) == 0;
}

/* ---- sphere against triangle ------------------------------------------------------------------------ */

/* Whether all three corners (relative to the sphere) lie beyond the radius on one side of an axis. */
static int rpCollAxisSeparates(float a, float b, float c, float r)
{
    const int32_t signs = (rwBits(a) >> 31) + (rwBits(b) >> 31) + (rwBits(c) >> 31);
    if (signs == 0) {
        return (rwBits((float)((double)a - r)) | rwBits((float)((double)b - r)) |
                rwBits((float)((double)c - r))) >= 0;
    }
    if (signs == -3) {
        return (rwBits((float)((double)r + a)) & rwBits((float)((double)b + r)) &
                rwBits((float)((double)c + r))) < 0;
    }
    return 0;
}

/*
 * Whether the edge u -> v keeps the sphere off: the point of the edge's line closest to the centre is
 * outside the radius and on the far side of the third corner w (all relative to the centre).
 */
static int rpCollEdgeSeparates(const RwV3d *u, const RwV3d *v, const RwV3d *w, double r2)
{
    const double ex = (double)v->x - u->x, ey = (double)v->y - u->y, ez = (double)v->z - u->z;
    const double t = rwDot(u->x, u->y, u->z, ex, ey, ez) / rwDot(ex, ey, ez, ex, ey, ez);
    const double px = u->x - ex * t, py = u->y - ey * t, pz = u->z - ez * t;
    const double p2 = rwDot(px, py, pz, px, py, pz);
    return !(r2 >= p2) && rwDot(px, py, pz, w->x, w->y, w->z) > p2;
}

/*
 * Sphere against triangle: per axis trivial reject, then the triangle's plane (the unit normal is written to
 * `normal` and the negated plane distance of the centre to `distance`), the corners inside the sphere, the
 * Voronoi region of the nearest corner and the three edges.
 */
BMX_ADDR(0x004bf0b0)
__attribute__((regparm(3))) int32_t _rpCollSphereTriangleIntersect(const RwSphere *sphere, RwV3d *normal,
                                                                   const RwV3d *v0, uint32_t unused0,
                                                                   uint32_t unused1, uint32_t unused2,
                                                                   const RwV3d *v1, const RwV3d *v2,
                                                                   float *distance)
{
    (void)unused0;
    (void)unused1;
    (void)unused2;
    const RwV3d *c = &sphere->center;
    const float r = sphere->radius;
    RwV3d a, b, d;
    a.x = (float)((double)v0->x - c->x);
    a.y = (float)((double)v0->y - c->y);
    a.z = (float)((double)v0->z - c->z);
    b.x = (float)((double)v1->x - c->x);
    b.y = (float)((double)v1->y - c->y);
    b.z = (float)((double)v1->z - c->z);
    d.x = (float)((double)v2->x - c->x);
    d.y = (float)((double)v2->y - c->y);
    d.z = (float)((double)v2->z - c->z);
    if (rpCollAxisSeparates(a.x, b.x, d.x, r) || rpCollAxisSeparates(a.y, b.y, d.y, r) ||
        rpCollAxisSeparates(a.z, b.z, d.z, r)) {
        return 0;
    }
    normal->y = (float)rpCollCross(v2->x, v0->x, v1->z, v0->z, v2->z, v0->z, v1->x, v0->x);
    const double nz = rpCollCross(v1->x, v0->x, v2->y, v0->y, v2->x, v0->x, v1->y, v0->y);
    normal->z = (float)nz;
    normal->x = (float)rpCollCross(v2->z, v0->z, v1->y, v0->y, v1->z, v0->z, v2->y, v0->y);
    const double length2 = (nz * nz + rwMul(normal->y, normal->y)) + rwMul(normal->x, normal->x);
    if (!(0.0 < length2)) {
        return 0;
    }
    const double scale = _rwFastInvSqrt(rwBits((float)length2));
    const double nzScaled = normal->z * scale;
    normal->z = (float)nzScaled;
    normal->y = (float)(normal->y * scale);
    normal->x = (float)(scale * normal->x);
    const double dot = ((normal->y * (double)a.y) + nzScaled * a.z) + rwMul(normal->x, a.x);
    if (-(double)r > dot || r < dot) {
        return 0;
    }
    *distance = (float)-dot;
    const double r2 = rwMul(r, r);
    const double a2 = rwDotV(&a, &a);
    if (!(r2 < a2)) {
        return 1;
    }
    const double b2 = rwDotV(&b, &b);
    if (!(r2 < b2)) {
        return 1;
    }
    const double d2 = rwDotV(&d, &d);
    if (!(r2 < d2)) {
        return 1;
    }
    /* The corner nearest to the centre: is the centre in its Voronoi region? */
    if (a2 >= b2) {
        if (d2 >= b2) {
            if (rwDotV(&b, &a) > b2 && rwDotV(&b, &d) > b2) {
                return 0;
            }
        } else if (rwDotV(&d, &a) > d2 && rwDotV(&d, &b) > d2) {
            return 0;
        }
    } else if (d2 >= a2) {
        if (rwDotV(&a, &b) > a2 && rwDotV(&a, &d) > a2) {
            return 0;
        }
    } else if (rwDotV(&d, &a) > d2 && rwDotV(&d, &b) > d2) {
        return 0;
    }
    if (rpCollEdgeSeparates(&a, &b, &d, r2) || rpCollEdgeSeparates(&b, &d, &a, r2) ||
        rpCollEdgeSeparates(&d, &a, &b, r2)) {
        return 0;
    }
    return 1;
}

/* ---- world BSP walks -------------------------------------------------------------------------------- */

/* start - (end - start) * t: the point where the line crosses a plane (t from the signed distances). */
static void rpCollLinePoint(RwV3d *out, const RwLine *line, double t)
{
    out->x = (float)((double)line->start.x - ((double)line->end.x - (double)line->start.x) * t);
    out->y = (float)((double)line->start.y - ((double)line->end.y - (double)line->start.y) * t);
    out->z = (float)((double)line->start.z - t * ((double)line->end.z - (double)line->start.z));
}

static double rpCollRatio(float d0, float d1)
{
    return (double)d0 / ((double)d1 - (double)d0);
}

/*
 * Walks the BSP along the line: at each plane sector the line goes to the child(ren) whose region it
 * touches, clipped where it crosses the left or right plane (signs taken from the bits of the float
 * distances). Each world sector gets the clipped line in query->t.line; a NULL from sectorCallBack stops.
 */
BMX_ADDR(0x004bd400)
__attribute__((regparm(3))) RpWorld *
_rpCollWorldForAllLineSectors(RpWorld *world, const RwLine *line, RpSectorCallBack sectorCallBack,
                              uint32_t unused0, uint32_t unused1, uint32_t unused2,
                              RpIntersection *intersection, RpCollisionQuery *query)
{
    (void)unused0;
    (void)unused1;
    (void)unused2;
    RpSector *stack[kCollStackDepth];
    RwLine lines[kCollStackDepth];
    int32_t top = 0;
    stack[0] = NULL;
    lines[0] = *line;
    RpSector *sector = world->rootSector;
    RwLine cur = *line;
    for (;;) {
        if (sector->type < 0) {
            query->t.line = cur;
            if (sectorCallBack(intersection, (RpWorldSector *)sector, query) == NULL) {
                return world;
            }
            sector = stack[top];
            cur = lines[top];
            top--;
        } else {
            const RpPlaneSector *plane = (const RpPlaneSector *)sector;
            const float s = rwAxis(&cur.start, plane->type), e = rwAxis(&cur.end, plane->type);
            const float sRight = (float)((double)s - plane->rightValue);
            const float sLeft = (float)((double)s - plane->leftValue);
            const float eLeft = (float)((double)e - plane->leftValue);
            const float eRight = (float)((double)e - plane->rightValue);
            const int32_t sR = rwBits(sRight), sL = rwBits(sLeft), eL = rwBits(eLeft), eR = rwBits(eRight);
            if (sR < 0 && eR < 0) {
                sector = plane->leftSubTree;
                continue;
            }
            if (sL > 0 && eL > 0) {
                sector = plane->rightSubTree;
                continue;
            }
            top++;
            if (((sL ^ eL) & INT32_MIN) == 0 && ((sR ^ eR) & INT32_MIN) == 0) {
                /* In the overlap: both children get the whole line. */
                lines[top] = cur;
                if (sL < eL) {
                    stack[top] = plane->rightSubTree;
                    sector = plane->leftSubTree;
                } else {
                    stack[top] = plane->leftSubTree;
                    sector = plane->rightSubTree;
                }
            } else if (((sL ^ eL) & INT32_MIN) != 0 && sR > 0 && eR > 0) {
                /* Crosses the left plane only, all of it right of the right plane. */
                RwV3d p;
                rpCollLinePoint(&p, &cur, rpCollRatio(sLeft, eLeft));
                if (sL < 0) {
                    lines[top] = cur;
                    stack[top] = plane->rightSubTree;
                    sector = plane->leftSubTree;
                    cur.end = p;
                } else {
                    stack[top] = plane->leftSubTree;
                    sector = plane->rightSubTree;
                    lines[top].start = p;
                    lines[top].end = cur.end;
                }
            } else if (((sR ^ eR) & INT32_MIN) != 0 && sL < 0 && eL < 0) {
                /* Crosses the right plane only, all of it left of the left plane. */
                RwV3d p;
                rpCollLinePoint(&p, &cur, rpCollRatio(sRight, eRight));
                if (sR < 0) {
                    stack[top] = plane->rightSubTree;
                    sector = plane->leftSubTree;
                    lines[top].start = p;
                    lines[top].end = cur.end;
                } else {
                    lines[top] = cur;
                    stack[top] = plane->leftSubTree;
                    sector = plane->rightSubTree;
                    cur.end = p;
                }
            } else if (sL < 0) {
                /* Crosses both: the far part goes on the stack, the near part on. */
                stack[top] = plane->rightSubTree;
                sector = plane->leftSubTree;
                rpCollLinePoint(&lines[top].start, &cur, rpCollRatio(sRight, eRight));
                lines[top].end = cur.end;
                rpCollLinePoint(&cur.end, &cur, rpCollRatio(sLeft, eLeft));
            } else {
                stack[top] = plane->leftSubTree;
                sector = plane->rightSubTree;
                rpCollLinePoint(&lines[top].start, &cur, rpCollRatio(sLeft, eLeft));
                lines[top].end = cur.end;
                rpCollLinePoint(&cur.end, &cur, rpCollRatio(sRight, eRight));
            }
        }
        if (top < 0) {
            return world;
        }
    }
}

/* A float of a vector as an integer (the BSP walks compare coordinates by their bit patterns). */
static int32_t rpCollPlaneLeft(const RpPlaneSector *plane)
{
    return rwBits(plane->leftValue);
}

static int32_t rpCollPlaneRight(const RpPlaneSector *plane)
{
    return rwBits(plane->rightValue);
}

typedef RpWorldSector *(BMX_CDECL *RpCollSectorTest)(RpIntersection *intersection, RpWorldSector *sector,
                                                     RpCollisionQuery *query);

/*
 * Walks the BSP over the box inf..sup (float bit patterns compared as integers) and runs `test` on every
 * world sector it reaches until a test returns NULL.
 */
static void rpCollWalkBox(RpWorld *world, const RwV3d *inf, const RwV3d *sup, RpCollSectorTest test,
                          RpIntersection *intersection, RpCollisionQuery *query)
{
    RpSector *stack[kCollStackDepth];
    int32_t top = 0;
    stack[0] = NULL;
    RpSector *sector = world->rootSector;
    do {
        if (sector->type < 0) {
            if (test(intersection, (RpWorldSector *)sector, query) == NULL) {
                return;
            }
            sector = stack[top];
            top--;
        } else {
            const RpPlaneSector *plane = (const RpPlaneSector *)sector;
            if (rwAxisBits(inf, plane->type) < rpCollPlaneLeft(plane)) {
                if (rpCollPlaneRight(plane) < rwAxisBits(sup, plane->type)) {
                    stack[++top] = plane->rightSubTree;
                }
                sector = plane->leftSubTree;
            } else if (rpCollPlaneRight(plane) < rwAxisBits(sup, plane->type)) {
                sector = plane->rightSubTree;
            } else {
                sector = stack[top];
                top--;
            }
        }
    } while (top >= 0);
}

/* The box around a sphere; 1 (and an error) when a corner of it has a negative coordinate. */
static int rpCollSphereBox(const RwSphere *sphere, RwV3d *inf, RwV3d *sup)
{
    inf->x = (float)((double)sphere->center.x - sphere->radius);
    inf->y = (float)((double)sphere->center.y - sphere->radius);
    inf->z = (float)((double)sphere->center.z - sphere->radius);
    sup->x = (float)((double)sphere->center.x + sphere->radius);
    sup->y = (float)((double)sphere->center.y + sphere->radius);
    sup->z = (float)((double)sphere->center.z + sphere->radius);
    if ((rwBits(inf->x) | rwBits(inf->z) | rwBits(inf->y)) < 0) {
        rwCollError(rwERR_NEGATIVEBOX);
        return 1;
    }
    return 0;
}

static int rpCollNegativeBox(const RwBBox *box)
{
    if ((rwBits(box->inf.x) | rwBits(box->inf.z) | rwBits(box->inf.y)) < 0) {
        rwCollError(rwERR_NEGATIVEBOX);
        return 1;
    }
    return 0;
}

/* Runs `test` on the world sectors an atomic is tied to, with the atomic's world sphere (the atomic skipped).
 */
static void rpCollWalkAtomicSectors(RpAtomic *atomic, RpCollSectorTest test, RpIntersection *intersection,
                                    RpCollisionQuery *query)
{
    RwLLLink *head = &atomic->llWorldSectorsInAtomic;
    RwLLLink *link = head->next;
    while (link != head) {
        RpWorldSector *sector =
            ((RpTie *)((uint8_t *)link - offsetof(RpTie, lWorldSectorInAtomic)))->worldSector;
        /* A cursor after the current link survives the callback unlinking it. */
        RwLLLink cursor;
        cursor.prev = link;
        cursor.next = link->next;
        link->next->prev = &cursor;
        link->next = &cursor;
        const RpWorldSector *result = test(intersection, sector, query);
        cursor.prev->next = cursor.next;
        cursor.next->prev = cursor.prev;
        if (result == NULL) {
            return;
        }
        link = cursor.next;
    }
}

/* Point: the atomics whose spheres contain it (the walk goes on whatever the callback returns). */
BMX_ADDR(0x004be160)
__attribute__((regparm(3))) RpWorld *_rpCollWorldAtomicsPoint(RpWorld *world, const RwV3d *point,
                                                              void *callBack, uint32_t unused0,
                                                              uint32_t unused1, uint32_t unused2,
                                                              RpIntersection *intersection, void *data)
{
    (void)unused0;
    (void)unused1;
    (void)unused2;
    rwCollNextTag();
    RpCollisionQuery query;
    query.callBack = callBack;
    query.data = data;
    query.t.sphere.center = *point;
    query.t.sphere.radius = 0.0f;
    query.exclude = NULL;
    RpSector *stack[kCollStackDepth];
    int32_t top = 0;
    stack[0] = NULL;
    RpSector *sector = world->rootSector;
    do {
        if (sector->type < 0) {
            _rpCollSectorAtomicsSphere(intersection, (RpWorldSector *)sector, &query);
            sector = stack[top];
            top--;
        } else {
            const RpPlaneSector *plane = (const RpPlaneSector *)sector;
            const int32_t p = rwAxisBits(point, plane->type);
            if (rpCollPlaneLeft(plane) > p) {
                if (p > rpCollPlaneRight(plane)) {
                    stack[++top] = plane->rightSubTree;
                }
                sector = plane->leftSubTree;
            } else if (p > rpCollPlaneRight(plane)) {
                sector = plane->rightSubTree;
            } else {
                sector = stack[top];
                top--;
            }
        }
    } while (top >= 0);
    return world;
}

BMX_ADDR(0x004be260)
__attribute__((regparm(3))) RpWorld *_rpCollWorldAtomicsLine(RpWorld *world, const RwLine *line,
                                                             void *callBack, uint32_t unused0,
                                                             uint32_t unused1, uint32_t unused2,
                                                             RpIntersection *intersection, void *data)
{
    (void)unused0;
    (void)unused1;
    (void)unused2;
    rwCollNextTag();
    RpCollisionQuery query;
    query.callBack = callBack;
    query.data = data;
    query.exclude = NULL;
    _rpCollWorldForAllLineSectors(world, line, _rpCollSectorAtomicsLine, 0, 0, 0, intersection, &query);
    return world;
}

BMX_ADDR(0x004be2c0)
__attribute__((regparm(3))) RpWorld *_rpCollWorldAtomicsSphere(RpWorld *world, const RwSphere *sphere,
                                                               void *callBack, uint32_t unused0,
                                                               uint32_t unused1, uint32_t unused2,
                                                               RpIntersection *intersection, void *data)
{
    (void)unused0;
    (void)unused1;
    (void)unused2;
    rwCollNextTag();
    RpCollisionQuery query;
    query.callBack = callBack;
    query.data = data;
    query.t.sphere = *sphere;
    query.exclude = NULL;
    RwV3d inf, sup;
    if (rpCollSphereBox(sphere, &inf, &sup) == 0) {
        rpCollWalkBox(world, &inf, &sup, _rpCollSectorAtomicsSphere, intersection, &query);
    }
    return world;
}

BMX_ADDR(0x004c0b00)
__attribute__((regparm(3))) RpWorld *_rpCollWorldAtomicsBox(RpWorld *world, const RwBBox *box, void *callBack,
                                                            uint32_t unused0, uint32_t unused1,
                                                            uint32_t unused2, RpIntersection *intersection,
                                                            void *data)
{
    (void)unused0;
    (void)unused1;
    (void)unused2;
    rwCollNextTag();
    RpCollisionQuery query;
    query.callBack = callBack;
    query.data = data;
    query.t.box = *box;
    query.exclude = NULL;
    if (rpCollNegativeBox(box) == 0) {
        rpCollWalkBox(world, &box->inf, &box->sup, _rpCollSectorAtomicsBox, intersection, &query);
    }
    return world;
}

/* The atomics near another atomic: its world sphere against the atomics in the sectors it is tied to. */
BMX_ADDR(0x004be480)
__attribute__((regparm(3))) RpWorld *_rpCollWorldAtomicsAtomic(RpWorld *world, RpAtomic *atomic,
                                                               void *callBack, uint32_t unused0,
                                                               uint32_t unused1, uint32_t unused2,
                                                               RpIntersection *intersection, void *data)
{
    (void)unused0;
    (void)unused1;
    (void)unused2;
    rwCollNextTag();
    RpCollisionQuery query;
    query.callBack = callBack;
    query.data = data;
    query.exclude = atomic;
    query.t.sphere = *RpAtomicGetWorldBoundingSphere(atomic);
    rpCollWalkAtomicSectors(atomic, _rpCollSectorAtomicsSphere, intersection, &query);
    return world;
}

BMX_ADDR(0x004c05d0)
__attribute__((regparm(3))) RpWorld *_rpCollWorldTrianglesLine(RpWorld *world, const RwLine *line,
                                                               void *callBack, uint32_t unused0,
                                                               uint32_t unused1, uint32_t unused2,
                                                               RpIntersection *intersection, void *data)
{
    (void)unused0;
    (void)unused1;
    (void)unused2;
    rwCollNextTag();
    RpCollisionQuery query;
    query.callBack = callBack;
    query.data = data;
    query.t.line = *line;
    query.exclude = NULL;
    _rpCollWorldForAllLineSectors(world, line, _rpCollSectorTrianglesLine, 0, 0, 0, intersection, &query);
    return world;
}

BMX_ADDR(0x004c0650)
__attribute__((regparm(3))) RpWorld *_rpCollWorldTrianglesSphere(RpWorld *world, const RwSphere *sphere,
                                                                 void *callBack, uint32_t unused0,
                                                                 uint32_t unused1, uint32_t unused2,
                                                                 RpIntersection *intersection, void *data)
{
    (void)unused0;
    (void)unused1;
    (void)unused2;
    rwCollNextTag();
    RpCollisionQuery query;
    query.callBack = callBack;
    query.data = data;
    query.t.sphere = *sphere;
    query.exclude = NULL;
    RwV3d inf, sup;
    if (rpCollSphereBox(sphere, &inf, &sup) == 0) {
        rpCollWalkBox(world, &inf, &sup, _rpCollSectorTrianglesSphere, intersection, &query);
    }
    return world;
}

BMX_ADDR(0x004c0810)
__attribute__((regparm(3))) RpWorld *_rpCollWorldTrianglesBox(RpWorld *world, const RwBBox *box,
                                                              void *callBack, uint32_t unused0,
                                                              uint32_t unused1, uint32_t unused2,
                                                              RpIntersection *intersection, void *data)
{
    (void)unused0;
    (void)unused1;
    (void)unused2;
    rwCollNextTag();
    RpCollisionQuery query;
    query.callBack = callBack;
    query.data = data;
    query.t.box = *box;
    query.exclude = NULL;
    if (rpCollNegativeBox(box) == 0) {
        rpCollWalkBox(world, &box->inf, &box->sup, _rpCollSectorTrianglesBox, intersection, &query);
    }
    return world;
}

/* The world's triangles near an atomic: its world sphere against the sectors it is tied to. */
BMX_ADDR(0x004c0c60)
__attribute__((regparm(3))) RpWorld *_rpCollWorldTrianglesAtomic(RpWorld *world, RpAtomic *atomic,
                                                                 void *callBack, uint32_t unused0,
                                                                 uint32_t unused1, uint32_t unused2,
                                                                 RpIntersection *intersection, void *data)
{
    (void)unused0;
    (void)unused1;
    (void)unused2;
    rwCollNextTag();
    RpCollisionQuery query;
    query.callBack = callBack;
    query.data = data;
    query.exclude = atomic;
    query.t.sphere = *RpAtomicGetWorldBoundingSphere(atomic);
    rpCollWalkAtomicSectors(atomic, _rpCollSectorTrianglesSphere, intersection, &query);
    return world;
}

/* ---- per world sector tests ------------------------------------------------------------------------- */

/*
 * Runs `test` on the atomics in the sector not visited yet by this query (and not the excluded one), with a
 * cursor link after the current one so the callback may unlink atomics. NULL when a callback stopped.
 */
typedef int (*RpCollAtomicTest)(const RwSphere *sphere, const RpCollisionQuery *query, float *distance);

static RpWorldSector *rpCollForSectorAtomics(RpIntersection *intersection, RpWorldSector *sector,
                                             RpCollisionQuery *query, RpCollAtomicTest test)
{
    RpAtomic *exclude = query->exclude;
    RwLLLink *head = &sector->collAtomicsInWorldSector;
    RwLLLink *link = head->next;
    while (link != head) {
        RpAtomic *atomic = ((RpTie *)link)->apAtom;
        RwLLLink cursor;
        cursor.prev = link;
        cursor.next = link->next;
        link->next->prev = &cursor;
        link->next = &cursor;
        if (atomic != NULL && atomic != exclude && atomic->collisionTag != RwEngineInstance->lightFrame) {
            atomic->collisionTag = RwEngineInstance->lightFrame;
            float distance;
            if (test(RpAtomicGetWorldBoundingSphere(atomic), query, &distance)) {
                const RpIntersectionCallBackAtomic callBack = (RpIntersectionCallBackAtomic)query->callBack;
                if (callBack(intersection, sector, atomic, distance, query->data) == NULL) {
                    cursor.prev->next = cursor.next;
                    cursor.next->prev = cursor.prev;
                    return NULL;
                }
            }
        }
        cursor.prev->next = cursor.next;
        cursor.next->prev = cursor.prev;
        link = cursor.next;
    }
    return sector;
}

/* Spheres overlap: the distance is sqrt(d^2 / (r1 + r2)^2) from the engine's table. */
static int rpCollAtomicSphereTest(const RwSphere *s, const RpCollisionQuery *query, float *distance)
{
    const RwSphere *q = &query->t.sphere;
    const double r = (double)s->radius + q->radius;
    const double r2 = r * r;
    const double dx = (double)s->center.x - q->center.x, dy = (double)s->center.y - q->center.y,
                 dz = (double)s->center.z - q->center.z;
    const double d2 = ((dz * dz) + (dy * dy)) + (dx * dx);
    if (d2 >= r2) {
        return 0;
    }
    *distance = (float)_rwFastSqrt(rwBits((float)(d2 / r2)));
    return 1;
}

/* Sphere overlaps the box (by its bounding box); the distance is 0. */
static int rpCollAtomicBoxTest(const RwSphere *s, const RpCollisionQuery *query, float *distance)
{
    const RwBBox *box = &query->t.box;
    const float r = s->radius;
    if (!((double)s->center.x + r > box->inf.x) || (double)s->center.x - r >= box->sup.x ||
        !((double)r + s->center.y > box->inf.y) || (double)s->center.y - r >= box->sup.y ||
        !((double)r + s->center.z > box->inf.z) || (double)s->center.z - r >= box->sup.z) {
        return 0;
    }
    *distance = 0.0f;
    return 1;
}

/* a - b with b opaque to the optimiser (no rewrite as a + -b, which flips the sign of a NaN b). */
static double collSub(double a, double b)
{
    __asm__("" : "+x"(b));
    return a - b;
}

/*
 * Line against the bounding spheres of the atomics in the sector not visited yet by this query (the excluded
 * atomic is not skipped here). The distance handed to the callback is the projection of the centre on the
 * line as a fraction of its length; a line starting inside a sphere always hits it, else the projection must
 * be in (0, 1] and the line close enough to the centre. The cursor link after the current tie lets callbacks
 * unlink atomics. NULL when a callback stopped.
 */
BMX_ADDR(0x004bdde0)
RpWorldSector *_rpCollSectorAtomicsLine(RpIntersection *intersection, RpWorldSector *sector,
                                        RpCollisionQuery *query)
{
    const RwLine *line = &intersection->t.line;
    RwV3d dir;
    dir.x = (float)collSub(line->end.x, line->start.x);
    dir.y = (float)collSub(line->end.y, line->start.y);
    dir.z = (float)collSub(line->end.z, line->start.z);
    const double recipLength = _rwV3dNormalize(&dir, &dir);
    RwLLLink *head = &sector->collAtomicsInWorldSector;
    RwLLLink *link = head->next;
    while (link != head) {
        RpAtomic *atomic = ((RpTie *)link)->apAtom;
        RwLLLink cursor;
        cursor.next = link->next;
        cursor.prev = link;
        link->next->prev = &cursor;
        link->next = &cursor;
        if (atomic != NULL && atomic->collisionTag != RwEngineInstance->lightFrame) {
            atomic->collisionTag = RwEngineInstance->lightFrame;
            const RwSphere *s = RpAtomicGetWorldBoundingSphere(atomic);
            const double dz = collSub(s->center.z, line->start.z);
            const double dy = collSub(s->center.y, line->start.y);
            const double dx = collSub(s->center.x, line->start.x);
            const double radius2 = rwMul(s->radius, s->radius);
            const double distance2 = ((dz * dz) + (dy * dy)) + (dx * dx);
            const double along = ((dz * dir.z) + (dy * dir.y)) + (dx * dir.x);
            int hit = 0;
            double distance = 0.0;
            if (radius2 > distance2) {
                hit = 1;
                distance = along * recipLength;
            } else if (along > 0.0) {
                distance = along * recipLength;
                hit = distance <= 1.0 && collSub(radius2, collSub(distance2, along * along)) >= 0.0;
            }
            if (hit) {
                const RpIntersectionCallBackAtomic callBack = (RpIntersectionCallBackAtomic)query->callBack;
                if (callBack(intersection, sector, atomic, (float)distance, query->data) == NULL) {
                    cursor.prev->next = cursor.next;
                    cursor.next->prev = cursor.prev;
                    return NULL;
                }
            }
        }
        cursor.prev->next = cursor.next;
        cursor.next->prev = cursor.prev;
        link = cursor.next;
    }
    return sector;
}

BMX_ADDR(0x004be000)
RpWorldSector *_rpCollSectorAtomicsSphere(RpIntersection *intersection, RpWorldSector *sector,
                                          RpCollisionQuery *query)
{
    return rpCollForSectorAtomics(intersection, sector, query, rpCollAtomicSphereTest);
}

BMX_ADDR(0x004c0970)
RpWorldSector *_rpCollSectorAtomicsBox(RpIntersection *intersection, RpWorldSector *sector,
                                       RpCollisionQuery *query)
{
    return rpCollForSectorAtomics(intersection, sector, query, rpCollAtomicBoxTest);
}

/*
 * Walks the sector's polygon split tree over the box inf..sup (node n has children 2n, 2n + 1; a node with
 * bit 7 splits at a vertex along axis (flags >> 3) & 0xc and holds flags & 0x1f polygons, a leaf holds
 * `flags` polygons) and runs `test` on each polygon reached. NULL when a callback stopped.
 */
typedef int (*RpCollPolygonTest)(RpIntersection *intersection, RpWorldSector *sector, RpCollisionQuery *query,
                                 int32_t index, RwV3d *v0, RwV3d *v1, RwV3d *v2, const void *context);

static RpWorldSector *rpCollForSectorPolygons(RpIntersection *intersection, RpWorldSector *sector,
                                              RpCollisionQuery *query, const RwV3d *inf, const RwV3d *sup,
                                              RpCollPolygonTest test, const void *context)
{
    const uint8_t *tree = sector->colSectorRoot;
    int32_t stack[kCollTreeStackDepth];
    int32_t top = 0;
    int32_t node = 1;
    stack[0] = 0;
    do {
        const uint8_t *entry = tree + node * 3;
        uint32_t flags = entry[0];
        int32_t index = entry[2];
        if ((flags & 0x80) != 0) {
            const int32_t axis = (int32_t)(flags >> 3) & 0xc;
            const float split = rwAxis(&sector->vertices[entry[1]], axis);
            const float below = (float)((double)rwAxis(inf, axis) - split);
            const float above = (float)((double)split - rwAxis(sup, axis));
            flags &= 0x1f;
            if (rwBits(below) < 0) {
                if (rwBits(above) < 0) {
                    stack[++top] = node * 2 + 1;
                }
                node *= 2;
            } else if (rwBits(above) < 0) {
                node = node * 2 + 1;
            }
        } else {
            node = stack[top];
            top--;
        }
        for (uint32_t i = 0; i < flags; i++, index++) {
            const RpPolygon *polygon = &sector->polygons[index];
            RwV3d *vertices = sector->vertices;
            if (!test(intersection, sector, query, index, &vertices[polygon->vertIndex[0]],
                      &vertices[polygon->vertIndex[1]], &vertices[polygon->vertIndex[2]], context)) {
                return NULL;
            }
        }
    } while (top >= 0);
    return sector;
}

static void rpCollFillTriangle(RpCollisionTriangle *triangle, int32_t index, RwV3d *v0, RwV3d *v1, RwV3d *v2)
{
    triangle->point = *v0;
    triangle->index = index;
    triangle->vertices[0] = v0;
    triangle->vertices[1] = v1;
    triangle->vertices[2] = v2;
}

/* Sphere polygon test: the callback gets the plane distance in radii. 0 when it stopped the query. */
static int rpCollPolygonSphereTest(RpIntersection *intersection, RpWorldSector *sector,
                                   RpCollisionQuery *query, int32_t index, RwV3d *v0, RwV3d *v1, RwV3d *v2,
                                   const void *context)
{
    const double invRadius = *(const double *)context;
    RpCollisionTriangle triangle;
    float distance;
    if (_rpCollSphereTriangleIntersect(&query->t.sphere, &triangle.normal, v0, 0, 0, 0, v1, v2, &distance)) {
        rpCollFillTriangle(&triangle, index, v0, v1, v2);
        const RpIntersectionCallBackWorldTriangle callBack =
            (RpIntersectionCallBackWorldTriangle)query->callBack;
        if (callBack(intersection, sector, &triangle, (float)(invRadius * distance), query->data) == NULL) {
            return 0;
        }
    }
    return 1;
}

BMX_ADDR(0x004bed60)
RpWorldSector *_rpCollSectorTrianglesSphere(RpIntersection *intersection, RpWorldSector *sector,
                                            RpCollisionQuery *query)
{
    if (sector->colSectorRoot == NULL) {
        return sector;
    }
    const RwSphere *sphere = &query->t.sphere;
    RwV3d inf, sup;
    inf.x = (float)((double)sphere->center.x - sphere->radius);
    inf.y = (float)((double)sphere->center.y - sphere->radius);
    inf.z = (float)((double)sphere->center.z - sphere->radius);
    sup.x = (float)((double)sphere->center.x + sphere->radius);
    sup.y = (float)((double)sphere->center.y + sphere->radius);
    sup.z = (float)((double)sphere->center.z + sphere->radius);
    const double invRadius = 1.0 / sphere->radius;
    return rpCollForSectorPolygons(intersection, sector, query, &inf, &sup, rpCollPolygonSphereTest,
                                   &invRadius);
}

/* Box polygon test; the triangle's normal is left unset and the distance is 0, as in the original. */
static int rpCollPolygonBoxTest(RpIntersection *intersection, RpWorldSector *sector, RpCollisionQuery *query,
                                int32_t index, RwV3d *v0, RwV3d *v1, RwV3d *v2, const void *context)
{
    const RwMatrix *matrix = context;
    RpCollisionTriangle triangle;
    float distance = 0.0f;
    if (_rpCollTriangleBoxIntersect(&query->t.box, &triangle.normal, v0, 0, 0, 0, v1, v2, &distance,
                                    matrix)) {
        rpCollFillTriangle(&triangle, index, v0, v1, v2);
        const RpIntersectionCallBackWorldTriangle callBack =
            (RpIntersectionCallBackWorldTriangle)query->callBack;
        if (callBack(intersection, sector, &triangle, distance, query->data) == NULL) {
            return 0;
        }
    }
    return 1;
}

/*
 * The matrix taking the box to the unit cube around the origin: translate by minus the centre, then scale by
 * 1 / size. (The original rebuilds it at every tree node and leaves its padding words uninitialised.)
 */
static void rpCollBoxMatrix(RwMatrix *m, const RwBBox *box)
{
    RwV3d scale, translation;
    scale.x = (float)(1.0 / ((double)box->sup.x - box->inf.x));
    scale.y = (float)(1.0 / ((double)box->sup.y - box->inf.y));
    scale.z = (float)(1.0 / ((double)box->sup.z - box->inf.z));
    translation.x = (float)(-((double)box->inf.x + box->sup.x) * 0.5);
    translation.y = (float)(-((double)box->inf.y + box->sup.y) * 0.5);
    translation.z = (float)(-((double)box->inf.z + box->sup.z) * 0.5);
    memset(m, 0, sizeof *m);
    m->right.x = 1.0f;
    m->up.y = 1.0f;
    m->at.z = 1.0f;
    m->flags = 0x20003;
    RwMatrixTranslate(m, &translation, rwCOMBINEREPLACE);
    RwMatrixScale(m, &scale, rwCOMBINEPOSTCONCAT);
}

BMX_ADDR(0x004bfb20)
RpWorldSector *_rpCollSectorTrianglesBox(RpIntersection *intersection, RpWorldSector *sector,
                                         RpCollisionQuery *query)
{
    if (sector->colSectorRoot == NULL) {
        return sector;
    }
    const RwBBox *box = &query->t.box;
    RwMatrix matrix;
    rpCollBoxMatrix(&matrix, box);
    return rpCollForSectorPolygons(intersection, sector, query, &box->inf, &box->sup, rpCollPolygonBoxTest,
                                   &matrix);
}

/*
 * Line against a triangle (Moeller-Trumbore with a unit direction): 1 when the line from `start` along `dir`
 * meets the triangle a, b, c within `length`, with the hit in *distance as a fraction of the length.
 * *distance gets the unscaled distance already when that is in front of the start but beyond the length.
 */
__attribute__((used, noinline)) static int32_t rpCollLineTriangleImpl(const RwV3d *start, const RwV3d *dir,
                                                                      const RwV3d *a, const RwV3d *b,
                                                                      const RwV3d *c, float *distance,
                                                                      double length)
{
    const double e2y = (double)c->y - a->y, e2x = (double)c->x - a->x, e2z = (double)c->z - a->z;
    const double e1z = (double)b->z - a->z, e1y = (double)b->y - a->y, e1x = (double)b->x - a->x;
    const double pz = dir->x * e2y - dir->y * e2x;
    const double py = e2x * dir->z - dir->x * e2z;
    const double px = e2z * dir->y - e2y * dir->z;
    const double det = ((pz * e1z) + (py * e1y)) + (px * e1x);
    if (0x1.0c6f7ap-20 >= det) {
        return 0;
    }
    const double tz = (double)start->z - a->z, ty = (double)start->y - a->y, tx = (double)start->x - a->x;
    const double u = ((pz * tz) + (py * ty)) + (px * tx);
    if (0.0 > u || u > det) {
        return 0;
    }
    const double qz = tx * e1y - ty * e1x;
    const double qy = e1x * tz - tx * e1z;
    const double qx = e1z * ty - tz * e1y;
    const double v = dir->x * qx + (dir->z * qz + dir->y * qy);
    if (0.0 > v || v + u > det) {
        return 0;
    }
    const double t = ((e2z * qz) + (qy * e2y)) + (qx * e2x);
    const double limit = det * length;
    *distance = (float)t;
    if (0.0 > t || t > limit) {
        return 0;
    }
    *distance = (float)(t / limit);
    return 1;
}

/* The original's entry: EAX = start, EDX = dir, ECX = a, b / c / distance 0x10 / 0x14 / 0x18 bytes above the
 * return address's slot and the length in ST0 (popped). */
BMX_ADDR(0x004beab0)
__attribute__((naked)) int32_t _rpCollLineTriangleIntersect(uint32_t unused0, uint32_t unused1,
                                                            uint32_t unused2, uint32_t unused3,
                                                            const RwV3d *b, const RwV3d *c, float *distance)
{
    BMX_X86_ENTRY("subl $8, %esp\n\t"
                  "fstpl (%esp)\n\t"
                  "pushl 0x24(%esp)\n\t"
                  "pushl 0x24(%esp)\n\t"
                  "pushl 0x24(%esp)\n\t"
                  "pushl %ecx\n\t"
                  "pushl %edx\n\t"
                  "pushl %eax\n\t"
                  "call _rpCollLineTriangleImpl\n\t"
                  "addl $32, %esp\n\t"
                  "ret");
}

/*
 * Line against the sector's triangles: walks the polygon split tree with the line clipped to the sector
 * (query->t.line), cut again where it crosses a split plane (the far part waits on the stack with the node on
 * that side), and tests each polygon reached against the whole line of the intersection. Degenerate triangles
 * are skipped; the callback gets the unit normal and the hit as a fraction of the line's length.
 */
BMX_ADDR(0x004be560)
RpWorldSector *_rpCollSectorTrianglesLine(RpIntersection *intersection, RpWorldSector *sector,
                                          RpCollisionQuery *query)
{
    const uint8_t *tree = sector->colSectorRoot;
    if (tree == NULL) {
        return sector;
    }
    const RwLine *line = &intersection->t.line;
    RwV3d dir;
    dir.x = (float)((double)line->end.x - line->start.x);
    dir.y = (float)((double)line->end.y - line->start.y);
    dir.z = (float)((double)line->end.z - line->start.z);
    const double length = RwV3dNormalize(&dir, &dir);
    int32_t stack[kCollTreeStackDepth];
    RwLine lines[kCollTreeStackDepth];
    int32_t top = 0;
    int32_t node = 1;
    stack[0] = 0; /* the root's pop (a leaf root) reads them, then the walk ends */
    lines[0] = query->t.line;
    RwLine cur = query->t.line;
    do {
        const uint8_t *entry = tree + node * 3;
        uint32_t count = entry[0];
        int32_t index = entry[2];
        if ((count & 0x80) != 0) {
            const int32_t axis = (int32_t)(count >> 3) & 0xc;
            const float split = rwAxis(&sector->vertices[entry[1]], axis);
            const float d0 = (float)((double)rwAxis(&cur.start, axis) - split);
            const float d1 = (float)((double)rwAxis(&cur.end, axis) - split);
            count &= 0x1f;
            if (((rwBits(d0) ^ rwBits(d1)) & INT32_MIN) != 0) {
                RwV3d p;
                rpCollLinePoint(&p, &cur, rpCollRatio(d0, d1));
                top++;
                stack[top] = rwBits(d0) < 0 ? node * 2 + 1 : node * 2;
                lines[top].start = p;
                lines[top].end = cur.end;
                cur.end = p;
            }
            node = rwBits(d0) < 0 ? node * 2 : node * 2 + 1;
        } else {
            node = stack[top];
            cur = lines[top];
            top--;
        }
        for (uint32_t i = 0; i < count; i++, index++) {
            const RpPolygon *polygon = &sector->polygons[index];
            RwV3d *a = &sector->vertices[polygon->vertIndex[0]];
            RwV3d *b = &sector->vertices[polygon->vertIndex[1]];
            RwV3d *c = &sector->vertices[polygon->vertIndex[2]];
            RpCollisionTriangle triangle;
            triangle.normal.y = (float)rpCollCross(b->z, a->z, c->x, a->x, b->x, a->x, c->z, a->z);
            triangle.normal.x = (float)rpCollCross(c->z, a->z, b->y, a->y, b->z, a->z, c->y, a->y);
            const double nz = rpCollCross(c->y, a->y, b->x, a->x, c->x, a->x, b->y, a->y);
            triangle.normal.z = (float)nz;
            const double length2 = (nz * nz + rwMul(triangle.normal.y, triangle.normal.y)) +
                                   rwMul(triangle.normal.x, triangle.normal.x);
            if (0.0 >= length2) {
                continue;
            }
            float distance;
            if (!rpCollLineTriangleImpl(&line->start, &dir, a, b, c, &distance, length)) {
                continue;
            }
            const double scale = _rwFastInvSqrt(rwBits((float)length2));
            triangle.normal.z = (float)(triangle.normal.z * scale);
            triangle.normal.y = (float)(triangle.normal.y * scale);
            triangle.normal.x = (float)(scale * triangle.normal.x);
            rpCollFillTriangle(&triangle, index, a, b, c);
            const RpIntersectionCallBackWorldTriangle callBack =
                (RpIntersectionCallBackWorldTriangle)query->callBack;
            if (callBack(intersection, sector, &triangle, distance, query->data) == NULL) {
                return NULL;
            }
        }
    } while (top >= 0);
    return sector;
}

/* ---- one atomic's triangles ------------------------------------------------------------------------- */

/*
 * The vertices of the atomic's geometry the tests use: with a morph interpolation in progress the three
 * corners of each triangle are blended into `blend`, else they are the start target's (or target 0's).
 */
typedef struct RpCollMorph {
    const RwV3d *start;
    const RwV3d *end;
    double t;
} RpCollMorph;

static void rpCollMorphSetup(RpCollMorph *morph, const RpAtomic *atomic)
{
    const RpGeometry *geometry = atomic->geometry;
    const int32_t start = atomic->interpolator.startMorphTarget;
    const int32_t end = atomic->interpolator.endMorphTarget;
    const int32_t count = geometry->numMorphTargets;
    morph->t = 0.0;
    if (start != end && start < count && end < count) {
        morph->t = rwMul(atomic->interpolator.position, atomic->interpolator.recipTime);
        morph->start = geometry->morphTarget[start].verts;
        morph->end = geometry->morphTarget[end].verts;
    } else if (start < count && end < count) {
        morph->start = morph->end = geometry->morphTarget[start].verts;
    } else {
        morph->start = morph->end = geometry->morphTarget[0].verts;
    }
}

/* start + t * (end - start), each step rounded to float as the original stores it. */
static void rpCollMorphVertex(RwV3d *out, const RpCollMorph *morph, uint16_t index)
{
    const RwV3d *a = &morph->start[index];
    const RwV3d *b = &morph->end[index];
    out->x = (float)((double)b->x - a->x);
    out->y = (float)((double)b->y - a->y);
    out->z = (float)((double)b->z - a->z);
    out->y = (float)(morph->t * out->y);
    out->x = (float)(morph->t * out->x);
    out->z = (float)(morph->t * out->z);
    out->x = (float)((double)a->x + out->x);
    out->y = (float)((double)out->y + a->y);
    out->z = (float)((double)out->z + a->z);
}

static void rpCollMorphTriangle(const RpCollMorph *morph, const RpTriangle *triangle, RwV3d blend[3],
                                RwV3d *corner[3])
{
    if (morph->start == morph->end) {
        corner[0] = (RwV3d *)&morph->start[triangle->vertIndex[0]];
        corner[1] = (RwV3d *)&morph->start[triangle->vertIndex[1]];
        corner[2] = (RwV3d *)&morph->start[triangle->vertIndex[2]];
        return;
    }
    rpCollMorphVertex(&blend[0], morph, triangle->vertIndex[0]);
    rpCollMorphVertex(&blend[1], morph, triangle->vertIndex[1]);
    rpCollMorphVertex(&blend[2], morph, triangle->vertIndex[2]);
    corner[0] = &blend[0];
    corner[1] = &blend[1];
    corner[2] = &blend[2];
}

/* The inverse of the atomic's frame LTM (the original presets the flags word of its uninitialised matrix). */
static void rpCollAtomicInverse(RwMatrix *inverse, RpAtomic *atomic)
{
    memset(inverse, 0, sizeof *inverse);
    inverse->flags = 1;
    RwMatrixInvert(inverse, RwFrameGetLTM(atomic->object.object.parent));
}

/*
 * Line against the atomic's triangles in the atomic's space (Moeller-Trumbore with a unit direction); the
 * callback gets the distance along the line as a fraction of its length.
 */
BMX_ADDR(0x004c0d40)
__attribute__((regparm(3))) RpAtomic *
_rpCollAtomicTrianglesLine(RpAtomic *atomic, const RwLine *line,
                           RpIntersectionCallBackGeometryTriangle callBack, uint32_t unused0,
                           uint32_t unused1, uint32_t unused2, RpIntersection *intersection, void *data)
{
    (void)unused0;
    (void)unused1;
    (void)unused2;
    RwV3d blend[3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
    RwMatrix inverse;
    rpCollAtomicInverse(&inverse, atomic);
    RwV3d start, end, dir;
    RwV3dTransformPoints(&start, &line->start, 1, &inverse);
    RwV3dTransformPoints(&end, &line->end, 1, &inverse);
    dir.x = (float)((double)end.x - start.x);
    dir.y = (float)((double)end.y - start.y);
    dir.z = (float)((double)end.z - start.z);
    const double length = RwV3dNormalize(&dir, &dir);
    RpCollMorph morph;
    rpCollMorphSetup(&morph, atomic);
    const RpTriangle *triangle = ((const RpGeometry *)atomic->geometry)->triangles;
    for (int32_t index = 0; index < ((const RpGeometry *)atomic->geometry)->numTriangles;
         index++, triangle++) {
        RwV3d *corner[3];
        rpCollMorphTriangle(&morph, triangle, blend, corner);
        const RwV3d *v0 = corner[0], *v1 = corner[1], *v2 = corner[2];
        RpCollisionTriangle hit;
        hit.normal.y = (float)rpCollCross(v2->x, v0->x, v1->z, v0->z, v2->z, v0->z, v1->x, v0->x);
        const double nz = rpCollCross(v1->x, v0->x, v2->y, v0->y, v2->x, v0->x, v1->y, v0->y);
        hit.normal.z = (float)nz;
        hit.normal.x = (float)rpCollCross(v2->z, v0->z, v1->y, v0->y, v1->z, v0->z, v2->y, v0->y);
        const double length2 =
            (nz * nz + rwMul(hit.normal.y, hit.normal.y)) + rwMul(hit.normal.x, hit.normal.x);
        if (0.0 >= length2) {
            continue;
        }
        float ratio;
        if (!rpCollLineTriangleImpl(&start, &dir, v0, v1, v2, &ratio, length)) {
            continue;
        }
        const double scale = _rwFastInvSqrt(rwBits((float)length2));
        hit.normal.z = (float)(hit.normal.z * scale);
        hit.normal.y = (float)(hit.normal.y * scale);
        hit.normal.x = (float)(scale * hit.normal.x);
        rpCollFillTriangle(&hit, index, corner[0], corner[1], corner[2]);
        if (callBack(intersection, &hit, ratio, data) == NULL) {
            return atomic;
        }
    }
    return atomic;
}

/*
 * Sphere against the atomic's triangles: the sphere goes into the atomic's space (radius scaled by the length
 * of the inverse LTM's at row); the callback gets the plane distance in world radii.
 */
BMX_ADDR(0x004c1600)
__attribute__((regparm(3))) RpAtomic *
_rpCollAtomicTrianglesSphere(RpAtomic *atomic, const RwSphere *sphere,
                             RpIntersectionCallBackGeometryTriangle callBack, uint32_t unused0,
                             uint32_t unused1, uint32_t unused2, RpIntersection *intersection, void *data)
{
    (void)unused0;
    (void)unused1;
    (void)unused2;
    RwV3d blend[3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
    RwMatrix inverse;
    rpCollAtomicInverse(&inverse, atomic);
    RwSphere local;
    RwV3dTransformPoints(&local.center, &sphere->center, 1, &inverse);
    const float scale2 = (float)((rwMul(inverse.at.z, inverse.at.z) + rwMul(inverse.at.y, inverse.at.y)) +
                                 rwMul(inverse.at.x, inverse.at.x));
    local.radius = (float)(_rwFastSqrt(rwBits(scale2)) * sphere->radius);
    const double invRadius = _rwFastInvSqrt(rwBits(scale2)) / sphere->radius;
    RpCollMorph morph;
    rpCollMorphSetup(&morph, atomic);
    const RpTriangle *triangle = ((const RpGeometry *)atomic->geometry)->triangles;
    float distance = 0.0f;
    for (int32_t index = 0; index < ((const RpGeometry *)atomic->geometry)->numTriangles;
         index++, triangle++) {
        RwV3d *corner[3];
        rpCollMorphTriangle(&morph, triangle, blend, corner);
        RpCollisionTriangle hit;
        if (_rpCollSphereTriangleIntersect(&local, &hit.normal, corner[0], 0, 0, 0, corner[1], corner[2],
                                           &distance)) {
            rpCollFillTriangle(&hit, index, corner[0], corner[1], corner[2]);
            if (callBack(intersection, &hit, (float)(invRadius * distance), data) == NULL) {
                return atomic;
            }
        }
    }
    return atomic;
}

/* ---- entry points ----------------------------------------------------------------------------------- */

BMX_ADDR(0x004c1b10)
RpWorld *rwWorldIntersectTriangles(RpWorld *world, RpIntersection *intersection,
                                   RpIntersectionCallBackWorldTriangle callBack, void *data)
{
    switch (intersection->type) {
    case rpINTERSECTLINE:
        _rpCollWorldTrianglesLine(world, &intersection->t.line, callBack, 0, 0, 0, intersection, data);
        return world;
    case rpINTERSECTSPHERE:
        _rpCollWorldTrianglesSphere(world, &intersection->t.sphere, callBack, 0, 0, 0, intersection, data);
        return world;
    case rpINTERSECTBOX:
        _rpCollWorldTrianglesBox(world, &intersection->t.box, callBack, 0, 0, 0, intersection, data);
        return world;
    case rpINTERSECTATOMIC:
        _rpCollWorldTrianglesAtomic(world, intersection->t.object, callBack, 0, 0, 0, intersection, data);
        return world;
    default: /* points included */
        rwCollError(rwERR_INTERSECTIONTYPE);
        return NULL;
    }
}

BMX_ADDR(0x004c1c10)
RpWorld *rwWorldIntersectObjects(RpWorld *world, RpIntersection *intersection,
                                 RpIntersectionCallBackAtomic callBack, void *data)
{
    switch (intersection->type) {
    case rpINTERSECTPOINT:
        _rpCollWorldAtomicsPoint(world, &intersection->t.point, callBack, 0, 0, 0, intersection, data);
        return world;
    case rpINTERSECTLINE:
        _rpCollWorldAtomicsLine(world, &intersection->t.line, callBack, 0, 0, 0, intersection, data);
        return world;
    case rpINTERSECTSPHERE:
        _rpCollWorldAtomicsSphere(world, &intersection->t.sphere, callBack, 0, 0, 0, intersection, data);
        return world;
    case rpINTERSECTBOX:
        _rpCollWorldAtomicsBox(world, &intersection->t.box, callBack, 0, 0, 0, intersection, data);
        return world;
    case rpINTERSECTATOMIC:
        _rpCollWorldAtomicsAtomic(world, intersection->t.object, callBack, 0, 0, 0, intersection, data);
        return world;
    default:
        rwCollError(rwERR_INTERSECTIONTYPE);
        return NULL;
    }
}

/*
 * The atomic's triangles against a line or a sphere. For an atomic volume the original tests the volume
 * atomic's own triangles against its world sphere (not the given atomic's) and returns that atomic.
 */
BMX_ADDR(0x004c1f30)
RpAtomic *rwObjectIntersectTriangles(RpAtomic *atomic, RpIntersection *intersection,
                                     RpIntersectionCallBackGeometryTriangle callBack, void *data)
{
    switch (intersection->type) {
    case rpINTERSECTLINE:
        _rpCollAtomicTrianglesLine(atomic, &intersection->t.line, callBack, 0, 0, 0, intersection, data);
        return atomic;
    case rpINTERSECTSPHERE:
        _rpCollAtomicTrianglesSphere(atomic, &intersection->t.sphere, callBack, 0, 0, 0, intersection, data);
        return atomic;
    case rpINTERSECTATOMIC: {
        RpAtomic *other = intersection->t.object;
        const RwSphere *sphere = RpAtomicGetWorldBoundingSphere(other);
        _rpCollAtomicTrianglesSphere(other, sphere, callBack, 0, 0, 0, intersection, data);
        return other;
    }
    default:
        rwCollError(rwERR_INTERSECTIONTYPE);
        return NULL;
    }
}
