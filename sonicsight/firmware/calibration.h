/**
 * @file    calibration.h
 * @brief   SonicSight — Calibration module.
 *          Per-channel gain/timing offset calibration using reference phantom.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <stdint.h>
#include "board.h"

/** Number of calibrated channels */
#define CAL_NUM_CHANNELS    (ADC_NUM_CHANNELS)

/** Calibration data structure — stored in QSPI flash as a single block */
typedef struct {
    uint32_t magic;                              /**< Magic number = 0x534F4E49 ("SONI") */
    uint32_t version;                            /**< Calibration data format version */
    float    channel_gain_db[CAL_NUM_CHANNELS];  /**< Per-channel gain offset (dB) */
    float    channel_delay_us[CAL_NUM_CHANNELS]; /**< Per-channel cable/phase delay (µs) */
    float    channel_offset[CAL_NUM_CHANNELS];   /**< Per-channel DC offset (ADC counts) */
    float    temp_reference;                     /**< Reference temperature (°C) */
    float    alpha_velocity;                     /**< Temperature coefficient (°C⁻¹) */
    uint32_t timestamp;                          /**< Unix timestamp of calibration */
    uint32_t crc32;                              /**< CRC-32 of entire structure (excluding crc32 field) */
} calibration_data_t;

/**
 * @brief Initialise calibration subsystem.
 * @param data Calibration data pointer.
 */
void calibration_init(calibration_data_t *data);

/**
 * @brief Load calibration from QSPI flash.
 * @param data Calibration data pointer.
 * @return ERR_OK on success, negative on error.
 */
int32_t calibration_load(calibration_data_t *data);

/**
 * @brief Save calibration to QSPI flash.
 * @param data Calibration data pointer.
 * @return ERR_OK on success, negative on error.
 */
int32_t calibration_save(const calibration_data_t *data);

/**
 * @brief Set factory-default calibration values.
 * @param data Calibration data pointer.
 */
void calibration_set_defaults(calibration_data_t *data);

/**
 * @brief Run calibration scan using reference phantom.
 * @param data Calibration data pointer (updated with results).
 * @return ERR_OK on success, error code on failure.
 */
int32_t run_calibration_scan(calibration_data_t *data);

#endif /* CALIBRATION_H */