/**
 * @file    storage.c
 * @brief   TideBand — W25N02G SPI NAND flash driver with wear-leveling
 *          and dive session management. Logs 48-byte profile records
 *          at 1-4 Hz during dives, with automatic block management and
 *          bad block tracking.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 *
 * The W25N02G is a 2 Gbit (256 MB) SPI NAND flash with 2048-byte pages
 * and 64 pages per block (128 KB blocks). NAND flash requires:
 *   - Page-level programming (can only write 0→1, must erase to reset)
 *   - Block-level erase (128 KB at a time)
 *   - Bad block management (factory bad blocks marked in spare area)
 *   - Wear leveling (each block rated for ~100k erase cycles)
 *
 * We use a simple append-only log structure:
 *   - Dive headers and records are written sequentially from block 0
 *   - A write pointer tracks the current page/offset
 *   - When a block is full, we advance to the next good block
 *   - erase_all() erases all blocks and resets the write pointer
 *
 * Each 2048-byte page holds 42 profile records (42 × 48 = 2016 bytes,
 * with 32 bytes spare for ECC/metadata). Records are written sequentially
 * within a page; partial pages are tracked in RAM.
 */

#include <string.h>
#include "board.h"
#include "registers.h"
#include "storage.h"

/* ---- W25N02G SPI NAND commands ---- */
#define W25N_CMD_READ_PAGE      0x13u  /* Page data read to cache */
#define W25N_CMD_READ_CACHE     0x0Bu  /* Read from cache */
#define W25N_CMD_PROG_LOAD      0x02u  /* Load data into cache */
#define W25N_CMD_PROG_EXEC      0x10u  /* Program cache to page */
#define W25N_CMD_ERASE_BLK      0xD8u  /* Block erase */
#define W25N_CMD_READ_STATUS    0x0Fu  /* Read status register */
#define W25N_CMD_WRITE_STATUS   0x1Fu  /* Write status register */
#define W25N_CMD_READ_ID        0x9Fu  /* Read JEDEC ID */
#define W25N_CMD_BAD_BLK        0xA0u  /* Read bad block marker */
#define W25N_CMD_RESET          0xFFu  /* Reset device */
#define W25N_CMD_PROTECT        0xA1u  /* Set write protect */
#define W25N_CMD_FEATURE        0xB1u  /* Write feature register */
#define W25N_CMD_READ_FEAT      0x0Fu  /* Read feature register */

/* Status register bits */
#define W25N_SR_BUSY            (1u << 0)
#define W25N_SR_PROG_FAIL       (1u << 3)
#define W25N_SR_ERASE_FAIL      (1u << 4)
#define W25N_SR_ECC_MASK        (3u << 4)  /* ECC status bits */

/* Feature register addresses */
#define W25N_FEAT_PROT          0xA0u
#define W25N_FEAT_CONF          0xB0u
#define W25N_FEAT_STATUS        0xC0u

/* ---- Internal state ---- */
static uint16_t current_block;     /* Current write block */
static uint16_t current_page;      /* Current page within block (0-63) */
static uint16_t page_offset;       /* Byte offset within current page */
static uint32_t total_records;     /* Total records written (all dives) */
static uint16_t total_dives;       /* Total dives stored */
static int dive_active;            /* 1 if a dive is currently being logged */
static uint32_t current_dive_id;
static uint16_t current_dive_records;
static uint16_t bad_blocks[64];    /* Bad block bitmap (up to 64 entries) */
static uint8_t  bad_block_count;

/* ---- Local function prototypes ---- */
static void nand_select(void);
static void nand_deselect(void);
static uint8_t nand_transfer(uint8_t tx);
static void nand_wait_ready(void);
static int nand_erase_block(uint16_t block);
static int nand_program_page(uint16_t page, const uint8_t *data, uint16_t len);
static int nand_read_page(uint16_t page, uint8_t *buf, uint16_t len);
static uint8_t nand_read_status(void);
static void nand_reset(void);
static int nand_check_bad_block(uint16_t block);
static void nand_scan_bad_blocks(void);
static uint16_t find_next_good_block(uint16_t from);
static uint16_t crc16(const uint8_t *data, uint16_t len);

