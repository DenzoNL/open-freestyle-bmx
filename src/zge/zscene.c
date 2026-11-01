/* Scene database (C:\NgEngine\Zge\zScene.c). */
#include "zge/zscene.h"

#include "compat/crt.h"
#include "zge/com/zglrw/zrwcam.h"
#include "zge/com/zglrw/zrwmat.h"
#include "zge/com/zglrw/zrwworld.h"
#include "zge/zasset.h"
#include "zge/zdebug.h"
#include "zge/zfile.h"
#include "zge/zgemods/zfont.h"
#include "zge/zgemods/zobjlight.h"
#include "zge/zgemods/zscnpts.h"
#include "zge/zgemods/ztaginf.h"
#include "zge/ziff.h"
#include "zge/zmem.h"
#include "zge/zobj.h"
#include "zge/zstring.h"
#include "zge/ztoken.h"

static const char zsceneFile[] = "C:\\NgEngine\\Zge\\zScene.c";

/* Extension handlers (models, animations): {extension, handler}, ended by a NULL extension. */
typedef struct ZsceneExtension {
    const char *extension;
    int(BMX_CDECL *handler)(uint32_t *request);
} ZsceneExtension;

#define gSceneModelHandlers ORIG_VAR(0x005618dc, const ZsceneExtension *, gSceneModelHandlers)
#define gSceneAnimHandlers  ORIG_VAR(0x005618e4, const ZsceneExtension *, gSceneAnimHandlers)

/* A scene component chunk table entry: chunk id and handler. */
typedef struct ZsceneChunkHandler {
    uint32_t id;
    int(BMX_CDECL *handler)(void *iff);
} ZsceneChunkHandler;

/* Light object hooks forwarding to the renderer. TODO(file): these three probably belong to zobjlight.c. */
BMX_ADDR(0x004900d0)
void zobjLightSetColor(void *light, const uint8_t *rgba)
{
    gZrwLightColor(((ZObj *)light)->model, rgba);
}

BMX_ADDR(0x004900f0)
void zobjLightSetFlags(void *light, uint8_t flags)
{
    uint8_t bits = (flags & 1) != 0;

    if ((flags & 2) != 0) {
        bits |= 2;
    }
    gZrwLightFlags(((ZObj *)light)->model, bits);
}

BMX_ADDR(0x00490120)
void zobjLightSetDirection(void *light, const ZVec3 *direction)
{
    gZrwLightDirection(((ZObj *)light)->model, direction);
}

/* Free list destructor of a scene node. */
BMX_ADDR(0x004901b0)
int zsceneFreeNode(ZsceneNode *node)
{
    zmemFreeListFree(gScene.nodes, node);
    return 1;
}

BMX_ADDR(0x00490230)
int zsceneParseNode(ZsceneNode *node)
{
    char token[0x80];
    int more;

    if (ztokenExpect((char *)(uintptr_t)"{") == 0) {
        return 0;
    }
    for (more = ztokenNext(token, 0x80); more != 0; more = ztokenNext(token, 0x80)) {
        if (zstrcmp(token, "NAME") == 0) {
            if (ztokenReadString(token, 0x80) != 0) {
                if ((uint32_t)zstrlen(token) >= 0x20) {
                    zdebugFail("Object name is too long.", zsceneFile, 294, 0);
                }
                zstrcpy(node->name, token);
                node->nameHash = (uint32_t)zstrHash(node->name);
            }
        } else if (zstrcmp(token, "FILE") == 0) {
            if (ztokenReadString(token, 0x80) != 0) {
                if ((uint32_t)zstrlen(token) >= 0x40) {
                    zdebugFail("File name is too long.", zsceneFile, 307, 0);
                }
                zstrcpy(node->file, token);
            }
        } else if (zstrcmp(token, "TYPE") == 0) {
            int value;

            if (ztokenReadInt(&value) != 0) {
                node->token68 = value;
            }
        } else if (zstrcmp(token, "SUBTYPE") == 0) {
            int value;

            if (ztokenReadInt(&value) != 0) {
                node->subtype = value;
            }
        } else if (zstrcmp(token, "ANIM") == 0 || zstrcmp(token, "MODEL") == 0) {
            ZsceneNode *child = zmemFreeListAlloc(gScene.nodes);

            zmemFill(child, 0, sizeof(ZsceneNode));
            child->type = token[0] == 'A' ? kZsceneNodeAnim : kZsceneNodeModel;
            child->sibling = node->child;
            node->child = child;
            zsceneParseNode(child);
        } else if (zstrcmp(token, "}") == 0) {
            if (node != &gScene.common && (node == 0 || (uintptr_t)node == (uintptr_t)-0x20)) {
                gZdebugPrint("**ERROR**: Missing data\n");
                ztokenPrintTrace();
                return 0;
            }
            return 1;
        } else {
            gZdebugPrint("**ERROR** Unknown token:'%s'\n", token);
            ztokenPrintTrace();
            return 0;
        }
    }
    return 0;
}

