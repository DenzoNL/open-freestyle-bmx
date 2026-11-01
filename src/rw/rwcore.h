/*
 * RenderWare 3.1 core, statically linked into NgBMX.exe: the engine globals, the plugin registry, free
 * lists, error reporting and the device interface (rw/core.c, rw/engine.c).
 *
 * The engine talks to its Direct3D 7 driver through function pointers stored in the engine instance
 * (RwGlobals.dOpenDevice) and in the device struct in .data. Those calls stay indirect: they are the seam
 * where another driver plugs in.
 */
#ifndef RW_RWCORE_H
#define RW_RWCORE_H

#include <stddef.h>
#include <stdint.h>

#include "compat/layout.h"
#include "compat/orig.h"
#include "zge/zmath.h"

typedef ZMatrix RwMatrix;
typedef void(BMX_CDECL *RwMatrixMultiplyFunc)(RwMatrix *out, const RwMatrix *a, const RwMatrix *b);

/*
 * A callback (or table) by its original address, as RenderWare's own code wrote it: the plugin registries and
 * the engine only store it. In the hook build it is that address; in the native exe (BMX_IMAGE) the function
 * serving it, or the address in the relocated data copy for a table. addr may also be a value read from a
 * table of original addresses (the attach routines'), so this is a lookup at run time (BMX_ORIG_PTR).
 */
#define RW_CB(type, addr) ((type)BMX_ORIG_PTR(addr))

/* Intrusive circular list link; a list head is a link that points at itself when empty. */
typedef struct RwLLLink {
    struct RwLLLink *next;
    struct RwLLLink *prev;
} RwLLLink;

/* Error record: plugin id (0 for the core codes with the high bit set) and the error code. */
typedef struct RwError {
    int32_t pluginID;
    int32_t errorCode;
} RwError;

enum {
    rwERR_DEVICE = 0x12, /* followed by the device system request that failed */
    rwERR_NOCAMERA = 0x14,
    rwERR_NULLP = (int32_t)0x80000001,
    rwERR_NOMEM = (int32_t)0x80000002,
    rwERR_INVALIDSIZE = (int32_t)0x80000006, /* "Invalid data block size specified" */
    rwERR_NOTOPEN = (int32_t)0x80000007,
    rwERR_FILEOPEN = (int32_t)0x80000003, /* "Cannot open file" (argument: the name) */
    rwERR_READ = (int32_t)0x80000004,
    rwERR_WRITE = (int32_t)0x80000005,
    rwERR_ALREADYOPEN = (int32_t)0x80000008, /* plugins can only be registered before RwEngineOpen */
    rwERR_BADVERSION = (int32_t)0x80000018,
    rwERRNONE = (int32_t)0x80000000, /* the code of an empty error slot */
};

/* Device system requests (RwSystemFunc's first argument). */
enum {
    rwDEVICESYSTEMOPEN = 0,
    rwDEVICESYSTEMCLOSE = 1,
    rwDEVICESYSTEMSTART = 2,
    rwDEVICESYSTEMSTOP = 3,
    rwDEVICESYSTEMREGISTER = 4,
    rwDEVICESYSTEMGETNUMMODES = 5,
    rwDEVICESYSTEMGETMODEINFO = 6,
    rwDEVICESYSTEMUSEMODE = 7,
    rwDEVICESYSTEMPIPELINE = 9, /* the Im3D pipeline asks the driver to fill in its stages */
    rwDEVICESYSTEMGETMODE = 10,
    rwDEVICESYSTEMSTANDARDS = 11,
    rwDEVICESYSTEMGETNUMSUBSYSTEMS = 13,
    rwDEVICESYSTEMGETSUBSYSTEMINFO = 14,
    rwDEVICESYSTEMGETCURRENTSUBSYSTEM = 15,
    rwDEVICESYSTEMSETSUBSYSTEM = 16,
    rwDEVICESYSTEMFINALIZESTART = 17,
};

enum {
    rwENGINESTATUSIDLE = 0,
    rwENGINESTATUSINITED = 1,
    rwENGINESTATUSOPENED = 2,
    rwENGINESTATUSSTARTED = 3,
    rwSTANDARDNUMOFSTANDARD = 0x1c,
};

