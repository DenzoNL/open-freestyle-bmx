/*
 * The original's globals of the game's engine layer (src/bmx/com/zge), with their initial values
 * (docs/PORTING.md X2a; types and declarations in src/data/globals.h).
 *
 * Generated once by tools/gen_data.py from NgBMX.exe; from here on it is source like the code.
 */
#include "data/data.h"

/* 0x0055b530 */
int32_t gBmxSoundState = -1;

/* 0x0055b534 */
int32_t gBmxMusicMode = -1;

/* 0x0055b538, also gBmxMusicVolume, gBmxSfxVolume */
BmxVolumeTable gBmxVolumes = {127, 127, 127};

/* 0x0055b53c */
uint16_t gBmxSfxFile = 65535;

/* 0x0055b53e */
uint16_t gBmxMenuSfxFile = 65535;

/* 0x0055b540 */
uint16_t gBmxGameSfxFile = 65535;

/* 0x0055b542 */
uint16_t gBmxGunSfxFile = 65535;

/* 0x0055b544 */
uint16_t gBmxRiderSfxFile = 65535;

/* 0x0055b546 */
uint16_t gBmxLevelSfxFile = 65535;

/* 0x0055b548 */
uint16_t gBmxMusicFile = 65535;

/* 0x0055b54a */
uint16_t gBmxSpeechFile = 65535;

/* 0x0055b54c */
uint8_t gBmxNextTrack = 255;

/* 0x0055b54d */
uint8_t gBmxTrackLoop = 1;

/* 0x0055b550 */
BmxLevelTrackTable gBmxLevelTracks = {7, 1, 5, 2, 3, 4, 8, 6, 0, 9, 7, 6};

/* 0x0055b564 */
char sErrorGettingTimerDeviceCapabilities[48] = "error getting timer device capabilities";

/* 0x0055b594 */
char sErrorCreatingTimerEvent[32] = "error creating timer event";

/* 0x0055b5b4 */
char sFailureLoadingSpeechAssetFile[40] = "failure loading speech asset file";

/* 0x0055b5dc */
char sSPEECHZSD[16] = "SPEECH.ZSD";

/* 0x0055b5ec */
char sFailureLoadingMusicAssetFile[40] = "failure loading music asset file";

/* 0x0055b614 */
char sMUSICZSD[12] = "MUSIC.ZSD";

/* 0x0055b620 */
char sFailureLoadingCommonSFXAssetFile[48] = "failure loading common SFX asset file";

/* 0x0055b650 */
char sBinAudio[16] = "bin\\audio\\";

/* 0x0055b660 */
char sSFXCOMZSD[16] = "SFXCOM.ZSD";

/* 0x0055b670 */
char sFailureInitializing3DSoundSourceManager[56] = "failure initializing 3-d sound source manager";

/* 0x0055b6a8 */
char sFailureInitializingSequencedAudioManage[56] = "failure initializing sequenced audio manager";

/* 0x0055b6e0 */
char sFailureInitializingChannelManager[44] = "failure initializing channel manager";

/* 0x0055b70c */
char sSoundAlreadyOpen[24] = "sound already open";

/* 0x0055b724 */
char sCZaxisBmxSrcComZgeBmxBmxsoundC[48] = "C:\\zaxis\\Bmx\\src\\com\\zge\\bmx\\bmxsound.c";

/* 0x0055b754 */
char sSoundAlreadyClosed[28] = "sound already closed";

/* 0x0055b770 */
char sPropLevelSoundTypeOutOfRange[44] = "prop/level sound type out of range";

/* 0x0055b79c */
char sNULLPointerPassedAsPositionalSFXArgumen[56] = "NULL pointer passed as positional SFX argument";

/* 0x0055b7d4 */
char sFailureLoadingMenuSFXAssetFile[44] = "failure loading menu SFX asset file";

/* 0x0055b800 */
char sSFXMEN02dZSD[20] = "SFXMEN%02d.ZSD";

/* 0x0055b814 */
char sFailureCreatingMusicStreamProbablyOutOf[68] = "failure creating music stream (probably out of SPU RAM)";

/* 0x0055b858 */
char sFailureLoadingCharacterSFXAssetFile[52] = "failure loading character SFX asset file";

/* 0x0055b88c */
char sSFXCHR02dZSD[20] = "SFXCHR%02d.ZSD";

/* 0x0055b8a0 */
char sFailureLoadingLevelSFXAssetFile[44] = "failure loading level SFX asset file";

/* 0x0055b8cc */
char sFailureLoadingGunSFXAssetFile[44] = "failure loading gun SFX asset file";

/* 0x0055b8f8 */
char sSFXGAME2ZSD[16] = "SFXGAME2.ZSD";

/* 0x0055b908 */
char sSFXLEV02dZSD[20] = "SFXLEV%02d.ZSD";

/* 0x0055b91c */
char sFailureLoadingGameSFXAssetFile[44] = "failure loading game SFX asset file";

/* 0x0055b948 */
char sSFXGAMEZSD[16] = "SFXGAME.ZSD";

/* 0x0055b958 */
float d_0055b958 = 1.58f;

/* 0x0055b95c */
float d_0055b95c = -10.0f;

struct BmxGroup_gZoverGlyphGroup gZoverGlyphGroup __attribute__((aligned(16))) = {
    .gZoverGlyph = {.pad00 = {49}},
    .d_0055b974 = {255},
};

/* 0x0055b97c */
char sDirtyListOverflow[24] = "dirty list overflow";

/* 0x0055b994 */
char sFreeListOverflow[24] = "free list overflow";

/* 0x0055b9ac */
char sCZaxisBmxSrcComZgeBmxZoveriiC[48] = "C:\\zaxis\\Bmx\\src\\com\\zge\\bmx\\zoverii.c";

/* 0x0055b9dc */
char sNullFontInOverlay[28] = "Null font in overlay";

/* 0x0055b9f8 */
char sOverflow[12] = "Overflow";

struct BmxGroup_gNgAssetsDirGroup gNgAssetsDirGroup __attribute__((aligned(16))) = {
    .gNgAssetsDir = sAssets,
    .p_0055ba08 =
        {
            &sMotionZal,
            &sGeometryZal,
            &sMaterialZal,
            &sMiscdataZal,
            &sWSsndZal,
            &sSystemZal,
            &sTablesZal,
            &sReplayZal,
        },
};

/* 0x0055ba30 */
char sReplayZal[16] = "replay.zal";

/* 0x0055ba40 */
char sTablesZal[16] = "Tables.zal";

/* 0x0055ba50 */
char sSystemZal[16] = "system.zal";

/* 0x0055ba60 */
char sWSsndZal[12] = "WSsnd.zal";

/* 0x0055ba6c */
char sMiscdataZal[16] = "miscdata.zal";

/* 0x0055ba7c */
char sMaterialZal[16] = "material.zal";

/* 0x0055ba8c */
char sGeometryZal[16] = "geometry.zal";

/* 0x0055ba9c */
char sMotionZal[16] = "motion.zal";

/* 0x0055baac */
char sAssets[12] = "assets\\";

/* 0x0055bab8 */
char sSDS[12] = "%s %d %s\n";

/* 0x0055bac4 */
char sCZaxisBmxSrcComZgeNgIFMissingZGEC[52] = "C:\\zaxis\\Bmx\\src\\com\\zge\\NgIF\\MissingZGE.c";

/* 0x0055baf8 */
char sZconsoleLog[16] = "zconsoleLog";

/* 0x0055bb08 */
char sEnableHorizon[20] = "Enable Horizon";

/* 0x0055bb1c */
char sZglDisableHorizon[24] = "zgl disable horizon";

/* 0x0055bb34 */
char sZsystemSetActionMode[28] = "zsystemSetActionMode()";

/* 0x0055bb50 */
char sZsystemSetMenuMode[24] = "zsystemSetMenuMode";

/* 0x0055bb68 */
char sZfxDestroyAll[16] = "zfxDestroyAll";

/* 0x0055bb78 */
char sZfxRegisterFuncTable[28] = "zfxRegisterFuncTable";

/* 0x0055bb94 */
char sAVI[4] = "AVI";

struct BmxGroup_gNgRamDiskGroup gNgRamDiskGroup __attribute__((aligned(16))) = {
    .gNgKeyMaps = {0x5cd04cb, 0x7d006c8, 0x91f0812, 0xb200a21, 0xf130e11, 0xd1c0c01, 0xff1439,  0,
                   0x54d044b, 0x74c0648, 0x91f0839, 0xb520a20, 0xf210e1e, 0xd1c0c29, 0x6c81401, 0xff07d0,
                   0x520041e, 0x71f0611, 0x94b0848, 0xb4c0a4d, 0xf490e47, 0xd1c0c4e, 0x6c81401, 0xff07d0,
                   0x5cd04cb, 0x7d006c8, 0x94b084c, 0xb9d0a4d, 0xf9c0e52, 0xd1c0ccf, 0xff1401,  0,
                   0x520041e, 0x71f0611, 0x9d208c7, 0xb390ac9, 0xfd10ed3, 0xd1c0c29, 0x6c81401, 0xff07d0},
    .gNgRamDisk = 0xffffffff,
    .gNgLegalPending = 1,
    .gNgWrapName = sIntwrapZal,
    .gNgLevelNames =
        {
            &sGreenvilleDirt,
            &sGreenvilleStreet,
            &sGreenvilleVert,
            &sWoodwardDirt,
            &sWoodwardStreet,
            &sWoodwardVert,
            &sProTourDirt,
            &sProTourStreet,
            &sProTourVert,
            &sAcclaimDirt,
            &sAcclaimStreet,
            &sAcclaimVert,
            &sTestA,
            &sTestB,
        },
    .gNgDebugFaceColor = 0x80000080,
    .gNgDebugEdgeColor = 0x80008000,
    .gNgDebugTaggedColor = 0x80800000,
    .gNgDebugRollerColor = 0x80808000,
    .gNgMainLightColor = 128,
    .d_0055bc8d = {128, 128},
    .gNgLightDir =
        {
            {.v = {.x = -5200.0f, .y = -7200.0f}},
            {.v = {.x = 4218.0f, .y = -9100.0f, .z = 3408.0f}},
            {.v = {.x = 5600.0f, .y = 2400.0f}},
        },
    .gNgLightRef =
        {
            {.v = {.x = -5148.0f, .y = -7116.0f}},
            {.v = {.x = 4170.0f, .y = -9022.0f, .z = 3366.0f}},
            {.v = {.x = 5522.0f, .y = 2340.0f, .z = 20.0f}},
        },
};

/* 0x0055bcf0 */
BmxType_gNgLightColors gNgLightColors = {
    {153, 173, 180},
    {249, 247, 227},
    {177, 153, 153},
};

struct BmxGroup_gNgSpotMainColorGroup gNgSpotMainColorGroup __attribute__((aligned(16))) = {
    .gNgSpotMainColor = 48,
    .d_0055bcfd = {48, 48, 0, 1},
};

/* 0x0055bd04 */
uint32_t gNgLegalTime = 0xffffffff;

/* 0x0055bd08 */
float gFxQuality = 200.0f;

struct BmxGroup_gFxFpsGroup gFxFpsGroup __attribute__((aligned(16))) = {
    .gFxFps = 25.0f,
};

/* 0x0055bd5c */
char sTestB[8] = "Test B";

/* 0x0055bd64 */
char sTestA[8] = "Test A";

/* 0x0055bd6c */
char sAcclaimVert[16] = "Acclaim Vert";

/* 0x0055bd7c */
char sAcclaimStreet[20] = "Acclaim Street";

/* 0x0055bd90 */
char sAcclaimDirt[16] = "Acclaim Dirt";

/* 0x0055bda0 */
char sProTourVert[16] = "ProTour Vert";

/* 0x0055bdb0 */
char sProTourStreet[20] = "ProTour Street";

/* 0x0055bdc4 */
char sProTourDirt[16] = "ProTour Dirt";

/* 0x0055bdd4 */
char sWoodwardVert[16] = "Woodward Vert";

/* 0x0055bde4 */
char sWoodwardStreet[20] = "Woodward Street";

