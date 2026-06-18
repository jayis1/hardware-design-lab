/*
 * main.c — FloraPulse firmware main entry point
 * FloraPulse — Plant Electrophysiology & Sap-Flow Stress Monitor
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * This is the main firmware for the FloraPulse plant stress monitor.
 * It initializes all hardware peripherals, runs the main state machine
 * super-loop, and coordinates data acquisition, AP detection, sap-flow
 * measurement, environmental sensing, SD logging, BLE communication,
 * and LoRa uplink.
 *
 * Architecture: cooperative super-loop with interrupt-driven I/O.
 *  - TIM2 generates 1 Hz system tick → triggers sampling cycle
 *  - ADS1292 samples 2 channels at 250 SPS (plant action potentials)
 *  - ADS1220 reads thermistors for sap-flow (on demand)
 *  - TIM3 counts leaf-wetness oscillator frequency (on demand)
 *  - BME280 reads temperature/humidity/pressure (on demand)
 *  - APDS-9301 reads ambient light / PAR (on demand)
 *  - USART1 receives commands from nRF52840 BLE module (interrupt-driven)
 *  - SPI2 communicates with SX1276 LoRa + SD card (CS-multiplexed)
 *  - SD card logging via SPI2 (CS-multiplexed)
 *
 * State machine:
 *  BOOT → IDLE → MONITOR (BLE streaming) / LOGGING (SD only) /
 *                CALIB / STREAM (waveform) / LORA (low-power uplink) → SLEEP
 */

#include "board.h"
#include "registers.h"
#include "drivers/ads1292.h"
#include "drivers/bme280.h"
#include "drivers/sapflow.h"
#include "drivers/leafwet.h"
#include "drivers/lora.h"
#include "drivers/ble.h"
#include "drivers/sdlog.h"
#include "drivers/apdetection.h"
#include <string.h>

/* ===================================================================== */
/*  System tick (1 ms, from SysTick)                                      */
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
    /* Configure SysTick for 1 ms interrupts at 80 MHz */
    SYSTICK_LOAD = 80000U - 1;
    SYSTICK_VAL = 0;
    SYSTICK_CTRL = 0x7;  /* Enable, interrupt, core clock */
}

static void delay_ms(uint32_t ms)
{
    uint32_t start = system_tick_ms;
    while ((system_tick_ms - start) < ms)
        ;
}

static void delay_us(uint32_t us)
{
    /* Approximate at 80 MHz: 80 cycles per µs */
    for (volatile uint32_t i = 0; i < us * 10; i++)
        ;
}

/* ===================================================================== */
/*  Clock initialization (HSI16 → PLL → 80 MHz SYSCLK)                    */
/* ===================================================================== */

static void clock_init(void)
{
    /* Enable HSI16 */
    RCC_CR |= RCC_CR_HSION;
    while (!(RCC_CR & RCC_CR_HSIRDY))
        ;

    /* Configure PLL: source = HSI16, M=1, N=10, R=2 → 80 MHz */
    RCC_PLLCFGR = 0;
    RCC_PLLCFGR |= RCC_PLLCFGR_PLLSRC_HSI;
    RCC_PLLCFGR |= (1U << 4);    /* M = 1 */
    RCC_PLLCFGR |= (10U << 8);   /* N = 10 */
    RCC_PLLCFGR |= (0U << 25);   /* R = 2 (00) */
    /* Enable R output */
    RCC_PLLCFGR |= (1U << 24);   /* PLLREN */

    /* Enable PLL */
    RCC_CR |= RCC_CR_PLLON;
    while (!(RCC_CR & RCC_CR_PLLRDY))
        ;

    /* Set flash latency to 4 wait states (80 MHz @ 3.3V) */
    volatile uint32_t *flash_acr = (volatile uint32_t *)(0x40022000U + 0x00U);
    *flash_acr |= 0x4U;   /* Latency = 4 WS */
    *flash_acr |= (1U << 8) | (1U << 9);  /* PRFTEN + ICEN */
    *flash_acr |= (1U << 10);  /* DCEN */

    /* Configure bus prescalers: HCLK=80, APB1=80, APB2=80 */
    RCC_CFGR = 0;
    RCC_CFGR |= RCC_CFGR_HPRE_DIV1;
    RCC_CFGR |= RCC_CFGR_PPRE1_DIV1;
    RCC_CFGR |= RCC_CFGR_PPRE2_DIV1;

    /* Switch SYSCLK to PLL */
    RCC_CFGR |= RCC_CFGR_SW_PLL;
    while (((RCC_CFGR >> 2) & 0x3U) != 0x3U)
        ;
}