BMX_ADDR(0x004904e0)
int zsceneLoadDatabase(const char *path)
{
    char token[0x80];

    if (ztokenOpen(path) == 0) {
        zdebugFail(zstrFormatTemp("Database not found:%s", path), zsceneFile, 466, 0);
        return 0;
    }
    if (ztokenExpect((char *)(uintptr_t)"SCENE_COMMON") != 0 && zsceneParseNode(&gScene.common) != 0 &&
        ztokenNext(token, 0x80) != 0) {
        while (zstrcmp(token, "SCENE") == 0) {
            ZsceneNode *node = zmemFreeListAlloc(gScene.nodes);

            zmemFill(node, 0, sizeof(ZsceneNode));
            node->type = kZsceneNodeScene;
            if (gScene.current != 0) {
                node->sibling = gScene.current->sibling;
                gScene.current->sibling = node;
            } else {
                gScene.current = node;
            }
            if (zsceneParseNode(node) == 0 || ztokenNext(token, 0x80) == 0) {
                goto fail;
            }
        }
        if (zstrcmp(token, "END") == 0) {
            ztokenClose();
            return 1;
        }
    }
fail:
    ztokenClose();
    return 0;
}

BMX_ADDR(0x00490630)
int zsceneFindNode(ZsceneNode *node, int32_t *query)
{
    if (node->type == query[0]) {
        if (query[1] == 0) {
            query[1] = -1;
            query[3] = (int32_t)(intptr_t)node;
            return 0;
        }
        if ((int32_t)node->nameHash == query[1]) {
            query[3] = (int32_t)(intptr_t)node;
            return 0;
        }
    }
    if (node->child != 0 && zsceneFindNode(node->child, query) == 0) {
        return 0;
    }
    if (node->sibling != 0 && zsceneFindNode(node->sibling, query) == 0) {
        return 0;
    }
    return (int)(intptr_t)node;
}

BMX_ADDR(0x004906a0)
int zsceneEntryMatch(ZsceneEntry *entry, int32_t *query)
{
    if ((int8_t)entry->flags == query[0] && (int32_t)entry->id == query[1]) {
        query[2] = (int32_t)(intptr_t)entry;
        return 0;
    }
    return 1;
}

BMX_ADDR(0x00490710)
ZsceneEntry *zsceneAddEntry(ZsceneNode *node, uint32_t id, uint32_t type, uint8_t kind, void *object)
{
    ZsceneEntry *entry;

    if (gScene.open == 0) {
        zdebugFail("No scene is open", zsceneFile, 612, 0);
        return 0;
    }
    entry = zmemFreeListAlloc(gScene.entries);
    entry->id = id;
    entry->node = node;
    entry->object = object;
    entry->flags =
        ((entry->flags & 0xffff0000u) ^ (type & 0xff)) | ((uint32_t)kind << 8) | kZsceneEntryScoped;
    return entry;
}

