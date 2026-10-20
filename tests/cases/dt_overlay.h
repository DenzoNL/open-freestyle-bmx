/* Shared fixtures for the overlay tests (HUD elements and menu screens). */
#ifndef DT_OVERLAY_H
#define DT_OVERLAY_H

#include <stdint.h>

#include "dt.h"

/* One bit per stubbed callee (dt_stub supports 32 at a time); a case installs only what it calls. */
#define DTS(n)          (1ull << (n))
#define DTS_STRING      DTS(0)  /* zstringGet */
#define DTS_TEXTWIDTH   DTS(1)  /* zoverTextWidth */
#define DTS_ADDTEXT     DTS(2)  /* zoverAddText */
#define DTS_REMOVE      DTS(3)  /* zoverRemove */
#define DTS_SHOW        DTS(4)  /* zoverShow */
#define DTS_SETCOLOR    DTS(5)  /* zoverSetColor */
#define DTS_DRAWTEXT    DTS(6)  /* zoverDrawText */
#define DTS_DRAWRECT    DTS(7)  /* zoverDrawRect */
#define DTS_SOUND       DTS(8)  /* bmxSoundPlayAt */
#define DTS_PRESSED     DTS(9)  /* contrlerPressed */
#define DTS_ADDRECT     DTS(10) /* zoverAddRect */
#define DTS_SETRECT     DTS(11) /* zoverSetRect */
#define DTS_ADDGRADIENT DTS(12) /* zoverAddGradient */
#define DTS_ADDSPRITE   DTS(13) /* zoverAddSprite */
#define DTS_FLUSH       DTS(14) /* zoverFlush */
#define DTS_LAYERFLAG   DTS(15) /* zoverSetLayerFlag */
#define DTS_GETMODE     DTS(16) /* gameGetMode */
#define DTS_GETSUB      DTS(17) /* gameGetSubMode */
#define DTS_TESTOPT     DTS(18) /* gameTestOption */
#define DTS_RECORD      DTS(19) /* gameGetRecord */
#define DTS_RUNTYPE     DTS(20) /* gameGetRunType */
#define DTS_RUNCOUNT    DTS(21) /* gameGetRunCount */
#define DTS_STARTMODE   DTS(22) /* gameStartMode */
#define DTS_SETSUB      DTS(23) /* gameSetSubMode */
#define DTS_BIGGAME     DTS(24) /* gameBigGameFunction */
#define DTS_NAMETABLE   DTS(25) /* gameGetNameTable */
#define DTS_MODEBYTE    DTS(26) /* gameGetModeByte */
#define DTS_LEAVERUN    DTS(27) /* gameLeaveRun */
#define DTS_RESETGOALS  DTS(28) /* gameResetGoals */
#define DTS_PENDING     DTS(29) /* gameHasPendingUnlock */
#define DTS_GOALINFO    DTS(30) /* comdataGetGoalInfo */
#define DTS_LEVELREC    DTS(31) /* comdataGetLevelRecord */
#define DTS_SETVIEW     DTS(32) /* comdataSetViewMode */
#define DTS_RESETVIEW   DTS(33) /* comdataResetView */
#define DTS_CONTROLLER  DTS(34) /* contrlerGet */
#define DTS_CAMERA      DTS(35) /* cameraSelectPlayer */
#define DTS_RIDERRESET  DTS(36) /* riderReset */
#define DTS_ACTION      DTS(37) /* actionSetMode */
#define DTS_SAVEWRITE   DTS(38) /* saveFileWrite */
#define DTS_SAVECLOSE   DTS(39) /* saveFileClose */
#define DTS_SAVEQUERY   DTS(40) /* memcardFormat */
#define DTS_HEADER      DTS(41) /* overlympDrawHeader */
#define DTS_BODY        DTS(42) /* overlympDrawBody */
#define DTS_REPLAY      DTS(43) /* replayGetBuffer */
#define DTS_FILEOPEN    DTS(44) /* zfileOpen */
#define DTS_FILEWRITE   DTS(45) /* zfileWrite */
#define DTS_FILLBAR     DTS(46) /* ngifFillScreenBar */
#define DTS_RIDERREC    DTS(47) /* ngifzresGetRiderRecord */
#define DTS_REGISTER    DTS(48) /* ngifzresRegister */
#define DTS_ROLLERREC   DTS(49) /* zrollerGetRecord */
#define DTS_RESERVE     DTS(50) /* zrollerReserve */
#define DTS_DRAWCB      DTS(51) /* zrollerAddDrawCallback */
#define DTS_MENUMODE    DTS(52) /* zsystemSetMenuMode */
#define DTS_WINSTART    DTS(53) /* winchanStart */
#define DTS_WINCALLBACK DTS(54) /* winchanSetCallback */
#define DTS_WINQUERY    DTS(55) /* winchanQuery */
#define DTS_WINBEGIN    DTS(56) /* winchanBegin */
#define DTS_WINEND      DTS(57) /* winchanEnd */
#define DTS_ASSET       DTS(58) /* zassetFind, zassetSize, zassetRead */
#define DTS_ALLOC       DTS(59) /* zmemStaticAlloc */
#define DTS_PARSEFONT   DTS(60) /* zoverParseFont */
#define DTS_STRSETS     DTS(61) /* zstringRegister, zstringUnregister */

