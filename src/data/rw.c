/*
 * The original's globals of RenderWare (src/rw), with their initial values (docs/PORTING.md
 * X2a; types and declarations in src/data/globals.h).
 *
 * Generated once by tools/gen_data.py from NgBMX.exe; from here on it is source like the code.
 */
#include "data/data.h"

/* Functions the data points at that no header declares (assembler name = C name). */
void drvCameraBeginUpdate(void) __asm__("_drvCameraBeginUpdate");
void drvCameraClear(void) __asm__("_drvCameraClear");
void drvCameraEndUpdate(void) __asm__("_drvCameraEndUpdate");
void drvCoreExtAttach(void) __asm__("_drvCoreExtAttach");
void drvHintRenderFront2Back(void) __asm__("_drvHintRenderFront2Back");
void drvIm2DNothing(void) __asm__("_drvIm2DNothing");
void drvImageFindRasterFormat(void) __asm__("_drvImageFindRasterFormat");
void drvRasterCreate(void) __asm__("_drvRasterCreate");
void drvRasterDestroy(void) __asm__("_drvRasterDestroy");
void drvRasterLock(void) __asm__("_drvRasterLock");
void drvRasterSetFromImage(void) __asm__("_drvRasterSetFromImage");
void drvRasterShowRaster(void) __asm__("_drvRasterShowRaster");
void drvRasterSubRaster(void) __asm__("_drvRasterSubRaster");
void drvRasterUnlock(void) __asm__("_drvRasterUnlock");
void drvSetRasterContext(void) __asm__("_drvSetRasterContext");
void drvSystem(void) __asm__("_drvSystem");
void drvTextureSetRaster(void) __asm__("_drvTextureSetRaster");

/* Code nothing serves (unported, unreachable): calling one reports the address and stops. */
BMX_DATA_UNSERVED(004aa42d)
BMX_DATA_UNSERVED(004aa432)
BMX_DATA_UNSERVED(004d5410)
BMX_DATA_UNSERVED(004d5560)
BMX_DATA_UNSERVED(004db0b0)
BMX_DATA_UNSERVED(004db1f0)
BMX_DATA_UNSERVED(004db400)
BMX_DATA_UNSERVED(004e32eb)
BMX_DATA_UNSERVED(004e3334)
BMX_DATA_UNSERVED(004e3354)
BMX_DATA_UNSERVED(004ee03b)
BMX_DATA_UNSERVED(004ee047)
BMX_DATA_UNSERVED(004ee058)
BMX_DATA_UNSERVED(004ee064)
BMX_DATA_UNSERVED(004ee07e)
BMX_DATA_UNSERVED(004ee08f)
BMX_DATA_UNSERVED(004ee0a0)
BMX_DATA_UNSERVED(004ee0ac)
BMX_DATA_UNSERVED(004ee0b8)
BMX_DATA_UNSERVED(004ee0c4)
BMX_DATA_UNSERVED(004ee0db)
BMX_DATA_UNSERVED(004ee0e9)
BMX_DATA_UNSERVED(004ee0f7)
BMX_DATA_UNSERVED(004ee100)
BMX_DATA_UNSERVED(004ee10e)
BMX_DATA_UNSERVED(004ee117)
BMX_DATA_UNSERVED(004ee125)
BMX_DATA_UNSERVED(004ee12e)
BMX_DATA_UNSERVED(004ee13a)
BMX_DATA_UNSERVED(004f6bc0)
BMX_DATA_UNSERVED(004f7ea0)
BMX_DATA_UNSERVED(004f80f0)
BMX_DATA_UNSERVED(004f90f0)
BMX_DATA_UNSERVED(004f92e0)
BMX_DATA_UNSERVED(004f9510)
BMX_DATA_UNSERVED(004f9740)
BMX_DATA_UNSERVED(004f9780)
BMX_DATA_UNSERVED(004f9c90)
BMX_DATA_UNSERVED(004f9db0)
BMX_DATA_UNSERVED(00511510)
BMX_DATA_UNSERVED(005115b7)
BMX_DATA_UNSERVED(00511640)
BMX_DATA_UNSERVED(00511677)
BMX_DATA_UNSERVED(005116b3)
BMX_DATA_UNSERVED(005116ef)
BMX_DATA_UNSERVED(00511739)
BMX_DATA_UNSERVED(0051175c)
BMX_DATA_UNSERVED(005117d7)
BMX_DATA_UNSERVED(00511808)
BMX_DATA_UNSERVED(00511840)
BMX_DATA_UNSERVED(00511878)
BMX_DATA_UNSERVED(005118d0)
BMX_DATA_UNSERVED(005118dc)
BMX_DATA_UNSERVED(00518e20)
BMX_DATA_UNSERVED(00519081)
BMX_DATA_UNSERVED(005191fa)
BMX_DATA_UNSERVED(005193eb)
BMX_DATA_UNSERVED(00519531)
BMX_DATA_UNSERVED(00519741)
BMX_DATA_UNSERVED(00519c22)
BMX_DATA_UNSERVED(00519d00)
BMX_DATA_UNSERVED(00519d7e)
BMX_DATA_UNSERVED(00519dd9)
BMX_DATA_UNSERVED(0051ac60)
BMX_DATA_UNSERVED(0051ad58)
BMX_DATA_UNSERVED(0051af31)
BMX_DATA_UNSERVED(0051b010)
BMX_DATA_UNSERVED(0051b1a1)
BMX_DATA_UNSERVED(0051b270)
BMX_DATA_UNSERVED(0051b3c0)
BMX_DATA_UNSERVED(0051b4c0)
BMX_DATA_UNSERVED(0051b549)
BMX_DATA_UNSERVED(0051b599)
BMX_DATA_UNSERVED(0051e111)
BMX_DATA_UNSERVED(0051e251)
BMX_DATA_UNSERVED(0051e924)
BMX_DATA_UNSERVED(0051e92a)
BMX_DATA_UNSERVED(0051e92f)
BMX_DATA_UNSERVED(0051e935)
BMX_DATA_UNSERVED(0051e93a)
BMX_DATA_UNSERVED(0051e940)
BMX_DATA_UNSERVED(0051e946)
BMX_DATA_UNSERVED(0051e94c)
BMX_DATA_UNSERVED(0051e952)
BMX_DATA_UNSERVED(0051e96e)
BMX_DATA_UNSERVED(0051e973)
BMX_DATA_UNSERVED(0051e989)
BMX_DATA_UNSERVED(0051e98e)
BMX_DATA_UNSERVED(0051e9aa)
BMX_DATA_UNSERVED(0051e9ba)
BMX_DATA_UNSERVED(0051e9ce)
BMX_DATA_UNSERVED(0051e9de)
BMX_DATA_UNSERVED(0051e9fe)
BMX_DATA_UNSERVED(0051ea03)
BMX_DATA_UNSERVED(0051ea1d)
BMX_DATA_UNSERVED(0051ea22)
BMX_DATA_UNSERVED(0051ea42)
BMX_DATA_UNSERVED(0051ea56)
BMX_DATA_UNSERVED(0051ea6e)
BMX_DATA_UNSERVED(0051ea82)
BMX_DATA_UNSERVED(0051eaa2)
BMX_DATA_UNSERVED(0051eaa7)
BMX_DATA_UNSERVED(0051eac1)
BMX_DATA_UNSERVED(0051eac6)
BMX_DATA_UNSERVED(0051eae6)
BMX_DATA_UNSERVED(0051eafa)
BMX_DATA_UNSERVED(0051eb12)
BMX_DATA_UNSERVED(0051eb26)
BMX_DATA_UNSERVED(0051eb46)
BMX_DATA_UNSERVED(0051eb4b)
BMX_DATA_UNSERVED(0051eb65)
BMX_DATA_UNSERVED(0051eb6a)
BMX_DATA_UNSERVED(0051eb8a)
BMX_DATA_UNSERVED(0051eb9e)
BMX_DATA_UNSERVED(0051ebb6)
BMX_DATA_UNSERVED(0051ebca)
BMX_DATA_UNSERVED(0051ebea)
BMX_DATA_UNSERVED(0051ebef)
BMX_DATA_UNSERVED(0051ec09)
BMX_DATA_UNSERVED(0051ec0e)
BMX_DATA_UNSERVED(0051ec2e)
BMX_DATA_UNSERVED(0051ec42)
BMX_DATA_UNSERVED(0051ec5a)
BMX_DATA_UNSERVED(0051ec6e)
BMX_DATA_UNSERVED(0051ec8e)
BMX_DATA_UNSERVED(0051ec93)
BMX_DATA_UNSERVED(0051ecad)
BMX_DATA_UNSERVED(0051ecb2)
BMX_DATA_UNSERVED(0051ecd2)
BMX_DATA_UNSERVED(0051ece6)
BMX_DATA_UNSERVED(0051ecfe)
BMX_DATA_UNSERVED(0051ed12)
BMX_DATA_UNSERVED(0051ed32)
BMX_DATA_UNSERVED(0051ed37)
BMX_DATA_UNSERVED(0051ed51)
BMX_DATA_UNSERVED(0051ed56)
BMX_DATA_UNSERVED(0051ed76)
BMX_DATA_UNSERVED(0051ed8a)
BMX_DATA_UNSERVED(0051eda2)

/* 0x005667b0 */
RwPluginRegistry rpClumpRegistry = {.sizeOfStruct = sizeof(RpClump), .origSizeOfStruct = sizeof(RpClump)};

/* 0x005667c4 */
RwPluginRegistry rpAtomicRegistry = {.sizeOfStruct = sizeof(RpAtomic), .origSizeOfStruct = sizeof(RpAtomic)};

/* 0x005667e0 */
RwPluginRegistry rpGeometryRegistry = {.sizeOfStruct = sizeof(RpGeometry),
                                       .origSizeOfStruct = sizeof(RpGeometry)};

/* 0x00566800 */
RwPluginRegistry rpMaterialRegistry = {.sizeOfStruct = sizeof(RpMaterial),
                                       .origSizeOfStruct = sizeof(RpMaterial)};

/* 0x00566814 */
RwSurfaceProperties rpMaterialDefaultSurfaceProps = {.ambient = 1.0f, .specular = 1.0f, .diffuse = 1.0f};

/* 0x00566820 */
RwPluginRegistry rpWorldRegistry = {.sizeOfStruct = sizeof(RpWorld), .origSizeOfStruct = sizeof(RpWorld)};

/* 0x00566840 */
float d_00566840 = 0.5f;

/* 0x00566844 */
float d_00566844 = 0.5f;

/* 0x00566848 */
float d_00566848 = 0.5f;

/* 0x0056684c */
float d_0056684c = -0.5f;

/* 0x00566850 */
float d_00566850 = -0.5f;

/* 0x00566854 */
float d_00566854[3] = {-0.5f};

/* 0x00566860 */
RwPluginRegistry rpWorldSectorRegistry = {.sizeOfStruct = sizeof(RpWorldSector),
                                          .origSizeOfStruct = sizeof(RpWorldSector)};

/* 0x00566880 */
RwPluginRegistry rpLightRegistry = {.sizeOfStruct = sizeof(RpLight), .origSizeOfStruct = sizeof(RpLight)};

/* 0x005668a0 */
uint32_t rwD3DAtomicDefaultColor = 0xff000000;

/* 0x005668b0 */
uint32_t rwD3DSectorDefaultColor = 0xff000000;

/* 0x005668c0 */
BmxType_rtCharsetGlyphs rtCharsetGlyphs = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   24,  60,  60,  60,  24,
    24,  0,   24,  24,  0,   0,   0,   0,   102, 102, 102, 36,  0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   54,  54,  127, 54,  54,  54,  127, 54,  54,  0,   0,   0,   24,  24,  62,  99,  67,  3,   62,
    96,  97,  99,  62,  24,  24,  0,   0,   0,   0,   0,   67,  99,  48,  24,  12,  102, 99,  0,   0,   0,
    0,   0,   28,  54,  54,  28,  110, 59,  51,  51,  110, 0,   0,   0,   0,   12,  12,  12,  6,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   48,  24,  12,  12,  12,  12,  12,  24,  48,  0,   0,   0,
    0,   0,   12,  24,  48,  48,  48,  48,  48,  24,  12,  0,   0,   0,   0,   0,   0,   0,   102, 60,  255,
    60,  102, 0,   0,   0,   0,   0,   0,   0,   0,   0,   24,  24,  126, 24,  24,  0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   24,  24,  24,  12,  0,   0,   0,   0,   0,   0,   0,   0,   127,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   24,  24,  0,   0,   0,
    0,   0,   64,  96,  48,  24,  12,  6,   3,   1,   0,   0,   0,   0,   0,   0,   62,  99,  115, 123, 111,
    103, 99,  99,  62,  0,   0,   0,   0,   0,   24,  28,  30,  24,  24,  24,  24,  24,  126, 0,   0,   0,
    0,   0,   62,  99,  96,  48,  24,  12,  6,   99,  127, 0,   0,   0,   0,   0,   62,  99,  96,  96,  60,
    96,  96,  99,  62,  0,   0,   0,   0,   0,   48,  56,  60,  54,  51,  127, 48,  48,  120, 0,   0,   0,
    0,   0,   127, 3,   3,   3,   63,  96,  96,  99,  62,  0,   0,   0,   0,   0,   28,  6,   3,   3,   63,
    99,  99,  99,  62,  0,   0,   0,   0,   0,   127, 99,  96,  48,  24,  12,  12,  12,  12,  0,   0,   0,
    0,   0,   62,  99,  99,  99,  62,  99,  99,  99,  62,  0,   0,   0,   0,   0,   62,  99,  99,  99,  126,
    96,  96,  48,  30,  0,   0,   0,   0,   0,   0,   24,  24,  0,   0,   0,   24,  24,  0,   0,   0,   0,
    0,   0,   0,   24,  24,  0,   0,   0,   24,  24,  12,  0,   0,   0,   0,   0,   96,  48,  24,  12,  6,
    12,  24,  48,  96,  0,   0,   0,   0,   0,   0,   0,   0,   126, 0,   0,   126, 0,   0,   0,   0,   0,
    0,   0,   6,   12,  24,  48,  96,  48,  24,  12,  6,   0,   0,   0,   0,   0,   62,  99,  99,  48,  24,
    24,  0,   24,  24,  0,   0,   0,   0,   0,   62,  99,  99,  123, 123, 123, 59,  3,   62,  0,   0,   0,
    0,   0,   8,   28,  54,  99,  99,  127, 99,  99,  99,  0,   0,   0,   0,   0,   63,  102, 102, 102, 62,
    102, 102, 102, 63,  0,   0,   0,   0,   0,   60,  102, 67,  3,   3,   3,   67,  102, 60,  0,   0,   0,
    0,   0,   31,  54,  102, 102, 102, 102, 102, 54,  31,  0,   0,   0,   0,   0,   127, 102, 70,  22,  30,
    22,  70,  102, 127, 0,   0,   0,   0,   0,   127, 102, 70,  22,  30,  22,  6,   6,   15,  0,   0,   0,
    0,   0,   60,  102, 67,  3,   3,   123, 99,  102, 92,  0,   0,   0,   0,   0,   99,  99,  99,  99,  127,
    99,  99,  99,  99,  0,   0,   0,   0,   0,   60,  24,  24,  24,  24,  24,  24,  24,  60,  0,   0,   0,
    0,   0,   120, 48,  48,  48,  48,  48,  51,  51,  30,  0,   0,   0,   0,   0,   103, 102, 54,  54,  30,
    54,  54,  102, 103, 0,   0,   0,   0,   0,   15,  6,   6,   6,   6,   6,   70,  102, 127, 0,   0,   0,
    0,   0,   99,  119, 127, 127, 107, 99,  99,  99,  99,  0,   0,   0,   0,   0,   99,  103, 111, 127, 123,
    115, 99,  99,  99,  0,   0,   0,   0,   0,   28,  54,  99,  99,  99,  99,  99,  54,  28,  0,   0,   0,
    0,   0,   63,  102, 102, 102, 62,  6,   6,   6,   15,  0,   0,   0,   0,   0,   62,  99,  99,  99,  99,
    107, 123, 62,  48,  112, 0,   0,   0,   0,   63,  102, 102, 102, 62,  54,  102, 102, 103, 0,   0,   0,
    0,   0,   62,  99,  99,  6,   28,  48,  99,  99,  62,  0,   0,   0,   0,   0,   126, 126, 90,  24,  24,
    24,  24,  24,  60,  0,   0,   0,   0,   0,   99,  99,  99,  99,  99,  99,  99,  99,  62,  0,   0,   0,
    0,   0,   99,  99,  99,  99,  99,  99,  54,  28,  8,   0,   0,   0,   0,   0,   99,  99,  99,  99,  107,
    107, 127, 62,  54,  0,   0,   0,   0,   0,   99,  99,  54,  28,  28,  28,  54,  99,  99,  0,   0,   0,
    0,   0,   102, 102, 102, 102, 60,  24,  24,  24,  60,  0,   0,   0,   0,   0,   127, 99,  49,  24,  12,
    6,   67,  99,  127, 0,   0,   0,   0,   0,   60,  12,  12,  12,  12,  12,  12,  12,  60,  0,   0,   0,
    0,   0,   1,   3,   7,   14,  28,  56,  112, 96,  64,  0,   0,   0,   0,   0,   60,  48,  48,  48,  48,
    48,  48,  48,  60,  0,   0,   0,   8,   28,  54,  99,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   255, 0,   12,  12,  24,  0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   30,  48,  62,  51,  51,  110, 0,   0,   0,
    0,   0,   7,   6,   6,   30,  54,  102, 102, 102, 62,  0,   0,   0,   0,   0,   0,   0,   0,   62,  99,
    3,   3,   99,  62,  0,   0,   0,   0,   0,   56,  48,  48,  60,  54,  51,  51,  51,  110, 0,   0,   0,
    0,   0,   0,   0,   0,   62,  99,  127, 3,   99,  62,  0,   0,   0,   0,   0,   28,  54,  38,  6,   15,
    6,   6,   6,   15,  0,   0,   0,   0,   0,   0,   0,   0,   110, 51,  51,  51,  62,  48,  51,  30,  0,
    0,   0,   7,   6,   6,   54,  110, 102, 102, 102, 103, 0,   0,   0,   0,   0,   24,  24,  0,   28,  24,
    24,  24,  24,  60,  0,   0,   0,   0,   0,   96,  96,  0,   112, 96,  96,  96,  96,  102, 102, 60,  0,
    0,   0,   7,   6,   6,   102, 54,  30,  54,  102, 103, 0,   0,   0,   0,   0,   28,  24,  24,  24,  24,
    24,  24,  24,  60,  0,   0,   0,   0,   0,   0,   0,   0,   55,  127, 107, 107, 107, 99,  0,   0,   0,
    0,   0,   0,   0,   0,   59,  102, 102, 102, 102, 102, 0,   0,   0,   0,   0,   0,   0,   0,   62,  99,
    99,  99,  99,  62,  0,   0,   0,   0,   0,   0,   0,   0,   59,  102, 102, 102, 62,  6,   6,   15,  0,
    0,   0,   0,   0,   0,   110, 51,  51,  51,  62,  48,  48,  120, 0,   0,   0,   0,   0,   0,   59,  110,
    102, 6,   6,   15,  0,   0,   0,   0,   0,   0,   0,   0,   62,  99,  14,  56,  99,  62,  0,   0,   0,
    0,   0,   8,   12,  12,  63,  12,  12,  12,  108, 56,  0,   0,   0,   0,   0,   0,   0,   0,   51,  51,
    51,  51,  51,  110, 0,   0,   0,   0,   0,   0,   0,   0,   102, 102, 102, 102, 60,  24,  0,   0,   0,
    0,   0,   0,   0,   0,   99,  99,  107, 107, 127, 54,  0,   0,   0,   0,   0,   0,   0,   0,   99,  54,
    28,  28,  54,  99,  0,   0,   0,   0,   0,   0,   0,   0,   99,  99,  99,  99,  126, 96,  48,  31,  0,
    0,   0,   0,   0,   0,   127, 51,  24,  12,  102, 127};

