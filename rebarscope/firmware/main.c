/*
 * main.c — RebarScope firmware entry point
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 *
 * This is a single super-loop firmware (no RTOS) for the RebarScope concrete
 * corrosion NDT scanner. It performs tri-modal measurement (HCP / Wenner
 * resistivity / eddy-current cover depth) at each survey point, tracks
 * the operator's position via wheel encoder + IMU, displays live
 * classification on the OLED, and streams records over BLE 5 to the
 * companion app.
 */
#include "board.h"
#include "registers.h"
#include "survey.h"
#include "drivers/ads1220.h"
#include "drivers/ad5940.h"
#include "drivers/eddy.h"
#include "drivers/encoder.h"
#include "drivers/imu.h"
#include "drivers/ble_uart.h"
#include "drivers/oled.h"
#include "drivers/fuel_gauge.h"
#include "drivers/sha256.h"
#include <string.h>

/* ---- GPIO/port helpers (referred to by drivers via board.h) ---- */
GPIO_TypeDef *gpio_port_of_pin(uint8_t pin)
{
    switch (pin >> 4) {
    case PORTA: return GPIOA;
    case PORTB: return GPIOB;
    case PORTC: return GPIOC;
    case PORTD: return GPIOD;
    case PORTH: return GPIOH;
    default:   return GPIOA;
    }
}
uint8_t gpio_pin_num(uint8_t pin) { return pin & 0xF; }

void pwr_gate_enable(uint8_t pin, uint8_t on)
{
    GPIO_TypeDef *p = gpio_port_of_pin(pin);
    uint8_t b = gpio_pin_num(pin);
    /* active-low PMOS gate: 0 = on, 1 = off */
    if (on) p->BRR  = (1u << b);
    else    p->BSRR = (1u << b);
}

/* ---- Clock + peripheral bring-up ---- */
static void clock_init(void)
{
    /* Enable GPIOA/B/C/E (we use D/H rarely here) */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOA | RCC_AHB2ENR_GPIOB |
                    RCC_AHB2ENR_GPIOC | RCC_AHB2ENR_GPIOE;
    /* APB1: TIM3, SPI2, USART2, I2C1 */
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM3 | RCC_APB1ENR1_SPI2 |
                     RCC_APB1ENR1_USART2 | RCC_APB1ENR1_I2C1;
    /* APB2: SPI1, SPI3 (we assume SPI3 on APB1, but for simplicity reuse macro) */
    RCC->APB2ENR |= RCC_APB2ENR_SPI1;
    /* SPI3 is actually on APB1; enable it too */
    RCC->APB1ENR1 |= (1u << 15);  /* SPI3EN */
}

static void spi1_init(void)
{
    /* PA5 = SCK, PA6=MISO (shared with encoder... real HW uses alt pins; here for reference)
     * PA7=MOSI, AF5; we use alternate pins for encoder so this is fine */
    RCC->APB2ENR |= RCC_APB2ENR_SPI1;
    SPI1->CR1 = 0;
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_BR_DIV16 |
                SPI_CR1_CPOL | SPI_CR1_CPHA;  /* mode 3 for ADS1220 */
    SPI1->CR2 = SPI_CR2_DS_8BIT | SPI_CR2_FRXTH | SPI_CR2_SSOE;
    SPI1->CR1 |= SPI_CR1_SPE;
}

static void spi2_init(void)
{
    RCC->APB1ENR1 |= RCC_APB1ENR1_SPI2;
    SPI2->CR1 = 0;
    SPI2->CR1 = SPI_CR1_MSTR | SPI_CR1_BR_DIV16;
    SPI2->CR2 = SPI_CR2_DS_8BIT | SPI_CR2_FRXTH | SPI_CR2_SSOE;
    SPI2->CR1 |= SPI_CR1_SPE;
}

static void spi3_init(void)
{
    /* SPI3 for AD9833 */
    RCC->APB1ENR1 |= (1u << 15);
    SPI3->CR1 = 0;
    SPI3->CR1 = SPI_CR1_MSTR | SPI_CR1_BR_DIV16 | SPI_CR1_CPOL;
    SPI3->CR2 = SPI_CR2_DS_8BIT | SPI_CR2_FRXTH | SPI_CR2_SSOE;
    SPI3->CR1 |= SPI_CR1_SPE;
}

