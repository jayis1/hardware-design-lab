/*
 * main.c — LignoScan Main Firmware Entry Point and State Machine
 *
 * LignoScan — Portable Acoustic Tomography Scanner
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * This firmware implements a portable acoustic tomography scanner that
 * non-destructively detects internal decay in tree trunks using ultrasonic
 * stress-wave time-of-flight measurements and SART reconstruction.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "board.h"
#include "drivers/tdc.h"
#include "drivers/mux.h"
#include "drivers/hv.h"
#include "drivers/afe.h"
#include "drivers/tomography.h"
#include "drivers/ble.h"
#include "drivers/gps.h"
#include "drivers/sdlog.h"
#include "drivers/display.h"
#include "drivers/power.h"

/* ---- System state machine ---- */
typedef enum {
    STATE_BOOT = 0,
    STATE_IDLE,
    STATE_CALIBRATE,
    STATE_ACQUIRE,
    STATE_RECONSTRUCT,
    STATE_TRANSMIT,
    STATE_ERROR,
} system_state_t;

static volatile system_state_t g_state = STATE_BOOT;
static volatile uint32_t g_scan_progress = 0;  /* 0-100 percent */
static volatile uint32_t g_tick_ms = 0;

/* ---- Global scan data ---- */
typedef struct {
    int num_sensors;                        /* Detected sensor count (8-16) */
    float trunk_diameter_cm;                /* User-entered trunk diameter */
    float sensor_pos[MAX_SENSORS];          /* Angular position (radians) of each sensor */
    float tof_matrix[MAX_SENSORS][MAX_SENSORS]; /* ToF in nanoseconds; [tx][rx] */
    float amplitude[MAX_SENSORS][MAX_SENSORS];  /* Signal amplitude in mV */
    signal_quality_t quality[MAX_SENSORS][MAX_SENSORS];
    gps_fix_t gps;
    char tree_id[32];
    char timestamp[32];
} scan_session_t;

static scan_session_t g_session;

/* ---- SysTick interrupt handler (1 ms tick) ---- */
void SysTick_Handler(void) {
    g_tick_ms++;
}

uint32_t millis(void) {
    return g_tick_ms;
}

/* ---- Simple delay functions ---- */
void delay_ms(uint32_t ms) {
    uint32_t start = g_tick_ms;
    while ((g_tick_ms - start) < ms) {
        /* Wait for SysTick */
    }
}

void delay_us(uint32_t us) {
    /* DWT-based microsecond delay for precise timing */
    uint32_t cycles = (us * (SYSCLK_FREQ / 1000000UL));
    uint32_t start = *(volatile uint32_t *)0xE0001004UL; /* DWT_CYCCNT */
    while ((*(volatile uint32_t *)0xE0001004UL - start) < cycles) {
        /* busy wait */
    }
}

/* ---- Clock initialization: HSE → PLL → 280 MHz ---- */
void clock_init(void) {
    /* Enable HSE (8 MHz external crystal) */
    RCC_CR |= RCC_CR_HSEON;
    while (!(RCC_CR & RCC_CR_HSERDY)) { }

    /* Configure PLL1: HSE / 2 * 70 / 1 = 8/2*70 = 280 MHz */
    RCC_PLLCFGR = (2U << RCC_PLLCFGR_PLL1M_SHIFT) |    /* M = 2 */
                  (70U << RCC_PLLCFGR_PLL1N_SHIFT) |   /* N = 70 */
                  (0U << RCC_PLLCFGR_PLL1P_SHIFT) |    /* P = 1 (00) */
                  RCC_PLLCFGR_PLLSRC_HSE;

    RCC_CR |= RCC_CR_PLL1ON;
    while (!(RCC_CR & RCC_CR_PLL1RDY)) { }

    /* Set flash latency for 280 MHz (5 wait states) */
    *(volatile uint32_t *)(0x40022000UL + 0x00) = 5U << 0; /* FLASH_ACR */

    /* Switch SYSCLK to PLL1 */
    RCC_CFGR = 3U << 0;  /* SW = PLL1 */
    while (((RCC_CFGR >> 3) & 3U) != 3U) { }  /* Wait SWS */

    /* Enable DWT cycle counter for delay_us */
    *(volatile uint32_t *)0xE000EDFCUL |= (1U << 24); /* DEMCR.TRCENA */
    *(volatile uint32_t *)0xE0001000UL = 0;           /* DWT_CTRL reset */
    *(volatile uint32_t *)0xE0001000UL |= 1U;         /* DWT_CYCCNT_EN */
    *(volatile uint32_t *)0xE0001004UL = 0;           /* Reset counter */
}

