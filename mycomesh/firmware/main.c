/*
 * main.c — MycoMesh firmware main application
 * MycoMesh — Open-Source Fungal Mycelium Electrophysiology & Chemical Sensing Network
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * This is the main firmware entry point for the MycoMesh sensor node.
 * It initializes all hardware peripherals, runs the electrophysiology
 * sampling + DSP pipeline, monitors environmental sensors, classifies
 * fungal activity, logs data to microSD, and transmits summaries over
 * LoRaWan.
 *
 * The firmware uses a super-loop with a cooperative task scheduler driven
 * by the 1 ms SysTick.  The main state machine:
 *
 *   INIT → IDLE → ACQUIRE → DSP → CLASSIFY → LOG → TELEMETRY → SLEEP → IDLE
 *
 * A long-press of the user button enters calibration mode; a short press
 * cycles the active window duration.  USB-C connection pauses the sleep
 * cycle for configuration and bulk data download.
 */

#include "board.h"
#include "registers.h"
#include "drivers/ads1298.h"
#include "drivers/dsp.h"
#include "drivers/spike_sort.h"
#include "drivers/classifier.h"
#include "drivers/env_sensors.h"
#include "drivers/sdlog.h"
#include "drivers/lorawan.h"
#include "drivers/power.h"
#include "drivers/protocol.h"
#include <string.h>
#include <stdio.h>

/* ===================================================================== */
/*  Global state                                                          */
/* ===================================================================== */

/* System tick counter (incremented in SysTick_Handler, 1 ms resolution) */
static volatile uint32_t g_tick_ms = 0;

/* Current system state */
typedef enum {
    STATE_INIT = 0,
    STATE_IDLE,
    STATE_ACQUIRE,
    STATE_DSP,
    STATE_CLASSIFY,
    STATE_LOG,
    STATE_TELEMETRY,
    STATE_SLEEP,
    STATE_CALIBRATE,
    STATE_USB_CONNECTED,
} sys_state_t;

static volatile sys_state_t g_state = STATE_INIT;
static volatile uint32_t g_state_enter_time = 0;

/* Acquisition buffer: one block of 64 samples × 16 channels */
static int16_t g_sample_buf[ADS1298_TOTAL_CHANS][DSP_BLOCK_SIZE];
static volatile uint16_t g_samples_collected = 0;
static volatile uint8_t  g_block_ready = 0;

/* DSP output: filtered data + detected spikes */
static int16_t g_filtered_buf[ADS1298_TOTAL_CHANS][DSP_BLOCK_SIZE];
static spike_event_t g_spikes[64];  /* max spikes per block */
static volatile uint16_t g_spike_count = 0;

/* Epoch accumulator (10-second classification window) */
static epoch_summary_t g_epoch;
static uint16_t g_epoch_spike_total = 0;
static uint16_t g_epoch_propagation_total = 0;
static uint32_t g_epoch_amplitude_sum = 0;
static uint32_t g_epoch_isi_intervals[256];
static uint16_t g_epoch_isi_count = 0;
static uint32_t g_epoch_start_tick = 0;
static uint8_t  g_epoch_complete = 0;

/* Environmental data */
static env_data_t g_env;

/* Configuration */
static uint8_t  g_duty_cycle_pct = DUTY_CYCLE_DEFAULT_PCT;
static uint32_t g_active_window_ms = ACTIVE_WINDOW_SEC * 1000;
static uint32_t g_sleep_window_ms = SLEEP_WINDOW_SEC * 1000;
static uint8_t  g_mains_freq_hz = 50;     /* 50 Hz EU default */
static uint8_t  g_lora_region = 0;         /* 0 = EU868, 1 = US915 */

/* Flags */
static volatile uint8_t g_usb_connected = 0;
static volatile uint8_t g_button_press = 0;
static volatile uint32_t g_button_press_time = 0;
static volatile uint8_t g_lora_joined = 0;

/* ===================================================================== */
/*  SysTick handler — 1 ms tick                                           */
/* ===================================================================== */

void SysTick_Handler(void)
{
    g_tick_ms++;
}

uint32_t millis(void)
{
    return g_tick_ms;
}

uint32_t elapsed_ms(uint32_t start)
{
    return g_tick_ms - start;
}

/* ===================================================================== */
/*  System clock configuration                                            */
/*  PLL: HSE 8 MHz / M=1 × N=30 / R=2 = 120 MHz                          */
/* ===================================================================== */

