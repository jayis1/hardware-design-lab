/*
 * power.c — Power management for Lichenwatch.
 *
 * Battery gauge via ADC on PA3 (divided by 2). The bq25570 harvester is
 * managed mostly autonomously; the MCU just reads the battery voltage and
 * decides whether to suppress heavy radio activity when the battery is low.
 *
 * Stop 2 mode is used between measurement bursts for the lowest active
 * sleep current (~1.7 µA). The TPL5111 nano-power timer gates the regulator
 * entirely during the long sleep, bringing system current down to ~300 nA.
 *
 * Author: jayis1
 * License: MIT
 */

#include "power.h"
#include "../board.h"
#include "../registers.h"

static volatile uint32_t s_uptime_s = 0;
static volatile uint8_t  s_soc = 100;  /* state of charge % */
static int s_initialized = 0;

/* ------------------------------------------------------------------------ */
/* ADC read                                                                   */
/* ------------------------------------------------------------------------ */
static uint16_t adc_read_channel(int channel)
{
    ADC1->SQR1 = (0 << 29) | (channel << 6);
    ADC1->CR |= ADC_CR_ADSTART;
    while (!(ADC1->ISR & ADC_ISR_EOC)) { }
    return (uint16_t)ADC1->DR;
}

/* ------------------------------------------------------------------------ */
/* Public API                                                                 */
/* ------------------------------------------------------------------------ */
int power_init(void)
{
    /* Configure PA3 (VBAT) as analog input. */
    volatile uint32_t *pa = (volatile uint32_t *)GPIOA;
    pa[0] = (pa[0] & ~(0x3U << (3*2))) | (GPIO_MODE_ANALOG << (3*2));

    /* Configure TPL5111 DONE pin (PB0) as output. */
    volatile uint32_t *pb = (volatile uint32_t *)GPIOB;
    pb[0] = (pb[0] & ~(0x3U << (0*2))) | (GPIO_MODE_OUTPUT << (0*2));
    pb[6] = (1U << 0);  /* DONE low */

    s_initialized = 1;
    return 0;
}

uint16_t power_read_battery_mv(void)
{
    if (!s_initialized) return 0;

    /* ADC VREF = 3.0 V (VREFINT), 12-bit. PA3 is divided by 2. */
    /* Average 8 reads for noise rejection. */
    uint32_t acc = 0;
    for (int i = 0; i < 8; i++) {
        acc += adc_read_channel(8);  /* ADC1 IN8 = PA3 */
    }
    uint16_t avg = (uint16_t)(acc / 8);

    /* Convert: Vbat = (avg / 4095) * 3.0 * 2 (divider) * 1000 mV */
    float v = (float)avg / 4095.0f * 3.0f * POWER_VBAT_DIVIDER * 1000.0f;
    return (uint16_t)v;
}

void power_update_soc(void)
{
    uint16_t mv = power_read_battery_mv();
    /* Linear approximation: 3.6 V = 100%, 3.2 V = 0%. */
    if (mv >= POWER_BAT_MAX_MV) {
        s_soc = 100;
    } else if (mv <= POWER_BAT_CRIT_MV) {
        s_soc = 0;
    } else {
        s_soc = (uint8_t)(100UL * (mv - POWER_BAT_CRIT_MV) /
                           (POWER_BAT_MAX_MV - POWER_BAT_CRIT_MV));
    }
}

uint8_t power_state_of_charge(void)
{
    return s_soc;
}

void power_enter_stop2(void)
{
    /* Configure PWR for Stop 2. */
    PWR->CR1 = (PWR->CR1 & ~0x7U) | PWR_CR1_LPMS_STOP2;
    /* Set SLEEPDEEP and WFI. */
    SCB_SCR |= SCB_SCR_SLEEPDEEP;
#ifdef ARM_CM4
    __asm volatile ("wfi");
#endif
    /* Clear SLEEPDEEP on wake. */
    SCB_SCR &= ~SCB_SCR_SLEEPDEEP;
}

uint32_t power_uptime_s(void)
{
    /* Uptime increments on each wake; in the real build this uses the RTC. */
    return s_uptime_s;
}

/* Called from the SysTick handler to track uptime. */
void power_tick(void)
{
    s_uptime_s++;
}