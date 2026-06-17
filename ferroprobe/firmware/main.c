/*
 * main.c — FerroProbe firmware main entry point
 * FerroProbe — Open-Source 3-Axis Fluxgate Magnetometer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * This is the main firmware for the FerroProbe 3-axis fluxgate
 * magnetometer.  It initializes all hardware peripherals, runs the
 * main state machine super-loop, and coordinates data acquisition,
 * processing, logging, and BLE communication.
 *
 * Architecture: cooperative super-loop with interrupt-driven I/O.
 *  - TIM1 generates the 15.625 kHz fluxgate excitation PWM
 *  - ADS1256 ADC samples 4 channels (X, Y, Z, temp) at 30 kSPS
 *  - EXTI15 interrupt fires on ADS1256 DRDY → reads sample → feeds lock-in
 *  - TIM2 triggers ADC conversions at 30 kSPS, synchronized to 2f0
 *  - USART1 receives NMEA sentences from the GPS (interrupt-driven)
 *  - UART4 communicates with the nRF52840 BLE module (interrupt-driven)
 *  - I2C1 reads the BMI270 IMU at 400 Hz (polled in main loop)
 *  - SD card logging via SPI1 (CS-multiplexed with ADS1256)
 *  - SSD1306 OLED status display via I2C1
 *
 * State machine:
 *  BOOT → IDLE → SURVEY (GPS+SD logging) / CALIB / MONITOR / CONFIG → SHUTDOWN
 */

#include "board.h"
#include "registers.h"
#include "drivers/fluxgate.h"
#include "drivers/lockin.h"
#include "drivers/adc_drv.h"
#include "drivers/imu.h"
#include "drivers/gps.h"
#include "drivers/sdlog.h"
#include "drivers/ble.h"
#include "drivers/calib.h"
#include <string.h>

/* ===================================================================== */
/*  System tick counter (updated by SysTick)                              */
/* ===================================================================== */

static volatile uint32_t system_tick_ms = 0;

void SysTick_Handler(void)
{
    system_tick_ms++;
}

static uint32_t get_tick_ms(void)
{
    return system_tick_ms;
}

static void systick_init(void)
{
    /* Configure SysTick for 1 ms interrupts at 480 MHz
     * Reload = 480 MHz / 1000 = 480000 - 1
     */
    SYSTICK_LOAD = 480000U - 1;
    SYSTICK_VAL = 0;
    SYSTICK_CTRL = 0x7;  /* Enable, interrupt, core clock */
}

/* ===================================================================== */
/*  Clock initialization (HSI → PLL1 → 480 MHz SYSCLK)                      */
/* ===================================================================== */

