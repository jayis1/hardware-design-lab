/*
 * haptic_render.c — TactiScript haptic rendering engine
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * Synthesizes tactile patterns for navigation arrows, music rhythm,
 * and animated textures. These patterns are rendered on the 4×6
 * actuator array.
 *
 * Pattern format: 24 bytes (4 rows × 6 cols), values 0-255 representing
 * per-element drive intensity.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "haptic_render.h"
#include "../board.h"

/* ----------------------------------------------------------------
 * Navigation arrow patterns
 *   4×6 array (row-major). Active elements = 255, inactive = 0.
 *
 * Direction encoding:
 *   0 = stop, 1 = N (forward), 2 = E (right), 3 = S (back),
 *   4 = W (left), 5 = NE, 6 = SE, 7 = SW, 8 = NW
 * ---------------------------------------------------------------- */

/* Forward arrow (N): triangle pointing up */
static const uint8_t NAV_FORWARD[ACT_COUNT] = {
    /* row 0 (top)    */   0,   0,   0, 255,   0,   0,
    /* row 1         */   0,   0, 255, 255, 255,   0,
    /* row 2         */   0, 255, 255, 255, 255, 255,
    /* row 3 (bottom) */   0,   0,   0, 255,   0,   0,
};

/* Right arrow (E): triangle pointing right */
static const uint8_t NAV_RIGHT[ACT_COUNT] = {
    0,   0,   0,   0, 255,   0,
    0,   0,   0, 255, 255,   0,
    0,   0, 255, 255, 255,   0,
    0,   0,   0, 255, 255,   0,
};

/* Backward arrow (S): triangle pointing down */
static const uint8_t NAV_BACKWARD[ACT_COUNT] = {
    0,   0,   0, 255,   0,   0,
    0, 255, 255, 255, 255, 255,
    0,   0, 255, 255, 255,   0,
    0,   0,   0, 255,   0,   0,
};

/* Left arrow (W): triangle pointing left */
static const uint8_t NAV_LEFT[ACT_COUNT] = {
    0, 255,   0,   0,   0,   0,
    0, 255, 255,   0,   0,   0,
    0, 255, 255, 255,   0,   0,
    0, 255, 255,   0,   0,   0,
};

/* Stop: X pattern */
static const uint8_t NAV_STOP[ACT_COUNT] = {
    255,   0,   0,   0,   0, 255,
      0, 255,   0,   0, 255,   0,
      0,   0,   0,   0,   0,   0,
      0, 255,   0,   0, 255,   0,
};

/* Diagonal: NE (forward-right) */
static const uint8_t NAV_NE[ACT_COUNT] = {
    0,   0,   0,   0, 255,   0,
    0,   0,   0, 255,   0,   0,
    0,   0, 255,   0,   0,   0,
    0, 255,   0,   0,   0,   0,
};

/* Diagonal: SE (backward-right) */
static const uint8_t NAV_SE[ACT_COUNT] = {
    0, 255,   0,   0,   0,   0,
    0,   0, 255,   0,   0,   0,
    0,   0,   0, 255,   0,   0,
    0,   0,   0,   0, 255,   0,
};

/* Diagonal: SW (backward-left) */
static const uint8_t NAV_SW[ACT_COUNT] = {
    0,   0,   0,   0, 255,   0,
    0,   0,   0, 255,   0,   0,
    0,   0, 255,   0,   0,   0,
    0, 255,   0,   0,   0,   0,
};

/* Diagonal: NW (forward-left) */
static const uint8_t NAV_NW[ACT_COUNT] = {
    255,   0,   0,   0,   0,   0,
      0, 255,   0,   0,   0,   0,
      0,   0, 255,   0,   0,   0,
      0,   0,   0, 255,   0,   0,
};

static const uint8_t *nav_patterns[9] = {
    NAV_STOP, NAV_FORWARD, NAV_RIGHT, NAV_BACKWARD, NAV_LEFT,
    NAV_NE, NAV_SE, NAV_SW, NAV_NW
};

/* ----------------------------------------------------------------
 * Music haptic synthesis state
 * ---------------------------------------------------------------- */
static uint8_t s_music_frame[ACT_COUNT];
static uint16_t s_music_tempo_bpm = 120;
static uint16_t s_music_beat_counter = 0;
static uint16_t s_music_subbeat = 0;

/* Music pattern: predefined rhythm patterns for demo.
 * Each beat generates a different tactile "note" pattern. */
static const uint8_t MUSIC_BEAT_STRONG[ACT_COUNT] = {
    255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255,
};

static const uint8_t MUSIC_BEAT_WEAK[ACT_COUNT] = {
      0,   0, 128, 128,   0,   0,
      0, 128, 128, 128, 128,   0,
    128, 128, 128, 128, 128, 128,
      0, 128, 128, 128, 128,   0,
};

static const uint8_t MUSIC_BEAT_OFF[ACT_COUNT] = {
    0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0,
};

/* Melody contour patterns: simulate pitch by activating different rows.
 * Higher pitch = top rows, lower pitch = bottom rows. */
static const uint8_t MUSIC_PITCH_HIGH[ACT_COUNT] = {
    200, 200, 200, 200, 200, 200,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
};

static const uint8_t MUSIC_PITCH_MID[ACT_COUNT] = {
      0,   0,   0,   0,   0,   0,
    200, 200, 200, 200, 200, 200,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
};

static const uint8_t MUSIC_PITCH_LOW[ACT_COUNT] = {
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
    200, 200, 200, 200, 200, 200,
};

/* ----------------------------------------------------------------
 * Texture animation state
 * ---------------------------------------------------------------- */
static uint8_t s_texture_phase = 0;

