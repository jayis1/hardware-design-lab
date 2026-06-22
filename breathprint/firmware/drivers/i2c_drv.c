/*
 * i2c_drv.c — I2C driver with DMA support for multi-sensor bus
 * BreathPrint — Portable Breath VOC Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 *
 * Manages the I2C1 bus with 7 devices (BME688×2, SCD41, DGS×3, BMP390).
 * Uses polling mode for simplicity; DMA mode for multi-byte reads.
 */

#include "i2c_drv.h"
#include "../board.h"

/* ========================================================================
 * I2C Timing Presets
 * ======================================================================== */

/* For 400 kHz at 96 MHz PCLK (APB1) */
static const uint32_t i2c_timing_400k = 0x10A13E56U;
/* For 100 kHz at 96 MHz PCLK */
static const uint32_t i2c_timing_100k = 0x10909CECUL;

/* ========================================================================
 * Initialize I2C peripheral
 * ======================================================================== */

void i2c_init(i2c_reg_t *i2c, uint32_t timing) {
    /* Disable peripheral */
    i2c->CR1 &= ~I2C_CR1_PE;

    /* Configure timing */
    i2c->TIMINGR = timing;

    /* Configure: No general call, 7-bit addressing, digital noise filter */
    i2c->OAR1 = 0;  /* We are always master */
    i2c->OAR2 = 0;
    i2c->CR1 = I2C_CR1_PE;  /* Enable peripheral */
}

/* ========================================================================
 * Wait for flag with timeout
 * ======================================================================== */

static int i2c_wait_flag(i2c_reg_t *i2c, uint32_t flag, uint8_t set,
                         uint32_t timeout_ms) {
    uint32_t start = millis();
    while (set ? !(i2c->ISR & flag) : (i2c->ISR & flag)) {
        if (millis() - start > timeout_ms) return -1;
    }
    return 0;
}

/* ========================================================================
 * Master write (blocking)
 * ======================================================================== */

int i2c_write(i2c_reg_t *i2c, uint8_t addr, const uint8_t *data,
              uint16_t len, uint8_t stop) {
    if (len == 0) return -1;

    /* Wait for bus to be free */
    if (i2c_wait_flag(i2c, I2C_ISR_BUSY, 0, 100) < 0) return -1;

    /* Configure transfer: write, N bytes, auto-end if stop requested */
    uint32_t cr2 = ((uint32_t)addr << 1) << I2C_CR2_SADD_SHIFT;
    cr2 |= ((uint32_t)len << I2C_CR2_NBYTES_SHIFT);
    if (stop) cr2 |= I2C_CR2_AUTOEND;
    else cr2 |= I2C_CR2_RELOAD;

    /* Start transfer */
    i2c->CR2 = cr2 | I2C_CR2_START;

    /* Send each byte */
    for (uint16_t i = 0; i < len; i++) {
        if (i2c_wait_flag(i2c, I2C_ISR_TXIS, 1, 100) < 0) {
            i2c->CR2 |= I2C_CR2_STOP;
            return -1;
        }
        i2c->TXDR = data[i];
    }

    /* Wait for transfer complete or auto-end */
    if (stop) {
        if (i2c_wait_flag(i2c, I2C_ISR_STOPF, 1, 200) < 0) {
            i2c->CR2 |= I2C_CR2_STOP;
            return -1;
        }
        i2c->ICR = I2C_ICR_STOPCF;
    } else {
        if (i2c_wait_flag(i2c, I2C_ISR_TCR, 1, 200) < 0) return -1;
    }

    return 0;
}

/* ========================================================================
 * Master read (blocking)
 * ======================================================================== */

int i2c_read(i2c_reg_t *i2c, uint8_t addr, uint8_t *data,
             uint16_t len, uint8_t stop) {
    if (len == 0) return -1;

    /* Wait for bus to be free */
    if (i2c_wait_flag(i2c, I2C_ISR_BUSY, 0, 100) < 0) return -1;

    /* Configure transfer: read, N bytes, auto-end */
    uint32_t cr2 = ((uint32_t)addr << 1) << I2C_CR2_SADD_SHIFT;
    cr2 |= I2C_CR2_RD_WRN;  /* Read direction */
    cr2 |= ((uint32_t)len << I2C_CR2_NBYTES_SHIFT);
    if (stop) cr2 |= I2C_CR2_AUTOEND;
    else cr2 |= I2C_CR2_RELOAD;

    /* Start transfer */
    i2c->CR2 = cr2 | I2C_CR2_START;

    /* Read each byte */
    for (uint16_t i = 0; i < len; i++) {
        if (i2c_wait_flag(i2c, I2C_ISR_RXNE, 1, 200) < 0) {
            i2c->CR2 |= I2C_CR2_STOP;
            return -1;
        }
        data[i] = (uint8_t)i2c->RXDR;
    }

    /* Wait for stop */
    if (stop) {
        if (i2c_wait_flag(i2c, I2C_ISR_STOPF, 1, 200) < 0) return -1;
        i2c->ICR = I2C_ICR_STOPCF;
    }

    return 0;
}

/* ========================================================================
 * Write register (single byte)
 * ======================================================================== */