static void clock_init(void)
{
    /* Enable HSI and wait for ready */
    RCC_CR |= RCC_CR_HSION;
    while (!(RCC_CR & RCC_CR_HSIRDY))
        ;

    /* Configure PLL1: source = HSI64, M=8, N=120, P=2, Q=4, R=2
     * ref = 64/8 = 8 MHz, VCO = 8×120 = 960 MHz
     * SYSCLK = 960/2 = 480 MHz, USB/SDMMC = 960/4 = 240 MHz
     */
    RCC_PLLCKSELR = 0;  /* PLL1 source = HSI, CK_REF = HSI/M = 64/8 = 8 MHz */
    RCC_PLLCKSELR |= (0x3U << 0);  /* PLLSRC = HSI */
    RCC_PLLCKSELR |= (BOARD_PLL1_M << 4);  /* M = 8 */

    RCC_PLL1CFGR = 0;
    RCC_PLL1CFGR |= (BOARD_PLL1_N << 0);     /* N = 120 */
    RCC_PLL1CFGR |= (1U << 0);               /* PLL1FRACEN already? No */
    /* P divider = 2 → bits [17:16] = 01 (but on H7 it's P[1:0]) */
    /* Actually: PLL1DIVR register holds the dividers. */
    /* Simplified: use the CMSIS-style configuration. */
    /* Set P = 2 */
    volatile uint32_t *pll1divr = (volatile uint32_t *)(RCC_BASE + 0x030U);
    *pll1divr = 0;
    *pll1divr |= (BOARD_PLL1_P - 1) << 9;   /* P = 2, field [9:0] is R, [17:16] is Q... */
    /* Actually the H7 PLL1DIVR layout: R[6:0], Q[13:8], P[17:16], N[22:16] */
    /* This is getting complex.  Use simplified direct writes. */
    *pll1divr = ((BOARD_PLL1_R - 1) << 0)   /* R = 2-1 = 1 */
              | ((BOARD_PLL1_Q - 1) << 8)   /* Q = 4-1 = 3 */
              | (((BOARD_PLL1_P / 2 - 1)) << 16); /* P = 2 → /2 */

    /* Enable PLL1 */
    RCC_CR |= RCC_CR_PLL1ON;
    while (!(RCC_CR & RCC_CR_PLL1RDY))
        ;

    /* Configure bus prescalers:
     * D1 (SYSCLK → HCLK) = /2 → 240 MHz
     * D2 APB1 = /2 → 120 MHz
     * D2 APB2 = /2 → 120 MHz
     * D3 APB4 = /2 → 120 MHz
     */
    RCC_D1CFGR = 0;
    RCC_D1CFGR |= (0x8U << 0);   /* D1 CPRE = /2 (HCLK = 240 MHz) */
    RCC_D2CFGR = 0;
    RCC_D2CFGR |= (0x4U << 0);   /* D2 PPRE1 = /2 (APB1 = 120 MHz) */
    RCC_D2CFGR |= (0x4U << 8);   /* D2 PPRE2 = /2 (APB2 = 120 MHz) */
    RCC_D3CFGR = 0;
    RCC_D3CFGR |= (0x4U << 0);   /* D3 PPRE4 = /2 (APB4 = 120 MHz) */

    /* Switch SYSCLK to PLL1 */
    volatile uint32_t *cfgr = &RCC_CFGR;
    *cfgr = 0x3U;  /* SW = PLL1 (011) */
    while (((*cfgr >> 3) & 0x7) != 0x3)
        ;
}

/* ===================================================================== */
/*  GPIO initialization for LEDs, buttons, buzzer                         */
/* ===================================================================== */

static void gpio_init(void)
{
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIODEN | RCC_AHB4ENR_GPIOCEN;

    /* LEDs (PD8, PD9, PD10) as outputs */
    volatile uint32_t *pd_moder = (volatile uint32_t *)(GPIOD_BASE + GPIO_MODER);
    volatile uint32_t *pd_pupdr = (volatile uint32_t *)(GPIOD_BASE + GPIO_PUPDR);

    /* LED R/G/B: output push-pull */
    for (uint8_t pin = 8; pin <= 10; pin++) {
        *pd_moder &= ~(0x3U << (pin * 2));
        *pd_moder |= (0x1U << (pin * 2));
    }

    /* Buzzer (PD11): output */
    *pd_moder &= ~(0x3U << (BUZZER_PIN * 2));
    *pd_moder |= (0x1U << (BUZZER_PIN * 2));

    /* Buttons (PD12, PD13): input with pull-up */
    for (uint8_t pin = BTN_MODE_PIN; pin <= BTN_START_PIN; pin++) {
        *pd_moder &= ~(0x3U << (pin * 2));   /* Input */
        *pd_pupdr |= (0x1U << (pin * 2));    /* Pull-up */
    }

    /* USB detect (PC13): input */
    volatile uint32_t *pc_moder = (volatile uint32_t *)(GPIOC_BASE + GPIO_MODER);
    *pc_moder &= ~(0x3U << (USB_DETECT_PIN * 2));

    /* Charge status (PC15): input */
    *pc_moder &= ~(0x3U << (CHARGE_STAT_PIN * 2));

    /* Turn off all LEDs initially */
    gpio_set(LED_R_PORT, LED_R_PIN);   /* Active-low LEDs: set = off */
    gpio_set(LED_G_PORT, LED_G_PIN);
    gpio_set(LED_B_PORT, LED_B_PIN);
    gpio_reset(BUZZER_PORT, BUZZER_PIN);  /* Buzzer off */
}

/* ===================================================================== */
/*  SSD1306 OLED display driver (minimal I2C)                             */
/* ===================================================================== */

