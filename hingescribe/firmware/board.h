/*
 * HingeScribe board contract
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */
#ifndef HINGESCRIBE_BOARD_H
#define HINGESCRIBE_BOARD_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define HS_FW_VERSION "1.0.0"
#define HS_PROTOCOL_VERSION 1u
#define HS_SAMPLE_HZ 100u
#define HS_TRACE_SAMPLES 256u
#define HS_HISTORY_RECORDS 128u
#define HS_PACKET_MAX 128u
#define HS_MAGIC 0x4853u

/* nRF52840-QIAA pin assignment; all signals are 3.3 V. */
#define PIN_I2C_SDA 26u
#define PIN_I2C_SCL 27u
#define PIN_ANGLE_ALERT 6u
#define PIN_LOAD_DOUT 8u
#define PIN_LOAD_SCK 9u
#define PIN_HARVEST_SENSE 2u
#define PIN_BATTERY_SENSE 3u
#define PIN_STATUS_LED 13u
#define PIN_USER_BUTTON 11u
#define PIN_SWDIO 20u
#define PIN_SWCLK 18u

#define AS5600_ADDRESS 0x36u
#define TMP117_ADDRESS 0x48u
#define AS5600_RAW_ANGLE 0x0Eu
#define AS5600_STATUS 0x0Bu
#define TMP117_TEMP 0x00u
#define TMP117_CONFIG 0x01u

#define BATTERY_MIN_MV 2800u
#define BATTERY_MAX_MV 4200u
#define OPEN_THRESHOLD_DEG 8.0f
#define CLOSED_THRESHOLD_DEG 2.0f
#define STALL_FORCE_N 55.0f
#define SAG_WARN_MM 2.5f
#define CLOSER_WARN_SECONDS 8.0f
#define EVENT_DEBOUNCE_MS 100u
#define RADIO_IDLE_TIMEOUT_MS 30000u

typedef enum {
    HS_OK = 0,
    HS_ERR_ARGUMENT = -1,
    HS_ERR_TIMEOUT = -2,
    HS_ERR_IO = -3,
    HS_ERR_CRC = -4,
    HS_ERR_RANGE = -5,
    HS_ERR_STATE = -6,
    HS_ERR_STORAGE = -7
} hs_result_t;

typedef enum {
    DOOR_CLOSED,
    DOOR_OPENING,
    DOOR_OPEN,
    DOOR_CLOSING,
    DOOR_OBSTRUCTED,
    DOOR_UNKNOWN
} door_state_t;

typedef struct {
    uint32_t timestamp_ms;
    float angle_deg;
    float angular_velocity_dps;
    float force_n;
    float temperature_c;
    uint16_t battery_mv;
    uint16_t harvested_mj;
    bool magnet_valid;
    bool load_valid;
} hs_sample_t;

typedef struct {
    uint32_t sequence;
    uint32_t started_ms;
    uint32_t duration_ms;
    float peak_open_force_n;
    float peak_close_force_n;
    float max_velocity_dps;
    float closing_time_s;
    float final_angle_deg;
    float estimated_sag_mm;
    uint16_t harvested_mj;
    uint16_t health_score;
    uint16_t flags;
} hs_event_t;

#define HS_FLAG_STICKING       (1u << 0)
#define HS_FLAG_SAG            (1u << 1)
#define HS_FLAG_CLOSER_SLOW    (1u << 2)
#define HS_FLAG_SLAM           (1u << 3)
#define HS_FLAG_SENSOR_FAULT   (1u << 4)
#define HS_FLAG_LOW_BATTERY    (1u << 5)
#define HS_FLAG_OBSTRUCTED     (1u << 6)

typedef struct {
    float closed_zero_deg;
    float load_zero_counts;
    float load_counts_per_n;
    float sag_mm_per_deg;
    uint32_t event_count;
    uint32_t crc32;
} hs_calibration_t;

uint32_t platform_millis(void);
void platform_delay_ms(uint32_t milliseconds);
void platform_init(void);
void platform_sleep(void);
void platform_set_led(bool enabled);
hs_result_t platform_i2c_read(uint8_t address, uint8_t reg, uint8_t *data, size_t length);
hs_result_t platform_i2c_write(uint8_t address, uint8_t reg, const uint8_t *data, size_t length);
hs_result_t platform_loadcell_read(int32_t *counts, uint32_t timeout_ms);
uint16_t platform_adc_mv(uint8_t pin);
uint16_t platform_harvest_mj(void);
void platform_ble_notify(const uint8_t *data, size_t length);
int platform_ble_receive(uint8_t *data, size_t capacity);
bool platform_button_pressed(void);
void platform_sim_advance(void);

#endif
