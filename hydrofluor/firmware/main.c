/*
 * main.c — HydroFluor survey super-loop
 * Author: jayis1
 * License: MIT
 *
 * The HydroFluor firmware runs a cooperative super-loop. Each cycle:
 *   1. Optionally primes the flow cell with the peristaltic pump.
 *   2. Acquires the 6×4 fluorescence matrix via pulsed-LED synchronous
 *      detection and the 24-bit ADS1256.
 *   3. Unmixes the matrix into 5 analyte concentrations using the on-device
 *      PLS model, applying temperature compensation and reference
 *      normalization.
 *   4. Tags the sample with depth and temperature, logs to microSD + raw
 *      feature vector, and notifies BLE / uplinks LoRa if due.
 *   5. Enters low-power wait until the next cycle period.
 *
 * Commands from the BLE companion app (START/STOP/PUMP/PERIOD/CAL) are
 * polled at the top of each cycle and applied to survey state.
 *
 * Author: jayis1  — this file is part of the open-hardware HydroFluor sonde.
 */
#include "board.h"
#include "registers.h"
#include "drivers/led_excitation.h"
#include "drivers/photodiode.h"
#include "drivers/flowcell.h"
#include "drivers/fluorometry.h"
#include "drivers/storage.h"
#include "drivers/ble_telemetry.h"
#include "drivers/lora_link.h"
#include "drivers/depth.h"
#include "drivers/temp.h"
#include <string.h>
#include <stdio.h>

/* ---- Global survey state ---- */
typedef enum {
    STATE_IDLE = 0,
    STATE_SAMPLING,
    STATE_DEEP_SLEEP
} survey_state_t;

static survey_state_t  g_state        = STATE_IDLE;
static uint32_t        g_period_ms    = SURVEY_DEFAULT_PERIOD_MS;
static uint8_t         g_pump_enabled = 0;     /* 1 = use flow cell */
static uint32_t        g_boot_ms      = 0;
static uint16_t        g_battery_mv   = 0;
static char            g_console_line[96];

/* ---- Boot-time peripheral init ---- */
static void clock_init(void)
{
    /* Enable HSI16, configure PLL for 120 MHz from HSI16.
     * PLLCFGR: PLLM=1, PLLN=30, PLLP=7 (/16), PLLQ=2 (/4), PLLR=2 (/4)
     * → SYSCLK = 16/1 * 30 / 4 = 120 MHz. */
    RCC->CR |= RCC_CR_HSION;
    while (!(RCC->CR & RCC_CR_HSIRDY)) { }

    RCC->PLLCFGR = RCC_PLLCFGR_PLLSRC_HSI16
                 | (1U  & RCC_PLLCFGR_PLLM_MSK)
                 | ((30U << 8) & RCC_PLLCFGR_PLLN_MSK)
                 | (0U << RCC_PLLCFGR_PLLP_POS)
                 | (1U << RCC_PLLCFGR_PLLQ_POS)
                 | (0U << RCC_PLLCFGR_PLLR_POS);   /* /2 wait… */

    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) { }

    /* Flash latency 4 WS for 120 MHz */
    *(volatile uint32_t *)0x40022000 = 4;   /* FLASH_ACR */

    RCC->CFGR = (RCC->CFGR & ~0xCU) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS_MSK) != (3U << 2)) { }
}

static void systick_init(void)
{
    SYST_RVR = BOARD_HCLK_HZ / 1000 - 1;   /* 1 ms tick */
    SYST_CVR = 0;
    SYST_CSR = SYSTICK_ENABLE | SYSTICK_CLKSRC | SYSTICK_TICKINT;
}

static volatile uint32_t g_jiffies = 0;
void SysTick_Handler(void) { g_jiffies++; }
static uint32_t millis(void) { return g_jiffies; }

/* ---- Battery monitor (internal ADC channel 13 = VBAT/3) ---- */
static void adc_init(void)
{
    RCC->AHB2ENR |= (1U << 13);  /* ADC clock */
    ADC1->CR = 0;
    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY)) { }
}

