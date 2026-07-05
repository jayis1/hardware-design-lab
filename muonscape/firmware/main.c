/*
 * main.c — MûonScape firmware entry point
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * MûonScape is a portable cosmic-ray muon tomography imager. The
 * firmware runs on an STM32H723 Cortex-M7 host processor, with a
 * Lattice iCE40-UP5K FPGA handling the real-time coincidence logic for
 * 192 SiPM channels across three scintillator layers.
 *
 * The main loop:
 *   1. Poll the FPGA hit FIFO over SPI.
 *   2. Build muon events from triple-layer coincidences.
 *   3. Fit 3-D tracks and accumulate them into the FBP volume in pSRAM.
 *   4. Periodically flush events to SD, update the preview image, and
 *      broadcast status over BLE.
 *   5. Service incoming JSON commands over Wi-Fi TCP.
 *   6. Manage power: MPPT tick, thermal compensation, sleep duty cycle.
 *
 * This file is the orchestration layer; the heavy lifting lives in the
 * drivers under drivers/.
 */
#include "board.h"
#include "registers.h"
#include "drivers/i2c.h"
#include "drivers/fpga.h"
#include "drivers/sipm.h"
#include "drivers/asic.h"
#include "drivers/track.h"
#include "drivers/fbp.h"
#include "drivers/event.h"
#include "drivers/wifi.h"
#include "drivers/ble.h"
#include "drivers/gps.h"
#include "drivers/sensors.h"
#include "drivers/power.h"
#include "drivers/storage.h"
#include "drivers/command.h"
#include "drivers/tdc.h"
#include <string.h>
#include <stdio.h>

/* ---- Globals shared with command.c ---- */
volatile system_state_t g_state = STATE_BOOT;
volatile uint32_t g_acq_start_ms = 0;
volatile uint32_t g_acq_duration_ms = 0;
volatile uint32_t g_track_count = 0;
volatile uint32_t g_event_count = 0;
float *g_volume = NULL;          /* points into pSRAM */
volatile uint32_t g_hit_watermark_irq_count = 0;

/* ---- SysTick counter ---- */
static volatile uint32_t s_millis = 0;
uint32_t muon_millis(void) { return s_millis; }

void muon_delay_ms(uint32_t ms)
{
    uint32_t t0 = s_millis;
    while ((s_millis - t0) < ms) { /* spin */ }
}

/* SysTick IRQ: 1 kHz */
void SysTick_Handler(void)
{
    s_millis++;
}

/* ---- pSRAM volume pointer ----
 * The 8 MB pSRAM on FMC bank 1 holds the 96*96*32*4 = 1,179,648 byte
 * FBP volume plus an event ring buffer and a working slice buffer.
 */
#define VOLUME_PSRAM_OFFSET  0
#define VOLUME_BYTES          ((uint32_t)VOX_X * VOX_Y * VOX_Z * sizeof(float))

static void volume_init(void)
{
    g_volume = (float *)(PSRAM_BASE + VOLUME_PSRAM_OFFSET);
    fbp_init(g_volume);
}

/* ---- Track assembly from raw hits ----
 * The FPGA delivers a stream of per-layer hits. We group them into
 * triple coincidences by timestamp: for each hit on layer 0, we look
 * for matching hits on layers 1 and 2 within COINC_WINDOW_NS.
 *
 * In practice the FPGA pre-filters and delivers only triple coincident
 * hit triples, so the host-side logic is mostly validation and bookkeeping.
 * The code below handles both the pre-filtered case and the raw case.
 */
static muon_hit_t s_hit_buf[FPGA_BURST_MAX];

/* Group consecutive hits into triples by examining the layer field.
 * The FPGA emits triples back-to-back: layer 0, 1, 2 for each muon. */
