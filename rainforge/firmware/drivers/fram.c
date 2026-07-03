/*
 * fram.c — CY15B104Q 4 Mbit FRAM driver (SPI)
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * The CY15B104Q is a 4 Mbit (512 KB) F-RAM with SPI interface, 10^14
 * endurance cycles and 150-year retention. It is used for:
 *
 *   0x00000 - 0x007FF:  Calibration table (2 KB)
 *   0x00800 - 0x00FFF:  Rolling statistics block (1 KB, double-buffered)
 *   0x01000 - 0x7EFFF:  Event ring buffer (511 KB, ~32K events)
 *   0x7F000 - 0x7FFFF:  Configuration block (4 KB)
 *
 * FRAM's infinite endurance makes it ideal for the event ring buffer,
 * which sees a write on every raindrop — potentially millions per
 * storm season. EEPROM or flash would wear out in months.
 */
#include <stdint.h>
#include <string.h>
#include "../board.h"
#include "../registers.h"
#include "fram.h"

/* ---- Weak SPI stubs ---- */
__attribute__((weak)) void spi2_init(uint32_t hz);
__attribute__((weak)) void spi2_cs(int cs_pin, int low);
__attribute__((weak)) void spi2_xfer(const uint8_t *tx, uint8_t *rx, uint16_t len);
__attribute__((weak)) void delay_ms(uint32_t ms);

/* ---- Event ring buffer pointers (stored at end of event region) ---- */
#define FRAM_EVENT_PTR_ADDR  (FRAM_ADDR_EVENTLOG + EVENT_RING_CAP * EVENT_SIZE)
#define FRAM_EVENT_MAGIC     0x52465631U   /* "RFV1" */

typedef struct {
    uint32_t magic;
    uint32_t write_idx;    /* next write position (wraps) */
    uint32_t total_count;  /* total events ever logged (monotonic) */
} event_ptr_t;

static event_ptr_t g_event_ptr;

/* ================================================================
 * Low-level SPI helpers
 * ================================================================ */
static void fram_write_enable(void)
{
    uint8_t cmd = FRAM_CMD_WREN;
    spi2_cs(PIN_SPI2_CS_FRAM, 0);
    spi2_xfer(&cmd, NULL, 1);
    spi2_cs(PIN_SPI2_CS_FRAM, 1);
}

static void fram_wait_wel(void)
{
    /* FRAM writes are instant (~no busy state), but we check WEL anyway */
    uint8_t cmd = FRAM_CMD_RDSR;
    uint8_t sr = 0;
    for (int i = 0; i < 4; ++i) {
        spi2_cs(PIN_SPI2_CS_FRAM, 0);
        spi2_xfer(&cmd, &sr, 1);
        spi2_xfer(NULL, &sr, 1);
        spi2_cs(PIN_SPI2_CS_FRAM, 1);
        if (sr & FRAM_SR_WEL) break;
    }
}

/* ================================================================
 * Initialize
 * ================================================================ */
void fram_init(void)
{
    /* SPI2 is shared with the ADC; it should already be initialized
     * by piezo_init(), but we ensure it here too. */
    spi2_init(SPI2_SPEED_HZ);

    /* Wake FRAM from sleep (if it was sleeping) */
    fram_wake();

    /* Read the event ring pointer */
    memset(&g_event_ptr, 0, sizeof(g_event_ptr));
    uint8_t buf[sizeof(event_ptr_t)];
    if (fram_read(FRAM_EVENT_PTR_ADDR, buf, sizeof(buf)) == 0) {
        memcpy(&g_event_ptr, buf, sizeof(buf));
        if (g_event_ptr.magic != FRAM_EVENT_MAGIC) {
            /* First boot or corruption — reset pointers */
            g_event_ptr.magic = FRAM_EVENT_MAGIC;
            g_event_ptr.write_idx = 0;
            g_event_ptr.total_count = 0;
            fram_save_config((uint8_t *)&g_event_ptr, sizeof(g_event_ptr));
        }
    }
}

/* ================================================================
 * Read / write raw bytes
 * ================================================================ */
int fram_read(uint32_t addr, uint8_t *buf, uint16_t len)
{
    if (addr + len > FRAM_SIZE) return -1;

    uint8_t cmd[4] = {
        FRAM_CMD_READ,
        (uint8_t)(addr >> 16),
        (uint8_t)(addr >> 8),
        (uint8_t)(addr & 0xFF)
    };
    spi2_cs(PIN_SPI2_CS_FRAM, 0);
    spi2_xfer(cmd, NULL, 4);
    spi2_xfer(NULL, buf, len);
    spi2_cs(PIN_SPI2_CS_FRAM, 1);
    return 0;
}

