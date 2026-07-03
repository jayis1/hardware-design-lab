/*
 * main.c — RainForge firmware super-loop
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * Entry point and orchestration for the RainForge piezoelectric
 * raindrop energy harvester & precipitation microphysics analyzer.
 *
 * Lifecycle:
 *   1. Boot: determine wake cause (GPIO peak / RTC timer / cold-start).
 *   2. If cold-start: wait in low-power loop until supercap > 3.3 V.
 *   3. Initialize drivers (FRAM, sensors, LoRa, piezo ADC, harvester).
 *   4. Load calibration from FRAM (fall back to defaults if invalid).
 *   5. Load rolling DSD statistics from FRAM (resume mid-interval).
 *   6. Dispatch based on wake cause:
 *      - WAKE_PEAK:   capture droplet event, extract features, log to
 *                     FRAM, add to DSD, log energy to harvester.
 *      - WAKE_RTC:    compute DSD products, read environmental sensors,
 *                     build LoRa payload, TX if energy permits, reset
 *                     DSD accumulator for next interval.
 *      - WAKE_VOK:    transition from cold-start to normal operation.
 *      - WAKE_RESET:  full initialization sequence.
 *   7. Configure wake sources and enter deep sleep.
 *
 * main() never exits (the deep-sleep call does not return; on wake the
 * ESP32-C3 resets and re-enters main() from the top).
 */
#include <stdint.h>
#include <string.h>
#include "board.h"
#include "registers.h"
#include "drivers/piezo.h"
#include "drivers/harvest.h"
#include "drivers/disdrometer.h"
#include "drivers/fram.h"
#include "drivers/lora.h"
#include "drivers/sensors.h"
#include "drivers/power.h"

/* ---- Weak ESP-IDF stubs ---- */
__attribute__((weak)) void esp_deep_sleep_start(void);
__attribute__((weak)) void delay_ms(uint32_t ms);
__attribute__((weak)) uint32_t millis(void);

/* ---- Global board context ---- */
board_ctx_t g_board = {0};

/* ---- DSD accumulator for the current reporting interval ---- */
static dsd_t g_dsd;

/* ---- Calibration ---- */
static calibration_t g_cal;

/* ---- LoRaWAN credentials (would be stored in FRAM config block) ---- */
static uint8_t g_dev_eui[8]  = { 0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x00, 0x00, 0x01 };
static uint8_t g_app_eui[8]  = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static uint8_t g_app_key[16] = { 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
                                 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C };

/* ---- Effective sample area of the impact plate ----
 * 90 mm diameter, minus 10 mm central deflector hub
 * A = π × (0.045² - 0.005²) = 6.283 × 10⁻³ m² */
#define SAMPLE_AREA_M2  6.283e-3f

/* ---- Reporting interval (ms) — overridden by adaptive logic ---- */
static uint32_t g_report_interval_ms = 900000;  /* 15 min default */

/* ================================================================
 * Downlink handler (called from lora.c when a downlink is received)
 * ================================================================ */
static void on_downlink(uint8_t port, const uint8_t *data, uint8_t len)
{
    if (port == 1 && len >= 1) {
        /* Port 1 = command byte */
        switch (data[0]) {
        case 0x01:   /* start calibration mode */
            g_board.state = STATE_CALIBRATION;
            break;
        case 0x02:   /* clear event log */
            fram_clear_events();
            break;
        case 0x03:   /* set report interval (2-byte little-endian, in minutes) */
            if (len >= 3) {
                uint16_t min = (uint16_t)data[1] | ((uint16_t)data[2] << 8);
                if (min > 0 && min <= 360)
                    g_report_interval_ms = (uint32_t)min * 60 * 1000;
            }
            break;
        case 0x04:   /* heater pulse (condensation removal) */
            sensors_heater_pulse();
            break;
        default:
            break;
        }
    }
}

