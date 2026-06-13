/*
 * EAG-7000 — UART Driver Header
 *
 * Copyright (c) 2026 EAG-7000 Project
 * SPDX-License-Identifier: MIT
 */

#ifndef EAG7000_UART_H
#define EAG7000_UART_H

#include <stdint.h>

/* UART device handle */
typedef struct {
    uintptr_t base;        /* UART base address */
    uint32_t  baudrate;    /* Configured baud rate */
} uart_dev_t;

/**
 * Initialize UART peripheral.
 * @param dev       UART device handle
 * @param base_addr UART base address (UART1/2/3_BASE)
 * @param baudrate  Desired baud rate (e.g., 115200)
 * @return 0 on success, negative on error
 */
int uart_init(uart_dev_t *dev, uintptr_t base_addr, uint32_t baudrate);

/**
 * Transmit a single character (blocking).
 * @param dev  UART device handle
 * @param ch   Character to transmit
 * @return 0 on success, -1 on timeout
 */
int uart_putchar(uart_dev_t *dev, uint8_t ch);

/**
 * Receive a single character (blocking with timeout).
 * @param dev           UART device handle
 * @param ch            Pointer to store received character
 * @param timeout_ticks Timeout in RTOS ticks (0 = non-blocking)
 * @return 0 on success, -1 on timeout
 */
int uart_getchar(uart_dev_t *dev, uint8_t *ch, uint32_t timeout_ticks);

/**
 * Transmit a null-terminated string (blocking).
 * @param dev UART device handle
 * @param str String to transmit
 * @return 0 on success, -1 on error
 */
int uart_puts(uart_dev_t *dev, const char *str);

/**
 * Transmit a buffer of specified length (blocking).
 * @param dev UART device handle
 * @param buf Buffer to transmit
 * @param len Number of bytes to transmit
 * @return 0 on success, -1 on error
 */
int uart_write(uart_dev_t *dev, const uint8_t *buf, uint16_t len);

#endif /* EAG7000_UART_H */