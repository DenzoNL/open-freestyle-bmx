/* RenderWare 3.1 engine: init/open/start, device queries, memory interface, resource arena, small helpers. */
#include "rw/rwcore.h"

#include "compat/crt.h"
#include "rw/rwmath.h"
#include "rw/rwplugins.h"
#include "rw/rwstream.h"
#include "rw/rwtexture.h"

/* The matrix module's data (RwMatrixModule, rwmath.h). */
static RwMatrixModule *rwMatrixGlobals(void)
{
    return (RwMatrixModule *)((uint8_t *)RwEngineInstance + rwMatrixOffset);
}

static RwResourcesGlobals *rwResources(void)
{
    return (RwResourcesGlobals *)((uint8_t *)RwEngineInstance + rwResourcesOffset);
}

/* Reports a failed device system request through the error slot. */
static void rwDeviceError(int32_t request)
{
    RwError error;
    error.pluginID = 1;
    error.errorCode = _rwerror(rwERR_DEVICE, request);
    RwErrorSet(&error);
}

/* ---- device queries --------------------------------------------------------------------------------- */

/* Number of rendering sub systems (graphics adapters); 1 if the device does not say. */
BMX_ADDR(0x004cf870)
int32_t RwEngineGetNumSubSystems(void)
{
    int32_t count = 1;
    if (!RwEngineInstance->dOpenDevice.fpSystem(rwDEVICESYSTEMGETNUMSUBSYSTEMS, &count, NULL, 0)) {
        count = 1;
    }
    return count;
}

/* Name of a sub system; without a device answer, "Only rendering sub system" for index 0. */
BMX_ADDR(0x004cf8b0)
void *RwEngineGetSubSystemInfo(void *info, int32_t subSystem)
{
    if (!RwEngineInstance->dOpenDevice.fpSystem(rwDEVICESYSTEMGETSUBSYSTEMINFO, info, NULL, subSystem)) {
        if (subSystem != 0) {
            rwDeviceError(rwDEVICESYSTEMGETSUBSYSTEMINFO);
            return NULL;
        }
        RwEngineInstance->stringFuncs.rwstrcpy(info, "Only rendering sub system");
    }
    return info;
}

BMX_ADDR(0x004cf940)
int32_t RwEngineGetCurrentSubSystem(void)
{
    int32_t current;
    if (!RwEngineInstance->dOpenDevice.fpSystem(rwDEVICESYSTEMGETCURRENTSUBSYSTEM, &current, NULL, 0)) {
        current = 0;
    }
    return current;
}

BMX_ADDR(0x004cf980)
int RwEngineSetSubSystem(int32_t subSystem)
{
    if (!RwEngineInstance->dOpenDevice.fpSystem(rwDEVICESYSTEMSETSUBSYSTEM, NULL, NULL, subSystem) &&
        subSystem != 0) {
        rwDeviceError(rwDEVICESYSTEMSETSUBSYSTEM);
        return 0;
    }
    return 1;
}

BMX_ADDR(0x004cf9f0)
int32_t RwEngineGetNumVideoModes(void)
{
    int32_t count;
    if (!RwEngineInstance->dOpenDevice.fpSystem(rwDEVICESYSTEMGETNUMMODES, &count, NULL, 0)) {
        rwDeviceError(rwDEVICESYSTEMGETNUMMODES);
        return -1;
    }
    return count;
}

BMX_ADDR(0x004cfa60)
void *RwEngineGetVideoModeInfo(void *info, int32_t modeIndex)
{
    if (!RwEngineInstance->dOpenDevice.fpSystem(rwDEVICESYSTEMGETMODEINFO, info, NULL, modeIndex)) {
        rwDeviceError(rwDEVICESYSTEMGETMODEINFO);
        return NULL;
    }
    return info;
}

BMX_ADDR(0x004cfad0)
int32_t RwEngineGetCurrentVideoMode(void)
{
    int32_t mode;
    if (!RwEngineInstance->dOpenDevice.fpSystem(rwDEVICESYSTEMGETMODE, &mode, NULL, 0)) {
        rwDeviceError(rwDEVICESYSTEMGETMODE);
        return -1;
    }
    return mode;
}

