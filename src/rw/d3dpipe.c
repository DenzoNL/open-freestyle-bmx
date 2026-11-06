/*
 * RenderWare 3.1's Direct3D 7 driver, the instancing pipelines of atomics and world sectors (lib/d3d7driver,
 * 0x4c8ba0..0x4cb000). They are Im3D pipelines (rpWorldPipelineInit / rpAtomicPipelineInit): the instance
 * stage caches the object's vertices (position, normal, prelit colour, texture coordinates; morph targets
 * interpolated) and triangles in a resource entry, the driver's Im3D transform stage (0x4f97c0) puts them
 * into camera space and the TL buffer, the colour stages light the camera vertices' float colours (one
 * routine per light type, called through the world's light walk) and convert them, and the render stage draws
 * mesh by mesh through the Im3D index lists and RwRenderStateSet. Nothing here talks to Direct3D.
 *
 * Float code follows the original's x87 code operation by operation and runs in its rounding mode (inside a
 * camera update: 24-bit precision, rounding down; rwD3DRoundBegin). The compiler assumes rounding to nearest:
 * where that lets it move a negation (x - y * -d into x + y * d), the operands are made opaque (negMul).
 */
#include "rw/rwclump.h"
#include "rw/rwd3d.h"
#include "rw/rwlight.h"
#include "rw/rwmath.h"
#include "rw/rwworld.h"

/*
 * The instanced data of an atomic: the work record, then the vertices and the triangles. A world sector's
 * record stops before the matrix (it is drawn in world space). The resource entry header is not counted in
 * the size asked for.
 */
#define kSectorWork     offsetof(RwD3DIm3DWork, combined)
#define kAtomicWorkData (sizeof(RwD3DIm3DWork) - sizeof(RwResEntry))
#define kSectorWorkData (kSectorWork - sizeof(RwResEntry))

/* A float the routines read from the driver's data (values in the original: 1/128, 1/255, 255, 1, 0, -1). */
#define K(addr) ORIG_VAR(addr, const float, rwD3DK##addr)
/* The prelit colour of vertices without one (opaque black), for atomics and for world sectors. */
#define rwD3DAtomicDefaultColor ORIG_VAR(0x005668a0, uint32_t, rwD3DAtomicDefaultColor)
#define rwD3DSectorDefaultColor ORIG_VAR(0x005668b0, uint32_t, rwD3DSectorDefaultColor)
/* The colour of meshes without a material. */
#define rwD3DNoMaterialColor ((const RwRGBA *)BMX_ORIG(0x0069b600))

/* RwRenderState values the render stage sets, and rwTEXTUREADDRESSBORDER. */
enum {
    kStateTextureRaster = 1,
    kStateTextureAddress = 2,
    kStateTextureFilter = 7,
    kStateVertexAlpha = 10,
    kStateBorderColor = 11,
    kAddressBorder = 4,
};

static int32_t floatBits(float f)
{
    return rwD3DBits(f);
}

/* ---- instancing ------------------------------------------------------------------------------------------
 */

/* Triangles in a mesh header: strips give count - 2 per mesh, lists count / 3. */
static uint32_t meshTriangles(const RpMeshHeader *mesh)
{
    if ((mesh->flags & 1) != 0) {
        return mesh->totalIndices - 2u * mesh->numMeshes;
    }
    return mesh->totalIndices / 3;
}

/* Writes the triangles of every mesh (strips unrolled with alternating winding) as three indices each. */
static void instanceTriangles(RwD3DIm3DWork *work, const RpMeshHeader *mesh)
{
    uint8_t *out = work->triangles + work->triangleOffset;
    const RpMesh *m = (const RpMesh *)((const uint8_t *)mesh + sizeof(RpMeshHeader) + mesh->firstMeshOffset);

    if ((mesh->flags & 1) != 0) {
        for (uint32_t n = mesh->numMeshes; n != 0; n--, m++) {
            const int32_t count = (int32_t)m->numIndices - 2;
            const uint16_t *indices = m->indices;
            for (int32_t i = 0; i < count; i++) {
                uint16_t *tri = (uint16_t *)out;
                tri[0] = indices[i + (i & 1)];
                tri[1] = indices[i + (~i & 1)];
                tri[2] = indices[i + 2];
                out += work->triangleStride;
            }
        }
        return;
    }
    for (uint32_t n = mesh->numMeshes; n != 0; n--, m++) {
        const uint16_t *indices = m->indices;
        for (uint32_t i = m->numIndices / 3; i != 0; i--, indices += 3) {
            uint16_t *tri = (uint16_t *)out;
            tri[0] = indices[0];
            tri[1] = indices[1];
            tri[2] = indices[2];
            out += work->triangleStride;
        }
    }
}

/* The Im3D level gets the instance's vertices; the buffers grow to them plus a quarter for clipping. */
static void instanceLevel(int32_t numVertices)
{
    rwD3DIm3DLevel->numVertices = numVertices;
    if ((uint32_t)(numVertices + rwD3DIm3DLevel->base + (numVertices >> 2)) > rwD3DBufferSize) {
        rwD3DIm3DGrow((uint32_t)((numVertices >> 2) + numVertices));
    }
}

/*
 * 0x4c8ba0 (EAX = atomic): the atomic's vertex positions (and normals) into its instance: from its start
 * morph target, or interpolated towards the end target by position / time. Out of range targets use the
 * first.
 */
