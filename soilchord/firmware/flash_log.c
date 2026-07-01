/*
 * flash_log.c — Soilchord NOR-flash wear-levelled log ring buffer
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Stores measurement_t records in a W25Q64 SPI NOR flash as a circular
 * ring. The sector erase size is 4 KB; we pack records contiguously within
 * a sector and erase the next sector ahead of the write pointer (levelled
 * wear). A double-buffered header (mirrored in the last two sectors) holds
 * the write/read cursors and a magic so we can recover after power loss.
 */
#include <string.h>
#include "soilchord.h"
#include "registers.h"
#include "board.h"

/* W25Q64: 8 MB, 64 sectors × 64 pages × 256 bytes (page=256B, sector=4KB). */
#define FLASH_PAGE_BYTES        256
#define FLASH_SECTOR_BYTES      4096
#define FLASH_TOTAL_BYTES       (8UL * 1024UL * 1024UL)
#define FLASH_LOG_START         0x00000000UL
#define FLASH_LOG_END           (FLASH_TOTAL_BYTES - 2UL * FLASH_SECTOR_BYTES)
#define FLASH_HEADER_SECTOR0    (FLASH_TOTAL_BYTES - 2UL * FLASH_SECTOR_BYTES)
#define FLASH_HEADER_SECTOR1    (FLASH_TOTAL_BYTES - 1UL * FLASH_SECTOR_BYTES)

/* Record: we store a compacted form of measurement_t to keep flash usage low.
 * The on-flash layout is: [seq(4)][time(4)][batt(2)][solar(2)][flags(1)]
 *   [6 × chord(f0_q8(2), q_freq_u8, q_time_u8, temp_u8, flags)] = 4+4+2+2+1 + 6*5 = 41 bytes.
 * We pad to 48 bytes (a multiple of 8) for alignment. */
#define RECORD_BYTES            48
#define RECORDS_PER_SECTOR      (FLASH_SECTOR_BYTES / RECORD_BYTES)
#define MAX_RECORDABLE          (FLASH_LOG_END / RECORD_BYTES)
#define HEADER_MAGIC            0x5C4F494CU   /* 'SOIL' */

typedef struct {
    uint32_t magic;
    uint32_t write_offset;          /* byte offset into the log area */
    uint32_t count;                 /* total records ever written */
    uint16_t erased_next_sector;    /* flag: is the next sector pre-erased? */
    uint16_t crc;
} log_header_t;

static log_header_t s_hdr;
static bool s_hdr_loaded = false;

/* ---- Bit-banged SPI for W25Q64 (see board.h pin map) ---- */
static volatile uint32_t *port_base(char p)
{
    if (p == 'A') return (volatile uint32_t *)GPIOA_BASE_REG;
    if (p == 'B') return (volatile uint32_t *)GPIOB_BASE_REG;
    return (volatile uint32_t *)GPIOC_BASE_REG;
}
static void gpio_write(char port, int pin, int val)
{
    volatile uint32_t *b = port_base(port);
    if (val) GPIO_BSRR(b) = (1U << pin);
    else     GPIO_BSRR(b) = (1U << (pin + 16));
}
static int gpio_read(char port, int pin)
{
    volatile uint32_t *b = port_base(port);
    return (GPIO_IDR(b) >> pin) & 1;
}
static void f_cs(int v){ gpio_write('A', 12, v); }
static void f_sck(int v){ gpio_write('A', 13, v); }
static void f_mosi(int v){ gpio_write('A', 14, v); }
static int  f_miso(void){ return gpio_read('A', 5); }

static uint8_t f_xfer(uint8_t tx)
{
    uint8_t rx = 0;
    for (int i = 7; i >= 0; i--) {
        f_mosi((tx >> i) & 1);
        f_sck(1);
        if (f_miso()) rx |= (1U << i);
        f_sck(0);
    }
    return rx;
}

/* W25Q64 commands */
#define W25_CMD_READ            0x03
#define W25_CMD_WRITE_ENABLE    0x06
#define W25_CMD_PAGE_PROGRAM    0x02
#define W25_CMD_SECTOR_ERASE    0x20
#define W25_CMD_READ_STATUS     0x05
#define W25_CMD_POWER_DOWN      0xB9
#define W25_CMD_WAKE            0xAB

static void w25_wait_busy(void)
{
    f_cs(0);
    f_xfer(W25_CMD_READ_STATUS);
    uint8_t s;
    do { s = f_xfer(0xFF); } while (s & 0x01);
    f_cs(1);
}