/* 0x0055bdf8 */
char sWoodwardDirt[16] = "Woodward Dirt";

/* 0x0055be08 */
char sGreenvilleVert[20] = "Greenville Vert";

/* 0x0055be1c */
char sGreenvilleStreet[24] = "Greenville Street";

/* 0x0055be34 */
char sGreenvilleDirt[20] = "Greenville Dirt";

/* 0x0055be48 */
char sIntwrapZal[16] = "intwrap.zal";

/* 0x0055be58 */
char s155417[12] = "15:54:17";

/* 0x0055be64 */
char s_0055be64[4] = ",";

/* 0x0055be68 */
char sNov222000[16] = "Nov 22 2000";

/* 0x0055be78 */
char sNextGenBMX[20] = "NextGen BMX : ";

/* 0x0055be8c */
char sLight[8] = "light_";

/* 0x0055be94 */
char sSpotLightT[16] = "SpotLightT";

/* 0x0055bea4 */
char sSpotLight[12] = "SpotLight";

/* 0x0055beb0 */
char sCouldNotGenerateFileNameForRAMDisk[52] = "Could not generate file name for RAM disk";

/* 0x0055bee4 */
char sSIZE[8] = "SIZE";

/* 0x0055beec */
char sADDRESS[12] = "ADDRESS";

/* 0x0055bef8 */
char sCouldNotLocateARAMDiskDeviceOnThisSyste[64] = "Could not locate a RAM disk device on this system!";

/* 0x0055c0d8 */
const char *gFxSkidBase = sFAN1;

/* 0x0055c0dc */
BmxType_gFxSkidFrameNames gFxSkidFrameNames = {
    sFAN1,
    sFAN2,
    sFAN3,
    gGameRoundStatesGroup.gOvlEmpty,
    gGameRoundStatesGroup.gOvlEmpty,
    gGameRoundStatesGroup.gOvlEmpty,
    gGameRoundStatesGroup.gOvlEmpty,
    gGameRoundStatesGroup.gOvlEmpty,
    gGameRoundStatesGroup.gOvlEmpty,
    gGameRoundStatesGroup.gOvlEmpty,
    gGameRoundStatesGroup.gOvlEmpty,
    gGameRoundStatesGroup.gOvlEmpty,
    gGameRoundStatesGroup.gOvlEmpty,
    gGameRoundStatesGroup.gOvlEmpty,
    gGameRoundStatesGroup.gOvlEmpty,
    gGameRoundStatesGroup.gOvlEmpty,
};

/* 0x0055c11c */
const char *gFxSkidName = sANIM;

/* 0x0055c120 */
uint32_t gFxSkidConfig = 798;

struct BmxGroup_gFxSparkDefaultGroup gFxSparkDefaultGroup __attribute__((aligned(16))) = {
    .gFxSparkDefault = {[8] = 10.0f, [9] = 10.0f, [10] = 2.803e-42f, [11] = 6.3382526e+29f},
    .d_0055c168 = {[10] = 0x41000000, [11] = 2000, [12] = 0x70af3f3f},
};

struct BmxGroup_gFxDustDefaultGroup gFxDustDefaultGroup __attribute__((aligned(16))) = {
    .gFxDustDefault = {[5] = -10.0f, [8] = 1.0f, [9] = 2.803e-42f},
    .d_0055c1e0 = {[12] = 5000, [13] = 0x70ffffff},
};

struct BmxGroup_gFxRippleDefaultGroup gFxRippleDefaultGroup __attribute__((aligned(16))) = {
    .gFxRippleDefault = {[4] = 4.204e-42f, [5] = 10.0f, [6] = 5.0f, [7] = 6.3382526e+29f, [15] = 100.0f},
    .d_0055c260 = {0, 0, 8, 0x3f800000, 0x41200000},
    .gFxSpark =
        {
            .destroy = zobjPEmitEffectDestroy,
            .pad04 = (uintptr_t)zobjPEmitAddModifier,
            .attach = (void *)zobjPEmitAddModifierByName,
            .pad0c =
                {
                    (uintptr_t)zobjPEmitClearModifiers,
                    (uintptr_t)zobjPEmitCreate,
                    (uintptr_t)zobjPEmitDestroy,
                    (uintptr_t)ngiffxSparkSpawn,
                    (uintptr_t)zobjPEmitUpdate,
                    (uintptr_t)ngiffxSparkRender,
                },
        },
};

/* 0x0055c2b0 */
ZfxEffect gFxShard = {
    .destroy = zobjPEmitEffectDestroy,
    .pad04 = (uintptr_t)zobjPEmitAddModifier,
    .attach = (void *)zobjPEmitAddModifierByName,
    .pad0c =
        {
            (uintptr_t)zobjPEmitClearModifiers,
            (uintptr_t)zobjPEmitCreate,
            (uintptr_t)zobjPEmitDestroy,
            (uintptr_t)ngiffxShardSpawn,
            (uintptr_t)zobjPEmitUpdate,
            (uintptr_t)ngiffxShardRender,
        },
};

/* 0x0055c2e8 */
ZfxEffect gFxSplash = {
    .destroy = zobjPEmitEffectDestroy,
    .pad04 = (uintptr_t)zobjPEmitAddModifier,
    .attach = (void *)zobjPEmitAddModifierByName,
    .pad0c =
        {
            (uintptr_t)zobjPEmitClearModifiers,
            (uintptr_t)zobjPEmitCreate,
            (uintptr_t)zobjPEmitDestroy,
            (uintptr_t)ngiffxSparkSpawn,
            (uintptr_t)zobjPEmitUpdate,
            (uintptr_t)ngiffxSplashRender,
        },
};

/* 0x0055c320 */
ZfxEffect gFxDust = {
    .destroy = zobjPEmitEffectDestroy,
    .pad04 = (uintptr_t)zobjPEmitAddModifier,
    .attach = (void *)zobjPEmitAddModifierByName,
    .pad0c =
        {
            (uintptr_t)zobjPEmitClearModifiers,
            (uintptr_t)zobjPEmitCreate,
            (uintptr_t)zobjPEmitDestroy,
            (uintptr_t)ngiffxDustSpawn,
            (uintptr_t)zobjPEmitUpdate,
            (uintptr_t)ngiffxDustRender,
        },
};

/* 0x0055c358 */
ZfxEffect gFxPoster = {
    .destroy = zobjPEmitEffectDestroy,
    .pad04 = (uintptr_t)zobjPEmitAddModifier,
    .attach = (void *)zobjPEmitAddModifierByName,
    .pad0c =
        {
            (uintptr_t)zobjPEmitClearModifiers,
            (uintptr_t)ngiffxPosterCreate,
            (uintptr_t)zobjPEmitDestroy,
            (uintptr_t)ngiffxPosterSpawn,
            (uintptr_t)zobjPEmitUpdate,
            (uintptr_t)ngiffxPosterRender,
        },
};

/* 0x0055c390 */
ZfxEffect gFxRipple = {
    .destroy = zobjPEmitEffectDestroy,
    .pad04 = (uintptr_t)zobjPEmitAddModifier,
    .attach = (void *)zobjPEmitAddModifierByName,
    .pad0c =
        {
            (uintptr_t)zobjPEmitClearModifiers,
            (uintptr_t)zobjPEmitCreate,
            (uintptr_t)zobjPEmitDestroy,
            (uintptr_t)ngiffxRippleSpawn,
            (uintptr_t)ngiffxUpdateEmitter,
            (uintptr_t)ngiffxRippleRender,
        },
};

/* 0x0055c3c8 */
ZfxEffect gFxLightning = {
    .destroy = zobjPEmitEffectDestroy,
    .pad04 = (uintptr_t)zobjPEmitAddModifier,
    .attach = (void *)zobjPEmitAddModifierByName,
    .pad0c =
        {
            (uintptr_t)zobjPEmitClearModifiers,
            (uintptr_t)zobjPEmitCreate,
            (uintptr_t)zobjPEmitDestroy,
            (uintptr_t)ngiffxLightningSpawn,
            (uintptr_t)zobjPEmitUpdate,
            (uintptr_t)ngiffxLightningRender,
        },
};

struct BmxGroup_gFxSparkCreateDefaultGroup gFxSparkCreateDefaultGroup __attribute__((aligned(16))) = {
    .gFxSparkCreateDefault =
        {[5] = -10.0f, [8] = 0.5f, [9] = 0.25f, [10] = 2.803e-42f, [11] = 6.3382526e+29f},
    .d_0055c440 = {0, 0, 0, 0xc1200000, 0, 0, 0x3f000000, 0x3e800000, 2000, 0x70ffffff},
    .gFxGravityParams = {0, 16.0f},
};

/* 0x0055c53c */
char sANIM[8] = "ANIM";

/* 0x0055c544 */
char sFAN3[8] = "FAN3";

/* 0x0055c54c */
char sFAN2[8] = "FAN2";

/* 0x0055c554 */
char sFAN1[8] = "FAN1";

/* 0x0055c55c */
char sPoster[8] = "poster";

/* 0x0055c564 */
char sLightning[12] = "lightning";

/* 0x0055c570 */
char sDUST16[8] = "DUST16";

/* 0x0055c578 */
char sDUST15[8] = "DUST15";

/* 0x0055c580 */
char sDUST14[8] = "DUST14";

/* 0x0055c588 */
char sDUST13[8] = "DUST13";

/* 0x0055c590 */
char sDUST12[8] = "DUST12";

/* 0x0055c598 */
char sDUST11[8] = "DUST11";

/* 0x0055c5a0 */
char sDUST10[8] = "DUST10";

/* 0x0055c5a8 */
char sDUST09[8] = "DUST09";

/* 0x0055c5b0 */
char sDUST08[8] = "DUST08";

/* 0x0055c5b8 */
char sDUST07[8] = "DUST07";

/* 0x0055c5c0 */
char sDUST06[8] = "DUST06";

/* 0x0055c5c8 */
char sDUST05[8] = "DUST05";

/* 0x0055c5d0 */
char sDUST04[8] = "DUST04";

/* 0x0055c5d8 */
char sDUST03[8] = "DUST03";

/* 0x0055c5e0 */
char sDUST02[8] = "DUST02";

/* 0x0055c5e8 */
char sDUST01[8] = "DUST01";

/* 0x0055c5f0 */
char sSPLASH[8] = "SPLASH";

/* 0x0055c5f8 */
char sRIPPLE[8] = "RIPPLE";

/* 0x0055c600 */
char sSKID[8] = "SKID";

/* 0x0055c608 */
char sSPARK[8] = "SPARK";

/* 0x0055c610 */
char sSFXCube[12] = "SFXCube";

/* 0x0055c61c */
char sFloorcoll[12] = "floorcoll";

/* 0x0055c628 */
char sGravity[12] = "gravity";

/* 0x0055c634 */
char sCZaxisBmxSrcComZgeNgIFNgIFfxC[48] = "C:\\zaxis\\Bmx\\src\\com\\zge\\NgIF\\NgIFfx.c";

/* 0x0055c664 */
char sZfxCreateUnsupportedEffect[40] = "zfxCreate : Unsupported effect";

/* 0x0055c68c */
uint32_t gNgifBrightness = 255;

/* 0x0055c694 */
float gNgifScreenH = 480.0f;

/* 0x0055c698, also gNgifOtBase */
uint32_t *gZoverOt = (void *)((char *)&d_0068ec00 + 0);

/* 0x0055c6a0 */
char sCZaxisBmxSrcComZgeNgIFNgIFpsxC[48] = "C:\\zaxis\\Bmx\\src\\com\\zge\\NgIF\\NgIFpsx.c";

/* 0x0055c6d0 */
char sDrawPrimPrimNotCodedLINEG4[40] = "DrawPrim:  Prim not coded LINEG4";

/* 0x0055c6f8 */
char sDrawPrimPrimNotCodedLINEF4[40] = "DrawPrim:  Prim not coded LINEF4";

/* 0x0055c720 */
char sDrawPrimPrimNotCodedLINEG3[40] = "DrawPrim:  Prim not coded LINEG3";

