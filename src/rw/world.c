/* RenderWare 3.1 world: world sector accessors. */
#include "rw/rwworld.h"

BMX_ADDR(0x004c2050)
int32_t RpWorldSectorGetNumVertices(const RpWorldSector *sector)
{
    return sector->numVertices;
}

BMX_ADDR(0x004c2060)
int32_t RpWorldSectorGetNumPolygons(const RpWorldSector *sector)
{
    return sector->numPolygons;
}
