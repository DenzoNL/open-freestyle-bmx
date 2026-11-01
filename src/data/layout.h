/*
 * The layout of the original's data blocks (docs/PORTING.md X1b): one struct per block, its members the
 * globals at their original offsets (asserted below), the bytes nothing names as d_/p_/pad_<address>
 * and strings as s<Text>. The values are in the block's file (src/data/<block>.c).
 *
 * Generated once by tools/gen_data.py from NgBMX.exe; from here on it is source like the code.
 */
#ifndef BMX_DATA_LAYOUT_H
#define BMX_DATA_LAYOUT_H

/* The headers' ORIG_VAR macros of these names: here they are the members. */
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
#undef gComdataRecords
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
#undef rwMatrixNormalLimit0
#undef rwMatrixNormalLimit1
#undef rwMatrixNormalLimit2
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

/* The original's .rdata, 0x0054d000..0x00550bd8. */
struct BmxRdata {
    uint32_t d_0054d000[7];                   /* 0x0054d000 */
    uint32_t d_0054d01c;                      /* 0x0054d01c */
    float d_0054d020;                         /* 0x0054d020 */
    float d_0054d024;                         /* 0x0054d024 */
    float d_0054d028;                         /* 0x0054d028 */
    float d_0054d02c;                         /* 0x0054d02c */
    float d_0054d030;                         /* 0x0054d030 */
    float d_0054d034;                         /* 0x0054d034 */
    float d_0054d038;                         /* 0x0054d038 */
    float d_0054d03c;                         /* 0x0054d03c */
    float d_0054d040;                         /* 0x0054d040 */
    float d_0054d044;                         /* 0x0054d044 */
    float d_0054d048;                         /* 0x0054d048 */
    float d_0054d04c;                         /* 0x0054d04c */
    float d_0054d050;                         /* 0x0054d050 */
    float d_0054d054;                         /* 0x0054d054 */
    float d_0054d058;                         /* 0x0054d058 */
    float gRateB;                             /* 0x0054d05c */
    float d_0054d060;                         /* 0x0054d060 */
    float d_0054d064;                         /* 0x0054d064 */
    float d_0054d068;                         /* 0x0054d068 */
    float d_0054d06c;                         /* 0x0054d06c */
    float d_0054d070;                         /* 0x0054d070 */
    float gScoreRound;                        /* 0x0054d074 */
    float d_0054d078;                         /* 0x0054d078 */
    float d_0054d07c;                         /* 0x0054d07c */
    float d_0054d080[3];                      /* 0x0054d080 */
    float d_0054d08c;                         /* 0x0054d08c */
    float d_0054d090;                         /* 0x0054d090 */
    float d_0054d094;                         /* 0x0054d094 */
    float d_0054d098;                         /* 0x0054d098 */
    float d_0054d09c;                         /* 0x0054d09c */
    float d_0054d0a0;                         /* 0x0054d0a0 */
    float d_0054d0a4;                         /* 0x0054d0a4 */
    float d_0054d0a8;                         /* 0x0054d0a8 */
    uint32_t d_0054d0ac;                      /* 0x0054d0ac */
    float d_0054d0b0;                         /* 0x0054d0b0 */
    uint32_t d_0054d0b4;                      /* 0x0054d0b4 */
    float d_0054d0b8;                         /* 0x0054d0b8 */
    float d_0054d0bc;                         /* 0x0054d0bc */
    uint32_t d_0054d0c0;                      /* 0x0054d0c0 */
    float d_0054d0c4;                         /* 0x0054d0c4 */
    float d_0054d0c8;                         /* 0x0054d0c8 */
    float d_0054d0cc;                         /* 0x0054d0cc */
    float d_0054d0d0;                         /* 0x0054d0d0 */
    float d_0054d0d4;                         /* 0x0054d0d4 */
    float d_0054d0d8;                         /* 0x0054d0d8 */
    float d_0054d0dc;                         /* 0x0054d0dc */
    float d_0054d0e0;                         /* 0x0054d0e0 */
    float d_0054d0e4;                         /* 0x0054d0e4 */
    float d_0054d0e8;                         /* 0x0054d0e8 */
    float gScoreToTime;                       /* 0x0054d0ec */
    float d_0054d0f0;                         /* 0x0054d0f0 */
    float d_0054d0f4;                         /* 0x0054d0f4 */
    float d_0054d0f8;                         /* 0x0054d0f8 */
    float d_0054d0fc;                         /* 0x0054d0fc */
    float d_0054d100;                         /* 0x0054d100 */
    float d_0054d104;                         /* 0x0054d104 */
    float d_0054d108;                         /* 0x0054d108 */
    float d_0054d10c;                         /* 0x0054d10c */
    float d_0054d110;                         /* 0x0054d110 */
    float d_0054d114;                         /* 0x0054d114 */
    float d_0054d118;                         /* 0x0054d118 */
    float d_0054d11c;                         /* 0x0054d11c */
    uint32_t d_0054d120;                      /* 0x0054d120 */
    float d_0054d124;                         /* 0x0054d124 */
    float d_0054d128;                         /* 0x0054d128 */
    float d_0054d12c;                         /* 0x0054d12c */
    float d_0054d130;                         /* 0x0054d130 */
    float d_0054d134;                         /* 0x0054d134 */
    float d_0054d138;                         /* 0x0054d138 */
    float d_0054d13c;                         /* 0x0054d13c */
    float d_0054d140;                         /* 0x0054d140 */
    float d_0054d144;                         /* 0x0054d144 */
    float d_0054d148;                         /* 0x0054d148 */
    float d_0054d14c;                         /* 0x0054d14c */
    float d_0054d150;                         /* 0x0054d150 */
    float d_0054d154;                         /* 0x0054d154 */
    uint32_t d_0054d158;                      /* 0x0054d158 */
    float d_0054d15c;                         /* 0x0054d15c */
    float d_0054d160;                         /* 0x0054d160 */
    float d_0054d164;                         /* 0x0054d164 */
    float d_0054d168;                         /* 0x0054d168 */
    float d_0054d16c;                         /* 0x0054d16c */
    float d_0054d170;                         /* 0x0054d170 */
    float d_0054d174;                         /* 0x0054d174 */
    float d_0054d178;                         /* 0x0054d178 */
    float d_0054d17c;                         /* 0x0054d17c */
    float d_0054d180;                         /* 0x0054d180 */
    float d_0054d184;                         /* 0x0054d184 */
    float d_0054d188;                         /* 0x0054d188 */
    float d_0054d18c;                         /* 0x0054d18c */
    float d_0054d190;                         /* 0x0054d190 */
    float d_0054d194;                         /* 0x0054d194 */
    float d_0054d198;                         /* 0x0054d198 */
    float d_0054d19c;                         /* 0x0054d19c */
    float d_0054d1a0;                         /* 0x0054d1a0 */
    float d_0054d1a4;                         /* 0x0054d1a4 */
    float d_0054d1a8;                         /* 0x0054d1a8 */
    float d_0054d1ac;                         /* 0x0054d1ac */
    float d_0054d1b0;                         /* 0x0054d1b0 */
    float d_0054d1b4;                         /* 0x0054d1b4 */
    float d_0054d1b8;                         /* 0x0054d1b8 */
    float d_0054d1bc;                         /* 0x0054d1bc */
    uint32_t d_0054d1c0;                      /* 0x0054d1c0 */
    uint32_t d_0054d1c4;                      /* 0x0054d1c4 */
    float d_0054d1c8;                         /* 0x0054d1c8 */
    float d_0054d1cc;                         /* 0x0054d1cc */
    uint32_t d_0054d1d0;                      /* 0x0054d1d0 */
    uint32_t d_0054d1d4;                      /* 0x0054d1d4 */
    float d_0054d1d8;                         /* 0x0054d1d8 */
    float d_0054d1dc;                         /* 0x0054d1dc */
    float d_0054d1e0;                         /* 0x0054d1e0 */
    float d_0054d1e4;                         /* 0x0054d1e4 */
    float d_0054d1e8;                         /* 0x0054d1e8 */
    float d_0054d1ec;                         /* 0x0054d1ec */
    uint32_t d_0054d1f0;                      /* 0x0054d1f0 */
    float d_0054d1f4;                         /* 0x0054d1f4 */
    float d_0054d1f8;                         /* 0x0054d1f8 */
    float d_0054d1fc;                         /* 0x0054d1fc */
    float d_0054d200;                         /* 0x0054d200 */
    float d_0054d204;                         /* 0x0054d204 */
    float d_0054d208[2];                      /* 0x0054d208 */
    unsigned char d_0054d210[12];             /* 0x0054d210 */
    float d_0054d21c;                         /* 0x0054d21c */
    float gStripScale1;                       /* 0x0054d220 */
    SelIdTable gSelPicIds;                    /* 0x0054d224 */
    SelIdTable gSelTextIds;                   /* 0x0054d230 */
    SelAssetTable gRiderTextures;             /* 0x0054d23c */
    SelAssetTable gRiderNames;                /* 0x0054d254, also gRiderNameIds */
    StatWidthTable gStatWidths;               /* 0x0054d26c */
    SelWideTable gLevelTextures;              /* 0x0054d35c */
    SelWideTable gLevelNames;                 /* 0x0054d38c, also gHiscoreTitleIds */
    SelAsset60 gStyleTextures;                /* 0x0054d3bc */
    SelAssetTable gStyleGroups;               /* 0x0054d434 */
    StatLayerTable gStatLayers;               /* 0x0054d44c */
    StatPosTable gStatPositions;              /* 0x0054d80c */
    SelAsset60 gBikePalettes;                 /* 0x0054d82c */
    BikeStatTable gBikeStats;                 /* 0x0054d8a4 */
    unsigned char pad_0054d9d0[392];          /* 0x0054d9d0 */
    LevelIdTable gHiscorePictureIds;          /* 0x0054db58 */
    unsigned char pad_0054db88[12];           /* 0x0054db88 */
    uint32_t d_0054db94;                      /* 0x0054db94 */
    float d_0054db98;                         /* 0x0054db98 */
    float d_0054db9c;                         /* 0x0054db9c */
    float d_0054dba0;                         /* 0x0054dba0 */
    float d_0054dba4;                         /* 0x0054dba4 */
    float d_0054dba8;                         /* 0x0054dba8 */
    float d_0054dbac;                         /* 0x0054dbac */
    uint32_t d_0054dbb0;                      /* 0x0054dbb0 */
    float d_0054dbb4;                         /* 0x0054dbb4 */
    float gRateC;                             /* 0x0054dbb8 */
    uint32_t d_0054dbbc;                      /* 0x0054dbbc */
    float d_0054dbc0[2];                      /* 0x0054dbc0 */
    double d_0054dbc8[2];                     /* 0x0054dbc8 */
    double d_0054dbd8[2];                     /* 0x0054dbd8 */
    uint32_t d_0054dbe8[3];                   /* 0x0054dbe8 */
    uint32_t d_0054dbf4;                      /* 0x0054dbf4 */
    float d_0054dbf8;                         /* 0x0054dbf8 */
    float d_0054dbfc;                         /* 0x0054dbfc */
    float d_0054dc00;                         /* 0x0054dc00 */
    float d_0054dc04;                         /* 0x0054dc04 */
    float d_0054dc08;                         /* 0x0054dc08 */
    float d_0054dc0c;                         /* 0x0054dc0c */
    double d_0054dc10[2];                     /* 0x0054dc10 */
    uint32_t d_0054dc20[4];                   /* 0x0054dc20 */
    double d_0054dc30[2];                     /* 0x0054dc30 */
    float d_0054dc40[3];                      /* 0x0054dc40 */
    float d_0054dc4c;                         /* 0x0054dc4c */
    uint32_t d_0054dc50;                      /* 0x0054dc50 */
    char sMbP[8];                             /* 0x0054dc54 */
    float d_0054dc5c;                         /* 0x0054dc5c */
    float d_0054dc60[2];                      /* 0x0054dc60 */
    double d_0054dc68[2];                     /* 0x0054dc68 */
    double d_0054dc78[2];                     /* 0x0054dc78 */
    uint32_t d_0054dc88[3];                   /* 0x0054dc88 */
    float d_0054dc94;                         /* 0x0054dc94 */
    float d_0054dc98;                         /* 0x0054dc98 */
    float d_0054dc9c;                         /* 0x0054dc9c */
    float d_0054dca0;                         /* 0x0054dca0 */
    float d_0054dca4;                         /* 0x0054dca4 */
    float d_0054dca8;                         /* 0x0054dca8 */
    float d_0054dcac;                         /* 0x0054dcac */
    float d_0054dcb0;                         /* 0x0054dcb0 */
    uint32_t d_0054dcb4;                      /* 0x0054dcb4 */
    float d_0054dcb8;                         /* 0x0054dcb8 */
    float d_0054dcbc;                         /* 0x0054dcbc */
    float d_0054dcc0;                         /* 0x0054dcc0 */
    float d_0054dcc4;                         /* 0x0054dcc4 */
    uint32_t d_0054dcc8[2];                   /* 0x0054dcc8 */
    double d_0054dcd0;                        /* 0x0054dcd0 */
    float d_0054dcd8;                         /* 0x0054dcd8 */
    float d_0054dcdc;                         /* 0x0054dcdc */
    float d_0054dce0;                         /* 0x0054dce0 */
    uint32_t d_0054dce4;                      /* 0x0054dce4 */
    uint32_t gZrwKeyActions[256];             /* 0x0054dce8; the type is local to zge/com/zglrw/zrwzgl.c */
    float d_0054e0e8[2];                      /* 0x0054e0e8 */
    uint32_t d_0054e0f0[4];                   /* 0x0054e0f0 */
    float d_0054e100[2];                      /* 0x0054e100 */
    uint32_t d_0054e108[7];                   /* 0x0054e108 */
    uint32_t d_0054e124;                      /* 0x0054e124 */
    float d_0054e128;                         /* 0x0054e128 */
    float d_0054e12c;                         /* 0x0054e12c */
    uint32_t gZrwSeekOrigins[3];              /* 0x0054e130; the type is local to zge/com/zglrw/zrwfile.c */
    float d_0054e13c;                         /* 0x0054e13c */
    float d_0054e140;                         /* 0x0054e140 */
    float d_0054e144;                         /* 0x0054e144 */
    float d_0054e148[2];                      /* 0x0054e148 */
    double d_0054e150;                        /* 0x0054e150 */
    float d_0054e158;                         /* 0x0054e158 */
    char s_0054e15c[4];                       /* 0x0054e15c */
    char s_0054e160[4];                       /* 0x0054e160 */
    char sMicrosoftVisualCRuntimeLibrary[40]; /* 0x0054e164 */
    char sPressRetryToDebugTheApplicationJITMustB[64];  /* 0x0054e18c */
    char sForInformationOnHowYourProgramCanCauseA[112]; /* 0x0054e1cc */
    char sExpression[16];                               /* 0x0054e23c */
    char sLine[8];                                      /* 0x0054e24c */
    char sFile[8];                                      /* 0x0054e254 */
    char sProgramNameUnknown[24];                       /* 0x0054e25c */
    char sProgram[12];                                  /* 0x0054e274 */
    char sAssertionFailed[20];                          /* 0x0054e280 */
    unsigned char pad_0054e294[4];                      /* 0x0054e294 */
    uint32_t d_0054e298;                                /* 0x0054e298 */
    void *p_0054e29c;                                   /* 0x0054e29c */
    void *p_0054e2a0;                                   /* 0x0054e2a0 */
    unsigned char pad_0054e2a4[4];                      /* 0x0054e2a4 */
    double d_0054e2a8;                                  /* 0x0054e2a8 */
    double d_0054e2b0;                                  /* 0x0054e2b0 */
    double d_0054e2b8;                                  /* 0x0054e2b8 */
    char sIsProcessorFeaturePresent[28];                /* 0x0054e2c0 */
    char sKERNEL32[12];                                 /* 0x0054e2dc */
    char sE000[8];                                      /* 0x0054e2e8 */
    char sGLOBALHEAPSELECTED[24];                       /* 0x0054e2f0 */
    char sMSVCRTHEAPSELECT[24];                         /* 0x0054e308 */
    char sGetLastActivePopup[20];                       /* 0x0054e320 */
    uint32_t d_0054e334[2];                             /* 0x0054e334 */
    char sEWindow[8];                                   /* 0x0054e33c */
    char sMessageBoxA[12];                              /* 0x0054e344 */
    char sUser32Dll[12];                                /* 0x0054e350 */
    uint32_t d_0054e35c[15];                            /* 0x0054e35c */
    char s_0054e398[4];                                 /* 0x0054e398 */
    uint32_t d_0054e39c[7];                             /* 0x0054e39c */
    char s_0054e3b8[2];                                 /* 0x0054e3b8 */
    unsigned char d_0054e3ba[14];                       /* 0x0054e3ba */
    char sNull[8];                                      /* 0x0054e3c8 */
    char sRuntimeError[16];                             /* 0x0054e3d0 */
    char s_0054e3e0[4];                                 /* 0x0054e3e0 */
    char sTLOSSError[16];                               /* 0x0054e3e4 */
    char sSINGError[16];                                /* 0x0054e3f4 */
    char sDOMAINError[16];                              /* 0x0054e404 */
    char sR6028UnableToInitializeHeap[40];              /* 0x0054e414 */
    char sR6027NotEnoughSpaceForLowioInitializati[56];  /* 0x0054e43c */
    char sR6026NotEnoughSpaceForStdioInitializati[56];  /* 0x0054e474 */
    char sR6025PureVirtualFunctionCall[40];             /* 0x0054e4ac */
    char sR6024NotEnoughSpaceForOnexitAtexitTable[56];  /* 0x0054e4d4 */
    char sR6019UnableToOpenConsoleDevice[44];           /* 0x0054e50c */
    char sR6018UnexpectedHeapError[36];                 /* 0x0054e538 */
    char sR6017UnexpectedMultithreadLockError[48];      /* 0x0054e55c */
    char sR6016NotEnoughSpaceForThreadData[44];         /* 0x0054e58c */
    char sAbnormalProgramTermination[36];               /* 0x0054e5b8 */
    char sR6009NotEnoughSpaceForEnvironment[44];        /* 0x0054e5dc */
    char sR6008NotEnoughSpaceForArguments[44];          /* 0x0054e608 */
    char sR6002FloatingPointNotLoaded[40];              /* 0x0054e634 */
    char sRuntimeErrorProgram[28];                      /* 0x0054e65c */
    unsigned char pad_0054e678[8];                      /* 0x0054e678 */
    double d_0054e680;                                  /* 0x0054e680 */
    unsigned char d_0054e688[2];                        /* 0x0054e688 */
    unsigned char d_0054e68a[10];                       /* 0x0054e68a */
    uint32_t d_0054e694[2];                             /* 0x0054e694 */
    uint32_t d_0054e69c[2];                             /* 0x0054e69c */
    float d_0054e6a4[2];                                /* 0x0054e6a4 */
    float d_0054e6ac[2];                                /* 0x0054e6ac */
    uint32_t d_0054e6b4[2];                             /* 0x0054e6b4 */
    unsigned char d_0054e6bc[8];                        /* 0x0054e6bc */
    char sYn[4];                                        /* 0x0054e6c4 */
    char sY1[4];                                        /* 0x0054e6c8 */
    char sY0[4];                                        /* 0x0054e6cc */
    char sFrexp[8];                                     /* 0x0054e6d0 */
    char sFmod[8];                                      /* 0x0054e6d8 */
    char sHypot[8];                                     /* 0x0054e6e0 */
    char sCabs[8];                                      /* 0x0054e6e8 */
    char sLdexp[8];                                     /* 0x0054e6f0 */
    char sModf[8];                                      /* 0x0054e6f8 */
    char sFabs[8];                                      /* 0x0054e700 */
    char sFloor[8];                                     /* 0x0054e708 */
    char sCeil[8];                                      /* 0x0054e710 */
    char sTan[4];                                       /* 0x0054e718 */
    char sCos[4];                                       /* 0x0054e71c */
    char sSin[4];                                       /* 0x0054e720 */
    char sSqrt[8];                                      /* 0x0054e724 */
    char sAtan2[8];                                     /* 0x0054e72c */
    char sAtan[8];                                      /* 0x0054e734 */
    char sAcos[8];                                      /* 0x0054e73c */
    char sAsin[8];                                      /* 0x0054e744 */
    char sTanh[8];                                      /* 0x0054e74c */
    char sCosh[8];                                      /* 0x0054e754 */
    char sSinh[8];                                      /* 0x0054e75c */
    char sLog10[8];                                     /* 0x0054e764 */
    char sLog[4];                                       /* 0x0054e76c */
    char sPow[4];                                       /* 0x0054e770 */
    char sExp[4];                                       /* 0x0054e774 */
    unsigned char d_0054e778[4];                        /* 0x0054e778 */
    unsigned char d_0054e77c[4];                        /* 0x0054e77c */
    uint32_t d_0054e780;                                /* 0x0054e780 */
    void *p_0054e784;                                   /* 0x0054e784 */
    void *p_0054e788;                                   /* 0x0054e788 */
    uint32_t d_0054e78c;                                /* 0x0054e78c */
    void *p_0054e790;                                   /* 0x0054e790 */
    void *p_0054e794;                                   /* 0x0054e794 */
    uint32_t d_0054e798;                                /* 0x0054e798 */
    void *p_0054e79c;                                   /* 0x0054e79c */
    void *p_0054e7a0;                                   /* 0x0054e7a0 */
    char s1QNAN[8];                                     /* 0x0054e7a4 */
    char s1INF[8];                                      /* 0x0054e7ac */
    char s1IND[8];                                      /* 0x0054e7b4 */
    char s1SNAN[8];                                     /* 0x0054e7bc */
    unsigned char pad_0054e7c4[4];                      /* 0x0054e7c4 */
    uint8_t gZpadFormatJoystick;                        /* 0x0054e7c8 */
    unsigned char d_0054e7c9[19];                       /* 0x0054e7c9 */
    void *p_0054e7dc;                                   /* 0x0054e7dc */
    uint8_t gZpadFormatKeyboard;                        /* 0x0054e7e0 */
    unsigned char d_0054e7e1[19];                       /* 0x0054e7e1 */
    void *p_0054e7f4;                                   /* 0x0054e7f4 */
    uint32_t d_0054e7f8[704];                           /* 0x0054e7f8 */
    ZpadGuid gZpadGuidKeyboard;                         /* 0x0054f2f8 */
    uint32_t d_0054f308[8];                             /* 0x0054f308 */
    uint32_t d_0054f328[4];                             /* 0x0054f328 */
    uint32_t d_0054f338[8];                             /* 0x0054f338 */
    uint32_t d_0054f358[4];                             /* 0x0054f358 */
    uint32_t d_0054f368[4];                             /* 0x0054f368 */
    uint32_t d_0054f378[4];                             /* 0x0054f378 */
    uint32_t d_0054f388[4];                             /* 0x0054f388 */
    uint32_t d_0054f398[4];                             /* 0x0054f398 */
    uint32_t d_0054f3a8[4];                             /* 0x0054f3a8 */
    uint32_t d_0054f3b8[20];                            /* 0x0054f3b8 */
    ZpadGuid gZpadIidDevice;                            /* 0x0054f408 */
    uint32_t d_0054f418[12];                            /* 0x0054f418 */
    ZpadGuid gZpadIidDirectInput;                       /* 0x0054f448 */
    uint32_t d_0054f458[676];                           /* 0x0054f458 */
    uint32_t d_0054fee8[32];                            /* 0x0054fee8 */
    uint32_t d_0054ff68[8];                             /* 0x0054ff68 */
    uint32_t d_0054ff88[4];                             /* 0x0054ff88 */
    uint32_t d_0054ff98[12];                            /* 0x0054ff98 */
    uint32_t d_0054ffc8[132];                           /* 0x0054ffc8 */
    uint32_t d_005501d8[4];                             /* 0x005501d8 */
    uint32_t d_005501e8[20];                            /* 0x005501e8 */
    double d_00550238;                                  /* 0x00550238 */
    uint32_t d_00550240;                                /* 0x00550240 */
    void *p_00550244;                                   /* 0x00550244 */
    void *p_00550248;                                   /* 0x00550248 */
    char sSunMonTueWedThuFriSat[23];                    /* 0x0055024c */
    unsigned char pad_00550263[1];                      /* 0x00550263 */
    char sJanFebMarAprMayJunJulAugSepOctNovDec[40];     /* 0x00550264 */
    char sTZ[4];                                        /* 0x0055028c */
    uint32_t d_00550290;                                /* 0x00550290 */
    void *p_00550294;                                   /* 0x00550294 */
    void *p_00550298;                                   /* 0x00550298 */
    uint32_t d_0055029c;                                /* 0x0055029c */
    void *p_005502a0;                                   /* 0x005502a0 */
    void *p_005502a4;                                   /* 0x005502a4 */
    unsigned char pad_005502a8[2352];                   /* 0x005502a8 */
};

#define AT(member, addr) _Static_assert(offsetof(struct BmxRdata, member) == (addr) - 0x0054d000, #member)
AT(gRateB, 0x0054d05c);
AT(gScoreRound, 0x0054d074);
AT(gScoreToTime, 0x0054d0ec);
AT(gStripScale1, 0x0054d220);
AT(gSelPicIds, 0x0054d224);
AT(gSelTextIds, 0x0054d230);
AT(gRiderTextures, 0x0054d23c);
AT(gRiderNames, 0x0054d254);
AT(gStatWidths, 0x0054d26c);
AT(gLevelTextures, 0x0054d35c);
AT(gLevelNames, 0x0054d38c);
AT(gStyleTextures, 0x0054d3bc);
AT(gStyleGroups, 0x0054d434);
AT(gStatLayers, 0x0054d44c);
AT(gStatPositions, 0x0054d80c);
AT(gBikePalettes, 0x0054d82c);
AT(gBikeStats, 0x0054d8a4);
AT(gHiscorePictureIds, 0x0054db58);
AT(gRateC, 0x0054dbb8);
AT(gZrwKeyActions, 0x0054dce8);
AT(gZrwSeekOrigins, 0x0054e130);
AT(gZpadFormatJoystick, 0x0054e7c8);
AT(gZpadFormatKeyboard, 0x0054e7e0);
AT(gZpadGuidKeyboard, 0x0054f2f8);
AT(gZpadIidDevice, 0x0054f408);
AT(gZpadIidDirectInput, 0x0054f448);
#undef AT
_Static_assert(offsetof(struct BmxRdata, pad_005502a8) + sizeof(((struct BmxRdata *)0)->pad_005502a8) ==
                   0x3bd8,
               "bmxRdata");

/* The original's .data, initialised part, 0x00551000..0x0056a000. */
struct BmxData {
    unsigned char d_00551000[260];                  /* 0x00551000 */
    unsigned char d_00551104[260];                  /* 0x00551104 */
    unsigned char d_00551208[260];                  /* 0x00551208 */
    void *p_0055130c;                               /* 0x0055130c */
    void *p_00551310;                               /* 0x00551310 */
    unsigned char pad_00551314[260];                /* 0x00551314 */
    unsigned char d_00551418[260];                  /* 0x00551418 */
    unsigned char d_0055151c[260];                  /* 0x0055151c */
    void *p_00551620;                               /* 0x00551620 */
    unsigned char pad_00551624[256];                /* 0x00551624 */
    unsigned char d_00551724[260];                  /* 0x00551724 */
    unsigned char d_00551828[260];                  /* 0x00551828 */
    unsigned char d_0055192c[260];                  /* 0x0055192c */
    uint8_t gActionMode;                            /* 0x00551a30 */
    unsigned char pad_00551a31[3];                  /* 0x00551a31 */
    ActionBytes6 gActionDemoTable;                  /* 0x00551a34 */
    unsigned char pad_00551a3a[2];                  /* 0x00551a3a */
    int32_t gActionFrameReload;                     /* 0x00551a3c */
    int32_t gActionFrameCount;                      /* 0x00551a40 */
    unsigned char pad_00551a44[4];                  /* 0x00551a44 */
    char sFailedToOpenMotions[24];                  /* 0x00551a48 */
    unsigned char pad_00551a60[4];                  /* 0x00551a60 */
    char sCZaxisBmxSrcComActionActionC[40];         /* 0x00551a64 */
    unsigned char pad_00551a8c[4];                  /* 0x00551a8c */
    char sZARTVIEWFailedToOpenMotionsS[40];         /* 0x00551a90 */
    unsigned char pad_00551ab8[4];                  /* 0x00551ab8 */
    char sAssetsMOTIONZAL[20];                      /* 0x00551abc */
    unsigned char pad_00551ad0[4];                  /* 0x00551ad0 */
    char sFailedToCreateDemoRider[32];              /* 0x00551ad4 */
    unsigned char pad_00551af4[4];                  /* 0x00551af4 */
    char sCodeDoesNotMatchMotionAssetFile[40];      /* 0x00551af8 */
    unsigned char pad_00551b20[8];                  /* 0x00551b20 */
    char sZARTVIEWFailedToOpenMotionBankD[44];      /* 0x00551b28 */
    unsigned char pad_00551b54[8];                  /* 0x00551b54 */
    char sFailedToLoadAssets[24];                   /* 0x00551b5c */
    unsigned char pad_00551b74[4];                  /* 0x00551b74 */
    char sFailedToCreateRiderLd[28];                /* 0x00551b78 */
    unsigned char pad_00551b94[8];                  /* 0x00551b94 */
    char sAPPShutdownACTION[24];                    /* 0x00551b9c */
    unsigned char pad_00551bb4[4];                  /* 0x00551bb4 */
    char sReplayDataNotFoundInAssetLibrary[40];     /* 0x00551bb8 */
    unsigned char pad_00551be0[8];                  /* 0x00551be0 */
    char sReplayDataTooBigForDemoReplayBuffer[44];  /* 0x00551be8 */
    unsigned char pad_00551c14[8];                  /* 0x00551c14 */
    char sFailedToCreateReplayProbablyOutOfRAM[48]; /* 0x00551c1c */
    unsigned char pad_00551c4c[8];                  /* 0x00551c4c */
    CameraTuning gCameraTuning;                     /* 0x00551c54 */
    unsigned char pad_00551ee4[132];                /* 0x00551ee4 */
    char sPlayerNumberOutOfRange[28];               /* 0x00551f68 */
    unsigned char pad_00551f84[4];                  /* 0x00551f84 */
    char sCZaxisBmxSrcComActionCameraC[40];         /* 0x00551f88 */
    unsigned char pad_00551fb0[8];                  /* 0x00551fb0 */
    uint8_t gGameLevelInfoIndex[12];                /* 0x00551fb8, also gActionMapTable; the type is local to
                                                       bmx/com/action/game.c */
    uint32_t d_00551fc4[4];                         /* 0x00551fc4 */
    uint32_t gTitleUnknown551fd4;                   /* 0x00551fd4, also gActionQuit, gNgQuit */
    uint32_t gComdataRecords[2];                    /* 0x00551fd8 */
    char sIracleBoy[12];                            /* 0x00551fe0 */
    uint32_t d_00551fec[2];                         /* 0x00551fec */
    char s_00551ff4[12];                            /* 0x00551ff4 */
    uint32_t d_00552000[2];                         /* 0x00552000 */
    char s_00552008[12];                            /* 0x00552008 */
    uint32_t d_00552014[2];                         /* 0x00552014 */
    char sYTor[8];                                  /* 0x0055201c */
    uint32_t d_00552024[3];                         /* 0x00552024 */
    char s_00552030[8];                             /* 0x00552030 */
    uint32_t d_00552038[3];                         /* 0x00552038 */
    char s_00552044[8];                             /* 0x00552044 */
    uint32_t d_0055204c[3];                         /* 0x0055204c */
    char s_00552058[8];                             /* 0x00552058 */
    unsigned char pad_00552060[4];                  /* 0x00552060 */
    uint32_t d_00552064[2];                         /* 0x00552064 */
    char s_0055206c[8];                             /* 0x0055206c */
    unsigned char pad_00552074[4];                  /* 0x00552074 */
    uint32_t d_00552078[2];                         /* 0x00552078 */
    char s_00552080[8];                             /* 0x00552080 */
    unsigned char pad_00552088[4];                  /* 0x00552088 */
    uint32_t d_0055208c[2];                         /* 0x0055208c */
    char s_00552094[8];                             /* 0x00552094 */
    unsigned char pad_0055209c[4];                  /* 0x0055209c */
    uint32_t d_005520a0[2];                         /* 0x005520a0 */
    char s_005520a8[8];                             /* 0x005520a8 */
    unsigned char pad_005520b0[4];                  /* 0x005520b0 */
    uint32_t d_005520b4[2];                         /* 0x005520b4 */
    char s_005520bc[8];                             /* 0x005520bc */
    unsigned char pad_005520c4[4];                  /* 0x005520c4 */
    uint32_t d_005520c8[2];                         /* 0x005520c8 */
    char s_005520d0[8];                             /* 0x005520d0 */
    uint32_t d_005520d8[3];                         /* 0x005520d8 */
    char s_005520e4[8];                             /* 0x005520e4 */
    unsigned char pad_005520ec[4];                  /* 0x005520ec */
    uint32_t d_005520f0[2];                         /* 0x005520f0 */
    char s_005520f8[8];                             /* 0x005520f8 */
    uint32_t d_00552100[3];                         /* 0x00552100 */
    char s_0055210c[8];                             /* 0x0055210c */
    uint32_t d_00552114[3];                         /* 0x00552114 */
    char s_00552120[12];                            /* 0x00552120 */
    uint32_t d_0055212c[2];                         /* 0x0055212c */
    char s_00552134[12];                            /* 0x00552134 */
    uint32_t d_00552140[2];                         /* 0x00552140 */
    char s_00552148[12];                            /* 0x00552148 */
    uint32_t d_00552154[2];                         /* 0x00552154 */
    char s_0055215c[12];                            /* 0x0055215c */
    uint32_t d_00552168[2];                         /* 0x00552168 */
    char s_00552170[12];                            /* 0x00552170 */
    uint32_t d_0055217c[2];                         /* 0x0055217c */
    char s_00552184[12];                            /* 0x00552184 */
    uint32_t d_00552190[2];                         /* 0x00552190 */
    char s_00552198[12];                            /* 0x00552198 */
    uint32_t d_005521a4[2];                         /* 0x005521a4 */
    char s_005521ac[12];                            /* 0x005521ac */
    uint32_t d_005521b8[2];                         /* 0x005521b8 */
    char sOrCal[8];                                 /* 0x005521c0 */
    uint32_t d_005521c8[3];                         /* 0x005521c8 */
    char s_005521d4[8];                             /* 0x005521d4 */
    uint32_t d_005521dc[3];                         /* 0x005521dc */
    char s_005521e8[8];                             /* 0x005521e8 */
    uint32_t d_005521f0[3];                         /* 0x005521f0 */
    char s_005521fc[8];                             /* 0x005521fc */
    uint32_t d_00552204[3];                         /* 0x00552204 */
    char s_00552210[8];                             /* 0x00552210 */
    uint32_t d_00552218[3];                         /* 0x00552218 */
    char s_00552224[8];                             /* 0x00552224 */
    uint32_t d_0055222c[3];                         /* 0x0055222c */
    char s_00552238[8];                             /* 0x00552238 */
    uint32_t d_00552240[3];                         /* 0x00552240 */
    char s_0055224c[8];                             /* 0x0055224c */
    uint32_t d_00552254[3];                         /* 0x00552254 */
    char s_00552260[8];                             /* 0x00552260 */
    uint32_t d_00552268[3];                         /* 0x00552268 */
    char s_00552274[8];                             /* 0x00552274 */
    uint32_t d_0055227c[3];                         /* 0x0055227c */
    char s_00552288[8];                             /* 0x00552288 */
    uint32_t d_00552290[3];                         /* 0x00552290 */
    char s_0055229c[8];                             /* 0x0055229c */
    uint32_t d_005522a4[3];                         /* 0x005522a4 */
    char s_005522b0[8];                             /* 0x005522b0 */
    uint32_t d_005522b8[3];                         /* 0x005522b8 */
    char s_005522c4[8];                             /* 0x005522c4 */
    uint32_t d_005522cc[3];                         /* 0x005522cc */
    char s_005522d8[8];                             /* 0x005522d8 */
    uint32_t d_005522e0[3];                         /* 0x005522e0 */
    char s_005522ec[8];                             /* 0x005522ec */
    uint32_t d_005522f4[3];                         /* 0x005522f4 */
    char s_00552300[8];                             /* 0x00552300 */
    uint32_t d_00552308[3];                         /* 0x00552308 */
    char s_00552314[8];                             /* 0x00552314 */
    uint32_t d_0055231c[3];                         /* 0x0055231c */
    char s_00552328[8];                             /* 0x00552328 */
    uint32_t d_00552330[3];                         /* 0x00552330 */
    char s_0055233c[8];                             /* 0x0055233c */
    uint32_t d_00552344[3];                         /* 0x00552344 */
    char s_00552350[8];                             /* 0x00552350 */
    uint32_t d_00552358[3];                         /* 0x00552358 */
    char s_00552364[8];                             /* 0x00552364 */
    uint32_t d_0055236c[3];                         /* 0x0055236c */
    char s_00552378[8];                             /* 0x00552378 */
    uint32_t d_00552380[3];                         /* 0x00552380 */
    char s_0055238c[8];                             /* 0x0055238c */
    uint32_t d_00552394[3];                         /* 0x00552394 */
    char sArpig[8];                                 /* 0x005523a0 */
    uint32_t d_005523a8[3];                         /* 0x005523a8 */
    char s_005523b4[8];                             /* 0x005523b4 */
    uint32_t d_005523bc[3];                         /* 0x005523bc */
    char s_005523c8[8];                             /* 0x005523c8 */
    uint32_t d_005523d0[3];                         /* 0x005523d0 */
    char sEbby[8];                                  /* 0x005523dc */
    uint32_t d_005523e4[3];                         /* 0x005523e4 */
    char s_005523f0[8];                             /* 0x005523f0 */
    uint32_t d_005523f8[3];                         /* 0x005523f8 */
    char s_00552404[8];                             /* 0x00552404 */
    uint32_t d_0055240c[3];                         /* 0x0055240c */
    char s_00552418[8];                             /* 0x00552418 */
    uint32_t d_00552420[3];                         /* 0x00552420 */
    char s_0055242c[8];                             /* 0x0055242c */
    uint32_t d_00552434[3];                         /* 0x00552434 */
    char s_00552440[8];                             /* 0x00552440 */
    uint32_t d_00552448[3];                         /* 0x00552448 */
    char s_00552454[8];                             /* 0x00552454 */
    uint32_t d_0055245c[3];                         /* 0x0055245c */
    char s_00552468[8];                             /* 0x00552468 */
    uint32_t d_00552470[3];                         /* 0x00552470 */
    char s_0055247c[8];                             /* 0x0055247c */
    uint32_t d_00552484[3];                         /* 0x00552484 */
    char s_00552490[8];                             /* 0x00552490 */
    uint32_t d_00552498[3];                         /* 0x00552498 */
    char s_005524a4[8];                             /* 0x005524a4 */
    uint32_t d_005524ac[3];                         /* 0x005524ac */
    char s_005524b8[8];                             /* 0x005524b8 */
    uint32_t d_005524c0[3];                         /* 0x005524c0 */
    char s_005524cc[8];                             /* 0x005524cc */
    uint32_t d_005524d4[3];                         /* 0x005524d4 */
    char s_005524e0[8];                             /* 0x005524e0 */
    uint32_t d_005524e8[3];                         /* 0x005524e8 */
    char s_005524f4[8];                             /* 0x005524f4 */
    uint32_t d_005524fc[3];                         /* 0x005524fc */
    char s_00552508[8];                             /* 0x00552508 */
    uint32_t d_00552510[3];                         /* 0x00552510 */
    char s_0055251c[8];                             /* 0x0055251c */
    uint32_t d_00552524[3];                         /* 0x00552524 */
    char s_00552530[8];                             /* 0x00552530 */
    uint32_t d_00552538[3];                         /* 0x00552538 */
    char s_00552544[8];                             /* 0x00552544 */
    uint32_t d_0055254c[3];                         /* 0x0055254c */
    char s_00552558[8];                             /* 0x00552558 */
    uint32_t d_00552560[3];                         /* 0x00552560 */
    char s_0055256c[8];                             /* 0x0055256c */
    uint32_t d_00552574[3];                         /* 0x00552574 */
    char sBreed[8];                                 /* 0x00552580 */
    uint32_t d_00552588[3];                         /* 0x00552588 */
    char s_00552594[8];                             /* 0x00552594 */
    uint32_t d_0055259c[3];                         /* 0x0055259c */
    char s_005525a8[8];                             /* 0x005525a8 */
    uint32_t d_005525b0[3];                         /* 0x005525b0 */
    char s_005525bc[8];                             /* 0x005525bc */
    uint32_t d_005525c4[3];                         /* 0x005525c4 */
    char s_005525d0[8];                             /* 0x005525d0 */
    uint32_t d_005525d8[3];                         /* 0x005525d8 */
    char s_005525e4[8];                             /* 0x005525e4 */
    uint32_t d_005525ec[3];                         /* 0x005525ec */
    char s_005525f8[8];                             /* 0x005525f8 */
    uint32_t d_00552600[3];                         /* 0x00552600 */
    char s_0055260c[8];                             /* 0x0055260c */
    uint32_t d_00552614[3];                         /* 0x00552614 */
    char s_00552620[8];                             /* 0x00552620 */
    uint32_t d_00552628[3];                         /* 0x00552628 */
    char s_00552634[8];                             /* 0x00552634 */
    uint32_t d_0055263c[3];                         /* 0x0055263c */
    char s_00552648[8];                             /* 0x00552648 */
    uint32_t d_00552650[3];                         /* 0x00552650 */
    char s_0055265c[8];                             /* 0x0055265c */
    uint32_t d_00552664[3];                         /* 0x00552664 */
    char s_00552670[8];                             /* 0x00552670 */
    uint32_t d_00552678[3];                         /* 0x00552678 */
    char s_00552684[8];                             /* 0x00552684 */
    uint32_t d_0055268c[3];                         /* 0x0055268c */
    char s_00552698[8];                             /* 0x00552698 */
    uint32_t d_005526a0[3];                         /* 0x005526a0 */
    char s_005526ac[8];                             /* 0x005526ac */
    uint32_t d_005526b4[3];                         /* 0x005526b4 */
    char s_005526c0[8];                             /* 0x005526c0 */
    uint32_t d_005526c8[3];                         /* 0x005526c8 */
    char s_005526d4[8];                             /* 0x005526d4 */
    uint32_t d_005526dc[3];                         /* 0x005526dc */
    char s_005526e8[8];                             /* 0x005526e8 */
    uint32_t d_005526f0[3];                         /* 0x005526f0 */
    char s_005526fc[8];                             /* 0x005526fc */
    uint32_t d_00552704[3];                         /* 0x00552704 */
    char s_00552710[8];                             /* 0x00552710 */
    uint32_t d_00552718[3];                         /* 0x00552718 */
    char s_00552724[8];                             /* 0x00552724 */
    uint32_t d_0055272c[3];                         /* 0x0055272c */
    char s_00552738[8];                             /* 0x00552738 */
    uint32_t d_00552740[3];                         /* 0x00552740 */
    char s_0055274c[8];                             /* 0x0055274c */
    uint32_t d_00552754[3];                         /* 0x00552754 */
    char sAGY[8];                                   /* 0x00552760 */
    uint32_t d_00552768[3];                         /* 0x00552768 */
    char s_00552774[8];                             /* 0x00552774 */
    uint32_t d_0055277c[3];                         /* 0x0055277c */
    char s_00552788[8];                             /* 0x00552788 */
    uint32_t d_00552790[3];                         /* 0x00552790 */
    char s_0055279c[8];                             /* 0x0055279c */
    uint32_t d_005527a4[3];                         /* 0x005527a4 */
    char s_005527b0[8];                             /* 0x005527b0 */
    uint32_t d_005527b8[3];                         /* 0x005527b8 */
    char s_005527c4[8];                             /* 0x005527c4 */
    uint32_t d_005527cc[3];                         /* 0x005527cc */
    char s_005527d8[8];                             /* 0x005527d8 */
    uint32_t d_005527e0[3];                         /* 0x005527e0 */
    char s_005527ec[8];                             /* 0x005527ec */
    uint32_t d_005527f4[3];                         /* 0x005527f4 */
    char s_00552800[8];                             /* 0x00552800 */
    uint32_t d_00552808[3];                         /* 0x00552808 */
    char s_00552814[8];                             /* 0x00552814 */
    uint32_t d_0055281c[3];                         /* 0x0055281c */
    char s_00552828[8];                             /* 0x00552828 */
    uint32_t d_00552830[3];                         /* 0x00552830 */
    char s_0055283c[8];                             /* 0x0055283c */
    uint32_t d_00552844[3];                         /* 0x00552844 */
    char s_00552850[8];                             /* 0x00552850 */
    uint32_t d_00552858[3];                         /* 0x00552858 */
    char s_00552864[8];                             /* 0x00552864 */
    uint32_t d_0055286c[3];                         /* 0x0055286c */
    char s_00552878[8];                             /* 0x00552878 */
    uint32_t d_00552880[3];                         /* 0x00552880 */
    char s_0055288c[8];                             /* 0x0055288c */
    uint32_t d_00552894[3];                         /* 0x00552894 */
    char s_005528a0[8];                             /* 0x005528a0 */
    uint32_t d_005528a8[3];                         /* 0x005528a8 */
    char s_005528b4[8];                             /* 0x005528b4 */
    uint32_t d_005528bc[3];                         /* 0x005528bc */
    char s_005528c8[8];                             /* 0x005528c8 */
    uint32_t d_005528d0[3];                         /* 0x005528d0 */
    char s_005528dc[8];                             /* 0x005528dc */
    uint32_t d_005528e4[3];                         /* 0x005528e4 */
    char s_005528f0[8];                             /* 0x005528f0 */
    uint32_t d_005528f8[3];                         /* 0x005528f8 */
    char s_00552904[8];                             /* 0x00552904 */
    uint32_t d_0055290c[3];                         /* 0x0055290c */
    char s_00552918[8];                             /* 0x00552918 */
    uint32_t d_00552920[3];                         /* 0x00552920 */
    char s_0055292c[8];                             /* 0x0055292c */
    uint32_t d_00552934[3];                         /* 0x00552934 */
    char sUrbo[8];                                  /* 0x00552940 */
    uint32_t d_00552948[3];                         /* 0x00552948 */
    char s_00552954[8];                             /* 0x00552954 */
    uint32_t d_0055295c[3];                         /* 0x0055295c */
    char s_00552968[8];                             /* 0x00552968 */
    uint32_t d_00552970[3];                         /* 0x00552970 */
    char s_0055297c[8];                             /* 0x0055297c */
    uint32_t d_00552984[3];                         /* 0x00552984 */
    char s_00552990[8];                             /* 0x00552990 */
    uint32_t d_00552998[3];                         /* 0x00552998 */
    char s_005529a4[8];                             /* 0x005529a4 */
    uint32_t d_005529ac[3];                         /* 0x005529ac */
    char s_005529b8[8];                             /* 0x005529b8 */
    uint32_t d_005529c0[3];                         /* 0x005529c0 */
    char s_005529cc[8];                             /* 0x005529cc */
    uint32_t d_005529d4[3];                         /* 0x005529d4 */
    char s_005529e0[8];                             /* 0x005529e0 */
    uint32_t d_005529e8[3];                         /* 0x005529e8 */
    char s_005529f4[8];                             /* 0x005529f4 */
    uint32_t d_005529fc[3];                         /* 0x005529fc */
    char s_00552a08[8];                             /* 0x00552a08 */
    uint32_t d_00552a10[3];                         /* 0x00552a10 */
    char s_00552a1c[8];                             /* 0x00552a1c */
    uint32_t d_00552a24[3];                         /* 0x00552a24 */
    char s_00552a30[8];                             /* 0x00552a30 */
    uint32_t d_00552a38[3];                         /* 0x00552a38 */
    char s_00552a44[8];                             /* 0x00552a44 */
    uint32_t d_00552a4c[3];                         /* 0x00552a4c */
    char s_00552a58[8];                             /* 0x00552a58 */
    uint32_t d_00552a60[3];                         /* 0x00552a60 */
    char s_00552a6c[8];                             /* 0x00552a6c */
    uint32_t d_00552a74[3];                         /* 0x00552a74 */
    char s_00552a80[8];                             /* 0x00552a80 */
    uint32_t d_00552a88[3];                         /* 0x00552a88 */
    char s_00552a94[8];                             /* 0x00552a94 */
    uint32_t d_00552a9c[3];                         /* 0x00552a9c */
    char s_00552aa8[8];                             /* 0x00552aa8 */
    uint32_t d_00552ab0[3];                         /* 0x00552ab0 */
    char s_00552abc[8];                             /* 0x00552abc */
    uint32_t d_00552ac4[3];                         /* 0x00552ac4 */
    char s_00552ad0[8];                             /* 0x00552ad0 */
    uint32_t d_00552ad8[3];                         /* 0x00552ad8 */
    char s_00552ae4[8];                             /* 0x00552ae4 */
    uint32_t d_00552aec[3];                         /* 0x00552aec */
    char s_00552af8[8];                             /* 0x00552af8 */
    uint32_t d_00552b00[3];                         /* 0x00552b00 */
    char s_00552b0c[8];                             /* 0x00552b0c */
    uint32_t d_00552b14[3];                         /* 0x00552b14 */
    char sArkDog[8];                                /* 0x00552b20 */
    uint32_t d_00552b28[3];                         /* 0x00552b28 */
    char s_00552b34[8];                             /* 0x00552b34 */
    uint32_t d_00552b3c[3];                         /* 0x00552b3c */
    char s_00552b48[8];                             /* 0x00552b48 */
    uint32_t d_00552b50[3];                         /* 0x00552b50 */
    char s_00552b5c[8];                             /* 0x00552b5c */
    uint32_t d_00552b64[3];                         /* 0x00552b64 */
    char s_00552b70[8];                             /* 0x00552b70 */
    uint32_t d_00552b78[3];                         /* 0x00552b78 */
    char s_00552b84[8];                             /* 0x00552b84 */
    uint32_t d_00552b8c[3];                         /* 0x00552b8c */
    char s_00552b98[8];                             /* 0x00552b98 */
    uint32_t d_00552ba0[3];                         /* 0x00552ba0 */
    char s_00552bac[8];                             /* 0x00552bac */
    uint32_t d_00552bb4[3];                         /* 0x00552bb4 */
    char s_00552bc0[8];                             /* 0x00552bc0 */
    uint32_t d_00552bc8[3];                         /* 0x00552bc8 */
    char s_00552bd4[8];                             /* 0x00552bd4 */
    uint32_t d_00552bdc[3];                         /* 0x00552bdc */
    char s_00552be8[8];                             /* 0x00552be8 */
    uint32_t d_00552bf0[3];                         /* 0x00552bf0 */
    char s_00552bfc[8];                             /* 0x00552bfc */
    uint32_t d_00552c04[3];                         /* 0x00552c04 */
    char s_00552c10[8];                             /* 0x00552c10 */
    uint32_t d_00552c18[3];                         /* 0x00552c18 */
    char s_00552c24[8];                             /* 0x00552c24 */
    uint32_t d_00552c2c[3];                         /* 0x00552c2c */
    char s_00552c38[8];                             /* 0x00552c38 */
    uint32_t d_00552c40[3];                         /* 0x00552c40 */
    char s_00552c4c[8];                             /* 0x00552c4c */
    uint32_t d_00552c54[3];                         /* 0x00552c54 */
    char s_00552c60[8];                             /* 0x00552c60 */
    uint32_t d_00552c68[3];                         /* 0x00552c68 */
    char s_00552c74[8];                             /* 0x00552c74 */
    uint32_t d_00552c7c[3];                         /* 0x00552c7c */
    char s_00552c88[8];                             /* 0x00552c88 */
    uint32_t d_00552c90[3];                         /* 0x00552c90 */
    char s_00552c9c[8];                             /* 0x00552c9c */
    uint32_t d_00552ca4[3];                         /* 0x00552ca4 */
    char s_00552cb0[8];                             /* 0x00552cb0 */
    uint32_t d_00552cb8[3];                         /* 0x00552cb8 */
    char s_00552cc4[8];                             /* 0x00552cc4 */
    uint32_t d_00552ccc[3];                         /* 0x00552ccc */
    char s_00552cd8[8];                             /* 0x00552cd8 */
    uint32_t d_00552ce0[3];                         /* 0x00552ce0 */
    char s_00552cec[8];                             /* 0x00552cec */
    uint32_t d_00552cf4[3];                         /* 0x00552cf4 */
    char sUmber[8];                                 /* 0x00552d00 */
    uint32_t d_00552d08[3];                         /* 0x00552d08 */
    char s_00552d14[8];                             /* 0x00552d14 */
    uint32_t d_00552d1c[3];                         /* 0x00552d1c */
    char s_00552d28[8];                             /* 0x00552d28 */
    uint32_t d_00552d30[3];                         /* 0x00552d30 */
    char s_00552d3c[8];                             /* 0x00552d3c */
    uint32_t d_00552d44[3];                         /* 0x00552d44 */
    char s_00552d50[8];                             /* 0x00552d50 */
    uint32_t d_00552d58[3];                         /* 0x00552d58 */
    char s_00552d64[8];                             /* 0x00552d64 */
    uint32_t d_00552d6c[3];                         /* 0x00552d6c */
    char s_00552d78[8];                             /* 0x00552d78 */
    uint32_t d_00552d80[3];                         /* 0x00552d80 */
    char s_00552d8c[8];                             /* 0x00552d8c */
    uint32_t d_00552d94[3];                         /* 0x00552d94 */
    char s_00552da0[8];                             /* 0x00552da0 */
    uint32_t d_00552da8[3];                         /* 0x00552da8 */
    char s_00552db4[8];                             /* 0x00552db4 */
    uint32_t d_00552dbc[3];                         /* 0x00552dbc */
    char s_00552dc8[8];                             /* 0x00552dc8 */
    uint32_t d_00552dd0[3];                         /* 0x00552dd0 */
    char s_00552ddc[8];                             /* 0x00552ddc */
    uint32_t d_00552de4[3];                         /* 0x00552de4 */
    char s_00552df0[8];                             /* 0x00552df0 */
    uint32_t d_00552df8[3];                         /* 0x00552df8 */
    char s_00552e04[8];                             /* 0x00552e04 */
    uint32_t d_00552e0c[3];                         /* 0x00552e0c */
    char s_00552e18[8];                             /* 0x00552e18 */
    uint32_t d_00552e20[3];                         /* 0x00552e20 */
    char s_00552e2c[8];                             /* 0x00552e2c */
    uint32_t d_00552e34[3];                         /* 0x00552e34 */
    char s_00552e40[8];                             /* 0x00552e40 */
    uint32_t d_00552e48[3];                         /* 0x00552e48 */
    char s_00552e54[8];                             /* 0x00552e54 */
    uint32_t d_00552e5c[3];                         /* 0x00552e5c */
    char s_00552e68[8];                             /* 0x00552e68 */
    uint32_t d_00552e70[3];                         /* 0x00552e70 */
    char s_00552e7c[8];                             /* 0x00552e7c */
    uint32_t d_00552e84[3];                         /* 0x00552e84 */
    char s_00552e90[8];                             /* 0x00552e90 */
    uint32_t d_00552e98[3];                         /* 0x00552e98 */
    char s_00552ea4[8];                             /* 0x00552ea4 */
    uint32_t d_00552eac[3];                         /* 0x00552eac */
    char s_00552eb8[8];                             /* 0x00552eb8 */
    uint32_t d_00552ec0[3];                         /* 0x00552ec0 */
    char s_00552ecc[8];                             /* 0x00552ecc */
    uint32_t d_00552ed4[3];                         /* 0x00552ed4 */
    char sAptInsano[12];                            /* 0x00552ee0 */
    uint32_t d_00552eec[2];                         /* 0x00552eec */
    char s_00552ef4[12];                            /* 0x00552ef4 */
    uint32_t d_00552f00[2];                         /* 0x00552f00 */
    char s_00552f08[12];                            /* 0x00552f08 */
    uint32_t d_00552f14[2];                         /* 0x00552f14 */
    char s_00552f1c[8];                             /* 0x00552f1c */
    uint32_t d_00552f24[3];                         /* 0x00552f24 */
    char s_00552f30[8];                             /* 0x00552f30 */
    uint32_t d_00552f38[3];                         /* 0x00552f38 */
    char s_00552f44[8];                             /* 0x00552f44 */
    uint32_t d_00552f4c[3];                         /* 0x00552f4c */
    char s_00552f58[8];                             /* 0x00552f58 */
    uint32_t d_00552f60[3];                         /* 0x00552f60 */
    char s_00552f6c[8];                             /* 0x00552f6c */
    uint32_t d_00552f74[3];                         /* 0x00552f74 */
    char s_00552f80[8];                             /* 0x00552f80 */
    uint32_t d_00552f88[3];                         /* 0x00552f88 */
    char s_00552f94[8];                             /* 0x00552f94 */
    uint32_t d_00552f9c[3];                         /* 0x00552f9c */
    char s_00552fa8[8];                             /* 0x00552fa8 */
    uint32_t d_00552fb0[3];                         /* 0x00552fb0 */
    char s_00552fbc[8];                             /* 0x00552fbc */
    uint32_t d_00552fc4[3];                         /* 0x00552fc4 */
    char s_00552fd0[8];                             /* 0x00552fd0 */
    uint32_t d_00552fd8[3];                         /* 0x00552fd8 */
    char s_00552fe4[8];                             /* 0x00552fe4 */
    uint32_t d_00552fec[3];                         /* 0x00552fec */
    char s_00552ff8[8];                             /* 0x00552ff8 */
    uint32_t d_00553000[3];                         /* 0x00553000 */
    char s_0055300c[8];                             /* 0x0055300c */
    uint32_t d_00553014[3];                         /* 0x00553014 */
    char s_00553020[12];                            /* 0x00553020 */
    uint32_t d_0055302c[2];                         /* 0x0055302c */
    char s_00553034[12];                            /* 0x00553034 */
    uint32_t d_00553040[2];                         /* 0x00553040 */
    char s_00553048[12];                            /* 0x00553048 */
    uint32_t d_00553054[2];                         /* 0x00553054 */
    char s_0055305c[12];                            /* 0x0055305c */
    uint32_t d_00553068[2];                         /* 0x00553068 */
    char s_00553070[12];                            /* 0x00553070 */
    uint32_t d_0055307c[2];                         /* 0x0055307c */
    char s_00553084[12];                            /* 0x00553084 */
    uint32_t d_00553090[2];                         /* 0x00553090 */
    char s_00553098[12];                            /* 0x00553098 */
    uint32_t d_005530a4[2];                         /* 0x005530a4 */
    char s_005530ac[12];                            /* 0x005530ac */
    uint32_t d_005530b8[2];                         /* 0x005530b8 */
    char sHeBoss[8];                                /* 0x005530c0 */
    uint32_t d_005530c8[3];                         /* 0x005530c8 */
    char s_005530d4[8];                             /* 0x005530d4 */
    uint32_t d_005530dc[3];                         /* 0x005530dc */
    char s_005530e8[8];                             /* 0x005530e8 */
    uint32_t d_005530f0[3];                         /* 0x005530f0 */
    char s_005530fc[8];                             /* 0x005530fc */
    uint32_t d_00553104[3];                         /* 0x00553104 */
    char s_00553110[8];                             /* 0x00553110 */
    uint32_t d_00553118[3];                         /* 0x00553118 */
    char s_00553124[8];                             /* 0x00553124 */
    uint32_t d_0055312c[3];                         /* 0x0055312c */
    char s_00553138[8];                             /* 0x00553138 */
    uint32_t d_00553140[3];                         /* 0x00553140 */
    char s_0055314c[8];                             /* 0x0055314c */
    uint32_t d_00553154[3];                         /* 0x00553154 */
    char s_00553160[8];                             /* 0x00553160 */
    uint32_t d_00553168[3];                         /* 0x00553168 */
    char s_00553174[8];                             /* 0x00553174 */
    uint32_t d_0055317c[3];                         /* 0x0055317c */
    char s_00553188[8];                             /* 0x00553188 */
    uint32_t d_00553190[3];                         /* 0x00553190 */
    char s_0055319c[8];                             /* 0x0055319c */
    uint32_t d_005531a4[3];                         /* 0x005531a4 */
    char s_005531b0[8];                             /* 0x005531b0 */
    uint32_t d_005531b8[3];                         /* 0x005531b8 */
    char s_005531c4[8];                             /* 0x005531c4 */
    uint32_t d_005531cc[3];                         /* 0x005531cc */
    char s_005531d8[8];                             /* 0x005531d8 */
    uint32_t d_005531e0[3];                         /* 0x005531e0 */
    char s_005531ec[8];                             /* 0x005531ec */
    uint32_t d_005531f4[3];                         /* 0x005531f4 */
    char s_00553200[8];                             /* 0x00553200 */
    uint32_t d_00553208[3];                         /* 0x00553208 */
    char s_00553214[8];                             /* 0x00553214 */
    uint32_t d_0055321c[3];                         /* 0x0055321c */
    char s_00553228[8];                             /* 0x00553228 */
    uint32_t d_00553230[3];                         /* 0x00553230 */
    char s_0055323c[8];                             /* 0x0055323c */
    uint32_t d_00553244[3];                         /* 0x00553244 */
    char s_00553250[8];                             /* 0x00553250 */
    uint32_t d_00553258[3];                         /* 0x00553258 */
    char s_00553264[8];                             /* 0x00553264 */
    uint32_t d_0055326c[3];                         /* 0x0055326c */
    char s_00553278[8];                             /* 0x00553278 */
    uint32_t d_00553280[3];                         /* 0x00553280 */
    char s_0055328c[8];                             /* 0x0055328c */
    uint32_t d_00553294[3];                         /* 0x00553294 */
    char s_005532a0[12];                            /* 0x005532a0 */
    uint32_t d_005532ac[2];                         /* 0x005532ac */
    char s_005532b4[12];                            /* 0x005532b4 */
    uint32_t d_005532c0[2];                         /* 0x005532c0 */
    char s_005532c8[12];                            /* 0x005532c8 */
    uint32_t d_005532d4[2];                         /* 0x005532d4 */
    char s_005532dc[8];                             /* 0x005532dc */
    uint32_t d_005532e4[3];                         /* 0x005532e4 */
    char s_005532f0[8];                             /* 0x005532f0 */
    uint32_t d_005532f8[3];                         /* 0x005532f8 */
    char s_00553304[8];                             /* 0x00553304 */
    uint32_t d_0055330c[3];                         /* 0x0055330c */
    char s_00553318[8];                             /* 0x00553318 */
    uint32_t d_00553320[3];                         /* 0x00553320 */
    char s_0055332c[8];                             /* 0x0055332c */
    uint32_t d_00553334[3];                         /* 0x00553334 */
    char s_00553340[8];                             /* 0x00553340 */
    uint32_t d_00553348[3];                         /* 0x00553348 */
    char s_00553354[8];                             /* 0x00553354 */
    uint32_t d_0055335c[3];                         /* 0x0055335c */
    char s_00553368[8];                             /* 0x00553368 */
    uint32_t d_00553370[3];                         /* 0x00553370 */
    char s_0055337c[8];                             /* 0x0055337c */
    uint32_t d_00553384[3];                         /* 0x00553384 */
    char s_00553390[8];                             /* 0x00553390 */
    uint32_t d_00553398[3];                         /* 0x00553398 */
    char s_005533a4[8];                             /* 0x005533a4 */
    uint32_t d_005533ac[3];                         /* 0x005533ac */
    char s_005533b8[8];                             /* 0x005533b8 */
    uint32_t d_005533c0[3];                         /* 0x005533c0 */
    char s_005533cc[8];                             /* 0x005533cc */
    uint32_t d_005533d4[3];                         /* 0x005533d4 */
    char s_005533e0[12];                            /* 0x005533e0 */
    uint32_t d_005533ec[2];                         /* 0x005533ec */
    char s_005533f4[12];                            /* 0x005533f4 */
    uint32_t d_00553400[2];                         /* 0x00553400 */
    char s_00553408[12];                            /* 0x00553408 */
    uint32_t d_00553414[2];                         /* 0x00553414 */
    char s_0055341c[12];                            /* 0x0055341c */
    uint32_t d_00553428[2];                         /* 0x00553428 */
    char s_00553430[12];                            /* 0x00553430 */
    uint32_t d_0055343c[2];                         /* 0x0055343c */
    char s_00553444[12];                            /* 0x00553444 */
    uint32_t d_00553450[2];                         /* 0x00553450 */
    char s_00553458[12];                            /* 0x00553458 */
    uint32_t d_00553464[2];                         /* 0x00553464 */
    char s_0055346c[12];                            /* 0x0055346c */
    uint32_t d_00553478[2];                         /* 0x00553478 */
    char s_00553480[12];                            /* 0x00553480 */
    uint32_t d_0055348c[2];                         /* 0x0055348c */
    char s_00553494[12];                            /* 0x00553494 */
    uint32_t d_005534a0[2];                         /* 0x005534a0 */
    char s_005534a8[12];                            /* 0x005534a8 */
    uint32_t d_005534b4[2];                         /* 0x005534b4 */
    char s_005534bc[8];                             /* 0x005534bc */
    uint32_t d_005534c4[3];                         /* 0x005534c4 */
    char s_005534d0[8];                             /* 0x005534d0 */
    uint32_t d_005534d8[3];                         /* 0x005534d8 */
    char s_005534e4[8];                             /* 0x005534e4 */
    uint32_t d_005534ec[3];                         /* 0x005534ec */
    char s_005534f8[8];                             /* 0x005534f8 */
    uint32_t d_00553500[3];                         /* 0x00553500 */
    char s_0055350c[8];                             /* 0x0055350c */
    uint32_t d_00553514[3];                         /* 0x00553514 */
    char s_00553520[8];                             /* 0x00553520 */
    uint32_t d_00553528[3];                         /* 0x00553528 */
    char s_00553534[8];                             /* 0x00553534 */
    uint32_t d_0055353c[3];                         /* 0x0055353c */
    char s_00553548[8];                             /* 0x00553548 */
    uint32_t d_00553550[3];                         /* 0x00553550 */
    char s_0055355c[8];                             /* 0x0055355c */
    uint32_t d_00553564[3];                         /* 0x00553564 */
    char s_00553570[8];                             /* 0x00553570 */
    uint32_t d_00553578[3];                         /* 0x00553578 */
    char s_00553584[8];                             /* 0x00553584 */
    uint32_t d_0055358c[3];                         /* 0x0055358c */
    char s_00553598[8];                             /* 0x00553598 */
    uint32_t d_005535a0[3];                         /* 0x005535a0 */
    char s_005535ac[8];                             /* 0x005535ac */
    uint32_t d_005535b4[3];                         /* 0x005535b4 */
    char s_005535c0[12];                            /* 0x005535c0 */
    uint32_t d_005535cc[2];                         /* 0x005535cc */
    char s_005535d4[12];                            /* 0x005535d4 */
    uint32_t d_005535e0[2];                         /* 0x005535e0 */
    char s_005535e8[12];                            /* 0x005535e8 */
    uint32_t d_005535f4[2];                         /* 0x005535f4 */
    char s_005535fc[12];                            /* 0x005535fc */
    uint32_t d_00553608[2];                         /* 0x00553608 */
    char s_00553610[12];                            /* 0x00553610 */
    uint32_t d_0055361c[2];                         /* 0x0055361c */
    char s_00553624[12];                            /* 0x00553624 */
    uint32_t d_00553630[2];                         /* 0x00553630 */
    char s_00553638[12];                            /* 0x00553638 */
    uint32_t d_00553644[2];                         /* 0x00553644 */
    char s_0055364c[12];                            /* 0x0055364c */
    OvlBytes16 gScoreRecordSlot;                    /* 0x00553658, also gRiderRiderStats */
    float d_00553668[11];                           /* 0x00553668 */
    char s33s33s33s[14];                            /* 0x00553694 */
    unsigned char d_005536a2[2];                    /* 0x005536a2 */
    char s33s33s[10];                               /* 0x005536a4 */
    unsigned char d_005536ae[34];                   /* 0x005536ae */
    char s_005536d0[10];                            /* 0x005536d0 */
    unsigned char d_005536da[6];                    /* 0x005536da */
    char s33s[6];                                   /* 0x005536e0 */
    unsigned char d_005536e6[6];                    /* 0x005536e6 */
    char s_005536ec[10];                            /* 0x005536ec */
    unsigned char d_005536f6[34];                   /* 0x005536f6 */
    char s_00553718[6];                             /* 0x00553718 */
    unsigned char d_0055371e[50];                   /* 0x0055371e */
    char s_00553750[6];                             /* 0x00553750 */
    unsigned char d_00553756[94];                   /* 0x00553756 */
    float gRiderBikeStats[35];                      /* 0x005537b4 */
    struct {
        uint8_t count;
        uint8_t dirs[3];
        uint8_t anim;
        uint8_t pose;
    } gRiderSignatureTricks[12][4];           /* 0x00553840; the type is local to bmx/com/action/rider.c */
    unsigned char pad_00553960[1316];         /* 0x00553960 */
    char sPlayer[8];                          /* 0x00553e84 */
    char sInvalidLevelForGoalInfo[28];        /* 0x00553e8c */
    unsigned char pad_00553ea8[8];            /* 0x00553ea8 */
    char sCZaxisBmxSrcComActionComdataC[40];  /* 0x00553eb0 */
    unsigned char pad_00553ed8[8];            /* 0x00553ed8 */
    ContrlerBindings gContrlerBindings;       /* 0x00553ee0 */
    unsigned char pad_005544c8[304];          /* 0x005544c8 */
    char sCouldNotCreateControllerDevice[36]; /* 0x005545f8 */
    unsigned char pad_0055461c[8];            /* 0x0055461c */
    char sCZaxisBmxSrcComActionContrlerC[40]; /* 0x00554624 */
    unsigned char pad_0055464c[8];            /* 0x0055464c */
    char sActionStatusOutOfRange[28];         /* 0x00554654 */
    unsigned char pad_00554670[4];            /* 0x00554670 */
    char sActionOutOfRange[20];               /* 0x00554674 */
    unsigned char pad_00554688[4];            /* 0x00554688 */
    char sControllerConfigOutOfRange[32];     /* 0x0055468c */
    unsigned char pad_005546ac[8];            /* 0x005546ac */
    char sStatusOutOfRange[20];               /* 0x005546b4 */
    unsigned char pad_005546c8[4];            /* 0x005546c8 */
    char sJoystickNumberOutOfRange[32];       /* 0x005546cc */
    unsigned char pad_005546ec[4];            /* 0x005546ec */
    char sControllerNumberOutOfRange[32];     /* 0x005546f0 */
    unsigned char pad_00554710[8];            /* 0x00554710 */
    void *gFxHandlerTable;                    /* 0x00554718 */
    void *p_0055471c;                         /* 0x0055471c */
    void *p_00554720;                         /* 0x00554720 */
    uint8_t gFxBurstBlock;                    /* 0x00554724 */
    unsigned char d_00554725[19];             /* 0x00554725 */
    struct {
        uint8_t pad00[12];
        struct {
            float x;
            float y;
            float z;
        } pos;
    } gFxPuffBlock;          /* 0x00554738; the type is local to bmx/com/action/fx.c */
    uint32_t d_00554750[18]; /* 0x00554750 */
    struct {
        uint16_t index;
        uint16_t value;
        int16_t x;
        int16_t y;
    } gFxScatterSlots[42];  /* 0x00554798; the type is local to bmx/com/action/fx.c */
    uint32_t d_005548e8[4]; /* 0x005548e8 */
    uint32_t d_005548f8[4]; /* 0x005548f8 */
    uint32_t d_00554908[2]; /* 0x00554908 */
    uint32_t d_00554910[2]; /* 0x00554910 */
    uint32_t d_00554918[2]; /* 0x00554918 */
    uint32_t d_00554920[2]; /* 0x00554920 */
    uint32_t d_00554928[4]; /* 0x00554928 */
    void *p_00554938;       /* 0x00554938 */
    void *p_0055493c;       /* 0x0055493c */
    void *p_00554940;       /* 0x00554940 */
    void *p_00554944;       /* 0x00554944 */
    void *p_00554948;       /* 0x00554948 */
    void *p_0055494c;       /* 0x0055494c */
    void *p_00554950;       /* 0x00554950 */
    uint32_t d_00554954[4]; /* 0x00554954 */
    struct {
        struct {
            float x;
            float y;
            float z;
        } pos;
        uint32_t other;
    } gFxSkidCorners[4];           /* 0x00554964; the type is local to bmx/com/action/fx.c */
    uint32_t d_005549a4;           /* 0x005549a4 */
    void *p_005549a8;              /* 0x005549a8 */
    uint32_t d_005549ac[20];       /* 0x005549ac */
    uint32_t d_005549fc[20];       /* 0x005549fc */
    uint32_t d_00554a4c[20];       /* 0x00554a4c */
    void *gFxPresets[6];           /* 0x00554a9c; the type is local to bmx/com/action/fx.c */
    unsigned char pad_00554ab4[4]; /* 0x00554ab4 */
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
    } gFxEmitBlock; /* 0x00554ab8; the type is local to bmx/com/action/fx.c */
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
    } gFxCreateBlock;                         /* 0x00554ad8; the type is local to bmx/com/action/fx.c */
    uint32_t d_00554af8[7];                   /* 0x00554af8 */
    float d_00554b14[56];                     /* 0x00554b14 */
    char sSpark[8];                           /* 0x00554bf4 */
    char sDust[8];                            /* 0x00554bfc */
    char sShard[8];                           /* 0x00554c04 */
    char sSkid[8];                            /* 0x00554c0c */
    char sTypeOutOfRange[20];                 /* 0x00554c14 */
    unsigned char pad_00554c28[4];            /* 0x00554c28 */
    char sCZaxisBmxSrcComActionFxC[36];       /* 0x00554c2c */
    unsigned char pad_00554c50[4];            /* 0x00554c50 */
    char sSplash[8];                          /* 0x00554c54 */
    char sRipple[8];                          /* 0x00554c5c */
    uint32_t d_00554c64[7];                   /* 0x00554c64 */
    char gGameJudgePar[8];                    /* 0x00554c80 */
    unsigned char pad_00554c88[4];            /* 0x00554c88 */
    GameRoundCounts gGameRoundCounts;         /* 0x00554c8c */
    uint32_t d_00554c98[6];                   /* 0x00554c98 */
    uint32_t gGameRideSeconds;                /* 0x00554cb0 */
    GameMarkerColors gGameMarkerColors;       /* 0x00554cb4 */
    GameJudgeBias gGameJudgeBias;             /* 0x00554cc0 */
    unsigned char pad_00554cca[14];           /* 0x00554cca */
    char sNoStoredHighScoreEntryFound[36];    /* 0x00554cd8 */
    unsigned char pad_00554cfc[4];            /* 0x00554cfc */
    uint32_t gGameDefaultWord;                /* 0x00554d00 */
    char sCZaxisBmxSrcComActionGameC[36];     /* 0x00554d04 */
    unsigned char pad_00554d28[8];            /* 0x00554d28 */
    char sRoundNumberOutOfRange[28];          /* 0x00554d30 */
    unsigned char pad_00554d4c[4];            /* 0x00554d4c */
    char sHorseWordTooLong[20];               /* 0x00554d50 */
    unsigned char pad_00554d64[4];            /* 0x00554d64 */
    char sImproperLevelForObjectiveLogic[36]; /* 0x00554d68 */
    unsigned char pad_00554d8c[8];            /* 0x00554d8c */
    char sJudgeDAwardsD[24];                  /* 0x00554d94 */
    unsigned char pad_00554dac[4];            /* 0x00554dac */
    char sNonRandomJudgeScoreIsD[32];         /* 0x00554db0 */
    unsigned char pad_00554dd0[4];            /* 0x00554dd0 */
    char sNumberOfFallsD[24];                 /* 0x00554dd4 */
    unsigned char pad_00554dec[4];            /* 0x00554dec */
    char sNumberOfManualsD[24];               /* 0x00554df0 */
    unsigned char pad_00554e08[4];            /* 0x00554e08 */
    char sNumberOfStallsD[24];                /* 0x00554e0c */
    unsigned char pad_00554e24[4];            /* 0x00554e24 */
    char sNumberOfGrindsD[24];                /* 0x00554e28 */
    unsigned char pad_00554e40[4];            /* 0x00554e40 */
    char sNumberOfModifiedTricksD[32];        /* 0x00554e44 */
    unsigned char pad_00554e64[8];            /* 0x00554e64 */
    char sNumberOfModifiersD[28];             /* 0x00554e6c */
    unsigned char pad_00554e88[4];            /* 0x00554e88 */
    char sNumberOfTricksD[24];                /* 0x00554e8c */
    unsigned char pad_00554ea4[4];            /* 0x00554ea4 */
    char sTotalAreasUsedD[24];                /* 0x00554ea8 */
    unsigned char pad_00554ec0[4];            /* 0x00554ec0 */
    char sTotalUniqueModifiersPerformedD[40]; /* 0x00554ec4 */
    unsigned char pad_00554eec[8];            /* 0x00554eec */
    char sTotalUniqueTricksPerformedD[36];    /* 0x00554ef4 */
    unsigned char pad_00554f18[8];            /* 0x00554f18 */
    char sJudgeScoreCalculation[28];          /* 0x00554f20 */
    unsigned char pad_00554f3c[4];            /* 0x00554f3c */
    char sGoalCompletionErrorLevel[32];       /* 0x00554f40 */
    unsigned char pad_00554f60[4];            /* 0x00554f60 */
    char sGoalCompletionErrorChar[32];        /* 0x00554f64 */
    unsigned char pad_00554f84[4];            /* 0x00554f84 */
    char sSaveSAVEDAT[16];                    /* 0x00554f88 */
    unsigned char pad_00554f98[4];            /* 0x00554f98 */
    char sErrorAllocatingTemporaryBufferForMemCar[52]; /* 0x00554f9c */
    unsigned char pad_00554fd0[12];                    /* 0x00554fd0 */
    char sCZaxisBmxSrcComActionMemcardC[40];           /* 0x00554fdc */
    unsigned char pad_00555004[8];                     /* 0x00555004 */
    char sErrorCouldnTOpenFileSForReading[44];         /* 0x0055500c */
    unsigned char pad_00555038[8];                     /* 0x00555038 */
    char sErrorCouldnTReadFromFileS[40];               /* 0x00555040 */
    unsigned char pad_00555068[4];                     /* 0x00555068 */
    char sErrorMemoryCardDataIsInvalidProbablyOld[60]; /* 0x0055506c */
    unsigned char pad_005550a8[12];                    /* 0x005550a8 */
    char sErrorCouldnTOpenFileSForWriting[44];         /* 0x005550b4 */
    unsigned char pad_005550e0[8];                     /* 0x005550e0 */
    char sErrorCouldnTWriteToFileS[36];                /* 0x005550e8 */
    unsigned char pad_0055510c[12];                    /* 0x0055510c */
    GameGoalTable gGameGoals;                          /* 0x00555118 */
    OvlGoalEntries gGoalEntriesA;                      /* 0x005568b8 */
    uint32_t d_00556938[4];                            /* 0x00556938 */
    OvlGoalEntries gGoalEntriesB;     /* 0x00556948, also gGoalColumns, gGoalEntriesC, gGoalEntriesD */
    uint32_t d_005569c8[8];           /* 0x005569c8 */
    char s0123456[20];                /* 0x005569e8 */
    unsigned char pad_005569fc[1276]; /* 0x005569fc */
    int8_t gOvlSavedSubMode;          /* 0x00556ef8 */
    unsigned char pad_00556ef9[3];    /* 0x00556ef9 */
    int32_t gOvlSaveStep;             /* 0x00556efc */
    OvlElementTable gOvlElements;     /* 0x00556f00 */
    void *p_00557098;                 /* 0x00557098 */
    void *p_0055709c;                 /* 0x0055709c */
    uint32_t d_005570a0[2];           /* 0x005570a0 */
    void *p_005570a8;                 /* 0x005570a8 */
    void *p_005570ac;                 /* 0x005570ac */
    uint32_t d_005570b0[2];           /* 0x005570b0 */
    void *p_005570b8;                 /* 0x005570b8 */
    void *p_005570bc;                 /* 0x005570bc */
    void *p_005570c0;                 /* 0x005570c0 */
    uint32_t d_005570c4;              /* 0x005570c4 */
    void *p_005570c8;                 /* 0x005570c8 */
    void *p_005570cc;                 /* 0x005570cc */
    void *p_005570d0;                 /* 0x005570d0 */
    uint32_t d_005570d4;              /* 0x005570d4 */
    void *p_005570d8;                 /* 0x005570d8 */
    void *p_005570dc;                 /* 0x005570dc */
    uint32_t d_005570e0[2];           /* 0x005570e0 */
    unsigned char d_005570e8[4];      /* 0x005570e8 */
    void *p_005570ec;                 /* 0x005570ec */
    void *p_005570f0;                 /* 0x005570f0 */
    uint32_t d_005570f4;              /* 0x005570f4 */
    unsigned char d_005570f8[4];      /* 0x005570f8 */
    void *p_005570fc;                 /* 0x005570fc */
    void *p_00557100;                 /* 0x00557100 */
    uint32_t d_00557104;              /* 0x00557104 */
    void *p_00557108;                 /* 0x00557108 */
    void *p_0055710c;                 /* 0x0055710c */
    void *p_00557110;                 /* 0x00557110 */
    uint32_t d_00557114;              /* 0x00557114 */
    void *p_00557118;                 /* 0x00557118 */
    void *p_0055711c;                 /* 0x0055711c */
    void *p_00557120;                 /* 0x00557120 */
    unsigned char pad_00557124[4];    /* 0x00557124 */
    void *p_00557128;                 /* 0x00557128 */
    void *p_0055712c;                 /* 0x0055712c */
    void *p_00557130;                 /* 0x00557130 */
    uint32_t d_00557134;              /* 0x00557134 */
    void *p_00557138;                 /* 0x00557138 */
    void *p_0055713c;                 /* 0x0055713c */
    void *p_00557140;                 /* 0x00557140 */
    uint32_t d_00557144;              /* 0x00557144 */
    void *p_00557148;                 /* 0x00557148 */
    void *p_0055714c;                 /* 0x0055714c */
    void *p_00557150;                 /* 0x00557150 */
    uint32_t d_00557154;              /* 0x00557154 */
    void *p_00557158;                 /* 0x00557158 */
    void *p_0055715c;                 /* 0x0055715c */
    void *p_00557160;                 /* 0x00557160 */
    uint32_t d_00557164;              /* 0x00557164 */
    void *p_00557168;                 /* 0x00557168 */
    void *p_0055716c;                 /* 0x0055716c */
    void *p_00557170;                 /* 0x00557170 */
    uint32_t d_00557174;              /* 0x00557174 */
    unsigned char d_00557178[4];      /* 0x00557178 */
    void *p_0055717c;                 /* 0x0055717c */
    void *p_00557180;                 /* 0x00557180 */
    uint8_t gOvlMpModeLines;          /* 0x00557184 */
    unsigned char pad_00557185[3];    /* 0x00557185 */
    unsigned char d_00557188[4];      /* 0x00557188 */
    void *p_0055718c;                 /* 0x0055718c */
    void *p_00557190;                 /* 0x00557190 */
    uint32_t d_00557194;              /* 0x00557194 */
    unsigned char d_00557198[4];      /* 0x00557198 */
    void *p_0055719c;                 /* 0x0055719c */
    void *p_005571a0;                 /* 0x005571a0 */
    uint32_t d_005571a4;              /* 0x005571a4 */
    unsigned char d_005571a8[4];      /* 0x005571a8 */
    void *p_005571ac;                 /* 0x005571ac */
    void *p_005571b0;                 /* 0x005571b0 */
    uint32_t d_005571b4;              /* 0x005571b4 */
    OvlScreenTable gOvlScreens;       /* 0x005571b8 */
    OvlPageDefs gOvlSavePages;        /* 0x00557224 */
    OvlMpWord gOvlMpWord;             /* 0x00557254 */
    uint8_t gOvlMpWordCursor;         /* 0x0055725d */
    unsigned char pad_0055725e[2];    /* 0x0055725e */
    OvlSize2 gOvlBarSize;             /* 0x00557260 */
    unsigned char pad_00557268[48];   /* 0x00557268 */
    OvlSize2 gOvlFadeSize;            /* 0x00557298 */
    unsigned char pad_005572a0[27];   /* 0x005572a0 */
    unsigned char d_005572bb[16];     /* 0x005572bb, also gHintGlyphs */
    unsigned char gButtonGlyphs[9];   /* 0x005572cb */
    OvlStrTable5 gIndicatorText;      /* 0x005572d4 */
    OvlColors5 gIndicatorColor;       /* 0x005572e8 */
    uint32_t d_005572fc[3];           /* 0x005572fc */
    OvlStrTable4 gTrickDoubleNames;   /* 0x00557308 */
    unsigned char pad_00557318[212];  /* 0x00557318 */
    char sSick[8];                    /* 0x005573ec */
    char sQuad[8];                    /* 0x005573f4 */
    char sTriple[8];                  /* 0x005573fc */
    char sDouble[8];                  /* 0x00557404 */
    char sP[4];                       /* 0x0055740c */
    char s_00557410[4];               /* 0x00557410 */
    char sR[4];                       /* 0x00557414 */
    char sSS[8];                      /* 0x00557418 */
    unsigned char pad_00557420[4];    /* 0x00557420 */
    char sC[4];                       /* 0x00557424 */
    char sTo[4];                      /* 0x00557428 */
    char s_0055742c[4];               /* 0x0055742c */
    char sS[4];                       /* 0x00557430 */
    char sCZaxisBmxSrcComActionOverlympC[40];          /* 0x00557434 */
    unsigned char pad_0055745c[8];                     /* 0x0055745c */
    char sLd[4];                                       /* 0x00557464 */
    char s_00557468[4];                                /* 0x00557468 */
    char s_0055746c[8];                                /* 0x0055746c */
    char sSLd[8];                                      /* 0x00557474 */
    char s_0055747c[8];                                /* 0x0055747c */
    char s_00557484[4];                                /* 0x00557484 */
    char sLdLd[8];                                     /* 0x00557488 */
    unsigned char pad_00557490[4];                     /* 0x00557490 */
    char s_00557494[4];                                /* 0x00557494 */
    char sSSS[12];                                     /* 0x00557498 */
    char s_005574a4[8];                                /* 0x005574a4 */
    char sErrorParsingFontIcons[28];                   /* 0x005574ac */
    unsigned char pad_005574c8[4];                     /* 0x005574c8 */
    char sMissingFontDataIcons[28];                    /* 0x005574cc */
    unsigned char pad_005574e8[4];                     /* 0x005574e8 */
    char sErrorParsingFontButton[28];                  /* 0x005574ec */
    unsigned char pad_00557508[8];                     /* 0x00557508 */
    char sMissingFontDataButton[28];                   /* 0x00557510 */
    unsigned char pad_0055752c[4];                     /* 0x0055752c */
    char sErrorParsingFontTiny[28];                    /* 0x00557530 */
    unsigned char pad_0055754c[4];                     /* 0x0055754c */
    char sMissingFontDataTiny[28];                     /* 0x00557550 */
    unsigned char pad_0055756c[4];                     /* 0x0055756c */
    char sErrorParsingFontSmall[28];                   /* 0x00557570 */
    unsigned char pad_0055758c[4];                     /* 0x0055758c */
    char sNoMemoryForFont[20];                         /* 0x00557590 */
    unsigned char pad_005575a4[4];                     /* 0x005575a4 */
    char sMissingFontDataSmall[28];                    /* 0x005575a8 */
    unsigned char pad_005575c4[4];                     /* 0x005575c4 */
    char sCZaxisBmxSrcComActionOverlayC[40];           /* 0x005575c8 */
    unsigned char pad_005575f0[8];                     /* 0x005575f0 */
    char sStringTableNotFoundInvalidLanguage[44];      /* 0x005575f8 */
    unsigned char pad_00557624[8];                     /* 0x00557624 */
    char sNoMemoryForStringTable[28];                  /* 0x0055762c */
    unsigned char pad_00557648[4];                     /* 0x00557648 */
    char sSAVINGGAMEDATA[20];                          /* 0x0055764c */
    unsigned char pad_00557660[4];                     /* 0x00557660 */
    char s_00557664[4];                                /* 0x00557664 */
    char s_00557668[8];                                /* 0x00557668 */
    char s_00557670[4];                                /* 0x00557670 */
    char s02dD[8];                                     /* 0x00557674 */
    unsigned char pad_0055767c[4];                     /* 0x0055767c */
    char sSBmxAssetsSReplayDemo02d02dRpy[44];          /* 0x00557680 */
    unsigned char pad_005576ac[8];                     /* 0x005576ac */
    char sPC[4];                                       /* 0x005576b4 */
    char sUl[4];                                       /* 0x005576b8 */
    char s1ldC02ld[12];                                /* 0x005576bc */
    unsigned char pad_005576c8[4];                     /* 0x005576c8 */
    char sTO[4];                                       /* 0x005576cc */
    char sInvalidRecordTypeInDisplaySetupData[44];     /* 0x005576d0 */
    unsigned char pad_005576fc[8];                     /* 0x005576fc */
    char sLdft[8];                                     /* 0x00557704 */
    char sTooManyButtonsInHintStringIncreaseAlloc[56]; /* 0x0055770c */
    unsigned char pad_00557744[8];                     /* 0x00557744 */
    char s_0055774c[4];                                /* 0x0055774c */
    char sLdS[8];                                      /* 0x00557750 */
    char s0ldLd[12];                                   /* 0x00557758 */
    unsigned char pad_00557764[4];                     /* 0x00557764 */
    char sFlail[8];                                    /* 0x00557768 */
    char sDriver[8];                                   /* 0x00557770 */
    unsigned char pad_00557778[4];                     /* 0x00557778 */
    char sVette[8];                                    /* 0x0055777c */
    char sTripleFlip[12];                              /* 0x00557784 */
    unsigned char pad_00557790[4];                     /* 0x00557790 */
    char sLimo[8];                                     /* 0x00557794 */
    char sFlip[8];                                     /* 0x0055779c */
    char sFlair[8];                                    /* 0x005577a4 */
    char sTruckDriver[16];                             /* 0x005577ac */
    char s_005577bc[4];                                /* 0x005577bc */
    char sSpin[8];                                     /* 0x005577c0 */
    char sAlleyOop[12];                                /* 0x005577c8 */
    char sOpposite[12];                                /* 0x005577d4 */
    char sHalfCab[12];                                 /* 0x005577e0 */
    float gPhysRollFriction[5];              /* 0x005577ec, also gPhysGrindDrag; the type is local to
                                                bmx/com/action/physics_roll.h */
    float gPhysAirDrag;                      /* 0x00557800 */
    float gPhysRollYawDamp;                  /* 0x00557804 */
    float gRiderAirYawDamp;                  /* 0x00557808 */
    float gPhysGrindYawDamp;                 /* 0x0055780c */
    float gPhysGravity;                      /* 0x00557810 */
    float gPhysRestSpeed;                    /* 0x00557814 */
    float gPhysLeanSpeedMax;                 /* 0x00557818 */
    float gPhysBailImpact;                   /* 0x0055781c, also gPhysAirCrashSpeed */
    float gPhysLandCrashSpeed;               /* 0x00557820 */
    float gPhysWheelSideSlip;                /* 0x00557824 */
    float gPhysRollSpinSpeed;                /* 0x00557828, also gPhysLeanSpeedMin */
    float gPhysAirSpeedCap;                  /* 0x0055782c */
    float d_00557830;                        /* 0x00557830 */
    float gPhysPipeLaunchScale;              /* 0x00557834 */
    float gPhysGrindMinSpeed;                /* 0x00557838 */
    ZVec3 gPhysAirProbe;                     /* 0x0055783c, also gPhysProbeOffset */
    int16_t gPhysWallAngle;                  /* 0x00557848 */
    int16_t gPhysSlopeAngle;                 /* 0x0055784a */
    ZAngle gPhysMaxSlope;                    /* 0x0055784c */
    int16_t gPhysRailAngleLimit;             /* 0x0055784e */
    unsigned char d_00557850[6];             /* 0x00557850 */
    int16_t gPhysPipeAngleLimit;             /* 0x00557856 */
    int16_t gPhysPipeTiltLimit;              /* 0x00557858 */
    unsigned char pad_0055785a[2];           /* 0x0055785a */
    uint32_t gPhysGrindInSteps;              /* 0x0055785c */
    uint32_t gPhysStallInSteps;              /* 0x00557860 */
    int32_t gPhysAirTurnSteps;               /* 0x00557864, also gPhysAirAlignFrames */
    float gPhysGrindInRadius;                /* 0x00557868 */
    float d_0055786c;                        /* 0x0055786c */
    float gPhysEdgeSearchRadius;             /* 0x00557870 */
    float gPhysGrindBalance;                 /* 0x00557874 */
    float gPhysSuspGravityScale;             /* 0x00557878 */
    float gPhysRollSteer;                    /* 0x0055787c */
    float gPhysUpBlend;                      /* 0x00557880, also gPhysAlignRate, gPhysLandUpBlend */
    float gPhysLeanMax;                      /* 0x00557884 */
    ZVec3 gPhysDown;                         /* 0x00557888, also gPhysAirUp */
    unsigned char pad_00557894[4];           /* 0x00557894 */
    PhysTransitionTable gPhysStateExit;      /* 0x00557898 */
    PhysTransitionTable gPhysStateEnter;     /* 0x005578b4 */
    PhysStepTable gPhysStateOrient;          /* 0x005578d0 */
    PhysStepTable gPhysStateVelocity;        /* 0x005578ec */
    PhysStepTable gPhysStateMove;            /* 0x00557908 */
    PhysStepTable gPhysStateSync;            /* 0x00557924 */
    unsigned char pad_00557940[68];          /* 0x00557940 */
    char sCouldnTFindRespawnFace[28];        /* 0x00557984 */
    unsigned char pad_005579a0[4];           /* 0x005579a0 */
    char sCZaxisBmxSrcComActionPhysicsC[40]; /* 0x005579a4 */
    unsigned char pad_005579cc[8];           /* 0x005579cc */
    char sWarningNoTaggedRespawnFacesFoundInLevel[52]; /* 0x005579d4 */
    unsigned char pad_00557a08[8];                     /* 0x00557a08 */
    char sWarningNoRespawnFacesFoundByDistanceChe[52]; /* 0x00557a10 */
    unsigned char pad_00557a44[12];                    /* 0x00557a44 */
    char sBmxphysStateInitAirNoValidTerrainFoundU[84]; /* 0x00557a50 */
    unsigned char pad_00557aa4[16];                    /* 0x00557aa4 */
    char sBmxphysStateInitAirQuarterpipeFaceWithN[68]; /* 0x00557ab4 */
    unsigned char pad_00557af8[16];                    /* 0x00557af8 */
    char sAllComponentsOfYOrientationAreZero[44];      /* 0x00557b08 */
    unsigned char pad_00557b34[8];                     /* 0x00557b34 */
    char sAllComponentsOfZOrientationAreZero[44];      /* 0x00557b3c */
    unsigned char pad_00557b68[8];                     /* 0x00557b68 */
    char sAllComponentsOfXOrientationAreZero[44];      /* 0x00557b70 */
    unsigned char pad_00557b9c[8];                     /* 0x00557b9c */
    char sBmxphysUpdatePositionRollNoValidTerrain[84]; /* 0x00557ba4 */
    unsigned char pad_00557bf8[16];                    /* 0x00557bf8 */
    char sBmxphysUpdatePositionRollQuarterpipeFac[76]; /* 0x00557c08 */
    unsigned char pad_00557c54[12];                    /* 0x00557c54 */
    char sBmxphysUpdatePositionAirNoValidTerrainF[88]; /* 0x00557c60 */
    unsigned char pad_00557cb8[20];                    /* 0x00557cb8 */
    char sBmxphysUpdatePositionAirQuarterpipeFace[76]; /* 0x00557ccc */
    unsigned char pad_00557d18[16];                    /* 0x00557d18 */
    PropDefs gPropDefs;                                /* 0x00557d28 */
    unsigned char pad_005584a8[384];                   /* 0x005584a8 */
    char sBmxpropInitErrorFindingPropWithTagD[48];     /* 0x00558628 */
    unsigned char pad_00558658[8];                     /* 0x00558658 */
    char sStateInfoOverflow[20];                       /* 0x00558660 */
    unsigned char pad_00558674[4];                     /* 0x00558674 */
    char sCZaxisBmxSrcComActionPropC[36];              /* 0x00558678 */
    unsigned char pad_0055869c[8];                     /* 0x0055869c */
    char sBmxpropInitErrorExtraPropWithTagDXDZD[60];   /* 0x005586a4 */
    unsigned char pad_005586e0[12];                    /* 0x005586e0 */
    char sBmxpropInitErrorFindingFaceWithTagD[48];     /* 0x005586ec */
    unsigned char pad_0055871c[8];                     /* 0x0055871c */
    char sBmxpropGetPropByIDCouldnTFindPropD[44];      /* 0x00558724 */
    unsigned char pad_00558750[8];                     /* 0x00558750 */
    char sBmxreplayUpdateReachedEndOfReplayBuffer[72]; /* 0x00558758 */
    unsigned char pad_005587a0[16];                    /* 0x005587a0 */
    char sBmxreplayCreateFailedToAllocateBuffer[44];   /* 0x005587b0 */
    unsigned char pad_005587dc[8];                     /* 0x005587dc */
    char sCZaxisBmxSrcComActionReplayC[40];            /* 0x005587e4 */
    unsigned char pad_0055880c[4];                     /* 0x0055880c */
    char sInvalidHandlePassedOrReplayHasBeenDestr[52]; /* 0x00558810 */
    unsigned char pad_00558844[12];                    /* 0x00558844 */
    char sInvalidReplayHandle[24];                     /* 0x00558850 */
    unsigned char pad_00558868[4];                     /* 0x00558868 */
    float gRiderHopSpeed;                              /* 0x0055886c */
    float gRiderHopBoost;                              /* 0x00558870 */
    float gRiderHopMargin;                             /* 0x00558874 */
    float gRiderJumpPower;                             /* 0x00558878 */
    float gRiderJumpBase;                              /* 0x0055887c */
    float d_00558880[2];                               /* 0x00558880 */
    float gRiderLandGrip;                              /* 0x00558888 */
    float gRiderSteerBase;                             /* 0x0055888c */
    float gPhysLeanRange;                              /* 0x00558890, also gRiderSteerSpeedGain */
    float gRiderSteerFalloff;                          /* 0x00558894 */
    float gRiderAirTurnNudge;                          /* 0x00558898 */
    float gRiderAirTurnStart;                          /* 0x0055889c */
    float gRiderAirTurnMax;                            /* 0x005588a0 */
    float gRiderBikeSpinStep;                          /* 0x005588a4 */
    float gRiderBikeSpinMax;                           /* 0x005588a8 */
    float d_005588ac;                                  /* 0x005588ac */
    float gRiderTrickSteer;                            /* 0x005588b0 */
    uint32_t gRiderJumpFrames;                         /* 0x005588b4 */
    RiderModeTable gRiderModeStep;                     /* 0x005588b8 */
    void *gRiderRollStep;                              /* 0x005588d4 */
    void *p_005588d8;                                  /* 0x005588d8 */
    void *p_005588dc;                                  /* 0x005588dc */
    void *p_005588e0;                                  /* 0x005588e0 */
    void *p_005588e4;                                  /* 0x005588e4 */
    void *p_005588e8;                                  /* 0x005588e8 */
    RiderAirTable gRiderAirStep;                       /* 0x005588ec */
    RiderSingleTable gRiderGrindStep;                  /* 0x005588f8 */
    RiderSingleTable gRiderLipStep;                    /* 0x005588fc */
    RiderSingleTable gRiderManualStep;                 /* 0x00558900 */
    uint32_t gRiderStickDir[4];                        /* 0x00558904 */
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
    char sFailedToCreateRiderActorForRiderLdLookN[60]; /* 0x00558a48 */
    unsigned char pad_00558a84[12];                    /* 0x00558a84 */
    char sStateLevelOutOfRange[28];                    /* 0x00558a90 */
    unsigned char pad_00558aac[4];                     /* 0x00558aac */
    char sCouldnTAssociateRiderWithDynamicCollisi[60]; /* 0x00558ab0 */
    unsigned char pad_00558aec[11];                    /* 0x00558aec */
    unsigned char gRiderAirTrickPick[73];              /* 0x00558af7 */
    uint8_t gRiderTrickCombo[256]; /* 0x00558b40, also gRiderAirTrickMod; the type is local to
                                      bmx/com/action/rider.c */
    uint32_t d_00558c40[11];       /* 0x00558c40 */
    char s_00558c6c[5];            /* 0x00558c6c */
    unsigned char d_00558c71[71];  /* 0x00558c71 */
    char s_00558cb8[5];            /* 0x00558cb8 */
    unsigned char d_00558cbd[31];  /* 0x00558cbd */
    char s_00558cdc[7];            /* 0x00558cdc */
    unsigned char d_00558ce3[33];  /* 0x00558ce3 */
    char s_00558d04[5];            /* 0x00558d04 */
    unsigned char d_00558d09[31];  /* 0x00558d09 */
    char s_00558d28[7];            /* 0x00558d28 */
    unsigned char d_00558d2f[33];  /* 0x00558d2f */
    char s_00558d50[5];            /* 0x00558d50 */
    unsigned char d_00558d55[31];  /* 0x00558d55 */
    char s_00558d74[7];            /* 0x00558d74 */
    unsigned char d_00558d7b[69];  /* 0x00558d7b */
    char s_00558dc0[7];            /* 0x00558dc0 */
    unsigned char d_00558dc7[33];  /* 0x00558dc7 */
    char s_00558de8[5];            /* 0x00558de8 */
    unsigned char d_00558ded[71];  /* 0x00558ded */
    char s_00558e34[5];            /* 0x00558e34 */
    unsigned char d_00558e39[183]; /* 0x00558e39 */
    char s_00558ef0[7];            /* 0x00558ef0 */
    unsigned char d_00558ef7[109]; /* 0x00558ef7 */
    char s_00558f64[5];            /* 0x00558f64 */
    unsigned char d_00558f69[39];  /* 0x00558f69 */
    uint32_t gRiderTrickA[3];      /* 0x00558f90 */
    unsigned char gRiderTrickB[6]; /* 0x00558f9c */
    char gTrickClassHi[6];         /* 0x00558fa2 */
    uint32_t gRiderTrickKind[3];   /* 0x00558fa8 */
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
    OvlStrTable gTrickTurnNames;                       /* 0x00559390, also gTrickBarspinNames */
    void *p_00559490;                                  /* 0x00559490 */
    void *p_00559494;                                  /* 0x00559494 */
    void *p_00559498;                                  /* 0x00559498 */
    void *p_0055949c;                                  /* 0x0055949c */
    void *p_005594a0;                                  /* 0x005594a0 */
    void *p_005594a4;                                  /* 0x005594a4 */
    void *p_005594a8;                                  /* 0x005594a8 */
    void *p_005594ac;                                  /* 0x005594ac */
    void *p_005594b0;                                  /* 0x005594b0 */
    void *p_005594b4;                                  /* 0x005594b4 */
    void *p_005594b8;                                  /* 0x005594b8 */
    void *p_005594bc;                                  /* 0x005594bc */
    void *p_005594c0;                                  /* 0x005594c0 */
    void *p_005594c4;                                  /* 0x005594c4 */
    void *p_005594c8;                                  /* 0x005594c8 */
    void *p_005594cc;                                  /* 0x005594cc */
    void *p_005594d0;                                  /* 0x005594d0 */
    void *p_005594d4;                                  /* 0x005594d4 */
    void *p_005594d8;                                  /* 0x005594d8 */
    void *p_005594dc;                                  /* 0x005594dc */
    OvlStrTable gTrickSpinNames;                       /* 0x005594e0 */
    unsigned char pad_005595e0[296];                   /* 0x005595e0 */
    char sExtreme[8];                                  /* 0x00559708 */
    unsigned char pad_00559710[4];                     /* 0x00559710 */
    char s1260[8];                                     /* 0x00559714 */
    char s1080[8];                                     /* 0x0055971c */
    char s900[4];                                      /* 0x00559724 */
    char s720[4];                                      /* 0x00559728 */
    char s540[4];                                      /* 0x0055972c */
    char s360[4];                                      /* 0x00559730 */
    char s180[4];                                      /* 0x00559734 */
    char sBusdriver[12];                               /* 0x00559738 */
    char sBarspinSeatGrab[20];                         /* 0x00559744 */
    unsigned char pad_00559758[4];                     /* 0x00559758 */
    char sXUpSeatGrab[16];                             /* 0x0055975c */
    unsigned char pad_0055976c[4];                     /* 0x0055976c */
    char sSwitchFooter[16];                            /* 0x00559770 */
    char sIndian[8];                                   /* 0x00559780 */
    char sRocketOneFooter[20];                         /* 0x00559788 */
    unsigned char pad_0055979c[4];                     /* 0x0055979c */
    char sRocketBarspin[16];                           /* 0x005597a0 */
    unsigned char pad_005597b0[4];                     /* 0x005597b0 */
    char sRocketNoHander[20];                          /* 0x005597b4 */
    char sRocketOneHander[20];                         /* 0x005597c8 */
    unsigned char pad_005597dc[4];                     /* 0x005597dc */
    char sCrotchRocket[16];                            /* 0x005597e0 */
    char sRocketXUp[12];                               /* 0x005597f0 */
    unsigned char pad_005597fc[4];                     /* 0x005597fc */
    char sOneFootedBarspin[20];                        /* 0x00559800 */
    unsigned char pad_00559814[4];                     /* 0x00559814 */
    char sNoFootedBarspin[20];                         /* 0x00559818 */
    unsigned char pad_0055982c[4];                     /* 0x0055982c */
    char sBarspinToBarspinBack[24];                    /* 0x00559830 */
    unsigned char pad_00559848[4];                     /* 0x00559848 */
    char sNoHandedBarspin[20];                         /* 0x0055984c */
    unsigned char pad_00559860[4];                     /* 0x00559860 */
    char sNoHandedOneFooter[24];                       /* 0x00559864 */
    unsigned char pad_0055987c[4];                     /* 0x0055987c */
    char sNoFootedOneHander[24];                       /* 0x00559880 */
    unsigned char pad_00559898[4];                     /* 0x00559898 */
    char sOneFootedXUp[16];                            /* 0x0055989c */
    unsigned char pad_005598ac[4];                     /* 0x005598ac */
    char sNoFootedXUp[16];                             /* 0x005598b0 */
    unsigned char pad_005598c0[4];                     /* 0x005598c0 */
    char sSwitchHander[16];                            /* 0x005598c4 */
    char sDoubleSeatGrab[20];                          /* 0x005598d4 */
    char sOneFootedSeatGrab[24];                       /* 0x005598e8 */
    unsigned char pad_00559900[4];                     /* 0x00559900 */
    char sNoFootedSeatGrab[20];                        /* 0x00559904 */
    unsigned char pad_00559918[4];                     /* 0x00559918 */
    char sOneHandedSeatGrab[24];                       /* 0x0055991c */
    unsigned char pad_00559934[4];                     /* 0x00559934 */
    char sDoublePegGrab[16];                           /* 0x00559938 */
    unsigned char pad_00559948[4];                     /* 0x00559948 */
    char sRocketAir[12];                               /* 0x0055994c */
    unsigned char pad_00559958[4];                     /* 0x00559958 */
    char sOneHandXUp[16];                              /* 0x0055995c */
    char sOneHandOneFooter[20];                        /* 0x0055996c */
    unsigned char pad_00559980[4];                     /* 0x00559980 */
    char sNothing[8];                                  /* 0x00559984 */
    unsigned char pad_0055998c[4];                     /* 0x0055998c */
    char sNoHander[12];                                /* 0x00559990 */
    char sNoFooter[12];                                /* 0x0055999c */
    char sSeatGrab[12];                                /* 0x005599a8 */
    char sSuicideNoHander[20];                         /* 0x005599b4 */
    unsigned char pad_005599c8[4];                     /* 0x005599c8 */
    char sXDown[8];                                    /* 0x005599cc */
    char sXUp[8];                                      /* 0x005599d4 */
    char sOneHander[12];                               /* 0x005599dc */
    unsigned char pad_005599e8[4];                     /* 0x005599e8 */
    char sOneFooter[12];                               /* 0x005599ec */
    unsigned char pad_005599f8[4];                     /* 0x005599f8 */
    char sBarspin[8];                                  /* 0x005599fc */
    unsigned char pad_00559a04[4];                     /* 0x00559a04 */
    char sNoseWheelie[16];                             /* 0x00559a08 */
    char sManual[8];                                   /* 0x00559a18 */
    char sWallTap[12];                                 /* 0x00559a20 */
    char sFastPlant[12];                               /* 0x00559a2c */
    unsigned char pad_00559a38[4];                     /* 0x00559a38 */
    char sDisaster[12];                                /* 0x00559a3c */
    char sTailTap[12];                                 /* 0x00559a48 */
    char sNosepick[12];                                /* 0x00559a54 */
    char sIcepickStall[16];                            /* 0x00559a60 */
    char sToothpickStall[16];                          /* 0x00559a70 */
    unsigned char pad_00559a80[4];                     /* 0x00559a80 */
    char sDoublePegStall[20];                          /* 0x00559a84 */
    char sSprocketGrind[16];                           /* 0x00559a98 */
    unsigned char pad_00559aa8[4];                     /* 0x00559aa8 */
    char sCrookedGrind[16];                            /* 0x00559aac */
    char sLucEGrind[12];                               /* 0x00559abc */
    unsigned char pad_00559ac8[4];                     /* 0x00559ac8 */
    char sLipSlide[12];                                /* 0x00559acc */
    char sIcepickGrind[16];                            /* 0x00559ad8 */
    char sToothpickGrind[16];                          /* 0x00559ae8 */
    unsigned char pad_00559af8[4];                     /* 0x00559af8 */
    char sFeebleGrind[16];                             /* 0x00559afc */
    char sSmithGrind[12];                              /* 0x00559b0c */
    unsigned char pad_00559b18[4];                     /* 0x00559b18 */
    char sDoublePegGrind[20];                          /* 0x00559b1c */
    char sSupermanIndian[16];                          /* 0x00559b30 */
    unsigned char pad_00559b40[4];                     /* 0x00559b40 */
    char sCandyBarLookdown[20];                        /* 0x00559b44 */
    unsigned char pad_00559b58[4];                     /* 0x00559b58 */
    char sCanCanLookdown[20];                          /* 0x00559b5c */
    char sDoubleTailwhip[16];                          /* 0x00559b70 */
    unsigned char pad_00559b80[4];                     /* 0x00559b80 */
    char sUnTurndown[12];                              /* 0x00559b84 */
    unsigned char pad_00559b90[4];                     /* 0x00559b90 */
    char sNoFootedCanCan[20];                          /* 0x00559b94 */
    unsigned char pad_00559ba8[4];                     /* 0x00559ba8 */
    char sDecade[8];                                   /* 0x00559bac */
    char sDecadeBackflip[16];                          /* 0x00559bb4 */
    unsigned char pad_00559bc4[4];                     /* 0x00559bc4 */
    char sTailwhipBackflip[20];                        /* 0x00559bc8 */
    unsigned char pad_00559bdc[4];                     /* 0x00559bdc */
    char sCandyBarBackflip[20];                        /* 0x00559be0 */
    unsigned char pad_00559bf4[4];                     /* 0x00559bf4 */
    char sSuperflip[12];                               /* 0x00559bf8 */
    char sTobagganBackflip[20];                        /* 0x00559c04 */
    unsigned char pad_00559c18[4];                     /* 0x00559c18 */
    char sTabletopBackflip[20];                        /* 0x00559c1c */
    unsigned char pad_00559c30[4];                     /* 0x00559c30 */
    char sCanCanBackflip[20];                          /* 0x00559c34 */
    char sTurndownBackflip[20];                        /* 0x00559c48 */
    unsigned char pad_00559c5c[4];                     /* 0x00559c5c */
    char sDoubleFrontFlip[20];                         /* 0x00559c60 */
    unsigned char pad_00559c74[4];                     /* 0x00559c74 */
    char sFrontFlip[12];                               /* 0x00559c78 */
    unsigned char pad_00559c84[4];                     /* 0x00559c84 */
    char sTripleBackflip[16];                          /* 0x00559c88 */
    unsigned char pad_00559c98[4];                     /* 0x00559c98 */
    char sDoubleBackflip[16];                          /* 0x00559c9c */
    unsigned char pad_00559cac[4];                     /* 0x00559cac */
    char sTobaggan[12];                                /* 0x00559cb0 */
    char sTailwhip[12];                                /* 0x00559cbc */
    char sCandyBar[12];                                /* 0x00559cc8 */
    char sSuperman[12];                                /* 0x00559cd4 */
    char sBackflip[12];                                /* 0x00559ce0 */
    char sTabletop[12];                                /* 0x00559cec */
    char sCanCan[8];                                   /* 0x00559cf8 */
    unsigned char pad_00559d00[4];                     /* 0x00559d00 */
    char sTurndown[12];                                /* 0x00559d04 */
    void *p_00559d10;                                  /* 0x00559d10 */
    char sCZaxisBmxSrcComMenuGuiGuiinputC[44];         /* 0x00559d14 */
    unsigned char pad_00559d40[8];                     /* 0x00559d40 */
    const char *gGuiAssetFile;                         /* 0x00559d48 */
    int gGuiPulseStep;                                 /* 0x00559d4c */
    unsigned char pad_00559d50[4];                     /* 0x00559d50 */
    char sIntartZal[12];                               /* 0x00559d54 */
    unsigned char pad_00559d60[4];                     /* 0x00559d60 */
    char sGUIMAINErrorReinitializingApp[40];           /* 0x00559d64 */
    unsigned char pad_00559d8c[4];                     /* 0x00559d8c */
    char sGUIMAINFailedToInitHelpSystem[40];           /* 0x00559d90 */
    unsigned char pad_00559db8[4];                     /* 0x00559db8 */
    char sGUIMAINFailedToLoadPermanentScreenAsset[52]; /* 0x00559dbc */
    unsigned char pad_00559df0[8];                     /* 0x00559df0 */
    char sGUIMAINFailedToInitializeZPRIM[40];          /* 0x00559df8 */
    unsigned char pad_00559e20[4];                     /* 0x00559e20 */
    char sGUIMAINFailedToInitializeZMEMDMM[40];        /* 0x00559e24 */
    unsigned char pad_00559e4c[8];                     /* 0x00559e4c */
    char sGUIMAINErrorReinitializingMenuMode[44];      /* 0x00559e54 */
    unsigned char pad_00559e80[8];                     /* 0x00559e80 */
    char sGUIMAINErrorInitializingMenuModeBeforeA[52]; /* 0x00559e88 */
    unsigned char pad_00559ebc[12];                    /* 0x00559ebc */
    char sGUIMAINFailedToDestroyAllElements[44];       /* 0x00559ec8 */
    unsigned char pad_00559ef4[8];                     /* 0x00559ef4 */
    char sGUIMAINErrorFailedToLoadScreen[40];          /* 0x00559efc */
    unsigned char pad_00559f24[8];                     /* 0x00559f24 */
    char sGUISFILEFailedToLoadScreenAssets[40];        /* 0x00559f2c */
    unsigned char pad_00559f54[8];                     /* 0x00559f54 */
    char sGUIMAINErrorCallingUpdateBeforeInitiali[56]; /* 0x00559f5c */
    unsigned char pad_00559f94[12];                    /* 0x00559f94 */
    char sGUIMAINErrorFailedToAllocateElement[48];     /* 0x00559fa0 */
    unsigned char pad_00559fd0[8];                     /* 0x00559fd0 */
    char sGUIMAINErrorFreeingElementNotInList[48];     /* 0x00559fd8 */
    unsigned char pad_0055a008[8];                     /* 0x0055a008 */
    char sGUIMAINErrorFailedToLoadSprite044X[48];      /* 0x0055a010 */
    unsigned char pad_0055a040[8];                     /* 0x0055a040 */
    char sGUIMAINErrorFailedToLoadSelectedSprite0[56]; /* 0x0055a048 */
    unsigned char pad_0055a080[12];                    /* 0x0055a080 */
    char sGUIMAINErrorFailedToLoadFont044X[48];        /* 0x0055a08c */
    unsigned char pad_0055a0bc[8];                     /* 0x0055a0bc */
    char sGUIMAINErrorFailedToLoadBackdrop044X[52];    /* 0x0055a0c4 */
    unsigned char pad_0055a0f8[8];                     /* 0x0055a0f8 */
    char sPOSTMESSAGENoTypeHandler[32];                /* 0x0055a100 */
    unsigned char pad_0055a120[8];                     /* 0x0055a120 */
    char sGUIMAINErrorSendingMessageDToNULLElemen[56]; /* 0x0055a128 */
    unsigned char pad_0055a160[8];                     /* 0x0055a160 */
    char sGUISFILEErrorFailedToAllocateElement[48];    /* 0x0055a168 */
    unsigned char pad_0055a198[8];                     /* 0x0055a198 */
    char sGUISFILEErrorBadScreenElementDef044X[52];    /* 0x0055a1a0 */
    unsigned char pad_0055a1d4[8];                     /* 0x0055a1d4 */
    uint32_t gGuiElementMagic;                         /* 0x0055a1dc */
    unsigned char pad_0055a1e0[4];                     /* 0x0055a1e0 */
    char sGUISFILEErrorBadScreen044X[40];              /* 0x0055a1e4 */
    unsigned char pad_0055a20c[4];                     /* 0x0055a20c */
    uint32_t gGuiScreenMagic;                          /* 0x0055a210 */
    unsigned char pad_0055a214[4];                     /* 0x0055a214 */
    char sGUISFILEErrorFailedToLoadScreenX[44];        /* 0x0055a218 */
    unsigned char pad_0055a244[8];                     /* 0x0055a244 */
    char sGUITOGGLFailedToLockToggleText[40];          /* 0x0055a24c */
    unsigned char pad_0055a274[8];                     /* 0x0055a274 */
    char sGUITOGGLFailedToLockToggleData[40];          /* 0x0055a27c */
    unsigned char pad_0055a2a4[8];                     /* 0x0055a2a4 */
    char sCZaxisBmxSrcComMenuGuiGuitogglC[44];         /* 0x0055a2ac */
    unsigned char pad_0055a2d8[8];                     /* 0x0055a2d8 */
    char sGUITOGGLErrorFailedToLoadFont044X[48];       /* 0x0055a2e0 */
    unsigned char pad_0055a310[8];                     /* 0x0055a310 */
    char sGUITOGGLErrorFailedToMallocTextString[48];   /* 0x0055a318 */
    unsigned char pad_0055a348[12];                    /* 0x0055a348 */
    int gOptMainPulseStep;                             /* 0x0055a354, also gExtrasPulseStep */
    LanguageTable gLanguageTable;                      /* 0x0055a358 */
    unsigned char pad_0055a35b[1];                     /* 0x0055a35b */
    MenuEntryStrings gMenuEntryStrings;                /* 0x0055a35c */
    MenuSlotPosTable gMenuSlotPos;                     /* 0x0055a368 */
    MenuSlotShortTable gMenuSlotAlpha;                 /* 0x0055a384 */
    unsigned char pad_0055a392[2];                     /* 0x0055a392 */
    MenuSlotShortTable gMenuSlotTint;                  /* 0x0055a394 */
    unsigned char pad_0055a3a2[14];                    /* 0x0055a3a2 */
    OptionStateTable gOptStateStrings;                 /* 0x0055a3b0 */
    unsigned char pad_0055a3b8[4];                     /* 0x0055a3b8 */
    OptionTable gOptionTable;                          /* 0x0055a3bc */
    StripColorTable gStripColors;                      /* 0x0055a3dc */
    KeyNameTable gKeyNames;                            /* 0x0055a3f4 */
    KeyTitleTable gKbLayoutTitles;                     /* 0x0055a43c */
    KeyLabelTable gKbLayoutLabels;                     /* 0x0055a450 */
    unsigned char pad_0055a5b8[104];                   /* 0x0055a5b8 */
    char sS4h[4];                                      /* 0x0055a620 */
    char sS4g[4];                                      /* 0x0055a624 */
    char sS4f[4];                                      /* 0x0055a628 */
    char sS4e[4];                                      /* 0x0055a62c */
    char sS4d[4];                                      /* 0x0055a630 */
    char sS4c[4];                                      /* 0x0055a634 */
    char sS4b[4];                                      /* 0x0055a638 */
    char sPGDOWN[8];                                   /* 0x0055a63c */
    char sDEL[4];                                      /* 0x0055a644 */
    char sHOME[8];                                     /* 0x0055a648 */
    char sPGUP[8];                                     /* 0x0055a650 */
    char sINS[4];                                      /* 0x0055a658 */
    char sS3h[4];                                      /* 0x0055a65c */
    char sS3g[4];                                      /* 0x0055a660 */
    char sS3f[4];                                      /* 0x0055a664 */
    char sS3e[4];                                      /* 0x0055a668 */
    char sS3d[4];                                      /* 0x0055a66c */
    char sS3c[4];                                      /* 0x0055a670 */
    char sS3b[4];                                      /* 0x0055a674 */
    char sEND[4];                                      /* 0x0055a678 */
    char sNUMENTER[12];                                /* 0x0055a67c */
    char sCTRL[8];                                     /* 0x0055a688 */
    char sS2h[4];                                      /* 0x0055a690 */
    char sS2g[4];                                      /* 0x0055a694 */
    char sS2f[4];                                      /* 0x0055a698 */
    char sS2e[4];                                      /* 0x0055a69c */
    char sS2d[4];                                      /* 0x0055a6a0 */
    char sS2c[4];                                      /* 0x0055a6a4 */
    char sS2b[4];                                      /* 0x0055a6a8 */
    char sNUMPAD[12];                                  /* 0x0055a6ac */
    char sNUMPAD9[12];                                 /* 0x0055a6b8 */
    char sNUMPAD7[12];                                 /* 0x0055a6c4 */
    char sS1i[4];                                      /* 0x0055a6d0 */
    char sS1h[4];                                      /* 0x0055a6d4 */
    char sS1g[4];                                      /* 0x0055a6d8 */
    char sS1f[4];                                      /* 0x0055a6dc */
    char sS1e[4];                                      /* 0x0055a6e0 */
    char sS1d[4];                                      /* 0x0055a6e4 */
    char sS1c[4];                                      /* 0x0055a6e8 */
    char sTILDE[8];                                    /* 0x0055a6ec */
    char sA[4];                                        /* 0x0055a6f4 */
    char sSPACE[8];                                    /* 0x0055a6f8 */
    char sNUMPAD0[12];                                 /* 0x0055a700 */
    char sNUMPAD6[12];                                 /* 0x0055a70c */
    char sNUMPAD4[12];                                 /* 0x0055a718 */
    char sNUMPAD5[12];                                 /* 0x0055a724 */
    char sNUMPAD8[12];                                 /* 0x0055a730 */
    char sS0i[4];                                      /* 0x0055a73c */
    char sS0h[4];                                      /* 0x0055a740 */
    char sS0g[4];                                      /* 0x0055a744 */
    char sS0f[4];                                      /* 0x0055a748 */
    char sS0e[4];                                      /* 0x0055a74c */
    char sS0d[4];                                      /* 0x0055a750 */
    char sS0c[4];                                      /* 0x0055a754 */
    char sESCAPE[8];                                   /* 0x0055a758 */
    char sW[4];                                        /* 0x0055a760 */
    char sE[4];                                        /* 0x0055a764 */
    char sF[4];                                        /* 0x0055a768 */
    char s_0055a76c[4];                                /* 0x0055a76c */
    char s_0055a770[4];                                /* 0x0055a770 */
    char sRIGHT[8];                                    /* 0x0055a774 */
    char sLEFT[8];                                     /* 0x0055a77c */
    char sDOWN[8];                                     /* 0x0055a784 */
    char sUP[4];                                       /* 0x0055a78c */
    char sKEYBOARDLAYOUT5[24];                         /* 0x0055a790 */
    unsigned char pad_0055a7a8[4];                     /* 0x0055a7a8 */
    char sKEYBOARDLAYOUT4[24];                         /* 0x0055a7ac */
    unsigned char pad_0055a7c4[4];                     /* 0x0055a7c4 */
    char sKEYBOARDLAYOUT3[24];                         /* 0x0055a7c8 */
    unsigned char pad_0055a7e0[4];                     /* 0x0055a7e0 */
    char sKEYBOARDLAYOUT2[24];                         /* 0x0055a7e4 */
    unsigned char pad_0055a7fc[4];                     /* 0x0055a7fc */
    char sKEYBOARDLAYOUT1[24];                         /* 0x0055a800 */
    unsigned char pad_0055a818[4];                     /* 0x0055a818 */
    char sFire6[8];                                    /* 0x0055a81c */
    char sFire5[8];                                    /* 0x0055a824 */
    char sSelect[8];                                   /* 0x0055a82c */
    char sStart[8];                                    /* 0x0055a834 */
    char sRTrig3[12];                                  /* 0x0055a83c */
    char sLTrig3[12];                                  /* 0x0055a848 */
    char sRTrig2[12];                                  /* 0x0055a854 */
    char sLTrig2[12];                                  /* 0x0055a860 */
    char sRTrig1[12];                                  /* 0x0055a86c */
    char sLTrig1[12];                                  /* 0x0055a878 */
    char sFire4[8];                                    /* 0x0055a884 */
    char sFire3[8];                                    /* 0x0055a88c */
    char sFire2[8];                                    /* 0x0055a894 */
    char sFire1[8];                                    /* 0x0055a89c */
    char sRight[8];                                    /* 0x0055a8a4 */
    char sLeft[8];                                     /* 0x0055a8ac */
    char sDown[8];                                     /* 0x0055a8b4 */
    char sUp[4];                                       /* 0x0055a8bc */
    uint32_t d_0055a8c0[10];                           /* 0x0055a8c0 */
    uint32_t d_0055a8e8[10];                           /* 0x0055a8e8 */
    uint32_t d_0055a910[10];                           /* 0x0055a910 */
    uint32_t d_0055a938[10];                           /* 0x0055a938 */
    uint32_t d_0055a960[10];                           /* 0x0055a960 */
    uint32_t d_0055a988[10];                           /* 0x0055a988 */
    uint32_t d_0055a9b0[10];                           /* 0x0055a9b0 */
    uint32_t d_0055a9d8[10];                           /* 0x0055a9d8 */
    uint32_t d_0055aa00[10];                           /* 0x0055aa00 */
    uint32_t d_0055aa28[10];                           /* 0x0055aa28 */
    uint32_t d_0055aa50[10];                           /* 0x0055aa50 */
    uint32_t d_0055aa78[10];                           /* 0x0055aa78 */
    SelCaptionPtrTable gRiderCaptions;                 /* 0x0055aaa0 */
    SelCaptionKinds gCaptionKinds;                     /* 0x0055aad0 */
    SelCaptionOrders gCaptionOrders;                   /* 0x0055aaf0 */
    uint32_t d_0055ab08[2];                            /* 0x0055ab08 */
    SelCaptionLocked gCaptionLocked;                   /* 0x0055ab10 */
    SelAssetTable gBikeNames;                          /* 0x0055ab18 */
    SelAssetTable gBikeTextures;                       /* 0x0055ab30 */
    SelCaptionLocked gBikeLocked;                      /* 0x0055ab48 */
    char s_0055ab50[2];                                /* 0x0055ab50 */
    unsigned char d_0055ab52[6];                       /* 0x0055ab52 */
    char s_0055ab58[2];                                /* 0x0055ab58 */
    unsigned char d_0055ab5a[6];                       /* 0x0055ab5a */
    char sG[2];                                        /* 0x0055ab60 */
    unsigned char d_0055ab62[6];                       /* 0x0055ab62 */
    char sJ[2];                                        /* 0x0055ab68 */
    unsigned char d_0055ab6a[6];                       /* 0x0055ab6a */
    char s_0055ab70[2];                                /* 0x0055ab70 */
    unsigned char d_0055ab72[6];                       /* 0x0055ab72 */
    char sM[2];                                        /* 0x0055ab78 */
    unsigned char d_0055ab7a[6];                       /* 0x0055ab7a */
    char s_0055ab80[2];                                /* 0x0055ab80 */
    unsigned char d_0055ab82[6];                       /* 0x0055ab82 */
    char s_0055ab88[2];                                /* 0x0055ab88 */
    unsigned char d_0055ab8a[6];                       /* 0x0055ab8a */
    char sV[2];                                        /* 0x0055ab90 */
    unsigned char d_0055ab92[6];                       /* 0x0055ab92 */
    char s_0055ab98[2];                                /* 0x0055ab98 */
    unsigned char d_0055ab9a[6];                       /* 0x0055ab9a */
    char sY[2];                                        /* 0x0055aba0 */
    unsigned char d_0055aba2[6];                       /* 0x0055aba2 */
    char s_0055aba8[2];                                /* 0x0055aba8 */
    unsigned char d_0055abaa[6];                       /* 0x0055abaa */
    char s_0055abb0[2];                                /* 0x0055abb0 */
    unsigned char d_0055abb2[6];                       /* 0x0055abb2 */
    char sB[2];                                        /* 0x0055abb8 */
    unsigned char d_0055abba[6];                       /* 0x0055abba */
    char s_0055abc0[2];                                /* 0x0055abc0 */
    unsigned char d_0055abc2[6];                       /* 0x0055abc2 */
    char s_0055abc8[2];                                /* 0x0055abc8 */
    unsigned char d_0055abca[6];                       /* 0x0055abca */
    char sH[2];                                        /* 0x0055abd0 */
    unsigned char d_0055abd2[6];                       /* 0x0055abd2 */
    char sK[2];                                        /* 0x0055abd8 */
    unsigned char d_0055abda[6];                       /* 0x0055abda */
    char sN[2];                                        /* 0x0055abe0 */
    unsigned char d_0055abe2[6];                       /* 0x0055abe2 */
    char s_0055abe8[2];                                /* 0x0055abe8 */
    unsigned char d_0055abea[6];                       /* 0x0055abea */
    char sQ[2];                                        /* 0x0055abf0 */
    unsigned char d_0055abf2[6];                       /* 0x0055abf2 */
    char sT[2];                                        /* 0x0055abf8 */
    unsigned char d_0055abfa[6];                       /* 0x0055abfa */
    char s_0055ac00[2];                                /* 0x0055ac00 */
    unsigned char d_0055ac02[6];                       /* 0x0055ac02 */
    char sZ[2];                                        /* 0x0055ac08 */
    unsigned char d_0055ac0a[6];                       /* 0x0055ac0a */
    char s_0055ac10[2];                                /* 0x0055ac10 */
    unsigned char d_0055ac12[6];                       /* 0x0055ac12 */
    char s_0055ac18[2];                                /* 0x0055ac18 */
    unsigned char d_0055ac1a[6];                       /* 0x0055ac1a */
    uint32_t d_0055ac20[2];                            /* 0x0055ac20 */
    uint32_t d_0055ac28[2];                            /* 0x0055ac28 */
    uint32_t d_0055ac30[2];                            /* 0x0055ac30 */
    uint32_t d_0055ac38[2];                            /* 0x0055ac38 */
    uint32_t d_0055ac40[2];                            /* 0x0055ac40 */
    uint32_t d_0055ac48[2];                            /* 0x0055ac48 */
    uint32_t d_0055ac50[2];                            /* 0x0055ac50 */
    uint32_t d_0055ac58[2];                            /* 0x0055ac58 */
    uint32_t d_0055ac60[2];                            /* 0x0055ac60 */
    uint32_t d_0055ac68[2];                            /* 0x0055ac68 */
    uint32_t d_0055ac70[2];                            /* 0x0055ac70 */
    uint32_t d_0055ac78[2];                            /* 0x0055ac78 */
    uint32_t d_0055ac80[2];                            /* 0x0055ac80 */
    uint32_t d_0055ac88[2];                            /* 0x0055ac88 */
    uint32_t d_0055ac90[2];                            /* 0x0055ac90 */
    uint32_t d_0055ac98[2];                            /* 0x0055ac98 */
    uint32_t d_0055aca0[2];                            /* 0x0055aca0 */
    uint32_t d_0055aca8[2];                            /* 0x0055aca8 */
    uint32_t d_0055acb0[2];                            /* 0x0055acb0 */
    uint32_t d_0055acb8[2];                            /* 0x0055acb8 */
    uint32_t d_0055acc0[2];                            /* 0x0055acc0 */
    uint32_t d_0055acc8[2];                            /* 0x0055acc8 */
    uint32_t d_0055acd0[2];                            /* 0x0055acd0 */
    uint32_t d_0055acd8[2];                            /* 0x0055acd8 */
    uint32_t d_0055ace0[2];                            /* 0x0055ace0 */
    uint32_t d_0055ace8[2];                            /* 0x0055ace8 */
    uint32_t d_0055acf0[2];                            /* 0x0055acf0 */
    uint32_t d_0055acf8[2];                            /* 0x0055acf8 */
    uint32_t d_0055ad00[2];                            /* 0x0055ad00 */
    uint32_t d_0055ad08[2];                            /* 0x0055ad08 */
    uint32_t d_0055ad10[2];                            /* 0x0055ad10 */
    uint32_t d_0055ad18[2];                            /* 0x0055ad18 */
    uint32_t d_0055ad20[2];                            /* 0x0055ad20 */
    uint32_t d_0055ad28[2];                            /* 0x0055ad28 */
    SelCaptionPtr60 gBikeCaptions;                     /* 0x0055ad30 */
    unsigned char pad_0055ae20[276];                   /* 0x0055ae20 */
    char sSD[8];                                       /* 0x0055af34 */
    unsigned char pad_0055af3c[4];                     /* 0x0055af3c */
    ExtrasTitle gTitleIntro;                           /* 0x0055af40 */
    ExtrasTitle gTitleDave;                            /* 0x0055afbc */
    ExtrasTitle gTitleRyan;                            /* 0x0055b038 */
    ExtrasTitle gTitleAmish;                           /* 0x0055b0b4 */
    InfoHeadTable gInfoHeadIds;                        /* 0x0055b130 */
    InfoLineTable gInfoLineIds;                        /* 0x0055b138 */
    unsigned char pad_0055b1c8[132];                   /* 0x0055b1c8 */
    char sBinMovieAmishStr[20];                        /* 0x0055b24c */
    unsigned char pad_0055b260[4];                     /* 0x0055b260 */
    char sBinMovieNyquistStr[24];                      /* 0x0055b264 */
    unsigned char pad_0055b27c[4];                     /* 0x0055b27c */
    char sBinMovieMirraStr[20];                        /* 0x0055b280 */
    unsigned char pad_0055b294[4];                     /* 0x0055b294 */
    char sBinMovieIntroStr[20];                        /* 0x0055b298 */
    unsigned char pad_0055b2ac[4];                     /* 0x0055b2ac */
    char s6ld[8];                                      /* 0x0055b2b0 */
    char s3ld2ld[16];                                  /* 0x0055b2b8 */
    char gCreditsMovie[22];        /* 0x0055b2c8; the type is local to bmx/com/menu/frontend_extras.c */
    unsigned char pad_0055b2de[6]; /* 0x0055b2de */
    char sFRONTENDFailedToAllocMemory[36];             /* 0x0055b2e4 */
    unsigned char pad_0055b308[4];                     /* 0x0055b308 */
    char sFRONTENDFailedToGuiInitializeMenuMode[44];   /* 0x0055b30c */
    unsigned char pad_0055b338[8];                     /* 0x0055b338 */
    char sFRONTENDFailedToLoadFrontendMacros[44];      /* 0x0055b340 */
    unsigned char pad_0055b36c[8];                     /* 0x0055b36c */
    char sFrontendInitAppFailedToGuiInitializeApp[48]; /* 0x0055b374 */
    unsigned char pad_0055b3a4[8];                     /* 0x0055b3a4 */
    char sAPPInitMENU[16];                             /* 0x0055b3ac */
    unsigned char pad_0055b3bc[4];                     /* 0x0055b3bc */
    char sFRONTENDMacroDataTooBigOrTooSmall[44];       /* 0x0055b3c0 */
    unsigned char pad_0055b3ec[8];                     /* 0x0055b3ec */
    char s_0055b3f4[44];                               /* 0x0055b3f4 */
    unsigned char pad_0055b420[8];                     /* 0x0055b420 */
    char sFRONTENDFailedToLoadMacroStringSet[44];      /* 0x0055b428 */
    unsigned char pad_0055b454[8];                     /* 0x0055b454 */
    char s_0055b45c[44];                               /* 0x0055b45c */
    unsigned char pad_0055b488[8];                     /* 0x0055b488 */
    char sFRONTENDCouldNotFindMacroData[40];           /* 0x0055b490 */
    unsigned char pad_0055b4b8[8];                     /* 0x0055b4b8 */
    char sCZaxisBmxSrcComMenuFrontendC[40];            /* 0x0055b4c0 */
    unsigned char pad_0055b4e8[4];                     /* 0x0055b4e8 */
    char s_0055b4ec[40];                               /* 0x0055b4ec */
    unsigned char pad_0055b514[4];                     /* 0x0055b514 */
    char sAPPShutdownMENU[20];                         /* 0x0055b518 */
    unsigned char pad_0055b52c[4];                     /* 0x0055b52c */
    int32_t gBmxSoundState;                            /* 0x0055b530 */
    int32_t gBmxMusicMode;                             /* 0x0055b534 */
    BmxVolumeTable gBmxVolumes;                        /* 0x0055b538, also gBmxMusicVolume, gBmxSfxVolume */
    uint16_t gBmxSfxFile;                              /* 0x0055b53c */
    uint16_t gBmxMenuSfxFile;                          /* 0x0055b53e */
    uint16_t gBmxGameSfxFile;                          /* 0x0055b540 */
    uint16_t gBmxGunSfxFile;                           /* 0x0055b542 */
    uint16_t gBmxRiderSfxFile;                         /* 0x0055b544 */
    uint16_t gBmxLevelSfxFile;                         /* 0x0055b546 */
    uint16_t gBmxMusicFile;                            /* 0x0055b548 */
    uint16_t gBmxSpeechFile;                           /* 0x0055b54a */
    uint8_t gBmxNextTrack;                             /* 0x0055b54c */
    uint8_t gBmxTrackLoop;                             /* 0x0055b54d */
    unsigned char pad_0055b54e[2];                     /* 0x0055b54e */
    BmxLevelTrackTable gBmxLevelTracks;                /* 0x0055b550 */
    char sErrorGettingTimerDeviceCapabilities[40];     /* 0x0055b564 */
    unsigned char pad_0055b58c[8];                     /* 0x0055b58c */
    char sErrorCreatingTimerEvent[28];                 /* 0x0055b594 */
    unsigned char pad_0055b5b0[4];                     /* 0x0055b5b0 */
    char sFailureLoadingSpeechAssetFile[36];           /* 0x0055b5b4 */
    unsigned char pad_0055b5d8[4];                     /* 0x0055b5d8 */
    char sSPEECHZSD[12];                               /* 0x0055b5dc */
    unsigned char pad_0055b5e8[4];                     /* 0x0055b5e8 */
    char sFailureLoadingMusicAssetFile[36];            /* 0x0055b5ec */
    unsigned char pad_0055b610[4];                     /* 0x0055b610 */
    char sMUSICZSD[12];                                /* 0x0055b614 */
    char sFailureLoadingCommonSFXAssetFile[40];        /* 0x0055b620 */
    unsigned char pad_0055b648[8];                     /* 0x0055b648 */
    char sBinAudio[12];                                /* 0x0055b650 */
    unsigned char pad_0055b65c[4];                     /* 0x0055b65c */
    char sSFXCOMZSD[12];                               /* 0x0055b660 */
    unsigned char pad_0055b66c[4];                     /* 0x0055b66c */
    char sFailureInitializing3DSoundSourceManager[48]; /* 0x0055b670 */
    unsigned char pad_0055b6a0[8];                     /* 0x0055b6a0 */
    char sFailureInitializingSequencedAudioManage[48]; /* 0x0055b6a8 */
    unsigned char pad_0055b6d8[8];                     /* 0x0055b6d8 */
    char sFailureInitializingChannelManager[40];       /* 0x0055b6e0 */
    unsigned char pad_0055b708[4];                     /* 0x0055b708 */
    char sSoundAlreadyOpen[20];                        /* 0x0055b70c */
    unsigned char pad_0055b720[4];                     /* 0x0055b720 */
    char sCZaxisBmxSrcComZgeBmxBmxsoundC[40];          /* 0x0055b724 */
    unsigned char pad_0055b74c[8];                     /* 0x0055b74c */
    char sSoundAlreadyClosed[24];                      /* 0x0055b754 */
    unsigned char pad_0055b76c[4];                     /* 0x0055b76c */
    char sPropLevelSoundTypeOutOfRange[36];            /* 0x0055b770 */
    unsigned char pad_0055b794[8];                     /* 0x0055b794 */
    char sNULLPointerPassedAsPositionalSFXArgumen[48]; /* 0x0055b79c */
    unsigned char pad_0055b7cc[8];                     /* 0x0055b7cc */
    char sFailureLoadingMenuSFXAssetFile[36];          /* 0x0055b7d4 */
    unsigned char pad_0055b7f8[8];                     /* 0x0055b7f8 */
    char sSFXMEN02dZSD[16];                            /* 0x0055b800 */
    unsigned char pad_0055b810[4];                     /* 0x0055b810 */
    char sFailureCreatingMusicStreamProbablyOutOf[56]; /* 0x0055b814 */
    unsigned char pad_0055b84c[12];                    /* 0x0055b84c */
    char sFailureLoadingCharacterSFXAssetFile[44];     /* 0x0055b858 */
    unsigned char pad_0055b884[8];                     /* 0x0055b884 */
    char sSFXCHR02dZSD[16];                            /* 0x0055b88c */
    unsigned char pad_0055b89c[4];                     /* 0x0055b89c */
    char sFailureLoadingLevelSFXAssetFile[40];         /* 0x0055b8a0 */
    unsigned char pad_0055b8c8[4];                     /* 0x0055b8c8 */
    char sFailureLoadingGunSFXAssetFile[36];           /* 0x0055b8cc */
    unsigned char pad_0055b8f0[8];                     /* 0x0055b8f0 */
    char sSFXGAME2ZSD[16];                             /* 0x0055b8f8 */
    char sSFXLEV02dZSD[16];                            /* 0x0055b908 */
    unsigned char pad_0055b918[4];                     /* 0x0055b918 */
    char sFailureLoadingGameSFXAssetFile[36];          /* 0x0055b91c */
    unsigned char pad_0055b940[8];                     /* 0x0055b940 */
    char sSFXGAMEZSD[12];                              /* 0x0055b948 */
    unsigned char pad_0055b954[4];                     /* 0x0055b954 */
    float d_0055b958;                                  /* 0x0055b958 */
    float d_0055b95c;                                  /* 0x0055b95c */
    ZOverGlyphRecord gZoverGlyph;                      /* 0x0055b960 */
    uint32_t d_0055b974[2];                            /* 0x0055b974 */
    char sDirtyListOverflow[20];                       /* 0x0055b97c */
    unsigned char pad_0055b990[4];                     /* 0x0055b990 */
    char sFreeListOverflow[20];                        /* 0x0055b994 */
    unsigned char pad_0055b9a8[4];                     /* 0x0055b9a8 */
    char sCZaxisBmxSrcComZgeBmxZoveriiC[40];           /* 0x0055b9ac */
    unsigned char pad_0055b9d4[8];                     /* 0x0055b9d4 */
    char sNullFontInOverlay[24];                       /* 0x0055b9dc */
    unsigned char pad_0055b9f4[4];                     /* 0x0055b9f4 */
    char sOverflow[12];                                /* 0x0055b9f8 */
    const char *gNgAssetsDir;                          /* 0x0055ba04 */
    void *p_0055ba08;                                  /* 0x0055ba08 */
    void *p_0055ba0c;                                  /* 0x0055ba0c */
    void *p_0055ba10;                                  /* 0x0055ba10 */
    void *p_0055ba14;                                  /* 0x0055ba14 */
    void *p_0055ba18;                                  /* 0x0055ba18 */
    void *p_0055ba1c;                                  /* 0x0055ba1c */
    void *p_0055ba20;                                  /* 0x0055ba20 */
    void *p_0055ba24;                                  /* 0x0055ba24 */
    unsigned char pad_0055ba28[8];                     /* 0x0055ba28 */
    char sReplayZal[12];                               /* 0x0055ba30 */
    unsigned char pad_0055ba3c[4];                     /* 0x0055ba3c */
    char sTablesZal[12];                               /* 0x0055ba40 */
    unsigned char pad_0055ba4c[4];                     /* 0x0055ba4c */
    char sSystemZal[12];                               /* 0x0055ba50 */
    unsigned char pad_0055ba5c[4];                     /* 0x0055ba5c */
    char sWSsndZal[12];                                /* 0x0055ba60 */
    char sMiscdataZal[16];                             /* 0x0055ba6c */
    char sMaterialZal[16];                             /* 0x0055ba7c */
    char sGeometryZal[16];                             /* 0x0055ba8c */
    char sMotionZal[12];                               /* 0x0055ba9c */
    unsigned char pad_0055baa8[4];                     /* 0x0055baa8 */
    char sAssets[8];                                   /* 0x0055baac */
    unsigned char pad_0055bab4[4];                     /* 0x0055bab4 */
    char sSDS[12];                                     /* 0x0055bab8 */
    char sCZaxisBmxSrcComZgeNgIFMissingZGEC[44];       /* 0x0055bac4 */
    unsigned char pad_0055baf0[8];                     /* 0x0055baf0 */
    char sZconsoleLog[12];                             /* 0x0055baf8 */
    unsigned char pad_0055bb04[4];                     /* 0x0055bb04 */
    char sEnableHorizon[16];                           /* 0x0055bb08 */
    unsigned char pad_0055bb18[4];                     /* 0x0055bb18 */
    char sZglDisableHorizon[20];                       /* 0x0055bb1c */
    unsigned char pad_0055bb30[4];                     /* 0x0055bb30 */
    char sZsystemSetActionMode[24];                    /* 0x0055bb34 */
    unsigned char pad_0055bb4c[4];                     /* 0x0055bb4c */
    char sZsystemSetMenuMode[20];                      /* 0x0055bb50 */
    unsigned char pad_0055bb64[4];                     /* 0x0055bb64 */
    char sZfxDestroyAll[16];                           /* 0x0055bb68 */
    char sZfxRegisterFuncTable[24];                    /* 0x0055bb78 */
    unsigned char pad_0055bb90[4];                     /* 0x0055bb90 */
    char sAVI[4];                                      /* 0x0055bb94 */
    uint32_t gNgKeyMaps[40];                           /* 0x0055bb98 */
    uint32_t gNgRamDisk;                               /* 0x0055bc38 */
    int gNgLegalPending;                               /* 0x0055bc3c */
    const char *gNgWrapName;                           /* 0x0055bc40 */
    void *gNgLevelNames;                               /* 0x0055bc44 */
    void *p_0055bc48;                                  /* 0x0055bc48 */
    void *p_0055bc4c;                                  /* 0x0055bc4c */
    void *p_0055bc50;                                  /* 0x0055bc50 */
    void *p_0055bc54;                                  /* 0x0055bc54 */
    void *p_0055bc58;                                  /* 0x0055bc58 */
    void *p_0055bc5c;                                  /* 0x0055bc5c */
    void *p_0055bc60;                                  /* 0x0055bc60 */
    void *p_0055bc64;                                  /* 0x0055bc64 */
    void *p_0055bc68;                                  /* 0x0055bc68 */
    void *p_0055bc6c;                                  /* 0x0055bc6c */
    void *p_0055bc70;                                  /* 0x0055bc70 */
    void *p_0055bc74;                                  /* 0x0055bc74 */
    void *p_0055bc78;                                  /* 0x0055bc78 */
    uint32_t gNgDebugFaceColor;                        /* 0x0055bc7c */
    uint32_t gNgDebugEdgeColor;                        /* 0x0055bc80 */
    uint32_t gNgDebugTaggedColor;                      /* 0x0055bc84 */
    uint32_t gNgDebugRollerColor;                      /* 0x0055bc88 */
    uint8_t gNgMainLightColor;                         /* 0x0055bc8c */
    unsigned char d_0055bc8d[3];                       /* 0x0055bc8d */
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
    } gNgLightRef[3];               /* 0x0055bcc0; the type is local to bmx/com/zge/ngif/ngapp.c */
    uint8_t gNgLightColors[3][4];   /* 0x0055bcf0; the type is local to bmx/com/zge/ngif/ngapp.c */
    uint8_t gNgSpotMainColor;       /* 0x0055bcfc */
    unsigned char d_0055bcfd[7];    /* 0x0055bcfd */
    uint32_t gNgLegalTime;          /* 0x0055bd04 */
    float gFxQuality;               /* 0x0055bd08 */
    float gFxFps;                   /* 0x0055bd0c */
    unsigned char pad_0055bd10[76]; /* 0x0055bd10 */
    char sTestB[8];                 /* 0x0055bd5c */
    char sTestA[8];                 /* 0x0055bd64 */
    char sAcclaimVert[16];          /* 0x0055bd6c */
    char sAcclaimStreet[16];        /* 0x0055bd7c */
    unsigned char pad_0055bd8c[4];  /* 0x0055bd8c */
    char sAcclaimDirt[16];          /* 0x0055bd90 */
    char sProTourVert[16];          /* 0x0055bda0 */
    char sProTourStreet[16];        /* 0x0055bdb0 */
    unsigned char pad_0055bdc0[4];  /* 0x0055bdc0 */
    char sProTourDirt[16];          /* 0x0055bdc4 */
    char sWoodwardVert[16];         /* 0x0055bdd4 */
    char sWoodwardStreet[16];       /* 0x0055bde4 */
    unsigned char pad_0055bdf4[4];  /* 0x0055bdf4 */
    char sWoodwardDirt[16];         /* 0x0055bdf8 */
    char sGreenvilleVert[16];       /* 0x0055be08 */
    unsigned char pad_0055be18[4];  /* 0x0055be18 */
    char sGreenvilleStreet[20];     /* 0x0055be1c */
    unsigned char pad_0055be30[4];  /* 0x0055be30 */
    char sGreenvilleDirt[16];       /* 0x0055be34 */
    unsigned char pad_0055be44[4];  /* 0x0055be44 */
    char sIntwrapZal[12];           /* 0x0055be48 */
    unsigned char pad_0055be54[4];  /* 0x0055be54 */
    char s155417[12];               /* 0x0055be58 */
    char s_0055be64[4];             /* 0x0055be64 */
    char sNov222000[12];            /* 0x0055be68 */
    unsigned char pad_0055be74[4];  /* 0x0055be74 */
    char sNextGenBMX[16];           /* 0x0055be78 */
    unsigned char pad_0055be88[4];  /* 0x0055be88 */
    char sLight[8];                 /* 0x0055be8c */
    char sSpotLightT[12];           /* 0x0055be94 */
    unsigned char pad_0055bea0[4];  /* 0x0055bea0 */
    char sSpotLight[12];            /* 0x0055bea4 */
    char sCouldNotGenerateFileNameForRAMDisk[44];      /* 0x0055beb0 */
    unsigned char pad_0055bedc[8];                     /* 0x0055bedc */
    char sSIZE[8];                                     /* 0x0055bee4 */
    char sADDRESS[8];                                  /* 0x0055beec */
    unsigned char pad_0055bef4[4];                     /* 0x0055bef4 */
    char sCouldNotLocateARAMDiskDeviceOnThisSyste[52]; /* 0x0055bef8 */
    unsigned char pad_0055bf2c[12];                    /* 0x0055bf2c */
    ZramdiskDeviceName gZramdiskDeviceName;            /* 0x0055bf38 */
    char sNoDevices[12];                               /* 0x0055bf40 */
    unsigned char pad_0055bf4c[4];                     /* 0x0055bf4c */
    char sCouldNotLoadMenuArtAssetFile[36];            /* 0x0055bf50 */
    unsigned char pad_0055bf74[8];                     /* 0x0055bf74 */
    char sInterfaceAssetWrapperDoesNotExist[40];       /* 0x0055bf7c */
    unsigned char pad_0055bfa4[8];                     /* 0x0055bfa4 */
    char sInterfaceArtAlreadyLoaded[32];               /* 0x0055bfac */
    unsigned char pad_0055bfcc[4];                     /* 0x0055bfcc */
    char sCZaxisBmxSrcComZgeNgIFNgAppC[40];            /* 0x0055bfd0 */
    unsigned char pad_0055bff8[8];                     /* 0x0055bff8 */
    char sBinMovieINTROAVI[20];                        /* 0x0055c000 */
    unsigned char pad_0055c014[4];                     /* 0x0055c014 */
    char sBinMovieZAXISAVI[20];                        /* 0x0055c018 */
    unsigned char pad_0055c02c[4];                     /* 0x0055c02c */
    char sBinMovieACCLAIMAVI[24];                      /* 0x0055c030 */
    unsigned char pad_0055c048[4];                     /* 0x0055c048 */
    char sDaveMirraFreestyleBMX[28];                   /* 0x0055c04c */
    unsigned char pad_0055c068[4];                     /* 0x0055c068 */
    char sLOADING[8];                                  /* 0x0055c06c */
    unsigned char pad_0055c074[4];                     /* 0x0055c074 */
    char sGlobals[8];                                  /* 0x0055c078 */
    unsigned char pad_0055c080[4];                     /* 0x0055c080 */
    char sErrorLoadingLevel[20];                       /* 0x0055c084 */
    unsigned char pad_0055c098[4];                     /* 0x0055c098 */
    char sFailedToLoadInterfaceAssets[32];             /* 0x0055c09c */
    unsigned char pad_0055c0bc[8];                     /* 0x0055c0bc */
    char sLEGAL2[8];                                   /* 0x0055c0c4 */
    char sLEGAL[8];                                    /* 0x0055c0cc */
    unsigned char pad_0055c0d4[4];                     /* 0x0055c0d4 */
    const char *gFxSkidBase;                           /* 0x0055c0d8 */
    void *gFxSkidFrameNames[16];              /* 0x0055c0dc; the type is local to bmx/com/zge/ngif/ngiffx.c */
    const char *gFxSkidName;                  /* 0x0055c11c */
    uint32_t gFxSkidConfig;                   /* 0x0055c120 */
    unsigned char pad_0055c124[4];            /* 0x0055c124 */
    float gFxSparkDefault[16];                /* 0x0055c128, also gFxShardDefault; the type is local to
                                                 bmx/com/zge/ngif/ngiffx.c */
    uint32_t d_0055c168[14];                  /* 0x0055c168 */
    float gFxDustDefault[16];                 /* 0x0055c1a0, also gFxPosterDefault; the type is local to
                                                 bmx/com/zge/ngif/ngiffx.c */
    uint32_t d_0055c1e0[16];                  /* 0x0055c1e0 */
    float gFxRippleDefault[16];               /* 0x0055c220, also gFxLightningDefault; the type is local to
                                                 bmx/com/zge/ngif/ngiffx.c */
    uint32_t d_0055c260[6];                   /* 0x0055c260 */
    ZfxEffect gFxSpark;                       /* 0x0055c278 */
    ZfxEffect gFxShard;                       /* 0x0055c2b0 */
    ZfxEffect gFxSplash;                      /* 0x0055c2e8 */
    ZfxEffect gFxDust;                        /* 0x0055c320 */
    ZfxEffect gFxPoster;                      /* 0x0055c358 */
    ZfxEffect gFxRipple;                      /* 0x0055c390 */
    ZfxEffect gFxLightning;                   /* 0x0055c3c8 */
    float gFxSparkCreateDefault[16];          /* 0x0055c400, also gFxSplashCreateDefault; the type is local to
                                                 bmx/com/zge/ngif/ngiffx.c */
    uint32_t d_0055c440[12];                  /* 0x0055c440 */
    float gFxGravityParams[16];               /* 0x0055c470; the type is local to bmx/com/zge/ngif/ngiffx.c */
    unsigned char pad_0055c4b0[140];          /* 0x0055c4b0 */
    char sANIM[8];                            /* 0x0055c53c */
    char sFAN3[8];                            /* 0x0055c544 */
    char sFAN2[8];                            /* 0x0055c54c */
    char sFAN1[8];                            /* 0x0055c554 */
    char sPoster[8];                          /* 0x0055c55c */
    char sLightning[12];                      /* 0x0055c564 */
    char sDUST16[8];                          /* 0x0055c570 */
    char sDUST15[8];                          /* 0x0055c578 */
    char sDUST14[8];                          /* 0x0055c580 */
    char sDUST13[8];                          /* 0x0055c588 */
    char sDUST12[8];                          /* 0x0055c590 */
    char sDUST11[8];                          /* 0x0055c598 */
    char sDUST10[8];                          /* 0x0055c5a0 */
    char sDUST09[8];                          /* 0x0055c5a8 */
    char sDUST08[8];                          /* 0x0055c5b0 */
    char sDUST07[8];                          /* 0x0055c5b8 */
    char sDUST06[8];                          /* 0x0055c5c0 */
    char sDUST05[8];                          /* 0x0055c5c8 */
    char sDUST04[8];                          /* 0x0055c5d0 */
    char sDUST03[8];                          /* 0x0055c5d8 */
    char sDUST02[8];                          /* 0x0055c5e0 */
    char sDUST01[8];                          /* 0x0055c5e8 */
    char sSPLASH[8];                          /* 0x0055c5f0 */
    char sRIPPLE[8];                          /* 0x0055c5f8 */
    char sSKID[8];                            /* 0x0055c600 */
    char sSPARK[8];                           /* 0x0055c608 */
    char sSFXCube[8];                         /* 0x0055c610 */
    unsigned char pad_0055c618[4];            /* 0x0055c618 */
    char sFloorcoll[12];                      /* 0x0055c61c */
    char sGravity[8];                         /* 0x0055c628 */
    unsigned char pad_0055c630[4];            /* 0x0055c630 */
    char sCZaxisBmxSrcComZgeNgIFNgIFfxC[40];  /* 0x0055c634 */
    unsigned char pad_0055c65c[8];            /* 0x0055c65c */
    char sZfxCreateUnsupportedEffect[32];     /* 0x0055c664 */
    unsigned char pad_0055c684[8];            /* 0x0055c684 */
    uint32_t gNgifBrightness;                 /* 0x0055c68c */
    float gStripScale0;                       /* 0x0055c690, also gNgifScreenW */
    float gNgifScreenH;                       /* 0x0055c694 */
    uint32_t *gZoverOt;                       /* 0x0055c698, also gNgifOtBase */
    unsigned char pad_0055c69c[4];            /* 0x0055c69c */
    char sCZaxisBmxSrcComZgeNgIFNgIFpsxC[40]; /* 0x0055c6a0 */
    unsigned char pad_0055c6c8[8];            /* 0x0055c6c8 */
    char sDrawPrimPrimNotCodedLINEG4[36];     /* 0x0055c6d0 */
    unsigned char pad_0055c6f4[4];            /* 0x0055c6f4 */
    char sDrawPrimPrimNotCodedLINEF4[36];     /* 0x0055c6f8 */
    unsigned char pad_0055c71c[4];            /* 0x0055c71c */
    char sDrawPrimPrimNotCodedLINEG3[36];     /* 0x0055c720 */
    unsigned char pad_0055c744[4];            /* 0x0055c744 */
    char sDrawPrimPrimNotCodedLINEF3[36];     /* 0x0055c748 */
    unsigned char pad_0055c76c[4];            /* 0x0055c76c */
    char sDrawPrimPrimNotCodedSPRT[32];       /* 0x0055c770 */
    unsigned char pad_0055c790[8];            /* 0x0055c790 */
    char sDrawPrimPrimNotCodedSPRT16[36];     /* 0x0055c798 */
    unsigned char pad_0055c7bc[4];            /* 0x0055c7bc */
    char sDrawPrimPrimNotCodedSPRT8[32];      /* 0x0055c7c0 */
    unsigned char pad_0055c7e0[8];            /* 0x0055c7e0 */
    char sDrawPrimPrimNotCodedGT3[32];        /* 0x0055c7e8 */
    unsigned char pad_0055c808[4];            /* 0x0055c808 */
    char sDrawPrimPrimNotCodedG3[32];         /* 0x0055c80c */
    unsigned char pad_0055c82c[4];            /* 0x0055c82c */
    char sDrawPrimPrimNotCodedFT3[32];        /* 0x0055c830 */
    unsigned char pad_0055c850[4];            /* 0x0055c850 */
    char sCannotGetSizeOfObject[28];          /* 0x0055c854 */
    unsigned char pad_0055c870[4];            /* 0x0055c870 */
    char sCanNotGetSizeOfObject[28];          /* 0x0055c874 */
    unsigned char pad_0055c890[4];            /* 0x0055c890 */
    char sNotCodedForTexturedTris[28];        /* 0x0055c894 */
    unsigned char pad_0055c8b0[8];            /* 0x0055c8b0 */
    char sUncodedForPolygonsInScreenSpace[40];         /* 0x0055c8b8 */
    unsigned char pad_0055c8e0[4];                     /* 0x0055c8e0 */
    char sInvalidPaletteIndex[24];                     /* 0x0055c8e4 */
    unsigned char pad_0055c8fc[4];                     /* 0x0055c8fc */
    char sBufferOverflowInPsxprimDrawFontChar[44];     /* 0x0055c900 */
    unsigned char pad_0055c92c[8];                     /* 0x0055c92c */
    char sRightFoot[12];                               /* 0x0055c934 */
    char sRightLowLeg[12];                             /* 0x0055c940 */
    unsigned char pad_0055c94c[4];                     /* 0x0055c94c */
    char sRightUpLeg[12];                              /* 0x0055c950 */
    unsigned char pad_0055c95c[4];                     /* 0x0055c95c */
    char sLeftFoot[12];                                /* 0x0055c960 */
    char sLeftLowLeg[12];                              /* 0x0055c96c */
    unsigned char pad_0055c978[4];                     /* 0x0055c978 */
    char sLeftUpLeg[12];                               /* 0x0055c97c */
    char sRightHand[12];                               /* 0x0055c988 */
    char sRightLowArm[12];                             /* 0x0055c994 */
    unsigned char pad_0055c9a0[4];                     /* 0x0055c9a0 */
    char sRightUpArm[12];                              /* 0x0055c9a4 */
    unsigned char pad_0055c9b0[4];                     /* 0x0055c9b0 */
    char sLeftHand[12];                                /* 0x0055c9b4 */
    char sLeftLowArm[12];                              /* 0x0055c9c0 */
    unsigned char pad_0055c9cc[4];                     /* 0x0055c9cc */
    char sLeftUpArm[12];                               /* 0x0055c9d0 */
    char sHead[8];                                     /* 0x0055c9dc */
    char sChest[8];                                    /* 0x0055c9e4 */
    char sHips[8];                                     /* 0x0055c9ec */
    char sSeatGrip[12];                                /* 0x0055c9f4 */
    char sRightGrip[12];                               /* 0x0055ca00 */
    char sLeftGrip[12];                                /* 0x0055ca0c */
    char sFrontWheel[12];                              /* 0x0055ca18 */
    unsigned char pad_0055ca24[4];                     /* 0x0055ca24 */
    char sBackWheel[12];                               /* 0x0055ca28 */
    char sLeftPedal[12];                               /* 0x0055ca34 */
    char sRightPedal[12];                              /* 0x0055ca40 */
    unsigned char pad_0055ca4c[4];                     /* 0x0055ca4c */
    char sSprocket[12];                                /* 0x0055ca50 */
    char sHandlebars[12];                              /* 0x0055ca5c */
    unsigned char pad_0055ca68[4];                     /* 0x0055ca68 */
    char sHipsdeck[12];                                /* 0x0055ca6c */
    char sCZaxisBmxSrcComZgeNgIFNgIFzactorC[44];       /* 0x0055ca78 */
    unsigned char pad_0055caa4[8];                     /* 0x0055caa4 */
    char sTestzactorGetAbsolutePartPositionUnknow[48]; /* 0x0055caac */
    unsigned char pad_0055cadc[12];                    /* 0x0055cadc */
    char sTestzactorGetPartLengthUnknownPart[40];      /* 0x0055cae8 */
    unsigned char pad_0055cb10[8];                     /* 0x0055cb10 */
    char sMissingHashFunction[24];                     /* 0x0055cb18 */
    unsigned char pad_0055cb30[4];                     /* 0x0055cb30 */
    char sCZaxisBmxSrcComZgeNgIFNgIFzbspC[44];         /* 0x0055cb34 */
    unsigned char pad_0055cb60[8];                     /* 0x0055cb60 */
    char sBspInit[12];                                 /* 0x0055cb68 */
    char sBspUpdate[12];                               /* 0x0055cb74 */
    unsigned char pad_0055cb80[4];                     /* 0x0055cb80 */
    char sShutdown[12];                                /* 0x0055cb84 */
    char sInitializeScene[20];                         /* 0x0055cb90 */
    char sLoadScene[12];                               /* 0x0055cba4 */
    unsigned char pad_0055cbb0[4];                     /* 0x0055cbb0 */
    char sLoadAndProcessMap[24];                       /* 0x0055cbb4 */
    unsigned char pad_0055cbcc[4];                     /* 0x0055cbcc */
    char sAllocAndLoadMap[20];                         /* 0x0055cbd0 */
    unsigned char pad_0055cbe4[4];                     /* 0x0055cbe4 */
    char sFreeMap[12];                                 /* 0x0055cbe8 */
    char sSetTreeVisible[20];                          /* 0x0055cbf4 */
    char sConvertTreeFromLoad[24];                     /* 0x0055cc08 */
    unsigned char pad_0055cc20[4];                     /* 0x0055cc20 */
    char sRelativeLodZ[16];                            /* 0x0055cc24 */
    unsigned char pad_0055cc34[4];                     /* 0x0055cc34 */
    char sPreprocessScene[20];                         /* 0x0055cc38 */
    char sPreprocessTerrainNode[24];                   /* 0x0055cc4c */
    unsigned char pad_0055cc64[4];                     /* 0x0055cc64 */
    char sPreprocessFaceList[24];                      /* 0x0055cc68 */
    unsigned char pad_0055cc80[4];                     /* 0x0055cc80 */
    char sViewerInPosSide[20];                         /* 0x0055cc84 */
    unsigned char pad_0055cc98[4];                     /* 0x0055cc98 */
    char sSndRegister[16];                             /* 0x0055cc9c */
    char sSndSetState[16];                             /* 0x0055ccac */
    char sSndGetHandle[16];                            /* 0x0055ccbc */
    unsigned char pad_0055cccc[4];                     /* 0x0055cccc */
    char sDvlitehandle[16];                            /* 0x0055ccd0 */
    char sDvlitegetcolor[16];                          /* 0x0055cce0 */
    unsigned char pad_0055ccf0[4];                     /* 0x0055ccf0 */
    char sDvlitelite[12];                              /* 0x0055ccf4 */
    unsigned char pad_0055cd00[4];                     /* 0x0055cd00 */
    char sDvliteinitialize[20];                        /* 0x0055cd04 */
    char sDvliteAttachToProps[24];                     /* 0x0055cd18 */
    unsigned char pad_0055cd30[4];                     /* 0x0055cd30 */
    char sPropActivate[16];                            /* 0x0055cd34 */
    char sPropDeactivate[16];                          /* 0x0055cd44 */
    unsigned char pad_0055cd54[4];                     /* 0x0055cd54 */
    char sActivateTree[16];                            /* 0x0055cd58 */
    char sDeactivateTree[16];                          /* 0x0055cd68 */
    unsigned char pad_0055cd78[4];                     /* 0x0055cd78 */
    char sInitializeTreeRef[20];                       /* 0x0055cd7c */
    unsigned char pad_0055cd90[4];                     /* 0x0055cd90 */
    char sCreateTreeRef[16];                           /* 0x0055cd94 */
    unsigned char pad_0055cda4[4];                     /* 0x0055cda4 */
    char sDestroyTreeRef[20];                          /* 0x0055cda8 */
    char sPropAddTreeRef[20];                          /* 0x0055cdbc */
    unsigned char pad_0055cdd0[4];                     /* 0x0055cdd0 */
    char sRemoveTreeRef[16];                           /* 0x0055cdd4 */
    unsigned char pad_0055cde4[4];                     /* 0x0055cde4 */
    char sUpdateTreeRef[16];                           /* 0x0055cde8 */
    unsigned char pad_0055cdf8[4];                     /* 0x0055cdf8 */
    char sAddToTree[12];                               /* 0x0055cdfc */
    unsigned char pad_0055ce08[4];                     /* 0x0055ce08 */
    char sRemoveFromTree[20];                          /* 0x0055ce0c */
    char sTotallyWithinTree[20];                       /* 0x0055ce20 */
    unsigned char pad_0055ce34[4];                     /* 0x0055ce34 */
    char sOverlapsTree[16];                            /* 0x0055ce38 */
    char sFindTagged[12];                              /* 0x0055ce48 */
    unsigned char pad_0055ce54[4];                     /* 0x0055ce54 */
    char sZbspPropMove[16];                            /* 0x0055ce58 */
    char sGetNumDefs[16];                              /* 0x0055ce68 */
    char sGetTypeFromHash[20];                         /* 0x0055ce78 */
    unsigned char pad_0055ce8c[4];                     /* 0x0055ce8c */
    char sFxCreateTree[16];                            /* 0x0055ce90 */
    unsigned char pad_0055cea0[4];                     /* 0x0055cea0 */
    char sDestroyTree[16];                             /* 0x0055cea4 */
    char sDrawTerrain[16];                             /* 0x0055ceb4 */
    char sDrawTerrainNode[20];                         /* 0x0055cec4 */
    unsigned char pad_0055ced8[4];                     /* 0x0055ced8 */
    char sDrawFaceList[16];                            /* 0x0055cedc */
    unsigned char pad_0055ceec[4];                     /* 0x0055ceec */
    char sSetDebug[12];                                /* 0x0055cef0 */
    char sDrawLineZ[12];                               /* 0x0055cefc */
    unsigned char pad_0055cf08[4];                     /* 0x0055cf08 */
    char sDrawFaceNormal[20];                          /* 0x0055cf0c */
    char sDrawFaceFlat[16];                            /* 0x0055cf20 */
    unsigned char pad_0055cf30[4];                     /* 0x0055cf30 */
    char sShowBoundingBox[20];                         /* 0x0055cf34 */
    unsigned char pad_0055cf48[4];                     /* 0x0055cf48 */
    char sShowEdges[12];                               /* 0x0055cf4c */
    unsigned char pad_0055cf58[4];                     /* 0x0055cf58 */
    char sShowPropVolume[20];                          /* 0x0055cf5c */
    char sGetFaceNum[16];                              /* 0x0055cf70 */
    char sTrnDumpTree[16];                             /* 0x0055cf80 */
    char sShowPartition[16];                           /* 0x0055cf90 */
    unsigned char pad_0055cfa0[4];                     /* 0x0055cfa0 */
    char sDrawPartition[16];                           /* 0x0055cfa4 */
    unsigned char pad_0055cfb4[4];                     /* 0x0055cfb4 */
    char sGetTerrainFaces[20];                         /* 0x0055cfb8 */
    unsigned char pad_0055cfcc[4];                     /* 0x0055cfcc */
    char sGetTreeObject[16];                           /* 0x0055cfd0 */
    unsigned char pad_0055cfe0[4];                     /* 0x0055cfe0 */
    char sPointWithinTreeObject[28];                   /* 0x0055cfe4 */
    unsigned char pad_0055d000[4];                     /* 0x0055d000 */
    char sGetTreeObjects[20];                          /* 0x0055d004 */
    char sGetTerrainNode[20];                          /* 0x0055d018 */
    char sGetMaterialInfo[20];                         /* 0x0055d02c */
    unsigned char pad_0055d040[4];                     /* 0x0055d040 */
    char sGetMatRefData[20];                           /* 0x0055d044 */
    char sGetMatIdx[12];                               /* 0x0055d058 */
    unsigned char pad_0055d064[4];                     /* 0x0055d064 */
    char sMatRefList[16];                              /* 0x0055d068 */
    char sDoesVertExist[16];                           /* 0x0055d078 */
    unsigned char pad_0055d088[4];                     /* 0x0055d088 */
    char sFacesAtSamePos[20];                          /* 0x0055d08c */
    unsigned char pad_0055d0a0[4];                     /* 0x0055d0a0 */
    char sSceneSaveState[20];                          /* 0x0055d0a4 */
    char sSceneRestoreState[20];                       /* 0x0055d0b8 */
    unsigned char pad_0055d0cc[4];                     /* 0x0055d0cc */
    char sEdgeListOverflow[20];                        /* 0x0055d0d0 */
    unsigned char pad_0055d0e4[4];                     /* 0x0055d0e4 */
    char sGetNormalVector[20];                         /* 0x0055d0e8 */
    unsigned char pad_0055d0fc[4];                     /* 0x0055d0fc */
    char sGetSideVector[16];                           /* 0x0055d100 */
    unsigned char pad_0055d110[4];                     /* 0x0055d110 */
    char sBspcollide[12];                              /* 0x0055d114 */
    unsigned char pad_0055d120[4];                     /* 0x0055d120 */
    char sCreateSpanPlanes[20];                        /* 0x0055d124 */
    unsigned char pad_0055d138[4];                     /* 0x0055d138 */
    char sAddToFacelist[16];                           /* 0x0055d13c */
    unsigned char pad_0055d14c[4];                     /* 0x0055d14c */
    char sGetTerrainHeightinf[24];                     /* 0x0055d150 */
    unsigned char pad_0055d168[4];                     /* 0x0055d168 */
    char sTestRayCollision[20];                        /* 0x0055d16c */
    unsigned char pad_0055d180[4];                     /* 0x0055d180 */
    char sBsptest[8];                                  /* 0x0055d184 */
    unsigned char pad_0055d18c[4];                     /* 0x0055d18c */
    char sTestresetfacecolors[20];                     /* 0x0055d190 */
    unsigned char pad_0055d1a4[4];                     /* 0x0055d1a4 */
    char sHilightnodefaces[20];                        /* 0x0055d1a8 */
    char sSetFaceRgb[16];                              /* 0x0055d1bc */
    char sPrintFaceListInf[20];                        /* 0x0055d1cc */
    unsigned char pad_0055d1e0[4];                     /* 0x0055d1e0 */
    char sTrntestraycollision[20];                     /* 0x0055d1e4 */
    unsigned char pad_0055d1f8[4];                     /* 0x0055d1f8 */
    char sCreatenodeshit[16];                          /* 0x0055d1fc */
    unsigned char pad_0055d20c[4];                     /* 0x0055d20c */
    char sAddToNodeslist[20];                          /* 0x0055d210 */
    char sAddToFacesList[20];                          /* 0x0055d224 */
    unsigned char pad_0055d238[4];                     /* 0x0055d238 */
    char sLineToNodes[16];                             /* 0x0055d23c */
    char sLineToFaces[16];                             /* 0x0055d24c */
    char sLineplaneintersection[24];                   /* 0x0055d25c */
    unsigned char pad_0055d274[4];                     /* 0x0055d274 */
    char sClipRayToObject[20];                         /* 0x0055d278 */
    unsigned char pad_0055d28c[4];                     /* 0x0055d28c */
    char sClipLineToProps[20];                         /* 0x0055d290 */
    unsigned char pad_0055d2a4[4];                     /* 0x0055d2a4 */
    char sCliplinetoprop[16];                          /* 0x0055d2a8 */
    unsigned char pad_0055d2b8[4];                     /* 0x0055d2b8 */
    char sBuildrayboundingbox[20];                     /* 0x0055d2bc */
    unsigned char pad_0055d2d0[4];                     /* 0x0055d2d0 */
    char sAddpropfaces[16];                            /* 0x0055d2d4 */
    char sZbspComputeFacePlaneEq[24];                  /* 0x0055d2e4 */
    unsigned char pad_0055d2fc[4];                     /* 0x0055d2fc */
    char sTaginitialize[16];                           /* 0x0055d300 */
    char sTagregisterprofunc[20];                      /* 0x0055d310 */
    unsigned char pad_0055d324[4];                     /* 0x0055d324 */
    char sFacetagregister[16];                         /* 0x0055d328 */
    unsigned char pad_0055d338[4];                     /* 0x0055d338 */
    char sFacetagpreprocess[20];                       /* 0x0055d33c */
    unsigned char pad_0055d350[4];                     /* 0x0055d350 */
    char sFacetagprocess[16];                          /* 0x0055d354 */
    unsigned char pad_0055d364[4];                     /* 0x0055d364 */
    char sFacetagfinddef[16];                          /* 0x0055d368 */
    unsigned char pad_0055d378[4];                     /* 0x0055d378 */
    char sFacetagregregister[20];                      /* 0x0055d37c */
    unsigned char pad_0055d390[4];                     /* 0x0055d390 */
    char sFacetagpreattachasref[24];                   /* 0x0055d394 */
    unsigned char pad_0055d3ac[4];                     /* 0x0055d3ac */
    char sFacetagpreattachsmposasref[28];              /* 0x0055d3b0 */
    unsigned char pad_0055d3cc[4];                     /* 0x0055d3cc */
    char sFacetagproswapwindow[24];                    /* 0x0055d3d0 */
    unsigned char pad_0055d3e8[4];                     /* 0x0055d3e8 */
    char sBufferOverflow[16];                          /* 0x0055d3ec */
    unsigned char pad_0055d3fc[4];                     /* 0x0055d3fc */
    char sColspherecreatenodeshit[24];                 /* 0x0055d400 */
    unsigned char pad_0055d418[4];                     /* 0x0055d418 */
    char sHitnodeslist[16];                            /* 0x0055d41c */
    char sTestwithfacelist[20];                        /* 0x0055d42c */
    char sFaceboundingbox[16];                         /* 0x0055d440 */
    unsigned char pad_0055d450[4];                     /* 0x0055d450 */
    char sZbspColSphereDraw[20];                       /* 0x0055d454 */
    unsigned char pad_0055d468[4];                     /* 0x0055d468 */
    char sPropListOverflow[20];                        /* 0x0055d46c */
    unsigned char pad_0055d480[4];                     /* 0x0055d480 */
    char sZbspFindFaceByType[24];                      /* 0x0055d484 */
    uint32_t d_0055d49c[23];                           /* 0x0055d49c */
    ZcollideBoxEdges gZcollideBoxEdges;                /* 0x0055d4f8 */
    ZcollideBoxEdge gZcollideBarEdge;                  /* 0x0055d504 */
    unsigned char pad_0055d507[21];                    /* 0x0055d507 */
    char sDegenerateCollision[24];                     /* 0x0055d51c */
    unsigned char pad_0055d534[4];                     /* 0x0055d534 */
    char sInvalidParameter[20];                        /* 0x0055d538 */
    unsigned char pad_0055d54c[4];                     /* 0x0055d54c */
    char sCZaxisBmxSrcComZgeNgIFNgIFzcollideC[48];     /* 0x0055d550 */
    unsigned char pad_0055d580[8];                     /* 0x0055d580 */
    char sZcolbboxcollision[20];                       /* 0x0055d588 */
    unsigned char pad_0055d59c[4];                     /* 0x0055d59c */
    char sGetTerrainFace[20];                          /* 0x0055d5a0 */
    char sPropHitBox[16];                              /* 0x0055d5b4 */
    char sGetPlaneHeight[16];                          /* 0x0055d5c4 */
    unsigned char pad_0055d5d4[4];                     /* 0x0055d5d4 */
    char sGenerateBoundBox[20];                        /* 0x0055d5d8 */
    unsigned char pad_0055d5ec[4];                     /* 0x0055d5ec */
    char sInitPropFacesList[24];                       /* 0x0055d5f0 */
    unsigned char pad_0055d608[4];                     /* 0x0055d608 */
    char sGeneratePropBoundingVolume[32];              /* 0x0055d60c */
    unsigned char pad_0055d62c[4];                     /* 0x0055d62c */
    char sNotCoded[12];                                /* 0x0055d630 */
    char sPropMove[12];                                /* 0x0055d63c */
    char sZMEMDMMFailedToInitializeDMM[36];            /* 0x0055d648 */
    unsigned char pad_0055d66c[8];                     /* 0x0055d66c */
    char sCZaxisBmxSrcComZgeNgIFNgIfzmemDMMC[44];      /* 0x0055d674 */
    unsigned char pad_0055d6a0[8];                     /* 0x0055d6a0 */
    char sZMEMDMMErrorDMMAlreadyInitialized[44];       /* 0x0055d6a8 */
    unsigned char pad_0055d6d4[8];                     /* 0x0055d6d4 */
    char sZMEMDMMDMMShutdownWithoutBeingInitializ[52]; /* 0x0055d6dc */
    unsigned char pad_0055d710[8];                     /* 0x0055d710 */
    char sZMEMOutOfDMMHandles[28];                     /* 0x0055d718 */
    unsigned char pad_0055d734[4];                     /* 0x0055d734 */
    char sZMEMDMMDMMFailedToAllocate[32];              /* 0x0055d738 */
    unsigned char pad_0055d758[8];                     /* 0x0055d758 */
    char sZMEMOutOfRAM[20];                            /* 0x0055d760 */
    char sZMEMDMMDMMAllocationSize0[36];               /* 0x0055d774 */
    unsigned char pad_0055d798[4];                     /* 0x0055d798 */
    char sZMEMDMMDMMUsedWithoutBeingInitialized[48];   /* 0x0055d79c */
    unsigned char pad_0055d7cc[8];                     /* 0x0055d7cc */
    char sZMEMDMMInvalidHandlePassedToZmemDMMDeal[52]; /* 0x0055d7d4 */
    unsigned char pad_0055d808[8];                     /* 0x0055d808 */
    char sLocked[8];                                   /* 0x0055d810 */
    char sZMEMDMMTryingToLockHandle0[36];              /* 0x0055d818 */
    unsigned char pad_0055d83c[4];                     /* 0x0055d83c */
    char sZMEMDMMTryingToUnlockHandle0[36];            /* 0x0055d840 */
    unsigned char pad_0055d864[8];                     /* 0x0055d864 */
    char sZMEMDMMDMMOptimizeFoundUnrecognizedPoin[76]; /* 0x0055d86c */
    unsigned char pad_0055d8b8[16];                    /* 0x0055d8b8 */
    char s_0055d8c8[2];                                /* 0x0055d8c8 */
    unsigned char d_0055d8ca[174];                     /* 0x0055d8ca */
    ZresOverlaySourceTable gZresOverlaySources;        /* 0x0055d978 */
    unsigned char d_0055da28[72];                      /* 0x0055da28 */
    char sOutOfSyncMapData[24];                        /* 0x0055da70 */
    unsigned char pad_0055da88[4];                     /* 0x0055da88 */
    char sCZaxisBmxSrcComZgeNgIFNgIFzresC[44];         /* 0x0055da8c */
    unsigned char pad_0055dab8[8];                     /* 0x0055dab8 */
    char sMemoryOverflowInZresIncreaseFIXEDDEFSSI[56]; /* 0x0055dac0 */
    unsigned char pad_0055daf8[12];                    /* 0x0055daf8 */
    char sOutOfSyncPlayerData[24];                     /* 0x0055db04 */
    unsigned char pad_0055db1c[4];                     /* 0x0055db1c */
    char sInvalidPlayer[16];                           /* 0x0055db20 */
    unsigned char pad_0055db30[4];                     /* 0x0055db30 */
    char sBikeNumberOutOfRange[28];                    /* 0x0055db34 */
    unsigned char pad_0055db50[4];                     /* 0x0055db50 */
    char sAssetsMaterialZAL[20];                       /* 0x0055db54 */
    unsigned char pad_0055db68[4];                     /* 0x0055db68 */
    char sAssetsReplayZAL[20];                         /* 0x0055db6c */
    unsigned char pad_0055db80[4];                     /* 0x0055db80 */
    char sAssetsTablesZAL[20];                         /* 0x0055db84 */
    unsigned char pad_0055db98[4];                     /* 0x0055db98 */
    char sCouldNotOpenZAL[20];                         /* 0x0055db9c */
    unsigned char pad_0055dbb0[4];                     /* 0x0055dbb0 */
    char sZRESFailedToOpenS[28];                       /* 0x0055dbb4 */
    unsigned char pad_0055dbd0[4];                     /* 0x0055dbd0 */
    char sAssetsMiscDataZAL[20];                       /* 0x0055dbd4 */
    unsigned char pad_0055dbe8[4];                     /* 0x0055dbe8 */
    char sERRORLoadingOverlayMaterial[32];             /* 0x0055dbec */
    unsigned char pad_0055dc0c[8];                     /* 0x0055dc0c */
    char sOverlayD[12];                                /* 0x0055dc14 */
    char sZresAddMaterialToLoadListNotFound[40];       /* 0x0055dc20 */
    unsigned char pad_0055dc48[8];                     /* 0x0055dc48 */
    ZrollerNameTable gZrollerRiderNames;               /* 0x0055dc50 */
    void *p_0055dc54;                                  /* 0x0055dc54 */
    void *p_0055dc58;                                  /* 0x0055dc58 */
    void *p_0055dc5c;                                  /* 0x0055dc5c */
    void *p_0055dc60;                                  /* 0x0055dc60 */
    void *p_0055dc64;                                  /* 0x0055dc64 */
    void *p_0055dc68;                                  /* 0x0055dc68 */
    void *p_0055dc6c;                                  /* 0x0055dc6c */
    void *p_0055dc70;                                  /* 0x0055dc70 */
    void *p_0055dc74;                                  /* 0x0055dc74 */
    void *p_0055dc78;                                  /* 0x0055dc78 */
    void *p_0055dc7c;                                  /* 0x0055dc7c */
    ZrollerNameTable gZrollerBikeNames;                /* 0x0055dc80 */
    void *p_0055dc84;                                  /* 0x0055dc84 */
    void *p_0055dc88;                                  /* 0x0055dc88 */
    void *p_0055dc8c;                                  /* 0x0055dc8c */
    void *p_0055dc90;                                  /* 0x0055dc90 */
    void *p_0055dc94;                                  /* 0x0055dc94 */
    void *p_0055dc98;                                  /* 0x0055dc98 */
    void *p_0055dc9c;                                  /* 0x0055dc9c */
    void *p_0055dca0;                                  /* 0x0055dca0 */
    void *p_0055dca4;                                  /* 0x0055dca4 */
    void *p_0055dca8;                                  /* 0x0055dca8 */
    void *p_0055dcac;                                  /* 0x0055dcac */
    void *p_0055dcb0;                                  /* 0x0055dcb0 */
    void *p_0055dcb4;                                  /* 0x0055dcb4 */
    void *p_0055dcb8;                                  /* 0x0055dcb8 */
    void *p_0055dcbc;                                  /* 0x0055dcbc */
    void *p_0055dcc0;                                  /* 0x0055dcc0 */
    void *p_0055dcc4;                                  /* 0x0055dcc4 */
    void *p_0055dcc8;                                  /* 0x0055dcc8 */
    void *p_0055dccc;                                  /* 0x0055dccc */
    void *p_0055dcd0;                                  /* 0x0055dcd0 */
    void *p_0055dcd4;                                  /* 0x0055dcd4 */
    void *p_0055dcd8;                                  /* 0x0055dcd8 */
    void *p_0055dcdc;                                  /* 0x0055dcdc */
    void *p_0055dce0;                                  /* 0x0055dce0 */
    void *p_0055dce4;                                  /* 0x0055dce4 */
    void *p_0055dce8;                                  /* 0x0055dce8 */
    void *p_0055dcec;                                  /* 0x0055dcec */
    void *p_0055dcf0;                                  /* 0x0055dcf0 */
    void *p_0055dcf4;                                  /* 0x0055dcf4 */
    void *p_0055dcf8;                                  /* 0x0055dcf8 */
    void *p_0055dcfc;                                  /* 0x0055dcfc */
    void *p_0055dd00;                                  /* 0x0055dd00 */
    void *p_0055dd04;                                  /* 0x0055dd04 */
    void *p_0055dd08;                                  /* 0x0055dd08 */
    void *p_0055dd0c;                                  /* 0x0055dd0c */
    void *p_0055dd10;                                  /* 0x0055dd10 */
    void *p_0055dd14;                                  /* 0x0055dd14 */
    void *p_0055dd18;                                  /* 0x0055dd18 */
    void *p_0055dd1c;                                  /* 0x0055dd1c */
    void *p_0055dd20;                                  /* 0x0055dd20 */
    void *p_0055dd24;                                  /* 0x0055dd24 */
    void *p_0055dd28;                                  /* 0x0055dd28 */
    void *p_0055dd2c;                                  /* 0x0055dd2c */
    void *p_0055dd30;                                  /* 0x0055dd30 */
    void *p_0055dd34;                                  /* 0x0055dd34 */
    void *p_0055dd38;                                  /* 0x0055dd38 */
    void *p_0055dd3c;                                  /* 0x0055dd3c */
    void *p_0055dd40;                                  /* 0x0055dd40 */
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
    char s_0055e5b4[8];                                /* 0x0055e5b4 */
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
    uint16_t gSemitones[73];       /* 0x0055e820; the type is local to bmx/pc/zge/sound/winchan.c */
    unsigned char pad_0055e8b2[2]; /* 0x0055e8b2 */
    uint16_t gCents[100];          /* 0x0055e8b4; the type is local to bmx/pc/zge/sound/winchan.c */
    float gGainCurve[128];         /* 0x0055e97c; the type is local to bmx/pc/zge/sound/winchan.c */
    uint32_t gEnvelopeMs[64];      /* 0x0055eb7c */
    int16_t gDsVolume[128];        /* 0x0055ec7c; the type is local to bmx/pc/zge/sound/winstrm.c */
    uint32_t d_0055ed7c[71];       /* 0x0055ed7c */
    char sInitHWnd[12];            /* 0x0055ee98 */
    unsigned char pad_0055eea4[4]; /* 0x0055eea4 */
    char sCZaxisBmxSrcPcZgeSoundWinChanC[40];          /* 0x0055eea8 */
    unsigned char pad_0055eed0[8];                     /* 0x0055eed0 */
    char sDirectSoundCreate[20];                       /* 0x0055eed8 */
    unsigned char pad_0055eeec[4];                     /* 0x0055eeec */
    char sDSOUNDDLL[12];                               /* 0x0055eef0 */
    unsigned char pad_0055eefc[4];                     /* 0x0055eefc */
    char sFzsndcInitialized[20];                       /* 0x0055ef00 */
    unsigned char pad_0055ef14[4];                     /* 0x0055ef14 */
    char sIFzsndcNextChannelHandleNum[32];             /* 0x0055ef18 */
    unsigned char pad_0055ef38[8];                     /* 0x0055ef38 */
    char sSample[8];                                   /* 0x0055ef40 */
    char sFzsndcHandleHChanChannelNumKzsndNumChan[52]; /* 0x0055ef48 */
    unsigned char pad_0055ef7c[8];                     /* 0x0055ef7c */
    char sFzsndcHandleHChanAllocated[32];              /* 0x0055ef84 */
    unsigned char pad_0055efa4[4];                     /* 0x0055efa4 */
    char sHChanKzsndNumChannelHandles[32];             /* 0x0055efa8 */
    unsigned char pad_0055efc8[8];                     /* 0x0055efc8 */
    char sFileNameTooLong[20];                         /* 0x0055efd0 */
    unsigned char pad_0055efe4[4];                     /* 0x0055efe4 */
    char sCZaxisBmxSrcPcZgeSoundWinLoadC[40];          /* 0x0055efe8 */
    unsigned char pad_0055f010[8];                     /* 0x0055f010 */
    char sInvalidSampleFileNumber[28];                 /* 0x0055f018 */
    unsigned char pad_0055f034[4];                     /* 0x0055f034 */
    char sInvalidAudioAssetFilePlatform[36];           /* 0x0055f038 */
    unsigned char pad_0055f05c[4];                     /* 0x0055f05c */
    char sWIN[8];                                      /* 0x0055f060 */
    char sInvalidAudioAssetFileHeader[32];             /* 0x0055f068 */
    unsigned char pad_0055f088[8];                     /* 0x0055f088 */
    char sZSND[8];                                     /* 0x0055f090 */
    char sTooManyAudioAssetFiles[28];                  /* 0x0055f098 */
    unsigned char pad_0055f0b4[4];                     /* 0x0055f0b4 */
    char sFileNotLoaded[16];                           /* 0x0055f0b8 */
    unsigned char pad_0055f0c8[4];                     /* 0x0055f0c8 */
    char sInvalidFileHandle[20];                       /* 0x0055f0cc */
    unsigned char pad_0055f0e0[4];                     /* 0x0055f0e0 */
    char sInvalidVoiceNumber[24];                      /* 0x0055f0e4 */
    unsigned char pad_0055f0fc[4];                     /* 0x0055f0fc */
    char sSampleNumberOutOfRange[28];                  /* 0x0055f100 */
    unsigned char pad_0055f11c[4];                     /* 0x0055f11c */
    char sSampleFileNumberOutOfRange[32];              /* 0x0055f120 */
    unsigned char pad_0055f140[8];                     /* 0x0055f140 */
    char sPhraseNumberOutOfRange[28];                  /* 0x0055f148 */
    unsigned char pad_0055f164[4];                     /* 0x0055f164 */
    char sTrackDefNumberOutOfRange[32];                /* 0x0055f168 */
    unsigned char pad_0055f188[4];                     /* 0x0055f188 */
    char sKeymapNumberOutOfRange[28];                  /* 0x0055f18c */
    unsigned char pad_0055f1a8[4];                     /* 0x0055f1a8 */
    int32_t gAdpcmIndex[16];          /* 0x0055f1ac; the type is local to bmx/pc/zge/sound/winstrm.c */
    uint16_t gAdpcmStep[89];          /* 0x0055f1ec; the type is local to bmx/pc/zge/sound/winstrm.c */
    unsigned char pad_0055f29e[50];   /* 0x0055f29e */
    char sFzsndStreamInitialized[24]; /* 0x0055f2d0 */
    unsigned char pad_0055f2e8[4];    /* 0x0055f2e8 */
    char sCZaxisBmxSrcPcZgeSoundWinStrmC[40]; /* 0x0055f2ec */
    unsigned char pad_0055f314[8];            /* 0x0055f314 */
    char s_0055f31c[24];                      /* 0x0055f31c */
    unsigned char pad_0055f334[4];            /* 0x0055f334 */
    char sHResultDSOK[20];                    /* 0x0055f338 */
    char sDwLen20[12];                        /* 0x0055f34c */
    unsigned char pad_0055f358[4];            /* 0x0055f358 */
    float gMotionTimeScale;                   /* 0x0055f35c */
    int32_t gZactorMax;                       /* 0x0055f360 */
    uint16_t gMotionMirrorMap[19];            /* 0x0055f364; the type is local to bmx/com/zge/zactor.c */
    unsigned char pad_0055f38a[2];            /* 0x0055f38a */
    uint32_t gMotionLoadedBank;               /* 0x0055f38c */
    int8_t gSkeletonChain[1];                 /* 0x0055f390; the type is local to bmx/com/zge/zactor.c */
    unsigned char d_0055f391[303];            /* 0x0055f391 */
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
    uint32_t gZsndTicksPerSec;            /* 0x0055ffc0, also gWinstrmTicksPerSec, gZsndTickRate */
    char s_0055ffc4[20];                  /* 0x0055ffc4 */
    char sCNgEngineZgeZdebugC[28];        /* 0x0055ffd8 */
    char s104f104f104f[28];               /* 0x0055fff4 */
    char sS104f104f104f[28];              /* 0x00560010 */
    unsigned char pad_0056002c[4];        /* 0x0056002c */
    void *gZdebugStart;                   /* 0x00560030; the type is local to zge/zdebug.c */
    void *p_00560034;                     /* 0x00560034 */
    void *gZProfilePrintAt;               /* 0x00560038; the type is local to zge/zprofile.c */
    void *p_0056003c;                     /* 0x0056003c */
    void *p_00560040;                     /* 0x00560040 */
    void *p_00560044;                     /* 0x00560044 */
    void *p_00560048;                     /* 0x00560048 */
    void *p_0056004c;                     /* 0x0056004c */
    void *p_00560050;                     /* 0x00560050 */
    void *p_00560054;                     /* 0x00560054 */
    void *p_00560058;                     /* 0x00560058 */
    void *p_0056005c;                     /* 0x0056005c */
    void *p_00560060;                     /* 0x00560060 */
    ZrwDbgLineFn gZrwDbgLineHook;         /* 0x00560064 */
    ZrwDbgTriangleFn gZrwDbgTriangleHook; /* 0x00560068 */
    ZrwDbgSphereFn gZrwDbgSphereHook;     /* 0x0056006c */
    void *p_00560070;                     /* 0x00560070 */
    ZDebugPrintFn gZdebugPrint;           /* 0x00560074, also gGuiDebugPrint */
    ZDebugIndentFn gZdebugIndent;         /* 0x00560078 */
    void *p_0056007c;                     /* 0x0056007c */
    int gLineFresh;                       /* 0x00560080 */
    int gAxesFresh;                       /* 0x00560084 */
    int gTriangleFresh;                   /* 0x00560088 */
    int gBoxFresh;                        /* 0x0056008c */
    uint8_t gBoxEdges[24];                /* 0x00560090; the type is local to zge/com/zglrw/zrwdbg.c */
    unsigned char d_005600a8[1];          /* 0x005600a8 */
    char sVSSaved02d[15];                 /* 0x005600a9 */
    char sFPS02d[12];                     /* 0x005600b8 */
    char sFPS02dNumTri05dProc05dVerts05dResAlloc0[76]; /* 0x005600c4 */
    char sZrwDbgUnkownGeometryType[32];                /* 0x00560110 */
    char sCannotCreateRasterChraset[32];               /* 0x00560130 */
    char sCNgEngineZgeComZglRwZrwDbgC[36];             /* 0x00560150 */
    char sZdebugPrintOutOfBufferSpacePrintingS[48];    /* 0x00560174 */
    char sA104f[12];                                   /* 0x005601a4 */
    char sS104f[12];                                   /* 0x005601b0 */
    char s_005601bc[24];                               /* 0x005601bc */
    char s_005601d4[28];                               /* 0x005601d4 */
    char s104f104f104f104f[32];                        /* 0x005601f0 */
    char sS104f104f104f104f[36];                       /* 0x00560210 */
    char sA104f104f104f[28];                           /* 0x00560234 */
    char sZrwDbgOutOfBufferSpace[32];                  /* 0x00560250 */
    char s_00560270[16];                               /* 0x00560270 */
    char sCNgEngineZgeZinputC[28];                     /* 0x00560280 */
    char sAssetDSizeD[20];                             /* 0x0056029c */
    char sNumGroupsDAssetsPerGroupD[36];               /* 0x005602b0 */
    char sFAILEDToLoadLibraryDirectory[36];            /* 0x005602d4 */
    char sCompressionInRefMode[24];                    /* 0x005602f8 */
    char sCNgEngineZgeZassetC[28];                     /* 0x00560310 */
    char sFAILEDCouldNotLoadHeader[36];                /* 0x0056032c */
    char sFAILEDCouldNotAlocateMemoryForZfileLibr[60]; /* 0x00560350 */
    char sOPENLIBRARY[16];                             /* 0x0056038c */
    char sCLOSELIBRARY[16];                            /* 0x0056039c */
    char sFailedToFindAssetGroupDAssetNumD[44];        /* 0x005603ac */
    char sNULL[8];                                     /* 0x005603d8 */
    char sNotFoundS[16];                               /* 0x005603e0 */
    char sERRORSizeMismatchInDecompressedData[44];     /* 0x005603f0 */
    char s_0056041c[44];                               /* 0x0056041c */
    char sUnknownDecompressionMethod[32];              /* 0x00560448 */
    char s_00560468[16];                               /* 0x00560468 */
    char sFAILEDCouldNotLoadAsset[32];                 /* 0x00560478 */
    float d_00560498[32];                              /* 0x00560498 */
    ZObjCameraVtable gZobjCameraVtable;                /* 0x00560518 */
    unsigned char pad_00560528[24];                    /* 0x00560528 */
    ZVec3 gZobjCameraHome;                             /* 0x00560540 */
    unsigned char pad_0056054c[4];                     /* 0x0056054c */
    char sCNgEngineZgeZgeModsZobjCameraC[40];          /* 0x00560550 */
    char sUnknownType[16];                             /* 0x00560578 */
    char sError[8];                                    /* 0x00560588 */
    char sCNgEngineZgeZstringC[28];                    /* 0x00560590 */
    char sSTRS[8];                                     /* 0x005605ac */
    char sString0xXNotFoundInSetD[36];                 /* 0x005605b4 */
    char sNotFound[12];                                /* 0x005605d8 */
    char sGUIMAINBadMacroExpansion[32];                /* 0x005605e4 */
    char sCNgEngineZgeZquatC[24];                      /* 0x00560604 */
    char sDivisionBy0[16];                             /* 0x0056061c */
    intptr_t gZfileRamDisk;                            /* 0x0056062c */
    char sZfileRegisterErrorMessageS[36];              /* 0x00560630 */
    char sAnInvalidDeviceIdentifierWasGiven[40];       /* 0x00560654 */
    char sFailedToCreateFileHandleAllocator[40];       /* 0x0056067c */
    char sFileSystemShouldNotBeInitialized[40];        /* 0x005606a4 */
    char sThereAreNoDevicesConnectedToTheSystem[48];   /* 0x005606cc */
    char sErrorInitializingPlatformFileSystem[40];     /* 0x005606fc */
    char sCNgEngineZgeZfileC[24];                      /* 0x00560724 */
    char sErrorWhenTryingToCloseFileThroughDevice[48]; /* 0x0056073c */
    char s_0056076c[20];                               /* 0x0056076c */
    char sFileSystemHasNotBeenIntialized[40];          /* 0x00560780 */
    char sDeviceListMayBeCorrupt[28];                  /* 0x005607a8 */
    char sTheDefaultDeviceIsInvalid[32];               /* 0x005607c4 */
    char s_005607e4[20];                               /* 0x005607e4 */
    char sFailedToIdentifyDeviceFromFileName[44];      /* 0x005607f8 */
    char sDeviceNameIsNotValid[28];                    /* 0x00560824 */
    char sCouldNotDetermineDeviceForFileName[44];      /* 0x00560840 */
    char sInvalidParamters[20];                        /* 0x0056086c */
    char sAnInvalidFileNameWasGiven[32];               /* 0x00560880 */
    char sErrorOpeningFile[20];                        /* 0x005608a0 */
    char sFileSystemShouldBeInitialized[36];           /* 0x005608b4 */
    char sMemoryCorruptionWhenBuildingPathName[44];    /* 0x005608d8 */
    char sCNgEngineZgeZmemC[24];                       /* 0x00560904 */
    char sMemoryAllocationFailed[28];                  /* 0x0056091c */
    char sCanOnlyResizeLastAllocatedBlock[40];         /* 0x00560938 */
    char sCantResize[12];                              /* 0x00560960 */
    char sCannotResizeMemoryWhenAllocationsAreReD[68]; /* 0x0056096c */
    char sUnknownPool[16];                             /* 0x005609b0 */
    char sUseZmemFreeToResizeBlockTo0[36];             /* 0x005609c0 */
    char sCouldNotInitializeStaticMemoryManager[44];   /* 0x005609e4 */
    char sMemoryAllocationFailedOutOfRAM[40];          /* 0x00560a10 */
    char sFreeListPoolHasBeenExhausted[36];            /* 0x00560a38 */
    char sAnInvalidFreeListWasDestroyed[36];           /* 0x00560a5c */
    char sFreeListShouldBeEmptyButFreeBlocksStill[56]; /* 0x00560a80 */
    char sFreeListIsCorruptHeadOfUsedBlockListIsN[68]; /* 0x00560ab8 */
    char sInvalidBlockReleasedFromFreeList[40];        /* 0x00560afc */
    char sMemoryPoolSystemHasBeenPreviouslyInitia[52]; /* 0x00560b24 */
    char sMemoryPoolSystemHasBeenPreviouslyShutdo[52]; /* 0x00560b58 */
    char sAnInvalidMemoryPoolWasDestroyed[40];         /* 0x00560b8c */
    char sMemoryPoolChacheHasBeenBeenExhausted[44];    /* 0x00560bb4 */
    char sMemoryBeingReleasedIsNotFromThisPool[48];    /* 0x00560be0 */
    char sZmemPoolFreeMemoryHasBeenPreviouslyFree[52]; /* 0x00560c10 */
    char sZpropPropDefHasNoGeometry[36];               /* 0x00560c44 */
    char sZpropPropDefHeaderSizeInvalid[40];           /* 0x00560c68 */
    char sZpropPropDefHasNoData[32];                   /* 0x00560c90 */
    char sCNgEngineZgeZgeModsZpropC[32];               /* 0x00560cb0 */
    char sZpropUnableToCreateAnObjBasedOnThePropD[60]; /* 0x00560cd0 */
    char sZpropNoPropDefCorrespondsToTheProp[44];      /* 0x00560d0c */
    char sZpropPropHeaderSizeInvalid[36];              /* 0x00560d38 */
    char sZpropPropHasNoData[28];                      /* 0x00560d5c */
    char sZObjUsedD[16];                               /* 0x00560d78 */
    char sCanTInitializeWith0[24];                     /* 0x00560d88 */
    char sCNgEngineZgeZobjC[24];                       /* 0x00560da0 */
    char sZOBJErrorFailedToAllocateObj[40];            /* 0x00560db8 */
    char sZOBJObjectIsAlreadyAttachedToAnother[48];    /* 0x00560de0 */
    char sErrorInHierarchy[20];                        /* 0x00560e10 */
    char sNONAMEKzobjTypeD[24];                        /* 0x00560e24 */
    char s_00560e3c[4];                                /* 0x00560e3c */
    char sZOBJMemAllocatedDObjectsInUseD[44];          /* 0x00560e40 */
    unsigned char pad_00560e6c[4];                     /* 0x00560e6c */
    int gZrwMdlPluginOffset;                           /* 0x00560e70 */
    uint32_t d_00560e74;                               /* 0x00560e74 */
    void *gZglInitModel;                     /* 0x00560e78; the type is local to zge/com/zglrw/zrwzgl.c */
    void *gZglExitModel;                     /* 0x00560e7c; the type is local to zge/com/zglrw/zrwzgl.c */
    void *p_00560e80;                        /* 0x00560e80 */
    void *gZrwVtLoadModel;                   /* 0x00560e84; the type is local to zge/com/zglrw/zrwmat.c */
    void *p_00560e88;                        /* 0x00560e88 */
    void *gZrwVtLoadChunk;                   /* 0x00560e8c; the type is local to zge/com/zglrw/zrwmat.c */
    void *gZrwVtInstance;                    /* 0x00560e90; the type is local to zge/com/zglrw/zrwmat.c */
    ZrwModelCall gZrwVtDestroy;              /* 0x00560e94, also gZobjModelFree */
    ZrwModelCall gZrwVtUpdate;               /* 0x00560e98 */
    ZrwModelCall2 gZrwVtSetPos;              /* 0x00560e9c */
    ZrwModelCall2 gZrwVtSetRot;              /* 0x00560ea0 */
    ZrwModelCall2 gZrwVtSetScale;            /* 0x00560ea4 */
    void *gZrwVtPartCount;                   /* 0x00560ea8; the type is local to zge/com/zglrw/zrwmat.c */
    void *gZrwVtFindNode;                    /* 0x00560eac; the type is local to zge/com/zglrw/zrwmat.c */
    void *gZrwVtNodeToWorld;                 /* 0x00560eb0; the type is local to zge/com/zglrw/zrwmat.c */
    void *gZrwVtNodeVector;                  /* 0x00560eb4; the type is local to zge/com/zglrw/zrwmat.c */
    void *gZrwVtNodeMatrix;                  /* 0x00560eb8; the type is local to zge/com/zglrw/zrwmat.c */
    void *p_00560ebc;                        /* 0x00560ebc */
    ZrwModelLookup gZrwVtKey;                /* 0x00560ec0 */
    ZrwModelLookup gZrwVtEntry;              /* 0x00560ec4 */
    void *gZrwVtNodeHashes;                  /* 0x00560ec8; the type is local to zge/com/zglrw/zrwmat.c */
    void *gZrwVtSkeleton;                    /* 0x00560ecc; the type is local to zge/com/zglrw/zrwmat.c */
    void *p_00560ed0;                        /* 0x00560ed0 */
    void *p_00560ed4;                        /* 0x00560ed4 */
    ZrwModelFlagsFn gZrwVtSetFlags;          /* 0x00560ed8, also gZobjSetFlags */
    void *gZrwVtOwner;                       /* 0x00560edc; the type is local to zge/com/zglrw/zrwmat.c */
    ZbspObjectFrameFn gZbspObjectFrameFn;    /* 0x00560ee0 */
    ZrwModelCall gZrwVtClump;                /* 0x00560ee4 */
    void *gFxFindTexture;                    /* 0x00560ee8; the type is local to bmx/com/zge/ngif/ngiffx.c */
    ZrwModelCall gZrwVtMatCount;             /* 0x00560eec */
    void *p_00560ef0;                        /* 0x00560ef0 */
    ZobjColorFn gZobjSetColor;               /* 0x00560ef4 */
    void *p_00560ef8;                        /* 0x00560ef8 */
    void *p_00560efc;                        /* 0x00560efc */
    void *p_00560f00;                        /* 0x00560f00 */
    void *p_00560f04;                        /* 0x00560f04 */
    void *p_00560f08;                        /* 0x00560f08 */
    void *p_00560f0c;                        /* 0x00560f0c */
    void *p_00560f10;                        /* 0x00560f10 */
    void *p_00560f14;                        /* 0x00560f14 */
    void *p_00560f18;                        /* 0x00560f18 */
    char sCNgEngineZgeComZglRwZrwModelC[40]; /* 0x00560f1c */
    char sBodyPartNotMatchedHashX[32];       /* 0x00560f44 */
    char sPart0xXNotFoundInModelS[36];       /* 0x00560f64 */
    char sMaterialIndexOutOfRange[28];       /* 0x00560f88 */
    unsigned char pad_00560fa4[4];           /* 0x00560fa4 */
    uint32_t gZrwOpsSkinned;                 /* 0x00560fa8 */
    void *p_00560fac;                        /* 0x00560fac */
    void *p_00560fb0;                        /* 0x00560fb0 */
    void *p_00560fb4;                        /* 0x00560fb4 */
    void *p_00560fb8;                        /* 0x00560fb8 */
    void *p_00560fbc;                        /* 0x00560fbc */
    void *p_00560fc0;                        /* 0x00560fc0 */
    void *p_00560fc4;                        /* 0x00560fc4 */
    void *p_00560fc8;                        /* 0x00560fc8 */
    unsigned char pad_00560fcc[4];           /* 0x00560fcc */
    uint32_t gZrwOpsSimple;                  /* 0x00560fd0 */
    void *p_00560fd4;                        /* 0x00560fd4 */
    void *p_00560fd8;                        /* 0x00560fd8 */
    void *p_00560fdc;                        /* 0x00560fdc */
    void *p_00560fe0;                        /* 0x00560fe0 */
    void *p_00560fe4;                        /* 0x00560fe4 */
    void *p_00560fe8;                        /* 0x00560fe8 */
    void *p_00560fec;                        /* 0x00560fec */
    void *p_00560ff0;                        /* 0x00560ff0 */
    char sCNgEngineZgeZgeModsZobjModelC[36]; /* 0x00560ff4 */
    void *gZglInitMaterial;                  /* 0x00561018; the type is local to zge/com/zglrw/zrwzgl.c */
    void *gZglExitMaterial;                  /* 0x0056101c; the type is local to zge/com/zglrw/zrwzgl.c */
    void *p_00561020;                        /* 0x00561020 */
    ZrollerFreeHandleFn gZrollerFreeHandle;  /* 0x00561024, also gGuiFreeSpriteSheet, gRwTextureFree,
                                                gZrwMaterialRelease, gZrwWorldMatRelease */
    void *p_00561028;                        /* 0x00561028 */
    void *gFxSetRaster;                      /* 0x0056102c; the type is local to bmx/com/zge/ngif/ngiffx.c */
    void *p_00561030;                        /* 0x00561030 */
    ZrwMaterialTextureFn
        gZrwMaterialTexture;            /* 0x00561034, also gFxGetRaster, gNgifGetTexture, gZresGetTexture */
    ZbspMaterialFn gZbspMaterialFn;     /* 0x00561038 */
    ZRwTextureCloneFn gRwTextureClone;  /* 0x0056103c */
    void *p_00561040;                   /* 0x00561040 */
    void *gZrwWorldSetHook;             /* 0x00561044; the type is local to zge/com/zglrw/zrwworld.c */
    ZresLoadTextureFn gZresLoadTexture; /* 0x00561048, also gRwTextureLoad, gZrwShadowLoad */
    void *gFxTextureByHash;             /* 0x0056104c; the type is local to bmx/com/zge/ngif/ngiffx.c */
    void *p_00561050;                   /* 0x00561050 */
    char sRwMatProblemLoadingImage[32]; /* 0x00561054 */
    char sMADFMismatch[16];             /* 0x00561074 */
    char sDataTooBig[16];               /* 0x00561084 */
    char sCNgEngineZgeComZglRwZrwMatC[36];              /* 0x00561094 */
    char sNoTextureHasBeenAddedForThisRenderPass[48];   /* 0x005610b8 */
    char sTooManyTexturesAdded[24];                     /* 0x005610e8 */
    char sPMatPRwMatNotSet[24];                         /* 0x00561100 */
    char sZrwMatCreateFromImageErrorLoadingS[44];       /* 0x00561118 */
    char sZrwMatCreateProceduralErrorCreatingS[44];     /* 0x00561144 */
    char sZrwMatCreatePaletteFromMemoryError[40];       /* 0x00561170 */
    char sOutOfRange[16];                               /* 0x00561198 */
    char sNotLoadedMADF[16];                            /* 0x005611a8 */
    char sZGLMATObjectsInUseD[28];                      /* 0x005611b8 */
    void *p_005611d4;                                   /* 0x005611d4 */
    const char *gErrorCaption;                          /* 0x005611d8 */
    void *p_005611dc;                                   /* 0x005611dc */
    char sRenderWareTmApplicationWarning[36];           /* 0x005611e0 */
    char sRenderWareTmApplicationError[36];             /* 0x00561204 */
    char sRwAppClass[12];                               /* 0x00561228 */
    char sASSERTFuncSFileSLineDWhySDoYouWishToDeb[148]; /* 0x00561234 */
    char sASSERTFileSLineDWhySDoYouWishToDebugThe[140]; /* 0x005612c8 */
    char sLuXLuXLuS[20];                                /* 0x00561354 */
    char sFullscreen[16];                               /* 0x00561368 */
    char sSW[4];                                        /* 0x00561378 */
    char sRenderWareApplication[24];                    /* 0x0056137c */
    char sARenderWareTmApplicationIsAlreadyRunnin[52];  /* 0x00561394 */
    char sDoYouWantToQuitTheGame[32];                   /* 0x005613c8 */
    char sPleaseInsertGameCD[24];                       /* 0x005613e8 */
    char sRb[4];                                        /* 0x00561400 */
    char sBinMovieAcclaimAvi[24];                       /* 0x00561404 */
    char sSRCDISK[8];                                   /* 0x0056141c */
    char sSoftwareAcclaimEntertainmentDaveMirraFr[68];  /* 0x00561424 */
    void *gZrwGraphicLibraryInitialize;                 /* 0x00561468; the type is local to zge/zge.c */
    void *p_0056146c;                                   /* 0x0056146c */
    ZrollerRegisterFn gZrollerRegister; /* 0x00561470, also gNgifRegister, gTimerRegister, gZrwDbgRegister,
                                           gZrwModelRegister */
    ZrollerUnregisterFn
        gZrollerUnregister;   /* 0x00561474, also gNgifUnregister, gZrwDbgUnregister, gZrwModelUnregister */
    void *p_00561478;         /* 0x00561478 */
    void *gZdebugFailHandler; /* 0x0056147c; the type is local to zge/zdebug.c */
    ZrwTimeFn gTimeGetTime;   /* 0x00561480, also gFxTime, gZrwTimeGetTime */
    void *gZrwWorldSetBounds; /* 0x00561484, also gFxSetParam, gZrwSetBounds; the type is local to
                                 zge/com/zglrw/zrwworld.c */
    ZsystemGetObjectFn gZsystemGetObject; /* 0x00561488, also gNgQuery, gNgifQuery, gZpadQuery, gZrollerQuery,
                                             gZscnptsQuery */
    void *gZrwDbgFrameStart;              /* 0x0056148c; the type is local to zge/com/zglrw/zrwdbg.c */
    void *gZrwRenderStateHook;   /* 0x00561490, also gZglRenderStateHook, gZrwPrimStateHook; the type is local
                                    to zge/com/zglrw/zrwmodelrender.c */
    void *gZrwBeginHook;         /* 0x00561494; the type is local to zge/com/zglrw/zrwmodelrender.c */
    void *gZrwEndHook;           /* 0x00561498; the type is local to zge/com/zglrw/zrwmodelrender.c */
    ZMemAllocFn gRwMalloc;       /* 0x0056149c */
    ZMemFreeFn gRwFree;          /* 0x005614a0 */
    ZMemReallocFn gRwRealloc;    /* 0x005614a4 */
    uint32_t gZglMemoryFuncs;    /* 0x005614a8 */
    void *gZglAllocHookNop;      /* 0x005614ac; the type is local to zge/com/zglrw/zrwzgl.c */
    char sUnknownParameterD[24]; /* 0x005614b0 */
    char sCNgEngineZgeComZglRwZrwZglC[36];             /* 0x005614c8 */
    char sInvalidCameraSpecified[28];                  /* 0x005614ec */
    char sCannotCreateWorld[24];                       /* 0x00561508 */
    char sZrwMdlRegisterPluginFailed[28];              /* 0x00561520 */
    char sRwMatRegisterPluginFailed[28];               /* 0x0056153c */
    char sRpPVSPluginAttachFailed[28];                 /* 0x00561558 */
    char sRpLODAtomicPluginAttachFailed[32];           /* 0x00561574 */
    char sRpBonePluginAttachFailed[28];                /* 0x00561594 */
    char sRpAnimPluginAttachFailed[28];                /* 0x005615b0 */
    char sRpWorldPluginAttachFailed[28];               /* 0x005615cc */
    char sFailedInCallTo0xX[24];                       /* 0x005615e8 */
    char sThereIsAPendingRenderWareErrorRendering[68]; /* 0x00561600 */
    char sInvalidType[16];                             /* 0x00561644 */
    char sRenderStateStackIsFull[28];                  /* 0x00561654 */
    char sRenderStateStackIsEmpty[28];                 /* 0x00561670 */
    char sNotTested[12];                               /* 0x0056168c */
    char sMovieDonePlaying[24];                        /* 0x00561698 */
    char sMovieKeyAbort[20];                           /* 0x005616b0 */
    char sMovieAudioTick[20];                          /* 0x005616c4 */
    char sCannotInsertNULLValuesIntoAHeapList[44];     /* 0x005616d8 */
    char sCNgEngineZgeZListC[24];                      /* 0x00561704 */
    unsigned char pad_0056171c[4];                     /* 0x0056171c */
    void *gZglInitPrim;         /* 0x00561720; the type is local to zge/com/zglrw/zrwzgl.c */
    void *gZglExitPrim;         /* 0x00561724; the type is local to zge/com/zglrw/zrwzgl.c */
    ZrwDrawFn gZrwDrawPolys;    /* 0x00561728, also gFxDrawLines */
    ZrwDrawFn gZrwDrawTriangle; /* 0x0056172c, also gFxDrawTriangles, gZProfileDrawTriangles */
    ZrwDrawFn gZrwDrawSphere;   /* 0x00561730 */
    void *p_00561734;           /* 0x00561734 */
    ZrwDrawFn gNgifDrawQuads;   /* 0x00561738, also gFxDrawQuads, gNgDrawQuads, gZProfileDrawQuads */
    char sCNgEngineZgeComZglRwZrwPrimC[36];            /* 0x0056173c */
    char sZtaginfExtraNodesInTree[36];                 /* 0x00561760 */
    char sZtaginfTaggingInfoFaceStructureMismatch[48]; /* 0x00561784 */
    char sChunkSyncError[20];                          /* 0x005617b4 */
    char sTagDataNotFound[20];                         /* 0x005617c8 */
    char sCNgEngineZgeZgeModsZtaginfC[36];             /* 0x005617dc */
    char sFUCKEDTAGINFONORMAL[24];                     /* 0x00561800 */
    uint32_t gZrwModelColor1;                          /* 0x00561818 */
    uint32_t gZrwModelColor2;                          /* 0x0056181c */
    uint32_t d_00561820;                               /* 0x00561820 */
    char sCNgEngineZgeComZglRwZrwModelRenderC[44];     /* 0x00561824 */
    char sUnkown[8];                                   /* 0x00561850 */
    uint32_t gZobjLightOps;                            /* 0x00561858 */
    unsigned char pad_0056185c[4];                     /* 0x0056185c */
    void *p_00561860;                                  /* 0x00561860 */
    void *p_00561864;                                  /* 0x00561864 */
    unsigned char pad_00561868[20];                    /* 0x00561868 */
    char sCNgEngineZgeZgeModsZobjLightC[36];           /* 0x0056187c */
    void *p_005618a0;                                  /* 0x005618a0 */
    void *p_005618a4;                                  /* 0x005618a4 */
    unsigned char pad_005618a8[8];                     /* 0x005618a8 */
    void *p_005618b0;                                  /* 0x005618b0 */
    void *p_005618b4;                                  /* 0x005618b4 */
    void *p_005618b8;                                  /* 0x005618b8 */
    void *p_005618bc;                                  /* 0x005618bc */
    void *p_005618c0;                                  /* 0x005618c0 */
    void *p_005618c4;                                  /* 0x005618c4 */
    unsigned char pad_005618c8[8];                     /* 0x005618c8 */
    void *p_005618d0;                                  /* 0x005618d0 */
    unsigned char d_005618d4[4];                       /* 0x005618d4 */
    void *p_005618d8;                                  /* 0x005618d8 */
    void *gSceneModelHandlers;                         /* 0x005618dc; the type is local to zge/zscene.c */
    void *p_005618e0;                                  /* 0x005618e0 */
    void *gSceneAnimHandlers;                          /* 0x005618e4; the type is local to zge/zscene.c */
    unsigned char pad_005618e8[24];                    /* 0x005618e8 */
    uint32_t d_00561900;                               /* 0x00561900 */
    void *p_00561904;                                  /* 0x00561904 */
    uint32_t d_00561908;                               /* 0x00561908 */
    void *p_0056190c;                                  /* 0x0056190c */
    uint32_t d_00561910;                               /* 0x00561910 */
    void *p_00561914;                                  /* 0x00561914 */
    uint32_t d_00561918;                               /* 0x00561918 */
    void *p_0056191c;                                  /* 0x0056191c */
    uint32_t d_00561920;                               /* 0x00561920 */
    void *p_00561924;                                  /* 0x00561924 */
    char sMODEL[8];                                    /* 0x00561928 */
    char sSCENE[8];                                    /* 0x00561930 */
    char sZAL[4];                                      /* 0x00561938 */
    char sIFF[4];                                      /* 0x0056193c */
    char sNAD[4];                                      /* 0x00561940 */
    char sZMD[4];                                      /* 0x00561944 */
    char sERRORUnknownTokenS[32];                      /* 0x00561948 */
    char sERRORMissingData[28];                        /* 0x00561968 */
    char s_00561984[4];                                /* 0x00561984 */
    char sSUBTYPE[8];                                  /* 0x00561988 */
    char sTYPE[8];                                     /* 0x00561990 */
    char sFileNameIsTooLong[24];                       /* 0x00561998 */
    char sFILE[8];                                     /* 0x005619b0 */
    char sObjectNameIsTooLong[28];                     /* 0x005619b8 */
    char sCNgEngineZgeZSceneC[28];                     /* 0x005619d4 */
    char sNAME[8];                                     /* 0x005619f0 */
    char s_005619f8[4];                                /* 0x005619f8 */
    char sDatabaseNotFoundS[24];                       /* 0x005619fc */
    char sSCENECOMMON[16];                             /* 0x00561a14 */
    char sNoSceneIsOpen[20];                           /* 0x00561a24 */
    char sCouldnTOpenRamdiskFile[28];                  /* 0x00561a38 */
    char sSceneComponentFileNotFoundS[36];             /* 0x00561a54 */
    char sZrwWorldFailedToLoadScenePoint[40];          /* 0x00561a78 */
    char sZrwWorldFailedToLoadSceneTaggingInfo[48];    /* 0x00561aa0 */
    char sUnknownSceneEzsceneObjTypeD[32];             /* 0x00561ad0 */
    char sNULLAnimLibID[16];                           /* 0x00561af0 */
    char sUncoded[8];                                  /* 0x00561b00 */
    char sZSCENEWarningObjectWasNotInSceneTypeDId[72]; /* 0x00561b08 */
    char sAnimLibNotFoundS[24];                        /* 0x00561b50 */
    char sERRORZSCENELoadingDatabase[36];              /* 0x00561b68 */
    char sSceneTxt[12];                                /* 0x00561b8c */
    char sErrorLoadingS[20];                           /* 0x00561b98 */
    char sZSCENECouldNotLoadS[28];                     /* 0x00561bac */
    char sSceneNotFoundS[20];                          /* 0x00561bc8 */
    char sThereIsAlreadyASceneOpen[32];                /* 0x00561bdc */
    char sFailedToCreateObjectForS[32];                /* 0x00561bfc */
    char sModelNotFoundS[20];                          /* 0x00561c1c */
    char sUnknownModelExtensionS[28];                  /* 0x00561c30 */
    char sModelNotInSceneS[24];                        /* 0x00561c4c */
    char sAnimNotFoundS[20];                           /* 0x00561c64 */
    char sUnknownExtension[20];                        /* 0x00561c78 */
    char sAnimNotInSceneS[24];                         /* 0x00561c8c */
    char s_00561ca4[4];                                /* 0x00561ca4 */
    char s_00561ca8[4];                                /* 0x00561ca8 */
    char sWARNINGZSCENEMotion0xXNotFoundInAnimati[68]; /* 0x00561cac */
    char sTableNotBigEnoughTabSizeDNumMotionsD[48];    /* 0x00561cf0 */
    char sZscnptsPointDataSizeMismatch[36];            /* 0x00561d20 */
    char sZscnptsPointHasNoData[32];                   /* 0x00561d44 */
    char sCNgEngineZgeZgeModsZscnptsC[36];             /* 0x00561d64 */
    ZrwCamInitFn gZrwCamInit;                          /* 0x00561d88 */
    void *p_00561d8c;                                  /* 0x00561d8c */
    ZrwCamCreateFn gZrwCamCreate;                      /* 0x00561d90 */
    ZrwCamDestroyFn gZrwCamDestroy;                    /* 0x00561d94 */
    ZrwCamSetFovFn gZrwCamSetFov;                      /* 0x00561d98 */
    void *p_00561d9c;                                  /* 0x00561d9c */
    ZrwCamSetViewFn gZrwCamSetView;                    /* 0x00561da0 */
    void *p_00561da4;                                  /* 0x00561da4 */
    void *p_00561da8;                                  /* 0x00561da8 */
    void *p_00561dac;                                  /* 0x00561dac */
    void *gZrwDbgGetCamera;    /* 0x00561db0, also gFxGetCamera, gNgifGetCamera; the type is local to
                                  zge/com/zglrw/zrwdbg.c */
    void *gZrwDbgCameraObject; /* 0x00561db4, also gFxCameraObject; the type is local to
                                  zge/com/zglrw/zrwdbg.c */
    void *gNgifSetViewport;    /* 0x00561db8; the type is local to bmx/com/zge/ngif/ngifpsx.c */
    void *p_00561dbc;          /* 0x00561dbc */
    void *gNgifSetViewOffset;  /* 0x00561dc0; the type is local to bmx/com/zge/ngif/ngifpsx.c */
    char sErrorCreatingCameraZBuffer[32];           /* 0x00561dc4 */
    char sErrorCreatingCameraRaster[32];            /* 0x00561de4 */
    char sCNgEngineZgeComZglRwZrwCamC[36];          /* 0x00561e04 */
    char sCanTSetScreenResoltionIfCamerasExist[44]; /* 0x00561e28 */
    char sRwCamNoCamera[20];                        /* 0x00561e54 */
    uint8_t gZpadKeyMapFirst;                       /* 0x00561e68 */
    unsigned char d_00561e69[31];                   /* 0x00561e69 */
    uint8_t gZpadKeyMapOther;                       /* 0x00561e88 */
    unsigned char d_00561e89[31];                   /* 0x00561e89 */
    uint8_t gZpadDefaultButtonMap;                  /* 0x00561ea8 */
    unsigned char d_00561ea9[31];                   /* 0x00561ea9 */
    void *p_00561ec8;                               /* 0x00561ec8 */
    void *p_00561ecc;                               /* 0x00561ecc */
    unsigned char pad_00561ed0[8];                  /* 0x00561ed0 */
    void *p_00561ed8;                               /* 0x00561ed8 */
    uint32_t d_00561edc[7];                         /* 0x00561edc */
    void *p_00561ef8;                               /* 0x00561ef8 */
    void *p_00561efc;                               /* 0x00561efc */
    unsigned char pad_00561f00[4];                  /* 0x00561f00 */
    void *p_00561f04;                               /* 0x00561f04 */
    unsigned char pad_00561f08[8];                  /* 0x00561f08 */
    void *p_00561f10;                               /* 0x00561f10 */
    void *p_00561f14;                               /* 0x00561f14 */
    uint32_t d_00561f18[5];                         /* 0x00561f18 */
    const ZpadProfile *gZpadProfiles;               /* 0x00561f2c */
    void *p_00561f30;                               /* 0x00561f30 */
    unsigned char pad_00561f34[4];                  /* 0x00561f34 */
    char sMicrosoftSideWinderAutoDetect[36];        /* 0x00561f38 */
    char sMicrosoftSideWinderGamePad[32];           /* 0x00561f5c */
    char sGravisGamePadPro[20];                     /* 0x00561f7c */
    char sGravisGrIP[12];                           /* 0x00561f90 */
    char sDirectInputCreateFailed[28];              /* 0x00561f9c */
    char sCNgEngineZgePcPcinputC[32];               /* 0x00561fb8 */
    char sPCINPUTCouldNotCreateController[40];      /* 0x00561fd8 */
    char sCouldNotCreateAnInputDevice[36];          /* 0x00562000 */
    char sSetCooperativeLevelFailed[32];            /* 0x00562024 */
    char sSetDataFormatFailed[24];                  /* 0x00562044 */
    unsigned char pad_0056205c[4];                  /* 0x0056205c */
    void *gZglInitCol;                       /* 0x00562060; the type is local to zge/com/zglrw/zrwzgl.c */
    void *gZglExitCol;                       /* 0x00562064; the type is local to zge/com/zglrw/zrwzgl.c */
    void *gZrwColLineFn;                     /* 0x00562068; the type is local to zge/com/zglrw/zrwcol.c */
    void *gZrwColSphereFn;                   /* 0x0056206c; the type is local to zge/com/zglrw/zrwcol.c */
    char sCNgEngineZgeComZglRwZrwColC[36];   /* 0x00562070 */
    unsigned char pad_00562094[4];           /* 0x00562094 */
    uint32_t gZrwWorldEffectOps;             /* 0x00562098 */
    void *p_0056209c;                        /* 0x0056209c */
    void *p_005620a0;                        /* 0x005620a0 */
    void *p_005620a4;                        /* 0x005620a4 */
    void *p_005620a8;                        /* 0x005620a8 */
    void *p_005620ac;                        /* 0x005620ac */
    unsigned char pad_005620b0[12];          /* 0x005620b0 */
    char sCNgEngineZgeZgeModsZobjPEmitC[36]; /* 0x005620bc */
    ZrwWorldLoadFn gZrwWorldLoad;            /* 0x005620e0 */
    ZrwWorldCloseFn gZrwWorldClose;          /* 0x005620e4 */
    ZrwWorldFindFn gZrwWorldFind;            /* 0x005620e8 */
    void *gFxLoadTexture;                    /* 0x005620ec; the type is local to bmx/com/zge/ngif/ngiffx.c */
    void *p_005620f0;                        /* 0x005620f0 */
    void *p_005620f4;                        /* 0x005620f4 */
    void *p_005620f8;                        /* 0x005620f8 */
    void *p_005620fc;                        /* 0x005620fc */
    void *p_00562100;                        /* 0x00562100 */
    void *p_00562104;                        /* 0x00562104 */
    void *p_00562108;                        /* 0x00562108 */
    void *gZrwLightKinds[7];                 /* 0x0056210c; the type is local to zge/com/zglrw/zrwworld.c */
    uint32_t gZrwWorldPathArgs;              /* 0x00562128 */
    const char *gZrwPvsKeys;                 /* 0x0056212c */
    char gZrwPathKey[8];                     /* 0x00562130; the type is local to zge/com/zglrw/zrwfile.c */
    char sERROR[8];                          /* 0x00562138 */
    char sRpNALIGHTTYPE[16];                 /* 0x00562140 */
    char sRpLIGHTSPOTSOFT[16];               /* 0x00562150 */
    char sRpLIGHTSPOT[12];                   /* 0x00562160 */
    char sRpLIGHTPOINT[16];                  /* 0x0056216c */
    char sRpLIGHTAMBIENT[16];                /* 0x0056217c */
    char sRpLIGHTDIRECTIONAL[20];            /* 0x0056218c */
    char sNumberOfClumpsD[24];               /* 0x005621a0 */
    char sCLUMPSPropsEtc[24];                /* 0x005621b8 */
    char sNumberOfVerticesD[24];             /* 0x005621d0 */
    char sNumberOfPolygonsD[24];             /* 0x005621e8 */
    char sSTATICGEOM[16];                    /* 0x00562200 */
    char s3dS[12];                           /* 0x00562210 */
    char sMATERIALS[16];                     /* 0x0056221c */
    char sLIGHTS[12];                        /* 0x0056222c */
    char sSCENEINFOFORS[24];                 /* 0x00562238 */
    char s_00562250[64];                     /* 0x00562250 */
    char sConeAngleF[16];                    /* 0x00562290 */
    char sRadiusF[12];                       /* 0x005622a0 */
    char sLightWorld[12];                    /* 0x005622ac */
    char sLightAtomics[16];                  /* 0x005622b8 */
    char sRFGFBF[16];                        /* 0x005622c8 */
    char sLight3dS[16];                      /* 0x005622d8 */
    char sChunkOutOfSync[20];                /* 0x005622e8 */
    char sCNgEngineZgeComZglRwZrwWorldC[40]; /* 0x005622fc */
    char sCouldnTOpenS[20];                  /* 0x00562324 */
    char sZrwWorldHookRpMaterialsFailedToMatchRpM[56]; /* 0x00562338 */
    char sPercentCompleteF[24];                        /* 0x00562370 */
    char sThereIsAlreadyAWorldOpen[32];                /* 0x00562388 */
    char sZRWWORLDNoPVSData[24];                       /* 0x005623a8 */
    ZVec3 gZrwDefaultGravity;                          /* 0x005623c0 */
    unsigned char pad_005623cc[4];                     /* 0x005623cc */
    float gZrwDefaultBounce[2];    /* 0x005623d0; the type is local to zge/com/zglrw/zrwworld.c */
    void *gFxGravity;              /* 0x005623d8 */
    void *p_005623dc;              /* 0x005623dc */
    void *gFxFloorColl;            /* 0x005623e0 */
    void *p_005623e4;              /* 0x005623e4 */
    int gZrwMatPluginOffset;       /* 0x005623e8 */
    unsigned char pad_005623ec[4]; /* 0x005623ec */
    void *gZglInitTexture;         /* 0x005623f0; the type is local to zge/com/zglrw/zrwzgl.c */
    void *gZglExitTexture;         /* 0x005623f4; the type is local to zge/com/zglrw/zrwzgl.c */
    void *p_005623f8;              /* 0x005623f8 */
    void *gZrwTextureLoad;         /* 0x005623fc; the type is local to zge/com/zglrw/zrwmat.c */
    void *gZrwWorldTexDrop; /* 0x00562400, also gZrwTextureDrop, gZrwTextureRelease; the type is local to
                               zge/com/zglrw/zrwworld.c */
    ZresTextureSizeFn gZresTextureWidth;  /* 0x00562404, also gNgifTextureWidth */
    ZresTextureSizeFn gZresTextureHeight; /* 0x00562408, also gNgifTextureHeight */
    void *p_0056240c;                     /* 0x0056240c */
    void *gZrwWorldTexName;               /* 0x00562410; the type is local to zge/com/zglrw/zrwworld.c */
    ZrwTextureHashFn gZrwTextureHash;     /* 0x00562414 */
    void *p_00562418;                     /* 0x00562418 */
    void *p_0056241c;                     /* 0x0056241c */
    void *gZrwTextureRetain;              /* 0x00562420; the type is local to zge/com/zglrw/zrwmat.c */
    void *p_00562424;                     /* 0x00562424 */
    void *p_00562428;                     /* 0x00562428 */
    char sFormatSWidthDHeightDStrideDDepthD[56]; /* 0x0056242c */
    char sRwRASTERFORMATPAL4[20];                /* 0x00562464 */
    char sRwRASTERFORMATMIPMAP[24];              /* 0x00562478 */
    char sRwRASTERFORMATMASK[20];                /* 0x00562490 */
    char sUnknown[8];                            /* 0x005624a4 */
    char sRwRASTERFORMATPAL8[20];                /* 0x005624ac */
    char sRwRASTERFORMAT32[20];                  /* 0x005624c0 */
    char sRwRASTERFORMAT555[20];                 /* 0x005624d4 */
    char sRwRASTERFORMATPIXELFORMATMASK[32];     /* 0x005624e8 */
    char sRwRASTERFORMAT888[20];                 /* 0x00562508 */
    char sRwRASTERFORMAT24[20];                  /* 0x0056251c */
    char sRwRASTERFORMATLUM8[20];                /* 0x00562530 */
    char sRwRASTERFORMAT4444[20];                /* 0x00562544 */
    char sRwRASTERFORMAT565[20];                 /* 0x00562558 */
    char sRwRASTERFORMATDEFAULT[24];             /* 0x0056256c */
    char sRwRASTERFORMAT1555[20];                /* 0x00562584 */
    char sFailedToCreateTexture[28];             /* 0x00562598 */
    char kZrwExtBmp[4];                          /* 0x005625b4; the type is local to zge/com/zglrw/zrwtxr.c */
    char kZrwExtPng[4];                          /* 0x005625b8; the type is local to zge/com/zglrw/zrwtxr.c */
    char sBadEzglImageTypes[20];                 /* 0x005625bc */
    char sErrorInParameters[20];                 /* 0x005625d0 */
    char sCNgEngineZgeComZglRwZrwTxrC[36];       /* 0x005625e4 */
    char sZrwTxrLoadPalettedImageFromMemoryErrorI[56]; /* 0x00562608 */
    char sZrwTxrLoadPalettedImageFromMemoryError[44];  /* 0x00562640 */
    char sUnableToLockRaster[24];                      /* 0x0056266c */
    char sUnableToLockRasterPalette[32];               /* 0x00562684 */
    char sZrwTxrCreateProceduralTextureErrorInPar[52]; /* 0x005626a4 */
    char sZrwTxrCreateProceduralTextureError[40];      /* 0x005626d8 */
    char sZrwTxrErrorLoadingTextureS[36];              /* 0x00562700 */
    char sLoadBufferIsTooSmall[28];                    /* 0x00562724 */
    char sNameSWidthDHeightDBPPD[36];                  /* 0x00562740 */
    char kZrwFormatDds[4]; /* 0x00562764; the type is local to zge/com/zglrw/zrwtxr.c */
    char kZrwFormatPng[4]; /* 0x00562768; the type is local to zge/com/zglrw/zrwtxr.c */
    char kZrwFormatBmp[4]; /* 0x0056276c; the type is local to zge/com/zglrw/zrwtxr.c */
    char sTextureReferenceCountHasBecomeNegative[48]; /* 0x00562770 */
    char sTextureIsnTPalettedS[28];                   /* 0x005627a0 */
    char sUnknownMode[16];                            /* 0x005627bc */
    char sZGLTEXTUREObjectsInUseD[32];                /* 0x005627cc */
    char sLoadBufferHasAlreadyBeenPrepared[40];       /* 0x005627ec */
    char sLoadBufferHasAlreadyBeenDestroyed[40];      /* 0x00562814 */
    char sCannotAllocate[16];                         /* 0x0056283c */
    char sCNgEngineZgeZanimC[24];                     /* 0x0056284c */
    char sCanTAllocateNadname[24];                    /* 0x00562864 */
    char sDEFISizeMismatch[20];                       /* 0x0056287c */
    char sCanTAllocateNamelist[24];                   /* 0x00562890 */
    char sCanTAllocateNadRots[24];                    /* 0x005628a8 */
    char sCanTAllocateRotChannel[28];                 /* 0x005628c0 */
    char sCanTAllocateNadTrans[24];                   /* 0x005628dc */
    char sCanTAllocateTransChannel[32];               /* 0x005628f4 */
    char sHeyJackoFloatTimeIsTooBigResetIt[48];       /* 0x00562914 */
    char sNonNonblendedAnimation[28];                 /* 0x00562944 */
    char sEmptyStreamList[20];                        /* 0x00562960 */
    char sCNgEngineZgeZiffC[24];                      /* 0x00562974 */
    char sZIFFBadIFFFile[20];                         /* 0x0056298c */
    char sZIFFNonMatchingChunkIDCCCCCCCC[52];         /* 0x005629a0 */
    char sZIFFChunksStillOpen[24];                    /* 0x005629d4 */
    char sUnregisteredHeaderCCCC[32];                 /* 0x005629ec */
    char sCCCC[12];                                   /* 0x00562a0c */
    char sZIFFContentsFor0xX[28];                     /* 0x00562a18 */
    char sZIFFNotReRegisteringCCCC[40];               /* 0x00562a34 */
    char sZIFFChangingFunctionForCCCC[40];            /* 0x00562a5c */
    char sNotAtStartOfChunk[24];                      /* 0x00562a84 */
    char sZIFFZiffChunkReadDataClampingSize[40];      /* 0x00562a9c */
    char sPassedTheEndOfChunk[24];                    /* 0x00562ac4 */
    char sAllDataRead[16];                            /* 0x00562adc */
    char sTryingToRead0Bytes[24];                     /* 0x00562aec */
    char sDataLengthNotWordSizeForHeaderCCCC[48];     /* 0x00562b04 */
    char sSeekedPassedTheStart[24];                   /* 0x00562b34 */
    char sSeekedPassedTheEnd[24];                     /* 0x00562b4c */
    char sInvalidSeekMode[20];                        /* 0x00562b64 */
    char sNotAtTheStartOfTheChunk[32];                /* 0x00562b78 */
    char sNestedTooDeep[16];                          /* 0x00562b98 */
    char sTooManyCloses[16];                          /* 0x00562ba8 */
    char sShadowMap[12];                              /* 0x00562bb8 */
    char sAssetsShadowMapBmp[24];                     /* 0x00562bc4 */
    char sCNgEngineZgeComZglRwZrwShadowC[40];         /* 0x00562bdc */
    char s123456789012345678[20];                     /* 0x00562c04 */
    char sZCOMPRESUnsupportedCompressionMethod[44];   /* 0x00562c18 */
    char sFaultyVValueOnEOFD[28];                     /* 0x00562c44 */
    char sZCOMPRESDecompressionFailed[36];            /* 0x00562c60 */
    char sIllegalStartingByte[24];                    /* 0x00562c84 */
    char sZCOMPRESUnknownDecompressionMethod[40];     /* 0x00562c9c */
    float gZrwLodDefaults[4]; /* 0x00562cc4; the type is local to zge/com/zglrw/zrwmodelcreate.c */
    char sZrwModelSetRwInfoFailedToMatchRpMateria[52]; /* 0x00562cd4 */
    char sFDVD[16];                                    /* 0x00562d08 */
    char sRpGEOMETRYTRISTRIP[20];                      /* 0x00562d18 */
    char sRpGEOMETRYLIGHT[20];                         /* 0x00562d2c */
    char sRpGEOMETRYNORMALS[20];                       /* 0x00562d40 */
    char sRpGEOMETRYPRELIT[20];                        /* 0x00562d54 */
    char sRpGEOMETRYTEXTURED[20];                      /* 0x00562d68 */
    char sFlagsAtomic0[20];                            /* 0x00562d7c */
    char sNumVerticesD[20];                            /* 0x00562d90 */
    char sNumTrianglesD[20];                           /* 0x00562da4 */
    char s_00562db8[4];                                /* 0x00562db8 */
    char sNumAtomicsD[20];                             /* 0x00562dbc */
    char s0xX[8];                                      /* 0x00562dd0 */
    char s0xXS[12];                                    /* 0x00562dd8 */
    char sHIERARCHY[16];                               /* 0x00562de4 */
    char s_00562df4[16];                               /* 0x00562df4 */
    char sZrwModelCreateInstanceNotInstancingSkin[68]; /* 0x00562e04 */
    char sClumpFailedToLoad[24];                       /* 0x00562e48 */
    char s_00562e60[24];                               /* 0x00562e60 */
    char sCNgEngineZgeComZglRwZrwModelCreateC[44];     /* 0x00562e78 */
    char sIgnoringExtraLODInS[28];                     /* 0x00562ea4 */
    char sMissingModelDefBlock[24];                    /* 0x00562ec0 */
    char sLoadingModelS[20];                           /* 0x00562ed8 */
    char s_00562eec[52];                               /* 0x00562eec */
    char sCouldnTOpenMODL[20];                         /* 0x00562f20 */
    char sNotAMODL[12];                                /* 0x00562f34 */
    char sZGLMODELObjectsInUseD[32];                   /* 0x00562f40 */
    char sInfoForModelS[20];                           /* 0x00562f60 */
    char sCNgEngineZgeComZglRwZrwModelGeomC[44];       /* 0x00562f74 */
    char sCouldNotLockModelGeometryForS[40];           /* 0x00562fa0 */
    char sCouldNotUnlockModelGeometryForS[44];         /* 0x00562fc8 */
    unsigned char pad_00562ff4[4];                     /* 0x00562ff4 */
    void *gZglFileDeviceRw;                            /* 0x00562ff8 */
    void *p_00562ffc;                                  /* 0x00562ffc */
    void *p_00563000;                                  /* 0x00563000 */
    void *p_00563004;                                  /* 0x00563004 */
    void *p_00563008;                                  /* 0x00563008 */
    void *p_0056300c;                                  /* 0x0056300c */
    void *p_00563010;                                  /* 0x00563010 */
    void *p_00563014;                                  /* 0x00563014 */
    void *p_00563018;                                  /* 0x00563018 */
    void *p_0056301c;                                  /* 0x0056301c */
    void *p_00563020;                                  /* 0x00563020 */
    void *p_00563024;                                  /* 0x00563024 */
    void *p_00563028;                                  /* 0x00563028 */
    void *p_0056302c;                                  /* 0x0056302c */
    void *p_00563030;                                  /* 0x00563030 */
    unsigned char pad_00563034[4];                     /* 0x00563034 */
    void *gZrwOpenModes[4];                 /* 0x00563038; the type is local to zge/com/zglrw/zrwfile.c */
    char sRB[4];                            /* 0x00563048 */
    char sWb[4];                            /* 0x0056304c */
    char gZrwDeviceName[8];                 /* 0x00563050; the type is local to zge/com/zglrw/zrwfile.c */
    char sCNgEngineZgeComZglRwZrwFileC[36]; /* 0x00563058 */
    char sInvalidAttributesForRenderWareFileName[44];  /* 0x0056307c */
    char sInvalidFileNameGiven[24];                    /* 0x005630a8 */
    char sMustSpecifyAtLeastEitherAReturnValueOrA[60]; /* 0x005630c0 */
    char sFunctionNotPermittedOnThisDevice[40];        /* 0x005630fc */
    char sFunctionNotImplementedForThisDevice[44];     /* 0x00563124 */
    char sRwfopenFailed[20];                           /* 0x00563150 */
    char sInvalidParameters[20];                       /* 0x00563164 */
    char sRwfcloseFailed[20];                          /* 0x00563178 */
    char sRwfreadFailed[20];                           /* 0x0056318c */
    char sRwfwriteFailed[20];                          /* 0x005631a0 */
    char sRwfseekFailed[20];                           /* 0x005631b4 */
    char sInvalidPointerToFileSize[32];                /* 0x005631c8 */
    char sFileSystemHasAlreadyBeenIntialized[44];      /* 0x005631e8 */
    char sFileSystemHasAlreadyBeenShutDown[40];        /* 0x00563214 */
    unsigned char pad_0056323c[4];                     /* 0x0056323c */
    void *gZglFileDeviceRam;                           /* 0x00563240 */
    void *p_00563244;                                  /* 0x00563244 */
    void *p_00563248;                                  /* 0x00563248 */
    void *p_0056324c;                                  /* 0x0056324c */
    void *p_00563250;                                  /* 0x00563250 */
    void *p_00563254;                                  /* 0x00563254 */
    void *p_00563258;                                  /* 0x00563258 */
    void *p_0056325c;                                  /* 0x0056325c */
    void *p_00563260;                                  /* 0x00563260 */
    void *p_00563264;                                  /* 0x00563264 */
    void *p_00563268;                                  /* 0x00563268 */
    void *p_0056326c;                                  /* 0x0056326c */
    void *p_00563270;                                  /* 0x00563270 */
    void *p_00563274;                                  /* 0x00563274 */
    void *p_00563278;                                  /* 0x00563278 */
    unsigned char pad_0056327c[4];                     /* 0x0056327c */
    char sCNgEngineZgeZgeModsZramdiskC[36];            /* 0x00563280 */
    char sInvalidRAMDiskFileNameAttributes[40];        /* 0x005632a4 */
    char sInvalidFileNameCreatedForRAMDisk[40];        /* 0x005632cc */
    char sRd0010x10x[20];                              /* 0x005632f4 */
    char sFunctionNotSupportedOnThisDevice[40];        /* 0x00563308 */
    char sErrorAllocatingRAMDiskFileHandle[40];        /* 0x00563330 */
    char sInvalidFileNameGivenForRAMDisk[40];          /* 0x00563358 */
    char sInvalidFileName[20];                         /* 0x00563380 */
    char sInvalidRAMDiskFileHandle[32];                /* 0x00563394 */
    char sFileAccessViolation[24];                     /* 0x005633b4 */
    char sSeekPastEndOfFile[24];                       /* 0x005633cc */
    char sInvalidPointerForFileSize[32];               /* 0x005633e4 */
    unsigned char pad_00563404[4];                     /* 0x00563404 */
    uint32_t gZrwLightTypes[4];              /* 0x00563408; the type is local to zge/com/zglrw/zrwlight.c */
    uint32_t d_00563418[2];                  /* 0x00563418 */
    void *gZglInitLight;                     /* 0x00563420; the type is local to zge/com/zglrw/zrwzgl.c */
    void *gZglExitLight;                     /* 0x00563424; the type is local to zge/com/zglrw/zrwzgl.c */
    ZrwLightCreateFn gZrwLightCreate;        /* 0x00563428 */
    ZrwLightDestroyFn gZrwLightDestroy;      /* 0x0056342c */
    ZrwLightColorFn gZrwLightColor;          /* 0x00563430 */
    ZrwLightFlagsFn gZrwLightFlags;          /* 0x00563434 */
    ZrwLightParamFn gZrwLightSetParam;       /* 0x00563438 */
    ZrwLightDirectionFn gZrwLightDirection;  /* 0x0056343c */
    void *p_00563440;                        /* 0x00563440 */
    void *p_00563444;                        /* 0x00563444 */
    char sCNgEngineZgeComZglRwZrwLightC[40]; /* 0x00563448 */
    char sCNgEngineZgeZgeC[24];              /* 0x00563470 */
    char sZGLDUMP[12];                       /* 0x00563488 */
    char sZGEDUMP[12];                       /* 0x00563494 */
    char s_005634a0[60];                     /* 0x005634a0 */
    char sMovieInit[16];                     /* 0x005634dc */
    char sMovieCloseAudio[24];               /* 0x005634ec */
    char sMovieShutdown[20];                 /* 0x00563504 */
    char sMovieFillAudio[20];                /* 0x00563518 */
    char sMovieClose[16];                    /* 0x0056352c */
    char sMovieEndOfMovie[24];               /* 0x0056353c */
    char sPanorama[12];                      /* 0x00563554 */
    char sOpenedFileS[28];                   /* 0x00563560 */
    char sClosedFile[24];                    /* 0x0056357c */
    char sSymbolNotFoundS[24];               /* 0x00563594 */
    char s_005635ac[4];                      /* 0x005635ac */
    char sX[4];                              /* 0x005635b0 */
    char s_005635b4[4];                      /* 0x005635b4 */
    char s0x[4];                             /* 0x005635b8 */
    char s0X[4];                             /* 0x005635bc */
    char sFileSLineD[20];                    /* 0x005635c0 */
    char sIncludedFrom[16];                  /* 0x005635d4 */
    char sCanTOpenIncludeFileS[32];          /* 0x005635e4 */
    char sERRORMaximumIncludeNestingReached[44];  /* 0x00563604 */
    char sDEFINE[8];                              /* 0x00563630 */
    char sEndif[8];                               /* 0x00563638 */
    char sIfndef[8];                              /* 0x00563640 */
    char sINCLUDEPATH[16];                        /* 0x00563648 */
    char sINCLUDE[12];                            /* 0x00563658 */
    char s_00563664[4];                           /* 0x00563664 */
    char s_00563668[4];                           /* 0x00563668 */
    char s_0056366c[4];                           /* 0x0056366c */
    char s_00563670[4];                           /* 0x00563670 */
    char s_00563674[8];                           /* 0x00563674 */
    char sOutOfSymbolSpace[24];                   /* 0x0056367c */
    char sReplacingSymbolS[24];                   /* 0x00563694 */
    char s_005636ac[4];                           /* 0x005636ac */
    char sZTOKENAssertFailedFoundSExpectingS[52]; /* 0x005636b0 */
    char sSetCapitalizeFALSE[32];                 /* 0x005636e4 */
    char sSetCapitalizeTRUE[32];                  /* 0x00563704 */
    char sCNgEngineZgeZgeModsZFontC[32];          /* 0x00563724 */
    char sBadFontGlyphNumber[24];                 /* 0x00563744 */
    char sBadFontVersion[20];                     /* 0x0056375c */
    float kZrwBBoxFaces[6][4];          /* 0x00563770; the type is local to zge/com/zglrw/zrwramdisk.c */
    char sPBBoxOutPBBoxIn[20];          /* 0x005637d0 */
    char sCNgEngineZgeZutilC[24];       /* 0x005637e4 */
    char s_005637fc[24];                /* 0x005637fc */
    char sXXXXXXXXXXXXXXXXXXXX[24];     /* 0x00563814 */
    char s7dXXXXXXXXXXX[24];            /* 0x0056382c */
    char s_00563844[24];                /* 0x00563844 */
    char s7d[24];                       /* 0x0056385c */
    intptr_t gZrwRamDiskDevice;         /* 0x00563874 */
    char sErrorOpeningRAMDiskFile[32];  /* 0x00563878 */
    char sTooManyRAMDiskFilesInUse[32]; /* 0x00563898 */
    char sCNgEngineZgeComZglRwZrwRamDiskC[40];         /* 0x005638b8 */
    char sZrwFileRamDiskSetInfNotCalled[36];           /* 0x005638e0 */
    uint32_t gZProfileColors[5];                       /* 0x00563904; the type is local to zge/zprofile.c */
    uint32_t d_00563918;                               /* 0x00563918 */
    uint32_t d_0056391c;                               /* 0x0056391c */
    char sCNgEngineZgeZprofileC[28];                   /* 0x00563920 */
    char sInvalidParameterToZprofileSetLayout[44];     /* 0x0056393c */
    char sFailedToAllocateObj[24];                     /* 0x00563968 */
    char sUnknownBarType[20];                          /* 0x00563980 */
    char sApp[4];                                      /* 0x00563994 */
    char sEngCPU[8];                                   /* 0x00563998 */
    char sZTIMERAssertFailedUnableToRegisterUpdat[60]; /* 0x005639a0 */
    char sEventTypeTimerForObjD[32];                   /* 0x005639dc */
    char sScriptTypeTimer[20];                         /* 0x005639fc */
    char sPROFILENoHardwareCounterAvailable[40];       /* 0x00563a10 */
    char sCNgEngineZgePcPcProfileC[32];                /* 0x00563a38 */
    uint32_t d_00563a58[2];                            /* 0x00563a58 */
    void *p_00563a60;                                  /* 0x00563a60 */
    void *p_00563a64;                                  /* 0x00563a64 */
    void *p_00563a68;                                  /* 0x00563a68 */
    unsigned char pad_00563a6c[4];                     /* 0x00563a6c */
    char sAssertionFailedSFileSLineD[40];              /* 0x00563a70 */
    void *p_00563a98;                                  /* 0x00563a98 */
    void *p_00563a9c;                                  /* 0x00563a9c */
    void *p_00563aa0;                                  /* 0x00563aa0 */
    unsigned char pad_00563aa4[12];                    /* 0x00563aa4 */
    void *p_00563ab0;                                  /* 0x00563ab0 */
    uint32_t d_00563ab4[3];                            /* 0x00563ab4 */
    char sAsin[8];                                     /* 0x00563ac0 */
    uint32_t d_00563ac8;                               /* 0x00563ac8 */
    uint32_t d_00563acc;                               /* 0x00563acc */
    void *gCrtFloatHooks[6];             /* 0x00563ad0; the type is local to zge/pc/pcprofile.c */
    uint32_t d_00563ae8[2];              /* 0x00563ae8 */
    void *p_00563af0;                    /* 0x00563af0 */
    void *p_00563af4;                    /* 0x00563af4 */
    void *p_00563af8;                    /* 0x00563af8 */
    void *p_00563afc;                    /* 0x00563afc */
    uint32_t d_00563b00[2];              /* 0x00563b00 */
    uint32_t d_00563b08[2050];           /* 0x00563b08 */
    void *p_00565b10;                    /* 0x00565b10 */
    uint32_t d_00565b14;                 /* 0x00565b14 */
    void *p_00565b18;                    /* 0x00565b18 */
    unsigned char pad_00565b1c[4];       /* 0x00565b1c */
    void *p_00565b20;                    /* 0x00565b20 */
    uint32_t d_00565b24;                 /* 0x00565b24 */
    uint32_t d_00565b28[4];              /* 0x00565b28 */
    uint32_t d_00565b38[8];              /* 0x00565b38 */
    unsigned char d_00565b58[12];        /* 0x00565b58 */
    uint32_t d_00565b64[9];              /* 0x00565b64 */
    unsigned char d_00565b88[528];       /* 0x00565b88 */
    unsigned char d_00565d98[8];         /* 0x00565d98 */
    void *p_00565da0;                    /* 0x00565da0 */
    void *p_00565da4;                    /* 0x00565da4 */
    uint32_t d_00565da8;                 /* 0x00565da8 */
    uint32_t d_00565dac;                 /* 0x00565dac */
    uint32_t d_00565db0[28];             /* 0x00565db0 */
    uint32_t d_00565e20;                 /* 0x00565e20 */
    uint32_t d_00565e24;                 /* 0x00565e24 */
    char s_00565e28[4];                  /* 0x00565e28 */
    uint32_t d_00565e2c;                 /* 0x00565e2c */
    uint32_t d_00565e30[2];              /* 0x00565e30 */
    uint32_t d_00565e38[4];              /* 0x00565e38 */
    uint32_t d_00565e48;                 /* 0x00565e48 */
    void *p_00565e4c;                    /* 0x00565e4c */
    uint32_t d_00565e50;                 /* 0x00565e50 */
    void *p_00565e54;                    /* 0x00565e54 */
    uint32_t d_00565e58;                 /* 0x00565e58 */
    void *p_00565e5c;                    /* 0x00565e5c */
    uint32_t d_00565e60;                 /* 0x00565e60 */
    void *p_00565e64;                    /* 0x00565e64 */
    uint32_t d_00565e68;                 /* 0x00565e68 */
    void *p_00565e6c;                    /* 0x00565e6c */
    uint32_t d_00565e70;                 /* 0x00565e70 */
    void *p_00565e74;                    /* 0x00565e74 */
    uint32_t d_00565e78;                 /* 0x00565e78 */
    void *p_00565e7c;                    /* 0x00565e7c */
    uint32_t d_00565e80;                 /* 0x00565e80 */
    void *p_00565e84;                    /* 0x00565e84 */
    uint32_t d_00565e88;                 /* 0x00565e88 */
    void *p_00565e8c;                    /* 0x00565e8c */
    uint32_t d_00565e90;                 /* 0x00565e90 */
    void *p_00565e94;                    /* 0x00565e94 */
    uint32_t d_00565e98;                 /* 0x00565e98 */
    void *p_00565e9c;                    /* 0x00565e9c */
    uint32_t d_00565ea0;                 /* 0x00565ea0 */
    void *p_00565ea4;                    /* 0x00565ea4 */
    uint32_t d_00565ea8;                 /* 0x00565ea8 */
    void *p_00565eac;                    /* 0x00565eac */
    uint32_t d_00565eb0;                 /* 0x00565eb0 */
    void *p_00565eb4;                    /* 0x00565eb4 */
    uint32_t d_00565eb8;                 /* 0x00565eb8 */
    void *p_00565ebc;                    /* 0x00565ebc */
    uint32_t d_00565ec0;                 /* 0x00565ec0 */
    void *p_00565ec4;                    /* 0x00565ec4 */
    uint32_t d_00565ec8;                 /* 0x00565ec8 */
    void *p_00565ecc;                    /* 0x00565ecc */
    uint32_t d_00565ed0;                 /* 0x00565ed0 */
    void *p_00565ed4;                    /* 0x00565ed4 */
    unsigned char d_00565ed8[8];         /* 0x00565ed8 */
    unsigned char d_00565ee0[10];        /* 0x00565ee0 */
    unsigned char d_00565eea[10];        /* 0x00565eea */
    unsigned char d_00565ef4[9];         /* 0x00565ef4 */
    unsigned char d_00565efd[19];        /* 0x00565efd */
    uint32_t d_00565f10;                 /* 0x00565f10 */
    void *p_00565f14;                    /* 0x00565f14 */
    uint32_t d_00565f18;                 /* 0x00565f18 */
    void *p_00565f1c;                    /* 0x00565f1c */
    uint32_t d_00565f20;                 /* 0x00565f20 */
    void *p_00565f24;                    /* 0x00565f24 */
    uint32_t d_00565f28;                 /* 0x00565f28 */
    void *p_00565f2c;                    /* 0x00565f2c */
    uint32_t d_00565f30;                 /* 0x00565f30 */
    void *p_00565f34;                    /* 0x00565f34 */
    uint32_t d_00565f38;                 /* 0x00565f38 */
    void *p_00565f3c;                    /* 0x00565f3c */
    uint32_t d_00565f40;                 /* 0x00565f40 */
    void *p_00565f44;                    /* 0x00565f44 */
    uint32_t d_00565f48;                 /* 0x00565f48 */
    void *p_00565f4c;                    /* 0x00565f4c */
    uint32_t d_00565f50;                 /* 0x00565f50 */
    void *p_00565f54;                    /* 0x00565f54 */
    uint32_t d_00565f58;                 /* 0x00565f58 */
    void *p_00565f5c;                    /* 0x00565f5c */
    uint32_t d_00565f60;                 /* 0x00565f60 */
    void *p_00565f64;                    /* 0x00565f64 */
    uint32_t d_00565f68;                 /* 0x00565f68 */
    void *p_00565f6c;                    /* 0x00565f6c */
    uint32_t d_00565f70;                 /* 0x00565f70 */
    void *p_00565f74;                    /* 0x00565f74 */
    uint32_t d_00565f78;                 /* 0x00565f78 */
    void *p_00565f7c;                    /* 0x00565f7c */
    uint32_t d_00565f80;                 /* 0x00565f80 */
    void *p_00565f84;                    /* 0x00565f84 */
    uint32_t d_00565f88;                 /* 0x00565f88 */
    void *p_00565f8c;                    /* 0x00565f8c */
    uint32_t d_00565f90;                 /* 0x00565f90 */
    void *p_00565f94;                    /* 0x00565f94 */
    uint32_t d_00565f98;                 /* 0x00565f98 */
    void *p_00565f9c;                    /* 0x00565f9c */
    uint32_t d_00565fa0;                 /* 0x00565fa0 */
    void *p_00565fa4;                    /* 0x00565fa4 */
    uint32_t d_00565fa8;                 /* 0x00565fa8 */
    void *p_00565fac;                    /* 0x00565fac */
    uint32_t d_00565fb0;                 /* 0x00565fb0 */
    void *p_00565fb4;                    /* 0x00565fb4 */
    uint32_t d_00565fb8;                 /* 0x00565fb8 */
    void *p_00565fbc;                    /* 0x00565fbc */
    uint32_t d_00565fc0;                 /* 0x00565fc0 */
    void *p_00565fc4;                    /* 0x00565fc4 */
    uint32_t d_00565fc8;                 /* 0x00565fc8 */
    void *p_00565fcc;                    /* 0x00565fcc */
    uint32_t d_00565fd0;                 /* 0x00565fd0 */
    void *p_00565fd4;                    /* 0x00565fd4 */
    uint32_t d_00565fd8;                 /* 0x00565fd8 */
    void *p_00565fdc;                    /* 0x00565fdc */
    uint32_t d_00565fe0;                 /* 0x00565fe0 */
    void *p_00565fe4;                    /* 0x00565fe4 */
    uint32_t d_00565fe8[2];              /* 0x00565fe8 */
    uint32_t d_00565ff0[2];              /* 0x00565ff0 */
    uint32_t d_00565ff8[4];              /* 0x00565ff8 */
    uint32_t d_00566008[2];              /* 0x00566008 */
    uint32_t d_00566010[3];              /* 0x00566010 */
    uint32_t d_0056601c[3];              /* 0x0056601c */
    void *p_00566028;                    /* 0x00566028 */
    void *p_0056602c;                    /* 0x0056602c */
    unsigned char pad_00566030[2];       /* 0x00566030 */
    char s_00566032[2];                  /* 0x00566032 */
    uint32_t d_00566034[128];            /* 0x00566034 */
    uint32_t d_00566234;                 /* 0x00566234 */
    char s_00566238[4];                  /* 0x00566238 */
    uint32_t d_0056623c;                 /* 0x0056623c */
    char s_00566240[8];                  /* 0x00566240 */
    char s_00566248[4];                  /* 0x00566248 */
    unsigned char pad_0056624c[4];       /* 0x0056624c */
    uint32_t d_00566250[6];              /* 0x00566250 */
    uint32_t d_00566268[6];              /* 0x00566268 */
    uint32_t d_00566280[2];              /* 0x00566280 */
    uint32_t d_00566288;                 /* 0x00566288 */
    uint32_t d_0056628c[3];              /* 0x0056628c */
    uint32_t d_00566298[56];             /* 0x00566298 */
    unsigned char d_00566378[8];         /* 0x00566378 */
    uint32_t d_00566380[2];              /* 0x00566380 */
    uint32_t d_00566388;                 /* 0x00566388 */
    uint32_t d_0056638c[89];             /* 0x0056638c */
    uint32_t d_005664f0[88];             /* 0x005664f0 */
    uint32_t d_00566650[88];             /* 0x00566650 */
    RwPluginRegistry rpClumpRegistry;    /* 0x005667b0 */
    RwPluginRegistry rpAtomicRegistry;   /* 0x005667c4 */
    unsigned char pad_005667d8[8];       /* 0x005667d8 */
    RwPluginRegistry rpGeometryRegistry; /* 0x005667e0 */
    unsigned char pad_005667f4[12];      /* 0x005667f4 */
    RwPluginRegistry rpMaterialRegistry; /* 0x00566800 */
    RwSurfaceProperties rpMaterialDefaultSurfaceProps; /* 0x00566814 */
    RwPluginRegistry rpWorldRegistry;                  /* 0x00566820 */
    unsigned char pad_00566834[12];                    /* 0x00566834 */
    float d_00566840;                                  /* 0x00566840 */
    float d_00566844;                                  /* 0x00566844 */
    float d_00566848;                                  /* 0x00566848 */
    float d_0056684c;                                  /* 0x0056684c */
    float d_00566850;                                  /* 0x00566850 */
    float d_00566854[3];                               /* 0x00566854 */
    RwPluginRegistry rpWorldSectorRegistry;            /* 0x00566860 */
    unsigned char pad_00566874[12];                    /* 0x00566874 */
    RwPluginRegistry rpLightRegistry;                  /* 0x00566880 */
    unsigned char pad_00566894[12];                    /* 0x00566894 */
    uint32_t rwD3DAtomicDefaultColor;                  /* 0x005668a0 */
    unsigned char pad_005668a4[12];                    /* 0x005668a4 */
    uint32_t rwD3DSectorDefaultColor;                  /* 0x005668b0 */
    unsigned char pad_005668b4[12];                    /* 0x005668b4 */
    uint8_t rtCharsetGlyphs[1274];                     /* 0x005668c0; the type is local to rw/charset.c */
    unsigned char pad_00566dba[6];                     /* 0x00566dba */
    RwPluginRegistry rwEngineRegistry;                 /* 0x00566dc0 */
    uint32_t d_00566dd4[67];                           /* 0x00566dd4 */
    RwPluginRegistry rwFrameRegistry;                  /* 0x00566ee0 */
    unsigned char pad_00566ef4[12];                    /* 0x00566ef4 */
    char sSoftwareCriterionRenderWare[32];             /* 0x00566f00 */
    int32_t rwD3DRenderOrder;                          /* 0x00566f20 */
    unsigned char pad_00566f24[12];                    /* 0x00566f24 */
    RwPluginRegistry rwRasterRegistry;                 /* 0x00566f30 */
    unsigned char pad_00566f44[12];                    /* 0x00566f44 */
    RwPluginRegistry rwCameraRegistry;                 /* 0x00566f50 */
    unsigned char pad_00566f64[12];                    /* 0x00566f64 */
    char s0123456789abcdefghijklmnopqrstuvwxyz[40];    /* 0x00566f70 */
    RwPluginRegistry rwTextureRegistry;                /* 0x00566f98 */
    RwPluginRegistry rwTexDictionaryRegistry;          /* 0x00566fac */
    RwPluginRegistry rwImageRegistry;                  /* 0x00566fc0 */
    unsigned char pad_00566fd4[12];                    /* 0x00566fd4 */
    uint32_t d_00566fe0;                               /* 0x00566fe0 */
    RwDevice rwD3D7Device;                             /* 0x00566fe4, also rwOrigDevice */
    uint32_t d_0056701c;                               /* 0x0056701c */
    void *p_00567020;                                  /* 0x00567020 */
    uint32_t d_00567024;                               /* 0x00567024 */
    void *p_00567028;                                  /* 0x00567028 */
    uint32_t d_0056702c;                               /* 0x0056702c */
    void *p_00567030;                                  /* 0x00567030 */
    uint32_t d_00567034;                               /* 0x00567034 */
    void *p_00567038;                                  /* 0x00567038 */
    uint32_t d_0056703c;                               /* 0x0056703c */
    void *p_00567040;                                  /* 0x00567040 */
    uint32_t d_00567044;                               /* 0x00567044 */
    void *p_00567048;                                  /* 0x00567048 */
    uint32_t d_0056704c;                               /* 0x0056704c */
    void *p_00567050;                                  /* 0x00567050 */
    uint32_t d_00567054;                               /* 0x00567054 */
    void *p_00567058;                                  /* 0x00567058 */
    uint32_t d_0056705c;                               /* 0x0056705c */
    void *p_00567060;                                  /* 0x00567060 */
    uint32_t d_00567064;                               /* 0x00567064 */
    void *p_00567068;                                  /* 0x00567068 */
    uint32_t d_0056706c;                               /* 0x0056706c */
    void *p_00567070;                                  /* 0x00567070 */
    uint32_t d_00567074;                               /* 0x00567074 */
    void *p_00567078;                                  /* 0x00567078 */
    uint32_t d_0056707c;                               /* 0x0056707c */
    void *p_00567080;                                  /* 0x00567080 */
    uint32_t d_00567084;                               /* 0x00567084 */
    void *p_00567088;                                  /* 0x00567088 */
    uint32_t d_0056708c;                               /* 0x0056708c */
    void *p_00567090;                                  /* 0x00567090 */
    uint32_t d_00567094;                               /* 0x00567094 */
    void *p_00567098;                                  /* 0x00567098 */
    uint32_t d_0056709c;                               /* 0x0056709c */
    void *p_005670a0;                                  /* 0x005670a0 */
    uint32_t d_005670a4;                               /* 0x005670a4 */
    void *p_005670a8;                                  /* 0x005670a8 */
    uint32_t d_005670ac;                               /* 0x005670ac */
    void *p_005670b0;                                  /* 0x005670b0 */
    uint32_t d_005670b4;                               /* 0x005670b4 */
    void *p_005670b8;                                  /* 0x005670b8 */
    uint32_t d_005670bc;                               /* 0x005670bc */
    void *p_005670c0;                                  /* 0x005670c0 */
    uint32_t d_005670c4;                               /* 0x005670c4 */
    void *p_005670c8;                                  /* 0x005670c8 */
    uint32_t d_005670cc;                               /* 0x005670cc */
    void *p_005670d0;                                  /* 0x005670d0 */
    uint32_t d_005670d4;                               /* 0x005670d4 */
    void *p_005670d8;                                  /* 0x005670d8 */
    uint32_t d_005670dc;                               /* 0x005670dc */
    void *p_005670e0;                                  /* 0x005670e0 */
    uint32_t d_005670e4;                               /* 0x005670e4 */
    void *p_005670e8;                                  /* 0x005670e8 */
    uint32_t d_005670ec;                               /* 0x005670ec */
    void *p_005670f0;                                  /* 0x005670f0 */
    unsigned char pad_005670f4[12];                    /* 0x005670f4 */
    uint32_t d_00567100[32];                           /* 0x00567100 */
    uint32_t d_00567180[32];                           /* 0x00567180 */
    uint32_t d_00567200[31];                           /* 0x00567200 */
    uint32_t d_0056727c[8];                            /* 0x0056727c */
    void *p_0056729c;                                  /* 0x0056729c */
    unsigned char pad_005672a0[92];                    /* 0x005672a0 */
    void *p_005672fc;                                  /* 0x005672fc */
    uint32_t d_00567300[4];                            /* 0x00567300 */
    char s105[8];                                      /* 0x00567310 */
    unsigned char pad_00567318[4];                     /* 0x00567318 */
    uint32_t d_0056731c[2];                            /* 0x0056731c */
    char sIHDR[8];                                     /* 0x00567324 */
    char sIDAT[8];                                     /* 0x0056732c */
    char sIEND[8];                                     /* 0x00567334 */
    char sPLTE[8];                                     /* 0x0056733c */
    char sBKGD[8];                                     /* 0x00567344 */
    char sCHRM[8];                                     /* 0x0056734c */
    char sGAMA[8];                                     /* 0x00567354 */
    char sHIST[8];                                     /* 0x0056735c */
    char sOFFs[8];                                     /* 0x00567364 */
    char sPCAL[8];                                     /* 0x0056736c */
    char sPHYs[8];                                     /* 0x00567374 */
    char sSBIT[8];                                     /* 0x0056737c */
    char sSRGB[8];                                     /* 0x00567384 */
    char sTEXt[8];                                     /* 0x0056738c */
    char sTIME[8];                                     /* 0x00567394 */
    char sTRNS[8];                                     /* 0x0056739c */
    char sZTXt[8];                                     /* 0x005673a4 */
    uint32_t d_005673ac[7];                            /* 0x005673ac */
    uint32_t d_005673c8[7];                            /* 0x005673c8 */
    uint32_t d_005673e4[7];                            /* 0x005673e4 */
    uint32_t d_00567400[7];                            /* 0x00567400 */
    uint32_t d_0056741c[7];                            /* 0x0056741c */
    uint32_t d_00567438[10];                           /* 0x00567438 */
    uint32_t d_00567460[8];                            /* 0x00567460 */
    uint32_t d_00567480[64];                           /* 0x00567480 */
    uint32_t d_00567580[64];                           /* 0x00567580 */
    uint32_t d_00567680[64];                           /* 0x00567680 */
    uint32_t d_00567780[4];                            /* 0x00567780 */
    char sErrorDecodingZTXtChunk[28];                  /* 0x00567790 */
    unsigned char pad_005677ac[4];                     /* 0x005677ac */
    uint32_t d_005677b0[256];                          /* 0x005677b0 */
    uint32_t d_00567bb0[20];                           /* 0x00567bb0 */
    void *p_00567c00;                                  /* 0x00567c00 */
    void *p_00567c04;                                  /* 0x00567c04 */
    void *p_00567c08;                                  /* 0x00567c08 */
    void *p_00567c0c;                                  /* 0x00567c0c */
    void *p_00567c10;                                  /* 0x00567c10 */
    void *p_00567c14;                                  /* 0x00567c14 */
    void *p_00567c18;                                  /* 0x00567c18 */
    void *p_00567c1c;                                  /* 0x00567c1c */
    void *p_00567c20;                                  /* 0x00567c20 */
    void *p_00567c24;                                  /* 0x00567c24 */
    unsigned char pad_00567c28[8];                     /* 0x00567c28 */
    uint32_t d_00567c30[20];                           /* 0x00567c30 */
    uint32_t d_00567c80[30];                           /* 0x00567c80 */
    uint32_t d_00567cf8[30];                           /* 0x00567cf8 */
    uint32_t d_00567d70[31];                           /* 0x00567d70 */
    uint32_t d_00567dec[31];                           /* 0x00567dec */
    uint32_t d_00567e68[64];                           /* 0x00567e68 */
    uint32_t d_00567f68[1024];                         /* 0x00567f68 */
    uint32_t d_00568f68;                               /* 0x00568f68 */
    char s_00568f6c[4];                                /* 0x00568f6c */
    char sInflate113Copyright19951998MarkAdler[48];    /* 0x00568f70 */
    char sTan[4];                                      /* 0x00568fa0 */
    unsigned char pad_00568fa4[12];                    /* 0x00568fa4 */
    char sSin[4];                                      /* 0x00568fb0 */
    unsigned char pad_00568fb4[12];                    /* 0x00568fb4 */
    char sCos[4];                                      /* 0x00568fc0 */
    unsigned char pad_00568fc4[12];                    /* 0x00568fc4 */
    double d_00568fd0;                                 /* 0x00568fd0 */
    char sPow[4];                                      /* 0x00568fd8 */
    unsigned char pad_00568fdc[4];                     /* 0x00568fdc */
    unsigned char d_00568fe0[10];                      /* 0x00568fe0 */
    unsigned char d_00568fea[20];                      /* 0x00568fea */
    unsigned char d_00568ffe[20];                      /* 0x00568ffe */
    unsigned char d_00569012[34];                      /* 0x00569012 */
    void *p_00569034;                                  /* 0x00569034 */
    void *p_00569038;                                  /* 0x00569038 */
    void *p_0056903c;                                  /* 0x0056903c */
    void *p_00569040;                                  /* 0x00569040 */
    unsigned char pad_00569044[12];                    /* 0x00569044 */
    uint32_t d_00569050[4];                            /* 0x00569050 */
    float d_00569060;                                  /* 0x00569060 */
    float d_00569064;                                  /* 0x00569064 */
    uint32_t d_00569068;                               /* 0x00569068 */
    uint32_t d_0056906c[6];                            /* 0x0056906c */
    float d_00569084[2];                               /* 0x00569084 */
    uint32_t d_0056908c[4];                            /* 0x0056908c */
    float d_0056909c[2];                               /* 0x0056909c */
    unsigned char d_005690a4[10];                      /* 0x005690a4 */
    BmxUnalignedPtr p_005690ae;                        /* 0x005690ae */
    BmxUnalignedPtr p_005690b2;                        /* 0x005690b2 */
    BmxUnalignedPtr p_005690b6;                        /* 0x005690b6 */
    BmxUnalignedPtr p_005690ba;                        /* 0x005690ba */
    BmxUnalignedPtr p_005690be;                        /* 0x005690be */
    BmxUnalignedPtr p_005690c2;                        /* 0x005690c2 */
    BmxUnalignedPtr p_005690c6;                        /* 0x005690c6 */
    BmxUnalignedPtr p_005690ca;                        /* 0x005690ca */
    BmxUnalignedPtr p_005690ce;                        /* 0x005690ce */
    BmxUnalignedPtr p_005690d2;                        /* 0x005690d2 */
    BmxUnalignedPtr p_005690d6;                        /* 0x005690d6 */
    BmxUnalignedPtr p_005690da;                        /* 0x005690da */
    BmxUnalignedPtr p_005690de;                        /* 0x005690de */
    BmxUnalignedPtr p_005690e2;                        /* 0x005690e2 */
    BmxUnalignedPtr p_005690e6;                        /* 0x005690e6 */
    BmxUnalignedPtr p_005690ea;                        /* 0x005690ea */
    BmxUnalignedPtr p_005690ee;                        /* 0x005690ee */
    BmxUnalignedPtr p_005690f2;                        /* 0x005690f2 */
    BmxUnalignedPtr p_005690f6;                        /* 0x005690f6 */
    BmxUnalignedPtr p_005690fa;                        /* 0x005690fa */
    BmxUnalignedPtr p_005690fe;                        /* 0x005690fe */
    BmxUnalignedPtr p_00569102;                        /* 0x00569102 */
    BmxUnalignedPtr p_00569106;                        /* 0x00569106 */
    BmxUnalignedPtr p_0056910a;                        /* 0x0056910a */
    BmxUnalignedPtr p_0056910e;                        /* 0x0056910e */
    BmxUnalignedPtr p_00569112;                        /* 0x00569112 */
    BmxUnalignedPtr p_00569116;                        /* 0x00569116 */
    BmxUnalignedPtr p_0056911a;                        /* 0x0056911a */
    BmxUnalignedPtr p_0056911e;                        /* 0x0056911e */
    BmxUnalignedPtr p_00569122;                        /* 0x00569122 */
    BmxUnalignedPtr p_00569126;                        /* 0x00569126 */
    BmxUnalignedPtr p_0056912a;                        /* 0x0056912a */
    BmxUnalignedPtr p_0056912e;                        /* 0x0056912e */
    BmxUnalignedPtr p_00569132;                        /* 0x00569132 */
    BmxUnalignedPtr p_00569136;                        /* 0x00569136 */
    BmxUnalignedPtr p_0056913a;                        /* 0x0056913a */
    BmxUnalignedPtr p_0056913e;                        /* 0x0056913e */
    BmxUnalignedPtr p_00569142;                        /* 0x00569142 */
    BmxUnalignedPtr p_00569146;                        /* 0x00569146 */
    BmxUnalignedPtr p_0056914a;                        /* 0x0056914a */
    BmxUnalignedPtr p_0056914e;                        /* 0x0056914e */
    BmxUnalignedPtr p_00569152;                        /* 0x00569152 */
    BmxUnalignedPtr p_00569156;                        /* 0x00569156 */
    BmxUnalignedPtr p_0056915a;                        /* 0x0056915a */
    BmxUnalignedPtr p_0056915e;                        /* 0x0056915e */
    BmxUnalignedPtr p_00569162;                        /* 0x00569162 */
    BmxUnalignedPtr p_00569166;                        /* 0x00569166 */
    BmxUnalignedPtr p_0056916a;                        /* 0x0056916a */
    BmxUnalignedPtr p_0056916e;                        /* 0x0056916e */
    BmxUnalignedPtr p_00569172;                        /* 0x00569172 */
    BmxUnalignedPtr p_00569176;                        /* 0x00569176 */
    BmxUnalignedPtr p_0056917a;                        /* 0x0056917a */
    BmxUnalignedPtr p_0056917e;                        /* 0x0056917e */
    BmxUnalignedPtr p_00569182;                        /* 0x00569182 */
    BmxUnalignedPtr p_00569186;                        /* 0x00569186 */
    BmxUnalignedPtr p_0056918a;                        /* 0x0056918a */
    BmxUnalignedPtr p_0056918e;                        /* 0x0056918e */
    BmxUnalignedPtr p_00569192;                        /* 0x00569192 */
    BmxUnalignedPtr p_00569196;                        /* 0x00569196 */
    BmxUnalignedPtr p_0056919a;                        /* 0x0056919a */
    BmxUnalignedPtr p_0056919e;                        /* 0x0056919e */
    BmxUnalignedPtr p_005691a2;                        /* 0x005691a2 */
    BmxUnalignedPtr p_005691a6;                        /* 0x005691a6 */
    BmxUnalignedPtr p_005691aa;                        /* 0x005691aa */
    unsigned char pad_005691ae[2];                     /* 0x005691ae */
    uint32_t d_005691b0;                               /* 0x005691b0 */
    uint32_t d_005691b4;                               /* 0x005691b4 */
    uint32_t d_005691b8;                               /* 0x005691b8 */
    char sPST[4];                                      /* 0x005691bc */
    unsigned char pad_005691c0[60];                    /* 0x005691c0 */
    char sPDT[4];                                      /* 0x005691fc */
    unsigned char pad_00569200[60];                    /* 0x00569200 */
    void *p_0056923c;                                  /* 0x0056923c */
    void *p_00569240;                                  /* 0x00569240 */
    unsigned char pad_00569244[4];                     /* 0x00569244 */
    uint32_t d_00569248;                               /* 0x00569248 */
    unsigned char d_0056924c[4];                       /* 0x0056924c */
    unsigned char d_00569250[8];                       /* 0x00569250 */
    uint32_t d_00569258;                               /* 0x00569258 */
    unsigned char d_0056925c[4];                       /* 0x0056925c */
    unsigned char d_00569260[4];                       /* 0x00569260 */
    uint32_t d_00569264[12];                           /* 0x00569264 */
    uint32_t d_00569294;                               /* 0x00569294 */
    uint32_t d_00569298[858];                          /* 0x00569298 */
};

#define AT(member, addr) _Static_assert(offsetof(struct BmxData, member) == (addr) - 0x00551000, #member)
AT(gActionMode, 0x00551a30);
AT(gActionDemoTable, 0x00551a34);
AT(gActionFrameReload, 0x00551a3c);
AT(gActionFrameCount, 0x00551a40);
AT(gCameraTuning, 0x00551c54);
AT(gGameLevelInfoIndex, 0x00551fb8);
AT(gTitleUnknown551fd4, 0x00551fd4);
AT(gScoreRecordSlot, 0x00553658);
AT(gRiderSignatureTricks, 0x00553840);
AT(gContrlerBindings, 0x00553ee0);
AT(gFxBurstBlock, 0x00554724);
AT(gFxPuffBlock, 0x00554738);
AT(gFxScatterSlots, 0x00554798);
AT(gFxSkidCorners, 0x00554964);
AT(gFxPresets, 0x00554a9c);
AT(gFxEmitBlock, 0x00554ab8);
AT(gFxCreateBlock, 0x00554ad8);
AT(gGameRoundCounts, 0x00554c8c);
AT(gGameRideSeconds, 0x00554cb0);
AT(gGameMarkerColors, 0x00554cb4);
AT(gGameJudgeBias, 0x00554cc0);
AT(gGameDefaultWord, 0x00554d00);
AT(gGameGoals, 0x00555118);
AT(gGoalEntriesA, 0x005568b8);
AT(gGoalEntriesB, 0x00556948);
AT(gOvlSavedSubMode, 0x00556ef8);
AT(gOvlSaveStep, 0x00556efc);
AT(gOvlElements, 0x00556f00);
AT(gOvlMpModeLines, 0x00557184);
AT(gOvlScreens, 0x005571b8);
AT(gOvlSavePages, 0x00557224);
AT(gOvlMpWord, 0x00557254);
AT(gOvlMpWordCursor, 0x0055725d);
AT(gOvlBarSize, 0x00557260);
AT(gOvlFadeSize, 0x00557298);
AT(gIndicatorText, 0x005572d4);
AT(gIndicatorColor, 0x005572e8);
AT(gTrickDoubleNames, 0x00557308);
AT(gPhysRollFriction, 0x005577ec);
AT(gPhysAirDrag, 0x00557800);
AT(gPhysRollYawDamp, 0x00557804);
AT(gRiderAirYawDamp, 0x00557808);
AT(gPhysGrindYawDamp, 0x0055780c);
AT(gPhysGravity, 0x00557810);
AT(gPhysRestSpeed, 0x00557814);
AT(gPhysLeanSpeedMax, 0x00557818);
AT(gPhysBailImpact, 0x0055781c);
AT(gPhysLandCrashSpeed, 0x00557820);
AT(gPhysWheelSideSlip, 0x00557824);
AT(gPhysRollSpinSpeed, 0x00557828);
AT(gPhysAirSpeedCap, 0x0055782c);
AT(gPhysPipeLaunchScale, 0x00557834);
AT(gPhysGrindMinSpeed, 0x00557838);
AT(gPhysAirProbe, 0x0055783c);
AT(gPhysWallAngle, 0x00557848);
AT(gPhysSlopeAngle, 0x0055784a);
AT(gPhysMaxSlope, 0x0055784c);
AT(gPhysRailAngleLimit, 0x0055784e);
AT(gPhysPipeAngleLimit, 0x00557856);
AT(gPhysPipeTiltLimit, 0x00557858);
AT(gPhysGrindInSteps, 0x0055785c);
AT(gPhysStallInSteps, 0x00557860);
AT(gPhysAirTurnSteps, 0x00557864);
AT(gPhysGrindInRadius, 0x00557868);
AT(gPhysEdgeSearchRadius, 0x00557870);
AT(gPhysGrindBalance, 0x00557874);
AT(gPhysSuspGravityScale, 0x00557878);
AT(gPhysRollSteer, 0x0055787c);
AT(gPhysUpBlend, 0x00557880);
AT(gPhysLeanMax, 0x00557884);
AT(gPhysDown, 0x00557888);
AT(gPhysStateExit, 0x00557898);
AT(gPhysStateEnter, 0x005578b4);
AT(gPhysStateOrient, 0x005578d0);
AT(gPhysStateVelocity, 0x005578ec);
AT(gPhysStateMove, 0x00557908);
AT(gPhysStateSync, 0x00557924);
AT(gPropDefs, 0x00557d28);
AT(gRiderHopSpeed, 0x0055886c);
AT(gRiderHopBoost, 0x00558870);
AT(gRiderHopMargin, 0x00558874);
AT(gRiderJumpPower, 0x00558878);
AT(gRiderJumpBase, 0x0055887c);
AT(gRiderLandGrip, 0x00558888);
AT(gRiderSteerBase, 0x0055888c);
AT(gPhysLeanRange, 0x00558890);
AT(gRiderSteerFalloff, 0x00558894);
AT(gRiderAirTurnNudge, 0x00558898);
AT(gRiderAirTurnStart, 0x0055889c);
AT(gRiderAirTurnMax, 0x005588a0);
AT(gRiderBikeSpinStep, 0x005588a4);
AT(gRiderBikeSpinMax, 0x005588a8);
AT(gRiderTrickSteer, 0x005588b0);
AT(gRiderJumpFrames, 0x005588b4);
AT(gRiderModeStep, 0x005588b8);
AT(gRiderAirStep, 0x005588ec);
AT(gRiderGrindStep, 0x005588f8);
AT(gRiderLipStep, 0x005588fc);
AT(gRiderManualStep, 0x00558900);
AT(gRiderScoreWeight, 0x00558914);
AT(gRiderTrickCombo, 0x00558b40);
AT(gTrickClassLo, 0x00558fb4);
AT(gScoreTrick, 0x00559118);
AT(gScoreTierAdd, 0x0055926c);
AT(gScoreTierMul, 0x0055931c);
AT(gTrickTurnNames, 0x00559390);
AT(gTrickSpinNames, 0x005594e0);
AT(gGuiAssetFile, 0x00559d48);
AT(gGuiPulseStep, 0x00559d4c);
AT(gGuiElementMagic, 0x0055a1dc);
AT(gGuiScreenMagic, 0x0055a210);
AT(gOptMainPulseStep, 0x0055a354);
AT(gLanguageTable, 0x0055a358);
AT(gMenuEntryStrings, 0x0055a35c);
AT(gMenuSlotPos, 0x0055a368);
AT(gMenuSlotAlpha, 0x0055a384);
AT(gMenuSlotTint, 0x0055a394);
AT(gOptStateStrings, 0x0055a3b0);
AT(gOptionTable, 0x0055a3bc);
AT(gStripColors, 0x0055a3dc);
AT(gKeyNames, 0x0055a3f4);
AT(gKbLayoutTitles, 0x0055a43c);
AT(gKbLayoutLabels, 0x0055a450);
AT(gRiderCaptions, 0x0055aaa0);
AT(gCaptionKinds, 0x0055aad0);
AT(gCaptionOrders, 0x0055aaf0);
AT(gCaptionLocked, 0x0055ab10);
AT(gBikeNames, 0x0055ab18);
AT(gBikeTextures, 0x0055ab30);
AT(gBikeLocked, 0x0055ab48);
AT(gBikeCaptions, 0x0055ad30);
AT(gTitleIntro, 0x0055af40);
AT(gTitleDave, 0x0055afbc);
AT(gTitleRyan, 0x0055b038);
AT(gTitleAmish, 0x0055b0b4);
AT(gInfoHeadIds, 0x0055b130);
AT(gInfoLineIds, 0x0055b138);
AT(gCreditsMovie, 0x0055b2c8);
AT(gBmxSoundState, 0x0055b530);
AT(gBmxMusicMode, 0x0055b534);
AT(gBmxVolumes, 0x0055b538);
AT(gBmxSfxFile, 0x0055b53c);
AT(gBmxMenuSfxFile, 0x0055b53e);
AT(gBmxGameSfxFile, 0x0055b540);
AT(gBmxGunSfxFile, 0x0055b542);
AT(gBmxRiderSfxFile, 0x0055b544);
AT(gBmxLevelSfxFile, 0x0055b546);
AT(gBmxMusicFile, 0x0055b548);
AT(gBmxSpeechFile, 0x0055b54a);
AT(gBmxNextTrack, 0x0055b54c);
AT(gBmxTrackLoop, 0x0055b54d);
AT(gBmxLevelTracks, 0x0055b550);
AT(gZoverGlyph, 0x0055b960);
AT(gNgAssetsDir, 0x0055ba04);
AT(gNgRamDisk, 0x0055bc38);
AT(gNgLegalPending, 0x0055bc3c);
AT(gNgWrapName, 0x0055bc40);
AT(gNgDebugFaceColor, 0x0055bc7c);
AT(gNgDebugEdgeColor, 0x0055bc80);
AT(gNgDebugTaggedColor, 0x0055bc84);
AT(gNgDebugRollerColor, 0x0055bc88);
AT(gNgMainLightColor, 0x0055bc8c);
AT(gNgLightDir, 0x0055bc90);
AT(gNgLightRef, 0x0055bcc0);
AT(gNgLightColors, 0x0055bcf0);
AT(gNgSpotMainColor, 0x0055bcfc);
AT(gNgLegalTime, 0x0055bd04);
AT(gFxQuality, 0x0055bd08);
AT(gFxFps, 0x0055bd0c);
AT(gZramdiskDeviceName, 0x0055bf38);
AT(gFxSkidBase, 0x0055c0d8);
AT(gFxSkidFrameNames, 0x0055c0dc);
AT(gFxSkidName, 0x0055c11c);
AT(gFxSkidConfig, 0x0055c120);
AT(gFxSparkDefault, 0x0055c128);
AT(gFxDustDefault, 0x0055c1a0);
AT(gFxRippleDefault, 0x0055c220);
AT(gFxSpark, 0x0055c278);
AT(gFxShard, 0x0055c2b0);
AT(gFxSplash, 0x0055c2e8);
AT(gFxDust, 0x0055c320);
AT(gFxPoster, 0x0055c358);
AT(gFxRipple, 0x0055c390);
AT(gFxLightning, 0x0055c3c8);
AT(gFxSparkCreateDefault, 0x0055c400);
AT(gFxGravityParams, 0x0055c470);
AT(gNgifBrightness, 0x0055c68c);
AT(gStripScale0, 0x0055c690);
AT(gNgifScreenH, 0x0055c694);
AT(gZoverOt, 0x0055c698);
AT(gZcollideBoxEdges, 0x0055d4f8);
AT(gZcollideBarEdge, 0x0055d504);
AT(gZresOverlaySources, 0x0055d978);
AT(gZrollerRiderNames, 0x0055dc50);
AT(gZrollerBikeNames, 0x0055dc80);
AT(gSemitones, 0x0055e820);
AT(gCents, 0x0055e8b4);
AT(gGainCurve, 0x0055e97c);
AT(gDsVolume, 0x0055ec7c);
AT(gAdpcmIndex, 0x0055f1ac);
AT(gAdpcmStep, 0x0055f1ec);
AT(gMotionTimeScale, 0x0055f35c);
AT(gZactorMax, 0x0055f360);
AT(gMotionMirrorMap, 0x0055f364);
AT(gMotionLoadedBank, 0x0055f38c);
AT(gSkeletonChain, 0x0055f390);
AT(gSkeletonOffsets, 0x0055f4c0);
AT(gZsndTicksPerSec, 0x0055ffc0);
AT(gZdebugStart, 0x00560030);
AT(gZProfilePrintAt, 0x00560038);
AT(gZrwDbgLineHook, 0x00560064);
AT(gZrwDbgTriangleHook, 0x00560068);
AT(gZrwDbgSphereHook, 0x0056006c);
AT(gZdebugPrint, 0x00560074);
AT(gZdebugIndent, 0x00560078);
AT(gLineFresh, 0x00560080);
AT(gAxesFresh, 0x00560084);
AT(gTriangleFresh, 0x00560088);
AT(gBoxFresh, 0x0056008c);
AT(gBoxEdges, 0x00560090);
AT(gZobjCameraVtable, 0x00560518);
AT(gZobjCameraHome, 0x00560540);
AT(gZfileRamDisk, 0x0056062c);
AT(gZrwMdlPluginOffset, 0x00560e70);
AT(gZglInitModel, 0x00560e78);
AT(gZglExitModel, 0x00560e7c);
AT(gZrwVtLoadModel, 0x00560e84);
AT(gZrwVtLoadChunk, 0x00560e8c);
AT(gZrwVtInstance, 0x00560e90);
AT(gZrwVtDestroy, 0x00560e94);
AT(gZrwVtUpdate, 0x00560e98);
AT(gZrwVtSetPos, 0x00560e9c);
AT(gZrwVtSetRot, 0x00560ea0);
AT(gZrwVtSetScale, 0x00560ea4);
AT(gZrwVtPartCount, 0x00560ea8);
AT(gZrwVtFindNode, 0x00560eac);
AT(gZrwVtNodeToWorld, 0x00560eb0);
AT(gZrwVtNodeVector, 0x00560eb4);
AT(gZrwVtNodeMatrix, 0x00560eb8);
AT(gZrwVtKey, 0x00560ec0);
AT(gZrwVtEntry, 0x00560ec4);
AT(gZrwVtNodeHashes, 0x00560ec8);
AT(gZrwVtSkeleton, 0x00560ecc);
AT(gZrwVtSetFlags, 0x00560ed8);
AT(gZrwVtOwner, 0x00560edc);
AT(gZbspObjectFrameFn, 0x00560ee0);
AT(gZrwVtClump, 0x00560ee4);
AT(gFxFindTexture, 0x00560ee8);
AT(gZrwVtMatCount, 0x00560eec);
AT(gZobjSetColor, 0x00560ef4);
AT(gZrwOpsSkinned, 0x00560fa8);
AT(gZrwOpsSimple, 0x00560fd0);
AT(gZglInitMaterial, 0x00561018);
AT(gZglExitMaterial, 0x0056101c);
AT(gZrollerFreeHandle, 0x00561024);
AT(gFxSetRaster, 0x0056102c);
AT(gZrwMaterialTexture, 0x00561034);
AT(gZbspMaterialFn, 0x00561038);
AT(gRwTextureClone, 0x0056103c);
AT(gZrwWorldSetHook, 0x00561044);
AT(gZresLoadTexture, 0x00561048);
AT(gFxTextureByHash, 0x0056104c);
AT(gErrorCaption, 0x005611d8);
AT(gZrwGraphicLibraryInitialize, 0x00561468);
AT(gZrollerRegister, 0x00561470);
AT(gZrollerUnregister, 0x00561474);
AT(gZdebugFailHandler, 0x0056147c);
AT(gTimeGetTime, 0x00561480);
AT(gZrwWorldSetBounds, 0x00561484);
AT(gZsystemGetObject, 0x00561488);
AT(gZrwDbgFrameStart, 0x0056148c);
AT(gZrwRenderStateHook, 0x00561490);
AT(gZrwBeginHook, 0x00561494);
AT(gZrwEndHook, 0x00561498);
AT(gRwMalloc, 0x0056149c);
AT(gRwFree, 0x005614a0);
AT(gRwRealloc, 0x005614a4);
AT(gZglMemoryFuncs, 0x005614a8);
AT(gZglAllocHookNop, 0x005614ac);
AT(gZglInitPrim, 0x00561720);
AT(gZglExitPrim, 0x00561724);
AT(gZrwDrawPolys, 0x00561728);
AT(gZrwDrawTriangle, 0x0056172c);
AT(gZrwDrawSphere, 0x00561730);
AT(gNgifDrawQuads, 0x00561738);
AT(gZrwModelColor1, 0x00561818);
AT(gZrwModelColor2, 0x0056181c);
AT(gZobjLightOps, 0x00561858);
AT(gSceneModelHandlers, 0x005618dc);
AT(gSceneAnimHandlers, 0x005618e4);
AT(gZrwCamInit, 0x00561d88);
AT(gZrwCamCreate, 0x00561d90);
AT(gZrwCamDestroy, 0x00561d94);
AT(gZrwCamSetFov, 0x00561d98);
AT(gZrwCamSetView, 0x00561da0);
AT(gZrwDbgGetCamera, 0x00561db0);
AT(gZrwDbgCameraObject, 0x00561db4);
AT(gNgifSetViewport, 0x00561db8);
AT(gNgifSetViewOffset, 0x00561dc0);
AT(gZpadKeyMapFirst, 0x00561e68);
AT(gZpadKeyMapOther, 0x00561e88);
AT(gZpadDefaultButtonMap, 0x00561ea8);
AT(gZpadProfiles, 0x00561f2c);
AT(gZglInitCol, 0x00562060);
AT(gZglExitCol, 0x00562064);
AT(gZrwColLineFn, 0x00562068);
AT(gZrwColSphereFn, 0x0056206c);
AT(gZrwWorldEffectOps, 0x00562098);
AT(gZrwWorldLoad, 0x005620e0);
AT(gZrwWorldClose, 0x005620e4);
AT(gZrwWorldFind, 0x005620e8);
AT(gFxLoadTexture, 0x005620ec);
AT(gZrwLightKinds, 0x0056210c);
AT(gZrwWorldPathArgs, 0x00562128);
AT(gZrwPvsKeys, 0x0056212c);
AT(gZrwPathKey, 0x00562130);
AT(gZrwDefaultGravity, 0x005623c0);
AT(gZrwDefaultBounce, 0x005623d0);
AT(gZrwMatPluginOffset, 0x005623e8);
AT(gZglInitTexture, 0x005623f0);
AT(gZglExitTexture, 0x005623f4);
AT(gZrwTextureLoad, 0x005623fc);
AT(gZrwWorldTexDrop, 0x00562400);
AT(gZresTextureWidth, 0x00562404);
AT(gZresTextureHeight, 0x00562408);
AT(gZrwWorldTexName, 0x00562410);
AT(gZrwTextureHash, 0x00562414);
AT(gZrwTextureRetain, 0x00562420);
AT(kZrwExtBmp, 0x005625b4);
AT(kZrwExtPng, 0x005625b8);
AT(kZrwFormatDds, 0x00562764);
AT(kZrwFormatPng, 0x00562768);
AT(kZrwFormatBmp, 0x0056276c);
AT(gZrwLodDefaults, 0x00562cc4);
AT(gZrwOpenModes, 0x00563038);
AT(gZrwDeviceName, 0x00563050);
AT(gZrwLightTypes, 0x00563408);
AT(gZglInitLight, 0x00563420);
AT(gZglExitLight, 0x00563424);
AT(gZrwLightCreate, 0x00563428);
AT(gZrwLightDestroy, 0x0056342c);
AT(gZrwLightColor, 0x00563430);
AT(gZrwLightFlags, 0x00563434);
AT(gZrwLightSetParam, 0x00563438);
AT(gZrwLightDirection, 0x0056343c);
AT(kZrwBBoxFaces, 0x00563770);
AT(gZrwRamDiskDevice, 0x00563874);
AT(gZProfileColors, 0x00563904);
AT(gCrtFloatHooks, 0x00563ad0);
AT(rpClumpRegistry, 0x005667b0);
AT(rpAtomicRegistry, 0x005667c4);
AT(rpGeometryRegistry, 0x005667e0);
AT(rpMaterialRegistry, 0x00566800);
AT(rpMaterialDefaultSurfaceProps, 0x00566814);
AT(rpWorldRegistry, 0x00566820);
AT(rpWorldSectorRegistry, 0x00566860);
AT(rpLightRegistry, 0x00566880);
AT(rwD3DAtomicDefaultColor, 0x005668a0);
AT(rwD3DSectorDefaultColor, 0x005668b0);
AT(rtCharsetGlyphs, 0x005668c0);
AT(rwEngineRegistry, 0x00566dc0);
AT(rwFrameRegistry, 0x00566ee0);
AT(rwD3DRenderOrder, 0x00566f20);
AT(rwRasterRegistry, 0x00566f30);
AT(rwCameraRegistry, 0x00566f50);
AT(rwTextureRegistry, 0x00566f98);
AT(rwTexDictionaryRegistry, 0x00566fac);
AT(rwImageRegistry, 0x00566fc0);
AT(rwD3D7Device, 0x00566fe4);
#undef AT
_Static_assert(offsetof(struct BmxData, d_00569298) + sizeof(((struct BmxData *)0)->d_00569298) == 0x19000,
               "bmxData");

/* The original's .data, zero-initialised tail, 0x0056a000..0x00698f90. */
struct BmxBss {
    unsigned char pad_0056a000[5088];       /* 0x0056a000 */
    ActionState gAction;                    /* 0x0056b3e0 */
    unsigned char pad_0056b3fc[12];         /* 0x0056b3fc */
    uint8_t gCameraPlayer;                  /* 0x0056b408, also gCurrentPlayer */
    unsigned char pad_0056b409[7];          /* 0x0056b409 */
    CameraRect gCameraRect;                 /* 0x0056b410 */
    unsigned char pad_0056b418[48];         /* 0x0056b418 */
    CameraStates gCameras;                  /* 0x0056b448 */
    uint32_t gCameraReset;                  /* 0x0056b5b8 */
    unsigned char pad_0056b5bc[88];         /* 0x0056b5bc */
    uint8_t gSelLevelSetting;               /* 0x0056b614, also gCurrentLevel, gNgLevel */
    uint8_t gCurrentPlayer;                 /* 0x0056b615, also gNgPlayer */
    uint8_t gGameModeSel;                   /* 0x0056b616, also gActionSubMode */
    uint8_t gSelResume;                     /* 0x0056b617, also gOvlWarnLevel */
    uint32_t gAutoLoadDone;                 /* 0x0056b618 */
    unsigned char pad_0056b61c[4];          /* 0x0056b61c */
    uint8_t gContrlerMode;                  /* 0x0056b620 */
    unsigned char pad_0056b621[7];          /* 0x0056b621 */
    ContrlerPads gContrlerPads;             /* 0x0056b628 */
    ContrlerAxes gContrlerAxes;             /* 0x0056b6a0 */
    unsigned char d_0056b6a8[4];            /* 0x0056b6a8 */
    unsigned char d_0056b6ac[8];            /* 0x0056b6ac */
    unsigned char d_0056b6b4[16];           /* 0x0056b6b4 */
    uint32_t gFxActive;                     /* 0x0056b6c4 */
    uint32_t gFxUnused;                     /* 0x0056b6c8 */
    PhysRider *gFxRider;                    /* 0x0056b6cc */
    ZVec3 gFxZero;                          /* 0x0056b6d0 */
    unsigned char pad_0056b6dc[12];         /* 0x0056b6dc */
    GameStartPos gGameStartPos;             /* 0x0056b6e8 */
    unsigned char pad_0056b6f4[4];          /* 0x0056b6f4 */
    uint8_t gGameReplayAll;                 /* 0x0056b6f8 */
    uint8_t gGameReplayShared;              /* 0x0056b6f9 */
    unsigned char pad_0056b6fa[2];          /* 0x0056b6fa */
    GameReplayHandles gGameReplays;         /* 0x0056b6fc */
    uint8_t gGameOptions;                   /* 0x0056b700 */
    uint8_t gGameMode;                      /* 0x0056b701 */
    uint8_t gGameSubMode;                   /* 0x0056b702 */
    uint8_t gGameRunCount;                  /* 0x0056b703 */
    uint8_t gGameRunType;                   /* 0x0056b704 */
    uint8_t gGamePhase;                     /* 0x0056b705 */
    uint8_t gGamePhaseTicks;                /* 0x0056b706 */
    uint8_t gGameStartPlayer;               /* 0x0056b707 */
    uint32_t gGameTimer;                    /* 0x0056b708 */
    unsigned char d_0056b70c[24];           /* 0x0056b70c, also gGameRecords */
    unsigned char d_0056b724[32];           /* 0x0056b724 */
    unsigned char d_0056b744[8];            /* 0x0056b744 */
    unsigned char d_0056b74c[8];            /* 0x0056b74c */
    unsigned char d_0056b754[8];            /* 0x0056b754 */
    unsigned char d_0056b75c[8];            /* 0x0056b75c */
    unsigned char d_0056b764[8];            /* 0x0056b764 */
    unsigned char d_0056b76c[16];           /* 0x0056b76c */
    unsigned char d_0056b77c[656];          /* 0x0056b77c */
    GameRoundStates gGameRoundStates;       /* 0x0056ba0c, also gGameLeader */
    GameHorseWord gGameHorseWord;           /* 0x0056ba1c */
    GameMarkerList gGameMarkersA;           /* 0x0056ba28 */
    GameMarkerList gGameMarkersB;           /* 0x0056bb28 */
    GameExtent gGameExtent;                 /* 0x0056bc28 */
    uint32_t gGameFrame;                    /* 0x0056bc2c */
    uint32_t gGameFrozen;                   /* 0x0056bc30 */
    unsigned char pad_0056bc34[272];        /* 0x0056bc34 */
    OvlText4 gOvlEmpty;                     /* 0x0056bd44, also gGuiEmptyChar, rwEmptyString */
    float gOvlHeightScroll;                 /* 0x0056bd48 */
    unsigned char pad_0056bd4c[4];          /* 0x0056bd4c */
    uint32_t gOvlScoreShown;                /* 0x0056bd50 */
    unsigned char pad_0056bd54[4];          /* 0x0056bd54 */
    uint32_t gOvlScoreTarget;               /* 0x0056bd58 */
    uint8_t gOvlScoreFlags;                 /* 0x0056bd5c */
    unsigned char pad_0056bd5d[3];          /* 0x0056bd5d */
    uint8_t gOvlLastScreen;                 /* 0x0056bd60 */
    unsigned char pad_0056bd61[7];          /* 0x0056bd61 */
    uint32_t gOvlTrickCount;                /* 0x0056bd68 */
    OvlTrickList gOvlTrickList;             /* 0x0056bd6c */
    uint32_t gOvlFrame;                     /* 0x0056be14 */
    OvlTimedMsg gOvlFlash;                  /* 0x0056be18 */
    unsigned char pad_0056be1c[4];          /* 0x0056be1c */
    OvlHandles4 gOvlFlashHandles;           /* 0x0056be20 */
    uint16_t gOvlImgTiny;                   /* 0x0056be28 */
    uint8_t gOvlBackdrop;                   /* 0x0056be2a */
    unsigned char pad_0056be2b[1];          /* 0x0056be2b */
    uint16_t gOvlScoreHandle;               /* 0x0056be2c */
    unsigned char pad_0056be2e[2];          /* 0x0056be2e */
    uint16_t gOvlResA;                      /* 0x0056be30 */
    uint8_t gOvlReturnScreen;               /* 0x0056be32 */
    unsigned char pad_0056be33[1];          /* 0x0056be33 */
    OvlStringSets gOvlStrings;              /* 0x0056be34 */
    uint32_t gOvlSharedPad;                 /* 0x0056be3c */
    OvlHandleGrid gOvlRunTimes;             /* 0x0056be40 */
    uint16_t gOvlIconHandle;                /* 0x0056be90 */
    uint16_t gOvlImgIcons;                  /* 0x0056be92 */
    uint16_t gPhysHudHandle;                /* 0x0056be94 */
    unsigned char pad_0056be96[2];          /* 0x0056be96 */
    OvlFont gOvlFontTiny;                   /* 0x0056be98 */
    OvlHandles7 gOvlVolHandles;             /* 0x0056bea4 */
    unsigned char d_0056beb2[2];            /* 0x0056beb2 */
    OvlHandles3 gOvlGoalHandles;            /* 0x0056beb4 */
    uint8_t gOvlScreenId;                   /* 0x0056beba */
    unsigned char pad_0056bebb[1];          /* 0x0056bebb */
    unsigned char d_0056bebc[2];            /* 0x0056bebc, also gOvlHandles */
    unsigned char d_0056bebe[2];            /* 0x0056bebe */
    unsigned char d_0056bec0[2];            /* 0x0056bec0 */
    unsigned char d_0056bec2[2];            /* 0x0056bec2 */
    OvlGoalRows gOvlGoalRows;               /* 0x0056bec4 */
    uint8_t gOvlCaptionCurrent;             /* 0x0056beec */
    uint8_t gOvlCaptionWanted;              /* 0x0056beed */
    unsigned char pad_0056beee[6];          /* 0x0056beee */
    uint16_t gOvlImgSmall;                  /* 0x0056bef4 */
    unsigned char pad_0056bef6[2];          /* 0x0056bef6 */
    uint16_t gOvlBackdropRect;              /* 0x0056bef8 */
    unsigned char pad_0056befa[2];          /* 0x0056befa */
    OvlMenuItems7 gOvlMenuItems;            /* 0x0056befc */
    float gOvlBalanceValue;                 /* 0x0056bf18 */
    unsigned char pad_0056bf1c[4];          /* 0x0056bf1c */
    OvlScreen *gOvlScreen;                  /* 0x0056bf20 */
    OvlHandles6 gOvlHeightHandles;          /* 0x0056bf24 */
    uint8_t gOvlPage;                       /* 0x0056bf30 */
    unsigned char pad_0056bf31[7];          /* 0x0056bf31 */
    uint8_t gPhysHudFont;                   /* 0x0056bf38, also gOvlFontSmall */
    unsigned char pad_0056bf39[11];         /* 0x0056bf39 */
    OvlTimedMsg gOvlHint;                   /* 0x0056bf44 */
    unsigned char pad_0056bf48[4];          /* 0x0056bf48 */
    OvlTimedMsg gOvlGoalMsg;                /* 0x0056bf4c */
    unsigned char pad_0056bf50[4];          /* 0x0056bf50 */
    uint32_t gOvlTimerShownTicks;           /* 0x0056bf54 */
    uint32_t gOvlTimerTicks;                /* 0x0056bf58 */
    OvlIndicator gOvlIndicator;             /* 0x0056bf5c */
    unsigned char pad_0056bf60[4];          /* 0x0056bf60 */
    uint8_t gOvlNameCursor;                 /* 0x0056bf64 */
    unsigned char pad_0056bf65[3];          /* 0x0056bf65 */
    uint32_t gOvlPulse;                     /* 0x0056bf68 */
    OvlHandles21 gOvlTrickRows;             /* 0x0056bf6c, also gOvlTrickDesc */
    unsigned char d_0056bf96[2];            /* 0x0056bf96 */
    OvlFont gOvlFontIcons;                  /* 0x0056bf98 */
    uint32_t gOvlDrawDelay;                 /* 0x0056bfa4 */
    uint16_t gOvlStatusHandle;              /* 0x0056bfa8 */
    unsigned char pad_0056bfaa[2];          /* 0x0056bfaa */
    OvlText8 gOvlTimerText;                 /* 0x0056bfac */
    unsigned char d_0056bfb4[1];            /* 0x0056bfb4, also gOvlNameBuf */
    unsigned char d_0056bfb5[11];           /* 0x0056bfb5 */
    float gOvlHeightValue;                  /* 0x0056bfc0 */
    float gOvlHeightTarget;                 /* 0x0056bfc4 */
    unsigned char d_0056bfc8[2];            /* 0x0056bfc8, also gOvlHeightExtra */
    uint16_t gOvlHeightKind;                /* 0x0056bfca */
    unsigned char pad_0056bfcc[13];         /* 0x0056bfcc */
    uint8_t gOvlElement8Value;              /* 0x0056bfd9 */
    unsigned char pad_0056bfda[2];          /* 0x0056bfda */
    OvlHandles7 gOvlHintHandles;            /* 0x0056bfdc */
    unsigned char d_0056bfea[2];            /* 0x0056bfea */
    OvlHandles7 gOvlBalanceHandles;         /* 0x0056bfec */
    uint16_t gOvlImgButton;                 /* 0x0056bffa */
    uint16_t gOvlCaptionHandle;             /* 0x0056bffc */
    unsigned char pad_0056bffe[2];          /* 0x0056bffe */
    uint16_t gOvlTimerHandle;               /* 0x0056c000 */
    unsigned char pad_0056c002[6];          /* 0x0056c002 */
    OvlFont gOvlFontButton;                 /* 0x0056c008 */
    unsigned char pad_0056c014[4];          /* 0x0056c014 */
    OvlStatusMsg gOvlStatus;                /* 0x0056c018 */
    unsigned char pad_0056c020[8];          /* 0x0056c020 */
    OvlMenuItems10 gOvlMenuItems2;          /* 0x0056c028 */
    OvlFade gOvlFade;                       /* 0x0056c050 */
    OvlTrickView gOvlTrickView;             /* 0x0056c070 */
    uint8_t gOvlSaveSlot;                   /* 0x0056c074 */
    unsigned char pad_0056c075[7];          /* 0x0056c075 */
    uint32_t gPhysTick;                     /* 0x0056c07c */
    unsigned char pad_0056c080[168];        /* 0x0056c080 */
    PhysRayHit gPhysRay;                    /* 0x0056c128 */
    PhysEdge gPhysBestEdge;                 /* 0x0056c200 */
    PhysEdge gPhysAlignEdge;                /* 0x0056c238 */
    unsigned char pad_0056c270[80];         /* 0x0056c270 */
    PhysEdge gPhysNearEdge;                 /* 0x0056c2c0 */
    PhysEdge gPhysEdgeProbe;                /* 0x0056c2f8 */
    PhysEdge gPhysGrindEdge;                /* 0x0056c330 */
    unsigned char pad_0056c368[80];         /* 0x0056c368 */
    PhysFace gPhysLipFace;                  /* 0x0056c3b8 */
    int32_t gPhysForceStep;                 /* 0x0056c440 */
    int32_t gPhysDebugDraw;                 /* 0x0056c444, also gPhysDebugContact */
    PhysEdge **gPhysDebugEdges;             /* 0x0056c448 */
    uint32_t gPhysDebugFrame;               /* 0x0056c44c */
    unsigned char pad_0056c450[168];        /* 0x0056c450 */
    PropState gPropDefaults;                /* 0x0056c4f8 */
    unsigned char pad_0056d038[576];        /* 0x0056d038 */
    ReplayTable gReplays;                   /* 0x0056d278 */
    unsigned char d_0056d28c[4];            /* 0x0056d28c */
    uint32_t gGuiMarkupTarget;              /* 0x0056d290 */
    unsigned char pad_0056d294[4];          /* 0x0056d294 */
    GuiPad gGuiPad;                         /* 0x0056d298 */
    GuiElementArray gGuiElements;           /* 0x0056d2c0 */
    uint16_t gGuiScreenId;                  /* 0x00573080 */
    unsigned char pad_00573082[6];          /* 0x00573082 */
    GuiScreenState gGuiScreen;              /* 0x00573088 */
    GuiHookFn gGuiDefaultHook;              /* 0x005730a0 */
    GuiElementPtrArray gGuiSorted;          /* 0x005730a4 */
    int gGuiInitialized;                    /* 0x00573234 */
    int gGuiModeReady;                      /* 0x00573238 */
    int gGuiElementCount;                   /* 0x0057323c */
    int gGuiScreenLoaded;                   /* 0x00573240 */
    int gGuiReloadPending;                  /* 0x00573244 */
    int gGuiReloadDelay;                    /* 0x00573248 */
    int gGuiSkipUpdate;                     /* 0x0057324c */
    int gGuiAssetsLoaded;                   /* 0x00573250, also gNgGuiReady */
    int gGuiPulse;                          /* 0x00573254 */
    int gGuiPulseHold;                      /* 0x00573258 */
    int gGuiSlideState;                     /* 0x0057325c */
    int gGuiSlideMax;                       /* 0x00573260 */
    unsigned char pad_00573264[4896];       /* 0x00573264 */
    int gOptMainPulse;                      /* 0x00574584, also gExtrasPulse */
    SelStepFn gSelPreviewFn;                /* 0x00574588 */
    FrontendStepFn gFrontendStep;           /* 0x0057458c */
    SelStepFn gSelCommitFn;                 /* 0x00574590 */
    ZPrimAsset *gMenuFrameSprite;           /* 0x00574594 */
    unsigned char pad_00574598[4];          /* 0x00574598 */
    McDoneFn gMcDone;                       /* 0x0057459c */
    uint16_t gTitleSel;                     /* 0x005745a0 */
    unsigned char pad_005745a2[2];          /* 0x005745a2 */
    TitleActionFn gTitleAction;             /* 0x005745a4 */
    uint32_t gAutoLoadOk;                   /* 0x005745a8 */
    uint16_t gAutoTick;                     /* 0x005745ac */
    uint16_t gTitleStage;                   /* 0x005745ae */
    unsigned char pad_005745b0[4];          /* 0x005745b0 */
    KeyWork gKbWork;                        /* 0x005745b4 */
    uint8_t gKbCursor;                      /* 0x005745e0 */
    uint8_t gKbState;                       /* 0x005745e1 */
    uint8_t gKbLayout;                      /* 0x005745e2 */
    unsigned char pad_005745e3[9];          /* 0x005745e3 */
    SelStyleSprites gStyleSprites;          /* 0x005745ec */
    int gStyleFrame;                        /* 0x0057462c */
    unsigned char pad_00574630[20];         /* 0x00574630 */
    uint16_t gExtrasSel;                    /* 0x00574644 */
    unsigned char pad_00574646[2];          /* 0x00574646 */
    uint32_t gExtrasPlayed;                 /* 0x00574648 */
    void *gAssetSetData;                    /* 0x0057464c */
    unsigned char pad_00574650[4];          /* 0x00574650 */
    int gExitFlag;                          /* 0x00574654, also gBmxMenuExit */
    int gExitCode;                          /* 0x00574658, also gBmxMenuExitResult */
    void *gMacroBuf;                        /* 0x0057465c */
    uint32_t gMacroSize;                    /* 0x00574660 */
    unsigned char pad_00574664[4];          /* 0x00574664 */
    uint8_t gBmxCurrentTrack;               /* 0x00574668 */
    unsigned char pad_00574669[3];          /* 0x00574669 */
    unsigned char d_0057466c[4];            /* 0x0057466c, also gBmxRiderLoopB */
    unsigned char d_00574670[4];            /* 0x00574670, also gBmxRiderLoopA */
    uint32_t gBmxTimerId;                   /* 0x00574674 */
    BmxAmbientTrackTable gBmxAmbientTracks; /* 0x00574678 */
    WinchanEffect gBmxSoundEffect;          /* 0x00574680 */
    uint32_t gBmxTimerPeriod;               /* 0x00574694 */
    uint8_t gBmxGunChannel;                 /* 0x00574698 */
    unsigned char pad_00574699[3];          /* 0x00574699 */
    BmxMenuChannelTable gBmxMenuChannels;   /* 0x0057469c */
    uint8_t gBmxLevelIndex;                 /* 0x0057469e */
    uint8_t gBmxRiderIndex;                 /* 0x0057469f */
    uint8_t gBmxAmbientSide;                /* 0x005746a0 */
    unsigned char pad_005746a1[3];          /* 0x005746a1 */
    struct WinstrmStream *gBmxMusicStream;  /* 0x005746a4 */
    uint32_t gBmxTrackCooldown;             /* 0x005746a8 */
    uint8_t gBmxMenuSfxSet;                 /* 0x005746ac */
    unsigned char pad_005746ad[3];          /* 0x005746ad */
    uint32_t gLastPollTime;                 /* 0x005746b0 */
    unsigned char pad_005746b4[16];         /* 0x005746b4 */
    ZVec3 gZoverArmSide;                    /* 0x005746c4 */
    unsigned char pad_005746d0[4];          /* 0x005746d0 */
    ZVec3 gZoverLimb3Elbow;                 /* 0x005746d4 */
    unsigned char pad_005746e0[4];          /* 0x005746e0 */
    ZMatrix gZoverLimb1Upper;               /* 0x005746e4 */
    ZVec3 gZoverLimb0Elbow;                 /* 0x00574724 */
    unsigned char pad_00574730[4];          /* 0x00574730 */
    ZoverRigFlags gZoverRelaxed;            /* 0x00574734 */
    ZoverRigFlags gZoverStiff;              /* 0x0057473c */
    ZVec3 gZoverLimb3Target;                /* 0x00574744 */
    unsigned char pad_00574750[4];          /* 0x00574750 */
    ZMatrix gZoverLimb0Lower;               /* 0x00574754 */
    ZMatrix gZoverFrameShin46;              /* 0x00574794 */
    ZVec3 gZoverLimb0Bend;                  /* 0x005747d4 */
    unsigned char pad_005747e0[4];          /* 0x005747e0 */
    ZVec3 gZoverLimb0Target;                /* 0x005747e4 */
    unsigned char pad_005747f0[4];          /* 0x005747f0 */
    ZMatrix gZoverFrameShin35;              /* 0x005747f4 */
    ZVec3 gZoverLimb1Target;                /* 0x00574834 */
    unsigned char pad_00574840[4];          /* 0x00574840 */
    ZVec3 gZoverLimb2Target;                /* 0x00574844 */
    unsigned char pad_00574850[4];          /* 0x00574850 */
    ZMatrix gZoverFrameForearm911;          /* 0x00574854 */
    ZMatrix gZoverSpineFrame;               /* 0x00574894 */
    ZoverRigFlags gZoverHandsEased;         /* 0x005748d4 */
    ZVec3 gZoverLimb2Root;                  /* 0x005748dc */
    unsigned char pad_005748e8[4];          /* 0x005748e8 */
    ZVec3 gZoverLimb2Elbow;                 /* 0x005748ec */
    unsigned char pad_005748f8[4];          /* 0x005748f8 */
    ZVec3 gZoverLimb1Elbow;                 /* 0x005748fc */
    unsigned char pad_00574908[4];          /* 0x00574908 */
    ZVec3 gZoverLimb3Bend;                  /* 0x0057490c */
    unsigned char pad_00574918[4];          /* 0x00574918 */
    ZoverRigFlags gZoverHandsGrabbed;       /* 0x0057491c */
    ZoverRigFlags gZoverHandsHolding;       /* 0x00574924 */
    float gZoverArmDist;                    /* 0x0057492c */
    ZoverRigFlags gZoverLimp;               /* 0x00574930 */
    ZMatrix gZoverFrameForearm1012;         /* 0x00574938 */
    ZMatrix gZoverLimb3Upper;               /* 0x00574978 */
    ZVec3 gZoverLimb1Root;                  /* 0x005749b8 */
    unsigned char pad_005749c4[4];          /* 0x005749c4 */
    ZMatrix gZoverLimb2Upper;               /* 0x005749c8 */
    float gZoverLimbUpperLen;               /* 0x00574a08 */
    float gZoverLimbSin;                    /* 0x00574a0c */
    ZVec3 gZoverLimb1Bend;                  /* 0x00574a10 */
    unsigned char pad_00574a1c[4];          /* 0x00574a1c */
    ZVec3 gZoverLegSide;                    /* 0x00574a20 */
    unsigned char pad_00574a2c[4];          /* 0x00574a2c */
    ZVec3 gZoverLimb0Root;                  /* 0x00574a30 */
    unsigned char pad_00574a3c[4];          /* 0x00574a3c */
    ZAngle gZoverLimbAngle;                 /* 0x00574a40 */
    unsigned char pad_00574a42[2];          /* 0x00574a42 */
    ZVec3 gZoverLimb3Root;                  /* 0x00574a44 */
    unsigned char pad_00574a50[4];          /* 0x00574a50 */
    ZMatrix gZoverFrameHead;                /* 0x00574a54 */
    ZMatrix gZoverLimb1Lower;               /* 0x00574a94 */
    ZoverRigFlags gZoverSticky;             /* 0x00574ad4 */
    ZMatrix gZoverActorFrame;               /* 0x00574adc */
    ZoverRigFlags gZoverYawOn;              /* 0x00574b1c */
    ZoverRigFlags gZoverPitchOn;            /* 0x00574b24 */
    ZMatrix gZoverLimbProduct;              /* 0x00574b2c */
    ZVec3 gZoverLimbDir;                    /* 0x00574b6c */
    unsigned char pad_00574b78[4];          /* 0x00574b78 */
    float gZoverLegDist;                    /* 0x00574b7c */
    float gZoverLimbCos;                    /* 0x00574b80 */
    ZMatrix gZoverLimb3Lower;               /* 0x00574b84 */
    ZoverRigTable gZoverRigs;               /* 0x00574bc4 */
    float gZoverLimbDistCopy;               /* 0x00574bcc */
    ZMatrix gZoverLimb2Lower;               /* 0x00574bd0 */
    ZMatrix gZoverLimbTransposed;           /* 0x00574c10 */
    float gZoverLimbLowerLen;               /* 0x00574c50 */
    ZVec3 gZoverLimb2Bend;                  /* 0x00574c54 */
    unsigned char pad_00574c60[4];          /* 0x00574c60 */
    ZMatrix gZoverLimb0Upper;               /* 0x00574c64 */
    int32_t gZoverRigNext;                  /* 0x00574ca4 */
    int32_t gZoverRigIndex;                 /* 0x00574ca8 */
    ZoverRigAngles gZoverPitch;             /* 0x00574cac */
    ZoverRigAngles gZoverYaw;               /* 0x00574cb0 */
    ZoverIndices6 gZoverHingeIdx;           /* 0x00574cb4 */
    ZoverIndices8 gZoverStickIdx;           /* 0x00574ccc */
    ZoverIndices3 gZoverBackIdx;            /* 0x00574cec */
    ZVec3 gZoverLimbOffset;                 /* 0x00574cf8 */
    unsigned char pad_00574d04[4];          /* 0x00574d04 */
    int32_t gZoverLimbSynced;               /* 0x00574d08 */
    unsigned char pad_00574d0c[324];        /* 0x00574d0c */
    ZOverFontTable gZoverFonts;             /* 0x00574e50 */
    ZOverPool3 gZoverPool3;                 /* 0x00574e70 */
    ZOverPool2 gZoverPool2;                 /* 0x00594270 */
    ZOverRectSize gZoverRectSize;           /* 0x00596570 */
    unsigned char pad_00596578[42];         /* 0x00596578 */
    unsigned char d_005965a2[2];            /* 0x005965a2 */
    ZOverHandleList gZoverFreeList;         /* 0x005965a4 */
    unsigned char pad_00596a1c[4];          /* 0x00596a1c */
    ZOverTerminators gZoverTerminators;     /* 0x00596a20 */
    uint8_t gZoverFontCount;                /* 0x00596a30 */
    unsigned char pad_00596a31[7];          /* 0x00596a31 */
    ZOverPool1 gZoverPool1;                 /* 0x00596a38 */
    ZOverPool0 gZoverPool0;                 /* 0x005a0a38 */
    ZOverHandleList gZoverDirtyList;        /* 0x005a1e38 */
    uint32_t gZoverBusy;                    /* 0x005a22b0 */
    uint32_t gZoverDirtyCount;              /* 0x005a22b4 */
    uint32_t gZoverFreeCount;               /* 0x005a22b8 */
    uint8_t gZoverFrame;                    /* 0x005a22bc */
    uint8_t gZoverLayer;                    /* 0x005a22bd */
    unsigned char pad_005a22be[37090];      /* 0x005a22be */
    uint8_t gZsystemMode;                   /* 0x005ab3a0, also gGuiAnalogPad, gNgMode */
    unsigned char pad_005ab3a1[11];         /* 0x005ab3a1 */
    void *gNgSpotLights[3];                 /* 0x005ab3ac; the type is local to bmx/com/zge/ngif/ngapp.c */
    char gNgArtName;                        /* 0x005ab3b8 */
    unsigned char pad_005ab3b9[127];        /* 0x005ab3b9 */
    void *gNgLights[3];                     /* 0x005ab438; the type is local to bmx/com/zge/ngif/ngapp.c */
    unsigned char pad_005ab444[16];         /* 0x005ab444 */
    float gNgElapsed;                       /* 0x005ab454, also gFxDebugFloat */
    void *gNgMainLight;                     /* 0x005ab458 */
    unsigned char pad_005ab45c[4];          /* 0x005ab45c */
    uint32_t gNgWake;                       /* 0x005ab460 */
    unsigned char pad_005ab464[4];          /* 0x005ab464 */
    uint32_t *gNgPrevMark;                  /* 0x005ab468 */
    void *gNgArt;                           /* 0x005ab46c */
    ZrwUpdateNode *gNgLoadNode;             /* 0x005ab470 */
    int gMenuInstant;                       /* 0x005ab474, also gGuiIdleMode, gNgFading */
    uint32_t gNgCountdown;                  /* 0x005ab478 */
    uint32_t gNgSunTick;                    /* 0x005ab47c */
    uint16_t gNgLoadFlag;                   /* 0x005ab480 */
    unsigned char pad_005ab482[6];          /* 0x005ab482 */
    void *gNgLegal1;                        /* 0x005ab488 */
    void *gNgLoadTexture;                   /* 0x005ab48c */
    void *gNgLegal2;                        /* 0x005ab490 */
    unsigned char pad_005ab494[8];          /* 0x005ab494 */
    uint32_t gFxQualityTime;                /* 0x005ab49c */
    int32_t gFxFrames;                      /* 0x005ab4a0 */
    int32_t gFxAdjusting;                   /* 0x005ab4a4 */
    float gFxQualityStep;                   /* 0x005ab4a8 */
    unsigned char pad_005ab4ac[52];         /* 0x005ab4ac */
    uint32_t gFxSparkTexture;               /* 0x005ab4e0 */
    uint32_t gFxSkidTexture;                /* 0x005ab4e4 */
    uint32_t gFxRippleTexture;              /* 0x005ab4e8 */
    uint32_t gFxSplashTexture;              /* 0x005ab4ec */
    const int8_t *gNgifScript;              /* 0x005ab4f0 */
    void *gFxCube;                          /* 0x005ab4f4 */
    unsigned char pad_005ab4f8[4];          /* 0x005ab4f8 */
    NgifWords gNgifPrimBuffer;              /* 0x005ab4fc */
    unsigned char pad_005ab500[159740];     /* 0x005ab500 */
    int gNgifFlushPending;                  /* 0x005d24fc */
    int gNgifFlushNode;                     /* 0x005d2500 */
    int gNgifFlushPending2;                 /* 0x005d2504, also gZprimFrameEnded */
    int gNgifFlushNode2;                    /* 0x005d2508 */
    uint32_t *gNgifPrimStart;               /* 0x005d250c */
    uint8_t *gPrimCursor;                   /* 0x005d2510, also gNgifPrim */
    unsigned char pad_005d2514[31956];      /* 0x005d2514 */
    ZactorPoseTables gZactorPose;           /* 0x005da1e8 */
    unsigned char pad_005da2c0[8];          /* 0x005da2c0 */
    ZactorArmHashes gZactorArmHashes;       /* 0x005da2c8 */
    unsigned char pad_005da2e0[49400];      /* 0x005da2e0 */
    ZbspNearHitTable gZbspNearHits;         /* 0x005e63d8 */
    uint32_t gZbspMatRefValue;              /* 0x005e7fd8 */
    unsigned char pad_005e7fdc[4];          /* 0x005e7fdc */
    ZbspPropFaceTable gZbspPropFaces;       /* 0x005e7fe0 */
    unsigned char pad_005e8068[952];        /* 0x005e8068 */
    ZbspNearFaceTable gZbspNearFaces;       /* 0x005e8420 */
    unsigned char d_005f0b98[136];          /* 0x005f0b98 */
    ZbspSphereResult gZbspSphere;           /* 0x005f0c20 */
    ZbspFaceQuery gZbspFaceQuery;           /* 0x005f0c28 */
    ZbspHitList gZbspHits;                  /* 0x005f0c68 */
    unsigned char pad_005f106c[4];          /* 0x005f106c */
    PhysEdge gZbspEdgeRecord;               /* 0x005f1070 */
    ZbspNearCornerTable gZbspNearCorners;   /* 0x005f10a8 */
    unsigned char pad_005f3484[40];         /* 0x005f3484 */
    struct ZMemFreeList *gZresListA;        /* 0x005f34ac, also gZbspFacePool */
    struct ZMemFreeList *gZresListB;        /* 0x005f34b0, also gZbspEdgePool */
    FxLightFn gFxLightCallback;             /* 0x005f34b4 */
    FxListFn gFxListCallback;               /* 0x005f34b8 */
    uint32_t gZbspPropFaceCount;            /* 0x005f34bc */
    unsigned char pad_005f34c0[20568];      /* 0x005f34c0 */
    struct {
        uint32_t flags;
        uint32_t extra;
    } gZcollideProbeMaterial;      /* 0x005f8518; the type is local to bmx/com/zge/ngif/ngifzcollide.c */
    void *gZcollideEdgeList[32];   /* 0x005f8520; the type is local to bmx/com/zge/ngif/ngifzcollide.c */
    unsigned char pad_005f85a0[8]; /* 0x005f85a0 */
    struct {
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
    } gZcollideEdgeRecords[33];       /* 0x005f85a8; the type is local to bmx/com/zge/ngif/ngifzcollide.c */
    void *gZcollideEdgeCells[34];     /* 0x005f8ce0; the type is local to bmx/com/zge/ngif/ngifzcollide.c */
    uint8_t gZbspNoShapeKind;         /* 0x005f8d68 */
    unsigned char pad_005f8d69[455];  /* 0x005f8d69 */
    ZmemdmmState gZmemdmm;            /* 0x005f8f30 */
    unsigned char pad_005f8f58[8];    /* 0x005f8f58 */
    ZresState gZres;                  /* 0x005f8f60 */
    unsigned char pad_005fb034[1684]; /* 0x005fb034 */
    ZrollerDrawTable gZrollerDrawCallbacks; /* 0x005fb6c8 */
    int gZrollerReserved;                   /* 0x005fb6e8 */
    unsigned char pad_005fb6ec[4];          /* 0x005fb6ec */
    ZrollerSlotTable gZrollerSlots;         /* 0x005fb6f0 */
    int32_t gZrollerShadowSlots;            /* 0x005fb710 */
    unsigned char pad_005fb714[4];          /* 0x005fb714 */
    ZrollerBounds gZrollerBounds;           /* 0x005fb718 */
    unsigned char pad_005fb734[4];          /* 0x005fb734 */
    int gZrollerSlotCount;                  /* 0x005fb738 */
    uint32_t gNgWord5fb73c;                 /* 0x005fb73c */
    ZrwUpdateNode *gNgRollerNode;           /* 0x005fb740, also gZrollerUpdateHandle */
    unsigned char pad_005fb744[28];         /* 0x005fb744 */
    DsBuffer *gWinchanBuffer;               /* 0x005fb760 */
    int32_t gWinchanFlangerDelay;           /* 0x005fb764 */
    int32_t gWinchanActive;                 /* 0x005fb768 */
    uint32_t gWinchanEffectMaxFrames;       /* 0x005fb76c */
    WinchanChannelTable gWinchanChannels;   /* 0x005fb770 */
    int32_t gWinchanMixRight;               /* 0x005fbe30 */
    float gWinchanEffectAGain;              /* 0x005fbe34 */
    unsigned char d_005fbe38[1];            /* 0x005fbe38, also gWinchanHandles */
    unsigned char d_005fbe39[127];          /* 0x005fbe39 */
    int32_t gWinchanMixLeft;                /* 0x005fbeb8 */
    WinchanFlangerLine gWinchanFlangerLine; /* 0x005fbebc */
    uint32_t gWinchanCopyOffset;            /* 0x005fdebc */
    uint32_t gWinchanWriteOffset;           /* 0x005fdec0 */
    int32_t gWinchanEchoPos;                /* 0x005fdec4 */
    WinchanLfoTable gWinchanLfoTable;       /* 0x005fdec8 */
    unsigned char d_005ffec8[4];            /* 0x005ffec8 */
    int32_t gWinchanFlangerPhase;           /* 0x005ffecc */
    WinchanFrame *gWinchanEchoBuffer;       /* 0x005ffed0 */
    void *gWinchanDsoundModule;             /* 0x005ffed4 */
    uint8_t *gWinchanRing;                  /* 0x005ffed8 */
    uint32_t gWinchanChunkBytes;            /* 0x005ffedc */
    uint32_t gWinchanEffectBFrames;         /* 0x005ffee0 */
    uint32_t gWinchanRate;                  /* 0x005ffee4 */
    float gWinchanEffectBGain;              /* 0x005ffee8 */
    uint32_t gWinchanEffectAFrames;         /* 0x005ffeec */
    float gWinchanEchoFeedback;             /* 0x005ffef0, also gWinchanEffectAParam */
    uint32_t gWinchanMixOffset;             /* 0x005ffef4 */
    uint32_t gWinchanLastPlay;              /* 0x005ffef8 */
    int32_t gWinchanEffectBTailStart;       /* 0x005ffefc */
    int32_t gWinchanFlangerPos;             /* 0x005fff00 */
    int32_t gWinchanEffectBTail;            /* 0x005fff04 */
    uint32_t gWinchanRingBytes;             /* 0x005fff08 */
    int32_t gWinchanInitialized;            /* 0x005fff0c */
    int32_t gWinchanLock;                   /* 0x005fff10 */
    int32_t gWinchanNextChannel;            /* 0x005fff14 */
    int32_t gWinchanNextHandle;             /* 0x005fff18 */
    void *gWinchanDsoundCreate;             /* 0x005fff1c */
    DsBuffer *gWinchanPrimary;              /* 0x005fff20 */
    int32_t gWinchanPumpCount;              /* 0x005fff24 */
    DsDevice *gWinstrmDirectSound;          /* 0x005fff28, also gWinchanDirectSound */
    unsigned char pad_005fff2c[3676];       /* 0x005fff2c */
    WinloadSampleFile gWinloadSampleFile;   /* 0x00600d88 */
    unsigned char pad_00600d94[4];          /* 0x00600d94 */
    WinloadFileTable gWinloadFiles;         /* 0x00600d98 */
    WinloadCounts gWinloadCounts;           /* 0x00601218 */
    unsigned char pad_00601234[244];        /* 0x00601234 */
    int32_t gWinstrmSourceHint;             /* 0x00601328 */
    unsigned char pad_0060132c[4];          /* 0x0060132c */
    WinstrmLockStorage gWinstrmLock;        /* 0x00601330 */
    int32_t gWinstrmStreamHint;             /* 0x00601348 */
    unsigned char pad_0060134c[4];          /* 0x0060134c */
    WinstrmSourceTable gWinstrmSources;     /* 0x00601350 */
    WinstrmStreamTable gWinstrmStreams;     /* 0x006017d0 */
    int32_t gWinstrmNodeHint;               /* 0x006019f0 */
    unsigned char pad_006019f4[4];          /* 0x006019f4 */
    WinstrmNodeTable gWinstrmNodes;         /* 0x006019f8 */
    int32_t gWinstrmInitialized;            /* 0x00601b78 */
    int32_t gWinstrmPaused;                 /* 0x00601b7c */
    int32_t gWinstrmTick;                   /* 0x00601b80 */
    unsigned char pad_00601b84[428];        /* 0x00601b84 */
    ZActorRootPose gActorRootPoseTemp;      /* 0x00601d30 */
    ZMotionDelta gMotionLastDelta;          /* 0x00601d48 */
    ZMotionEntry gMotionEntryTemp;          /* 0x00601d60 */
    ZMotionDelta gMotionDeltaA;             /* 0x00601d68 */
    ZMotionDelta gMotionDeltaB;             /* 0x00601d80 */
    ZMotionDelta gMotionDeltaC;             /* 0x00601d98 */
    ZMotionDelta gMotionDeltaD;             /* 0x00601db0 */
    unsigned char d_00601dc8[24];           /* 0x00601dc8, also gMotionBanks */
    ZMotionDelta gMotionDeltaE;             /* 0x00601de0 */
    unsigned char pad_00601df8[16];         /* 0x00601df8 */
    int16_t gMotionPrevYawE;                /* 0x00601e08 */
    unsigned char pad_00601e0a[6];          /* 0x00601e0a */
    void *gMotionFirstBank;                 /* 0x00601e10 */
    ZMotionDelta *gMotionSampleA;           /* 0x00601e14 */
    ZMotionDelta *gMotionSampleB;           /* 0x00601e18 */
    ZMotionStaging gMotionStaging;          /* 0x00601e1c */
    ZActor *gZactors;                       /* 0x00641e1c */
    int32_t gMotionBankCount;               /* 0x00641e20 */
    int32_t gActorAngleBase;                /* 0x00641e24 */
    int32_t gActorAngleFlip;                /* 0x00641e28 */
    int32_t gMotionLibrary;                 /* 0x00641e2c */
    void *gMotionBuffer;                    /* 0x00641e30 */
    unsigned char pad_00641e34[52480];      /* 0x00641e34 */
    uint32_t gZprimTextureCounter;          /* 0x0064eb34 */
    ZsndNotePool gZsndNotes;                /* 0x0064eb38 */
    int32_t gZsndNextNote;                  /* 0x0064f038 */
    int32_t gZsndMuteAll;                   /* 0x0064f03c */
    int32_t gZsndNextTrack;                 /* 0x0064f040 */
    unsigned char pad_0064f044[4];          /* 0x0064f044 */
    ZsndTrackPool gZsndTrackPool;           /* 0x0064f048 */
    ZsndTrackTable gZsndTracks;             /* 0x0064f948 */
    int32_t gZsndSeqLock;                   /* 0x0064f9c8 */
    unsigned char pad_0064f9cc[748];        /* 0x0064f9cc */
    WinstrmStream *gZsndStream;             /* 0x0064fcb8 */
    uint32_t gZsndStreamBytes;              /* 0x0064fcbc */
    unsigned char pad_0064fcc0[8];          /* 0x0064fcc0 */
    int32_t gZsndSrcNoDoppler;              /* 0x0064fcc8 */
    uint32_t gZsndSrcRange;                 /* 0x0064fccc */
    ZsndSourceTable gZsndSources;           /* 0x0064fcd0 */
    float gZsndSrcDopplerScale;             /* 0x00650090 */
    uint32_t gZsndSrcRange2;                /* 0x00650094 */
    int32_t gZsndSrcInitialized;            /* 0x00650098 */
    uint32_t gZsndSrcNext;                  /* 0x0065009c */
    uint32_t gZsndSrcLifetime;              /* 0x006500a0 */
    unsigned char pad_006500a4[12];         /* 0x006500a4 */
    unsigned char d_006500b0[192];          /* 0x006500b0 */
    int gZdebugCreated;                     /* 0x00650170 */
    unsigned char d_00650174[4];            /* 0x00650174 */
    unsigned char d_00650178[8];            /* 0x00650178 */
    struct {
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
    } gScratch;                       /* 0x00650180; the type is local to zge/com/zglrw/zrwdbg.c */
    unsigned char pad_0065046c[4];    /* 0x0065046c */
    int gZrwDbgInitialized;           /* 0x00650470 */
    uint8_t gZrwDbgFpsMode;           /* 0x00650474 */
    unsigned char pad_00650475[3];    /* 0x00650475 */
    ZrwUpdateNode *gZrwDbgUpdateNode; /* 0x00650478 */
    ZrwList gZrwDbgCallbacks;         /* 0x0065047c */
    void *gZrwDbgCharset;             /* 0x00650488 */
    int gZrwDbgCharWidth;             /* 0x0065048c */
    int gZrwDbgCharHeight;            /* 0x00650490 */
    char gFormatBuffer[512];          /* 0x00650494; the type is local to zge/com/zglrw/zrwdbg.c */
    uint8_t gTextQueue[1024];         /* 0x00650694; the type is local to zge/com/zglrw/zrwdbg.c */
    uint8_t *gTextEnd;                /* 0x00650a94 */
    int gZrwDbgItemCount;             /* 0x00650a98 */
    struct {
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
    } gItems[512];                       /* 0x00650a9c; the type is local to zge/com/zglrw/zrwdbg.c */
    int gZdebugIndentLevel;              /* 0x0065e29c, also gDebugIndent */
    ZrwDbgVertex *gSphereVerts;          /* 0x0065e2a0 */
    ZrwDbgVertex **gSpherePointers;      /* 0x0065e2a4 */
    int gZinputReady;                    /* 0x0065e2a8 */
    ZMemFreeList *gZpadDevices;          /* 0x0065e2ac */
    ZMemFreeList *gZassetLibraries;      /* 0x0065e2b0 */
    unsigned char pad_0065e2b4[8];       /* 0x0065e2b4 */
    ZMathTables gZmathTables;            /* 0x0065e2bc */
    unsigned char pad_006702d4[4];       /* 0x006702d4 */
    ZVec3 gZmathOrigin;                  /* 0x006702d8, also gPhysZero */
    unsigned char pad_006702e4[20];      /* 0x006702e4 */
    int gZobjCameraInitialized;          /* 0x006702f8 */
    ZMemFreeList *gZobjCameraCaches;     /* 0x006702fc */
    ZVec3 gZobjCameraTarget;             /* 0x00670300 */
    unsigned char pad_0067030c[4];       /* 0x0067030c */
    unsigned char d_00670310[200];       /* 0x00670310 */
    ZstrTempBuffer gZstrTempBuffer;      /* 0x006703d8 */
    ZStringSetTable gZstringSets;        /* 0x006704d8 */
    int gNameTablesReady;                /* 0x00670518 */
    ZNameTables gNameTables;             /* 0x0067051c */
    ZfileErrorBuffer gZfileErrorMessage; /* 0x00670528 */
    ZfileDevice *gZfileDefaultDevice;    /* 0x00670568 */
    unsigned char d_0067056c[64];        /* 0x0067056c */
    ZfileNameBuffer gZfileNameBuffer;    /* 0x006705ac */
    unsigned char pad_0067062c[4];       /* 0x0067062c */
    ZfileDeviceTable gZfileDevices;      /* 0x00670630 */
    int gZfileInitialized;               /* 0x00670674 */
    ZMemFreeList *gZfileHandles;         /* 0x00670678 */
    ZMemPool *gZfileLoadPool;            /* 0x0067067c */
    ZMemInUseTable gZmemFreeListInUse;   /* 0x00670680 */
    ZMemState gZmem;                     /* 0x00670880 */
    unsigned char pad_006708a4[4];       /* 0x006708a4 */
    ZMemFreeListTable gZmemFreeLists;    /* 0x006708a8 */
    ZMemHooks gZmemHooks;                /* 0x006718a8 */
    unsigned char d_006718b0[16];        /* 0x006718b0 */
    unsigned char d_006718c0[2544];      /* 0x006718c0 */
    int gZmemHooksInstalled;             /* 0x006722b0 */
    unsigned char pad_006722b4[4];       /* 0x006722b4 */
    ZrwList gZpropDefs;                  /* 0x006722b8, also gDeferredB, gZobjTreeObjects */
    ZrwList gZpropInsts;                 /* 0x006722c4, also gDeferredC, gZobjProps */
    int gZobjInitialized;                /* 0x006722d0 */
    ZMemFreeList *gZobjPool;             /* 0x006722d4 */
    ZMemFreeList *gZobjLinkPool;         /* 0x006722d8 */
    ZrwUpdateNode *gZobjDebugNode;       /* 0x006722dc */
    ZrwUpdateNode *gZobjUpdateNode;      /* 0x006722e0 */
    unsigned char pad_006722e4[4];       /* 0x006722e4 */
    ZrwModelState gZrwModelState;        /* 0x006722e8 */
    unsigned char pad_006722f4[4];       /* 0x006722f4 */
    ZVec3 gZrwOrigin;                    /* 0x006722f8 */
    unsigned char pad_00672304[4];       /* 0x00672304 */
    int gZobjModelReady;                 /* 0x00672308 */
    int gZrwModelFlag2;                  /* 0x0067230c */
    unsigned char d_00672310[8];         /* 0x00672310 */
    unsigned char d_00672318[8];         /* 0x00672318, also gZrwMadfRec */
    unsigned char d_00672320[72];        /* 0x00672320 */
    unsigned char d_00672368[64];        /* 0x00672368 */
    ZrwMatState gZrwMat;                 /* 0x006723a8, also gZglMaterials */
    unsigned char pad_006723cc[4];       /* 0x006723cc */
    char gEpicMessage[256];              /* 0x006723d0; the type is local to zge/com/zglrw/zrwzgl.c */
    uint8_t gZglSubSystemNames;          /* 0x006724d0 */
    unsigned char pad_006724d1[639];     /* 0x006724d1 */
    int gZglSubSystemCount;              /* 0x00672750 */
    int gZglSubSystem;                   /* 0x00672754 */
    uint32_t gZglVideoMode;              /* 0x00672758 */
    uint32_t gLastTick;                  /* 0x0067275c */
    int gZrwInitialized;                 /* 0x00672760 */
    char *gZrwArgument;                  /* 0x00672764 */
    ZrwUpdateListTable gZrwUpdateLists;  /* 0x00672768, also gLists */
    ZrwStateStack gZrwStateStack;        /* 0x006727d4 */
    int gZrwStateDepth;                  /* 0x006727f4 */
    ZMemFreeList *gZrwNodePool;          /* 0x006727f8 */
    int gZrwSkipDevice;                  /* 0x006727fc */
    uint32_t gZrwEngineOpen;             /* 0x00672800 */
    uint32_t gZrwEngineInit;             /* 0x00672804 */
    char gZrwArgBuffer;                  /* 0x00672808 */
    unsigned char pad_00672809[131];     /* 0x00672809 */
    int gNoMovies;                       /* 0x0067288c, also gMovieSkip */
    int gZrwPrimFrame;                   /* 0x00672890 */
    unsigned char pad_00672894[4];       /* 0x00672894 */
    ZrwPrimState gZrwPrim;               /* 0x00672898 */
    float gZrwPrimDepthBias;             /* 0x006728b0 */
    unsigned char pad_006728b4[4];       /* 0x006728b4 */
    uint32_t gZtaginfUnused;             /* 0x006728b8 */
    uint32_t gZtaginfFaceCount;          /* 0x006728bc */
    struct ZbspLevelFace *gZtaginfFaces; /* 0x006728c0 */
    ZtaginfNode gZtaginfRoot;            /* 0x006728c4, also gZrwColRoot */
    unsigned char pad_006728f8[4];       /* 0x006728f8 */
    void *gZtaginfData;                  /* 0x006728fc */
    int gZrwColDebug;                    /* 0x00672900, also gZtaginfUnused2 */
    unsigned char pad_00672904[4];       /* 0x00672904 */
    struct {
        uint8_t valid;
        uint8_t pad01[3];
        struct {
            float x;
            float y;
            float z;
        } v0;
        uint32_t pad10;
        struct {
            float x;
            float y;
            float z;
        } v1;
        uint32_t pad20;
        struct {
            float x;
            float y;
            float z;
        } v2;
        uint32_t pad30;
        uint32_t tag;
    } gZrwColList[64];                   /* 0x00672908; the type is local to zge/com/zglrw/zrwmodelrender.c */
    ZrwRenderColHit gZrwColHit;          /* 0x00673708 */
    unsigned char d_00673740[4];         /* 0x00673740 */
    unsigned char d_00673744[4];         /* 0x00673744 */
    unsigned char d_00673748[4];         /* 0x00673748 */
    unsigned char d_0067374c[40];        /* 0x0067374c */
    unsigned char d_00673774[4];         /* 0x00673774 */
    unsigned char d_00673778[4];         /* 0x00673778 */
    unsigned char d_0067377c[4];         /* 0x0067377c */
    unsigned char d_00673780[4];         /* 0x00673780 */
    unsigned char d_00673784[8];         /* 0x00673784 */
    unsigned char d_0067378c[4];         /* 0x0067378c */
    unsigned char d_00673790[4];         /* 0x00673790 */
    unsigned char d_00673794[24];        /* 0x00673794 */
    unsigned char d_006737ac[4];         /* 0x006737ac */
    ZrwRenderColQuery gZrwColQuery;      /* 0x006737b0 */
    ZMemFreeList *gZtaginfNodes;         /* 0x006737b8 */
    unsigned char pad_006737bc[4];       /* 0x006737bc */
    unsigned char d_006737c0[4];         /* 0x006737c0 */
    unsigned char d_006737c4[1724];      /* 0x006737c4 */
    ZrwModelRenderState gZrwModelRender; /* 0x00673e80 */
    unsigned char d_00673ea8[2];         /* 0x00673ea8 */
    unsigned char d_00673eaa[190];       /* 0x00673eaa */
    int gZobjLightInitialized;           /* 0x00673f68 */
    unsigned char pad_00673f6c[4];       /* 0x00673f6c */
    ZsceneState gScene;                  /* 0x00673f70 */
    unsigned char pad_00673ffc[4];       /* 0x00673ffc */
    ZscnptsState gScnpts;                /* 0x00674000, also gZrwMarkers */
    int gZrwCamReady;                    /* 0x00674018 */
    ZMemFreeList *gZrwCamPool;           /* 0x0067401c */
    ZrwCamera *gZrwCurrentCamera;        /* 0x00674020 */
    unsigned char pad_00674024[4];       /* 0x00674024 */
    int gZpadDiActive;                   /* 0x00674028 */
    ZMemFreeList *gZpadDiRecords;        /* 0x0067402c */
    uint16_t gZpadKeyboardOk;            /* 0x00674030 */
    unsigned char pad_00674032[2];       /* 0x00674032 */
    void *gZpadWindow;                   /* 0x00674034 */
    ZpadDirectInput *gZpadDirectInput;   /* 0x00674038 */
    ZpadGuid gZpadJoystickGuid;          /* 0x0067403c */
    ZpadDiDevice *gZpadKeyboard;         /* 0x0067404c */
    uint32_t gZpadKeyboardFlags;         /* 0x00674050 */
    ZpadKeyState gZpadKeyState;          /* 0x00674054 */
    unsigned char pad_00674154[4];       /* 0x00674154 */
    ZrwColColor gZrwColColor;            /* 0x00674158 */
    unsigned char pad_0067415c[4];       /* 0x0067415c */
    ZrwColState gZrwCol;                 /* 0x00674160 */
    uint32_t gZrwColEnabled;             /* 0x006741b0 */
    int gZobjPEmitInitialized;           /* 0x006741b4 */
    ZrwWorldRecord gZrwWorldRecord;      /* 0x006741b8 */
    int gZrwWorldReady;                  /* 0x006741e0 */
    int gZrwWorldActive;                 /* 0x006741e4 */
    ZList *gZrwWorldMaterials;           /* 0x006741e8 */
    ZrwWorldStats gZrwWorldStats;        /* 0x006741ec, also gZrwWorldPvsState */
    char gZrwWorldName;                  /* 0x00674208, also gZrwPvsBuffer */
    unsigned char pad_00674209[135];     /* 0x00674209 */
    ZrwTextureState gZrwTxr;             /* 0x00674290 */
    int gZanimInitialized;               /* 0x006746c0 */
    ZSkeletonItem *gZiffItemPool;        /* 0x006746c4 */
    void *gZiffAnim;                     /* 0x006746c8 */
    unsigned char pad_006746cc[4];       /* 0x006746cc */
    ZiffState gZiff;                     /* 0x006746d0 */
    unsigned char pad_006746ec[4];       /* 0x006746ec */
    ZrwShadowPointList gZrwShadowPoints; /* 0x006746f0 */
    ZrwShadowQuadList gZrwShadowQuads;   /* 0x00674930 */
    unsigned char pad_00674b24[4];       /* 0x00674b24 */
    uint32_t gZrwShadowActive;           /* 0x00674b28 */
    unsigned char pad_00674b2c[8];       /* 0x00674b2c */
    ZrwShadowPointList gZrwShadowOffsets;     /* 0x00674b34 */
    ZrwShadowVertexList gZrwShadowVertices;   /* 0x00674d74 */
    ZrwShadowMissList gZrwShadowMiss;         /* 0x00675164 */
    uint32_t gZrwShadowTexture;               /* 0x006751f4 */
    struct ZrwUpdateNode *gZrwShadowNode;     /* 0x006751f8 */
    uint32_t gZrwShadowReady;                 /* 0x006751fc */
    float gZrwShadowScale;                    /* 0x00675200 */
    float gZrwShadowDouble;                   /* 0x00675204 */
    unsigned char pad_00675208[4];            /* 0x00675208 */
    uint32_t gZrwShadowInit;                  /* 0x0067520c */
    unsigned char d_00675210[4];              /* 0x00675210 */
    unsigned char d_00675214[4];              /* 0x00675214 */
    ZrwModelCreateState gZrwModelCreate;      /* 0x00675218 */
    unsigned char d_0067523c[4];              /* 0x0067523c */
    char gZrwNameBuffer[128];                 /* 0x00675240; the type is local to zge/com/zglrw/zrwfile.c */
    ZrwFileOps gZrwFileOps;                   /* 0x006752c0 */
    int gZrwFileInitialized;                  /* 0x006752e8 */
    ZramdiskNameBuffer gZramdiskName;         /* 0x006752ec */
    ZMemFreeList *gZramdiskFiles;             /* 0x0067536c */
    int gZrwLightInitialized;                 /* 0x00675370 */
    ZMemFreeList *gZrwLightPool;              /* 0x00675374 */
    int gZgeInitialized;                      /* 0x00675378 */
    void *gPanorama;                          /* 0x0067537c */
    ZtokenState gToken;                       /* 0x00675380 */
    ZFontState gFont;                         /* 0x006763d0 */
    unsigned char d_006763e0[4];              /* 0x006763e0 */
    unsigned char d_006763e4[4];              /* 0x006763e4 */
    ZfileHandle *gZrwRamDiskFile;             /* 0x006763e8 */
    ZrwRamDiskInfoSlots gZrwRamDiskInfoSlots; /* 0x006763ec */
    ZrwRamDiskInfo *gZrwRamDiskNoInfo;        /* 0x00676404 */
    ZrwRamDiskInfo *gZrwRamDiskInfo;          /* 0x00676408 */
    unsigned char pad_0067640c[4];            /* 0x0067640c */
    ZrwFileInterface gZrwSavedFileInterface;  /* 0x00676410 */
    ZProfileState gZProfile;                  /* 0x00676438 */
    uint16_t gZtimerSlots;                    /* 0x006764a0 */
    unsigned char pad_006764a2[2];            /* 0x006764a2 */
    ZtimerHeap gZtimerHeap;                   /* 0x006764a4 */
    ZrwList gZtimerFree;                      /* 0x006764b4 */
    ZrwUpdateNode *gZtimerUpdate;             /* 0x006764c0 */
    uint32_t gZtimerReady;                    /* 0x006764c4 */
    unsigned char d_006764c8[8];              /* 0x006764c8 */
    PcProfileClock gPcProfileClock;           /* 0x006764d0 */
    ZtimerHeapEntry gZtimerPopped;            /* 0x006764e0 */
    unsigned char d_006764e8[4];              /* 0x006764e8 */
    uint32_t gCrtFpStatus;                    /* 0x006764ec */
    unsigned char d_006764f0[8];              /* 0x006764f0 */
    unsigned char d_006764f8[4];              /* 0x006764f8 */
    unsigned char d_006764fc[4];              /* 0x006764fc */
    unsigned char d_00676500[4];              /* 0x00676500 */
    unsigned char d_00676504[4];              /* 0x00676504 */
    unsigned char d_00676508[4];              /* 0x00676508 */
    unsigned char d_0067650c[4];              /* 0x0067650c */
    unsigned char d_00676510[4];              /* 0x00676510 */
    unsigned char d_00676514[4];              /* 0x00676514 */
    unsigned char d_00676518[4];              /* 0x00676518 */
    unsigned char d_0067651c[8];              /* 0x0067651c */
    unsigned char d_00676524[4];              /* 0x00676524 */
    unsigned char d_00676528[4];              /* 0x00676528 */
    unsigned char d_0067652c[8];              /* 0x0067652c */
    unsigned char d_00676534[8];              /* 0x00676534 */
    unsigned char d_0067653c[4];              /* 0x0067653c */
    unsigned char d_00676540[4];              /* 0x00676540 */
    unsigned char d_00676544[4];              /* 0x00676544 */
    unsigned char d_00676548[4];              /* 0x00676548 */
    unsigned char d_0067654c[4];              /* 0x0067654c */
    unsigned char d_00676550[4];              /* 0x00676550 */
    unsigned char d_00676554[4];              /* 0x00676554 */
    unsigned char d_00676558[4];              /* 0x00676558 */
    unsigned char d_0067655c[4];              /* 0x0067655c */
    unsigned char d_00676560[4];              /* 0x00676560 */
    unsigned char d_00676564[4];              /* 0x00676564 */
    unsigned char d_00676568[4];              /* 0x00676568 */
    unsigned char d_0067656c[4];              /* 0x0067656c */
    unsigned char d_00676570[4];              /* 0x00676570 */
    unsigned char d_00676574[4];              /* 0x00676574 */
    unsigned char d_00676578[4];              /* 0x00676578 */
    unsigned char d_0067657c[4];              /* 0x0067657c */
    unsigned char d_00676580[4];              /* 0x00676580 */
    unsigned char d_00676584[4];              /* 0x00676584 */
    unsigned char d_00676588[8];              /* 0x00676588 */
    unsigned char d_00676590[4];              /* 0x00676590 */
    unsigned char d_00676594[260];            /* 0x00676594 */
    unsigned char d_00676698[4];              /* 0x00676698 */
    unsigned char d_0067669c[4];              /* 0x0067669c */
    unsigned char d_006766a0[4];              /* 0x006766a0 */
    unsigned char d_006766a4[8];              /* 0x006766a4 */
    unsigned char d_006766ac[16];             /* 0x006766ac */
    unsigned char d_006766bc[12];             /* 0x006766bc */
    unsigned char d_006766c8[2];              /* 0x006766c8 */
    unsigned char d_006766ca[2];              /* 0x006766ca */
    unsigned char d_006766cc[28];             /* 0x006766cc */
    unsigned char d_006766e8[4];              /* 0x006766e8 */
    unsigned char d_006766ec[4];              /* 0x006766ec */
    unsigned char d_006766f0[4];              /* 0x006766f0 */
    unsigned char d_006766f4[4];              /* 0x006766f4 */
    unsigned char d_006766f8[4];              /* 0x006766f8 */
    unsigned char d_006766fc[4];              /* 0x006766fc */
    unsigned char d_00676700[4];              /* 0x00676700 */
    unsigned char d_00676704[12];             /* 0x00676704 */
    int32_t rpClumpModuleOffset;              /* 0x00676710 */
    int32_t rpClumpModuleCount;               /* 0x00676714 */
    unsigned char pad_00676718[8];            /* 0x00676718 */
    int32_t rpGeometryOffset;                 /* 0x00676720 */
    int32_t rpGeometryModuleCount;            /* 0x00676724 */
    unsigned char pad_00676728[8];            /* 0x00676728 */
    int32_t rpTieOffset;                      /* 0x00676730 */
    int32_t rpTieModuleCount;                 /* 0x00676734 */
    int32_t rpWorldCameraOffset;              /* 0x00676738 */
    int32_t rpWorldAtomicOffset;              /* 0x0067673c */
    int32_t rpWorldClumpOffset;               /* 0x00676740 */
    int32_t rpWorldLightOffset;               /* 0x00676744 */
    unsigned char pad_00676748[8];            /* 0x00676748 */
    int32_t rpMaterialOffset;                 /* 0x00676750 */
    int32_t rpMaterialModuleCount;            /* 0x00676754 */
    unsigned char pad_00676758[8];            /* 0x00676758 */
    int32_t rpWorldGlobalsOffset;             /* 0x00676760 */
    int32_t rpWorldModuleCount;               /* 0x00676764 */
    RwSurfaceProperties rpWorldDefaultSurfaceProps; /* 0x00676768 */
    unsigned char pad_00676774[16];                 /* 0x00676774 */
    int32_t rpBspModuleCount;                       /* 0x00676784 */
    unsigned char pad_00676788[12];                 /* 0x00676788 */
    int32_t rpSectorModuleCount;                    /* 0x00676794 */
    unsigned char pad_00676798[8];                  /* 0x00676798 */
    int32_t rpLightOffset;                          /* 0x006767a0 */
    int32_t rpLightModuleCount;                     /* 0x006767a4 */
    unsigned char pad_006767a8[12];                 /* 0x006767a8 */
    int32_t rpBinaryWorldModuleCount;               /* 0x006767b4 */
    unsigned char pad_006767b8[8];                  /* 0x006767b8 */
    RpMeshHeader rpMeshEmptyHeader;                 /* 0x006767c0 */
    int32_t rpMeshStripMode;                        /* 0x006767d0 */
    unsigned char pad_006767d4[16];                 /* 0x006767d4 */
    int32_t rpMaterialListModuleCount;              /* 0x006767e4 */
    unsigned char pad_006767e8[8];                  /* 0x006767e8 */
    unsigned char d_006767f0[8];                    /* 0x006767f0 */
    unsigned char d_006767f8[8];                    /* 0x006767f8 */
    unsigned char d_00676800[8];                    /* 0x00676800 */
    unsigned char d_00676808[8];                    /* 0x00676808 */
    RpWorld *rpPVSWorld;                            /* 0x00676810 */
    RwCamera *rpPVSCamera;                          /* 0x00676814, also rwPvsCamera */
    RwImage *rpPVSImage;                            /* 0x00676818, also rwPvsImage */
    unsigned char pad_0067681c[4];                  /* 0x0067681c */
    RwGlobals RwDefaultGlobals;                     /* 0x00676820 */
    int32_t rwEngineInstanceCount;                  /* 0x00676968 */
    RwGlobals *RwEngineInstance;                    /* 0x0067696c, also gZrwStringApi */
    int32_t rwMatrixOffset;                         /* 0x00676970, also rwMatrixGlobalsOffset */
    int32_t rwMatrixModuleCount;                    /* 0x00676974 */
    unsigned char pad_00676978[8];                  /* 0x00676978 */
    int32_t rwFrameOffset;                          /* 0x00676980 */
    int32_t rwFrameModuleCount;                     /* 0x00676984 */
    unsigned char pad_00676988[8];                  /* 0x00676988 */
    int32_t rwFastSqrtOffset;                       /* 0x00676990 */
    int32_t rwVectorModuleCount;                    /* 0x00676994 */
    unsigned char pad_00676998[8];                  /* 0x00676998 */
    int32_t rwCameraFpuSet;                         /* 0x006769a0 */
    unsigned char d_006769a4[256];                  /* 0x006769a4 */
    unsigned char d_00676aa4[256];                  /* 0x00676aa4 */
    int32_t rwD3DFlipPending;                       /* 0x00676ba4 */
    unsigned char d_00676ba8[4];                    /* 0x00676ba8 */
    RwD3DDeferList *rwD3DDefer;                     /* 0x00676bac */
    uint32_t rwD3DSortNeed;                         /* 0x00676bb0 */
    uint32_t rwD3DSortCap;                          /* 0x00676bb4 */
    uint32_t rwD3DSortCount;                        /* 0x00676bb8 */
    RwD3DSortItem *rwD3DSortItems;                  /* 0x00676bbc */
    unsigned char d_00676bc0[4];                    /* 0x00676bc0 */
    unsigned char d_00676bc4[256];                  /* 0x00676bc4 */
    int32_t rwD3DAlphaTestCaps;                     /* 0x00676cc4 */
    uint32_t rwD3DMergeCap;                         /* 0x00676cc8 */
    uint16_t *rwD3DMergeBuf;                        /* 0x00676ccc */
    int32_t rwD3DRemapCap;                          /* 0x00676cd0 */
    uint16_t *rwD3DRemap;                           /* 0x00676cd4 */
    uint16_t *rwD3DRemapUsed;                       /* 0x00676cd8 */
    RwDevice *rwD3DEngineDevice;                    /* 0x00676cdc */
    RwLLLink rwFreeListHead;                        /* 0x00676ce0 */
    unsigned char pad_00676ce8[8];                  /* 0x00676ce8 */
    int32_t rwResourcesOffset;                      /* 0x00676cf0 */
    int32_t rwResourcesModuleCount;                 /* 0x00676cf4 */
    unsigned char pad_00676cf8[8];                  /* 0x00676cf8 */
    int32_t rwRasterOffset;                         /* 0x00676d00 */
    int32_t rwRasterModuleCount;                    /* 0x00676d04 */
    unsigned char pad_00676d08[8];                  /* 0x00676d08 */
    int32_t rwCameraOffset;                         /* 0x00676d10 */
    int32_t rwCameraModuleCount;                    /* 0x00676d14 */
    unsigned char pad_00676d18[8];                  /* 0x00676d18 */
    int32_t rwErrorOffset;                          /* 0x00676d20 */
    int32_t rwErrorModuleCount;                     /* 0x00676d24 */
    unsigned char pad_00676d28[8];                  /* 0x00676d28 */
    int32_t rwIm3DPipelineOffset;                   /* 0x00676d30 */
    int32_t rwIm3DModuleCount;                      /* 0x00676d34 */
    unsigned char pad_00676d38[8];                  /* 0x00676d38 */
    int32_t rwTextureOffset;                        /* 0x00676d40 */
    int32_t rwTextureModuleCount;                   /* 0x00676d44 */
    unsigned char pad_00676d48[8];                  /* 0x00676d48 */
    int32_t rwStreamOffset;                         /* 0x00676d50 */
    int32_t rwStreamModuleCount;                    /* 0x00676d54 */
    unsigned char pad_00676d58[8];                  /* 0x00676d58 */
    int32_t rwImageOffset;                          /* 0x00676d60 */
    int32_t rwImageModuleCount;                     /* 0x00676d64 */
    unsigned char pad_00676d68[8];                  /* 0x00676d68 */
    int32_t rwIm3DOffset;                           /* 0x00676d70 */
    int32_t rwRenderPipelineModuleCount;            /* 0x00676d74 */
    unsigned char pad_00676d78[8];                  /* 0x00676d78 */
    RwFreeList *rwPluginRegEntryFreeList;           /* 0x00676d80 */
    unsigned char pad_00676d84[12];                 /* 0x00676d84 */
    char rwEmptyString;                             /* 0x00676d90 */
    unsigned char pad_00676d91[19];                 /* 0x00676d91 */
    int32_t rwColorModuleCount;                     /* 0x00676da4 */
    unsigned char pad_00676da8[8];                  /* 0x00676da8 */
    unsigned char d_00676db0[4];                    /* 0x00676db0 */
    unsigned char d_00676db4[4];                    /* 0x00676db4 */
    unsigned char d_00676db8[4];                    /* 0x00676db8 */
    unsigned char d_00676dbc[4];                    /* 0x00676dbc */
    unsigned char d_00676dc0[4];                    /* 0x00676dc0 */
    unsigned char d_00676dc4[4];                    /* 0x00676dc4 */
    unsigned char d_00676dc8[4];                    /* 0x00676dc8 */
    unsigned char d_00676dcc[28];                   /* 0x00676dcc */
    unsigned char d_00676de8[4012];                 /* 0x00676de8 */
    unsigned char d_00677d94[4];                    /* 0x00677d94 */
    unsigned char d_00677d98[4];                    /* 0x00677d98 */
    unsigned char d_00677d9c[4];                    /* 0x00677d9c */
    unsigned char d_00677da0[4];                    /* 0x00677da0 */
    unsigned char d_00677da4[4];                    /* 0x00677da4 */
    unsigned char d_00677da8[4];                    /* 0x00677da8 */
    unsigned char d_00677dac[4];                    /* 0x00677dac */
    unsigned char d_00677db0[4];                    /* 0x00677db0 */
    unsigned char d_00677db4[4];                    /* 0x00677db4 */
    unsigned char d_00677db8[4];                    /* 0x00677db8 */
    unsigned char d_00677dbc[4];                    /* 0x00677dbc */
    unsigned char d_00677dc0[4];                    /* 0x00677dc0 */
    unsigned char d_00677dc4[4];                    /* 0x00677dc4 */
    unsigned char d_00677dc8[4];                    /* 0x00677dc8 */
    unsigned char d_00677dcc[4];                    /* 0x00677dcc */
    unsigned char d_00677dd0[4];                    /* 0x00677dd0 */
    unsigned char d_00677dd4[4];                    /* 0x00677dd4 */
    unsigned char d_00677dd8[4];                    /* 0x00677dd8 */
    unsigned char d_00677ddc[4];                    /* 0x00677ddc */
    unsigned char d_00677de0[4];                    /* 0x00677de0 */
    unsigned char d_00677de4[4];                    /* 0x00677de4 */
    unsigned char d_00677de8[4];                    /* 0x00677de8 */
    unsigned char d_00677dec[4];                    /* 0x00677dec */
    unsigned char d_00677df0[4];                    /* 0x00677df0 */
    unsigned char d_00677df4[12];                   /* 0x00677df4 */
    unsigned char d_00677e00[4];                    /* 0x00677e00 */
    unsigned char d_00677e04[4];                    /* 0x00677e04 */
    unsigned char d_00677e08[4];                    /* 0x00677e08 */
    unsigned char d_00677e0c[4];                    /* 0x00677e0c */
    unsigned char d_00677e10[4];                    /* 0x00677e10 */
    unsigned char d_00677e14[4];                    /* 0x00677e14 */
    unsigned char d_00677e18[4];                    /* 0x00677e18 */
    unsigned char d_00677e1c[4];                    /* 0x00677e1c */
    unsigned char d_00677e20[4];                    /* 0x00677e20 */
    unsigned char d_00677e24[4];                    /* 0x00677e24 */
    unsigned char d_00677e28[4];                    /* 0x00677e28 */
    unsigned char d_00677e2c[4];                    /* 0x00677e2c */
    unsigned char d_00677e30[4];                    /* 0x00677e30 */
    unsigned char d_00677e34[4];                    /* 0x00677e34 */
    unsigned char d_00677e38[4];                    /* 0x00677e38 */
    unsigned char d_00677e3c[4];                    /* 0x00677e3c */
    unsigned char d_00677e40[16];                   /* 0x00677e40 */
    unsigned char d_00677e50[1024];                 /* 0x00677e50 */
    unsigned char d_00678250[4];                    /* 0x00678250 */
    unsigned char d_00678254[4];                    /* 0x00678254 */
    unsigned char d_00678258[4];                    /* 0x00678258 */
    unsigned char d_0067825c[4];                    /* 0x0067825c */
    unsigned char d_00678260[4];                    /* 0x00678260 */
    unsigned char d_00678264[4];                    /* 0x00678264 */
    unsigned char d_00678268[8];                    /* 0x00678268 */
    int32_t rwD3DXformStride;                       /* 0x00678270 */
    const RwMatrix *rwD3DXformMatrix;               /* 0x00678274 */
    RwD3DTLVertex *rwD3DXformOut;                   /* 0x00678278 */
    int32_t rwD3DXformFog;                          /* 0x0067827c */
    int32_t rpAnimEngineOffset;                     /* 0x00678280 */
    int32_t rpAnimModuleCount;                      /* 0x00678284 */
    int32_t rpAnimFrameOffset;                      /* 0x00678288 */
    int32_t rpAnimClumpOffset;                      /* 0x0067828c */
    int32_t rpBoneFrameOffset;                      /* 0x00678290 */
    unsigned char d_00678294[4];                    /* 0x00678294 */
    int32_t rpBoneEngineOffset;                     /* 0x00678298 */
    int32_t rpBoneModuleCount;                      /* 0x0067829c */
    int32_t rpBoneStreamResult;                     /* 0x006782a0 */
    int32_t rpBoneGeometryOffset;                   /* 0x006782a4 */
    unsigned char pad_006782a8[8];                  /* 0x006782a8 */
    unsigned char d_006782b0[8];                    /* 0x006782b0 */
    unsigned char d_006782b8[8];                    /* 0x006782b8 */
    unsigned char d_006782c0[2];                    /* 0x006782c0 */
    unsigned char d_006782c2[4];                    /* 0x006782c2 */
    unsigned char d_006782c6[2];                    /* 0x006782c6 */
    unsigned char d_006782c8[2];                    /* 0x006782c8 */
    unsigned char d_006782ca[6];                    /* 0x006782ca */
    unsigned char d_006782d0[3];                    /* 0x006782d0 */
    unsigned char d_006782d3[29];                   /* 0x006782d3 */
    unsigned char d_006782f0[8];                    /* 0x006782f0 */
    unsigned char d_006782f8[4];                    /* 0x006782f8 */
    unsigned char d_006782fc[64];                   /* 0x006782fc */
    unsigned char d_0067833c[2];                    /* 0x0067833c */
    unsigned char d_0067833e[2];                    /* 0x0067833e */
    unsigned char d_00678340[2];                    /* 0x00678340 */
    unsigned char d_00678342[2];                    /* 0x00678342 */
    unsigned char d_00678344[2];                    /* 0x00678344 */
    unsigned char d_00678346[2];                    /* 0x00678346 */
    unsigned char d_00678348[2];                    /* 0x00678348 */
    unsigned char d_0067834a[2];                    /* 0x0067834a */
    unsigned char d_0067834c[4];                    /* 0x0067834c */
    unsigned char d_00678350[64];                   /* 0x00678350 */
    unsigned char d_00678390[2];                    /* 0x00678390 */
    unsigned char d_00678392[2];                    /* 0x00678392 */
    unsigned char d_00678394[2];                    /* 0x00678394 */
    unsigned char d_00678396[2];                    /* 0x00678396 */
    unsigned char d_00678398[2];                    /* 0x00678398 */
    unsigned char d_0067839a[2];                    /* 0x0067839a */
    unsigned char d_0067839c[2];                    /* 0x0067839c */
    unsigned char d_0067839e[2];                    /* 0x0067839e */
    unsigned char d_006783a0[4];                    /* 0x006783a0 */
    unsigned char d_006783a4[4];                    /* 0x006783a4 */
    unsigned char d_006783a8[8];                    /* 0x006783a8 */
    unsigned char d_006783b0[4];                    /* 0x006783b0 */
    unsigned char d_006783b4[4];                    /* 0x006783b4 */
    unsigned char d_006783b8[4];                    /* 0x006783b8 */
    unsigned char d_006783bc[4];                    /* 0x006783bc */
    unsigned char d_006783c0[4];                    /* 0x006783c0 */
    unsigned char d_006783c4[4];                    /* 0x006783c4 */
    unsigned char d_006783c8[4];                    /* 0x006783c8 */
    unsigned char d_006783cc[4];                    /* 0x006783cc */
    unsigned char d_006783d0[4];                    /* 0x006783d0 */
    unsigned char d_006783d4[40];                   /* 0x006783d4 */
    int32_t gZprimHandle;                           /* 0x006783fc */
    int32_t gZprimLibrary;                          /* 0x00678400 */
    ZPrimAsset *gZprimPool;                         /* 0x00678404 */
    ZPrimAsset *gZprimFree;                         /* 0x00678408 */
    ZPrimAsset *gZprimUsed;                         /* 0x0067840c */
    ZPrimRect gZprimClip;                           /* 0x00678410 */
    ZPrimGroupTable gZprimGroups;                   /* 0x00678418 */
    uint16_t gZprimNextId;                          /* 0x00678428 */
    unsigned char pad_0067842a[2];                  /* 0x0067842a */
    int32_t gZprimDepth;                            /* 0x0067842c */
    ZPrimState *gZprimStates;                       /* 0x00678430 */
    unsigned char pad_00678434[12];                 /* 0x00678434 */
    ZrollerShadow gZrollerShadow;                   /* 0x00678440 */
    unsigned char pad_00678468[20];                 /* 0x00678468 */
    int gZresTablesLib;                             /* 0x0067847c */
    unsigned char pad_00678480[12];                 /* 0x00678480 */
    int gZresMaterialLib;                           /* 0x0067848c */
    int gZresMiscLib;                               /* 0x00678490, also gAssetLibrary, gOvlLibrary */
    ZList *gZrollerEdges;                           /* 0x00678494, also gNgRoller */
    unsigned char pad_00678498[200];                /* 0x00678498 */
    NgifWords gNgifPrim3DBuffer;                    /* 0x00678560 */
    unsigned char pad_00678564[81516];              /* 0x00678564 */
    uint32_t *gNgifOrderTable;                      /* 0x0068c3d0 */
    unsigned char pad_0068c3d4[12];                 /* 0x0068c3d4 */
    ZrollerModelTable gZrollerModels; /* 0x0068c3e0, also gNgifTextures, gZoverSprites, gZresOverlaySlots */
    uint32_t *gNgifPrim3D;            /* 0x0068ebe0 */
    unsigned char pad_0068ebe4[12];   /* 0x0068ebe4 */
    unsigned char d_0068ebf0[16];     /* 0x0068ebf0 */
    unsigned char d_0068ec00[4096];   /* 0x0068ec00 */
    uint32_t gNgifColor;              /* 0x0068fc00 */
    unsigned char pad_0068fc04[28];   /* 0x0068fc04 */
    uint32_t gFxDustTextures[16];     /* 0x0068fc20; the type is local to bmx/com/zge/ngif/ngiffx.c */
    unsigned char pad_0068fc60[152];  /* 0x0068fc60 */
    int gFxAnimHandle;                /* 0x0068fcf8 */
    int gNgifScriptNode;              /* 0x0068fcfc */
    ZfxSkidTable gFxSkids;            /* 0x0068fd00 */
    uint8_t *gFxWorldObject;          /* 0x0068ff80 */
    uint32_t gNgWord68ff84;           /* 0x0068ff84 */
    unsigned char pad_0068ff88[12];   /* 0x0068ff88 */
    struct ZMemFreeList *gZprimFontPageList; /* 0x0068ff94, also gNgFontPages, gNgifGlyphPages */
    ZObjCamera *gNgCamera;                   /* 0x0068ff98, also gCameraObject, gFxSceneObject */
    unsigned char pad_0068ff9c[4];           /* 0x0068ff9c */
    uint8_t gNgFlags68ffa0;                  /* 0x0068ffa0 */
    uint8_t gNgFlags68ffa1;                  /* 0x0068ffa1 */
    uint8_t gNgFlags68ffa2;                  /* 0x0068ffa2 */
    uint8_t gNgFlags68ffa3;                  /* 0x0068ffa3 */
    ZrwUpdateNode *gNgTickNode;              /* 0x0068ffa4 */
    int gZresReplayLib;                      /* 0x0068ffa8, also gActionReplayLib */
    unsigned char pad_0068ffac[20];          /* 0x0068ffac */
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
    } gZoverSweep;                     /* 0x0068ffc0; the type is local to bmx/com/zge/bmx/zoverii.c */
    unsigned char pad_0068ffdc[20];    /* 0x0068ffdc */
    ExtrasTitleList gExtrasTitles;     /* 0x0068fff0 */
    unsigned char pad_00690000[1232];  /* 0x00690000 */
    HiscoreColumn gHiscoreTimes;       /* 0x006904d0 */
    uint32_t gAssetSetSize;            /* 0x00690890 */
    uint16_t gHiscoreWait;             /* 0x00690894 */
    uint16_t gExtrasWait2;             /* 0x00690896 */
    uint16_t gInfoWait;                /* 0x00690898 */
    unsigned char pad_0069089a[246];   /* 0x0069089a */
    HiscoreColumn gHiscoreLabels;      /* 0x00690990 */
    unsigned char pad_00690d50[4];     /* 0x00690d50 */
    unsigned char d_00690d54[252];     /* 0x00690d54 */
    HiscoreColumn gHiscoreRiders;      /* 0x00690e50 */
    ZPrimAsset *gInfoSprite;           /* 0x00691210 */
    int gExtrasChoice;                 /* 0x00691214 */
    uint16_t gInfoPage;                /* 0x00691218 */
    unsigned char pad_0069121a[6];     /* 0x0069121a */
    SlideTable gInfoSlide;             /* 0x00691220 */
    int gExtrasReady;                  /* 0x00691228 */
    uint16_t gInfoTicks;               /* 0x0069122c */
    unsigned char pad_0069122e[2];     /* 0x0069122e */
    SlideTable gExtrasSlide;           /* 0x00691230 */
    int gHiscoreHandle;                /* 0x00691238 */
    ZPrimAsset *gHiscoreSprite;        /* 0x0069123c */
    unsigned char pad_00691240[240];   /* 0x00691240 */
    HiscoreColumn gHiscoreNames;       /* 0x00691330 */
    uint16_t gExtrasCount;             /* 0x006916f0 */
    uint16_t gHiscoreBlink;            /* 0x006916f2 */
    unsigned char pad_006916f4[4];     /* 0x006916f4 */
    SlideTable gHiscoreSlide;          /* 0x006916f8 */
    uint16_t gExtrasWait;              /* 0x00691700 */
    unsigned char pad_00691702[2];     /* 0x00691702 */
    int gSelTick;                      /* 0x00691704 */
    unsigned char pad_00691708[279];   /* 0x00691708 */
    uint8_t gSelBikeCount;             /* 0x0069181f */
    SelSlideTable gSelSlide;           /* 0x00691820 */
    int gSelReadyTick;                 /* 0x0069182c */
    int gSelPage;                      /* 0x00691830 */
    uint16_t gSelExtentA;              /* 0x00691834 */
    uint16_t gSelExtentB;              /* 0x00691836 */
    int gSelSlideDir;                  /* 0x00691838 */
    uint8_t gSelStyleCount;            /* 0x0069183c */
    unsigned char pad_0069183d[3];     /* 0x0069183d */
    CaptionSlotTable gCaptionSlots;    /* 0x00691840 */
    SelPlayerInts gSelRider;           /* 0x00691ca0 */
    int gSelPlayer;                    /* 0x00691ca8 */
    const SelCaption *gCaptionPtr;     /* 0x00691cac */
    int gSelStage;                     /* 0x00691cb0 */
    unsigned char pad_00691cb4[12];    /* 0x00691cb4 */
    CheatRing gCheatRing;              /* 0x00691cc0 */
    uint16_t gCaptionOn;               /* 0x00691ce0 */
    unsigned char pad_00691ce2[2];     /* 0x00691ce2 */
    int gSelPages;                     /* 0x00691ce4 */
    int gSelPicturesOn;                /* 0x00691ce8 */
    uint16_t gSelExtentC;              /* 0x00691cec */
    uint16_t gSelDimA;                 /* 0x00691cee */
    uint16_t gCheatPos;                /* 0x00691cf0 */
    unsigned char pad_00691cf2[2];     /* 0x00691cf2 */
    int gSelFlipTick;                  /* 0x00691cf4 */
    SelSpriteTable gSelSprites;        /* 0x00691cf8 */
    uint16_t gSelDimB;                 /* 0x00691d04 */
    uint16_t gCaptionLine;             /* 0x00691d06 */
    uint8_t gSelLevelCount;            /* 0x00691d08 */
    unsigned char pad_00691d09[79];    /* 0x00691d09 */
    SelPlayerInts gSelStyle;           /* 0x00691d58 */
    uint16_t gSelUnknown60;            /* 0x00691d60 */
    unsigned char pad_00691d62[30];    /* 0x00691d62 */
    CaptionColors gCaptionColors;      /* 0x00691d80 */
    int gSelLevel;                     /* 0x00691d94 */
    SelPlayerInts gSelBike;            /* 0x00691d98 */
    uint32_t gSelUnknownA0;            /* 0x00691da0 */
    uint16_t gCaptionTime;             /* 0x00691da4 */
    uint16_t gCaptionAlpha;            /* 0x00691da6 */
    unsigned char pad_00691da8[8];     /* 0x00691da8 */
    CaptionWidths gCaptionWidths;      /* 0x00691db0 */
    uint16_t gCaptionHold;             /* 0x00691dba */
    int gSelPending;                   /* 0x00691dbc */
    ScreenReturnFn gScreenReturnFn;    /* 0x00691dc0 */
    unsigned char pad_00691dc4[10];    /* 0x00691dc4 */
    uint16_t gSlotSel;                 /* 0x00691dce */
    uint32_t gScreenReturnArg;         /* 0x00691dd0 */
    int gSoundLevel0;                  /* 0x00691dd4 */
    unsigned char pad_00691dd8[32];    /* 0x00691dd8 */
    int gOptMainReady;                 /* 0x00691df8 */
    unsigned char pad_00691dfc[4];     /* 0x00691dfc */
    int gSoundSlideA;                  /* 0x00691e00 */
    int gSoundSlideB;                  /* 0x00691e04 */
    int gKeyPlayer;                    /* 0x00691e08 */
    unsigned char pad_00691e0c[4];     /* 0x00691e0c */
    uint16_t gOptCount;                /* 0x00691e10 */
    unsigned char pad_00691e12[2];     /* 0x00691e12 */
    int gOptSaveDelay;                 /* 0x00691e14 */
    unsigned char pad_00691e18[4];     /* 0x00691e18 */
    int gOptTicks;                     /* 0x00691e1c */
    int gSoundSel;                     /* 0x00691e20 */
    unsigned char pad_00691e24[24];    /* 0x00691e24 */
    unsigned char d_00691e3c[4];       /* 0x00691e3c */
    OptionSlideTable gOptSlide;        /* 0x00691e40 */
    int gOptMainSel;                   /* 0x00691e60 */
    int gSoundLevel1;                  /* 0x00691e64 */
    unsigned char pad_00691e68[24];    /* 0x00691e68 */
    OptionRowTable gOptRows;           /* 0x00691e80 */
    int gOptSel;                       /* 0x00691ea0 */
    unsigned char pad_00691ea4[4];     /* 0x00691ea4 */
    uint16_t gScrollTick;              /* 0x00691ea8 */
    unsigned char pad_00691eaa[10];    /* 0x00691eaa */
    uint16_t gTitleIdle;               /* 0x00691eb4 */
    unsigned char pad_00691eb6[2];     /* 0x00691eb6 */
    int gTitleDelay;                   /* 0x00691eb8 */
    uint16_t gAutoSlot;                /* 0x00691ebc */
    uint16_t gTitleFrames;             /* 0x00691ebe */
    uint16_t gMcChoice;                /* 0x00691ec0 */
    unsigned char pad_00691ec2[18];    /* 0x00691ec2 */
    uint16_t gMcLoadOk;                /* 0x00691ed4 */
    int16_t gMcCountdown;              /* 0x00691ed6 */
    uint16_t gMcMode;                  /* 0x00691ed8 */
    uint16_t gMcSaveOk;                /* 0x00691eda */
    unsigned char pad_00691edc[4];     /* 0x00691edc */
    McRows gMcRows;                    /* 0x00691ee0 */
    uint16_t gMcState;                 /* 0x00691f00 */
    uint16_t gMcSlot;                  /* 0x00691f02 */
    uint16_t gLanguageSel;             /* 0x00691f04 */
    unsigned char pad_00691f06[10];    /* 0x00691f06 */
    int gMenuSlide3;                   /* 0x00691f10 */
    unsigned char pad_00691f14[12];    /* 0x00691f14 */
    int gMenuSlide2;                   /* 0x00691f20 */
    int gMenuSlide1;                   /* 0x00691f24 */
    int gMenuSlide0;                   /* 0x00691f28 */
    uint32_t gGuiPadRepeatHeld;        /* 0x00691f2c */
    unsigned char pad_00691f30[12];    /* 0x00691f30 */
    uint32_t gGuiPadRepeatCount;       /* 0x00691f3c */
    OvlPlayerRecs gPlayerRecs;         /* 0x00691f40, also gPhysRiders */
    unsigned char pad_00693e20[104];   /* 0x00693e20 */
    unsigned char d_00693e88[1496];    /* 0x00693e88 */
    PropRecords gPropRecords;          /* 0x00694460, also gPropState, gReplayTable */
    unsigned char d_00694ca0[1];       /* 0x00694ca0 */
    unsigned char d_00694ca1[3];       /* 0x00694ca1 */
    PropNodeStates gPropNodeStates;    /* 0x00694ca4 */
    unsigned char d_00694fa4[4];       /* 0x00694fa4 */
    unsigned char d_00694fa8[576];     /* 0x00694fa8 */
    uint16_t gPhysDebugTexture;        /* 0x006951e8, also gOvlResB */
    unsigned char pad_006951ea[21];    /* 0x006951ea */
    unsigned char d_006951ff[1];       /* 0x006951ff */
    GameJudgeScores gGameJudgeScores;  /* 0x00695200, also gCareer */
    OvlCareerUnlocked gCareerUnlocked; /* 0x0069521e, also gGameJudgeRank */
    unsigned char pad_00695221[63];    /* 0x00695221 */
    GameHorseTricks gGameHorseTricks;  /* 0x00695260 */
    uint8_t gGameHorseCount;           /* 0x00695290 */
    unsigned char pad_00695291[15];    /* 0x00695291 */
    OvlProfiles gProfiles;             /* 0x006952a0, also gNgKeyMapSel, gPlayers */
    unsigned char pad_00695338[4];     /* 0x00695338 */
    unsigned char d_0069533c[676];     /* 0x0069533c */
    uint8_t gOptionFlags;              /* 0x006955e0, also gGameUnlockBits, gOptionsAvail, gSaveOptions */
    unsigned char pad_006955e1[1];     /* 0x006955e1 */
    uint8_t gPhysSyncFlags;            /* 0x006955e2, also gActionOptions, gComdataOptions, gOptionsValue */
    unsigned char pad_006955e3[1];     /* 0x006955e3 */
    OvlVolumes gSoundVolumes;          /* 0x006955e4, also gRiderCount, gSoundVolume0, gSoundVolume1 */
    OvlRiderData
        gRiderData; /* 0x006955e8, also gGameRiders, gRiders, gUnlockAmish, gUnlockDave, gUnlockRyan */
    uint8_t gActionLanguage;                /* 0x00696218, also gLanguageCurrent */
    unsigned char pad_00696219[3];          /* 0x00696219 */
    uint8_t gPlayerCount;                   /* 0x0069621c, also gNgPlayerCount, gPhysPlayerCount */
    unsigned char d_0069621d[216];          /* 0x0069621d */
    uint8_t gDemoPlayer;                    /* 0x006962f5 */
    unsigned char pad_006962f6[6];          /* 0x006962f6 */
    PhysMotionTable gPhysMotions;           /* 0x006962fc */
    ActionMotions gActionMotions;           /* 0x00696300 */
    unsigned char pad_006966b8[36];         /* 0x006966b8 */
    uint32_t gOvlSaving;                    /* 0x006966dc */
    unsigned char pad_006966e0[8];          /* 0x006966e0 */
    int32_t rpMeshOffset;                   /* 0x006966e8 */
    int32_t rpMeshModuleCount;              /* 0x006966ec */
    unsigned char pad_006966f0[12];         /* 0x006966f0 */
    int32_t rpPVSSectorOffset;              /* 0x006966fc */
    unsigned char pad_00696700[12];         /* 0x00696700 */
    int32_t rpPVSWorldOffset;               /* 0x0069670c */
    uint32_t rwD3DOrClip;                   /* 0x00696710 */
    unsigned char pad_00696714[12];         /* 0x00696714 */
    uint32_t rwD3DAndClip;                  /* 0x00696720 */
    unsigned char pad_00696724[28];         /* 0x00696724 */
    RwD3DStateCache rwD3DState;             /* 0x00696740 */
    unsigned char pad_006968a0[80];         /* 0x006968a0 */
    const uint8_t *rwD3DCaps;               /* 0x006968f0 */
    unsigned char pad_006968f4[12];         /* 0x006968f4 */
    RwD3DTriFunc rwD3DClipTriCull;          /* 0x00696900 */
    RwD3DTriFunc rwD3DTriCull;              /* 0x00696904 */
    RwD3DTriVoidFunc rwD3DClipTri;          /* 0x00696908 */
    RwD3DLineFunc rwD3DClipLine;            /* 0x0069690c */
    RwD3DVoidFunc rwD3DIndexStart;          /* 0x00696910 */
    RwD3DTriVoidFunc rwD3DIndexTri;         /* 0x00696914 */
    RwD3DLineFunc rwD3DIndexLine;           /* 0x00696918 */
    RwD3DVoidFunc rwD3DIndexFlush;          /* 0x0069691c */
    RwD3DProjectionFunc rwD3DSetProjection; /* 0x00696920 */
    RwD3DUVSetFunc rwD3DSetUVStage;         /* 0x00696924 */
    float rwD3DZScale;                      /* 0x00696928 */
    float rwD3DZShift;                      /* 0x0069692c */
    float rwD3DXScale;                      /* 0x00696930 */
    float rwD3DYScale;                      /* 0x00696934 */
    float rwD3DXOffset;                     /* 0x00696938 */
    float rwD3DYOffset;                     /* 0x0069693c */
    int32_t rwD3DNearZ;                     /* 0x00696940 */
    int32_t rwD3DFarZ;                      /* 0x00696944 */
    float rwD3DNearScreenZ;                 /* 0x00696948 */
    float rwD3DFarScreenZ;                  /* 0x0069694c */
    RwV2d rwD3DRecipWindow;                 /* 0x00696950 */
    RwD3DTLVertex *rwD3DTLBase;             /* 0x00696958 */
    RwD3DCamVertex *rwD3DCamBase;           /* 0x0069695c */
    int32_t rwIm3DGouraud;                  /* 0x00696960 */
    uint32_t rwD3DIm3DDepth;                /* 0x00696964 */
    RwD3DIm3DLevel *rwD3DIm3DLevel;         /* 0x00696968 */
    unsigned char d_0069696c[12];           /* 0x0069696c */
    unsigned char d_00696978[4];            /* 0x00696978 */
    unsigned char d_0069697c[4];            /* 0x0069697c */
    unsigned char d_00696980[220];          /* 0x00696980 */
    uint32_t *rwD3DVertexBits;              /* 0x00696a5c */
    uint32_t rwD3DVertexBitsSize;           /* 0x00696a60 */
    RwD3DIndexList rwD3DTriIndices;         /* 0x00696a64 */
    RwD3DIndexList rwD3DLineIndices;        /* 0x00696a70 */
    RwD3DTLVertex *rwD3DTLBuffer;           /* 0x00696a7c */
    RwD3DCamVertex *rwD3DCamBuffer;         /* 0x00696a80 */
    uint32_t rwD3DBufferSize;               /* 0x00696a84 */
    RwD3DIm3DWork *rwD3DIm3DWork;           /* 0x00696a88 */
    unsigned char pad_00696a8c[116];        /* 0x00696a8c */
    unsigned char d_00696b00[1024];         /* 0x00696b00 */
    unsigned char d_00696f00[4];            /* 0x00696f00 */
    unsigned char d_00696f04[380];          /* 0x00696f04 */
    unsigned char d_00697080[8];            /* 0x00697080 */
    unsigned char d_00697088[4];            /* 0x00697088 */
    unsigned char d_0069708c[4];            /* 0x0069708c */
    unsigned char d_00697090[56];           /* 0x00697090 */
    unsigned char d_006970c8[4];            /* 0x006970c8 */
    unsigned char d_006970cc[4];            /* 0x006970cc */
    unsigned char d_006970d0[4];            /* 0x006970d0 */
    unsigned char d_006970d4[4];            /* 0x006970d4 */
    unsigned char d_006970d8[4];            /* 0x006970d8 */
    unsigned char d_006970dc[4];            /* 0x006970dc */
    unsigned char d_006970e0[4];            /* 0x006970e0 */
    unsigned char d_006970e4[28];           /* 0x006970e4 */
    unsigned char d_00697100[16];           /* 0x00697100 */
    unsigned char d_00697110[56];           /* 0x00697110 */
    unsigned char d_00697148[4];            /* 0x00697148 */
    unsigned char d_0069714c[4];            /* 0x0069714c */
    unsigned char d_00697150[4];            /* 0x00697150 */
    unsigned char d_00697154[4];            /* 0x00697154 */
    unsigned char d_00697158[4];            /* 0x00697158 */
    unsigned char d_0069715c[4];            /* 0x0069715c */
    unsigned char d_00697160[4];            /* 0x00697160 */
    unsigned char d_00697164[28];           /* 0x00697164 */
    unsigned char d_00697180[4];            /* 0x00697180 */
    unsigned char d_00697184[4];            /* 0x00697184 */
    unsigned char d_00697188[4];            /* 0x00697188 */
    unsigned char d_0069718c[52];           /* 0x0069718c */
    unsigned char d_006971c0[4];            /* 0x006971c0 */
    unsigned char d_006971c4[4];            /* 0x006971c4 */
    unsigned char d_006971c8[4];            /* 0x006971c8 */
    unsigned char d_006971cc[4];            /* 0x006971cc */
    unsigned char d_006971d0[4];            /* 0x006971d0 */
    unsigned char d_006971d4[4];            /* 0x006971d4 */
    unsigned char d_006971d8[4];            /* 0x006971d8 */
    unsigned char d_006971dc[4];            /* 0x006971dc */
    unsigned char d_006971e0[4];            /* 0x006971e0 */
    unsigned char d_006971e4[4];            /* 0x006971e4 */
    unsigned char d_006971e8[20];           /* 0x006971e8 */
    unsigned char d_006971fc[4];            /* 0x006971fc */
    unsigned char d_00697200[4];            /* 0x00697200 */
    unsigned char d_00697204[4];            /* 0x00697204 */
    unsigned char d_00697208[4];            /* 0x00697208 */
    unsigned char d_0069720c[60];           /* 0x0069720c */
    unsigned char d_00697248[4];            /* 0x00697248 */
    unsigned char d_0069724c[4];            /* 0x0069724c */
    unsigned char d_00697250[4];            /* 0x00697250 */
    unsigned char d_00697254[4];            /* 0x00697254 */
    unsigned char d_00697258[4];            /* 0x00697258 */
    unsigned char d_0069725c[4];            /* 0x0069725c */
    unsigned char d_00697260[4];            /* 0x00697260 */
    unsigned char d_00697264[4];            /* 0x00697264 */
    unsigned char d_00697268[24];           /* 0x00697268 */
    unsigned char d_00697280[4];            /* 0x00697280 */
    unsigned char d_00697284[4];            /* 0x00697284 */
    unsigned char d_00697288[4];            /* 0x00697288 */
    unsigned char d_0069728c[60];           /* 0x0069728c */
    unsigned char d_006972c8[4];            /* 0x006972c8 */
    unsigned char d_006972cc[4];            /* 0x006972cc */
    unsigned char d_006972d0[4];            /* 0x006972d0 */
    unsigned char d_006972d4[4];            /* 0x006972d4 */
    unsigned char d_006972d8[4];            /* 0x006972d8 */
    unsigned char d_006972dc[4];            /* 0x006972dc */
    unsigned char d_006972e0[4];            /* 0x006972e0 */
    unsigned char d_006972e4[4];            /* 0x006972e4 */
    unsigned char d_006972e8[20];           /* 0x006972e8 */
    unsigned char d_006972fc[4];            /* 0x006972fc */
    unsigned char d_00697300[4];            /* 0x00697300 */
    unsigned char d_00697304[4];            /* 0x00697304 */
    unsigned char d_00697308[4];            /* 0x00697308 */
    void *gDirectDraw;                      /* 0x0069730c */
    unsigned char pad_00697310[16];         /* 0x00697310 */
    unsigned char d_00697320[4];            /* 0x00697320 */
    unsigned char d_00697324[4];            /* 0x00697324 */
    unsigned char d_00697328[4];            /* 0x00697328 */
    unsigned char d_0069732c[4];            /* 0x0069732c */
    unsigned char d_00697330[4];            /* 0x00697330 */
    unsigned char d_00697334[1];            /* 0x00697334 */
    unsigned char d_00697335[1];            /* 0x00697335 */
    unsigned char d_00697336[10];           /* 0x00697336 */
    uint8_t *rwD3DContext;                  /* 0x00697340 */
    unsigned char pad_00697344[4];          /* 0x00697344 */
    RwCamera *rwD3DCurrentCamera;           /* 0x00697348 */
    const RwMemoryFunctions *rwD3DMemory;   /* 0x0069734c */
    unsigned char d_00697350[4];            /* 0x00697350 */
    int32_t rwD3DFogStart;                  /* 0x00697354 */
    float rwD3DFogEnd;                      /* 0x00697358 */
    float rwD3DFogScale;                    /* 0x0069735c */
    unsigned char d_00697360[4];            /* 0x00697360 */
    int32_t rwD3DRasterExtOffset;           /* 0x00697364 */
    int32_t rwD3DCameraExtOffset;           /* 0x00697368 */
    unsigned char pad_0069736c[4];          /* 0x0069736c */
    int32_t rpLodEngineOffset;              /* 0x00697370 */
    int32_t rpLodAtomicOffset;              /* 0x00697374 */
    void *rpLodCamera;                      /* 0x00697378 */
    RwFreeList *rpLodFreeList;              /* 0x0069737c, also rpLodReserved */
    unsigned char pad_00697380[32];         /* 0x00697380 */
    ZMovie gMovie;                          /* 0x006973a0 */
    unsigned char pad_0069762c[148];        /* 0x0069762c */
    void *gZrwRaster;                       /* 0x006976c0, also gRaster, gZrwCamRaster */
    void *gZrwZRaster;                      /* 0x006976c4 */
    void *gZrwWorld;                        /* 0x006976c8, also gZrwDebugLight, gZrwSceneState */
    ZVec3 gZrwWorldCenter;                  /* 0x006976cc, also gZrwOriginX, gZrwOriginY, gZrwOriginZ */
    ZVec3 gZrwWorldCenterScaled;            /* 0x006976d8, also gZrwWorldOffset */
    int gZrwPvsLoaded;                      /* 0x006976e4 */
    int gZrwPvsEnabled;                     /* 0x006976e8, also gDebugLight, gZrwDebugLightOn */
    uint32_t gZrwLightParamA;               /* 0x006976ec */
    uint32_t gZrwLightParamB;               /* 0x006976f0 */
    void *gZrwPrimCamera;                   /* 0x006976f4, also gZrwActiveRw, gZrwCamera */
    int gZrwInUpdate;                       /* 0x006976f8 */
    ZrwColBox gZrwColBox;                   /* 0x006976fc, also gZrwName */
    float gZrwFrameLimit;                   /* 0x0069771c, also gFrameLimit, gRateA */
    float gZrwScale;                        /* 0x00697720, also gZrwInvScale, gZrwWorldScale */
    float gZrwScale_00697724;               /* 0x00697724, also gZrwInvScale */
    uint8_t gZrwFlags;                      /* 0x00697728 */
    unsigned char pad_00697729[3];          /* 0x00697729 */
    int gZrwClearOn;                        /* 0x0069772c, also gClearOn, gZrwCamViewportWord */
    uint32_t gZrwClearColor;  /* 0x00697730, also gClearColor, gZrwCamViewWindowWord, gZrwClearColorRec */
    uint32_t gZrwClearColor2; /* 0x00697734 */
    uint32_t gZrwLastTime;    /* 0x00697738 */
    uint32_t gZrwFpsTime;     /* 0x0069773c */
    int gZrwFps;              /* 0x00697740 */
    int gZrwFrames;           /* 0x00697744 */
    unsigned char pad_00697748[40]; /* 0x00697748 */
    uint32_t gZrwScreenW;           /* 0x00697770, also gZrwCamWidth */
    uint32_t gZrwScreenH;           /* 0x00697774, also gZrwCamHeight */
    uint32_t gZrwRenderFlags;       /* 0x00697778 */
    int gZrwRenderExtra;            /* 0x0069777c */
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
    void *gGdiSurface;              /* 0x006977cc, also gDirectDrawSurface */
    unsigned char pad_006977d0[32]; /* 0x006977d0 */
    unsigned char d_006977f0[128];  /* 0x006977f0 */
    unsigned char d_00697870[12];   /* 0x00697870 */
    unsigned char d_0069787c[4];    /* 0x0069787c */
    unsigned char d_00697880[256];  /* 0x00697880 */
    unsigned char d_00697980[1];    /* 0x00697980 */
    unsigned char d_00697981[259];  /* 0x00697981 */
    unsigned char d_00697a84[28];   /* 0x00697a84 */
    unsigned char d_00697aa0[4];    /* 0x00697aa0 */
    unsigned char d_00697aa4[252];  /* 0x00697aa4 */
    unsigned char d_00697ba0[60];   /* 0x00697ba0 */
    unsigned char d_00697bdc[4];    /* 0x00697bdc */
    unsigned char d_00697be0[832];  /* 0x00697be0 */
    unsigned char d_00697f20[4096]; /* 0x00697f20 */
    unsigned char d_00698f20[4];    /* 0x00698f20 */
    unsigned char d_00698f24[20];   /* 0x00698f24 */
    unsigned char d_00698f38[4];    /* 0x00698f38 */
    unsigned char d_00698f3c[8];    /* 0x00698f3c */
    unsigned char d_00698f44[4];    /* 0x00698f44 */
    unsigned char d_00698f48[4];    /* 0x00698f48 */
    unsigned char d_00698f4c[4];    /* 0x00698f4c */
    unsigned char d_00698f50[4];    /* 0x00698f50 */
    unsigned char d_00698f54[16];   /* 0x00698f54 */
    unsigned char d_00698f64[4];    /* 0x00698f64 */
    unsigned char d_00698f68[16];   /* 0x00698f68 */
    unsigned char d_00698f78[4];    /* 0x00698f78 */
    unsigned char d_00698f7c[4];    /* 0x00698f7c */
    unsigned char d_00698f80[4];    /* 0x00698f80 */
    unsigned char d_00698f84[12];   /* 0x00698f84 */
};

#define AT(member, addr) _Static_assert(offsetof(struct BmxBss, member) == (addr) - 0x0056a000, #member)
AT(gAction, 0x0056b3e0);
AT(gCameraPlayer, 0x0056b408);
AT(gCameraRect, 0x0056b410);
AT(gCameras, 0x0056b448);
AT(gCameraReset, 0x0056b5b8);
AT(gSelLevelSetting, 0x0056b614);
AT(gCurrentPlayer, 0x0056b615);
AT(gGameModeSel, 0x0056b616);
AT(gSelResume, 0x0056b617);
AT(gAutoLoadDone, 0x0056b618);
AT(gContrlerMode, 0x0056b620);
AT(gContrlerPads, 0x0056b628);
AT(gContrlerAxes, 0x0056b6a0);
AT(gFxActive, 0x0056b6c4);
AT(gFxUnused, 0x0056b6c8);
AT(gFxRider, 0x0056b6cc);
AT(gFxZero, 0x0056b6d0);
AT(gGameStartPos, 0x0056b6e8);
AT(gGameReplayAll, 0x0056b6f8);
AT(gGameReplayShared, 0x0056b6f9);
AT(gGameReplays, 0x0056b6fc);
AT(gGameOptions, 0x0056b700);
AT(gGameMode, 0x0056b701);
AT(gGameSubMode, 0x0056b702);
AT(gGameRunCount, 0x0056b703);
AT(gGameRunType, 0x0056b704);
AT(gGamePhase, 0x0056b705);
AT(gGamePhaseTicks, 0x0056b706);
AT(gGameStartPlayer, 0x0056b707);
AT(gGameTimer, 0x0056b708);
AT(gGameRoundStates, 0x0056ba0c);
AT(gGameHorseWord, 0x0056ba1c);
AT(gGameMarkersA, 0x0056ba28);
AT(gGameMarkersB, 0x0056bb28);
AT(gGameExtent, 0x0056bc28);
AT(gGameFrame, 0x0056bc2c);
AT(gGameFrozen, 0x0056bc30);
AT(gOvlEmpty, 0x0056bd44);
AT(gOvlHeightScroll, 0x0056bd48);
AT(gOvlScoreShown, 0x0056bd50);
AT(gOvlScoreTarget, 0x0056bd58);
AT(gOvlScoreFlags, 0x0056bd5c);
AT(gOvlLastScreen, 0x0056bd60);
AT(gOvlTrickCount, 0x0056bd68);
AT(gOvlTrickList, 0x0056bd6c);
AT(gOvlFrame, 0x0056be14);
AT(gOvlFlash, 0x0056be18);
AT(gOvlFlashHandles, 0x0056be20);
AT(gOvlImgTiny, 0x0056be28);
AT(gOvlBackdrop, 0x0056be2a);
AT(gOvlScoreHandle, 0x0056be2c);
AT(gOvlResA, 0x0056be30);
AT(gOvlReturnScreen, 0x0056be32);
AT(gOvlStrings, 0x0056be34);
AT(gOvlSharedPad, 0x0056be3c);
AT(gOvlRunTimes, 0x0056be40);
AT(gOvlIconHandle, 0x0056be90);
AT(gOvlImgIcons, 0x0056be92);
AT(gPhysHudHandle, 0x0056be94);
AT(gOvlFontTiny, 0x0056be98);
AT(gOvlVolHandles, 0x0056bea4);
AT(gOvlGoalHandles, 0x0056beb4);
AT(gOvlScreenId, 0x0056beba);
AT(gOvlGoalRows, 0x0056bec4);
AT(gOvlCaptionCurrent, 0x0056beec);
AT(gOvlCaptionWanted, 0x0056beed);
AT(gOvlImgSmall, 0x0056bef4);
AT(gOvlBackdropRect, 0x0056bef8);
AT(gOvlMenuItems, 0x0056befc);
AT(gOvlBalanceValue, 0x0056bf18);
AT(gOvlScreen, 0x0056bf20);
AT(gOvlHeightHandles, 0x0056bf24);
AT(gOvlPage, 0x0056bf30);
AT(gPhysHudFont, 0x0056bf38);
AT(gOvlHint, 0x0056bf44);
AT(gOvlGoalMsg, 0x0056bf4c);
AT(gOvlTimerShownTicks, 0x0056bf54);
AT(gOvlTimerTicks, 0x0056bf58);
AT(gOvlIndicator, 0x0056bf5c);
AT(gOvlNameCursor, 0x0056bf64);
AT(gOvlPulse, 0x0056bf68);
AT(gOvlTrickRows, 0x0056bf6c);
AT(gOvlFontIcons, 0x0056bf98);
AT(gOvlDrawDelay, 0x0056bfa4);
AT(gOvlStatusHandle, 0x0056bfa8);
AT(gOvlTimerText, 0x0056bfac);
AT(gOvlHeightValue, 0x0056bfc0);
AT(gOvlHeightTarget, 0x0056bfc4);
AT(gOvlHeightKind, 0x0056bfca);
AT(gOvlElement8Value, 0x0056bfd9);
AT(gOvlHintHandles, 0x0056bfdc);
AT(gOvlBalanceHandles, 0x0056bfec);
AT(gOvlImgButton, 0x0056bffa);
AT(gOvlCaptionHandle, 0x0056bffc);
AT(gOvlTimerHandle, 0x0056c000);
AT(gOvlFontButton, 0x0056c008);
AT(gOvlStatus, 0x0056c018);
AT(gOvlMenuItems2, 0x0056c028);
AT(gOvlFade, 0x0056c050);
AT(gOvlTrickView, 0x0056c070);
AT(gOvlSaveSlot, 0x0056c074);
AT(gPhysTick, 0x0056c07c);
AT(gPhysRay, 0x0056c128);
AT(gPhysBestEdge, 0x0056c200);
AT(gPhysAlignEdge, 0x0056c238);
AT(gPhysNearEdge, 0x0056c2c0);
AT(gPhysEdgeProbe, 0x0056c2f8);
AT(gPhysGrindEdge, 0x0056c330);
AT(gPhysLipFace, 0x0056c3b8);
AT(gPhysForceStep, 0x0056c440);
AT(gPhysDebugDraw, 0x0056c444);
AT(gPhysDebugEdges, 0x0056c448);
AT(gPhysDebugFrame, 0x0056c44c);
AT(gPropDefaults, 0x0056c4f8);
AT(gReplays, 0x0056d278);
AT(gGuiMarkupTarget, 0x0056d290);
AT(gGuiPad, 0x0056d298);
AT(gGuiElements, 0x0056d2c0);
AT(gGuiScreenId, 0x00573080);
AT(gGuiScreen, 0x00573088);
AT(gGuiDefaultHook, 0x005730a0);
AT(gGuiSorted, 0x005730a4);
AT(gGuiInitialized, 0x00573234);
AT(gGuiModeReady, 0x00573238);
AT(gGuiElementCount, 0x0057323c);
AT(gGuiScreenLoaded, 0x00573240);
AT(gGuiReloadPending, 0x00573244);
AT(gGuiReloadDelay, 0x00573248);
AT(gGuiSkipUpdate, 0x0057324c);
AT(gGuiAssetsLoaded, 0x00573250);
AT(gGuiPulse, 0x00573254);
AT(gGuiPulseHold, 0x00573258);
AT(gGuiSlideState, 0x0057325c);
AT(gGuiSlideMax, 0x00573260);
AT(gOptMainPulse, 0x00574584);
AT(gSelPreviewFn, 0x00574588);
AT(gFrontendStep, 0x0057458c);
AT(gSelCommitFn, 0x00574590);
AT(gMenuFrameSprite, 0x00574594);
AT(gMcDone, 0x0057459c);
AT(gTitleSel, 0x005745a0);
AT(gTitleAction, 0x005745a4);
AT(gAutoLoadOk, 0x005745a8);
AT(gAutoTick, 0x005745ac);
AT(gTitleStage, 0x005745ae);
AT(gKbWork, 0x005745b4);
AT(gKbCursor, 0x005745e0);
AT(gKbState, 0x005745e1);
AT(gKbLayout, 0x005745e2);
AT(gStyleSprites, 0x005745ec);
AT(gStyleFrame, 0x0057462c);
AT(gExtrasSel, 0x00574644);
AT(gExtrasPlayed, 0x00574648);
AT(gAssetSetData, 0x0057464c);
AT(gExitFlag, 0x00574654);
AT(gExitCode, 0x00574658);
AT(gMacroBuf, 0x0057465c);
AT(gMacroSize, 0x00574660);
AT(gBmxCurrentTrack, 0x00574668);
AT(gBmxTimerId, 0x00574674);
AT(gBmxAmbientTracks, 0x00574678);
AT(gBmxSoundEffect, 0x00574680);
AT(gBmxTimerPeriod, 0x00574694);
AT(gBmxGunChannel, 0x00574698);
AT(gBmxMenuChannels, 0x0057469c);
AT(gBmxLevelIndex, 0x0057469e);
AT(gBmxRiderIndex, 0x0057469f);
AT(gBmxAmbientSide, 0x005746a0);
AT(gBmxMusicStream, 0x005746a4);
AT(gBmxTrackCooldown, 0x005746a8);
AT(gBmxMenuSfxSet, 0x005746ac);
AT(gLastPollTime, 0x005746b0);
AT(gZoverArmSide, 0x005746c4);
AT(gZoverLimb3Elbow, 0x005746d4);
AT(gZoverLimb1Upper, 0x005746e4);
AT(gZoverLimb0Elbow, 0x00574724);
AT(gZoverRelaxed, 0x00574734);
AT(gZoverStiff, 0x0057473c);
AT(gZoverLimb3Target, 0x00574744);
AT(gZoverLimb0Lower, 0x00574754);
AT(gZoverFrameShin46, 0x00574794);
AT(gZoverLimb0Bend, 0x005747d4);
AT(gZoverLimb0Target, 0x005747e4);
AT(gZoverFrameShin35, 0x005747f4);
AT(gZoverLimb1Target, 0x00574834);
AT(gZoverLimb2Target, 0x00574844);
AT(gZoverFrameForearm911, 0x00574854);
AT(gZoverSpineFrame, 0x00574894);
AT(gZoverHandsEased, 0x005748d4);
AT(gZoverLimb2Root, 0x005748dc);
AT(gZoverLimb2Elbow, 0x005748ec);
AT(gZoverLimb1Elbow, 0x005748fc);
AT(gZoverLimb3Bend, 0x0057490c);
AT(gZoverHandsGrabbed, 0x0057491c);
AT(gZoverHandsHolding, 0x00574924);
AT(gZoverArmDist, 0x0057492c);
AT(gZoverLimp, 0x00574930);
AT(gZoverFrameForearm1012, 0x00574938);
AT(gZoverLimb3Upper, 0x00574978);
AT(gZoverLimb1Root, 0x005749b8);
AT(gZoverLimb2Upper, 0x005749c8);
AT(gZoverLimbUpperLen, 0x00574a08);
AT(gZoverLimbSin, 0x00574a0c);
AT(gZoverLimb1Bend, 0x00574a10);
AT(gZoverLegSide, 0x00574a20);
AT(gZoverLimb0Root, 0x00574a30);
AT(gZoverLimbAngle, 0x00574a40);
AT(gZoverLimb3Root, 0x00574a44);
AT(gZoverFrameHead, 0x00574a54);
AT(gZoverLimb1Lower, 0x00574a94);
AT(gZoverSticky, 0x00574ad4);
AT(gZoverActorFrame, 0x00574adc);
AT(gZoverYawOn, 0x00574b1c);
AT(gZoverPitchOn, 0x00574b24);
AT(gZoverLimbProduct, 0x00574b2c);
AT(gZoverLimbDir, 0x00574b6c);
AT(gZoverLegDist, 0x00574b7c);
AT(gZoverLimbCos, 0x00574b80);
AT(gZoverLimb3Lower, 0x00574b84);
AT(gZoverRigs, 0x00574bc4);
AT(gZoverLimbDistCopy, 0x00574bcc);
AT(gZoverLimb2Lower, 0x00574bd0);
AT(gZoverLimbTransposed, 0x00574c10);
AT(gZoverLimbLowerLen, 0x00574c50);
AT(gZoverLimb2Bend, 0x00574c54);
AT(gZoverLimb0Upper, 0x00574c64);
AT(gZoverRigNext, 0x00574ca4);
AT(gZoverRigIndex, 0x00574ca8);
AT(gZoverPitch, 0x00574cac);
AT(gZoverYaw, 0x00574cb0);
AT(gZoverHingeIdx, 0x00574cb4);
AT(gZoverStickIdx, 0x00574ccc);
AT(gZoverBackIdx, 0x00574cec);
AT(gZoverLimbOffset, 0x00574cf8);
AT(gZoverLimbSynced, 0x00574d08);
AT(gZoverFonts, 0x00574e50);
AT(gZoverPool3, 0x00574e70);
AT(gZoverPool2, 0x00594270);
AT(gZoverRectSize, 0x00596570);
AT(gZoverFreeList, 0x005965a4);
AT(gZoverTerminators, 0x00596a20);
AT(gZoverFontCount, 0x00596a30);
AT(gZoverPool1, 0x00596a38);
AT(gZoverPool0, 0x005a0a38);
AT(gZoverDirtyList, 0x005a1e38);
AT(gZoverBusy, 0x005a22b0);
AT(gZoverDirtyCount, 0x005a22b4);
AT(gZoverFreeCount, 0x005a22b8);
AT(gZoverFrame, 0x005a22bc);
AT(gZoverLayer, 0x005a22bd);
AT(gZsystemMode, 0x005ab3a0);
AT(gNgSpotLights, 0x005ab3ac);
AT(gNgArtName, 0x005ab3b8);
AT(gNgLights, 0x005ab438);
AT(gNgElapsed, 0x005ab454);
AT(gNgMainLight, 0x005ab458);
AT(gNgWake, 0x005ab460);
AT(gNgPrevMark, 0x005ab468);
AT(gNgArt, 0x005ab46c);
AT(gNgLoadNode, 0x005ab470);
AT(gMenuInstant, 0x005ab474);
AT(gNgCountdown, 0x005ab478);
AT(gNgSunTick, 0x005ab47c);
AT(gNgLoadFlag, 0x005ab480);
AT(gNgLegal1, 0x005ab488);
AT(gNgLoadTexture, 0x005ab48c);
AT(gNgLegal2, 0x005ab490);
AT(gFxQualityTime, 0x005ab49c);
AT(gFxFrames, 0x005ab4a0);
AT(gFxAdjusting, 0x005ab4a4);
AT(gFxQualityStep, 0x005ab4a8);
AT(gFxSparkTexture, 0x005ab4e0);
AT(gFxSkidTexture, 0x005ab4e4);
AT(gFxRippleTexture, 0x005ab4e8);
AT(gFxSplashTexture, 0x005ab4ec);
AT(gNgifScript, 0x005ab4f0);
AT(gFxCube, 0x005ab4f4);
AT(gNgifPrimBuffer, 0x005ab4fc);
AT(gNgifFlushPending, 0x005d24fc);
AT(gNgifFlushNode, 0x005d2500);
AT(gNgifFlushPending2, 0x005d2504);
AT(gNgifFlushNode2, 0x005d2508);
AT(gNgifPrimStart, 0x005d250c);
AT(gPrimCursor, 0x005d2510);
AT(gZactorPose, 0x005da1e8);
AT(gZactorArmHashes, 0x005da2c8);
AT(gZbspNearHits, 0x005e63d8);
AT(gZbspMatRefValue, 0x005e7fd8);
AT(gZbspPropFaces, 0x005e7fe0);
AT(gZbspNearFaces, 0x005e8420);
AT(gZbspSphere, 0x005f0c20);
AT(gZbspFaceQuery, 0x005f0c28);
AT(gZbspHits, 0x005f0c68);
AT(gZbspEdgeRecord, 0x005f1070);
AT(gZbspNearCorners, 0x005f10a8);
AT(gZresListA, 0x005f34ac);
AT(gZresListB, 0x005f34b0);
AT(gFxLightCallback, 0x005f34b4);
AT(gFxListCallback, 0x005f34b8);
AT(gZbspPropFaceCount, 0x005f34bc);
AT(gZcollideProbeMaterial, 0x005f8518);
AT(gZcollideEdgeList, 0x005f8520);
AT(gZcollideEdgeRecords, 0x005f85a8);
AT(gZcollideEdgeCells, 0x005f8ce0);
AT(gZbspNoShapeKind, 0x005f8d68);
AT(gZmemdmm, 0x005f8f30);
AT(gZres, 0x005f8f60);
AT(gZrollerDrawCallbacks, 0x005fb6c8);
AT(gZrollerReserved, 0x005fb6e8);
AT(gZrollerSlots, 0x005fb6f0);
AT(gZrollerShadowSlots, 0x005fb710);
AT(gZrollerBounds, 0x005fb718);
AT(gZrollerSlotCount, 0x005fb738);
AT(gNgWord5fb73c, 0x005fb73c);
AT(gNgRollerNode, 0x005fb740);
AT(gWinchanBuffer, 0x005fb760);
AT(gWinchanFlangerDelay, 0x005fb764);
AT(gWinchanActive, 0x005fb768);
AT(gWinchanEffectMaxFrames, 0x005fb76c);
AT(gWinchanChannels, 0x005fb770);
AT(gWinchanMixRight, 0x005fbe30);
AT(gWinchanEffectAGain, 0x005fbe34);
AT(gWinchanMixLeft, 0x005fbeb8);
AT(gWinchanFlangerLine, 0x005fbebc);
AT(gWinchanCopyOffset, 0x005fdebc);
AT(gWinchanWriteOffset, 0x005fdec0);
AT(gWinchanEchoPos, 0x005fdec4);
AT(gWinchanLfoTable, 0x005fdec8);
AT(gWinchanFlangerPhase, 0x005ffecc);
AT(gWinchanEchoBuffer, 0x005ffed0);
AT(gWinchanDsoundModule, 0x005ffed4);
AT(gWinchanRing, 0x005ffed8);
AT(gWinchanChunkBytes, 0x005ffedc);
AT(gWinchanEffectBFrames, 0x005ffee0);
AT(gWinchanRate, 0x005ffee4);
AT(gWinchanEffectBGain, 0x005ffee8);
AT(gWinchanEffectAFrames, 0x005ffeec);
AT(gWinchanEchoFeedback, 0x005ffef0);
AT(gWinchanMixOffset, 0x005ffef4);
AT(gWinchanLastPlay, 0x005ffef8);
AT(gWinchanEffectBTailStart, 0x005ffefc);
AT(gWinchanFlangerPos, 0x005fff00);
AT(gWinchanEffectBTail, 0x005fff04);
AT(gWinchanRingBytes, 0x005fff08);
AT(gWinchanInitialized, 0x005fff0c);
AT(gWinchanLock, 0x005fff10);
AT(gWinchanNextChannel, 0x005fff14);
AT(gWinchanNextHandle, 0x005fff18);
AT(gWinchanDsoundCreate, 0x005fff1c);
AT(gWinchanPrimary, 0x005fff20);
AT(gWinchanPumpCount, 0x005fff24);
AT(gWinstrmDirectSound, 0x005fff28);
AT(gWinloadSampleFile, 0x00600d88);
AT(gWinloadFiles, 0x00600d98);
AT(gWinloadCounts, 0x00601218);
AT(gWinstrmSourceHint, 0x00601328);
AT(gWinstrmLock, 0x00601330);
AT(gWinstrmStreamHint, 0x00601348);
AT(gWinstrmSources, 0x00601350);
AT(gWinstrmStreams, 0x006017d0);
AT(gWinstrmNodeHint, 0x006019f0);
AT(gWinstrmNodes, 0x006019f8);
AT(gWinstrmInitialized, 0x00601b78);
AT(gWinstrmPaused, 0x00601b7c);
AT(gWinstrmTick, 0x00601b80);
AT(gActorRootPoseTemp, 0x00601d30);
AT(gMotionLastDelta, 0x00601d48);
AT(gMotionEntryTemp, 0x00601d60);
AT(gMotionDeltaA, 0x00601d68);
AT(gMotionDeltaB, 0x00601d80);
AT(gMotionDeltaC, 0x00601d98);
AT(gMotionDeltaD, 0x00601db0);
AT(gMotionDeltaE, 0x00601de0);
AT(gMotionPrevYawE, 0x00601e08);
AT(gMotionFirstBank, 0x00601e10);
AT(gMotionSampleA, 0x00601e14);
AT(gMotionSampleB, 0x00601e18);
AT(gMotionStaging, 0x00601e1c);
AT(gZactors, 0x00641e1c);
AT(gMotionBankCount, 0x00641e20);
AT(gActorAngleBase, 0x00641e24);
AT(gActorAngleFlip, 0x00641e28);
AT(gMotionLibrary, 0x00641e2c);
AT(gMotionBuffer, 0x00641e30);
AT(gZprimTextureCounter, 0x0064eb34);
AT(gZsndNotes, 0x0064eb38);
AT(gZsndNextNote, 0x0064f038);
AT(gZsndMuteAll, 0x0064f03c);
AT(gZsndNextTrack, 0x0064f040);
AT(gZsndTrackPool, 0x0064f048);
AT(gZsndTracks, 0x0064f948);
AT(gZsndSeqLock, 0x0064f9c8);
AT(gZsndStream, 0x0064fcb8);
AT(gZsndStreamBytes, 0x0064fcbc);
AT(gZsndSrcNoDoppler, 0x0064fcc8);
AT(gZsndSrcRange, 0x0064fccc);
AT(gZsndSources, 0x0064fcd0);
AT(gZsndSrcDopplerScale, 0x00650090);
AT(gZsndSrcRange2, 0x00650094);
AT(gZsndSrcInitialized, 0x00650098);
AT(gZsndSrcNext, 0x0065009c);
AT(gZsndSrcLifetime, 0x006500a0);
AT(gZdebugCreated, 0x00650170);
AT(gScratch, 0x00650180);
AT(gZrwDbgInitialized, 0x00650470);
AT(gZrwDbgFpsMode, 0x00650474);
AT(gZrwDbgUpdateNode, 0x00650478);
AT(gZrwDbgCallbacks, 0x0065047c);
AT(gZrwDbgCharset, 0x00650488);
AT(gZrwDbgCharWidth, 0x0065048c);
AT(gZrwDbgCharHeight, 0x00650490);
AT(gFormatBuffer, 0x00650494);
AT(gTextQueue, 0x00650694);
AT(gTextEnd, 0x00650a94);
AT(gZrwDbgItemCount, 0x00650a98);
AT(gItems, 0x00650a9c);
AT(gZdebugIndentLevel, 0x0065e29c);
AT(gSphereVerts, 0x0065e2a0);
AT(gSpherePointers, 0x0065e2a4);
AT(gZinputReady, 0x0065e2a8);
AT(gZpadDevices, 0x0065e2ac);
AT(gZassetLibraries, 0x0065e2b0);
AT(gZmathTables, 0x0065e2bc);
AT(gZmathOrigin, 0x006702d8);
AT(gZobjCameraInitialized, 0x006702f8);
AT(gZobjCameraCaches, 0x006702fc);
AT(gZobjCameraTarget, 0x00670300);
AT(gZstrTempBuffer, 0x006703d8);
AT(gZstringSets, 0x006704d8);
AT(gNameTablesReady, 0x00670518);
AT(gNameTables, 0x0067051c);
AT(gZfileErrorMessage, 0x00670528);
AT(gZfileDefaultDevice, 0x00670568);
AT(gZfileNameBuffer, 0x006705ac);
AT(gZfileDevices, 0x00670630);
AT(gZfileInitialized, 0x00670674);
AT(gZfileHandles, 0x00670678);
AT(gZfileLoadPool, 0x0067067c);
AT(gZmemFreeListInUse, 0x00670680);
AT(gZmem, 0x00670880);
AT(gZmemFreeLists, 0x006708a8);
AT(gZmemHooks, 0x006718a8);
AT(gZmemHooksInstalled, 0x006722b0);
AT(gZpropDefs, 0x006722b8);
AT(gZpropInsts, 0x006722c4);
AT(gZobjInitialized, 0x006722d0);
AT(gZobjPool, 0x006722d4);
AT(gZobjLinkPool, 0x006722d8);
AT(gZobjDebugNode, 0x006722dc);
AT(gZobjUpdateNode, 0x006722e0);
AT(gZrwModelState, 0x006722e8);
AT(gZrwOrigin, 0x006722f8);
AT(gZobjModelReady, 0x00672308);
AT(gZrwModelFlag2, 0x0067230c);
AT(gZrwMat, 0x006723a8);
AT(gEpicMessage, 0x006723d0);
AT(gZglSubSystemNames, 0x006724d0);
AT(gZglSubSystemCount, 0x00672750);
AT(gZglSubSystem, 0x00672754);
AT(gZglVideoMode, 0x00672758);
AT(gLastTick, 0x0067275c);
AT(gZrwInitialized, 0x00672760);
AT(gZrwArgument, 0x00672764);
AT(gZrwUpdateLists, 0x00672768);
AT(gZrwStateStack, 0x006727d4);
AT(gZrwStateDepth, 0x006727f4);
AT(gZrwNodePool, 0x006727f8);
AT(gZrwSkipDevice, 0x006727fc);
AT(gZrwEngineOpen, 0x00672800);
AT(gZrwEngineInit, 0x00672804);
AT(gZrwArgBuffer, 0x00672808);
AT(gNoMovies, 0x0067288c);
AT(gZrwPrimFrame, 0x00672890);
AT(gZrwPrim, 0x00672898);
AT(gZrwPrimDepthBias, 0x006728b0);
AT(gZtaginfUnused, 0x006728b8);
AT(gZtaginfFaceCount, 0x006728bc);
AT(gZtaginfFaces, 0x006728c0);
AT(gZtaginfRoot, 0x006728c4);
AT(gZtaginfData, 0x006728fc);
AT(gZrwColDebug, 0x00672900);
AT(gZrwColList, 0x00672908);
AT(gZrwColHit, 0x00673708);
AT(gZrwColQuery, 0x006737b0);
AT(gZtaginfNodes, 0x006737b8);
AT(gZrwModelRender, 0x00673e80);
AT(gZobjLightInitialized, 0x00673f68);
AT(gScene, 0x00673f70);
AT(gScnpts, 0x00674000);
AT(gZrwCamReady, 0x00674018);
AT(gZrwCamPool, 0x0067401c);
AT(gZrwCurrentCamera, 0x00674020);
AT(gZpadDiActive, 0x00674028);
AT(gZpadDiRecords, 0x0067402c);
AT(gZpadKeyboardOk, 0x00674030);
AT(gZpadWindow, 0x00674034);
AT(gZpadDirectInput, 0x00674038);
AT(gZpadJoystickGuid, 0x0067403c);
AT(gZpadKeyboard, 0x0067404c);
AT(gZpadKeyboardFlags, 0x00674050);
AT(gZpadKeyState, 0x00674054);
AT(gZrwColColor, 0x00674158);
AT(gZrwCol, 0x00674160);
AT(gZrwColEnabled, 0x006741b0);
AT(gZobjPEmitInitialized, 0x006741b4);
AT(gZrwWorldRecord, 0x006741b8);
AT(gZrwWorldReady, 0x006741e0);
AT(gZrwWorldActive, 0x006741e4);
AT(gZrwWorldMaterials, 0x006741e8);
AT(gZrwWorldStats, 0x006741ec);
AT(gZrwWorldName, 0x00674208);
AT(gZrwTxr, 0x00674290);
AT(gZanimInitialized, 0x006746c0);
AT(gZiffItemPool, 0x006746c4);
AT(gZiffAnim, 0x006746c8);
AT(gZiff, 0x006746d0);
AT(gZrwShadowPoints, 0x006746f0);
AT(gZrwShadowQuads, 0x00674930);
AT(gZrwShadowActive, 0x00674b28);
AT(gZrwShadowOffsets, 0x00674b34);
AT(gZrwShadowVertices, 0x00674d74);
AT(gZrwShadowMiss, 0x00675164);
AT(gZrwShadowTexture, 0x006751f4);
AT(gZrwShadowNode, 0x006751f8);
AT(gZrwShadowReady, 0x006751fc);
AT(gZrwShadowScale, 0x00675200);
AT(gZrwShadowDouble, 0x00675204);
AT(gZrwShadowInit, 0x0067520c);
AT(gZrwModelCreate, 0x00675218);
AT(gZrwNameBuffer, 0x00675240);
AT(gZrwFileOps, 0x006752c0);
AT(gZrwFileInitialized, 0x006752e8);
AT(gZramdiskName, 0x006752ec);
AT(gZramdiskFiles, 0x0067536c);
AT(gZrwLightInitialized, 0x00675370);
AT(gZrwLightPool, 0x00675374);
AT(gZgeInitialized, 0x00675378);
AT(gPanorama, 0x0067537c);
AT(gToken, 0x00675380);
AT(gFont, 0x006763d0);
AT(gZrwRamDiskFile, 0x006763e8);
AT(gZrwRamDiskInfoSlots, 0x006763ec);
AT(gZrwRamDiskNoInfo, 0x00676404);
AT(gZrwRamDiskInfo, 0x00676408);
AT(gZrwSavedFileInterface, 0x00676410);
AT(gZProfile, 0x00676438);
AT(gZtimerSlots, 0x006764a0);
AT(gZtimerHeap, 0x006764a4);
AT(gZtimerFree, 0x006764b4);
AT(gZtimerUpdate, 0x006764c0);
AT(gZtimerReady, 0x006764c4);
AT(gPcProfileClock, 0x006764d0);
AT(gZtimerPopped, 0x006764e0);
AT(gCrtFpStatus, 0x006764ec);
AT(rpClumpModuleOffset, 0x00676710);
AT(rpClumpModuleCount, 0x00676714);
AT(rpGeometryOffset, 0x00676720);
AT(rpGeometryModuleCount, 0x00676724);
AT(rpTieOffset, 0x00676730);
AT(rpTieModuleCount, 0x00676734);
AT(rpWorldCameraOffset, 0x00676738);
AT(rpWorldAtomicOffset, 0x0067673c);
AT(rpWorldClumpOffset, 0x00676740);
AT(rpWorldLightOffset, 0x00676744);
AT(rpMaterialOffset, 0x00676750);
AT(rpMaterialModuleCount, 0x00676754);
AT(rpWorldGlobalsOffset, 0x00676760);
AT(rpWorldModuleCount, 0x00676764);
AT(rpWorldDefaultSurfaceProps, 0x00676768);
AT(rpBspModuleCount, 0x00676784);
AT(rpSectorModuleCount, 0x00676794);
AT(rpLightOffset, 0x006767a0);
AT(rpLightModuleCount, 0x006767a4);
AT(rpBinaryWorldModuleCount, 0x006767b4);
AT(rpMeshEmptyHeader, 0x006767c0);
AT(rpMeshStripMode, 0x006767d0);
AT(rpMaterialListModuleCount, 0x006767e4);
AT(rpPVSWorld, 0x00676810);
AT(rpPVSCamera, 0x00676814);
AT(rpPVSImage, 0x00676818);
AT(RwDefaultGlobals, 0x00676820);
AT(rwEngineInstanceCount, 0x00676968);
AT(RwEngineInstance, 0x0067696c);
AT(rwMatrixOffset, 0x00676970);
AT(rwMatrixModuleCount, 0x00676974);
AT(rwFrameOffset, 0x00676980);
AT(rwFrameModuleCount, 0x00676984);
AT(rwFastSqrtOffset, 0x00676990);
AT(rwVectorModuleCount, 0x00676994);
AT(rwCameraFpuSet, 0x006769a0);
AT(rwD3DFlipPending, 0x00676ba4);
AT(rwD3DDefer, 0x00676bac);
AT(rwD3DSortNeed, 0x00676bb0);
AT(rwD3DSortCap, 0x00676bb4);
AT(rwD3DSortCount, 0x00676bb8);
AT(rwD3DSortItems, 0x00676bbc);
AT(rwD3DAlphaTestCaps, 0x00676cc4);
AT(rwD3DMergeCap, 0x00676cc8);
AT(rwD3DMergeBuf, 0x00676ccc);
AT(rwD3DRemapCap, 0x00676cd0);
AT(rwD3DRemap, 0x00676cd4);
AT(rwD3DRemapUsed, 0x00676cd8);
AT(rwD3DEngineDevice, 0x00676cdc);
AT(rwFreeListHead, 0x00676ce0);
AT(rwResourcesOffset, 0x00676cf0);
AT(rwResourcesModuleCount, 0x00676cf4);
AT(rwRasterOffset, 0x00676d00);
AT(rwRasterModuleCount, 0x00676d04);
AT(rwCameraOffset, 0x00676d10);
AT(rwCameraModuleCount, 0x00676d14);
AT(rwErrorOffset, 0x00676d20);
AT(rwErrorModuleCount, 0x00676d24);
AT(rwIm3DPipelineOffset, 0x00676d30);
AT(rwIm3DModuleCount, 0x00676d34);
AT(rwTextureOffset, 0x00676d40);
AT(rwTextureModuleCount, 0x00676d44);
AT(rwStreamOffset, 0x00676d50);
AT(rwStreamModuleCount, 0x00676d54);
AT(rwImageOffset, 0x00676d60);
AT(rwImageModuleCount, 0x00676d64);
AT(rwIm3DOffset, 0x00676d70);
AT(rwRenderPipelineModuleCount, 0x00676d74);
AT(rwPluginRegEntryFreeList, 0x00676d80);
AT(rwEmptyString, 0x00676d90);
AT(rwColorModuleCount, 0x00676da4);
AT(rwD3DXformStride, 0x00678270);
AT(rwD3DXformMatrix, 0x00678274);
AT(rwD3DXformOut, 0x00678278);
AT(rwD3DXformFog, 0x0067827c);
AT(rpAnimEngineOffset, 0x00678280);
AT(rpAnimModuleCount, 0x00678284);
AT(rpAnimFrameOffset, 0x00678288);
AT(rpAnimClumpOffset, 0x0067828c);
AT(rpBoneFrameOffset, 0x00678290);
AT(rpBoneEngineOffset, 0x00678298);
AT(rpBoneModuleCount, 0x0067829c);
AT(rpBoneStreamResult, 0x006782a0);
AT(rpBoneGeometryOffset, 0x006782a4);
AT(gZprimHandle, 0x006783fc);
AT(gZprimLibrary, 0x00678400);
AT(gZprimPool, 0x00678404);
AT(gZprimFree, 0x00678408);
AT(gZprimUsed, 0x0067840c);
AT(gZprimClip, 0x00678410);
AT(gZprimGroups, 0x00678418);
AT(gZprimNextId, 0x00678428);
AT(gZprimDepth, 0x0067842c);
AT(gZprimStates, 0x00678430);
AT(gZrollerShadow, 0x00678440);
AT(gZresTablesLib, 0x0067847c);
AT(gZresMaterialLib, 0x0067848c);
AT(gZresMiscLib, 0x00678490);
AT(gZrollerEdges, 0x00678494);
AT(gNgifPrim3DBuffer, 0x00678560);
AT(gNgifOrderTable, 0x0068c3d0);
AT(gZrollerModels, 0x0068c3e0);
AT(gNgifPrim3D, 0x0068ebe0);
AT(gNgifColor, 0x0068fc00);
AT(gFxDustTextures, 0x0068fc20);
AT(gFxAnimHandle, 0x0068fcf8);
AT(gNgifScriptNode, 0x0068fcfc);
AT(gFxSkids, 0x0068fd00);
AT(gFxWorldObject, 0x0068ff80);
AT(gNgWord68ff84, 0x0068ff84);
AT(gZprimFontPageList, 0x0068ff94);
AT(gNgCamera, 0x0068ff98);
AT(gNgFlags68ffa0, 0x0068ffa0);
AT(gNgFlags68ffa1, 0x0068ffa1);
AT(gNgFlags68ffa2, 0x0068ffa2);
AT(gNgFlags68ffa3, 0x0068ffa3);
AT(gNgTickNode, 0x0068ffa4);
AT(gZresReplayLib, 0x0068ffa8);
AT(gZoverSweep, 0x0068ffc0);
AT(gExtrasTitles, 0x0068fff0);
AT(gHiscoreTimes, 0x006904d0);
AT(gAssetSetSize, 0x00690890);
AT(gHiscoreWait, 0x00690894);
AT(gExtrasWait2, 0x00690896);
AT(gInfoWait, 0x00690898);
AT(gHiscoreLabels, 0x00690990);
AT(gHiscoreRiders, 0x00690e50);
AT(gInfoSprite, 0x00691210);
AT(gExtrasChoice, 0x00691214);
AT(gInfoPage, 0x00691218);
AT(gInfoSlide, 0x00691220);
AT(gExtrasReady, 0x00691228);
AT(gInfoTicks, 0x0069122c);
AT(gExtrasSlide, 0x00691230);
AT(gHiscoreHandle, 0x00691238);
AT(gHiscoreSprite, 0x0069123c);
AT(gHiscoreNames, 0x00691330);
AT(gExtrasCount, 0x006916f0);
AT(gHiscoreBlink, 0x006916f2);
AT(gHiscoreSlide, 0x006916f8);
AT(gExtrasWait, 0x00691700);
AT(gSelTick, 0x00691704);
AT(gSelBikeCount, 0x0069181f);
AT(gSelSlide, 0x00691820);
AT(gSelReadyTick, 0x0069182c);
AT(gSelPage, 0x00691830);
AT(gSelExtentA, 0x00691834);
AT(gSelExtentB, 0x00691836);
AT(gSelSlideDir, 0x00691838);
AT(gSelStyleCount, 0x0069183c);
AT(gCaptionSlots, 0x00691840);
AT(gSelRider, 0x00691ca0);
AT(gSelPlayer, 0x00691ca8);
AT(gCaptionPtr, 0x00691cac);
AT(gSelStage, 0x00691cb0);
AT(gCheatRing, 0x00691cc0);
AT(gCaptionOn, 0x00691ce0);
AT(gSelPages, 0x00691ce4);
AT(gSelPicturesOn, 0x00691ce8);
AT(gSelExtentC, 0x00691cec);
AT(gSelDimA, 0x00691cee);
AT(gCheatPos, 0x00691cf0);
AT(gSelFlipTick, 0x00691cf4);
AT(gSelSprites, 0x00691cf8);
AT(gSelDimB, 0x00691d04);
AT(gCaptionLine, 0x00691d06);
AT(gSelLevelCount, 0x00691d08);
AT(gSelStyle, 0x00691d58);
AT(gSelUnknown60, 0x00691d60);
AT(gCaptionColors, 0x00691d80);
AT(gSelLevel, 0x00691d94);
AT(gSelBike, 0x00691d98);
AT(gSelUnknownA0, 0x00691da0);
AT(gCaptionTime, 0x00691da4);
AT(gCaptionAlpha, 0x00691da6);
AT(gCaptionWidths, 0x00691db0);
AT(gCaptionHold, 0x00691dba);
AT(gSelPending, 0x00691dbc);
AT(gScreenReturnFn, 0x00691dc0);
AT(gSlotSel, 0x00691dce);
AT(gScreenReturnArg, 0x00691dd0);
AT(gSoundLevel0, 0x00691dd4);
AT(gOptMainReady, 0x00691df8);
AT(gSoundSlideA, 0x00691e00);
AT(gSoundSlideB, 0x00691e04);
AT(gKeyPlayer, 0x00691e08);
AT(gOptCount, 0x00691e10);
AT(gOptSaveDelay, 0x00691e14);
AT(gOptTicks, 0x00691e1c);
AT(gSoundSel, 0x00691e20);
AT(gOptSlide, 0x00691e40);
AT(gOptMainSel, 0x00691e60);
AT(gSoundLevel1, 0x00691e64);
AT(gOptRows, 0x00691e80);
AT(gOptSel, 0x00691ea0);
AT(gScrollTick, 0x00691ea8);
AT(gTitleIdle, 0x00691eb4);
AT(gTitleDelay, 0x00691eb8);
AT(gAutoSlot, 0x00691ebc);
AT(gTitleFrames, 0x00691ebe);
AT(gMcChoice, 0x00691ec0);
AT(gMcLoadOk, 0x00691ed4);
AT(gMcCountdown, 0x00691ed6);
AT(gMcMode, 0x00691ed8);
AT(gMcSaveOk, 0x00691eda);
AT(gMcRows, 0x00691ee0);
AT(gMcState, 0x00691f00);
AT(gMcSlot, 0x00691f02);
AT(gLanguageSel, 0x00691f04);
AT(gMenuSlide3, 0x00691f10);
AT(gMenuSlide2, 0x00691f20);
AT(gMenuSlide1, 0x00691f24);
AT(gMenuSlide0, 0x00691f28);
AT(gGuiPadRepeatHeld, 0x00691f2c);
AT(gGuiPadRepeatCount, 0x00691f3c);
AT(gPlayerRecs, 0x00691f40);
AT(gPropRecords, 0x00694460);
AT(gPropNodeStates, 0x00694ca4);
AT(gPhysDebugTexture, 0x006951e8);
AT(gGameJudgeScores, 0x00695200);
AT(gCareerUnlocked, 0x0069521e);
AT(gGameHorseTricks, 0x00695260);
AT(gGameHorseCount, 0x00695290);
AT(gProfiles, 0x006952a0);
AT(gOptionFlags, 0x006955e0);
AT(gPhysSyncFlags, 0x006955e2);
AT(gSoundVolumes, 0x006955e4);
AT(gRiderData, 0x006955e8);
AT(gActionLanguage, 0x00696218);
AT(gPlayerCount, 0x0069621c);
AT(gDemoPlayer, 0x006962f5);
AT(gPhysMotions, 0x006962fc);
AT(gActionMotions, 0x00696300);
AT(gOvlSaving, 0x006966dc);
AT(rpMeshOffset, 0x006966e8);
AT(rpMeshModuleCount, 0x006966ec);
AT(rpPVSSectorOffset, 0x006966fc);
AT(rpPVSWorldOffset, 0x0069670c);
AT(rwD3DOrClip, 0x00696710);
AT(rwD3DAndClip, 0x00696720);
AT(rwD3DState, 0x00696740);
AT(rwD3DCaps, 0x006968f0);
AT(rwD3DClipTriCull, 0x00696900);
AT(rwD3DTriCull, 0x00696904);
AT(rwD3DClipTri, 0x00696908);
AT(rwD3DClipLine, 0x0069690c);
AT(rwD3DIndexStart, 0x00696910);
AT(rwD3DIndexTri, 0x00696914);
AT(rwD3DIndexLine, 0x00696918);
AT(rwD3DIndexFlush, 0x0069691c);
AT(rwD3DSetProjection, 0x00696920);
AT(rwD3DSetUVStage, 0x00696924);
AT(rwD3DZScale, 0x00696928);
AT(rwD3DZShift, 0x0069692c);
AT(rwD3DXScale, 0x00696930);
AT(rwD3DYScale, 0x00696934);
AT(rwD3DXOffset, 0x00696938);
AT(rwD3DYOffset, 0x0069693c);
AT(rwD3DNearZ, 0x00696940);
AT(rwD3DFarZ, 0x00696944);
AT(rwD3DNearScreenZ, 0x00696948);
AT(rwD3DFarScreenZ, 0x0069694c);
AT(rwD3DRecipWindow, 0x00696950);
AT(rwD3DTLBase, 0x00696958);
AT(rwD3DCamBase, 0x0069695c);
AT(rwIm3DGouraud, 0x00696960);
AT(rwD3DIm3DDepth, 0x00696964);
AT(rwD3DIm3DLevel, 0x00696968);
AT(rwD3DVertexBits, 0x00696a5c);
AT(rwD3DVertexBitsSize, 0x00696a60);
AT(rwD3DTriIndices, 0x00696a64);
AT(rwD3DLineIndices, 0x00696a70);
AT(rwD3DTLBuffer, 0x00696a7c);
AT(rwD3DCamBuffer, 0x00696a80);
AT(rwD3DBufferSize, 0x00696a84);
AT(rwD3DIm3DWork, 0x00696a88);
AT(gDirectDraw, 0x0069730c);
AT(rwD3DContext, 0x00697340);
AT(rwD3DCurrentCamera, 0x00697348);
AT(rwD3DMemory, 0x0069734c);
AT(rwD3DFogStart, 0x00697354);
AT(rwD3DFogEnd, 0x00697358);
AT(rwD3DFogScale, 0x0069735c);
AT(rwD3DRasterExtOffset, 0x00697364);
AT(rwD3DCameraExtOffset, 0x00697368);
AT(rpLodEngineOffset, 0x00697370);
AT(rpLodAtomicOffset, 0x00697374);
AT(rpLodCamera, 0x00697378);
AT(rpLodFreeList, 0x0069737c);
AT(gMovie, 0x006973a0);
AT(gZrwRaster, 0x006976c0);
AT(gZrwZRaster, 0x006976c4);
AT(gZrwWorld, 0x006976c8);
AT(gZrwWorldCenter, 0x006976cc);
AT(gZrwWorldCenterScaled, 0x006976d8);
AT(gZrwPvsLoaded, 0x006976e4);
AT(gZrwPvsEnabled, 0x006976e8);
AT(gZrwLightParamA, 0x006976ec);
AT(gZrwLightParamB, 0x006976f0);
AT(gZrwPrimCamera, 0x006976f4);
AT(gZrwInUpdate, 0x006976f8);
AT(gZrwColBox, 0x006976fc);
AT(gZrwFrameLimit, 0x0069771c);
AT(gZrwScale, 0x00697720);
AT(gZrwScale_00697724, 0x00697724);
AT(gZrwFlags, 0x00697728);
AT(gZrwClearOn, 0x0069772c);
AT(gZrwClearColor, 0x00697730);
AT(gZrwClearColor2, 0x00697734);
AT(gZrwLastTime, 0x00697738);
AT(gZrwFpsTime, 0x0069773c);
AT(gZrwFps, 0x00697740);
AT(gZrwFrames, 0x00697744);
AT(gZrwScreenW, 0x00697770);
AT(gZrwScreenH, 0x00697774);
AT(gZrwRenderFlags, 0x00697778);
AT(gZrwRenderExtra, 0x0069777c);
AT(gZrwWindow, 0x00697780);
AT(gZglInstance, 0x00697784);
AT(gZrwFullscreen, 0x00697788);
AT(gWindow0, 0x0069778c);
AT(gWindow1, 0x00697790);
AT(gAppTitle, 0x00697794);
AT(gZrwWantW, 0x00697798);
AT(gZrwWantH, 0x0069779c);
AT(gZglWantW2, 0x006977a0);
AT(gZglWantH2, 0x006977a4);
AT(gZglWantDepth, 0x006977a8);
AT(gZrwQuit, 0x006977ac);
AT(gZrwKeysLocked, 0x006977b0);
AT(gGdiSurface, 0x006977cc);
#undef AT
_Static_assert(offsetof(struct BmxBss, d_00698f84) + sizeof(((struct BmxBss *)0)->d_00698f84) == 0x12ef90,
               "bmxBss");

/* The original's .idata up to the end of its import slots, 0x00699000..0x0069987c. */
struct BmxImports {
    unsigned char pad_00699000[1176];         /* 0x00699000 */
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

_Static_assert(offsetof(struct BmxImports, end_00699878) + sizeof(((struct BmxImports *)0)->end_00699878) ==
                   0x87c,
               "bmxImports");

/* The original's .data1, 0x0069b000..0x0069f086. */
struct BmxData1 {
    float d_0069b000[44];                               /* 0x0069b000 */
    char sNegativeRadiusSpecified[28];                  /* 0x0069b0b0 */
    unsigned char pad_0069b0cc[4];                      /* 0x0069b0cc */
    char sInvalidDataBlockSizeSpecified[36];            /* 0x0069b0d0 */
    unsigned char pad_0069b0f4[12];                     /* 0x0069b0f4 */
    unsigned char d_0069b100[4];                        /* 0x0069b100 */
    float d_0069b104;                                   /* 0x0069b104 */
    float d_0069b108;                                   /* 0x0069b108 */
    float d_0069b10c;                                   /* 0x0069b10c */
    unsigned char d_0069b110[4];                        /* 0x0069b110 */
    float d_0069b114[7];                                /* 0x0069b114 */
    unsigned char d_0069b130[16];                       /* 0x0069b130 */
    unsigned char d_0069b140[4];                        /* 0x0069b140 */
    float d_0069b144;                                   /* 0x0069b144 */
    float d_0069b148;                                   /* 0x0069b148 */
    float d_0069b14c;                                   /* 0x0069b14c */
    unsigned char d_0069b150[4];                        /* 0x0069b150 */
    float d_0069b154;                                   /* 0x0069b154 */
    unsigned char d_0069b158[4];                        /* 0x0069b158 */
    float d_0069b15c;                                   /* 0x0069b15c */
    float d_0069b160;                                   /* 0x0069b160 */
    float d_0069b164;                                   /* 0x0069b164 */
    unsigned char d_0069b168[4];                        /* 0x0069b168 */
    float d_0069b16c[16];                               /* 0x0069b16c */
    float d_0069b1ac;                                   /* 0x0069b1ac */
    float d_0069b1b0[4];                                /* 0x0069b1b0 */
    char sNegativeRadius[16];                           /* 0x0069b1c0 */
    char sInvalidVertexIndex[24];                       /* 0x0069b1d0 */
    unsigned char pad_0069b1e8[8];                      /* 0x0069b1e8 */
    char sNegativeParameterPassed[28];                  /* 0x0069b1f0 */
    float d_0069b20c[9];                                /* 0x0069b20c */
    float d_0069b230;                                   /* 0x0069b230 */
    float d_0069b234;                                   /* 0x0069b234 */
    float d_0069b238;                                   /* 0x0069b238 */
    float d_0069b23c;                                   /* 0x0069b23c */
    float d_0069b240;                                   /* 0x0069b240 */
    float d_0069b244;                                   /* 0x0069b244 */
    float d_0069b248;                                   /* 0x0069b248 */
    float d_0069b24c;                                   /* 0x0069b24c */
    float d_0069b250;                                   /* 0x0069b250 */
    float d_0069b254;                                   /* 0x0069b254 */
    float d_0069b258;                                   /* 0x0069b258 */
    float d_0069b25c;                                   /* 0x0069b25c */
    float d_0069b260;                                   /* 0x0069b260 */
    float d_0069b264;                                   /* 0x0069b264 */
    float d_0069b268;                                   /* 0x0069b268 */
    float d_0069b26c;                                   /* 0x0069b26c */
    float d_0069b270;                                   /* 0x0069b270 */
    float d_0069b274;                                   /* 0x0069b274 */
    float d_0069b278;                                   /* 0x0069b278 */
    float d_0069b27c[2];                                /* 0x0069b27c */
    float d_0069b284[2];                                /* 0x0069b284 */
    float d_0069b28c[2];                                /* 0x0069b28c */
    float d_0069b294[2];                                /* 0x0069b294 */
    float d_0069b29c[2];                                /* 0x0069b29c */
    float d_0069b2a4[2];                                /* 0x0069b2a4 */
    float d_0069b2ac;                                   /* 0x0069b2ac */
    float d_0069b2b0;                                   /* 0x0069b2b0 */
    float d_0069b2b4;                                   /* 0x0069b2b4 */
    float d_0069b2b8;                                   /* 0x0069b2b8 */
    float d_0069b2bc;                                   /* 0x0069b2bc */
    float d_0069b2c0;                                   /* 0x0069b2c0 */
    float d_0069b2c4;                                   /* 0x0069b2c4 */
    float d_0069b2c8;                                   /* 0x0069b2c8 */
    float d_0069b2cc;                                   /* 0x0069b2cc */
    float d_0069b2d0;                                   /* 0x0069b2d0 */
    float d_0069b2d4;                                   /* 0x0069b2d4 */
    float d_0069b2d8;                                   /* 0x0069b2d8 */
    float d_0069b2dc;                                   /* 0x0069b2dc */
    float d_0069b2e0;                                   /* 0x0069b2e0 */
    float d_0069b2e4;                                   /* 0x0069b2e4 */
    float d_0069b2e8;                                   /* 0x0069b2e8 */
    float d_0069b2ec;                                   /* 0x0069b2ec */
    float d_0069b2f0;                                   /* 0x0069b2f0 */
    float d_0069b2f4[13];                               /* 0x0069b2f4 */
    float rpSphereMinRadius;                            /* 0x0069b328 */
    unsigned char pad_0069b32c[4];                      /* 0x0069b32c */
    float rpSphereMinRadiusSq;                          /* 0x0069b330 */
    float rpSphereMinDistance;                          /* 0x0069b334 */
    unsigned char d_0069b338[4];                        /* 0x0069b338 */
    float d_0069b33c;                                   /* 0x0069b33c */
    double d_0069b340[2];                               /* 0x0069b340 */
    unsigned char d_0069b350[4];                        /* 0x0069b350 */
    float d_0069b354;                                   /* 0x0069b354 */
    unsigned char d_0069b358[4];                        /* 0x0069b358 */
    unsigned char d_0069b35c[4];                        /* 0x0069b35c */
    float d_0069b360[3];                                /* 0x0069b360 */
    float d_0069b36c;                                   /* 0x0069b36c */
    float d_0069b370[5];                                /* 0x0069b370 */
    float d_0069b384[2];                                /* 0x0069b384 */
    float d_0069b38c[2];                                /* 0x0069b38c */
    float d_0069b394;                                   /* 0x0069b394 */
    float d_0069b398;                                   /* 0x0069b398 */
    float d_0069b39c;                                   /* 0x0069b39c */
    float d_0069b3a0[15];                               /* 0x0069b3a0 */
    unsigned char d_0069b3dc[4];                        /* 0x0069b3dc */
    float d_0069b3e0[2];                                /* 0x0069b3e0 */
    unsigned char d_0069b3e8[4];                        /* 0x0069b3e8 */
    float d_0069b3ec[2];                                /* 0x0069b3ec */
    unsigned char d_0069b3f4[4];                        /* 0x0069b3f4 */
    float d_0069b3f8[2];                                /* 0x0069b3f8 */
    unsigned char d_0069b400[4];                        /* 0x0069b400 */
    float d_0069b404[2];                                /* 0x0069b404 */
    unsigned char d_0069b40c[40];                       /* 0x0069b40c */
    unsigned char d_0069b434[4];                        /* 0x0069b434 */
    float d_0069b438;                                   /* 0x0069b438 */
    unsigned char d_0069b43c[4];                        /* 0x0069b43c */
    unsigned char d_0069b440[4];                        /* 0x0069b440 */
    float d_0069b444[2];                                /* 0x0069b444 */
    float d_0069b44c[13];                               /* 0x0069b44c */
    char sNegativeSphereRadius[24];                     /* 0x0069b480 */
    unsigned char pad_0069b498[8];                      /* 0x0069b498 */
    float d_0069b4a0;                                   /* 0x0069b4a0 */
    float d_0069b4a4;                                   /* 0x0069b4a4 */
    float d_0069b4a8[14];                               /* 0x0069b4a8 */
    char s_0069b4e0[16];                                /* 0x0069b4e0 */
    float rpWorldScale0;                                /* 0x0069b4f0 */
    float rpWorldScale1;                                /* 0x0069b4f4 */
    float rpWorldScale2;                                /* 0x0069b4f8 */
    float rpWorldReadScale0;                            /* 0x0069b4fc */
    float rpWorldReadScale1;                            /* 0x0069b500 */
    float rpWorldReadScale2;                            /* 0x0069b504 */
    float d_0069b508[6];                                /* 0x0069b508 */
    void *p_0069b520;                                   /* 0x0069b520 */
    void *p_0069b524;                                   /* 0x0069b524 */
    void *p_0069b528;                                   /* 0x0069b528 */
    void *p_0069b52c;                                   /* 0x0069b52c */
    float rwD3DK0x0069b530;                             /* 0x0069b530 */
    float rwD3DK0x0069b534;                             /* 0x0069b534 */
    float rwD3DK0x0069b538;                             /* 0x0069b538 */
    float d_0069b53c[2];                                /* 0x0069b53c */
    float rwD3DK0x0069b544;                             /* 0x0069b544 */
    float rwD3DK0x0069b548;                             /* 0x0069b548 */
    float rwD3DK0x0069b54c;                             /* 0x0069b54c */
    float rwD3DK0x0069b550;                             /* 0x0069b550 */
    float rwD3DK0x0069b554;                             /* 0x0069b554 */
    float rwD3DK0x0069b558;                             /* 0x0069b558 */
    float rwD3DK0x0069b55c;                             /* 0x0069b55c */
    float rwD3DK0x0069b560;                             /* 0x0069b560 */
    float rwD3DK0x0069b564;                             /* 0x0069b564 */
    float rwD3DK0x0069b568;                             /* 0x0069b568 */
    float rwD3DK0x0069b56c;                             /* 0x0069b56c */
    float rwD3DK0x0069b570;                             /* 0x0069b570 */
    float rwD3DK0x0069b574;                             /* 0x0069b574 */
    float d_0069b578;                                   /* 0x0069b578 */
    float rwD3DK0x0069b57c;                             /* 0x0069b57c */
    float rwD3DK0x0069b580;                             /* 0x0069b580 */
    float rwD3DK0x0069b584;                             /* 0x0069b584 */
    float rwD3DK0x0069b588;                             /* 0x0069b588 */
    float rwD3DK0x0069b58c;                             /* 0x0069b58c */
    float rwD3DK0x0069b590;                             /* 0x0069b590 */
    float d_0069b594;                                   /* 0x0069b594 */
    float rwD3DK0x0069b598;                             /* 0x0069b598 */
    float d_0069b59c;                                   /* 0x0069b59c */
    float rwD3DK0x0069b5a0;                             /* 0x0069b5a0 */
    float rwD3DK0x0069b5a4;                             /* 0x0069b5a4 */
    float rwD3DK0x0069b5a8;                             /* 0x0069b5a8 */
    float rwD3DK0x0069b5ac;                             /* 0x0069b5ac */
    float rwD3DK0x0069b5b0;                             /* 0x0069b5b0 */
    float rwD3DK0x0069b5b4;                             /* 0x0069b5b4 */
    float d_0069b5b8;                                   /* 0x0069b5b8 */
    float rwD3DK0x0069b5bc;                             /* 0x0069b5bc */
    float d_0069b5c0;                                   /* 0x0069b5c0 */
    float rwD3DK0x0069b5c4;                             /* 0x0069b5c4 */
    float rwD3DK0x0069b5c8;                             /* 0x0069b5c8 */
    float rwD3DK0x0069b5cc;                             /* 0x0069b5cc */
    float d_0069b5d0;                                   /* 0x0069b5d0 */
    float rwD3DK0x0069b5d4;                             /* 0x0069b5d4 */
    float rwD3DK0x0069b5d8;                             /* 0x0069b5d8 */
    float rwD3DK0x0069b5dc;                             /* 0x0069b5dc */
    float rwD3DK0x0069b5e0;                             /* 0x0069b5e0 */
    float d_0069b5e4;                                   /* 0x0069b5e4 */
    float rwD3DK0x0069b5e8;                             /* 0x0069b5e8 */
    float d_0069b5ec;                                   /* 0x0069b5ec */
    float rwD3DK0x0069b5f0;                             /* 0x0069b5f0 */
    unsigned char pad_0069b5f4[12];                     /* 0x0069b5f4 */
    uint32_t d_0069b600;                                /* 0x0069b600 */
    float rwD3DK0x0069b604;                             /* 0x0069b604 */
    float rwD3DK0x0069b608;                             /* 0x0069b608 */
    float rwD3DK0x0069b60c;                             /* 0x0069b60c */
    float d_0069b610[2];                                /* 0x0069b610 */
    float d_0069b618[2];                                /* 0x0069b618 */
    float d_0069b620[2];                                /* 0x0069b620 */
    float d_0069b628[3];                                /* 0x0069b628 */
    float d_0069b634[3];                                /* 0x0069b634 */
    double d_0069b640;                                  /* 0x0069b640 */
    double d_0069b648;                                  /* 0x0069b648 */
    double d_0069b650;                                  /* 0x0069b650 */
    float d_0069b658[3];                                /* 0x0069b658 */
    float d_0069b664[3];                                /* 0x0069b664 */
    char s03d[8];                                       /* 0x0069b670 */
    unsigned char pad_0069b678[8];                      /* 0x0069b678 */
    char s02x[8];                                       /* 0x0069b680 */
    unsigned char pad_0069b688[8];                      /* 0x0069b688 */
    char s_0069b690[4];                                 /* 0x0069b690 */
    unsigned char pad_0069b694[12];                     /* 0x0069b694 */
    char sW[4];                                         /* 0x0069b6a0 */
    unsigned char pad_0069b6a4[12];                     /* 0x0069b6a4 */
    float d_0069b6b0;                                   /* 0x0069b6b0 */
    float d_0069b6b4[2];                                /* 0x0069b6b4 */
    float d_0069b6bc;                                   /* 0x0069b6bc */
    float d_0069b6c0;                                   /* 0x0069b6c0 */
    float d_0069b6c4;                                   /* 0x0069b6c4 */
    float d_0069b6c8;                                   /* 0x0069b6c8 */
    float d_0069b6cc;                                   /* 0x0069b6cc */
    float d_0069b6d0[2];                                /* 0x0069b6d0 */
    uint32_t d_0069b6d8;                                /* 0x0069b6d8 */
    uint32_t d_0069b6dc;                                /* 0x0069b6dc */
    uint32_t d_0069b6e0[2];                             /* 0x0069b6e0 */
    float d_0069b6e8;                                   /* 0x0069b6e8 */
    float d_0069b6ec[2];                                /* 0x0069b6ec */
    float d_0069b6f4;                                   /* 0x0069b6f4 */
    float d_0069b6f8;                                   /* 0x0069b6f8 */
    float d_0069b6fc[5];                                /* 0x0069b6fc */
    float d_0069b710[2];                                /* 0x0069b710 */
    float d_0069b718[10];                               /* 0x0069b718 */
    char sOnlyRenderingSubSystem[28];                   /* 0x0069b740 */
    unsigned char pad_0069b75c[4];                      /* 0x0069b75c */
    unsigned char d_0069b760[4];                        /* 0x0069b760 */
    unsigned char d_0069b764[4];                        /* 0x0069b764 */
    unsigned char d_0069b768[4];                        /* 0x0069b768 */
    unsigned char d_0069b76c[4];                        /* 0x0069b76c */
    unsigned char d_0069b770[4];                        /* 0x0069b770 */
    unsigned char d_0069b774[4];                        /* 0x0069b774 */
    float d_0069b778;                                   /* 0x0069b778 */
    float d_0069b77c;                                   /* 0x0069b77c */
    float d_0069b780;                                   /* 0x0069b780 */
    float d_0069b784;                                   /* 0x0069b784 */
    float d_0069b788;                                   /* 0x0069b788 */
    float d_0069b78c[4];                                /* 0x0069b78c */
    uint32_t d_0069b79c;                                /* 0x0069b79c */
    float d_0069b7a0;                                   /* 0x0069b7a0 */
    float d_0069b7a4[2];                                /* 0x0069b7a4 */
    float d_0069b7ac[25];                               /* 0x0069b7ac */
    float d_0069b810;                                   /* 0x0069b810 */
    float d_0069b814[2];                                /* 0x0069b814 */
    float d_0069b81c;                                   /* 0x0069b81c */
    uint32_t d_0069b820;                                /* 0x0069b820 */
    float d_0069b824;                                   /* 0x0069b824 */
    unsigned char d_0069b828[4];                        /* 0x0069b828 */
    float d_0069b82c[13];                               /* 0x0069b82c */
    float d_0069b860;                                   /* 0x0069b860 */
    float d_0069b864;                                   /* 0x0069b864 */
    float d_0069b868[16];                               /* 0x0069b868 */
    float d_0069b8a8;                                   /* 0x0069b8a8 */
    float d_0069b8ac;                                   /* 0x0069b8ac */
    float d_0069b8b0;                                   /* 0x0069b8b0 */
    float rwMatrixNormalLimit0;                         /* 0x0069b8b4 */
    float rwMatrixNormalLimit1;                         /* 0x0069b8b8 */
    float rwMatrixNormalLimit2;                         /* 0x0069b8bc */
    char sInvalidCombinationType[28];                   /* 0x0069b8c0 */
    float d_0069b8dc[25];                               /* 0x0069b8dc */
    unsigned char d_0069b940[4];                        /* 0x0069b940 */
    float d_0069b944[3];                                /* 0x0069b944 */
    double rwInvSqrtScaleOdd;                           /* 0x0069b950 */
    double rwInvSqrtScaleEven;                          /* 0x0069b958 */
    float d_0069b960[2];                                /* 0x0069b960 */
    float d_0069b968[3];                                /* 0x0069b968 */
    float d_0069b974;                                   /* 0x0069b974 */
    double d_0069b978;                                  /* 0x0069b978 */
    uint32_t d_0069b980[3];                             /* 0x0069b980 */
    float rwD3DFogRange;                                /* 0x0069b98c */
    unsigned char pad_0069b990[4];                      /* 0x0069b990 */
    float d_0069b994;                                   /* 0x0069b994 */
    float d_0069b998;                                   /* 0x0069b998 */
    float d_0069b99c[18];                               /* 0x0069b99c */
    float d_0069b9e4;                                   /* 0x0069b9e4 */
    double d_0069b9e8;                                  /* 0x0069b9e8 */
    unsigned char d_0069b9f0[4];                        /* 0x0069b9f0 */
    uint32_t d_0069b9f4;                                /* 0x0069b9f4 */
    void *p_0069b9f8;                                   /* 0x0069b9f8 */
    void *p_0069b9fc;                                   /* 0x0069b9fc */
    void *p_0069ba00;                                   /* 0x0069ba00 */
    void *p_0069ba04;                                   /* 0x0069ba04 */
    void *p_0069ba08;                                   /* 0x0069ba08 */
    void *p_0069ba0c;                                   /* 0x0069ba0c */
    void *p_0069ba10;                                   /* 0x0069ba10 */
    void *p_0069ba14;                                   /* 0x0069ba14 */
    void *p_0069ba18;                                   /* 0x0069ba18 */
    void *p_0069ba1c;                                   /* 0x0069ba1c */
    void *p_0069ba20;                                   /* 0x0069ba20 */
    void *p_0069ba24;                                   /* 0x0069ba24 */
    void *p_0069ba28;                                   /* 0x0069ba28 */
    void *p_0069ba2c;                                   /* 0x0069ba2c */
    void *p_0069ba30;                                   /* 0x0069ba30 */
    void *p_0069ba34;                                   /* 0x0069ba34 */
    void *p_0069ba38;                                   /* 0x0069ba38 */
    void *p_0069ba3c;                                   /* 0x0069ba3c */
    void *p_0069ba40;                                   /* 0x0069ba40 */
    void *p_0069ba44;                                   /* 0x0069ba44 */
    void *p_0069ba48;                                   /* 0x0069ba48 */
    void *p_0069ba4c;                                   /* 0x0069ba4c */
    void *p_0069ba50;                                   /* 0x0069ba50 */
    void *p_0069ba54;                                   /* 0x0069ba54 */
    void *p_0069ba58;                                   /* 0x0069ba58 */
    void *p_0069ba5c;                                   /* 0x0069ba5c */
    void *p_0069ba60;                                   /* 0x0069ba60 */
    void *p_0069ba64;                                   /* 0x0069ba64 */
    void *p_0069ba68;                                   /* 0x0069ba68 */
    void *p_0069ba6c;                                   /* 0x0069ba6c */
    void *p_0069ba70;                                   /* 0x0069ba70 */
    void *p_0069ba74;                                   /* 0x0069ba74 */
    void *p_0069ba78;                                   /* 0x0069ba78 */
    void *p_0069ba7c;                                   /* 0x0069ba7c */
    void *p_0069ba80;                                   /* 0x0069ba80 */
    void *p_0069ba84;                                   /* 0x0069ba84 */
    void *p_0069ba88;                                   /* 0x0069ba88 */
    void *p_0069ba8c;                                   /* 0x0069ba8c */
    void *p_0069ba90;                                   /* 0x0069ba90 */
    void *p_0069ba94;                                   /* 0x0069ba94 */
    void *p_0069ba98;                                   /* 0x0069ba98 */
    void *p_0069ba9c;                                   /* 0x0069ba9c */
    void *p_0069baa0;                                   /* 0x0069baa0 */
    void *p_0069baa4;                                   /* 0x0069baa4 */
    void *p_0069baa8;                                   /* 0x0069baa8 */
    void *p_0069baac;                                   /* 0x0069baac */
    void *p_0069bab0;                                   /* 0x0069bab0 */
    void *p_0069bab4;                                   /* 0x0069bab4 */
    void *p_0069bab8;                                   /* 0x0069bab8 */
    void *p_0069babc;                                   /* 0x0069babc */
    void *p_0069bac0;                                   /* 0x0069bac0 */
    void *p_0069bac4;                                   /* 0x0069bac4 */
    void *p_0069bac8;                                   /* 0x0069bac8 */
    void *p_0069bacc;                                   /* 0x0069bacc */
    void *p_0069bad0;                                   /* 0x0069bad0 */
    void *p_0069bad4;                                   /* 0x0069bad4 */
    void *p_0069bad8;                                   /* 0x0069bad8 */
    void *p_0069badc;                                   /* 0x0069badc */
    void *p_0069bae0;                                   /* 0x0069bae0 */
    void *p_0069bae4;                                   /* 0x0069bae4 */
    void *p_0069bae8;                                   /* 0x0069bae8 */
    void *p_0069baec;                                   /* 0x0069baec */
    void *p_0069baf0;                                   /* 0x0069baf0 */
    void *p_0069baf4;                                   /* 0x0069baf4 */
    void *p_0069baf8;                                   /* 0x0069baf8 */
    void *p_0069bafc;                                   /* 0x0069bafc */
    void *p_0069bb00;                                   /* 0x0069bb00 */
    void *p_0069bb04;                                   /* 0x0069bb04 */
    void *p_0069bb08;                                   /* 0x0069bb08 */
    void *p_0069bb0c;                                   /* 0x0069bb0c */
    void *p_0069bb10;                                   /* 0x0069bb10 */
    void *p_0069bb14;                                   /* 0x0069bb14 */
    void *p_0069bb18;                                   /* 0x0069bb18 */
    void *p_0069bb1c;                                   /* 0x0069bb1c */
    void *p_0069bb20;                                   /* 0x0069bb20 */
    void *p_0069bb24;                                   /* 0x0069bb24 */
    void *p_0069bb28;                                   /* 0x0069bb28 */
    void *p_0069bb2c;                                   /* 0x0069bb2c */
    float d_0069bb30[24];                               /* 0x0069bb30 */
    char sFailedToLockD3DSurface[28];                   /* 0x0069bb90 */
    unsigned char pad_0069bbac[4];                      /* 0x0069bbac */
    char sFailedToUnlockD3DSurface[32];                 /* 0x0069bbb0 */
    char sFailedToCreateDDSurfaceForTexture[40];        /* 0x0069bbd0 */
    unsigned char pad_0069bbf8[8];                      /* 0x0069bbf8 */
    char sDDS[8];                                       /* 0x0069bc00 */
    unsigned char pad_0069bc08[8];                      /* 0x0069bc08 */
    char sDds[8];                                       /* 0x0069bc10 */
    unsigned char pad_0069bc18[8];                      /* 0x0069bc18 */
    char sD3D[4];                                       /* 0x0069bc20 */
    unsigned char pad_0069bc24[12];                     /* 0x0069bc24 */
    char sRefRast[8];                                   /* 0x0069bc30 */
    unsigned char pad_0069bc38[8];                      /* 0x0069bc38 */
    char sNegativeResourceArenaSizeRequested[40];       /* 0x0069bc40 */
    unsigned char pad_0069bc68[8];                      /* 0x0069bc68 */
    float rwPerspCornerSteps[6];                        /* 0x0069bc70; the type is local to rw/camera.c */
    float rwViewFactors[10];                            /* 0x0069bc88; the type is local to rw/camera.c */
    float d_0069bcb0;                                   /* 0x0069bcb0 */
    float d_0069bcb4[3];                                /* 0x0069bcb4 */
    float d_0069bcc0[2];                                /* 0x0069bcc0 */
    float d_0069bcc8[2];                                /* 0x0069bcc8 */
    float d_0069bcd0[2];                                /* 0x0069bcd0 */
    float d_0069bcd8[22];                               /* 0x0069bcd8 */
    char sNearClipPlaneIsBeyondFarClipPlane[44];        /* 0x0069bd30 */
    unsigned char pad_0069bd5c[4];                      /* 0x0069bd5c */
    char sInvalidProjectionTypeSpecified[36];           /* 0x0069bd60 */
    unsigned char pad_0069bd84[12];                     /* 0x0069bd84 */
    char sNull[8];                                      /* 0x0069bd90 */
    unsigned char pad_0069bd98[8];                      /* 0x0069bd98 */
    char rwMipSuffix;                                   /* 0x0069bda0 */
    unsigned char pad_0069bda1[15];                     /* 0x0069bda1 */
    char sRb[4];                                        /* 0x0069bdb0 */
    unsigned char pad_0069bdb4[12];                     /* 0x0069bdb4 */
    char sWb[4];                                        /* 0x0069bdc0 */
    unsigned char pad_0069bdc4[12];                     /* 0x0069bdc4 */
    char sAb[4];                                        /* 0x0069bdd0 */
    unsigned char pad_0069bdd4[12];                     /* 0x0069bdd4 */
    char sInvalidLengthPassed[24];                      /* 0x0069bde0 */
    unsigned char pad_0069bdf8[4];                      /* 0x0069bdf8 */
    unsigned char d_0069bdfc[4];                        /* 0x0069bdfc */
    void *p_0069be00;                                   /* 0x0069be00 */
    void *p_0069be04;                                   /* 0x0069be04 */
    void *p_0069be08;                                   /* 0x0069be08 */
    void *p_0069be0c;                                   /* 0x0069be0c */
    void *p_0069be10;                                   /* 0x0069be10 */
    void *p_0069be14;                                   /* 0x0069be14 */
    void *p_0069be18;                                   /* 0x0069be18 */
    void *p_0069be1c;                                   /* 0x0069be1c */
    void *p_0069be20;                                   /* 0x0069be20 */
    void *p_0069be24;                                   /* 0x0069be24 */
    void *p_0069be28;                                   /* 0x0069be28 */
    void *p_0069be2c;                                   /* 0x0069be2c */
    void *p_0069be30;                                   /* 0x0069be30 */
    void *p_0069be34;                                   /* 0x0069be34 */
    void *p_0069be38;                                   /* 0x0069be38 */
    void *p_0069be3c;                                   /* 0x0069be3c */
    void *p_0069be40;                                   /* 0x0069be40 */
    void *p_0069be44;                                   /* 0x0069be44 */
    void *p_0069be48;                                   /* 0x0069be48 */
    void *p_0069be4c;                                   /* 0x0069be4c */
    float d_0069be50;                                   /* 0x0069be50 */
    float d_0069be54[3];                                /* 0x0069be54 */
    double d_0069be60;                                  /* 0x0069be60 */
    double d_0069be68;                                  /* 0x0069be68 */
    double d_0069be70;                                  /* 0x0069be70 */
    float d_0069be78[3];                                /* 0x0069be78 */
    float d_0069be84;                                   /* 0x0069be84 */
    float d_0069be88[2];                                /* 0x0069be88 */
    double d_0069be90;                                  /* 0x0069be90 */
    double d_0069be98;                                  /* 0x0069be98 */
    double d_0069bea0;                                  /* 0x0069bea0 */
    double d_0069bea8;                                  /* 0x0069bea8 */
    char sIndexOutOfRange[20];                          /* 0x0069beb0 */
    unsigned char pad_0069bec4[12];                     /* 0x0069bec4 */
    float d_0069bed0;                                   /* 0x0069bed0 */
    float d_0069bed4;                                   /* 0x0069bed4 */
    float d_0069bed8[2];                                /* 0x0069bed8 */
    void *p_0069bee0;                                   /* 0x0069bee0 */
    void *p_0069bee4;                                   /* 0x0069bee4 */
    void *p_0069bee8;                                   /* 0x0069bee8 */
    void *p_0069beec;                                   /* 0x0069beec */
    float d_0069bef0[2];                                /* 0x0069bef0 */
    float d_0069bef8[15];                               /* 0x0069bef8 */
    float d_0069bf34[3];                                /* 0x0069bf34 */
    void *p_0069bf40;                                   /* 0x0069bf40 */
    void *p_0069bf44;                                   /* 0x0069bf44 */
    void *p_0069bf48;                                   /* 0x0069bf48 */
    void *p_0069bf4c;                                   /* 0x0069bf4c */
    void *p_0069bf50;                                   /* 0x0069bf50 */
    void *p_0069bf54;                                   /* 0x0069bf54 */
    void *p_0069bf58;                                   /* 0x0069bf58 */
    void *p_0069bf5c;                                   /* 0x0069bf5c */
    void *p_0069bf60;                                   /* 0x0069bf60 */
    void *p_0069bf64;                                   /* 0x0069bf64 */
    void *p_0069bf68;                                   /* 0x0069bf68 */
    void *p_0069bf6c;                                   /* 0x0069bf6c */
    void *p_0069bf70;                                   /* 0x0069bf70 */
    void *p_0069bf74;                                   /* 0x0069bf74 */
    void *p_0069bf78;                                   /* 0x0069bf78 */
    void *p_0069bf7c;                                   /* 0x0069bf7c */
    void *p_0069bf80;                                   /* 0x0069bf80 */
    void *p_0069bf84;                                   /* 0x0069bf84 */
    void *p_0069bf88;                                   /* 0x0069bf88 */
    void *p_0069bf8c;                                   /* 0x0069bf8c */
    char sReferenceRasterizer[24];                      /* 0x0069bf90 */
    unsigned char pad_0069bfa8[8];                      /* 0x0069bfa8 */
    char sSSSW[12];                                     /* 0x0069bfb0 */
    unsigned char pad_0069bfbc[4];                      /* 0x0069bfbc */
    char sSSW[8];                                       /* 0x0069bfc0 */
    unsigned char pad_0069bfc8[8];                      /* 0x0069bfc8 */
    char sSSHW[12];                                     /* 0x0069bfd0 */
    unsigned char pad_0069bfdc[4];                      /* 0x0069bfdc */
    char sSHW[8];                                       /* 0x0069bfe0 */
    unsigned char pad_0069bfe8[8];                      /* 0x0069bfe8 */
    char sUnableToCreateD3DTextureFormatFreeList[48];   /* 0x0069bff0 */
    char sUnableToEnumerateD3DTextureFormats[40];       /* 0x0069c020 */
    unsigned char pad_0069c048[8];                      /* 0x0069c048 */
    char sUnableToFindAnySuitableTextureFormats[44];    /* 0x0069c050 */
    unsigned char pad_0069c07c[4];                      /* 0x0069c07c */
    char sInvalidSubsystemNumber[28];                   /* 0x0069c080 */
    unsigned char pad_0069c09c[4];                      /* 0x0069c09c */
    char s_0069c0a0[28];                                /* 0x0069c0a0 */
    unsigned char pad_0069c0bc[4];                      /* 0x0069c0bc */
    char sFailedToCreateDirectDraw[32];                 /* 0x0069c0c0 */
    char sFailedToCreateDirectDraw4[32];                /* 0x0069c0e0 */
    char sFailedToCreateD3D3[24];                       /* 0x0069c100 */
    unsigned char pad_0069c118[8];                      /* 0x0069c118 */
    char sFailedToSetDirectDrawCooperativeLevel[44];    /* 0x0069c120 */
    unsigned char pad_0069c14c[4];                      /* 0x0069c14c */
    char sFailedToSetDisplayMode[28];                   /* 0x0069c150 */
    unsigned char pad_0069c16c[4];                      /* 0x0069c16c */
    char sFailedToCreatePrimarySurface[36];             /* 0x0069c170 */
    unsigned char pad_0069c194[12];                     /* 0x0069c194 */
    char sFailedToCreateZBufferSurface[36];             /* 0x0069c1a0 */
    unsigned char pad_0069c1c4[12];                     /* 0x0069c1c4 */
    char sFailedToAttachZBufferSurface[36];             /* 0x0069c1d0 */
    unsigned char pad_0069c1f4[12];                     /* 0x0069c1f4 */
    char sFailedToCreateD3D3Device[32];                 /* 0x0069c200 */
    char sFailedToCreateViewport[28];                   /* 0x0069c220 */
    unsigned char pad_0069c23c[4];                      /* 0x0069c23c */
    char sFailedToAddViewport[24];                      /* 0x0069c240 */
    unsigned char pad_0069c258[8];                      /* 0x0069c258 */
    char sFailedToSetTheViewport[28];                   /* 0x0069c260 */
    unsigned char pad_0069c27c[4];                      /* 0x0069c27c */
    char sFailedToCreateBackgroundMaterial[40];         /* 0x0069c280 */
    unsigned char pad_0069c2a8[8];                      /* 0x0069c2a8 */
    char sFailedToSetBackground[28];                    /* 0x0069c2b0 */
    unsigned char pad_0069c2cc[4];                      /* 0x0069c2cc */
    char sFailedToSetCooperativeLevel[32];              /* 0x0069c2d0 */
    char sFailedToCreateDirectDrawClipper[40];          /* 0x0069c2f0 */
    unsigned char pad_0069c318[8];                      /* 0x0069c318 */
    char sFailedToAllocateTexChainElement[40];          /* 0x0069c320 */
    unsigned char pad_0069c348[8];                      /* 0x0069c348 */
    float d_0069c350;                                   /* 0x0069c350 */
    float d_0069c354;                                   /* 0x0069c354 */
    float d_0069c358;                                   /* 0x0069c358 */
    float d_0069c35c;                                   /* 0x0069c35c */
    float d_0069c360;                                   /* 0x0069c360 */
    float d_0069c364;                                   /* 0x0069c364 */
    float d_0069c368;                                   /* 0x0069c368 */
    float d_0069c36c;                                   /* 0x0069c36c */
    float d_0069c370;                                   /* 0x0069c370 */
    float d_0069c374[2];                                /* 0x0069c374 */
    unsigned char d_0069c37c[4];                        /* 0x0069c37c */
    unsigned char d_0069c380[4];                        /* 0x0069c380 */
    unsigned char d_0069c384[4];                        /* 0x0069c384 */
    float d_0069c388[3];                                /* 0x0069c388 */
    unsigned char d_0069c394[4];                        /* 0x0069c394 */
    float d_0069c398[3];                                /* 0x0069c398 */
    unsigned char d_0069c3a4[4];                        /* 0x0069c3a4 */
    float d_0069c3a8[3];                                /* 0x0069c3a8 */
    unsigned char d_0069c3b4[4];                        /* 0x0069c3b4 */
    float d_0069c3b8[3];                                /* 0x0069c3b8 */
    unsigned char d_0069c3c4[4];                        /* 0x0069c3c4 */
    double d_0069c3c8;                                  /* 0x0069c3c8 */
    float d_0069c3d0;                                   /* 0x0069c3d0 */
    unsigned char d_0069c3d4[4];                        /* 0x0069c3d4 */
    unsigned char d_0069c3d8[4];                        /* 0x0069c3d8 */
    unsigned char d_0069c3dc[4];                        /* 0x0069c3dc */
    unsigned char d_0069c3e0[4];                        /* 0x0069c3e0 */
    float d_0069c3e4[11];                               /* 0x0069c3e4 */
    uint32_t d_0069c410;                                /* 0x0069c410 */
    float d_0069c414;                                   /* 0x0069c414 */
    float d_0069c418;                                   /* 0x0069c418 */
    float d_0069c41c;                                   /* 0x0069c41c */
    float d_0069c420;                                   /* 0x0069c420 */
    uint32_t d_0069c424;                                /* 0x0069c424 */
    uint32_t d_0069c428;                                /* 0x0069c428 */
    uint32_t d_0069c42c;                                /* 0x0069c42c */
    uint32_t d_0069c430;                                /* 0x0069c430 */
    float d_0069c434;                                   /* 0x0069c434 */
    float d_0069c438;                                   /* 0x0069c438 */
    float d_0069c43c;                                   /* 0x0069c43c */
    float d_0069c440;                                   /* 0x0069c440 */
    float d_0069c444;                                   /* 0x0069c444 */
    float d_0069c448;                                   /* 0x0069c448 */
    float d_0069c44c;                                   /* 0x0069c44c */
    float d_0069c450;                                   /* 0x0069c450 */
    float d_0069c454;                                   /* 0x0069c454 */
    float d_0069c458;                                   /* 0x0069c458 */
    uint32_t d_0069c45c;                                /* 0x0069c45c */
    uint32_t d_0069c460;                                /* 0x0069c460 */
    uint32_t d_0069c464;                                /* 0x0069c464 */
    uint32_t d_0069c468;                                /* 0x0069c468 */
    float d_0069c46c;                                   /* 0x0069c46c */
    float d_0069c470;                                   /* 0x0069c470 */
    float d_0069c474;                                   /* 0x0069c474 */
    float d_0069c478;                                   /* 0x0069c478 */
    float d_0069c47c;                                   /* 0x0069c47c */
    float d_0069c480;                                   /* 0x0069c480 */
    float d_0069c484;                                   /* 0x0069c484 */
    float d_0069c488;                                   /* 0x0069c488 */
    uint32_t d_0069c48c;                                /* 0x0069c48c */
    uint32_t d_0069c490;                                /* 0x0069c490 */
    uint32_t d_0069c494;                                /* 0x0069c494 */
    uint32_t d_0069c498;                                /* 0x0069c498 */
    float d_0069c49c[2];                                /* 0x0069c49c */
    float d_0069c4a4[2];                                /* 0x0069c4a4 */
    uint32_t d_0069c4ac;                                /* 0x0069c4ac */
    uint32_t d_0069c4b0;                                /* 0x0069c4b0 */
    uint32_t d_0069c4b4;                                /* 0x0069c4b4 */
    uint32_t d_0069c4b8;                                /* 0x0069c4b8 */
    uint32_t d_0069c4bc[2];                             /* 0x0069c4bc */
    float d_0069c4c4;                                   /* 0x0069c4c4 */
    float d_0069c4c8;                                   /* 0x0069c4c8 */
    float d_0069c4cc;                                   /* 0x0069c4cc */
    float d_0069c4d0;                                   /* 0x0069c4d0 */
    float d_0069c4d4;                                   /* 0x0069c4d4 */
    float d_0069c4d8;                                   /* 0x0069c4d8 */
    float d_0069c4dc;                                   /* 0x0069c4dc */
    float d_0069c4e0;                                   /* 0x0069c4e0 */
    float d_0069c4e4;                                   /* 0x0069c4e4 */
    float d_0069c4e8;                                   /* 0x0069c4e8 */
    uint32_t d_0069c4ec;                                /* 0x0069c4ec */
    uint32_t d_0069c4f0;                                /* 0x0069c4f0 */
    uint32_t d_0069c4f4;                                /* 0x0069c4f4 */
    uint32_t d_0069c4f8;                                /* 0x0069c4f8 */
    uint32_t d_0069c4fc;                                /* 0x0069c4fc */
    uint32_t d_0069c500;                                /* 0x0069c500 */
    uint32_t d_0069c504;                                /* 0x0069c504 */
    uint32_t d_0069c508[3];                             /* 0x0069c508 */
    float d_0069c514[2];                                /* 0x0069c514 */
    float d_0069c51c[3];                                /* 0x0069c51c */
    float d_0069c528[2];                                /* 0x0069c528 */
    float d_0069c530[3];                                /* 0x0069c530 */
    float d_0069c53c[2];                                /* 0x0069c53c */
    float d_0069c544[2];                                /* 0x0069c544 */
    float d_0069c54c[2];                                /* 0x0069c54c */
    float d_0069c554;                                   /* 0x0069c554 */
    float d_0069c558;                                   /* 0x0069c558 */
    float d_0069c55c[2];                                /* 0x0069c55c */
    float d_0069c564;                                   /* 0x0069c564 */
    float d_0069c568;                                   /* 0x0069c568 */
    float d_0069c56c[3];                                /* 0x0069c56c */
    float d_0069c578;                                   /* 0x0069c578 */
    float d_0069c57c[2];                                /* 0x0069c57c */
    float d_0069c584[2];                                /* 0x0069c584 */
    float d_0069c58c;                                   /* 0x0069c58c */
    float d_0069c590[3];                                /* 0x0069c590 */
    float d_0069c59c[2];                                /* 0x0069c59c */
    float d_0069c5a4[2];                                /* 0x0069c5a4 */
    float d_0069c5ac[3];                                /* 0x0069c5ac */
    float d_0069c5b8[2];                                /* 0x0069c5b8 */
    float rwD3DOne;                                     /* 0x0069c5c0 */
    float d_0069c5c4[4];                                /* 0x0069c5c4 */
    unsigned char d_0069c5d4[4];                        /* 0x0069c5d4 */
    float d_0069c5d8;                                   /* 0x0069c5d8 */
    float d_0069c5dc;                                   /* 0x0069c5dc */
    float d_0069c5e0;                                   /* 0x0069c5e0 */
    float d_0069c5e4;                                   /* 0x0069c5e4 */
    float d_0069c5e8;                                   /* 0x0069c5e8 */
    float d_0069c5ec;                                   /* 0x0069c5ec */
    float d_0069c5f0;                                   /* 0x0069c5f0 */
    float d_0069c5f4;                                   /* 0x0069c5f4 */
    float d_0069c5f8;                                   /* 0x0069c5f8 */
    float d_0069c5fc;                                   /* 0x0069c5fc */
    float d_0069c600;                                   /* 0x0069c600 */
    float d_0069c604[4];                                /* 0x0069c604 */
    float d_0069c614;                                   /* 0x0069c614 */
    float d_0069c618[9];                                /* 0x0069c618 */
    float rpAnimDurationStart;                          /* 0x0069c63c */
    float rpAnimSumStartA;                              /* 0x0069c640 */
    float rpAnimSumStartB;                              /* 0x0069c644 */
    unsigned char pad_0069c648[24];                     /* 0x0069c648 */
    unsigned char d_0069c660[4];                        /* 0x0069c660 */
    unsigned char d_0069c664[4];                        /* 0x0069c664 */
    unsigned char d_0069c668[4];                        /* 0x0069c668 */
    unsigned char d_0069c66c[4];                        /* 0x0069c66c */
    unsigned char d_0069c670[4];                        /* 0x0069c670 */
    unsigned char d_0069c674[4];                        /* 0x0069c674 */
    float d_0069c678;                                   /* 0x0069c678 */
    float d_0069c67c;                                   /* 0x0069c67c */
    unsigned char d_0069c680[4];                        /* 0x0069c680 */
    unsigned char d_0069c684[4];                        /* 0x0069c684 */
    unsigned char d_0069c688[4];                        /* 0x0069c688 */
    unsigned char d_0069c68c[4];                        /* 0x0069c68c */
    unsigned char d_0069c690[8];                        /* 0x0069c690 */
    float rpAnimTimeOffset;                             /* 0x0069c698 */
    float rpAnimStartStep;                              /* 0x0069c69c */
    unsigned char pad_0069c6a0[16];                     /* 0x0069c6a0 */
    char rpAnimHeadName;                                /* 0x0069c6b0 */
    char sDummy[7];                                     /* 0x0069c6b1 */
    float d_0069c6b8[26];                               /* 0x0069c6b8 */
    float d_0069c720;                                   /* 0x0069c720 */
    uint32_t d_0069c724[64];                            /* 0x0069c724 */
    float d_0069c824;                                   /* 0x0069c824 */
    float d_0069c828;                                   /* 0x0069c828 */
    float d_0069c82c;                                   /* 0x0069c82c */
    double d_0069c830;                                  /* 0x0069c830 */
    float d_0069c838;                                   /* 0x0069c838 */
    float d_0069c83c[5];                                /* 0x0069c83c */
    char s105[8];                                       /* 0x0069c850 */
    unsigned char pad_0069c858[8];                      /* 0x0069c858 */
    char sIncompatibleLibpngVersionInApplicationA[56];  /* 0x0069c860 */
    unsigned char pad_0069c898[8];                      /* 0x0069c898 */
    char sZlibMemoryError[20];                          /* 0x0069c8a0 */
    unsigned char pad_0069c8b4[12];                     /* 0x0069c8b4 */
    char sZlibVersionError[20];                         /* 0x0069c8c0 */
    unsigned char pad_0069c8d4[12];                     /* 0x0069c8d4 */
    char sUnknownZlibError[20];                         /* 0x0069c8e0 */
    unsigned char pad_0069c8f4[12];                     /* 0x0069c8f4 */
    char s113[8];                                       /* 0x0069c900 */
    unsigned char pad_0069c908[8];                      /* 0x0069c908 */
    char sZlibMemory[12];                               /* 0x0069c910 */
    unsigned char pad_0069c91c[4];                      /* 0x0069c91c */
    char sZlibVersion[16];                              /* 0x0069c920 */
    char sNotAPNGFile[16];                              /* 0x0069c930 */
    char sPNGFileCorruptedByASCIIConversion[40];        /* 0x0069c940 */
    unsigned char pad_0069c968[8];                      /* 0x0069c968 */
    char sMissingIHDRBeforeIDAT[28];                    /* 0x0069c970 */
    unsigned char pad_0069c98c[4];                      /* 0x0069c98c */
    char sMissingPLTEBeforeIDAT[28];                    /* 0x0069c990 */
    unsigned char pad_0069c9ac[4];                      /* 0x0069c9ac */
    char sInvalidAttemptToReadRowData[36];              /* 0x0069c9b0 */
    unsigned char pad_0069c9d4[12];                     /* 0x0069c9d4 */
    char sNotEnoughImageData[24];                       /* 0x0069c9e0 */
    unsigned char pad_0069c9f8[8];                      /* 0x0069c9f8 */
    char sExtraCompressedData[24];                      /* 0x0069ca00 */
    unsigned char pad_0069ca18[8];                      /* 0x0069ca18 */
    char sDecompressionError[20];                       /* 0x0069ca20 */
    unsigned char pad_0069ca34[12];                     /* 0x0069ca34 */
    char sTooManyIDATSFound[24];                        /* 0x0069ca40 */
    unsigned char pad_0069ca58[8];                      /* 0x0069ca58 */
    char sLibpngVersion105October151999Header[52];      /* 0x0069ca60 */
    unsigned char pad_0069ca94[12];                     /* 0x0069ca94 */
    char sTooManyBytesForPNGSignature[36];              /* 0x0069caa0 */
    unsigned char pad_0069cac4[12];                     /* 0x0069cac4 */
    char sLibpngVersion105October151999CopyrightC[200]; /* 0x0069cad0 */
    unsigned char pad_0069cb98[8];                      /* 0x0069cb98 */
    char sCallToNULLReadFunction[28];                   /* 0x0069cba0 */
    unsigned char pad_0069cbbc[4];                      /* 0x0069cbbc */
    char sItSAnErrorToSetBothReadDataFnAndWriteDa[68];  /* 0x0069cbc0 */
    unsigned char pad_0069cc04[12];                     /* 0x0069cc04 */
    char sSameStructureResettingWriteDataFnToNULL[52];  /* 0x0069cc10 */
    unsigned char pad_0069cc44[12];                     /* 0x0069cc44 */
    double d_0069cc50;                                  /* 0x0069cc50 */
    unsigned char d_0069cc58[8];                        /* 0x0069cc58 */
    char sWidthTooLargeForLibpngToProcessImageDat[52];  /* 0x0069cc60 */
    unsigned char pad_0069cc94[12];                     /* 0x0069cc94 */
    double d_0069cca0;                                  /* 0x0069cca0 */
    double d_0069cca8;                                  /* 0x0069cca8 */
    double d_0069ccb0;                                  /* 0x0069ccb0 */
    double d_0069ccb8;                                  /* 0x0069ccb8 */
    double d_0069ccc0;                                  /* 0x0069ccc0 */
    double d_0069ccc8;                                  /* 0x0069ccc8 */
    unsigned char d_0069ccd0[8];                        /* 0x0069ccd0 */
    unsigned char d_0069ccd8[8];                        /* 0x0069ccd8 */
    unsigned char d_0069cce0[8];                        /* 0x0069cce0 */
    double d_0069cce8;                                  /* 0x0069cce8 */
    double d_0069ccf0[2];                               /* 0x0069ccf0 */
    uint32_t d_0069cd00[2];                             /* 0x0069cd00 */
    double d_0069cd08;                                  /* 0x0069cd08 */
    double d_0069cd10;                                  /* 0x0069cd10 */
    double d_0069cd18[2];                               /* 0x0069cd18 */
    uint32_t d_0069cd28[2];                             /* 0x0069cd28 */
    double d_0069cd30;                                  /* 0x0069cd30 */
    double d_0069cd38;                                  /* 0x0069cd38 */
    double d_0069cd40;                                  /* 0x0069cd40 */
    double d_0069cd48[2];                               /* 0x0069cd48 */
    uint32_t d_0069cd58[2];                             /* 0x0069cd58 */
    double d_0069cd60;                                  /* 0x0069cd60 */
    double d_0069cd68;                                  /* 0x0069cd68 */
    double d_0069cd70;                                  /* 0x0069cd70 */
    double d_0069cd78;                                  /* 0x0069cd78 */
    double d_0069cd80[2];                               /* 0x0069cd80 */
    double d_0069cd90;                                  /* 0x0069cd90 */
    double d_0069cd98;                                  /* 0x0069cd98 */
    double d_0069cda0;                                  /* 0x0069cda0 */
    double d_0069cda8[2];                               /* 0x0069cda8 */
    uint32_t d_0069cdb8[2];                             /* 0x0069cdb8 */
    double d_0069cdc0;                                  /* 0x0069cdc0 */
    double d_0069cdc8;                                  /* 0x0069cdc8 */
    double d_0069cdd0;                                  /* 0x0069cdd0 */
    double d_0069cdd8[2];                               /* 0x0069cdd8 */
    uint32_t d_0069cde8[2];                             /* 0x0069cde8 */
    double d_0069cdf0;                                  /* 0x0069cdf0 */
    double d_0069cdf8;                                  /* 0x0069cdf8 */
    uint32_t d_0069ce00[2];                             /* 0x0069ce00 */
    double d_0069ce08;                                  /* 0x0069ce08 */
    double d_0069ce10;                                  /* 0x0069ce10 */
    double d_0069ce18;                                  /* 0x0069ce18 */
    double d_0069ce20;                                  /* 0x0069ce20 */
    double d_0069ce28;                                  /* 0x0069ce28 */
    double d_0069ce30;                                  /* 0x0069ce30 */
    uint32_t d_0069ce38[2];                             /* 0x0069ce38 */
    uint32_t d_0069ce40[2];                             /* 0x0069ce40 */
    uint32_t d_0069ce48[2];                             /* 0x0069ce48 */
    double d_0069ce50;                                  /* 0x0069ce50 */
    double d_0069ce58;                                  /* 0x0069ce58 */
    double d_0069ce60;                                  /* 0x0069ce60 */
    double d_0069ce68;                                  /* 0x0069ce68 */
    double d_0069ce70;                                  /* 0x0069ce70 */
    double d_0069ce78;                                  /* 0x0069ce78 */
    uint32_t d_0069ce80[2];                             /* 0x0069ce80 */
    double d_0069ce88;                                  /* 0x0069ce88 */
    double d_0069ce90;                                  /* 0x0069ce90 */
    uint32_t d_0069ce98[2];                             /* 0x0069ce98 */
    double d_0069cea0;                                  /* 0x0069cea0 */
    double d_0069cea8;                                  /* 0x0069cea8 */
    uint32_t d_0069ceb0[2];                             /* 0x0069ceb0 */
    double d_0069ceb8;                                  /* 0x0069ceb8 */
    double d_0069cec0;                                  /* 0x0069cec0 */
    double d_0069cec8[2];                               /* 0x0069cec8 */
    double d_0069ced8[2];                               /* 0x0069ced8 */
    double d_0069cee8[2];                               /* 0x0069cee8 */
    double d_0069cef8[2];                               /* 0x0069cef8 */
    double d_0069cf08;                                  /* 0x0069cf08 */
    double d_0069cf10;                                  /* 0x0069cf10 */
    double d_0069cf18;                                  /* 0x0069cf18 */
    double d_0069cf20;                                  /* 0x0069cf20 */
    double d_0069cf28;                                  /* 0x0069cf28 */
    double d_0069cf30;                                  /* 0x0069cf30 */
    double d_0069cf38;                                  /* 0x0069cf38 */
    double d_0069cf40;                                  /* 0x0069cf40 */
    double d_0069cf48;                                  /* 0x0069cf48 */
    double d_0069cf50;                                  /* 0x0069cf50 */
    double d_0069cf58;                                  /* 0x0069cf58 */
    double d_0069cf60;                                  /* 0x0069cf60 */
    double d_0069cf68;                                  /* 0x0069cf68 */
    double d_0069cf70;                                  /* 0x0069cf70 */
    double d_0069cf78;                                  /* 0x0069cf78 */
    double d_0069cf80[2];                               /* 0x0069cf80 */
    double d_0069cf90[2];                               /* 0x0069cf90 */
    double d_0069cfa0[2];                               /* 0x0069cfa0 */
    double d_0069cfb0[2];                               /* 0x0069cfb0 */
    char sCanTDiscardCriticalDataOnCRCError[44];        /* 0x0069cfc0 */
    unsigned char pad_0069cfec[4];                      /* 0x0069cfec */
    char sApplicationMustSupplyAKnownBackgroundGa[52];  /* 0x0069cff0 */
    unsigned char pad_0069d024[12];                     /* 0x0069d024 */
    char sIgnoringOutOfRangeRgbToGrayCoefficients[48];  /* 0x0069d030 */
    char sNULLRowBuffer[16];                            /* 0x0069d060 */
    char sPngDoRgbToGrayFoundNongrayPixel[40];          /* 0x0069d070 */
    unsigned char pad_0069d098[8];                      /* 0x0069d098 */
    char sPngDoDitherReturnedRowbytes0[36];             /* 0x0069d0a0 */
    unsigned char pad_0069d0c4[12];                     /* 0x0069d0c4 */
    char sOutOfMemory[16];                              /* 0x0069d0d0 */
    char sOverflowInPngMemcpyCheck[32];                 /* 0x0069d0e0 */
    char sOverflowInPngMemsetCheck[32];                 /* 0x0069d100 */
    void *p_0069d120;                                   /* 0x0069d120 */
    void *p_0069d124;                                   /* 0x0069d124 */
    void *p_0069d128;                                   /* 0x0069d128 */
    void *p_0069d12c;                                   /* 0x0069d12c */
    void *p_0069d130;                                   /* 0x0069d130 */
    void *p_0069d134;                                   /* 0x0069d134 */
    void *p_0069d138;                                   /* 0x0069d138 */
    void *p_0069d13c;                                   /* 0x0069d13c */
    void *p_0069d140;                                   /* 0x0069d140 */
    void *p_0069d144;                                   /* 0x0069d144 */
    void *p_0069d148;                                   /* 0x0069d148 */
    void *p_0069d14c;                                   /* 0x0069d14c */
    void *p_0069d150;                                   /* 0x0069d150 */
    void *p_0069d154;                                   /* 0x0069d154 */
    unsigned char pad_0069d158[8];                      /* 0x0069d158 */
    char sUnknownCompressionMethod[28];                 /* 0x0069d160 */
    unsigned char pad_0069d17c[4];                      /* 0x0069d17c */
    char sInvalidWindowSize[20];                        /* 0x0069d180 */
    unsigned char pad_0069d194[12];                     /* 0x0069d194 */
    char sIncorrectHeaderCheck[24];                     /* 0x0069d1a0 */
    unsigned char pad_0069d1b8[8];                      /* 0x0069d1b8 */
    char sNeedDictionary[16];                           /* 0x0069d1c0 */
    char sIncorrectDataCheck[24];                       /* 0x0069d1d0 */
    unsigned char pad_0069d1e8[8];                      /* 0x0069d1e8 */
    double d_0069d1f0;                                  /* 0x0069d1f0 */
    double d_0069d1f8;                                  /* 0x0069d1f8 */
    float d_0069d200;                                   /* 0x0069d200 */
    float d_0069d204;                                   /* 0x0069d204 */
    float d_0069d208;                                   /* 0x0069d208 */
    unsigned char d_0069d20c[4];                        /* 0x0069d20c */
    double d_0069d210;                                  /* 0x0069d210 */
    unsigned char d_0069d218[8];                        /* 0x0069d218 */
    double d_0069d220;                                  /* 0x0069d220 */
    double d_0069d228;                                  /* 0x0069d228 */
    float d_0069d230;                                   /* 0x0069d230 */
    float d_0069d234;                                   /* 0x0069d234 */
    unsigned char d_0069d238[8];                        /* 0x0069d238 */
    double d_0069d240;                                  /* 0x0069d240 */
    unsigned char d_0069d248[8];                        /* 0x0069d248 */
    double d_0069d250;                                  /* 0x0069d250 */
    double d_0069d258;                                  /* 0x0069d258 */
    float d_0069d260;                                   /* 0x0069d260 */
    float d_0069d264;                                   /* 0x0069d264 */
    unsigned char d_0069d268[8];                        /* 0x0069d268 */
    double d_0069d270;                                  /* 0x0069d270 */
    unsigned char d_0069d278[8];                        /* 0x0069d278 */
    double d_0069d280;                                  /* 0x0069d280 */
    double d_0069d288;                                  /* 0x0069d288 */
    float d_0069d290;                                   /* 0x0069d290 */
    float d_0069d294;                                   /* 0x0069d294 */
    unsigned char d_0069d298[4];                        /* 0x0069d298 */
    float d_0069d29c;                                   /* 0x0069d29c */
    unsigned char d_0069d2a0[4];                        /* 0x0069d2a0 */
    float d_0069d2a4;                                   /* 0x0069d2a4 */
    float d_0069d2a8;                                   /* 0x0069d2a8 */
    float d_0069d2ac;                                   /* 0x0069d2ac */
    uint32_t d_0069d2b0[2];                             /* 0x0069d2b0 */
    float d_0069d2b8[2];                                /* 0x0069d2b8 */
    uint32_t d_0069d2c0[2];                             /* 0x0069d2c0 */
    float d_0069d2c8[2];                                /* 0x0069d2c8 */
    uint32_t d_0069d2d0[2];                             /* 0x0069d2d0 */
    float d_0069d2d8[2];                                /* 0x0069d2d8 */
    uint32_t d_0069d2e0[2];                             /* 0x0069d2e0 */
    float d_0069d2e8[2];                                /* 0x0069d2e8 */
    uint32_t d_0069d2f0[2];                             /* 0x0069d2f0 */
    float d_0069d2f8[2];                                /* 0x0069d2f8 */
    uint32_t d_0069d300[2];                             /* 0x0069d300 */
    float d_0069d308[2];                                /* 0x0069d308 */
    uint32_t d_0069d310[2];                             /* 0x0069d310 */
    float d_0069d318[2];                                /* 0x0069d318 */
    uint32_t d_0069d320[2];                             /* 0x0069d320 */
    double d_0069d328;                                  /* 0x0069d328 */
    float d_0069d330[2];                                /* 0x0069d330 */
    double d_0069d338;                                  /* 0x0069d338 */
    double d_0069d340;                                  /* 0x0069d340 */
    float d_0069d348[2];                                /* 0x0069d348 */
    uint32_t d_0069d350[2];                             /* 0x0069d350 */
    float d_0069d358[2];                                /* 0x0069d358 */
    uint32_t d_0069d360[2];                             /* 0x0069d360 */
    float d_0069d368[2];                                /* 0x0069d368 */
    uint32_t d_0069d370[2];                             /* 0x0069d370 */
    float d_0069d378[2];                                /* 0x0069d378 */
    uint32_t d_0069d380[2];                             /* 0x0069d380 */
    float d_0069d388[2];                                /* 0x0069d388 */
    uint32_t d_0069d390[2];                             /* 0x0069d390 */
    float d_0069d398[2];                                /* 0x0069d398 */
    uint32_t d_0069d3a0[2];                             /* 0x0069d3a0 */
    float d_0069d3a8[2];                                /* 0x0069d3a8 */
    uint32_t d_0069d3b0[2];                             /* 0x0069d3b0 */
    float d_0069d3b8[2];                                /* 0x0069d3b8 */
    uint32_t d_0069d3c0;                                /* 0x0069d3c0 */
    char sMbP[8];                                       /* 0x0069d3c4 */
    unsigned char pad_0069d3cc[4];                      /* 0x0069d3cc */
    char sCRCError[12];                                 /* 0x0069d3d0 */
    unsigned char pad_0069d3dc[4];                      /* 0x0069d3dc */
    char sOutOfPlaceIHDR[20];                           /* 0x0069d3e0 */
    unsigned char pad_0069d3f4[12];                     /* 0x0069d3f4 */
    char sInvalidIHDRChunk[20];                         /* 0x0069d400 */
    unsigned char pad_0069d414[12];                     /* 0x0069d414 */
    char sInvalidImageSizeInIHDR[28];                   /* 0x0069d420 */
    unsigned char pad_0069d43c[4];                      /* 0x0069d43c */
    char sInvalidBitDepthInIHDR[28];                    /* 0x0069d440 */
    unsigned char pad_0069d45c[4];                      /* 0x0069d45c */
    char sInvalidColorTypeInIHDR[28];                   /* 0x0069d460 */
    unsigned char pad_0069d47c[4];                      /* 0x0069d47c */
    char sInvalidColorTypeBitDepthCombinationInIH[52];  /* 0x0069d480 */
    unsigned char pad_0069d4b4[12];                     /* 0x0069d4b4 */
    char sUnknownInterlaceMethodInIHDR[36];             /* 0x0069d4c0 */
    unsigned char pad_0069d4e4[12];                     /* 0x0069d4e4 */
    char sUnknownCompressionMethodInIHDR[36];           /* 0x0069d4f0 */
    unsigned char pad_0069d514[12];                     /* 0x0069d514 */
    char sUnknownFilterMethodInIHDR[32];                /* 0x0069d520 */
    char sMissingIHDRBeforePLTE[28];                    /* 0x0069d540 */
    unsigned char pad_0069d55c[4];                      /* 0x0069d55c */
    char sInvalidPLTEAfterIDAT[24];                     /* 0x0069d560 */
    unsigned char pad_0069d578[8];                      /* 0x0069d578 */
    char sDuplicatePLTEChunk[24];                       /* 0x0069d580 */
    unsigned char pad_0069d598[8];                      /* 0x0069d598 */
    char sInvalidPaletteChunk[24];                      /* 0x0069d5a0 */
    unsigned char pad_0069d5b8[8];                      /* 0x0069d5b8 */
    char sTruncatingIncorrectTRNSChunkLength[40];       /* 0x0069d5c0 */
    unsigned char pad_0069d5e8[8];                      /* 0x0069d5e8 */
    char sNoImageInFile[20];                            /* 0x0069d5f0 */
    unsigned char pad_0069d604[12];                     /* 0x0069d604 */
    char sIncorrectIENDChunkLength[28];                 /* 0x0069d610 */
    unsigned char pad_0069d62c[4];                      /* 0x0069d62c */
    char sMissingIHDRBeforeGAMA[28];                    /* 0x0069d630 */
    unsigned char pad_0069d64c[4];                      /* 0x0069d64c */
    char sInvalidGAMAAfterIDAT[24];                     /* 0x0069d650 */
    unsigned char pad_0069d668[8];                      /* 0x0069d668 */
    char sOutOfPlaceGAMAChunk[24];                      /* 0x0069d670 */
    unsigned char pad_0069d688[8];                      /* 0x0069d688 */
    char sDuplicateGAMAChunk[24];                       /* 0x0069d690 */
    unsigned char pad_0069d6a8[8];                      /* 0x0069d6a8 */
    char sIncorrectGAMAChunkLength[28];                 /* 0x0069d6b0 */
    unsigned char pad_0069d6cc[4];                      /* 0x0069d6cc */
    char sIgnoringIncorrectGAMAValueWhenSRGBIsAls[56];  /* 0x0069d6d0 */
    unsigned char pad_0069d708[8];                      /* 0x0069d708 */
    char sMissingIHDRBeforeSBIT[28];                    /* 0x0069d710 */
    unsigned char pad_0069d72c[4];                      /* 0x0069d72c */
    char sInvalidSBITAfterIDAT[24];                     /* 0x0069d730 */
    unsigned char pad_0069d748[8];                      /* 0x0069d748 */
    char sOutOfPlaceSBITChunk[24];                      /* 0x0069d750 */
    unsigned char pad_0069d768[8];                      /* 0x0069d768 */
    char sDuplicateSBITChunk[24];                       /* 0x0069d770 */
    unsigned char pad_0069d788[8];                      /* 0x0069d788 */
    char sIncorrectSBITChunkLength[28];                 /* 0x0069d790 */
    unsigned char pad_0069d7ac[4];                      /* 0x0069d7ac */
    char sInvalidCHRMAfterIDAT[24];                     /* 0x0069d7b0 */
    unsigned char pad_0069d7c8[8];                      /* 0x0069d7c8 */
    char sMissingPLTEBeforeCHRM[28];                    /* 0x0069d7d0 */
    unsigned char pad_0069d7ec[4];                      /* 0x0069d7ec */
    char sDuplicateCHRMChunk[24];                       /* 0x0069d7f0 */
    unsigned char pad_0069d808[8];                      /* 0x0069d808 */
    char sIncorrectCHRMChunkLength[28];                 /* 0x0069d810 */
    unsigned char pad_0069d82c[4];                      /* 0x0069d82c */
    char sInvalidCHRMWhitePoint[28];                    /* 0x0069d830 */
    unsigned char pad_0069d84c[4];                      /* 0x0069d84c */
    char sInvalidCHRMRedPoint[24];                      /* 0x0069d850 */
    unsigned char pad_0069d868[8];                      /* 0x0069d868 */
    char sInvalidCHRMGreenPoint[28];                    /* 0x0069d870 */
    unsigned char pad_0069d88c[4];                      /* 0x0069d88c */
    char sInvalidCHRMBluePoint[24];                     /* 0x0069d890 */
    unsigned char pad_0069d8a8[8];                      /* 0x0069d8a8 */
    char sIgnoringIncorrectCHRMValueWhenSRGBIsAls[56];  /* 0x0069d8b0 */
    unsigned char pad_0069d8e8[8];                      /* 0x0069d8e8 */
    char sMissingIHDRBeforeSRGB[28];                    /* 0x0069d8f0 */
    unsigned char pad_0069d90c[4];                      /* 0x0069d90c */
    char sInvalidSRGBAfterIDAT[24];                     /* 0x0069d910 */
    unsigned char pad_0069d928[8];                      /* 0x0069d928 */
    char sOutOfPlaceSRGBChunk[24];                      /* 0x0069d930 */
    unsigned char pad_0069d948[8];                      /* 0x0069d948 */
    char sDuplicateSRGBChunk[24];                       /* 0x0069d950 */
    unsigned char pad_0069d968[8];                      /* 0x0069d968 */
    char sIncorrectSRGBChunkLength[28];                 /* 0x0069d970 */
    unsigned char pad_0069d98c[4];                      /* 0x0069d98c */
    char sUnknownSRGBIntent[20];                        /* 0x0069d990 */
    unsigned char pad_0069d9a4[12];                     /* 0x0069d9a4 */
    char sMissingIHDRBeforeTRNS[28];                    /* 0x0069d9b0 */
    unsigned char pad_0069d9cc[4];                      /* 0x0069d9cc */
    char sInvalidTRNSAfterIDAT[24];                     /* 0x0069d9d0 */
    unsigned char pad_0069d9e8[8];                      /* 0x0069d9e8 */
    char sDuplicateTRNSChunk[24];                       /* 0x0069d9f0 */
    unsigned char pad_0069da08[8];                      /* 0x0069da08 */
    char sMissingPLTEBeforeTRNS[28];                    /* 0x0069da10 */
    unsigned char pad_0069da2c[4];                      /* 0x0069da2c */
    char sIncorrectTRNSChunkLength[28];                 /* 0x0069da30 */
    unsigned char pad_0069da4c[4];                      /* 0x0069da4c */
    char sZeroLengthTRNSChunk[24];                      /* 0x0069da50 */
    unsigned char pad_0069da68[8];                      /* 0x0069da68 */
    char sTRNSChunkNotAllowedWithAlphaChannel[44];      /* 0x0069da70 */
    unsigned char pad_0069da9c[4];                      /* 0x0069da9c */
    char sMissingIHDRBeforeBKGD[28];                    /* 0x0069daa0 */
    unsigned char pad_0069dabc[4];                      /* 0x0069dabc */
    char sInvalidBKGDAfterIDAT[24];                     /* 0x0069dac0 */
    unsigned char pad_0069dad8[8];                      /* 0x0069dad8 */
    char sMissingPLTEBeforeBKGD[28];                    /* 0x0069dae0 */
    unsigned char pad_0069dafc[4];                      /* 0x0069dafc */
    char sDuplicateBKGDChunk[24];                       /* 0x0069db00 */
    unsigned char pad_0069db18[8];                      /* 0x0069db18 */
    char sIncorrectBKGDChunkLength[28];                 /* 0x0069db20 */
    unsigned char pad_0069db3c[4];                      /* 0x0069db3c */
    char sIncorrectBKGDChunkIndexValue[36];             /* 0x0069db40 */
    unsigned char pad_0069db64[12];                     /* 0x0069db64 */
    char sMissingIHDRBeforeHIST[28];                    /* 0x0069db70 */
    unsigned char pad_0069db8c[4];                      /* 0x0069db8c */
    char sInvalidHISTAfterIDAT[24];                     /* 0x0069db90 */
    unsigned char pad_0069dba8[8];                      /* 0x0069dba8 */
    char sMissingPLTEBeforeHIST[28];                    /* 0x0069dbb0 */
    unsigned char pad_0069dbcc[4];                      /* 0x0069dbcc */
    char sDuplicateHISTChunk[24];                       /* 0x0069dbd0 */
    unsigned char pad_0069dbe8[8];                      /* 0x0069dbe8 */
    char sIncorrectHISTChunkLength[28];                 /* 0x0069dbf0 */
    unsigned char pad_0069dc0c[4];                      /* 0x0069dc0c */
    char sMissingIHDRBeforePHYS[28];                    /* 0x0069dc10 */
    unsigned char pad_0069dc2c[4];                      /* 0x0069dc2c */
    char sInvalidPHYSAfterIDAT[24];                     /* 0x0069dc30 */
    unsigned char pad_0069dc48[8];                      /* 0x0069dc48 */
    char sDuplicatePHYSChunk[24];                       /* 0x0069dc50 */
    unsigned char pad_0069dc68[8];                      /* 0x0069dc68 */
    char sIncorrectPHYsChunkLength[28];                 /* 0x0069dc70 */
    unsigned char pad_0069dc8c[4];                      /* 0x0069dc8c */
    char sMissingIHDRBeforeOFFs[28];                    /* 0x0069dc90 */
    unsigned char pad_0069dcac[4];                      /* 0x0069dcac */
    char sInvalidOFFsAfterIDAT[24];                     /* 0x0069dcb0 */
    unsigned char pad_0069dcc8[8];                      /* 0x0069dcc8 */
    char sDuplicateOFFsChunk[24];                       /* 0x0069dcd0 */
    unsigned char pad_0069dce8[8];                      /* 0x0069dce8 */
    char sIncorrectOFFsChunkLength[28];                 /* 0x0069dcf0 */
    unsigned char pad_0069dd0c[4];                      /* 0x0069dd0c */
    char sMissingIHDRBeforePCAL[28];                    /* 0x0069dd10 */
    unsigned char pad_0069dd2c[4];                      /* 0x0069dd2c */
    char sInvalidPCALAfterIDAT[24];                     /* 0x0069dd30 */
    unsigned char pad_0069dd48[8];                      /* 0x0069dd48 */
    char sDuplicatePCALChunk[24];                       /* 0x0069dd50 */
    unsigned char pad_0069dd68[8];                      /* 0x0069dd68 */
    char sInvalidPCALData[20];                          /* 0x0069dd70 */
    unsigned char pad_0069dd84[12];                     /* 0x0069dd84 */
    char sInvalidPCALParametersForEquationType[44];     /* 0x0069dd90 */
    unsigned char pad_0069ddbc[4];                      /* 0x0069ddbc */
    char sUnrecognizedEquationTypeForPCALChunk[44];     /* 0x0069ddc0 */
    unsigned char pad_0069ddec[4];                      /* 0x0069ddec */
    char sOutOfPlaceTIMEChunk[24];                      /* 0x0069ddf0 */
    unsigned char pad_0069de08[8];                      /* 0x0069de08 */
    char sDuplicateTIMEChunk[24];                       /* 0x0069de10 */
    unsigned char pad_0069de28[8];                      /* 0x0069de28 */
    char sIncorrectTIMEChunkLength[28];                 /* 0x0069de30 */
    unsigned char pad_0069de4c[4];                      /* 0x0069de4c */
    char sMissingIHDRBeforeTEXt[28];                    /* 0x0069de50 */
    unsigned char pad_0069de6c[4];                      /* 0x0069de6c */
    char sMissingIHDRBeforeZTXt[28];                    /* 0x0069de70 */
    unsigned char pad_0069de8c[4];                      /* 0x0069de8c */
    char sZeroLengthZTXtChunk[24];                      /* 0x0069de90 */
    unsigned char pad_0069dea8[8];                      /* 0x0069dea8 */
    char sUnknownZTXtCompressionType[32];               /* 0x0069deb0 */
    char sUnknownCriticalChunk[24];                     /* 0x0069ded0 */
    unsigned char pad_0069dee8[8];                      /* 0x0069dee8 */
    char sInvalidChunkType[20];                         /* 0x0069def0 */
    unsigned char pad_0069df04[12];                     /* 0x0069df04 */
    char sIgnoringBadAdaptiveFilterType[36];            /* 0x0069df10 */
    unsigned char pad_0069df34[12];                     /* 0x0069df34 */
    char s_0069df40[20];                                /* 0x0069df40 */
    unsigned char pad_0069df54[12];                     /* 0x0069df54 */
    char sExtraCompressionData[24];                     /* 0x0069df60 */
    unsigned char pad_0069df78[8];                      /* 0x0069df78 */
    char sRowbytesOverflowInPngReadStartRow[40];        /* 0x0069df80 */
    unsigned char pad_0069dfa8[8];                      /* 0x0069dfa8 */
    void *p_0069dfb0;                                   /* 0x0069dfb0 */
    void *p_0069dfb4;                                   /* 0x0069dfb4 */
    void *p_0069dfb8;                                   /* 0x0069dfb8 */
    void *p_0069dfbc;                                   /* 0x0069dfbc */
    void *p_0069dfc0;                                   /* 0x0069dfc0 */
    void *p_0069dfc4;                                   /* 0x0069dfc4 */
    void *p_0069dfc8;                                   /* 0x0069dfc8 */
    void *p_0069dfcc;                                   /* 0x0069dfcc */
    void *p_0069dfd0;                                   /* 0x0069dfd0 */
    void *p_0069dfd4;                                   /* 0x0069dfd4 */
    unsigned char pad_0069dfd8[8];                      /* 0x0069dfd8 */
    char sInvalidBlockType[20];                         /* 0x0069dfe0 */
    unsigned char pad_0069dff4[12];                     /* 0x0069dff4 */
    char sInvalidStoredBlockLengths[32];                /* 0x0069e000 */
    char sTooManyLengthOrDistanceSymbols[36];           /* 0x0069e020 */
    unsigned char pad_0069e044[12];                     /* 0x0069e044 */
    char sInvalidBitLengthRepeat[28];                   /* 0x0069e050 */
    unsigned char pad_0069e06c[4];                      /* 0x0069e06c */
    char sStreamEnd[12];                                /* 0x0069e070 */
    unsigned char pad_0069e07c[4];                      /* 0x0069e07c */
    char sFileError[12];                                /* 0x0069e080 */
    unsigned char pad_0069e08c[4];                      /* 0x0069e08c */
    char sStreamError[16];                              /* 0x0069e090 */
    char sDataError[12];                                /* 0x0069e0a0 */
    unsigned char pad_0069e0ac[4];                      /* 0x0069e0ac */
    char sInsufficientMemory[20];                       /* 0x0069e0b0 */
    unsigned char pad_0069e0c4[12];                     /* 0x0069e0c4 */
    char sBufferError[16];                              /* 0x0069e0d0 */
    char sIncompatibleVersion[24];                      /* 0x0069e0e0 */
    unsigned char pad_0069e0f8[8];                      /* 0x0069e0f8 */
    uint32_t d_0069e100[20];                            /* 0x0069e100 */
    char sWidthTooLargeToProcessImageDataRowbytes[64];  /* 0x0069e150 */
    void *p_0069e190;                                   /* 0x0069e190 */
    void *p_0069e194;                                   /* 0x0069e194 */
    void *p_0069e198;                                   /* 0x0069e198 */
    void *p_0069e19c;                                   /* 0x0069e19c */
    void *p_0069e1a0;                                   /* 0x0069e1a0 */
    void *p_0069e1a4;                                   /* 0x0069e1a4 */
    void *p_0069e1a8;                                   /* 0x0069e1a8 */
    void *p_0069e1ac;                                   /* 0x0069e1ac */
    void *p_0069e1b0;                                   /* 0x0069e1b0 */
    void *p_0069e1b4;                                   /* 0x0069e1b4 */
    unsigned char pad_0069e1b8[8];                      /* 0x0069e1b8 */
    char sInvalidLiteralLengthCode[28];                 /* 0x0069e1c0 */
    unsigned char pad_0069e1dc[4];                      /* 0x0069e1dc */
    char sInvalidDistanceCode[24];                      /* 0x0069e1e0 */
    unsigned char pad_0069e1f8[8];                      /* 0x0069e1f8 */
    char sOversubscribedDynamicBitLengthsTree[40];      /* 0x0069e200 */
    unsigned char pad_0069e228[8];                      /* 0x0069e228 */
    char sIncompleteDynamicBitLengthsTree[36];          /* 0x0069e230 */
    unsigned char pad_0069e254[12];                     /* 0x0069e254 */
    char sOversubscribedLiteralLengthTree[36];          /* 0x0069e260 */
    unsigned char pad_0069e284[12];                     /* 0x0069e284 */
    char sIncompleteLiteralLengthTree[32];              /* 0x0069e290 */
    char sOversubscribedDistanceTree[32];               /* 0x0069e2b0 */
    char sIncompleteDistanceTree[28];                   /* 0x0069e2d0 */
    unsigned char pad_0069e2ec[4];                      /* 0x0069e2ec */
    char sEmptyDistanceTreeWithLengths[36];             /* 0x0069e2f0 */
    unsigned char pad_0069e314[12];                     /* 0x0069e314 */
    unsigned char d_0069e320[4];                        /* 0x0069e320 */
    float d_0069e324[4];                                /* 0x0069e324 */
    unsigned char d_0069e334[4];                        /* 0x0069e334 */
    float d_0069e338;                                   /* 0x0069e338 */
    float d_0069e33c;                                   /* 0x0069e33c */
    float d_0069e340[2];                                /* 0x0069e340 */
    double d_0069e348;                                  /* 0x0069e348 */
    float d_0069e350;                                   /* 0x0069e350 */
    float d_0069e354;                                   /* 0x0069e354 */
    unsigned char d_0069e358[4];                        /* 0x0069e358 */
    float d_0069e35c[3];                                /* 0x0069e35c */
    float d_0069e368[3];                                /* 0x0069e368 */
    unsigned char d_0069e374[4];                        /* 0x0069e374 */
    unsigned char d_0069e378[4];                        /* 0x0069e378 */
    float d_0069e37c[13];                               /* 0x0069e37c */
    float d_0069e3b0[5];                                /* 0x0069e3b0 */
    unsigned char d_0069e3c4[4];                        /* 0x0069e3c4 */
    unsigned char d_0069e3c8[4];                        /* 0x0069e3c8 */
    unsigned char d_0069e3cc[4];                        /* 0x0069e3cc */
    unsigned char d_0069e3d0[4];                        /* 0x0069e3d0 */
    float d_0069e3d4;                                   /* 0x0069e3d4 */
    unsigned char d_0069e3d8[4];                        /* 0x0069e3d8 */
    unsigned char d_0069e3dc[4];                        /* 0x0069e3dc */
    float d_0069e3e0;                                   /* 0x0069e3e0 */
    unsigned char d_0069e3e4[4];                        /* 0x0069e3e4 */
    unsigned char d_0069e3e8[4];                        /* 0x0069e3e8 */
    float d_0069e3ec;                                   /* 0x0069e3ec */
    float d_0069e3f0;                                   /* 0x0069e3f0 */
    float d_0069e3f4;                                   /* 0x0069e3f4 */
    float d_0069e3f8[19];                               /* 0x0069e3f8 */
    float d_0069e444;                                   /* 0x0069e444 */
    float d_0069e448;                                   /* 0x0069e448 */
    float d_0069e44c;                                   /* 0x0069e44c */
    float d_0069e450;                                   /* 0x0069e450 */
    float d_0069e454;                                   /* 0x0069e454 */
    float d_0069e458;                                   /* 0x0069e458 */
    float d_0069e45c;                                   /* 0x0069e45c */
    float d_0069e460;                                   /* 0x0069e460 */
    float d_0069e464;                                   /* 0x0069e464 */
    float d_0069e468;                                   /* 0x0069e468 */
    float d_0069e46c;                                   /* 0x0069e46c */
    float d_0069e470[4];                                /* 0x0069e470 */
    char sEntryNumberNotWithinListBounds[36];           /* 0x0069e480 */
    unsigned char pad_0069e4a4[12];                     /* 0x0069e4a4 */
    char sAttemptingToRemoveMoreEntriesThanExist[48];   /* 0x0069e4b0 */
    char sSListIndex[12];                               /* 0x0069e4e0 */
    unsigned char pad_0069e4ec[2970];                   /* 0x0069e4ec */
};

#define AT(member, addr) _Static_assert(offsetof(struct BmxData1, member) == (addr) - 0x0069b000, #member)
AT(rpSphereMinRadius, 0x0069b328);
AT(rpSphereMinRadiusSq, 0x0069b330);
AT(rpSphereMinDistance, 0x0069b334);
AT(rpWorldScale0, 0x0069b4f0);
AT(rpWorldScale1, 0x0069b4f4);
AT(rpWorldScale2, 0x0069b4f8);
AT(rpWorldReadScale0, 0x0069b4fc);
AT(rpWorldReadScale1, 0x0069b500);
AT(rpWorldReadScale2, 0x0069b504);
AT(rwD3DK0x0069b530, 0x0069b530);
AT(rwD3DK0x0069b534, 0x0069b534);
AT(rwD3DK0x0069b538, 0x0069b538);
AT(rwD3DK0x0069b544, 0x0069b544);
AT(rwD3DK0x0069b548, 0x0069b548);
AT(rwD3DK0x0069b54c, 0x0069b54c);
AT(rwD3DK0x0069b550, 0x0069b550);
AT(rwD3DK0x0069b554, 0x0069b554);
AT(rwD3DK0x0069b558, 0x0069b558);
AT(rwD3DK0x0069b55c, 0x0069b55c);
AT(rwD3DK0x0069b560, 0x0069b560);
AT(rwD3DK0x0069b564, 0x0069b564);
AT(rwD3DK0x0069b568, 0x0069b568);
AT(rwD3DK0x0069b56c, 0x0069b56c);
AT(rwD3DK0x0069b570, 0x0069b570);
AT(rwD3DK0x0069b574, 0x0069b574);
AT(rwD3DK0x0069b57c, 0x0069b57c);
AT(rwD3DK0x0069b580, 0x0069b580);
AT(rwD3DK0x0069b584, 0x0069b584);
AT(rwD3DK0x0069b588, 0x0069b588);
AT(rwD3DK0x0069b58c, 0x0069b58c);
AT(rwD3DK0x0069b590, 0x0069b590);
AT(rwD3DK0x0069b598, 0x0069b598);
AT(rwD3DK0x0069b5a0, 0x0069b5a0);
AT(rwD3DK0x0069b5a4, 0x0069b5a4);
AT(rwD3DK0x0069b5a8, 0x0069b5a8);
AT(rwD3DK0x0069b5ac, 0x0069b5ac);
AT(rwD3DK0x0069b5b0, 0x0069b5b0);
AT(rwD3DK0x0069b5b4, 0x0069b5b4);
AT(rwD3DK0x0069b5bc, 0x0069b5bc);
AT(rwD3DK0x0069b5c4, 0x0069b5c4);
AT(rwD3DK0x0069b5c8, 0x0069b5c8);
AT(rwD3DK0x0069b5cc, 0x0069b5cc);
AT(rwD3DK0x0069b5d4, 0x0069b5d4);
AT(rwD3DK0x0069b5d8, 0x0069b5d8);
AT(rwD3DK0x0069b5dc, 0x0069b5dc);
AT(rwD3DK0x0069b5e0, 0x0069b5e0);
AT(rwD3DK0x0069b5e8, 0x0069b5e8);
AT(rwD3DK0x0069b5f0, 0x0069b5f0);
AT(rwD3DK0x0069b604, 0x0069b604);
AT(rwD3DK0x0069b608, 0x0069b608);
AT(rwD3DK0x0069b60c, 0x0069b60c);
AT(rwMatrixNormalLimit0, 0x0069b8b4);
AT(rwMatrixNormalLimit1, 0x0069b8b8);
AT(rwMatrixNormalLimit2, 0x0069b8bc);
AT(rwInvSqrtScaleOdd, 0x0069b950);
AT(rwInvSqrtScaleEven, 0x0069b958);
AT(rwD3DFogRange, 0x0069b98c);
AT(rwPerspCornerSteps, 0x0069bc70);
AT(rwViewFactors, 0x0069bc88);
AT(rwMipSuffix, 0x0069bda0);
AT(rwD3DOne, 0x0069c5c0);
AT(rpAnimDurationStart, 0x0069c63c);
AT(rpAnimSumStartA, 0x0069c640);
AT(rpAnimSumStartB, 0x0069c644);
AT(rpAnimTimeOffset, 0x0069c698);
AT(rpAnimStartStep, 0x0069c69c);
AT(rpAnimHeadName, 0x0069c6b0);
#undef AT
_Static_assert(offsetof(struct BmxData1, pad_0069e4ec) + sizeof(((struct BmxData1 *)0)->pad_0069e4ec) ==
                   0x4086,
               "bmxData1");

/* The original's _rwdseg (RenderWare data), 0x006a0000..0x006a0109. */
struct BmxRwBss {
    unsigned char d_006a0000[4];   /* 0x006a0000 */
    unsigned char d_006a0004[261]; /* 0x006a0004 */
};

_Static_assert(offsetof(struct BmxRwBss, d_006a0004) + sizeof(((struct BmxRwBss *)0)->d_006a0004) == 0x109,
               "bmxRwBss");

/* Dinput.lib's object tables, which the linker put into the original's .text, 0x004afc60..0x004b0f20. */
struct BmxInputTables {
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

_Static_assert(offsetof(struct BmxInputTables, d_004b0f14) +
                       sizeof(((struct BmxInputTables *)0)->d_004b0f14) ==
                   0x12c0,
               "bmxInputTables");

#endif
