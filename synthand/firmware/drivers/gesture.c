/*
 * gesture.c — Gesture recognition engine using TCN inference.
 *
 * Manages the TCN sliding window, runs inference every 20 ms, and applies
 * event logic (debounce, refractory period, continuous gesture tracking)
 * to produce MIDI events.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include <stdint.h>
#include <string.h>
#include "board.h"
#include "drivers/gesture.h"
#include "drivers/tcn_model.h"
#include "drivers/signal.h"

/* -------------------------------------------------------------------------
 * Gesture detection state
 * Author: jayis1
 * ------------------------------------------------------------------------- */

/* Current gesture probability distribution */
static q15_t g_probs[GESTURE_CLASSES];

/* Detected gesture tracking */
static gesture_id_t g_current_gesture = GESTURE_COUNT;  /* no gesture */
static uint8_t g_current_finger = 255;
static int g_consecutive_count = 0;      /* consecutive windows above threshold */
static uint32_t g_last_event_time = 0;   /* for refractory period */
static uint32_t g_gesture_start_time = 0; /* for continuous gesture tracking */

/* Continuous gesture state */
static int g_continuous_active = 0;
static gesture_id_t g_continuous_gesture = GESTURE_COUNT;

/* Detection thresholds (configurable via mapping) */
static uint16_t g_emg_threshold = 0x4000;      /* Q15 ≈ 0.25 */
static uint16_t g_tap_accel_threshold = 200;    /* Q8 */
static uint8_t  g_vibrato_sensitivity = 5;

/* Per-finger note-on tracking (for note-off logic) */
static uint8_t g_finger_active[NUM_FINGERS] = {0};
static uint8_t g_finger_note[NUM_FINGERS] = {0};

/* Gesture name strings (for debugging / OSC) */
static const char *gesture_names[GESTURE_CLASSES] = {
    "tap", "press", "release", "pluck", "strum_down", "strum_up",
    "vibrato", "tremolo", "glide", "fist", "open", "snap"
};

/* -------------------------------------------------------------------------
 * Initialize gesture recognition
 * Author: jayis1
 * ------------------------------------------------------------------------- */
void gesture_init(void)
{
    tcn_model_init();
    gesture_reset_window();
    memset(g_probs, 0, sizeof(g_probs));
    memset(g_finger_active, 0, sizeof(g_finger_active));
    memset(g_finger_note, 0, sizeof(g_finger_note));
    g_current_gesture = GESTURE_COUNT;
    g_current_finger = 255;
    g_consecutive_count = 0;
    g_continuous_active = 0;
}

/* -------------------------------------------------------------------------
 * Set detection thresholds from mapping configuration
 * Author: jayis1
 * ------------------------------------------------------------------------- */
void gesture_set_thresholds(uint16_t emg_threshold,
                             uint16_t tap_accel_threshold,
                             uint8_t vibrato_sensitivity)
{
    g_emg_threshold = emg_threshold;
    g_tap_accel_threshold = tap_accel_threshold;
    g_vibrato_sensitivity = vibrato_sensitivity;
}

/* -------------------------------------------------------------------------
 * Reset the TCN sliding window
 * ------------------------------------------------------------------------- */
void gesture_reset_window(void)
{
    tcn_model_reset_window();
}

/* -------------------------------------------------------------------------
 * Get current gesture probability distribution
 * ------------------------------------------------------------------------- */
void gesture_get_probabilities(q15_t probs[GESTURE_CLASSES])
{
    memcpy(probs, g_probs, sizeof(q15_t) * GESTURE_CLASSES);
}

/* -------------------------------------------------------------------------
 * Determine which finger is most active for the current gesture
 * Uses per-finger EMG envelope and curl velocity to identify the dominant finger.
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static uint8_t identify_dominant_finger(const feature_vector_t *features)
{
    /* Find the finger with the highest combined EMG + accel activity */
    int32_t max_activity = 0;
    uint8_t best_finger = 0;

    for (int i = 0; i < NUM_FINGERS; i++) {
        /* Combined score: EMG envelope (channel maps to finger) + accel magnitude */
        int32_t emg = features->emg_envelope[i % NUM_EMG_CHANNELS];
        int32_t accel_mag = (int32_t)features->finger_velocity[i];
        int32_t score = emg + (accel_mag >> 1);

        if (score > max_activity) {
            max_activity = score;
            best_finger = (uint8_t)i;
        }
    }

    return best_finger;
}