BMX_ADDR(0x00490790)
int zsceneLoadChunks(void *data, uint32_t size, const char *directory, int kind, int tableCount,
                     const void *table)
{
    const ZsceneChunkHandler *handlers = table;
    uint32_t dataSize;
    uint8_t *record;
    char path[0x80];
    void *image;
    ZfileHandle *file;
    void *iff;

    record = gZrwWorldFind(data, kind, &dataSize);
    if (record == 0) {
        return 0;
    }
    if (*(uint32_t *)(record + 4) != 0) {
        zstrFormat(path, "%s%s", directory, (char *)(record + 8));
        image = zfileLoadBytes(path, (int)dataSize);
        if (image == 0) {
            zdebugFail(zstrFormatTemp("Scene component file not found:%s", path), zsceneFile, 750, 0);
            return 0;
        }
        file = zfileOpenRamDisk((uint32_t)(uintptr_t)image, dataSize);
        if (file == 0) {
            zdebugFail("Couldn't open ramdisk file", zsceneFile, 752, 0);
            return 0;
        }
        iff = ziffOpen(file, 0xffffffffu, dataSize);
        if (iff != 0) {
            while (tableCount != 0) {
                int found;

                tableCount--;
                for (found = ziffFindChunk(iff, handlers->id); found != 0;
                     found = ziffNextChunk(iff, handlers->id)) {
                    if (handlers->handler(iff) == 0) {
                        ziffClose(iff);
                        zfileClose(file, 0);
                        zfileFreeBuffer(image);
                        return 0;
                    }
                }
                handlers++;
            }
            ziffClose(iff);
        }
        zfileClose(file, 0);
        zfileFreeBuffer(image);
    }
    return 1;
}

BMX_ADDR(0x004909d0)
void zsceneReleaseEntries(int all)
{
    ZMemFreeNode *node = zmemFreeListFirst(gScene.entries);

    while (node != 0) {
        ZMemFreeNode *next = zmemFreeNodeNext(node);
        ZsceneEntry *entry = zmemFreeNodePayload(node);
        uint32_t flags = entry->flags;

        node = next;
        if ((flags & kZsceneEntryScoped) == 0 && all == 0) {
            continue;
        }
        switch ((int8_t)flags) {
        case kZsceneEntryModel:
            if ((uint8_t)(flags >> 8) == 0) {
                void *object = entry->object;

                if (((ZObj *)object)->model != 0) {
                    zsceneForgetEntry(1, 0, object, 0);
                }
                zobjFree(object);
            } else {
                zrwmatReleaseModel(entry->object);
                zsceneForgetEntry((int8_t)flags, 1, entry->object, entry);
            }
            break;
        case kZsceneEntryAnimLib:
            if (entry != 0) {
                uint32_t *anim = entry->object;

                while (anim != 0) {
                    uint32_t *following = (uint32_t *)(uintptr_t)anim[2];

                    ziffFreeAnim((void *)(uintptr_t)anim[1]);
                    zmemFree(anim);
                    anim = following;
                }
                zsceneForgetEntry(2, 2, entry->object, 0);
            } else {
                zdebugFail("NULL animLibID", zsceneFile, 0x603, 0);
            }
            break;
        case kZsceneEntryLight:
        case kZsceneEntryType4:
        case kZsceneEntryWorld: {
            void *object = entry->object;

            zsceneForgetEntry((int8_t)flags, 0, object, 0);
            zobjFree(object);
            break;
        }
        default:
            zdebugFail(zstrFormatTemp("Unknown scene EzsceneObjType:%d", (int8_t)flags), zsceneFile, 0x395,
                       0);
            break;
        }
    }
}

