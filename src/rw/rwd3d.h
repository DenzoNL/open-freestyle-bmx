/*
 * The CPU side of RenderWare 3.1's Direct3D 7 driver (lib/d3d7driver): Im3D transform, clip flags and index
 * lists (rw/d3dim3d.c). It keeps its state in the exe's data, like the rest of the engine, so the ports run
 * in the hook DLL next to the original driver as well as in the native exe (docs/SDL3-DRIVER.md, step 2).
 *
 * Where the original calls the IDirect3DDevice7, the ports call the rwD3D* functions below instead: the hook
 * DLL and the tests forward them to the device the original uses (src/hook/d3dcom.c), the native exe draws
 * with SDL3 GPU (src/driver).
 */
#ifndef RW_RWD3D_H
#define RW_RWD3D_H

#include "rw/rwcamera.h"
#include "rw/rwcore.h"
#include "rw/rwim.h"
#include "rw/rwtexture.h"

/*
 * Rounding: inside a camera update the original runs the x87 at 24-bit precision, rounding down (_rwcseg
 * 0x54b000, set by the camera's begin update). Our float code runs on SSE2, which already rounds every
 * operation to single precision; rwD3DRoundBegin copies the x87 rounding mode into MXCSR for the duration of
 * a function, so results match the original's bit for bit in whatever mode it runs (in the tests, the hook
 * DLL and the native exe). rwD3DFistp is FISTP: float to int in the current rounding mode.
 */
static inline uint32_t rwD3DRoundBegin(void)
{
    uint16_t cw;
    uint32_t old, mxcsr;
    __asm__ volatile("fnstcw %0" : "=m"(cw)::"memory");
    __asm__ volatile("stmxcsr %0" : "=m"(old)::"memory");
    mxcsr = (old & ~0x6000u) | ((uint32_t)(cw & 0x0c00u) << 3);
    __asm__ volatile("ldmxcsr %0" ::"m"(mxcsr) : "memory");
    return old;
}

static inline void rwD3DRoundEnd(uint32_t old)
{
    __asm__ volatile("ldmxcsr %0" ::"m"(old) : "memory");
}

static inline int32_t rwD3DFistp(float value)
{
    int32_t result;
    __asm__ volatile("cvtss2si %1, %0" : "=r"(result) : "x"(value) : "memory");
    return result;
}

static inline int32_t rwD3DBits(float value)
{
    int32_t bits;
    __builtin_memcpy(&bits, &value, 4);
    return bits;
}

/* A transformed and lit vertex as Direct3D 7 takes it (D3DFVF_TLVERTEX, 0x1c4). */
typedef struct RwD3DTLVertex {
    float sx, sy, sz, rhw; /* 0x00 screen position, 1/w */
    uint32_t color;        /* 0x10 D3DCOLOR (A8R8G8B8) */
    uint32_t specular;     /* 0x14 alpha: vertex fog factor */
    float u, v;            /* 0x18 */
} RwD3DTLVertex;

_Static_assert(sizeof(RwD3DTLVertex) == 0x20, "RwD3DTLVertex layout");

/*
 * A vertex in camera space: position, the clip flags of RwD3DClip, the colour the instancing pipelines light
 * (0..255 per channel, clamped when converted to the TL vertex), and the texture coordinates.
 */
typedef struct RwD3DCamVertex {
    float x, y, z; /* 0x00 */
    uint8_t clip;  /* 0x0c */
    uint8_t pad0d[3];
    float r, g, b, a; /* 0x10 */
    float u, v;       /* 0x20 */
} RwD3DCamVertex;

_Static_assert(sizeof(RwD3DCamVertex) == 0x28, "RwD3DCamVertex layout");
_Static_assert(offsetof(RwD3DCamVertex, r) == 0x10, "RwD3DCamVertex layout");

/* Clip flags (outcodes) of a camera space vertex. */
enum {
    rwD3DCLIPLEFT = 0x01,
    rwD3DCLIPRIGHT = 0x02,
    rwD3DCLIPTOP = 0x04,
    rwD3DCLIPBOTTOM = 0x08,
    rwD3DCLIPNEAR = 0x10,
    rwD3DCLIPFAR = 0x20,
};

