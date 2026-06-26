/*
 * adc_driver.h — ADS1256 24-bit seismic ADC driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef ADC_DRIVER_H
#define ADC_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "board.h"

/* Per-channel sample (3 geophone + mems-downsampled + temp + vbat + vsol + cal) */
typedef struct {
    int32_t  raw[ADC_CHANNELS];   /* 24-bit signed, MSB-extended */
    float    volts[ADC_CHANNELS]; /* scaled to volts (Vref=2.5, PGA=32) */
    uint32_t ts_us;               /* timestamp from 1-PPS-disciplined clock */
} adc_sample_t;

/* Double-buffered frame of N channels' worth of samples */
#define ADC_FRAME_LEN  64          /* 64-sample frames at 200 SPS = 0.32 s */

typedef struct {
    adc_sample_t samples[ADC_FRAME_LEN];
    uint16_t     len;
    volatile bool ready;           /* set when frame full */
} adc_frame_t;

void     adc_init(void);
void     adc_start_continuous(void);
void     adc_stop(void);
void     adc_reset(void);
int32_t  adc_read_single(uint8_t mux);   /* blocking, one channel */
void     adc_calibrate(void);             /* self-offset calibration */
void     adc_set_pga(uint8_t pga);
void     adc_set_drate(uint8_t drate_cmd);

/* ISR handler — called from EXTI on DRDY; fills background frame */
void     adc_drdy_isr(void);

/* Active-frame accessor for the seismology module */
adc_frame_t *adc_take_frame(void);
void     adc_release_frame(void);

/* Board-temperature / voltage helpers (use channels 4-7) */
float    adc_read_board_temp_c(void);
float    adc_read_vbat_mv(void);
float    adc_read_vsol_mv(void);

#endif /* ADC_DRIVER_H */