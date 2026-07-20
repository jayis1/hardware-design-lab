/*
 * sdlog.c — SD Card Logging
 * RootTrace — Open-Source Electrical Impedance Tomography Root Imaging System
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 *
 * Provides a simple file-based logging system using the STM32H7 SDMMC1
 * peripheral in 8-bit mode.  Each EIT frame is logged as a binary file
 * containing the raw measurement data and reconstruction results.
 *
 * File format (RTFR — RootTrace Frame Record):
 *   Offset  Size  Field
 *   0x00    4     Magic: 'RTF1' (0x31524652)
 *   0x04    2     Version: 1
 *   0x06    2     Header size: 64
 *   0x08    4     Timestamp (ms since boot)
 *   0x0C    2     Frame sequence number
 *   0x0E    1     Frequency index
 *   0x0F    1     Status
 *   0x10    2     Electrode contact mask
 *   0x12    2     Reserved
 *   0x14    4     Frame size (208 * sizeof(meas) = 208 * 20 = 4160)
 *   0x18    4     Mesh node count (576)
 *   0x1C   32     Reserved / metadata
 *   0x40   4160   Raw measurements (208 × {re, im, freq, mag, phase} = 20 bytes)
 *   0x1080 2304   Conductivity array (576 × float = 2304 bytes)
 *   Total:  6400 bytes per frame file
 */

#include "sdlog.h"
#include "board.h"
#include "registers.h"
#include <string.h>

/* SDMMC1 register offsets (simplified) */
#define SDMMC_POWER     (*(volatile uint32_t *)(SDMMC1_BASE + 0x00))
#define SDMMC_CLKCR     (*(volatile uint32_t *)(SDMMC1_BASE + 0x04))
#define SDMMC_ARG       (*(volatile uint32_t *)(SDMMC1_BASE + 0x08))
#define SDMMC_CMD       (*(volatile uint32_t *)(SDMMC1_BASE + 0x0C))
#define SDMMC_RESP1     (*(volatile uint32_t *)(SDMMC1_BASE + 0x14))
#define SDMMC_RESP2     (*(volatile uint32_t *)(SDMMC1_BASE + 0x18))
#define SDMMC_RESP3     (*(volatile uint32_t *)(SDMMC1_BASE + 0x1C))
#define SDMMC_RESP4     (*(volatile uint32_t *)(SDMMC1_BASE + 0x20))
#define SDMMC_DTIMER    (*(volatile uint32_t *)(SDMMC1_BASE + 0x24))
#define SDMMC_DLEN      (*(volatile uint32_t *)(SDMMC1_BASE + 0x28))
#define SDMMC_DCTRL     (*(volatile uint32_t *)(SDMMC1_BASE + 0x2C))
#define SDMMC_DCOUNT    (*(volatile uint32_t *)(SDMMC1_BASE + 0x30))
#define SDMMC_STA       (*(volatile uint32_t *)(SDMMC1_BASE + 0x34))
#define SDMMC_ICR       (*(volatile uint32_t *)(SDMMC1_BASE + 0x38))
#define SDMMC_MASK      (*(volatile uint32_t *)(SDMMC1_BASE + 0x3C))
#define SDMMC_IDMACTRL  (*(volatile uint32_t *)(SDMMC1_BASE + 0x50))
#define SDMMC_IDMABSIZE (*(volatile uint32_t *)(SDMMC1_BASE + 0x54))
#define SDMMC_IDMABASE0 (*(volatile uint32_t *)(SDMMC1_BASE + 0x58))

#define SDMMC_STA_CMDREND   (1U << 6)
#define SDMMC_STA_CMDSENT   (1U << 7)
#define SDMMC_STA_DATAEND   (1U << 8)
#define SDMMC_STA_DBCKEND   (1U << 10)
#define SDMMC_STA_RXDAVL    (1U << 21)
#define SDMMC_STA_TXDAVL    (1U << 22)
#define SDMMC_STA_RXFIFOE   (1U << 19)
#define SDMMC_STA_TXFIFOE   (1U << 18)
#define SDMMC_ICR_CLEAR_ALL 0xFFFFFFFFU

