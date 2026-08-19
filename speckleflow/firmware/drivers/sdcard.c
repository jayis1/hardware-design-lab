/*
 * sdcard.c — microSD card SDIO 4-bit logging driver for SpeckleFlow
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * Implements a minimal SDIO 4-bit driver for writing flow-map frames
 * and session metadata to a FAT32-formatted microSD card. We implement
 * a simple flat-file scheme: each session creates a file SSSSNNNN.BIN
 * where SSSS is the session number and NNNN is the frame number.
 *
 * For simplicity, we write raw frames (no FAT filesystem parsing in
 * this driver — we assume the card is pre-formatted FAT32 and we
 * append to a pre-allocated file). A full FAT32 driver would be too
 * large for this example; the interface is designed to be compatible
 * with an external FAT library (e.g., FatFs via ff.c).
 */

#include "sdcard.h"
#include "board.h"
#include "registers.h"
#include <string.h>

/* SDMMC commands */
#define SD_CMD_GO_IDLE_STATE     0
#define SD_CMD_SEND_IF_COND      8
#define SD_CMD_APP_CMD           55
#define SD_CMD_SD_SEND_OP_COND   41
#define SD_CMD_ALL_SEND_CID      2
#define SD_CMD_SEND_RELATIVE_ADDR 3
#define SD_CMD_SELECT_CARD       7
#define SD_CMD_SET_BLOCKLEN      16
#define SD_CMD_READ_SINGLE_BLOCK 17
#define SD_CMD_WRITE_SINGLE_BLOCK 24
#define SD_CMD_APP_SD_SEND_OP_COND 41

#define SD_RESP_R1  1
#define SD_RESP_R3  3
#define SD_RESP_R6  6
#define SD_RESP_R7  7

static uint32_t session_frame_count = 0;
static uint32_t session_number = 0;
static int sd_initialized = 0;

/* ---- SDMMC primitives --------------------------------------------------- */

static void sd_wait_cmdrend(void) {
    while (!(SDMMC1->STA & SDMMC_STA_CMDREND)) { }
}

static int sd_send_cmd(uint32_t cmd, uint32_t arg, uint32_t resp_type) {
    uint32_t cmdreg = cmd | SDMMC_CMD_CPSMEN;
    if (resp_type == SD_RESP_R3 || resp_type == SD_RESP_R6 || resp_type == SD_RESP_R7)
        cmdreg |= (1u << 6) | (1u << 7) | (1u << 9);  /* wait response, long or short */
    else if (resp_type == SD_RESP_R1)
        cmdreg |= (1u << 6) | (1u << 9);  /* wait short response */

    SDMMC1->ARG = arg;
    SDMMC1->CMD = cmdreg;

    if (resp_type == 0) {
        /* No response expected */
        for (volatile int i = 0; i < 1000; i++) { }
        SDMMC1->ICR = SDMMC_ICR_CLEAR_ALL;
        return 0;
    }

    /* Wait for command response */
    uint32_t timeout = 1000000;
    while (!(SDMMC1->STA & SDMMC_STA_CMDREND)) {
        if (--timeout == 0) {
            SDMMC1->ICR = SDMMC_ICR_CLEAR_ALL;
            return -1;
        }
    }
    SDMMC1->ICR = SDMMC_ICR_CLEAR_ALL;
    return (int)(SDMMC1->RESP1 & 0xFF);  /* return R1 status bits */
}

static int sd_write_block(uint32_t block_addr, const uint8_t *data) {
    /* Set up data transfer */
    SDMMC1->DLEN = 512;
    SDMMC1->DCTRL = (1u << 0) |  /* DTEN */
                    (9u << 4) |  /* block size = 2^9 = 512 */
                    (1u << 3) |  /* write */
                    (1u << 1);   /* block mode */

    /* Send CMD24 (WRITE_SINGLE_BLOCK) */
    sd_send_cmd(SD_CMD_WRITE_SINGLE_BLOCK, block_addr, SD_RESP_R1);

    /* Write 512 bytes via FIFO (polling for simplicity) */
    for (int i = 0; i < 128; i++) {
        while (!(SDMMC1->STA & (1u << 19))) {  /* TXFIFOE */
            if (SDMMC1->STA & (1u << 1)) return -1;  /* TXUNDERR */
        }
        uint32_t word = ((uint32_t)data[i*4]) |
                        ((uint32_t)data[i*4+1] << 8) |
                        ((uint32_t)data[i*4+2] << 16) |
                        ((uint32_t)data[i*4+3] << 24);
        SDMMC1->FIFO = word;
    }

    /* Wait for data end */
    uint32_t timeout = 10000000;
    while (!(SDMMC1->STA & SDMMC_STA_DATAEND)) {
        if (--timeout == 0) return -1;
    }
    SDMMC1->ICR = SDMMC_ICR_CLEAR_ALL;
    return 0;
}

/* ---- Public API --------------------------------------------------------- */

