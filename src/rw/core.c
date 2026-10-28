/* RenderWare 3.1 core: free lists, error slot, object/frame linkage, plugin registry, string table. */
#include "rw/rwcore.h"
#include "rw/rwmath.h"
#include "rw/rwstream.h"

/* ---- free lists ------------------------------------------------------------------------------------- */

/*
 * Creates a free list: entries of entrySize (rounded up to the alignment) in blocks of entriesPerBlock.
 * The list of free entry pointers starts with room for one pointer; blocks are added on demand.
 */
BMX_ADDR(0x004dba90)
RwFreeList *RwFreeListCreate(int32_t entrySize, int32_t entriesPerBlock, int32_t alignment)
{
    RwFreeList *freeList = RwEngineInstance->memoryFuncs.rwmalloc(sizeof(RwFreeList));
    if (freeList == NULL) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(rwERR_NOMEM, (int32_t)sizeof(RwFreeList));
        RwErrorSet(&error);
        return NULL;
    }
    if (alignment == 0) {
        alignment = 1;
    }
    freeList->alignmentMask = (uint32_t)alignment - 1;
    freeList->entriesPerBlock = entriesPerBlock;
    freeList->entrySize =
        (int32_t)(((uint32_t)entrySize + (uint32_t)alignment - 1) & ~freeList->alignmentMask);
    freeList->blockSize =
        (int32_t)((uint32_t)entriesPerBlock * (uint32_t)freeList->entrySize + (uint32_t)alignment + 3);
    freeList->freeBegin = (void **)RwEngineInstance->memoryFuncs.rwmalloc(4);
    if (freeList->freeBegin == NULL) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(rwERR_NOMEM, 4);
        RwErrorSet(&error);
        RwEngineInstance->memoryFuncs.rwfree(freeList);
        return NULL;
    }
    freeList->blocks = NULL;
    freeList->numBlocks = 0;
    freeList->freeTop = freeList->freeBegin - 1;
    freeList->link.prev = &rwFreeListHead;
    freeList->link.next = rwFreeListHead.next;
    rwFreeListHead.next->prev = &freeList->link;
    rwFreeListHead.next = &freeList->link;
    return freeList;
}

/* Releases the free entry stack, all blocks and the list itself; always returns 1. */
BMX_ADDR(0x004dbbc0)
int RwFreeListDestroy(RwFreeList *freeList)
{
    if (freeList->freeBegin != NULL) {
        RwEngineInstance->memoryFuncs.rwfree((void *)freeList->freeBegin);
    }
    RwFreeListBlock *block = freeList->blocks;
    while (block != NULL) {
        RwFreeListBlock *next = block->next;
        RwEngineInstance->memoryFuncs.rwfree(block);
        block = next;
    }
    freeList->link.prev->next = freeList->link.next;
    freeList->link.next->prev = freeList->link.prev;
    RwEngineInstance->memoryFuncs.rwfree(freeList);
    return 1;
}

/* Calls fn for every entry of every block that is not on the free stack. */
BMX_ADDR(0x004dbf70)
RwFreeList *RwFreeListForAllUsed(RwFreeList *freeList, RwFreeListCallBack fn, void *data)
{
    for (RwFreeListBlock *block = freeList->blocks; block != NULL; block = block->next) {
        uint8_t *entry = (uint8_t *)(block + 1);
        for (uint32_t left = (uint32_t)freeList->entriesPerBlock; left != 0; left--) {
            void **top = freeList->freeTop;
            void **at = freeList->freeBegin;
            while ((uintptr_t)at <= (uintptr_t)top && *at != (void *)entry) {
                at++;
            }
            if ((uintptr_t)at > (uintptr_t)top) {
                fn(entry, data);
            }
            entry += freeList->entrySize;
        }
    }
    return freeList;
}

/*
 * The engine's default free list allocator (RwGlobals.freeListAlloc): pops the top free entry. An empty list
 * first gets a new block of entriesPerBlock entries and a larger free stack (the old stack is empty and
 * freed), then hands out the entry at the block's end. NULL, with the error set, when memory runs out.
 */
