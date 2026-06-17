/**
 * @file    sdlog.c
 * @brief   SonicSight — microSD card data logging implementation.
 *          Uses FatFs (ELM_CHAN) or HAL SD/SPI driver for file I/O.
 *          Writes CSV waveform files, ToF matrices, and binary image data.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#include <string.h>
#include <stdio.h>
#include "sdlog.h"
#include "board.h"
#include "registers.h"
#include "acquisition.h"

/* ========================================================================== */
/*  Initialisation                                                            */
/* ========================================================================== */

/* For simplicity, we stub the file system operations.
 * In production, this would use FatFs (ff.c/ff.h) with SPI SD driver.
 */

int32_t sdlog_init(void)
{
    /* 1. Initialise SPI SD card driver */
    /* SPI6 is configured in main, CS = PG8 */

    /* 2. Detect card */
    /* Check SD_DETECT_PIN (PD15) */
    if (HAL_GPIO_ReadPin(SD_DETECT_PORT, SD_DETECT_PIN) != GPIO_PIN_RESET) {
        return ERR_SD_FAIL; /* No card inserted */
    }

    /* 3. Send SPI init sequence to SD card (CMD0, CMD8, ACMD41, etc.) */
    /* For brevity, this assumes successful init */
    HAL_Delay(100);

    /* 4. Mount FAT32 filesystem */
    /* f_mount(&SDFatFS, "", 1); */

    return ERR_OK;
}

/* ========================================================================== */
/*  Waveform CSV Writer                                                       */
/* ========================================================================== */

int32_t sdlog_write_waveforms(uint8_t tx_index, int16_t **buffers,
                               uint8_t num_ch, uint32_t samples)
{
    (void)tx_index;
    (void)buffers;
    (void)num_ch;
    (void)samples;

    /* In production:
     * char filename[32];
     * snprintf(filename, sizeof(filename), "SIGHT_%03lu_CH%d.CSV", scan_count, tx_index);
     * FIL fil;
     * f_open(&fil, filename, FA_CREATE_ALWAYS | FA_WRITE);
     * f_printf(&fil, "Sample,Ch0,Ch1,Ch2,...\r\n");
     * for (uint32_t s = 0; s < samples; s++) {
     *     f_printf(&fil, "%lu", s);
     *     for (uint8_t ch = 0; ch < num_ch; ch++) {
     *         f_printf(&fil, ",%d", buffers[ch][s]);
     *     }
     *     f_printf(&fil, "\r\n");
     * }
     * f_close(&fil);
     */

    return ERR_OK;
}

/* ========================================================================== */
/*  ToF Matrix CSV Writer                                                     */
/* ========================================================================== */

int32_t sdlog_write_tof_matrix(const acquisition_ctx_t *ctx)
{
    (void)ctx;

    /* In production:
     * char filename[32];
     * snprintf(filename, sizeof(filename), "SIGHT_%03lu_TOF.CSV", scan_count);
     * FIL fil;
     * f_open(&fil, filename, FA_CREATE_ALWAYS | FA_WRITE);
     * // Header: TX\\RX, then RX index
     * f_printf(&fil, "TX\\\\RX");
     * for (uint8_t rx = 0; rx < TOMO_MAX_SENSORS; rx++)
     *     f_printf(&fil, ",S%u", rx);
     * f_printf(&fil, "\r\n");
     * // Data rows
     * for (uint8_t tx = 0; tx < TOMO_MAX_SENSORS; tx++) {
     *     f_printf(&fil, "S%u", tx);
     *     for (uint8_t rx = 0; rx < TOMO_MAX_SENSORS; rx++)
     *         f_printf(&fil, ",%.2f", ctx->tof_matrix[tx][rx]);
     *     f_printf(&fil, "\r\n");
     * }
     * f_close(&fil);
     */

    return ERR_OK;
}

/* ========================================================================== */
/*  Image Binary Writer                                                       */
/* ========================================================================== */

int32_t sdlog_write_image(const float *image, uint32_t width, uint32_t height)
{
    (void)image;
    (void)width;
    (void)height;

    /* In production:
     * char filename[32];
     * snprintf(filename, sizeof(filename), "SIGHT_%03lu_IMG.RAW", scan_count);
     * FIL fil;
     * f_open(&fil, filename, FA_CREATE_ALWAYS | FA_WRITE);
     * // Write header: width, height, then raw float data
     * uint32_t header[2] = { width, height };
     * UINT bw;
     * f_write(&fil, header, 8, &bw);
     * f_write(&fil, image, width * height * sizeof(float), &bw);
     * f_close(&fil);
     */

    return ERR_OK;
}