/* ---- SysTick setup for 1 ms tick ---- */
static void systick_init(void) {
    SYSTICK_RVR = (SYSCLK_FREQ / 1000UL) - 1; /* 1 ms at 280 MHz */
    SYSTICK_CVR = 0;
    SYSTICK_CSR = SYSTICK_CSR_ENABLE | SYSTICK_CSR_CLKSRC;
}

/* ---- GPIO initialization for all pins ---- */
void gpio_init_all(void) {
    /* Enable all GPIO port clocks */
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIOAEN | RCC_AHB4ENR_GPIOBEN |
                   RCC_AHB4ENR_GPIOCEN | RCC_AHB4ENR_GPIODEN |
                   RCC_AHB4ENR_GPIOEEN | RCC_AHB4ENR_GPIOHEN;

    /* SPI1 pins: SCK(PA5), MISO(PA6), CS(PA4) */
    GPIOA->MODER = (GPIOA->MODER & ~(0x3U << (5*2))) | (GPIO_MODE_AF << (5*2));
    GPIOA->AFRL = (GPIOA->AFRL & ~(0xFU << (5*4))) | (TDC_SPI_AF << (5*4));
    GPIOA->OSPEEDR |= (GPIO_OSPEED_VHIGH << (5*2));

    GPIOA->MODER = (GPIOA->MODER & ~(0x3U << (6*2))) | (GPIO_MODE_AF << (6*2));
    GPIOA->AFRL = (GPIOA->AFRL & ~(0xFU << (6*4))) | (TDC_SPI_AF << (6*4));

    GPIOA->MODER = (GPIOA->MODER & ~(0x3U << (4*2))) | (GPIO_MODE_OUTPUT << (4*2));
    GPIO_SET(TDC_SPI_CS, TDC_SPI_CS_PIN);  /* CS idle high */

    /* USART1 BLE: TX(PB6), RX(PB7) */
    GPIOB->MODER = (GPIOB->MODER & ~(0x3U << (6*2))) | (GPIO_MODE_AF << (6*2));
    GPIOB->AFRL = (GPIOB->AFRL & ~(0xFU << (6*4))) | (BLE_UART_AF << (6*4));
    GPIOB->MODER = (GPIOB->MODER & ~(0x3U << (7*2))) | (GPIO_MODE_AF << (7*2));
    GPIOB->AFRL = (GPIOB->AFRL & ~(0xFU << (7*4))) | (BLE_UART_AF << (7*4));

    /* HV control pins: PC0-PC4 as output */
    for (int i = 0; i <= 4; i++) {
        GPIOC->MODER = (GPIOC->MODER & ~(0x3U << (i*2))) | (GPIO_MODE_OUTPUT << (i*2));
    }
    GPIO_CLR(HV_EN, HV_EN_PIN);  /* HV off initially */

    /* TX MUX: PD0-PD4 as output */
    for (int i = 0; i <= 4; i++) {
        GPIOD->MODER = (GPIOD->MODER & ~(0x3U << (i*2))) | (GPIO_MODE_OUTPUT << (i*2));
    }
    GPIO_SET(TX_MUX_EN, TX_MUX_EN_PIN);  /* MUX disabled (active low) */

    /* RX MUX: PD5-PD9 as output */
    for (int i = 5; i <= 9; i++) {
        GPIOD->MODER = (GPIOD->MODER & ~(0x3U << (i*2))) | (GPIO_MODE_OUTPUT << (i*2));
    }
    GPIO_SET(RX_MUX_EN, RX_MUX_EN_PIN);  /* MUX disabled */

    /* User buttons: PE0-PE5 as input with pull-up */
    for (int i = 0; i <= 5; i++) {
        GPIOE->MODER &= ~(0x3U << (i*2));  /* Input */
        GPIOE->PUPDR = (GPIOE->PUPDR & ~(0x3U << (i*2))) | (GPIO_PUPD_PU << (i*2));
    }

    /* Status LED: PB0 output, PE6 output */
    GPIOB->MODER = (GPIOB->MODER & ~(0x3U << (0*2))) | (GPIO_MODE_OUTPUT << (0*2));
    GPIOE->MODER = (GPIOE->MODER & ~(0x3U << (6*2))) | (GPIO_MODE_OUTPUT << (6*2));

    /* TDC interrupt pin: PC6 input */
    GPIOC->MODER &= ~(0x3U << (6*2));

    /* VBUS detect: PA9 input */
    GPIOA->MODER &= ~(0x3U << (9*2));

    /* Charger status: PC13, PC14 input */
    GPIOC->MODER &= ~(0x3U << (13*2));
    GPIOC->MODER &= ~(0x3U << (14*2));
}

