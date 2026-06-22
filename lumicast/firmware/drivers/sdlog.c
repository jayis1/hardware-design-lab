/*
 * sdlog.c — microSD card logging driver
 * LumiCast — Open-Source Circadian Light & SPD Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Implements SPI-mode microSD card access for logging spectral data to
 * a FAT-like flat file structure on the SD card.  Uses a simple append-
 * only scheme: each session opens a new block-aligned region and writes
 * fixed-size records sequentially.
 */

#include "sdlog.h"
#include "timer_drv.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

static bool card_present = false;
static uint32_t file_start_block = 0;
static uint32_t file_write_offset = 0;
static uint8_t  block_buf[SD_BLOCK_SIZE];
static uint16_t buf_pos = 0;

/* ------------------------------------------------------------------ */
/*  SPI bit-bang helpers                                               */
/* ------------------------------------------------------------------ */

static void spi_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

    /* PA5=SCK, PA6=MISO, PA7=MOSI, AF5. */
    GPIOA->MODER &= ~((3U << (5U*2U)) | (3U << (6U*2U)) | (3U << (7U*2U)));
    GPIOA->MODER |= (GPIO_MODE_AF << (5U*2U)) | (GPIO_MODE_AF << (6U*2U)) | (GPIO_MODE_AF << (7U*2U));
    GPIOA->AFRL &= ~((0xFU << (5U*4U)) | (0xFU << (6U*4U)) | (0xFU << (7U*4U)));
    GPIOA->AFRL |= (5U << (5U*4U)) | (5U << (6U*4U)) | (5U << (7U*4U));

    /* PA10 = CS, push-pull output. */
    GPIOA->MODER &= ~(3U << (10U*2U));
    GPIOA->MODER |= (GPIO_MODE_OUTPUT << (10U*2U));
    SD_CS_HIGH();

    /* SPI1: Master, CPOL=0, CPHA=0, fPCLK/4 = 16 MHz, 8-bit. */
    SPI1->CR1 = 0;
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | (1U << SPI_CR1_BR_Pos);
    SPI1->CR2 = SPI_CR2_FRXTH | (SPI_CR2_DS_8BIT << SPI_CR2_DS_Pos);
    SPI1->CR1 |= SPI_CR1_SPE;
}

static uint8_t spi_xfer(uint8_t tx)
{
    while (!(SPI1->SR & SPI_SR_TXE));
    *((volatile uint8_t *)&SPI1->DR) = tx;
    while (!(SPI1->SR & SPI_SR_RXNE));
    return *((volatile uint8_t *)&SPI1->DR);
}

static void spi_clock_delay(void)
{
    for (volatile int i = 0; i < 8; i++);
}

/* ------------------------------------------------------------------ */
/*  SD card SPI protocol                                                */
/* ------------------------------------------------------------------ */

static uint8_t sd_wait_ready(void)
{
    uint32_t t = 0;
    uint8_t r;
    do {
        r = spi_xfer(0xFF);
        if (t++ > 100000) return 0;
    } while (r != 0xFF);
    return r;
}

static int sd_send_cmd(uint8_t cmd, uint32_t arg, uint8_t crc)
{
    uint8_t r1, i;
    sd_wait_ready();
    spi_xfer(0x40 | cmd);
    spi_xfer((arg >> 24) & 0xFF);
    spi_xfer((arg >> 16) & 0xFF);
    spi_xfer((arg >> 8) & 0xFF);
    spi_xfer(arg & 0xFF);
    spi_xfer(crc | 1);

    /* Wait for response (up to 10 bytes). */
    for (i = 0; i < 10; i++) {
        r1 = spi_xfer(0xFF);
        if (!(r1 & 0x80)) return r1;
    }
    return -1;
}