/* ===================================================================== */
/*  GPIO initialization for LEDs, buttons                                 */
/* ===================================================================== */

static void gpio_init(void)
{
    RCC_AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOCEN;

    /* LED pins: PA8 (status), PA15 (alert), PC10 (BLE TX), PC11 (LoRa) */
    volatile uint32_t *pa_moder = (volatile uint32_t *)(GPIOA_BASE + GPIO_MODER);
    volatile uint32_t *pc_moder = (volatile uint32_t *)(GPIOC_BASE + GPIO_MODER);

    *pa_moder |= (0x1U << (LED_STATUS_PIN * 2));
    *pa_moder |= (0x1U << (LED_ALERT_PIN * 2));
    *pc_moder |= (0x1U << (LED_TX_PIN * 2));
    *pc_moder |= (0x1U << (LED_LORA_PIN * 2));

    /* Buttons: PC12 (input, pull-up) */
    *pc_moder &= ~(0x3U << (BTN_USER_PIN * 2));
    volatile uint32_t *pc_pupdr = (volatile uint32_t *)(GPIOC_BASE + GPIO_PUPDR);
    *pc_pupdr |= (0x1U << (BTN_USER_PIN * 2));

    /* Turn off all LEDs (active low: set = off) */
    gpio_set(LED_STATUS_PORT, LED_STATUS_PIN);
    gpio_set(LED_ALERT_PORT, LED_ALERT_PIN);
    gpio_set(LED_TX_PORT, LED_TX_PIN);
    gpio_set(LED_LORA_PORT, LED_LORA_PIN);
}

/* ===================================================================== */
/*  RTC initialization (LSE → calendar)                                    */
/* ===================================================================== */

static void rtc_init(void)
{
    /* Enable LSE */
    RCC_AHB2ENR |= (1U << 18);  /* LSEEN in RCC_BDCR — but L4 has BDCR at offset 0x90 */
    volatile uint32_t *bdcr = (volatile uint32_t *)(RCC_BASE + 0x90U);
    *bdcr |= (1U << 0);  /* LSEON */
    while (!(*bdcr & (1U << 1)))  /* LSERDY */
        ;

    /* Enable RTC clock from LSE */
    *bdcr |= (1U << 15);  /* RTCSEL = LSE (01) */
    *bdcr |= (1U << 8);   /* RTCEN */

    /* Unlock RTC write protection */
    RTC_WPR = 0xCAU;
    RTC_WPR = 0x53U;

    /* Enter init mode */
    RTC_ISR |= RTC_ISR_INIT;
    while (!(RTC_ISR & RTC_ISR_INIT))
        ;

    /* Configure prescaler for 1 Hz from 32768 Hz */
    RTC_PRER = (127U << 16) | 255U;

    /* Exit init mode */
    RTC_ISR &= ~RTC_ISR_INIT;
    while (RTC_ISR & RTC_ISR_INIT)
        ;

    /* Re-enable write protection */
    RTC_WPR = 0xFFU;
}

/* Get current Unix timestamp from RTC (simplified) */
static uint32_t rtc_get_unix_time(void)
{
    /* Read TR (time) and DR (date) registers */
    uint32_t tr = RTC_TR;
    uint32_t dr = RTC_DR;

    /* Parse BCD fields */
    uint8_t seconds = ((tr >> 0) & 0xF) + (((tr >> 4) & 0x7) * 10);
    uint8_t minutes = ((tr >> 8) & 0xF) + (((tr >> 12) & 0x7) * 10);
    uint8_t hours   = ((tr >> 16) & 0xF) + (((tr >> 20) & 0x3) * 10);
    uint8_t day     = ((dr >> 0) & 0xF) + (((dr >> 4) & 0x3) * 10);
    uint8_t month   = ((dr >> 8) & 0xF) + (((dr >> 12) & 0x1) * 10);
    uint8_t year    = ((dr >> 16) & 0xF) + (((dr >> 20) & 0xF) * 10);

    /* Simplified Unix time conversion (ignoring leap years for brevity) */
    uint32_t days = (uint32_t)year * 365 + (uint32_t)(year / 4) +
                    (uint32_t)month * 30 + day;
    return days * 86400U + (uint32_t)hours * 3600U +
           (uint32_t)minutes * 60U + seconds;
}

