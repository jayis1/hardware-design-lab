/*
 * adc_driver.c — ADS1256 24-bit seismic ADC driver with DMA double-buffer
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#include "adc_driver.h"
#include "registers.h"
#include "board.h"
#include <string.h>

/* ---- Internal state ------------------------------------------------------ */
static volatile adc_frame_t  g_frame_a;
static volatile adc_frame_t  g_frame_b;
static volatile adc_frame_t *g_active;    /* ISR fills this */
static volatile adc_frame_t *g_ready;     /* seismology reads this */
static volatile bool         g_owns_ready; /* true until released */
static uint8_t               g_pga = ADS1256_ADCON_PGA_32;
static uint8_t               g_drate = ADS1256_DRATE_1000SPS;
static const float           VREF = 2.5f;
static const uint8_t         CHANNEL_ORDER[ADC_CHANNELS] = {
    ADS1256_MUX_AIN0_AINCOM,   /* geophone Z   */
    ADS1256_MUX_AIN1_AINCOM,   /* geophone N-S */
    ADS1256_MUX_AIN2_AINCOM,   /* geophone E-W */
    ADS1256_MUX_AIN3_AINCOM,   /* MEMS downsample (routed from ICM-42688 analog test) */
    ADS1256_MUX_AIN4_AINCOM,   /* board temp   */
    ADS1256_MUX_AIN5_AINCOM,   /* battery V    */
    ADS1256_MUX_AIN6_AINCOM,   /* solar V      */
    ADS1256_MUX_AIN7_AINCOM,   /* cal loop     */
};
static uint8_t               g_ch_idx = 0;
static uint16_t              g_sample_idx = 0;

/* ---- Low-level SPI (stubbed HAL — real build links stm32wl55_hal) ------- */
static void spi_select(void)        { /* CS low */ }
static void spi_deselect(void)      { /* CS high */ }
static uint8_t spi_xfer(uint8_t tx) { (void)tx; return 0xFF; /* placeholder */ }
static void delay_us(uint32_t us)   { (void)us; /* SysTick spin */ }
static void delay_ms(uint32_t ms)   { delay_us(ms * 1000); }

/* ---- Internal helpers ---------------------------------------------------- */
static void ads1256_write_reg(uint8_t reg, uint8_t val)
{
    spi_select();
    spi_xfer(ADS1256_CMD_WREG | (reg & 0x0F));
    spi_xfer(0x00);            /* 1 register */
    spi_xfer(val);
    spi_deselect();
    delay_us(100);
}

static uint8_t ads1256_read_reg(uint8_t reg)
{
    uint8_t v;
    spi_select();
    spi_xfer(ADS1256_CMD_RREG | (reg & 0x0F));
    spi_xfer(0x00);
    delay_us(50);
    v = spi_xfer(0x00);
    spi_deselect();
    return v;
}

static void ads1256_wait_drdy(uint32_t timeout_ms)
{
    /* In real HW, poll DRDY line; here we busy-wait a bounded time. */
    delay_ms(timeout_ms);
}

static int32_t ads1256_read_data24(void)
{
    uint8_t b0, b1, b2;
    int32_t v;
    spi_select();
    spi_xfer(ADS1256_CMD_RDATA);
    delay_us(7);
    b0 = spi_xfer(0x00);
    b1 = spi_xfer(0x00);
    b2 = spi_xfer(0x00);
    spi_deselect();
    v = ((int32_t)b0 << 16) | ((int32_t)b1 << 8) | b2;
    if (v & 0x800000) v |= 0xFF000000;   /* sign-extend 24→32 */
    return v;
}

/* ---- Public API ---------------------------------------------------------- */
void adc_init(void)
{
    g_active = &g_frame_a;
    g_ready  = NULL;
    g_owns_ready = false;
    g_sample_idx = 0;
    g_ch_idx = 0;

    /* Hardware reset via SYNC pin pulse */
    /* GPIO write low, delay 4*tclk, high */
    delay_ms(2);

    /* Configure STATUS: enable input buffer, disable order bit */
    ads1256_write_reg(ADS1256_REG_STATUS, ADS1256_STATUS_BUFEN);

    /* ADCON: PGA=32, clock-out off, sensor-detect off */
    ads1256_write_reg(ADS1256_REG_ADCON, g_pga);

    /* DRATE: 1000 SPS default (auto-scan effective ~125 SPS/ch) */
    ads1256_write_reg(ADS1256_REG_DRATE, g_drate);

    /* MUX: start on AIN0 */
    ads1256_write_reg(ADS1256_REG_MUX, CHANNEL_ORDER[0]);

    /* Issue self-calibration */
    spi_select();
    spi_xfer(ADS1256_CMD_SELFCAL);
    spi_deselect();
    ads1256_wait_drdy(400);
}

void adc_start_continuous(void)
{
    /* Enable RDATAc mode for continuous streaming */
    spi_select();
    spi_xfer(ADS1256_CMD_RDATAC);
    spi_deselect();
    /* Enable EXTI for DRDY pin (configured in board_init) */
}

