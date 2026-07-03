/*
 * power.c — Deep-sleep management & wake-source routing
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * The ESP32-C3 deep-sleep current is 3.5 µA (RTC + ULP off). We use
 * three wake sources:
 *
 *   1. GPIO wake (PIN_PEAK_IRQ): falling edge from the LTC5500 peak
 *      comparator — a raindrop impact exceeded the energy threshold.
 *      This is the primary wake source during rain.
 *
 *   2. RTC timer: periodic wake every REPORT_INTERVAL_MS (15 min in
 *      active rain, 6 hr in dry conditions). This drives the reporting
 *      cycle even if no individual droplet triggers a GPIO wake.
 *
 *   3. GPIO wake (PIN_HARVEST_VOK): rising edge when the SE1010
 *      signals the supercapacitor has reached 3.3 V. Used only during
 *      cold-start (when the system boots from a fully depleted state).
 *
 * On wake, power_get_wake_cause() determines which source fired by
 * checking the ESP32-C3's RTC_IO and GPIO status registers (abstracted
 * via the weak esp_sleep_get_wakeup_cause() stub).
 */
#include <stdint.h>
#include "../board.h"
#include "power.h"

/* ---- Weak ESP-IDF abstraction ---- */
__attribute__((weak)) void esp_deep_sleep_start(void);
__attribute__((weak)) void esp_sleep_enable_gpio_wakeup(int pin, int level);
__attribute__((weak)) void esp_sleep_enable_timer_us(uint64_t us);
__attribute__((weak)) int  esp_sleep_get_wakeup_cause(void);
__attribute__((weak)) int  gpio_read(int pin);
__attribute__((weak)) void gpio_dir(int pin, int output);
__attribute__((weak)) void delay_ms(uint32_t ms);
__attribute__((weak)) uint32_t adc1_read_mv(int channel);

/* ---- Constants ---- */
/* Report interval: 15 min in active rain, 6 hr dry */
#define REPORT_INTERVAL_RAIN_MS   (15UL * 60 * 1000)
#define REPORT_INTERVAL_DRY_MS    (6UL * 60 * 60 * 1000)
#define COLD_START_POLL_MS        5000

/* Voltage divider ratio (3:1) on the supercap sense line */
#define SCAP_DIVIDER  3.0f

/* ---- Current wake cause (read on boot) ---- */
static wake_source_t g_wake_cause = WAKE_RESET;

/* ================================================================
 * Initialize
 * ================================================================ */
void power_init(void)
{
    /* Configure wake-source pins as inputs */
    gpio_dir(PIN_PEAK_IRQ, 0);
    gpio_dir(PIN_HARVEST_VOK, 0);
    gpio_dir(PIN_HARVEST_LOW, 0);

    /* Determine wake cause from ESP-IDF */
    int cause = esp_sleep_get_wakeup_cause();
    /* 0 = not from sleep (power-on reset / watchdog), 1 = GPIO, 2 = timer */
    if (cause == 0)
        g_wake_cause = WAKE_RESET;
    else if (cause == 1) {
        /* Distinguish which GPIO woke us by reading pin states.
         * The LTC5500 peak IRQ is a ~10 µs pulse, so by the time we
         * read it the pulse may be gone. We use the RTC_IO latch
         * (abstracted here as a priority check: if VOK is high, it
         * was the cold-start wake; otherwise assume peak IRQ). */
        if (gpio_read(PIN_HARVEST_VOK) == 1 && g_board.state == STATE_COLDSTART)
            g_wake_cause = WAKE_VOK;
        else
            g_wake_cause = WAKE_PEAK;
    }
    else if (cause == 2)
        g_wake_cause = WAKE_RTC;
    else
        g_wake_cause = WAKE_RESET;
}

/* ================================================================
 * Configure wake sources for the next sleep cycle
 * ================================================================ */
void power_configure_wake_sources(void)
{
    /* GPIO wake on PIN_PEAK_IRQ (active-low pulse from LTC5500) */
    esp_sleep_enable_gpio_wakeup(PIN_PEAK_IRQ, 0);  /* 0 = low level */

    /* GPIO wake on PIN_HARVEST_VOK (active-high, for cold-start) */
    esp_sleep_enable_gpio_wakeup(PIN_HARVEST_VOK, 1);  /* 1 = high level */

    /* RTC timer: choose interval based on current state.
     * If we're in active rain (events_this_interval > 0), use 15 min.
     * If dry, use 6 hr. */
    uint32_t interval = (g_board.events_this_interval > 0)
                        ? REPORT_INTERVAL_RAIN_MS
                        : REPORT_INTERVAL_DRY_MS;
    esp_sleep_enable_timer_us((uint64_t)interval * 1000);
}

/* ================================================================
 * Enter deep sleep
 * ================================================================ */
void power_deep_sleep(uint32_t rtc_wake_ms, wake_source_t expected)
{
    /* Configure RTC timer (override default interval if provided) */
    if (rtc_wake_ms > 0)
        esp_sleep_enable_timer_us((uint64_t)rtc_wake_ms * 1000);
    else
        power_configure_wake_sources();

    /* Enter deep sleep — does not return */
    esp_deep_sleep_start();
}

/* ================================================================
 * Get the wake cause for this boot
 * ================================================================ */
wake_source_t power_get_wake_cause(void)
{
    return g_wake_cause;
}

/* ================================================================
 * Supercapacitor voltage & status
 * ================================================================ */
float power_get_scap_voltage(void)
{
    int mv = adc1_read_mv(0);   /* ADC channel 0 on the sense divider */
    if (mv < 0) mv = 0;
    return (float)mv * 0.001f * SCAP_DIVIDER;
}

uint8_t power_scap_ok(void)
{
    return (uint8_t)(gpio_read(PIN_HARVEST_VOK) == 1);
}

uint8_t power_scap_low(void)
{
    return (uint8_t)(gpio_read(PIN_HARVEST_LOW) == 0);  /* active-low */
}

/* ================================================================
 * Cold-start: wait in a low-power loop until the supercap charges
 * to the minimum operating voltage (3.3 V).
 *
 * During cold-start the ESP32-C3 is awake but idle (modem off, no
 * peripherals). Current draw is ~5 mA — acceptable because the
 * SE1010 harvester provides this even in light rain. Once VOK goes
 * high, we proceed to normal initialization.
 *
 * If the rain stops during cold-start, the supercap will eventually
 * droop below 2.5 V and the SE1010's V_STORE_LOW IRQ will fire. We
 * detect this and go to deep sleep to preserve remaining charge.
 * ================================================================ */
void power_cold_start_wait(void)
{
    g_board.state = STATE_COLDSTART;

    while (1) {
        /* Check if supercap has reached operating voltage */
        if (gpio_read(PIN_HARVEST_VOK) == 1) {
            g_board.state = STATE_HARVESTING;
            return;   /* proceed to normal boot */
        }

        /* Check if supercap is critically low (shouldn't happen during
         * cold-start since we're not consuming much, but safety check) */
        if (gpio_read(PIN_HARVEST_LOW) == 0) {
            /* Too low — go to deep sleep and wait for VOK wake */
            power_deep_sleep(0, WAKE_VOK);
            /* does not return, but if it does, loop again */
        }

        /* Poll every 5 seconds. In a real implementation we'd use a
         * light-sleep with GPIO wake here to save power. */
        delay_ms(COLD_START_POLL_MS);
        g_board.boot_count++;
    }
}