/* The RwIm3DVertex of this driver (0x24 bytes): object position and normal, RwRGBA, texture coordinates. */
typedef struct RwD3DIm3DVertex {
    float x, y, z;      /* 0x00 */
    float nx, ny, nz;   /* 0x0c */
    uint8_t r, g, b, a; /* 0x18 */
    float u, v;         /* 0x1c */
} RwD3DIm3DVertex;

_Static_assert(sizeof(RwD3DIm3DVertex) == 0x24, "RwD3DIm3DVertex layout");

/*
 * The work object an instance stage hands to the other stages. Im3D allocates one (0x88 bytes); the
 * instancing pipelines of atomics and world sectors (rw/d3dpipe.c) keep theirs in a resource entry of the
 * object, with the vertices and triangles behind it: an atomic's at 0x88, a world sector's already at 0x48
 * (it has no matrix of its own and uses the camera's view matrix).
 */
typedef struct RwD3DIm3DWork {
    RwResEntry entry;          /* 0x00 the resource entry (pipelines) */
    uint32_t serialNum;        /* 0x18 the mesh header's serial number at instancing */
    uint32_t flags;            /* 0x1c geometry or world flags; 0x40: modulate by the material colour */
    int32_t numVertices;       /* 0x20 */
    int32_t stride;            /* 0x24 0x24 for Im3D; 0x1c or 0x24 (with normals) for the pipelines */
    void *vertices;            /* 0x28 RwD3DIm3DVertex layout */
    uint32_t numTriangles;     /* 0x2c */
    int32_t triangleStride;    /* 0x30 bytes per triangle (6) */
    int32_t triangleOffset;    /* 0x34 */
    uint8_t *triangles;        /* 0x38 three vertex indices each */
    uint16_t type;             /* 0x3c the vertex type of the pipeline's table */
    uint16_t lit;              /* 0x3e bit 0: a light was applied; zeroed by every instance */
    struct RpMeshHeader *mesh; /* 0x40 */
    const RwMatrix *matrix;    /* 0x44 object to camera space */
    RwMatrix combined;         /* 0x48 ltm x view */
} RwD3DIm3DWork;

BMX_LAYOUT32_ASSERT(sizeof(RwD3DIm3DWork) == 0x88, "RwD3DIm3DWork layout");
BMX_LAYOUT32_ASSERT(offsetof(RwD3DIm3DWork, numVertices) == 0x20, "RwD3DIm3DWork layout");
BMX_LAYOUT32_ASSERT(offsetof(RwD3DIm3DWork, mesh) == 0x40, "RwD3DIm3DWork layout");

typedef int(BMX_CDECL *RwD3DTriFunc)(int32_t a, int32_t b, int32_t c);
typedef void(BMX_CDECL *RwD3DTriVoidFunc)(int32_t a, int32_t b, int32_t c);
typedef void(BMX_CDECL *RwD3DLineFunc)(int32_t a, int32_t b);
typedef void(BMX_CDECL *RwD3DVoidFunc)(void);
typedef void(BMX_CDECL *RwD3DProjectionFunc)(int32_t projection);
typedef void(BMX_CDECL *RwD3DUVSetFunc)(uint32_t flags);
typedef void(BMX_CDECL *RwD3DUVStageFunc)(RwD3DIm3DWork *work);

/* One level of the Im3D transform stack (RwIm3DTransform inside RwIm3DTransform...): up to 10. */
typedef struct RwD3DIm3DLevel {
    RwD3DUVStageFunc uvStage; /* 0x00 copies the texture coordinates (or not) */
    uint32_t orClip;          /* 0x04 clip flags of all vertices, or'ed */
    uint32_t andClip;         /* 0x08 and'ed: non zero = everything is outside */
    int32_t base;             /* 0x0c first vertex of the level in the buffers */
    int32_t numVertices;      /* 0x10 transformed vertices */
    int32_t numTL;            /* 0x14 TL vertices including those clipping adds */
} RwD3DIm3DLevel;

BMX_LAYOUT32_ASSERT(sizeof(RwD3DIm3DLevel) == 0x18, "RwD3DIm3DLevel layout");

/* A growable index list of the Im3D draw calls: triangles (0x696a64) and lines (0x696a70). */
typedef struct RwD3DIndexList {
    uint16_t *indices; /* 0x00 */
    uint32_t capacity; /* 0x04 in indices */
    uint32_t count;    /* 0x08 */
} RwD3DIndexList;

