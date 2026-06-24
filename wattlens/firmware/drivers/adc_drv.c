/*
 * adc_drv.c — ADS131M08 8-channel 24-bit ADC driver
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 *
 * The ADS131M08 is a simultaneous-sampling 24-bit delta-sigma ADC
 * communicating over SPI.  It asserts DRDY at the configured sample rate
 * (2048 Hz).  Each DRDY triggers a DMA SPI read of 8 channels × 3 bytes = 24 bytes.
 *
 * The driver sets up:
 *   - SPI2 in master mode, 20 MHz clock, 24-bit frames
 *   - DMA2 Stream 0 (RX) for autonomous sample transfer
 *   - GPIO PC4 (RESET) and PE1 (PWDN/SYNC) for ADC control
 */

#include "adc_drv.h"
#include "registers.h"
#include "board.h"
#include <string.h>

/* SPI2 register accessors */
#define SPI2_CR1   SPI_REG(SPI2_BASE, SPI_CR1)
#define SPI2_CR2   SPI_REG(SPI2_BASE, SPI_CR2)
#define SPI2_CFG1  SPI_REG(SPI2_BASE, SPI_CFG1)
#define SPI2_CFG2  SPI_REG(SPI2_BASE, SPI_CFG2)
#define SPI2_SR    SPI_REG(SPI2_BASE, SPI_SR)
#define SPI2_IFCR  SPI_REG(SPI2_BASE, SPI_IFCR)

/* DMA stream for SPI2 RX (DMA2 Stream 0) */
#define SPI2_DMA_RX  DMA_STREAM(DMA2_BASE, 0)

/* ADC CS pin: PB12 */
#define ADC_CS_HIGH()  (GPIO_REG(GPIOB_BASE, GPIO_BSRR_OFF) = BIT(28))
#define ADC_CS_LOW()   (GPIO_REG(GPIOB_BASE, GPIO_BSRR_OFF) = BIT(12))
#define ADC_RESET_HIGH() (GPIO_REG(GPIOC_BASE, GPIO_BSRR_OFF) = BIT(4))
#define ADC_RESET_LOW()  (GPIO_REG(GPIOC_BASE, GPIO_BSRR_OFF) = BIT(36))
#define ADC_PWDN_HIGH() (GPIO_REG(GPIOE_BASE, GPIO_BSRR_OFF) = BIT(1))
#define ADC_PWDN_LOW()  (GPIO_REG(GPIOE_BASE, GPIO_BSRR_OFF) = BIT(33))

static volatile uint8_t *dma_dest;
static volatile uint16_t dma_count;

/* ========================================================================
 * SPI2 initialization (master, 20 MHz, 8-bit frames)
 * ======================================================================== */
static void spi2_init(void) {
    /* Disable SPI */
    SPI2_CR1 = 0;

    /* Configuration: master, no slave management hardware, Motorola mode */
    SPI2_CFG2 = (1 << 2)    /* MASTER */
              | (0 << 0);   /* MSTR — master mode */

    /* CFG1: baud rate = APB1 / 12 = 120 MHz / 12 = 10 MHz (safe for ADS131M08) */
    SPI2_CFG1 = (11 << 0)   /* MBR = div 12 */
              | (7 << 5);   /* FTHLV = 8-byte FIFO threshold */

    /* Configure CR2 for 8-bit data size and basic operation */
    SPI2_CR2 = (7 << 0)     /* DSIZE = 8 bits (7+1) */
             | (1 << 6);    /* TXDR — not used */

    /* Enable SPI */
    SPI2_CR1 |= BIT(0);     /* SPE */
}

/* ========================================================================
 * DMA2 Stream 0 configuration for SPI2 RX
 * ======================================================================== */
