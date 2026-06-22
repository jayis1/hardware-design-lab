/*
 * main.c — LumiCast firmware main application
 * LumiCast — Open-Source Circadian Light & SPD Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * This is the main firmware entry point for LumiCast.  It initializes all
 * hardware peripherals, runs the spectral sampling + circadian photometry
 * pipeline, drives the OLED display, logs data to microSD, and exposes
 * measurements over BLE.
 *
 * The firmware runs a simple super-loop with a state machine for each
 * measurement cycle:
 *
 *   IDLE → SAMPLE_SPECTRUM → SAMPLE_FLICKER → COMPUTE → DISPLAY →
 *     LOG → BLE_NOTIFY → IDLE
 *
 * A long-press of the user button enters calibration mode; a short press
 * cycles the display view between the five available screens.
 */

#include "board.h"
#include "registers.h"
#include "drivers/timer_drv.h"
#include "drivers/i2c_drv.h"
#include "drivers/spectrometer.h"
#include "drivers/circadian.h"
#include "drivers/flicker.h"
#include "drivers/display.h"
#include "drivers/sdlog.h"
#include "drivers/ble.h"
#include <string.h>
#include <stdio.h>

/* ===================================================================== */
/*  System clock configuration                                           */
/* ===================================================================== */

static void clock_init(void)
{
    /* Enable HSE (32 MHz). */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));

    /* Configure PLL: HSE / 2 * 4 = 64 MHz SYSCLK. */
    RCC->PLLCFGR = (2U << RCC_PLLCFGR_PLLM_Pos)
                 | (8U << RCC_PLLCFGR_PLLN_Pos)
                 | (0U << RCC_PLLCFGR_PLLP_Pos)
                 | (2U << RCC_PLLCFGR_PLLR_Pos)
                 | RCC_PLLCFGR_PLLREN;
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    /* Flash latency 3 wait states for 64 MHz. */
    FLASH->ACR = FLASH_ACR_LATENCY_3WS | FLASH_ACR_PRFTEN;

    /* Switch SYSCLK to PLL. */
    RCC->CFGR = RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS_MASK) != (RCC_CFGR_SW_PLL << 2));

    /* Enable peripheral clocks. */
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_CRCEN;
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN
                  | RCC_AHB2ENR_GPIOCEN | RCC_AHB2ENR_GPIOHEN;
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN | RCC_APB1ENR1_RTCAPBEN;
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN | RCC_APB2ENR_TIM16EN | RCC_APB2ENR_TIM17EN;

    /* Enable Cortex-M4 FPU. */
    SCB->CPACR |= (0xFU << 20);
}

static void gpio_init(void)
{
    /* User button PC13 — input with pull-up. */
    GPIOC->MODER &= ~(3U << (13U*2U));
    GPIOC->PUPDR |= (GPIO_PUPD_PU << (13U*2U));

    /* Sensor LED PB1 — output. */
    GPIOB->MODER &= ~(3U << (1U*2U));
    GPIOB->MODER |= (GPIO_MODE_OUTPUT << (1U*2U));

    /* PA4 — CAL indicator LED output. */
    GPIOA->MODER &= ~(3U << (4U*2U));
    GPIOA->MODER |= (GPIO_MODE_OUTPUT << (4U*2U));

    /* PB8 — VBUS detect input. */
    GPIOB->MODER &= ~(3U << (8U*2U));
    GPIOB->PUPDR |= (GPIO_PUPD_PD << (8U*2U));

    /* LPUART1 TX/RX on PA2/PA3 — AF7. */
    GPIOA->MODER &= ~((3U << (2U*2U)) | (3U << (3U*2U)));
    GPIOA->MODER |= (GPIO_MODE_AF << (2U*2U)) | (GPIO_MODE_AF << (3U*2U));
    GPIOA->AFRL &= ~((0xFU << (2U*4U)) | (0xFU << (3U*4U)));
    GPIOA->AFRL |= (7U << (2U*4U)) | (7U << (3U*4U));
}