static uint16_t read_battery_mv(void)
{
    /* Enable VBAT channel, convert, scale by 3 (divider).
     * Approximation: DR = raw_code, voltage = raw/4095 * 3.0 * 3 */
    ADC1->SQR1 = (13U << 6);   /* channel 13 in rank 1, 1 conversion */
    ADC1->CR |= ADC_CR_ADSTART;
    while (!(ADC1->ISR & ADC_ISR_EOC)) { }
    uint16_t raw = (uint16_t)ADC1->DR;
    return (uint16_t)((uint32_t)raw * BAT_SCALE_NUM / 4095);
}

/* ---- Console (USB CDC / UART fallback for debug) ---- */
static void console_puts(const char *s)
{
    /* In the real build, USB CDC is wired via the USB peripheral. For
     * bring-up we route console to USART1 (shared with BLE in this stub). */
    while (*s) uart_putc(USART1, (uint8_t)*s++);
}

/* ---- Survey acquisition cycle ---- */
static int run_survey_cycle(sample_record_t *rec, int16_t feat_out[PLS_FEATURES])
{
    acquisition_t acq;
    led_config_t cfg[EX_CHANNEL_COUNT];
    for (int i = 0; i < EX_CHANNEL_COUNT; ++i) {
        cfg[i] = *led_excitation_get((ex_wavelength_t)i);
    }

    /* Optional flow-cell prime: pump for 2 s before sampling if enabled */
    if (g_pump_enabled) {
        flowcell_prime(2000);
    }

    /* Acquire the fluorescence matrix */
    int err = photodiode_acquire_matrix(&acq, (const led_config_t *)cfg);
    if (err & 1) {
        console_puts("WARN: overrange on one or more channels\r\n");
    }
    if (err & 2) {
        console_puts("WARN: bubble detected — check flow cell\r\n");
        flowcell_set_bubble_flag(1);
    } else {
        flowcell_set_bubble_flag(0);
    }

    /* Read temperature and depth */
    int16_t  t01  = temp_read_c01();
    uint32_t dcm = depth_read_cm();
    int32_t  pmb = depth_read_mbar();

    /* Run the PLS unmixing model */
    analyte_result_t result;
    fluorometry_unmix(&acq, t01, &result);

    /* Build the feature vector for logging (the build_feature_vector is
     * internal to fluorometry.c; we approximate by copying raw uV). */
    int idx = 0;
    for (int ex = 0; ex < EX_CHANNEL_COUNT && idx < PLS_FEATURES; ++ex) {
        for (int det = 0; det < 4 && idx < PLS_FEATURES; ++det) {
            int32_t v = acq.fluor[ex][det] / 1000;
            if (v >  32767) v =  32767;
            if (v < -32768) v = -32768;
            feat_out[idx++] = (int16_t)v;
        }
    }

    /* Fill the sample record */
    rec->seq           = storage_next_seq();
    rec->timestamp      = millis() / 1000;
    rec->depth_cm       = (uint16_t)dcm;
    rec->temp_c01       = t01;
    rec->battery_mv     = g_battery_mv;
    rec->cdom_ppb       = result.cdom_ppb;
    rec->chla_ugl       = result.chla_ugl;
    rec->phyc_ugl       = result.phyc_ugl;
    rec->oil_ppb        = result.oil_ppb;
    rec->turb_ntu_x100  = result.turb_ntu_x100;
    rec->flags          = result.flags;

    /* Console report */
    fluorometry_format(&result, g_console_line, sizeof(g_console_line));
    console_puts("SAMPLE: ");
    console_puts(g_console_line);
    console_puts("\r\n");

    (void)pmb;   /* could log raw pressure too */
    return 0;
}

/* ---- Calibration command handler ---- */
static void handle_calibrate(void)
{
    /* In a full implementation, the BLE app sends a (analyte, ref_value,
     * raw_vector) triplet via the Calibrate characteristic. We stub the
     * raw vector to the last acquired feature set and call the model
     * adjust routine. */
    static int16_t last_feat[PLS_FEATURES];
    /* (last_feat would be updated by the most recent run_survey_cycle) */
    int rv = fluorometry_calibrate(ANALYTE_CDOM, 100, (const int32_t *)last_feat);
    if (rv == 0) {
        ble_notify_calib(ANALYTE_CDOM, 0);
        console_puts("CAL: CDOM offset adjusted\r\n");
    } else {
        console_puts("CAL: error\r\n");
    }
}