BMX_ADDR(0x004dbc30)
void *_rwFreeListAllocReal(RwFreeList *freeList)
{
    if ((uintptr_t)freeList->freeTop < (uintptr_t)freeList->freeBegin) {
        uint8_t *block = RwEngineInstance->memoryFuncs.rwmalloc((size_t)freeList->blockSize);
        if (block == NULL) {
            RwError error;
            error.pluginID = 1;
            error.errorCode = _rwerror(rwERR_NOMEM, freeList->blockSize);
            RwErrorSet(&error);
            return NULL;
        }
        const uint32_t stackSize = ((uint32_t)freeList->entriesPerBlock + (uint32_t)freeList->numBlocks) * 4;
        void **stack = (void **)RwEngineInstance->memoryFuncs.rwmalloc(stackSize);
        if (stack == NULL) {
            RwError error;
            error.pluginID = 1;
            error.errorCode = _rwerror(rwERR_NOMEM, (int32_t)stackSize);
            RwErrorSet(&error);
            RwEngineInstance->memoryFuncs.rwfree(block);
            return NULL;
        }
        RwEngineInstance->memoryFuncs.rwfree((void *)freeList->freeBegin);
        freeList->freeBegin = stack;
        freeList->numBlocks += freeList->entriesPerBlock;
        ((RwFreeListBlock *)block)->next = freeList->blocks;
        uintptr_t entry = ((uintptr_t)(freeList->blockSize - freeList->entrySize) + (uintptr_t)block) &
                          ~(uintptr_t)freeList->alignmentMask;
        freeList->blocks = (RwFreeListBlock *)block;
        for (int32_t left = freeList->entriesPerBlock; left != 0; left--) {
            *stack++ = (void *)entry;
            entry -= (uintptr_t)freeList->entrySize;
        }
        freeList->freeTop =
            (void **)((uintptr_t)freeList->freeBegin + (uint32_t)freeList->entriesPerBlock * 4 - 4);
    }
    void **top = freeList->freeTop;
    freeList->freeTop = top - 1;
    return *top;
}

/* ---- errors ----------------------------------------------------------------------------------------- */

static RwError *rwErrorSlot(void)
{
    return (RwError *)((uint8_t *)RwEngineInstance + rwErrorOffset);
}

/* Returns its first argument; the original takes (and ignores) further arguments for the message. */
BMX_ADDR(0x004ded80)
int32_t _rwerror(int32_t code, ...)
{
    return code;
}

/* Hands out the pending error and clears the slot. */
BMX_ADDR(0x004ded90)
void RwErrorGet(RwError *error)
{
    error->pluginID = rwErrorSlot()->pluginID;
    error->errorCode = rwErrorSlot()->errorCode;
    rwErrorSlot()->pluginID = 0;
    rwErrorSlot()->errorCode = rwERRNONE;
}

/* Records an error unless one is already pending (the first error wins); core codes carry plugin id 0. */
BMX_ADDR(0x004dedd0)
void RwErrorSet(const RwError *error)
{
    if (rwErrorSlot()->pluginID == 0 && rwErrorSlot()->errorCode == rwERRNONE) {
        if ((error->errorCode & (int32_t)0x80000000) == 0) {
            rwErrorSlot()->pluginID = error->pluginID;
        } else {
            rwErrorSlot()->pluginID = 0;
        }
        rwErrorSlot()->errorCode = error->errorCode;
    }
}

/* ---- object / frame linkage ------------------------------------------------------------------------- */

/* Moves the object into the frame's object list (leaving the old frame's), then updates the frame. */
BMX_ADDR(0x004e8740)
void _rwObjectHasFrameSetFrame(RwObjectHasFrame *object, RwFrame *frame)
{
    if (object->object.parent != NULL) {
        object->lFrame.prev->next = object->lFrame.next;
        object->lFrame.next->prev = object->lFrame.prev;
    }
    object->object.parent = frame;
    if (frame != NULL) {
        object->lFrame.prev = &frame->objectList;
        object->lFrame.next = frame->objectList.next;
        frame->objectList.next->prev = &object->lFrame;
        frame->objectList.next = &object->lFrame;
        RwFrameUpdateObjects(frame);
    }
}