BMX_ADDR(0x004c8ba0)
BMX_REGPARM(1) void rwD3DAtomicMorph(RpAtomic *atomic)
{
    const int32_t start = atomic->interpolator.startMorphTarget;
    const int32_t end = atomic->interpolator.endMorphTarget;
    const RpGeometry *geometry = atomic->geometry;
    const RwD3DIm3DWork *work = (const RwD3DIm3DWork *)atomic->repEntry;
    const int32_t numTargets = geometry->numMorphTargets;
    const uint32_t n = (uint32_t)geometry->numVertices;
    uint8_t *out = work->vertices;

    if (start == end || start >= numTargets || end >= numTargets) {
        const RpMorphTarget *target =
            start < numTargets && end < numTargets ? &geometry->morphTarget[start] : geometry->morphTarget;
        const RwV3d *verts = target->verts;
        const RwV3d *normals = target->normals;
        for (uint32_t i = n; i != 0; i--, out += work->stride) {
            RwD3DIm3DVertex *v = (RwD3DIm3DVertex *)out;
            __builtin_memcpy(&v->x, verts++, sizeof(RwV3d));
            if ((geometry->flags & rpGEOMETRYNORMALS) != 0) {
                __builtin_memcpy(&v->nx, normals++, sizeof(RwV3d));
            }
        }
        return;
    }

    const uint32_t mxcsr = rwD3DRoundBegin();
    const float scale = atomic->interpolator.position * atomic->interpolator.recipTime;
    const RpMorphTarget *from = &geometry->morphTarget[start];
    const RpMorphTarget *to = &geometry->morphTarget[end];
    const RwV3d *a = from->verts, *b = to->verts;
    const RwV3d *na = from->normals, *nb = to->normals;
    for (uint32_t i = n; i != 0; i--, out += work->stride, a++, b++) {
        RwD3DIm3DVertex *v = (RwD3DIm3DVertex *)out;
        v->x = (b->x - a->x) * scale + a->x;
        v->y = (b->y - a->y) * scale + a->y;
        v->z = (b->z - a->z) * scale + a->z;
        if (na != NULL) {
            v->nx = (nb->x - na->x) * scale + na->x;
            v->ny = (nb->y - na->y) * scale + na->y;
            v->nz = (nb->z - na->z) * scale + na->z;
            na++;
            nb++;
        }
    }
    rwD3DRoundEnd(mxcsr);
}

/*
 * 0x4c8e30 (instance stage of the atomic pipeline, rpWorldPipelineInit): reuses the atomic's instance while
 * the mesh and vertex layout are unchanged and refreshes what the geometry's lock bits say changed (8 texture
 * coordinates, 0x20 prelit colours, 1 triangles, 6 positions / normals); a new one refreshes everything. The
 * matrix is the frame's LTM times the camera's view matrix. NULL without geometry or memory.
 */
BMX_ADDR(0x004c8e30)
RwD3DIm3DWork *rwD3DAtomicInstance(RpAtomic *atomic)
{
    RpGeometry *geometry = atomic->geometry;
    RpMeshHeader *mesh;
    RwD3DIm3DWork *work;
    uint32_t textured, type, stride, numTriangles, changed;
    int32_t numVertices;

    if (geometry == NULL) {
        return NULL;
    }
    mesh = geometry->mesh;
    numVertices = geometry->numVertices;
    textured = geometry->flags & rpGEOMETRYTEXTURED;
    type = rpPipelineTypes[textured];
    stride = rpPipelineSizes[textured];
    numTriangles = meshTriangles(mesh);
    work = (RwD3DIm3DWork *)atomic->repEntry;
    if (work != NULL && (uint16_t)work->serialNum == mesh->serialNum && work->type == type &&
        (uint32_t)work->stride == stride) {
        changed = geometry->lockedSinceLastInst;
        RwResourcesUseResEntry(&work->entry);
    } else {
        if (work != NULL) {
            RwResourcesFreeResEntry(&work->entry);
        }
        if (RwResourcesAllocateResEntry(
                atomic, &atomic->repEntry,
                (int32_t)(numTriangles * 6 + kAtomicWorkData + (uint32_t)numVertices * stride),
                NULL) == NULL) {
            return NULL;
        }
        work = (RwD3DIm3DWork *)atomic->repEntry;
        work->numTriangles = numTriangles;
        work->triangleOffset = 0;
        work->triangles = (uint8_t *)work + (uint32_t)numVertices * stride + sizeof(RwD3DIm3DWork);
        work->triangleStride = 6;
        work->stride = (int32_t)stride;
        work->numVertices = numVertices;
        work->vertices = (uint8_t *)work + sizeof(RwD3DIm3DWork);
        work->matrix = &work->combined;
        work->mesh = mesh;
        work->type = rpPipelineTypes[textured];
        work->serialNum = mesh->serialNum;
        work->flags = RpGeometryGetFlags(geometry);
        changed = 0x3f;
    }
    if ((changed & 8) != 0 && (geometry->flags & rpGEOMETRYTEXTURED) != 0) {
        uint8_t *out = work->vertices;
        const RwTexCoords *uv = geometry->texCoords;
        for (uint32_t n = (uint32_t)numVertices; n != 0; n--, uv++, out += work->stride) {
            __builtin_memcpy(&((RwD3DIm3DVertex *)out)->u, uv, sizeof *uv);
        }
    }
    if ((changed & 0x20) != 0) {
        uint8_t *out = work->vertices;
        if ((geometry->flags & rpGEOMETRYPRELIT) != 0) {
            const uint32_t *color = geometry->preLitLum;
            for (uint32_t n = (uint32_t)numVertices; n != 0; n--, out += work->stride) {
                __builtin_memcpy(&((RwD3DIm3DVertex *)out)->r, color++, 4);
            }
        } else {
            const uint32_t color = rwD3DAtomicDefaultColor;
            for (uint32_t n = (uint32_t)numVertices; n != 0; n--, out += work->stride) {
                __builtin_memcpy(&((RwD3DIm3DVertex *)out)->r, &color, 4);
            }
        }
    }
    if ((changed & 1) != 0) {
        instanceTriangles(work, mesh);
    }
    if ((atomic->interpolator.flags & 1) != 0 || (changed & 6) != 0) {
        rwD3DAtomicMorph(atomic);
        atomic->interpolator.flags &= ~1u;
    }
    geometry->lockedSinceLastInst = 0;
    RwMatrixMultiply((RwMatrix *)work->matrix, RwFrameGetLTM(atomic->object.object.parent),
                     &((RwCamera *)RwEngineInstance->curCamera)->viewMatrix);
    rwD3DSetUVStage0(type);
    work->lit = 0;
    instanceLevel(numVertices);
    return (RwD3DIm3DWork *)atomic->repEntry;
}