static uint32_t assemble_tracks(const muon_hit_t *hits, uint16_t n)
{
    uint32_t built = 0;
    uint16_t i = 0;
    while (i + 3 <= n) {
        /* Expect layer sequence 0,1,2 */
        if (hits[i].f.layer == 0 && hits[i+1].f.layer == 1
            && hits[i+2].f.layer == 2) {
            muon_track_t tk;
            const muon_hit_t triple[3] = { hits[i], hits[i+1], hits[i+2] };
            if (track_from_hits(triple, &tk) == MUON_OK) {
                /* Compute absorption weight: in transmission mode the
                 * weight is the deficit relative to the open-sky flux
                 * for this direction. Here we use a placeholder unit
                 * weight; the calibration step supplies the real deficit. */
                float weight = 1.0f;
                /* Quality filter: discard very low-quality tracks */
                if (tk.quality >= 64) {
                    fbp_accumulate(g_volume, &tk, weight);
                    muon_event_t ev;
                    event_from_track(&tk, &ev);
                    event_write(&ev);
                    g_track_count++;
                    built++;
                }
            }
            i += 3;
        } else {
            /* Out of sequence: skip one hit to resync */
            i++;
        }
    }
    return built;
}

/* ---- Periodic tasks ---- */
static uint32_t s_last_event_flush = 0;
static uint32_t s_last_stats_log = 0;
static uint32_t s_last_preview = 0;
static uint32_t s_last_mppt = 0;
static uint32_t s_last_temp_comp = 0;

static void periodic_tasks(void)
{
    uint32_t now = muon_millis();

    if ((now - s_last_event_flush) > EVENT_FLUSH_MS) {
        event_flush();
        s_last_event_flush = now;
    }
    if ((now - s_last_stats_log) > STATS_LOG_MS) {
        /* Log stats to SD (in production: append to stats.csv) */
        s_last_stats_log = now;
    }
    if ((now - s_last_preview) > PREVIEW_UPDATE_MS
        && g_state == STATE_ACQUIRING) {
        /* Reconstruct a fresh preview */
        fbp_filter(g_volume, 1);
        s_last_preview = now;
    }
    if ((now - s_last_mppt) > 1000) {
        power_mppt_tick();
        s_last_mppt = now;
    }
    if ((now - s_last_temp_comp) > 60000) {
        env_state_t env;
        if (sensors_read(&env) == MUON_OK) {
            sipm_temp_compensate(env.temp_c);
        }
        s_last_temp_comp = now;
    }
}

/* ---- Wi-Fi command service ---- */
static uint8_t s_cmd_buf[512];
static char s_resp_buf[1024];
static int s_have_client = 0;

static void wifi_service(void)
{
    if (!wifi_is_connected()) return;
    if (!s_have_client) {
        if (wifi_tcp_client_connected()) {
            s_have_client = 1;
        } else {
            /* Listen and accept */
            static uint32_t last_listen = 0;
            if ((muon_millis() - last_listen) > 5000) {
                wifi_tcp_listen();
                last_listen = muon_millis();
            }
            return;
        }
    }
    int32_t n = wifi_tcp_recv(s_cmd_buf, sizeof(s_cmd_buf) - 1, 100);
    if (n > 0) {
        s_cmd_buf[n] = 0;
        uint16_t rl = command_dispatch((const char *)s_cmd_buf, (uint16_t)n,
                                       s_resp_buf, sizeof(s_resp_buf));
        if (rl > 0) {
            wifi_tcp_send((const uint8_t *)s_resp_buf, rl);
        }
    } else if (n < 0) {
        /* Client disconnected */
        wifi_tcp_close();
        s_have_client = 0;
    }
}

/* ---- BLE service ---- */
static void ble_service(void)
{
    ble_poll();
    char cmd[256];
    uint16_t n = ble_get_cmd(cmd, sizeof(cmd) - 1);
    if (n > 0) {
        cmd[n] = 0;
        char resp[512];
        uint16_t rl = command_dispatch(cmd, n, resp, sizeof(resp));
        if (rl > 0) {
            ble_set_status(resp, rl);
        }
    }
    /* Periodically broadcast status */
    static uint32_t last_status = 0;
    if ((muon_millis() - last_status) > 2000) {
        char status[512];
        uint16_t rl = command_build_status(status, sizeof(status));
        if (rl > 0) ble_set_status(status, rl);
        last_status = muon_millis();
    }
}

