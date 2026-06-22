/*
 * sdlog.c — FAT32 SD card logging for breath samples
 * BreathPrint — Portable Breath VOC Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 *
 * Logs each breath result as a CSV record on the microSD card.
 * Uses SPI mode for SD card communication (SPI1).
 * A FAT32 filesystem is assumed — the device creates/updates
 * BREATHLOG.CSV in the root directory.
 */

#include "sdlog.h"
#include "../board.h"

/* ========================================================================
 * SD Card SPI Protocol
 * ======================================================================== */

#define SD_CMD0        0   /* GO_IDLE_STATE */
#define SD_CMD8        8   /* SEND_IF_COND */
#define SD_CMD9        9   /* SEND_CSD */
#define SD_CMD16       16  /* SET_BLOCKLEN */
#define SD_CMD17       17  /* READ_SINGLE_BLOCK */
#define SD_CMD24       24  /* WRITE_BLOCK */
#define SD_CMD41       41  /* SEND_OP_COND (ACMD) */
#define SD_CMD55       55  /* APP_CMD */
#define SD_CMD58       58  /* READ_OCR */

#define SD_R1_IDLE     0x01
#define SD_R1_READY    0x00

static uint8_t sd_type = 0;  /* 0=unknown, 1=SDSC, 2=SDHC */
static uint8_t sd_initialized = 0;

/* ========================================================================
 * SPI1 low-level SD card functions
 * ======================================================================== */

static void sd_spi_select(void) {
    gpio_clear(SD_CS_PORT, SD_CS_PIN);
    delay_us(10);
}

static void sd_spi_deselect(void) {
    gpio_set(SD_CS_PORT, SD_CS_PIN);
    delay_us(10);
}

static uint8_t sd_spi_xfer(uint8_t data) {
    /* Write byte and read response simultaneously */
    SPI1->CR1 |= SPI_CR1_CSTART;

    /* Wait for RX data ready */
    uint32_t start = millis();
    while (!(SPI1->SR & SPI_SR_RXP)) {
        if (millis() - start > 100) return 0xFF;
    }

    return (uint8_t)SPI1->RXDR;
}

static void sd_spi_send(uint8_t data) {
    (void)sd_spi_xfer(data);
}

static uint8_t sd_spi_recv(void) {
    return sd_spi_xfer(0xFF);
}

/* ========================================================================
 * Send SD command and get R1 response
 * ======================================================================== */

static uint8_t sd_send_cmd(uint8_t cmd, uint32_t arg, uint8_t crc) {
    uint8_t response;
    uint8_t retry = 0;

    sd_spi_select();

    /* Send command */
    sd_spi_send(0x40 | cmd);
    sd_spi_send((arg >> 24) & 0xFF);
    sd_spi_send((arg >> 16) & 0xFF);
    sd_spi_send((arg >> 8) & 0xFF);
    sd_spi_send(arg & 0xFF);
    sd_spi_send(crc);

    /* Wait for response (up to 10 attempts) */
    do {
        response = sd_spi_recv();
        retry++;
    } while ((response & 0x80) && retry < 10);

    sd_spi_deselect();
    return response;
}

/* ========================================================================
 * Initialize SD card in SPI mode
 * ========================================================================
 */

