/*
 * main.c — RootTrace Main Application
 * RootTrace — Open-Source Electrical Impedance Tomography Root Imaging System
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 *
 * Top-level firmware entry point.  Implements the main state machine
 * that orchestrates EIT acquisition, on-device reconstruction, display,
 * BLE communication, SD logging, and power management.
 *
 * State machine:
 *   BOOT -> MENU -> (SCAN | CALIBRATE | SETTINGS | INFO)
 *   SCAN -> ACQUIRE -> RECONSTRUCT -> DISPLAY -> LOG -> BLE_TX -> ACQUIRE...
 */

#include "board.h"
#include "registers.h"
#include "ad5940.h"
#include "mux.h"
#include "eit_acq.h"
#include "reconstruct.h"
#include "display.h"
#include "sdlog.h"
#include "ble_proto.h"
#include "power.h"
#include "env_sens.h"
#include <string.h>
#include <stdio.h>

/* ---------------------------------------------------------------------
 * System clock configuration
 *  HSE = 25 MHz -> PLL1 -> SYSCLK 480 MHz, HCLK 240 MHz
 * --------------------------------------------------------------------- */

static void system_clock_config(void)
{
    /* Enable HSE and wait for ready */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY)) { }

    /* Configure PLL1: M=5, N=192, P=2, Q=20, R=8
     *  VCO = 25/5 * 192 = 960 MHz
     *  PLL1P = 960/2 = 480 MHz (SYSCLK)
     *  PLL1Q = 960/20 = 48 MHz (USB)
     *  PLL1R = 960/8 = 120 MHz */
    RCC->PLLCKSELR = (5U << 0) | (1U << 8);  /* M=5, source=HSE */
    RCC->PLL1DIVR = ((192U - 1) << 0)   /* N=192 */
                  | ((2U - 1) << 9)     /* P=2 */
                  | ((20U - 1) << 16)   /* Q=20 */
                  | ((8U - 1) << 24);    /* R=8 */
    RCC->PLL1FRACR = 0;
    RCC->PLL1FRACR = 0;  /* no fractional */

    /* Enable PLL1 */
    RCC->CR |= RCC_CR_PLL1ON;
    while (!(RCC->CR & RCC_CR_PLL1RDY)) { }

    /* Set flash latency: 3 wait states for 240 MHz HCLK at VOS1 */
    FLASHCCR1 = 3U << 0;  /* WRHIGHFREQ=0, LATENCY=3 */

    /* Set bus dividers: AHB=/2 (240), APB1=/2 (120), APB2=/2 (120) */
    RCC->D1CFGR = (8U << 0)   /* HPRE = /2 (SYSCLK/2 = 240 MHz HCLK) */
               | (0U << 4)    /* D1PPRE = /1 */
               | (0U << 8);   /* D1CPRE = /1 */
    RCC->D2CFGR = (4U << 0)  /* D1PPRE2 = /2 -> PCLK2 = 120 MHz */
               | (4U << 8);   /* D1PPRE1 = /2 -> PCLK1 = 120 MHz */
    RCC->D3CFGR = (4U << 0);  /* D3PPRE = /2 */

    /* Switch system clock to PLL1P */
    RCC->CFGR = (3U << 0);  /* SW = PLL1 */
    while (((RCC->CFGR >> 3) & 3) != 3) { }  /* wait SWS = PLL1 */

    /* Enable FPU (CP10, CP11 full access) */
    *(volatile uint32_t *)FPU_CPACR |= (0xFU << 20);  /* CP10/CP11 full access */
    __asm volatile ("dsb");
    __asm volatile ("isb");

    /* Enable all needed peripheral clocks */
    RCC->AHB4ENR |= BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4)
                  | BIT(5) | BIT(6) | BIT(7);  /* all GPIO ports */
    RCC->APB1LENR |= BIT(17);  /* USART2 */
    RCC->APB1LENR |= BIT(18);  /* USART3 */
    RCC->APB2ENR |= BIT(14);   /* SPI1 (done in ad5940_init, but enable anyway) */
}

/* ---------------------------------------------------------------------
 * Watchdog initialization
 * --------------------------------------------------------------------- */

static void iwdg_init(void)
{
    IWDG_KR = IWDG_KR_KEY;       /* Start watchdog */
    IWDG_KR = IWDG_KR_UNLOCK;   /* Allow register access */
    IWDG_PR = 4;                 /* Prescaler /256 -> 6.4 kHz */
    IWDG_RLR = 2000;             /* Reload: 2000 / 6400 = ~312 ms */
    IWDG_KR = IWDG_KR_RELOAD;    /* Reload counter */
}

