/*
 * ads1256.h — ADS1256 24-bit ADC driver header
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef LITHOCORE_ADS1256_H
#define LITHOCORE_ADS1256_H

#include <stdint.h>
#include "board.h"

/* ADS1256 commands */
#define ADS1256_CMD_WAKEUP   0x00
#define ADS1256_CMD_RDATA    0x01
#define ADS1256_CMD_RDATAC   0x03
#define ADS1256_CMD_SDATAC   0x0F
#define ADS1256_CMD_RREG     0x10  /* + register offset */
#define ADS1256_CMD_WREG     0x20  /* + register offset */
#define ADS1256_CMD_SELFCAL  0xF0
#define ADS1256_CMD_SYNC     0xFC
#define ADS1256_CMD_STANDBY  0xFD
#define ADS1256_CMD_RESET    0xFE

/* ADS1256 registers */
#define ADS1256_REG_STATUS   0x00
#define ADS1256_REG_MUX      0x01
#define ADS1256_REG_ADCON    0x02
#define ADS1256_REG_DRATE    0x03
#define ADS1256_REG_IO       0x04
#define ADS1256_REG_OFC0     0x05
#define ADS1256_REG_OFC1     0x06
#define ADS1256_REG_OFC2     0x07
#define ADS1256_REG_FSC0     0x08
#define ADS1256_REG_FSC1     0x09
#define ADS1256_REG_FSC2     0x0A

/* Data rates */
#define ADS1256_DRATE_30000  0xF0
#define ADS1256_DRATE_15000  0xE0
#define ADS1256_DRATE_7500   0xD0
#define ADS1256_DRATE_1000   0xA2
#define ADS1256_DRATE_500    0x93
#define ADS1256_DRATE_100    0x82
#define ADS1256_DRATE_30     0x72

/* PGA gain settings */
#define ADS1256_GAIN_1       0x00
#define ADS1256_GAIN_2       0x01
#define ADS1256_GAIN_4       0x02
#define ADS1256_GAIN_8       0x03
#define ADS1256_GAIN_16      0x04
#define ADS1256_GAIN_32      0x05
#define ADS1256_GAIN_64      0x06

/* MUX channel settings (AINP | AINN) — differential pairs */
#define ADS1256_MUX_AIN0_AIN1   0x00  /* V_cell differential */
#define ADS1256_MUX_AIN2_AIN3   0x14  /* V_ac differential */
#define ADS1256_MUX_AIN4_AINCOM 0x08  /* I_sense single-ended */

/* Conversion: 24-bit signed → voltage.
 * Vref = 2.5 V, PGA = 1 → full scale = ±2.5 V
 * LSB = 2 * 2.5 / 2^24 = 0.298 µV */
#define ADS1256_LSB_UV      0.298023224f
#define ADS1256_VREF_MV     2500.0f

/* Sample buffer for one frequency point */
#define ADS1256_MAX_SAMPLES 2048

typedef struct {
    int32_t  v_raw[ADS1256_MAX_SAMPLES];   /* raw 24-bit V samples */
    int32_t  i_raw[ADS1256_MAX_SAMPLES];   /* raw 24-bit I samples */
    uint16_t count;
    uint32_t sample_rate_hz;
    uint32_t freq_hz;
    uint32_t dds_phase;    /* DDS phase register at acquisition start */
} ads1256_capture_t;

/* API */
int  ads1256_init(void);
void ads1256_reset(void);
int  ads1256_set_drate(uint8_t drate_reg);
int  ads1256_set_mux(uint8_t mux_reg);
int  ads1256_set_pga(uint8_t gain_reg);
int  ads1256_self_cal(void);
int  ads1256_read_register(uint8_t reg, uint8_t *val);
int  ads1256_write_register(uint8_t reg, uint8_t val);
int  ads1256_read_data(int32_t *value);  /* single shot */
int  ads1256_capture(ads1256_capture_t *cap, uint16_t num_samples,
                     uint32_t target_rate);
int  ads1256_capture_dual(ads1256_capture_t *v_cap, ads1256_capture_t *i_cap,
                          uint16_t num_samples, uint32_t target_rate);
void ads1256_standby(void);

/* SPI low-level (used by ads1256.c) */
void ads1256_spi_write(uint8_t *data, uint8_t len);
void ads1256_spi_read(uint8_t *data, uint8_t len);
void ads1256_cs_low(void);
void ads1256_cs_high(void);
uint8_t ads1256_wait_drdy(uint32_t timeout_ms);

#endif /* LITHOCORE_ADS1256_H */