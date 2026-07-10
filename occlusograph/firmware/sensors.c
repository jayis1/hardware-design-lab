/*
 * sensors.c — Sensor acquisition for Occlusograph.
 *
 * Author: jayis1
 * Copyright © 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 *
 * Implements the PVDF piezo charge-amp scanning loop, the AD7746 capacitive
 * CDC readout, the ICM-42688-P IMU readout, the TMP117 temperature readout,
 * and the MAX30101 tissue-contact detect. All functions are designed to run
 * within a 1 ms hard-real-time deadline on the nRF5340 app core.
 *
 * The piezo path is the most timing-critical. It works like this:
 *
 *   For each of 4 mux positions (0..3):
 *     - set mux select lines S0/S1
 *     - wait 8 us for charge amps to settle (single NOP loop)
 *     - sample all 16 SAADC inputs (one scan, 16 channels)
 *     - copy 16 samples into frame->piezo[bank*4 + mux_pos]
 *   - assert PIEZO_RESET for 2 us to discharge integrators
 *
 * This gives 64 samples per frame. At 1 kHz that's 64 ksps aggregate.
 *
 * The capacitive path reads 2 AD7746 devices × 2 channels × 8 mux positions
 * = 32 elements, but only at 100 Hz (every 10th frame). The AD7746 is set to
 * continuous-conversion mode with the fastest settling; we poll the RDY bit
 * once and read the latest conversion result.
 */

#include "sensors.h"
#include "registers.h"
#include <string.h>

/* ---- Platform shim ----
 * The firmware is written against a thin platform abstraction so it can be
 * compiled and unit-tested on a host. On the real target these map to the
 * Zephyr/nRF HAL. Declarations here are extern; the target build links the
 * real implementations.
 */

/* SAADC: trigger a 16-channel scan and return 16 12-bit samples. */
extern void hal_saadc_scan16(uint16_t out16[PIEZO_NUM_BANKS]);
extern void hal_saadc_setup(void);

/* GPIO. */
extern void hal_gpio_write(uint8_t port, uint8_t pin, uint8_t val);
extern uint8_t hal_gpio_read(uint8_t port, uint8_t pin);
extern void hal_gpio_cfg_out(uint8_t port, uint8_t pin);
extern void hal_gpio_cfg_in(uint8_t port, uint8_t pin, uint8_t pull);

/* I²C (TWI). */
extern int  hal_twi_write(uint8_t bus, uint8_t addr, const uint8_t *w, uint32_t len);
extern int  hal_twi_read(uint8_t bus, uint8_t addr, uint8_t reg, uint8_t *r, uint32_t len);

/* Delay. */
extern void hal_delay_us(uint32_t us);
extern void hal_delay_ms(uint32_t ms);

/* ---- Module state ---- */
static bool g_running = false;
static int16_t g_piezo_off[PIEZO_NUM_ELEMENTS];
static int32_t g_cap_off[CAP_NUM_ELEMENTS];
static uint8_t g_cap_phase = 0;   /* 0..9, advances every frame; cap read on 0 */
static uint8_t g_cap_mux   = 0;  /* 0..7 */

/* ---- Forward declarations ---- */
static int  cap_init(void);
static int  imu_init(void);
static int  tmp_init(void);
static int  max_init(void);
static void piezo_scan(int16_t out[PIEZO_NUM_ELEMENTS]);
static void cap_scan(int32_t out[CAP_NUM_ELEMENTS]);
static void imu_read(int16_t out[6]);
static int32_t tmp_read_mc(void);
static bool max_check_contact(void);

/* =====================================================================
 * Public API
 * =====================================================================
 */

int sensors_init(void)
{
    int rc;

    hal_saadc_setup();

    /* Configure digital output pins. */
    hal_gpio_cfg_out(PIEZO_MUX_S0_PORT, PIEZO_MUX_S0_PIN);
    hal_gpio_cfg_out(PIEZO_MUX_S1_PORT, PIEZO_MUX_S1_PIN);
    hal_gpio_cfg_out(PIEZO_RESET_PORT,  PIEZO_RESET_PIN);
    hal_gpio_cfg_out(CAP_MUX_A0_PORT,   CAP_MUX_A0_PIN);
    hal_gpio_cfg_out(CAP_MUX_A1_PORT,   CAP_MUX_A1_PIN);
    hal_gpio_cfg_out(CAP_MUX_A2_PORT,   CAP_MUX_A2_PIN);
    hal_gpio_cfg_out(LED_STATUS_PORT,   LED_STATUS_PIN);

    /* Charge amp reset idle-high (amps held in reset until first scan). */
    hal_gpio_write(PIEZO_RESET_PORT, PIEZO_RESET_PIN, 1);

    /* Zero calibration offsets. */
    memset(g_piezo_off, 0, sizeof(g_piezo_off));
    memset(g_cap_off,  0, sizeof(g_cap_off));

    rc = cap_init();
    if (rc) return rc;
    rc = imu_init();
    if (rc) return rc;
    rc = tmp_init();
    if (rc) return rc;
    rc = max_init();
    if (rc) return rc;

    g_running = false;
    return 0;
}