BMX_ADDR(0x004cfb40)
int RwEngineSetVideoMode(int32_t modeIndex)
{
    if (!RwEngineInstance->dOpenDevice.fpSystem(rwDEVICESYSTEMUSEMODE, NULL, NULL, modeIndex)) {
        rwDeviceError(rwDEVICESYSTEMUSEMODE);
        return 0;
    }
    return 1;
}

/* ---- engine life cycle ------------------------------------------------------------------------------ */

/*
 * Starts the device, initialises the engine object's plugin data and applies the device's gamma. Stops
 * the device again when a plugin constructor fails.
 */
BMX_ADDR(0x004cfd50)
int RwEngineStart(void)
{
    if (!RwEngineInstance->dOpenDevice.fpSystem(rwDEVICESYSTEMSTART, NULL, NULL, 0)) {
        rwDeviceError(rwDEVICESYSTEMSTART);
        return 0;
    }
    if (_rwPluginRegistryInitObject(&rwEngineRegistry, RwEngineInstance) != NULL) {
        RwImageSetGamma(RwEngineInstance->dOpenDevice.gammaCorrection);
        RwEngineInstance->dOpenDevice.fpSystem(rwDEVICESYSTEMFINALIZESTART, NULL, NULL, 0);
        RwEngineInstance->engineStatus = rwENGINESTATUSSTARTED;
        return 1;
    }
    if (!RwEngineInstance->dOpenDevice.fpSystem(rwDEVICESYSTEMSTOP, NULL, NULL, 0)) {
        rwDeviceError(rwDEVICESYSTEMSTOP);
    }
    return 0;
}

/*
 * Register-argument function (EAX = device, EDX = open parameters). Allocates the engine instance (the
 * size grew with every engine plugin), copies the static defaults into it, registers the device and opens
 * it. A failed registration or standards query is reported but not fatal; a failed open restores the
 * static instance and frees the new one.
 */
BMX_ADDR(0x004cfe60)
BMX_REGPARM(2) int EngineOpen(const RwDevice *device, void *openParams)
{
    RwGlobals *instance = RwEngineInstance->memoryFuncs.rwmalloc((size_t)rwEngineRegistry.sizeOfStruct);
    RwEngineInstance = instance;
    if (instance == NULL) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(rwERR_NOMEM, rwEngineRegistry.sizeOfStruct);
        RwErrorSet(&error);
        return 0;
    }
    *instance = RwDefaultGlobals;
    /*
     * Port fix (docs/PORTING.md X3): the original leaves the plugin part (the modules' data) as malloc
     * returned it, and the image module never sets its name buffer fields (0x210 / 0x214), so it relies on
     * that memory being zero. With a reused heap block the buffer size is garbage, no name buffer is ever
     * made and every image lookup fails (the shadow texture at start-up). Zeroed here.
     */
    if ((size_t)rwEngineRegistry.sizeOfStruct > sizeof *instance) {
        memset((uint8_t *)instance + sizeof *instance, 0,
               (size_t)rwEngineRegistry.sizeOfStruct - sizeof *instance);
    }
    if (!device->fpSystem(rwDEVICESYSTEMREGISTER, &RwEngineInstance->dOpenDevice,
                          &RwEngineInstance->memoryFuncs, 0)) {
        rwDeviceError(rwDEVICESYSTEMREGISTER);
    }
    if (!device->fpSystem(rwDEVICESYSTEMOPEN, NULL, openParams, 0)) {
        rwDeviceError(rwDEVICESYSTEMOPEN);
        RwEngineInstance = &RwDefaultGlobals;
        RwDefaultGlobals = *instance;
        RwEngineInstance->memoryFuncs.rwfree(instance);
        return 0;
    }
    if (!device->fpSystem(rwDEVICESYSTEMSTANDARDS, (void *)RwEngineInstance->stdFunc, NULL,
                          rwSTANDARDNUMOFSTANDARD)) {
        rwDeviceError(rwDEVICESYSTEMSTANDARDS);
    }
    rwEngineInstanceCount++;
    return 1;
}