BMX_ADDR(0x004e87a0)
void _rwObjectHasFrameReleaseFrame(RwObjectHasFrame *object)
{
    if (object->object.parent != NULL) {
        object->lFrame.prev->next = object->lFrame.next;
        object->lFrame.next->prev = object->lFrame.prev;
    }
}

/* ---- plugin registry -------------------------------------------------------------------------------- */

/* The defaults for a plugin without constructor, destructor or copy callback: return the object. */
BMX_ADDR(0x004e8b60)
void *rwPluginDefaultCopy(void *dst, const void *src, int32_t offset, int32_t size)
{
    return dst;
}

BMX_ADDR(0x004e8b70)
void *rwPluginDefaultDestructor(void *object, int32_t offset, int32_t size)
{
    return object;
}

BMX_ADDR(0x004e8b80)
void *rwPluginDefaultConstructor(void *object, int32_t offset, int32_t size)
{
    return object;
}

/* Free list callback (0x4e8800) used when the registry closes: resets the registry, returns the entry. */
BMX_ADDR(0x004e8800)
void rwPluginRegEntryRelease(void *entry, void *freeList)
{
    RwPluginRegistry *registry = ((RwPluginRegEntry *)entry)->parentRegistry;
    if (registry->firstRegEntry != NULL) {
        registry->sizeOfStruct = registry->origSizeOfStruct;
        ((RwPluginRegEntry *)entry)->parentRegistry->firstRegEntry = NULL;
        ((RwPluginRegEntry *)entry)->parentRegistry->lastRegEntry = NULL;
    }
    RwEngineInstance->freeListFree(freeList, entry);
}

BMX_ADDR(0x004e87c0)
int _rwPluginRegistryOpen(void)
{
    rwPluginRegEntryFreeList = RwFreeListCreate(0x34, 0x14, 0);
    return rwPluginRegEntryFreeList != NULL;
}

BMX_ADDR(0x004e8840)
int _rwPluginRegistryClose(void)
{
    if (rwPluginRegEntryFreeList != NULL) {
        RwFreeListForAllUsed(rwPluginRegEntryFreeList,
                             BMX_FNPTR(RwFreeListCallBack, 0x004e8800, rwPluginRegEntryRelease),
                             rwPluginRegEntryFreeList);
        RwFreeListDestroy(rwPluginRegEntryFreeList);
        rwPluginRegEntryFreeList = NULL;
    }
    return 1;
}

/*
 * Appends a plugin to the registry and returns its data offset inside the object (the object grows by
 * size rounded up to 4). A plugin id that is already registered returns its existing offset. Fails (-1)
 * when the registry is not open, after the engine opened, over the size limit or out of entries.
 */
BMX_ADDR(0x004e88d0)
int32_t _rwPluginRegistryAddPlugin(RwPluginRegistry *registry, int32_t size, int32_t pluginID,
                                   RwPluginObjectConstructor constructCB, RwPluginObjectDestructor destructCB,
                                   RwPluginObjectCopy copyCB)
{
    if (rwPluginRegEntryFreeList == NULL) {
        return -1;
    }
    if (_rwGetNumEngineInstances() != 0) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(rwERR_ALREADYOPEN);
        RwErrorSet(&error);
        return -1;
    }
    for (RwPluginRegEntry *entry = registry->firstRegEntry; entry != NULL; entry = entry->nextRegEntry) {
        if (entry->pluginID == pluginID) {
            return entry->offset;
        }
    }
    const int32_t newSize = (int32_t)((((uint32_t)size + 3u) & ~3u) + (uint32_t)registry->sizeOfStruct);
    if (registry->maxSizeOfStruct != 0 && newSize > registry->maxSizeOfStruct) {
        return -1;
    }
    RwPluginRegEntry *entry = RwEngineInstance->freeListAlloc(rwPluginRegEntryFreeList);
    if (entry == NULL) {
        return -1;
    }
    entry->offset = registry->sizeOfStruct;
    registry->sizeOfStruct = newSize;
    entry->size = size;
    entry->pluginID = pluginID;
    entry->readCB = NULL;
    entry->writeCB = NULL;
    entry->getSizeCB = NULL;
    entry->constructCB = constructCB != NULL
                             ? constructCB
                             : BMX_FNPTR(RwPluginObjectConstructor, 0x004e8b80, rwPluginDefaultConstructor);
    entry->destructCB = destructCB != NULL
                            ? destructCB
                            : BMX_FNPTR(RwPluginObjectDestructor, 0x004e8b70, rwPluginDefaultDestructor);
    entry->copyCB = copyCB != NULL ? copyCB : BMX_FNPTR(RwPluginObjectCopy, 0x004e8b60, rwPluginDefaultCopy);
    entry->parentRegistry = registry;
    entry->prevRegEntry = NULL;
    entry->nextRegEntry = NULL;
    entry->alwaysCB = NULL;
    if (registry->firstRegEntry == NULL) {
        registry->firstRegEntry = entry;
        registry->lastRegEntry = entry;
    } else {
        registry->lastRegEntry->nextRegEntry = entry;
        entry->prevRegEntry = registry->lastRegEntry;
        registry->lastRegEntry = entry;
    }
    return entry->offset;
}

