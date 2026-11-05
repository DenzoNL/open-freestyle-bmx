/*
 * The original's globals as C definitions (docs/PORTING.md X2a): their declarations, the types of the
 * groups (globals the code uses as one piece: a struct whose members keep their original offsets, asserted
 * below) and of the globals whose own type is local to a .c file (a same-layout copy). Included by the
 * data files only (src/data/data.h); the game's code reaches the globals through ORIG_VAR (data/addrs.h).
 *
 * Generated once by tools/gen_data.py from NgBMX.exe; from here on it is source like the code.
 */
#ifndef BMX_DATA_GLOBALS_H
#define BMX_DATA_GLOBALS_H

/* The headers' ORIG_VAR macros of these names: here the names are the definitions. */
#undef RwDefaultGlobals
#undef RwEngineInstance
#undef gAction
#undef gActionDemoTable
#undef gActionFrameCount
#undef gActionFrameReload
#undef gActionLanguage
#undef gActionMode
#undef gActionMotions
#undef gActorAngleBase
#undef gActorAngleFlip
#undef gActorRootPoseTemp
#undef gAdpcmIndex
#undef gAdpcmStep
#undef gAppTitle
#undef gAssetSetData
#undef gAssetSetSize
#undef gAutoLoadDone
#undef gAutoLoadOk
#undef gAutoSlot
#undef gAutoTick
#undef gAxesFresh
#undef gBikeCaptions
#undef gBikeLocked
#undef gBikeNames
#undef gBikePalettes
#undef gBikeStats
#undef gBikeTextures
#undef gBmxAmbientSide
#undef gBmxAmbientTracks
#undef gBmxCurrentTrack
#undef gBmxGameSfxFile
#undef gBmxGunChannel
#undef gBmxGunSfxFile
#undef gBmxLevelIndex
#undef gBmxLevelSfxFile
#undef gBmxLevelTracks
#undef gBmxMenuChannels
#undef gBmxMenuSfxFile
#undef gBmxMenuSfxSet
#undef gBmxMusicFile
#undef gBmxMusicMode
#undef gBmxMusicStream
#undef gBmxNextTrack
#undef gBmxRiderIndex
#undef gBmxRiderSfxFile
#undef gBmxSfxFile
#undef gBmxSoundEffect
#undef gBmxSoundState
#undef gBmxSpeechFile
#undef gBmxTimerId
#undef gBmxTimerPeriod
#undef gBmxTrackCooldown
#undef gBmxTrackLoop
#undef gBmxVolumes
#undef gBoxEdges
#undef gBoxFresh
#undef gButtonGlyphs
#undef gCameraPlayer
#undef gCameraRect
#undef gCameraReset
#undef gCameraTuning
#undef gCameras
#undef gCaptionAlpha
#undef gCaptionColors
#undef gCaptionHold
#undef gCaptionKinds
#undef gCaptionLine
#undef gCaptionLocked
#undef gCaptionOn
#undef gCaptionOrders
#undef gCaptionPtr
#undef gCaptionSlots
#undef gCaptionTime
#undef gCaptionWidths
#undef gCareerUnlocked
#undef gCents
#undef gCheatPos
#undef gCheatRing
#undef gContrlerAxes
#undef gContrlerBindings
#undef gContrlerMode
#undef gContrlerPads
#undef gCreditsMovie
#undef gCrtFloatHooks
#undef gCrtFpStatus
#undef gCurrentPlayer
#undef gDemoPlayer
#undef gDirectDraw
#undef gDsVolume
#undef gEnvelopeMs
#undef gEpicMessage
#undef gErrorCaption
#undef gExitCode
#undef gExitFlag
#undef gExtrasChoice
#undef gExtrasCount
#undef gExtrasPlayed
#undef gExtrasReady
#undef gExtrasSel
#undef gExtrasSlide
#undef gExtrasTitles
#undef gExtrasWait
#undef gExtrasWait2
#undef gFont
#undef gFormatBuffer
#undef gFrontendStep
#undef gFxActive
#undef gFxAdjusting
#undef gFxAnimHandle
#undef gFxBurstBlock
#undef gFxCreateBlock
#undef gFxCube
#undef gFxDust
#undef gFxDustDefault
#undef gFxDustTextures
#undef gFxEmitBlock
#undef gFxFindTexture
#undef gFxFloorColl
#undef gFxFps
#undef gFxFrames
#undef gFxGravity
#undef gFxGravityParams
#undef gFxHandlerTable
#undef gFxLightCallback
#undef gFxLightning
#undef gFxListCallback
#undef gFxLoadTexture
#undef gFxPoster
#undef gFxPresets
#undef gFxPuffBlock
#undef gFxQuality
#undef gFxQualityStep
#undef gFxQualityTime
#undef gFxRider
#undef gFxRipple
#undef gFxRippleDefault
#undef gFxRippleTexture
#undef gFxScatterSlots
#undef gFxSetRaster
#undef gFxShard
#undef gFxSkidBase
#undef gFxSkidConfig
#undef gFxSkidCorners
#undef gFxSkidFrameNames
#undef gFxSkidName
#undef gFxSkidTexture
#undef gFxSkids
#undef gFxSpark
#undef gFxSparkCreateDefault
#undef gFxSparkDefault
#undef gFxSparkTexture
#undef gFxSplash
#undef gFxSplashTexture
#undef gFxTextureByHash
#undef gFxUnused
#undef gFxWorldObject
#undef gFxZero
#undef gGainCurve
#undef gGameDefaultWord
#undef gGameExtent
#undef gGameFrame
#undef gGameFrozen
#undef gGameGoals
#undef gGameHorseCount
#undef gGameHorseTricks
#undef gGameHorseWord
#undef gGameJudgeBias
#undef gGameJudgePar
#undef gGameJudgeScores
#undef gGameLevelInfoIndex
#undef gGameMarkerColors
#undef gGameMarkersA
#undef gGameMarkersB
#undef gGameMode
#undef gGameModeSel
#undef gGameOptions
#undef gGamePhase
#undef gGamePhaseTicks
#undef gGameReplayAll
#undef gGameReplayShared
#undef gGameReplays
#undef gGameRideSeconds
#undef gGameRoundCounts
#undef gGameRoundStates
#undef gGameRunCount
#undef gGameRunType
#undef gGameStartPlayer
#undef gGameStartPos
#undef gGameSubMode
#undef gGameTimer
#undef gGdiSurface
#undef gGoalEntriesA
#undef gGoalEntriesB
#undef gGuiAssetFile
#undef gGuiAssetsLoaded
#undef gGuiDefaultHook
#undef gGuiElementCount
#undef gGuiElementMagic
#undef gGuiElements
#undef gGuiInitialized
#undef gGuiMarkupTarget
#undef gGuiModeReady
#undef gGuiPad
#undef gGuiPadRepeatCount
#undef gGuiPadRepeatHeld
#undef gGuiPulse
#undef gGuiPulseHold
#undef gGuiPulseStep
#undef gGuiReloadDelay
#undef gGuiReloadPending
#undef gGuiScreen
#undef gGuiScreenId
#undef gGuiScreenLoaded
#undef gGuiScreenMagic
#undef gGuiSkipUpdate
#undef gGuiSlideMax
#undef gGuiSlideState
#undef gGuiSorted
#undef gHiscoreBlink
#undef gHiscoreHandle
#undef gHiscoreLabels
#undef gHiscoreNames
#undef gHiscorePictureIds
#undef gHiscoreRiders
#undef gHiscoreSlide
#undef gHiscoreSprite
#undef gHiscoreTimes
#undef gHiscoreWait
#undef gIndicatorColor
#undef gIndicatorText
#undef gInfoHeadIds
#undef gInfoLineIds
#undef gInfoPage
#undef gInfoSlide
#undef gInfoSprite
#undef gInfoTicks
#undef gInfoWait
#undef gItems
#undef gKbCursor
#undef gKbLayout
#undef gKbLayoutLabels
#undef gKbLayoutTitles
#undef gKbState
#undef gKbWork
#undef gKeyNames
#undef gKeyPlayer
#undef gLanguageSel
#undef gLanguageTable
#undef gLastPollTime
#undef gLastTick
#undef gLevelNames
#undef gLevelTextures
#undef gLineFresh
#undef gMacroBuf
#undef gMacroSize
#undef gMcChoice
#undef gMcCountdown
#undef gMcDone
#undef gMcLoadOk
#undef gMcMode
#undef gMcRows
#undef gMcSaveOk
#undef gMcSlot
#undef gMcState
#undef gMenuEntryStrings
#undef gMenuFrameSprite
#undef gMenuInstant
#undef gMenuSlide0
#undef gMenuSlide1
#undef gMenuSlide2
#undef gMenuSlide3
#undef gMenuSlotAlpha
#undef gMenuSlotPos
#undef gMenuSlotTint
#undef gMotionBankCount
#undef gMotionBuffer
#undef gMotionDeltaA
#undef gMotionDeltaB
#undef gMotionDeltaC
#undef gMotionDeltaD
#undef gMotionDeltaE
#undef gMotionEntryTemp
#undef gMotionFirstBank
#undef gMotionLastDelta
#undef gMotionLibrary
#undef gMotionLoadedBank
#undef gMotionMirrorMap
#undef gMotionPrevYawE
#undef gMotionSampleA
#undef gMotionSampleB
#undef gMotionStaging
#undef gMotionTimeScale
#undef gMovie
#undef gNameTables
#undef gNameTablesReady
#undef gNgArt
#undef gNgArtName
#undef gNgAssetsDir
#undef gNgCamera
#undef gNgCountdown
#undef gNgDebugEdgeColor
#undef gNgDebugFaceColor
#undef gNgDebugRollerColor
#undef gNgDebugTaggedColor
#undef gNgElapsed
#undef gNgFlags68ffa0
#undef gNgFlags68ffa1
#undef gNgFlags68ffa2
#undef gNgFlags68ffa3
#undef gNgKeyMaps
#undef gNgLegal1
#undef gNgLegal2
#undef gNgLegalPending
#undef gNgLegalTime
#undef gNgLevelNames
#undef gNgLightColors
#undef gNgLightDir
#undef gNgLightRef
#undef gNgLights
#undef gNgLoadFlag
#undef gNgLoadNode
#undef gNgLoadTexture
#undef gNgMainLight
#undef gNgMainLightColor
#undef gNgPrevMark
#undef gNgRamDisk
#undef gNgRollerNode
#undef gNgSpotLights
#undef gNgSpotMainColor
#undef gNgSunTick
#undef gNgTickNode
#undef gNgWake
#undef gNgWord5fb73c
#undef gNgWord68ff84
#undef gNgWrapName
#undef gNgifBrightness
#undef gNgifColor
#undef gNgifDrawQuads
#undef gNgifFlushNode
#undef gNgifFlushNode2
#undef gNgifFlushPending
#undef gNgifFlushPending2
#undef gNgifOrderTable
#undef gNgifPrim3D
#undef gNgifPrim3DBuffer
#undef gNgifPrimBuffer
#undef gNgifPrimStart
#undef gNgifScreenH
#undef gNgifScript
#undef gNgifScriptNode
#undef gNgifSetViewOffset
#undef gNgifSetViewport
#undef gNoMovies
#undef gOptCount
#undef gOptMainPulse
#undef gOptMainPulseStep
#undef gOptMainReady
#undef gOptMainSel
#undef gOptRows
#undef gOptSaveDelay
#undef gOptSel
#undef gOptSlide
#undef gOptStateStrings
#undef gOptTicks
#undef gOptionFlags
#undef gOptionTable
#undef gOvlBackdrop
#undef gOvlBackdropRect
#undef gOvlBalanceHandles
#undef gOvlBalanceValue
#undef gOvlBarSize
#undef gOvlCaptionCurrent
#undef gOvlCaptionHandle
#undef gOvlCaptionWanted
#undef gOvlDrawDelay
#undef gOvlElement8Value
#undef gOvlElements
#undef gOvlEmpty
#undef gOvlFade
#undef gOvlFadeSize
#undef gOvlFlash
#undef gOvlFlashHandles
#undef gOvlFontButton
#undef gOvlFontIcons
#undef gOvlFontTiny
#undef gOvlFrame
#undef gOvlGoalHandles
#undef gOvlGoalMsg
#undef gOvlGoalRows
#undef gOvlHeightHandles
#undef gOvlHeightKind
#undef gOvlHeightScroll
#undef gOvlHeightTarget
#undef gOvlHeightValue
#undef gOvlHint
#undef gOvlHintHandles
#undef gOvlIconHandle
#undef gOvlImgButton
#undef gOvlImgIcons
#undef gOvlImgSmall
#undef gOvlImgTiny
#undef gOvlIndicator
#undef gOvlLastScreen
#undef gOvlMenuItems
#undef gOvlMenuItems2
#undef gOvlMpModeLines
#undef gOvlMpWord
#undef gOvlMpWordCursor
#undef gOvlNameCursor
#undef gOvlPage
#undef gOvlPulse
#undef gOvlResA
#undef gOvlReturnScreen
#undef gOvlRunTimes
#undef gOvlSavePages
#undef gOvlSaveSlot
#undef gOvlSaveStep
#undef gOvlSavedSubMode
#undef gOvlSaving
#undef gOvlScoreFlags
#undef gOvlScoreHandle
#undef gOvlScoreShown
#undef gOvlScoreTarget
#undef gOvlScreen
#undef gOvlScreenId
#undef gOvlScreens
#undef gOvlSharedPad
#undef gOvlStatus
#undef gOvlStatusHandle
#undef gOvlStrings
#undef gOvlTimerHandle
#undef gOvlTimerShownTicks
#undef gOvlTimerText
#undef gOvlTimerTicks
#undef gOvlTrickCount
#undef gOvlTrickList
#undef gOvlTrickRows
#undef gOvlTrickView
#undef gOvlVolHandles
#undef gPanorama
#undef gPcProfileClock
#undef gPhysAirDrag
#undef gPhysAirProbe
#undef gPhysAirSpeedCap
#undef gPhysAirTurnSteps
#undef gPhysAlignEdge
#undef gPhysBailImpact
#undef gPhysBestEdge
#undef gPhysDebugDraw
#undef gPhysDebugEdges
#undef gPhysDebugFrame
#undef gPhysDebugTexture
#undef gPhysDown
#undef gPhysEdgeProbe
#undef gPhysEdgeSearchRadius
#undef gPhysForceStep
#undef gPhysGravity
#undef gPhysGrindBalance
#undef gPhysGrindEdge
#undef gPhysGrindInRadius
#undef gPhysGrindInSteps
#undef gPhysGrindMinSpeed
#undef gPhysGrindYawDamp
#undef gPhysHudFont
#undef gMotionBanks
#undef gPhysHudHandle
#undef gPhysIkLimbMasks
#undef gPhysLandCrashSpeed
#undef gPhysLeanMax
#undef gPhysLeanRange
#undef gPhysLeanSpeedMax
#undef gPhysLipFace
#undef gPhysMaxSlope
#undef gPhysMotions
#undef gPhysNearEdge
#undef gPhysPipeAngleLimit
#undef gPhysPipeLaunchScale
#undef gPhysPipeTiltLimit
#undef gPhysRailAngleLimit
#undef gPhysRay
#undef gPhysRestSpeed
#undef gPhysRollFriction
#undef gPhysRollSpinSpeed
#undef gPhysRollSteer
#undef gPhysRollYawDamp
#undef gPhysSlopeAngle
#undef gPhysStallInSteps
#undef gPhysStateEnter
#undef gPhysStateExit
#undef gPhysStateMove
#undef gPhysStateOrient
#undef gPhysStateSync
#undef gPhysStateVelocity
#undef gPhysSuspGravityScale
#undef gPhysSyncFlags
#undef gPhysTick
#undef gPhysUpBlend
#undef gPhysWallAngle
#undef gPhysWheelSideSlip
#undef gPlayerCount
#undef gPlayerRecs
#undef gPrimCursor
#undef gProfiles
#undef gPropDefaults
#undef gPropDefs
#undef gPropNodeStates
#undef gPropRecords
#undef gPropState
#undef gRateB
#undef gRateC
#undef gReplays
#undef gRiderAirStep
#undef gRiderAirTrickPick
#undef gRiderAirTurnMax
#undef gRiderAirTurnNudge
#undef gRiderAirTurnStart
#undef gRiderAirYawDamp
#undef gRiderBikeSpinMax
#undef gRiderBikeSpinStep
#undef gRiderBikeStats
#undef gRiderCaptions
#undef gRiderData
#undef gRiderGrindStep
#undef gRiderHopBoost
#undef gRiderHopMargin
#undef gRiderHopSpeed
#undef gRiderJumpBase
#undef gRiderJumpFrames
#undef gRiderJumpPower
#undef gRiderLandGrip
#undef gRiderLipStep
#undef gRiderManualStep
#undef gRiderModeStep
#undef gRiderNames
#undef gRiderPoseAnim
#undef gRiderRollStep
#undef gRiderScoreWeight
#undef gRiderSignatureTricks
#undef gRiderSteerBase
#undef gRiderSteerFalloff
#undef gRiderStickDir
#undef gRiderTextures
#undef gRiderTrickA
#undef gRiderTrickAnim
#undef gRiderTrickB
#undef gRiderTrickCombo
#undef gRiderTrickKind
#undef gRiderTrickSteer
#undef gRwFree
#undef gRwMalloc
#undef gRwRealloc
#undef gRwTextureClone
#undef gScene
#undef gSceneAnimHandlers
#undef gSceneModelHandlers
#undef gScnpts
#undef gScoreRecordSlot
#undef gScoreRound
#undef gScoreSpinMul
#undef gScoreTierAdd
#undef gScoreTierMul
#undef gScoreToTime
#undef gScoreTrick
#undef gScratch
#undef gScreenReturnArg
#undef gScreenReturnFn
#undef gScrollTick
#undef gSelBike
#undef gSelBikeCount
#undef gSelCommitFn
#undef gSelDimA
#undef gSelDimB
#undef gSelExtentA
#undef gSelExtentB
#undef gSelExtentC
#undef gSelFlipTick
#undef gSelLevel
#undef gSelLevelCount
#undef gSelLevelSetting
#undef gSelPage
#undef gSelPages
#undef gSelPending
#undef gSelPicIds
#undef gSelPicturesOn
#undef gSelPlayer
#undef gSelPreviewFn
#undef gSelReadyTick
#undef gSelResume
#undef gSelRider
#undef gSelSlide
#undef gSelSlideDir
#undef gSelSprites
#undef gSelStage
#undef gSelStyle
#undef gSelStyleCount
#undef gSelTextIds
#undef gSelTick
#undef gSelUnknown60
#undef gSelUnknownA0
#undef gSemitones
#undef gSkeletonChain
#undef gSkeletonOffsets
#undef gSlotSel
#undef gSoundLevel0
#undef gSoundLevel1
#undef gSoundSel
#undef gSoundSlideA
#undef gSoundSlideB
#undef gSoundVolumes
#undef gSpherePointers
#undef gSphereVerts
#undef gStatLayers
#undef gStatPositions
#undef gStatWidths
#undef gStripColors
#undef gStripScale0
#undef gStripScale1
#undef gStyleFrame
#undef gStyleGroups
#undef gStyleSprites
#undef gStyleTextures
#undef gTextEnd
#undef gTextQueue
#undef gTimeGetTime
#undef gTitleAction
#undef gTitleAmish
#undef gTitleDave
#undef gTitleDelay
#undef gTitleFrames
#undef gTitleIdle
#undef gTitleIntro
#undef gTitleRyan
#undef gTitleSel
#undef gTitleStage
#undef gTitleUnknown551fd4
#undef gToken
#undef gTriangleFresh
#undef gTrickClassHi
#undef gTrickClassLo
#undef gTrickDoubleNames
#undef gTrickSpinNames
#undef gTrickTurnNames
#undef gWinchanActive
#undef gWinchanBuffer
#undef gWinchanChannels
#undef gWinchanChunkBytes
#undef gWinchanCopyOffset
#undef gWinchanDsoundCreate
#undef gWinchanDsoundModule
#undef gWinchanEchoBuffer
#undef gWinchanEchoFeedback
#undef gWinchanEchoPos
#undef gWinchanEffectAFrames
#undef gWinchanEffectAGain
#undef gWinchanEffectBFrames
#undef gWinchanEffectBGain
#undef gWinchanEffectBTail
#undef gWinchanEffectBTailStart
#undef gWinchanEffectMaxFrames
#undef gWinchanFlangerDelay
#undef gWinchanFlangerLine
#undef gWinchanFlangerPhase
#undef gWinchanFlangerPos
#undef gWinchanInitialized
#undef gWinchanLastPlay
#undef gWinchanLfoTable
#undef gWinchanLock
#undef gWinchanMixLeft
#undef gWinchanMixOffset
#undef gWinchanMixRight
#undef gWinchanNextChannel
#undef gWinchanNextHandle
#undef gWinchanPrimary
#undef gWinchanPumpCount
#undef gWinchanRate
#undef gWinchanRing
#undef gWinchanRingBytes
#undef gWinchanWriteOffset
#undef gWindow0
#undef gWindow1
#undef gWinloadCounts
#undef gWinloadFiles
#undef gWinloadSampleFile
#undef gWinstrmDirectSound
#undef gWinstrmInitialized
#undef gWinstrmLock
#undef gWinstrmNodeHint
#undef gWinstrmNodes
#undef gWinstrmPaused
#undef gWinstrmSourceHint
#undef gWinstrmSources
#undef gWinstrmStreamHint
#undef gWinstrmStreams
#undef gWinstrmTick
#undef gZProfile
#undef gZProfileColors
#undef gZProfilePrintAt
#undef gZactorArmHashes
#undef gZactorMax
#undef gZactorPose
#undef gZactors
#undef gZanimInitialized
#undef gZassetLibraries
#undef gZbspEdgeRecord
#undef gZbspFaceQuery
#undef gZbspHits
#undef gZbspMatRefValue
#undef gZbspMaterialFn
#undef gZbspNearCorners
#undef gZbspNearFaces
#undef gZbspNearHits
#undef gZbspNoShapeKind
#undef gZbspObjectFrameFn
#undef gZbspPropFaceCount
#undef gZbspPropFaces
#undef gZbspSphere
#undef gZcollideBarEdge
#undef gZcollideBoxEdges
#undef gZcollideEdgeCells
#undef gZcollideEdgeList
#undef gZcollideEdgeRecords
#undef gZcollideProbeMaterial
#undef gZdebugCreated
#undef gZdebugFailHandler
#undef gZdebugIndent
#undef gZdebugIndentLevel
#undef gZdebugPrint
#undef gZdebugStart
#undef gZfileDefaultDevice
#undef gZfileDevices
#undef gZfileErrorMessage
#undef gZfileHandles
#undef gZfileInitialized
#undef gZfileLoadPool
#undef gZfileNameBuffer
#undef gZfileRamDisk
#undef gZgeInitialized
#undef gZglAllocHookNop
#undef gZglExitCol
#undef gZglExitLight
#undef gZglExitMaterial
#undef gZglExitModel
#undef gZglExitPrim
#undef gZglExitTexture
#undef gZglFileDeviceRam
#undef gZglFileDeviceRw
#undef gZglInitCol
#undef gZglInitLight
#undef gZglInitMaterial
#undef gZglInitModel
#undef gZglInitPrim
#undef gZglInitTexture
#undef gZglInstance
#undef gZglMemoryFuncs
#undef gZglSubSystem
#undef gZglSubSystemCount
#undef gZglSubSystemNames
#undef gZglVideoMode
#undef gZglWantDepth
#undef gZglWantH2
#undef gZglWantW2
#undef gZiff
#undef gZiffAnim
#undef gZiffItemPool
#undef gZinputReady
#undef gZmathOrigin
#undef gZmathTables
#undef gZmem
#undef gZmemFreeListInUse
#undef gZmemFreeLists
#undef gZmemHooks
#undef gZmemHooksInstalled
#undef gZmemdmm
#undef gZobjCameraCaches
#undef gZobjCameraHome
#undef gZobjCameraInitialized
#undef gZobjCameraTarget
#undef gZobjCameraVtable
#undef gZobjDebugNode
#undef gZobjInitialized
#undef gZobjLightInitialized
#undef gZobjLightOps
#undef gZobjLinkPool
#undef gZobjModelReady
#undef gZobjPEmitInitialized
#undef gZobjPool
#undef gZobjSetColor
#undef gZobjUpdateNode
#undef gZoverActorFrame
#undef gZoverArmDist
#undef gZoverArmSide
#undef gZoverBackIdx
#undef gZoverBusy
#undef gZoverDirtyCount
#undef gZoverDirtyList
#undef gZoverFontCount
#undef gZoverFonts
#undef gZoverFrame
#undef gZoverFrameForearm1012
#undef gZoverFrameForearm911
#undef gZoverFrameHead
#undef gZoverFrameShin35
#undef gZoverFrameShin46
#undef gZoverFreeCount
#undef gZoverFreeList
#undef gZoverGlyph
#undef gZoverHandsEased
#undef gZoverHandsGrabbed
#undef gZoverHandsHolding
#undef gZoverHingeIdx
#undef gZoverLayer
#undef gZoverLegDist
#undef gZoverLegSide
#undef gZoverLimb0Bend
#undef gZoverLimb0Elbow
#undef gZoverLimb0Lower
#undef gZoverLimb0Root
#undef gZoverLimb0Target
#undef gZoverLimb0Upper
#undef gZoverLimb1Bend
#undef gZoverLimb1Elbow
#undef gZoverLimb1Lower
#undef gZoverLimb1Root
#undef gZoverLimb1Target
#undef gZoverLimb1Upper
#undef gZoverLimb2Bend
#undef gZoverLimb2Elbow
#undef gZoverLimb2Lower
#undef gZoverLimb2Root
#undef gZoverLimb2Target
#undef gZoverLimb2Upper
#undef gZoverLimb3Bend
#undef gZoverLimb3Elbow
#undef gZoverLimb3Lower
#undef gZoverLimb3Root
#undef gZoverLimb3Target
#undef gZoverLimb3Upper
#undef gZoverLimbAngle
#undef gZoverLimbCos
#undef gZoverLimbDir
#undef gZoverLimbDistCopy
#undef gZoverLimbLowerLen
#undef gZoverLimbOffset
#undef gZoverLimbProduct
#undef gZoverLimbSin
#undef gZoverLimbSynced
#undef gZoverLimbTransposed
#undef gZoverLimbUpperLen
#undef gZoverLimp
#undef gZoverOt
#undef gZoverPitch
#undef gZoverPitchOn
#undef gZoverPool0
#undef gZoverPool1
#undef gZoverPool2
#undef gZoverPool3
#undef gZoverRectSize
#undef gZoverRelaxed
#undef gZoverRigIndex
#undef gZoverRigNext
#undef gZoverRigs
#undef gZoverSpineFrame
#undef gZoverStickIdx
#undef gZoverSticky
#undef gZoverStiff
#undef gZoverSweep
#undef gZoverTerminators
#undef gZoverYaw
#undef gZoverYawOn
#undef gZpadDefaultButtonMap
#undef gZpadDevices
#undef gZpadDiActive
#undef gZpadDiRecords
#undef gZpadDirectInput
#undef gZpadFormatJoystick
#undef gZpadFormatKeyboard
#undef gZpadGuidKeyboard
#undef gZpadIidDevice
#undef gZpadIidDirectInput
#undef gZpadJoystickGuid
#undef gZpadKeyMapFirst
#undef gZpadKeyMapOther
#undef gZpadKeyState
#undef gZpadKeyboard
#undef gZpadKeyboardFlags
#undef gZpadKeyboardOk
#undef gZpadProfiles
#undef gZpadWindow
#undef gZprimClip
#undef gZprimDepth
#undef gZprimFontPageList
#undef gZprimFree
#undef gZprimGroups
#undef gZprimHandle
#undef gZprimLibrary
#undef gZprimNextId
#undef gZprimPool
#undef gZprimStates
#undef gZprimTextureCounter
#undef gZprimUsed
#undef gZpropDefs
#undef gZpropInsts
#undef gZramdiskDeviceName
#undef gZramdiskFiles
#undef gZramdiskName
#undef gZres
#undef gZresListA
#undef gZresListB
#undef gZresLoadTexture
#undef gZresMaterialLib
#undef gZresMiscLib
#undef gZresOverlaySources
#undef gZresReplayLib
#undef gZresTablesLib
#undef gZresTextureHeight
#undef gZresTextureWidth
#undef gZrollerBikeNames
#undef gZrollerBounds
#undef gZrollerDrawCallbacks
#undef gZrollerEdges
#undef gZrollerFreeHandle
#undef gZrollerModels
#undef gZrollerRegister
#undef gZrollerReserved
#undef gZrollerRiderNames
#undef gZrollerShadow
#undef gZrollerShadowSlots
#undef gZrollerSlotCount
#undef gZrollerSlots
#undef gZrollerUnregister
#undef gZrwArgBuffer
#undef gZrwArgument
#undef gZrwBeginHook
#undef gZrwCamCreate
#undef gZrwCamDestroy
#undef gZrwCamInit
#undef gZrwCamPool
#undef gZrwCamReady
#undef gZrwCamSetFov
#undef gZrwCamSetView
#undef gZrwClearColor
#undef gZrwClearColor2
#undef gZrwClearOn
#undef gZrwCol
#undef gZrwColBox
#undef gZrwColColor
#undef gZrwColDebug
#undef gZrwColEnabled
#undef gZrwColHit
#undef gZrwColLineFn
#undef gZrwColList
#undef gZrwColQuery
#undef gZrwColSphereFn
#undef gZrwCurrentCamera
#undef gZrwDbgCallbacks
#undef gZrwDbgCameraObject
#undef gZrwDbgCharHeight
#undef gZrwDbgCharWidth
#undef gZrwDbgCharset
#undef gZrwDbgFpsMode
#undef gZrwDbgFrameStart
#undef gZrwDbgGetCamera
#undef gZrwDbgInitialized
#undef gZrwDbgItemCount
#undef gZrwDbgLineHook
#undef gZrwDbgSphereHook
#undef gZrwDbgTriangleHook
#undef gZrwDbgUpdateNode
#undef gZrwDefaultBounce
#undef gZrwDefaultGravity
#undef gZrwDeviceName
#undef gZrwDrawPolys
#undef gZrwDrawSphere
#undef gZrwDrawTriangle
#undef gZrwEndHook
#undef gZrwEngineInit
#undef gZrwEngineOpen
#undef gZrwFileInitialized
#undef gZrwFileOps
#undef gZrwFlags
#undef gZrwFps
#undef gZrwFpsTime
#undef gZrwFrameLimit
#undef gZrwFrames
#undef gZrwFullscreen
#undef gZrwGraphicLibraryInitialize
#undef gZrwInUpdate
#undef gZrwInitialized
#undef gZrwKeyActions
#undef gZrwKeysLocked
#undef gZrwLastTime
#undef gZrwLightColor
#undef gZrwLightCreate
#undef gZrwLightDestroy
#undef gZrwLightDirection
#undef gZrwLightFlags
#undef gZrwLightInitialized
#undef gZrwLightKinds
#undef gZrwLightParamA
#undef gZrwLightParamB
#undef gZrwLightPool
#undef gZrwLightSetParam
#undef gZrwLightTypes
#undef gZrwLodDefaults
#undef gZrwMat
#undef gZrwMatPluginOffset
#undef gZrwMaterialTexture
#undef gZrwMdlPluginOffset
#undef gZrwModelColor1
#undef gZrwModelColor2
#undef gZrwModelCreate
#undef gZrwModelFlag2
#undef gZrwModelRender
#undef gZrwModelState
#undef gZrwNameBuffer
#undef gZrwNodePool
#undef gZrwOpenModes
#undef gZrwOpsSimple
#undef gZrwOpsSkinned
#undef gZrwOrigin
#undef gZrwPathKey
#undef gZrwPrim
#undef gZrwPrimCamera
#undef gZrwPrimDepthBias
#undef gZrwPrimFrame
#undef gZrwPvsEnabled
#undef gZrwPvsKeys
#undef gZrwPvsLoaded
#undef gZrwQuit
#undef gZrwRamDiskDevice
#undef gZrwRamDiskFile
#undef gZrwRamDiskInfo
#undef gZrwRamDiskInfoSlots
#undef gZrwRamDiskNoInfo
#undef gZrwRaster
#undef gZrwRenderExtra
#undef gZrwRenderFlags
#undef gZrwRenderStateHook
#undef gZrwSavedFileInterface
#undef gZrwScale
#undef gZrwScreenH
#undef gZrwScreenW
#undef gZrwSeekOrigins
#undef gZrwShadowActive
#undef gZrwShadowDouble
#undef gZrwShadowInit
#undef gZrwShadowMiss
#undef gZrwShadowNode
#undef gZrwShadowOffsets
#undef gZrwShadowPoints
#undef gZrwShadowQuads
#undef gZrwShadowReady
#undef gZrwShadowScale
#undef gZrwShadowTexture
#undef gZrwShadowVertices
#undef gZrwSkipDevice
#undef gZrwStateDepth
#undef gZrwStateStack
#undef gZrwTextureHash
#undef gZrwTextureLoad
#undef gZrwTextureRetain
#undef gZrwTxr
#undef gZrwUpdateLists
#undef gZrwVtClump
#undef gZrwVtDestroy
#undef gZrwVtEntry
#undef gZrwVtFindNode
#undef gZrwVtInstance
#undef gZrwVtKey
#undef gZrwVtLoadChunk
#undef gZrwVtLoadModel
#undef gZrwVtMatCount
#undef gZrwVtNodeHashes
#undef gZrwVtNodeMatrix
#undef gZrwVtNodeToWorld
#undef gZrwVtNodeVector
#undef gZrwVtOwner
#undef gZrwVtPartCount
#undef gZrwVtSetFlags
#undef gZrwVtSetPos
#undef gZrwVtSetRot
#undef gZrwVtSetScale
#undef gZrwVtSkeleton
#undef gZrwVtUpdate
#undef gZrwWantH
#undef gZrwWantW
#undef gZrwWindow
#undef gZrwWorld
#undef gZrwWorldActive
#undef gZrwWorldCenter
#undef gZrwWorldCenterScaled
#undef gZrwWorldClose
#undef gZrwWorldEffectOps
#undef gZrwWorldFind
#undef gZrwWorldLoad
#undef gZrwWorldMaterials
#undef gZrwWorldName
#undef gZrwWorldPathArgs
#undef gZrwWorldReady
#undef gZrwWorldRecord
#undef gZrwWorldSetBounds
#undef gZrwWorldSetHook
#undef gZrwWorldStats
#undef gZrwWorldTexDrop
#undef gZrwWorldTexName
#undef gZrwZRaster
#undef gZsndMuteAll
#undef gZsndNextNote
#undef gZsndNextTrack
#undef gZsndNotes
#undef gZsndSeqLock
#undef gZsndSources
#undef gZsndSrcDopplerScale
#undef gZsndSrcInitialized
#undef gZsndSrcLifetime
#undef gZsndSrcNext
#undef gZsndSrcNoDoppler
#undef gZsndSrcRange
#undef gZsndSrcRange2
#undef gZsndStream
#undef gZsndStreamBytes
#undef gZsndTicksPerSec
#undef gZsndTrackPool
#undef gZsndTracks
#undef gZstrTempBuffer
#undef gZstringSets
#undef gZsystemGetObject
#undef gZsystemMode
#undef gZtaginfData
#undef gZtaginfFaceCount
#undef gZtaginfFaces
#undef gZtaginfNodes
#undef gZtaginfRoot
#undef gZtaginfUnused
#undef gZtimerFree
#undef gZtimerHeap
#undef gZtimerPopped
#undef gZtimerReady
#undef gZtimerSlots
#undef gZtimerUpdate
#undef kZrwBBoxFaces
#undef kZrwExtBmp
#undef kZrwExtPng
#undef kZrwFormatBmp
#undef kZrwFormatDds
#undef kZrwFormatPng
#undef rpAnimClumpOffset
#undef rpAnimDurationStart
#undef rpAnimEngineOffset
#undef rpAnimFrameOffset
#undef rpAnimHeadName
#undef rpAnimModuleCount
#undef rpAnimStartStep
#undef rpAnimSumStartA
#undef rpAnimSumStartB
#undef rpAnimTimeOffset
#undef rpAtomicRegistry
#undef rpBinaryWorldModuleCount
#undef rpBoneEngineOffset
#undef rpBoneFrameOffset
#undef rpBoneGeometryOffset
#undef rpBoneModuleCount
#undef rpBoneStreamResult
#undef rpBspModuleCount
#undef rpClumpModuleCount
#undef rpClumpModuleOffset
#undef rpClumpRegistry
#undef rpGeometryModuleCount
#undef rpGeometryOffset
#undef rpGeometryRegistry
#undef rpLightModuleCount
#undef rpLightOffset
#undef rpLightRegistry
#undef rpLodAtomicOffset
#undef rpLodCamera
#undef rpLodEngineOffset
#undef rpLodFreeList
#undef rpMaterialDefaultSurfaceProps
#undef rpMaterialListModuleCount
#undef rpMaterialModuleCount
#undef rpMaterialOffset
#undef rpMaterialRegistry
#undef rpMeshEmptyHeader
#undef rpMeshModuleCount
#undef rpMeshOffset
#undef rpMeshStripMode
#undef rpPVSCamera
#undef rpPVSImage
#undef rpPVSSectorOffset
#undef rpPVSWorld
#undef rpPVSWorldOffset
#undef rpSectorModuleCount
#undef rpSphereMinDistance
#undef rpSphereMinRadius
#undef rpSphereMinRadiusSq
#undef rpTieModuleCount
#undef rpTieOffset
#undef rpWorldAtomicOffset
#undef rpWorldCameraOffset
#undef rpWorldClumpOffset
#undef rpWorldDefaultSurfaceProps
#undef rpWorldGlobalsOffset
#undef rpWorldLightOffset
#undef rpWorldModuleCount
#undef rpWorldReadScale0
#undef rpWorldReadScale1
#undef rpWorldReadScale2
#undef rpWorldRegistry
#undef rpWorldScale0
#undef rpWorldScale1
#undef rpWorldScale2
#undef rpWorldSectorRegistry
#undef rtCharsetGlyphs
#undef rwCameraFpuSet
#undef rwCameraModuleCount
#undef rwCameraOffset
#undef rwCameraRegistry
#undef rwColorModuleCount
#undef rwD3D7Device
#undef rwD3DAlphaTestCaps
#undef rwD3DAndClip
#undef rwD3DAtomicDefaultColor
#undef rwD3DBufferSize
#undef rwD3DCamBase
#undef rwD3DCamBuffer
#undef rwD3DCameraExtOffset
#undef rwD3DCaps
#undef rwD3DClipLine
#undef rwD3DClipTri
#undef rwD3DClipTriCull
#undef rwD3DContext
#undef rwD3DCurrentCamera
#undef rwD3DDefer
#undef rwD3DEngineDevice
#undef rwD3DFarScreenZ
#undef rwD3DFarZ
#undef rwD3DFlipPending
#undef rwD3DFogEnd
#undef rwD3DFogRange
#undef rwD3DFogScale
#undef rwD3DFogStart
#undef rwD3DIm3DDepth
#undef rwD3DIm3DLevel
#undef rwD3DIm3DWork
#undef rwD3DIndexFlush
#undef rwD3DIndexLine
#undef rwD3DIndexStart
#undef rwD3DIndexTri
#undef rwD3DK0x0069b530
#undef rwD3DK0x0069b534
#undef rwD3DK0x0069b538
#undef rwD3DK0x0069b544
#undef rwD3DK0x0069b548
#undef rwD3DK0x0069b54c
#undef rwD3DK0x0069b550
#undef rwD3DK0x0069b554
#undef rwD3DK0x0069b558
#undef rwD3DK0x0069b55c
#undef rwD3DK0x0069b560
#undef rwD3DK0x0069b564
#undef rwD3DK0x0069b568
#undef rwD3DK0x0069b56c
#undef rwD3DK0x0069b570
#undef rwD3DK0x0069b574
#undef rwD3DK0x0069b57c
#undef rwD3DK0x0069b580
#undef rwD3DK0x0069b584
#undef rwD3DK0x0069b588
#undef rwD3DK0x0069b58c
#undef rwD3DK0x0069b590
#undef rwD3DK0x0069b598
#undef rwD3DK0x0069b5a0
#undef rwD3DK0x0069b5a4
#undef rwD3DK0x0069b5a8
#undef rwD3DK0x0069b5ac
#undef rwD3DK0x0069b5b0
#undef rwD3DK0x0069b5b4
#undef rwD3DK0x0069b5bc
#undef rwD3DK0x0069b5c4
#undef rwD3DK0x0069b5c8
#undef rwD3DK0x0069b5cc
#undef rwD3DK0x0069b5d4
#undef rwD3DK0x0069b5d8
#undef rwD3DK0x0069b5dc
#undef rwD3DK0x0069b5e0
#undef rwD3DK0x0069b5e8
#undef rwD3DK0x0069b5f0
#undef rwD3DK0x0069b604
#undef rwD3DK0x0069b608
#undef rwD3DK0x0069b60c
#undef rwD3DLineIndices
#undef rwD3DMemory
#undef rwD3DMergeBuf
#undef rwD3DMergeCap
#undef rwD3DNearScreenZ
#undef rwD3DNearZ
#undef rwD3DOne
#undef rwD3DOrClip
#undef rwD3DRasterExtOffset
#undef rwD3DRecipWindow
#undef rwD3DRemap
#undef rwD3DRemapCap
#undef rwD3DRemapUsed
#undef rwD3DRenderOrder
#undef rwD3DSectorDefaultColor
#undef rwD3DSetProjection
#undef rwD3DSetUVStage
#undef rwD3DSortCap
#undef rwD3DSortCount
#undef rwD3DSortItems
#undef rwD3DSortNeed
#undef rwD3DState
#undef rwD3DTLBase
#undef rwD3DTLBuffer
#undef rwD3DTriCull
#undef rwD3DTriIndices
#undef rwD3DVertexBits
#undef rwD3DVertexBitsSize
#undef rwD3DXOffset
#undef rwD3DXScale
#undef rwD3DXformFog
#undef rwD3DXformMatrix
#undef rwD3DXformOut
#undef rwD3DXformStride
#undef rwD3DYOffset
#undef rwD3DYScale
#undef rwD3DZScale
#undef rwD3DZShift
#undef rwEmptyString
#undef rwEngineInstanceCount
#undef rwEngineRegistry
#undef rwErrorModuleCount
#undef rwErrorOffset
#undef rwFastSqrtOffset
#undef rwFrameModuleCount
#undef rwFrameOffset
#undef rwFrameRegistry
#undef rwFreeListHead
#undef rwIm3DGouraud
#undef rwIm3DModuleCount
#undef rwIm3DOffset
#undef rwIm3DPipelineOffset
#undef rwImageModuleCount
#undef rwImageOffset
#undef rwImageRegistry
#undef rwInvSqrtScaleEven
#undef rwInvSqrtScaleOdd
#undef rwMatrixModuleCount
#undef rwMatrixNormalErrorLimits
#undef rwMatrixOffset
#undef rwMipSuffix
#undef rwPerspCornerSteps
#undef rwPluginRegEntryFreeList
#undef rwRasterModuleCount
#undef rwRasterOffset
#undef rwRasterRegistry
#undef rwRenderPipelineModuleCount
#undef rwResourcesModuleCount
#undef rwResourcesOffset
#undef rwStreamModuleCount
#undef rwStreamOffset
#undef rwTexDictionaryRegistry
#undef rwTextureModuleCount
#undef rwTextureOffset
#undef rwTextureRegistry
#undef rwVectorModuleCount
#undef rwViewFactors

/* Dinput.lib's object tables, which the linker put into the original's .text, 0x004afc60..0x004b0f20. */
struct BmxGroup_bmxInputTables {
    void *p_004afc60;         /* 0x004afc60 */
    uint32_t d_004afc64[3];   /* 0x004afc64 */
    void *p_004afc70;         /* 0x004afc70 */
    uint32_t d_004afc74[3];   /* 0x004afc74 */
    void *p_004afc80;         /* 0x004afc80 */
    uint32_t d_004afc84[3];   /* 0x004afc84 */
    void *p_004afc90;         /* 0x004afc90 */
    uint32_t d_004afc94[3];   /* 0x004afc94 */
    void *p_004afca0;         /* 0x004afca0 */
    uint32_t d_004afca4[3];   /* 0x004afca4 */
    void *p_004afcb0;         /* 0x004afcb0 */
    uint32_t d_004afcb4[3];   /* 0x004afcb4 */
    void *p_004afcc0;         /* 0x004afcc0 */
    uint32_t d_004afcc4[3];   /* 0x004afcc4 */
    void *p_004afcd0;         /* 0x004afcd0 */
    uint32_t d_004afcd4[3];   /* 0x004afcd4 */
    void *p_004afce0;         /* 0x004afce0 */
    uint32_t d_004afce4[3];   /* 0x004afce4 */
    void *p_004afcf0;         /* 0x004afcf0 */
    uint32_t d_004afcf4[3];   /* 0x004afcf4 */
    void *p_004afd00;         /* 0x004afd00 */
    uint32_t d_004afd04[3];   /* 0x004afd04 */
    void *p_004afd10;         /* 0x004afd10 */
    uint32_t d_004afd14[131]; /* 0x004afd14 */
    void *p_004aff20;         /* 0x004aff20 */
    uint32_t d_004aff24[3];   /* 0x004aff24 */
    void *p_004aff30;         /* 0x004aff30 */
    uint32_t d_004aff34[3];   /* 0x004aff34 */
    void *p_004aff40;         /* 0x004aff40 */
    uint32_t d_004aff44[3];   /* 0x004aff44 */
    void *p_004aff50;         /* 0x004aff50 */
    uint32_t d_004aff54[3];   /* 0x004aff54 */
    void *p_004aff60;         /* 0x004aff60 */
    uint32_t d_004aff64[3];   /* 0x004aff64 */
    void *p_004aff70;         /* 0x004aff70 */
    uint32_t d_004aff74[3];   /* 0x004aff74 */
    void *p_004aff80;         /* 0x004aff80 */
    uint32_t d_004aff84[3];   /* 0x004aff84 */
    void *p_004aff90;         /* 0x004aff90 */
    uint32_t d_004aff94[3];   /* 0x004aff94 */
    void *p_004affa0;         /* 0x004affa0 */
    uint32_t d_004affa4[3];   /* 0x004affa4 */
    void *p_004affb0;         /* 0x004affb0 */
    uint32_t d_004affb4[3];   /* 0x004affb4 */
    void *p_004affc0;         /* 0x004affc0 */
    uint32_t d_004affc4[3];   /* 0x004affc4 */
    void *p_004affd0;         /* 0x004affd0 */
    uint32_t d_004affd4[3];   /* 0x004affd4 */
    void *p_004affe0;         /* 0x004affe0 */
    uint32_t d_004affe4[3];   /* 0x004affe4 */
    void *p_004afff0;         /* 0x004afff0 */
    uint32_t d_004afff4[3];   /* 0x004afff4 */
    void *p_004b0000;         /* 0x004b0000 */
    uint32_t d_004b0004[3];   /* 0x004b0004 */
    void *p_004b0010;         /* 0x004b0010 */
    uint32_t d_004b0014[3];   /* 0x004b0014 */
    void *p_004b0020;         /* 0x004b0020 */
    uint32_t d_004b0024[3];   /* 0x004b0024 */
    void *p_004b0030;         /* 0x004b0030 */
    uint32_t d_004b0034[3];   /* 0x004b0034 */
    void *p_004b0040;         /* 0x004b0040 */
    uint32_t d_004b0044[3];   /* 0x004b0044 */
    void *p_004b0050;         /* 0x004b0050 */
    uint32_t d_004b0054[3];   /* 0x004b0054 */
    void *p_004b0060;         /* 0x004b0060 */
    uint32_t d_004b0064[3];   /* 0x004b0064 */
    void *p_004b0070;         /* 0x004b0070 */
    uint32_t d_004b0074[3];   /* 0x004b0074 */
    void *p_004b0080;         /* 0x004b0080 */
    uint32_t d_004b0084[3];   /* 0x004b0084 */
    void *p_004b0090;         /* 0x004b0090 */
    uint32_t d_004b0094[3];   /* 0x004b0094 */
    void *p_004b00a0;         /* 0x004b00a0 */
    uint32_t d_004b00a4[3];   /* 0x004b00a4 */
    void *p_004b00b0;         /* 0x004b00b0 */
    uint32_t d_004b00b4[3];   /* 0x004b00b4 */
    void *p_004b00c0;         /* 0x004b00c0 */
    uint32_t d_004b00c4[3];   /* 0x004b00c4 */
    void *p_004b00d0;         /* 0x004b00d0 */
    uint32_t d_004b00d4[3];   /* 0x004b00d4 */
    void *p_004b00e0;         /* 0x004b00e0 */
    uint32_t d_004b00e4[3];   /* 0x004b00e4 */
    void *p_004b00f0;         /* 0x004b00f0 */
    uint32_t d_004b00f4[3];   /* 0x004b00f4 */
    void *p_004b0100;         /* 0x004b0100 */
    uint32_t d_004b0104[3];   /* 0x004b0104 */
    void *p_004b0110;         /* 0x004b0110 */
    uint32_t d_004b0114[3];   /* 0x004b0114 */
    void *p_004b0120;         /* 0x004b0120 */
    uint32_t d_004b0124[3];   /* 0x004b0124 */
    void *p_004b0130;         /* 0x004b0130 */
    uint32_t d_004b0134[3];   /* 0x004b0134 */
    void *p_004b0140;         /* 0x004b0140 */
    uint32_t d_004b0144[3];   /* 0x004b0144 */
    void *p_004b0150;         /* 0x004b0150 */
    uint32_t d_004b0154[3];   /* 0x004b0154 */
    void *p_004b0160;         /* 0x004b0160 */
    uint32_t d_004b0164[3];   /* 0x004b0164 */
    void *p_004b0170;         /* 0x004b0170 */
    uint32_t d_004b0174[3];   /* 0x004b0174 */
    void *p_004b0180;         /* 0x004b0180 */
    uint32_t d_004b0184[3];   /* 0x004b0184 */
    void *p_004b0190;         /* 0x004b0190 */
    uint32_t d_004b0194[3];   /* 0x004b0194 */
    void *p_004b01a0;         /* 0x004b01a0 */
    uint32_t d_004b01a4[3];   /* 0x004b01a4 */
    void *p_004b01b0;         /* 0x004b01b0 */
    uint32_t d_004b01b4[3];   /* 0x004b01b4 */
    void *p_004b01c0;         /* 0x004b01c0 */
    uint32_t d_004b01c4[3];   /* 0x004b01c4 */
    void *p_004b01d0;         /* 0x004b01d0 */
    uint32_t d_004b01d4[3];   /* 0x004b01d4 */
    void *p_004b01e0;         /* 0x004b01e0 */
    uint32_t d_004b01e4[3];   /* 0x004b01e4 */
    void *p_004b01f0;         /* 0x004b01f0 */
    uint32_t d_004b01f4[3];   /* 0x004b01f4 */
    void *p_004b0200;         /* 0x004b0200 */
    uint32_t d_004b0204[3];   /* 0x004b0204 */
    void *p_004b0210;         /* 0x004b0210 */
    uint32_t d_004b0214[3];   /* 0x004b0214 */
    void *p_004b0220;         /* 0x004b0220 */
    uint32_t d_004b0224[3];   /* 0x004b0224 */
    void *p_004b0230;         /* 0x004b0230 */
    uint32_t d_004b0234[3];   /* 0x004b0234 */
    void *p_004b0240;         /* 0x004b0240 */
    uint32_t d_004b0244[3];   /* 0x004b0244 */
    void *p_004b0250;         /* 0x004b0250 */
    uint32_t d_004b0254[3];   /* 0x004b0254 */
    void *p_004b0260;         /* 0x004b0260 */
    uint32_t d_004b0264[3];   /* 0x004b0264 */
    void *p_004b0270;         /* 0x004b0270 */
    uint32_t d_004b0274[3];   /* 0x004b0274 */
    void *p_004b0280;         /* 0x004b0280 */
    uint32_t d_004b0284[3];   /* 0x004b0284 */
    void *p_004b0290;         /* 0x004b0290 */
    uint32_t d_004b0294[3];   /* 0x004b0294 */
    void *p_004b02a0;         /* 0x004b02a0 */
    uint32_t d_004b02a4[3];   /* 0x004b02a4 */
    void *p_004b02b0;         /* 0x004b02b0 */
    uint32_t d_004b02b4[3];   /* 0x004b02b4 */
    void *p_004b02c0;         /* 0x004b02c0 */
    uint32_t d_004b02c4[3];   /* 0x004b02c4 */
    void *p_004b02d0;         /* 0x004b02d0 */
    uint32_t d_004b02d4[3];   /* 0x004b02d4 */
    void *p_004b02e0;         /* 0x004b02e0 */
    uint32_t d_004b02e4[3];   /* 0x004b02e4 */
    void *p_004b02f0;         /* 0x004b02f0 */
    uint32_t d_004b02f4[3];   /* 0x004b02f4 */
    void *p_004b0300;         /* 0x004b0300 */
    uint32_t d_004b0304[3];   /* 0x004b0304 */
    void *p_004b0310;         /* 0x004b0310 */
    uint32_t d_004b0314[3];   /* 0x004b0314 */
    void *p_004b0320;         /* 0x004b0320 */
    uint32_t d_004b0324[3];   /* 0x004b0324 */
    void *p_004b0330;         /* 0x004b0330 */
    uint32_t d_004b0334[3];   /* 0x004b0334 */
    void *p_004b0340;         /* 0x004b0340 */
    uint32_t d_004b0344[3];   /* 0x004b0344 */
    void *p_004b0350;         /* 0x004b0350 */
    uint32_t d_004b0354[3];   /* 0x004b0354 */
    void *p_004b0360;         /* 0x004b0360 */
    uint32_t d_004b0364[3];   /* 0x004b0364 */
    void *p_004b0370;         /* 0x004b0370 */
    uint32_t d_004b0374[3];   /* 0x004b0374 */
    void *p_004b0380;         /* 0x004b0380 */
    uint32_t d_004b0384[3];   /* 0x004b0384 */
    void *p_004b0390;         /* 0x004b0390 */
    uint32_t d_004b0394[3];   /* 0x004b0394 */
    void *p_004b03a0;         /* 0x004b03a0 */
    uint32_t d_004b03a4[3];   /* 0x004b03a4 */
    void *p_004b03b0;         /* 0x004b03b0 */
    uint32_t d_004b03b4[3];   /* 0x004b03b4 */
    void *p_004b03c0;         /* 0x004b03c0 */
    uint32_t d_004b03c4[3];   /* 0x004b03c4 */
    void *p_004b03d0;         /* 0x004b03d0 */
    uint32_t d_004b03d4[3];   /* 0x004b03d4 */
    void *p_004b03e0;         /* 0x004b03e0 */
    uint32_t d_004b03e4[3];   /* 0x004b03e4 */
    void *p_004b03f0;         /* 0x004b03f0 */
    uint32_t d_004b03f4[3];   /* 0x004b03f4 */
    void *p_004b0400;         /* 0x004b0400 */
    uint32_t d_004b0404[3];   /* 0x004b0404 */
    void *p_004b0410;         /* 0x004b0410 */
    uint32_t d_004b0414[3];   /* 0x004b0414 */
    void *p_004b0420;         /* 0x004b0420 */
    uint32_t d_004b0424[3];   /* 0x004b0424 */
    void *p_004b0430;         /* 0x004b0430 */
    uint32_t d_004b0434[3];   /* 0x004b0434 */
    void *p_004b0440;         /* 0x004b0440 */
    uint32_t d_004b0444[3];   /* 0x004b0444 */
    void *p_004b0450;         /* 0x004b0450 */
    uint32_t d_004b0454[3];   /* 0x004b0454 */
    void *p_004b0460;         /* 0x004b0460 */
    uint32_t d_004b0464[3];   /* 0x004b0464 */
    void *p_004b0470;         /* 0x004b0470 */
    uint32_t d_004b0474[3];   /* 0x004b0474 */
    void *p_004b0480;         /* 0x004b0480 */
    uint32_t d_004b0484[3];   /* 0x004b0484 */
    void *p_004b0490;         /* 0x004b0490 */
    uint32_t d_004b0494[3];   /* 0x004b0494 */
    void *p_004b04a0;         /* 0x004b04a0 */
    uint32_t d_004b04a4[3];   /* 0x004b04a4 */
    void *p_004b04b0;         /* 0x004b04b0 */
    uint32_t d_004b04b4[3];   /* 0x004b04b4 */
    void *p_004b04c0;         /* 0x004b04c0 */
    uint32_t d_004b04c4[3];   /* 0x004b04c4 */
    void *p_004b04d0;         /* 0x004b04d0 */
    uint32_t d_004b04d4[3];   /* 0x004b04d4 */
    void *p_004b04e0;         /* 0x004b04e0 */
    uint32_t d_004b04e4[3];   /* 0x004b04e4 */
    void *p_004b04f0;         /* 0x004b04f0 */
    uint32_t d_004b04f4[3];   /* 0x004b04f4 */
    void *p_004b0500;         /* 0x004b0500 */
    uint32_t d_004b0504[3];   /* 0x004b0504 */
    void *p_004b0510;         /* 0x004b0510 */
    uint32_t d_004b0514[3];   /* 0x004b0514 */
    void *p_004b0520;         /* 0x004b0520 */
    uint32_t d_004b0524[3];   /* 0x004b0524 */
    void *p_004b0530;         /* 0x004b0530 */
    uint32_t d_004b0534[3];   /* 0x004b0534 */
    void *p_004b0540;         /* 0x004b0540 */
    uint32_t d_004b0544[3];   /* 0x004b0544 */
    void *p_004b0550;         /* 0x004b0550 */
    uint32_t d_004b0554[3];   /* 0x004b0554 */
    void *p_004b0560;         /* 0x004b0560 */
    uint32_t d_004b0564[3];   /* 0x004b0564 */
    void *p_004b0570;         /* 0x004b0570 */
    uint32_t d_004b0574[3];   /* 0x004b0574 */
    void *p_004b0580;         /* 0x004b0580 */
    uint32_t d_004b0584[3];   /* 0x004b0584 */
    void *p_004b0590;         /* 0x004b0590 */
    uint32_t d_004b0594[3];   /* 0x004b0594 */
    void *p_004b05a0;         /* 0x004b05a0 */
    uint32_t d_004b05a4[3];   /* 0x004b05a4 */
    void *p_004b05b0;         /* 0x004b05b0 */
    uint32_t d_004b05b4[3];   /* 0x004b05b4 */
    void *p_004b05c0;         /* 0x004b05c0 */
    uint32_t d_004b05c4[3];   /* 0x004b05c4 */
    void *p_004b05d0;         /* 0x004b05d0 */
    uint32_t d_004b05d4[3];   /* 0x004b05d4 */
    void *p_004b05e0;         /* 0x004b05e0 */
    uint32_t d_004b05e4[3];   /* 0x004b05e4 */
    void *p_004b05f0;         /* 0x004b05f0 */
    uint32_t d_004b05f4[3];   /* 0x004b05f4 */
    void *p_004b0600;         /* 0x004b0600 */
    uint32_t d_004b0604[3];   /* 0x004b0604 */
    void *p_004b0610;         /* 0x004b0610 */
    uint32_t d_004b0614[3];   /* 0x004b0614 */
    void *p_004b0620;         /* 0x004b0620 */
    uint32_t d_004b0624[3];   /* 0x004b0624 */
    void *p_004b0630;         /* 0x004b0630 */
    uint32_t d_004b0634[3];   /* 0x004b0634 */
    void *p_004b0640;         /* 0x004b0640 */
    uint32_t d_004b0644[3];   /* 0x004b0644 */
    void *p_004b0650;         /* 0x004b0650 */
    uint32_t d_004b0654[3];   /* 0x004b0654 */
    void *p_004b0660;         /* 0x004b0660 */
    uint32_t d_004b0664[3];   /* 0x004b0664 */
    void *p_004b0670;         /* 0x004b0670 */
    uint32_t d_004b0674[3];   /* 0x004b0674 */
    void *p_004b0680;         /* 0x004b0680 */
    uint32_t d_004b0684[3];   /* 0x004b0684 */
    void *p_004b0690;         /* 0x004b0690 */
    uint32_t d_004b0694[3];   /* 0x004b0694 */
    void *p_004b06a0;         /* 0x004b06a0 */
    uint32_t d_004b06a4[3];   /* 0x004b06a4 */
    void *p_004b06b0;         /* 0x004b06b0 */
    uint32_t d_004b06b4[3];   /* 0x004b06b4 */
    void *p_004b06c0;         /* 0x004b06c0 */
    uint32_t d_004b06c4[3];   /* 0x004b06c4 */
    void *p_004b06d0;         /* 0x004b06d0 */
    uint32_t d_004b06d4[3];   /* 0x004b06d4 */
    void *p_004b06e0;         /* 0x004b06e0 */
    uint32_t d_004b06e4[3];   /* 0x004b06e4 */
    void *p_004b06f0;         /* 0x004b06f0 */
    uint32_t d_004b06f4[3];   /* 0x004b06f4 */
    void *p_004b0700;         /* 0x004b0700 */
    uint32_t d_004b0704[3];   /* 0x004b0704 */
    void *p_004b0710;         /* 0x004b0710 */
    uint32_t d_004b0714[3];   /* 0x004b0714 */
    void *p_004b0720;         /* 0x004b0720 */
    uint32_t d_004b0724[3];   /* 0x004b0724 */
    void *p_004b0730;         /* 0x004b0730 */
    uint32_t d_004b0734[3];   /* 0x004b0734 */
    void *p_004b0740;         /* 0x004b0740 */
    uint32_t d_004b0744[3];   /* 0x004b0744 */
    void *p_004b0750;         /* 0x004b0750 */
    uint32_t d_004b0754[3];   /* 0x004b0754 */
    void *p_004b0760;         /* 0x004b0760 */
    uint32_t d_004b0764[3];   /* 0x004b0764 */
    void *p_004b0770;         /* 0x004b0770 */
    uint32_t d_004b0774[3];   /* 0x004b0774 */
    void *p_004b0780;         /* 0x004b0780 */
    uint32_t d_004b0784[3];   /* 0x004b0784 */
    void *p_004b0790;         /* 0x004b0790 */
    uint32_t d_004b0794[3];   /* 0x004b0794 */
    void *p_004b07a0;         /* 0x004b07a0 */
    uint32_t d_004b07a4[3];   /* 0x004b07a4 */
    void *p_004b07b0;         /* 0x004b07b0 */
    uint32_t d_004b07b4[3];   /* 0x004b07b4 */
    void *p_004b07c0;         /* 0x004b07c0 */
    uint32_t d_004b07c4[3];   /* 0x004b07c4 */
    void *p_004b07d0;         /* 0x004b07d0 */
    uint32_t d_004b07d4[3];   /* 0x004b07d4 */
    void *p_004b07e0;         /* 0x004b07e0 */
    uint32_t d_004b07e4[3];   /* 0x004b07e4 */
    void *p_004b07f0;         /* 0x004b07f0 */
    uint32_t d_004b07f4[3];   /* 0x004b07f4 */
    void *p_004b0800;         /* 0x004b0800 */
    uint32_t d_004b0804[3];   /* 0x004b0804 */
    void *p_004b0810;         /* 0x004b0810 */
    uint32_t d_004b0814[3];   /* 0x004b0814 */
    void *p_004b0820;         /* 0x004b0820 */
    uint32_t d_004b0824[3];   /* 0x004b0824 */
    void *p_004b0830;         /* 0x004b0830 */
    uint32_t d_004b0834[3];   /* 0x004b0834 */
    void *p_004b0840;         /* 0x004b0840 */
    uint32_t d_004b0844[3];   /* 0x004b0844 */
    void *p_004b0850;         /* 0x004b0850 */
    uint32_t d_004b0854[3];   /* 0x004b0854 */
    void *p_004b0860;         /* 0x004b0860 */
    uint32_t d_004b0864[3];   /* 0x004b0864 */
    void *p_004b0870;         /* 0x004b0870 */
    uint32_t d_004b0874[3];   /* 0x004b0874 */
    void *p_004b0880;         /* 0x004b0880 */
    uint32_t d_004b0884[3];   /* 0x004b0884 */
    void *p_004b0890;         /* 0x004b0890 */
    uint32_t d_004b0894[3];   /* 0x004b0894 */
    void *p_004b08a0;         /* 0x004b08a0 */
    uint32_t d_004b08a4[3];   /* 0x004b08a4 */
    void *p_004b08b0;         /* 0x004b08b0 */
    uint32_t d_004b08b4[3];   /* 0x004b08b4 */
    void *p_004b08c0;         /* 0x004b08c0 */
    uint32_t d_004b08c4[3];   /* 0x004b08c4 */
    void *p_004b08d0;         /* 0x004b08d0 */
    uint32_t d_004b08d4[3];   /* 0x004b08d4 */
    void *p_004b08e0;         /* 0x004b08e0 */
    uint32_t d_004b08e4[3];   /* 0x004b08e4 */
    void *p_004b08f0;         /* 0x004b08f0 */
    uint32_t d_004b08f4[3];   /* 0x004b08f4 */
    void *p_004b0900;         /* 0x004b0900 */
    uint32_t d_004b0904[3];   /* 0x004b0904 */
    void *p_004b0910;         /* 0x004b0910 */
    uint32_t d_004b0914[3];   /* 0x004b0914 */
    void *p_004b0920;         /* 0x004b0920 */
    uint32_t d_004b0924[3];   /* 0x004b0924 */
    void *p_004b0930;         /* 0x004b0930 */
    uint32_t d_004b0934[3];   /* 0x004b0934 */
    void *p_004b0940;         /* 0x004b0940 */
    uint32_t d_004b0944[3];   /* 0x004b0944 */
    void *p_004b0950;         /* 0x004b0950 */
    uint32_t d_004b0954[3];   /* 0x004b0954 */
    void *p_004b0960;         /* 0x004b0960 */
    uint32_t d_004b0964[3];   /* 0x004b0964 */
    void *p_004b0970;         /* 0x004b0970 */
    uint32_t d_004b0974[3];   /* 0x004b0974 */
    void *p_004b0980;         /* 0x004b0980 */
    uint32_t d_004b0984[3];   /* 0x004b0984 */
    void *p_004b0990;         /* 0x004b0990 */
    uint32_t d_004b0994[3];   /* 0x004b0994 */
    void *p_004b09a0;         /* 0x004b09a0 */
    uint32_t d_004b09a4[3];   /* 0x004b09a4 */
    void *p_004b09b0;         /* 0x004b09b0 */
    uint32_t d_004b09b4[3];   /* 0x004b09b4 */
    void *p_004b09c0;         /* 0x004b09c0 */
    uint32_t d_004b09c4[3];   /* 0x004b09c4 */
    void *p_004b09d0;         /* 0x004b09d0 */
    uint32_t d_004b09d4[3];   /* 0x004b09d4 */
    void *p_004b09e0;         /* 0x004b09e0 */
    uint32_t d_004b09e4[3];   /* 0x004b09e4 */
    void *p_004b09f0;         /* 0x004b09f0 */
    uint32_t d_004b09f4[3];   /* 0x004b09f4 */
    void *p_004b0a00;         /* 0x004b0a00 */
    uint32_t d_004b0a04[3];   /* 0x004b0a04 */
    void *p_004b0a10;         /* 0x004b0a10 */
    uint32_t d_004b0a14[3];   /* 0x004b0a14 */
    void *p_004b0a20;         /* 0x004b0a20 */
    uint32_t d_004b0a24[3];   /* 0x004b0a24 */
    void *p_004b0a30;         /* 0x004b0a30 */
    uint32_t d_004b0a34[3];   /* 0x004b0a34 */
    void *p_004b0a40;         /* 0x004b0a40 */
    uint32_t d_004b0a44[3];   /* 0x004b0a44 */
    void *p_004b0a50;         /* 0x004b0a50 */
    uint32_t d_004b0a54[3];   /* 0x004b0a54 */
    void *p_004b0a60;         /* 0x004b0a60 */
    uint32_t d_004b0a64[3];   /* 0x004b0a64 */
    void *p_004b0a70;         /* 0x004b0a70 */
    uint32_t d_004b0a74[3];   /* 0x004b0a74 */
    void *p_004b0a80;         /* 0x004b0a80 */
    uint32_t d_004b0a84[3];   /* 0x004b0a84 */
    void *p_004b0a90;         /* 0x004b0a90 */
    uint32_t d_004b0a94[3];   /* 0x004b0a94 */
    void *p_004b0aa0;         /* 0x004b0aa0 */
    uint32_t d_004b0aa4[3];   /* 0x004b0aa4 */
    void *p_004b0ab0;         /* 0x004b0ab0 */
    uint32_t d_004b0ab4[3];   /* 0x004b0ab4 */
    void *p_004b0ac0;         /* 0x004b0ac0 */
    uint32_t d_004b0ac4[3];   /* 0x004b0ac4 */
    void *p_004b0ad0;         /* 0x004b0ad0 */
    uint32_t d_004b0ad4[3];   /* 0x004b0ad4 */
    void *p_004b0ae0;         /* 0x004b0ae0 */
    uint32_t d_004b0ae4[3];   /* 0x004b0ae4 */
    void *p_004b0af0;         /* 0x004b0af0 */
    uint32_t d_004b0af4[3];   /* 0x004b0af4 */
    void *p_004b0b00;         /* 0x004b0b00 */
    uint32_t d_004b0b04[3];   /* 0x004b0b04 */
    void *p_004b0b10;         /* 0x004b0b10 */
    uint32_t d_004b0b14[3];   /* 0x004b0b14 */
    void *p_004b0b20;         /* 0x004b0b20 */
    uint32_t d_004b0b24[3];   /* 0x004b0b24 */
    void *p_004b0b30;         /* 0x004b0b30 */
    uint32_t d_004b0b34[3];   /* 0x004b0b34 */
    void *p_004b0b40;         /* 0x004b0b40 */
    uint32_t d_004b0b44[3];   /* 0x004b0b44 */
    void *p_004b0b50;         /* 0x004b0b50 */
    uint32_t d_004b0b54[3];   /* 0x004b0b54 */
    void *p_004b0b60;         /* 0x004b0b60 */
    uint32_t d_004b0b64[3];   /* 0x004b0b64 */
    void *p_004b0b70;         /* 0x004b0b70 */
    uint32_t d_004b0b74[3];   /* 0x004b0b74 */
    void *p_004b0b80;         /* 0x004b0b80 */
    uint32_t d_004b0b84[3];   /* 0x004b0b84 */
    void *p_004b0b90;         /* 0x004b0b90 */
    uint32_t d_004b0b94[3];   /* 0x004b0b94 */
    void *p_004b0ba0;         /* 0x004b0ba0 */
    uint32_t d_004b0ba4[3];   /* 0x004b0ba4 */
    void *p_004b0bb0;         /* 0x004b0bb0 */
    uint32_t d_004b0bb4[3];   /* 0x004b0bb4 */
    void *p_004b0bc0;         /* 0x004b0bc0 */
    uint32_t d_004b0bc4[3];   /* 0x004b0bc4 */
    void *p_004b0bd0;         /* 0x004b0bd0 */
    uint32_t d_004b0bd4[3];   /* 0x004b0bd4 */
    void *p_004b0be0;         /* 0x004b0be0 */
    uint32_t d_004b0be4[3];   /* 0x004b0be4 */
    void *p_004b0bf0;         /* 0x004b0bf0 */
    uint32_t d_004b0bf4[3];   /* 0x004b0bf4 */
    void *p_004b0c00;         /* 0x004b0c00 */
    uint32_t d_004b0c04[3];   /* 0x004b0c04 */
    void *p_004b0c10;         /* 0x004b0c10 */
    uint32_t d_004b0c14[3];   /* 0x004b0c14 */
    void *p_004b0c20;         /* 0x004b0c20 */
    uint32_t d_004b0c24[3];   /* 0x004b0c24 */
    void *p_004b0c30;         /* 0x004b0c30 */
    uint32_t d_004b0c34[3];   /* 0x004b0c34 */
    void *p_004b0c40;         /* 0x004b0c40 */
    uint32_t d_004b0c44[3];   /* 0x004b0c44 */
    void *p_004b0c50;         /* 0x004b0c50 */
    uint32_t d_004b0c54[3];   /* 0x004b0c54 */
    void *p_004b0c60;         /* 0x004b0c60 */
    uint32_t d_004b0c64[3];   /* 0x004b0c64 */
    void *p_004b0c70;         /* 0x004b0c70 */
    uint32_t d_004b0c74[3];   /* 0x004b0c74 */
    void *p_004b0c80;         /* 0x004b0c80 */
    uint32_t d_004b0c84[3];   /* 0x004b0c84 */
    void *p_004b0c90;         /* 0x004b0c90 */
    uint32_t d_004b0c94[3];   /* 0x004b0c94 */
    void *p_004b0ca0;         /* 0x004b0ca0 */
    uint32_t d_004b0ca4[3];   /* 0x004b0ca4 */
    void *p_004b0cb0;         /* 0x004b0cb0 */
    uint32_t d_004b0cb4[3];   /* 0x004b0cb4 */
    void *p_004b0cc0;         /* 0x004b0cc0 */
    uint32_t d_004b0cc4[3];   /* 0x004b0cc4 */
    void *p_004b0cd0;         /* 0x004b0cd0 */
    uint32_t d_004b0cd4[3];   /* 0x004b0cd4 */
    void *p_004b0ce0;         /* 0x004b0ce0 */
    uint32_t d_004b0ce4[3];   /* 0x004b0ce4 */
    void *p_004b0cf0;         /* 0x004b0cf0 */
    uint32_t d_004b0cf4[3];   /* 0x004b0cf4 */
    void *p_004b0d00;         /* 0x004b0d00 */
    uint32_t d_004b0d04[3];   /* 0x004b0d04 */
    void *p_004b0d10;         /* 0x004b0d10 */
    uint32_t d_004b0d14[3];   /* 0x004b0d14 */
    void *p_004b0d20;         /* 0x004b0d20 */
    uint32_t d_004b0d24[3];   /* 0x004b0d24 */
    void *p_004b0d30;         /* 0x004b0d30 */
    uint32_t d_004b0d34[3];   /* 0x004b0d34 */
    void *p_004b0d40;         /* 0x004b0d40 */
    uint32_t d_004b0d44[3];   /* 0x004b0d44 */
    void *p_004b0d50;         /* 0x004b0d50 */
    uint32_t d_004b0d54[3];   /* 0x004b0d54 */
    void *p_004b0d60;         /* 0x004b0d60 */
    uint32_t d_004b0d64[3];   /* 0x004b0d64 */
    void *p_004b0d70;         /* 0x004b0d70 */
    uint32_t d_004b0d74[3];   /* 0x004b0d74 */
    void *p_004b0d80;         /* 0x004b0d80 */
    uint32_t d_004b0d84[3];   /* 0x004b0d84 */
    void *p_004b0d90;         /* 0x004b0d90 */
    uint32_t d_004b0d94[3];   /* 0x004b0d94 */
    void *p_004b0da0;         /* 0x004b0da0 */
    uint32_t d_004b0da4[3];   /* 0x004b0da4 */
    void *p_004b0db0;         /* 0x004b0db0 */
    uint32_t d_004b0db4[3];   /* 0x004b0db4 */
    void *p_004b0dc0;         /* 0x004b0dc0 */
    uint32_t d_004b0dc4[3];   /* 0x004b0dc4 */
    void *p_004b0dd0;         /* 0x004b0dd0 */
    uint32_t d_004b0dd4[3];   /* 0x004b0dd4 */
    void *p_004b0de0;         /* 0x004b0de0 */
    uint32_t d_004b0de4[3];   /* 0x004b0de4 */
    void *p_004b0df0;         /* 0x004b0df0 */
    uint32_t d_004b0df4[3];   /* 0x004b0df4 */
    void *p_004b0e00;         /* 0x004b0e00 */
    uint32_t d_004b0e04[3];   /* 0x004b0e04 */
    void *p_004b0e10;         /* 0x004b0e10 */
    uint32_t d_004b0e14[3];   /* 0x004b0e14 */
    void *p_004b0e20;         /* 0x004b0e20 */
    uint32_t d_004b0e24[3];   /* 0x004b0e24 */
    void *p_004b0e30;         /* 0x004b0e30 */
    uint32_t d_004b0e34[3];   /* 0x004b0e34 */
    void *p_004b0e40;         /* 0x004b0e40 */
    uint32_t d_004b0e44[3];   /* 0x004b0e44 */
    void *p_004b0e50;         /* 0x004b0e50 */
    uint32_t d_004b0e54[3];   /* 0x004b0e54 */
    void *p_004b0e60;         /* 0x004b0e60 */
    uint32_t d_004b0e64[3];   /* 0x004b0e64 */
    void *p_004b0e70;         /* 0x004b0e70 */
    uint32_t d_004b0e74[3];   /* 0x004b0e74 */
    void *p_004b0e80;         /* 0x004b0e80 */
    uint32_t d_004b0e84[3];   /* 0x004b0e84 */
    void *p_004b0e90;         /* 0x004b0e90 */
    uint32_t d_004b0e94[3];   /* 0x004b0e94 */
    void *p_004b0ea0;         /* 0x004b0ea0 */
    uint32_t d_004b0ea4[3];   /* 0x004b0ea4 */
    void *p_004b0eb0;         /* 0x004b0eb0 */
    uint32_t d_004b0eb4[3];   /* 0x004b0eb4 */
    void *p_004b0ec0;         /* 0x004b0ec0 */
    uint32_t d_004b0ec4[3];   /* 0x004b0ec4 */
    void *p_004b0ed0;         /* 0x004b0ed0 */
    uint32_t d_004b0ed4[3];   /* 0x004b0ed4 */
    void *p_004b0ee0;         /* 0x004b0ee0 */
    uint32_t d_004b0ee4[3];   /* 0x004b0ee4 */
    void *p_004b0ef0;         /* 0x004b0ef0 */
    uint32_t d_004b0ef4[3];   /* 0x004b0ef4 */
    void *p_004b0f00;         /* 0x004b0f00 */
    uint32_t d_004b0f04[3];   /* 0x004b0f04 */
    void *p_004b0f10;         /* 0x004b0f10 */
    uint32_t d_004b0f14[3];   /* 0x004b0f14 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_bmxInputTables, member) ==                                       \
                       BMX_DATA_OFF((addr) - 0x004afc60, off64),                                             \
                   #member)
AT(p_004afc60, 0x004afc60, 0x0);
AT(d_004afc64, 0x004afc64, 0x8);
AT(p_004afc70, 0x004afc70, 0x18);
AT(d_004afc74, 0x004afc74, 0x20);
AT(p_004afc80, 0x004afc80, 0x30);
AT(d_004afc84, 0x004afc84, 0x38);
AT(p_004afc90, 0x004afc90, 0x48);
AT(d_004afc94, 0x004afc94, 0x50);
AT(p_004afca0, 0x004afca0, 0x60);
AT(d_004afca4, 0x004afca4, 0x68);
AT(p_004afcb0, 0x004afcb0, 0x78);
AT(d_004afcb4, 0x004afcb4, 0x80);
AT(p_004afcc0, 0x004afcc0, 0x90);
AT(d_004afcc4, 0x004afcc4, 0x98);
AT(p_004afcd0, 0x004afcd0, 0xa8);
AT(d_004afcd4, 0x004afcd4, 0xb0);
AT(p_004afce0, 0x004afce0, 0xc0);
AT(d_004afce4, 0x004afce4, 0xc8);
AT(p_004afcf0, 0x004afcf0, 0xd8);
AT(d_004afcf4, 0x004afcf4, 0xe0);
AT(p_004afd00, 0x004afd00, 0xf0);
AT(d_004afd04, 0x004afd04, 0xf8);
AT(p_004afd10, 0x004afd10, 0x108);
AT(d_004afd14, 0x004afd14, 0x110);
AT(p_004aff20, 0x004aff20, 0x320);
AT(d_004aff24, 0x004aff24, 0x328);
AT(p_004aff30, 0x004aff30, 0x338);
AT(d_004aff34, 0x004aff34, 0x340);
AT(p_004aff40, 0x004aff40, 0x350);
AT(d_004aff44, 0x004aff44, 0x358);
AT(p_004aff50, 0x004aff50, 0x368);
AT(d_004aff54, 0x004aff54, 0x370);
AT(p_004aff60, 0x004aff60, 0x380);
AT(d_004aff64, 0x004aff64, 0x388);
AT(p_004aff70, 0x004aff70, 0x398);
AT(d_004aff74, 0x004aff74, 0x3a0);
AT(p_004aff80, 0x004aff80, 0x3b0);
AT(d_004aff84, 0x004aff84, 0x3b8);
AT(p_004aff90, 0x004aff90, 0x3c8);
AT(d_004aff94, 0x004aff94, 0x3d0);
AT(p_004affa0, 0x004affa0, 0x3e0);
AT(d_004affa4, 0x004affa4, 0x3e8);
AT(p_004affb0, 0x004affb0, 0x3f8);
AT(d_004affb4, 0x004affb4, 0x400);
AT(p_004affc0, 0x004affc0, 0x410);
AT(d_004affc4, 0x004affc4, 0x418);
AT(p_004affd0, 0x004affd0, 0x428);
AT(d_004affd4, 0x004affd4, 0x430);
AT(p_004affe0, 0x004affe0, 0x440);
AT(d_004affe4, 0x004affe4, 0x448);
AT(p_004afff0, 0x004afff0, 0x458);
AT(d_004afff4, 0x004afff4, 0x460);
AT(p_004b0000, 0x004b0000, 0x470);
AT(d_004b0004, 0x004b0004, 0x478);
AT(p_004b0010, 0x004b0010, 0x488);
AT(d_004b0014, 0x004b0014, 0x490);
AT(p_004b0020, 0x004b0020, 0x4a0);
AT(d_004b0024, 0x004b0024, 0x4a8);
AT(p_004b0030, 0x004b0030, 0x4b8);
AT(d_004b0034, 0x004b0034, 0x4c0);
AT(p_004b0040, 0x004b0040, 0x4d0);
AT(d_004b0044, 0x004b0044, 0x4d8);
AT(p_004b0050, 0x004b0050, 0x4e8);
AT(d_004b0054, 0x004b0054, 0x4f0);
AT(p_004b0060, 0x004b0060, 0x500);
AT(d_004b0064, 0x004b0064, 0x508);
AT(p_004b0070, 0x004b0070, 0x518);
AT(d_004b0074, 0x004b0074, 0x520);
AT(p_004b0080, 0x004b0080, 0x530);
AT(d_004b0084, 0x004b0084, 0x538);
AT(p_004b0090, 0x004b0090, 0x548);
AT(d_004b0094, 0x004b0094, 0x550);
AT(p_004b00a0, 0x004b00a0, 0x560);
AT(d_004b00a4, 0x004b00a4, 0x568);
AT(p_004b00b0, 0x004b00b0, 0x578);
AT(d_004b00b4, 0x004b00b4, 0x580);
AT(p_004b00c0, 0x004b00c0, 0x590);
AT(d_004b00c4, 0x004b00c4, 0x598);
AT(p_004b00d0, 0x004b00d0, 0x5a8);
AT(d_004b00d4, 0x004b00d4, 0x5b0);
AT(p_004b00e0, 0x004b00e0, 0x5c0);
AT(d_004b00e4, 0x004b00e4, 0x5c8);
AT(p_004b00f0, 0x004b00f0, 0x5d8);
AT(d_004b00f4, 0x004b00f4, 0x5e0);
AT(p_004b0100, 0x004b0100, 0x5f0);
AT(d_004b0104, 0x004b0104, 0x5f8);
AT(p_004b0110, 0x004b0110, 0x608);
AT(d_004b0114, 0x004b0114, 0x610);
AT(p_004b0120, 0x004b0120, 0x620);
AT(d_004b0124, 0x004b0124, 0x628);
AT(p_004b0130, 0x004b0130, 0x638);
AT(d_004b0134, 0x004b0134, 0x640);
AT(p_004b0140, 0x004b0140, 0x650);
AT(d_004b0144, 0x004b0144, 0x658);
AT(p_004b0150, 0x004b0150, 0x668);
AT(d_004b0154, 0x004b0154, 0x670);
AT(p_004b0160, 0x004b0160, 0x680);
AT(d_004b0164, 0x004b0164, 0x688);
AT(p_004b0170, 0x004b0170, 0x698);
AT(d_004b0174, 0x004b0174, 0x6a0);
AT(p_004b0180, 0x004b0180, 0x6b0);
AT(d_004b0184, 0x004b0184, 0x6b8);
AT(p_004b0190, 0x004b0190, 0x6c8);
AT(d_004b0194, 0x004b0194, 0x6d0);
AT(p_004b01a0, 0x004b01a0, 0x6e0);
AT(d_004b01a4, 0x004b01a4, 0x6e8);
AT(p_004b01b0, 0x004b01b0, 0x6f8);
AT(d_004b01b4, 0x004b01b4, 0x700);
AT(p_004b01c0, 0x004b01c0, 0x710);
AT(d_004b01c4, 0x004b01c4, 0x718);
AT(p_004b01d0, 0x004b01d0, 0x728);
AT(d_004b01d4, 0x004b01d4, 0x730);
AT(p_004b01e0, 0x004b01e0, 0x740);
AT(d_004b01e4, 0x004b01e4, 0x748);
AT(p_004b01f0, 0x004b01f0, 0x758);
AT(d_004b01f4, 0x004b01f4, 0x760);
AT(p_004b0200, 0x004b0200, 0x770);
AT(d_004b0204, 0x004b0204, 0x778);
AT(p_004b0210, 0x004b0210, 0x788);
AT(d_004b0214, 0x004b0214, 0x790);
AT(p_004b0220, 0x004b0220, 0x7a0);
AT(d_004b0224, 0x004b0224, 0x7a8);
AT(p_004b0230, 0x004b0230, 0x7b8);
AT(d_004b0234, 0x004b0234, 0x7c0);
AT(p_004b0240, 0x004b0240, 0x7d0);
AT(d_004b0244, 0x004b0244, 0x7d8);
AT(p_004b0250, 0x004b0250, 0x7e8);
AT(d_004b0254, 0x004b0254, 0x7f0);
AT(p_004b0260, 0x004b0260, 0x800);
AT(d_004b0264, 0x004b0264, 0x808);
AT(p_004b0270, 0x004b0270, 0x818);
AT(d_004b0274, 0x004b0274, 0x820);
AT(p_004b0280, 0x004b0280, 0x830);
AT(d_004b0284, 0x004b0284, 0x838);
AT(p_004b0290, 0x004b0290, 0x848);
AT(d_004b0294, 0x004b0294, 0x850);
AT(p_004b02a0, 0x004b02a0, 0x860);
AT(d_004b02a4, 0x004b02a4, 0x868);
AT(p_004b02b0, 0x004b02b0, 0x878);
AT(d_004b02b4, 0x004b02b4, 0x880);
AT(p_004b02c0, 0x004b02c0, 0x890);
AT(d_004b02c4, 0x004b02c4, 0x898);
AT(p_004b02d0, 0x004b02d0, 0x8a8);
AT(d_004b02d4, 0x004b02d4, 0x8b0);
AT(p_004b02e0, 0x004b02e0, 0x8c0);
AT(d_004b02e4, 0x004b02e4, 0x8c8);
AT(p_004b02f0, 0x004b02f0, 0x8d8);
AT(d_004b02f4, 0x004b02f4, 0x8e0);
AT(p_004b0300, 0x004b0300, 0x8f0);
AT(d_004b0304, 0x004b0304, 0x8f8);
AT(p_004b0310, 0x004b0310, 0x908);
AT(d_004b0314, 0x004b0314, 0x910);
AT(p_004b0320, 0x004b0320, 0x920);
AT(d_004b0324, 0x004b0324, 0x928);
AT(p_004b0330, 0x004b0330, 0x938);
AT(d_004b0334, 0x004b0334, 0x940);
AT(p_004b0340, 0x004b0340, 0x950);
AT(d_004b0344, 0x004b0344, 0x958);
AT(p_004b0350, 0x004b0350, 0x968);
AT(d_004b0354, 0x004b0354, 0x970);
AT(p_004b0360, 0x004b0360, 0x980);
AT(d_004b0364, 0x004b0364, 0x988);
AT(p_004b0370, 0x004b0370, 0x998);
AT(d_004b0374, 0x004b0374, 0x9a0);
AT(p_004b0380, 0x004b0380, 0x9b0);
AT(d_004b0384, 0x004b0384, 0x9b8);
AT(p_004b0390, 0x004b0390, 0x9c8);
AT(d_004b0394, 0x004b0394, 0x9d0);
AT(p_004b03a0, 0x004b03a0, 0x9e0);
AT(d_004b03a4, 0x004b03a4, 0x9e8);
AT(p_004b03b0, 0x004b03b0, 0x9f8);
AT(d_004b03b4, 0x004b03b4, 0xa00);
AT(p_004b03c0, 0x004b03c0, 0xa10);
AT(d_004b03c4, 0x004b03c4, 0xa18);
AT(p_004b03d0, 0x004b03d0, 0xa28);
AT(d_004b03d4, 0x004b03d4, 0xa30);
AT(p_004b03e0, 0x004b03e0, 0xa40);
AT(d_004b03e4, 0x004b03e4, 0xa48);
AT(p_004b03f0, 0x004b03f0, 0xa58);
AT(d_004b03f4, 0x004b03f4, 0xa60);
AT(p_004b0400, 0x004b0400, 0xa70);
AT(d_004b0404, 0x004b0404, 0xa78);
AT(p_004b0410, 0x004b0410, 0xa88);
AT(d_004b0414, 0x004b0414, 0xa90);
AT(p_004b0420, 0x004b0420, 0xaa0);
AT(d_004b0424, 0x004b0424, 0xaa8);
AT(p_004b0430, 0x004b0430, 0xab8);
AT(d_004b0434, 0x004b0434, 0xac0);
AT(p_004b0440, 0x004b0440, 0xad0);
AT(d_004b0444, 0x004b0444, 0xad8);
AT(p_004b0450, 0x004b0450, 0xae8);
AT(d_004b0454, 0x004b0454, 0xaf0);
AT(p_004b0460, 0x004b0460, 0xb00);
AT(d_004b0464, 0x004b0464, 0xb08);
AT(p_004b0470, 0x004b0470, 0xb18);
AT(d_004b0474, 0x004b0474, 0xb20);
AT(p_004b0480, 0x004b0480, 0xb30);
AT(d_004b0484, 0x004b0484, 0xb38);
AT(p_004b0490, 0x004b0490, 0xb48);
AT(d_004b0494, 0x004b0494, 0xb50);
AT(p_004b04a0, 0x004b04a0, 0xb60);
AT(d_004b04a4, 0x004b04a4, 0xb68);
AT(p_004b04b0, 0x004b04b0, 0xb78);
AT(d_004b04b4, 0x004b04b4, 0xb80);
AT(p_004b04c0, 0x004b04c0, 0xb90);
AT(d_004b04c4, 0x004b04c4, 0xb98);
AT(p_004b04d0, 0x004b04d0, 0xba8);
AT(d_004b04d4, 0x004b04d4, 0xbb0);
AT(p_004b04e0, 0x004b04e0, 0xbc0);
AT(d_004b04e4, 0x004b04e4, 0xbc8);
AT(p_004b04f0, 0x004b04f0, 0xbd8);
AT(d_004b04f4, 0x004b04f4, 0xbe0);
AT(p_004b0500, 0x004b0500, 0xbf0);
AT(d_004b0504, 0x004b0504, 0xbf8);
AT(p_004b0510, 0x004b0510, 0xc08);
AT(d_004b0514, 0x004b0514, 0xc10);
AT(p_004b0520, 0x004b0520, 0xc20);
AT(d_004b0524, 0x004b0524, 0xc28);
AT(p_004b0530, 0x004b0530, 0xc38);
AT(d_004b0534, 0x004b0534, 0xc40);
AT(p_004b0540, 0x004b0540, 0xc50);
AT(d_004b0544, 0x004b0544, 0xc58);
AT(p_004b0550, 0x004b0550, 0xc68);
AT(d_004b0554, 0x004b0554, 0xc70);
AT(p_004b0560, 0x004b0560, 0xc80);
AT(d_004b0564, 0x004b0564, 0xc88);
AT(p_004b0570, 0x004b0570, 0xc98);
AT(d_004b0574, 0x004b0574, 0xca0);
AT(p_004b0580, 0x004b0580, 0xcb0);
AT(d_004b0584, 0x004b0584, 0xcb8);
AT(p_004b0590, 0x004b0590, 0xcc8);
AT(d_004b0594, 0x004b0594, 0xcd0);
AT(p_004b05a0, 0x004b05a0, 0xce0);
AT(d_004b05a4, 0x004b05a4, 0xce8);
AT(p_004b05b0, 0x004b05b0, 0xcf8);
AT(d_004b05b4, 0x004b05b4, 0xd00);
AT(p_004b05c0, 0x004b05c0, 0xd10);
AT(d_004b05c4, 0x004b05c4, 0xd18);
AT(p_004b05d0, 0x004b05d0, 0xd28);
AT(d_004b05d4, 0x004b05d4, 0xd30);
AT(p_004b05e0, 0x004b05e0, 0xd40);
AT(d_004b05e4, 0x004b05e4, 0xd48);
AT(p_004b05f0, 0x004b05f0, 0xd58);
AT(d_004b05f4, 0x004b05f4, 0xd60);
AT(p_004b0600, 0x004b0600, 0xd70);
AT(d_004b0604, 0x004b0604, 0xd78);
AT(p_004b0610, 0x004b0610, 0xd88);
AT(d_004b0614, 0x004b0614, 0xd90);
AT(p_004b0620, 0x004b0620, 0xda0);
AT(d_004b0624, 0x004b0624, 0xda8);
AT(p_004b0630, 0x004b0630, 0xdb8);
AT(d_004b0634, 0x004b0634, 0xdc0);
AT(p_004b0640, 0x004b0640, 0xdd0);
AT(d_004b0644, 0x004b0644, 0xdd8);
AT(p_004b0650, 0x004b0650, 0xde8);
AT(d_004b0654, 0x004b0654, 0xdf0);
AT(p_004b0660, 0x004b0660, 0xe00);
AT(d_004b0664, 0x004b0664, 0xe08);
AT(p_004b0670, 0x004b0670, 0xe18);
AT(d_004b0674, 0x004b0674, 0xe20);
AT(p_004b0680, 0x004b0680, 0xe30);
AT(d_004b0684, 0x004b0684, 0xe38);
AT(p_004b0690, 0x004b0690, 0xe48);
AT(d_004b0694, 0x004b0694, 0xe50);
AT(p_004b06a0, 0x004b06a0, 0xe60);
AT(d_004b06a4, 0x004b06a4, 0xe68);
AT(p_004b06b0, 0x004b06b0, 0xe78);
AT(d_004b06b4, 0x004b06b4, 0xe80);
AT(p_004b06c0, 0x004b06c0, 0xe90);
AT(d_004b06c4, 0x004b06c4, 0xe98);
AT(p_004b06d0, 0x004b06d0, 0xea8);
AT(d_004b06d4, 0x004b06d4, 0xeb0);
AT(p_004b06e0, 0x004b06e0, 0xec0);
AT(d_004b06e4, 0x004b06e4, 0xec8);
AT(p_004b06f0, 0x004b06f0, 0xed8);
AT(d_004b06f4, 0x004b06f4, 0xee0);
AT(p_004b0700, 0x004b0700, 0xef0);
AT(d_004b0704, 0x004b0704, 0xef8);
AT(p_004b0710, 0x004b0710, 0xf08);
AT(d_004b0714, 0x004b0714, 0xf10);
AT(p_004b0720, 0x004b0720, 0xf20);
AT(d_004b0724, 0x004b0724, 0xf28);
AT(p_004b0730, 0x004b0730, 0xf38);
AT(d_004b0734, 0x004b0734, 0xf40);
AT(p_004b0740, 0x004b0740, 0xf50);
AT(d_004b0744, 0x004b0744, 0xf58);
AT(p_004b0750, 0x004b0750, 0xf68);
AT(d_004b0754, 0x004b0754, 0xf70);
AT(p_004b0760, 0x004b0760, 0xf80);
AT(d_004b0764, 0x004b0764, 0xf88);
AT(p_004b0770, 0x004b0770, 0xf98);
AT(d_004b0774, 0x004b0774, 0xfa0);
AT(p_004b0780, 0x004b0780, 0xfb0);
AT(d_004b0784, 0x004b0784, 0xfb8);
AT(p_004b0790, 0x004b0790, 0xfc8);
AT(d_004b0794, 0x004b0794, 0xfd0);
AT(p_004b07a0, 0x004b07a0, 0xfe0);
AT(d_004b07a4, 0x004b07a4, 0xfe8);
AT(p_004b07b0, 0x004b07b0, 0xff8);
AT(d_004b07b4, 0x004b07b4, 0x1000);
AT(p_004b07c0, 0x004b07c0, 0x1010);
AT(d_004b07c4, 0x004b07c4, 0x1018);
AT(p_004b07d0, 0x004b07d0, 0x1028);
AT(d_004b07d4, 0x004b07d4, 0x1030);
AT(p_004b07e0, 0x004b07e0, 0x1040);
AT(d_004b07e4, 0x004b07e4, 0x1048);
AT(p_004b07f0, 0x004b07f0, 0x1058);
AT(d_004b07f4, 0x004b07f4, 0x1060);
AT(p_004b0800, 0x004b0800, 0x1070);
AT(d_004b0804, 0x004b0804, 0x1078);
AT(p_004b0810, 0x004b0810, 0x1088);
AT(d_004b0814, 0x004b0814, 0x1090);
AT(p_004b0820, 0x004b0820, 0x10a0);
AT(d_004b0824, 0x004b0824, 0x10a8);
AT(p_004b0830, 0x004b0830, 0x10b8);
AT(d_004b0834, 0x004b0834, 0x10c0);
AT(p_004b0840, 0x004b0840, 0x10d0);
AT(d_004b0844, 0x004b0844, 0x10d8);
AT(p_004b0850, 0x004b0850, 0x10e8);
AT(d_004b0854, 0x004b0854, 0x10f0);
AT(p_004b0860, 0x004b0860, 0x1100);
AT(d_004b0864, 0x004b0864, 0x1108);
AT(p_004b0870, 0x004b0870, 0x1118);
AT(d_004b0874, 0x004b0874, 0x1120);
AT(p_004b0880, 0x004b0880, 0x1130);
AT(d_004b0884, 0x004b0884, 0x1138);
AT(p_004b0890, 0x004b0890, 0x1148);
AT(d_004b0894, 0x004b0894, 0x1150);
AT(p_004b08a0, 0x004b08a0, 0x1160);
AT(d_004b08a4, 0x004b08a4, 0x1168);
AT(p_004b08b0, 0x004b08b0, 0x1178);
AT(d_004b08b4, 0x004b08b4, 0x1180);
AT(p_004b08c0, 0x004b08c0, 0x1190);
AT(d_004b08c4, 0x004b08c4, 0x1198);
AT(p_004b08d0, 0x004b08d0, 0x11a8);
AT(d_004b08d4, 0x004b08d4, 0x11b0);
AT(p_004b08e0, 0x004b08e0, 0x11c0);
AT(d_004b08e4, 0x004b08e4, 0x11c8);
AT(p_004b08f0, 0x004b08f0, 0x11d8);
AT(d_004b08f4, 0x004b08f4, 0x11e0);
AT(p_004b0900, 0x004b0900, 0x11f0);
AT(d_004b0904, 0x004b0904, 0x11f8);
AT(p_004b0910, 0x004b0910, 0x1208);
AT(d_004b0914, 0x004b0914, 0x1210);
AT(p_004b0920, 0x004b0920, 0x1220);
AT(d_004b0924, 0x004b0924, 0x1228);
AT(p_004b0930, 0x004b0930, 0x1238);
AT(d_004b0934, 0x004b0934, 0x1240);
AT(p_004b0940, 0x004b0940, 0x1250);
AT(d_004b0944, 0x004b0944, 0x1258);
AT(p_004b0950, 0x004b0950, 0x1268);
AT(d_004b0954, 0x004b0954, 0x1270);
AT(p_004b0960, 0x004b0960, 0x1280);
AT(d_004b0964, 0x004b0964, 0x1288);
AT(p_004b0970, 0x004b0970, 0x1298);
AT(d_004b0974, 0x004b0974, 0x12a0);
AT(p_004b0980, 0x004b0980, 0x12b0);
AT(d_004b0984, 0x004b0984, 0x12b8);
AT(p_004b0990, 0x004b0990, 0x12c8);
AT(d_004b0994, 0x004b0994, 0x12d0);
AT(p_004b09a0, 0x004b09a0, 0x12e0);
AT(d_004b09a4, 0x004b09a4, 0x12e8);
AT(p_004b09b0, 0x004b09b0, 0x12f8);
AT(d_004b09b4, 0x004b09b4, 0x1300);
AT(p_004b09c0, 0x004b09c0, 0x1310);
AT(d_004b09c4, 0x004b09c4, 0x1318);
AT(p_004b09d0, 0x004b09d0, 0x1328);
AT(d_004b09d4, 0x004b09d4, 0x1330);
AT(p_004b09e0, 0x004b09e0, 0x1340);
AT(d_004b09e4, 0x004b09e4, 0x1348);
AT(p_004b09f0, 0x004b09f0, 0x1358);
AT(d_004b09f4, 0x004b09f4, 0x1360);
AT(p_004b0a00, 0x004b0a00, 0x1370);
AT(d_004b0a04, 0x004b0a04, 0x1378);
AT(p_004b0a10, 0x004b0a10, 0x1388);
AT(d_004b0a14, 0x004b0a14, 0x1390);
AT(p_004b0a20, 0x004b0a20, 0x13a0);
AT(d_004b0a24, 0x004b0a24, 0x13a8);
AT(p_004b0a30, 0x004b0a30, 0x13b8);
AT(d_004b0a34, 0x004b0a34, 0x13c0);
AT(p_004b0a40, 0x004b0a40, 0x13d0);
AT(d_004b0a44, 0x004b0a44, 0x13d8);
AT(p_004b0a50, 0x004b0a50, 0x13e8);
AT(d_004b0a54, 0x004b0a54, 0x13f0);
AT(p_004b0a60, 0x004b0a60, 0x1400);
AT(d_004b0a64, 0x004b0a64, 0x1408);
AT(p_004b0a70, 0x004b0a70, 0x1418);
AT(d_004b0a74, 0x004b0a74, 0x1420);
AT(p_004b0a80, 0x004b0a80, 0x1430);
AT(d_004b0a84, 0x004b0a84, 0x1438);
AT(p_004b0a90, 0x004b0a90, 0x1448);
AT(d_004b0a94, 0x004b0a94, 0x1450);
AT(p_004b0aa0, 0x004b0aa0, 0x1460);
AT(d_004b0aa4, 0x004b0aa4, 0x1468);
AT(p_004b0ab0, 0x004b0ab0, 0x1478);
AT(d_004b0ab4, 0x004b0ab4, 0x1480);
AT(p_004b0ac0, 0x004b0ac0, 0x1490);
AT(d_004b0ac4, 0x004b0ac4, 0x1498);
AT(p_004b0ad0, 0x004b0ad0, 0x14a8);
AT(d_004b0ad4, 0x004b0ad4, 0x14b0);
AT(p_004b0ae0, 0x004b0ae0, 0x14c0);
AT(d_004b0ae4, 0x004b0ae4, 0x14c8);
AT(p_004b0af0, 0x004b0af0, 0x14d8);
AT(d_004b0af4, 0x004b0af4, 0x14e0);
AT(p_004b0b00, 0x004b0b00, 0x14f0);
AT(d_004b0b04, 0x004b0b04, 0x14f8);
AT(p_004b0b10, 0x004b0b10, 0x1508);
AT(d_004b0b14, 0x004b0b14, 0x1510);
AT(p_004b0b20, 0x004b0b20, 0x1520);
AT(d_004b0b24, 0x004b0b24, 0x1528);
AT(p_004b0b30, 0x004b0b30, 0x1538);
AT(d_004b0b34, 0x004b0b34, 0x1540);
AT(p_004b0b40, 0x004b0b40, 0x1550);
AT(d_004b0b44, 0x004b0b44, 0x1558);
AT(p_004b0b50, 0x004b0b50, 0x1568);
AT(d_004b0b54, 0x004b0b54, 0x1570);
AT(p_004b0b60, 0x004b0b60, 0x1580);
AT(d_004b0b64, 0x004b0b64, 0x1588);
AT(p_004b0b70, 0x004b0b70, 0x1598);
AT(d_004b0b74, 0x004b0b74, 0x15a0);
AT(p_004b0b80, 0x004b0b80, 0x15b0);
AT(d_004b0b84, 0x004b0b84, 0x15b8);
AT(p_004b0b90, 0x004b0b90, 0x15c8);
AT(d_004b0b94, 0x004b0b94, 0x15d0);
AT(p_004b0ba0, 0x004b0ba0, 0x15e0);
AT(d_004b0ba4, 0x004b0ba4, 0x15e8);
AT(p_004b0bb0, 0x004b0bb0, 0x15f8);
AT(d_004b0bb4, 0x004b0bb4, 0x1600);
AT(p_004b0bc0, 0x004b0bc0, 0x1610);
AT(d_004b0bc4, 0x004b0bc4, 0x1618);
AT(p_004b0bd0, 0x004b0bd0, 0x1628);
AT(d_004b0bd4, 0x004b0bd4, 0x1630);
AT(p_004b0be0, 0x004b0be0, 0x1640);
AT(d_004b0be4, 0x004b0be4, 0x1648);
AT(p_004b0bf0, 0x004b0bf0, 0x1658);
AT(d_004b0bf4, 0x004b0bf4, 0x1660);
AT(p_004b0c00, 0x004b0c00, 0x1670);
AT(d_004b0c04, 0x004b0c04, 0x1678);
AT(p_004b0c10, 0x004b0c10, 0x1688);
AT(d_004b0c14, 0x004b0c14, 0x1690);
AT(p_004b0c20, 0x004b0c20, 0x16a0);
AT(d_004b0c24, 0x004b0c24, 0x16a8);
AT(p_004b0c30, 0x004b0c30, 0x16b8);
AT(d_004b0c34, 0x004b0c34, 0x16c0);
AT(p_004b0c40, 0x004b0c40, 0x16d0);
AT(d_004b0c44, 0x004b0c44, 0x16d8);
AT(p_004b0c50, 0x004b0c50, 0x16e8);
AT(d_004b0c54, 0x004b0c54, 0x16f0);
AT(p_004b0c60, 0x004b0c60, 0x1700);
AT(d_004b0c64, 0x004b0c64, 0x1708);
AT(p_004b0c70, 0x004b0c70, 0x1718);
AT(d_004b0c74, 0x004b0c74, 0x1720);
AT(p_004b0c80, 0x004b0c80, 0x1730);
AT(d_004b0c84, 0x004b0c84, 0x1738);
AT(p_004b0c90, 0x004b0c90, 0x1748);
AT(d_004b0c94, 0x004b0c94, 0x1750);
AT(p_004b0ca0, 0x004b0ca0, 0x1760);
AT(d_004b0ca4, 0x004b0ca4, 0x1768);
AT(p_004b0cb0, 0x004b0cb0, 0x1778);
AT(d_004b0cb4, 0x004b0cb4, 0x1780);
AT(p_004b0cc0, 0x004b0cc0, 0x1790);
AT(d_004b0cc4, 0x004b0cc4, 0x1798);
AT(p_004b0cd0, 0x004b0cd0, 0x17a8);
AT(d_004b0cd4, 0x004b0cd4, 0x17b0);
AT(p_004b0ce0, 0x004b0ce0, 0x17c0);
AT(d_004b0ce4, 0x004b0ce4, 0x17c8);
AT(p_004b0cf0, 0x004b0cf0, 0x17d8);
AT(d_004b0cf4, 0x004b0cf4, 0x17e0);
AT(p_004b0d00, 0x004b0d00, 0x17f0);
AT(d_004b0d04, 0x004b0d04, 0x17f8);
AT(p_004b0d10, 0x004b0d10, 0x1808);
AT(d_004b0d14, 0x004b0d14, 0x1810);
AT(p_004b0d20, 0x004b0d20, 0x1820);
AT(d_004b0d24, 0x004b0d24, 0x1828);
AT(p_004b0d30, 0x004b0d30, 0x1838);
AT(d_004b0d34, 0x004b0d34, 0x1840);
AT(p_004b0d40, 0x004b0d40, 0x1850);
AT(d_004b0d44, 0x004b0d44, 0x1858);
AT(p_004b0d50, 0x004b0d50, 0x1868);
AT(d_004b0d54, 0x004b0d54, 0x1870);
AT(p_004b0d60, 0x004b0d60, 0x1880);
AT(d_004b0d64, 0x004b0d64, 0x1888);
AT(p_004b0d70, 0x004b0d70, 0x1898);
AT(d_004b0d74, 0x004b0d74, 0x18a0);
AT(p_004b0d80, 0x004b0d80, 0x18b0);
AT(d_004b0d84, 0x004b0d84, 0x18b8);
AT(p_004b0d90, 0x004b0d90, 0x18c8);
AT(d_004b0d94, 0x004b0d94, 0x18d0);
AT(p_004b0da0, 0x004b0da0, 0x18e0);
AT(d_004b0da4, 0x004b0da4, 0x18e8);
AT(p_004b0db0, 0x004b0db0, 0x18f8);
AT(d_004b0db4, 0x004b0db4, 0x1900);
AT(p_004b0dc0, 0x004b0dc0, 0x1910);
AT(d_004b0dc4, 0x004b0dc4, 0x1918);
AT(p_004b0dd0, 0x004b0dd0, 0x1928);
AT(d_004b0dd4, 0x004b0dd4, 0x1930);
AT(p_004b0de0, 0x004b0de0, 0x1940);
AT(d_004b0de4, 0x004b0de4, 0x1948);
AT(p_004b0df0, 0x004b0df0, 0x1958);
AT(d_004b0df4, 0x004b0df4, 0x1960);
AT(p_004b0e00, 0x004b0e00, 0x1970);
AT(d_004b0e04, 0x004b0e04, 0x1978);
AT(p_004b0e10, 0x004b0e10, 0x1988);
AT(d_004b0e14, 0x004b0e14, 0x1990);
AT(p_004b0e20, 0x004b0e20, 0x19a0);
AT(d_004b0e24, 0x004b0e24, 0x19a8);
AT(p_004b0e30, 0x004b0e30, 0x19b8);
AT(d_004b0e34, 0x004b0e34, 0x19c0);
AT(p_004b0e40, 0x004b0e40, 0x19d0);
AT(d_004b0e44, 0x004b0e44, 0x19d8);
AT(p_004b0e50, 0x004b0e50, 0x19e8);
AT(d_004b0e54, 0x004b0e54, 0x19f0);
AT(p_004b0e60, 0x004b0e60, 0x1a00);
AT(d_004b0e64, 0x004b0e64, 0x1a08);
AT(p_004b0e70, 0x004b0e70, 0x1a18);
AT(d_004b0e74, 0x004b0e74, 0x1a20);
AT(p_004b0e80, 0x004b0e80, 0x1a30);
AT(d_004b0e84, 0x004b0e84, 0x1a38);
AT(p_004b0e90, 0x004b0e90, 0x1a48);
AT(d_004b0e94, 0x004b0e94, 0x1a50);
AT(p_004b0ea0, 0x004b0ea0, 0x1a60);
AT(d_004b0ea4, 0x004b0ea4, 0x1a68);
AT(p_004b0eb0, 0x004b0eb0, 0x1a78);
AT(d_004b0eb4, 0x004b0eb4, 0x1a80);
AT(p_004b0ec0, 0x004b0ec0, 0x1a90);
AT(d_004b0ec4, 0x004b0ec4, 0x1a98);
AT(p_004b0ed0, 0x004b0ed0, 0x1aa8);
AT(d_004b0ed4, 0x004b0ed4, 0x1ab0);
AT(p_004b0ee0, 0x004b0ee0, 0x1ac0);
AT(d_004b0ee4, 0x004b0ee4, 0x1ac8);
AT(p_004b0ef0, 0x004b0ef0, 0x1ad8);
AT(d_004b0ef4, 0x004b0ef4, 0x1ae0);
AT(p_004b0f00, 0x004b0f00, 0x1af0);
AT(d_004b0f04, 0x004b0f04, 0x1af8);
AT(p_004b0f10, 0x004b0f10, 0x1b08);
AT(d_004b0f14, 0x004b0f14, 0x1b10);
#undef AT
_Static_assert(offsetof(struct BmxGroup_bmxInputTables, d_004b0f14) +
                       sizeof(((struct BmxGroup_bmxInputTables *)0)->d_004b0f14) ==
                   BMX_DATA_OFF(0x12c0, 0x1b1c),
               "bmxInputTables");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0054d8a4..0x0054db58.
 */
struct BmxGroup_gBikeStatsGroup {
    uint8_t lead_[4];                /* nothing: the members' offsets keep their original alignment */
    BikeStatTable gBikeStats;        /* 0x0054d8a4 */
    unsigned char pad_0054d9d0[392]; /* 0x0054d9d0 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gBikeStatsGroup, member) == (addr) - 0x0054d8a0, #member)
AT(gBikeStats, 0x0054d8a4);
AT(pad_0054d9d0, 0x0054d9d0);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gBikeStatsGroup, pad_0054d9d0) +
                       sizeof(((struct BmxGroup_gBikeStatsGroup *)0)->pad_0054d9d0) ==
                   0x2b8,
               "gBikeStatsGroup");

/* The type is local to zge/com/zglrw/zrwzgl.c. */
typedef uint32_t BmxType_gZrwKeyActions[256];

/* The type is local to zge/com/zglrw/zrwfile.c. */
typedef uint32_t BmxType_gZrwSeekOrigins[3];

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0054e298..0x0054e2a8.
 */
struct BmxGroup_d_0054e298Group {
    uint8_t lead_[8];              /* nothing: the members' offsets keep their original alignment */
    uint32_t d_0054e298;           /* 0x0054e298 */
    void *p_0054e29c[2];           /* 0x0054e29c */
    unsigned char pad_0054e2a4[4]; /* 0x0054e2a4 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_d_0054e298Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x0054e290, off64),                                             \
                   #member)
AT(d_0054e298, 0x0054e298, 0x8);
AT(p_0054e29c, 0x0054e29c, 0x10);
AT(pad_0054e2a4, 0x0054e2a4, 0x20);
#undef AT
_Static_assert(offsetof(struct BmxGroup_d_0054e298Group, pad_0054e2a4) +
                       sizeof(((struct BmxGroup_d_0054e298Group *)0)->pad_0054e2a4) ==
                   BMX_DATA_OFF(0x18, 0x24),
               "d_0054e298Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0054e780..0x0054e798.
 */
struct BmxGroup_d_0054e780Group {
    uint32_t d_0054e780; /* 0x0054e780 */
    void *p_0054e784[2]; /* 0x0054e784 */
    uint32_t d_0054e78c; /* 0x0054e78c */
    void *p_0054e790[2]; /* 0x0054e790 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_d_0054e780Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x0054e780, off64),                                             \
                   #member)
AT(d_0054e780, 0x0054e780, 0x0);
AT(p_0054e784, 0x0054e784, 0x8);
AT(d_0054e78c, 0x0054e78c, 0x18);
AT(p_0054e790, 0x0054e790, 0x20);
#undef AT
_Static_assert(offsetof(struct BmxGroup_d_0054e780Group, p_0054e790) +
                       sizeof(((struct BmxGroup_d_0054e780Group *)0)->p_0054e790) ==
                   BMX_DATA_OFF(0x18, 0x30),
               "d_0054e780Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0054e798..0x0054e7a4.
 */
struct BmxGroup_d_0054e798Group {
    uint8_t lead_[8];    /* nothing: the members' offsets keep their original alignment */
    uint32_t d_0054e798; /* 0x0054e798 */
    void *p_0054e79c[2]; /* 0x0054e79c */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_d_0054e798Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x0054e790, off64),                                             \
                   #member)
AT(d_0054e798, 0x0054e798, 0x8);
AT(p_0054e79c, 0x0054e79c, 0x10);
#undef AT
_Static_assert(offsetof(struct BmxGroup_d_0054e798Group, p_0054e79c) +
                       sizeof(((struct BmxGroup_d_0054e798Group *)0)->p_0054e79c) ==
                   BMX_DATA_OFF(0x14, 0x20),
               "d_0054e798Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0054e7c8..0x0054e7e0.
 */
struct BmxGroup_gZpadFormatJoystickGroup {
    uint8_t lead_[8];             /* nothing: the members' offsets keep their original alignment */
    uint8_t gZpadFormatJoystick;  /* 0x0054e7c8 */
    unsigned char d_0054e7c9[19]; /* 0x0054e7c9 */
    void *p_0054e7dc;             /* 0x0054e7dc */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gZpadFormatJoystickGroup, member) ==                             \
                       BMX_DATA_OFF((addr) - 0x0054e7c0, off64),                                             \
                   #member)
AT(gZpadFormatJoystick, 0x0054e7c8, 0x8);
AT(d_0054e7c9, 0x0054e7c9, 0x9);
AT(p_0054e7dc, 0x0054e7dc, 0x20);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZpadFormatJoystickGroup, p_0054e7dc) +
                       sizeof(((struct BmxGroup_gZpadFormatJoystickGroup *)0)->p_0054e7dc) ==
                   BMX_DATA_OFF(0x20, 0x28),
               "gZpadFormatJoystickGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0054e7e0..0x0054f2f8.
 */
struct BmxGroup_gZpadFormatKeyboardGroup {
    uint8_t gZpadFormatKeyboard;  /* 0x0054e7e0 */
    unsigned char d_0054e7e1[19]; /* 0x0054e7e1 */
    void *p_0054e7f4;             /* 0x0054e7f4 */
    uint32_t d_0054e7f8[704];     /* 0x0054e7f8 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gZpadFormatKeyboardGroup, member) ==                             \
                       BMX_DATA_OFF((addr) - 0x0054e7e0, off64),                                             \
                   #member)
AT(gZpadFormatKeyboard, 0x0054e7e0, 0x0);
AT(d_0054e7e1, 0x0054e7e1, 0x1);
AT(p_0054e7f4, 0x0054e7f4, 0x18);
AT(d_0054e7f8, 0x0054e7f8, 0x20);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZpadFormatKeyboardGroup, d_0054e7f8) +
                       sizeof(((struct BmxGroup_gZpadFormatKeyboardGroup *)0)->d_0054e7f8) ==
                   BMX_DATA_OFF(0xb18, 0xb20),
               "gZpadFormatKeyboardGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0054f2f8..0x0054f328.
 */
struct BmxGroup_gZpadGuidKeyboardGroup {
    uint8_t lead_[8];           /* nothing: the members' offsets keep their original alignment */
    ZpadGuid gZpadGuidKeyboard; /* 0x0054f2f8 */
    uint32_t d_0054f308[8];     /* 0x0054f308 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gZpadGuidKeyboardGroup, member) == (addr) - 0x0054f2f0, #member)
AT(gZpadGuidKeyboard, 0x0054f2f8);
AT(d_0054f308, 0x0054f308);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZpadGuidKeyboardGroup, d_0054f308) +
                       sizeof(((struct BmxGroup_gZpadGuidKeyboardGroup *)0)->d_0054f308) ==
                   0x38,
               "gZpadGuidKeyboardGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0054f408..0x0054f448.
 */
struct BmxGroup_gZpadIidDeviceGroup {
    uint8_t lead_[8];        /* nothing: the members' offsets keep their original alignment */
    ZpadGuid gZpadIidDevice; /* 0x0054f408 */
    uint32_t d_0054f418[12]; /* 0x0054f418 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gZpadIidDeviceGroup, member) == (addr) - 0x0054f400, #member)
AT(gZpadIidDevice, 0x0054f408);
AT(d_0054f418, 0x0054f418);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZpadIidDeviceGroup, d_0054f418) +
                       sizeof(((struct BmxGroup_gZpadIidDeviceGroup *)0)->d_0054f418) ==
                   0x48,
               "gZpadIidDeviceGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0054f448..0x0054fee8.
 */
struct BmxGroup_gZpadIidDirectInputGroup {
    uint8_t lead_[8];             /* nothing: the members' offsets keep their original alignment */
    ZpadGuid gZpadIidDirectInput; /* 0x0054f448 */
    uint32_t d_0054f458[676];     /* 0x0054f458 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gZpadIidDirectInputGroup, member) == (addr) - 0x0054f440, #member)
AT(gZpadIidDirectInput, 0x0054f448);
AT(d_0054f458, 0x0054f458);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZpadIidDirectInputGroup, d_0054f458) +
                       sizeof(((struct BmxGroup_gZpadIidDirectInputGroup *)0)->d_0054f458) ==
                   0xaa8,
               "gZpadIidDirectInputGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00550240..0x0055024c.
 */
struct BmxGroup_d_00550240Group {
    uint32_t d_00550240; /* 0x00550240 */
    void *p_00550244[2]; /* 0x00550244 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_d_00550240Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x00550240, off64),                                             \
                   #member)
AT(d_00550240, 0x00550240, 0x0);
AT(p_00550244, 0x00550244, 0x8);
#undef AT
_Static_assert(offsetof(struct BmxGroup_d_00550240Group, p_00550244) +
                       sizeof(((struct BmxGroup_d_00550240Group *)0)->p_00550244) ==
                   BMX_DATA_OFF(0xc, 0x18),
               "d_00550240Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00550290..0x00550bd8.
 */
struct BmxGroup_d_00550290Group {
    uint32_t d_00550290;              /* 0x00550290 */
    void *p_00550294[2];              /* 0x00550294 */
    uint32_t d_0055029c;              /* 0x0055029c */
    void *p_005502a0[2];              /* 0x005502a0 */
    unsigned char pad_005502a8[2352]; /* 0x005502a8 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_d_00550290Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x00550290, off64),                                             \
                   #member)
AT(d_00550290, 0x00550290, 0x0);
AT(p_00550294, 0x00550294, 0x8);
AT(d_0055029c, 0x0055029c, 0x18);
AT(p_005502a0, 0x005502a0, 0x20);
AT(pad_005502a8, 0x005502a8, 0x30);
#undef AT
_Static_assert(offsetof(struct BmxGroup_d_00550290Group, pad_005502a8) +
                       sizeof(((struct BmxGroup_d_00550290Group *)0)->pad_005502a8) ==
                   BMX_DATA_OFF(0x948, 0x960),
               "d_00550290Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00551208..0x00551418.
 */
struct BmxGroup_d_00551208Group {
    uint8_t lead_[8];                /* nothing: the members' offsets keep their original alignment */
    unsigned char d_00551208[260];   /* 0x00551208 */
    void *p_0055130c[2];             /* 0x0055130c */
    unsigned char pad_00551314[260]; /* 0x00551314 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_d_00551208Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x00551200, off64),                                             \
                   #member)
AT(d_00551208, 0x00551208, 0x8);
AT(p_0055130c, 0x0055130c, 0x110);
AT(pad_00551314, 0x00551314, 0x120);
#undef AT
_Static_assert(offsetof(struct BmxGroup_d_00551208Group, pad_00551314) +
                       sizeof(((struct BmxGroup_d_00551208Group *)0)->pad_00551314) ==
                   BMX_DATA_OFF(0x218, 0x224),
               "d_00551208Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0055151c..0x00551724.
 */
struct BmxGroup_d_0055151cGroup {
    uint8_t lead_[12];               /* nothing: the members' offsets keep their original alignment */
    unsigned char d_0055151c[260];   /* 0x0055151c */
    void *p_00551620;                /* 0x00551620 */
    unsigned char pad_00551624[256]; /* 0x00551624 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_d_0055151cGroup, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x00551510, off64),                                             \
                   #member)
AT(d_0055151c, 0x0055151c, 0xc);
AT(p_00551620, 0x00551620, 0x110);
AT(pad_00551624, 0x00551624, 0x118);
#undef AT
_Static_assert(offsetof(struct BmxGroup_d_0055151cGroup, pad_00551624) +
                       sizeof(((struct BmxGroup_d_0055151cGroup *)0)->pad_00551624) ==
                   BMX_DATA_OFF(0x214, 0x218),
               "d_0055151cGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00551c54..0x00551f68.
 */
struct BmxGroup_gCameraTuningGroup {
    uint8_t lead_[4];                /* nothing: the members' offsets keep their original alignment */
    CameraTuning gCameraTuning;      /* 0x00551c54 */
    unsigned char pad_00551ee4[132]; /* 0x00551ee4 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gCameraTuningGroup, member) == (addr) - 0x00551c50, #member)
AT(gCameraTuning, 0x00551c54);
AT(pad_00551ee4, 0x00551ee4);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gCameraTuningGroup, pad_00551ee4) +
                       sizeof(((struct BmxGroup_gCameraTuningGroup *)0)->pad_00551ee4) ==
                   0x318,
               "gCameraTuningGroup");

/* The type is local to bmx/com/action/game.c. */
typedef uint8_t BmxType_gGameLevelInfoIndex[12];

/* SAVE.DAT's records: memcard.c copies them as one range, 0x00551fd8..0x00553658. */
struct BmxGroup_bmxSaveRecords {
    uint8_t lead_[8];              /* nothing: the members' offsets keep their original alignment */
    uint32_t d_00551fd8[2];        /* 0x00551fd8, also gComdataRecords */
    char sIracleBoy[12];           /* 0x00551fe0 */
    uint32_t d_00551fec[2];        /* 0x00551fec */
    char s_00551ff4[12];           /* 0x00551ff4 */
    uint32_t d_00552000[2];        /* 0x00552000 */
    char s_00552008[12];           /* 0x00552008 */
    uint32_t d_00552014[2];        /* 0x00552014 */
    char sYTor[8];                 /* 0x0055201c */
    uint32_t d_00552024[3];        /* 0x00552024 */
    char s_00552030[8];            /* 0x00552030 */
    uint32_t d_00552038[3];        /* 0x00552038 */
    char s_00552044[8];            /* 0x00552044 */
    uint32_t d_0055204c[3];        /* 0x0055204c */
    char s_00552058[8];            /* 0x00552058 */
    unsigned char pad_00552060[4]; /* 0x00552060 */
    uint32_t d_00552064[2];        /* 0x00552064 */
    char s_0055206c[8];            /* 0x0055206c */
    unsigned char pad_00552074[4]; /* 0x00552074 */
    uint32_t d_00552078[2];        /* 0x00552078 */
    char s_00552080[8];            /* 0x00552080 */
    unsigned char pad_00552088[4]; /* 0x00552088 */
    uint32_t d_0055208c[2];        /* 0x0055208c */
    char s_00552094[8];            /* 0x00552094 */
    unsigned char pad_0055209c[4]; /* 0x0055209c */
    uint32_t d_005520a0[2];        /* 0x005520a0 */
    char s_005520a8[8];            /* 0x005520a8 */
    unsigned char pad_005520b0[4]; /* 0x005520b0 */
    uint32_t d_005520b4[2];        /* 0x005520b4 */
    char s_005520bc[8];            /* 0x005520bc */
    unsigned char pad_005520c4[4]; /* 0x005520c4 */
    uint32_t d_005520c8[2];        /* 0x005520c8 */
    char s_005520d0[8];            /* 0x005520d0 */
    uint32_t d_005520d8[3];        /* 0x005520d8 */
    char s_005520e4[8];            /* 0x005520e4 */
    unsigned char pad_005520ec[4]; /* 0x005520ec */
    uint32_t d_005520f0[2];        /* 0x005520f0 */
    char s_005520f8[8];            /* 0x005520f8 */
    uint32_t d_00552100[3];        /* 0x00552100 */
    char s_0055210c[8];            /* 0x0055210c */
    uint32_t d_00552114[3];        /* 0x00552114 */
    char s_00552120[12];           /* 0x00552120 */
    uint32_t d_0055212c[2];        /* 0x0055212c */
    char s_00552134[12];           /* 0x00552134 */
    uint32_t d_00552140[2];        /* 0x00552140 */
    char s_00552148[12];           /* 0x00552148 */
    uint32_t d_00552154[2];        /* 0x00552154 */
    char s_0055215c[12];           /* 0x0055215c */
    uint32_t d_00552168[2];        /* 0x00552168 */
    char s_00552170[12];           /* 0x00552170 */
    uint32_t d_0055217c[2];        /* 0x0055217c */
    char s_00552184[12];           /* 0x00552184 */
    uint32_t d_00552190[2];        /* 0x00552190 */
    char s_00552198[12];           /* 0x00552198 */
    uint32_t d_005521a4[2];        /* 0x005521a4 */
    char s_005521ac[12];           /* 0x005521ac */
    uint32_t d_005521b8[2];        /* 0x005521b8 */
    char sOrCal[8];                /* 0x005521c0 */
    uint32_t d_005521c8[3];        /* 0x005521c8 */
    char s_005521d4[8];            /* 0x005521d4 */
    uint32_t d_005521dc[3];        /* 0x005521dc */
    char s_005521e8[8];            /* 0x005521e8 */
    uint32_t d_005521f0[3];        /* 0x005521f0 */
    char s_005521fc[8];            /* 0x005521fc */
    uint32_t d_00552204[3];        /* 0x00552204 */
    char s_00552210[8];            /* 0x00552210 */
    uint32_t d_00552218[3];        /* 0x00552218 */
    char s_00552224[8];            /* 0x00552224 */
    uint32_t d_0055222c[3];        /* 0x0055222c */
    char s_00552238[8];            /* 0x00552238 */
    uint32_t d_00552240[3];        /* 0x00552240 */
    char s_0055224c[8];            /* 0x0055224c */
    uint32_t d_00552254[3];        /* 0x00552254 */
    char s_00552260[8];            /* 0x00552260 */
    uint32_t d_00552268[3];        /* 0x00552268 */
    char s_00552274[8];            /* 0x00552274 */
    uint32_t d_0055227c[3];        /* 0x0055227c */
    char s_00552288[8];            /* 0x00552288 */
    uint32_t d_00552290[3];        /* 0x00552290 */
    char s_0055229c[8];            /* 0x0055229c */
    uint32_t d_005522a4[3];        /* 0x005522a4 */
    char s_005522b0[8];            /* 0x005522b0 */
    uint32_t d_005522b8[3];        /* 0x005522b8 */
    char s_005522c4[8];            /* 0x005522c4 */
    uint32_t d_005522cc[3];        /* 0x005522cc */
    char s_005522d8[8];            /* 0x005522d8 */
    uint32_t d_005522e0[3];        /* 0x005522e0 */
    char s_005522ec[8];            /* 0x005522ec */
    uint32_t d_005522f4[3];        /* 0x005522f4 */
    char s_00552300[8];            /* 0x00552300 */
    uint32_t d_00552308[3];        /* 0x00552308 */
    char s_00552314[8];            /* 0x00552314 */
    uint32_t d_0055231c[3];        /* 0x0055231c */
    char s_00552328[8];            /* 0x00552328 */
    uint32_t d_00552330[3];        /* 0x00552330 */
    char s_0055233c[8];            /* 0x0055233c */
    uint32_t d_00552344[3];        /* 0x00552344 */
    char s_00552350[8];            /* 0x00552350 */
    uint32_t d_00552358[3];        /* 0x00552358 */
    char s_00552364[8];            /* 0x00552364 */
    uint32_t d_0055236c[3];        /* 0x0055236c */
    char s_00552378[8];            /* 0x00552378 */
    uint32_t d_00552380[3];        /* 0x00552380 */
    char s_0055238c[8];            /* 0x0055238c */
    uint32_t d_00552394[3];        /* 0x00552394 */
    char sArpig[8];                /* 0x005523a0 */
    uint32_t d_005523a8[3];        /* 0x005523a8 */
    char s_005523b4[8];            /* 0x005523b4 */
    uint32_t d_005523bc[3];        /* 0x005523bc */
    char s_005523c8[8];            /* 0x005523c8 */
    uint32_t d_005523d0[3];        /* 0x005523d0 */
    char sEbby[8];                 /* 0x005523dc */
    uint32_t d_005523e4[3];        /* 0x005523e4 */
    char s_005523f0[8];            /* 0x005523f0 */
    uint32_t d_005523f8[3];        /* 0x005523f8 */
    char s_00552404[8];            /* 0x00552404 */
    uint32_t d_0055240c[3];        /* 0x0055240c */
    char s_00552418[8];            /* 0x00552418 */
    uint32_t d_00552420[3];        /* 0x00552420 */
    char s_0055242c[8];            /* 0x0055242c */
    uint32_t d_00552434[3];        /* 0x00552434 */
    char s_00552440[8];            /* 0x00552440 */
    uint32_t d_00552448[3];        /* 0x00552448 */
    char s_00552454[8];            /* 0x00552454 */
    uint32_t d_0055245c[3];        /* 0x0055245c */
    char s_00552468[8];            /* 0x00552468 */
    uint32_t d_00552470[3];        /* 0x00552470 */
    char s_0055247c[8];            /* 0x0055247c */
    uint32_t d_00552484[3];        /* 0x00552484 */
    char s_00552490[8];            /* 0x00552490 */
    uint32_t d_00552498[3];        /* 0x00552498 */
    char s_005524a4[8];            /* 0x005524a4 */
    uint32_t d_005524ac[3];        /* 0x005524ac */
    char s_005524b8[8];            /* 0x005524b8 */
    uint32_t d_005524c0[3];        /* 0x005524c0 */
    char s_005524cc[8];            /* 0x005524cc */
    uint32_t d_005524d4[3];        /* 0x005524d4 */
    char s_005524e0[8];            /* 0x005524e0 */
    uint32_t d_005524e8[3];        /* 0x005524e8 */
    char s_005524f4[8];            /* 0x005524f4 */
    uint32_t d_005524fc[3];        /* 0x005524fc */
    char s_00552508[8];            /* 0x00552508 */
    uint32_t d_00552510[3];        /* 0x00552510 */
    char s_0055251c[8];            /* 0x0055251c */
    uint32_t d_00552524[3];        /* 0x00552524 */
    char s_00552530[8];            /* 0x00552530 */
    uint32_t d_00552538[3];        /* 0x00552538 */
    char s_00552544[8];            /* 0x00552544 */
    uint32_t d_0055254c[3];        /* 0x0055254c */
    char s_00552558[8];            /* 0x00552558 */
    uint32_t d_00552560[3];        /* 0x00552560 */
    char s_0055256c[8];            /* 0x0055256c */
    uint32_t d_00552574[3];        /* 0x00552574 */
    char sBreed[8];                /* 0x00552580 */
    uint32_t d_00552588[3];        /* 0x00552588 */
    char s_00552594[8];            /* 0x00552594 */
    uint32_t d_0055259c[3];        /* 0x0055259c */
    char s_005525a8[8];            /* 0x005525a8 */
    uint32_t d_005525b0[3];        /* 0x005525b0 */
    char s_005525bc[8];            /* 0x005525bc */
    uint32_t d_005525c4[3];        /* 0x005525c4 */
    char s_005525d0[8];            /* 0x005525d0 */
    uint32_t d_005525d8[3];        /* 0x005525d8 */
    char s_005525e4[8];            /* 0x005525e4 */
    uint32_t d_005525ec[3];        /* 0x005525ec */
    char s_005525f8[8];            /* 0x005525f8 */
    uint32_t d_00552600[3];        /* 0x00552600 */
    char s_0055260c[8];            /* 0x0055260c */
    uint32_t d_00552614[3];        /* 0x00552614 */
    char s_00552620[8];            /* 0x00552620 */
    uint32_t d_00552628[3];        /* 0x00552628 */
    char s_00552634[8];            /* 0x00552634 */
    uint32_t d_0055263c[3];        /* 0x0055263c */
    char s_00552648[8];            /* 0x00552648 */
    uint32_t d_00552650[3];        /* 0x00552650 */
    char s_0055265c[8];            /* 0x0055265c */
    uint32_t d_00552664[3];        /* 0x00552664 */
    char s_00552670[8];            /* 0x00552670 */
    uint32_t d_00552678[3];        /* 0x00552678 */
    char s_00552684[8];            /* 0x00552684 */
    uint32_t d_0055268c[3];        /* 0x0055268c */
    char s_00552698[8];            /* 0x00552698 */
    uint32_t d_005526a0[3];        /* 0x005526a0 */
    char s_005526ac[8];            /* 0x005526ac */
    uint32_t d_005526b4[3];        /* 0x005526b4 */
    char s_005526c0[8];            /* 0x005526c0 */
    uint32_t d_005526c8[3];        /* 0x005526c8 */
    char s_005526d4[8];            /* 0x005526d4 */
    uint32_t d_005526dc[3];        /* 0x005526dc */
    char s_005526e8[8];            /* 0x005526e8 */
    uint32_t d_005526f0[3];        /* 0x005526f0 */
    char s_005526fc[8];            /* 0x005526fc */
    uint32_t d_00552704[3];        /* 0x00552704 */
    char s_00552710[8];            /* 0x00552710 */
    uint32_t d_00552718[3];        /* 0x00552718 */
    char s_00552724[8];            /* 0x00552724 */
    uint32_t d_0055272c[3];        /* 0x0055272c */
    char s_00552738[8];            /* 0x00552738 */
    uint32_t d_00552740[3];        /* 0x00552740 */
    char s_0055274c[8];            /* 0x0055274c */
    uint32_t d_00552754[3];        /* 0x00552754 */
    char sAGY[8];                  /* 0x00552760 */
    uint32_t d_00552768[3];        /* 0x00552768 */
    char s_00552774[8];            /* 0x00552774 */
    uint32_t d_0055277c[3];        /* 0x0055277c */
    char s_00552788[8];            /* 0x00552788 */
    uint32_t d_00552790[3];        /* 0x00552790 */
    char s_0055279c[8];            /* 0x0055279c */
    uint32_t d_005527a4[3];        /* 0x005527a4 */
    char s_005527b0[8];            /* 0x005527b0 */
    uint32_t d_005527b8[3];        /* 0x005527b8 */
    char s_005527c4[8];            /* 0x005527c4 */
    uint32_t d_005527cc[3];        /* 0x005527cc */
    char s_005527d8[8];            /* 0x005527d8 */
    uint32_t d_005527e0[3];        /* 0x005527e0 */
    char s_005527ec[8];            /* 0x005527ec */
    uint32_t d_005527f4[3];        /* 0x005527f4 */
    char s_00552800[8];            /* 0x00552800 */
    uint32_t d_00552808[3];        /* 0x00552808 */
    char s_00552814[8];            /* 0x00552814 */
    uint32_t d_0055281c[3];        /* 0x0055281c */
    char s_00552828[8];            /* 0x00552828 */
    uint32_t d_00552830[3];        /* 0x00552830 */
    char s_0055283c[8];            /* 0x0055283c */
    uint32_t d_00552844[3];        /* 0x00552844 */
    char s_00552850[8];            /* 0x00552850 */
    uint32_t d_00552858[3];        /* 0x00552858 */
    char s_00552864[8];            /* 0x00552864 */
    uint32_t d_0055286c[3];        /* 0x0055286c */
    char s_00552878[8];            /* 0x00552878 */
    uint32_t d_00552880[3];        /* 0x00552880 */
    char s_0055288c[8];            /* 0x0055288c */
    uint32_t d_00552894[3];        /* 0x00552894 */
    char s_005528a0[8];            /* 0x005528a0 */
    uint32_t d_005528a8[3];        /* 0x005528a8 */
    char s_005528b4[8];            /* 0x005528b4 */
    uint32_t d_005528bc[3];        /* 0x005528bc */
    char s_005528c8[8];            /* 0x005528c8 */
    uint32_t d_005528d0[3];        /* 0x005528d0 */
    char s_005528dc[8];            /* 0x005528dc */
    uint32_t d_005528e4[3];        /* 0x005528e4 */
    char s_005528f0[8];            /* 0x005528f0 */
    uint32_t d_005528f8[3];        /* 0x005528f8 */
    char s_00552904[8];            /* 0x00552904 */
    uint32_t d_0055290c[3];        /* 0x0055290c */
    char s_00552918[8];            /* 0x00552918 */
    uint32_t d_00552920[3];        /* 0x00552920 */
    char s_0055292c[8];            /* 0x0055292c */
    uint32_t d_00552934[3];        /* 0x00552934 */
    char sUrbo[8];                 /* 0x00552940 */
    uint32_t d_00552948[3];        /* 0x00552948 */
    char s_00552954[8];            /* 0x00552954 */
    uint32_t d_0055295c[3];        /* 0x0055295c */
    char s_00552968[8];            /* 0x00552968 */
    uint32_t d_00552970[3];        /* 0x00552970 */
    char s_0055297c[8];            /* 0x0055297c */
    uint32_t d_00552984[3];        /* 0x00552984 */
    char s_00552990[8];            /* 0x00552990 */
    uint32_t d_00552998[3];        /* 0x00552998 */
    char s_005529a4[8];            /* 0x005529a4 */
    uint32_t d_005529ac[3];        /* 0x005529ac */
    char s_005529b8[8];            /* 0x005529b8 */
    uint32_t d_005529c0[3];        /* 0x005529c0 */
    char s_005529cc[8];            /* 0x005529cc */
    uint32_t d_005529d4[3];        /* 0x005529d4 */
    char s_005529e0[8];            /* 0x005529e0 */
    uint32_t d_005529e8[3];        /* 0x005529e8 */
    char s_005529f4[8];            /* 0x005529f4 */
    uint32_t d_005529fc[3];        /* 0x005529fc */
    char s_00552a08[8];            /* 0x00552a08 */
    uint32_t d_00552a10[3];        /* 0x00552a10 */
    char s_00552a1c[8];            /* 0x00552a1c */
    uint32_t d_00552a24[3];        /* 0x00552a24 */
    char s_00552a30[8];            /* 0x00552a30 */
    uint32_t d_00552a38[3];        /* 0x00552a38 */
    char s_00552a44[8];            /* 0x00552a44 */
    uint32_t d_00552a4c[3];        /* 0x00552a4c */
    char s_00552a58[8];            /* 0x00552a58 */
    uint32_t d_00552a60[3];        /* 0x00552a60 */
    char s_00552a6c[8];            /* 0x00552a6c */
    uint32_t d_00552a74[3];        /* 0x00552a74 */
    char s_00552a80[8];            /* 0x00552a80 */
    uint32_t d_00552a88[3];        /* 0x00552a88 */
    char s_00552a94[8];            /* 0x00552a94 */
    uint32_t d_00552a9c[3];        /* 0x00552a9c */
    char s_00552aa8[8];            /* 0x00552aa8 */
    uint32_t d_00552ab0[3];        /* 0x00552ab0 */
    char s_00552abc[8];            /* 0x00552abc */
    uint32_t d_00552ac4[3];        /* 0x00552ac4 */
    char s_00552ad0[8];            /* 0x00552ad0 */
    uint32_t d_00552ad8[3];        /* 0x00552ad8 */
    char s_00552ae4[8];            /* 0x00552ae4 */
    uint32_t d_00552aec[3];        /* 0x00552aec */
    char s_00552af8[8];            /* 0x00552af8 */
    uint32_t d_00552b00[3];        /* 0x00552b00 */
    char s_00552b0c[8];            /* 0x00552b0c */
    uint32_t d_00552b14[3];        /* 0x00552b14 */
    char sArkDog[8];               /* 0x00552b20 */
    uint32_t d_00552b28[3];        /* 0x00552b28 */
    char s_00552b34[8];            /* 0x00552b34 */
    uint32_t d_00552b3c[3];        /* 0x00552b3c */
    char s_00552b48[8];            /* 0x00552b48 */
    uint32_t d_00552b50[3];        /* 0x00552b50 */
    char s_00552b5c[8];            /* 0x00552b5c */
    uint32_t d_00552b64[3];        /* 0x00552b64 */
    char s_00552b70[8];            /* 0x00552b70 */
    uint32_t d_00552b78[3];        /* 0x00552b78 */
    char s_00552b84[8];            /* 0x00552b84 */
    uint32_t d_00552b8c[3];        /* 0x00552b8c */
    char s_00552b98[8];            /* 0x00552b98 */
    uint32_t d_00552ba0[3];        /* 0x00552ba0 */
    char s_00552bac[8];            /* 0x00552bac */
    uint32_t d_00552bb4[3];        /* 0x00552bb4 */
    char s_00552bc0[8];            /* 0x00552bc0 */
    uint32_t d_00552bc8[3];        /* 0x00552bc8 */
    char s_00552bd4[8];            /* 0x00552bd4 */
    uint32_t d_00552bdc[3];        /* 0x00552bdc */
    char s_00552be8[8];            /* 0x00552be8 */
    uint32_t d_00552bf0[3];        /* 0x00552bf0 */
    char s_00552bfc[8];            /* 0x00552bfc */
    uint32_t d_00552c04[3];        /* 0x00552c04 */
    char s_00552c10[8];            /* 0x00552c10 */
    uint32_t d_00552c18[3];        /* 0x00552c18 */
    char s_00552c24[8];            /* 0x00552c24 */
    uint32_t d_00552c2c[3];        /* 0x00552c2c */
    char s_00552c38[8];            /* 0x00552c38 */
    uint32_t d_00552c40[3];        /* 0x00552c40 */
    char s_00552c4c[8];            /* 0x00552c4c */
    uint32_t d_00552c54[3];        /* 0x00552c54 */
    char s_00552c60[8];            /* 0x00552c60 */
    uint32_t d_00552c68[3];        /* 0x00552c68 */
    char s_00552c74[8];            /* 0x00552c74 */
    uint32_t d_00552c7c[3];        /* 0x00552c7c */
    char s_00552c88[8];            /* 0x00552c88 */
    uint32_t d_00552c90[3];        /* 0x00552c90 */
    char s_00552c9c[8];            /* 0x00552c9c */
    uint32_t d_00552ca4[3];        /* 0x00552ca4 */
    char s_00552cb0[8];            /* 0x00552cb0 */
    uint32_t d_00552cb8[3];        /* 0x00552cb8 */
    char s_00552cc4[8];            /* 0x00552cc4 */
    uint32_t d_00552ccc[3];        /* 0x00552ccc */
    char s_00552cd8[8];            /* 0x00552cd8 */
    uint32_t d_00552ce0[3];        /* 0x00552ce0 */
    char s_00552cec[8];            /* 0x00552cec */
    uint32_t d_00552cf4[3];        /* 0x00552cf4 */
    char sUmber[8];                /* 0x00552d00 */
    uint32_t d_00552d08[3];        /* 0x00552d08 */
    char s_00552d14[8];            /* 0x00552d14 */
    uint32_t d_00552d1c[3];        /* 0x00552d1c */
    char s_00552d28[8];            /* 0x00552d28 */
    uint32_t d_00552d30[3];        /* 0x00552d30 */
    char s_00552d3c[8];            /* 0x00552d3c */
    uint32_t d_00552d44[3];        /* 0x00552d44 */
    char s_00552d50[8];            /* 0x00552d50 */
    uint32_t d_00552d58[3];        /* 0x00552d58 */
    char s_00552d64[8];            /* 0x00552d64 */
    uint32_t d_00552d6c[3];        /* 0x00552d6c */
    char s_00552d78[8];            /* 0x00552d78 */
    uint32_t d_00552d80[3];        /* 0x00552d80 */
    char s_00552d8c[8];            /* 0x00552d8c */
    uint32_t d_00552d94[3];        /* 0x00552d94 */
    char s_00552da0[8];            /* 0x00552da0 */
    uint32_t d_00552da8[3];        /* 0x00552da8 */
    char s_00552db4[8];            /* 0x00552db4 */
    uint32_t d_00552dbc[3];        /* 0x00552dbc */
    char s_00552dc8[8];            /* 0x00552dc8 */
    uint32_t d_00552dd0[3];        /* 0x00552dd0 */
    char s_00552ddc[8];            /* 0x00552ddc */
    uint32_t d_00552de4[3];        /* 0x00552de4 */
    char s_00552df0[8];            /* 0x00552df0 */
    uint32_t d_00552df8[3];        /* 0x00552df8 */
    char s_00552e04[8];            /* 0x00552e04 */
    uint32_t d_00552e0c[3];        /* 0x00552e0c */
    char s_00552e18[8];            /* 0x00552e18 */
    uint32_t d_00552e20[3];        /* 0x00552e20 */
    char s_00552e2c[8];            /* 0x00552e2c */
    uint32_t d_00552e34[3];        /* 0x00552e34 */
    char s_00552e40[8];            /* 0x00552e40 */
    uint32_t d_00552e48[3];        /* 0x00552e48 */
    char s_00552e54[8];            /* 0x00552e54 */
    uint32_t d_00552e5c[3];        /* 0x00552e5c */
    char s_00552e68[8];            /* 0x00552e68 */
    uint32_t d_00552e70[3];        /* 0x00552e70 */
    char s_00552e7c[8];            /* 0x00552e7c */
    uint32_t d_00552e84[3];        /* 0x00552e84 */
    char s_00552e90[8];            /* 0x00552e90 */
    uint32_t d_00552e98[3];        /* 0x00552e98 */
    char s_00552ea4[8];            /* 0x00552ea4 */
    uint32_t d_00552eac[3];        /* 0x00552eac */
    char s_00552eb8[8];            /* 0x00552eb8 */
    uint32_t d_00552ec0[3];        /* 0x00552ec0 */
    char s_00552ecc[8];            /* 0x00552ecc */
    uint32_t d_00552ed4[3];        /* 0x00552ed4 */
    char sAptInsano[12];           /* 0x00552ee0 */
    uint32_t d_00552eec[2];        /* 0x00552eec */
    char s_00552ef4[12];           /* 0x00552ef4 */
    uint32_t d_00552f00[2];        /* 0x00552f00 */
    char s_00552f08[12];           /* 0x00552f08 */
    uint32_t d_00552f14[2];        /* 0x00552f14 */
    char s_00552f1c[8];            /* 0x00552f1c */
    uint32_t d_00552f24[3];        /* 0x00552f24 */
    char s_00552f30[8];            /* 0x00552f30 */
    uint32_t d_00552f38[3];        /* 0x00552f38 */
    char s_00552f44[8];            /* 0x00552f44 */
    uint32_t d_00552f4c[3];        /* 0x00552f4c */
    char s_00552f58[8];            /* 0x00552f58 */
    uint32_t d_00552f60[3];        /* 0x00552f60 */
    char s_00552f6c[8];            /* 0x00552f6c */
    uint32_t d_00552f74[3];        /* 0x00552f74 */
    char s_00552f80[8];            /* 0x00552f80 */
    uint32_t d_00552f88[3];        /* 0x00552f88 */
    char s_00552f94[8];            /* 0x00552f94 */
    uint32_t d_00552f9c[3];        /* 0x00552f9c */
    char s_00552fa8[8];            /* 0x00552fa8 */
    uint32_t d_00552fb0[3];        /* 0x00552fb0 */
    char s_00552fbc[8];            /* 0x00552fbc */
    uint32_t d_00552fc4[3];        /* 0x00552fc4 */
    char s_00552fd0[8];            /* 0x00552fd0 */
    uint32_t d_00552fd8[3];        /* 0x00552fd8 */
    char s_00552fe4[8];            /* 0x00552fe4 */
    uint32_t d_00552fec[3];        /* 0x00552fec */
    char s_00552ff8[8];            /* 0x00552ff8 */
    uint32_t d_00553000[3];        /* 0x00553000 */
    char s_0055300c[8];            /* 0x0055300c */
    uint32_t d_00553014[3];        /* 0x00553014 */
    char s_00553020[12];           /* 0x00553020 */
    uint32_t d_0055302c[2];        /* 0x0055302c */
    char s_00553034[12];           /* 0x00553034 */
    uint32_t d_00553040[2];        /* 0x00553040 */
    char s_00553048[12];           /* 0x00553048 */
    uint32_t d_00553054[2];        /* 0x00553054 */
    char s_0055305c[12];           /* 0x0055305c */
    uint32_t d_00553068[2];        /* 0x00553068 */
    char s_00553070[12];           /* 0x00553070 */
    uint32_t d_0055307c[2];        /* 0x0055307c */
    char s_00553084[12];           /* 0x00553084 */
    uint32_t d_00553090[2];        /* 0x00553090 */
    char s_00553098[12];           /* 0x00553098 */
    uint32_t d_005530a4[2];        /* 0x005530a4 */
    char s_005530ac[12];           /* 0x005530ac */
    uint32_t d_005530b8[2];        /* 0x005530b8 */
    char sHeBoss[8];               /* 0x005530c0 */
    uint32_t d_005530c8[3];        /* 0x005530c8 */
    char s_005530d4[8];            /* 0x005530d4 */
    uint32_t d_005530dc[3];        /* 0x005530dc */
    char s_005530e8[8];            /* 0x005530e8 */
    uint32_t d_005530f0[3];        /* 0x005530f0 */
    char s_005530fc[8];            /* 0x005530fc */
    uint32_t d_00553104[3];        /* 0x00553104 */
    char s_00553110[8];            /* 0x00553110 */
    uint32_t d_00553118[3];        /* 0x00553118 */
    char s_00553124[8];            /* 0x00553124 */
    uint32_t d_0055312c[3];        /* 0x0055312c */
    char s_00553138[8];            /* 0x00553138 */
    uint32_t d_00553140[3];        /* 0x00553140 */
    char s_0055314c[8];            /* 0x0055314c */
    uint32_t d_00553154[3];        /* 0x00553154 */
    char s_00553160[8];            /* 0x00553160 */
    uint32_t d_00553168[3];        /* 0x00553168 */
    char s_00553174[8];            /* 0x00553174 */
    uint32_t d_0055317c[3];        /* 0x0055317c */
    char s_00553188[8];            /* 0x00553188 */
    uint32_t d_00553190[3];        /* 0x00553190 */
    char s_0055319c[8];            /* 0x0055319c */
    uint32_t d_005531a4[3];        /* 0x005531a4 */
    char s_005531b0[8];            /* 0x005531b0 */
    uint32_t d_005531b8[3];        /* 0x005531b8 */
    char s_005531c4[8];            /* 0x005531c4 */
    uint32_t d_005531cc[3];        /* 0x005531cc */
    char s_005531d8[8];            /* 0x005531d8 */
    uint32_t d_005531e0[3];        /* 0x005531e0 */
    char s_005531ec[8];            /* 0x005531ec */
    uint32_t d_005531f4[3];        /* 0x005531f4 */
    char s_00553200[8];            /* 0x00553200 */
    uint32_t d_00553208[3];        /* 0x00553208 */
    char s_00553214[8];            /* 0x00553214 */
    uint32_t d_0055321c[3];        /* 0x0055321c */
    char s_00553228[8];            /* 0x00553228 */
    uint32_t d_00553230[3];        /* 0x00553230 */
    char s_0055323c[8];            /* 0x0055323c */
    uint32_t d_00553244[3];        /* 0x00553244 */
    char s_00553250[8];            /* 0x00553250 */
    uint32_t d_00553258[3];        /* 0x00553258 */
    char s_00553264[8];            /* 0x00553264 */
    uint32_t d_0055326c[3];        /* 0x0055326c */
    char s_00553278[8];            /* 0x00553278 */
    uint32_t d_00553280[3];        /* 0x00553280 */
    char s_0055328c[8];            /* 0x0055328c */
    uint32_t d_00553294[3];        /* 0x00553294 */
    char s_005532a0[12];           /* 0x005532a0 */
    uint32_t d_005532ac[2];        /* 0x005532ac */
    char s_005532b4[12];           /* 0x005532b4 */
    uint32_t d_005532c0[2];        /* 0x005532c0 */
    char s_005532c8[12];           /* 0x005532c8 */
    uint32_t d_005532d4[2];        /* 0x005532d4 */
    char s_005532dc[8];            /* 0x005532dc */
    uint32_t d_005532e4[3];        /* 0x005532e4 */
    char s_005532f0[8];            /* 0x005532f0 */
    uint32_t d_005532f8[3];        /* 0x005532f8 */
    char s_00553304[8];            /* 0x00553304 */
    uint32_t d_0055330c[3];        /* 0x0055330c */
    char s_00553318[8];            /* 0x00553318 */
    uint32_t d_00553320[3];        /* 0x00553320 */
    char s_0055332c[8];            /* 0x0055332c */
    uint32_t d_00553334[3];        /* 0x00553334 */
    char s_00553340[8];            /* 0x00553340 */
    uint32_t d_00553348[3];        /* 0x00553348 */
    char s_00553354[8];            /* 0x00553354 */
    uint32_t d_0055335c[3];        /* 0x0055335c */
    char s_00553368[8];            /* 0x00553368 */
    uint32_t d_00553370[3];        /* 0x00553370 */
    char s_0055337c[8];            /* 0x0055337c */
    uint32_t d_00553384[3];        /* 0x00553384 */
    char s_00553390[8];            /* 0x00553390 */
    uint32_t d_00553398[3];        /* 0x00553398 */
    char s_005533a4[8];            /* 0x005533a4 */
    uint32_t d_005533ac[3];        /* 0x005533ac */
    char s_005533b8[8];            /* 0x005533b8 */
    uint32_t d_005533c0[3];        /* 0x005533c0 */
    char s_005533cc[8];            /* 0x005533cc */
    uint32_t d_005533d4[3];        /* 0x005533d4 */
    char s_005533e0[12];           /* 0x005533e0 */
    uint32_t d_005533ec[2];        /* 0x005533ec */
    char s_005533f4[12];           /* 0x005533f4 */
    uint32_t d_00553400[2];        /* 0x00553400 */
    char s_00553408[12];           /* 0x00553408 */
    uint32_t d_00553414[2];        /* 0x00553414 */
    char s_0055341c[12];           /* 0x0055341c */
    uint32_t d_00553428[2];        /* 0x00553428 */
    char s_00553430[12];           /* 0x00553430 */
    uint32_t d_0055343c[2];        /* 0x0055343c */
    char s_00553444[12];           /* 0x00553444 */
    uint32_t d_00553450[2];        /* 0x00553450 */
    char s_00553458[12];           /* 0x00553458 */
    uint32_t d_00553464[2];        /* 0x00553464 */
    char s_0055346c[12];           /* 0x0055346c */
    uint32_t d_00553478[2];        /* 0x00553478 */
    char s_00553480[12];           /* 0x00553480 */
    uint32_t d_0055348c[2];        /* 0x0055348c */
    char s_00553494[12];           /* 0x00553494 */
    uint32_t d_005534a0[2];        /* 0x005534a0 */
    char s_005534a8[12];           /* 0x005534a8 */
    uint32_t d_005534b4[2];        /* 0x005534b4 */
    char s_005534bc[8];            /* 0x005534bc */
    uint32_t d_005534c4[3];        /* 0x005534c4 */
    char s_005534d0[8];            /* 0x005534d0 */
    uint32_t d_005534d8[3];        /* 0x005534d8 */
    char s_005534e4[8];            /* 0x005534e4 */
    uint32_t d_005534ec[3];        /* 0x005534ec */
    char s_005534f8[8];            /* 0x005534f8 */
    uint32_t d_00553500[3];        /* 0x00553500 */
    char s_0055350c[8];            /* 0x0055350c */
    uint32_t d_00553514[3];        /* 0x00553514 */
    char s_00553520[8];            /* 0x00553520 */
    uint32_t d_00553528[3];        /* 0x00553528 */
    char s_00553534[8];            /* 0x00553534 */
    uint32_t d_0055353c[3];        /* 0x0055353c */
    char s_00553548[8];            /* 0x00553548 */
    uint32_t d_00553550[3];        /* 0x00553550 */
    char s_0055355c[8];            /* 0x0055355c */
    uint32_t d_00553564[3];        /* 0x00553564 */
    char s_00553570[8];            /* 0x00553570 */
    uint32_t d_00553578[3];        /* 0x00553578 */
    char s_00553584[8];            /* 0x00553584 */
    uint32_t d_0055358c[3];        /* 0x0055358c */
    char s_00553598[8];            /* 0x00553598 */
    uint32_t d_005535a0[3];        /* 0x005535a0 */
    char s_005535ac[8];            /* 0x005535ac */
    uint32_t d_005535b4[3];        /* 0x005535b4 */
    char s_005535c0[12];           /* 0x005535c0 */
    uint32_t d_005535cc[2];        /* 0x005535cc */
    char s_005535d4[12];           /* 0x005535d4 */
    uint32_t d_005535e0[2];        /* 0x005535e0 */
    char s_005535e8[12];           /* 0x005535e8 */
    uint32_t d_005535f4[2];        /* 0x005535f4 */
    char s_005535fc[12];           /* 0x005535fc */
    uint32_t d_00553608[2];        /* 0x00553608 */
    char s_00553610[12];           /* 0x00553610 */
    uint32_t d_0055361c[2];        /* 0x0055361c */
    char s_00553624[12];           /* 0x00553624 */
    uint32_t d_00553630[2];        /* 0x00553630 */
    char s_00553638[12];           /* 0x00553638 */
    uint32_t d_00553644[2];        /* 0x00553644 */
    char s_0055364c[12];           /* 0x0055364c */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_bmxSaveRecords, member) == (addr) - 0x00551fd0, #member)
AT(d_00551fd8, 0x00551fd8);
AT(sIracleBoy, 0x00551fe0);
AT(d_00551fec, 0x00551fec);
AT(s_00551ff4, 0x00551ff4);
AT(d_00552000, 0x00552000);
AT(s_00552008, 0x00552008);
AT(d_00552014, 0x00552014);
AT(sYTor, 0x0055201c);
AT(d_00552024, 0x00552024);
AT(s_00552030, 0x00552030);
AT(d_00552038, 0x00552038);
AT(s_00552044, 0x00552044);
AT(d_0055204c, 0x0055204c);
AT(s_00552058, 0x00552058);
AT(pad_00552060, 0x00552060);
AT(d_00552064, 0x00552064);
AT(s_0055206c, 0x0055206c);
AT(pad_00552074, 0x00552074);
AT(d_00552078, 0x00552078);
AT(s_00552080, 0x00552080);
AT(pad_00552088, 0x00552088);
AT(d_0055208c, 0x0055208c);
AT(s_00552094, 0x00552094);
AT(pad_0055209c, 0x0055209c);
AT(d_005520a0, 0x005520a0);
AT(s_005520a8, 0x005520a8);
AT(pad_005520b0, 0x005520b0);
AT(d_005520b4, 0x005520b4);
AT(s_005520bc, 0x005520bc);
AT(pad_005520c4, 0x005520c4);
AT(d_005520c8, 0x005520c8);
AT(s_005520d0, 0x005520d0);
AT(d_005520d8, 0x005520d8);
AT(s_005520e4, 0x005520e4);
AT(pad_005520ec, 0x005520ec);
AT(d_005520f0, 0x005520f0);
AT(s_005520f8, 0x005520f8);
AT(d_00552100, 0x00552100);
AT(s_0055210c, 0x0055210c);
AT(d_00552114, 0x00552114);
AT(s_00552120, 0x00552120);
AT(d_0055212c, 0x0055212c);
AT(s_00552134, 0x00552134);
AT(d_00552140, 0x00552140);
AT(s_00552148, 0x00552148);
AT(d_00552154, 0x00552154);
AT(s_0055215c, 0x0055215c);
AT(d_00552168, 0x00552168);
AT(s_00552170, 0x00552170);
AT(d_0055217c, 0x0055217c);
AT(s_00552184, 0x00552184);
AT(d_00552190, 0x00552190);
AT(s_00552198, 0x00552198);
AT(d_005521a4, 0x005521a4);
AT(s_005521ac, 0x005521ac);
AT(d_005521b8, 0x005521b8);
AT(sOrCal, 0x005521c0);
AT(d_005521c8, 0x005521c8);
AT(s_005521d4, 0x005521d4);
AT(d_005521dc, 0x005521dc);
AT(s_005521e8, 0x005521e8);
AT(d_005521f0, 0x005521f0);
AT(s_005521fc, 0x005521fc);
AT(d_00552204, 0x00552204);
AT(s_00552210, 0x00552210);
AT(d_00552218, 0x00552218);
AT(s_00552224, 0x00552224);
AT(d_0055222c, 0x0055222c);
AT(s_00552238, 0x00552238);
AT(d_00552240, 0x00552240);
AT(s_0055224c, 0x0055224c);
AT(d_00552254, 0x00552254);
AT(s_00552260, 0x00552260);
AT(d_00552268, 0x00552268);
AT(s_00552274, 0x00552274);
AT(d_0055227c, 0x0055227c);
AT(s_00552288, 0x00552288);
AT(d_00552290, 0x00552290);
AT(s_0055229c, 0x0055229c);
AT(d_005522a4, 0x005522a4);
AT(s_005522b0, 0x005522b0);
AT(d_005522b8, 0x005522b8);
AT(s_005522c4, 0x005522c4);
AT(d_005522cc, 0x005522cc);
AT(s_005522d8, 0x005522d8);
AT(d_005522e0, 0x005522e0);
AT(s_005522ec, 0x005522ec);
AT(d_005522f4, 0x005522f4);
AT(s_00552300, 0x00552300);
AT(d_00552308, 0x00552308);
AT(s_00552314, 0x00552314);
AT(d_0055231c, 0x0055231c);
AT(s_00552328, 0x00552328);
AT(d_00552330, 0x00552330);
AT(s_0055233c, 0x0055233c);
AT(d_00552344, 0x00552344);
AT(s_00552350, 0x00552350);
AT(d_00552358, 0x00552358);
AT(s_00552364, 0x00552364);
AT(d_0055236c, 0x0055236c);
AT(s_00552378, 0x00552378);
AT(d_00552380, 0x00552380);
AT(s_0055238c, 0x0055238c);
AT(d_00552394, 0x00552394);
AT(sArpig, 0x005523a0);
AT(d_005523a8, 0x005523a8);
AT(s_005523b4, 0x005523b4);
AT(d_005523bc, 0x005523bc);
AT(s_005523c8, 0x005523c8);
AT(d_005523d0, 0x005523d0);
AT(sEbby, 0x005523dc);
AT(d_005523e4, 0x005523e4);
AT(s_005523f0, 0x005523f0);
AT(d_005523f8, 0x005523f8);
AT(s_00552404, 0x00552404);
AT(d_0055240c, 0x0055240c);
AT(s_00552418, 0x00552418);
AT(d_00552420, 0x00552420);
AT(s_0055242c, 0x0055242c);
AT(d_00552434, 0x00552434);
AT(s_00552440, 0x00552440);
AT(d_00552448, 0x00552448);
AT(s_00552454, 0x00552454);
AT(d_0055245c, 0x0055245c);
AT(s_00552468, 0x00552468);
AT(d_00552470, 0x00552470);
AT(s_0055247c, 0x0055247c);
AT(d_00552484, 0x00552484);
AT(s_00552490, 0x00552490);
AT(d_00552498, 0x00552498);
AT(s_005524a4, 0x005524a4);
AT(d_005524ac, 0x005524ac);
AT(s_005524b8, 0x005524b8);
AT(d_005524c0, 0x005524c0);
AT(s_005524cc, 0x005524cc);
AT(d_005524d4, 0x005524d4);
AT(s_005524e0, 0x005524e0);
AT(d_005524e8, 0x005524e8);
AT(s_005524f4, 0x005524f4);
AT(d_005524fc, 0x005524fc);
AT(s_00552508, 0x00552508);
AT(d_00552510, 0x00552510);
AT(s_0055251c, 0x0055251c);
AT(d_00552524, 0x00552524);
AT(s_00552530, 0x00552530);
AT(d_00552538, 0x00552538);
AT(s_00552544, 0x00552544);
AT(d_0055254c, 0x0055254c);
AT(s_00552558, 0x00552558);
AT(d_00552560, 0x00552560);
AT(s_0055256c, 0x0055256c);
AT(d_00552574, 0x00552574);
AT(sBreed, 0x00552580);
AT(d_00552588, 0x00552588);
AT(s_00552594, 0x00552594);
AT(d_0055259c, 0x0055259c);
AT(s_005525a8, 0x005525a8);
AT(d_005525b0, 0x005525b0);
AT(s_005525bc, 0x005525bc);
AT(d_005525c4, 0x005525c4);
AT(s_005525d0, 0x005525d0);
AT(d_005525d8, 0x005525d8);
AT(s_005525e4, 0x005525e4);
AT(d_005525ec, 0x005525ec);
AT(s_005525f8, 0x005525f8);
AT(d_00552600, 0x00552600);
AT(s_0055260c, 0x0055260c);
AT(d_00552614, 0x00552614);
AT(s_00552620, 0x00552620);
AT(d_00552628, 0x00552628);
AT(s_00552634, 0x00552634);
AT(d_0055263c, 0x0055263c);
AT(s_00552648, 0x00552648);
AT(d_00552650, 0x00552650);
AT(s_0055265c, 0x0055265c);
AT(d_00552664, 0x00552664);
AT(s_00552670, 0x00552670);
AT(d_00552678, 0x00552678);
AT(s_00552684, 0x00552684);
AT(d_0055268c, 0x0055268c);
AT(s_00552698, 0x00552698);
AT(d_005526a0, 0x005526a0);
AT(s_005526ac, 0x005526ac);
AT(d_005526b4, 0x005526b4);
AT(s_005526c0, 0x005526c0);
AT(d_005526c8, 0x005526c8);
AT(s_005526d4, 0x005526d4);
AT(d_005526dc, 0x005526dc);
AT(s_005526e8, 0x005526e8);
AT(d_005526f0, 0x005526f0);
AT(s_005526fc, 0x005526fc);
AT(d_00552704, 0x00552704);
AT(s_00552710, 0x00552710);
AT(d_00552718, 0x00552718);
AT(s_00552724, 0x00552724);
AT(d_0055272c, 0x0055272c);
AT(s_00552738, 0x00552738);
AT(d_00552740, 0x00552740);
AT(s_0055274c, 0x0055274c);
AT(d_00552754, 0x00552754);
AT(sAGY, 0x00552760);
AT(d_00552768, 0x00552768);
AT(s_00552774, 0x00552774);
AT(d_0055277c, 0x0055277c);
AT(s_00552788, 0x00552788);
AT(d_00552790, 0x00552790);
AT(s_0055279c, 0x0055279c);
AT(d_005527a4, 0x005527a4);
AT(s_005527b0, 0x005527b0);
AT(d_005527b8, 0x005527b8);
AT(s_005527c4, 0x005527c4);
AT(d_005527cc, 0x005527cc);
AT(s_005527d8, 0x005527d8);
AT(d_005527e0, 0x005527e0);
AT(s_005527ec, 0x005527ec);
AT(d_005527f4, 0x005527f4);
AT(s_00552800, 0x00552800);
AT(d_00552808, 0x00552808);
AT(s_00552814, 0x00552814);
AT(d_0055281c, 0x0055281c);
AT(s_00552828, 0x00552828);
AT(d_00552830, 0x00552830);
AT(s_0055283c, 0x0055283c);
AT(d_00552844, 0x00552844);
AT(s_00552850, 0x00552850);
AT(d_00552858, 0x00552858);
AT(s_00552864, 0x00552864);
AT(d_0055286c, 0x0055286c);
AT(s_00552878, 0x00552878);
AT(d_00552880, 0x00552880);
AT(s_0055288c, 0x0055288c);
AT(d_00552894, 0x00552894);
AT(s_005528a0, 0x005528a0);
AT(d_005528a8, 0x005528a8);
AT(s_005528b4, 0x005528b4);
AT(d_005528bc, 0x005528bc);
AT(s_005528c8, 0x005528c8);
AT(d_005528d0, 0x005528d0);
AT(s_005528dc, 0x005528dc);
AT(d_005528e4, 0x005528e4);
AT(s_005528f0, 0x005528f0);
AT(d_005528f8, 0x005528f8);
AT(s_00552904, 0x00552904);
AT(d_0055290c, 0x0055290c);
AT(s_00552918, 0x00552918);
AT(d_00552920, 0x00552920);
AT(s_0055292c, 0x0055292c);
AT(d_00552934, 0x00552934);
AT(sUrbo, 0x00552940);
AT(d_00552948, 0x00552948);
AT(s_00552954, 0x00552954);
AT(d_0055295c, 0x0055295c);
AT(s_00552968, 0x00552968);
AT(d_00552970, 0x00552970);
AT(s_0055297c, 0x0055297c);
AT(d_00552984, 0x00552984);
AT(s_00552990, 0x00552990);
AT(d_00552998, 0x00552998);
AT(s_005529a4, 0x005529a4);
AT(d_005529ac, 0x005529ac);
AT(s_005529b8, 0x005529b8);
AT(d_005529c0, 0x005529c0);
AT(s_005529cc, 0x005529cc);
AT(d_005529d4, 0x005529d4);
AT(s_005529e0, 0x005529e0);
AT(d_005529e8, 0x005529e8);
AT(s_005529f4, 0x005529f4);
AT(d_005529fc, 0x005529fc);
AT(s_00552a08, 0x00552a08);
AT(d_00552a10, 0x00552a10);
AT(s_00552a1c, 0x00552a1c);
AT(d_00552a24, 0x00552a24);
AT(s_00552a30, 0x00552a30);
AT(d_00552a38, 0x00552a38);
AT(s_00552a44, 0x00552a44);
AT(d_00552a4c, 0x00552a4c);
AT(s_00552a58, 0x00552a58);
AT(d_00552a60, 0x00552a60);
AT(s_00552a6c, 0x00552a6c);
AT(d_00552a74, 0x00552a74);
AT(s_00552a80, 0x00552a80);
AT(d_00552a88, 0x00552a88);
AT(s_00552a94, 0x00552a94);
AT(d_00552a9c, 0x00552a9c);
AT(s_00552aa8, 0x00552aa8);
AT(d_00552ab0, 0x00552ab0);
AT(s_00552abc, 0x00552abc);
AT(d_00552ac4, 0x00552ac4);
AT(s_00552ad0, 0x00552ad0);
AT(d_00552ad8, 0x00552ad8);
AT(s_00552ae4, 0x00552ae4);
AT(d_00552aec, 0x00552aec);
AT(s_00552af8, 0x00552af8);
AT(d_00552b00, 0x00552b00);
AT(s_00552b0c, 0x00552b0c);
AT(d_00552b14, 0x00552b14);
AT(sArkDog, 0x00552b20);
AT(d_00552b28, 0x00552b28);
AT(s_00552b34, 0x00552b34);
AT(d_00552b3c, 0x00552b3c);
AT(s_00552b48, 0x00552b48);
AT(d_00552b50, 0x00552b50);
AT(s_00552b5c, 0x00552b5c);
AT(d_00552b64, 0x00552b64);
AT(s_00552b70, 0x00552b70);
AT(d_00552b78, 0x00552b78);
AT(s_00552b84, 0x00552b84);
AT(d_00552b8c, 0x00552b8c);
AT(s_00552b98, 0x00552b98);
AT(d_00552ba0, 0x00552ba0);
AT(s_00552bac, 0x00552bac);
AT(d_00552bb4, 0x00552bb4);
AT(s_00552bc0, 0x00552bc0);
AT(d_00552bc8, 0x00552bc8);
AT(s_00552bd4, 0x00552bd4);
AT(d_00552bdc, 0x00552bdc);
AT(s_00552be8, 0x00552be8);
AT(d_00552bf0, 0x00552bf0);
AT(s_00552bfc, 0x00552bfc);
AT(d_00552c04, 0x00552c04);
AT(s_00552c10, 0x00552c10);
AT(d_00552c18, 0x00552c18);
AT(s_00552c24, 0x00552c24);
AT(d_00552c2c, 0x00552c2c);
AT(s_00552c38, 0x00552c38);
AT(d_00552c40, 0x00552c40);
AT(s_00552c4c, 0x00552c4c);
AT(d_00552c54, 0x00552c54);
AT(s_00552c60, 0x00552c60);
AT(d_00552c68, 0x00552c68);
AT(s_00552c74, 0x00552c74);
AT(d_00552c7c, 0x00552c7c);
AT(s_00552c88, 0x00552c88);
AT(d_00552c90, 0x00552c90);
AT(s_00552c9c, 0x00552c9c);
AT(d_00552ca4, 0x00552ca4);
AT(s_00552cb0, 0x00552cb0);
AT(d_00552cb8, 0x00552cb8);
AT(s_00552cc4, 0x00552cc4);
AT(d_00552ccc, 0x00552ccc);
AT(s_00552cd8, 0x00552cd8);
AT(d_00552ce0, 0x00552ce0);
AT(s_00552cec, 0x00552cec);
AT(d_00552cf4, 0x00552cf4);
AT(sUmber, 0x00552d00);
AT(d_00552d08, 0x00552d08);
AT(s_00552d14, 0x00552d14);
AT(d_00552d1c, 0x00552d1c);
AT(s_00552d28, 0x00552d28);
AT(d_00552d30, 0x00552d30);
AT(s_00552d3c, 0x00552d3c);
AT(d_00552d44, 0x00552d44);
AT(s_00552d50, 0x00552d50);
AT(d_00552d58, 0x00552d58);
AT(s_00552d64, 0x00552d64);
AT(d_00552d6c, 0x00552d6c);
AT(s_00552d78, 0x00552d78);
AT(d_00552d80, 0x00552d80);
AT(s_00552d8c, 0x00552d8c);
AT(d_00552d94, 0x00552d94);
AT(s_00552da0, 0x00552da0);
AT(d_00552da8, 0x00552da8);
AT(s_00552db4, 0x00552db4);
AT(d_00552dbc, 0x00552dbc);
AT(s_00552dc8, 0x00552dc8);
AT(d_00552dd0, 0x00552dd0);
AT(s_00552ddc, 0x00552ddc);
AT(d_00552de4, 0x00552de4);
AT(s_00552df0, 0x00552df0);
AT(d_00552df8, 0x00552df8);
AT(s_00552e04, 0x00552e04);
AT(d_00552e0c, 0x00552e0c);
AT(s_00552e18, 0x00552e18);
AT(d_00552e20, 0x00552e20);
AT(s_00552e2c, 0x00552e2c);
AT(d_00552e34, 0x00552e34);
AT(s_00552e40, 0x00552e40);
AT(d_00552e48, 0x00552e48);
AT(s_00552e54, 0x00552e54);
AT(d_00552e5c, 0x00552e5c);
AT(s_00552e68, 0x00552e68);
AT(d_00552e70, 0x00552e70);
AT(s_00552e7c, 0x00552e7c);
AT(d_00552e84, 0x00552e84);
AT(s_00552e90, 0x00552e90);
AT(d_00552e98, 0x00552e98);
AT(s_00552ea4, 0x00552ea4);
AT(d_00552eac, 0x00552eac);
AT(s_00552eb8, 0x00552eb8);
AT(d_00552ec0, 0x00552ec0);
AT(s_00552ecc, 0x00552ecc);
AT(d_00552ed4, 0x00552ed4);
AT(sAptInsano, 0x00552ee0);
AT(d_00552eec, 0x00552eec);
AT(s_00552ef4, 0x00552ef4);
AT(d_00552f00, 0x00552f00);
AT(s_00552f08, 0x00552f08);
AT(d_00552f14, 0x00552f14);
AT(s_00552f1c, 0x00552f1c);
AT(d_00552f24, 0x00552f24);
AT(s_00552f30, 0x00552f30);
AT(d_00552f38, 0x00552f38);
AT(s_00552f44, 0x00552f44);
AT(d_00552f4c, 0x00552f4c);
AT(s_00552f58, 0x00552f58);
AT(d_00552f60, 0x00552f60);
AT(s_00552f6c, 0x00552f6c);
AT(d_00552f74, 0x00552f74);
AT(s_00552f80, 0x00552f80);
AT(d_00552f88, 0x00552f88);
AT(s_00552f94, 0x00552f94);
AT(d_00552f9c, 0x00552f9c);
AT(s_00552fa8, 0x00552fa8);
AT(d_00552fb0, 0x00552fb0);
AT(s_00552fbc, 0x00552fbc);
AT(d_00552fc4, 0x00552fc4);
AT(s_00552fd0, 0x00552fd0);
AT(d_00552fd8, 0x00552fd8);
AT(s_00552fe4, 0x00552fe4);
AT(d_00552fec, 0x00552fec);
AT(s_00552ff8, 0x00552ff8);
AT(d_00553000, 0x00553000);
AT(s_0055300c, 0x0055300c);
AT(d_00553014, 0x00553014);
AT(s_00553020, 0x00553020);
AT(d_0055302c, 0x0055302c);
AT(s_00553034, 0x00553034);
AT(d_00553040, 0x00553040);
AT(s_00553048, 0x00553048);
AT(d_00553054, 0x00553054);
AT(s_0055305c, 0x0055305c);
AT(d_00553068, 0x00553068);
AT(s_00553070, 0x00553070);
AT(d_0055307c, 0x0055307c);
AT(s_00553084, 0x00553084);
AT(d_00553090, 0x00553090);
AT(s_00553098, 0x00553098);
AT(d_005530a4, 0x005530a4);
AT(s_005530ac, 0x005530ac);
AT(d_005530b8, 0x005530b8);
AT(sHeBoss, 0x005530c0);
AT(d_005530c8, 0x005530c8);
AT(s_005530d4, 0x005530d4);
AT(d_005530dc, 0x005530dc);
AT(s_005530e8, 0x005530e8);
AT(d_005530f0, 0x005530f0);
AT(s_005530fc, 0x005530fc);
AT(d_00553104, 0x00553104);
AT(s_00553110, 0x00553110);
AT(d_00553118, 0x00553118);
AT(s_00553124, 0x00553124);
AT(d_0055312c, 0x0055312c);
AT(s_00553138, 0x00553138);
AT(d_00553140, 0x00553140);
AT(s_0055314c, 0x0055314c);
AT(d_00553154, 0x00553154);
AT(s_00553160, 0x00553160);
AT(d_00553168, 0x00553168);
AT(s_00553174, 0x00553174);
AT(d_0055317c, 0x0055317c);
AT(s_00553188, 0x00553188);
AT(d_00553190, 0x00553190);
AT(s_0055319c, 0x0055319c);
AT(d_005531a4, 0x005531a4);
AT(s_005531b0, 0x005531b0);
AT(d_005531b8, 0x005531b8);
AT(s_005531c4, 0x005531c4);
AT(d_005531cc, 0x005531cc);
AT(s_005531d8, 0x005531d8);
AT(d_005531e0, 0x005531e0);
AT(s_005531ec, 0x005531ec);
AT(d_005531f4, 0x005531f4);
AT(s_00553200, 0x00553200);
AT(d_00553208, 0x00553208);
AT(s_00553214, 0x00553214);
AT(d_0055321c, 0x0055321c);
AT(s_00553228, 0x00553228);
AT(d_00553230, 0x00553230);
AT(s_0055323c, 0x0055323c);
AT(d_00553244, 0x00553244);
AT(s_00553250, 0x00553250);
AT(d_00553258, 0x00553258);
AT(s_00553264, 0x00553264);
AT(d_0055326c, 0x0055326c);
AT(s_00553278, 0x00553278);
AT(d_00553280, 0x00553280);
AT(s_0055328c, 0x0055328c);
AT(d_00553294, 0x00553294);
AT(s_005532a0, 0x005532a0);
AT(d_005532ac, 0x005532ac);
AT(s_005532b4, 0x005532b4);
AT(d_005532c0, 0x005532c0);
AT(s_005532c8, 0x005532c8);
AT(d_005532d4, 0x005532d4);
AT(s_005532dc, 0x005532dc);
AT(d_005532e4, 0x005532e4);
AT(s_005532f0, 0x005532f0);
AT(d_005532f8, 0x005532f8);
AT(s_00553304, 0x00553304);
AT(d_0055330c, 0x0055330c);
AT(s_00553318, 0x00553318);
AT(d_00553320, 0x00553320);
AT(s_0055332c, 0x0055332c);
AT(d_00553334, 0x00553334);
AT(s_00553340, 0x00553340);
AT(d_00553348, 0x00553348);
AT(s_00553354, 0x00553354);
AT(d_0055335c, 0x0055335c);
AT(s_00553368, 0x00553368);
AT(d_00553370, 0x00553370);
AT(s_0055337c, 0x0055337c);
AT(d_00553384, 0x00553384);
AT(s_00553390, 0x00553390);
AT(d_00553398, 0x00553398);
AT(s_005533a4, 0x005533a4);
AT(d_005533ac, 0x005533ac);
AT(s_005533b8, 0x005533b8);
AT(d_005533c0, 0x005533c0);
AT(s_005533cc, 0x005533cc);
AT(d_005533d4, 0x005533d4);
AT(s_005533e0, 0x005533e0);
AT(d_005533ec, 0x005533ec);
AT(s_005533f4, 0x005533f4);
AT(d_00553400, 0x00553400);
AT(s_00553408, 0x00553408);
AT(d_00553414, 0x00553414);
AT(s_0055341c, 0x0055341c);
AT(d_00553428, 0x00553428);
AT(s_00553430, 0x00553430);
AT(d_0055343c, 0x0055343c);
AT(s_00553444, 0x00553444);
AT(d_00553450, 0x00553450);
AT(s_00553458, 0x00553458);
AT(d_00553464, 0x00553464);
AT(s_0055346c, 0x0055346c);
AT(d_00553478, 0x00553478);
AT(s_00553480, 0x00553480);
AT(d_0055348c, 0x0055348c);
AT(s_00553494, 0x00553494);
AT(d_005534a0, 0x005534a0);
AT(s_005534a8, 0x005534a8);
AT(d_005534b4, 0x005534b4);
AT(s_005534bc, 0x005534bc);
AT(d_005534c4, 0x005534c4);
AT(s_005534d0, 0x005534d0);
AT(d_005534d8, 0x005534d8);
AT(s_005534e4, 0x005534e4);
AT(d_005534ec, 0x005534ec);
AT(s_005534f8, 0x005534f8);
AT(d_00553500, 0x00553500);
AT(s_0055350c, 0x0055350c);
AT(d_00553514, 0x00553514);
AT(s_00553520, 0x00553520);
AT(d_00553528, 0x00553528);
AT(s_00553534, 0x00553534);
AT(d_0055353c, 0x0055353c);
AT(s_00553548, 0x00553548);
AT(d_00553550, 0x00553550);
AT(s_0055355c, 0x0055355c);
AT(d_00553564, 0x00553564);
AT(s_00553570, 0x00553570);
AT(d_00553578, 0x00553578);
AT(s_00553584, 0x00553584);
AT(d_0055358c, 0x0055358c);
AT(s_00553598, 0x00553598);
AT(d_005535a0, 0x005535a0);
AT(s_005535ac, 0x005535ac);
AT(d_005535b4, 0x005535b4);
AT(s_005535c0, 0x005535c0);
AT(d_005535cc, 0x005535cc);
AT(s_005535d4, 0x005535d4);
AT(d_005535e0, 0x005535e0);
AT(s_005535e8, 0x005535e8);
AT(d_005535f4, 0x005535f4);
AT(s_005535fc, 0x005535fc);
AT(d_00553608, 0x00553608);
AT(s_00553610, 0x00553610);
AT(d_0055361c, 0x0055361c);
AT(s_00553624, 0x00553624);
AT(d_00553630, 0x00553630);
AT(s_00553638, 0x00553638);
AT(d_00553644, 0x00553644);
AT(s_0055364c, 0x0055364c);
#undef AT
_Static_assert(offsetof(struct BmxGroup_bmxSaveRecords, s_0055364c) +
                       sizeof(((struct BmxGroup_bmxSaveRecords *)0)->s_0055364c) ==
                   0x1688,
               "bmxSaveRecords");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00553658..0x00553e84.
 */
struct BmxGroup_gScoreRecordSlotGroup {
    uint8_t lead_[8];             /* nothing: the members' offsets keep their original alignment */
    OvlBytes16 gScoreRecordSlot;  /* 0x00553658, also gRiderRiderStats */
    float d_00553668[11];         /* 0x00553668 */
    char s33s33s33s[14];          /* 0x00553694 */
    unsigned char d_005536a2[2];  /* 0x005536a2 */
    char s33s33s[10];             /* 0x005536a4 */
    unsigned char d_005536ae[34]; /* 0x005536ae */
    char s_005536d0[10];          /* 0x005536d0 */
    unsigned char d_005536da[6];  /* 0x005536da */
    char s33s[6];                 /* 0x005536e0 */
    unsigned char d_005536e6[6];  /* 0x005536e6 */
    char s_005536ec[10];          /* 0x005536ec */
    unsigned char d_005536f6[34]; /* 0x005536f6 */
    char s_00553718[6];           /* 0x00553718 */
    unsigned char d_0055371e[50]; /* 0x0055371e */
    char s_00553750[6];           /* 0x00553750 */
    unsigned char d_00553756[94]; /* 0x00553756 */
    float gRiderBikeStats[35];    /* 0x005537b4 */
    struct {
        uint8_t count;
        uint8_t dirs[3];
        uint8_t anim;
        uint8_t pose;
    } gRiderSignatureTricks[12][4];   /* 0x00553840; the type is local to bmx/com/action/rider.c */
    unsigned char pad_00553960[1316]; /* 0x00553960 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gScoreRecordSlotGroup, member) == (addr) - 0x00553650, #member)
AT(gScoreRecordSlot, 0x00553658);
AT(d_00553668, 0x00553668);
AT(s33s33s33s, 0x00553694);
AT(d_005536a2, 0x005536a2);
AT(s33s33s, 0x005536a4);
AT(d_005536ae, 0x005536ae);
AT(s_005536d0, 0x005536d0);
AT(d_005536da, 0x005536da);
AT(s33s, 0x005536e0);
AT(d_005536e6, 0x005536e6);
AT(s_005536ec, 0x005536ec);
AT(d_005536f6, 0x005536f6);
AT(s_00553718, 0x00553718);
AT(d_0055371e, 0x0055371e);
AT(s_00553750, 0x00553750);
AT(d_00553756, 0x00553756);
AT(gRiderBikeStats, 0x005537b4);
AT(gRiderSignatureTricks, 0x00553840);
AT(pad_00553960, 0x00553960);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gScoreRecordSlotGroup, pad_00553960) +
                       sizeof(((struct BmxGroup_gScoreRecordSlotGroup *)0)->pad_00553960) ==
                   0x834,
               "gScoreRecordSlotGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00553ee0..0x005545f8.
 */
struct BmxGroup_gContrlerBindingsGroup {
    ContrlerBindings gContrlerBindings; /* 0x00553ee0 */
    unsigned char pad_005544c8[304];    /* 0x005544c8 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gContrlerBindingsGroup, member) == (addr) - 0x00553ee0, #member)
AT(gContrlerBindings, 0x00553ee0);
AT(pad_005544c8, 0x005544c8);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gContrlerBindingsGroup, pad_005544c8) +
                       sizeof(((struct BmxGroup_gContrlerBindingsGroup *)0)->pad_005544c8) ==
                   0x718,
               "gContrlerBindingsGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00554724..0x00554738.
 */
struct BmxGroup_gFxBurstBlockGroup {
    uint8_t lead_[4];             /* nothing: the members' offsets keep their original alignment */
    uint8_t gFxBurstBlock;        /* 0x00554724 */
    unsigned char d_00554725[19]; /* 0x00554725 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gFxBurstBlockGroup, member) == (addr) - 0x00554720, #member)
AT(gFxBurstBlock, 0x00554724);
AT(d_00554725, 0x00554725);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gFxBurstBlockGroup, d_00554725) +
                       sizeof(((struct BmxGroup_gFxBurstBlockGroup *)0)->d_00554725) ==
                   0x18,
               "gFxBurstBlockGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00554738..0x00554798.
 */
struct BmxGroup_gFxPuffBlockGroup {
    uint8_t lead_[8]; /* nothing: the members' offsets keep their original alignment */
    struct {
        uint8_t pad00[12];
        struct {
            float x;
            float y;
            float z;
        } pos;
    } gFxPuffBlock;          /* 0x00554738; the type is local to bmx/com/action/fx.c */
    uint32_t d_00554750[18]; /* 0x00554750 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gFxPuffBlockGroup, member) == (addr) - 0x00554730, #member)
AT(gFxPuffBlock, 0x00554738);
AT(d_00554750, 0x00554750);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gFxPuffBlockGroup, d_00554750) +
                       sizeof(((struct BmxGroup_gFxPuffBlockGroup *)0)->d_00554750) ==
                   0x68,
               "gFxPuffBlockGroup");

/* The type is local to bmx/com/action/fx.c. */
typedef struct {
    uint16_t index;
    uint16_t value;
    int16_t x;
    int16_t y;
} BmxType_gFxScatterSlots[42];

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00554928..0x00554954.
 */
struct BmxGroup_d_00554928Group {
    uint8_t lead_[8];       /* nothing: the members' offsets keep their original alignment */
    uint32_t d_00554928[4]; /* 0x00554928 */
    void *p_00554938[7];    /* 0x00554938 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_d_00554928Group, member) == (addr) - 0x00554920, #member)
AT(d_00554928, 0x00554928);
AT(p_00554938, 0x00554938);
#undef AT
_Static_assert(offsetof(struct BmxGroup_d_00554928Group, p_00554938) +
                       sizeof(((struct BmxGroup_d_00554928Group *)0)->p_00554938) ==
                   BMX_DATA_OFF(0x34, 0x50),
               "d_00554928Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00554964..0x005549ac.
 */
struct BmxGroup_gFxSkidCornersGroup {
    uint8_t lead_[4]; /* nothing: the members' offsets keep their original alignment */
    struct {
        struct {
            float x;
            float y;
            float z;
        } pos;
        uint32_t other;
    } gFxSkidCorners[4]; /* 0x00554964; the type is local to bmx/com/action/fx.c */
    uint32_t d_005549a4; /* 0x005549a4 */
    void *p_005549a8;    /* 0x005549a8 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gFxSkidCornersGroup, member) == (addr) - 0x00554960, #member)
AT(gFxSkidCorners, 0x00554964);
AT(d_005549a4, 0x005549a4);
AT(p_005549a8, 0x005549a8);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gFxSkidCornersGroup, p_005549a8) +
                       sizeof(((struct BmxGroup_gFxSkidCornersGroup *)0)->p_005549a8) ==
                   BMX_DATA_OFF(0x4c, 0x50),
               "gFxSkidCornersGroup");

/* The type is local to bmx/com/action/fx.c. */
typedef void *BmxType_gFxPresets[6];

/* The type is local to bmx/com/action/fx.c. */
typedef struct {
    uint8_t pad00[2];
    uint16_t value;
    uint32_t id;
    struct {
        float x;
        float y;
        float z;
    } pos;
    uint8_t pad14[4];
    void *preset;
    uint8_t pad1c[4];
} BmxType_gFxEmitBlock;

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00554ad8..0x00554b14.
 */
struct BmxGroup_gFxCreateBlockGroup {
    uint8_t lead_[8]; /* nothing: the members' offsets keep their original alignment */
    struct {
        uint8_t pad00[2];
        uint16_t value;
        uint32_t id;
        struct {
            float x;
            float y;
            float z;
        } pos;
        uint8_t pad14[4];
        void *preset;
        uint8_t pad1c[4];
    } gFxCreateBlock;       /* 0x00554ad8; the type is local to bmx/com/action/fx.c */
    uint32_t d_00554af8[7]; /* 0x00554af8 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gFxCreateBlockGroup, member) ==                                  \
                       BMX_DATA_OFF((addr) - 0x00554ad0, off64),                                             \
                   #member)
AT(gFxCreateBlock, 0x00554ad8, 0x8);
AT(d_00554af8, 0x00554af8, 0x30);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gFxCreateBlockGroup, d_00554af8) +
                       sizeof(((struct BmxGroup_gFxCreateBlockGroup *)0)->d_00554af8) ==
                   BMX_DATA_OFF(0x44, 0x4c),
               "gFxCreateBlockGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00554c80..0x00554cb0.
 */
struct BmxGroup_gGameRoundCountsGroup {
    char gGameJudgePar[8];            /* 0x00554c80 */
    unsigned char pad_00554c88[4];    /* 0x00554c88 */
    GameRoundCounts gGameRoundCounts; /* 0x00554c8c */
    uint32_t d_00554c98[6];           /* 0x00554c98 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gGameRoundCountsGroup, member) == (addr) - 0x00554c80, #member)
AT(gGameJudgePar, 0x00554c80);
AT(pad_00554c88, 0x00554c88);
AT(gGameRoundCounts, 0x00554c8c);
AT(d_00554c98, 0x00554c98);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gGameRoundCountsGroup, d_00554c98) +
                       sizeof(((struct BmxGroup_gGameRoundCountsGroup *)0)->d_00554c98) ==
                   0x30,
               "gGameRoundCountsGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x005568b8..0x00556948.
 */
struct BmxGroup_gGoalEntriesAGroup {
    uint8_t lead_[8];             /* nothing: the members' offsets keep their original alignment */
    OvlGoalEntries gGoalEntriesA; /* 0x005568b8 */
    uint32_t d_00556938[4];       /* 0x00556938 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gGoalEntriesAGroup, member) == (addr) - 0x005568b0, #member)
AT(gGoalEntriesA, 0x005568b8);
AT(d_00556938, 0x00556938);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gGoalEntriesAGroup, d_00556938) +
                       sizeof(((struct BmxGroup_gGoalEntriesAGroup *)0)->d_00556938) ==
                   0x98,
               "gGoalEntriesAGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00556948..0x00556ef8.
 */
struct BmxGroup_gGoalEntriesBGroup {
    uint8_t lead_[8];                 /* nothing: the members' offsets keep their original alignment */
    OvlGoalEntries gGoalEntriesB;     /* 0x00556948, also gGoalColumns, gGoalEntriesC, gGoalEntriesD */
    uint32_t d_005569c8[8];           /* 0x005569c8 */
    char s0123456[20];                /* 0x005569e8 */
    unsigned char pad_005569fc[1276]; /* 0x005569fc */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gGoalEntriesBGroup, member) == (addr) - 0x00556940, #member)
AT(gGoalEntriesB, 0x00556948);
AT(d_005569c8, 0x005569c8);
AT(s0123456, 0x005569e8);
AT(pad_005569fc, 0x005569fc);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gGoalEntriesBGroup, pad_005569fc) +
                       sizeof(((struct BmxGroup_gGoalEntriesBGroup *)0)->pad_005569fc) ==
                   0x5b8,
               "gGoalEntriesBGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x005570a8..0x005570b8.
 */
struct BmxGroup_p_005570a8Group {
    uint8_t lead_[8];       /* nothing: the members' offsets keep their original alignment */
    void *p_005570a8[2];    /* 0x005570a8 */
    uint32_t d_005570b0[2]; /* 0x005570b0 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_p_005570a8Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x005570a0, off64),                                             \
                   #member)
AT(p_005570a8, 0x005570a8, 0x8);
AT(d_005570b0, 0x005570b0, 0x18);
#undef AT
_Static_assert(offsetof(struct BmxGroup_p_005570a8Group, d_005570b0) +
                       sizeof(((struct BmxGroup_p_005570a8Group *)0)->d_005570b0) ==
                   BMX_DATA_OFF(0x18, 0x20),
               "p_005570a8Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x005570b8..0x005570c8.
 */
struct BmxGroup_p_005570b8Group {
    uint8_t lead_[8];    /* nothing: the members' offsets keep their original alignment */
    void *p_005570b8[3]; /* 0x005570b8 */
    uint32_t d_005570c4; /* 0x005570c4 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_p_005570b8Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x005570b0, off64),                                             \
                   #member)
AT(p_005570b8, 0x005570b8, 0x8);
AT(d_005570c4, 0x005570c4, 0x20);
#undef AT
_Static_assert(offsetof(struct BmxGroup_p_005570b8Group, d_005570c4) +
                       sizeof(((struct BmxGroup_p_005570b8Group *)0)->d_005570c4) ==
                   BMX_DATA_OFF(0x18, 0x24),
               "p_005570b8Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x005570c8..0x005570d8.
 */
struct BmxGroup_p_005570c8Group {
    uint8_t lead_[8];    /* nothing: the members' offsets keep their original alignment */
    void *p_005570c8[3]; /* 0x005570c8 */
    uint32_t d_005570d4; /* 0x005570d4 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_p_005570c8Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x005570c0, off64),                                             \
                   #member)
AT(p_005570c8, 0x005570c8, 0x8);
AT(d_005570d4, 0x005570d4, 0x20);
#undef AT
_Static_assert(offsetof(struct BmxGroup_p_005570c8Group, d_005570d4) +
                       sizeof(((struct BmxGroup_p_005570c8Group *)0)->d_005570d4) ==
                   BMX_DATA_OFF(0x18, 0x24),
               "p_005570c8Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x005570d8..0x005570e8.
 */
struct BmxGroup_p_005570d8Group {
    uint8_t lead_[8];       /* nothing: the members' offsets keep their original alignment */
    void *p_005570d8[2];    /* 0x005570d8 */
    uint32_t d_005570e0[2]; /* 0x005570e0 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_p_005570d8Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x005570d0, off64),                                             \
                   #member)
AT(p_005570d8, 0x005570d8, 0x8);
AT(d_005570e0, 0x005570e0, 0x18);
#undef AT
_Static_assert(offsetof(struct BmxGroup_p_005570d8Group, d_005570e0) +
                       sizeof(((struct BmxGroup_p_005570d8Group *)0)->d_005570e0) ==
                   BMX_DATA_OFF(0x18, 0x20),
               "p_005570d8Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x005570e8..0x005570f8.
 */
struct BmxGroup_d_005570e8Group {
    uint8_t lead_[8];            /* nothing: the members' offsets keep their original alignment */
    unsigned char d_005570e8[4]; /* 0x005570e8 */
    void *p_005570ec[2];         /* 0x005570ec */
    uint32_t d_005570f4;         /* 0x005570f4 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_d_005570e8Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x005570e0, off64),                                             \
                   #member)
AT(d_005570e8, 0x005570e8, 0x8);
AT(p_005570ec, 0x005570ec, 0x10);
AT(d_005570f4, 0x005570f4, 0x20);
#undef AT
_Static_assert(offsetof(struct BmxGroup_d_005570e8Group, d_005570f4) +
                       sizeof(((struct BmxGroup_d_005570e8Group *)0)->d_005570f4) ==
                   BMX_DATA_OFF(0x18, 0x24),
               "d_005570e8Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x005570f8..0x00557108.
 */
struct BmxGroup_d_005570f8Group {
    uint8_t lead_[8];            /* nothing: the members' offsets keep their original alignment */
    unsigned char d_005570f8[4]; /* 0x005570f8 */
    void *p_005570fc[2];         /* 0x005570fc */
    uint32_t d_00557104;         /* 0x00557104 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_d_005570f8Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x005570f0, off64),                                             \
                   #member)
AT(d_005570f8, 0x005570f8, 0x8);
AT(p_005570fc, 0x005570fc, 0x10);
AT(d_00557104, 0x00557104, 0x20);
#undef AT
_Static_assert(offsetof(struct BmxGroup_d_005570f8Group, d_00557104) +
                       sizeof(((struct BmxGroup_d_005570f8Group *)0)->d_00557104) ==
                   BMX_DATA_OFF(0x18, 0x24),
               "d_005570f8Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00557108..0x00557118.
 */
struct BmxGroup_p_00557108Group {
    uint8_t lead_[8];    /* nothing: the members' offsets keep their original alignment */
    void *p_00557108[3]; /* 0x00557108 */
    uint32_t d_00557114; /* 0x00557114 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_p_00557108Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x00557100, off64),                                             \
                   #member)
AT(p_00557108, 0x00557108, 0x8);
AT(d_00557114, 0x00557114, 0x20);
#undef AT
_Static_assert(offsetof(struct BmxGroup_p_00557108Group, d_00557114) +
                       sizeof(((struct BmxGroup_p_00557108Group *)0)->d_00557114) ==
                   BMX_DATA_OFF(0x18, 0x24),
               "p_00557108Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00557118..0x00557128.
 */
struct BmxGroup_p_00557118Group {
    uint8_t lead_[8];              /* nothing: the members' offsets keep their original alignment */
    void *p_00557118[3];           /* 0x00557118 */
    unsigned char pad_00557124[4]; /* 0x00557124 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_p_00557118Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x00557110, off64),                                             \
                   #member)
AT(p_00557118, 0x00557118, 0x8);
AT(pad_00557124, 0x00557124, 0x20);
#undef AT
_Static_assert(offsetof(struct BmxGroup_p_00557118Group, pad_00557124) +
                       sizeof(((struct BmxGroup_p_00557118Group *)0)->pad_00557124) ==
                   BMX_DATA_OFF(0x18, 0x24),
               "p_00557118Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00557128..0x00557138.
 */
struct BmxGroup_p_00557128Group {
    uint8_t lead_[8];    /* nothing: the members' offsets keep their original alignment */
    void *p_00557128[3]; /* 0x00557128 */
    uint32_t d_00557134; /* 0x00557134 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_p_00557128Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x00557120, off64),                                             \
                   #member)
AT(p_00557128, 0x00557128, 0x8);
AT(d_00557134, 0x00557134, 0x20);
#undef AT
_Static_assert(offsetof(struct BmxGroup_p_00557128Group, d_00557134) +
                       sizeof(((struct BmxGroup_p_00557128Group *)0)->d_00557134) ==
                   BMX_DATA_OFF(0x18, 0x24),
               "p_00557128Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00557138..0x00557148.
 */
struct BmxGroup_p_00557138Group {
    uint8_t lead_[8];    /* nothing: the members' offsets keep their original alignment */
    void *p_00557138[3]; /* 0x00557138 */
    uint32_t d_00557144; /* 0x00557144 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_p_00557138Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x00557130, off64),                                             \
                   #member)
AT(p_00557138, 0x00557138, 0x8);
AT(d_00557144, 0x00557144, 0x20);
#undef AT
_Static_assert(offsetof(struct BmxGroup_p_00557138Group, d_00557144) +
                       sizeof(((struct BmxGroup_p_00557138Group *)0)->d_00557144) ==
                   BMX_DATA_OFF(0x18, 0x24),
               "p_00557138Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00557148..0x00557158.
 */
struct BmxGroup_p_00557148Group {
    uint8_t lead_[8];    /* nothing: the members' offsets keep their original alignment */
    void *p_00557148[3]; /* 0x00557148 */
    uint32_t d_00557154; /* 0x00557154 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_p_00557148Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x00557140, off64),                                             \
                   #member)
AT(p_00557148, 0x00557148, 0x8);
AT(d_00557154, 0x00557154, 0x20);
#undef AT
_Static_assert(offsetof(struct BmxGroup_p_00557148Group, d_00557154) +
                       sizeof(((struct BmxGroup_p_00557148Group *)0)->d_00557154) ==
                   BMX_DATA_OFF(0x18, 0x24),
               "p_00557148Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00557158..0x00557168.
 */
struct BmxGroup_p_00557158Group {
    uint8_t lead_[8];    /* nothing: the members' offsets keep their original alignment */
    void *p_00557158[3]; /* 0x00557158 */
    uint32_t d_00557164; /* 0x00557164 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_p_00557158Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x00557150, off64),                                             \
                   #member)
AT(p_00557158, 0x00557158, 0x8);
AT(d_00557164, 0x00557164, 0x20);
#undef AT
_Static_assert(offsetof(struct BmxGroup_p_00557158Group, d_00557164) +
                       sizeof(((struct BmxGroup_p_00557158Group *)0)->d_00557164) ==
                   BMX_DATA_OFF(0x18, 0x24),
               "p_00557158Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00557168..0x00557178.
 */
struct BmxGroup_p_00557168Group {
    uint8_t lead_[8];    /* nothing: the members' offsets keep their original alignment */
    void *p_00557168[3]; /* 0x00557168 */
    uint32_t d_00557174; /* 0x00557174 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_p_00557168Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x00557160, off64),                                             \
                   #member)
AT(p_00557168, 0x00557168, 0x8);
AT(d_00557174, 0x00557174, 0x20);
#undef AT
_Static_assert(offsetof(struct BmxGroup_p_00557168Group, d_00557174) +
                       sizeof(((struct BmxGroup_p_00557168Group *)0)->d_00557174) ==
                   BMX_DATA_OFF(0x18, 0x24),
               "p_00557168Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00557178..0x00557184.
 */
struct BmxGroup_d_00557178Group {
    uint8_t lead_[8];            /* nothing: the members' offsets keep their original alignment */
    unsigned char d_00557178[4]; /* 0x00557178 */
    void *p_0055717c[2];         /* 0x0055717c */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_d_00557178Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x00557170, off64),                                             \
                   #member)
AT(d_00557178, 0x00557178, 0x8);
AT(p_0055717c, 0x0055717c, 0x10);
#undef AT
_Static_assert(offsetof(struct BmxGroup_d_00557178Group, p_0055717c) +
                       sizeof(((struct BmxGroup_d_00557178Group *)0)->p_0055717c) ==
                   BMX_DATA_OFF(0x14, 0x20),
               "d_00557178Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00557188..0x00557198.
 */
struct BmxGroup_d_00557188Group {
    uint8_t lead_[8];            /* nothing: the members' offsets keep their original alignment */
    unsigned char d_00557188[4]; /* 0x00557188 */
    void *p_0055718c[2];         /* 0x0055718c */
    uint32_t d_00557194;         /* 0x00557194 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_d_00557188Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x00557180, off64),                                             \
                   #member)
AT(d_00557188, 0x00557188, 0x8);
AT(p_0055718c, 0x0055718c, 0x10);
AT(d_00557194, 0x00557194, 0x20);
#undef AT
_Static_assert(offsetof(struct BmxGroup_d_00557188Group, d_00557194) +
                       sizeof(((struct BmxGroup_d_00557188Group *)0)->d_00557194) ==
                   BMX_DATA_OFF(0x18, 0x24),
               "d_00557188Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00557198..0x005571a8.
 */
struct BmxGroup_d_00557198Group {
    uint8_t lead_[8];            /* nothing: the members' offsets keep their original alignment */
    unsigned char d_00557198[4]; /* 0x00557198 */
    void *p_0055719c[2];         /* 0x0055719c */
    uint32_t d_005571a4;         /* 0x005571a4 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_d_00557198Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x00557190, off64),                                             \
                   #member)
AT(d_00557198, 0x00557198, 0x8);
AT(p_0055719c, 0x0055719c, 0x10);
AT(d_005571a4, 0x005571a4, 0x20);
#undef AT
_Static_assert(offsetof(struct BmxGroup_d_00557198Group, d_005571a4) +
                       sizeof(((struct BmxGroup_d_00557198Group *)0)->d_005571a4) ==
                   BMX_DATA_OFF(0x18, 0x24),
               "d_00557198Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x005571a8..0x005571b8.
 */
struct BmxGroup_d_005571a8Group {
    uint8_t lead_[8];            /* nothing: the members' offsets keep their original alignment */
    unsigned char d_005571a8[4]; /* 0x005571a8 */
    void *p_005571ac[2];         /* 0x005571ac */
    uint32_t d_005571b4;         /* 0x005571b4 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_d_005571a8Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x005571a0, off64),                                             \
                   #member)
AT(d_005571a8, 0x005571a8, 0x8);
AT(p_005571ac, 0x005571ac, 0x10);
AT(d_005571b4, 0x005571b4, 0x20);
#undef AT
_Static_assert(offsetof(struct BmxGroup_d_005571a8Group, d_005571b4) +
                       sizeof(((struct BmxGroup_d_005571a8Group *)0)->d_005571b4) ==
                   BMX_DATA_OFF(0x18, 0x24),
               "d_005571a8Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00557260..0x00557298.
 */
struct BmxGroup_gOvlBarSizeGroup {
    OvlSize2 gOvlBarSize;           /* 0x00557260 */
    unsigned char pad_00557268[48]; /* 0x00557268 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gOvlBarSizeGroup, member) == (addr) - 0x00557260, #member)
AT(gOvlBarSize, 0x00557260);
AT(pad_00557268, 0x00557268);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gOvlBarSizeGroup, pad_00557268) +
                       sizeof(((struct BmxGroup_gOvlBarSizeGroup *)0)->pad_00557268) ==
                   0x38,
               "gOvlBarSizeGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00557298..0x005572bb.
 */
struct BmxGroup_gOvlFadeSizeGroup {
    uint8_t lead_[8];               /* nothing: the members' offsets keep their original alignment */
    OvlSize2 gOvlFadeSize;          /* 0x00557298 */
    unsigned char pad_005572a0[27]; /* 0x005572a0 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gOvlFadeSizeGroup, member) == (addr) - 0x00557290, #member)
AT(gOvlFadeSize, 0x00557298);
AT(pad_005572a0, 0x005572a0);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gOvlFadeSizeGroup, pad_005572a0) +
                       sizeof(((struct BmxGroup_gOvlFadeSizeGroup *)0)->pad_005572a0) ==
                   0x2b,
               "gOvlFadeSizeGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x005572bb..0x005573ec.
 */
struct BmxGroup_gIndicatorTextGroup {
    uint8_t lead_[11];               /* nothing: the members' offsets keep their original alignment */
    unsigned char d_005572bb[16];    /* 0x005572bb, also gHintGlyphs */
    unsigned char gButtonGlyphs[9];  /* 0x005572cb */
    OvlStrTable5 gIndicatorText;     /* 0x005572d4 */
    OvlColors5 gIndicatorColor;      /* 0x005572e8 */
    uint32_t d_005572fc[3];          /* 0x005572fc */
    OvlStrTable4 gTrickDoubleNames;  /* 0x00557308 */
    unsigned char pad_00557318[212]; /* 0x00557318 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gIndicatorTextGroup, member) ==                                  \
                       BMX_DATA_OFF((addr) - 0x005572b0, off64),                                             \
                   #member)
AT(d_005572bb, 0x005572bb, 0xb);
AT(gButtonGlyphs, 0x005572cb, 0x1b);
AT(gIndicatorText, 0x005572d4, 0x28);
AT(gIndicatorColor, 0x005572e8, 0x50);
AT(d_005572fc, 0x005572fc, 0x64);
AT(gTrickDoubleNames, 0x00557308, 0x70);
AT(pad_00557318, 0x00557318, 0x90);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gIndicatorTextGroup, pad_00557318) +
                       sizeof(((struct BmxGroup_gIndicatorTextGroup *)0)->pad_00557318) ==
                   BMX_DATA_OFF(0x13c, 0x164),
               "gIndicatorTextGroup");

/* The type is local to bmx/com/action/physics_roll.h. */
typedef float BmxType_gPhysRollFriction[5];

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0055782c..0x00557834.
 */
struct BmxGroup_gPhysAirSpeedCapGroup {
    uint8_t lead_[12];      /* nothing: the members' offsets keep their original alignment */
    float gPhysAirSpeedCap; /* 0x0055782c */
    float d_00557830;       /* 0x00557830 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gPhysAirSpeedCapGroup, member) == (addr) - 0x00557820, #member)
AT(gPhysAirSpeedCap, 0x0055782c);
AT(d_00557830, 0x00557830);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gPhysAirSpeedCapGroup, d_00557830) +
                       sizeof(((struct BmxGroup_gPhysAirSpeedCapGroup *)0)->d_00557830) ==
                   0x14,
               "gPhysAirSpeedCapGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0055784e..0x00557856.
 */
struct BmxGroup_gPhysRailAngleLimitGroup {
    uint8_t lead_[14];           /* nothing: the members' offsets keep their original alignment */
    int16_t gPhysRailAngleLimit; /* 0x0055784e */
    unsigned char d_00557850[6]; /* 0x00557850 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gPhysRailAngleLimitGroup, member) == (addr) - 0x00557840, #member)
AT(gPhysRailAngleLimit, 0x0055784e);
AT(d_00557850, 0x00557850);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gPhysRailAngleLimitGroup, d_00557850) +
                       sizeof(((struct BmxGroup_gPhysRailAngleLimitGroup *)0)->d_00557850) ==
                   0x16,
               "gPhysRailAngleLimitGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00557868..0x00557870.
 */
struct BmxGroup_gPhysGrindInRadiusGroup {
    uint8_t lead_[8];         /* nothing: the members' offsets keep their original alignment */
    float gPhysGrindInRadius; /* 0x00557868 */
    float d_0055786c;         /* 0x0055786c */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gPhysGrindInRadiusGroup, member) == (addr) - 0x00557860, #member)
AT(gPhysGrindInRadius, 0x00557868);
AT(d_0055786c, 0x0055786c);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gPhysGrindInRadiusGroup, d_0055786c) +
                       sizeof(((struct BmxGroup_gPhysGrindInRadiusGroup *)0)->d_0055786c) ==
                   0x10,
               "gPhysGrindInRadiusGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00557924..0x00557984.
 */
struct BmxGroup_gPhysStateSyncGroup {
    uint8_t lead_[4];               /* nothing: the members' offsets keep their original alignment */
    PhysStepTable gPhysStateSync;   /* 0x00557924 */
    unsigned char pad_00557940[68]; /* 0x00557940 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gPhysStateSyncGroup, member) ==                                  \
                       BMX_DATA_OFF((addr) - 0x00557920, off64),                                             \
                   #member)
AT(gPhysStateSync, 0x00557924, 0x8);
AT(pad_00557940, 0x00557940, 0x40);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gPhysStateSyncGroup, pad_00557940) +
                       sizeof(((struct BmxGroup_gPhysStateSyncGroup *)0)->pad_00557940) ==
                   BMX_DATA_OFF(0x64, 0x84),
               "gPhysStateSyncGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00557d28..0x00558628.
 */
struct BmxGroup_gPropDefsGroup {
    uint8_t lead_[8];                /* nothing: the members' offsets keep their original alignment */
    PropDefs gPropDefs;              /* 0x00557d28 */
    unsigned char pad_005584a8[384]; /* 0x005584a8 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gPropDefsGroup, member) == (addr) - 0x00557d20, #member)
AT(gPropDefs, 0x00557d28);
AT(pad_005584a8, 0x005584a8);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gPropDefsGroup, pad_005584a8) +
                       sizeof(((struct BmxGroup_gPropDefsGroup *)0)->pad_005584a8) ==
                   0x908,
               "gPropDefsGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0055887c..0x00558888.
 */
struct BmxGroup_gRiderJumpBaseGroup {
    uint8_t lead_[12];    /* nothing: the members' offsets keep their original alignment */
    float gRiderJumpBase; /* 0x0055887c */
    float d_00558880[2];  /* 0x00558880 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gRiderJumpBaseGroup, member) == (addr) - 0x00558870, #member)
AT(gRiderJumpBase, 0x0055887c);
AT(d_00558880, 0x00558880);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gRiderJumpBaseGroup, d_00558880) +
                       sizeof(((struct BmxGroup_gRiderJumpBaseGroup *)0)->d_00558880) ==
                   0x18,
               "gRiderJumpBaseGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x005588a8..0x005588b0.
 */
struct BmxGroup_gRiderBikeSpinMaxGroup {
    uint8_t lead_[8];        /* nothing: the members' offsets keep their original alignment */
    float gRiderBikeSpinMax; /* 0x005588a8 */
    float d_005588ac;        /* 0x005588ac */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gRiderBikeSpinMaxGroup, member) == (addr) - 0x005588a0, #member)
AT(gRiderBikeSpinMax, 0x005588a8);
AT(d_005588ac, 0x005588ac);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gRiderBikeSpinMaxGroup, d_005588ac) +
                       sizeof(((struct BmxGroup_gRiderBikeSpinMaxGroup *)0)->d_005588ac) ==
                   0x10,
               "gRiderBikeSpinMaxGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x005588d4..0x005588f8.
 */
struct BmxGroup_gRiderAirStepGroup {
    uint8_t lead_[4];            /* nothing: the members' offsets keep their original alignment */
    void *gRiderRollStep[6];     /* 0x005588d4 */
    RiderAirTable gRiderAirStep; /* 0x005588ec */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gRiderAirStepGroup, member) ==                                   \
                       BMX_DATA_OFF((addr) - 0x005588d0, off64),                                             \
                   #member)
AT(gRiderRollStep, 0x005588d4, 0x8);
AT(gRiderAirStep, 0x005588ec, 0x38);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gRiderAirStepGroup, gRiderAirStep) +
                       sizeof(((struct BmxGroup_gRiderAirStepGroup *)0)->gRiderAirStep) ==
                   BMX_DATA_OFF(0x28, 0x50),
               "gRiderAirStepGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x005588f8..0x00558a48.
 */
struct BmxGroup_gRiderGrindStepGroup {
    uint8_t lead_[8];                        /* nothing: the members' offsets keep their original alignment */
    RiderSingleTable gRiderGrindStep;        /* 0x005588f8 */
    RiderSingleTable gRiderLipStep;          /* 0x005588fc */
    RiderSingleTable gRiderManualStep;       /* 0x00558900 */
    uint32_t gRiderStickDir[4];              /* 0x00558904 */
    float gRiderScoreWeight[16];             /* 0x00558914; the type is local to bmx/com/action/rider.c */
    unsigned char pad_00558954[36];          /* 0x00558954 */
    char sInvalidMainUpdateProcNULL[36];     /* 0x00558978 */
    unsigned char pad_0055899c[4];           /* 0x0055899c */
    char sDirectionBufferOutOfRangePost[40]; /* 0x005589a0 */
    unsigned char pad_005589c8[8];           /* 0x005589c8 */
    char sCZaxisBmxSrcComActionRiderC[36];   /* 0x005589d0 */
    unsigned char pad_005589f4[8];           /* 0x005589f4 */
    char sFailedToCreateBikeActorForRiderLdBikeLo[64]; /* 0x005589fc */
    unsigned char pad_00558a3c[12];                    /* 0x00558a3c */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gRiderGrindStepGroup, member) ==                                 \
                       BMX_DATA_OFF((addr) - 0x005588f0, off64),                                             \
                   #member)
AT(gRiderGrindStep, 0x005588f8, 0x8);
AT(gRiderLipStep, 0x005588fc, 0x10);
AT(gRiderManualStep, 0x00558900, 0x18);
AT(gRiderStickDir, 0x00558904, 0x20);
AT(gRiderScoreWeight, 0x00558914, 0x30);
AT(pad_00558954, 0x00558954, 0x70);
AT(sInvalidMainUpdateProcNULL, 0x00558978, 0x94);
AT(pad_0055899c, 0x0055899c, 0xb8);
AT(sDirectionBufferOutOfRangePost, 0x005589a0, 0xbc);
AT(pad_005589c8, 0x005589c8, 0xe4);
AT(sCZaxisBmxSrcComActionRiderC, 0x005589d0, 0xec);
AT(pad_005589f4, 0x005589f4, 0x110);
AT(sFailedToCreateBikeActorForRiderLdBikeLo, 0x005589fc, 0x118);
AT(pad_00558a3c, 0x00558a3c, 0x158);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gRiderGrindStepGroup, pad_00558a3c) +
                       sizeof(((struct BmxGroup_gRiderGrindStepGroup *)0)->pad_00558a3c) ==
                   BMX_DATA_OFF(0x158, 0x164),
               "gRiderGrindStepGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00558af7..0x00559b94.
 */
struct BmxGroup_gRiderTrickComboGroup {
    uint8_t lead_[7];                     /* nothing: the members' offsets keep their original alignment */
    unsigned char gRiderAirTrickPick[73]; /* 0x00558af7 */
    uint8_t gRiderTrickCombo[256];        /* 0x00558b40, also gRiderAirTrickMod; the type is local to
                                             bmx/com/action/rider.c */
    uint32_t d_00558c40[11];              /* 0x00558c40 */
    char s_00558c6c[5];                   /* 0x00558c6c */
    unsigned char d_00558c71[71];         /* 0x00558c71 */
    char s_00558cb8[5];                   /* 0x00558cb8 */
    unsigned char d_00558cbd[31];         /* 0x00558cbd */
    char s_00558cdc[7];                   /* 0x00558cdc */
    unsigned char d_00558ce3[33];         /* 0x00558ce3 */
    char s_00558d04[5];                   /* 0x00558d04 */
    unsigned char d_00558d09[31];         /* 0x00558d09 */
    char s_00558d28[7];                   /* 0x00558d28 */
    unsigned char d_00558d2f[33];         /* 0x00558d2f */
    char s_00558d50[5];                   /* 0x00558d50 */
    unsigned char d_00558d55[31];         /* 0x00558d55 */
    char s_00558d74[7];                   /* 0x00558d74 */
    unsigned char d_00558d7b[69];         /* 0x00558d7b */
    char s_00558dc0[7];                   /* 0x00558dc0 */
    unsigned char d_00558dc7[33];         /* 0x00558dc7 */
    char s_00558de8[5];                   /* 0x00558de8 */
    unsigned char d_00558ded[71];         /* 0x00558ded */
    char s_00558e34[5];                   /* 0x00558e34 */
    unsigned char d_00558e39[183];        /* 0x00558e39 */
    char s_00558ef0[7];                   /* 0x00558ef0 */
    unsigned char d_00558ef7[109];        /* 0x00558ef7 */
    char s_00558f64[5];                   /* 0x00558f64 */
    unsigned char d_00558f69[39];         /* 0x00558f69 */
    uint32_t gRiderTrickA[3];             /* 0x00558f90 */
    unsigned char gRiderTrickB[6];        /* 0x00558f9c */
    char gTrickClassHi[6];                /* 0x00558fa2 */
    uint32_t gRiderTrickKind[3];          /* 0x00558fa8 */
    OvlBytes32 gTrickClassLo;      /* 0x00558fb4, also gRiderAnimFlags, gRiderPoseFlags, gRiderTrickFlags */
    uint32_t d_00558fd4[9];        /* 0x00558fd4 */
    uint32_t gRiderTrickAnim[14];  /* 0x00558ff8 */
    char gRiderPoseAnim[2];        /* 0x00559030 */
    unsigned char d_00559032[74];  /* 0x00559032 */
    uint32_t gPhysIkLimbMasks[39]; /* 0x0055907c */
    struct {
        uint16_t base;
        uint16_t limit;
    } gScoreTrick[64];       /* 0x00559118, also gScoreStyle; the type is local to bmx/com/action/game.c */
    uint32_t d_00559218[21]; /* 0x00559218 */
    uint16_t
        gScoreTierAdd[16];   /* 0x0055926c, also gScoreStyleMul; the type is local to bmx/com/action/game.c */
    float d_0055928c[36];    /* 0x0055928c */
    float gScoreTierMul[16]; /* 0x0055931c, also gScoreLevelMul, gScoreMultTable; the type is local to
                                bmx/com/action/game.c */
    float d_0055935c[5];     /* 0x0055935c */
    float gScoreSpinMul[8];  /* 0x00559370 */
    OvlStrTable gTrickTurnNames;     /* 0x00559390, also gTrickBarspinNames */
    void *p_00559490[20];            /* 0x00559490 */
    OvlStrTable gTrickSpinNames;     /* 0x005594e0 */
    unsigned char pad_005595e0[296]; /* 0x005595e0 */
    char sExtreme[8];                /* 0x00559708 */
    unsigned char pad_00559710[4];   /* 0x00559710 */
    char s1260[8];                   /* 0x00559714 */
    char s1080[8];                   /* 0x0055971c */
    char s900[4];                    /* 0x00559724 */
    char s720[4];                    /* 0x00559728 */
    char s540[4];                    /* 0x0055972c */
    char s360[4];                    /* 0x00559730 */
    char s180[4];                    /* 0x00559734 */
    char sBusdriver[12];             /* 0x00559738 */
    char sBarspinSeatGrab[20];       /* 0x00559744 */
    unsigned char pad_00559758[4];   /* 0x00559758 */
    char sXUpSeatGrab[16];           /* 0x0055975c */
    unsigned char pad_0055976c[4];   /* 0x0055976c */
    char sSwitchFooter[16];          /* 0x00559770 */
    char sIndian[8];                 /* 0x00559780 */
    char sRocketOneFooter[20];       /* 0x00559788 */
    unsigned char pad_0055979c[4];   /* 0x0055979c */
    char sRocketBarspin[16];         /* 0x005597a0 */
    unsigned char pad_005597b0[4];   /* 0x005597b0 */
    char sRocketNoHander[20];        /* 0x005597b4 */
    char sRocketOneHander[20];       /* 0x005597c8 */
    unsigned char pad_005597dc[4];   /* 0x005597dc */
    char sCrotchRocket[16];          /* 0x005597e0 */
    char sRocketXUp[12];             /* 0x005597f0 */
    unsigned char pad_005597fc[4];   /* 0x005597fc */
    char sOneFootedBarspin[20];      /* 0x00559800 */
    unsigned char pad_00559814[4];   /* 0x00559814 */
    char sNoFootedBarspin[20];       /* 0x00559818 */
    unsigned char pad_0055982c[4];   /* 0x0055982c */
    char sBarspinToBarspinBack[24];  /* 0x00559830 */
    unsigned char pad_00559848[4];   /* 0x00559848 */
    char sNoHandedBarspin[20];       /* 0x0055984c */
    unsigned char pad_00559860[4];   /* 0x00559860 */
    char sNoHandedOneFooter[24];     /* 0x00559864 */
    unsigned char pad_0055987c[4];   /* 0x0055987c */
    char sNoFootedOneHander[24];     /* 0x00559880 */
    unsigned char pad_00559898[4];   /* 0x00559898 */
    char sOneFootedXUp[16];          /* 0x0055989c */
    unsigned char pad_005598ac[4];   /* 0x005598ac */
    char sNoFootedXUp[16];           /* 0x005598b0 */
    unsigned char pad_005598c0[4];   /* 0x005598c0 */
    char sSwitchHander[16];          /* 0x005598c4 */
    char sDoubleSeatGrab[20];        /* 0x005598d4 */
    char sOneFootedSeatGrab[24];     /* 0x005598e8 */
    unsigned char pad_00559900[4];   /* 0x00559900 */
    char sNoFootedSeatGrab[20];      /* 0x00559904 */
    unsigned char pad_00559918[4];   /* 0x00559918 */
    char sOneHandedSeatGrab[24];     /* 0x0055991c */
    unsigned char pad_00559934[4];   /* 0x00559934 */
    char sDoublePegGrab[16];         /* 0x00559938 */
    unsigned char pad_00559948[4];   /* 0x00559948 */
    char sRocketAir[12];             /* 0x0055994c */
    unsigned char pad_00559958[4];   /* 0x00559958 */
    char sOneHandXUp[16];            /* 0x0055995c */
    char sOneHandOneFooter[20];      /* 0x0055996c */
    unsigned char pad_00559980[4];   /* 0x00559980 */
    char sNothing[8];                /* 0x00559984 */
    unsigned char pad_0055998c[4];   /* 0x0055998c */
    char sNoHander[12];              /* 0x00559990 */
    char sNoFooter[12];              /* 0x0055999c */
    char sSeatGrab[12];              /* 0x005599a8 */
    char sSuicideNoHander[20];       /* 0x005599b4 */
    unsigned char pad_005599c8[4];   /* 0x005599c8 */
    char sXDown[8];                  /* 0x005599cc */
    char sXUp[8];                    /* 0x005599d4 */
    char sOneHander[12];             /* 0x005599dc */
    unsigned char pad_005599e8[4];   /* 0x005599e8 */
    char sOneFooter[12];             /* 0x005599ec */
    unsigned char pad_005599f8[4];   /* 0x005599f8 */
    char sBarspin[8];                /* 0x005599fc */
    unsigned char pad_00559a04[4];   /* 0x00559a04 */
    char sNoseWheelie[16];           /* 0x00559a08 */
    char sManual[8];                 /* 0x00559a18 */
    char sWallTap[12];               /* 0x00559a20 */
    char sFastPlant[12];             /* 0x00559a2c */
    unsigned char pad_00559a38[4];   /* 0x00559a38 */
    char sDisaster[12];              /* 0x00559a3c */
    char sTailTap[12];               /* 0x00559a48 */
    char sNosepick[12];              /* 0x00559a54 */
    char sIcepickStall[16];          /* 0x00559a60 */
    char sToothpickStall[16];        /* 0x00559a70 */
    unsigned char pad_00559a80[4];   /* 0x00559a80 */
    char sDoublePegStall[20];        /* 0x00559a84 */
    char sSprocketGrind[16];         /* 0x00559a98 */
    unsigned char pad_00559aa8[4];   /* 0x00559aa8 */
    char sCrookedGrind[16];          /* 0x00559aac */
    char sLucEGrind[12];             /* 0x00559abc */
    unsigned char pad_00559ac8[4];   /* 0x00559ac8 */
    char sLipSlide[12];              /* 0x00559acc */
    char sIcepickGrind[16];          /* 0x00559ad8 */
    char sToothpickGrind[16];        /* 0x00559ae8 */
    unsigned char pad_00559af8[4];   /* 0x00559af8 */
    char sFeebleGrind[16];           /* 0x00559afc */
    char sSmithGrind[12];            /* 0x00559b0c */
    unsigned char pad_00559b18[4];   /* 0x00559b18 */
    char sDoublePegGrind[20];        /* 0x00559b1c */
    char sSupermanIndian[16];        /* 0x00559b30 */
    unsigned char pad_00559b40[4];   /* 0x00559b40 */
    char sCandyBarLookdown[20];      /* 0x00559b44 */
    unsigned char pad_00559b58[4];   /* 0x00559b58 */
    char sCanCanLookdown[20];        /* 0x00559b5c */
    char sDoubleTailwhip[16];        /* 0x00559b70 */
    unsigned char pad_00559b80[4];   /* 0x00559b80 */
    char sUnTurndown[12];            /* 0x00559b84 */
    unsigned char pad_00559b90[4];   /* 0x00559b90 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gRiderTrickComboGroup, member) ==                                \
                       BMX_DATA_OFF((addr) - 0x00558af0, off64),                                             \
                   #member)
AT(gRiderAirTrickPick, 0x00558af7, 0x7);
AT(gRiderTrickCombo, 0x00558b40, 0x50);
AT(d_00558c40, 0x00558c40, 0x150);
AT(s_00558c6c, 0x00558c6c, 0x17c);
AT(d_00558c71, 0x00558c71, 0x181);
AT(s_00558cb8, 0x00558cb8, 0x1c8);
AT(d_00558cbd, 0x00558cbd, 0x1cd);
AT(s_00558cdc, 0x00558cdc, 0x1ec);
AT(d_00558ce3, 0x00558ce3, 0x1f3);
AT(s_00558d04, 0x00558d04, 0x214);
AT(d_00558d09, 0x00558d09, 0x219);
AT(s_00558d28, 0x00558d28, 0x238);
AT(d_00558d2f, 0x00558d2f, 0x23f);
AT(s_00558d50, 0x00558d50, 0x260);
AT(d_00558d55, 0x00558d55, 0x265);
AT(s_00558d74, 0x00558d74, 0x284);
AT(d_00558d7b, 0x00558d7b, 0x28b);
AT(s_00558dc0, 0x00558dc0, 0x2d0);
AT(d_00558dc7, 0x00558dc7, 0x2d7);
AT(s_00558de8, 0x00558de8, 0x2f8);
AT(d_00558ded, 0x00558ded, 0x2fd);
AT(s_00558e34, 0x00558e34, 0x344);
AT(d_00558e39, 0x00558e39, 0x349);
AT(s_00558ef0, 0x00558ef0, 0x400);
AT(d_00558ef7, 0x00558ef7, 0x407);
AT(s_00558f64, 0x00558f64, 0x474);
AT(d_00558f69, 0x00558f69, 0x479);
AT(gRiderTrickA, 0x00558f90, 0x4a0);
AT(gRiderTrickB, 0x00558f9c, 0x4ac);
AT(gTrickClassHi, 0x00558fa2, 0x4b2);
AT(gRiderTrickKind, 0x00558fa8, 0x4b8);
AT(gTrickClassLo, 0x00558fb4, 0x4c4);
AT(d_00558fd4, 0x00558fd4, 0x4e4);
AT(gRiderTrickAnim, 0x00558ff8, 0x508);
AT(gRiderPoseAnim, 0x00559030, 0x540);
AT(d_00559032, 0x00559032, 0x542);
AT(gPhysIkLimbMasks, 0x0055907c, 0x58c);
AT(gScoreTrick, 0x00559118, 0x628);
AT(d_00559218, 0x00559218, 0x728);
AT(gScoreTierAdd, 0x0055926c, 0x77c);
AT(d_0055928c, 0x0055928c, 0x79c);
AT(gScoreTierMul, 0x0055931c, 0x82c);
AT(d_0055935c, 0x0055935c, 0x86c);
AT(gScoreSpinMul, 0x00559370, 0x880);
AT(gTrickTurnNames, 0x00559390, 0x8a0);
AT(p_00559490, 0x00559490, 0xaa0);
AT(gTrickSpinNames, 0x005594e0, 0xb40);
AT(pad_005595e0, 0x005595e0, 0xd40);
AT(sExtreme, 0x00559708, 0xe68);
AT(pad_00559710, 0x00559710, 0xe70);
AT(s1260, 0x00559714, 0xe74);
AT(s1080, 0x0055971c, 0xe7c);
AT(s900, 0x00559724, 0xe84);
AT(s720, 0x00559728, 0xe88);
AT(s540, 0x0055972c, 0xe8c);
AT(s360, 0x00559730, 0xe90);
AT(s180, 0x00559734, 0xe94);
AT(sBusdriver, 0x00559738, 0xe98);
AT(sBarspinSeatGrab, 0x00559744, 0xea4);
AT(pad_00559758, 0x00559758, 0xeb8);
AT(sXUpSeatGrab, 0x0055975c, 0xebc);
AT(pad_0055976c, 0x0055976c, 0xecc);
AT(sSwitchFooter, 0x00559770, 0xed0);
AT(sIndian, 0x00559780, 0xee0);
AT(sRocketOneFooter, 0x00559788, 0xee8);
AT(pad_0055979c, 0x0055979c, 0xefc);
AT(sRocketBarspin, 0x005597a0, 0xf00);
AT(pad_005597b0, 0x005597b0, 0xf10);
AT(sRocketNoHander, 0x005597b4, 0xf14);
AT(sRocketOneHander, 0x005597c8, 0xf28);
AT(pad_005597dc, 0x005597dc, 0xf3c);
AT(sCrotchRocket, 0x005597e0, 0xf40);
AT(sRocketXUp, 0x005597f0, 0xf50);
AT(pad_005597fc, 0x005597fc, 0xf5c);
AT(sOneFootedBarspin, 0x00559800, 0xf60);
AT(pad_00559814, 0x00559814, 0xf74);
AT(sNoFootedBarspin, 0x00559818, 0xf78);
AT(pad_0055982c, 0x0055982c, 0xf8c);
AT(sBarspinToBarspinBack, 0x00559830, 0xf90);
AT(pad_00559848, 0x00559848, 0xfa8);
AT(sNoHandedBarspin, 0x0055984c, 0xfac);
AT(pad_00559860, 0x00559860, 0xfc0);
AT(sNoHandedOneFooter, 0x00559864, 0xfc4);
AT(pad_0055987c, 0x0055987c, 0xfdc);
AT(sNoFootedOneHander, 0x00559880, 0xfe0);
AT(pad_00559898, 0x00559898, 0xff8);
AT(sOneFootedXUp, 0x0055989c, 0xffc);
AT(pad_005598ac, 0x005598ac, 0x100c);
AT(sNoFootedXUp, 0x005598b0, 0x1010);
AT(pad_005598c0, 0x005598c0, 0x1020);
AT(sSwitchHander, 0x005598c4, 0x1024);
AT(sDoubleSeatGrab, 0x005598d4, 0x1034);
AT(sOneFootedSeatGrab, 0x005598e8, 0x1048);
AT(pad_00559900, 0x00559900, 0x1060);
AT(sNoFootedSeatGrab, 0x00559904, 0x1064);
AT(pad_00559918, 0x00559918, 0x1078);
AT(sOneHandedSeatGrab, 0x0055991c, 0x107c);
AT(pad_00559934, 0x00559934, 0x1094);
AT(sDoublePegGrab, 0x00559938, 0x1098);
AT(pad_00559948, 0x00559948, 0x10a8);
AT(sRocketAir, 0x0055994c, 0x10ac);
AT(pad_00559958, 0x00559958, 0x10b8);
AT(sOneHandXUp, 0x0055995c, 0x10bc);
AT(sOneHandOneFooter, 0x0055996c, 0x10cc);
AT(pad_00559980, 0x00559980, 0x10e0);
AT(sNothing, 0x00559984, 0x10e4);
AT(pad_0055998c, 0x0055998c, 0x10ec);
AT(sNoHander, 0x00559990, 0x10f0);
AT(sNoFooter, 0x0055999c, 0x10fc);
AT(sSeatGrab, 0x005599a8, 0x1108);
AT(sSuicideNoHander, 0x005599b4, 0x1114);
AT(pad_005599c8, 0x005599c8, 0x1128);
AT(sXDown, 0x005599cc, 0x112c);
AT(sXUp, 0x005599d4, 0x1134);
AT(sOneHander, 0x005599dc, 0x113c);
AT(pad_005599e8, 0x005599e8, 0x1148);
AT(sOneFooter, 0x005599ec, 0x114c);
AT(pad_005599f8, 0x005599f8, 0x1158);
AT(sBarspin, 0x005599fc, 0x115c);
AT(pad_00559a04, 0x00559a04, 0x1164);
AT(sNoseWheelie, 0x00559a08, 0x1168);
AT(sManual, 0x00559a18, 0x1178);
AT(sWallTap, 0x00559a20, 0x1180);
AT(sFastPlant, 0x00559a2c, 0x118c);
AT(pad_00559a38, 0x00559a38, 0x1198);
AT(sDisaster, 0x00559a3c, 0x119c);
AT(sTailTap, 0x00559a48, 0x11a8);
AT(sNosepick, 0x00559a54, 0x11b4);
AT(sIcepickStall, 0x00559a60, 0x11c0);
AT(sToothpickStall, 0x00559a70, 0x11d0);
AT(pad_00559a80, 0x00559a80, 0x11e0);
AT(sDoublePegStall, 0x00559a84, 0x11e4);
AT(sSprocketGrind, 0x00559a98, 0x11f8);
AT(pad_00559aa8, 0x00559aa8, 0x1208);
AT(sCrookedGrind, 0x00559aac, 0x120c);
AT(sLucEGrind, 0x00559abc, 0x121c);
AT(pad_00559ac8, 0x00559ac8, 0x1228);
AT(sLipSlide, 0x00559acc, 0x122c);
AT(sIcepickGrind, 0x00559ad8, 0x1238);
AT(sToothpickGrind, 0x00559ae8, 0x1248);
AT(pad_00559af8, 0x00559af8, 0x1258);
AT(sFeebleGrind, 0x00559afc, 0x125c);
AT(sSmithGrind, 0x00559b0c, 0x126c);
AT(pad_00559b18, 0x00559b18, 0x1278);
AT(sDoublePegGrind, 0x00559b1c, 0x127c);
AT(sSupermanIndian, 0x00559b30, 0x1290);
AT(pad_00559b40, 0x00559b40, 0x12a0);
AT(sCandyBarLookdown, 0x00559b44, 0x12a4);
AT(pad_00559b58, 0x00559b58, 0x12b8);
AT(sCanCanLookdown, 0x00559b5c, 0x12bc);
AT(sDoubleTailwhip, 0x00559b70, 0x12d0);
AT(pad_00559b80, 0x00559b80, 0x12e0);
AT(sUnTurndown, 0x00559b84, 0x12e4);
AT(pad_00559b90, 0x00559b90, 0x12f0);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gRiderTrickComboGroup, pad_00559b90) +
                       sizeof(((struct BmxGroup_gRiderTrickComboGroup *)0)->pad_00559b90) ==
                   BMX_DATA_OFF(0x10a4, 0x12f4),
               "gRiderTrickComboGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00559d04..0x00559d14.
 */
struct BmxGroup_sTurndownGroup {
    uint8_t lead_[4];   /* nothing: the members' offsets keep their original alignment */
    char sTurndown[12]; /* 0x00559d04 */
    void *p_00559d10;   /* 0x00559d10 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_sTurndownGroup, member) == (addr) - 0x00559d00, #member)
AT(sTurndown, 0x00559d04);
AT(p_00559d10, 0x00559d10);
#undef AT
_Static_assert(offsetof(struct BmxGroup_sTurndownGroup, p_00559d10) +
                       sizeof(((struct BmxGroup_sTurndownGroup *)0)->p_00559d10) ==
                   BMX_DATA_OFF(0x14, 0x18),
               "sTurndownGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0055a450..0x0055a620.
 */
struct BmxGroup_gKbLayoutLabelsGroup {
    KeyLabelTable gKbLayoutLabels;   /* 0x0055a450 */
    unsigned char pad_0055a5b8[104]; /* 0x0055a5b8 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gKbLayoutLabelsGroup, member) ==                                 \
                       BMX_DATA_OFF((addr) - 0x0055a450, off64),                                             \
                   #member)
AT(gKbLayoutLabels, 0x0055a450, 0x0);
AT(pad_0055a5b8, 0x0055a5b8, 0x2d0);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gKbLayoutLabelsGroup, pad_0055a5b8) +
                       sizeof(((struct BmxGroup_gKbLayoutLabelsGroup *)0)->pad_0055a5b8) ==
                   BMX_DATA_OFF(0x1d0, 0x338),
               "gKbLayoutLabelsGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0055aaf0..0x0055ab10.
 */
struct BmxGroup_gCaptionOrdersGroup {
    SelCaptionOrders gCaptionOrders; /* 0x0055aaf0 */
    uint32_t d_0055ab08[2];          /* 0x0055ab08 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gCaptionOrdersGroup, member) == (addr) - 0x0055aaf0, #member)
AT(gCaptionOrders, 0x0055aaf0);
AT(d_0055ab08, 0x0055ab08);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gCaptionOrdersGroup, d_0055ab08) +
                       sizeof(((struct BmxGroup_gCaptionOrdersGroup *)0)->d_0055ab08) ==
                   0x20,
               "gCaptionOrdersGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0055ad30..0x0055af34.
 */
struct BmxGroup_gBikeCaptionsGroup {
    SelCaptionPtr60 gBikeCaptions;   /* 0x0055ad30 */
    unsigned char pad_0055ae20[276]; /* 0x0055ae20 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gBikeCaptionsGroup, member) ==                                   \
                       BMX_DATA_OFF((addr) - 0x0055ad30, off64),                                             \
                   #member)
AT(gBikeCaptions, 0x0055ad30, 0x0);
AT(pad_0055ae20, 0x0055ae20, 0x1e0);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gBikeCaptionsGroup, pad_0055ae20) +
                       sizeof(((struct BmxGroup_gBikeCaptionsGroup *)0)->pad_0055ae20) ==
                   BMX_DATA_OFF(0x204, 0x2f4),
               "gBikeCaptionsGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0055b138..0x0055b24c.
 */
struct BmxGroup_gInfoLineIdsGroup {
    uint8_t lead_[8];                /* nothing: the members' offsets keep their original alignment */
    InfoLineTable gInfoLineIds;      /* 0x0055b138 */
    unsigned char pad_0055b1c8[132]; /* 0x0055b1c8 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gInfoLineIdsGroup, member) == (addr) - 0x0055b130, #member)
AT(gInfoLineIds, 0x0055b138);
AT(pad_0055b1c8, 0x0055b1c8);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gInfoLineIdsGroup, pad_0055b1c8) +
                       sizeof(((struct BmxGroup_gInfoLineIdsGroup *)0)->pad_0055b1c8) ==
                   0x11c,
               "gInfoLineIdsGroup");

/* The type is local to bmx/com/menu/frontend_extras.c. */
typedef char BmxType_gCreditsMovie[22];

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0055b960..0x0055b97c.
 */
struct BmxGroup_gZoverGlyphGroup {
    ZOverGlyphRecord gZoverGlyph; /* 0x0055b960 */
    uint32_t d_0055b974[2];       /* 0x0055b974 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gZoverGlyphGroup, member) == (addr) - 0x0055b960, #member)
AT(gZoverGlyph, 0x0055b960);
AT(d_0055b974, 0x0055b974);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZoverGlyphGroup, d_0055b974) +
                       sizeof(((struct BmxGroup_gZoverGlyphGroup *)0)->d_0055b974) ==
                   0x1c,
               "gZoverGlyphGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0055ba04..0x0055ba30.
 */
struct BmxGroup_gNgAssetsDirGroup {
    uint8_t lead_[4];              /* nothing: the members' offsets keep their original alignment */
    const char *gNgAssetsDir;      /* 0x0055ba04 */
    void *p_0055ba08[8];           /* 0x0055ba08 */
    unsigned char pad_0055ba28[8]; /* 0x0055ba28 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gNgAssetsDirGroup, member) ==                                    \
                       BMX_DATA_OFF((addr) - 0x0055ba00, off64),                                             \
                   #member)
AT(gNgAssetsDir, 0x0055ba04, 0x8);
AT(p_0055ba08, 0x0055ba08, 0x10);
AT(pad_0055ba28, 0x0055ba28, 0x50);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gNgAssetsDirGroup, pad_0055ba28) +
                       sizeof(((struct BmxGroup_gNgAssetsDirGroup *)0)->pad_0055ba28) ==
                   BMX_DATA_OFF(0x30, 0x58),
               "gNgAssetsDirGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0055bb98..0x0055bcf0.
 */
struct BmxGroup_gNgRamDiskGroup {
    uint8_t lead_[8];             /* nothing: the members' offsets keep their original alignment */
    uint32_t gNgKeyMaps[40];      /* 0x0055bb98 */
    intptr_t gNgRamDisk;          /* 0x0055bc38 */
    int gNgLegalPending;          /* 0x0055bc3c */
    const char *gNgWrapName;      /* 0x0055bc40 */
    void *gNgLevelNames[14];      /* 0x0055bc44 */
    uint32_t gNgDebugFaceColor;   /* 0x0055bc7c */
    uint32_t gNgDebugEdgeColor;   /* 0x0055bc80 */
    uint32_t gNgDebugTaggedColor; /* 0x0055bc84 */
    uint32_t gNgDebugRollerColor; /* 0x0055bc88 */
    uint8_t gNgMainLightColor;    /* 0x0055bc8c */
    unsigned char d_0055bc8d[3];  /* 0x0055bc8d */
    struct {
        struct {
            float x;
            float y;
            float z;
        } v;
        uint32_t pad;
    } gNgLightDir[3]; /* 0x0055bc90; the type is local to bmx/com/zge/ngif/ngapp.c */
    struct {
        struct {
            float x;
            float y;
            float z;
        } v;
        uint32_t pad;
    } gNgLightRef[3]; /* 0x0055bcc0; the type is local to bmx/com/zge/ngif/ngapp.c */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gNgRamDiskGroup, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x0055bb90, off64),                                             \
                   #member)
AT(gNgKeyMaps, 0x0055bb98, 0x8);
AT(gNgRamDisk, 0x0055bc38, 0xa8);
AT(gNgLegalPending, 0x0055bc3c, 0xb0);
AT(gNgWrapName, 0x0055bc40, 0xb8);
AT(gNgLevelNames, 0x0055bc44, 0xc0);
AT(gNgDebugFaceColor, 0x0055bc7c, 0x130);
AT(gNgDebugEdgeColor, 0x0055bc80, 0x134);
AT(gNgDebugTaggedColor, 0x0055bc84, 0x138);
AT(gNgDebugRollerColor, 0x0055bc88, 0x13c);
AT(gNgMainLightColor, 0x0055bc8c, 0x140);
AT(d_0055bc8d, 0x0055bc8d, 0x141);
AT(gNgLightDir, 0x0055bc90, 0x144);
AT(gNgLightRef, 0x0055bcc0, 0x174);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gNgRamDiskGroup, gNgLightRef) +
                       sizeof(((struct BmxGroup_gNgRamDiskGroup *)0)->gNgLightRef) ==
                   BMX_DATA_OFF(0x160, 0x1a4),
               "gNgRamDiskGroup");

/* The type is local to bmx/com/zge/ngif/ngapp.c. */
typedef uint8_t BmxType_gNgLightColors[3][4];

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0055bcfc..0x0055bd04.
 */
struct BmxGroup_gNgSpotMainColorGroup {
    uint8_t lead_[12];           /* nothing: the members' offsets keep their original alignment */
    uint8_t gNgSpotMainColor;    /* 0x0055bcfc */
    unsigned char d_0055bcfd[7]; /* 0x0055bcfd */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gNgSpotMainColorGroup, member) == (addr) - 0x0055bcf0, #member)
AT(gNgSpotMainColor, 0x0055bcfc);
AT(d_0055bcfd, 0x0055bcfd);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gNgSpotMainColorGroup, d_0055bcfd) +
                       sizeof(((struct BmxGroup_gNgSpotMainColorGroup *)0)->d_0055bcfd) ==
                   0x14,
               "gNgSpotMainColorGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0055bd0c..0x0055bd5c.
 */
struct BmxGroup_gFxFpsGroup {
    uint8_t lead_[12];              /* nothing: the members' offsets keep their original alignment */
    float gFxFps;                   /* 0x0055bd0c */
    unsigned char pad_0055bd10[76]; /* 0x0055bd10 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gFxFpsGroup, member) == (addr) - 0x0055bd00, #member)
AT(gFxFps, 0x0055bd0c);
AT(pad_0055bd10, 0x0055bd10);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gFxFpsGroup, pad_0055bd10) +
                       sizeof(((struct BmxGroup_gFxFpsGroup *)0)->pad_0055bd10) ==
                   0x5c,
               "gFxFpsGroup");

/* The type is local to bmx/com/zge/ngif/ngiffx.c. */
typedef void *BmxType_gFxSkidFrameNames[16];

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0055c128..0x0055c1a0.
 */
struct BmxGroup_gFxSparkDefaultGroup {
    uint8_t lead_[8];          /* nothing: the members' offsets keep their original alignment */
    float gFxSparkDefault[16]; /* 0x0055c128, also gFxShardDefault; the type is local to
                                  bmx/com/zge/ngif/ngiffx.c */
    uint32_t d_0055c168[14];   /* 0x0055c168 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gFxSparkDefaultGroup, member) == (addr) - 0x0055c120, #member)
AT(gFxSparkDefault, 0x0055c128);
AT(d_0055c168, 0x0055c168);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gFxSparkDefaultGroup, d_0055c168) +
                       sizeof(((struct BmxGroup_gFxSparkDefaultGroup *)0)->d_0055c168) ==
                   0x80,
               "gFxSparkDefaultGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0055c1a0..0x0055c220.
 */
struct BmxGroup_gFxDustDefaultGroup {
    float gFxDustDefault[16]; /* 0x0055c1a0, also gFxPosterDefault; the type is local to
                                 bmx/com/zge/ngif/ngiffx.c */
    uint32_t d_0055c1e0[16];  /* 0x0055c1e0 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gFxDustDefaultGroup, member) == (addr) - 0x0055c1a0, #member)
AT(gFxDustDefault, 0x0055c1a0);
AT(d_0055c1e0, 0x0055c1e0);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gFxDustDefaultGroup, d_0055c1e0) +
                       sizeof(((struct BmxGroup_gFxDustDefaultGroup *)0)->d_0055c1e0) ==
                   0x80,
               "gFxDustDefaultGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0055c220..0x0055c2b0.
 */
struct BmxGroup_gFxRippleDefaultGroup {
    float gFxRippleDefault[16]; /* 0x0055c220, also gFxLightningDefault; the type is local to
                                   bmx/com/zge/ngif/ngiffx.c */
    uint32_t d_0055c260[6];     /* 0x0055c260 */
    ZfxEffect gFxSpark;         /* 0x0055c278 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gFxRippleDefaultGroup, member) == (addr) - 0x0055c220, #member)
AT(gFxRippleDefault, 0x0055c220);
AT(d_0055c260, 0x0055c260);
AT(gFxSpark, 0x0055c278);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gFxRippleDefaultGroup, gFxSpark) +
                       sizeof(((struct BmxGroup_gFxRippleDefaultGroup *)0)->gFxSpark) ==
                   BMX_DATA_OFF(0x90, 0xc0),
               "gFxRippleDefaultGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0055c400..0x0055c53c.
 */
struct BmxGroup_gFxSparkCreateDefaultGroup {
    float gFxSparkCreateDefault[16]; /* 0x0055c400, also gFxSplashCreateDefault; the type is local to
                                        bmx/com/zge/ngif/ngiffx.c */
    uint32_t d_0055c440[12];         /* 0x0055c440 */
    float gFxGravityParams[16];      /* 0x0055c470; the type is local to bmx/com/zge/ngif/ngiffx.c */
    unsigned char pad_0055c4b0[140]; /* 0x0055c4b0 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gFxSparkCreateDefaultGroup, member) == (addr) - 0x0055c400,      \
                   #member)
AT(gFxSparkCreateDefault, 0x0055c400);
AT(d_0055c440, 0x0055c440);
AT(gFxGravityParams, 0x0055c470);
AT(pad_0055c4b0, 0x0055c4b0);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gFxSparkCreateDefaultGroup, pad_0055c4b0) +
                       sizeof(((struct BmxGroup_gFxSparkCreateDefaultGroup *)0)->pad_0055c4b0) ==
                   0x13c,
               "gFxSparkCreateDefaultGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0055d504..0x0055d51c.
 */
struct BmxGroup_gZcollideBarEdgeGroup {
    uint8_t lead_[4];                 /* nothing: the members' offsets keep their original alignment */
    ZcollideBoxEdge gZcollideBarEdge; /* 0x0055d504 */
    unsigned char pad_0055d507[21];   /* 0x0055d507 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gZcollideBarEdgeGroup, member) == (addr) - 0x0055d500, #member)
AT(gZcollideBarEdge, 0x0055d504);
AT(pad_0055d507, 0x0055d507);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZcollideBarEdgeGroup, pad_0055d507) +
                       sizeof(((struct BmxGroup_gZcollideBarEdgeGroup *)0)->pad_0055d507) ==
                   0x1c,
               "gZcollideBarEdgeGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0055dc50..0x0055e8b2.
 */
struct BmxGroup_gZrollerRiderNamesGroup {
    ZrollerNameTable gZrollerRiderNames;               /* 0x0055dc50 */
    void *p_0055dc54[11];                              /* 0x0055dc54 */
    ZrollerNameTable gZrollerBikeNames;                /* 0x0055dc80 */
    void *p_0055dc84[48];                              /* 0x0055dc84 */
    unsigned char pad_0055dd44[48];                    /* 0x0055dd44 */
    char sXXbike01[12];                                /* 0x0055dd74 */
    char sABbike04[12];                                /* 0x0055dd80 */
    char sABbike03[12];                                /* 0x0055dd8c */
    char sABbike02[12];                                /* 0x0055dd98 */
    char sABbike01[12];                                /* 0x0055dda4 */
    char sSJbike04[12];                                /* 0x0055ddb0 */
    char sSJbike03[12];                                /* 0x0055ddbc */
    char sSJbike02[12];                                /* 0x0055ddc8 */
    char sSJbike01[12];                                /* 0x0055ddd4 */
    char sJGbike04[12];                                /* 0x0055dde0 */
    char sJGbike03[12];                                /* 0x0055ddec */
    char sJGbike02[12];                                /* 0x0055ddf8 */
    char sJGbike01[12];                                /* 0x0055de04 */
    char sLRbike04[12];                                /* 0x0055de10 */
    char sLRbike03[12];                                /* 0x0055de1c */
    char sLRbike02[12];                                /* 0x0055de28 */
    char sLRbike01[12];                                /* 0x0055de34 */
    char sSBbike04[12];                                /* 0x0055de40 */
    char sSBbike03[12];                                /* 0x0055de4c */
    char sSBbike02[12];                                /* 0x0055de58 */
    char sSBbike01[12];                                /* 0x0055de64 */
    char sKHbike04[12];                                /* 0x0055de70 */
    char sKHbike03[12];                                /* 0x0055de7c */
    char sKHbike02[12];                                /* 0x0055de88 */
    char sKHbike01[12];                                /* 0x0055de94 */
    char sTMibike04[12];                               /* 0x0055dea0 */
    char sTMibike03[12];                               /* 0x0055deac */
    char sTMibike02[12];                               /* 0x0055deb8 */
    char sTMibike01[12];                               /* 0x0055dec4 */
    char sCKbike04[12];                                /* 0x0055ded0 */
    char sCKbike03[12];                                /* 0x0055dedc */
    char sCKbike02[12];                                /* 0x0055dee8 */
    char sCKbike01[12];                                /* 0x0055def4 */
    char sMLbike04[12];                                /* 0x0055df00 */
    char sMLbike03[12];                                /* 0x0055df0c */
    char sMLbike02[12];                                /* 0x0055df18 */
    char sMLbike01[12];                                /* 0x0055df24 */
    char sTMcbike04[12];                               /* 0x0055df30 */
    char sTMcbike03[12];                               /* 0x0055df3c */
    char sTMcbike02[12];                               /* 0x0055df48 */
    char sTMcbike01[12];                               /* 0x0055df54 */
    char sRNbike04[12];                                /* 0x0055df60 */
    char sRNbike03[12];                                /* 0x0055df6c */
    char sRNbike02[12];                                /* 0x0055df78 */
    char sRNbike01[12];                                /* 0x0055df84 */
    char sDMbike04[12];                                /* 0x0055df90 */
    char sDMbike03[12];                                /* 0x0055df9c */
    char sDMbike02[12];                                /* 0x0055dfa8 */
    char sDMbike01[12];                                /* 0x0055dfb4 */
    char sAmishBoy[12];                                /* 0x0055dfc0 */
    unsigned char pad_0055dfcc[4];                     /* 0x0055dfcc */
    char sSlimJimGuy[16];                              /* 0x0055dfd0 */
    char sJoeyGarcia[16];                              /* 0x0055dfe0 */
    char sLeighRamsdell[16];                           /* 0x0055dff0 */
    unsigned char pad_0055e000[4];                     /* 0x0055e000 */
    char sShaunButler[16];                             /* 0x0055e004 */
    char sKenanHarkin[16];                             /* 0x0055e014 */
    char sTimMirra[12];                                /* 0x0055e024 */
    unsigned char pad_0055e030[4];                     /* 0x0055e030 */
    char sChadKagy[12];                                /* 0x0055e034 */
    unsigned char pad_0055e040[4];                     /* 0x0055e040 */
    char sMikeLaird[12];                               /* 0x0055e044 */
    unsigned char pad_0055e050[4];                     /* 0x0055e050 */
    char sTroyMcMurray[16];                            /* 0x0055e054 */
    unsigned char pad_0055e064[4];                     /* 0x0055e064 */
    char sRyanNyquist[16];                             /* 0x0055e068 */
    char sDaveMirra[12];                               /* 0x0055e078 */
    unsigned char pad_0055e084[4];                     /* 0x0055e084 */
    char sZROLLERFailedToIntializeZRES[36];            /* 0x0055e088 */
    unsigned char pad_0055e0ac[8];                     /* 0x0055e0ac */
    char sCZaxisBmxSrcComZgeNgIFNgIFzrollerC[44];      /* 0x0055e0b4 */
    unsigned char pad_0055e0e0[8];                     /* 0x0055e0e0 */
    char sZrollerUpdateZGL[20];                        /* 0x0055e0e8 */
    char sZrollerCreateChaser[20];                     /* 0x0055e0fc */
    unsigned char pad_0055e110[4];                     /* 0x0055e110 */
    char sZrollerLoadAssets[20];                       /* 0x0055e114 */
    unsigned char pad_0055e128[4];                     /* 0x0055e128 */
    char sRiderShadow[16];                             /* 0x0055e12c */
    char sOutOfPlayerDefs[20];                         /* 0x0055e13c */
    unsigned char pad_0055e150[4];                     /* 0x0055e150 */
    char sBikeShadow[12];                              /* 0x0055e154 */
    unsigned char pad_0055e160[4];                     /* 0x0055e160 */
    char sInvalidBikeSelected[24];                     /* 0x0055e164 */
    unsigned char pad_0055e17c[4];                     /* 0x0055e17c */
    char sZrollerCreateProp[20];                       /* 0x0055e180 */
    unsigned char pad_0055e194[4];                     /* 0x0055e194 */
    char sZrollerSetEngineState[24];                   /* 0x0055e198 */
    unsigned char pad_0055e1b0[4];                     /* 0x0055e1b0 */
    char sZrollerGetNumPlayerDefs[24];                 /* 0x0055e1b4 */
    unsigned char pad_0055e1cc[4];                     /* 0x0055e1cc */
    char sZrollerLoadMdlPlayerCB[24];                  /* 0x0055e1d0 */
    unsigned char pad_0055e1e8[4];                     /* 0x0055e1e8 */
    char sZrollerIsPlayerClose[24];                    /* 0x0055e1ec */
    unsigned char pad_0055e204[4];                     /* 0x0055e204 */
    char sChangePlayerModeLook[24];                    /* 0x0055e208 */
    unsigned char pad_0055e220[4];                     /* 0x0055e220 */
    char sZrollerAddPlayerMatsToLoadList[32];          /* 0x0055e224 */
    unsigned char pad_0055e244[8];                     /* 0x0055e244 */
    char sZrollerChangePlayerSponsor[28];              /* 0x0055e24c */
    unsigned char pad_0055e268[4];                     /* 0x0055e268 */
    char sZrollerchangeboardsponsor[28];               /* 0x0055e26c */
    unsigned char pad_0055e288[4];                     /* 0x0055e288 */
    char sZrollerLoadMdlGenericCB[24];                 /* 0x0055e28c */
    unsigned char pad_0055e2a4[4];                     /* 0x0055e2a4 */
    char sZrollerLoadMdlCommon[24];                    /* 0x0055e2a8 */
    unsigned char pad_0055e2c0[4];                     /* 0x0055e2c0 */
    char sLoadMdlCommonMat[20];                        /* 0x0055e2c4 */
    char sZrollerProcessModelFlags[28];                /* 0x0055e2d8 */
    unsigned char pad_0055e2f4[4];                     /* 0x0055e2f4 */
    char sZrollerProcessActorDrawFlags[32];            /* 0x0055e2f8 */
    unsigned char pad_0055e318[4];                     /* 0x0055e318 */
    char sZrollerSetAnimMaterialPalette[32];           /* 0x0055e31c */
    unsigned char pad_0055e33c[4];                     /* 0x0055e33c */
    char sZrollerInitializeLoadingScreen[32];          /* 0x0055e340 */
    unsigned char pad_0055e360[8];                     /* 0x0055e360 */
    char sZrollerShutdownLoadingScreen[32];            /* 0x0055e368 */
    unsigned char pad_0055e388[4];                     /* 0x0055e388 */
    char sZrollerDisplayLoadingScreen[28];             /* 0x0055e38c */
    unsigned char pad_0055e3a8[8];                     /* 0x0055e3a8 */
    char sZrollerSetLocal[16];                         /* 0x0055e3b0 */
    unsigned char pad_0055e3c0[4];                     /* 0x0055e3c0 */
    char sZrollerLoadModule[20];                       /* 0x0055e3c4 */
    unsigned char pad_0055e3d8[4];                     /* 0x0055e3d8 */
    char sLoadCodeModule[16];                          /* 0x0055e3dc */
    unsigned char pad_0055e3ec[4];                     /* 0x0055e3ec */
    char sZrollerAllocMaterials[24];                   /* 0x0055e3f0 */
    unsigned char pad_0055e408[4];                     /* 0x0055e408 */
    char sZrollerAddGenericMaterialsToLoad[36];        /* 0x0055e40c */
    unsigned char pad_0055e430[4];                     /* 0x0055e430 */
    char sZrollerInitializeMaps[24];                   /* 0x0055e434 */
    unsigned char pad_0055e44c[4];                     /* 0x0055e44c */
    char sZrollerSetMap[16];                           /* 0x0055e450 */
    char sZrollerLoadMaps[16];                         /* 0x0055e460 */
    unsigned char pad_0055e470[4];                     /* 0x0055e470 */
    char sZrollerLoadMatTerrainFirstCB[32];            /* 0x0055e474 */
    unsigned char pad_0055e494[4];                     /* 0x0055e494 */
    char sZrollerLoadMatTerrainLastCB[28];             /* 0x0055e498 */
    unsigned char pad_0055e4b4[8];                     /* 0x0055e4b4 */
    char sZrollerAssociateMapMaterials[32];            /* 0x0055e4bc */
    unsigned char pad_0055e4dc[4];                     /* 0x0055e4dc */
    char sZroolerAssociateMaterialLlljlkfsdjflsjd[44]; /* 0x0055e4e0 */
    unsigned char pad_0055e50c[8];                     /* 0x0055e50c */
    char sAssociateMaterialPalAnimation[36];           /* 0x0055e514 */
    unsigned char pad_0055e538[4];                     /* 0x0055e538 */
    char sZrollerGetPlayerStartPosHARDCODED[36];       /* 0x0055e53c */
    unsigned char pad_0055e560[8];                     /* 0x0055e560 */
    char sPlayerStartPointNotFoundUsingDefaultVal[52]; /* 0x0055e568 */
    unsigned char pad_0055e59c[12];                    /* 0x0055e59c */
    char sPlayerT[8];                                  /* 0x0055e5a8 */
    unsigned char pad_0055e5b0[4];                     /* 0x0055e5b0 */
    char sPlayer[8];                                   /* 0x0055e5b4 */
    char sZrollerGetPlayerRestartPos[28];              /* 0x0055e5bc */
    unsigned char pad_0055e5d8[4];                     /* 0x0055e5d8 */
    char sZrollerInitPropLoadList[24];                 /* 0x0055e5dc */
    unsigned char pad_0055e5f4[4];                     /* 0x0055e5f4 */
    char sZrollerAddPropToLoadList[28];                /* 0x0055e5f8 */
    unsigned char pad_0055e614[4];                     /* 0x0055e614 */
    char sCreateProps[16];                             /* 0x0055e618 */
    char sZrollerLoadPropMaterials[28];                /* 0x0055e628 */
    unsigned char pad_0055e644[4];                     /* 0x0055e644 */
    char sZrollerCreatePropActor[24];                  /* 0x0055e648 */
    unsigned char pad_0055e660[4];                     /* 0x0055e660 */
    char sZrollerDestroyActor[20];                     /* 0x0055e664 */
    unsigned char pad_0055e678[4];                     /* 0x0055e678 */
    char sZrollerGetAsyncKeyState[24];                 /* 0x0055e67c */
    unsigned char pad_0055e694[4];                     /* 0x0055e694 */
    char sZrollerSelectMap[20];                        /* 0x0055e698 */
    char sZrollerSelectPlayer[20];                     /* 0x0055e6ac */
    unsigned char pad_0055e6c0[4];                     /* 0x0055e6c0 */
    char sZrollerSelectBikeNum[24];                    /* 0x0055e6c4 */
    unsigned char pad_0055e6dc[4];                     /* 0x0055e6dc */
    char sSetFogDistance[20];                          /* 0x0055e6e0 */
    char sGetFogDistance[20];                          /* 0x0055e6f4 */
    char sZrollerDisableFog[20];                       /* 0x0055e708 */
    unsigned char pad_0055e71c[4];                     /* 0x0055e71c */
    char sZrollerSetFogColor[20];                      /* 0x0055e720 */
    unsigned char pad_0055e734[4];                     /* 0x0055e734 */
    char sZrollerReplaceMaterial[24];                  /* 0x0055e738 */
    unsigned char pad_0055e750[4];                     /* 0x0055e750 */
    char sZrollerSetShadeMode[20];                     /* 0x0055e754 */
    unsigned char pad_0055e768[4];                     /* 0x0055e768 */
    char sZrollerStartCrossFade[24];                   /* 0x0055e76c */
    unsigned char pad_0055e784[4];                     /* 0x0055e784 */
    char sZrollerGetSceneStateNODATA[32];              /* 0x0055e788 */
    unsigned char pad_0055e7a8[4];                     /* 0x0055e7a8 */
    char sFreeSceneState[16];                          /* 0x0055e7ac */
    unsigned char pad_0055e7bc[4];                     /* 0x0055e7bc */
    char sSetSceneState[16];                           /* 0x0055e7c0 */
    char sZrollerSelectLook[20];                       /* 0x0055e7d0 */
    unsigned char pad_0055e7e4[4];                     /* 0x0055e7e4 */
    char sRegisteredTooManyDrawCallbacksToZroller[48]; /* 0x0055e7e8 */
    unsigned char pad_0055e818[8];                     /* 0x0055e818 */
    uint16_t gSemitones[73]; /* 0x0055e820; the type is local to bmx/pc/zge/sound/winchan.c */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gZrollerRiderNamesGroup, member) ==                              \
                       BMX_DATA_OFF((addr) - 0x0055dc50, off64),                                             \
                   #member)
AT(gZrollerRiderNames, 0x0055dc50, 0x0);
AT(p_0055dc54, 0x0055dc54, 0x8);
AT(gZrollerBikeNames, 0x0055dc80, 0x60);
AT(p_0055dc84, 0x0055dc84, 0x68);
AT(pad_0055dd44, 0x0055dd44, 0x1e8);
AT(sXXbike01, 0x0055dd74, 0x218);
AT(sABbike04, 0x0055dd80, 0x224);
AT(sABbike03, 0x0055dd8c, 0x230);
AT(sABbike02, 0x0055dd98, 0x23c);
AT(sABbike01, 0x0055dda4, 0x248);
AT(sSJbike04, 0x0055ddb0, 0x254);
AT(sSJbike03, 0x0055ddbc, 0x260);
AT(sSJbike02, 0x0055ddc8, 0x26c);
AT(sSJbike01, 0x0055ddd4, 0x278);
AT(sJGbike04, 0x0055dde0, 0x284);
AT(sJGbike03, 0x0055ddec, 0x290);
AT(sJGbike02, 0x0055ddf8, 0x29c);
AT(sJGbike01, 0x0055de04, 0x2a8);
AT(sLRbike04, 0x0055de10, 0x2b4);
AT(sLRbike03, 0x0055de1c, 0x2c0);
AT(sLRbike02, 0x0055de28, 0x2cc);
AT(sLRbike01, 0x0055de34, 0x2d8);
AT(sSBbike04, 0x0055de40, 0x2e4);
AT(sSBbike03, 0x0055de4c, 0x2f0);
AT(sSBbike02, 0x0055de58, 0x2fc);
AT(sSBbike01, 0x0055de64, 0x308);
AT(sKHbike04, 0x0055de70, 0x314);
AT(sKHbike03, 0x0055de7c, 0x320);
AT(sKHbike02, 0x0055de88, 0x32c);
AT(sKHbike01, 0x0055de94, 0x338);
AT(sTMibike04, 0x0055dea0, 0x344);
AT(sTMibike03, 0x0055deac, 0x350);
AT(sTMibike02, 0x0055deb8, 0x35c);
AT(sTMibike01, 0x0055dec4, 0x368);
AT(sCKbike04, 0x0055ded0, 0x374);
AT(sCKbike03, 0x0055dedc, 0x380);
AT(sCKbike02, 0x0055dee8, 0x38c);
AT(sCKbike01, 0x0055def4, 0x398);
AT(sMLbike04, 0x0055df00, 0x3a4);
AT(sMLbike03, 0x0055df0c, 0x3b0);
AT(sMLbike02, 0x0055df18, 0x3bc);
AT(sMLbike01, 0x0055df24, 0x3c8);
AT(sTMcbike04, 0x0055df30, 0x3d4);
AT(sTMcbike03, 0x0055df3c, 0x3e0);
AT(sTMcbike02, 0x0055df48, 0x3ec);
AT(sTMcbike01, 0x0055df54, 0x3f8);
AT(sRNbike04, 0x0055df60, 0x404);
AT(sRNbike03, 0x0055df6c, 0x410);
AT(sRNbike02, 0x0055df78, 0x41c);
AT(sRNbike01, 0x0055df84, 0x428);
AT(sDMbike04, 0x0055df90, 0x434);
AT(sDMbike03, 0x0055df9c, 0x440);
AT(sDMbike02, 0x0055dfa8, 0x44c);
AT(sDMbike01, 0x0055dfb4, 0x458);
AT(sAmishBoy, 0x0055dfc0, 0x464);
AT(pad_0055dfcc, 0x0055dfcc, 0x470);
AT(sSlimJimGuy, 0x0055dfd0, 0x474);
AT(sJoeyGarcia, 0x0055dfe0, 0x484);
AT(sLeighRamsdell, 0x0055dff0, 0x494);
AT(pad_0055e000, 0x0055e000, 0x4a4);
AT(sShaunButler, 0x0055e004, 0x4a8);
AT(sKenanHarkin, 0x0055e014, 0x4b8);
AT(sTimMirra, 0x0055e024, 0x4c8);
AT(pad_0055e030, 0x0055e030, 0x4d4);
AT(sChadKagy, 0x0055e034, 0x4d8);
AT(pad_0055e040, 0x0055e040, 0x4e4);
AT(sMikeLaird, 0x0055e044, 0x4e8);
AT(pad_0055e050, 0x0055e050, 0x4f4);
AT(sTroyMcMurray, 0x0055e054, 0x4f8);
AT(pad_0055e064, 0x0055e064, 0x508);
AT(sRyanNyquist, 0x0055e068, 0x50c);
AT(sDaveMirra, 0x0055e078, 0x51c);
AT(pad_0055e084, 0x0055e084, 0x528);
AT(sZROLLERFailedToIntializeZRES, 0x0055e088, 0x52c);
AT(pad_0055e0ac, 0x0055e0ac, 0x550);
AT(sCZaxisBmxSrcComZgeNgIFNgIFzrollerC, 0x0055e0b4, 0x558);
AT(pad_0055e0e0, 0x0055e0e0, 0x584);
AT(sZrollerUpdateZGL, 0x0055e0e8, 0x58c);
AT(sZrollerCreateChaser, 0x0055e0fc, 0x5a0);
AT(pad_0055e110, 0x0055e110, 0x5b4);
AT(sZrollerLoadAssets, 0x0055e114, 0x5b8);
AT(pad_0055e128, 0x0055e128, 0x5cc);
AT(sRiderShadow, 0x0055e12c, 0x5d0);
AT(sOutOfPlayerDefs, 0x0055e13c, 0x5e0);
AT(pad_0055e150, 0x0055e150, 0x5f4);
AT(sBikeShadow, 0x0055e154, 0x5f8);
AT(pad_0055e160, 0x0055e160, 0x604);
AT(sInvalidBikeSelected, 0x0055e164, 0x608);
AT(pad_0055e17c, 0x0055e17c, 0x620);
AT(sZrollerCreateProp, 0x0055e180, 0x624);
AT(pad_0055e194, 0x0055e194, 0x638);
AT(sZrollerSetEngineState, 0x0055e198, 0x63c);
AT(pad_0055e1b0, 0x0055e1b0, 0x654);
AT(sZrollerGetNumPlayerDefs, 0x0055e1b4, 0x658);
AT(pad_0055e1cc, 0x0055e1cc, 0x670);
AT(sZrollerLoadMdlPlayerCB, 0x0055e1d0, 0x674);
AT(pad_0055e1e8, 0x0055e1e8, 0x68c);
AT(sZrollerIsPlayerClose, 0x0055e1ec, 0x690);
AT(pad_0055e204, 0x0055e204, 0x6a8);
AT(sChangePlayerModeLook, 0x0055e208, 0x6ac);
AT(pad_0055e220, 0x0055e220, 0x6c4);
AT(sZrollerAddPlayerMatsToLoadList, 0x0055e224, 0x6c8);
AT(pad_0055e244, 0x0055e244, 0x6e8);
AT(sZrollerChangePlayerSponsor, 0x0055e24c, 0x6f0);
AT(pad_0055e268, 0x0055e268, 0x70c);
AT(sZrollerchangeboardsponsor, 0x0055e26c, 0x710);
AT(pad_0055e288, 0x0055e288, 0x72c);
AT(sZrollerLoadMdlGenericCB, 0x0055e28c, 0x730);
AT(pad_0055e2a4, 0x0055e2a4, 0x748);
AT(sZrollerLoadMdlCommon, 0x0055e2a8, 0x74c);
AT(pad_0055e2c0, 0x0055e2c0, 0x764);
AT(sLoadMdlCommonMat, 0x0055e2c4, 0x768);
AT(sZrollerProcessModelFlags, 0x0055e2d8, 0x77c);
AT(pad_0055e2f4, 0x0055e2f4, 0x798);
AT(sZrollerProcessActorDrawFlags, 0x0055e2f8, 0x79c);
AT(pad_0055e318, 0x0055e318, 0x7bc);
AT(sZrollerSetAnimMaterialPalette, 0x0055e31c, 0x7c0);
AT(pad_0055e33c, 0x0055e33c, 0x7e0);
AT(sZrollerInitializeLoadingScreen, 0x0055e340, 0x7e4);
AT(pad_0055e360, 0x0055e360, 0x804);
AT(sZrollerShutdownLoadingScreen, 0x0055e368, 0x80c);
AT(pad_0055e388, 0x0055e388, 0x82c);
AT(sZrollerDisplayLoadingScreen, 0x0055e38c, 0x830);
AT(pad_0055e3a8, 0x0055e3a8, 0x84c);
AT(sZrollerSetLocal, 0x0055e3b0, 0x854);
AT(pad_0055e3c0, 0x0055e3c0, 0x864);
AT(sZrollerLoadModule, 0x0055e3c4, 0x868);
AT(pad_0055e3d8, 0x0055e3d8, 0x87c);
AT(sLoadCodeModule, 0x0055e3dc, 0x880);
AT(pad_0055e3ec, 0x0055e3ec, 0x890);
AT(sZrollerAllocMaterials, 0x0055e3f0, 0x894);
AT(pad_0055e408, 0x0055e408, 0x8ac);
AT(sZrollerAddGenericMaterialsToLoad, 0x0055e40c, 0x8b0);
AT(pad_0055e430, 0x0055e430, 0x8d4);
AT(sZrollerInitializeMaps, 0x0055e434, 0x8d8);
AT(pad_0055e44c, 0x0055e44c, 0x8f0);
AT(sZrollerSetMap, 0x0055e450, 0x8f4);
AT(sZrollerLoadMaps, 0x0055e460, 0x904);
AT(pad_0055e470, 0x0055e470, 0x914);
AT(sZrollerLoadMatTerrainFirstCB, 0x0055e474, 0x918);
AT(pad_0055e494, 0x0055e494, 0x938);
AT(sZrollerLoadMatTerrainLastCB, 0x0055e498, 0x93c);
AT(pad_0055e4b4, 0x0055e4b4, 0x958);
AT(sZrollerAssociateMapMaterials, 0x0055e4bc, 0x960);
AT(pad_0055e4dc, 0x0055e4dc, 0x980);
AT(sZroolerAssociateMaterialLlljlkfsdjflsjd, 0x0055e4e0, 0x984);
AT(pad_0055e50c, 0x0055e50c, 0x9b0);
AT(sAssociateMaterialPalAnimation, 0x0055e514, 0x9b8);
AT(pad_0055e538, 0x0055e538, 0x9dc);
AT(sZrollerGetPlayerStartPosHARDCODED, 0x0055e53c, 0x9e0);
AT(pad_0055e560, 0x0055e560, 0xa04);
AT(sPlayerStartPointNotFoundUsingDefaultVal, 0x0055e568, 0xa0c);
AT(pad_0055e59c, 0x0055e59c, 0xa40);
AT(sPlayerT, 0x0055e5a8, 0xa4c);
AT(pad_0055e5b0, 0x0055e5b0, 0xa54);
AT(sPlayer, 0x0055e5b4, 0xa58);
AT(sZrollerGetPlayerRestartPos, 0x0055e5bc, 0xa60);
AT(pad_0055e5d8, 0x0055e5d8, 0xa7c);
AT(sZrollerInitPropLoadList, 0x0055e5dc, 0xa80);
AT(pad_0055e5f4, 0x0055e5f4, 0xa98);
AT(sZrollerAddPropToLoadList, 0x0055e5f8, 0xa9c);
AT(pad_0055e614, 0x0055e614, 0xab8);
AT(sCreateProps, 0x0055e618, 0xabc);
AT(sZrollerLoadPropMaterials, 0x0055e628, 0xacc);
AT(pad_0055e644, 0x0055e644, 0xae8);
AT(sZrollerCreatePropActor, 0x0055e648, 0xaec);
AT(pad_0055e660, 0x0055e660, 0xb04);
AT(sZrollerDestroyActor, 0x0055e664, 0xb08);
AT(pad_0055e678, 0x0055e678, 0xb1c);
AT(sZrollerGetAsyncKeyState, 0x0055e67c, 0xb20);
AT(pad_0055e694, 0x0055e694, 0xb38);
AT(sZrollerSelectMap, 0x0055e698, 0xb3c);
AT(sZrollerSelectPlayer, 0x0055e6ac, 0xb50);
AT(pad_0055e6c0, 0x0055e6c0, 0xb64);
AT(sZrollerSelectBikeNum, 0x0055e6c4, 0xb68);
AT(pad_0055e6dc, 0x0055e6dc, 0xb80);
AT(sSetFogDistance, 0x0055e6e0, 0xb84);
AT(sGetFogDistance, 0x0055e6f4, 0xb98);
AT(sZrollerDisableFog, 0x0055e708, 0xbac);
AT(pad_0055e71c, 0x0055e71c, 0xbc0);
AT(sZrollerSetFogColor, 0x0055e720, 0xbc4);
AT(pad_0055e734, 0x0055e734, 0xbd8);
AT(sZrollerReplaceMaterial, 0x0055e738, 0xbdc);
AT(pad_0055e750, 0x0055e750, 0xbf4);
AT(sZrollerSetShadeMode, 0x0055e754, 0xbf8);
AT(pad_0055e768, 0x0055e768, 0xc0c);
AT(sZrollerStartCrossFade, 0x0055e76c, 0xc10);
AT(pad_0055e784, 0x0055e784, 0xc28);
AT(sZrollerGetSceneStateNODATA, 0x0055e788, 0xc2c);
AT(pad_0055e7a8, 0x0055e7a8, 0xc4c);
AT(sFreeSceneState, 0x0055e7ac, 0xc50);
AT(pad_0055e7bc, 0x0055e7bc, 0xc60);
AT(sSetSceneState, 0x0055e7c0, 0xc64);
AT(sZrollerSelectLook, 0x0055e7d0, 0xc74);
AT(pad_0055e7e4, 0x0055e7e4, 0xc88);
AT(sRegisteredTooManyDrawCallbacksToZroller, 0x0055e7e8, 0xc8c);
AT(pad_0055e818, 0x0055e818, 0xcbc);
AT(gSemitones, 0x0055e820, 0xcc4);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZrollerRiderNamesGroup, gSemitones) +
                       sizeof(((struct BmxGroup_gZrollerRiderNamesGroup *)0)->gSemitones) ==
                   BMX_DATA_OFF(0xc62, 0xd56),
               "gZrollerRiderNamesGroup");

/* The type is local to bmx/pc/zge/sound/winchan.c. */
typedef uint16_t BmxType_gCents[100];

/* The type is local to bmx/pc/zge/sound/winchan.c. */
typedef float BmxType_gGainCurve[128];

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0055eb7c..0x0055ed7c.
 */
struct BmxGroup_gDsVolumeGroup {
    uint8_t lead_[12];        /* nothing: the members' offsets keep their original alignment */
    uint32_t gEnvelopeMs[64]; /* 0x0055eb7c */
    int16_t gDsVolume[128];   /* 0x0055ec7c; the type is local to bmx/pc/zge/sound/winstrm.c */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gDsVolumeGroup, member) == (addr) - 0x0055eb70, #member)
AT(gEnvelopeMs, 0x0055eb7c);
AT(gDsVolume, 0x0055ec7c);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gDsVolumeGroup, gDsVolume) +
                       sizeof(((struct BmxGroup_gDsVolumeGroup *)0)->gDsVolume) ==
                   0x20c,
               "gDsVolumeGroup");

/* The type is local to bmx/pc/zge/sound/winstrm.c. */
typedef int32_t BmxType_gAdpcmIndex[16];

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0055f1ec..0x0055f2d0.
 */
struct BmxGroup_gAdpcmStepGroup {
    uint8_t lead_[12];              /* nothing: the members' offsets keep their original alignment */
    uint16_t gAdpcmStep[89];        /* 0x0055f1ec; the type is local to bmx/pc/zge/sound/winstrm.c */
    unsigned char pad_0055f29e[50]; /* 0x0055f29e */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gAdpcmStepGroup, member) == (addr) - 0x0055f1e0, #member)
AT(gAdpcmStep, 0x0055f1ec);
AT(pad_0055f29e, 0x0055f29e);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gAdpcmStepGroup, pad_0055f29e) +
                       sizeof(((struct BmxGroup_gAdpcmStepGroup *)0)->pad_0055f29e) ==
                   0xf0,
               "gAdpcmStepGroup");

/* The type is local to bmx/com/zge/zactor.c. */
typedef uint16_t BmxType_gMotionMirrorMap[19];

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0055f390..0x0055ffc4.
 */
struct BmxGroup_gSkeletonChainGroup {
    int8_t gSkeletonChain[1];      /* 0x0055f390; the type is local to bmx/com/zge/zactor.c */
    unsigned char d_0055f391[303]; /* 0x0055f391 */
    struct {
        float x;
        float y;
        float z;
    } gSkeletonOffsets[1];                      /* 0x0055f4c0; the type is local to bmx/com/zge/zactor.c */
    float d_0055f4cc[197];                      /* 0x0055f4cc */
    char sOnlyCodedForPropTypes[28];            /* 0x0055f7e0 */
    unsigned char pad_0055f7fc[4];              /* 0x0055f7fc */
    char sCZaxisBmxSrcComZgeZactorC[36];        /* 0x0055f800 */
    unsigned char pad_0055f824[4];              /* 0x0055f824 */
    char sZACTORErrorFailedToAllocateActor[44]; /* 0x0055f828 */
    unsigned char pad_0055f854[8];              /* 0x0055f854 */
    char sMotionsAreTooLarge[24];               /* 0x0055f85c */
    unsigned char pad_0055f874[4];              /* 0x0055f874 */
    char sMotionLibraryAlreadyOpen[28];         /* 0x0055f878 */
    unsigned char pad_0055f894[8];              /* 0x0055f894 */
    char sMotionsNotOpened[20];                 /* 0x0055f89c */
    unsigned char pad_0055f8b0[4];              /* 0x0055f8b0 */
    char sTooBig[8];                            /* 0x0055f8b4 */
    unsigned char pad_0055f8bc[4];              /* 0x0055f8bc */
    char sCouldNotFindMotionBank[28];           /* 0x0055f8c0 */
    unsigned char pad_0055f8dc[4];              /* 0x0055f8dc */
    char sNotReloadingMotionLibrary[32];        /* 0x0055f8e0 */
    unsigned char pad_0055f900[4];              /* 0x0055f900 */
    char sMotionBankNotOpened[24];              /* 0x0055f904 */
    unsigned char pad_0055f91c[4];              /* 0x0055f91c */
    char sExeededMaximumLODsForActor[32];       /* 0x0055f920 */
    unsigned char pad_0055f940[8];              /* 0x0055f940 */
    char sZACTORUnknownTagIdentifier[36];       /* 0x0055f948 */
    unsigned char pad_0055f96c[4];              /* 0x0055f96c */
    char sNotAPersonType[20];                   /* 0x0055f970 */
    unsigned char pad_0055f984[4];              /* 0x0055f984 */
    char sZPRIMFailedToAllocateBuffer[36];      /* 0x0055f988 */
    unsigned char pad_0055f9ac[4];              /* 0x0055f9ac */
    char sErrorOptimizingDMMMemory[28];         /* 0x0055f9b0 */
    unsigned char pad_0055f9cc[8];              /* 0x0055f9cc */
    char sCZaxisBmxSrcComZgeZprimC[36];         /* 0x0055f9d4 */
    unsigned char pad_0055f9f8[4];              /* 0x0055f9f8 */
    char sZPRIMZprimGetColorNotImplemented[40]; /* 0x0055f9fc */
    unsigned char pad_0055fa24[8];              /* 0x0055fa24 */
    char sZPRIMFailedToAllocatePicture[36];     /* 0x0055fa2c */
    unsigned char pad_0055fa50[8];              /* 0x0055fa50 */
    char sZPRIMFailedToFindPictureX[36];        /* 0x0055fa58 */
    unsigned char pad_0055fa7c[4];              /* 0x0055fa7c */
    char sBufferOverflowInZprimReplaceVRAMGrey[44];    /* 0x0055fa80 */
    unsigned char pad_0055faac[8];                     /* 0x0055faac */
    char sZPRIMFailedToAllocateSprite[36];             /* 0x0055fab4 */
    unsigned char pad_0055fad8[4];                     /* 0x0055fad8 */
    char sZPRIMFailedToFindSpriteX[36];                /* 0x0055fadc */
    unsigned char pad_0055fb00[4];                     /* 0x0055fb00 */
    char sZPRIMFailedToAllocateAssetHandle[40];        /* 0x0055fb04 */
    unsigned char pad_0055fb2c[8];                     /* 0x0055fb2c */
    char sZPRIMFailedToAllocateBufferInCreateSpri[52]; /* 0x0055fb34 */
    unsigned char pad_0055fb68[8];                     /* 0x0055fb68 */
    char sZPRIMErrorCanOnlyDecompressFromCompress[88]; /* 0x0055fb70 */
    unsigned char pad_0055fbc8[16];                    /* 0x0055fbc8 */
    char sZPRIMErrorFrameOutOfBoundsDuringDecompr[56]; /* 0x0055fbd8 */
    unsigned char pad_0055fc10[8];                     /* 0x0055fc10 */
    char sPossibleMemoryLeakOfFntMemHdlHere[40];       /* 0x0055fc18 */
    unsigned char pad_0055fc40[8];                     /* 0x0055fc40 */
    char sZPRIMFailedToAllocateFont[32];               /* 0x0055fc48 */
    unsigned char pad_0055fc68[8];                     /* 0x0055fc68 */
    char sZPRIMFailedToFindFontX[32];                  /* 0x0055fc70 */
    unsigned char pad_0055fc90[8];                     /* 0x0055fc90 */
    char sZPRIMFailedToAllocateGroup[36];              /* 0x0055fc98 */
    unsigned char pad_0055fcbc[4];                     /* 0x0055fcbc */
    char sZPRIMFailedToFindGroup[32];                  /* 0x0055fcc0 */
    unsigned char pad_0055fce0[4];                     /* 0x0055fce0 */
    char sZPRIMLoadFailed[20];                         /* 0x0055fce4 */
    unsigned char pad_0055fcf8[4];                     /* 0x0055fcf8 */
    char sZPRIMErrorLoadingGroupIntoUsedSlot[48];      /* 0x0055fcfc */
    unsigned char pad_0055fd2c[8];                     /* 0x0055fd2c */
    char sZPRIMErrorBadSlotForGroup[36];               /* 0x0055fd34 */
    unsigned char pad_0055fd58[8];                     /* 0x0055fd58 */
    char sZPRIMFailedToLoadAsset[28];                  /* 0x0055fd60 */
    unsigned char pad_0055fd7c[8];                     /* 0x0055fd7c */
    char sZPRIMFailedToAllocateScreen[36];             /* 0x0055fd84 */
    unsigned char pad_0055fda8[4];                     /* 0x0055fda8 */
    char sZPRIMFailedToFindScreen[32];                 /* 0x0055fdac */
    unsigned char pad_0055fdcc[4];                     /* 0x0055fdcc */
    char sZPRIMErrorCanOnlyDrawtexttospriteFromFo[80]; /* 0x0055fdd0 */
    unsigned char pad_0055fe20[16];                    /* 0x0055fe20 */
    char sZPRIMErrorFrameOutOfBoundsDuringDrawtex[60]; /* 0x0055fe30 */
    unsigned char pad_0055fe6c[12];                    /* 0x0055fe6c */
    char sInvalidMaterialHandle[24];                   /* 0x0055fe78 */
    unsigned char pad_0055fe90[4];                     /* 0x0055fe90 */
    char sSpriteD[12];                                 /* 0x0055fe94 */
    char sTrack[8];                                    /* 0x0055fea0 */
    char sHTrackKzsndSeqMaxTracks[28];                 /* 0x0055fea8 */
    unsigned char pad_0055fec4[4];                     /* 0x0055fec4 */
    char sCZaxisBmxSrcComZgeZsndseqC[36];              /* 0x0055fec8 */
    unsigned char pad_0055feec[8];                     /* 0x0055feec */
    char sTrackHTrack[16];                             /* 0x0055fef4 */
    char sNoteEventFlagsKzsndNoteEventFlagActive[44];  /* 0x0055ff04 */
    unsigned char pad_0055ff30[8];                     /* 0x0055ff30 */
    char sAlreadyInitialized[20];                      /* 0x0055ff38 */
    unsigned char pad_0055ff4c[4];                     /* 0x0055ff4c */
    char sCZaxisBmxSrcComZgeZsndsrcC[36];              /* 0x0055ff50 */
    unsigned char pad_0055ff74[8];                     /* 0x0055ff74 */
    char sNotInitialized[16];                          /* 0x0055ff7c */
    unsigned char pad_0055ff8c[4];                     /* 0x0055ff8c */
    char sSourceNotAllocated[24];                      /* 0x0055ff90 */
    unsigned char pad_0055ffa8[4];                     /* 0x0055ffa8 */
    char sInvalidHandle[16];                           /* 0x0055ffac */
    unsigned char pad_0055ffbc[4];                     /* 0x0055ffbc */
    uint32_t gZsndTicksPerSec; /* 0x0055ffc0, also gWinstrmTicksPerSec, gZsndTickRate */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gSkeletonChainGroup, member) == (addr) - 0x0055f390, #member)
AT(gSkeletonChain, 0x0055f390);
AT(d_0055f391, 0x0055f391);
AT(gSkeletonOffsets, 0x0055f4c0);
AT(d_0055f4cc, 0x0055f4cc);
AT(sOnlyCodedForPropTypes, 0x0055f7e0);
AT(pad_0055f7fc, 0x0055f7fc);
AT(sCZaxisBmxSrcComZgeZactorC, 0x0055f800);
AT(pad_0055f824, 0x0055f824);
AT(sZACTORErrorFailedToAllocateActor, 0x0055f828);
AT(pad_0055f854, 0x0055f854);
AT(sMotionsAreTooLarge, 0x0055f85c);
AT(pad_0055f874, 0x0055f874);
AT(sMotionLibraryAlreadyOpen, 0x0055f878);
AT(pad_0055f894, 0x0055f894);
AT(sMotionsNotOpened, 0x0055f89c);
AT(pad_0055f8b0, 0x0055f8b0);
AT(sTooBig, 0x0055f8b4);
AT(pad_0055f8bc, 0x0055f8bc);
AT(sCouldNotFindMotionBank, 0x0055f8c0);
AT(pad_0055f8dc, 0x0055f8dc);
AT(sNotReloadingMotionLibrary, 0x0055f8e0);
AT(pad_0055f900, 0x0055f900);
AT(sMotionBankNotOpened, 0x0055f904);
AT(pad_0055f91c, 0x0055f91c);
AT(sExeededMaximumLODsForActor, 0x0055f920);
AT(pad_0055f940, 0x0055f940);
AT(sZACTORUnknownTagIdentifier, 0x0055f948);
AT(pad_0055f96c, 0x0055f96c);
AT(sNotAPersonType, 0x0055f970);
AT(pad_0055f984, 0x0055f984);
AT(sZPRIMFailedToAllocateBuffer, 0x0055f988);
AT(pad_0055f9ac, 0x0055f9ac);
AT(sErrorOptimizingDMMMemory, 0x0055f9b0);
AT(pad_0055f9cc, 0x0055f9cc);
AT(sCZaxisBmxSrcComZgeZprimC, 0x0055f9d4);
AT(pad_0055f9f8, 0x0055f9f8);
AT(sZPRIMZprimGetColorNotImplemented, 0x0055f9fc);
AT(pad_0055fa24, 0x0055fa24);
AT(sZPRIMFailedToAllocatePicture, 0x0055fa2c);
AT(pad_0055fa50, 0x0055fa50);
AT(sZPRIMFailedToFindPictureX, 0x0055fa58);
AT(pad_0055fa7c, 0x0055fa7c);
AT(sBufferOverflowInZprimReplaceVRAMGrey, 0x0055fa80);
AT(pad_0055faac, 0x0055faac);
AT(sZPRIMFailedToAllocateSprite, 0x0055fab4);
AT(pad_0055fad8, 0x0055fad8);
AT(sZPRIMFailedToFindSpriteX, 0x0055fadc);
AT(pad_0055fb00, 0x0055fb00);
AT(sZPRIMFailedToAllocateAssetHandle, 0x0055fb04);
AT(pad_0055fb2c, 0x0055fb2c);
AT(sZPRIMFailedToAllocateBufferInCreateSpri, 0x0055fb34);
AT(pad_0055fb68, 0x0055fb68);
AT(sZPRIMErrorCanOnlyDecompressFromCompress, 0x0055fb70);
AT(pad_0055fbc8, 0x0055fbc8);
AT(sZPRIMErrorFrameOutOfBoundsDuringDecompr, 0x0055fbd8);
AT(pad_0055fc10, 0x0055fc10);
AT(sPossibleMemoryLeakOfFntMemHdlHere, 0x0055fc18);
AT(pad_0055fc40, 0x0055fc40);
AT(sZPRIMFailedToAllocateFont, 0x0055fc48);
AT(pad_0055fc68, 0x0055fc68);
AT(sZPRIMFailedToFindFontX, 0x0055fc70);
AT(pad_0055fc90, 0x0055fc90);
AT(sZPRIMFailedToAllocateGroup, 0x0055fc98);
AT(pad_0055fcbc, 0x0055fcbc);
AT(sZPRIMFailedToFindGroup, 0x0055fcc0);
AT(pad_0055fce0, 0x0055fce0);
AT(sZPRIMLoadFailed, 0x0055fce4);
AT(pad_0055fcf8, 0x0055fcf8);
AT(sZPRIMErrorLoadingGroupIntoUsedSlot, 0x0055fcfc);
AT(pad_0055fd2c, 0x0055fd2c);
AT(sZPRIMErrorBadSlotForGroup, 0x0055fd34);
AT(pad_0055fd58, 0x0055fd58);
AT(sZPRIMFailedToLoadAsset, 0x0055fd60);
AT(pad_0055fd7c, 0x0055fd7c);
AT(sZPRIMFailedToAllocateScreen, 0x0055fd84);
AT(pad_0055fda8, 0x0055fda8);
AT(sZPRIMFailedToFindScreen, 0x0055fdac);
AT(pad_0055fdcc, 0x0055fdcc);
AT(sZPRIMErrorCanOnlyDrawtexttospriteFromFo, 0x0055fdd0);
AT(pad_0055fe20, 0x0055fe20);
AT(sZPRIMErrorFrameOutOfBoundsDuringDrawtex, 0x0055fe30);
AT(pad_0055fe6c, 0x0055fe6c);
AT(sInvalidMaterialHandle, 0x0055fe78);
AT(pad_0055fe90, 0x0055fe90);
AT(sSpriteD, 0x0055fe94);
AT(sTrack, 0x0055fea0);
AT(sHTrackKzsndSeqMaxTracks, 0x0055fea8);
AT(pad_0055fec4, 0x0055fec4);
AT(sCZaxisBmxSrcComZgeZsndseqC, 0x0055fec8);
AT(pad_0055feec, 0x0055feec);
AT(sTrackHTrack, 0x0055fef4);
AT(sNoteEventFlagsKzsndNoteEventFlagActive, 0x0055ff04);
AT(pad_0055ff30, 0x0055ff30);
AT(sAlreadyInitialized, 0x0055ff38);
AT(pad_0055ff4c, 0x0055ff4c);
AT(sCZaxisBmxSrcComZgeZsndsrcC, 0x0055ff50);
AT(pad_0055ff74, 0x0055ff74);
AT(sNotInitialized, 0x0055ff7c);
AT(pad_0055ff8c, 0x0055ff8c);
AT(sSourceNotAllocated, 0x0055ff90);
AT(pad_0055ffa8, 0x0055ffa8);
AT(sInvalidHandle, 0x0055ffac);
AT(pad_0055ffbc, 0x0055ffbc);
AT(gZsndTicksPerSec, 0x0055ffc0);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gSkeletonChainGroup, gZsndTicksPerSec) +
                       sizeof(((struct BmxGroup_gSkeletonChainGroup *)0)->gZsndTicksPerSec) ==
                   0xc34,
               "gSkeletonChainGroup");

/* The type is local to zge/zdebug.c. */
typedef void *BmxType_gZdebugStart;

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00560038..0x00560058.
 */
struct BmxGroup_gZProfilePrintAtGroup {
    uint8_t lead_[8];       /* nothing: the members' offsets keep their original alignment */
    void *gZProfilePrintAt; /* 0x00560038; the type is local to zge/zprofile.c */
    void *p_0056003c[7];    /* 0x0056003c */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gZProfilePrintAtGroup, member) ==                                \
                       BMX_DATA_OFF((addr) - 0x00560030, off64),                                             \
                   #member)
AT(gZProfilePrintAt, 0x00560038, 0x8);
AT(p_0056003c, 0x0056003c, 0x10);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZProfilePrintAtGroup, p_0056003c) +
                       sizeof(((struct BmxGroup_gZProfilePrintAtGroup *)0)->p_0056003c) ==
                   BMX_DATA_OFF(0x28, 0x48),
               "gZProfilePrintAtGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0056006c..0x00560074.
 */
struct BmxGroup_gZrwDbgSphereHookGroup {
    uint8_t lead_[12];                /* nothing: the members' offsets keep their original alignment */
    ZrwDbgSphereFn gZrwDbgSphereHook; /* 0x0056006c */
    void *p_00560070;                 /* 0x00560070 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gZrwDbgSphereHookGroup, member) ==                               \
                       BMX_DATA_OFF((addr) - 0x00560060, off64),                                             \
                   #member)
AT(gZrwDbgSphereHook, 0x0056006c, 0x10);
AT(p_00560070, 0x00560070, 0x18);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZrwDbgSphereHookGroup, p_00560070) +
                       sizeof(((struct BmxGroup_gZrwDbgSphereHookGroup *)0)->p_00560070) ==
                   BMX_DATA_OFF(0x14, 0x20),
               "gZrwDbgSphereHookGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00560078..0x00560080.
 */
struct BmxGroup_gZdebugIndentGroup {
    uint8_t lead_[8];             /* nothing: the members' offsets keep their original alignment */
    ZDebugIndentFn gZdebugIndent; /* 0x00560078 */
    void *p_0056007c;             /* 0x0056007c */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gZdebugIndentGroup, member) ==                                   \
                       BMX_DATA_OFF((addr) - 0x00560070, off64),                                             \
                   #member)
AT(gZdebugIndent, 0x00560078, 0x8);
AT(p_0056007c, 0x0056007c, 0x10);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZdebugIndentGroup, p_0056007c) +
                       sizeof(((struct BmxGroup_gZdebugIndentGroup *)0)->p_0056007c) ==
                   BMX_DATA_OFF(0x10, 0x18),
               "gZdebugIndentGroup");

/* The type is local to zge/com/zglrw/zrwdbg.c. */
typedef uint8_t BmxType_gBoxEdges[24];

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00560518..0x00560540.
 */
struct BmxGroup_gZobjCameraVtableGroup {
    uint8_t lead_[8];                   /* nothing: the members' offsets keep their original alignment */
    ZObjCameraVtable gZobjCameraVtable; /* 0x00560518 */
    unsigned char pad_00560528[24];     /* 0x00560528 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gZobjCameraVtableGroup, member) ==                               \
                       BMX_DATA_OFF((addr) - 0x00560510, off64),                                             \
                   #member)
AT(gZobjCameraVtable, 0x00560518, 0x8);
AT(pad_00560528, 0x00560528, 0x28);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZobjCameraVtableGroup, pad_00560528) +
                       sizeof(((struct BmxGroup_gZobjCameraVtableGroup *)0)->pad_00560528) ==
                   BMX_DATA_OFF(0x30, 0x40),
               "gZobjCameraVtableGroup");

/* The type is local to zge/com/zglrw/zrwzgl.c. */
typedef void *BmxType_gZglInitModel;

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00560e7c..0x00560e84.
 */
struct BmxGroup_gZglExitModelGroup {
    uint8_t lead_[12];   /* nothing: the members' offsets keep their original alignment */
    void *gZglExitModel; /* 0x00560e7c; the type is local to zge/com/zglrw/zrwzgl.c */
    void *p_00560e80;    /* 0x00560e80 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gZglExitModelGroup, member) ==                                   \
                       BMX_DATA_OFF((addr) - 0x00560e70, off64),                                             \
                   #member)
AT(gZglExitModel, 0x00560e7c, 0x10);
AT(p_00560e80, 0x00560e80, 0x18);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZglExitModelGroup, p_00560e80) +
                       sizeof(((struct BmxGroup_gZglExitModelGroup *)0)->p_00560e80) ==
                   BMX_DATA_OFF(0x14, 0x20),
               "gZglExitModelGroup");

/* The type is local to zge/com/zglrw/zrwmat.c. */
typedef void *BmxType_gZrwVtLoadModel;

/* The type is local to zge/com/zglrw/zrwmat.c. */
typedef void *BmxType_gZrwVtLoadChunk;

/* The type is local to zge/com/zglrw/zrwmat.c. */
typedef void *BmxType_gZrwVtInstance;

/* The type is local to zge/com/zglrw/zrwmat.c. */
typedef void *BmxType_gZrwVtPartCount;

/* The type is local to zge/com/zglrw/zrwmat.c. */
typedef void *BmxType_gZrwVtFindNode;

/* The type is local to zge/com/zglrw/zrwmat.c. */
typedef void *BmxType_gZrwVtNodeToWorld;

/* The type is local to zge/com/zglrw/zrwmat.c. */
typedef void *BmxType_gZrwVtNodeVector;

/* The type is local to zge/com/zglrw/zrwmat.c. */
typedef void *BmxType_gZrwVtNodeMatrix;

/* The type is local to zge/com/zglrw/zrwmat.c. */
typedef void *BmxType_gZrwVtNodeHashes;

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00560ecc..0x00560ed4.
 */
struct BmxGroup_gZrwVtSkeletonGroup {
    uint8_t lead_[12];    /* nothing: the members' offsets keep their original alignment */
    void *gZrwVtSkeleton; /* 0x00560ecc; the type is local to zge/com/zglrw/zrwmat.c */
    void *p_00560ed0;     /* 0x00560ed0 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gZrwVtSkeletonGroup, member) ==                                  \
                       BMX_DATA_OFF((addr) - 0x00560ec0, off64),                                             \
                   #member)
AT(gZrwVtSkeleton, 0x00560ecc, 0x10);
AT(p_00560ed0, 0x00560ed0, 0x18);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZrwVtSkeletonGroup, p_00560ed0) +
                       sizeof(((struct BmxGroup_gZrwVtSkeletonGroup *)0)->p_00560ed0) ==
                   BMX_DATA_OFF(0x14, 0x20),
               "gZrwVtSkeletonGroup");

/* The type is local to zge/com/zglrw/zrwmat.c. */
typedef void *BmxType_gZrwVtOwner;

/* The type is local to bmx/com/zge/ngif/ngiffx.c. */
typedef void *BmxType_gFxFindTexture;

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00560eec..0x00560ef4.
 */
struct BmxGroup_gZrwVtMatCountGroup {
    uint8_t lead_[12];           /* nothing: the members' offsets keep their original alignment */
    ZrwModelCall gZrwVtMatCount; /* 0x00560eec */
    void *p_00560ef0;            /* 0x00560ef0 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gZrwVtMatCountGroup, member) ==                                  \
                       BMX_DATA_OFF((addr) - 0x00560ee0, off64),                                             \
                   #member)
AT(gZrwVtMatCount, 0x00560eec, 0x10);
AT(p_00560ef0, 0x00560ef0, 0x18);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZrwVtMatCountGroup, p_00560ef0) +
                       sizeof(((struct BmxGroup_gZrwVtMatCountGroup *)0)->p_00560ef0) ==
                   BMX_DATA_OFF(0x14, 0x20),
               "gZrwVtMatCountGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00560ef4..0x00560f18.
 */
struct BmxGroup_gZobjSetColorGroup {
    uint8_t lead_[4];          /* nothing: the members' offsets keep their original alignment */
    ZobjColorFn gZobjSetColor; /* 0x00560ef4 */
    void *p_00560ef8[8];       /* 0x00560ef8 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gZobjSetColorGroup, member) ==                                   \
                       BMX_DATA_OFF((addr) - 0x00560ef0, off64),                                             \
                   #member)
AT(gZobjSetColor, 0x00560ef4, 0x8);
AT(p_00560ef8, 0x00560ef8, 0x10);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZobjSetColorGroup, p_00560ef8) +
                       sizeof(((struct BmxGroup_gZobjSetColorGroup *)0)->p_00560ef8) ==
                   BMX_DATA_OFF(0x28, 0x50),
               "gZobjSetColorGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00560fa8..0x00560fd0.
 */
struct BmxGroup_gZrwOpsSkinnedGroup {
    uint8_t lead_[8];              /* nothing: the members' offsets keep their original alignment */
    void *gZrwOpsSkinned;          /* 0x00560fa8 */
    void *p_00560fac[8];           /* 0x00560fac */
    unsigned char pad_00560fcc[4]; /* 0x00560fcc */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gZrwOpsSkinnedGroup, member) ==                                  \
                       BMX_DATA_OFF((addr) - 0x00560fa0, off64),                                             \
                   #member)
AT(gZrwOpsSkinned, 0x00560fa8, 0x8);
AT(p_00560fac, 0x00560fac, 0x10);
AT(pad_00560fcc, 0x00560fcc, 0x50);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZrwOpsSkinnedGroup, pad_00560fcc) +
                       sizeof(((struct BmxGroup_gZrwOpsSkinnedGroup *)0)->pad_00560fcc) ==
                   BMX_DATA_OFF(0x30, 0x54),
               "gZrwOpsSkinnedGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00560fd0..0x00560ff4.
 */
struct BmxGroup_gZrwOpsSimpleGroup {
    void *gZrwOpsSimple; /* 0x00560fd0 */
    void *p_00560fd4[8]; /* 0x00560fd4 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gZrwOpsSimpleGroup, member) ==                                   \
                       BMX_DATA_OFF((addr) - 0x00560fd0, off64),                                             \
                   #member)
AT(gZrwOpsSimple, 0x00560fd0, 0x0);
AT(p_00560fd4, 0x00560fd4, 0x8);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZrwOpsSimpleGroup, p_00560fd4) +
                       sizeof(((struct BmxGroup_gZrwOpsSimpleGroup *)0)->p_00560fd4) ==
                   BMX_DATA_OFF(0x24, 0x48),
               "gZrwOpsSimpleGroup");

/* The type is local to zge/com/zglrw/zrwzgl.c. */
typedef void *BmxType_gZglInitMaterial;

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0056101c..0x00561024.
 */
struct BmxGroup_gZglExitMaterialGroup {
    uint8_t lead_[12];      /* nothing: the members' offsets keep their original alignment */
    void *gZglExitMaterial; /* 0x0056101c; the type is local to zge/com/zglrw/zrwzgl.c */
    void *p_00561020;       /* 0x00561020 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gZglExitMaterialGroup, member) ==                                \
                       BMX_DATA_OFF((addr) - 0x00561010, off64),                                             \
                   #member)
AT(gZglExitMaterial, 0x0056101c, 0x10);
AT(p_00561020, 0x00561020, 0x18);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZglExitMaterialGroup, p_00561020) +
                       sizeof(((struct BmxGroup_gZglExitMaterialGroup *)0)->p_00561020) ==
                   BMX_DATA_OFF(0x14, 0x20),
               "gZglExitMaterialGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00561024..0x0056102c.
 */
struct BmxGroup_gZrollerFreeHandleGroup {
    uint8_t lead_[4];                       /* nothing: the members' offsets keep their original alignment */
    ZrollerFreeHandleFn gZrollerFreeHandle; /* 0x00561024, also gGuiFreeSpriteSheet, gRwTextureFree,
                                               gZrwMaterialRelease, gZrwWorldMatRelease */
    void *p_00561028;                       /* 0x00561028 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gZrollerFreeHandleGroup, member) ==                              \
                       BMX_DATA_OFF((addr) - 0x00561020, off64),                                             \
                   #member)
AT(gZrollerFreeHandle, 0x00561024, 0x8);
AT(p_00561028, 0x00561028, 0x10);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZrollerFreeHandleGroup, p_00561028) +
                       sizeof(((struct BmxGroup_gZrollerFreeHandleGroup *)0)->p_00561028) ==
                   BMX_DATA_OFF(0xc, 0x18),
               "gZrollerFreeHandleGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0056102c..0x00561034.
 */
struct BmxGroup_gFxSetRasterGroup {
    uint8_t lead_[12];  /* nothing: the members' offsets keep their original alignment */
    void *gFxSetRaster; /* 0x0056102c; the type is local to bmx/com/zge/ngif/ngiffx.c */
    void *p_00561030;   /* 0x00561030 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gFxSetRasterGroup, member) ==                                    \
                       BMX_DATA_OFF((addr) - 0x00561020, off64),                                             \
                   #member)
AT(gFxSetRaster, 0x0056102c, 0x10);
AT(p_00561030, 0x00561030, 0x18);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gFxSetRasterGroup, p_00561030) +
                       sizeof(((struct BmxGroup_gFxSetRasterGroup *)0)->p_00561030) ==
                   BMX_DATA_OFF(0x14, 0x20),
               "gFxSetRasterGroup");

/* The type is local to zge/com/zglrw/zrwworld.c. */
typedef void *BmxType_gZrwWorldSetHook;

/* The type is local to bmx/com/zge/ngif/ngiffx.c. */
typedef void *BmxType_gFxTextureByHash;

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x005611d8..0x005611e0.
 */
struct BmxGroup_gErrorCaptionGroup {
    uint8_t lead_[8];          /* nothing: the members' offsets keep their original alignment */
    const char *gErrorCaption; /* 0x005611d8 */
    void *p_005611dc;          /* 0x005611dc */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gErrorCaptionGroup, member) ==                                   \
                       BMX_DATA_OFF((addr) - 0x005611d0, off64),                                             \
                   #member)
AT(gErrorCaption, 0x005611d8, 0x8);
AT(p_005611dc, 0x005611dc, 0x10);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gErrorCaptionGroup, p_005611dc) +
                       sizeof(((struct BmxGroup_gErrorCaptionGroup *)0)->p_005611dc) ==
                   BMX_DATA_OFF(0x10, 0x18),
               "gErrorCaptionGroup");

/* The type is local to zge/zge.c. */
typedef void *BmxType_gZrwGraphicLibraryInitialize;

/* The type is local to zge/zdebug.c. */
typedef void *BmxType_gZdebugFailHandler;

/* The type is local to zge/com/zglrw/zrwworld.c. */
typedef void *BmxType_gZrwWorldSetBounds;

/* The type is local to zge/com/zglrw/zrwdbg.c. */
typedef void *BmxType_gZrwDbgFrameStart;

/* The type is local to zge/com/zglrw/zrwmodelrender.c. */
typedef void *BmxType_gZrwRenderStateHook;

/* The type is local to zge/com/zglrw/zrwmodelrender.c. */
typedef void *BmxType_gZrwBeginHook;

/* The type is local to zge/com/zglrw/zrwmodelrender.c. */
typedef void *BmxType_gZrwEndHook;

/* The type is local to zge/com/zglrw/zrwzgl.c. */
typedef void *BmxType_gZglAllocHookNop;

/* The type is local to zge/com/zglrw/zrwzgl.c. */
typedef void *BmxType_gZglInitPrim;

/* The type is local to zge/com/zglrw/zrwzgl.c. */
typedef void *BmxType_gZglExitPrim;

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00561858..0x0056187c.
 */
struct BmxGroup_gZobjLightOpsGroup {
    uint8_t lead_[8];               /* nothing: the members' offsets keep their original alignment */
    void *gZobjLightOps;            /* 0x00561858 */
    unsigned char pad_0056185c[4];  /* 0x0056185c */
    void *p_00561860[2];            /* 0x00561860 */
    unsigned char pad_00561868[20]; /* 0x00561868 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gZobjLightOpsGroup, member) ==                                   \
                       BMX_DATA_OFF((addr) - 0x00561850, off64),                                             \
                   #member)
AT(gZobjLightOps, 0x00561858, 0x8);
AT(pad_0056185c, 0x0056185c, 0x10);
AT(p_00561860, 0x00561860, 0x18);
AT(pad_00561868, 0x00561868, 0x28);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZobjLightOpsGroup, pad_00561868) +
                       sizeof(((struct BmxGroup_gZobjLightOpsGroup *)0)->pad_00561868) ==
                   BMX_DATA_OFF(0x2c, 0x3c),
               "gZobjLightOpsGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x005618a0..0x005618b0.
 */
struct BmxGroup_p_005618a0Group {
    void *p_005618a0[2];           /* 0x005618a0 */
    unsigned char pad_005618a8[8]; /* 0x005618a8 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_p_005618a0Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x005618a0, off64),                                             \
                   #member)
AT(p_005618a0, 0x005618a0, 0x0);
AT(pad_005618a8, 0x005618a8, 0x10);
#undef AT
_Static_assert(offsetof(struct BmxGroup_p_005618a0Group, pad_005618a8) +
                       sizeof(((struct BmxGroup_p_005618a0Group *)0)->pad_005618a8) ==
                   BMX_DATA_OFF(0x10, 0x18),
               "p_005618a0Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x005618b0..0x005618d4.
 */
struct BmxGroup_p_005618b0Group {
    void *p_005618b0[6];           /* 0x005618b0 */
    unsigned char pad_005618c8[8]; /* 0x005618c8 */
    void *p_005618d0;              /* 0x005618d0 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_p_005618b0Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x005618b0, off64),                                             \
                   #member)
AT(p_005618b0, 0x005618b0, 0x0);
AT(pad_005618c8, 0x005618c8, 0x30);
AT(p_005618d0, 0x005618d0, 0x38);
#undef AT
_Static_assert(offsetof(struct BmxGroup_p_005618b0Group, p_005618d0) +
                       sizeof(((struct BmxGroup_p_005618b0Group *)0)->p_005618d0) ==
                   BMX_DATA_OFF(0x24, 0x40),
               "p_005618b0Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x005618d4..0x005618dc.
 */
struct BmxGroup_d_005618d4Group {
    uint8_t lead_[4];            /* nothing: the members' offsets keep their original alignment */
    unsigned char d_005618d4[4]; /* 0x005618d4 */
    void *p_005618d8;            /* 0x005618d8 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_d_005618d4Group, member) == (addr) - 0x005618d0, #member)
AT(d_005618d4, 0x005618d4);
AT(p_005618d8, 0x005618d8);
#undef AT
_Static_assert(offsetof(struct BmxGroup_d_005618d4Group, p_005618d8) +
                       sizeof(((struct BmxGroup_d_005618d4Group *)0)->p_005618d8) ==
                   BMX_DATA_OFF(0xc, 0x10),
               "d_005618d4Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x005618dc..0x005618e4.
 */
struct BmxGroup_gSceneModelHandlersGroup {
    uint8_t lead_[12];         /* nothing: the members' offsets keep their original alignment */
    void *gSceneModelHandlers; /* 0x005618dc; the type is local to zge/zscene.c */
    void *p_005618e0;          /* 0x005618e0 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gSceneModelHandlersGroup, member) ==                             \
                       BMX_DATA_OFF((addr) - 0x005618d0, off64),                                             \
                   #member)
AT(gSceneModelHandlers, 0x005618dc, 0x10);
AT(p_005618e0, 0x005618e0, 0x18);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gSceneModelHandlersGroup, p_005618e0) +
                       sizeof(((struct BmxGroup_gSceneModelHandlersGroup *)0)->p_005618e0) ==
                   BMX_DATA_OFF(0x14, 0x20),
               "gSceneModelHandlersGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x005618e4..0x00561900.
 */
struct BmxGroup_gSceneAnimHandlersGroup {
    uint8_t lead_[4];               /* nothing: the members' offsets keep their original alignment */
    void *gSceneAnimHandlers;       /* 0x005618e4; the type is local to zge/zscene.c */
    unsigned char pad_005618e8[24]; /* 0x005618e8 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gSceneAnimHandlersGroup, member) ==                              \
                       BMX_DATA_OFF((addr) - 0x005618e0, off64),                                             \
                   #member)
AT(gSceneAnimHandlers, 0x005618e4, 0x8);
AT(pad_005618e8, 0x005618e8, 0x10);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gSceneAnimHandlersGroup, pad_005618e8) +
                       sizeof(((struct BmxGroup_gSceneAnimHandlersGroup *)0)->pad_005618e8) ==
                   BMX_DATA_OFF(0x20, 0x28),
               "gSceneAnimHandlersGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00561900..0x00561908.
 */
struct BmxGroup_d_00561900Group {
    uint32_t d_00561900; /* 0x00561900 */
    void *p_00561904;    /* 0x00561904 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_d_00561900Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x00561900, off64),                                             \
                   #member)
AT(d_00561900, 0x00561900, 0x0);
AT(p_00561904, 0x00561904, 0x8);
#undef AT
_Static_assert(offsetof(struct BmxGroup_d_00561900Group, p_00561904) +
                       sizeof(((struct BmxGroup_d_00561900Group *)0)->p_00561904) ==
                   BMX_DATA_OFF(0x8, 0x10),
               "d_00561900Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00561908..0x00561918.
 */
struct BmxGroup_d_00561908Group {
    uint8_t lead_[8];    /* nothing: the members' offsets keep their original alignment */
    uint32_t d_00561908; /* 0x00561908 */
    void *p_0056190c;    /* 0x0056190c */
    uint32_t d_00561910; /* 0x00561910 */
    void *p_00561914;    /* 0x00561914 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_d_00561908Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x00561900, off64),                                             \
                   #member)
AT(d_00561908, 0x00561908, 0x8);
AT(p_0056190c, 0x0056190c, 0x10);
AT(d_00561910, 0x00561910, 0x18);
AT(p_00561914, 0x00561914, 0x20);
#undef AT
_Static_assert(offsetof(struct BmxGroup_d_00561908Group, p_00561914) +
                       sizeof(((struct BmxGroup_d_00561908Group *)0)->p_00561914) ==
                   BMX_DATA_OFF(0x18, 0x28),
               "d_00561908Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00561918..0x00561920.
 */
struct BmxGroup_d_00561918Group {
    uint8_t lead_[8];    /* nothing: the members' offsets keep their original alignment */
    uint32_t d_00561918; /* 0x00561918 */
    void *p_0056191c;    /* 0x0056191c */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_d_00561918Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x00561910, off64),                                             \
                   #member)
AT(d_00561918, 0x00561918, 0x8);
AT(p_0056191c, 0x0056191c, 0x10);
#undef AT
_Static_assert(offsetof(struct BmxGroup_d_00561918Group, p_0056191c) +
                       sizeof(((struct BmxGroup_d_00561918Group *)0)->p_0056191c) ==
                   BMX_DATA_OFF(0x10, 0x18),
               "d_00561918Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00561920..0x00561928.
 */
struct BmxGroup_d_00561920Group {
    uint32_t d_00561920; /* 0x00561920 */
    void *p_00561924;    /* 0x00561924 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_d_00561920Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x00561920, off64),                                             \
                   #member)
AT(d_00561920, 0x00561920, 0x0);
AT(p_00561924, 0x00561924, 0x8);
#undef AT
_Static_assert(offsetof(struct BmxGroup_d_00561920Group, p_00561924) +
                       sizeof(((struct BmxGroup_d_00561920Group *)0)->p_00561924) ==
                   BMX_DATA_OFF(0x8, 0x10),
               "d_00561920Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00561d98..0x00561da0.
 */
struct BmxGroup_gZrwCamSetFovGroup {
    uint8_t lead_[8];             /* nothing: the members' offsets keep their original alignment */
    ZrwCamSetFovFn gZrwCamSetFov; /* 0x00561d98 */
    void *p_00561d9c;             /* 0x00561d9c */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gZrwCamSetFovGroup, member) ==                                   \
                       BMX_DATA_OFF((addr) - 0x00561d90, off64),                                             \
                   #member)
AT(gZrwCamSetFov, 0x00561d98, 0x8);
AT(p_00561d9c, 0x00561d9c, 0x10);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZrwCamSetFovGroup, p_00561d9c) +
                       sizeof(((struct BmxGroup_gZrwCamSetFovGroup *)0)->p_00561d9c) ==
                   BMX_DATA_OFF(0x10, 0x18),
               "gZrwCamSetFovGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00561da0..0x00561dac.
 */
struct BmxGroup_gZrwCamSetViewGroup {
    ZrwCamSetViewFn gZrwCamSetView; /* 0x00561da0 */
    void *p_00561da4[2];            /* 0x00561da4 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gZrwCamSetViewGroup, member) ==                                  \
                       BMX_DATA_OFF((addr) - 0x00561da0, off64),                                             \
                   #member)
AT(gZrwCamSetView, 0x00561da0, 0x0);
AT(p_00561da4, 0x00561da4, 0x8);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZrwCamSetViewGroup, p_00561da4) +
                       sizeof(((struct BmxGroup_gZrwCamSetViewGroup *)0)->p_00561da4) ==
                   BMX_DATA_OFF(0xc, 0x18),
               "gZrwCamSetViewGroup");

/* The type is local to zge/com/zglrw/zrwdbg.c. */
typedef void *BmxType_gZrwDbgGetCamera;

/* The type is local to zge/com/zglrw/zrwdbg.c. */
typedef void *BmxType_gZrwDbgCameraObject;

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00561db8..0x00561dc0.
 */
struct BmxGroup_gNgifSetViewportGroup {
    uint8_t lead_[8];       /* nothing: the members' offsets keep their original alignment */
    void *gNgifSetViewport; /* 0x00561db8; the type is local to bmx/com/zge/ngif/ngifpsx.c */
    void *p_00561dbc;       /* 0x00561dbc */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gNgifSetViewportGroup, member) ==                                \
                       BMX_DATA_OFF((addr) - 0x00561db0, off64),                                             \
                   #member)
AT(gNgifSetViewport, 0x00561db8, 0x8);
AT(p_00561dbc, 0x00561dbc, 0x10);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gNgifSetViewportGroup, p_00561dbc) +
                       sizeof(((struct BmxGroup_gNgifSetViewportGroup *)0)->p_00561dbc) ==
                   BMX_DATA_OFF(0x10, 0x18),
               "gNgifSetViewportGroup");

/* The type is local to bmx/com/zge/ngif/ngifpsx.c. */
typedef void *BmxType_gNgifSetViewOffset;

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00561e68..0x00561e88.
 */
struct BmxGroup_gZpadKeyMapFirstGroup {
    uint8_t lead_[8];             /* nothing: the members' offsets keep their original alignment */
    uint8_t gZpadKeyMapFirst;     /* 0x00561e68 */
    unsigned char d_00561e69[31]; /* 0x00561e69 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gZpadKeyMapFirstGroup, member) == (addr) - 0x00561e60, #member)
AT(gZpadKeyMapFirst, 0x00561e68);
AT(d_00561e69, 0x00561e69);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZpadKeyMapFirstGroup, d_00561e69) +
                       sizeof(((struct BmxGroup_gZpadKeyMapFirstGroup *)0)->d_00561e69) ==
                   0x28,
               "gZpadKeyMapFirstGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00561e88..0x00561ea8.
 */
struct BmxGroup_gZpadKeyMapOtherGroup {
    uint8_t lead_[8];             /* nothing: the members' offsets keep their original alignment */
    uint8_t gZpadKeyMapOther;     /* 0x00561e88 */
    unsigned char d_00561e89[31]; /* 0x00561e89 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gZpadKeyMapOtherGroup, member) == (addr) - 0x00561e80, #member)
AT(gZpadKeyMapOther, 0x00561e88);
AT(d_00561e89, 0x00561e89);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZpadKeyMapOtherGroup, d_00561e89) +
                       sizeof(((struct BmxGroup_gZpadKeyMapOtherGroup *)0)->d_00561e89) ==
                   0x28,
               "gZpadKeyMapOtherGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00561ea8..0x00561ec8.
 */
struct BmxGroup_gZpadDefaultButtonMapGroup {
    uint8_t lead_[8];              /* nothing: the members' offsets keep their original alignment */
    uint8_t gZpadDefaultButtonMap; /* 0x00561ea8 */
    unsigned char d_00561ea9[31];  /* 0x00561ea9 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gZpadDefaultButtonMapGroup, member) == (addr) - 0x00561ea0,      \
                   #member)
AT(gZpadDefaultButtonMap, 0x00561ea8);
AT(d_00561ea9, 0x00561ea9);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZpadDefaultButtonMapGroup, d_00561ea9) +
                       sizeof(((struct BmxGroup_gZpadDefaultButtonMapGroup *)0)->d_00561ea9) ==
                   0x28,
               "gZpadDefaultButtonMapGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00561ec8..0x00561ed8.
 */
struct BmxGroup_p_00561ec8Group {
    uint8_t lead_[8];              /* nothing: the members' offsets keep their original alignment */
    void *p_00561ec8[2];           /* 0x00561ec8 */
    unsigned char pad_00561ed0[8]; /* 0x00561ed0 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_p_00561ec8Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x00561ec0, off64),                                             \
                   #member)
AT(p_00561ec8, 0x00561ec8, 0x8);
AT(pad_00561ed0, 0x00561ed0, 0x18);
#undef AT
_Static_assert(offsetof(struct BmxGroup_p_00561ec8Group, pad_00561ed0) +
                       sizeof(((struct BmxGroup_p_00561ec8Group *)0)->pad_00561ed0) ==
                   BMX_DATA_OFF(0x18, 0x20),
               "p_00561ec8Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00561ed8..0x00561ef8.
 */
struct BmxGroup_p_00561ed8Group {
    uint8_t lead_[8];       /* nothing: the members' offsets keep their original alignment */
    void *p_00561ed8;       /* 0x00561ed8 */
    uint32_t d_00561edc[7]; /* 0x00561edc */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_p_00561ed8Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x00561ed0, off64),                                             \
                   #member)
AT(p_00561ed8, 0x00561ed8, 0x8);
AT(d_00561edc, 0x00561edc, 0x10);
#undef AT
_Static_assert(offsetof(struct BmxGroup_p_00561ed8Group, d_00561edc) +
                       sizeof(((struct BmxGroup_p_00561ed8Group *)0)->d_00561edc) ==
                   BMX_DATA_OFF(0x28, 0x2c),
               "p_00561ed8Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00561ef8..0x00561f04.
 */
struct BmxGroup_p_00561ef8Group {
    uint8_t lead_[8];              /* nothing: the members' offsets keep their original alignment */
    void *p_00561ef8[2];           /* 0x00561ef8 */
    unsigned char pad_00561f00[4]; /* 0x00561f00 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_p_00561ef8Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x00561ef0, off64),                                             \
                   #member)
AT(p_00561ef8, 0x00561ef8, 0x8);
AT(pad_00561f00, 0x00561f00, 0x18);
#undef AT
_Static_assert(offsetof(struct BmxGroup_p_00561ef8Group, pad_00561f00) +
                       sizeof(((struct BmxGroup_p_00561ef8Group *)0)->pad_00561f00) ==
                   BMX_DATA_OFF(0x14, 0x1c),
               "p_00561ef8Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00561f04..0x00561f10.
 */
struct BmxGroup_p_00561f04Group {
    uint8_t lead_[4];              /* nothing: the members' offsets keep their original alignment */
    void *p_00561f04;              /* 0x00561f04 */
    unsigned char pad_00561f08[8]; /* 0x00561f08 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_p_00561f04Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x00561f00, off64),                                             \
                   #member)
AT(p_00561f04, 0x00561f04, 0x8);
AT(pad_00561f08, 0x00561f08, 0x10);
#undef AT
_Static_assert(offsetof(struct BmxGroup_p_00561f04Group, pad_00561f08) +
                       sizeof(((struct BmxGroup_p_00561f04Group *)0)->pad_00561f08) ==
                   BMX_DATA_OFF(0x10, 0x18),
               "p_00561f04Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00561f10..0x00561f2c.
 */
struct BmxGroup_p_00561f10Group {
    void *p_00561f10[2];    /* 0x00561f10 */
    uint32_t d_00561f18[5]; /* 0x00561f18 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_p_00561f10Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x00561f10, off64),                                             \
                   #member)
AT(p_00561f10, 0x00561f10, 0x0);
AT(d_00561f18, 0x00561f18, 0x10);
#undef AT
_Static_assert(offsetof(struct BmxGroup_p_00561f10Group, d_00561f18) +
                       sizeof(((struct BmxGroup_p_00561f10Group *)0)->d_00561f18) ==
                   BMX_DATA_OFF(0x1c, 0x24),
               "p_00561f10Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00561f2c..0x00561f38.
 */
struct BmxGroup_gZpadProfilesGroup {
    uint8_t lead_[12];                /* nothing: the members' offsets keep their original alignment */
    const ZpadProfile *gZpadProfiles; /* 0x00561f2c */
    void *p_00561f30;                 /* 0x00561f30 */
    unsigned char pad_00561f34[4];    /* 0x00561f34 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gZpadProfilesGroup, member) ==                                   \
                       BMX_DATA_OFF((addr) - 0x00561f20, off64),                                             \
                   #member)
AT(gZpadProfiles, 0x00561f2c, 0x10);
AT(p_00561f30, 0x00561f30, 0x18);
AT(pad_00561f34, 0x00561f34, 0x20);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZpadProfilesGroup, pad_00561f34) +
                       sizeof(((struct BmxGroup_gZpadProfilesGroup *)0)->pad_00561f34) ==
                   BMX_DATA_OFF(0x18, 0x24),
               "gZpadProfilesGroup");

/* The type is local to zge/com/zglrw/zrwzgl.c. */
typedef void *BmxType_gZglInitCol;

/* The type is local to zge/com/zglrw/zrwzgl.c. */
typedef void *BmxType_gZglExitCol;

/* The type is local to zge/com/zglrw/zrwcol.c. */
typedef void *BmxType_gZrwColLineFn;

/* The type is local to zge/com/zglrw/zrwcol.c. */
typedef void *BmxType_gZrwColSphereFn;

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00562098..0x005620bc.
 */
struct BmxGroup_gZrwWorldEffectOpsGroup {
    uint8_t lead_[8];               /* nothing: the members' offsets keep their original alignment */
    void *gZrwWorldEffectOps;       /* 0x00562098 */
    void *p_0056209c[5];            /* 0x0056209c */
    unsigned char pad_005620b0[12]; /* 0x005620b0 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gZrwWorldEffectOpsGroup, member) ==                              \
                       BMX_DATA_OFF((addr) - 0x00562090, off64),                                             \
                   #member)
AT(gZrwWorldEffectOps, 0x00562098, 0x8);
AT(p_0056209c, 0x0056209c, 0x10);
AT(pad_005620b0, 0x005620b0, 0x38);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZrwWorldEffectOpsGroup, pad_005620b0) +
                       sizeof(((struct BmxGroup_gZrwWorldEffectOpsGroup *)0)->pad_005620b0) ==
                   BMX_DATA_OFF(0x2c, 0x44),
               "gZrwWorldEffectOpsGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x005620ec..0x0056210c.
 */
struct BmxGroup_gFxLoadTextureGroup {
    uint8_t lead_[12];    /* nothing: the members' offsets keep their original alignment */
    void *gFxLoadTexture; /* 0x005620ec; the type is local to bmx/com/zge/ngif/ngiffx.c */
    void *p_005620f0[7];  /* 0x005620f0 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gFxLoadTextureGroup, member) ==                                  \
                       BMX_DATA_OFF((addr) - 0x005620e0, off64),                                             \
                   #member)
AT(gFxLoadTexture, 0x005620ec, 0x10);
AT(p_005620f0, 0x005620f0, 0x18);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gFxLoadTextureGroup, p_005620f0) +
                       sizeof(((struct BmxGroup_gFxLoadTextureGroup *)0)->p_005620f0) ==
                   BMX_DATA_OFF(0x2c, 0x50),
               "gFxLoadTextureGroup");

/* The type is local to zge/com/zglrw/zrwworld.c. */
typedef void *BmxType_gZrwLightKinds[7];

/* The type is local to zge/com/zglrw/zrwfile.c. */
typedef char BmxType_gZrwPathKey[8];

/* The type is local to zge/com/zglrw/zrwworld.c. */
typedef float BmxType_gZrwDefaultBounce[2];

/* The type is local to zge/com/zglrw/zrwzgl.c. */
typedef void *BmxType_gZglInitTexture;

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x005623f4..0x005623fc.
 */
struct BmxGroup_gZglExitTextureGroup {
    uint8_t lead_[4];      /* nothing: the members' offsets keep their original alignment */
    void *gZglExitTexture; /* 0x005623f4; the type is local to zge/com/zglrw/zrwzgl.c */
    void *p_005623f8;      /* 0x005623f8 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gZglExitTextureGroup, member) ==                                 \
                       BMX_DATA_OFF((addr) - 0x005623f0, off64),                                             \
                   #member)
AT(gZglExitTexture, 0x005623f4, 0x8);
AT(p_005623f8, 0x005623f8, 0x10);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZglExitTextureGroup, p_005623f8) +
                       sizeof(((struct BmxGroup_gZglExitTextureGroup *)0)->p_005623f8) ==
                   BMX_DATA_OFF(0xc, 0x18),
               "gZglExitTextureGroup");

/* The type is local to zge/com/zglrw/zrwmat.c. */
typedef void *BmxType_gZrwTextureLoad;

/* The type is local to zge/com/zglrw/zrwworld.c. */
typedef void *BmxType_gZrwWorldTexDrop;

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00562408..0x00562410.
 */
struct BmxGroup_gZresTextureHeightGroup {
    uint8_t lead_[8];                     /* nothing: the members' offsets keep their original alignment */
    ZresTextureSizeFn gZresTextureHeight; /* 0x00562408, also gNgifTextureHeight */
    void *p_0056240c;                     /* 0x0056240c */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gZresTextureHeightGroup, member) ==                              \
                       BMX_DATA_OFF((addr) - 0x00562400, off64),                                             \
                   #member)
AT(gZresTextureHeight, 0x00562408, 0x8);
AT(p_0056240c, 0x0056240c, 0x10);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZresTextureHeightGroup, p_0056240c) +
                       sizeof(((struct BmxGroup_gZresTextureHeightGroup *)0)->p_0056240c) ==
                   BMX_DATA_OFF(0x10, 0x18),
               "gZresTextureHeightGroup");

/* The type is local to zge/com/zglrw/zrwworld.c. */
typedef void *BmxType_gZrwWorldTexName;

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00562414..0x00562420.
 */
struct BmxGroup_gZrwTextureHashGroup {
    uint8_t lead_[4];                 /* nothing: the members' offsets keep their original alignment */
    ZrwTextureHashFn gZrwTextureHash; /* 0x00562414 */
    void *p_00562418[2];              /* 0x00562418 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gZrwTextureHashGroup, member) ==                                 \
                       BMX_DATA_OFF((addr) - 0x00562410, off64),                                             \
                   #member)
AT(gZrwTextureHash, 0x00562414, 0x8);
AT(p_00562418, 0x00562418, 0x10);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZrwTextureHashGroup, p_00562418) +
                       sizeof(((struct BmxGroup_gZrwTextureHashGroup *)0)->p_00562418) ==
                   BMX_DATA_OFF(0x10, 0x20),
               "gZrwTextureHashGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00562420..0x00562428.
 */
struct BmxGroup_gZrwTextureRetainGroup {
    void *gZrwTextureRetain; /* 0x00562420; the type is local to zge/com/zglrw/zrwmat.c */
    void *p_00562424;        /* 0x00562424 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gZrwTextureRetainGroup, member) ==                               \
                       BMX_DATA_OFF((addr) - 0x00562420, off64),                                             \
                   #member)
AT(gZrwTextureRetain, 0x00562420, 0x0);
AT(p_00562424, 0x00562424, 0x8);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZrwTextureRetainGroup, p_00562424) +
                       sizeof(((struct BmxGroup_gZrwTextureRetainGroup *)0)->p_00562424) ==
                   BMX_DATA_OFF(0x8, 0x10),
               "gZrwTextureRetainGroup");

/* The type is local to zge/com/zglrw/zrwtxr.c. */
typedef char BmxType_kZrwExtBmp[4];

/* The type is local to zge/com/zglrw/zrwtxr.c. */
typedef char BmxType_kZrwExtPng[4];

/* The type is local to zge/com/zglrw/zrwtxr.c. */
typedef char BmxType_kZrwFormatDds[4];

/* The type is local to zge/com/zglrw/zrwtxr.c. */
typedef char BmxType_kZrwFormatPng[4];

/* The type is local to zge/com/zglrw/zrwtxr.c. */
typedef char BmxType_kZrwFormatBmp[4];

/* The type is local to zge/com/zglrw/zrwmodelcreate.c. */
typedef float BmxType_gZrwLodDefaults[4];

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00562ff8..0x00563038.
 */
struct BmxGroup_gZglFileDeviceRwGroup {
    uint8_t lead_[8];              /* nothing: the members' offsets keep their original alignment */
    void *gZglFileDeviceRw[15];    /* 0x00562ff8 */
    unsigned char pad_00563034[4]; /* 0x00563034 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gZglFileDeviceRwGroup, member) ==                                \
                       BMX_DATA_OFF((addr) - 0x00562ff0, off64),                                             \
                   #member)
AT(gZglFileDeviceRw, 0x00562ff8, 0x8);
AT(pad_00563034, 0x00563034, 0x80);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZglFileDeviceRwGroup, pad_00563034) +
                       sizeof(((struct BmxGroup_gZglFileDeviceRwGroup *)0)->pad_00563034) ==
                   BMX_DATA_OFF(0x48, 0x84),
               "gZglFileDeviceRwGroup");

/* The type is local to zge/com/zglrw/zrwfile.c. */
typedef void *BmxType_gZrwOpenModes[4];

/* The type is local to zge/com/zglrw/zrwfile.c. */
typedef char BmxType_gZrwDeviceName[8];

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00563240..0x00563280.
 */
struct BmxGroup_gZglFileDeviceRamGroup {
    void *gZglFileDeviceRam[15];   /* 0x00563240 */
    unsigned char pad_0056327c[4]; /* 0x0056327c */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gZglFileDeviceRamGroup, member) ==                               \
                       BMX_DATA_OFF((addr) - 0x00563240, off64),                                             \
                   #member)
AT(gZglFileDeviceRam, 0x00563240, 0x0);
AT(pad_0056327c, 0x0056327c, 0x78);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZglFileDeviceRamGroup, pad_0056327c) +
                       sizeof(((struct BmxGroup_gZglFileDeviceRamGroup *)0)->pad_0056327c) ==
                   BMX_DATA_OFF(0x40, 0x7c),
               "gZglFileDeviceRamGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00563408..0x00563420.
 */
struct BmxGroup_gZrwLightTypesGroup {
    uint8_t lead_[8];           /* nothing: the members' offsets keep their original alignment */
    uint32_t gZrwLightTypes[4]; /* 0x00563408; the type is local to zge/com/zglrw/zrwlight.c */
    uint32_t d_00563418[2];     /* 0x00563418 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gZrwLightTypesGroup, member) == (addr) - 0x00563400, #member)
AT(gZrwLightTypes, 0x00563408);
AT(d_00563418, 0x00563418);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZrwLightTypesGroup, d_00563418) +
                       sizeof(((struct BmxGroup_gZrwLightTypesGroup *)0)->d_00563418) ==
                   0x20,
               "gZrwLightTypesGroup");

/* The type is local to zge/com/zglrw/zrwzgl.c. */
typedef void *BmxType_gZglInitLight;

/* The type is local to zge/com/zglrw/zrwzgl.c. */
typedef void *BmxType_gZglExitLight;

/* The type is local to zge/com/zglrw/zrwramdisk.c. */
typedef float BmxType_kZrwBBoxFaces[6][4];

/* The type is local to zge/zprofile.c. */
typedef uint32_t BmxType_gZProfileColors[5];

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00563a60..0x00563a70.
 */
struct BmxGroup_p_00563a60Group {
    void *p_00563a60[3];           /* 0x00563a60 */
    unsigned char pad_00563a6c[4]; /* 0x00563a6c */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_p_00563a60Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x00563a60, off64),                                             \
                   #member)
AT(p_00563a60, 0x00563a60, 0x0);
AT(pad_00563a6c, 0x00563a6c, 0x18);
#undef AT
_Static_assert(offsetof(struct BmxGroup_p_00563a60Group, pad_00563a6c) +
                       sizeof(((struct BmxGroup_p_00563a60Group *)0)->pad_00563a6c) ==
                   BMX_DATA_OFF(0x10, 0x1c),
               "p_00563a60Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00563aa0..0x00563ab0.
 */
struct BmxGroup_p_00563aa0Group {
    void *p_00563aa0;               /* 0x00563aa0 */
    unsigned char pad_00563aa4[12]; /* 0x00563aa4 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_p_00563aa0Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x00563aa0, off64),                                             \
                   #member)
AT(p_00563aa0, 0x00563aa0, 0x0);
AT(pad_00563aa4, 0x00563aa4, 0x8);
#undef AT
_Static_assert(offsetof(struct BmxGroup_p_00563aa0Group, pad_00563aa4) +
                       sizeof(((struct BmxGroup_p_00563aa0Group *)0)->pad_00563aa4) ==
                   BMX_DATA_OFF(0x10, 0x14),
               "p_00563aa0Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00563ad0..0x00563af0.
 */
struct BmxGroup_gCrtFloatHooksGroup {
    void *gCrtFloatHooks[6]; /* 0x00563ad0; the type is local to zge/pc/pcprofile.c */
    uint32_t d_00563ae8[2];  /* 0x00563ae8 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gCrtFloatHooksGroup, member) ==                                  \
                       BMX_DATA_OFF((addr) - 0x00563ad0, off64),                                             \
                   #member)
AT(gCrtFloatHooks, 0x00563ad0, 0x0);
AT(d_00563ae8, 0x00563ae8, 0x30);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gCrtFloatHooksGroup, d_00563ae8) +
                       sizeof(((struct BmxGroup_gCrtFloatHooksGroup *)0)->d_00563ae8) ==
                   BMX_DATA_OFF(0x20, 0x38),
               "gCrtFloatHooksGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00565b18..0x00565b28.
 */
struct BmxGroup_p_00565b18Group {
    uint8_t lead_[8];              /* nothing: the members' offsets keep their original alignment */
    void *p_00565b18;              /* 0x00565b18 */
    unsigned char pad_00565b1c[4]; /* 0x00565b1c */
    void *p_00565b20;              /* 0x00565b20 */
    uint32_t d_00565b24;           /* 0x00565b24 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_p_00565b18Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x00565b10, off64),                                             \
                   #member)
AT(p_00565b18, 0x00565b18, 0x8);
AT(pad_00565b1c, 0x00565b1c, 0x10);
AT(p_00565b20, 0x00565b20, 0x18);
AT(d_00565b24, 0x00565b24, 0x20);
#undef AT
_Static_assert(offsetof(struct BmxGroup_p_00565b18Group, d_00565b24) +
                       sizeof(((struct BmxGroup_p_00565b18Group *)0)->d_00565b24) ==
                   BMX_DATA_OFF(0x18, 0x24),
               "p_00565b18Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00565e4c..0x00565ed8.
 */
struct BmxGroup_p_00565e4cGroup {
    uint8_t lead_[12];   /* nothing: the members' offsets keep their original alignment */
    void *p_00565e4c;    /* 0x00565e4c */
    uint32_t d_00565e50; /* 0x00565e50 */
    void *p_00565e54;    /* 0x00565e54 */
    uint32_t d_00565e58; /* 0x00565e58 */
    void *p_00565e5c;    /* 0x00565e5c */
    uint32_t d_00565e60; /* 0x00565e60 */
    void *p_00565e64;    /* 0x00565e64 */
    uint32_t d_00565e68; /* 0x00565e68 */
    void *p_00565e6c;    /* 0x00565e6c */
    uint32_t d_00565e70; /* 0x00565e70 */
    void *p_00565e74;    /* 0x00565e74 */
    uint32_t d_00565e78; /* 0x00565e78 */
    void *p_00565e7c;    /* 0x00565e7c */
    uint32_t d_00565e80; /* 0x00565e80 */
    void *p_00565e84;    /* 0x00565e84 */
    uint32_t d_00565e88; /* 0x00565e88 */
    void *p_00565e8c;    /* 0x00565e8c */
    uint32_t d_00565e90; /* 0x00565e90 */
    void *p_00565e94;    /* 0x00565e94 */
    uint32_t d_00565e98; /* 0x00565e98 */
    void *p_00565e9c;    /* 0x00565e9c */
    uint32_t d_00565ea0; /* 0x00565ea0 */
    void *p_00565ea4;    /* 0x00565ea4 */
    uint32_t d_00565ea8; /* 0x00565ea8 */
    void *p_00565eac;    /* 0x00565eac */
    uint32_t d_00565eb0; /* 0x00565eb0 */
    void *p_00565eb4;    /* 0x00565eb4 */
    uint32_t d_00565eb8; /* 0x00565eb8 */
    void *p_00565ebc;    /* 0x00565ebc */
    uint32_t d_00565ec0; /* 0x00565ec0 */
    void *p_00565ec4;    /* 0x00565ec4 */
    uint32_t d_00565ec8; /* 0x00565ec8 */
    void *p_00565ecc;    /* 0x00565ecc */
    uint32_t d_00565ed0; /* 0x00565ed0 */
    void *p_00565ed4;    /* 0x00565ed4 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_p_00565e4cGroup, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x00565e40, off64),                                             \
                   #member)
AT(p_00565e4c, 0x00565e4c, 0x10);
AT(d_00565e50, 0x00565e50, 0x18);
AT(p_00565e54, 0x00565e54, 0x20);
AT(d_00565e58, 0x00565e58, 0x28);
AT(p_00565e5c, 0x00565e5c, 0x30);
AT(d_00565e60, 0x00565e60, 0x38);
AT(p_00565e64, 0x00565e64, 0x40);
AT(d_00565e68, 0x00565e68, 0x48);
AT(p_00565e6c, 0x00565e6c, 0x50);
AT(d_00565e70, 0x00565e70, 0x58);
AT(p_00565e74, 0x00565e74, 0x60);
AT(d_00565e78, 0x00565e78, 0x68);
AT(p_00565e7c, 0x00565e7c, 0x70);
AT(d_00565e80, 0x00565e80, 0x78);
AT(p_00565e84, 0x00565e84, 0x80);
AT(d_00565e88, 0x00565e88, 0x88);
AT(p_00565e8c, 0x00565e8c, 0x90);
AT(d_00565e90, 0x00565e90, 0x98);
AT(p_00565e94, 0x00565e94, 0xa0);
AT(d_00565e98, 0x00565e98, 0xa8);
AT(p_00565e9c, 0x00565e9c, 0xb0);
AT(d_00565ea0, 0x00565ea0, 0xb8);
AT(p_00565ea4, 0x00565ea4, 0xc0);
AT(d_00565ea8, 0x00565ea8, 0xc8);
AT(p_00565eac, 0x00565eac, 0xd0);
AT(d_00565eb0, 0x00565eb0, 0xd8);
AT(p_00565eb4, 0x00565eb4, 0xe0);
AT(d_00565eb8, 0x00565eb8, 0xe8);
AT(p_00565ebc, 0x00565ebc, 0xf0);
AT(d_00565ec0, 0x00565ec0, 0xf8);
AT(p_00565ec4, 0x00565ec4, 0x100);
AT(d_00565ec8, 0x00565ec8, 0x108);
AT(p_00565ecc, 0x00565ecc, 0x110);
AT(d_00565ed0, 0x00565ed0, 0x118);
AT(p_00565ed4, 0x00565ed4, 0x120);
#undef AT
_Static_assert(offsetof(struct BmxGroup_p_00565e4cGroup, p_00565ed4) +
                       sizeof(((struct BmxGroup_p_00565e4cGroup *)0)->p_00565ed4) ==
                   BMX_DATA_OFF(0x98, 0x128),
               "p_00565e4cGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00565f14..0x00565fe8.
 */
struct BmxGroup_p_00565f14Group {
    uint8_t lead_[4];    /* nothing: the members' offsets keep their original alignment */
    void *p_00565f14;    /* 0x00565f14 */
    uint32_t d_00565f18; /* 0x00565f18 */
    void *p_00565f1c;    /* 0x00565f1c */
    uint32_t d_00565f20; /* 0x00565f20 */
    void *p_00565f24;    /* 0x00565f24 */
    uint32_t d_00565f28; /* 0x00565f28 */
    void *p_00565f2c;    /* 0x00565f2c */
    uint32_t d_00565f30; /* 0x00565f30 */
    void *p_00565f34;    /* 0x00565f34 */
    uint32_t d_00565f38; /* 0x00565f38 */
    void *p_00565f3c;    /* 0x00565f3c */
    uint32_t d_00565f40; /* 0x00565f40 */
    void *p_00565f44;    /* 0x00565f44 */
    uint32_t d_00565f48; /* 0x00565f48 */
    void *p_00565f4c;    /* 0x00565f4c */
    uint32_t d_00565f50; /* 0x00565f50 */
    void *p_00565f54;    /* 0x00565f54 */
    uint32_t d_00565f58; /* 0x00565f58 */
    void *p_00565f5c;    /* 0x00565f5c */
    uint32_t d_00565f60; /* 0x00565f60 */
    void *p_00565f64;    /* 0x00565f64 */
    uint32_t d_00565f68; /* 0x00565f68 */
    void *p_00565f6c;    /* 0x00565f6c */
    uint32_t d_00565f70; /* 0x00565f70 */
    void *p_00565f74;    /* 0x00565f74 */
    uint32_t d_00565f78; /* 0x00565f78 */
    void *p_00565f7c;    /* 0x00565f7c */
    uint32_t d_00565f80; /* 0x00565f80 */
    void *p_00565f84;    /* 0x00565f84 */
    uint32_t d_00565f88; /* 0x00565f88 */
    void *p_00565f8c;    /* 0x00565f8c */
    uint32_t d_00565f90; /* 0x00565f90 */
    void *p_00565f94;    /* 0x00565f94 */
    uint32_t d_00565f98; /* 0x00565f98 */
    void *p_00565f9c;    /* 0x00565f9c */
    uint32_t d_00565fa0; /* 0x00565fa0 */
    void *p_00565fa4;    /* 0x00565fa4 */
    uint32_t d_00565fa8; /* 0x00565fa8 */
    void *p_00565fac;    /* 0x00565fac */
    uint32_t d_00565fb0; /* 0x00565fb0 */
    void *p_00565fb4;    /* 0x00565fb4 */
    uint32_t d_00565fb8; /* 0x00565fb8 */
    void *p_00565fbc;    /* 0x00565fbc */
    uint32_t d_00565fc0; /* 0x00565fc0 */
    void *p_00565fc4;    /* 0x00565fc4 */
    uint32_t d_00565fc8; /* 0x00565fc8 */
    void *p_00565fcc;    /* 0x00565fcc */
    uint32_t d_00565fd0; /* 0x00565fd0 */
    void *p_00565fd4;    /* 0x00565fd4 */
    uint32_t d_00565fd8; /* 0x00565fd8 */
    void *p_00565fdc;    /* 0x00565fdc */
    uint32_t d_00565fe0; /* 0x00565fe0 */
    void *p_00565fe4;    /* 0x00565fe4 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_p_00565f14Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x00565f10, off64),                                             \
                   #member)
AT(p_00565f14, 0x00565f14, 0x8);
AT(d_00565f18, 0x00565f18, 0x10);
AT(p_00565f1c, 0x00565f1c, 0x18);
AT(d_00565f20, 0x00565f20, 0x20);
AT(p_00565f24, 0x00565f24, 0x28);
AT(d_00565f28, 0x00565f28, 0x30);
AT(p_00565f2c, 0x00565f2c, 0x38);
AT(d_00565f30, 0x00565f30, 0x40);
AT(p_00565f34, 0x00565f34, 0x48);
AT(d_00565f38, 0x00565f38, 0x50);
AT(p_00565f3c, 0x00565f3c, 0x58);
AT(d_00565f40, 0x00565f40, 0x60);
AT(p_00565f44, 0x00565f44, 0x68);
AT(d_00565f48, 0x00565f48, 0x70);
AT(p_00565f4c, 0x00565f4c, 0x78);
AT(d_00565f50, 0x00565f50, 0x80);
AT(p_00565f54, 0x00565f54, 0x88);
AT(d_00565f58, 0x00565f58, 0x90);
AT(p_00565f5c, 0x00565f5c, 0x98);
AT(d_00565f60, 0x00565f60, 0xa0);
AT(p_00565f64, 0x00565f64, 0xa8);
AT(d_00565f68, 0x00565f68, 0xb0);
AT(p_00565f6c, 0x00565f6c, 0xb8);
AT(d_00565f70, 0x00565f70, 0xc0);
AT(p_00565f74, 0x00565f74, 0xc8);
AT(d_00565f78, 0x00565f78, 0xd0);
AT(p_00565f7c, 0x00565f7c, 0xd8);
AT(d_00565f80, 0x00565f80, 0xe0);
AT(p_00565f84, 0x00565f84, 0xe8);
AT(d_00565f88, 0x00565f88, 0xf0);
AT(p_00565f8c, 0x00565f8c, 0xf8);
AT(d_00565f90, 0x00565f90, 0x100);
AT(p_00565f94, 0x00565f94, 0x108);
AT(d_00565f98, 0x00565f98, 0x110);
AT(p_00565f9c, 0x00565f9c, 0x118);
AT(d_00565fa0, 0x00565fa0, 0x120);
AT(p_00565fa4, 0x00565fa4, 0x128);
AT(d_00565fa8, 0x00565fa8, 0x130);
AT(p_00565fac, 0x00565fac, 0x138);
AT(d_00565fb0, 0x00565fb0, 0x140);
AT(p_00565fb4, 0x00565fb4, 0x148);
AT(d_00565fb8, 0x00565fb8, 0x150);
AT(p_00565fbc, 0x00565fbc, 0x158);
AT(d_00565fc0, 0x00565fc0, 0x160);
AT(p_00565fc4, 0x00565fc4, 0x168);
AT(d_00565fc8, 0x00565fc8, 0x170);
AT(p_00565fcc, 0x00565fcc, 0x178);
AT(d_00565fd0, 0x00565fd0, 0x180);
AT(p_00565fd4, 0x00565fd4, 0x188);
AT(d_00565fd8, 0x00565fd8, 0x190);
AT(p_00565fdc, 0x00565fdc, 0x198);
AT(d_00565fe0, 0x00565fe0, 0x1a0);
AT(p_00565fe4, 0x00565fe4, 0x1a8);
#undef AT
_Static_assert(offsetof(struct BmxGroup_p_00565f14Group, p_00565fe4) +
                       sizeof(((struct BmxGroup_p_00565f14Group *)0)->p_00565fe4) ==
                   BMX_DATA_OFF(0xd8, 0x1b0),
               "p_00565f14Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00566028..0x00566032.
 */
struct BmxGroup_p_00566028Group {
    uint8_t lead_[8];              /* nothing: the members' offsets keep their original alignment */
    void *p_00566028[2];           /* 0x00566028 */
    unsigned char pad_00566030[2]; /* 0x00566030 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_p_00566028Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x00566020, off64),                                             \
                   #member)
AT(p_00566028, 0x00566028, 0x8);
AT(pad_00566030, 0x00566030, 0x18);
#undef AT
_Static_assert(offsetof(struct BmxGroup_p_00566028Group, pad_00566030) +
                       sizeof(((struct BmxGroup_p_00566028Group *)0)->pad_00566030) ==
                   BMX_DATA_OFF(0x12, 0x1a),
               "p_00566028Group");

/* The type is local to rw/charset.c. */
typedef uint8_t BmxType_rtCharsetGlyphs[1274];

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00566dc0..0x00566ee0.
 */
struct BmxGroup_rwEngineRegistryGroup {
    RwPluginRegistry rwEngineRegistry; /* 0x00566dc0 */
    uint32_t d_00566dd4[67];           /* 0x00566dd4 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_rwEngineRegistryGroup, member) ==                                \
                       BMX_DATA_OFF((addr) - 0x00566dc0, off64),                                             \
                   #member)
AT(rwEngineRegistry, 0x00566dc0, 0x0);
AT(d_00566dd4, 0x00566dd4, 0x20);
#undef AT
_Static_assert(offsetof(struct BmxGroup_rwEngineRegistryGroup, d_00566dd4) +
                       sizeof(((struct BmxGroup_rwEngineRegistryGroup *)0)->d_00566dd4) ==
                   BMX_DATA_OFF(0x120, 0x12c),
               "rwEngineRegistryGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0056701c..0x00567100.
 */
struct BmxGroup_d_0056701cGroup {
    uint8_t lead_[12]; /* nothing: the members' offsets keep their original alignment */
    /* The D3D7 driver's standards: (slot, function) pairs (src/driver/device.c, DrvStandard). */
    struct {
        int32_t id;
        void *fn;
    } d_0056701c[27];               /* 0x0056701c */
    unsigned char pad_005670f4[12]; /* 0x005670f4 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_d_0056701cGroup, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x00567010, off64),                                             \
                   #member)
AT(d_0056701c, 0x0056701c, 0x10);
AT(pad_005670f4, 0x005670f4, 0x1c0);
#undef AT
_Static_assert(offsetof(struct BmxGroup_d_0056701cGroup, pad_005670f4) +
                       sizeof(((struct BmxGroup_d_0056701cGroup *)0)->pad_005670f4) ==
                   BMX_DATA_OFF(0xf0, 0x1cc),
               "d_0056701cGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0056727c..0x00567300.
 */
struct BmxGroup_d_0056727cGroup {
    uint8_t lead_[12];              /* nothing: the members' offsets keep their original alignment */
    uint32_t d_0056727c[8];         /* 0x0056727c */
    void *p_0056729c;               /* 0x0056729c */
    unsigned char pad_005672a0[92]; /* 0x005672a0 */
    void *p_005672fc;               /* 0x005672fc */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_d_0056727cGroup, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x00567270, off64),                                             \
                   #member)
AT(d_0056727c, 0x0056727c, 0xc);
AT(p_0056729c, 0x0056729c, 0x30);
AT(pad_005672a0, 0x005672a0, 0x38);
AT(p_005672fc, 0x005672fc, 0x98);
#undef AT
_Static_assert(offsetof(struct BmxGroup_d_0056727cGroup, p_005672fc) +
                       sizeof(((struct BmxGroup_d_0056727cGroup *)0)->p_005672fc) ==
                   BMX_DATA_OFF(0x90, 0xa0),
               "d_0056727cGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00567bb0..0x00567c08.
 */
struct BmxGroup_d_00567bb0Group {
    uint32_t d_00567bb0[20]; /* 0x00567bb0 */
    void *p_00567c00[2];     /* 0x00567c00 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_d_00567bb0Group, member) == (addr) - 0x00567bb0, #member)
AT(d_00567bb0, 0x00567bb0);
AT(p_00567c00, 0x00567c00);
#undef AT
_Static_assert(offsetof(struct BmxGroup_d_00567bb0Group, p_00567c00) +
                       sizeof(((struct BmxGroup_d_00567bb0Group *)0)->p_00567c00) ==
                   BMX_DATA_OFF(0x58, 0x60),
               "d_00567bb0Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00567c08..0x00567c30.
 */
struct BmxGroup_p_00567c08Group {
    uint8_t lead_[8];              /* nothing: the members' offsets keep their original alignment */
    void *p_00567c08[8];           /* 0x00567c08 */
    unsigned char pad_00567c28[8]; /* 0x00567c28 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_p_00567c08Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x00567c00, off64),                                             \
                   #member)
AT(p_00567c08, 0x00567c08, 0x8);
AT(pad_00567c28, 0x00567c28, 0x48);
#undef AT
_Static_assert(offsetof(struct BmxGroup_p_00567c08Group, pad_00567c28) +
                       sizeof(((struct BmxGroup_p_00567c08Group *)0)->pad_00567c28) ==
                   BMX_DATA_OFF(0x30, 0x50),
               "p_00567c08Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00569012..0x00569050.
 */
struct BmxGroup_d_00569012Group {
    uint8_t lead_[2];               /* nothing: the members' offsets keep their original alignment */
    unsigned char d_00569012[34];   /* 0x00569012 */
    void *p_00569034[4];            /* 0x00569034 */
    unsigned char pad_00569044[12]; /* 0x00569044 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_d_00569012Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x00569010, off64),                                             \
                   #member)
AT(d_00569012, 0x00569012, 0x2);
AT(p_00569034, 0x00569034, 0x28);
AT(pad_00569044, 0x00569044, 0x48);
#undef AT
_Static_assert(offsetof(struct BmxGroup_d_00569012Group, pad_00569044) +
                       sizeof(((struct BmxGroup_d_00569012Group *)0)->pad_00569044) ==
                   BMX_DATA_OFF(0x40, 0x54),
               "d_00569012Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x005690ae..0x005691b0.
 */
struct BmxGroup_p_005690aeGroup {
    uint8_t lead_[14];             /* nothing: the members' offsets keep their original alignment */
    BmxUnalignedPtr p_005690ae;    /* 0x005690ae */
    BmxUnalignedPtr p_005690b2;    /* 0x005690b2 */
    BmxUnalignedPtr p_005690b6;    /* 0x005690b6 */
    BmxUnalignedPtr p_005690ba;    /* 0x005690ba */
    BmxUnalignedPtr p_005690be;    /* 0x005690be */
    BmxUnalignedPtr p_005690c2;    /* 0x005690c2 */
    BmxUnalignedPtr p_005690c6;    /* 0x005690c6 */
    BmxUnalignedPtr p_005690ca;    /* 0x005690ca */
    BmxUnalignedPtr p_005690ce;    /* 0x005690ce */
    BmxUnalignedPtr p_005690d2;    /* 0x005690d2 */
    BmxUnalignedPtr p_005690d6;    /* 0x005690d6 */
    BmxUnalignedPtr p_005690da;    /* 0x005690da */
    BmxUnalignedPtr p_005690de;    /* 0x005690de */
    BmxUnalignedPtr p_005690e2;    /* 0x005690e2 */
    BmxUnalignedPtr p_005690e6;    /* 0x005690e6 */
    BmxUnalignedPtr p_005690ea;    /* 0x005690ea */
    BmxUnalignedPtr p_005690ee;    /* 0x005690ee */
    BmxUnalignedPtr p_005690f2;    /* 0x005690f2 */
    BmxUnalignedPtr p_005690f6;    /* 0x005690f6 */
    BmxUnalignedPtr p_005690fa;    /* 0x005690fa */
    BmxUnalignedPtr p_005690fe;    /* 0x005690fe */
    BmxUnalignedPtr p_00569102;    /* 0x00569102 */
    BmxUnalignedPtr p_00569106;    /* 0x00569106 */
    BmxUnalignedPtr p_0056910a;    /* 0x0056910a */
    BmxUnalignedPtr p_0056910e;    /* 0x0056910e */
    BmxUnalignedPtr p_00569112;    /* 0x00569112 */
    BmxUnalignedPtr p_00569116;    /* 0x00569116 */
    BmxUnalignedPtr p_0056911a;    /* 0x0056911a */
    BmxUnalignedPtr p_0056911e;    /* 0x0056911e */
    BmxUnalignedPtr p_00569122;    /* 0x00569122 */
    BmxUnalignedPtr p_00569126;    /* 0x00569126 */
    BmxUnalignedPtr p_0056912a;    /* 0x0056912a */
    BmxUnalignedPtr p_0056912e;    /* 0x0056912e */
    BmxUnalignedPtr p_00569132;    /* 0x00569132 */
    BmxUnalignedPtr p_00569136;    /* 0x00569136 */
    BmxUnalignedPtr p_0056913a;    /* 0x0056913a */
    BmxUnalignedPtr p_0056913e;    /* 0x0056913e */
    BmxUnalignedPtr p_00569142;    /* 0x00569142 */
    BmxUnalignedPtr p_00569146;    /* 0x00569146 */
    BmxUnalignedPtr p_0056914a;    /* 0x0056914a */
    BmxUnalignedPtr p_0056914e;    /* 0x0056914e */
    BmxUnalignedPtr p_00569152;    /* 0x00569152 */
    BmxUnalignedPtr p_00569156;    /* 0x00569156 */
    BmxUnalignedPtr p_0056915a;    /* 0x0056915a */
    BmxUnalignedPtr p_0056915e;    /* 0x0056915e */
    BmxUnalignedPtr p_00569162;    /* 0x00569162 */
    BmxUnalignedPtr p_00569166;    /* 0x00569166 */
    BmxUnalignedPtr p_0056916a;    /* 0x0056916a */
    BmxUnalignedPtr p_0056916e;    /* 0x0056916e */
    BmxUnalignedPtr p_00569172;    /* 0x00569172 */
    BmxUnalignedPtr p_00569176;    /* 0x00569176 */
    BmxUnalignedPtr p_0056917a;    /* 0x0056917a */
    BmxUnalignedPtr p_0056917e;    /* 0x0056917e */
    BmxUnalignedPtr p_00569182;    /* 0x00569182 */
    BmxUnalignedPtr p_00569186;    /* 0x00569186 */
    BmxUnalignedPtr p_0056918a;    /* 0x0056918a */
    BmxUnalignedPtr p_0056918e;    /* 0x0056918e */
    BmxUnalignedPtr p_00569192;    /* 0x00569192 */
    BmxUnalignedPtr p_00569196;    /* 0x00569196 */
    BmxUnalignedPtr p_0056919a;    /* 0x0056919a */
    BmxUnalignedPtr p_0056919e;    /* 0x0056919e */
    BmxUnalignedPtr p_005691a2;    /* 0x005691a2 */
    BmxUnalignedPtr p_005691a6;    /* 0x005691a6 */
    BmxUnalignedPtr p_005691aa;    /* 0x005691aa */
    unsigned char pad_005691ae[2]; /* 0x005691ae */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_p_005690aeGroup, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x005690a0, off64),                                             \
                   #member)
AT(p_005690ae, 0x005690ae, 0xe);
AT(p_005690b2, 0x005690b2, 0x16);
AT(p_005690b6, 0x005690b6, 0x1e);
AT(p_005690ba, 0x005690ba, 0x26);
AT(p_005690be, 0x005690be, 0x2e);
AT(p_005690c2, 0x005690c2, 0x36);
AT(p_005690c6, 0x005690c6, 0x3e);
AT(p_005690ca, 0x005690ca, 0x46);
AT(p_005690ce, 0x005690ce, 0x4e);
AT(p_005690d2, 0x005690d2, 0x56);
AT(p_005690d6, 0x005690d6, 0x5e);
AT(p_005690da, 0x005690da, 0x66);
AT(p_005690de, 0x005690de, 0x6e);
AT(p_005690e2, 0x005690e2, 0x76);
AT(p_005690e6, 0x005690e6, 0x7e);
AT(p_005690ea, 0x005690ea, 0x86);
AT(p_005690ee, 0x005690ee, 0x8e);
AT(p_005690f2, 0x005690f2, 0x96);
AT(p_005690f6, 0x005690f6, 0x9e);
AT(p_005690fa, 0x005690fa, 0xa6);
AT(p_005690fe, 0x005690fe, 0xae);
AT(p_00569102, 0x00569102, 0xb6);
AT(p_00569106, 0x00569106, 0xbe);
AT(p_0056910a, 0x0056910a, 0xc6);
AT(p_0056910e, 0x0056910e, 0xce);
AT(p_00569112, 0x00569112, 0xd6);
AT(p_00569116, 0x00569116, 0xde);
AT(p_0056911a, 0x0056911a, 0xe6);
AT(p_0056911e, 0x0056911e, 0xee);
AT(p_00569122, 0x00569122, 0xf6);
AT(p_00569126, 0x00569126, 0xfe);
AT(p_0056912a, 0x0056912a, 0x106);
AT(p_0056912e, 0x0056912e, 0x10e);
AT(p_00569132, 0x00569132, 0x116);
AT(p_00569136, 0x00569136, 0x11e);
AT(p_0056913a, 0x0056913a, 0x126);
AT(p_0056913e, 0x0056913e, 0x12e);
AT(p_00569142, 0x00569142, 0x136);
AT(p_00569146, 0x00569146, 0x13e);
AT(p_0056914a, 0x0056914a, 0x146);
AT(p_0056914e, 0x0056914e, 0x14e);
AT(p_00569152, 0x00569152, 0x156);
AT(p_00569156, 0x00569156, 0x15e);
AT(p_0056915a, 0x0056915a, 0x166);
AT(p_0056915e, 0x0056915e, 0x16e);
AT(p_00569162, 0x00569162, 0x176);
AT(p_00569166, 0x00569166, 0x17e);
AT(p_0056916a, 0x0056916a, 0x186);
AT(p_0056916e, 0x0056916e, 0x18e);
AT(p_00569172, 0x00569172, 0x196);
AT(p_00569176, 0x00569176, 0x19e);
AT(p_0056917a, 0x0056917a, 0x1a6);
AT(p_0056917e, 0x0056917e, 0x1ae);
AT(p_00569182, 0x00569182, 0x1b6);
AT(p_00569186, 0x00569186, 0x1be);
AT(p_0056918a, 0x0056918a, 0x1c6);
AT(p_0056918e, 0x0056918e, 0x1ce);
AT(p_00569192, 0x00569192, 0x1d6);
AT(p_00569196, 0x00569196, 0x1de);
AT(p_0056919a, 0x0056919a, 0x1e6);
AT(p_0056919e, 0x0056919e, 0x1ee);
AT(p_005691a2, 0x005691a2, 0x1f6);
AT(p_005691a6, 0x005691a6, 0x1fe);
AT(p_005691aa, 0x005691aa, 0x206);
AT(pad_005691ae, 0x005691ae, 0x20e);
#undef AT
_Static_assert(offsetof(struct BmxGroup_p_005690aeGroup, pad_005691ae) +
                       sizeof(((struct BmxGroup_p_005690aeGroup *)0)->pad_005691ae) ==
                   BMX_DATA_OFF(0x110, 0x210),
               "p_005690aeGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00569240..0x00569248.
 */
struct BmxGroup_p_00569240Group {
    void *p_00569240;              /* 0x00569240 */
    unsigned char pad_00569244[4]; /* 0x00569244 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_p_00569240Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x00569240, off64),                                             \
                   #member)
AT(p_00569240, 0x00569240, 0x0);
AT(pad_00569244, 0x00569244, 0x8);
#undef AT
_Static_assert(offsetof(struct BmxGroup_p_00569240Group, pad_00569244) +
                       sizeof(((struct BmxGroup_p_00569240Group *)0)->pad_00569244) ==
                   BMX_DATA_OFF(0x8, 0xc),
               "p_00569240Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0056b410..0x0056b448.
 */
struct BmxGroup_gCameraRectGroup {
    CameraRect gCameraRect;         /* 0x0056b410 */
    unsigned char pad_0056b418[48]; /* 0x0056b418 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gCameraRectGroup, member) == (addr) - 0x0056b410, #member)
AT(gCameraRect, 0x0056b410);
AT(pad_0056b418, 0x0056b418);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gCameraRectGroup, pad_0056b418) +
                       sizeof(((struct BmxGroup_gCameraRectGroup *)0)->pad_0056b418) ==
                   0x38,
               "gCameraRectGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0056b5b8..0x0056b614.
 */
struct BmxGroup_gCameraResetGroup {
    uint8_t lead_[8];               /* nothing: the members' offsets keep their original alignment */
    uint32_t gCameraReset;          /* 0x0056b5b8 */
    unsigned char pad_0056b5bc[88]; /* 0x0056b5bc */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gCameraResetGroup, member) == (addr) - 0x0056b5b0, #member)
AT(gCameraReset, 0x0056b5b8);
AT(pad_0056b5bc, 0x0056b5bc);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gCameraResetGroup, pad_0056b5bc) +
                       sizeof(((struct BmxGroup_gCameraResetGroup *)0)->pad_0056b5bc) ==
                   0x64,
               "gCameraResetGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0056b70c..0x0056d278.
 */
struct BmxGroup_gGameRoundStatesGroup {
    uint8_t lead_[12];                /* nothing: the members' offsets keep their original alignment */
    unsigned char d_0056b70c[24];     /* 0x0056b70c, also gGameRecords */
    unsigned char d_0056b724[32];     /* 0x0056b724 */
    unsigned char d_0056b744[8];      /* 0x0056b744 */
    unsigned char d_0056b74c[8];      /* 0x0056b74c */
    unsigned char d_0056b754[8];      /* 0x0056b754 */
    unsigned char d_0056b75c[8];      /* 0x0056b75c */
    unsigned char d_0056b764[8];      /* 0x0056b764 */
    unsigned char d_0056b76c[16];     /* 0x0056b76c */
    unsigned char d_0056b77c[656];    /* 0x0056b77c */
    GameRoundStates gGameRoundStates; /* 0x0056ba0c, also gGameLeader */
    GameHorseWord gGameHorseWord;     /* 0x0056ba1c */
    GameMarkerList gGameMarkersA;     /* 0x0056ba28 */
    GameMarkerList gGameMarkersB;     /* 0x0056bb28 */
    GameExtent gGameExtent;           /* 0x0056bc28 */
    uint32_t gGameFrame;              /* 0x0056bc2c */
    uint32_t gGameFrozen;             /* 0x0056bc30 */
    unsigned char pad_0056bc34[272];  /* 0x0056bc34 */
    OvlText4 gOvlEmpty;               /* 0x0056bd44, also gGuiEmptyChar, rwEmptyString */
    float gOvlHeightScroll;           /* 0x0056bd48 */
    unsigned char pad_0056bd4c[4];    /* 0x0056bd4c */
    uint32_t gOvlScoreShown;          /* 0x0056bd50 */
    unsigned char pad_0056bd54[4];    /* 0x0056bd54 */
    uint32_t gOvlScoreTarget;         /* 0x0056bd58 */
    uint8_t gOvlScoreFlags;           /* 0x0056bd5c */
    unsigned char pad_0056bd5d[3];    /* 0x0056bd5d */
    uint8_t gOvlLastScreen;           /* 0x0056bd60 */
    unsigned char pad_0056bd61[7];    /* 0x0056bd61 */
    uint32_t gOvlTrickCount;          /* 0x0056bd68 */
    OvlTrickList gOvlTrickList;       /* 0x0056bd6c */
    uint32_t gOvlFrame;               /* 0x0056be14 */
    OvlTimedMsg gOvlFlash;            /* 0x0056be18 */
    unsigned char pad_0056be1c[4];    /* 0x0056be1c */
    OvlHandles4 gOvlFlashHandles;     /* 0x0056be20 */
    uint16_t gOvlImgTiny;             /* 0x0056be28 */
    uint8_t gOvlBackdrop;             /* 0x0056be2a */
    unsigned char pad_0056be2b[1];    /* 0x0056be2b */
    uint16_t gOvlScoreHandle;         /* 0x0056be2c */
    unsigned char pad_0056be2e[2];    /* 0x0056be2e */
    uint16_t gOvlResA;                /* 0x0056be30 */
    uint8_t gOvlReturnScreen;         /* 0x0056be32 */
    unsigned char pad_0056be33[1];    /* 0x0056be33 */
    OvlStringSets gOvlStrings;        /* 0x0056be34 */
    uint32_t gOvlSharedPad;           /* 0x0056be3c */
    OvlHandleGrid gOvlRunTimes;       /* 0x0056be40 */
    uint16_t gOvlIconHandle;          /* 0x0056be90 */
    uint16_t gOvlImgIcons;            /* 0x0056be92 */
    uint16_t gPhysHudHandle;          /* 0x0056be94 */
    unsigned char pad_0056be96[2];    /* 0x0056be96 */
    OvlFont gOvlFontTiny;             /* 0x0056be98 */
    OvlHandles7 gOvlVolHandles;       /* 0x0056bea4 */
    unsigned char d_0056beb2[2];      /* 0x0056beb2 */
    OvlHandles3 gOvlGoalHandles;      /* 0x0056beb4 */
    uint8_t gOvlScreenId;             /* 0x0056beba */
    unsigned char pad_0056bebb[1];    /* 0x0056bebb */
    unsigned char d_0056bebc[2];      /* 0x0056bebc, also gOvlHandles */
    unsigned char d_0056bebe[2];      /* 0x0056bebe */
    unsigned char d_0056bec0[2];      /* 0x0056bec0 */
    unsigned char d_0056bec2[2];      /* 0x0056bec2 */
    OvlGoalRows gOvlGoalRows;         /* 0x0056bec4 */
    uint8_t gOvlCaptionCurrent;       /* 0x0056beec */
    uint8_t gOvlCaptionWanted;        /* 0x0056beed */
    unsigned char pad_0056beee[6];    /* 0x0056beee */
    uint16_t gOvlImgSmall;            /* 0x0056bef4 */
    unsigned char pad_0056bef6[2];    /* 0x0056bef6 */
    uint16_t gOvlBackdropRect;        /* 0x0056bef8 */
    unsigned char pad_0056befa[2];    /* 0x0056befa */
    OvlMenuItems7 gOvlMenuItems;      /* 0x0056befc */
    float gOvlBalanceValue;           /* 0x0056bf18 */
    unsigned char pad_0056bf1c[4];    /* 0x0056bf1c */
    OvlScreen *gOvlScreen;            /* 0x0056bf20 */
    OvlHandles6 gOvlHeightHandles;    /* 0x0056bf24 */
    uint8_t gOvlPage;                 /* 0x0056bf30 */
    unsigned char pad_0056bf31[7];    /* 0x0056bf31 */
    OvlFont gPhysHudFont;             /* 0x0056bf38, also gOvlFontSmall */
    OvlTimedMsg gOvlHint;             /* 0x0056bf44 */
    unsigned char pad_0056bf48[4];    /* 0x0056bf48 */
    OvlTimedMsg gOvlGoalMsg;          /* 0x0056bf4c */
    unsigned char pad_0056bf50[4];    /* 0x0056bf50 */
    uint32_t gOvlTimerShownTicks;     /* 0x0056bf54 */
    uint32_t gOvlTimerTicks;          /* 0x0056bf58 */
    OvlIndicator gOvlIndicator;       /* 0x0056bf5c */
    unsigned char pad_0056bf60[4];    /* 0x0056bf60 */
    uint8_t gOvlNameCursor;           /* 0x0056bf64 */
    unsigned char pad_0056bf65[3];    /* 0x0056bf65 */
    uint32_t gOvlPulse;               /* 0x0056bf68 */
    OvlHandles21 gOvlTrickRows;       /* 0x0056bf6c, also gOvlTrickDesc */
    unsigned char d_0056bf96[2];      /* 0x0056bf96 */
    OvlFont gOvlFontIcons;            /* 0x0056bf98 */
    uint32_t gOvlDrawDelay;           /* 0x0056bfa4 */
    uint16_t gOvlStatusHandle;        /* 0x0056bfa8 */
    unsigned char pad_0056bfaa[2];    /* 0x0056bfaa */
    OvlText8 gOvlTimerText;           /* 0x0056bfac */
    unsigned char d_0056bfb4[1];      /* 0x0056bfb4, also gOvlNameBuf */
    unsigned char d_0056bfb5[11];     /* 0x0056bfb5 */
    float gOvlHeightValue;            /* 0x0056bfc0 */
    float gOvlHeightTarget;           /* 0x0056bfc4 */
    unsigned char d_0056bfc8[2];      /* 0x0056bfc8, also gOvlHeightExtra */
    uint16_t gOvlHeightKind;          /* 0x0056bfca */
    unsigned char pad_0056bfcc[13];   /* 0x0056bfcc */
    uint8_t gOvlElement8Value;        /* 0x0056bfd9 */
    unsigned char pad_0056bfda[2];    /* 0x0056bfda */
    OvlHandles7 gOvlHintHandles;      /* 0x0056bfdc */
    unsigned char d_0056bfea[2];      /* 0x0056bfea */
    OvlHandles7 gOvlBalanceHandles;   /* 0x0056bfec */
    uint16_t gOvlImgButton;           /* 0x0056bffa */
    uint16_t gOvlCaptionHandle;       /* 0x0056bffc */
    unsigned char pad_0056bffe[2];    /* 0x0056bffe */
    uint16_t gOvlTimerHandle;         /* 0x0056c000 */
    unsigned char pad_0056c002[6];    /* 0x0056c002 */
    OvlFont gOvlFontButton;           /* 0x0056c008 */
    unsigned char pad_0056c014[4];    /* 0x0056c014 */
    OvlStatusMsg gOvlStatus;          /* 0x0056c018 */
    unsigned char pad_0056c020[8];    /* 0x0056c020 */
    OvlMenuItems10 gOvlMenuItems2;    /* 0x0056c028 */
    OvlFade gOvlFade;                 /* 0x0056c050 */
    OvlTrickView gOvlTrickView;       /* 0x0056c070 */
    uint8_t gOvlSaveSlot;             /* 0x0056c074 */
    unsigned char pad_0056c075[7];    /* 0x0056c075 */
    uint32_t gPhysTick;               /* 0x0056c07c */
    unsigned char pad_0056c080[168];  /* 0x0056c080 */
    PhysRayHit gPhysRay;              /* 0x0056c128 */
    PhysEdge gPhysBestEdge;           /* 0x0056c200 */
    PhysEdge gPhysAlignEdge;          /* 0x0056c238 */
    unsigned char pad_0056c270[80];   /* 0x0056c270 */
    PhysEdge gPhysNearEdge;           /* 0x0056c2c0 */
    PhysEdge gPhysEdgeProbe;          /* 0x0056c2f8 */
    PhysEdge gPhysGrindEdge;          /* 0x0056c330 */
    unsigned char pad_0056c368[80];   /* 0x0056c368 */
    PhysFace gPhysLipFace;            /* 0x0056c3b8 */
    int32_t gPhysForceStep;           /* 0x0056c440 */
    int32_t gPhysDebugDraw;           /* 0x0056c444, also gPhysDebugContact */
    PhysEdge **gPhysDebugEdges;       /* 0x0056c448 */
    uint32_t gPhysDebugFrame;         /* 0x0056c44c */
    unsigned char pad_0056c450[168];  /* 0x0056c450 */
    PropStateBlock gPropDefaults;     /* 0x0056c4f8 */
    unsigned char pad_0056d038[576];  /* 0x0056d038 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gGameRoundStatesGroup, member) ==                                \
                       BMX_DATA_OFF((addr) - 0x0056b700, off64),                                             \
                   #member)
AT(d_0056b70c, 0x0056b70c, 0xc);
AT(d_0056b724, 0x0056b724, 0x24);
AT(d_0056b744, 0x0056b744, 0x44);
AT(d_0056b74c, 0x0056b74c, 0x4c);
AT(d_0056b754, 0x0056b754, 0x54);
AT(d_0056b75c, 0x0056b75c, 0x5c);
AT(d_0056b764, 0x0056b764, 0x64);
AT(d_0056b76c, 0x0056b76c, 0x6c);
AT(d_0056b77c, 0x0056b77c, 0x7c);
AT(gGameRoundStates, 0x0056ba0c, 0x30c);
AT(gGameHorseWord, 0x0056ba1c, 0x31c);
AT(gGameMarkersA, 0x0056ba28, 0x328);
AT(gGameMarkersB, 0x0056bb28, 0x528);
AT(gGameExtent, 0x0056bc28, 0x728);
AT(gGameFrame, 0x0056bc2c, 0x72c);
AT(gGameFrozen, 0x0056bc30, 0x730);
AT(pad_0056bc34, 0x0056bc34, 0x734);
AT(gOvlEmpty, 0x0056bd44, 0x844);
AT(gOvlHeightScroll, 0x0056bd48, 0x848);
AT(pad_0056bd4c, 0x0056bd4c, 0x84c);
AT(gOvlScoreShown, 0x0056bd50, 0x850);
AT(pad_0056bd54, 0x0056bd54, 0x854);
AT(gOvlScoreTarget, 0x0056bd58, 0x858);
AT(gOvlScoreFlags, 0x0056bd5c, 0x85c);
AT(pad_0056bd5d, 0x0056bd5d, 0x85d);
AT(gOvlLastScreen, 0x0056bd60, 0x860);
AT(pad_0056bd61, 0x0056bd61, 0x861);
AT(gOvlTrickCount, 0x0056bd68, 0x868);
AT(gOvlTrickList, 0x0056bd6c, 0x86c);
AT(gOvlFrame, 0x0056be14, 0x914);
AT(gOvlFlash, 0x0056be18, 0x918);
AT(pad_0056be1c, 0x0056be1c, 0x91c);
AT(gOvlFlashHandles, 0x0056be20, 0x920);
AT(gOvlImgTiny, 0x0056be28, 0x928);
AT(gOvlBackdrop, 0x0056be2a, 0x92a);
AT(pad_0056be2b, 0x0056be2b, 0x92b);
AT(gOvlScoreHandle, 0x0056be2c, 0x92c);
AT(pad_0056be2e, 0x0056be2e, 0x92e);
AT(gOvlResA, 0x0056be30, 0x930);
AT(gOvlReturnScreen, 0x0056be32, 0x932);
AT(pad_0056be33, 0x0056be33, 0x933);
AT(gOvlStrings, 0x0056be34, 0x938);
AT(gOvlSharedPad, 0x0056be3c, 0x948);
AT(gOvlRunTimes, 0x0056be40, 0x94c);
AT(gOvlIconHandle, 0x0056be90, 0x99c);
AT(gOvlImgIcons, 0x0056be92, 0x99e);
AT(gPhysHudHandle, 0x0056be94, 0x9a0);
AT(pad_0056be96, 0x0056be96, 0x9a2);
AT(gOvlFontTiny, 0x0056be98, 0x9a8);
AT(gOvlVolHandles, 0x0056bea4, 0x9c0);
AT(d_0056beb2, 0x0056beb2, 0x9ce);
AT(gOvlGoalHandles, 0x0056beb4, 0x9d0);
AT(gOvlScreenId, 0x0056beba, 0x9d6);
AT(pad_0056bebb, 0x0056bebb, 0x9d7);
AT(d_0056bebc, 0x0056bebc, 0x9d8);
AT(d_0056bebe, 0x0056bebe, 0x9da);
AT(d_0056bec0, 0x0056bec0, 0x9dc);
AT(d_0056bec2, 0x0056bec2, 0x9de);
AT(gOvlGoalRows, 0x0056bec4, 0x9e0);
AT(gOvlCaptionCurrent, 0x0056beec, 0xa08);
AT(gOvlCaptionWanted, 0x0056beed, 0xa09);
AT(pad_0056beee, 0x0056beee, 0xa0a);
AT(gOvlImgSmall, 0x0056bef4, 0xa10);
AT(pad_0056bef6, 0x0056bef6, 0xa12);
AT(gOvlBackdropRect, 0x0056bef8, 0xa14);
AT(pad_0056befa, 0x0056befa, 0xa16);
AT(gOvlMenuItems, 0x0056befc, 0xa18);
AT(gOvlBalanceValue, 0x0056bf18, 0xa34);
AT(pad_0056bf1c, 0x0056bf1c, 0xa38);
AT(gOvlScreen, 0x0056bf20, 0xa40);
AT(gOvlHeightHandles, 0x0056bf24, 0xa48);
AT(gOvlPage, 0x0056bf30, 0xa54);
AT(pad_0056bf31, 0x0056bf31, 0xa55);
AT(gPhysHudFont, 0x0056bf38, 0xa60);
AT(gOvlHint, 0x0056bf44, 0xa78);
AT(pad_0056bf48, 0x0056bf48, 0xa7c);
AT(gOvlGoalMsg, 0x0056bf4c, 0xa80);
AT(pad_0056bf50, 0x0056bf50, 0xa84);
AT(gOvlTimerShownTicks, 0x0056bf54, 0xa88);
AT(gOvlTimerTicks, 0x0056bf58, 0xa8c);
AT(gOvlIndicator, 0x0056bf5c, 0xa90);
AT(pad_0056bf60, 0x0056bf60, 0xa94);
AT(gOvlNameCursor, 0x0056bf64, 0xa98);
AT(pad_0056bf65, 0x0056bf65, 0xa99);
AT(gOvlPulse, 0x0056bf68, 0xa9c);
AT(gOvlTrickRows, 0x0056bf6c, 0xaa0);
AT(d_0056bf96, 0x0056bf96, 0xaca);
AT(gOvlFontIcons, 0x0056bf98, 0xad0);
AT(gOvlDrawDelay, 0x0056bfa4, 0xae8);
AT(gOvlStatusHandle, 0x0056bfa8, 0xaec);
AT(pad_0056bfaa, 0x0056bfaa, 0xaee);
AT(gOvlTimerText, 0x0056bfac, 0xaf0);
AT(d_0056bfb4, 0x0056bfb4, 0xaf8);
AT(d_0056bfb5, 0x0056bfb5, 0xaf9);
AT(gOvlHeightValue, 0x0056bfc0, 0xb04);
AT(gOvlHeightTarget, 0x0056bfc4, 0xb08);
AT(d_0056bfc8, 0x0056bfc8, 0xb0c);
AT(gOvlHeightKind, 0x0056bfca, 0xb0e);
AT(pad_0056bfcc, 0x0056bfcc, 0xb10);
AT(gOvlElement8Value, 0x0056bfd9, 0xb1d);
AT(pad_0056bfda, 0x0056bfda, 0xb1e);
AT(gOvlHintHandles, 0x0056bfdc, 0xb20);
AT(d_0056bfea, 0x0056bfea, 0xb2e);
AT(gOvlBalanceHandles, 0x0056bfec, 0xb30);
AT(gOvlImgButton, 0x0056bffa, 0xb3e);
AT(gOvlCaptionHandle, 0x0056bffc, 0xb40);
AT(pad_0056bffe, 0x0056bffe, 0xb42);
AT(gOvlTimerHandle, 0x0056c000, 0xb44);
AT(pad_0056c002, 0x0056c002, 0xb46);
AT(gOvlFontButton, 0x0056c008, 0xb50);
AT(pad_0056c014, 0x0056c014, 0xb68);
AT(gOvlStatus, 0x0056c018, 0xb6c);
AT(pad_0056c020, 0x0056c020, 0xb74);
AT(gOvlMenuItems2, 0x0056c028, 0xb7c);
AT(gOvlFade, 0x0056c050, 0xba4);
AT(gOvlTrickView, 0x0056c070, 0xbc4);
AT(gOvlSaveSlot, 0x0056c074, 0xbc8);
AT(pad_0056c075, 0x0056c075, 0xbc9);
AT(gPhysTick, 0x0056c07c, 0xbd0);
AT(pad_0056c080, 0x0056c080, 0xbd4);
AT(gPhysRay, 0x0056c128, 0xc80);
AT(gPhysBestEdge, 0x0056c200, 0xd90);
AT(gPhysAlignEdge, 0x0056c238, 0xdd0);
AT(pad_0056c270, 0x0056c270, 0xe10);
AT(gPhysNearEdge, 0x0056c2c0, 0xe60);
AT(gPhysEdgeProbe, 0x0056c2f8, 0xea0);
AT(gPhysGrindEdge, 0x0056c330, 0xee0);
AT(pad_0056c368, 0x0056c368, 0xf20);
AT(gPhysLipFace, 0x0056c3b8, 0xf70);
AT(gPhysForceStep, 0x0056c440, 0x1018);
AT(gPhysDebugDraw, 0x0056c444, 0x101c);
AT(gPhysDebugEdges, 0x0056c448, 0x1020);
AT(gPhysDebugFrame, 0x0056c44c, 0x1028);
AT(pad_0056c450, 0x0056c450, 0x102c);
AT(gPropDefaults, 0x0056c4f8, 0x10d4);
AT(pad_0056d038, 0x0056d038, 0x1f54);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gGameRoundStatesGroup, pad_0056d038) +
                       sizeof(((struct BmxGroup_gGameRoundStatesGroup *)0)->pad_0056d038) ==
                   BMX_DATA_OFF(0x1b78, 0x2194),
               "gGameRoundStatesGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00573260..0x00574584.
 */
struct BmxGroup_gGuiSlideMaxGroup {
    int gGuiSlideMax;                 /* 0x00573260 */
    unsigned char pad_00573264[4896]; /* 0x00573264 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gGuiSlideMaxGroup, member) == (addr) - 0x00573260, #member)
AT(gGuiSlideMax, 0x00573260);
AT(pad_00573264, 0x00573264);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gGuiSlideMaxGroup, pad_00573264) +
                       sizeof(((struct BmxGroup_gGuiSlideMaxGroup *)0)->pad_00573264) ==
                   0x1324,
               "gGuiSlideMaxGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0057462c..0x00574644.
 */
struct BmxGroup_gStyleFrameGroup {
    uint8_t lead_[12];              /* nothing: the members' offsets keep their original alignment */
    int gStyleFrame;                /* 0x0057462c */
    unsigned char pad_00574630[20]; /* 0x00574630 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gStyleFrameGroup, member) == (addr) - 0x00574620, #member)
AT(gStyleFrame, 0x0057462c);
AT(pad_00574630, 0x00574630);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gStyleFrameGroup, pad_00574630) +
                       sizeof(((struct BmxGroup_gStyleFrameGroup *)0)->pad_00574630) ==
                   0x24,
               "gStyleFrameGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0057466c..0x00574694.
 */
struct BmxGroup_gBmxTimerIdGroup {
    uint8_t lead_[12];                      /* nothing: the members' offsets keep their original alignment */
    unsigned char d_0057466c[4];            /* 0x0057466c, also gBmxRiderLoopB */
    unsigned char d_00574670[4];            /* 0x00574670, also gBmxRiderLoopA */
    uint32_t gBmxTimerId;                   /* 0x00574674 */
    BmxAmbientTrackTable gBmxAmbientTracks; /* 0x00574678 */
    WinchanEffect gBmxSoundEffect;          /* 0x00574680 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gBmxTimerIdGroup, member) == (addr) - 0x00574660, #member)
AT(d_0057466c, 0x0057466c);
AT(d_00574670, 0x00574670);
AT(gBmxTimerId, 0x00574674);
AT(gBmxAmbientTracks, 0x00574678);
AT(gBmxSoundEffect, 0x00574680);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gBmxTimerIdGroup, gBmxSoundEffect) +
                       sizeof(((struct BmxGroup_gBmxTimerIdGroup *)0)->gBmxSoundEffect) ==
                   0x34,
               "gBmxTimerIdGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x005746b0..0x005746c4.
 */
struct BmxGroup_gLastPollTimeGroup {
    uint32_t gLastPollTime;         /* 0x005746b0 */
    unsigned char pad_005746b4[16]; /* 0x005746b4 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gLastPollTimeGroup, member) == (addr) - 0x005746b0, #member)
AT(gLastPollTime, 0x005746b0);
AT(pad_005746b4, 0x005746b4);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gLastPollTimeGroup, pad_005746b4) +
                       sizeof(((struct BmxGroup_gLastPollTimeGroup *)0)->pad_005746b4) ==
                   0x14,
               "gLastPollTimeGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00574d08..0x00574e50.
 */
struct BmxGroup_gZoverLimbSyncedGroup {
    uint8_t lead_[8];                /* nothing: the members' offsets keep their original alignment */
    int32_t gZoverLimbSynced;        /* 0x00574d08 */
    unsigned char pad_00574d0c[324]; /* 0x00574d0c */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gZoverLimbSyncedGroup, member) == (addr) - 0x00574d00, #member)
AT(gZoverLimbSynced, 0x00574d08);
AT(pad_00574d0c, 0x00574d0c);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZoverLimbSyncedGroup, pad_00574d0c) +
                       sizeof(((struct BmxGroup_gZoverLimbSyncedGroup *)0)->pad_00574d0c) ==
                   0x150,
               "gZoverLimbSyncedGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00596570..0x005965a2.
 */
struct BmxGroup_gZoverRectSizeGroup {
    ZOverRectSize gZoverRectSize;   /* 0x00596570 */
    unsigned char pad_00596578[42]; /* 0x00596578 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gZoverRectSizeGroup, member) == (addr) - 0x00596570, #member)
AT(gZoverRectSize, 0x00596570);
AT(pad_00596578, 0x00596578);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZoverRectSizeGroup, pad_00596578) +
                       sizeof(((struct BmxGroup_gZoverRectSizeGroup *)0)->pad_00596578) ==
                   0x32,
               "gZoverRectSizeGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x005a22bd..0x005ab3a0.
 */
struct BmxGroup_gZoverLayerGroup {
    uint8_t lead_[13];                 /* nothing: the members' offsets keep their original alignment */
    uint8_t gZoverLayer;               /* 0x005a22bd */
    unsigned char pad_005a22be[37090]; /* 0x005a22be */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gZoverLayerGroup, member) == (addr) - 0x005a22b0, #member)
AT(gZoverLayer, 0x005a22bd);
AT(pad_005a22be, 0x005a22be);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZoverLayerGroup, pad_005a22be) +
                       sizeof(((struct BmxGroup_gZoverLayerGroup *)0)->pad_005a22be) ==
                   0x90f0,
               "gZoverLayerGroup");

/* The type is local to bmx/com/zge/ngif/ngapp.c. */
typedef void *BmxType_gNgSpotLights[3];

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x005ab3b8..0x005ab438.
 */
struct BmxGroup_gNgArtNameGroup {
    uint8_t lead_[8];                /* nothing: the members' offsets keep their original alignment */
    char gNgArtName;                 /* 0x005ab3b8 */
    unsigned char pad_005ab3b9[127]; /* 0x005ab3b9 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gNgArtNameGroup, member) == (addr) - 0x005ab3b0, #member)
AT(gNgArtName, 0x005ab3b8);
AT(pad_005ab3b9, 0x005ab3b9);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gNgArtNameGroup, pad_005ab3b9) +
                       sizeof(((struct BmxGroup_gNgArtNameGroup *)0)->pad_005ab3b9) ==
                   0x88,
               "gNgArtNameGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x005ab438..0x005ab454.
 */
struct BmxGroup_gNgLightsGroup {
    uint8_t lead_[8];               /* nothing: the members' offsets keep their original alignment */
    void *gNgLights[3];             /* 0x005ab438; the type is local to bmx/com/zge/ngif/ngapp.c */
    unsigned char pad_005ab444[16]; /* 0x005ab444 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gNgLightsGroup, member) ==                                       \
                       BMX_DATA_OFF((addr) - 0x005ab430, off64),                                             \
                   #member)
AT(gNgLights, 0x005ab438, 0x8);
AT(pad_005ab444, 0x005ab444, 0x20);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gNgLightsGroup, pad_005ab444) +
                       sizeof(((struct BmxGroup_gNgLightsGroup *)0)->pad_005ab444) ==
                   BMX_DATA_OFF(0x24, 0x30),
               "gNgLightsGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x005ab4a8..0x005ab4e0.
 */
struct BmxGroup_gFxQualityStepGroup {
    uint8_t lead_[8];               /* nothing: the members' offsets keep their original alignment */
    float gFxQualityStep;           /* 0x005ab4a8 */
    unsigned char pad_005ab4ac[52]; /* 0x005ab4ac */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gFxQualityStepGroup, member) == (addr) - 0x005ab4a0, #member)
AT(gFxQualityStep, 0x005ab4a8);
AT(pad_005ab4ac, 0x005ab4ac);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gFxQualityStepGroup, pad_005ab4ac) +
                       sizeof(((struct BmxGroup_gFxQualityStepGroup *)0)->pad_005ab4ac) ==
                   0x40,
               "gFxQualityStepGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x005ab4fc..0x005d2500.
 */
struct BmxGroup_gNgifPrimBufferGroup {
    uint8_t lead_[12];                  /* nothing: the members' offsets keep their original alignment */
    NgifWords gNgifPrimBuffer;          /* 0x005ab4fc */
    unsigned char pad_005ab500[159740]; /* 0x005ab500 */
    int gNgifFlushPending;              /* 0x005d24fc */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gNgifPrimBufferGroup, member) == (addr) - 0x005ab4f0, #member)
AT(gNgifPrimBuffer, 0x005ab4fc);
AT(pad_005ab500, 0x005ab500);
AT(gNgifFlushPending, 0x005d24fc);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gNgifPrimBufferGroup, gNgifFlushPending) +
                       sizeof(((struct BmxGroup_gNgifPrimBufferGroup *)0)->gNgifFlushPending) ==
                   0x27010,
               "gNgifPrimBufferGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x005d2510..0x005da1e8.
 */
struct BmxGroup_gPrimCursorGroup {
    uint8_t *gPrimCursor;              /* 0x005d2510, also gNgifPrim */
    unsigned char pad_005d2514[31956]; /* 0x005d2514 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gPrimCursorGroup, member) ==                                     \
                       BMX_DATA_OFF((addr) - 0x005d2510, off64),                                             \
                   #member)
AT(gPrimCursor, 0x005d2510, 0x0);
AT(pad_005d2514, 0x005d2514, 0x8);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gPrimCursorGroup, pad_005d2514) +
                       sizeof(((struct BmxGroup_gPrimCursorGroup *)0)->pad_005d2514) ==
                   BMX_DATA_OFF(0x7cd8, 0x7cdc),
               "gPrimCursorGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x005da2c8..0x005e63d8.
 */
struct BmxGroup_gZactorArmHashesGroup {
    uint8_t lead_[8];                  /* nothing: the members' offsets keep their original alignment */
    ZactorArmHashes gZactorArmHashes;  /* 0x005da2c8 */
    unsigned char pad_005da2e0[49400]; /* 0x005da2e0 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gZactorArmHashesGroup, member) == (addr) - 0x005da2c0, #member)
AT(gZactorArmHashes, 0x005da2c8);
AT(pad_005da2e0, 0x005da2e0);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZactorArmHashesGroup, pad_005da2e0) +
                       sizeof(((struct BmxGroup_gZactorArmHashesGroup *)0)->pad_005da2e0) ==
                   0xc118,
               "gZactorArmHashesGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x005e7fe0..0x005f0b98.
 */
struct BmxGroup_gZbspPropFacesGroup {
    ZbspPropFaceTable gZbspPropFaces; /* 0x005e7fe0 */
    unsigned char pad_005e8068[952];  /* 0x005e8068 */
    ZbspNearFaceTable gZbspNearFaces; /* 0x005e8420 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gZbspPropFacesGroup, member) ==                                  \
                       BMX_DATA_OFF((addr) - 0x005e7fe0, off64),                                             \
                   #member)
AT(gZbspPropFaces, 0x005e7fe0, 0x0);
AT(pad_005e8068, 0x005e8068, 0xa8);
AT(gZbspNearFaces, 0x005e8420, 0x460);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZbspPropFacesGroup, gZbspNearFaces) +
                       sizeof(((struct BmxGroup_gZbspPropFacesGroup *)0)->gZbspNearFaces) ==
                   BMX_DATA_OFF(0x8bb8, 0xabb8),
               "gZbspPropFacesGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x005f10a8..0x005f34ac.
 */
struct BmxGroup_gZbspNearCornersGroup {
    uint8_t lead_[8];                     /* nothing: the members' offsets keep their original alignment */
    ZbspNearCornerTable gZbspNearCorners; /* 0x005f10a8 */
    unsigned char pad_005f3484[40];       /* 0x005f3484 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gZbspNearCornersGroup, member) == (addr) - 0x005f10a0, #member)
AT(gZbspNearCorners, 0x005f10a8);
AT(pad_005f3484, 0x005f3484);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZbspNearCornersGroup, pad_005f3484) +
                       sizeof(((struct BmxGroup_gZbspNearCornersGroup *)0)->pad_005f3484) ==
                   0x240c,
               "gZbspNearCornersGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x005f34bc..0x005f8518.
 */
struct BmxGroup_gZbspPropFaceCountGroup {
    uint8_t lead_[12];                 /* nothing: the members' offsets keep their original alignment */
    uint32_t gZbspPropFaceCount;       /* 0x005f34bc */
    unsigned char pad_005f34c0[20568]; /* 0x005f34c0 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gZbspPropFaceCountGroup, member) == (addr) - 0x005f34b0, #member)
AT(gZbspPropFaceCount, 0x005f34bc);
AT(pad_005f34c0, 0x005f34c0);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZbspPropFaceCountGroup, pad_005f34c0) +
                       sizeof(((struct BmxGroup_gZbspPropFaceCountGroup *)0)->pad_005f34c0) ==
                   0x5068,
               "gZbspPropFaceCountGroup");

/* The type is local to bmx/com/zge/ngif/ngifzcollide.c. */
typedef struct {
    uint32_t flags;
    uint32_t extra;
} BmxType_gZcollideProbeMaterial;

/* The type is local to bmx/com/zge/ngif/ngifzcollide.c. */
typedef void *BmxType_gZcollideEdgeList[32];

/* The type is local to bmx/com/zge/ngif/ngifzcollide.c. */
typedef struct {
    uint8_t side;
    uint8_t pad001[3];
    int8_t kind;
    uint8_t pad005[3];
    int16_t a[3];
    int16_t b[3];
    int16_t length;
    uint16_t tag;
    struct {
        float x;
        float y;
        float z;
    } dir;
    struct {
        float x;
        float y;
        float z;
    } normal;
    float d;
    void *prop;
} BmxType_gZcollideEdgeRecords[33];

/* The type is local to bmx/com/zge/ngif/ngifzcollide.c. */
typedef void *BmxType_gZcollideEdgeCells[34];

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x005f8d68..0x005f8f30.
 */
struct BmxGroup_gZbspNoShapeKindGroup {
    uint8_t lead_[8];                /* nothing: the members' offsets keep their original alignment */
    uint8_t gZbspNoShapeKind;        /* 0x005f8d68 */
    unsigned char pad_005f8d69[455]; /* 0x005f8d69 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gZbspNoShapeKindGroup, member) == (addr) - 0x005f8d60, #member)
AT(gZbspNoShapeKind, 0x005f8d68);
AT(pad_005f8d69, 0x005f8d69);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZbspNoShapeKindGroup, pad_005f8d69) +
                       sizeof(((struct BmxGroup_gZbspNoShapeKindGroup *)0)->pad_005f8d69) ==
                   0x1d0,
               "gZbspNoShapeKindGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x005f8f60..0x005fb6c8.
 */
struct BmxGroup_gZresGroup {
    ZresState gZres;                  /* 0x005f8f60 */
    unsigned char pad_005fb034[1684]; /* 0x005fb034 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gZresGroup, member) == BMX_DATA_OFF((addr) - 0x005f8f60, off64), \
                   #member)
AT(gZres, 0x005f8f60, 0x0);
AT(pad_005fb034, 0x005fb034, 0x2108);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZresGroup, pad_005fb034) +
                       sizeof(((struct BmxGroup_gZresGroup *)0)->pad_005fb034) ==
                   BMX_DATA_OFF(0x2768, 0x279c),
               "gZresGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x005fb740..0x005fb760.
 */
struct BmxGroup_gNgRollerNodeGroup {
    ZrwUpdateNode *gNgRollerNode;   /* 0x005fb740, also gZrollerUpdateHandle */
    unsigned char pad_005fb744[28]; /* 0x005fb744 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gNgRollerNodeGroup, member) ==                                   \
                       BMX_DATA_OFF((addr) - 0x005fb740, off64),                                             \
                   #member)
AT(gNgRollerNode, 0x005fb740, 0x0);
AT(pad_005fb744, 0x005fb744, 0x8);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gNgRollerNodeGroup, pad_005fb744) +
                       sizeof(((struct BmxGroup_gNgRollerNodeGroup *)0)->pad_005fb744) ==
                   BMX_DATA_OFF(0x20, 0x24),
               "gNgRollerNodeGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x005fbe38..0x005fbebc.
 */
struct BmxGroup_gWinchanMixLeftGroup {
    uint8_t lead_[8];              /* nothing: the members' offsets keep their original alignment */
    unsigned char d_005fbe38[1];   /* 0x005fbe38, also gWinchanHandles */
    unsigned char d_005fbe39[127]; /* 0x005fbe39 */
    int32_t gWinchanMixLeft;       /* 0x005fbeb8 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gWinchanMixLeftGroup, member) == (addr) - 0x005fbe30, #member)
AT(d_005fbe38, 0x005fbe38);
AT(d_005fbe39, 0x005fbe39);
AT(gWinchanMixLeft, 0x005fbeb8);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gWinchanMixLeftGroup, gWinchanMixLeft) +
                       sizeof(((struct BmxGroup_gWinchanMixLeftGroup *)0)->gWinchanMixLeft) ==
                   0x8c,
               "gWinchanMixLeftGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x005fff28..0x00600d88.
 */
struct BmxGroup_gWinstrmDirectSoundGroup {
    uint8_t lead_[8];                 /* nothing: the members' offsets keep their original alignment */
    DsDevice *gWinstrmDirectSound;    /* 0x005fff28, also gWinchanDirectSound */
    unsigned char pad_005fff2c[3676]; /* 0x005fff2c */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gWinstrmDirectSoundGroup, member) ==                             \
                       BMX_DATA_OFF((addr) - 0x005fff20, off64),                                             \
                   #member)
AT(gWinstrmDirectSound, 0x005fff28, 0x8);
AT(pad_005fff2c, 0x005fff2c, 0x10);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gWinstrmDirectSoundGroup, pad_005fff2c) +
                       sizeof(((struct BmxGroup_gWinstrmDirectSoundGroup *)0)->pad_005fff2c) ==
                   BMX_DATA_OFF(0xe68, 0xe6c),
               "gWinstrmDirectSoundGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00601218..0x00601328.
 */
struct BmxGroup_gWinloadCountsGroup {
    uint8_t lead_[8];                /* nothing: the members' offsets keep their original alignment */
    WinloadCounts gWinloadCounts;    /* 0x00601218 */
    unsigned char pad_00601234[244]; /* 0x00601234 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gWinloadCountsGroup, member) == (addr) - 0x00601210, #member)
AT(gWinloadCounts, 0x00601218);
AT(pad_00601234, 0x00601234);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gWinloadCountsGroup, pad_00601234) +
                       sizeof(((struct BmxGroup_gWinloadCountsGroup *)0)->pad_00601234) ==
                   0x118,
               "gWinloadCountsGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00601b80..0x00601d30.
 */
struct BmxGroup_gWinstrmTickGroup {
    int32_t gWinstrmTick;            /* 0x00601b80 */
    unsigned char pad_00601b84[428]; /* 0x00601b84 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gWinstrmTickGroup, member) == (addr) - 0x00601b80, #member)
AT(gWinstrmTick, 0x00601b80);
AT(pad_00601b84, 0x00601b84);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gWinstrmTickGroup, pad_00601b84) +
                       sizeof(((struct BmxGroup_gWinstrmTickGroup *)0)->pad_00601b84) ==
                   0x1b0,
               "gWinstrmTickGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00601dc8..0x00601e08.
 */
struct BmxGroup_gMotionDeltaEGroup {
    uint8_t lead_[8];               /* nothing: the members' offsets keep their original alignment */
    ZMotionBankTable gMotionBanks;  /* 0x00601dc8 */
    unsigned char pad_00601ddc[4];  /* 0x00601ddc */
    ZMotionDelta gMotionDeltaE;     /* 0x00601de0 */
    unsigned char pad_00601df8[16]; /* 0x00601df8 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gMotionDeltaEGroup, member) ==                                   \
                       BMX_DATA_OFF((addr) - 0x00601dc0, off64),                                             \
                   #member)
AT(gMotionBanks, 0x00601dc8, 0x8);
AT(pad_00601ddc, 0x00601ddc, 0x30);
AT(gMotionDeltaE, 0x00601de0, 0x34);
AT(pad_00601df8, 0x00601df8, 0x4c);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gMotionDeltaEGroup, pad_00601df8) +
                       sizeof(((struct BmxGroup_gMotionDeltaEGroup *)0)->pad_00601df8) ==
                   BMX_DATA_OFF(0x48, 0x5c),
               "gMotionDeltaEGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00641e30..0x0064eb34.
 */
struct BmxGroup_gMotionBufferGroup {
    void *gMotionBuffer;               /* 0x00641e30 */
    unsigned char pad_00641e34[52480]; /* 0x00641e34 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gMotionBufferGroup, member) ==                                   \
                       BMX_DATA_OFF((addr) - 0x00641e30, off64),                                             \
                   #member)
AT(gMotionBuffer, 0x00641e30, 0x0);
AT(pad_00641e34, 0x00641e34, 0x8);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gMotionBufferGroup, pad_00641e34) +
                       sizeof(((struct BmxGroup_gMotionBufferGroup *)0)->pad_00641e34) ==
                   BMX_DATA_OFF(0xcd04, 0xcd08),
               "gMotionBufferGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0064f9c8..0x0064fcb8.
 */
struct BmxGroup_gZsndSeqLockGroup {
    uint8_t lead_[8];                /* nothing: the members' offsets keep their original alignment */
    int32_t gZsndSeqLock;            /* 0x0064f9c8 */
    unsigned char pad_0064f9cc[748]; /* 0x0064f9cc */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gZsndSeqLockGroup, member) == (addr) - 0x0064f9c0, #member)
AT(gZsndSeqLock, 0x0064f9c8);
AT(pad_0064f9cc, 0x0064f9cc);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZsndSeqLockGroup, pad_0064f9cc) +
                       sizeof(((struct BmxGroup_gZsndSeqLockGroup *)0)->pad_0064f9cc) ==
                   0x2f8,
               "gZsndSeqLockGroup");

/* The type is local to zge/com/zglrw/zrwdbg.c. */
typedef struct {
    struct {
        uint32_t head;
        void *verts[2];
    } boxPolys[12];
    struct {
        float x;
        float y;
        float z;
        uint32_t pad0c;
        uint32_t u;
        uint32_t v;
        uint8_t color[4];
    } lineVerts[2];
    struct {
        float x;
        float y;
        float z;
        uint32_t pad0c;
        uint32_t u;
        uint32_t v;
        uint8_t color[4];
    } triVerts[3];
    uint32_t pad65029c;
    struct {
        float x;
        float y;
        float z;
        uint32_t pad0c;
        uint32_t u;
        uint32_t v;
        uint8_t color[4];
    } boxVerts[8];
    struct {
        uint32_t head;
        void *verts[3];
    } triPoly;
    struct {
        float x;
        float y;
        float z;
        uint32_t pad0c;
        uint32_t u;
        uint32_t v;
        uint8_t color[4];
    } axesVerts[6];
    struct {
        uint32_t head;
        void *verts[2];
    } linePoly;
    uint32_t pad650444;
    struct {
        uint32_t head;
        void *verts[2];
    } axesPolys[3];
} BmxType_gScratch;

/* The type is local to zge/com/zglrw/zrwdbg.c. */
typedef char BmxType_gFormatBuffer[512];

/* The type is local to zge/com/zglrw/zrwdbg.c. */
typedef uint8_t BmxType_gTextQueue[1024];

/* The type is local to zge/com/zglrw/zrwdbg.c. */
typedef struct {
    uint32_t type;
    union {
        uint32_t words[26];
        struct {
            struct {
                uint32_t x;
                uint32_t y;
                uint32_t z;
                uint32_t pad0c;
            } point[2];
            uint8_t color[4];
            uint16_t flags;
        } line;
        struct {
            struct {
                struct {
                    float x;
                    float y;
                    float z;
                } right;
                uint32_t flags;
                struct {
                    float x;
                    float y;
                    float z;
                } up;
                uint32_t pad1;
                struct {
                    float x;
                    float y;
                    float z;
                } at;
                uint32_t pad2;
                struct {
                    float x;
                    float y;
                    float z;
                } pos;
                uint32_t pad3;
            } matrix;
            uint16_t mode;
            uint16_t flags;
        } axes;
        struct {
            struct {
                uint32_t x;
                uint32_t y;
                uint32_t z;
                uint32_t pad0c;
            } point[3];
            uint8_t color[4];
            uint16_t flags;
        } triangle;
        struct {
            struct {
                uint32_t x;
                uint32_t y;
                uint32_t z;
                uint32_t pad0c;
            } center;
            uint32_t radius;
            uint8_t color[4];
            uint16_t flags;
        } sphere;
        struct {
            struct {
                uint32_t x;
                uint32_t y;
                uint32_t z;
                uint32_t pad0c;
            } corner[2];
            struct {
                struct {
                    float x;
                    float y;
                    float z;
                } right;
                uint32_t flags;
                struct {
                    float x;
                    float y;
                    float z;
                } up;
                uint32_t pad1;
                struct {
                    float x;
                    float y;
                    float z;
                } at;
                uint32_t pad2;
                struct {
                    float x;
                    float y;
                    float z;
                } pos;
                uint32_t pad3;
            } matrix;
            uint8_t color[4];
            uint16_t flags;
        } box;
    } data;
} BmxType_gItems[512];

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x006702d8..0x006702f8.
 */
struct BmxGroup_gZmathOriginGroup {
    uint8_t lead_[8];               /* nothing: the members' offsets keep their original alignment */
    ZVec3 gZmathOrigin;             /* 0x006702d8, also gPhysZero */
    unsigned char pad_006702e4[20]; /* 0x006702e4 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gZmathOriginGroup, member) == (addr) - 0x006702d0, #member)
AT(gZmathOrigin, 0x006702d8);
AT(pad_006702e4, 0x006702e4);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZmathOriginGroup, pad_006702e4) +
                       sizeof(((struct BmxGroup_gZmathOriginGroup *)0)->pad_006702e4) ==
                   0x28,
               "gZmathOriginGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00672318..0x006723cc.
 */
struct BmxGroup_gZrwMatGroup {
    uint8_t lead_[8];             /* nothing: the members' offsets keep their original alignment */
    unsigned char d_00672318[8];  /* 0x00672318, also gZrwMadfRec */
    unsigned char d_00672320[72]; /* 0x00672320 */
    unsigned char d_00672368[64]; /* 0x00672368 */
    ZrwMatState gZrwMat;          /* 0x006723a8, also gZglMaterials */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gZrwMatGroup, member) == (addr) - 0x00672310, #member)
AT(d_00672318, 0x00672318);
AT(d_00672320, 0x00672320);
AT(d_00672368, 0x00672368);
AT(gZrwMat, 0x006723a8);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZrwMatGroup, gZrwMat) +
                       sizeof(((struct BmxGroup_gZrwMatGroup *)0)->gZrwMat) ==
                   BMX_DATA_OFF(0xbc, 0xd0),
               "gZrwMatGroup");

/* The type is local to zge/com/zglrw/zrwzgl.c. */
typedef char BmxType_gEpicMessage[256];

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x006724d0..0x00672750.
 */
struct BmxGroup_gZglSubSystemNamesGroup {
    uint8_t gZglSubSystemNames;      /* 0x006724d0 */
    unsigned char pad_006724d1[639]; /* 0x006724d1 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gZglSubSystemNamesGroup, member) == (addr) - 0x006724d0, #member)
AT(gZglSubSystemNames, 0x006724d0);
AT(pad_006724d1, 0x006724d1);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZglSubSystemNamesGroup, pad_006724d1) +
                       sizeof(((struct BmxGroup_gZglSubSystemNamesGroup *)0)->pad_006724d1) ==
                   0x280,
               "gZglSubSystemNamesGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00672808..0x0067288c.
 */
struct BmxGroup_gZrwArgBufferGroup {
    uint8_t lead_[8];                /* nothing: the members' offsets keep their original alignment */
    char gZrwArgBuffer;              /* 0x00672808 */
    unsigned char pad_00672809[131]; /* 0x00672809 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gZrwArgBufferGroup, member) == (addr) - 0x00672800, #member)
AT(gZrwArgBuffer, 0x00672808);
AT(pad_00672809, 0x00672809);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZrwArgBufferGroup, pad_00672809) +
                       sizeof(((struct BmxGroup_gZrwArgBufferGroup *)0)->pad_00672809) ==
                   0x8c,
               "gZrwArgBufferGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x006728c4..0x00672900.
 */
struct BmxGroup_gZtaginfRootGroup {
    uint8_t lead_[4];              /* nothing: the members' offsets keep their original alignment */
    ZtaginfNode gZtaginfRoot;      /* 0x006728c4, also gZrwColRoot */
    unsigned char pad_006728f8[4]; /* 0x006728f8 */
    void *gZtaginfData;            /* 0x006728fc */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gZtaginfRootGroup, member) ==                                    \
                       BMX_DATA_OFF((addr) - 0x006728c0, off64),                                             \
                   #member)
AT(gZtaginfRoot, 0x006728c4, 0x8);
AT(pad_006728f8, 0x006728f8, 0x48);
AT(gZtaginfData, 0x006728fc, 0x50);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZtaginfRootGroup, gZtaginfData) +
                       sizeof(((struct BmxGroup_gZtaginfRootGroup *)0)->gZtaginfData) ==
                   BMX_DATA_OFF(0x40, 0x58),
               "gZtaginfRootGroup");

/* zrwmodelrender.c names the array type locally (ZrwRenderColList). */
typedef ZrwRenderColHit BmxType_gZrwColList[64];

/* The world's state: zrwworld.c clears it in one memset from gZrwWorldReady, 0x006741e0..0x0067428c. */
struct BmxGroup_bmxZrwWorldState {
    int gZrwWorldReady;              /* 0x006741e0 */
    int gZrwWorldActive;             /* 0x006741e4 */
    ZList *gZrwWorldMaterials;       /* 0x006741e8 */
    ZrwWorldStats gZrwWorldStats;    /* 0x006741ec, also gZrwWorldPvsState */
    char gZrwWorldName;              /* 0x00674208, also gZrwPvsBuffer */
    unsigned char pad_00674209[131]; /* 0x00674209 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_bmxZrwWorldState, member) ==                                     \
                       BMX_DATA_OFF((addr) - 0x006741e0, off64),                                             \
                   #member)
AT(gZrwWorldReady, 0x006741e0, 0x0);
AT(gZrwWorldActive, 0x006741e4, 0x4);
AT(gZrwWorldMaterials, 0x006741e8, 0x8);
AT(gZrwWorldStats, 0x006741ec, 0x10);
AT(gZrwWorldName, 0x00674208, 0x2c);
AT(pad_00674209, 0x00674209, 0x2d);
#undef AT
_Static_assert(offsetof(struct BmxGroup_bmxZrwWorldState, pad_00674209) +
                       sizeof(((struct BmxGroup_bmxZrwWorldState *)0)->pad_00674209) ==
                   BMX_DATA_OFF(0xac, 0xb0),
               "bmxZrwWorldState");

/* The type is local to zge/com/zglrw/zrwfile.c. */
typedef char BmxType_gZrwNameBuffer[128];

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00676768..0x00676784.
 */
struct BmxGroup_rpWorldDefaultSurfacePropsGroup {
    uint8_t lead_[8]; /* nothing: the members' offsets keep their original alignment */
    RwSurfaceProperties rpWorldDefaultSurfaceProps; /* 0x00676768 */
    unsigned char pad_00676774[16];                 /* 0x00676774 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_rpWorldDefaultSurfacePropsGroup, member) == (addr) - 0x00676760, \
                   #member)
AT(rpWorldDefaultSurfaceProps, 0x00676768);
AT(pad_00676774, 0x00676774);
#undef AT
_Static_assert(offsetof(struct BmxGroup_rpWorldDefaultSurfacePropsGroup, pad_00676774) +
                       sizeof(((struct BmxGroup_rpWorldDefaultSurfacePropsGroup *)0)->pad_00676774) ==
                   0x24,
               "rpWorldDefaultSurfacePropsGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x006767d0..0x006767e4.
 */
struct BmxGroup_rpMeshStripModeGroup {
    int32_t rpMeshStripMode;        /* 0x006767d0 */
    unsigned char pad_006767d4[16]; /* 0x006767d4 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_rpMeshStripModeGroup, member) == (addr) - 0x006767d0, #member)
AT(rpMeshStripMode, 0x006767d0);
AT(pad_006767d4, 0x006767d4);
#undef AT
_Static_assert(offsetof(struct BmxGroup_rpMeshStripModeGroup, pad_006767d4) +
                       sizeof(((struct BmxGroup_rpMeshStripModeGroup *)0)->pad_006767d4) ==
                   0x14,
               "rpMeshStripModeGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00676d90..0x00676da4.
 */
struct BmxGroup_rwEmptyStringGroup {
    char rwEmptyString;             /* 0x00676d90 */
    unsigned char pad_00676d91[19]; /* 0x00676d91 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_rwEmptyStringGroup, member) == (addr) - 0x00676d90, #member)
AT(rwEmptyString, 0x00676d90);
AT(pad_00676d91, 0x00676d91);
#undef AT
_Static_assert(offsetof(struct BmxGroup_rwEmptyStringGroup, pad_00676d91) +
                       sizeof(((struct BmxGroup_rwEmptyStringGroup *)0)->pad_00676d91) ==
                   0x14,
               "rwEmptyStringGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00678440..0x0067847c.
 */
struct BmxGroup_gZrollerShadowGroup {
    ZrollerShadow gZrollerShadow;   /* 0x00678440 */
    unsigned char pad_00678468[20]; /* 0x00678468 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gZrollerShadowGroup, member) ==                                  \
                       BMX_DATA_OFF((addr) - 0x00678440, off64),                                             \
                   #member)
AT(gZrollerShadow, 0x00678440, 0x0);
AT(pad_00678468, 0x00678468, 0x38);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZrollerShadowGroup, pad_00678468) +
                       sizeof(((struct BmxGroup_gZrollerShadowGroup *)0)->pad_00678468) ==
                   BMX_DATA_OFF(0x3c, 0x4c),
               "gZrollerShadowGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00678494..0x00678560.
 */
struct BmxGroup_gZrollerEdgesGroup {
    uint8_t lead_[4];                /* nothing: the members' offsets keep their original alignment */
    ZList *gZrollerEdges;            /* 0x00678494, also gNgRoller */
    unsigned char pad_00678498[200]; /* 0x00678498 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gZrollerEdgesGroup, member) ==                                   \
                       BMX_DATA_OFF((addr) - 0x00678490, off64),                                             \
                   #member)
AT(gZrollerEdges, 0x00678494, 0x8);
AT(pad_00678498, 0x00678498, 0x10);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZrollerEdgesGroup, pad_00678498) +
                       sizeof(((struct BmxGroup_gZrollerEdgesGroup *)0)->pad_00678498) ==
                   BMX_DATA_OFF(0xd0, 0xd8),
               "gZrollerEdgesGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00678560..0x0068c3d4.
 */
struct BmxGroup_gNgifPrim3DBufferGroup {
    NgifWords gNgifPrim3DBuffer;       /* 0x00678560 */
    unsigned char pad_00678564[81516]; /* 0x00678564 */
    uint32_t *gNgifOrderTable;         /* 0x0068c3d0 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gNgifPrim3DBufferGroup, member) == (addr) - 0x00678560, #member)
AT(gNgifPrim3DBuffer, 0x00678560);
AT(pad_00678564, 0x00678564);
AT(gNgifOrderTable, 0x0068c3d0);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gNgifPrim3DBufferGroup, gNgifOrderTable) +
                       sizeof(((struct BmxGroup_gNgifPrim3DBufferGroup *)0)->gNgifOrderTable) ==
                   BMX_DATA_OFF(0x13e74, 0x13e78),
               "gNgifPrim3DBufferGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0068fc00..0x0068fc20.
 */
struct BmxGroup_gNgifColorGroup {
    uint32_t gNgifColor;            /* 0x0068fc00 */
    unsigned char pad_0068fc04[28]; /* 0x0068fc04 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gNgifColorGroup, member) == (addr) - 0x0068fc00, #member)
AT(gNgifColor, 0x0068fc00);
AT(pad_0068fc04, 0x0068fc04);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gNgifColorGroup, pad_0068fc04) +
                       sizeof(((struct BmxGroup_gNgifColorGroup *)0)->pad_0068fc04) ==
                   0x20,
               "gNgifColorGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0068fc20..0x0068fcf8.
 */
struct BmxGroup_gFxDustTexturesGroup {
    uint32_t gFxDustTextures[16];    /* 0x0068fc20; the type is local to bmx/com/zge/ngif/ngiffx.c */
    unsigned char pad_0068fc60[152]; /* 0x0068fc60 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gFxDustTexturesGroup, member) == (addr) - 0x0068fc20, #member)
AT(gFxDustTextures, 0x0068fc20);
AT(pad_0068fc60, 0x0068fc60);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gFxDustTexturesGroup, pad_0068fc60) +
                       sizeof(((struct BmxGroup_gFxDustTexturesGroup *)0)->pad_0068fc60) ==
                   0xd8,
               "gFxDustTexturesGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0068ffa8..0x0068ffc0.
 */
struct BmxGroup_gZresReplayLibGroup {
    uint8_t lead_[8];                     /* nothing: the members' offsets keep their original alignment */
    struct ZassetLibrary *gZresReplayLib; /* 0x0068ffa8, also gActionReplayLib */
    unsigned char pad_0068ffac[20];       /* 0x0068ffac */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gZresReplayLibGroup, member) ==                                  \
                       BMX_DATA_OFF((addr) - 0x0068ffa0, off64),                                             \
                   #member)
AT(gZresReplayLib, 0x0068ffa8, 0x8);
AT(pad_0068ffac, 0x0068ffac, 0x10);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZresReplayLibGroup, pad_0068ffac) +
                       sizeof(((struct BmxGroup_gZresReplayLibGroup *)0)->pad_0068ffac) ==
                   BMX_DATA_OFF(0x20, 0x24),
               "gZresReplayLibGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0068ffc0..0x0068fff0.
 */
struct BmxGroup_gZoverSweepGroup {
    struct {
        float mid;
        float depth;
        float planeDist;
        float zero;
        struct {
            float x;
            float y;
            float z;
        } normal;
    } gZoverSweep;                  /* 0x0068ffc0; the type is local to bmx/com/zge/bmx/zoverii.c */
    unsigned char pad_0068ffdc[20]; /* 0x0068ffdc */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gZoverSweepGroup, member) == (addr) - 0x0068ffc0, #member)
AT(gZoverSweep, 0x0068ffc0);
AT(pad_0068ffdc, 0x0068ffdc);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZoverSweepGroup, pad_0068ffdc) +
                       sizeof(((struct BmxGroup_gZoverSweepGroup *)0)->pad_0068ffdc) ==
                   0x30,
               "gZoverSweepGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0068fff0..0x006904d0.
 */
struct BmxGroup_gExtrasTitlesGroup {
    ExtrasTitleList gExtrasTitles;    /* 0x0068fff0 */
    unsigned char pad_00690000[1232]; /* 0x00690000 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gExtrasTitlesGroup, member) ==                                   \
                       BMX_DATA_OFF((addr) - 0x0068fff0, off64),                                             \
                   #member)
AT(gExtrasTitles, 0x0068fff0, 0x0);
AT(pad_00690000, 0x00690000, 0x20);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gExtrasTitlesGroup, pad_00690000) +
                       sizeof(((struct BmxGroup_gExtrasTitlesGroup *)0)->pad_00690000) ==
                   BMX_DATA_OFF(0x4e0, 0x4f0),
               "gExtrasTitlesGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00690898..0x00690990.
 */
struct BmxGroup_gInfoWaitGroup {
    uint8_t lead_[8];                /* nothing: the members' offsets keep their original alignment */
    uint16_t gInfoWait;              /* 0x00690898 */
    unsigned char pad_0069089a[246]; /* 0x0069089a */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gInfoWaitGroup, member) == (addr) - 0x00690890, #member)
AT(gInfoWait, 0x00690898);
AT(pad_0069089a, 0x0069089a);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gInfoWaitGroup, pad_0069089a) +
                       sizeof(((struct BmxGroup_gInfoWaitGroup *)0)->pad_0069089a) ==
                   0x100,
               "gInfoWaitGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0069123c..0x00691330.
 */
struct BmxGroup_gHiscoreSpriteGroup {
    uint8_t lead_[12];               /* nothing: the members' offsets keep their original alignment */
    ZPrimAsset *gHiscoreSprite;      /* 0x0069123c */
    unsigned char pad_00691240[240]; /* 0x00691240 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gHiscoreSpriteGroup, member) ==                                  \
                       BMX_DATA_OFF((addr) - 0x00691230, off64),                                             \
                   #member)
AT(gHiscoreSprite, 0x0069123c, 0x10);
AT(pad_00691240, 0x00691240, 0x18);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gHiscoreSpriteGroup, pad_00691240) +
                       sizeof(((struct BmxGroup_gHiscoreSpriteGroup *)0)->pad_00691240) ==
                   BMX_DATA_OFF(0x100, 0x108),
               "gHiscoreSpriteGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00691704..0x0069181f.
 */
struct BmxGroup_gSelTickGroup {
    uint8_t lead_[4];                /* nothing: the members' offsets keep their original alignment */
    int gSelTick;                    /* 0x00691704 */
    unsigned char pad_00691708[279]; /* 0x00691708 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gSelTickGroup, member) == (addr) - 0x00691700, #member)
AT(gSelTick, 0x00691704);
AT(pad_00691708, 0x00691708);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gSelTickGroup, pad_00691708) +
                       sizeof(((struct BmxGroup_gSelTickGroup *)0)->pad_00691708) ==
                   0x11f,
               "gSelTickGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00691d08..0x00691d58.
 */
struct BmxGroup_gSelLevelCountGroup {
    uint8_t lead_[8];               /* nothing: the members' offsets keep their original alignment */
    uint8_t gSelLevelCount;         /* 0x00691d08 */
    unsigned char pad_00691d09[79]; /* 0x00691d09 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gSelLevelCountGroup, member) == (addr) - 0x00691d00, #member)
AT(gSelLevelCount, 0x00691d08);
AT(pad_00691d09, 0x00691d09);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gSelLevelCountGroup, pad_00691d09) +
                       sizeof(((struct BmxGroup_gSelLevelCountGroup *)0)->pad_00691d09) ==
                   0x58,
               "gSelLevelCountGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00691d60..0x00691d80.
 */
struct BmxGroup_gSelUnknown60Group {
    uint16_t gSelUnknown60;         /* 0x00691d60 */
    unsigned char pad_00691d62[30]; /* 0x00691d62 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gSelUnknown60Group, member) == (addr) - 0x00691d60, #member)
AT(gSelUnknown60, 0x00691d60);
AT(pad_00691d62, 0x00691d62);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gSelUnknown60Group, pad_00691d62) +
                       sizeof(((struct BmxGroup_gSelUnknown60Group *)0)->pad_00691d62) ==
                   0x20,
               "gSelUnknown60Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00691dd4..0x00691df8.
 */
struct BmxGroup_gSoundLevel0Group {
    uint8_t lead_[4];               /* nothing: the members' offsets keep their original alignment */
    int gSoundLevel0;               /* 0x00691dd4 */
    unsigned char pad_00691dd8[32]; /* 0x00691dd8 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gSoundLevel0Group, member) == (addr) - 0x00691dd0, #member)
AT(gSoundLevel0, 0x00691dd4);
AT(pad_00691dd8, 0x00691dd8);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gSoundLevel0Group, pad_00691dd8) +
                       sizeof(((struct BmxGroup_gSoundLevel0Group *)0)->pad_00691dd8) ==
                   0x28,
               "gSoundLevel0Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00691e20..0x00691e3c.
 */
struct BmxGroup_gSoundSelGroup {
    int gSoundSel;                  /* 0x00691e20 */
    unsigned char pad_00691e24[24]; /* 0x00691e24 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gSoundSelGroup, member) == (addr) - 0x00691e20, #member)
AT(gSoundSel, 0x00691e20);
AT(pad_00691e24, 0x00691e24);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gSoundSelGroup, pad_00691e24) +
                       sizeof(((struct BmxGroup_gSoundSelGroup *)0)->pad_00691e24) ==
                   0x1c,
               "gSoundSelGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00691e64..0x00691e80.
 */
struct BmxGroup_gSoundLevel1Group {
    uint8_t lead_[4];               /* nothing: the members' offsets keep their original alignment */
    int gSoundLevel1;               /* 0x00691e64 */
    unsigned char pad_00691e68[24]; /* 0x00691e68 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gSoundLevel1Group, member) == (addr) - 0x00691e60, #member)
AT(gSoundLevel1, 0x00691e64);
AT(pad_00691e68, 0x00691e68);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gSoundLevel1Group, pad_00691e68) +
                       sizeof(((struct BmxGroup_gSoundLevel1Group *)0)->pad_00691e68) ==
                   0x20,
               "gSoundLevel1Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00691ec0..0x00691ed4.
 */
struct BmxGroup_gMcChoiceGroup {
    uint16_t gMcChoice;             /* 0x00691ec0 */
    unsigned char pad_00691ec2[18]; /* 0x00691ec2 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gMcChoiceGroup, member) == (addr) - 0x00691ec0, #member)
AT(gMcChoice, 0x00691ec0);
AT(pad_00691ec2, 0x00691ec2);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gMcChoiceGroup, pad_00691ec2) +
                       sizeof(((struct BmxGroup_gMcChoiceGroup *)0)->pad_00691ec2) ==
                   0x14,
               "gMcChoiceGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00691f40..0x00693e88.
 */
struct BmxGroup_gPlayerRecsGroup {
    OvlPlayerRecs gPlayerRecs;       /* 0x00691f40, also gPhysRiders */
    unsigned char pad_00693e20[104]; /* 0x00693e20 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gPlayerRecsGroup, member) ==                                     \
                       BMX_DATA_OFF((addr) - 0x00691f40, off64),                                             \
                   #member)
AT(gPlayerRecs, 0x00691f40, 0x0);
AT(pad_00693e20, 0x00693e20, 0x1fd0);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gPlayerRecsGroup, pad_00693e20) +
                       sizeof(((struct BmxGroup_gPlayerRecsGroup *)0)->pad_00693e20) ==
                   BMX_DATA_OFF(0x1f48, 0x2038),
               "gPlayerRecsGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00694460..0x00694fa4.
 */
struct BmxGroup_gPropRecordsGroup {
    PropState gPropState; /* 0x00694460, also gPropRecords, gReplayTable; gPropNodeStates at 0x00694ca4 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gPropRecordsGroup, member) == (addr) - 0x00694460, #member)
AT(gPropState, 0x00694460);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gPropRecordsGroup, gPropState) +
                       sizeof(((struct BmxGroup_gPropRecordsGroup *)0)->gPropState) ==
                   BMX_DATA_OFF(0xb44, 0xe88),
               "gPropRecordsGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x006951e8..0x006951ff.
 */
struct BmxGroup_gPhysDebugTextureGroup {
    uint8_t lead_[8];               /* nothing: the members' offsets keep their original alignment */
    uint16_t gPhysDebugTexture;     /* 0x006951e8, also gOvlResB */
    unsigned char pad_006951ea[21]; /* 0x006951ea */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gPhysDebugTextureGroup, member) == (addr) - 0x006951e0, #member)
AT(gPhysDebugTexture, 0x006951e8);
AT(pad_006951ea, 0x006951ea);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gPhysDebugTextureGroup, pad_006951ea) +
                       sizeof(((struct BmxGroup_gPhysDebugTextureGroup *)0)->pad_006951ea) ==
                   0x1f,
               "gPhysDebugTextureGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00695200..0x00695260.
 */
struct BmxGroup_gGameJudgeScoresGroup {
    GameJudgeScores gGameJudgeScores;  /* 0x00695200, also gCareer */
    OvlCareerUnlocked gCareerUnlocked; /* 0x0069521e, also gGameJudgeRank */
    unsigned char pad_00695221[63];    /* 0x00695221 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gGameJudgeScoresGroup, member) == (addr) - 0x00695200, #member)
AT(gGameJudgeScores, 0x00695200);
AT(gCareerUnlocked, 0x0069521e);
AT(pad_00695221, 0x00695221);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gGameJudgeScoresGroup, pad_00695221) +
                       sizeof(((struct BmxGroup_gGameJudgeScoresGroup *)0)->pad_00695221) ==
                   0x60,
               "gGameJudgeScoresGroup");

/* SAVE.DAT's profiles: memcard.c copies them as one range, 0x006952a0..0x006955e0. */
struct BmxGroup_bmxSaveProfiles {
    OvlProfiles gProfiles;         /* 0x006952a0, also gNgKeyMapSel, gPlayers */
    unsigned char pad_00695338[4]; /* 0x00695338 */
    unsigned char d_0069533c[676]; /* 0x0069533c */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_bmxSaveProfiles, member) == (addr) - 0x006952a0, #member)
AT(gProfiles, 0x006952a0);
AT(pad_00695338, 0x00695338);
AT(d_0069533c, 0x0069533c);
#undef AT
_Static_assert(offsetof(struct BmxGroup_bmxSaveProfiles, d_0069533c) +
                       sizeof(((struct BmxGroup_bmxSaveProfiles *)0)->d_0069533c) ==
                   0x340,
               "bmxSaveProfiles");

/* SAVE.DAT's options and riders: memcard.c copies them as one range, 0x006955e0..0x0069621c. */
struct BmxGroup_bmxSaveOptions {
    uint8_t gOptionFlags;          /* 0x006955e0, also gGameUnlockBits, gOptionsAvail, gSaveOptions */
    unsigned char pad_006955e1[1]; /* 0x006955e1 */
    uint8_t gPhysSyncFlags;        /* 0x006955e2, also gActionOptions, gComdataOptions, gOptionsValue */
    unsigned char pad_006955e3[1]; /* 0x006955e3 */
    OvlVolumes gSoundVolumes;      /* 0x006955e4, also gRiderCount, gSoundVolume0, gSoundVolume1 */
    OvlRiderData
        gRiderData; /* 0x006955e8, also gGameRiders, gRiders, gUnlockAmish, gUnlockDave, gUnlockRyan */
    uint8_t gActionLanguage;       /* 0x00696218, also gLanguageCurrent */
    unsigned char pad_00696219[3]; /* 0x00696219 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_bmxSaveOptions, member) == (addr) - 0x006955e0, #member)
AT(gOptionFlags, 0x006955e0);
AT(pad_006955e1, 0x006955e1);
AT(gPhysSyncFlags, 0x006955e2);
AT(pad_006955e3, 0x006955e3);
AT(gSoundVolumes, 0x006955e4);
AT(gRiderData, 0x006955e8);
AT(gActionLanguage, 0x00696218);
AT(pad_00696219, 0x00696219);
#undef AT
_Static_assert(offsetof(struct BmxGroup_bmxSaveOptions, pad_00696219) +
                       sizeof(((struct BmxGroup_bmxSaveOptions *)0)->pad_00696219) ==
                   0xc3c,
               "bmxSaveOptions");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x006962fc..0x006966dc.
 */
struct BmxGroup_gPhysMotionsGroup {
    uint8_t lead_[12];              /* nothing: the members' offsets keep their original alignment */
    PhysMotionTable gPhysMotions;   /* 0x006962fc */
    ActionMotions gActionMotions;   /* 0x00696300 */
    unsigned char pad_006966b8[36]; /* 0x006966b8 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gPhysMotionsGroup, member) ==                                    \
                       BMX_DATA_OFF((addr) - 0x006962f0, off64),                                             \
                   #member)
AT(gPhysMotions, 0x006962fc, 0x10);
AT(gActionMotions, 0x00696300, 0x18);
AT(pad_006966b8, 0x006966b8, 0x788);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gPhysMotionsGroup, pad_006966b8) +
                       sizeof(((struct BmxGroup_gPhysMotionsGroup *)0)->pad_006966b8) ==
                   BMX_DATA_OFF(0x3ec, 0x7ac),
               "gPhysMotionsGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00696720..0x00696740.
 */
struct BmxGroup_rwD3DAndClipGroup {
    uint32_t rwD3DAndClip;          /* 0x00696720 */
    unsigned char pad_00696724[28]; /* 0x00696724 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_rwD3DAndClipGroup, member) == (addr) - 0x00696720, #member)
AT(rwD3DAndClip, 0x00696720);
AT(pad_00696724, 0x00696724);
#undef AT
_Static_assert(offsetof(struct BmxGroup_rwD3DAndClipGroup, pad_00696724) +
                       sizeof(((struct BmxGroup_rwD3DAndClipGroup *)0)->pad_00696724) ==
                   0x20,
               "rwD3DAndClipGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00696740..0x006968f0.
 */
struct BmxGroup_rwD3DStateGroup {
    RwD3DStateCache rwD3DState;     /* 0x00696740 */
    unsigned char pad_006968a0[80]; /* 0x006968a0 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_rwD3DStateGroup, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x00696740, off64),                                             \
                   #member)
AT(rwD3DState, 0x00696740, 0x0);
AT(pad_006968a0, 0x006968a0, 0x168);
#undef AT
_Static_assert(offsetof(struct BmxGroup_rwD3DStateGroup, pad_006968a0) +
                       sizeof(((struct BmxGroup_rwD3DStateGroup *)0)->pad_006968a0) ==
                   BMX_DATA_OFF(0x1b0, 0x1b8),
               "rwD3DStateGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00696a88..0x00696b00.
 */
struct BmxGroup_rwD3DIm3DWorkGroup {
    uint8_t lead_[8];                /* nothing: the members' offsets keep their original alignment */
    RwD3DIm3DWork *rwD3DIm3DWork;    /* 0x00696a88 */
    unsigned char pad_00696a8c[116]; /* 0x00696a8c */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_rwD3DIm3DWorkGroup, member) ==                                   \
                       BMX_DATA_OFF((addr) - 0x00696a80, off64),                                             \
                   #member)
AT(rwD3DIm3DWork, 0x00696a88, 0x8);
AT(pad_00696a8c, 0x00696a8c, 0x10);
#undef AT
_Static_assert(offsetof(struct BmxGroup_rwD3DIm3DWorkGroup, pad_00696a8c) +
                       sizeof(((struct BmxGroup_rwD3DIm3DWorkGroup *)0)->pad_00696a8c) ==
                   BMX_DATA_OFF(0x80, 0x84),
               "rwD3DIm3DWorkGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0069730c..0x00697320.
 */
struct BmxGroup_gDirectDrawGroup {
    uint8_t lead_[12];              /* nothing: the members' offsets keep their original alignment */
    void *gDirectDraw;              /* 0x0069730c */
    unsigned char pad_00697310[16]; /* 0x00697310 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gDirectDrawGroup, member) ==                                     \
                       BMX_DATA_OFF((addr) - 0x00697300, off64),                                             \
                   #member)
AT(gDirectDraw, 0x0069730c, 0x10);
AT(pad_00697310, 0x00697310, 0x18);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gDirectDrawGroup, pad_00697310) +
                       sizeof(((struct BmxGroup_gDirectDrawGroup *)0)->pad_00697310) ==
                   BMX_DATA_OFF(0x20, 0x28),
               "gDirectDrawGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0069737c..0x006973a0.
 */
struct BmxGroup_rpLodFreeListGroup {
    uint8_t lead_[12];              /* nothing: the members' offsets keep their original alignment */
    RwFreeList *rpLodFreeList;      /* 0x0069737c, also rpLodReserved */
    unsigned char pad_00697380[32]; /* 0x00697380 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_rpLodFreeListGroup, member) ==                                   \
                       BMX_DATA_OFF((addr) - 0x00697370, off64),                                             \
                   #member)
AT(rpLodFreeList, 0x0069737c, 0x10);
AT(pad_00697380, 0x00697380, 0x18);
#undef AT
_Static_assert(offsetof(struct BmxGroup_rpLodFreeListGroup, pad_00697380) +
                       sizeof(((struct BmxGroup_rpLodFreeListGroup *)0)->pad_00697380) ==
                   BMX_DATA_OFF(0x30, 0x38),
               "rpLodFreeListGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x006973a0..0x006976c0.
 */
struct BmxGroup_gMovieGroup {
    ZMovie gMovie;                   /* 0x006973a0 */
    unsigned char pad_0069762c[148]; /* 0x0069762c */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(                                                                                          \
        offsetof(struct BmxGroup_gMovieGroup, member) == BMX_DATA_OFF((addr) - 0x006973a0, off64), #member)
AT(gMovie, 0x006973a0, 0x0);
AT(pad_0069762c, 0x0069762c, 0x380);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gMovieGroup, pad_0069762c) +
                       sizeof(((struct BmxGroup_gMovieGroup *)0)->pad_0069762c) ==
                   BMX_DATA_OFF(0x320, 0x414),
               "gMovieGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00697728..0x0069772c.
 */
struct BmxGroup_gZrwFlagsGroup {
    uint8_t lead_[8];              /* nothing: the members' offsets keep their original alignment */
    uint8_t gZrwFlags;             /* 0x00697728 */
    unsigned char pad_00697729[3]; /* 0x00697729 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gZrwFlagsGroup, member) == (addr) - 0x00697720, #member)
AT(gZrwFlags, 0x00697728);
AT(pad_00697729, 0x00697729);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZrwFlagsGroup, pad_00697729) +
                       sizeof(((struct BmxGroup_gZrwFlagsGroup *)0)->pad_00697729) ==
                   0xc,
               "gZrwFlagsGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x00697744..0x00697770.
 */
struct BmxGroup_gZrwFramesGroup {
    uint8_t lead_[4];               /* nothing: the members' offsets keep their original alignment */
    int gZrwFrames;                 /* 0x00697744 */
    unsigned char pad_00697748[40]; /* 0x00697748 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_gZrwFramesGroup, member) == (addr) - 0x00697740, #member)
AT(gZrwFrames, 0x00697744);
AT(pad_00697748, 0x00697748);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gZrwFramesGroup, pad_00697748) +
                       sizeof(((struct BmxGroup_gZrwFramesGroup *)0)->pad_00697748) ==
                   0x30,
               "gZrwFramesGroup");

/* WinMain's globals: main.c's initGlobals clears them in one memset, 0x00697780..0x006977cc. */
struct BmxGroup_bmxWinMainGlobals {
    void *gZrwWindow;               /* 0x00697780, also gHwnd, gWindow */
    void *gZglInstance;             /* 0x00697784, also gHinstance */
    int gZrwFullscreen;             /* 0x00697788, also gFullscreen */
    int gWindow0;                   /* 0x0069778c */
    int gWindow1;                   /* 0x00697790 */
    const char *gAppTitle;          /* 0x00697794 */
    int gZrwWantW;                  /* 0x00697798, also gWidth */
    int gZrwWantH;                  /* 0x0069779c, also gHeight */
    int gZglWantW2;                 /* 0x006977a0, also gResX */
    int gZglWantH2;                 /* 0x006977a4, also gResY */
    int gZglWantDepth;              /* 0x006977a8, also gResZ */
    int gZrwQuit;                   /* 0x006977ac, also gQuit */
    int gZrwKeysLocked;             /* 0x006977b0 */
    unsigned char pad_006977b4[24]; /* 0x006977b4 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_bmxWinMainGlobals, member) ==                                    \
                       BMX_DATA_OFF((addr) - 0x00697780, off64),                                             \
                   #member)
AT(gZrwWindow, 0x00697780, 0x0);
AT(gZglInstance, 0x00697784, 0x8);
AT(gZrwFullscreen, 0x00697788, 0x10);
AT(gWindow0, 0x0069778c, 0x14);
AT(gWindow1, 0x00697790, 0x18);
AT(gAppTitle, 0x00697794, 0x20);
AT(gZrwWantW, 0x00697798, 0x28);
AT(gZrwWantH, 0x0069779c, 0x2c);
AT(gZglWantW2, 0x006977a0, 0x30);
AT(gZglWantH2, 0x006977a4, 0x34);
AT(gZglWantDepth, 0x006977a8, 0x38);
AT(gZrwQuit, 0x006977ac, 0x3c);
AT(gZrwKeysLocked, 0x006977b0, 0x40);
AT(pad_006977b4, 0x006977b4, 0x44);
#undef AT
_Static_assert(offsetof(struct BmxGroup_bmxWinMainGlobals, pad_006977b4) +
                       sizeof(((struct BmxGroup_bmxWinMainGlobals *)0)->pad_006977b4) ==
                   BMX_DATA_OFF(0x4c, 0x5c),
               "bmxWinMainGlobals");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x006977cc..0x006977f0.
 */
struct BmxGroup_gGdiSurfaceGroup {
    uint8_t lead_[12];              /* nothing: the members' offsets keep their original alignment */
    void *gGdiSurface;              /* 0x006977cc, also gDirectDrawSurface */
    unsigned char pad_006977d0[32]; /* 0x006977d0 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_gGdiSurfaceGroup, member) ==                                     \
                       BMX_DATA_OFF((addr) - 0x006977c0, off64),                                             \
                   #member)
AT(gGdiSurface, 0x006977cc, 0x10);
AT(pad_006977d0, 0x006977d0, 0x18);
#undef AT
_Static_assert(offsetof(struct BmxGroup_gGdiSurfaceGroup, pad_006977d0) +
                       sizeof(((struct BmxGroup_gGdiSurfaceGroup *)0)->pad_006977d0) ==
                   BMX_DATA_OFF(0x30, 0x38),
               "gGdiSurfaceGroup");

/* The original's .idata up to the end of its import slots, 0x00699498..0x0069987c. */
struct BmxGroup_bmxImports {
    void *ADVAPI32_RegQueryValueExA;          /* 0x00699498 */
    void *ADVAPI32_RegCloseKey;               /* 0x0069949c */
    void *ADVAPI32_RegOpenKeyExA;             /* 0x006994a0 */
    void *ADVAPI32_RegCreateKeyExA;           /* 0x006994a4 */
    void *end_006994a8;                       /* 0x006994a8 */
    void *end_006994ac;                       /* 0x006994ac */
    void *end_006994b0;                       /* 0x006994b0 */
    void *end_006994b4;                       /* 0x006994b4 */
    void *end_006994b8;                       /* 0x006994b8 */
    void *end_006994bc;                       /* 0x006994bc */
    void *end_006994c0;                       /* 0x006994c0 */
    void *end_006994c4;                       /* 0x006994c4 */
    void *end_006994c8;                       /* 0x006994c8 */
    void *end_006994cc;                       /* 0x006994cc */
    void *end_006994d0;                       /* 0x006994d0 */
    void *AVIFIL32_AVIStreamGetFrame;         /* 0x006994d4 */
    void *AVIFIL32_AVIStreamReadFormat;       /* 0x006994d8 */
    void *AVIFIL32_AVIFileExit;               /* 0x006994dc */
    void *AVIFIL32_AVIStreamGetFrameClose;    /* 0x006994e0 */
    void *AVIFIL32_AVISaveOptionsFree;        /* 0x006994e4 */
    void *AVIFIL32_AVIStreamRelease;          /* 0x006994e8 */
    void *AVIFIL32_AVIFileOpenA;              /* 0x006994ec */
    void *AVIFIL32_AVIStreamTimeToSample;     /* 0x006994f0 */
    void *AVIFIL32_AVIStreamLength;           /* 0x006994f4 */
    void *AVIFIL32_AVIStreamStart;            /* 0x006994f8 */
    void *AVIFIL32_AVIFileRelease;            /* 0x006994fc */
    void *AVIFIL32_AVIStreamSampleToTime;     /* 0x00699500 */
    void *AVIFIL32_AVIStreamGetFrameOpen;     /* 0x00699504 */
    void *AVIFIL32_AVIStreamInfoA;            /* 0x00699508 */
    void *AVIFIL32_AVIStreamRead;             /* 0x0069950c */
    void *AVIFIL32_AVIFileInit;               /* 0x00699510 */
    void *AVIFIL32_AVIStreamAddRef;           /* 0x00699514 */
    void *AVIFIL32_AVIFileGetStream;          /* 0x00699518 */
    void *end_0069951c;                       /* 0x0069951c */
    void *end_00699520;                       /* 0x00699520 */
    void *end_00699524;                       /* 0x00699524 */
    void *end_00699528;                       /* 0x00699528 */
    void *end_0069952c;                       /* 0x0069952c */
    void *end_00699530;                       /* 0x00699530 */
    void *end_00699534;                       /* 0x00699534 */
    void *end_00699538;                       /* 0x00699538 */
    void *end_0069953c;                       /* 0x0069953c */
    void *end_00699540;                       /* 0x00699540 */
    void *end_00699544;                       /* 0x00699544 */
    void *end_00699548;                       /* 0x00699548 */
    void *end_0069954c;                       /* 0x0069954c */
    void *end_00699550;                       /* 0x00699550 */
    void *DDRAW_DirectDrawCreate;             /* 0x00699554 */
    void *DDRAW_DirectDrawEnumerateA;         /* 0x00699558 */
    void *end_0069955c;                       /* 0x0069955c */
    void *end_00699560;                       /* 0x00699560 */
    void *end_00699564;                       /* 0x00699564 */
    void *end_00699568;                       /* 0x00699568 */
    void *end_0069956c;                       /* 0x0069956c */
    void *end_00699570;                       /* 0x00699570 */
    void *end_00699574;                       /* 0x00699574 */
    void *end_00699578;                       /* 0x00699578 */
    void *end_0069957c;                       /* 0x0069957c */
    void *end_00699580;                       /* 0x00699580 */
    void *end_00699584;                       /* 0x00699584 */
    void *DINPUT_DirectInputCreateEx;         /* 0x00699588 */
    void *end_0069958c;                       /* 0x0069958c */
    void *end_00699590;                       /* 0x00699590 */
    void *end_00699594;                       /* 0x00699594 */
    void *end_00699598;                       /* 0x00699598 */
    void *end_0069959c;                       /* 0x0069959c */
    void *end_006995a0;                       /* 0x006995a0 */
    void *end_006995a4;                       /* 0x006995a4 */
    void *end_006995a8;                       /* 0x006995a8 */
    void *end_006995ac;                       /* 0x006995ac */
    void *end_006995b0;                       /* 0x006995b0 */
    void *end_006995b4;                       /* 0x006995b4 */
    void *KERNEL32_ReadFile;                  /* 0x006995b8 */
    void *KERNEL32_FreeLibrary;               /* 0x006995bc */
    void *KERNEL32_CloseHandle;               /* 0x006995c0 */
    void *KERNEL32_LoadLibraryA;              /* 0x006995c4 */
    void *KERNEL32_SetFilePointer;            /* 0x006995c8 */
    void *KERNEL32_EnterCriticalSection;      /* 0x006995cc */
    void *KERNEL32_LeaveCriticalSection;      /* 0x006995d0 */
    void *KERNEL32_DeleteCriticalSection;     /* 0x006995d4 */
    void *KERNEL32_InitializeCriticalSection; /* 0x006995d8 */
    void *KERNEL32_CreateFileA;               /* 0x006995dc */
    void *KERNEL32_FreeEnvironmentStringsW;   /* 0x006995e0 */
    void *KERNEL32_LCMapStringA;              /* 0x006995e4 */
    void *KERNEL32_GetTimeZoneInformation;    /* 0x006995e8 */
    void *KERNEL32_LCMapStringW;              /* 0x006995ec */
    void *KERNEL32_OutputDebugStringA;        /* 0x006995f0 */
    void *KERNEL32_DebugBreak;                /* 0x006995f4 */
    void *KERNEL32_Sleep;                     /* 0x006995f8 */
    void *KERNEL32_GetTickCount;              /* 0x006995fc */
    void *KERNEL32_QueryPerformanceFrequency; /* 0x00699600 */
    void *KERNEL32_QueryPerformanceCounter;   /* 0x00699604 */
    void *KERNEL32_HeapFree;                  /* 0x00699608 */
    void *KERNEL32_GetModuleFileNameA;        /* 0x0069960c */
    void *KERNEL32_HeapAlloc;                 /* 0x00699610 */
    void *KERNEL32_GetModuleHandleA;          /* 0x00699614 */
    void *KERNEL32_GetStartupInfoA;           /* 0x00699618 */
    void *KERNEL32_GetCommandLineA;           /* 0x0069961c */
    void *KERNEL32_GetVersion;                /* 0x00699620 */
    void *KERNEL32_ExitProcess;               /* 0x00699624 */
    void *KERNEL32_TerminateProcess;          /* 0x00699628 */
    void *KERNEL32_GetCurrentProcess;         /* 0x0069962c */
    void *KERNEL32_GetEnvironmentVariableA;   /* 0x00699630 */
    void *KERNEL32_GetVersionExA;             /* 0x00699634 */
    void *KERNEL32_HeapDestroy;               /* 0x00699638 */
    void *KERNEL32_HeapCreate;                /* 0x0069963c */
    void *KERNEL32_VirtualFree;               /* 0x00699640 */
    void *KERNEL32_VirtualAlloc;              /* 0x00699644 */
    void *KERNEL32_HeapReAlloc;               /* 0x00699648 */
    void *KERNEL32_IsBadWritePtr;             /* 0x0069964c */
    void *KERNEL32_GetLastError;              /* 0x00699650 */
    void *KERNEL32_SetConsoleCtrlHandler;     /* 0x00699654 */
    void *KERNEL32_UnhandledExceptionFilter;  /* 0x00699658 */
    void *KERNEL32_FreeEnvironmentStringsA;   /* 0x0069965c */
    void *KERNEL32_GetProcAddress;            /* 0x00699660 */
    void *KERNEL32_WideCharToMultiByte;       /* 0x00699664 */
    void *KERNEL32_GetEnvironmentStrings;     /* 0x00699668 */
    void *KERNEL32_GetEnvironmentStringsW;    /* 0x0069966c */
    void *KERNEL32_SetHandleCount;            /* 0x00699670 */
    void *KERNEL32_GetStdHandle;              /* 0x00699674 */
    void *KERNEL32_GetFileType;               /* 0x00699678 */
    void *KERNEL32_RtlUnwind;                 /* 0x0069967c */
    void *KERNEL32_WriteFile;                 /* 0x00699680 */
    void *KERNEL32_RaiseException;            /* 0x00699684 */
    void *KERNEL32_MultiByteToWideChar;       /* 0x00699688 */
    void *KERNEL32_GetSystemTime;             /* 0x0069968c */
    void *KERNEL32_GetLocalTime;              /* 0x00699690 */
    void *KERNEL32_FlushFileBuffers;          /* 0x00699694 */
    void *KERNEL32_GetCPInfo;                 /* 0x00699698 */
    void *KERNEL32_GetACP;                    /* 0x0069969c */
    void *KERNEL32_GetOEMCP;                  /* 0x006996a0 */
    void *KERNEL32_GetStringTypeA;            /* 0x006996a4 */
    void *KERNEL32_GetStringTypeW;            /* 0x006996a8 */
    void *KERNEL32_SetStdHandle;              /* 0x006996ac */
    void *KERNEL32_SetEndOfFile;              /* 0x006996b0 */
    void *KERNEL32_CompareStringA;            /* 0x006996b4 */
    void *KERNEL32_CompareStringW;            /* 0x006996b8 */
    void *KERNEL32_SetEnvironmentVariableA;   /* 0x006996bc */
    void *end_006996c0;                       /* 0x006996c0 */
    void *end_006996c4;                       /* 0x006996c4 */
    void *end_006996c8;                       /* 0x006996c8 */
    void *end_006996cc;                       /* 0x006996cc */
    void *end_006996d0;                       /* 0x006996d0 */
    void *end_006996d4;                       /* 0x006996d4 */
    void *end_006996d8;                       /* 0x006996d8 */
    void *end_006996dc;                       /* 0x006996dc */
    void *end_006996e0;                       /* 0x006996e0 */
    void *end_006996e4;                       /* 0x006996e4 */
    void *end_006996e8;                       /* 0x006996e8 */
    void *end_006996ec;                       /* 0x006996ec */
    void *end_006996f0;                       /* 0x006996f0 */
    void *end_006996f4;                       /* 0x006996f4 */
    void *end_006996f8;                       /* 0x006996f8 */
    void *end_006996fc;                       /* 0x006996fc */
    void *end_00699700;                       /* 0x00699700 */
    void *end_00699704;                       /* 0x00699704 */
    void *end_00699708;                       /* 0x00699708 */
    void *end_0069970c;                       /* 0x0069970c */
    void *end_00699710;                       /* 0x00699710 */
    void *end_00699714;                       /* 0x00699714 */
    void *end_00699718;                       /* 0x00699718 */
    void *end_0069971c;                       /* 0x0069971c */
    void *MSVFW32_DrawDibDraw;                /* 0x00699720 */
    void *MSVFW32_DrawDibOpen;                /* 0x00699724 */
    void *MSVFW32_DrawDibClose;               /* 0x00699728 */
    void *end_0069972c;                       /* 0x0069972c */
    void *end_00699730;                       /* 0x00699730 */
    void *end_00699734;                       /* 0x00699734 */
    void *end_00699738;                       /* 0x00699738 */
    void *end_0069973c;                       /* 0x0069973c */
    void *end_00699740;                       /* 0x00699740 */
    void *end_00699744;                       /* 0x00699744 */
    void *end_00699748;                       /* 0x00699748 */
    void *end_0069974c;                       /* 0x0069974c */
    void *end_00699750;                       /* 0x00699750 */
    void *end_00699754;                       /* 0x00699754 */
    void *USER32_DispatchMessageA;            /* 0x00699758 */
    void *USER32_EndDialog;                   /* 0x0069975c */
    void *USER32_GetDlgItem;                  /* 0x00699760 */
    void *USER32_SendMessageA;                /* 0x00699764 */
    void *USER32_TranslateMessage;            /* 0x00699768 */
    void *USER32_ShowWindow;                  /* 0x0069976c */
    void *USER32_SetWindowPos;                /* 0x00699770 */
    void *USER32_PostQuitMessage;             /* 0x00699774 */
    void *USER32_DialogBoxParamA;             /* 0x00699778 */
    void *USER32_SetFocus;                    /* 0x0069977c */
    void *USER32_AdjustWindowRect;            /* 0x00699780 */
    void *USER32_RegisterClassA;              /* 0x00699784 */
    void *USER32_LoadCursorA;                 /* 0x00699788 */
    void *USER32_DefWindowProcA;              /* 0x0069978c */
    void *USER32_ClipCursor;                  /* 0x00699790 */
    void *USER32_GetWindowLongA;              /* 0x00699794 */
    void *USER32_GetClientRect;               /* 0x00699798 */
    void *USER32_ReleaseDC;                   /* 0x0069979c */
    void *USER32_GetDC;                       /* 0x006997a0 */
    void *USER32_CreateWindowExA;             /* 0x006997a4 */
    void *USER32_ShowCursor;                  /* 0x006997a8 */
    void *USER32_SetWindowLongA;              /* 0x006997ac */
    void *USER32_DestroyWindow;               /* 0x006997b0 */
    void *USER32_PeekMessageA;                /* 0x006997b4 */
    void *USER32_UpdateWindow;                /* 0x006997b8 */
    void *USER32_MessageBoxA;                 /* 0x006997bc */
    void *USER32_SetWindowTextA;              /* 0x006997c0 */
    void *USER32_ClientToScreen;              /* 0x006997c4 */
    void *end_006997c8;                       /* 0x006997c8 */
    void *end_006997cc;                       /* 0x006997cc */
    void *end_006997d0;                       /* 0x006997d0 */
    void *end_006997d4;                       /* 0x006997d4 */
    void *end_006997d8;                       /* 0x006997d8 */
    void *end_006997dc;                       /* 0x006997dc */
    void *end_006997e0;                       /* 0x006997e0 */
    void *end_006997e4;                       /* 0x006997e4 */
    void *end_006997e8;                       /* 0x006997e8 */
    void *end_006997ec;                       /* 0x006997ec */
    void *end_006997f0;                       /* 0x006997f0 */
    void *end_006997f4;                       /* 0x006997f4 */
    void *end_006997f8;                       /* 0x006997f8 */
    void *end_006997fc;                       /* 0x006997fc */
    void *end_00699800;                       /* 0x00699800 */
    void *end_00699804;                       /* 0x00699804 */
    void *WINMM_waveOutWrite;                 /* 0x00699808 */
    void *WINMM_waveOutOpen;                  /* 0x0069980c */
    void *WINMM_timeGetDevCaps;               /* 0x00699810 */
    void *WINMM_timeBeginPeriod;              /* 0x00699814 */
    void *WINMM_sndPlaySoundA;                /* 0x00699818 */
    void *WINMM_waveOutRestart;               /* 0x0069981c */
    void *WINMM_waveOutPause;                 /* 0x00699820 */
    void *WINMM_timeSetEvent;                 /* 0x00699824 */
    void *WINMM_waveOutReset;                 /* 0x00699828 */
    void *WINMM_waveOutUnprepareHeader;       /* 0x0069982c */
    void *WINMM_waveOutClose;                 /* 0x00699830 */
    void *WINMM_timeKillEvent;                /* 0x00699834 */
    void *WINMM_timeGetTime;                  /* 0x00699838 */
    void *WINMM_waveOutPrepareHeader;         /* 0x0069983c */
    void *WINMM_timeEndPeriod;                /* 0x00699840 */
    void *end_00699844;                       /* 0x00699844 */
    void *end_00699848;                       /* 0x00699848 */
    void *end_0069984c;                       /* 0x0069984c */
    void *end_00699850;                       /* 0x00699850 */
    void *end_00699854;                       /* 0x00699854 */
    void *end_00699858;                       /* 0x00699858 */
    void *end_0069985c;                       /* 0x0069985c */
    void *end_00699860;                       /* 0x00699860 */
    void *end_00699864;                       /* 0x00699864 */
    void *end_00699868;                       /* 0x00699868 */
    void *end_0069986c;                       /* 0x0069986c */
    void *end_00699870;                       /* 0x00699870 */
    void *end_00699874;                       /* 0x00699874 */
    void *end_00699878;                       /* 0x00699878 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_bmxImports, member) == BMX_DATA_OFF((addr) - 0x00699498, off64), \
                   #member)
AT(ADVAPI32_RegQueryValueExA, 0x00699498, 0x0);
AT(ADVAPI32_RegCloseKey, 0x0069949c, 0x8);
AT(ADVAPI32_RegOpenKeyExA, 0x006994a0, 0x10);
AT(ADVAPI32_RegCreateKeyExA, 0x006994a4, 0x18);
AT(end_006994a8, 0x006994a8, 0x20);
AT(end_006994ac, 0x006994ac, 0x28);
AT(end_006994b0, 0x006994b0, 0x30);
AT(end_006994b4, 0x006994b4, 0x38);
AT(end_006994b8, 0x006994b8, 0x40);
AT(end_006994bc, 0x006994bc, 0x48);
AT(end_006994c0, 0x006994c0, 0x50);
AT(end_006994c4, 0x006994c4, 0x58);
AT(end_006994c8, 0x006994c8, 0x60);
AT(end_006994cc, 0x006994cc, 0x68);
AT(end_006994d0, 0x006994d0, 0x70);
AT(AVIFIL32_AVIStreamGetFrame, 0x006994d4, 0x78);
AT(AVIFIL32_AVIStreamReadFormat, 0x006994d8, 0x80);
AT(AVIFIL32_AVIFileExit, 0x006994dc, 0x88);
AT(AVIFIL32_AVIStreamGetFrameClose, 0x006994e0, 0x90);
AT(AVIFIL32_AVISaveOptionsFree, 0x006994e4, 0x98);
AT(AVIFIL32_AVIStreamRelease, 0x006994e8, 0xa0);
AT(AVIFIL32_AVIFileOpenA, 0x006994ec, 0xa8);
AT(AVIFIL32_AVIStreamTimeToSample, 0x006994f0, 0xb0);
AT(AVIFIL32_AVIStreamLength, 0x006994f4, 0xb8);
AT(AVIFIL32_AVIStreamStart, 0x006994f8, 0xc0);
AT(AVIFIL32_AVIFileRelease, 0x006994fc, 0xc8);
AT(AVIFIL32_AVIStreamSampleToTime, 0x00699500, 0xd0);
AT(AVIFIL32_AVIStreamGetFrameOpen, 0x00699504, 0xd8);
AT(AVIFIL32_AVIStreamInfoA, 0x00699508, 0xe0);
AT(AVIFIL32_AVIStreamRead, 0x0069950c, 0xe8);
AT(AVIFIL32_AVIFileInit, 0x00699510, 0xf0);
AT(AVIFIL32_AVIStreamAddRef, 0x00699514, 0xf8);
AT(AVIFIL32_AVIFileGetStream, 0x00699518, 0x100);
AT(end_0069951c, 0x0069951c, 0x108);
AT(end_00699520, 0x00699520, 0x110);
AT(end_00699524, 0x00699524, 0x118);
AT(end_00699528, 0x00699528, 0x120);
AT(end_0069952c, 0x0069952c, 0x128);
AT(end_00699530, 0x00699530, 0x130);
AT(end_00699534, 0x00699534, 0x138);
AT(end_00699538, 0x00699538, 0x140);
AT(end_0069953c, 0x0069953c, 0x148);
AT(end_00699540, 0x00699540, 0x150);
AT(end_00699544, 0x00699544, 0x158);
AT(end_00699548, 0x00699548, 0x160);
AT(end_0069954c, 0x0069954c, 0x168);
AT(end_00699550, 0x00699550, 0x170);
AT(DDRAW_DirectDrawCreate, 0x00699554, 0x178);
AT(DDRAW_DirectDrawEnumerateA, 0x00699558, 0x180);
AT(end_0069955c, 0x0069955c, 0x188);
AT(end_00699560, 0x00699560, 0x190);
AT(end_00699564, 0x00699564, 0x198);
AT(end_00699568, 0x00699568, 0x1a0);
AT(end_0069956c, 0x0069956c, 0x1a8);
AT(end_00699570, 0x00699570, 0x1b0);
AT(end_00699574, 0x00699574, 0x1b8);
AT(end_00699578, 0x00699578, 0x1c0);
AT(end_0069957c, 0x0069957c, 0x1c8);
AT(end_00699580, 0x00699580, 0x1d0);
AT(end_00699584, 0x00699584, 0x1d8);
AT(DINPUT_DirectInputCreateEx, 0x00699588, 0x1e0);
AT(end_0069958c, 0x0069958c, 0x1e8);
AT(end_00699590, 0x00699590, 0x1f0);
AT(end_00699594, 0x00699594, 0x1f8);
AT(end_00699598, 0x00699598, 0x200);
AT(end_0069959c, 0x0069959c, 0x208);
AT(end_006995a0, 0x006995a0, 0x210);
AT(end_006995a4, 0x006995a4, 0x218);
AT(end_006995a8, 0x006995a8, 0x220);
AT(end_006995ac, 0x006995ac, 0x228);
AT(end_006995b0, 0x006995b0, 0x230);
AT(end_006995b4, 0x006995b4, 0x238);
AT(KERNEL32_ReadFile, 0x006995b8, 0x240);
AT(KERNEL32_FreeLibrary, 0x006995bc, 0x248);
AT(KERNEL32_CloseHandle, 0x006995c0, 0x250);
AT(KERNEL32_LoadLibraryA, 0x006995c4, 0x258);
AT(KERNEL32_SetFilePointer, 0x006995c8, 0x260);
AT(KERNEL32_EnterCriticalSection, 0x006995cc, 0x268);
AT(KERNEL32_LeaveCriticalSection, 0x006995d0, 0x270);
AT(KERNEL32_DeleteCriticalSection, 0x006995d4, 0x278);
AT(KERNEL32_InitializeCriticalSection, 0x006995d8, 0x280);
AT(KERNEL32_CreateFileA, 0x006995dc, 0x288);
AT(KERNEL32_FreeEnvironmentStringsW, 0x006995e0, 0x290);
AT(KERNEL32_LCMapStringA, 0x006995e4, 0x298);
AT(KERNEL32_GetTimeZoneInformation, 0x006995e8, 0x2a0);
AT(KERNEL32_LCMapStringW, 0x006995ec, 0x2a8);
AT(KERNEL32_OutputDebugStringA, 0x006995f0, 0x2b0);
AT(KERNEL32_DebugBreak, 0x006995f4, 0x2b8);
AT(KERNEL32_Sleep, 0x006995f8, 0x2c0);
AT(KERNEL32_GetTickCount, 0x006995fc, 0x2c8);
AT(KERNEL32_QueryPerformanceFrequency, 0x00699600, 0x2d0);
AT(KERNEL32_QueryPerformanceCounter, 0x00699604, 0x2d8);
AT(KERNEL32_HeapFree, 0x00699608, 0x2e0);
AT(KERNEL32_GetModuleFileNameA, 0x0069960c, 0x2e8);
AT(KERNEL32_HeapAlloc, 0x00699610, 0x2f0);
AT(KERNEL32_GetModuleHandleA, 0x00699614, 0x2f8);
AT(KERNEL32_GetStartupInfoA, 0x00699618, 0x300);
AT(KERNEL32_GetCommandLineA, 0x0069961c, 0x308);
AT(KERNEL32_GetVersion, 0x00699620, 0x310);
AT(KERNEL32_ExitProcess, 0x00699624, 0x318);
AT(KERNEL32_TerminateProcess, 0x00699628, 0x320);
AT(KERNEL32_GetCurrentProcess, 0x0069962c, 0x328);
AT(KERNEL32_GetEnvironmentVariableA, 0x00699630, 0x330);
AT(KERNEL32_GetVersionExA, 0x00699634, 0x338);
AT(KERNEL32_HeapDestroy, 0x00699638, 0x340);
AT(KERNEL32_HeapCreate, 0x0069963c, 0x348);
AT(KERNEL32_VirtualFree, 0x00699640, 0x350);
AT(KERNEL32_VirtualAlloc, 0x00699644, 0x358);
AT(KERNEL32_HeapReAlloc, 0x00699648, 0x360);
AT(KERNEL32_IsBadWritePtr, 0x0069964c, 0x368);
AT(KERNEL32_GetLastError, 0x00699650, 0x370);
AT(KERNEL32_SetConsoleCtrlHandler, 0x00699654, 0x378);
AT(KERNEL32_UnhandledExceptionFilter, 0x00699658, 0x380);
AT(KERNEL32_FreeEnvironmentStringsA, 0x0069965c, 0x388);
AT(KERNEL32_GetProcAddress, 0x00699660, 0x390);
AT(KERNEL32_WideCharToMultiByte, 0x00699664, 0x398);
AT(KERNEL32_GetEnvironmentStrings, 0x00699668, 0x3a0);
AT(KERNEL32_GetEnvironmentStringsW, 0x0069966c, 0x3a8);
AT(KERNEL32_SetHandleCount, 0x00699670, 0x3b0);
AT(KERNEL32_GetStdHandle, 0x00699674, 0x3b8);
AT(KERNEL32_GetFileType, 0x00699678, 0x3c0);
AT(KERNEL32_RtlUnwind, 0x0069967c, 0x3c8);
AT(KERNEL32_WriteFile, 0x00699680, 0x3d0);
AT(KERNEL32_RaiseException, 0x00699684, 0x3d8);
AT(KERNEL32_MultiByteToWideChar, 0x00699688, 0x3e0);
AT(KERNEL32_GetSystemTime, 0x0069968c, 0x3e8);
AT(KERNEL32_GetLocalTime, 0x00699690, 0x3f0);
AT(KERNEL32_FlushFileBuffers, 0x00699694, 0x3f8);
AT(KERNEL32_GetCPInfo, 0x00699698, 0x400);
AT(KERNEL32_GetACP, 0x0069969c, 0x408);
AT(KERNEL32_GetOEMCP, 0x006996a0, 0x410);
AT(KERNEL32_GetStringTypeA, 0x006996a4, 0x418);
AT(KERNEL32_GetStringTypeW, 0x006996a8, 0x420);
AT(KERNEL32_SetStdHandle, 0x006996ac, 0x428);
AT(KERNEL32_SetEndOfFile, 0x006996b0, 0x430);
AT(KERNEL32_CompareStringA, 0x006996b4, 0x438);
AT(KERNEL32_CompareStringW, 0x006996b8, 0x440);
AT(KERNEL32_SetEnvironmentVariableA, 0x006996bc, 0x448);
AT(end_006996c0, 0x006996c0, 0x450);
AT(end_006996c4, 0x006996c4, 0x458);
AT(end_006996c8, 0x006996c8, 0x460);
AT(end_006996cc, 0x006996cc, 0x468);
AT(end_006996d0, 0x006996d0, 0x470);
AT(end_006996d4, 0x006996d4, 0x478);
AT(end_006996d8, 0x006996d8, 0x480);
AT(end_006996dc, 0x006996dc, 0x488);
AT(end_006996e0, 0x006996e0, 0x490);
AT(end_006996e4, 0x006996e4, 0x498);
AT(end_006996e8, 0x006996e8, 0x4a0);
AT(end_006996ec, 0x006996ec, 0x4a8);
AT(end_006996f0, 0x006996f0, 0x4b0);
AT(end_006996f4, 0x006996f4, 0x4b8);
AT(end_006996f8, 0x006996f8, 0x4c0);
AT(end_006996fc, 0x006996fc, 0x4c8);
AT(end_00699700, 0x00699700, 0x4d0);
AT(end_00699704, 0x00699704, 0x4d8);
AT(end_00699708, 0x00699708, 0x4e0);
AT(end_0069970c, 0x0069970c, 0x4e8);
AT(end_00699710, 0x00699710, 0x4f0);
AT(end_00699714, 0x00699714, 0x4f8);
AT(end_00699718, 0x00699718, 0x500);
AT(end_0069971c, 0x0069971c, 0x508);
AT(MSVFW32_DrawDibDraw, 0x00699720, 0x510);
AT(MSVFW32_DrawDibOpen, 0x00699724, 0x518);
AT(MSVFW32_DrawDibClose, 0x00699728, 0x520);
AT(end_0069972c, 0x0069972c, 0x528);
AT(end_00699730, 0x00699730, 0x530);
AT(end_00699734, 0x00699734, 0x538);
AT(end_00699738, 0x00699738, 0x540);
AT(end_0069973c, 0x0069973c, 0x548);
AT(end_00699740, 0x00699740, 0x550);
AT(end_00699744, 0x00699744, 0x558);
AT(end_00699748, 0x00699748, 0x560);
AT(end_0069974c, 0x0069974c, 0x568);
AT(end_00699750, 0x00699750, 0x570);
AT(end_00699754, 0x00699754, 0x578);
AT(USER32_DispatchMessageA, 0x00699758, 0x580);
AT(USER32_EndDialog, 0x0069975c, 0x588);
AT(USER32_GetDlgItem, 0x00699760, 0x590);
AT(USER32_SendMessageA, 0x00699764, 0x598);
AT(USER32_TranslateMessage, 0x00699768, 0x5a0);
AT(USER32_ShowWindow, 0x0069976c, 0x5a8);
AT(USER32_SetWindowPos, 0x00699770, 0x5b0);
AT(USER32_PostQuitMessage, 0x00699774, 0x5b8);
AT(USER32_DialogBoxParamA, 0x00699778, 0x5c0);
AT(USER32_SetFocus, 0x0069977c, 0x5c8);
AT(USER32_AdjustWindowRect, 0x00699780, 0x5d0);
AT(USER32_RegisterClassA, 0x00699784, 0x5d8);
AT(USER32_LoadCursorA, 0x00699788, 0x5e0);
AT(USER32_DefWindowProcA, 0x0069978c, 0x5e8);
AT(USER32_ClipCursor, 0x00699790, 0x5f0);
AT(USER32_GetWindowLongA, 0x00699794, 0x5f8);
AT(USER32_GetClientRect, 0x00699798, 0x600);
AT(USER32_ReleaseDC, 0x0069979c, 0x608);
AT(USER32_GetDC, 0x006997a0, 0x610);
AT(USER32_CreateWindowExA, 0x006997a4, 0x618);
AT(USER32_ShowCursor, 0x006997a8, 0x620);
AT(USER32_SetWindowLongA, 0x006997ac, 0x628);
AT(USER32_DestroyWindow, 0x006997b0, 0x630);
AT(USER32_PeekMessageA, 0x006997b4, 0x638);
AT(USER32_UpdateWindow, 0x006997b8, 0x640);
AT(USER32_MessageBoxA, 0x006997bc, 0x648);
AT(USER32_SetWindowTextA, 0x006997c0, 0x650);
AT(USER32_ClientToScreen, 0x006997c4, 0x658);
AT(end_006997c8, 0x006997c8, 0x660);
AT(end_006997cc, 0x006997cc, 0x668);
AT(end_006997d0, 0x006997d0, 0x670);
AT(end_006997d4, 0x006997d4, 0x678);
AT(end_006997d8, 0x006997d8, 0x680);
AT(end_006997dc, 0x006997dc, 0x688);
AT(end_006997e0, 0x006997e0, 0x690);
AT(end_006997e4, 0x006997e4, 0x698);
AT(end_006997e8, 0x006997e8, 0x6a0);
AT(end_006997ec, 0x006997ec, 0x6a8);
AT(end_006997f0, 0x006997f0, 0x6b0);
AT(end_006997f4, 0x006997f4, 0x6b8);
AT(end_006997f8, 0x006997f8, 0x6c0);
AT(end_006997fc, 0x006997fc, 0x6c8);
AT(end_00699800, 0x00699800, 0x6d0);
AT(end_00699804, 0x00699804, 0x6d8);
AT(WINMM_waveOutWrite, 0x00699808, 0x6e0);
AT(WINMM_waveOutOpen, 0x0069980c, 0x6e8);
AT(WINMM_timeGetDevCaps, 0x00699810, 0x6f0);
AT(WINMM_timeBeginPeriod, 0x00699814, 0x6f8);
AT(WINMM_sndPlaySoundA, 0x00699818, 0x700);
AT(WINMM_waveOutRestart, 0x0069981c, 0x708);
AT(WINMM_waveOutPause, 0x00699820, 0x710);
AT(WINMM_timeSetEvent, 0x00699824, 0x718);
AT(WINMM_waveOutReset, 0x00699828, 0x720);
AT(WINMM_waveOutUnprepareHeader, 0x0069982c, 0x728);
AT(WINMM_waveOutClose, 0x00699830, 0x730);
AT(WINMM_timeKillEvent, 0x00699834, 0x738);
AT(WINMM_timeGetTime, 0x00699838, 0x740);
AT(WINMM_waveOutPrepareHeader, 0x0069983c, 0x748);
AT(WINMM_timeEndPeriod, 0x00699840, 0x750);
AT(end_00699844, 0x00699844, 0x758);
AT(end_00699848, 0x00699848, 0x760);
AT(end_0069984c, 0x0069984c, 0x768);
AT(end_00699850, 0x00699850, 0x770);
AT(end_00699854, 0x00699854, 0x778);
AT(end_00699858, 0x00699858, 0x780);
AT(end_0069985c, 0x0069985c, 0x788);
AT(end_00699860, 0x00699860, 0x790);
AT(end_00699864, 0x00699864, 0x798);
AT(end_00699868, 0x00699868, 0x7a0);
AT(end_0069986c, 0x0069986c, 0x7a8);
AT(end_00699870, 0x00699870, 0x7b0);
AT(end_00699874, 0x00699874, 0x7b8);
AT(end_00699878, 0x00699878, 0x7c0);
#undef AT
_Static_assert(offsetof(struct BmxGroup_bmxImports, end_00699878) +
                       sizeof(((struct BmxGroup_bmxImports *)0)->end_00699878) ==
                   BMX_DATA_OFF(0x3e4, 0x7c8),
               "bmxImports");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0069b504..0x0069b520.
 */
struct BmxGroup_rpWorldReadScale2Group {
    uint8_t lead_[4];        /* nothing: the members' offsets keep their original alignment */
    float rpWorldReadScale2; /* 0x0069b504 */
    float d_0069b508[6];     /* 0x0069b508 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_rpWorldReadScale2Group, member) == (addr) - 0x0069b500, #member)
AT(rpWorldReadScale2, 0x0069b504);
AT(d_0069b508, 0x0069b508);
#undef AT
_Static_assert(offsetof(struct BmxGroup_rpWorldReadScale2Group, d_0069b508) +
                       sizeof(((struct BmxGroup_rpWorldReadScale2Group *)0)->d_0069b508) ==
                   0x20,
               "rpWorldReadScale2Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0069b538..0x0069b544.
 */
struct BmxGroup_rwD3DK0x0069b538Group {
    uint8_t lead_[8];       /* nothing: the members' offsets keep their original alignment */
    float rwD3DK0x0069b538; /* 0x0069b538 */
    float d_0069b53c[2];    /* 0x0069b53c */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_rwD3DK0x0069b538Group, member) == (addr) - 0x0069b530, #member)
AT(rwD3DK0x0069b538, 0x0069b538);
AT(d_0069b53c, 0x0069b53c);
#undef AT
_Static_assert(offsetof(struct BmxGroup_rwD3DK0x0069b538Group, d_0069b53c) +
                       sizeof(((struct BmxGroup_rwD3DK0x0069b538Group *)0)->d_0069b53c) ==
                   0x14,
               "rwD3DK0x0069b538Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0069b574..0x0069b57c.
 */
struct BmxGroup_rwD3DK0x0069b574Group {
    uint8_t lead_[4];       /* nothing: the members' offsets keep their original alignment */
    float rwD3DK0x0069b574; /* 0x0069b574 */
    float d_0069b578;       /* 0x0069b578 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_rwD3DK0x0069b574Group, member) == (addr) - 0x0069b570, #member)
AT(rwD3DK0x0069b574, 0x0069b574);
AT(d_0069b578, 0x0069b578);
#undef AT
_Static_assert(offsetof(struct BmxGroup_rwD3DK0x0069b574Group, d_0069b578) +
                       sizeof(((struct BmxGroup_rwD3DK0x0069b574Group *)0)->d_0069b578) ==
                   0xc,
               "rwD3DK0x0069b574Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0069b590..0x0069b598.
 */
struct BmxGroup_rwD3DK0x0069b590Group {
    float rwD3DK0x0069b590; /* 0x0069b590 */
    float d_0069b594;       /* 0x0069b594 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_rwD3DK0x0069b590Group, member) == (addr) - 0x0069b590, #member)
AT(rwD3DK0x0069b590, 0x0069b590);
AT(d_0069b594, 0x0069b594);
#undef AT
_Static_assert(offsetof(struct BmxGroup_rwD3DK0x0069b590Group, d_0069b594) +
                       sizeof(((struct BmxGroup_rwD3DK0x0069b590Group *)0)->d_0069b594) ==
                   0x8,
               "rwD3DK0x0069b590Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0069b598..0x0069b5a0.
 */
struct BmxGroup_rwD3DK0x0069b598Group {
    uint8_t lead_[8];       /* nothing: the members' offsets keep their original alignment */
    float rwD3DK0x0069b598; /* 0x0069b598 */
    float d_0069b59c;       /* 0x0069b59c */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_rwD3DK0x0069b598Group, member) == (addr) - 0x0069b590, #member)
AT(rwD3DK0x0069b598, 0x0069b598);
AT(d_0069b59c, 0x0069b59c);
#undef AT
_Static_assert(offsetof(struct BmxGroup_rwD3DK0x0069b598Group, d_0069b59c) +
                       sizeof(((struct BmxGroup_rwD3DK0x0069b598Group *)0)->d_0069b59c) ==
                   0x10,
               "rwD3DK0x0069b598Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0069b5b4..0x0069b5bc.
 */
struct BmxGroup_rwD3DK0x0069b5b4Group {
    uint8_t lead_[4];       /* nothing: the members' offsets keep their original alignment */
    float rwD3DK0x0069b5b4; /* 0x0069b5b4 */
    float d_0069b5b8;       /* 0x0069b5b8 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_rwD3DK0x0069b5b4Group, member) == (addr) - 0x0069b5b0, #member)
AT(rwD3DK0x0069b5b4, 0x0069b5b4);
AT(d_0069b5b8, 0x0069b5b8);
#undef AT
_Static_assert(offsetof(struct BmxGroup_rwD3DK0x0069b5b4Group, d_0069b5b8) +
                       sizeof(((struct BmxGroup_rwD3DK0x0069b5b4Group *)0)->d_0069b5b8) ==
                   0xc,
               "rwD3DK0x0069b5b4Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0069b5bc..0x0069b5c4.
 */
struct BmxGroup_rwD3DK0x0069b5bcGroup {
    uint8_t lead_[12];      /* nothing: the members' offsets keep their original alignment */
    float rwD3DK0x0069b5bc; /* 0x0069b5bc */
    float d_0069b5c0;       /* 0x0069b5c0 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_rwD3DK0x0069b5bcGroup, member) == (addr) - 0x0069b5b0, #member)
AT(rwD3DK0x0069b5bc, 0x0069b5bc);
AT(d_0069b5c0, 0x0069b5c0);
#undef AT
_Static_assert(offsetof(struct BmxGroup_rwD3DK0x0069b5bcGroup, d_0069b5c0) +
                       sizeof(((struct BmxGroup_rwD3DK0x0069b5bcGroup *)0)->d_0069b5c0) ==
                   0x14,
               "rwD3DK0x0069b5bcGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0069b5cc..0x0069b5d4.
 */
struct BmxGroup_rwD3DK0x0069b5ccGroup {
    uint8_t lead_[12];      /* nothing: the members' offsets keep their original alignment */
    float rwD3DK0x0069b5cc; /* 0x0069b5cc */
    float d_0069b5d0;       /* 0x0069b5d0 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_rwD3DK0x0069b5ccGroup, member) == (addr) - 0x0069b5c0, #member)
AT(rwD3DK0x0069b5cc, 0x0069b5cc);
AT(d_0069b5d0, 0x0069b5d0);
#undef AT
_Static_assert(offsetof(struct BmxGroup_rwD3DK0x0069b5ccGroup, d_0069b5d0) +
                       sizeof(((struct BmxGroup_rwD3DK0x0069b5ccGroup *)0)->d_0069b5d0) ==
                   0x14,
               "rwD3DK0x0069b5ccGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0069b5e0..0x0069b5e8.
 */
struct BmxGroup_rwD3DK0x0069b5e0Group {
    float rwD3DK0x0069b5e0; /* 0x0069b5e0 */
    float d_0069b5e4;       /* 0x0069b5e4 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_rwD3DK0x0069b5e0Group, member) == (addr) - 0x0069b5e0, #member)
AT(rwD3DK0x0069b5e0, 0x0069b5e0);
AT(d_0069b5e4, 0x0069b5e4);
#undef AT
_Static_assert(offsetof(struct BmxGroup_rwD3DK0x0069b5e0Group, d_0069b5e4) +
                       sizeof(((struct BmxGroup_rwD3DK0x0069b5e0Group *)0)->d_0069b5e4) ==
                   0x8,
               "rwD3DK0x0069b5e0Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0069b5e8..0x0069b5f0.
 */
struct BmxGroup_rwD3DK0x0069b5e8Group {
    uint8_t lead_[8];       /* nothing: the members' offsets keep their original alignment */
    float rwD3DK0x0069b5e8; /* 0x0069b5e8 */
    float d_0069b5ec;       /* 0x0069b5ec */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_rwD3DK0x0069b5e8Group, member) == (addr) - 0x0069b5e0, #member)
AT(rwD3DK0x0069b5e8, 0x0069b5e8);
AT(d_0069b5ec, 0x0069b5ec);
#undef AT
_Static_assert(offsetof(struct BmxGroup_rwD3DK0x0069b5e8Group, d_0069b5ec) +
                       sizeof(((struct BmxGroup_rwD3DK0x0069b5e8Group *)0)->d_0069b5ec) ==
                   0x10,
               "rwD3DK0x0069b5e8Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0069b60c..0x0069b618.
 */
struct BmxGroup_rwD3DK0x0069b60cGroup {
    uint8_t lead_[12];      /* nothing: the members' offsets keep their original alignment */
    float rwD3DK0x0069b60c; /* 0x0069b60c */
    float d_0069b610[2];    /* 0x0069b610 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_rwD3DK0x0069b60cGroup, member) == (addr) - 0x0069b600, #member)
AT(rwD3DK0x0069b60c, 0x0069b60c);
AT(d_0069b610, 0x0069b610);
#undef AT
_Static_assert(offsetof(struct BmxGroup_rwD3DK0x0069b60cGroup, d_0069b610) +
                       sizeof(((struct BmxGroup_rwD3DK0x0069b60cGroup *)0)->d_0069b610) ==
                   0x18,
               "rwD3DK0x0069b60cGroup");

/* The type is local to rw/clump.c. */
typedef float BmxType_rwMatrixNormalErrorLimits[3];

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0069b958..0x0069b968.
 */
struct BmxGroup_rwInvSqrtScaleEvenGroup {
    uint8_t lead_[8];          /* nothing: the members' offsets keep their original alignment */
    double rwInvSqrtScaleEven; /* 0x0069b958 */
    float d_0069b960[2];       /* 0x0069b960 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_rwInvSqrtScaleEvenGroup, member) == (addr) - 0x0069b950, #member)
AT(rwInvSqrtScaleEven, 0x0069b958);
AT(d_0069b960, 0x0069b960);
#undef AT
_Static_assert(offsetof(struct BmxGroup_rwInvSqrtScaleEvenGroup, d_0069b960) +
                       sizeof(((struct BmxGroup_rwInvSqrtScaleEvenGroup *)0)->d_0069b960) ==
                   0x18,
               "rwInvSqrtScaleEvenGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0069b9f4..0x0069ba20.
 */
struct BmxGroup_d_0069b9f4Group {
    uint8_t lead_[4];     /* nothing: the members' offsets keep their original alignment */
    uint32_t d_0069b9f4;  /* 0x0069b9f4 */
    void *p_0069b9f8[10]; /* 0x0069b9f8 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_d_0069b9f4Group, member) == (addr) - 0x0069b9f0, #member)
AT(d_0069b9f4, 0x0069b9f4);
AT(p_0069b9f8, 0x0069b9f8);
#undef AT
_Static_assert(offsetof(struct BmxGroup_d_0069b9f4Group, p_0069b9f8) +
                       sizeof(((struct BmxGroup_d_0069b9f4Group *)0)->p_0069b9f8) ==
                   BMX_DATA_OFF(0x30, 0x58),
               "d_0069b9f4Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0069baec..0x0069bb90.
 */
struct BmxGroup_p_0069baecGroup {
    uint8_t lead_[12];    /* nothing: the members' offsets keep their original alignment */
    void *p_0069baec[17]; /* 0x0069baec */
    float d_0069bb30[24]; /* 0x0069bb30 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_p_0069baecGroup, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x0069bae0, off64),                                             \
                   #member)
AT(p_0069baec, 0x0069baec, 0x10);
AT(d_0069bb30, 0x0069bb30, 0x98);
#undef AT
_Static_assert(offsetof(struct BmxGroup_p_0069baecGroup, d_0069bb30) +
                       sizeof(((struct BmxGroup_p_0069baecGroup *)0)->d_0069bb30) ==
                   BMX_DATA_OFF(0xb0, 0xf8),
               "p_0069baecGroup");

/* The type is local to rw/camera.c. */
typedef float BmxType_rwPerspCornerSteps[6];

/* The type is local to rw/camera.c. */
typedef float BmxType_rwViewFactors[10];

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0069bdfc..0x0069be54.
 */
struct BmxGroup_d_0069bdfcGroup {
    uint8_t lead_[12];           /* nothing: the members' offsets keep their original alignment */
    unsigned char d_0069bdfc[4]; /* 0x0069bdfc */
    void *p_0069be00[20];        /* 0x0069be00 */
    float d_0069be50;            /* 0x0069be50 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_d_0069bdfcGroup, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x0069bdf0, off64),                                             \
                   #member)
AT(d_0069bdfc, 0x0069bdfc, 0xc);
AT(p_0069be00, 0x0069be00, 0x10);
AT(d_0069be50, 0x0069be50, 0xb0);
#undef AT
_Static_assert(offsetof(struct BmxGroup_d_0069bdfcGroup, d_0069be50) +
                       sizeof(((struct BmxGroup_d_0069bdfcGroup *)0)->d_0069be50) ==
                   BMX_DATA_OFF(0x64, 0xb4),
               "d_0069bdfcGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0069bee0..0x0069bef8.
 */
struct BmxGroup_p_0069bee0Group {
    void *p_0069bee0[4]; /* 0x0069bee0 */
    float d_0069bef0[2]; /* 0x0069bef0 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_p_0069bee0Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x0069bee0, off64),                                             \
                   #member)
AT(p_0069bee0, 0x0069bee0, 0x0);
AT(d_0069bef0, 0x0069bef0, 0x20);
#undef AT
_Static_assert(offsetof(struct BmxGroup_p_0069bee0Group, d_0069bef0) +
                       sizeof(((struct BmxGroup_p_0069bee0Group *)0)->d_0069bef0) ==
                   BMX_DATA_OFF(0x18, 0x28),
               "p_0069bee0Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0069c5c0..0x0069c5d4.
 */
struct BmxGroup_rwD3DOneGroup {
    float rwD3DOne;      /* 0x0069c5c0 */
    float d_0069c5c4[4]; /* 0x0069c5c4 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_rwD3DOneGroup, member) == (addr) - 0x0069c5c0, #member)
AT(rwD3DOne, 0x0069c5c0);
AT(d_0069c5c4, 0x0069c5c4);
#undef AT
_Static_assert(offsetof(struct BmxGroup_rwD3DOneGroup, d_0069c5c4) +
                       sizeof(((struct BmxGroup_rwD3DOneGroup *)0)->d_0069c5c4) ==
                   0x14,
               "rwD3DOneGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0069c644..0x0069c660.
 */
struct BmxGroup_rpAnimSumStartBGroup {
    uint8_t lead_[4];               /* nothing: the members' offsets keep their original alignment */
    float rpAnimSumStartB;          /* 0x0069c644 */
    unsigned char pad_0069c648[24]; /* 0x0069c648 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_rpAnimSumStartBGroup, member) == (addr) - 0x0069c640, #member)
AT(rpAnimSumStartB, 0x0069c644);
AT(pad_0069c648, 0x0069c648);
#undef AT
_Static_assert(offsetof(struct BmxGroup_rpAnimSumStartBGroup, pad_0069c648) +
                       sizeof(((struct BmxGroup_rpAnimSumStartBGroup *)0)->pad_0069c648) ==
                   0x20,
               "rpAnimSumStartBGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0069c69c..0x0069c6b0.
 */
struct BmxGroup_rpAnimStartStepGroup {
    uint8_t lead_[12];              /* nothing: the members' offsets keep their original alignment */
    float rpAnimStartStep;          /* 0x0069c69c */
    unsigned char pad_0069c6a0[16]; /* 0x0069c6a0 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_rpAnimStartStepGroup, member) == (addr) - 0x0069c690, #member)
AT(rpAnimStartStep, 0x0069c69c);
AT(pad_0069c6a0, 0x0069c6a0);
#undef AT
_Static_assert(offsetof(struct BmxGroup_rpAnimStartStepGroup, pad_0069c6a0) +
                       sizeof(((struct BmxGroup_rpAnimStartStepGroup *)0)->pad_0069c6a0) ==
                   0x20,
               "rpAnimStartStepGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0069c6b0..0x0069c720.
 */
struct BmxGroup_rpAnimHeadNameGroup {
    char rpAnimHeadName;  /* 0x0069c6b0 */
    char sDummy[7];       /* 0x0069c6b1 */
    float d_0069c6b8[26]; /* 0x0069c6b8 */
};
#define AT(member, addr)                                                                                     \
    _Static_assert(offsetof(struct BmxGroup_rpAnimHeadNameGroup, member) == (addr) - 0x0069c6b0, #member)
AT(rpAnimHeadName, 0x0069c6b0);
AT(sDummy, 0x0069c6b1);
AT(d_0069c6b8, 0x0069c6b8);
#undef AT
_Static_assert(offsetof(struct BmxGroup_rpAnimHeadNameGroup, d_0069c6b8) +
                       sizeof(((struct BmxGroup_rpAnimHeadNameGroup *)0)->d_0069c6b8) ==
                   0x70,
               "rpAnimHeadNameGroup");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0069d120..0x0069d160.
 */
struct BmxGroup_p_0069d120Group {
    void *p_0069d120[14];          /* 0x0069d120 */
    unsigned char pad_0069d158[8]; /* 0x0069d158 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_p_0069d120Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x0069d120, off64),                                             \
                   #member)
AT(p_0069d120, 0x0069d120, 0x0);
AT(pad_0069d158, 0x0069d158, 0x70);
#undef AT
_Static_assert(offsetof(struct BmxGroup_p_0069d120Group, pad_0069d158) +
                       sizeof(((struct BmxGroup_p_0069d120Group *)0)->pad_0069d158) ==
                   BMX_DATA_OFF(0x40, 0x78),
               "p_0069d120Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0069dfb0..0x0069dfe0.
 */
struct BmxGroup_p_0069dfb0Group {
    void *p_0069dfb0[10];          /* 0x0069dfb0 */
    unsigned char pad_0069dfd8[8]; /* 0x0069dfd8 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_p_0069dfb0Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x0069dfb0, off64),                                             \
                   #member)
AT(p_0069dfb0, 0x0069dfb0, 0x0);
AT(pad_0069dfd8, 0x0069dfd8, 0x50);
#undef AT
_Static_assert(offsetof(struct BmxGroup_p_0069dfb0Group, pad_0069dfd8) +
                       sizeof(((struct BmxGroup_p_0069dfb0Group *)0)->pad_0069dfd8) ==
                   BMX_DATA_OFF(0x30, 0x58),
               "p_0069dfb0Group");

/* Globals the code uses as one piece (a view over them, or a table declared shorter), 0x0069e190..0x0069e1c0.
 */
struct BmxGroup_p_0069e190Group {
    void *p_0069e190[10];          /* 0x0069e190 */
    unsigned char pad_0069e1b8[8]; /* 0x0069e1b8 */
};
#define AT(member, addr, off64)                                                                              \
    _Static_assert(offsetof(struct BmxGroup_p_0069e190Group, member) ==                                      \
                       BMX_DATA_OFF((addr) - 0x0069e190, off64),                                             \
                   #member)
AT(p_0069e190, 0x0069e190, 0x0);
AT(pad_0069e1b8, 0x0069e1b8, 0x50);
#undef AT
_Static_assert(offsetof(struct BmxGroup_p_0069e190Group, pad_0069e1b8) +
                       sizeof(((struct BmxGroup_p_0069e190Group *)0)->pad_0069e1b8) ==
                   BMX_DATA_OFF(0x30, 0x58),
               "p_0069e190Group");

/* Every definition, by original address. */
extern const struct BmxGroup_bmxInputTables bmxInputTables;      /* 0x004afc60 */
extern const uint32_t d_0054d000[7];                             /* 0x0054d000 */
extern const uint32_t d_0054d01c;                                /* 0x0054d01c */
extern const float d_0054d020;                                   /* 0x0054d020 */
extern const float d_0054d024;                                   /* 0x0054d024 */
extern const float d_0054d028;                                   /* 0x0054d028 */
extern const float d_0054d02c;                                   /* 0x0054d02c */
extern const float d_0054d030;                                   /* 0x0054d030 */
extern const float d_0054d034;                                   /* 0x0054d034 */
extern const float d_0054d038;                                   /* 0x0054d038 */
extern const float d_0054d03c;                                   /* 0x0054d03c */
extern const float d_0054d040;                                   /* 0x0054d040 */
extern const float d_0054d044;                                   /* 0x0054d044 */
extern const float d_0054d048;                                   /* 0x0054d048 */
extern const float d_0054d04c;                                   /* 0x0054d04c */
extern const float d_0054d050;                                   /* 0x0054d050 */
extern const float d_0054d054;                                   /* 0x0054d054 */
extern const float d_0054d058;                                   /* 0x0054d058 */
extern const float gRateB;                                       /* 0x0054d05c */
extern const float d_0054d060;                                   /* 0x0054d060 */
extern const float d_0054d064;                                   /* 0x0054d064 */
extern const float d_0054d068;                                   /* 0x0054d068 */
extern const float d_0054d06c;                                   /* 0x0054d06c */
extern const float d_0054d070;                                   /* 0x0054d070 */
extern const float gScoreRound;                                  /* 0x0054d074 */
extern const float d_0054d078;                                   /* 0x0054d078 */
extern const float d_0054d07c;                                   /* 0x0054d07c */
extern const float d_0054d080[3];                                /* 0x0054d080 */
extern const float d_0054d08c;                                   /* 0x0054d08c */
extern const float d_0054d090;                                   /* 0x0054d090 */
extern const float d_0054d094;                                   /* 0x0054d094 */
extern const float d_0054d098;                                   /* 0x0054d098 */
extern const float d_0054d09c;                                   /* 0x0054d09c */
extern const float d_0054d0a0;                                   /* 0x0054d0a0 */
extern const float d_0054d0a4;                                   /* 0x0054d0a4 */
extern const float d_0054d0a8;                                   /* 0x0054d0a8 */
extern const uint32_t d_0054d0ac;                                /* 0x0054d0ac */
extern const float d_0054d0b0;                                   /* 0x0054d0b0 */
extern const uint32_t d_0054d0b4;                                /* 0x0054d0b4 */
extern const float d_0054d0b8;                                   /* 0x0054d0b8 */
extern const float d_0054d0bc;                                   /* 0x0054d0bc */
extern const uint32_t d_0054d0c0;                                /* 0x0054d0c0 */
extern const float d_0054d0c4;                                   /* 0x0054d0c4 */
extern const float d_0054d0c8;                                   /* 0x0054d0c8 */
extern const float d_0054d0cc;                                   /* 0x0054d0cc */
extern const float d_0054d0d0;                                   /* 0x0054d0d0 */
extern const float d_0054d0d4;                                   /* 0x0054d0d4 */
extern const float d_0054d0d8;                                   /* 0x0054d0d8 */
extern const float d_0054d0dc;                                   /* 0x0054d0dc */
extern const float d_0054d0e0;                                   /* 0x0054d0e0 */
extern const float d_0054d0e4;                                   /* 0x0054d0e4 */
extern const float d_0054d0e8;                                   /* 0x0054d0e8 */
extern const float gScoreToTime;                                 /* 0x0054d0ec */
extern const float d_0054d0f0;                                   /* 0x0054d0f0 */
extern const float d_0054d0f4;                                   /* 0x0054d0f4 */
extern const float d_0054d0f8;                                   /* 0x0054d0f8 */
extern const float d_0054d0fc;                                   /* 0x0054d0fc */
extern const float d_0054d100;                                   /* 0x0054d100 */
extern const float d_0054d104;                                   /* 0x0054d104 */
extern const float d_0054d108;                                   /* 0x0054d108 */
extern const float d_0054d10c;                                   /* 0x0054d10c */
extern const float d_0054d110;                                   /* 0x0054d110 */
extern const float d_0054d114;                                   /* 0x0054d114 */
extern const float d_0054d118;                                   /* 0x0054d118 */
extern const float d_0054d11c;                                   /* 0x0054d11c */
extern const uint32_t d_0054d120;                                /* 0x0054d120 */
extern const float d_0054d124;                                   /* 0x0054d124 */
extern const float d_0054d128;                                   /* 0x0054d128 */
extern const float d_0054d12c;                                   /* 0x0054d12c */
extern const float d_0054d130;                                   /* 0x0054d130 */
extern const float d_0054d134;                                   /* 0x0054d134 */
extern const float d_0054d138;                                   /* 0x0054d138 */
extern const float d_0054d13c;                                   /* 0x0054d13c */
extern const float d_0054d140;                                   /* 0x0054d140 */
extern const float d_0054d144;                                   /* 0x0054d144 */
extern const float d_0054d148;                                   /* 0x0054d148 */
extern const float d_0054d14c;                                   /* 0x0054d14c */
extern const float d_0054d150;                                   /* 0x0054d150 */
extern const float d_0054d154;                                   /* 0x0054d154 */
extern const uint32_t d_0054d158;                                /* 0x0054d158 */
extern const float d_0054d15c;                                   /* 0x0054d15c */
extern const float d_0054d160;                                   /* 0x0054d160 */
extern const float d_0054d164;                                   /* 0x0054d164 */
extern const float d_0054d168;                                   /* 0x0054d168 */
extern const float d_0054d16c;                                   /* 0x0054d16c */
extern const float d_0054d170;                                   /* 0x0054d170 */
extern const float d_0054d174;                                   /* 0x0054d174 */
extern const float d_0054d178;                                   /* 0x0054d178 */
extern const float d_0054d17c;                                   /* 0x0054d17c */
extern const float d_0054d180;                                   /* 0x0054d180 */
extern const float d_0054d184;                                   /* 0x0054d184 */
extern const float d_0054d188;                                   /* 0x0054d188 */
extern const float d_0054d18c;                                   /* 0x0054d18c */
extern const float d_0054d190;                                   /* 0x0054d190 */
extern const float d_0054d194;                                   /* 0x0054d194 */
extern const float d_0054d198;                                   /* 0x0054d198 */
extern const float d_0054d19c;                                   /* 0x0054d19c */
extern const float d_0054d1a0;                                   /* 0x0054d1a0 */
extern const float d_0054d1a4;                                   /* 0x0054d1a4 */
extern const float d_0054d1a8;                                   /* 0x0054d1a8 */
extern const float d_0054d1ac;                                   /* 0x0054d1ac */
extern const float d_0054d1b0;                                   /* 0x0054d1b0 */
extern const float d_0054d1b4;                                   /* 0x0054d1b4 */
extern const float d_0054d1b8;                                   /* 0x0054d1b8 */
extern const float d_0054d1bc;                                   /* 0x0054d1bc */
extern const uint32_t d_0054d1c0;                                /* 0x0054d1c0 */
extern const uint32_t d_0054d1c4;                                /* 0x0054d1c4 */
extern const float d_0054d1c8;                                   /* 0x0054d1c8 */
extern const float d_0054d1cc;                                   /* 0x0054d1cc */
extern const uint32_t d_0054d1d0;                                /* 0x0054d1d0 */
extern const uint32_t d_0054d1d4;                                /* 0x0054d1d4 */
extern const float d_0054d1d8;                                   /* 0x0054d1d8 */
extern const float d_0054d1dc;                                   /* 0x0054d1dc */
extern const float d_0054d1e0;                                   /* 0x0054d1e0 */
extern const float d_0054d1e4;                                   /* 0x0054d1e4 */
extern const float d_0054d1e8;                                   /* 0x0054d1e8 */
extern const float d_0054d1ec;                                   /* 0x0054d1ec */
extern const uint32_t d_0054d1f0;                                /* 0x0054d1f0 */
extern const float d_0054d1f4;                                   /* 0x0054d1f4 */
extern const float d_0054d1f8;                                   /* 0x0054d1f8 */
extern const float d_0054d1fc;                                   /* 0x0054d1fc */
extern const float d_0054d200;                                   /* 0x0054d200 */
extern const float d_0054d204;                                   /* 0x0054d204 */
extern const float d_0054d208[2];                                /* 0x0054d208 */
extern const unsigned char d_0054d210[12];                       /* 0x0054d210 */
extern const float d_0054d21c;                                   /* 0x0054d21c */
extern const float gStripScale1;                                 /* 0x0054d220 */
extern const SelIdTable gSelPicIds;                              /* 0x0054d224 */
extern const SelIdTable gSelTextIds;                             /* 0x0054d230 */
extern const SelAssetTable gRiderTextures;                       /* 0x0054d23c */
extern const SelAssetTable gRiderNames;                          /* 0x0054d254, also gRiderNameIds */
extern const StatWidthTable gStatWidths;                         /* 0x0054d26c */
extern const SelWideTable gLevelTextures;                        /* 0x0054d35c */
extern const SelWideTable gLevelNames;                           /* 0x0054d38c, also gHiscoreTitleIds */
extern const SelAsset60 gStyleTextures;                          /* 0x0054d3bc */
extern const SelAssetTable gStyleGroups;                         /* 0x0054d434 */
extern const StatLayerTable gStatLayers;                         /* 0x0054d44c */
extern const StatPosTable gStatPositions;                        /* 0x0054d80c */
extern const SelAsset60 gBikePalettes;                           /* 0x0054d82c */
extern const struct BmxGroup_gBikeStatsGroup gBikeStatsGroup;    /* 0x0054d8a4 */
extern const LevelIdTable gHiscorePictureIds;                    /* 0x0054db58 */
extern const uint32_t d_0054db94;                                /* 0x0054db94 */
extern const float d_0054db98;                                   /* 0x0054db98 */
extern const float d_0054db9c;                                   /* 0x0054db9c */
extern const float d_0054dba0;                                   /* 0x0054dba0 */
extern const float d_0054dba4;                                   /* 0x0054dba4 */
extern const float d_0054dba8;                                   /* 0x0054dba8 */
extern const float d_0054dbac;                                   /* 0x0054dbac */
extern const uint32_t d_0054dbb0;                                /* 0x0054dbb0 */
extern const float d_0054dbb4;                                   /* 0x0054dbb4 */
extern const float gRateC;                                       /* 0x0054dbb8 */
extern const uint32_t d_0054dbbc;                                /* 0x0054dbbc */
extern const float d_0054dbc0[2];                                /* 0x0054dbc0 */
extern const double d_0054dbc8[2];                               /* 0x0054dbc8 */
extern const double d_0054dbd8[2];                               /* 0x0054dbd8 */
extern const uint32_t d_0054dbe8[3];                             /* 0x0054dbe8 */
extern const uint32_t d_0054dbf4;                                /* 0x0054dbf4 */
extern const float d_0054dbf8;                                   /* 0x0054dbf8 */
extern const float d_0054dbfc;                                   /* 0x0054dbfc */
extern const float d_0054dc00;                                   /* 0x0054dc00 */
extern const float d_0054dc04;                                   /* 0x0054dc04 */
extern const float d_0054dc08;                                   /* 0x0054dc08 */
extern const float d_0054dc0c;                                   /* 0x0054dc0c */
extern const double d_0054dc10[2];                               /* 0x0054dc10 */
extern const uint32_t d_0054dc20[4];                             /* 0x0054dc20 */
extern const double d_0054dc30[2];                               /* 0x0054dc30 */
extern const float d_0054dc40[3];                                /* 0x0054dc40 */
extern const float d_0054dc4c;                                   /* 0x0054dc4c */
extern const uint32_t d_0054dc50[3];                             /* 0x0054dc50 */
extern const float d_0054dc5c;                                   /* 0x0054dc5c */
extern const float d_0054dc60[2];                                /* 0x0054dc60 */
extern const double d_0054dc68[2];                               /* 0x0054dc68 */
extern const double d_0054dc78[2];                               /* 0x0054dc78 */
extern const uint32_t d_0054dc88[3];                             /* 0x0054dc88 */
extern const float d_0054dc94;                                   /* 0x0054dc94 */
extern const float d_0054dc98;                                   /* 0x0054dc98 */
extern const float d_0054dc9c;                                   /* 0x0054dc9c */
extern const float d_0054dca0;                                   /* 0x0054dca0 */
extern const float d_0054dca4;                                   /* 0x0054dca4 */
extern const float d_0054dca8;                                   /* 0x0054dca8 */
extern const float d_0054dcac;                                   /* 0x0054dcac */
extern const float d_0054dcb0;                                   /* 0x0054dcb0 */
extern const uint32_t d_0054dcb4;                                /* 0x0054dcb4 */
extern const float d_0054dcb8;                                   /* 0x0054dcb8 */
extern const float d_0054dcbc;                                   /* 0x0054dcbc */
extern const float d_0054dcc0;                                   /* 0x0054dcc0 */
extern const float d_0054dcc4;                                   /* 0x0054dcc4 */
extern const uint32_t d_0054dcc8[2];                             /* 0x0054dcc8 */
extern const double d_0054dcd0;                                  /* 0x0054dcd0 */
extern const float d_0054dcd8;                                   /* 0x0054dcd8 */
extern const float d_0054dcdc;                                   /* 0x0054dcdc */
extern const float d_0054dce0;                                   /* 0x0054dce0 */
extern const uint32_t d_0054dce4;                                /* 0x0054dce4 */
extern const BmxType_gZrwKeyActions gZrwKeyActions;              /* 0x0054dce8 */
extern const float d_0054e0e8[2];                                /* 0x0054e0e8 */
extern const uint32_t d_0054e0f0[4];                             /* 0x0054e0f0 */
extern const float d_0054e100[2];                                /* 0x0054e100 */
extern const uint32_t d_0054e108[7];                             /* 0x0054e108 */
extern const uint32_t d_0054e124;                                /* 0x0054e124 */
extern const float d_0054e128;                                   /* 0x0054e128 */
extern const float d_0054e12c;                                   /* 0x0054e12c */
extern const BmxType_gZrwSeekOrigins gZrwSeekOrigins;            /* 0x0054e130 */
extern const float d_0054e13c;                                   /* 0x0054e13c */
extern const float d_0054e140;                                   /* 0x0054e140 */
extern const float d_0054e144;                                   /* 0x0054e144 */
extern const float d_0054e148[2];                                /* 0x0054e148 */
extern const double d_0054e150;                                  /* 0x0054e150 */
extern const float d_0054e158;                                   /* 0x0054e158 */
extern const char s_0054e15c[4];                                 /* 0x0054e15c */
extern const char s_0054e160[4];                                 /* 0x0054e160 */
extern const char sMicrosoftVisualCRuntimeLibrary[40];           /* 0x0054e164 */
extern const char sPressRetryToDebugTheApplicationJITMustB[64];  /* 0x0054e18c */
extern const char sForInformationOnHowYourProgramCanCauseA[112]; /* 0x0054e1cc */
extern const char sExpression[16];                               /* 0x0054e23c */
extern const char sLine[8];                                      /* 0x0054e24c */
extern const char sFile[8];                                      /* 0x0054e254 */
extern const char sProgramNameUnknown[24];                       /* 0x0054e25c */
extern const char sProgram[12];                                  /* 0x0054e274 */
extern const char sAssertionFailed[24];                          /* 0x0054e280 */
extern const struct BmxGroup_d_0054e298Group d_0054e298Group;    /* 0x0054e298 */
extern const double d_0054e2a8;                                  /* 0x0054e2a8 */
extern const double d_0054e2b0;                                  /* 0x0054e2b0 */
extern const double d_0054e2b8;                                  /* 0x0054e2b8 */
extern const char sIsProcessorFeaturePresent[28];                /* 0x0054e2c0 */
extern const char sKERNEL32[12];                                 /* 0x0054e2dc */
extern const char sE000[8];                                      /* 0x0054e2e8 */
extern const char sGLOBALHEAPSELECTED[24];                       /* 0x0054e2f0 */
extern const char sMSVCRTHEAPSELECT[24];                         /* 0x0054e308 */
extern const char sGetLastActivePopup[20];                       /* 0x0054e320 */
extern const uint32_t d_0054e334[2];                             /* 0x0054e334 */
extern const char sEWindow[8];                                   /* 0x0054e33c */
extern const char sMessageBoxA[12];                              /* 0x0054e344 */
extern const char sUser32Dll[12];                                /* 0x0054e350 */
extern const uint32_t d_0054e35c[23];                            /* 0x0054e35c */
extern const char s_0054e3b8[16];                                /* 0x0054e3b8 */
extern const char sNull[8];                                      /* 0x0054e3c8 */
extern const char sRuntimeError[16];                             /* 0x0054e3d0 */
extern const char s_0054e3e0[4];                                 /* 0x0054e3e0 */
extern const char sTLOSSError[16];                               /* 0x0054e3e4 */
extern const char sSINGError[16];                                /* 0x0054e3f4 */
extern const char sDOMAINError[16];                              /* 0x0054e404 */
extern const char sR6028UnableToInitializeHeap[40];              /* 0x0054e414 */
extern const char sR6027NotEnoughSpaceForLowioInitializati[56];  /* 0x0054e43c */
extern const char sR6026NotEnoughSpaceForStdioInitializati[56];  /* 0x0054e474 */
extern const char sR6025PureVirtualFunctionCall[40];             /* 0x0054e4ac */
extern const char sR6024NotEnoughSpaceForOnexitAtexitTable[56];  /* 0x0054e4d4 */
extern const char sR6019UnableToOpenConsoleDevice[44];           /* 0x0054e50c */
extern const char sR6018UnexpectedHeapError[36];                 /* 0x0054e538 */
extern const char sR6017UnexpectedMultithreadLockError[48];      /* 0x0054e55c */
extern const char sR6016NotEnoughSpaceForThreadData[44];         /* 0x0054e58c */
extern const char sAbnormalProgramTermination[36];               /* 0x0054e5b8 */
extern const char sR6009NotEnoughSpaceForEnvironment[44];        /* 0x0054e5dc */
extern const char sR6008NotEnoughSpaceForArguments[44];          /* 0x0054e608 */
extern const char sR6002FloatingPointNotLoaded[40];              /* 0x0054e634 */
extern const char sRuntimeErrorProgram[36];                      /* 0x0054e65c */
extern const double d_0054e680;                                  /* 0x0054e680 */
extern const unsigned char d_0054e688[2];                        /* 0x0054e688 */
extern const unsigned char d_0054e68a[10];                       /* 0x0054e68a */
extern const uint32_t d_0054e694[2];                             /* 0x0054e694 */
extern const uint32_t d_0054e69c[2];                             /* 0x0054e69c */
extern const float d_0054e6a4[2];                                /* 0x0054e6a4 */
extern const float d_0054e6ac[2];                                /* 0x0054e6ac */
extern const uint32_t d_0054e6b4[2];                             /* 0x0054e6b4 */
extern const unsigned char d_0054e6bc[8];                        /* 0x0054e6bc */
extern const char sYn[4];                                        /* 0x0054e6c4 */
extern const char sY1[4];                                        /* 0x0054e6c8 */
extern const char sY0[4];                                        /* 0x0054e6cc */
extern const char sFrexp[8];                                     /* 0x0054e6d0 */
extern const char sFmod[8];                                      /* 0x0054e6d8 */
extern const char sHypot[8];                                     /* 0x0054e6e0 */
extern const char sCabs[8];                                      /* 0x0054e6e8 */
extern const char sLdexp[8];                                     /* 0x0054e6f0 */
extern const char sModf[8];                                      /* 0x0054e6f8 */
extern const char sFabs[8];                                      /* 0x0054e700 */
extern const char sFloor[8];                                     /* 0x0054e708 */
extern const char sCeil[8];                                      /* 0x0054e710 */
extern const char sTan[4];                                       /* 0x0054e718 */
extern const char sCos[4];                                       /* 0x0054e71c */
extern const char sSin[4];                                       /* 0x0054e720 */
extern const char sSqrt[8];                                      /* 0x0054e724 */
extern const char sAtan2[8];                                     /* 0x0054e72c */
extern const char sAtan[8];                                      /* 0x0054e734 */
extern const char sAcos[8];                                      /* 0x0054e73c */
extern const char sAsin[8];                                      /* 0x0054e744 */
extern const char sTanh[8];                                      /* 0x0054e74c */
extern const char sCosh[8];                                      /* 0x0054e754 */
extern const char sSinh[8];                                      /* 0x0054e75c */
extern const char sLog10[8];                                     /* 0x0054e764 */
extern const char sLog[4];                                       /* 0x0054e76c */
extern const char sPow[4];                                       /* 0x0054e770 */
extern const char sExp[4];                                       /* 0x0054e774 */
extern const unsigned char d_0054e778[4];                        /* 0x0054e778 */
extern const unsigned char d_0054e77c[4];                        /* 0x0054e77c */
extern const struct BmxGroup_d_0054e780Group d_0054e780Group;    /* 0x0054e780 */
extern const struct BmxGroup_d_0054e798Group d_0054e798Group;    /* 0x0054e798 */
extern const char s1QNAN[8];                                     /* 0x0054e7a4 */
extern const char s1INF[8];                                      /* 0x0054e7ac */
extern const char s1IND[8];                                      /* 0x0054e7b4 */
extern const char s1SNAN[12];                                    /* 0x0054e7bc */
extern const struct BmxGroup_gZpadFormatJoystickGroup gZpadFormatJoystickGroup; /* 0x0054e7c8 */
extern const struct BmxGroup_gZpadFormatKeyboardGroup gZpadFormatKeyboardGroup; /* 0x0054e7e0 */
extern const struct BmxGroup_gZpadGuidKeyboardGroup gZpadGuidKeyboardGroup;     /* 0x0054f2f8 */
extern const uint32_t d_0054f328[4];                                            /* 0x0054f328 */
extern const uint32_t d_0054f338[8];                                            /* 0x0054f338 */
extern const uint32_t d_0054f358[4];                                            /* 0x0054f358 */
extern const uint32_t d_0054f368[4];                                            /* 0x0054f368 */
extern const uint32_t d_0054f378[4];                                            /* 0x0054f378 */
extern const uint32_t d_0054f388[4];                                            /* 0x0054f388 */
extern const uint32_t d_0054f398[4];                                            /* 0x0054f398 */
extern const uint32_t d_0054f3a8[4];                                            /* 0x0054f3a8 */
extern const uint32_t d_0054f3b8[20];                                           /* 0x0054f3b8 */
extern const struct BmxGroup_gZpadIidDeviceGroup gZpadIidDeviceGroup;           /* 0x0054f408 */
extern const struct BmxGroup_gZpadIidDirectInputGroup gZpadIidDirectInputGroup; /* 0x0054f448 */
extern const uint32_t d_0054fee8[32];                                           /* 0x0054fee8 */
extern const uint32_t d_0054ff68[8];                                            /* 0x0054ff68 */
extern const uint32_t d_0054ff88[4];                                            /* 0x0054ff88 */
extern const uint32_t d_0054ff98[12];                                           /* 0x0054ff98 */
extern const uint32_t d_0054ffc8[132];                                          /* 0x0054ffc8 */
extern const uint32_t d_005501d8[4];                                            /* 0x005501d8 */
extern const uint32_t d_005501e8[20];                                           /* 0x005501e8 */
extern const double d_00550238;                                                 /* 0x00550238 */
extern const struct BmxGroup_d_00550240Group d_00550240Group;                   /* 0x00550240 */
extern const char sSunMonTueWedThuFriSat[23];                                   /* 0x0055024c */
extern const unsigned char pad_00550263[41];                                    /* 0x00550263 */
extern const char sTZ[4];                                                       /* 0x0055028c */
extern const struct BmxGroup_d_00550290Group d_00550290Group;                   /* 0x00550290 */
extern unsigned char d_00551000[260];                                           /* 0x00551000 */
extern unsigned char d_00551104[260];                                           /* 0x00551104 */
extern struct BmxGroup_d_00551208Group d_00551208Group;                         /* 0x00551208 */
extern unsigned char d_00551418[260];                                           /* 0x00551418 */
extern struct BmxGroup_d_0055151cGroup d_0055151cGroup;                         /* 0x0055151c */
extern unsigned char d_00551724[260];                                           /* 0x00551724 */
extern unsigned char d_00551828[260];                                           /* 0x00551828 */
extern unsigned char d_0055192c[260];                                           /* 0x0055192c */
extern uint8_t gActionMode;                                                     /* 0x00551a30 */
extern ActionBytes6 gActionDemoTable;                                           /* 0x00551a34 */
extern int32_t gActionFrameReload;                                              /* 0x00551a3c */
extern int32_t gActionFrameCount;                                               /* 0x00551a40 */
extern char sFailedToOpenMotions[28];                                           /* 0x00551a48 */
extern char sCZaxisBmxSrcComActionActionC[44];                                  /* 0x00551a64 */
extern char sZARTVIEWFailedToOpenMotionsS[44];                                  /* 0x00551a90 */
extern char sAssetsMOTIONZAL[24];                                               /* 0x00551abc */
extern char sFailedToCreateDemoRider[36];                                       /* 0x00551ad4 */
extern char sCodeDoesNotMatchMotionAssetFile[48];                               /* 0x00551af8 */
extern char sZARTVIEWFailedToOpenMotionBankD[52];                               /* 0x00551b28 */
extern char sFailedToLoadAssets[28];                                            /* 0x00551b5c */
extern char sFailedToCreateRiderLd[36];                                         /* 0x00551b78 */
extern char sAPPShutdownACTION[28];                                             /* 0x00551b9c */
extern char sReplayDataNotFoundInAssetLibrary[48];                              /* 0x00551bb8 */
extern char sReplayDataTooBigForDemoReplayBuffer[52];                           /* 0x00551be8 */
extern char sFailedToCreateReplayProbablyOutOfRAM[56];                          /* 0x00551c1c */
extern struct BmxGroup_gCameraTuningGroup gCameraTuningGroup;                   /* 0x00551c54 */
extern char sPlayerNumberOutOfRange[32];                                        /* 0x00551f68 */
extern char sCZaxisBmxSrcComActionCameraC[48];                                  /* 0x00551f88 */
extern BmxType_gGameLevelInfoIndex gGameLevelInfoIndex; /* 0x00551fb8, also gActionMapTable */
extern uint32_t d_00551fc4[4];                          /* 0x00551fc4 */
extern uint32_t gTitleUnknown551fd4;                    /* 0x00551fd4, also gActionQuit, gNgQuit */
extern struct BmxGroup_bmxSaveRecords bmxSaveRecords;   /* 0x00551fd8 */
extern struct BmxGroup_gScoreRecordSlotGroup gScoreRecordSlotGroup;   /* 0x00553658 */
extern char sPlayer[8];                                               /* 0x00553e84 */
extern char sInvalidLevelForGoalInfo[36];                             /* 0x00553e8c */
extern char sCZaxisBmxSrcComActionComdataC[48];                       /* 0x00553eb0 */
extern struct BmxGroup_gContrlerBindingsGroup gContrlerBindingsGroup; /* 0x00553ee0 */
extern char sCouldNotCreateControllerDevice[44];                      /* 0x005545f8 */
extern char sCZaxisBmxSrcComActionContrlerC[48];                      /* 0x00554624 */
extern char sActionStatusOutOfRange[32];                              /* 0x00554654 */
extern char sActionOutOfRange[24];                                    /* 0x00554674 */
extern char sControllerConfigOutOfRange[40];                          /* 0x0055468c */
extern char sStatusOutOfRange[24];                                    /* 0x005546b4 */
extern char sJoystickNumberOutOfRange[36];                            /* 0x005546cc */
extern char sControllerNumberOutOfRange[40];                          /* 0x005546f0 */
extern void *gFxHandlerTable[3];                                      /* 0x00554718 */
extern struct BmxGroup_gFxBurstBlockGroup gFxBurstBlockGroup;         /* 0x00554724 */
extern struct BmxGroup_gFxPuffBlockGroup gFxPuffBlockGroup;           /* 0x00554738 */
extern BmxType_gFxScatterSlots gFxScatterSlots;                       /* 0x00554798 */
extern uint32_t d_005548e8[4];                                        /* 0x005548e8 */
extern uint32_t d_005548f8[4];                                        /* 0x005548f8 */
extern uint32_t d_00554908[2];                                        /* 0x00554908 */
extern uint32_t d_00554910[2];                                        /* 0x00554910 */
extern uint32_t d_00554918[2];                                        /* 0x00554918 */
extern uint32_t d_00554920[2];                                        /* 0x00554920 */
extern struct BmxGroup_d_00554928Group d_00554928Group;               /* 0x00554928 */
extern uint32_t d_00554954[4];                                        /* 0x00554954 */
extern struct BmxGroup_gFxSkidCornersGroup gFxSkidCornersGroup;       /* 0x00554964 */
extern uint32_t d_005549ac[20];                                       /* 0x005549ac */
extern uint32_t d_005549fc[20];                                       /* 0x005549fc */
extern uint32_t d_00554a4c[20];                                       /* 0x00554a4c */
extern BmxType_gFxPresets gFxPresets;                                 /* 0x00554a9c */
extern BmxType_gFxEmitBlock gFxEmitBlock;                             /* 0x00554ab8 */
extern struct BmxGroup_gFxCreateBlockGroup gFxCreateBlockGroup;       /* 0x00554ad8 */
extern float d_00554b14[56];                                          /* 0x00554b14 */
extern char sSpark[8];                                                /* 0x00554bf4 */
extern char sDust[8];                                                 /* 0x00554bfc */
extern char sShard[8];                                                /* 0x00554c04 */
extern char sSkid[8];                                                 /* 0x00554c0c */
extern char sTypeOutOfRange[24];                                      /* 0x00554c14 */
extern char sCZaxisBmxSrcComActionFxC[40];                            /* 0x00554c2c */
extern char sSplash[8];                                               /* 0x00554c54 */
extern char sRipple[8];                                               /* 0x00554c5c */
extern uint32_t d_00554c64[7];                                        /* 0x00554c64 */
extern struct BmxGroup_gGameRoundCountsGroup gGameRoundCountsGroup;   /* 0x00554c80 */
extern uint32_t gGameRideSeconds;                                     /* 0x00554cb0 */
extern GameMarkerColors gGameMarkerColors;                            /* 0x00554cb4 */
extern GameJudgeBias gGameJudgeBias;                                  /* 0x00554cc0 */
extern char sNoStoredHighScoreEntryFound[40];                         /* 0x00554cd8 */
extern uint32_t gGameDefaultWord;                                     /* 0x00554d00 */
extern char sCZaxisBmxSrcComActionGameC[44];                          /* 0x00554d04 */
extern char sRoundNumberOutOfRange[32];                               /* 0x00554d30 */
extern char sHorseWordTooLong[24];                                    /* 0x00554d50 */
extern char sImproperLevelForObjectiveLogic[44];                      /* 0x00554d68 */
extern char sJudgeDAwardsD[28];                                       /* 0x00554d94 */
extern char sNonRandomJudgeScoreIsD[36];                              /* 0x00554db0 */
extern char sNumberOfFallsD[28];                                      /* 0x00554dd4 */
extern char sNumberOfManualsD[28];                                    /* 0x00554df0 */
extern char sNumberOfStallsD[28];                                     /* 0x00554e0c */
extern char sNumberOfGrindsD[28];                                     /* 0x00554e28 */
extern char sNumberOfModifiedTricksD[40];                             /* 0x00554e44 */
extern char sNumberOfModifiersD[32];                                  /* 0x00554e6c */
extern char sNumberOfTricksD[28];                                     /* 0x00554e8c */
extern char sTotalAreasUsedD[28];                                     /* 0x00554ea8 */
extern char sTotalUniqueModifiersPerformedD[48];                      /* 0x00554ec4 */
extern char sTotalUniqueTricksPerformedD[44];                         /* 0x00554ef4 */
extern char sJudgeScoreCalculation[32];                               /* 0x00554f20 */
extern char sGoalCompletionErrorLevel[36];                            /* 0x00554f40 */
extern char sGoalCompletionErrorChar[36];                             /* 0x00554f64 */
extern char sSaveSAVEDAT[20];                                         /* 0x00554f88 */
extern char sErrorAllocatingTemporaryBufferForMemCar[64];             /* 0x00554f9c */
extern char sCZaxisBmxSrcComActionMemcardC[48];                       /* 0x00554fdc */
extern char sErrorCouldnTOpenFileSForReading[52];                     /* 0x0055500c */
extern char sErrorCouldnTReadFromFileS[44];                           /* 0x00555040 */
extern char sErrorMemoryCardDataIsInvalidProbablyOld[72];             /* 0x0055506c */
extern char sErrorCouldnTOpenFileSForWriting[52];                     /* 0x005550b4 */
extern char sErrorCouldnTWriteToFileS[48];                            /* 0x005550e8 */
extern GameGoalTable gGameGoals;                                      /* 0x00555118 */
extern struct BmxGroup_gGoalEntriesAGroup gGoalEntriesAGroup;         /* 0x005568b8 */
extern struct BmxGroup_gGoalEntriesBGroup gGoalEntriesBGroup;         /* 0x00556948 */
extern int8_t gOvlSavedSubMode;                                       /* 0x00556ef8 */
extern int32_t gOvlSaveStep;                                          /* 0x00556efc */
extern OvlElementTable gOvlElements;                                  /* 0x00556f00 */
extern void *p_00557098;                                              /* 0x00557098 */
extern void *p_0055709c;                                              /* 0x0055709c */
extern uint32_t d_005570a0[2];                                        /* 0x005570a0 */
extern struct BmxGroup_p_005570a8Group p_005570a8Group;               /* 0x005570a8 */
extern struct BmxGroup_p_005570b8Group p_005570b8Group;               /* 0x005570b8 */
extern struct BmxGroup_p_005570c8Group p_005570c8Group;               /* 0x005570c8 */
extern struct BmxGroup_p_005570d8Group p_005570d8Group;               /* 0x005570d8 */
extern struct BmxGroup_d_005570e8Group d_005570e8Group;               /* 0x005570e8 */
extern struct BmxGroup_d_005570f8Group d_005570f8Group;               /* 0x005570f8 */
extern struct BmxGroup_p_00557108Group p_00557108Group;               /* 0x00557108 */
extern struct BmxGroup_p_00557118Group p_00557118Group;               /* 0x00557118 */
extern struct BmxGroup_p_00557128Group p_00557128Group;               /* 0x00557128 */
extern struct BmxGroup_p_00557138Group p_00557138Group;               /* 0x00557138 */
extern struct BmxGroup_p_00557148Group p_00557148Group;               /* 0x00557148 */
extern struct BmxGroup_p_00557158Group p_00557158Group;               /* 0x00557158 */
extern struct BmxGroup_p_00557168Group p_00557168Group;               /* 0x00557168 */
extern struct BmxGroup_d_00557178Group d_00557178Group;               /* 0x00557178 */
extern uint8_t gOvlMpModeLines;                                       /* 0x00557184 */
extern struct BmxGroup_d_00557188Group d_00557188Group;               /* 0x00557188 */
extern struct BmxGroup_d_00557198Group d_00557198Group;               /* 0x00557198 */
extern struct BmxGroup_d_005571a8Group d_005571a8Group;               /* 0x005571a8 */
extern OvlScreenTable gOvlScreens;                                    /* 0x005571b8 */
extern OvlPageDefs gOvlSavePages;                                     /* 0x00557224 */
extern OvlMpWord gOvlMpWord;                                          /* 0x00557254 */
extern uint8_t gOvlMpWordCursor;                                      /* 0x0055725d */
extern struct BmxGroup_gOvlBarSizeGroup gOvlBarSizeGroup;             /* 0x00557260 */
extern struct BmxGroup_gOvlFadeSizeGroup gOvlFadeSizeGroup;           /* 0x00557298 */
extern struct BmxGroup_gIndicatorTextGroup gIndicatorTextGroup;       /* 0x005572bb */
extern char sSick[8];                                                 /* 0x005573ec */
extern char sQuad[8];                                                 /* 0x005573f4 */
extern char sTriple[8];                                               /* 0x005573fc */
extern char sDouble[8];                                               /* 0x00557404 */
extern char sP[4];                                                    /* 0x0055740c */
extern char sP_00557410[4];                                           /* 0x00557410 */
extern char sR[4];                                                    /* 0x00557414 */
extern char sSS[12];                                                  /* 0x00557418 */
extern char sC[4];                                                    /* 0x00557424 */
extern char sTo[4];                                                   /* 0x00557428 */
extern char s_0055742c[4];                                            /* 0x0055742c */
extern char sS[4];                                                    /* 0x00557430 */
extern char sCZaxisBmxSrcComActionOverlympC[48];                      /* 0x00557434 */
extern char sLd[4];                                                   /* 0x00557464 */
extern char sS_00557468[4];                                           /* 0x00557468 */
extern char sSS_0055746c[8];                                          /* 0x0055746c */
extern char sSLd[8];                                                  /* 0x00557474 */
extern char sSLd_0055747c[8];                                         /* 0x0055747c */
extern char s_00557484[4];                                            /* 0x00557484 */
extern char sLdLd[12];                                                /* 0x00557488 */
extern char sS_00557494[4];                                           /* 0x00557494 */
extern char sSSS[12];                                                 /* 0x00557498 */
extern char sSS_005574a4[8];                                          /* 0x005574a4 */
extern char sErrorParsingFontIcons[32];                               /* 0x005574ac */
extern char sMissingFontDataIcons[32];                                /* 0x005574cc */
extern char sErrorParsingFontButton[36];                              /* 0x005574ec */
extern char sMissingFontDataButton[32];                               /* 0x00557510 */
extern char sErrorParsingFontTiny[32];                                /* 0x00557530 */
extern char sMissingFontDataTiny[32];                                 /* 0x00557550 */
extern char sErrorParsingFontSmall[32];                               /* 0x00557570 */
extern char sNoMemoryForFont[24];                                     /* 0x00557590 */
extern char sMissingFontDataSmall[32];                                /* 0x005575a8 */
extern char sCZaxisBmxSrcComActionOverlayC[48];                       /* 0x005575c8 */
extern char sStringTableNotFoundInvalidLanguage[52];                  /* 0x005575f8 */
extern char sNoMemoryForStringTable[32];                              /* 0x0055762c */
extern char sSAVINGGAMEDATA[24];                                      /* 0x0055764c */
extern char sC_00557664[4];                                           /* 0x00557664 */
extern char sSS_00557668[8];                                          /* 0x00557668 */
extern char s_00557670[4];                                            /* 0x00557670 */
extern char s02dD[12];                                                /* 0x00557674 */
extern char sSBmxAssetsSReplayDemo02d02dRpy[52];                      /* 0x00557680 */
extern char sPC[4];                                                   /* 0x005576b4 */
extern char sUl[4];                                                   /* 0x005576b8 */
extern char s1ldC02ld[16];                                            /* 0x005576bc */
extern char sTO[4];                                                   /* 0x005576cc */
extern char sInvalidRecordTypeInDisplaySetupData[52];                 /* 0x005576d0 */
extern char sLdft[8];                                                 /* 0x00557704 */
extern char sTooManyButtonsInHintStringIncreaseAlloc[64];             /* 0x0055770c */
extern char s_0055774c[4];                                            /* 0x0055774c */
extern char sLdS[8];                                                  /* 0x00557750 */
extern char s0ldLd[16];                                               /* 0x00557758 */
extern char sFlail[8];                                                /* 0x00557768 */
extern char sDriver[12];                                              /* 0x00557770 */
extern char sVette[8];                                                /* 0x0055777c */
extern char sTripleFlip[16];                                          /* 0x00557784 */
extern char sLimo[8];                                                 /* 0x00557794 */
extern char sFlip[8];                                                 /* 0x0055779c */
extern char sFlair[8];                                                /* 0x005577a4 */
extern char sTruckDriver[16];                                         /* 0x005577ac */
extern char sS_005577bc[4];                                           /* 0x005577bc */
extern char sSpin[8];                                                 /* 0x005577c0 */
extern char sAlleyOop[12];                                            /* 0x005577c8 */
extern char sOpposite[12];                                            /* 0x005577d4 */
extern char sHalfCab[12];                                             /* 0x005577e0 */
extern BmxType_gPhysRollFriction gPhysRollFriction;                   /* 0x005577ec, also gPhysGrindDrag */
extern float gPhysAirDrag;                                            /* 0x00557800 */
extern float gPhysRollYawDamp;                                        /* 0x00557804 */
extern float gRiderAirYawDamp;                                        /* 0x00557808 */
extern float gPhysGrindYawDamp;                                       /* 0x0055780c */
extern float gPhysGravity;                                            /* 0x00557810 */
extern float gPhysRestSpeed;                                          /* 0x00557814 */
extern float gPhysLeanSpeedMax;                                       /* 0x00557818 */
extern float gPhysBailImpact;                                       /* 0x0055781c, also gPhysAirCrashSpeed */
extern float gPhysLandCrashSpeed;                                   /* 0x00557820 */
extern float gPhysWheelSideSlip;                                    /* 0x00557824 */
extern float gPhysRollSpinSpeed;                                    /* 0x00557828, also gPhysLeanSpeedMin */
extern struct BmxGroup_gPhysAirSpeedCapGroup gPhysAirSpeedCapGroup; /* 0x0055782c */
extern float gPhysPipeLaunchScale;                                  /* 0x00557834 */
extern float gPhysGrindMinSpeed;                                    /* 0x00557838 */
extern ZVec3 gPhysAirProbe;                                         /* 0x0055783c, also gPhysProbeOffset */
extern int16_t gPhysWallAngle;                                      /* 0x00557848 */
extern int16_t gPhysSlopeAngle;                                     /* 0x0055784a */
extern ZAngle gPhysMaxSlope;                                        /* 0x0055784c */
extern struct BmxGroup_gPhysRailAngleLimitGroup gPhysRailAngleLimitGroup; /* 0x0055784e */
extern int16_t gPhysPipeAngleLimit;                                       /* 0x00557856 */
extern int16_t gPhysPipeTiltLimit;                                        /* 0x00557858 */
extern uint32_t gPhysGrindInSteps;                                        /* 0x0055785c */
extern uint32_t gPhysStallInSteps;                                        /* 0x00557860 */
extern int32_t gPhysAirTurnSteps; /* 0x00557864, also gPhysAirAlignFrames */
extern struct BmxGroup_gPhysGrindInRadiusGroup gPhysGrindInRadiusGroup; /* 0x00557868 */
extern float gPhysEdgeSearchRadius;                                     /* 0x00557870 */
extern float gPhysGrindBalance;                                         /* 0x00557874 */
extern float gPhysSuspGravityScale;                                     /* 0x00557878 */
extern float gPhysRollSteer;                                            /* 0x0055787c */
extern float gPhysUpBlend;                  /* 0x00557880, also gPhysAlignRate, gPhysLandUpBlend */
extern float gPhysLeanMax;                  /* 0x00557884 */
extern ZVec3 gPhysDown;                     /* 0x00557888, also gPhysAirUp */
extern PhysTransitionTable gPhysStateExit;  /* 0x00557898 */
extern PhysTransitionTable gPhysStateEnter; /* 0x005578b4 */
extern PhysStepTable gPhysStateOrient;      /* 0x005578d0 */
extern PhysStepTable gPhysStateVelocity;    /* 0x005578ec */
extern PhysStepTable gPhysStateMove;        /* 0x00557908 */
extern struct BmxGroup_gPhysStateSyncGroup gPhysStateSyncGroup; /* 0x00557924 */
extern char sCouldnTFindRespawnFace[32];                        /* 0x00557984 */
extern char sCZaxisBmxSrcComActionPhysicsC[48];                 /* 0x005579a4 */
extern char sWarningNoTaggedRespawnFacesFoundInLevel[60];       /* 0x005579d4 */
extern char sWarningNoRespawnFacesFoundByDistanceChe[64];       /* 0x00557a10 */
extern char sBmxphysStateInitAirNoValidTerrainFoundU[100];      /* 0x00557a50 */
extern char sBmxphysStateInitAirQuarterpipeFaceWithN[84];       /* 0x00557ab4 */
extern char sAllComponentsOfYOrientationAreZero[52];            /* 0x00557b08 */
extern char sAllComponentsOfZOrientationAreZero[52];            /* 0x00557b3c */
extern char sAllComponentsOfXOrientationAreZero[52];            /* 0x00557b70 */
extern char sBmxphysUpdatePositionRollNoValidTerrain[100];      /* 0x00557ba4 */
extern char sBmxphysUpdatePositionRollQuarterpipeFac[88];       /* 0x00557c08 */
extern char sBmxphysUpdatePositionAirNoValidTerrainF[108];      /* 0x00557c60 */
extern char sBmxphysUpdatePositionAirQuarterpipeFace[92];       /* 0x00557ccc */
extern struct BmxGroup_gPropDefsGroup gPropDefsGroup;           /* 0x00557d28 */
extern char sBmxpropInitErrorFindingPropWithTagD[56];           /* 0x00558628 */
extern char sStateInfoOverflow[24];                             /* 0x00558660 */
extern char sCZaxisBmxSrcComActionPropC[44];                    /* 0x00558678 */
extern char sBmxpropInitErrorExtraPropWithTagDXDZD[72];         /* 0x005586a4 */
extern char sBmxpropInitErrorFindingFaceWithTagD[56];           /* 0x005586ec */
extern char sBmxpropGetPropByIDCouldnTFindPropD[52];            /* 0x00558724 */
extern char sBmxreplayUpdateReachedEndOfReplayBuffer[88];       /* 0x00558758 */
extern char sBmxreplayCreateFailedToAllocateBuffer[52];         /* 0x005587b0 */
extern char sCZaxisBmxSrcComActionReplayC[44];                  /* 0x005587e4 */
extern char sInvalidHandlePassedOrReplayHasBeenDestr[64];       /* 0x00558810 */
extern char sInvalidReplayHandle[28];                           /* 0x00558850 */
extern float gRiderHopSpeed;                                    /* 0x0055886c */
extern float gRiderHopBoost;                                    /* 0x00558870 */
extern float gRiderHopMargin;                                   /* 0x00558874 */
extern float gRiderJumpPower;                                   /* 0x00558878 */
extern struct BmxGroup_gRiderJumpBaseGroup gRiderJumpBaseGroup; /* 0x0055887c */
extern float gRiderLandGrip;                                    /* 0x00558888 */
extern float gRiderSteerBase;                                   /* 0x0055888c */
extern float gPhysLeanRange;                                    /* 0x00558890, also gRiderSteerSpeedGain */
extern float gRiderSteerFalloff;                                /* 0x00558894 */
extern float gRiderAirTurnNudge;                                /* 0x00558898 */
extern float gRiderAirTurnStart;                                /* 0x0055889c */
extern float gRiderAirTurnMax;                                  /* 0x005588a0 */
extern float gRiderBikeSpinStep;                                /* 0x005588a4 */
extern struct BmxGroup_gRiderBikeSpinMaxGroup gRiderBikeSpinMaxGroup; /* 0x005588a8 */
extern float gRiderTrickSteer;                                        /* 0x005588b0 */
extern uint32_t gRiderJumpFrames;                                     /* 0x005588b4 */
extern RiderModeTable gRiderModeStep;                                 /* 0x005588b8 */
extern struct BmxGroup_gRiderAirStepGroup gRiderAirStepGroup;         /* 0x005588d4 */
extern struct BmxGroup_gRiderGrindStepGroup gRiderGrindStepGroup;     /* 0x005588f8 */
extern char sFailedToCreateRiderActorForRiderLdLookN[72];             /* 0x00558a48 */
extern char sStateLevelOutOfRange[32];                                /* 0x00558a90 */
extern char sCouldnTAssociateRiderWithDynamicCollisi[71];             /* 0x00558ab0 */
extern struct BmxGroup_gRiderTrickComboGroup gRiderTrickComboGroup;   /* 0x00558af7 */
extern char sNoFootedCanCan[24];                                      /* 0x00559b94 */
extern char sDecade[8];                                               /* 0x00559bac */
extern char sDecadeBackflip[20];                                      /* 0x00559bb4 */
extern char sTailwhipBackflip[24];                                    /* 0x00559bc8 */
extern char sCandyBarBackflip[24];                                    /* 0x00559be0 */
extern char sSuperflip[12];                                           /* 0x00559bf8 */
extern char sTobagganBackflip[24];                                    /* 0x00559c04 */
extern char sTabletopBackflip[24];                                    /* 0x00559c1c */
extern char sCanCanBackflip[20];                                      /* 0x00559c34 */
extern char sTurndownBackflip[24];                                    /* 0x00559c48 */
extern char sDoubleFrontFlip[24];                                     /* 0x00559c60 */
extern char sFrontFlip[16];                                           /* 0x00559c78 */
extern char sTripleBackflip[20];                                      /* 0x00559c88 */
extern char sDoubleBackflip[20];                                      /* 0x00559c9c */
extern char sTobaggan[12];                                            /* 0x00559cb0 */
extern char sTailwhip[12];                                            /* 0x00559cbc */
extern char sCandyBar[12];                                            /* 0x00559cc8 */
extern char sSuperman[12];                                            /* 0x00559cd4 */
extern char sBackflip[12];                                            /* 0x00559ce0 */
extern char sTabletop[12];                                            /* 0x00559cec */
extern char sCanCan[12];                                              /* 0x00559cf8 */
extern struct BmxGroup_sTurndownGroup sTurndownGroup;                 /* 0x00559d04 */
extern char sCZaxisBmxSrcComMenuGuiGuiinputC[52];                     /* 0x00559d14 */
extern const char *gGuiAssetFile;                                     /* 0x00559d48 */
extern int gGuiPulseStep;                                             /* 0x00559d4c */
extern char sIntartZal[16];                                           /* 0x00559d54 */
extern char sGUIMAINErrorReinitializingApp[44];                       /* 0x00559d64 */
extern char sGUIMAINFailedToInitHelpSystem[44];                       /* 0x00559d90 */
extern char sGUIMAINFailedToLoadPermanentScreenAsset[60];             /* 0x00559dbc */
extern char sGUIMAINFailedToInitializeZPRIM[44];                      /* 0x00559df8 */
extern char sGUIMAINFailedToInitializeZMEMDMM[48];                    /* 0x00559e24 */
extern char sGUIMAINErrorReinitializingMenuMode[52];                  /* 0x00559e54 */
extern char sGUIMAINErrorInitializingMenuModeBeforeA[64];             /* 0x00559e88 */
extern char sGUIMAINFailedToDestroyAllElements[52];                   /* 0x00559ec8 */
extern char sGUIMAINErrorFailedToLoadScreen[48];                      /* 0x00559efc */
extern char sGUISFILEFailedToLoadScreenAssets[48];                    /* 0x00559f2c */
extern char sGUIMAINErrorCallingUpdateBeforeInitiali[68];             /* 0x00559f5c */
extern char sGUIMAINErrorFailedToAllocateElement[56];                 /* 0x00559fa0 */
extern char sGUIMAINErrorFreeingElementNotInList[56];                 /* 0x00559fd8 */
extern char sGUIMAINErrorFailedToLoadSprite044X[56];                  /* 0x0055a010 */
extern char sGUIMAINErrorFailedToLoadSelectedSprite0[68];             /* 0x0055a048 */
extern char sGUIMAINErrorFailedToLoadFont044X[56];                    /* 0x0055a08c */
extern char sGUIMAINErrorFailedToLoadBackdrop044X[60];                /* 0x0055a0c4 */
extern char sPOSTMESSAGENoTypeHandler[40];                            /* 0x0055a100 */
extern char sGUIMAINErrorSendingMessageDToNULLElemen[64];             /* 0x0055a128 */
extern char sGUISFILEErrorFailedToAllocateElement[56];                /* 0x0055a168 */
extern char sGUISFILEErrorBadScreenElementDef044X[60];                /* 0x0055a1a0 */
extern uint32_t gGuiElementMagic;                                     /* 0x0055a1dc */
extern char sGUISFILEErrorBadScreen044X[44];                          /* 0x0055a1e4 */
extern uint32_t gGuiScreenMagic;                                      /* 0x0055a210 */
extern char sGUISFILEErrorFailedToLoadScreenX[52];                    /* 0x0055a218 */
extern char sGUITOGGLFailedToLockToggleText[48];                      /* 0x0055a24c */
extern char sGUITOGGLFailedToLockToggleData[48];                      /* 0x0055a27c */
extern char sCZaxisBmxSrcComMenuGuiGuitogglC[52];                     /* 0x0055a2ac */
extern char sGUITOGGLErrorFailedToLoadFont044X[56];                   /* 0x0055a2e0 */
extern char sGUITOGGLErrorFailedToMallocTextString[60];               /* 0x0055a318 */
extern int gOptMainPulseStep;                                         /* 0x0055a354, also gExtrasPulseStep */
extern LanguageTable gLanguageTable;                                  /* 0x0055a358 */
extern MenuEntryStrings gMenuEntryStrings;                            /* 0x0055a35c */
extern MenuSlotPosTable gMenuSlotPos;                                 /* 0x0055a368 */
extern MenuSlotShortTable gMenuSlotAlpha;                             /* 0x0055a384 */
extern MenuSlotShortTable gMenuSlotTint;                              /* 0x0055a394 */
extern OptionStateTable gOptStateStrings;                             /* 0x0055a3b0 */
extern OptionTable gOptionTable;                                      /* 0x0055a3bc */
extern StripColorTable gStripColors;                                  /* 0x0055a3dc */
extern KeyNameTable gKeyNames;                                        /* 0x0055a3f4 */
extern KeyTitleTable gKbLayoutTitles;                                 /* 0x0055a43c */
extern struct BmxGroup_gKbLayoutLabelsGroup gKbLayoutLabelsGroup;     /* 0x0055a450 */
extern char sS4h[4];                                                  /* 0x0055a620 */
extern char sS4g[4];                                                  /* 0x0055a624 */
extern char sS4f[4];                                                  /* 0x0055a628 */
extern char sS4e[4];                                                  /* 0x0055a62c */
extern char sS4d[4];                                                  /* 0x0055a630 */
extern char sS4c[4];                                                  /* 0x0055a634 */
extern char sS4b[4];                                                  /* 0x0055a638 */
extern char sPGDOWN[8];                                               /* 0x0055a63c */
extern char sDEL[4];                                                  /* 0x0055a644 */
extern char sHOME[8];                                                 /* 0x0055a648 */
extern char sPGUP[8];                                                 /* 0x0055a650 */
extern char sINS[4];                                                  /* 0x0055a658 */
extern char sS3h[4];                                                  /* 0x0055a65c */
extern char sS3g[4];                                                  /* 0x0055a660 */
extern char sS3f[4];                                                  /* 0x0055a664 */
extern char sS3e[4];                                                  /* 0x0055a668 */
extern char sS3d[4];                                                  /* 0x0055a66c */
extern char sS3c[4];                                                  /* 0x0055a670 */
extern char sS3b[4];                                                  /* 0x0055a674 */
extern char sEND[4];                                                  /* 0x0055a678 */
extern char sNUMENTER[12];                                            /* 0x0055a67c */
extern char sCTRL[8];                                                 /* 0x0055a688 */
extern char sS2h[4];                                                  /* 0x0055a690 */
extern char sS2g[4];                                                  /* 0x0055a694 */
extern char sS2f[4];                                                  /* 0x0055a698 */
extern char sS2e[4];                                                  /* 0x0055a69c */
extern char sS2d[4];                                                  /* 0x0055a6a0 */
extern char sS2c[4];                                                  /* 0x0055a6a4 */
extern char sS2b[4];                                                  /* 0x0055a6a8 */
extern char sNUMPAD[12];                                              /* 0x0055a6ac */
extern char sNUMPAD9[12];                                             /* 0x0055a6b8 */
extern char sNUMPAD7[12];                                             /* 0x0055a6c4 */
extern char sS1i[4];                                                  /* 0x0055a6d0 */
extern char sS1h[4];                                                  /* 0x0055a6d4 */
extern char sS1g[4];                                                  /* 0x0055a6d8 */
extern char sS1f[4];                                                  /* 0x0055a6dc */
extern char sS1e[4];                                                  /* 0x0055a6e0 */
extern char sS1d[4];                                                  /* 0x0055a6e4 */
extern char sS1c[4];                                                  /* 0x0055a6e8 */
extern char sTILDE[8];                                                /* 0x0055a6ec */
extern char sA[4];                                                    /* 0x0055a6f4 */
extern char sSPACE[8];                                                /* 0x0055a6f8 */
extern char sNUMPAD0[12];                                             /* 0x0055a700 */
extern char sNUMPAD6[12];                                             /* 0x0055a70c */
extern char sNUMPAD4[12];                                             /* 0x0055a718 */
extern char sNUMPAD5[12];                                             /* 0x0055a724 */
extern char sNUMPAD8[12];                                             /* 0x0055a730 */
extern char sS0i[4];                                                  /* 0x0055a73c */
extern char sS0h[4];                                                  /* 0x0055a740 */
extern char sS0g[4];                                                  /* 0x0055a744 */
extern char sS0f[4];                                                  /* 0x0055a748 */
extern char sS0e[4];                                                  /* 0x0055a74c */
extern char sS0d[4];                                                  /* 0x0055a750 */
extern char sS0c[4];                                                  /* 0x0055a754 */
extern char sESCAPE[8];                                               /* 0x0055a758 */
extern char sW[4];                                                    /* 0x0055a760 */
extern char sE[4];                                                    /* 0x0055a764 */
extern char sF[4];                                                    /* 0x0055a768 */
extern char sS_0055a76c[4];                                           /* 0x0055a76c */
extern char sD[4];                                                    /* 0x0055a770 */
extern char sRIGHT[8];                                                /* 0x0055a774 */
extern char sLEFT[8];                                                 /* 0x0055a77c */
extern char sDOWN[8];                                                 /* 0x0055a784 */
extern char sUP[4];                                                   /* 0x0055a78c */
extern char sKEYBOARDLAYOUT5[28];                                     /* 0x0055a790 */
extern char sKEYBOARDLAYOUT4[28];                                     /* 0x0055a7ac */
extern char sKEYBOARDLAYOUT3[28];                                     /* 0x0055a7c8 */
extern char sKEYBOARDLAYOUT2[28];                                     /* 0x0055a7e4 */
extern char sKEYBOARDLAYOUT1[28];                                     /* 0x0055a800 */
extern char sFire6[8];                                                /* 0x0055a81c */
extern char sFire5[8];                                                /* 0x0055a824 */
extern char sSelect[8];                                               /* 0x0055a82c */
extern char sStart[8];                                                /* 0x0055a834 */
extern char sRTrig3[12];                                              /* 0x0055a83c */
extern char sLTrig3[12];                                              /* 0x0055a848 */
extern char sRTrig2[12];                                              /* 0x0055a854 */
extern char sLTrig2[12];                                              /* 0x0055a860 */
extern char sRTrig1[12];                                              /* 0x0055a86c */
extern char sLTrig1[12];                                              /* 0x0055a878 */
extern char sFire4[8];                                                /* 0x0055a884 */
extern char sFire3[8];                                                /* 0x0055a88c */
extern char sFire2[8];                                                /* 0x0055a894 */
extern char sFire1[8];                                                /* 0x0055a89c */
extern char sRight[8];                                                /* 0x0055a8a4 */
extern char sLeft[8];                                                 /* 0x0055a8ac */
extern char sDown[8];                                                 /* 0x0055a8b4 */
extern char sUp[4];                                                   /* 0x0055a8bc */
extern uint32_t d_0055a8c0[10];                                       /* 0x0055a8c0 */
extern uint32_t d_0055a8e8[10];                                       /* 0x0055a8e8 */
extern uint32_t d_0055a910[10];                                       /* 0x0055a910 */
extern uint32_t d_0055a938[10];                                       /* 0x0055a938 */
extern uint32_t d_0055a960[10];                                       /* 0x0055a960 */
extern uint32_t d_0055a988[10];                                       /* 0x0055a988 */
extern uint32_t d_0055a9b0[10];                                       /* 0x0055a9b0 */
extern uint32_t d_0055a9d8[10];                                       /* 0x0055a9d8 */
extern uint32_t d_0055aa00[10];                                       /* 0x0055aa00 */
extern uint32_t d_0055aa28[10];                                       /* 0x0055aa28 */
extern uint32_t d_0055aa50[10];                                       /* 0x0055aa50 */
extern uint32_t d_0055aa78[10];                                       /* 0x0055aa78 */
extern SelCaptionPtrTable gRiderCaptions;                             /* 0x0055aaa0 */
extern SelCaptionKinds gCaptionKinds;                                 /* 0x0055aad0 */
extern struct BmxGroup_gCaptionOrdersGroup gCaptionOrdersGroup;       /* 0x0055aaf0 */
extern SelCaptionLocked gCaptionLocked;                               /* 0x0055ab10 */
extern SelAssetTable gBikeNames;                                      /* 0x0055ab18 */
extern SelAssetTable gBikeTextures;                                   /* 0x0055ab30 */
extern SelCaptionLocked gBikeLocked;                                  /* 0x0055ab48 */
extern char sA_0055ab50[8];                                           /* 0x0055ab50 */
extern char sD_0055ab58[8];                                           /* 0x0055ab58 */
extern char sG[8];                                                    /* 0x0055ab60 */
extern char sJ[8];                                                    /* 0x0055ab68 */
extern char sJ_0055ab70[8];                                           /* 0x0055ab70 */
extern char sM[8];                                                    /* 0x0055ab78 */
extern char sP_0055ab80[8];                                           /* 0x0055ab80 */
extern char sS_0055ab88[8];                                           /* 0x0055ab88 */
extern char sV[8];                                                    /* 0x0055ab90 */
extern char sV_0055ab98[8];                                           /* 0x0055ab98 */
extern char sY[8];                                                    /* 0x0055aba0 */
extern char s_0055aba8[8];                                            /* 0x0055aba8 */
extern char s_0055abb0[8];                                            /* 0x0055abb0 */
extern char sB[8];                                                    /* 0x0055abb8 */
extern char sB_0055abc0[8];                                           /* 0x0055abc0 */
extern char sE_0055abc8[8];                                           /* 0x0055abc8 */
extern char sH[8];                                                    /* 0x0055abd0 */
extern char sK[8];                                                    /* 0x0055abd8 */
extern char sN[8];                                                    /* 0x0055abe0 */
extern char sN_0055abe8[8];                                           /* 0x0055abe8 */
extern char sQ[8];                                                    /* 0x0055abf0 */
extern char sT[8];                                                    /* 0x0055abf8 */
extern char sW_0055ac00[8];                                           /* 0x0055ac00 */
extern char sZ[8];                                                    /* 0x0055ac08 */
extern char sZ_0055ac10[8];                                           /* 0x0055ac10 */
extern char s_0055ac18[8];                                            /* 0x0055ac18 */
extern uint32_t d_0055ac20[2];                                        /* 0x0055ac20 */
extern uint32_t d_0055ac28[2];                                        /* 0x0055ac28 */
extern uint32_t d_0055ac30[2];                                        /* 0x0055ac30 */
extern uint32_t d_0055ac38[2];                                        /* 0x0055ac38 */
extern uint32_t d_0055ac40[2];                                        /* 0x0055ac40 */
extern uint32_t d_0055ac48[2];                                        /* 0x0055ac48 */
extern uint32_t d_0055ac50[2];                                        /* 0x0055ac50 */
extern uint32_t d_0055ac58[2];                                        /* 0x0055ac58 */
extern uint32_t d_0055ac60[2];                                        /* 0x0055ac60 */
extern uint32_t d_0055ac68[2];                                        /* 0x0055ac68 */
extern uint32_t d_0055ac70[2];                                        /* 0x0055ac70 */
extern uint32_t d_0055ac78[2];                                        /* 0x0055ac78 */
extern uint32_t d_0055ac80[2];                                        /* 0x0055ac80 */
extern uint32_t d_0055ac88[2];                                        /* 0x0055ac88 */
extern uint32_t d_0055ac90[2];                                        /* 0x0055ac90 */
extern uint32_t d_0055ac98[2];                                        /* 0x0055ac98 */
extern uint32_t d_0055aca0[2];                                        /* 0x0055aca0 */
extern uint32_t d_0055aca8[2];                                        /* 0x0055aca8 */
extern uint32_t d_0055acb0[2];                                        /* 0x0055acb0 */
extern uint32_t d_0055acb8[2];                                        /* 0x0055acb8 */
extern uint32_t d_0055acc0[2];                                        /* 0x0055acc0 */
extern uint32_t d_0055acc8[2];                                        /* 0x0055acc8 */
extern uint32_t d_0055acd0[2];                                        /* 0x0055acd0 */
extern uint32_t d_0055acd8[2];                                        /* 0x0055acd8 */
extern uint32_t d_0055ace0[2];                                        /* 0x0055ace0 */
extern uint32_t d_0055ace8[2];                                        /* 0x0055ace8 */
extern uint32_t d_0055acf0[2];                                        /* 0x0055acf0 */
extern uint32_t d_0055acf8[2];                                        /* 0x0055acf8 */
extern uint32_t d_0055ad00[2];                                        /* 0x0055ad00 */
extern uint32_t d_0055ad08[2];                                        /* 0x0055ad08 */
extern uint32_t d_0055ad10[2];                                        /* 0x0055ad10 */
extern uint32_t d_0055ad18[2];                                        /* 0x0055ad18 */
extern uint32_t d_0055ad20[2];                                        /* 0x0055ad20 */
extern uint32_t d_0055ad28[2];                                        /* 0x0055ad28 */
extern struct BmxGroup_gBikeCaptionsGroup gBikeCaptionsGroup;         /* 0x0055ad30 */
extern char sSD[12];                                                  /* 0x0055af34 */
extern ExtrasTitle gTitleIntro;                                       /* 0x0055af40 */
extern ExtrasTitle gTitleDave;                                        /* 0x0055afbc */
extern ExtrasTitle gTitleRyan;                                        /* 0x0055b038 */
extern ExtrasTitle gTitleAmish;                                       /* 0x0055b0b4 */
extern InfoHeadTable gInfoHeadIds;                                    /* 0x0055b130 */
extern struct BmxGroup_gInfoLineIdsGroup gInfoLineIdsGroup;           /* 0x0055b138 */
extern char sBinMovieAmishStr[24];                                    /* 0x0055b24c */
extern char sBinMovieNyquistStr[28];                                  /* 0x0055b264 */
extern char sBinMovieMirraStr[24];                                    /* 0x0055b280 */
extern char sBinMovieIntroStr[24];                                    /* 0x0055b298 */
extern char s6ld[8];                                                  /* 0x0055b2b0 */
extern char s3ld2ld[16];                                              /* 0x0055b2b8 */
extern BmxType_gCreditsMovie gCreditsMovie;                           /* 0x0055b2c8 */
extern char sFRONTENDFailedToAllocMemory[40];                         /* 0x0055b2e4 */
extern char sFRONTENDFailedToGuiInitializeMenuMode[52];               /* 0x0055b30c */
extern char sFRONTENDFailedToLoadFrontendMacros[52];                  /* 0x0055b340 */
extern char sFrontendInitAppFailedToGuiInitializeApp[56];             /* 0x0055b374 */
extern char sAPPInitMENU[20];                                         /* 0x0055b3ac */
extern char sFRONTENDMacroDataTooBigOrTooSmall[52];                   /* 0x0055b3c0 */
extern char sFRONTENDMacroDataTooBigOrTooSmall_0055b3f4[52];          /* 0x0055b3f4 */
extern char sFRONTENDFailedToLoadMacroStringSet[52];                  /* 0x0055b428 */
extern char sFRONTENDFailedToLoadMacroStringSet_0055b45c[52];         /* 0x0055b45c */
extern char sFRONTENDCouldNotFindMacroData[48];                       /* 0x0055b490 */
extern char sCZaxisBmxSrcComMenuFrontendC[44];                        /* 0x0055b4c0 */
extern char sFRONTENDCouldNotFindMacroData_0055b4ec[44];              /* 0x0055b4ec */
extern char sAPPShutdownMENU[24];                                     /* 0x0055b518 */
extern int32_t gBmxSoundState;                                        /* 0x0055b530 */
extern int32_t gBmxMusicMode;                                         /* 0x0055b534 */
extern BmxVolumeTable gBmxVolumes;                    /* 0x0055b538, also gBmxMusicVolume, gBmxSfxVolume */
extern uint16_t gBmxSfxFile;                          /* 0x0055b53c */
extern uint16_t gBmxMenuSfxFile;                      /* 0x0055b53e */
extern uint16_t gBmxGameSfxFile;                      /* 0x0055b540 */
extern uint16_t gBmxGunSfxFile;                       /* 0x0055b542 */
extern uint16_t gBmxRiderSfxFile;                     /* 0x0055b544 */
extern uint16_t gBmxLevelSfxFile;                     /* 0x0055b546 */
extern uint16_t gBmxMusicFile;                        /* 0x0055b548 */
extern uint16_t gBmxSpeechFile;                       /* 0x0055b54a */
extern uint8_t gBmxNextTrack;                         /* 0x0055b54c */
extern uint8_t gBmxTrackLoop;                         /* 0x0055b54d */
extern BmxLevelTrackTable gBmxLevelTracks;            /* 0x0055b550 */
extern char sErrorGettingTimerDeviceCapabilities[48]; /* 0x0055b564 */
extern char sErrorCreatingTimerEvent[32];             /* 0x0055b594 */
extern char sFailureLoadingSpeechAssetFile[40];       /* 0x0055b5b4 */
extern char sSPEECHZSD[16];                           /* 0x0055b5dc */
extern char sFailureLoadingMusicAssetFile[40];        /* 0x0055b5ec */
extern char sMUSICZSD[12];                            /* 0x0055b614 */
extern char sFailureLoadingCommonSFXAssetFile[48];    /* 0x0055b620 */
extern char sBinAudio[16];                            /* 0x0055b650 */
extern char sSFXCOMZSD[16];                           /* 0x0055b660 */
extern char sFailureInitializing3DSoundSourceManager[56];                     /* 0x0055b670 */
extern char sFailureInitializingSequencedAudioManage[56];                     /* 0x0055b6a8 */
extern char sFailureInitializingChannelManager[44];                           /* 0x0055b6e0 */
extern char sSoundAlreadyOpen[24];                                            /* 0x0055b70c */
extern char sCZaxisBmxSrcComZgeBmxBmxsoundC[48];                              /* 0x0055b724 */
extern char sSoundAlreadyClosed[28];                                          /* 0x0055b754 */
extern char sPropLevelSoundTypeOutOfRange[44];                                /* 0x0055b770 */
extern char sNULLPointerPassedAsPositionalSFXArgumen[56];                     /* 0x0055b79c */
extern char sFailureLoadingMenuSFXAssetFile[44];                              /* 0x0055b7d4 */
extern char sSFXMEN02dZSD[20];                                                /* 0x0055b800 */
extern char sFailureCreatingMusicStreamProbablyOutOf[68];                     /* 0x0055b814 */
extern char sFailureLoadingCharacterSFXAssetFile[52];                         /* 0x0055b858 */
extern char sSFXCHR02dZSD[20];                                                /* 0x0055b88c */
extern char sFailureLoadingLevelSFXAssetFile[44];                             /* 0x0055b8a0 */
extern char sFailureLoadingGunSFXAssetFile[44];                               /* 0x0055b8cc */
extern char sSFXGAME2ZSD[16];                                                 /* 0x0055b8f8 */
extern char sSFXLEV02dZSD[20];                                                /* 0x0055b908 */
extern char sFailureLoadingGameSFXAssetFile[44];                              /* 0x0055b91c */
extern char sSFXGAMEZSD[16];                                                  /* 0x0055b948 */
extern float d_0055b958;                                                      /* 0x0055b958 */
extern float d_0055b95c;                                                      /* 0x0055b95c */
extern struct BmxGroup_gZoverGlyphGroup gZoverGlyphGroup;                     /* 0x0055b960 */
extern char sDirtyListOverflow[24];                                           /* 0x0055b97c */
extern char sFreeListOverflow[24];                                            /* 0x0055b994 */
extern char sCZaxisBmxSrcComZgeBmxZoveriiC[48];                               /* 0x0055b9ac */
extern char sNullFontInOverlay[28];                                           /* 0x0055b9dc */
extern char sOverflow[12];                                                    /* 0x0055b9f8 */
extern struct BmxGroup_gNgAssetsDirGroup gNgAssetsDirGroup;                   /* 0x0055ba04 */
extern char sReplayZal[16];                                                   /* 0x0055ba30 */
extern char sTablesZal[16];                                                   /* 0x0055ba40 */
extern char sSystemZal[16];                                                   /* 0x0055ba50 */
extern char sWSsndZal[12];                                                    /* 0x0055ba60 */
extern char sMiscdataZal[16];                                                 /* 0x0055ba6c */
extern char sMaterialZal[16];                                                 /* 0x0055ba7c */
extern char sGeometryZal[16];                                                 /* 0x0055ba8c */
extern char sMotionZal[16];                                                   /* 0x0055ba9c */
extern char sAssets[12];                                                      /* 0x0055baac */
extern char sSDS[12];                                                         /* 0x0055bab8 */
extern char sCZaxisBmxSrcComZgeNgIFMissingZGEC[52];                           /* 0x0055bac4 */
extern char sZconsoleLog[16];                                                 /* 0x0055baf8 */
extern char sEnableHorizon[20];                                               /* 0x0055bb08 */
extern char sZglDisableHorizon[24];                                           /* 0x0055bb1c */
extern char sZsystemSetActionMode[28];                                        /* 0x0055bb34 */
extern char sZsystemSetMenuMode[24];                                          /* 0x0055bb50 */
extern char sZfxDestroyAll[16];                                               /* 0x0055bb68 */
extern char sZfxRegisterFuncTable[28];                                        /* 0x0055bb78 */
extern char sAVI[4];                                                          /* 0x0055bb94 */
extern struct BmxGroup_gNgRamDiskGroup gNgRamDiskGroup;                       /* 0x0055bb98 */
extern BmxType_gNgLightColors gNgLightColors;                                 /* 0x0055bcf0 */
extern struct BmxGroup_gNgSpotMainColorGroup gNgSpotMainColorGroup;           /* 0x0055bcfc */
extern uint32_t gNgLegalTime;                                                 /* 0x0055bd04 */
extern float gFxQuality;                                                      /* 0x0055bd08 */
extern struct BmxGroup_gFxFpsGroup gFxFpsGroup;                               /* 0x0055bd0c */
extern char sTestB[8];                                                        /* 0x0055bd5c */
extern char sTestA[8];                                                        /* 0x0055bd64 */
extern char sAcclaimVert[16];                                                 /* 0x0055bd6c */
extern char sAcclaimStreet[20];                                               /* 0x0055bd7c */
extern char sAcclaimDirt[16];                                                 /* 0x0055bd90 */
extern char sProTourVert[16];                                                 /* 0x0055bda0 */
extern char sProTourStreet[20];                                               /* 0x0055bdb0 */
extern char sProTourDirt[16];                                                 /* 0x0055bdc4 */
extern char sWoodwardVert[16];                                                /* 0x0055bdd4 */
extern char sWoodwardStreet[20];                                              /* 0x0055bde4 */
extern char sWoodwardDirt[16];                                                /* 0x0055bdf8 */
extern char sGreenvilleVert[20];                                              /* 0x0055be08 */
extern char sGreenvilleStreet[24];                                            /* 0x0055be1c */
extern char sGreenvilleDirt[20];                                              /* 0x0055be34 */
extern char sIntwrapZal[16];                                                  /* 0x0055be48 */
extern char s155417[12];                                                      /* 0x0055be58 */
extern char s_0055be64[4];                                                    /* 0x0055be64 */
extern char sNov222000[16];                                                   /* 0x0055be68 */
extern char sNextGenBMX[20];                                                  /* 0x0055be78 */
extern char sLight[8];                                                        /* 0x0055be8c */
extern char sSpotLightT[16];                                                  /* 0x0055be94 */
extern char sSpotLight[12];                                                   /* 0x0055bea4 */
extern char sCouldNotGenerateFileNameForRAMDisk[52];                          /* 0x0055beb0 */
extern char sSIZE[8];                                                         /* 0x0055bee4 */
extern char sADDRESS[12];                                                     /* 0x0055beec */
extern char sCouldNotLocateARAMDiskDeviceOnThisSyste[64];                     /* 0x0055bef8 */
extern ZramdiskDeviceName gZramdiskDeviceName;                                /* 0x0055bf38 */
extern char sNoDevices[16];                                                   /* 0x0055bf40 */
extern char sCouldNotLoadMenuArtAssetFile[44];                                /* 0x0055bf50 */
extern char sInterfaceAssetWrapperDoesNotExist[48];                           /* 0x0055bf7c */
extern char sInterfaceArtAlreadyLoaded[36];                                   /* 0x0055bfac */
extern char sCZaxisBmxSrcComZgeNgIFNgAppC[48];                                /* 0x0055bfd0 */
extern char sBinMovieINTROAVI[24];                                            /* 0x0055c000 */
extern char sBinMovieZAXISAVI[24];                                            /* 0x0055c018 */
extern char sBinMovieACCLAIMAVI[28];                                          /* 0x0055c030 */
extern char sDaveMirraFreestyleBMX[32];                                       /* 0x0055c04c */
extern char sLOADING[12];                                                     /* 0x0055c06c */
extern char sGlobals[12];                                                     /* 0x0055c078 */
extern char sErrorLoadingLevel[24];                                           /* 0x0055c084 */
extern char sFailedToLoadInterfaceAssets[40];                                 /* 0x0055c09c */
extern char sLEGAL2[8];                                                       /* 0x0055c0c4 */
extern char sLEGAL[12];                                                       /* 0x0055c0cc */
extern const char *gFxSkidBase;                                               /* 0x0055c0d8 */
extern BmxType_gFxSkidFrameNames gFxSkidFrameNames;                           /* 0x0055c0dc */
extern const char *gFxSkidName;                                               /* 0x0055c11c */
extern uint32_t gFxSkidConfig;                                                /* 0x0055c120 */
extern struct BmxGroup_gFxSparkDefaultGroup gFxSparkDefaultGroup;             /* 0x0055c128 */
extern struct BmxGroup_gFxDustDefaultGroup gFxDustDefaultGroup;               /* 0x0055c1a0 */
extern struct BmxGroup_gFxRippleDefaultGroup gFxRippleDefaultGroup;           /* 0x0055c220 */
extern ZfxEffect gFxShard;                                                    /* 0x0055c2b0 */
extern ZfxEffect gFxSplash;                                                   /* 0x0055c2e8 */
extern ZfxEffect gFxDust;                                                     /* 0x0055c320 */
extern ZfxEffect gFxPoster;                                                   /* 0x0055c358 */
extern ZfxEffect gFxRipple;                                                   /* 0x0055c390 */
extern ZfxEffect gFxLightning;                                                /* 0x0055c3c8 */
extern struct BmxGroup_gFxSparkCreateDefaultGroup gFxSparkCreateDefaultGroup; /* 0x0055c400 */
extern char sANIM[8];                                                         /* 0x0055c53c */
extern char sFAN3[8];                                                         /* 0x0055c544 */
extern char sFAN2[8];                                                         /* 0x0055c54c */
extern char sFAN1[8];                                                         /* 0x0055c554 */
extern char sPoster[8];                                                       /* 0x0055c55c */
extern char sLightning[12];                                                   /* 0x0055c564 */
extern char sDUST16[8];                                                       /* 0x0055c570 */
extern char sDUST15[8];                                                       /* 0x0055c578 */
extern char sDUST14[8];                                                       /* 0x0055c580 */
extern char sDUST13[8];                                                       /* 0x0055c588 */
extern char sDUST12[8];                                                       /* 0x0055c590 */
extern char sDUST11[8];                                                       /* 0x0055c598 */
extern char sDUST10[8];                                                       /* 0x0055c5a0 */
extern char sDUST09[8];                                                       /* 0x0055c5a8 */
extern char sDUST08[8];                                                       /* 0x0055c5b0 */
extern char sDUST07[8];                                                       /* 0x0055c5b8 */
extern char sDUST06[8];                                                       /* 0x0055c5c0 */
extern char sDUST05[8];                                                       /* 0x0055c5c8 */
extern char sDUST04[8];                                                       /* 0x0055c5d0 */
extern char sDUST03[8];                                                       /* 0x0055c5d8 */
extern char sDUST02[8];                                                       /* 0x0055c5e0 */
extern char sDUST01[8];                                                       /* 0x0055c5e8 */
extern char sSPLASH[8];                                                       /* 0x0055c5f0 */
extern char sRIPPLE[8];                                                       /* 0x0055c5f8 */
extern char sSKID[8];                                                         /* 0x0055c600 */
extern char sSPARK[8];                                                        /* 0x0055c608 */
extern char sSFXCube[12];                                                     /* 0x0055c610 */
extern char sFloorcoll[12];                                                   /* 0x0055c61c */
extern char sGravity[12];                                                     /* 0x0055c628 */
extern char sCZaxisBmxSrcComZgeNgIFNgIFfxC[48];                               /* 0x0055c634 */
extern char sZfxCreateUnsupportedEffect[40];                                  /* 0x0055c664 */
extern uint32_t gNgifBrightness;                                              /* 0x0055c68c */
extern float gStripScale0;                                              /* 0x0055c690, also gNgifScreenW */
extern float gNgifScreenH;                                              /* 0x0055c694 */
extern uint32_t *gZoverOt;                                              /* 0x0055c698, also gNgifOtBase */
extern char sCZaxisBmxSrcComZgeNgIFNgIFpsxC[48];                        /* 0x0055c6a0 */
extern char sDrawPrimPrimNotCodedLINEG4[40];                            /* 0x0055c6d0 */
extern char sDrawPrimPrimNotCodedLINEF4[40];                            /* 0x0055c6f8 */
extern char sDrawPrimPrimNotCodedLINEG3[40];                            /* 0x0055c720 */
extern char sDrawPrimPrimNotCodedLINEF3[40];                            /* 0x0055c748 */
extern char sDrawPrimPrimNotCodedSPRT[40];                              /* 0x0055c770 */
extern char sDrawPrimPrimNotCodedSPRT16[40];                            /* 0x0055c798 */
extern char sDrawPrimPrimNotCodedSPRT8[40];                             /* 0x0055c7c0 */
extern char sDrawPrimPrimNotCodedGT3[36];                               /* 0x0055c7e8 */
extern char sDrawPrimPrimNotCodedG3[36];                                /* 0x0055c80c */
extern char sDrawPrimPrimNotCodedFT3[36];                               /* 0x0055c830 */
extern char sCannotGetSizeOfObject[32];                                 /* 0x0055c854 */
extern char sCanNotGetSizeOfObject[32];                                 /* 0x0055c874 */
extern char sNotCodedForTexturedTris[36];                               /* 0x0055c894 */
extern char sUncodedForPolygonsInScreenSpace[44];                       /* 0x0055c8b8 */
extern char sInvalidPaletteIndex[28];                                   /* 0x0055c8e4 */
extern char sBufferOverflowInPsxprimDrawFontChar[52];                   /* 0x0055c900 */
extern char sRightFoot[12];                                             /* 0x0055c934 */
extern char sRightLowLeg[16];                                           /* 0x0055c940 */
extern char sRightUpLeg[16];                                            /* 0x0055c950 */
extern char sLeftFoot[12];                                              /* 0x0055c960 */
extern char sLeftLowLeg[16];                                            /* 0x0055c96c */
extern char sLeftUpLeg[12];                                             /* 0x0055c97c */
extern char sRightHand[12];                                             /* 0x0055c988 */
extern char sRightLowArm[16];                                           /* 0x0055c994 */
extern char sRightUpArm[16];                                            /* 0x0055c9a4 */
extern char sLeftHand[12];                                              /* 0x0055c9b4 */
extern char sLeftLowArm[16];                                            /* 0x0055c9c0 */
extern char sLeftUpArm[12];                                             /* 0x0055c9d0 */
extern char sHead[8];                                                   /* 0x0055c9dc */
extern char sChest[8];                                                  /* 0x0055c9e4 */
extern char sHips[8];                                                   /* 0x0055c9ec */
extern char sSeatGrip[12];                                              /* 0x0055c9f4 */
extern char sRightGrip[12];                                             /* 0x0055ca00 */
extern char sLeftGrip[12];                                              /* 0x0055ca0c */
extern char sFrontWheel[16];                                            /* 0x0055ca18 */
extern char sBackWheel[12];                                             /* 0x0055ca28 */
extern char sLeftPedal[12];                                             /* 0x0055ca34 */
extern char sRightPedal[16];                                            /* 0x0055ca40 */
extern char sSprocket[12];                                              /* 0x0055ca50 */
extern char sHandlebars[16];                                            /* 0x0055ca5c */
extern char sHipsdeck[12];                                              /* 0x0055ca6c */
extern char sCZaxisBmxSrcComZgeNgIFNgIFzactorC[52];                     /* 0x0055ca78 */
extern char sTestzactorGetAbsolutePartPositionUnknow[60];               /* 0x0055caac */
extern char sTestzactorGetPartLengthUnknownPart[48];                    /* 0x0055cae8 */
extern char sMissingHashFunction[28];                                   /* 0x0055cb18 */
extern char sCZaxisBmxSrcComZgeNgIFNgIFzbspC[52];                       /* 0x0055cb34 */
extern char sBspInit[12];                                               /* 0x0055cb68 */
extern char sBspUpdate[16];                                             /* 0x0055cb74 */
extern char sShutdown[12];                                              /* 0x0055cb84 */
extern char sInitializeScene[20];                                       /* 0x0055cb90 */
extern char sLoadScene[16];                                             /* 0x0055cba4 */
extern char sLoadAndProcessMap[28];                                     /* 0x0055cbb4 */
extern char sAllocAndLoadMap[24];                                       /* 0x0055cbd0 */
extern char sFreeMap[12];                                               /* 0x0055cbe8 */
extern char sSetTreeVisible[20];                                        /* 0x0055cbf4 */
extern char sConvertTreeFromLoad[28];                                   /* 0x0055cc08 */
extern char sRelativeLodZ[20];                                          /* 0x0055cc24 */
extern char sPreprocessScene[20];                                       /* 0x0055cc38 */
extern char sPreprocessTerrainNode[28];                                 /* 0x0055cc4c */
extern char sPreprocessFaceList[28];                                    /* 0x0055cc68 */
extern char sViewerInPosSide[24];                                       /* 0x0055cc84 */
extern char sSndRegister[16];                                           /* 0x0055cc9c */
extern char sSndSetState[16];                                           /* 0x0055ccac */
extern char sSndGetHandle[20];                                          /* 0x0055ccbc */
extern char sDvlitehandle[16];                                          /* 0x0055ccd0 */
extern char sDvlitegetcolor[20];                                        /* 0x0055cce0 */
extern char sDvlitelite[16];                                            /* 0x0055ccf4 */
extern char sDvliteinitialize[20];                                      /* 0x0055cd04 */
extern char sDvliteAttachToProps[28];                                   /* 0x0055cd18 */
extern char sPropActivate[16];                                          /* 0x0055cd34 */
extern char sPropDeactivate[20];                                        /* 0x0055cd44 */
extern char sActivateTree[16];                                          /* 0x0055cd58 */
extern char sDeactivateTree[20];                                        /* 0x0055cd68 */
extern char sInitializeTreeRef[24];                                     /* 0x0055cd7c */
extern char sCreateTreeRef[20];                                         /* 0x0055cd94 */
extern char sDestroyTreeRef[20];                                        /* 0x0055cda8 */
extern char sPropAddTreeRef[24];                                        /* 0x0055cdbc */
extern char sRemoveTreeRef[20];                                         /* 0x0055cdd4 */
extern char sUpdateTreeRef[20];                                         /* 0x0055cde8 */
extern char sAddToTree[16];                                             /* 0x0055cdfc */
extern char sRemoveFromTree[20];                                        /* 0x0055ce0c */
extern char sTotallyWithinTree[24];                                     /* 0x0055ce20 */
extern char sOverlapsTree[16];                                          /* 0x0055ce38 */
extern char sFindTagged[16];                                            /* 0x0055ce48 */
extern char sZbspPropMove[16];                                          /* 0x0055ce58 */
extern char sGetNumDefs[16];                                            /* 0x0055ce68 */
extern char sGetTypeFromHash[24];                                       /* 0x0055ce78 */
extern char sFxCreateTree[20];                                          /* 0x0055ce90 */
extern char sDestroyTree[16];                                           /* 0x0055cea4 */
extern char sDrawTerrain[16];                                           /* 0x0055ceb4 */
extern char sDrawTerrainNode[24];                                       /* 0x0055cec4 */
extern char sDrawFaceList[20];                                          /* 0x0055cedc */
extern char sSetDebug[12];                                              /* 0x0055cef0 */
extern char sDrawLineZ[16];                                             /* 0x0055cefc */
extern char sDrawFaceNormal[20];                                        /* 0x0055cf0c */
extern char sDrawFaceFlat[20];                                          /* 0x0055cf20 */
extern char sShowBoundingBox[24];                                       /* 0x0055cf34 */
extern char sShowEdges[16];                                             /* 0x0055cf4c */
extern char sShowPropVolume[20];                                        /* 0x0055cf5c */
extern char sGetFaceNum[16];                                            /* 0x0055cf70 */
extern char sTrnDumpTree[16];                                           /* 0x0055cf80 */
extern char sShowPartition[20];                                         /* 0x0055cf90 */
extern char sDrawPartition[20];                                         /* 0x0055cfa4 */
extern char sGetTerrainFaces[24];                                       /* 0x0055cfb8 */
extern char sGetTreeObject[20];                                         /* 0x0055cfd0 */
extern char sPointWithinTreeObject[32];                                 /* 0x0055cfe4 */
extern char sGetTreeObjects[20];                                        /* 0x0055d004 */
extern char sGetTerrainNode[20];                                        /* 0x0055d018 */
extern char sGetMaterialInfo[24];                                       /* 0x0055d02c */
extern char sGetMatRefData[20];                                         /* 0x0055d044 */
extern char sGetMatIdx[16];                                             /* 0x0055d058 */
extern char sMatRefList[16];                                            /* 0x0055d068 */
extern char sDoesVertExist[20];                                         /* 0x0055d078 */
extern char sFacesAtSamePos[24];                                        /* 0x0055d08c */
extern char sSceneSaveState[20];                                        /* 0x0055d0a4 */
extern char sSceneRestoreState[24];                                     /* 0x0055d0b8 */
extern char sEdgeListOverflow[24];                                      /* 0x0055d0d0 */
extern char sGetNormalVector[24];                                       /* 0x0055d0e8 */
extern char sGetSideVector[20];                                         /* 0x0055d100 */
extern char sBspcollide[16];                                            /* 0x0055d114 */
extern char sCreateSpanPlanes[24];                                      /* 0x0055d124 */
extern char sAddToFacelist[20];                                         /* 0x0055d13c */
extern char sGetTerrainHeightinf[28];                                   /* 0x0055d150 */
extern char sTestRayCollision[24];                                      /* 0x0055d16c */
extern char sBsptest[12];                                               /* 0x0055d184 */
extern char sTestresetfacecolors[24];                                   /* 0x0055d190 */
extern char sHilightnodefaces[20];                                      /* 0x0055d1a8 */
extern char sSetFaceRgb[16];                                            /* 0x0055d1bc */
extern char sPrintFaceListInf[24];                                      /* 0x0055d1cc */
extern char sTrntestraycollision[24];                                   /* 0x0055d1e4 */
extern char sCreatenodeshit[20];                                        /* 0x0055d1fc */
extern char sAddToNodeslist[20];                                        /* 0x0055d210 */
extern char sAddToFacesList[24];                                        /* 0x0055d224 */
extern char sLineToNodes[16];                                           /* 0x0055d23c */
extern char sLineToFaces[16];                                           /* 0x0055d24c */
extern char sLineplaneintersection[28];                                 /* 0x0055d25c */
extern char sClipRayToObject[24];                                       /* 0x0055d278 */
extern char sClipLineToProps[24];                                       /* 0x0055d290 */
extern char sCliplinetoprop[20];                                        /* 0x0055d2a8 */
extern char sBuildrayboundingbox[24];                                   /* 0x0055d2bc */
extern char sAddpropfaces[16];                                          /* 0x0055d2d4 */
extern char sZbspComputeFacePlaneEq[28];                                /* 0x0055d2e4 */
extern char sTaginitialize[16];                                         /* 0x0055d300 */
extern char sTagregisterprofunc[24];                                    /* 0x0055d310 */
extern char sFacetagregister[20];                                       /* 0x0055d328 */
extern char sFacetagpreprocess[24];                                     /* 0x0055d33c */
extern char sFacetagprocess[20];                                        /* 0x0055d354 */
extern char sFacetagfinddef[20];                                        /* 0x0055d368 */
extern char sFacetagregregister[24];                                    /* 0x0055d37c */
extern char sFacetagpreattachasref[28];                                 /* 0x0055d394 */
extern char sFacetagpreattachsmposasref[32];                            /* 0x0055d3b0 */
extern char sFacetagproswapwindow[28];                                  /* 0x0055d3d0 */
extern char sBufferOverflow[20];                                        /* 0x0055d3ec */
extern char sColspherecreatenodeshit[28];                               /* 0x0055d400 */
extern char sHitnodeslist[16];                                          /* 0x0055d41c */
extern char sTestwithfacelist[20];                                      /* 0x0055d42c */
extern char sFaceboundingbox[20];                                       /* 0x0055d440 */
extern char sZbspColSphereDraw[24];                                     /* 0x0055d454 */
extern char sPropListOverflow[24];                                      /* 0x0055d46c */
extern char sZbspFindFaceByType[116];                                   /* 0x0055d484 */
extern ZcollideBoxEdges gZcollideBoxEdges;                              /* 0x0055d4f8 */
extern struct BmxGroup_gZcollideBarEdgeGroup gZcollideBarEdgeGroup;     /* 0x0055d504 */
extern char sDegenerateCollision[28];                                   /* 0x0055d51c */
extern char sInvalidParameter[24];                                      /* 0x0055d538 */
extern char sCZaxisBmxSrcComZgeNgIFNgIFzcollideC[56];                   /* 0x0055d550 */
extern char sZcolbboxcollision[24];                                     /* 0x0055d588 */
extern char sGetTerrainFace[20];                                        /* 0x0055d5a0 */
extern char sPropHitBox[16];                                            /* 0x0055d5b4 */
extern char sGetPlaneHeight[20];                                        /* 0x0055d5c4 */
extern char sGenerateBoundBox[24];                                      /* 0x0055d5d8 */
extern char sInitPropFacesList[28];                                     /* 0x0055d5f0 */
extern char sGeneratePropBoundingVolume[36];                            /* 0x0055d60c */
extern char sNotCoded[12];                                              /* 0x0055d630 */
extern char sPropMove[12];                                              /* 0x0055d63c */
extern char sZMEMDMMFailedToInitializeDMM[44];                          /* 0x0055d648 */
extern char sCZaxisBmxSrcComZgeNgIFNgIfzmemDMMC[52];                    /* 0x0055d674 */
extern char sZMEMDMMErrorDMMAlreadyInitialized[52];                     /* 0x0055d6a8 */
extern char sZMEMDMMDMMShutdownWithoutBeingInitializ[60];               /* 0x0055d6dc */
extern char sZMEMOutOfDMMHandles[32];                                   /* 0x0055d718 */
extern char sZMEMDMMDMMFailedToAllocate[40];                            /* 0x0055d738 */
extern char sZMEMOutOfRAM[20];                                          /* 0x0055d760 */
extern char sZMEMDMMDMMAllocationSize0[40];                             /* 0x0055d774 */
extern char sZMEMDMMDMMUsedWithoutBeingInitialized[56];                 /* 0x0055d79c */
extern char sZMEMDMMInvalidHandlePassedToZmemDMMDeal[60];               /* 0x0055d7d4 */
extern char sLocked[8];                                                 /* 0x0055d810 */
extern char sZMEMDMMTryingToLockHandle0[40];                            /* 0x0055d818 */
extern char sZMEMDMMTryingToUnlockHandle0[44];                          /* 0x0055d840 */
extern char sZMEMDMMDMMOptimizeFoundUnrecognizedPoin[92];               /* 0x0055d86c */
extern char sF_0055d8c8[176];                                           /* 0x0055d8c8 */
extern ZresOverlaySourceTable gZresOverlaySources;                      /* 0x0055d978 */
extern unsigned char d_0055da28[72];                                    /* 0x0055da28 */
extern char sOutOfSyncMapData[28];                                      /* 0x0055da70 */
extern char sCZaxisBmxSrcComZgeNgIFNgIFzresC[52];                       /* 0x0055da8c */
extern char sMemoryOverflowInZresIncreaseFIXEDDEFSSI[68];               /* 0x0055dac0 */
extern char sOutOfSyncPlayerData[28];                                   /* 0x0055db04 */
extern char sInvalidPlayer[20];                                         /* 0x0055db20 */
extern char sBikeNumberOutOfRange[32];                                  /* 0x0055db34 */
extern char sAssetsMaterialZAL[24];                                     /* 0x0055db54 */
extern char sAssetsReplayZAL[24];                                       /* 0x0055db6c */
extern char sAssetsTablesZAL[24];                                       /* 0x0055db84 */
extern char sCouldNotOpenZAL[24];                                       /* 0x0055db9c */
extern char sZRESFailedToOpenS[32];                                     /* 0x0055dbb4 */
extern char sAssetsMiscDataZAL[24];                                     /* 0x0055dbd4 */
extern char sERRORLoadingOverlayMaterial[40];                           /* 0x0055dbec */
extern char sOverlayD[12];                                              /* 0x0055dc14 */
extern char sZresAddMaterialToLoadListNotFound[48];                     /* 0x0055dc20 */
extern struct BmxGroup_gZrollerRiderNamesGroup gZrollerRiderNamesGroup; /* 0x0055dc50 */
extern BmxType_gCents gCents;                                           /* 0x0055e8b4 */
extern BmxType_gGainCurve gGainCurve;                                   /* 0x0055e97c */
extern struct BmxGroup_gDsVolumeGroup gDsVolumeGroup;                   /* 0x0055eb7c */
extern uint32_t d_0055ed7c[71];                                         /* 0x0055ed7c */
extern char sInitHWnd[16];                                              /* 0x0055ee98 */
extern char sCZaxisBmxSrcPcZgeSoundWinChanC[48];                        /* 0x0055eea8 */
extern char sDirectSoundCreate[24];                                     /* 0x0055eed8 */
extern char sDSOUNDDLL[16];                                             /* 0x0055eef0 */
extern char sFzsndcInitialized[24];                                     /* 0x0055ef00 */
extern char sIFzsndcNextChannelHandleNum[40];                           /* 0x0055ef18 */
extern char sSample[8];                                                 /* 0x0055ef40 */
extern char sFzsndcHandleHChanChannelNumKzsndNumChan[60];               /* 0x0055ef48 */
extern char sFzsndcHandleHChanAllocated[36];                            /* 0x0055ef84 */
extern char sHChanKzsndNumChannelHandles[40];                           /* 0x0055efa8 */
extern char sFileNameTooLong[24];                                       /* 0x0055efd0 */
extern char sCZaxisBmxSrcPcZgeSoundWinLoadC[48];                        /* 0x0055efe8 */
extern char sInvalidSampleFileNumber[32];                               /* 0x0055f018 */
extern char sInvalidAudioAssetFilePlatform[40];                         /* 0x0055f038 */
extern char sWIN[8];                                                    /* 0x0055f060 */
extern char sInvalidAudioAssetFileHeader[40];                           /* 0x0055f068 */
extern char sZSND[8];                                                   /* 0x0055f090 */
extern char sTooManyAudioAssetFiles[32];                                /* 0x0055f098 */
extern char sFileNotLoaded[20];                                         /* 0x0055f0b8 */
extern char sInvalidFileHandle[24];                                     /* 0x0055f0cc */
extern char sInvalidVoiceNumber[28];                                    /* 0x0055f0e4 */
extern char sSampleNumberOutOfRange[32];                                /* 0x0055f100 */
extern char sSampleFileNumberOutOfRange[40];                            /* 0x0055f120 */
extern char sPhraseNumberOutOfRange[32];                                /* 0x0055f148 */
extern char sTrackDefNumberOutOfRange[36];                              /* 0x0055f168 */
extern char sKeymapNumberOutOfRange[32];                                /* 0x0055f18c */
extern BmxType_gAdpcmIndex gAdpcmIndex;                                 /* 0x0055f1ac */
extern struct BmxGroup_gAdpcmStepGroup gAdpcmStepGroup;                 /* 0x0055f1ec */
extern char sFzsndStreamInitialized[28];                                /* 0x0055f2d0 */
extern char sCZaxisBmxSrcPcZgeSoundWinStrmC[48];                        /* 0x0055f2ec */
extern char sFzsndStreamInitialized_0055f31c[28];                       /* 0x0055f31c */
extern char sHResultDSOK[20];                                           /* 0x0055f338 */
extern char sDwLen20[16];                                               /* 0x0055f34c */
extern float gMotionTimeScale;                                          /* 0x0055f35c */
extern int32_t gZactorMax;                                              /* 0x0055f360 */
extern BmxType_gMotionMirrorMap gMotionMirrorMap;                       /* 0x0055f364 */
extern uint32_t gMotionLoadedBank;                                      /* 0x0055f38c */
extern struct BmxGroup_gSkeletonChainGroup gSkeletonChainGroup;         /* 0x0055f390 */
extern char sAlreadyInitialized[20];                                    /* 0x0055ffc4 */
extern char sCNgEngineZgeZdebugC[28];                                   /* 0x0055ffd8 */
extern char s104f104f104f[28];                                          /* 0x0055fff4 */
extern char sS104f104f104f[32];                                         /* 0x00560010 */
extern BmxType_gZdebugStart gZdebugStart;                               /* 0x00560030 */
extern void *p_00560034;                                                /* 0x00560034 */
extern struct BmxGroup_gZProfilePrintAtGroup gZProfilePrintAtGroup;     /* 0x00560038 */
extern void *p_00560058;                                                /* 0x00560058 */
extern void *p_0056005c;                                                /* 0x0056005c */
extern void *p_00560060;                                                /* 0x00560060 */
extern ZrwDbgLineFn gZrwDbgLineHook;                                    /* 0x00560064 */
extern ZrwDbgTriangleFn gZrwDbgTriangleHook;                            /* 0x00560068 */
extern struct BmxGroup_gZrwDbgSphereHookGroup gZrwDbgSphereHookGroup;   /* 0x0056006c */
extern ZDebugPrintFn gZdebugPrint;                                      /* 0x00560074, also gGuiDebugPrint */
extern struct BmxGroup_gZdebugIndentGroup gZdebugIndentGroup;           /* 0x00560078 */
extern int gLineFresh;                                                  /* 0x00560080 */
extern int gAxesFresh;                                                  /* 0x00560084 */
extern int gTriangleFresh;                                              /* 0x00560088 */
extern int gBoxFresh;                                                   /* 0x0056008c */
extern BmxType_gBoxEdges gBoxEdges;                                     /* 0x00560090 */
extern unsigned char d_005600a8[1];                                     /* 0x005600a8 */
extern char sVSSaved02d[15];                                            /* 0x005600a9 */
extern char sFPS02d[12];                                                /* 0x005600b8 */
extern char sFPS02dNumTri05dProc05dVerts05dResAlloc0[76];               /* 0x005600c4 */
extern char sZrwDbgUnkownGeometryType[32];                              /* 0x00560110 */
extern char sCannotCreateRasterChraset[32];                             /* 0x00560130 */
extern char sCNgEngineZgeComZglRwZrwDbgC[36];                           /* 0x00560150 */
extern char sZdebugPrintOutOfBufferSpacePrintingS[48];                  /* 0x00560174 */
extern char sA104f[12];                                                 /* 0x005601a4 */
extern char sS104f[12];                                                 /* 0x005601b0 */
extern char s104f104f104f_005601bc[24];                                 /* 0x005601bc */
extern char sS104f104f104f_005601d4[28];                                /* 0x005601d4 */
extern char s104f104f104f104f[32];                                      /* 0x005601f0 */
extern char sS104f104f104f104f[36];                                     /* 0x00560210 */
extern char sA104f104f104f[28];                                         /* 0x00560234 */
extern char sZrwDbgOutOfBufferSpace[32];                                /* 0x00560250 */
extern char sNotInitialized[16];                                        /* 0x00560270 */
extern char sCNgEngineZgeZinputC[28];                                   /* 0x00560280 */
extern char sAssetDSizeD[20];                                           /* 0x0056029c */
extern char sNumGroupsDAssetsPerGroupD[36];                             /* 0x005602b0 */
extern char sFAILEDToLoadLibraryDirectory[36];                          /* 0x005602d4 */
extern char sCompressionInRefMode[24];                                  /* 0x005602f8 */
extern char sCNgEngineZgeZassetC[28];                                   /* 0x00560310 */
extern char sFAILEDCouldNotLoadHeader[36];                              /* 0x0056032c */
extern char sFAILEDCouldNotAlocateMemoryForZfileLibr[60];               /* 0x00560350 */
extern char sOPENLIBRARY[16];                                           /* 0x0056038c */
extern char sCLOSELIBRARY[16];                                          /* 0x0056039c */
extern char sFailedToFindAssetGroupDAssetNumD[44];                      /* 0x005603ac */
extern char sNULL[8];                                                   /* 0x005603d8 */
extern char sNotFoundS[16];                                             /* 0x005603e0 */
extern char sERRORSizeMismatchInDecompressedData[44];                   /* 0x005603f0 */
extern char sERRORSizeMismatchInDecompressedData_0056041c[44];          /* 0x0056041c */
extern char sUnknownDecompressionMethod[32];                            /* 0x00560448 */
extern char sNotFoundS_00560468[16];                                    /* 0x00560468 */
extern char sFAILEDCouldNotLoadAsset[160];                              /* 0x00560478 */
extern struct BmxGroup_gZobjCameraVtableGroup gZobjCameraVtableGroup;   /* 0x00560518 */
extern ZVec3 gZobjCameraHome;                                           /* 0x00560540 */
extern char sCNgEngineZgeZgeModsZobjCameraC[40];                        /* 0x00560550 */
extern char sUnknownType[16];                                           /* 0x00560578 */
extern char sError[8];                                                  /* 0x00560588 */
extern char sCNgEngineZgeZstringC[28];                                  /* 0x00560590 */
extern char sSTRS[8];                                                   /* 0x005605ac */
extern char sString0xXNotFoundInSetD[36];                               /* 0x005605b4 */
extern char sNotFound[12];                                              /* 0x005605d8 */
extern char sGUIMAINBadMacroExpansion[32];                              /* 0x005605e4 */
extern char sCNgEngineZgeZquatC[24];                                    /* 0x00560604 */
extern char sDivisionBy0[16];                                           /* 0x0056061c */
extern intptr_t gZfileRamDisk;                                          /* 0x0056062c */
extern char sZfileRegisterErrorMessageS[36];                            /* 0x00560630 */
extern char sAnInvalidDeviceIdentifierWasGiven[40];                     /* 0x00560654 */
extern char sFailedToCreateFileHandleAllocator[40];                     /* 0x0056067c */
extern char sFileSystemShouldNotBeInitialized[40];                      /* 0x005606a4 */
extern char sThereAreNoDevicesConnectedToTheSystem[48];                 /* 0x005606cc */
extern char sErrorInitializingPlatformFileSystem[40];                   /* 0x005606fc */
extern char sCNgEngineZgeZfileC[24];                                    /* 0x00560724 */
extern char sErrorWhenTryingToCloseFileThroughDevice[48];               /* 0x0056073c */
extern char sInvalidFileHandle_0056076c[20];                            /* 0x0056076c */
extern char sFileSystemHasNotBeenIntialized[40];                        /* 0x00560780 */
extern char sDeviceListMayBeCorrupt[28];                                /* 0x005607a8 */
extern char sTheDefaultDeviceIsInvalid[32];                             /* 0x005607c4 */
extern char sInvalidParameter_005607e4[20];                             /* 0x005607e4 */
extern char sFailedToIdentifyDeviceFromFileName[44];                    /* 0x005607f8 */
extern char sDeviceNameIsNotValid[28];                                  /* 0x00560824 */
extern char sCouldNotDetermineDeviceForFileName[44];                    /* 0x00560840 */
extern char sInvalidParamters[20];                                      /* 0x0056086c */
extern char sAnInvalidFileNameWasGiven[32];                             /* 0x00560880 */
extern char sErrorOpeningFile[20];                                      /* 0x005608a0 */
extern char sFileSystemShouldBeInitialized[36];                         /* 0x005608b4 */
extern char sMemoryCorruptionWhenBuildingPathName[44];                  /* 0x005608d8 */
extern char sCNgEngineZgeZmemC[24];                                     /* 0x00560904 */
extern char sMemoryAllocationFailed[28];                                /* 0x0056091c */
extern char sCanOnlyResizeLastAllocatedBlock[40];                       /* 0x00560938 */
extern char sCantResize[12];                                            /* 0x00560960 */
extern char sCannotResizeMemoryWhenAllocationsAreReD[68];               /* 0x0056096c */
extern char sUnknownPool[16];                                           /* 0x005609b0 */
extern char sUseZmemFreeToResizeBlockTo0[36];                           /* 0x005609c0 */
extern char sCouldNotInitializeStaticMemoryManager[44];                 /* 0x005609e4 */
extern char sMemoryAllocationFailedOutOfRAM[40];                        /* 0x00560a10 */
extern char sFreeListPoolHasBeenExhausted[36];                          /* 0x00560a38 */
extern char sAnInvalidFreeListWasDestroyed[36];                         /* 0x00560a5c */
extern char sFreeListShouldBeEmptyButFreeBlocksStill[56];               /* 0x00560a80 */
extern char sFreeListIsCorruptHeadOfUsedBlockListIsN[68];               /* 0x00560ab8 */
extern char sInvalidBlockReleasedFromFreeList[40];                      /* 0x00560afc */
extern char sMemoryPoolSystemHasBeenPreviouslyInitia[52];               /* 0x00560b24 */
extern char sMemoryPoolSystemHasBeenPreviouslyShutdo[52];               /* 0x00560b58 */
extern char sAnInvalidMemoryPoolWasDestroyed[40];                       /* 0x00560b8c */
extern char sMemoryPoolChacheHasBeenBeenExhausted[44];                  /* 0x00560bb4 */
extern char sMemoryBeingReleasedIsNotFromThisPool[48];                  /* 0x00560be0 */
extern char sZmemPoolFreeMemoryHasBeenPreviouslyFree[52];               /* 0x00560c10 */
extern char sZpropPropDefHasNoGeometry[36];                             /* 0x00560c44 */
extern char sZpropPropDefHeaderSizeInvalid[40];                         /* 0x00560c68 */
extern char sZpropPropDefHasNoData[32];                                 /* 0x00560c90 */
extern char sCNgEngineZgeZgeModsZpropC[32];                             /* 0x00560cb0 */
extern char sZpropUnableToCreateAnObjBasedOnThePropD[60];               /* 0x00560cd0 */
extern char sZpropNoPropDefCorrespondsToTheProp[44];                    /* 0x00560d0c */
extern char sZpropPropHeaderSizeInvalid[36];                            /* 0x00560d38 */
extern char sZpropPropHasNoData[28];                                    /* 0x00560d5c */
extern char sZObjUsedD[16];                                             /* 0x00560d78 */
extern char sCanTInitializeWith0[24];                                   /* 0x00560d88 */
extern char sCNgEngineZgeZobjC[24];                                     /* 0x00560da0 */
extern char sZOBJErrorFailedToAllocateObj[40];                          /* 0x00560db8 */
extern char sZOBJObjectIsAlreadyAttachedToAnother[48];                  /* 0x00560de0 */
extern char sErrorInHierarchy[20];                                      /* 0x00560e10 */
extern char sNONAMEKzobjTypeD[24];                                      /* 0x00560e24 */
extern char sS_00560e3c[4];                                             /* 0x00560e3c */
extern char sZOBJMemAllocatedDObjectsInUseD[48];                        /* 0x00560e40 */
extern int gZrwMdlPluginOffset;                                         /* 0x00560e70 */
extern uint32_t d_00560e74;                                             /* 0x00560e74 */
extern BmxType_gZglInitModel gZglInitModel;                             /* 0x00560e78 */
extern struct BmxGroup_gZglExitModelGroup gZglExitModelGroup;           /* 0x00560e7c */
extern BmxType_gZrwVtLoadModel gZrwVtLoadModel;                         /* 0x00560e84 */
extern void *p_00560e88;                                                /* 0x00560e88 */
extern BmxType_gZrwVtLoadChunk gZrwVtLoadChunk;                         /* 0x00560e8c */
extern BmxType_gZrwVtInstance gZrwVtInstance;                           /* 0x00560e90 */
extern ZrwModelCall gZrwVtDestroy;                                      /* 0x00560e94, also gZobjModelFree */
extern ZrwModelCall gZrwVtUpdate;                                       /* 0x00560e98 */
extern ZrwModelCall2 gZrwVtSetPos;                                      /* 0x00560e9c */
extern ZrwModelCall2 gZrwVtSetRot;                                      /* 0x00560ea0 */
extern ZrwModelCall2 gZrwVtSetScale;                                    /* 0x00560ea4 */
extern BmxType_gZrwVtPartCount gZrwVtPartCount;                         /* 0x00560ea8 */
extern BmxType_gZrwVtFindNode gZrwVtFindNode;                           /* 0x00560eac */
extern BmxType_gZrwVtNodeToWorld gZrwVtNodeToWorld;                     /* 0x00560eb0 */
extern BmxType_gZrwVtNodeVector gZrwVtNodeVector;                       /* 0x00560eb4 */
extern BmxType_gZrwVtNodeMatrix gZrwVtNodeMatrix;                       /* 0x00560eb8 */
extern void *p_00560ebc;                                                /* 0x00560ebc */
extern ZrwModelKeyFn gZrwVtKey;                                         /* 0x00560ec0 */
extern ZrwModelLookup gZrwVtEntry;                                      /* 0x00560ec4 */
extern BmxType_gZrwVtNodeHashes gZrwVtNodeHashes;                       /* 0x00560ec8 */
extern struct BmxGroup_gZrwVtSkeletonGroup gZrwVtSkeletonGroup;         /* 0x00560ecc */
extern void *p_00560ed4;                                                /* 0x00560ed4 */
extern ZrwModelFlagsFn gZrwVtSetFlags;                                  /* 0x00560ed8, also gZobjSetFlags */
extern BmxType_gZrwVtOwner gZrwVtOwner;                                 /* 0x00560edc */
extern ZbspObjectFrameFn gZbspObjectFrameFn;                            /* 0x00560ee0 */
extern ZrwModelClumpFn gZrwVtClump;                                     /* 0x00560ee4 */
extern BmxType_gFxFindTexture gFxFindTexture;                           /* 0x00560ee8 */
extern struct BmxGroup_gZrwVtMatCountGroup gZrwVtMatCountGroup;         /* 0x00560eec */
extern struct BmxGroup_gZobjSetColorGroup gZobjSetColorGroup;           /* 0x00560ef4 */
extern void *p_00560f18;                                                /* 0x00560f18 */
extern char sCNgEngineZgeComZglRwZrwModelC[40];                         /* 0x00560f1c */
extern char sBodyPartNotMatchedHashX[32];                               /* 0x00560f44 */
extern char sPart0xXNotFoundInModelS[36];                               /* 0x00560f64 */
extern char sMaterialIndexOutOfRange[32];                               /* 0x00560f88 */
extern struct BmxGroup_gZrwOpsSkinnedGroup gZrwOpsSkinnedGroup;         /* 0x00560fa8 */
extern struct BmxGroup_gZrwOpsSimpleGroup gZrwOpsSimpleGroup;           /* 0x00560fd0 */
extern char sCNgEngineZgeZgeModsZobjModelC[36];                         /* 0x00560ff4 */
extern BmxType_gZglInitMaterial gZglInitMaterial;                       /* 0x00561018 */
extern struct BmxGroup_gZglExitMaterialGroup gZglExitMaterialGroup;     /* 0x0056101c */
extern struct BmxGroup_gZrollerFreeHandleGroup gZrollerFreeHandleGroup; /* 0x00561024 */
extern struct BmxGroup_gFxSetRasterGroup gFxSetRasterGroup;             /* 0x0056102c */
extern ZrwMaterialTextureFn
    gZrwMaterialTexture;               /* 0x00561034, also gFxGetRaster, gNgifGetTexture, gZresGetTexture */
extern ZbspMaterialFn gZbspMaterialFn; /* 0x00561038 */
extern ZRwTextureCloneFn gRwTextureClone;                /* 0x0056103c */
extern void *p_00561040;                                 /* 0x00561040 */
extern BmxType_gZrwWorldSetHook gZrwWorldSetHook;        /* 0x00561044 */
extern ZresLoadTextureFn gZresLoadTexture;               /* 0x00561048, also gRwTextureLoad, gZrwShadowLoad */
extern BmxType_gFxTextureByHash gFxTextureByHash;        /* 0x0056104c */
extern void *p_00561050;                                 /* 0x00561050 */
extern char sRwMatProblemLoadingImage[32];               /* 0x00561054 */
extern char sMADFMismatch[16];                           /* 0x00561074 */
extern char sDataTooBig[16];                             /* 0x00561084 */
extern char sCNgEngineZgeComZglRwZrwMatC[36];            /* 0x00561094 */
extern char sNoTextureHasBeenAddedForThisRenderPass[48]; /* 0x005610b8 */
extern char sTooManyTexturesAdded[24];                   /* 0x005610e8 */
extern char sPMatPRwMatNotSet[24];                       /* 0x00561100 */
extern char sZrwMatCreateFromImageErrorLoadingS[44];     /* 0x00561118 */
extern char sZrwMatCreateProceduralErrorCreatingS[44];   /* 0x00561144 */
extern char sZrwMatCreatePaletteFromMemoryError[40];     /* 0x00561170 */
extern char sOutOfRange[16];                             /* 0x00561198 */
extern char sNotLoadedMADF[16];                          /* 0x005611a8 */
extern char sZGLMATObjectsInUseD[28];                    /* 0x005611b8 */
extern void *p_005611d4;                                 /* 0x005611d4 */
extern struct BmxGroup_gErrorCaptionGroup gErrorCaptionGroup;             /* 0x005611d8 */
extern char sRenderWareTmApplicationWarning[36];                          /* 0x005611e0 */
extern char sRenderWareTmApplicationError[36];                            /* 0x00561204 */
extern char sRwAppClass[12];                                              /* 0x00561228 */
extern char sASSERTFuncSFileSLineDWhySDoYouWishToDeb[148];                /* 0x00561234 */
extern char sASSERTFileSLineDWhySDoYouWishToDebugThe[140];                /* 0x005612c8 */
extern char sLuXLuXLuS[20];                                               /* 0x00561354 */
extern char sFullscreen[16];                                              /* 0x00561368 */
extern char sSW[4];                                                       /* 0x00561378 */
extern char sRenderWareApplication[24];                                   /* 0x0056137c */
extern char sARenderWareTmApplicationIsAlreadyRunnin[52];                 /* 0x00561394 */
extern char sDoYouWantToQuitTheGame[32];                                  /* 0x005613c8 */
extern char sPleaseInsertGameCD[24];                                      /* 0x005613e8 */
extern char sRb[4];                                                       /* 0x00561400 */
extern char sBinMovieAcclaimAvi[24];                                      /* 0x00561404 */
extern char sSRCDISK[8];                                                  /* 0x0056141c */
extern char sSoftwareAcclaimEntertainmentDaveMirraFr[68];                 /* 0x00561424 */
extern BmxType_gZrwGraphicLibraryInitialize gZrwGraphicLibraryInitialize; /* 0x00561468 */
extern void *p_0056146c;                                                  /* 0x0056146c */
extern ZrollerRegisterFn
    gZrollerRegister; /* 0x00561470, also gNgifRegister, gTimerRegister, gZrwDbgRegister, gZrwModelRegister */
extern ZrollerUnregisterFn
    gZrollerUnregister;  /* 0x00561474, also gNgifUnregister, gZrwDbgUnregister, gZrwModelUnregister */
extern void *p_00561478; /* 0x00561478 */
extern BmxType_gZdebugFailHandler gZdebugFailHandler; /* 0x0056147c */
extern ZrwTimeFn gTimeGetTime;                        /* 0x00561480, also gFxTime, gZrwTimeGetTime */
extern BmxType_gZrwWorldSetBounds gZrwWorldSetBounds; /* 0x00561484, also gFxSetParam, gZrwSetBounds */
extern ZsystemGetObjectFn
    gZsystemGetObject; /* 0x00561488, also gNgQuery, gNgifQuery, gZpadQuery, gZrollerQuery, gZscnptsQuery */
extern BmxType_gZrwDbgFrameStart gZrwDbgFrameStart; /* 0x0056148c */
extern BmxType_gZrwRenderStateHook
    gZrwRenderStateHook;                    /* 0x00561490, also gZglRenderStateHook, gZrwPrimStateHook */
extern BmxType_gZrwBeginHook gZrwBeginHook; /* 0x00561494 */
extern BmxType_gZrwEndHook gZrwEndHook;     /* 0x00561498 */
extern ZMemAllocFn gRwMalloc;               /* 0x0056149c */
extern ZMemFreeFn gRwFree;                  /* 0x005614a0 */
extern ZMemReallocFn gRwRealloc;            /* 0x005614a4 */
extern void *gZglMemoryFuncs;               /* 0x005614a8 */
extern BmxType_gZglAllocHookNop gZglAllocHookNop;         /* 0x005614ac */
extern char sUnknownParameterD[24];                       /* 0x005614b0 */
extern char sCNgEngineZgeComZglRwZrwZglC[36];             /* 0x005614c8 */
extern char sInvalidCameraSpecified[28];                  /* 0x005614ec */
extern char sCannotCreateWorld[24];                       /* 0x00561508 */
extern char sZrwMdlRegisterPluginFailed[28];              /* 0x00561520 */
extern char sRwMatRegisterPluginFailed[28];               /* 0x0056153c */
extern char sRpPVSPluginAttachFailed[28];                 /* 0x00561558 */
extern char sRpLODAtomicPluginAttachFailed[32];           /* 0x00561574 */
extern char sRpBonePluginAttachFailed[28];                /* 0x00561594 */
extern char sRpAnimPluginAttachFailed[28];                /* 0x005615b0 */
extern char sRpWorldPluginAttachFailed[28];               /* 0x005615cc */
extern char sFailedInCallTo0xX[24];                       /* 0x005615e8 */
extern char sThereIsAPendingRenderWareErrorRendering[68]; /* 0x00561600 */
extern char sInvalidType[16];                             /* 0x00561644 */
extern char sRenderStateStackIsFull[28];                  /* 0x00561654 */
extern char sRenderStateStackIsEmpty[28];                 /* 0x00561670 */
extern char sNotTested[12];                               /* 0x0056168c */
extern char sMovieDonePlaying[24];                        /* 0x00561698 */
extern char sMovieKeyAbort[20];                           /* 0x005616b0 */
extern char sMovieAudioTick[20];                          /* 0x005616c4 */
extern char sCannotInsertNULLValuesIntoAHeapList[44];     /* 0x005616d8 */
extern char sCNgEngineZgeZListC[28];                      /* 0x00561704 */
extern BmxType_gZglInitPrim gZglInitPrim;                 /* 0x00561720 */
extern BmxType_gZglExitPrim gZglExitPrim;                 /* 0x00561724 */
extern ZrwDrawFn gZrwDrawPolys;                           /* 0x00561728, also gFxDrawLines */
extern ZrwDrawFn gZrwDrawTriangle; /* 0x0056172c, also gFxDrawTriangles, gZProfileDrawTriangles */
extern ZrwDrawFn gZrwDrawSphere;   /* 0x00561730 */
extern void *p_00561734;           /* 0x00561734 */
extern ZrwDrawFn gNgifDrawQuads;   /* 0x00561738, also gFxDrawQuads, gNgDrawQuads, gZProfileDrawQuads */
extern char sCNgEngineZgeComZglRwZrwPrimC[36];                            /* 0x0056173c */
extern char sZtaginfExtraNodesInTree[36];                                 /* 0x00561760 */
extern char sZtaginfTaggingInfoFaceStructureMismatch[48];                 /* 0x00561784 */
extern char sChunkSyncError[20];                                          /* 0x005617b4 */
extern char sTagDataNotFound[20];                                         /* 0x005617c8 */
extern char sCNgEngineZgeZgeModsZtaginfC[36];                             /* 0x005617dc */
extern char sFUCKEDTAGINFONORMAL[24];                                     /* 0x00561800 */
extern uint32_t gZrwModelColor1;                                          /* 0x00561818 */
extern uint32_t gZrwModelColor2;                                          /* 0x0056181c */
extern uint32_t d_00561820;                                               /* 0x00561820 */
extern char sCNgEngineZgeComZglRwZrwModelRenderC[44];                     /* 0x00561824 */
extern char sUnkown[8];                                                   /* 0x00561850 */
extern struct BmxGroup_gZobjLightOpsGroup gZobjLightOpsGroup;             /* 0x00561858 */
extern char sCNgEngineZgeZgeModsZobjLightC[36];                           /* 0x0056187c */
extern struct BmxGroup_p_005618a0Group p_005618a0Group;                   /* 0x005618a0 */
extern struct BmxGroup_p_005618b0Group p_005618b0Group;                   /* 0x005618b0 */
extern struct BmxGroup_d_005618d4Group d_005618d4Group;                   /* 0x005618d4 */
extern struct BmxGroup_gSceneModelHandlersGroup gSceneModelHandlersGroup; /* 0x005618dc */
extern struct BmxGroup_gSceneAnimHandlersGroup gSceneAnimHandlersGroup;   /* 0x005618e4 */
extern struct BmxGroup_d_00561900Group d_00561900Group;                   /* 0x00561900 */
extern struct BmxGroup_d_00561908Group d_00561908Group;                   /* 0x00561908 */
extern struct BmxGroup_d_00561918Group d_00561918Group;                   /* 0x00561918 */
extern struct BmxGroup_d_00561920Group d_00561920Group;                   /* 0x00561920 */
extern char sMODEL[8];                                                    /* 0x00561928 */
extern char sSCENE[8];                                                    /* 0x00561930 */
extern char sZAL[4];                                                      /* 0x00561938 */
extern char sIFF[4];                                                      /* 0x0056193c */
extern char sNAD[4];                                                      /* 0x00561940 */
extern char sZMD[4];                                                      /* 0x00561944 */
extern char sERRORUnknownTokenS[32];                                      /* 0x00561948 */
extern char sERRORMissingData[28];                                        /* 0x00561968 */
extern char s_00561984[4];                                                /* 0x00561984 */
extern char sSUBTYPE[8];                                                  /* 0x00561988 */
extern char sTYPE[8];                                                     /* 0x00561990 */
extern char sFileNameIsTooLong[24];                                       /* 0x00561998 */
extern char sFILE[8];                                                     /* 0x005619b0 */
extern char sObjectNameIsTooLong[28];                                     /* 0x005619b8 */
extern char sCNgEngineZgeZSceneC[28];                                     /* 0x005619d4 */
extern char sNAME[8];                                                     /* 0x005619f0 */
extern char s_005619f8[4];                                                /* 0x005619f8 */
extern char sDatabaseNotFoundS[24];                                       /* 0x005619fc */
extern char sSCENECOMMON[16];                                             /* 0x00561a14 */
extern char sNoSceneIsOpen[20];                                           /* 0x00561a24 */
extern char sCouldnTOpenRamdiskFile[28];                                  /* 0x00561a38 */
extern char sSceneComponentFileNotFoundS[36];                             /* 0x00561a54 */
extern char sZrwWorldFailedToLoadScenePoint[40];                          /* 0x00561a78 */
extern char sZrwWorldFailedToLoadSceneTaggingInfo[48];                    /* 0x00561aa0 */
extern char sUnknownSceneEzsceneObjTypeD[32];                             /* 0x00561ad0 */
extern char sNULLAnimLibID[16];                                           /* 0x00561af0 */
extern char sUncoded[8];                                                  /* 0x00561b00 */
extern char sZSCENEWarningObjectWasNotInSceneTypeDId[72];                 /* 0x00561b08 */
extern char sAnimLibNotFoundS[24];                                        /* 0x00561b50 */
extern char sERRORZSCENELoadingDatabase[36];                              /* 0x00561b68 */
extern char sSceneTxt[12];                                                /* 0x00561b8c */
extern char sErrorLoadingS[20];                                           /* 0x00561b98 */
extern char sZSCENECouldNotLoadS[28];                                     /* 0x00561bac */
extern char sSceneNotFoundS[20];                                          /* 0x00561bc8 */
extern char sThereIsAlreadyASceneOpen[32];                                /* 0x00561bdc */
extern char sFailedToCreateObjectForS[32];                                /* 0x00561bfc */
extern char sModelNotFoundS[20];                                          /* 0x00561c1c */
extern char sUnknownModelExtensionS[28];                                  /* 0x00561c30 */
extern char sModelNotInSceneS[24];                                        /* 0x00561c4c */
extern char sAnimNotFoundS[20];                                           /* 0x00561c64 */
extern char sUnknownExtension[20];                                        /* 0x00561c78 */
extern char sAnimNotInSceneS[24];                                         /* 0x00561c8c */
extern char s_00561ca4[4];                                                /* 0x00561ca4 */
extern char sS_00561ca8[4];                                               /* 0x00561ca8 */
extern char sWARNINGZSCENEMotion0xXNotFoundInAnimati[68];                 /* 0x00561cac */
extern char sTableNotBigEnoughTabSizeDNumMotionsD[48];                    /* 0x00561cf0 */
extern char sZscnptsPointDataSizeMismatch[36];                            /* 0x00561d20 */
extern char sZscnptsPointHasNoData[32];                                   /* 0x00561d44 */
extern char sCNgEngineZgeZgeModsZscnptsC[36];                             /* 0x00561d64 */
extern ZrwCamInitFn gZrwCamInit;                                          /* 0x00561d88 */
extern void *p_00561d8c;                                                  /* 0x00561d8c */
extern ZrwCamCreateFn gZrwCamCreate;                                      /* 0x00561d90 */
extern ZrwCamDestroyFn gZrwCamDestroy;                                    /* 0x00561d94 */
extern struct BmxGroup_gZrwCamSetFovGroup gZrwCamSetFovGroup;             /* 0x00561d98 */
extern struct BmxGroup_gZrwCamSetViewGroup gZrwCamSetViewGroup;           /* 0x00561da0 */
extern void *p_00561dac;                                                  /* 0x00561dac */
extern BmxType_gZrwDbgGetCamera gZrwDbgGetCamera;       /* 0x00561db0, also gFxGetCamera, gNgifGetCamera */
extern BmxType_gZrwDbgCameraObject gZrwDbgCameraObject; /* 0x00561db4, also gFxCameraObject */
extern struct BmxGroup_gNgifSetViewportGroup gNgifSetViewportGroup;           /* 0x00561db8 */
extern BmxType_gNgifSetViewOffset gNgifSetViewOffset;                         /* 0x00561dc0 */
extern char sErrorCreatingCameraZBuffer[32];                                  /* 0x00561dc4 */
extern char sErrorCreatingCameraRaster[32];                                   /* 0x00561de4 */
extern char sCNgEngineZgeComZglRwZrwCamC[36];                                 /* 0x00561e04 */
extern char sCanTSetScreenResoltionIfCamerasExist[44];                        /* 0x00561e28 */
extern char sRwCamNoCamera[20];                                               /* 0x00561e54 */
extern struct BmxGroup_gZpadKeyMapFirstGroup gZpadKeyMapFirstGroup;           /* 0x00561e68 */
extern struct BmxGroup_gZpadKeyMapOtherGroup gZpadKeyMapOtherGroup;           /* 0x00561e88 */
extern struct BmxGroup_gZpadDefaultButtonMapGroup gZpadDefaultButtonMapGroup; /* 0x00561ea8 */
extern struct BmxGroup_p_00561ec8Group p_00561ec8Group;                       /* 0x00561ec8 */
extern struct BmxGroup_p_00561ed8Group p_00561ed8Group;                       /* 0x00561ed8 */
extern struct BmxGroup_p_00561ef8Group p_00561ef8Group;                       /* 0x00561ef8 */
extern struct BmxGroup_p_00561f04Group p_00561f04Group;                       /* 0x00561f04 */
extern struct BmxGroup_p_00561f10Group p_00561f10Group;                       /* 0x00561f10 */
extern struct BmxGroup_gZpadProfilesGroup gZpadProfilesGroup;                 /* 0x00561f2c */
extern char sMicrosoftSideWinderAutoDetect[36];                               /* 0x00561f38 */
extern char sMicrosoftSideWinderGamePad[32];                                  /* 0x00561f5c */
extern char sGravisGamePadPro[20];                                            /* 0x00561f7c */
extern char sGravisGrIP[12];                                                  /* 0x00561f90 */
extern char sDirectInputCreateFailed[28];                                     /* 0x00561f9c */
extern char sCNgEngineZgePcPcinputC[32];                                      /* 0x00561fb8 */
extern char sPCINPUTCouldNotCreateController[40];                             /* 0x00561fd8 */
extern char sCouldNotCreateAnInputDevice[36];                                 /* 0x00562000 */
extern char sSetCooperativeLevelFailed[32];                                   /* 0x00562024 */
extern char sSetDataFormatFailed[28];                                         /* 0x00562044 */
extern BmxType_gZglInitCol gZglInitCol;                                       /* 0x00562060 */
extern BmxType_gZglExitCol gZglExitCol;                                       /* 0x00562064 */
extern BmxType_gZrwColLineFn gZrwColLineFn;                                   /* 0x00562068 */
extern BmxType_gZrwColSphereFn gZrwColSphereFn;                               /* 0x0056206c */
extern char sCNgEngineZgeComZglRwZrwColC[40];                                 /* 0x00562070 */
extern struct BmxGroup_gZrwWorldEffectOpsGroup gZrwWorldEffectOpsGroup;       /* 0x00562098 */
extern char sCNgEngineZgeZgeModsZobjPEmitC[36];                               /* 0x005620bc */
extern ZrwWorldLoadFn gZrwWorldLoad;                                          /* 0x005620e0 */
extern ZrwWorldCloseFn gZrwWorldClose;                                        /* 0x005620e4 */
extern ZrwWorldFindFn gZrwWorldFind;                                          /* 0x005620e8 */
extern struct BmxGroup_gFxLoadTextureGroup gFxLoadTextureGroup;               /* 0x005620ec */
extern BmxType_gZrwLightKinds gZrwLightKinds;                                 /* 0x0056210c */
extern const void *gZrwWorldPathArgs;                                         /* 0x00562128 */
extern const char *gZrwPvsKeys;                                               /* 0x0056212c */
extern BmxType_gZrwPathKey gZrwPathKey;                                       /* 0x00562130 */
extern char sERROR[8];                                                        /* 0x00562138 */
extern char sRpNALIGHTTYPE[16];                                               /* 0x00562140 */
extern char sRpLIGHTSPOTSOFT[16];                                             /* 0x00562150 */
extern char sRpLIGHTSPOT[12];                                                 /* 0x00562160 */
extern char sRpLIGHTPOINT[16];                                                /* 0x0056216c */
extern char sRpLIGHTAMBIENT[16];                                              /* 0x0056217c */
extern char sRpLIGHTDIRECTIONAL[20];                                          /* 0x0056218c */
extern char sNumberOfClumpsD[24];                                             /* 0x005621a0 */
extern char sCLUMPSPropsEtc[24];                                              /* 0x005621b8 */
extern char sNumberOfVerticesD[24];                                           /* 0x005621d0 */
extern char sNumberOfPolygonsD[24];                                           /* 0x005621e8 */
extern char sSTATICGEOM[16];                                                  /* 0x00562200 */
extern char s3dS[12];                                                         /* 0x00562210 */
extern char sMATERIALS[16];                                                   /* 0x0056221c */
extern char sLIGHTS[12];                                                      /* 0x0056222c */
extern char sSCENEINFOFORS[24];                                               /* 0x00562238 */
extern char s_00562250[64];                                                   /* 0x00562250 */
extern char sConeAngleF[16];                                                  /* 0x00562290 */
extern char sRadiusF[12];                                                     /* 0x005622a0 */
extern char sLightWorld[12];                                                  /* 0x005622ac */
extern char sLightAtomics[16];                                                /* 0x005622b8 */
extern char sRFGFBF[16];                                                      /* 0x005622c8 */
extern char sLight3dS[16];                                                    /* 0x005622d8 */
extern char sChunkOutOfSync[20];                                              /* 0x005622e8 */
extern char sCNgEngineZgeComZglRwZrwWorldC[40];                               /* 0x005622fc */
extern char sCouldnTOpenS[20];                                                /* 0x00562324 */
extern char sZrwWorldHookRpMaterialsFailedToMatchRpM[56];                     /* 0x00562338 */
extern char sPercentCompleteF[24];                                            /* 0x00562370 */
extern char sThereIsAlreadyAWorldOpen[32];                                    /* 0x00562388 */
extern char sZRWWORLDNoPVSData[24];                                           /* 0x005623a8 */
extern ZVec3 gZrwDefaultGravity;                                              /* 0x005623c0 */
extern BmxType_gZrwDefaultBounce gZrwDefaultBounce;                           /* 0x005623d0 */
extern ZfxModifier gFxGravity;                                                /* 0x005623d8 */
extern ZfxModifier gFxFloorColl;                                              /* 0x005623e0 */
extern int gZrwMatPluginOffset;                                               /* 0x005623e8 */
extern BmxType_gZglInitTexture gZglInitTexture;                               /* 0x005623f0 */
extern struct BmxGroup_gZglExitTextureGroup gZglExitTextureGroup;             /* 0x005623f4 */
extern BmxType_gZrwTextureLoad gZrwTextureLoad;                               /* 0x005623fc */
extern BmxType_gZrwWorldTexDrop gZrwWorldTexDrop; /* 0x00562400, also gZrwTextureDrop, gZrwTextureRelease */
extern ZresTextureSizeFn gZresTextureWidth;       /* 0x00562404, also gNgifTextureWidth */
extern struct BmxGroup_gZresTextureHeightGroup gZresTextureHeightGroup; /* 0x00562408 */
extern BmxType_gZrwWorldTexName gZrwWorldTexName;                       /* 0x00562410 */
extern struct BmxGroup_gZrwTextureHashGroup gZrwTextureHashGroup;       /* 0x00562414 */
extern struct BmxGroup_gZrwTextureRetainGroup gZrwTextureRetainGroup;   /* 0x00562420 */
extern void *p_00562428;                                                /* 0x00562428 */
extern char sFormatSWidthDHeightDStrideDDepthD[56];                     /* 0x0056242c */
extern char sRwRASTERFORMATPAL4[20];                                    /* 0x00562464 */
extern char sRwRASTERFORMATMIPMAP[24];                                  /* 0x00562478 */
extern char sRwRASTERFORMATMASK[20];                                    /* 0x00562490 */
extern char sUnknown[8];                                                /* 0x005624a4 */
extern char sRwRASTERFORMATPAL8[20];                                    /* 0x005624ac */
extern char sRwRASTERFORMAT32[20];                                      /* 0x005624c0 */
extern char sRwRASTERFORMAT555[20];                                     /* 0x005624d4 */
extern char sRwRASTERFORMATPIXELFORMATMASK[32];                         /* 0x005624e8 */
extern char sRwRASTERFORMAT888[20];                                     /* 0x00562508 */
extern char sRwRASTERFORMAT24[20];                                      /* 0x0056251c */
extern char sRwRASTERFORMATLUM8[20];                                    /* 0x00562530 */
extern char sRwRASTERFORMAT4444[20];                                    /* 0x00562544 */
extern char sRwRASTERFORMAT565[20];                                     /* 0x00562558 */
extern char sRwRASTERFORMATDEFAULT[24];                                 /* 0x0056256c */
extern char sRwRASTERFORMAT1555[20];                                    /* 0x00562584 */
extern char sFailedToCreateTexture[28];                                 /* 0x00562598 */
extern BmxType_kZrwExtBmp kZrwExtBmp;                                   /* 0x005625b4 */
extern BmxType_kZrwExtPng kZrwExtPng;                                   /* 0x005625b8 */
extern char sBadEzglImageTypes[20];                                     /* 0x005625bc */
extern char sErrorInParameters[20];                                     /* 0x005625d0 */
extern char sCNgEngineZgeComZglRwZrwTxrC[36];                           /* 0x005625e4 */
extern char sZrwTxrLoadPalettedImageFromMemoryErrorI[56];               /* 0x00562608 */
extern char sZrwTxrLoadPalettedImageFromMemoryError[44];                /* 0x00562640 */
extern char sUnableToLockRaster[24];                                    /* 0x0056266c */
extern char sUnableToLockRasterPalette[32];                             /* 0x00562684 */
extern char sZrwTxrCreateProceduralTextureErrorInPar[52];               /* 0x005626a4 */
extern char sZrwTxrCreateProceduralTextureError[40];                    /* 0x005626d8 */
extern char sZrwTxrErrorLoadingTextureS[36];                            /* 0x00562700 */
extern char sLoadBufferIsTooSmall[28];                                  /* 0x00562724 */
extern char sNameSWidthDHeightDBPPD[36];                                /* 0x00562740 */
extern BmxType_kZrwFormatDds kZrwFormatDds;                             /* 0x00562764 */
extern BmxType_kZrwFormatPng kZrwFormatPng;                             /* 0x00562768 */
extern BmxType_kZrwFormatBmp kZrwFormatBmp;                             /* 0x0056276c */
extern char sTextureReferenceCountHasBecomeNegative[48];                /* 0x00562770 */
extern char sTextureIsnTPalettedS[28];                                  /* 0x005627a0 */
extern char sUnknownMode[16];                                           /* 0x005627bc */
extern char sZGLTEXTUREObjectsInUseD[32];                               /* 0x005627cc */
extern char sLoadBufferHasAlreadyBeenPrepared[40];                      /* 0x005627ec */
extern char sLoadBufferHasAlreadyBeenDestroyed[40];                     /* 0x00562814 */
extern char sCannotAllocate[16];                                        /* 0x0056283c */
extern char sCNgEngineZgeZanimC[24];                                    /* 0x0056284c */
extern char sCanTAllocateNadname[24];                                   /* 0x00562864 */
extern char sDEFISizeMismatch[20];                                      /* 0x0056287c */
extern char sCanTAllocateNamelist[24];                                  /* 0x00562890 */
extern char sCanTAllocateNadRots[24];                                   /* 0x005628a8 */
extern char sCanTAllocateRotChannel[28];                                /* 0x005628c0 */
extern char sCanTAllocateNadTrans[24];                                  /* 0x005628dc */
extern char sCanTAllocateTransChannel[32];                              /* 0x005628f4 */
extern char sHeyJackoFloatTimeIsTooBigResetIt[48];                      /* 0x00562914 */
extern char sNonNonblendedAnimation[28];                                /* 0x00562944 */
extern char sEmptyStreamList[20];                                       /* 0x00562960 */
extern char sCNgEngineZgeZiffC[24];                                     /* 0x00562974 */
extern char sZIFFBadIFFFile[20];                                        /* 0x0056298c */
extern char sZIFFNonMatchingChunkIDCCCCCCCC[52];                        /* 0x005629a0 */
extern char sZIFFChunksStillOpen[24];                                   /* 0x005629d4 */
extern char sUnregisteredHeaderCCCC[32];                                /* 0x005629ec */
extern char sCCCC[12];                                                  /* 0x00562a0c */
extern char sZIFFContentsFor0xX[28];                                    /* 0x00562a18 */
extern char sZIFFNotReRegisteringCCCC[40];                              /* 0x00562a34 */
extern char sZIFFChangingFunctionForCCCC[40];                           /* 0x00562a5c */
extern char sNotAtStartOfChunk[24];                                     /* 0x00562a84 */
extern char sZIFFZiffChunkReadDataClampingSize[40];                     /* 0x00562a9c */
extern char sPassedTheEndOfChunk[24];                                   /* 0x00562ac4 */
extern char sAllDataRead[16];                                           /* 0x00562adc */
extern char sTryingToRead0Bytes[24];                                    /* 0x00562aec */
extern char sDataLengthNotWordSizeForHeaderCCCC[48];                    /* 0x00562b04 */
extern char sSeekedPassedTheStart[24];                                  /* 0x00562b34 */
extern char sSeekedPassedTheEnd[24];                                    /* 0x00562b4c */
extern char sInvalidSeekMode[20];                                       /* 0x00562b64 */
extern char sNotAtTheStartOfTheChunk[32];                               /* 0x00562b78 */
extern char sNestedTooDeep[16];                                         /* 0x00562b98 */
extern char sTooManyCloses[16];                                         /* 0x00562ba8 */
extern char sShadowMap[12];                                             /* 0x00562bb8 */
extern char sAssetsShadowMapBmp[24];                                    /* 0x00562bc4 */
extern char sCNgEngineZgeComZglRwZrwShadowC[40];                        /* 0x00562bdc */
extern char s123456789012345678[20];                                    /* 0x00562c04 */
extern char sZCOMPRESUnsupportedCompressionMethod[44];                  /* 0x00562c18 */
extern char sFaultyVValueOnEOFD[28];                                    /* 0x00562c44 */
extern char sZCOMPRESDecompressionFailed[36];                           /* 0x00562c60 */
extern char sIllegalStartingByte[24];                                   /* 0x00562c84 */
extern char sZCOMPRESUnknownDecompressionMethod[40];                    /* 0x00562c9c */
extern BmxType_gZrwLodDefaults gZrwLodDefaults;                         /* 0x00562cc4 */
extern char sZrwModelSetRwInfoFailedToMatchRpMateria[52];               /* 0x00562cd4 */
extern char sFDVD[16];                                                  /* 0x00562d08 */
extern char sRpGEOMETRYTRISTRIP[20];                                    /* 0x00562d18 */
extern char sRpGEOMETRYLIGHT[20];                                       /* 0x00562d2c */
extern char sRpGEOMETRYNORMALS[20];                                     /* 0x00562d40 */
extern char sRpGEOMETRYPRELIT[20];                                      /* 0x00562d54 */
extern char sRpGEOMETRYTEXTURED[20];                                    /* 0x00562d68 */
extern char sFlagsAtomic0[20];                                          /* 0x00562d7c */
extern char sNumVerticesD[20];                                          /* 0x00562d90 */
extern char sNumTrianglesD[20];                                         /* 0x00562da4 */
extern char s_00562db8[4];                                              /* 0x00562db8 */
extern char sNumAtomicsD[20];                                           /* 0x00562dbc */
extern char s0xX[8];                                                    /* 0x00562dd0 */
extern char s0xXS[12];                                                  /* 0x00562dd8 */
extern char sHIERARCHY[16];                                             /* 0x00562de4 */
extern char sMATERIALS_00562df4[16];                                    /* 0x00562df4 */
extern char sZrwModelCreateInstanceNotInstancingSkin[68];               /* 0x00562e04 */
extern char sClumpFailedToLoad[24];                                     /* 0x00562e48 */
extern char sClumpFailedToLoad_00562e60[24];                            /* 0x00562e60 */
extern char sCNgEngineZgeComZglRwZrwModelCreateC[44];                   /* 0x00562e78 */
extern char sIgnoringExtraLODInS[28];                                   /* 0x00562ea4 */
extern char sMissingModelDefBlock[24];                                  /* 0x00562ec0 */
extern char sLoadingModelS[20];                                         /* 0x00562ed8 */
extern char s_00562eec[52];                                             /* 0x00562eec */
extern char sCouldnTOpenMODL[20];                                       /* 0x00562f20 */
extern char sNotAMODL[12];                                              /* 0x00562f34 */
extern char sZGLMODELObjectsInUseD[32];                                 /* 0x00562f40 */
extern char sInfoForModelS[20];                                         /* 0x00562f60 */
extern char sCNgEngineZgeComZglRwZrwModelGeomC[44];                     /* 0x00562f74 */
extern char sCouldNotLockModelGeometryForS[40];                         /* 0x00562fa0 */
extern char sCouldNotUnlockModelGeometryForS[48];                       /* 0x00562fc8 */
extern struct BmxGroup_gZglFileDeviceRwGroup gZglFileDeviceRwGroup;     /* 0x00562ff8 */
extern BmxType_gZrwOpenModes gZrwOpenModes;                             /* 0x00563038 */
extern char sRB[4];                                                     /* 0x00563048 */
extern char sWb[4];                                                     /* 0x0056304c */
extern BmxType_gZrwDeviceName gZrwDeviceName;                           /* 0x00563050 */
extern char sCNgEngineZgeComZglRwZrwFileC[36];                          /* 0x00563058 */
extern char sInvalidAttributesForRenderWareFileName[44];                /* 0x0056307c */
extern char sInvalidFileNameGiven[24];                                  /* 0x005630a8 */
extern char sMustSpecifyAtLeastEitherAReturnValueOrA[60];               /* 0x005630c0 */
extern char sFunctionNotPermittedOnThisDevice[40];                      /* 0x005630fc */
extern char sFunctionNotImplementedForThisDevice[44];                   /* 0x00563124 */
extern char sRwfopenFailed[20];                                         /* 0x00563150 */
extern char sInvalidParameters[20];                                     /* 0x00563164 */
extern char sRwfcloseFailed[20];                                        /* 0x00563178 */
extern char sRwfreadFailed[20];                                         /* 0x0056318c */
extern char sRwfwriteFailed[20];                                        /* 0x005631a0 */
extern char sRwfseekFailed[20];                                         /* 0x005631b4 */
extern char sInvalidPointerToFileSize[32];                              /* 0x005631c8 */
extern char sFileSystemHasAlreadyBeenIntialized[44];                    /* 0x005631e8 */
extern char sFileSystemHasAlreadyBeenShutDown[44];                      /* 0x00563214 */
extern struct BmxGroup_gZglFileDeviceRamGroup gZglFileDeviceRamGroup;   /* 0x00563240 */
extern char sCNgEngineZgeZgeModsZramdiskC[36];                          /* 0x00563280 */
extern char sInvalidRAMDiskFileNameAttributes[40];                      /* 0x005632a4 */
extern char sInvalidFileNameCreatedForRAMDisk[40];                      /* 0x005632cc */
extern char sRd0010x10x[20];                                            /* 0x005632f4 */
extern char sFunctionNotSupportedOnThisDevice[40];                      /* 0x00563308 */
extern char sErrorAllocatingRAMDiskFileHandle[40];                      /* 0x00563330 */
extern char sInvalidFileNameGivenForRAMDisk[40];                        /* 0x00563358 */
extern char sInvalidFileName[20];                                       /* 0x00563380 */
extern char sInvalidRAMDiskFileHandle[32];                              /* 0x00563394 */
extern char sFileAccessViolation[24];                                   /* 0x005633b4 */
extern char sSeekPastEndOfFile[24];                                     /* 0x005633cc */
extern char sInvalidPointerForFileSize[36];                             /* 0x005633e4 */
extern struct BmxGroup_gZrwLightTypesGroup gZrwLightTypesGroup;         /* 0x00563408 */
extern BmxType_gZglInitLight gZglInitLight;                             /* 0x00563420 */
extern BmxType_gZglExitLight gZglExitLight;                             /* 0x00563424 */
extern ZrwLightCreateFn gZrwLightCreate;                                /* 0x00563428 */
extern ZrwLightDestroyFn gZrwLightDestroy;                              /* 0x0056342c */
extern ZrwLightColorFn gZrwLightColor;                                  /* 0x00563430 */
extern ZrwLightFlagsFn gZrwLightFlags;                                  /* 0x00563434 */
extern ZrwLightParamFn gZrwLightSetParam;                               /* 0x00563438 */
extern ZrwLightDirectionFn gZrwLightDirection;                          /* 0x0056343c */
extern void *p_00563440;                                                /* 0x00563440 */
extern void *p_00563444;                                                /* 0x00563444 */
extern char sCNgEngineZgeComZglRwZrwLightC[40];                         /* 0x00563448 */
extern char sCNgEngineZgeZgeC[24];                                      /* 0x00563470 */
extern char sZGLDUMP[12];                                               /* 0x00563488 */
extern char sZGEDUMP[12];                                               /* 0x00563494 */
extern char s_005634a0[60];                                             /* 0x005634a0 */
extern char sMovieInit[16];                                             /* 0x005634dc */
extern char sMovieCloseAudio[24];                                       /* 0x005634ec */
extern char sMovieShutdown[20];                                         /* 0x00563504 */
extern char sMovieFillAudio[20];                                        /* 0x00563518 */
extern char sMovieClose[16];                                            /* 0x0056352c */
extern char sMovieEndOfMovie[24];                                       /* 0x0056353c */
extern char sPanorama[12];                                              /* 0x00563554 */
extern char sOpenedFileS[28];                                           /* 0x00563560 */
extern char sClosedFile[24];                                            /* 0x0056357c */
extern char sSymbolNotFoundS[24];                                       /* 0x00563594 */
extern char sD_005635ac[4];                                             /* 0x005635ac */
extern char sX[4];                                                      /* 0x005635b0 */
extern char s_005635b4[4];                                              /* 0x005635b4 */
extern char s0x[4];                                                     /* 0x005635b8 */
extern char s0X[4];                                                     /* 0x005635bc */
extern char sFileSLineD[20];                                            /* 0x005635c0 */
extern char sIncludedFrom[16];                                          /* 0x005635d4 */
extern char sCanTOpenIncludeFileS[32];                                  /* 0x005635e4 */
extern char sERRORMaximumIncludeNestingReached[44];                     /* 0x00563604 */
extern char sDEFINE[8];                                                 /* 0x00563630 */
extern char sEndif[8];                                                  /* 0x00563638 */
extern char sIfndef[8];                                                 /* 0x00563640 */
extern char sINCLUDEPATH[16];                                           /* 0x00563648 */
extern char sINCLUDE[12];                                               /* 0x00563658 */
extern char s_00563664[4];                                              /* 0x00563664 */
extern char s_00563668[4];                                              /* 0x00563668 */
extern char s_0056366c[4];                                              /* 0x0056366c */
extern char s_00563670[4];                                              /* 0x00563670 */
extern char s_00563674[8];                                              /* 0x00563674 */
extern char sOutOfSymbolSpace[24];                                      /* 0x0056367c */
extern char sReplacingSymbolS[24];                                      /* 0x00563694 */
extern char sF_005636ac[4];                                             /* 0x005636ac */
extern char sZTOKENAssertFailedFoundSExpectingS[52];                    /* 0x005636b0 */
extern char sSetCapitalizeFALSE[32];                                    /* 0x005636e4 */
extern char sSetCapitalizeTRUE[32];                                     /* 0x00563704 */
extern char sCNgEngineZgeZgeModsZFontC[32];                             /* 0x00563724 */
extern char sBadFontGlyphNumber[24];                                    /* 0x00563744 */
extern char sBadFontVersion[20];                                        /* 0x0056375c */
extern BmxType_kZrwBBoxFaces kZrwBBoxFaces;                             /* 0x00563770 */
extern char sPBBoxOutPBBoxIn[20];                                       /* 0x005637d0 */
extern char sCNgEngineZgeZutilC[24];                                    /* 0x005637e4 */
extern char s_005637fc[24];                                             /* 0x005637fc */
extern char sXXXXXXXXXXXXXXXXXXXX[24];                                  /* 0x00563814 */
extern char s7dXXXXXXXXXXX[24];                                         /* 0x0056382c */
extern char s_00563844[24];                                             /* 0x00563844 */
extern char s7d[24];                                                    /* 0x0056385c */
extern intptr_t gZrwRamDiskDevice;                                      /* 0x00563874 */
extern char sErrorOpeningRAMDiskFile[32];                               /* 0x00563878 */
extern char sTooManyRAMDiskFilesInUse[32];                              /* 0x00563898 */
extern char sCNgEngineZgeComZglRwZrwRamDiskC[40];                       /* 0x005638b8 */
extern char sZrwFileRamDiskSetInfNotCalled[36];                         /* 0x005638e0 */
extern BmxType_gZProfileColors gZProfileColors;                         /* 0x00563904 */
extern uint32_t d_00563918;                                             /* 0x00563918 */
extern uint32_t d_0056391c;                                             /* 0x0056391c */
extern char sCNgEngineZgeZprofileC[28];                                 /* 0x00563920 */
extern char sInvalidParameterToZprofileSetLayout[44];                   /* 0x0056393c */
extern char sFailedToAllocateObj[24];                                   /* 0x00563968 */
extern char sUnknownBarType[20];                                        /* 0x00563980 */
extern char sApp[4];                                                    /* 0x00563994 */
extern char sEngCPU[8];                                                 /* 0x00563998 */
extern char sZTIMERAssertFailedUnableToRegisterUpdat[60];               /* 0x005639a0 */
extern char sEventTypeTimerForObjD[32];                                 /* 0x005639dc */
extern char sScriptTypeTimer[20];                                       /* 0x005639fc */
extern char sPROFILENoHardwareCounterAvailable[40];                     /* 0x00563a10 */
extern char sCNgEngineZgePcPcProfileC[40];                              /* 0x00563a38 */
extern struct BmxGroup_p_00563a60Group p_00563a60Group;                 /* 0x00563a60 */
extern char sAssertionFailedSFileSLineD[40];                            /* 0x00563a70 */
extern void *p_00563a98;                                                /* 0x00563a98 */
extern void *p_00563a9c;                                                /* 0x00563a9c */
extern struct BmxGroup_p_00563aa0Group p_00563aa0Group;                 /* 0x00563aa0 */
extern void *p_00563ab0;                                                /* 0x00563ab0 */
extern uint32_t d_00563ab4[3];                                          /* 0x00563ab4 */
extern char sAsin_00563ac0[8];                                          /* 0x00563ac0 */
extern uint32_t d_00563ac8;                                             /* 0x00563ac8 */
extern uint32_t d_00563acc;                                             /* 0x00563acc */
extern struct BmxGroup_gCrtFloatHooksGroup gCrtFloatHooksGroup;         /* 0x00563ad0 */
extern void *p_00563af0;                                                /* 0x00563af0 */
extern void *p_00563af4[3];                                             /* 0x00563af4 */
extern uint32_t d_00563b00[2];                                          /* 0x00563b00 */
extern uint32_t d_00563b08[2050];                                       /* 0x00563b08 */
extern void *p_00565b10;                                                /* 0x00565b10 */
extern uint32_t d_00565b14;                                             /* 0x00565b14 */
extern struct BmxGroup_p_00565b18Group p_00565b18Group;                 /* 0x00565b18 */
extern uint32_t d_00565b28[4];                                          /* 0x00565b28 */
extern uint32_t d_00565b38[8];                                          /* 0x00565b38 */
extern unsigned char d_00565b58[12];                                    /* 0x00565b58 */
extern uint32_t d_00565b64[9];                                          /* 0x00565b64 */
extern unsigned char d_00565b88[528];                                   /* 0x00565b88 */
extern unsigned char d_00565d98[8];                                     /* 0x00565d98 */
extern void *p_00565da0;                                                /* 0x00565da0 */
extern void *p_00565da4;                                                /* 0x00565da4 */
extern uint32_t d_00565da8;                                             /* 0x00565da8 */
extern uint32_t d_00565dac;                                             /* 0x00565dac */
extern uint32_t d_00565db0[28];                                         /* 0x00565db0 */
extern uint32_t d_00565e20;                                             /* 0x00565e20 */
extern uint32_t d_00565e24;                                             /* 0x00565e24 */
extern char s_00565e28[4];                                              /* 0x00565e28 */
extern uint32_t d_00565e2c;                                             /* 0x00565e2c */
extern uint32_t d_00565e30[2];                                          /* 0x00565e30 */
extern uint32_t d_00565e38[4];                                          /* 0x00565e38 */
extern uint32_t d_00565e48;                                             /* 0x00565e48 */
extern struct BmxGroup_p_00565e4cGroup p_00565e4cGroup;                 /* 0x00565e4c */
extern unsigned char d_00565ed8[8];                                     /* 0x00565ed8 */
extern unsigned char d_00565ee0[10];                                    /* 0x00565ee0 */
extern unsigned char d_00565eea[10];                                    /* 0x00565eea */
extern unsigned char d_00565ef4[9];                                     /* 0x00565ef4 */
extern unsigned char d_00565efd[19];                                    /* 0x00565efd */
extern uint32_t d_00565f10;                                             /* 0x00565f10 */
extern struct BmxGroup_p_00565f14Group p_00565f14Group;                 /* 0x00565f14 */
extern uint32_t d_00565fe8[2];                                          /* 0x00565fe8 */
extern uint32_t d_00565ff0[2];                                          /* 0x00565ff0 */
extern uint32_t d_00565ff8[4];                                          /* 0x00565ff8 */
extern uint32_t d_00566008[2];                                          /* 0x00566008 */
extern uint32_t d_00566010[3];                                          /* 0x00566010 */
extern uint32_t d_0056601c[3];                                          /* 0x0056601c */
extern struct BmxGroup_p_00566028Group p_00566028Group;                 /* 0x00566028 */
extern char s_00566032[514];                                            /* 0x00566032 */
extern uint32_t d_00566234;                                             /* 0x00566234 */
extern char s_00566238[8];                                              /* 0x00566238 */
extern char s_00566240[8];                                              /* 0x00566240 */
extern char s_00566248[8];                                              /* 0x00566248 */
extern uint32_t d_00566250[6];                                          /* 0x00566250 */
extern uint32_t d_00566268[6];                                          /* 0x00566268 */
extern uint32_t d_00566280[2];                                          /* 0x00566280 */
extern uint32_t d_00566288;                                             /* 0x00566288 */
extern uint32_t d_0056628c[3];                                          /* 0x0056628c */
extern uint32_t d_00566298[56];                                         /* 0x00566298 */
extern unsigned char d_00566378[8];                                     /* 0x00566378 */
extern uint32_t d_00566380[2];                                          /* 0x00566380 */
extern uint32_t d_00566388;                                             /* 0x00566388 */
extern uint32_t d_0056638c[89];                                         /* 0x0056638c */
extern uint32_t d_005664f0[88];                                         /* 0x005664f0 */
extern uint32_t d_00566650[88];                                         /* 0x00566650 */
extern RwPluginRegistry rpClumpRegistry;                                /* 0x005667b0 */
extern RwPluginRegistry rpAtomicRegistry;                               /* 0x005667c4 */
extern RwPluginRegistry rpGeometryRegistry;                             /* 0x005667e0 */
extern RwPluginRegistry rpMaterialRegistry;                             /* 0x00566800 */
extern RwSurfaceProperties rpMaterialDefaultSurfaceProps;               /* 0x00566814 */
extern RwPluginRegistry rpWorldRegistry;                                /* 0x00566820 */
extern float d_00566840;                                                /* 0x00566840 */
extern float d_00566844;                                                /* 0x00566844 */
extern float d_00566848;                                                /* 0x00566848 */
extern float d_0056684c;                                                /* 0x0056684c */
extern float d_00566850;                                                /* 0x00566850 */
extern float d_00566854[3];                                             /* 0x00566854 */
extern RwPluginRegistry rpWorldSectorRegistry;                          /* 0x00566860 */
extern RwPluginRegistry rpLightRegistry;                                /* 0x00566880 */
extern uint32_t rwD3DAtomicDefaultColor;                                /* 0x005668a0 */
extern uint32_t rwD3DSectorDefaultColor;                                /* 0x005668b0 */
extern BmxType_rtCharsetGlyphs rtCharsetGlyphs;                         /* 0x005668c0 */
extern struct BmxGroup_rwEngineRegistryGroup rwEngineRegistryGroup;     /* 0x00566dc0 */
extern RwPluginRegistry rwFrameRegistry;                                /* 0x00566ee0 */
extern char sSoftwareCriterionRenderWare[32];                           /* 0x00566f00 */
extern int32_t rwD3DRenderOrder;                                        /* 0x00566f20 */
extern RwPluginRegistry rwRasterRegistry;                               /* 0x00566f30 */
extern RwPluginRegistry rwCameraRegistry;                               /* 0x00566f50 */
extern char s0123456789abcdefghijklmnopqrstuvwxyz[40];                  /* 0x00566f70 */
extern RwPluginRegistry rwTextureRegistry;                              /* 0x00566f98 */
extern RwPluginRegistry rwTexDictionaryRegistry;                        /* 0x00566fac */
extern RwPluginRegistry rwImageRegistry;                                /* 0x00566fc0 */
extern uint32_t d_00566fe0;                                             /* 0x00566fe0 */
extern RwDevice rwD3D7Device;                                           /* 0x00566fe4, also rwOrigDevice */
extern struct BmxGroup_d_0056701cGroup d_0056701cGroup;                 /* 0x0056701c */
extern uint32_t d_00567100[32];                                         /* 0x00567100 */
extern uint32_t d_00567180[32];                                         /* 0x00567180 */
extern uint32_t d_00567200[31];                                         /* 0x00567200 */
extern struct BmxGroup_d_0056727cGroup d_0056727cGroup;                 /* 0x0056727c */
extern uint32_t d_00567300[4];                                          /* 0x00567300 */
extern char s105[12];                                                   /* 0x00567310 */
extern uint32_t d_0056731c[2];                                          /* 0x0056731c */
extern char sIHDR[8];                                                   /* 0x00567324 */
extern char sIDAT[8];                                                   /* 0x0056732c */
extern char sIEND[8];                                                   /* 0x00567334 */
extern char sPLTE[8];                                                   /* 0x0056733c */
extern char sBKGD[8];                                                   /* 0x00567344 */
extern char sCHRM[8];                                                   /* 0x0056734c */
extern char sGAMA[8];                                                   /* 0x00567354 */
extern char sHIST[8];                                                   /* 0x0056735c */
extern char sOFFs[8];                                                   /* 0x00567364 */
extern char sPCAL[8];                                                   /* 0x0056736c */
extern char sPHYs[8];                                                   /* 0x00567374 */
extern char sSBIT[8];                                                   /* 0x0056737c */
extern char sSRGB[8];                                                   /* 0x00567384 */
extern char sTEXt[8];                                                   /* 0x0056738c */
extern char sTIME[8];                                                   /* 0x00567394 */
extern char sTRNS[8];                                                   /* 0x0056739c */
extern char sZTXt[8];                                                   /* 0x005673a4 */
extern uint32_t d_005673ac[7];                                          /* 0x005673ac */
extern uint32_t d_005673c8[7];                                          /* 0x005673c8 */
extern uint32_t d_005673e4[7];                                          /* 0x005673e4 */
extern uint32_t d_00567400[7];                                          /* 0x00567400 */
extern uint32_t d_0056741c[7];                                          /* 0x0056741c */
extern uint32_t d_00567438[10];                                         /* 0x00567438 */
extern uint32_t d_00567460[8];                                          /* 0x00567460 */
extern uint32_t d_00567480[64];                                         /* 0x00567480 */
extern uint32_t d_00567580[64];                                         /* 0x00567580 */
extern uint32_t d_00567680[64];                                         /* 0x00567680 */
extern uint32_t d_00567780[4];                                          /* 0x00567780 */
extern char sErrorDecodingZTXtChunk[32];                                /* 0x00567790 */
extern uint32_t d_005677b0[256];                                        /* 0x005677b0 */
extern struct BmxGroup_d_00567bb0Group d_00567bb0Group;                 /* 0x00567bb0 */
extern struct BmxGroup_p_00567c08Group p_00567c08Group;                 /* 0x00567c08 */
extern uint32_t d_00567c30[20];                                         /* 0x00567c30 */
extern uint32_t d_00567c80[30];                                         /* 0x00567c80 */
extern uint32_t d_00567cf8[30];                                         /* 0x00567cf8 */
extern uint32_t d_00567d70[31];                                         /* 0x00567d70 */
extern uint32_t d_00567dec[31];                                         /* 0x00567dec */
extern uint32_t d_00567e68[64];                                         /* 0x00567e68 */
extern uint32_t d_00567f68[1024];                                       /* 0x00567f68 */
extern uint32_t d_00568f68;                                             /* 0x00568f68 */
extern char s_00568f6c[52];                                             /* 0x00568f6c */
extern char sTan_00568fa0[16];                                          /* 0x00568fa0 */
extern char sSin_00568fb0[16];                                          /* 0x00568fb0 */
extern char sCos_00568fc0[16];                                          /* 0x00568fc0 */
extern double d_00568fd0;                                               /* 0x00568fd0 */
extern char sPow_00568fd8[8];                                           /* 0x00568fd8 */
extern unsigned char d_00568fe0[10];                                    /* 0x00568fe0 */
extern unsigned char d_00568fea[20];                                    /* 0x00568fea */
extern unsigned char d_00568ffe[20];                                    /* 0x00568ffe */
extern struct BmxGroup_d_00569012Group d_00569012Group;                 /* 0x00569012 */
extern uint32_t d_00569050[4];                                          /* 0x00569050 */
extern float d_00569060;                                                /* 0x00569060 */
extern float d_00569064;                                                /* 0x00569064 */
extern uint32_t d_00569068;                                             /* 0x00569068 */
extern uint32_t d_0056906c[6];                                          /* 0x0056906c */
extern float d_00569084[2];                                             /* 0x00569084 */
extern uint32_t d_0056908c[4];                                          /* 0x0056908c */
extern float d_0056909c[2];                                             /* 0x0056909c */
extern unsigned char d_005690a4[10];                                    /* 0x005690a4 */
extern struct BmxGroup_p_005690aeGroup p_005690aeGroup;                 /* 0x005690ae */
extern uint32_t d_005691b0;                                             /* 0x005691b0 */
extern uint32_t d_005691b4;                                             /* 0x005691b4 */
extern uint32_t d_005691b8;                                             /* 0x005691b8 */
extern char sPST[64];                                                   /* 0x005691bc */
extern char sPDT[64];                                                   /* 0x005691fc */
extern void *p_0056923c;                                                /* 0x0056923c */
extern struct BmxGroup_p_00569240Group p_00569240Group;                 /* 0x00569240 */
extern uint32_t d_00569248;                                             /* 0x00569248 */
extern unsigned char d_0056924c[4];                                     /* 0x0056924c */
extern unsigned char d_00569250[8];                                     /* 0x00569250 */
extern uint32_t d_00569258;                                             /* 0x00569258 */
extern unsigned char d_0056925c[4];                                     /* 0x0056925c */
extern unsigned char d_00569260[4];                                     /* 0x00569260 */
extern uint32_t d_00569264[12];                                         /* 0x00569264 */
extern uint32_t d_00569294;                                             /* 0x00569294 */
extern uint32_t d_00569298[2130];                                       /* 0x00569298 */
extern ActionState gAction;                                             /* 0x0056b3e0 */
extern uint8_t gCameraPlayer;                                           /* 0x0056b408, also gCurrentPlayer */
extern struct BmxGroup_gCameraRectGroup gCameraRectGroup;               /* 0x0056b410 */
extern CameraStates gCameras;                                           /* 0x0056b448 */
extern struct BmxGroup_gCameraResetGroup gCameraResetGroup;             /* 0x0056b5b8 */
extern uint8_t gSelLevelSetting;       /* 0x0056b614, also gCurrentLevel, gNgLevel */
extern uint8_t gCurrentPlayer;         /* 0x0056b615, also gNgPlayer */
extern uint8_t gGameModeSel;           /* 0x0056b616, also gActionSubMode */
extern uint8_t gSelResume;             /* 0x0056b617, also gOvlWarnLevel */
extern uint32_t gAutoLoadDone;         /* 0x0056b618 */
extern uint8_t gContrlerMode;          /* 0x0056b620 */
extern ContrlerPads gContrlerPads;     /* 0x0056b628 */
extern ContrlerAxes gContrlerAxes;     /* 0x0056b6a0 */
extern unsigned char d_0056b6a8[4];    /* 0x0056b6a8 */
extern unsigned char d_0056b6ac[8];    /* 0x0056b6ac */
extern unsigned char d_0056b6b4[16];   /* 0x0056b6b4 */
extern uint32_t gFxActive;             /* 0x0056b6c4 */
extern uint32_t gFxUnused;             /* 0x0056b6c8 */
extern PhysRider *gFxRider;            /* 0x0056b6cc */
extern ZVec3 gFxZero;                  /* 0x0056b6d0 */
extern GameStartPos gGameStartPos;     /* 0x0056b6e8 */
extern uint8_t gGameReplayAll;         /* 0x0056b6f8 */
extern uint8_t gGameReplayShared;      /* 0x0056b6f9 */
extern GameReplayHandles gGameReplays; /* 0x0056b6fc */
extern uint8_t gGameOptions;           /* 0x0056b700 */
extern uint8_t gGameMode;              /* 0x0056b701 */
extern uint8_t gGameSubMode;           /* 0x0056b702 */
extern uint8_t gGameRunCount;          /* 0x0056b703 */
extern uint8_t gGameRunType;           /* 0x0056b704 */
extern uint8_t gGamePhase;             /* 0x0056b705 */
extern uint8_t gGamePhaseTicks;        /* 0x0056b706 */
extern uint8_t gGameStartPlayer;       /* 0x0056b707 */
extern uint32_t gGameTimer;            /* 0x0056b708 */
extern struct BmxGroup_gGameRoundStatesGroup gGameRoundStatesGroup; /* 0x0056b70c */
extern ReplayTable gReplays;                                        /* 0x0056d278 */
extern unsigned char d_0056d28c[4];                                 /* 0x0056d28c */
extern uint32_t gGuiMarkupTarget;                                   /* 0x0056d290 */
extern GuiPad gGuiPad;                                              /* 0x0056d298 */
extern GuiElementArray gGuiElements;                                /* 0x0056d2c0 */
extern uint16_t gGuiScreenId;                                       /* 0x00573080 */
extern GuiScreenState gGuiScreen;                                   /* 0x00573088 */
extern GuiHookFn gGuiDefaultHook;                                   /* 0x005730a0 */
extern GuiElementPtrArray gGuiSorted;                               /* 0x005730a4 */
extern int gGuiInitialized;                                         /* 0x00573234 */
extern int gGuiModeReady;                                           /* 0x00573238 */
extern int gGuiElementCount;                                        /* 0x0057323c */
extern int gGuiScreenLoaded;                                        /* 0x00573240 */
extern int gGuiReloadPending;                                       /* 0x00573244 */
extern int gGuiReloadDelay;                                         /* 0x00573248 */
extern int gGuiSkipUpdate;                                          /* 0x0057324c */
extern int gGuiAssetsLoaded;                                        /* 0x00573250, also gNgGuiReady */
extern int gGuiPulse;                                               /* 0x00573254 */
extern int gGuiPulseHold;                                           /* 0x00573258 */
extern int gGuiSlideState;                                          /* 0x0057325c */
extern struct BmxGroup_gGuiSlideMaxGroup gGuiSlideMaxGroup;         /* 0x00573260 */
extern int gOptMainPulse;                                           /* 0x00574584, also gExtrasPulse */
extern SelStepFn gSelPreviewFn;                                     /* 0x00574588 */
extern FrontendStepFn gFrontendStep;                                /* 0x0057458c */
extern SelStepFn gSelCommitFn;                                      /* 0x00574590 */
extern ZPrimAsset *gMenuFrameSprite;                                /* 0x00574594 */
extern McDoneFn gMcDone;                                            /* 0x0057459c */
extern uint16_t gTitleSel;                                          /* 0x005745a0 */
extern TitleActionFn gTitleAction;                                  /* 0x005745a4 */
extern uint32_t gAutoLoadOk;                                        /* 0x005745a8 */
extern uint16_t gAutoTick;                                          /* 0x005745ac */
extern uint16_t gTitleStage;                                        /* 0x005745ae */
extern KeyWork gKbWork;                                             /* 0x005745b4 */
extern uint8_t gKbCursor;                                           /* 0x005745e0 */
extern uint8_t gKbState;                                            /* 0x005745e1 */
extern uint8_t gKbLayout;                                           /* 0x005745e2 */
extern SelStyleSprites gStyleSprites;                               /* 0x005745ec */
extern struct BmxGroup_gStyleFrameGroup gStyleFrameGroup;           /* 0x0057462c */
extern uint16_t gExtrasSel;                                         /* 0x00574644 */
extern uint32_t gExtrasPlayed;                                      /* 0x00574648 */
extern void *gAssetSetData;                                         /* 0x0057464c */
extern int gExitFlag;                                               /* 0x00574654, also gBmxMenuExit */
extern int gExitCode;                                               /* 0x00574658, also gBmxMenuExitResult */
extern void *gMacroBuf;                                             /* 0x0057465c */
extern uint32_t gMacroSize;                                         /* 0x00574660 */
extern uint8_t gBmxCurrentTrack;                                    /* 0x00574668 */
extern struct BmxGroup_gBmxTimerIdGroup gBmxTimerIdGroup;           /* 0x0057466c */
extern uint32_t gBmxTimerPeriod;                                    /* 0x00574694 */
extern uint8_t gBmxGunChannel;                                      /* 0x00574698 */
extern BmxMenuChannelTable gBmxMenuChannels;                        /* 0x0057469c */
extern uint8_t gBmxLevelIndex;                                      /* 0x0057469e */
extern uint8_t gBmxRiderIndex;                                      /* 0x0057469f */
extern uint8_t gBmxAmbientSide;                                     /* 0x005746a0 */
extern struct WinstrmStream *gBmxMusicStream;                       /* 0x005746a4 */
extern uint32_t gBmxTrackCooldown;                                  /* 0x005746a8 */
extern uint8_t gBmxMenuSfxSet;                                      /* 0x005746ac */
extern struct BmxGroup_gLastPollTimeGroup gLastPollTimeGroup;       /* 0x005746b0 */
extern ZVec3 gZoverArmSide;                                         /* 0x005746c4 */
extern ZVec3 gZoverLimb3Elbow;                                      /* 0x005746d4 */
extern ZMatrix gZoverLimb1Upper;                                    /* 0x005746e4 */
extern ZVec3 gZoverLimb0Elbow;                                      /* 0x00574724 */
extern ZoverRigFlags gZoverRelaxed;                                 /* 0x00574734 */
extern ZoverRigFlags gZoverStiff;                                   /* 0x0057473c */
extern ZVec3 gZoverLimb3Target;                                     /* 0x00574744 */
extern ZMatrix gZoverLimb0Lower;                                    /* 0x00574754 */
extern ZMatrix gZoverFrameShin46;                                   /* 0x00574794 */
extern ZVec3 gZoverLimb0Bend;                                       /* 0x005747d4 */
extern ZVec3 gZoverLimb0Target;                                     /* 0x005747e4 */
extern ZMatrix gZoverFrameShin35;                                   /* 0x005747f4 */
extern ZVec3 gZoverLimb1Target;                                     /* 0x00574834 */
extern ZVec3 gZoverLimb2Target;                                     /* 0x00574844 */
extern ZMatrix gZoverFrameForearm911;                               /* 0x00574854 */
extern ZMatrix gZoverSpineFrame;                                    /* 0x00574894 */
extern ZoverRigFlags gZoverHandsEased;                              /* 0x005748d4 */
extern ZVec3 gZoverLimb2Root;                                       /* 0x005748dc */
extern ZVec3 gZoverLimb2Elbow;                                      /* 0x005748ec */
extern ZVec3 gZoverLimb1Elbow;                                      /* 0x005748fc */
extern ZVec3 gZoverLimb3Bend;                                       /* 0x0057490c */
extern ZoverRigFlags gZoverHandsGrabbed;                            /* 0x0057491c */
extern ZoverRigFlags gZoverHandsHolding;                            /* 0x00574924 */
extern float gZoverArmDist;                                         /* 0x0057492c */
extern ZoverRigFlags gZoverLimp;                                    /* 0x00574930 */
extern ZMatrix gZoverFrameForearm1012;                              /* 0x00574938 */
extern ZMatrix gZoverLimb3Upper;                                    /* 0x00574978 */
extern ZVec3 gZoverLimb1Root;                                       /* 0x005749b8 */
extern ZMatrix gZoverLimb2Upper;                                    /* 0x005749c8 */
extern float gZoverLimbUpperLen;                                    /* 0x00574a08 */
extern float gZoverLimbSin;                                         /* 0x00574a0c */
extern ZVec3 gZoverLimb1Bend;                                       /* 0x00574a10 */
extern ZVec3 gZoverLegSide;                                         /* 0x00574a20 */
extern ZVec3 gZoverLimb0Root;                                       /* 0x00574a30 */
extern ZAngle gZoverLimbAngle;                                      /* 0x00574a40 */
extern ZVec3 gZoverLimb3Root;                                       /* 0x00574a44 */
extern ZMatrix gZoverFrameHead;                                     /* 0x00574a54 */
extern ZMatrix gZoverLimb1Lower;                                    /* 0x00574a94 */
extern ZoverRigFlags gZoverSticky;                                  /* 0x00574ad4 */
extern ZMatrix gZoverActorFrame;                                    /* 0x00574adc */
extern ZoverRigFlags gZoverYawOn;                                   /* 0x00574b1c */
extern ZoverRigFlags gZoverPitchOn;                                 /* 0x00574b24 */
extern ZMatrix gZoverLimbProduct;                                   /* 0x00574b2c */
extern ZVec3 gZoverLimbDir;                                         /* 0x00574b6c */
extern float gZoverLegDist;                                         /* 0x00574b7c */
extern float gZoverLimbCos;                                         /* 0x00574b80 */
extern ZMatrix gZoverLimb3Lower;                                    /* 0x00574b84 */
extern ZoverRigTable gZoverRigs;                                    /* 0x00574bc4 */
extern float gZoverLimbDistCopy;                                    /* 0x00574bcc */
extern ZMatrix gZoverLimb2Lower;                                    /* 0x00574bd0 */
extern ZMatrix gZoverLimbTransposed;                                /* 0x00574c10 */
extern float gZoverLimbLowerLen;                                    /* 0x00574c50 */
extern ZVec3 gZoverLimb2Bend;                                       /* 0x00574c54 */
extern ZMatrix gZoverLimb0Upper;                                    /* 0x00574c64 */
extern int32_t gZoverRigNext;                                       /* 0x00574ca4 */
extern int32_t gZoverRigIndex;                                      /* 0x00574ca8 */
extern ZoverRigAngles gZoverPitch;                                  /* 0x00574cac */
extern ZoverRigAngles gZoverYaw;                                    /* 0x00574cb0 */
extern ZoverIndices6 gZoverHingeIdx;                                /* 0x00574cb4 */
extern ZoverIndices8 gZoverStickIdx;                                /* 0x00574ccc */
extern ZoverIndices3 gZoverBackIdx;                                 /* 0x00574cec */
extern ZVec3 gZoverLimbOffset;                                      /* 0x00574cf8 */
extern struct BmxGroup_gZoverLimbSyncedGroup gZoverLimbSyncedGroup; /* 0x00574d08 */
extern ZOverFontTable gZoverFonts;                                  /* 0x00574e50 */
extern ZOverPool3 gZoverPool3;                                      /* 0x00574e70 */
extern ZOverPool2 gZoverPool2;                                      /* 0x00594270 */
extern struct BmxGroup_gZoverRectSizeGroup gZoverRectSizeGroup;     /* 0x00596570 */
extern unsigned char d_005965a2[2];                                 /* 0x005965a2 */
extern ZOverHandleList gZoverFreeList;                              /* 0x005965a4 */
extern ZOverTerminators gZoverTerminators;                          /* 0x00596a20 */
extern uint8_t gZoverFontCount;                                     /* 0x00596a30 */
extern ZOverPool1 gZoverPool1;                                      /* 0x00596a38 */
extern ZOverPool0 gZoverPool0;                                      /* 0x005a0a38 */
extern ZOverHandleList gZoverDirtyList;                             /* 0x005a1e38 */
extern uint32_t gZoverBusy;                                         /* 0x005a22b0 */
extern uint32_t gZoverDirtyCount;                                   /* 0x005a22b4 */
extern uint32_t gZoverFreeCount;                                    /* 0x005a22b8 */
extern uint8_t gZoverFrame;                                         /* 0x005a22bc */
extern struct BmxGroup_gZoverLayerGroup gZoverLayerGroup;           /* 0x005a22bd */
extern uint8_t gZsystemMode;                                    /* 0x005ab3a0, also gGuiAnalogPad, gNgMode */
extern BmxType_gNgSpotLights gNgSpotLights;                     /* 0x005ab3ac */
extern struct BmxGroup_gNgArtNameGroup gNgArtNameGroup;         /* 0x005ab3b8 */
extern struct BmxGroup_gNgLightsGroup gNgLightsGroup;           /* 0x005ab438 */
extern float gNgElapsed;                                        /* 0x005ab454, also gFxDebugFloat */
extern void *gNgMainLight;                                      /* 0x005ab458 */
extern uint32_t gNgWake;                                        /* 0x005ab460 */
extern uint32_t *gNgPrevMark;                                   /* 0x005ab468 */
extern void *gNgArt;                                            /* 0x005ab46c */
extern ZrwUpdateNode *gNgLoadNode;                              /* 0x005ab470 */
extern int gMenuInstant;                                        /* 0x005ab474, also gGuiIdleMode, gNgFading */
extern uint32_t gNgCountdown;                                   /* 0x005ab478 */
extern uint32_t gNgSunTick;                                     /* 0x005ab47c */
extern uint16_t gNgLoadFlag;                                    /* 0x005ab480 */
extern void *gNgLegal1;                                         /* 0x005ab488 */
extern void *gNgLoadTexture;                                    /* 0x005ab48c */
extern void *gNgLegal2;                                         /* 0x005ab490 */
extern uint32_t gFxQualityTime;                                 /* 0x005ab49c */
extern int32_t gFxFrames;                                       /* 0x005ab4a0 */
extern int32_t gFxAdjusting;                                    /* 0x005ab4a4 */
extern struct BmxGroup_gFxQualityStepGroup gFxQualityStepGroup; /* 0x005ab4a8 */
extern uint32_t gFxSparkTexture;                                /* 0x005ab4e0 */
extern uint32_t gFxSkidTexture;                                 /* 0x005ab4e4 */
extern uint32_t gFxRippleTexture;                               /* 0x005ab4e8 */
extern uint32_t gFxSplashTexture;                               /* 0x005ab4ec */
extern const int8_t *gNgifScript;                               /* 0x005ab4f0 */
extern void *gFxCube;                                           /* 0x005ab4f4 */
extern struct BmxGroup_gNgifPrimBufferGroup gNgifPrimBufferGroup;   /* 0x005ab4fc */
extern void *gNgifFlushNode;                                        /* 0x005d2500 */
extern int gNgifFlushPending2;                                      /* 0x005d2504, also gZprimFrameEnded */
extern void *gNgifFlushNode2;                                       /* 0x005d2508 */
extern uint32_t *gNgifPrimStart;                                    /* 0x005d250c */
extern struct BmxGroup_gPrimCursorGroup gPrimCursorGroup;           /* 0x005d2510 */
extern ZactorPoseTables gZactorPose;                                /* 0x005da1e8 */
extern struct BmxGroup_gZactorArmHashesGroup gZactorArmHashesGroup; /* 0x005da2c8 */
extern ZbspNearHitTable gZbspNearHits;                              /* 0x005e63d8 */
extern uint32_t gZbspMatRefValue;                                   /* 0x005e7fd8 */
extern struct BmxGroup_gZbspPropFacesGroup gZbspPropFacesGroup;     /* 0x005e7fe0 */
extern unsigned char d_005f0b98[136];                               /* 0x005f0b98 */
extern ZbspSphereResult gZbspSphere;                                /* 0x005f0c20 */
extern ZbspFaceQuery gZbspFaceQuery;                                /* 0x005f0c28 */
extern ZbspHitList gZbspHits;                                       /* 0x005f0c68 */
extern PhysEdge gZbspEdgeRecord;                                    /* 0x005f1070 */
extern struct BmxGroup_gZbspNearCornersGroup gZbspNearCornersGroup; /* 0x005f10a8 */
extern struct ZMemFreeList *gZresListA;                             /* 0x005f34ac, also gZbspFacePool */
extern struct ZMemFreeList *gZresListB;                             /* 0x005f34b0, also gZbspEdgePool */
extern FxLightFn gFxLightCallback;                                  /* 0x005f34b4 */
extern FxListFn gFxListCallback;                                    /* 0x005f34b8 */
extern struct BmxGroup_gZbspPropFaceCountGroup gZbspPropFaceCountGroup; /* 0x005f34bc */
extern BmxType_gZcollideProbeMaterial gZcollideProbeMaterial;           /* 0x005f8518 */
extern BmxType_gZcollideEdgeList gZcollideEdgeList;                     /* 0x005f8520 */
extern BmxType_gZcollideEdgeRecords gZcollideEdgeRecords;               /* 0x005f85a8 */
extern BmxType_gZcollideEdgeCells gZcollideEdgeCells;                   /* 0x005f8ce0 */
extern struct BmxGroup_gZbspNoShapeKindGroup gZbspNoShapeKindGroup;     /* 0x005f8d68 */
extern ZmemdmmState gZmemdmm;                                           /* 0x005f8f30 */
extern struct BmxGroup_gZresGroup gZresGroup;                           /* 0x005f8f60 */
extern ZrollerDrawTable gZrollerDrawCallbacks;                          /* 0x005fb6c8 */
extern int gZrollerReserved;                                            /* 0x005fb6e8 */
extern ZrollerSlotTable gZrollerSlots;                                  /* 0x005fb6f0 */
extern ZrollerShadowSlots gZrollerShadowSlots;                          /* 0x005fb710 */
extern ZrollerBounds gZrollerBounds;                                    /* 0x005fb718 */
extern int gZrollerSlotCount;                                           /* 0x005fb738 */
extern uint32_t gNgWord5fb73c;                                          /* 0x005fb73c */
extern struct BmxGroup_gNgRollerNodeGroup gNgRollerNodeGroup;           /* 0x005fb740 */
extern DsBuffer *gWinchanBuffer;                                        /* 0x005fb760 */
extern int32_t gWinchanFlangerDelay;                                    /* 0x005fb764 */
extern int32_t gWinchanActive;                                          /* 0x005fb768 */
extern uint32_t gWinchanEffectMaxFrames;                                /* 0x005fb76c */
extern WinchanChannelTable gWinchanChannels;                            /* 0x005fb770 */
extern int32_t gWinchanMixRight;                                        /* 0x005fbe30 */
extern float gWinchanEffectAGain;                                       /* 0x005fbe34 */
extern struct BmxGroup_gWinchanMixLeftGroup gWinchanMixLeftGroup;       /* 0x005fbe38 */
extern WinchanFlangerLine gWinchanFlangerLine;                          /* 0x005fbebc */
extern uint32_t gWinchanCopyOffset;                                     /* 0x005fdebc */
extern uint32_t gWinchanWriteOffset;                                    /* 0x005fdec0 */
extern int32_t gWinchanEchoPos;                                         /* 0x005fdec4 */
extern WinchanLfoTable gWinchanLfoTable;                                /* 0x005fdec8 */
extern unsigned char d_005ffec8[4];                                     /* 0x005ffec8 */
extern int32_t gWinchanFlangerPhase;                                    /* 0x005ffecc */
extern WinchanFrame *gWinchanEchoBuffer;                                /* 0x005ffed0 */
extern void *gWinchanDsoundModule;                                      /* 0x005ffed4 */
extern uint8_t *gWinchanRing;                                           /* 0x005ffed8 */
extern uint32_t gWinchanChunkBytes;                                     /* 0x005ffedc */
extern uint32_t gWinchanEffectBFrames;                                  /* 0x005ffee0 */
extern uint32_t gWinchanRate;                                           /* 0x005ffee4 */
extern float gWinchanEffectBGain;                                       /* 0x005ffee8 */
extern uint32_t gWinchanEffectAFrames;                                  /* 0x005ffeec */
extern float gWinchanEchoFeedback;       /* 0x005ffef0, also gWinchanEffectAParam */
extern uint32_t gWinchanMixOffset;       /* 0x005ffef4 */
extern uint32_t gWinchanLastPlay;        /* 0x005ffef8 */
extern int32_t gWinchanEffectBTailStart; /* 0x005ffefc */
extern int32_t gWinchanFlangerPos;       /* 0x005fff00 */
extern int32_t gWinchanEffectBTail;      /* 0x005fff04 */
extern uint32_t gWinchanRingBytes;       /* 0x005fff08 */
extern int32_t gWinchanInitialized;      /* 0x005fff0c */
extern int32_t gWinchanLock;             /* 0x005fff10 */
extern int32_t gWinchanNextChannel;      /* 0x005fff14 */
extern int32_t gWinchanNextHandle;       /* 0x005fff18 */
extern void *gWinchanDsoundCreate;       /* 0x005fff1c */
extern DsBuffer *gWinchanPrimary;        /* 0x005fff20 */
extern int32_t gWinchanPumpCount;        /* 0x005fff24 */
extern struct BmxGroup_gWinstrmDirectSoundGroup gWinstrmDirectSoundGroup; /* 0x005fff28 */
extern WinloadSampleFile gWinloadSampleFile;                              /* 0x00600d88 */
extern WinloadFileTable gWinloadFiles;                                    /* 0x00600d98 */
extern struct BmxGroup_gWinloadCountsGroup gWinloadCountsGroup;           /* 0x00601218 */
extern int32_t gWinstrmSourceHint;                                        /* 0x00601328 */
extern WinstrmLockStorage gWinstrmLock;                                   /* 0x00601330 */
extern int32_t gWinstrmStreamHint;                                        /* 0x00601348 */
extern WinstrmSourceTable gWinstrmSources;                                /* 0x00601350 */
extern WinstrmStreamTable gWinstrmStreams;                                /* 0x006017d0 */
extern int32_t gWinstrmNodeHint;                                          /* 0x006019f0 */
extern WinstrmNodeTable gWinstrmNodes;                                    /* 0x006019f8 */
extern int32_t gWinstrmInitialized;                                       /* 0x00601b78 */
extern int32_t gWinstrmPaused;                                            /* 0x00601b7c */
extern struct BmxGroup_gWinstrmTickGroup gWinstrmTickGroup;               /* 0x00601b80 */
extern ZActorRootPose gActorRootPoseTemp;                                 /* 0x00601d30 */
extern ZMotionDelta gMotionLastDelta;                                     /* 0x00601d48 */
extern ZMotionEntry gMotionEntryTemp;                                     /* 0x00601d60 */
extern ZMotionDelta gMotionDeltaA;                                        /* 0x00601d68 */
extern ZMotionDelta gMotionDeltaB;                                        /* 0x00601d80 */
extern ZMotionDelta gMotionDeltaC;                                        /* 0x00601d98 */
extern ZMotionDelta gMotionDeltaD;                                        /* 0x00601db0 */
extern struct BmxGroup_gMotionDeltaEGroup gMotionDeltaEGroup;             /* 0x00601dc8 */
extern int16_t gMotionPrevYawE;                                           /* 0x00601e08 */
extern void *gMotionFirstBank;                                            /* 0x00601e10 */
extern ZMotionDelta *gMotionSampleA;                                      /* 0x00601e14 */
extern ZMotionDelta *gMotionSampleB;                                      /* 0x00601e18 */
extern ZMotionStaging gMotionStaging;                                     /* 0x00601e1c */
extern ZActor *gZactors;                                                  /* 0x00641e1c */
extern int32_t gMotionBankCount;                                          /* 0x00641e20 */
extern int32_t gActorAngleBase;                                           /* 0x00641e24 */
extern int32_t gActorAngleFlip;                                           /* 0x00641e28 */
extern struct ZassetLibrary *gMotionLibrary;                              /* 0x00641e2c */
extern struct BmxGroup_gMotionBufferGroup gMotionBufferGroup;             /* 0x00641e30 */
extern uint32_t gZprimTextureCounter;                                     /* 0x0064eb34 */
extern ZsndNotePool gZsndNotes;                                           /* 0x0064eb38 */
extern int32_t gZsndNextNote;                                             /* 0x0064f038 */
extern int32_t gZsndMuteAll;                                              /* 0x0064f03c */
extern int32_t gZsndNextTrack;                                            /* 0x0064f040 */
extern ZsndTrackPool gZsndTrackPool;                                      /* 0x0064f048 */
extern ZsndTrackTable gZsndTracks;                                        /* 0x0064f948 */
extern struct BmxGroup_gZsndSeqLockGroup gZsndSeqLockGroup;               /* 0x0064f9c8 */
extern WinstrmStream *gZsndStream;                                        /* 0x0064fcb8 */
extern uint32_t gZsndStreamBytes;                                         /* 0x0064fcbc */
extern int32_t gZsndSrcNoDoppler;                                         /* 0x0064fcc8 */
extern uint32_t gZsndSrcRange;                                            /* 0x0064fccc */
extern ZsndSourceTable gZsndSources;                                      /* 0x0064fcd0 */
extern float gZsndSrcDopplerScale;                                        /* 0x00650090 */
extern uint32_t gZsndSrcRange2;                                           /* 0x00650094 */
extern int32_t gZsndSrcInitialized;                                       /* 0x00650098 */
extern uint32_t gZsndSrcNext;                                             /* 0x0065009c */
extern uint32_t gZsndSrcLifetime;                                         /* 0x006500a0 */
extern unsigned char d_006500b0[192];                                     /* 0x006500b0 */
extern int gZdebugCreated;                                                /* 0x00650170 */
extern unsigned char d_00650174[4];                                       /* 0x00650174 */
extern unsigned char d_00650178[8];                                       /* 0x00650178 */
extern BmxType_gScratch gScratch;                                         /* 0x00650180 */
extern int gZrwDbgInitialized;                                            /* 0x00650470 */
extern uint8_t gZrwDbgFpsMode;                                            /* 0x00650474 */
extern ZrwUpdateNode *gZrwDbgUpdateNode;                                  /* 0x00650478 */
extern ZrwList gZrwDbgCallbacks;                                          /* 0x0065047c */
extern void *gZrwDbgCharset;                                              /* 0x00650488 */
extern int gZrwDbgCharWidth;                                              /* 0x0065048c */
extern int gZrwDbgCharHeight;                                             /* 0x00650490 */
extern BmxType_gFormatBuffer gFormatBuffer;                               /* 0x00650494 */
extern BmxType_gTextQueue gTextQueue;                                     /* 0x00650694 */
extern uint8_t *gTextEnd;                                                 /* 0x00650a94 */
extern int gZrwDbgItemCount;                                              /* 0x00650a98 */
extern BmxType_gItems gItems;                                             /* 0x00650a9c */
extern int gZdebugIndentLevel;                                            /* 0x0065e29c, also gDebugIndent */
extern ZrwDbgVertex *gSphereVerts;                                        /* 0x0065e2a0 */
extern ZrwDbgVertex **gSpherePointers;                                    /* 0x0065e2a4 */
extern int gZinputReady;                                                  /* 0x0065e2a8 */
extern ZMemFreeList *gZpadDevices;                                        /* 0x0065e2ac */
extern ZMemFreeList *gZassetLibraries;                                    /* 0x0065e2b0 */
extern ZMathTables gZmathTables;                                          /* 0x0065e2bc */
extern struct BmxGroup_gZmathOriginGroup gZmathOriginGroup;               /* 0x006702d8 */
extern int gZobjCameraInitialized;                                        /* 0x006702f8 */
extern ZMemFreeList *gZobjCameraCaches;                                   /* 0x006702fc */
extern ZVec3 gZobjCameraTarget;                                           /* 0x00670300 */
extern unsigned char d_00670310[200];                                     /* 0x00670310 */
extern ZstrTempBuffer gZstrTempBuffer;                                    /* 0x006703d8 */
extern ZStringSetTable gZstringSets;                                      /* 0x006704d8 */
extern int gNameTablesReady;                                              /* 0x00670518 */
extern ZNameTables gNameTables;                                           /* 0x0067051c */
extern ZfileErrorBuffer gZfileErrorMessage;                               /* 0x00670528 */
extern ZfileDevice *gZfileDefaultDevice;                                  /* 0x00670568 */
extern unsigned char d_0067056c[64];                                      /* 0x0067056c */
extern ZfileNameBuffer gZfileNameBuffer;                                  /* 0x006705ac */
extern ZfileDeviceTable gZfileDevices;                                    /* 0x00670630 */
extern int gZfileInitialized;                                             /* 0x00670674 */
extern ZMemFreeList *gZfileHandles;                                       /* 0x00670678 */
extern ZMemPool *gZfileLoadPool;                                          /* 0x0067067c */
extern ZMemInUseTable gZmemFreeListInUse;                                 /* 0x00670680 */
extern ZMemState gZmem;                                                   /* 0x00670880 */
extern ZMemFreeListTable gZmemFreeLists;                                  /* 0x006708a8 */
extern ZMemHooks gZmemHooks;                                              /* 0x006718a8 */
extern unsigned char d_006718b0[16];                                      /* 0x006718b0 */
extern unsigned char d_006718c0[2544];                                    /* 0x006718c0 */
extern int gZmemHooksInstalled;                                           /* 0x006722b0 */
extern ZrwList gZpropDefs;                        /* 0x006722b8, also gDeferredB, gZobjTreeObjects */
extern ZrwList gZpropInsts;                       /* 0x006722c4, also gDeferredC, gZobjProps */
extern int gZobjInitialized;                      /* 0x006722d0 */
extern ZMemFreeList *gZobjPool;                   /* 0x006722d4 */
extern ZMemFreeList *gZobjLinkPool;               /* 0x006722d8 */
extern ZrwUpdateNode *gZobjDebugNode;             /* 0x006722dc */
extern ZrwUpdateNode *gZobjUpdateNode;            /* 0x006722e0 */
extern ZrwModelState gZrwModelState;              /* 0x006722e8 */
extern ZVec3 gZrwOrigin;                          /* 0x006722f8 */
extern int gZobjModelReady;                       /* 0x00672308 */
extern int gZrwModelFlag2;                        /* 0x0067230c */
extern unsigned char d_00672310[8];               /* 0x00672310 */
extern struct BmxGroup_gZrwMatGroup gZrwMatGroup; /* 0x00672318 */
extern BmxType_gEpicMessage gEpicMessage;         /* 0x006723d0 */
extern struct BmxGroup_gZglSubSystemNamesGroup gZglSubSystemNamesGroup; /* 0x006724d0 */
extern int gZglSubSystemCount;                                          /* 0x00672750 */
extern int gZglSubSystem;                                               /* 0x00672754 */
extern uint32_t gZglVideoMode;                                          /* 0x00672758 */
extern uint32_t gLastTick;                                              /* 0x0067275c */
extern int gZrwInitialized;                                             /* 0x00672760 */
extern char *gZrwArgument;                                              /* 0x00672764 */
extern ZrwUpdateListTable gZrwUpdateLists;                              /* 0x00672768, also gLists */
extern ZrwStateStack gZrwStateStack;                                    /* 0x006727d4 */
extern int gZrwStateDepth;                                              /* 0x006727f4 */
extern ZMemFreeList *gZrwNodePool;                                      /* 0x006727f8 */
extern int gZrwSkipDevice;                                              /* 0x006727fc */
extern void *gZrwEngineOpen;                                            /* 0x00672800 */
extern uint32_t gZrwEngineInit;                                         /* 0x00672804 */
extern struct BmxGroup_gZrwArgBufferGroup gZrwArgBufferGroup;           /* 0x00672808 */
extern int gNoMovies;                                                   /* 0x0067288c, also gMovieSkip */
extern int gZrwPrimFrame;                                               /* 0x00672890 */
extern ZrwPrimState gZrwPrim;                                           /* 0x00672898 */
extern float gZrwPrimDepthBias;                                         /* 0x006728b0 */
extern uint32_t gZtaginfUnused;                                         /* 0x006728b8 */
extern uint32_t gZtaginfFaceCount;                                      /* 0x006728bc */
extern struct ZbspLevelFace *gZtaginfFaces;                             /* 0x006728c0 */
extern struct BmxGroup_gZtaginfRootGroup gZtaginfRootGroup;             /* 0x006728c4 */
extern int gZrwColDebug;                                                /* 0x00672900, also gZtaginfUnused2 */
extern BmxType_gZrwColList gZrwColList;                                 /* 0x00672908 */
extern ZrwRenderColHit gZrwColHit;                                      /* 0x00673708 */
extern unsigned char d_00673740[4];                                     /* 0x00673740 */
extern unsigned char d_00673744[4];                                     /* 0x00673744 */
extern unsigned char d_00673748[4];                                     /* 0x00673748 */
extern unsigned char d_0067374c[40];                                    /* 0x0067374c */
extern unsigned char d_00673774[4];                                     /* 0x00673774 */
extern unsigned char d_00673778[4];                                     /* 0x00673778 */
extern unsigned char d_0067377c[4];                                     /* 0x0067377c */
extern unsigned char d_00673780[4];                                     /* 0x00673780 */
extern unsigned char d_00673784[8];                                     /* 0x00673784 */
extern unsigned char d_0067378c[4];                                     /* 0x0067378c */
extern unsigned char d_00673790[4];                                     /* 0x00673790 */
extern unsigned char d_00673794[24];                                    /* 0x00673794 */
extern unsigned char d_006737ac[4];                                     /* 0x006737ac */
extern ZrwRenderColQuery gZrwColQuery;                                  /* 0x006737b0 */
extern ZMemFreeList *gZtaginfNodes;                                     /* 0x006737b8 */
extern unsigned char d_006737c0[4];                                     /* 0x006737c0 */
extern unsigned char d_006737c4[1724];                                  /* 0x006737c4 */
extern ZrwModelRenderState gZrwModelRender;                             /* 0x00673e80 */
extern unsigned char d_00673ea8[2];                                     /* 0x00673ea8 */
extern unsigned char d_00673eaa[190];                                   /* 0x00673eaa */
extern int gZobjLightInitialized;                                       /* 0x00673f68 */
extern ZsceneState gScene;                                              /* 0x00673f70 */
extern ZscnptsState gScnpts;                                            /* 0x00674000, also gZrwMarkers */
extern int gZrwCamReady;                                                /* 0x00674018 */
extern ZMemFreeList *gZrwCamPool;                                       /* 0x0067401c */
extern ZrwCamera *gZrwCurrentCamera;                                    /* 0x00674020 */
extern int gZpadDiActive;                                               /* 0x00674028 */
extern ZMemFreeList *gZpadDiRecords;                                    /* 0x0067402c */
extern uint16_t gZpadKeyboardOk;                                        /* 0x00674030 */
extern void *gZpadWindow;                                               /* 0x00674034 */
extern ZpadDirectInput *gZpadDirectInput;                               /* 0x00674038 */
extern ZpadGuid gZpadJoystickGuid;                                      /* 0x0067403c */
extern ZpadDiDevice *gZpadKeyboard;                                     /* 0x0067404c */
extern uint32_t gZpadKeyboardFlags;                                     /* 0x00674050 */
extern ZpadKeyState gZpadKeyState;                                      /* 0x00674054 */
extern ZrwColColor gZrwColColor;                                        /* 0x00674158 */
extern ZrwColState gZrwCol;                                             /* 0x00674160 */
extern uint32_t gZrwColEnabled;                                         /* 0x006741b0 */
extern int gZobjPEmitInitialized;                                       /* 0x006741b4 */
extern ZrwWorldRecord gZrwWorldRecord;                                  /* 0x006741b8 */
extern struct BmxGroup_bmxZrwWorldState bmxZrwWorldState;               /* 0x006741e0 */
extern ZrwTextureState gZrwTxr;                                         /* 0x00674290 */
extern int gZanimInitialized;                                           /* 0x006746c0 */
extern ZSkeletonItem *gZiffItemPool;                                    /* 0x006746c4 */
extern void *gZiffAnim;                                                 /* 0x006746c8 */
extern ZiffState gZiff;                                                 /* 0x006746d0 */
extern ZrwShadowPointList gZrwShadowPoints;                             /* 0x006746f0 */
extern ZrwShadowQuadList gZrwShadowQuads;                               /* 0x00674930 */
extern uint32_t gZrwShadowActive;                                       /* 0x00674b28 */
extern ZrwShadowPointList gZrwShadowOffsets;                            /* 0x00674b34 */
extern ZrwShadowVertexList gZrwShadowVertices;                          /* 0x00674d74 */
extern ZrwShadowMissList gZrwShadowMiss;                                /* 0x00675164 */
extern uint32_t gZrwShadowTexture;                                      /* 0x006751f4 */
extern struct ZrwUpdateNode *gZrwShadowNode;                            /* 0x006751f8 */
extern uint32_t gZrwShadowReady;                                        /* 0x006751fc */
extern float gZrwShadowScale;                                           /* 0x00675200 */
extern float gZrwShadowDouble;                                          /* 0x00675204 */
extern uint32_t gZrwShadowInit;                                         /* 0x0067520c */
extern unsigned char d_00675210[4];                                     /* 0x00675210 */
extern unsigned char d_00675214[4];                                     /* 0x00675214 */
extern ZrwModelCreateState gZrwModelCreate;                             /* 0x00675218 */
extern unsigned char d_0067523c[4];                                     /* 0x0067523c */
extern BmxType_gZrwNameBuffer gZrwNameBuffer;                           /* 0x00675240 */
extern ZrwFileOps gZrwFileOps;                                          /* 0x006752c0 */
extern int gZrwFileInitialized;                                         /* 0x006752e8 */
extern ZramdiskNameBuffer gZramdiskName;                                /* 0x006752ec */
extern ZMemFreeList *gZramdiskFiles;                                    /* 0x0067536c */
extern int gZrwLightInitialized;                                        /* 0x00675370 */
extern ZMemFreeList *gZrwLightPool;                                     /* 0x00675374 */
extern int gZgeInitialized;                                             /* 0x00675378 */
extern void *gPanorama;                                                 /* 0x0067537c */
extern ZtokenState gToken;                                              /* 0x00675380 */
extern ZFontState gFont;                                                /* 0x006763d0 */
extern unsigned char d_006763e0[4];                                     /* 0x006763e0 */
extern unsigned char d_006763e4[4];                                     /* 0x006763e4 */
extern ZfileHandle *gZrwRamDiskFile;                                    /* 0x006763e8 */
extern ZrwRamDiskInfoSlots gZrwRamDiskInfoSlots;                        /* 0x006763ec */
extern ZrwRamDiskInfo *gZrwRamDiskNoInfo;                               /* 0x00676404 */
extern ZrwRamDiskInfo *gZrwRamDiskInfo;                                 /* 0x00676408 */
extern ZrwFileInterface gZrwSavedFileInterface;                         /* 0x00676410 */
extern ZProfileState gZProfile;                                         /* 0x00676438 */
extern uint16_t gZtimerSlots;                                           /* 0x006764a0 */
extern ZtimerHeap gZtimerHeap;                                          /* 0x006764a4 */
extern ZrwList gZtimerFree;                                             /* 0x006764b4 */
extern ZrwUpdateNode *gZtimerUpdate;                                    /* 0x006764c0 */
extern uint32_t gZtimerReady;                                           /* 0x006764c4 */
extern unsigned char d_006764c8[8];                                     /* 0x006764c8 */
extern PcProfileClock gPcProfileClock;                                  /* 0x006764d0 */
extern ZtimerHeapEntry gZtimerPopped;                                   /* 0x006764e0 */
extern unsigned char d_006764e8[4];                                     /* 0x006764e8 */
extern uint32_t gCrtFpStatus;                                           /* 0x006764ec */
extern unsigned char d_006764f0[8];                                     /* 0x006764f0 */
extern unsigned char d_006764f8[4];                                     /* 0x006764f8 */
extern unsigned char d_006764fc[4];                                     /* 0x006764fc */
extern unsigned char d_00676500[4];                                     /* 0x00676500 */
extern unsigned char d_00676504[4];                                     /* 0x00676504 */
extern unsigned char d_00676508[4];                                     /* 0x00676508 */
extern unsigned char d_0067650c[4];                                     /* 0x0067650c */
extern unsigned char d_00676510[4];                                     /* 0x00676510 */
extern unsigned char d_00676514[4];                                     /* 0x00676514 */
extern unsigned char d_00676518[4];                                     /* 0x00676518 */
extern unsigned char d_0067651c[8];                                     /* 0x0067651c */
extern unsigned char d_00676524[4];                                     /* 0x00676524 */
extern unsigned char d_00676528[4];                                     /* 0x00676528 */
extern unsigned char d_0067652c[8];                                     /* 0x0067652c */
extern unsigned char d_00676534[8];                                     /* 0x00676534 */
extern unsigned char d_0067653c[4];                                     /* 0x0067653c */
extern unsigned char d_00676540[4];                                     /* 0x00676540 */
extern unsigned char d_00676544[4];                                     /* 0x00676544 */
extern unsigned char d_00676548[4];                                     /* 0x00676548 */
extern unsigned char d_0067654c[4];                                     /* 0x0067654c */
extern unsigned char d_00676550[4];                                     /* 0x00676550 */
extern unsigned char d_00676554[4];                                     /* 0x00676554 */
extern unsigned char d_00676558[4];                                     /* 0x00676558 */
extern unsigned char d_0067655c[4];                                     /* 0x0067655c */
extern unsigned char d_00676560[4];                                     /* 0x00676560 */
extern unsigned char d_00676564[4];                                     /* 0x00676564 */
extern unsigned char d_00676568[4];                                     /* 0x00676568 */
extern unsigned char d_0067656c[4];                                     /* 0x0067656c */
extern unsigned char d_00676570[4];                                     /* 0x00676570 */
extern unsigned char d_00676574[4];                                     /* 0x00676574 */
extern unsigned char d_00676578[4];                                     /* 0x00676578 */
extern unsigned char d_0067657c[4];                                     /* 0x0067657c */
extern unsigned char d_00676580[4];                                     /* 0x00676580 */
extern unsigned char d_00676584[4];                                     /* 0x00676584 */
extern unsigned char d_00676588[8];                                     /* 0x00676588 */
extern unsigned char d_00676590[4];                                     /* 0x00676590 */
extern unsigned char d_00676594[260];                                   /* 0x00676594 */
extern unsigned char d_00676698[4];                                     /* 0x00676698 */
extern unsigned char d_0067669c[4];                                     /* 0x0067669c */
extern unsigned char d_006766a0[4];                                     /* 0x006766a0 */
extern unsigned char d_006766a4[8];                                     /* 0x006766a4 */
extern unsigned char d_006766ac[16];                                    /* 0x006766ac */
extern unsigned char d_006766bc[12];                                    /* 0x006766bc */
extern unsigned char d_006766c8[2];                                     /* 0x006766c8 */
extern unsigned char d_006766ca[2];                                     /* 0x006766ca */
extern unsigned char d_006766cc[28];                                    /* 0x006766cc */
extern unsigned char d_006766e8[4];                                     /* 0x006766e8 */
extern unsigned char d_006766ec[4];                                     /* 0x006766ec */
extern unsigned char d_006766f0[4];                                     /* 0x006766f0 */
extern unsigned char d_006766f4[4];                                     /* 0x006766f4 */
extern unsigned char d_006766f8[4];                                     /* 0x006766f8 */
extern unsigned char d_006766fc[4];                                     /* 0x006766fc */
extern unsigned char d_00676700[4];                                     /* 0x00676700 */
extern unsigned char d_00676704[12];                                    /* 0x00676704 */
extern int32_t rpClumpModuleOffset;                                     /* 0x00676710 */
extern int32_t rpClumpModuleCount;                                      /* 0x00676714 */
extern int32_t rpGeometryOffset;                                        /* 0x00676720 */
extern int32_t rpGeometryModuleCount;                                   /* 0x00676724 */
extern int32_t rpTieOffset;                                             /* 0x00676730 */
extern int32_t rpTieModuleCount;                                        /* 0x00676734 */
extern int32_t rpWorldCameraOffset;                                     /* 0x00676738 */
extern int32_t rpWorldAtomicOffset;                                     /* 0x0067673c */
extern int32_t rpWorldClumpOffset;                                      /* 0x00676740 */
extern int32_t rpWorldLightOffset;                                      /* 0x00676744 */
extern int32_t rpMaterialOffset;                                        /* 0x00676750 */
extern int32_t rpMaterialModuleCount;                                   /* 0x00676754 */
extern int32_t rpWorldGlobalsOffset;                                    /* 0x00676760 */
extern int32_t rpWorldModuleCount;                                      /* 0x00676764 */
extern struct BmxGroup_rpWorldDefaultSurfacePropsGroup rpWorldDefaultSurfacePropsGroup; /* 0x00676768 */
extern int32_t rpBspModuleCount;                                                        /* 0x00676784 */
extern int32_t rpSectorModuleCount;                                                     /* 0x00676794 */
extern int32_t rpLightOffset;                                                           /* 0x006767a0 */
extern int32_t rpLightModuleCount;                                                      /* 0x006767a4 */
extern int32_t rpBinaryWorldModuleCount;                                                /* 0x006767b4 */
extern RpMeshHeader rpMeshEmptyHeader;                                                  /* 0x006767c0 */
extern struct BmxGroup_rpMeshStripModeGroup rpMeshStripModeGroup;                       /* 0x006767d0 */
extern int32_t rpMaterialListModuleCount;                                               /* 0x006767e4 */
extern unsigned char d_006767f0[8];                                                     /* 0x006767f0 */
extern unsigned char d_006767f8[8];                                                     /* 0x006767f8 */
extern unsigned char d_00676800[8];                                                     /* 0x00676800 */
extern unsigned char d_00676808[8];                                                     /* 0x00676808 */
extern RpWorld *rpPVSWorld;                                                             /* 0x00676810 */
extern RwCamera *rpPVSCamera;                                   /* 0x00676814, also rwPvsCamera */
extern RwImage *rpPVSImage;                                     /* 0x00676818, also rwPvsImage */
extern RwGlobals RwDefaultGlobals;                              /* 0x00676820 */
extern int32_t rwEngineInstanceCount;                           /* 0x00676968 */
extern RwGlobals *RwEngineInstance;                             /* 0x0067696c, also gZrwStringApi */
extern int32_t rwMatrixOffset;                                  /* 0x00676970, also rwMatrixGlobalsOffset */
extern int32_t rwMatrixModuleCount;                             /* 0x00676974 */
extern int32_t rwFrameOffset;                                   /* 0x00676980 */
extern int32_t rwFrameModuleCount;                              /* 0x00676984 */
extern int32_t rwFastSqrtOffset;                                /* 0x00676990 */
extern int32_t rwVectorModuleCount;                             /* 0x00676994 */
extern int32_t rwCameraFpuSet;                                  /* 0x006769a0 */
extern unsigned char d_006769a4[256];                           /* 0x006769a4 */
extern unsigned char d_00676aa4[256];                           /* 0x00676aa4 */
extern int32_t rwD3DFlipPending;                                /* 0x00676ba4 */
extern unsigned char d_00676ba8[4];                             /* 0x00676ba8 */
extern RwD3DDeferList *rwD3DDefer;                              /* 0x00676bac */
extern uint32_t rwD3DSortNeed;                                  /* 0x00676bb0 */
extern uint32_t rwD3DSortCap;                                   /* 0x00676bb4 */
extern uint32_t rwD3DSortCount;                                 /* 0x00676bb8 */
extern RwD3DSortItem *rwD3DSortItems;                           /* 0x00676bbc */
extern unsigned char d_00676bc0[4];                             /* 0x00676bc0 */
extern unsigned char d_00676bc4[256];                           /* 0x00676bc4 */
extern int32_t rwD3DAlphaTestCaps;                              /* 0x00676cc4 */
extern uint32_t rwD3DMergeCap;                                  /* 0x00676cc8 */
extern uint16_t *rwD3DMergeBuf;                                 /* 0x00676ccc */
extern int32_t rwD3DRemapCap;                                   /* 0x00676cd0 */
extern uint16_t *rwD3DRemap;                                    /* 0x00676cd4 */
extern uint16_t *rwD3DRemapUsed;                                /* 0x00676cd8 */
extern RwDevice *rwD3DEngineDevice;                             /* 0x00676cdc */
extern RwLLLink rwFreeListHead;                                 /* 0x00676ce0 */
extern int32_t rwResourcesOffset;                               /* 0x00676cf0 */
extern int32_t rwResourcesModuleCount;                          /* 0x00676cf4 */
extern int32_t rwRasterOffset;                                  /* 0x00676d00 */
extern int32_t rwRasterModuleCount;                             /* 0x00676d04 */
extern int32_t rwCameraOffset;                                  /* 0x00676d10 */
extern int32_t rwCameraModuleCount;                             /* 0x00676d14 */
extern int32_t rwErrorOffset;                                   /* 0x00676d20 */
extern int32_t rwErrorModuleCount;                              /* 0x00676d24 */
extern int32_t rwIm3DPipelineOffset;                            /* 0x00676d30 */
extern int32_t rwIm3DModuleCount;                               /* 0x00676d34 */
extern int32_t rwTextureOffset;                                 /* 0x00676d40 */
extern int32_t rwTextureModuleCount;                            /* 0x00676d44 */
extern int32_t rwStreamOffset;                                  /* 0x00676d50 */
extern int32_t rwStreamModuleCount;                             /* 0x00676d54 */
extern int32_t rwImageOffset;                                   /* 0x00676d60 */
extern int32_t rwImageModuleCount;                              /* 0x00676d64 */
extern int32_t rwIm3DOffset;                                    /* 0x00676d70 */
extern int32_t rwRenderPipelineModuleCount;                     /* 0x00676d74 */
extern RwFreeList *rwPluginRegEntryFreeList;                    /* 0x00676d80 */
extern struct BmxGroup_rwEmptyStringGroup rwEmptyStringGroup;   /* 0x00676d90 */
extern int32_t rwColorModuleCount;                              /* 0x00676da4 */
extern unsigned char d_00676db0[4];                             /* 0x00676db0 */
extern unsigned char d_00676db4[4];                             /* 0x00676db4 */
extern unsigned char d_00676db8[4];                             /* 0x00676db8 */
extern unsigned char d_00676dbc[4];                             /* 0x00676dbc */
extern unsigned char d_00676dc0[4];                             /* 0x00676dc0 */
extern unsigned char d_00676dc4[4];                             /* 0x00676dc4 */
extern unsigned char d_00676dc8[4];                             /* 0x00676dc8 */
extern unsigned char d_00676dcc[28];                            /* 0x00676dcc */
extern unsigned char d_00676de8[4012];                          /* 0x00676de8 */
extern unsigned char d_00677d94[4];                             /* 0x00677d94 */
extern unsigned char d_00677d98[4];                             /* 0x00677d98 */
extern unsigned char d_00677d9c[4];                             /* 0x00677d9c */
extern unsigned char d_00677da0[4];                             /* 0x00677da0 */
extern unsigned char d_00677da4[4];                             /* 0x00677da4 */
extern unsigned char d_00677da8[4];                             /* 0x00677da8 */
extern unsigned char d_00677dac[4];                             /* 0x00677dac */
extern unsigned char d_00677db0[4];                             /* 0x00677db0 */
extern unsigned char d_00677db4[4];                             /* 0x00677db4 */
extern unsigned char d_00677db8[4];                             /* 0x00677db8 */
extern unsigned char d_00677dbc[4];                             /* 0x00677dbc */
extern unsigned char d_00677dc0[4];                             /* 0x00677dc0 */
extern unsigned char d_00677dc4[4];                             /* 0x00677dc4 */
extern unsigned char d_00677dc8[4];                             /* 0x00677dc8 */
extern unsigned char d_00677dcc[4];                             /* 0x00677dcc */
extern unsigned char d_00677dd0[4];                             /* 0x00677dd0 */
extern unsigned char d_00677dd4[4];                             /* 0x00677dd4 */
extern unsigned char d_00677dd8[4];                             /* 0x00677dd8 */
extern unsigned char d_00677ddc[4];                             /* 0x00677ddc */
extern unsigned char d_00677de0[4];                             /* 0x00677de0 */
extern unsigned char d_00677de4[4];                             /* 0x00677de4 */
extern unsigned char d_00677de8[4];                             /* 0x00677de8 */
extern unsigned char d_00677dec[4];                             /* 0x00677dec */
extern unsigned char d_00677df0[4];                             /* 0x00677df0 */
extern unsigned char d_00677df4[12];                            /* 0x00677df4 */
extern unsigned char d_00677e00[4];                             /* 0x00677e00 */
extern unsigned char d_00677e04[4];                             /* 0x00677e04 */
extern unsigned char d_00677e08[4];                             /* 0x00677e08 */
extern unsigned char d_00677e0c[4];                             /* 0x00677e0c */
extern unsigned char d_00677e10[4];                             /* 0x00677e10 */
extern unsigned char d_00677e14[4];                             /* 0x00677e14 */
extern unsigned char d_00677e18[4];                             /* 0x00677e18 */
extern unsigned char d_00677e1c[4];                             /* 0x00677e1c */
extern unsigned char d_00677e20[4];                             /* 0x00677e20 */
extern unsigned char d_00677e24[4];                             /* 0x00677e24 */
extern unsigned char d_00677e28[4];                             /* 0x00677e28 */
extern unsigned char d_00677e2c[4];                             /* 0x00677e2c */
extern unsigned char d_00677e30[4];                             /* 0x00677e30 */
extern unsigned char d_00677e34[4];                             /* 0x00677e34 */
extern unsigned char d_00677e38[4];                             /* 0x00677e38 */
extern unsigned char d_00677e3c[4];                             /* 0x00677e3c */
extern unsigned char d_00677e40[16];                            /* 0x00677e40 */
extern unsigned char d_00677e50[1024];                          /* 0x00677e50 */
extern unsigned char d_00678250[4];                             /* 0x00678250 */
extern unsigned char d_00678254[4];                             /* 0x00678254 */
extern unsigned char d_00678258[4];                             /* 0x00678258 */
extern unsigned char d_0067825c[4];                             /* 0x0067825c */
extern unsigned char d_00678260[4];                             /* 0x00678260 */
extern unsigned char d_00678264[4];                             /* 0x00678264 */
extern unsigned char d_00678268[8];                             /* 0x00678268 */
extern int32_t rwD3DXformStride;                                /* 0x00678270 */
extern const RwMatrix *rwD3DXformMatrix;                        /* 0x00678274 */
extern RwD3DTLVertex *rwD3DXformOut;                            /* 0x00678278 */
extern int32_t rwD3DXformFog;                                   /* 0x0067827c */
extern int32_t rpAnimEngineOffset;                              /* 0x00678280 */
extern int32_t rpAnimModuleCount;                               /* 0x00678284 */
extern int32_t rpAnimFrameOffset;                               /* 0x00678288 */
extern int32_t rpAnimClumpOffset;                               /* 0x0067828c */
extern int32_t rpBoneFrameOffset;                               /* 0x00678290 */
extern unsigned char d_00678294[4];                             /* 0x00678294 */
extern int32_t rpBoneEngineOffset;                              /* 0x00678298 */
extern int32_t rpBoneModuleCount;                               /* 0x0067829c */
extern int32_t rpBoneStreamResult;                              /* 0x006782a0 */
extern int32_t rpBoneGeometryOffset;                            /* 0x006782a4 */
extern unsigned char d_006782b0[8];                             /* 0x006782b0 */
extern unsigned char d_006782b8[8];                             /* 0x006782b8 */
extern unsigned char d_006782c0[2];                             /* 0x006782c0 */
extern unsigned char d_006782c2[4];                             /* 0x006782c2 */
extern unsigned char d_006782c6[2];                             /* 0x006782c6 */
extern unsigned char d_006782c8[2];                             /* 0x006782c8 */
extern unsigned char d_006782ca[6];                             /* 0x006782ca */
extern unsigned char d_006782d0[3];                             /* 0x006782d0 */
extern unsigned char d_006782d3[29];                            /* 0x006782d3 */
extern unsigned char d_006782f0[8];                             /* 0x006782f0 */
extern unsigned char d_006782f8[4];                             /* 0x006782f8 */
extern unsigned char d_006782fc[64];                            /* 0x006782fc */
extern unsigned char d_0067833c[2];                             /* 0x0067833c */
extern unsigned char d_0067833e[2];                             /* 0x0067833e */
extern unsigned char d_00678340[2];                             /* 0x00678340 */
extern unsigned char d_00678342[2];                             /* 0x00678342 */
extern unsigned char d_00678344[2];                             /* 0x00678344 */
extern unsigned char d_00678346[2];                             /* 0x00678346 */
extern unsigned char d_00678348[2];                             /* 0x00678348 */
extern unsigned char d_0067834a[2];                             /* 0x0067834a */
extern unsigned char d_0067834c[4];                             /* 0x0067834c */
extern unsigned char d_00678350[64];                            /* 0x00678350 */
extern unsigned char d_00678390[2];                             /* 0x00678390 */
extern unsigned char d_00678392[2];                             /* 0x00678392 */
extern unsigned char d_00678394[2];                             /* 0x00678394 */
extern unsigned char d_00678396[2];                             /* 0x00678396 */
extern unsigned char d_00678398[2];                             /* 0x00678398 */
extern unsigned char d_0067839a[2];                             /* 0x0067839a */
extern unsigned char d_0067839c[2];                             /* 0x0067839c */
extern unsigned char d_0067839e[2];                             /* 0x0067839e */
extern unsigned char d_006783a0[4];                             /* 0x006783a0 */
extern unsigned char d_006783a4[4];                             /* 0x006783a4 */
extern unsigned char d_006783a8[8];                             /* 0x006783a8 */
extern unsigned char d_006783b0[4];                             /* 0x006783b0 */
extern unsigned char d_006783b4[4];                             /* 0x006783b4 */
extern unsigned char d_006783b8[4];                             /* 0x006783b8 */
extern unsigned char d_006783bc[4];                             /* 0x006783bc */
extern unsigned char d_006783c0[4];                             /* 0x006783c0 */
extern unsigned char d_006783c4[4];                             /* 0x006783c4 */
extern unsigned char d_006783c8[4];                             /* 0x006783c8 */
extern unsigned char d_006783cc[4];                             /* 0x006783cc */
extern unsigned char d_006783d0[4];                             /* 0x006783d0 */
extern unsigned char d_006783d4[40];                            /* 0x006783d4 */
extern int32_t gZprimHandle;                                    /* 0x006783fc */
extern struct ZassetLibrary *gZprimLibrary;                     /* 0x00678400 */
extern ZPrimAsset *gZprimPool;                                  /* 0x00678404 */
extern ZPrimAsset *gZprimFree;                                  /* 0x00678408 */
extern ZPrimAsset *gZprimUsed;                                  /* 0x0067840c */
extern ZPrimRect gZprimClip;                                    /* 0x00678410 */
extern ZPrimGroupTable gZprimGroups;                            /* 0x00678418 */
extern uint16_t gZprimNextId;                                   /* 0x00678428 */
extern int32_t gZprimDepth;                                     /* 0x0067842c */
extern ZPrimState *gZprimStates;                                /* 0x00678430 */
extern struct BmxGroup_gZrollerShadowGroup gZrollerShadowGroup; /* 0x00678440 */
extern struct ZassetLibrary *gZresTablesLib;                    /* 0x0067847c */
extern struct ZassetLibrary *gZresMaterialLib;                  /* 0x0067848c */
extern struct ZassetLibrary *gZresMiscLib; /* 0x00678490, also gAssetLibrary, gOvlLibrary */
extern struct BmxGroup_gZrollerEdgesGroup gZrollerEdgesGroup;         /* 0x00678494 */
extern struct BmxGroup_gNgifPrim3DBufferGroup gNgifPrim3DBufferGroup; /* 0x00678560 */
extern ZrollerModelTable
    gZrollerModels;                    /* 0x0068c3e0, also gNgifTextures, gZoverSprites, gZresOverlaySlots */
extern uint32_t *gNgifPrim3D;          /* 0x0068ebe0 */
extern unsigned char d_0068ebf0[16];   /* 0x0068ebf0 */
extern unsigned char d_0068ec00[4096]; /* 0x0068ec00 */
extern struct BmxGroup_gNgifColorGroup gNgifColorGroup;           /* 0x0068fc00 */
extern struct BmxGroup_gFxDustTexturesGroup gFxDustTexturesGroup; /* 0x0068fc20 */
extern void *gFxAnimHandle;                                       /* 0x0068fcf8 */
extern void *gNgifScriptNode;                                     /* 0x0068fcfc */
extern ZfxSkidTable gFxSkids;                                     /* 0x0068fd00 */
extern uint8_t *gFxWorldObject;                                   /* 0x0068ff80 */
extern uint32_t gNgWord68ff84;                                    /* 0x0068ff84 */
extern struct ZMemFreeList *gZprimFontPageList; /* 0x0068ff94, also gNgFontPages, gNgifGlyphPages */
extern ZObjCamera *gNgCamera;                   /* 0x0068ff98, also gCameraObject, gFxSceneObject */
extern uint8_t gNgFlags68ffa0;                  /* 0x0068ffa0 */
extern uint8_t gNgFlags68ffa1;                  /* 0x0068ffa1 */
extern uint8_t gNgFlags68ffa2;                  /* 0x0068ffa2 */
extern uint8_t gNgFlags68ffa3;                  /* 0x0068ffa3 */
extern ZrwUpdateNode *gNgTickNode;              /* 0x0068ffa4 */
extern struct BmxGroup_gZresReplayLibGroup gZresReplayLibGroup;       /* 0x0068ffa8 */
extern struct BmxGroup_gZoverSweepGroup gZoverSweepGroup;             /* 0x0068ffc0 */
extern struct BmxGroup_gExtrasTitlesGroup gExtrasTitlesGroup;         /* 0x0068fff0 */
extern HiscoreColumn gHiscoreTimes;                                   /* 0x006904d0 */
extern uint32_t gAssetSetSize;                                        /* 0x00690890 */
extern uint16_t gHiscoreWait;                                         /* 0x00690894 */
extern uint16_t gExtrasWait2;                                         /* 0x00690896 */
extern struct BmxGroup_gInfoWaitGroup gInfoWaitGroup;                 /* 0x00690898 */
extern HiscoreColumn gHiscoreLabels;                                  /* 0x00690990 */
extern unsigned char d_00690d54[252];                                 /* 0x00690d54 */
extern HiscoreColumn gHiscoreRiders;                                  /* 0x00690e50 */
extern ZPrimAsset *gInfoSprite;                                       /* 0x00691210 */
extern int gExtrasChoice;                                             /* 0x00691214 */
extern uint16_t gInfoPage;                                            /* 0x00691218 */
extern SlideTable gInfoSlide;                                         /* 0x00691220 */
extern int gExtrasReady;                                              /* 0x00691228 */
extern uint16_t gInfoTicks;                                           /* 0x0069122c */
extern SlideTable gExtrasSlide;                                       /* 0x00691230 */
extern int gHiscoreHandle;                                            /* 0x00691238 */
extern struct BmxGroup_gHiscoreSpriteGroup gHiscoreSpriteGroup;       /* 0x0069123c */
extern HiscoreColumn gHiscoreNames;                                   /* 0x00691330 */
extern uint16_t gExtrasCount;                                         /* 0x006916f0 */
extern uint16_t gHiscoreBlink;                                        /* 0x006916f2 */
extern SlideTable gHiscoreSlide;                                      /* 0x006916f8 */
extern uint16_t gExtrasWait;                                          /* 0x00691700 */
extern struct BmxGroup_gSelTickGroup gSelTickGroup;                   /* 0x00691704 */
extern uint8_t gSelBikeCount;                                         /* 0x0069181f */
extern SelSlideTable gSelSlide;                                       /* 0x00691820 */
extern int gSelReadyTick;                                             /* 0x0069182c */
extern int gSelPage;                                                  /* 0x00691830 */
extern uint16_t gSelExtentA;                                          /* 0x00691834 */
extern uint16_t gSelExtentB;                                          /* 0x00691836 */
extern int gSelSlideDir;                                              /* 0x00691838 */
extern uint8_t gSelStyleCount;                                        /* 0x0069183c */
extern CaptionSlotTable gCaptionSlots;                                /* 0x00691840 */
extern SelPlayerInts gSelRider;                                       /* 0x00691ca0 */
extern int gSelPlayer;                                                /* 0x00691ca8 */
extern const SelCaption *gCaptionPtr;                                 /* 0x00691cac */
extern int gSelStage;                                                 /* 0x00691cb0 */
extern CheatRing gCheatRing;                                          /* 0x00691cc0 */
extern uint16_t gCaptionOn;                                           /* 0x00691ce0 */
extern int gSelPages;                                                 /* 0x00691ce4 */
extern int gSelPicturesOn;                                            /* 0x00691ce8 */
extern uint16_t gSelExtentC;                                          /* 0x00691cec */
extern uint16_t gSelDimA;                                             /* 0x00691cee */
extern uint16_t gCheatPos;                                            /* 0x00691cf0 */
extern int gSelFlipTick;                                              /* 0x00691cf4 */
extern SelSpriteTable gSelSprites;                                    /* 0x00691cf8 */
extern uint16_t gSelDimB;                                             /* 0x00691d04 */
extern uint16_t gCaptionLine;                                         /* 0x00691d06 */
extern struct BmxGroup_gSelLevelCountGroup gSelLevelCountGroup;       /* 0x00691d08 */
extern SelPlayerInts gSelStyle;                                       /* 0x00691d58 */
extern struct BmxGroup_gSelUnknown60Group gSelUnknown60Group;         /* 0x00691d60 */
extern CaptionColors gCaptionColors;                                  /* 0x00691d80 */
extern int gSelLevel;                                                 /* 0x00691d94 */
extern SelPlayerInts gSelBike;                                        /* 0x00691d98 */
extern uint32_t gSelUnknownA0;                                        /* 0x00691da0 */
extern uint16_t gCaptionTime;                                         /* 0x00691da4 */
extern uint16_t gCaptionAlpha;                                        /* 0x00691da6 */
extern CaptionWidths gCaptionWidths;                                  /* 0x00691db0 */
extern uint16_t gCaptionHold;                                         /* 0x00691dba */
extern int gSelPending;                                               /* 0x00691dbc */
extern ScreenReturnFn gScreenReturnFn;                                /* 0x00691dc0 */
extern uint16_t gSlotSel;                                             /* 0x00691dce */
extern uint32_t gScreenReturnArg;                                     /* 0x00691dd0 */
extern struct BmxGroup_gSoundLevel0Group gSoundLevel0Group;           /* 0x00691dd4 */
extern int gOptMainReady;                                             /* 0x00691df8 */
extern int gSoundSlideA;                                              /* 0x00691e00 */
extern int gSoundSlideB;                                              /* 0x00691e04 */
extern int gKeyPlayer;                                                /* 0x00691e08 */
extern uint16_t gOptCount;                                            /* 0x00691e10 */
extern int gOptSaveDelay;                                             /* 0x00691e14 */
extern int gOptTicks;                                                 /* 0x00691e1c */
extern struct BmxGroup_gSoundSelGroup gSoundSelGroup;                 /* 0x00691e20 */
extern unsigned char d_00691e3c[4];                                   /* 0x00691e3c */
extern OptionSlideTable gOptSlide;                                    /* 0x00691e40 */
extern int gOptMainSel;                                               /* 0x00691e60 */
extern struct BmxGroup_gSoundLevel1Group gSoundLevel1Group;           /* 0x00691e64 */
extern OptionRowTable gOptRows;                                       /* 0x00691e80 */
extern int gOptSel;                                                   /* 0x00691ea0 */
extern uint16_t gScrollTick;                                          /* 0x00691ea8 */
extern uint16_t gTitleIdle;                                           /* 0x00691eb4 */
extern int gTitleDelay;                                               /* 0x00691eb8 */
extern uint16_t gAutoSlot;                                            /* 0x00691ebc */
extern uint16_t gTitleFrames;                                         /* 0x00691ebe */
extern struct BmxGroup_gMcChoiceGroup gMcChoiceGroup;                 /* 0x00691ec0 */
extern uint16_t gMcLoadOk;                                            /* 0x00691ed4 */
extern int16_t gMcCountdown;                                          /* 0x00691ed6 */
extern uint16_t gMcMode;                                              /* 0x00691ed8 */
extern uint16_t gMcSaveOk;                                            /* 0x00691eda */
extern McRows gMcRows;                                                /* 0x00691ee0 */
extern uint16_t gMcState;                                             /* 0x00691f00 */
extern uint16_t gMcSlot;                                              /* 0x00691f02 */
extern uint16_t gLanguageSel;                                         /* 0x00691f04 */
extern int gMenuSlide3;                                               /* 0x00691f10 */
extern int gMenuSlide2;                                               /* 0x00691f20 */
extern int gMenuSlide1;                                               /* 0x00691f24 */
extern int gMenuSlide0;                                               /* 0x00691f28 */
extern uint32_t gGuiPadRepeatHeld;                                    /* 0x00691f2c */
extern uint32_t gGuiPadRepeatCount;                                   /* 0x00691f3c */
extern struct BmxGroup_gPlayerRecsGroup gPlayerRecsGroup;             /* 0x00691f40 */
extern unsigned char d_00693e88[1496];                                /* 0x00693e88 */
extern struct BmxGroup_gPropRecordsGroup gPropRecordsGroup;           /* 0x00694460 */
extern unsigned char d_00694fa4[4];                                   /* 0x00694fa4 */
extern unsigned char d_00694fa8[576];                                 /* 0x00694fa8 */
extern struct BmxGroup_gPhysDebugTextureGroup gPhysDebugTextureGroup; /* 0x006951e8 */
extern unsigned char d_006951ff[1];                                   /* 0x006951ff */
extern struct BmxGroup_gGameJudgeScoresGroup gGameJudgeScoresGroup;   /* 0x00695200 */
extern GameHorseTricks gGameHorseTricks;                              /* 0x00695260 */
extern uint8_t gGameHorseCount;                                       /* 0x00695290 */
extern struct BmxGroup_bmxSaveProfiles bmxSaveProfiles;               /* 0x006952a0 */
extern struct BmxGroup_bmxSaveOptions bmxSaveOptions;                 /* 0x006955e0 */
extern uint8_t gPlayerCount;          /* 0x0069621c, also gNgPlayerCount, gPhysPlayerCount */
extern unsigned char d_0069621d[216]; /* 0x0069621d */
extern uint8_t gDemoPlayer;           /* 0x006962f5 */
extern struct BmxGroup_gPhysMotionsGroup gPhysMotionsGroup;   /* 0x006962fc */
extern uint32_t gOvlSaving;                                   /* 0x006966dc */
extern int32_t rpMeshOffset;                                  /* 0x006966e8 */
extern int32_t rpMeshModuleCount;                             /* 0x006966ec */
extern int32_t rpPVSSectorOffset;                             /* 0x006966fc */
extern int32_t rpPVSWorldOffset;                              /* 0x0069670c */
extern uint32_t rwD3DOrClip;                                  /* 0x00696710 */
extern struct BmxGroup_rwD3DAndClipGroup rwD3DAndClipGroup;   /* 0x00696720 */
extern struct BmxGroup_rwD3DStateGroup rwD3DStateGroup;       /* 0x00696740 */
extern const uint8_t *rwD3DCaps;                              /* 0x006968f0 */
extern RwD3DTriFunc rwD3DClipTriCull;                         /* 0x00696900 */
extern RwD3DTriFunc rwD3DTriCull;                             /* 0x00696904 */
extern RwD3DTriVoidFunc rwD3DClipTri;                         /* 0x00696908 */
extern RwD3DLineFunc rwD3DClipLine;                           /* 0x0069690c */
extern RwD3DVoidFunc rwD3DIndexStart;                         /* 0x00696910 */
extern RwD3DTriVoidFunc rwD3DIndexTri;                        /* 0x00696914 */
extern RwD3DLineFunc rwD3DIndexLine;                          /* 0x00696918 */
extern RwD3DVoidFunc rwD3DIndexFlush;                         /* 0x0069691c */
extern RwD3DProjectionFunc rwD3DSetProjection;                /* 0x00696920 */
extern RwD3DUVSetFunc rwD3DSetUVStage;                        /* 0x00696924 */
extern float rwD3DZScale;                                     /* 0x00696928 */
extern float rwD3DZShift;                                     /* 0x0069692c */
extern float rwD3DXScale;                                     /* 0x00696930 */
extern float rwD3DYScale;                                     /* 0x00696934 */
extern float rwD3DXOffset;                                    /* 0x00696938 */
extern float rwD3DYOffset;                                    /* 0x0069693c */
extern int32_t rwD3DNearZ;                                    /* 0x00696940 */
extern int32_t rwD3DFarZ;                                     /* 0x00696944 */
extern float rwD3DNearScreenZ;                                /* 0x00696948 */
extern float rwD3DFarScreenZ;                                 /* 0x0069694c */
extern RwV2d rwD3DRecipWindow;                                /* 0x00696950 */
extern RwD3DTLVertex *rwD3DTLBase;                            /* 0x00696958 */
extern RwD3DCamVertex *rwD3DCamBase;                          /* 0x0069695c */
extern int32_t rwIm3DGouraud;                                 /* 0x00696960 */
extern uint32_t rwD3DIm3DDepth;                               /* 0x00696964 */
extern RwD3DIm3DLevel *rwD3DIm3DLevel;                        /* 0x00696968 */
extern unsigned char d_0069696c[12];                          /* 0x0069696c */
extern unsigned char d_00696978[4];                           /* 0x00696978 */
extern unsigned char d_0069697c[4];                           /* 0x0069697c */
extern unsigned char d_00696980[220];                         /* 0x00696980 */
extern uint32_t *rwD3DVertexBits;                             /* 0x00696a5c */
extern uint32_t rwD3DVertexBitsSize;                          /* 0x00696a60 */
extern RwD3DIndexList rwD3DTriIndices;                        /* 0x00696a64 */
extern RwD3DIndexList rwD3DLineIndices;                       /* 0x00696a70 */
extern RwD3DTLVertex *rwD3DTLBuffer;                          /* 0x00696a7c */
extern RwD3DCamVertex *rwD3DCamBuffer;                        /* 0x00696a80 */
extern uint32_t rwD3DBufferSize;                              /* 0x00696a84 */
extern struct BmxGroup_rwD3DIm3DWorkGroup rwD3DIm3DWorkGroup; /* 0x00696a88 */
extern unsigned char d_00696b00[1024];                        /* 0x00696b00 */
extern unsigned char d_00696f00[4];                           /* 0x00696f00 */
extern unsigned char d_00696f04[380];                         /* 0x00696f04 */
extern unsigned char d_00697080[8];                           /* 0x00697080 */
extern unsigned char d_00697088[4];                           /* 0x00697088 */
extern unsigned char d_0069708c[4];                           /* 0x0069708c */
extern unsigned char d_00697090[56];                          /* 0x00697090 */
extern unsigned char d_006970c8[4];                           /* 0x006970c8 */
extern unsigned char d_006970cc[4];                           /* 0x006970cc */
extern unsigned char d_006970d0[4];                           /* 0x006970d0 */
extern unsigned char d_006970d4[4];                           /* 0x006970d4 */
extern unsigned char d_006970d8[4];                           /* 0x006970d8 */
extern unsigned char d_006970dc[4];                           /* 0x006970dc */
extern unsigned char d_006970e0[4];                           /* 0x006970e0 */
extern unsigned char d_006970e4[28];                          /* 0x006970e4 */
extern unsigned char d_00697100[16];                          /* 0x00697100 */
extern unsigned char d_00697110[56];                          /* 0x00697110 */
extern unsigned char d_00697148[4];                           /* 0x00697148 */
extern unsigned char d_0069714c[4];                           /* 0x0069714c */
extern unsigned char d_00697150[4];                           /* 0x00697150 */
extern unsigned char d_00697154[4];                           /* 0x00697154 */
extern unsigned char d_00697158[4];                           /* 0x00697158 */
extern unsigned char d_0069715c[4];                           /* 0x0069715c */
extern unsigned char d_00697160[4];                           /* 0x00697160 */
extern unsigned char d_00697164[28];                          /* 0x00697164 */
extern unsigned char d_00697180[4];                           /* 0x00697180 */
extern unsigned char d_00697184[4];                           /* 0x00697184 */
extern unsigned char d_00697188[4];                           /* 0x00697188 */
extern unsigned char d_0069718c[52];                          /* 0x0069718c */
extern unsigned char d_006971c0[4];                           /* 0x006971c0 */
extern unsigned char d_006971c4[4];                           /* 0x006971c4 */
extern unsigned char d_006971c8[4];                           /* 0x006971c8 */
extern unsigned char d_006971cc[4];                           /* 0x006971cc */
extern unsigned char d_006971d0[4];                           /* 0x006971d0 */
extern unsigned char d_006971d4[4];                           /* 0x006971d4 */
extern unsigned char d_006971d8[4];                           /* 0x006971d8 */
extern unsigned char d_006971dc[4];                           /* 0x006971dc */
extern unsigned char d_006971e0[4];                           /* 0x006971e0 */
extern unsigned char d_006971e4[4];                           /* 0x006971e4 */
extern unsigned char d_006971e8[20];                          /* 0x006971e8 */
extern unsigned char d_006971fc[4];                           /* 0x006971fc */
extern unsigned char d_00697200[4];                           /* 0x00697200 */
extern unsigned char d_00697204[4];                           /* 0x00697204 */
extern unsigned char d_00697208[4];                           /* 0x00697208 */
extern unsigned char d_0069720c[60];                          /* 0x0069720c */
extern unsigned char d_00697248[4];                           /* 0x00697248 */
extern unsigned char d_0069724c[4];                           /* 0x0069724c */
extern unsigned char d_00697250[4];                           /* 0x00697250 */
extern unsigned char d_00697254[4];                           /* 0x00697254 */
extern unsigned char d_00697258[4];                           /* 0x00697258 */
extern unsigned char d_0069725c[4];                           /* 0x0069725c */
extern unsigned char d_00697260[4];                           /* 0x00697260 */
extern unsigned char d_00697264[4];                           /* 0x00697264 */
extern unsigned char d_00697268[24];                          /* 0x00697268 */
extern unsigned char d_00697280[4];                           /* 0x00697280 */
extern unsigned char d_00697284[4];                           /* 0x00697284 */
extern unsigned char d_00697288[4];                           /* 0x00697288 */
extern unsigned char d_0069728c[60];                          /* 0x0069728c */
extern unsigned char d_006972c8[4];                           /* 0x006972c8 */
extern unsigned char d_006972cc[4];                           /* 0x006972cc */
extern unsigned char d_006972d0[4];                           /* 0x006972d0 */
extern unsigned char d_006972d4[4];                           /* 0x006972d4 */
extern unsigned char d_006972d8[4];                           /* 0x006972d8 */
extern unsigned char d_006972dc[4];                           /* 0x006972dc */
extern unsigned char d_006972e0[4];                           /* 0x006972e0 */
extern unsigned char d_006972e4[4];                           /* 0x006972e4 */
extern unsigned char d_006972e8[20];                          /* 0x006972e8 */
extern unsigned char d_006972fc[4];                           /* 0x006972fc */
extern unsigned char d_00697300[4];                           /* 0x00697300 */
extern unsigned char d_00697304[4];                           /* 0x00697304 */
extern unsigned char d_00697308[4];                           /* 0x00697308 */
extern struct BmxGroup_gDirectDrawGroup gDirectDrawGroup;     /* 0x0069730c */
extern unsigned char d_00697320[4];                           /* 0x00697320 */
extern unsigned char d_00697324[4];                           /* 0x00697324 */
extern unsigned char d_00697328[4];                           /* 0x00697328 */
extern unsigned char d_0069732c[4];                           /* 0x0069732c */
extern unsigned char d_00697330[4];                           /* 0x00697330 */
extern unsigned char d_00697334[1];                           /* 0x00697334 */
extern unsigned char d_00697335[1];                           /* 0x00697335 */
extern unsigned char d_00697336[10];                          /* 0x00697336 */
extern uint8_t *rwD3DContext;                                 /* 0x00697340 */
extern RwCamera *rwD3DCurrentCamera;                          /* 0x00697348 */
extern const RwMemoryFunctions *rwD3DMemory;                  /* 0x0069734c */
extern unsigned char d_00697350[4];                           /* 0x00697350 */
extern int32_t rwD3DFogStart;                                 /* 0x00697354 */
extern float rwD3DFogEnd;                                     /* 0x00697358 */
extern float rwD3DFogScale;                                   /* 0x0069735c */
extern unsigned char d_00697360[4];                           /* 0x00697360 */
extern int32_t rwD3DRasterExtOffset;                          /* 0x00697364 */
extern int32_t rwD3DCameraExtOffset;                          /* 0x00697368 */
extern int32_t rpLodEngineOffset;                             /* 0x00697370 */
extern int32_t rpLodAtomicOffset;                             /* 0x00697374 */
extern void *rpLodCamera;                                     /* 0x00697378 */
extern struct BmxGroup_rpLodFreeListGroup rpLodFreeListGroup; /* 0x0069737c */
extern struct BmxGroup_gMovieGroup gMovieGroup;               /* 0x006973a0 */
extern void *gZrwRaster;                                      /* 0x006976c0, also gRaster, gZrwCamRaster */
extern void *gZrwZRaster;                                     /* 0x006976c4 */
extern void *gZrwWorld;             /* 0x006976c8, also gZrwDebugLight, gZrwSceneState */
extern ZVec3 gZrwWorldCenter;       /* 0x006976cc, also gZrwOriginX, gZrwOriginY, gZrwOriginZ */
extern ZVec3 gZrwWorldCenterScaled; /* 0x006976d8, also gZrwWorldOffset */
extern int gZrwPvsLoaded;           /* 0x006976e4 */
extern int gZrwPvsEnabled;          /* 0x006976e8, also gDebugLight, gZrwDebugLightOn */
extern uint32_t gZrwLightParamA;    /* 0x006976ec */
extern uint32_t gZrwLightParamB;    /* 0x006976f0 */
extern void *gZrwPrimCamera;        /* 0x006976f4, also gZrwActiveRw, gZrwCamera */
extern int gZrwInUpdate;            /* 0x006976f8 */
extern ZrwColBox gZrwColBox;        /* 0x006976fc, also gZrwName */
extern float gZrwFrameLimit;        /* 0x0069771c, also gFrameLimit, gRateA */
extern float gZrwScale;             /* 0x00697720, also gZrwInvScale, gZrwWorldScale */
extern float gZrwScale_00697724;    /* 0x00697724, also gZrwInvScale */
extern struct BmxGroup_gZrwFlagsGroup gZrwFlagsGroup; /* 0x00697728 */
extern int gZrwClearOn;                               /* 0x0069772c, also gClearOn, gZrwCamViewportWord */
extern uint32_t gZrwClearColor;  /* 0x00697730, also gClearColor, gZrwCamViewWindowWord, gZrwClearColorRec */
extern uint32_t gZrwClearColor2; /* 0x00697734 */
extern uint32_t gZrwLastTime;    /* 0x00697738 */
extern uint32_t gZrwFpsTime;     /* 0x0069773c */
extern int gZrwFps;              /* 0x00697740 */
extern struct BmxGroup_gZrwFramesGroup gZrwFramesGroup;                 /* 0x00697744 */
extern uint32_t gZrwScreenW;                                            /* 0x00697770, also gZrwCamWidth */
extern uint32_t gZrwScreenH;                                            /* 0x00697774, also gZrwCamHeight */
extern uint32_t gZrwRenderFlags;                                        /* 0x00697778 */
extern const void *gZrwRenderExtra;                                     /* 0x0069777c */
extern struct BmxGroup_bmxWinMainGlobals bmxWinMainGlobals;             /* 0x00697780 */
extern struct BmxGroup_gGdiSurfaceGroup gGdiSurfaceGroup;               /* 0x006977cc */
extern unsigned char d_006977f0[128];                                   /* 0x006977f0 */
extern unsigned char d_00697870[12];                                    /* 0x00697870 */
extern unsigned char d_0069787c[4];                                     /* 0x0069787c */
extern unsigned char d_00697880[256];                                   /* 0x00697880 */
extern unsigned char d_00697980[1];                                     /* 0x00697980 */
extern unsigned char d_00697981[259];                                   /* 0x00697981 */
extern unsigned char d_00697a84[28];                                    /* 0x00697a84 */
extern unsigned char d_00697aa0[4];                                     /* 0x00697aa0 */
extern unsigned char d_00697aa4[252];                                   /* 0x00697aa4 */
extern unsigned char d_00697ba0[60];                                    /* 0x00697ba0 */
extern unsigned char d_00697bdc[4];                                     /* 0x00697bdc */
extern unsigned char d_00697be0[832];                                   /* 0x00697be0 */
extern unsigned char d_00697f20[4096];                                  /* 0x00697f20 */
extern unsigned char d_00698f20[4];                                     /* 0x00698f20 */
extern unsigned char d_00698f24[20];                                    /* 0x00698f24 */
extern unsigned char d_00698f38[4];                                     /* 0x00698f38 */
extern unsigned char d_00698f3c[8];                                     /* 0x00698f3c */
extern unsigned char d_00698f44[4];                                     /* 0x00698f44 */
extern unsigned char d_00698f48[4];                                     /* 0x00698f48 */
extern unsigned char d_00698f4c[4];                                     /* 0x00698f4c */
extern unsigned char d_00698f50[4];                                     /* 0x00698f50 */
extern unsigned char d_00698f54[16];                                    /* 0x00698f54 */
extern unsigned char d_00698f64[4];                                     /* 0x00698f64 */
extern unsigned char d_00698f68[16];                                    /* 0x00698f68 */
extern unsigned char d_00698f78[4];                                     /* 0x00698f78 */
extern unsigned char d_00698f7c[4];                                     /* 0x00698f7c */
extern unsigned char d_00698f80[4];                                     /* 0x00698f80 */
extern unsigned char d_00698f84[12];                                    /* 0x00698f84 */
extern struct BmxGroup_bmxImports bmxImports;                           /* 0x00699498 */
extern uint32_t d_0069b000[52];                                         /* 0x0069b000 */
extern char sInvalidDataBlockSizeSpecified[48];                         /* 0x0069b0d0 */
extern unsigned char d_0069b100[4];                                     /* 0x0069b100 */
extern float d_0069b104;                                                /* 0x0069b104 */
extern float d_0069b108;                                                /* 0x0069b108 */
extern float d_0069b10c;                                                /* 0x0069b10c */
extern unsigned char d_0069b110[4];                                     /* 0x0069b110 */
extern float d_0069b114[7];                                             /* 0x0069b114 */
extern unsigned char d_0069b130[16];                                    /* 0x0069b130 */
extern unsigned char d_0069b140[4];                                     /* 0x0069b140 */
extern float d_0069b144;                                                /* 0x0069b144 */
extern float d_0069b148;                                                /* 0x0069b148 */
extern float d_0069b14c;                                                /* 0x0069b14c */
extern unsigned char d_0069b150[4];                                     /* 0x0069b150 */
extern float d_0069b154;                                                /* 0x0069b154 */
extern unsigned char d_0069b158[4];                                     /* 0x0069b158 */
extern float d_0069b15c;                                                /* 0x0069b15c */
extern float d_0069b160;                                                /* 0x0069b160 */
extern float d_0069b164;                                                /* 0x0069b164 */
extern unsigned char d_0069b168[4];                                     /* 0x0069b168 */
extern float d_0069b16c[16];                                            /* 0x0069b16c */
extern float d_0069b1ac;                                                /* 0x0069b1ac */
extern float d_0069b1b0[4];                                             /* 0x0069b1b0 */
extern char sNegativeRadius[16];                                        /* 0x0069b1c0 */
extern char sInvalidVertexIndex[32];                                    /* 0x0069b1d0 */
extern char sNegativeParameterPassed[64];                               /* 0x0069b1f0 */
extern float d_0069b230;                                                /* 0x0069b230 */
extern float d_0069b234;                                                /* 0x0069b234 */
extern float d_0069b238;                                                /* 0x0069b238 */
extern float d_0069b23c;                                                /* 0x0069b23c */
extern float d_0069b240;                                                /* 0x0069b240 */
extern float d_0069b244;                                                /* 0x0069b244 */
extern float d_0069b248;                                                /* 0x0069b248 */
extern float d_0069b24c;                                                /* 0x0069b24c */
extern float d_0069b250;                                                /* 0x0069b250 */
extern float d_0069b254;                                                /* 0x0069b254 */
extern float d_0069b258;                                                /* 0x0069b258 */
extern float d_0069b25c;                                                /* 0x0069b25c */
extern float d_0069b260;                                                /* 0x0069b260 */
extern float d_0069b264;                                                /* 0x0069b264 */
extern float d_0069b268;                                                /* 0x0069b268 */
extern float d_0069b26c;                                                /* 0x0069b26c */
extern float d_0069b270;                                                /* 0x0069b270 */
extern float d_0069b274;                                                /* 0x0069b274 */
extern float d_0069b278;                                                /* 0x0069b278 */
extern float d_0069b27c[2];                                             /* 0x0069b27c */
extern float d_0069b284[2];                                             /* 0x0069b284 */
extern float d_0069b28c[2];                                             /* 0x0069b28c */
extern float d_0069b294[2];                                             /* 0x0069b294 */
extern float d_0069b29c[2];                                             /* 0x0069b29c */
extern float d_0069b2a4[2];                                             /* 0x0069b2a4 */
extern float d_0069b2ac;                                                /* 0x0069b2ac */
extern float d_0069b2b0;                                                /* 0x0069b2b0 */
extern float d_0069b2b4;                                                /* 0x0069b2b4 */
extern float d_0069b2b8;                                                /* 0x0069b2b8 */
extern float d_0069b2bc;                                                /* 0x0069b2bc */
extern float d_0069b2c0;                                                /* 0x0069b2c0 */
extern float d_0069b2c4;                                                /* 0x0069b2c4 */
extern float d_0069b2c8;                                                /* 0x0069b2c8 */
extern float d_0069b2cc;                                                /* 0x0069b2cc */
extern float d_0069b2d0;                                                /* 0x0069b2d0 */
extern float d_0069b2d4;                                                /* 0x0069b2d4 */
extern float d_0069b2d8;                                                /* 0x0069b2d8 */
extern float d_0069b2dc;                                                /* 0x0069b2dc */
extern float d_0069b2e0;                                                /* 0x0069b2e0 */
extern float d_0069b2e4;                                                /* 0x0069b2e4 */
extern float d_0069b2e8;                                                /* 0x0069b2e8 */
extern float d_0069b2ec;                                                /* 0x0069b2ec */
extern float d_0069b2f0;                                                /* 0x0069b2f0 */
extern float d_0069b2f4[13];                                            /* 0x0069b2f4 */
extern float rpSphereMinRadius;                                         /* 0x0069b328 */
extern float rpSphereMinRadiusSq;                                       /* 0x0069b330 */
extern float rpSphereMinDistance;                                       /* 0x0069b334 */
extern unsigned char d_0069b338[4];                                     /* 0x0069b338 */
extern float d_0069b33c;                                                /* 0x0069b33c */
extern double d_0069b340[2];                                            /* 0x0069b340 */
extern unsigned char d_0069b350[4];                                     /* 0x0069b350 */
extern float d_0069b354;                                                /* 0x0069b354 */
extern unsigned char d_0069b358[4];                                     /* 0x0069b358 */
extern unsigned char d_0069b35c[4];                                     /* 0x0069b35c */
extern float d_0069b360[3];                                             /* 0x0069b360 */
extern float d_0069b36c;                                                /* 0x0069b36c */
extern float d_0069b370[5];                                             /* 0x0069b370 */
extern float d_0069b384[2];                                             /* 0x0069b384 */
extern float d_0069b38c[2];                                             /* 0x0069b38c */
extern float d_0069b394;                                                /* 0x0069b394 */
extern float d_0069b398;                                                /* 0x0069b398 */
extern float d_0069b39c;                                                /* 0x0069b39c */
extern float d_0069b3a0[15];                                            /* 0x0069b3a0 */
extern unsigned char d_0069b3dc[4];                                     /* 0x0069b3dc */
extern float d_0069b3e0[2];                                             /* 0x0069b3e0 */
extern unsigned char d_0069b3e8[4];                                     /* 0x0069b3e8 */
extern float d_0069b3ec[2];                                             /* 0x0069b3ec */
extern unsigned char d_0069b3f4[4];                                     /* 0x0069b3f4 */
extern float d_0069b3f8[2];                                             /* 0x0069b3f8 */
extern unsigned char d_0069b400[4];                                     /* 0x0069b400 */
extern float d_0069b404[2];                                             /* 0x0069b404 */
extern unsigned char d_0069b40c[40];                                    /* 0x0069b40c */
extern unsigned char d_0069b434[4];                                     /* 0x0069b434 */
extern float d_0069b438;                                                /* 0x0069b438 */
extern unsigned char d_0069b43c[4];                                     /* 0x0069b43c */
extern unsigned char d_0069b440[4];                                     /* 0x0069b440 */
extern float d_0069b444[2];                                             /* 0x0069b444 */
extern float d_0069b44c[13];                                            /* 0x0069b44c */
extern char sNegativeSphereRadius[32];                                  /* 0x0069b480 */
extern float d_0069b4a0;                                                /* 0x0069b4a0 */
extern float d_0069b4a4;                                                /* 0x0069b4a4 */
extern uint32_t d_0069b4a8[18];                                         /* 0x0069b4a8 */
extern float rpWorldScale0;                                             /* 0x0069b4f0 */
extern float rpWorldScale1;                                             /* 0x0069b4f4 */
extern float rpWorldScale2;                                             /* 0x0069b4f8 */
extern float rpWorldReadScale0;                                         /* 0x0069b4fc */
extern float rpWorldReadScale1;                                         /* 0x0069b500 */
extern struct BmxGroup_rpWorldReadScale2Group rpWorldReadScale2Group;   /* 0x0069b504 */
extern void *p_0069b520[4];                                             /* 0x0069b520 */
extern float rwD3DK0x0069b530;                                          /* 0x0069b530 */
extern float rwD3DK0x0069b534;                                          /* 0x0069b534 */
extern struct BmxGroup_rwD3DK0x0069b538Group rwD3DK0x0069b538Group;     /* 0x0069b538 */
extern float rwD3DK0x0069b544;                                          /* 0x0069b544 */
extern float rwD3DK0x0069b548;                                          /* 0x0069b548 */
extern float rwD3DK0x0069b54c;                                          /* 0x0069b54c */
extern float rwD3DK0x0069b550;                                          /* 0x0069b550 */
extern float rwD3DK0x0069b554;                                          /* 0x0069b554 */
extern float rwD3DK0x0069b558;                                          /* 0x0069b558 */
extern float rwD3DK0x0069b55c;                                          /* 0x0069b55c */
extern float rwD3DK0x0069b560;                                          /* 0x0069b560 */
extern float rwD3DK0x0069b564;                                          /* 0x0069b564 */
extern float rwD3DK0x0069b568;                                          /* 0x0069b568 */
extern float rwD3DK0x0069b56c;                                          /* 0x0069b56c */
extern float rwD3DK0x0069b570;                                          /* 0x0069b570 */
extern struct BmxGroup_rwD3DK0x0069b574Group rwD3DK0x0069b574Group;     /* 0x0069b574 */
extern float rwD3DK0x0069b57c;                                          /* 0x0069b57c */
extern float rwD3DK0x0069b580;                                          /* 0x0069b580 */
extern float rwD3DK0x0069b584;                                          /* 0x0069b584 */
extern float rwD3DK0x0069b588;                                          /* 0x0069b588 */
extern float rwD3DK0x0069b58c;                                          /* 0x0069b58c */
extern struct BmxGroup_rwD3DK0x0069b590Group rwD3DK0x0069b590Group;     /* 0x0069b590 */
extern struct BmxGroup_rwD3DK0x0069b598Group rwD3DK0x0069b598Group;     /* 0x0069b598 */
extern float rwD3DK0x0069b5a0;                                          /* 0x0069b5a0 */
extern float rwD3DK0x0069b5a4;                                          /* 0x0069b5a4 */
extern float rwD3DK0x0069b5a8;                                          /* 0x0069b5a8 */
extern float rwD3DK0x0069b5ac;                                          /* 0x0069b5ac */
extern float rwD3DK0x0069b5b0;                                          /* 0x0069b5b0 */
extern struct BmxGroup_rwD3DK0x0069b5b4Group rwD3DK0x0069b5b4Group;     /* 0x0069b5b4 */
extern struct BmxGroup_rwD3DK0x0069b5bcGroup rwD3DK0x0069b5bcGroup;     /* 0x0069b5bc */
extern float rwD3DK0x0069b5c4;                                          /* 0x0069b5c4 */
extern float rwD3DK0x0069b5c8;                                          /* 0x0069b5c8 */
extern struct BmxGroup_rwD3DK0x0069b5ccGroup rwD3DK0x0069b5ccGroup;     /* 0x0069b5cc */
extern float rwD3DK0x0069b5d4;                                          /* 0x0069b5d4 */
extern float rwD3DK0x0069b5d8;                                          /* 0x0069b5d8 */
extern float rwD3DK0x0069b5dc;                                          /* 0x0069b5dc */
extern struct BmxGroup_rwD3DK0x0069b5e0Group rwD3DK0x0069b5e0Group;     /* 0x0069b5e0 */
extern struct BmxGroup_rwD3DK0x0069b5e8Group rwD3DK0x0069b5e8Group;     /* 0x0069b5e8 */
extern float rwD3DK0x0069b5f0;                                          /* 0x0069b5f0 */
extern uint32_t d_0069b600;                                             /* 0x0069b600 */
extern float rwD3DK0x0069b604;                                          /* 0x0069b604 */
extern float rwD3DK0x0069b608;                                          /* 0x0069b608 */
extern struct BmxGroup_rwD3DK0x0069b60cGroup rwD3DK0x0069b60cGroup;     /* 0x0069b60c */
extern float d_0069b618[2];                                             /* 0x0069b618 */
extern float d_0069b620[2];                                             /* 0x0069b620 */
extern float d_0069b628[3];                                             /* 0x0069b628 */
extern float d_0069b634[3];                                             /* 0x0069b634 */
extern double d_0069b640;                                               /* 0x0069b640 */
extern double d_0069b648;                                               /* 0x0069b648 */
extern double d_0069b650;                                               /* 0x0069b650 */
extern float d_0069b658[3];                                             /* 0x0069b658 */
extern float d_0069b664[3];                                             /* 0x0069b664 */
extern char s03d[16];                                                   /* 0x0069b670 */
extern char s02x[16];                                                   /* 0x0069b680 */
extern char s_0069b690[16];                                             /* 0x0069b690 */
extern char sW_0069b6a0[16];                                            /* 0x0069b6a0 */
extern float d_0069b6b0;                                                /* 0x0069b6b0 */
extern float d_0069b6b4[2];                                             /* 0x0069b6b4 */
extern float d_0069b6bc;                                                /* 0x0069b6bc */
extern float d_0069b6c0;                                                /* 0x0069b6c0 */
extern float d_0069b6c4;                                                /* 0x0069b6c4 */
extern float d_0069b6c8;                                                /* 0x0069b6c8 */
extern float d_0069b6cc;                                                /* 0x0069b6cc */
extern float d_0069b6d0[2];                                             /* 0x0069b6d0 */
extern uint32_t d_0069b6d8;                                             /* 0x0069b6d8 */
extern uint32_t d_0069b6dc;                                             /* 0x0069b6dc */
extern uint32_t d_0069b6e0[2];                                          /* 0x0069b6e0 */
extern float d_0069b6e8;                                                /* 0x0069b6e8 */
extern float d_0069b6ec[2];                                             /* 0x0069b6ec */
extern float d_0069b6f4;                                                /* 0x0069b6f4 */
extern float d_0069b6f8;                                                /* 0x0069b6f8 */
extern float d_0069b6fc[5];                                             /* 0x0069b6fc */
extern float d_0069b710[2];                                             /* 0x0069b710 */
extern float d_0069b718[10];                                            /* 0x0069b718 */
extern char sOnlyRenderingSubSystem[32];                                /* 0x0069b740 */
extern unsigned char d_0069b760[4];                                     /* 0x0069b760 */
extern unsigned char d_0069b764[4];                                     /* 0x0069b764 */
extern unsigned char d_0069b768[4];                                     /* 0x0069b768 */
extern unsigned char d_0069b76c[4];                                     /* 0x0069b76c */
extern unsigned char d_0069b770[4];                                     /* 0x0069b770 */
extern unsigned char d_0069b774[4];                                     /* 0x0069b774 */
extern float d_0069b778;                                                /* 0x0069b778 */
extern float d_0069b77c;                                                /* 0x0069b77c */
extern float d_0069b780;                                                /* 0x0069b780 */
extern float d_0069b784;                                                /* 0x0069b784 */
extern float d_0069b788;                                                /* 0x0069b788 */
extern float d_0069b78c[4];                                             /* 0x0069b78c */
extern uint32_t d_0069b79c;                                             /* 0x0069b79c */
extern float d_0069b7a0;                                                /* 0x0069b7a0 */
extern float d_0069b7a4[2];                                             /* 0x0069b7a4 */
extern float d_0069b7ac[25];                                            /* 0x0069b7ac */
extern float d_0069b810;                                                /* 0x0069b810 */
extern float d_0069b814[2];                                             /* 0x0069b814 */
extern float d_0069b81c;                                                /* 0x0069b81c */
extern uint32_t d_0069b820;                                             /* 0x0069b820 */
extern float d_0069b824;                                                /* 0x0069b824 */
extern unsigned char d_0069b828[4];                                     /* 0x0069b828 */
extern float d_0069b82c[13];                                            /* 0x0069b82c */
extern float d_0069b860;                                                /* 0x0069b860 */
extern float d_0069b864;                                                /* 0x0069b864 */
extern float d_0069b868[16];                                            /* 0x0069b868 */
extern float d_0069b8a8;                                                /* 0x0069b8a8 */
extern float d_0069b8ac;                                                /* 0x0069b8ac */
extern float d_0069b8b0;                                                /* 0x0069b8b0 */
extern BmxType_rwMatrixNormalErrorLimits rwMatrixNormalErrorLimits;     /* 0x0069b8b4 */
extern char sInvalidCombinationType[128];                               /* 0x0069b8c0 */
extern unsigned char d_0069b940[4];                                     /* 0x0069b940 */
extern float d_0069b944[3];                                             /* 0x0069b944 */
extern double rwInvSqrtScaleOdd;                                        /* 0x0069b950 */
extern struct BmxGroup_rwInvSqrtScaleEvenGroup rwInvSqrtScaleEvenGroup; /* 0x0069b958 */
extern float d_0069b968[3];                                             /* 0x0069b968 */
extern float d_0069b974;                                                /* 0x0069b974 */
extern double d_0069b978;                                               /* 0x0069b978 */
extern uint32_t d_0069b980[3];                                          /* 0x0069b980 */
extern float rwD3DFogRange;                                             /* 0x0069b98c */
extern float d_0069b994;                                                /* 0x0069b994 */
extern float d_0069b998;                                                /* 0x0069b998 */
extern float d_0069b99c[18];                                            /* 0x0069b99c */
extern float d_0069b9e4;                                                /* 0x0069b9e4 */
extern double d_0069b9e8;                                               /* 0x0069b9e8 */
extern unsigned char d_0069b9f0[4];                                     /* 0x0069b9f0 */
extern struct BmxGroup_d_0069b9f4Group d_0069b9f4Group;                 /* 0x0069b9f4 */
extern void *p_0069ba20[11];                                            /* 0x0069ba20 */
extern void *p_0069ba4c[17];                                            /* 0x0069ba4c */
extern void *p_0069ba90[11];                                            /* 0x0069ba90 */
extern void *p_0069babc[12];                                            /* 0x0069babc */
extern struct BmxGroup_p_0069baecGroup p_0069baecGroup;                 /* 0x0069baec */
extern char sFailedToLockD3DSurface[32];                                /* 0x0069bb90 */
extern char sFailedToUnlockD3DSurface[32];                              /* 0x0069bbb0 */
extern char sFailedToCreateDDSurfaceForTexture[48];                     /* 0x0069bbd0 */
extern char sDDS[16];                                                   /* 0x0069bc00 */
extern char sDds[16];                                                   /* 0x0069bc10 */
extern char sD3D[16];                                                   /* 0x0069bc20 */
extern char sRefRast[64];                                               /* 0x0069bc30 */
extern BmxType_rwPerspCornerSteps rwPerspCornerSteps;                   /* 0x0069bc70 */
extern BmxType_rwViewFactors rwViewFactors;                             /* 0x0069bc88 */
extern float d_0069bcb0;                                                /* 0x0069bcb0 */
extern float d_0069bcb4[3];                                             /* 0x0069bcb4 */
extern float d_0069bcc0[2];                                             /* 0x0069bcc0 */
extern float d_0069bcc8[2];                                             /* 0x0069bcc8 */
extern float d_0069bcd0[2];                                             /* 0x0069bcd0 */
extern uint32_t d_0069bcd8[34];                                         /* 0x0069bcd8 */
extern char sInvalidProjectionTypeSpecified[48];                        /* 0x0069bd60 */
extern char sNull_0069bd90[16];                                         /* 0x0069bd90 */
extern char rwMipSuffix;                                                /* 0x0069bda0 */
extern char sRb_0069bdb0[16];                                           /* 0x0069bdb0 */
extern char sWb_0069bdc0[16];                                           /* 0x0069bdc0 */
extern char sAb[16];                                                    /* 0x0069bdd0 */
extern char sInvalidLengthPassed[28];                                   /* 0x0069bde0 */
extern struct BmxGroup_d_0069bdfcGroup d_0069bdfcGroup;                 /* 0x0069bdfc */
extern float d_0069be54[3];                                             /* 0x0069be54 */
extern double d_0069be60;                                               /* 0x0069be60 */
extern double d_0069be68;                                               /* 0x0069be68 */
extern double d_0069be70;                                               /* 0x0069be70 */
extern float d_0069be78[3];                                             /* 0x0069be78 */
extern float d_0069be84;                                                /* 0x0069be84 */
extern float d_0069be88[2];                                             /* 0x0069be88 */
extern double d_0069be90;                                               /* 0x0069be90 */
extern double d_0069be98;                                               /* 0x0069be98 */
extern double d_0069bea0;                                               /* 0x0069bea0 */
extern uint32_t d_0069bea8[10];                                         /* 0x0069bea8 */
extern float d_0069bed0;                                                /* 0x0069bed0 */
extern float d_0069bed4;                                                /* 0x0069bed4 */
extern float d_0069bed8[2];                                             /* 0x0069bed8 */
extern struct BmxGroup_p_0069bee0Group p_0069bee0Group;                 /* 0x0069bee0 */
extern float d_0069bef8[15];                                            /* 0x0069bef8 */
extern float d_0069bf34[3];                                             /* 0x0069bf34 */
extern void *p_0069bf40[20];                                            /* 0x0069bf40 */
extern char sReferenceRasterizer[32];                                   /* 0x0069bf90 */
extern char sSSSW[16];                                                  /* 0x0069bfb0 */
extern char sSSW[16];                                                   /* 0x0069bfc0 */
extern char sSSHW[16];                                                  /* 0x0069bfd0 */
extern char sSHW[16];                                                   /* 0x0069bfe0 */
extern char sUnableToCreateD3DTextureFormatFreeList[48];                /* 0x0069bff0 */
extern char sUnableToEnumerateD3DTextureFormats[48];                    /* 0x0069c020 */
extern char sUnableToFindAnySuitableTextureFormats[48];                 /* 0x0069c050 */
extern char sInvalidSubsystemNumber[32];                                /* 0x0069c080 */
extern char sInvalidSubsystemNumber_0069c0a0[32];                       /* 0x0069c0a0 */
extern char sFailedToCreateDirectDraw[32];                              /* 0x0069c0c0 */
extern char sFailedToCreateDirectDraw4[32];                             /* 0x0069c0e0 */
extern char sFailedToCreateD3D3[32];                                    /* 0x0069c100 */
extern char sFailedToSetDirectDrawCooperativeLevel[48];                 /* 0x0069c120 */
extern char sFailedToSetDisplayMode[32];                                /* 0x0069c150 */
extern char sFailedToCreatePrimarySurface[48];                          /* 0x0069c170 */
extern char sFailedToCreateZBufferSurface[48];                          /* 0x0069c1a0 */
extern char sFailedToAttachZBufferSurface[48];                          /* 0x0069c1d0 */
extern char sFailedToCreateD3D3Device[32];                              /* 0x0069c200 */
extern char sFailedToCreateViewport[32];                                /* 0x0069c220 */
extern char sFailedToAddViewport[32];                                   /* 0x0069c240 */
extern char sFailedToSetTheViewport[32];                                /* 0x0069c260 */
extern char sFailedToCreateBackgroundMaterial[48];                      /* 0x0069c280 */
extern char sFailedToSetBackground[32];                                 /* 0x0069c2b0 */
extern char sFailedToSetCooperativeLevel[32];                           /* 0x0069c2d0 */
extern char sFailedToCreateDirectDrawClipper[48];                       /* 0x0069c2f0 */
extern char sFailedToAllocateTexChainElement[48];                       /* 0x0069c320 */
extern float d_0069c350;                                                /* 0x0069c350 */
extern float d_0069c354;                                                /* 0x0069c354 */
extern float d_0069c358;                                                /* 0x0069c358 */
extern float d_0069c35c;                                                /* 0x0069c35c */
extern float d_0069c360;                                                /* 0x0069c360 */
extern float d_0069c364;                                                /* 0x0069c364 */
extern float d_0069c368;                                                /* 0x0069c368 */
extern float d_0069c36c;                                                /* 0x0069c36c */
extern float d_0069c370;                                                /* 0x0069c370 */
extern float d_0069c374[2];                                             /* 0x0069c374 */
extern unsigned char d_0069c37c[4];                                     /* 0x0069c37c */
extern unsigned char d_0069c380[4];                                     /* 0x0069c380 */
extern unsigned char d_0069c384[4];                                     /* 0x0069c384 */
extern float d_0069c388[3];                                             /* 0x0069c388 */
extern unsigned char d_0069c394[4];                                     /* 0x0069c394 */
extern float d_0069c398[3];                                             /* 0x0069c398 */
extern unsigned char d_0069c3a4[4];                                     /* 0x0069c3a4 */
extern float d_0069c3a8[3];                                             /* 0x0069c3a8 */
extern unsigned char d_0069c3b4[4];                                     /* 0x0069c3b4 */
extern float d_0069c3b8[3];                                             /* 0x0069c3b8 */
extern unsigned char d_0069c3c4[4];                                     /* 0x0069c3c4 */
extern double d_0069c3c8;                                               /* 0x0069c3c8 */
extern float d_0069c3d0;                                                /* 0x0069c3d0 */
extern unsigned char d_0069c3d4[4];                                     /* 0x0069c3d4 */
extern unsigned char d_0069c3d8[4];                                     /* 0x0069c3d8 */
extern unsigned char d_0069c3dc[4];                                     /* 0x0069c3dc */
extern unsigned char d_0069c3e0[4];                                     /* 0x0069c3e0 */
extern float d_0069c3e4[11];                                            /* 0x0069c3e4 */
extern uint32_t d_0069c410;                                             /* 0x0069c410 */
extern float d_0069c414;                                                /* 0x0069c414 */
extern float d_0069c418;                                                /* 0x0069c418 */
extern float d_0069c41c;                                                /* 0x0069c41c */
extern float d_0069c420;                                                /* 0x0069c420 */
extern uint32_t d_0069c424;                                             /* 0x0069c424 */
extern uint32_t d_0069c428;                                             /* 0x0069c428 */
extern uint32_t d_0069c42c;                                             /* 0x0069c42c */
extern uint32_t d_0069c430;                                             /* 0x0069c430 */
extern float d_0069c434;                                                /* 0x0069c434 */
extern float d_0069c438;                                                /* 0x0069c438 */
extern float d_0069c43c;                                                /* 0x0069c43c */
extern float d_0069c440;                                                /* 0x0069c440 */
extern float d_0069c444;                                                /* 0x0069c444 */
extern float d_0069c448;                                                /* 0x0069c448 */
extern float d_0069c44c;                                                /* 0x0069c44c */
extern float d_0069c450;                                                /* 0x0069c450 */
extern float d_0069c454;                                                /* 0x0069c454 */
extern float d_0069c458;                                                /* 0x0069c458 */
extern uint32_t d_0069c45c;                                             /* 0x0069c45c */
extern uint32_t d_0069c460;                                             /* 0x0069c460 */
extern uint32_t d_0069c464;                                             /* 0x0069c464 */
extern uint32_t d_0069c468;                                             /* 0x0069c468 */
extern float d_0069c46c;                                                /* 0x0069c46c */
extern float d_0069c470;                                                /* 0x0069c470 */
extern float d_0069c474;                                                /* 0x0069c474 */
extern float d_0069c478;                                                /* 0x0069c478 */
extern float d_0069c47c;                                                /* 0x0069c47c */
extern float d_0069c480;                                                /* 0x0069c480 */
extern float d_0069c484;                                                /* 0x0069c484 */
extern float d_0069c488;                                                /* 0x0069c488 */
extern uint32_t d_0069c48c;                                             /* 0x0069c48c */
extern uint32_t d_0069c490;                                             /* 0x0069c490 */
extern uint32_t d_0069c494;                                             /* 0x0069c494 */
extern uint32_t d_0069c498;                                             /* 0x0069c498 */
extern float d_0069c49c[2];                                             /* 0x0069c49c */
extern float d_0069c4a4[2];                                             /* 0x0069c4a4 */
extern uint32_t d_0069c4ac;                                             /* 0x0069c4ac */
extern uint32_t d_0069c4b0;                                             /* 0x0069c4b0 */
extern uint32_t d_0069c4b4;                                             /* 0x0069c4b4 */
extern uint32_t d_0069c4b8;                                             /* 0x0069c4b8 */
extern uint32_t d_0069c4bc[2];                                          /* 0x0069c4bc */
extern float d_0069c4c4;                                                /* 0x0069c4c4 */
extern float d_0069c4c8;                                                /* 0x0069c4c8 */
extern float d_0069c4cc;                                                /* 0x0069c4cc */
extern float d_0069c4d0;                                                /* 0x0069c4d0 */
extern float d_0069c4d4;                                                /* 0x0069c4d4 */
extern float d_0069c4d8;                                                /* 0x0069c4d8 */
extern float d_0069c4dc;                                                /* 0x0069c4dc */
extern float d_0069c4e0;                                                /* 0x0069c4e0 */
extern float d_0069c4e4;                                                /* 0x0069c4e4 */
extern float d_0069c4e8;                                                /* 0x0069c4e8 */
extern uint32_t d_0069c4ec;                                             /* 0x0069c4ec */
extern uint32_t d_0069c4f0;                                             /* 0x0069c4f0 */
extern uint32_t d_0069c4f4;                                             /* 0x0069c4f4 */
extern uint32_t d_0069c4f8;                                             /* 0x0069c4f8 */
extern uint32_t d_0069c4fc;                                             /* 0x0069c4fc */
extern uint32_t d_0069c500;                                             /* 0x0069c500 */
extern uint32_t d_0069c504;                                             /* 0x0069c504 */
extern uint32_t d_0069c508[3];                                          /* 0x0069c508 */
extern float d_0069c514[2];                                             /* 0x0069c514 */
extern float d_0069c51c[3];                                             /* 0x0069c51c */
extern float d_0069c528[2];                                             /* 0x0069c528 */
extern float d_0069c530[3];                                             /* 0x0069c530 */
extern float d_0069c53c[2];                                             /* 0x0069c53c */
extern float d_0069c544[2];                                             /* 0x0069c544 */
extern float d_0069c54c[2];                                             /* 0x0069c54c */
extern float d_0069c554;                                                /* 0x0069c554 */
extern float d_0069c558;                                                /* 0x0069c558 */
extern float d_0069c55c[2];                                             /* 0x0069c55c */
extern float d_0069c564;                                                /* 0x0069c564 */
extern float d_0069c568;                                                /* 0x0069c568 */
extern float d_0069c56c[3];                                             /* 0x0069c56c */
extern float d_0069c578;                                                /* 0x0069c578 */
extern float d_0069c57c[2];                                             /* 0x0069c57c */
extern float d_0069c584[2];                                             /* 0x0069c584 */
extern float d_0069c58c;                                                /* 0x0069c58c */
extern float d_0069c590[3];                                             /* 0x0069c590 */
extern float d_0069c59c[2];                                             /* 0x0069c59c */
extern float d_0069c5a4[2];                                             /* 0x0069c5a4 */
extern float d_0069c5ac[3];                                             /* 0x0069c5ac */
extern float d_0069c5b8[2];                                             /* 0x0069c5b8 */
extern struct BmxGroup_rwD3DOneGroup rwD3DOneGroup;                     /* 0x0069c5c0 */
extern unsigned char d_0069c5d4[4];                                     /* 0x0069c5d4 */
extern float d_0069c5d8;                                                /* 0x0069c5d8 */
extern float d_0069c5dc;                                                /* 0x0069c5dc */
extern float d_0069c5e0;                                                /* 0x0069c5e0 */
extern float d_0069c5e4;                                                /* 0x0069c5e4 */
extern float d_0069c5e8;                                                /* 0x0069c5e8 */
extern float d_0069c5ec;                                                /* 0x0069c5ec */
extern float d_0069c5f0;                                                /* 0x0069c5f0 */
extern float d_0069c5f4;                                                /* 0x0069c5f4 */
extern float d_0069c5f8;                                                /* 0x0069c5f8 */
extern float d_0069c5fc;                                                /* 0x0069c5fc */
extern float d_0069c600;                                                /* 0x0069c600 */
extern float d_0069c604[4];                                             /* 0x0069c604 */
extern float d_0069c614;                                                /* 0x0069c614 */
extern float d_0069c618[9];                                             /* 0x0069c618 */
extern float rpAnimDurationStart;                                       /* 0x0069c63c */
extern float rpAnimSumStartA;                                           /* 0x0069c640 */
extern struct BmxGroup_rpAnimSumStartBGroup rpAnimSumStartBGroup;       /* 0x0069c644 */
extern unsigned char d_0069c660[4];                                     /* 0x0069c660 */
extern unsigned char d_0069c664[4];                                     /* 0x0069c664 */
extern unsigned char d_0069c668[4];                                     /* 0x0069c668 */
extern unsigned char d_0069c66c[4];                                     /* 0x0069c66c */
extern unsigned char d_0069c670[4];                                     /* 0x0069c670 */
extern unsigned char d_0069c674[4];                                     /* 0x0069c674 */
extern float d_0069c678;                                                /* 0x0069c678 */
extern float d_0069c67c;                                                /* 0x0069c67c */
extern unsigned char d_0069c680[4];                                     /* 0x0069c680 */
extern unsigned char d_0069c684[4];                                     /* 0x0069c684 */
extern unsigned char d_0069c688[4];                                     /* 0x0069c688 */
extern unsigned char d_0069c68c[4];                                     /* 0x0069c68c */
extern unsigned char d_0069c690[8];                                     /* 0x0069c690 */
extern float rpAnimTimeOffset;                                          /* 0x0069c698 */
extern struct BmxGroup_rpAnimStartStepGroup rpAnimStartStepGroup;       /* 0x0069c69c */
extern struct BmxGroup_rpAnimHeadNameGroup rpAnimHeadNameGroup;         /* 0x0069c6b0 */
extern float d_0069c720;                                                /* 0x0069c720 */
extern uint32_t d_0069c724[64];                                         /* 0x0069c724 */
extern float d_0069c824;                                                /* 0x0069c824 */
extern float d_0069c828;                                                /* 0x0069c828 */
extern float d_0069c82c;                                                /* 0x0069c82c */
extern double d_0069c830;                                               /* 0x0069c830 */
extern float d_0069c838;                                                /* 0x0069c838 */
extern float d_0069c83c[5];                                             /* 0x0069c83c */
extern char s105_0069c850[16];                                          /* 0x0069c850 */
extern char sIncompatibleLibpngVersionInApplicationA[64];               /* 0x0069c860 */
extern char sZlibMemoryError[32];                                       /* 0x0069c8a0 */
extern char sZlibVersionError[32];                                      /* 0x0069c8c0 */
extern char sUnknownZlibError[32];                                      /* 0x0069c8e0 */
extern char s113[16];                                                   /* 0x0069c900 */
extern char sZlibMemory[16];                                            /* 0x0069c910 */
extern char sZlibVersion[16];                                           /* 0x0069c920 */
extern char sNotAPNGFile[16];                                           /* 0x0069c930 */
extern char sPNGFileCorruptedByASCIIConversion[48];                     /* 0x0069c940 */
extern char sMissingIHDRBeforeIDAT[32];                                 /* 0x0069c970 */
extern char sMissingPLTEBeforeIDAT[32];                                 /* 0x0069c990 */
extern char sInvalidAttemptToReadRowData[48];                           /* 0x0069c9b0 */
extern char sNotEnoughImageData[32];                                    /* 0x0069c9e0 */
extern char sExtraCompressedData[32];                                   /* 0x0069ca00 */
extern char sDecompressionError[32];                                    /* 0x0069ca20 */
extern char sTooManyIDATSFound[32];                                     /* 0x0069ca40 */
extern char sLibpngVersion105October151999Header[64];                   /* 0x0069ca60 */
extern char sTooManyBytesForPNGSignature[48];                           /* 0x0069caa0 */
extern char sLibpngVersion105October151999CopyrightC[208];              /* 0x0069cad0 */
extern char sCallToNULLReadFunction[32];                                /* 0x0069cba0 */
extern char sItSAnErrorToSetBothReadDataFnAndWriteDa[80];               /* 0x0069cbc0 */
extern char sSameStructureResettingWriteDataFnToNULL[64];               /* 0x0069cc10 */
extern double d_0069cc50;                                               /* 0x0069cc50 */
extern unsigned char d_0069cc58[8];                                     /* 0x0069cc58 */
extern char sWidthTooLargeForLibpngToProcessImageDat[64];               /* 0x0069cc60 */
extern double d_0069cca0;                                               /* 0x0069cca0 */
extern double d_0069cca8;                                               /* 0x0069cca8 */
extern double d_0069ccb0;                                               /* 0x0069ccb0 */
extern double d_0069ccb8;                                               /* 0x0069ccb8 */
extern double d_0069ccc0;                                               /* 0x0069ccc0 */
extern double d_0069ccc8;                                               /* 0x0069ccc8 */
extern unsigned char d_0069ccd0[8];                                     /* 0x0069ccd0 */
extern unsigned char d_0069ccd8[8];                                     /* 0x0069ccd8 */
extern unsigned char d_0069cce0[8];                                     /* 0x0069cce0 */
extern double d_0069cce8;                                               /* 0x0069cce8 */
extern double d_0069ccf0[2];                                            /* 0x0069ccf0 */
extern uint32_t d_0069cd00[2];                                          /* 0x0069cd00 */
extern double d_0069cd08;                                               /* 0x0069cd08 */
extern double d_0069cd10;                                               /* 0x0069cd10 */
extern double d_0069cd18[2];                                            /* 0x0069cd18 */
extern uint32_t d_0069cd28[2];                                          /* 0x0069cd28 */
extern double d_0069cd30;                                               /* 0x0069cd30 */
extern double d_0069cd38;                                               /* 0x0069cd38 */
extern double d_0069cd40;                                               /* 0x0069cd40 */
extern double d_0069cd48[2];                                            /* 0x0069cd48 */
extern uint32_t d_0069cd58[2];                                          /* 0x0069cd58 */
extern double d_0069cd60;                                               /* 0x0069cd60 */
extern double d_0069cd68;                                               /* 0x0069cd68 */
extern double d_0069cd70;                                               /* 0x0069cd70 */
extern double d_0069cd78;                                               /* 0x0069cd78 */
extern double d_0069cd80[2];                                            /* 0x0069cd80 */
extern double d_0069cd90;                                               /* 0x0069cd90 */
extern double d_0069cd98;                                               /* 0x0069cd98 */
extern double d_0069cda0;                                               /* 0x0069cda0 */
extern double d_0069cda8[2];                                            /* 0x0069cda8 */
extern uint32_t d_0069cdb8[2];                                          /* 0x0069cdb8 */
extern double d_0069cdc0;                                               /* 0x0069cdc0 */
extern double d_0069cdc8;                                               /* 0x0069cdc8 */
extern double d_0069cdd0;                                               /* 0x0069cdd0 */
extern double d_0069cdd8[2];                                            /* 0x0069cdd8 */
extern uint32_t d_0069cde8[2];                                          /* 0x0069cde8 */
extern double d_0069cdf0;                                               /* 0x0069cdf0 */
extern double d_0069cdf8;                                               /* 0x0069cdf8 */
extern uint32_t d_0069ce00[2];                                          /* 0x0069ce00 */
extern double d_0069ce08;                                               /* 0x0069ce08 */
extern double d_0069ce10;                                               /* 0x0069ce10 */
extern double d_0069ce18;                                               /* 0x0069ce18 */
extern double d_0069ce20;                                               /* 0x0069ce20 */
extern double d_0069ce28;                                               /* 0x0069ce28 */
extern double d_0069ce30;                                               /* 0x0069ce30 */
extern uint32_t d_0069ce38[2];                                          /* 0x0069ce38 */
extern uint32_t d_0069ce40[2];                                          /* 0x0069ce40 */
extern uint32_t d_0069ce48[2];                                          /* 0x0069ce48 */
extern double d_0069ce50;                                               /* 0x0069ce50 */
extern double d_0069ce58;                                               /* 0x0069ce58 */
extern double d_0069ce60;                                               /* 0x0069ce60 */
extern double d_0069ce68;                                               /* 0x0069ce68 */
extern double d_0069ce70;                                               /* 0x0069ce70 */
extern double d_0069ce78;                                               /* 0x0069ce78 */
extern uint32_t d_0069ce80[2];                                          /* 0x0069ce80 */
extern double d_0069ce88;                                               /* 0x0069ce88 */
extern double d_0069ce90;                                               /* 0x0069ce90 */
extern uint32_t d_0069ce98[2];                                          /* 0x0069ce98 */
extern double d_0069cea0;                                               /* 0x0069cea0 */
extern double d_0069cea8;                                               /* 0x0069cea8 */
extern uint32_t d_0069ceb0[2];                                          /* 0x0069ceb0 */
extern double d_0069ceb8;                                               /* 0x0069ceb8 */
extern double d_0069cec0;                                               /* 0x0069cec0 */
extern double d_0069cec8[2];                                            /* 0x0069cec8 */
extern double d_0069ced8[2];                                            /* 0x0069ced8 */
extern double d_0069cee8[2];                                            /* 0x0069cee8 */
extern double d_0069cef8[2];                                            /* 0x0069cef8 */
extern double d_0069cf08;                                               /* 0x0069cf08 */
extern double d_0069cf10;                                               /* 0x0069cf10 */
extern double d_0069cf18;                                               /* 0x0069cf18 */
extern double d_0069cf20;                                               /* 0x0069cf20 */
extern double d_0069cf28;                                               /* 0x0069cf28 */
extern double d_0069cf30;                                               /* 0x0069cf30 */
extern double d_0069cf38;                                               /* 0x0069cf38 */
extern double d_0069cf40;                                               /* 0x0069cf40 */
extern double d_0069cf48;                                               /* 0x0069cf48 */
extern double d_0069cf50;                                               /* 0x0069cf50 */
extern double d_0069cf58;                                               /* 0x0069cf58 */
extern double d_0069cf60;                                               /* 0x0069cf60 */
extern double d_0069cf68;                                               /* 0x0069cf68 */
extern double d_0069cf70;                                               /* 0x0069cf70 */
extern double d_0069cf78;                                               /* 0x0069cf78 */
extern double d_0069cf80[2];                                            /* 0x0069cf80 */
extern double d_0069cf90[2];                                            /* 0x0069cf90 */
extern double d_0069cfa0[2];                                            /* 0x0069cfa0 */
extern double d_0069cfb0[2];                                            /* 0x0069cfb0 */
extern char sCanTDiscardCriticalDataOnCRCError[48];                     /* 0x0069cfc0 */
extern char sApplicationMustSupplyAKnownBackgroundGa[64];               /* 0x0069cff0 */
extern char sIgnoringOutOfRangeRgbToGrayCoefficients[48];               /* 0x0069d030 */
extern char sNULLRowBuffer[16];                                         /* 0x0069d060 */
extern char sPngDoRgbToGrayFoundNongrayPixel[48];                       /* 0x0069d070 */
extern char sPngDoDitherReturnedRowbytes0[48];                          /* 0x0069d0a0 */
extern char sOutOfMemory[80];                                           /* 0x0069d0d0 */
extern struct BmxGroup_p_0069d120Group p_0069d120Group;                 /* 0x0069d120 */
extern char sUnknownCompressionMethod[32];                              /* 0x0069d160 */
extern char sInvalidWindowSize[32];                                     /* 0x0069d180 */
extern char sIncorrectHeaderCheck[32];                                  /* 0x0069d1a0 */
extern char sNeedDictionary[16];                                        /* 0x0069d1c0 */
extern char sIncorrectDataCheck[32];                                    /* 0x0069d1d0 */
extern double d_0069d1f0;                                               /* 0x0069d1f0 */
extern double d_0069d1f8;                                               /* 0x0069d1f8 */
extern float d_0069d200;                                                /* 0x0069d200 */
extern float d_0069d204;                                                /* 0x0069d204 */
extern float d_0069d208;                                                /* 0x0069d208 */
extern unsigned char d_0069d20c[4];                                     /* 0x0069d20c */
extern double d_0069d210;                                               /* 0x0069d210 */
extern unsigned char d_0069d218[8];                                     /* 0x0069d218 */
extern double d_0069d220;                                               /* 0x0069d220 */
extern double d_0069d228;                                               /* 0x0069d228 */
extern float d_0069d230;                                                /* 0x0069d230 */
extern float d_0069d234;                                                /* 0x0069d234 */
extern unsigned char d_0069d238[8];                                     /* 0x0069d238 */
extern double d_0069d240;                                               /* 0x0069d240 */
extern unsigned char d_0069d248[8];                                     /* 0x0069d248 */
extern double d_0069d250;                                               /* 0x0069d250 */
extern double d_0069d258;                                               /* 0x0069d258 */
extern float d_0069d260;                                                /* 0x0069d260 */
extern float d_0069d264;                                                /* 0x0069d264 */
extern unsigned char d_0069d268[8];                                     /* 0x0069d268 */
extern double d_0069d270;                                               /* 0x0069d270 */
extern unsigned char d_0069d278[8];                                     /* 0x0069d278 */
extern double d_0069d280;                                               /* 0x0069d280 */
extern double d_0069d288;                                               /* 0x0069d288 */
extern float d_0069d290;                                                /* 0x0069d290 */
extern float d_0069d294;                                                /* 0x0069d294 */
extern unsigned char d_0069d298[4];                                     /* 0x0069d298 */
extern float d_0069d29c;                                                /* 0x0069d29c */
extern unsigned char d_0069d2a0[4];                                     /* 0x0069d2a0 */
extern float d_0069d2a4;                                                /* 0x0069d2a4 */
extern float d_0069d2a8;                                                /* 0x0069d2a8 */
extern float d_0069d2ac;                                                /* 0x0069d2ac */
extern uint32_t d_0069d2b0[2];                                          /* 0x0069d2b0 */
extern float d_0069d2b8[2];                                             /* 0x0069d2b8 */
extern uint32_t d_0069d2c0[2];                                          /* 0x0069d2c0 */
extern float d_0069d2c8[2];                                             /* 0x0069d2c8 */
extern uint32_t d_0069d2d0[2];                                          /* 0x0069d2d0 */
extern float d_0069d2d8[2];                                             /* 0x0069d2d8 */
extern uint32_t d_0069d2e0[2];                                          /* 0x0069d2e0 */
extern float d_0069d2e8[2];                                             /* 0x0069d2e8 */
extern uint32_t d_0069d2f0[2];                                          /* 0x0069d2f0 */
extern float d_0069d2f8[2];                                             /* 0x0069d2f8 */
extern uint32_t d_0069d300[2];                                          /* 0x0069d300 */
extern float d_0069d308[2];                                             /* 0x0069d308 */
extern uint32_t d_0069d310[2];                                          /* 0x0069d310 */
extern float d_0069d318[2];                                             /* 0x0069d318 */
extern uint32_t d_0069d320[2];                                          /* 0x0069d320 */
extern double d_0069d328;                                               /* 0x0069d328 */
extern float d_0069d330[2];                                             /* 0x0069d330 */
extern double d_0069d338;                                               /* 0x0069d338 */
extern double d_0069d340;                                               /* 0x0069d340 */
extern float d_0069d348[2];                                             /* 0x0069d348 */
extern uint32_t d_0069d350[2];                                          /* 0x0069d350 */
extern float d_0069d358[2];                                             /* 0x0069d358 */
extern uint32_t d_0069d360[2];                                          /* 0x0069d360 */
extern float d_0069d368[2];                                             /* 0x0069d368 */
extern uint32_t d_0069d370[2];                                          /* 0x0069d370 */
extern float d_0069d378[2];                                             /* 0x0069d378 */
extern uint32_t d_0069d380[2];                                          /* 0x0069d380 */
extern float d_0069d388[2];                                             /* 0x0069d388 */
extern uint32_t d_0069d390[2];                                          /* 0x0069d390 */
extern float d_0069d398[2];                                             /* 0x0069d398 */
extern uint32_t d_0069d3a0[2];                                          /* 0x0069d3a0 */
extern float d_0069d3a8[2];                                             /* 0x0069d3a8 */
extern uint32_t d_0069d3b0[2];                                          /* 0x0069d3b0 */
extern float d_0069d3b8[2];                                             /* 0x0069d3b8 */
extern uint32_t d_0069d3c0[4];                                          /* 0x0069d3c0 */
extern char sCRCError[16];                                              /* 0x0069d3d0 */
extern char sOutOfPlaceIHDR[32];                                        /* 0x0069d3e0 */
extern char sInvalidIHDRChunk[32];                                      /* 0x0069d400 */
extern char sInvalidImageSizeInIHDR[32];                                /* 0x0069d420 */
extern char sInvalidBitDepthInIHDR[32];                                 /* 0x0069d440 */
extern char sInvalidColorTypeInIHDR[32];                                /* 0x0069d460 */
extern char sInvalidColorTypeBitDepthCombinationInIH[64];               /* 0x0069d480 */
extern char sUnknownInterlaceMethodInIHDR[48];                          /* 0x0069d4c0 */
extern char sUnknownCompressionMethodInIHDR[48];                        /* 0x0069d4f0 */
extern char sUnknownFilterMethodInIHDR[32];                             /* 0x0069d520 */
extern char sMissingIHDRBeforePLTE[32];                                 /* 0x0069d540 */
extern char sInvalidPLTEAfterIDAT[32];                                  /* 0x0069d560 */
extern char sDuplicatePLTEChunk[32];                                    /* 0x0069d580 */
extern char sInvalidPaletteChunk[32];                                   /* 0x0069d5a0 */
extern char sTruncatingIncorrectTRNSChunkLength[48];                    /* 0x0069d5c0 */
extern char sNoImageInFile[32];                                         /* 0x0069d5f0 */
extern char sIncorrectIENDChunkLength[32];                              /* 0x0069d610 */
extern char sMissingIHDRBeforeGAMA[32];                                 /* 0x0069d630 */
extern char sInvalidGAMAAfterIDAT[32];                                  /* 0x0069d650 */
extern char sOutOfPlaceGAMAChunk[32];                                   /* 0x0069d670 */
extern char sDuplicateGAMAChunk[32];                                    /* 0x0069d690 */
extern char sIncorrectGAMAChunkLength[32];                              /* 0x0069d6b0 */
extern char sIgnoringIncorrectGAMAValueWhenSRGBIsAls[64];               /* 0x0069d6d0 */
extern char sMissingIHDRBeforeSBIT[32];                                 /* 0x0069d710 */
extern char sInvalidSBITAfterIDAT[32];                                  /* 0x0069d730 */
extern char sOutOfPlaceSBITChunk[32];                                   /* 0x0069d750 */
extern char sDuplicateSBITChunk[32];                                    /* 0x0069d770 */
extern char sIncorrectSBITChunkLength[32];                              /* 0x0069d790 */
extern char sInvalidCHRMAfterIDAT[32];                                  /* 0x0069d7b0 */
extern char sMissingPLTEBeforeCHRM[32];                                 /* 0x0069d7d0 */
extern char sDuplicateCHRMChunk[32];                                    /* 0x0069d7f0 */
extern char sIncorrectCHRMChunkLength[32];                              /* 0x0069d810 */
extern char sInvalidCHRMWhitePoint[32];                                 /* 0x0069d830 */
extern char sInvalidCHRMRedPoint[32];                                   /* 0x0069d850 */
extern char sInvalidCHRMGreenPoint[32];                                 /* 0x0069d870 */
extern char sInvalidCHRMBluePoint[32];                                  /* 0x0069d890 */
extern char sIgnoringIncorrectCHRMValueWhenSRGBIsAls[64];               /* 0x0069d8b0 */
extern char sMissingIHDRBeforeSRGB[32];                                 /* 0x0069d8f0 */
extern char sInvalidSRGBAfterIDAT[32];                                  /* 0x0069d910 */
extern char sOutOfPlaceSRGBChunk[32];                                   /* 0x0069d930 */
extern char sDuplicateSRGBChunk[32];                                    /* 0x0069d950 */
extern char sIncorrectSRGBChunkLength[32];                              /* 0x0069d970 */
extern char sUnknownSRGBIntent[32];                                     /* 0x0069d990 */
extern char sMissingIHDRBeforeTRNS[32];                                 /* 0x0069d9b0 */
extern char sInvalidTRNSAfterIDAT[32];                                  /* 0x0069d9d0 */
extern char sDuplicateTRNSChunk[32];                                    /* 0x0069d9f0 */
extern char sMissingPLTEBeforeTRNS[32];                                 /* 0x0069da10 */
extern char sIncorrectTRNSChunkLength[32];                              /* 0x0069da30 */
extern char sZeroLengthTRNSChunk[32];                                   /* 0x0069da50 */
extern char sTRNSChunkNotAllowedWithAlphaChannel[48];                   /* 0x0069da70 */
extern char sMissingIHDRBeforeBKGD[32];                                 /* 0x0069daa0 */
extern char sInvalidBKGDAfterIDAT[32];                                  /* 0x0069dac0 */
extern char sMissingPLTEBeforeBKGD[32];                                 /* 0x0069dae0 */
extern char sDuplicateBKGDChunk[32];                                    /* 0x0069db00 */
extern char sIncorrectBKGDChunkLength[32];                              /* 0x0069db20 */
extern char sIncorrectBKGDChunkIndexValue[48];                          /* 0x0069db40 */
extern char sMissingIHDRBeforeHIST[32];                                 /* 0x0069db70 */
extern char sInvalidHISTAfterIDAT[32];                                  /* 0x0069db90 */
extern char sMissingPLTEBeforeHIST[32];                                 /* 0x0069dbb0 */
extern char sDuplicateHISTChunk[32];                                    /* 0x0069dbd0 */
extern char sIncorrectHISTChunkLength[32];                              /* 0x0069dbf0 */
extern char sMissingIHDRBeforePHYS[32];                                 /* 0x0069dc10 */
extern char sInvalidPHYSAfterIDAT[32];                                  /* 0x0069dc30 */
extern char sDuplicatePHYSChunk[32];                                    /* 0x0069dc50 */
extern char sIncorrectPHYsChunkLength[32];                              /* 0x0069dc70 */
extern char sMissingIHDRBeforeOFFs[32];                                 /* 0x0069dc90 */
extern char sInvalidOFFsAfterIDAT[32];                                  /* 0x0069dcb0 */
extern char sDuplicateOFFsChunk[32];                                    /* 0x0069dcd0 */
extern char sIncorrectOFFsChunkLength[32];                              /* 0x0069dcf0 */
extern char sMissingIHDRBeforePCAL[32];                                 /* 0x0069dd10 */
extern char sInvalidPCALAfterIDAT[32];                                  /* 0x0069dd30 */
extern char sDuplicatePCALChunk[32];                                    /* 0x0069dd50 */
extern char sInvalidPCALData[32];                                       /* 0x0069dd70 */
extern char sInvalidPCALParametersForEquationType[48];                  /* 0x0069dd90 */
extern char sUnrecognizedEquationTypeForPCALChunk[48];                  /* 0x0069ddc0 */
extern char sOutOfPlaceTIMEChunk[32];                                   /* 0x0069ddf0 */
extern char sDuplicateTIMEChunk[32];                                    /* 0x0069de10 */
extern char sIncorrectTIMEChunkLength[32];                              /* 0x0069de30 */
extern char sMissingIHDRBeforeTEXt[32];                                 /* 0x0069de50 */
extern char sMissingIHDRBeforeZTXt[32];                                 /* 0x0069de70 */
extern char sZeroLengthZTXtChunk[32];                                   /* 0x0069de90 */
extern char sUnknownZTXtCompressionType[32];                            /* 0x0069deb0 */
extern char sUnknownCriticalChunk[32];                                  /* 0x0069ded0 */
extern char sInvalidChunkType[32];                                      /* 0x0069def0 */
extern char sIgnoringBadAdaptiveFilterType[48];                         /* 0x0069df10 */
extern char sDecompressionError_0069df40[32];                           /* 0x0069df40 */
extern char sExtraCompressionData[80];                                  /* 0x0069df60 */
extern struct BmxGroup_p_0069dfb0Group p_0069dfb0Group;                 /* 0x0069dfb0 */
extern char sInvalidBlockType[32];                                      /* 0x0069dfe0 */
extern char sInvalidStoredBlockLengths[32];                             /* 0x0069e000 */
extern char sTooManyLengthOrDistanceSymbols[48];                        /* 0x0069e020 */
extern char sInvalidBitLengthRepeat[32];                                /* 0x0069e050 */
extern char sStreamEnd[16];                                             /* 0x0069e070 */
extern char sFileError[16];                                             /* 0x0069e080 */
extern char sStreamError[16];                                           /* 0x0069e090 */
extern char sDataError[16];                                             /* 0x0069e0a0 */
extern char sInsufficientMemory[32];                                    /* 0x0069e0b0 */
extern char sBufferError[16];                                           /* 0x0069e0d0 */
extern char sIncompatibleVersion[32];                                   /* 0x0069e0e0 */
extern uint32_t d_0069e100[20];                                         /* 0x0069e100 */
extern char sWidthTooLargeToProcessImageDataRowbytes[64];               /* 0x0069e150 */
extern struct BmxGroup_p_0069e190Group p_0069e190Group;                 /* 0x0069e190 */
extern char sInvalidLiteralLengthCode[32];                              /* 0x0069e1c0 */
extern char sInvalidDistanceCode[32];                                   /* 0x0069e1e0 */
extern char sOversubscribedDynamicBitLengthsTree[48];                   /* 0x0069e200 */
extern char sIncompleteDynamicBitLengthsTree[48];                       /* 0x0069e230 */
extern char sOversubscribedLiteralLengthTree[48];                       /* 0x0069e260 */
extern char sIncompleteLiteralLengthTree[32];                           /* 0x0069e290 */
extern char sOversubscribedDistanceTree[32];                            /* 0x0069e2b0 */
extern char sIncompleteDistanceTree[32];                                /* 0x0069e2d0 */
extern char sEmptyDistanceTreeWithLengths[48];                          /* 0x0069e2f0 */
extern unsigned char d_0069e320[4];                                     /* 0x0069e320 */
extern float d_0069e324[4];                                             /* 0x0069e324 */
extern unsigned char d_0069e334[4];                                     /* 0x0069e334 */
extern float d_0069e338;                                                /* 0x0069e338 */
extern float d_0069e33c;                                                /* 0x0069e33c */
extern float d_0069e340[2];                                             /* 0x0069e340 */
extern double d_0069e348;                                               /* 0x0069e348 */
extern float d_0069e350;                                                /* 0x0069e350 */
extern float d_0069e354;                                                /* 0x0069e354 */
extern unsigned char d_0069e358[4];                                     /* 0x0069e358 */
extern float d_0069e35c[3];                                             /* 0x0069e35c */
extern float d_0069e368[3];                                             /* 0x0069e368 */
extern unsigned char d_0069e374[4];                                     /* 0x0069e374 */
extern unsigned char d_0069e378[4];                                     /* 0x0069e378 */
extern float d_0069e37c[13];                                            /* 0x0069e37c */
extern float d_0069e3b0[5];                                             /* 0x0069e3b0 */
extern unsigned char d_0069e3c4[4];                                     /* 0x0069e3c4 */
extern unsigned char d_0069e3c8[4];                                     /* 0x0069e3c8 */
extern unsigned char d_0069e3cc[4];                                     /* 0x0069e3cc */
extern unsigned char d_0069e3d0[4];                                     /* 0x0069e3d0 */
extern float d_0069e3d4;                                                /* 0x0069e3d4 */
extern unsigned char d_0069e3d8[4];                                     /* 0x0069e3d8 */
extern unsigned char d_0069e3dc[4];                                     /* 0x0069e3dc */
extern float d_0069e3e0;                                                /* 0x0069e3e0 */
extern unsigned char d_0069e3e4[4];                                     /* 0x0069e3e4 */
extern unsigned char d_0069e3e8[4];                                     /* 0x0069e3e8 */
extern float d_0069e3ec;                                                /* 0x0069e3ec */
extern float d_0069e3f0;                                                /* 0x0069e3f0 */
extern float d_0069e3f4;                                                /* 0x0069e3f4 */
extern float d_0069e3f8[19];                                            /* 0x0069e3f8 */
extern float d_0069e444;                                                /* 0x0069e444 */
extern float d_0069e448;                                                /* 0x0069e448 */
extern float d_0069e44c;                                                /* 0x0069e44c */
extern float d_0069e450;                                                /* 0x0069e450 */
extern float d_0069e454;                                                /* 0x0069e454 */
extern float d_0069e458;                                                /* 0x0069e458 */
extern float d_0069e45c;                                                /* 0x0069e45c */
extern float d_0069e460;                                                /* 0x0069e460 */
extern float d_0069e464;                                                /* 0x0069e464 */
extern float d_0069e468;                                                /* 0x0069e468 */
extern float d_0069e46c;                                                /* 0x0069e46c */
extern float d_0069e470[4];                                             /* 0x0069e470 */
extern char sEntryNumberNotWithinListBounds[48];                        /* 0x0069e480 */
extern char sAttemptingToRemoveMoreEntriesThanExist[48];                /* 0x0069e4b0 */
extern char sSListIndex[2982];                                          /* 0x0069e4e0 */
extern unsigned char d_006a0000[4];                                     /* 0x006a0000 */
extern unsigned char d_006a0004[261];                                   /* 0x006a0004 */

#endif
