/*
 * piezo.c — ADS131M04 ADC driver & droplet event extractor
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * Drives the ADS131M04 4-channel 24-bit delta-sigma ADC over SPI2 at
 * 4 kSPS/channel. On a droplet impact (signalled by the LTC5500 peak
 * comparator IRQ on PIN_PEAK_IRQ), the MCU wakes from deep sleep,
 * powers on the charge amplifiers and ADC via PIN_ADC_PWREN, waits
 * for the ADS131M04 to settle (~2 ms), then captures a 128-sample
 * window across all 4 PVDF cantilever channels.
 *
 * For each window the driver extracts the raw impulse features:
 *   - peak amplitude (µV) per channel
 *   - integrated V²·dt (energy proxy) per channel
 *   - pulse full-width half-max (µs)
 *   - bipolar asymmetry (+peak / -peak ratio, Q0.15)
 *   - ringing decay constant (µs)
 *   - charge polarity flag (from asymmetry sign)
 *
 * The physical features (diameter, velocity, charge) are derived from
 * the raw features via the calibration polynomial in piezo_extract().
 */
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "../board.h"
#include "../registers.h"
#include "piezo.h"

/* ---- SPI abstraction (ESP-IDF-style, stubbed here for portability) ----
 * In a real build these map to esp_rom_spi_* or the HAL. We declare
 * them weak so the ESP-IDF build can override them. */
__attribute__((weak)) void spi2_init(uint32_t hz);
__attribute__((weak)) void spi2_cs(int cs_pin, int low);
__attribute__((weak)) void spi2_xfer(const uint8_t *tx, uint8_t *rx, uint16_t len);

/* ---- GPIO abstraction ---- */
__attribute__((weak)) void gpio_dir(int pin, int output);
__attribute__((weak)) void gpio_write(int pin, int high);
__attribute__((weak)) int  gpio_read(int pin);
__attribute__((weak)) void gpio_irq_enable(int pin, int falling_edge);
__attribute__((weak)) void delay_ms(uint32_t ms);
__attribute__((weak)) uint32_t millis(void);

/* ---- Internal state ---- */
static int g_adc_powered = 0;
static volatile int g_peak_flag = 0;   /* set by peak IRQ (LTC5500) */

/* DMA-style capture buffer: 128 samples × 4 channels × 4 bytes */
static int32_t g_capture_buf[ADC_WINDOW * ADC_CHANNELS];
static uint16_t g_capture_idx;

/* ---- Forward decls ---- */
static void adc_write_reg(uint8_t reg, uint16_t val);
static uint16_t adc_read_reg(uint8_t reg);

/* ================================================================
 * Peak-detection ISR (called from GPIO interrupt on PIN_PEAK_IRQ)
 * ================================================================ */
void piezo_peak_isr(void)   /* weak override target */
{
    g_peak_flag = 1;
}

/* ================================================================
 * Initialization
 * ================================================================ */
void piezo_init(void)
{
    /* Configure ADC control pins as outputs, default low (ADC off) */
    gpio_dir(PIN_ADC_DRDY, 0);      /* input */
    gpio_dir(PIN_ADC_SYNC, 1);      /* output */
    gpio_dir(PIN_ADC_PWREN, 1);     /* output */
    gpio_dir(PIN_SPI2_CS_ADC, 1);   /* output (CS) */
    gpio_write(PIN_ADC_PWREN, 0);
    gpio_write(PIN_ADC_SYNC, 0);
    gpio_write(PIN_SPI2_CS_ADC, 1); /* CS high = idle */

    /* Configure peak-IRQ pin as input with falling-edge interrupt.
     * The LTC5500 comparator output pulses low for ~10 µs on a
     * raindrop impact that exceeds the energy threshold. */
    gpio_dir(PIN_PEAK_IRQ, 0);
    gpio_irq_enable(PIN_PEAK_IRQ, 1);

    g_adc_powered = 0;
    g_peak_flag = 0;
    g_capture_idx = 0;
}

/* ================================================================
 * Power on the analog front-end and configure the ADS131M04
 * ================================================================ */
