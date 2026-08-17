/*
 * sdlog.c — SD Card Data Logging Driver Implementation
 *
 * LignoScan — Portable Acoustic Tomography Scanner
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * Logs raw scan data (ToF matrices, amplitudes, GPS, metadata) to a
 * MicroSD card via SPI. Uses a simple sector-based file system:
 * each scan is stored as a sequence of 512-byte blocks with a
 * CSV-like header for human readability when mounted on a PC.
 */

#include "sdlog.h"
#include "board.h"
#include <string.h>
#include <stdio.h>

static uint32_t next_block = 1;  /* Next free block (0 = reserved for TOC) */
static int sd_initialized = 0;

/* ---- SPI2 transfer for SD card ---- */
static uint8_t sd_spi_xfer(uint8_t tx) {
    while (!(SD_SPI->SR & SPI_SR_TXP)) { }
    *(volatile uint8_t *)&SD_SPI->TXDR = tx;
    while (!(SD_SPI->SR & SPI_SR_RXP)) { }
    return *(volatile uint8_t *)&SD_SPI->RXDR;
}

/* ---- SD card CS control ---- */
static void sd_cs_low(void) { GPIO_CLR(SD_CS, SD_CS_PIN); }
static void sd_cs_high(void) { GPIO_SET(SD_CS, SD_CS_PIN); }

/* ---- Send SD command ---- */
static int sd_send_cmd(uint8_t cmd, uint32_t arg, uint8_t crc) {
    sd_spi_xfer(0xFF);  /* Dummy */
    sd_spi_xfer(cmd | 0x40);
    sd_spi_xfer((arg >> 24) & 0xFF);
    sd_spi_xfer((arg >> 16) & 0xFF);
    sd_spi_xfer((arg >>  8) & 0xFF);
    sd_spi_xfer((arg >>  0) & 0xFF);
    sd_spi_xfer(crc | 0x01);

    /* Wait for response (0xFF = idle, 0x00 = OK) */
    uint8_t resp;
    int retries = 0;
    do {
        resp = sd_spi_xfer(0xFF);
        retries++;
    } while (resp == 0xFF && retries < 100);

    return resp;
}

/* ---- Initialize SD card (SPI mode) ---- */
int sd_card_init(void) {
    sd_cs_high();

    /* Send 80 dummy clocks to enter SPI mode */
    for (int i = 0; i < 10; i++) {
        sd_spi_xfer(0xFF);
    }

    sd_cs_low();

    /* CMD0: Reset / enter idle state */
    int resp = sd_send_cmd(0, 0, 0x94);
    if (resp != 0x01) {
        sd_cs_high();
        return -1;  /* No card or not in idle */
    }

    /* CMD8: Check voltage range (SD v2) */
    resp = sd_send_cmd(8, 0x000001AA, 0x86);
    if (resp == 0x01) {
        /* SD v2 — read OCR response */
        for (int i = 0; i < 4; i++) sd_spi_xfer(0xFF);

        /* ACMD41: Initialize (HCS = 1 for SDHC) */
        int retries = 0;
        do {
            sd_send_cmd(55, 0, 0);  /* CMD55 */
            resp = sd_send_cmd(41, 0x40000000, 0);  /* ACMD41 with HCS */
            retries++;
        } while (resp != 0x00 && retries < 100);
    } else {
        /* SD v1 — use CMD1 */
        int retries = 0;
        do {
            resp = sd_send_cmd(1, 0, 0);
            retries++;
        } while (resp != 0x00 && retries < 100);
    }

    sd_cs_high();
    sd_spi_xfer(0xFF);  /* Dummy */

    if (resp != 0x00) return -2;

    sd_initialized = 1;
    return 0;
}

/* ---- Read a single 512-byte block ---- */
int sd_read_block(uint32_t block, uint8_t *buf) {
    if (!sd_initialized) return -1;

    sd_cs_low();

    /* CMD17: Read single block */
    int resp = sd_send_cmd(17, block, 0);
    if (resp != 0x00) {
        sd_cs_high();
        return -1;
    }

    /* Wait for data start token (0xFE) */
    int retries = 0;
    uint8_t token;
    do {
        token = sd_spi_xfer(0xFF);
        retries++;
    } while (token != 0xFE && retries < 1000);

    if (token != 0xFE) {
        sd_cs_high();
        return -1;
    }

    /* Read 512 bytes */
    for (int i = 0; i < SD_BLOCK_SIZE; i++) {
        buf[i] = sd_spi_xfer(0xFF);
    }

    /* Read and discard CRC (2 bytes) */
    sd_spi_xfer(0xFF);
    sd_spi_xfer(0xFF);

    sd_cs_high();
    return 0;
}

/* ---- Write a single 512-byte block ---- */
int sd_write_block(uint32_t block, const uint8_t *buf) {
    if (!sd_initialized) return -1;

    sd_cs_low();

    /* CMD24: Write single block */
    int resp = sd_send_cmd(24, block, 0);
    if (resp != 0x00) {
        sd_cs_high();
        return -1;
    }

    /* Send data start token */
    sd_spi_xfer(0xFE);

    /* Write 512 bytes */
    for (int i = 0; i < SD_BLOCK_SIZE; i++) {
        sd_spi_xfer(buf[i]);
    }

    /* Send dummy CRC */
    sd_spi_xfer(0xFF);
    sd_spi_xfer(0xFF);

    /* Check data response token */
    uint8_t token = sd_spi_xfer(0xFF);
    if ((token & 0x1F) != 0x05) {
        sd_cs_high();
        return -1;  /* Write error */
    }

    /* Wait for write completion (card returns 0x00 while busy) */
    int retries = 0;
    while (sd_spi_xfer(0xFF) == 0x00 && retries < 1000) {
        retries++;
    }

    sd_cs_high();
    return 0;
}

