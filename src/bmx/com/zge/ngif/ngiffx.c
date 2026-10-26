/* Particle effects (C:\zaxis\Bmx\src\com\zge\NgIF\NgIFfx.c). */
#include "bmx/com/zge/ngif/ngiffx.h"

#include "bmx/com/zge/ngif/ngifzroller.h"
#include "compat/crt.h"
#include "zge/com/zglrw/zrwzgl.h"
#include "zge/zdebug.h"
#include "zge/zgemods/zobjcamera.h"
#include "zge/zgemods/zobjpemit.h"
#include "zge/zlist.h"
#include "zge/zmem.h"
#include "zge/zobj.h"
#include "zge/zscene.h"
#include "zge/zstring.h"

static const char kFxFile[] = "C:\\zaxis\\Bmx\\src\\com\\zge\\NgIF\\NgIFfx.c";

/* One millisecond in seconds, as the float constant at 0x54dc04. */
static const float kMillisecond = 0x1.0624dep-10f;
/* 1/128 and 1/32 (0x54dc08, 0x54d07c): scale of a signed random byte. */
static const float kRandomScale = 0x1p-7f;
static const float kRandomScaleSmall = 0x1p-5f;

/* Effect handler tables in the exe's data and their default parameter blocks. */
typedef ZfxEffect ZfxEffectTable[7];
#define gFxSpark     ORIG_VAR(0x0055c278, ZfxEffect, gFxSpark)
#define gFxShard     ORIG_VAR(0x0055c2b0, ZfxEffect, gFxShard)
#define gFxSplash    ORIG_VAR(0x0055c2e8, ZfxEffect, gFxSplash)
#define gFxDust      ORIG_VAR(0x0055c320, ZfxEffect, gFxDust)
#define gFxPoster    ORIG_VAR(0x0055c358, ZfxEffect, gFxPoster)
#define gFxRipple    ORIG_VAR(0x0055c390, ZfxEffect, gFxRipple)
#define gFxLightning ORIG_VAR(0x0055c3c8, ZfxEffect, gFxLightning)
#define gFxGravity   ORIG_VAR(0x005623d8, uint8_t, gFxGravity)
#define gFxFloorColl ORIG_VAR(0x005623e0, uint8_t, gFxFloorColl)

typedef float ZfxParams[16];
#define gFxSparkDefault        ORIG_VAR(0x0055c128, ZfxParams, gFxSparkDefault)
#define gFxShardDefault        ORIG_VAR(0x0055c160, ZfxParams, gFxShardDefault)
#define gFxDustDefault         ORIG_VAR(0x0055c1a0, ZfxParams, gFxDustDefault)
#define gFxPosterDefault       ORIG_VAR(0x0055c1d0, ZfxParams, gFxPosterDefault)
#define gFxRippleDefault       ORIG_VAR(0x0055c220, ZfxParams, gFxRippleDefault)
#define gFxLightningDefault    ORIG_VAR(0x0055c248, ZfxParams, gFxLightningDefault)
#define gFxSparkCreateDefault  ORIG_VAR(0x0055c400, ZfxParams, gFxSparkCreateDefault)
#define gFxSplashCreateDefault ORIG_VAR(0x0055c438, ZfxParams, gFxSplashCreateDefault)
#define gFxGravityParams       ORIG_VAR(0x0055c470, ZfxParams, gFxGravityParams)

/* Quality adaption state. */
#define gFxQuality     ORIG_VAR(0x0055bd08, float, gFxQuality)
#define gFxFps         ORIG_VAR(0x0055bd0c, float, gFxFps)
#define gFxQualityTime ORIG_VAR(0x005ab49c, uint32_t, gFxQualityTime)
#define gFxFrames      ORIG_VAR(0x005ab4a0, int32_t, gFxFrames)
#define gFxAdjusting   ORIG_VAR(0x005ab4a4, int32_t, gFxAdjusting)
#define gFxQualityStep ORIG_VAR(0x005ab4a8, float, gFxQualityStep)
#define gFxDebugFloat  ORIG_VAR(0x005ab454, float, gFxDebugFloat)
#define gFxSceneObject ORIG_VAR(0x0068ff98, int32_t, gFxSceneObject)

/* Texture animation config: "ANIM" objects with frames named by a table of strings. */
typedef const char *ZfxNames[16];
#define gFxSkidConfig     ORIG_VAR(0x0055c120, uint32_t, gFxSkidConfig)
#define gFxSkidName       ORIG_VAR(0x0055c11c, const char *, gFxSkidName)
#define gFxSkidBase       ORIG_VAR(0x0055c0d8, const char *, gFxSkidBase)
#define gFxSkidFrameNames ORIG_VAR(0x0055c0dc, ZfxNames, gFxSkidFrameNames)

/* Textures looked up by name at registration. */
#define gFxSparkTexture  ORIG_VAR(0x005ab4e0, uint32_t, gFxSparkTexture)
#define gFxSkidTexture   ORIG_VAR(0x005ab4e4, uint32_t, gFxSkidTexture)
#define gFxRippleTexture ORIG_VAR(0x005ab4e8, uint32_t, gFxRippleTexture)
#define gFxSplashTexture ORIG_VAR(0x005ab4ec, uint32_t, gFxSplashTexture)
#define gFxCube          ORIG_VAR(0x005ab4f4, void *, gFxCube)
typedef uint32_t ZfxDustTextures[16];
#define gFxDustTextures ORIG_VAR(0x0068fc20, ZfxDustTextures, gFxDustTextures)
#define gFxWorldObject  ORIG_VAR(0x0068ff80, uint8_t *, gFxWorldObject)
#define gFxAnimHandle   ORIG_VAR(0x0068fcf8, int, gFxAnimHandle)

/* Renderer entry points reached through pointers in the exe's data. */
typedef void(BMX_CDECL *ZfxSetParamFn)(int id, void *model, const void *value);
typedef void(BMX_CDECL *ZfxDrawFn)(uint32_t flags, const void *primitives, int count, int a, int b);
typedef void *(BMX_CDECL *ZfxCameraFn)(void);
typedef ZObjCamera *(BMX_CDECL *ZfxCameraObjectFn)(void *camera);
typedef uint32_t(BMX_CDECL *ZfxTextureByHashFn)(int hash, int flags);
typedef uint32_t(BMX_CDECL *ZfxFindTextureFn)(void *library, int hash);
typedef uint32_t(BMX_CDECL *ZfxLoadTextureFn)(int hash);
typedef void *(BMX_CDECL *ZfxGetRasterFn)(uint32_t material, int index);
typedef void(BMX_CDECL *ZfxSetRasterFn)(uint32_t material, void *raster, int index);
#define gFxSetParam      ORIG_VAR(0x00561484, ZfxSetParamFn, gFxSetParam)
#define gFxDrawTriangles ORIG_VAR(0x0056172c, ZfxDrawFn, gFxDrawTriangles)
#define gFxDrawQuads     ORIG_VAR(0x00561738, ZfxDrawFn, gFxDrawQuads)
#define gFxDrawLines     ORIG_VAR(0x00561728, ZfxDrawFn, gFxDrawLines)
#define gFxGetCamera     ORIG_VAR(0x00561db0, ZfxCameraFn, gFxGetCamera)
#define gFxCameraObject  ORIG_VAR(0x00561db4, ZfxCameraObjectFn, gFxCameraObject)
#define gFxTextureByHash ORIG_VAR(0x0056104c, ZfxTextureByHashFn, gFxTextureByHash)
#define gFxFindTexture   ORIG_VAR(0x00560ee8, ZfxFindTextureFn, gFxFindTexture)
#define gFxLoadTexture   ORIG_VAR(0x005620ec, ZfxLoadTextureFn, gFxLoadTexture)
#define gFxGetRaster     ORIG_VAR(0x00561034, ZfxGetRasterFn, gFxGetRaster)
#define gFxSetRaster     ORIG_VAR(0x0056102c, ZfxSetRasterFn, gFxSetRaster)

