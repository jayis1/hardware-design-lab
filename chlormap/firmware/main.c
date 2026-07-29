/*
 * main.c — ChloroMap application core (Cortex-M4F)
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * The main core runs the acquisition FSM, spectral processing, index
 * calculation, GPS geotagging, BLE/USB comms, SD logging, and power FSM.
 *
 * Build: make (uses arm-none-eabi-gcc)
 * Target: STM32L432KCU6
 */

#include "board.h"
#include "registers.h"
#include "drivers/adc.h"
#include "drivers/spectrometer.h"
#include "drivers/indices.h"
#include "drivers/calib.h"
#include "drivers/ble.h"
#include "drivers/gps.h"
#include "drivers/display.h"
#include "drivers/storage.h"
#include "drivers/power.h"
#include "drivers/usb_cdc.h"
#include <string.h>
#include <stdio.h>

/* ---- Firmware metadata ---- */
static const char g_fw_version[] = "ChloroMap v1.0 — Author: jayis1 — GPL-2.0";

/* ---- Global state ---- */
typedef enum {
    STATE_IDLE = 0,
    STATE_WAKE,
    STATE_DARK,
    STATE_WHITE,
    STATE_NIR,
    STATE_PROCESS,
    STATE_LOG,
    STATE_CAL_WHITE_REF,
    STATE_CAL_WHITE_MEAS,
    STATE_ERROR
} sys_state_t;

static volatile sys_state_t g_state = STATE_IDLE;
static volatile bool g_trigger_flag = false;
static volatile bool g_rtc_flag = false;
static volatile uint32_t g_systick_ms = 0;

/* ---- Latest measurement ---- */
typedef struct {
    uint32_t timestamp_ms;
    int32_t  lat_e7;        /* GPS lat × 1e7 */
    int32_t  lon_e7;        /* GPS lon × 1e7 */
    int16_t  spad;          /* SPAD equivalent */
    int16_t  ndvi_x1000;   /* NDVI × 1000 */
    int16_t  nsi_x1000;    /* NSI × 1000 */
    int16_t  lwbi_x1000;   /* LWBI × 1000 */
    int16_t  rededge_x1000; /* red-edge slope × 1000 */
    int16_t  bands_x1000[NUM_BANDS]; /* reflectance × 1000 */
    int16_t  temp_c_x10;   /* temperature × 10 */
    uint16_t batt_mv;
    uint8_t  sats;
    uint8_t  fix_type;
} measurement_t;

static measurement_t g_last_meas;
static uint16_t g_band_mask = 0xFFFF; /* all 16 bands active */

/* ---- Raw spectral buffers ---- */
static int32_t g_dark_frame[ARRAY_ELEMENTS];
static int32_t g_white_frame[ARRAY_ELEMENTS];
static int32_t g_nir_frame[ARRAY_ELEMENTS];
static int16_t g_reflectance[ARRAY_ELEMENTS];  /* × 1000 */

/* ---- SysTick (1 ms) ---- */
void SysTick_Handler(void)
{
    g_systick_ms++;
}

uint32_t millis(void)
{
    return g_systick_ms;
}

void delay_ms(uint32_t ms)
{
    uint32_t start = g_systick_ms;
    while ((g_systick_ms - start) < ms) {
        /* wait */
    }
}

/* ---- Trigger EXTI ---- */
void EXTI8_IRQHandler(void)
{
    /* Clear EXTI8 pending */
    /* EXTI->PR1 = EXTI_PR1_PIF8; */
    g_trigger_flag = true;
}

/* ---- RTC 1 Hz wakeup ---- */
void RTC_IRQHandler(void)
{
    /* Clear RTC alarm flag */
    g_rtc_flag = true;
}

