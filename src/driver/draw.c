/*
 * The native side of the D3D7 driver's cut (rw/rwd3d.h): Direct3D 7 render states, texture stage states,
 * textures and DrawPrimitive / DrawIndexedPrimitive of TL vertices, on SDL3 GPU.
 *
 * Draws are batched per frame: vertices and indices go into one buffer each, every draw records its pipeline
 * (blend, z, cull and primitive class, cached by key), texture, sampler and fragment state. drvDrawFlush
 * uploads the buffers and dirty textures and replays the draws in one render pass on the camera's target; the
 * camera's end update, clears, the present and texture changes flush. Fans and strips become lists (SDL3 GPU
 * has no fans). Colour writes keep the target's alpha at 1 (the original's camera surfaces have no alpha).
 */
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL.h>

#include "driver/driver.h"
#include "native/native.h"
#include "rw/rwd3d.h"

#include "driver/shaders/tl_ps.h"
#include "driver/shaders/tl_vs.h"
#include "driver/shaders/ui_ps.h"

/* D3DRENDERSTATETYPE / D3DTEXTURESTAGESTATETYPE values the backend reads. */
enum {
    kRsZEnable = 7,
    kRsZWriteEnable = 0xe,
    kRsAlphaTestEnable = 0xf,
    kRsSrcBlend = 0x13,
    kRsDestBlend = 0x14,
    kRsCullMode = 0x16,
    kRsZFunc = 0x17,
    kRsAlphaRef = 0x18,
    kRsAlphaFunc = 0x19,
    kRsAlphaBlendEnable = 0x1b,
    kRsFogEnable = 0x1c,
    kRsSpecularEnable = 0x1d,
    kRsFogColor = 0x22,
    kTssColorOp = 1,
    kTssAlphaOp = 4,
    kTssAddress = 0xc,
    kTssMagFilter = 0x10,
    kTssMinFilter = 0x11,
    kNumRs = 0x100,
    kNumTss = 0x20,
};

static uint32_t rs[kNumRs];
static uint32_t tss[kNumTss];
static RwRaster *boundTexture;
static int uiMode; /* drvDrawSetUiMode */

/* What a draw needs besides its vertices (compared by value to reuse pipelines and samplers). */
typedef struct PipelineKey {
    uint8_t lines;
    uint8_t blend, src, dst;
    uint8_t zTest, zWrite, zFunc, cull;
    uint8_t depth; /* the target has a z buffer */
    uint8_t ui;    /* the UI pixel shader (sharp bilinear, blurred fill) */
    uint8_t pad[2];
} PipelineKey;

typedef struct PsUniforms {
    int32_t ops[4];
    float params[4];
    float fogColor[4];
} PsUniforms;

typedef struct DrawCmd {
    SDL_GPUGraphicsPipeline *pipeline;
    RwRaster *texture;
    SDL_GPUSampler *sampler;
    PsUniforms ps;
    uint32_t firstIndex, numIndices;
    int32_t vertexOffset;
} DrawCmd;

static struct {
    SDL_GPUShader *vs, *ps, *psUi;
    struct {
        PipelineKey key;
        SDL_GPUGraphicsPipeline *pipeline;
    } pipelines[64];
    int numPipelines;
    SDL_GPUSampler *samplers[2][4]; /* [linear][address 0..3] */
    SDL_GPUTexture *white;          /* bound when no texture is (the shader ignores it) */
    /* The frame's batch. */
    RwD3DTLVertex *vertices;
    uint32_t numVertices, capVertices;
    uint16_t *indices;
    uint32_t numIndices, capIndices;
    DrawCmd *draws;
    uint32_t numDraws, capDraws;
    SDL_GPUBuffer *vertexBuffer, *indexBuffer;
    uint32_t vertexBufferSize, indexBufferSize;
    /* The target of the batch: the camera's raster (top level) and z buffer. */
    SDL_GPUTexture *color, *depth;
    int32_t width, height;
    SDL_Rect scissor;
} draw;

static void *grow(void *p, uint32_t *cap, uint32_t need, uint32_t size)
{
    if (need <= *cap) {
        return p;
    }
    uint32_t n = *cap != 0 ? *cap : 256;
    while (n < need) {
        n *= 2;
    }
    void *grown = realloc(p, (size_t)n * size);
    if (grown == NULL) {
        bmxLog("driver: out of memory for the draw batch\n");
        abort();
    }
    *cap = n;
    return grown;
}

/* ---- render states ----------------------------------------------------------------------------------------
 */

int rwD3DDevSetRenderState(uint32_t state, uint32_t value)
{
    DRV_TRACE("rwD3DDevSetRenderState", state, value, 0);
    if (state < kNumRs) {
        rs[state] = value;
    }
    return 0;
}