/* The D3D7 driver's device table. */
BMX_ADDR(0x004ee140)
RwDevice *_rwDeviceGetHandle(void)
{
    return &rwD3D7Device;
}

/* Opens the engine on the device; openParams is the driver's (the window handle for D3D7). */
BMX_ADDR(0x004d0020)
int RwEngineOpen(void *openParams)
{
    if (RwEngineInstance == NULL) {
        RwEngineInstance = &RwDefaultGlobals;
    }
    RwDevice *device = _rwDeviceGetHandle();
    if (device == NULL) {
        return 0;
    }
    const int opened = EngineOpen(device, openParams);
    if (opened != 0) {
        RwEngineInstance->engineStatus = rwENGINESTATUSOPENED;
    }
    return opened;
}

BMX_ADDR(0x004d0120)
int32_t *RwEngineGetMetrics(void)
{
    return RwEngineInstance->metrics;
}

BMX_ADDR(0x004d0130)
int32_t _rwGetNumEngineInstances(void)
{
    return rwEngineInstanceCount;
}

/* Counterpart of _rwFSOpen; does nothing. */
BMX_ADDR(0x004e8380)
void _rwFSClose(void) {}

/* The first entry of the table at 0x69bee0 is the attach routine of the core plugin it belongs to. */
BMX_ADDR(0x004eb110)
void **rwCoreExtGetFuncs(void)
{
    return RW_CB(void **, 0x0069bee0);
}

BMX_ADDR(0x004e86f0)
int rwCoreExtPluginAttach(void)
{
    return ((int(BMX_CDECL *)(void))rwCoreExtGetFuncs()[0])();
}

/*
 * Initialises RenderWare before anything else: memory interface (the CRT's unless the caller passes its
 * own), string functions, file system, plugin registry and the core plugins. initFlags bit 0 selects the
 * plain free list entry points.
 */
BMX_ADDR(0x004d01f0)
int RwEngineInit(const RwMemoryFunctions *memoryFunctions, uint32_t initFlags)
{
    RwEngineInstance = &RwDefaultGlobals;
    if ((initFlags & 1) == 0) {
        RwEngineInstance->freeListAlloc = RW_CB(RwFreeListAllocFunc, 0x004dbc30);
        RwEngineInstance->freeListFree = RW_CB(RwFreeListFreeFunc, 0x004dbd90);
    } else {
        RwEngineInstance->freeListAlloc = RW_CB(RwFreeListAllocFunc, 0x004cf800);
        RwEngineInstance->freeListFree = RW_CB(RwFreeListFreeFunc, 0x004cf820);
    }
    int result = _rwStringOpen();
    if (result != 0) {
        result = _rwMemoryOpen(memoryFunctions);
        if (result != 0) {
            result = _rwFSOpen();
            if (result != 0) {
                result = _rwPluginRegistryOpen();
                if (result != 0) {
                    result = CorePluginAttach();
                    if (result != 0) {
                        RwEngineInstance->engineStatus = rwENGINESTATUSINITED;
                        return result;
                    }
                    _rwPluginRegistryClose();
                }
                _rwFSClose();
            }
            _rwMemoryClose();
        }
        _rwStringClose();
    }
    return result;
}

/* ---- memory interface ------------------------------------------------------------------------------- */

/* calloc of the default memory interface: the exe's malloc plus a zero fill. */
BMX_ADDR(0x004db9a0)
void *rwDefaultCalloc(size_t count, size_t size)
{
    const size_t bytes = count * size;
    uint8_t *block = malloc(bytes);
    if (block != NULL) {
        for (size_t i = 0; i < bytes; i++) {
            block[i] = 0;
        }
    }
    return block;
}