/* 0x0055c748 */
char sDrawPrimPrimNotCodedLINEF3[40] = "DrawPrim:  Prim not coded LINEF3";

/* 0x0055c770 */
char sDrawPrimPrimNotCodedSPRT[40] = "DrawPrim:  Prim not coded SPRT";

/* 0x0055c798 */
char sDrawPrimPrimNotCodedSPRT16[40] = "DrawPrim:  Prim not coded SPRT16";

/* 0x0055c7c0 */
char sDrawPrimPrimNotCodedSPRT8[40] = "DrawPrim:  Prim not coded SPRT8";

/* 0x0055c7e8 */
char sDrawPrimPrimNotCodedGT3[36] = "DrawPrim:  Prim not coded GT3";

/* 0x0055c80c */
char sDrawPrimPrimNotCodedG3[36] = "DrawPrim:  Prim not coded G3";

/* 0x0055c830 */
char sDrawPrimPrimNotCodedFT3[36] = "DrawPrim:  Prim not coded FT3";

/* 0x0055c854 */
char sCannotGetSizeOfObject[32] = "Cannot get size of object";

/* 0x0055c874 */
char sCanNotGetSizeOfObject[32] = "Can not get size of object";

/* 0x0055c894 */
char sNotCodedForTexturedTris[36] = "Not coded for textured tris";

/* 0x0055c8b8 */
char sUncodedForPolygonsInScreenSpace[44] = "Uncoded for polygons in screen space";

/* 0x0055c8e4 */
char sInvalidPaletteIndex[28] = "Invalid palette index.";

/* 0x0055c900 */
char sBufferOverflowInPsxprimDrawFontChar[52] = "Buffer overflow in psxprimDrawFontChar()";

/* 0x0055c934 */
char sRightFoot[12] = "RightFoot";

/* 0x0055c940 */
char sRightLowLeg[16] = "RightLowLeg";

/* 0x0055c950 */
char sRightUpLeg[16] = "RightUpLeg";

/* 0x0055c960 */
char sLeftFoot[12] = "LeftFoot";

/* 0x0055c96c */
char sLeftLowLeg[16] = "LeftLowLeg";

/* 0x0055c97c */
char sLeftUpLeg[12] = "LeftUpLeg";

/* 0x0055c988 */
char sRightHand[12] = "RightHand";

/* 0x0055c994 */
char sRightLowArm[16] = "RightLowArm";

/* 0x0055c9a4 */
char sRightUpArm[16] = "RightUpArm";

/* 0x0055c9b4 */
char sLeftHand[12] = "LeftHand";

/* 0x0055c9c0 */
char sLeftLowArm[16] = "LeftLowArm";

/* 0x0055c9d0 */
char sLeftUpArm[12] = "LeftUpArm";

/* 0x0055c9dc */
char sHead[8] = "Head";

/* 0x0055c9e4 */
char sChest[8] = "Chest";

/* 0x0055c9ec */
char sHips[8] = "Hips";

/* 0x0055c9f4 */
char sSeatGrip[12] = "SeatGrip";

/* 0x0055ca00 */
char sRightGrip[12] = "RightGrip";

/* 0x0055ca0c */
char sLeftGrip[12] = "LeftGrip";

/* 0x0055ca18 */
char sFrontWheel[16] = "FrontWheel";

/* 0x0055ca28 */
char sBackWheel[12] = "BackWheel";

/* 0x0055ca34 */
char sLeftPedal[12] = "LeftPedal";

/* 0x0055ca40 */
char sRightPedal[16] = "RightPedal";

/* 0x0055ca50 */
char sSprocket[12] = "Sprocket";

/* 0x0055ca5c */
char sHandlebars[16] = "Handlebars";

/* 0x0055ca6c */
char sHipsdeck[12] = "Hipsdeck";

/* 0x0055ca78 */
char sCZaxisBmxSrcComZgeNgIFNgIFzactorC[52] = "C:\\zaxis\\Bmx\\src\\com\\zge\\NgIF\\NgIFzactor.c";

/* 0x0055caac */
char sTestzactorGetAbsolutePartPositionUnknow[60] = "testzactorGetAbsolutePartPosition: Unknown part";

/* 0x0055cae8 */
char sTestzactorGetPartLengthUnknownPart[48] = "testzactorGetPartLength: Unknown part";

/* 0x0055cb18 */
char sMissingHashFunction[28] = "Missing hash function";

/* 0x0055cb34 */
char sCZaxisBmxSrcComZgeNgIFNgIFzbspC[52] = "C:\\zaxis\\Bmx\\src\\com\\zge\\NgIF\\NgIFzbsp.c";

/* 0x0055cb68 */
char sBspInit[12] = "bsp init";

/* 0x0055cb74 */
char sBspUpdate[16] = "bsp update";

/* 0x0055cb84 */
char sShutdown[12] = "shutdown";

/* 0x0055cb90 */
char sInitializeScene[20] = "initialize scene";

/* 0x0055cba4 */
char sLoadScene[16] = "load scene";

/* 0x0055cbb4 */
char sLoadAndProcessMap[28] = "load and process map";

/* 0x0055cbd0 */
char sAllocAndLoadMap[24] = "alloc and load map";

/* 0x0055cbe8 */
char sFreeMap[12] = "free map";

/* 0x0055cbf4 */
char sSetTreeVisible[20] = "set tree visible";

/* 0x0055cc08 */
char sConvertTreeFromLoad[28] = "convert tree from load";

/* 0x0055cc24 */
char sRelativeLodZ[20] = "relative lod z";

/* 0x0055cc38 */
char sPreprocessScene[20] = "preprocess scene";

/* 0x0055cc4c */
char sPreprocessTerrainNode[28] = "preprocess terrain node";

/* 0x0055cc68 */
char sPreprocessFaceList[28] = "preprocess face list";

/* 0x0055cc84 */
char sViewerInPosSide[24] = "viewer in pos side";

/* 0x0055cc9c */
char sSndRegister[16] = "snd register";

/* 0x0055ccac */
char sSndSetState[16] = "snd set state";

/* 0x0055ccbc */
char sSndGetHandle[20] = "snd get handle";

/* 0x0055ccd0 */
char sDvlitehandle[16] = "dvlitehandle";

/* 0x0055cce0 */
char sDvlitegetcolor[20] = "dvlitegetcolor";

/* 0x0055ccf4 */
char sDvlitelite[16] = "dvlitelite";

/* 0x0055cd04 */
char sDvliteinitialize[20] = "dvliteinitialize";

/* 0x0055cd18 */
char sDvliteAttachToProps[28] = "dvlite attach to props";

/* 0x0055cd34 */
char sPropActivate[16] = "prop activate";

/* 0x0055cd44 */
char sPropDeactivate[20] = "prop deactivate";

/* 0x0055cd58 */
char sActivateTree[16] = "activate tree";

/* 0x0055cd68 */
char sDeactivateTree[20] = "deactivate tree";

/* 0x0055cd7c */
char sInitializeTreeRef[24] = "initialize tree ref";

/* 0x0055cd94 */
char sCreateTreeRef[20] = "create tree ref";

/* 0x0055cda8 */
char sDestroyTreeRef[20] = "destroy tree ref";

/* 0x0055cdbc */
char sPropAddTreeRef[24] = "prop add tree ref";

/* 0x0055cdd4 */
char sRemoveTreeRef[20] = "remove tree ref";

/* 0x0055cde8 */
char sUpdateTreeRef[20] = "update tree ref";

/* 0x0055cdfc */
char sAddToTree[16] = "add to tree";

/* 0x0055ce0c */
char sRemoveFromTree[20] = "remove from tree";

/* 0x0055ce20 */
char sTotallyWithinTree[24] = "totally within tree";

/* 0x0055ce38 */
char sOverlapsTree[16] = "overlaps tree";

/* 0x0055ce48 */
char sFindTagged[16] = "find tagged";

/* 0x0055ce58 */
char sZbspPropMove[16] = "zbspPropMove";

/* 0x0055ce68 */
char sGetNumDefs[16] = "get num defs";

/* 0x0055ce78 */
char sGetTypeFromHash[24] = "get type from hash";

/* 0x0055ce90 */
char sFxCreateTree[20] = "fx create tree";

/* 0x0055cea4 */
char sDestroyTree[16] = "destroy tree";

/* 0x0055ceb4 */
char sDrawTerrain[16] = "draw terrain";

/* 0x0055cec4 */
char sDrawTerrainNode[24] = "draw terrain node";

/* 0x0055cedc */
char sDrawFaceList[20] = "draw face list";

/* 0x0055cef0 */
char sSetDebug[12] = "set debug";

/* 0x0055cefc */
char sDrawLineZ[16] = "draw line z";

/* 0x0055cf0c */
char sDrawFaceNormal[20] = "draw face normal";

/* 0x0055cf20 */
char sDrawFaceFlat[20] = "draw face flat";

/* 0x0055cf34 */
char sShowBoundingBox[24] = "show bounding box";

/* 0x0055cf4c */
char sShowEdges[16] = "show edges";

/* 0x0055cf5c */
char sShowPropVolume[20] = "show prop volume";

/* 0x0055cf70 */
char sGetFaceNum[16] = "get face num";

/* 0x0055cf80 */
char sTrnDumpTree[16] = "trn dump tree";

/* 0x0055cf90 */
char sShowPartition[20] = "show partition";

/* 0x0055cfa4 */
char sDrawPartition[20] = "draw partition";

/* 0x0055cfb8 */
char sGetTerrainFaces[24] = "get terrain faces";

/* 0x0055cfd0 */
char sGetTreeObject[20] = "get tree object";

/* 0x0055cfe4 */
char sPointWithinTreeObject[32] = "point within tree object";

/* 0x0055d004 */
char sGetTreeObjects[20] = "get tree objects";

/* 0x0055d018 */
char sGetTerrainNode[20] = "get terrain node";

/* 0x0055d02c */
char sGetMaterialInfo[24] = "get material info";

/* 0x0055d044 */
char sGetMatRefData[20] = "get mat ref data";

/* 0x0055d058 */
char sGetMatIdx[16] = "get mat idx";

/* 0x0055d068 */
char sMatRefList[16] = "mat ref list";

/* 0x0055d078 */
char sDoesVertExist[20] = "does vert exist";

/* 0x0055d08c */
char sFacesAtSamePos[24] = "faces at same pos";

/* 0x0055d0a4 */
char sSceneSaveState[20] = "scene save state";

/* 0x0055d0b8 */
char sSceneRestoreState[24] = "scene restore state";

/* 0x0055d0d0 */
char sEdgeListOverflow[24] = "edge list overflow";

/* 0x0055d0e8 */
char sGetNormalVector[24] = "get normal vector";

/* 0x0055d100 */
char sGetSideVector[20] = "get side vector";

/* 0x0055d114 */
char sBspcollide[16] = "bspcollide";

/* 0x0055d124 */
char sCreateSpanPlanes[24] = "create span planes";

/* 0x0055d13c */
char sAddToFacelist[20] = "add to facelist";

/* 0x0055d150 */
char sGetTerrainHeightinf[28] = "get terrain heightinf";

/* 0x0055d16c */
char sTestRayCollision[24] = "test ray collision";

/* 0x0055d184 */
char sBsptest[12] = "bsptest";

/* 0x0055d190 */
char sTestresetfacecolors[24] = "testresetfacecolors";

/* 0x0055d1a8 */
char sHilightnodefaces[20] = "hilightnodefaces";

/* 0x0055d1bc */
char sSetFaceRgb[16] = "set face rgb";

/* 0x0055d1cc */
char sPrintFaceListInf[24] = "print face list inf";

/* 0x0055d1e4 */
char sTrntestraycollision[24] = "trntestraycollision";

/* 0x0055d1fc */
char sCreatenodeshit[20] = "createnodeshit";

/* 0x0055d210 */
char sAddToNodeslist[20] = "add to nodeslist";

/* 0x0055d224 */
char sAddToFacesList[24] = "add to faces list";

/* 0x0055d23c */
char sLineToNodes[16] = "line to nodes";

/* 0x0055d24c */
char sLineToFaces[16] = "line to faces";