int rwD3DDevSetTextureStageState(uint32_t stage, uint32_t type, uint32_t value)
{
    DRV_TRACE("rwD3DDevSetTextureStageState", stage, type, value);
    if (stage == 0 && type < kNumTss) {
        tss[type] = value;
    }
    return 0;
}

/* The texture is the handle rwD3DRasterHandle reads: the raster itself (texture.c). */
int rwD3DDevSetTexture(uint32_t stage, void *texture)
{
    DRV_TRACE("rwD3DDevSetTexture", stage, (uintptr_t)texture, 0);
    if (stage == 0) {
        boundTexture = texture;
    }
    return 0;
}

/* ---- textures
 * ------------------------------------------------------------------------------------------------ */

/* 5 and 6 bit channels to 8 bits, replicating the top bits (as the hardware expands them). */
static uint8_t expand5(uint32_t v)
{
    v &= 31;
    return (uint8_t)(v << 3 | v >> 2);
}

static uint8_t expand6(uint32_t v)
{
    v &= 63;
    return (uint8_t)(v << 2 | v >> 4);
}

/* One row of the CPU copy in RGBA8. */
static void convertRow(const uint8_t *src, uint8_t *dst, int32_t width, int32_t format,
                       const uint8_t *palette)
{
    for (int32_t x = 0; x < width; x++, dst += 4) {
        uint32_t p;
        switch (format) {
        case kDrvFormat565:
            p = (uint32_t)src[2 * x] | (uint32_t)src[2 * x + 1] << 8;
            dst[0] = expand5(p >> 11), dst[1] = expand6(p >> 5), dst[2] = expand5(p), dst[3] = 0xff;
            break;
        case kDrvFormat1555:
            p = (uint32_t)src[2 * x] | (uint32_t)src[2 * x + 1] << 8;
            dst[0] = expand5(p >> 10), dst[1] = expand5(p >> 5), dst[2] = expand5(p);
            dst[3] = (p & 0x8000) != 0 ? 0xff : 0;
            break;
        case kDrvFormat4444:
            p = (uint32_t)src[2 * x] | (uint32_t)src[2 * x + 1] << 8;
            dst[0] = (uint8_t)((p >> 8 & 15) * 17), dst[1] = (uint8_t)((p >> 4 & 15) * 17);
            dst[2] = (uint8_t)((p & 15) * 17), dst[3] = (uint8_t)((p >> 12 & 15) * 17);
            break;
        case kDrvFormat888:
        case kDrvFormat8888:
            dst[0] = src[4 * x + 2], dst[1] = src[4 * x + 1], dst[2] = src[4 * x];
            dst[3] = format == kDrvFormat8888 ? src[4 * x + 3] : 0xff;
            break;
        default: /* paletted (never chosen by this driver) */
            if (palette != NULL) {
                memcpy(dst, palette + 4 * src[x], 4);
            } else {
                memset(dst, 0x80, 4);
            }
            break;
        }
    }
}

/* The GPU texture of a texture raster, created on first use; uploads go into `copy` when it is dirty. */
static SDL_GPUTexture *textureFor(RwRaster *raster, SDL_GPUCopyPass *copy)
{
    SDL_GPUDevice *device = drvGpuDevice();
    RwRaster *top = drvTopRaster(raster);
    DrvRasterExt *ext = drvRasterExt(top);

    if (ext->texture == NULL) {
        SDL_GPUTextureCreateInfo info;
        SDL_zero(info);
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        info.width = (Uint32)(top->width > 0 ? top->width : 1);
        info.height = (Uint32)(top->height > 0 ? top->height : 1);
        info.layer_count_or_depth = 1;
        info.num_levels = 1;
        ext->texture = DRV_FPU_GUARDED(SDL_CreateGPUTexture(device, &info));
        ext->dirty = 1;
    }
    if (ext->texture != NULL && ext->dirty && copy != NULL && ext->cpu != NULL && top->width > 0 &&
        top->height > 0) {
        const uint32_t size = (uint32_t)(top->width * top->height * 4);
        SDL_GPUTransferBufferCreateInfo tinfo;
        SDL_GPUTransferBuffer *buffer;
        SDL_zero(tinfo);
        tinfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        tinfo.size = size;
        buffer = DRV_FPU_GUARDED(SDL_CreateGPUTransferBuffer(device, &tinfo));
        if (buffer != NULL) {
            uint8_t *dst = SDL_MapGPUTransferBuffer(device, buffer, false);
            const int32_t stride = ((top->width * top->depth + 7) / 8 + 3) & ~3;
            const int32_t format = (top->format << 8) & kDrvFormatPixelMask;
            for (int32_t y = 0; dst != NULL && y < top->height; y++) {
                convertRow(ext->cpu + y * stride, dst + y * top->width * 4, top->width, format, top->palette);
            }
            SDL_UnmapGPUTransferBuffer(device, buffer);
            SDL_GPUTextureTransferInfo src;
            SDL_GPUTextureRegion region;
            SDL_zero(src);
            SDL_zero(region);
            src.transfer_buffer = buffer;
            region.texture = ext->texture;
            region.w = (Uint32)top->width;
            region.h = (Uint32)top->height;
            region.d = 1;
            SDL_UploadToGPUTexture(copy, &src, &region, false);
            SDL_ReleaseGPUTransferBuffer(device, buffer);
        }
        ext->dirty = 0;
    }
    return ext->texture;
}