typedef int(BMX_CDECL *RwSystemFunc)(int32_t request, void *out, void *inOut, int32_t in);
typedef int(BMX_CDECL *RwRenderStateSetFunc)(int32_t state, int32_t value);
typedef int(BMX_CDECL *RwRenderStateGetFunc)(int32_t state, void *value);
typedef int(BMX_CDECL *RwIm2DRenderPrimitiveFunc)(int32_t primType, void *vertices, int32_t numVertices);
typedef int(BMX_CDECL *RwIm3DRenderLineFunc)(int32_t from, int32_t to);
typedef int(BMX_CDECL *RwIm3DRenderPrimitiveFunc)(int32_t primType);
typedef int(BMX_CDECL *RwIm3DRenderIndexedPrimitiveFunc)(int32_t primType, const uint16_t *indices,
                                                         int32_t numIndices);

/* The device driver's function table (the D3D7 driver's lives in .data at 0x566fe4). */
typedef struct RwDevice {
    float gammaCorrection;                                         /* 0x00 */
    RwSystemFunc fpSystem;                                         /* 0x04 */
    float zBufferNear;                                             /* 0x08 */
    float zBufferFar;                                              /* 0x0c */
    RwRenderStateSetFunc fpRenderStateSet;                         /* 0x10 */
    RwRenderStateGetFunc fpRenderStateGet;                         /* 0x14 */
    void *fpIm2DRenderLine;                                        /* 0x18 */
    void *fpIm2DRenderTriangle;                                    /* 0x1c */
    RwIm2DRenderPrimitiveFunc fpIm2DRenderPrimitive;               /* 0x20 */
    void *fpIm2DRenderIndexedPrimitive;                            /* 0x24 */
    RwIm3DRenderLineFunc fpIm3DRenderLine;                         /* 0x28 */
    void *fpIm3DRenderTriangle;                                    /* 0x2c */
    RwIm3DRenderPrimitiveFunc fpIm3DRenderPrimitive;               /* 0x30 */
    RwIm3DRenderIndexedPrimitiveFunc fpIm3DRenderIndexedPrimitive; /* 0x34 */
} RwDevice;

BMX_LAYOUT32_ASSERT(sizeof(RwDevice) == 0x38, "RwDevice layout");

typedef void *(BMX_CDECL *RwMemMallocFunc)(size_t size);
typedef void(BMX_CDECL *RwMemFreeFunc)(void *block);
typedef void *(BMX_CDECL *RwMemReallocFunc)(void *block, size_t size);
typedef void *(BMX_CDECL *RwMemCallocFunc)(size_t count, size_t size);

/* The memory interface (RwOsGetMemoryInterface returns a pointer to it). */
typedef struct RwMemoryFunctions {
    RwMemMallocFunc rwmalloc;
    RwMemFreeFunc rwfree;
    RwMemReallocFunc rwrealloc;
    RwMemCallocFunc rwcalloc;
} RwMemoryFunctions;

typedef struct RwFreeList RwFreeList;
typedef void *(BMX_CDECL *RwFreeListAllocFunc)(RwFreeList *freeList);
typedef void(BMX_CDECL *RwFreeListFreeFunc)(RwFreeList *freeList, void *entry);

/* The C string functions the engine hands to plugins; _rwStringOpen fills them from the exe's CRT. */
typedef struct RwStringFunctions {
    int(BMX_CDECL *rwsprintf)(char *dst, const char *format, ...);
    void *rwvsprintf;
    char *(BMX_CDECL *rwstrcpy)(char *dst, const char *src);
    void *rwstrncpy;
    void *rwstrcat;
    void *rwstrncat;
    void *rwstrrchr;
    void *rwstrchr;
    void *rwstrstr;
    void *rwstrcmp;
    void *rwstricmp;
    void *rwstrlen;
    void *rwstrupr;
    void *rwstrlwr;
    void *rwstrtok;
    void *rwsscanf;
} RwStringFunctions;