static void lpuart_init(void)
{
    RCC->APB1ENR2 |= RCC_APB1ENR2_LPUART1EN;
    LPUART1->CR1 = 0;
    /* BRR for LPUART = (256 * PCLK) / baud.  PCLK=64MHz, baud=115200. */
    LPUART1->BRR = (256U * 64000000U) / 115200U;
    LPUART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

static void log_msg(const char *msg)
{
    while (*msg) {
        while (!(LPUART1->ISR & USART_ISR_TXE));
        LPUART1->TDR = (uint8_t)*msg++;
    }
}

static void log_num(const char *label, float val)
{
    char buf[64];
    int len = snprintf(buf, sizeof(buf), "%s: %.2f\r\n", label, val);
    for (int i = 0; i < len; i++) {
        while (!(LPUART1->ISR & USART_ISR_TXE));
        LPUART1->TDR = (uint8_t)buf[i];
    }
}

/* ===================================================================== */
/*  Display views                                                        */
/* ===================================================================== */

typedef enum {
    VIEW_OVERVIEW = 0,
    VIEW_SPECTRUM,
    VIEW_CIRCADIAN,
    VIEW_FLICKER,
    VIEW_COUNT
} display_view_t;

static display_view_t cur_view = VIEW_OVERVIEW;

static void draw_view_overview(const circadian_result_t *c, const flicker_result_t *f)
{
    char buf[32];

    display_clear();
    display_text(0, 0, "LumiCast", 2);
    display_hline(0, 16, 128);

    /* Melanopic EDI (big number). */
    display_text(0, 20, "mEDI", 1);
    snprintf(buf, sizeof(buf), "%.0f", c->melanopic_edi);
    display_text(64, 18, buf, 2);

    /* Lux & CCT. */
    snprintf(buf, sizeof(buf), "%.0f lux", c->illuminance_lux);
    display_text(0, 36, buf, 1);
    snprintf(buf, sizeof(buf), "%.0fK", c->cct_k);
    display_text(0, 46, buf, 1);

    /* CS bar. */
    display_text(70, 36, "CS", 1);
    display_bar(86, 36, 40, 8, c->circadian_stimulus / 0.7f * 100.0f);
    snprintf(buf, sizeof(buf), "%.2f", c->circadian_stimulus);
    display_text(86, 46, buf, 1);

    display_flush();
}

static void draw_view_spectrum(const spectral_sample_t *s)
{
    char buf[32];
    display_clear();
    display_text(0, 0, "Spectrum", 2);
    display_hline(0, 16, 128);

    /* Draw spectrogram of channels F1..F8 + NIR. */
    float vals[9];
    for (int i = 0; i < 9; i++) vals[i] = s->irradiance_uw_cm2[i];
    display_spectrogram(0, 20, vals, 9, 50.0f);

    /* Wavelength labels. */
    display_text(0,   58, "415", 1);
    display_text(30,  58, "515", 1);
    display_text(60,  58, "630", 1);
    display_text(100, 58, "NIR", 1);

    snprintf(buf, sizeof(buf), "G:%d A:%d", s->gain, s->atime);
    display_text(80, 0, buf, 1);

    display_flush();
}

static void draw_view_circadian(const circadian_result_t *c)
{
    char buf[32];
    display_clear();
    display_text(0, 0, "Circadian", 2);
    display_hline(0, 16, 128);

    display_text(0, 20, "mEDI", 1);
    snprintf(buf, sizeof(buf), "%.0f", c->melanopic_edi);
    display_text(50, 20, buf, 1);

    display_text(0, 30, "EML", 1);
    snprintf(buf, sizeof(buf), "%.0f", c->equivalent_melanopic_lux);
    display_text(50, 30, buf, 1);

    display_text(0, 40, "CLA", 1);
    snprintf(buf, sizeof(buf), "%.0f", c->circadian_lightCLA);
    display_text(50, 40, buf, 1);

    display_text(0, 50, "CS", 1);
    snprintf(buf, sizeof(buf), "%.3f", c->circadian_stimulus);
    display_text(50, 50, buf, 1);

    snprintf(buf, sizeof(buf), "CRI:%.0f", c->cri_ra);
    display_text(80, 20, buf, 1);
    snprintf(buf, sizeof(buf), "Duv:%.3f", c->duv);
    display_text(80, 30, buf, 1);

    display_flush();
}

static void draw_view_flicker(const flicker_result_t *f)
{
    char buf[32];
    display_clear();
    display_text(0, 0, "Flicker", 2);
    display_hline(0, 16, 128);

    display_text(0, 20, "% Flk", 1);
    snprintf(buf, sizeof(buf), "%.1f%%", f->percent_flicker);
    display_text(50, 20, buf, 1);

    display_text(0, 30, "Index", 1);
    snprintf(buf, sizeof(buf), "%.3f", f->flicker_index);
    display_text(50, 30, buf, 1);

    display_text(0, 40, "Freq", 1);
    snprintf(buf, sizeof(buf), "%.0fHz", f->fundamental_hz);
    display_text(50, 40, buf, 1);

    display_text(0, 50, "Safety", 1);
    display_bar(50, 51, 70, 7, f->safety_rating);

    display_flush();
}

static void draw_no_sd(void)
{
    display_clear();
    display_text(0, 0, "No SD card", 1);
    display_text(0, 10, "Logging off", 1);
    display_flush();
}

/* ===================================================================== */
/*  Calibration                                                          */
/* ===================================================================== */

typedef struct {
    bool active;
    uint32_t start_ms;
    float dark_offset[AS7343_NUM_CHANNELS];
    float d65_scale[AS7343_NUM_CHANNELS];
} calibration_t;

static calibration_t calib;

static void run_calibration(void)
{
    spectral_sample_t spec;
    int i;

    log_msg("\r\n[LumiCast] Calibration: step 1/2 — dark offset\r\n");
    calib.active = true;
    calib.start_ms = timer_millis();

    display_clear();
    display_text(0, 0, "Calibrating", 2);
    display_text(0, 20, "Step 1: Dark", 1);
    display_text(0, 30, "Cover sensor", 1);
    display_flush();

    timer_delay_ms(3000);

    if (as7343_sample(&spec) != 0) {
        log_msg("[LumiCast] Calibration: dark sample failed\r\n");
        calib.active = false;
        return;
    }

    for (i = 0; i < AS7343_NUM_CHANNELS; i++) {
        calib.dark_offset[i] = spec.irradiance_uw_cm2[i];
    }

    log_msg("[LumiCast] Calibration: step 2/2 — D65 scale\r\n");
    display_clear();
    display_text(0, 0, "Calibrating", 2);
    display_text(0, 20, "Step 2: D65", 1);
    display_text(0, 30, "Expose to D65", 1);
    display_flush();

    timer_delay_ms(3000);

    if (as7343_sample(&spec) != 0) {
        log_msg("[LumiCast] Calibration: D65 sample failed\r\n");
        calib.active = false;
        return;
    }

    /* D65 reference irradiance per channel (normalized). */
    static const float d65_ref[9] = {
        1.85f, 2.21f, 2.48f, 2.59f, 2.69f, 2.62f, 2.45f, 2.15f, 1.40f
    };

    for (i = 0; i < 9; i++) {
        float net = spec.irradiance_uw_cm2[i] - calib.dark_offset[i];
        if (net > 1e-6f) {
            calib.d65_scale[i] = d65_ref[i] / net;
        } else {
            calib.d65_scale[i] = 1.0f;
        }
    }

    calib.active = false;
    log_msg("[LumiCast] Calibration complete\r\n");

    display_clear();
    display_text(0, 0, "Calibrated", 2);
    display_flush();
    timer_delay_ms(1000);
}

/* ===================================================================== */
/*  SD logging                                                           */
/* ===================================================================== */

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint32_t timestamp;
    float melanopic_edi;
    float illuminance;
    float cct;
    float duv;
    float circadian_stimulus;
    float flicker_percent;
    float flicker_freq;
    uint16_t spectrum_raw[9];
    uint8_t gain;
    uint8_t atime;
    uint8_t reserved[3];
} log_record_t;

