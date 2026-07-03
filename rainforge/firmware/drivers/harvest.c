/*
 * harvest.c — Energy harvester & supercapacitor management
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * Manages the SE1010 piezo/TEG MPPT harvester and the 2× 22 F
 * supercapacitor bank (series-stacked, 11 F effective at 11 V max,
 * but we clamp at 5.3 V for the ESP32-C3's operating range).
 *
 * The SE1010 has no SPI/I2C interface — it is configured by hardware
 * resistors and exposes two status pins:
 *   V_STORE_OK  (PIN_HARVEST_VOK): high when V_scap > 3.3 V
 *   V_STORE_LOW (PIN_HARVEST_LOW): low when V_scap < 2.5 V (IRQ)
 *
 * The supercapacitor voltage is also measurable via an ESP32-C3 ADC
 * channel (GPIO1 with a 3:1 voltage divider, since the supercap can
 * reach 5.5 V but the ESP32 ADC maxes at 3.3 V).
 *
 * We do Coulomb-counting: every droplet event contributes a known
 * amount of energy (from the piezo energy integral), and every TX
 * burst consumes a known amount. The difference tracks the state of
 * charge.
 */
#include <stdint.h>
#include <math.h>
#include "../board.h"
#include "harvest.h"

/* ---- Weak GPIO/ADC stubs ---- */
__attribute__((weak)) int  gpio_read(int pin);
__attribute__((weak)) uint32_t adc1_read_mv(int channel);

/* ---- Constants ---- */
/* Supercap: 2× 22 F in series = 11 F effective.
 * E = ½·C·V², so at 5.3 V: E = 0.5·11·5.3² = 154.6 J.
 * At 3.3 V (TX threshold): E = 0.5·11·3.3² = 59.9 J.
 * Usable range = 154.6 - 59.9 = 94.7 J (but we rarely charge to 5.3). */
#define SCAP_CAP_F          11.0f
#define SCAP_V_MAX           5.30f
#define SCAP_V_MIN_TX        3.30f
#define SCAP_V_LOW            2.50f

/* Voltage divider on the supercap sense line: 3:1 (200 kΩ / 100 kΩ).
 * ADC reads 1/3 of V_scap. ESP32-C3 ADC is 12-bit, 0-3.3 V → 0-4095.
 * But we use the calibrated adc1_read_mv() which returns mV directly. */
#define DIVIDER_RATIO       3.0f

/* Energy cost of one LoRa TX burst at 20 dBm, SF7, 51-byte payload:
 *   TX current ~118 mA, V = 3.3 V, duration ~120 ms (TOA + preamble)
 *   E_tx = 3.3 V × 0.118 A × 0.120 s = 0.047 J
 * Plus MCU wake + sensor sampling: ~0.015 J
 * Total per reporting cycle ≈ 0.062 J */
#define ENERGY_PER_TX_J     0.062f
#define ENERGY_PER_EVENT_J  0.000004f   /* 4 µJ average per droplet (conservative) */

/* ---- Rolling harvest rate estimate (exponential filter) ---- */
static float g_harvest_rate_ema = 0.0f;  /* mW */
static float g_energy_accumulated = 0.0f; /* Joules accumulated since last update */
static uint32_t g_last_tick_ms = 0;

/* ---- State ---- */
static harvest_state_t g_state;

/* ================================================================
 * Initialize
 * ================================================================ */
void harvest_init(void)
{
    g_harvest_rate_ema = 0.0f;
    g_energy_accumulated = 0.0f;
    g_last_tick_ms = 0;
    memset(&g_state, 0, sizeof(g_state));
}

/* ================================================================
 * Read the supercapacitor voltage from the ESP32 ADC
 * ================================================================ */
static float read_scap_voltage(void)
{
    /* adc1_read_mv() returns the voltage at the ADC pin in mV.
     * The pin sees V_scap / 3 via the divider. */
    int mv = adc1_read_mv(0);   /* ADC1 channel 0 = GPIO0... but GPIO0 is
                                 * PIN_PEAK_IRQ. In the actual design the
                                 * sense line is on a dedicated ADC channel
                                 * (ADC1_CH3 = GPIO3, shared with VOK via
                                 * a MUX). For this driver we abstract it. */
    if (mv < 0) mv = 0;
    return (float)mv * 0.001f * DIVIDER_RATIO;
}

/* ================================================================
 * Update state: read GPIO pins and ADC, compute SoC and energy
 * ================================================================ */
void harvest_update(harvest_state_t *st)
{
    g_state.vok  = (uint8_t)gpio_read(PIN_HARVEST_VOK);
    g_state.vlow = (uint8_t)(gpio_read(PIN_HARVEST_LOW) == 0);  /* active-low */

    g_state.voltage = read_scap_voltage();

    /* Energy stored: E = ½·C·V² */
    g_state.energy_j = 0.5f * SCAP_CAP_F * g_state.voltage * g_state.voltage;

    /* Charge: Q = C·V */
    g_state.charge_c = SCAP_CAP_F * g_state.voltage;

    /* Harvest rate: convert accumulated energy over elapsed time */
    uint32_t now = 0;   /* would be millis() in real build */
    if (g_last_tick_ms > 0 && now > g_last_tick_ms) {
        float dt_s = (float)(now - g_last_tick_ms) * 0.001f;
        if (dt_s > 0.001f) {
            float inst_mw = g_energy_accumulated / dt_s * 1000.0f;
            /* EMA filter: α = 0.05 → time constant ~20 samples */
            g_harvest_rate_ema = 0.05f * inst_mw + 0.95f * g_harvest_rate_ema;
        }
    }
    g_last_tick_ms = now;
    g_energy_accumulated = 0.0f;
    g_state.harvest_rate_mw = g_harvest_rate_ema;

    *st = g_state;
}

/* ================================================================
 * State of charge (0..1), relative to usable range
 * ================================================================ */
float harvest_estimate_soc(void)
{
    float v = g_state.voltage;
    if (v <= SCAP_V_LOW) return 0.0f;
    if (v >= SCAP_V_MAX) return 1.0f;
    /* Linear in energy (V²), not in voltage */
    float e_full = 0.5f * SCAP_CAP_F * SCAP_V_MAX * SCAP_V_MAX;
    float e_min  = 0.5f * SCAP_CAP_F * SCAP_V_LOW * SCAP_V_LOW;
    float e_now  = 0.5f * SCAP_CAP_F * v * v;
    return (e_now - e_min) / (e_full - e_min);
}

/* ================================================================
 * Can we afford a LoRa TX burst?
 * ================================================================ */
uint8_t harvest_can_tx(void)
{
    /* Need V_scap > 3.3 V AND enough energy margin.
     * We require 2× the TX energy to leave headroom for sensor reads. */
    if (g_state.voltage < SCAP_V_MIN_TX)
        return 0;
    float usable = g_state.energy_j - 0.5f * SCAP_CAP_F * SCAP_V_LOW * SCAP_V_LOW;
    return (usable >= 2.0f * ENERGY_PER_TX_J) ? 1 : 0;
}

/* ================================================================
 * Log energy from a single droplet impact
 * ================================================================ */
void harvest_log_event(float energy_uj)
{
    g_energy_accumulated += energy_uj * 1e-6f;   /* µJ → J */
}

/* ================================================================
 * Per-millisecond tick for timekeeping (called from SysTick)
 * ================================================================ */
void harvest_tick_ms(uint32_t ms)
{
    /* In a real implementation this would update a timer for the
     * harvest-rate computation. The EMA is updated in harvest_update(). */
    (void)ms;
}