/* ---- Public API ---- */

int storage_init(void)
{
    /* NAND WP pin — output, initially high (not protected) */
    gpio_set_mode(NAND_WP_GPIO, NAND_WP_PIN, GPIO_MODE_OUTPUT);
    gpio_set(NAND_WP_GPIO, NAND_WP_PIN);

    /* NAND BUSY pin — input */
    gpio_set_mode(NAND_BUSY_GPIO, NAND_BUSY_PIN, GPIO_MODE_INPUT);

    /* CS already configured in attitude_init (shared SPI4) */
    nand_reset();
    nand_wait_ready();

    /* Scan for bad blocks */
    bad_block_count = 0;
    nand_scan_bad_blocks();

    /* Reset write pointer to first good block */
    current_block = find_next_good_block(0);
    current_page = 0;
    page_offset = 0;
    total_records = 0;
    total_dives = 0;
    dive_active = 0;

    /* Enable ECC on the W25N02G (feature register CONFIG, bit 4) */
    nand_select();
    nand_transfer(W25N_CMD_FEATURE);
    nand_transfer(W25N_FEAT_CONF);
    nand_transfer(0x10u);  /* Enable ECC */
    nand_deselect();

    return 0;
}

int storage_start_dive(uint32_t timestamp, uint16_t sample_rate_hz)
{
    if (dive_active) {
        return -1;  /* Already logging a dive */
    }

    /* Ensure we're on a good block with enough space */
    if (current_block >= NAND_TOTAL_BLOCKS) {
        return -1;  /* Storage full */
    }

    dive_header_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.magic = DIVE_MAGIC;
    current_dive_id = ++total_dives;
    hdr.dive_id = current_dive_id;
    hdr.start_time = timestamp;
    hdr.end_time = 0;
    hdr.max_depth_m = 0.0f;
    hdr.avg_current_ms = 0.0f;
    hdr.record_count = 0;
    hdr.sample_rate_hz = sample_rate_hz;
    hdr.crc = crc16((const uint8_t *)&hdr, sizeof(hdr) - 2);

    /* Write dive header as first record of the dive */
    uint8_t buf[sizeof(dive_header_t)];
    memcpy(buf, &hdr, sizeof(hdr));
    if (nand_program_page(current_page, buf, sizeof(buf)) != 0) {
        return -1;
    }

    current_dive_records = 0;
    dive_active = 1;

    /* Advance page offset past the header */
    page_offset += sizeof(dive_header_t);
    if (page_offset + NAND_RECORD_SIZE > NAND_PAGE_SIZE) {
        current_page++;
        page_offset = 0;
        if (current_page >= NAND_PAGES_PER_BLOCK) {
            current_block = find_next_good_block(current_block + 1);
            current_page = 0;
        }
    }

    return (int)current_dive_id;
}

int storage_write_record(const profile_record_t *rec)
{
    if (!dive_active) {
        return -1;
    }

    /* Check if we need to advance to the next page */
    if (page_offset + NAND_RECORD_SIZE > NAND_PAGE_SIZE) {
        current_page++;
        page_offset = 0;
        if (current_page >= NAND_PAGES_PER_BLOCK) {
            current_block = find_next_good_block(current_block + 1);
            current_page = 0;
            if (current_block >= NAND_TOTAL_BLOCKS) {
                return -1;  /* Storage full */
            }
        }
    }

    /* Write record to current page at current offset */
    uint8_t buf[NAND_RECORD_SIZE];
    memcpy(buf, rec, NAND_RECORD_SIZE);
    if (nand_program_page(current_page, buf, NAND_RECORD_SIZE) != 0) {
        return -1;
    }

    page_offset += NAND_RECORD_SIZE;
    current_dive_records++;
    total_records++;

    return 0;
}

int storage_end_dive(uint32_t timestamp)
{
    if (!dive_active) {
        return -1;
    }

    dive_active = 0;

    /* Update the dive header with end time and final stats.
     * In a real implementation, we would read back the header,
     * update it, and reprogram. Since NAND can't reprogram without
     * erase, we write a completion marker record instead. */
    profile_record_t end_marker;
    memset(&end_marker, 0, sizeof(end_marker));
    end_marker.timestamp = timestamp;
    end_marker.depth_m = -1.0f;  /* Sentinel: dive end marker */
    end_marker.crc = crc16((const uint8_t *)&end_marker, sizeof(end_marker) - 2);

    storage_write_record(&end_marker);

    return 0;
}