/* Code the effects call in the exe: effect creation by name, modifier attach, axis matrix, scene calls. */
ORIG_FN(0x00495d50, void, ngiffxSpawn,
        (uint32_t handle, const char *name, int flags, int unused, const void *params));

/* Copies a 32 bit word between float / integer views without touching its bits. */
#define COPY32(dst, src) __builtin_memcpy(&(dst), &(src), 4)

static inline float loadFloat(const uint32_t *word)
{
    float f;

    __builtin_memcpy(&f, word, 4);
    return f;
}

static inline void storeFloat(uint32_t *word, float f)
{
    __builtin_memcpy(word, &f, 4);
}

BMX_ADDR(0x0045c690)
void ngiffxLoadAnimTable(const char *name, uint32_t *table, int size)
{
    void *library = zsceneFindAnim(0, name);

    if (library == NULL) {
        return;
    }
    zsceneAnimTable(library, table, size);
}

BMX_ADDR(0x0045c6d0)
float ngiffxGetDebugFloat(void)
{
    return gFxDebugFloat;
}

BMX_ADDR(0x0045c6f0)
void ngiffxNop(void) {}

/*
 * Adapts the effect quality (60..200) to the frame rate measured over 100 ms windows: it follows the
 * rate towards 28 fps, easing back to 200 when the rate is high enough, and settles when the quality is
 * within 10 of 200 and the rate above 26.
 */
BMX_ADDR(0x0045c710)
void ngiffxAdaptQuality(void)
{
    const uint32_t now = gTimeGetTime();
    uint32_t elapsed;
    int16_t frames;
    double sum;

    if (gFxQualityTime == 0) {
        ngiffxSetQuality(gFxQuality);
        gFxQualityTime = now;
        return;
    }
    gFxFrames++;
    elapsed = now - gFxQualityTime;
    if (elapsed < 100) {
        return;
    }
    frames = (int16_t)gFxFrames;
    gFxFrames = 0;
    gFxQualityTime = now;
    gFxFps = (float)((double)frames / ((double)(int16_t)elapsed * kMillisecond));
    if ((double)gFxFps > 26.0 && !(__builtin_fabs((double)gFxQuality - 200.0) >= 10.0)) {
        gFxAdjusting = 0;
        return;
    }
    if (gFxAdjusting == 0) {
        gFxQualityStep = (float)(((double)gFxFps - 28.0) * 10.0);
    } else if ((double)gFxFps - 26.0 >= 0.0) {
        gFxQualityStep = (float)((200.0 - (double)gFxQuality) * 0x1.b4e81cp-7f);
    } else {
        gFxQualityStep = (float)(((double)gFxFps - 28.0) * 0x1.99999ap-3f);
    }
    sum = (double)gFxQualityStep + (double)gFxQuality;
    gFxAdjusting = 1;
    gFxQuality = (float)sum;
    if (sum >= 200.0) {
        gFxQuality = 200.0f;
        ngiffxSetQuality(200.0f);
        return;
    }
    if (!((double)gFxQuality > 60.0)) {
        gFxQuality = 60.0f;
    }
    ngiffxSetQuality(gFxQuality);
}

BMX_ADDR(0x0045c920)
void ngiffxSetQuality(float quality)
{
    gFxSetParam(5, zobjGetModel((const void *)(intptr_t)gFxSceneObject), &quality);
}

/* Random byte minus 128 as a signed value, as the original's (short)_ftol of the double. */
static int16_t randomSigned(void)
{
    return (int16_t)bmxFtol((double)(zmathRandom() & 0xff) - 128.0);
}

/* The object's transform (+0x30), whose position sits at +8. */
static const float *objectTransform(const void *object)
{
    return *(const float *const *)((const uint8_t *)object + 0x30);
}

/* Spark spawner: moving particles with a random velocity around the base velocity. */
BMX_ADDR(0x0045c950)
void ngiffxSparkSpawn(int enabled, const void *object, ZfxEmitter *emitter, uint32_t count, int unused,
                      const float *params)
{
    const uint32_t now = gTimeGetTime();
    const float *const xf = objectTransform(object);
    const int given = params != NULL;

    if (!given) {
        params = gFxSparkDefault;
    }
    if (enabled == 0 || emitter == NULL) {
        return;
    }
    for (uint32_t n = count & 0xff; n > 0; n--) {
        ZfxParticle *p = zmemFreeListAlloc(emitter->pool);

        p->birth = now;
        p->kind = 1;
        COPY32(p->rgba, params[11]);
        p->prev.x = p->pos.x = (float)((double)xf[2] + params[0]);
        p->prev.y = p->pos.y = (float)((double)xf[3] + params[1]);
        p->prev.z = p->pos.z = (float)((double)xf[4] + params[2]);
        if (given) {
            p->vel.x = (float)(((double)randomSigned() * kRandomScale) * params[9] + params[4]);
            p->vel.y = (float)(((double)randomSigned() * kRandomScale) * params[9] + params[5]);
            p->vel.z = (float)(((double)randomSigned() * kRandomScale) * params[9] + params[6]);
        } else {
            p->vel.x = (float)((double)randomSigned() * kRandomScaleSmall);
            p->vel.y = (float)((double)randomSigned() * kRandomScaleSmall);
            p->vel.z = (float)((double)randomSigned() * kRandomScaleSmall);
        }
        COPY32(p->size, params[8]);
        COPY32(p->life, params[10]);
        COPY32(p->texture, params[12]);
        zlistInsert(emitter->particles, p);
    }
}

/* Fills one vertex of an immediate mode primitive (the padding word is left alone). */
static void setVertex(ZfxVertex *v, float x, float y, float z, float u, float tv, uint32_t color)
{
    v->x = x;
    v->y = y;
    v->z = z;
    v->u = u;
    v->v = tv;
    v->color = color;
}

