/*
 * pam_drv.c — PAM fluorometer driver for Lichenwatch.
 *
 * Implements a saturating-pulse protocol:
 *   1. Dark-adapt the thallus (shroud closed in main, LEDs off).
 *   2. Measure F0 with low-frequency 620 nm measuring pulses.
 *   3. Fire an 0.8 s saturating 470 nm actinic pulse; measure Fm.
 *   4. Compute Fv/Fm = (Fm - F0) / Fm.
 *
 * The photodiode signal is AC-coupled and demodulated with a synchronous
 * (lock-in) detector in firmware to reject ambient daylight leakage.
 *
 * Author: jayis1
 * License: MIT
 */

#include "pam_drv.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

/* ------------------------------------------------------------------------ */
/* Static state                                                              */
/* ------------------------------------------------------------------------ */
static int s_initialized = 0;

/* ADC sample buffer for lock-in demodulation */
#define PAM_SAMPLES 256
static uint16_t s_adc_buf[PAM_SAMPLES];

/* ------------------------------------------------------------------------ */
/* Low-level helpers                                                          */
/* ------------------------------------------------------------------------ */
static void delay_us(uint32_t us)
{
    /* rough delay at 80 MHz: ~10 cycles per iteration */
    volatile uint32_t count = us * 10;
    while (count--) { __asm volatile ("nop"); }
}

static void gpio_set(int pin, int val)
{
    int port = (pin >> 4) & 0xF;
    int num  = pin & 0xF;
    volatile uint32_t *gpio = (volatile uint32_t *)
        (AHB2PERIPH_BASE + (uint32_t)port * 0x400);
    if (val) gpio[5] = (1U << num);  /* BSRR */
    else     gpio[6] = (1U << num);  /* BRR  */
}

static uint16_t adc1_read_channel(int channel)
{
    /* Configure sequence length = 1 and channel. */
    ADC1->SQR1 = (0 << 29) | (channel << 6);  /* L=0, SQ1=channel */
    ADC1->CR |= ADC_CR_ADSTART;
    while (!(ADC1->ISR & ADC_ISR_EOC)) { }
    return (uint16_t)ADC1->DR;
}

/* ------------------------------------------------------------------------ */
/* PWM for LEDs via TIM3                                                       */
/* ------------------------------------------------------------------------ */
static void tim3_pwm_setup(void)
{
    /* Enable TIM3 clock already done in board_init. */
    /* Prescaler: 80 MHz / 80 = 1 MHz tick. */
    TIM3->PSC = 80 - 1;
    /* Auto-reload: 1 MHz / 1000 = 1 kHz base frequency. */
    TIM3->ARR = 1000 - 1;

    /* Configure channels 1,2,3 as PWM mode 1 on PC6, PC7, PC8. */
    TIM3->CCMR1 = TIM_CCMR1_OC1M_PWM1 | TIM_CCMR1_OC2M_PWM1;
    TIM3->CCMR2 = TIM_CCMR2_OC3M_PWM1;
    TIM3->CCER  = TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E;
    TIM3->CCR1 = 0;  /* actinic LED A off */
    TIM3->CCR2 = 0;  /* actinic LED B off */
    TIM3->CCR3 = 0;  /* measuring LED off */
    TIM3->CR1 |= TIM_CR1_CEN;
}

static void led_actinic_set(uint16_t duty)
{
    TIM3->CCR1 = duty;
    TIM3->CCR2 = duty;
}

static void led_measure_set(uint16_t duty)
{
    TIM3->CCR3 = duty;
}

/* ------------------------------------------------------------------------ */
/* Lock-in demodulation                                                       */
/* ------------------------------------------------------------------------ */
/*
 * Fire measuring pulses at ~1 kHz, sample the photodiode synchronously,
 * multiply by the reference square wave, and accumulate. This extracts the
 * modulated fluorescence signal from ambient daylight background.
 */
static uint16_t lock_in_measure(uint16_t duty, uint32_t cycles)
{
    int32_t acc = 0;
    for (uint32_t i = 0; i < cycles; i++) {
        /* measuring pulse ON */
        led_measure_set(duty);
        delay_us(50);
        uint16_t on = adc1_read_channel(1);  /* PA0 = ADC IN1 */

        /* measuring pulse OFF */
        led_measure_set(0);
        delay_us(50);
        uint16_t off = adc1_read_channel(1);

        /* difference removes ambient; accumulate */
        acc += (int32_t)on - (int32_t)off;
    }
    if (acc < 0) acc = 0;
    return (uint16_t)(acc / (int32_t)cycles);
}

