/*
 * The opt-in smooth resampling mixer (smoothmix.c): native exe only.
 */
#ifndef BMX_PLATFORM_SMOOTHMIX_H
#define BMX_PLATFORM_SMOOTHMIX_H

#include <stdint.h>

#include "hook/hook.h"

struct WinchanChannel;

/*
 * Reads [audio] smooth_resample (config.c) and returns the jumps to install: one (the entry of our own
 * ported winchanMix to bmxSmoothMix) for 1 or 2, none (count 0) for 0 or when unset. Values above 2 count
 * as 2. The ported mixer is called directly by winchanPump, so the jump goes over the ported function's
 * entry (the loader makes it writable), not over the original's address.
 */
const BmxHook *bmxSmoothJumps(uint32_t *count);

/* winchanMix with the position's fraction used: mode 1 linear, 2 cubic (bmxSmoothMix: the configured one). */
void bmxSmoothMix(uint8_t *ring, uint32_t bytes);
void bmxSmoothMixMode(uint8_t *ring, uint32_t bytes, int mode);

/* The frame a neighbour index of the channel's position stands for (loop wrap, ping-pong mirror, clamp). */
int32_t bmxSmoothNeighbour(const struct WinchanChannel *ch, int32_t i);

#endif