int fram_write(uint32_t addr, const uint8_t *buf, uint16_t len)
{
    if (addr + len > FRAM_SIZE) return -1;

    fram_write_enable();
    fram_wait_wel();

    uint8_t cmd[4] = {
        FRAM_CMD_WRITE,
        (uint8_t)(addr >> 16),
        (uint8_t)(addr >> 8),
        (uint8_t)(addr & 0xFF)
    };
    spi2_cs(PIN_SPI2_CS_FRAM, 0);
    spi2_xfer(cmd, NULL, 4);
    spi2_xfer(buf, NULL, len);
    spi2_cs(PIN_SPI2_CS_FRAM, 1);
    return 0;
}

/* ================================================================
 * Sleep / wake
 * ================================================================ */
void fram_sleep(void)
{
    uint8_t cmd = FRAM_CMD_SLEEP;
    spi2_cs(PIN_SPI2_CS_FRAM, 0);
    spi2_xfer(&cmd, NULL, 1);
    spi2_cs(PIN_SPI2_CS_FRAM, 1);
}

void fram_wake(void)
{
    uint8_t cmd = FRAM_CMD_WAKE;
    spi2_cs(PIN_SPI2_CS_FRAM, 0);
    spi2_xfer(&cmd, NULL, 1);
    spi2_cs(PIN_SPI2_CS_FRAM, 1);
    delay_ms(1);   /* wake-up time < 1 ms */
}

/* ================================================================
 * Read ID (9 bytes)
 * ================================================================ */
int fram_read_id(uint8_t *id_out)
{
    uint8_t cmd = FRAM_CMD_RDID;
    spi2_cs(PIN_SPI2_CS_FRAM, 0);
    spi2_xfer(&cmd, NULL, 1);
    spi2_xfer(NULL, id_out, FRAM_ID_LEN);
    spi2_cs(PIN_SPI2_CS_FRAM, 1);

    /* CY15B104Q ID: 7F7F7F7F 7F7F7F7F 7F2C2C (last 2 bytes = 0x2C2C) */
    return (id_out[7] == 0x2C && id_out[8] == 0x2C) ? 0 : -1;
}

/* ================================================================
 * Calibration storage
 * ================================================================ */
int fram_load_calibration(calibration_t *cal)
{
    uint8_t buf[sizeof(calibration_t)];
    if (fram_read(FRAM_ADDR_CALIB, buf, sizeof(buf)) != 0)
        return -1;
    memcpy(cal, buf, sizeof(buf));
    /* Validate magic */
    return (cal->magic == CALIB_MAGIC) ? 0 : -1;
}

int fram_save_calibration(const calibration_t *cal)
{
    return fram_write(FRAM_ADDR_CALIB, (const uint8_t *)cal, sizeof(*cal));
}

/* ================================================================
 * Statistics block (double-buffered: two copies at 0x800 and 0xC00)
 * ================================================================ */
#define STATS_BUF_A  FRAM_ADDR_STATS
#define STATS_BUF_B  (FRAM_ADDR_STATS + 512)
#define STATS_MAGIC  0x53544131U   /* "STA1" */

typedef struct {
    uint32_t magic;
    uint32_t seq;   /* incrementing sequence number (odd=A, even=B) */
    dsd_t    dsd;
} stats_block_t;

int fram_load_stats(dsd_t *dsd)
{
    stats_block_t a, b;
    fram_read(STATS_BUF_A, (uint8_t *)&a, sizeof(a));
    fram_read(STATS_BUF_B, (uint8_t *)&b, sizeof(b));

    /* Pick the block with the higher sequence number and valid magic */
    int use_b = 0;
    if (a.magic == STATS_MAGIC && b.magic == STATS_MAGIC) {
        use_b = (b.seq > a.seq);
    } else if (b.magic == STATS_MAGIC) {
        use_b = 1;
    } else if (a.magic != STATS_MAGIC) {
        return -1;   /* neither valid */
    }

    memcpy(dsd, use_b ? &b.dsd : &a.dsd, sizeof(dsd_t));
    return 0;
}

int fram_save_stats(const dsd_t *dsd)
{
    /* Read current sequence, increment, write to the other buffer */
    stats_block_t cur;
    uint32_t next_seq = 0;
    if (fram_read(STATS_BUF_A, (uint8_t *)&cur, sizeof(cur)) == 0
        && cur.magic == STATS_MAGIC) {
        next_seq = cur.seq + 1;
    }

    stats_block_t blk;
    blk.magic = STATS_MAGIC;
    blk.seq = next_seq;
    memcpy(&blk.dsd, dsd, sizeof(dsd_t));

    /* Write to alternate buffer: even seq → A, odd seq → B */
    uint32_t addr = (next_seq & 1) ? STATS_BUF_B : STATS_BUF_A;
    return fram_write(addr, (const uint8_t *)&blk, sizeof(blk));
}