static void clock_init(void)
{
    /* Enable HSE (8 MHz external crystal). */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));

    /* Configure PLL: 8 MHz / 1 * 30 / 2 = 120 MHz on PLLR (SYSCLK). */
    RCC->PLLCFGR = RCC_PLLCFGR_PLLSRC_HSE
                 | (1U << RCC_PLLCFGR_PLLM_Pos)
                 | (30U << RCC_PLLCFGR_PLLN_Pos)
                 | (0U << RCC_PLLCFGR_PLLP_Pos)   /* PLLP not used */
                 | (0U << RCC_PLLCFGR_PLLR_Pos)   /* /2 */
                 | RCC_PLLCFGR_PLLREN;
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    /* Flash latency 4 wait states for 120 MHz + prefetch + caches. */
    FLASH->ACR = FLASH_ACR_LATENCY(FLASH_LATENCY_WS)
               | FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN;

    /* Switch SYSCLK to PLL. */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW_MASK) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS_MASK) != (RCC_CFGR_SW_PLL << 2));

    /* Enable peripheral clocks. */
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_CRCEN;
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN
                  | RCC_AHB2ENR_GPIOCEN | RCC_AHB2ENR_GPIODEN;
    RCC->APB1ENR1 |= RCC_APB1ENR1_SPI2EN | RCC_APB1ENR1_I2C1EN
                   | RCC_APB1ENR1_UART4EN | RCC_APB1ENR1_TIM2EN
                   | RCC_APB1ENR1_RTCAPBEN | RCC_APB1ENR1_PWREN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN | RCC_APB2ENR_SPI3EN
                  | RCC_APB2ENR_SYSCFGEN | RCC_APB2ENR_USART1EN;

    /* Enable Cortex-M4 FPU (CP10, CP11 full access). */
    SCB->CPACR |= (0xFU << 20);

    /* Configure SysTick for 1 ms tick at 120 MHz. */
    SysTick_LOAD = 120000UL - 1;
    SysTick_VAL = 0;
    SysTick_CSR = SysTick_ENABLE | SysTick_TICKINT | SysTick_CLKSOURCE;
}

/* ===================================================================== */
/*  GPIO initialization                                                   */
/* ===================================================================== */