/*
 * 0x4c92d0 (instance stage of the world sector pipeline, rpAtomicPipelineInit): a sector's instance is built
 * once (again when the mesh or the world's vertex layout changes); normals come packed as signed bytes. The
 * matrix is the camera's view matrix. NULL without triangles (and no instance yet) or memory.
 */
BMX_ADDR(0x004c92d0)
RwD3DIm3DWork *rwD3DSectorInstance(RpWorldSector *sector)
{
    const RpWorld *world = RwEngineInstance->curWorld;
    const int32_t numVertices = sector->numVertices;
    const uint32_t textured = world->object.flags & 4;
    const uint32_t stride = rpPipelineSizesB[textured];
    const uint32_t type = rpPipelineTypesB[textured];
    RpMeshHeader *mesh = sector->mesh;
    const uint32_t numTriangles = meshTriangles(mesh);
    RwD3DIm3DWork *work = (RwD3DIm3DWork *)sector->repEntry;

    if (numTriangles != 0 && work != NULL &&
        (work->serialNum != mesh->serialNum || work->type != type || (uint32_t)work->stride != stride)) {
        RwResourcesFreeResEntry(&work->entry);
    }
    if (sector->repEntry != NULL) {
        work = (RwD3DIm3DWork *)sector->repEntry;
        RwResourcesUseResEntry(&work->entry);
    } else {
        uint8_t *out;
        uint32_t flags;

        if (numTriangles == 0 || RwResourcesAllocateResEntry(sector, &sector->repEntry,
                                                             (int32_t)(numTriangles * 6 + kSectorWorkData +
                                                                       (uint32_t)numVertices * stride),
                                                             NULL) == NULL) {
            return NULL;
        }
        work = (RwD3DIm3DWork *)sector->repEntry;
        work->mesh = mesh;
        work->type = (uint16_t)type;
        work->triangles = (uint8_t *)work + (uint32_t)numVertices * stride + kSectorWork;
        work->triangleOffset = 0;
        work->numTriangles = numTriangles;
        work->triangleStride = 6;
        work->numVertices = numVertices;
        work->vertices = (uint8_t *)work + kSectorWork;
        work->stride = (int32_t)stride;
        work->serialNum = mesh->serialNum;
        work->flags = RpWorldGetFlags(world);
        instanceTriangles(work, mesh);
        flags = world->object.flags;
        out = work->vertices;
        if ((flags & rpGEOMETRYNORMALS) != 0) {
            const uint32_t mxcsr = rwD3DRoundBegin();
            const RwV3d *pos = sector->vertices;
            const int8_t *normal = (const int8_t *)sector->normals;
            for (uint32_t n = (uint32_t)numVertices; n != 0; n--, out += work->stride, normal += 4) {
                RwD3DIm3DVertex *v = (RwD3DIm3DVertex *)out;
                v->nx = (float)normal[0] * K(0x0069b530);
                v->ny = (float)normal[1] * K(0x0069b534);
                v->nz = (float)normal[2] * K(0x0069b538);
                __builtin_memcpy(&v->x, pos++, sizeof(RwV3d));
            }
            rwD3DRoundEnd(mxcsr);
        } else {
            const RwV3d *pos = sector->vertices;
            for (uint32_t n = (uint32_t)numVertices; n != 0; n--, out += work->stride) {
                __builtin_memcpy(&((RwD3DIm3DVertex *)out)->x, pos++, sizeof(RwV3d));
            }
        }
        out = work->vertices;
        if ((flags & 4) != 0) {
            const RwTexCoords *uv = sector->texCoords;
            for (uint32_t n = (uint32_t)numVertices; n != 0; n--, uv++, out += work->stride) {
                __builtin_memcpy(&((RwD3DIm3DVertex *)out)->u, uv, sizeof *uv);
            }
        }
        out = work->vertices;
        if ((flags & 8) != 0) {
            const uint32_t *color = sector->preLitLum;
            for (uint32_t n = (uint32_t)numVertices; n != 0; n--, out += work->stride) {
                __builtin_memcpy(&((RwD3DIm3DVertex *)out)->r, color++, 4);
            }
        } else {
            const uint32_t color = rwD3DSectorDefaultColor;
            for (uint32_t n = (uint32_t)numVertices; n != 0; n--, out += work->stride) {
                __builtin_memcpy(&((RwD3DIm3DVertex *)out)->r, &color, 4);
            }
        }
    }
    work->matrix = &((const RwCamera *)RwEngineInstance->curCamera)->viewMatrix;
    rwD3DSetUVStage0(type);
    instanceLevel(numVertices);
    work->lit = 0;
    return (RwD3DIm3DWork *)sector->repEntry;
}