BMX_ADDR(0x00490b70)
int zsceneForgetEntry(int type, int kind, void *object, ZsceneEntry *entry)
{
    ZMemFreeNode *node;
    int found = 0;

    if (entry != 0) {
        zmemFreeListFree(gScene.entries, entry);
        return 1;
    }
    node = zmemFreeListFirst(gScene.entries);
    while (node != 0) {
        if (found) {
            break;
        }
        entry = zmemFreeNodePayload(node);
        if ((int8_t)entry->flags == type) {
            switch (type) {
            case 1:
                if ((int8_t)(entry->flags >> 8) != kind) {
                    goto next;
                }
                if (entry->object == object) {
                    found = 1;
                }
                break;
            case 2:
            case 3:
            case 4:
            case 5:
                if (entry->object == object) {
                    found = 1;
                }
                break;
            default:
                zdebugFail("Uncoded", zsceneFile, 0x2ae, 0);
                return 0;
            }
        }
    next:
        node = zmemFreeNodeNext(node);
    }
    if (!found) {
        gZdebugPrint("ZSCENE: Warning object was not in scene:Type:%d, idType:%d, ZID:0x%x\n", type, kind,
                     (uint32_t)(uintptr_t)object);
        return 0;
    }
    zmemFreeListFree(gScene.entries, entry);
    return 1;
}

BMX_ADDR(0x00490cc0)
int zsceneLoadModelFile(uint32_t *request)
{
    void *model = zrwmatLoadModel((void *)(uintptr_t)request[0]);

    request[2] = (uint32_t)(uintptr_t)model;
    return model != 0;
}

BMX_ADDR(0x00490d90)
int zsceneLoadAnimFile(uint32_t *request)
{
    uint32_t size;
    void *image = zfileLoad((const char *)(uintptr_t)request[0], (int *)&size);
    ZfileHandle *file;
    void *iff;
    ZsceneAnim *list = 0;

    if (image == 0) {
        zdebugFail(zstrFormatTemp("AnimLib not found:%s", (const char *)(uintptr_t)request[0]), zsceneFile,
                   1062, 0);
        return 0;
    }
    file = zfileOpenRamDisk((uint32_t)(uintptr_t)image, size);
    iff = ziffOpen(file, 0x4d524f46, 0);
    if (iff != 0) {
        int found;

        for (found = ziffFindChunk(iff, 0x4d494e41); found != 0; found = ziffNextChunk(iff, 0x4d494e41)) {
            void *anim = ziffReadAnim(iff);
            ZsceneAnim *entry = list;

            if (anim != 0) {
                entry = zmemAlloc(sizeof *entry);
                entry->anim = anim;
                entry->name = ziffAnimName(anim);
                entry->next = list;
            }
            list = entry;
        }
        ziffClose(iff);
    }
    request[1] = (uint32_t)(uintptr_t)list;
    zfileClose(file, 0);
    zfileFreeBuffer(image);
    return 1;
}

BMX_ADDR(0x00490e90)
int zsceneLoadAnimLibrary(uint32_t *request)
{
    ZsceneAnim *list = 0;
    int lib = zassetOpenLibrary((const char *)(uintptr_t)request[0]);
    int count;

    if (lib == 0) {
        zdebugFail(zstrFormatTemp("AnimLib not found:%s", (const char *)(uintptr_t)request[0]), zsceneFile,
                   1096, 0);
        return 0;
    }
    for (count = zassetCount(lib); count != 0; count--) {
        uint32_t size = zassetSize(lib);
        void *image = zmemAlloc(size);

        if (zassetRead(lib, image) != 0) {
            ZfileHandle *file = zfileOpenRamDisk((uint32_t)(uintptr_t)image, size);
            void *iff = ziffOpen(file, 0x4d524f46, 0);

            if (iff != 0) {
                int found;

                for (found = ziffFindChunk(iff, 0x4d494e41); found != 0;
                     found = ziffNextChunk(iff, 0x4d494e41)) {
                    void *anim = ziffReadAnim(iff);
                    ZsceneAnim *entry = list;

                    if (anim != 0) {
                        entry = zmemAlloc(sizeof *entry);
                        entry->anim = anim;
                        entry->name = ziffAnimName(anim);
                        entry->next = list;
                    }
                    list = entry;
                }
                ziffClose(iff);
            }
            zfileClose(file, 0);
        }
        zmemFree(image);
        zassetNext(lib);
    }
    zassetCloseLibrary(lib);
    request[1] = (uint32_t)(uintptr_t)list;
    return 1;
}