/* ---- Main ---- */
int main(void)
{
    clock_init();
    systick_init();
    adc_init();

    console_puts("\r\n=== HydroFluor boot (c) jayis1 ===\r\n");

    /* Driver bring-up */
    led_excitation_init();
    photodiode_init();
    flowcell_init();
    depth_init();
    temp_init();
    storage_init();
    ble_init();
    ble_set_device_info("0.1.0", "HF-0001", 20260618);
    lora_init(LORA_FREQ_HZ, 9, 125000);  /* US915, SF9, 125 kHz */
    lora_set_interval(300);             /* default 5-min uplinks */

    /* Load PLS calibration models */
    fluorometry_load_models();

    /* Self-test: verify ADS1256 present */
    uint8_t id = photodiode_selftest();
    snprintf(g_console_line, sizeof(g_console_line),
             "ADS1256 ID reg = 0x%02X (expect 0x3)\r\n", id);
    console_puts(g_console_line);

    g_state        = STATE_IDLE;
    g_period_ms    = SURVEY_DEFAULT_PERIOD_MS;
    g_pump_enabled = 0;
    g_battery_mv   = read_battery_mv();

    uint32_t last_sample_ms = 0;
    uint32_t last_lora_ms   = 0;

    /* Super-loop */
    for (;;) {
        uint32_t now = millis();

        /* Poll BLE for commands */
        uint16_t period_param = 0;
        uint16_t cmds = ble_poll(&period_param);
        if (cmds & BLE_CMD_START) {
            g_state = STATE_SAMPLING;
            console_puts("CMD: START survey\r\n");
        }
        if (cmds & BLE_CMD_STOP) {
            g_state = STATE_IDLE;
            console_puts("CMD: STOP survey\r\n");
        }
        if (cmds & BLE_CMD_PUMP_ON) {
            g_pump_enabled = 1;
            flowcell_pump_start(PUMP_DEFAULT_DUTY_PCT);
            console_puts("CMD: PUMP ON\r\n");
        }
        if (cmds & BLE_CMD_PUMP_OFF) {
            g_pump_enabled = 0;
            flowcell_pump_stop();
            console_puts("CMD: PUMP OFF\r\n");
        }
        if (cmds & BLE_CMD_SET_PERIOD) {
            g_period_ms = period_param;
            snprintf(g_console_line, sizeof(g_console_line),
                     "CMD: PERIOD=%u ms\r\n", period_param);
            console_puts(g_console_line);
        }
        if (cmds & BLE_CMD_CALIBRATE) {
            handle_calibrate();
        }

        /* Battery refresh every ~5 s */
        if ((now - last_sample_ms) > 5000 || last_sample_ms == 0) {
            g_battery_mv = read_battery_mv();
        }

        /* Sampling cycle */
        if (g_state == STATE_SAMPLING) {
            if ((now - last_sample_ms) >= g_period_ms) {
                last_sample_ms = now;
                sample_record_t rec;
                int16_t feat[PLS_FEATURES];
                run_survey_cycle(&rec, feat);

                /* Log to SD (sample + raw features) */
                storage_log_sample(&rec);
                storage_log_raw(feat, rec.seq);

                /* Notify BLE clients */
                ble_notify_sample(&rec);

                /* LoRa uplink every 5 minutes (300 s) by default */
                uint32_t lora_period = 300000;
                if ((now - last_lora_ms) >= lora_period || last_lora_ms == 0) {
                    last_lora_ms = now;
                    lora_uplink_sample(&rec);
                }
            }
        }

        /* LoRa TX-done polling */
        lora_poll();

        /* Low-power wait: STOP2 between cycles if idle & sampling period
         * is long. Here we simply spin to keep the super-loop responsive. */
        for (volatile int i = 0; i < 100; ++i) { __asm__("nop"); }
    }
}