/* ---- Board-level initialization ---- */
void board_init(void) {
    clock_init();
    systick_init();
    gpio_init_all();

    /* Initialize IWDG watchdog (4 second timeout) */
    IWDG_KR = 0xCCCC;  /* Enable */
    IWDG_PR = 4;       /* Prescaler /128 */
    IWDG_RLR = 0xFFF;  /* Reload ~4s */
    while (!(IWDG_SR & 0x1)) { }  /* Wait for PVU */
}

/* ---- Detect connected sensors via cable ID resistors ---- */
static int detect_sensors(void) {
    /* Read cable ID via ADC on PA3.
     * Each sensor cable has a unique resistor divider producing
     * a voltage from 0V (ID 0) to 3.3V (ID 15).
     * We scan for consecutive IDs starting from 0. */
    int count = 0;
    for (int i = 0; i < MAX_SENSORS; i++) {
        /* In a real implementation, this would mux through the cable
         * ID lines. Here we simulate by reading a single ADC value
         * and dividing by the expected step voltage. */
        /* Simplified: assume all cables present up to detected count */
        uint32_t adc_val = 0;  /* ADC read placeholder */
        if (adc_val > 0) {
            count++;
        }
    }
    /* For now, default to 12 sensors (typical configuration) */
    count = 12;
    if (count < MIN_SENSORS) count = MIN_SENSORS;
    if (count > MAX_SENSORS) count = MAX_SENSORS;
    return count;
}

/* ---- Compute sensor angular positions around trunk ---- */
static void compute_sensor_positions(scan_session_t *s) {
    for (int i = 0; i < s->num_sensors; i++) {
        s->sensor_pos[i] = (2.0f * M_PI * (float)i) / (float)s->num_sensors;
    }
}

/* ---- Calibration phase: measure cable delays and coupling ---- */
static int run_calibration(scan_session_t *s) {
    display_clear();
    display_draw_string(0, 0, "LignoScan - Calibrate", 1);
    display_draw_string(0, 16, "Tap sensor 0...", 1);
    display_refresh();

    /* In calibration mode, we listen on all channels for a manual tap
     * on sensor 0 to measure cable propagation delays. */
    afe_set_vga_gain(40);  /* High gain for tap detection */

    int calibrated = 0;
    uint32_t timeout = millis() + 30000;  /* 30s timeout */

    while (!calibrated && millis() < timeout) {
        /* Check each channel for signal */
        for (int rx = 0; rx < s->num_sensors; rx++) {
            mux_select_rx(rx);
            delay_us(MUX_SETTLE_US);

            float amp = afe_measure_amplitude();
            if (amp > 200.0f) {  /* mV threshold */
                /* Record baseline cable delay for this channel */
                float tof = tdc_measure_cable_delay(rx);
                s->tof_matrix[0][rx] = tof;  /* Store as reference */
                calibrated++;
            }
        }
        /* Service watchdog during calibration */
        IWDG_KR = 0xAAAA;  /* Feed watchdog */
    }

    if (calibrated < s->num_sensors - 1) {
        display_draw_string(0, 32, "CAL: partial!", 1);
        display_refresh();
        delay_ms(2000);
        return -1;
    }

    display_draw_string(0, 32, "CAL: OK", 1);
    display_refresh();
    delay_ms(500);
    return 0;
}