/* I2C is already initialized by imu_init() (shared I2C1).
 * We add OLED-specific commands here.
 */

static uint8_t oled_write_cmd(uint8_t cmd)
{
    /* Send control byte (0x00 = command) + data byte */
    I2C_CR2 = ((uint32_t)SSD1306_I2C_ADDR << 1) | I2C_CR2_NBYTES_2;
    I2C_CR2 |= I2C_CR2_START;
    while (!(I2C_ISR & I2C_ISR_TXIS))
        ;
    I2C_TXDR = 0x00;  /* Co=0, D/C=0 → command */
    while (!(I2C_ISR & I2C_ISR_TXIS))
        ;
    I2C_TXDR = cmd;
    while (!(I2C_ISR & I2C_ISR_TC))
        ;
    I2C_CR2 |= I2C_CR2_STOP;
    while (I2C_ISR & I2C_ISR_BUSY)
        ;
    return 1;
}

static uint8_t oled_write_data(uint8_t data)
{
    I2C_CR2 = ((uint32_t)SSD1306_I2C_ADDR << 1) | I2C_CR2_NBYTES_2;
    I2C_CR2 |= I2C_CR2_START;
    while (!(I2C_ISR & I2C_ISR_TXIS))
        ;
    I2C_TXDR = 0x40;  /* Co=0, D/C=1 → data */
    while (!(I2C_ISR & I2C_ISR_TXIS))
        ;
    I2C_TXDR = data;
    while (!(I2C_ISR & I2C_ISR_TC))
        ;
    I2C_CR2 |= I2C_CR2_STOP;
    while (I2C_ISR & I2C_ISR_BUSY)
        ;
    return 1;
}

static void oled_init(void)
{
    delay_ms(100);
    oled_write_cmd(SSD1306_CMD_DISPLAY_OFF);

    /* Set display clock divide, multiplex ratio, offset */
    oled_write_cmd(0xD5U); oled_write_cmd(0x80U);  /* Display clock divide */
    oled_write_cmd(0xA8U); oled_write_cmd(0x3FU);  /* Multiplex ratio = 64 */
    oled_write_cmd(0xD3U); oled_write_cmd(0x00U);  /* Display offset = 0 */
    oled_write_cmd(0x40U);                          /* Start line = 0 */

    /* Charge pump */
    oled_write_cmd(0x8DU); oled_write_cmd(0x14U);  /* Enable charge pump */

    /* Addressing mode: horizontal */
    oled_write_cmd(0x20U); oled_write_cmd(0x00U);

    /* Segment remap, COM scan direction */
    oled_write_cmd(0xA1U);  /* Segment remap (I2C: A1) */
    oled_write_cmd(0xC8U);  /* COM scan direction (C8) */

    /* Display config */
    oled_write_cmd(0xDAU); oled_write_cmd(0x12U);  /* COM pins = 12 */
    oled_write_cmd(0xD9U); oled_write_cmd(0xF1U);  /* Pre-charge period */
    oled_write_cmd(0xDBU); oled_write_cmd(0x40U);  /* VCOMH deselect */
    oled_write_cmd(0xA4U);  /* Display follows RAM */
    oled_write_cmd(0xA6U);  /* Normal (non-inverted) */

    /* Contrast */
    oled_write_cmd(SSD1306_CMD_CONTRAST); oled_write_cmd(0xCFU);

    /* Turn on */
    oled_write_cmd(SSD1306_CMD_DISPLAY_ON);
}

static void oled_clear(void)
{
    /* Set column and page address range to full display */
    oled_write_cmd(SSD1306_CMD_COL_ADDR_SET);
    oled_write_cmd(0x00U);  /* Start = 0 */
    oled_write_cmd(0x7FU);  /* End = 127 */
    oled_write_cmd(SSD1306_CMD_PAGE_ADDR_SET);
    oled_write_cmd(0x00U);  /* Start page = 0 */
    oled_write_cmd(0x07U);  /* End page = 7 */

    /* Write 128 × 8 pages × 4 bytes each = 1024 bytes of zeros */
    for (int i = 0; i < 1024; i++)
        oled_write_data(0x00);
}

