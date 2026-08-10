/**
 * @file    haptic.h
 * @brief   TideBand — Haptic feedback driver for current direction encoding.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef TIDEBAND_HAPTIC_H
#define TIDEBAND_HAPTIC_H

#include <stdint.h>

/* ---- Haptic pattern types ---- */
typedef enum {
    HAPTIC_NONE = 0,
    HAPTIC_SHORT,       /* Single 50ms pulse */
    HAPTIC_LONG,        /* Single 200ms pulse */
    HAPTIC_DOUBLE,      /* Two 50ms pulses */
    HAPTIC_TRIPLE,      /* Three 50ms pulses */
    HAPTIC_CONTINUOUS,  /* Continuous vibration (for alarm) */
} haptic_pattern_t;

/* ---- Public API ---- */

/** Initialize haptic motor PWM (TIM2_CH1) and enable pin. */
void haptic_init(void);

/** Trigger a haptic pattern. Non-blocking; runs in background via timer. */
void haptic_trigger(haptic_pattern_t pattern);

/** Update haptic state — call from main loop (~100 Hz). */
void haptic_update(void);

/** Stop all haptic output immediately. */
void haptic_stop(void);

/** Set the current-speed threshold for auto-haptic feedback. */
void haptic_set_threshold(float speed_ms);

/** Set haptic enabled/disabled (user preference). */
void haptic_set_enabled(uint8_t enabled);

/** Check if a haptic pattern is currently active. */
uint8_t haptic_is_active(void);

#endif /* TIDEBAND_HAPTIC_H */