/* ---- Clock initialization ---- */
static void board_init_clocks(void)
{
    /*
     * HSI16 → PLL → 80 MHz SYSCLK
     * PLL: HSI16 / 1 × 20 / 2 = 80 MHz
     * AHB/APB1/APB2 = 80 MHz
     * Enable clocks: SPI1, SPI2, SPI3, I2C1, USART2, USB, ADC, DMA, PWR, EXTI
     */
    /* In real build:
     * RCC->CR |= RCC_CR_HSION; while(!(RCC->CR & RCC_CR_HSIRDY));
     * RCC->PLLCFGR = ...; RCC->CR |= RCC_CR_PLLON; ...
     * RCC->CFGR = RCC_CFGR_SW_PLL; ...
     * RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
     * RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN | ...;
     * RCC->APB1ENR1 |= RCC_APB1ENR1_SPI2EN | RCC_APB1ENR1_I2C1EN | ...;
     * RCC->APB2ENR |= RCC_APB2ENR_SPI1EN | RCC_APB2ENR_USART2EN | ...;
     */
}

/* ---- GPIO initialization ---- */
static void board_init_gpio(void)
{
    /*
     * Configure all pins per board.h:
     * - SPI1: PA5(SCK), PA6(MISO), PA7(MOSI), PA4(CS output), PA1(EXTI falling)
     * - SPI2: PB13(SCK), PB15(MOSI), PB12(CS), PB14(DC), PB2(RST)
     * - SPI3: PB3(SCK), PB4(MISO), PB5(MOSI), PA15(CS)
     * - I2C1: PB6(SCL), PB7(SDA) — open-drain, pull-up
     * - USART2: PA2(TX), PA3(RX)
     * - USB: PA11(DM), PA12(DP) — analog
     * - Trigger: PB8 (input pull-up, EXTI falling)
     * - LED SR: PB9, PB10, PB11 (outputs)
     * - VLED_EN, VANA_EN: outputs, initially low
     * - BATT_SENSE: PA0 analog
     */
}

/* ---- RTC initialization ---- */
static void board_init_rtc(void)
{
    /*
     * LSE (32.768 kHz) → RTC
     * Calendar + 1 Hz wakeup interrupt for periodic tasks
     */
}

/* ---- SysTick init ---- */
static void board_init_systick(void)
{
    /* SysTick_Config(SYSCLK_HZ / 1000); — 1 ms tick */
}

/* ---- LED control (shift register) ---- */
static void led_shift_out(uint8_t bits)
{
    /* Bit-bang 8 bits to SN74LV595A: data → clk → latch */
    for (int i = 0; i < 8; i++) {
        /* Set DATA pin = (bits >> i) & 1 */
        /* Toggle CLK pin */
    }
    /* Pulse LATCH pin */
    (void)bits; /* suppress unused warning in stub */
}

static void led_white_on(void)
{
    led_shift_out(1u << LED_WHITE_BIT);
}

static void led_nir_on(void)
{
    led_shift_out(1u << LED_NIR_BIT);
}

static void leds_off(void)
{
    led_shift_out(0);
}

