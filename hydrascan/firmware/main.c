/*
 * main.c — HydraScan firmware main orchestrator
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * The main loop is a small state machine driven by the single capacitive
 * button:
 *
 *   SLEEP  ──press──► MEASURE ──done──► SHOW ──4 s──► SLEEP
 *                       │
 *                       └─ failure ─► SHOW("ERROR")
 *
 * Long press (3 s) forces a power-off (into standby); the device is
 * woken again by the same button via the wake-capable capacitive sense
 * (TSC) input.
 */
#include "board.h"
#include "registers.h"
#include "classifier.h"
#include "drivers/optical.h"
#include "drivers/eis.h"
#include "drivers/thermal.h"
#include "drivers/display.h"
#include "drivers/ble.h"
#include "drivers/flash_lib.h"

/* ---- Globals ------------------------------------------------------ */
static library_t        g_lib;        /* in-RAM classifier mirror      */
static float            g_opt[OPTICAL_WAVELENGHS];
static eis_point_t      g_eis[EIS_FREQ_POINTS];
static float            g_temp;
static float            g_raw[FEATURE_DIM_RAW];
static int32_t          g_z[FEATURE_DIM_PCA];
static classify_result_t g_result;

/* ---- SysTick (1 ms) ------------------------------------------------ */
static volatile uint32_t g_ticks;
void SysTick_Handler(void) { ++g_ticks; }
uint32_t board_millis(void) { return g_ticks; }
void board_delay_ms(uint32_t ms) {
    uint32_t t0 = g_ticks;
    while ((g_ticks - t0) < ms) { __asm volatile("wfi"); }
}

/* ---- Clock setup: HSE 16 MHz → PLL1 → 240 MHz HCLK ----------------- */
static void clocks_init(void)
{
    /* Enable HSE (assume 16 MHz external on PD0/PD1) and wait. */
    RCC.CR |= (1u << 8);          /* HSEON                             */
    while (!(RCC.CR & (1u << 9))) { } /* HSERDY                          */
    /* Configure PLL1: M=4, N=120, P=2 → VCO=480 MHz, HCLK=240 MHz. */
    RCC.PLLCKSELR = (4u << 0) | (1u << 4);   /* DIVM1=4, source=HSE     */
    RCC.PLLCFGR   = (0u << 1) | (120u << 8) | (0u << 24);  /* P=2, N=120 */
    RCC.PLL1DIVR  = (120u << 0) | (2u << 9);  /* N=120+1, P=2             */
    RCC.D1CFGR    = (0u << 0) | (4u << 8) | (4u << 16); /* HCLK=240/APB=120 */
    RCC.CR       |= (1u << 24);                /* PLL1ON                  */
    while (!(RCC.CR & (1u << 25))) { }        /* PLL1RDY                  */
    /* Switch system clock to PLL1. */
    RCC.CFGR = 0x3u;                          /* SW = PLL1                */
    while (((RCC.CFGR >> 3) & 0x3u) != 0x3u) { }

    /* SysTick 1 ms from HCLK/8 = 30 MHz → reload = 30000-1. */
    SYSTICK.LOAD = 30000u - 1u;
    SYSTICK.VAL  = 0;
    SYSTICK.CSR  = SYSTICK_CSR_CLKSOURCE | SYSTICK_CSR_TICKINT
                 | SYSTICK_CSR_ENABLE;
}

/* ---- GPIO enable helper ------------------------------------------- */
static void gpio_clocks_on(void)
{
    RCC_REG32(RCC_AHB4ENR_OF) |= RCC_AHB4ENR_GPIOAEN | RCC_AHB4ENR_GPIOBEN
                              | RCC_AHB4ENR_GPIOCEN | RCC_AHB4ENR_GPIODEN
                              | RCC_AHB4ENR_GPIOEEN;
    (void)RCC_REG32(RCC_AHB4ENR_OF);
}

/* ---- Button FSM --------------------------------------------------- */
static const hgpio_t btn = PIN_BUTTON;
typedef enum { EV_NONE, EV_SHORT, EV_LONG } btn_event_t;
static btn_event_t button_read(void)
{
    static uint32_t press_start = 0;
    static uint8_t  pressed = 0;
    uint8_t cur = (btn.port->IDR & (1u << btn.pin)) ? 1 : 0;
    if (cur && !pressed) { pressed = 1; press_start = g_ticks; }
    if (!cur && pressed) {
        pressed = 0;
        uint32_t dur = g_ticks - press_start;
        if (dur >= 3000) return EV_LONG;
        if (dur >= 30)   return EV_SHORT;
    }
    return EV_NONE;
}

/* ---- Measurement orchestrator ------------------------------------- */
static hydra_err_t run_measurement(void)
{
    hydra_err_t e;
    e = thermal_read(&g_temp);    if (e != HYDRA_OK) return e;
    display_text(0, 0, "Optical...");
    e = optical_sweep(g_opt);      if (e != HYDRA_OK) return e;
    display_text(1, 0, "EIS...");
    e = eis_sweep(g_eis);          if (e != HYDRA_OK) return e;
    display_text(2, 0, "Classify...");
    classifier_build_feature(g_raw, g_opt, g_eis, g_temp);
    classifier_project(g_raw, g_z);
    e = classifier_classify(&g_lib, g_z, &g_result);
    return e;
}

/* ---- Logging ------------------------------------------------------ */
static void log_result(void)
{
    /* Append a line to the QSPI FATFS image; the app pulls the log
     * over BLE on connect. Here we just store the latest one in a
     * small ring; a full log would page out to flash. */
    static struct { uint16_t id; float conf; uint8_t ad; float t; } ring[32];
    static uint8_t head = 0;
    ring[head].id  = g_result.class_id;
    ring[head].conf = g_result.confidence;
    ring[head].ad  = g_result.adulterant;
    ring[head].t   = g_temp;
    head = (head + 1) % 32;
}

/* ---- main --------------------------------------------------------- */
int main(void)
{
    clocks_init();
    gpio_clocks_on();

    (void)thermal_init();
    (void)optical_init();
    (void)eis_init();
    (void)display_init();
    (void)ble_init();
    (void)flash_lib_init();

    /* Load the liquid library from QSPI; if empty, we start with zero
     * classes (the app will push calibration data). */
    if (flash_lib_load(&g_lib) != HYDRA_OK) {
        g_lib.n_classes = 0;
    }

    display_text(0, 0, "HydraScan ready");
    display_text(1, 0, "press to scan");

    for (;;) {
        ble_poll();

        btn_event_t ev = button_read();
        if (ev == EV_SHORT) {
            display_clear();
            display_text(0, 0, "Measuring...");
            hydra_err_t e = run_measurement();
            if (e == HYDRA_OK) {
                display_result(g_result.name, g_result.confidence,
                               g_result.adulterant,
                               g_result.adulterant_ratio, g_temp);
                if (ble_is_connected())
                    ble_notify_result(&g_result, g_temp);
                log_result();
            } else {
                display_clear();
                display_text(0, 0, "ERROR");
            }
            board_delay_ms(4000);
            display_clear();
            display_text(0, 0, "HydraScan ready");
        } else if (ev == EV_LONG) {
            display_clear();
            display_text(0, 0, "Power off");
            board_delay_ms(500);
            /* Enter standby (the device wakes on the next button via TSC). */
            REG32(0x58024800u + 0x04u) = (1u << 1); /* PWR.CR1 LPMS = standby */
            __asm volatile("wfi");
            for (;;) { } /* not reached */
        }
        __asm volatile("wfi");   /* sleep until SysTick/button IRQ  */
    }
}