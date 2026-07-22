/*
 * drivers/ads1220.c — TI ADS1220 24-bit ΔΣ ADC driver (HCP + eddy demod)
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#include "../board.h"
#include "../registers.h"
#include "ads1220.h"

/* ---- ADS1220 register map ---- */
#define ADS1220_REG_CONFIG0  0x00
#define ADS1220_REG_CONFIG1  0x01
#define ADS1220_REG_CONFIG2  0x02
#define ADS1220_REG_CONFIG3  0x03

/* Config0 fields: PGA bypass, gain, mux */
#define ADS1220_MUX_AIN0_AIN1   (0x0u << 1)
#define ADS1220_MUX_AIN2_AIN3   (0x4u << 1)
#define ADS1220_MUX_TEMP        (0xFu << 1)
#define ADS1220_PGA_BYPASS      (1u << 0)

/* Config1: data rate, mode, conv mode */
#define ADS1220_DR_20SPS         (0x0u << 5)
#define ADS1220_DR_45SPS         (0x1u << 5)
#define ADS1220_DR_175SPS        (0x2u << 5)
#define ADS1220_MODE_NORMAL      (0x0u << 2)
#define ADS1220_CM_SINGLE        (0x0u << 1)
#define ADS1220_CM_CONT          (0x1u << 1)

/* Config2: Vref, FIR, IDAC */
#define ADS1220_VREF_INT          (0x0u << 6)
#define ADS1220_VREF_EXT          (0x2u << 6)
#define ADS1220_FIR_50_60         (0x3u << 4)  /* 50/60 Hz rejection */
#define ADS1220_IDAC_OFF          (0x0u << 0)
#define ADS1220_IDAC_50UA         (0x4u << 0)

/* Config3: IDAC routing, SPI mode */
#define ADS1220_I1MUX_OFF         (0x0u << 5)
#define ADS1220_I2MUX_OFF         (0x0u << 2)

/* Commands */
#define ADS1220_CMD_RESET         0x06
#define ADS1220_CMD_START         0x08
#define ADS1220_CMD_POWERDOWN     0x02
#define ADS1220_CMD_RDATA         0x10
#define ADS1220_CMD_RREG(r)       (0x20u | ((r) << 2))
#define ADS1220_CMD_WREG(r)       (0x40u | ((r) << 2))

#define ADS1220_VREF_MV   2048.0f    /* internal Vref 2.048 V */
#define ADS1220_PGA_GAIN  4.0f        /* gain 4 = 0.512 V full scale */

/* ---- Low-level SPI helpers (SPI1 shared with OLED/SD, manual CS) ---- */
static void spi1_cs_low(uint8_t pin);
static void spi1_cs_high(uint8_t pin);
static uint8_t spi1_xfer(uint8_t tx);

static void ads1220_delay_us(uint32_t us)
{
    /* crude blocking delay: at 120 MHz each loop ~6 cycles */
    volatile uint32_t n = us * 20u;
    while (n--) __asm volatile("nop");
}

static void spi1_cs_low(uint8_t pin)
{
    GPIO_TypeDef *port = gpio_port_of_pin(pin);
    uint8_t b = gpio_pin_num(pin);
    port->BSRR = (1u << (b + 16));   /* reset */
}

static void spi1_cs_high(uint8_t pin)
{
    GPIO_TypeDef *port = gpio_port_of_pin(pin);
    uint8_t b = gpio_pin_num(pin);
    port->BSRR = (1u << b);          /* set */
}

static uint8_t spi1_xfer(uint8_t tx)
{
    while (!(SPI1->SR & SPI_SR_TXE)) ;
    *(volatile uint8_t *)&SPI1->DR = tx;
    while (!(SPI1->SR & SPI_SR_RXNE)) ;
    return *(volatile uint8_t *)&SPI1->DR;
}

