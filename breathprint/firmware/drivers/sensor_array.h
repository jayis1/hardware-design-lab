/*
 * sensor_array.h — Multi-sensor sampling header
 * BreathPrint — Portable Breath VOC Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

#ifndef SENSOR_ARRAY_H
#define SENSOR_ARRAY_H

#include <stdint.h>
#include <string.h>
#include "../board.h"

void sensor_array_init(void);
void sensor_array_sample_all(sensor_raw_t *sample, calib_data_t *calib);
void sensor_array_read_ambient(sensor_raw_t *sample, calib_data_t *calib);
void sensor_array_apply_calib(sensor_raw_t *sample, calib_data_t *calib);
float sensor_array_get_channel(const sensor_raw_t *sample, int channel);
void sensor_array_set_channel(sensor_raw_t *sample, int channel, uint16_t val);
void sensor_array_extract_features(const sensor_raw_t *samples,
                                   uint32_t count,
                                   calib_data_t *calib,
                                   feature_vector_t *features);
uint8_t sensor_array_check_ready(void);
void sensor_array_heaters_on(void);
void sensor_array_heaters_off(void);
uint8_t sensor_array_health(void);
uint8_t breath_validate(const sensor_raw_t *samples, uint32_t count,
                        calib_data_t *calib);
void breath_estimate(const sensor_raw_t *samples, uint32_t count,
                     calib_data_t *calib, feature_vector_t *features,
                     breath_result_t *result);

/* BME688 helpers */
void bme688_configure(uint8_t addr, const void *profile);
uint8_t bme688_calc_res_heat(uint16_t temp);
uint8_t bme688_calc_gas_wait(uint16_t ms);
uint32_t bme688_read_gas(uint8_t addr);
void bme688_read_env(uint8_t addr, float *temp, float *humidity);

/* SCD41 */
int scd41_read(uint16_t *co2, float *temp, float *humidity);

/* SenseAir S8 */
uint16_t s8_read_ch4(void);

/* Spec DGS */
int dgs_sensor_init(uint8_t addr);
int dgs_sensor_read(uint8_t addr, uint16_t *gas_ppb, float *temp);

/* BMP390 */
void bmp390_configure(void);
float bmp390_read_pressure(void);

#endif /* SENSOR_ARRAY_H */