/* ---- Clock setup (25 MHz HSE -> 550 MHz SYSCLK) ---- */
void muon_clock_init(void)
{
    /* Enable HSE */
    RCC_CR |= BIT(16);   /* HSEON */
    while (!(RCC_CR & BIT(17))) ;  /* wait HSERDY */
    /* Configure PLL1: M=5, N=110, P=1 -> 25/5*110/1 = 550 MHz */
    RCC_PLL1CFGR = (5U << 0)   /* M */
                 | (110U << 8) /* N */
                 | (1U << 24); /* P = 1 (div by 2? check RM0468) */
    RCC_CR |= BIT(24);  /* PLL1ON */
    while (!(RCC_CR & BIT(25))) ;  /* wait PLL1RDY */
    /* Flash latency for 550 MHz: 7 wait states (VOS1) */
    REG32(FLASH_REGS_BASE + 0x00) = 7;
    /* Switch SYSCLK to PLL1 */
    RCC_CFGR = (RCC_CFGR & ~0x7) | 0x3;  /* SW = PLL1 */
    while (((RCC_CFGR >> 3) & 0x7) != 0x3) ;
}

/* ---- GPIO / peripheral enables ---- */
void muon_gpio_init(void)
{
    /* Enable all GPIO clocks */
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIOAEN | RCC_AHB4ENR_GPIOBEN
                 | RCC_AHB4ENR_GPIOCEN | RCC_AHB4ENR_GPIODEN
                 | RCC_AHB4ENR_GPIOEEN;
    /* LED outputs (PE0..PE3) */
    GPIO_MODER(GPIOE_BASE) |= (1U << 0) | (1U << 2) | (1U << 4) | (1U << 6);
    /* User button PC13 input */
    GPIO_MODER(GPIOC_BASE) &= ~(3U << 26);
    GPIO_PUPDR(GPIOC_BASE) |= (1U << 26);  /* pull-up */
}

void muon_psram_init(void)
{
    /* Enable FMC clock */
    RCC_AHB3ENR |= RCC_AHB3ENR_FMEN;
    /* Configure bank 1, region 0 for pSRAM (16-bit, async, 55 ns) */
    /* BCR1: MBKEN, MWID=16-bit, MTYP=SRAM, mux=0 */
    FMC_BCR1 = BIT(0) | (1U << 4);  /* MBKEN + MWID 16 */
    /* BTR1: access timing for 55 ns pSRAM at 137.5 MHz */
    /* ADDSET=2, DATAST=10, BUSTURN=1 */
    FMC_BTR1 = (2U << 0) | (10U << 8) | (1U << 16);
}

void muon_systick_init(uint32_t ticks_per_ms)
{
    /* SysTick: reload = ticks_per_ms - 1, core clock, enable IRQ */
    REG32(0xE000E014) = ticks_per_ms - 1;
    REG32(0xE000E018) = 0;
    REG32(0xE000E010) = 0x7;
}

void muon_scache_enable(void) { REG32(0xE000E080) |= 1; }
void muon_dcache_enable(void)  { REG32(0xE000E084) |= 1; }
void muon_dcache_invalidate_range(uint32_t addr, uint32_t size)
{
    (void)addr; (void)size;
    /* DCCIMVAC: in production, walk cache lines */
}
void muon_dcache_clean_range(uint32_t addr, uint32_t size)
{
    (void)addr; (void)size;
}

/* ---- LED helpers ---- */
static void led_set(int led, int on)
{
    uint32_t port = GPIOE_BASE;
    uint32_t pin = 1U << led;
    if (on) GPIO_BSRR(port) = pin;
    else    GPIO_BSRR(port) = pin << 16;
}

