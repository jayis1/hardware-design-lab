/*
 * Canopy Sentinel BLE driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */
#include "ble.h"
#include "../registers.h"

#include <stdio.h>
#include <string.h>

static void append_bytes(cs_packet_t *packet, const void *data, size_t size) {
    if (packet->length + size > sizeof(packet->data)) {
        return;
    }
    memcpy(&packet->data[packet->length], data, size);
    packet->length += size;
}

void ble_init(const cs_device_state_t *device) {
    printf("[ble] advertising %s (%s)\n", device->name, device->serial);
}

cs_packet_t ble_build_status_packet(const cs_device_state_t *device, const cs_power_state_t *power) {
    cs_packet_t packet = { .length = 0 };
    uint8_t type = CS_PKT_DEVICE_STATUS;
    append_bytes(&packet, &type, sizeof(type));
    append_bytes(&packet, device->serial, sizeof(device->serial));
    append_bytes(&packet, &device->boot_count, sizeof(device->boot_count));
    append_bytes(&packet, &device->active_crop, sizeof(device->active_crop));
    append_bytes(&packet, &power->battery_percent, sizeof(power->battery_percent));
    append_bytes(&packet, &power->charging, sizeof(power->charging));
    return packet;
}

cs_packet_t ble_build_scan_packet(const cs_scan_result_t *result) {
    cs_packet_t packet = { .length = 0 };
    uint8_t type = CS_PKT_SCAN_SUMMARY;
    append_bytes(&packet, &type, sizeof(type));
    append_bytes(&packet, &result->session_id, sizeof(result->session_id));
    append_bytes(&packet, result->row_id, sizeof(result->row_id));
    append_bytes(&packet, &result->crop, sizeof(result->crop));
    append_bytes(&packet, &result->risk_score, sizeof(result->risk_score));
    append_bytes(&packet, &result->risk_level, sizeof(result->risk_level));
    append_bytes(&packet, &result->dew_margin_c, sizeof(result->dew_margin_c));
    append_bytes(&packet, &result->leaf.normalized_wetness, sizeof(result->leaf.normalized_wetness));
    append_bytes(&packet, &result->spore.fluorescence_index, sizeof(result->spore.fluorescence_index));
    append_bytes(&packet, &result->airflow.stagnation_score, sizeof(result->airflow.stagnation_score));
    return packet;
}

void ble_print_packet(const cs_packet_t *packet, const char *label) {
    printf("[ble] %s len=%zu :", label, packet->length);
    for (size_t i = 0; i < packet->length; ++i) {
        printf(" %02X", packet->data[i]);
    }
    printf("\n");
}