/* Set RTC time from Unix timestamp (for BLE time sync) */
static void rtc_set_unix_time(uint32_t unix_time)
{
    uint32_t seconds = unix_time % 60U;
    uint32_t minutes = (unix_time / 60U) % 60U;
    uint32_t hours   = (unix_time / 3600U) % 24U;
    uint32_t days    = unix_time / 86400U;

    /* Unlock RTC */
    RTC_WPR = 0xCAU;
    RTC_WPR = 0x53U;
    RTC_ISR |= RTC_ISR_INIT;
    while (!(RTC_ISR & RTC_ISR_INIT))
        ;

    /* Write time (BCD) */
    RTC_TR = ((seconds % 10) << 0) | ((seconds / 10) << 4) |
             ((minutes % 10) << 8) | ((minutes / 10) << 12) |
             ((hours % 10) << 16) | ((hours / 10) << 20);

    /* Simplified date write */
    uint32_t day = days % 30U + 1;
    uint32_t month = (days / 30U) % 12U + 1;
    uint32_t year = (days / 365U) % 100U;
    RTC_DR = ((day % 10) << 0) | ((day / 10) << 4) |
             ((month % 10) << 8) | ((month / 10) << 12) |
             ((year % 10) << 16) | ((year / 10) << 20);

    RTC_ISR &= ~RTC_ISR_INIT;
    RTC_WPR = 0xFFU;
}

/* ===================================================================== */
/*  Internal ADC (battery voltage)                                        */
/* ===================================================================== */

static void adc_init_internal(void)
{
    RCC_AHB2ENR |= RCC_AHB2ENR_ADCEN;

    /* Configure PA1 as analog input */
    volatile uint32_t *pa_moder = (volatile uint32_t *)(GPIOA_BASE + GPIO_MODER);
    *pa_moder |= (0x3U << (BATT_SENSE_PIN * 2));  /* Analog mode */

    /* Enable ADC voltage regulator */
    ADC_CR = 0;
    ADC_CR |= ADC_CR_ADVREN;
    delay_ms(1);

    /* Calibrate */
    ADC_CR |= ADC_CR_ADCAL;
    while (ADC_CR & ADC_CR_ADCAL)
        ;

    /* Enable ADC */
    ADC_CR |= ADC_CR_ADEN;
    while (!(ADC_ISR & ADC_ISR_ADRDY))
        ;

    /* Configure channel sequence: channel 1 (PA1 = ADC1_IN1 on L4) */
    ADC_SQR1 = (1U << 6);  /* L=1 (1 conversion), SQ1=1 (channel 1) */

    /* Sampling time: 247.5 cycles (long for stable battery reading) */
    volatile uint32_t *smpr1 = (volatile uint32_t *)(ADC1_BASE + 0x14U);
    *smpr1 |= (0x5U << 3);  /* SMP1 = 101 (47.5 cycles) */
}

static uint16_t adc_read_batt(void)
{
    /* Start conversion */
    ADC_CR |= (1U << 2);  /* ADSTART */
    while (!(ADC_ISR & ADC_ISR_EOC))
        ;
    uint16_t val = (uint16_t)ADC_DR;
    return val;
}

static uint8_t battery_get_percent(void)
{
    uint16_t adc_val = adc_read_batt();
    /* ADC 12-bit, Vref = 3.0 V (VREFINT on L4 is ~1.5V, but we use VDD)
     * Battery voltage = adc_val / 4095 × 3.0 × divider_ratio (2.0)
     * → mV = adc_val × 6000 / 4095
     */
    uint32_t batt_mv = (uint32_t)adc_val * 6000U / 4095U;

    if (batt_mv >= BOARD_BATT_FULL_MV) return 100;
    if (batt_mv <= BOARD_BATT_EMPTY_MV) return 0;
    return (uint8_t)((batt_mv - BOARD_BATT_EMPTY_MV) * 100U /
                     (BOARD_BATT_FULL_MV - BOARD_BATT_EMPTY_MV));
}

/* ===================================================================== */
/*  APDS-9301 ambient light sensor                                       */
/* ===================================================================== */