static void iwdg_refresh(void)
{
    IWDG_KR = IWDG_KR_RELOAD;
}

/* ---------------------------------------------------------------------
 * Application state machine
 * --------------------------------------------------------------------- */

typedef enum {
    STATE_BOOT,
    STATE_MENU,
    STATE_SCAN,
    STATE_CALIBRATE,
    STATE_SETTINGS,
    STATE_INFO,
    STATE_SLEEP,
    STATE_ERROR,
} app_state_t;

typedef enum {
    MENU_SCAN,
    MENU_CALIBRATE,
    MENU_SETTINGS,
    MENU_INFO,
    MENU_SLEEP,
    MENU_COUNT,
} menu_item_t;

static app_state_t s_state = STATE_BOOT;
static menu_item_t s_menu_sel = MENU_SCAN;
static eit_frame_t s_frame;
static uint8_t s_scan_freq_index = 0;
static int s_scanning = 0;
static int s_sd_ok = 0;
static power_status_t s_power;
static env_data_t s_env;
static char s_status_line1[32];
static char s_status_line2[32];

/* ---------------------------------------------------------------------
 * Button handling (PC13 — active low)
 * --------------------------------------------------------------------- */

static uint32_t s_last_button_time = 0;
static int s_button_pressed = 0;

static int button_read(void)
{
    /* PC13: active low (pressed = 0) */
    return (PIN_GET(BTN_PORT, BTN_PIN) == 0) ? 1 : 0;
}

static void button_poll(uint32_t tick_ms)
{
    int pressed = button_read();
    if (pressed && !s_button_pressed) {
        /* Button just pressed (debounce: 200ms) */
        if (tick_ms - s_last_button_time > 200) {
            s_button_pressed = 1;
            s_last_button_time = tick_ms;
            /* Cycle menu selection or trigger action */
            switch (s_state) {
            case STATE_MENU:
                s_menu_sel = (menu_item_t)((s_menu_sel + 1) % MENU_COUNT);
                break;
            case STATE_SCAN:
                /* Long press to stop scan — simplified: stop on any press */
                s_scanning = 0;
                break;
            case STATE_SLEEP:
                /* Wake up */
                power_wakeup();
                s_state = STATE_MENU;
                break;
            default:
                s_state = STATE_MENU;
                break;
            }
        }
    } else if (!pressed) {
        s_button_pressed = 0;
    }

    /* Double-press detection for menu action (simplified) */
    if (s_button_pressed && (tick_ms - s_last_button_time < 400)) {
        /* Could implement double-click here */
    }
}

/* ---------------------------------------------------------------------
 * BLE command handlers
 * --------------------------------------------------------------------- */

static int ble_handle_start_scan(const ble_cmd_t *cmd, ble_resp_t *resp)
{
    if (cmd->length >= 1) {
        s_scan_freq_index = cmd->payload[0];
        if (s_scan_freq_index >= EIT_FREQ_COUNT)
            s_scan_freq_index = 0;
    }
    s_scanning = 1;
    s_state = STATE_SCAN;
    resp->status = BLE_RESP_OK;
    resp->length = 0;
    return 0;
}

static int ble_handle_stop_scan(const ble_cmd_t *cmd, ble_resp_t *resp)
{
    (void)cmd;
    s_scanning = 0;
    s_state = STATE_MENU;
    resp->status = BLE_RESP_OK;
    resp->length = 0;
    return 0;
}

static int ble_handle_get_frame(const ble_cmd_t *cmd, ble_resp_t *resp)
{
    (void)cmd;
    /* Send the last captured frame */
    ble_proto_send_frame(&s_frame);
    resp->status = BLE_RESP_OK;
    resp->length = 0;
    return 0;
}

static int ble_handle_get_status(const ble_cmd_t *cmd, ble_resp_t *resp)
{
    (void)cmd;
    resp->status = BLE_RESP_OK;
    resp->length = 16;
    resp->payload[0] = s_state;
    resp->payload[1] = s_scanning;
    resp->payload[2] = s_scan_freq_index;
    resp->payload[3] = s_sd_ok;
    *(uint16_t *)&resp->payload[4] = s_frame.frame_seq;
    *(uint16_t *)&resp->payload[6] = s_frame.status;
    *(uint16_t *)&resp->payload[8] = s_frame.electrode_contact_mask;
    *(uint16_t *)&resp->payload[10] = (uint16_t)s_power.percent;
    *(uint16_t *)&resp->payload[12] = (uint16_t)s_power.voltage_mv;
    resp->payload[14] = s_power.charging;
    resp->payload[15] = s_power.usb_connected;
    return 0;
}