/* ---- driver globals (0x696900..0x696a8c) ---- */
#define rwD3DClipTriCull   ORIG_VAR(0x00696900, RwD3DTriFunc, rwD3DClipTriCull)
#define rwD3DTriCull       ORIG_VAR(0x00696904, RwD3DTriFunc, rwD3DTriCull)
#define rwD3DClipTri       ORIG_VAR(0x00696908, RwD3DTriVoidFunc, rwD3DClipTri)
#define rwD3DClipLine      ORIG_VAR(0x0069690c, RwD3DLineFunc, rwD3DClipLine)
#define rwD3DIndexStart    ORIG_VAR(0x00696910, RwD3DVoidFunc, rwD3DIndexStart)
#define rwD3DIndexTri      ORIG_VAR(0x00696914, RwD3DTriVoidFunc, rwD3DIndexTri)
#define rwD3DIndexLine     ORIG_VAR(0x00696918, RwD3DLineFunc, rwD3DIndexLine)
#define rwD3DIndexFlush    ORIG_VAR(0x0069691c, RwD3DVoidFunc, rwD3DIndexFlush)
#define rwD3DSetProjection ORIG_VAR(0x00696920, RwD3DProjectionFunc, rwD3DSetProjection)
#define rwD3DSetUVStage    ORIG_VAR(0x00696924, RwD3DUVSetFunc, rwD3DSetUVStage)
/* Camera to screen: sz = z' * zScale + zShift, sx = x' * xScale + xOffset (x', z' after the projection). */
#define rwD3DZScale  ORIG_VAR(0x00696928, float, rwD3DZScale)
#define rwD3DZShift  ORIG_VAR(0x0069692c, float, rwD3DZShift)
#define rwD3DXScale  ORIG_VAR(0x00696930, float, rwD3DXScale)
#define rwD3DYScale  ORIG_VAR(0x00696934, float, rwD3DYScale)
#define rwD3DXOffset ORIG_VAR(0x00696938, float, rwD3DXOffset)
#define rwD3DYOffset ORIG_VAR(0x0069693c, float, rwD3DYOffset)
/* Near and far plane, compared as integers (the bits of positive floats order like the floats). */
#define rwD3DNearZ       ORIG_VAR(0x00696940, int32_t, rwD3DNearZ)
#define rwD3DFarZ        ORIG_VAR(0x00696944, int32_t, rwD3DFarZ)
#define rwD3DNearScreenZ ORIG_VAR(0x00696948, float, rwD3DNearScreenZ)
#define rwD3DFarScreenZ  ORIG_VAR(0x0069694c, float, rwD3DFarScreenZ)
#define rwD3DRecipWindow ORIG_VAR(0x00696950, RwV2d, rwD3DRecipWindow)
#define rwD3DTLBase      ORIG_VAR(0x00696958, RwD3DTLVertex *, rwD3DTLBase)
#define rwD3DCamBase     ORIG_VAR(0x0069695c, RwD3DCamVertex *, rwD3DCamBase)
#define rwD3DIm3DDepth   ORIG_VAR(0x00696964, uint32_t, rwD3DIm3DDepth)
#define rwD3DIm3DLevel   ORIG_VAR(0x00696968, RwD3DIm3DLevel *, rwD3DIm3DLevel)
#define rwD3DIm3DLevels  ((RwD3DIm3DLevel *)BMX_ORIG(0x0069696c))
#define rwD3DTriIndices  ORIG_VAR(0x00696a64, RwD3DIndexList, rwD3DTriIndices)
#define rwD3DLineIndices ORIG_VAR(0x00696a70, RwD3DIndexList, rwD3DLineIndices)
#define rwD3DTLBuffer    ORIG_VAR(0x00696a7c, RwD3DTLVertex *, rwD3DTLBuffer)
#define rwD3DCamBuffer   ORIG_VAR(0x00696a80, RwD3DCamVertex *, rwD3DCamBuffer)
#define rwD3DBufferSize  ORIG_VAR(0x00696a84, uint32_t, rwD3DBufferSize)
#define rwD3DIm3DWork    ORIG_VAR(0x00696a88, RwD3DIm3DWork *, rwD3DIm3DWork)
/* One bit per instanced vertex: the pipelines' render stage colours each vertex of a mesh once. */
#define rwD3DVertexBits     ORIG_VAR(0x00696a5c, uint32_t *, rwD3DVertexBits)
#define rwD3DVertexBitsSize ORIG_VAR(0x00696a60, uint32_t, rwD3DVertexBitsSize)
/* Clip flags of the last transform: or'ed and and'ed over the vertices. */
#define rwD3DOrClip  ORIG_VAR(0x00696710, uint32_t, rwD3DOrClip)
#define rwD3DAndClip ORIG_VAR(0x00696720, uint32_t, rwD3DAndClip)
/* The transform's scratch words: vertex stride, matrix, output vertex, fog index. */
#define rwD3DXformStride ORIG_VAR(0x00678270, int32_t, rwD3DXformStride)
#define rwD3DXformMatrix ORIG_VAR(0x00678274, const RwMatrix *, rwD3DXformMatrix)
#define rwD3DXformOut    ORIG_VAR(0x00678278, RwD3DTLVertex *, rwD3DXformOut)
#define rwD3DXformFog    ORIG_VAR(0x0067827c, int32_t, rwD3DXformFog)
/* Vertex fog: start (camera fog plane) and end (far plane) as compared bit patterns, 255 / (end - start). */
#define rwD3DFogStart ORIG_VAR(0x00697354, int32_t, rwD3DFogStart)
#define rwD3DFogEnd   ORIG_VAR(0x00697358, float, rwD3DFogEnd)
#define rwD3DFogScale ORIG_VAR(0x0069735c, float, rwD3DFogScale)
/* Specular colour (fog factor in alpha) per fog index 0..255. */
#define rwD3DFogTable ((const uint32_t *)BMX_ORIG(0x00696b00))
/* The 1.0f the transform uses for 1/w and the parallel clip limit. */
#define rwD3DOne ORIG_VAR(0x0069c5c0, float, rwD3DOne)