static void gpio_init(void)
{
    /* --- SPI1 pins: PA5(SCK), PA6(MISO), PA7(MOSI) — AF5 --- */
    GPIO_SET_MODE(GPIOA, SPI1_SCK_PIN, GPIO_MODE_AF);
    GPIO_SET_AF(GPIOA, SPI1_SCK_PIN, GPIO_AF5_SPI1);
    GPIO_SET_SPEED(GPIOA, SPI1_SCK_PIN, GPIO_SPEED_VHIGH);

    GPIO_SET_MODE(GPIOA, SPI1_MISO_PIN, GPIO_MODE_AF);
    GPIO_SET_AF(GPIOA, SPI1_MISO_PIN, GPIO_AF5_SPI1);

    GPIO_SET_MODE(GPIOA, SPI1_MOSI_PIN, GPIO_MODE_AF);
    GPIO_SET_AF(GPIOA, SPI1_MOSI_PIN, GPIO_AF5_SPI1);
    GPIO_SET_SPEED(GPIOA, SPI1_MOSI_PIN, GPIO_SPEED_VHIGH);

    /* --- ADS1298 control pins --- */
    GPIO_SET_MODE(ADS1298_CS0_PORT, ADS1298_CS0_PIN, GPIO_MODE_OUTPUT);
    GPIO_SET(ADS1298_CS0_PORT, ADS1298_CS0_PIN);  /* idle high */

    GPIO_SET_MODE(ADS1298_CS1_PORT, ADS1298_CS1_PIN, GPIO_MODE_OUTPUT);
    GPIO_SET(ADS1298_CS1_PORT, ADS1298_CS1_PIN);

    GPIO_SET_MODE(ADS1298_DRDY_PORT, ADS1298_DRDY_PIN, GPIO_MODE_INPUT);
    GPIO_SET_PUPD(ADS1298_DRDY_PORT, ADS1298_DRDY_PIN, GPIO_PUPD_PU);

    GPIO_SET_MODE(ADS1298_START_PORT, ADS1298_START_PIN, GPIO_MODE_OUTPUT);
    GPIO_RESET(ADS1298_START_PORT, ADS1298_START_PIN);

    GPIO_SET_MODE(ADS1298_RESET_PORT, ADS1298_RESET_PIN, GPIO_MODE_OUTPUT);
    GPIO_SET(ADS1298_RESET_PORT, ADS1298_RESET_PIN);

    GPIO_SET_MODE(ADS1298_PWDN_PORT, ADS1298_PWDN_PIN, GPIO_MODE_OUTPUT);
    GPIO_RESET(ADS1298_PWDN_PORT, ADS1298_PWDN_PIN);  /* powered down initially */

    /* --- SPI2 pins: PB13(SCK), PB14(MISO), PB15(MOSI) --- */
    GPIO_SET_MODE(GPIOB, SD_SCK_PIN, GPIO_MODE_AF);
    GPIO_SET_AF(GPIOB, SD_SCK_PIN, GPIO_AF5_SPI1);
    GPIO_SET_SPEED(GPIOB, SD_SCK_PIN, GPIO_SPEED_HIGH);

    GPIO_SET_MODE(GPIOB, SD_MISO_PIN, GPIO_MODE_AF);
    GPIO_SET_AF(GPIOB, SD_MISO_PIN, GPIO_AF5_SPI1);

    GPIO_SET_MODE(GPIOB, SD_MOSI_PIN, GPIO_MODE_AF);
    GPIO_SET_AF(GPIOB, SD_MOSI_PIN, GPIO_AF5_SPI1);
    GPIO_SET_SPEED(GPIOB, SD_MOSI_PIN, GPIO_SPEED_HIGH);

    GPIO_SET_MODE(SD_CS_PORT, SD_CS_PIN, GPIO_MODE_OUTPUT);
    GPIO_SET(SD_CS_PORT, SD_CS_PIN);  /* idle high */

    /* --- SPI3 pins: PC10(SCK), PC11(MISO), PC12(MOSI) — AF6 --- */
    GPIO_SET_MODE(GPIOC, LORA_SCK_PIN, GPIO_MODE_AF);
    GPIO_SET_AF(GPIOC, LORA_SCK_PIN, GPIO_AF6_SPI3);
    GPIO_SET_SPEED(GPIOC, LORA_SCK_PIN, GPIO_SPEED_HIGH);

    GPIO_SET_MODE(GPIOC, LORA_MISO_PIN, GPIO_MODE_AF);
    GPIO_SET_AF(GPIOC, LORA_MISO_PIN, GPIO_AF6_SPI3);

    GPIO_SET_MODE(GPIOC, LORA_MOSI_PIN, GPIO_MODE_AF);
    GPIO_SET_AF(GPIOC, LORA_MOSI_PIN, GPIO_AF6_SPI3);
    GPIO_SET_SPEED(GPIOC, LORA_MOSI_PIN, GPIO_SPEED_HIGH);

    GPIO_SET_MODE(LORA_CS_PORT, LORA_CS_PIN, GPIO_MODE_OUTPUT);
    GPIO_SET(LORA_CS_PORT, LORA_CS_PIN);

    GPIO_SET_MODE(LORA_BUSY_PORT, LORA_BUSY_PIN, GPIO_MODE_INPUT);
    GPIO_SET_MODE(LORA_DIO1_PORT, LORA_DIO1_PIN, GPIO_MODE_INPUT);
    GPIO_SET_MODE(LORA_NRST_PORT, LORA_NRST_PIN, GPIO_MODE_OUTPUT);
    GPIO_SET(LORA_NRST_PORT, LORA_NRST_PIN);

    /* --- I2C1 pins: PB6(SCL), PB7(SDA) — AF4 --- */
    GPIO_SET_MODE(GPIOB, I2C1_SCL_PIN, GPIO_MODE_AF);
    GPIO_SET_AF(GPIOB, I2C1_SCL_PIN, GPIO_AF4_I2C1);
    GPIO_SET_PUPD(GPIOB, I2C1_SCL_PIN, GPIO_PUPD_PU);

    GPIO_SET_MODE(GPIOB, I2C1_SDA_PIN, GPIO_MODE_AF);
    GPIO_SET_AF(GPIOB, I2C1_SDA_PIN, GPIO_AF4_I2C1);
    GPIO_SET_PUPD(GPIOB, I2C1_SDA_PIN, GPIO_PUPD_PU);

    /* --- UART4 debug: PA0(TX), PA1(RX) — AF6 --- */
    GPIO_SET_MODE(GPIOA, DEBUG_TX_PIN, GPIO_MODE_AF);
    GPIO_SET_AF(GPIOA, DEBUG_TX_PIN, GPIO_AF7_USART);
    GPIO_SET_MODE(GPIOA, DEBUG_RX_PIN, GPIO_MODE_AF);
    GPIO_SET_AF(GPIOA, DEBUG_RX_PIN, GPIO_AF7_USART);

    /* --- USB pins: PA11(DM), PA12(DP) — AF10 --- */
    GPIO_SET_MODE(GPIOA, USB_DM_PIN, GPIO_MODE_AF);
    GPIO_SET_AF(GPIOA, USB_DM_PIN, GPIO_AF10_USB);
    GPIO_SET_MODE(GPIOA, USB_DP_PIN, GPIO_MODE_AF);
    GPIO_SET_AF(GPIOA, USB_DP_PIN, GPIO_AF10_USB);

    /* --- DS18B20 1-Wire: PC0 — open-drain output --- */
    GPIO_SET_MODE(DS18B20_PORT, DS18B20_PIN, GPIO_MODE_OUTPUT);
    GPIOC->OTYPER |= (1U << DS18B20_PIN);  /* open-drain */
    GPIO_SET(DS18B20_PORT, DS18B20_PIN);   /* idle high (external PU) */

    /* --- Load switch enables: PC1, PC2, PC3 --- */
    GPIO_SET_MODE(LOADSW_ANA_PORT, LOADSW_ANA_PIN, GPIO_MODE_OUTPUT);
    GPIO_RESET(LOADSW_ANA_PORT, LOADSW_ANA_PIN);
    GPIO_SET_MODE(LOADSW_ENV_PORT, LOADSW_ENV_PIN, GPIO_MODE_OUTPUT);
    GPIO_RESET(LOADSW_ENV_PORT, LOADSW_ENV_PIN);
    GPIO_SET_MODE(LOADSW_SD_PORT, LOADSW_SD_PIN, GPIO_MODE_OUTPUT);
    GPIO_RESET(LOADSW_SD_PORT, LOADSW_SD_PIN);

    /* --- Status LEDs: PB4(green), PB5(red) --- */
    GPIO_SET_MODE(LED_STATUS_PORT, LED_STATUS_PIN, GPIO_MODE_OUTPUT);
    GPIO_RESET(LED_STATUS_PORT, LED_STATUS_PIN);
    GPIO_SET_MODE(LED_ERROR_PORT, LED_ERROR_PIN, GPIO_MODE_OUTPUT);
    GPIO_RESET(LED_ERROR_PORT, LED_ERROR_PIN);

    /* --- User button: PC14 — input with pull-up --- */
    GPIO_SET_MODE(BUTTON_PORT, BUTTON_PIN, GPIO_MODE_INPUT);
    GPIO_SET_PUPD(BUTTON_PORT, BUTTON_PIN, GPIO_PUPD_PU);
}

