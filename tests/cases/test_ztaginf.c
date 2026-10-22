/*
 * Differential tests of ztaginf.c. Memory management and the IFF reader are stubbed: allocations come from a
 * bump arena (both sides see the same addresses) and the chunk reader hands out scripted random records, so
 * the loader sees trees, triangle records (with NaNs, -1 pointers and bad sizes) and every chunk failure.
 */
#include "dt.h"
#include "dt_gameplay.h"
#include "zge/zgemods/ztaginf.h"

#define ARENA_SIZE 0x10000

static uint8_t *arena;
static uint32_t arenaTop;
static struct {
    int32_t budget;     /* internal nodes left */
    int32_t tagfRemain; /* 'TAGF' chunks left after the current one */
    uint32_t faceCount;
} script;

static void *arenaAlloc(uint32_t n)
{
    void *p = arena + arenaTop;

    arenaTop += (n + 15u) & ~15u;
    if (arenaTop > ARENA_SIZE - 0x400) {
        arenaTop = 0;
    }
    return p;
}

static void *BMX_CDECL stubAlloc(uint32_t size)
{
    dtgpLog(0xa1100000u);
    dtgpLog(size);
    return arenaAlloc(size);
}

static void BMX_CDECL stubFree(void *p)
{
    dtgpLog(0xf4ee0000u);
    dtgpLogP(p);
}

static void *BMX_CDECL stubNodeAlloc(void *list)
{
    dtgpLog(0xa1100001u);
    dtgpLogP(list);
    return arenaAlloc(sizeof(ZtaginfNode));
}

static void BMX_CDECL stubNodeFree(void *list, void *p)
{
    dtgpLog(0xf4ee0001u);
    dtgpLogP(list);
    dtgpLogP(p);
}

static void *BMX_CDECL stubListCreate(uint32_t size, uint32_t count, void *destroy)
{
    dtgpLog(0xc4ea0000u);
    dtgpLog(size);
    dtgpLog(count);
    dtgpLogP(destroy);
    return arenaAlloc(0x40);
}

static uint32_t wordValue(void)
{
    static const uint32_t edge[] = {0xffffffffu, 0,           0x7f800001u, 0x7fa00000u,
                                    0xff800001u, 0x00000001u, 0x80000000u};

    return (dt_u32() & 3) == 0 ? edge[dt_u32() % (sizeof edge / sizeof edge[0])] : dt_u32();
}

static int BMX_CDECL stubIffRead(void *iff, void *dst, uint32_t size)
{
    uint32_t *w = dst;

    dtgpLog(0x1ff4ead0u);
    dtgpLogP(iff);
    dtgpLog(size);
    for (uint32_t i = 0; i < size / 4; i++) {
        w[i] = (size == 0x50 || size == 0x28) ? wordValue() : dt_u32() % (script.faceCount + 3);
    }
    if (size == 0x28) {
        static const uint8_t kinds[] = {1, 1, 0, 2, 3};
        const uint8_t kind = script.budget <= 0 ? 1 : kinds[dt_u32() % sizeof kinds];

        if (kind != 1) {
            script.budget--;
        }
        ((uint8_t *)dst)[4] = kind;
        w[9] = (dt_u32() & 0xffff0000u) | (dt_u32() % 5);
    } else if (size == 0x50) {
        w[0] = (dt_u32() & 7) == 0 ? 0x4f : 0x50;
    } else if (size == 8) {
        w[0] = (dt_u32() & 15) == 0 ? 7 : 8;
        w[1] = dt_u32() % 6;
        script.faceCount = w[1];
    }
    return (int)size;
}

static int BMX_CDECL stubFindChunk(void *iff, uint32_t id)
{
    dtgpLog(0xf14dc000u);
    dtgpLog(id);
    return (dt_u32() & 15) != 0;
}

static int BMX_CDECL stubNextChunk(void *iff, uint32_t id)
{
    dtgpLog(0x4e87c000u);
    dtgpLog(id);
    if (id == 0x46474154u) {
        return script.tagfRemain-- > 0;
    }
    return (dt_u32() & 7) == 0;
}

static uint32_t BMX_CDECL stubChunkSize(void *iff)
{
    dtgpLog(0xc5120000u);
    return 8 + ((dt_u32() & 3) == 0 ? 0 : dt_u32() % 64);
}

static int BMX_CDECL stubBeginModel(void *iff)
{
    dtgpLog(0xbe6a0000u);
    return (dt_u32() & 7) != 0;
}

static int BMX_CDECL stubEndModel(void *iff)
{
    dtgpLog(0xe4d00000u);
    return 1;
}