static int ble_handle_get_env(const ble_cmd_t *cmd, ble_resp_t *resp)
{
    (void)cmd;
    resp->status = BLE_RESP_OK;
    resp->length = 20;
    float *f = (float *)resp->payload;
    f[0] = s_env.soil_moisture_pct;
    f[1] = s_env.soil_temp_c;
    f[2] = s_env.ambient_temp_c;
    f[3] = s_env.ambient_rh;
    f[4] = (float)s_env.rtc_unix_time;
    return 0;
}

static int ble_handle_get_batt(const ble_cmd_t *cmd, ble_resp_t *resp)
{
    (void)cmd;
    resp->status = BLE_RESP_OK;
    resp->length = 8;
    *(uint16_t *)&resp->payload[0] = s_power.voltage_mv;
    resp->payload[2] = s_power.percent;
    resp->payload[3] = s_power.temp_c;
    resp->payload[4] = s_power.charging;
    resp->payload[5] = s_power.usb_connected;
    return 0;
}

static int ble_handle_set_freq(const ble_cmd_t *cmd, ble_resp_t *resp)
{
    if (cmd->length >= 1 && cmd->payload[0] < EIT_FREQ_COUNT) {
        s_scan_freq_index = cmd->payload[0];
        resp->status = BLE_RESP_OK;
    } else {
        resp->status = BLE_RESP_ERR;
    }
    resp->length = 0;
    return 0;
}

static int ble_handle_set_current(const ble_cmd_t *cmd, ble_resp_t *resp)
{
    if (cmd->length >= 2) {
        uint16_t current_ua = *(uint16_t *)cmd->payload;
        if (current_ua > 0 && current_ua <= EIT_CURRENT_MAX_UA) {
            /* Update AD5940 current amplitude */
            ad5940_config_t cfg = {
                .freq_hz = EIT_FREQ_0,
                .current_ua = current_ua,
                .pga_gain = 4,
                .dft_len_log2 = 6,
                .adc_rate_div = 8,
            };
            ad5940_configure(&cfg);
            resp->status = BLE_RESP_OK;
        } else {
            resp->status = BLE_RESP_ERR;
        }
    } else {
        resp->status = BLE_RESP_ERR;
    }
    resp->length = 0;
    return 0;
}

static int ble_handle_self_test(const ble_cmd_t *cmd, ble_resp_t *resp)
{
    (void)cmd;
    int result = ad5940_self_test();
    resp->status = (result == 0) ? BLE_RESP_OK : BLE_RESP_ERR_HW;
    resp->length = 1;
    resp->payload[0] = (uint8_t)result;
    return 0;
}

static int ble_handle_enter_dfu(const ble_cmd_t *cmd, ble_resp_t *resp)
{
    (void)cmd;
    resp->status = BLE_RESP_OK;
    resp->length = 0;
    /* Trigger a system reset to enter bootloader */
    SCB_AIRCR = (0x5FAU << 16) | (1U << 2);  /* SYSRESETREQ */
    return 0;
}

static int ble_handle_get_file_list(const ble_cmd_t *cmd, ble_resp_t *resp)
{
    (void)cmd;
    int count = 0;
    if (sdlog_get_file_count(&count) == 0) {
        resp->status = BLE_RESP_OK;
        resp->length = 2;
        *(uint16_t *)&resp->payload[0] = (uint16_t)count;
    } else {
        resp->status = BLE_RESP_ERR_NO_SD;
        resp->length = 0;
    }
    return 0;
}

static int ble_handle_get_calib(const ble_cmd_t *cmd, ble_resp_t *resp)
{
    (void)cmd;
    resp->status = BLE_RESP_OK;
    resp->length = 4;
    /* Return calibration status (simplified) */
    *(uint32_t *)&resp->payload[0] = CALIB_MAGIC;
    return 0;
}

static int ble_handle_erase_logs(const ble_cmd_t *cmd, ble_resp_t *resp)
{
    (void)cmd;
    sdlog_erase_all();
    resp->status = BLE_RESP_OK;
    resp->length = 0;
    return 0;
}

