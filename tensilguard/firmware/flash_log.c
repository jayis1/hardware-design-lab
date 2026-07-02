/*
 * flash_log.c — W25Q128 SPI flash logging, calibration page, AE waveform store
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * The W25Q128JVSIQ is a 16 MB SPI flash. We carve it into three logical
 * regions (see tensilguard.h for indices):
 *   - cal page: 4 KB at page 0, holding cal_page_t + CRC
 *   - measurement ring: pages 1..1023, ~4 MB, circular, each entry 32 bytes
 *     → ~130k entries / 9 years at 30-min cycle
 *   - AE events: pages 1024..end, variable-length records with waveform
 *
 * The driver uses SPI2 with DMA. Erase is on 4 KB sector boundaries; we erase
 * a sector lazily when the write pointer crosses into a fresh sector.
 */
#include <string.h>
#include "tensilguard.h"
#include "board.h"
#include "registers.h"
#include "hal.h"

/* W25Q128 commands */
#define W25_READ                0x03
#define W25_PAGE_PROGRAM        0x02
#define W25_SECTOR_ERASE        0x20
#define W25_WRITE_ENABLE        0x06
#define W25_READ_STATUS         0x05
#define W25_JEDEC_ID            0x9F

#define W25_PAGE_BYTES          256
#define W25_SECTOR_PAGES        16
#define LOG_ENTRY_BYTES         32
#define LOG_ENTRIES_PER_PAGE    (TG_FLASH_PAGE_BYTES / LOG_ENTRY_BYTES)

static uint32_t s_log_write_ptr = 0;   /* byte offset of next entry */
static uint32_t s_log_end       = TG_AE_PAGE_IDX_START * TG_FLASH_PAGE_BYTES;
static uint32_t s_ae_write_ptr  = 0;

/* ----------------------------- Prototypes --------------------------------- */
static void w25_wait_ready(void);
static void w25_write_enable(void);
static void w25_sector_erase(uint32_t addr);
static void w25_page_program(uint32_t addr, const uint8_t *data, uint16_t n);
static void w25_read(uint32_t addr, uint8_t *out, uint16_t n);
static uint16_t crc16_local(const uint8_t *data, size_t len);
static void spi2_xfer(const uint8_t *tx, uint8_t *rx, uint16_t n);
static void spi2_cs(bool low);

/* ----------------------------- Public API --------------------------------- */

bool flash_init(void)
{
    /* read JEDEC ID: 0xEF 0x40 0x18 (Winbond, W25Q128) */
    uint8_t id[3] = { 0, 0, 0 };
    spi2_cs(true);
    uint8_t cmd = W25_JEDEC_ID;
    spi2_xfer(&cmd, NULL, 1);
    spi2_xfer(NULL, id, 3);
    spi2_cs(false);
    if (id[0] != 0xEF || id[1] != 0x40 || id[2] != 0x18) return false;

    /* recover write pointers from scan (simple linear scan; in practice we
     * keep a pointer in the cal page to avoid a full flash scan) */
    s_log_write_ptr = sizeof(cal_page_t);
    s_ae_write_ptr  = 0;
    return true;
}

/*
 * flash_write_cal — store calibration page (page 0) with CRC.
 */
bool flash_write_cal(const cal_page_t *cal)
{
    cal_page_t tmp = *cal;
    tmp.crc16 = crc16_local((const uint8_t *)cal,
                           offsetof(cal_page_t, crc16));
    w25_sector_erase(0);
    w25_write_enable();
    w25_page_program(0, (const uint8_t *)&tmp, sizeof(cal_page_t));
    return true;
}

/*
 * flash_read_cal — read calibration page, verify CRC.
 */
bool flash_read_cal(cal_page_t *cal)
{
    w25_read(0, (uint8_t *)cal, sizeof(cal_page_t));
    uint16_t calc = crc16_local((const uint8_t *)cal,
                                offsetof(cal_page_t, crc16));
    return (calc == cal->crc16);
}

/*
 * flash_log_measurement — append a measurement_t as a compact 32-byte entry.
 */
