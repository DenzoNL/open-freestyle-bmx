/*
 * The Im3D clipper of RenderWare 3.1's Direct3D 7 driver (rw/d3dclip.c): clips triangles and lines that are
 * partly outside the camera's frustum against its planes (near, far, top, bottom, left, right), adding the
 * new vertices after the level's TL vertices, and emits the pieces through rwD3DIndexTri / rwD3DIndexLine.
 */
#ifndef RW_RWD3DCLIP_H
#define RW_RWD3DCLIP_H

#include "rw/rwd3d.h"

/*
 * An edge interpolator: the vertex where the edge from `out` (outside the plane) to `in` (inside) crosses the
 * plane, in camera space with its clip flags (position, texture coordinates) and as TL vertex (colour only).
 */
typedef void(BMX_CDECL *RwD3DClipInterpFunc)(RwD3DCamVertex *vertex, const RwD3DCamVertex *out,
                                             const RwD3DCamVertex *in, RwD3DTLVertex *tl,
                                             const RwD3DTLVertex *outTL, const RwD3DTLVertex *inTL);

/* rw/d3dclip.c */
BMX_REGPARM(3)
int32_t rwD3DClipPolyPlane(int32_t *ring, int32_t *start, int32_t *end, int32_t unused0, int32_t unused1,
                           int32_t unused2, uint32_t plane, RwD3DClipInterpFunc interp);
BMX_REGPARM(3)
int32_t rwD3DClipLinePlane(int32_t *ring, int32_t *start, int32_t *end, int32_t unused0, int32_t unused1,
                           int32_t unused2, uint32_t plane, RwD3DClipInterpFunc interp);
void rwD3DInterpNearPersp(RwD3DCamVertex *vertex, const RwD3DCamVertex *out, const RwD3DCamVertex *in,
                          RwD3DTLVertex *tl, const RwD3DTLVertex *outTL, const RwD3DTLVertex *inTL);
void rwD3DInterpFarPersp(RwD3DCamVertex *vertex, const RwD3DCamVertex *out, const RwD3DCamVertex *in,
                         RwD3DTLVertex *tl, const RwD3DTLVertex *outTL, const RwD3DTLVertex *inTL);
void rwD3DInterpTopPersp(RwD3DCamVertex *vertex, const RwD3DCamVertex *out, const RwD3DCamVertex *in,
                         RwD3DTLVertex *tl, const RwD3DTLVertex *outTL, const RwD3DTLVertex *inTL);
void rwD3DInterpBottomPersp(RwD3DCamVertex *vertex, const RwD3DCamVertex *out, const RwD3DCamVertex *in,
                            RwD3DTLVertex *tl, const RwD3DTLVertex *outTL, const RwD3DTLVertex *inTL);
void rwD3DInterpLeftPersp(RwD3DCamVertex *vertex, const RwD3DCamVertex *out, const RwD3DCamVertex *in,
                          RwD3DTLVertex *tl, const RwD3DTLVertex *outTL, const RwD3DTLVertex *inTL);
void rwD3DInterpRightPersp(RwD3DCamVertex *vertex, const RwD3DCamVertex *out, const RwD3DCamVertex *in,
                           RwD3DTLVertex *tl, const RwD3DTLVertex *outTL, const RwD3DTLVertex *inTL);
void rwD3DInterpNearParallel(RwD3DCamVertex *vertex, const RwD3DCamVertex *out, const RwD3DCamVertex *in,
                             RwD3DTLVertex *tl, const RwD3DTLVertex *outTL, const RwD3DTLVertex *inTL);
void rwD3DInterpFarParallel(RwD3DCamVertex *vertex, const RwD3DCamVertex *out, const RwD3DCamVertex *in,
                            RwD3DTLVertex *tl, const RwD3DTLVertex *outTL, const RwD3DTLVertex *inTL);
void rwD3DInterpTopParallel(RwD3DCamVertex *vertex, const RwD3DCamVertex *out, const RwD3DCamVertex *in,
                            RwD3DTLVertex *tl, const RwD3DTLVertex *outTL, const RwD3DTLVertex *inTL);
void rwD3DInterpBottomParallel(RwD3DCamVertex *vertex, const RwD3DCamVertex *out, const RwD3DCamVertex *in,
                               RwD3DTLVertex *tl, const RwD3DTLVertex *outTL, const RwD3DTLVertex *inTL);
void rwD3DInterpLeftParallel(RwD3DCamVertex *vertex, const RwD3DCamVertex *out, const RwD3DCamVertex *in,
                             RwD3DTLVertex *tl, const RwD3DTLVertex *outTL, const RwD3DTLVertex *inTL);
void rwD3DInterpRightParallel(RwD3DCamVertex *vertex, const RwD3DCamVertex *out, const RwD3DCamVertex *in,
                              RwD3DTLVertex *tl, const RwD3DTLVertex *outTL, const RwD3DTLVertex *inTL);
void rwD3DClipTriPersp(int32_t a, int32_t b, int32_t c);
void rwD3DClipTriParallel(int32_t a, int32_t b, int32_t c);
void rwD3DClipLinePersp(int32_t a, int32_t b);
void rwD3DClipLineParallel(int32_t a, int32_t b);

#endif