static uint32_t apds9301_read_lux(void)
{
    uint8_t buf[4];

    /* Write control register: power on */
    uint8_t cmd = APDS9301_REG_CTRL | APDS9301_CMD_CMD;
    uint8_t val = APDS9301_POWER_ON;

    /* Use I2C write (reuse BME280's I2C functions via inline) */
    /* For simplicity, we just read the data registers */
    volatile uint32_t *i2c_cr2 = &I2C_CR2;
    volatile uint32_t *i2c_isr = &I2C_ISR;
    volatile uint32_t *i2c_txdr = &I2C_TXDR;
    volatile uint32_t *i2c_rxdr = &I2C_RXDR;

    /* Write control reg to power on */
    I2C_CR2 = ((uint32_t)APDS9301_I2C_ADDR << 1) | (2U << 16) | I2C_CR2_AUTOEND;
    I2C_CR2 |= I2C_CR2_START;
    while (!(I2C_ISR & I2C_ISR_TXIS))
        ;
    I2C_TXDR = cmd;
    while (!(I2C_ISR & I2C_ISR_TXIS))
        ;
    I2C_TXDR = val;
    while (!(I2C_ISR & I2C_ISR_STOPF))
        ;
    I2C_ICR = I2C_ISR_STOPF;

    delay_ms(15);  /* Integration time */

    /* Read channel 0 data (2 bytes) */
    I2C_CR2 = ((uint32_t)APDS9301_I2C_ADDR << 1) | (1U << 16) | I2C_CR2_RELOAD;
    I2C_CR2 |= I2C_CR2_START;
    while (!(I2C_ISR & I2C_ISR_TXIS))
        ;
    I2C_TXDR = APDS9301_REG_DATA0 | APDS9301_CMD_CMD;
    while (!(I2C_ISR & I2C_ISR_TCR))
        ;

    I2C_CR2 = ((uint32_t)APDS9301_I2C_ADDR << 1) | (2U << 16) |
              I2C_CR2_RD_WRN | I2C_CR2_AUTOEND;
    I2C_CR2 |= I2C_CR2_START;
    while (!(I2C_ISR & I2C_ISR_RXNE))
        ;
    buf[0] = (uint8_t)I2C_RXDR;
    while (!(I2C_ISR & I2C_ISR_RXNE))
        ;
    buf[1] = (uint8_t)I2C_RXDR;
    while (!(I2C_ISR & I2C_ISR_STOPF))
        ;
    I2C_ICR = I2C_ISR_STOPF;

    uint16_t ch0 = (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);

    /* Simplified lux calculation: lux ≈ ch0 × 0.5 (depends on sensor config) */
    return (uint32_t)ch0 / 2U;
}

/* ===================================================================== */
/*  Current sample data                                                  */
/* ===================================================================== */

typedef struct {
    uint32_t timestamp;
    int32_t  ap_ch1_uv;
    int32_t  ap_ch2_uv;
    int32_t  t_upper_c;     /* ×100 */
    int32_t  t_lower_c;
    int16_t  t_ambient_c;   /* ×100 */
    uint16_t rh_pct;         /* ×100 */
    uint32_t pressure_pa;
    uint32_t lux;
    uint16_t leaf_wet_hz;
    uint16_t sap_flow_vh;   /* ×10 */
    uint16_t ap_rms_uv;
    uint16_t ap_rate_hz;    /* ×10 */
    uint8_t  batt_pct;
    uint8_t  flags;
} flora_sample_t;

static flora_sample_t current_sample;

/* Forward declarations */
static void int32_to_bytes(int32_t val, uint8_t *buf);

/* ===================================================================== */
/*  Helper: pack int32 into byte array (little-endian)                   */
/* ===================================================================== */

static void int32_to_bytes(int32_t val, uint8_t *buf)
{
    buf[0] = val & 0xFF;
    buf[1] = (val >> 8) & 0xFF;
    buf[2] = (val >> 16) & 0xFF;
    buf[3] = (val >> 24) & 0xFF;
}

/* ===================================================================== */
/*  BLE command handler                                                  */
/* ===================================================================== */