/* Calls the destructors of all plugins, last registered first. */
BMX_ADDR(0x004e8ab0)
RwPluginRegistry *_rwPluginRegistryDeInitObject(RwPluginRegistry *registry, void *object)
{
    for (RwPluginRegEntry *entry = registry->lastRegEntry; entry != NULL; entry = entry->prevRegEntry) {
        entry->destructCB(object, entry->offset, entry->size);
    }
    return registry;
}

/*
 * Calls the constructors in registration order. When one fails (returns NULL) the destructors of the
 * plugins before it run, in reverse order, and the result is NULL.
 */
BMX_ADDR(0x004e8af0)
RwPluginRegistry *_rwPluginRegistryInitObject(RwPluginRegistry *registry, void *object)
{
    RwPluginRegEntry *entry = registry->firstRegEntry;
    for (;;) {
        if (entry == NULL) {
            return registry;
        }
        if (entry->constructCB(object, entry->offset, entry->size) == NULL) {
            break;
        }
        entry = entry->nextRegEntry;
    }
    for (entry = entry->prevRegEntry; entry != NULL; entry = entry->prevRegEntry) {
        entry->destructCB(object, entry->offset, entry->size);
    }
    return NULL;
}

/* Sets the stream callbacks of an already registered plugin; returns its offset, or -1 if unknown. */
BMX_ADDR(0x004e8b90)
int32_t _rwPluginRegistryAddPluginStream(RwPluginRegistry *registry, int32_t pluginID,
                                         RwPluginDataChunkRead readCB, RwPluginDataChunkWrite writeCB,
                                         RwPluginDataChunkGetSize getSizeCB)
{
    RwPluginRegEntry *entry = registry->firstRegEntry;
    while (entry != NULL && entry->pluginID != pluginID) {
        entry = entry->nextRegEntry;
    }
    if (entry == NULL) {
        return -1;
    }
    entry->readCB = readCB;
    entry->writeCB = writeCB;
    entry->getSizeCB = getSizeCB;
    return entry->offset;
}

/*
 * Reads the extension chunk (type 3) of an object: a version 0x301..0x303 chunk holding plugin chunks.
 * Each plugin chunk goes to its plugin's read callback, or is skipped.
 */
BMX_ADDR(0x004e8bf0)
RwPluginRegistry *_rwPluginRegistryReadDataChunks(RwPluginRegistry *registry, void *stream, void *object)
{
    uint32_t length;
    uint32_t version;
    if (!RwStreamFindChunk(stream, 3, &length, &version)) {
        return NULL;
    }
    if (version < 0x301 || version > 0x303) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(rwERR_BADVERSION);
        RwErrorSet(&error);
        return NULL;
    }
    while (length != 0) {
        uint32_t chunkType;
        uint32_t chunkLength;
        if (!_rwStreamReadChunkHeader(stream, &chunkType, &chunkLength, NULL)) {
            return NULL;
        }
        RwPluginRegEntry *entry = registry->firstRegEntry;
        while (entry != NULL && (uint32_t)entry->pluginID != chunkType) {
            entry = entry->nextRegEntry;
        }
        if (entry != NULL && entry->readCB != NULL) {
            if (entry->readCB(stream, (int32_t)chunkLength, object, entry->offset, entry->size) == NULL) {
                return NULL;
            }
        } else if (RwStreamSkip(stream, chunkLength) == NULL) {
            return NULL;
        }
        length -= chunkLength + 12;
    }
    return registry;
}