/*
 * Resets the free list chain and fills the engine's memory interface: from the caller's, or the exe's
 * malloc/free/realloc and a calloc built on malloc. Always 1.
 */
BMX_ADDR(0x004db9e0)
int _rwMemoryOpen(const RwMemoryFunctions *memoryFunctions)
{
    rwFreeListHead.next = &rwFreeListHead;
    rwFreeListHead.prev = &rwFreeListHead;
    RwMemoryFunctions *memory = &RwEngineInstance->memoryFuncs;
    if (memoryFunctions == NULL) {
        memory->rwmalloc = (RwMemMallocFunc)malloc;
        memory->rwfree = (RwMemFreeFunc)free;
        memory->rwrealloc = RW_CB(RwMemReallocFunc, 0x0051d67e);
        memory->rwcalloc = BMX_FNPTR(RwMemCallocFunc, 0x004db9a0, rwDefaultCalloc);
    } else {
        memory->rwmalloc = memoryFunctions->rwmalloc;
        memory->rwfree = memoryFunctions->rwfree;
        memory->rwrealloc = memoryFunctions->rwrealloc;
        memory->rwcalloc = memoryFunctions->rwcalloc;
    }
    return 1;
}

BMX_ADDR(0x004dba70)
void _rwMemoryClose(void) {}

BMX_ADDR(0x004dba80)
const RwMemoryFunctions *RwOsGetMemoryInterface(void)
{
    return &RwEngineInstance->memoryFuncs;
}

/* ---- resource arena --------------------------------------------------------------------------------- */

/* Unlinks a resource entry, calls its destroy notification and gives the memory back; always 1. */
BMX_ADDR(0x004dc380)
int RwResourcesFreeResEntry(RwResEntry *entry)
{
    if (entry->destroyNotify != NULL) {
        entry->destroyNotify(entry);
    }
    entry->link.prev->next = entry->link.next;
    entry->link.next->prev = entry->link.prev;
    *entry->ownerRef = NULL;
    rwResources()->currentSize -= entry->size;
    RwEngineInstance->memoryFuncs.rwfree(entry);
    return 1;
}

/* Moves the older entry list onto the newer one, swaps the two lists and resets the reuse counter. */
BMX_ADDR(0x004dc3f0)
void _rwResourcesPurge(void)
{
    RwLLLink *first = rwResources()->usedEntries;
    RwLLLink *second = rwResources()->freeEntries;
    RwLLLink *firstNext = first->next;
    if (firstNext != first) {
        if (second == second->next) {
            second->next = firstNext;
            firstNext->prev = second;
            second->prev = first->prev;
            first->prev->next = second;
        } else {
            RwLLLink *secondLast = second->prev;
            secondLast->next = firstNext;
            firstNext->prev = secondLast;
            first->prev->next = second;
            second->prev = first->prev;
        }
        first->next = first;
        first->prev = first;
    }
    rwResources()->freeEntries = first;
    rwResources()->usedEntries = second;
    rwResources()->reusageSize = 0;
}

/*
 * Sets the arena limit and frees entries, newest first from the first list, then the second, until the
 * arena fits. Returns 0 when both lists run empty before it does.
 */
BMX_ADDR(0x004dc6e0)
int RwResourcesSetArenaSize(int32_t size)
{
    rwResources()->maxSize = size;
    while (rwResources()->maxSize < rwResources()->currentSize) {
        RwLLLink *link = rwResources()->usedEntries->prev;
        if (link == rwResources()->usedEntries) {
            link = rwResources()->freeEntries->prev;
            if (link == rwResources()->freeEntries) {
                return 0;
            }
        }
        RwResourcesFreeResEntry((RwResEntry *)link);
    }
    return 1;
}

enum { rwERR_RESOURCEARENAFULL = 0x18 };

/* Links the entry at the front of the arena's second list (the entries in use this frame). */
static void resLinkFirst(RwResEntry *entry)
{
    RwLLLink *head = rwResources()->freeEntries;
    entry->link.next = head->next;
    entry->link.prev = head;
    head->next->prev = &entry->link;
    head->next = &entry->link;
}

