/*
 * Shared fixtures of the clump, geometry, material and world tests (rw/clump.c): an engine instance with the
 * module data of the clump, geometry, material, mesh, frame and world modules, free list entry points that
 * hand out blocks of 0x100 bytes (or fail on demand), scripted stream functions, logging stubs for the
 * plugin registry and for the callees that live elsewhere, and builders for frames, atomics, geometries,
 * materials and worlds.
 */
#ifndef DT_RWCLUMP_H
#define DT_RWCLUMP_H

#include "dt_rwtex.h"
#include "rw/rwclump.h"

#define CL_FN __attribute__((unused)) static

/* Module data after the texture/light fixture: clump (0xc), geometry, material, mesh, frame and world. */
enum {
    kClOffClump = kTexEngineBytes,
    kClOffGeometry = kClOffClump + 0x10,
    kClOffMaterial = kClOffGeometry + 0x10,
    kClOffMesh = kClOffMaterial + 0x10,
    kClOffFrame = kClOffMesh + 0x10,
    kClOffWorld = kClOffFrame + 0x10,
    kClEngineBytes = kClOffWorld + 0x20,
};

static int gClMaskWord =
    -1; /* a 0x1c byte write: this word is not logged (the original writes stack garbage) */
static int gClAllocCalls, gClAllocFailAt;
static int gClMallocCalls, gClMallocFailAt;
static uint32_t gClOldSize;

CL_FN void *BMX_CDECL clEntryAlloc(RwFreeList *list)
{
    rwtLog(0x4dbc30);
    rwtLogPtr(list);
    gClAllocCalls++;
    if (gClAllocFailAt != 0 && gClAllocCalls == gClAllocFailAt) {
        return NULL;
    }
    return dt_alloc(0x100);
}

CL_FN void BMX_CDECL clEntryFree(RwFreeList *list, void *entry)
{
    rwtLog(0x4dbd90);
    rwtLogPtr(list);
    rwtLogPtr(entry);
}

CL_FN void *BMX_CDECL clMalloc(size_t size)
{
    rwtLog(0x4a50d2);
    rwtLog((uint32_t)size);
    gClMallocCalls++;
    if (gClMallocFailAt != 0 && gClMallocCalls == gClMallocFailAt) {
        return NULL;
    }
    return dt_alloc(size < 0x40 ? 0x40 : (uint32_t)size);
}

CL_FN void BMX_CDECL clFree(void *block)
{
    rwtLog(0x4a4d7f);
    rwtLogPtr(block);
}

CL_FN void *BMX_CDECL clRealloc(void *block, size_t size)
{
    rwtLog(0x130);
    rwtLogPtr(block);
    rwtLog((uint32_t)size);
    gClMallocCalls++;
    if (gClMallocFailAt != 0 && gClMallocCalls == gClMallocFailAt) {
        return NULL;
    }
    uint8_t *fresh = dt_alloc(size < 0x40 ? 0x40 : (uint32_t)size);
    __builtin_memcpy(fresh, block, gClOldSize);
    return fresh;
}

/* The engine instance: texEnv's plus the module data, the free list ids and the module counters. */
CL_FN RwGlobals *clEnv(DtCtx *c)
{
    RwGlobals *base = texEnv(c);
    RwGlobals *g = dt_alloc(kClEngineBytes);
    __builtin_memcpy(g, base, kTexEngineBytes);
    RwEngineInstance = g;
    g->freeListAlloc = clEntryAlloc;
    g->freeListFree = clEntryFree;
    g->memoryFuncs.rwmalloc = clMalloc;
    g->memoryFuncs.rwfree = clFree;
    g->memoryFuncs.rwrealloc = clRealloc;
    rpClumpModuleOffset = kClOffClump;
    rpGeometryOffset = kClOffGeometry;
    rpMaterialOffset = kClOffMaterial;
    rpMeshOffset = kClOffMesh;
    rwFrameOffset = kClOffFrame;
    rpWorldGlobalsOffset = kClOffWorld;
    RpClumpGlobals *cg = (RpClumpGlobals *)((uint8_t *)g + kClOffClump);
    cg->atomicFreeList = texFreeListId();
    cg->clumpFreeList = texFreeListId();
    *(RwFreeList **)((uint8_t *)g + kClOffGeometry) = texFreeListId();
    *(RwFreeList **)((uint8_t *)g + kClOffMaterial) = texFreeListId();
    *(uint16_t *)((uint8_t *)g + kClOffMesh) = (uint16_t)dt_u32();
    *(RwFreeList **)((uint8_t *)g + kClOffMesh + 4) = texFreeListId();
    *(RwFreeList **)((uint8_t *)g + kClOffFrame) = texFreeListId();
    *(RwFreeList **)((uint8_t *)g + kClOffWorld + 4) = texFreeListId();
    RwLLLink *head = (RwLLLink *)((uint8_t *)g + kClOffWorld + 8);
    head->next = head->prev = head;
    g->dirtyFrameList.next = g->dirtyFrameList.prev = &g->dirtyFrameList;
    rpGeometryModuleCount = 1;
    rpWorldModuleCount = 1;
    gClMaskWord = -1;
    gClAllocCalls = gClAllocFailAt = 0;
    gClMallocCalls = gClMallocFailAt = 0;
    gClOldSize = 0x40;
    return g;
}