/* ---- pipelines and samplers
 * ---------------------------------------------------------------------------------- */

static SDL_GPUBlendFactor blendFactor(uint32_t d3d)
{
    static const SDL_GPUBlendFactor kFactors[12] = {
        SDL_GPU_BLENDFACTOR_ONE,
        SDL_GPU_BLENDFACTOR_ZERO,
        SDL_GPU_BLENDFACTOR_ONE,
        SDL_GPU_BLENDFACTOR_SRC_COLOR,
        SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_COLOR,
        SDL_GPU_BLENDFACTOR_SRC_ALPHA,
        SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
        SDL_GPU_BLENDFACTOR_DST_ALPHA,
        SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_ALPHA,
        SDL_GPU_BLENDFACTOR_DST_COLOR,
        SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_COLOR,
        SDL_GPU_BLENDFACTOR_SRC_ALPHA_SATURATE,
    };
    return d3d < 12 ? kFactors[d3d] : SDL_GPU_BLENDFACTOR_ONE;
}

static SDL_GPUCompareOp compareOp(uint32_t d3d)
{
    return d3d >= 1 && d3d <= 8 ? (SDL_GPUCompareOp)d3d : SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
}

static SDL_GPUGraphicsPipeline *pipelineFor(const PipelineKey *key)
{
    SDL_GPUGraphicsPipelineCreateInfo info;
    SDL_GPUColorTargetDescription target;
    SDL_GPUVertexBufferDescription buffer;
    SDL_GPUVertexAttribute attributes[4];
    SDL_GPUGraphicsPipeline *pipeline;

    for (int i = 0; i < draw.numPipelines; i++) {
        if (memcmp(&draw.pipelines[i].key, key, sizeof *key) == 0) {
            return draw.pipelines[i].pipeline;
        }
    }
    SDL_zero(info);
    SDL_zero(target);
    SDL_zero(buffer);
    SDL_zero(attributes);
    buffer.slot = 0;
    buffer.pitch = sizeof(RwD3DTLVertex);
    buffer.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    attributes[0] = (SDL_GPUVertexAttribute){0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, 0x00};
    attributes[1] = (SDL_GPUVertexAttribute){1, 0, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, 0x10};
    attributes[2] = (SDL_GPUVertexAttribute){2, 0, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, 0x14};
    attributes[3] = (SDL_GPUVertexAttribute){3, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 0x18};
    info.vertex_shader = draw.vs;
    info.fragment_shader = key->ui ? draw.psUi : draw.ps;
    info.vertex_input_state.vertex_buffer_descriptions = &buffer;
    info.vertex_input_state.num_vertex_buffers = 1;
    info.vertex_input_state.vertex_attributes = attributes;
    info.vertex_input_state.num_vertex_attributes = 4;
    info.primitive_type = key->lines ? SDL_GPU_PRIMITIVETYPE_LINELIST : SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    /* D3D culls by screen winding (y down): D3DCULL_CW (2) culls clockwise, CCW (3) counter-clockwise. With
     * the flip to y up, a clockwise screen triangle is counter-clockwise in SDL's terms. */
    info.rasterizer_state.cull_mode = key->cull == 1 ? SDL_GPU_CULLMODE_NONE : SDL_GPU_CULLMODE_BACK;
    info.rasterizer_state.front_face =
        key->cull == 2 ? SDL_GPU_FRONTFACE_CLOCKWISE : SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    info.rasterizer_state.enable_depth_clip = false;
    info.depth_stencil_state.enable_depth_test = key->depth && key->zTest;
    info.depth_stencil_state.enable_depth_write = key->depth && key->zTest && key->zWrite;
    info.depth_stencil_state.compare_op = compareOp(key->zFunc);
    target.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    target.blend_state.enable_blend = key->blend != 0;
    target.blend_state.src_color_blendfactor = blendFactor(key->src);
    target.blend_state.dst_color_blendfactor = blendFactor(key->dst);
    target.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    target.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    target.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    target.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    target.blend_state.enable_color_write_mask = true;
    target.blend_state.color_write_mask =
        SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G | SDL_GPU_COLORCOMPONENT_B;
    info.target_info.color_target_descriptions = &target;
    info.target_info.num_color_targets = 1;
    info.target_info.has_depth_stencil_target = key->depth != 0;
    info.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
    pipeline = DRV_FPU_GUARDED(SDL_CreateGPUGraphicsPipeline(drvGpuDevice(), &info));
    if (pipeline == NULL) {
        bmxLog("driver: SDL_CreateGPUGraphicsPipeline failed: %s\n", SDL_GetError());
        return NULL;
    }
    if (draw.numPipelines < (int)(sizeof draw.pipelines / sizeof draw.pipelines[0])) {
        draw.pipelines[draw.numPipelines].key = *key;
        draw.pipelines[draw.numPipelines].pipeline = pipeline;
        draw.numPipelines++;
    }
    return pipeline;
}