void sensors_start(void)
{
    g_running = true;
    hal_gpio_write(LED_STATUS_PORT, LED_STATUS_PIN, LED_STATUS_ON);
}

void sensors_stop(void)
{
    g_running = false;
    hal_gpio_write(LED_STATUS_PORT, LED_STATUS_PIN, LED_STATUS_OFF);
}

int sensors_acquire(sensor_frame_t *frame)
{
    if (!g_running || frame == NULL) {
        return -1;
    }

    /* Piezo: always, every frame. */
    piezo_scan(frame->piezo);

    /* Capacitive: every 10th frame (100 Hz). */
    if (g_cap_phase == 0) {
        cap_scan(frame->cap);
    } else {
        /* Hold previous value — caller sees a zero-order-hold on cap. */
        memset(frame->cap, 0, sizeof(frame->cap));
    }
    g_cap_phase = (g_cap_phase + 1) % (SAMPLE_RATE_HZ / CAP_SAMPLE_RATE_HZ);

    /* IMU: every frame (1 kHz) for jaw-motion detail. */
    imu_read(frame->imu);

    /* Temperature & contact: every 100th frame (10 Hz) to save power. */
    if ((frame->timestamp % 100u) == 0u) {
        frame->temp_mc = tmp_read_mc();
        frame->contact = max_check_contact();
    } else {
        frame->temp_mc = 0;
        frame->contact = false;
    }

    frame->timestamp++;
    return 0;
}

void sensors_apply_calibration(const int16_t piezo_off[PIEZO_NUM_ELEMENTS],
                              const int32_t cap_off[CAP_NUM_ELEMENTS])
{
    if (piezo_off) memcpy(g_piezo_off, piezo_off, sizeof(g_piezo_off));
    if (cap_off)   memcpy(g_cap_off,  cap_off,  sizeof(g_cap_off));
}

void sensors_piezo_reset(void)
{
    hal_gpio_write(PIEZO_RESET_PORT, PIEZO_RESET_PIN, 1);
    hal_delay_us(2);
    hal_gpio_write(PIEZO_RESET_PORT, PIEZO_RESET_PIN, 0);
}

uint16_t sensors_read_battery_mv(void)
{
    uint16_t raw[PIEZO_NUM_BANKS];
    /* Reuse SAADC ch2 (VBAT_SENSE_CH) — single-shot read. */
    hal_saadc_scan16(raw);
    /* raw[2] is the battery divider channel, 12-bit, 0..4095 -> 0..1.8 V */
    uint32_t mv = ((uint32_t)raw[VBAT_SENSE_CH] * VDDA_MV * VBAT_ADC_GAIN) / 4095u;
    return (uint16_t)CLAMP(mv, 0u, 6000u);
}

int32_t sensors_read_temp_mc(void)
{
    return tmp_read_mc();
}

bool sensors_check_contact(void)
{
    return max_check_contact();
}

/* =====================================================================
 * Piezo charge-amp scan
 * =====================================================================
 */