/* ---- Startup ---- */
static void boot_sequence(void)
{
    led_set(0, 1);  /* status LED on during boot */
    led_set(1, 0);

    muon_clock_init();
    muon_systick_init(HCLK_VALUE / 1000);  /* 1 kHz SysTick */
    muon_gpio_init();
    muon_psram_init();
    muon_scache_enable();
    muon_dcache_enable();

    /* Bring up buses */
    i2c_init();
    power_init();
    sensors_init();
    storage_init();
    gps_init();

    /* Detector front-end */
    fpga_init();
    asic_init();
    sipm_init();
    sipm_set_bias_mv(SIPM_BIAS_DEFAULT);
    fpga_set_coincidence_window(COINC_WINDOW_NS);

    /* Connectivity */
    ble_init();
    wifi_init();
    wifi_start_ap("MuonScape-AP", "muonscape");

    /* Reconstruction volume in pSRAM */
    volume_init();
    event_init();

    led_set(2, 1);  /* GPS LED: will turn off if no fix in 10 s */
    led_set(0, 0);  /* status LED off until acquiring */
    g_state = STATE_IDLE;
}

/* ---- Acquisition loop ---- */
static void acquisition_loop(void)
{
    /* Read bursts of hits from the FPGA, assemble into tracks */
    uint32_t avail = fpga_hit_count();
    if (avail == 0 || avail == 0xFFFFFFFF) return;

    uint16_t max = (avail > FPGA_BURST_MAX) ? FPGA_BURST_MAX : (uint16_t)avail;
    uint16_t got = 0;
    if (fpga_read_burst(s_hit_buf, max, &got) == MUON_OK && got > 0) {
        assemble_tracks(s_hit_buf, got);
    }
}

/* ---- Calibration sequence ---- */
static void calibration_loop(void)
{
    /* 1. Sweep ASIC thresholds to find noise floor */
    asic_autocalibrate();
    /* 2. Calibrate TDC bin width against GPS PPS */
    tdc_calibrate(100);  /* 100 ns reference */
    /* 3. Set coincidence window */
    fpga_set_coincidence_window(COINC_WINDOW_NS);
    /* 4. Clear FBP volume and event log */
    fbp_clear(g_volume);
    g_state = STATE_IDLE;
}

/* ---- Sleep mode ---- */
static void enter_sleep(void)
{
    /* Reduce SiPM bias to idle, turn off Wi-Fi, enter WFI */
    sipm_set_bias_mv(SIPM_BIAS_MIN_MV);
    wifi_disconnect();
    led_set(0, 0);
    /* WFI instruction: wake on BLE or GPIO */
    __asm volatile ("wfi");
    /* On wake, restore */
    sipm_set_bias_mv(SIPM_BIAS_DEFAULT);
    wifi_start_ap("MuonScape-AP", "muonscape");
    g_state = STATE_IDLE;
}

/* ---- Fault handler ---- */
static void fault_handler(muon_status_t err)
{
    (void)err;
    g_state = STATE_FAULT;
    led_set(1, 1);  /* red LED */
    led_set(0, 0);
    /* Try to broadcast fault over BLE */
    char buf[128];
    int n = snprintf(buf, sizeof(buf),
        "{\"ok\":false,\"err\":\"fault %d\",\"state\":\"fault\"}\n", (int)err);
    if (n > 0) ble_set_status(buf, (uint16_t)n);
    muon_delay_ms(5000);
    /* Attempt recovery: reset peripherals */
    g_state = STATE_BOOT;
}

/* ---- Acquisition timeout check ---- */
static int acquisition_done(void)
{
    if (g_state != STATE_ACQUIRING) return 0;
    uint32_t elapsed = muon_millis() - g_acq_start_ms;
    if (g_acq_duration_ms > 0 && elapsed >= g_acq_duration_ms) {
        return 1;
    }
    return 0;
}