BMX_ADDR(0x00490ff0)
int zsceneInitialize(const char *path)
{
    if (gScene.initialized != 0) {
        zdebugFail("Already initialized", zsceneFile, 1148, 0);
    }
    zmemFill(&gScene, 0, sizeof gScene);
    gScene.entries = zmemFreeListCreate(sizeof(ZsceneEntry), 0x100, 0);
    gScene.nodes =
        zmemFreeListCreate(sizeof(ZsceneNode), 0x100, BMX_FNPTR(ZMemDestroyFn, 0x004901b0, zsceneFreeNode));
    if (path == 0) {
        path = "Scene.Txt";
    }
    if (zsceneLoadDatabase(path) == 0) {
        gZdebugPrint("**ERROR** ZSCENE: Loading database\n");
        return 0;
    }
    gScene.initialized = 1;
    if (zscnptsInitialize() == 0) {
        return 0;
    }
    if (zpanoramaInitialize() == 0) {
        return 0;
    }
    if (zmemDeferredInit() == 0) {
        return 0;
    }
    return zfontInitialize() != 0;
}

/* Tables of {chunk id, handler} for the scene components (in the exe's data). */
#define kSceneTagChunks  ((const void *)BMX_ORIG(0x00561900))
#define kSceneScptChunks ((const void *)BMX_ORIG(0x00561908))
#define kScenePdblChunks ((const void *)BMX_ORIG(0x00561918))
#define kScenePrblChunks ((const void *)BMX_ORIG(0x00561920))

BMX_ADDR(0x004911c0)
int zsceneOpen(const char *name)
{
    int32_t query[4];
    ZsceneNode *current;
    ZsceneNode *node = 0;
    uint32_t size;
    char directory[0x80];
    void *image;
    const char *file;

    if (gScene.open != 0) {
        zdebugFail("There is already a scene open", zsceneFile, 1265, 0);
        return 0;
    }
    query[0] = 0;
    query[1] = zstrHash(name);
    query[2] = 0;
    query[3] = 0;
    current = gScene.current;
    if (current->type == 0 && (query[1] == 0 || current->nameHash == (uint32_t)query[1])) {
        node = current;
    } else {
        int found = 1;

        if (current->child != 0) {
            found = zsceneFindNode(current->child, query);
            node = (ZsceneNode *)(intptr_t)query[3];
        }
        if ((current->child == 0 || found != 0) && current->sibling != 0) {
            zsceneFindNode(current->sibling, query);
            node = (ZsceneNode *)(intptr_t)query[3];
        }
    }
    if (node == 0) {
        zdebugFail(zstrFormatTemp("Scene not found:%s\n", name), zsceneFile, 1276, 0);
        return 0;
    }
    gScene.open = node;
    file = node->file;
    image = zfileLoad(file, (int *)&size);
    if (image == 0) {
        zdebugFail(zstrFormatTemp("ZSCENE: Could not load %s", file), zsceneFile, 1289, 0);
        return 0;
    }
    zfileGetDirectory(file, directory);
    if (gZrwWorldLoad(image, directory) == 0) {
        zdebugFail(zstrFormatTemp("Error loading:%s", file), zsceneFile, 1298, 0);
        zfileFreeBuffer(image);
        return 0;
    }
    if (zsceneLoadChunks(image, size, directory, 3, 1, kSceneTagChunks) == 0) {
        gZdebugPrint("zrwWorld: Failed to load scene tagging info\n");
    } else if (zsceneLoadChunks(image, size, directory, 6, 2, kSceneScptChunks) == 0 ||
               zsceneLoadChunks(image, size, directory, 2, 1, kScenePdblChunks) == 0 ||
               zsceneLoadChunks(image, size, directory, 5, 1, kScenePrblChunks) == 0) {
        gZdebugPrint("zrwWorld: Failed to load scene point\n");
    }
    zfileFreeBuffer(image);
    return 1;
}