/* Stage 0's sampler: D3DTFG_LINEAR (2) magnification filters, D3DTADDRESS_WRAP/MIRROR/CLAMP/BORDER (1..4). */
static SDL_GPUSampler *samplerFor(int forceLinear)
{
    const int linear = forceLinear || tss[kTssMagFilter] == 2 || tss[kTssMinFilter] == 2;
    const uint32_t address = tss[kTssAddress] >= 1 && tss[kTssAddress] <= 4 ? tss[kTssAddress] - 1 : 0;
    SDL_GPUSampler **slot = &draw.samplers[linear][address];

    if (*slot == NULL) {
        static const SDL_GPUSamplerAddressMode kModes[4] = {
            SDL_GPU_SAMPLERADDRESSMODE_REPEAT, SDL_GPU_SAMPLERADDRESSMODE_MIRRORED_REPEAT,
            SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE, SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE};
        SDL_GPUSamplerCreateInfo info;
        SDL_zero(info);
        info.min_filter = info.mag_filter = linear ? SDL_GPU_FILTER_LINEAR : SDL_GPU_FILTER_NEAREST;
        info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
        info.address_mode_u = info.address_mode_v = info.address_mode_w = kModes[address];
        *slot = DRV_FPU_GUARDED(SDL_CreateGPUSampler(drvGpuDevice(), &info));
    }
    return *slot;
}

/* ---- draws
 * ------------------------------------------------------------------------------------------------- */

