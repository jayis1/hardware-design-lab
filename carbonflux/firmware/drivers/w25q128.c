/**
 * @file    w25q128.c
 * @brief   W25Q128JV SPI flash driver with ring buffer log.
 * @author  jayis1
 * @copyright Copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#include "w25q128.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

/* Commands */
#define CMD_WRITE_ENABLE    0x06
#define CMD_WRITE_DISABLE   0x04
#define CMD_READ_STATUS1    0x05
#define CMD_READ_STATUS2    0x35
#define CMD_PAGE_PROGRAM    0x02
#define CMD_QUAD_PAGE_PROG  0x32
#define CMD_SECTOR_ERASE    0x20
#define CMD_BLOCK_ERASE_32K 0x52
#define CMD_BLOCK_ERASE_64K 0xD8
#define CMD_CHIP_ERASE      0xC7
#define CMD_READ_DATA       0x03
#define CMD_READ_DATA_FAST  0x0B
#define CMD_READ_ID         0x9F
#define CMD_ENTER_QSPI      0x38

#define W25Q128_MFG_ID      0xEF
#define W25Q128_DEV_ID      0x18

static struct {
    uint32_t head;   /* Next write index */
    uint32_t tail;   /* Oldest valid index */
    uint32_t count;  /* Number of valid entries */
    bool     initialized;
} g_flash_log = {0};

/* SPI helpers */
static void flash_cs_low(void)  { GPIO_RESET_PIN(GPIOA, 8); }
static void flash_cs_high(void) { GPIO_SET_PIN(GPIOA, 8); }

static uint8_t flash_spi_xfer(uint8_t byte) {
    while (!(SPI2->SR & SPI_SR_TXE));
    SPI2->TXDR = byte;
    while (!(SPI2->SR & SPI_SR_RXP));
    return (uint8_t)(SPI2->RXDR & 0xFF);
}

static void flash_spi_write(const uint8_t *data, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        while (!(SPI2->SR & SPI_SR_TXE));
        SPI2->TXDR = data[i];
    }
    /* Wait for FIFO to drain */
    while (SPI2->SR & SPI_SR_BSY);
}

static void flash_spi_read(uint8_t *buf, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        while (!(SPI2->SR & SPI_SR_RXP));
        buf[i] = (uint8_t)(SPI2->RXDR & 0xFF);
    }
}

static void flash_cmd(uint8_t cmd) {
    flash_cs_low();
    flash_spi_xfer(cmd);
    flash_cs_high();
}

static uint8_t flash_read_status(void) {
    flash_cs_low();
    flash_spi_xfer(CMD_READ_STATUS1);
    uint8_t s = flash_spi_xfer(0);
    flash_cs_high();
    return s;
}

static void flash_wait_busy(void) {
    while (flash_read_status() & 0x01);
}

static void flash_write_enable(void) {
    flash_cmd(CMD_WRITE_ENABLE);
}

int w25q128_init(void) {
    /* Read manufacturer and device ID */
    flash_cs_low();
    flash_spi_xfer(CMD_READ_ID);
    uint8_t mfr = flash_spi_xfer(0);
    uint8_t dev = flash_spi_xfer(0);
    flash_cs_high();

    if (mfr != W25Q128_MFG_ID || dev != W25Q128_DEV_ID) {
        return -ERR_SENSOR_NOT_FOUND;
    }
    return 0;
}

int flash_log_init(void) {
    g_flash_log.head = 0;
    g_flash_log.tail = 0;
    g_flash_log.count = 0;
    g_flash_log.initialized = true;
    return 0;
}

