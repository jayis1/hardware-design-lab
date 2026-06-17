/*
 * adc_drv.c — ADS1256 24-bit ADC driver
 * FerroProbe — Open-Source 3-Axis Fluxgate Magnetometer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Drives the TI ADS1256 24-bit delta-sigma ADC via SPI1.
 * The ADS1256 provides 8 single-ended inputs (or 4 differential pairs).
 * FerroProbe uses 4 single-ended channels:
 *   AIN0 = X-axis fluxgate sense (after preamp + bandpass)
 *   AIN1 = Y-axis fluxgate sense
 *   AIN2 = Z-axis fluxgate sense
 *   AIN3 = LM35 temperature sensor (10 mV/°C)
 *
 * The ADC runs at 30 kSPS with PGA = 1x, Vref = 2.5V (external reference).
 * Data ready (DRDY) signals when a conversion is complete; the firmware
 * reads the result via SPI and feeds it to the lock-in engine.
 */

#include "adc_drv.h"
#include "lockin.h"
#include "../board.h"
#include "../registers.h"

/* ===================================================================== */
/*  SPI1 low-level routines                                                */
/* ===================================================================== */

static void spi1_init(void)
{
    /* Enable GPIOA and SPI1 clocks */
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIOAEN;
    RCC_APB2ENR |= RCC_APB2ENR_SPI1EN;

    /* PA5 = SPI1_SCK (AF5), PA6 = SPI1_MISO (AF5), PA7 = SPI1_MOSI (AF5) */
    volatile uint32_t *afrl = (volatile uint32_t *)(GPIOA_BASE + GPIO_AFRL);
    volatile uint32_t *moder = (volatile uint32_t *)(GPIOA_BASE + GPIO_MODER);
    volatile uint32_t *ospeedr = (volatile uint32_t *)(GPIOA_BASE + GPIO_OSPEEDR);

    for (uint8_t pin = 5; pin <= 7; pin++) {
        *afrl &= ~(0xFU << (pin * 4));
        *afrl |= ((uint32_t)GPIO_AF5 << (pin * 4));
        *moder &= ~(0x3U << (pin * 2));
        *moder |= (0x2U << (pin * 2));   /* AF mode */
        *ospeedr |= (0x3U << (pin * 2)); /* Very high speed */
    }

    /* Configure CS pins as GPIO outputs (manual chip select) */
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIODEN;
    volatile uint32_t *pd_moder = (volatile uint32_t *)(GPIOD_BASE + GPIO_MODER);
    volatile uint32_t *pd_pupdr = (volatile uint32_t *)(GPIOD_BASE + GPIO_PUPDR);

    /* ADS_CS on PD14, SD_CS on PD1, ADS_SYNC on PD0 */
    for (uint8_t pin = 0; pin <= 1; pin += 1) {  /* PD0, PD1 */
        *pd_moder &= ~(0x3U << (pin * 2));
        *pd_moder |= (0x1U << (pin * 2));   /* Output */
        *pd_pupdr |= (0x1U << (pin * 2));    /* Pull-up */
    }
    *pd_moder &= ~(0x3U << (ADS_CS_PIN * 2));
    *pd_moder |= (0x1U << (ADS_CS_PIN * 2));
    *pd_pupdr |= (0x1U << (ADS_CS_PIN * 2);

    /* Deselect all CS lines (active low) */
    gpio_set(ADS_CS_PORT, ADS_CS_PIN);
    gpio_set(SD_CS_PORT, SD_CS_PIN);
    gpio_set(ADS_SYNC_PORT, ADS_SYNC_PIN);   /* SYNC high = normal */

    /* DRDY pin: PD15 input, pull-up */
    *pd_moder &= ~(0x3U << (ADS_DRDY_PIN * 2));   /* Input mode (00) */
    *pd_pupdr |= (0x1U << (ADS_DRDY_PIN * 2));    /* Pull-up */

    /* Configure SPI1: Master, 12 MHz, CPOL=1, CPHA=1 (Mode 3 for ADS1256) */
    SPI_REG(SPI1_BASE, SPI_CR1) = 0;  /* Disable SPI */
    SPI_REG(SPI1_BASE, SPI_CFG1) = 0;
    SPI_REG(SPI1_BASE, SPI_CFG2) = 0;

    SPI_REG(SPI1_BASE, SPI_CFG1) |= SPI_CFG1_MASTER;
    SPI_REG(SPI1_BASE, SPI_CFG1) |= SPI_CFG1_DSIZE_8;   /* 8-bit frames */
    SPI_REG(SPI1_BASE, SPI_CFG1) |= (0x7U << 28);        /* FTH = 7 (8-byte FIFO) */

    /* Clock: APB2 = 120 MHz, divider /10 → 12 MHz
     * H7 SPI: baud = fPCLK / (2^(MBR+1))
     * MBR=2 → /8 = 15 MHz, MBR=3 → /16 = 7.5 MHz. Use MBR=2 (15 MHz).
     */
    SPI_REG(SPI1_BASE, SPI_CFG1) |= (0x2U << 28);  /* MBR[2:0] = 2 → /8 */

    /* CPOL=1, CPHA=1 (Mode 3) */
    SPI_REG(SPI1_BASE, SPI_CFG2) |= SPI_CFG2_CPOL | SPI_CFG2_CPHA;
    SPI_REG(SPI1_BASE, SPI_CFG2) |= SPI_CFG2_SSOE;  /* Hardware NSS for CS */
    SPI_REG(SPI1_BASE, SPI_CFG2) |= SPI_CFG2_AFCNTR;  /* AF-controlled CS */

    /* Enable SPI */
    SPI_REG(SPI1_BASE, SPI_CR1) |= SPI_CR1_SPE;
}

/* Send/receive a single byte over SPI1 (blocking) */
static uint8_t spi1_transfer(uint8_t tx_byte)
{
    /* Wait for TX FIFO space */
    while (!(SPI_REG(SPI1_BASE, SPI_SR) & SPI_SR_TXP))
        ;
    SPI_REG(SPI1_BASE, SPI_TXDR) = tx_byte;

    /* Wait for receive data */
    while (!(SPI_REG(SPI1_BASE, SPI_SR) & SPI_SR_RXP))
        ;
    uint8_t rx_byte = (uint8_t)SPI_REG(SPI1_BASE, SPI_RXDR);

    return rx_byte;
}

/* Send a command to the ADS1256 (no data) */
static void ads_cmd(uint8_t cmd)
{
    gpio_reset(ADS_CS_PORT, ADS_CS_PIN);
    delay_us(2);
    spi1_transfer(cmd);
    delay_us(2);
    gpio_set(ADS_CS_PORT, ADS_CS_PIN);
}

/* Write to an ADS1256 register */
static void ads_write_reg(uint8_t reg, uint8_t value)
{
    gpio_reset(ADS_CS_PORT, ADS_CS_PIN);
    delay_us(2);
    spi1_transfer(ADS1256_CMD_WREG | (reg << 2));  /* WREG command + reg */
    spi1_transfer(0x00);   /* Number of registers - 1 = 0 */
    spi1_transfer(value);
    delay_us(2);
    gpio_set(ADS_CS_PORT, ADS_CS_PIN);
}

/* Read from an ADS1256 register */
static uint8_t ads_read_reg(uint8_t reg)
{
    gpio_reset(ADS_CS_PORT, ADS_CS_PIN);
    delay_us(2);
    spi1_transfer(ADS1256_CMD_RREG | (reg << 2));
    spi1_transfer(0x00);   /* Number of registers - 1 = 0 */
    /* Wait for data: send dummy bytes and read response */
    delay_us(50);  /* tSCLK delay after RREG */
    uint8_t value = spi1_transfer(0xFF);
    delay_us(2);
    gpio_set(ADS_CS_PORT, ADS_CS_PIN);
    return value;
}

/* Wait for DRDY to go low (conversion complete) */
static void ads_wait_drdy(void)
{
    uint32_t timeout = 100000;
    while (gpio_read(ADS_DRDY_PORT, ADS_DRDY_PIN) && --timeout)
        ;
}

/* Read a 24-bit conversion result */
static int32_t ads_read_data(void)
{
    gpio_reset(ADS_CS_PORT, ADS_CS_PIN);
    delay_us(2);

    /* Send RDATA command */
    spi1_transfer(ADS1256_CMD_RDATA);
    delay_us(50);  /* t6 delay */

    /* Read 3 bytes (24-bit signed) */
    uint8_t byte0 = spi1_transfer(0xFF);
    uint8_t byte1 = spi1_transfer(0xFF);
    uint8_t byte2 = spi1_transfer(0xFF);

    gpio_set(ADS_CS_PORT, ADS_CS_PIN);

    /* Assemble 24-bit signed value */
    int32_t result = ((int32_t)byte0 << 16) | ((int32_t)byte1 << 8) | (int32_t)byte2;
    if (result & 0x800000)
        result |= 0xFF000000;  /* Sign-extend to 32 bits */

    return result;
}

/* ===================================================================== */
/*  Public API                                                             */
/* ===================================================================== */

static uint8_t current_channel = 0;
static volatile int32_t latest_samples[ADC_N_CHANNELS];
static volatile uint8_t adc_dma_busy = 0;

void adc_init(void)
{
    spi1_init();

    /* Reset the ADS1256 */
    gpio_reset(ADS_SYNC_PORT, ADS_SYNC_PIN);
    delay_us(10);
    gpio_set(ADS_SYNC_PORT, ADS_SYNC_PIN);
    delay_ms(2);  /* Reset recovery */

    /* Read chip ID to verify communication */
    uint8_t status = ads_read_reg(ADS1256_REG_STATUS);
    (void)status;  /* Status[5:0] = chip ID, should be 0x03 for ADS1256 */

    /* Configure ADS1256 registers:
     * STATUS: ORDER=0 (MSB first), ACAL=0, BUFEN=1, DRDY=1
     * MUX: AIN0 (default)
     * ADCON: PGA=1x, clock out off, sensor detect off
     * DRATE: 30 kSPS
     */
    ads_write_reg(ADS1256_REG_STATUS, 0x0CU);   /* BUFEN=1, DRDY=1 */
    ads_write_reg(ADS1256_REG_MUX, 0x08U);      /* AIN0 positive, AINCOM negative */
    ads_write_reg(ADS1256_REG_ADCON, ADS1256_PGA_1X);  /* PGA = 1x */
    ads_write_reg(ADS1256_REG_DRATE, ADS1256_DRATE_30000SPS);

    /* Perform self-calibration */
    adc_self_cal();

    current_channel = 0;
    for (int i = 0; i < ADC_N_CHANNELS; i++)
        latest_samples[i] = 0;
}

void adc_self_cal(void)
{
    ads_cmd(ADS1256_CMD_SYNC);
    delay_ms(2);
    /* Self-calibration: offset + gain
     * The ADS1256 performs this on SYNC after reset, but we force it.
     * A full self-cal takes ~600 ms at 30 kSPS.
     */
    /* Issue WREG to STATUS with ACAL=1 */
    ads_write_reg(ADS1256_REG_STATUS, 0x0EU);  /* ACAL=1, BUFEN=1, DRDY=1 */
    ads_cmd(ADS1256_CMD_SYNC);
    delay_ms(650);  /* Wait for calibration to complete */
    ads_write_reg(ADS1256_REG_STATUS, 0x0CU);  /* ACAL=0 */
}

int32_t adc_read_channel(uint8_t channel)
{
    if (channel >= ADC_N_CHANNELS)
        return 0;

    /* Set multiplexer: channel positive, AINCOM (8) negative */
    uint8_t mux_val = (channel << 4) | 0x08U;
    ads_write_reg(ADS1256_REG_MUX, mux_val);

    /* SYNC to start new conversion on the new channel */
    ads_cmd(ADS1256_CMD_SYNC);

    /* Wait for conversion complete */
    ads_wait_drdy();

    /* Read the result */
    int32_t result = ads_read_data();
    latest_samples[channel] = result;

    /* Feed to lock-in engine (for magnetic channels 0-2) */
    lockin_feed_sample(channel, result);

    return result;
}

/* Multiplex through all 4 channels and read each */
void adc_read_all(int32_t samples[ADC_N_CHANNELS])
{
    for (uint8_t ch = 0; ch < ADC_N_CHANNELS; ch++) {
        samples[ch] = adc_read_channel(ch);
    }
}

void adc_start_continuous(void)
{
    /* Enable continuous read mode (RDATAC) */
    gpio_reset(ADS_CS_PORT, ADS_CS_PIN);
    delay_us(2);
    spi1_transfer(ADS1256_CMD_RDATAC);
    delay_us(50);

    /* Configure DRDY EXTI interrupt on falling edge */
    /* Enable EXTI line 15 for PD15 */
    RCC_APB4ENR |= RCC_APB4ENR_SYSCFGEN;
    /* SYSCFG_EXTICR4 for line 15 → port D (0x3) */
    volatile uint32_t *exticr4 = (volatile uint32_t *)(0x5800040CU);
    *exticr4 &= ~(0xFU << 8);   /* EXTI15 */
    *exticr4 |= (0x3U << 8);    /* Port D */

    /* Enable EXTI15 interrupt (falling edge) */
    volatile uint32_t *exti_imr1 = (volatile uint32_t *)(0x58000440U);
    *exti_imr1 |= (1U << 15);
    /* Falling edge trigger */
    volatile uint32_t *exti_ftsr1 = (volatile uint32_t *)(0x58000448U);
    *exti_ftsr1 |= (1U << 15);

    /* Enable NVIC interrupt for EXTI15 */
    NVIC_ISER0 |= (1U << 40);  /* EXTI15_10 is IRQ 40 */

    adc_dma_busy = 0;
    current_channel = 0;
}

void adc_stop_continuous(void)
{
    /* Disable EXTI15 interrupt */
    volatile uint32_t *exti_imr1 = (volatile uint32_t *)(0x58000440U);
    *exti_imr1 &= ~(1U << 15);

    /* Send SDATAC command */
    delay_us(10);
    spi1_transfer(ADS1256_CMD_SDATAC);
    delay_us(10);
    gpio_set(ADS_CS_PORT, ADS_CS_PIN);

    adc_dma_busy = 0;
}

void adc_set_pga(uint8_t gain)
{
    uint8_t gain_code;
    switch (gain) {
        case 1:  gain_code = ADS1256_PGA_1X;  break;
        case 2:  gain_code = ADS1256_PGA_2X;  break;
        case 4:  gain_code = ADS1256_PGA_4X;  break;
        case 8:  gain_code = ADS1256_PGA_8X;  break;
        case 16: gain_code = ADS1256_PGA_16X; break;
        case 32: gain_code = ADS1256_PGA_32X; break;
        default: gain_code = ADS1256_PGA_1X;  break;
    }
    ads_write_reg(ADS1256_REG_ADCON, gain_code);
}

void adc_set_drate(uint8_t drate_code)
{
    ads_write_reg(ADS1256_REG_DRATE, drate_code);
}

int32_t adc_get_latest(uint8_t channel)
{
    if (channel >= ADC_N_CHANNELS)
        return 0;
    return latest_samples[channel];
}

/* ===================================================================== */
/*  DRDY interrupt handler (EXTI15)                                       */
/*  Called when ADS1256 DRDY goes low → new conversion available.        */
/*  Reads the 24-bit data and advances to the next channel.               */
/* ===================================================================== */

/* This would be defined in the startup vector table as EXTI15_10_IRQHandler */
void EXTI15_10_IRQHandler(void)
{
    /* Clear EXTI15 pending flag */
    volatile uint32_t *exti_pr1 = (volatile uint32_t *)(0x5800045CU);
    *exti_pr1 = (1U << 15);

    if (adc_dma_busy)
        return;

    /* Read data from ADS1256 (in continuous mode, just clock out 3 bytes) */
    int32_t result = 0;
    spi1_transfer(0xFF);  /* Dummy to clock RDATA response */
    uint8_t byte0 = spi1_transfer(0xFF);
    uint8_t byte1 = spi1_transfer(0xFF);
    uint8_t byte2 = spi1_transfer(0xFF);

    result = ((int32_t)byte0 << 16) | ((int32_t)byte1 << 8) | (int32_t)byte2;
    if (result & 0x800000)
        result |= 0xFF000000;

    latest_samples[current_channel] = result;

    /* Feed to lock-in */
    lockin_feed_sample(current_channel, result);

    /* Advance to next channel (multiplexer cycling) */
    current_channel = (current_channel + 1) % ADC_N_CHANNELS;

    /* Set new MUX and SYNC for next conversion */
    /* In continuous mode, we need to issue WREG MUX + SYNC each cycle */
    if (current_channel < ADC_N_CHANNELS) {
        uint8_t mux_val = (current_channel << 4) | 0x08U;
        /* Quick MUX write (in RDATAC mode, WREG works) */
        spi1_transfer(ADS1256_CMD_SDATAC);  /* Must stop RDATAC first */
        delay_us(5);
        ads_write_reg(ADS1256_REG_MUX, mux_val);
        ads_cmd(ADS1256_CMD_SYNC);
        spi1_transfer(ADS1256_CMD_RDATAC);  /* Resume continuous read */
    }
}