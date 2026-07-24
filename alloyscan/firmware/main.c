/*
 * main.c — AlloyScan main firmware entry point
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * AlloyScan: A handheld multi-frequency eddy current metal alloy
 * identifier. This file implements the main state machine that
 * orchestrates the measurement pipeline:
 *
 *   Trigger → Excitation → ADC capture → Lock-in demod →
 *   Lift-off compensation → k-NN classification → Display + BLE
 *
 * The main loop runs at ~100 Hz, polling buttons and processing
 * the state machine. Measurements take ~200 ms when triggered.
 */

#include "board.h"
#include "registers.h"
#include "alloy_db.h"
#include "classifier.h"
#include "calibration.h"
#include "lockin.h"
#include "ui.h"

#include "drivers/dac_drv.h"
#include "drivers/adc_drv.h"
#include "drivers/oled_drv.h"
#include "drivers/flash_drv.h"
#include "drivers/ble_drv.h"
#include "drivers/tof_drv.h"
#include "drivers/button.h"
#include "drivers/battery.h"

#include <string.h>

/* ---- Global state ---- */

/* Excitation frequencies (defined here, referenced by board.h extern) */
const uint32_t excitation_freqs[FREQ_COUNT] = {
    FREQ_1K, FREQ_10K, FREQ_100K, FREQ_500K
};

/* Scan log entry structure (stored in flash) */
typedef struct {
    uint32_t timestamp;       /* Unix time (from BLE sync or RTC) */
    int16_t  alloy_index;     /* Index into alloy_database, -1 if uncertain */
    uint8_t  confidence_x100; /* Confidence * 100 (0-100) */
    int16_t  lifoff_mm_x10;   /* Lift-off * 10 (mm * 10) */
} __attribute__((packed)) scan_log_entry_t;

static scan_log_entry_t scan_log[MAX_SCANS_LOGGED];
static uint32_t scan_log_count = 0;
static uint32_t current_scan_log_index = 0;

/* ADC sample buffer */
static uint16_t adc_buffer[ADC_SAMPLE_COUNT];
static int16_t  signed_samples[ADC_SAMPLE_COUNT];

/* Lock-in state */
static lockin_state_t lockin;

/* SysTick counter (ms) */
static volatile uint32_t sys_ticks = 0;

/* ---- Clock initialization ---- */

static void clock_init(void)
{
    /* Enable HSI (16 MHz internal oscillator) */
    RCC_CR |= (1UL << 8);  /* HSION */
    while (!(RCC_CR & (1UL << 10)))  /* Wait for HSIRDY */
        ;

    /* Configure PLL: HSI / 4 * 85 = 16/4*85 = 340 MHz VCO
     * SYSCLK = VCO / 2 = 170 MHz
     * PLLCFGR: PLLM=4, PLLN=85, PLLP=2, PLLQ=4, PLLR=2 */
    RCC_PLLCFGR = (4UL << 4)     /* PLLM = 4 */
                | (85UL << 8)    /* PLLN = 85 */
                | (0UL << 17)    /* PLLP = 2 (00) */
                | (4UL << 21)    /* PLLQ = 4 */
                | (2UL << 25)    /* PLLR = 2 */
                | (1UL << 0);    /* PLLSRC = HSI */

    /* Enable PLL */
    RCC_CR |= (1UL << 24);  /* PLLON */
    while (!(RCC_CR & (1UL << 25)))  /* Wait for PLLRDY */
        ;

    /* Set flash latency for 170 MHz (5 wait states) */
    *(volatile uint32_t *)(0x40022000 + 0x00) = 5;  /* FLASH_ACR */

    /* Switch SYSCLK to PLL */
    RCC_CFGR = (3UL << 0);  /* SW = PLL */
    while (((RCC_CFGR >> 3) & 3) != 3)  /* Wait for SWS = PLL */
        ;

    /* Enable all needed peripheral clocks */
    RCC_AHB1ENR |= RCC_AHB1ENR_DMA1 | RCC_AHB1ENR_CRCEN | RCC_AHB1ENR_FLASHEN;
    RCC_AHB2ENR |= RCC_AHB2ENR_GPIOA | RCC_AHB2ENR_GPIOB | RCC_AHB2ENR_GPIOC;
    RCC_APB1ENR1 |= RCC_APB1ENR1_USART2 | RCC_APB1ENR1_SPI2;
    RCC_APB2ENR |= RCC_APB2ENR_SPI1 | RCC_APB2ENR_SYSCFG;

    /* Enable CORDIC clock */
    RCC_AHB1ENR |= (1UL << 13);  /* CORDICEN bit (approximate) */
}