struct BmxGroup_rwEngineRegistryGroup rwEngineRegistryGroup __attribute__((aligned(16))) = {
    .rwEngineRegistry = {.sizeOfStruct = sizeof(RwGlobals), .origSizeOfStruct = sizeof(RwGlobals)},
    .d_00566dd4 = {0x2020100, 0x3030303, 0x4040404, 0x4040404, 0x5050505, 0x5050505, 0x5050505, 0x5050505,
                   0x6060606, 0x6060606, 0x6060606, 0x6060606, 0x6060606, 0x6060606, 0x6060606, 0x6060606,
                   0x7070707, 0x7070707, 0x7070707, 0x7070707, 0x7070707, 0x7070707, 0x7070707, 0x7070707,
                   0x7070707, 0x7070707, 0x7070707, 0x7070707, 0x7070707, 0x7070707, 0x7070707, 0x7070707,
                   0x8080808, 0x8080808, 0x8080808, 0x8080808, 0x8080808, 0x8080808, 0x8080808, 0x8080808,
                   0x8080808, 0x8080808, 0x8080808, 0x8080808, 0x8080808, 0x8080808, 0x8080808, 0x8080808,
                   0x8080808, 0x8080808, 0x8080808, 0x8080808, 0x8080808, 0x8080808, 0x8080808, 0x8080808,
                   0x8080808, 0x8080808, 0x8080808, 0x8080808, 0x8080808, 0x8080808, 0x8080808, 0x8080808},
};

/* 0x00566ee0 */
RwPluginRegistry rwFrameRegistry = {.sizeOfStruct = sizeof(RwFrame), .origSizeOfStruct = sizeof(RwFrame)};

/* 0x00566f00 */
char sSoftwareCriterionRenderWare[32] = "Software\\Criterion\\RenderWare";

/* 0x00566f20 */
int32_t rwD3DRenderOrder = 2;

/* 0x00566f30 */
RwPluginRegistry rwRasterRegistry = {.sizeOfStruct = sizeof(RwRaster), .origSizeOfStruct = sizeof(RwRaster)};

/* 0x00566f50 */
RwPluginRegistry rwCameraRegistry = {.sizeOfStruct = sizeof(RwCamera), .origSizeOfStruct = sizeof(RwCamera)};

/* 0x00566f70 */
char s0123456789abcdefghijklmnopqrstuvwxyz[40] = "0123456789abcdefghijklmnopqrstuvwxyz";

/* 0x00566f98 */
RwPluginRegistry rwTextureRegistry = {.sizeOfStruct = sizeof(RwTexture),
                                      .origSizeOfStruct = sizeof(RwTexture)};

/* 0x00566fac */
RwPluginRegistry rwTexDictionaryRegistry = {.sizeOfStruct = sizeof(RwTexDictionary),
                                            .origSizeOfStruct = sizeof(RwTexDictionary)};

/* 0x00566fc0 */
RwPluginRegistry rwImageRegistry = {.sizeOfStruct = sizeof(RwImage), .origSizeOfStruct = sizeof(RwImage)};

/* 0x00566fe0 */
uint32_t d_00566fe0 = 0xffffffff;

/* 0x00566fe4, also rwOrigDevice */
RwDevice rwD3D7Device = {
    .gammaCorrection = 1.0f,
    .fpSystem = (RwSystemFunc)drvSystem,
    .zBufferFar = 0.9999f,
    .fpRenderStateSet = rwD3DRenderStateSet,
    .fpRenderStateGet = rwD3DRenderStateGet,
    .fpIm2DRenderLine = drvIm2DNothing,
    .fpIm2DRenderTriangle = drvIm2DNothing,
    .fpIm2DRenderPrimitive = (RwIm2DRenderPrimitiveFunc)drvIm2DNothing,
    .fpIm2DRenderIndexedPrimitive = drvIm2DNothing,
};

struct BmxGroup_d_0056701cGroup d_0056701cGroup __attribute__((aligned(16))) = {
    .d_0056701c = 1,
    .p_00567020 = drvCameraBeginUpdate,
    .d_00567024 = 10,
    .p_00567028 = drvCameraEndUpdate,
    .d_0056702c = 21,
    .p_00567030 = drvCameraClear,
    .d_00567034 = 20,
    .p_00567038 = drvRasterShowRaster,
    .d_0056703c = 2,
    .p_00567040 = unserved_004f7ea0,
    .d_00567044 = 3,
    .p_00567048 = unserved_004f80f0,
    .d_0056704c = 7,
    .p_00567050 = drvRasterSetFromImage,
    .d_00567054 = 6,
    .p_00567058 = unserved_004f6bc0,
    .d_0056705c = 5,
    .p_00567060 = drvRasterDestroy,
    .d_00567064 = 4,
    .p_00567068 = drvRasterCreate,
    .d_0056706c = 9,
    .p_00567070 = drvImageFindRasterFormat,
    .d_00567074 = 8,
    .p_00567078 = drvTextureSetRaster,
    .d_0056707c = 15,
    .p_00567080 = drvRasterLock,
    .d_00567084 = 16,
    .p_00567088 = drvRasterUnlock,
    .d_0056708c = 23,
    .p_00567090 = unserved_004d5410,
    .d_00567094 = 24,
    .p_00567098 = unserved_004d5560,
    .d_0056709c = 14,
    .p_005670a0 = unserved_004f92e0,
    .d_005670a4 = 13,
    .p_005670a8 = unserved_004f90f0,
    .d_005670ac = 17,
    .p_005670b0 = unserved_004f9740,
    .d_005670b4 = 18,
    .p_005670b8 = unserved_004f9780,
    .d_005670bc = 19,
    .p_005670c0 = unserved_004f9510,
    .d_005670c4 = 11,
    .p_005670c8 = drvSetRasterContext,
    .d_005670cc = 12,
    .p_005670d0 = drvRasterSubRaster,
    .d_005670d4 = 22,
    .p_005670d8 = drvHintRenderFront2Back,
    .d_005670dc = 25,
    .p_005670e0 = unserved_004db0b0,
    .d_005670e4 = 27,
    .p_005670e8 = unserved_004db1f0,
    .d_005670ec = 26,
    .p_005670f0 = unserved_004db400,
};

/* 0x00567100 */
uint32_t d_00567100[32] = {8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                           8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8};

/* 0x00567180 */
uint32_t d_00567180[32] = {8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                           8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8};

/* 0x00567200 */
uint32_t d_00567200[31] = {8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                           8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8};

struct BmxGroup_d_0056727cGroup d_0056727cGroup __attribute__((aligned(16))) = {
    .d_0056727c = {8},
    .p_0056729c = unserved_004f9c90,
    .p_005672fc = unserved_004f9db0,
};

/* 0x00567300 */
uint32_t d_00567300[4] = {0x33323130, 0x37363534, 0x42413938, 0x46454443};

/* 0x00567310 */
char s105[12] = "1.0.5";

/* 0x0056731c */
uint32_t d_0056731c[2] = {0x474e5089, 0xa1a0a0d};

/* 0x00567324 */
char sIHDR[8] = "IHDR";

/* 0x0056732c */
char sIDAT[8] = "IDAT";

/* 0x00567334 */
char sIEND[8] = "IEND";

/* 0x0056733c */
char sPLTE[8] = "PLTE";

/* 0x00567344 */
char sBKGD[8] = "bKGD";

/* 0x0056734c */
char sCHRM[8] = "cHRM";

/* 0x00567354 */
char sGAMA[8] = "gAMA";

/* 0x0056735c */
char sHIST[8] = "hIST";

/* 0x00567364 */
char sOFFs[8] = "oFFs";

/* 0x0056736c */
char sPCAL[8] = "pCAL";

/* 0x00567374 */
char sPHYs[8] = "pHYs";

/* 0x0056737c */
char sSBIT[8] = "sBIT";

/* 0x00567384 */
char sSRGB[8] = "sRGB";

/* 0x0056738c */
char sTEXt[8] = "tEXt";

/* 0x00567394 */
char sTIME[8] = "tIME";

/* 0x0056739c */
char sTRNS[8] = "tRNS";

/* 0x005673a4 */
char sZTXt[8] = "zTXt";

/* 0x005673ac */
uint32_t d_005673ac[7] = {0, 4, 0, 2, 0, 1};

/* 0x005673c8 */
uint32_t d_005673c8[7] = {8, 8, 4, 4, 2, 2, 1};

/* 0x005673e4 */
uint32_t d_005673e4[7] = {0, 0, 4, 0, 2, 0, 1};

/* 0x00567400 */
uint32_t d_00567400[7] = {8, 8, 8, 4, 4, 2, 2};

/* 0x0056741c */
uint32_t d_0056741c[7] = {128, 8, 136, 34, 170, 85, 255};

/* 0x00567438 */
uint32_t d_00567438[10] = {255, 15, 255, 51, 255, 85, 255};

/* 0x00567460 */
uint32_t d_00567460[8] = {16, 33, 66, 132, 272, 584, 1360, 4080};

/* 0x00567480 */
uint32_t d_00567480[64] = {
    0x30201000, 0x70605040, 0xb0a09080, 0xf0e0d0c0, 0x31211101, 0x71615141, 0xb1a19181, 0xf1e1d1c1,
    0x32221202, 0x72625242, 0xb2a29282, 0xf2e2d2c2, 0x33231303, 0x73635343, 0xb3a39383, 0xf3e3d3c3,
    0x34241404, 0x74645444, 0xb4a49484, 0xf4e4d4c4, 0x35251505, 0x75655545, 0xb5a59585, 0xf5e5d5c5,
    0x36261606, 0x76665646, 0xb6a69686, 0xf6e6d6c6, 0x37271707, 0x77675747, 0xb7a79787, 0xf7e7d7c7,
    0x38281808, 0x78685848, 0xb8a89888, 0xf8e8d8c8, 0x39291909, 0x79695949, 0xb9a99989, 0xf9e9d9c9,
    0x3a2a1a0a, 0x7a6a5a4a, 0xbaaa9a8a, 0xfaeadaca, 0x3b2b1b0b, 0x7b6b5b4b, 0xbbab9b8b, 0xfbebdbcb,
    0x3c2c1c0c, 0x7c6c5c4c, 0xbcac9c8c, 0xfcecdccc, 0x3d2d1d0d, 0x7d6d5d4d, 0xbdad9d8d, 0xfdedddcd,
    0x3e2e1e0e, 0x7e6e5e4e, 0xbeae9e8e, 0xfeeedece, 0x3f2f1f0f, 0x7f6f5f4f, 0xbfaf9f8f, 0xffefdfcf};

/* 0x00567580 */
uint32_t d_00567580[64] = {
    0xc0804000, 0xd0905010, 0xe0a06020, 0xf0b07030, 0xc4844404, 0xd4945414, 0xe4a46424, 0xf4b47434,
    0xc8884808, 0xd8985818, 0xe8a86828, 0xf8b87838, 0xcc8c4c0c, 0xdc9c5c1c, 0xecac6c2c, 0xfcbc7c3c,
    0xc1814101, 0xd1915111, 0xe1a16121, 0xf1b17131, 0xc5854505, 0xd5955515, 0xe5a56525, 0xf5b57535,
    0xc9894909, 0xd9995919, 0xe9a96929, 0xf9b97939, 0xcd8d4d0d, 0xdd9d5d1d, 0xedad6d2d, 0xfdbd7d3d,
    0xc2824202, 0xd2925212, 0xe2a26222, 0xf2b27232, 0xc6864606, 0xd6965616, 0xe6a66626, 0xf6b67636,
    0xca8a4a0a, 0xda9a5a1a, 0xeaaa6a2a, 0xfaba7a3a, 0xce8e4e0e, 0xde9e5e1e, 0xeeae6e2e, 0xfebe7e3e,
    0xc3834303, 0xd3935313, 0xe3a36323, 0xf3b37333, 0xc7874707, 0xd7975717, 0xe7a76727, 0xf7b77737,
    0xcb8b4b0b, 0xdb9b5b1b, 0xebab6b2b, 0xfbbb7b3b, 0xcf8f4f0f, 0xdf9f5f1f, 0xefaf6f2f, 0xffbf7f3f};

/* 0x00567680 */
uint32_t d_00567680[64] = {
    0xc0408000, 0xe060a020, 0xd0509010, 0xf070b030, 0xc8488808, 0xe868a828, 0xd8589818, 0xf878b838,
    0xc4448404, 0xe464a424, 0xd4549414, 0xf474b434, 0xcc4c8c0c, 0xec6cac2c, 0xdc5c9c1c, 0xfc7cbc3c,
    0xc2428202, 0xe262a222, 0xd2529212, 0xf272b232, 0xca4a8a0a, 0xea6aaa2a, 0xda5a9a1a, 0xfa7aba3a,
    0xc6468606, 0xe666a626, 0xd6569616, 0xf676b636, 0xce4e8e0e, 0xee6eae2e, 0xde5e9e1e, 0xfe7ebe3e,
    0xc1418101, 0xe161a121, 0xd1519111, 0xf171b131, 0xc9498909, 0xe969a929, 0xd9599919, 0xf979b939,
    0xc5458505, 0xe565a525, 0xd5559515, 0xf575b535, 0xcd4d8d0d, 0xed6dad2d, 0xdd5d9d1d, 0xfd7dbd3d,
    0xc3438303, 0xe363a323, 0xd3539313, 0xf373b333, 0xcb4b8b0b, 0xeb6bab2b, 0xdb5b9b1b, 0xfb7bbb3b,
    0xc7478707, 0xe767a727, 0xd7579717, 0xf777b737, 0xcf4f8f0f, 0xef6faf2f, 0xdf5f9f1f, 0xff7fbf3f};

/* 0x00567780 */
uint32_t d_00567780[4] = {0xffff0000};

/* 0x00567790 */
char sErrorDecodingZTXtChunk[32] = "Error decoding zTXt chunk";

/* 0x005677b0 */
uint32_t d_005677b0[256] = {
    0,          0x77073096, 0xee0e612c, 0x990951ba, 0x76dc419,  0x706af48f, 0xe963a535, 0x9e6495a3,
    0xedb8832,  0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x9b64c2b,  0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
    0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
    0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
    0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
    0x76dc4190, 0x1db7106,  0x98d220bc, 0xefd5102a, 0x71b18589, 0x6b6b51f,  0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0xf00f934,  0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x86d3d2d,  0x91646c97, 0xe6635c01,
    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
    0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
    0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
    0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
    0xedb88320, 0x9abfb3b6, 0x3b6e20c,  0x74b1d29a, 0xead54739, 0x9dd277af, 0x4db2615,  0x73dc1683,
    0xe3630b12, 0x94643b84, 0xd6d6a3e,  0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0xa00ae27,  0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
    0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
    0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
    0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x26d930a,  0x9c0906a9, 0xeb0e363f, 0x72076785, 0x5005713,
    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0xcb61b38,  0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0xbdbdf21,
    0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
    0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
    0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
    0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d};

struct BmxGroup_d_00567bb0Group d_00567bb0Group __attribute__((aligned(16))) = {
    .d_00567bb0 = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15},
    .p_00567c00 =
        {
            &sNeedDictionary,
            &sStreamEnd,
        },
};

struct BmxGroup_p_00567c08Group p_00567c08Group __attribute__((aligned(16))) = {
    .p_00567c08 =
        {
            &gGameRoundStatesGroup.gOvlEmpty,
            &sFileError,
            &sStreamError,
            &sDataError,
            &sInsufficientMemory,
            &sBufferError,
            &sIncompatibleVersion,
            &gGameRoundStatesGroup.gOvlEmpty,
        },
};

/* 0x00567c30 */
uint32_t d_00567c30[20] = {0,   1,    3,    7,    15,   31,    63,    127,  255,
                           511, 1023, 2047, 4095, 8191, 16383, 32767, 65535};

/* 0x00567c80 */
uint32_t d_00567c80[30] = {0, 0, 0, 0, 1, 1, 2, 2,  3,  3,  4,  4,  5,  5,  6,
                           6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

/* 0x00567cf8 */
uint32_t d_00567cf8[30] = {1,    2,    3,    4,    5,    7,    9,    13,    17,    25,
                           33,   49,   65,   97,   129,  193,  257,  385,   513,   769,
                           1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};

/* 0x00567d70 */
uint32_t d_00567d70[31] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2,   2,  2,
                           3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 112, 112};

/* 0x00567dec */
uint32_t d_00567dec[31] = {3,  4,  5,  6,  7,  8,  9,  10, 11,  13,  15,  17,  19,  23, 27,
                           31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};

/* 0x00567e68 */
uint32_t d_00567e68[64] = {1360, 1,    1367,  257,   1363, 17,    1371, 4097, 1361,  5,    1369, 1025, 1365,
                           65,   1373, 16385, 1360,  3,    1368,  513,  1364, 33,    1372, 8193, 1362, 9,
                           1370, 2049, 1366,  129,   1472, 24577, 1360, 2,    1367,  385,  1363, 25,   1371,
                           6145, 1361, 7,     1369,  1537, 1365,  97,   1373, 24577, 1360, 4,    1368, 769,
                           1364, 49,   1372,  12289, 1362, 13,    1370, 3073, 1366,  193,  1472, 24577};