static void dma_spi2_rx_init(void) {
    /* Disable stream */
    SPI2_DMA_RX->CR = 0;

    /* Clear DMA flags for stream 0 (LIFCR bits 0-4) */
    DMA_LIFCR = 0x0000003F;

    /* Configure: peripheral=SPI2_RXDR, memory=buffer, 24 bytes (3 per channel × 8) */
    SPI2_DMA_RX->PAR  = SPI2_BASE + SPI_RXDR;
    SPI2_DMA_RX->M0AR = 0;  /* set per transfer */
    SPI2_DMA_RX->NDTR = NUM_CHANNELS * 3;  /* 24 bytes */
    SPI2_DMA_RX->FCR  = 0;  /* direct mode, no FIFO */

    /* CR: MINC (memory increment), DIR=peripheral-to-memory, 8-bit, TCIE */
    SPI2_DMA_RX->CR = (0 << 6)   /* DIR: P2M */
                    | BIT(10)     /* MINC: memory increment */
                    | (0 << 11)   /* MSIZE: 8-bit */
                    | (0 << 8)    /* PSIZE: 8-bit */
                    | BIT(4)      /* TCIE: transfer complete interrupt */
                    | (4 << 25);  /* CHSEL: channel 4 (SPI2_RX on DMA2) */
}

/* ========================================================================
 * Write to an ADS131M08 register
 * ======================================================================== */
int adc_write_register(uint8_t reg, uint16_t val) {
    uint16_t cmd = ADS_CMD_WRITE | ((reg & 0x1F) << 7);
    uint16_t cmd_inv = ~cmd;

    ADC_CS_LOW();

    /* Send 3 words: command, inverted command, data */
    /* (Simplified — full SPI transfer with dummy reads) */
    SPI2_CFG1 &= ~(7 << 5);  /* flush FIFO */
    while (!(SPI2_SR & BIT(1))) { }  /* wait TXP */
    *(volatile uint8_t *)(SPI2_BASE + SPI_TXDR) = (cmd >> 8) & 0xFF;
    *(volatile uint8_t *)(SPI2_BASE + SPI_TXDR) = cmd & 0xFF;
    *(volatile uint8_t *)(SPI2_BASE + SPI_TXDR) = (cmd_inv >> 8) & 0xFF;
    *(volatile uint8_t *)(SPI2_BASE + SPI_TXDR) = cmd_inv & 0xFF;
    *(volatile uint8_t *)(SPI2_BASE + SPI_TXDR) = (val >> 8) & 0xFF;
    *(volatile uint8_t *)(SPI2_BASE + SPI_TXDR) = val & 0xFF;

    /* Wait for completion */
    while (SPI2_SR & BIT(13)) { }  /* wait until RXFIFO drained */
    SPI2_IFCR = 0xFFFFFFFF;

    ADC_CS_HIGH();
    return 0;
}

/* ========================================================================
 * Read from an ADS131M08 register
 * ======================================================================== */
int adc_read_register(uint8_t reg, uint16_t *val) {
    uint16_t cmd = ADS_CMD_READ | ((reg & 0x1F) << 7);
    uint16_t cmd_inv = ~cmd;

    ADC_CS_LOW();
    /* Send command + inverted command, then read response */
    *(volatile uint8_t *)(SPI2_BASE + SPI_TXDR) = (cmd >> 8) & 0xFF;
    *(volatile uint8_t *)(SPI2_BASE + SPI_TXDR) = cmd & 0xFF;
    *(volatile uint8_t *)(SPI2_BASE + SPI_TXDR) = (cmd_inv >> 8) & 0xFF;
    *(volatile uint8_t *)(SPI2_BASE + SPI_TXDR) = cmd_inv & 0xFF;
    /* Dummy bytes to clock out response */
    *(volatile uint8_t *)(SPI2_BASE + SPI_TXDR) = 0;
    *(volatile uint8_t *)(SPI2_BASE + SPI_TXDR) = 0;
    *(volatile uint8_t *)(SPI2_BASE + SPI_TXDR) = 0;
    *(volatile uint8_t *)(SPI2_BASE + SPI_TXDR) = 0;

    while (SPI2_SR & BIT(13)) { }
    SPI2_IFCR = 0xFFFFFFFF;
    ADC_CS_HIGH();

    /* Parse response (simplified) */
    *val = 0;
    return 0;
}

