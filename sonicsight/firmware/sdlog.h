/**
 * @file    sdlog.h
 * @brief   SonicSight — microSD card data logging module.
 *          Writes raw waveforms, ToF matrices, and reconstructed images
 *          to FAT32 filesystem for offline analysis.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef SDLOG_H
#define SDLOG_H

#include <stdint.h>
#include "board.h"

/**
 * @brief Initialise SD card subsystem.
 * @return ERR_OK on success, ERR_SD_FAIL on error.
 */
int32_t sdlog_init(void);

/**
 * @brief Write raw ADC waveforms for a given transmitter to file.
 * @param tx_index Transmitter index.
 * @param buffers  Array of 8 ADC channel buffers.
 * @param num_ch   Number of channels.
 * @param samples  Samples per channel.
 * @return ERR_OK on success, negative on error.
 */
int32_t sdlog_write_waveforms(uint8_t tx_index, int16_t **buffers,
                               uint8_t num_ch, uint32_t samples);

/**
 * @brief Write time-of-flight matrix to SD card as CSV.
 * @param ctx Acquisition context with populated ToF matrix.
 * @return ERR_OK on success, negative on error.
 */
int32_t sdlog_write_tof_matrix(const acquisition_ctx_t *ctx);

/**
 * @brief Write reconstructed slowness image to SD card (binary float dump).
 * @param image     Image buffer (row-major).
 * @param width     Image width.
 * @param height    Image height.
 * @return ERR_OK on success, negative on error.
 */
int32_t sdlog_write_image(const float *image, uint32_t width, uint32_t height);

#endif /* SDLOG_H */