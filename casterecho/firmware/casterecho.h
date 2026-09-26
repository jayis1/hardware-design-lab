/*
 * CasterEcho firmware interfaces.
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */
#ifndef CASTERECHO_H
#define CASTERECHO_H
#include "board.h"

typedef struct {
    ce_sample_t samples[CE_WINDOW_SAMPLES];
    size_t count;
    size_t write_index;
    uint32_t started_ms;
    ce_motion_t motion;
    float last_speed_mps;
    uint32_t last_motion_ms;
} ce_capture_t;

typedef struct {
    ce_pass_t records[CE_HISTORY_RECORDS];
    uint16_t head;
    uint16_t count;
    uint32_t next_sequence;
} ce_history_t;

typedef enum {
    CE_CMD_INFO = 0x01,
    CE_CMD_LATEST = 0x02,
    CE_CMD_HISTORY = 0x03,
    CE_CMD_CALIBRATE = 0x04,
    CE_CMD_CLEAR = 0x05,
    CE_CMD_SET_WHEEL = 0x06,
    CE_CMD_LIVE = 0x07
} ce_command_t;

typedef enum {
    CE_MSG_INFO = 0x81,
    CE_MSG_PASS = 0x82,
    CE_MSG_HISTORY = 0x83,
    CE_MSG_ACK = 0x84,
    CE_MSG_ERROR = 0x85,
    CE_MSG_LIVE = 0x86
} ce_message_t;

typedef struct {
    uint8_t type;
    uint8_t version;
    uint16_t payload_length;
    uint32_t sequence;
    uint8_t payload[CE_PACKET_MAX - 12u];
    uint32_t crc32;
} ce_packet_t;

ce_result_t sensors_init(void);
ce_result_t sensors_read(ce_sample_t *sample);
void sensors_reset(void);
float sensors_speed_from_period(uint32_t period_us, float circumference_mm);
ce_motion_t sensors_classify_motion(const ce_sample_t *sample, const ce_sample_t *previous);
void capture_init(ce_capture_t *capture);
bool capture_push(ce_capture_t *capture, const ce_sample_t *sample);
ce_result_t analytics_compute(const ce_capture_t *capture, const ce_calibration_t *calibration, ce_features_t *features);
void analytics_update_baseline(ce_calibration_t *calibration, const ce_features_t *features);
uint16_t analytics_health_score(const ce_features_t *features, const ce_calibration_t *calibration);
uint16_t analytics_flags(const ce_features_t *features, const ce_calibration_t *calibration);
float analytics_band_energy(const ce_capture_t *capture, float low_hz, float high_hz);
void history_init(ce_history_t *history);
ce_result_t history_append(ce_history_t *history, ce_pass_t *pass);
const ce_pass_t *history_latest(const ce_history_t *history);
const ce_pass_t *history_at(const ce_history_t *history, uint16_t age);
uint32_t ce_crc32(const void *data, size_t length);
size_t protocol_encode(const ce_packet_t *packet, uint8_t *output, size_t capacity);
ce_result_t protocol_decode(const uint8_t *input, size_t length, ce_packet_t *packet);
ce_result_t protocol_handle(const ce_packet_t *request, ce_packet_t *response, ce_history_t *history, ce_calibration_t *calibration);
void protocol_make_live(const ce_sample_t *sample, ce_packet_t *packet);
#endif