/* ========================================================================
 * Initialize the ADS131M08
 * ======================================================================== */
void adc_drv_init(void) {
    /* Hardware reset */
    ADC_PWDN_LOW();
    ADC_RESET_LOW();
    for (volatile int i = 0; i < 100000; i++) { }
    ADC_RESET_HIGH();
    ADC_PWDN_HIGH();
    for (volatile int i = 0; i < 100000; i++) { }

    spi2_init();
    dma_spi2_rx_init();

    /* Enable DMA interrupt in NVIC */
    NVIC_IP(56) = 1;  /* high priority for ADC DMA */
    NVIC_ISER(1) |= BIT(56 - 32);

    /* --- Configure ADS131M08 registers --- */
    /* Reset command */
    adc_write_register(ADS_REG_CLOCK, 0x0F);  /* enable clock */

    /* CONFIG1: enable all 8 channels */
    adc_write_register(ADS_REG_CONFIG1, 0xFF00 | 0x00FF);

    /* OSR: set to 2048 samples/sec (OSR=256 for 2.048 kHz at 4 MHz modulator) */
    adc_write_register(ADS_REG_OSR, 0x0100);  /* OSR = 256 */

    /* GAIN: PGA gain = 1 for all channels (voltage already scaled) */
    adc_write_register(ADS_REG_GAIN, 0x0000);

    /* CHANNEL: enable all channels */
    adc_write_register(ADS_REG_CHANNEL, 0x00FF);
}

/* ========================================================================
 * Start continuous sampling
 * ======================================================================== */
void adc_drv_start(void) {
    /* Enable EXTI5 (DRDY) interrupt */
    EXTI_IMR1 |= BIT(5);
}

/* ========================================================================
 * Stop sampling
 * ======================================================================== */
void adc_drv_stop(void) {
    EXTI_IMR1 &= ~BIT(5);
}

/* ========================================================================
 * Start a DMA SPI read of one ADC frame (24 bytes)
 * Called from EXTI9_5 ISR on each DRDY falling edge.
 * ======================================================================== */
void adc_start_frame_read(uint8_t *dest) {
    ADC_CS_LOW();

    /* Configure DMA transfer */
    SPI2_DMA_RX->CR = 0;  /* disable */
    DMA_LIFCR = 0x0000003F;  /* clear flags */

    SPI2_DMA_RX->M0AR = (uint32_t)dest;
    SPI2_DMA_RX->NDTR = NUM_CHANNELS * 3;  /* 24 bytes */
    SPI2_DMA_RX->CR = (0 << 6)    /* DIR: P2M */
                    | BIT(10)     /* MINC */
                    | (0 << 11)   /* MSIZE: byte */
                    | (0 << 8)    /* PSIZE: byte */
                    | (4 << 25);  /* CHSEL: SPI2_RX */

    /* Enable DMA on SPI2 RX */
    SPI2_CFG1 |= BIT(0);  /* RXDMAEN */

    /* Enable DMA stream */
    SPI2_DMA_RX->CR |= BIT(0);  /* EN */

    /* CS is raised in the DMA TC interrupt (not shown for brevity) */
}

/* ========================================================================
 * DMA2 Stream 0 interrupt handler (SPI2 RX complete)
 * ======================================================================== */
void DMA2_Stream0_IRQHandler(void) {
    if (DMA_LISR & BIT(5)) {  /* TCIF0 */
        DMA_LIFCR = BIT(5);   /* clear */

        /* Raise ADC CS */
        ADC_CS_HIGH();

        /* Disable DMA */
        SPI2_DMA_RX->CR = 0;
        SPI2_CFG1 &= ~BIT(0);  /* disable RXDMAEN */
    }
}