/* ---- colours ---------------------------------------------------------------------------------------------
 */

/* 0x4c9790 (the world's light walk calls it for each light): marks the instance lit and runs the light's
 * routine.
 */
BMX_ADDR(0x004c9790)
void rwD3DApplyLight(RwD3DIm3DWork *work, RpLight *light, const RwMatrix *inverse, float scale,
                     const RwSurfaceProperties *surface)
{
    if (rwD3DIm3DLevel->andClip != 0) {
        return;
    }
    work->lit |= 1;
    ((RwD3DLightFunc)light->worldExtCallback)(work, light, inverse, scale, surface);
}

/* 0x4c9800 (stage 2): the prelit colours (RwRGBA) into the camera vertices' float colours. */
BMX_ADDR(0x004c9800)
void rwD3DPrelight(RwD3DIm3DWork *work)
{
    RwD3DCamVertex *cam = rwD3DCamBase;
    const uint8_t *src = work->vertices;

    if (rwD3DIm3DLevel->andClip != 0) {
        return;
    }
    for (uint32_t n = (uint32_t)work->numVertices; n != 0; n--, cam++, src += work->stride) {
        const RwD3DIm3DVertex *v = (const RwD3DIm3DVertex *)src;
        cam->r = (float)v->r;
        cam->g = (float)v->g;
        cam->b = (float)v->b;
        cam->a = (float)v->a;
    }
}

/* A lit colour channel above 255 (or negative: the bits compare above 255's as unsigned) is clamped. */
static void clampChannel(float *c)
{
    const uint32_t bits = (uint32_t)floatBits(*c);

    if (bits > 0x437f0000u) {
        *c = (int32_t)bits < 0 ? 0.0f : 255.0f;
    }
}

static uint32_t packColor(const RwD3DCamVertex *cam)
{
    const int32_t a = rwD3DFistp(cam->a), r = rwD3DFistp(cam->r);
    const int32_t g = rwD3DFistp(cam->g), b = rwD3DFistp(cam->b);

    return (uint32_t)a << 24 | (uint32_t)b | (uint32_t)g << 8 | (uint32_t)r << 16;
}

/*
 * 0x4c9880 (stage 5): clamps lit colours and converts them to the TL vertices' D3DCOLOR. Instances that
 * modulate by the material colour are only clamped: the render stage converts them per mesh.
 */
BMX_ADDR(0x004c9880)
void rwD3DLightColors(RwD3DIm3DWork *work)
{
    RwD3DCamVertex *cam = rwD3DCamBase;
    RwD3DTLVertex *tl;
    uint32_t mxcsr;

    if (rwD3DIm3DLevel->andClip != 0) {
        return;
    }
    if ((work->flags & 0x40) != 0) {
        if ((work->lit & 1) != 0) {
            for (uint32_t n = (uint32_t)work->numVertices; n != 0; n--, cam++) {
                clampChannel(&cam->r);
                clampChannel(&cam->g);
                clampChannel(&cam->b);
            }
        }
        return;
    }
    tl = rwD3DTLBase;
    mxcsr = rwD3DRoundBegin();
    for (uint32_t n = (uint32_t)work->numVertices; n != 0; n--, cam++, tl++) {
        if ((work->lit & 1) != 0) {
            clampChannel(&cam->r);
            clampChannel(&cam->g);
            clampChannel(&cam->b);
        }
        tl->color = packColor(cam);
    }
    rwD3DRoundEnd(mxcsr);
}

/* ---- the light routines (rpWorldExtLightSelectCallback)
 * ---------------------------------------------------- */

/* 0x4c9b20 (ambient): adds the light's colour times the surface's ambient factor to every vertex. */
BMX_ADDR(0x004c9b20)
void rwD3DLightAmbient(RwD3DIm3DWork *work, RpLight *light, const RwMatrix *inverse, float scale,
                       const RwSurfaceProperties *surface)
{
    const uint32_t mxcsr = rwD3DRoundBegin();
    RwD3DCamVertex *cam = rwD3DCamBase;
    uint32_t n = (uint32_t)work->numVertices;

    (void)inverse;
    (void)scale;
    if ((light->object.object.privateFlags & 1) != 0) {
        const float s = light->color[0] * surface->ambient * K(0x0069b544);
        for (; n != 0; n--, cam++) {
            const float k = K(0x0069b548) * s;
            cam->g = cam->g + k;
            cam->r = cam->r + k;
            cam->b = k + cam->b;
            cam->a = K(0x0069b54c) + cam->a;
        }
    } else {
        const float s = surface->ambient * K(0x0069b550);
        for (; n != 0; n--, cam++) {
            cam->r = light->color[0] * s + cam->r;
            cam->a = K(0x0069b554) + cam->a;
            cam->g = cam->g + light->color[1] * s;
            cam->b = cam->b + light->color[2] * s;
        }
    }
    rwD3DRoundEnd(mxcsr);
}

/* n . l of a vertex normal, summed z, y, x like the original. */
static float normalDot(const RwD3DIm3DVertex *v, const RwV3d *l)
{
    return (l->z * v->nz + v->ny * l->y) + v->nx * l->x;
}

/*
 * 0x4c9c20 (directional): the light shines along its frame's at vector (into object space by `inverse`, then
 * renormalised); vertices facing it get the colour times n . -at times the diffuse factor.
 */