static void ble_command_handler(uint8_t opcode, const uint8_t *payload, uint8_t len)
{
    switch (opcode) {
    case BLE_CMD_PING:
        ble_send(BLE_RSP_OK, 0, 0);
        break;

    case BLE_CMD_GET_STATUS: {
        uint8_t status[8];
        status[0] = MODE_MONITOR;  /* Current mode */
        status[1] = battery_get_percent();
        status[2] = sd_is_present();
        status[3] = (uint8_t)(sd_get_record_count() & 0xFF);
        status[4] = (uint8_t)((sd_get_record_count() >> 8) & 0xFF);
        status[5] = (uint8_t)((sd_get_record_count() >> 16) & 0xFF);
        status[6] = (uint8_t)((sd_get_record_count() >> 24) & 0xFF);
        status[7] = sapflow_is_measuring();
        ble_send(BLE_RSP_STATUS, status, 8);
        break;
    }

    case BLE_CMD_GET_SAMPLE: {
        /* Pack the current sample into the BLE payload */
        uint8_t buf[48];
        memset(buf, 0, sizeof(buf));

        uint32_t ts = current_sample.timestamp;
        buf[0] = ts & 0xFF; buf[1] = (ts >> 8) & 0xFF;
        buf[2] = (ts >> 16) & 0xFF; buf[3] = (ts >> 24) & 0xFF;

        int32_t v = current_sample.ap_ch1_uv;
        buf[4] = v & 0xFF; buf[5] = (v >> 8) & 0xFF;
        buf[6] = (v >> 16) & 0xFF; buf[7] = (v >> 24) & 0xFF;

        v = current_sample.ap_ch2_uv;
        buf[8] = v & 0xFF; buf[9] = (v >> 8) & 0xFF;
        buf[10] = (v >> 16) & 0xFF; buf[11] = (v >> 24) & 0xFF;

        int32_t t = current_sample.t_ambient_c;
        buf[12] = t & 0xFF; buf[13] = (t >> 8) & 0xFF;

        uint16_t rh = current_sample.rh_pct;
        buf[14] = rh & 0xFF; buf[15] = (rh >> 8) & 0xFF;

        uint16_t sap = current_sample.sap_flow_vh;
        buf[16] = sap & 0xFF; buf[17] = (sap >> 8) & 0xFF;

        buf[18] = current_sample.batt_pct;
        buf[19] = current_sample.flags;

        ble_send(BLE_RSP_DATA, buf, 20);
        break;
    }

    case BLE_CMD_START_STREAM:
        /* Enter streaming mode — handled by state machine */
        ble_send(BLE_RSP_OK, 0, 0);
        break;

    case BLE_CMD_STOP_STREAM:
        ble_send(BLE_RSP_OK, 0, 0);
        break;

    case BLE_CMD_SET_RATE: {
        uint8_t rate = payload[0];
        (void)rate;  /* TODO: adjust sample rate */
        ble_send(BLE_RSP_OK, 0, 0);
        break;
    }

    case BLE_CMD_SET_THRESH: {
        float thresh;
        memcpy(&thresh, payload, 4);
        ap_detect_set_threshold(thresh);
        ble_send(BLE_RSP_OK, 0, 0);
        break;
    }

    case BLE_CMD_DOWNLOAD_LOG: {
        uint32_t start_idx;
        memcpy(&start_idx, payload, 4);
        uint8_t log_buf[128];
        uint16_t n = sd_read_records(start_idx, log_buf, 2);  /* 2 records = 96 bytes */
        if (n > 0) {
            ble_send_log_chunk(start_idx * LOG_RECORD_SIZE, log_buf, n * LOG_RECORD_SIZE);
        } else {
            ble_send(BLE_RSP_ERROR, 0, 0);
        }
        break;
    }

    case BLE_CMD_ERASE_LOG:
        sd_erase_logs();
        ble_send(BLE_RSP_OK, 0, 0);
        break;

    case BLE_CMD_SET_TIME: {
        uint32_t new_time;
        memcpy(&new_time, payload, 4);
        rtc_set_unix_time(new_time);
        ble_send(BLE_RSP_OK, 0, 0);
        break;
    }

    case BLE_CMD_GET_VERSION: {
        uint8_t ver[8];
        ver[0] = FW_VERSION_MAJOR;
        ver[1] = FW_VERSION_MINOR;
        ver[2] = FW_VERSION_PATCH;
        ver[3] = 0;  /* Hardware rev */
        ver[4] = 0;  /* Reserved */
        ver[5] = 0;
        ver[6] = 0;
        ver[7] = 0;
        ble_send(BLE_RSP_VERSION, ver, 8);
        break;
    }

    case BLE_CMD_TRIGGER_HEATER:
        sapflow_trigger_async();
        ble_send(BLE_RSP_OK, 0, 0);
        break;

    case BLE_CMD_GET_WAVEFORM: {
        /* Stream 250 samples (1 second) of AP data */
        int32_t ch1, ch2;
        ads1292_read_both(&ch1, &ch2);
        uint8_t buf[8];
        int32_to_bytes(ch1, &buf[0]);
        int32_to_bytes(ch2, &buf[4]);
        ble_send(BLE_RSP_WAVEFORM, buf, 8);
        break;
    }

    default:
        ble_send(BLE_RSP_ERROR, 0, 0);
        break;
    }
}