int flash_log_write(const flux_record_t *record) {
    if (!g_flash_log.initialized) return -ERR_FLASH_WRITE_FAIL;

    uint32_t addr = FLASH_LOG_START_ADDR + g_flash_log.head * sizeof(flux_record_t);
    uint32_t sector_base = addr & ~(FLASH_SECTOR_SIZE - 1);

    /* Check if sector needs erase */
    flash_cs_low();
    flash_spi_xfer(CMD_READ_DATA_FAST);
    flash_spi_xfer((addr >> 16) & 0xFF);
    flash_spi_xfer((addr >> 8) & 0xFF);
    flash_spi_xfer(addr & 0xFF);
    flash_spi_xfer(0); /* Dummy */
    uint8_t first_byte;
    flash_spi_read(&first_byte, 1);
    flash_cs_high();

    if (first_byte != 0xFF) {
        /* Sector is not erased — erase it */
        flash_write_enable();
        flash_cs_low();
        flash_spi_xfer(CMD_SECTOR_ERASE);
        flash_spi_xfer((sector_base >> 16) & 0xFF);
        flash_spi_xfer((sector_base >> 8) & 0xFF);
        flash_spi_xfer(sector_base & 0xFF);
        flash_cs_high();
        flash_wait_busy();
    }

    /* Page program (max 256 bytes per program) */
    flash_write_enable();
    flash_cs_low();
    flash_spi_xfer(CMD_PAGE_PROGRAM);
    flash_spi_xfer((addr >> 16) & 0xFF);
    flash_spi_xfer((addr >> 8) & 0xFF);
    flash_spi_xfer(addr & 0xFF);
    flash_spi_write((const uint8_t *)record, sizeof(flux_record_t));
    flash_cs_high();
    flash_wait_busy();

    /* Update ring buffer pointers */
    g_flash_log.head++;
    if (g_flash_log.head >= FLASH_LOG_MAX_ENTRIES) {
        g_flash_log.head = 0;
    }
    if (g_flash_log.count < FLASH_LOG_MAX_ENTRIES) {
        g_flash_log.count++;
    } else {
        g_flash_log.tail = (g_flash_log.tail + 1) % FLASH_LOG_MAX_ENTRIES;
    }

    return 0;
}

int flash_log_read(uint32_t index, flux_record_t *record) {
    if (!g_flash_log.initialized || index >= g_flash_log.count) {
        return -ERR_FLASH_WRITE_FAIL;
    }

    uint32_t addr = FLASH_LOG_START_ADDR
                  + ((g_flash_log.tail + index) % FLASH_LOG_MAX_ENTRIES)
                  * sizeof(flux_record_t);

    flash_cs_low();
    flash_spi_xfer(CMD_READ_DATA_FAST);
    flash_spi_xfer((addr >> 16) & 0xFF);
    flash_spi_xfer((addr >> 8) & 0xFF);
    flash_spi_xfer(addr & 0xFF);
    flash_spi_xfer(0); /* Dummy */
    flash_spi_read((uint8_t *)record, sizeof(flux_record_t));
    flash_cs_high();

    return 0;
}

void flash_log_dump(uint32_t max_entries) {
    uint32_t n = (g_flash_log.count < max_entries) ? g_flash_log.count : max_entries;
    for (uint32_t i = 0; i < n; i++) {
        flux_record_t rec;
        if (flash_log_read(i, &rec) == 0) {
            /* Output in CSV format over USB CDC */
            printf("%lu,%.3f,%.3f,%d,%d,%d,%.1f,%.1f,%.1f,%u,%u\r\n",
                   rec.timestamp,
                   (float)rec.co2_ppm_x1000 / 1000.0f,
                   (float)rec.flux_umol_x1000 / 1000.0f,
                   rec.soil_temp_5cm_x100,
                   rec.soil_temp_15cm_x100,
                   rec.soil_temp_30cm_x100,
                   (float)rec.vwc_x100 / 100.0f,
                   (float)rec.pressure_x10 / 10.0f,
                   (float)rec.air_temp_c_x100 / 100.0f,
                   rec.par_umol,
                   rec.battery_soc);
        }
    }
}

void flash_read_config(uint8_t *buf, uint32_t len) {
    flash_cs_low();
    flash_spi_xfer(CMD_READ_DATA_FAST);
    flash_spi_xfer(0x00); flash_spi_xfer(0x00); flash_spi_xfer(0x00);
    flash_spi_xfer(0); /* Dummy */
    flash_spi_read(buf, len);
    flash_cs_high();
}

uint32_t flash_log_get_head(void) {
    return g_flash_log.head;
}