/* What the drawing code calls. */
#define DTG_TEXT                                                                                             \
    (DTS_STRING | DTS_TEXTWIDTH | DTS_ADDTEXT | DTS_REMOVE | DTS_SETCOLOR | DTS_DRAWTEXT | DTS_DRAWRECT)
/* What ovlSetScreen calls. */
#define DTG_SCREEN                                                                                           \
    (DTS_REMOVE | DTS_SHOW | DTS_LAYERFLAG | DTS_ACTION | DTS_GETMODE | DTS_GETSUB | DTS_TESTOPT | DTS_SOUND)
/* Menu input: the pad and the click sounds. */
#define DTG_INPUT (DTS_PRESSED | DTS_SOUND)

/*
 * Seeds the generator from the case seed, clears the log, randomises all overlay state (screens,
 * elements, handles, fonts, player data) and installs the stubs in `stubs`. Call first in setup().
 */
void dtOvlBegin(const DtCtx *ctx, uint64_t stubs);

/* Setup shorthands: seed and randomise, install `mask`; the create flavour adds a 0/1 argument. */
#define DTO_SETUP(name, mask)                                                                                \
    static void name(DtCtx *c)                                                                               \
    {                                                                                                        \
        dtOvlBegin(c, mask);                                                                                 \
    }
#define DTO_SETUP_CREATE(name, mask)                                                                         \
    static void name(DtCtx *c)                                                                               \
    {                                                                                                        \
        dtOvlBegin(c, mask);                                                                                 \
        DT_ARG_INT(c, 0, dt_u32() % 3 != 0);                                                                 \
    }

/* `size` random bytes / a random lower case string of up to maxLen characters, on the test heap. */
void *dtOvlRandomBytes(uint32_t size);
char *dtOvlRandomText(int maxLen);
/* Makes gameGetRecord always return a record (for callers that do not check). */
void dtOvlNoNullRecords(void);

/* Makes gameGetRecord return NULL for keys whose slot (key >> 16 & 15) is `slot`; keyBit as in the stub. */
void dtOvlNullRecord(uint32_t slot, uint32_t keyBit);
/* Releases every button of every pad. */
void dtOvlNoInput(void);

/* Clears the current screen pointer in one case of five (the overlay is closed). */
void dtOvlMaybeNoScreen(void);
/* Forces the game mode and sub mode the stubs report. */
void dtOvlSetMode(uint32_t mode, uint32_t subMode);
/* Forces the buttons a player's pad reports. */
void dtOvlSetPad(int player, uint32_t buttons);

#endif
