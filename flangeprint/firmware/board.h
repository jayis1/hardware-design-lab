/* FlangePrint board contract and public model
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */
#ifndef FLANGEPRINT_BOARD_H
#define FLANGEPRINT_BOARD_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FP_FW_VERSION "1.0.0"
#define FP_PROTOCOL_VERSION 1u
#define FP_SECTORS 8u
#define FP_MIN_FIT_CHANNELS 6u
#define FP_ACOUSTIC_BANDS 3u
#define FP_PACKET_MAX 160u
#define FP_HISTORY_RECORDS 64u
#define FP_MAGIC 0x4650u
#define FP_GEOMETRY_PERIOD_MS 160u
#define FP_JOB_TIMEOUT_MS 1800000u
#define FP_BATTERY_EXCITE_MIN_MV 3450u
#define FP_MAX_BOARD_TEMP_C 72.0f
#define FP_GAP_LIMIT_MM 12.0f
#define FP_RESIDUAL_WARN_MM 0.40f
#define FP_TILT_WARN_MM 0.70f
#define FP_LEAK_WARN_SCORE 65u

#define PIN_I2C_SDA 26u
#define PIN_I2C_SCL 27u
#define PIN_SPI_SCK 14u
#define PIN_SPI_MOSI 15u
#define PIN_SPI_MISO 16u
#define PIN_ADC_CS 17u
#define PIN_FLASH_CS 19u
#define PIN_MUX_A0 3u
#define PIN_MUX_A1 4u
#define PIN_MUX_A2 5u
#define PIN_CHIRP_PWM 6u
#define PIN_CHIRP_ENABLE 7u
#define PIN_SENSOR_POWER 8u
#define PIN_LED_RED 9u
#define PIN_LED_GREEN 10u
#define PIN_LED_BLUE 11u
#define PIN_BUTTON 12u
#define PIN_BATTERY_ADC 2u
#define TCA9548_ADDRESS 0x70u
#define VL6180X_ADDRESS 0x29u
#define TMP117_ADDRESS 0x48u

typedef enum { FP_OK=0, FP_ERR_ARGUMENT=-1, FP_ERR_TIMEOUT=-2, FP_ERR_IO=-3,
 FP_ERR_CRC=-4, FP_ERR_RANGE=-5, FP_ERR_STATE=-6, FP_ERR_STORAGE=-7,
 FP_ERR_INTERLOCK=-8 } fp_result_t;
typedef enum { FP_JOB_IDLE, FP_JOB_ZEROING, FP_JOB_GEOMETRY, FP_JOB_GUIDANCE,
 FP_JOB_ACOUSTIC_BASELINE, FP_JOB_VERIFY, FP_JOB_COMPLETE, FP_JOB_FAULT } fp_job_state_t;
typedef enum { FP_SERVICE_OFFLINE, FP_SERVICE_WATER, FP_SERVICE_APPROVED,
 FP_SERVICE_FORBID_EXCITATION } fp_service_mode_t;

typedef struct { uint32_t timestamp_ms; float gap_mm[FP_SECTORS];
 float temperature_c[FP_SECTORS]; uint8_t valid_mask; uint16_t battery_mv;
 float board_temperature_c; } fp_geometry_frame_t;
typedef struct { float mean_gap_mm, tilt_x_mm, tilt_y_mm, tilt_magnitude_mm;
 float tilt_direction_deg, residual_mm[FP_SECTORS], rms_residual_mm;
 uint8_t valid_mask, valid_count, suggested_sector; bool recommendation_valid; } fp_fit_t;
typedef struct { float band_energy[FP_SECTORS][FP_ACOUSTIC_BANDS];
 float correlation[FP_SECTORS]; uint8_t valid_mask, risk_score, hottest_sector; } fp_acoustic_frame_t;
typedef struct { uint32_t sequence, started_ms, completed_ms, calibration_id;
 uint16_t bolt_count, battery_mv; fp_service_mode_t service_mode;
 fp_fit_t initial_fit, final_fit; fp_acoustic_frame_t acoustic; uint32_t crc32; } fp_job_record_t;
typedef struct { fp_job_state_t state; fp_service_mode_t service_mode;
 uint32_t started_ms, last_activity_ms, sequence; uint16_t bolt_count;
 uint8_t completed_steps, fault_count; bool excitation_armed; } fp_job_t;
typedef struct { float zero_mm[FP_SECTORS], scale[FP_SECTORS];
 float acoustic_baseline[FP_SECTORS][FP_ACOUSTIC_BANDS];
 uint32_t calibration_id, crc32; } fp_calibration_t;

uint32_t fp_millis(void);
void fp_platform_init(void);
void fp_platform_advance(void);
void fp_platform_sleep(void);
void fp_set_status(bool red, bool green, bool blue);
fp_result_t fp_i2c_write(uint8_t address,const uint8_t *data,size_t length);
fp_result_t fp_i2c_read(uint8_t address,uint16_t reg,uint8_t *data,size_t length);
fp_result_t fp_spi_transfer(const uint8_t *tx,uint8_t *rx,size_t length);
int fp_ble_receive(uint8_t *data,size_t capacity);
void fp_ble_notify(const uint8_t *data,size_t length);
uint16_t fp_read_battery_mv(void);
float fp_read_board_temperature(void);
void fp_sensors_init(void);
fp_result_t fp_capture_geometry(fp_geometry_frame_t *frame,const fp_calibration_t *calibration);
fp_result_t fp_capture_acoustic(fp_acoustic_frame_t *frame,const fp_calibration_t *calibration,fp_service_mode_t mode,bool active);
fp_result_t fp_fit_geometry(const fp_geometry_frame_t *frame,fp_fit_t *fit);
uint8_t fp_score_risk(const fp_fit_t *fit,const fp_acoustic_frame_t *acoustic);
void fp_job_init(fp_job_t *job);
fp_result_t fp_job_start(fp_job_t *job,fp_service_mode_t mode,uint16_t bolts);
fp_result_t fp_job_step(fp_job_t *job,const fp_geometry_frame_t *frame,const fp_fit_t *fit);
fp_result_t fp_record_save(const fp_job_record_t *record);
bool fp_record_get(size_t newest_index,fp_job_record_t *record);
size_t fp_record_count(void);
size_t fp_encode_status(const fp_job_t *job,const fp_geometry_frame_t *frame,const fp_fit_t *fit,const fp_acoustic_frame_t *acoustic,uint8_t *output,size_t capacity);
fp_result_t fp_handle_command(const uint8_t *packet,size_t length,fp_job_t *job);
uint16_t fp_crc16(const uint8_t *data,size_t length);
uint32_t fp_crc32(const uint8_t *data,size_t length);
#endif
