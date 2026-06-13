/*
 * EAG-7000 — GPIO Driver Header
 *
 * Copyright (c) 2026 EAG-7000 Project
 * SPDX-License-Identifier: MIT
 */

#ifndef EAG7000_GPIO_H
#define EAG7000_GPIO_H

#include <stdint.h>

/**
 * Initialize GPIO pins and IOMUX configuration for EAG-7000 board.
 * Configures all GPIO directions, pad settings, and interrupt modes.
 * @return 0 on success
 */
int gpio_init(void);

/**
 * Set a GPIO pin value.
 * @param gpio_base  GPIO port base address
 * @param pin        Pin number (0-31)
 * @param value      0 or 1
 */
void gpio_set_pin(uintptr_t gpio_base, uint8_t pin, uint8_t value);

/**
 * Read a GPIO pin value.
 * @param gpio_base  GPIO port base address
 * @param pin        Pin number (0-31)
 * @return 0 or 1
 */
uint8_t gpio_get_pin(uintptr_t gpio_base, uint8_t pin);

/**
 * Toggle a GPIO pin.
 */
void gpio_toggle_pin(uintptr_t gpio_base, uint8_t pin);

/**
 * Configure a GPIO pin direction.
 * @param gpio_base  GPIO port base address
 * @param pin        Pin number (0-31)
 * @param output     1 for output, 0 for input
 */
void gpio_set_dir(uintptr_t gpio_base, uint8_t pin, uint8_t output);

#endif /* EAG7000_GPIO_H */