/* D3DPRIMITIVETYPE: 2 line list, 3 line strip, 4 triangle list, 5 strip, 6 fan. */
static void appendDraw(uint32_t type, const RwD3DTLVertex *vertices, uint32_t numVertices,
                       const uint16_t *indices, uint32_t numIndices)
{
    const uint32_t first = draw.numIndices;
    const int lines = type == 2 || type == 3;
    const uint32_t n = indices != NULL ? numIndices : numVertices;
    PipelineKey key;
    DrawCmd *cmd;
    int mode;

    if (draw.color == NULL || n == 0 || numVertices == 0 || numVertices > 0x10000) {
        return;
    }
    draw.vertices =
        grow(draw.vertices, &draw.capVertices, draw.numVertices + numVertices, sizeof *draw.vertices);
    memcpy(draw.vertices + draw.numVertices, vertices, numVertices * sizeof *vertices);
    draw.indices = grow(draw.indices, &draw.capIndices, draw.numIndices + 3 * n, sizeof *draw.indices);
#define IDX(i) (indices != NULL ? indices[i] : (uint16_t)(i))
    switch (type) {
    case 2:
    case 4: {
        const uint32_t k = type == 2 ? 2 : 3;
        for (uint32_t i = 0; i < n / k * k; i++) {
            draw.indices[draw.numIndices++] = IDX(i);
        }
        break;
    }
    case 3:
        for (uint32_t i = 0; i + 1 < n; i++) {
            draw.indices[draw.numIndices++] = IDX(i);
            draw.indices[draw.numIndices++] = IDX(i + 1);
        }
        break;
    case 5:
        for (uint32_t i = 0; i + 2 < n; i++) {
            draw.indices[draw.numIndices++] = IDX(i + (i & 1));
            draw.indices[draw.numIndices++] = IDX(i + 1 - (i & 1));
            draw.indices[draw.numIndices++] = IDX(i + 2);
        }
        break;
    case 6:
        for (uint32_t i = 1; i + 1 < n; i++) {
            draw.indices[draw.numIndices++] = IDX(0);
            draw.indices[draw.numIndices++] = IDX(i);
            draw.indices[draw.numIndices++] = IDX(i + 1);
        }
        break;
    default:
        break;
    }
#undef IDX
    if (draw.numIndices == first) {
        return;
    }
    memset(&key, 0, sizeof key);
    key.lines = (uint8_t)lines;
    key.blend = rs[kRsAlphaBlendEnable] != 0;
    key.src = (uint8_t)rs[kRsSrcBlend];
    key.dst = (uint8_t)rs[kRsDestBlend];
    key.zTest = rs[kRsZEnable] != 0;
    key.zWrite = rs[kRsZWriteEnable] != 0;
    key.zFunc = (uint8_t)rs[kRsZFunc];
    key.cull = (uint8_t)rs[kRsCullMode];
    key.depth = draw.depth != NULL;
    /* The 2D funnel's sampling (drvDrawSetUiMode): the game's nearest sampling becomes linear (1) or sharp
     * bilinear (2); the blurred fill (3) always. Draws the game already samples linearly are left alone. */
    {
        const int nearest = tss[kTssMagFilter] != 2 && tss[kTssMinFilter] != 2;

        mode = boundTexture == NULL ? 0 : uiMode == 3 ? 3 : nearest ? uiMode : 0;
    }
    key.ui = mode == 2 || mode == 3;
    if (mode == 3) {
        key.blend = 0; /* the blurred fill replaces what was there (a stale frame must not show through) */
    }
    draw.draws = grow(draw.draws, &draw.capDraws, draw.numDraws + 1, sizeof *draw.draws);
    cmd = &draw.draws[draw.numDraws++];
    memset(cmd, 0, sizeof *cmd);
    cmd->pipeline = pipelineFor(&key);
    cmd->texture = boundTexture;
    cmd->sampler = samplerFor(mode != 0);
    cmd->ps.params[3] = key.ui ? (float)mode : 0.0f;
    cmd->ps.fogColor[3] = mode == 3 ? 0.5f : 1.0f;
    cmd->ps.ops[0] = (int32_t)tss[kTssColorOp];
    cmd->ps.ops[1] = (int32_t)tss[kTssAlphaOp];
    cmd->ps.ops[2] = boundTexture != NULL;
    cmd->ps.ops[3] = rs[kRsAlphaTestEnable] != 0 && mode != 3 ? (int32_t)rs[kRsAlphaFunc] : 8;
    cmd->ps.params[0] = (float)(rs[kRsAlphaRef] & 0xff) / 255.0f;
    cmd->ps.params[1] = rs[kRsFogEnable] != 0 ? 1.0f : 0.0f;
    cmd->ps.params[2] = rs[kRsSpecularEnable] != 0 ? 1.0f : 0.0f;
    cmd->ps.fogColor[0] = (float)(rs[kRsFogColor] >> 16 & 0xff) / 255.0f;
    cmd->ps.fogColor[1] = (float)(rs[kRsFogColor] >> 8 & 0xff) / 255.0f;
    cmd->ps.fogColor[2] = (float)(rs[kRsFogColor] & 0xff) / 255.0f;
    cmd->firstIndex = first;
    cmd->numIndices = draw.numIndices - first;
    cmd->vertexOffset = (int32_t)draw.numVertices;
    draw.numVertices += numVertices;
}

int rwD3DDevDrawPrimitive(uint32_t type, uint32_t fvf, const void *vertices, uint32_t count, uint32_t flags)
{
    DRV_TRACE("rwD3DDevDrawPrimitive", type, count, 0);
    appendDraw(type, vertices, count, NULL, 0);
    return 0;
}

int rwD3DDevDrawIndexedPrimitive(uint32_t type, uint32_t fvf, const void *vertices, uint32_t numVertices,
                                 const uint16_t *indices, uint32_t numIndices, uint32_t flags)
{
    DRV_TRACE("rwD3DDevDrawIndexedPrimitive", type, numVertices, numIndices);
    appendDraw(type, vertices, numVertices, indices, numIndices);
    return 0;
}

/* Grows a GPU buffer to `size` bytes (contents are rewritten every flush). */
static SDL_GPUBuffer *gpuBuffer(SDL_GPUBuffer *buffer, uint32_t *have, uint32_t size,
                                SDL_GPUBufferUsageFlags usage)
{
    if (buffer != NULL && *have >= size) {
        return buffer;
    }
    if (buffer != NULL) {
        SDL_ReleaseGPUBuffer(drvGpuDevice(), buffer);
    }
    SDL_GPUBufferCreateInfo info;
    SDL_zero(info);
    info.usage = usage;
    info.size = size < 0x10000 ? 0x10000 : size * 2;
    *have = info.size;
    return DRV_FPU_GUARDED(SDL_CreateGPUBuffer(drvGpuDevice(), &info));
}

