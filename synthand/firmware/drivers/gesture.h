/*
 * gesture.h — Gesture recognition via temporal convolutional network.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef SYNTHAND_GESTURE_H
#define SYNTHAND_GESTURE_H

#include <stdint.h>
#include "board.h"
#include "drivers/signal.h"

/* Gesture class IDs */
typedef enum {
    GESTURE_TAP = 0,        /* per-finger drum tap */
    GESTURE_PRESS,          /* sustained finger press */
    GESTURE_RELEASE,        /* finger release (note off) */
    GESTURE_PLUCK,          /* pinch-release (string pluck) */
    GESTURE_STRUM_DOWN,     /* down strum across fingers */
    GESTURE_STRUM_UP,       /* up strum */
    GESTURE_VIBRATO,        /* 4-8 Hz oscillation */
    GESTURE_TREMOLO,        /* rapid alternating taps */
    GESTURE_GLIDE,          /* slow pitch slide */
    GESTURE_FIST,           /* all fingers curl (sustain on) */
    GESTURE_OPEN,           /* all fingers extend (sustain off) */
    GESTURE_SNAP,           /* thumb-finger release snap */
    GESTURE_COUNT
} gesture_id_t;

/* MIDI event types emitted by gesture recognition */
typedef enum {
    GESTURE_EVENT_NONE = 0,
    GESTURE_EVENT_NOTE_ON,
    GESTURE_EVENT_NOTE_OFF,
    GESTURE_EVENT_CC,
    GESTURE_EVENT_PITCH_BEND,
    GESTURE_EVENT_PROGRAM_CHANGE,
    GESTURE_EVENT_CHANNEL_PRESSURE,
} gesture_event_type_t;

/* Gesture result — output of the TCN classifier + event logic */
typedef struct {
    gesture_id_t gesture_id;        /* classified gesture */
    uint8_t finger;                 /* which finger (0-4), 255 = all */
    uint8_t velocity;               /* MIDI velocity 1-127 */
    uint8_t pressure;               /* MIDI pressure/CC value 0-127 */
    uint16_t pitch_bend;            /* 14-bit pitch bend value */
    uint8_t program;                /* program change number */
    q15_t confidence;               /* classification confidence (Q15) */
    gesture_event_type_t event_type;/* what MIDI event to emit */
} gesture_result_t;

/* Initialize gesture recognition (load TCN model into RAM) */
void gesture_init(void);

/* Run TCN inference on the current feature vector.
 * Maintains a sliding window of TCN_WINDOW samples.
 * Called every 20 ms (every 10th sample at 500 Hz).
 * Sets gest->event_type to NONE if no event this cycle. */
void gesture_infer(const feature_vector_t *features,
                   gesture_result_t *gest);

/* Get the current gesture probability distribution.
 * Fills a 12-element array of Q15 probabilities (sums to ~0x7FFF). */
void gesture_get_probabilities(q15_t probs[GESTURE_CLASSES]);

/* Set gesture detection thresholds (from mapping configuration).
 * Called when the app updates sensitivity settings. */
void gesture_set_thresholds(uint16_t emg_threshold,
                             uint16_t tap_accel_threshold,
                             uint8_t vibrato_sensitivity);

/* Reset the TCN sliding window (used when transitioning states) */
void gesture_reset_window(void);

#endif /* SYNTHAND_GESTURE_H */