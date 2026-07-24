/*
 * button.h — Debounced GPIO button driver
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef ALLOYSCAN_BUTTON_H
#define ALLOYSCAN_BUTTON_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    BUTTON_TRIGGER = 0,
    BUTTON_UP,
    BUTTON_DOWN,
    BUTTON_OK,
    BUTTON_COUNT
} button_id_t;

/* Initialize button GPIOs */
void button_init(void);

/* Poll button states (call every ~1ms from main loop or SysTick) */
void button_poll(void);

/* Check if a button was pressed (edge-detected, auto-clears) */
bool button_pressed(button_id_t id);

/* Check if a button is currently held down */
bool button_held(button_id_t id);

/* Get the press count for a button since last reset */
uint32_t button_press_count(button_id_t id);

#endif /* ALLOYSCAN_BUTTON_H */