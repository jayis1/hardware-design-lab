/*
 * flash_drv.c — MX25R6435F 8MB SPI flash driver
 * TremorSense — Wearable Tremor Characterization Band
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * Driver for Macronix MX25R6435F SPI NOR flash (8 MB).
 * Provides page program, sector/block erase, read, power management,
 * and append-mode circular logging for tremor episode records.
 */

#include "flash_drv.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

static uint32_t write_pointer = 0;  /* Next write address in flash */
static uint32_t sector_write_count[128];  /* Track writes per 64KB sector */

/* ---- SPI low-level (shared with other SPI devices on different bus) ---- */
static void flash_cs_low(void) { /* gpio_set(FLASH_CS_PIN, 0); */ }
static void flash_cs_high(void) { /* gpio_set(FLASH_CS_PIN, 1); */ }

static void flash_spi_xfer(const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    /* In production: use nRF SPIM1 with DMA */
    (void)tx; (void)rx; (void)len;
}

/* ---- Basic commands ---- */
void flash_write_enable(void)
{
    uint8_t cmd = FLASH_CMD_WREN;
    flash_cs_low();
    flash_spi_xfer(&cmd, NULL, 1);
    flash_cs_high();
}

uint8_t flash_read_status(void)
{
    uint8_t cmd = FLASH_CMD_RDSR;
    uint8_t status = 0;
    flash_cs_low();
    flash_spi_xfer(&cmd, NULL, 1);
    flash_spi_xfer(NULL, &status, 1);
    flash_cs_high();
    return status;
}

void flash_wait_ready(void)
{
    /* Poll status register WIP bit with timeout */
    uint32_t timeout = 1000000;
    while ((flash_read_status() & FLASH_SR_WIP) && timeout--) {
        /* Spin wait */
    }
}

int flash_init(void)
{
    /* Wake from power-down if needed */
    flash_wakeup();

    /* Check JEDEC ID */
    uint32_t jedec = flash_read_jedec_id();
    if ((jedec & 0xFFFFFF) != (FLASH_MX25R6435F_JEDEC & 0xFFFFFF)) {
        return -1;
    }

    /* Clear write pointer to start of data area (after model weights) */
    /* Model weights occupy 0x000000 – 0x020000 (128 KB) */
    /* Episode data starts at 0x020000 */
    write_pointer = 0x020000;

    /* Clear sector wear counters */
    memset(sector_write_count, 0, sizeof(sector_write_count));

    /* Deassert WP and HOLD pins (drive high) */
    /* gpio_set(FLASH_WP_PIN, 1); */
    /* gpio_set(FLASH_HOLD_PIN, 1); */

    return 0;
}

uint32_t flash_read_jedec_id(void)
{
    uint8_t cmd = FLASH_CMD_RDID;
    uint8_t id[3] = {0, 0, 0};
    flash_cs_low();
    flash_spi_xfer(&cmd, NULL, 1);
    flash_spi_xfer(NULL, id, 3);
    flash_cs_high();
    return ((uint32_t)id[0] << 16) | ((uint32_t)id[1] << 8) | id[2];
}

int flash_read(uint32_t addr, uint8_t *buf, uint32_t len)
{
    if (addr + len > FLASH_TOTAL_SIZE) return -1;

    uint8_t cmd[4] = {
        FLASH_CMD_READ,
        (uint8_t)(addr >> 16),
        (uint8_t)(addr >> 8),
        (uint8_t)(addr)
    };

    flash_cs_low();
    flash_spi_xfer(cmd, NULL, 4);
    flash_spi_xfer(NULL, buf, len);
    flash_cs_high();

    return 0;
}

int flash_write_page(uint32_t addr, const uint8_t *data, uint16_t len)
{
    if (len > FLASH_PAGE_SIZE) len = FLASH_PAGE_SIZE;
    if (addr + len > FLASH_TOTAL_SIZE) return -1;

    flash_write_enable();

    uint8_t cmd[4] = {
        FLASH_CMD_PP,
        (uint8_t)(addr >> 16),
        (uint8_t)(addr >> 8),
        (uint8_t)(addr)
    };

    flash_cs_low();
    flash_spi_xfer(cmd, NULL, 4);
    flash_spi_xfer(data, NULL, len);
    flash_cs_high();

    flash_wait_ready();

    /* Track wear on this 64KB sector */
    uint32_t sector = addr / FLASH_BLOCK_SIZE;
    if (sector < 128) sector_write_count[sector]++;

    return 0;
}