/*
 * The file system interface (RwOSGetFileInterface): the C stdio subset RenderWare streams use, filled in by
 * _rwFSOpen with the CRT's functions. The game replaces entries while a texture is read from memory.
 */
typedef struct RwFileFunctions {
    int(BMX_CDECL *rwfexist)(const char *name);                                          /* 0x00 */
    void *(BMX_CDECL *rwfopen)(const char *name, const char *mode);                      /* 0x04 */
    int(BMX_CDECL *rwfclose)(void *file);                                                /* 0x08 */
    size_t(BMX_CDECL *rwfread)(void *buf, size_t size, size_t count, void *file);        /* 0x0c */
    size_t(BMX_CDECL *rwfwrite)(const void *buf, size_t size, size_t count, void *file); /* 0x10 */
    char *(BMX_CDECL *rwfgets)(char *buf, int32_t size, void *file);                     /* 0x14 */
    int(BMX_CDECL *rwfputs)(const char *str, void *file);                                /* 0x18 */
    int(BMX_CDECL *rwfeof)(void *file);                                                  /* 0x1c */
    int(BMX_CDECL *rwfseek)(void *file, int32_t offset, int32_t origin);                 /* 0x20 */
    int(BMX_CDECL *rwfflush)(void *file);                                                /* 0x24 */
} RwFileFunctions;

BMX_LAYOUT32_ASSERT(sizeof(RwFileFunctions) == 0x28, "RwFileFunctions layout");

/*
 * The engine instance. RwEngineInstance points at it; RwDefaultGlobals is the static one used until
 * RwEngineOpen allocates the real one. Plugins registered on the engine registry keep their data after
 * sizeof(RwGlobals): the error slot, the resource arena, the matrix globals.
 */
typedef struct RwGlobals {
    void *curCamera;                           /* 0x00 */
    void *curWorld;                            /* 0x04 */
    uint16_t renderFrame;                      /* 0x08 */
    uint16_t lightFrame;                       /* 0x0a */
    uint16_t lightVisit;                       /* 0x0c stamp of the light walk (rpLightVisitOnce) */
    uint8_t pad0e[2];                          /* 0x0e */
    RwDevice dOpenDevice;                      /* 0x10 */
    uint32_t stdFunc[rwSTANDARDNUMOFSTANDARD]; /* 0x48 */
    RwLLLink dirtyFrameList;                   /* 0xb8 frame hierarchies waiting for a sync */
    RwFileFunctions fileFuncs;                 /* 0xc0 */
    RwStringFunctions stringFuncs;             /* 0xe8 */
    RwMemoryFunctions memoryFuncs;             /* 0x128 */
    RwFreeListAllocFunc freeListAlloc;         /* 0x138 */
    RwFreeListFreeFunc freeListFree;           /* 0x13c */
    int32_t *metrics;                          /* 0x140 */
    int32_t engineStatus;                      /* 0x144 */
} RwGlobals;

BMX_LAYOUT32_ASSERT(sizeof(RwGlobals) == 0x148, "RwGlobals layout");
BMX_LAYOUT32_ASSERT(offsetof(RwGlobals, dOpenDevice) == 0x10, "RwGlobals layout");
BMX_LAYOUT32_ASSERT(offsetof(RwGlobals, stdFunc) == 0x48, "RwGlobals layout");
BMX_LAYOUT32_ASSERT(offsetof(RwGlobals, fileFuncs) == 0xc0, "RwGlobals layout");
BMX_LAYOUT32_ASSERT(offsetof(RwGlobals, stringFuncs) == 0xe8, "RwGlobals layout");
BMX_LAYOUT32_ASSERT(offsetof(RwGlobals, memoryFuncs) == 0x128, "RwGlobals layout");
BMX_LAYOUT32_ASSERT(offsetof(RwGlobals, freeListAlloc) == 0x138, "RwGlobals layout");
BMX_LAYOUT32_ASSERT(offsetof(RwGlobals, engineStatus) == 0x144, "RwGlobals layout");