/* ===================================================================== */
/*  LoRa uplink                                                          */
/* ===================================================================== */

static void lora_send_telemetry(const flora_sample_t *s)
{
    uint8_t pkt[LORA_PKT_SIZE];
    memset(pkt, 0, sizeof(pkt));

    pkt[0] = 0x01;  /* Device ID */
    uint32_t ts = s->timestamp;
    pkt[1] = ts & 0xFF; pkt[2] = (ts >> 8) & 0xFF;
    pkt[3] = (ts >> 16) & 0xFF; pkt[4] = (ts >> 24) & 0xFF;

    int16_t ap_avg = (int16_t)((s->ap_ch1_uv + s->ap_ch2_uv) / 2);
    pkt[5] = ap_avg & 0xFF; pkt[6] = (ap_avg >> 8) & 0xFF;

    uint16_t sap = s->sap_flow_vh;
    pkt[7] = sap & 0xFF; pkt[8] = (sap >> 8) & 0xFF;

    int16_t temp = s->t_ambient_c;
    pkt[9] = temp & 0xFF; pkt[10] = (temp >> 8) & 0xFF;

    uint16_t rh = s->rh_pct;
    pkt[11] = rh & 0xFF; pkt[12] = (rh >> 8) & 0xFF;

    uint16_t pa_lo = s->pressure_pa & 0xFFFF;
    pkt[13] = pa_lo & 0xFF; pkt[14] = (pa_lo >> 8) & 0xFF;

    uint16_t lux = s->lux & 0xFFFF;
    pkt[15] = lux & 0xFF; pkt[16] = (lux >> 8) & 0xFF;

    pkt[17] = leafwet_to_pct(s->leaf_wet_hz);
    pkt[18] = s->batt_pct;
    pkt[19] = s->flags;

    uint16_t rate = s->ap_rate_hz;
    pkt[20] = rate & 0xFF; pkt[21] = (rate >> 8) & 0xFF;

    /* Simple CRC16 */
    uint16_t crc = 0;
    for (int i = 0; i < 22; i++)
        crc += pkt[i];
    pkt[22] = crc & 0xFF; pkt[23] = (crc >> 8) & 0xFF;

    lora_send(pkt, LORA_PKT_SIZE);
}

/* ===================================================================== */
/*  Main state machine                                                    */
/* ===================================================================== */

static flora_mode_t current_mode = MODE_BOOT;
static uint32_t last_sample_ms = 0;
static uint32_t last_lora_ms = 0;
static uint32_t last_sapflow_ms = 0;
static uint8_t  ble_streaming = 0;