/* ---- SysTick ---- */

void SysTick_Handler(void)
{
    sys_ticks++;
}

static void systick_init(uint32_t ticks)
{
    SYST_RVR = ticks - 1;
    SYST_CVR = 0;
    SYST_CSR = 0x7;  /* Enable, interrupt, core clock */
}

static uint32_t get_ticks(void)
{
    return sys_ticks;
}

static void delay_ms(uint32_t ms)
{
    uint32_t start = get_ticks();
    while ((get_ticks() - start) < ms)
        ;
}

/* ---- GPIO LED initialization ---- */

static void leds_init(void)
{
    /* Configure PC1 (green), PC2 (yellow), PC3 (red) as outputs */
    GPIOC->MODER &= ~(3UL << (LED_GREEN_PIN * 2));
    GPIOC->MODER |= (GPIO_MODE_OUTPUT << (LED_GREEN_PIN * 2));
    GPIOC->OTYPER &= ~(1UL << LED_GREEN_PIN);

    GPIOC->MODER &= ~(3UL << (LED_YELLOW_PIN * 2));
    GPIOC->MODER |= (GPIO_MODE_OUTPUT << (LED_YELLOW_PIN * 2));
    GPIOC->OTYPER &= ~(1UL << LED_YELLOW_PIN);

    GPIOC->MODER &= ~(3UL << (LED_RED_PIN * 2));
    GPIOC->MODER |= (GPIO_MODE_OUTPUT << (LED_RED_PIN * 2));
    GPIOC->OTYPER &= ~(1UL << LED_RED_PIN);

    /* All LEDs off initially */
    GPIOC->BRR = (1UL << LED_GREEN_PIN) | (1UL << LED_YELLOW_PIN) | (1UL << LED_RED_PIN);
}

/* ---- Buzzer ---- */

static void buzzer_init(void)
{
    GPIOB->MODER &= ~(3UL << (BUZZER_EN_PIN * 2));
    GPIOB->MODER |= (GPIO_MODE_OUTPUT << (BUZZER_EN_PIN * 2));
    GPIOB->BRR = (1UL << BUZZER_EN_PIN);
}

static void buzzer_beep(uint32_t duration_ms)
{
    GPIOB->BSRR = (1UL << BUZZER_EN_PIN);
    delay_ms(duration_ms);
    GPIOB->BRR = (1UL << BUZZER_EN_PIN);
}

static void buzzer_feedback(float confidence)
{
    if (confidence >= CONFIDENCE_HIGH) {
        /* High confidence: two short high beeps */
        buzzer_beep(50);
        delay_ms(50);
        buzzer_beep(50);
    } else if (confidence >= CONFIDENCE_MEDIUM) {
        /* Medium confidence: one medium beep */
        buzzer_beep(100);
    } else {
        /* Low confidence: one long low beep */
        buzzer_beep(200);
    }
}

/* ---- Amplifier enable ---- */

static void amp_init(void)
{
    GPIOB->MODER &= ~(3UL << (AMP_ENABLE_PIN * 2));
    GPIOB->MODER |= (GPIO_MODE_OUTPUT << (AMP_ENABLE_PIN * 2));
    GPIOB->BRR = (1UL << AMP_ENABLE_PIN);  /* Disabled initially */
}

static void amp_enable(bool on)
{
    if (on)
        GPIOB->BSRR = (1UL << AMP_ENABLE_PIN);
    else
        GPIOB->BRR = (1UL << AMP_ENABLE_PIN);
}

