/* Author: jayis1 */
#ifndef CANOPY_SENTINEL_BLE_H
#define CANOPY_SENTINEL_BLE_H

#include "../board.h"
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t data[CS_PACKET_MAX];
    size_t length;
} cs_packet_t;

void ble_init(const cs_device_state_t *device);
cs_packet_t ble_build_status_packet(const cs_device_state_t *device, const cs_power_state_t *power);
cs_packet_t ble_build_scan_packet(const cs_scan_result_t *result);
void ble_print_packet(const cs_packet_t *packet, const char *label);

#endif
