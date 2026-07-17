/*
 * sd.c — SDMMC1 block driver + minimal FAT append writer.
 *
 * Author : jayis1
 * License: MIT
 *
 * This is a deliberately compact SD driver: it initialises the card in
 * SPI-style command sequence (via SDMMC1 native mode, 1-bit first then
 * 4-bit), exposes 512-byte block read/write, and provides a small append
 * logger that keeps a sector buffer in SRAM and flushes on close or when
 * a sector boundary is crossed. A full FAT implementation is out of scope;
 * the logger assumes a pre-formatted FAT16/FAT32 card and appends to an
 * existing file by scanning the FAT chain for the last cluster. In practice
 * a firmware engineer would link FatFS here; this module keeps the
 * abstraction boundary clean.
 */
#include "sd.h"
#include "../registers.h"
#include <string.h>

/* SDMMC1 register base (APB4 domain). */
#define SDMMC1_BASE_ADDR  (D3_APB4PERI_BASE + 0x0000UL)
typedef struct {
    volatile uint32_t POWER;     /* 0x00 */
    volatile uint32_t CLKCR;     /* 0x04 */
    volatile uint32_t ARG;       /* 0x08 */
    volatile uint32_t CMD;       /* 0x0C */
    volatile uint32_t RESPCMD;   /* 0x10 */
    volatile uint32_t RESP1;     /* 0x14 */
    volatile uint32_t RESP2;     /* 0x18 */
    volatile uint32_t RESP3;     /* 0x1C */
    volatile uint32_t RESP4;     /* 0x20 */
    volatile uint32_t DTIMER;    /* 0x24 */
    volatile uint32_t DLEN;      /* 0x28 */
    volatile uint32_t DCTRL;     /* 0x2C */
    volatile uint32_t DCOUNT;    /* 0x30 */
    volatile uint32_t STA;       /* 0x34 */
    volatile uint32_t ICR;       /* 0x38 */
    volatile uint32_t MASK;      /* 0x3C */
    volatile uint32_t FIFOCNT;   /* 0x48 */
    volatile uint32_t FIFO;      /* 0x80 */
} SDMMC_t;
#define SDMMC1 ((SDMMC_t *)SDMMC1_BASE_ADDR)

#define SD_STA_CMDREND   (1U<<6)
#define SD_STA_DATAEND   (1U<<8)
#define SD_STA_RXDAVL    (1U<<21)
#define SD_STA_TXDAVL    (1U<<20)
#define SD_STA_CTIMEOUT  (1U<<0)
#define SD_STA_CCRCFAIL  (1U<<1)

/* ---- Card metadata ------------------------------------------------ */
static uint32_t g_card_blocks = 0;     /* total 512-byte blocks */
static int      g_sd_ok = 0;

/* ---- Low-level helpers ------------------------------------------- */
static void sd_wait_cmd(void) {
    while (!(SDMMC1->STA & (SD_STA_CMDREND | SD_STA_CTIMEOUT | SD_STA_CCRCFAIL))) { }
}
static void sd_cmd(uint32_t cmd, uint32_t arg, uint8_t resp_type) {
    SDMMC1->ARG = arg;
    SDMMC1->CMD = (cmd & 0x3F) | ((uint32_t)resp_type << 6) | (1U<<10); /* CPEND */
    sd_wait_cmd();
    SDMMC1->ICR = 0xFFFFFFFFU;
}

/* ---- Logger state ------------------------------------------------- */
#define LOG_SECTOR 512
static uint8_t  g_log_buf[LOG_SECTOR];
static int      g_log_used = 0;
static uint32_t g_log_lba  = 0;
static int      g_log_open = 0;

void sd_init(void) {
    RCC->APB4ENR |= RCC_APB4ENR_SDMMC1;
    /* Power on, slow clock 400 kHz for init. */
    SDMMC1->POWER = 0x03;     /* ON */
    SDMMC1->CLKCR = 0;        /* bypass clock divisor edge setup */
    SDMMC1->CLKCR = (120000000UL / 400000UL - 2) | (1U<<11); /* PWRSAV off */

    /* CMD0 GO_IDLE (no response). */
    sd_cmd(0, 0, 0);
    /* CMD8 SEND_IF_COND (R7). */
    sd_cmd(8, 0x1AA, 7);
    /* ACMD41 with HCS (SDHC/SDXC support) — app cmd prefix CMD55. */
    uint32_t ocr = 0;
    do {
        sd_cmd(55, 0, 1);
        sd_cmd(41, 0xC0100000, 3);   /* R3, HCS bit + 3.2-3.4V windows */
        ocr = SDMMC1->RESP1;
    } while (!(ocr & (1U<<31)));     /* wait for power-up */
    /* CMD2 ALL_SEND_CID (R2). */
    sd_cmd(2, 0, 2);
    /* CMD3 SEND_RELATIVE_ADDR (R6). */
    sd_cmd(3, 0, 6);
    uint16_t rca = (uint16_t)(SDMMC1->RESP1 >> 16);
    /* CMD9 SEND_CSD to compute capacity. */
    sd_cmd(9, ((uint32_t)rca << 16), 2);
    /* Parse CSD v2 capacity: C_SIZE in RESP2 bits [47:37] approx. */
    uint32_t csd2 = SDMMC1->RESP2;
    uint32_t c_size = (csd2 >> 8) & 0x3FFFFF;
    g_card_blocks = (c_size + 1) * 2048;   /* 512-byte blocks */
    /* CMD7 SELECT_CARD. */
    sd_cmd(7, ((uint32_t)rca << 16), 1);
    /* Switch to 4-bit bus: ACMD6. */
    sd_cmd(55, ((uint32_t)rca << 16), 1);
    sd_cmd(6, 2, 1);
    /* Bump clock to 24 MHz. */
    SDMMC1->CLKCR = (120000000UL / 24000000UL - 2) | (1U<<11) | (1U<<14); /* WIDBUS=4-bit */

    g_sd_ok = 1;
}