/*
 * The camera rotation (position from the camera matrix) and its transpose with a zero position, which
 * takes world points into the camera's frame.
 */
static void cameraMatrices(ZMatrix *camera, ZMatrix *toCamera)
{
    zmatrixCopyRotation(camera, zobjCameraGetMatrix(gFxCameraObject(gFxGetCamera()), 1));
    zmatrixCopyRotation(toCamera, camera);
    zmatrixTranspose(toCamera, toCamera);
    toCamera->pos = (ZVec3){0.0f, 0.0f, 0.0f};
}

/*
 * Draws the spark particles: untextured ones as a line from the previous to the current position,
 * textured ones as a quad of width 2 * size swept along the motion as seen by the camera. The alpha
 * ramps down over the last 0x200 ms.
 */
BMX_ADDR(0x0045cc50)
int ngiffxSparkRender(const void *effect, ZfxEmitter *emitter)
{
    const uint32_t now = gTimeGetTime();
    int drawn = 0;
    ZfxParticle *p;
    ZfxVertex v[4];
    ZfxQuad quad;
    ZfxLine line = {.vertex = {&v[0], &v[1]}};
    ZMatrix camera, toCamera;

    cameraMatrices(&camera, &toCamera);
    p = zlistFirst(emitter->particles);
    if (p == NULL) {
        return 0;
    }
    do {
        const uint32_t rem = (p->birth + p->life) - now;
        uint32_t color;

        __builtin_memcpy(&color, p->rgba, 4);
        color = (color & 0x00ffffffu) | ((rem < 0x200 ? (rem >> 1) & 0xff : 0xffu) << 24);
        if (p->texture == 0) {
            setVertex(&v[0], p->prev.x, p->prev.y, p->prev.z, 0.0f, 0.0f, color);
            setVertex(&v[1], p->pos.x, p->pos.y, p->pos.z, 0.0f, 0.0f, color);
            line.texture = p->texture;
            gFxDrawLines(0x2202, &line, 1, 0, 0);
        } else {
            ZVec3 a = p->pos, b = p->prev, side;
            float dx, dy, dz;
            float s;

            camera.pos = p->prev;
            zmatrixTransformPoint(&a, &toCamera, &a);
            zmatrixTransformPoint(&b, &toCamera, &b);
            dx = (float)((double)a.x - b.x);
            dy = (float)((double)a.y - b.y);
            dz = (float)((double)a.z - b.z);
            side = (ZVec3){dy, -dx, 0.0f};
            zvec3NormalizeScaled(&side, &side);
            s = p->size;
            setVertex(&v[0], (float)((double)s * side.x), (float)((double)s * side.y),
                      (float)((double)s * side.z), 0.0f, 0.0f, color);
            zmatrixTransformPoint((ZVec3 *)&v[0], &camera, (ZVec3 *)&v[0]);
            setVertex(&v[1], (float)-((double)s * side.x), (float)-((double)s * side.y),
                      (float)-((double)s * side.z), 0.0f, 1.0f, color);
            zmatrixTransformPoint((ZVec3 *)&v[1], &camera, (ZVec3 *)&v[1]);
            setVertex(&v[2], (float)((double)dx - (double)s * side.x),
                      (float)((double)dy - (double)s * side.y), (float)((double)dz - (double)s * side.z),
                      1.0f, 1.0f, color);
            zmatrixTransformPoint((ZVec3 *)&v[2], &camera, (ZVec3 *)&v[2]);
            setVertex(&v[3], (float)((double)s * side.x + dx), (float)((double)s * side.y + dy),
                      (float)((double)s * side.z + dz), 1.0f, 0.0f, color);
            zmatrixTransformPoint((ZVec3 *)&v[3], &camera, (ZVec3 *)&v[3]);
            quad = (ZfxQuad){p->texture, {&v[0], &v[1], &v[2], &v[3]}};
            gFxDrawQuads(0x2202, &quad, 1, 0, 0);
        }
        p = zlistNext(emitter->particles);
        drawn++;
    } while (p != NULL);
    return drawn;
}

/* Shard spawner: a particle with a velocity, and an axis perpendicular-ish to it that spins. */
BMX_ADDR(0x0045d260)
void ngiffxShardSpawn(int enabled, const void *object, ZfxEmitter *emitter, uint32_t count, int unused,
                      const float *params)
{
    const uint32_t now = gTimeGetTime();
    const float *const xf = objectTransform(object);

    if (params == NULL) {
        params = gFxShardDefault;
    }
    if (enabled == 0 || emitter == NULL) {
        return;
    }
    for (uint32_t n = count & 0xff; n > 0; n--) {
        ZfxParticle *p = zmemFreeListAlloc(emitter->pool);

        p->birth = now;
        p->kind = 3;
        COPY32(p->rgba, params[14]);
        p->prev.x = p->pos.x = (float)((double)xf[2] + params[0]);
        p->prev.y = p->pos.y = (float)((double)xf[3] + params[1]);
        p->prev.z = p->pos.z = (float)((double)xf[4] + params[2]);
        COPY32(p->vel.x, params[4]);
        COPY32(p->vel.y, params[5]);
        COPY32(p->vel.z, params[6]);
        p->axis.x = p->vel.x;
        p->axis.y = 0.0f;
        p->axis.z = p->vel.z;
        zvec3NormalizeScaled(&p->axis, &p->axis);
        if (__builtin_fabsf(p->axis.x) <= __builtin_fabsf(p->axis.z)) {
            p->axis.z = 1.0f;
            p->axis.y = 0.0f;
            p->axis.x = 0.0f;
        } else {
            p->axis.x = 1.0f;
            p->axis.y = 0.0f;
            p->axis.z = 0.0f;
        }
        p->spin.x = (int16_t)bmxFtol((double)params[8]);
        p->spin.y = (int16_t)bmxFtol((double)params[9]);
        p->spin.z = (int16_t)bmxFtol((double)params[10]);
        COPY32(p->size, params[12]);
        COPY32(p->life, params[13]);
        COPY32(p->texture, params[15]);
        zlistInsert(emitter->particles, p);
    }
}

/* Alpha from the remaining life: `rem` in ms; 0 beyond `far`, the ramp below `knee`, else `top`. */
static uint8_t fadeAlpha(uint32_t rem, uint32_t far, uint32_t knee, int shift, uint8_t top)
{
    if (rem > far) {
        return 0;
    }
    if (rem < knee) {
        return (uint8_t)(rem >> shift);
    }
    return top;
}