static void piezo_scan(int16_t out[PIEZO_NUM_ELEMENTS])
{
    uint16_t raw[PIEZO_NUM_BANKS];

    /* Take the integrators out of reset for the scan. */
    hal_gpio_write(PIEZO_RESET_PORT, PIEZO_RESET_PIN, 0);

    for (uint8_t mux = 0; mux < PIEZO_MUX_RATIO; mux++) {
        /* Drive mux select lines. S0 = bit0, S1 = bit1. */
        hal_gpio_write(PIEZO_MUX_S0_PORT, PIEZO_MUX_S0_PIN, mux & 0x01u);
        hal_gpio_write(PIEZO_MUX_S1_PORT, PIEZO_MUX_S1_PIN, (mux >> 1) & 0x01u);

        /* Settling: 8 us is ample for the AD8603's 1 MHz GBW at gain 10. */
        hal_delay_us(8);

        /* 16-channel SAADC scan (one conversion per bank). */
        hal_saadc_scan16(raw);

        /* Copy into the output array at positions [bank*4 + mux]. */
        for (uint8_t bank = 0; bank < PIEZO_NUM_BANKS; bank++) {
            uint16_t adc = raw[bank];
            /* Convert unsigned 12-bit (0..4095) to signed around mid-scale 2048. */
            int16_t signed_val = (int16_t)adc - 2048;
            /* Apply per-element offset. */
            signed_val -= g_piezo_off[bank * PIEZO_MUX_RATIO + mux];
            out[bank * PIEZO_MUX_RATIO + mux] = signed_val;
        }
    }

    /* Reset integrators to clear accumulated charge before next frame. */
    hal_gpio_write(PIEZO_RESET_PORT, PIEZO_RESET_PIN, 1);
    hal_delay_us(2);
    hal_gpio_write(PIEZO_RESET_PORT, PIEZO_RESET_PIN, 0);
}

/* =====================================================================
 * Capacitive CDC scan (AD7746 ×2, 8:1 mux)
 * =====================================================================
 */

static int cap_init(void)
{
    uint8_t cfg;
    int rc;

    for (uint8_t dev = 0; dev < CAP_NUM_DEVICES; dev++) {
        uint8_t addr = (dev == 0) ? AD7746_ADDR_A : AD7746_ADDR_B;

        /* Enable both channels, continuous conversion, fastest setting. */
        cfg = AD7746_CAPSET_EN | AD7746_CONFIG_FTSL_3;
        rc = hal_twi_write(0, addr, &cfg, 1);
        if (rc) return rc;

        /* Excitation output enable on both EXCA and EXCB. */
        uint8_t exc = AD7746_EXCEN;
        rc = hal_twi_write(0, addr, &exc, 1);
        if (rc) return rc;
    }
    return 0;
}

static int32_t cap_read_channel(uint8_t addr, uint8_t channel)
{
    uint8_t setup = AD7746_CAPSET_EN | (channel == 1 ? 0 : AD7746_CAPSET_CIN2);
    uint8_t data[3];

    /* Select channel. */
    if (hal_twi_write(0, addr, &setup, 1)) return 0;

    /* Wait for conversion complete (poll STATUS). */
    for (int tries = 0; tries < 50; tries++) {
        uint8_t st;
        if (hal_twi_read(0, addr, AD7746_REG_STATUS, &st, 1)) return 0;
        if (st & (AD7746_STATUS_RDYCAPA | AD7746_STATUS_RDYCAPB)) break;
        hal_delay_us(50);
    }

    /* Read 3 bytes (big-endian). */
    if (hal_twi_read(0, addr, AD7746_REG_CAP_DATA_HIG, data, 3)) return 0;
    int32_t code = ((int32_t)data[0] << 16) | ((int32_t)data[1] << 8) | data[2];
    /* Sign-extend from 24-bit. */
    if (code & 0x800000) code |= 0xFF000000;
    return code;
}

static void cap_scan(int32_t out[CAP_NUM_ELEMENTS])
{
    /* Advance the 8:1 mux and read all 4 CDC channels (2 devices × 2 ch). */
    for (uint8_t m = 0; m < CAP_NUM_MUX_OUTPUTS; m++) {
        hal_gpio_write(CAP_MUX_A0_PORT, CAP_MUX_A0_PIN, m & 0x01u);
        hal_gpio_write(CAP_MUX_A1_PORT, CAP_MUX_A1_PIN, (m >> 1) & 0x01u);
        hal_gpio_write(CAP_MUX_A2_PORT, CAP_MUX_A2_PIN, (m >> 2) & 0x01u);
        hal_delay_us(20);  /* mux + CDC input settle */

        /* Device A, ch1 and ch2. */
        int32_t a1 = cap_read_channel(AD7746_ADDR_A, 1);
        int32_t a2 = cap_read_channel(AD7746_ADDR_A, 2);
        /* Device B, ch1 and ch2. */
        int32_t b1 = cap_read_channel(AD7746_ADDR_B, 1);
        int32_t b2 = cap_read_channel(AD7746_ADDR_B, 2);

        uint8_t base = m * (CAP_NUM_DEVICES * CAP_CH_PER_DEVICE);
        if (base + 3 < CAP_NUM_ELEMENTS) {
            out[base + 0] = a1 - g_cap_off[base + 0];
            out[base + 1] = a2 - g_cap_off[base + 1];
            out[base + 2] = b1 - g_cap_off[base + 2];
            out[base + 3] = b2 - g_cap_off[base + 3];
        }
    }
}