int i2c_write_reg(i2c_reg_t *i2c, uint8_t addr, uint8_t reg, uint8_t val) {
    uint8_t buf[2] = { reg, val };
    return i2c_write(i2c, addr, buf, 2, 1);
}

/* ========================================================================
 * Write 16-bit register (big-endian, for SCD41)
 * ======================================================================== */

int i2c_write_reg16(i2c_reg_t *i2c, uint8_t addr, uint16_t reg) {
    uint8_t buf[2] = { (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF) };
    return i2c_write(i2c, addr, buf, 2, 1);
}

/* ========================================================================
 * Read register (multi-byte)
 * ======================================================================== */

int i2c_read_reg(i2c_reg_t *i2c, uint8_t addr, uint8_t reg,
                 uint8_t *data, uint16_t len) {
    /* Write register address (no stop) */
    if (i2c_write(i2c, addr, &reg, 1, 0) < 0) return -1;
    /* Restart and read */
    return i2c_read(i2c, addr, data, len, 1);
}

/* ========================================================================
 * Read 16-bit register (big-endian, for SCD41 command-based reads)
 * ======================================================================== */

int i2c_read_cmd16(i2c_reg_t *i2c, uint8_t addr, uint16_t cmd,
                   uint8_t *data, uint16_t len) {
    /* Send command */
    if (i2c_write_reg16(i2c, addr, cmd) < 0) return -1;
    /* Read response */
    return i2c_read(i2c, addr, data, len, 1);
}

/* ========================================================================
 * CRC-8 checksum (for SCD41 and Spec DGS sensors)
 * Polynomial: 0x31, Initial: 0xFF
 * ======================================================================== */

uint8_t i2c_crc8(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0xFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

/* ========================================================================
 * Bus scan — detect all devices on the bus
 * ======================================================================== */

int i2c_scan(i2c_reg_t *i2c, uint8_t *found_addrs, uint8_t max_addrs) {
    int count = 0;
    for (uint8_t addr = 1; addr < 128; addr++) {
        /* Skip reserved addresses */
        if (addr < 0x08 || addr > 0x77) continue;

        /* Quick write of 0 bytes to check ACK */
        i2c->CR2 = ((uint32_t)addr << 1) << I2C_CR2_SADD_SHIFT;
        i2c->CR2 |= I2C_CR2_START;

        /* Check for NACK */
        uint32_t start = millis();
        int ack = 0;
        while (millis() - start < 5) {
            if (i2c->ISR & I2C_ISR_NACKF) {
                ack = 0;
                break;
            }
            if (i2c->ISR & (I2C_ISR_ADDR | I2C_ISR_TXIS)) {
                ack = 1;
                break;
            }
        }

        /* Send stop */
        i2c->CR2 |= I2C_CR2_STOP;
        i2c->ICR = I2C_ICR_NACKCF | I2C_ICR_STOPCF;

        if (ack && count < max_addrs) {
            found_addrs[count++] = addr;
        }
    }
    return count;
}

/* ========================================================================
 * Error recovery — reset bus if stuck
 * ======================================================================== */

void i2c_reset(i2c_reg_t *i2c) {
    /* Disable peripheral */
    i2c->CR1 &= ~I2C_CR1_PE;
    delay_ms(10);

    /* Clear all flags */
    i2c->ICR = 0x3F38;

    /* Re-enable */
    i2c->CR1 = I2C_CR1_PE;
    delay_ms(1);
}

/* ========================================================================
 * DMA-based read (for continuous sensor streaming)
 * ======================================================================== */

int i2c_read_dma(i2c_reg_t *i2c, uint8_t addr, uint8_t *data,
                 uint16_t len, dma_stream_reg_t *dma) {
    /* Configure DMA stream for peripheral-to-memory */
    dma->CR = DMA_CR_MINC | DMA_CR_PSIZE_8BIT | DMA_CR_MSIZE_8BIT |
              DMA_CR_DIR_P2M | DMA_CR_TCIE;
    dma->NDTR = len;
    dma->PAR = (uint32_t)&i2c->RXDR;
    dma->M0AR = (uint32_t)data;
    dma->CR |= DMA_CR_EN;

    /* Configure I2C transfer with DMA */
    i2c->CR1 |= I2C_CR1_RXIE;

    uint32_t cr2 = ((uint32_t)addr << 1) << I2C_CR2_SADD_SHIFT;
    cr2 |= I2C_CR2_RD_WRN;
    cr2 |= ((uint32_t)len << I2C_CR2_NBYTES_SHIFT);
    cr2 |= I2C_CR2_AUTOEND;
    i2c->CR2 = cr2 | I2C_CR2_START;

    /* Wait for DMA completion */
    uint32_t start = millis();
    while (!(DMA1->LISR & (1U << 5))) {  /* TCIF for stream 1 */
        if (millis() - start > 500) {
            dma->CR &= ~DMA_CR_EN;
            return -1;
        }
    }

    /* Clear DMA interrupt flag */
    DMA1->LIFCR = (1U << 5);
    dma->CR &= ~DMA_CR_EN;

    return 0;
}