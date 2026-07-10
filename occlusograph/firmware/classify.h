/*
 * classify.h — On-device bruxism / activity classifier for Occlusograph.
 *
 * Author: jayis1
 * Copyright © 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef OCCLUSOGRAPH_CLASSIFY_H
#define OCCLUSOGRAPH_CLASSIFY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "fusion.h"

/* Activity classes. */
typedef enum {
    CLASS_REST = 0,
    CLASS_LIGHT_CONTACT,
    CLASS_CHEWING,
    CLASS_CLENCHING,
    CLASS_BRUXISM_PHASIC,
    CLASS_BRUXISM_TONIC,
    CLASS_SWALLOWING,
    CLASS_COUNT
} activity_class_t;

/* Event record emitted when a class transition or sustained activity occurs. */
typedef struct {
    uint32_t timestamp;       /* frame counter at event onset */
    uint16_t duration_ms;      /* how long the activity lasted */
    uint8_t  class_id;         /* activity_class_t */
    uint8_t  peak_element;     /* element index with highest force */
    int16_t  peak_force_mN;    /* peak force on that element */
    int16_t  rms_force_mN;     /* RMS across all elements over the window */
    int16_t  jaw_open_deg;     /* jaw opening angle from IMU integration */
} event_record_t;

/* Initialize the classifier (loads weights into RAM). */
void classify_init(void);

/* Push one force frame into the classifier's sliding window.
 * If a complete window is ready, runs inference and, if the class differs
 * from the previous emission (or a force threshold is exceeded), emits an
 * event_record_t into *out. Returns 1 if an event was emitted, 0 otherwise.
 */
int  classify_push(const force_frame_t *frame, event_record_t *out);

/* Get the current (most-recent) classification. */
activity_class_t classify_current(void);

/* Reset window state (used after a gap in streaming). */
void classify_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* OCCLUSOGRAPH_CLASSIFY_H */