/* 0x0055d25c */
char sLineplaneintersection[28] = "lineplaneintersection";

/* 0x0055d278 */
char sClipRayToObject[24] = "clip ray to object";

/* 0x0055d290 */
char sClipLineToProps[24] = "clip line to props";

/* 0x0055d2a8 */
char sCliplinetoprop[20] = "Cliplinetoprop";

/* 0x0055d2bc */
char sBuildrayboundingbox[24] = "buildrayboundingbox";

/* 0x0055d2d4 */
char sAddpropfaces[16] = "addpropfaces";

/* 0x0055d2e4 */
char sZbspComputeFacePlaneEq[28] = "zbspComputeFacePlaneEq";

/* 0x0055d300 */
char sTaginitialize[16] = "taginitialize";

/* 0x0055d310 */
char sTagregisterprofunc[24] = "tagregisterprofunc";

/* 0x0055d328 */
char sFacetagregister[20] = "facetagregister";

/* 0x0055d33c */
char sFacetagpreprocess[24] = "facetagpreprocess";

/* 0x0055d354 */
char sFacetagprocess[20] = "facetagprocess";

/* 0x0055d368 */
char sFacetagfinddef[20] = "facetagfinddef";

/* 0x0055d37c */
char sFacetagregregister[24] = "facetagregregister";

/* 0x0055d394 */
char sFacetagpreattachasref[28] = "facetagpreattachasref";

/* 0x0055d3b0 */
char sFacetagpreattachsmposasref[32] = "facetagpreattachsmposasref";

/* 0x0055d3d0 */
char sFacetagproswapwindow[28] = "facetagproswapwindow";

/* 0x0055d3ec */
char sBufferOverflow[20] = "Buffer overflow";

/* 0x0055d400 */
char sColspherecreatenodeshit[28] = "colspherecreatenodeshit";

/* 0x0055d41c */
char sHitnodeslist[16] = "hitnodeslist";

/* 0x0055d42c */
char sTestwithfacelist[20] = "testwithfacelist";

/* 0x0055d440 */
char sFaceboundingbox[20] = "faceboundingbox";

/* 0x0055d454 */
char sZbspColSphereDraw[24] = "zbspColSphereDraw";

/* 0x0055d46c */
char sPropListOverflow[24] = "prop list overflow";

/* 0x0055d484 */
char sZbspFindFaceByType[116] = {
    122, 98, 115, 112, 70, 105, 110, 100, 70, 97, 99, 101, 66, 121, 84, 121, 112, 101, 40, 41, 0, 0, 0,
    0,   0,  0,   0,   0,  4,   0,   3,   7,  1,  5,  6,   2,  0,   1,  2,   3,   5,   4,  7,  6, 4, 5,
    1,   0,  0,   1,   2,  -1,  3,   5,   4,  -1, 0,  3,   4,  1,   3,  0,   2,   5,   0,  1,  2, 3, 5,
    4,   7,  6,   4,   0,  3,   7,   1,   5,  6,  2,  4,   5,  1,   0,  0,   0,   0,   0,  0,  0, 0, 0,
    8,   0,  0,   0,   5,  0,   0,   0,   8,  0,  0,  0,   5,  0,   0,  0,   6,   0,   0,  0,  4};

/* 0x0055d4f8 */
ZcollideBoxEdges gZcollideBoxEdges = {
    {.b = 1, .side = 1},
    {.a = 1, .b = 5, .side = 1},
    {.a = 5, .b = 4, .side = 1},
    {.a = 4, .side = 1},
};

struct BmxGroup_gZcollideBarEdgeGroup gZcollideBarEdgeGroup __attribute__((aligned(16))) = {
    .gZcollideBarEdge = {.b = 1, .side = 1},
};

/* 0x0055d51c */
char sDegenerateCollision[28] = "Degenerate collision.";

/* 0x0055d538 */
char sInvalidParameter[24] = "Invalid parameter.";

/* 0x0055d550 */
char sCZaxisBmxSrcComZgeNgIFNgIFzcollideC[56] = "C:\\zaxis\\Bmx\\src\\com\\zge\\NgIF\\NgIFzcollide.c";

/* 0x0055d588 */
char sZcolbboxcollision[24] = "zcolbboxcollision";

/* 0x0055d5a0 */
char sGetTerrainFace[20] = "Get terrain face";

/* 0x0055d5b4 */
char sPropHitBox[16] = "prop hit box";

/* 0x0055d5c4 */
char sGetPlaneHeight[20] = "GetPlaneHeight";

/* 0x0055d5d8 */
char sGenerateBoundBox[24] = "Generate bound box";

/* 0x0055d5f0 */
char sInitPropFacesList[28] = "Init Prop faces list";

/* 0x0055d60c */
char sGeneratePropBoundingVolume[36] = "Generate prop bounding volume";

/* 0x0055d630 */
char sNotCoded[12] = "Not coded";

/* 0x0055d63c */
char sPropMove[12] = "prop move";

/* 0x0055d648 */
char sZMEMDMMFailedToInitializeDMM[44] = "ZMEMDMM: Failed to initialize DMM\n";

/* 0x0055d674 */
char sCZaxisBmxSrcComZgeNgIFNgIfzmemDMMC[52] = "C:\\zaxis\\Bmx\\src\\com\\zge\\NgIF\\NgIfzmemDMM.c";

/* 0x0055d6a8 */
char sZMEMDMMErrorDMMAlreadyInitialized[52] = "ZMEMDMM: Error - DMM already initialized\n";

/* 0x0055d6dc */
char sZMEMDMMDMMShutdownWithoutBeingInitializ[60] = "ZMEMDMM: DMM Shutdown without being initialized\n";

/* 0x0055d718 */
char sZMEMOutOfDMMHandles[32] = "ZMEM: Out of DMM handles";

/* 0x0055d738 */
char sZMEMDMMDMMFailedToAllocate[40] = "ZMEMDMM: DMM failed to allocate";

/* 0x0055d760 */
char sZMEMOutOfRAM[20] = "ZMEM: Out of RAM";

/* 0x0055d774 */
char sZMEMDMMDMMAllocationSize0[40] = "ZMEMDMM: DMM allocation size = 0\n";

/* 0x0055d79c */
char sZMEMDMMDMMUsedWithoutBeingInitialized[56] = "ZMEMDMM: DMM used without being initialized\n";

/* 0x0055d7d4 */
char sZMEMDMMInvalidHandlePassedToZmemDMMDeal[60] = "ZMEMDMM: invalid handle passed to zmemDMMDealloc\n";

/* 0x0055d810 */
char sLocked[8] = "Locked";

/* 0x0055d818 */
char sZMEMDMMTryingToLockHandle0[40] = "ZMEMDMM: Trying to lock handle 0\n";

/* 0x0055d840 */
char sZMEMDMMTryingToUnlockHandle0[44] = "ZMEMDMM: Trying to unlock handle 0\n";

/* 0x0055d86c */
char sZMEMDMMDMMOptimizeFoundUnrecognizedPoin[92] =
    "ZMEMDMM: DMM optimize found unrecognized pointer, memory corruption likely\n";

/* 0x0055d8c8 */
char sF_0055d8c8[176] = {
    70,  0, 12, 0, 82,  0, 12, 0, 94,  0, 12, 0, 106, 0, 12, 0, 118, 0, 14, 0, -124, 0, 14, 0, -110, 0, 14, 0,
    -96, 0, 13, 0, -83, 0, 12, 0, -71, 0, 12, 0, -59, 0, 12, 0, -47, 0, 12, 0, -35,  0, 12, 0, -23,  0, 12, 0,
    -11, 0, 12, 0, 1,   1, 12, 0, 13,  1, 12, 0, 25,  1, 12, 0, 37,  1, 12, 0, 49,   1, 12, 0, 61,   1, 12, 0,
    73,  1, 12, 0, 85,  1, 12, 0, 97,  1, 12, 0, 109, 1, 12, 0, 121, 1, 12, 0, -123, 1, 12, 0, -111, 1, 12, 0,
    -99, 1, 12, 0, -87, 1, 12, 0, -75, 1, 12, 0, -63, 1, 12, 0, -51, 1, 12, 0, -39,  1, 12, 0, -27,  1, 12, 0,
    -15, 1, 12, 0, -3,  1, 12, 0, 9,   2, 12, 0, 21,  2, 12, 0, 33,  2, 12, 0, 45,   2, 12, 0, 57,   2, 12, 0,
    69,  2, 12};

/* 0x0055d978 */
ZresOverlaySourceTable gZresOverlaySources = {
    {.id = 48, .asset = 9, .pad = {0, 0, 0, 0, 0, 8}},
    {.id = 49, .asset = 10, .pad = {0, 0, 0, 0, 128, 16}},
    {.id = 50, .asset = 11},
    {.id = 51, .asset = 12},
    {.id = 52, .asset = 13, .pad = {0, 0, 0, 0, 0, 8}},
    {.id = 53, .asset = 14, .pad = {0, 0, 0, 0, 128, 16}},
    {.id = 54, .asset = 15},
    {.id = 55, .asset = 16},
    {.id = 56, .asset = 17},
    {.id = 57, .asset = 18},
    {.id = 58, .asset = 19},
};

/* 0x0055da28 */
unsigned char d_0055da28[72];

/* 0x0055da70 */
char sOutOfSyncMapData[28] = "Out of sync map data";

/* 0x0055da8c */
char sCZaxisBmxSrcComZgeNgIFNgIFzresC[52] = "C:\\zaxis\\Bmx\\src\\com\\zge\\NgIF\\NgIFzres.c";

/* 0x0055dac0 */
char sMemoryOverflowInZresIncreaseFIXEDDEFSSI[68] = "Memory overflow in zres()!!! Increase FIXED_DEFS_SIZE.";

/* 0x0055db04 */
char sOutOfSyncPlayerData[28] = "Out of sync player data";

/* 0x0055db20 */
char sInvalidPlayer[20] = "Invalid player";

/* 0x0055db34 */
char sBikeNumberOutOfRange[32] = "bike number out of range";

/* 0x0055db54 */
char sAssetsMaterialZAL[24] = "Assets\\Material.ZAL";

/* 0x0055db6c */
char sAssetsReplayZAL[24] = "Assets\\Replay.ZAL";

/* 0x0055db84 */
char sAssetsTablesZAL[24] = "Assets\\Tables.ZAL";

/* 0x0055db9c */
char sCouldNotOpenZAL[24] = "Could not open ZAL";

/* 0x0055dbb4 */
char sZRESFailedToOpenS[32] = "ZRES: Failed to open:%s\n";

/* 0x0055dbd4 */
char sAssetsMiscDataZAL[24] = "Assets\\MiscData.ZAL";

/* 0x0055dbec */
char sERRORLoadingOverlayMaterial[40] = "ERROR loading overlay material";

/* 0x0055dc14 */
char sOverlayD[12] = "Overlay%d";

/* 0x0055dc20 */
char sZresAddMaterialToLoadListNotFound[48] = "zresAddMaterialToLoadList() Not found";

struct BmxGroup_gZrollerRiderNamesGroup gZrollerRiderNamesGroup __attribute__((aligned(16))) = {
    .gZrollerRiderNames =
        {
            sDaveMirra,
        },
    .p_0055dc54 =
        {
            &sRyanNyquist,
            &sTroyMcMurray,
            &sMikeLaird,
            &sChadKagy,
            &sTimMirra,
            &sKenanHarkin,
            &sShaunButler,
            &sLeighRamsdell,
            &sJoeyGarcia,
            &sSlimJimGuy,
            &sAmishBoy,
        },
};

