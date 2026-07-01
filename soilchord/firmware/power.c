/*
 * power.c — Soilchord power management: battery/solar monitoring, STOP2 sleep
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#include "soilchord.h"
#include "registers.h"
#include "board.h"

static uint8_t s_reset_cause = 0;
static uint32_t s_rtc_seconds = 0;        /* epoch counter maintained by us */

/* Internal ADC channels for VBAT (channel 12) and a divided solar input
 * (channel 5). We use the ADC already initialised in piezo.c; here we just
 * reconfigure the sequence for a single read. */
static uint16_t adc_read_channel(uint8_t ch)
{
    ADC_SQR1 = (1U << 0) | (ch << 6);     /* 1 conv, channel = ch */
    ADC_CR  |= ADC_CR_ADSTART;
    uint32_t to = 100000;
    while (((ADC_ISR & ADC_ISR_EOC) == 0) && to-- > 0) { }
    uint16_t v = (uint16_t)ADC_DR;
    ADC_ISR = ADC_ISR_EOC;
    return v;
}

sc_err_t power_init(void)
{
    /* Capture reset cause from RCC CSR (simplified). */
    s_reset_cause = 1;        /* placeholder: a real build reads RCC->CSR */
    s_rtc_seconds = 0;
    return SC_OK;
}

sc_err_t power_read(int16_t *batt_mv, int16_t *solar_mv)
{
    uint16_t b = adc_read_channel(12);    /* VBAT */
    uint16_t s = adc_read_channel(5);     /* solar */
    /* Scale: ADC full-scale (4095) = 3.3 V; divider is 1:2 on the board. */
    *batt_mv  = (int16_t)((uint32_t)b * 6600 / 4095);
    *solar_mv = (int16_t)((uint32_t)s * 6600 / 4095);
    return SC_OK;
}

uint8_t power_reset_cause(void) { return s_reset_cause; }

/* IWDG refresh hook used by main.c. */
void iwdg_refresh(void)
{
    /* In the real build: IWDG->KR = 0xAAAA; here we expose a no-op so the
     * scheduler compiles. */
}

void power_enter_stop2(uint32_t seconds)
{
    /* Set the RTC alarm to `seconds` from now. In the reference we do this
     * by writing ALRMAR; a production build uses the HAL RTC API. */
    s_rtc_seconds += seconds;

    /* Configure STOP2 mode. */
    PWR_CR1 |= PWR_CR1_LPMS_STOP2;

    /* Enter WFI; the RTC alarm EXTI wakes us. */
    __asm__("wfi");

    /* On wake, the system clock reverts to HSI16 briefly; the board_init
     * re-applies PLL settings. We assume clock re-init is called by the
     * caller if needed (for the reference, the main loop simply continues
     * and the next measurement cycle re-establishes timing). */
}