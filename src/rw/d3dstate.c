/*
 * RenderWare 3.1's Direct3D 7 driver, render states (lib/d3d7driver): RwRenderStateSet / Get of the device.
 * Each state is checked against the device caps, cached (rwD3DState) and sent to Direct3D only when it
 * changes. Fog density (0x4d6590, x87 exp tables) is still the original's.
 */
#include "rw/rwd3d.h"

ORIG_FN_REGPARM(1, 0x004d6590, int, rwD3DSetFogDensity, (const float *density));

/* D3DRENDERSTATETYPE and D3DTEXTURESTAGESTATETYPE values the driver sends. */
enum {
    kRsTexturePerspective = 4,
    kRsZEnable = 7,
    kRsShadeMode = 9,
    kRsZWriteEnable = 0xe,
    kRsAlphaTestEnable = 0xf,
    kRsSrcBlend = 0x13,
    kRsDestBlend = 0x14,
    kRsAlphaBlendEnable = 0x1b,
    kRsFogEnable = 0x1c,
    kRsFogColor = 0x22,
    kRsColorKeyEnable = 0x29,
    kTssColorOp = 1,
    kTssAlphaOp = 4,
    kTssAddress = 0xc,
    kTssBorderColor = 0xf,
    kTssMagFilter = 0x10,
    kTssMinFilter = 0x11,
    kTssMipFilter = 0x12,
    kTopSelectArg1 = 2,
    kTopSelectArg2 = 3,
    kTopModulate = 4,
};

/* Sends a render state when it differs from the cached value. */
static void setState(int32_t *cache, uint32_t state, int32_t value)
{
    if (*cache != value) {
        rwD3DDevSetRenderState(state, (uint32_t)value);
        *cache = value;
    }
}

static void setStage(int32_t *cache, uint32_t type, int32_t value)
{
    if (*cache != value) {
        rwD3DDevSetTextureStageState(0, type, (uint32_t)value);
        *cache = value;
    }
}

/*
 * Blending follows vertex and texture alpha: alpha blend on if either has alpha, alpha test with it when the
 * device can, and the stage's alpha from the texture, the vertices or both.
 */
static void updateAlpha(void)
{
    RwD3DStateCache *s = &rwD3DState;
    const int32_t blend = s->vertexAlpha != 0 || s->textureAlpha != 0;
    const int32_t test = blend && rwD3DAlphaTestCaps != 0;

    setState(&s->alphaBlend, kRsAlphaBlendEnable, blend);
    if (test != s->alphaTest) {
        rwD3DDevSetRenderState(kRsAlphaTestEnable, (uint32_t)test);
        s->alphaTest = test;
    }
    if (s->textureAlpha != 0 && s->vertexAlpha != 0) {
        setStage(&s->alphaOp, kTssAlphaOp, kTopModulate);
    } else if (s->textureAlpha != 0) {
        setStage(&s->alphaOp, kTssAlphaOp, kTopSelectArg1);
    } else if (s->vertexAlpha != 0) {
        setStage(&s->alphaOp, kTssAlphaOp, kTopSelectArg2);
    }
}

/* 0x4ef3d0: the Direct3D texture of a raster (+0xc of the driver's raster data). */
BMX_ADDR(0x004ef3d0)
void *rwD3DRasterHandle(const RwRaster *raster)
{
    return ((const RwD3DRasterExt *)((const uint8_t *)raster + rwD3DRasterExtOffset))->handle;
}