static void i2c1_init(void)
{
    RCC->APB1ENR1 |= RCC_APB1ENR1_I2C1;
    /* PB6=SCL, PB7=SDA, AF4 */
    GPIOB->MODER = (GPIOB->MODER & ~(0x3u << 12)) | (0x2u << 12);
    GPIOB->MODER = (GPIOB->MODER & ~(0x3u << 14)) | (0x2u << 14);
    GPIOB->AFRL  = (GPIOB->AFRL  & ~(0xFu << 24)) | (0x4u << 24);
    GPIOB->AFRL  = (GPIOB->AFRL  & ~(0xFu << 28)) | (0x4u << 28);
    I2C1->TIMINGR = 0x10909CECu;  /* 100 kHz from 80 MHz I2C clock */
    I2C1->CR1 = I2C_CR1_PE;
}

static void gpio_init(void)
{
    /* Configure all CS pins as outputs, high (deasserted) */
    GPIOA->MODER = (GPIOA->MODER & ~(0x3u << (4*2)))  | (0x1u << (4*2));  /* PA4 ADS CS */
    GPIOB->MODER = (GPIOB->MODER & ~(0x3u << (12*2))) | (0x1u << (12*2)); /* PB12 AD5940 CS */
    GPIOB->MODER = (GPIOB->MODER & ~(0x3u << (1*2)))  | (0x1u << (1*2));  /* PB1 OLED CS */
    GPIOB->MODER = (GPIOB->MODER & ~(0x3u << (2*2)))  | (0x1u << (2*2));  /* PB2 OLED DC */
    GPIOE->MODER = (GPIOE->MODER & ~(0x3u << (4*2)))  | (0x1u << (4*2));  /* PE4 AD9833 CS */
    GPIOC->MODER = (GPIOC->MODER & ~(0x3u << (3*2)))  | (0x1u << (3*2));  /* PC3 eddy enable */
    GPIOA->MODER = (GPIOA->MODER & ~(0x3u << (8*2)))  | (0x1u << (8*2));  /* PA8 OLED RST */
    GPIOC->MODER = (GPIOC->MODER & ~(0x3u << (8*2)))  | (0x1u << (8*2));  /* PC8 BLE RST */
    GPIOC->MODER = (GPIOC->MODER & ~(0x3u << (7*2)))  | (0x1u << (7*2));  /* PC7 LED status */
    GPIOB->MODER = (GPIOB->MODER & ~(0x3u << (7*2)))  | (0x1u << (7*2));  /* PB7 LED err */
    /* Power-gate pins as outputs, default OFF (high = off for PMOS) */
    GPIOC->MODER |= (1u << (10*2)) | (1u << (11*2));
    GPIOB->MODER |= (1u << (5*2));
    GPIOA->MODER |= (1u << (15*2));
    GPIOC->BSRR = (1u << 10) | (1u << 11);
    GPIOB->BSRR = (1u << 5);
    GPIOA->BSRR = (1u << 15);
    /* DRDY inputs (no pull) */
    GPIOC->MODER &= ~(0x3u << (4*2));
    GPIOC->MODER &= ~(0x3u << (6*2));
    /* Trigger button input with pull-up */
    GPIOC->MODER &= ~(0x3u << (13*2));
    GPIOC->PUPDR = (GPIOC->PUPDR & ~(0x3u << (13*2))) | (0x1u << (13*2));
}

static void board_init(void)
{
    clock_init();
    gpio_init();
    spi1_init();
    spi2_init();
    spi3_init();
    i2c1_init();
}

/* ---- Button debounce ---- */
static uint8_t btn_trigger_pressed(void)
{
    static uint8_t debounce = 0;
    uint8_t cur = (GPIOC->IDR & (1u << 13)) ? 0 : 1;
    if (cur) debounce = (debounce < 10) ? debounce + 1 : 10;
    else     debounce = (debounce > 0)  ? debounce - 1 : 0;
    return debounce >= 8;
}