/* ---- Acquisition phase: measure all TX→RX ToF pairs ---- */
static int run_acquisition(scan_session_t *s) {
    display_clear();
    display_draw_string(0, 0, "LignoScan - Scan", 1);

    char buf[24];
    int total_pairs = s->num_sensors * (s->num_sensors - 1);
    int pair_count = 0;

    /* Enable HV supply */
    hv_enable();
    delay_ms(10);  /* HV cap charge time */

    for (int tx = 0; tx < s->num_sensors; tx++) {
        for (int rx = 0; rx < s->num_sensors; rx++) {
            if (tx == rx) {
                s->tof_matrix[tx][rx] = 0.0f;
                s->amplitude[tx][rx] = 0.0f;
                s->quality[tx][rx] = QUALITY_SKIP;
                continue;
            }

            /* Select TX and RX channels */
            mux_select_tx(tx);
            mux_select_rx(rx);
            delay_us(MUX_SETTLE_US);

            /* Auto-adjust VGA gain for this pair */
            afe_auto_gain(rx);

            /* Fire multiple shots and average */
            float tof_sum = 0.0f;
            float amp_sum = 0.0f;
            int valid = 0;

            for (int shot = 0; shot < SHOTS_PER_PAIR; shot++) {
                /* Arm TDC */
                tdc_arm();

                /* Fire HV pulse */
                hv_fire(HV_PULSE_WIDTH_US);

                /* Wait for TDC result */
                float tof;
                int ret = tdc_wait_result(&tof, TDC_TIMEOUT_US);

                if (ret == 0 && tof > 0.0f) {
                    tof_sum += tof;
                    amp_sum += afe_measure_amplitude();
                    valid++;
                }

                /* Allow HV cap to recharge */
                delay_ms(HV_RECHARGE_MS);

                /* Feed watchdog */
                IWDG_KR = 0xAAAA;
            }

            if (valid > 0) {
                s->tof_matrix[tx][rx] = tof_sum / (float)valid;
                s->amplitude[tx][rx] = amp_sum / (float)valid;

                /* Quality assessment */
                if (valid >= SHOTS_PER_PAIR * 3 / 4 && s->amplitude[tx][rx] > 100.0f) {
                    s->quality[tx][rx] = QUALITY_GOOD;
                } else if (valid >= SHOTS_PER_PAIR / 2) {
                    s->quality[tx][rx] = QUALITY_MARGINAL;
                } else {
                    s->quality[tx][rx] = QUALITY_POOR;
                }
            } else {
                s->tof_matrix[tx][rx] = -1.0f;  /* Invalid */
                s->amplitude[tx][rx] = 0.0f;
                s->quality[tx][rx] = QUALITY_NO_SIGNAL;
            }

            pair_count++;
            g_scan_progress = (uint32_t)((float)pair_count / (float)total_pairs * 100.0f);

            /* Update display every 10% */
            if (pair_count % (total_pairs / 10 + 1) == 0) {
                snprintf(buf, sizeof(buf), "Progress: %lu%%", (unsigned long)g_scan_progress);
                display_draw_string(0, 16, buf, 1);
                display_refresh();
                ble_send_status(BLE_STATE_SCANNING, g_scan_progress);
            }
        }
    }

    /* Disable HV supply */
    hv_disable();

    /* Check for poor coupling sensors */
    int poor_count = 0;
    for (int tx = 0; tx < s->num_sensors; tx++) {
        int bad_pairs = 0;
        for (int rx = 0; rx < s->num_sensors; rx++) {
            if (tx != rx && s->quality[tx][rx] == QUALITY_NO_SIGNAL) {
                bad_pairs++;
            }
        }
        if (bad_pairs > s->num_sensors / 2) {
            poor_count++;
            snprintf(buf, sizeof(buf), "Sensor %d: poor coupling", tx);
            display_draw_string(0, 32 + poor_count * 8, buf, 1);
        }
    }

    if (poor_count > 0) {
        display_refresh();
        delay_ms(3000);
    }

    return 0;
}