/* Simple 5×8 font for status display (simplified) */
static const uint8_t font5x8[][5] = {
    {0x3E,0x51,0x49,0x45,0x3E}, /* 0 */
    {0x00,0x42,0x7F,0x40,0x00}, /* 1 */
    {0x42,0x61,0x51,0x49,0x46}, /* 2 */
    {0x21,0x41,0x45,0x4B,0x31}, /* 3 */
    {0x18,0x14,0x12,0x7F,0x10}, /* 4 */
    {0x27,0x45,0x45,0x45,0x39}, /* 5 */
    {0x3C,0x4A,0x49,0x49,0x30}, /* 6 */
    {0x01,0x71,0x09,0x05,0x03}, /* 7 */
    {0x36,0x49,0x49,0x49,0x36}, /* 8 */
    {0x06,0x49,0x49,0x29,0x1E}, /* 9 */
    {0x7C,0x12,0x11,0x12,0x7C}, /* A */
    {0x7F,0x49,0x49,0x49,0x36}, /* B */
    {0x3E,0x41,0x41,0x41,0x22}, /* C */
    {0x7F,0x41,0x41,0x22,0x1C}, /* D */
    {0x7F,0x49,0x49,0x49,0x41}, /* E */
    {0x7F,0x09,0x09,0x09,0x01}, /* F */
    {0x20,0x41,0x41,0x5F,0x41}, /* G */
    {0x7F,0x08,0x08,0x08,0x7F}, /* H */
    {0x00,0x41,0x7F,0x41,0x00}, /* I */
    {0x02,0x01,0x01,0x7F,0x01}, /* J */
    {0x7F,0x08,0x14,0x22,0x41}, /* K */
    {0x7F,0x40,0x40,0x40,0x40}, /* L */
    {0x7F,0x02,0x0C,0x02,0x7F}, /* M */
    {0x7F,0x04,0x08,0x10,0x7F}, /* N */
    {0x3E,0x41,0x41,0x41,0x3E}, /* O */
    {0x7F,0x09,0x09,0x09,0x06}, /* P */
    {0x1E,0x21,0x21,0x21,0x5E}, /* Q */
    {0x7F,0x09,0x19,0x29,0x46}, /* R */
    {0x46,0x49,0x49,0x49,0x31}, /* S */
    {0x01,0x01,0x7F,0x01,0x01}, /* T */
    {0x3F,0x40,0x40,0x40,0x3F}, /* U */
    {0x1F,0x20,0x20,0x20,0x1F}, /* V */
    {0x3F,0x40,0x38,0x40,0x3F}, /* W */
    {0x63,0x14,0x08,0x14,0x63}, /* X */
    {0x07,0x08,0x70,0x08,0x07}, /* Y */
    {0x61,0x51,0x49,0x45,0x43}, /* Z */
    {0x00,0x00,0x00,0x00,0x00}, /* space */
};

static void oled_set_cursor(uint8_t col, uint8_t page)
{
    oled_write_cmd(0x00U + (col & 0x0FU));            /* Low column */
    oled_write_cmd(0x10U + ((col >> 4) & 0x0FU));     /* High column */
    oled_write_cmd(0xB0U + page);                      /* Page address */
}

static void oled_draw_char(uint8_t col, uint8_t page, char c)
{
    uint8_t idx;
    if (c >= '0' && c <= '9') idx = c - '0';
    else if (c >= 'A' && c <= 'Z') idx = c - 'A' + 10;
    else idx = 36;  /* space */

    oled_set_cursor(col, page);
    for (int i = 0; i < 5; i++) {
        oled_write_data(font5x8[idx][i]);
    }
    oled_write_data(0x00);  /* Inter-char spacing */
}

static void oled_draw_string(uint8_t col, uint8_t page, const char *str)
{
    while (*str) {
        oled_draw_char(col, page, *str);
        col += 6;
        if (col > 122) {
            col = 0;
            page++;
        }
        str++;
    }
}

/* ===================================================================== */
/*  Global state                                                           */
/* ===================================================================== */