static void w25_read(uint32_t addr, uint8_t *buf, size_t n)
{
    f_cs(0);
    f_xfer(W25_CMD_READ);
    f_xfer((addr >> 16) & 0xFF);
    f_xfer((addr >> 8) & 0xFF);
    f_xfer(addr & 0xFF);
    for (size_t i = 0; i < n; i++) buf[i] = f_xfer(0xFF);
    f_cs(1);
}

static void w25_write_enable(void) { f_cs(0); f_xfer(W25_CMD_WRITE_ENABLE); f_cs(1); }

static void w25_program_page(uint32_t addr, const uint8_t *buf, size_t n)
{
    if (n > FLASH_PAGE_BYTES) n = FLASH_PAGE_BYTES;
    w25_write_enable();
    f_cs(0);
    f_xfer(W25_CMD_PAGE_PROGRAM);
    f_xfer((addr >> 16) & 0xFF);
    f_xfer((addr >> 8) & 0xFF);
    f_xfer(addr & 0xFF);
    for (size_t i = 0; i < n; i++) f_xfer(buf[i]);
    f_cs(1);
    w25_wait_busy();
}

static void w25_erase_sector(uint32_t addr)
{
    w25_write_enable();
    f_cs(0);
    f_xfer(W25_CMD_SECTOR_ERASE);
    f_xfer((addr >> 16) & 0xFF);
    f_xfer((addr >> 8) & 0xFF);
    f_xfer(addr & 0xFF);
    f_cs(1);
    w25_wait_busy();
}

/* ---- Header load/store ---- */
static uint16_t crc16(const uint8_t *p, size_t n)
{
    uint16_t crc = 0;
    for (size_t i = 0; i < n; i++) {
        crc ^= (uint16_t)p[i] << 8;
        for (int j = 0; j < 8; j++) crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
    }
    return crc;
}

static void header_compute_crc(log_header_t *h)
{
    uint8_t *p = (uint8_t *)h;
    h->crc = 0;
    h->crc = crc16(p, sizeof(*h) - sizeof(uint16_t));
}

static void header_persist(void)
{
    header_compute_crc(&s_hdr);
    uint8_t buf[sizeof(log_header_t)];
    memcpy(buf, &s_hdr, sizeof(s_hdr));
    /* Write to both mirror sectors for robustness. */
    w25_erase_sector(FLASH_HEADER_SECTOR0);
    w25_program_page(FLASH_HEADER_SECTOR0, buf, sizeof(buf));
    w25_erase_sector(FLASH_HEADER_SECTOR1);
    w25_program_page(FLASH_HEADER_SECTOR1, buf, sizeof(buf));
}

static void header_load(void)
{
    log_header_t h0, h1;
    w25_read(FLASH_HEADER_SECTOR0, (uint8_t *)&h0, sizeof(h0));
    w25_read(FLASH_HEADER_SECTOR1, (uint8_t *)&h1, sizeof(h1));

    uint16_t c0 = h0.crc; h0.crc = 0; uint16_t r0 = crc16((uint8_t*)&h0, sizeof(h0)-2); h0.crc = c0;
    uint16_t c1 = h1.crc; h1.crc = 0; uint16_t r1 = crc16((uint8_t*)&h1, sizeof(h1)-2); h1.crc = c1;
    int ok0 = (h0.magic == HEADER_MAGIC && r0 == c0);
    int ok1 = (h1.magic == HEADER_MAGIC && r1 == c1);

    if (ok0 && ok1) {
        s_hdr = (h0.count >= h1.count) ? h0 : h1;
    } else if (ok0) s_hdr = h0;
    else if (ok1)   s_hdr = h1;
    else {
        /* Fresh flash: initialise. */
        memset(&s_hdr, 0, sizeof(s_hdr));
        s_hdr.magic = HEADER_MAGIC;
        s_hdr.write_offset = 0;
        s_hdr.count = 0;
        s_hdr.erased_next_sector = 1;
        header_persist();
    }
    s_hdr_loaded = true;
}