void flash_log_measurement(const measurement_t *m, uint32_t seq, uint32_t t)
{
    if (s_log_write_ptr + LOG_ENTRY_BYTES > s_log_end) {
        /* wrap to start of log region */
        s_log_write_ptr = sizeof(cal_page_t);
    }
    /* erase sector boundary crossing */
    if ((s_log_write_ptr % (W25_SECTOR_PAGES * W25_PAGE_BYTES)) == 0) {
        w25_sector_erase(s_log_write_ptr);
    }

    /* compact entry: 32 bytes */
    uint8_t entry[LOG_ENTRY_BYTES];
    memset(entry, 0, sizeof(entry));
    uint32_t *p = (uint32_t *)entry;
    p[0] = seq;
    p[1] = t;
    /* pack floats as Q8.8 */
    int16_t *q = (int16_t *)(entry + 8);
    q[0] = (int16_t)(m->t_mag_kn * 16.0f);
    q[1] = (int16_t)(m->t_vib_kn * 16.0f);
    q[2] = (int16_t)(m->dt_kn * 16.0f);
    q[3] = (int16_t)(m->temp_c * 10.0f);
    q[4] = (int16_t)(m->f1_hz * 1000.0f);
    q[5] = (int16_t)(m->battery_mv);
    q[6] = (int16_t)(m->panel_mv);
    entry[22] = m->ae_count;
    entry[23] = m->flags;
    entry[24] = (uint8_t)(m->battery_pct);
    entry[25] = (uint8_t)(m->accel_rms_mg & 0xFF);
    entry[26] = (uint8_t)((m->accel_rms_mg >> 8) & 0xFF);
    /* 27..31 reserved */
    /* 32-bit CRC over the entry (bytes 28..31) */
    uint16_t c = crc16_local(entry, 28);
    entry[28] = c & 0xFF;
    entry[29] = (c >> 8) & 0xFF;

    w25_write_enable();
    w25_page_program(s_log_write_ptr, entry, LOG_ENTRY_BYTES);
    s_log_write_ptr += LOG_ENTRY_BYTES;
}

/*
 * flash_log_ae — append an AE event with its waveform summary to the AE
 * region.  Returns the byte offset (for retrieval) or 0 on failure.
 */
uint32_t flash_log_ae(const ae_event_t *ev)
{
    (void)ev;  /* forward decl used in ae.c; real impl below */
    /* In the AE region we store: header (sizeof ae_event_t) + waveform bytes.
     * For this build we keep it as a sequential append with sector erase on
     * boundary. The real implementation writes through the same w25 helpers. */
    uint32_t base = TG_AE_PAGE_IDX_START * TG_FLASH_PAGE_BYTES + s_ae_write_ptr;
    if ((base % (W25_SECTOR_PAGES * W25_PAGE_BYTES)) == 0) {
        w25_sector_erase(base);
    }
    w25_write_enable();
    w25_page_program(base, (const uint8_t *)ev, sizeof(ae_event_t));
    s_ae_write_ptr += sizeof(ae_event_t);
    /* waveform stored inside the ae_event_t.waveform[] already */
    return base;
}

/*
 * flash_read_log_range — read measurement entries [from, to) into a buffer.
 * Used by the gateway when backhauling history. Returns entries read.
 */
int flash_read_log_range(uint32_t from_seq, uint32_t max_entries,
                          uint8_t *out, uint32_t out_len)
{
    (void)from_seq; (void)max_entries; (void)out; (void)out_len;
    /* real firmware: walk s_log_write_ptr backwards to find matching seq,
     * read entries into out, return count.  Stub for build-test. */
    return 0;
}

/* ----------------------------- Internals ---------------------------------- */

static void w25_wait_ready(void)
{
    uint8_t st = 0x01;
    uint32_t timeout = 100000;
    while ((st & 0x01) && timeout--) {
        spi2_cs(true);
        uint8_t cmd = W25_READ_STATUS;
        spi2_xfer(&cmd, NULL, 1);
        spi2_xfer(NULL, &st, 1);
        spi2_cs(false);
    }
}

static void w25_write_enable(void)
{
    spi2_cs(true);
    uint8_t cmd = W25_WRITE_ENABLE;
    spi2_xfer(&cmd, NULL, 1);
    spi2_cs(false);
}

static void w25_sector_erase(uint32_t addr)
{
    w25_write_enable();
    spi2_cs(true);
    uint8_t cmd = W25_SECTOR_ERASE;
    spi2_xfer(&cmd, NULL, 1);
    uint8_t a[3] = { (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF };
    spi2_xfer(a, NULL, 3);
    spi2_cs(false);
    w25_wait_ready();
}

static void w25_page_program(uint32_t addr, const uint8_t *data, uint16_t n)
{
    spi2_cs(true);
    uint8_t cmd = W25_PAGE_PROGRAM;
    spi2_xfer(&cmd, NULL, 1);
    uint8_t a[3] = { (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF };
    spi2_xfer(a, NULL, 3);
    spi2_xfer(data, NULL, n);
    spi2_cs(false);
    w25_wait_ready();
}

static void w25_read(uint32_t addr, uint8_t *out, uint16_t n)
{
    spi2_cs(true);
    uint8_t cmd = W25_READ;
    spi2_xfer(&cmd, NULL, 1);
    uint8_t a[3] = { (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF };
    spi2_xfer(a, NULL, 3);
    spi2_xfer(NULL, out, n);
    spi2_cs(false);
}

static uint16_t crc16_local(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int b = 0; b < 8; b++) {
            if (crc & 0x8000) crc = (crc << 1) ^ 0x1021;
            else              crc <<= 1;
        }
    }
    return crc;
}

/* ---- SPI2 stubs ---- */
static void spi2_xfer(const uint8_t *tx, uint8_t *rx, uint16_t n)
{
    (void)tx; (void)rx; (void)n;
    /* real firmware: SPI2 full-duplex master, DMA */
}

static void spi2_cs(bool low)
{
    (void)low;
    /* real firmware: GPIO_BSRR on PIN_FLASH_NSS */
}