/**
 * @file    servo.c
 * @brief   SG90 micro servo driver for chamber lid actuation.
 * @author  jayis1
 * @copyright Copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#include "servo.h"
#include "../board.h"
#include "../registers.h"

void servo_init(void) {
    /* TIM2 is already configured in tim2_init(). Just ensure CCR1 is safe. */
    TIM2->CCR1 = 1000;  /* 1 ms → closed position */
    TIM2->CCER |= TIM_CCER_CC1E;
}

void servo_set_position(uint16_t pulse_us) {
    if (pulse_us < 500) pulse_us = 500;
    if (pulse_us > 2500) pulse_us = 2500;
    TIM2->CCR1 = pulse_us;  /* With 1 MHz timer, 1 count = 1 µs */
}

uint16_t servo_get_position(void) {
    return (uint16_t)TIM2->CCR1;
}

bool servo_is_stalled(void) {
    return GPIO_READ_PIN(GPIOC, 5) == 0;
}