/* ================================================================
 * Full initialization sequence (WAKE_RESET or first boot)
 * ================================================================ */
static void full_init(void)
{
    /* Initialize power/wake management first */
    power_init();

    /* Initialize FRAM (needed for calibration & stats) */
    fram_init();

    /* Verify FRAM is present */
    uint8_t fram_id[FRAM_ID_LEN];
    if (fram_read_id(fram_id) != 0) {
        g_board.state = STATE_FAULT;
        return;
    }

    /* Load calibration from FRAM, fall back to defaults */
    if (fram_load_calibration(&g_cal) != 0) {
        const calibration_t *def = disdrometer_default_cal();
        memcpy(&g_cal, def, sizeof(g_cal));
        fram_save_calibration(&g_cal);
    }

    /* Load rolling DSD statistics (resume mid-interval after reset) */
    if (fram_load_stats(&g_dsd) != 0)
        disdrometer_init(&g_dsd);

    /* Initialize energy harvester state */
    harvest_init();

    /* Initialize environmental sensors */
    sensors_init();

    /* Initialize piezo ADC (configured but powered off) */
    piezo_init();

    /* Initialize LoRa radio */
    lora_init();
    lora_set_downlink_cb(on_downlink);

    /* Attempt LoRaWAN join (non-fatal if it fails — we retry next cycle) */
    lora_join(g_app_eui, g_app_key, g_dev_eui);

    g_board.state = STATE_HARVESTING;
}

/* ================================================================
 * Handle a droplet impact event (WAKE_PEAK)
 * ================================================================ */
static void handle_droplet_event(void)
{
    droplet_raw_t raw;
    droplet_feature_t feat;

    /* Capture the impulse waveform and extract raw features */
    if (piezo_capture(&raw) == 0)
        return;   /* ADC timeout or no event — go back to sleep */

    /* Convert raw features to physical features using calibration */
    piezo_extract(&raw, &feat, &g_cal);

    /* Sanity-check: discard events with unreasonable values */
    if (feat.diameter_mm < 0.1f || feat.diameter_mm > 8.0f)
        return;
    if (feat.velocity_ms < 0.1f || feat.velocity_ms > 12.0f)
        return;

    /* Log the event to FRAM ring buffer */
    fram_log_event(&feat);

    /* Add to the DSD histogram */
    disdrometer_add(&g_dsd, &feat);

    /* Log the harvested energy to the harvester's Coulomb counter */
    harvest_log_event(feat.kinetic_energy_uj * 0.001f);  /* µJ → approx J fraction */

    /* Update board context */
    g_board.events_this_interval++;
    g_board.events_total++;
}

/* ================================================================
 * Handle reporting interval expiry (WAKE_RTC)
 * ================================================================ */
static void handle_report(void)
{
    env_sensors_t env;
    harvest_state_t hst;

    /* Read environmental sensors */
    sensors_read_all(&env);
    g_board.temperature = env.temperature_c;
    g_board.humidity = env.humidity_pct;
    g_board.pressure = env.pressure_pa;
    g_board.ambient_lux = env.ambient_lux;

    /* Update harvester state */
    harvest_update(&hst);
    g_board.scap_voltage = hst.voltage;

    /* If SHT45 reads >99% RH, pulse the heater to clear condensation */
    if (env.humidity_pct > 99.0f && env.temperature_c < 5.0f)
        sensors_heater_pulse();

    /* Compute DSD derived products for this interval */
    float interval_s = (float)g_report_interval_ms * 0.001f;
    disdrometer_compute(&g_dsd, SAMPLE_AREA_M2, interval_s);

    /* Save statistics to FRAM (double-buffered) */
    fram_save_stats(&g_dsd);

    /* Build and transmit LoRa payload if we have enough energy */
    if (harvest_can_tx()) {
        uint8_t payload[LORA_PAYLOAD_LEN];
        lora_build_payload(&g_dsd, hst.voltage, env.temperature_c, payload);

        g_board.state = STATE_TX;
        int rc = lora_tx(payload, LORA_PAYLOAD_LEN, 2);
        (void)rc;   /* result logged but not retried in this cycle */

        g_board.state = STATE_HARVESTING;
    }

    /* Reset DSD accumulator for the next interval */
    disdrometer_reset(&g_dsd);
    g_board.events_this_interval = 0;

    /* Adaptive reporting interval: if rain is active, report every
     * 15 min; if dry (no events), extend to 6 hours to conserve energy. */
    if (g_board.events_this_interval > 0)
        g_report_interval_ms = 900000;    /* 15 min */
    else
        g_report_interval_ms = 21600000;  /* 6 hr */
}

/* ================================================================
 * Main entry point — called on every wake from deep sleep
 * ================================================================ */
int main(void)
{
    /* Determine why we woke up */
    wake_source_t cause = power_get_wake_cause();
    g_board.wake_reason = cause;
    g_board.boot_count++;

    /* Cold-start handling: if the supercap is too low, wait for it
     * to charge before doing anything. */
    if (cause == WAKE_RESET && !power_scap_ok()) {
        power_cold_start_wait();
        cause = WAKE_VOK;
    }

    /* Full initialization (drivers, FRAM, sensors, LoRa)
     * On every wake we re-initialize because deep sleep preserves
     * only RTC and ULP memory — all peripheral state is lost. */
    if (g_board.state == STATE_FAULT)
        g_board.state = STATE_HARVESTING;  /* retry after fault */

    full_init();

    /* Dispatch based on wake cause */
    switch (cause) {
    case WAKE_PEAK:
        /* A raindrop impacted the plate — capture and log it */
        handle_droplet_event();
        break;

    case WAKE_RTC:
        /* Reporting interval elapsed — compute & transmit DSD */
        handle_report();
        break;

    case WAKE_VOK:
        /* Supercap reached operating voltage after cold-start */
        g_board.state = STATE_HARVESTING;
        /* Fall through to also check for pending events */

    case WAKE_RESET:
        /* Power-on or external reset — do a sensor sweep + first report
         * if we have enough energy, otherwise just start harvesting. */
        if (harvest_can_tx())
            handle_report();
        break;

    case WAKE_BLE:
        /* BLE connection for calibration — handled in calibration mode */
        g_board.state = STATE_CALIBRATION;
        /* In calibration mode, we stay awake and process commands via
         * the BLE GATT server (not implemented in this driver set). */
        break;

    default:
        break;
    }

    /* Also check if the peak IRQ fired during our processing
     * (multiple droplets may have arrived while we were awake). */
    int events_processed = 0;
    while (events_processed < 8) {   /* cap to avoid staying awake too long */
        /* Check peak flag (would be set by the GPIO ISR) */
        extern void piezo_peak_isr(void);
        /* In a real build, the ISR sets a flag; we check it via piezo_is_busy()
         * or a global. For this super-loop we attempt one capture. */
        droplet_raw_t raw;
        if (piezo_capture(&raw) == 0)
            break;

        droplet_feature_t feat;
        piezo_extract(&raw, &feat, &g_cal);
        if (feat.diameter_mm >= 0.1f && feat.diameter_mm <= 8.0f) {
            fram_log_event(&feat);
            disdrometer_add(&g_dsd, &feat);
            harvest_log_event(feat.kinetic_energy_uj * 0.001f);
            g_board.events_this_interval++;
            g_board.events_total++;
        }
        events_processed++;
    }

    /* Put peripherals to sleep */
    piezo_power_off();
    lora_sleep();
    fram_sleep();

    /* Configure wake sources for next sleep */
    power_configure_wake_sources();

    /* Enter deep sleep — does not return */
    power_deep_sleep(g_report_interval_ms, WAKE_PEAK);

    /* Should never reach here */
    while (1) {
        delay_ms(1000);
    }

    return 0;
}