/* ---- Record serialisation ---- */
static void serialise(const measurement_t *m, uint8_t *buf)
{
    memset(buf, 0xFF, RECORD_BYTES);
    uint8_t *p = buf;
    memcpy(p, &m->seq, 4);          p += 4;
    memcpy(p, &m->unix_time, 4);    p += 4;
    memcpy(p, &m->battery_mv, 2);   p += 2;
    memcpy(p, &m->solar_mv, 2);     p += 2;
    *p++ = (uint8_t)((m->urgent << 7) | (m->reset_cause & 0x7F));
    for (int i = 0; i < NCHORDS; i++) {
        uint16_t f0q8 = (uint16_t)(m->chord[i].f0 * 256.0f);
        memcpy(p, &f0q8, 2); p += 2;
        *p++ = (uint8_t)(m->chord[i].q_freq * 4.0f);
        *p++ = (uint8_t)(m->chord[i].q_time * 4.0f);
        *p++ = (uint8_t)(m->chord[i].temp_c + 64.0f);
        *p++ = m->chord[i].flags & 0x07;
    }
}

static void deserialise(measurement_t *m, const uint8_t *buf)
{
    memset(m, 0, sizeof(*m));
    const uint8_t *p = buf;
    memcpy(&m->seq, p, 4);          p += 4;
    memcpy(&m->unix_time, p, 4);    p += 4;
    memcpy(&m->battery_mv, p, 2);   p += 2;
    memcpy(&m->solar_mv, p, 2);     p += 2;
    uint8_t f = *p++;
    m->urgent = (f >> 7) & 1;
    m->reset_cause = f & 0x7F;
    for (int i = 0; i < NCHORDS; i++) {
        uint16_t f0q8; memcpy(&f0q8, p, 2); p += 2;
        m->chord[i].f0 = (float)f0q8 / 256.0f;
        m->chord[i].q_freq = (float)(*p++) / 4.0f;
        m->chord[i].q_time = (float)(*p++) / 4.0f;
        m->chord[i].temp_c = (float)(*p++) - 64.0f;
        m->chord[i].flags = *p++ & 0x07;
    }
}

/* ---- Public API ---- */
sc_err_t flash_log_init(void)
{
    if (!s_hdr_loaded) header_load();
    /* Pre-erase the next sector if needed. */
    if (!s_hdr.erased_next_sector) {
        uint32_t next = s_hdr.write_offset + FLASH_SECTOR_BYTES;
        if (next >= FLASH_LOG_END) next = 0;
        w25_erase_sector(FLASH_LOG_START + next);
        s_hdr.erased_next_sector = 1;
        header_persist();
    }
    return SC_OK;
}

sc_err_t flash_log_append(const measurement_t *m)
{
    if (!s_hdr_loaded) header_load();

    uint8_t buf[RECORD_BYTES];
    serialise(m, buf);

    uint32_t addr = FLASH_LOG_START + s_hdr.write_offset;
    w25_program_page(addr, buf, RECORD_BYTES);

    s_hdr.write_offset += RECORD_BYTES;
    s_hdr.count++;

    /* Sector boundary? mark next as needing erase. */
    if ((s_hdr.write_offset % FLASH_SECTOR_BYTES) == 0) {
        s_hdr.erased_next_sector = 0;
        /* Wrap if we hit the end. */
        if (s_hdr.write_offset >= FLASH_LOG_END) s_hdr.write_offset = 0;
        /* Erase the next sector ahead so it is ready. */
        uint32_t next = s_hdr.write_offset;
        w25_erase_sector(FLASH_LOG_START + next);
        s_hdr.erased_next_sector = 1;
    }
    header_persist();
    return SC_OK;
}

sc_err_t flash_log_read(uint32_t idx, measurement_t *out)
{
    if (!s_hdr_loaded) header_load();
    if (idx >= s_hdr.count) return SC_ERR_RANGE;
    uint32_t off = (idx * RECORD_BYTES) % FLASH_LOG_END;
    uint8_t buf[RECORD_BYTES];
    w25_read(FLASH_LOG_START + off, buf, RECORD_BYTES);
    deserialise(out, buf);
    return SC_OK;
}

uint32_t flash_log_count(void)
{
    if (!s_hdr_loaded) header_load();
    return s_hdr.count;
}

sc_err_t flash_log_erase(void)
{
    for (uint32_t a = 0; a < FLASH_LOG_END; a += FLASH_SECTOR_BYTES) {
        w25_erase_sector(FLASH_LOG_START + a);
    }
    memset(&s_hdr, 0, sizeof(s_hdr));
    s_hdr.magic = HEADER_MAGIC;
    s_hdr.write_offset = 0;
    s_hdr.count = 0;
    s_hdr.erased_next_sector = 1;
    header_persist();
    return SC_OK;
}