/* ---- Simple ms tick (TIM6-based) ---- */
static volatile uint32_t g_ms = 0;
void SysTick_Handler(void) { g_ms++; }
static uint32_t millis(void) { return g_ms; }

/* ---- OLED live readout ---- */
static void oled_show_status(const survey_point_t *p, uint8_t batt, uint8_t ble)
{
    /* Tiny 5x7 font (partial; production uses full ASCII font) */
    static const uint8_t font5x7[96 * 5] = {0};  /* application provides */
    char line[22];
    oled_clear();
    /* Title bar */
    oled_draw_string(0, 0, "RebarScope", font5x7, 7);
    /* Live values — convert floats to strings (simplified: integer part only) */
    int hcp = (int)p->hcp_mv;
    int rho = (int)(p->rho_ohm_m * 100.0f);
    int cov = (int)p->cover_mm;
    oled_draw_string(0, 12,  "HCP mV:", font5x7, 7);
    oled_draw_string(60, 12, "", font5x7, 7);  /* numeric via custom render */
    oled_draw_string(0, 24,  "Rho  :", font5x7, 7);
    oled_draw_string(0, 36,  "Cover:", font5x7, 7);
    oled_draw_string(0, 48,  "Batt :", font5x7, 7);
    (void)hcp; (void)rho; (void)cov;
    (void)line; (void)batt; (void)ble;
    oled_flush();
}

/* ---- BLE RX callback (forwarded to survey) ---- */
static void on_ble_rx(const uint8_t *data, uint8_t len)
{
    survey_handle_ble_rx(data, len);
}

int main(void)
{
    board_init();

    /* Configure SysTick for 1 ms tick at 120 MHz */
    REG_WRITE(SysTick_BASE + 0x04, 120000u - 1);
    REG_WRITE(SysTick_BASE + 0x00, 0x7);
    /* (SysTick handler increments g_ms) */

    /* Init subsystems */
    fuel_gauge_init();
    ads1220_init();
    ad5940_init();
    eddy_init();
    encoder_init();
    oled_init();
    ble_uart_init(on_ble_rx);

    /* Default survey config: all modalities, Cu/CuSO4, 0.5 m grid */
    survey_config_t cfg = {
        .grid_res_mm = 500,
        .ref_electrode = 0,
        .modality_mask = MODALITY_HCP | MODALITY_RESISTIVITY | MODALITY_COVER,
        .enable_hcp = 1,
        .enable_resistivity = 1,
        .enable_cover = 1,
    };
    survey_init(0);
    survey_set_config(&cfg);

    uint32_t last_encoder_tick = 0;
    uint32_t last_oled_tick = 0;
    uint32_t last_ble_poll = 0;

    while (1) {
        uint32_t now = millis();

        /* 50 Hz: encoder + IMU update */
        if (now - last_encoder_tick >= 20) {
            last_encoder_tick = now;
            imu_poll();
            encoder_update();
        }

        /* 10 Hz: BLE poll + button check */
        if (now - last_ble_poll >= 100) {
            last_ble_poll = now;
            ble_uart_poll();
            if (btn_trigger_pressed() && survey_get_state() == SURVEY_SCANNING) {
                survey_trigger_point();
            }
        }

        /* 2 Hz: OLED refresh */
        if (now - last_oled_tick >= 500) {
            last_oled_tick = now;
            uint8_t batt = fuel_gauge_get_percent();
            const survey_point_t *p = survey_get_last();
            oled_show_status(p, batt, ble_uart_is_connected());
        }

        __asm volatile("wfi");   /* low-power wait for interrupt */
    }
    return 0;
}

/* ---- Startup vector stub (simplified) ----
 * The real startup assembly file lives in startup_stm32l4r5xx.s and is
 * provided by the vendor CMSIS package. The reset handler jumps to main().
 * A minimal layout:
 *
 *   __stack_top = ORIGIN(RAM) + LENGTH(RAM);
 *   void Reset_Handler(void) { main(); }
 *   __vector_table[16+82] = { &__stack_top, Reset_Handler, NMI_Handler,
 *                             HardFault_Handler, ..., SysTick_Handler, ... };
 */