#define RwEngineInstance      ORIG_VAR(0x0067696c, RwGlobals *, RwEngineInstance)
#define RwDefaultGlobals      ORIG_VAR(0x00676820, RwGlobals, RwDefaultGlobals)
#define rwEngineInstanceCount ORIG_VAR(0x00676968, int32_t, rwEngineInstanceCount)
#define rwD3D7Device          ORIG_VAR(0x00566fe4, RwDevice, rwD3D7Device)
/* Offsets of plugin data inside the engine instance, set by the plugins' constructors. */
#define rwErrorOffset     ORIG_VAR(0x00676d20, int32_t, rwErrorOffset)
#define rwResourcesOffset ORIG_VAR(0x00676cf0, int32_t, rwResourcesOffset)
#define rwMatrixOffset    ORIG_VAR(0x00676970, int32_t, rwMatrixOffset)
#define rwFastSqrtOffset  ORIG_VAR(0x00676990, int32_t, rwFastSqrtOffset)
/* The stream plugin's data: the free list the RwStream objects come from (rw/stream.c). */
#define rwStreamOffset ORIG_VAR(0x00676d50, int32_t, rwStreamOffset)

/* A block of free list entries: the next block's address, then the entries. */
typedef struct RwFreeListBlock {
    struct RwFreeListBlock *next;
} RwFreeListBlock;

/* Free lists: fixed-size entries carved from blocks, all lists chained on rwFreeListHead. */
struct RwFreeList {
    void **freeBegin;        /* 0x00 stack of free entry pointers */
    void **freeTop;          /* 0x04 its last element (begin - 4 bytes when empty) */
    RwFreeListBlock *blocks; /* 0x08 */
    int32_t blockSize;       /* 0x0c bytes per block */
    int32_t entrySize;       /* 0x10 entry size rounded up to the alignment */
    uint32_t alignmentMask;  /* 0x14 alignment - 1 */
    int32_t entriesPerBlock; /* 0x18 */
    int32_t numBlocks;       /* 0x1c */
    RwLLLink link;           /* 0x20 in the list of all free lists */
};

BMX_LAYOUT32_ASSERT(sizeof(RwFreeList) == 0x28, "RwFreeList layout");

#define rwFreeListHead ORIG_VAR(0x00676ce0, RwLLLink, rwFreeListHead)

typedef void(BMX_CDECL *RwFreeListCallBack)(void *entry, void *data);

RwFreeList *RwFreeListCreate(int32_t entrySize, int32_t entriesPerBlock, int32_t alignment);
int RwFreeListDestroy(RwFreeList *freeList);
RwFreeList *RwFreeListForAllUsed(RwFreeList *freeList, RwFreeListCallBack fn, void *data);

/* Error reporting. RwErrorGet returns the pending error and clears it. */
int32_t _rwerror(int32_t code, ...);
void RwErrorSet(const RwError *error);
void RwErrorGet(RwError *error);

/* Frame / object linkage (the object keeps its place in the frame's object list). */
typedef struct RwObject {
    uint8_t type;
    uint8_t subType;
    uint8_t flags;
    uint8_t privateFlags;
    void *parent;
} RwObject;

/*
 * RwFrame: a node of the frame hierarchy. object.parent is the parent frame; privateFlags bits 0 and 1 (3)
 * mean "in the engine's dirty list", bits 2 and 3 (0xc) mark a changed frame / subtree.
 */
typedef struct RwFrame {
    RwObject object;
    uint8_t version;       /* 0x08 bumped whenever the frame changes */
    uint8_t pad09[3];      /* 0x09 */
    RwLLLink dirtyLink;    /* 0x0c in RwGlobals.dirtyFrameList (only while privateFlags & 3) */
    RwLLLink objectList;   /* 0x14 objects attached to the frame */
    RwMatrix modelling;    /* 0x1c relative to the parent */
    RwMatrix ltm;          /* 0x5c local transformation matrix (world space) */
    struct RwFrame *child; /* 0x9c first child */
    struct RwFrame *next;  /* 0xa0 next sibling */
    struct RwFrame *root;  /* 0xa4 root of the hierarchy */
} RwFrame;

