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

/* Chunk ids (little endian fourccs). */
enum {
    kChunkPdin = 0x4e494450, /* "PDIN" */
    kChunkModl = 0x4c444f4d, /* "MODL" */
    kChunkObji = 0x494a424f, /* "OBJI" */
    kChunkPrbl = 0x4c425250, /* "PRBL" */
};

/* The PDIN chunk (a prop definition): size (0x38), id, model name, parameters. */
typedef struct ZPropDefHeader {
    uint32_t size;
    uint32_t id;
    char name[0x14];
    uint32_t params[7];
} ZPropDefHeader;

/* The PRBL chunk (a placed prop): size (0x48), definition id, tag, a 3x3 basis and the position. */
typedef struct ZPropInstHeader {
    uint32_t size;
    uint32_t id;
    uint32_t tag[4];
    ZVec3 right;
    ZVec3 up;
    ZVec3 at;
    float position[3];
} ZPropInstHeader;

/* The OBJI chunk starts with its own size word and the byte count that follows. */
typedef struct ZPropBlockHeader {
    uint32_t size;
    uint32_t bytes;
} ZPropBlockHeader;

_Static_assert(sizeof(ZPropDefHeader) == 0x38, "PDIN header");
_Static_assert(sizeof(ZPropInstHeader) == 0x48, "PRBL header");
_Static_assert(__builtin_offsetof(ZPropInstHeader, position) == 0x3c, "PRBL header");

#define gZpropDefs  ORIG_VAR(0x006722b8, ZrwList, gZpropDefs)
#define gZpropInsts ORIG_VAR(0x006722c4, ZrwList, gZpropInsts)

BMX_ADDR(0x00486a20)
int zpropLoadDef(void *iff)
{
    ZPropDef *def;
    ZPropDefHeader header;
    ZPropBlockHeader block;

    if (ziffBeginModel(iff) == 0) {
        return 1;
    }
    def = memset(zmemAlloc(sizeof(ZPropDef)), 0, sizeof(ZPropDef));
    if (ziffFindChunk(iff, kChunkPdin) == 0) {
        zdebugFail("zprop : Prop Def has no data!!!", kZpropFile, 0x94, 0);
        return 0;
    }
    ziffRead(iff, &header, sizeof header);
    if (header.size != sizeof header) {
        zdebugFail("zprop : Prop Def header size invalid", kZpropFile, 0x97, 0);
        return 0;
    }
    def->id = header.id;
    for (int i = 0; i < 7; i++) {
        def->params[i] = header.params[i];
    }
    zstrcpy(def->name, header.name);
    if (ziffFindChunk(iff, kChunkModl) == 0) {
        zdebugFail("zprop : Prop Def has no geometry!!!", kZpropFile, 0xa7, 0);
        return 0;
    }
    def->model = zrwmatLoadChunk(iff);
    gZobjSetFlags(def->model, 2, 0);
    gZobjSetFlags(def->model, 8, 0);
    if (ziffFindChunk(iff, kChunkObji) != 0) {
        ziffRead(iff, &block, sizeof block);
        def->extra = zmemAlloc(block.bytes);
        ziffRead(iff, def->extra, block.bytes);
    }
    def->pad2c[2] = 0;
    def->pad2c[0] = 0;
    def->pad2c[1] = 0;
    zrwListAdd(&gZpropDefs, def);
    zsceneAddEntry(NULL, header.id, 1, 1, def->model);
    ziffEndModel(iff);
    return 1;
}

BMX_ADDR(0x00486be0)
int zpropLoad(void *iff)
{
    ZPropInstHeader header;
    ZPropBlockHeader block;
    ZPropDef *def;
    ZPropInst *inst;
    float scale;
    float *position;

    if (ziffBeginModel(iff) == 0) {
        return 1;
    }
    if (ziffFindChunk(iff, kChunkPrbl) == 0) {
        zdebugFail("zprop : Prop has no data!!!", kZpropFile, 0xde, 0);
        return 0;
    }
    ziffRead(iff, &header, sizeof header);
    if (header.size != sizeof header) {
        zdebugFail("zprop : Prop header size invalid", kZpropFile, 0xe0, 0);
        return 0;
    }
    for (def = (ZPropDef *)gZpropDefs.head; def != NULL; def = def->next) {
        if (def->id == header.id) {
            break;
        }
    }
    if (def == NULL) {
        zdebugFail("zprop : No prop def corresponds to the prop", kZpropFile, 0xe4, 0);
        return 0;
    }
    inst = memset(zmemAlloc(sizeof(ZPropInst)), 0, sizeof(ZPropInst));
    zmemCopy(inst->tag, header.tag, sizeof inst->tag);
    inst->id = header.id;
    inst->pad28[1] = 0;
    inst->extra = NULL;
    zrwListAdd(&gZpropInsts, inst);
    zrwGetParameter(1, &scale, NULL);
    position = header.position;
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
        matrix.right.x = header.right.x;
        matrix.right.y = header.right.y;
        matrix.right.z = header.right.z;
        matrix.up.x = header.up.x;
        matrix.up.y = header.up.y;
        matrix.up.z = header.up.z;
        matrix.at.x = header.at.x;
        matrix.at.y = header.at.y;
        matrix.at.z = header.at.z;
        zmatrixToEuler(&angles, &matrix, 0);
        zquatFromEuler(&rotation, &angles, 0);
        zobjSetRotation(inst->object, &rotation);
    }
    if (ziffFindChunk(iff, kChunkObji) == 0) {
        inst->extra = NULL;
    } else {
        ziffRead(iff, &block, sizeof block);
        inst->extra = zmemAlloc(block.bytes);
        ziffRead(iff, inst->extra, block.bytes);
    }
    inst->pad28[2] = 0;
    inst->pad28[0] = 0;
    ziffEndModel(iff);
    return 1;
}