/* ===================================================================== */
/*  SPI initialization                                                     */
/* ===================================================================== */

static void spi1_init(void)
{
    /* Disable SPI before config. */
    SPI1->CR1 = 0;

    /* Master, CPOL=0, CPHA=0 (mode 0), baud = 120/32 = 3.75 MHz for ADS1298. */
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI
              | (SPI_BR_DIV32 << SPI_CR1_BR_SHIFT);

    /* 8-bit data, FRXTH for 8-bit RXNE threshold. */
    SPI1->CR2 = SPI_CR2_DS_8BIT | SPI_CR2_FRXTH | SPI_CR2_SSOE;

    /* Enable SPI. */
    SPI1->CR1 |= SPI_CR1_SPE;
}

static void spi2_init(void)
{
    SPI2->CR1 = 0;
    /* Master, mode 0, baud = 120/8 = 15 MHz for SD card. */
    SPI2->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI
              | (SPI_BR_DIV8 << SPI_CR1_BR_SHIFT);
    SPI2->CR2 = SPI_CR2_DS_8BIT | SPI_CR2_FRXTH | SPI_CR2_SSOE;
    SPI2->CR1 |= SPI_CR1_SPE;
}

static void spi3_init(void)
{
    SPI3->CR1 = 0;
    /* Master, mode 0, baud = 120/8 = 15 MHz for SX1262. */
    SPI3->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI
              | (SPI_BR_DIV8 << SPI_CR1_BR_SHIFT);
    SPI3->CR2 = SPI_CR2_DS_8BIT | SPI_CR2_FRXTH | SPI_CR2_SSOE;
    SPI3->CR1 |= SPI_CR1_SPE;
}

/* ===================================================================== */
/*  UART4 debug console                                                    */
/* ===================================================================== */