/* File format constants */
#define RTF_MAGIC       0x31524652U  /* 'RTF1' little-endian */
#define RTF_VERSION      1
#define RTF_HEADER_SIZE  64
#define RTF_MEAS_SIZE    (EIT_FRAME_SIZE * 20)
#define RTF_COND_SIZE    (EIT_MESH_NODES * 4)
#define RTF_TOTAL_SIZE   (RTF_HEADER_SIZE + RTF_MEAS_SIZE + RTF_COND_SIZE)

static int s_sd_initialized = 0;
static int s_file_count = 0;
static uint32_t s_current_block = 0;  /* current write block address */

/* ---------------------------------------------------------------------
 * GPIO and peripheral initialization for SDMMC1
 * --------------------------------------------------------------------- */

static void sdmmc_gpio_init(void)
{
    RCC->AHB4ENR |= BIT(2) | BIT(3) | BIT(6);  /* GPIOC, GPIOD, GPIOG */
    RCC->APB2ENR |= BIT(10);  /* SDMMC1 */

    /* PC8-PC12: SDMMC1 D0-D3, CLK (AF12) */
    int pc_pins[] = {8, 9, 10, 11, 12};
    for (int i = 0; i < 5; i++) {
        int p = pc_pins[i];
        GPIOC->MODER &= ~(3U << (p*2));
        GPIOC->MODER |= (GPIO_MODE_AF << (p*2));
        GPIOC->OSPEEDR |= (GPIO_SPEED_VHIGH << (p*2));
        GPIOC->PUPDR |= (GPIO_PUPD_PU << (p*2));
        if (p < 8) {
            GPIOC->AFRL &= ~(0xFU << (p*4));
            GPIOC->AFRL |= (12U << (p*4));
        } else {
            GPIOC->AFRH &= ~(0xFU << ((p-8)*4));
            GPIOC->AFRH |= (12U << ((p-8)*4));
        }
    }

    /* PD2: SDMMC1_CMD (AF12) */
    GPIOD->MODER &= ~(3U << (2*2));
    GPIOD->MODER |= (GPIO_MODE_AF << (2*2));
    GPIOD->OSPEEDR |= (GPIO_SPEED_VHIGH << (2*2));
    GPIOD->PUPDR |= (GPIO_PUPD_PU << (2*2));
    GPIOD->AFRL &= ~(0xFU << (2*4));
    GPIOD->AFRL |= (12U << (2*4));
}

/* ---------------------------------------------------------------------
 * Send a command to the SD card and wait for response
 * --------------------------------------------------------------------- */

static int sdmmc_send_cmd(uint8_t cmd_idx, uint32_t arg, uint8_t resp_type)
{
    SDMMC_ICR = SDMMC_ICR_CLEAR_ALL;
    SDMMC_ARG = arg;
    SDMMC_CMD = (cmd_idx << 0) | (resp_type << 6) | BIT(10);  /* CPSMEN */

    uint32_t timeout = 1000000;
    while (timeout-- > 0) {
        uint32_t sta = SDMMC_STA;
        if (sta & (SDMMC_STA_CMDREND | SDMMC_STA_CMDSENT))
            return 0;
    }
    return -1;
}

static int sdmmc_wait_data_end(uint32_t timeout)
{
    while (timeout-- > 0) {
        if (SDMMC_STA & SDMMC_STA_DATAEND)
            return 0;
    }
    return -1;
}

/* ---------------------------------------------------------------------
 * Initialize SD card (simplified SDIO mode)
 * --------------------------------------------------------------------- */