BMX_ADDR(0x004913d0)
int zsceneClose(void)
{
    if (gScene.open != 0) {
        ztaginfRelease();
        zscnptsShutdown();
        zpanoramaShutdown();
        zmemDeferredFlush();
        zsceneReleaseEntries(0);
        gZrwWorldClose();
        gScene.open = 0;
    }
    return 1;
}

typedef void(BMX_CDECL *ZMemEachFn)(void *payload, void *arg);

/*
 * The node of the query's type and name below the open scene (or the last one of the file). A name hash
 * of 0 takes the first node of the type and marks the query with -1.
 */
static ZsceneNode *findNodeOfType(int32_t *query)
{
    ZsceneNode *start = gScene.open != 0 ? gScene.open : gScene.current;
    ZsceneNode *node = 0;
    int found = 1;

    if (start->type == query[0] && (query[1] == 0 || start->nameHash == (uint32_t)query[1])) {
        if (query[1] == 0) {
            query[1] = -1;
        }
        return start;
    }
    if (start->child != 0) {
        found = zsceneFindNode(start->child, query);
        node = (ZsceneNode *)(intptr_t)query[3];
    }
    if ((start->child == 0 || found != 0) && start->sibling != 0) {
        zsceneFindNode(start->sibling, query);
        node = (ZsceneNode *)(intptr_t)query[3];
    }
    return node;
}

/* Falls back to the common scene when the search found nothing. */
static ZsceneNode *findInCommon(int32_t *query, ZsceneNode *node)
{
    int found = 1;

    if (node != 0) {
        return node;
    }
    if (gScene.common.type == query[0] && (query[1] == 0 || (int32_t)gScene.common.nameHash == query[1])) {
        return &gScene.common;
    }
    if (gScene.common.child != 0) {
        found = zsceneFindNode(gScene.common.child, query);
        node = (ZsceneNode *)(intptr_t)query[3];
    }
    if ((gScene.common.child == 0 || found != 0) && gScene.common.sibling != 0) {
        zsceneFindNode(gScene.common.sibling, query);
        node = (ZsceneNode *)(intptr_t)query[3];
    }
    return node;
}

/* Handler registered for the (upper cased) extension; NULL when there is none or it has no handler. */
static const ZsceneExtension *findExtensionHandler(const ZsceneExtension *table, const char *extension)
{
    if (table == 0 || table->extension == 0) {
        return 0;
    }
    while (table->extension != 0) {
        if (zstrcmp(table->extension, extension) == 0) {
            return table->handler != 0 ? table : 0;
        }
        table++;
    }
    return 0;
}

BMX_ADDR(0x00491410)
void *zsceneFindModel(int flags, const char *name)
{
    const uint32_t hash = (uint32_t)zstrHash(name);
    int32_t query[4];
    int32_t search[3] = {1, (int32_t)hash, 0};
    ZsceneEntry *entry;
    ZsceneNode *node;
    void *model;
    void *object;
    char extension[4];

    zmemFreeListForEach(gScene.entries, BMX_FNPTR(ZMemEachFn, 0x004906a0, zsceneEntryMatch), search);
    entry = (ZsceneEntry *)(intptr_t)search[2];
    if (entry != 0) {
        if ((uint8_t)(entry->flags >> 8) == 0) {
            model = zrwmatModelFromObject(zobjGetModel(entry->object));
        } else {
            model = zrwmatModelFromObject(entry->object);
        }
        node = entry->node;
    } else {
        const ZsceneExtension *handler;
        const char *file;

        query[0] = 1;
        query[1] = (int32_t)hash;
        query[2] = 0;
        query[3] = 0;
        node = findInCommon(query, findNodeOfType(query));
        if (node == 0) {
            zdebugFail(zstrFormatTemp("Model not in scene:%s", name), zsceneFile, 1388, 0);
            return 0;
        }
        file = node->file;
        zfileGetExtension(file, extension);
        zstrupr(extension);
        handler = findExtensionHandler(gSceneModelHandlers, extension);
        if (handler == 0) {
            zdebugFail(zstrFormatTemp("Unknown model extension:%s", extension), zsceneFile, 1397, 0);
            return 0;
        }
        /* The request reuses the query's stack slots: {file, 0, result}. */
        query[0] = (int32_t)(intptr_t)file;
        query[1] = 0;
        query[2] = 0;
        if (handler->handler((uint32_t *)query) == 0) {
            zdebugFail(zstrFormatTemp("Model not found:%s", file), zsceneFile, 1406, 0);
            return 0;
        }
        model = (void *)(intptr_t)query[2];
    }
    if (model == 0) {
        return 0;
    }
    if ((flags & 2) != 0) {
        object = (void *)(uintptr_t)name; /* returns the name as is */
    } else {
        object = zrwmatCreateObject(model);
        if (object == 0) {
            zdebugFail(zstrFormatTemp("Failed to create object for:%s", name), zsceneFile, 1420, 0);
            return 0;
        }
    }
    if ((flags & 1) == 0) {
        ZsceneEntry *created;

        if (gScene.open == 0) {
            zdebugFail("No scene is open", zsceneFile, 612, 0);
            return object;
        }
        created = zmemFreeListAlloc(gScene.entries);
        created->node = node;
        created->id = hash;
        created->object = object;
        created->flags = (created->flags & 0xffff0001u) | 0x10001u;
    }
    return object;
}

