/*
 * EAG-7000 — MCP2518FD CAN-FD Controller Driver Header (M4F side)
 *
 * Copyright (c) 2026 EAG-7000 Project
 * SPDX-License-Identifier: MIT
 */

#ifndef EAG7000_MCP2518FD_H
#define EAG7000_MCP2518FD_H

#include <stdint.h>
#include "spi.h"

/* MCP2518FD device handle */
typedef struct {
    spi_dev_t *spi;      /* SPI bus device (ECSPI2 or ECSPI3) */
    uint8_t    cs;        /* SPI chip select (0 or 1) */
} mcp2518fd_dev_t;

/* CAN frame flags */
#define CAN_FRAME_EXT    0x01    /* Extended ID (29-bit) */
#define CAN_FRAME_RTR    0x02    /* Remote Transmission Request */
#define CAN_FRAME_FDF    0x04    /* CAN-FD format */
#define CAN_FRAME_BRS    0x08    /* Bit Rate Switch */
#define CAN_FRAME_ESI    0x10    /* Error State Indicator */

/**
 * Initialize MCP2518FD CAN-FD controller.
 * Resets the controller, configures bit timing, and enables normal mode.
 *
 * @param dev         MCP2518FD device handle
 * @param spi         SPI bus device (already initialized)
 * @param nom_brp     Nominal bit-time BRP divider
 * @param nom_tseg1   Nominal TSEG1
 * @param nom_tseg2   Nominal TSEG2
 * @param nom_sjw      Nominal SJW
 * @param data_brp     Data bit-time BRP divider
 * @param data_tseg1   Data TSEG1
 * @param data_tseg2   Data TSEG2
 * @param data_sjw      Data SJW
 * @return 0 on success, negative on error
 */
int mcp2518fd_init(mcp2518fd_dev_t *dev, spi_dev_t *spi,
                    uint8_t nom_brp, uint8_t nom_tseg1, uint8_t nom_tseg2, uint8_t nom_sjw,
                    uint8_t data_brp, uint8_t data_tseg1, uint8_t data_tseg2, uint8_t data_sjw);

/**
 * Check if a CAN frame is available in RX FIFO.
 * @return 1 if frame available, 0 otherwise
 */
int mcp2518fd_rx_ready(mcp2518fd_dev_t *dev);

/**
 * Receive a CAN/CAN-FD frame from MCP2518FD.
 * @param dev    MCP2518FD device handle
 * @param id     Pointer to store CAN ID
 * @param dlc    Pointer to store Data Length Code
 * @param data   Buffer for frame data (>= 64 bytes)
 * @param flags  Pointer to store frame flags
 * @return 0 on success, negative on error
 */
int mcp2518fd_receive(mcp2518fd_dev_t *dev, uint32_t *id, uint8_t *dlc,
                       uint8_t *data, uint8_t *flags);

/**
 * Transmit a CAN/CAN-FD frame via MCP2518FD.
 * @param dev    MCP2518FD device handle
 * @param id     CAN identifier
 * @param dlc    Data Length Code
 * @param data   Frame data
 * @param flags  Frame flags (CAN_FRAME_EXT, CAN_FRAME_FDF, etc.)
 * @return 0 on success, negative on error
 */
int mcp2518fd_transmit(mcp2518fd_dev_t *dev, uint32_t id, uint8_t dlc,
                        const uint8_t *data, uint8_t flags);

#endif /* EAG7000_MCP2518FD_H */