/* 0x00567f68 */
uint32_t d_00567f68[1024] = {
    1888, 256, 2048, 80,  2048, 16,  2132, 115, 1874, 31,  2048, 112, 2048, 48,  2304, 192, 1872, 10,
    2048, 96,  2048, 32,  2304, 160, 2048, 0,   2048, 128, 2048, 64,  2304, 224, 1872, 6,   2048, 88,
    2048, 24,  2304, 144, 1875, 59,  2048, 120, 2048, 56,  2304, 208, 1873, 17,  2048, 104, 2048, 40,
    2304, 176, 2048, 8,   2048, 136, 2048, 72,  2304, 240, 1872, 4,   2048, 84,  2048, 20,  2133, 227,
    1875, 43,  2048, 116, 2048, 52,  2304, 200, 1873, 13,  2048, 100, 2048, 36,  2304, 168, 2048, 4,
    2048, 132, 2048, 68,  2304, 232, 1872, 8,   2048, 92,  2048, 28,  2304, 152, 1876, 83,  2048, 124,
    2048, 60,  2304, 216, 1874, 23,  2048, 108, 2048, 44,  2304, 184, 2048, 12,  2048, 140, 2048, 76,
    2304, 248, 1872, 3,   2048, 82,  2048, 18,  2133, 163, 1875, 35,  2048, 114, 2048, 50,  2304, 196,
    1873, 11,  2048, 98,  2048, 34,  2304, 164, 2048, 2,   2048, 130, 2048, 66,  2304, 228, 1872, 7,
    2048, 90,  2048, 26,  2304, 148, 1876, 67,  2048, 122, 2048, 58,  2304, 212, 1874, 19,  2048, 106,
    2048, 42,  2304, 180, 2048, 10,  2048, 138, 2048, 74,  2304, 244, 1872, 5,   2048, 86,  2048, 22,
    2240, 0,   1875, 51,  2048, 118, 2048, 54,  2304, 204, 1873, 15,  2048, 102, 2048, 38,  2304, 172,
    2048, 6,   2048, 134, 2048, 70,  2304, 236, 1872, 9,   2048, 94,  2048, 30,  2304, 156, 1876, 99,
    2048, 126, 2048, 62,  2304, 220, 1874, 27,  2048, 110, 2048, 46,  2304, 188, 2048, 14,  2048, 142,
    2048, 78,  2304, 252, 1888, 256, 2048, 81,  2048, 17,  2133, 131, 1874, 31,  2048, 113, 2048, 49,
    2304, 194, 1872, 10,  2048, 97,  2048, 33,  2304, 162, 2048, 1,   2048, 129, 2048, 65,  2304, 226,
    1872, 6,   2048, 89,  2048, 25,  2304, 146, 1875, 59,  2048, 121, 2048, 57,  2304, 210, 1873, 17,
    2048, 105, 2048, 41,  2304, 178, 2048, 9,   2048, 137, 2048, 73,  2304, 242, 1872, 4,   2048, 85,
    2048, 21,  2128, 258, 1875, 43,  2048, 117, 2048, 53,  2304, 202, 1873, 13,  2048, 101, 2048, 37,
    2304, 170, 2048, 5,   2048, 133, 2048, 69,  2304, 234, 1872, 8,   2048, 93,  2048, 29,  2304, 154,
    1876, 83,  2048, 125, 2048, 61,  2304, 218, 1874, 23,  2048, 109, 2048, 45,  2304, 186, 2048, 13,
    2048, 141, 2048, 77,  2304, 250, 1872, 3,   2048, 83,  2048, 19,  2133, 195, 1875, 35,  2048, 115,
    2048, 51,  2304, 198, 1873, 11,  2048, 99,  2048, 35,  2304, 166, 2048, 3,   2048, 131, 2048, 67,
    2304, 230, 1872, 7,   2048, 91,  2048, 27,  2304, 150, 1876, 67,  2048, 123, 2048, 59,  2304, 214,
    1874, 19,  2048, 107, 2048, 43,  2304, 182, 2048, 11,  2048, 139, 2048, 75,  2304, 246, 1872, 5,
    2048, 87,  2048, 23,  2240, 0,   1875, 51,  2048, 119, 2048, 55,  2304, 206, 1873, 15,  2048, 103,
    2048, 39,  2304, 174, 2048, 7,   2048, 135, 2048, 71,  2304, 238, 1872, 9,   2048, 95,  2048, 31,
    2304, 158, 1876, 99,  2048, 127, 2048, 63,  2304, 222, 1874, 27,  2048, 111, 2048, 47,  2304, 190,
    2048, 15,  2048, 143, 2048, 79,  2304, 254, 1888, 256, 2048, 80,  2048, 16,  2132, 115, 1874, 31,
    2048, 112, 2048, 48,  2304, 193, 1872, 10,  2048, 96,  2048, 32,  2304, 161, 2048, 0,   2048, 128,
    2048, 64,  2304, 225, 1872, 6,   2048, 88,  2048, 24,  2304, 145, 1875, 59,  2048, 120, 2048, 56,
    2304, 209, 1873, 17,  2048, 104, 2048, 40,  2304, 177, 2048, 8,   2048, 136, 2048, 72,  2304, 241,
    1872, 4,   2048, 84,  2048, 20,  2133, 227, 1875, 43,  2048, 116, 2048, 52,  2304, 201, 1873, 13,
    2048, 100, 2048, 36,  2304, 169, 2048, 4,   2048, 132, 2048, 68,  2304, 233, 1872, 8,   2048, 92,
    2048, 28,  2304, 153, 1876, 83,  2048, 124, 2048, 60,  2304, 217, 1874, 23,  2048, 108, 2048, 44,
    2304, 185, 2048, 12,  2048, 140, 2048, 76,  2304, 249, 1872, 3,   2048, 82,  2048, 18,  2133, 163,
    1875, 35,  2048, 114, 2048, 50,  2304, 197, 1873, 11,  2048, 98,  2048, 34,  2304, 165, 2048, 2,
    2048, 130, 2048, 66,  2304, 229, 1872, 7,   2048, 90,  2048, 26,  2304, 149, 1876, 67,  2048, 122,
    2048, 58,  2304, 213, 1874, 19,  2048, 106, 2048, 42,  2304, 181, 2048, 10,  2048, 138, 2048, 74,
    2304, 245, 1872, 5,   2048, 86,  2048, 22,  2240, 0,   1875, 51,  2048, 118, 2048, 54,  2304, 205,
    1873, 15,  2048, 102, 2048, 38,  2304, 173, 2048, 6,   2048, 134, 2048, 70,  2304, 237, 1872, 9,
    2048, 94,  2048, 30,  2304, 157, 1876, 99,  2048, 126, 2048, 62,  2304, 221, 1874, 27,  2048, 110,
    2048, 46,  2304, 189, 2048, 14,  2048, 142, 2048, 78,  2304, 253, 1888, 256, 2048, 81,  2048, 17,
    2133, 131, 1874, 31,  2048, 113, 2048, 49,  2304, 195, 1872, 10,  2048, 97,  2048, 33,  2304, 163,
    2048, 1,   2048, 129, 2048, 65,  2304, 227, 1872, 6,   2048, 89,  2048, 25,  2304, 147, 1875, 59,
    2048, 121, 2048, 57,  2304, 211, 1873, 17,  2048, 105, 2048, 41,  2304, 179, 2048, 9,   2048, 137,
    2048, 73,  2304, 243, 1872, 4,   2048, 85,  2048, 21,  2128, 258, 1875, 43,  2048, 117, 2048, 53,
    2304, 203, 1873, 13,  2048, 101, 2048, 37,  2304, 171, 2048, 5,   2048, 133, 2048, 69,  2304, 235,
    1872, 8,   2048, 93,  2048, 29,  2304, 155, 1876, 83,  2048, 125, 2048, 61,  2304, 219, 1874, 23,
    2048, 109, 2048, 45,  2304, 187, 2048, 13,  2048, 141, 2048, 77,  2304, 251, 1872, 3,   2048, 83,
    2048, 19,  2133, 195, 1875, 35,  2048, 115, 2048, 51,  2304, 199, 1873, 11,  2048, 99,  2048, 35,
    2304, 167, 2048, 3,   2048, 131, 2048, 67,  2304, 231, 1872, 7,   2048, 91,  2048, 27,  2304, 151,
    1876, 67,  2048, 123, 2048, 59,  2304, 215, 1874, 19,  2048, 107, 2048, 43,  2304, 183, 2048, 11,
    2048, 139, 2048, 75,  2304, 247, 1872, 5,   2048, 87,  2048, 23,  2240, 0,   1875, 51,  2048, 119,
    2048, 55,  2304, 207, 1873, 15,  2048, 103, 2048, 39,  2304, 175, 2048, 7,   2048, 135, 2048, 71,
    2304, 239, 1872, 9,   2048, 95,  2048, 31,  2304, 159, 1876, 99,  2048, 127, 2048, 63,  2304, 223,
    1874, 27,  2048, 111, 2048, 47,  2304, 191, 2048, 15,  2048, 143, 2048, 79,  2304, 255};

/* 0x00568f68 */
uint32_t d_00568f68 = 5;

/* 0x00568f6c */
char s_00568f6c[52] = "\t\0\0\0 inflate 1.1.3 Copyright 1995-1998 Mark Adler ";

/* 0x00568fa0 */
char sTan_00568fa0[16] = "tan";

/* 0x00568fb0 */
char sSin_00568fb0[16] = "sin";

/* 0x00568fc0 */
char sCos_00568fc0[16] = "cos";

/* 0x00568fd0 */
double d_00568fd0 = 0.5;

/* 0x00568fd8 */
char sPow_00568fd8[8] = "pow";

/* 0x00568fe0 */
unsigned char d_00568fe0[10] = {0, 0, 0, 0, 0, 0, 0, 128, 255, 127};

/* 0x00568fea */
unsigned char d_00568fea[20] = {0,   0,   0,   0,   0,   0,   0,   128, 255, 255,
                                220, 167, 215, 185, 133, 102, 113, 177, 13,  64};

/* 0x00568ffe */
unsigned char d_00568ffe[20] = {0,   0,  0,  0,  0,   0,  255, 255, 13,  64,
                                247, 54, 67, 12, 152, 25, 246, 149, 253, 63};

struct BmxGroup_d_00569012Group d_00569012Group __attribute__((aligned(16))) = {
    .d_00569012 = {0, 0, 0,   0,   0,   0, 224, 63, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                   0, 3, 101, 120, 112, 0, 0,   0,  0, 0, 0, 0, 0, 0, 1, 20},
    .p_00569034 =
        {
            unserved_0051e111,
            unserved_004aa42d,
            unserved_004aa432,
            unserved_0051e251,
        },
};

/* 0x00569050 */
uint32_t d_00569050[4] = {256, 0x7000004, 0xa0000, 3328};

/* 0x00569060 */
float d_00569060 = 0.9375f;

/* 0x00569064 */
float d_00569064 = 1.0625f;

/* 0x00569068 */
uint32_t d_00569068 = 0x5f000000;

/* 0x0056906c */
uint32_t d_0056906c[6] = {256, 0x7000004, 0xa0000, 3328, 0, 0x3fee0000};

/* 0x00569084 */
float d_00569084[2] = {0, 480.0f};

/* 0x0056908c */
uint32_t d_0056908c[4] = {0, 0x3bf00000, 0, 0x3ff00000};

/* 0x0056909c */
float d_0056909c[2] = {0, 1.75f};

/* 0x005690a4 */
unsigned char d_005690a4[10] = {0, 0, 0, 0, 0, 0, 255, 255, 254, 127};

struct BmxGroup_p_005690aeGroup p_005690aeGroup __attribute__((aligned(16))) = {
    .p_005690ae =
        {
            .p = unserved_0051e924,
        },
    .p_005690b2 =
        {
            .p = unserved_0051e92a,
        },
    .p_005690b6 =
        {
            .p = unserved_0051e92f,
        },
    .p_005690ba =
        {
            .p = unserved_0051e935,
        },
    .p_005690be =
        {
            .p = unserved_0051e93a,
        },
    .p_005690c2 =
        {
            .p = unserved_0051e940,
        },
    .p_005690c6 =
        {
            .p = unserved_0051e946,
        },
    .p_005690ca =
        {
            .p = unserved_0051e94c,
        },
    .p_005690ce =
        {
            .p = unserved_0051e952,
        },
    .p_005690d2 =
        {
            .p = unserved_0051e96e,
        },
    .p_005690d6 =
        {
            .p = unserved_0051e973,
        },
    .p_005690da =
        {
            .p = unserved_0051e989,
        },
    .p_005690de =
        {
            .p = unserved_0051e98e,
        },
    .p_005690e2 =
        {
            .p = unserved_0051e9aa,
        },
    .p_005690e6 =
        {
            .p = unserved_0051e9ba,
        },
    .p_005690ea =
        {
            .p = unserved_0051e9ce,
        },
    .p_005690ee =
        {
            .p = unserved_0051e9de,
        },
    .p_005690f2 =
        {
            .p = unserved_0051e9fe,
        },
    .p_005690f6 =
        {
            .p = unserved_0051ea03,
        },
    .p_005690fa =
        {
            .p = unserved_0051ea1d,
        },
    .p_005690fe =
        {
            .p = unserved_0051ea22,
        },
    .p_00569102 =
        {
            .p = unserved_0051ea42,
        },
    .p_00569106 =
        {
            .p = unserved_0051ea56,
        },
    .p_0056910a =
        {
            .p = unserved_0051ea6e,
        },
    .p_0056910e =
        {
            .p = unserved_0051ea82,
        },
    .p_00569112 =
        {
            .p = unserved_0051eaa2,
        },
    .p_00569116 =
        {
            .p = unserved_0051eaa7,
        },
    .p_0056911a =
        {
            .p = unserved_0051eac1,
        },
    .p_0056911e =
        {
            .p = unserved_0051eac6,
        },
    .p_00569122 =
        {
            .p = unserved_0051eae6,
        },
    .p_00569126 =
        {
            .p = unserved_0051eafa,
        },
    .p_0056912a =
        {
            .p = unserved_0051eb12,
        },
    .p_0056912e =
        {
            .p = unserved_0051eb26,
        },
    .p_00569132 =
        {
            .p = unserved_0051eb46,
        },
    .p_00569136 =
        {
            .p = unserved_0051eb4b,
        },
    .p_0056913a =
        {
            .p = unserved_0051eb65,
        },
    .p_0056913e =
        {
            .p = unserved_0051eb6a,
        },
    .p_00569142 =
        {
            .p = unserved_0051eb8a,
        },
    .p_00569146 =
        {
            .p = unserved_0051eb9e,
        },
    .p_0056914a =
        {
            .p = unserved_0051ebb6,
        },
    .p_0056914e =
        {
            .p = unserved_0051ebca,
        },
    .p_00569152 =
        {
            .p = unserved_0051ebea,
        },
    .p_00569156 =
        {
            .p = unserved_0051ebef,
        },
    .p_0056915a =
        {
            .p = unserved_0051ec09,
        },
    .p_0056915e =
        {
            .p = unserved_0051ec0e,
        },
    .p_00569162 =
        {
            .p = unserved_0051ec2e,
        },
    .p_00569166 =
        {
            .p = unserved_0051ec42,
        },
    .p_0056916a =
        {
            .p = unserved_0051ec5a,
        },
    .p_0056916e =
        {
            .p = unserved_0051ec6e,
        },
    .p_00569172 =
        {
            .p = unserved_0051ec8e,
        },
    .p_00569176 =
        {
            .p = unserved_0051ec93,
        },
    .p_0056917a =
        {
            .p = unserved_0051ecad,
        },
    .p_0056917e =
        {
            .p = unserved_0051ecb2,
        },
    .p_00569182 =
        {
            .p = unserved_0051ecd2,
        },
    .p_00569186 =
        {
            .p = unserved_0051ece6,
        },
    .p_0056918a =
        {
            .p = unserved_0051ecfe,
        },
    .p_0056918e =
        {
            .p = unserved_0051ed12,
        },
    .p_00569192 =
        {
            .p = unserved_0051ed32,
        },
    .p_00569196 =
        {
            .p = unserved_0051ed37,
        },
    .p_0056919a =
        {
            .p = unserved_0051ed51,
        },
    .p_0056919e =
        {
            .p = unserved_0051ed56,
        },
    .p_005691a2 =
        {
            .p = unserved_0051ed76,
        },
    .p_005691a6 =
        {
            .p = unserved_0051ed8a,
        },
    .p_005691aa =
        {
            .p = unserved_0051eda2,
        },
};

/* 0x005691b0 */
uint32_t d_005691b0 = 28800;

/* 0x005691b4 */
uint32_t d_005691b4 = 1;

/* 0x005691b8 */
uint32_t d_005691b8 = 0xfffff1f0;

/* 0x005691bc */
char sPST[64] = "PST";

/* 0x005691fc */
char sPDT[64] = "PDT";

/* 0x0056923c */
void *p_0056923c = &sPST;

struct BmxGroup_p_00569240Group p_00569240Group __attribute__((aligned(16))) = {
    .p_00569240 = &sPDT,
};

/* 0x00569248 */
uint32_t d_00569248 = 0xffffffff;

/* 0x0056924c */
unsigned char d_0056924c[4];

/* 0x00569250 */
unsigned char d_00569250[8];

/* 0x00569258 */
uint32_t d_00569258 = 0xffffffff;

/* 0x0056925c */
unsigned char d_0056925c[4];

/* 0x00569260 */
unsigned char d_00569260[4];