static bool logging_enabled = false;

static void log_record(const circadian_result_t *c,
                        const spectral_sample_t *s,
                        const flicker_result_t *f)
{
    log_record_t rec;
    memset(&rec, 0, sizeof(rec));

    rec.magic = LOG_MAGIC;
    rec.version = LOG_VERSION;
    rec.timestamp = s->timestamp_ms / 1000U;
    rec.melanopic_edi = c->melanopic_edi;
    rec.illuminance = c->illuminance_lux;
    rec.cct = c->cct_k;
    rec.duv = c->duv;
    rec.circadian_stimulus = c->circadian_stimulus;
    rec.flicker_percent = f->percent_flicker;
    rec.flicker_freq = f->fundamental_hz;
    for (int i = 0; i < 9; i++) rec.spectrum_raw[i] = s->raw[i];
    rec.gain = s->gain;
    rec.atime = (uint8_t)(s->atime & 0xFF);

    if (sdlog_append((const uint8_t *)&rec, sizeof(rec)) != SD_OK) {
        logging_enabled = false;
        log_msg("[LumiCast] SD write failed, logging disabled\r\n");
        draw_no_sd();
        timer_delay_ms(2000);
    }
}

/* ===================================================================== */
/*  Button handling                                                      */
/* ===================================================================== */