/*
 * A resource entry with `size` bytes after its 0x18 byte header for `owner` (*ownerRef points to it, the
 * destroy notification is called when it is freed), at the front of the second list. While the arena would
 * overflow, the oldest entries are freed: those of the first list, then those of the second (their sizes
 * count as reused). A failed allocation sets an error and frees the next entry. With both lists empty:
 * *ownerRef = NULL, error, NULL.
 */
BMX_ADDR(0x004dc460)
RwResEntry *RwResourcesAllocateResEntry(void *owner, RwResEntry **ownerRef, int32_t size,
                                        RwResEntryDestroyNotify destroyNotify)
{
    for (;;) {
        if (size + rwResources()->currentSize <= rwResources()->maxSize) {
            RwResEntry *entry = RwEngineInstance->memoryFuncs.rwmalloc((size_t)size + sizeof(RwResEntry));
            if (entry != NULL) {
                resLinkFirst(entry);
                entry->destroyNotify = destroyNotify;
                entry->ownerRef = ownerRef;
                entry->size = size;
                entry->owner = owner;
                rwResources()->currentSize += size;
                *ownerRef = entry;
                return entry;
            }
            RwError error;
            error.pluginID = 1;
            error.errorCode = _rwerror(rwERR_NOMEM, size + (int32_t)sizeof(RwResEntry));
            RwErrorSet(&error);
        }
        RwLLLink *link = rwResources()->usedEntries->prev;
        if (link == rwResources()->usedEntries) {
            link = rwResources()->freeEntries->prev;
            if (link == rwResources()->freeEntries) {
                *ownerRef = NULL;
                RwError error;
                error.pluginID = 1;
                error.errorCode = _rwerror(rwERR_RESOURCEARENAFULL, size);
                RwErrorSet(&error);
                return NULL;
            }
            rwResources()->reusageSize += ((RwResEntry *)link)->size;
        }
        RwResourcesFreeResEntry((RwResEntry *)link);
    }
}

/* Marks the entry as used: moves it to the front of the second list. */
BMX_ADDR(0x004dc680)
RwResEntry *RwResourcesUseResEntry(RwResEntry *entry)
{
    entry->link.prev->next = entry->link.next;
    entry->link.next->prev = entry->link.prev;
    resLinkFirst(entry);
    return entry;
}

/* ---- render state and matrix helpers ---------------------------------------------------------------- */

/* The near z of the 2D immediate mode, as the device reports it (the value is returned on the x87 stack). */
BMX_ADDR(0x004db800)
float RwIm2DGetNearScreenZ(void)
{
    return RwEngineInstance->dOpenDevice.zBufferNear;
}

/* Sets a render state through the device; needs a current camera, else reports and returns 0. */
BMX_ADDR(0x004db820)
int RwRenderStateSet(int32_t state, intptr_t value)
{
    if (RwEngineInstance->curCamera == NULL) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(rwERR_NOCAMERA);
        RwErrorSet(&error);
        return 0;
    }
    return RwEngineInstance->dOpenDevice.fpRenderStateSet(state, value);
}

BMX_ADDR(0x004db920)
int RwIm2DRenderPrimitive(int32_t primType, void *vertices, int32_t numVertices)
{
    return RwEngineInstance->dOpenDevice.fpIm2DRenderPrimitive(primType, vertices, numVertices);
}

/* Marks a matrix as changed: bumps its version byte and drops the type and identity flags. */
BMX_ADDR(0x004d04d0)
RwMatrix *RwMatrixUpdate(RwMatrix *matrix)
{
    ((uint8_t *)&matrix->pad1)[0]++;
    matrix->flags &= 0xfffdfffc;
    return matrix;
}

/*
 * out = a * b through the engine's multiply routine. An identity operand (flag bit) short-cuts to a copy
 * of the other one. The type flags survive only when both operands have a type.
 */