/* ================================================================
 * Event ring buffer
 * ================================================================ */
int fram_log_event(const droplet_feature_t *feat)
{
    /* Pack a droplet_feature_t into 16 bytes for compact storage */
    struct __attribute__((packed)) packed_event {
        uint16_t diameter_q8;    /* mm × 256 → 0.3..7 mm in 0.0039 mm steps */
        uint16_t velocity_q8;    /* m/s × 256 → 0..10 m/s in 0.0039 m/s steps */
        int16_t  charge_q4;      /* pC × 16 → -2048..2047 pC in 0.0625 pC steps */
        uint16_t energy_q4;      /* µJ × 16 → 0..4095 µJ in 0.0625 µJ steps */
        uint32_t timestamp;      /* ms since boot (wraps every 49 days) */
        uint16_t flags;          /* reserved */
    } pe;

    pe.diameter_q8 = (uint16_t)(feat->diameter_mm * 256.0f);
    pe.velocity_q8 = (uint16_t)(feat->velocity_ms * 256.0f);
    pe.charge_q4   = (int16_t)(feat->charge_pc * 16.0f);
    pe.energy_q4   = (uint16_t)(feat->kinetic_energy_uj * 16.0f);
    pe.timestamp   = (uint32_t)feat->timestamp_ms;
    pe.flags       = 0;

    uint32_t addr = FRAM_ADDR_EVENTLOG + g_event_ptr.write_idx * EVENT_SIZE;
    if (addr + EVENT_SIZE > FRAM_EVENT_PTR_ADDR) {
        /* Wrap around */
        g_event_ptr.write_idx = 0;
        addr = FRAM_ADDR_EVENTLOG;
    }

    if (fram_write(addr, (const uint8_t *)&pe, sizeof(pe)) != 0)
        return -1;

    g_event_ptr.write_idx++;
    g_event_ptr.total_count++;

    /* Persist the pointer (only 12 bytes — cheap even in FRAM) */
    fram_write(FRAM_EVENT_PTR_ADDR, (const uint8_t *)&g_event_ptr,
               sizeof(g_event_ptr));
    return 0;
}

int fram_read_events(uint32_t start_idx, droplet_feature_t *out, uint16_t max)
{
    uint16_t read = 0;
    for (uint16_t i = 0; i < max; ++i) {
        uint32_t idx = (start_idx + i) % EVENT_RING_CAP;
        uint32_t addr = FRAM_ADDR_EVENTLOG + idx * EVENT_SIZE;

        struct __attribute__((packed)) packed_event {
            uint16_t diameter_q8;
            uint16_t velocity_q8;
            int16_t  charge_q4;
            uint16_t energy_q4;
            uint32_t timestamp;
            uint16_t flags;
        } pe;

        if (fram_read(addr, (uint8_t *)&pe, sizeof(pe)) != 0)
            break;

        /* Check for empty slot (diameter = 0 means unwritten) */
        if (pe.diameter_q8 == 0 && pe.timestamp == 0)
            break;

        out[i].diameter_mm = pe.diameter_q8 / 256.0f;
        out[i].velocity_ms = pe.velocity_q8 / 256.0f;
        out[i].charge_pc   = pe.charge_q4 / 16.0f;
        out[i].kinetic_energy_uj = pe.energy_q4 / 16.0f;
        out[i].timestamp_ms = pe.timestamp;
        read++;
    }
    return read;
}

uint32_t fram_event_count(void)
{
    return g_event_ptr.total_count;
}

void fram_clear_events(void)
{
    g_event_ptr.write_idx = 0;
    g_event_ptr.total_count = 0;
    fram_write(FRAM_EVENT_PTR_ADDR, (const uint8_t *)&g_event_ptr,
               sizeof(g_event_ptr));
}

/* ================================================================
 * Config block
 * ================================================================ */
int fram_load_config(uint8_t *buf, uint16_t len)
{
    if (len > 4096) return -1;
    return fram_read(FRAM_ADDR_CONFIG, buf, len);
}

int fram_save_config(const uint8_t *buf, uint16_t len)
{
    if (len > 4096) return -1;
    return fram_write(FRAM_ADDR_CONFIG, buf, len);
}