struct BmxGroup_gZrollerBikeNamesGroup gZrollerBikeNamesGroup __attribute__((aligned(16))) = {
    .gZrollerBikeNames =
        {
            sDMbike01,
        },
    .p_0055dc84 =
        {
            &sDMbike02,  &sDMbike03,  &sDMbike04,  &sRNbike01,  &sRNbike02, &sRNbike03,  &sRNbike04,
            &sTMcbike01, &sTMcbike02, &sTMcbike03, &sTMcbike04, &sMLbike01, &sMLbike02,  &sMLbike03,
            &sMLbike04,  &sCKbike01,  &sCKbike02,  &sCKbike03,  &sCKbike04, &sTMibike01, &sTMibike02,
            &sTMibike03, &sTMibike04, &sKHbike01,  &sKHbike02,  &sKHbike03, &sKHbike04,  &sSBbike01,
            &sSBbike02,  &sSBbike03,  &sSBbike04,  &sLRbike01,  &sLRbike02, &sLRbike03,  &sLRbike04,
            &sJGbike01,  &sJGbike02,  &sJGbike03,  &sJGbike04,  &sSJbike01, &sSJbike02,  &sSJbike03,
            &sSJbike04,  &sABbike01,  &sABbike02,  &sABbike03,  &sABbike04, &sXXbike01,
        },
};

/* 0x0055dd74 */
char sXXbike01[12] = "XXbike01";

/* 0x0055dd80 */
char sABbike04[12] = "ABbike04";

/* 0x0055dd8c */
char sABbike03[12] = "ABbike03";

/* 0x0055dd98 */
char sABbike02[12] = "ABbike02";

/* 0x0055dda4 */
char sABbike01[12] = "ABbike01";

/* 0x0055ddb0 */
char sSJbike04[12] = "SJbike04";

/* 0x0055ddbc */
char sSJbike03[12] = "SJbike03";

/* 0x0055ddc8 */
char sSJbike02[12] = "SJbike02";

/* 0x0055ddd4 */
char sSJbike01[12] = "SJbike01";

/* 0x0055dde0 */
char sJGbike04[12] = "JGbike04";

/* 0x0055ddec */
char sJGbike03[12] = "JGbike03";

/* 0x0055ddf8 */
char sJGbike02[12] = "JGbike02";

/* 0x0055de04 */
char sJGbike01[12] = "JGbike01";

/* 0x0055de10 */
char sLRbike04[12] = "LRbike04";

/* 0x0055de1c */
char sLRbike03[12] = "LRbike03";

/* 0x0055de28 */
char sLRbike02[12] = "LRbike02";

/* 0x0055de34 */
char sLRbike01[12] = "LRbike01";

/* 0x0055de40 */
char sSBbike04[12] = "SBbike04";

/* 0x0055de4c */
char sSBbike03[12] = "SBbike03";

/* 0x0055de58 */
char sSBbike02[12] = "SBbike02";

/* 0x0055de64 */
char sSBbike01[12] = "SBbike01";

/* 0x0055de70 */
char sKHbike04[12] = "KHbike04";

/* 0x0055de7c */
char sKHbike03[12] = "KHbike03";

/* 0x0055de88 */
char sKHbike02[12] = "KHbike02";

/* 0x0055de94 */
char sKHbike01[12] = "KHbike01";

/* 0x0055dea0 */
char sTMibike04[12] = "TMibike04";

/* 0x0055deac */
char sTMibike03[12] = "TMibike03";

/* 0x0055deb8 */
char sTMibike02[12] = "TMibike02";

/* 0x0055dec4 */
char sTMibike01[12] = "TMibike01";

/* 0x0055ded0 */
char sCKbike04[12] = "CKbike04";

/* 0x0055dedc */
char sCKbike03[12] = "CKbike03";

/* 0x0055dee8 */
char sCKbike02[12] = "CKbike02";

/* 0x0055def4 */
char sCKbike01[12] = "CKbike01";

/* 0x0055df00 */
char sMLbike04[12] = "MLbike04";

/* 0x0055df0c */
char sMLbike03[12] = "MLbike03";

/* 0x0055df18 */
char sMLbike02[12] = "MLbike02";

/* 0x0055df24 */
char sMLbike01[12] = "MLbike01";

/* 0x0055df30 */
char sTMcbike04[12] = "TMcbike04";

/* 0x0055df3c */
char sTMcbike03[12] = "TMcbike03";

/* 0x0055df48 */
char sTMcbike02[12] = "TMcbike02";

/* 0x0055df54 */
char sTMcbike01[12] = "TMcbike01";

/* 0x0055df60 */
char sRNbike04[12] = "RNbike04";

/* 0x0055df6c */
char sRNbike03[12] = "RNbike03";

/* 0x0055df78 */
char sRNbike02[12] = "RNbike02";

/* 0x0055df84 */
char sRNbike01[12] = "RNbike01";

/* 0x0055df90 */
char sDMbike04[12] = "DMbike04";

/* 0x0055df9c */
char sDMbike03[12] = "DMbike03";

/* 0x0055dfa8 */
char sDMbike02[12] = "DMbike02";

/* 0x0055dfb4 */
char sDMbike01[12] = "DMbike01";

/* 0x0055dfc0 */
char sAmishBoy[16] = "Amish Boy ";

/* 0x0055dfd0 */
char sSlimJimGuy[16] = "Slim Jim Guy ";

/* 0x0055dfe0 */
char sJoeyGarcia[16] = "Joey Garcia ";

/* 0x0055dff0 */
char sLeighRamsdell[20] = "Leigh Ramsdell ";

/* 0x0055e004 */
char sShaunButler[16] = "Shaun Butler ";

/* 0x0055e014 */
char sKenanHarkin[16] = "Kenan Harkin ";

/* 0x0055e024 */
char sTimMirra[16] = "Tim Mirra ";

/* 0x0055e034 */
char sChadKagy[16] = "Chad Kagy ";

/* 0x0055e044 */
char sMikeLaird[16] = "Mike Laird ";

/* 0x0055e054 */
char sTroyMcMurray[20] = "Troy McMurray ";

/* 0x0055e068 */
char sRyanNyquist[16] = "Ryan Nyquist ";

/* 0x0055e078 */
char sDaveMirra[16] = "Dave Mirra ";

/* 0x0055e088 */
char sZROLLERFailedToIntializeZRES[44] = "ZROLLER: Failed to intialize ZRES\n";

/* 0x0055e0b4 */
char sCZaxisBmxSrcComZgeNgIFNgIFzrollerC[52] = "C:\\zaxis\\Bmx\\src\\com\\zge\\NgIF\\NgIFzroller.c";

/* 0x0055e0e8 */
char sZrollerUpdateZGL[20] = "zrollerUpdateZGL";

/* 0x0055e0fc */
char sZrollerCreateChaser[24] = "zrollerCreateChaser";

/* 0x0055e114 */
char sZrollerLoadAssets[24] = "zrollerLoadAssets";

/* 0x0055e12c */
char sRiderShadow[16] = "Rider Shadow";

/* 0x0055e13c */
char sOutOfPlayerDefs[24] = "Out of player defs";

/* 0x0055e154 */
char sBikeShadow[16] = "Bike Shadow";

/* 0x0055e164 */
char sInvalidBikeSelected[28] = "Invalid bike selected!";

/* 0x0055e180 */
char sZrollerCreateProp[24] = "zrollerCreateProp";

/* 0x0055e198 */
char sZrollerSetEngineState[28] = "zrollerSetEngineState";

/* 0x0055e1b4 */
char sZrollerGetNumPlayerDefs[28] = "zrollerGetNumPlayerDefs";

/* 0x0055e1d0 */
char sZrollerLoadMdlPlayerCB[28] = "zrollerLoadMdlPlayerCB";

/* 0x0055e1ec */
char sZrollerIsPlayerClose[28] = "zrollerIsPlayerClose";

/* 0x0055e208 */
char sChangePlayerModeLook[28] = "ChangePlayerModeLook";

/* 0x0055e224 */
char sZrollerAddPlayerMatsToLoadList[40] = "zrollerAddPlayerMatsToLoadList";

/* 0x0055e24c */
char sZrollerChangePlayerSponsor[32] = "zrollerChangePlayerSponsor";

/* 0x0055e26c */
char sZrollerchangeboardsponsor[32] = "Zrollerchangeboardsponsor";

/* 0x0055e28c */
char sZrollerLoadMdlGenericCB[28] = "zrollerLoadMdlGenericCB";

/* 0x0055e2a8 */
char sZrollerLoadMdlCommon[28] = "zrollerLoadMdlCommon";

/* 0x0055e2c4 */
char sLoadMdlCommonMat[20] = "LoadMdlCommonMat";

/* 0x0055e2d8 */
char sZrollerProcessModelFlags[32] = "zrollerProcessModelFlags";

/* 0x0055e2f8 */
char sZrollerProcessActorDrawFlags[36] = "zrollerProcessActorDrawFlags";

/* 0x0055e31c */
char sZrollerSetAnimMaterialPalette[36] = "zrollerSetAnimMaterialPalette";

/* 0x0055e340 */
char sZrollerInitializeLoadingScreen[40] = "zrollerInitializeLoadingScreen";

/* 0x0055e368 */
char sZrollerShutdownLoadingScreen[36] = "zrollerShutdownLoadingScreen";

/* 0x0055e38c */
char sZrollerDisplayLoadingScreen[36] = "zrollerDisplayLoadingScreen";

/* 0x0055e3b0 */
char sZrollerSetLocal[20] = "zrollerSetLocal";

/* 0x0055e3c4 */
char sZrollerLoadModule[24] = "zrollerLoadModule";

/* 0x0055e3dc */
char sLoadCodeModule[20] = "LoadCodeModule";

/* 0x0055e3f0 */
char sZrollerAllocMaterials[28] = "zrollerAllocMaterials";

/* 0x0055e40c */
char sZrollerAddGenericMaterialsToLoad[40] = "zrollerAddGenericMaterialsToLoad";

/* 0x0055e434 */
char sZrollerInitializeMaps[28] = "zrollerInitializeMaps";

/* 0x0055e450 */
char sZrollerSetMap[16] = "zrollerSetMap";

/* 0x0055e460 */
char sZrollerLoadMaps[20] = "zrollerLoadMaps";

/* 0x0055e474 */
char sZrollerLoadMatTerrainFirstCB[36] = "zrollerLoadMatTerrainFirstCB";

/* 0x0055e498 */
char sZrollerLoadMatTerrainLastCB[36] = "zrollerLoadMatTerrainLastCB";

/* 0x0055e4bc */
char sZrollerAssociateMapMaterials[36] = "zrollerAssociateMapMaterials";

/* 0x0055e4e0 */
char sZroolerAssociateMaterialLlljlkfsdjflsjd[52] = "zroolerAssociateMaterial llljlkfsdjflsjdklf";

/* 0x0055e514 */
char sAssociateMaterialPalAnimation[40] = "Associate material pal animation";

/* 0x0055e53c */
char sZrollerGetPlayerStartPosHARDCODED[44] = "zrollerGetPlayerStartPos: HARDCODED";

/* 0x0055e568 */
char sPlayerStartPointNotFoundUsingDefaultVal[64] = "Player start point not found - Using default value";

/* 0x0055e5a8 */
char sPlayerT[12] = "PlayerT";

/* 0x0055e5b4 */
char sPlayer_0055e5b4[8] = "Player";

/* 0x0055e5bc */
char sZrollerGetPlayerRestartPos[32] = "zrollerGetPlayerRestartPos";

/* 0x0055e5dc */
char sZrollerInitPropLoadList[28] = "zrollerInitPropLoadList";

/* 0x0055e5f8 */
char sZrollerAddPropToLoadList[32] = "zrollerAddPropToLoadList";

/* 0x0055e618 */
char sCreateProps[16] = "Create Props";

/* 0x0055e628 */
char sZrollerLoadPropMaterials[32] = "zrollerLoadPropMaterials";

/* 0x0055e648 */
char sZrollerCreatePropActor[28] = "zrollerCreatePropActor";

/* 0x0055e664 */
char sZrollerDestroyActor[24] = "zrollerDestroyActor";

/* 0x0055e67c */
char sZrollerGetAsyncKeyState[28] = "zrollerGetAsyncKeyState";

/* 0x0055e698 */
char sZrollerSelectMap[20] = "zrollerSelectMap";

/* 0x0055e6ac */
char sZrollerSelectPlayer[24] = "zrollerSelectPlayer";

/* 0x0055e6c4 */
char sZrollerSelectBikeNum[28] = "zrollerSelectBikeNum";