/* 0x4d6840 (raster in EAX): rwRENDERSTATETEXTURERASTER. NULL draws with the vertex colours only. */
BMX_ADDR(0x004d6840)
BMX_REGPARM(1) int rwD3DSetTextureRaster(RwRaster *raster)
{
    RwD3DStateCache *s = &rwD3DState;
    const int32_t offset = rwD3DRasterExtOffset;

    if (raster == NULL) {
        rwD3DDevSetTexture(0, NULL);
        setStage(&s->colorOp, kTssColorOp, kTopSelectArg2);
        s->textureAlpha = 0;
        if (s->colorKey != 0) {
            rwD3DDevSetRenderState(kRsColorKeyEnable, 0);
            s->colorKey = 0;
        }
    } else {
        const RwD3DRasterExt *ext = (const RwD3DRasterExt *)((const uint8_t *)raster + offset);
        const RwD3DRasterInfo *info = ext->info;
        int32_t key;

        rwD3DDevSetTexture(0, rwD3DRasterHandle(raster));
        setStage(&s->colorOp, kTssColorOp, kTopModulate);
        s->textureAlpha = (int32_t)(info->flags & 1);
        key = ext->colorKey != 0;
        if (key != s->colorKey) {
            rwD3DDevSetRenderState(kRsColorKeyEnable, (uint32_t)key);
            s->colorKey = *(const int32_t *)((const uint8_t *)raster + offset + 4) != 0;
        }
    }
    s->textureRaster = raster;
    updateAlpha();
    return 1;
}

/* 0x4d6ba0 (blend in EAX): rwRENDERSTATESRCBLEND, rwBLEND* 1..11 if the device has the matching cap. */
BMX_ADDR(0x004d6ba0)
BMX_REGPARM(1) int rwD3DSetSrcBlend(int32_t blend)
{
    if (blend < 1 || blend > 11 || (rwD3DCap(rwD3DCAPSSRCBLEND) & (1u << (blend - 1))) == 0) {
        return 0;
    }
    /* rwBLENDDESTALPHA (6) goes out as D3DBLEND_ZERO, as in the original. */
    setState(&rwD3DState.srcBlend, kRsSrcBlend, blend == 6 ? 1 : blend);
    return 1;
}

/* 0x4d6f60 (blend in EAX): rwRENDERSTATEDESTBLEND. */
BMX_ADDR(0x004d6f60)
BMX_REGPARM(1) int rwD3DSetDestBlend(int32_t blend)
{
    if (blend < 1 || blend > 11 || (rwD3DCap(rwD3DCAPSDESTBLEND) & (1u << (blend - 1))) == 0) {
        return 0;
    }
    setState(&rwD3DState.destBlend, kRsDestBlend, blend);
    return 1;
}

/* 0x4d6530 (type in EAX): copies the fog table of a fog type (1 linear, 2 exponential, 3 squared). */
BMX_ADDR(0x004d6530)
BMX_REGPARM(1) int rwD3DFogTableSelect(int32_t type)
{
    const uint8_t *src;

    if (type == 1) {
        src = rwD3DFogLinear;
    } else if (type == 2) {
        src = rwD3DFogExp;
    } else if (type == 3) {
        src = rwD3DFogExp2;
    } else {
        return 0;
    }
    __builtin_memcpy(rwD3DState.fogTable, src, sizeof rwD3DState.fogTable);
    return 1;
}

/* The specular colours of the fog indices: alpha = 255 - fog table entry. */
static void buildFogSpecular(void)
{
    for (int i = 0; i < 256; i++) {
        rwD3DFogSpecular[i] = (uint32_t)(rwD3DState.fogTable[i] ^ 0xff) << 24;
    }
}

/* 0x4d6740 (type in EAX): rwRENDERSTATEFOGTYPE, if the device fogs vertices. */
BMX_ADDR(0x004d6740)
BMX_REGPARM(1) int rwD3DSetFogType(int32_t type)
{
    if ((rwD3DCap(rwD3DCAPSSHADE) & 0xc0000) == 0 || rwD3DFogTableSelect(type) == 0) {
        return 0;
    }
    buildFogSpecular();
    rwD3DState.fogType = type;
    return 1;
}

