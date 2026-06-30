/*
 * drivers/storage.c — FRAM and SPI flash storage for event logging
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * Manages two non-volatile storage devices:
 *
 * 1. MB85RS2MTA FRAM (256 KB, SPI2):
 *    Used for the detection event log as a circular buffer. FRAM has
 *    10^14 write cycles, so we can log events continuously without
 *    wear concerns. The event log starts at address 0x0000 and each
 *    event is sizeof(detection_event_t) bytes. A header at address 0
 *    stores the write pointer and event count.
 *
 * 2. W25Q128JVSIQ SPI NOR Flash (16 MB, SPI1):
 *    Used for storing multispectral image thumbnails and acoustic
 *    spectrograms for later review. Flash has 10^5 write cycles, so
 *    we use wear-leveling by writing to sequential addresses and
 *    erasing sectors only when full. The flash is organized as:
 *      0x000000-0xEFFFFF: Image archive (14 MB, ~560 thumbnails at 25 KB each)
 *      0xF00000-0xFFFFFF: Detection event backup (1 MB)
 *
 * SPI1 is shared between the LoRa transceiver and flash via separate
 * NSS signals. SPI2 is dedicated to FRAM.
 */

#include "storage.h"
#include "../registers.h"
#include "../board.h"
#include <string.h>

/* ----------------------------------------------------------------- *
 *  FRAM SPI commands (MB85RS2MTA)
 * ----------------------------------------------------------------- */
#define FRAM_CMD_WREN    0x06  /* Write enable */
#define FRAM_CMD_WRDI    0x04  /* Write disable */
#define FRAM_CMD_READ     0x03 /* Read data */
#define FRAM_CMD_WRITE    0x02 /* Write data */
#define FRAM_CMD_RDID     0x9F /* Read device ID */
#define FRAM_CMD_SLEEP    0xB9 /* Enter sleep mode */
#define FRAM_CMD_WAKE     0xAB /* Wake up */

/* ----------------------------------------------------------------- *
 *  Flash SPI commands (W25Q128)
 * ----------------------------------------------------------------- */
#define FLASH_CMD_WREN    0x06
#define FLASH_CMD_READ     0x03
#define FLASH_CMD_FAST_READ 0x0B
#define FLASH_CMD_PAGE_PROGRAM 0x02
#define FLASH_CMD_SECTOR_ERASE 0x20
#define FLASH_CMD_CHIP_ERASE 0xC7
#define FLASH_CMD_RDID     0x9F
#define FLASH_CMD_RDSR     0x05  /* Read status register */
#define FLASH_CMD_WRSR     0x01
#define FLASH_CMD_READ_SR2 0x35
#define FLASH_CMD_READ_SR3 0x15

#define FLASH_SR_BUSY     0x01
#define FLASH_SR_WEL      0x02

/* ----------------------------------------------------------------- *
 *  Event log header (stored at FRAM address 0)
 * ----------------------------------------------------------------- */
typedef struct {
    uint32_t magic;          /* FRAM_EVENT_MAGIC = "SPET" */
    uint32_t write_offset;    /* Next write address (after header) */
    uint32_t event_count;     /* Total events logged */
    uint32_t overflow_count;  /* Times the buffer wrapped around */
} fram_header_t;

#define FRAM_HEADER_ADDR  0
#define FRAM_EVENT_START  (sizeof(fram_header_t))
#define FRAM_EVENT_SIZE   (sizeof(detection_event_t))
#define FRAM_MAX_EVENTS   ((FRAM_SIZE_BYTES - FRAM_EVENT_START) / FRAM_EVENT_SIZE)

static fram_header_t g_header;

/* ----------------------------------------------------------------- *
 *  SPI2 transfer (for FRAM)
 * ----------------------------------------------------------------- */
static void spi2_transfer(uint8_t *tx, uint8_t *rx, uint32_t len) {
    /* Enable SPI2 */
    SPI2->CR1 |= SPI_CR1_SPE;

    for (uint32_t i = 0; i < len; i++) {
        /* Wait for TX ready */
        while (!(SPI2->SR & SPI_SR_TXP));
        SPI2->TXDR = tx ? tx[i] : 0x00;
        /* Wait for RX ready */
        while (!(SPI2->SR & SPI_SR_RXP));
        if (rx) rx[i] = SPI2->RXDR;
        else (void)SPI2->RXDR;
    }

    /* Wait for completion */
    while (!(SPI2->SR & SPI_SR_EOT));
    SPI2->CR1 &= ~SPI_CR1_SPE;
}

/* ----------------------------------------------------------------- *
 *  SPI1 transfer (for Flash — shares with LoRa via NSS)
 * ----------------------------------------------------------------- */
