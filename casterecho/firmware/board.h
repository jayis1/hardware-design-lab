/*
 * CasterEcho board contract and domain model.
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */
#ifndef CASTERECHO_BOARD_H
#define CASTERECHO_BOARD_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CE_FW_VERSION "1.0.0"
#define CE_PROTOCOL_VERSION 1u
#define CE_SAMPLE_HZ 800u
#define CE_WINDOW_SAMPLES 256u
#define CE_BANDS 8u
#define CE_HISTORY_RECORDS 192u
#define CE_PACKET_MAX 160u
#define CE_MAGIC 0x4345u
#define CE_WHEEL_DIAMETER_MM 125.0f
#define CE_MIN_SPEED_MPS 0.08f
#define CE_MAX_SPEED_MPS 3.0f
#define CE_FLAT_WARN_G 1.8f
#define CE_BEARING_WARN_DB 12.0f
#define CE_DRAG_WARN_INDEX 0.62f
#define CE_TEMP_WARN_C 70.0f
#define CE_EVENT_GAP_MS 1500u
#define CE_RADIO_IDLE_MS 30000u
#define CE_BASELINE_MIN_PASSES 12u

/* nRF52840-QIAA, 3.3 V logic. */
#define PIN_I2C_SDA 26u
#define PIN_I2C_SCL 27u
#define PIN_IMU_INT 6u
#define PIN_HALL_A 8u
#define PIN_HALL_B 9u
#define PIN_PIEZO_ADC 2u
#define PIN_TEMP_ADC 3u
#define PIN_GENERATOR_ADC 4u
#define PIN_STATUS_LED 13u
#define PIN_USER_BUTTON 11u
#define PIN_SWDIO 20u
#define PIN_SWCLK 18u
#define LIS3DH_ADDRESS 0x18u
#define LIS3DH_WHO_AM_I 0x0Fu
#define LIS3DH_CTRL1 0x20u
#define LIS3DH_CTRL4 0x23u
#define LIS3DH_OUT_X_L 0x28u
#define FRAM_ADDRESS 0x50u

#define CE_FLAG_FLAT_SPOT       (1u << 0)
#define CE_FLAG_BEARING_ROUGH   (1u << 1)
#define CE_FLAG_DRAG            (1u << 2)
#define CE_FLAG_SWIVEL_CHATTER  (1u << 3)
#define CE_FLAG_OVERHEAT        (1u << 4)
#define CE_FLAG_SENSOR_FAULT    (1u << 5)
#define CE_FLAG_LOW_ENERGY      (1u << 6)
#define CE_FLAG_UNCALIBRATED    (1u << 7)

typedef enum {
    CE_OK = 0,
    CE_ERR_ARGUMENT = -1,
    CE_ERR_TIMEOUT = -2,
    CE_ERR_IO = -3,
    CE_ERR_CRC = -4,
    CE_ERR_RANGE = -5,
    CE_ERR_STATE = -6,
    CE_ERR_STORAGE = -7
} ce_result_t;

typedef enum {
    MOTION_STOPPED,
    MOTION_STARTING,
    MOTION_ROLLING,
    MOTION_COASTING,
    MOTION_SHOCK,
    MOTION_UNKNOWN
} ce_motion_t;

typedef struct {
    uint32_t timestamp_ms;
    float accel_x_g;
    float accel_y_g;
    float accel_z_g;
    float piezo_g;
    float wheel_rpm;
    float speed_mps;
    float generator_mv;
    float bearing_temp_c;
    int8_t hall_direction;
    bool imu_valid;
    bool hall_valid;
} ce_sample_t;

typedef struct {
    float vibration_rms_g;
    float crest_factor;
    float wheel_order_energy;
    float high_band_energy;
    float coast_decay_s;
    float drag_index;
    float swivel_chatter;
    float peak_shock_g;
    float mean_speed_mps;
    float temperature_c;
    uint16_t revolutions;
    uint16_t flags;
    uint16_t health_score;
} ce_features_t;

typedef struct {
    uint32_t sequence;
    uint32_t started_ms;
    uint32_t duration_ms;
    ce_features_t features;
    uint16_t harvested_mj;
    uint16_t samples;
    uint32_t crc32;
} ce_pass_t;

typedef struct {
    float baseline_rms_g;
    float baseline_wheel_order;
    float baseline_high_band;
    float baseline_drag;
    float wheel_circumference_mm;
    uint32_t accepted_passes;
    uint32_t lifetime_revolutions;
    uint32_t crc32;
} ce_calibration_t;

uint32_t platform_millis(void);
void platform_delay_ms(uint32_t milliseconds);
void platform_init(void);
void platform_sleep(void);
void platform_set_led(bool enabled);
ce_result_t platform_i2c_read(uint8_t address, uint8_t reg, uint8_t *data, size_t length);
ce_result_t platform_i2c_write(uint8_t address, uint8_t reg, const uint8_t *data, size_t length);
uint16_t platform_adc_mv(uint8_t pin);
bool platform_hall_edges(uint32_t *period_us, int8_t *direction);
uint16_t platform_harvested_mj(void);
ce_result_t platform_store(uint16_t slot, const void *data, size_t length);
ce_result_t platform_load(uint16_t slot, void *data, size_t length);
void platform_ble_notify(const uint8_t *data, size_t length);
int platform_ble_receive(uint8_t *data, size_t capacity);
bool platform_button_pressed(void);
void platform_sim_advance(void);

#endif