/* ---- Reconstruction phase: SART tomographic imaging ---- */
static int run_reconstruction(scan_session_t *s) {
    display_clear();
    display_draw_string(0, 0, "LignoScan - Reconstruct", 1);
    display_draw_string(0, 16, "Running SART...", 1);
    display_refresh();

    /* Initialize tomography context */
    tomo_ctx_t ctx;
    tomo_init(&ctx, s->num_sensors, s->sensor_pos, s->trunk_diameter_cm,
              TOMO_RADIAL_CELLS, TOMO_ANGULAR_CELLS);

    /* Load ToF measurements into reconstruction */
    for (int tx = 0; tx < s->num_sensors; tx++) {
        for (int rx = 0; rx < s->num_sensors; rx++) {
            if (tx != rx && s->quality[tx][rx] == QUALITY_GOOD) {
                tomo_add_ray(&ctx, tx, rx, s->tof_matrix[tx][rx]);
            }
        }
    }

    /* Run SART iterations */
    for (int iter = 0; iter < TOMO_ITERATIONS; iter++) {
        tomo_sart_iterate(&ctx);

        /* Feed watchdog periodically */
        if (iter % 10 == 0) {
            IWDG_KR = 0xAAAA;
            char buf[24];
            snprintf(buf, sizeof(buf), "Iter %d/%d", iter + 1, TOMO_ITERATIONS);
            display_draw_string(0, 32, buf, 1);
            display_refresh();
        }
    }

    /* Finalize: compute velocity map and classify decay */
    tomo_finalize(&ctx);

    /* Compute Tomographic Decay Index (TDI) */
    float tdi = tomo_compute_tdi(&ctx, SOUND_WOOD_VMIN, MOD_DECAY_VMIN);
    char buf[32];
    snprintf(buf, sizeof(buf), "TDI: %.1f%%", tdi * 100.0f);
    display_draw_string(0, 48, buf, 1);
    display_refresh();

    /* Store reconstruction results for transmission */
    memcpy(s->tof_matrix, ctx.velocity_map, sizeof(ctx.velocity_map));

    return 0;
}

/* ---- Transmission phase: send results to app and log to SD ---- */
static int run_transmit(scan_session_t *s) {
    display_clear();
    display_draw_string(0, 0, "LignoScan - Transmit", 1);
    display_refresh();

    /* Get GPS fix */
    gps_get_fix(&s->gps);

    /* Format timestamp */
    gps_format_timestamp(&s->gps, s->timestamp, sizeof(s->timestamp));

    /* Send tof matrix via BLE */
    ble_send_tof_matrix((float *)s->tof_matrix, s->num_sensors);

    /* Send tomogram via BLE */
    ble_send_tomogram(s->tof_matrix, (uint8_t *)s->quality,
                      TOMO_RADIAL_CELLS * TOMO_ANGULAR_CELLS);

    /* Send GPS data */
    ble_send_gps(&s->gps);

    /* Log raw data to SD card */
    sdlog_write_scan(s->tree_id, s->timestamp, &s->gps,
                     s->num_sensors, s->trunk_diameter_cm,
                     (float *)s->tof_matrix, (float *)s->amplitude,
                     (int *)s->quality);

    display_draw_string(0, 16, "Done! Saved.", 1);
    display_refresh();
    delay_ms(2000);

    return 0;
}

