/* Renderer models and materials (C:\NgEngine\Zge\Com\ZglRw\zrwmat.c). Not ported yet; callees only. */
#ifndef ZGE_COM_ZGLRW_ZRWMAT_H
#define ZGE_COM_ZGLRW_ZRWMAT_H

#include <stdint.h>

#include "compat/orig.h"

/* Wrappers over the renderer's model table: load from a chunk, release, and look up. */
ORIG_FN(0x00488fb0, void *, zrwmatCreateObject, (void *model));
ORIG_FN(0x00489070, void *, zrwmatLoadModel, (void *source));
ORIG_FN(0x00489090, void *, zrwmatLoadChunk, (void *iff));
ORIG_FN(0x004890b0, void, zrwmatReleaseModel, (void *model));
ORIG_FN(0x004890c0, void *, zrwmatModelFromObject, (void *model));
/* Wrappers over the renderer's clump functions (they pass the clump at +0x2c of the object on). */
ORIG_FN(0x004890d0, int, zrwmatNodeCount, (void *object));
ORIG_FN(0x004890f0, void, zrwmatGetNodeHashes, (void *object, uint32_t *hashes));
/* Node of the object with the given name hash (0 when there is none). */
ORIG_FN(0x00489110, uint32_t, zrwmatFindNode, (void *object, uint32_t hash));
/* World position of a point given in the frame of a node (the flag selects the pose used). */
ORIG_FN(0x00489150, void, zrwmatNodeToWorld,
        (void *object, uint32_t node, const float *local, float *out, int flag));
/* Vector stored in a node's frame. */
ORIG_FN(0x00489180, void, zrwmatNodeVector, (void *object, uint32_t node, float *out, int flag));
/* Stores the skeleton at +0x30 of the object and hands it to the renderer. */
ORIG_FN(0x00489210, void, zrwmatSetSkeleton, (void *object, void *skeleton));
/* Matrix of a node of the object (the parent frame of an attached scene object). */
ORIG_FN(0x004891b0, void, zrwmatNodeMatrix, (void *object, uint32_t node, void *out, int flag));
/* Material and texture interface of the renderer (called through the pointers at 0x561024..0x561048). */
/* Releases a texture handle. */
ORIG_FN(0x00489530, void, zrwmatReleaseTexture, (void *texture));
/* Sets the material callback of the renderer (receives each material read by a model); returns the old one.
 */
ORIG_FN(0x004896c0, void *, zrwmatSetMaterialHook, (void *hook));
/* Creates a texture from `size` bytes of image data under the given name. */
ORIG_FN(0x004896d0, void *, zrwmatLoadSpriteTexture,
        (void *data, uint32_t size, const char *name, int a, int b));
/* Texture `index` of a material. */
ORIG_FN(0x00489a40, void *, zrwmatGetTexture, (void *material, int index));
/* Registers (takes a reference to) a material. */
ORIG_FN(0x00489af0, void, zrwmatRetainMaterial, (void *material));
/* Replaces the texture of a material. */
ORIG_FN(0x00489b80, void, zrwmatSetTexture, (void *material, void *texture));
/* Refreshes the world matrices of an object. */
ORIG_FN(0x004892a0, void, zrwmatUpdate, (void *object));

#endif