void piezo_power_on(void)
{
    if (g_adc_powered)
        return;

    /* 1. Enable the op-amp + ADC power rail */
    gpio_write(PIN_ADC_PWREN, 1);

    /* 2. Wait for the AD8656 op-amps to settle (chopper startup) */
    delay_ms(3);

    /* 3. Initialize SPI2 at 4 MHz for ADC + FRAM */
    spi2_init(SPI2_SPEED_HZ);

    /* 4. Reset the ADS131M04 via the SYNC/RESET pin */
    gpio_write(PIN_ADC_SYNC, 0);
    delay_ms(1);
    gpio_write(PIN_ADC_SYNC, 1);
    delay_ms(2);   /* internal startup */

    /* 5. Send software RESET command for good measure */
    spi2_cs(PIN_SPI2_CS_ADC, 1);
    uint8_t cmd[2] = { (uint8_t)(ADS131_CMD_RESET >> 8),
                       (uint8_t)(ADS131_CMD_RESET & 0xFF) };
    spi2_cs(PIN_SPI2_CS_ADC, 0);
    spi2_xfer(cmd, NULL, 2);
    spi2_cs(PIN_SPI2_CS_ADC, 1);
    delay_ms(1);

    /* 6. Configure: MODE register — enable CRC, DRDY as push-pull,
     *    high-Z when not ready. */
    uint16_t mode = ADS131_MODE_DRDY_HIZ | ADS131_MODE_CRC_EN;
    adc_write_reg(ADS131_REG_MODE, mode);

    /* 7. CLOCK register — set clock divider for 4 kSPS.
     *    The ADS131M04 uses clock = 8.192 MHz internal; dividing by
     *    2 gives 4.096 kSPS. Register value 0x05 = /2 + high-res. */
    adc_write_reg(ADS131_REG_CLOCK, 0x0500);

    /* 8. GAIN register — PGA gain 1 on all 4 channels.
     *    Bits [13:12] ch3, [11:10] ch2, [9:8] ch1, [7:6] ch0.
     *    0b00 = gain 1. Leave upper bits 0. */
    adc_write_reg(ADS131_REG_GAIN, 0x0000);

    /* 9. CONFIG register — set all channels to enabled, no test. */
    adc_write_reg(ADS131_REG_CFG, 0x0000);

    g_adc_powered = 1;
    delay_ms(1);
}

void piezo_power_off(void)
{
    if (!g_adc_powered)
        return;

    /* Put ADC in standby */
    uint8_t cmd[2] = { (uint8_t)(ADS131_CMD_STANDBY >> 8),
                       (uint8_t)(ADS131_CMD_STANDBY & 0xFF) };
    spi2_cs(PIN_SPI2_CS_ADC, 0);
    spi2_xfer(cmd, NULL, 2);
    spi2_cs(PIN_SPI2_CS_ADC, 1);

    /* Cut the op-amp / ADC power rail */
    gpio_write(PIN_ADC_PWREN, 0);
    g_adc_powered = 0;
}

uint8_t piezo_is_busy(void)
{
    return g_peak_flag || (g_capture_idx < ADC_WINDOW && g_capture_idx > 0);
}

/* ================================================================
 * Low-level ADC register access
 * ================================================================ */
static void adc_write_reg(uint8_t reg, uint16_t val)
{
    uint16_t cmd = ADS131_CMD_WREG | (reg << 0);
    uint8_t tx[4] = {
        (uint8_t)(cmd >> 8),
        (uint8_t)(cmd & 0xFF),
        (uint8_t)(val >> 8),
        (uint8_t)(val & 0xFF)
    };
    spi2_cs(PIN_SPI2_CS_ADC, 0);
    spi2_xfer(tx, NULL, 4);
    spi2_cs(PIN_SPI2_CS_ADC, 1);
}

static uint16_t adc_read_reg(uint8_t reg)
{
    uint16_t cmd = ADS131_CMD_RREG | (reg << 0);
    uint8_t tx[2] = { (uint8_t)(cmd >> 8), (uint8_t)(cmd & 0xFF) };
    uint8_t rx[2] = { 0, 0 };
    spi2_cs(PIN_SPI2_CS_ADC, 0);
    spi2_xfer(tx, rx, 2);
    /* ADS131M04 sends 2-byte response after the command (no dummy) */
    spi2_xfer(NULL, rx, 2);
    spi2_cs(PIN_SPI2_CS_ADC, 1);
    return ((uint16_t)rx[0] << 8) | rx[1];
}