/* ---- logging stubs of callees outside this file ----------------------------------------------------- */

static int gClFailAt[8], gClFailCalls[8]; /* per stub kind: the n-th call returns the failure value */

enum { kClStubRead, kClStubWrite, kClStubMesh, kClStubOther };

CL_FN int clFails(int kind)
{
    gClFailCalls[kind]++;
    return gClFailAt[kind] != 0 && gClFailCalls[kind] == gClFailAt[kind];
}

CL_FN void *BMX_CDECL stInitObject(void *registry, void *object)
{
    rwtLog(0x4e8af0);
    rwtLogPtr(registry);
    rwtLogPtr(object);
    return registry;
}

CL_FN void *BMX_CDECL stDeInitObject(void *registry, void *object)
{
    rwtLog(0x4e8ab0);
    rwtLogPtr(registry);
    rwtLogPtr(object);
    return registry;
}

CL_FN void *BMX_CDECL stCopyObject(void *registry, void *dst, const void *src)
{
    rwtLog(0x4e8a60);
    rwtLogPtr(registry);
    rwtLogPtr(dst);
    rwtLogPtr(src);
    return registry;
}

CL_FN void *BMX_CDECL stReadChunks(void *registry, void *stream, void *object)
{
    rwtLog(0x4e8bf0);
    rwtLogPtr(registry);
    rwtLogPtr(stream);
    rwtLogPtr(object);
    return clFails(kClStubRead) ? NULL : registry;
}

CL_FN void *BMX_CDECL stWriteChunks(void *registry, void *stream, const void *object)
{
    rwtLog(0x4e8db0);
    rwtLogPtr(registry);
    rwtLogPtr(stream);
    rwtLogPtr(object);
    return clFails(kClStubWrite) ? NULL : registry;
}

CL_FN void *BMX_CDECL stSetFrame(void *object, void *frame)
{
    rwtLog(0x4e8740);
    rwtLogPtr(object);
    rwtLogPtr(frame);
    return object;
}

CL_FN void BMX_CDECL stReleaseFrame(void *object)
{
    rwtLog(0x4e87a0);
    rwtLogPtr(object);
}

CL_FN void *BMX_CDECL stUpdateObjects(void *frame)
{
    rwtLog(0x4d2a80);
    rwtLogPtr(frame);
    return frame;
}

CL_FN void *BMX_CDECL stGetLTM(void *frame)
{
    rwtLog(0x4d2b10);
    rwtLogPtr(frame);
    return frame;
}

CL_FN int BMX_CDECL stFreeResEntry(void *entry)
{
    rwtLog(0x4dc380);
    rwtLogPtr(entry);
    return 1;
}

CL_FN int BMX_CDECL stGeometryDestroy(void *geometry)
{
    rwtLog(0x4b5ab0);
    rwtLogPtr(geometry);
    return 1;
}

CL_FN void *BMX_CDECL stGeometryAddRef(void *geometry)
{
    rwtLog(0x4b5a30);
    rwtLogPtr(geometry);
    return geometry;
}

CL_FN int BMX_CDECL stTextureDestroy(void *texture)
{
    rwtLog(0x4df760);
    rwtLogPtr(texture);
    return 1;
}

CL_FN int BMX_CDECL stMaterialDestroy(void *material)
{
    rwtLog(0x4b9290);
    rwtLogPtr(material);
    return 1;
}

/* The plugin registry and frame callees of the object code, replaced by logging stubs. */
CL_FN void clRegistryStubs(void)
{
    dt_stub(0x004e8af0, NULL, (void *)stInitObject);
    dt_stub(0x004e8ab0, NULL, (void *)stDeInitObject);
    dt_stub(0x004e8a60, NULL, (void *)stCopyObject);
    dt_stub(0x004e8bf0, NULL, (void *)stReadChunks);
    dt_stub(0x004e8db0, NULL, (void *)stWriteChunks);
    for (int i = 0; i < 8; i++) {
        gClFailAt[i] = 0;
        gClFailCalls[i] = 0;
    }
}