int flash_erase_sector(uint32_t addr)
{
    flash_write_enable();

    uint8_t cmd[4] = {
        FLASH_CMD_SE,
        (uint8_t)(addr >> 16),
        (uint8_t)(addr >> 8),
        (uint8_t)(addr)
    };

    flash_cs_low();
    flash_spi_xfer(cmd, NULL, 4);
    flash_cs_high();

    flash_wait_ready();
    return 0;
}

int flash_erase_block64(uint32_t addr)
{
    flash_write_enable();

    uint8_t cmd[4] = {
        FLASH_CMD_BE64,
        (uint8_t)(addr >> 16),
        (uint8_t)(addr >> 8),
        (uint8_t)(addr)
    };

    flash_cs_low();
    flash_spi_xfer(cmd, NULL, 4);
    flash_cs_high();

    flash_wait_ready();
    return 0;
}

int flash_chip_erase(void)
{
    flash_write_enable();
    uint8_t cmd = FLASH_CMD_CE;
    flash_cs_low();
    flash_spi_xfer(&cmd, NULL, 1);
    flash_cs_high();
    flash_wait_ready();
    return 0;
}

void flash_enter_powerdown(void)
{
    uint8_t cmd = FLASH_CMD_PD;
    flash_cs_low();
    flash_spi_xfer(&cmd, NULL, 1);
    flash_cs_high();
}

void flash_wakeup(void)
{
    uint8_t cmd = FLASH_CMD_PU;
    flash_cs_low();
    flash_spi_xfer(&cmd, NULL, 1);
    flash_cs_high();
    /* Wait ~30 µs for wakeup */
}

/* ---- Append-mode logging (circular buffer) ---- */
int flash_append_record(const uint8_t *data, uint16_t len)
{
    if (len == 0) return 0;

    /* Check if we need to erase the current sector before writing */
    uint32_t current_sector = write_pointer / FLASH_SECTOR_SIZE;
    uint32_t sector_start = current_sector * FLASH_SECTOR_SIZE;

    /* If we're at the start of a new sector, erase it first */
    if (write_pointer == sector_start && sector_start >= 0x020000) {
        flash_erase_sector(sector_start);
    }

    /* Write in page-sized chunks */
    uint16_t offset = 0;
    while (offset < len) {
        uint16_t chunk = len - offset;
        uint16_t page_remaining = FLASH_PAGE_SIZE
                                - (write_pointer % FLASH_PAGE_SIZE);
        if (chunk > page_remaining) chunk = page_remaining;

        flash_write_page(write_pointer, data + offset, chunk);
        write_pointer += chunk;
        offset += chunk;

        /* Wrap around at end of flash */
        if (write_pointer >= FLASH_TOTAL_SIZE - FLASH_SECTOR_SIZE) {
            write_pointer = 0x020000;
        }
    }

    return 0;
}

uint32_t flash_get_write_pointer(void)
{
    return write_pointer;
}

int flash_read_record(uint32_t addr, uint8_t *buf, uint16_t max_len,
                      uint16_t *record_len)
{
    /* Read record header (26 bytes) */
    if (addr + 26 > FLASH_TOTAL_SIZE) return -1;

    uint8_t header[26];
    flash_read(addr, header, 26);

    /* Validate magic */
    uint32_t magic;
    memcpy(&magic, &header[0], 4);
    if (magic != EPISODE_MAGIC) return -1;

    /* Extract record length from sample count */
    uint16_t sample_count;
    memcpy(&sample_count, &header[10], 2);
    uint16_t total_len = 26 + (sample_count * IMU_SAMPLE_SIZE / 10);
    *record_len = total_len;

    if (total_len > max_len) return -2;  /* Buffer too small */

    flash_read(addr, buf, total_len > max_len ? max_len : total_len);
    return 0;
}

int flash_read_all_records(uint8_t *buf, uint32_t max_len, uint16_t *count)
{
    *count = 0;
    uint32_t addr = 0x020000;
    uint32_t buf_offset = 0;

    while (addr < write_pointer && buf_offset < max_len) {
        uint16_t record_len = 0;
        int ret = flash_read_record(addr, buf + buf_offset,
                                     max_len - buf_offset, &record_len);
        if (ret == 0) {
            buf_offset += record_len;
            addr += record_len;
            (*count)++;
        } else if (ret == -2) {
            break;  /* Buffer full */
        } else {
            addr += 26;  /* Skip invalid record */
        }
    }

    return 0;
}

void flash_reset_write_pointer(void)
{
    write_pointer = 0x020000;
}

uint32_t flash_get_write_count(uint32_t sector)
{
    if (sector >= 128) return 0;
    return sector_write_count[sector];
}

void flash_mark_sector_worn(uint32_t sector)
{
    if (sector < 128) {
        sector_write_count[sector] = 0xFFFFFFFF;  /* Mark as worn out */
    }
}

/* ---- End of flash_drv.c ---- */