/* 0x0055e6e0 */
char sSetFogDistance[20] = "set fog distance";

/* 0x0055e6f4 */
char sGetFogDistance[20] = "get fog distance";

/* 0x0055e708 */
char sZrollerDisableFog[24] = "zrollerDisableFog";

/* 0x0055e720 */
char sZrollerSetFogColor[24] = "zrollerSetFogColor";

/* 0x0055e738 */
char sZrollerReplaceMaterial[28] = "zrollerReplaceMaterial";

/* 0x0055e754 */
char sZrollerSetShadeMode[24] = "zrollerSetShadeMode";

/* 0x0055e76c */
char sZrollerStartCrossFade[28] = "zrollerStartCrossFade";

/* 0x0055e788 */
char sZrollerGetSceneStateNODATA[36] = "zrollerGetSceneState: NO DATA";

/* 0x0055e7ac */
char sFreeSceneState[20] = "FreeSceneState";

/* 0x0055e7c0 */
char sSetSceneState[16] = "SetSceneState";

/* 0x0055e7d0 */
char sZrollerSelectLook[24] = "zrollerSelectLook()";

/* 0x0055e7e8 */
char sRegisteredTooManyDrawCallbacksToZroller[56] = "registered too many draw callbacks to zroller";

/* 0x0055f35c */
float gMotionTimeScale = 34.0f;

/* 0x0055f360 */
int32_t gZactorMax = 64;

/* 0x0055f364 */
BmxType_gMotionMirrorMap gMotionMirrorMap = {0, 1, 2, 6, 7, 8, 3, 4, 5, 12, 13, 14, 9, 10, 11};

/* 0x0055f38c */
uint32_t gMotionLoadedBank = 0xffffffff;

struct BmxGroup_gSkeletonChainGroup gSkeletonChainGroup __attribute__((aligned(16))) = {
    .d_0055f391 = {255, 0, 0,   0,  0,   0, 0,   1,  0,   255, 0,   0,   0,  0,   0, 2,   1, 0,   255, 0,
                   0,   0, 0,   3,  1,   0, 255, 0,  0,   0,   0,   4,   3,  1,   0, 255, 0, 0,   0,   5,
                   4,   3, 1,   0,  255, 0, 0,   6,  1,   0,   255, 0,   0,  0,   0, 7,   6, 1,   0,   255,
                   0,   0, 0,   8,  7,   6, 1,   0,  255, 0,   0,   9,   0,  255, 0, 0,   0, 0,   0,   10,
                   9,   0, 255, 0,  0,   0, 0,   11, 10,  9,   0,   255, 0,  0,   0, 12,  0, 255, 0,   0,
                   0,   0, 0,   13, 12,  0, 255, 0,  0,   0,   0,   14,  13, 12,  0, 255},
};

struct BmxGroup_gSkeletonOffsetsGroup gSkeletonOffsetsGroup __attribute__((aligned(16))) = {
    .d_0055f4cc = {0, 0,         -19.94354f, 2.67121f,  0, -1e-05f,    -36.75616f, 1.55506f,
                   0, 18.27357f, -35.97014f, -0.57176f, 0, 0.26444f,   26.30201f,  0.03595f,
                   0, -0.88317f, 27.1319f,   -0.01658f, 0, -18.27357f, -28.97013f, -0.57176f,
                   0, -0.26458f, 24.30201f,  0.00008f,  0, 0.88317f,   27.1319f,   -0.01658f,
                   0, 8.93869f,  0.6647f,    0.01879f,  0, -0.01302f,  37.76207f,  -0.03539f,
                   0, 0,         33.15057f,  0.45551f,  0, -8.93869f,  1.6647f,    0.01879f,
                   0, 0.01303f,  37.76208f,  -0.0354f,  0, -1e-05f,    33.15056f,  0.45551f},
};

/* 0x0055f7e0 */
char sOnlyCodedForPropTypes[32] = "Only coded for prop types";

/* 0x0055f800 */
char sCZaxisBmxSrcComZgeZactorC[40] = "C:\\zaxis\\Bmx\\src\\com\\zge\\zactor.c";

/* 0x0055f828 */
char sZACTORErrorFailedToAllocateActor[52] = "ZACTOR: Error - failed to allocate actor\n";

/* 0x0055f85c */
char sMotionsAreTooLarge[28] = "Motions are too large.";

/* 0x0055f878 */
char sMotionLibraryAlreadyOpen[36] = "Motion library already open";

/* 0x0055f89c */
char sMotionsNotOpened[24] = "Motions not opened";

/* 0x0055f8b4 */
char sTooBig[12] = "Too big";

/* 0x0055f8c0 */
char sCouldNotFindMotionBank[32] = "Could not find motion bank";

/* 0x0055f8e0 */
char sNotReloadingMotionLibrary[36] = "Not reloading motion library\n";

/* 0x0055f904 */
char sMotionBankNotOpened[28] = "Motion bank not opened";

/* 0x0055f920 */
char sExeededMaximumLODsForActor[40] = "Exeeded maximum LODs for actor";

/* 0x0055f948 */
char sZACTORUnknownTagIdentifier[40] = "ZACTOR: Unknown tag identifier \n";

/* 0x0055f970 */
char sNotAPersonType[24] = "Not a person type";

/* 0x0055f988 */
char sZPRIMFailedToAllocateBuffer[40] = "ZPRIM: Failed to allocate buffer\n";

/* 0x0055f9b0 */
char sErrorOptimizingDMMMemory[36] = "Error Optimizing DMM memory";

/* 0x0055f9d4 */
char sCZaxisBmxSrcComZgeZprimC[40] = "C:\\zaxis\\Bmx\\src\\com\\zge\\zprim.c";

/* 0x0055f9fc */
char sZPRIMZprimGetColorNotImplemented[48] = "ZPRIM: zprimGetColor not implemented\n";

/* 0x0055fa2c */
char sZPRIMFailedToAllocatePicture[44] = "ZPRIM: Failed to allocate picture\n";

/* 0x0055fa58 */
char sZPRIMFailedToFindPictureX[40] = "ZPRIM: Failed to find picture %x\n";

/* 0x0055fa80 */
char sBufferOverflowInZprimReplaceVRAMGrey[52] = "Buffer overflow in zprimReplaceVRAMGrey()";

/* 0x0055fab4 */
char sZPRIMFailedToAllocateSprite[40] = "ZPRIM: Failed to allocate sprite\n";

/* 0x0055fadc */
char sZPRIMFailedToFindSpriteX[40] = "ZPRIM: Failed to find sprite %x\n";

/* 0x0055fb04 */
char sZPRIMFailedToAllocateAssetHandle[48] = "ZPRIM: Failed to allocate asset handle\n";

/* 0x0055fb34 */
char sZPRIMFailedToAllocateBufferInCreateSpri[60] = "ZPRIM: Failed to allocate buffer in CreateSprite\n";

/* 0x0055fb70 */
char sZPRIMErrorCanOnlyDecompressFromCompress[104] =
    "ZPRIM: Error - can only decompress from compressed sprite to virtual sprite (%d->%d)\n";

/* 0x0055fbd8 */
char sZPRIMErrorFrameOutOfBoundsDuringDecompr[64] = "ZPRIM: Error - frame out of bounds during decompress\n";

/* 0x0055fc18 */
char sPossibleMemoryLeakOfFntMemHdlHere[48] = "Possible memory leak of fntMemHdl here.";

/* 0x0055fc48 */
char sZPRIMFailedToAllocateFont[40] = "ZPRIM: Failed to allocate font\n";

/* 0x0055fc70 */
char sZPRIMFailedToFindFontX[40] = "ZPRIM: Failed to find font %x\n";

/* 0x0055fc98 */
char sZPRIMFailedToAllocateGroup[40] = "ZPRIM: Failed to allocate group\n";

/* 0x0055fcc0 */
char sZPRIMFailedToFindGroup[36] = "ZPRIM: Failed to find group\n";

/* 0x0055fce4 */
char sZPRIMLoadFailed[24] = "ZPRIM: Load Failed\n";

/* 0x0055fcfc */
char sZPRIMErrorLoadingGroupIntoUsedSlot[56] = "ZPRIM: Error - loading group into used slot\n";

/* 0x0055fd34 */
char sZPRIMErrorBadSlotForGroup[44] = "ZPRIM: Error - bad slot for group\n";

/* 0x0055fd60 */
char sZPRIMFailedToLoadAsset[36] = "ZPRIM: Failed to load asset";

/* 0x0055fd84 */
char sZPRIMFailedToAllocateScreen[40] = "ZPRIM: Failed to allocate screen\n";

/* 0x0055fdac */
char sZPRIMFailedToFindScreen[36] = "ZPRIM: Failed to find screen\n";

/* 0x0055fdd0 */
char sZPRIMErrorCanOnlyDrawtexttospriteFromFo[96] =
    "ZPRIM: Error - can only drawtexttosprite from font to virtual sprite (%d->%d)\n";

/* 0x0055fe30 */
char sZPRIMErrorFrameOutOfBoundsDuringDrawtex[72] =
    "ZPRIM: Error - frame out of bounds during drawtexttosprite\n";

/* 0x0055fe78 */
char sInvalidMaterialHandle[28] = "Invalid material handle";

/* 0x0055fe94 */
char sSpriteD[12] = "Sprite%d";

/* 0x0055fea0 */
char sTrack[8] = "track";

/* 0x0055fea8 */
char sHTrackKzsndSeqMaxTracks[32] = "hTrack < kzsndSeqMaxTracks";

/* 0x0055fec8 */
char sCZaxisBmxSrcComZgeZsndseqC[44] = "C:\\zaxis\\Bmx\\src\\com\\zge\\zsndseq.c";

/* 0x0055fef4 */
char sTrackHTrack[16] = "Track[hTrack]";

/* 0x0055ff04 */
char sNoteEventFlagsKzsndNoteEventFlagActive[52] = "noteEvent->flags & kzsndNoteEventFlagActive";

/* 0x0055ff38 */
char sAlreadyInitialized[24] = "already initialized";

/* 0x0055ff50 */
char sCZaxisBmxSrcComZgeZsndsrcC[44] = "C:\\zaxis\\Bmx\\src\\com\\zge\\zsndsrc.c";

/* 0x0055ff7c */
char sNotInitialized[20] = "not initialized";

/* 0x0055ff90 */
char sSourceNotAllocated[28] = "source not allocated";

/* 0x0055ffac */
char sInvalidHandle[20] = "invalid handle";

/* 0x0055ffc0, also gWinstrmTicksPerSec, gZsndTickRate */
uint32_t gZsndTicksPerSec = 30;

/* 0x0055ffc4 */
char sAlreadyInitialized_0055ffc4[20] = "Already initialized";

/* 0x0055ffd8 */
char sCNgEngineZgeZdebugC[28] = "C:\\NgEngine\\Zge\\zdebug.c";

/* 0x0055fff4 */
char s104f104f104f[28] = "% 10.4f,% 10.4f,% 10.4f\n";

/* 0x00560010 */
char sS104f104f104f[32] = "%s% 10.4f,% 10.4f,% 10.4f\n";

/* 0x00560ee0 */
ZbspObjectFrameFn gZbspObjectFrameFn = (ZbspObjectFrameFn)zrwModelGetUserValue;

/* 0x00560ee8 */
BmxType_gFxFindTexture gFxFindTexture = zrwModelFindMaterial;

struct BmxGroup_gZrollerFreeHandleGroup gZrollerFreeHandleGroup __attribute__((aligned(16))) = {
    .gZrollerFreeHandle = (ZrollerFreeHandleFn)zrwmatReleaseTexture,
    .p_00561028 = zrwmatAddTexture,
};

struct BmxGroup_gFxSetRasterGroup gFxSetRasterGroup __attribute__((aligned(16))) = {
    .gFxSetRaster = zrwmatReplaceTexture,
    .p_00561030 = zrwmatSetColor,
};

/* 0x00561038 */
ZbspMaterialFn gZbspMaterialFn = (ZbspMaterialFn)zrwmatGetColor;

/* 0x0056103c */
ZRwTextureCloneFn gRwTextureClone = (ZRwTextureCloneFn)zrwmatClone;