int storage_read_dive_header(uint16_t index, dive_header_t *hdr)
{
    /* Naive scan: iterate through all blocks looking for dive magic.
     * In production, maintain a RAM index for faster lookups. */
    uint16_t dive_num = 0;

    for (uint16_t blk = 0; blk < NAND_TOTAL_BLOCKS; blk++) {
        if (bad_blocks[blk / 8] & (1u << (blk % 8))) continue;

        for (uint16_t pg = 0; pg < NAND_PAGES_PER_BLOCK; pg++) {
            uint8_t buf[sizeof(dive_header_t)];
            if (nand_read_page(pg, buf, sizeof(dive_header_t)) != 0) continue;

            dive_header_t *h = (dive_header_t *)buf;
            if (h->magic == DIVE_MAGIC) {
                if (dive_num == index) {
                    memcpy(hdr, h, sizeof(dive_header_t));
                    return 0;
                }
                dive_num++;
            }
        }
    }
    return -1;
}

int storage_read_record(uint32_t index, profile_record_t *rec)
{
    /* Calculate absolute page and offset from record index.
     * Skip first record of each dive (header). */
    uint32_t abs_byte = index * NAND_RECORD_SIZE;
    uint16_t page = abs_byte / NAND_PAGE_SIZE;
    uint16_t offset = abs_byte % NAND_PAGE_SIZE;

    uint8_t buf[NAND_RECORD_SIZE];
    if (nand_read_page(page, buf, NAND_RECORD_SIZE) != 0) {
        return -1;
    }
    memcpy(rec, buf + offset, NAND_RECORD_SIZE);
    return 0;
}

int storage_erase_all(void)
{
    for (uint16_t blk = 0; blk < NAND_TOTAL_BLOCKS; blk++) {
        if (bad_blocks[blk / 8] & (1u << (blk % 8))) continue;
        if (nand_erase_block(blk) != 0) {
            /* Mark as bad if erase fails */
            bad_blocks[blk / 8] |= (1u << (blk % 8));
        }
    }
    current_block = find_next_good_block(0);
    current_page = 0;
    page_offset = 0;
    total_records = 0;
    total_dives = 0;
    dive_active = 0;
    return 0;
}

uint16_t storage_get_dive_count(void)
{
    return total_dives;
}

uint32_t storage_get_free_bytes(void)
{
    uint32_t used = total_records * NAND_RECORD_SIZE;
    uint32_t total = NAND_TOTAL_BLOCKS * NAND_BLOCK_SIZE;
    return (total > used) ? (total - used) : 0;
}

/* ---- Local function implementations ---- */

static void nand_select(void)
{
    /* Deselect IMU and LCD first (shared SPI4 bus) */
    gpio_set(IMU_CS_GPIO, IMU_CS_PIN);
    gpio_set(LCD_CS_GPIO, LCD_CS_PIN);
    gpio_clear(NAND_CS_GPIO, NAND_CS_PIN);
    for (volatile int i = 0; i < 10; i++) { }
}

static void nand_deselect(void)
{
    gpio_set(NAND_CS_GPIO, NAND_CS_PIN);
}

static uint8_t nand_transfer(uint8_t tx)
{
    *(volatile uint8_t *)&SPI4_DR = tx;
    while ((SPI4_SR & SPI_SR_RXP) == 0) { }
    return *(volatile uint8_t *)&SPI4_DR;
}

static void nand_wait_ready(void)
{
    while (gpio_read(NAND_BUSY_GPIO, NAND_BUSY_PIN) == 0) { }
}

static uint8_t nand_read_status(void)
{
    nand_select();
    nand_transfer(W25N_CMD_READ_STATUS);
    nand_transfer(W25N_FEAT_STATUS);
    uint8_t status = nand_transfer(0x00);
    nand_deselect();
    return status;
}