enum {
    rwFRAMEPRIVATEHIERARCHYSYNCLTM = 0x01,
    rwFRAMEPRIVATEHIERARCHYSYNCOBJ = 0x02,
    rwFRAMEPRIVATESUBTREESYNCLTM = 0x04,
    rwFRAMEPRIVATESUBTREESYNCOBJ = 0x08,
};

typedef struct RwObjectHasFrame {
    RwObject object; /* parent is the frame */
    RwLLLink lFrame; /* link in the frame's object list */
    void *sync;
} RwObjectHasFrame;

BMX_LAYOUT32_ASSERT(offsetof(RwFrame, objectList) == 0x14, "RwFrame layout");
BMX_LAYOUT32_ASSERT(offsetof(RwFrame, ltm) == 0x5c, "RwFrame layout");
BMX_LAYOUT32_ASSERT(offsetof(RwFrame, root) == 0xa4, "RwFrame layout");
BMX_LAYOUT32_ASSERT(sizeof(RwFrame) == 0xa8, "RwFrame layout");
BMX_LAYOUT32_ASSERT(offsetof(RwGlobals, dirtyFrameList) == 0xb8, "RwGlobals layout");
BMX_LAYOUT32_ASSERT(sizeof(RwObjectHasFrame) == 0x14, "RwObjectHasFrame layout");

/* Callback of the for-all helpers: gets the object and the caller's data, returns NULL to stop. */
typedef void *(BMX_CDECL *RwObjectCallBack)(void *object, void *data);

void _rwObjectHasFrameSetFrame(RwObjectHasFrame *object, RwFrame *frame);
void _rwObjectHasFrameReleaseFrame(RwObjectHasFrame *object);

/* ---- plugin registry -------------------------------------------------------------------------------- */

typedef void *(BMX_CDECL *RwPluginObjectConstructor)(void *object, int32_t offsetInObject,
                                                     int32_t sizeInObject);
typedef void *(BMX_CDECL *RwPluginObjectDestructor)(void *object, int32_t offsetInObject,
                                                    int32_t sizeInObject);
typedef void *(BMX_CDECL *RwPluginObjectCopy)(void *dst, const void *src, int32_t offsetInObject,
                                              int32_t sizeInObject);
typedef void *(BMX_CDECL *RwPluginDataChunkRead)(void *stream, int32_t length, void *object,
                                                 int32_t offsetInObject, int32_t sizeInObject);
typedef void *(BMX_CDECL *RwPluginDataChunkWrite)(void *stream, int32_t length, const void *object,
                                                  int32_t offsetInObject, int32_t sizeInObject);
typedef int32_t(BMX_CDECL *RwPluginDataChunkGetSize)(const void *object, int32_t offsetInObject,
                                                     int32_t sizeInObject);

#define RW_CTOR(addr)    RW_CB(RwPluginObjectConstructor, addr)
#define RW_DTOR(addr)    RW_CB(RwPluginObjectDestructor, addr)
#define RW_COPY(addr)    RW_CB(RwPluginObjectCopy, addr)
#define RW_READ(addr)    RW_CB(RwPluginDataChunkRead, addr)
#define RW_WRITE(addr)   RW_CB(RwPluginDataChunkWrite, addr)
#define RW_GETSIZE(addr) RW_CB(RwPluginDataChunkGetSize, addr)

typedef struct RwPluginRegEntry RwPluginRegEntry;
struct RwPluginRegEntry {
    int32_t offset;                          /* 0x00 of the plugin's data inside the object */
    int32_t size;                            /* 0x04 */
    int32_t pluginID;                        /* 0x08 */
    RwPluginDataChunkRead readCB;            /* 0x0c */
    RwPluginDataChunkWrite writeCB;          /* 0x10 */
    RwPluginDataChunkGetSize getSizeCB;      /* 0x14 */
    RwPluginObjectConstructor constructCB;   /* 0x18 */
    RwPluginObjectDestructor destructCB;     /* 0x1c */
    RwPluginObjectCopy copyCB;               /* 0x20 */
    void *alwaysCB;                          /* 0x24 cleared, never set */
    RwPluginRegEntry *nextRegEntry;          /* 0x28 */
    RwPluginRegEntry *prevRegEntry;          /* 0x2c */
    struct RwPluginRegistry *parentRegistry; /* 0x30 */
};