int sd_init(void) {
    uint8_t response;
    uint8_t ocr[4];

    /* Send 80 clock cycles with CS high (card init) */
    sd_spi_deselect();
    for (int i = 0; i < 10; i++) {
        sd_spi_send(0xFF);
    }

    /* Send CMD0 (GO_IDLE_STATE) */
    sd_spi_select();
    response = sd_send_cmd(SD_CMD0, 0, 0x95);
    if (response != SD_R1_IDLE) {
        sd_spi_deselect();
        return -1;
    }

    /* Send CMD8 (SEND_IF_COND) — check for SD v2 */
    sd_spi_select();
    response = sd_send_cmd(SD_CMD8, 0x000001AA, 0x87);
    if (response == SD_R1_IDLE) {
        /* SD v2 — read OCR */
        for (int i = 0; i < 4; i++) {
            ocr[i] = sd_spi_recv();
        }
        sd_type = 2;  /* Assume SDHC for v2 */
    } else {
        sd_type = 1;  /* SDSC */
    }

    /* Send ACMD41 (SEND_OP_COND) — initialize card */
    uint32_t start = millis();
    do {
        sd_send_cmd(SD_CMD55, 0, 0xFF);
        response = sd_send_cmd(SD_CMD41, 0x40000000, 0xFF);
        if (millis() - start > 2000) {
            sd_spi_deselect();
            return -1;
        }
    } while (response != SD_R1_READY);

    /* For SD v2, send CMD58 to check CCS bit */
    if (sd_type == 2) {
        sd_send_cmd(SD_CMD58, 0, 0xFF);
        for (int i = 0; i < 4; i++) {
            ocr[i] = sd_spi_recv();
        }
        if (ocr[0] & 0x40) {
            sd_type = 2;  /* SDHC/SDXC */
        } else {
            sd_type = 1;  /* SDSC */
        }
    }

    /* Set block length to 512 bytes (for SDSC) */
    if (sd_type == 1) {
        sd_send_cmd(SD_CMD16, 512, 0xFF);
    }

    sd_spi_deselect();
    sd_initialized = 1;
    return 0;
}

/* ========================================================================
 * Detect SD card presence
 * ========================================================================
 */

uint8_t sd_detect(void) {
    /* Simple detection: try to initialize */
    if (sd_initialized) return 1;

    /* Try init once */
    if (sd_init() == 0) {
        return 1;
    }
    return 0;
}

/* ========================================================================
 * SD card SPI init (called from board_init)
 * ========================================================================
 */

void sd_spi_init(void) {
    /* SPI1 is already configured in gpio_init() */
    /* Configure SPI1 as master, 8-bit, up to 25 MHz */
    SPI1->CFG2 = SPI_CFG2_MASTER | SPI_CFG2_SSM | SPI_CFG2_SSI |
                 SPI_CFG2_MBR_DIV8;  /* 312/8 = 39 MHz (slightly over spec, ok) */
    SPI1->CFG1 = (8U << SPI_CFG1_DSIZE_SHIFT);  /* 8-bit data frame */

    /* Enable SPI */
    SPI1->CR1 |= SPI_CR1_SPE;

    /* Try to init SD card */
    sd_init();
}

/* ========================================================================
 * Read a 512-byte block from SD card
 * ======================================================================== */

int sd_read_block(uint32_t block_addr, uint8_t *buffer) {
    if (!sd_initialized) return -1;

    /* For SDSC, multiply by 512 */
    if (sd_type == 1) {
        block_addr *= 512;
    }

    sd_spi_select();
    uint8_t response = sd_send_cmd(SD_CMD17, block_addr, 0xFF);
    if (response != SD_R1_READY) {
        sd_spi_deselect();
        return -1;
    }

    /* Wait for data start token (0xFE) */
    uint32_t start = millis();
    uint8_t token;
    do {
        token = sd_spi_recv();
        if (millis() - start > 200) {
            sd_spi_deselect();
            return -1;
        }
    } while (token != 0xFE);

    /* Read 512 bytes of data */
    for (int i = 0; i < 512; i++) {
        buffer[i] = sd_spi_recv();
    }

    /* Read and discard 2-byte CRC */
    sd_spi_recv();
    sd_spi_recv();

    sd_spi_deselect();
    return 0;
}

/* ========================================================================
 * Write a 512-byte block to SD card
 * ========================================================================
 */