static void spi1_transfer(uint8_t *tx, uint8_t *rx, uint32_t len) {
    SPI1->CR1 |= SPI_CR1_SPE;

    for (uint32_t i = 0; i < len; i++) {
        while (!(SPI1->SR & SPI_SR_TXP));
        SPI1->TXDR = tx ? tx[i] : 0x00;
        while (!(SPI1->SR & SPI_SR_RXP));
        if (rx) rx[i] = SPI1->RXDR;
        else (void)SPI1->RXDR;
    }

    while (!(SPI1->SR & SPI_SR_EOT));
    SPI1->CR1 &= ~SPI_CR1_SPE;
}

/* ----------------------------------------------------------------- *
 *  Initialize storage subsystem
 * ----------------------------------------------------------------- */
int storage_init(void) {
    /* --- Configure SPI2 for FRAM --- */
    RCC->APB1ENR1 |= RCC_APB1ENR1_SPI2EN;

    /* SPI2 pins: SCK=PB14, MISO=PB15, MOSI=PB3 (alternate) */
    gpio_set_mode(GPIOB, 14, GPIO_MODE_AF);
    gpio_set_af(GPIOB, 14, 5);  /* AF5 for SPI2 */
    gpio_set_ospeed(GPIOB, 14, GPIO_SPEED_VHIGH);

    gpio_set_mode(GPIOB, 15, GPIO_MODE_AF);
    gpio_set_af(GPIOB, 15, 5);
    gpio_set_ospeed(GPIOB, 15, GPIO_SPEED_VHIGH);

    gpio_set_mode(GPIOB, 3, GPIO_MODE_AF);
    gpio_set_af(GPIOB, 3, 5);
    gpio_set_ospeed(GPIOB, 3, GPIO_SPEED_VHIGH);

    /* FRAM NSS */
    gpio_set_mode(FRAM_NSS_PORT, FRAM_NSS_PIN, GPIO_MODE_OUTPUT);
    gpio_write(FRAM_NSS_PORT, FRAM_NSS_PIN, 1);

    /* Configure SPI2: master, 8-bit, CPOL=0, CPHA=0 */
    SPI2->CFG1 = SPI_CFG1_MASTER | SPI_CFG1_CSIZE_8BIT;
    SPI2->CFG2 = SPI_CFG2_COMM_FULL;
    SPI2->CR1 = 0;

    /* --- Configure Flash NSS (SPI1 already initialized by LoRa) --- */
    gpio_set_mode(FLASH_NSS_PORT, FLASH_NSS_PIN, GPIO_MODE_OUTPUT);
    gpio_write(FLASH_NSS_PORT, FLASH_NSS_PIN, 1);
    gpio_set_mode(FLASH_HOLD_PORT, FLASH_HOLD_PIN, GPIO_MODE_OUTPUT);
    gpio_write(FLASH_HOLD_PORT, FLASH_HOLD_PIN, 1);
    gpio_set_mode(FLASH_WP_PORT, FLASH_WP_PIN, GPIO_MODE_OUTPUT);
    gpio_write(FLASH_WP_PORT, FLASH_WP_PIN, 1);

    /* --- Read FRAM header --- */
    fram_read(FRAM_HEADER_ADDR, (uint8_t *)&g_header, sizeof(g_header));

    if (g_header.magic != FRAM_EVENT_MAGIC) {
        /* Initialize fresh header */
        memset(&g_header, 0, sizeof(g_header));
        g_header.magic = FRAM_EVENT_MAGIC;
        g_header.write_offset = FRAM_EVENT_START;
        g_header.event_count = 0;
        g_header.overflow_count = 0;
        fram_write(FRAM_HEADER_ADDR, (uint8_t *)&g_header, sizeof(g_header));
    }

    return 0;
}

/* ----------------------------------------------------------------- *
 *  FRAM read/write
 * ----------------------------------------------------------------- */
int fram_read(uint32_t addr, uint8_t *data, uint32_t len) {
    uint8_t cmd[4];
    cmd[0] = FRAM_CMD_READ;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;

    gpio_write(FRAM_NSS_PORT, FRAM_NSS_PIN, 0);
    spi2_transfer(cmd, NULL, 4);
    spi2_transfer(NULL, data, len);
    gpio_write(FRAM_NSS_PORT, FRAM_NSS_PIN, 1);

    return 0;
}

int fram_write(uint32_t addr, const uint8_t *data, uint32_t len) {
    /* Enable write */
    uint8_t wren = FRAM_CMD_WREN;
    gpio_write(FRAM_NSS_PORT, FRAM_NSS_PIN, 0);
    spi2_transfer(&wren, NULL, 1);
    gpio_write(FRAM_NSS_PORT, FRAM_NSS_PIN, 1);

    /* Write data */
    uint8_t cmd[4];
    cmd[0] = FRAM_CMD_WRITE;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;

    gpio_write(FRAM_NSS_PORT, FRAM_NSS_PIN, 0);
    spi2_transfer(cmd, NULL, 4);
    spi2_transfer((uint8_t *)data, NULL, len);
    gpio_write(FRAM_NSS_PORT, FRAM_NSS_PIN, 1);

    return 0;
}

