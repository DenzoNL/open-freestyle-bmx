/*
 * The original's globals of the PC layer (src/bmx/pc), with their initial values (docs/PORTING.md
 * X2a; types and declarations in src/data/globals.h).
 *
 * Generated once by tools/gen_data.py from NgBMX.exe; from here on it is source like the code.
 */
#include "data/data.h"

/* 0x0055e820 */
BmxType_gSemitones gSemitones = {
    512,   542,   574,   608,   645,   683,   724,   767,   812,   861,   912,   966,   1024,  1084,  1149,
    1217,  1290,  1366,  1448,  1534,  1625,  1722,  1824,  1933,  2048,  2169,  2298,  2435,  2580,  2733,
    2896,  3068,  3250,  3444,  3649,  3866,  4096,  4339,  4597,  4870,  5160,  5467,  5792,  6137,  6501,
    6888,  7298,  7732,  8192,  8679,  9195,  9741,  10321, 10935, 11585, 12274, 13003, 13777, 14596, 15464,
    16384, 17358, 18390, 19483, 20642, 21870, 23170, 24548, 26007, 27554, 29192, 30928, 32768};

/* 0x0055e8b4 */
BmxType_gCents gCents = {
    4096, 4098, 4100, 4103, 4105, 4107, 4110, 4112, 4114, 4117, 4119, 4122, 4124, 4126, 4129, 4131, 4134,
    4136, 4138, 4141, 4143, 4145, 4148, 4150, 4153, 4155, 4157, 4160, 4162, 4165, 4167, 4170, 4172, 4174,
    4177, 4179, 4182, 4184, 4186, 4189, 4191, 4194, 4196, 4199, 4201, 4203, 4206, 4208, 4211, 4213, 4216,
    4218, 4220, 4223, 4225, 4228, 4230, 4233, 4235, 4237, 4240, 4242, 4245, 4247, 4250, 4252, 4255, 4257,
    4260, 4262, 4265, 4267, 4269, 4272, 4274, 4277, 4279, 4282, 4284, 4287, 4289, 4292, 4294, 4297, 4299,
    4302, 4304, 4307, 4309, 4312, 4314, 4317, 4319, 4322, 4324, 4327, 4329, 4332, 4334, 4337};

/* 0x0055e97c */
BmxType_gGainCurve gGainCurve = {
    0,       0.031f,  0.0319f, 0.0328f, 0.0337f, 0.0347f, 0.0356f, 0.0366f, 0.0376f, 0.0387f, 0.0398f,
    0.0409f, 0.042f,  0.0432f, 0.0444f, 0.0457f, 0.0469f, 0.0482f, 0.0496f, 0.051f,  0.0524f, 0.0539f,
    0.0554f, 0.0569f, 0.0585f, 0.0601f, 0.0618f, 0.0636f, 0.0653f, 0.0672f, 0.069f,  0.071f,  0.0729f,
    0.075f,  0.0771f, 0.0792f, 0.0814f, 0.0837f, 0.0861f, 0.0885f, 0.0909f, 0.0935f, 0.0961f, 0.0988f,
    0.1015f, 0.1044f, 0.1073f, 0.1103f, 0.1134f, 0.1165f, 0.1198f, 0.1231f, 0.1266f, 0.1301f, 0.1337f,
    0.1375f, 0.1413f, 0.1453f, 0.1493f, 0.1535f, 0.1578f, 0.1622f, 0.1667f, 0.1714f, 0.1762f, 0.1811f,
    0.1862f, 0.1914f, 0.1967f, 0.2022f, 0.2079f, 0.2137f, 0.2196f, 0.2258f, 0.2321f, 0.2386f, 0.2452f,
    0.2521f, 0.2591f, 0.2664f, 0.2738f, 0.2815f, 0.2893f, 0.2974f, 0.3057f, 0.3143f, 0.3231f, 0.3321f,
    0.3414f, 0.3509f, 0.3607f, 0.3708f, 0.3811f, 0.3918f, 0.4027f, 0.414f,  0.4256f, 0.4375f, 0.4497f,
    0.4622f, 0.4752f, 0.4884f, 0.5021f, 0.5161f, 0.5305f, 0.5454f, 0.5606f, 0.5763f, 0.5924f, 0.6089f,
    0.6259f, 0.6434f, 0.6614f, 0.6799f, 0.6989f, 0.7184f, 0.7385f, 0.7591f, 0.7803f, 0.8021f, 0.8246f,
    0.8476f, 0.8713f, 0.8956f, 0.9206f, 0.9464f, 0.9728f, 1.0f};