BMX_ADDR(0x004c9c20)
void rwD3DLightDirectional(RwD3DIm3DWork *work, RpLight *light, const RwMatrix *inverse, float scale,
                           const RwSurfaceProperties *surface)
{
    const uint32_t mxcsr = rwD3DRoundBegin();
    RwD3DCamVertex *cam = rwD3DCamBase;
    const uint8_t *src = work->vertices;
    uint32_t n = (uint32_t)work->numVertices;
    RwFrame *frame = RpLightGetFrame(light);
    RwV3d dir;

    (void)scale;
    if (frame == NULL) {
        rwD3DRoundEnd(mxcsr);
        return;
    }
    dir = RwFrameGetLTM(frame)->at;
    if (inverse != NULL) {
        RwV3dTransformVectors(&dir, &dir, 1, inverse);
        _rwV3dNormalize(&dir, &dir);
    }
    dir.z = K(0x0069b560) * dir.z;
    dir.y = K(0x0069b55c) * dir.y;
    dir.x = K(0x0069b558) * dir.x;
    if ((light->object.object.privateFlags & 1) != 0) {
        const float s = light->color[0] * surface->diffuse * K(0x0069b564);
        for (; n != 0; n--, cam++, src += work->stride) {
            const float dot = normalDot((const RwD3DIm3DVertex *)src, &dir);
            if (floatBits(dot) > 0) {
                const float k = dot * s * K(0x0069b568);
                cam->a = K(0x0069b56c) + cam->a;
                cam->r = cam->r + k;
                cam->g = cam->g + k;
                cam->b = cam->b + k;
            }
        }
    } else {
        const float s = surface->diffuse * K(0x0069b570);
        for (; n != 0; n--, cam++, src += work->stride) {
            const float dot = normalDot((const RwD3DIm3DVertex *)src, &dir);
            if (floatBits(dot) > 0) {
                const float t = dot * s;
                cam->r = light->color[0] * t + cam->r;
                cam->a = K(0x0069b574) + cam->a;
                cam->g = cam->g + light->color[1] * t;
                cam->b = cam->b + t * light->color[2];
            }
        }
    }
    rwD3DRoundEnd(mxcsr);
}

/*
 * a * -b as the x87 computes it (FCHS, then FMUL): rounding down, that is not -(a * b), so neither the
 * negated factor nor the product may be visible to the optimiser, which would fold the negation into a
 * subtraction.
 */
static float negMul(float a, float b)
{
    float nb = -b;
    float product;

    BMX_FP_OPAQUE(nb);
    product = a * nb;
    BMX_FP_OPAQUE(product);
    return product;
}

/* A vertex seen from a local light: the offset to the light and n . offset. */
typedef struct LightRay {
    float dx, dy, dz;
    float dot;
} LightRay;

static LightRay lightRay(const RwD3DIm3DVertex *v, const RwV3d *pos)
{
    LightRay ray;

    ray.dz = pos->z - v->z;
    ray.dy = pos->y - v->y;
    ray.dx = pos->x - v->x;
    ray.dot = (v->nz * ray.dz + v->ny * ray.dy) + v->nx * ray.dx;
    return ray;
}

static float rayLength2(const LightRay *ray)
{
    return (ray->dz * ray->dz + ray->dy * ray->dy) + ray->dx * ray->dx;
}

/*
 * The cone test of the spot lights at distance d: the squared distance of the offset from the light's axis
 * (`dir`, pointing away from the light) against (d * cone)^2. Both are returned for the soft spot's falloff.
 */
static void coneDistance(const LightRay *ray, const RwV3d *dir, float d, float cone, float *off2,
                         float *edge2)
{
    const float wy = ray->dy - negMul(dir->y, d);
    const float wx = ray->dx - negMul(dir->x, d);
    const float wz = ray->dz - negMul(dir->z, d);
    const float dc = d * cone;

    *off2 = (wy * wy + wz * wz) + wx * wx;
    *edge2 = dc * dc;
}

/*
 * A local light's radius in object space (scaled by `scale` when there is an `inverse` matrix): its square
 * and the falloff offset `one` / radius (`one` is a constant of each routine, 1 in the original).
 */
static void localLight(const RpLight *light, const RwMatrix *inverse, float scale, float one, float *radius2,
                       float *recip)
{
    if (inverse != NULL) {
        const float r = light->radius * scale;
        *recip = one / r;
        *radius2 = r * r;
    } else {
        *recip = one / light->radius;
        *radius2 = light->radius * light->radius;
    }
}

/*
 * 0x4c9e20 (point): within the radius, vertices facing the light get the colour times n . offset times
 * (1 / distance - 1 / radius) times the diffuse factor.
 */
