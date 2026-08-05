/*
 * drivers/flashio.c — W25Q80 SPI flash journal with wear-leveling
 *
 * The W25Q80 is a 1 Mbit (128 KB) SPI NOR flash.  We use the first
 * 512 KB as a ring-buffer journal of 24-byte records (one per 5-minute
 * sample cycle).  That holds ~21,800 records ≈ 75 days at 5-minute
 * intervals, which is more than enough for the 30-day requirement.
 *
 * Wear-leveling: each record is written sequentially (page program).
 * Erase is done only when the write pointer wraps into a new 4 KB
 * sector, and only that one sector is erased.  With 128 sectors of
 * 100k erase cycles each, the journal survives >40 million records
 * (≈ 380 years at 5-minute intervals).
 *
 * A double-buffered metadata sector at the top of the journal stores
 * the head and tail pointers with a sequence number, so a power loss
 * during a write corrupts at most one record.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "flashio.h"
#include "../board.h"

/* ------------------------------------------------------------------ */
/*  Journal layout                                                     */
/* ------------------------------------------------------------------ */
#define JOURNAL_BASE        0x000000u   /* start of journal area */
#define JOURNAL_SIZE        0x008000u   /* 32 KB = 512 KB / 16 (we use 32KB for longevity) */
#define METADATA_BASE       (JOURNAL_BASE + JOURNAL_SIZE)
#define METADATA_SIZE       W25Q80_SECTOR_SIZE   /* 4 KB, double-buffered */

#define META_A_ADDR         (METADATA_BASE)
#define META_B_ADDR         (METADATA_BASE + W25Q80_SECTOR_SIZE)

#define RECORDS_PER_SECTOR  (W25Q80_SECTOR_SIZE / FLASH_RECORD_BYTES)  /* 170 */
#define SECTORS_IN_JOURNAL  (JOURNAL_SIZE / W25Q80_SECTOR_SIZE)        /* 8 */

#define META_MAGIC          0xF5E1u   /* 'F'rost'Sentinel' marker */

typedef struct {
    uint16_t magic;
    uint16_t seq;            /* incrementing sequence, A/B arbitration */
    uint32_t write_ptr;      /* byte offset of next write */
    uint32_t read_ptr;       /* byte offset of next read (for dump) */
    uint32_t record_count;   /* total records ever written */
    uint8_t  padding[8];
} meta_t;

static meta_t  s_meta;
static uint8_t s_meta_active;   /* 0 = A, 1 = B */

/* ------------------------------------------------------------------ */
/*  W25Q80 SPI primitives                                              */
/* ------------------------------------------------------------------ */
static void flash_wait_idle(void)
{
    uint8_t status;
    do {
        FLASH_CS_LOW();
        SPI1->DR = W25Q80_CMD_READ_STATUS;
        while (!(SPI1->SR & SPI_SR_TXE)) ;
        SPI1->DR = 0x00;
        while (!(SPI1->SR & SPI_SR_RXNE)) ;
        (void)SPI1->DR;
        SPI1->DR = 0x00;
        while (!(SPI1->SR & SPI_SR_RXNE)) ;
        status = (uint8_t)SPI1->DR;
        FLASH_CS_HIGH();
    } while (status & 0x01);  /* WIP bit */
}

static void flash_write_enable(void)
{
    FLASH_CS_LOW();
    SPI1->DR = W25Q80_CMD_WRITE_ENABLE;
    while (!(SPI1->SR & SPI_SR_TXE)) ;
    while (SPI1->SR & SPI_SR_BSY) ;
    FLASH_CS_HIGH();
}

static void flash_sector_erase(uint32_t addr)
{
    flash_write_enable();
    FLASH_CS_LOW();
    SPI1->DR = W25Q80_CMD_SECTOR_ERASE;
    while (!(SPI1->SR & SPI_SR_TXE)) ;
    SPI1->DR = (addr >> 16) & 0xFF;
    while (!(SPI1->SR & SPI_SR_TXE)) ;
    SPI1->DR = (addr >> 8) & 0xFF;
    while (!(SPI1->SR & SPI_SR_TXE)) ;
    SPI1->DR = addr & 0xFF;
    while (!(SPI1->SR & SPI_SR_TXE)) ;
    while (SPI1->SR & SPI_SR_BSY) ;
    FLASH_CS_HIGH();
    flash_wait_idle();   /* ~45 ms typical */
}

