/**
 * @file    haptic.c
 * @brief   TideBand — Haptic motor driver with direction-encoded patterns.
 *          Uses TIM2_CH1 PWM to drive an eccentric rotating mass (ERM)
 *          motor through a MOSFET. Patterns encode current direction:
 *            - Long pulse: current flowing northward
 *            - Short pulse: current flowing southward
 *            - Double pulse: eastward
 *            - Triple pulse: westward
 *          The intensity (PWM duty) scales with current speed.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#include "board.h"
#include "registers.h"
#include "haptic.h"

/* ---- State ---- */
static haptic_pattern_t active_pattern = HAPTIC_NONE;
static uint16_t pattern_timer_ms;    /* Time elapsed in current pattern phase */
static uint8_t  pattern_step;        /* Current step within multi-pulse pattern */
static float    speed_threshold = 0.5f;  /* Default: alert above 0.5 m/s */
static uint8_t  haptic_enabled = 1;

/* ---- Pattern definitions ---- */
/* Each pattern is a sequence of (on_ms, off_ms) pairs. */
typedef struct {
    uint8_t num_steps;
    uint16_t on_ms[4];
    uint16_t off_ms[4];
} pattern_def_t;

static const pattern_def_t patterns[] = {
    {0, {0,0,0,0}, {0,0,0,0}},            /* NONE */
    {1, {50,0,0,0}, {0,0,0,0}},           /* SHORT: 50ms on */
    {1, {200,0,0,0}, {0,0,0,0}},          /* LONG: 200ms on */
    {2, {50,50,0,0}, {50,0,0,0}},         /* DOUBLE: on-off-on */
    {3, {50,50,50,0}, {50,50,0,0}},       /* TRIPLE: on-off-on-off-on */
    {1, {65535,0,0,0}, {0,0,0,0}},        /* CONTINUOUS */
};

/* ---- Local functions ---- */
static void motor_on(uint8_t duty);
static void motor_off(void);

/* ---- Public API ---- */

void haptic_init(void)
{
    /* Enable TIM2 clock */
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2;

    /* Configure PA0 as TIM2_CH1 (AF1) */
    gpio_set_mode(HAPTIC_PWM_GPIO, HAPTIC_PWM_PIN, GPIO_MODE_AF);
    gpio_set_af(HAPTIC_PWM_GPIO, HAPTIC_PWM_PIN, HAPTIC_PWM_AF);
    gpio_set_speed(HAPTIC_PWM_GPIO, HAPTIC_PWM_PIN, GPIO_SPEED_HIGH);

    /* Motor enable pin — output, initially low */
    gpio_set_mode(HAPTIC_EN_GPIO, HAPTIC_EN_PIN, GPIO_MODE_OUTPUT);
    gpio_clear(HAPTIC_EN_GPIO, HAPTIC_EN_PIN);

    /* TIM2: APB1 timer clock = 140 MHz
     * PWM at 200 Hz: PSC = 5600-1, ARR = 125-1
     * 140MHz / 5600 / 125 = 200 Hz
     * This frequency is below the motor's resonant frequency for
     * smooth vibration rather than buzzing. */
    TIM2_PSC = 5599;
    TIM2_ARR = 124;
    TIM2_CCMR1 = TIM_CCMR1_OC1M_PWM1 | TIM_CCMR1_OC1PE;
    TIM2_CCER = TIM_CCER_CC1E;
    TIM2_CCR1 = 0;  /* 0% duty initially */
    TIM2_CR1 = TIM_CR1_ARPE;  /* Not started yet */
}

void haptic_trigger(haptic_pattern_t pattern)
{
    if (!haptic_enabled || pattern == HAPTIC_NONE) {
        return;
    }
    active_pattern = pattern;
    pattern_timer_ms = 0;
    pattern_step = 0;
    motor_on(80);  /* 80% duty for strong vibration */
}

void haptic_update(void)
{
    if (active_pattern == HAPTIC_NONE) {
        return;
    }

    const pattern_def_t *p = &patterns[active_pattern];
    pattern_timer_ms++;

    if (pattern_step >= p->num_steps) {
        /* Pattern complete */
        motor_off();
        active_pattern = HAPTIC_NONE;
        return;
    }

    /* Check if we're in the ON phase of the current step */
    if (pattern_timer_ms <= p->on_ms[pattern_step]) {
        /* Still in ON phase */
        motor_on(80);
    } else if (pattern_timer_ms <= p->on_ms[pattern_step] + p->off_ms[pattern_step]) {
        /* In OFF phase */
        motor_off();
    } else {
        /* Move to next step */
        pattern_step++;
        pattern_timer_ms = 0;
        if (pattern_step < p->num_steps) {
            motor_on(80);
        }
    }
}

void haptic_stop(void)
{
    motor_off();
    active_pattern = HAPTIC_NONE;
}

void haptic_set_threshold(float speed_ms)
{
    speed_threshold = speed_ms;
}

void haptic_set_enabled(uint8_t enabled)
{
    haptic_enabled = enabled;
    if (!enabled) {
        haptic_stop();
    }
}

uint8_t haptic_is_active(void)
{
    return (active_pattern != HAPTIC_NONE) ? 1 : 0;
}

/* ---- Local functions ---- */

static void motor_on(uint8_t duty)
{
    /* Enable motor driver */
    gpio_set(HAPTIC_EN_GPIO, HAPTIC_EN_PIN);
    /* Set PWM duty */
    TIM2_CCR1 = (uint32_t)duty * 125u / 100u;  /* Scale to ARR */
    /* Start timer */
    TIM2_CR1 |= TIM_CR1_CEN;
}

static void motor_off(void)
{
    TIM2_CCR1 = 0;  /* 0% duty */
    TIM2_CR1 &= ~TIM_CR1_CEN;  /* Stop timer */
    gpio_clear(HAPTIC_EN_GPIO, HAPTIC_EN_PIN);  /* Disable driver */
}