/* Draws the shard particles as two sided triangles (flat, UVs zero). */
BMX_ADDR(0x0045d420)
int ngiffxShardRender(const void *effect, ZfxEmitter *emitter)
{
    const uint32_t now = gTimeGetTime();
    int drawn = 0;
    ZfxParticle *p;
    ZfxVertex v[3];
    ZfxTriangle triangles[2];
    ZMatrix matrix;
    static const ZVec3 kUp = {0.0f, 0.0f, 1.0f};

    p = zlistFirst(emitter->particles);
    if (p == NULL) {
        return 0;
    }
    do {
        uint32_t rem;
        uint8_t alpha;
        uint32_t rgba;

        __builtin_memcpy(&rgba, p->rgba, 4);
        zobjAxisMatrix(&matrix, &kUp, &p->axis);
        matrix.pos.x = 0.0f;
        matrix.pos.y = 0.0f;
        matrix.pos.z = 0.0f;
        rem = (p->birth + p->life) - now;
        if (rem < 0x100) {
            alpha = (uint8_t)(rem >> 1);
        } else {
            alpha = 0x80;
        }
        if (rem > 40000) {
            alpha = 0;
        }
        rgba = (rgba & 0x00ffffffu) | ((uint32_t)alpha << 24);
        v[0] = (ZfxVertex){.x = 0.0f, .y = -p->size, .z = 0.0f, .u = 0.0f, .v = 0.0f, .color = rgba};
        zmatrixTransformPoint((ZVec3 *)&v[0], &matrix, (ZVec3 *)&v[0]);
        zvec3Add((ZVec3 *)&v[0], (ZVec3 *)&v[0], &p->pos);
        v[1] = (ZfxVertex){.x = p->size, .y = p->size, .z = 0.0f, .u = 0.0f, .v = 0.0f, .color = rgba};
        zmatrixTransformPoint((ZVec3 *)&v[1], &matrix, (ZVec3 *)&v[1]);
        zvec3Add((ZVec3 *)&v[1], (ZVec3 *)&v[1], &p->pos);
        v[2] = (ZfxVertex){.x = -p->size, .y = p->size, .z = 0.0f, .u = 0.0f, .v = 0.0f, .color = rgba};
        zmatrixTransformPoint((ZVec3 *)&v[2], &matrix, (ZVec3 *)&v[2]);
        zvec3Add((ZVec3 *)&v[2], (ZVec3 *)&v[2], &p->pos);
        triangles[0] = (ZfxTriangle){p->texture, {&v[0], &v[1], &v[2]}};
        triangles[1] = (ZfxTriangle){p->texture, {&v[2], &v[1], &v[0]}};
        gFxDrawTriangles(0x6502, triangles, 2, 0, 0);
        p = zlistNext(emitter->particles);
        drawn++;
    } while (p != NULL);
    return drawn;
}

/* Draws the splash particles as camera facing quads that fade out. */
BMX_ADDR(0x0045d730)
int ngiffxSplashRender(const void *effect, ZfxEmitter *emitter)
{
    const uint32_t now = gTimeGetTime();
    int drawn = 0;
    ZfxParticle *p;
    ZfxVertex v[4];
    ZfxQuad quad;
    ZMatrix matrix;

    zmatrixCopyRotation(&matrix, zobjCameraGetMatrix(gFxCameraObject(gFxGetCamera()), 1));
    p = zlistFirst(emitter->particles);
    if (p == NULL) {
        return 0;
    }
    do {
        const uint32_t rem = (p->birth + p->life) - now;
        const uint8_t shade = rem > 0x7530 ? 0 : (rem < 0x400 ? (uint8_t)(rem >> 3) : 0x80);
        const uint32_t color = shade * 0x01010101u;
        const float s = p->size;

        matrix.pos = p->pos;
        v[0] = (ZfxVertex){.x = -s, .y = -s, .z = 0.0f, .u = 0.0f, .v = 0.0f, .color = color};
        zmatrixTransformPoint((ZVec3 *)&v[0], &matrix, (ZVec3 *)&v[0]);
        v[1] = (ZfxVertex){.x = -s, .y = s, .z = 0.0f, .u = 0.0f, .v = 1.0f, .color = color};
        zmatrixTransformPoint((ZVec3 *)&v[1], &matrix, (ZVec3 *)&v[1]);
        v[2] = (ZfxVertex){.x = s, .y = s, .z = 0.0f, .u = 1.0f, .v = 1.0f, .color = color};
        zmatrixTransformPoint((ZVec3 *)&v[2], &matrix, (ZVec3 *)&v[2]);
        v[3] = (ZfxVertex){.x = s, .y = -s, .z = 0.0f, .u = 1.0f, .v = 0.0f, .color = color};
        zmatrixTransformPoint((ZVec3 *)&v[3], &matrix, (ZVec3 *)&v[3]);
        quad = (ZfxQuad){p->texture, {&v[0], &v[1], &v[2], &v[3]}};
        gFxDrawQuads(0x2202, &quad, 1, 0, 0);
        p = zlistNext(emitter->particles);
        drawn++;
    } while (p != NULL);
    return drawn;
}

/* Dust spawner: one slow particle with the fixed DUST01 texture. */
BMX_ADDR(0x0045da40)
void ngiffxDustSpawn(int enabled, const void *object, ZfxEmitter *emitter, uint32_t count, int unused,
                     const float *params)
{
    const uint32_t now = gTimeGetTime();
    const float *const xf = objectTransform(object);
    ZfxParticle *p;

    if (params == NULL) {
        params = gFxDustDefault;
    }
    if (enabled == 0 || emitter == NULL) {
        return;
    }
    p = zmemFreeListAlloc(emitter->pool);
    p->birth = now;
    p->kind = 1;
    p->rgba[3] = 0x7f;
    p->rgba[0] = 0x7f;
    p->rgba[1] = 0x7f;
    p->rgba[2] = 0x7f;
    p->prev.x = p->pos.x = (float)((double)xf[2] + params[0]);
    p->prev.y = p->pos.y = (float)((double)params[1] + xf[3]);
    p->prev.z = p->pos.z = (float)((double)params[2] + xf[4]);
    COPY32(p->vel.x, params[4]);
    COPY32(p->vel.y, params[5]);
    COPY32(p->vel.z, params[6]);
    COPY32(p->size, params[8]);
    COPY32(p->life, params[9]);
    p->texture = gFxDustTextures[0];
    zlistInsert(emitter->particles, p);
}