int sdcard_init(void) {
    /* 1. Power on SDMMC1 */
    SDMMC1->POWER = SDMMC_POWER_PWRCTRL_ON;
    for (volatile int i = 0; i < 100000; i++) { }

    /* 2. Set clock: 400 kHz for initialization */
    SDMMC1->CLKCR = 0;  /* clock divide = 0 → use default */
    SDMMC1->CLKCR |= SDMMC_CLKCR_CLKEN;

    /* 3. Send CMD0 (GO_IDLE_STATE) — reset card */
    sd_send_cmd(SD_CMD_GO_IDLE_STATE, 0, 0);

    /* 4. Send CMD8 (SEND_IF_COND) — check voltage */
    int resp = sd_send_cmd(SD_CMD_SEND_IF_COND, 0x1AA, SD_RESP_R7);
    if (resp < 0) return -1;

    /* 5. Send ACMD41 (SD_SEND_OP_COND) — initialize card */
    for (int retry = 0; retry < 100; retry++) {
        sd_send_cmd(SD_CMD_APP_CMD, 0, SD_RESP_R1);
        sd_send_cmd(SD_CMD_SD_SEND_OP_COND, 0x80100000, SD_RESP_R3);
        if (SDMMC1->RESP1 & (1u << 31)) break;  /* busy bit cleared */
        for (volatile int i = 0; i < 100000; i++) { }
    }

    /* 6. Send CMD2 (ALL_SEND_CID) */
    sd_send_cmd(SD_CMD_ALL_SEND_CID, 0, SD_RESP_R3);

    /* 7. Send CMD3 (SEND_RELATIVE_ADDR) */
    sd_send_cmd(SD_CMD_SEND_RELATIVE_ADDR, 0, SD_RESP_R6);
    uint32_t rca = (SDMMC1->RESP1 >> 16) & 0xFFFF;

    /* 8. Select card with CMD7 */
    sd_send_cmd(SD_CMD_SELECT_CARD, (rca << 16), SD_RESP_R1);

    /* 9. Set block length to 512 */
    sd_send_cmd(SD_CMD_SET_BLOCKLEN, 512, SD_RESP_R1);

    /* 10. Increase clock to 25 MHz for data transfer */
    SDMMC1->CLKCR = (1u << 0) | (2u << 0) | SDMMC_CLKCR_CLKEN;  /* div by 2 */

    sd_initialized = 1;
    return 0;
}

int sdcard_start_session(void) {
    session_frame_count = 0;
    session_number++;
    return 0;
}

int sdcard_log_frame(const uint8_t *flow_map, uint32_t frame_size,
                     uint32_t timestamp_ms) {
    if (!sd_initialized) return -1;

    /* Write a 512-byte header followed by the frame data.
     * Each frame occupies: 8 bytes (timestamp) + frame_size bytes,
     * padded to 512-byte blocks. */
    uint8_t header[16];
    header[0] = 'S';  /* magic */
    header[1] = 'P';
    header[2] = 'F';  /* SpeckleFlow */
    header[3] = '1';  /* version */
    header[4] = (uint8_t)(session_number & 0xFF);
    header[5] = (uint8_t)((session_number >> 8) & 0xFF);
    header[6] = (uint8_t)(session_frame_count & 0xFF);
    header[7] = (uint8_t)((session_frame_count >> 8) & 0xFF);
    header[8] = (uint8_t)(timestamp_ms & 0xFF);
    header[9] = (uint8_t)((timestamp_ms >> 8) & 0xFF);
    header[10] = (uint8_t)((timestamp_ms >> 16) & 0xFF);
    header[11] = (uint8_t)((timestamp_ms >> 24) & 0xFF);
    header[12] = (uint8_t)(frame_size & 0xFF);
    header[13] = (uint8_t)((frame_size >> 8) & 0xFF);
    header[14] = (uint8_t)((frame_size >> 16) & 0xFF);
    header[15] = (uint8_t)((frame_size >> 24) & 0xFF);

    /* Calculate block address (each session starts at a fixed offset) */
    uint32_t base_block = session_number * 8192u;  /* 4 MB per session */
    uint32_t frame_blocks = (frame_size + 511) / 512;
    uint32_t block = base_block + 1 + session_frame_count * (frame_blocks + 1);

    /* Write header */
    uint8_t hdr_block[512];
    memset(hdr_block, 0, sizeof(hdr_block));
    memcpy(hdr_block, header, sizeof(header));
    if (sd_write_block(block, hdr_block) != 0) return -1;
    block++;

    /* Write frame data in 512-byte blocks */
    uint32_t offset = 0;
    while (offset < frame_size) {
        uint8_t blk[512];
        uint32_t to_copy = frame_size - offset;
        if (to_copy > 512) to_copy = 512;
        memset(blk, 0, sizeof(blk));
        memcpy(blk, flow_map + offset, to_copy);
        if (sd_write_block(block, blk) != 0) return -1;
        block++;
        offset += to_copy;
    }

    session_frame_count++;
    return 0;
}

uint32_t sdcard_get_session_number(void) {
    return session_number;
}

uint32_t sdcard_get_frame_count(void) {
    return session_frame_count;
}

int sdcard_is_present(void) {
    return (SD_CD_PORT->IDR & (1u << SD_CD_PIN)) ? 0 : 1;
}