typedef struct {
    bool pressed;
    uint32_t press_start;
    bool long_pressed;
} button_t;

static button_t btn;

static void button_update(void)
{
    bool raw = BTN_PRESSED();

    if (raw && !btn.pressed) {
        btn.pressed = true;
        btn.press_start = timer_millis();
        btn.long_pressed = false;
    } else if (raw && btn.pressed && !btn.long_pressed) {
        if (timer_elapsed(btn.press_start, 2000)) {
            btn.long_pressed = true;
            run_calibration();
        }
    } else if (!raw && btn.pressed) {
        if (!btn.long_pressed) {
            /* Short press — cycle view. */
            cur_view = (cur_view + 1) % VIEW_COUNT;
            log_msg("[LumiCast] View: ");
            log_num("view", (float)cur_view);
        }
        btn.pressed = false;
    }
}

/* ===================================================================== */
/*  BLE command handler                                                  */
/* ===================================================================== */

static void ble_cmd_handler(uint8_t cmd, const uint8_t *data, uint8_t len)
{
    switch (cmd) {
        case BLE_CMD_START_LOG:
            if (sdlog_present()) {
                logging_enabled = true;
                log_msg("[LumiCast] Logging started\r\n");
            }
            break;
        case BLE_CMD_STOP_LOG:
            logging_enabled = false;
            sdlog_sync();
            log_msg("[LumiCast] Logging stopped\r\n");
            break;
        case BLE_CMD_CALIBRATE:
            run_calibration();
            break;
        case BLE_CMD_SET_GAIN:
            if (len >= 1) as7343_set_gain(data[0]);
            break;
        case BLE_CMD_SET_ATIME:
            if (len >= 1) as7343_set_atime(data[0]);
            break;
        case BLE_CMD_SET_TIME:
            if (len >= 4) {
                uint32_t t = data[0] | (data[1] << 8) |
                             (data[2] << 16) | (data[3] << 24);
                (void)t;  /* would set RTC here */
            }
            break;
        default:
            break;
    }
}