static void debug_uart_init(void)
{
    UART4->CR1 = 0;
    /* Baud rate: APB1 / BRR = 120 MHz / 115200 = 0x341 (approx). */
    UART4->BRR = (120000000UL + DEBUG_BAUD / 2) / DEBUG_BAUD;
    UART4->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

static void debug_print(const char *s)
{
    while (*s) {
        while (!(UART4->ISR & USART_ISR_TXE));
        UART4->TDR = (uint8_t)*s++;
    }
}

static void debug_print_u32(uint32_t val)
{
    char buf[12];
    int i = 10;
    buf[11] = '\0';
    if (val == 0) { debug_print("0"); return; }
    while (val && i >= 0) {
        buf[i--] = '0' + (val % 10);
        val /= 10;
    }
    debug_print(&buf[i + 1]);
}

/* ===================================================================== */
/*  I2C1 initialization (for SCD41 CO₂ sensor)                            */
/* ===================================================================== */

static void i2c1_init(void)
{
    I2C1->CR1 = 0;
    /* Timing for 100 kHz I2C at 120 MHz: PRESC=7, SCLL=0x97, SCLH=0x9B */
    I2C1->TIMINGR = (7U << 28) | (0x9BU << 8) | 0x97U;
    I2C1->CR1 = I2C_CR1_PE;
}

/* ===================================================================== */
/*  Button handling                                                       */
/* ===================================================================== */

static void button_poll(void)
{
    static uint8_t prev_state = 1;
    static uint32_t press_start = 0;
    uint8_t cur = GPIO_READ(BUTTON_PORT, BUTTON_PIN);

    if (prev_state == 1 && cur == 0) {
        /* Button pressed (falling edge). */
        press_start = g_tick_ms;
    } else if (prev_state == 0 && cur == 1) {
        /* Button released (rising edge). */
        uint32_t dur = g_tick_ms - press_start;
        if (dur >= 2000) {
            /* Long press: enter/exit calibration mode. */
            if (g_state == STATE_CALIBRATE) {
                g_state = STATE_IDLE;
                debug_print("[MYCO] Exit calibration mode\n");
            } else {
                g_state = STATE_CALIBRATE;
                debug_print("[MYCO] Enter calibration mode\n");
            }
        } else if (dur >= 50) {
            /* Short press: cycle active window. */
            if (g_active_window_ms == 10000) {
                g_active_window_ms = 30000;
                g_duty_cycle_pct = 15;
            } else if (g_active_window_ms == 30000) {
                g_active_window_ms = 60000;
                g_duty_cycle_pct = 30;
            } else {
                g_active_window_ms = 10000;
                g_duty_cycle_pct = 5;
            }
            g_sleep_window_ms = (g_active_window_ms * 100U / g_duty_cycle_pct)
                              - g_active_window_ms;
            debug_print("[MYCO] Active window: ");
            debug_print_u32(g_active_window_ms / 1000);
            debug_print("s\n");
        }
    }
    prev_state = cur;
}

/* ===================================================================== */
/*  Epoch accumulator                                                     */
/* ===================================================================== */

static void epoch_reset(void)
{
    memset(&g_epoch, 0, sizeof(g_epoch));
    g_epoch_spike_total = 0;
    g_epoch_propagation_total = 0;
    g_epoch_amplitude_sum = 0;
    g_epoch_isi_count = 0;
    g_epoch_start_tick = g_tick_ms;
    g_epoch_complete = 0;
}

static void epoch_add_spike(const spike_event_t *spike)
{
    g_epoch_spike_total++;
    g_epoch_amplitude_sum += (spike->amplitude_uv > 0)
                           ? (uint32_t)spike->amplitude_uv
                           : (uint32_t)(-spike->amplitude_uv);

    /* Track inter-spike intervals on this channel for CV calculation. */
    static uint32_t last_spike_time[ADS1298_TOTAL_CHANS] = {0};
    uint8_t ch = spike->channel;
    if (last_spike_time[ch] > 0) {
        uint32_t isi = spike->timestamp_ms - last_spike_time[ch];
        if (g_epoch_isi_count < 256) {
            g_epoch_isi_intervals[g_epoch_isi_count++] = isi;
        }
    }
    last_spike_time[ch] = spike->timestamp_ms;
}

static float epoch_compute_isi_cv(void)
{
    if (g_epoch_isi_count < 2) return 0.0f;

    /* Compute mean. */
    float mean = 0.0f;
    for (uint16_t i = 0; i < g_epoch_isi_count; i++) {
        mean += (float)g_epoch_isi_intervals[i];
    }
    mean /= g_epoch_isi_count;

    /* Compute standard deviation. */
    float var = 0.0f;
    for (uint16_t i = 0; i < g_epoch_isi_count; i++) {
        float d = (float)g_epoch_isi_intervals[i] - mean;
        var += d * d;
    }
    var /= g_epoch_isi_count;
    float std = 0.0f;
    /* Newton's method for sqrt (avoiding libm dependency). */
    if (var > 0.0f) {
        float x = var;
        for (int i = 0; i < 8; i++) {
            x = 0.5f * (x + var / x);
        }
        std = x;
    }
    return (mean > 0.0f) ? (std / mean) : 0.0f;
}

static void epoch_finalize(void)
{
    float duration_sec = (float)(g_tick_ms - g_epoch_start_tick) / 1000.0f;
    if (duration_sec < 1.0f) duration_sec = 1.0f;

    g_epoch.spike_rate = (uint8_t)(g_epoch_spike_total / duration_sec);
    g_epoch.propagation_count = (uint8_t)g_epoch_propagation_total;
    g_epoch.mean_amplitude_uv = (g_epoch_spike_total > 0)
        ? (uint16_t)(g_epoch_amplitude_sum / g_epoch_spike_total)
        : 0;
    g_epoch.isi_cv_x100 = (uint16_t)(epoch_compute_isi_cv() * 100.0f);
    g_epoch.soil_moisture = g_env.moisture_pct_x10;
    g_epoch.soil_temp_cx10 = g_env.temp_cx10;
    g_epoch.co2_ppm = g_env.co2_ppm;
    g_epoch.timestamp = 0;  /* RTC would fill this; placeholder for now */
    g_epoch_complete = 1;
}

/* ===================================================================== */
/*  State machine transitions                                             */
/* ===================================================================== */

static void enter_state(sys_state_t new_state)
{
    g_state = new_state;
    g_state_enter_time = g_tick_ms;
}

/* ===================================================================== */
/*  Main acquisition and processing cycle                                 */
/* ===================================================================== */

static void run_acquire_block(void)
{
    /* Read one 64-sample block from ADS1298 (polling mode). */
    if (ads1298_read_block(g_sample_buf, DSP_BLOCK_SIZE) == 0) {
        g_block_ready = 1;
    }
}

static void run_dsp_block(void)
{
    /* Process each channel through the DSP pipeline. */
    for (uint8_t ch = 0; ch < ADS1298_TOTAL_CHANS; ch++) {
        dsp_process_channel(ch,
                            g_sample_buf[ch],
                            g_filtered_buf[ch],
                            DSP_BLOCK_SIZE,
                            g_mains_freq_hz);
    }

    /* Detect spikes across all channels. */
    g_spike_count = 0;
    for (uint8_t ch = 0; ch < ADS1298_TOTAL_CHANS; ch++) {
        uint16_t detected = dsp_detect_spikes(ch,
                                               g_filtered_buf[ch],
                                               DSP_BLOCK_SIZE,
                                               &g_spikes[g_spike_count],
                                               64 - g_spike_count,
                                               g_tick_ms);
        g_spike_count += detected;
    }

    /* Sort spikes using template matching. */
    for (uint16_t i = 0; i < g_spike_count; i++) {
        spike_sort_classify(&g_spikes[i]);
        epoch_add_spike(&g_spikes[i]);
    }

    /* Compute cross-channel propagation events. */
    g_epoch_propagation_total += dsp_compute_propagation(
        g_filtered_buf, ADS1298_TOTAL_CHANS, DSP_BLOCK_SIZE,
        PROP_WINDOW_MS, PROP_CORR_THRESHOLD);

    g_block_ready = 0;
}

static void run_epoch_check(void)
{
    if (elapsed_ms(g_epoch_start_tick) >= (EPOCH_DURATION_SEC * 1000)) {
        epoch_finalize();
        enter_state(STATE_CLASSIFY);
    }
}

/* ===================================================================== */
/*  Main loop                                                             */
/* ===================================================================== */

int main(void)
{
    /* --- Initialization --- */
    clock_init();
    gpio_init();
    spi1_init();
    spi2_init();
    spi3_init();
    i2c1_init();
    debug_uart_init();

    debug_print("\n[MYCO] MycoMesh v" BOARD_VERSION " booting...\n");
    debug_print("[MYCO] Author: " BOARD_AUTHOR "\n");
    debug_print("[MYCO] MCU: STM32L4R9ZI @ 120 MHz\n");

    /* Initialize drivers. */
    power_init();
    debug_print("[MYCO] Power management initialized\n");

    /* Power up analog front end. */
    power_enable_analog();
    ads1298_init();
    debug_print("[MYCO] ADS1298 dual-ADC initialized (16 channels)\n");

    /* Initialize DSP. */
    dsp_init(g_mains_freq_hz, ADS1298_SAMPLE_RATE);
    debug_print("[MYCO] DSP pipeline initialized\n");

    /* Initialize spike sorter with empty templates. */
    spike_sort_init();
    debug_print("[MYCO] Spike sorter initialized\n");

    /* Initialize activity classifier. */
    classifier_init();
    debug_print("[MYCO] Activity classifier initialized\n");

    /* Initialize environmental sensors. */
    power_enable_env();
    env_sensors_init();
    debug_print("[MYCO] Environmental sensors initialized\n");

    /* Initialize SD card logging. */
    power_enable_sd();
    if (sdlog_init() == 0) {
        debug_print("[MYCO] SD card logging initialized\n");
    } else {
        debug_print("[MYCO] WARNING: SD card not detected\n");
        GPIO_SET(LED_ERROR_PORT, LED_ERROR_PIN);
    }

    /* Initialize LoRaWAN. */
    lorawan_init(g_lora_region);
    debug_print("[MYCO] LoRaWAN initialized\n");

    /* Initialize USB protocol handler. */
    protocol_init();
    debug_print("[MYCO] USB protocol handler initialized\n");

    /* Enable analog front end for initial acquisition. */
    ads1298_start();
    debug_print("[MYCO] ADS1298 acquisition started\n");

    /* Enter IDLE state. */
    enter_state(STATE_IDLE);
    epoch_reset();
    debug_print("[MYCO] Entering main loop\n");

    /* Status LED on. */
    GPIO_SET(LED_STATUS_PORT, LED_STATUS_PIN);

    /* --- Main super-loop --- */
    uint32_t last_env_read = 0;
    uint32_t last_telemetry = 0;
    uint32_t last_health = 0;

    while (1) {
        button_poll();
        protocol_poll();

        switch (g_state) {
        case STATE_INIT:
            /* Should not happen, but recover. */
            enter_state(STATE_IDLE);
            break;

        case STATE_IDLE:
            /* Check if it's time to start an active window. */
            if (g_usb_connected) {
                enter_state(STATE_USB_CONNECTED);
                break;
            }
            /* Immediately begin acquisition (IDLE is just a brief checkpoint). */
            enter_state(STATE_ACQUIRE);
            break;

        case STATE_ACQUIRE:
            /* Collect samples from ADS1298. */
            if (!g_block_ready) {
                run_acquire_block();
            }
            if (g_block_ready) {
                run_dsp_block();
            }

            /* Check if epoch is complete. */
            run_epoch_check();

            /* Check if active window has elapsed. */
            if (elapsed_ms(g_state_enter_time) >= g_active_window_ms
                && !g_usb_connected) {
                ads1298_stop();
                enter_state(STATE_LOG);
            }
            break;

        case STATE_CLASSIFY:
            /* Classify the completed epoch. */
            g_epoch.class_label = classifier_classify(&g_epoch);
            debug_print("[MYCO] Epoch classified: class=");
            debug_print_u32(g_epoch.class_label);
            debug_print(" spikes=");
            debug_print_u32(g_epoch_spike_total);
            debug_print("\n");

            /* Enter LOG state to persist the epoch. */
            enter_state(STATE_LOG);
            break;

        case STATE_LOG:
            /* Log raw block data to SD. */
            if (g_block_ready || g_spike_count > 0) {
                sdlog_write_block(g_filtered_buf, DSP_BLOCK_SIZE, g_tick_ms);
            }
            /* Log epoch summary if complete. */
            if (g_epoch_complete) {
                sdlog_write_epoch(&g_epoch);
                epoch_reset();
            }
            enter_state(STATE_TELEMETRY);
            break;

        case STATE_TELEMETRY:
            /* Send LoRaWAN telemetry if enough time has passed. */
            if (g_epoch_complete && elapsed_ms(last_telemetry) >= 60000) {
                if (lorawan_send(LORAWAN_PORT_STATUS,
                                 (uint8_t *)&g_epoch,
                                 sizeof(g_epoch)) == 0) {
                    debug_print("[MYCO] LoRaWAN status uplink sent\n");
                    last_telemetry = g_tick_ms;
                }
            }

            /* Send health packet every 10 minutes. */
            if (elapsed_ms(last_health) >= 600000) {
                uint16_t batt_mv = power_read_battery_mv();
                int16_t temp_cx10 = g_env.temp_cx10;
                uint8_t health[8];
                health[0] = (uint8_t)(batt_mv >> 8);
                health[1] = (uint8_t)(batt_mv & 0xFF);
                health[2] = (uint8_t)(temp_cx10 >> 8);
                health[3] = (uint8_t)(temp_cx10 & 0xFF);
                health[4] = g_epoch.class_label;
                health[5] = g_duty_cycle_pct;
                health[6] = (uint8_t)(g_samples_collected >> 8);
                health[7] = (uint8_t)(g_samples_collected & 0xFF);
                lorawan_send(LORAWAN_PORT_HEALTH, health, sizeof(health));
                last_health = g_tick_ms;
            }

            /* Send alarm if stress/compete detected. */
            if (g_epoch_complete
                && (g_epoch.class_label == CLASS_STRESS
                 || g_epoch.class_label == CLASS_COMPETE)) {
                uint8_t alarm[8];
                alarm[0] = g_epoch.class_label;
                alarm[1] = 0;  /* node ID (would be from config) */
                uint32_t ts = g_epoch.timestamp;
                memcpy(&alarm[2], &ts, 4);
                alarm[6] = (uint8_t)(g_epoch.co2_ppm >> 8);
                alarm[7] = (uint8_t)(g_epoch.co2_ppm & 0xFF);
                lorawan_send(LORAWAN_PORT_ALARM, alarm, sizeof(alarm));
                debug_print("[MYCO] ALARM: stress/compete detected\n");
            }

            /* Read environmental sensors every 30 seconds. */
            if (elapsed_ms(last_env_read) >= 30000) {
                env_sensors_read_all(&g_env);
                last_env_read = g_tick_ms;
            }

            /* Enter sleep if not USB-connected. */
            if (!g_usb_connected) {
                enter_state(STATE_SLEEP);
            } else {
                enter_state(STATE_USB_CONNECTED);
            }
            break;

        case STATE_SLEEP:
            /* Power down analog and environmental sensors. */
            ads1298_power_down();
            power_disable_analog();
            power_disable_env();

            /* Blink status LED to indicate sleep entry. */
            GPIO_RESET(LED_STATUS_PORT, LED_STATUS_PIN);
            for (volatile int i = 0; i < 100000; i++);
            GPIO_SET(LED_STATUS_PORT, LED_STATUS_PIN);

            debug_print("[MYCO] Entering STOP2 sleep for ");
            debug_print_u32(g_sleep_window_ms / 1000);
            debug_print("s\n");

            /* Enter STOP2 mode. RTC wake-up will resume execution. */
            power_enter_stop2(g_sleep_window_ms);

            /* --- Woke up from STOP2 --- */
            debug_print("[MYCO] Woke from sleep\n");

            /* Re-initialize clocks (STOP2 loses PLL). */
            clock_init();
            spi1_init();
            spi2_init();
            spi3_init();

            /* Power up analog front end. */
            power_enable_analog();
            ads1298_wake();
            ads1298_start();

            /* Power up environmental sensors. */
            power_enable_env();

            /* Reset epoch if we slept through it. */
            if (elapsed_ms(g_epoch_start_tick) > (EPOCH_DURATION_SEC * 10000)) {
                epoch_reset();
            }

            enter_state(STATE_ACQUIRE);
            break;

        case STATE_CALIBRATE:
            /* Calibration mode: stream raw ADC data to USB for electrode
               impedance measurement and offset calibration. */
            ads1298_read_block(g_sample_buf, DSP_BLOCK_SIZE);

            /* Send raw calibration data over USB. */
            protocol_send_calibration(g_sample_buf[0], DSP_BLOCK_SIZE);

            /* Check electrode lead-off status. */
            uint8_t lead_off[ADS1298_TOTAL_CHANS];
            ads1298_check_lead_off(lead_off);
            for (uint8_t ch = 0; ch < ADS1298_TOTAL_CHANS; ch++) {
                if (lead_off[ch]) {
                    GPIO_SET(LED_ERROR_PORT, LED_ERROR_PIN);
                    break;
                }
            }

            /* Back to IDLE on button release (handled by button_poll). */
            break;

        case STATE_USB_CONNECTED:
            /* USB is connected — stay active for config/download. */
            /* Keep reading data but don't sleep. */
            if (!g_block_ready) {
                run_acquire_block();
            }
            if (g_block_ready) {
                run_dsp_block();
            }
            run_epoch_check();

            if (!g_usb_connected) {
                enter_state(STATE_IDLE);
            }
            break;
        }

        /* Yield to SysTick — ensure at least one tick elapses between
           iterations to prevent starvation of the button/protocol poll. */
        uint32_t t0 = g_tick_ms;
        while (g_tick_ms == t0) {
            __asm volatile ("wfe");  /* Wait For Event (low power idle) */
        }
    }

    return 0;  /* unreachable */
}

/* ===================================================================== */
/*  USB connection detection (called from USB ISR context)               */
/* ===================================================================== */

void myco_usb_connect_callback(void)
{
    g_usb_connected = 1;
    debug_print("[MYCO] USB connected — staying active\n");
}

void myco_usb_disconnect_callback(void)
{
    g_usb_connected = 0;
    debug_print("[MYCO] USB disconnected\n");
}

/* ===================================================================== */
/*  Configuration update (called from protocol handler)                   */
/* ===================================================================== */

void myco_apply_config(uint8_t duty_cycle, uint8_t mains_hz, uint8_t region)
{
    if (duty_cycle > 0 && duty_cycle <= 100) {
        g_duty_cycle_pct = duty_cycle;
        g_active_window_ms = ACTIVE_WINDOW_SEC * 1000;
        g_sleep_window_ms = (g_active_window_ms * 100U / g_duty_cycle_pct)
                          - g_active_window_ms;
    }
    if (mains_hz == 50 || mains_hz == 60) {
        g_mains_freq_hz = mains_hz;
        dsp_set_notch_freq(mains_hz);
    }
    if (region <= 1) {
        g_lora_region = region;
        lorawan_set_region(region);
    }

    debug_print("[MYCO] Config updated: duty=");
    debug_print_u32(g_duty_cycle_pct);
    debug_print("% mains=");
    debug_print_u32(g_mains_freq_hz);
    debug_print("Hz region=");
    debug_print_u32(g_lora_region);
    debug_print("\n");
}

/* ===================================================================== */
/*  Interrupt vector table (minimal — placed at 0x08000000 by linker)    */
/* ===================================================================== */

extern uint32_t _estack;

__attribute__((section(".isr_vector")))
void (* const g_pfnVectors[])(void) = {
    (void (*)(void))(&_estack),    /* Initial stack pointer */
    0,                              /* Reset handler (set by linker/startup) */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* Reserved + NMI + HardFault */
    0, 0, 0, 0, 0,                  /* MemManage, BusFault, UsageFault, */
    0, 0, 0, 0,                    /* SVCall, DebugMon, Reserved, PendSV */
    SysTick_Handler,                /* SysTick */
    /* External interrupts (subset — full table would cover all 83) */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* IRQ 0–9 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* IRQ 10–19 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* IRQ 20–29 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* IRQ 30–39 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* IRQ 40–49 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* IRQ 50–59 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* IRQ 60–69 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* IRQ 70–79 */
    0, 0,                           /* IRQ 80–81 */
};