/* ---- render state (rw/d3dstate.c) ---- */

/*
 * The device caps the driver checks (a D3DDEVICEDESC7-like block behind 0x6968f0, triangle caps at +0xf8):
 * raster, z compare, source / destination blend, alpha compare, shade, texture, filter and address caps.
 */
#define rwD3DCaps ORIG_VAR(0x006968f0, const uint8_t *, rwD3DCaps)
enum {
    rwD3DCAPSRASTER = 0x100,
    rwD3DCAPSZCMP = 0x104,
    rwD3DCAPSSRCBLEND = 0x108,
    rwD3DCAPSDESTBLEND = 0x10c,
    rwD3DCAPSALPHACMP = 0x110,
    rwD3DCAPSSHADE = 0x114,
    rwD3DCAPSTEXTURE = 0x118,
    rwD3DCAPSFILTER = 0x11c,
    rwD3DCAPSADDRESS = 0x124,
};

static inline uint32_t rwD3DCap(uint32_t offset)
{
    return *(const uint32_t *)(rwD3DCaps + offset);
}

/* The render target being drawn to, non NULL inside a camera update; +8 is the IDirect3DDevice7. */
#define rwD3DContext ORIG_VAR(0x00697340, uint8_t *, rwD3DContext)
/* The driver's raster and camera plugin offsets. */
#define rwD3DRasterExtOffset ORIG_VAR(0x00697364, int32_t, rwD3DRasterExtOffset)
#define rwD3DCameraExtOffset ORIG_VAR(0x00697368, int32_t, rwD3DCameraExtOffset)
/* The engine memory functions as the device got them at registration. */
#define rwD3DMemory ORIG_VAR(0x0069734c, const RwMemoryFunctions *, rwD3DMemory)
/* The engine device (RwGlobals.dOpenDevice) as the driver got it at registration. */
#define rwD3DEngineDevice ORIG_VAR(0x00676cdc, RwDevice *, rwD3DEngineDevice)