BMX_ADDR(0x004c9e20)
void rwD3DLightPoint(RwD3DIm3DWork *work, RpLight *light, const RwMatrix *inverse, float scale,
                     const RwSurfaceProperties *surface)
{
    const uint32_t mxcsr = rwD3DRoundBegin();
    RwD3DCamVertex *cam = rwD3DCamBase;
    const uint8_t *src = work->vertices;
    uint32_t n = (uint32_t)work->numVertices;
    RwV3d pos = RwFrameGetLTM(RpLightGetFrame(light))->pos;
    float radius2, recip;

    localLight(light, inverse, scale, inverse != NULL ? K(0x0069b57c) : K(0x0069b598), &radius2, &recip);
    if (inverse != NULL) {
        RwV3dTransformPoints(&pos, &pos, 1, inverse);
    }
    if ((light->object.object.privateFlags & 1) != 0) {
        const float s = light->color[0] * surface->diffuse * K(0x0069b580);
        for (; n != 0; n--, cam++, src += work->stride) {
            const LightRay ray = lightRay((const RwD3DIm3DVertex *)src, &pos);
            if (floatBits(ray.dot) <= 0) {
                continue;
            }
            const int32_t d2 = floatBits(rayLength2(&ray));
            if (d2 >= floatBits(radius2)) {
                continue;
            }
            const float invd = (float)_rwFastInvSqrt(d2);
            (void)_rwFastSqrt(d2);
            const float k = (invd - recip) * ray.dot * s * K(0x0069b584);
            cam->a = K(0x0069b588) + cam->a;
            cam->r = cam->r + k;
            cam->g = cam->g + k;
            cam->b = cam->b + k;
        }
    } else {
        const float s = surface->diffuse * K(0x0069b58c);
        for (; n != 0; n--, cam++, src += work->stride) {
            const LightRay ray = lightRay((const RwD3DIm3DVertex *)src, &pos);
            if (floatBits(ray.dot) <= 0) {
                continue;
            }
            const int32_t d2 = floatBits(rayLength2(&ray));
            if (d2 >= floatBits(radius2)) {
                continue;
            }
            const float invd = (float)_rwFastInvSqrt(d2);
            (void)_rwFastSqrt(d2);
            const float ts = (invd - recip) * ray.dot * s;
            cam->r = light->color[0] * ts + cam->r;
            cam->a = K(0x0069b590) + cam->a;
            cam->g = cam->g + light->color[1] * ts;
            cam->b = cam->b + ts * light->color[2];
        }
    }
    rwD3DRoundEnd(mxcsr);
}

/* Position and axis (normalised in object space) of a spot light; the radius terms as localLight. */
static void spotLight(const RpLight *light, const RwMatrix *inverse, float scale, float one, RwV3d *pos,
                      RwV3d *dir, float *radius2, float *recip)
{
    *pos = RwFrameGetLTM(RpLightGetFrame(light))->pos;
    *dir = RwFrameGetLTM(RpLightGetFrame(light))->at;
    localLight(light, inverse, scale, one, radius2, recip);
    if (inverse != NULL) {
        RwV3dTransformPoints(pos, pos, 1, inverse);
        RwV3dTransformVectors(dir, dir, 1, inverse);
        _rwV3dNormalize(dir, dir);
    }
}

/* 0x4ca130 (spot): a point light limited to the cone around its frame's at vector. */
BMX_ADDR(0x004ca130)
void rwD3DLightSpot(RwD3DIm3DWork *work, RpLight *light, const RwMatrix *inverse, float scale,
                    const RwSurfaceProperties *surface)
{
    const uint32_t mxcsr = rwD3DRoundBegin();
    RwD3DCamVertex *cam = rwD3DCamBase;
    const uint8_t *src = work->vertices;
    uint32_t n = (uint32_t)work->numVertices;
    RwV3d pos, dir;
    float radius2, recip;

    spotLight(light, inverse, scale, inverse != NULL ? K(0x0069b5a0) : K(0x0069b5bc), &pos, &dir, &radius2,
              &recip);
    if ((light->object.object.privateFlags & 1) != 0) {
        const float s = surface->diffuse * light->color[0] * K(0x0069b5a4);
        for (; n != 0; n--, cam++, src += work->stride) {
            const LightRay ray = lightRay((const RwD3DIm3DVertex *)src, &pos);
            float off2, edge2;
            if (floatBits(ray.dot) <= 0) {
                continue;
            }
            const int32_t d2 = floatBits(rayLength2(&ray));
            if (d2 >= floatBits(radius2)) {
                continue;
            }
            const float invd = (float)_rwFastInvSqrt(d2);
            coneDistance(&ray, &dir, (float)_rwFastSqrt(d2), light->coneAngle, &off2, &edge2);
            if (floatBits(off2) >= floatBits(edge2)) {
                continue;
            }
            const float k = (invd - recip) * ray.dot * s * K(0x0069b5a8);
            cam->a = K(0x0069b5ac) + cam->a;
            cam->b = cam->b + k;
            cam->r = cam->r + k;
            cam->g = cam->g + k;
        }
    } else {
        const float s = surface->diffuse * K(0x0069b5b0);
        for (; n != 0; n--, cam++, src += work->stride) {
            const LightRay ray = lightRay((const RwD3DIm3DVertex *)src, &pos);
            float off2, edge2;
            if (floatBits(ray.dot) <= 0) {
                continue;
            }
            const int32_t d2 = floatBits(rayLength2(&ray));
            if (d2 >= floatBits(radius2)) {
                continue;
            }
            const float invd = (float)_rwFastInvSqrt(d2);
            coneDistance(&ray, &dir, (float)_rwFastSqrt(d2), light->coneAngle, &off2, &edge2);
            if (floatBits(off2) >= floatBits(edge2)) {
                continue;
            }
            const float ts = (invd - recip) * ray.dot * s;
            cam->r = light->color[0] * ts + cam->r;
            cam->a = K(0x0069b5b4) + cam->a;
            cam->g = cam->g + light->color[1] * ts;
            cam->b = cam->b + ts * light->color[2];
        }
    }
    rwD3DRoundEnd(mxcsr);
}