struct BmxGroup_gDsVolumeGroup gDsVolumeGroup __attribute__((aligned(16))) = {
    .gEnvelopeMs = {0x20001,    0x40003,    0x60005,    0x90008,    0xe000b,    0x150012,   0x1e001a,
                    0x290023,   0x36002f,   0x44003d,   0x55004d,   0x68005e,   0x7e0073,   0x950089,
                    0xaf00a2,   0xcc00be,   0xec00dc,   0x10e00fd,  0x1330120,  0x15b0147,  0x1860171,
                    0x1b5019d,  0x1e701cd,  0x21b0201,  0x2540237,  0x2900272,  0x2d002af,  0x31302f1,
                    0x35a0336,  0x3a5037f,  0x3f503cd,  0x448041e,  0x4a60473,  0x53104e5,  0x5f2058b,
                    0x6e60666,  0x8100775,  0x96e08b8,  0xb010a31,  0xcc90bde,  0xec60dc1,  0x10f70fd8,
                    0x135f1224, 0x15fb14a6, 0x18cd175d, 0x1bd41a4a, 0x1f111d6b, 0x228320c3, 0x262b2450,
                    0x2a092813, 0x2e1d2c0d, 0x3267303c, 0x36e834a1, 0x3b9e393c, 0x408b3e0e, 0x45ae4316,
                    0x4b084854, 0x50984dc9, 0x565f5375, 0x5c5d5957, 0x62925f71, 0x68fe65c1, 0x6fa16c48,
                    30000},
    .gDsVolume = {-10000, -4200, -4083, -4025, -3967, -3909, -3851, -3793, -3735, -3677, -3619, -3561, -3503,
                  -3445,  -3387, -3329, -3270, -3212, -3154, -3096, -3038, -2980, -2922, -2864, -2806, -2748,
                  -2690,  -2632, -2574, -2516, -2458, -2400, -2350, -2297, -2246, -2197, -2149, -2102, -2057,
                  -2013,  -1969, -1927, -1886, -1846, -1807, -1769, -1731, -1694, -1658, -1623, -1589, -1555,
                  -1522,  -1490, -1458, -1426, -1396, -1365, -1336, -1307, -1278, -1250, -1222, -1195, -1168,
                  -1142,  -1116, -1090, -1065, -1040, -1015, -991,  -967,  -944,  -920,  -898,  -875,  -853,
                  -831,   -809,  -788,  -766,  -745,  -725,  -704,  -684,  -664,  -644,  -625,  -606,  -587,
                  -568,   -549,  -531,  -513,  -494,  -477,  -459,  -441,  -424,  -407,  -390,  -373,  -357,
                  -340,   -324,  -308,  -292,  -276,  -260,  -245,  -229,  -214,  -199,  -184,  -169,  -154,
                  -139,   -125,  -110,  -96,   -82,   -68,   -54,   -40,   -27,   -13},
};

/* 0x0055ed7c */
uint32_t d_0055ed7c[71] = {1000, 0x42fe0000};

/* 0x0055ee98 */
char sInitHWnd[16] = "init->hWnd";

/* 0x0055eea8 */
char sCZaxisBmxSrcPcZgeSoundWinChanC[48] = "C:\\zaxis\\Bmx\\src\\Pc\\zge\\sound\\winChan.c";

/* 0x0055eed8 */
char sDirectSoundCreate[24] = "DirectSoundCreate";

/* 0x0055eef0 */
char sDSOUNDDLL[16] = "DSOUND.DLL";

