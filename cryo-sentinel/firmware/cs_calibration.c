/*
 * cs_calibration.c — 4-point capacitive level calibration for Cryo-Sentinel.
 *
 * Author: jayis1
 * License: MIT
 */
#include "cs_calibration.h"
#include "board.h"
#include "registers.h"
#include "cs_sensor.h"
#include <string.h>

static cs_calibration_t g_cal;
static bool g_cal_loaded = false;

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

/* FRAM accessors shared with cs_log.c; in the real build these are Zephyr
 * I2C transfers. Weak so the test harness can override them. */
__attribute__((weak)) int fram_read_cal(uint16_t off, uint8_t *buf, int n)
{
    (void)off; (void)buf; (void)n; return 0;
}
__attribute__((weak)) int fram_write_cal(uint16_t off, const uint8_t *buf, int n)
{
    (void)off; (void)buf; (void)n; return 0;
}

int cs_calibration_load(cs_calibration_t *out)
{
    uint8_t buf[sizeof(cs_calibration_t)];
    fram_read_cal(CS_FRAM_OFF_CAL_BASE, buf, sizeof(buf));
    memcpy(out, buf, sizeof(*out));
    uint16_t expect = crc16(buf, (int)sizeof(*out) - 2);
    if (out->magic != CS_FRAM_MAGIC || out->crc != expect) return -1;
    g_cal = *out;
    g_cal_loaded = true;
    return 0;
}

int cs_calibration_save(const cs_calibration_t *c)
{
    cs_calibration_t tmp = *c;
    tmp.magic = CS_FRAM_MAGIC;
    tmp.crc   = crc16((const uint8_t *)&tmp, (int)sizeof(tmp) - 2);
    fram_write_cal(CS_FRAM_OFF_CAL_BASE, (const uint8_t *)&tmp, sizeof(tmp));
    g_cal = tmp;
    g_cal_loaded = true;
    return 0;
}

int cs_calibration_capture(int point_idx, uint32_t now_min)
{
    if (point_idx < 0 || point_idx >= CS_CAL_POINTS) return -1;
    if (!g_cal_loaded) memset(&g_cal, 0, sizeof(g_cal));
    /* Take 16 raw samples and median them. */
    static uint32_t buf[CS_LEVEL_SAMPLES];
    for (int i = 0; i < CS_LEVEL_SAMPLES; i++) {
        /* In the real build: fdc2214_read_ch0() directly; here we go via
         * the public sensor sample to keep the abstraction. */
        cs_reading_t r;
        cs_sensor_sample(&r);
        buf[i] = r.level_raw;
    }
    g_cal.raw[point_idx] = cs_median_u32(buf, CS_LEVEL_SAMPLES);
    g_cal.pct[point_idx] = (uint8_t)(25 * point_idx);  /* 0, 25, 75, 100 */
    if (point_idx == 0) g_cal.installed_epoch_min = now_min;
    /* If this is the last point, finalize and save. */
    if (point_idx == CS_CAL_POINTS - 1) {
        g_cal.magic = CS_FRAM_MAGIC;
        g_cal.crc   = crc16((const uint8_t *)&g_cal, (int)sizeof(g_cal) - 2);
        fram_write_cal(CS_FRAM_OFF_CAL_BASE, (const uint8_t *)&g_cal, sizeof(g_cal));
        g_cal_loaded = true;
    }
    return 0;
}

float cs_calibration_to_pct(uint32_t raw)
{
    if (!g_cal_loaded) return -1.0f;
    /* Piecewise-linear interpolation across the four anchor points, which
     * are stored in ascending pct order (0, 25, 75, 100). Find the segment
     * that brackets `raw` and linearly interpolate. */
    for (int i = 0; i < CS_CAL_POINTS - 1; i++) {
        uint32_t r0 = g_cal.raw[i];
        uint32_t r1 = g_cal.raw[i + 1];
        float    p0 = (float)g_cal.pct[i];
        float    p1 = (float)g_cal.pct[i + 1];
        if (r0 == r1) continue;
        /* Handle either polarity of the raw-vs-level relationship. */
        if (r0 < r1) {
            if (raw >= r0 && raw <= r1) {
                float frac = (float)(raw - r0) / (float)(r1 - r0);
                return p0 + frac * (p1 - p0);
            }
        } else {
            if (raw <= r0 && raw >= r1) {
                float frac = (float)(r0 - raw) / (float)(r0 - r1);
                return p0 + frac * (p1 - p0);
            }
        }
    }
    /* Clamp to the nearest endpoint. */
    if (g_cal.raw[0] < g_cal.raw[CS_CAL_POINTS-1]) {
        if (raw < g_cal.raw[0]) return (float)g_cal.pct[0];
        return (float)g_cal.pct[CS_CAL_POINTS-1];
    } else {
        if (raw > g_cal.raw[0]) return (float)g_cal.pct[0];
        return (float)g_cal.pct[CS_CAL_POINTS-1];
    }
}

bool cs_calibration_present(void)
{
    if (!g_cal_loaded) {
        cs_calibration_t tmp;
        if (cs_calibration_load(&tmp) == 0) g_cal_loaded = true;
    }
    return g_cal_loaded;
}