int sdlog_init(void)
{
    sdmmc_gpio_init();

    /* Power on SDMMC */
    SDMMC_POWER = 3;  /* SDMMC powered on */
    for (volatile int i = 0; i < 100000; i++) { }

    /* Set clock to 400 kHz for initialization */
    SDMMC_CLKCR = (BOARD_PCLK2_HZ / 400000) | BIT(17) /* HW flow control */
                | BIT(11)  /* clock enable */;

    /* CMD0: GO_IDLE_STATE (no response) */
    if (sdmmc_send_cmd(0, 0, 0) != 0) return -1;

    /* CMD8: SEND_IF_COND (R7 response) */
    if (sdmmc_send_cmd(8, 0x000001AA, 3) != 0) return -1;

    /* CMD55: APP_CMD (R1 response) */
    if (sdmmc_send_cmd(55, 0, 1) != 0) return -1;

    /* ACMD41: SD_SEND_OP_COND (R3 response) — initialize with OCR */
    uint32_t ocr = 0x40FF8000;  /* 3.2V-3.4V, SDHC */
    int retries = 100;
    do {
        sdmmc_send_cmd(55, 0, 1);
        sdmmc_send_cmd(41, ocr, 3);
        if (SDMMC_RESP1 & (1U << 31))  /* power up bit */
            break;
    } while (retries-- > 0);
    if (retries == 0) return -1;

    /* CMD2: ALL_SEND_CID (R2 response) */
    sdmmc_send_cmd(2, 0, 2);

    /* CMD3: SEND_RELATIVE_ADDR (R6 response) */
    sdmmc_send_cmd(3, 0, 1);
    uint16_t rca = (uint16_t)((SDMMC_RESP1 >> 16) & 0xFFFF);

    /* CMD9: SEND_CSD (R2 response) — get card size */
    sdmmc_send_cmd(9, ((uint32_t)rca << 16), 2);

    /* CMD7: SELECT_CARD (R1b response) */
    sdmmc_send_cmd(7, ((uint32_t)rca << 16), 1);

    /* Set clock to 24 MHz for data transfer */
    SDMMC_CLKCR = (BOARD_PCLK2_HZ / 24000000) | BIT(17) | BIT(11) | BIT(14) /* 4-bit bus */;

    /* ACMD6: SET_BUS_WIDTH 4-bit */
    sdmmc_send_cmd(55, ((uint32_t)rca << 16), 1);
    sdmmc_send_cmd(6, 2, 1);

    s_sd_initialized = 1;
    s_file_count = 0;
    s_current_block = 0;

    return 0;
}

int sdlog_is_present(void)
{
    /* Check SD card detect pin (PG2) */
    return (PIN_GET(SDMMC_CD_PORT, SDMMC_CD_PIN) == 0) ? 1 : 0;
}

/* ---------------------------------------------------------------------
 * Write a single 512-byte block to SD card
 * --------------------------------------------------------------------- */

static int sdmmc_write_block(uint32_t block_addr, const uint8_t *data)
{
    if (!s_sd_initialized) return -1;

    /* CMD24: WRITE_SINGLE_BLOCK */
    SDMMC_DTIMER = 0xFFFFFFFF;
    SDMMC_DLEN = 512;
    SDMMC_DCTRL = 0;  /* reset data control */
    SDMMC_ICR = SDMMC_ICR_CLEAR_ALL;

    if (sdmmc_send_cmd(24, block_addr, 1) != 0) return -1;

    /* Configure data transfer: block size 512, dir to card, enable */
    SDMMC_DCTRL = (9U << 4)  /* 2^9 = 512 bytes block */
                | BIT(2)     /* dir = to card */
                | BIT(0);    /* DTEN */

    /* Write 128 32-bit words to FIFO */
    for (int i = 0; i < 128; i++) {
        uint32_t word = (uint32_t)data[i*4]
                      | ((uint32_t)data[i*4+1] << 8)
                      | ((uint32_t)data[i*4+2] << 16)
                      | ((uint32_t)data[i*4+3] << 24);
        while (!(SDMMC_STA & SDMMC_STA_TXDAVL)) { }
        *(volatile uint32_t *)(SDMMC1_BASE + 0x80) = word;  /* FIFO */
    }

    if (sdmmc_wait_data_end(1000000) != 0) return -1;

    /* Wait for card to finish programming (CMD13: SEND_STATUS) */
    uint32_t timeout = 1000000;
    while (timeout-- > 0) {
        sdmmc_send_cmd(13, 0, 1);
        if ((SDMMC_RESP1 & 0x1F00) == 0) break;  /* TRAN state */
    }

    return 0;
}

/* ---------------------------------------------------------------------
 * Read a single 512-byte block from SD card
 * --------------------------------------------------------------------- */