/* ------------------------------------------------------------------------ */
/* Public API                                                                 */
/* ------------------------------------------------------------------------ */
int pam_init(void)
{
    /* Configure PAM photodiode ADC pin (PA0) as analog input. */
    {
        volatile uint32_t *pa = (volatile uint32_t *)GPIOA;
        uint32_t shift = 0 * 2;  /* pin 0 */
        pa[0] = (pa[0] & ~(0x3U << shift)) | (GPIO_MODE_ANALOG << shift);
    }

    /* Configure shroud control pin as output (PC9). */
    {
        volatile uint32_t *pc = (volatile uint32_t *)GPIOC;
        uint32_t shift = 9 * 2;
        pc[0] = (pc[0] & ~(0x3U << shift)) | (GPIO_MODE_OUTPUT << shift);
    }

    /* Configure actinic/measuring LED pins as TIM3 PWM AF. */
    /* PC6 = TIM3_CH1, PC7 = TIM3_CH2, PC8 = TIM3_CH3 — AF2 on L4. */
    {
        volatile uint32_t *pc = (volatile uint32_t *)GPIOC;
        /* PC6 AF: AFR[1] low nibble (index 6 → AFRH nibble 2) */
        pc[8] = (pc[8] & ~(0xFU << (2*4))) | (0x2U << (2*4));  /* PC6 */
        pc[8] = (pc[8] & ~(0xFU << (3*4))) | (0x2U << (3*4));  /* PC7 */
        pc[8] = (pc[8] & ~(0xFU << (4*4))) | (0x2U << (4*4));  /* PC8 */
        /* Set to AF mode */
        pc[0] = (pc[0] & ~(0x3U << (6*2))) | (GPIO_MODE_AF << (6*2));
        pc[0] = (pc[0] & ~(0x3U << (7*2))) | (GPIO_MODE_AF << (7*2));
        pc[0] = (pc[0] & ~(0x3U << (8*2))) | (GPIO_MODE_AF << (8*2));
    }

    /* Enable and calibrate ADC1. */
    {
        /* Exit deep power-down. */
        ADC1->CR &= ~ADC_CR_DEEPPWD;
        /* Enable ADC voltage regulator. */
        ADC1->CR |= ADC_CR_ADVREG;
        delay_us(20);
        /* Calibrate (single-ended). */
        ADC1->CR |= ADC_CR_ADCAL;
        while (ADC1->CR & ADC_CR_ADCAL) { }
        /* Enable ADC. */
        ADC1->ISR = ADC_ISR_ADRDY;  /* clear */
        ADC1->CR |= ADC_CR_ADEN;
        while (!(ADC1->ISR & ADC_ISR_ADRDY)) { }
    }

    tim3_pwm_setup();
    s_initialized = 1;
    return 0;
}

void pam_shroud_close(void)
{
    /* Drive shroud pin low — closes the mechanical shutter / dark cover. */
    gpio_set(PAM_SHROUD_PIN, 0);
}

void pam_shroud_open(void)
{
    gpio_set(PAM_SHROUD_PIN, 1);
}

int pam_measure(pam_result_t *res)
{
    if (!s_initialized || res == NULL) return -1;
    memset(res, 0, sizeof(*res));

    /* --- F0: low-intensity measuring pulses -------------------------------- */
    led_measure_set(0);
    led_actinic_set(0);
    delay_us(1000);

    uint16_t f0_raw = lock_in_measure(20, 64);  /* duty 2%, 64 cycles */
    float f0 = (float)f0_raw;

    /* --- Fm: saturating actinic pulse ------------------------------------- */
    led_actinic_set(900);  /* ~90% duty = near-max actinic intensity */

    /* Wait for the pulse to reach steady state, then measure peak. */
    delay_us(5000);
    uint16_t fm_raw = 0;
    for (int i = 0; i < 8; i++) {
        uint16_t v = adc1_read_channel(1);
        if (v > fm_raw) fm_raw = v;
        delay_us(100);
    }

    /* Fire measuring pulse during saturation to get the modulated Fm. */
    uint16_t fm_mod = lock_in_measure(20, 32);
    led_actinic_set(0);

    float fm = (float)fm_mod > (float)fm_raw ? (float)fm_mod : (float)fm_raw + fm_mod;

    /* --- Compute yield ----------------------------------------------------- */
    res->raw_f0 = f0_raw;
    res->raw_fm = fm_raw;
    res->f0     = f0;
    res->fm     = fm;

    if (fm > 1.0f) {
        res->fv_fm = (fm - f0) / fm;
    } else {
        res->fv_fm = 0.0f;
        return -2;  /* signal too low — shroud failure or no thallus */
    }

    /* Sanity: yield must be in [0, 0.9]. */
    if (res->fv_fm < 0.0f) res->fv_fm = 0.0f;
    if (res->fv_fm > 0.9f) res->fv_fm = 0.9f;

    return 0;
}