static ferro_mode_t current_mode = MODE_BOOT;
static ferro_mode_t requested_mode = MODE_IDLE;
static int32_t anomaly_threshold_nt = 2000;  /* 2 µT default */
static uint16_t survey_count = 0;
static uint8_t ble_streaming = 0;
static uint32_t last_display_update = 0;
static uint32_t last_ble_update = 0;
static uint32_t last_imu_read = 0;
static uint32_t last_field_check = 0;

/* ===================================================================== */
/*  BLE callbacks                                                          */
/* ===================================================================== */

static void on_mode_change(uint8_t mode)
{
    requested_mode = (ferro_mode_t)mode;
}

static void on_threshold_change(int32_t threshold)
{
    anomaly_threshold_nt = threshold;
}

static void on_rate_change(uint8_t rate)
{
    if (rate <= 3) {
        lockin_set_rate((lockin_rate_t)rate);
    }
}

/* ===================================================================== */
/*  Mode transitions                                                       */
/* ===================================================================== */

static void enter_mode(ferro_mode_t mode)
{
    switch (mode) {
    case MODE_SURVEY:
        /* Start GPS, SD logging, excitation */
        fluxgate_enable();
        adc_start_continuous();
        {
            char filename[16];
            /* Generate filename: SURV_NN.BIN */
            filename[0] = 'S'; filename[1] = 'U'; filename[2] = 'R';
            filename[3] = 'V'; filename[4] = '_';
            uint8_t n = survey_count / 10;
            filename[5] = '0' + (n / 10);
            filename[6] = '0' + (n % 10);
            filename[7] = '.'; filename[8] = 'B'; filename[9] = 'I';
            filename[10] = 'N'; filename[11] = '\0';
            if (sdlog_start(filename)) {
                survey_count++;
            }
        }
        break;

    case MODE_CALIB:
        fluxgate_enable();
        adc_start_continuous();
        calib_start_collection(CALIB_DEFAULT_DURATION_S);
        break;

    case MODE_MONITOR:
        fluxgate_enable();
        adc_start_continuous();
        ble_streaming = 1;
        break;

    case MODE_IDLE:
        fluxgate_disable();
        adc_stop_continuous();
        ble_streaming = 0;
        break;

    case MODE_SHUTDOWN:
        fluxgate_disable();
        adc_stop_continuous();
        sdlog_stop();
        break;

    default:
        break;
    }

    current_mode = mode;
}

/* ===================================================================== */
/*  Main function                                                          */
/* ===================================================================== */