/* ---- Measurement cycle ---- */
static bool run_measurement_cycle(measurement_t *m)
{
    int32_t raw_spectrum[ARRAY_ELEMENTS];

    /* --- DARK frame --- */
    g_state = STATE_DARK;
    leds_off();
    delay_ms(2); /* let ambient settle */
    if (!adc_acquire_frame(g_dark_frame, DARK_INTEG_MS)) {
        g_state = STATE_ERROR;
        return false;
    }

    /* --- WHITE frame (visible + NIR from white LED) --- */
    g_state = STATE_WHITE;
    led_white_on();
    delay_ms(5); /* LED warmup */
    if (!adc_acquire_frame(g_white_frame, WHITE_INTEG_MS)) {
        led_white_on(); /* will be turned off below */
        leds_off();
        g_state = STATE_ERROR;
        return false;
    }
    leds_off();
    delay_ms(2);

    /* --- NIR frame (940 nm LED) --- */
    g_state = STATE_NIR;
    led_nir_on();
    delay_ms(3);
    if (!adc_acquire_frame(g_nir_frame, NIR_INTEG_MS)) {
        leds_off();
        g_state = STATE_ERROR;
        return false;
    }
    leds_off();

    /* --- PROCESS: spectral pipeline --- */
    g_state = STATE_PROCESS;

    /* Merge white + NIR frames (NIR LED covers 900–1050 nm bands) */
    memcpy(raw_spectrum, g_white_frame, sizeof(int32_t) * ARRAY_ELEMENTS);

    /* For NIR bands (elements > ~96, i.e., λ > 850 nm), use NIR frame */
    for (int i = 96; i < ARRAY_ELEMENTS; i++) {
        raw_spectrum[i] = g_nir_frame[i];
    }

    /* Run spectrometer pipeline: dark-sub, calibrate, bin to 16 bands */
    spectrometer_result_t spec;
    if (!spectrometer_process(raw_spectrum, g_dark_frame, &spec)) {
        g_state = STATE_ERROR;
        return false;
    }

    /* Compute indices */
    indices_t idx;
    indices_compute(&spec, &idx);

    /* Fill measurement struct */
    m->timestamp_ms = millis();
    m->spad = idx.spad;
    m->ndvi_x1000 = idx.ndvi_x1000;
    m->nsi_x1000 = idx.nsi_x1000;
    m->lwbi_x1000 = idx.lwbi_x1000;
    m->rededge_x1000 = idx.rededge_x1000;
    m->temp_c_x10 = idx.temp_c_x10;
    memcpy(m->bands_x1000, spec.bands_x1000, sizeof(int16_t) * NUM_BANDS);

    /* GPS geotag */
    gps_data_t gps;
    if (gps_read(&gps)) {
        m->lat_e7 = gps.lat_e7;
        m->lon_e7 = gps.lon_e7;
        m->sats = gps.sats;
        m->fix_type = gps.fix_type;
    } else {
        m->lat_e7 = 0;
        m->lon_e7 = 0;
        m->sats = 0;
        m->fix_type = GPS_FIX_NONE;
    }

    /* Battery */
    m->batt_mv = power_read_battery_mv();

    return true;
}

/* ---- Calibration cycle (white reference) ---- */
static bool run_white_reference_calibration(void)
{
    int32_t dark[ARRAY_ELEMENTS];
    int32_t white[ARRAY_ELEMENTS];

    display_show_message("Cal: White Ref");
    delay_ms(500);

    /* Dark */
    leds_off();
    delay_ms(5);
    if (!adc_acquire_frame(dark, DARK_INTEG_MS)) return false;

    /* White reference tile under white LED */
    led_white_on();
    delay_ms(5);
    if (!adc_acquire_frame(white, WHITE_INTEG_MS)) {
        leds_off();
        return false;
    }
    leds_off();
    delay_ms(2);

    /* NIR reference */
    int32_t nir_ref[ARRAY_ELEMENTS];
    led_nir_on();
    delay_ms(3);
    if (!adc_acquire_frame(nir_ref, NIR_INTEG_MS)) {
        leds_off();
        return false;
    }
    leds_off();

    /* Merge and store as reference */
    int32_t ref[ARRAY_ELEMENTS];
    memcpy(ref, white, sizeof(int32_t) * ARRAY_ELEMENTS);
    for (int i = 96; i < ARRAY_ELEMENTS; i++) {
        ref[i] = nir_ref[i];
    }

    /* Dark-correct and store */
    for (int i = 0; i < ARRAY_ELEMENTS; i++) {
        ref[i] -= dark[i];
        if (ref[i] < 1) ref[i] = 1; /* avoid div-by-zero */
    }

    calib_store_white_reference(ref);
    display_show_message("Cal: Done!");
    delay_ms(1000);
    return true;
}

