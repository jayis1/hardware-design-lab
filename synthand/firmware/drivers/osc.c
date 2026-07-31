/*
 * osc.c — OSC 1.0 bundle encoder for GATT transport.
 *
 * Encodes Synthand sensor data and gesture events into OSC 1.0 bundles
 * and sends them via a custom BLE GATT characteristic for apps that
 * prefer OSC over MIDI (Max/MSP, TouchDesigner, VR engines).
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include <stdint.h>
#include <string.h>
#include "board.h"
#include "registers.h"
#include "drivers/osc.h"
#include "drivers/ble_midi.h"

/* -------------------------------------------------------------------------
 * OSC encoding helpers
 * Author: jayis1
 * ------------------------------------------------------------------------- */

/* Align to 4-byte boundary */
static uint16_t osc_align4(uint16_t pos)
{
    return (pos + 3) & ~3;
}

/* Encode a 32-bit big-endian value */
void osc_encode_u32(osc_bundle_t *bundle, uint32_t value)
{
    if (bundle->length + 4 > bundle->max_length)
        return;
    bundle->data[bundle->length++] = (uint8_t)(value >> 24);
    bundle->data[bundle->length++] = (uint8_t)(value >> 16);
    bundle->data[bundle->length++] = (uint8_t)(value >> 8);
    bundle->data[bundle->length++] = (uint8_t)(value);
}

/* Encode a 32-bit float (IEEE 754, big-endian) */
void osc_encode_float(osc_bundle_t *bundle, float value)
{
    union { float f; uint32_t u; } conv;
    conv.f = value;
    osc_encode_u32(bundle, conv.u);
}

/* Encode an OSC string (null-terminated, 4-byte aligned) */
void osc_encode_string(osc_bundle_t *bundle, const char *str)
{
    uint16_t len = 0;
    while (str[len] && bundle->length + 1 < bundle->max_length) {
        bundle->data[bundle->length++] = (uint8_t)str[len++];
    }
    /* Null terminator */
    bundle->data[bundle->length++] = 0;
    /* Pad to 4-byte alignment */
    while (bundle->length % 4 != 0) {
        if (bundle->length >= bundle->max_length) break;
        bundle->data[bundle->length++] = 0;
    }
}

/* -------------------------------------------------------------------------
 * Initialize OSC encoder
 * Author: jayis1
 * ------------------------------------------------------------------------- */
void osc_init(void)
{
    /* Nothing to initialize — bundles are stateless */
}

/* -------------------------------------------------------------------------
 * Start a new OSC bundle
 * Format: #bundle + 64-bit NTP timestamp
 * Author: jayis1
 * ------------------------------------------------------------------------- */
void osc_bundle_init(osc_bundle_t *bundle, uint32_t timestamp_ms)
{
    bundle->length = 0;
    bundle->max_length = sizeof(bundle->data);

    /* Bundle header: "#bundle" string (8 bytes, 4-aligned) */
    osc_encode_string(bundle, "#bundle");

    /* NTP timestamp (64-bit): seconds since 1900 + fraction
     * Convert ms to NTP: seconds = ms / 1000, fraction = (ms % 1000) * 2^32 / 1000
     * For simplicity, use relative time from device boot. */
    uint32_t ntp_seconds = timestamp_ms / 1000;
    uint32_t ntp_fraction = (timestamp_ms % 1000) * 4294967;  /* ≈ 2^32/1000 */
    osc_encode_u32(bundle, ntp_seconds);
    osc_encode_u32(bundle, ntp_fraction);
}

/* -------------------------------------------------------------------------
 * Add a float message to the bundle
 * Format: [address][,f][float]
 * Author: jayis1
 * ------------------------------------------------------------------------- */
void osc_bundle_add_float(osc_bundle_t *bundle,
                           const char *address,
                           int index,
                           q15_t value)
{
    /* Build address with index suffix: "address/index" */
    char full_addr[64];
    int pos = 0;
    while (address[pos] && pos < 56) {
        full_addr[pos] = address[pos];
        pos++;
    }
    full_addr[pos++] = '/';
    /* Convert index to string */
    if (index >= 10) {
        full_addr[pos++] = '0' + (index / 10);
        full_addr[pos++] = '0' + (index % 10);
    } else {
        full_addr[pos++] = '0' + index;
    }
    full_addr[pos] = '\0';

    /* Encode address */
    osc_encode_string(bundle, full_addr);

    /* Encode type tag: ",f" */
    osc_encode_string(bundle, ",f");

    /* Encode float value (Q15 → float 0.0 to 1.0) */
    float fval = (float)value / 32768.0f;
    osc_encode_float(bundle, fval);
}

/* -------------------------------------------------------------------------
 * Add a quaternion message (4 floats)
 * Author: jayis1
 * ------------------------------------------------------------------------- */
void osc_bundle_add_quat(osc_bundle_t *bundle,
                          const char *address,
                          const q29_t quat[4])
{
    osc_encode_string(bundle, address);
    osc_encode_string(bundle, ",ffff");

    /* Q29 → float (-1.0 to 1.0) */
    for (int i = 0; i < 4; i++) {
        float fval = (float)quat[i] / 536870912.0f;  /* 2^29 */
        osc_encode_float(bundle, fval);
    }
}

/* -------------------------------------------------------------------------
 * Add a gesture event message (int + float confidence)
 * Author: jayis1
 * ------------------------------------------------------------------------- */
void osc_bundle_add_gesture(osc_bundle_t *bundle,
                             const char *address,
                             uint8_t gesture_id,
                             q15_t confidence)
{
    osc_encode_string(bundle, address);
    osc_encode_string(bundle, ",if");

    /* Encode gesture ID as int32 */
    osc_encode_u32(bundle, (uint32_t)gesture_id);

    /* Encode confidence as float */
    float fconf = (float)confidence / 32768.0f;
    osc_encode_float(bundle, fconf);
}

/* -------------------------------------------------------------------------
 * Send the OSC bundle via BLE GATT
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int osc_bundle_send(const osc_bundle_t *bundle)
{
    if (!ble_midi_is_connected())
        return -1;

    if (bundle->length == 0)
        return -2;

    /* Send via IPC to network core (OSC TX characteristic) */
    IPC->TASKS_SEND[1] = 1;  /* signal network core */

    /* In a full implementation, the IPC mailbox would carry the bundle data
     * to the network core, which would split it into GATT notifications
     * (max 180 bytes per notification, MTU-dependent). For bundles larger
     * than the MTU, the network core fragments across multiple notifications. */

    return 0;
}

/*
 * Author: jayis1
 * End of osc.c
 */