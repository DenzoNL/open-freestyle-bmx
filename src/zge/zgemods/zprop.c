/* Level props (C:\NgEngine\Zge\ZgeMods\zprop.c). */
#include "zge/zgemods/zprop.h"

#include "compat/crt.h"
#include "zge/com/zglrw/zrwmat.h"
#include "zge/com/zglrw/zrwzgl.h"
#include "zge/zdebug.h"
#include "zge/zgemods/zobjcamera.h"
#include "zge/ziff.h"
#include "zge/zmem.h"
#include "zge/zobj.h"
#include "zge/zquat.h"
#include "zge/zscene.h"
#include "zge/zstring.h"

static const char kZpropFile[] = "C:\\NgEngine\\Zge\\ZgeMods\\zprop.c";

#define gZpropDefs  ORIG_VAR(0x006722b8, ZrwList, gZpropDefs)
#define gZpropInsts ORIG_VAR(0x006722c4, ZrwList, gZpropInsts)

BMX_ADDR(0x00486a20)
int zpropLoadDef(void *iff)
{
    ZPropDef *def;
    uint32_t header[14];
    uint32_t block[2];

    if (ziffBeginModel(iff) == 0) {
        return 1;
    }
    def = memset(zmemAlloc(0x54), 0, 0x54);
    if (ziffFindChunk(iff, 0x4e494450) == 0) {
        zdebugFail("zprop : Prop Def has no data!!!", kZpropFile, 0x94, 0);
        return 0;
    }
    ziffRead(iff, header, 0x38);
    if (header[0] != 0x38) {
        zdebugFail("zprop : Prop Def header size invalid", kZpropFile, 0x97, 0);
        return 0;
    }
    def->id = header[1];
    for (int i = 0; i < 7; i++) {
        def->params[i] = header[7 + i];
    }
    zstrcpy(def->name, (const char *)&header[2]);
    if (ziffFindChunk(iff, 0x4c444f4d) == 0) {
        zdebugFail("zprop : Prop Def has no geometry!!!", kZpropFile, 0xa7, 0);
        return 0;
    }
    def->model = zrwmatLoadChunk(iff);
    gZobjSetFlags(def->model, 2, 0);
    gZobjSetFlags(def->model, 8, 0);
    if (ziffFindChunk(iff, 0x494a424f) != 0) {
        ziffRead(iff, block, 8);
        def->extra = zmemAlloc(block[1]);
        ziffRead(iff, def->extra, block[1]);
    }
    def->pad2c[2] = 0;
    def->pad2c[0] = 0;
    def->pad2c[1] = 0;
    zrwListAdd(&gZpropDefs, def);
    zsceneAddEntry(NULL, header[1], 1, 1, def->model);
    ziffEndModel(iff);
    return 1;
}

BMX_ADDR(0x00486be0)
int zpropLoad(void *iff)
{
    uint32_t header[18];
    uint32_t block[2];
    ZPropDef *def;
    ZPropInst *inst;
    float scale;
    float *position;

    if (ziffBeginModel(iff) == 0) {
        return 1;
    }
    if (ziffFindChunk(iff, 0x4c425250) == 0) {
        zdebugFail("zprop : Prop has no data!!!", kZpropFile, 0xde, 0);
        return 0;
    }
    ziffRead(iff, header, 0x48);
    if (header[0] != 0x48) {
        zdebugFail("zprop : Prop header size invalid", kZpropFile, 0xe0, 0);
        return 0;
    }
    for (def = (ZPropDef *)gZpropDefs.head; def != NULL; def = def->next) {
        if (def->id == header[1]) {
            break;
        }
    }
    if (def == NULL) {
        zdebugFail("zprop : No prop def corresponds to the prop", kZpropFile, 0xe4, 0);
        return 0;
    }
    inst = memset(zmemAlloc(0x34), 0, 0x34);
    zmemCopy(inst->tag, &header[2], 0x10);
    inst->id = header[1];
    inst->pad28[1] = 0;
    inst->extra = NULL;
    zrwListAdd(&gZpropInsts, inst);
    zrwGetParameter(1, &scale, NULL);
    position = (float *)&header[15];
    position[0] = (float)((double)position[0] * (double)scale);
    position[1] = (float)((double)position[1] * (double)scale);
    position[2] = (float)((double)position[2] * (double)scale);
    inst->source = &def->id;
    inst->object = zsceneFindModel(0, def->name);
    if (inst->object == NULL) {
        zdebugFail("zprop : Unable to create an obj based on the prop def model", kZpropFile, 0xf9, 0);
    } else {
        ZMatrix matrix;
        ZVec3 at;
        ZAngle3 angles;
        ZQuat rotation;

        zmatrixIdentity(&matrix);
        at.x = position[0];
        at.y = position[1];
        at.z = position[2];
        zobjSetPosition(inst->object, &at);
        matrix.right.x = *(const float *)&header[6];
        matrix.right.y = *(const float *)&header[7];
        matrix.right.z = *(const float *)&header[8];
        matrix.up.x = *(const float *)&header[9];
        matrix.up.y = *(const float *)&header[10];
        matrix.up.z = *(const float *)&header[11];
        matrix.at.x = *(const float *)&header[12];
        matrix.at.y = *(const float *)&header[13];
        matrix.at.z = *(const float *)&header[14];
        zmatrixToEuler(&angles, &matrix, 0);
        zquatFromEuler(&rotation, &angles, 0);
        zobjSetRotation(inst->object, &rotation);
    }
    if (ziffFindChunk(iff, 0x494a424f) == 0) {
        inst->extra = NULL;
    } else {
        ziffRead(iff, block, 8);
        inst->extra = zmemAlloc(block[1]);
        ziffRead(iff, inst->extra, block[1]);
    }
    inst->pad28[2] = 0;
    inst->pad28[0] = 0;
    ziffEndModel(iff);
    return 1;
}