/* Uploads the batch and draws it into the target; called before anything else touches the target. */
void drvDrawFlush(void)
{
    SDL_GPUDevice *device = drvGpuDevice();
    SDL_GPUCommandBuffer *cmd;
    SDL_GPUTransferBuffer *transfer;
    SDL_GPUTransferBufferCreateInfo tinfo;
    SDL_GPUCopyPass *copy;
    SDL_GPURenderPass *pass;
    SDL_GPUColorTargetInfo target;
    SDL_GPUDepthStencilTargetInfo depthTarget;
    const uint32_t vbytes = draw.numVertices * sizeof(RwD3DTLVertex);
    const uint32_t ibytes = (draw.numIndices * 2 + 3) & ~3u;
    float vsUniforms[4];

    if (draw.numDraws == 0 || device == NULL || draw.color == NULL) {
        draw.numDraws = draw.numVertices = draw.numIndices = 0;
        return;
    }
    draw.vertexBuffer =
        gpuBuffer(draw.vertexBuffer, &draw.vertexBufferSize, vbytes, SDL_GPU_BUFFERUSAGE_VERTEX);
    draw.indexBuffer = gpuBuffer(draw.indexBuffer, &draw.indexBufferSize, ibytes, SDL_GPU_BUFFERUSAGE_INDEX);
    SDL_zero(tinfo);
    tinfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tinfo.size = vbytes + ibytes;
    transfer = DRV_FPU_GUARDED(SDL_CreateGPUTransferBuffer(device, &tinfo));
    cmd = SDL_AcquireGPUCommandBuffer(device);
    if (transfer == NULL || cmd == NULL || draw.vertexBuffer == NULL || draw.indexBuffer == NULL) {
        bmxLog("driver: draw flush failed: %s\n", SDL_GetError());
        draw.numDraws = draw.numVertices = draw.numIndices = 0;
        return;
    }
    uint8_t *mapped = SDL_MapGPUTransferBuffer(device, transfer, false);
    memcpy(mapped, draw.vertices, vbytes);
    memcpy(mapped + vbytes, draw.indices, draw.numIndices * 2);
    SDL_UnmapGPUTransferBuffer(device, transfer);
    copy = SDL_BeginGPUCopyPass(cmd);
    SDL_GPUTransferBufferLocation src = {transfer, 0};
    SDL_GPUBufferRegion dst = {draw.vertexBuffer, 0, vbytes};
    SDL_UploadToGPUBuffer(copy, &src, &dst, false);
    src.offset = vbytes;
    dst = (SDL_GPUBufferRegion){draw.indexBuffer, 0, ibytes};
    SDL_UploadToGPUBuffer(copy, &src, &dst, false);
    for (uint32_t i = 0; i < draw.numDraws; i++) {
        if (draw.draws[i].texture != NULL) {
            textureFor(draw.draws[i].texture, copy);
        }
    }
    SDL_EndGPUCopyPass(copy);
    SDL_ReleaseGPUTransferBuffer(device, transfer);

    SDL_zero(target);
    target.texture = draw.color;
    target.load_op = SDL_GPU_LOADOP_LOAD;
    target.store_op = SDL_GPU_STOREOP_STORE;
    SDL_zero(depthTarget);
    depthTarget.texture = draw.depth;
    depthTarget.load_op = SDL_GPU_LOADOP_LOAD;
    depthTarget.store_op = SDL_GPU_STOREOP_STORE;
    depthTarget.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
    depthTarget.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
    pass = SDL_BeginGPURenderPass(cmd, &target, 1, draw.depth != NULL ? &depthTarget : NULL);
    SDL_SetGPUScissor(pass, &draw.scissor);
    SDL_GPUBufferBinding vb = {draw.vertexBuffer, 0};
    SDL_GPUBufferBinding ib = {draw.indexBuffer, 0};
    SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);
    SDL_BindGPUIndexBuffer(pass, &ib, SDL_GPU_INDEXELEMENTSIZE_16BIT);
    vsUniforms[0] = 2.0f / (float)draw.width;
    vsUniforms[1] = -2.0f / (float)draw.height;
    vsUniforms[2] = vsUniforms[3] = 0.0f;
    SDL_PushGPUVertexUniformData(cmd, 0, vsUniforms, sizeof vsUniforms);
    for (uint32_t i = 0; i < draw.numDraws; i++) {
        const DrawCmd *d = &draw.draws[i];
        SDL_GPUTextureSamplerBinding binding;
        if (d->pipeline == NULL) {
            continue;
        }
        binding.texture = d->texture != NULL ? textureFor(d->texture, NULL) : draw.white;
        binding.sampler = d->sampler;
        if (binding.texture == NULL || binding.sampler == NULL) {
            continue;
        }
        SDL_BindGPUGraphicsPipeline(pass, d->pipeline);
        SDL_BindGPUFragmentSamplers(pass, 0, &binding, 1);
        SDL_PushGPUFragmentUniformData(cmd, 0, &d->ps, sizeof d->ps);
        SDL_DrawGPUIndexedPrimitives(pass, d->numIndices, 1, d->firstIndex, d->vertexOffset, 0);
    }
    SDL_EndGPURenderPass(pass);
    if (!DRV_FPU_GUARDED(SDL_SubmitGPUCommandBuffer(cmd))) {
        bmxLog("driver: SDL_SubmitGPUCommandBuffer failed: %s\n", SDL_GetError());
    }
    draw.numDraws = draw.numVertices = draw.numIndices = 0;
}