/* Draws the dust particles as camera facing quads; the texture steps through DUST01..16 as they age. */
BMX_ADDR(0x0045db20)
int ngiffxDustRender(const void *effect, ZfxEmitter *emitter)
{
    const uint32_t now = gTimeGetTime();
    int drawn = 0;
    ZfxParticle *p;
    ZfxVertex v[4];
    ZfxQuad quad;
    ZMatrix matrix;

    zmatrixCopyRotation(&matrix, zobjCameraGetMatrix(gFxCameraObject(gFxGetCamera()), 1));
    p = zlistFirst(emitter->particles);
    if (p == NULL) {
        return 0;
    }
    do {
        uint32_t rem = (p->birth + p->life) - now;
        uint32_t color;
        const float s = p->size;

        matrix.pos = p->pos;
        __builtin_memcpy(&color, p->rgba, 4);
        if (rem > 0x7530) {
            rem = 0;
        } else if (rem > 0x3e8) {
            rem = 0x3e8;
        }
        v[0] = (ZfxVertex){.x = -s, .y = -s, .z = 0.0f, .u = 0.0f, .v = 0.0f, .color = color};
        zmatrixTransformPoint((ZVec3 *)&v[0], &matrix, (ZVec3 *)&v[0]);
        v[1] = (ZfxVertex){.x = -s, .y = s, .z = 0.0f, .u = 0.0f, .v = 1.0f, .color = color};
        zmatrixTransformPoint((ZVec3 *)&v[1], &matrix, (ZVec3 *)&v[1]);
        v[2] = (ZfxVertex){.x = s, .y = s, .z = 0.0f, .u = 1.0f, .v = 1.0f, .color = color};
        zmatrixTransformPoint((ZVec3 *)&v[2], &matrix, (ZVec3 *)&v[2]);
        v[3] = (ZfxVertex){.x = s, .y = -s, .z = 0.0f, .u = 1.0f, .v = 0.0f, .color = color};
        zmatrixTransformPoint((ZVec3 *)&v[3], &matrix, (ZVec3 *)&v[3]);
        quad = (ZfxQuad){gFxDustTextures[(15000u - rem * 15u) / 1000u], {&v[0], &v[1], &v[2], &v[3]}};
        gFxDrawQuads(0x2202, &quad, 1, 0, 0);
        p = zlistNext(emitter->particles);
        drawn++;
    } while (p != NULL);
    return drawn;
}

/* Creates an emitter: the particle list, a free list of `capacity` particles and its owner effect. */
BMX_ADDR(0x0045de30)
ZfxEmitter *ngiffxPosterCreate(uint32_t effect, uint16_t capacity)
{
    ZfxEmitter *emitter = zmemAlloc(sizeof *emitter);

    if (emitter == NULL) {
        return NULL;
    }
    emitter->particles = zlistCreate(capacity);
    emitter->pool = zmemFreeListCreate(sizeof(ZfxParticle), capacity, NULL);
    emitter->effect = effect;
    emitter->lastUpdate = gTimeGetTime();
    return emitter;
}

/* Poster spawner: a static quad whose four corners are given relative to the object. */
BMX_ADDR(0x0045dea0)
void ngiffxPosterSpawn(int enabled, const void *object, ZfxEmitter *emitter, uint32_t count, int unused,
                       const float *params)
{
    const uint32_t now = gTimeGetTime();
    const float *const xf = objectTransform(object);
    ZfxParticle *p;

    if (params == NULL) {
        params = gFxPosterDefault;
    }
    if (enabled == 0 || emitter == NULL) {
        return;
    }
    p = zmemFreeListAlloc(emitter->pool);
    p->kind = 0;
    p->birth = now;
    COPY32(p->rgba, params[17]);
    p->pos.x = (float)((double)xf[2] + params[0]);
    p->pos.y = (float)((double)xf[3] + params[1]);
    p->pos.z = (float)((double)xf[4] + params[2]);
    p->vel.x = 0.0f;
    p->vel.y = 0.0f;
    p->vel.z = 0.0f;
    p->size = 0.0f;
    COPY32(p->life, params[16]);
    COPY32(p->texture, params[18]);
    for (int i = 0; i < 4; i++) {
        p->corner[i][0] = (float)((double)xf[2] + params[i * 4]);
        p->corner[i][1] = (float)((double)xf[3] + params[i * 4 + 1]);
        p->corner[i][2] = (float)((double)xf[4] + params[i * 4 + 2]);
    }
    zlistInsert(emitter->particles, p);
}

/* Draws the posters as quads from their stored corners, fading in the last second. */
BMX_ADDR(0x0045e010)
int ngiffxPosterRender(const void *effect, ZfxEmitter *emitter)
{
    const uint32_t now = gTimeGetTime();
    int drawn = 0;
    ZfxParticle *p = zlistFirst(emitter->particles);
    ZfxVertex v[4];
    ZfxQuad quad;

    while (p != NULL) {
        const uint32_t rem = (p->birth + p->life) - now;
        uint32_t color;

        __builtin_memcpy(&color, p->rgba, 4);
        color = (color & 0x00ffffffu) | ((uint32_t)fadeAlpha(rem, 0x7530, 0x400, 2, 0xff) << 24);
        for (int i = 0; i < 4; i++) {
            static const float kU[4] = {0.0f, 0.0f, 1.0f, 1.0f};
            static const float kV[4] = {0.0f, 1.0f, 1.0f, 0.0f};

            v[i].x = p->corner[i][0];
            v[i].y = p->corner[i][1];
            v[i].z = p->corner[i][2];
            v[i].u = kU[i];
            v[i].v = kV[i];
            v[i].color = color;
        }
        quad = (ZfxQuad){p->texture, {&v[0], &v[1], &v[2], &v[3]}};
        gFxDrawQuads(0x4102, &quad, 1, 0, 0);
        p = zlistNext(emitter->particles);
        drawn++;
    }
    return drawn;
}

/* Ripple spawner: a static particle that only grows (see the updater). */
BMX_ADDR(0x0045e210)
void ngiffxRippleSpawn(int enabled, const void *object, ZfxEmitter *emitter, uint32_t count, int unused,
                       const float *params)
{
    const uint32_t now = gTimeGetTime();
    const float *const xf = objectTransform(object);
    ZfxParticle *p;

    if (params == NULL) {
        params = gFxRippleDefault;
    }
    if (enabled == 0 || emitter == NULL) {
        return;
    }
    p = zmemFreeListAlloc(emitter->pool);
    p->birth = now;
    p->kind = 0;
    COPY32(p->rgba, params[7]);
    p->pos.x = (float)((double)xf[2] + params[0]);
    p->pos.y = (float)((double)params[1] + xf[3]);
    p->pos.z = (float)((double)params[2] + xf[4]);
    p->vel.y = 0.0f;
    COPY32(p->vel.x, params[6]);
    p->vel.z = 0.0f;
    COPY32(p->size, params[5]);
    COPY32(p->life, params[4]);
    COPY32(p->texture, params[8]);
    zlistInsert(emitter->particles, p);
}