/* ================================================================
 * Read one ADC frame (status + 4 samples + CRC) = 12 bytes
 * ================================================================ */
int piezo_read_adc_frame(int32_t samples[ADC_CHANNELS])
{
    uint8_t rx[ADS131_FRAME_BYTES];
    uint8_t tx[ADS131_FRAME_BYTES];
    memset(tx, 0, sizeof(tx));

    /* Wait for DRDY (active low on the DRDY pin) */
    int timeout = 5000;  /* up to ~1.25 ms at 4 kSPS */
    while (gpio_read(PIN_ADC_DRDY) == 1) {
        if (--timeout == 0)
            return -1;   /* timeout */
    }

    spi2_cs(PIN_SPI2_CS_ADC, 0);
    spi2_xfer(tx, rx, ADS131_FRAME_BYTES);
    spi2_cs(PIN_SPI2_CS_ADC, 1);

    /* Parse: word0 = status (2 bytes), words 1..4 = samples (4 bytes each,
     * 24-bit signed left-justified in 32-bit word), word5 = CRC (2 bytes). */
    for (int ch = 0; ch < ADC_CHANNELS; ++ch) {
        int off = 2 + ch * 4;   /* offset into rx[] */
        /* Big-endian 24-bit sample in 4-byte word, MSB at off[0] */
        int32_t raw = ((int32_t)rx[off] << 24) |
                      ((int32_t)rx[off + 1] << 16) |
                      ((int32_t)rx[off + 2] << 8) |
                      ((int32_t)rx[off + 3]);
        /* Arithmetic right-shift to get the 24-bit value */
        samples[ch] = raw >> 8;   /* now 24-bit signed */
    }
    return 0;
}

void piezo_reset_adc(void)
{
    gpio_write(PIN_ADC_SYNC, 0);
    delay_ms(1);
    gpio_write(PIN_ADC_SYNC, 1);
    delay_ms(2);
}

/* ================================================================
 * Capture a 128-sample window after a peak-detection event
 * ================================================================ */
static int capture_window(void)
{
    g_capture_idx = 0;
    int32_t tmp[ADC_CHANNELS];

    while (g_capture_idx < ADC_WINDOW) {
        if (piezo_read_adc_frame(tmp) != 0)
            return -1;
        for (int ch = 0; ch < ADC_CHANNELS; ++ch)
            g_capture_buf[g_capture_idx * ADC_CHANNELS + ch] = tmp[ch];
        g_capture_idx++;
    }
    return 0;
}

/* ================================================================
 * Extract raw features from the captured window
 * ================================================================ */
