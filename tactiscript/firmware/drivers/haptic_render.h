/*
 * haptic_render.h — TactiScript haptic rendering engine header
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef TACTISCRIPT_HAPTIC_RENDER_H
#define TACTISCRIPT_HAPTIC_RENDER_H

#include <stdint.h>

/* Initialize the haptic rendering engine */
void haptic_render_init(void);

/* Get a navigation arrow pattern for a direction byte.
 *   0 = stop (blank), 1 = forward, 2 = right, 3 = backward, 4 = left,
 *   5 = forward-right, 6 = backward-right, 7 = backward-left, 8 = forward-left
 * Returns pointer to a static 4×6 pattern (24 bytes), or NULL for invalid.
 */
const uint8_t *haptic_render_nav_pattern(uint8_t direction);

/* Get the next music haptic frame (called at 20 Hz in music mode).
 * Returns pointer to a 24-byte frame, or NULL if song is done.
 */
const uint8_t *haptic_render_music_next(void);

/* Animate a texture frame in-place (applies a traveling wave effect).
 *   frame: 24-byte buffer to modify
 *   len: should be 24
 *   intensity: 0-100 scale factor
 */
void haptic_render_texture_animate(uint8_t *frame, uint16_t len,
                                    uint8_t intensity);

/* Set the current music tempo (BPM) for music mode */
void haptic_render_set_tempo(uint16_t bpm);

/* Load a music pattern sequence (simplified: a series of 24-byte frames) */
void haptic_render_load_music(const uint8_t *sequence, uint16_t frame_count);

#endif /* TACTISCRIPT_HAPTIC_RENDER_H */