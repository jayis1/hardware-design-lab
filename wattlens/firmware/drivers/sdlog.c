/*
 * sdlog.c — microSD FAT32 event and metric logging
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 *
 * Logs power quality metrics (1 Hz), NILM state, and events to a FAT32
 * microSD card.  Uses SPI mode (SD card on SPI3 with CS on PC8).
 *
 * Files:
 *   /WATTLENS/METRICS.CSV  — one row per second
 *   /WATTLENS/EVENTS.CSV   — one row per event
 *   /WATTLENS/CAPTURE.RAW  — raw waveform captures (binary, with timestamp header)
 */

#include "sdlog.h"
#include "registers.h"
#include "nilm.h"
#include <string.h>

#define SD_CS_HIGH() (GPIO_REG(GPIOC_BASE, GPIO_BSRR_OFF) = BIT(8))
#define SD_CS_LOW()  (GPIO_REG(GPIOC_BASE, GPIO_BSRR_OFF) = BIT(40))

#define SD_BLOCK_SIZE    512
#define SD_CMD_TIMEOUT   1000

/* Simplified SD SPI protocol — production uses a full FAT32 library */
static bool sd_initialized = false;
static uint8_t sd_buf[SD_BLOCK_SIZE];
static int metrics_file_offset = 0;
static int events_file_offset = 0;

/* ========================================================================
 * SPI3 transfer (shared with display — need mutex in production)
 * ======================================================================== */
static uint8_t spi3_xfer(uint8_t tx) {
    /* Wait for TX buffer */
    while (!(SPI_REG(SPI3_BASE, SPI_SR) & BIT(1))) { }
    *(volatile uint8_t *)(SPI3_BASE + SPI_TXDR) = tx;
    /* Wait for RX */
    while (!(SPI_REG(SPI3_BASE, SPI_SR) & BIT(0))) { }
    SPI_REG(SPI3_BASE, SPI_IFCR) = 0xFFFFFFFF;
    return *(volatile uint8_t *)(SPI3_BASE + SPI_RXDR);
}

/* ========================================================================
 * Send SD command (SPI mode)
 * ======================================================================== */
static int sd_send_cmd(uint8_t cmd, uint32_t arg, uint8_t crc) {
    SD_CS_LOW();
    spi3_xfer(0xFF);  /* dummy */
    spi3_xfer(cmd | 0x40);
    spi3_xfer((arg >> 24) & 0xFF);
    spi3_xfer((arg >> 16) & 0xFF);
    spi3_xfer((arg >> 8) & 0xFF);
    spi3_xfer(arg & 0xFF);
    spi3_xfer(crc | 0x01);

    /* Wait for response (R1) */
    for (int i = 0; i < SD_CMD_TIMEOUT; i++) {
        uint8_t r = spi3_xfer(0xFF);
        if (r != 0xFF) {
            SD_CS_HIGH();
            return r;
        }
    }
    SD_CS_HIGH();
    return -1;
}

/* ========================================================================
 * Initialize SD card (SPI mode, simplified)
 * ======================================================================== */
void sdlog_init(void) {
    sd_initialized = false;

    /* Send 80 dummy clocks to enter SPI mode */
    SD_CS_HIGH();
    for (int i = 0; i < 10; i++) spi3_xfer(0xFF);

    /* CMD0: GO_IDLE_STATE */
    int r = sd_send_cmd(0, 0, 0x94);
    if (r != 0x01) return;  /* not idle */

    /* CMD8: SEND_IF_COND (check SD v2) */
    r = sd_send_cmd(8, 0x000001AA, 0x87);
    /* CMD55 + ACMD41: SD_SEND_OP_COND (initialize) */
    sd_send_cmd(55, 0, 0);
    r = sd_send_cmd(41, 0, 0);
    if (r != 0x00) return;

    /* CMD16: SET_BLOCKLEN to 512 */
    sd_send_cmd(16, SD_BLOCK_SIZE, 0);

    /* In production: mount FAT32 filesystem, open/create files */
    sd_initialized = true;
    metrics_file_offset = 0;
    events_file_offset = 0;
}

bool sdlog_is_present(void) { return sd_initialized; }

/* ========================================================================
 * Write CSV row to METRICS.CSV
 *
 * Format: timestamp,P_total,Q_total,S_total,PF,freq,V1,V2,V3,I1,I2,I3,
 *         THD_V_avg,THD_I_avg,nilm_mask,nilm_power_sum
 * ======================================================================== */
