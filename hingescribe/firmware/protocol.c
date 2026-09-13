/*
 * HingeScribe binary protocol and persistent configuration
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */
#include "hingescribe.h"
#include <string.h>

#define MSG_STATUS 0x01u
#define MSG_HISTORY 0x02u
#define CMD_TARE 0x80u
#define CMD_CLEAR_HISTORY 0x81u
#define CMD_SET_LOAD_SCALE 0x82u
#define CMD_GET_HISTORY 0x83u

static uint8_t calibration_page[sizeof(hs_calibration_t)];
static bool calibration_written;

static void put_u16(uint8_t *out, uint16_t value)
{
    out[0] = (uint8_t)(value & 0xFFu);
    out[1] = (uint8_t)(value >> 8);
}

static void put_u32(uint8_t *out, uint32_t value)
{
    out[0] = (uint8_t)(value & 0xFFu);
    out[1] = (uint8_t)((value >> 8) & 0xFFu);
    out[2] = (uint8_t)((value >> 16) & 0xFFu);
    out[3] = (uint8_t)(value >> 24);
}

static uint16_t get_u16(const uint8_t *in)
{
    return (uint16_t)((uint16_t)in[0] | ((uint16_t)in[1] << 8));
}

static uint32_t get_u32(const uint8_t *in)
{
    return (uint32_t)in[0] | ((uint32_t)in[1] << 8) | ((uint32_t)in[2] << 16) | ((uint32_t)in[3] << 24);
}

static int16_t quantize(float value, float scale)
{
    float converted = value * scale;
    if (converted > 32767.0f) return 32767;
    if (converted < -32768.0f) return -32768;
    return (int16_t)converted;
}

uint32_t hs_crc32(const uint8_t *data, size_t length)
{
    uint32_t crc = 0xFFFFFFFFu;
    if (data == NULL) return 0u;
    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (unsigned bit = 0; bit < 8u; ++bit) {
            uint32_t mask = (uint32_t)-(int32_t)(crc & 1u);
            crc = (crc >> 1) ^ (0xEDB88320u & mask);
        }
    }
    return ~crc;
}

uint16_t hs_crc16(const uint8_t *data, size_t length)
{
    uint16_t crc = 0xFFFFu;
    if (data == NULL) return 0u;
    for (size_t i = 0; i < length; ++i) {
        crc ^= (uint16_t)data[i] << 8;
        for (unsigned bit = 0; bit < 8u; ++bit) crc = (crc & 0x8000u) ? (uint16_t)((crc << 1) ^ 0x1021u) : (uint16_t)(crc << 1);
    }
    return crc;
}

static size_t frame(uint8_t type, const uint8_t *payload, size_t payload_length, uint8_t *out, size_t capacity)
{
    size_t total = payload_length + 8u;
    if (out == NULL || payload == NULL || total > capacity || payload_length > 255u) return 0u;
    put_u16(out, HS_MAGIC);
    out[2] = HS_PROTOCOL_VERSION;
    out[3] = type;
    out[4] = (uint8_t)payload_length;
    memcpy(out + 5, payload, payload_length);
    uint16_t crc = hs_crc16(out, payload_length + 5u);
    put_u16(out + payload_length + 5u, crc);
    out[payload_length + 7u] = 0x0Au;
    return total;
}

size_t protocol_encode_status(const hs_sample_t *sample, const hs_event_t *event, uint8_t *out, size_t capacity)
{
    if (sample == NULL || event == NULL) return 0u;
    uint8_t payload[32] = {0};
    put_u32(payload, sample->timestamp_ms);
    put_u16(payload + 4, (uint16_t)quantize(sample->angle_deg, 100.0f));
    put_u16(payload + 6, (uint16_t)quantize(sample->angular_velocity_dps, 10.0f));
    put_u16(payload + 8, (uint16_t)quantize(sample->force_n, 100.0f));
    put_u16(payload + 10, (uint16_t)quantize(sample->temperature_c, 100.0f));
    put_u16(payload + 12, sample->battery_mv);
    put_u16(payload + 14, sample->harvested_mj);
    put_u32(payload + 16, event->sequence);
    put_u16(payload + 20, event->health_score);
    put_u16(payload + 22, event->flags);
    payload[24] = sample->magnet_valid ? 1u : 0u;
    payload[25] = sample->load_valid ? 1u : 0u;
    return frame(MSG_STATUS, payload, 26u, out, capacity);
}