/* ---------------------------------------------------------------------
 * Register all BLE command handlers
 * --------------------------------------------------------------------- */

static void register_ble_handlers(void)
{
    ble_proto_register_handler(BLE_CMD_START_SCAN, ble_handle_start_scan);
    ble_proto_register_handler(BLE_CMD_STOP_SCAN, ble_handle_stop_scan);
    ble_proto_register_handler(BLE_CMD_GET_FRAME, ble_handle_get_frame);
    ble_proto_register_handler(BLE_CMD_GET_STATUS, ble_handle_get_status);
    ble_proto_register_handler(BLE_CMD_GET_ENV, ble_handle_get_env);
    ble_proto_register_handler(BLE_CMD_GET_BATT, ble_handle_get_batt);
    ble_proto_register_handler(BLE_CMD_SET_FREQ, ble_handle_set_freq);
    ble_proto_register_handler(BLE_CMD_SET_CURRENT, ble_handle_set_current);
    ble_proto_register_handler(BLE_CMD_SELF_TEST, ble_handle_self_test);
    ble_proto_register_handler(BLE_CMD_ENTER_DFU, ble_handle_enter_dfu);
    ble_proto_register_handler(BLE_CMD_GET_FILE_LIST, ble_handle_get_file_list);
    ble_proto_register_handler(BLE_CMD_GET_CALIB, ble_handle_get_calib);
    ble_proto_register_handler(BLE_CMD_ERASE_LOGS, ble_handle_erase_logs);
}

/* ---------------------------------------------------------------------
 * Render scan screen with live reconstruction
 * --------------------------------------------------------------------- */

static void render_scan_screen(void)
{
    /* Generate a 64×64 image buffer from the last reconstruction */
    static float image[64 * 64];
    reconstruct_get_image(&s_frame, image, 64, 64);

    display_clear();
    display_draw_text(0, 0, "SCAN", 0x0F);
    snprintf(s_status_line1, sizeof(s_status_line1), "F:%d S:%d",
             s_scan_freq_index, s_frame.frame_seq);
    display_draw_text(0, 8, s_status_line1, 0x0A);

    /* Draw the conductivity map */
    display_draw_conductivity_map(image, 64, 64);

    /* Draw contact mask indicators */
    display_draw_text(0, 56, "E:", 0x08);
    for (int i = 0; i < EIT_NUM_ELECTRODES; i++) {
        uint8_t gray = (s_frame.electrode_contact_mask & (1U << i)) ? 0x0F : 0x01;
        display_set_pixel(12 + i, 57, gray);
        display_set_pixel(12 + i, 58, gray);
    }

    display_flush();
}

/* ---------------------------------------------------------------------
 * Main state machine update
 * --------------------------------------------------------------------- */

static uint32_t get_tick(void)
{
    /* Use SysTick counter (from eit_acq.c) */
    extern volatile uint32_t s_tick_ms;
    return s_tick_ms;
}