void adc_stop(void)
{
    spi_select();
    spi_xfer(ADS1256_CMD_SDATAC);
    spi_deselect();
}

void adc_reset(void)
{
    spi_select();
    spi_xfer(ADS1256_CMD_RESET);
    spi_deselect();
    delay_ms(2);
    ads1256_write_reg(ADS1256_REG_STATUS, ADS1256_STATUS_BUFEN);
    ads1256_write_reg(ADS1256_REG_ADCON, g_pga);
    ads1256_write_reg(ADS1256_REG_DRATE, g_drate);
}

void adc_calibrate(void)
{
    spi_select();
    spi_xfer(ADS1256_CMD_SELFCAL);
    spi_deselect();
    ads1256_wait_drdy(400);
}

void adc_set_pga(uint8_t pga)
{
    g_pga = pga & 0x07;
    ads1256_write_reg(ADS1256_REG_ADCON, g_pga);
}

void adc_set_drate(uint8_t drate_cmd)
{
    g_drate = drate_cmd;
    ads1256_write_reg(ADS1256_REG_DRATE, g_drate);
}

int32_t adc_read_single(uint8_t mux)
{
    int32_t v;
    ads1256_write_reg(ADS1256_REG_MUX, mux);
    spi_select();
    spi_xfer(ADS1256_CMD_SYNC);
    spi_deselect();
    ads1256_wait_drdy(2);
    v = ads1256_read_data24();
    return v;
}

/* ---- ISR: fires on DRDY EXTI --------------------------------------------- */
void adc_drdy_isr(void)
{
    int32_t raw;
    float   v_pga;
    uint32_t ts;

    /* Read latest conversion from RDATAc stream */
    raw = ads1256_read_data24();
    ts  = 0; /* in real build: read disciplined µs counter */

    /* Convert to volts: Vref / (2^23-1) / PGA */
    v_pga = VREF / 8388607.0f;
    switch (g_pga) {
        case ADS1256_ADCON_PGA_1:  v_pga /= 1;  break;
        case ADS1256_ADCON_PGA_2:  v_pga /= 2;  break;
        case ADS1256_ADCON_PGA_4:  v_pga /= 4;  break;
        case ADS1256_ADCON_PGA_8:  v_pga /= 8;  break;
        case ADS1256_ADCON_PGA_16: v_pga /= 16; break;
        case ADS1256_ADCON_PGA_32: v_pga /= 32; break;
        case ADS1256_ADCON_PGA_64: v_pga /= 64; break;
        default: break;
    }

    g_active->samples[g_sample_idx].raw[g_ch_idx]    = raw;
    g_active->samples[g_sample_idx].volts[g_ch_idx]  = (float)raw * v_pga;
    g_active->samples[g_sample_idx].ts_us            = ts;

    /* Advance channel: auto-scan cycles 0..7 in MUX order */
    g_ch_idx = (g_ch_idx + 1) % ADC_CHANNELS;
    if (g_ch_idx == 0) {
        g_sample_idx++;
        if (g_sample_idx >= ADC_FRAME_LEN) {
            /* Frame complete — swap buffers */
            g_active->len    = ADC_FRAME_LEN;
            g_active->ready  = true;
            g_ready          = g_active;
            g_owns_ready     = true;
            g_active         = (g_active == &g_frame_a) ? &g_frame_b : &g_frame_a;
            g_sample_idx     = 0;
        }
    }
}

/* ---- Frame consumer API -------------------------------------------------- */
adc_frame_t *adc_take_frame(void)
{
    if (g_owns_ready && g_ready && g_ready->ready) {
        return (adc_frame_t *)g_ready;
    }
    return NULL;
}

void adc_release_frame(void)
{
    if (g_ready) {
        g_ready->ready = false;
    }
    g_owns_ready = false;
}

/* ---- Slow channels ------------------------------------------------------- */
float adc_read_board_temp_c(void)
{
    int32_t raw = adc_read_single(ADS1256_MUX_AIN4_AINCOM);
    /* TMP36: 500 mV @ 0C, 10 mV/C; after PGA=32, scale back */
    float v = (float)raw * (VREF / 8388607.0f) / 32.0f;
    return (v - 0.5f) * 100.0f;
}

float adc_read_vbat_mv(void)
{
    int32_t raw = adc_read_single(ADS1256_MUX_AIN5_AINCOM);
    /* voltage divider 2:1 on PCB; PGA=1 for slow channel */
    float v = (float)raw * (VREF / 8388607.0f) / 1.0f;
    return v * 2000.0f;   /* mV */
}

float adc_read_vsol_mv(void)
{
    int32_t raw = adc_read_single(ADS1256_MUX_AIN6_AINCOM);
    float v = (float)raw * (VREF / 8388607.0f) / 1.0f;
    return v * 3000.0f;   /* divider 3:1 → mV */
}