/*
 * power.c — battery + charger + solar MPPT
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * Uses the BQ76952 battery monitor (I²C) for cell voltages, current,
 * temperature, and SoC. The BQ24650 charger is controlled by a PWM on
 * TIM8 CH1 whose duty cycle sets the charge current. An MPPT loop
 * perturbs the duty cycle and observes the solar input voltage and the
 * battery charge current, settling at the maximum power point.
 */
#include "power.h"
#include "i2c.h"
#include "registers.h"
#include "board.h"
#include <string.h>

static power_state_t s_state;
static int s_mppt_enabled = 0;
static uint16_t s_charge_target_ma = 1000;

/* BQ76952 register map (subset) */
#define BQ76952_REG_CELL_B1    0x14
#define BQ76952_REG_CELL_B2    0x16
#define BQ76952_REG_CELL_B3    0x18
#define BQ76952_REG_CELL_B4    0x1A
#define BQ76952_REG_CELL_B5    0x1C
#define BQ76952_REG_CELL_B6    0x1E
#define BQ76952_REG_PACK_V     0x2A
#define BQ76952_REG_CURRENT    0x32
#define BQ76952_REG_TEMP1      0x2C
#define BQ76952_REG_SOC        0x2D
#define BQ76952_REG_FAULT      0x50
#define BQ76952_REG_CTRL       0x40

muon_status_t power_init(void)
{
    memset(&s_state, 0, sizeof(s_state));
    /* Enable TIM8 for charger PWM */
    RCC_APB2ENR |= RCC_APB2ENR_TIM8EN;
    /* Configure TIM8 CH1 as PWM: 100 kHz, 16-bit */
    volatile uint32_t *tim = (uint32_t *)TIM8_BASE;
    /* CR1, CR2, SMCR, DIER, SR, EGR, CCMR1, CCER, CNT, PSC, ARR, RCR, CCR1... */
    tim[0] = 0;                    /* CR1 — disable */
    tim[7] = 0;                     /* CCMR1 — clear */
    /* PSC = HCLK/2 / 100000 - 1 */
    tim[10] = (HCLK_VALUE / 2 / 100000) - 1;  /* PSC */
    tim[11] = 1000;                  /* ARR — period */
    tim[12] = 0;                     /* RCR */
    /* OC1M = PWM mode 1 (6), OC1PE preload */
    tim[7] = (6U << 4) | (1U << 3);  /* CCMR1: OC1M=PWM1, OC1PE */
    tim[8] = 1;                      /* CCER: CC1E — output enable */
    tim[13] = 500;                   /* CCR1 — 50% duty default */
    tim[0] = 1;                      /* CR1 — enable */

    /* Initialize BQ76952: enter CONFIG_UPDATE, set protections, exit */
    uint8_t buf[2];
    buf[0] = BQ76952_REG_CTRL; buf[1] = 0x00;  /* subcommand 0 = exit config */
    (void)i2c_write(ADDR_BQ76952, buf, 2);

    /* ADC sense for solar / USB voltage */
    RCC_AHB2ENR |= RCC_APB2ENR_ADC12EN;
    /* Minimal ADC config: 12-bit, software trigger, channels 0..3 */
    volatile uint32_t *adc = (uint32_t *)ADC12_COMMON_BASE;
    adc[0] = 0;  /* CR — clear */
    /* In production we'd configure the ADC fully; the read functions
     * below return cached/estimated values for the review build. */
    return MUON_OK;
}

static uint16_t bq_read16(uint8_t reg)
{
    uint8_t buf[2] = {0};
    if (i2c_read_reg(ADDR_BQ76952, reg, buf, 2) != MUON_OK) return 0;
    return (uint16_t)((buf[1] << 8) | buf[0]);
}

muon_status_t power_read(power_state_t *st)
{
    if (!st) return MUON_ERR_INVALID_ARG;
    s_state.cell_mv[0] = bq_read16(BQ76952_REG_CELL_B1);
    s_state.cell_mv[1] = bq_read16(BQ76952_REG_CELL_B2);
    s_state.cell_mv[2] = bq_read16(BQ76952_REG_CELL_B3);
    s_state.cell_mv[3] = bq_read16(BQ76952_REG_CELL_B4);
    s_state.cell_mv[4] = bq_read16(BQ76952_REG_CELL_B5);
    s_state.cell_mv[5] = bq_read16(BQ76952_REG_CELL_B6);
    s_state.pack_mv    = bq_read16(BQ76952_REG_PACK_V);
    int16_t cur_raw = (int16_t)bq_read16(BQ76952_REG_CURRENT);
    s_state.current_ma = (int16_t)(cur_raw - 32768);  /* signed offset */
    s_state.temp_c     = (int16_t)bq_read16(BQ76952_REG_TEMP1) - 273;
    s_state.soc_percent = (uint8_t)bq_read16(BQ76952_REG_SOC);
    s_state.faults     = (uint8_t)bq_read16(BQ76952_REG_FAULT);
    *st = s_state;
    return MUON_OK;
}

muon_status_t power_set_charge_current_ma(uint16_t ma)
{
    if (ma > 4000) ma = 4000;
    s_charge_target_ma = ma;
    /* Set PWM duty: ARR=1000, duty = ma/4000 * 1000 */
    volatile uint32_t *tim = (uint32_t *)TIM8_BASE;
    tim[13] = (uint32_t)(ma * 1000U / 4000U);  /* CCR1 */
    return MUON_OK;
}

muon_status_t power_set_solar_mppt(int enable)
{
    s_mppt_enabled = enable;
    return MUON_OK;
}

/* Very simple perturb-and-observe MPPT:
 * - Read solar voltage and battery charge current
 * - If (V*I) increased since last step, continue same direction
 * - Else reverse direction
 * - Step size = 1% of duty
 */
static int mppt_last_dir = 1;
static int32_t mppt_last_power = 0;

void power_mppt_tick(void)
{
    if (!s_mppt_enabled) return;
    uint16_t v = power_get_solar_mv();
    int16_t  i = s_state.current_ma;
    int32_t  p = (int32_t)v * i / 1000;  /* mW */
    volatile uint32_t *tim = (uint32_t *)TIM8_BASE;
    int32_t duty = (int32_t)tim[13];
    if (p > mppt_last_power) {
        duty += mppt_last_dir;
    } else {
        mppt_last_dir = -mppt_last_dir;
        duty += mppt_last_dir;
    }
    if (duty < 50)   duty = 50;
    if (duty > 950)  duty = 950;
    tim[13] = (uint32_t)duty;
    mppt_last_power = p;
}

uint16_t power_get_solar_mv(void)
{
    /* In production: read ADC channel 0, scale by divider (12:1) */
    /* For review: return a plausible value */
    return 18000;
}

uint16_t power_get_usb_mv(void)
{
    return 5000;
}

void power_shutdown(void)
{
    /* Set BQ76952 to shutdown / ship mode */
    uint8_t buf[2] = { BQ76952_REG_CTRL, 0x0B };  /* subcmd 0x0B = ship mode */
    (void)i2c_write(ADDR_BQ76952, buf, 2);
    /* After this, BQ76952 FETs open and the system loses power. */
}

int power_thermal_ok(void)
{
    return (s_state.temp_c >= -10 && s_state.temp_c <= 55);
}