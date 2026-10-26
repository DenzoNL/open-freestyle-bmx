/*
 * The NgIF application shell (C:\zaxis\Bmx\src\com\zge\NgIF\NgApp.c): start-up and shutdown of the NextGen
 * interface, the per frame menu / action switch, the legal screens and the loading screen, and the lighting
 * set up when a level is entered, and two renderer callbacks: the debug overlay and the loading screen.
 */
#ifndef BMX_COM_ZGE_NGIF_NGAPP_H
#define BMX_COM_ZGE_NGIF_NGAPP_H

#include <stdint.h>

#include "compat/orig.h"
#include "zge/zmath.h"

struct MovieDesc;
struct ZbspLevelFace;

/* Transforms a world position into the listener's space (camera of the current frame). */
void zsystemToListener(const ZVec3 *world, ZVec3 *local);
/* Two empty application callbacks that report failure (0). */
int ngappReturnZeroA(void);
int ngappReturnZeroB(void);
/* Plays the movie of the first descriptor (its name with the extension replaced by AVI) between waits. */
void moviePlay(struct MovieDesc *const *list);

/* Registers / unregisters the per frame callback of the application (type 2 update function). */
void ngappRegisterTick(void);
void ngappUnregisterTick(void);
/* Prints the build banner. */
void ngappPrintVersion(void);

/* Level setup: lights of the props, the directional lights, the spot lights and the tagged faces. */
void ngappAttachLightObjects(void);
void ngappResetPropColors(void);
void ngappInitLights(void);
void ngappInitSpotLights(void);
/* The face whose tag (the first word behind one of its three tag pointers) equals id, or NULL. */
struct ZbspLevelFace *ngappFindFace(struct ZbspLevelFace *faces, uint32_t id);
/* Links every face tag to the first face with the same tag id. */
void ngappLinkFaces(void);

/* The interface art asset (loaded as a RAM disk file the GUI reads from). */
void ngappLoadInterfaceArt(void);
void ngappFreeInterfaceArt(void);

int ngifInitialize(void);
int ngappShutdown(void);
/* Shows the LOADING picture for two frames. */
void ngappShowLoading(void);
/* Draws a full screen quad with the sprite texture. Returns 1. */
int ngappDrawSprite(void *sprite);
/* Per frame function of the shell: legal screens first, then the menu and the action loop. */
int ngifRunFrame(float dt);
/* Renderer callback: the collision debug overlay (faces, edges, roller edges, prop wireframe). Returns 1. */
int ngappDrawDebug(void *unused);
/* Renderer callback: the loading picture `sprite` and the progress bar. Returns 1. */
int ngappDrawLoading(void *sprite);
/* Every third frame: tints the main light with the colour of the ground below the rider. */
void ngappUpdateSun(void);

#endif