static void sample_all_sensors(void)
{
    /* Read ADS1292 (plant action potentials) */
    int32_t ap_ch1, ap_ch2;
    ads1292_read_both(&ap_ch1, &ap_ch2);
    float ap1_uv = ads1292_counts_to_uv(ap_ch1);
    float ap2_uv = ads1292_counts_to_uv(ap_ch2);

    /* Feed to AP detection engine */
    ap_event_t evt;
    ap_detect_set_tick(get_tick_ms());
    ap_detect_feed(1, ap1_uv, &evt);
    ap_detect_feed(2, ap2_uv, &evt);

    /* Read BME280 (environmental) */
    bme280_data_t env;
    bme280_read(&env);

    /* Read leaf wetness */
    uint16_t lw_hz = leafwet_measure_hz();

    /* Read ambient light */
    uint32_t lux = apds9301_read_lux();

    /* Get sap-flow (cached from last measurement) */
    float sap_v = sapflow_get_velocity();
    float t_upper, t_lower;
    sapflow_get_temps(&t_upper, &t_lower);

    /* Compute AP statistics */
    float ap_rms = ads1292_get_rms(250);  /* 1 second */
    float ap_rate = ap_detect_firing_rate();

    /* Battery */
    uint8_t batt = battery_get_percent();

    /* Assemble sample */
    current_sample.timestamp = rtc_get_unix_time();
    current_sample.ap_ch1_uv = (int32_t)(ap1_uv);
    current_sample.ap_ch2_uv = (int32_t)(ap2_uv);
    current_sample.t_upper_c = (int32_t)(t_upper * 100);
    current_sample.t_lower_c = (int32_t)(t_lower * 100);
    current_sample.t_ambient_c = (int16_t)(env.temperature * 100);
    current_sample.rh_pct = (uint16_t)(env.humidity * 100);
    current_sample.pressure_pa = (uint32_t)env.pressure;
    current_sample.lux = lux;
    current_sample.leaf_wet_hz = lw_hz;
    current_sample.sap_flow_vh = (uint16_t)(sap_v * 10);
    current_sample.ap_rms_uv = (uint16_t)ap_rms;
    current_sample.ap_rate_hz = (uint16_t)(ap_rate * 10);
    current_sample.batt_pct = batt;
    current_sample.flags = 0;
    if (ble_is_connected()) current_sample.flags |= 0x02;
    if (ap_rate > 5.0f) current_sample.flags |= 0x08;  /* Anomaly */

    /* Log to SD card */
    if (sd_is_present()) {
        uint8_t record[LOG_RECORD_SIZE];
        memset(record, 0, LOG_RECORD_SIZE);

        /* Pack sample into record (see board.h for format) */
        uint32_t ts = current_sample.timestamp;
        record[0] = ts & 0xFF; record[1] = (ts >> 8) & 0xFF;
        record[2] = (ts >> 16) & 0xFF; record[3] = (ts >> 24) & 0xFF;

        int32_t v = current_sample.ap_ch1_uv;
        record[4] = v & 0xFF; record[5] = (v >> 8) & 0xFF;
        record[6] = (v >> 16) & 0xFF; record[7] = (v >> 24) & 0xFF;

        v = current_sample.ap_ch2_uv;
        record[8] = v & 0xFF; record[9] = (v >> 8) & 0xFF;
        record[10] = (v >> 16) & 0xFF; record[11] = (v >> 24) & 0xFF;

        v = current_sample.t_upper_c;
        record[12] = v & 0xFF; record[13] = (v >> 8) & 0xFF;
        record[14] = (v >> 16) & 0xFF; record[15] = (v >> 24) & 0xFF;

        v = current_sample.t_lower_c;
        record[16] = v & 0xFF; record[17] = (v >> 8) & 0xFF;
        record[18] = (v >> 16) & 0xFF; record[19] = (v >> 24) & 0xFF;

        int16_t t = current_sample.t_ambient_c;
        record[20] = t & 0xFF; record[21] = (t >> 8) & 0xFF;

        uint16_t rh = current_sample.rh_pct;
        record[22] = rh & 0xFF; record[23] = (rh >> 8) & 0xFF;

        uint32_t pa = current_sample.pressure_pa;
        record[24] = pa & 0xFF; record[25] = (pa >> 8) & 0xFF;
        record[26] = (pa >> 16) & 0xFF; record[27] = (pa >> 24) & 0xFF;

        uint32_t lx = current_sample.lux;
        record[28] = lx & 0xFF; record[29] = (lx >> 8) & 0xFF;
        record[30] = (lx >> 16) & 0xFF; record[31] = (lx >> 24) & 0xFF;

        uint16_t lw = current_sample.leaf_wet_hz;
        record[32] = lw & 0xFF; record[33] = (lw >> 8) & 0xFF;

        uint16_t sf = current_sample.sap_flow_vh;
        record[34] = sf & 0xFF; record[35] = (sf >> 8) & 0xFF;

        uint16_t rms = current_sample.ap_rms_uv;
        record[36] = rms & 0xFF; record[37] = (rms >> 8) & 0xFF;

        uint16_t rate = current_sample.ap_rate_hz;
        record[38] = rate & 0xFF; record[39] = (rate >> 8) & 0xFF;

        record[40] = current_sample.batt_pct;
        record[41] = current_sample.flags;

        sd_write_record(record, LOG_RECORD_SIZE);
    }

    /* Stream to BLE if connected */
    if (ble_streaming && ble_is_connected()) {
        uint8_t stream_data[20];
        int32_to_bytes(current_sample.ap_ch1_uv, &stream_data[0]);
        int32_to_bytes(current_sample.ap_ch2_uv, &stream_data[4]);
        int16_t t = current_sample.t_ambient_c;
        stream_data[8] = t & 0xFF; stream_data[9] = (t >> 8) & 0xFF;
        uint16_t rh = current_sample.rh_pct;
        stream_data[10] = rh & 0xFF; stream_data[11] = (rh >> 8) & 0xFF;
        uint16_t sf = current_sample.sap_flow_vh;
        stream_data[12] = sf & 0xFF; stream_data[13] = (sf >> 8) & 0xFF;
        uint16_t lw = current_sample.leaf_wet_hz;
        stream_data[14] = lw & 0xFF; stream_data[15] = (lw >> 8) & 0xFF;
        uint16_t rms = current_sample.ap_rms_uv;
        stream_data[16] = rms & 0xFF; stream_data[17] = (rms >> 8) & 0xFF;
        uint16_t rate = current_sample.ap_rate_hz;
        stream_data[18] = rate & 0xFF; stream_data[19] = (rate >> 8) & 0xFF;
        ble_stream_sample(stream_data, 20);
    }
}