/* ---- Measurement pipeline ---- */

static classify_result_t perform_scan(void)
{
    classify_result_t result;
    memset(&result, 0, sizeof(result));
    result.alloy_index = -1;

    /* Enable amplifier */
    amp_enable(true);
    delay_ms(5);

    /* Start excitation */
    dac_drv_start();
    delay_ms(2);  /* Let signal settle */

    /* Measure lift-off (ToF distance sensor) */
    float liftoff_mm = 0.0f;
    uint16_t dist_mm = tof_drv_measure_mm(50);
    if (dist_mm > 0 && dist_mm < 300) {
        liftoff_mm = (float)dist_mm / 10.0f;  /* VL53L0X returns mm */
    }

    /* Initialize lock-in (reset phase accumulators) */
    lockin_init(&lockin, ADC_SAMPLE_RATE_HZ);
    lockin_reset(&lockin);

    /* Acquire ADC samples */
    if (!adc_drv_read_block(adc_buffer, ADC_SAMPLE_COUNT)) {
        amp_enable(false);
        dac_drv_stop();
        result.uncertain = true;
        return result;
    }

    /* Wait for ADC block to complete */
    if (!adc_drv_wait_block(100)) {
        amp_enable(false);
        dac_drv_stop();
        result.uncertain = true;
        return result;
    }

    /* Stop excitation */
    dac_drv_stop();
    amp_enable(false);

    /* Convert ADC samples to signed (centered at 0) */
    for (int i = 0; i < ADC_SAMPLE_COUNT; i++) {
        signed_samples[i] = adc_drv_to_signed(adc_buffer[i], 3300);
    }

    /* Process through digital lock-in amplifier */
    lockin_result_t lr = lockin_process(&lockin, signed_samples, ADC_SAMPLE_COUNT);

    if (!lr.valid) {
        result.uncertain = true;
        return result;
    }

    /* Pack I/Q into 8-dim feature vector */
    float feature[8];
    for (int k = 0; k < FREQ_COUNT; k++) {
        feature[k * 2]     = lr.I[k];
        feature[k * 2 + 1] = lr.Q[k];
    }

    /* Classify */
    result = classifier_classify(feature, liftoff_mm);

    return result;
}

/* ---- Scan logging ---- */

static void log_scan(const classify_result_t *result)
{
    if (scan_log_count >= MAX_SCANS_LOGGED)
        return;  /* Log full */

    scan_log_entry_t entry;
    entry.timestamp = get_ticks() / 1000;  /* Rough timestamp */
    entry.alloy_index = (int16_t)result->alloy_index;
    entry.confidence_x100 = (uint8_t)(result->confidence * 100.0f);
    entry.lifoff_mm_x10 = (int16_t)(result->liftoff_mm * 10.0f);

    scan_log[scan_log_count++] = entry;

    /* In production: also write to external SPI flash at
     * FLASH_SCAN_LOG_ADDR + scan_log_count * sizeof(scan_log_entry_t) */
}

/* ---- BLE scan result sending ---- */

static void send_ble_result(const classify_result_t *result)
{
    if (!ble_drv_is_connected())
        return;

    const char *alloy_name = "UNKNOWN";
    if (result->alloy_index >= 0)
        alloy_name = alloy_database[result->alloy_index].name;

    const char *alts[3] = { NULL, NULL, NULL };
    for (int i = 0; i < result->alt_count && i < 3; i++) {
        if (result->alternatives[i] >= 0)
            alts[i] = alloy_database[result->alternatives[i]].name;
    }

    ble_drv_send_scan_result(alloy_name, result->confidence,
                             alts, result->alt_count, result->liftoff_mm);
}

/* ---- Menu navigation ---- */