/* The cached device state (what was last sent to Direct3D), in the order the deferred list saves it. */
typedef struct RwD3DStateCache {
    int32_t vertexAlpha;     /* 0x696740 rwRENDERSTATEVERTEXALPHAENABLE */
    int32_t textureAlpha;    /* 0x696744 the texture raster has alpha */
    int32_t perspective;     /* 0x696748 D3DRENDERSTATE_TEXTUREPERSPECTIVE */
    int32_t zTest;           /* 0x69674c D3DRENDERSTATE_ZENABLE */
    int32_t zWrite;          /* 0x696750 D3DRENDERSTATE_ZWRITEENABLE */
    int32_t shadeMode;       /* 0x696754 D3DRENDERSTATE_SHADEMODE */
    int32_t srcBlend;        /* 0x696758 */
    int32_t destBlend;       /* 0x69675c */
    int32_t fogEnable;       /* 0x696760 */
    int32_t fogColor;        /* 0x696764 */
    int32_t alphaBlend;      /* 0x696768 D3DRENDERSTATE_ALPHABLENDENABLE */
    int32_t colorKey;        /* 0x69676c D3DRENDERSTATE_COLORKEYENABLE */
    int32_t alphaTest;       /* 0x696770 D3DRENDERSTATE_ALPHATESTENABLE */
    int32_t address;         /* 0x696774 stage 0 D3DTSS_ADDRESS */
    int32_t borderColor;     /* 0x696778 stage 0 D3DTSS_BORDERCOLOR */
    int32_t magFilter;       /* 0x69677c */
    int32_t minFilter;       /* 0x696780 */
    int32_t mipFilter;       /* 0x696784 */
    int32_t colorOp;         /* 0x696788 */
    int32_t alphaOp;         /* 0x69678c */
    RwRaster *textureRaster; /* 0x696790 */
    int32_t fogType;         /* 0x696794 */
    float fogDensity;        /* 0x696798 */
    uint8_t fogTable[256];   /* 0x69679c */
    int32_t alphaSort;       /* 0x69689c rwRENDERSTATEALPHAPRIMITIVEBUFFER: translucent primitives deferred */
} RwD3DStateCache;

BMX_LAYOUT32_ASSERT(sizeof(RwD3DStateCache) == 0x160, "RwD3DStateCache layout");

#define rwD3DState ORIG_VAR(0x00696740, RwD3DStateCache, rwD3DState)
/* 1 when the device can alpha test (greater or not equal). */
#define rwD3DAlphaTestCaps ORIG_VAR(0x00676cc4, int32_t, rwD3DAlphaTestCaps)
/* Fog: the specular colour per fog index and the source tables of the fog types. */
#define rwD3DFogSpecular ((uint32_t *)BMX_ORIG(0x00696b00))
#define rwD3DFogLinear   ((uint8_t *)BMX_ORIG(0x00676bc4))
#define rwD3DFogExp      ((const uint8_t *)BMX_ORIG(0x006769a4))
#define rwD3DFogExp2     ((const uint8_t *)BMX_ORIG(0x00676aa4))

/* The pixel format record of a raster's plugin data (RwD3DRasterExt.info). */
typedef struct RwD3DRasterInfo {
    uint8_t pad00[8];
    uint32_t flags; /* 0x08 bit 0: the texture has alpha */
} RwD3DRasterInfo;

/*
 * The start of the driver's raster plugin data (at rwD3DRasterExtOffset), the fields the texture state reads.
 * The original's Direct3D driver and ours (src/driver, DrvRasterExt) both lay it out this way.
 */
typedef struct RwD3DRasterExt {
    void *texture;               /* 0x00 */
    int32_t colorKey;            /* 0x04 colour key on */
    void *cpu;                   /* 0x08 */
    void *handle;                /* 0x0c the texture handle */
    const RwD3DRasterInfo *info; /* 0x10 */
} RwD3DRasterExt;

BMX_LAYOUT32_ASSERT(offsetof(RwD3DRasterExt, colorKey) == 4 && offsetof(RwD3DRasterExt, handle) == 0xc &&
                        offsetof(RwD3DRasterExt, info) == 0x10,
                    "RwD3DRasterExt layout");

/* ---- the deferred (alpha sorted) primitives (rw/d3dim2d.c) ---- */