int sdlog_write_metrics(const power_metrics_t *m, const nilm_result_t *n, uint32_t uptime) {
    if (!sd_initialized) return -1;

    /* Format CSV line (simplified — would use a real FAT32 write) */
    char line[128];
    int idx = 0;

    /* Timestamp */
    idx += int_to_dec(uptime, &line[idx]);
    line[idx++] = ',';

    /* Power values (0.1 W resolution) */
    idx += int_to_dec((int)(m->p_total_real * 10), &line[idx]);
    line[idx++] = ',';
    idx += int_to_dec((int)(m->p_total_reactive * 10), &line[idx]);
    line[idx++] = ',';
    idx += int_to_dec((int)(m->p_total_apparent * 10), &line[idx]);
    line[idx++] = ',';

    /* PF (0.001) */
    idx += int_to_dec((int)(m->pf_total * 1000), &line[idx]);
    line[idx++] = ',';

    /* Frequency (0.01 Hz) */
    idx += int_to_dec((int)(m->freq * 100), &line[idx]);
    line[idx++] = ',';

    /* Voltages and currents */
    for (int i = 0; i < 3; i++) {
        idx += int_to_dec((int)(m->v_rms[i] * 10), &line[idx]);
        line[idx++] = ',';
    }
    for (int i = 0; i < 3; i++) {
        idx += int_to_dec((int)(m->i_rms[i] * 100), &line[idx]);
        line[idx++] = ',';
    }

    /* THD */
    idx += int_to_dec((int)((m->thd_v[0] + m->thd_v[1] + m->thd_v[2]) / 3.0f * 10), &line[idx]);
    line[idx++] = ',';
    idx += int_to_dec((int)((m->thd_i[0] + m->thd_i[1] + m->thd_i[2]) / 3.0f * 10), &line[idx]);
    line[idx++] = ',';

    /* NILM mask */
    uint16_t mask = 0;
    float power_sum = 0.0f;
    for (int c = 0; c < NILM_MAX_CLASSES; c++) {
        if (n->nilm_state[c]) {
            mask |= (1 << c);
            power_sum += n->nilm_power[c];
        }
    }
    idx += int_to_dec(mask, &line[idx]);
    line[idx++] = ',';
    idx += int_to_dec((int)(power_sum * 10), &line[idx]);

    line[idx++] = '\r';
    line[idx++] = '\n';
    line[idx] = '\0';

    /* Write to SD (simplified — would write via FAT32 library) */
    metrics_file_offset += idx;

    return 0;
}

/* ========================================================================
 * Write event to EVENTS.CSV
 * ======================================================================== */
int sdlog_write_event(const pq_event_t *evt) {
    if (!sd_initialized) return -1;

    char line[64];
    int idx = 0;

    idx += int_to_dec(evt->timestamp, &line[idx]);
    line[idx++] = ',';
    line[idx++] = '0' + evt->type;
    line[idx++] = ',';
    line[idx++] = '0' + evt->phase;
    line[idx++] = ',';
    idx += int_to_dec((int)(evt->severity * 100), &line[idx]);
    line[idx++] = ',';
    idx += int_to_dec(evt->duration_ms, &line[idx]);
    line[idx++] = '\r';
    line[idx++] = '\n';
    line[idx] = '\0';

    events_file_offset += idx;
    return 0;
}

/* ========================================================================
 * Write raw waveform capture (binary)
 * Header: [timestamp(4)] [n_samples(2)] [n_channels(1)]
 * Data: int16 samples interleaved
 * ======================================================================== */
int sdlog_write_waveform(const float *v[3], const float *i[4], uint32_t timestamp) {
    if (!sd_initialized) return -1;

    /* Header (7 bytes) */
    memcpy(&sd_buf[0], &timestamp, 4);
    sd_buf[4] = (WINDOW_SAMPLES >> 8) & 0xFF;
    sd_buf[5] = WINDOW_SAMPLES & 0xFF;
    sd_buf[6] = 7;  /* 3V + 4I channels */

    /* In production: write header + all samples via FAT32 append */
    (void)v; (void)i;
    return 0;
}

/* ========================================================================
 * Flush write buffer to SD
 * ======================================================================== */
int sdlog_flush(void) {
    /* In production: flush FAT32 buffers */
    return 0;
}

/* Helper: integer to decimal string (returns number of chars written) */
static int int_to_dec(int32_t val, char *out) {
    if (val == 0) { out[0] = '0'; return 1; }
    int neg = 0;
    if (val < 0) { out[0] = '-'; val = -val; neg = 1; }
    char tmp[12];
    int i = 0;
    while (val > 0) { tmp[i++] = '0' + (val % 10); val /= 10; }
    int idx = neg;
    while (i > 0) out[idx++] = tmp[--i];
    return idx;
}