int sdlog_init(void)
{
    int i;
    uint8_t r1;

    spi_init();

    /* Send 80 dummy clocks with CS high to enter SPI mode. */
    SD_CS_HIGH();
    for (i = 0; i < 10; i++) spi_xfer(0xFF);

    /* CMD0: GO_IDLE_STATE. */
    SD_CS_LOW();
    r1 = sd_send_cmd(0, 0, 0x94);
    if (r1 != 0x01) {
        SD_CS_HIGH();
        return SD_ERR_INIT;
    }

    /* CMD8: SEND_IF_COND (check voltage). */
    r1 = sd_send_cmd(8, 0x000001AA, 0x87);
    if (r1 & 0x04) {
        /* SD v1.x — not supported in this driver. */
        SD_CS_HIGH();
        return SD_ERR_INIT;
    }
    /* Read R7 response (4 bytes). */
    for (i = 0; i < 4; i++) spi_xfer(0xFF);

    /* ACMD41: SD_SEND_OP_COND (HCS=1). */
    uint32_t t = 0;
    do {
        sd_send_cmd(55, 0, 0);
        r1 = sd_send_cmd(41, 0x40000000, 0);
        if (t++ > 50000) {
            SD_CS_HIGH();
            return SD_ERR_TIMEOUT;
        }
    } while (r1 != 0x00);

    /* CMD58: read OCR. */
    sd_send_cmd(58, 0, 0);
    for (i = 0; i < 4; i++) spi_xfer(0xFF);

    SD_CS_HIGH();
    spi_xfer(0xFF);

    card_present = true;
    file_start_block = 0;
    file_write_offset = 0;
    buf_pos = 0;
    return SD_OK;
}

bool sdlog_present(void) { return card_present; }

int sdlog_write_block(uint32_t block_addr, const uint8_t *data)
{
    int i;
    uint8_t r1;

    if (!card_present) return SD_ERR_NOCARD;

    SD_CS_LOW();
    r1 = sd_send_cmd(24, block_addr, 0);
    if (r1 != 0x00) {
        SD_CS_HIGH();
        return SD_ERR_WRITE;
    }

    spi_xfer(0xFE);  /* data token */
    for (i = 0; i < SD_BLOCK_SIZE; i++) spi_xfer(data[i]);
    spi_xfer(0xFF);  /* dummy CRC */
    spi_xfer(0xFF);

    r1 = spi_xfer(0xFF) & 0x1F;
    if (r1 != 0x05) {
        SD_CS_HIGH();
        return SD_ERR_WRITE;
    }

    sd_wait_ready();
    SD_CS_HIGH();
    spi_xfer(0xFF);
    return SD_OK;
}

int sdlog_read_block(uint32_t block_addr, uint8_t *data)
{
    int i;
    uint8_t r1;

    if (!card_present) return SD_ERR_NOCARD;

    SD_CS_LOW();
    r1 = sd_send_cmd(17, block_addr, 0);
    if (r1 != 0x00) {
        SD_CS_HIGH();
        return SD_ERR_TIMEOUT;
    }

    /* Wait for data token 0xFE. */
    uint32_t t = 0;
    while (spi_xfer(0xFF) != 0xFE) {
        if (t++ > 100000) {
            SD_CS_HIGH();
            return SD_ERR_TIMEOUT;
        }
    }

    for (i = 0; i < SD_BLOCK_SIZE; i++) data[i] = spi_xfer(0xFF);
    spi_xfer(0xFF);  /* CRC */
    spi_xfer(0xFF);

    SD_CS_HIGH();
    spi_xfer(0xFF);
    return SD_OK;
}

int sdlog_append(const uint8_t *data, uint32_t len)
{
    if (!card_present) return SD_ERR_NOCARD;

    while (len > 0) {
        uint32_t copy = SD_BLOCK_SIZE - buf_pos;
        if (copy > len) copy = len;
        memcpy(&block_buf[buf_pos], data, copy);
        buf_pos += copy;
        data += copy;
        len -= copy;

        if (buf_pos == SD_BLOCK_SIZE) {
            int rc = sdlog_write_block(file_start_block + file_write_offset, block_buf);
            if (rc != SD_OK) return rc;
            file_write_offset++;
            buf_pos = 0;
            memset(block_buf, 0, sizeof(block_buf));
        }
    }
    return SD_OK;
}

int sdlog_sync(void)
{
    int rc = SD_OK;
    if (buf_pos > 0) {
        rc = sdlog_write_block(file_start_block + file_write_offset, block_buf);
        if (rc == SD_OK) file_write_offset++;
        buf_pos = 0;
    }
    return rc;
}

uint32_t sdlog_get_file_size(void)
{
    return file_write_offset * SD_BLOCK_SIZE + buf_pos;
}