static void state_machine_update(void)
{
    uint32_t tick = get_tick();
    button_poll(tick);

    switch (s_state) {
    case STATE_BOOT:
        /* Show boot screen, then transition to menu */
        display_show_status("RootTrace v1.0", "Initializing...");
        power_read_status(&s_power);
        if (s_power.voltage_mv < BATT_EMPTY_MV) {
            display_show_status("Low Battery!", "Charge device");
            for (volatile int i = 0; i < 50000000; i++) { }
        }
        s_state = STATE_MENU;
        break;

    case STATE_MENU:
    {
        const char *items[] = {"Scan", "Calibrate", "Settings", "Info", "Sleep"};
        display_show_menu("RootTrace", items, MENU_COUNT, s_menu_sel);
        /* Check for menu selection (double-press or timeout) */
        if (s_button_pressed && (tick - s_last_button_time > 1500)) {
            /* Long press = select */
            switch (s_menu_sel) {
            case MENU_SCAN:
                s_state = STATE_SCAN;
                s_scanning = 1;
                break;
            case MENU_CALIBRATE:
                s_state = STATE_CALIBRATE;
                break;
            case MENU_SETTINGS:
                s_state = STATE_SETTINGS;
                break;
            case MENU_INFO:
                s_state = STATE_INFO;
                break;
            case MENU_SLEEP:
                s_state = STATE_SLEEP;
                break;
            default: break;
            }
        }
        break;
    }

    case STATE_SCAN:
        power_set_led(0, 1, 0);  /* green = scanning */
        if (s_scanning) {
            /* Capture a frame */
            int result = eit_acq_capture_frame(&s_frame, s_scan_freq_index);
            if (result == 0) {
                render_scan_screen();

                /* Log to SD card */
                if (s_sd_ok) {
                    sdlog_log_frame(&s_frame);
                }

                /* Update environmental data periodically */
                if ((s_frame.frame_seq % 10) == 0) {
                    env_sens_read_all(&s_env);
                    power_read_status(&s_power);
                }
            } else {
                snprintf(s_status_line1, sizeof(s_status_line1), "Error: %d", result);
                display_show_status(s_status_line1, "Check electrodes");
                s_scanning = 0;
            }
        } else {
            s_state = STATE_MENU;
            power_set_led(0, 0, 0);
        }
        break;

    case STATE_CALIBRATE:
        display_show_status("Calibrating...", "Please wait");
        eit_acq_calibrate();
        display_show_status("Calibration", "Complete!");
        for (volatile int i = 0; i < 50000000; i++) { }
        s_state = STATE_MENU;
        break;

    case STATE_SETTINGS:
    {
        char buf1[32], buf2[32];
        snprintf(buf1, sizeof(buf1), "Freq: %d kHz", s_scan_freq_index);
        snprintf(buf2, sizeof(buf2), "Batt: %d%%", s_power.percent);
        display_show_status(buf1, buf2);
        if (s_button_pressed && (tick - s_last_button_time > 1500))
            s_state = STATE_MENU;
        break;
    }

    case STATE_INFO:
    {
        char buf1[32], buf2[32];
        snprintf(buf1, sizeof(buf1), "Soil T: %.1fC", s_env.soil_temp_c);
        snprintf(buf2, sizeof(buf2), "Moist: %.0f%%", s_env.soil_moisture_pct);
        display_show_status(buf1, buf2);
        if (s_button_pressed && (tick - s_last_button_time > 1500)) {
            env_sens_read_all(&s_env);
            s_state = STATE_MENU;
        }
        break;
    }

    case STATE_SLEEP:
        display_show_status("Sleeping", "Press to wake");
        display_flush();
        power_set_led(0, 0, 1);  /* blue = sleeping */
        power_enter_low_power();
        /* On wakeup, return to menu */
        s_state = STATE_MENU;
        power_set_led(0, 1, 0);
        break;

    case STATE_ERROR:
        display_show_status("ERROR", "Reset device");
        power_set_led(1, 0, 0);  /* red = error */
        break;
    }

    /* Process BLE commands */
    ble_proto_process();
}

/* ---------------------------------------------------------------------
 * Interrupt vector table (minimal — placed at start of flash)
 * The actual vector table is in the linker script; we define handlers here.
 * --------------------------------------------------------------------- */

/* Default handler */
void Default_Handler(void) { while (1) { } }

/* Hard fault handler */
void HardFault_Handler(void) { while (1) { } }

/* MemManage fault */
void MemManage_Handler(void) { while (1) { } }

/* BusFault handler */
void BusFault_Handler(void) { while (1) { } }

/* UsageFault handler */
void UsageFault_Handler(void) { while (1) { } }

/* SVCall handler (for RTOS compatibility) */
void SVC_Handler(void) { }

/* PendSV handler */
void PendSV_Handler(void) { }

/* ---------------------------------------------------------------------
 * Main entry point
 * --------------------------------------------------------------------- */

int main(void)
{
    /* Initialize hardware */
    system_clock_config();
    iwdg_init();

    /* Initialize drivers */
    power_init();
    display_init();
    ad5940_init();
    eit_acq_init();
    reconstruct_init();
    env_sens_init();
    ble_proto_init();
    register_ble_handlers();

    /* Initialize SD card (optional — device works without SD) */
    s_sd_ok = (sdlog_init() == 0) ? 1 : 0;

    /* Read initial power and env status */
    power_read_status(&s_power);
    env_sens_read_all(&s_env);

    /* Set initial state */
    s_state = STATE_BOOT;
    power_set_led(0, 1, 0);  /* green = OK */

    /* Main loop */
    uint32_t last_watchdog = 0;
    while (1) {
        state_machine_update();

        /* Refresh watchdog periodically */
        uint32_t tick = get_tick();
        if (tick - last_watchdog > 100) {
            iwdg_refresh();
            last_watchdog = tick;
        }
    }

    return 0;  /* never reached */
}