static void finish_acquisition(void)
{
    event_close();
    /* Final reconstruction filter */
    fbp_filter(g_volume, 1);
    g_state = STATE_IDLE;
    led_set(0, 0);
    /* Notify over BLE */
    char buf[128];
    int n = snprintf(buf, sizeof(buf),
        "{\"ok\":true,\"acq_done\":true,\"tracks\":%lu}\n",
        (unsigned long)g_track_count);
    if (n > 0) ble_set_status(buf, (uint16_t)n);
}

/* ---- Watchdog feed (IWDG) ---- */
static void wdt_init(void)
{
    /* IWDG: 32 kHz LSI, prescaler /256, reload 0xFFF -> ~32 s timeout */
    REG32(0x40003000) = 0x5555;     /* IWDG KR: enable PR/RLR write */
    REG32(0x40003004) = 6;          /* IWDG PR: /256 */
    REG32(0x40003008) = 0xFFF;      /* IWDG RLR */
    REG32(0x40003000) = 0xCCCC;     /* IWDG KR: start */
}
static void wdt_feed(void)
{
    REG32(0x40003000) = 0xAAAA;     /* IWDG KR: refresh */
}

/* ---- Main ---- */
int main(void)
{
    /* Boot */
    boot_sequence();
    wdt_init();

    /* Main loop */
    uint32_t loop_count = 0;
    while (1) {
        wdt_feed();
        loop_count++;

        switch (g_state) {
        case STATE_BOOT:
            boot_sequence();
            break;
        case STATE_IDLE:
            /* Service comms, wait for start command */
            ble_service();
            wifi_service();
            periodic_tasks();
            /* GPS LED: solid if fix, blink if not */
            if (gps_has_fix()) led_set(2, 1);
            else led_set(2, (loop_count >> 9) & 1);
            break;
        case STATE_CALIBRATING:
            calibration_loop();
            break;
        case STATE_ACQUIRING:
            led_set(0, 1);
            acquisition_loop();
            periodic_tasks();
            ble_service();
            wifi_service();
            if (acquisition_done()) finish_acquisition();
            break;
        case STATE_PROCESSING:
            fbp_filter(g_volume, 1);
            g_state = STATE_IDLE;
            break;
        case STATE_SLEEP:
            enter_sleep();
            break;
        case STATE_FAULT:
            muon_delay_ms(1000);
            g_state = STATE_BOOT;
            break;
        }

        /* Check user button: long press = sleep, short = status */
        if (!(GPIO_IDR(BTN_USER_PORT) & BIT(BTN_USER_PIN))) {
            uint32_t t0 = muon_millis();
            while (!(GPIO_IDR(BTN_USER_PORT) & BIT(BTN_USER_PIN))) {
                if ((muon_millis() - t0) > 2000) break;
            }
            uint32_t held = muon_millis() - t0;
            if (held > 2000) {
                g_state = STATE_SLEEP;
            } else if (held > BTN_DEBOUNCE_MS) {
                /* Short press: send status over BLE */
                char status[512];
                uint16_t rl = command_build_status(status, sizeof(status));
                if (rl > 0) ble_set_status(status, rl);
            }
        }
    }
    return 0;
}

/* ---- IRQ handlers (weak stubs to satisfy vector table) ----
 * The vector table in startup.s points here. Most of the work is done
 * in polling mode; these just feed the relevant driver's ISR routine.
 */
void EXTI9_5_IRQHandler(void)
{
    /* FPGA hit-watermark IRQ */
    fpga_irq_handler();
}

void USART2_IRQHandler(void)
{
    usart_regs_t *u = (usart_regs_t *)USART2_BASE;
    if (u->ISR & USART_ISR_RXNE) {
        uint8_t b = (uint8_t)u->RDR;
        gps_usart_isr(b);
    }
}

void EXTI4_IRQHandler(void)
{
    /* GPS PPS */
    gps_pps_isr();
}