static void extract_features(droplet_raw_t *ev)
{
    int32_t max_val[ADC_CHANNELS], min_val[ADC_CHANNELS];
    int32_t sum_sq[ADC_CHANNELS];
    int32_t global_max = 0, global_min = 0;
    int max_ch = 0;

    memset(ev, 0, sizeof(*ev));

    for (int ch = 0; ch < ADC_CHANNELS; ++ch) {
        max_val[ch] = -0x7FFFFF;
        min_val[ch] =  0x7FFFFF;
        sum_sq[ch]  = 0;
    }

    /* Single pass: find peak/trough per channel and sum-of-squares.
     * Also identify which channel has the largest peak (closest to
     * the impact point — the 4 cantilevers are at 90° spacing). */
    for (int i = 0; i < ADC_WINDOW; ++i) {
        for (int ch = 0; ch < ADC_CHANNELS; ++ch) {
            int32_t s = g_capture_buf[i * ADC_CHANNELS + ch];
            if (s > max_val[ch]) max_val[ch] = s;
            if (s < min_val[ch]) min_val[ch] = s;

            /* Accumulate V² in (ADC-count)² units; convert to (µV)² later.
             * We use 64-bit to avoid overflow on long windows. */
            int64_t sq = (int64_t)s * (int64_t)s;
            sum_sq[ch] += (int32_t)(sq >> 16);   /* scale down to fit int32 */
        }
    }

    /* Find the dominant channel (largest |peak|) */
    int32_t largest_abs = 0;
    for (int ch = 0; ch < ADC_CHANNELS; ++ch) {
        int32_t pk = max_val[ch];
        int32_t tr = -min_val[ch];
        int32_t abs_max = pk > tr ? pk : tr;
        if (abs_max > largest_abs) {
            largest_abs = abs_max;
            max_ch = ch;
            global_max = max_val[ch];
            global_min = min_val[ch];
        }
    }

    /* Convert ADC counts to µV. The ADS131M04 at gain 1 with 2.5 V
     * reference has LSB = 2.5V / 2^23 = 0.298 µV/count. We use
     * 0.298 µV/count as the scale factor. */
    const float COUNT_TO_UV = 0.298f;
    const float SAMPLE_PERIOD_US = 1.0e6f / (float)ADC_SAMPLE_RATE; /* 250 µs */

    for (int ch = 0; ch < ADC_CHANNELS; ++ch) {
        int32_t pk = max_val[ch] > (-min_val[ch]) ? max_val[ch] : (-min_val[ch]);
        ev->peak_uv[ch] = (int32_t)(pk * COUNT_TO_UV);

        /* Energy ∫V² dt in (µV)²·µs: sum_sq[ch] is in (count)²/65536 units,
         * so E = sum_sq[ch] * 65536 * COUNT_TO_UV² * T_sample. */
        float e = (float)sum_sq[ch] * 65536.0f * COUNT_TO_UV * COUNT_TO_UV * SAMPLE_PERIOD_US;
        ev->energy_uv2us[ch] = (int32_t)e;
    }

    /* Pulse width (FWHM) on the dominant channel.
     * Find the half-max level and measure the time between the first
     * upward and first downward crossing. */
    int32_t half = global_max / 2;
    if (half < 0) half = -half / 2;
    int half_start = -1, half_end = -1;
    for (int i = 0; i < ADC_WINDOW; ++i) {
        int32_t s = g_capture_buf[i * ADC_CHANNELS + max_ch];
        if (half_start < 0 && s >= half) half_start = i;
        else if (half_start >= 0 && s < half) { half_end = i; break; }
    }
    if (half_start >= 0 && half_end > half_start)
        ev->width_us = (uint16_t)((half_end - half_start) * (int)SAMPLE_PERIOD_US);
    else
        ev->width_us = 0;

    /* Bipolar asymmetry = +peak / (+peak + |-peak|) in Q0.15.
     * 0.5 = symmetric, >0.5 = positive-dominant, <0.5 = negative-dominant. */
    if (global_max + (-global_min) > 0) {
        float ratio = (float)global_max / (float)(global_max + (-global_min));
        ev->asymmetry_q15 = (int16_t)(ratio * 32767.0f);
    }

    /* Charge polarity: positive-dominant piezo response indicates the
     * droplet carried net positive charge (or vice-versa, depending on
     * PVDF orientation — calibrated per-unit). */
    if (ev->asymmetry_q15 > 0.55f * 32767)
        ev->flags |= 0x01;   /* charge positive */
    else if (ev->asymmetry_q15 < 0.45f * 32767)
        ev->flags &= ~0x01;  /* charge negative */

    /* Saturation flag: if any channel hit within 5% of full-scale */
    if (largest_abs > 0x7A0000)
        ev->flags |= 0x02;

    /* Ringing decay constant: estimate by counting samples until the
     * envelope falls below 1/e of the peak. */
    int32_t threshold_decay = global_max / 3;   /* ~1/e */
    if (threshold_decay < 0) threshold_decay = -threshold_decay / 3;
    int decay_samples = 0;
    for (int i = (half_end > 0 ? half_end : 0); i < ADC_WINDOW; ++i) {
        int32_t s = g_capture_buf[i * ADC_CHANNELS + max_ch];
        if (s < 0) s = -s;
        if (s > threshold_decay) decay_samples++;
        else break;
    }
    ev->decay_tau_us = (uint16_t)(decay_samples * (int)SAMPLE_PERIOD_US);

    ev->timestamp_ms = millis();
}