/* ----------------------------------------------------------------
 * Initialize
 * ---------------------------------------------------------------- */
void haptic_render_init(void)
{
    memset(s_music_frame, 0, sizeof(s_music_frame));
    s_music_tempo_bpm = 120;
    s_music_beat_counter = 0;
    s_music_subbeat = 0;
    s_texture_phase = 0;
}

/* ----------------------------------------------------------------
 * Get navigation pattern
 * ---------------------------------------------------------------- */
const uint8_t *haptic_render_nav_pattern(uint8_t direction)
{
    if (direction > 8)
        return NAV_STOP;
    return nav_patterns[direction];
}

/* ----------------------------------------------------------------
 * Music frame generation
 *
 * Synthesizes a rhythmic haptic pattern based on tempo and a simple
 * melodic contour. Each call returns the next frame at ~20 Hz.
 *
 * Beat structure (4/4 time):
 *   Beat 1 (strong): full array pulse + high pitch
 *   Beat 2 (weak):   mid-row pulse + mid pitch
 *   Beat 3 (strong): full array pulse + low pitch
 *   Beat 4 (weak):   mid-row pulse + high pitch
 * ---------------------------------------------------------------- */
const uint8_t *haptic_render_music_next(void)
{
    /* Calculate subbeats per frame based on tempo.
     * At 20 Hz frame rate and 120 BPM:
     *   120 BPM = 2 beats/sec = 0.5 sec/beat
     *   At 20 Hz, 0.5 sec = 10 frames per beat
     *   So each beat lasts 10 frames.
     */
    uint16_t frames_per_beat = (20 * 60) / s_music_tempo_bpm; /* e.g. 10 @ 120 BPM */
    if (frames_per_beat < 2)
        frames_per_beat = 2;

    /* Beat pattern: strong-weak-strong-weak over 4 beats */
    uint8_t beat_in_measure = s_music_beat_counter % 4;
    bool is_strong = (beat_in_measure == 0 || beat_in_measure == 2);

    /* Envelope: attack-decay over the beat duration.
     * First 2 frames = attack (full intensity), then decay. */
    uint16_t frame_in_beat = s_music_subbeat;
    uint8_t envelope;
    if (frame_in_beat < 2)
        envelope = 255; /* attack */
    else if (frame_in_beat < frames_per_beat / 2)
        envelope = 180; /* sustain */
    else
        envelope = 80; /* decay */

    /* Combine rhythm (beat) + melody (pitch contour) */
    const uint8_t *pitch_pattern;
    switch (beat_in_measure) {
    case 0: pitch_pattern = MUSIC_PITCH_HIGH; break;
    case 1: pitch_pattern = MUSIC_PITCH_MID;   break;
    case 2: pitch_pattern = MUSIC_PITCH_LOW;   break;
    case 3: pitch_pattern = MUSIC_PITCH_HIGH;   break;
    default: pitch_pattern = MUSIC_PITCH_MID;   break;
    }

    const uint8_t *beat_pattern = is_strong ? MUSIC_BEAT_STRONG : MUSIC_BEAT_WEAK;

    /* Mix beat and pitch patterns with envelope */
    for (int i = 0; i < ACT_COUNT; i++) {
        uint16_t mix = ((uint16_t)beat_pattern[i] + pitch_pattern[i]) / 2;
        s_music_frame[i] = (uint8_t)((mix * envelope) / 255);
    }

    /* Advance subbeat */
    s_music_subbeat++;
    if (s_music_subbeat >= frames_per_beat) {
        s_music_subbeat = 0;
        s_music_beat_counter++;
    }

    return s_music_frame;
}

/* ----------------------------------------------------------------
 * Texture animation: traveling wave effect
 *
 * Creates a wave that travels across the 4×6 array, giving a
 * "flowing" texture sensation. The phase increments each call.
 * ---------------------------------------------------------------- */
void haptic_render_texture_animate(uint8_t *frame, uint16_t len,
                                    uint8_t intensity)
{
    if (len < ACT_COUNT)
        return;

    /* For each element, compute intensity based on a sine wave
     * with phase offset based on column position. */
    for (int row = 0; row < ACT_ROWS; row++) {
        for (int col = 0; col < ACT_COLS; col++) {
            /* Simple sine approximation using a lookup table */
            uint8_t phase = (s_texture_phase + col * 30 + row * 15) & 0xFF;
            /* 8-bit sine: approximate with triangle wave for speed */
            uint8_t tri;
            if (phase < 128)
                tri = phase * 2; /* 0-255 ramp up */
            else
                tri = (255 - phase) * 2; /* ramp down */

            /* Scale by user intensity */
            frame[row * ACT_COLS + col] = (uint8_t)(
                ((uint16_t)tri * intensity) / 100);
        }
    }

    s_texture_phase += 20; /* advance phase for next frame */
}

/* ----------------------------------------------------------------
 * Set tempo
 * ---------------------------------------------------------------- */
void haptic_render_set_tempo(uint16_t bpm)
{
    if (bpm < 40) bpm = 40;
    if (bpm > 240) bpm = 240;
    s_music_tempo_bpm = bpm;
}

/* ----------------------------------------------------------------
 * Load custom music sequence (for future use)
 * ---------------------------------------------------------------- */
void haptic_render_load_music(const uint8_t *sequence, uint16_t frame_count)
{
    /* In a full implementation, this would load a sequence of frames
     * from flash and play them in order. For now, we use the
     * synthesized rhythm generator above. */
    (void)sequence;
    (void)frame_count;
}

/* ----------------------------------------------------------------
 * End of haptic_render.c
 * Author: jayis1
 * ---------------------------------------------------------------- */