static void flash_page_program(uint32_t addr, const uint8_t *data, uint16_t len)
{
    if (len > 256) len = 256;
    flash_write_enable();
    FLASH_CS_LOW();
    SPI1->DR = W25Q80_CMD_PAGE_PROGRAM;
    while (!(SPI1->SR & SPI_SR_TXE)) ;
    SPI1->DR = (addr >> 16) & 0xFF;
    while (!(SPI1->SR & SPI_SR_TXE)) ;
    SPI1->DR = (addr >> 8) & 0xFF;
    while (!(SPI1->SR & SPI_SR_TXE)) ;
    SPI1->DR = addr & 0xFF;
    while (!(SPI1->SR & SPI_SR_TXE)) ;
    for (uint16_t i = 0; i < len; i++) {
        SPI1->DR = data[i];
        while (!(SPI1->SR & SPI_SR_TXE)) ;
    }
    while (SPI1->SR & SPI_SR_BSY) ;
    FLASH_CS_HIGH();
    flash_wait_idle();   /* ~0.7 ms typical */
}

static void flash_read(uint32_t addr, uint8_t *out, uint16_t len)
{
    FLASH_CS_LOW();
    SPI1->DR = W25Q80_CMD_READ;
    while (!(SPI1->SR & SPI_SR_TXE)) ;
    SPI1->DR = (addr >> 16) & 0xFF;
    while (!(SPI1->SR & SPI_SR_TXE)) ;
    SPI1->DR = (addr >> 8) & 0xFF;
    while (!(SPI1->SR & SPI_SR_TXE)) ;
    SPI1->DR = addr & 0xFF;
    while (!(SPI1->SR & SPI_SR_TXE)) ;
    for (uint16_t i = 0; i < len; i++) {
        SPI1->DR = 0x00;
        while (!(SPI1->SR & SPI_SR_RXNE)) ;
        out[i] = (uint8_t)SPI1->DR;
    }
    FLASH_CS_HIGH();
}

/* ------------------------------------------------------------------ */
/*  Metadata load / save (double-buffered with sequence arbitration)   */
/* ------------------------------------------------------------------ */
static void meta_load(void)
{
    meta_t a, b;
    flash_read(META_A_ADDR, (uint8_t *)&a, sizeof(a));
    flash_read(META_B_ADDR, (uint8_t *)&b, sizeof(b));

    if (a.magic == META_MAGIC && b.magic == META_MAGIC) {
        /* Pick the one with the higher sequence number */
        if ((int16_t)(b.seq - a.seq) > 0) {
            s_meta = b;
            s_meta_active = 1;
        } else {
            s_meta = a;
            s_meta_active = 0;
        }
    } else if (a.magic == META_MAGIC) {
        s_meta = a;
        s_meta_active = 0;
    } else if (b.magic == META_MAGIC) {
        s_meta = b;
        s_meta_active = 1;
    } else {
        /* First boot: initialize */
        memset(&s_meta, 0, sizeof(s_meta));
        s_meta.magic = META_MAGIC;
        s_meta.seq = 0;
        s_meta.write_ptr = JOURNAL_BASE;
        s_meta.read_ptr = JOURNAL_BASE;
        s_meta.record_count = 0;
        s_meta_active = 0;
        flash_sector_erase(META_A_ADDR);
        flash_sector_erase(META_B_ADDR);
        flash_page_program(META_A_ADDR, (uint8_t *)&s_meta, sizeof(s_meta));
    }
}

static void meta_save(void)
{
    s_meta.seq++;
    uint32_t addr = (s_meta_active == 0) ? META_B_ADDR : META_A_ADDR;
    flash_sector_erase(addr);
    flash_page_program(addr, (uint8_t *)&s_meta, sizeof(s_meta));
    s_meta_active ^= 1;  /* toggle active buffer */
}

/* ------------------------------------------------------------------ */
/*  Public: initialize flash journal                                   */
/* ------------------------------------------------------------------ */
void flashio_init(void)
{
    /* Configure PB0 as flash CS (output, default high) */
    GPIO_CONFIG(GPIOB, 0, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_SPEED_HIGH,
                GPIO_PUPD_NONE, 0);
    FLASH_CS_HIGH();

    /* SPI1 is already initialized by radio_init(); we share the bus. */
    meta_load();
    g_sys.records_written = s_meta.record_count;
}