/* ===================================================================== */
/*  Main function                                                         */
/* ===================================================================== */

int main(void)
{
    /* Disable interrupts during init */
    __asm volatile ("cpsid i");

    /* Initialize clock, SysTick, GPIO */
    clock_init();
    systick_init();
    gpio_init();

    /* Enable FPU (Cortex-M4F) */
    SCB_CPACR |= (0xFU << 20);  /* Enable CP10, CP11 full access */

    /* Enable SYSCFG for EXTI */
    RCC_APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    /* Initialize RTC */
    rtc_init();

    /* Initialize internal ADC (battery) */
    adc_init_internal();

    /* Initialize sensors */
    bme280_init();
    ads1292_init();
    leafwet_init();
    sapflow_init();

    /* Initialize communication */
    ble_init();
    ble_set_handler(ble_command_handler);
    lora_init();

    /* Initialize SD card */
    uint8_t sd_status = sd_init();
    if (sd_status == 0) {
        sd_open_log();
    }

    /* Start ADS1292 continuous sampling */
    ads1292_start();

    /* Initialize AP detection */
    ap_detect_init();
    ap_detect_set_threshold(50.0f);  /* 50 µV default */
    ap_detect_enable_adaptive(1);

    /* Enter monitor mode */
    current_mode = MODE_MONITOR;

    /* Enable interrupts */
    __asm volatile ("cpsie i");

    /* Blink status LED to indicate boot complete */
    for (int i = 0; i < 3; i++) {
        gpio_reset(LED_STATUS_PORT, LED_STATUS_PIN);
        delay_ms(100);
        gpio_set(LED_STATUS_PORT, LED_STATUS_PIN);
        delay_ms(100);
    }

    /* Main super-loop */
    while (1) {
        uint32_t now = get_tick_ms();

        /* Poll BLE for commands */
        ble_poll();

        /* Check for async sap-flow measurement */
        if (sapflow_poll()) {
            gpio_reset(LED_ALERT_PORT, LED_ALERT_PIN);
            delay_ms(200);
            gpio_set(LED_ALERT_PORT, LED_ALERT_PIN);
        }

        /* Periodic sampling (1 Hz) */
        if ((now - last_sample_ms) >= 1000) {
            last_sample_ms = now;

            gpio_reset(LED_STATUS_PORT, LED_STATUS_PIN);
            sample_all_sensors();
            gpio_set(LED_STATUS_PORT, LED_STATUS_PIN);
        }

        /* Periodic sap-flow measurement (every 15 minutes) */
        if ((now - last_sapflow_ms) >= 900000U) {
            last_sapflow_ms = now;
            if (!sapflow_is_measuring()) {
                sapflow_trigger_async();
            }
        }

        /* LoRa uplink (every 5 minutes) */
        if (current_mode == MODE_LORA || current_mode == MODE_MONITOR) {
            if ((now - last_lora_ms) >= (LORA_TX_INTERVAL_S * 1000U)) {
                last_lora_ms = now;
                gpio_reset(LED_LORA_PORT, LED_LORA_PIN);
                lora_send_telemetry(&current_sample);
                gpio_set(LED_LORA_PORT, LED_LORA_PIN);
            }
        }

        /* Check for anomaly (high AP rate = plant stress) */
        if (ap_detect_firing_rate() > 10.0f) {
            gpio_reset(LED_ALERT_PORT, LED_ALERT_PIN);
        } else {
            gpio_set(LED_ALERT_PORT, LED_ALERT_PIN);
        }

        /* Check user button (mode switch) */
        if (!gpio_read(BTN_USER_PORT, BTN_USER_PIN)) {
            delay_ms(50);  /* Debounce */
            if (!gpio_read(BTN_USER_PORT, BTN_USER_PIN)) {
                /* Cycle through modes */
                current_mode++;
                if (current_mode > MODE_LORA)
                    current_mode = MODE_IDLE;

                /* Wait for button release */
                while (!gpio_read(BTN_USER_PORT, BTN_USER_PIN))
                    ;
            }
        }

        /* Brief idle delay to reduce power */
        delay_ms(10);
    }

    return 0;
}