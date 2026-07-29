/*
 * gps.c — u-blox NEO-M9N GNSS driver (I2C)
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * The NEO-M9N communicates over I2C at address 0x42.
 * UBX protocol is used (binary, no NMEA overhead).
 * We read the NAV-PVT message for position, time, and fix status.
 */

#include "gps.h"
#include "board.h"
#include "registers.h"
#include <string.h>

static gps_data_t g_last_gps;
static bool g_gps_enabled = false;

/* ---- I2C low-level (stub: uses STM32L4 LL_I2C) ---- */
static void i2c1_init(void)
{
    /* I2C1: PB6(SCL), PB7(SDA), 400 kHz, 7-bit addressing
     * In real build: configure GPIO AF4, I2C TIMINGR, enable
     */
}

static bool i2c1_write(uint8_t addr, const uint8_t *data, uint16_t len)
{
    /* START → addr+W → data... → STOP
     * Check NAK, errors
     */
    (void)addr; (void)data; (void)len;
    return true; /* stub */
}

static bool i2c1_read(uint8_t addr, uint8_t *data, uint16_t len)
{
    /* START → addr+R → data... → NAK+STOP
     */
    (void)addr; (void)data; (void)len;
    return true; /* stub */
}

/* ---- UBX protocol helpers ---- */
static uint16_t ubx_checksum(const uint8_t *buf, uint16_t len)
{
    /* UBX checksum: CK_A, CK_B over class+id+length+payload */
    uint8_t ck_a = 0, ck_b = 0;
    for (uint16_t i = 0; i < len; i++) {
        ck_a += buf[i];
        ck_b += ck_a;
    }
    return (uint16_t)ck_a | ((uint16_t)ck_b << 8);
}

static bool gps_send_ubx(uint8_t class, uint8_t id,
                         const uint8_t *payload, uint16_t len)
{
    uint8_t buf[64];
    uint16_t idx = 0;

    buf[idx++] = GPS_UBX_SYNC1;
    buf[idx++] = GPS_UBX_SYNC2;
    buf[idx++] = class;
    buf[idx++] = id;
    buf[idx++] = (len >> 0) & 0xFF;
    buf[idx++] = (len >> 8) & 0xFF;

    if (len > 0 && payload) {
        memcpy(&buf[idx], payload, len);
        idx += len;
    }

    /* Compute checksum over class + id + length + payload */
    uint16_t cksum = ubx_checksum(&buf[2], idx - 2);
    buf[idx++] = (cksum >> 0) & 0xFF;
    buf[idx++] = (cksum >> 8) & 0xFF;

    /* Write to GPS I2C register 0xFF (data stream) */
    return i2c1_write(GPS_ADDR, buf, idx);
}

static bool gps_read_ubx(uint8_t class, uint8_t id,
                         uint8_t *payload, uint16_t *len)
{
    /* Poll NEO-M9N TX buffer availability via register 0xFD (2 bytes) */
    uint8_t avail_buf[2];
    if (!i2c1_write(GPS_ADDR, (const uint8_t[]){GPS_REG_TXBUF}, 1)) return false;
    if (!i2c1_read(GPS_ADDR, avail_buf, 2)) return false;
    uint16_t avail = avail_buf[0] | (avail_buf[1] << 8);
    if (avail == 0) return false;

    /* Read available bytes from register 0xFF */
    uint8_t buf[128];
    uint16_t to_read = avail > sizeof(buf) ? sizeof(buf) : avail;
    if (!i2c1_write(GPS_ADDR, (const uint8_t[]){GPS_REG_DATA}, 1)) return false;
    if (!i2c1_read(GPS_ADDR, buf, to_read)) return false;

    /* Search for desired UBX message in the stream */
    for (uint16_t i = 0; i + 8 <= to_read; i++) {
        if (buf[i] != GPS_UBX_SYNC1 || buf[i+1] != GPS_UBX_SYNC2) continue;

        uint8_t msg_class = buf[i+2];
        uint8_t msg_id    = buf[i+3];
        uint16_t msg_len  = buf[i+4] | (buf[i+5] << 8);

        if (msg_class == class && msg_id == id) {
            if (msg_len > *len) msg_len = *len;
            memcpy(payload, &buf[i+6], msg_len);
            *len = msg_len;
            return true;
        }
    }
    return false;
}