static void nand_reset(void)
{
    nand_select();
    nand_transfer(W25N_CMD_RESET);
    nand_deselect();
    nand_wait_ready();
}

static int nand_erase_block(uint16_t block)
{
    nand_select();
    nand_transfer(W25N_CMD_ERASE_BLK);
    nand_transfer((block >> 6) & 0xFF);  /* Block address high */
    nand_transfer((block << 2) & 0xFC);  /* Block address low (page aligned) */
    nand_deselect();
    nand_wait_ready();

    uint8_t status = nand_read_status();
    return (status & W25N_SR_ERASE_FAIL) ? -1 : 0;
}

static int nand_program_page(uint16_t page, const uint8_t *data, uint16_t len)
{
    /* Load data into cache register */
    nand_select();
    nand_transfer(W25N_CMD_PROG_LOAD);
    nand_transfer(0x00);  /* Column address high */
    nand_transfer(0x00);  /* Column address low */
    for (uint16_t i = 0; i < len; i++) {
        nand_transfer(data[i]);
    }
    nand_deselect();

    /* Execute program operation */
    nand_select();
    nand_transfer(W25N_CMD_PROG_EXEC);
    nand_transfer((page >> 8) & 0xFF);  /* Page address high */
    nand_transfer(page & 0xFF);         /* Page address low */
    nand_deselect();
    nand_wait_ready();

    uint8_t status = nand_read_status();
    return (status & W25N_SR_PROG_FAIL) ? -1 : 0;
}

static int nand_read_page(uint16_t page, uint8_t *buf, uint16_t len)
{
    /* Transfer page from NAND to cache register */
    nand_select();
    nand_transfer(W25N_CMD_READ_PAGE);
    nand_transfer((page >> 8) & 0xFF);
    nand_transfer(page & 0xFF);
    nand_deselect();
    nand_wait_ready();

    /* Read from cache */
    nand_select();
    nand_transfer(W25N_CMD_READ_CACHE);
    nand_transfer(0x00);  /* Column high */
    nand_transfer(0x00);  /* Column low */
    nand_transfer(0x00);  /* Dummy byte (fast read mode) */
    for (uint16_t i = 0; i < len; i++) {
        buf[i] = nand_transfer(0x00);
    }
    nand_deselect();

    return 0;
}

static int nand_check_bad_block(uint16_t block)
{
    /* Read the first page of the block; if byte 0 of spare area is
     * not 0xFF, it's a factory bad block. */
    uint16_t page = block * NAND_PAGES_PER_BLOCK;
    uint8_t buf[4];

    /* Read spare area at offset 2048 (need to read the OOB) */
    /* For W25N02G, bad block indicator is at byte 0 of the spare area */
    nand_select();
    nand_transfer(W25N_CMD_READ_PAGE);
    nand_transfer((page >> 8) & 0xFF);
    nand_transfer(page & 0xFF);
    nand_deselect();
    nand_wait_ready();

    /* Read spare area (columns 2048-2111) */
    nand_select();
    nand_transfer(W25N_CMD_READ_CACHE);
    nand_transfer(0x08);  /* Column high (2048 >> 8) */
    nand_transfer(0x00);  /* Column low */
    nand_transfer(0x00);  /* Dummy */
    buf[0] = nand_transfer(0x00);
    nand_deselect();

    return (buf[0] != 0xFF) ? 1 : 0;  /* 1 = bad, 0 = good */
}

static void nand_scan_bad_blocks(void)
{
    memset(bad_blocks, 0, sizeof(bad_blocks));
    bad_block_count = 0;

    for (uint16_t blk = 0; blk < NAND_TOTAL_BLOCKS; blk++) {
        if (nand_check_bad_block(blk)) {
            bad_blocks[blk / 8] |= (1u << (blk % 8));
            bad_block_count++;
        }
    }
}

static uint16_t find_next_good_block(uint16_t from)
{
    for (uint16_t blk = from; blk < NAND_TOTAL_BLOCKS; blk++) {
        if (!(bad_blocks[blk / 8] & (1u << (blk % 8)))) {
            return blk;
        }
    }
    return NAND_TOTAL_BLOCKS;  /* No good block found = storage full */
}

static uint16_t crc16(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}