/* Writes the extension chunk: every plugin with a size callback and a positive size gets a sub-chunk. */
BMX_ADDR(0x004e8db0)
RwPluginRegistry *_rwPluginRegistryWriteDataChunks(RwPluginRegistry *registry, void *stream,
                                                   const void *object)
{
    int32_t total = 0;
    for (const RwPluginRegEntry *entry = registry->firstRegEntry; entry != NULL;
         entry = entry->nextRegEntry) {
        if (entry->getSizeCB != NULL) {
            const int32_t size = entry->getSizeCB(object, entry->offset, entry->size);
            if (size > 0) {
                total += 12 + size;
            }
        }
    }
    if (!_rwStreamWriteVersionedChunkHeader(stream, 3, total, 0x303)) {
        return NULL;
    }
    for (const RwPluginRegEntry *entry = registry->firstRegEntry; entry != NULL;
         entry = entry->nextRegEntry) {
        if (entry->getSizeCB != NULL && entry->writeCB != NULL) {
            const int32_t size = entry->getSizeCB(object, entry->offset, entry->size);
            if (size > 0) {
                if (!_rwStreamWriteVersionedChunkHeader(stream, (uint32_t)entry->pluginID, size, 0x303)) {
                    return NULL;
                }
                if (entry->writeCB(stream, size, object, entry->offset, entry->size) == NULL) {
                    return NULL;
                }
            }
        }
    }
    return registry;
}

/* Skips the extension chunk of an object without reading it. */
BMX_ADDR(0x004e8ed0)
RwPluginRegistry *_rwPluginRegistrySkipDataChunks(RwPluginRegistry *registry, void *stream)
{
    uint32_t length;
    if (!RwStreamFindChunk(stream, 3, &length, NULL)) {
        return NULL;
    }
    while (length != 0) {
        uint32_t chunkLength;
        if (!_rwStreamReadChunkHeader(stream, NULL, &chunkLength, NULL)) {
            return NULL;
        }
        if (RwStreamSkip(stream, chunkLength) == NULL) {
            return NULL;
        }
        length -= chunkLength + 12;
    }
    return registry;
}

/* ---- strings ---------------------------------------------------------------------------------------- */

/* Fills the engine's string function table with the exe's CRT routines (and a few small helpers). */
BMX_ADDR(0x004ee300)
int _rwStringOpen(void)
{
    RwStringFunctions *strings = &RwEngineInstance->stringFuncs;
    strings->rwsprintf = RW_CB(int(BMX_CDECL *)(char *, const char *, ...), 0x0051dd9c);
    strings->rwvsprintf = RW_CB(void *, 0x004a5338);
    strings->rwstrcpy = RW_CB(char *(BMX_CDECL *)(char *, const char *), 0x004a8ca0);
    strings->rwstrncpy = RW_CB(void *, 0x004a5600);
    strings->rwstrcat = RW_CB(void *, 0x004a8cb0);
    strings->rwstrncat = RW_CB(void *, 0x004a8af0);
    strings->rwstrrchr = RW_CB(void *, 0x004ee2d0);
    strings->rwstrchr = RW_CB(void *, 0x004ee290);
    strings->rwstrstr = RW_CB(void *, 0x004ad060);
    strings->rwstrcmp = RW_CB(void *, 0x0051ddf0);
    strings->rwstricmp = RW_CB(void *, 0x004ee1d0);
    strings->rwstrlen = RW_CB(void *, 0x004a8c20);
    strings->rwstrupr = RW_CB(void *, 0x004ee230);
    strings->rwstrlwr = RW_CB(void *, 0x004ee260);
    strings->rwstrtok = RW_CB(void *, 0x0051de74);
    strings->rwsscanf = RW_CB(void *, 0x004a6391);
    return 1;
}

BMX_ADDR(0x004ee400)
void _rwStringClose(void) {}