/* A growing arena of records, each a callback slot followed by the record (rwD3DDeferAlloc). */
typedef struct RwD3DDeferList {
    uint8_t *arena;     /* 0x00 */
    uint32_t arenaSize; /* 0x04 */
    uint32_t used;      /* 0x08 */
    uint32_t *offsets;  /* 0x0c record offsets in the arena */
    uint32_t offsetCap; /* 0x10 */
    uint32_t count;     /* 0x14 */
    uint32_t frames;    /* 0x18 counts resets; every 64th may shrink the arena */
    uint32_t maxUsed;   /* 0x1c */
} RwD3DDeferList;

/* The state a deferred primitive is drawn with (what rwD3DSaveState copies from rwD3DState). */
typedef struct RwD3DSavedState {
    int32_t vertexAlpha;     /* 0x00 */
    int32_t perspective;     /* 0x04 */
    int32_t zTest;           /* 0x08 */
    int32_t zWrite;          /* 0x0c */
    int32_t shadeMode;       /* 0x10 */
    int32_t srcBlend;        /* 0x14 */
    int32_t destBlend;       /* 0x18 */
    int32_t fogEnable;       /* 0x1c */
    int32_t fogColor;        /* 0x20 */
    int32_t address;         /* 0x24 */
    int32_t borderColor;     /* 0x28 */
    int32_t magFilter;       /* 0x2c */
    int32_t minFilter;       /* 0x30 */
    int32_t mipFilter;       /* 0x34 */
    int32_t colorOp;         /* 0x38 */
    int32_t alphaOp;         /* 0x3c */
    RwRaster *textureRaster; /* 0x40 */
} RwD3DSavedState;

BMX_LAYOUT32_ASSERT(sizeof(RwD3DSavedState) == 0x44, "RwD3DSavedState layout");

/* A deferred Im2D line or triangle (after the callback slot): its state and its two or three vertices. */
typedef struct RwD3DDeferVerts {
    RwD3DSavedState state; /* 0x00 */
    RwD3DTLVertex v[];     /* 0x44 */
} RwD3DDeferVerts;

/* A deferred Im2D primitive (after the callback slot): its state, the D3D primitive type, the vertices and
 * then `count` uint16_t indices into them. */
typedef struct RwD3DDeferPrim {
    RwD3DSavedState state; /* 0x00 */
    uint32_t type;         /* 0x44 */
    uint32_t numVertices;  /* 0x48 */
    int32_t count;         /* 0x4c */
    RwD3DTLVertex v[];     /* 0x50 */
} RwD3DDeferPrim;

BMX_LAYOUT32_ASSERT(offsetof(RwD3DDeferVerts, v) == 0x44 && offsetof(RwD3DDeferPrim, v) == 0x50,
                    "deferred record layout");

/* The indices of a deferred primitive, after its vertices. */
static inline uint16_t *rwD3DDeferIndices(RwD3DDeferPrim *p)
{
    return (uint16_t *)&p->v[p->numVertices];
}

/* An element to sort: one primitive (triangle or line) of a deferred record. */
typedef struct RwD3DSortItem {
    uint8_t *record;   /* 0x00 */
    uint16_t *indices; /* 0x04 */
    uint16_t count;    /* 0x08 indices */
    uint16_t type;     /* 0x0a D3DPRIMITIVETYPE */
    float key;         /* 0x0c sum of the screen z of its vertices, compared as an integer */
} RwD3DSortItem;

#define rwD3DDefer       ORIG_VAR(0x00676bac, RwD3DDeferList *, rwD3DDefer)
#define rwD3DSortNeed    ORIG_VAR(0x00676bb0, uint32_t, rwD3DSortNeed)
#define rwD3DSortCap     ORIG_VAR(0x00676bb4, uint32_t, rwD3DSortCap)
#define rwD3DSortCount   ORIG_VAR(0x00676bb8, uint32_t, rwD3DSortCount)
#define rwD3DSortItems   ORIG_VAR(0x00676bbc, RwD3DSortItem *, rwD3DSortItems)
#define rwD3DMergeCap    ORIG_VAR(0x00676cc8, uint32_t, rwD3DMergeCap)
#define rwD3DMergeBuf    ORIG_VAR(0x00676ccc, uint16_t *, rwD3DMergeBuf)
#define rwD3DRemapCap    ORIG_VAR(0x00676cd0, int32_t, rwD3DRemapCap)
#define rwD3DRemap       ORIG_VAR(0x00676cd4, uint16_t *, rwD3DRemap)
#define rwD3DRemapUsed   ORIG_VAR(0x00676cd8, uint16_t *, rwD3DRemapUsed)
#define rwD3DRenderOrder ORIG_VAR(0x00566f20, int32_t, rwD3DRenderOrder)

