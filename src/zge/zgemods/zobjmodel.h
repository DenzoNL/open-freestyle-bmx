/* Model objects (C:\NgEngine\Zge\ZgeMods\zobjModel.c). Partly ported. */
#ifndef ZGE_ZGEMODS_ZOBJMODEL_H
#define ZGE_ZGEMODS_ZOBJMODEL_H

/* Starts the model object type (and the animation system); asserts when called twice. Returns 0 on failure.
 */
int zobjModelInitialize(void);
/* Releases the model handle (+0x2c) of an object, when it has one; returns 1. */
int zobjModelFree(void *object);

#endif