size_t protocol_encode_history(const hs_history_t *history, unsigned index, uint8_t *out, size_t capacity)
{
    hs_event_t event;
    if (!history_get(history, index, &event)) return 0u;
    uint8_t payload[40] = {0};
    put_u32(payload, event.sequence);
    put_u32(payload + 4, event.started_ms);
    put_u32(payload + 8, event.duration_ms);
    put_u16(payload + 12, (uint16_t)quantize(event.peak_open_force_n, 100.0f));
    put_u16(payload + 14, (uint16_t)quantize(event.peak_close_force_n, 100.0f));
    put_u16(payload + 16, (uint16_t)quantize(event.max_velocity_dps, 10.0f));
    put_u16(payload + 18, (uint16_t)quantize(event.closing_time_s, 100.0f));
    put_u16(payload + 20, (uint16_t)quantize(event.final_angle_deg, 100.0f));
    put_u16(payload + 22, (uint16_t)quantize(event.estimated_sag_mm, 100.0f));
    put_u16(payload + 24, event.harvested_mj);
    put_u16(payload + 26, event.health_score);
    put_u16(payload + 28, event.flags);
    return frame(MSG_HISTORY, payload, 30u, out, capacity);
}

static bool packet_valid(const uint8_t *packet, size_t length)
{
    if (packet == NULL || length < 8u) return false;
    if (get_u16(packet) != HS_MAGIC || packet[2] != HS_PROTOCOL_VERSION) return false;
    size_t payload_length = packet[4];
    if (length != payload_length + 8u || packet[length - 1u] != 0x0Au) return false;
    return get_u16(packet + length - 3u) == hs_crc16(packet, length - 3u);
}

hs_result_t protocol_handle_command(const uint8_t *packet, size_t length, hs_calibration_t *calibration, hs_history_t *history)
{
    if (!packet_valid(packet, length) || calibration == NULL || history == NULL) return HS_ERR_CRC;
    const uint8_t *payload = packet + 5;
    switch (packet[3]) {
        case CMD_TARE:
            return sensor_tare(calibration, 32u);
        case CMD_CLEAR_HISTORY:
            history_init(history);
            return HS_OK;
        case CMD_SET_LOAD_SCALE:
            if (packet[4] != 4u) return HS_ERR_ARGUMENT;
            {
                uint32_t raw = get_u32(payload);
                float value;
                memcpy(&value, &raw, sizeof(value));
                if (value < 100.0f || value > 1000000.0f) return HS_ERR_RANGE;
                calibration->load_counts_per_n = value;
                return calibration_save(calibration) ? HS_OK : HS_ERR_STORAGE;
            }
        case CMD_GET_HISTORY:
            return packet[4] == 2u && get_u16(payload) < history->count ? HS_OK : HS_ERR_RANGE;
        default:
            return HS_ERR_ARGUMENT;
    }
}

bool calibration_load(hs_calibration_t *calibration)
{
    if (calibration == NULL || !calibration_written) return false;
    memcpy(calibration, calibration_page, sizeof(*calibration));
    uint32_t stored = calibration->crc32;
    calibration->crc32 = 0u;
    uint32_t calculated = hs_crc32((const uint8_t *)calibration, sizeof(*calibration));
    calibration->crc32 = stored;
    return stored == calculated;
}

bool calibration_save(hs_calibration_t *calibration)
{
    if (calibration == NULL) return false;
    calibration->crc32 = 0u;
    calibration->crc32 = hs_crc32((const uint8_t *)calibration, sizeof(*calibration));
    memcpy(calibration_page, calibration, sizeof(*calibration));
    calibration_written = true;
    return true;
}