/* 0x0055ef00 */
char sFzsndcInitialized[24] = "fzsndcInitialized";

/* 0x0055ef18 */
char sIFzsndcNextChannelHandleNum[40] = "i != fzsndcNextChannelHandleNum";

/* 0x0055ef40 */
char sSample[8] = "sample";

/* 0x0055ef48 */
char sFzsndcHandleHChanChannelNumKzsndNumChan[60] = "fzsndcHandle[hChan].channelNum < kzsndNumChannels";

/* 0x0055ef84 */
char sFzsndcHandleHChanAllocated[36] = "fzsndcHandle[hChan].allocated";

/* 0x0055efa8 */
char sHChanKzsndNumChannelHandles[40] = "hChan < kzsndNumChannelHandles";

/* 0x0055efd0 */
char sFileNameTooLong[24] = "file name too long";

/* 0x0055efe8 */
char sCZaxisBmxSrcPcZgeSoundWinLoadC[48] = "C:\\zaxis\\Bmx\\src\\Pc\\zge\\sound\\winLoad.c";

/* 0x0055f018 */
char sInvalidSampleFileNumber[32] = "invalid sample file number";

/* 0x0055f038 */
char sInvalidAudioAssetFilePlatform[40] = "invalid audio asset file platform";

/* 0x0055f060 */
char sWIN[8] = "WIN ";

/* 0x0055f068 */
char sInvalidAudioAssetFileHeader[40] = "invalid audio asset file header";

/* 0x0055f090 */
char sZSND[8] = "ZSND";

/* 0x0055f098 */
char sTooManyAudioAssetFiles[32] = "too many audio asset files";

/* 0x0055f0b8 */
char sFileNotLoaded[20] = "file not loaded";

/* 0x0055f0cc */
char sInvalidFileHandle[24] = "invalid file handle";

/* 0x0055f0e4 */
char sInvalidVoiceNumber[28] = "invalid voice number";

/* 0x0055f100 */
char sSampleNumberOutOfRange[32] = "sample number out of range";

/* 0x0055f120 */
char sSampleFileNumberOutOfRange[40] = "sample file number out of range";

/* 0x0055f148 */
char sPhraseNumberOutOfRange[32] = "phrase number out of range";

/* 0x0055f168 */
char sTrackDefNumberOutOfRange[36] = "track def number out of range";

/* 0x0055f18c */
char sKeymapNumberOutOfRange[32] = "keymap number out of range";

/* 0x0055f1ac */
BmxType_gAdpcmIndex gAdpcmIndex = {-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8};

struct BmxGroup_gAdpcmStepGroup gAdpcmStepGroup __attribute__((aligned(16))) = {
    .gAdpcmStep = {7,     8,     9,     10,    11,    12,    13,    14,    16,    17,    19,   21,    23,
                   25,    28,    31,    34,    37,    41,    45,    50,    55,    60,    66,   73,    80,
                   88,    97,    107,   118,   130,   143,   157,   173,   190,   209,   230,  253,   279,
                   307,   337,   371,   408,   449,   494,   544,   598,   658,   724,   796,  876,   963,
                   1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,  2272,  2499,  2749, 3024,  3327,
                   3660,  4026,  4428,  4871,  5358,  5894,  6484,  7132,  7845,  8630,  9493, 10442, 11487,
                   12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767},
};

/* 0x0055f2d0 */
char sFzsndStreamInitialized[28] = "!fzsndStreamInitialized";

/* 0x0055f2ec */
char sCZaxisBmxSrcPcZgeSoundWinStrmC[48] = "C:\\zaxis\\Bmx\\src\\Pc\\zge\\sound\\winStrm.c";

/* 0x0055f31c */
char sFzsndStreamInitialized_0055f31c[28] = "fzsndStreamInitialized";

/* 0x0055f338 */
char sHResultDSOK[20] = "hResult == DS_OK";

/* 0x0055f34c */
char sDwLen20[16] = "dwLen2 == 0";