BMX_ADDR(0x00491720)
void *zsceneFindAnim(int flags, const char *name)
{
    const uint32_t hash = (uint32_t)zstrHash(name);
    int32_t search[3] = {2, (int32_t)hash, 0};
    int32_t query[4];
    ZsceneNode *node;
    ZsceneEntry *entry;
    const ZsceneExtension *handler;
    const char *file;
    char extension[4];
    uint32_t request[2];

    zmemFreeListForEach(gScene.entries, BMX_FNPTR(ZMemEachFn, 0x004906a0, zsceneEntryMatch), search);
    if (search[2] != 0) {
        return (void *)(intptr_t)search[2];
    }
    query[0] = 2;
    query[1] = (int32_t)hash;
    query[2] = 0;
    query[3] = 0;
    node = findInCommon(query, findNodeOfType(query));
    if (node == 0) {
        zdebugFail(zstrFormatTemp("Anim not in scene:%s", name), zsceneFile, 1489, 0);
        return 0;
    }
    file = node->file;
    zfileGetExtension(file, extension);
    zstrupr(extension);
    handler = findExtensionHandler(gSceneAnimHandlers, extension);
    if (handler == 0) {
        zdebugFail("Unknown extension", zsceneFile, 1498, 0);
        return 0;
    }
    request[0] = (uint32_t)(uintptr_t)file;
    request[1] = 0;
    if (handler->handler(request) == 0) {
        zdebugFail(zstrFormatTemp("Anim not found:%s", file), zsceneFile, 1507, 0);
        return 0;
    }
    if (gScene.open == 0) {
        zdebugFail("No scene is open", zsceneFile, 612, 0);
        entry = 0;
    } else {
        entry = zmemFreeListAlloc(gScene.entries);
        entry->node = node;
        entry->id = hash;
        entry->object = (void *)(uintptr_t)request[1];
        entry->flags = (entry->flags & 0xffff0202u) | 0x10202u;
    }
    if ((flags & 1) != 0) {
        entry->flags &= 0xfffeffffu;
    }
    return entry;
}

BMX_ADDR(0x00491a80)
int zsceneAnimTable(void *library, uint32_t *table, int size)
{
    const ZsceneAnim *node = ((const ZsceneEntry *)library)->object;
    int count = 0;
    const ZsceneAnim *p;

    for (p = node; p != 0; p = p->next) {
        count++;
    }
    if (count > size) {
        zdebugFail(zstrFormatTemp("Table not big enough (TabSize:%d, NumMotions:%d", size, count), zsceneFile,
                   1626, 0);
        return 0;
    }
    if (node != 0) {
        uint32_t *dst = table + count;

        do {
            dst--;
            *dst = (uint32_t)(uintptr_t)node->anim;
            node = node->next;
        } while (node != 0);
    }
    if (count < size) {
        uint32_t *dst = table + count;
        int left = size - count;

        do {
            *dst++ = table[0];
            left--;
        } while (left != 0);
    }
    return count;
}