/* Draws the ripples as flat squares on the ground plane. */
BMX_ADDR(0x0045e2d0)
int ngiffxRippleRender(const void *effect, ZfxEmitter *emitter)
{
    const uint32_t now = gTimeGetTime();
    int drawn = 0;
    ZfxParticle *p = zlistFirst(emitter->particles);
    ZfxVertex v[4];
    ZfxQuad quad;

    while (p != NULL) {
        const uint32_t rem = (p->birth + p->life) - now;
        const uint8_t shade = fadeAlpha(rem, 0x7530, 0x800, 3, 0xff);
        const uint32_t color = shade * 0x01010101u;

        v[0] = (ZfxVertex){.x = (float)((double)p->pos.x + p->size),
                           .y = p->pos.y,
                           .z = (float)((double)p->pos.z + p->size),
                           .u = 0.0f,
                           .v = 0.0f,
                           .color = color};
        v[1] = (ZfxVertex){.x = (float)((double)p->pos.x - p->size),
                           .y = p->pos.y,
                           .z = (float)((double)p->pos.z + p->size),
                           .u = 0.0f,
                           .v = 1.0f,
                           .color = color};
        v[2] = (ZfxVertex){.x = (float)((double)p->pos.x - p->size),
                           .y = p->pos.y,
                           .z = (float)((double)p->pos.z - p->size),
                           .u = 1.0f,
                           .v = 1.0f,
                           .color = color};
        v[3] = (ZfxVertex){.x = (float)((double)p->pos.x + p->size),
                           .y = p->pos.y,
                           .z = (float)((double)p->pos.z - p->size),
                           .u = 1.0f,
                           .v = 0.0f,
                           .color = color};
        quad = (ZfxQuad){p->texture, {&v[0], &v[1], &v[2], &v[3]}};
        gFxDrawQuads(0x2203, &quad, 1, 0, 0);
        p = zlistNext(emitter->particles);
        drawn++;
    }
    return drawn;
}

/*
 * Advances all particles of an emitter by the time since the last update, rebuilds the bounds, retires
 * particles past their life and runs the effect's modifiers (gravity, floor collision). Quirk: a moving
 * particle gets its velocity added once more on top of the time scaled step.
 */
BMX_ADDR(0x0045e4e0)
int ngiffxUpdateEmitter(ZfxEffect *effect, ZfxEmitter *emitter)
{
    const uint32_t now = gTimeGetTime();
    const uint32_t elapsed = now - emitter->lastUpdate;
    int count = 0;

    if (emitter != NULL && emitter->particles != NULL) {
        ZfxParticle *p;

        emitter->min.x = emitter->min.y = emitter->min.z = 99999.0f;
        emitter->max.x = emitter->max.y = emitter->max.z = -99999.0f;
        p = zlistFirst(emitter->particles);
        if (p != NULL) {
            const float dt = (float)(double)elapsed;

            do {
                count++;
                if ((p->kind & 1) != 0) {
                    p->prev = p->pos;
                    p->pos.x = (float)(((double)dt * p->vel.x) * kMillisecond + p->pos.x);
                    p->pos.y = (float)(((double)dt * p->vel.y) * kMillisecond + p->pos.y);
                    p->pos.z = (float)(((double)dt * p->vel.z) * kMillisecond + p->pos.z);
                    zvec3Add(&p->pos, &p->pos, &p->vel);
                    if (p->pos.x > emitter->max.x) {
                        emitter->max.x = p->pos.x;
                    }
                    if (p->pos.y > emitter->max.y) {
                        emitter->max.y = p->pos.y;
                    }
                    if (p->pos.z > emitter->max.z) {
                        emitter->max.z = p->pos.z;
                    }
                    if (p->pos.x < emitter->min.x) {
                        emitter->min.x = p->pos.x;
                    }
                    if (p->pos.y < emitter->min.y) {
                        emitter->min.y = p->pos.y;
                    }
                    if (p->pos.z < emitter->min.z) {
                        emitter->min.z = p->pos.z;
                    }
                }
                p->size = (float)(((double)dt * p->vel.x) * kMillisecond + p->size);
                if ((p->kind & 2) != 0) {
                    zvec3RotateEuler(&p->axis, &p->axis, &p->spin);
                }
                if (now - p->birth < p->life) {
                    p = zlistNext(emitter->particles);
                } else {
                    zlistRemoveCurrent(emitter->particles);
                    zmemFreeListFree(emitter->pool, p);
                    p = zlistCurrent(emitter->particles);
                }
            } while (p != NULL);
        }
    }
    for (ZfxModifierNode *node = effect->modifiers; node != NULL; node = node->next) {
        node->modifier->apply(node->modifier, emitter, node->arg);
    }
    emitter->lastUpdate = now;
    return count;
}

/* Lightning spawner: a chain of particles along a jittered line from the first to the second point. */
BMX_ADDR(0x0045e750)
void ngiffxLightningSpawn(int enabled, const void *object, ZfxEmitter *emitter, uint32_t count, int unused,
                          const float *params)
{
    const uint32_t now = gTimeGetTime();
    const float *const xf = objectTransform(object);

    if (params == NULL) {
        params = gFxLightningDefault;
    }
    if (enabled == 0 || emitter == NULL) {
        return;
    }
    for (uint32_t n = count & 0xff; n > 0; n--) {
        float start[3] = {params[0], params[1], params[2]};
        uint32_t total;
        uint32_t life = 300;
        uint32_t i = 0;

        COPY32(total, params[8]);
        if (total == 0) {
            continue;
        }
        do {
            ZfxParticle *p = zmemFreeListAlloc(emitter->pool);
            float end[3];

            if (i + 1 == total) {
                COPY32(end[0], params[4]);
                COPY32(end[1], params[5]);
                COPY32(end[2], params[6]);
            } else {
                for (int k = 0; k < 3; k++) {
                    const double jitter = ((double)randomSigned() * kRandomScale) * params[10];
                    const double step = ((double)params[4 + k] - (double)start[k]) / (double)(total - i);

                    end[k] = (float)((jitter + step) + (double)start[k]);
                }
            }
            p->kind = 0;
            p->birth = now;
            p->rgba[0] = 0x10;
            p->rgba[1] = 0x10;
            p->rgba[2] = 0xaf;
            p->rgba[3] = 0xff;
            p->prev.x = (float)((double)start[0] + xf[2]);
            p->prev.y = (float)((double)start[1] + xf[3]);
            p->prev.z = (float)((double)start[2] + xf[4]);
            p->pos.x = (float)((double)end[0] + xf[2]);
            p->pos.y = (float)((double)end[1] + xf[3]);
            p->pos.z = (float)((double)end[2] + xf[4]);
            p->life = life;
            COPY32(p->size, params[9]);
            COPY32(p->texture, params[11]);
            zlistInsert(emitter->particles, p);
            start[0] = end[0];
            start[1] = end[1];
            start[2] = end[2];
            life += 10;
            i++;
        } while (i < total);
    }
}

/*
 * Draws the lightning segments: untextured ones as a line, textured ones as a camera facing quad of
 * height 2 * size spanning the segment's camera space offset. The quad's winding flips when the segment
 * runs leftwards (camera x not positive). The alpha is the remaining life below 0x100 ms, else 0.
 */