/* 0x005fb760 */
DsBuffer *gWinchanBuffer;

/* 0x005fb764 */
int32_t gWinchanFlangerDelay;

/* 0x005fb768 */
int32_t gWinchanActive;

/* 0x005fb76c */
uint32_t gWinchanEffectMaxFrames;

/* 0x005fb770 */
WinchanChannelTable gWinchanChannels;

/* 0x005fbe30 */
int32_t gWinchanMixRight;

/* 0x005fbe34 */
float gWinchanEffectAGain;

struct BmxGroup_gWinchanMixLeftGroup gWinchanMixLeftGroup __attribute__((aligned(16)));

/* 0x005fbebc */
WinchanFlangerLine gWinchanFlangerLine;

/* 0x005fdebc */
uint32_t gWinchanCopyOffset;

/* 0x005fdec0 */
uint32_t gWinchanWriteOffset;

/* 0x005fdec4 */
int32_t gWinchanEchoPos;

/* 0x005fdec8 */
WinchanLfoTable gWinchanLfoTable;

/* 0x005ffec8 */
unsigned char d_005ffec8[4];

/* 0x005ffecc */
int32_t gWinchanFlangerPhase;

/* 0x005ffed0 */
WinchanFrame *gWinchanEchoBuffer;

/* 0x005ffed4 */
void *gWinchanDsoundModule;

/* 0x005ffed8 */
uint8_t *gWinchanRing;

/* 0x005ffedc */
uint32_t gWinchanChunkBytes;

/* 0x005ffee0 */
uint32_t gWinchanEffectBFrames;

/* 0x005ffee4 */
uint32_t gWinchanRate;

/* 0x005ffee8 */
float gWinchanEffectBGain;

/* 0x005ffeec */
uint32_t gWinchanEffectAFrames;

/* 0x005ffef0, also gWinchanEffectAParam */
float gWinchanEchoFeedback;

/* 0x005ffef4 */
uint32_t gWinchanMixOffset;

/* 0x005ffef8 */
uint32_t gWinchanLastPlay;

/* 0x005ffefc */
int32_t gWinchanEffectBTailStart;

/* 0x005fff00 */
int32_t gWinchanFlangerPos;

/* 0x005fff04 */
int32_t gWinchanEffectBTail;

/* 0x005fff08 */
uint32_t gWinchanRingBytes;

/* 0x005fff0c */
int32_t gWinchanInitialized;

/* 0x005fff10 */
int32_t gWinchanLock;

/* 0x005fff14 */
int32_t gWinchanNextChannel;

/* 0x005fff18 */
int32_t gWinchanNextHandle;

/* 0x005fff1c */
void *gWinchanDsoundCreate;

/* 0x005fff20 */
DsBuffer *gWinchanPrimary;

/* 0x005fff24 */
int32_t gWinchanPumpCount;

struct BmxGroup_gWinstrmDirectSoundGroup gWinstrmDirectSoundGroup __attribute__((aligned(16)));

/* 0x00600d88 */
WinloadSampleFile gWinloadSampleFile;

/* 0x00600d98 */
WinloadFileTable gWinloadFiles;

struct BmxGroup_gWinloadCountsGroup gWinloadCountsGroup __attribute__((aligned(16)));

/* 0x00601328 */
int32_t gWinstrmSourceHint;

/* 0x00601330 */
WinstrmLockStorage gWinstrmLock;

/* 0x00601348 */
int32_t gWinstrmStreamHint;

/* 0x00601350 */
WinstrmSourceTable gWinstrmSources;

/* 0x006017d0 */
WinstrmStreamTable gWinstrmStreams;

/* 0x006019f0 */
int32_t gWinstrmNodeHint;

/* 0x006019f8 */
WinstrmNodeTable gWinstrmNodes;

/* 0x00601b78 */
int32_t gWinstrmInitialized;

/* 0x00601b7c */
int32_t gWinstrmPaused;

struct BmxGroup_gWinstrmTickGroup gWinstrmTickGroup __attribute__((aligned(16)));