/*
 * ---- the cut: IDirect3DDevice7 calls ----
 * The hook DLL and the tests forward these to the device at rwD3DContext + 8 (src/hook/d3dcom.c); the native
 * exe implements them on SDL3 GPU (src/driver/draw.c). Arguments are Direct3D 7 ones.
 */
int rwD3DDevSetRenderState(uint32_t state, uint32_t value);
int rwD3DDevSetTextureStageState(uint32_t stage, uint32_t type, uint32_t value);
int rwD3DDevSetTexture(uint32_t stage, void *texture);
int rwD3DDevDrawPrimitive(uint32_t type, uint32_t fvf, const void *vertices, uint32_t count, uint32_t flags);
int rwD3DDevDrawIndexedPrimitive(uint32_t type, uint32_t fvf, const void *vertices, uint32_t numVertices,
                                 const uint16_t *indices, uint32_t numIndices, uint32_t flags);

/* rw/d3dstate.c */
__attribute__((regparm(2))) int rwD3DSetRenderState(int32_t state, intptr_t arg);
int rwD3DRenderStateSet(int32_t state, intptr_t value);
int rwD3DRenderStateGet(int32_t state, void *value);
void *rwD3DRasterHandle(const RwRaster *raster);
__attribute__((regparm(1))) int rwD3DSetTextureRaster(RwRaster *raster);
__attribute__((regparm(1))) int rwD3DSetSrcBlend(int32_t blend);
__attribute__((regparm(1))) int rwD3DSetDestBlend(int32_t blend);
__attribute__((regparm(1))) int rwD3DFogTableSelect(int32_t type);
__attribute__((regparm(1))) int rwD3DSetFogType(int32_t type);
__attribute__((regparm(1))) int rwD3DSetFogTable(const uint32_t *table);

/* rw/d3dim2d.c */
RwD3DDeferList *rwD3DDeferCreate(void);
void *rwD3DDeferAlloc(RwD3DDeferList *list, uint32_t size, void *callback);
RwD3DDeferList *rwD3DDeferRun(RwD3DDeferList *list, int32_t order);
RwD3DDeferList *rwD3DDeferReset(RwD3DDeferList *list);
int rwD3DIm2DOpen(void);
__attribute__((regparm(1))) void rwD3DSaveState(RwD3DSavedState *state);
__attribute__((regparm(1))) void rwD3DRestoreState(const RwD3DSavedState *saved);
void rwD3DDeferFlush(void);
int rwD3DSortCompare(const RwD3DSortItem *a, const RwD3DSortItem *b);
void rwD3DSortDraw(void);
void rwD3DDeferSplit(uint8_t *record);
void rwD3DDeferLine(uint8_t *record);
void rwD3DDeferTriangle(uint8_t *record);
int rwD3DIm2DLine(RwD3DTLVertex *vertices, int32_t numVertices, int32_t a, int32_t b);
int rwD3DIm2DTriangle(RwD3DTLVertex *vertices, int32_t numVertices, int32_t a, int32_t b, int32_t c);
int rwD3DIm2DPrimitive(int32_t primType, RwD3DTLVertex *vertices, int32_t numVertices);
int rwD3DIm2DIndexedPrimitive(int32_t primType, RwD3DTLVertex *vertices, int32_t numVertices,
                              const uint16_t *indices, int32_t numIndices);