/* ---- Public API ---- */

bool gps_init(void)
{
    i2c1_init();
    memset(&g_last_gps, 0, sizeof(g_last_gps));

    /* Configure NEO-M9N: enable NAV-PVT at 10 Hz, disable NMEA */
    uint8_t cfg_msg[8] = {
        GPS_UBX_CLASS_NAV, GPS_UBX_ID_NAV_PVT,
        0x01, 0x00,  /* rate on I2C port */
        0x00, 0x00, 0x00, 0x00
    };
    gps_send_ubx(0x06, 0x01, cfg_msg, sizeof(cfg_msg)); /* CFG-MSG */

    /* Set update rate to 10 Hz */
    gps_set_update_rate(10);

    return true;
}

bool gps_enable(void)
{
    /* Power on GPS (if VDD gated) and send CFG-RXM to active mode */
    g_gps_enabled = true;
    return true;
}

bool gps_disable(void)
{
    /* Set GPS to power-save / backup mode */
    uint8_t cfg_rxm[2] = { 0x08, 0x02 }; /* power save mode */
    gps_send_ubx(0x06, 0x11, cfg_rxm, sizeof(cfg_rxm));
    g_gps_enabled = false;
    return true;
}

bool gps_read(gps_data_t *data)
{
    if (!g_gps_enabled || !data) return false;

    uint8_t pvt[GPS_PVT_LEN];
    uint16_t len = sizeof(pvt);

    if (!gps_read_ubx(GPS_UBX_CLASS_NAV, GPS_UBX_ID_NAV_PVT, pvt, &len)) {
        /* Use cached data if no new fix */
        memcpy(data, &g_last_gps, sizeof(gps_data_t));
        return g_last_gps.fix_type > GPS_FIX_NONE;
    }

    /* Parse NAV-PVT fields */
    data->itow    = pvt[GPS_PVT_ITOW_OFFSET] |
                    (pvt[GPS_PVT_ITOW_OFFSET+1] << 8) |
                    (pvt[GPS_PVT_ITOW_OFFSET+2] << 16) |
                    (pvt[GPS_PVT_ITOW_OFFSET+3] << 24);
    data->year    = pvt[GPS_PVT_YEAR_OFFSET] | (pvt[GPS_PVT_YEAR_OFFSET+1] << 8);
    data->month   = pvt[GPS_PVT_MONTH_OFFSET];
    data->day     = pvt[GPS_PVT_DAY_OFFSET];
    data->hour    = pvt[GPS_PVT_HOUR_OFFSET];
    data->min     = pvt[GPS_PVT_MIN_OFFSET];
    data->sec     = pvt[GPS_PVT_SEC_OFFSET];
    data->fix_type = pvt[GPS_PVT_FIX_TYPE_OFFSET];
    data->sats    = pvt[GPS_PVT_SATS_OFFSET];

    /* Latitude/longitude: int32 × 1e-7 degrees */
    memcpy(&data->lon_e7, &pvt[GPS_PVT_LON_OFFSET], 4);
    memcpy(&data->lat_e7, &pvt[GPS_PVT_LAT_OFFSET], 4);

    /* Horizontal accuracy (mm) */
    memcpy(&data->hacc_mm, &pvt[GPS_PVT_HACC_OFFSET], 4);

    /* Cache */
    memcpy(&g_last_gps, data, sizeof(gps_data_t));
    return true;
}

bool gps_has_fix(void)
{
    return g_last_gps.fix_type >= GPS_FIX_2D;
}

bool gps_set_update_rate(uint8_t hz)
{
    if (hz == 0 || hz > 10) return false;
    uint16_t period_ms = 1000 / hz;
    uint8_t cfg_rate[6] = {
        (period_ms >> 0) & 0xFF, (period_ms >> 8) & 0xFF, /* measRate */
        0x01, 0x00, /* navRate = 1 */
        0x00, 0x00  /* timeRef = GPS */
    };
    return gps_send_ubx(0x06, 0x08, cfg_rate, sizeof(cfg_rate));
}

uint8_t gps_get_sats(void)
{
    return g_last_gps.sats;
}