/* 0x00561040 */
void *p_00561040 = zrwmatRetainMaterial;

/* 0x00561048, also gRwTextureLoad, gZrwShadowLoad */
ZresLoadTextureFn gZresLoadTexture = (ZresLoadTextureFn)zrwmatLoadSpriteTexture;

/* 0x0056104c */
BmxType_gFxTextureByHash gFxTextureByHash = zrwmatFindByTextureHash;

/* 0x00561050 */
void *p_00561050 = zrwPrintMaterialUsage;

/* 0x00561054 */
char sRwMatProblemLoadingImage[32] = "RwMat: problem loading image\n";

/* 0x00561074 */
char sMADFMismatch[16] = "MADF Mismatch";

/* 0x00561084 */
char sDataTooBig[16] = "Data too big";

/* 0x00561094 */
char sCNgEngineZgeComZglRwZrwMatC[36] = "C:\\NgEngine\\Zge\\Com\\ZglRw\\zrwMat.c";

/* 0x005610b8 */
char sNoTextureHasBeenAddedForThisRenderPass[48] = "No texture has been added for this render-pass";

/* 0x005610e8 */
char sTooManyTexturesAdded[24] = "Too many textures added";

/* 0x00561100 */
char sPMatPRwMatNotSet[24] = "pMat->pRwMat not set";

/* 0x00561118 */
char sZrwMatCreateFromImageErrorLoadingS[44] = "zrwMatCreateFromImage: Error loading:%s\n";

/* 0x00561144 */
char sZrwMatCreateProceduralErrorCreatingS[44] = "zrwMatCreateProcedural: Error Creating:%s\n";

/* 0x00561170 */
char sZrwMatCreatePaletteFromMemoryError[40] = "zrwMatCreatePaletteFromMemory: Error\n";

/* 0x00561198 */
char sOutOfRange[16] = "Out of range";

/* 0x005611a8 */
char sNotLoadedMADF[16] = "Not loadedMADF";

/* 0x005611b8 */
char sZGLMATObjectsInUseD[28] = "ZGLMAT: Objects in use:%d\n";

/* 0x005611d4 */
void *p_005611d4 = &sRwAppClass;

/* 0x00561470, also gNgifRegister, gTimerRegister, gZrwDbgRegister, gZrwModelRegister */
ZrollerRegisterFn gZrollerRegister = (ZrollerRegisterFn)zrwRegisterUpdateFunction;

/* 0x00561474, also gNgifUnregister, gZrwDbgUnregister, gZrwModelUnregister */
ZrollerUnregisterFn gZrollerUnregister = (ZrollerUnregisterFn)zrwUnregisterUpdateFunction;

/* 0x00561478 */
void *p_00561478 = zrwRunUpdateList;

/* 0x00561488, also gNgQuery, gNgifQuery, gZpadQuery, gZrollerQuery, gZscnptsQuery */
ZsystemGetObjectFn gZsystemGetObject = (ZsystemGetObjectFn)zrwGetParameter;

/* 0x00561738, also gFxDrawQuads, gNgDrawQuads, gZProfileDrawQuads */
ZrwDrawFn gNgifDrawQuads = (ZrwDrawFn)zrwprimDrawQuads;

/* 0x0056173c */
char sCNgEngineZgeComZglRwZrwPrimC[36] = "C:\\NgEngine\\Zge\\Com\\ZglRw\\zrwPrim.c";

/* 0x00561760 */
char sZtaginfExtraNodesInTree[36] = "ztaginf : Extra nodes in tree???";

/* 0x00561784 */
char sZtaginfTaggingInfoFaceStructureMismatch[48] = "ztaginf : Tagging info face structure mismatch";

/* 0x005617b4 */
char sChunkSyncError[20] = "Chunk sync error";

/* 0x005617c8 */
char sTagDataNotFound[20] = "Tag data not found";

/* 0x005617dc */
char sCNgEngineZgeZgeModsZtaginfC[36] = "C:\\NgEngine\\Zge\\ZgeMods\\ztaginf.c";

/* 0x00561800 */
char sFUCKEDTAGINFONORMAL[24] = "FUCKED TAG INFO NORMAL\n";

struct BmxGroup_gNgifSetViewportGroup gNgifSetViewportGroup __attribute__((aligned(16))) = {
    .gNgifSetViewport = zrwCameraSetViewport,
    .p_00561dbc = zrwCameraSetResolution,
};

/* 0x00561dc0 */
BmxType_gNgifSetViewOffset gNgifSetViewOffset = zrwCameraSetViewOffset;

/* 0x00561dc4 */
char sErrorCreatingCameraZBuffer[32] = "Error creating camera z buffer";

/* 0x00561de4 */
char sErrorCreatingCameraRaster[32] = "Error creating camera raster";

/* 0x00561e04 */
char sCNgEngineZgeComZglRwZrwCamC[36] = "C:\\NgEngine\\Zge\\Com\\ZglRw\\zrwCam.c";

/* 0x00561e28 */
char sCanTSetScreenResoltionIfCamerasExist[44] = "Can't set screen resoltion if cameras exist";

/* 0x00561e54 */
char sRwCamNoCamera[20] = "RwCam: No camera\n";

struct BmxGroup_gFxLoadTextureGroup gFxLoadTextureGroup __attribute__((aligned(16))) = {
    .gFxLoadTexture = zrwWorldFindMaterial,
    .p_005620f0 =
        {
            zrwWorldMaterialCount,
            zrwWorldGetMaterial,
            zrwworldLogInfo,
            zrwWorldHasPvs,
            zrwWorldSetupPvs,
            zrwWorldSetPvsEnabled,
            zrwWorldLoadPvs,
        },
};

/* 0x005623d8 */
void *gFxGravity[2] = {
    zrwWorldNop,
    zrwParticleAccelerate,
};

/* 0x005623e0 */
void *gFxFloorColl[2] = {
    zrwParticleNop,
    zrwParticleFloorBounce,
};

/* 0x00562404, also gNgifTextureWidth */
ZresTextureSizeFn gZresTextureWidth = (ZresTextureSizeFn)zrwTextureGetWidth;

struct BmxGroup_gZresTextureHeightGroup gZresTextureHeightGroup __attribute__((aligned(16))) = {
    .gZresTextureHeight = (ZresTextureSizeFn)zrwTextureGetHeight,
    .p_0056240c = zrwTextureGetDepth,
};

/* 0x00574668 */
uint8_t gBmxCurrentTrack;

struct BmxGroup_gBmxTimerIdGroup gBmxTimerIdGroup __attribute__((aligned(16)));

/* 0x00574694 */
uint32_t gBmxTimerPeriod;

/* 0x00574698 */
uint8_t gBmxGunChannel;

/* 0x0057469c */
BmxMenuChannelTable gBmxMenuChannels;

/* 0x0057469e */
uint8_t gBmxLevelIndex;

/* 0x0057469f */
uint8_t gBmxRiderIndex;

/* 0x005746a0 */
uint8_t gBmxAmbientSide;

/* 0x005746a4 */
struct WinstrmStream *gBmxMusicStream;

/* 0x005746a8 */
uint32_t gBmxTrackCooldown;

/* 0x005746ac */
uint8_t gBmxMenuSfxSet;

struct BmxGroup_gLastPollTimeGroup gLastPollTimeGroup __attribute__((aligned(16)));

/* 0x005746c4 */
ZVec3 gZoverArmSide;

/* 0x005746d4 */
ZVec3 gZoverLimb3Elbow;

/* 0x005746e4 */
ZMatrix gZoverLimb1Upper;

/* 0x00574724 */
ZVec3 gZoverLimb0Elbow;

/* 0x00574734 */
ZoverRigFlags gZoverRelaxed;

/* 0x0057473c */
ZoverRigFlags gZoverStiff;

/* 0x00574744 */
ZVec3 gZoverLimb3Target;

/* 0x00574754 */
ZMatrix gZoverLimb0Lower;

/* 0x00574794 */
ZMatrix gZoverFrameShin46;

/* 0x005747d4 */
ZVec3 gZoverLimb0Bend;

/* 0x005747e4 */
ZVec3 gZoverLimb0Target;

/* 0x005747f4 */
ZMatrix gZoverFrameShin35;

/* 0x00574834 */
ZVec3 gZoverLimb1Target;

/* 0x00574844 */
ZVec3 gZoverLimb2Target;

/* 0x00574854 */
ZMatrix gZoverFrameForearm911;

/* 0x00574894 */
ZMatrix gZoverSpineFrame;

/* 0x005748d4 */
ZoverRigFlags gZoverHandsEased;

/* 0x005748dc */
ZVec3 gZoverLimb2Root;

/* 0x005748ec */
ZVec3 gZoverLimb2Elbow;

/* 0x005748fc */
ZVec3 gZoverLimb1Elbow;

/* 0x0057490c */
ZVec3 gZoverLimb3Bend;

/* 0x0057491c */
ZoverRigFlags gZoverHandsGrabbed;

/* 0x00574924 */
ZoverRigFlags gZoverHandsHolding;

/* 0x0057492c */
float gZoverArmDist;

/* 0x00574930 */
ZoverRigFlags gZoverLimp;

/* 0x00574938 */
ZMatrix gZoverFrameForearm1012;

/* 0x00574978 */
ZMatrix gZoverLimb3Upper;

/* 0x005749b8 */
ZVec3 gZoverLimb1Root;

/* 0x005749c8 */
ZMatrix gZoverLimb2Upper;

/* 0x00574a08 */
float gZoverLimbUpperLen;

/* 0x00574a0c */
float gZoverLimbSin;

/* 0x00574a10 */
ZVec3 gZoverLimb1Bend;

/* 0x00574a20 */
ZVec3 gZoverLegSide;

/* 0x00574a30 */
ZVec3 gZoverLimb0Root;

/* 0x00574a40 */
ZAngle gZoverLimbAngle;

/* 0x00574a44 */
ZVec3 gZoverLimb3Root;

/* 0x00574a54 */
ZMatrix gZoverFrameHead;

/* 0x00574a94 */
ZMatrix gZoverLimb1Lower;

/* 0x00574ad4 */
ZoverRigFlags gZoverSticky;

/* 0x00574adc */
ZMatrix gZoverActorFrame;

/* 0x00574b1c */
ZoverRigFlags gZoverYawOn;

/* 0x00574b24 */
ZoverRigFlags gZoverPitchOn;

/* 0x00574b2c */
ZMatrix gZoverLimbProduct;

/* 0x00574b6c */
ZVec3 gZoverLimbDir;

/* 0x00574b7c */
float gZoverLegDist;

/* 0x00574b80 */
float gZoverLimbCos;

/* 0x00574b84 */
ZMatrix gZoverLimb3Lower;

/* 0x00574bc4 */
ZoverRigTable gZoverRigs;

/* 0x00574bcc */
float gZoverLimbDistCopy;

/* 0x00574bd0 */
ZMatrix gZoverLimb2Lower;

/* 0x00574c10 */
ZMatrix gZoverLimbTransposed;

/* 0x00574c50 */
float gZoverLimbLowerLen;

/* 0x00574c54 */
ZVec3 gZoverLimb2Bend;

/* 0x00574c64 */
ZMatrix gZoverLimb0Upper;

/* 0x00574ca4 */
int32_t gZoverRigNext;

/* 0x00574ca8 */
int32_t gZoverRigIndex;

/* 0x00574cac */
ZoverRigAngles gZoverPitch;

/* 0x00574cb0 */
ZoverRigAngles gZoverYaw;

/* 0x00574cb4 */
ZoverIndices6 gZoverHingeIdx;

/* 0x00574ccc */
ZoverIndices8 gZoverStickIdx;

/* 0x00574cec */
ZoverIndices3 gZoverBackIdx;

/* 0x00574cf8 */
ZVec3 gZoverLimbOffset;

struct BmxGroup_gZoverLimbSyncedGroup gZoverLimbSyncedGroup __attribute__((aligned(16)));

/* 0x00574e50 */
ZOverFontTable gZoverFonts;

/* 0x00574e70 */
ZOverPool3 gZoverPool3;