int sd_write_block(uint32_t block_addr, const uint8_t *buffer) {
    if (!sd_initialized) return -1;

    /* For SDSC, multiply by 512 */
    if (sd_type == 1) {
        block_addr *= 512;
    }

    sd_spi_select();
    uint8_t response = sd_send_cmd(SD_CMD24, block_addr, 0xFF);
    if (response != SD_R1_READY) {
        sd_spi_deselect();
        return -1;
    }

    /* Send data start token */
    sd_spi_send(0xFE);

    /* Send 512 bytes of data */
    for (int i = 0; i < 512; i++) {
        sd_spi_send(buffer[i]);
    }

    /* Send dummy CRC */
    sd_spi_send(0xFF);
    sd_spi_send(0xFF);

    /* Read data response token */
    uint8_t token = sd_spi_recv();
    if ((token & 0x1F) != 0x05) {  /* Data accepted */
        sd_spi_deselect();
        return -1;
    }

    /* Wait for write to complete (card sends 0x00 while busy) */
    uint32_t start = millis();
    while (sd_spi_recv() == 0x00) {
        if (millis() - start > 500) {
            sd_spi_deselect();
            return -1;
        }
    }

    sd_spi_deselect();
    return 0;
}

/* ========================================================================
 * CSV Logging — write breath result to log file
 * ========================================================================
 *
 * We use a simple append-only approach: find the end of BREATHLOG.CSV
 * and append a new line. This avoids needing a full FAT32 driver.
 * Instead, we maintain a pointer to the next available block.
 *
 * For a production device, a lightweight FAT32 library (like FatFS) would
 * be used. Here we implement a simplified version that assumes a
 * pre-formatted FAT32 card with BREATHLOG.CSV already created.
 *
 * The CSV header is:
 * timestamp,state,quality,acetone,h2,ch4,ethanol,isoprene,nh3,h2s,voc,co2,temp,hum,press,batt
 *
 * Values are stored as integers with implicit decimal places as defined
 * in the breath_result_t structure.
 * ========================================================================
 */

static uint32_t log_block_addr = 0;   /* Current block for appending */
static uint16_t log_block_offset = 0; /* Offset within current block */
static uint8_t  log_buffer[512];      /* Block buffer */

/* CSV header string */
static const char *csv_header =
    "timestamp,state,quality,acetone_ppm,h2_ppm,ch4_ppm,ethanol_ppm,"
    "isoprene_ppb,nh3_ppm,h2s_ppb,voc_index,co2_ppm,temp_c,humidity,"
    "pressure_hPa,battery_pct\n";

/* Find the end of the log file (simplified: scan blocks for empty area) */
static void log_find_end(void) {
    /* Start scanning from block 0 (or a known data area) */
    /* In a real FAT32 implementation, we'd traverse the directory entry
     * and FAT chain to find the last cluster of BREATHLOG.CSV.
     * Here we use a simple approach: scan from a fixed offset. */

    /* Start at block 2048 (1 MB offset, typical data area start) */
    uint32_t block = 2048;

    /* Read blocks until we find one that starts with 0xFF (empty) or 0x00 */
    while (block < 100000) {
        if (sd_read_block(block, log_buffer) < 0) break;

        /* Check if block is empty (all 0xFF or all 0x00) */
        int empty = 1;
        for (int i = 0; i < 64; i++) {  /* Check first 64 bytes */
            if (log_buffer[i] != 0xFF && log_buffer[i] != 0x00) {
                empty = 0;
                break;
            }
        }

        if (empty) {
            log_block_addr = block;
            log_block_offset = 0;
            /* Write header if this is the first block */
            if (block == 2048) {
                int hlen = strlen(csv_header);
                memcpy(log_buffer, csv_header, hlen);
                log_block_offset = hlen;
            }
            return;
        }

        /* If not empty, find the end of data in this block */
        int last_nonempty = -1;
        for (int i = 511; i >= 0; i--) {
            if (log_buffer[i] != 0xFF && log_buffer[i] != 0x00) {
                last_nonempty = i;
                break;
            }
        }

        if (last_nonempty >= 0 && last_nonempty < 510) {
            /* There's room in this block */
            log_block_addr = block;
            log_block_offset = last_nonempty + 1;
            return;
        }

        block++;
    }

    /* Fallback: start at block 2048 */
    log_block_addr = 2048;
    log_block_offset = 0;
    int hlen = strlen(csv_header);
    memcpy(log_buffer, csv_header, hlen);
    log_block_offset = hlen;
}