/* 0x00569264 */
uint32_t d_00569264[12] = {0xffffffff, 30, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

/* 0x00569294 */
uint32_t d_00569294 = 365;

/* 0x00569298 */
uint32_t d_00569298[2130] = {0xffffffff, 30, 58, 89, 119, 150, 180, 211, 242, 272, 303, 333, 364};

/* 0x00676710 */
int32_t rpClumpModuleOffset;

/* 0x00676714 */
int32_t rpClumpModuleCount;

/* 0x00676720 */
int32_t rpGeometryOffset;

/* 0x00676724 */
int32_t rpGeometryModuleCount;

/* 0x00676730 */
int32_t rpTieOffset;

/* 0x00676734 */
int32_t rpTieModuleCount;

/* 0x00676738 */
int32_t rpWorldCameraOffset;

/* 0x0067673c */
int32_t rpWorldAtomicOffset;

/* 0x00676740 */
int32_t rpWorldClumpOffset;

/* 0x00676744 */
int32_t rpWorldLightOffset;

/* 0x00676750 */
int32_t rpMaterialOffset;

/* 0x00676754 */
int32_t rpMaterialModuleCount;

/* 0x00676760 */
int32_t rpWorldGlobalsOffset;

/* 0x00676764 */
int32_t rpWorldModuleCount;

struct BmxGroup_rpWorldDefaultSurfacePropsGroup rpWorldDefaultSurfacePropsGroup __attribute__((aligned(16)));

/* 0x00676784 */
int32_t rpBspModuleCount;

/* 0x00676794 */
int32_t rpSectorModuleCount;

/* 0x006767a0 */
int32_t rpLightOffset;

/* 0x006767a4 */
int32_t rpLightModuleCount;

/* 0x006767b4 */
int32_t rpBinaryWorldModuleCount;

/* 0x006767c0 */
RpMeshHeader rpMeshEmptyHeader;

struct BmxGroup_rpMeshStripModeGroup rpMeshStripModeGroup __attribute__((aligned(16)));

/* 0x006767e4 */
int32_t rpMaterialListModuleCount;

/* 0x006767f0 */
unsigned char d_006767f0[8];

/* 0x006767f8 */
unsigned char d_006767f8[8];

/* 0x00676800 */
unsigned char d_00676800[8];

/* 0x00676808 */
unsigned char d_00676808[8];

/* 0x00676810 */
RpWorld *rpPVSWorld;

/* 0x00676814, also rwPvsCamera */
RwCamera *rpPVSCamera;

/* 0x00676818, also rwPvsImage */
RwImage *rpPVSImage;

/* 0x00676820 */
RwGlobals RwDefaultGlobals;

/* 0x00676968 */
int32_t rwEngineInstanceCount;

/* 0x0067696c, also gZrwStringApi */
RwGlobals *RwEngineInstance;

/* 0x00676970, also rwMatrixGlobalsOffset */
int32_t rwMatrixOffset;

/* 0x00676974 */
int32_t rwMatrixModuleCount;

/* 0x00676980 */
int32_t rwFrameOffset;

/* 0x00676984 */
int32_t rwFrameModuleCount;

/* 0x00676990 */
int32_t rwFastSqrtOffset;

/* 0x00676994 */
int32_t rwVectorModuleCount;

/* 0x00676ba4 */
int32_t rwD3DFlipPending;

/* 0x00676ba8 */
unsigned char d_00676ba8[4];

/* 0x00676bac */
RwD3DDeferList *rwD3DDefer;

/* 0x00676bb0 */
uint32_t rwD3DSortNeed;

/* 0x00676bb4 */
uint32_t rwD3DSortCap;

/* 0x00676bb8 */
uint32_t rwD3DSortCount;

/* 0x00676bbc */
RwD3DSortItem *rwD3DSortItems;

/* 0x00676bc0 */
unsigned char d_00676bc0[4];

/* 0x00676bc4 */
unsigned char d_00676bc4[256];

/* 0x00676cc4 */
int32_t rwD3DAlphaTestCaps;

/* 0x00676cc8 */
uint32_t rwD3DMergeCap;

/* 0x00676ccc */
uint16_t *rwD3DMergeBuf;

/* 0x00676cd0 */
int32_t rwD3DRemapCap;

/* 0x00676cd4 */
uint16_t *rwD3DRemap;

/* 0x00676cd8 */
uint16_t *rwD3DRemapUsed;

/* 0x00676cdc */
RwDevice *rwD3DEngineDevice;

/* 0x00676ce0 */
RwLLLink rwFreeListHead;

/* 0x00676cf0 */
int32_t rwResourcesOffset;

/* 0x00676cf4 */
int32_t rwResourcesModuleCount;

/* 0x00676d00 */
int32_t rwRasterOffset;

/* 0x00676d04 */
int32_t rwRasterModuleCount;

/* 0x00676d10 */
int32_t rwCameraOffset;

/* 0x00676d14 */
int32_t rwCameraModuleCount;

/* 0x00676d20 */
int32_t rwErrorOffset;

/* 0x00676d24 */
int32_t rwErrorModuleCount;

/* 0x00676d30 */
int32_t rwIm3DPipelineOffset;

/* 0x00676d34 */
int32_t rwIm3DModuleCount;

/* 0x00676d40 */
int32_t rwTextureOffset;

/* 0x00676d44 */
int32_t rwTextureModuleCount;

/* 0x00676d50 */
int32_t rwStreamOffset;

/* 0x00676d54 */
int32_t rwStreamModuleCount;

/* 0x00676d60 */
int32_t rwImageOffset;

/* 0x00676d64 */
int32_t rwImageModuleCount;

/* 0x00676d70 */
int32_t rwIm3DOffset;

/* 0x00676d74 */
int32_t rwRenderPipelineModuleCount;

/* 0x00676d80 */
RwFreeList *rwPluginRegEntryFreeList;

struct BmxGroup_rwEmptyStringGroup rwEmptyStringGroup __attribute__((aligned(16)));

/* 0x00676da4 */
int32_t rwColorModuleCount;

/* 0x00676db0 */
unsigned char d_00676db0[4];

/* 0x00676db4 */
unsigned char d_00676db4[4];

/* 0x00676db8 */
unsigned char d_00676db8[4];

/* 0x00676dbc */
unsigned char d_00676dbc[4];

/* 0x00676dc0 */
unsigned char d_00676dc0[4];

/* 0x00676dc4 */
unsigned char d_00676dc4[4];

/* 0x00676dc8 */
unsigned char d_00676dc8[4];

/* 0x00676dcc */
unsigned char d_00676dcc[28];

/* 0x00676de8 */
unsigned char d_00676de8[4012];

/* 0x00677d94 */
unsigned char d_00677d94[4];

/* 0x00677d98 */
unsigned char d_00677d98[4];

/* 0x00677d9c */
unsigned char d_00677d9c[4];

/* 0x00677da0 */
unsigned char d_00677da0[4];

/* 0x00677da4 */
unsigned char d_00677da4[4];

/* 0x00677da8 */
unsigned char d_00677da8[4];

/* 0x00677dac */
unsigned char d_00677dac[4];

/* 0x00677db0 */
unsigned char d_00677db0[4];

/* 0x00677db4 */
unsigned char d_00677db4[4];

/* 0x00677db8 */
unsigned char d_00677db8[4];

/* 0x00677dbc */
unsigned char d_00677dbc[4];

/* 0x00677dc0 */
unsigned char d_00677dc0[4];

/* 0x00677dc4 */
unsigned char d_00677dc4[4];

/* 0x00677dc8 */
unsigned char d_00677dc8[4];

/* 0x00677dcc */
unsigned char d_00677dcc[4];

/* 0x00677dd0 */
unsigned char d_00677dd0[4];

/* 0x00677dd4 */
unsigned char d_00677dd4[4];

/* 0x00677dd8 */
unsigned char d_00677dd8[4];

/* 0x00677ddc */
unsigned char d_00677ddc[4];

/* 0x00677de0 */
unsigned char d_00677de0[4];

/* 0x00677de4 */
unsigned char d_00677de4[4];

/* 0x00677de8 */
unsigned char d_00677de8[4];

/* 0x00677dec */
unsigned char d_00677dec[4];

/* 0x00677df0 */
unsigned char d_00677df0[4];

/* 0x00677df4 */
unsigned char d_00677df4[12];

/* 0x00677e00 */
unsigned char d_00677e00[4];

/* 0x00677e04 */
unsigned char d_00677e04[4];

/* 0x00677e08 */
unsigned char d_00677e08[4];

/* 0x00677e0c */
unsigned char d_00677e0c[4];

/* 0x00677e10 */
unsigned char d_00677e10[4];

/* 0x00677e14 */
unsigned char d_00677e14[4];

/* 0x00677e18 */
unsigned char d_00677e18[4];

/* 0x00677e1c */
unsigned char d_00677e1c[4];

/* 0x00677e20 */
unsigned char d_00677e20[4];

/* 0x00677e24 */
unsigned char d_00677e24[4];

/* 0x00677e28 */
unsigned char d_00677e28[4];

/* 0x00677e2c */
unsigned char d_00677e2c[4];

/* 0x00677e30 */
unsigned char d_00677e30[4];

/* 0x00677e34 */
unsigned char d_00677e34[4];

/* 0x00677e38 */
unsigned char d_00677e38[4];

/* 0x00677e3c */
unsigned char d_00677e3c[4];

/* 0x00677e40 */
unsigned char d_00677e40[16];

/* 0x00677e50 */
unsigned char d_00677e50[1024];

/* 0x00678250 */
unsigned char d_00678250[4];

/* 0x00678254 */
unsigned char d_00678254[4];

/* 0x00678258 */
unsigned char d_00678258[4];

/* 0x0067825c */
unsigned char d_0067825c[4];

/* 0x00678260 */
unsigned char d_00678260[4];

/* 0x00678264 */
unsigned char d_00678264[4];

/* 0x00678268 */
unsigned char d_00678268[8];

/* 0x00678270 */
int32_t rwD3DXformStride;

/* 0x00678274 */
const RwMatrix *rwD3DXformMatrix;

/* 0x00678278 */
RwD3DTLVertex *rwD3DXformOut;

/* 0x0067827c */
int32_t rwD3DXformFog;

/* 0x00678280 */
int32_t rpAnimEngineOffset;

/* 0x00678284 */
int32_t rpAnimModuleCount;

/* 0x00678288 */
int32_t rpAnimFrameOffset;

/* 0x0067828c */
int32_t rpAnimClumpOffset;

/* 0x00678290 */
int32_t rpBoneFrameOffset;

/* 0x00678294 */
unsigned char d_00678294[4];

/* 0x00678298 */
int32_t rpBoneEngineOffset;

/* 0x0067829c */
int32_t rpBoneModuleCount;

/* 0x006782a0 */
int32_t rpBoneStreamResult;

/* 0x006782a4 */
int32_t rpBoneGeometryOffset;

/* 0x006782b0 */
unsigned char d_006782b0[8];

/* 0x006782b8 */
unsigned char d_006782b8[8];

/* 0x006782c0 */
unsigned char d_006782c0[2];

/* 0x006782c2 */
unsigned char d_006782c2[4];

/* 0x006782c6 */
unsigned char d_006782c6[2];

/* 0x006782c8 */
unsigned char d_006782c8[2];

/* 0x006782ca */
unsigned char d_006782ca[6];

/* 0x006782d0 */
unsigned char d_006782d0[3];

/* 0x006782d3 */
unsigned char d_006782d3[29];

/* 0x006782f0 */
unsigned char d_006782f0[8];

/* 0x006782f8 */
unsigned char d_006782f8[4];

/* 0x006782fc */
unsigned char d_006782fc[64];

/* 0x0067833c */
unsigned char d_0067833c[2];

/* 0x0067833e */
unsigned char d_0067833e[2];

/* 0x00678340 */
unsigned char d_00678340[2];

/* 0x00678342 */
unsigned char d_00678342[2];

/* 0x00678344 */
unsigned char d_00678344[2];

/* 0x00678346 */
unsigned char d_00678346[2];

/* 0x00678348 */
unsigned char d_00678348[2];

/* 0x0067834a */
unsigned char d_0067834a[2];

/* 0x0067834c */
unsigned char d_0067834c[4];

/* 0x00678350 */
unsigned char d_00678350[64];

/* 0x00678390 */
unsigned char d_00678390[2];

/* 0x00678392 */
unsigned char d_00678392[2];

/* 0x00678394 */
unsigned char d_00678394[2];

/* 0x00678396 */
unsigned char d_00678396[2];

/* 0x00678398 */
unsigned char d_00678398[2];

/* 0x0067839a */
unsigned char d_0067839a[2];

/* 0x0067839c */
unsigned char d_0067839c[2];

/* 0x0067839e */
unsigned char d_0067839e[2];

/* 0x006783a0 */
unsigned char d_006783a0[4];

/* 0x006783a4 */
unsigned char d_006783a4[4];

/* 0x006783a8 */
unsigned char d_006783a8[8];

/* 0x006783b0 */
unsigned char d_006783b0[4];

/* 0x006783b4 */
unsigned char d_006783b4[4];

/* 0x006783b8 */
unsigned char d_006783b8[4];

/* 0x006783bc */
unsigned char d_006783bc[4];

/* 0x006783c0 */
unsigned char d_006783c0[4];

/* 0x006783c4 */
unsigned char d_006783c4[4];

/* 0x006783c8 */
unsigned char d_006783c8[4];

/* 0x006783cc */
unsigned char d_006783cc[4];

/* 0x006783d0 */
unsigned char d_006783d0[4];

/* 0x006783d4 */
unsigned char d_006783d4[40];

/* 0x006966e8 */
int32_t rpMeshOffset;

/* 0x006966ec */
int32_t rpMeshModuleCount;

/* 0x006966fc */
int32_t rpPVSSectorOffset;

/* 0x0069670c */
int32_t rpPVSWorldOffset;

/* 0x00696710 */
uint32_t rwD3DOrClip;

struct BmxGroup_rwD3DAndClipGroup rwD3DAndClipGroup __attribute__((aligned(16)));

struct BmxGroup_rwD3DStateGroup rwD3DStateGroup __attribute__((aligned(16)));

/* 0x006968f0 */
const uint8_t *rwD3DCaps;

/* 0x00696900 */
RwD3DTriFunc rwD3DClipTriCull;

/* 0x00696904 */
RwD3DTriFunc rwD3DTriCull;

/* 0x00696908 */
RwD3DTriVoidFunc rwD3DClipTri;

/* 0x0069690c */
RwD3DLineFunc rwD3DClipLine;

/* 0x00696910 */
RwD3DVoidFunc rwD3DIndexStart;

/* 0x00696914 */
RwD3DTriVoidFunc rwD3DIndexTri;

/* 0x00696918 */
RwD3DLineFunc rwD3DIndexLine;

/* 0x0069691c */
RwD3DVoidFunc rwD3DIndexFlush;

/* 0x00696920 */
RwD3DProjectionFunc rwD3DSetProjection;

/* 0x00696924 */
RwD3DUVSetFunc rwD3DSetUVStage;

/* 0x00696928 */
float rwD3DZScale;

/* 0x0069692c */
float rwD3DZShift;

/* 0x00696930 */
float rwD3DXScale;

/* 0x00696934 */
float rwD3DYScale;

/* 0x00696938 */
float rwD3DXOffset;

/* 0x0069693c */
float rwD3DYOffset;

/* 0x00696940 */
int32_t rwD3DNearZ;

/* 0x00696944 */
int32_t rwD3DFarZ;

/* 0x00696948 */
float rwD3DNearScreenZ;

/* 0x0069694c */
float rwD3DFarScreenZ;

/* 0x00696950 */
RwV2d rwD3DRecipWindow;

/* 0x00696958 */
RwD3DTLVertex *rwD3DTLBase;

/* 0x0069695c */
RwD3DCamVertex *rwD3DCamBase;

/* 0x00696960 */
int32_t rwIm3DGouraud;

/* 0x00696964 */
uint32_t rwD3DIm3DDepth;

/* 0x00696968 */
RwD3DIm3DLevel *rwD3DIm3DLevel;

/* 0x0069696c */
unsigned char d_0069696c[12];

/* 0x00696978 */
unsigned char d_00696978[4];

/* 0x0069697c */
unsigned char d_0069697c[4];

/* 0x00696980 */
unsigned char d_00696980[220];

/* 0x00696a5c */
uint32_t *rwD3DVertexBits;

/* 0x00696a60 */
uint32_t rwD3DVertexBitsSize;

/* 0x00696a64 */
RwD3DIndexList rwD3DTriIndices;

/* 0x00696a70 */
RwD3DIndexList rwD3DLineIndices;

/* 0x00696a7c */
RwD3DTLVertex *rwD3DTLBuffer;

/* 0x00696a80 */
RwD3DCamVertex *rwD3DCamBuffer;

/* 0x00696a84 */
uint32_t rwD3DBufferSize;

struct BmxGroup_rwD3DIm3DWorkGroup rwD3DIm3DWorkGroup __attribute__((aligned(16)));

/* 0x00696b00 */
unsigned char d_00696b00[1024];

/* 0x00696f00 */
unsigned char d_00696f00[4];

/* 0x00696f04 */
unsigned char d_00696f04[380];

/* 0x00697080 */
unsigned char d_00697080[8];

/* 0x00697088 */
unsigned char d_00697088[4];

/* 0x0069708c */
unsigned char d_0069708c[4];

/* 0x00697090 */
unsigned char d_00697090[56];

/* 0x006970c8 */
unsigned char d_006970c8[4];

/* 0x006970cc */
unsigned char d_006970cc[4];

/* 0x006970d0 */
unsigned char d_006970d0[4];

/* 0x006970d4 */
unsigned char d_006970d4[4];

/* 0x006970d8 */
unsigned char d_006970d8[4];

/* 0x006970dc */
unsigned char d_006970dc[4];

/* 0x006970e0 */
unsigned char d_006970e0[4];

/* 0x006970e4 */
unsigned char d_006970e4[28];

/* 0x00697100 */
unsigned char d_00697100[16];

/* 0x00697110 */
unsigned char d_00697110[56];

/* 0x00697148 */
unsigned char d_00697148[4];

/* 0x0069714c */
unsigned char d_0069714c[4];

/* 0x00697150 */
unsigned char d_00697150[4];

/* 0x00697154 */
unsigned char d_00697154[4];

/* 0x00697158 */
unsigned char d_00697158[4];

/* 0x0069715c */
unsigned char d_0069715c[4];

/* 0x00697160 */
unsigned char d_00697160[4];

/* 0x00697164 */
unsigned char d_00697164[28];

/* 0x00697180 */
unsigned char d_00697180[4];

/* 0x00697184 */
unsigned char d_00697184[4];

/* 0x00697188 */
unsigned char d_00697188[4];

/* 0x0069718c */
unsigned char d_0069718c[52];

/* 0x006971c0 */
unsigned char d_006971c0[4];

/* 0x006971c4 */
unsigned char d_006971c4[4];

/* 0x006971c8 */
unsigned char d_006971c8[4];

/* 0x006971cc */
unsigned char d_006971cc[4];

/* 0x006971d0 */
unsigned char d_006971d0[4];

/* 0x006971d4 */
unsigned char d_006971d4[4];

/* 0x006971d8 */
unsigned char d_006971d8[4];

/* 0x006971dc */
unsigned char d_006971dc[4];

/* 0x006971e0 */
unsigned char d_006971e0[4];

/* 0x006971e4 */
unsigned char d_006971e4[4];

/* 0x006971e8 */
unsigned char d_006971e8[20];

/* 0x006971fc */
unsigned char d_006971fc[4];

/* 0x00697200 */
unsigned char d_00697200[4];

/* 0x00697204 */
unsigned char d_00697204[4];

/* 0x00697208 */
unsigned char d_00697208[4];

/* 0x0069720c */
unsigned char d_0069720c[60];

/* 0x00697248 */
unsigned char d_00697248[4];

/* 0x0069724c */
unsigned char d_0069724c[4];

/* 0x00697250 */
unsigned char d_00697250[4];

/* 0x00697254 */
unsigned char d_00697254[4];

/* 0x00697258 */
unsigned char d_00697258[4];

/* 0x0069725c */
unsigned char d_0069725c[4];

/* 0x00697260 */
unsigned char d_00697260[4];

/* 0x00697264 */
unsigned char d_00697264[4];

/* 0x00697268 */
unsigned char d_00697268[24];

/* 0x00697280 */
unsigned char d_00697280[4];

/* 0x00697284 */
unsigned char d_00697284[4];

/* 0x00697288 */
unsigned char d_00697288[4];

/* 0x0069728c */
unsigned char d_0069728c[60];

/* 0x006972c8 */
unsigned char d_006972c8[4];

/* 0x006972cc */
unsigned char d_006972cc[4];

/* 0x006972d0 */
unsigned char d_006972d0[4];

/* 0x006972d4 */
unsigned char d_006972d4[4];

/* 0x006972d8 */
unsigned char d_006972d8[4];

/* 0x006972dc */
unsigned char d_006972dc[4];

/* 0x006972e0 */
unsigned char d_006972e0[4];

/* 0x006972e4 */
unsigned char d_006972e4[4];

/* 0x006972e8 */
unsigned char d_006972e8[20];

/* 0x006972fc */
unsigned char d_006972fc[4];

/* 0x00697300 */
unsigned char d_00697300[4];

/* 0x00697304 */
unsigned char d_00697304[4];

/* 0x00697308 */
unsigned char d_00697308[4];

/* 0x00697340 */
uint8_t *rwD3DContext;

/* 0x0069734c */
const RwMemoryFunctions *rwD3DMemory;

/* 0x00697350 */
unsigned char d_00697350[4];

/* 0x00697354 */
int32_t rwD3DFogStart;

/* 0x00697358 */
float rwD3DFogEnd;

/* 0x0069735c */
float rwD3DFogScale;

/* 0x00697360 */
unsigned char d_00697360[4];

/* 0x00697364 */
int32_t rwD3DRasterExtOffset;

/* 0x00697368 */
int32_t rwD3DCameraExtOffset;

/* 0x00697370 */
int32_t rpLodEngineOffset;

/* 0x00697374 */
int32_t rpLodAtomicOffset;

/* 0x00697378 */
void *rpLodCamera;

struct BmxGroup_rpLodFreeListGroup rpLodFreeListGroup __attribute__((aligned(16)));

/* 0x0069b328 */
float rpSphereMinRadius;

/* 0x0069b330 */
float rpSphereMinRadiusSq;

/* 0x0069b334 */
float rpSphereMinDistance;

/* 0x0069b338 */
unsigned char d_0069b338[4];

/* 0x0069b33c */
float d_0069b33c = 1.0f;

/* 0x0069b340 */
double d_0069b340[2] = {0.0078125, 0.0078125};

/* 0x0069b350 */
unsigned char d_0069b350[4];

/* 0x0069b354 */
float d_0069b354 = 1e-06f;

/* 0x0069b358 */
unsigned char d_0069b358[4];

/* 0x0069b35c */
unsigned char d_0069b35c[4];

/* 0x0069b360 */
float d_0069b360[3] = {0, 1.0f, 1.0f};

/* 0x0069b36c */
float d_0069b36c = 1.0f;

/* 0x0069b370 */
float d_0069b370[5] = {0, 1.0f, 1.0f, 1.0f, 1.0f};

/* 0x0069b384 */
float d_0069b384[2] = {1.0f, 1.0f};

/* 0x0069b38c */
float d_0069b38c[2] = {1.0f, 1.0f};

/* 0x0069b394 */
float d_0069b394 = 1.0f;

/* 0x0069b398 */
float d_0069b398 = 0.5f;

/* 0x0069b39c */
float d_0069b39c = 0.5f;

/* 0x0069b3a0 */
float d_0069b3a0[15] = {[0] = 0.5f, [1] = 1.0f, [2] = 1.0f, [3] = 1.0f, [14] = 1.0f};

/* 0x0069b3dc */
unsigned char d_0069b3dc[4];

/* 0x0069b3e0 */
float d_0069b3e0[2] = {0.5f, 1.0f};

/* 0x0069b3e8 */
unsigned char d_0069b3e8[4];

/* 0x0069b3ec */
float d_0069b3ec[2] = {0.5f, 1.0f};

/* 0x0069b3f4 */
unsigned char d_0069b3f4[4];

/* 0x0069b3f8 */
float d_0069b3f8[2] = {0.5f, 1.0f};

/* 0x0069b400 */
unsigned char d_0069b400[4];

/* 0x0069b404 */
float d_0069b404[2] = {0.5f};

/* 0x0069b40c */
unsigned char d_0069b40c[40];

/* 0x0069b434 */
unsigned char d_0069b434[4];

/* 0x0069b438 */
float d_0069b438 = 1e-06f;

/* 0x0069b43c */
unsigned char d_0069b43c[4];

/* 0x0069b440 */
unsigned char d_0069b440[4];

/* 0x0069b444 */
float d_0069b444[2] = {0, 1.0f};

/* 0x0069b44c */
float d_0069b44c[13] = {[10] = 1.0f};

/* 0x0069b480 */
char sNegativeSphereRadius[32] = "Negative Sphere radius";

/* 0x0069b4a0 */
float d_0069b4a0 = 0.0078125f;

/* 0x0069b4a4 */
float d_0069b4a4 = 0.0078125f;

/* 0x0069b4a8 */
uint32_t d_0069b4a8[18] = {0x3c000000, 0,          0,          0,          0x3f800000, 0x3f800000,
                           0x3f800000, 0x3f800000, 0,          0,          0x3f800000, 0x3f800000,
                           0x3f800000, 0x3f800000, 0x6167654e, 0x65766974, 0x64617220, 0x737569};

/* 0x0069b4f0 */
float rpWorldScale0 = -1.0f;

/* 0x0069b4f4 */
float rpWorldScale1 = -1.0f;

/* 0x0069b4f8 */
float rpWorldScale2 = -1.0f;

/* 0x0069b4fc */
float rpWorldReadScale0 = -1.0f;

/* 0x0069b500 */
float rpWorldReadScale1 = -1.0f;

struct BmxGroup_rpWorldReadScale2Group rpWorldReadScale2Group __attribute__((aligned(16))) = {
    .rpWorldReadScale2 = -1.0f,
    .d_0069b508 = {0, 0, 0, 1.0f, 1.0f},
};

/* 0x0069b520 */
void *p_0069b520[4] = {
    rpWorldExtAttachNothing,
    rpWorldPipelineInit,
    rpAtomicPipelineInit,
    rpWorldExtLightSelectCallback,
};

/* 0x0069b530 */
float rwD3DK0x0069b530 = 0.0078125f;

/* 0x0069b534 */
float rwD3DK0x0069b534 = 0.0078125f;

struct BmxGroup_rwD3DK0x0069b538Group rwD3DK0x0069b538Group __attribute__((aligned(16))) = {
    .rwD3DK0x0069b538 = 0.0078125f,
    .d_0069b53c = {0, 255.0f},
};

/* 0x0069b544 */
float rwD3DK0x0069b544 = 255.0f;

/* 0x0069b548 */
float rwD3DK0x0069b548 = 1.0f;

/* 0x0069b54c */
float rwD3DK0x0069b54c;

/* 0x0069b550 */
float rwD3DK0x0069b550 = 255.0f;

/* 0x0069b554 */
float rwD3DK0x0069b554;

/* 0x0069b558 */
float rwD3DK0x0069b558 = -1.0f;

/* 0x0069b55c */
float rwD3DK0x0069b55c = -1.0f;

/* 0x0069b560 */
float rwD3DK0x0069b560 = -1.0f;

/* 0x0069b564 */
float rwD3DK0x0069b564 = 255.0f;

/* 0x0069b568 */
float rwD3DK0x0069b568 = 1.0f;

/* 0x0069b56c */
float rwD3DK0x0069b56c;

/* 0x0069b570 */
float rwD3DK0x0069b570 = 255.0f;

struct BmxGroup_rwD3DK0x0069b574Group rwD3DK0x0069b574Group __attribute__((aligned(16))) = {
    .d_0069b578 = 1.0f,
};

/* 0x0069b57c */
float rwD3DK0x0069b57c = 1.0f;

/* 0x0069b580 */
float rwD3DK0x0069b580 = 255.0f;

/* 0x0069b584 */
float rwD3DK0x0069b584 = 1.0f;

/* 0x0069b588 */
float rwD3DK0x0069b588;

/* 0x0069b58c */
float rwD3DK0x0069b58c = 255.0f;

struct BmxGroup_rwD3DK0x0069b590Group rwD3DK0x0069b590Group __attribute__((aligned(16))) = {
    .d_0069b594 = 1.0f,
};

struct BmxGroup_rwD3DK0x0069b598Group rwD3DK0x0069b598Group __attribute__((aligned(16))) = {
    .rwD3DK0x0069b598 = 1.0f,
    .d_0069b59c = 1.0f,
};

/* 0x0069b5a0 */
float rwD3DK0x0069b5a0 = 1.0f;

/* 0x0069b5a4 */
float rwD3DK0x0069b5a4 = 255.0f;

/* 0x0069b5a8 */
float rwD3DK0x0069b5a8 = 1.0f;

/* 0x0069b5ac */
float rwD3DK0x0069b5ac;

/* 0x0069b5b0 */
float rwD3DK0x0069b5b0 = 255.0f;

struct BmxGroup_rwD3DK0x0069b5b4Group rwD3DK0x0069b5b4Group __attribute__((aligned(16))) = {
    .d_0069b5b8 = 1.0f,
};

struct BmxGroup_rwD3DK0x0069b5bcGroup rwD3DK0x0069b5bcGroup __attribute__((aligned(16))) = {
    .rwD3DK0x0069b5bc = 1.0f,
    .d_0069b5c0 = 1.0f,
};

/* 0x0069b5c4 */
float rwD3DK0x0069b5c4 = 1.0f;

/* 0x0069b5c8 */
float rwD3DK0x0069b5c8 = 255.0f;

struct BmxGroup_rwD3DK0x0069b5ccGroup rwD3DK0x0069b5ccGroup __attribute__((aligned(16))) = {
    .rwD3DK0x0069b5cc = 1.0f,
    .d_0069b5d0 = 1.0f,
};

/* 0x0069b5d4 */
float rwD3DK0x0069b5d4 = 1.0f;

/* 0x0069b5d8 */
float rwD3DK0x0069b5d8;

/* 0x0069b5dc */
float rwD3DK0x0069b5dc = 255.0f;

struct BmxGroup_rwD3DK0x0069b5e0Group rwD3DK0x0069b5e0Group __attribute__((aligned(16))) = {
    .rwD3DK0x0069b5e0 = 1.0f,
    .d_0069b5e4 = 1.0f,
};

struct BmxGroup_rwD3DK0x0069b5e8Group rwD3DK0x0069b5e8Group __attribute__((aligned(16))) = {
    .d_0069b5ec = 1.0f,
};

/* 0x0069b5f0 */
float rwD3DK0x0069b5f0 = 1.0f;

/* 0x0069b600 */
uint32_t d_0069b600 = 0xffffffff;

/* 0x0069b604 */
float rwD3DK0x0069b604 = 0.003921569f;

/* 0x0069b608 */
float rwD3DK0x0069b608 = 0.003921569f;

struct BmxGroup_rwD3DK0x0069b60cGroup rwD3DK0x0069b60cGroup __attribute__((aligned(16))) = {
    .rwD3DK0x0069b60c = 0.003921569f,
    .d_0069b610 = {1.0f, 1.0f},
};

/* 0x0069b618 */
float d_0069b618[2] = {91.0f, 1.0f};

/* 0x0069b620 */
float d_0069b620[2] = {1.0f, 1.0f};

/* 0x0069b628 */
float d_0069b628[3] = {1.0f, 1.0f, 1.0f};

/* 0x0069b634 */
float d_0069b634[3] = {91.0f, 1.0f, 1.0f};

/* 0x0069b640 */
double d_0069b640 = 0.5;

/* 0x0069b648 */
double d_0069b648 = 0.5;

/* 0x0069b650 */
double d_0069b650 = 0.5;

/* 0x0069b658 */
float d_0069b658[3] = {0, 1.75f, 1.0f};

/* 0x0069b664 */
float d_0069b664[3] = {91.0f};

/* 0x0069b670 */
char s03d[16] = "%03d: \n";

/* 0x0069b680 */
char s02x[16] = "%02x";

/* 0x0069b690 */
char s_0069b690[16] = "\n";

/* 0x0069b6a0 */
char sW_0069b6a0[16] = "w";

/* 0x0069b6b0 */
float d_0069b6b0 = 1.0f;

/* 0x0069b6b4 */
float d_0069b6b4[2] = {0, 1.0f};

/* 0x0069b6bc */
float d_0069b6bc = 0.25f;

/* 0x0069b6c0 */
float d_0069b6c0 = 0.5f;

/* 0x0069b6c4 */
float d_0069b6c4 = 0.25f;

/* 0x0069b6c8 */
float d_0069b6c8 = 0.5f;

/* 0x0069b6cc */
float d_0069b6cc = 0.25f;

/* 0x0069b6d0 */
float d_0069b6d0[2] = {0.5f, 0.3676955f};

/* 0x0069b6d8 */
uint32_t d_0069b6d8 = 0x3eaaaaab;

/* 0x0069b6dc */
uint32_t d_0069b6dc = 0x3eaaaaab;

/* 0x0069b6e0 */
uint32_t d_0069b6e0[2] = {0x3eaaaaab, 0x3f800000};

/* 0x0069b6e8 */
float d_0069b6e8 = 1.0f;

/* 0x0069b6ec */
float d_0069b6ec[2] = {0.001f, 1.0f};

/* 0x0069b6f4 */
float d_0069b6f4 = 0.33f;

/* 0x0069b6f8 */
float d_0069b6f8 = 0.33f;

/* 0x0069b6fc */
float d_0069b6fc[5] = {1.0f, 90.0f, 90.0f, 180.0f, 1.0f};

/* 0x0069b710 */
float d_0069b710[2] = {100.0f, 1.0f};

/* 0x0069b718 */
float d_0069b718[10] = {100.0f, 0.25f, 1.0f, 1.0f, 0, 100.0f, 0.3676955f};

/* 0x0069b740 */
char sOnlyRenderingSubSystem[32] = "Only rendering sub system";

/* 0x0069b760 */
unsigned char d_0069b760[4];

/* 0x0069b764 */
unsigned char d_0069b764[4];

/* 0x0069b768 */
unsigned char d_0069b768[4];

/* 0x0069b76c */
unsigned char d_0069b76c[4];

/* 0x0069b770 */
unsigned char d_0069b770[4];

/* 0x0069b774 */
unsigned char d_0069b774[4];

/* 0x0069b778 */
float d_0069b778 = 1.0f;

/* 0x0069b77c */
float d_0069b77c = 1.0f;

/* 0x0069b780 */
float d_0069b780 = 1.0f;

/* 0x0069b784 */
float d_0069b784 = 1.0f;

/* 0x0069b788 */
float d_0069b788 = 1.0f;

/* 0x0069b78c */
float d_0069b78c[4] = {1.0f};

/* 0x0069b79c */
uint32_t d_0069b79c = 0x3c8efa35;

/* 0x0069b7a0 */
float d_0069b7a0 = 1.0f;

/* 0x0069b7a4 */
float d_0069b7a4[2] = {0, 1.0f};

/* 0x0069b7ac */
float d_0069b7ac[25] = {
    [0] = 1.0f, [1] = 1.0f, [2] = 1.0f, [3] = 1.0f, [13] = 1.0f, [14] = 1.0f, [15] = 1.0f};

/* 0x0069b810 */
float d_0069b810 = 1.0f;

/* 0x0069b814 */
float d_0069b814[2] = {0, 1.0f};

/* 0x0069b81c */
float d_0069b81c = 1.0f;

/* 0x0069b820 */
uint32_t d_0069b820 = 0x42652ee0;

/* 0x0069b824 */
float d_0069b824 = 0.01f;

/* 0x0069b828 */
unsigned char d_0069b828[4];

/* 0x0069b82c */
float d_0069b82c[13] = {1.0f, 1.0f, 1.0f, 1.0f};

/* 0x0069b860 */
float d_0069b860 = 1.0f;

/* 0x0069b864 */
float d_0069b864 = 1.0f;

/* 0x0069b868 */
float d_0069b868[16] = {1.0f, 0, 0, 0, 1.0f, 1.0f, 1.0f};

/* 0x0069b8a8 */
float d_0069b8a8 = 1.0f;

/* 0x0069b8ac */
float d_0069b8ac = 1.0f;

/* 0x0069b8b0 */
float d_0069b8b0 = 1.0f;

/* 0x0069b8b4 */
BmxType_rwMatrixNormalErrorLimits rwMatrixNormalErrorLimits = {1.0f, 1.0f, 1.0f};

/* 0x0069b8c0 */
char sInvalidCombinationType[128] = {
    73,   110, 118, 97,  108,  105, 100,  32, 99, 111, 109,  98, 105, 110, 97,   116,  105, 111, 110,
    32,   116, 121, 112, 101,  0,   0,    0,  0,  0,   0,    0,  0,   0,   0,    -128, 63,  0,   0,
    -128, 63,  0,   0,   -128, 63,  0,    0,  0,  0,   0,    0,  0,   0,   0,    0,    0,   0,   0,
    0,    0,   0,   0,   0,    0,   0,    0,  0,  0,   0,    0,  0,   0,   0,    0,    0,   0,   0,
    0,    0,   0,   0,   0,    0,   -128, 63, 0,  0,   -128, 63, 0,   0,   -128, 63};

/* 0x0069b940 */
unsigned char d_0069b940[4];

/* 0x0069b944 */
float d_0069b944[3] = {0, 1.0f, 1.0f};

/* 0x0069b950 */
double rwInvSqrtScaleOdd = 1.0;

struct BmxGroup_rwInvSqrtScaleEvenGroup rwInvSqrtScaleEvenGroup __attribute__((aligned(16))) = {
    .rwInvSqrtScaleEven = 1.0,
    .d_0069b960 = {-1.0f, 2.0f},
};

/* 0x0069b968 */
float d_0069b968[3] = {0.5f, 0, 1.0f};

/* 0x0069b974 */
float d_0069b974 = 0.003921569f;

/* 0x0069b978 */
double d_0069b978 = 255.0;

/* 0x0069b980 */
uint32_t d_0069b980[3] = {0, 0x406fe000, 0x3f800000};

/* 0x0069bc70 */
BmxType_rwPerspCornerSteps rwPerspCornerSteps = {2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f};

/* 0x0069bc88 */
BmxType_rwViewFactors rwViewFactors = {-0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f, 0.5f};

/* 0x0069bcb0 */
float d_0069bcb0 = 0.0001f;

/* 0x0069bcb4 */
float d_0069bcb4[3] = {0.0001f, 1.0f, 1.0f};

/* 0x0069bcc0 */
float d_0069bcc0[2] = {1.0f, 1.0f};

/* 0x0069bcc8 */
float d_0069bcc8[2] = {1.0f, 1.0f};

/* 0x0069bcd0 */
float d_0069bcd0[2] = {1.0f, 1.0f};

/* 0x0069bcd8 */
uint32_t d_0069bcd8[34] = {0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000, 0,          0,
                           0x3d4ccccd, 0x41200000, 0x40a00000, 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000,
                           0,          0,          0x3d4ccccd, 0x41200000, 0x40a00000, 0,          0,
                           0,          0x7261654e, 0x696c6320, 0x6c702070, 0x20656e61, 0x62207369, 0x6e6f7965,
                           0x61662064, 0x6c632072, 0x70207069, 0x656e616c};

/* 0x0069bd60 */
char sInvalidProjectionTypeSpecified[48] = "Invalid projection type specified";

/* 0x0069bd90 */
char sNull_0069bd90[16] = "(null)";

/* 0x0069bda0 */
char rwMipSuffix = 109;

/* 0x0069bdb0 */
char sRb_0069bdb0[16] = "rb";

/* 0x0069bdc0 */
char sWb_0069bdc0[16] = "wb";

/* 0x0069bdd0 */
char sAb[16] = "ab";

/* 0x0069bde0 */
char sInvalidLengthPassed[28] = "Invalid length passed";

struct BmxGroup_d_0069bdfcGroup d_0069bdfcGroup __attribute__((aligned(16))) = {
    .p_0069be00 =
        {
            unserved_004e32eb,    rwStubReturn0_4e32f2, rwStubReturn0_4e32f9, rwStubReturn0_4e335f,
            rwStubReturn1_4e3300, rwStubReturn1_4e330a, rwStubReturn1_4e3311, rwStubReturn1_4e3318,
            rwStubReturn1_4e331f, rwStubReturn1_4e3326, rwStubReturn1_4e332d, rwStubReturn0_4e335f,
            unserved_004e3334,    rwStubReturn1_4e3338, rwStubReturn1_4e333f, rwStubReturn1_4e3346,
            rwStubReturn0_4e335f, rwStubReturn1_4e334d, unserved_004e3354,    rwStubReturn1_4e3358,
        },
    .d_0069be50 = 1.0f,
};

/* 0x0069be54 */
float d_0069be54[3] = {0.003921569f, 0, 1.875f};

/* 0x0069be60 */
double d_0069be60 = 255.0;

/* 0x0069be68 */
double d_0069be68 = 0.5;

/* 0x0069be70 */
double d_0069be70 = 255.0;

/* 0x0069be78 */
float d_0069be78[3] = {0, 1.75f, 1.0f};

/* 0x0069be84 */
float d_0069be84 = 1.0f;

/* 0x0069be88 */
float d_0069be88[2] = {0.003921569f};

/* 0x0069be90 */
double d_0069be90 = 255.0;

/* 0x0069be98 */
double d_0069be98 = 0.5;

/* 0x0069bea0 */
double d_0069bea0 = 255.0;

/* 0x0069bea8 */
uint32_t d_0069bea8[10] = {0, 0x3fe00000, 0x65646e49, 0x756f2078, 0x666f2074, 0x6e617220, 25959};

/* 0x0069bed0 */
float d_0069bed0 = 0.01f;

/* 0x0069bed4 */
float d_0069bed4 = 0.01f;

/* 0x0069bed8 */
float d_0069bed8[2] = {0.99f};

struct BmxGroup_p_0069bee0Group p_0069bee0Group __attribute__((aligned(16))) = {
    .p_0069bee0 =
        {
            drvCoreExtAttach,
            rwD3DIm3DCameraBegin,
            rwD3DIm3DRenderState,
            rwD3DIm3DPipeline,
        },
    .d_0069bef0 = {-1.0f, 2.0f},
};

/* 0x0069bef8 */
float d_0069bef8[15] = {[0] = 0.5f, [2] = 1.0f, [13] = -1.0f, [14] = 2.0f};

/* 0x0069bf34 */
float d_0069bf34[3] = {0.5f, 0, 1.0f};

/* 0x0069bf40 */
void *p_0069bf40[20] = {
    unserved_004ee03b, unserved_004ee047, unserved_004ee058, unserved_004ee064, unserved_004ee07e,
    unserved_004ee08f, unserved_004ee0a0, unserved_004ee0ac, unserved_004ee0b8, unserved_004ee0c4,
    unserved_004ee0db, unserved_004ee0e9, unserved_004ee0f7, unserved_004ee100, unserved_004ee10e,
    unserved_004ee117, unserved_004ee125, unserved_004ee13a, unserved_004ee13a, unserved_004ee12e,
};

/* 0x0069bf90 */
char sReferenceRasterizer[32] = "Reference Rasterizer";

/* 0x0069bfb0 */
char sSSSW[16] = "%s (%s) S/W";

/* 0x0069bfc0 */
char sSSW[16] = "%s S/W";

/* 0x0069bfd0 */
char sSSHW[16] = "%s (%s) H/W";

/* 0x0069bfe0 */
char sSHW[16] = "%s H/W";

/* 0x0069bff0 */
char sUnableToCreateD3DTextureFormatFreeList[48] = "Unable to create D3D Texture format free list";

/* 0x0069c020 */
char sUnableToEnumerateD3DTextureFormats[48] = "Unable to enumerate D3D Texture formats";

/* 0x0069c050 */
char sUnableToFindAnySuitableTextureFormats[48] = "Unable to find any suitable texture formats";

/* 0x0069c080 */
char sInvalidSubsystemNumber[32] = "Invalid subsystem number";

/* 0x0069c0a0 */
char sInvalidSubsystemNumber_0069c0a0[32] = "Invalid Subsystem number";

/* 0x0069c0c0 */
char sFailedToCreateDirectDraw[32] = "Failed to create Direct Draw";

/* 0x0069c0e0 */
char sFailedToCreateDirectDraw4[32] = "Failed to create Direct Draw4";

/* 0x0069c100 */
char sFailedToCreateD3D3[32] = "Failed to create D3D3";

/* 0x0069c120 */
char sFailedToSetDirectDrawCooperativeLevel[48] = "Failed to set Direct Draw Cooperative Level";

/* 0x0069c150 */
char sFailedToSetDisplayMode[32] = "Failed to set Display Mode";

/* 0x0069c170 */
char sFailedToCreatePrimarySurface[48] = "Failed to create Primary Surface";

/* 0x0069c1a0 */
char sFailedToCreateZBufferSurface[48] = "Failed to create Z buffer Surface";

/* 0x0069c1d0 */
char sFailedToAttachZBufferSurface[48] = "Failed to attach Z buffer surface";

/* 0x0069c200 */
char sFailedToCreateD3D3Device[32] = "Failed to create D3D3 Device";

/* 0x0069c220 */
char sFailedToCreateViewport[32] = "Failed to create viewport";

/* 0x0069c240 */
char sFailedToAddViewport[32] = "Failed to add viewport";

/* 0x0069c260 */
char sFailedToSetTheViewport[32] = "Failed to set the viewport";

/* 0x0069c280 */
char sFailedToCreateBackgroundMaterial[48] = "Failed to create background material";

/* 0x0069c2b0 */
char sFailedToSetBackground[32] = "Failed to set Background";

/* 0x0069c2d0 */
char sFailedToSetCooperativeLevel[32] = "Failed to set cooperative level";

/* 0x0069c2f0 */
char sFailedToCreateDirectDrawClipper[48] = "Failed to create Direct Draw clipper";

/* 0x0069c320 */
char sFailedToAllocateTexChainElement[48] = "Failed to allocate tex chain element\n";

/* 0x0069c350 */
float d_0069c350 = 0.003921569f;

/* 0x0069c354 */
float d_0069c354 = 0.003921569f;

/* 0x0069c358 */
float d_0069c358 = 0.003921569f;

/* 0x0069c35c */
float d_0069c35c = 0.003921569f;

/* 0x0069c360 */
float d_0069c360 = 1.0f;

/* 0x0069c364 */
float d_0069c364 = 0.003921569f;

/* 0x0069c368 */
float d_0069c368 = 0.003921569f;

/* 0x0069c36c */
float d_0069c36c = 0.003921569f;

/* 0x0069c370 */
float d_0069c370 = 0.003921569f;

/* 0x0069c374 */
float d_0069c374[2] = {1.0f, 1.0f};

/* 0x0069c37c */
unsigned char d_0069c37c[4];

/* 0x0069c380 */
unsigned char d_0069c380[4];

/* 0x0069c384 */
unsigned char d_0069c384[4];

/* 0x0069c388 */
float d_0069c388[3] = {0, 1.0f, 1.0f};

/* 0x0069c394 */
unsigned char d_0069c394[4];

/* 0x0069c398 */
float d_0069c398[3] = {0, 1.0f, 1.0f};

/* 0x0069c3a4 */
unsigned char d_0069c3a4[4];

/* 0x0069c3a8 */
float d_0069c3a8[3] = {0, 1.0f, 1.0f};

/* 0x0069c3b4 */
unsigned char d_0069c3b4[4];

/* 0x0069c3b8 */
float d_0069c3b8[3] = {0, 1.0f, 1.0f};

/* 0x0069c3c4 */
unsigned char d_0069c3c4[4];

/* 0x0069c3c8 */
double d_0069c3c8 = 0.0078125;

/* 0x0069c3d0 */
float d_0069c3d0 = 255.0f;

/* 0x0069c3d4 */
unsigned char d_0069c3d4[4];

/* 0x0069c3d8 */
unsigned char d_0069c3d8[4];

/* 0x0069c3dc */
unsigned char d_0069c3dc[4];

/* 0x0069c3e0 */
unsigned char d_0069c3e0[4];

/* 0x0069c3e4 */
float d_0069c3e4[11] = {0, 0, 1.0f, 1.0f};

/* 0x0069c410 */
uint32_t d_0069c410 = 0x37800000;

/* 0x0069c414 */
float d_0069c414 = 0.003921569f;

/* 0x0069c418 */
float d_0069c418 = 0.003921569f;

/* 0x0069c41c */
float d_0069c41c = 0.003921569f;

/* 0x0069c420 */
float d_0069c420 = 0.003921569f;

/* 0x0069c424 */
uint32_t d_0069c424 = 0x37800000;

/* 0x0069c428 */
uint32_t d_0069c428 = 0x37800000;

/* 0x0069c42c */
uint32_t d_0069c42c = 0x37800000;

/* 0x0069c430 */
uint32_t d_0069c430 = 0x37800000;

/* 0x0069c434 */
float d_0069c434 = 1.0f;

/* 0x0069c438 */
float d_0069c438 = 1.0f;

/* 0x0069c43c */
float d_0069c43c = 1.0f;

/* 0x0069c440 */
float d_0069c440 = 1.0f;

/* 0x0069c444 */
float d_0069c444 = 1.0f;

/* 0x0069c448 */
float d_0069c448 = 65536.0f;

/* 0x0069c44c */
float d_0069c44c = 0.003921569f;

/* 0x0069c450 */
float d_0069c450 = 0.003921569f;

/* 0x0069c454 */
float d_0069c454 = 0.003921569f;

/* 0x0069c458 */
float d_0069c458 = 0.003921569f;

/* 0x0069c45c */
uint32_t d_0069c45c = 0x37800000;

/* 0x0069c460 */
uint32_t d_0069c460 = 0x37800000;

/* 0x0069c464 */
uint32_t d_0069c464 = 0x37800000;

/* 0x0069c468 */
uint32_t d_0069c468 = 0x37800000;

/* 0x0069c46c */
float d_0069c46c = 0.003921569f;

/* 0x0069c470 */
float d_0069c470 = 0.003921569f;

/* 0x0069c474 */
float d_0069c474 = 0.003921569f;

/* 0x0069c478 */
float d_0069c478 = 0.003921569f;

/* 0x0069c47c */
float d_0069c47c = 0.003921569f;

/* 0x0069c480 */
float d_0069c480 = 0.003921569f;

/* 0x0069c484 */
float d_0069c484 = 0.003921569f;

/* 0x0069c488 */
float d_0069c488 = 0.003921569f;

/* 0x0069c48c */
uint32_t d_0069c48c = 0x37800000;

/* 0x0069c490 */
uint32_t d_0069c490 = 0x37800000;

/* 0x0069c494 */
uint32_t d_0069c494 = 0x37800000;

/* 0x0069c498 */
uint32_t d_0069c498 = 0x37800000;

/* 0x0069c49c */
float d_0069c49c[2] = {65536.0f, 1.0f};

/* 0x0069c4a4 */
float d_0069c4a4[2] = {65536.0f, 1.0f};

/* 0x0069c4ac */
uint32_t d_0069c4ac = 0x37800000;

/* 0x0069c4b0 */
uint32_t d_0069c4b0 = 0x37800000;

/* 0x0069c4b4 */
uint32_t d_0069c4b4 = 0x37800000;

/* 0x0069c4b8 */
uint32_t d_0069c4b8 = 0x37800000;

/* 0x0069c4bc */
uint32_t d_0069c4bc[2] = {0x37800000, 0x3f800000};

/* 0x0069c4c4 */
float d_0069c4c4 = 1.0f;

/* 0x0069c4c8 */
float d_0069c4c8 = 255.0f;

/* 0x0069c4cc */
float d_0069c4cc = 0.5f;

/* 0x0069c4d0 */
float d_0069c4d0 = 255.0f;

/* 0x0069c4d4 */
float d_0069c4d4 = 0.5f;

/* 0x0069c4d8 */
float d_0069c4d8 = 255.0f;

/* 0x0069c4dc */
float d_0069c4dc = 0.5f;

/* 0x0069c4e0 */
float d_0069c4e0 = 255.0f;

/* 0x0069c4e4 */
float d_0069c4e4 = 0.5f;

/* 0x0069c4e8 */
float d_0069c4e8 = 65536.0f;

/* 0x0069c4ec */
uint32_t d_0069c4ec = 0x37800000;

/* 0x0069c4f0 */
uint32_t d_0069c4f0 = 0x37800000;

/* 0x0069c4f4 */
uint32_t d_0069c4f4 = 0x37800000;

/* 0x0069c4f8 */
uint32_t d_0069c4f8 = 0x37800000;

/* 0x0069c4fc */
uint32_t d_0069c4fc = 0x37800000;

/* 0x0069c500 */
uint32_t d_0069c500 = 0x37800000;

/* 0x0069c504 */
uint32_t d_0069c504 = 0x37800000;

/* 0x0069c508 */
uint32_t d_0069c508[3] = {0x37800000, 0, 0x3f800000};

/* 0x0069c514 */
float d_0069c514[2] = {255.0f, 1.0f};

/* 0x0069c51c */
float d_0069c51c[3] = {255.0f, 1.0f};

/* 0x0069c528 */
float d_0069c528[2] = {255.0f, 1.0f};

/* 0x0069c530 */
float d_0069c530[3] = {255.0f, 1.0f};

/* 0x0069c53c */
float d_0069c53c[2] = {255.0f, 1.0f};

/* 0x0069c544 */
float d_0069c544[2] = {255.0f, 1.0f};

/* 0x0069c54c */
float d_0069c54c[2] = {1.0f, 1.0f};

/* 0x0069c554 */
float d_0069c554 = 255.0f;

/* 0x0069c558 */
float d_0069c558 = 1.0f;

/* 0x0069c55c */
float d_0069c55c[2] = {1.0f, 1.0f};

/* 0x0069c564 */
float d_0069c564 = 255.0f;

/* 0x0069c568 */
float d_0069c568 = 1.0f;

/* 0x0069c56c */
float d_0069c56c[3] = {1.0f, 1.0f};

/* 0x0069c578 */
float d_0069c578 = 255.0f;

/* 0x0069c57c */
float d_0069c57c[2] = {1.0f, 1.0f};

/* 0x0069c584 */
float d_0069c584[2] = {1.0f, 1.0f};

/* 0x0069c58c */
float d_0069c58c = 255.0f;

/* 0x0069c590 */
float d_0069c590[3] = {1.0f, 1.0f};

/* 0x0069c59c */
float d_0069c59c[2] = {255.0f, 1.0f};

/* 0x0069c5a4 */
float d_0069c5a4[2] = {1.0f, 1.0f};

/* 0x0069c5ac */
float d_0069c5ac[3] = {255.0f, 1.0f, 1.0f};

/* 0x0069c5b8 */
float d_0069c5b8[2] = {1.0f, 1.0f};

struct BmxGroup_rwD3DOneGroup rwD3DOneGroup __attribute__((aligned(16))) = {
    .rwD3DOne = 1.0f,
    .d_0069c5c4 = {0, 0, 0, 1.0f},
};

/* 0x0069c5d4 */
unsigned char d_0069c5d4[4];

/* 0x0069c5d8 */
float d_0069c5d8 = 1.0f;

/* 0x0069c5dc */
float d_0069c5dc = 2.0f;

/* 0x0069c5e0 */
float d_0069c5e0 = 2.0f;

/* 0x0069c5e4 */
float d_0069c5e4 = 2.0f;

/* 0x0069c5e8 */
float d_0069c5e8 = 2.0f;

/* 0x0069c5ec */
float d_0069c5ec = 1.0f;

/* 0x0069c5f0 */
float d_0069c5f0 = 2.0f;

/* 0x0069c5f4 */
float d_0069c5f4 = 2.0f;

/* 0x0069c5f8 */
float d_0069c5f8 = 2.0f;

/* 0x0069c5fc */
float d_0069c5fc = 2.0f;

/* 0x0069c600 */
float d_0069c600 = 1.0f;

/* 0x0069c604 */
float d_0069c604[4] = {2.0f};

/* 0x0069c614 */
float d_0069c614 = 1.0f;

/* 0x0069c618 */
float d_0069c618[9] = {1.0f, 1.0f};

/* 0x0069c63c */
float rpAnimDurationStart;

/* 0x0069c640 */
float rpAnimSumStartA;

struct BmxGroup_rpAnimSumStartBGroup rpAnimSumStartBGroup __attribute__((aligned(16)));

/* 0x0069c660 */
unsigned char d_0069c660[4];

/* 0x0069c664 */
unsigned char d_0069c664[4];

/* 0x0069c668 */
unsigned char d_0069c668[4];

/* 0x0069c66c */
unsigned char d_0069c66c[4];

/* 0x0069c670 */
unsigned char d_0069c670[4];

/* 0x0069c674 */
unsigned char d_0069c674[4];

/* 0x0069c678 */
float d_0069c678 = 1.0f;

/* 0x0069c67c */
float d_0069c67c = 1.0f;

/* 0x0069c680 */
unsigned char d_0069c680[4];

/* 0x0069c684 */
unsigned char d_0069c684[4];

/* 0x0069c688 */
unsigned char d_0069c688[4];

/* 0x0069c68c */
unsigned char d_0069c68c[4];

/* 0x0069c690 */
unsigned char d_0069c690[8];

/* 0x0069c698 */
float rpAnimTimeOffset;

struct BmxGroup_rpAnimStartStepGroup rpAnimStartStepGroup __attribute__((aligned(16)));

struct BmxGroup_rpAnimHeadNameGroup rpAnimHeadNameGroup __attribute__((aligned(16))) = {
    .rpAnimHeadName = 95,
    .sDummy = "dummy_",
    .d_0069c6b8 = {[2] = 1.0f, [3] = 1.0f, [4] = 1.0f, [14] = 1.0f, [15] = 1.0f, [16] = 1.0f},
};

/* 0x0069c720 */
float d_0069c720 = 0.98f;

/* 0x0069c724 */
uint32_t d_0069c724[64] = {
    [1] = 0x3f800000,  [2] = 0x3f800000,  [3] = 0x3f800000,  [13] = 0x3f800000, [14] = 0x3f800000,
    [15] = 0x3f800000, [25] = 0x7f7fffff, [26] = 0x3f800000, [27] = 0x3f800000, [28] = 0x3f800000,
    [38] = 0x7f7fffff, [39] = 0x3f800000, [40] = 0x3f800000, [41] = 0x3f800000, [51] = 0x7f7fffff,
    [52] = 0x3f800000, [53] = 0x3f800000, [54] = 0x3f800000};

/* 0x0069c824 */
float d_0069c824 = 0.5f;

/* 0x0069c828 */
float d_0069c828 = 0.5f;

/* 0x0069c82c */
float d_0069c82c = 0.5f;

/* 0x0069c830 */
double d_0069c830 = 0.0078125;

/* 0x0069c838 */
float d_0069c838 = 10.0f;

/* 0x0069c83c */
float d_0069c83c[5] = {-1.0f, -1.0f, -1.0f};

/* 0x0069c850 */
char s105_0069c850[16] = "1.0.5";

/* 0x0069c860 */
char sIncompatibleLibpngVersionInApplicationA[64] = "Incompatible libpng version in application and library";

/* 0x0069c8a0 */
char sZlibMemoryError[32] = "zlib memory error";

/* 0x0069c8c0 */
char sZlibVersionError[32] = "zlib version error";

/* 0x0069c8e0 */
char sUnknownZlibError[32] = "Unknown zlib error";

/* 0x0069c900 */
char s113[16] = "1.1.3";

/* 0x0069c910 */
char sZlibMemory[16] = "zlib memory";

/* 0x0069c920 */
char sZlibVersion[16] = "zlib version";

/* 0x0069c930 */
char sNotAPNGFile[16] = "Not a PNG file";

/* 0x0069c940 */
char sPNGFileCorruptedByASCIIConversion[48] = "PNG file corrupted by ASCII conversion";

/* 0x0069c970 */
char sMissingIHDRBeforeIDAT[32] = "Missing IHDR before IDAT";

/* 0x0069c990 */
char sMissingPLTEBeforeIDAT[32] = "Missing PLTE before IDAT";

/* 0x0069c9b0 */
char sInvalidAttemptToReadRowData[48] = "Invalid attempt to read row data";

/* 0x0069c9e0 */
char sNotEnoughImageData[32] = "Not enough image data";

/* 0x0069ca00 */
char sExtraCompressedData[32] = "Extra compressed data";

/* 0x0069ca20 */
char sDecompressionError[32] = "Decompression error";

/* 0x0069ca40 */
char sTooManyIDATSFound[32] = "Too many IDAT's found";

/* 0x0069ca60 */
char sLibpngVersion105October151999Header[64] = "\n libpng version 1.0.5 - October 15, 1999 (header)\n";

/* 0x0069caa0 */
char sTooManyBytesForPNGSignature[48] = "Too many bytes for PNG signature.";

/* 0x0069cad0 */
char sLibpngVersion105October151999CopyrightC[208] =
    "\n libpng version 1.0.5 - October 15, 1999\n   Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, "
    "Inc.\n   Copyright (c) 1996, 1997 Andreas Dilger\n   Copyright (c) 1998, 1999 Glenn Randers-Pehrson\n";

/* 0x0069cba0 */
char sCallToNULLReadFunction[32] = "Call to NULL read function";

/* 0x0069cbc0 */
char sItSAnErrorToSetBothReadDataFnAndWriteDa[80] =
    "It's an error to set both read_data_fn and write_data_fn in the ";

/* 0x0069cc10 */
char sSameStructureResettingWriteDataFnToNULL[64] = "same structure.  Resetting write_data_fn to NULL.";

/* 0x0069cc50 */
double d_0069cc50 = 0.0078125;

/* 0x0069cc58 */
unsigned char d_0069cc58[8];

/* 0x0069cc60 */
char sWidthTooLargeForLibpngToProcessImageDat[64] = "Width too large for libpng to process image data.";

/* 0x0069cca0 */
double d_0069cca0 = 1.0;

/* 0x0069cca8 */
double d_0069cca8 = 0.05;

/* 0x0069ccb0 */
double d_0069ccb0 = 0.5;

/* 0x0069ccb8 */
double d_0069ccb8 = 255.0;

/* 0x0069ccc0 */
double d_0069ccc0 = 0.5;

/* 0x0069ccc8 */
double d_0069ccc8 = 255.0;

/* 0x0069ccd0 */
unsigned char d_0069ccd0[8];

/* 0x0069ccd8 */
unsigned char d_0069ccd8[8];

/* 0x0069cce0 */
unsigned char d_0069cce0[8];

/* 0x0069cce8 */
double d_0069cce8 = 1e-06;

/* 0x0069ccf0 */
double d_0069ccf0[2] = {1.0, 1.0};

/* 0x0069cd00 */
uint32_t d_0069cd00[2] = {0x10101010, 0x3f701010};

/* 0x0069cd08 */
double d_0069cd08 = 255.0;

/* 0x0069cd10 */
double d_0069cd10 = 0.5;

/* 0x0069cd18 */
double d_0069cd18[2] = {1.0, 1.0};

/* 0x0069cd28 */
uint32_t d_0069cd28[2] = {0x10101010, 0x3f701010};

/* 0x0069cd30 */
double d_0069cd30 = 255.0;

/* 0x0069cd38 */
double d_0069cd38 = 0.5;

/* 0x0069cd40 */
double d_0069cd40 = 1e-06;

/* 0x0069cd48 */
double d_0069cd48[2] = {1.0, 1.0};

/* 0x0069cd58 */
uint32_t d_0069cd58[2] = {0x10101010, 0x3f701010};

/* 0x0069cd60 */
double d_0069cd60 = 255.0;

/* 0x0069cd68 */
double d_0069cd68 = 0.5;

/* 0x0069cd70 */
double d_0069cd70 = 1.0;

/* 0x0069cd78 */
double d_0069cd78 = 1e-06;

/* 0x0069cd80 */
double d_0069cd80[2] = {1.0, 1.0};

/* 0x0069cd90 */
double d_0069cd90 = 1.0;

/* 0x0069cd98 */
double d_0069cd98 = 0.00390625;

/* 0x0069cda0 */
double d_0069cda0 = 0.5;

/* 0x0069cda8 */
double d_0069cda8[2] = {1.0, 1.0};

/* 0x0069cdb8 */
uint32_t d_0069cdb8[2] = {0x100010, 0x3ef00010};

/* 0x0069cdc0 */
double d_0069cdc0 = 65535.0;

/* 0x0069cdc8 */
double d_0069cdc8 = 0.5;

/* 0x0069cdd0 */
double d_0069cdd0 = 1e-06;

/* 0x0069cdd8 */
double d_0069cdd8[2] = {1.0, 1.0};

/* 0x0069cde8 */
uint32_t d_0069cde8[2] = {0x100010, 0x3ef00010};

/* 0x0069cdf0 */
double d_0069cdf0 = 65535.0;

/* 0x0069cdf8 */
double d_0069cdf8 = 0.5;

/* 0x0069ce00 */
uint32_t d_0069ce00[2] = {0x100010, 0x3ef00010};

/* 0x0069ce08 */
double d_0069ce08 = 65535.0;

/* 0x0069ce10 */
double d_0069ce10 = 0.5;

/* 0x0069ce18 */
double d_0069ce18 = 1.0;

/* 0x0069ce20 */
double d_0069ce20 = 1.0;

/* 0x0069ce28 */
double d_0069ce28 = 1.0;

/* 0x0069ce30 */
double d_0069ce30 = 0.05;

/* 0x0069ce38 */
uint32_t d_0069ce38[2] = {0x10101010, 0x3f701010};

/* 0x0069ce40 */
uint32_t d_0069ce40[2] = {0x10101010, 0x3f701010};

/* 0x0069ce48 */
uint32_t d_0069ce48[2] = {0x10101010, 0x3f701010};

/* 0x0069ce50 */
double d_0069ce50 = 255.0;

/* 0x0069ce58 */
double d_0069ce58 = 0.5;

/* 0x0069ce60 */
double d_0069ce60 = 255.0;

/* 0x0069ce68 */
double d_0069ce68 = 0.5;

/* 0x0069ce70 */
double d_0069ce70 = 255.0;

/* 0x0069ce78 */
double d_0069ce78 = 0.5;

/* 0x0069ce80 */
uint32_t d_0069ce80[2] = {0x10101010, 0x3f701010};

/* 0x0069ce88 */
double d_0069ce88 = 255.0;

/* 0x0069ce90 */
double d_0069ce90 = 0.5;

/* 0x0069ce98 */
uint32_t d_0069ce98[2] = {0x10101010, 0x3f701010};

/* 0x0069cea0 */
double d_0069cea0 = 255.0;

/* 0x0069cea8 */
double d_0069cea8 = 0.5;

/* 0x0069ceb0 */
uint32_t d_0069ceb0[2] = {0x10101010, 0x3f701010};

/* 0x0069ceb8 */
double d_0069ceb8 = 255.0;

/* 0x0069cec0 */
double d_0069cec0 = 0.5;

/* 0x0069cec8 */
double d_0069cec8[2] = {1.0, 1.0};

/* 0x0069ced8 */
double d_0069ced8[2] = {1.0, 1.0};

/* 0x0069cee8 */
double d_0069cee8[2] = {1.0, 1.0};

/* 0x0069cef8 */
double d_0069cef8[2] = {1.0, 1.0};

/* 0x0069cf08 */
double d_0069cf08 = 1.0;

/* 0x0069cf10 */
double d_0069cf10 = 1.0;

/* 0x0069cf18 */
double d_0069cf18 = 1.0;

/* 0x0069cf20 */
double d_0069cf20 = 1.0;

/* 0x0069cf28 */
double d_0069cf28 = 1.0;

/* 0x0069cf30 */
double d_0069cf30 = 1.0;

/* 0x0069cf38 */
double d_0069cf38 = 0.5;

/* 0x0069cf40 */
double d_0069cf40 = 0.5;

/* 0x0069cf48 */
double d_0069cf48 = 0.5;

/* 0x0069cf50 */
double d_0069cf50 = 0.5;

/* 0x0069cf58 */
double d_0069cf58 = 0.5;

/* 0x0069cf60 */
double d_0069cf60 = 0.5;

/* 0x0069cf68 */
double d_0069cf68 = 1.0;

/* 0x0069cf70 */
double d_0069cf70 = 0.5;

/* 0x0069cf78 */
double d_0069cf78 = 0.5;

/* 0x0069cf80 */
double d_0069cf80[2] = {1.0, 1.0};

/* 0x0069cf90 */
double d_0069cf90[2] = {1.0, 1.0};

/* 0x0069cfa0 */
double d_0069cfa0[2] = {1.0, 1.0};

/* 0x0069cfb0 */
double d_0069cfb0[2] = {1.0, 1.0};

/* 0x0069cfc0 */
char sCanTDiscardCriticalDataOnCRCError[48] = "Can't discard critical data on CRC error.";

/* 0x0069cff0 */
char sApplicationMustSupplyAKnownBackgroundGa[64] = "Application must supply a known background gamma";

/* 0x0069d030 */
char sIgnoringOutOfRangeRgbToGrayCoefficients[48] = "ignoring out of range rgb_to_gray coefficients";

/* 0x0069d060 */
char sNULLRowBuffer[16] = "NULL row buffer";

/* 0x0069d070 */
char sPngDoRgbToGrayFoundNongrayPixel[48] = "png_do_rgb_to_gray found nongray pixel";

/* 0x0069d0a0 */
char sPngDoDitherReturnedRowbytes0[48] = "png_do_dither returned rowbytes=0";

/* 0x0069d0d0 */
char sOutOfMemory[80] = "Out of Memory\0\0\0Overflow in png_memcpy_check.\0\0\0Overflow in png_memset_check.";

struct BmxGroup_p_0069d120Group p_0069d120Group __attribute__((aligned(16))) = {
    .p_0069d120 =
        {
            unserved_00511510,
            unserved_005115b7,
            unserved_00511640,
            unserved_00511677,
            unserved_005116b3,
            unserved_005116ef,
            unserved_00511739,
            unserved_0051175c,
            unserved_005117d7,
            unserved_00511808,
            unserved_00511840,
            unserved_00511878,
            unserved_005118d0,
            unserved_005118dc,
        },
};

/* 0x0069d160 */
char sUnknownCompressionMethod[32] = "unknown compression method";

/* 0x0069d180 */
char sInvalidWindowSize[32] = "invalid window size";

/* 0x0069d1a0 */
char sIncorrectHeaderCheck[32] = "incorrect header check";

/* 0x0069d1c0 */
char sNeedDictionary[16] = "need dictionary";

/* 0x0069d1d0 */
char sIncorrectDataCheck[32] = "incorrect data check";

/* 0x0069d1f0 */
double d_0069d1f0 = 45455.0;

/* 0x0069d1f8 */
double d_0069d1f8 = 500.0;

/* 0x0069d200 */
float d_0069d200 = 1e-05f;

/* 0x0069d204 */
float d_0069d204 = 1e-05f;

/* 0x0069d208 */
float d_0069d208 = 1e-05f;

/* 0x0069d20c */
unsigned char d_0069d20c[4];

/* 0x0069d210 */
double d_0069d210 = 0.8;

/* 0x0069d218 */
unsigned char d_0069d218[8];

/* 0x0069d220 */
double d_0069d220 = 0.8;

/* 0x0069d228 */
double d_0069d228 = 1.0;

/* 0x0069d230 */
float d_0069d230 = 1e-05f;

/* 0x0069d234 */
float d_0069d234 = 1e-05f;

/* 0x0069d238 */
unsigned char d_0069d238[8];

/* 0x0069d240 */
double d_0069d240 = 0.8;

/* 0x0069d248 */
unsigned char d_0069d248[8];

/* 0x0069d250 */
double d_0069d250 = 0.8;

/* 0x0069d258 */
double d_0069d258 = 1.0;

/* 0x0069d260 */
float d_0069d260 = 1e-05f;

/* 0x0069d264 */
float d_0069d264 = 1e-05f;

/* 0x0069d268 */
unsigned char d_0069d268[8];

/* 0x0069d270 */
double d_0069d270 = 0.8;

/* 0x0069d278 */
unsigned char d_0069d278[8];

/* 0x0069d280 */
double d_0069d280 = 0.8;

/* 0x0069d288 */
double d_0069d288 = 1.0;

/* 0x0069d290 */
float d_0069d290 = 1e-05f;

/* 0x0069d294 */
float d_0069d294 = 1e-05f;

/* 0x0069d298 */
unsigned char d_0069d298[4];

/* 0x0069d29c */
float d_0069d29c = 0.8f;

/* 0x0069d2a0 */
unsigned char d_0069d2a0[4];

/* 0x0069d2a4 */
float d_0069d2a4 = 0.8f;

/* 0x0069d2a8 */
float d_0069d2a8 = 1.0f;

/* 0x0069d2ac */
float d_0069d2ac = 0.3127f;

/* 0x0069d2b0 */
uint32_t d_0069d2b0[2] = {0xe0000000, 0x3f50624d};

/* 0x0069d2b8 */
float d_0069d2b8[2] = {0.329f};

/* 0x0069d2c0 */
uint32_t d_0069d2c0[2] = {0xe0000000, 0x3f50624d};

/* 0x0069d2c8 */
float d_0069d2c8[2] = {0.64f};

/* 0x0069d2d0 */
uint32_t d_0069d2d0[2] = {0xe0000000, 0x3f50624d};

/* 0x0069d2d8 */
float d_0069d2d8[2] = {0.33f};

/* 0x0069d2e0 */
uint32_t d_0069d2e0[2] = {0xe0000000, 0x3f50624d};

/* 0x0069d2e8 */
float d_0069d2e8[2] = {0.3f};

/* 0x0069d2f0 */
uint32_t d_0069d2f0[2] = {0xe0000000, 0x3f50624d};

/* 0x0069d2f8 */
float d_0069d2f8[2] = {0.6f};

/* 0x0069d300 */
uint32_t d_0069d300[2] = {0xe0000000, 0x3f50624d};

/* 0x0069d308 */
float d_0069d308[2] = {0.15f};

/* 0x0069d310 */
uint32_t d_0069d310[2] = {0xe0000000, 0x3f50624d};

/* 0x0069d318 */
float d_0069d318[2] = {0.06f};

/* 0x0069d320 */
uint32_t d_0069d320[2] = {0xe0000000, 0x3f50624d};

/* 0x0069d328 */
double d_0069d328 = 0.5;

/* 0x0069d330 */
float d_0069d330[2] = {100000.0f};

/* 0x0069d338 */
double d_0069d338 = 45455.0;

/* 0x0069d340 */
double d_0069d340 = 500.0;

/* 0x0069d348 */
float d_0069d348[2] = {0.3127f};

/* 0x0069d350 */
uint32_t d_0069d350[2] = {0xe0000000, 0x3f50624d};

/* 0x0069d358 */
float d_0069d358[2] = {0.329f};

/* 0x0069d360 */
uint32_t d_0069d360[2] = {0xe0000000, 0x3f50624d};

/* 0x0069d368 */
float d_0069d368[2] = {0.64f};

/* 0x0069d370 */
uint32_t d_0069d370[2] = {0xe0000000, 0x3f50624d};

/* 0x0069d378 */
float d_0069d378[2] = {0.33f};

/* 0x0069d380 */
uint32_t d_0069d380[2] = {0xe0000000, 0x3f50624d};

/* 0x0069d388 */
float d_0069d388[2] = {0.3f};

/* 0x0069d390 */
uint32_t d_0069d390[2] = {0xe0000000, 0x3f50624d};

/* 0x0069d398 */
float d_0069d398[2] = {0.6f};

/* 0x0069d3a0 */
uint32_t d_0069d3a0[2] = {0xe0000000, 0x3f50624d};

/* 0x0069d3a8 */
float d_0069d3a8[2] = {0.15f};

/* 0x0069d3b0 */
uint32_t d_0069d3b0[2] = {0xe0000000, 0x3f50624d};

/* 0x0069d3b8 */
float d_0069d3b8[2] = {0.06f};

/* 0x0069d3c0 */
uint32_t d_0069d3c0[4] = {0xe0000000, 0x3f50624d};

/* 0x0069d3d0 */
char sCRCError[16] = "CRC error";

/* 0x0069d3e0 */
char sOutOfPlaceIHDR[32] = "Out of place IHDR";

/* 0x0069d400 */
char sInvalidIHDRChunk[32] = "Invalid IHDR chunk";

/* 0x0069d420 */
char sInvalidImageSizeInIHDR[32] = "Invalid image size in IHDR";

/* 0x0069d440 */
char sInvalidBitDepthInIHDR[32] = "Invalid bit depth in IHDR";

/* 0x0069d460 */
char sInvalidColorTypeInIHDR[32] = "Invalid color type in IHDR";

/* 0x0069d480 */
char sInvalidColorTypeBitDepthCombinationInIH[64] = "Invalid color type/bit depth combination in IHDR";

/* 0x0069d4c0 */
char sUnknownInterlaceMethodInIHDR[48] = "Unknown interlace method in IHDR";

/* 0x0069d4f0 */
char sUnknownCompressionMethodInIHDR[48] = "Unknown compression method in IHDR";

/* 0x0069d520 */
char sUnknownFilterMethodInIHDR[32] = "Unknown filter method in IHDR";

/* 0x0069d540 */
char sMissingIHDRBeforePLTE[32] = "Missing IHDR before PLTE";

/* 0x0069d560 */
char sInvalidPLTEAfterIDAT[32] = "Invalid PLTE after IDAT";

/* 0x0069d580 */
char sDuplicatePLTEChunk[32] = "Duplicate PLTE chunk";

/* 0x0069d5a0 */
char sInvalidPaletteChunk[32] = "Invalid palette chunk";

/* 0x0069d5c0 */
char sTruncatingIncorrectTRNSChunkLength[48] = "Truncating incorrect tRNS chunk length";

/* 0x0069d5f0 */
char sNoImageInFile[32] = "No image in file";

/* 0x0069d610 */
char sIncorrectIENDChunkLength[32] = "Incorrect IEND chunk length";

/* 0x0069d630 */
char sMissingIHDRBeforeGAMA[32] = "Missing IHDR before gAMA";

/* 0x0069d650 */
char sInvalidGAMAAfterIDAT[32] = "Invalid gAMA after IDAT";

/* 0x0069d670 */
char sOutOfPlaceGAMAChunk[32] = "Out of place gAMA chunk";

/* 0x0069d690 */
char sDuplicateGAMAChunk[32] = "Duplicate gAMA chunk";

/* 0x0069d6b0 */
char sIncorrectGAMAChunkLength[32] = "Incorrect gAMA chunk length";

/* 0x0069d6d0 */
char sIgnoringIncorrectGAMAValueWhenSRGBIsAls[64] = "Ignoring incorrect gAMA value when sRGB is also present";

/* 0x0069d710 */
char sMissingIHDRBeforeSBIT[32] = "Missing IHDR before sBIT";

/* 0x0069d730 */
char sInvalidSBITAfterIDAT[32] = "Invalid sBIT after IDAT";

/* 0x0069d750 */
char sOutOfPlaceSBITChunk[32] = "Out of place sBIT chunk";

/* 0x0069d770 */
char sDuplicateSBITChunk[32] = "Duplicate sBIT chunk";

/* 0x0069d790 */
char sIncorrectSBITChunkLength[32] = "Incorrect sBIT chunk length";

/* 0x0069d7b0 */
char sInvalidCHRMAfterIDAT[32] = "Invalid cHRM after IDAT";

/* 0x0069d7d0 */
char sMissingPLTEBeforeCHRM[32] = "Missing PLTE before cHRM";

/* 0x0069d7f0 */
char sDuplicateCHRMChunk[32] = "Duplicate cHRM chunk";

/* 0x0069d810 */
char sIncorrectCHRMChunkLength[32] = "Incorrect cHRM chunk length";

/* 0x0069d830 */
char sInvalidCHRMWhitePoint[32] = "Invalid cHRM white point";

/* 0x0069d850 */
char sInvalidCHRMRedPoint[32] = "Invalid cHRM red point";

/* 0x0069d870 */
char sInvalidCHRMGreenPoint[32] = "Invalid cHRM green point";

/* 0x0069d890 */
char sInvalidCHRMBluePoint[32] = "Invalid cHRM blue point";

/* 0x0069d8b0 */
char sIgnoringIncorrectCHRMValueWhenSRGBIsAls[64] = "Ignoring incorrect cHRM value when sRGB is also present";

/* 0x0069d8f0 */
char sMissingIHDRBeforeSRGB[32] = "Missing IHDR before sRGB";

/* 0x0069d910 */
char sInvalidSRGBAfterIDAT[32] = "Invalid sRGB after IDAT";

/* 0x0069d930 */
char sOutOfPlaceSRGBChunk[32] = "Out of place sRGB chunk";

/* 0x0069d950 */
char sDuplicateSRGBChunk[32] = "Duplicate sRGB chunk";

/* 0x0069d970 */
char sIncorrectSRGBChunkLength[32] = "Incorrect sRGB chunk length";

/* 0x0069d990 */
char sUnknownSRGBIntent[32] = "Unknown sRGB intent";

/* 0x0069d9b0 */
char sMissingIHDRBeforeTRNS[32] = "Missing IHDR before tRNS";

/* 0x0069d9d0 */
char sInvalidTRNSAfterIDAT[32] = "Invalid tRNS after IDAT";

/* 0x0069d9f0 */
char sDuplicateTRNSChunk[32] = "Duplicate tRNS chunk";

/* 0x0069da10 */
char sMissingPLTEBeforeTRNS[32] = "Missing PLTE before tRNS";

/* 0x0069da30 */
char sIncorrectTRNSChunkLength[32] = "Incorrect tRNS chunk length";

/* 0x0069da50 */
char sZeroLengthTRNSChunk[32] = "Zero length tRNS chunk";

/* 0x0069da70 */
char sTRNSChunkNotAllowedWithAlphaChannel[48] = "tRNS chunk not allowed with alpha channel";

/* 0x0069daa0 */
char sMissingIHDRBeforeBKGD[32] = "Missing IHDR before bKGD";

/* 0x0069dac0 */
char sInvalidBKGDAfterIDAT[32] = "Invalid bKGD after IDAT";

/* 0x0069dae0 */
char sMissingPLTEBeforeBKGD[32] = "Missing PLTE before bKGD";

/* 0x0069db00 */
char sDuplicateBKGDChunk[32] = "Duplicate bKGD chunk";

/* 0x0069db20 */
char sIncorrectBKGDChunkLength[32] = "Incorrect bKGD chunk length";

/* 0x0069db40 */
char sIncorrectBKGDChunkIndexValue[48] = "Incorrect bKGD chunk index value";

/* 0x0069db70 */
char sMissingIHDRBeforeHIST[32] = "Missing IHDR before hIST";

/* 0x0069db90 */
char sInvalidHISTAfterIDAT[32] = "Invalid hIST after IDAT";

/* 0x0069dbb0 */
char sMissingPLTEBeforeHIST[32] = "Missing PLTE before hIST";

/* 0x0069dbd0 */
char sDuplicateHISTChunk[32] = "Duplicate hIST chunk";

/* 0x0069dbf0 */
char sIncorrectHISTChunkLength[32] = "Incorrect hIST chunk length";

/* 0x0069dc10 */
char sMissingIHDRBeforePHYS[32] = "Missing IHDR before pHYS";

/* 0x0069dc30 */
char sInvalidPHYSAfterIDAT[32] = "Invalid pHYS after IDAT";

/* 0x0069dc50 */
char sDuplicatePHYSChunk[32] = "Duplicate pHYS chunk";

/* 0x0069dc70 */
char sIncorrectPHYsChunkLength[32] = "Incorrect pHYs chunk length";

/* 0x0069dc90 */
char sMissingIHDRBeforeOFFs[32] = "Missing IHDR before oFFs";

/* 0x0069dcb0 */
char sInvalidOFFsAfterIDAT[32] = "Invalid oFFs after IDAT";

/* 0x0069dcd0 */
char sDuplicateOFFsChunk[32] = "Duplicate oFFs chunk";

/* 0x0069dcf0 */
char sIncorrectOFFsChunkLength[32] = "Incorrect oFFs chunk length";

/* 0x0069dd10 */
char sMissingIHDRBeforePCAL[32] = "Missing IHDR before pCAL";

/* 0x0069dd30 */
char sInvalidPCALAfterIDAT[32] = "Invalid pCAL after IDAT";

/* 0x0069dd50 */
char sDuplicatePCALChunk[32] = "Duplicate pCAL chunk";

/* 0x0069dd70 */
char sInvalidPCALData[32] = "Invalid pCAL data";

/* 0x0069dd90 */
char sInvalidPCALParametersForEquationType[48] = "Invalid pCAL parameters for equation type";

/* 0x0069ddc0 */
char sUnrecognizedEquationTypeForPCALChunk[48] = "Unrecognized equation type for pCAL chunk";

/* 0x0069ddf0 */
char sOutOfPlaceTIMEChunk[32] = "Out of place tIME chunk";

/* 0x0069de10 */
char sDuplicateTIMEChunk[32] = "Duplicate tIME chunk";

/* 0x0069de30 */
char sIncorrectTIMEChunkLength[32] = "Incorrect tIME chunk length";

/* 0x0069de50 */
char sMissingIHDRBeforeTEXt[32] = "Missing IHDR before tEXt";

/* 0x0069de70 */
char sMissingIHDRBeforeZTXt[32] = "Missing IHDR before zTXt";

/* 0x0069de90 */
char sZeroLengthZTXtChunk[32] = "Zero length zTXt chunk";

/* 0x0069deb0 */
char sUnknownZTXtCompressionType[32] = "Unknown zTXt compression type";

/* 0x0069ded0 */
char sUnknownCriticalChunk[32] = "unknown critical chunk";

/* 0x0069def0 */
char sInvalidChunkType[32] = "invalid chunk type";

/* 0x0069df10 */
char sIgnoringBadAdaptiveFilterType[48] = "Ignoring bad adaptive filter type";

/* 0x0069df40 */
char sDecompressionError_0069df40[32] = "Decompression Error";

/* 0x0069df60 */
char sExtraCompressionData[80] =
    "Extra compression data\0\0\0\0\0\0\0\0\0\0Rowbytes overflow in png_read_start_row";

struct BmxGroup_p_0069dfb0Group p_0069dfb0Group __attribute__((aligned(16))) = {
    .p_0069dfb0 =
        {
            unserved_00518e20,
            unserved_00519081,
            unserved_005191fa,
            unserved_005193eb,
            unserved_00519531,
            unserved_00519741,
            unserved_00519c22,
            unserved_00519d00,
            unserved_00519d7e,
            unserved_00519dd9,
        },
};

/* 0x0069dfe0 */
char sInvalidBlockType[32] = "invalid block type";

/* 0x0069e000 */
char sInvalidStoredBlockLengths[32] = "invalid stored block lengths";

/* 0x0069e020 */
char sTooManyLengthOrDistanceSymbols[48] = "too many length or distance symbols";

/* 0x0069e050 */
char sInvalidBitLengthRepeat[32] = "invalid bit length repeat";

/* 0x0069e070 */
char sStreamEnd[16] = "stream end";

/* 0x0069e080 */
char sFileError[16] = "file error";

/* 0x0069e090 */
char sStreamError[16] = "stream error";

/* 0x0069e0a0 */
char sDataError[16] = "data error";

/* 0x0069e0b0 */
char sInsufficientMemory[32] = "insufficient memory";

/* 0x0069e0d0 */
char sBufferError[16] = "buffer error";

/* 0x0069e0e0 */
char sIncompatibleVersion[32] = "incompatible version";

/* 0x0069e100 */
uint32_t d_0069e100[20] = {0xe0000000, 0x3fd40346, 0,          0x3fd50e56, 0x40000000, 0x3fe47ae1,
                           0x60000000, 0x3fd51eb8, 0x40000000, 0x3fd33333, 0x40000000, 0x3fe33333,
                           0x40000000, 0x3fc33333, 0xe0000000, 0x3faeb851, 0x3ee8bac7};

/* 0x0069e150 */
char sWidthTooLargeToProcessImageDataRowbytes[64] =
    "Width too large to process image data; rowbytes will overflow.";

struct BmxGroup_p_0069e190Group p_0069e190Group __attribute__((aligned(16))) = {
    .p_0069e190 =
        {
            unserved_0051ac60,
            unserved_0051ad58,
            unserved_0051af31,
            unserved_0051b010,
            unserved_0051b1a1,
            unserved_0051b270,
            unserved_0051b3c0,
            unserved_0051b4c0,
            unserved_0051b549,
            unserved_0051b599,
        },
};

/* 0x0069e1c0 */
char sInvalidLiteralLengthCode[32] = "invalid literal/length code";

/* 0x0069e1e0 */
char sInvalidDistanceCode[32] = "invalid distance code";

/* 0x0069e200 */
char sOversubscribedDynamicBitLengthsTree[48] = "oversubscribed dynamic bit lengths tree";

/* 0x0069e230 */
char sIncompleteDynamicBitLengthsTree[48] = "incomplete dynamic bit lengths tree";

/* 0x0069e260 */
char sOversubscribedLiteralLengthTree[48] = "oversubscribed literal/length tree";

/* 0x0069e290 */
char sIncompleteLiteralLengthTree[32] = "incomplete literal/length tree";

/* 0x0069e2b0 */
char sOversubscribedDistanceTree[32] = "oversubscribed distance tree";

/* 0x0069e2d0 */
char sIncompleteDistanceTree[32] = "incomplete distance tree";

/* 0x0069e2f0 */
char sEmptyDistanceTreeWithLengths[48] = "empty distance tree with lengths";

/* 0x0069e320 */
unsigned char d_0069e320[4];

/* 0x0069e324 */
float d_0069e324[4] = {1.0f};

/* 0x0069e334 */
unsigned char d_0069e334[4];

/* 0x0069e338 */
float d_0069e338 = -1.0f;

/* 0x0069e33c */
float d_0069e33c = 1.0f;

/* 0x0069e340 */
float d_0069e340[2] = {0.99999f};

/* 0x0069e348 */
double d_0069e348 = 1.0;

/* 0x0069e350 */
float d_0069e350 = 1.0f;

/* 0x0069e354 */
float d_0069e354 = 1.0f;

/* 0x0069e358 */
unsigned char d_0069e358[4];

/* 0x0069e35c */
float d_0069e35c[3] = {0, 1.0f};

/* 0x0069e368 */
float d_0069e368[3] = {0, 1.0f};

/* 0x0069e374 */
unsigned char d_0069e374[4];

/* 0x0069e378 */
unsigned char d_0069e378[4];

/* 0x0069e37c */
float d_0069e37c[13] = {0, 1.0f, 1.0f, 1.0f};

/* 0x0069e3b0 */
float d_0069e3b0[5] = {2.0f, 0, 0, 0, 1.0f};

/* 0x0069e3c4 */
unsigned char d_0069e3c4[4];

/* 0x0069e3c8 */
unsigned char d_0069e3c8[4];

/* 0x0069e3cc */
unsigned char d_0069e3cc[4];

/* 0x0069e3d0 */
unsigned char d_0069e3d0[4];

/* 0x0069e3d4 */
float d_0069e3d4 = -1.0f;

/* 0x0069e3d8 */
unsigned char d_0069e3d8[4];

/* 0x0069e3dc */
unsigned char d_0069e3dc[4];

/* 0x0069e3e0 */
float d_0069e3e0 = -1.0f;

/* 0x0069e3e4 */
unsigned char d_0069e3e4[4];

/* 0x0069e3e8 */
unsigned char d_0069e3e8[4];

/* 0x0069e3ec */
float d_0069e3ec = -1.0f;

/* 0x0069e3f0 */
float d_0069e3f0 = -1.0f;

/* 0x0069e3f4 */
float d_0069e3f4 = 1.0f;

/* 0x0069e3f8 */
float d_0069e3f8[19] = {
    [0] = 1.0f, [3] = 1.0f, [5] = 1.0f, [7] = 1.0f, [10] = -1.0f, [14] = -1.0f, [18] = -1.0f};

/* 0x0069e444 */
float d_0069e444 = 1.0f;

/* 0x0069e448 */
float d_0069e448 = 1.0f;

/* 0x0069e44c */
float d_0069e44c = 1.0f;

/* 0x0069e450 */
float d_0069e450 = 1.0f;

/* 0x0069e454 */
float d_0069e454 = 1.0f;

/* 0x0069e458 */
float d_0069e458 = 1.0f;

/* 0x0069e45c */
float d_0069e45c = 1.0f;

/* 0x0069e460 */
float d_0069e460 = 1.0f;

/* 0x0069e464 */
float d_0069e464 = 1.0f;

/* 0x0069e468 */
float d_0069e468 = 1.0f;

/* 0x0069e46c */
float d_0069e46c = 1.0f;

/* 0x0069e470 */
float d_0069e470[4] = {1.0f};

/* 0x0069e480 */
char sEntryNumberNotWithinListBounds[48] = "Entry number not within list bounds";

/* 0x0069e4b0 */
char sAttemptingToRemoveMoreEntriesThanExist[48] = "Attempting to remove more entries than exist";

/* 0x0069e4e0 */
char sSListIndex[2982] = "SList index";

/* 0x006a0000 */
unsigned char d_006a0000[4];

/* 0x006a0004 */
unsigned char d_006a0004[261];