/* One per object type: the size of the object grows with every registered plugin. */
typedef struct RwPluginRegistry {
    int32_t sizeOfStruct;            /* 0x00 object size including the plugin data so far */
    int32_t origSizeOfStruct;        /* 0x04 */
    int32_t maxSizeOfStruct;         /* 0x08 0 = no limit */
    RwPluginRegEntry *firstRegEntry; /* 0x0c */
    RwPluginRegEntry *lastRegEntry;  /* 0x10 */
} RwPluginRegistry;

BMX_LAYOUT32_ASSERT(sizeof(RwPluginRegEntry) == 0x34, "RwPluginRegEntry layout");
BMX_LAYOUT32_ASSERT(sizeof(RwPluginRegistry) == 0x14, "RwPluginRegistry layout");

#define rwPluginRegEntryFreeList ORIG_VAR(0x00676d80, RwFreeList *, rwPluginRegEntryFreeList)
#define rwEngineRegistry         ORIG_VAR(0x00566dc0, RwPluginRegistry, rwEngineRegistry)

/*
 * Small routines the engine only reaches through pointers, so they are not functions of their own in the
 * exe's symbol list: the defaults of a plugin without constructor (0x4e8b80), destructor (0x4e8b70) or copy
 * callback (0x4e8b60), and the free list callback of _rwPluginRegistryClose (0x4e8800).
 */
void *rwPluginDefaultConstructor(void *object, int32_t offsetInObject, int32_t sizeInObject);
void *rwPluginDefaultDestructor(void *object, int32_t offsetInObject, int32_t sizeInObject);
void *rwPluginDefaultCopy(void *dst, const void *src, int32_t offsetInObject, int32_t sizeInObject);
void rwPluginRegEntryRelease(void *entry, void *freeList);

int _rwPluginRegistryOpen(void);
int _rwPluginRegistryClose(void);
int32_t _rwPluginRegistryAddPlugin(RwPluginRegistry *registry, int32_t size, int32_t pluginID,
                                   RwPluginObjectConstructor constructCB, RwPluginObjectDestructor destructCB,
                                   RwPluginObjectCopy copyCB);
int32_t _rwPluginRegistryAddPluginStream(RwPluginRegistry *registry, int32_t pluginID,
                                         RwPluginDataChunkRead readCB, RwPluginDataChunkWrite writeCB,
                                         RwPluginDataChunkGetSize getSizeCB);
RwPluginRegistry *_rwPluginRegistryInitObject(RwPluginRegistry *registry, void *object);
RwPluginRegistry *_rwPluginRegistryDeInitObject(RwPluginRegistry *registry, void *object);
RwPluginRegistry *_rwPluginRegistryReadDataChunks(RwPluginRegistry *registry, void *stream, void *object);
RwPluginRegistry *_rwPluginRegistryWriteDataChunks(RwPluginRegistry *registry, void *stream,
                                                   const void *object);
RwPluginRegistry *_rwPluginRegistrySkipDataChunks(RwPluginRegistry *registry, void *stream);

int _rwStringOpen(void);
void _rwStringClose(void);

/* ---- engine (rw/engine.c) --------------------------------------------------------------------------- */

int32_t RwEngineGetNumSubSystems(void);
void *RwEngineGetSubSystemInfo(void *info, int32_t subSystem);
int32_t RwEngineGetCurrentSubSystem(void);
int RwEngineSetSubSystem(int32_t subSystem);
int32_t RwEngineGetNumVideoModes(void);
void *RwEngineGetVideoModeInfo(void *info, int32_t modeIndex);
int32_t RwEngineGetCurrentVideoMode(void);
int RwEngineSetVideoMode(int32_t modeIndex);
int RwEngineStart(void);
int RwEngineOpen(void *openParams);
int RwEngineInit(const RwMemoryFunctions *memoryFunctions, uint32_t initFlags);
int32_t *RwEngineGetMetrics(void);
int32_t _rwGetNumEngineInstances(void);