/* The target of the following draws (camera begin update); flushes draws for another target first. */
void drvDrawSetTarget(struct SDL_GPUTexture *color, struct SDL_GPUTexture *depth, int32_t width,
                      int32_t height, int32_t x, int32_t y, int32_t w, int32_t h)
{
    if (color != draw.color || depth != draw.depth) {
        drvDrawFlush();
    }
    draw.color = color;
    draw.depth = depth;
    draw.width = width;
    draw.height = height;
    draw.scissor = (SDL_Rect){x, y, w, h};
}

/* How the 2D funnel's textured draws that follow are sampled (src/driver/wide.c): 0 as the game set it, 1
 * linear, 2 sharp bilinear, 3 the blurred, darkened fill behind a pillarboxed backdrop. */
void drvDrawSetUiMode(int mode)
{
    uiMode = mode;
}

/* An opaque black screen rectangle through the normal draw path (the bars around a pillarboxed backdrop). */
void drvDrawFillRect(float x0, float y0, float x1, float y1)
{
    const uint32_t keep[] = {rs[kRsZEnable],          rs[kRsZWriteEnable], rs[kRsAlphaTestEnable],
                             rs[kRsAlphaBlendEnable], rs[kRsFogEnable],    rs[kRsSpecularEnable],
                             rs[kRsCullMode],         tss[kTssColorOp],    tss[kTssAlphaOp]};
    RwRaster *const texture = boundTexture;
    const RwD3DTLVertex quad[4] = {
        {x0, y0, 0.0f, 1.0f, 0xff000000u, 0xff000000u, 0.0f, 0.0f},
        {x1, y0, 0.0f, 1.0f, 0xff000000u, 0xff000000u, 0.0f, 0.0f},
        {x1, y1, 0.0f, 1.0f, 0xff000000u, 0xff000000u, 0.0f, 0.0f},
        {x0, y1, 0.0f, 1.0f, 0xff000000u, 0xff000000u, 0.0f, 0.0f},
    };

    rs[kRsZEnable] = rs[kRsZWriteEnable] = rs[kRsAlphaTestEnable] = rs[kRsAlphaBlendEnable] = 0;
    rs[kRsFogEnable] = rs[kRsSpecularEnable] = 0;
    rs[kRsCullMode] = 1;
    tss[kTssColorOp] = tss[kTssAlphaOp] = 3; /* the vertex colour */
    boundTexture = NULL;
    appendDraw(6, quad, 4, NULL, 0);
    boundTexture = texture;
    rs[kRsZEnable] = keep[0];
    rs[kRsZWriteEnable] = keep[1];
    rs[kRsAlphaTestEnable] = keep[2];
    rs[kRsAlphaBlendEnable] = keep[3];
    rs[kRsFogEnable] = keep[4];
    rs[kRsSpecularEnable] = keep[5];
    rs[kRsCullMode] = keep[6];
    tss[kTssColorOp] = keep[7];
    tss[kTssAlphaOp] = keep[8];
}

/* A texture's CPU copy changed: draws already batched must see the old pixels, so flush them first. */
void drvDrawTextureChanged(RwRaster *raster)
{
    for (uint32_t i = 0; i < draw.numDraws; i++) {
        if (draw.draws[i].texture != NULL && drvTopRaster(draw.draws[i].texture) == drvTopRaster(raster)) {
            drvDrawFlush();
            break;
        }
    }
    drvRasterExt(drvTopRaster(raster))->dirty = 1;
}