/* -------------------------------------------------------------------------
 * Check if a gesture is discrete (one-shot event) or continuous
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static int is_discrete_gesture(gesture_id_t id)
{
    switch (id) {
    case GESTURE_TAP:
    case GESTURE_PLUCK:
    case GESTURE_STRUM_DOWN:
    case GESTURE_STRUM_UP:
    case GESTURE_FIST:
    case GESTURE_OPEN:
    case GESTURE_SNAP:
    case GESTURE_RELEASE:
        return 1;
    case GESTURE_PRESS:
    case GESTURE_VIBRATO:
    case GESTURE_TREMOLO:
    case GESTURE_GLIDE:
        return 0;
    default:
        return 1;
    }
}

/* -------------------------------------------------------------------------
 * Run gesture inference and event logic
 * Called every 20 ms (every 10th sample at 500 Hz).
 * Author: jayis1
 * ------------------------------------------------------------------------- */
void gesture_infer(const feature_vector_t *features,
                    gesture_result_t *gest)
{
    memset(gest, 0, sizeof(gesture_result_t));
    gest->event_type = GESTURE_EVENT_NONE;

    /* Push the new sample into the TCN window */
    tcn_model_push_sample(features);

    /* Run TCN inference */
    int16_t logits[TCN_FC1_OUT];
    int16_t regression[TCN_FC2_OUT];
    tcn_model_infer(logits, regression);

    /* Convert to probabilities */
    tcn_model_softmax(logits, g_probs);

    /* Get the most likely gesture */
    uint8_t best_gesture = tcn_model_argmax(g_probs);
    q15_t best_prob = g_probs[best_gesture];

    /* Identify which finger is driving this gesture */
    uint8_t finger = identify_dominant_finger(features);

    /* Process discrete gestures */
    if (is_discrete_gesture((gesture_id_t)best_gesture)) {
        /* Threshold: probability must exceed 0.75 (Q15 ≈ 0x5FFF) */
        if (best_prob > 0x5FFF) {
            g_consecutive_count++;
        } else {
            g_consecutive_count = 0;
        }

        /* Debounce: require 3 consecutive windows (60 ms) above threshold */
        if (g_consecutive_count >= 3) {
            uint32_t now = features->timestamp;

            /* Refractory period: 200 ms between same-type events */
            if ((now - g_last_event_time) > 200 ||
                g_current_gesture != (gesture_id_t)best_gesture) {

                g_current_gesture = (gesture_id_t)best_gesture;
                g_current_finger = finger;
                g_last_event_time = now;
                g_consecutive_count = 0;

                /* Emit the appropriate MIDI event */
                gest->gesture_id = (gesture_id_t)best_gesture;
                gest->finger = finger;
                gest->confidence = best_prob;
                gest->velocity = (uint8_t)regression[0];
                gest->pressure = (uint8_t)regression[1];

                /* Map gesture to MIDI event type */
                switch (best_gesture) {
                case GESTURE_TAP:
                case GESTURE_PLUCK:
                case GESTURE_STRUM_DOWN:
                case GESTURE_STRUM_UP:
                case GESTURE_SNAP:
                    gest->event_type = GESTURE_EVENT_NOTE_ON;
                    if (gest->velocity == 0) gest->velocity = 64;  /* default */
                    /* Track note for later note-off */
                    g_finger_active[finger] = 1;
                    g_finger_note[finger] = finger;  /* mapped in main.c */
                    break;

                case GESTURE_RELEASE:
                    gest->event_type = GESTURE_EVENT_NOTE_OFF;
                    /* Find the note for this finger */
                    if (g_finger_active[finger]) {
                        g_finger_active[finger] = 0;
                    }
                    break;

                case GESTURE_FIST:
                    /* Sustain pedal on (CC 64, value 127) */
                    gest->event_type = GESTURE_EVENT_CC;
                    gest->pressure = 127;
                    break;

                case GESTURE_OPEN:
                    /* Sustain pedal off (CC 64, value 0) */
                    gest->event_type = GESTURE_EVENT_CC;
                    gest->pressure = 0;
                    break;

                default:
                    gest->event_type = GESTURE_EVENT_NONE;
                    break;
                }
            }
        }
    } else {
        /* Continuous gestures (press, vibrato, tremolo, glide) */
        if (best_prob > 0x4000) {  /* 0.5 threshold to activate */
            if (!g_continuous_active) {
                /* Start continuous gesture */
                g_continuous_active = 1;
                g_continuous_gesture = (gesture_id_t)best_gesture;
                g_gesture_start_time = features->timestamp;

                /* For press: emit note on */
                if (best_gesture == GESTURE_PRESS) {
                    gest->event_type = GESTURE_EVENT_NOTE_ON;
                    gest->gesture_id = GESTURE_PRESS;
                    gest->finger = finger;
                    gest->velocity = (uint8_t)regression[0];
                    gest->pressure = (uint8_t)regression[1];
                    gest->confidence = best_prob;
                    g_finger_active[finger] = 1;
                }
            }

            /* Emit continuous CC updates while active */
            if (g_continuous_active) {
                gest->gesture_id = (gesture_id_t)best_gesture;
                gest->finger = finger;
                gest->confidence = best_prob;
                gest->pressure = (uint8_t)regression[1];

                switch (best_gesture) {
                case GESTURE_PRESS:
                    /* Channel pressure / aftertouch */
                    gest->event_type = GESTURE_EVENT_CHANNEL_PRESSURE;
                    break;

                case GESTURE_VIBRATO:
                    /* CC for vibrato depth */
                    gest->event_type = GESTURE_EVENT_CC;
                    gest->pressure = (uint8_t)regression[2];  /* vibrato depth */
                    break;

                case GESTURE_TREMOLO:
                    /* CC for tremolo rate */
                    gest->event_type = GESTURE_EVENT_CC;
                    gest->pressure = (uint8_t)regression[0];
                    break;

                case GESTURE_GLIDE:
                    /* Pitch bend from finger position */
                    gest->event_type = GESTURE_EVENT_PITCH_BEND;
                    /* Map curl to 14-bit pitch bend (0x2000 = center) */
                    int32_t pb = 0x2000 + (int32_t)(features->finger_curl[finger] -
                              0x4000) * 0x2000 / 0x4000;
                    if (pb < 0) pb = 0;
                    if (pb > 0x3FFF) pb = 0x3FFF;
                    gest->pitch_bend = (uint16_t)pb;
                    break;

                default:
                    gest->event_type = GESTURE_EVENT_NONE;
                    break;
                }
            }
        } else if (g_continuous_active && best_prob < 0x1800) {
            /* Below 0.15 threshold — end continuous gesture */
            g_continuous_active = 0;

            /* For press: emit note off */
            if (g_continuous_gesture == GESTURE_PRESS) {
                gest->event_type = GESTURE_EVENT_NOTE_OFF;
                gest->gesture_id = GESTURE_RELEASE;
                gest->finger = g_current_finger;
                if (g_finger_active[g_current_finger]) {
                    g_finger_active[g_current_finger] = 0;
                }
            }
            g_continuous_gesture = GESTURE_COUNT;
        }
    }

    /* Always update the pitch bend from wrist orientation when playing */
    /* Pitch bend from wrist tilt (Y-axis of wrist accel) */
    if (features->wrist_accel[1] > 0x1000 || features->wrist_accel[1] < -0x1000) {
        if (gest->event_type == GESTURE_EVENT_NONE) {
            int32_t pb = 0x2000 + (int32_t)features->wrist_accel[1] * 8;
            if (pb < 0) pb = 0;
            if (pb > 0x3FFF) pb = 0x3FFF;
            /* Only emit if changed significantly */
            static uint16_t last_pb = 0x2000;
            if ((uint16_t)pb > last_pb + 20 || (uint16_t)pb + 20 < last_pb) {
                gest->event_type = GESTURE_EVENT_PITCH_BEND;
                gest->pitch_bend = (uint16_t)pb;
                gest->gesture_id = GESTURE_GLIDE;
                last_pb = (uint16_t)pb;
            }
        }
    }
}

/*
 * Author: jayis1
 * End of gesture.c
 */