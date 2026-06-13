/*
 * EAG-7000 — MU Mailbox Driver Header
 *
 * Copyright (c) 2026 EAG-7000 Project
 * SPDX-License-Identifier: MIT
 */

#ifndef EAG7000_MU_H
#define EAG7000_MU_H

#include <stdint.h>

/* MU message structure */
typedef struct {
    uint8_t  type;       /* MU_MSG_TYPE_* */
    uint8_t  len;        /* Payload length hint */
    uint16_t data;       /* 16-bit data field */
} mu_msg_t;

/* Message type constants (defined in board.h) */
#ifndef MU_MSG_TYPE_HEARTBEAT
#define MU_MSG_TYPE_HEARTBEAT   0x01
#define MU_MSG_TYPE_CAN_RX      0x02
#define MU_MSG_TYPE_CAN_TX      0x03
#define MU_MSG_TYPE_MODBUS_RX   0x04
#define MU_MSG_TYPE_MODBUS_TX   0x05
#define MU_MSG_TYPE_POWER       0x06
#define MU_MSG_TYPE_STATUS      0x07
#endif

#ifndef MU_MSG_TYPE_SHIFT
#define MU_MSG_TYPE_SHIFT       24
#define MU_MSG_LEN_SHIFT        16
#define MU_MSG_DATA_MASK        0xFFFF
#endif

/**
 * Initialize MU mailbox peripheral.
 * Enables RX interrupts for A72→M4 communication.
 * @return 0 on success
 */
int mu_init(void);

/**
 * Send a message to A72 via MU TX register.
 * @param type  Message type (MU_MSG_TYPE_*)
 * @param len   Payload length hint
 * @param data  16-bit data field
 * @return 0 on success, -1 on timeout
 */
int mu_send(uint8_t type, uint8_t len, uint16_t data);

/**
 * Check if a message is available from A72.
 * @return 1 if available, 0 otherwise
 */
int mu_available(void);

/**
 * Receive a message from A72 (blocking with timeout).
 * @param msg     Pointer to message structure
 * @param timeout Timeout in RTOS ticks
 * @return 0 on success, -1 on timeout
 */
int mu_receive(mu_msg_t *msg, uint32_t timeout);

/**
 * Receive a message from A72 (non-blocking).
 * @param msg  Pointer to message structure
 * @return 0 on success, -1 if no message available
 */
int mu_receive_nb(mu_msg_t *msg);

#endif /* EAG7000_MU_H */