BMX_ADDR(0x0045eaa0)
int ngiffxLightningRender(const void *effect, ZfxEmitter *emitter)
{
    const uint32_t now = gTimeGetTime();
    int drawn = 0;
    ZfxParticle *p;
    ZfxVertex v[4];
    ZfxQuad quad = {.vertex = {&v[0], &v[1], &v[2], &v[3]}};
    ZfxLine line = {.vertex = {&v[0], &v[1]}};
    ZMatrix camera, toCamera;

    cameraMatrices(&camera, &toCamera);
    p = zlistFirst(emitter->particles);
    if (p == NULL) {
        return 0;
    }
    do {
        const uint32_t rem = (p->birth + p->life) - now;
        uint32_t color;

        __builtin_memcpy(&color, p->rgba, 4);
        color = (color & 0x00ffffffu) | ((rem < 0x100 ? rem & 0xff : 0u) << 24);
        if (p->texture == 0) {
            setVertex(&v[0], p->prev.x, p->prev.y, p->prev.z, 0.0f, 0.0f, color);
            setVertex(&v[1], p->pos.x, p->pos.y, p->pos.z, 0.0f, 0.0f, color);
            line.texture = p->texture;
            gFxDrawLines(0x2202, &line, 1, 0, 0);
        } else {
            ZVec3 a = p->pos, b = p->prev;
            float dx, dy, dz;

            camera.pos = p->prev;
            zmatrixTransformPoint(&a, &toCamera, &a);
            zmatrixTransformPoint(&b, &toCamera, &b);
            dx = (float)((double)a.x - b.x);
            dy = (float)((double)a.y - b.y);
            dz = (float)((double)a.z - b.z);
            if (dx > 0.0f) {
                quad.vertex[0] = &v[0];
                quad.vertex[1] = &v[1];
                quad.vertex[2] = &v[2];
                quad.vertex[3] = &v[3];
            } else {
                quad.vertex[0] = &v[3];
                quad.vertex[1] = &v[2];
                quad.vertex[2] = &v[1];
                quad.vertex[3] = &v[0];
            }
            setVertex(&v[0], 0.0f, -p->size, 0.0f, 0.0f, 0.0f, color);
            zmatrixTransformPoint((ZVec3 *)&v[0], &camera, (ZVec3 *)&v[0]);
            setVertex(&v[1], 0.0f, p->size, 0.0f, 0.0f, 1.0f, color);
            zmatrixTransformPoint((ZVec3 *)&v[1], &camera, (ZVec3 *)&v[1]);
            setVertex(&v[2], dx, (float)((double)p->size + dy), dz, 1.0f, 1.0f, color);
            zmatrixTransformPoint((ZVec3 *)&v[2], &camera, (ZVec3 *)&v[2]);
            setVertex(&v[3], dx, (float)((double)dy - p->size), dz, 1.0f, 0.0f, color);
            zmatrixTransformPoint((ZVec3 *)&v[3], &camera, (ZVec3 *)&v[3]);
            quad.texture = p->texture;
            gFxDrawQuads(0x2202, &quad, 1, 0, 0);
        }
        p = zlistNext(emitter->particles);
        drawn++;
    } while (p != NULL);
    return drawn;
}

BMX_ADDR(0x0045f030)
void ngiffxCreateSpark(uint32_t handle, float *params)
{
    if (params == NULL) {
        ngiffxSpawn(handle, "spark", 2, 0, gFxSparkCreateDefault);
        return;
    }
    COPY32(params[12], gFxSparkTexture);
    ngiffxSpawn(handle, "spark", 2, 0, params);
}

BMX_ADDR(0x0045f0a0)
void ngiffxCreateShard(uint32_t handle, const void *params)
{
    ngiffxSpawn(handle, "shard", 1, 0, params);
}

BMX_ADDR(0x0045f0d0)
void ngiffxCreateSplash(uint32_t handle, float *params)
{
    if (params == NULL) {
        ngiffxSpawn(handle, "splash", 2, 0, gFxSplashCreateDefault);
        return;
    }
    COPY32(params[12], gFxSplashTexture);
    ngiffxSpawn(handle, "splash", 0x20, 0, params);
}

BMX_ADDR(0x0045f140)
void ngiffxCreateDust(uint32_t handle, const void *params)
{
    ngiffxSpawn(handle, "dust", 1, 0, params != NULL ? params : gFxDustDefault);
}

BMX_ADDR(0x0045f190)
void ngiffxCreatePoster(uint32_t handle, const void *params)
{
    ngiffxSpawn(handle, "poster", 1, 0, params != NULL ? params : gFxPosterDefault);
}

/* Ripples spawn three times, each lower, larger and fainter than the one before. */
BMX_ADDR(0x0045f1e0)
void ngiffxCreateRipple(uint32_t handle, uint32_t *params)
{
    if (params == NULL) {
        ngiffxSpawn(handle, "ripple", 1, 0, gFxRippleDefault);
        return;
    }
    params[8] = gFxRippleTexture;
    params[4] = 1000;
    params[5] = 0x41f00000;
    params[6] = 0x437a0000;
    ngiffxSpawn(handle, "ripple", 1, 0, params);
    storeFloat(&params[1], (float)((double)loadFloat(&params[1]) - 5.0));
    params[4] = 0x4e2;
    params[5] = 0x41a00000;
    params[6] = 0x43480000;
    ngiffxSpawn(handle, "ripple", 1, 0, params);
    storeFloat(&params[1], (float)((double)loadFloat(&params[1]) - 10.0));
    params[4] = 0x5dc;
    params[5] = 0x41200000;
    params[6] = 0x43160000;
    ngiffxSpawn(handle, "ripple", 1, 0, params);
}

/* Skid marks are posters; a zero texture selects SKID, a texture of 1 selects RIPPLE. */
BMX_ADDR(0x0045f2d0)
void ngiffxCreateSkid(uint32_t handle, uint32_t *params)
{
    if (params == NULL) {
        params = (uint32_t *)gFxPosterDefault;
    } else {
        if (params[0x12] == 0) {
            params[0x12] = gFxSkidTexture;
        }
        if (params[0x12] == 1) {
            params[0x12] = gFxRippleTexture;
        }
    }
    ngiffxSpawn(handle, "poster", 1, 0, params);
}

/* A lightning bolt from the origin to a random point (each axis a signed byte). */
BMX_ADDR(0x0045f340)
void ngiffxCreateLightning(uint32_t handle)
{
    uint32_t block[12] = {0};

    storeFloat(&block[4], (float)randomSigned());
    storeFloat(&block[5], (float)randomSigned());
    storeFloat(&block[6], (float)randomSigned());
    block[8] = 8;
    storeFloat(&block[9], 5.0f);
    storeFloat(&block[10], 10.0f);
    ngiffxSpawn(handle, "lightning", 1, 0, block);
}

BMX_ADDR(0x0045f480)
const uint8_t *ngiffxGetNode(const void *obj, int index)
{
    return gFxWorldObject;
}

/*
 * Finds the level objects named "ANIM" and records their texture animation (frame textures looked up in
 * the object's library, falling back to the global ones). At most 8 are meant to exist (not checked).
 * The original also hashes a constant string first and discards the result.
 */