CL_FN void clFrameStubs(void)
{
    dt_stub(0x004e8740, NULL, (void *)stSetFrame);
    dt_stub(0x004e87a0, NULL, (void *)stReleaseFrame);
    dt_stub(0x004d2a80, NULL, (void *)stUpdateObjects);
    dt_stub(0x004d2b10, NULL, (void *)stGetLTM);
}

/* ---- scripted stream -------------------------------------------------------------------------------- */

enum { kClStreamBytes = 4096 };
static uint8_t gClStream[kClStreamBytes];
static uint32_t gClStreamPos, gClStreamLen;
static int gClStreamCalls, gClStreamFailAt; /* the n-th Read/ReadReal/ReadInt32 returns failure */
static int gClFindCalls, gClFindFailAt;     /* the n-th FindChunk fails */
static uint32_t gClVersions[16];
static uint32_t gClLengths[16];
static uint32_t gClBadVersionAt;

CL_FN int BMX_CDECL stFindChunk(void *stream, uint32_t type, uint32_t *length, uint32_t *version)
{
    rwtLog(0x4e2ec0);
    rwtLogPtr(stream);
    rwtLog(type);
    rwtLog(length != NULL);
    const int index = gClFindCalls++;
    if (gClFindFailAt != 0 && gClFindCalls == gClFindFailAt) {
        return 0;
    }
    if (length != NULL) {
        *length = gClLengths[index < 15 ? index : 15];
    }
    if (version != NULL) {
        *version = gClBadVersionAt != 0 && gClFindCalls == (int)gClBadVersionAt
                       ? 0x304
                       : gClVersions[index < 15 ? index : 15];
    }
    return 1;
}

CL_FN uint32_t BMX_CDECL stStreamRead(void *stream, void *buffer, uint32_t length)
{
    rwtLog(0x4e2800);
    rwtLogPtr(stream);
    rwtLog(length);
    gClStreamCalls++;
    if (gClStreamFailAt != 0 && gClStreamCalls == gClStreamFailAt) {
        return length ^ 1u;
    }
    for (uint32_t i = 0; buffer != NULL && i < length; i++) {
        ((uint8_t *)buffer)[i] = gClStream[(gClStreamPos + i) % kClStreamBytes];
    }
    gClStreamPos += length;
    return length;
}

CL_FN void *BMX_CDECL stStreamReadWords(void *stream, void *buffer, uint32_t length)
{
    rwtLog(0x4e3200);
    rwtLogPtr(stream);
    rwtLog(length);
    gClStreamCalls++;
    if (gClStreamFailAt != 0 && gClStreamCalls == gClStreamFailAt) {
        return NULL;
    }
    for (uint32_t i = 0; buffer != NULL && i < length; i++) {
        ((uint8_t *)buffer)[i] = gClStream[(gClStreamPos + i) % kClStreamBytes];
    }
    gClStreamPos += length;
    return stream;
}

CL_FN void *BMX_CDECL stStreamWrite(void *stream, const void *buffer, uint32_t length)
{
    rwtLog(0x4e29d0);
    rwtLogPtr(stream);
    rwtLog(length);
    for (uint32_t i = 0; i < length && i < 0x80; i++) {
        if (gClMaskWord >= 0 && length == 0x1c && (int)(i / 4) == gClMaskWord) {
            continue;
        }
        rwtLog(((const uint8_t *)buffer)[i]);
    }
    return clFails(kClStubWrite) ? NULL : stream;
}

CL_FN void *BMX_CDECL stStreamWriteHeader(void *stream, uint32_t type, int32_t size, uint32_t version)
{
    rwtLog(0x4e2fd0);
    rwtLogPtr(stream);
    rwtLog(type);
    rwtLog((uint32_t)size);
    rwtLog(version);
    return clFails(kClStubWrite) ? NULL : stream;
}

CL_FN void *BMX_CDECL stStreamWriteWords(void *stream, const void *buffer, uint32_t length)
{
    rwtLog(0x4e3040);
    rwtLogPtr(stream);
    rwtLog(length);
    for (uint32_t i = 0; i < length && i < 0x80; i++) {
        rwtLog(((const uint8_t *)buffer)[i]);
    }
    return clFails(kClStubWrite) ? NULL : stream;
}