/* ---- Initialize SD logging ---- */
void sdlog_init(void) {
    /* Enable SPI2 clock */
    RCC_APB1LENR |= RCC_APB1LENR_SPI2EN;

    /* Configure SPI2: Master, 8-bit, CPOL=1, CPHA=1 (mode 3 for SD) */
    SD_SPI->CR1 &= ~SPI_CR1_SPE;
    SD_SPI->CFG1 = (5U << SPI_CFG1_MBR_SHIFT) |  /* Baud /64 = ~2 MHz init */
                   (7U << SPI_CFG1_DSIZE_SHIFT) |
                   SPI_CFG1_MASTER;
    SD_SPI->CFG2 = (1U << 31) | (1U << 30);  /* CPOL=1, CPHA=1 */
    SD_SPI->CR1 |= SPI_CR1_SPE;

    /* Initialize SD card */
    if (sd_card_init() == 0) {
        /* Increase SPI speed for data transfer */
        SD_SPI->CR1 &= ~SPI_CR1_SPE;
        SD_SPI->CFG1 = (1U << SPI_CFG1_MBR_SHIFT) |  /* Baud /4 = ~35 MHz */
                       (7U << SPI_CFG1_DSIZE_SHIFT) |
                       SPI_CFG1_MASTER;
        SD_SPI->CR1 |= SPI_CR1_SPE;

        /* Read TOC from block 0 to find next free block */
        uint8_t toc[SD_BLOCK_SIZE];
        if (sd_read_block(0, toc) == 0) {
            /* TOC format: first 4 bytes = next free block address */
            next_block = ((uint32_t)toc[0] << 24) | ((uint32_t)toc[1] << 16) |
                         ((uint32_t)toc[2] << 8) | (uint32_t)toc[3];
            if (next_block == 0 || next_block > 0xFFFFFFFF) {
                next_block = 1;
            }
        }
    }
}

/* ---- Write scan header as CSV comment block ---- */
int sdlog_write_header(const char *tree_id, const char *timestamp,
                       gps_fix_t *gps, int n_sensors, float diameter) {
    uint8_t block[SD_BLOCK_SIZE];
    memset(block, 0, SD_BLOCK_SIZE);

    /* Format as CSV header */
    int offset = 0;
    offset += snprintf((char *)&block[offset], SD_BLOCK_SIZE - offset,
        "# LignoScan Raw Data — %s\n"
        "# Author: jayis1\n"
        "# Tree_ID: %s, GPS: %.6f,%.6f\n"
        "# Sensors: %d, Diameter: %.1fcm\n"
        "TX,RX,ToF_ns,Amplitude_mV,Quality\n",
        timestamp, tree_id,
        gps->latitude, gps->longitude,
        n_sensors, diameter);

    (void)offset;

    /* Write header block */
    int ret = sd_write_block(next_block++, block);
    return ret;
}

/* ---- Write ToF matrix data as CSV rows ---- */
int sdlog_write_matrix(float *tof, float *amp, int *qual, int n) {
    uint8_t block[SD_BLOCK_SIZE];
    memset(block, 0, SD_BLOCK_SIZE);

    int offset = 0;
    int blocks_written = 0;

    for (int tx = 0; tx < n; tx++) {
        for (int rx = 0; rx < n; rx++) {
            if (tx == rx) continue;

            /* Quality string */
            const char *qstr;
            switch (qual[tx * n + rx]) {
            case 1: qstr = "GOOD"; break;
            case 2: qstr = "MARGINAL"; break;
            case 3: qstr = "POOR"; break;
            case 4: qstr = "NO_SIGNAL"; break;
            default: qstr = "SKIP"; break;
            }

            offset += snprintf((char *)&block[offset], SD_BLOCK_SIZE - offset,
                "%d,%d,%.0f,%.0f,%s\n",
                tx, rx, tof[tx * n + rx], amp[tx * n + rx], qstr);

            /* Write block if nearly full */
            if (offset > SD_BLOCK_SIZE - 40) {
                sd_write_block(next_block++, block);
                blocks_written++;
                memset(block, 0, SD_BLOCK_SIZE);
                offset = 0;
            }
        }
    }

    /* Write final block if it has data */
    if (offset > 0) {
        sd_write_block(next_block++, block);
        blocks_written++;
    }

    /* Update TOC */
    uint8_t toc[SD_BLOCK_SIZE];
    memset(toc, 0, SD_BLOCK_SIZE);
    toc[0] = (next_block >> 24) & 0xFF;
    toc[1] = (next_block >> 16) & 0xFF;
    toc[2] = (next_block >> 8) & 0xFF;
    toc[3] = (next_block >> 0) & 0xFF;
    sd_write_block(0, toc);

    return blocks_written;
}

/* ---- Write a complete scan to SD card ---- */
int sdlog_write_scan(const char *tree_id, const char *timestamp,
                     gps_fix_t *gps, int n_sensors, float diameter,
                     float *tof_matrix, float *amplitude, int *quality) {
    if (!sd_initialized) return -1;

    int ret1 = sdlog_write_header(tree_id, timestamp, gps, n_sensors, diameter);
    int ret2 = sdlog_write_matrix(tof_matrix, amplitude, quality, n_sensors);

    if (ret1 != 0 || ret2 < 0) return -1;
    return 0;
}

/* EOF — sdlog.c
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */