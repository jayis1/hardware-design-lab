/*
 * EAG-7000 — MU (Message Unit) Mailbox Driver for Cortex-M4F
 *
 * Manages communication between the Cortex-M4F and Cortex-A72 via
 * the i.MX8M Plus MU (Message Unit) peripheral.
 *
 * The MU provides 4 TX registers (TR0-TR3) and 4 RX registers (RR0-RR3)
 * for 32-bit message passing. Interrupts are used for notification.
 *
 * Protocol: Each 32-bit message is encoded as:
 *   [TYPE(8)][LEN(8)][DATA(16)]
 * where TYPE is MU_MSG_TYPE_*, LEN is payload length hint, and DATA
 * is a 16-bit data field (or lower 16 bits of CAN ID, etc.)
 *
 * Copyright (c) 2026 EAG-7000 Project
 * SPDX-License-Identifier: MIT
 */

#include "mu.h"
#include "registers.h"

/* ============================================================================
 * MU Initialization
 * ============================================================================ */

int mu_init(void)
{
    /* Disable all MU interrupts first */
    mmio_write32(MU1_BASE + MU_CR, 0);

    /* Clear any pending RX flags by reading all RX registers */
    (void)mmio_read32(MU1_BASE + MU_RR0);
    (void)mmio_read32(MU1_BASE + MU_RR1);
    (void)mmio_read32(MU1_BASE + MU_RR2);
    (void)mmio_read32(MU1_BASE + MU_RR3);

    /* Enable RX full interrupt (RR0) — notification from A72 */
    mmio_write32(MU1_BASE + MU_CR,
                 (1U << (MU_CR_RIEn_SHIFT + 0)) |   /* RIEn for RR0 */
                 (1U << (MU_CR_RIEn_SHIFT + 1)));     /* RIEn for RR1 */

    return 0;
}

/* ============================================================================
 * MU Transmit — M4F → A72
 * ============================================================================ */

/**
 * Send a 32-bit message via MU TX register.
 * Blocks until the TX register is empty.
 *
 * @param type  Message type (MU_MSG_TYPE_*)
 * @param len   Payload length hint
 * @param data  16-bit data field
 * @return 0 on success
 */
int mu_send(uint8_t type, uint8_t len, uint16_t data)
{
    uint32_t msg = ((uint32_t)type << MU_MSG_TYPE_SHIFT) |
                   ((uint32_t)len << MU_MSG_LEN_SHIFT) |
                   ((uint32_t)data & MU_MSG_DATA_MASK);

    /* Wait for TR0 to be empty (TX empty flag for channel 0) */
    uint32_t timeout = 10000;
    while (!(mmio_read32(MU1_BASE + MU_SR) & (1U << (MU_SR_TEn_SHIFT + 0)))) {
        if (--timeout == 0) return -1;
    }

    /* Write message to TR0 */
    mmio_write32(MU1_BASE + MU_TR0, msg);

    /* Optionally trigger a general interrupt to A72 */
    /* GI0 interrupt — notify A72 that a message is pending */
    mmio_write32(MU1_BASE + MU_CR,
                 mmio_read32(MU1_BASE + MU_CR) | (1U << (MU_CR_Fn_SHIFT + 0)));

    return 0;
}

/* ============================================================================
 * MU Receive — A72 → M4F
 * ============================================================================ */

/**
 * Check if a message is available in MU RX register.
 * @return 1 if message available, 0 otherwise
 */
int mu_available(void)
{
    return (mmio_read32(MU1_BASE + MU_SR) & (1U << (MU_SR_RFn_SHIFT + 0))) ? 1 : 0;
}

/**
 * Receive a 32-bit message from MU RX register (blocking).
 * Decodes the message into type, len, data fields.
 *
 * @param msg       Pointer to mu_msg_t structure to fill
 * @param timeout   Timeout in RTOS ticks (portMAX_DELAY for infinite)
 * @return 0 on success, -1 on timeout
 */
int mu_receive(mu_msg_t *msg, uint32_t timeout)
{
    /* Wait for RX register to be full */
    uint32_t count = timeout;
    while (!(mmio_read32(MU1_BASE + MU_SR) & (1U << (MU_SR_RFn_SHIFT + 0)))) {
        if (count == 0) return -1;
        count--;
        /* In FreeRTOS context, use vTaskDelay(1) instead of busy wait */
    }

    /* Read message from RR0 */
    uint32_t raw = mmio_read32(MU1_BASE + MU_RR0);

    msg->type = (uint8_t)((raw >> MU_MSG_TYPE_SHIFT) & 0xFF);
    msg->len  = (uint8_t)((raw >> MU_MSG_LEN_SHIFT) & 0xFF);
    msg->data = (uint16_t)(raw & MU_MSG_DATA_MASK);

    return 0;
}

/**
 * Receive a message (non-blocking).
 * Returns immediately if no message available.
 */
int mu_receive_nb(mu_msg_t *msg)
{
    if (!mu_available()) return -1;

    uint32_t raw = mmio_read32(MU1_BASE + MU_RR0);
    msg->type = (uint8_t)((raw >> MU_MSG_TYPE_SHIFT) & 0xFF);
    msg->len  = (uint8_t)((raw >> MU_MSG_LEN_SHIFT) & 0xFF);
    msg->data = (uint16_t)(raw & MU_MSG_DATA_MASK);

    return 0;
}