/* 0x00594270 */
ZOverPool2 gZoverPool2;

struct BmxGroup_gZoverRectSizeGroup gZoverRectSizeGroup __attribute__((aligned(16)));

/* 0x005965a2 */
unsigned char d_005965a2[2];

/* 0x005965a4 */
ZOverHandleList gZoverFreeList;

/* 0x00596a20 */
ZOverTerminators gZoverTerminators;

/* 0x00596a30 */
uint8_t gZoverFontCount;

/* 0x00596a38 */
ZOverPool1 gZoverPool1;

/* 0x005a0a38 */
ZOverPool0 gZoverPool0;

/* 0x005a1e38 */
ZOverHandleList gZoverDirtyList;

/* 0x005a22b0 */
uint32_t gZoverBusy;

/* 0x005a22b4 */
uint32_t gZoverDirtyCount;

/* 0x005a22b8 */
uint32_t gZoverFreeCount;

/* 0x005a22bc */
uint8_t gZoverFrame;

struct BmxGroup_gZoverLayerGroup gZoverLayerGroup __attribute__((aligned(16)));

/* 0x005ab3a0, also gGuiAnalogPad, gNgMode */
uint8_t gZsystemMode;

/* 0x005ab3ac */
BmxType_gNgSpotLights gNgSpotLights;

struct BmxGroup_gNgArtNameGroup gNgArtNameGroup __attribute__((aligned(16)));

struct BmxGroup_gNgLightsGroup gNgLightsGroup __attribute__((aligned(16)));

/* 0x005ab454, also gFxDebugFloat */
float gNgElapsed;

/* 0x005ab458 */
void *gNgMainLight;

/* 0x005ab460 */
uint32_t gNgWake;

/* 0x005ab468 */
uint32_t *gNgPrevMark;

/* 0x005ab46c */
void *gNgArt;

/* 0x005ab470 */
ZrwUpdateNode *gNgLoadNode;

/* 0x005ab478 */
uint32_t gNgCountdown;

/* 0x005ab47c */
uint32_t gNgSunTick;

/* 0x005ab480 */
uint16_t gNgLoadFlag;

/* 0x005ab488 */
void *gNgLegal1;

/* 0x005ab48c */
void *gNgLoadTexture;

/* 0x005ab490 */
void *gNgLegal2;

/* 0x005ab49c */
uint32_t gFxQualityTime;

/* 0x005ab4a0 */
int32_t gFxFrames;

/* 0x005ab4a4 */
int32_t gFxAdjusting;

struct BmxGroup_gFxQualityStepGroup gFxQualityStepGroup __attribute__((aligned(16)));

/* 0x005ab4e0 */
uint32_t gFxSparkTexture;

/* 0x005ab4e4 */
uint32_t gFxSkidTexture;

/* 0x005ab4e8 */
uint32_t gFxRippleTexture;

/* 0x005ab4ec */
uint32_t gFxSplashTexture;

/* 0x005ab4f0 */
const int8_t *gNgifScript;

/* 0x005ab4f4 */
void *gFxCube;

struct BmxGroup_gNgifPrimBufferGroup gNgifPrimBufferGroup __attribute__((aligned(16)));

/* 0x005d24fc */
int gNgifFlushPending;

/* 0x005d2500 */
int gNgifFlushNode;

/* 0x005d2504, also gZprimFrameEnded */
int gNgifFlushPending2;

/* 0x005d2508 */
int gNgifFlushNode2;

/* 0x005d250c */
uint32_t *gNgifPrimStart;

/* 0x005da1e8 */
ZactorPoseTables gZactorPose;

struct BmxGroup_gZactorArmHashesGroup gZactorArmHashesGroup __attribute__((aligned(16)));

/* 0x005e63d8 */
ZbspNearHitTable gZbspNearHits;

/* 0x005e7fd8 */
uint32_t gZbspMatRefValue;

struct BmxGroup_gZbspPropFacesGroup gZbspPropFacesGroup __attribute__((aligned(16)));

/* 0x005e8420 */
ZbspNearFaceTable gZbspNearFaces;

/* 0x005f0b98 */
unsigned char d_005f0b98[136];

/* 0x005f0c20 */
ZbspSphereResult gZbspSphere;

/* 0x005f0c28 */
ZbspFaceQuery gZbspFaceQuery;

/* 0x005f0c68 */
ZbspHitList gZbspHits;

/* 0x005f1070 */
PhysEdge gZbspEdgeRecord;

struct BmxGroup_gZbspNearCornersGroup gZbspNearCornersGroup __attribute__((aligned(16)));

/* 0x005f34ac, also gZbspFacePool */
struct ZMemFreeList *gZresListA;

/* 0x005f34b0, also gZbspEdgePool */
struct ZMemFreeList *gZresListB;

struct BmxGroup_gZbspPropFaceCountGroup gZbspPropFaceCountGroup __attribute__((aligned(16)));

/* 0x005f8518 */
BmxType_gZcollideProbeMaterial gZcollideProbeMaterial;

/* 0x005f8520 */
BmxType_gZcollideEdgeList gZcollideEdgeList;

/* 0x005f85a8 */
BmxType_gZcollideEdgeRecords gZcollideEdgeRecords;

/* 0x005f8ce0 */
BmxType_gZcollideEdgeCells gZcollideEdgeCells;

struct BmxGroup_gZbspNoShapeKindGroup gZbspNoShapeKindGroup __attribute__((aligned(16)));

/* 0x005f8f30 */
ZmemdmmState gZmemdmm;

struct BmxGroup_gZresGroup gZresGroup __attribute__((aligned(16)));

/* 0x005fb6c8 */
ZrollerDrawTable gZrollerDrawCallbacks;

/* 0x005fb6e8 */
int gZrollerReserved;

/* 0x005fb6f0 */
ZrollerSlotTable gZrollerSlots;

/* 0x005fb710 */
int32_t gZrollerShadowSlots;

/* 0x005fb718 */
ZrollerBounds gZrollerBounds;

/* 0x005fb738 */
int gZrollerSlotCount;

/* 0x005fb73c */
uint32_t gNgWord5fb73c;

struct BmxGroup_gNgRollerNodeGroup gNgRollerNodeGroup __attribute__((aligned(16)));

/* 0x00601d30 */
ZActorRootPose gActorRootPoseTemp;

/* 0x00601d48 */
ZMotionDelta gMotionLastDelta;

/* 0x00601d60 */
ZMotionEntry gMotionEntryTemp;

/* 0x00601d68 */
ZMotionDelta gMotionDeltaA;

/* 0x00601d80 */
ZMotionDelta gMotionDeltaB;

/* 0x00601d98 */
ZMotionDelta gMotionDeltaC;

/* 0x00601db0 */
ZMotionDelta gMotionDeltaD;

struct BmxGroup_gMotionDeltaEGroup gMotionDeltaEGroup __attribute__((aligned(16)));

/* 0x00601e08 */
int16_t gMotionPrevYawE;

/* 0x00601e10 */
void *gMotionFirstBank;

/* 0x00601e14 */
ZMotionDelta *gMotionSampleA;

/* 0x00601e18 */
ZMotionDelta *gMotionSampleB;

/* 0x00601e1c */
ZMotionStaging gMotionStaging;

/* 0x00641e1c */
ZActor *gZactors;

/* 0x00641e20 */
int32_t gMotionBankCount;

/* 0x00641e24 */
int32_t gActorAngleBase;

/* 0x00641e28 */
int32_t gActorAngleFlip;

/* 0x00641e2c */
int32_t gMotionLibrary;

struct BmxGroup_gMotionBufferGroup gMotionBufferGroup __attribute__((aligned(16)));

/* 0x0064eb34 */
uint32_t gZprimTextureCounter;

/* 0x0064eb38 */
ZsndNotePool gZsndNotes;

/* 0x0064f038 */
int32_t gZsndNextNote;

/* 0x0064f03c */
int32_t gZsndMuteAll;

/* 0x0064f040 */
int32_t gZsndNextTrack;

/* 0x0064f048 */
ZsndTrackPool gZsndTrackPool;

/* 0x0064f948 */
ZsndTrackTable gZsndTracks;

struct BmxGroup_gZsndSeqLockGroup gZsndSeqLockGroup __attribute__((aligned(16)));

/* 0x0064fcb8 */
WinstrmStream *gZsndStream;

/* 0x0064fcbc */
uint32_t gZsndStreamBytes;

/* 0x0064fcc8 */
int32_t gZsndSrcNoDoppler;

/* 0x0064fccc */
uint32_t gZsndSrcRange;

/* 0x0064fcd0 */
ZsndSourceTable gZsndSources;

/* 0x00650090 */
float gZsndSrcDopplerScale;

/* 0x00650094 */
uint32_t gZsndSrcRange2;

/* 0x00650098 */
int32_t gZsndSrcInitialized;

/* 0x0065009c */
uint32_t gZsndSrcNext;

/* 0x006500a0 */
uint32_t gZsndSrcLifetime;

/* 0x006500b0 */
unsigned char d_006500b0[192];

/* 0x006783fc */
int32_t gZprimHandle;

/* 0x00678400 */
int32_t gZprimLibrary;

/* 0x00678404 */
ZPrimAsset *gZprimPool;

/* 0x00678408 */
ZPrimAsset *gZprimFree;

/* 0x0067840c */
ZPrimAsset *gZprimUsed;

/* 0x00678410 */
ZPrimRect gZprimClip;

/* 0x00678418 */
ZPrimGroupTable gZprimGroups;

/* 0x00678428 */
uint16_t gZprimNextId;

/* 0x0067842c */
int32_t gZprimDepth;

/* 0x00678430 */
ZPrimState *gZprimStates;

struct BmxGroup_gZrollerShadowGroup gZrollerShadowGroup __attribute__((aligned(16)));

/* 0x0067847c */
int gZresTablesLib;

/* 0x0067848c */
int gZresMaterialLib;

/* 0x00678490, also gAssetLibrary, gOvlLibrary */
int gZresMiscLib;

struct BmxGroup_gZrollerEdgesGroup gZrollerEdgesGroup __attribute__((aligned(16)));

struct BmxGroup_gNgifPrim3DBufferGroup gNgifPrim3DBufferGroup __attribute__((aligned(16)));

/* 0x0068c3d0 */
uint32_t *gNgifOrderTable;

/* 0x0068c3e0, also gNgifTextures, gZoverSprites, gZresOverlaySlots */
ZrollerModelTable gZrollerModels;

/* 0x0068ebe0 */
uint32_t *gNgifPrim3D;

/* 0x0068ebf0 */
unsigned char d_0068ebf0[16];

/* 0x0068ec00 */
unsigned char d_0068ec00[4096];

struct BmxGroup_gNgifColorGroup gNgifColorGroup __attribute__((aligned(16)));

struct BmxGroup_gFxDustTexturesGroup gFxDustTexturesGroup __attribute__((aligned(16)));

/* 0x0068fcf8 */
int gFxAnimHandle;

/* 0x0068fcfc */
int gNgifScriptNode;

/* 0x0068fd00 */
ZfxSkidTable gFxSkids;

/* 0x0068ff80 */
uint8_t *gFxWorldObject;

/* 0x0068ff84 */
uint32_t gNgWord68ff84;

/* 0x0068ff94, also gNgFontPages, gNgifGlyphPages */
struct ZMemFreeList *gZprimFontPageList;

/* 0x0068ff98, also gCameraObject, gFxSceneObject */
ZObjCamera *gNgCamera;

/* 0x0068ffa0 */
uint8_t gNgFlags68ffa0;

/* 0x0068ffa1 */
uint8_t gNgFlags68ffa1;

/* 0x0068ffa2 */
uint8_t gNgFlags68ffa2;

/* 0x0068ffa3 */
uint8_t gNgFlags68ffa3;

/* 0x0068ffa4 */
ZrwUpdateNode *gNgTickNode;

struct BmxGroup_gZresReplayLibGroup gZresReplayLibGroup __attribute__((aligned(16)));

struct BmxGroup_gZoverSweepGroup gZoverSweepGroup __attribute__((aligned(16)));
