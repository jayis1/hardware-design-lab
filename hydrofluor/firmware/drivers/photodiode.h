/*
 * photodiode.h — HydroFluor photodiode acquisition (ADS1256 + 74HC4051)
 * Author: jayis1
 * License: MIT
 */
#ifndef PHOTODIODE_H
#define PHOTODIODE_H

#include "board.h"
#include <stdint.h>

/* Initialize SPI1 + GPIO for ADS1256, mux, and DRDY EXTI. */
void photodiode_init(void);

/* Select one of 8 analog mux inputs feeding the ADS1256. */
void photodiode_select(pd_channel_t ch);

/* Read one 24-bit sample from the ADS1256 (blocking, ~33 µs at 30 kSPS).
 * Returns microvolts at the ADC input (after PGA gain). */
int32_t photodiode_read_uv(void);

/* Acquire a complete dark+light pair for the given detector channel.
 * dark_uv and light_uv are returned as ADC microvolts. */
void photodiode_acquire_pair(pd_channel_t ch,
                             int32_t *dark_uv, int32_t *light_uv);

/* Acquire a full 6-excitation × 4-detector fluorescence matrix.
 * out_matrix is EX_CHANNEL_COUNT*PD_CHANNEL_COUNT of net (light-dark) uV.
 * Also fills ref_255 and ref_660 transmittance values.
 * Returns 0 on success, nonzero on overrange (bit0) or bubble (bit1). */
typedef struct {
    int32_t fluor[EX_CHANNEL_COUNT][PD_CHANNEL_COUNT];  /* net uV, raw */
    int32_t ref_255_uv;       /* 255 nm transmission reference */
    int32_t ref_660_uv;       /* 660 nm transmission reference */
    int32_t scatter_660_uv;    /* 90° side-scatter for turbidity */
    uint16_t flags;           /* bit0=overrange, bit1=bubble detected */
} acquisition_t;

int photodiode_acquire_matrix(acquisition_t *out,
                               const led_config_t cfg[EX_CHANNEL_COUNT]);

/* Set the ADS1256 PGA gain (1,2,4,8,16,32,64). */
void photodiode_set_gain(uint8_t gain);

/* Self-test: reads ADS1256 ID register (should return 0x3). */
uint8_t photodiode_selftest(void);

#endif /* PHOTODIODE_H */