/* ----------------------------------------------------------------- *
 *  FRAM event log functions
 * ----------------------------------------------------------------- */
int fram_write_event(const detection_event_t *event) {
    if (!event) return -1;

    /* Check if we need to wrap around */
    if (g_header.write_offset + FRAM_EVENT_SIZE > FRAM_SIZE_BYTES) {
        g_header.write_offset = FRAM_EVENT_START;
        g_header.overflow_count++;
    }

    /* Write event */
    int result = fram_write(g_header.write_offset, (const uint8_t *)event,
                            sizeof(detection_event_t));
    if (result < 0) return -1;

    /* Update header */
    g_header.write_offset += FRAM_EVENT_SIZE;
    g_header.event_count++;
    fram_write(FRAM_HEADER_ADDR, (uint8_t *)&g_header, sizeof(g_header));

    return 0;
}

int fram_read_event(uint32_t index, detection_event_t *out) {
    if (index >= FRAM_MAX_EVENTS || !out) return -1;

    uint32_t addr = FRAM_EVENT_START + index * FRAM_EVENT_SIZE;
    return fram_read(addr, (uint8_t *)out, sizeof(detection_event_t));
}

uint32_t fram_get_event_count(void) {
    return g_header.event_count;
}

void fram_clear_events(void) {
    g_header.write_offset = FRAM_EVENT_START;
    g_header.event_count = 0;
    g_header.overflow_count = 0;
    fram_write(FRAM_HEADER_ADDR, (uint8_t *)&g_header, sizeof(g_header));
}

/* ----------------------------------------------------------------- *
 *  Flash operations
 * ----------------------------------------------------------------- */
int flash_wait_busy(void) {
    uint32_t timeout = board_get_tick_ms() + 1000;
    while (board_get_tick_ms() < timeout) {
        uint8_t cmd = FLASH_CMD_RDSR;
        uint8_t status;

        gpio_write(FLASH_NSS_PORT, FLASH_NSS_PIN, 0);
        spi1_transfer(&cmd, NULL, 1);
        spi1_transfer(NULL, &status, 1);
        gpio_write(FLASH_NSS_PORT, FLASH_NSS_PIN, 1);

        if (!(status & FLASH_SR_BUSY)) return 0;
    }
    return -1;
}

int flash_write_enable(void) {
    uint8_t cmd = FLASH_CMD_WREN;
    gpio_write(FLASH_NSS_PORT, FLASH_NSS_PIN, 0);
    spi1_transfer(&cmd, NULL, 1);
    gpio_write(FLASH_NSS_PORT, FLASH_NSS_PIN, 1);
    return 0;
}

int flash_erase_sector(uint32_t addr) {
    if (flash_write_enable() < 0) return -1;

    uint8_t cmd[4];
    cmd[0] = FLASH_CMD_SECTOR_ERASE;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;

    gpio_write(FLASH_NSS_PORT, FLASH_NSS_PIN, 0);
    spi1_transfer(cmd, NULL, 4);
    gpio_write(FLASH_NSS_PORT, FLASH_NSS_PIN, 1);

    return flash_wait_busy();
}

int flash_write_image(uint32_t addr, const uint8_t *data, uint32_t len) {
    /* Page program: writes up to 256 bytes at a time */
    uint32_t offset = 0;
    while (offset < len) {
        if (flash_write_enable() < 0) return -1;

        uint32_t page_remain = FLASH_PAGE_SIZE - (addr % FLASH_PAGE_SIZE);
        uint32_t chunk = (len - offset < page_remain) ? (len - offset) : page_remain;

        uint8_t cmd[4];
        cmd[0] = FLASH_CMD_PAGE_PROGRAM;
        cmd[1] = ((addr + offset) >> 16) & 0xFF;
        cmd[2] = ((addr + offset) >> 8) & 0xFF;
        cmd[3] = (addr + offset) & 0xFF;

        gpio_write(FLASH_NSS_PORT, FLASH_NSS_PIN, 0);
        spi1_transfer(cmd, NULL, 4);
        spi1_transfer((uint8_t *)&data[offset], NULL, chunk);
        gpio_write(FLASH_NSS_PORT, FLASH_NSS_PIN, 1);

        if (flash_wait_busy() < 0) return -1;

        offset += chunk;
    }

    return 0;
}

int flash_read_image(uint32_t addr, uint8_t *data, uint32_t len) {
    uint8_t cmd[4];
    cmd[0] = FLASH_CMD_READ;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;

    gpio_write(FLASH_NSS_PORT, FLASH_NSS_PIN, 0);
    spi1_transfer(cmd, NULL, 4);
    spi1_transfer(NULL, data, len);
    gpio_write(FLASH_NSS_PORT, FLASH_NSS_PIN, 1);

    return 0;
}