int main(void)
{
    /* Enable DWT cycle counter (for delay_us) */
    DWT_CTRL |= (1U << 0);
    DWT_CYCCNT = 0;

    /* Enable FPU (CP10/CP11) */
    SCB_CPACR |= (0xFU << 20);  /* Full access to CP10/CP11 */
    __asm volatile("DSB");
    __asm volatile("ISB");

    /* Initialize clock tree: HSI → PLL1 → 480 MHz */
    clock_init();

    /* Initialize SysTick (1 ms tick) */
    systick_init();

    /* Initialize GPIOs (LEDs, buttons, buzzer) */
    gpio_init();

    /* Initialize OLED display */
    oled_init();
    oled_clear();
    oled_draw_string(0, 0, "FERROPROBE");
    oled_draw_string(0, 1, "BOOTING...");
    oled_draw_string(0, 3, "BY JAYIS1");

    /* Initialize BLE first (for early status reporting) */
    ble_init();
    ble_set_mode_callback(on_mode_change);
    ble_set_threshold_callback(on_threshold_change);
    ble_set_rate_callback(on_rate_change);

    /* Initialize fluxgate excitation */
    fluxgate_init();
    fluxgate_set_amplitude(0.5f);  /* 50% drive for initial testing */

    /* Initialize 24-bit ADC */
    adc_init();

    /* Initialize lock-in engine */
    lockin_init();
    lockin_set_rate(LOCKIN_RATE_SURVEY);

    /* Initialize IMU */
    imu_init();
    imu_set_sample_rate_hz(400);

    /* Initialize GPS */
    gps_init();

    /* Initialize SD card logger */
    uint8_t sd_ok = sdlog_init();

    /* Initialize calibration module and load from flash */
    calib_init();
    calib_load_from_flash();

    /* Clear display and show idle screen */
    oled_clear();
    oled_draw_string(0, 0, "FERROPROBE");
    oled_draw_string(0, 1, "READY");
    if (sd_ok)
        oled_draw_string(0, 2, "SD OK");
    else
        oled_draw_string(0, 2, "NO SD");

    /* Enter idle mode */
    enter_mode(MODE_IDLE);

    /* Main super-loop */
    field_measurement_t field;
    imu_data_t imu_data;
    gps_data_t gps_data;

    while (1) {
        uint32_t now = get_tick_ms();

        /* Handle mode change requests from BLE */
        if (requested_mode != current_mode) {
            /* Leave current mode */
            if (current_mode == MODE_SURVEY)
                sdlog_stop();
            if (current_mode == MODE_MONITOR)
                ble_streaming = 0;

            /* Enter new mode */
            enter_mode(requested_mode);
        }

        /* Read IMU at 100 Hz (every 10 ms) */
        if (now - last_imu_read >= 10) {
            imu_get_orientation(&imu_data);
            last_imu_read = now;
        }

        /* Poll GPS for new NMEA sentences */
        gps_poll(&gps_data);

        /* Process lock-in data (check for new field measurement) */
        if (now - last_field_check >= 5) {
            field.timestamp_ms = now;
            if (lockin_process(&field)) {
                /* Apply orientation compensation */
                calib_apply_orientation(&field, &imu_data);

                /* Log to SD card in survey mode */
                if (current_mode == MODE_SURVEY) {
                    uint8_t flags = 0;
                    if (gps_has_fix()) flags |= LOG_FLAG_GPS_FIX;
                    if (lockin_calib_valid()) flags |= LOG_FLAG_CALIB_OK;
                    /* Check for anomaly */
                    float anomaly = field.b_total - 50000.0f;  /* Deviation from ~50 µT */
                    if ((int32_t)abs((int)anomaly) > anomaly_threshold_nt)
                        flags |= LOG_FLAG_ANOMALY;
                    sdlog_write(&field, &gps_data, &imu_data, flags);
                }

                /* Stream field data over BLE in monitor mode */
                if (ble_streaming) {
                    uint8_t flags = 0;
                    if (gps_has_fix()) flags |= LOG_FLAG_GPS_FIX;
                    ble_send_field(&field, &gps_data, flags);
                }

                /* Anomaly detection (audio + visual alert) */
                if (current_mode == MODE_SURVEY || current_mode == MODE_MONITOR) {
                    float dev = fabsf(field.b_total - 50000.0f);
                    if ((int32_t)dev > anomaly_threshold_nt) {
                        /* Beep the buzzer and set red LED */
                        gpio_set(BUZZER_PORT, BUZZER_PIN);
                        gpio_reset(LED_R_PORT, LED_R_PIN);
                    } else {
                        gpio_reset(BUZZER_PORT, BUZZER_PIN);
                        gpio_set(LED_R_PORT, LED_R_PIN);
                    }
                }
            }
            last_field_check = now;
        }

        /* Process BLE commands */
        ble_process();

        /* Update BLE status (1 Hz) */
        if (now - last_ble_update >= 1000) {
            ble_update_status(
                100,  /* Battery percentage (would read from ADC) */
                gps_has_fix() ? 1 : 0,
                current_mode,
                lockin_calib_valid() ? 1 : 0,
                sdlog_record_count(),
                (int8_t)25  /* Temperature (would read from sensor) */
            );
            last_ble_update = now;
        }

        /* Update OLED display (5 Hz) */
        if (now - last_display_update >= 200) {
            oled_clear();

            /* Line 0: Title */
            oled_draw_string(0, 0, "FERROPROBE");

            /* Line 1: Mode */
            const char *mode_str = "IDLE";
            switch (current_mode) {
                case MODE_SURVEY:  mode_str = "SURVEY"; break;
                case MODE_CALIB:   mode_str = "CALIB";  break;
                case MODE_MONITOR: mode_str = "MONITOR"; break;
                case MODE_CONFIG:  mode_str = "CONFIG"; break;
                default: break;
            }
            oled_draw_string(0, 1, mode_str);

            /* Line 2: Field magnitude */
            /* Display |B| in µT (e.g., "49.8UT" for 49.8 µT) */
            float bt_ut = field.b_total / 1000.0f;
            char field_str[12];
            int32_t bt_int = (int32_t)(bt_ut * 10.0f);
            field_str[0] = 'B';
            field_str[1] = '=';
            if (bt_int < 0) {
                field_str[2] = '-';
                bt_int = -bt_int;
            } else {
                field_str[2] = '0' + ((bt_int / 100) % 10);
            }
            field_str[3] = '0' + ((bt_int / 10) % 10);
            field_str[4] = '.';
            field_str[5] = '0' + (bt_int % 10);
            field_str[6] = 'U'; field_str[7] = 'T';
            field_str[8] = '\0';
            oled_draw_string(0, 2, field_str);

            /* Line 3: GPS status */
            if (gps_has_fix()) {
                oled_draw_string(0, 3, "GPS FIX OK");
            } else {
                oled_draw_string(0, 3, "NO GPS");
            }

            /* Line 4: Record count (in survey mode) */
            if (current_mode == MODE_SURVEY) {
                /* Display record count */
                uint32_t recs = sdlog_record_count();
                char rec_str[12];
                rec_str[0] = 'R';
                rec_str[1] = '=';
                rec_str[2] = '0' + ((recs / 1000) % 10);
                rec_str[3] = '0' + ((recs / 100) % 10);
                rec_str[4] = '0' + ((recs / 10) % 10);
                rec_str[5] = '0' + (recs % 10);
                rec_str[6] = '\0';
                oled_draw_string(0, 4, rec_str);
            }

            last_display_update = now;
        }

        /* Check calibration completion in calib mode */
        if (current_mode == MODE_CALIB && calib_collection_complete()) {
            calib_compute();
            const calib_params_t *params = calib_get();
            if (params->valid) {
                calib_save_to_flash();
                oled_clear();
                oled_draw_string(0, 0, "CALIB DONE");
                requested_mode = MODE_IDLE;
            } else {
                oled_clear();
                oled_draw_string(0, 0, "CALIB FAIL");
                requested_mode = MODE_IDLE;
            }
        }

        /* Check buttons */
        if (!gpio_read(BTN_MODE_PORT, BTN_MODE_PIN)) {
            /* Mode button pressed: cycle through modes */
            delay_ms(50);  /* Debounce */
            if (!gpio_read(BTN_MODE_PORT, BTN_MODE_PIN)) {
                switch (current_mode) {
                    case MODE_IDLE:     requested_mode = MODE_MONITOR; break;
                    case MODE_MONITOR:  requested_mode = MODE_SURVEY; break;
                    case MODE_SURVEY:   requested_mode = MODE_CALIB; break;
                    case MODE_CALIB:    requested_mode = MODE_IDLE; break;
                    default:            requested_mode = MODE_IDLE; break;
                }
                while (!gpio_read(BTN_MODE_PORT, BTN_MODE_PIN))
                    ;  /* Wait for release */
            }
        }

        if (!gpio_read(BTN_START_PORT, BTN_START_PIN)) {
            /* Start/stop button pressed */
            delay_ms(50);
            if (!gpio_read(BTN_START_PORT, BTN_START_PIN)) {
                if (current_mode == MODE_SURVEY || current_mode == MODE_MONITOR) {
                    requested_mode = MODE_IDLE;
                } else {
                    requested_mode = MODE_SURVEY;
                }
                while (!gpio_read(BTN_START_PORT, BTN_START_PIN))
                    ;
            }
        }

        /* Green LED heartbeat (toggle every 500 ms) */
        static uint32_t last_heartbeat = 0;
        if (now - last_heartbeat >= 500) {
            static uint8_t led_state = 0;
            if (led_state)
                gpio_set(LED_G_PORT, LED_G_PIN);
            else
                gpio_reset(LED_G_PORT, LED_G_PIN);
            led_state = !led_state;
            last_heartbeat = now;
        }
    }

    return 0;
}