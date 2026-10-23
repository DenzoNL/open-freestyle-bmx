/* The roller's shadow: start-up (the sampler and drawing are in zrwmodelcreate.c). */
#include "zge/com/zglrw/zrwmodelcreate.h"
#include "zge/zdebug.h"
#include "zge/zfile.h"

/* zrwzgl.h cannot be included next to zrwmodel.h (it names 0x697720 / 0x697724 the other way round). */
typedef int(BMX_CDECL *ZrwUpdateFn)(void *arg);
struct ZrwUpdateNode *zrwRegisterUpdateFunction(int type, ZrwUpdateFn fn, void *arg, int append);

static const char kZrwShadowFile[] = "C:\\NgEngine\\Zge\\Com\\ZglRw\\zrwShadow.c";

/* The exe's texture loader (zrwmatLoadSpriteTexture), called with a spare fifth argument. */
typedef uint32_t(BMX_CDECL *ZrwShadowLoadFn)(void *data, int size, const char *name, int type, int zero);
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
        gZrwShadowHead = 0;
    } else {
        gZrwShadowHead = gZrwShadowLoad(image, size, "ShadowMap", 0, 0);
        zfileFreeBuffer(image);
    }
    gZrwShadowReady = 0;
    gZrwShadowNode = zrwRegisterUpdateFunction(
        2, BMX_FNPTR(ZrwUpdateFn, 0x0049b3d0, (ZrwUpdateFn)zrollerShadowDraw), NULL, 1);
    return 1;
}