/* 0x4ca5b0 (soft spot): the spot light fading out towards the cone's edge (1 - offset^2 / edge^2). */
BMX_ADDR(0x004ca5b0)
void rwD3DLightSoftSpot(RwD3DIm3DWork *work, RpLight *light, const RwMatrix *inverse, float scale,
                        const RwSurfaceProperties *surface)
{
    const uint32_t mxcsr = rwD3DRoundBegin();
    RwD3DCamVertex *cam = rwD3DCamBase;
    const uint8_t *src = work->vertices;
    uint32_t n = (uint32_t)work->numVertices;
    RwV3d pos, dir;
    float radius2, recip;

    spotLight(light, inverse, scale, inverse != NULL ? K(0x0069b5c4) : K(0x0069b5f0), &pos, &dir, &radius2,
              &recip);
    if ((light->object.object.privateFlags & 1) != 0) {
        const float s = surface->diffuse * light->color[0] * K(0x0069b5c8);
        for (; n != 0; n--, cam++, src += work->stride) {
            const LightRay ray = lightRay((const RwD3DIm3DVertex *)src, &pos);
            float off2, edge2;
            if (floatBits(ray.dot) <= 0) {
                continue;
            }
            const int32_t d2 = floatBits(rayLength2(&ray));
            if (d2 >= floatBits(radius2)) {
                continue;
            }
            const float invd = (float)_rwFastInvSqrt(d2);
            coneDistance(&ray, &dir, (float)_rwFastSqrt(d2), light->coneAngle, &off2, &edge2);
            if (floatBits(off2) >= floatBits(edge2)) {
                continue;
            }
            const float f = off2 / edge2;
            const float ts = (invd - recip) * ray.dot * s;
            const float k = (K(0x0069b5cc) - f) * ts * K(0x0069b5d4);
            cam->a = K(0x0069b5d8) + cam->a;
            cam->r = cam->r + k;
            cam->g = cam->g + k;
            cam->b = cam->b + k;
        }
    } else {
        const float s = surface->diffuse * K(0x0069b5dc);
        for (; n != 0; n--, cam++, src += work->stride) {
            const LightRay ray = lightRay((const RwD3DIm3DVertex *)src, &pos);
            float off2, edge2;
            if (floatBits(ray.dot) <= 0) {
                continue;
            }
            const int32_t d2 = floatBits(rayLength2(&ray));
            if (d2 >= floatBits(radius2)) {
                continue;
            }
            const float invd = (float)_rwFastInvSqrt(d2);
            coneDistance(&ray, &dir, (float)_rwFastSqrt(d2), light->coneAngle, &off2, &edge2);
            if (floatBits(off2) >= floatBits(edge2)) {
                continue;
            }
            const float f = off2 / edge2;
            const float hts = (invd - recip) * ray.dot * s * (K(0x0069b5e0) - f);
            cam->r = light->color[0] * hts + cam->r;
            cam->a = K(0x0069b5e8) + cam->a;
            cam->g = cam->g + light->color[1] * hts;
            cam->b = cam->b + hts * light->color[2];
        }
    }
    rwD3DRoundEnd(mxcsr);
}

/* ---- the render stage -------------------------------------------------------------------------------------
 */

/* 0x4caad0: a vertex's lit colour times the material colour (r, g, b scaled to 0..1, a 0..255) into its TL
 * vertex.
 */
BMX_ADDR(0x004caad0)
void rwD3DModulateColor(uint32_t index, const float *color)
{
    const uint32_t mxcsr = rwD3DRoundBegin();
    const uint32_t i = (uint16_t)index;
    const RwD3DCamVertex *cam = &rwD3DCamBase[i];
    const int32_t a = rwD3DFistp(color[3]);
    const int32_t r = rwD3DFistp(cam->r * color[0]);
    const int32_t g = rwD3DFistp(cam->g * color[1]);
    const int32_t b = rwD3DFistp(cam->b * color[2]);

    rwD3DTLBase[i].color = (uint32_t)a << 24 | (uint32_t)b | (uint32_t)g << 8 | (uint32_t)r << 16;
    rwD3DRoundEnd(mxcsr);
}

/* 0x4cab70: a vertex's lit colour into its TL vertex. */
BMX_ADDR(0x004cab70)
void rwD3DVertexColor(uint32_t index)
{
    const uint32_t mxcsr = rwD3DRoundBegin();
    const uint32_t i = (uint16_t)index;

    rwD3DTLBase[i].color = packColor(&rwD3DCamBase[i]);
    rwD3DRoundEnd(mxcsr);
}

/* Clears the bit per vertex (as many bytes as the bits' size needs). */
static void clearVertexBits(void)
{
    uint8_t *bits = (uint8_t *)rwD3DVertexBits;

    for (uint32_t n = (rwD3DVertexBitsSize + 0x1f) >> 3; n != 0; n--) {
        *bits++ = 0;
    }
}

/* Colours every vertex the mesh uses once (modulated by `modulate` when given). */
static void colorMeshVertices(const RpMesh *mesh, const float *modulate)
{
    const uint16_t *indices = mesh->indices;

    clearVertexBits();
    for (uint32_t n = mesh->numIndices; n != 0; n--, indices++) {
        const uint32_t index = *indices;
        const uint32_t bit = 1u << (index & 31);
        uint32_t *word = &rwD3DVertexBits[(int32_t)index >> 5];
        if ((*word & bit) == 0) {
            *word |= bit;
            if (modulate != NULL) {
                rwD3DModulateColor(index, modulate);
            } else {
                rwD3DVertexColor(index);
            }
        }
    }
}