/* ---- Log measurement to SD ---- */
static void log_measurement(const measurement_t *m)
{
    char line[256];
    int n = snprintf(line, sizeof(line),
        "%lu,%.7ld,%.7ld,%d,%d,%d,%d,%d",
        (unsigned long)m->timestamp_ms,
        (long)m->lat_e7, (long)m->lon_e7,
        m->spad, m->ndvi_x1000, m->nsi_x1000,
        m->lwbi_x1000, m->rededge_x1000);

    for (int i = 0; i < NUM_BANDS; i++) {
        n += snprintf(line + n, sizeof(line) - n, ",%d", m->bands_x1000[i]);
    }
    n += snprintf(line + n, sizeof(line) - n, ",%d,%u,%u,%u\n",
        m->temp_c_x10, m->batt_mv, m->sats, m->fix_type);

    storage_append_line(line);
}

/* ---- Update display with measurement ---- */
static void display_measurement(const measurement_t *m)
{
    char line1[22];
    char line2[22];
    char line3[22];
    char line4[22];

    snprintf(line1, sizeof(line1), "SPAD: %d", m->spad);
    snprintf(line2, sizeof(line2), "NDVI: %.3f", m->ndvi_x1000 / 1000.0f);
    snprintf(line3, sizeof(line3), "NSI:  %.3f", m->nsi_x1000 / 1000.0f);
    snprintf(line4, sizeof(line4), "LWBI: %.3f", m->lwbi_x1000 / 1000.0f);

    display_clear();
    display_draw_string(0, 0, line1, 1);
    display_draw_string(0, 10, line2, 1);
    display_draw_string(0, 20, line3, 1);
    display_draw_string(0, 30, line4, 1);
    display_draw_string(0, 48, m->fix_type > 0 ? "GPS: FIX" : "GPS: NOFIX", 1);
    display_refresh();
}

/* ---- Send measurement over BLE ---- */
static void ble_send_measurement(const measurement_t *m)
{
    uint8_t pkt[BLE_PKT_LEN];
    memset(pkt, 0, sizeof(pkt));

    pkt[BLE_PKT_MAGIC_OFF] = BLE_PKT_MAGIC;
    pkt[BLE_PKT_VER_OFF] = BLE_PKT_VER;

    /* int16 fields */
    pkt[BLE_PKT_SPAD_OFF]     = (m->spad >> 0) & 0xFF;
    pkt[BLE_PKT_SPAD_OFF+1]   = (m->spad >> 8) & 0xFF;
    pkt[BLE_PKT_NDVI_OFF]     = (m->ndvi_x1000 >> 0) & 0xFF;
    pkt[BLE_PKT_NDVI_OFF+1]   = (m->ndvi_x1000 >> 8) & 0xFF;
    pkt[BLE_PKT_NSI_OFF]      = (m->nsi_x1000 >> 0) & 0xFF;
    pkt[BLE_PKT_NSI_OFF+1]    = (m->nsi_x1000 >> 8) & 0xFF;
    pkt[BLE_PKT_LWBI_OFF]     = (m->lwbi_x1000 >> 0) & 0xFF;
    pkt[BLE_PKT_LWBI_OFF+1]   = (m->lwbi_x1000 >> 8) & 0xFF;
    pkt[BLE_PKT_REDEDGE_OFF]  = (m->rededge_x1000 >> 0) & 0xFF;
    pkt[BLE_PKT_REDEDGE_OFF+1]= (m->rededge_x1000 >> 8) & 0xFF;

    /* GPS (int32) */
    memcpy(&pkt[BLE_PKT_LAT_OFF], &m->lat_e7, 4);
    memcpy(&pkt[BLE_PKT_LON_OFF], &m->lon_e7, 4);

    /* Timestamp */
    memcpy(&pkt[BLE_PKT_TS_OFF], &m->timestamp_ms, 4);

    /* 8 key bands */
    int16_t key_bands[8] = {
        m->bands_x1000[0],  /* 450 */
        m->bands_x1000[4],  /* 531 */
        m->bands_x1000[7],  /* 660 */
        m->bands_x1000[8],  /* 680 */
        m->bands_x1000[9],  /* 700 */
        m->bands_x1000[11], /* 800 */
        m->bands_x1000[13], /* 900 */
        m->bands_x1000[15], /* 970 */
    };
    memcpy(&pkt[BLE_PKT_BAND450_OFF], key_bands, 16);

    /* Battery, temp, sats */
    pkt[BLE_PKT_BATT_OFF] = (m->batt_mv >> 0) & 0xFF;
    pkt[BLE_PKT_BATT_OFF+1] = (m->batt_mv >> 8) & 0xFF;
    pkt[BLE_PKT_TEMP_OFF] = (m->temp_c_x10 >> 0) & 0xFF;
    pkt[BLE_PKT_TEMP_OFF+1] = (m->temp_c_x10 >> 8) & 0xFF;
    pkt[BLE_PKT_SAT_OFF] = m->sats;

    /* CRC-8 */
    uint8_t crc = 0;
    for (int i = 0; i < BLE_PKT_LEN - 2; i++) {
        crc ^= pkt[i];
        for (int b = 0; b < 8; b++) {
            crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : (crc << 1);
        }
    }
    pkt[BLE_PKT_CRC_OFF] = crc;

    ble_send_notification(pkt, BLE_PKT_LEN);
}

