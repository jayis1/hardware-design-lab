/*
 * wetness_drv.c — Surface wetness driver using an interdigitated gold grid.
 *
 * An interdigitated gold-plated flex PCB grid is pressed onto the lichen
 * thallus. When dry, resistance is very high (>10 MΩ); when wet, conductance
 * rises as ions become available. We excite the grid with a brief GPIO pulse
 * and read the resulting voltage divider on ADC1 IN2 (PA1).
 *
 * To avoid electrochemical corrosion of the gold grid, we only energize the
 * grid for ~1 ms per measurement — far below any electrode degradation
 * threshold.
 *
 * Author: jayis1
 * License: MIT
 */

#include "wetness_drv.h"
#include "../board.h"
#include "../registers.h"

/* Calibration constants (site-specific, refined in first 24 h after install). */
#define WETNESS_DRY_ADC    3800   /* ADC counts when thallus is bone-dry  */
#define WETNESS_WET_ADC    200    /* ADC counts when thallus is saturated  */

static int s_initialized = 0;

static void gpio_set(int pin, int val)
{
    int port = (pin >> 4) & 0xF;
    int num  = pin & 0xF;
    volatile uint32_t *gpio = (volatile uint32_t *)
        (AHB2PERIPH_BASE + (uint32_t)port * 0x400);
    if (val) gpio[5] = (1U << num);
    else     gpio[6] = (1U << num);
}

int wetness_init(void)
{
    /* Configure PA1 as analog input (ADC1 IN2). */
    volatile uint32_t *pa = (volatile uint32_t *)GPIOA;
    pa[0] = (pa[0] & ~(0x3U << (1*2))) | (GPIO_MODE_ANALOG << (1*2));

    /* Configure PB1 as output (excitation). */
    volatile uint32_t *pb = (volatile uint32_t *)GPIOB;
    pb[0] = (pb[0] & ~(0x3U << (1*2))) | (GPIO_MODE_OUTPUT << (1*2));
    gpio_set(WETNESS_EXCITE_PIN, 0);

    s_initialized = 1;
    return 0;
}

float wetness_read(void)
{
    if (!s_initialized) return 0.0f;

    /* Briefly energize the grid. */
    gpio_set(WETNESS_EXCITE_PIN, 1);
    /* Wait ~1 ms for the voltage divider to settle. */
    for (volatile int d = 0; d < 800; d++) { __asm volatile ("nop"); }

    /* Read ADC1 IN2. */
    ADC1->SQR1 = (0 << 29) | (2 << 6);  /* L=0, SQ1=channel 2 (PA1) */
    ADC1->CR |= ADC_CR_ADSTART;
    while (!(ADC1->ISR & ADC_ISR_EOC)) { }
    uint16_t adc = (uint16_t)ADC1->DR;

    /* De-energize immediately. */
    gpio_set(WETNESS_EXCITE_PIN, 0);

    /* Map ADC counts to 0..1 wetness scale (inverse: high ADC = dry). */
    if (adc >= WETNESS_DRY_ADC) return 0.0f;
    if (adc <= WETNESS_WET_ADC) return 1.0f;
    float w = (float)(WETNESS_DRY_ADC - adc) / (float)(WETNESS_DRY_ADC - WETNESS_WET_ADC);
    if (w < 0.0f) w = 0.0f;
    if (w > 1.0f) w = 1.0f;
    return w;
}