/* ================================================================
 * Public: capture a single droplet event (if peak flag is set)
 * ================================================================ */
uint8_t piezo_capture(droplet_raw_t *out)
{
    if (!g_peak_flag)
        return 0;

    g_peak_flag = 0;

    if (!g_adc_powered)
        piezo_power_on();

    /* Flush any stale samples by reading 2 frames (the ADS131M04
     * pipeline has 2 conversion-cycle latency after power-up). */
    int32_t dummy[ADC_CHANNELS];
    piezo_read_adc_frame(dummy);
    piezo_read_adc_frame(dummy);

    /* Capture the 128-sample window.
     * The window starts ~2 samples after the peak (due to pipeline
     * latency), which captures the full impulse + ringing. */
    if (capture_window() != 0) {
        piezo_power_off();
        return 0;   /* ADC timeout — discard */
    }

    extract_features(out);
    piezo_power_off();
    return 1;
}

/* ================================================================
 * Convert raw features → physical features via calibration
 * ================================================================ */
void piezo_extract(const droplet_raw_t *raw, droplet_feature_t *feat,
                   const calibration_t *cal)
{
    /* Use the dominant channel (channel with largest peak).
     * In a production unit the 4 cantilevers have nearly identical
     * sensitivity after calibration, so we average the two largest
     * channels to improve SNR. */
    int32_t sorted_pk[ADC_CHANNELS];
    memcpy(sorted_pk, raw->peak_uv, sizeof(sorted_pk));

    /* Simple insertion sort (4 elements) */
    for (int i = 1; i < ADC_CHANNELS; ++i) {
        int32_t v = sorted_pk[i];
        int j = i - 1;
        while (j >= 0 && sorted_pk[j] < v) {
            sorted_pk[j + 1] = sorted_pk[j];
            j--;
        }
        sorted_pk[j + 1] = v;
    }

    /* Average of the two largest peaks (mV) */
    float peak_mv = (sorted_pk[0] + sorted_pk[1]) * 0.5f * 0.001f;

    /* Total energy across all channels (µV²·µs → mV²·ms) */
    float total_e = 0.0f;
    for (int ch = 0; ch < ADC_CHANNELS; ++ch)
        total_e += (float)raw->energy_uv2us[ch];
    total_e *= 1e-9f;   /* (µV)²·µs → (mV)²·ms */

    /* Diameter: D = a·peak + b·width + c·decay + d  (mm) */
    float peak = peak_mv;
    float width = (float)raw->width_us * 0.001f;       /* ms */
    float decay = (float)raw->decay_tau_us * 0.001f;   /* ms */

    feat->diameter_mm = cal->diameter_a * peak
                      + cal->diameter_b * width
                      + cal->diameter_c * decay
                      + cal->diameter_d;

    /* Clamp to physical range */
    if (feat->diameter_mm < 0.1f) feat->diameter_mm = 0.1f;
    if (feat->diameter_mm > 8.0f) feat->diameter_mm = 8.0f;

    /* Velocity: v = a / width + b  (m/s) */
    if (raw->width_us > 0) {
        feat->velocity_ms = cal->velocity_a / (float)raw->width_us
                          + cal->velocity_b;
    } else {
        feat->velocity_ms = 0.0f;
    }

    /* Charge: q = a · asymmetry + b  (pC)
     * asymmetry is Q0.15, convert to float [0,1] */
    float asym = (float)raw->asymmetry_q15 / 32767.0f;
    feat->charge_pc = cal->charge_a * (asym - 0.5f) + cal->charge_b;

    /* Kinetic energy: E = ½·m·v² where m = (4/3)π·r³·ρ_water
     * We also report the piezo-harvested energy from the calib scale. */
    float r = feat->diameter_mm * 0.5f * 0.001f;   /* radius in m */
    float vol = (4.0f / 3.0f) * 3.14159f * r * r * r;   /* m³ */
    float mass = vol * 1000.0f;   /* kg (ρ_water = 1000 kg/m³) */
    feat->kinetic_energy_uj = 0.5f * mass * feat->velocity_ms * feat->velocity_ms * 1e6f;

    feat->timestamp_ms = raw->timestamp_ms;
}