/* ---- USB CDC command handler ---- */
static void handle_usb_command(const char *cmd)
{
    if (strncmp(cmd, USB_CMD_MEASURE, 7) == 0) {
        g_trigger_flag = true;
        usb_cdc_send("OK: triggering measurement\r\n");
    } else if (strncmp(cmd, USB_CMD_CAL_WHITE, 9) == 0) {
        if (run_white_reference_calibration()) {
            usb_cdc_send("OK: white reference stored\r\n");
        } else {
            usb_cdc_send("ERROR: calibration failed\r\n");
        }
    } else if (strncmp(cmd, USB_CMD_GET_SPECTRUM, 13) == 0) {
        char buf[64];
        usb_cdc_send("SPECTRUM 128 elements:\r\n");
        for (int i = 0; i < ARRAY_ELEMENTS; i++) {
            snprintf(buf, sizeof(buf), "  el%3d: %d\r\n", i, g_white_frame[i]);
            usb_cdc_send(buf);
        }
    } else if (strncmp(cmd, USB_CMD_GET_STATUS, 11) == 0) {
        char buf[128];
        snprintf(buf, sizeof(buf),
            "STATUS: state=%d batt=%umV sats=%u fix=%u\r\n",
            g_state, power_read_battery_mv(), g_last_meas.sats, g_last_meas.fix_type);
        usb_cdc_send(buf);
    } else if (strncmp(cmd, USB_CMD_SET_BANDS, 10) == 0) {
        uint16_t mask = (uint16_t)strtoul(cmd + 10, NULL, 0);
        g_band_mask = mask;
        usb_cdc_send("OK: band mask set\r\n");
    } else if (strncmp(cmd, USB_CMD_HELP, 4) == 0) {
        usb_cdc_send("ChloroMap commands — Author: jayis1\r\n");
        usb_cdc_send("  MEASURE       — trigger measurement\r\n");
        usb_cdc_send("  CAL WHITE     — white reference calibration\r\n");
        usb_cdc_send("  GET SPECTRUM  — dump raw 128-element spectrum\r\n");
        usb_cdc_send("  GET STATUS    — device status\r\n");
        usb_cdc_send("  SET BANDS <m> — set band mask (hex)\r\n");
        usb_cdc_send("  SET INTTIME <ms> — set integration time\r\n");
    } else {
        usb_cdc_send("ERROR: unknown command. Type HELP.\r\n");
    }
}