BMX_ADDR(0x004d04f0)
RwMatrix *RwMatrixMultiply(RwMatrix *out, const RwMatrix *a, const RwMatrix *b)
{
    const uint32_t identity = rwMatrixGlobals()->flagMask & 0x20000;
    if ((a->flags & identity) != 0) {
        *out = *b;
    } else if ((b->flags & identity) != 0) {
        *out = *a;
    } else {
        rwMatrixGlobals()->multiply(out, a, b);
        out->flags &= 0xfffdffff;
        if ((a->flags & 3) == 0 || (b->flags & 3) == 0) {
            out->flags &= 0xfffffffc;
        }
    }
    ((uint8_t *)&out->pad1)[0]++;
    return out;
}

/* ---- free list entry points ------------------------------------------------------------------------- */

/* The plain free list allocator (initFlags bit 0 of RwEngineInit): every entry comes from the memory
 * interface. */
BMX_ADDR(0x004cf800)
void *rwFreeListAllocPlain(RwFreeList *freeList)
{
    return RwEngineInstance->memoryFuncs.rwmalloc((size_t)freeList->entrySize);
}

/* Counterpart of rwFreeListAllocPlain; returns the free list. */
BMX_ADDR(0x004cf820)
RwFreeList *rwFreeListFreePlain(RwFreeList *freeList, void *entry)
{
    RwEngineInstance->memoryFuncs.rwfree(entry);
    return freeList;
}

/* The engine's default free list release: pushes the entry onto the free stack. */
BMX_ADDR(0x004dbd90)
void _rwFreeListFreeReal(RwFreeList *freeList, void *entry)
{
    void **top = freeList->freeTop + 1;
    freeList->freeTop = top;
    *top = entry;
}

/* ---- modules of the engine registry (opened by RwEngineOpen, in plugin order) ----------------------------
 */

#define rwErrorModuleCount     ORIG_VAR(0x00676d24, int32_t, rwErrorModuleCount)
#define rwMatrixModuleCount    ORIG_VAR(0x00676974, int32_t, rwMatrixModuleCount)
#define rwColorModuleCount     ORIG_VAR(0x00676da4, int32_t, rwColorModuleCount)
#define rwResourcesModuleCount ORIG_VAR(0x00676cf4, int32_t, rwResourcesModuleCount)

/* Error module: an empty error slot (plugin id 0, the "no error" code). Returns the engine. */
BMX_ADDR(0x004dee30)
void *_rwErrorOpen(void *engine, int32_t offset)
{
    rwErrorOffset = offset;
    RwError *slot = (RwError *)((uint8_t *)RwEngineInstance + offset);
    slot->pluginID = 0;
    rwErrorModuleCount++;
    slot->errorCode = rwERRNONE;
    return engine;
}

BMX_ADDR(0x004dee20)
void *_rwErrorClose(void *engine)
{
    rwErrorModuleCount--;
    return engine;
}

/* Colour module: nothing to set up. */
BMX_ADDR(0x004eb2a0)
void *_rwColorOpen(void *engine)
{
    rwColorModuleCount++;
    return engine;
}

BMX_ADDR(0x004eb2b0)
void *_rwColorClose(void *engine)
{
    rwColorModuleCount--;
    return engine;
}

/* Matrix module: the free list of matrices, the identity flag mask and the default product. NULL on no
 * memory. */
BMX_ADDR(0x004d2080)
void *_rwMatrixOpen(void *engine, int32_t offset)
{
    rwMatrixOffset = offset;
    RwMatrixModule *globals = (RwMatrixModule *)((uint8_t *)RwEngineInstance + offset);
    globals->freeList = RwFreeListCreate(sizeof(RwMatrix), 5, 4);
    if (globals->freeList == NULL) {
        return NULL;
    }
    globals->flagMask = 0x20000;
    rwMatrixModuleCount++;
    globals->multiply = BMX_FNPTR(RwMatrixMultiplyFunc, 0x004d02c0, rwMatrixMultiplyDefault);
    return engine;
}