static void begin(const DtCtx *c)
{
    dtgpBegin(c, DTGP_DEBUG);
    arena = dt_alloc(ARENA_SIZE);
    arenaTop = 0;
    script.budget = dt_range(0, 4);
    script.tagfRemain = dt_range(0, 5);
    script.faceCount = 3;
    dt_stub(0x00485aa0, NULL, (void *)stubAlloc);
    dt_stub(0x00485b20, NULL, (void *)stubFree);
    dt_stub(0x00486050, NULL, (void *)stubNodeAlloc);
    dt_stub(0x00486170, NULL, (void *)stubNodeFree);
    dt_stub(0x00485e80, NULL, (void *)stubListCreate);
}

static void stubIff(void)
{
    dt_stub(0x0049ac30, NULL, (void *)stubIffRead);
    dt_stub(0x0049ad90, NULL, (void *)stubFindChunk);
    dt_stub(0x0049aed0, NULL, (void *)stubNextChunk);
    dt_stub(0x0049abc0, NULL, (void *)stubChunkSize);
    dt_stub(0x0049b130, NULL, (void *)stubBeginModel);
    dt_stub(0x0049b250, NULL, (void *)stubEndModel);
}

/* A random tree of arena nodes (about half of the pointers null). */
static ZtaginfNode *randomTree(int depth)
{
    ZtaginfNode *n;

    if (depth <= 0 || (dt_u32() & 1) == 0) {
        return NULL;
    }
    n = arenaAlloc(sizeof *n);
    dt_fill(n, sizeof *n);
    n->faces = (dt_u32() & 1) != 0 ? (struct ZbspLevelFace **)arenaAlloc(16) : NULL;
    n->child[0] = randomTree(depth - 1);
    n->child[1] = randomTree(depth - 1);
    return n;
}

static void randomTables(void)
{
    gZtaginfFaces = (dt_u32() & 1) != 0 ? (struct ZbspLevelFace *)arenaAlloc(0x6c * 4) : NULL;
    gZtaginfFaceCount = dt_u32();
    gZtaginfData = (dt_u32() & 1) != 0 ? arenaAlloc(64) : NULL;
    dt_fill(&gZtaginfRoot, sizeof gZtaginfRoot);
    gZtaginfRoot.faces = (dt_u32() & 1) != 0 ? (struct ZbspLevelFace **)arenaAlloc(16) : NULL;
    gZtaginfRoot.child[0] = randomTree(3);
    gZtaginfRoot.child[1] = randomTree(3);
    gZtaginfNodes = arenaAlloc(0x40);
}

static void setupInit(DtCtx *c)
{
    begin(c);
    dt_fill(&gZtaginfRoot, sizeof gZtaginfRoot);
    gZtaginfFaces = arenaAlloc(8);
    gZtaginfData = arenaAlloc(8);
    gZtaginfNodes = NULL;
}

static void setupFreeNode(DtCtx *c)
{
    ZtaginfNode *n;

    begin(c);
    randomTables();
    n = arenaAlloc(sizeof *n);
    dt_fill(n, sizeof *n);
    n->faces = (dt_u32() & 1) != 0 ? (struct ZbspLevelFace **)arenaAlloc(16) : NULL;
    n->child[0] = randomTree(3);
    n->child[1] = randomTree(3);
    DT_ARG_PTR(c, 0, n);
}

static void setupRelease(DtCtx *c)
{
    begin(c);
    randomTables();
}

static void setupGetters(DtCtx *c)
{
    begin(c);
    randomTables();
}

static void setupReadNode(DtCtx *c)
{
    begin(c);
    stubIff();
    randomTables();
    gZtaginfFaces = (struct ZbspLevelFace *)arenaAlloc(0x6c * 8);
    DT_ARG_PTR(c, 0, arenaAlloc(sizeof(ZtaginfNode)));
    DT_ARG_PTR(c, 1, (void *)(uintptr_t)(0x00700000u + dt_range(0, 255) * 4));
}

static void setupLoad(DtCtx *c)
{
    begin(c);
    stubIff();
    randomTables();
    DT_ARG_PTR(c, 0, (void *)(uintptr_t)(0x00700000u + dt_range(0, 255) * 4));
}

DT_CASES(ztaginf_cases) = {
    {"ztaginfInitialize", 0x0048d6f0, ztaginfInitialize, 0, DT_RET_INT, setupInit, 20, 0},
    {"ztaginfFreeNode", 0x0048d860, ztaginfFreeNode, 1, DT_RET_VOID, setupFreeNode, 300, 0},
    {"ztaginfRelease", 0x0048d8d0, ztaginfRelease, 0, DT_RET_VOID, setupRelease, 300, 0},
    {"ztaginfGetFaces", 0x0048d980, ztaginfGetFaces, 0, DT_RET_INT, setupGetters, 20, 0},
    {"ztaginfGetFaceCount", 0x0048d990, ztaginfGetFaceCount, 0, DT_RET_INT, setupGetters, 20, 0},
    {"ztaginfReadNode", 0x0048d9a0, ztaginfReadNode, 2, DT_RET_INT, setupReadNode, 500, 0},
    {"ztaginfLoad", 0x0048dae0, ztaginfLoad, 1, DT_RET_INT, setupLoad, 1000, 0},
};
