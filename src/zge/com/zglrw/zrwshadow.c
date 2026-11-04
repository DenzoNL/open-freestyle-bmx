/* The roller's soft shadow (C:\NgEngine\Zge\Com\ZglRw\zrwShadow.c). */
#include "zge/com/zglrw/zrwshadow.h"

#include "compat/crt.h"
#include "zge/com/zglrw/zrwcol.h"
#include "zge/com/zglrw/zrwmat.h"
#include "zge/com/zglrw/zrwprim.h"
#include "zge/zdebug.h"
#include "zge/zfile.h"
#include "zge/zgemods/zobjcamera.h"
#include "zge/zgemods/zobjpemit.h"
#include "zge/zobj.h"

/* zrwzgl.h cannot be included next to zrwmodel.h (it names 0x697720 / 0x697724 the other way round). */
typedef int(BMX_CDECL *ZrwUpdateFn)(void *arg);
struct ZrwUpdateNode *zrwRegisterUpdateFunction(int type, ZrwUpdateFn fn, void *arg, int append);
int zrwUnregisterUpdateFunction(struct ZrwUpdateNode *node);
int zrwGetParameter(int id, void *out, const void *extra);

static const char kZrwShadowFile[] = "C:\\NgEngine\\Zge\\Com\\ZglRw\\zrwShadow.c";

_Static_assert(sizeof(ZrwShadowQuad) == sizeof(ZrwPrimQuad), "shadow quads are drawn as prim quads");
_Static_assert(sizeof(ZrwShadowVertex) == sizeof(ZrwPrimVertex),
               "shadow vertices are drawn as prim vertices");

/* The exe's texture loader (zrwmatLoadSpriteTexture), called with a spare fifth argument. */
typedef struct ZrwMaterial *(BMX_CDECL *ZrwShadowLoadFn)(void *data, int size, const char *name, int type,
                                                         int zero);
#define gZrwShadowLoad ORIG_VAR(0x00561048, ZrwShadowLoadFn, gZrwShadowLoad)

BMX_ADDR(0x0049b330)
int zrollerShadowStart(void)
{
    int size;
    void *image;

    if (gZrwShadowActive != 0) {
        zdebugFail("Already initialized", kZrwShadowFile, 0x29f, 0);
    }
    gZrwShadowActive = 1;
    image = zfileLoad("assets\\shadowMap.Bmp", &size);
    if (image == NULL) {
        gZrwShadowTexture = 0;
    } else {
        gZrwShadowTexture = zrwmatHandle(gZrwShadowLoad(image, size, "ShadowMap", 0, 0));
        zfileFreeBuffer(image);
    }
    gZrwShadowReady = 0;
    gZrwShadowNode = zrwRegisterUpdateFunction(
        2, BMX_FNPTR(ZrwUpdateFn, 0x0049b3d0, (ZrwUpdateFn)zrollerShadowDraw), NULL, 1);
    return 1;
}