/* ===================================================================== */
/*  Auto-gain control                                                    */
/* ===================================================================== */

static void auto_adjust_gain(spectral_sample_t *s)
{
    /* Target: keep the clear channel in 20%-80% of ADC full scale.      */
    /* If clear channel saturates, reduce gain.  If too low, increase.   */
    uint16_t clr = s->raw[9];  /* clear channel */

    if (clr > 60000) {
        /* Saturated — reduce gain. */
        uint8_t g = as7343_get_gain();
        if (g > 0) {
            as7343_set_gain(g - 1);
            log_msg("[LumiCast] Auto-gain: reduce\r\n");
        }
    } else if (clr < 1000) {
        /* Too dark — increase gain. */
        uint8_t g = as7343_get_gain();
        if (g < 10) {
            as7343_set_gain(g + 1);
            log_msg("[LumiCast] Auto-gain: increase\r\n");
        }
    }
}

/* ===================================================================== */
/*  Application of calibration to spectral sample                        */
/* ===================================================================== */

static void apply_calibration(spectral_sample_t *s)
{
    if (calib.active) return;
    /* Subtract dark offset and apply D65 scale to each channel. */
    for (int i = 0; i < 9; i++) {
        float v = s->irradiance_uw_cm2[i] - calib.dark_offset[i];
        if (v < 0) v = 0;
        s->irradiance_uw_cm2[i] = v * calib.d65_scale[i];
    }
    /* Recompute total. */
    float total = 0;
    for (int i = 0; i < 9; i++) total += s->irradiance_uw_cm2[i];
    s->total_irradiance = total;
}

/* ===================================================================== */
/*  Main application                                                     */
/* ===================================================================== */

typedef enum {
    STATE_IDLE,
    STATE_SAMPLE_SPECTRUM,
    STATE_SAMPLE_FLICKER,
    STATE_COMPUTE,
    STATE_DISPLAY,
    STATE_LOG,
    STATE_BLE_NOTIFY
} app_state_t;