/* ---- ADS1220 public API ---- */
void ads1220_init(void)
{
    /* Enable SPI1 clock (handled in board_init); power-gate ADS1220 AFE */
    pwr_gate_enable(PIN_PWR_ADS1220, 1);
    ads1220_delay_us(1000);

    spi1_cs_high(PIN_ADS1220_CS);
    spi1_xfer(ADS1220_CMD_RESET);
    ads1220_delay_us(200);

    /* Config0: AIN0/AIN1 (HCP differential), PGA gain 4 (bypass=0) */
    ads1220_write_reg(ADS1220_REG_CONFIG0,
                      ADS1220_MUX_AIN0_AIN1 | (0x2u << 1));  /* gain 4 */
    /* Config1: 20 SPS, normal, continuous */
    ads1220_write_reg(ADS1220_REG_CONFIG1,
                      ADS1220_DR_20SPS | ADS1220_MODE_NORMAL | ADS1220_CM_CONT);
    /* Config2: internal Vref, 50/60 Hz rejection, IDAC off */
    ads1220_write_reg(ADS1220_REG_CONFIG2,
                      ADS1220_VREF_INT | ADS1220_FIR_50_60 | ADS1220_IDAC_OFF);
    /* Config3: IDAC routing off, I2C-off */
    ads1220_write_reg(ADS1220_REG_CONFIG3,
                      ADS1220_I1MUX_OFF | ADS1220_I2MUX_OFF);

    ads1220_delay_us(100);
    spi1_xfer(ADS1220_CMD_START);
}

void ads1220_write_reg(uint8_t reg, uint8_t val)
{
    spi1_cs_low(PIN_ADS1220_CS);
    spi1_xfer(ADS1220_CMD_WREG(reg));
    spi1_xfer(val);
    spi1_cs_high(PIN_ADS1220_CS);
    ads1220_delay_us(50);
}

uint8_t ads1220_read_reg(uint8_t reg)
{
    spi1_cs_low(PIN_ADS1220_CS);
    spi1_xfer(ADS1220_CMD_RREG(reg));
    uint8_t v = spi1_xfer(0xFF);
    spi1_cs_high(PIN_ADS1220_CS);
    return v;
}

int ads1220_data_ready(void)
{
    GPIO_TypeDef *port = gpio_port_of_pin(PIN_ADS1220_DRDY);
    uint8_t b = gpio_pin_num(PIN_ADS1220_DRDY);
    return (port->IDR & (1u << b)) == 0;   /* DRDY active low */
}

int32_t ads1220_read_raw(void)
{
    /* wait for DRDY (timeout ~150 ms) */
    uint32_t timeout = 300000u;
    while (!ads1220_data_ready() && --timeout) ads1220_delay_us(1);
    if (!timeout) return 0;

    spi1_cs_low(PIN_ADS1220_CS);
    spi1_xfer(ADS1220_CMD_RDATA);
    ads1220_delay_us(20);

    uint8_t b0 = spi1_xfer(0xFF);
    uint8_t b1 = spi1_xfer(0xFF);
    uint8_t b2 = spi1_xfer(0xFF);
    spi1_cs_high(PIN_ADS1220_CS);

    int32_t raw = ((int32_t)b0 << 16) | ((int32_t)b1 << 8) | b2;
    if (raw & 0x800000) raw |= 0xFF000000;   /* sign-extend 24-bit */
    return raw;
}

void ads1220_select_mux(uint8_t mux)
{
    uint8_t cfg = ads1220_read_reg(ADS1220_REG_CONFIG0);
    cfg = (cfg & ~(0xFu << 1)) | (mux << 1);
    ads1220_write_reg(ADS1220_REG_CONFIG0, cfg);
}

/* Convert raw code to millivolts at the HCP input */
float ads1220_raw_to_mv(int32_t raw)
{
    /* Fullscale = Vref / (PGA gain) = 2048/4 = 512 mV
     * LSB = 512 mV / (2^23 - 1) ≈ 6.10e-5 mV/LSB */
    const float lsb_mv = ADS1220_VREF_MV / (ADS1220_PGA_GAIN * 8388607.0f);
    return (float)raw * lsb_mv;
}

void ads1220_powerdown(void)
{
    spi1_xfer(ADS1220_CMD_POWERDOWN);
    pwr_gate_enable(PIN_PWR_ADS1220, 0);
}