static void menu_navigate(void)
{
    if (button_pressed(BUTTON_UP)) {
        if (menu_index > 0)
            menu_index--;
    }
    if (button_pressed(BUTTON_DOWN)) {
        if (menu_index < (int)(6 - 1))  /* 6 menu items */
            menu_index++;
    }
    if (button_pressed(BUTTON_OK)) {
        switch (menu_index) {
            case 0:  /* Scan */
                ui_set_state(UI_STATE_IDLE);
                break;
            case 1:  /* Browse DB */
                /* TODO: show alloy browser */
                ui_set_state(UI_STATE_IDLE);
                break;
            case 2:  /* Calibrate */
                calibration_run_full();
                break;
            case 3:  /* Scan Log */
                ui_set_state(UI_STATE_LOG);
                current_scan_log_index = 0;
                break;
            case 4:  /* Battery */
                {
                    uint8_t pct = battery_percent();
                    if (pct < 20)
                        ui_set_state(UI_STATE_LOW_BATTERY);
                    else
                        ui_set_state(UI_STATE_IDLE);
                }
                break;
            case 5:  /* About */
                ui_show_error("AlloyScan v1.0 jayis1");
                break;
        }
    }
}

/* ---- Main ---- */

int main(void)
{
    /* Disable interrupts during init */
    __asm volatile ("cpsid i");

    /* Initialize clock system to 170 MHz */
    clock_init();

    /* Initialize SysTick for 1ms ticks */
    systick_init(SYSCLK_HZ / 1000);

    /* Initialize GPIO LEDs */
    leds_init();

    /* Initialize buzzer */
    buzzer_init();

    /* Initialize amplifier control */
    amp_init();

    /* Initialize button driver */
    button_init();

    /* Initialize OLED display */
    oled_drv_init();

    /* Show splash screen */
    oled_clear();
    oled_draw_string(20, 8, "AlloyScan", true);
    oled_draw_string(28, 24, "v1.0", false);
    oled_draw_string(16, 40, "by jayis1", false);
    oled_draw_string(8, 56, "Initializing...", false);
    oled_flush();

    /* Initialize DAC (excitation) */
    dac_drv_init();

    /* Initialize ADC (signal acquisition) */
    adc_drv_init();

    /* Initialize ToF sensor (lift-off) */
    if (!tof_drv_init()) {
        ui_show_error("ToF sensor fail");
        delay_ms(2000);
    }

    /* Initialize BLE module */
    ble_drv_init();

    /* Initialize battery monitor */
    battery_init();

    /* Initialize SPI flash */
    flash_drv_init();

    /* Initialize lock-in amplifier DDS table */
    lockin_init_dds_table();

    /* Initialize classifier (loads calibration from flash) */
    classifier_init();

    /* Check calibration status */
    if (!classifier_is_calibrated()) {
        ui_show_error("Uncalibrated! Run cal");
        delay_ms(2000);
        /* Auto-start calibration if not calibrated */
        calibration_run_full();
    }

    /* Enable interrupts */
    __asm volatile ("cpsie i");

    /* Initialize UI to idle state */
    ui_set_state(UI_STATE_IDLE);

    /* Main loop */
    uint32_t last_button_poll = 0;
    uint32_t scan_start_time = 0;
    classify_result_t last_result;

    while (1) {
        uint32_t now = get_ticks();

        /* Poll buttons every 1 ms */
        if (now - last_button_poll >= 1) {
            button_poll();
            last_button_poll = now;
        }

        /* Process BLE commands */
        ble_drv_process();

        /* State machine */
        switch (ui_get_state()) {
            case UI_STATE_IDLE:
                /* Check for trigger press */
                if (button_pressed(BUTTON_TRIGGER)) {
                    ui_set_state(UI_STATE_SCANNING);
                    scan_start_time = now;
                }
                /* Check for OK button (enter menu) */
                if (button_pressed(BUTTON_OK)) {
                    ui_set_state(UI_STATE_MENU);
                    menu_index = 0;
                }
                /* Update idle display periodically */
                if (now % 100 == 0) {
                    ui_show_idle();
                }

                /* Check battery */
                if (battery_is_critical()) {
                    ui_set_state(UI_STATE_LOW_BATTERY);
                }
                break;

            case UI_STATE_SCANNING:
                {
                    /* Update progress animation */
                    uint32_t elapsed = now - scan_start_time;
                    int progress = (int)((elapsed * 100) / SCAN_TIMEOUT_MS);
                    if (progress > 100) progress = 100;
                    ui_show_scanning(progress);

                    /* Perform scan after a brief settling period */
                    if (elapsed >= 50) {
                        /* Run measurement */
                        last_result = perform_scan();

                        /* Log the scan */
                        log_scan(&last_result);

                        /* Send via BLE */
                        send_ble_result(&last_result);

                        /* Audible feedback */
                        buzzer_feedback(last_result.confidence);

                        /* Show result */
                        ui_set_state(UI_STATE_RESULT);
                    }
                }
                break;

            case UI_STATE_RESULT:
                /* Show result on display */
                ui_show_result(&last_result);

                /* Wait for any button to return to idle */
                if (button_pressed(BUTTON_TRIGGER) ||
                    button_pressed(BUTTON_OK) ||
                    button_pressed(BUTTON_UP) ||
                    button_pressed(BUTTON_DOWN)) {
                    /* Clear LEDs */
                    GPIOC->BRR = (1UL << LED_GREEN_PIN)
                               | (1UL << LED_YELLOW_PIN)
                               | (1UL << LED_RED_PIN);
                    ui_set_state(UI_STATE_IDLE);
                }
                break;

            case UI_STATE_MENU:
                menu_navigate();
                if (button_pressed(BUTTON_TRIGGER)) {
                    /* Exit menu */
                    ui_set_state(UI_STATE_IDLE);
                }
                break;

            case UI_STATE_CALIBRATE:
                /* Calibration is handled by calibration_run_full()
                 * which is called from the menu. Nothing to do here. */
                break;

            case UI_STATE_LOG:
                {
                    /* Browse scan log */
                    if (button_pressed(BUTTON_UP) && current_scan_log_index > 0) {
                        current_scan_log_index--;
                    }
                    if (button_pressed(BUTTON_DOWN) &&
                        current_scan_log_index + 1 < scan_log_count) {
                        current_scan_log_index++;
                    }
                    if (button_pressed(BUTTON_OK)) {
                        ui_set_state(UI_STATE_IDLE);
                    }

                    /* Show current log entry */
                    const char *alloy_name = "UNKNOWN";
                    if (current_scan_log_index < scan_log_count) {
                        int idx = scan_log[current_scan_log_index].alloy_index;
                        if (idx >= 0)
                            alloy_name = alloy_database[idx].name;
                    }
                    ui_show_log_entry(current_scan_log_index, alloy_name, 0);
                }
                break;

            case UI_STATE_LOW_BATTERY:
                {
                    uint8_t pct = battery_percent();
                    ui_show_low_battery(pct);

                    /* If battery recovers or user presses button, go idle */
                    if (button_pressed(BUTTON_OK) || !battery_is_critical()) {
                        ui_set_state(UI_STATE_IDLE);
                    }
                }
                break;

            case UI_STATE_ERROR:
                if (button_pressed(BUTTON_OK)) {
                    ui_set_state(UI_STATE_IDLE);
                }
                break;
        }

        /* Brief yield to allow interrupts */
        __asm volatile ("wfe");
    }

    return 0;  /* Never reached */
}

/* ---- HardFault handler ---- */
void HardFault_Handler(void)
{
    while (1) {
        /* Flash all LEDs rapidly to indicate fault */
        GPIOC->BSRR = (1UL << LED_RED_PIN);
        delay_ms(100);
        GPIOC->BRR = (1UL << LED_RED_PIN);
        delay_ms(100);
    }
}

/* ---- Default interrupt handlers (weak aliases) ---- */
void NMI_Handler(void)        { while (1); }
void MemManage_Handler(void)   { while (1); }
void BusFault_Handler(void)   { while (1); }
void UsageFault_Handler(void) { while (1); }
void SVC_Handler(void)        { }
void DebugMon_Handler(void)   { }
void PendSV_Handler(void)     { }