/* The engine's free list entry points (RwGlobals.freeListAlloc / freeListFree), see rw/core.c and engine.c.
 */
void *_rwFreeListAllocReal(RwFreeList *freeList);
void _rwFreeListFreeReal(RwFreeList *freeList, void *entry);
void *rwFreeListAllocPlain(RwFreeList *freeList);
RwFreeList *rwFreeListFreePlain(RwFreeList *freeList, void *entry);

/* Modules of the engine registry: open(engine, offset of its data) returns the engine, NULL when it failed.
 */
void *_rwErrorOpen(void *engine, int32_t offset);
void *_rwErrorClose(void *engine);
void *_rwColorOpen(void *engine);
void *_rwColorClose(void *engine);
void *_rwMatrixClose(void *engine);
void *_rwMatrixOpen(void *engine, int32_t offset);
void *_rwResourcesOpen(void *engine, int32_t offset);
void *_rwResourcesClose(void *engine);
int _rwDeviceSystemRequest(RwDevice *device, int32_t request, void *out, void *inOut, int32_t in);

/* The default calloc of the memory interface (0x4db9a0): malloc plus a zero fill. */
void *rwDefaultCalloc(size_t count, size_t size);

int _rwMemoryOpen(const RwMemoryFunctions *memoryFunctions);
void _rwMemoryClose(void);
const RwMemoryFunctions *RwOsGetMemoryInterface(void);

/* The engine's resource arena (plugin data of the engine instance, size 0x24). */
typedef struct RwResEntry {
    RwLLLink link;                                            /* 0x00 */
    int32_t size;                                             /* 0x08 */
    void *owner;                                              /* 0x0c */
    struct RwResEntry **ownerRef;                             /* 0x10 cleared when the entry is freed */
    void(BMX_CDECL *destroyNotify)(struct RwResEntry *entry); /* 0x14 */
} RwResEntry;

typedef struct RwResourcesGlobals {
    int32_t maxSize;       /* 0x00 arena size limit */
    int32_t currentSize;   /* 0x04 */
    int32_t reusageSize;   /* 0x08 */
    RwLLLink entriesA;     /* 0x0c */
    RwLLLink entriesB;     /* 0x14 */
    RwLLLink *usedEntries; /* 0x1c */
    RwLLLink *freeEntries; /* 0x20 */
} RwResourcesGlobals;

BMX_LAYOUT32_ASSERT(sizeof(RwResEntry) == 0x18, "RwResEntry layout");
BMX_LAYOUT32_ASSERT(sizeof(RwResourcesGlobals) == 0x24, "RwResourcesGlobals layout");

int RwResourcesFreeResEntry(RwResEntry *entry);
typedef void(BMX_CDECL *RwResEntryDestroyNotify)(RwResEntry *entry);
RwResEntry *RwResourcesAllocateResEntry(void *owner, RwResEntry **ownerRef, int32_t size,
                                        RwResEntryDestroyNotify destroyNotify);
RwResEntry *RwResourcesUseResEntry(RwResEntry *entry);
void ResourcesInit(RwResourcesGlobals *resources);
void _rwResourcesPurge(void);
int RwResourcesSetArenaSize(int32_t size);

/* Matrix helpers of the engine plugin 0x401 (the rest of RwMatrix is in rw/math.c). */
RwMatrix *RwMatrixUpdate(RwMatrix *matrix);
RwMatrix *RwMatrixMultiply(RwMatrix *out, const RwMatrix *a, const RwMatrix *b);

float RwIm2DGetNearScreenZ(void);
int RwRenderStateSet(int32_t state, int32_t value);
int RwRenderStateGet(int32_t state, void *value);
int RwIm2DRenderPrimitive(int32_t primType, void *vertices, int32_t numVertices);

/* Pointer to the D3D7 driver's device table, and the two vtable-style module attach calls. */
RwDevice *_rwDeviceGetHandle(void);
void _rwFSClose(void);
void **rwCoreExtGetFuncs(void);
int rwCoreExtPluginAttach(void);

#endif
