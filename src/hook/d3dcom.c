/*
 * The D3D7 driver's cut (rw/rwd3d.h) for the hook DLL and the tests: each call goes to the IDirect3DDevice7
 * at rwD3DContext + 8, through its vtable, as in the original. The native exe has its own
 * (src/driver/draw.c).
 */
#include "rw/rwd3d.h"

/* IDirect3DDevice7 vtable slots (in bytes, as the original indexes them). */
enum {
    kSetRenderState = 0x58,
    kDrawPrimitive = 0x70,
    kDrawIndexedPrimitive = 0x74,
    kSetTexture = 0x98,
    kSetTextureStageState = 0xa0,
};

static void *device(void)
{
    return *(void **)(rwD3DContext + 8);
}

static void *method(void *self, uint32_t slot)
{
    return *(void **)(*(uint8_t **)self + slot);
}

int rwD3DDevSetRenderState(uint32_t state, uint32_t value)
{
    void *self = device();
    return ((int(BMX_STDCALL *)(void *, uint32_t, uint32_t))method(self, kSetRenderState))(self, state,
                                                                                           value);
}

int rwD3DDevSetTextureStageState(uint32_t stage, uint32_t type, uint32_t value)
{
    void *self = device();
    return ((int(BMX_STDCALL *)(void *, uint32_t, uint32_t, uint32_t))method(self, kSetTextureStageState))(
        self, stage, type, value);
}

int rwD3DDevSetTexture(uint32_t stage, void *texture)
{
    void *self = device();
    return ((int(BMX_STDCALL *)(void *, uint32_t, void *))method(self, kSetTexture))(self, stage, texture);
}

int rwD3DDevDrawPrimitive(uint32_t type, uint32_t fvf, const void *vertices, uint32_t count, uint32_t flags)
{
    void *self = device();
    return ((int(BMX_STDCALL *)(void *, uint32_t, uint32_t, const void *, uint32_t, uint32_t))method(
        self, kDrawPrimitive))(self, type, fvf, vertices, count, flags);
}

int rwD3DDevDrawIndexedPrimitive(uint32_t type, uint32_t fvf, const void *vertices, uint32_t numVertices,
                                 const uint16_t *indices, uint32_t numIndices, uint32_t flags)
{
    void *self = device();
    return ((int(BMX_STDCALL *)(void *, uint32_t, uint32_t, const void *, uint32_t, const uint16_t *,
                                uint32_t, uint32_t))method(self, kDrawIndexedPrimitive))(
        self, type, fvf, vertices, numVertices, indices, numIndices, flags);
}
