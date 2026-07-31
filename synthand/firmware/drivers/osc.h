/*
 * osc.h — OSC 1.0 bundle encoder for GATT transport.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef SYNTHAND_OSC_H
#define SYNTHAND_OSC_H

#include <stdint.h>
#include "board.h"
#include "drivers/signal.h"
#include "drivers/gesture.h"

/* OSC bundle structure — encoded and sent via GATT characteristic */
typedef struct {
    uint8_t  data[180];     /* encoded OSC bundle bytes */
    uint16_t length;        /* current byte length */
    uint16_t max_length;    /* maximum capacity */
} osc_bundle_t;

/* Initialize OSC encoder state */
void osc_init(void);

/* Start a new OSC bundle with the given timestamp.
 * timestamp_ms: wall-clock time from the device. */
void osc_bundle_init(osc_bundle_t *bundle, uint32_t timestamp_ms);

/* Add a float argument message to the bundle.
 * address: OSC address pattern (e.g., "/synthand/emg")
 * index: appended to the address as a suffix (e.g., "/synthand/emg/0")
 * value: Q15 float value (0.0 to 1.0, scaled from Q15) */
void osc_bundle_add_float(osc_bundle_t *bundle,
                           const char *address,
                           int index,
                           q15_t value);

/* Add a quaternion (4-float) message to the bundle.
 * address: OSC address (e.g., "/synthand/wrist/quaternion")
 * quat: 4-element Q29 quaternion (w, x, y, z) */
void osc_bundle_add_quat(osc_bundle_t *bundle,
                          const char *address,
                          const q29_t quat[4]);

/* Add a gesture event message (int + float confidence).
 * address: OSC address (e.g., "/synthand/gesture")
 * gesture_id: gesture class ID
 * confidence: Q15 confidence value */
void osc_bundle_add_gesture(osc_bundle_t *bundle,
                             const char *address,
                             uint8_t gesture_id,
                             q15_t confidence);

/* Send the encoded bundle via BLE GATT (OSC TX characteristic).
 * Returns 0 on success, nonzero on error. */
int osc_bundle_send(const osc_bundle_t *bundle);

/* Helper: encode a 32-bit big-endian value into the bundle */
void osc_encode_u32(osc_bundle_t *bundle, uint32_t value);

/* Helper: encode a 32-bit float into the bundle */
void osc_encode_float(osc_bundle_t *bundle, float value);

/* Helper: encode an OSC string (4-byte aligned, null-padded) */
void osc_encode_string(osc_bundle_t *bundle, const char *str);

#endif /* SYNTHAND_OSC_H */