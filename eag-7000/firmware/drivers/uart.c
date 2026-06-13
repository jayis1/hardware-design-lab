/*
 * EAG-7000 — UART Driver for Cortex-M4F
 *
 * UART1: Debug console (115200 8N1)
 * UART2: Modbus RTU slave (115200 8E1)
 * UART3: General purpose (115200 8N1)
 *
 * Copyright (c) 2026 EAG-7000 Project
 * SPDX-License-Identifier: MIT
 */

#include "uart.h"
#include "registers.h"

/* ============================================================================
 * UART Initialization
 * ============================================================================ */

int uart_init(uart_dev_t *dev, uintptr_t base_addr, uint32_t baudrate)
{
    dev->base = base_addr;
    dev->baudrate = baudrate;

    uint32_t reg;

    /* Step 1: Soft reset the UART */
    mmio_write32(dev->base + UART_UCR2, 0);
    while (!(mmio_read32(dev->base + UART_UCR2) & UART_UCR2_SRST)) {
        /* Wait for reset to complete */
    }

    /* Step 2: Disable UART before configuration */
    mmio_write32(dev->base + UART_UCR1, 0);

    /* Step 3: Configure UART parameters
     * - 8 data bits (WS=1)
     * - 1 stop bit (STPB=0)
     * - No parity (PREN=0) — for debug console
     * - Ignore RTS (IRTS=1)
     */
    reg = UART_UCR2_SRST | UART_UCR2_RXEN | UART_UCR2_TXEN |
          UART_UCR2_IRTS | UART_UCR2_WS;   /* 8-bit word size */
    mmio_write32(dev->base + UART_UCR2, reg);

    /* Step 4: Configure UCR3 — RX DSENTE0, RXDMUXSEL */
    mmio_write32(dev->base + UART_UCR3, 0x00000704);

    /* Step 5: Configure UCR4 — DTR, RTS, etc. */
    mmio_write32(dev->base + UART_UCR4, 0x00008000);

    /* Step 6: Set baud rate
     * Reference clock = 80 MHz (BOARD_UART_FREQ_HZ)
     * Baud rate = ref_clk / (UBMR + 1) * 16 / (UBIR + 1)
     * Simplified: UFCR_RFDIV=2 → ref_clk/2 = 40 MHz
     * For 115200: UBIR = 0, UBMR = 40MHz / (115200 * 16) - 1 ≈ 21
     * Actually: ONEMS = ref_freq / (1000 * (RFDIV+1))
     *           UBMR = ref_freq / (2 * baud * (RFDIV+1)) - 1
     */
    uint32_t ref_freq = 80000000UL;  /* 80 MHz UART reference clock */
    uint32_t rdiv = 0;  /* RFDIV=1 → ref_clk/1 = 80 MHz */

    /* Set RFDIV in UFCR: 0 = divide by 2 */
    mmio_write32(dev->base + UART_UFCR,
                 (2U << 7) |    /* RFDIV: divide by 2 = 40 MHz */
                 (1U << 0));     /* RX FIFO threshold: 1 byte */

    /* Calculate baud rate dividers
     * Using formula: baud = ref_freq / (16 * (UBMR + 1) / (UBIR + 1))
     * With RFDIV=2: effective_freq = 40 MHz
     * 115200 = 40000000 / (16 * (UBMR+1)) → UBMR = 40000000/(16*115200) - 1 ≈ 21.7 → 21
     */
    uint32_t effective_freq = 40000000UL;
    uint32_t ubir = 0;
    uint32_t ubmr = (effective_freq / (16 * baudrate)) - 1;

    mmio_write32(dev->base + UART_UBIR, ubir);
    mmio_write32(dev->base + UART_UBMR, ubmr);

    /* Set ONE_MS register for 1ms timer */
    mmio_write32(dev->base + UART_ONEMS, effective_freq / 1000);

    /* Step 7: Enable UART */
    reg = UART_UCR1_UARTEN | UART_UCR1_RRDYEN;
    mmio_write32(dev->base + UART_UCR1, reg);

    /* Wait for TX to be ready */
    while (!(mmio_read32(dev->base + UART_USR2) & UART_USR2_TXDC)) {
        /* Wait */
    }

    return 0;
}

/* ============================================================================
 * UART Transmit / Receive
 * ============================================================================ */

/**
 * Transmit a single character (blocking).
 */
int uart_putchar(uart_dev_t *dev, uint8_t ch)
{
    /* Wait for TX FIFO to have space */
    uint32_t timeout = 100000;
    while (!(mmio_read32(dev->base + UART_USR2) & UART_USR2_TDRE)) {
        if (--timeout == 0) return -1;
    }

    mmio_write32(dev->base + UART_UTXD, ch);
    return 0;
}

/**
 * Receive a single character (blocking with timeout).
 * @param dev     UART device handle
 * @param ch      Pointer to store received character
 * @param timeout Timeout in RTOS ticks (0 = non-blocking)
 * @return 0 on success, -1 on timeout
 */
int uart_getchar(uart_dev_t *dev, uint8_t *ch, uint32_t timeout_ticks)
{
    uint32_t timeout = 100000;
    while (!(mmio_read32(dev->base + UART_USR2) & UART_USR2_RDR)) {
        if (--timeout == 0) return -1;
    }

    *ch = (uint8_t)(mmio_read32(dev->base + UART_URXD) & 0xFF);
    return 0;
}

/**
 * Transmit a null-terminated string (blocking).
 */
int uart_puts(uart_dev_t *dev, const char *str)
{
    while (*str) {
        if (uart_putchar(dev, (uint8_t)*str) != 0)
            return -1;
        str++;
    }
    return 0;
}

/**
 * Transmit a buffer of specified length (blocking).
 */
int uart_write(uart_dev_t *dev, const uint8_t *buf, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        if (uart_putchar(dev, buf[i]) != 0)
            return -1;
    }
    return 0;
}