/* A quad is drawn only when all four of its corner probes hit the ground. */
BMX_ADDR(0x0049b3d0)
int zrollerShadowDraw(void)
{
    if (gZrwShadowReady != 0) {
        static const int8_t corner[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
        uint32_t count = 0;

        for (uint32_t i = 0; i < kZrwShadowCells; i++) {
            for (uint32_t j = 0; j < kZrwShadowCells; j++) {
                ZrwShadowQuad *quad = &gZrwShadowQuads[count];
                int complete = 1;

                quad->texture = gZrwShadowTexture;
                for (int c = 0; c < 4; c++) {
                    const uint32_t index = (uint32_t)(corner[c][1] + (int)j) +
                                           (uint32_t)(corner[c][0] + (int)i) * kZrwShadowGrid;
                    if (gZrwShadowMiss[index] != 0) {
                        complete = 0;
                        break;
                    }
                    quad->corner[c] = &gZrwShadowVertices[index];
                }
                if (complete) {
                    count++;
                }
            }
        }
        if (count != 0) {
            zrwprimDrawQuads(0x4103, (const ZrwPrimQuad *)&gZrwShadowQuads[0], (int)count, 0);
        }
        gZrwShadowReady = 0;
    }
    return 1;
}

BMX_ADDR(0x0049b4e0)
int zrollerShadowStop(void)
{
    if (gZrwShadowActive != 0) {
        if (gZrwShadowNode != NULL) {
            zrwUnregisterUpdateFunction(gZrwShadowNode);
        }
        gZrwShadowActive = 0;
    }
    return 1;
}

/*
 * The grid's offsets are built once from the shadow size (renderer parameter 1). Each frame they are moved
 * into a frame that looks from the object's position plus the state's offset (the eye) at the object, and a
 * line from the eye through each point (ten times as far) is tested against the collision world. A hit
 * becomes a vertex lifted along the ground normal, its grey level taken from the eye's height.
 *
 * Original quirk: the vertex position adds the hit point to a vector that is never set (a stack slot right
 * behind the hit record); it is taken as zero here.
 */
BMX_ADDR(0x0049b520)
void zrollerUpdateShadow(void *state)
{
    ZrwShadow *shadow = state;
    ZVec3 position;
    ZVec3 eye;
    ZVec3 zero;
    ZVec3 unset = {0.0f, 0.0f, 0.0f};
    ZMatrix look;
    ZMatrix frame;

    if (gZrwShadowInit == 0) {
        float size;
        zrwGetParameter(1, &size, NULL);
        const double extent = (double)size + (double)size;
        /* The same products, rounded to float for x and kept in a register for y. */
        const float halfX = (float)(extent * (double)0.5f);
        const float halfY = (float)(extent * (double)0.5f);
        const float stepX = (float)(extent * (double)0.2f);
        const double stepY = extent * (double)0.2f;

        for (uint32_t i = 0; i < kZrwShadowGrid; i++) {
            for (uint32_t j = 0; j < kZrwShadowGrid; j++) {
                ZrwShadowPoint *offset = &gZrwShadowOffsets[i * kZrwShadowGrid + j];
                offset->pos.x = (float)((double)i * (double)stepX - (double)halfX);
                offset->pos.z = 0.0f;
                offset->pos.y = (float)(((double)j * stepY - (double)halfY) * (double)-1.0f);
            }
        }
        gZrwShadowScale = (float)((double)size * (double)0.1f);
        gZrwShadowInit = 1;
        gZrwShadowDouble = (float)extent;
    }

    position = *zobjGetPosition(shadow->model->userValue);
    zvec3Add(&eye, &position, &shadow->offset);
    zobjLookAtMatrix(&look, &eye, &position);
    zmatrixInvertRigid(&look, &look);
    zero.x = 0.0f;
    zero.y = 0.0f;
    zero.z = 0.0f;
    zmatrixIdentity(&frame);
    frame.pos = position;
    zmatrixCopyBasis(&frame, &look);
    zmatrixTransformVector(&zero, &frame, &zero);
    zmatrixTranslate(&frame, &frame, &zero);

    for (uint32_t i = 0; i < kZrwShadowGrid; i++) {
        for (uint32_t j = 0; j < kZrwShadowGrid; j++) {
            const uint32_t k = i * kZrwShadowGrid + j;
            ZVec3 probe = eye;
            ZVec3 end;
            ZrwColLineHit hit;

            gZrwShadowMiss[k] = 1;
            zmatrixTransformPoint(&gZrwShadowPoints[k].pos, &frame, &gZrwShadowOffsets[k].pos);
            zvec3Lerp(&end, &probe, &gZrwShadowPoints[k].pos, 10.0f);
            if (zrwcolQueryLine(&probe.x, &end.x, 1, NULL, &hit) != 0) {
                ZrwShadowVertex *vertex = &gZrwShadowVertices[k];
                double alpha;

                zvec3Scale((ZVec3 *)hit.hit.normal, (ZVec3 *)hit.hit.normal, gZrwShadowScale);
                zvec3Add(&vertex->pos, &unset, (ZVec3 *)hit.hit.normal);
                vertex->u = (float)((double)i * (double)0.2f);
                vertex->v = (float)((double)(5 - j) * (double)0.2f);
                alpha = (double)probe.y - (double)unset.y;
                alpha = alpha + (double)1450.0f;
                alpha = alpha * (double)0.001f;
                if (!(alpha >= (double)0.0f)) {
                    alpha = (double)0.0f;
                }
                alpha = alpha * (double)128.0f;
                const uint8_t level = (uint8_t)bmxFtol(alpha);
                vertex->color[0] = level;
                vertex->color[1] = level;
                vertex->color[2] = level;
                vertex->color[3] = level;
                gZrwShadowMiss[k] = 0;
            }
        }
    }
    gZrwShadowReady = 1;
}
