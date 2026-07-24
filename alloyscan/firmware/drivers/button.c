/*
 * button.c — Debounced GPIO button driver implementation
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Implements debounced button reading with edge detection.
 * Uses a simple counter-based debounce: a button must read the
 * same state for DEBOUNCE_COUNT consecutive polls to register.
 */

#include "button.h"
#include "registers.h"
#include "board.h"
#include <string.h>

#define DEBOUNCE_COUNT      5    /* Polls to confirm state change */
#define REPEAT_DELAY_MS     500  /* Before auto-repeat starts */
#define REPEAT_INTERVAL_MS  100  /* Auto-repeat interval */

typedef struct {
    uint8_t  pin;
    GPIO_TypeDef *port;
    uint8_t  debounce_counter;
    bool     current_state;       /* Debounced state (true = pressed) */
    bool     prev_raw_state;
    bool     edge_pressed;        /* Set on press edge, cleared by button_pressed() */
    uint32_t press_count;
    uint32_t hold_timer;          /* ms since pressed */
} button_state_t;

static button_state_t buttons[BUTTON_COUNT] = {
    { TRIGGER_PIN,     GPIOC, 0, false, false, false, 0, 0 },
    { BUTTON_UP_PIN,    GPIOC, 0, false, false, false, 0, 0 },
    { BUTTON_DOWN_PIN,  GPIOC, 0, false, false, false, 0, 0 },
    { BUTTON_OK_PIN,     GPIOB, 0, false, false, false, 0, 0 },
};

void button_init(void)
{
    RCC_AHB2ENR |= RCC_AHB2ENR_GPIOB | RCC_AHB2ENR_GPIOC;

    for (int i = 0; i < BUTTON_COUNT; i++) {
        GPIO_TypeDef *port = buttons[i].port;
        uint8_t pin = buttons[i].pin;

        /* Set as input with pull-up (buttons are active-low) */
        port->MODER &= ~(3UL << (pin * 2));  /* Input mode */
        port->PUPDR &= ~(3UL << (pin * 2));
        port->PUPDR |= (GPIO_PUPD_PU << (pin * 2));
    }
}

void button_poll(void)
{
    for (int i = 0; i < BUTTON_COUNT; i++) {
        button_state_t *b = &buttons[i];

        /* Read raw state (active low: pressed = pin reads 0) */
        bool raw_pressed = !(b->port->IDR & (1UL << b->pin));

        /* Debounce: count consecutive same-state readings */
        if (raw_pressed == b->prev_raw_state) {
            if (b->debounce_counter < DEBOUNCE_COUNT)
                b->debounce_counter++;
        } else {
            b->debounce_counter = 0;
            b->prev_raw_state = raw_pressed;
        }

        /* Update debounced state when counter is full */
        if (b->debounce_counter >= DEBOUNCE_COUNT) {
            bool new_state = raw_pressed;

            /* Detect press edge */
            if (new_state && !b->current_state) {
                b->edge_pressed = true;
                b->press_count++;
                b->hold_timer = 0;
            }

            b->current_state = new_state;

            if (new_state) {
                b->hold_timer++;
            }
        }
    }
}

bool button_pressed(button_id_t id)
{
    if (id >= BUTTON_COUNT)
        return false;

    bool result = buttons[id].edge_pressed;
    buttons[id].edge_pressed = false;
    return result;
}

bool button_held(button_id_t id)
{
    if (id >= BUTTON_COUNT)
        return false;
    return buttons[id].current_state;
}

uint32_t button_press_count(button_id_t id)
{
    if (id >= BUTTON_COUNT)
        return 0;
    return buttons[id].press_count;
}