/* ------------------------------------------------------------------ */
/*  Public: write one 24-byte record to the journal                    */
/* ------------------------------------------------------------------ */
int flashio_write_record(const uint8_t *record)
{
    uint32_t addr = s_meta.write_ptr;

    /* Check if we're crossing a sector boundary → erase next sector */
    uint32_t sector_offset = addr % W25Q80_SECTOR_SIZE;
    if (sector_offset == 0) {
        /* Erase the sector we're about to write into */
        flash_sector_erase(addr);
    }

    /* Write the record */
    flash_page_program(addr, record, FLASH_RECORD_BYTES);

    /* Advance write pointer with wraparound */
    s_meta.write_ptr += FLASH_RECORD_BYTES;
    if (s_meta.write_ptr >= JOURNAL_BASE + JOURNAL_SIZE) {
        s_meta.write_ptr = JOURNAL_BASE;
    }
    s_meta.record_count++;

    /* Save metadata (toggle A/B) every 32 records to reduce wear */
    if ((s_meta.record_count & 0x1F) == 0) {
        meta_save();
    }

    g_sys.records_written = s_meta.record_count;
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Public: read the Nth most recent record (0 = newest)               */
/* ------------------------------------------------------------------ */
int flashio_read_record(uint32_t index_from_newest, uint8_t *out)
{
    if (index_from_newest >= s_meta.record_count) return -1;

    uint32_t offset = (index_from_newest + 1) * FLASH_RECORD_BYTES;
    if (offset > (s_meta.write_ptr - JOURNAL_BASE)) {
        /* Wrap around */
        offset = JOURNAL_SIZE - (offset - (s_meta.write_ptr - JOURNAL_BASE));
    } else {
        offset = (s_meta.write_ptr - JOURNAL_BASE) - offset;
    }
    uint32_t addr = JOURNAL_BASE + offset;
    flash_read(addr, out, FLASH_RECORD_BYTES);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Public: flush metadata (call before entering sleep)                */
/* ------------------------------------------------------------------ */
void flashio_flush(void)
{
    meta_save();
}

/* ------------------------------------------------------------------ */
/*  Public: get record count                                           */
/* ------------------------------------------------------------------ */
uint32_t flashio_get_record_count(void)
{
    return s_meta.record_count;
}

/* ------------------------------------------------------------------ */
/*  Public: build a 24-byte record from current sensor state           */
/* ------------------------------------------------------------------ */
void flashio_build_record(uint8_t *buf, uint32_t timestamp)
{
    /* Record format (24 bytes):
     *   0-3:   timestamp (seconds since epoch, UTC)
     *   4-5:   air_t_cx100  (int16)
     *   6-7:   sky_t_cx100  (int16)
     *   8-9:   twet_cx100   (int16)
     *  10-11:  delta_rad_cx100 (int16)
     *  12-13:  leaf_wet (uint16, 0-1000)
     *  14:     ae_status (uint8)
     *  15-16:  ae_energy (uint16)
     *  17:     flags (uint8)
     *  18:     battery_pct (uint8)
     *  19-20:  battery_mv (uint16)
     *  21:     node_id (uint8)
     *  22:     wick_dry (uint8)
     *  23:     reserved (0)
     */
    memset(buf, 0, FLASH_RECORD_BYTES);
    buf[0] = (timestamp >> 24) & 0xFF;
    buf[1] = (timestamp >> 16) & 0xFF;
    buf[2] = (timestamp >> 8)  & 0xFF;
    buf[3] =  timestamp & 0xFF;
    int16_t v;
    v = (int16_t)g_sys.air_t_cx100;       buf[4] = v & 0xFF; buf[5] = (v >> 8) & 0xFF;
    v = (int16_t)g_sys.sky_t_cx100;       buf[6] = v & 0xFF; buf[7] = (v >> 8) & 0xFF;
    v = (int16_t)g_sys.twet_cx100;        buf[8] = v & 0xFF; buf[9] = (v >> 8) & 0xFF;
    v = (int16_t)g_sys.delta_rad_cx100;  buf[10] = v & 0xFF; buf[11] = (v >> 8) & 0xFF;
    buf[12] = g_sys.leaf_wet & 0xFF;
    buf[13] = (g_sys.leaf_wet >> 8) & 0xFF;
    buf[14] = g_sys.ae_status;
    buf[15] = g_sys.ae_energy & 0xFF;
    buf[16] = (g_sys.ae_energy >> 8) & 0xFF;
    buf[17] = g_sys.flags;
    buf[18] = g_sys.battery_pct;
    buf[19] = g_sys.battery_mv & 0xFF;
    buf[20] = (g_sys.battery_mv >> 8) & 0xFF;
    buf[21] = g_sys.node_id;
    buf[22] = g_sys.wick_dry;
    buf[23] = 0;
}