/* ---- Main application loop ---- */
int main(void) {
    /* Hardware initialization */
    board_init();
    display_init();
    ble_init();
    gps_init();
    sdlog_init();
    power_init();
    tdc_init();
    mux_init();
    hv_init();
    afe_init();

    /* Welcome screen */
    display_clear();
    display_draw_string(0, 0, "LignoScan v1.0", 2);
    display_draw_string(0, 24, "Acoustic Tomography", 1);
    display_draw_string(0, 40, "Author: jayis1", 1);
    display_refresh();
    delay_ms(2000);

    g_state = STATE_IDLE;
    int last_battery_update = 0;

    while (1) {
        IWDG_KR = 0xAAAA;  /* Feed watchdog */

        switch (g_state) {
        case STATE_IDLE: {
            /* Update display with status info */
            display_clear();
            display_draw_string(0, 0, "LignoScan - Ready", 1);

            char buf[24];
            uint8_t batt = power_get_battery_percent();
            snprintf(buf, sizeof(buf), "Batt: %d%%", batt);
            display_draw_string(0, 12, buf, 1);

            int gps_fix = gps_has_fix();
            snprintf(buf, sizeof(buf), "GPS: %s", gps_fix ? "OK" : "No Fix");
            display_draw_string(0, 24, buf, 1);

            g_session.num_sensors = detect_sensors();
            snprintf(buf, sizeof(buf), "Sensors: %d", g_session.num_sensors);
            display_draw_string(0, 36, buf, 1);

            display_draw_string(0, 52, "SCAN -> Start", 1);
            display_refresh();

            /* Update battery every 30 seconds */
            if (millis() - last_battery_update > 30000) {
                power_update_gauge();
                last_battery_update = millis();
            }

            /* Check for SCAN button press */
            if (!GPIO_GET(BTN_SCAN, BTN_SCAN_PIN)) {
                delay_ms(50);  /* Debounce */
                if (!GPIO_GET(BTN_SCAN, BTN_SCAN_PIN)) {
                    g_state = STATE_CALIBRATE;
                    GPIO_SET(LED_SCAN, LED_SCAN_PIN);
                }
            }

            /* Low power: sleep if idle for 5 minutes */
            if (power_idle_time_seconds() > 300) {
                power_enter_sleep();
            }

            delay_ms(100);
            break;
        }

        case STATE_CALIBRATE: {
            compute_sensor_positions(&g_session);
            if (run_calibration(&g_session) == 0) {
                g_state = STATE_ACQUIRE;
            } else {
                GPIO_SET(LED_ERROR, 0);
                g_state = STATE_ERROR;
            }
            break;
        }

        case STATE_ACQUIRE: {
            if (run_acquisition(&g_session) == 0) {
                g_state = STATE_RECONSTRUCT;
            } else {
                g_state = STATE_ERROR;
            }
            break;
        }

        case STATE_RECONSTRUCT: {
            if (run_reconstruction(&g_session) == 0) {
                g_state = STATE_TRANSMIT;
            } else {
                g_state = STATE_ERROR;
            }
            break;
        }

        case STATE_TRANSMIT: {
            run_transmit(&g_session);
            GPIO_CLR(LED_SCAN, LED_SCAN_PIN);
            g_state = STATE_IDLE;
            break;
        }

        case STATE_ERROR: {
            display_clear();
            display_draw_string(0, 0, "LignoScan - ERROR", 1);
            display_draw_string(0, 16, "Check sensors", 1);
            display_draw_string(0, 32, "Press SCAN to retry", 1);
            display_refresh();

            if (!GPIO_GET(BTN_SCAN, BTN_SCAN_PIN)) {
                delay_ms(50);
                if (!GPIO_GET(BTN_SCAN, BTN_SCAN_PIN)) {
                    GPIO_CLR(LED_ERROR, 0);
                    g_state = STATE_IDLE;
                }
            }
            delay_ms(200);
            break;
        }

        default:
            g_state = STATE_IDLE;
            break;
        }
    }

    return 0;
}

/* ---- Inter-IC helper: formatted debug output via UART4 ---- */
int _write(int file, char *ptr, int len) {
    (void)file;
    for (int i = 0; i < len; i++) {
        while (!(DBG_UART->ISR & USART_ISR_TXE)) { }
        DBG_UART->TDR = (uint32_t)ptr[i];
    }
    return len;
}

/* EOF — main.c
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */