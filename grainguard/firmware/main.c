/*
 * main.c — GrainGuard firmware main application
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * GrainGuard is an in-silo grain condition probe that measures CO2,
 * 9-zone temperature, RH/moisture, and acoustic insect activity,
 * fuses them into a Spoilage Risk Index, logs to flash, and
 * transmits via a LoRa mesh.
 *
 * Architecture: bare-metal cooperative scheduler, no RTOS.
 *   - RTC wake-up every 15 min: measure T/RH/CO2, compute SRI, log, TX
 *   - RTC wake-up every 6 hr:  acoustic insect scan
 *   - Sleep in STOP2 (1.2 uA) between measurements
 *
 * Main loop runs once per wake, does the scheduled work, then sleeps.
 */

#include "board.h"
#include "registers.h"

#include "drivers/co2.h"
#include "drivers/temp.h"
#include "drivers/humid.h"
#include "drivers/acoustic.h"
#include "drivers/emc.h"
#include "drivers/sri.h"
#include "drivers/loramesh.h"
#include "drivers/storage.h"
#include "drivers/power.h"

/* ---- Configuration (from EEPROM in production; defaults here) ---- */
static uint8_t  cfg_grain_type = GRAIN_WHEAT;
static int16_t  cfg_safe_mc_x1000 = 13500;  /* 13.5% for wheat */
static uint8_t  cfg_caution_thresh  = SRI_DEFAULT_CAUTION;
static uint8_t  cfg_critical_thresh = SRI_DEFAULT_CRITICAL;
static uint8_t  cfg_serial = 1;
static uint8_t  cfg_aes_key[16] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};
static uint32_t cfg_meas_interval = SCHED_INTERVAL_T_RH_CO2;
static uint32_t cfg_acoustic_interval = SCHED_INTERVAL_ACOUSTIC;

/* ---- State ---- */
typedef enum {
    STATE_BOOT = 0,
    STATE_MEASURE_T_RH_CO2,
    STATE_MEASURE_ACOUSTIC,
    STATE_COMPUTE_SRI,
    STATE_LOG,
    STATE_TX,
    STATE_SLEEP,
} sys_state_t;

static sys_state_t state = STATE_BOOT;
static uint32_t boot_count = 0;
static uint32_t wake_count = 0;
static uint32_t last_acoustic_wake = 0;

/* ---- Measurement buffers ---- */
static co2_meas_t       g_co2;
static temp_profile_t   g_temp;
static humid_meas_t     g_humid;
static acoustic_result_t g_acoustic;
static sri_result_t     g_sri;

/* ---- Forward declarations ---- */
static void system_init(void);
static void load_config(void);
static void do_measurement_cycle(void);
static void do_acoustic_cycle(void);
static void do_log_and_tx(void);
static void blink_led(uint8_t n);
static uint32_t compute_sleep_duration(void);
static void build_mesh_packet(mesh_packet_t *pkt);
static void build_log_record(log_record_t *rec);

/* ---- EEPROM stub (simplified) ---- */
static void load_config(void) {
    /* In production: read 24LC02 EEPROM at addresses defined in board.h.
     * For now, use defaults.  This function would:
     *  - Read serial number (8 bytes ASCII)
     *  - Read grain type
     *  - Read safe MC threshold
     *  - Read SRI thresholds
     *  - Read measurement interval
     *  - Read AES key
     *  - Verify magic (0x47523147)
     * If magic doesn't match, write defaults and proceed.
     */
    cfg_grain_type = GRAIN_WHEAT;
    cfg_safe_mc_x1000 = (int16_t)emc_safe_threshold(cfg_grain_type);
    cfg_caution_thresh = SRI_DEFAULT_CAUTION;
    cfg_critical_thresh = SRI_DEFAULT_CRITICAL;
    cfg_meas_interval = SCHED_INTERVAL_T_RH_CO2;
    cfg_acoustic_interval = SCHED_INTERVAL_ACOUSTIC;
    cfg_serial = 1;
    /* AES key left at default (should be randomized per-device) */
}

static void system_init(void) {
    /* Enable clocks for all GPIO ports */
    /* Enable I2C1, SPI1, ADC1, RTC, AES, DMA1 clocks */

    /* Configure status LED (PA15) */
    GPIOA->MODER = (GPIOA->MODER & ~(0x3 << (PA15__LED * 2)))
                  | (GPIO_MODE_OUTPUT << (PA15__LED * 2));
    GPIOA->BSRR = (1 << (PA15__LED + 16));  /* LED off (active low assumed) */

    /* Initialize subsystems */
    power_init();
    mesh_set_serial(cfg_serial);

    /* Initialize sensors (only if cold boot) */
    if (!power_woke_from_stop()) {
        co2_init();
        temp_init();
        humid_init();
        storage_init();
    }

    /* Radio init (always, since STOP2 may reset radio) */
    mesh_init(LORA_FREQ_HZ_EU868);

    blink_led(LED_BLINK_BOOT_OK);
}

static void blink_led(uint8_t n) {
    for (uint8_t i = 0; i < n; i++) {
        GPIOA->BSRR = (1 << PA15__LED);        /* on */
        delay_ms(50);
        GPIOA->BSRR = (1 << (PA15__LED + 16)); /* off */
        delay_ms(50);
    }
}

static void do_measurement_cycle(void) {
    /* 1. Temperature (9 zones) */
    temp_trigger_conversion();   /* 750 ms blocking with strong pull-up */
    temp_read_profile(&g_temp);

    /* 2. Humidity + temperature (SHT45, 10 ms) */
    humid_measure(&g_humid);

    /* 3. CO2 (SCD41 single-shot, ~5 s blocking) */
    co2_measure_blocking(&g_co2);
    co2_power_off();  /* save power between measurements */

    /* 4. Compute EMC from T + RH */
    /* (done inside SRI, but we also log it) */
}

static void do_acoustic_cycle(void) {
    /* Run 5-minute acoustic insect scan */
    acoustic_scan(&g_acoustic);

    /* If events detected, run spectral confirmation */
    if (g_acoustic.events_per_min > 10 && g_acoustic.species == INSECT_UNKNOWN) {
        /* Spectral confirm already called inside acoustic_scan if needed */
    }
}

static void build_mesh_packet(mesh_packet_t *pkt) {
    pkt->version = 0x01;
    pkt->serial = cfg_serial;
    pkt->timestamp_min = (uint16_t)(rtc_get_epoch_seconds() / 60);

    pkt->co2_ppm_x10 = (uint16_t)(g_co2.co2_ppm / 10);
    pkt->sri = g_sri.sri;

    /* Temperature: max + 128 offset to fit in uint8 */
    int16_t tmax = 0, tmin = 0;
    if (g_temp.max_zone >= 0) tmax = g_temp.celsius_x10[g_temp.max_zone] / 10;
    if (g_temp.min_zone >= 0) tmin = g_temp.celsius_x10[g_temp.min_zone] / 10;
    pkt->tmax_offset = (uint8_t)CLAMP(tmax + 128, 0, 255);
    pkt->tmin_offset = (uint8_t)CLAMP(tmin + 128, 0, 255);
    pkt->delta_t = (uint8_t)CLAMP(g_temp.delta_x10 / 10, 0, 255);

    pkt->rh_pct = (uint8_t)CLAMP(g_humid.humidity_x100 / 100, 0, 100);

    int32_t emc = emc_compute(cfg_grain_type, g_humid.temperature_x100 / 10,
                              g_humid.humidity_x100);
    pkt->emc_x10 = (uint8_t)CLAMP(emc / 100, 0, 255);  /* EMC × 10 */

    pkt->ae_events_per_min = g_acoustic.events_per_min;
    pkt->insect_id = (uint8_t)g_acoustic.species;

    pkt->battery_mv = power_read_battery_mv();
    pkt->hop_count = 0;
    pkt->reserved = 0;
}

static void build_log_record(log_record_t *rec) {
    rec->timestamp_sec = rtc_get_epoch_seconds();
    rec->co2_ppm = g_co2.co2_ppm;

    if (g_temp.max_zone >= 0)
        rec->tmax_x10 = g_temp.celsius_x10[g_temp.max_zone];
    if (g_temp.min_zone >= 0)
        rec->tmin_x10 = g_temp.celsius_x10[g_temp.min_zone];
    rec->tdelta_x10 = g_temp.delta_x10;

    rec->rh_pct = (uint8_t)CLAMP(g_humid.humidity_x100 / 100, 0, 100);

    int32_t emc = emc_compute(cfg_grain_type, g_humid.temperature_x100 / 10,
                              g_humid.humidity_x100);
    rec->emc_x1000 = (uint16_t)CLAMP(emc, 0, 65535);

    rec->sri = g_sri.sri;
    rec->ae_events_per_min = g_acoustic.events_per_min;
    rec->insect_id = (uint8_t)g_acoustic.species;
    rec->grain_type = cfg_grain_type;
    rec->battery_mv = power_read_battery_mv();
    for (int i = 0; i < 10; i++) rec->reserved[i] = 0;
}

static void do_log_and_tx(void) {
    /* Compute SRI from all available data */
    sri_compute(&g_sri, &g_co2, &g_temp, &g_humid, &g_acoustic,
                cfg_grain_type, cfg_safe_mc_x1000,
                cfg_caution_thresh, cfg_critical_thresh);

    /* Log to flash */
    log_record_t rec;
    build_log_record(&rec);
    storage_append(&rec);

    /* Transmit via LoRa mesh */
    mesh_packet_t pkt;
    build_mesh_packet(&pkt);
    mesh_send(&pkt, cfg_aes_key);

    blink_led(LED_BLINK_TX);

    /* Alert: if SRI critical, reduce sleep interval to alert sooner.
     * If OK, sleep for full measurement interval. */
}

static uint32_t compute_sleep_duration(void) {
    uint32_t base = cfg_meas_interval;

    /* If SRI is critical, poll faster (every 5 min instead of 15) */
    if (g_sri.alert_level == 2) {
        base = 300;  /* 5 min */
    } else if (g_sri.alert_level == 1) {
        base = 600;  /* 10 min */
    }

    /* If it's time for the acoustic scan, we'll wake sooner */
    uint32_t now = rtc_get_epoch_seconds();
    uint32_t next_acoustic = last_acoustic_wake + cfg_acoustic_interval;
    if (next_acoustic - now < base) {
        base = next_acoustic - now;
    }

    /* Clamp to reasonable range */
    if (base < 60) base = 60;
    if (base > 21600) base = 21600;

    return base;
}

/* ---- Main loop ---- */

int main(void) {
    system_init();
    load_config();
    boot_count++;

    while (1) {
        uint32_t now = rtc_get_epoch_seconds();

        /* Determine what to do based on schedule */
        int do_acoustic = ((now - last_acoustic_wake) >= cfg_acoustic_interval)
                          || (boot_count == 1);

        /* Always do T/RH/CO2 measurement */
        do_measurement_cycle();

        /* Acoustic scan if scheduled */
        if (do_acoustic) {
            do_acoustic_cycle();
            last_acoustic_wake = now;
        }

        /* Compute SRI, log, transmit */
        do_log_and_tx();

        /* Check for received mesh packets to relay */
        mesh_packet_t rx_pkt;
        int rc = mesh_recv(&rx_pkt, cfg_aes_key, 2000);  /* 2 s RX window */
        if (rc == MESH_OK) {
            /* Packet was relayed inside mesh_recv().  Could also update
             * our view of other probes if needed. */
        }

        /* Compute sleep duration and enter STOP2 */
        uint32_t sleep_s = compute_sleep_duration();
        wake_count++;
        power_enter_stop2(sleep_s);

        /* ---- Woke up: loop back to top ---- */
    }

    return 0;  /* never reached */
}

/* ---- Interrupt handlers ---- */

/* RTC wake-up interrupt handler */
void RTC_WKUP_IRQHandler(void) {
    /* Clear wake-up flag */
    RTC->ISR = 0;
    /* The WFI in power_enter_stop2 will return; no action needed here. */
    nvic_clear_pending(IRQ_RTC_WUT);
}

/* Hard fault handler */
void HardFault_Handler(void) {
    while (1) {
        blink_led(LED_BLINK_ERROR);
        delay_ms(1000);
    }
}

/* ---- Version info ---- */
static const char firmware_version[] = "GrainGuard v1.0.0 (jayis1)";

/* End of main.c */