BMX_ADDR(0x00491ba0)
void *zsceneAddLight(int flags, int type, int param)
{
    void *light = zobjLightCreate(type, param);

    if (light != 0 && (flags & 1) == 0) {
        ZsceneEntry *entry;

        if (gScene.open == 0) {
            zdebugFail("No scene is open", zsceneFile, 612, 0);
            return light;
        }
        entry = zmemFreeListAlloc(gScene.entries);
        entry->node = 0;
        entry->id = 0;
        entry->object = light;
        entry->flags = (entry->flags & 0xffff0003u) | 0x10003u;
    }
    return light;
}

BMX_ADDR(0x00491c50)
void *zsceneAddWorldObject(int flags)
{
    void *object = zrwworldCreateObject();

    if (object != 0 && (flags & 1) == 0) {
        ZsceneEntry *entry;

        if (gScene.open == 0) {
            zdebugFail("No scene is open", zsceneFile, 612, 0);
            return object;
        }
        entry = zmemFreeListAlloc(gScene.entries);
        entry->node = 0;
        entry->id = 0;
        entry->object = object;
        entry->flags = (entry->flags & 0xffff0005u) | 0x10005u;
    }
    return object;
}

/* TODO(file): the scene point and panorama functions below probably belong to zscnpts.c. */
BMX_ADDR(0x00491d80)
int zscnptsReset(void)
{
    ZMemFreeList *points = gScnpts.points;

    gScnpts.count = 0;
    gScnpts.field14 = 0;
    if (points == 0) {
        gScnpts.points =
            zmemFreeListCreate(sizeof(ZrwMarker), 0, BMX_FNPTR(ZMemDestroyFn, 0x00491dc0, zscnptsFreeNode));
    }
    return 1;
}

BMX_ADDR(0x00491dc0)
int zscnptsFreeNode(void *node)
{
    uint32_t *fields = node;

    if (node == 0) {
        return 0;
    }
    if (fields[0x48 / 4] != 0) {
        zmemFree((void *)(uintptr_t)fields[0x48 / 4]);
        fields[0x48 / 4] = 0;
    }
    return 1;
}

BMX_ADDR(0x00491df0)
int zscnptsInitialize(void)
{
    gScnpts.first = 0;
    gScnpts.points = 0;
    gScnpts.count = 0;
    gScnpts.field14 = 0;
    gScnpts.points =
        zmemFreeListCreate(sizeof(ZrwMarker), 0, BMX_FNPTR(ZMemDestroyFn, 0x00491dc0, zscnptsFreeNode));
    return 1;
}

BMX_ADDR(0x00491ee0)
void zscnptsShutdown(void)
{
    gScnpts.count = 0;
    zmemFreeListForEach(gScnpts.points, (void(BMX_CDECL *)(void *, void *))(uintptr_t)zscnptsFreePoint, 0);
    zscnptsReset();
}

BMX_ADDR(0x004a1060)
int zpanoramaInitialize(void)
{
    gPanorama = 0;
    return 1;
}

BMX_ADDR(0x004a1080)
void zpanoramaShutdown(void) {}

BMX_ADDR(0x004a10a0)
int zpanoramaLoadChunk(void *iff)
{
    if (ziffBeginModel(iff) != 0) {
        if (ziffFindChunk(iff, 0x4c444f4d) == 0) {
            ziffEndModel(iff);
            return 0;
        }
        gPanorama = zrwmatLoadChunk(iff);
        if (gPanorama != 0) {
            zsceneAddEntry(0, (uint32_t)zstrHash("Panorama"), 1, 1, gPanorama);
        }
        ziffEndModel(iff);
    }
    return 1;
}