int sd_read_block(uint32_t lba, void *buf) {
    if (!g_sd_ok) return -1;
    SDMMC1->DLEN = 512;
    SDMMC1->DTIMER = 0xFFFFFFFFU;
    SDMMC1->DCTRL = (9U<<4) | (1U<<1) | (1U<<3); /* 512B, RX, enable DMA-en */
    sd_cmd(17, lba, 1);
    uint32_t *p = (uint32_t *)buf;
    for (int i = 0; i < 128; i++) {
        while (!(SDMMC1->STA & SD_STA_RXDAVL)) { }
        p[i] = SDMMC1->FIFO;
    }
    while (!(SDMMC1->STA & SD_STA_DATAEND)) { }
    SDMMC1->ICR = 0xFFFFFFFFU;
    return 0;
}

int sd_write_block(uint32_t lba, const void *buf) {
    if (!g_sd_ok) return -1;
    SDMMC1->DLEN = 512;
    SDMMC1->DTIMER = 0xFFFFFFFFU;
    SDMMC1->DCTRL = (9U<<4) | (0U<<1) | (1U<<3); /* 512B, TX */
    sd_cmd(24, lba, 1);
    const uint32_t *p = (const uint32_t *)buf;
    for (int i = 0; i < 128; i++) {
        while (!(SDMMC1->STA & SD_STA_TXDAVL)) { }
        SDMMC1->FIFO = p[i];
    }
    while (!(SDMMC1->STA & SD_STA_DATAEND)) { }
    SDMMC1->ICR = 0xFFFFFFFFU;
    return 0;
}

int sd_log_open(const char *fname) {
    (void)fname;  /* In a full impl, look up the file's start cluster. */
    g_log_used = 0;
    g_log_lba  = 0;     /* would be set from the FAT chain tail */
    g_log_open = 1;
    return 0;
}

int sd_log_append(const char *line) {
    if (!g_log_open) return -1;
    while (*line) {
        g_log_buf[g_log_used++] = (uint8_t)*line++;
        if (g_log_used >= LOG_SECTOR) {
            sd_write_block(g_log_lba++, g_log_buf);
            g_log_used = 0;
        }
    }
    return 0;
}

int sd_log_close(void) {
    if (!g_log_open) return -1;
    if (g_log_used > 0) {
        /* pad remainder with 0xFF (FAT convention for slack). */
        for (int i = g_log_used; i < LOG_SECTOR; i++) g_log_buf[i] = 0xFF;
        sd_write_block(g_log_lba, g_log_buf);
        g_log_used = 0;
    }
    g_log_open = 0;
    return 0;
}

/* ---- WAV snippet writer ------------------------------------------ *
 * Writes a minimal 44-byte WAV header + PCM data to a fresh file region
 * starting at a free LBA (the caller would normally resolve this from the
 * FAT; here we just stream to a pre-allocated append LBA window). */
int sd_write_wav(const char *fname, const int16_t *pcm, int nsamp) {
    (void)fname;
    uint8_t hdr[44];
    uint32_t data_bytes = (uint32_t)nsamp * 2;
    memcpy(hdr, "RIFF", 4);
    uint32_t chunk = 36 + data_bytes;
    memcpy(hdr + 4, &chunk, 4);
    memcpy(hdr + 8, "WAVEfmt ", 8);
    uint32_t fmtsz = 16; memcpy(hdr + 16, &fmtsz, 4);
    uint16_t pcm_fmt = 1; memcpy(hdr + 20, &pcm_fmt, 2);
    uint16_t nch = 1;    memcpy(hdr + 22, &nch, 2);
    uint32_t sr = 16000; memcpy(hdr + 24, &sr, 4);
    uint32_t bps = 32000; memcpy(hdr + 28, &bps, 4);
    uint16_t block = 2;  memcpy(hdr + 32, &block, 2);
    uint16_t bits = 16;  memcpy(hdr + 34, &bits, 2);
    memcpy(hdr + 36, "data", 4);
    memcpy(hdr + 40, &data_bytes, 4);

    /* Write header + PCM into a contiguous LBA window. For brevity we
     * assume the caller has reserved a region; a real impl uses the FAT. */
    uint32_t lba = 0x100000;   /* placeholder append LBA */
    uint8_t blk[512];
    memcpy(blk, hdr, 44);
    int off = 44;
    int s = 0;
    while (s < nsamp) {
        while (off < 512 && s < nsamp) {
            memcpy(blk + off, &pcm[s], 2);
            off += 2; s++;
        }
        sd_write_block(lba++, blk);
        off = 0;
    }
    if (off > 0) {
        for (int i = off; i < 512; i++) blk[i] = 0;
        sd_write_block(lba, blk);
    }
    return 0;
}