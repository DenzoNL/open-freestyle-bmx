/*
 * Model loading and instancing (C:\NgEngine\Zge\Com\ZglRw\zrwModelCreate.c): reads a MODL chunk (definition,
 * materials, up to four levels of detail and a collision clump) into a ZrwModel, clones models and frees
 * them. By address the file also holds the two decompressors (0x49bd10, 0x49c080) the asset reader uses.
 */
#ifndef ZGE_COM_ZGLRW_ZRWMODELCREATE_H
#define ZGE_COM_ZGLRW_ZRWMODELCREATE_H

#include <stddef.h>
#include <stdint.h>

#include "compat/orig.h"
#include "zge/com/zglrw/zrwmodel.h"
#include "zge/zmath.h"
#include "zge/zmem.h"

/* ---- decompression ------------------------------------------------------------------------------ */

/*
 * Method 1 (hash table based, byte 0 == 1 means stored): `work` is 0x4010 bytes of scratch. Writes the
 * length of the output to *outLength.
 */
void zassetDecompressHashed(void *work, const uint8_t *src, int srcLength, uint8_t *dst, int *outLength);
/* zassetDecompress (zasset.h) is method 1 (hashed) or 2 (LZO1X); it returns 0 on success, 1 on a bad method
 * or stream. */

/* ---- frames and clumps -------------------------------------------------------------------------- */

/* State of the model loader (0x675218, 0x24 bytes, cleared at initialisation). */
typedef struct ZrwModelCreateState {
    int initialized;                             /* 0x00 */
    ZMemFreeList *pool;                          /* 0x04 model records (ZrwModel) */
    uint32_t pad08;                              /* 0x08 */
    ZrwModelPart *cursor;                        /* 0x0c next free part while a hierarchy is built */
    struct RpClump *lodClumps[kZrwModelMaxLods]; /* 0x10 the clump of every level of detail loaded so far */
    ZrwModel *last;                              /* 0x20 the model the last MODL chunk produced */
} ZrwModelCreateState;

BMX_LAYOUT32_ASSERT(offsetof(ZrwModelCreateState, cursor) == 0x0c, "model create state");
BMX_LAYOUT32_ASSERT(offsetof(ZrwModelCreateState, lodClumps) == 0x10, "model create state");
BMX_LAYOUT32_ASSERT(offsetof(ZrwModelCreateState, last) == 0x20, "model create state");
BMX_LAYOUT32_ASSERT(sizeof(ZrwModelCreateState) == 0x24, "model create state");

#define gZrwModelCreate ORIG_VAR(0x00675218, ZrwModelCreateState, gZrwModelCreate)

/* RwFrameForAllChildren callbacks that move a frame's 32 bit user data into / out of its matrix (at.x, at.y).
 */
void *zrwFrameUserDataToMatrix(void *frame, void *arg);
void *zrwFrameMatrixToUserData(void *frame, void *arg);
/* RpClumpForAllAtomics callback: stores the first object and stops (returns NULL). */
void *zrwFrameCollectFirst(void *object, void *out);
/* First atomic of a clump (NULL when there is none). */
void *zrwModelCreateFirstAtomic(void *clump);
/* Adds a part for the frame under `parentNode` (a ZrwModelPart, NULL for the root) at the cursor, then
 * recurses into the frame's children. */
void *zrwFrameBuildPartNode(void *frame, void *parentNode);

/* Hands the model to the clumps and links every material to the texture of the clump's matching RpMaterial.
 */
void zrwModelSetRwInfo(ZrwModel *model);
/* RpClumpForAllAtomics callback: adds the triangles and vertices of the atomic's geometry to counts[0..1]. */
void *zrwAtomicCountGeometry(void *atomic, int *counts);
/* Prints the atomic count, triangle and vertex counts and geometry flags of a clump. */
void zrwClumpDump(void *clump);
/* Prints a frame and its children with their names from the model's entry table. */
void *zrwFrameDumpHierarchy(void *frame, void *model);
void zrwModelDumpHierarchy(const ZrwModel *model);
/* Prints the clump, hierarchy and materials of a model. */
void zrwModelDumpInfo(const ZrwModel *model);
void zrwModelPrintUsage(void);

/* Clones a model (a shared definition, its clumps cloned); a skinned model is loaded again from its file. */
ZrwModel *zrwModelCreateInstance(const ZrwModel *model);
/* Reads a clump from memory (an RwMemory: data pointer and size, freed afterwards) into the model: the
 * collision clump when `collision` is 1, else the next level of detail. */
int zrwModelAddClump(ZrwModel *model, const uint32_t *memory, const char *imagePath, int collision);

/* IFF chunk handlers: the MLOD (level of detail), MCOL (collision) and MDEF (definition) chunks. */
int zrwModelLoadLod(void *iff, ZrwModel *model);
int zrwModelLoadCollision(void *iff, ZrwModel *model);
int zrwModelReadDef(void *iff, ZrwModel *model);
/* Renderer material callback that collects a loaded material into the model. */
int zrwModelAddMaterial(void *material, ZrwModel *model);
int zrwModelLoadMaterials(void *iff, ZrwModel *model);
/* MODL chunk handler. Its tail is also a function of its own in the data file. */
int zrwModelLoad(void *iff);
int zrwModelLoadTail(void *iff, ZrwModel *model);

int zrwModelCreateInitialize(void);
int zrwModelCreateShutdown(void);
/* Reads the chunk the reader is on and returns the model it created. */
ZrwModel *zrwModelLoadFromChunk(void *iff);
/* Loads a MODL block held in memory / in a file. */
ZrwModel *zrwModelLoadFromBuffer(const uint32_t *buffer);
ZrwModel *zrwModelLoadFromFile(const char *path);
/* Placeholder that asserts "Not coded" and returns 0. */
int zrwModelNotCoded(void);
int zrwModelDestroy(ZrwModel *model);

#endif