/* One mesh's triangles through the culling, clipping and index functions of the driver. */
static void renderTriangles(const RwD3DIm3DWork *work, const uint8_t *triangles, uint32_t count)
{
    if (rwD3DIm3DLevel->orClip == 0) {
        for (; count != 0; count--, triangles += work->triangleStride) {
            const uint16_t *tri = (const uint16_t *)triangles;
            const int32_t a = tri[0], b = tri[1], c = tri[2];
            if (rwD3DTriCull(a, b, c) == 0) {
                rwD3DIndexTri(a, b, c);
            }
        }
        return;
    }
    for (; count != 0; count--, triangles += work->triangleStride) {
        const uint16_t *tri = (const uint16_t *)triangles;
        const int32_t a = tri[0], b = tri[1], c = tri[2];
        const uint8_t ca = rwD3DCamBase[a].clip, cb = rwD3DCamBase[b].clip, cc = rwD3DCamBase[c].clip;
        if ((cc & ca & cb) != 0) {
            continue;
        }
        if ((ca | cb | cc) == 0) {
            if (rwD3DTriCull(a, b, c) == 0) {
                rwD3DIndexTri(a, b, c);
            }
        } else if (rwD3DClipTriCull(a, b, c) == 0) {
            if ((uint32_t)(rwD3DIm3DLevel->numTL + 0x14 + rwD3DIm3DLevel->base) > rwD3DBufferSize) {
                rwD3DIm3DGrow((uint32_t)rwD3DIm3DLevel->numTL + 0x32);
            }
            rwD3DClipTri(a, b, c);
        }
    }
}

/*
 * 0x4cac00 (stage 6): draws the instance mesh by mesh: Gouraud shading, vertex alpha for translucent material
 * colours, the material's texture, filter and addressing (border colour = material colour), each used vertex
 * coloured once (modulated by the material colour when the instance asks for it), then the triangles.
 */
BMX_ADDR(0x004cac00)
void rwD3DRenderMeshes(RwD3DIm3DWork *work)
{
    const RpMeshHeader *header = work->mesh;
    const int32_t numMeshes = header->numMeshes;
    const RpMesh *mesh;
    uint32_t first = 0;
    int modulate = 0;

    if (rwD3DIm3DLevel->andClip != 0) {
        return;
    }
    if ((uint32_t)work->numVertices > rwD3DVertexBitsSize) {
        rwD3DVertexBitsAlloc(work->numVertices);
    }
    RwRenderStateSet(rwRENDERSTATESHADEMODE, rwSHADEMODEGOURAUD);
    rwIm3DGouraud = 1;
    if ((work->flags & 0x40) != 0) {
        modulate = 1;
    }
    mesh = (const RpMesh *)((const uint8_t *)header + sizeof(RpMeshHeader) + header->firstMeshOffset);
    for (int32_t m = 0; m < numMeshes; m++, mesh++) {
        const uint8_t *triangles =
            work->triangles + work->triangleOffset + first * (uint32_t)work->triangleStride;
        const RpMaterial *material = mesh->material;
        const RwRGBA *color = rwD3DNoMaterialColor;
        RwRaster *raster = NULL;
        int32_t filter = 0, address = 0;
        uint32_t count;

        count = (header->flags & 1) != 0 ? mesh->numIndices - 2 : mesh->numIndices / 3;
        if (material != NULL) {
            const RwTexture *texture = RpMaterialGetTexture(material);
            color = RpMaterialGetColor(material);
            if (texture != NULL) {
                raster = RwTextureGetRaster(texture);
                filter = RwTextureGetFilterMode(texture);
                address = RwTextureGetAddressing(texture);
            }
        }
        if (count * 3 + 0x14 + rwD3DTriIndices.count > rwD3DTriIndices.capacity) {
            rwD3DIndexListResize(&rwD3DTriIndices, count * 3 + 0x14 + rwD3DTriIndices.count);
        }
        if ((material->color.red & material->color.green & material->color.blue & material->color.alpha) ==
                0xff ||
            !modulate) {
            RwRenderStateSet(kStateVertexAlpha, 0);
            if (modulate == 1) {
                colorMeshVertices(mesh, NULL);
            }
        } else {
            float scaled[4];
            uint32_t mxcsr;

            RwRenderStateSet(kStateVertexAlpha, material->color.alpha == 0xff ? 0 : 1);
            mxcsr = rwD3DRoundBegin();
            scaled[0] = (float)material->color.red * K(0x0069b604);
            scaled[1] = (float)material->color.green * K(0x0069b608);
            scaled[2] = (float)material->color.blue * K(0x0069b60c);
            scaled[3] = (float)material->color.alpha;
            rwD3DRoundEnd(mxcsr);
            colorMeshVertices(mesh, scaled);
        }
        rwD3DIndexStart();
        if ((uint32_t)(rwD3DIm3DLevel->base + rwD3DIm3DLevel->numTL + (int32_t)count * 2) > rwD3DBufferSize) {
            rwD3DIm3DGrow(count * 2 + 0x14 + (uint32_t)rwD3DIm3DLevel->numTL);
        }
        RwRenderStateSet(kStateTextureRaster, (intptr_t)raster);
        if (raster != NULL) {
            RwRenderStateSet(kStateTextureFilter, filter);
            RwRenderStateSet(kStateTextureAddress, address);
            if (address == kAddressBorder) {
                RwRenderStateSet(kStateBorderColor,
                                 (int32_t)((uint32_t)color->alpha << 24 | (uint32_t)color->red << 16 |
                                           (uint32_t)color->green << 8 | color->blue));
            }
        }
        renderTriangles(work, triangles, count);
        rwD3DIndexFlush();
        first += count;
    }
}