static int sdmmc_read_block(uint32_t block_addr, uint8_t *data)
{
    if (!s_sd_initialized) return -1;

    SDMMC_DTIMER = 0xFFFFFFFF;
    SDMMC_DLEN = 512;
    SDMMC_DCTRL = 0;
    SDMMC_ICR = SDMMC_ICR_CLEAR_ALL;

    /* CMD17: READ_SINGLE_BLOCK */
    if (sdmmc_send_cmd(17, block_addr, 1) != 0) return -1;

    SDMMC_DCTRL = (9U << 4)   /* 512 bytes block */
                | BIT(1)      /* dir = from card */
                | BIT(0);     /* DTEN */

    for (int i = 0; i < 128; i++) {
        while (!(SDMMC_STA & SDMMC_STA_RXDAVL)) { }
        uint32_t word = *(volatile uint32_t *)(SDMMC1_BASE + 0x80);
        data[i*4]     = word & 0xFF;
        data[i*4+1]   = (word >> 8) & 0xFF;
        data[i*4+2]   = (word >> 16) & 0xFF;
        data[i*4+3]   = (word >> 24) & 0xFF;
    }

    return sdmmc_wait_data_end(1000000);
}

/* ---------------------------------------------------------------------
 * Log an EIT frame to SD card
 * --------------------------------------------------------------------- */

int sdlog_log_frame(const eit_frame_t *frame)
{
    if (!s_sd_initialized || !frame) return -1;

    /* Build the RTFR record in a local buffer (6400 bytes = 13 blocks) */
    static uint8_t buffer[RTF_TOTAL_SIZE];
    memset(buffer, 0, sizeof(buffer));

    /* Header */
    *(uint32_t *)&buffer[0x00] = RTF_MAGIC;
    *(uint16_t *)&buffer[0x04] = RTF_VERSION;
    *(uint16_t *)&buffer[0x06] = RTF_HEADER_SIZE;
    *(uint32_t *)&buffer[0x08] = frame->timestamp_ms;
    *(uint16_t *)&buffer[0x0C] = frame->frame_seq;
    buffer[0x0E] = frame->freq_index;
    buffer[0x0F] = frame->status;
    *(uint16_t *)&buffer[0x10] = frame->electrode_contact_mask;
    *(uint32_t *)&buffer[0x14] = RTF_MEAS_SIZE;
    *(uint32_t *)&buffer[0x18] = EIT_MESH_NODES;

    /* Raw measurements (208 × 20 bytes) */
    int offset = RTF_HEADER_SIZE;
    for (int i = 0; i < EIT_FRAME_SIZE; i++) {
        *(int32_t *)&buffer[offset + 0]  = frame->meas[i].real;
        *(int32_t *)&buffer[offset + 4]  = frame->meas[i].imag;
        *(uint32_t *)&buffer[offset + 8] = frame->meas[i].freq;
        *(uint32_t *)&buffer[offset + 12] = frame->meas[i].mag;
        *(int32_t *)&buffer[offset + 16] = frame->meas[i].phase_mdeg;
        offset += 20;
    }

    /* Conductivity array (576 × float) */
    offset = RTF_HEADER_SIZE + RTF_MEAS_SIZE;
    memcpy(&buffer[offset], frame->conductivity, RTF_COND_SIZE);

    /* Write 13 blocks (6400 bytes / 512 = 12.5 -> 13 blocks) */
    uint32_t block = s_current_block;
    for (int i = 0; i < 13; i++) {
        uint8_t block_data[512];
        memset(block_data, 0, 512);
        int copy = MIN(512, RTF_TOTAL_SIZE - i * 512);
        memcpy(block_data, &buffer[i * 512], copy);
        if (sdmmc_write_block(block + i, block_data) != 0)
            return -1;
    }

    s_current_block += 13;
    s_file_count++;
    return 0;
}

int sdlog_log_snapshot(const eit_frame_t *frame)
{
    /* A snapshot is a reduced record: just the reconstruction image.
     * For simplicity, we log the full frame. */
    return sdlog_log_frame(frame);
}

int sdlog_get_file_count(int *count)
{
    if (!count || !s_sd_initialized) return -1;
    *count = s_file_count;
    return 0;
}

int sdlog_read_file(int index, uint8_t *buffer, int max_size)
{
    if (!s_sd_initialized || !buffer || index < 0 || index >= s_file_count)
        return -1;

    int read_size = MIN(max_size, RTF_TOTAL_SIZE);
    int num_blocks = (read_size + 511) / 512;
    uint32_t start_block = index * 13;

    for (int i = 0; i < num_blocks; i++) {
        if (sdmmc_read_block(start_block + i, &buffer[i * 512]) != 0)
            return -1;
    }
    return read_size;
}

int sdlog_erase_all(void)
{
    /* Simple erase: reset block counter (data is overwritten) */
    s_current_block = 0;
    s_file_count = 0;
    return 0;
}