CL_FN int BMX_CDECL stReadChunkHeader(void *stream, uint32_t *type, uint32_t *length, uint32_t *version)
{
    rwtLog(0x4e2e20);
    rwtLogPtr(stream);
    const int index = gClFindCalls++;
    if (gClFindFailAt != 0 && gClFindCalls == gClFindFailAt) {
        return 0;
    }
    *type = gClLengths[(index * 2) & 15] % 4 == 0 ? 2 : (gClLengths[(index * 2) & 15] % 4 == 1 ? 0x13 : 5);
    *length = gClLengths[(index * 2 + 1) & 15] % 24;
    *version = gClBadVersionAt != 0 && gClFindCalls == (int)gClBadVersionAt ? 0x304 : gClVersions[index & 15];
    return 1;
}

CL_FN void *BMX_CDECL stStreamSkip(void *stream, uint32_t count)
{
    rwtLog(0x4e2c50);
    rwtLogPtr(stream);
    rwtLog(count);
    return clFails(kClStubOther) ? NULL : stream;
}

/*
 * Fills the stream script with random bytes (small counts and valid-looking indices mostly), picks the
 * versions FindChunk hands out (valid ones mostly) and the failures. `lengths` are what FindChunk reports.
 */
CL_FN void clStreamScript(void)
{
    dt_fill(gClStream, sizeof gClStream);
    for (int i = 0; i < 256; i += 4) {
        gClStream[i] = (uint8_t)(dt_u32() % 4);
        gClStream[i + 1] = gClStream[i + 2] = gClStream[i + 3] = 0;
    }
    gClStreamPos = 0;
    gClStreamCalls = 0;
    gClStreamFailAt = dt_u32() % 8 == 0 ? dt_range(1, 8) : 0;
    gClFindCalls = 0;
    gClFindFailAt = dt_u32() % 8 == 0 ? dt_range(1, 8) : 0;
    gClBadVersionAt = dt_u32() % 8 == 0 ? (uint32_t)dt_range(1, 8) : 0;
    for (int i = 0; i < 16; i++) {
        gClVersions[i] = 0x301 + dt_u32() % 3;
        gClLengths[i] = dt_u32() % 5 == 0 ? dt_u32() % 64 : 0x1c;
    }
    dt_stub(0x004e2ec0, NULL, (void *)stFindChunk);
    dt_stub(0x004e2800, NULL, (void *)stStreamRead);
    dt_stub(0x004e3200, NULL, (void *)stStreamReadWords);
    dt_stub(0x004e3270, NULL, (void *)stStreamReadWords);
}

CL_FN void clWriteStubs(void)
{
    dt_stub(0x004e29d0, NULL, (void *)stStreamWrite);
    dt_stub(0x004e2fd0, NULL, (void *)stStreamWriteHeader);
    dt_stub(0x004e3040, NULL, (void *)stStreamWriteWords);
    dt_stub(0x004e3130, NULL, (void *)stStreamWriteWords);
    gClFailAt[kClStubWrite] = dt_u32() % 8 == 0 ? dt_range(1, 12) : 0;
    gClFailCalls[kClStubWrite] = 0;
}

/* ---- builders --------------------------------------------------------------------------------------- */

CL_FN void clInsertLink(RwLLLink *head, RwLLLink *link)
{
    link->next = head;
    link->prev = head->prev;
    head->prev->next = link;
    head->prev = link;
}

/* A frame with an empty object list, random matrix and version; root = itself. */
CL_FN RwFrame *clFrame(void)
{
    RwFrame *f = dt_alloc(sizeof(RwFrame) + 8);
    dt_fill(f, sizeof(RwFrame));
    f->objectList.next = f->objectList.prev = &f->objectList;
    f->object.parent = NULL;
    f->object.privateFlags = (uint8_t)(dt_u32() % 2 ? 0 : (dt_u32() & 0xc));
    f->child = f->next = NULL;
    f->root = f;
    return f;
}

CL_FN void clAddChild(RwFrame *parent, RwFrame *child)
{
    child->next = parent->child;
    parent->child = child;
    child->object.parent = parent;
    child->root = parent->root;
}

/* A frame tree with up to `n` frames (random shape); returns the root. */
CL_FN RwFrame *clFrameTree(int n)
{
    RwFrame *frames[8];
    frames[0] = clFrame();
    for (int i = 1; i < n; i++) {
        frames[i] = clFrame();
        clAddChild(frames[dt_u32() % (uint32_t)i], frames[i]);
        frames[i]->root = frames[0];
    }
    return frames[0];
}

CL_FN RpGeometry *clGeometry(int numVertices, int numTriangles, uint32_t format, int numMaterials);