/* ---- Idle display ---- */
static void display_idle(void)
{
    display_clear();
    display_draw_string(0, 0, "ChloroMap v1.0", 1);
    display_draw_string(0, 12, "Author: jayis1", 1);
    display_draw_string(0, 28, "Ready — press", 1);
    display_draw_string(0, 38, "trigger to measure", 1);
    uint16_t batt = power_read_battery_mv();
    char buf[22];
    snprintf(buf, sizeof(buf), "BAT: %umV", batt);
    display_draw_string(0, 54, buf, 1);
    display_refresh();
}

/* ---- Main loop ---- */
int main(void)
{
    /* ---- Hardware init ---- */
    board_init_clocks();
    board_init_gpio();
    board_init_systick();
    board_init_rtc();

    /* ---- Driver init ---- */
    adc_init();
    spectrometer_init();
    calib_init();
    gps_init();
    display_init();
    storage_init();
    ble_init();
    usb_cdc_init();
    power_init();

    /* ---- Show splash ---- */
    display_clear();
    display_draw_string(0, 0, "ChloroMap v1.0", 1);
    display_draw_string(0, 12, "Author: jayis1", 2);
    display_draw_string(0, 40, "Initializing...", 1);
    display_refresh();
    delay_ms(1500);

    /* ---- Check calibration ---- */
    if (!calib_is_valid()) {
        display_show_message("No calibration!");
        delay_ms(2000);
        display_show_message("Use CAL WHITE");
        delay_ms(2000);
    }

    /* ---- Enable GPS ---- */
    gps_enable();

    /* ---- Start BLE advertising ---- */
    ble_start_advertising("ChloroMap-001");

    display_idle();

    /* ---- Main super-loop ---- */
    while (1) {
        /* Check trigger */
        if (g_trigger_flag) {
            g_trigger_flag = false;

            /* Wake from STOP2 if needed */
            power_wakeup();

            display_show_message("Measuring...");

            if (run_measurement_cycle(&g_last_meas)) {
                g_state = STATE_LOG;

                /* Log to SD */
                log_measurement(&g_last_meas);

                /* Update display */
                display_measurement(&g_last_meas);

                /* Send over BLE */
                ble_send_measurement(&g_last_meas);

                /* Send over USB if connected */
                if (usb_cdc_is_connected()) {
                    char buf[128];
                    snprintf(buf, sizeof(buf),
                        "MEAS: SPAD=%d NDVI=%.3f NSI=%.3f LWBI=%.3f\r\n",
                        g_last_meas.spad,
                        g_last_meas.ndvi_x1000 / 1000.0f,
                        g_last_meas.nsi_x1000 / 1000.0f,
                        g_last_meas.lwbi_x1000 / 1000.0f);
                    usb_cdc_send(buf);
                }
            } else {
                display_show_message("Measure ERROR!");
                delay_ms(2000);
            }

            g_state = STATE_IDLE;
            display_idle();
        }

        /* Check USB CDC commands */
        if (usb_cdc_has_command()) {
            char cmd[64];
            usb_cdc_get_command(cmd, sizeof(cmd));
            handle_usb_command(cmd);
        }

        /* RTC 1 Hz: periodic tasks */
        if (g_rtc_flag) {
            g_rtc_flag = false;

            /* Update BLE status characteristic */
            ble_status_t status;
            status.batt_mv = power_read_battery_mv();
            status.state = g_state;
            status.sats = g_last_meas.sats;
            status.fix_type = g_last_meas.fix_type;
            ble_send_status(&status);

            /* GPS duty cycle: if idle > 30s, disable GPS to save power */
            if (g_state == STATE_IDLE && (millis() % 60000) < 1000) {
                /* Could disable GPS here in production */
            }
        }

        /* Enter STOP2 when idle (ultra-low-power) */
        if (g_state == STATE_IDLE && !usb_cdc_is_connected()) {
            power_enter_stop2();
            /* Wakes here from EXTI (trigger) or RTC alarm */
            power_wakeup_cleanup();
        }
    }
}

/* ---- Required by libc (minimal stubs) ---- */
void _init(void) {}
void _fini(void) {}