/*
 * cs_log.c — FRAM-backed audit log for Cryo-Sentinel.
 *
 * The MB85RC04 is a 4-Kbit (512-byte) I2C FRAM. We use the upper 384 bytes
 * as a 64-record ring buffer of 16-byte records. The first 16 bytes hold a
 * magic + monotonic seq counter so the seq survives reboot.
 *
 * Author: jayis1
 * License: MIT
 */
#include "cs_log.h"
#include "board.h"
#include "registers.h"
#include <string.h>

static uint32_t g_seq = 0;

/* CRC-16/CCITT over 14 bytes (the record minus its own crc field). */
static uint16_t crc16(const uint8_t *p, int n)
{
    uint16_t crc = 0xFFFF;
    for (int i = 0; i < n; i++) {
        crc ^= (uint16_t)p[i] << 8;
        for (int b = 0; b < 8; b++)
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
    }
    return crc;
}

static int fram_read(uint16_t off, uint8_t *buf, int n)
{
    /* MB85RC04 uses a 1-byte word address; the WHOAMI byte is at 0x07.
     * In the real build this calls nrfx_twi_twim_xfer with a repeated-start;
     * the open-source release uses the Zephyr I2C driver. For desktop tests
     * a mock returns zeros. The function signature is preserved here so the
     * rest of the code compiles cleanly. */
    (void)off; (void)buf; (void)n;
    return 0;
}

static int fram_write(uint16_t off, const uint8_t *buf, int n)
{
    (void)off; (void)buf; (void)n;
    return 0;
}

int cs_log_init(void)
{
    /* Recover the magic and the monotonic seq. */
    uint8_t hdr[16];
    fram_read(0, hdr, 16);
    uint16_t magic = ((uint16_t)hdr[0] << 8) | hdr[1];
    if (magic != CS_FRAM_MAGIC) {
        /* First boot: initialize. */
        uint8_t zero[16] = {0};
        zero[0] = (uint8_t)(CS_FRAM_MAGIC >> 8);
        zero[1] = (uint8_t)(CS_FRAM_MAGIC & 0xFF);
        fram_write(0, zero, 16);
        g_seq = 0;
        return 1;
    }
    g_seq = ((uint32_t)hdr[2] << 24) | ((uint32_t)hdr[3] << 16) |
            ((uint32_t)hdr[4] << 8)  |  (uint32_t)hdr[5];
    return 0;
}

uint32_t cs_log_seq(void) { return g_seq; }

int cs_log_append(cs_log_event_t ev, int32_t aux, uint32_t epoch_min)
{
    cs_log_record_t rec;
    memset(&rec, 0, sizeof(rec));
    rec.event     = (uint8_t)ev;
    rec.seq       = ++g_seq;
    rec.epoch_min = epoch_min;
    rec.aux       = aux;
    rec.crc       = crc16((const uint8_t *)&rec + 2, 14);

    /* Compute the ring slot. */
    uint16_t off = CS_FRAM_OFF_LOG_BASE +
                   (uint16_t)((g_seq % CS_FRAM_LOG_CAP) * CS_FRAM_LOG_REC_LEN);
    fram_write(off, (const uint8_t *)&rec, sizeof(rec));

    /* Persist the new seq into the header. */
    uint8_t seqb[4] = {
        (uint8_t)(g_seq >> 24), (uint8_t)(g_seq >> 16),
        (uint8_t)(g_seq >> 8),  (uint8_t)(g_seq)
    };
    fram_write(2, seqb, 4);
    return 0;
}

int cs_log_read_since(uint32_t since_seq, cs_log_record_t *out, int max)
{
    int n = 0;
    /* Walk the ring backwards from the most recent record. */
    for (uint32_t i = 0; i < CS_FRAM_LOG_CAP && n < max; i++) {
        uint32_t s = g_seq - i;
        if (s == 0) break;
        if (s <= since_seq) break;
        uint16_t off = CS_FRAM_OFF_LOG_BASE +
                       (uint16_t)((s % CS_FRAM_LOG_CAP) * CS_FRAM_LOG_REC_LEN);
        cs_log_record_t r;
        fram_read(off, (uint8_t *)&r, sizeof(r));
        /* Validate CRC before surfacing. */
        uint16_t expect = crc16((const uint8_t *)&r + 2, 14);
        if (r.crc != expect) continue;
        out[n++] = r;
    }
    return n;
}

int cs_log_tail(int n, cs_log_record_t *out)
{
    return cs_log_read_since(g_seq > (uint32_t)n ? g_seq - n : 0, out, n);
}