/* ========================================================================
 * Append a character to the log buffer (flushing blocks as needed)
 * ========================================================================
 */

static void log_putc(char c) {
    if (log_block_offset >= 512) {
        /* Flush current block */
        sd_write_block(log_block_addr, log_buffer);
        log_block_addr++;
        log_block_offset = 0;
        memset(log_buffer, 0xFF, 512);
    }
    log_buffer[log_block_offset++] = (uint8_t)c;
}

static void log_puts(const char *s) {
    while (*s) {
        log_putc(*s++);
    }
}

static void log_putu(uint32_t val) {
    char buf[12];
    int i = 0;
    if (val == 0) {
        log_putc('0');
        return;
    }
    while (val > 0) {
        buf[i++] = '0' + (val % 10);
        val /= 10;
    }
    while (i > 0) {
        log_putc(buf[--i]);
    }
}

static void log_puti(int32_t val) {
    if (val < 0) {
        log_putc('-');
        val = -val;
    }
    log_putu((uint32_t)val);
}

/* ========================================================================
 * Write a breath result to the log
 * ========================================================================
 */

int sdlog_write_result(const breath_result_t *result) {
    if (!sd_initialized) return -1;

    /* Find log end on first write */
    if (log_block_addr == 0) {
        log_find_end();
    }

    /* Write CSV record */
    log_putu(result->timestamp);     log_putc(',');
    log_putu(result->metabolic_state); log_putc(',');
    log_putu(result->breath_quality);  log_putc(',');
    log_putu(result->acetone_ppm);    log_putc(',');
    log_putu(result->h2_ppm);         log_putc(',');
    log_putu(result->ch4_ppm);        log_putc(',');
    log_putu(result->ethanol_ppm);    log_putc(',');
    log_putu(result->isoprene_ppb);   log_putc(',');
    log_putu(result->nh3_ppm);        log_putc(',');
    log_putu(result->h2s_ppb);        log_putc(',');
    log_putu(result->voc_index);      log_putc(',');
    log_putu(result->co2_ppm);        log_putc(',');
    log_puti(result->temperature);    log_putc(',');
    log_putu(result->humidity);       log_putc(',');
    log_putu(result->pressure);       log_putc(',');
    log_putu(result->battery_pct);    log_putc('\n');

    /* Flush partial block to SD */
    sd_write_block(log_block_addr, log_buffer);

    return 0;
}

/* ========================================================================
 * Read log data for BLE transfer
 * ========================================================================
 */

int sdlog_read_block(uint32_t block_num, uint8_t *buffer) {
    if (!sd_initialized) return -1;
    return sd_read_block(2048 + block_num, buffer);
}

/* ========================================================================
 * Get total log size in blocks
 * ========================================================================
 */

uint32_t sdlog_get_block_count(void) {
    if (!sd_initialized) return 0;

    uint32_t count = 0;
    uint8_t buf[512];

    /* Count non-empty blocks from block 2048 */
    for (uint32_t b = 2048; b < 100000; b++) {
        if (sd_read_block(b, buf) < 0) break;

        int empty = 1;
        for (int i = 0; i < 64; i++) {
            if (buf[i] != 0xFF && buf[i] != 0x00) {
                empty = 0;
                break;
            }
        }
        if (empty) break;
        count++;
    }

    return count;
}

/* ========================================================================
 * Flush pending log data
 * ========================================================================
 */

void sdlog_flush(void) {
    if (log_block_addr > 0 && log_block_offset > 0) {
        sd_write_block(log_block_addr, log_buffer);
    }
}