/* 0x4d67c0 (table in EAX): rwRENDERSTATEFOGTABLE, 256 bytes of the game's own. */
BMX_ADDR(0x004d67c0)
BMX_REGPARM(1) int rwD3DSetFogTable(const uint32_t *table)
{
    if ((rwD3DCap(rwD3DCAPSSHADE) & 0xc0000) == 0) {
        return 0;
    }
    __builtin_memcpy(rwD3DState.fogTable, table, sizeof rwD3DState.fogTable);
    buildFogSpecular();
    rwD3DState.fogType = 0;
    return 1;
}

/* rwRENDERSTATETEXTUREFILTER: mag, min and mip filter per rwFILTER* 1..6, as far as the device can. */
static int setFilter(int32_t filter)
{
    const uint32_t caps = rwD3DCap(rwD3DCAPSFILTER);
    int32_t mag = 1, min = 1, mip = 1;

    switch (filter) {
    case 1:
        break;
    case 6: /* linear mip linear: falls back along the device's mip caps */
        if ((caps & 0x20) != 0) {
            mag = 2, min = 2, mip = 3;
            break;
        }
        /* fall through */
    case 4:
        if ((caps & 8) != 0) {
            mag = 2, min = 2, mip = 2;
            break;
        }
        /* fall through */
    case 5:
        if ((caps & 0x10) != 0) {
            mag = 1, min = 1, mip = 3;
            break;
        }
        /* fall through */
    case 3:
        if ((caps & 4) != 0) {
            mag = 1, min = 1, mip = 2;
            break;
        }
        /* fall through */
    case 2:
        if ((caps & 2) != 0) {
            mag = 2, min = 2, mip = 1;
        }
        break;
    default:
        return 0;
    }
    setStage(&rwD3DState.magFilter, kTssMagFilter, mag);
    setStage(&rwD3DState.minFilter, kTssMinFilter, min);
    setStage(&rwD3DState.mipFilter, kTssMipFilter, mip);
    return 1;
}

/*
 * 0x4d7328 (state in EAX, value in EDX): the device's render state set behind 0x4d7320. 0 outside a camera
 * update, for unknown states and for values the device cannot do.
 */
BMX_ADDR(0x004d7328)
BMX_REGPARM(2) int rwD3DSetRenderState(int32_t state, intptr_t arg)
{
    RwD3DStateCache *s = &rwD3DState;
    const int32_t value = (int32_t)arg; /* the number of the states that take one */

    if (rwD3DContext == NULL) {
        return 0;
    }
    switch (state) {
    case 1:
        rwD3DSetTextureRaster((RwRaster *)arg);
        return 1;
    case 2: { /* texture address: wrap, mirror, clamp, border */
        static const uint32_t kCaps[5] = {0, 1, 2, 4, 8};
        if (value < 1 || value > 4 || (rwD3DCap(rwD3DCAPSADDRESS) & kCaps[value]) == 0) {
            return 0;
        }
        setStage(&s->address, kTssAddress, value);
        return 1;
    }
    case 3:
        if ((rwD3DCap(rwD3DCAPSTEXTURE) & 1) == 0 && value != 0) {
            return 0;
        }
        setState(&s->perspective, kRsTexturePerspective, value);
        return 1;
    case 4:
        if (rwD3DCap(rwD3DCAPSZCMP) == 0) {
            return 0;
        }
        if (value == 0) {
            rwD3DDeferFlush();
            setState(&s->zTest, kRsZEnable, 0);
        } else {
            setState(&s->zTest, kRsZEnable, 1);
        }
        return 1;
    case 5:
        if (value == 1) {
            if ((rwD3DCap(rwD3DCAPSSHADE) & 2) == 0) {
                return 0;
            }
            setState(&s->shadeMode, kRsShadeMode, 1);
            return 1;
        }
        if ((rwD3DCap(rwD3DCAPSSHADE) & 8) == 0) {
            return 0;
        }
        setState(&s->shadeMode, kRsShadeMode, 2);
        return 1;
    case 6:
        if (rwD3DCap(rwD3DCAPSZCMP) == 0) {
            return 0;
        }
        setState(&s->zWrite, kRsZWriteEnable, value != 0);
        return 1;
    case 7:
        return setFilter(value);
    case 8:
        return rwD3DSetSrcBlend(value);
    case 9:
        return rwD3DSetDestBlend(value);
    case 10:
        s->vertexAlpha = value != 0;
        if ((rwD3DCap(rwD3DCAPSSHADE) & 0x5000) == 0) {
            return 0;
        }
        updateAlpha();
        return 1;
    case 11:
        if ((rwD3DCap(rwD3DCAPSTEXTURE) & 0x10) == 0) {
            return 0;
        }
        setStage(&s->borderColor, kTssBorderColor, value);
        return 1;
    case 12:
        if ((rwD3DCap(rwD3DCAPSSHADE) & 0xc0000) == 0) {
            return 0;
        }
        setState(&s->fogEnable, kRsFogEnable, value != 0);
        return 1;
    case 13:
        if ((rwD3DCap(rwD3DCAPSSHADE) & 0xc0000) == 0) {
            return 0;
        }
        setState(&s->fogColor, kRsFogColor, value);
        return 1;
    case 14:
        return rwD3DSetFogType(value);
    case 15:
        return rwD3DSetFogDensity((const float *)arg);
    case 16:
        return rwD3DSetFogTable((const uint32_t *)arg);
    case 17:
        if (s->alphaSort == 0) {
            if (value != 0) {
                s->alphaSort = 1;
            }
        } else if (value == 0) {
            rwD3DDeferFlush();
            s->alphaSort = 0;
        }
        return 1;
    default:
        return 0;
    }
}