/* rw/d3dim3d.c */
int rwD3DIm3DBegin(RwIm3DPipeline *pipeline);
int rwD3DIm3DEnd(void);
void *rwD3DIm3DOpen(void *engine);
void *rwD3DIm3DClose(void *engine);
int rwD3DIm3DGrow(uint32_t extra);
__attribute__((regparm(2))) int rwD3DIndexListGrow(RwD3DIndexList *list, uint32_t capacity);
int rwD3DIndexListResize(RwD3DIndexList *list, uint32_t capacity);
void rwD3DIndexListCreate(RwD3DIndexList *list);
void rwD3DIndexListDestroy(RwD3DIndexList *list);
void rwD3DIndexStartDefault(void);
void rwD3DIndexTriDefault(int32_t a, int32_t b, int32_t c);
void rwD3DIndexLineDefault(int32_t a, int32_t b);
void rwD3DIndexFlushDefault(void);
void rwD3DSetUVStageDefault(uint32_t flags);
void rwD3DSetUVStage0(uint32_t flags);
void rwD3DUVNone(RwD3DIm3DWork *work);
void rwD3DUVCopy(RwD3DIm3DWork *work);
void rwD3DIm3DCameraBegin(RwCamera *camera);
void rwD3DIm3DRenderState(int32_t state, int32_t value);
void rwD3DSetProjectionDefault(int32_t projection);
RwD3DIm3DWork *rwD3DIm3DInstance(RwIm3DTransformInfo *info);
void rwD3DIm3DNothing(RwD3DIm3DWork *work);
void rwD3DIm3DNothing6(RwD3DIm3DWork *work);
void rwD3DIm3DColors(RwD3DIm3DWork *work);
void rwD3DIm3DTransform(RwD3DIm3DWork *work);
int rwD3DIm3DPipeline(RwIm3DPipeline *pipeline);
int rwD3DTriCullPersp(int32_t a, int32_t b, int32_t c);
int rwD3DClipTriCullPersp(int32_t a, int32_t b, int32_t c);
int rwD3DTriCullParallel(int32_t a, int32_t b, int32_t c);
__attribute__((regparm(2))) void rwD3DRenderLineIndices(int32_t a, int32_t b);
__attribute__((regparm(3))) void rwD3DRenderTriIndices(int32_t a, int32_t b, int32_t c);
int rwD3DIm3DRenderLine(int32_t a, int32_t b);
int rwD3DIm3DRenderTriangle(int32_t a, int32_t b, int32_t c);
int rwD3DIm3DRenderPrimitive(int32_t primType);
int rwD3DIm3DRenderIndexedPrimitive(int32_t primType, const uint16_t *indices, int32_t numIndices);
int rwD3DVertexBitsAlloc(int32_t count);

/* rw/d3dpipe.c: the instancing pipelines of atomics and world sectors */
struct RpAtomic;
struct RpLight;
struct RpWorldSector;
struct RwSurfaceProperties;

/* A light routine (RpLight.worldExtCallback): adds the light to the camera vertices' colours. */
typedef void(BMX_CDECL *RwD3DLightFunc)(RwD3DIm3DWork *work, struct RpLight *light, const RwMatrix *inverse,
                                        float scale, const struct RwSurfaceProperties *surface);

__attribute__((regparm(1))) void rwD3DAtomicMorph(struct RpAtomic *atomic);
RwD3DIm3DWork *rwD3DAtomicInstance(struct RpAtomic *atomic);
RwD3DIm3DWork *rwD3DSectorInstance(struct RpWorldSector *sector);
void rwD3DApplyLight(RwD3DIm3DWork *work, struct RpLight *light, const RwMatrix *inverse, float scale,
                     const struct RwSurfaceProperties *surface);
void rwD3DPrelight(RwD3DIm3DWork *work);
void rwD3DLightColors(RwD3DIm3DWork *work);
void rwD3DLightAmbient(RwD3DIm3DWork *work, struct RpLight *light, const RwMatrix *inverse, float scale,
                       const struct RwSurfaceProperties *surface);
void rwD3DLightDirectional(RwD3DIm3DWork *work, struct RpLight *light, const RwMatrix *inverse, float scale,
                           const struct RwSurfaceProperties *surface);
void rwD3DLightPoint(RwD3DIm3DWork *work, struct RpLight *light, const RwMatrix *inverse, float scale,
                     const struct RwSurfaceProperties *surface);
void rwD3DLightSpot(RwD3DIm3DWork *work, struct RpLight *light, const RwMatrix *inverse, float scale,
                    const struct RwSurfaceProperties *surface);
void rwD3DLightSoftSpot(RwD3DIm3DWork *work, struct RpLight *light, const RwMatrix *inverse, float scale,
                        const struct RwSurfaceProperties *surface);
void rwD3DModulateColor(uint32_t index, const float *color);
void rwD3DVertexColor(uint32_t index);
void rwD3DRenderMeshes(RwD3DIm3DWork *work);

#endif