/* Destroys the matrix free list (if there is one) and counts the module down. */
BMX_ADDR(0x004d2040)
void *_rwMatrixClose(void *engine)
{
    if (rwMatrixGlobals()->freeList != NULL) {
        RwFreeListDestroy(rwMatrixGlobals()->freeList);
        rwMatrixGlobals()->freeList = NULL;
    }
    rwMatrixModuleCount--;
    return engine;
}

/* Resource arena: empty lists, no entries, 8 MB limit. */
BMX_ADDR(0x004dc990)
void ResourcesInit(RwResourcesGlobals *resources)
{
    resources->reusageSize = 0;
    resources->currentSize = 0;
    resources->entriesA.next = &resources->entriesA;
    resources->entriesA.prev = &resources->entriesA;
    resources->entriesB.next = &resources->entriesB;
    resources->entriesB.prev = &resources->entriesB;
    resources->maxSize = 0x800000;
    resources->freeEntries = &resources->entriesA;
    resources->usedEntries = &resources->entriesB;
}

BMX_ADDR(0x004dc210)
void *_rwResourcesOpen(void *engine, int32_t offset)
{
    rwResourcesOffset = offset;
    ResourcesInit((RwResourcesGlobals *)((uint8_t *)RwEngineInstance + offset));
    rwResourcesModuleCount++;
    return engine;
}

/* Frees every entry of both lists (calling the destroy notifications) and resets the arena lists. */
BMX_ADDR(0x004dc250)
void *_rwResourcesClose(void *engine)
{
    RwResourcesGlobals *r = rwResources();
    r->entriesA.prev->next = r->entriesB.next;
    RwLLLink *end = &r->entriesB;
    RwResEntry *entry = (RwResEntry *)r->entriesA.next;
    while ((RwLLLink *)entry != end) {
        RwResEntry *next = (RwResEntry *)entry->link.next;
        if (entry->destroyNotify != NULL) {
            entry->destroyNotify(entry);
        }
        entry->link.prev->next = entry->link.next;
        entry->link.next->prev = entry->link.prev;
        *entry->ownerRef = NULL;
        rwResources()->currentSize -= entry->size;
        RwEngineInstance->memoryFuncs.rwfree(entry);
        entry = next;
    }
    r->entriesA.next = &r->entriesA;
    r->entriesA.prev = &r->entriesA;
    r->entriesB.next = &r->entriesB;
    r->entriesB.prev = &r->entriesB;
    r->reusageSize = 0;
    rwResourcesModuleCount--;
    return engine;
}

/* ---- device system requests ------------------------------------------------------------------------- */

/*
 * Sends a request to the device's system function. Requests a device does not know are answered here
 * (sub system count 1, "Only rendering sub system" as the sub system name, no sub system switching); any
 * other unanswered request is reported as an error. Returns 1 when it was handled.
 */
BMX_ADDR(0x004cf3b0)
int _rwDeviceSystemRequest(RwDevice *device, int32_t request, void *out, void *inOut, int32_t in)
{
    if (device->fpSystem(request, out, inOut, in) != 0 || request == rwDEVICESYSTEMFINALIZESTART ||
        request == 0x12) {
        return 1;
    }
    if (request == rwDEVICESYSTEMGETNUMSUBSYSTEMS) {
        *(int32_t *)out = 1;
        return 1;
    }
    if (request == rwDEVICESYSTEMGETSUBSYSTEMINFO) {
        if (in == 0) {
            RwEngineInstance->stringFuncs.rwstrcpy((char *)out, RW_CB(const char *, 0x0069b740));
            return 1;
        }
    } else {
        if (request == rwDEVICESYSTEMGETCURRENTSUBSYSTEM) {
            *(int32_t *)out = 0;
            return 1;
        }
        if (request == rwDEVICESYSTEMSETSUBSYSTEM && in == 0) {
            return 1;
        }
    }
    RwError error;
    error.pluginID = 1;
    error.errorCode = _rwerror(rwERR_DEVICE, request);
    RwErrorSet(&error);
    return 0;
}