int main(void)
{
    app_state_t state = STATE_IDLE;
    spectral_sample_t spec;
    circadian_result_t circ;
    flicker_result_t flk;
    uint32_t last_sample_ms = 0;
    uint32_t last_ble_ms = 0;

    /* Hardware initialization. */
    clock_init();
    gpio_init();
    timer_init();
    lpuart_init();

    log_msg("\r\n========================================\r\n");
    log_msg("[LumiCast] Boot — author: jayis1\r\n");
    log_msg("[LumiCast] FW v" FW_VERSION_STR "\r\n");
    log_msg("[LumiCast] Clock: 64 MHz\r\n");
    log_msg("========================================\r\n");

    /* Display boot logo. */
    display_init();
    display_clear();
    display_text(0, 0, "LumiCast", 2);
    display_text(0, 20, "Circadian", 1);
    display_text(0, 30, "Light Analyzer", 1);
    display_text(0, 50, "by jayis1", 1);
    display_flush();

    /* Initialize sensor. */
    display_text(0, 40, "Init...", 1);
    display_flush();

    if (as7343_init() != 0) {
        log_msg("[LumiCast] AS7343 init FAILED\r\n");
        display_clear();
        display_text(0, 0, "Sensor ERR", 2);
        display_flush();
        while (1) { __asm__ volatile ("wfi"); }
    }
    log_msg("[LumiCast] AS7343 OK\r\n");

    /* Initialize flicker photodiode ADC. */
    flicker_init();
    log_msg("[LumiCast] Flicker ADC OK\r\n");

    /* Initialize SD card. */
    if (sdlog_init() == SD_OK) {
        logging_enabled = true;
        log_msg("[LumiCast] SD card OK, logging enabled\r\n");
    } else {
        log_msg("[LumiCast] SD card not detected\r\n");
    }

    /* Initialize BLE. */
    ble_init();
    ble_set_command_callback(ble_cmd_handler);
    ble_start_advertising();
    log_msg("[LumiCast] BLE advertising\r\n");

    /* Initialize calibration defaults (no correction). */
    memset(&calib, 0, sizeof(calib));
    for (int i = 0; i < AS7343_NUM_CHANNELS; i++) {
        calib.dark_offset[i] = 0.0f;
        calib.d65_scale[i] = 1.0f;
    }

    timer_delay_ms(1000);
    log_msg("[LumiCast] Starting main loop\r\n");

    /* Main super-loop. */
    while (1) {
        button_update();
        ble_tick();

        switch (state) {
            case STATE_IDLE:
                if (timer_elapsed(last_sample_ms, SAMPLE_INTERVAL_MS)) {
                    state = STATE_SAMPLE_SPECTRUM;
                    last_sample_ms = timer_millis();
                }
                break;

            case STATE_SAMPLE_SPECTRUM:
                if (as7343_sample(&spec) == 0) {
                    auto_adjust_gain(&spec);
                    apply_calibration(&spec);
                    state = STATE_SAMPLE_FLICKER;
                } else {
                    log_msg("[LumiCast] Spectral sample failed\r\n");
                    state = STATE_IDLE;
                }
                break;

            case STATE_SAMPLE_FLICKER:
                flicker_start_capture();
                /* Wait for buffer to fill (non-blocking). */
                if (flicker_is_ready()) {
                    flicker_get_result(&flk);
                    state = STATE_COMPUTE;
                }
                break;

            case STATE_COMPUTE:
                circadian_compute(&spec, &circ);
                state = STATE_DISPLAY;
                break;

            case STATE_DISPLAY:
                switch (cur_view) {
                    case VIEW_OVERVIEW:
                        draw_view_overview(&circ, &flk);
                        break;
                    case VIEW_SPECTRUM:
                        draw_view_spectrum(&spec);
                        break;
                    case VIEW_CIRCADIAN:
                        draw_view_circadian(&circ);
                        break;
                    case VIEW_FLICKER:
                        draw_view_flicker(&flk);
                        break;
                    default:
                        draw_view_overview(&circ, &flk);
                        break;
                }
                state = STATE_LOG;
                break;

            case STATE_LOG:
                if (logging_enabled) {
                    log_record(&circ, &spec, &flk);
                }
                state = STATE_BLE_NOTIFY;
                break;

            case STATE_BLE_NOTIFY:
                ble_update_measurements(&circ, &spec, &flk);
                if (timer_elapsed(last_ble_ms, 1000)) {
                    ble_notify_subscribers();
                    last_ble_ms = timer_millis();
                }
                state = STATE_IDLE;
                break;

            default:
                state = STATE_IDLE;
                break;
        }

        /* Enter sleep mode if idle to save power. */
        if (state == STATE_IDLE && !btn.pressed) {
            __asm__ volatile ("wfi");
        }
    }
}

/* ===================================================================== */
/*  Interrupt vectors (minimal — most handled by CMSIS)                  */
/* ===================================================================== */

void NMI_Handler(void)        { while (1); }
void HardFault_Handler(void)  { while (1); }
void MemManage_Handler(void)  { while (1); }
void BusFault_Handler(void)   { while (1); }
void UsageFault_Handler(void) { while (1); }

void DMA1_Channel1_IRQHandler(void)
{
    /* ADC DMA transfer complete — handled by flicker driver polling.   */
    if (DMA1_Channel1->CCR & DMA_CCR_TCIE) {
        /* Clear flag by disabling/enabling DMA. */
    }
}

void I2C1_EV_IRQHandler(void)
{
    /* I2C1 event interrupt — not used (blocking driver). */
}

void I2C3_EV_IRQHandler(void)
{
    /* I2C3 event interrupt — not used (blocking driver). */
}

void RTC_WKUP_IRQHandler(void)
{
    /* RTC wake-up — used for low-power sleep. */
}