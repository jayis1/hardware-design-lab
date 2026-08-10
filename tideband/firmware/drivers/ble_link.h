/**
 * @file    ble_link.h
 * @brief   TideBand — BLE UART link protocol to nRF52840 module.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef TIDEBAND_BLE_LINK_H
#define TIDEBAND_BLE_LINK_H

#include <stdint.h>
#include "doppler.h"
#include "depth.h"
#include "attitude.h"

/* ---- Protocol opcodes ---- */
#define BLE_OP_PROFILE_DATA   0x01u
#define BLE_OP_DIVE_START     0x02u
#define BLE_OP_DIVE_END       0x03u
#define BLE_OP_STATUS_REQ     0x04u
#define BLE_OP_STATUS_RSP     0x05u
#define BLE_OP_CAL_SET        0x06u
#define BLE_OP_CAL_RSP        0x07u
#define BLE_OP_OTA_BEGIN      0x10u
#define BLE_OP_OTA_CHUNK      0x11u
#define BLE_OP_OTA_END        0x12u
#define BLE_OP_OTA_ACK        0x13u
#define BLE_OP_ERASE_DIVES    0x20u
#define BLE_OP_EXPORT_BEGIN   0x21u
#define BLE_OP_EXPORT_CHUNK   0x22u
#define BLE_OP_EXPORT_END     0x23u
#define BLE_OP_SET_RATE       0x30u
#define BLE_OP_SET_THRESHOLD  0x31u
#define BLE_OP_GET_INFO       0x32u
#define BLE_OP_INFO_RSP       0x33u

/* ---- Packet structure ---- */
typedef struct __attribute__((packed)) {
    uint8_t sync;
    uint8_t opcode;
    uint8_t length;
    uint8_t payload[BLE_MAX_PAYLOAD];
    uint8_t crc;
} ble_packet_t;

/* ---- Status payload ---- */
typedef struct __attribute__((packed)) {
    uint8_t  battery_pct;
    uint8_t  dive_active;
    uint16_t dive_count;
    float    current_depth_m;
    float    current_temp_c;
    float    current_speed_ms;
    float    current_heading_deg;
    uint8_t  quality;
    uint16_t free_dive_slots;
} ble_status_payload_t;

/* ---- Public API ---- */

/** Initialize BLE UART link to nRF52840. */
void ble_link_init(void);

/** Send a profile data packet (current measurement + depth). */
void ble_link_send_profile(const doppler_result_t *doppler,
                           const depth_data_t *depth,
                           const attitude_t *att,
                           uint32_t timestamp);

/** Send status response. */
void ble_link_send_status(uint8_t battery_pct, uint8_t dive_active,
                          uint16_t dive_count, float depth, float temp,
                          float speed, float heading, uint8_t quality);

/** Send dive start/end notification. */
void ble_link_send_dive_event(uint8_t start, uint32_t timestamp,
                               uint32_t dive_id);

/** Process incoming BLE packets (call in main loop). */
void ble_link_process(void);

/** Check if BLE is connected. */
uint8_t ble_link_is_connected(void);

/** Set callback for incoming commands. */
typedef void (*ble_cmd_callback_t)(uint8_t opcode, const uint8_t *payload,
                                    uint8_t len);
void ble_link_set_callback(ble_cmd_callback_t cb);

#endif /* TIDEBAND_BLE_LINK_H */