BMX_ADDR(0x0045f4a0)
int ngiffxFindSkidObjects(void)
{
    const uint32_t now = gTimeGetTime();
    const uint32_t config = gFxSkidConfig;
    const int hash = zstrHash(gFxSkidName);
    const uint32_t *object;
    int found = 0;

    for (int i = 0; i < kFxSkidEntries; i++) {
        gFxSkids[i].object = NULL;
    }
    for (object = (const uint32_t *)*(void **)zobjTreeObjectList(); object != NULL;
         object = (const uint32_t *)(uintptr_t)object[0]) {
        ZfxSkid *skid = &gFxSkids[found];

        if (hash != (int)object[2]) {
            continue;
        }
        skid->object = (void *)(uintptr_t)object;
        skid->frame = 0;
        skid->fps = (uint8_t)config;
        skid->count = (uint8_t)(config >> 8);
        skid->lastTime = now;
        skid->base = gFxLoadTexture(zstrHash(gFxSkidBase));
        for (int j = 0; j < (int)(uint8_t)(config >> 8); j++) {
            uint32_t texture = gFxFindTexture((void *)(uintptr_t)object[8], zstrHash(gFxSkidFrameNames[j]));

            if (texture == 0) {
                texture = gFxLoadTexture(zstrHash(gFxSkidFrameNames[j]));
            }
            skid->texture[j] = texture;
        }
        found++;
    }
    return found != 0;
}

/* Steps each animated object to its next frame at `fps` by rotating the textures through its slots. */
BMX_ADDR(0x0045f650)
int ngiffxAnimateSkids(void)
{
    const uint32_t now = gTimeGetTime();

    for (int i = 0; i < kFxSkidEntries; i++) {
        ZfxSkid *const skid = &gFxSkids[i];

        if (skid->object == NULL) {
            return 1;
        }
        if (now - skid->lastTime >= (uint32_t)(1000 / (int)skid->fps)) {
            void *first;

            skid->lastTime = now;
            skid->frame = (uint8_t)((skid->frame + 1) % skid->count);
            first = gFxGetRaster(skid->texture[0], 0);
            for (int j = 0; j < skid->count - 1; j++) {
                gFxSetRaster(skid->texture[j], gFxGetRaster(skid->texture[j + 1], 0), 0);
            }
            gFxSetRaster(skid->texture[skid->count - 1], first, 0);
        }
    }
    return 1;
}

/* Registers the effects (handler tables, creators, modifiers) and looks the effect textures up. */
BMX_ADDR(0x0045f780)
void ngiffxRegisterHandlers(void)
{
    static const char *const kDust[16] = {"DUST01", "DUST02", "DUST03", "DUST04", "DUST05", "DUST06",
                                          "DUST07", "DUST08", "DUST09", "DUST10", "DUST11", "DUST12",
                                          "DUST13", "DUST14", "DUST15", "DUST16"};

    zstrNameTablesInit();
    zstrNameTable0Add(&gFxLightning, "lightning");
    zstrNameTable0Add(&gFxSpark, "spark");
    zstrNameTable0Add(&gFxShard, "shard");
    zstrNameTable0Add(&gFxSplash, "splash");
    zstrNameTable0Add(&gFxDust, "dust");
    zstrNameTable0Add(&gFxPoster, "poster");
    zstrNameTable0Add(&gFxRipple, "ripple");
    zstrNameTable1Add(&gFxGravity, "gravity");
    zstrNameTable1Add(&gFxFloorColl, "floorcoll");
    zstrNameTable2Add(BMX_FNPTR(ZNameHandlerFn, 0x004016f9, ngiffxCreateSpark), "spark");
    zstrNameTable2Add(BMX_FNPTR(ZNameHandlerFn, 0x004011fe, ngiffxCreateShard), "shard");
    zstrNameTable2Add(BMX_FNPTR(ZNameHandlerFn, 0x004016ea, ngiffxCreateSplash), "splash");
    zstrNameTable2Add(BMX_FNPTR(ZNameHandlerFn, 0x00401ca8, ngiffxCreateLightning), "lightning");
    zstrNameTable2Add(BMX_FNPTR(ZNameHandlerFn, 0x004015eb, ngiffxCreateDust), "dust");
    zstrNameTable2Add(BMX_FNPTR(ZNameHandlerFn, 0x004021b2, ngiffxCreatePoster), "poster");
    zstrNameTable2Add(BMX_FNPTR(ZNameHandlerFn, 0x0040243c, ngiffxCreateSkid), "skid");
    zstrNameTable2Add(BMX_FNPTR(ZNameHandlerFn, 0x004017ee, ngiffxCreateRipple), "ripple");
    gFxSpark.attach(&gFxSpark, "gravity", NULL);
    gFxSplash.attach(&gFxSplash, "gravity", NULL);
    gFxShard.attach(&gFxShard, "gravity", gFxGravityParams);
    gFxShard.attach(&gFxShard, "floorcoll", NULL);
    gFxCube = zsceneFindModel(0, "SFXCube");
    zobjSetVisible((int32_t)(intptr_t)gFxCube, 0);
    gFxSparkTexture = gFxTextureByHash(zstrHash("SPARK"), 0);
    gFxSkidTexture = gFxTextureByHash(zstrHash("SKID"), 0);
    gFxRippleTexture = gFxTextureByHash(zstrHash("RIPPLE"), 0);
    gFxSplashTexture = gFxTextureByHash(zstrHash("SPLASH"), 0);
    for (int i = 0; i < 16; i++) {
        gFxDustTextures[i] = gFxTextureByHash(zstrHash(kDust[i]), 0);
    }
    gFxWorldObject = zsceneAddWorldObject(0);
    if (gFxAnimHandle == 0 && ngiffxFindSkidObjects() == 1) {
        gFxAnimHandle = gZrollerRegister(8, BMX_FNPTR(void *, 0x004020d1, ngiffxAnimateSkids), NULL, 0);
    }
}

BMX_ADDR(0x0045fc10)
void ngiffxShutdown(void)
{
    if (gFxAnimHandle != 0) {
        gZrollerUnregister(gFxAnimHandle);
        gFxAnimHandle = 0;
    }
    gFxSpark.destroy(&gFxSpark);
    gFxDust.destroy(&gFxDust);
    gFxPoster.destroy(&gFxPoster);
    gFxLightning.destroy(&gFxLightning);
    gFxRipple.destroy(&gFxRipple);
    gFxSplash.destroy(&gFxSplash);
    gFxShard.destroy(&gFxShard);
    zstrNameTablesShutdown();
}

BMX_ADDR(0x0045fcb0)
int ngiffxCreate(int kind, const void *params, const void *arg)
{
    if (kind != 0) {
        gZdebugPrint("%s %d %s\n", kFxFile, 0x682, "zfxCreate : Unsupported effect");
    }
    return 0;
}