CL_FN RpAtomic *clAtomic(RwFrame *frame, int withGeometry)
{
    RpAtomic *a = dt_alloc(0x80);
    dt_fill(a, 0x70);
    a->object.object.type = 1;
    a->object.object.parent = frame;
    a->object.object.flags = (uint8_t)(dt_u32() % 2 ? 4 | (dt_u32() & 3) : dt_u32() & 3);
    a->repEntry = dt_u32() % 2 ? NULL : dt_alloc(0x20);
    a->geometry = withGeometry ? clGeometry(dt_range(0, 4), dt_range(0, 3), dt_u32() & 0x1f, 2) : NULL;
    a->clump = NULL;
    a->renderCallBack = NULL;
    a->interpolator.startMorphTarget = (int16_t)dt_u32();
    a->interpolator.endMorphTarget = (int16_t)dt_u32();
    a->llWorldSectorsInAtomic.next = a->llWorldSectorsInAtomic.prev = &a->llWorldSectorsInAtomic;
    return a;
}

CL_FN RpClump *clClump(int numAtomics, RwFrame *root)
{
    RpClump *c = dt_alloc(0x40);
    dt_fill(c, 0x20);
    c->object.type = 2;
    c->object.parent = root;
    c->atomicList.next = c->atomicList.prev = &c->atomicList;
    for (int i = 0; i < numAtomics; i++) {
        RpAtomic *a = clAtomic(root, dt_u32() % 4 != 0);
        a->clump = c;
        clInsertLink(&c->atomicList, &a->inClumpLink);
    }
    return c;
}

CL_FN RpMaterial *clMaterial(void)
{
    RpMaterial *m = dt_alloc(0x20);
    dt_fill(m, sizeof *m);
    m->texture = NULL;
    if (dt_u32() % 2) {
        m->texture = dt_alloc(0x30);
        m->texture->raster = mkRaster();
    }
    m->refCount = (uint16_t)(dt_u32() % 5 == 0 ? dt_u32() : (uint32_t)dt_range(0, 3));
    return m;
}

/* A material list of n entries (some shared) in `list`; its array has room for `space`. */
CL_FN void clMaterialList(RpMaterialList *list, int n)
{
    list->space = n + dt_range(0, 3);
    list->numMaterials = n;
    list->materials = (RpMaterial **)dt_alloc((uint32_t)(list->space + 1) * 4 + 0x40);
    for (int i = 0; i < n; i++) {
        list->materials[i] = i > 0 && dt_u32() % 3 == 0 ? list->materials[dt_range(0, i - 1)] : clMaterial();
    }
}

CL_FN RpGeometry *clGeometry(int numVertices, int numTriangles, uint32_t format, int numMaterials)
{
    RpGeometry *g = dt_alloc(0x80);
    dt_fill(g, 0x48);
    g->type = 8;
    g->flags = (uint8_t)format;
    g->refCount = (uint16_t)(dt_u32() % 4 == 0 ? 0 : dt_range(1, 3));
    g->numVertices = numVertices;
    g->numTriangles = numTriangles;
    g->numMorphTargets = dt_range(0, 3);
    g->mesh = NULL;
    g->preLitLum = dt_u32() % 2 ? NULL : dt_alloc(0x40);
    g->texCoords = dt_u32() % 2 ? NULL : dt_alloc(0x40);
    g->triangles = dt_alloc((uint32_t)numTriangles * 8 + 0x40);
    for (int i = 0; i < numTriangles; i++) {
        g->triangles[i].vertIndex[0] = (uint16_t)dt_range(0, 5);
        g->triangles[i].vertIndex[1] = (uint16_t)dt_range(0, 5);
        g->triangles[i].vertIndex[2] = (uint16_t)dt_range(0, 5);
        g->triangles[i].matIndex =
            (int16_t)(numMaterials == 0 || dt_u32() % 6 == 0 ? -1 : dt_range(0, numMaterials));
    }
    clMaterialList(&g->matList, numMaterials);
    g->morphTarget = dt_alloc((uint32_t)(g->numMorphTargets + 1) * 0x1c + 0x40);
    for (int i = 0; i < g->numMorphTargets; i++) {
        RpMorphTarget *m = &g->morphTarget[i];
        dt_fill(m, sizeof *m);
        m->parentGeom = g;
        m->verts = dt_u32() % 4 == 0 ? NULL : dt_alloc(0x40 + (uint32_t)numVertices * 24);
        m->normals = dt_u32() % 2 ? NULL : (ZVec3 *)((uint8_t *)m->verts + (uint32_t)numVertices * 12);
        if (m->verts == NULL) {
            m->normals = NULL;
        }
    }
    return g;
}

#endif