/* 0x4d7320: RwDevice.fpRenderStateSet, the stack entry of 0x4d7328. */
BMX_ADDR(0x004d7320)
int rwD3DRenderStateSet(int32_t state, intptr_t value)
{
    return rwD3DSetRenderState(state, value);
}

/* 0x4d7c10: RwDevice.fpRenderStateGet, the cached values in RenderWare's terms. */
BMX_ADDR(0x004d7c10)
int rwD3DRenderStateGet(int32_t state, void *value)
{
    const RwD3DStateCache *s = &rwD3DState;
    int32_t *out = value;

    switch (state) {
    case 1:
        *(RwRaster **)value = s->textureRaster; /* a pointer */
        return 1;
    case 2:
        if (s->address < 1 || s->address > 4) {
            return 0;
        }
        *out = s->address;
        return 1;
    case 3:
        *out = s->perspective;
        return 1;
    case 4:
        *out = s->zTest;
        return 1;
    case 5:
        *out = s->shadeMode == 2 ? 2 : 1;
        return 1;
    case 6:
        *out = s->zWrite;
        return 1;
    case 7: { /* mip filter none / point / linear times mag filter point / linear */
        if (s->mipFilter < 1 || s->mipFilter > 3 || (s->magFilter != 1 && s->magFilter != 2)) {
            return 0;
        }
        *out = (s->mipFilter - 1) * 2 + s->magFilter;
        return 1;
    }
    case 8:
        if (s->srcBlend < 1 || s->srcBlend > 11) {
            return 0;
        }
        *out = s->srcBlend;
        return 1;
    case 9:
        if (s->destBlend < 1 || s->destBlend > 11) {
            return 0;
        }
        *out = s->destBlend;
        return 1;
    case 10:
        *out = s->alphaBlend != 0;
        return 1;
    case 11:
        *out = s->borderColor;
        return 1;
    case 12:
        *out = s->fogEnable != 0;
        return 1;
    case 13:
        *out = s->fogColor;
        return 1;
    case 14:
        *out = s->fogType;
        return 1;
    case 15:
        __builtin_memcpy(out, &s->fogDensity, 4);
        return 1;
    case 16:
        *(const uint8_t **)value = s->fogTable; /* the table's address */
        return 1;
    case 17:
        *out = s->alphaSort;
        return 1;
    default:
        return 0;
    }
}