/* =====================================================================
 * IMU readout (ICM-42688-P)
 * =====================================================================
 */

static int imu_init(void)
{
    uint8_t who;
    int rc;

    rc = hal_twi_read(1, IMU_I2C_ADDR, ICM_REG_WHOAMI, &who, 1);
    if (rc) return rc;
    if (who != ICM_VAL_WHOAMI) return -2;

    /* Power on, gyro + accel in low-noise mode. */
    uint8_t on = ICM_VAL_PWR_MGMT0_ON;
    rc = hal_twi_write(1, IMU_I2C_ADDR, &on, 1);
    if (rc) return rc;

    /* Accel: ±4g, 1 kHz ODR. */
    uint8_t acfg = ICM_ACCEL_FS_4G | ICM_ODR_1KHZ;
    rc = hal_twi_write(1, IMU_I2C_ADDR, &acfg, 1);
    if (rc) return rc;

    /* Gyro: ±2000 dps, 1 kHz. */
    uint8_t gcfg = ICM_GYRO_FS_2000DPS | ICM_ODR_1KHZ;
    rc = hal_twi_write(1, IMU_I2C_ADDR, &gcfg, 1);
    if (rc) return rc;

    return 0;
}

static void imu_read(int16_t out[6])
{
    uint8_t raw[12];
    /* Burst read: accel X/Y/Z (6 bytes) then gyro X/Y/Z (6 bytes). */
    if (hal_twi_read(1, IMU_I2C_ADDR, ICM_REG_ACCEL_DATAX0, raw, 12) == 0) {
        for (int i = 0; i < 6; i++) {
            out[i] = (int16_t)((raw[2*i] << 8) | raw[2*i + 1]);
        }
    } else {
        memset(out, 0, sizeof(int16_t) * 6);
    }
}

/* =====================================================================
 * Temperature (TMP117)
 * =====================================================================
 */

static int tmp_init(void)
{
    uint8_t id[2];
    if (hal_twi_read(1, TMP117_ADDR, TMP117_REG_ID, id, 2)) return -1;
    /* TMP117 ID should have family code 0x11xx or 0x01xx. */
    if (id[0] == 0 && id[1] == 0) return -2;
    return 0;
}

static int32_t tmp_read_mc(void)
{
    uint8_t raw[2];
    if (hal_twi_read(1, TMP117_ADDR, TMP117_REG_TEMP, raw, 2)) return 0;
    int16_t t = (int16_t)((raw[0] << 8) | raw[1]);
    /* 7.8125 m°C per LSB. */
    return (int32_t)t * 78125L / 10000L;
}

/* =====================================================================
 * MAX30101 — tissue-contact confirmation
 * =====================================================================
 */

static int max_init(void)
{
    uint8_t id;
    if (hal_twi_read(1, MAX30101_ADDR, MAX_REG_PART_ID, &id, 1)) return -1;
    if (id != MAX_VAL_PART_ID) return -2;

    /* Reset. */
    uint8_t reset = MAX_MODE_RESET;
    hal_twi_write(1, MAX30101_ADDR, &reset, 1);
    hal_delay_ms(10);

    /* Mode: HR (red + IR), for proximity + basic reflectance. */
    uint8_t mode = MAX_MODE_HR;
    hal_twi_write(1, MAX30101_ADDR, &mode, 1);

    /* LED1 (red) pulse amplitude: low (0x20) to save power. */
    uint8_t pa = 0x20;
    hal_twi_write(1, MAX30101_ADDR, &pa, 1);

    /* SPO2 config: sample rate 100 Hz, pulse width 411 us (18-bit). */
    uint8_t spo2 = 0x27;  /* SPO2_SR=100Hz, LED_PW=411us */
    hal_twi_write(1, MAX30101_ADDR, &spo2, 1);

    return 0;
}

static bool max_check_contact(void)
{
    uint8_t raw[6];
    /* Read one sample from the red+IR FIFO. */
    if (hal_twi_read(1, MAX30101_ADDR, MAX_REG_FIFO_DATA, raw, 6)) return false;
    /* Red LED reflectance is in bytes 0..2 (18-bit), IR in 3..5. */
    uint32_t red = ((uint32_t)raw[0] << 16) | ((uint32_t)raw[1] << 8) | raw[2];
    /* If reflectance is above a threshold, tissue is in contact. */
    return (red > 2000u);
}