/* A raster goes away: forget it as the bound texture and drop draws that use it. */
void drvDrawForgetRaster(RwRaster *raster)
{
    if (boundTexture == raster) {
        boundTexture = NULL;
    }
    for (uint32_t i = 0; i < draw.numDraws; i++) {
        if (draw.draws[i].texture == raster) {
            drvDrawFlush();
            break;
        }
    }
}

/* Shaders and the white texture, once the GPU device exists. */
int drvDrawStart(void)
{
    SDL_GPUDevice *device = drvGpuDevice();
    const SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(device);
    SDL_GPUShaderCreateInfo info;

    SDL_zero(info);
    /* Per shader: DXIL (Direct3D 12), SPIR-V (Vulkan), MSL source text (Metal; docs/PORTING.md X5-A). */
    const unsigned char *vsCode, *psCode, *uiCode;
    size_t vsSize, psSize, uiSize;
    if ((formats & SDL_GPU_SHADERFORMAT_DXIL) != 0) {
        info.format = SDL_GPU_SHADERFORMAT_DXIL;
        vsCode = tl_vs_dxil;
        vsSize = sizeof tl_vs_dxil;
        psCode = tl_ps_dxil;
        psSize = sizeof tl_ps_dxil;
        uiCode = ui_ps_dxil;
        uiSize = sizeof ui_ps_dxil;
    } else if ((formats & SDL_GPU_SHADERFORMAT_SPIRV) != 0) {
        info.format = SDL_GPU_SHADERFORMAT_SPIRV;
        vsCode = tl_vs_spirv;
        vsSize = sizeof tl_vs_spirv;
        psCode = tl_ps_spirv;
        psSize = sizeof tl_ps_spirv;
        uiCode = ui_ps_spirv;
        uiSize = sizeof ui_ps_spirv;
    } else if ((formats & SDL_GPU_SHADERFORMAT_MSL) != 0) {
        info.format = SDL_GPU_SHADERFORMAT_MSL;
        vsCode = tl_vs_msl;
        vsSize = sizeof tl_vs_msl;
        psCode = tl_ps_msl;
        psSize = sizeof tl_ps_msl;
        uiCode = ui_ps_msl;
        uiSize = sizeof ui_ps_msl;
    } else {
        bmxLog("driver: no shader format for this GPU driver (formats %#x)\n", (unsigned)formats);
        return 0;
    }
    info.entrypoint = "vsMain";
    info.code = vsCode;
    info.code_size = vsSize;
    info.stage = SDL_GPU_SHADERSTAGE_VERTEX;
    info.num_uniform_buffers = 1;
    draw.vs = DRV_FPU_GUARDED(SDL_CreateGPUShader(device, &info));
    info.entrypoint = "psMain";
    info.code = psCode;
    info.code_size = psSize;
    info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    info.num_samplers = 1;
    info.num_uniform_buffers = 1;
    draw.ps = DRV_FPU_GUARDED(SDL_CreateGPUShader(device, &info));
    info.code = uiCode;
    info.code_size = uiSize;
    draw.psUi = DRV_FPU_GUARDED(SDL_CreateGPUShader(device, &info));
    if (draw.vs == NULL || draw.ps == NULL || draw.psUi == NULL) {
        bmxLog("driver: SDL_CreateGPUShader failed: %s\n", SDL_GetError());
        return 0;
    }
    SDL_GPUTextureCreateInfo tinfo;
    SDL_zero(tinfo);
    tinfo.type = SDL_GPU_TEXTURETYPE_2D;
    tinfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    tinfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    tinfo.width = tinfo.height = 1;
    tinfo.layer_count_or_depth = tinfo.num_levels = 1;
    draw.white = DRV_FPU_GUARDED(SDL_CreateGPUTexture(device, &tinfo));
    return draw.white != NULL;
}

void drvDrawStop(void)
{
    SDL_GPUDevice *device = drvGpuDevice();

    for (int i = 0; i < draw.numPipelines; i++) {
        SDL_ReleaseGPUGraphicsPipeline(device, draw.pipelines[i].pipeline);
    }
    draw.numPipelines = 0;
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 4; j++) {
            if (draw.samplers[i][j] != NULL) {
                SDL_ReleaseGPUSampler(device, draw.samplers[i][j]);
                draw.samplers[i][j] = NULL;
            }
        }
    }
    SDL_ReleaseGPUShader(device, draw.vs);
    SDL_ReleaseGPUShader(device, draw.ps);
    SDL_ReleaseGPUShader(device, draw.psUi);
    SDL_ReleaseGPUTexture(device, draw.white);
    SDL_ReleaseGPUBuffer(device, draw.vertexBuffer);
    SDL_ReleaseGPUBuffer(device, draw.indexBuffer);
    memset(&draw, 0, sizeof draw);
}
