/*
 * EAG-7000 — MCP2518FD CAN-FD Controller Driver (M4F side)
 *
 * Low-level driver for the Microchip MCP2518FD CAN-FD controller
 * connected via SPI (ECSPI2 or ECSPI3). This is the M4F firmware
 * counterpart to the Linux kernel driver in Phase 4 Section 4.4.
 *
 * Features:
 *   - CAN-FD up to 8 Mbps (data phase), 500 kbps (arbitration)
 *   - SPI bus at 10 MHz
 *   - Two RX FIFOs (0 and 1)
 *   - One TX Queue (FIFO 0)
 *   - Standard and extended frame support
 *   - Bit Rate Switch (BRS) support
 *
 * Copyright (c) 2026 EAG-7000 Project
 * SPDX-License-Identifier: MIT
 */

#include "mcp2518fd.h"
#include "registers.h"
#include "spi.h"

/* ============================================================================
 * SPI Register Access Helpers
 * ============================================================================ */

/**
 * Build SPI command byte for MCP2518FD.
 * Command format: [CMD(4)] [ADDR(12)]
 */
static void mcp2518fd_spi_cmd(uint8_t *buf, uint8_t cmd, uint16_t addr)
{
    buf[0] = cmd | ((addr >> 8) & 0x0F);
    buf[1] = addr & 0xFF;
}

/**
 * Read a single 32-bit register from MCP2518FD via SPI.
 */
static int mcp2518fd_reg_read(mcp2518fd_dev_t *dev, uint16_t addr, uint32_t *val)
{
    uint8_t tx_buf[6];  /* 2 cmd + 4 dummy */
    uint8_t rx_buf[6];  /* 2 dummy + 4 data */
    int ret;

    /* Build read command */
    mcp2518fd_spi_cmd(tx_buf, MCP2518FD_SPI_CMD_READ, addr);

    /* SPI transfer: send command, then read 4 bytes */
    ret = spi_transfer(dev->spi, tx_buf, rx_buf, 6);
    if (ret) return ret;

    /* Parse 32-bit value (little-endian) */
    *val = rx_buf[2] | (rx_buf[3] << 8) | (rx_buf[4] << 16) | (rx_buf[5] << 24);
    return 0;
}

/**
 * Write a single 32-bit register to MCP2518FD via SPI.
 */
static int mcp2518fd_reg_write(mcp2518fd_dev_t *dev, uint16_t addr, uint32_t val)
{
    uint8_t tx_buf[6];  /* 2 cmd + 4 data */
    int ret;

    /* Build write command */
    mcp2518fd_spi_cmd(tx_buf, MCP2518FD_SPI_CMD_WRITE, addr);

    /* Pack 32-bit value (little-endian) */
    tx_buf[2] = (uint8_t)(val & 0xFF);
    tx_buf[3] = (uint8_t)((val >> 8) & 0xFF);
    tx_buf[4] = (uint8_t)((val >> 16) & 0xFF);
    tx_buf[5] = (uint8_t)((val >> 24) & 0xFF);

    ret = spi_write(dev->spi, tx_buf, 6);
    return ret;
}

/* ============================================================================
 * MCP2518FD Initialization
 * ============================================================================ */

int mcp2518fd_init(mcp2518fd_dev_t *dev, spi_dev_t *spi,
                    uint8_t nom_brp, uint8_t nom_tseg1, uint8_t nom_tseg2, uint8_t nom_sjw,
                    uint8_t data_brp, uint8_t data_tseg1, uint8_t data_tseg2, uint8_t data_sjw)
{
    uint32_t reg;
    int ret;

    dev->spi = spi;

    /* Step 1: Reset MCP2518FD via SPI command */
    uint8_t reset_cmd[2];
    mcp2518fd_spi_cmd(reset_cmd, MCP2518FD_SPI_CMD_RESET, 0);
    spi_write(dev->spi, reset_cmd, 2);

    /* Wait for controller reset (50ms) */
    for (volatile uint32_t i = 0; i < 500000; i++);

    /* Step 2: Configure CAN mode — Configuration mode */
    ret = mcp2518fd_reg_write(dev, MCP2518FD_CON,
                               MCP2518FD_CON_REQOP_CONFIG |
                               MCP2518FD_CON_TXQEN |
                               (1U << 23));  /* STDEXT — extended ID support */
    if (ret) return ret;

    /* Verify configuration mode entered */
    ret = mcp2518fd_reg_read(dev, MCP2518FD_CON, &reg);
    if (ret) return ret;
    if ((reg & (0x7U << 21)) != (0x4U << 21)) {
        return -2;  /* Not in configuration mode */
    }

    /* Step 3: Configure nominal bit timing (500 kbps @ 80 MHz) */
    reg = ((uint32_t)nom_brp << 24) |
          ((uint32_t)nom_tseg1 << 16) |
          ((uint32_t)nom_tseg2 << 8) |
          ((uint32_t)nom_sjw << 0);
    ret = mcp2518fd_reg_write(dev, MCP2518FD_NBTCFG, reg);
    if (ret) return ret;

    /* Step 4: Configure data bit timing (2 Mbps @ 80 MHz for CAN-FD) */
    reg = ((uint32_t)data_brp << 24) |
          ((uint32_t)data_tseg1 << 16) |
          ((uint32_t)data_tseg2 << 8) |
          ((uint32_t)data_sjw << 0);
    ret = mcp2518fd_reg_write(dev, MCP2518FD_DBTCFG, reg);
    if (ret) return ret;

    /* Step 5: Configure transmitter delay compensation (TDC) */
    ret = mcp2518fd_reg_write(dev, MCP2518FD_TDC, 0x00000005);
    if (ret) return ret;

    /* Step 6: Configure TX Queue (FIFO 0) */
    /* TXQ: 12 messages, 8 data bytes each, priority 0 */
    ret = mcp2518fd_reg_write(dev, MCP2518FD_FIFOCON(0),
                               (0U << 27) |   /* Payload size: 8 bytes (0=8) */
                               (0U << 24) |   /* Number of messages - 1 */
                               (1U << 31));    /* TX Queue Enable */
    if (ret) return ret;

    /* Step 7: Configure RX FIFOs */
    /* RX FIFO 0: 12 messages, 8 data bytes */
    ret = mcp2518fd_reg_write(dev, MCP2518FD_FIFOCON(1),
                               (0U << 27) |    /* Payload size: 8 bytes */
                               (1U << 31) |    /* FIFO Enable */
                               (1U << 30));    /* RX Overflow Interrupt */
    if (ret) return ret;

    /* RX FIFO 1: 4 messages, 64 data bytes (CAN-FD) */
    ret = mcp2518fd_reg_write(dev, MCP2518FD_FIFOCON(2),
                               (7U << 27) |    /* Payload size: 64 bytes (7=64) */
                               (1U << 31) |    /* FIFO Enable */
                               (1U << 30));    /* RX Overflow Interrupt */
    if (ret) return ret;

    /* Step 8: Switch to Normal mode (CAN-FD with BRS) */
    ret = mcp2518fd_reg_write(dev, MCP2518FD_CON,
                               (0U << 24) |    /* Normal mode */
                               (1U << 24) |    /* TXQEN */
                               (1U << 23) |    /* STDEXT */
                               (1U << 22));    /* ESIGM — enable CAN-FD */
    if (ret) return ret;

    /* Step 9: Enable RX and TX interrupts */
    mcp2518fd_reg_write(dev, MCP2518FD_RXIF, 0x03);   /* RX FIFO 0 & 1 */
    mcp2518fd_reg_write(dev, MCP2518FD_TXIF, 0x01);   /* TX Queue */

    return 0;
}

/* ============================================================================
 * CAN TX/RX Operations
 * ============================================================================ */

/**
 * Check if a CAN frame is available in RX FIFO.
 * @return 1 if frame available, 0 otherwise
 */
int mcp2518fd_rx_ready(mcp2518fd_dev_t *dev)
{
    uint32_t rxif;
    if (mcp2518fd_reg_read(dev, MCP2518FD_RXIF, &rxif) != 0)
        return 0;
    return (rxif & 0x03) ? 1 : 0;   /* RX FIFO 0 or 1 has data */
}

/**
 * Receive a CAN/CAN-FD frame from MCP2518FD.
 *
 * @param dev    MCP2518FD device handle
 * @param id     Pointer to store CAN ID
 * @param dlc    Pointer to store Data Length Code
 * @param data   Buffer for frame data (must be >= 64 bytes for CAN-FD)
 * @param flags  Pointer to store frame flags
 * @return 0 on success, negative on error
 */
int mcp2518fd_receive(mcp2518fd_dev_t *dev, uint32_t *id, uint8_t *dlc,
                       uint8_t *data, uint8_t *flags)
{
    uint32_t rxif, fifocon, obj_flags, obj_id, obj_dlc;
    uint16_t fifo_ram;
    int ret;

    /* Check which FIFO has data */
    ret = mcp2518fd_reg_read(dev, MCP2518FD_RXIF, &rxif);
    if (ret) return ret;

    if (rxif & 0x01) {
        /* RX FIFO 0 — standard CAN frames */
        fifo_ram = MCP2518FD_RXFIFO0;
    } else if (rxif & 0x02) {
        /* RX FIFO 1 — CAN-FD frames */
        fifo_ram = MCP2518FD_RXFIFO1;
    } else {
        return -1;  /* No data */
    }

    /* Read message object from RX FIFO RAM */
    uint8_t rx_buf[76];  /* Max CAN-FD frame: 12 header + 64 data */
    uint8_t cmd_buf[2];
    mcp2518fd_spi_cmd(cmd_buf, MCP2518FD_SPI_CMD_READ, fifo_ram);

    /* SPI read: command(2) + data(76) */
    uint8_t tx_buf[78];
    uint8_t recv_buf[78];
    tx_buf[0] = cmd_buf[0];
    tx_buf[1] = cmd_buf[1];
    /* Remaining bytes are dummy for read */

    ret = spi_transfer(dev->spi, tx_buf, recv_buf, 78);
    if (ret) return ret;

    /* Parse message object header (skip 2 command bytes) */
    obj_flags = recv_buf[2] | (recv_buf[3] << 8) |
                (recv_buf[4] << 16) | (recv_buf[5] << 24);
    obj_id    = recv_buf[6] | (recv_buf[7] << 8) |
                (recv_buf[8] << 16) | (recv_buf[9] << 24);
    obj_dlc   = recv_buf[10] | (recv_buf[11] << 8);

    /* Extract ID */
    if (obj_flags & (1U << 1)) {
        /* Extended ID (29-bit) */
        *id = obj_id & 0x1FFFFFFF;
    } else {
        /* Standard ID (11-bit) */
        *id = obj_id & 0x7FF;
    }

    /* Extract DLC */
    *dlc = (uint8_t)(obj_dlc & 0x0F);

    /* Extract flags */
    *flags = 0;
    if (obj_flags & (1U << 30)) *flags |= 0x04;  /* FDF */
    if (obj_flags & (1U << 29)) *flags |= 0x08;  /* BRS */
    if (obj_flags & (1U << 28)) *flags |= 0x10;  /* ESI */
    if (obj_flags & (1U << 1))  *flags |= 0x01;  /* EXT */
    if (obj_flags & (1U << 0))  *flags |= 0x02;  /* RTR */

    /* Extract data */
    uint8_t data_len = *dlc;
    if (*flags & 0x04) {
        /* CAN-FD frame — DLC to length conversion */
        static const uint8_t dlc_to_len[] = {
            0,1,2,3,4,5,6,7,8,12,16,20,24,32,48,64
        };
        data_len = (*dlc < 16) ? dlc_to_len[*dlc] : 64;
    }
    for (uint8_t i = 0; i < data_len && i < 64; i++) {
        data[i] = recv_buf[14 + i];  /* Skip 2 cmd + 12 header */
    }

    /* Clear RX FIFO interrupt flag */
    mcp2518fd_reg_write(dev, MCP2518FD_RXIF, rxif & 0x03);

    return 0;
}

/**
 * Transmit a CAN/CAN-FD frame via MCP2518FD.
 *
 * @param dev    MCP2518FD device handle
 * @param id     CAN identifier (11-bit standard or 29-bit extended)
 * @param dlc    Data Length Code
 * @param data   Frame data
 * @param flags  Frame flags (CAN_FRAME_EXT, CAN_FRAME_FDF, etc.)
 * @return 0 on success, negative on error
 */
int mcp2518fd_transmit(mcp2518fd_dev_t *dev, uint32_t id, uint8_t dlc,
                        const uint8_t *data, uint8_t flags)
{
    uint8_t tx_data[76];  /* Max CAN-FD frame */
    uint32_t obj_flags = 0;
    uint32_t fifocon;
    int ret;

    /* Build message object header */
    if (flags & 0x01) {
        obj_flags |= (1U << 1);   /* Extended ID */
        obj_id = id & 0x1FFFFFFF;
    } else {
        obj_id = id & 0x7FF;
    }

    if (flags & 0x02) obj_flags |= (1U << 0);   /* RTR */
    if (flags & 0x04) obj_flags |= (1U << 30);   /* FDF */
    if (flags & 0x08) obj_flags |= (1U << 29);   /* BRS */

    /* Build SPI write buffer */
    mcp2518fd_spi_cmd(tx_data, MCP2518FD_SPI_CMD_WRITE, MCP2518FD_TXQ);

    /* Pack header (little-endian) */
    tx_data[2] = (uint8_t)(obj_flags & 0xFF);
    tx_data[3] = (uint8_t)((obj_flags >> 8) & 0xFF);
    tx_data[4] = (uint8_t)((obj_flags >> 16) & 0xFF);
    tx_data[5] = (uint8_t)((obj_flags >> 24) & 0xFF);
    tx_data[6] = (uint8_t)(obj_id & 0xFF);
    tx_data[7] = (uint8_t)((obj_id >> 8) & 0xFF);
    tx_data[8] = (uint8_t)((obj_id >> 16) & 0xFF);
    tx_data[9] = (uint8_t)((obj_id >> 24) & 0xFF);
    tx_data[10] = dlc;
    tx_data[11] = 0;  /* DLC high byte */
    tx_data[12] = 0;  /* Reserved */
    tx_data[13] = 0;  /* Reserved */

    /* Copy data */
    uint8_t data_len = (dlc < 9) ? dlc : ((dlc < 16) ? dlc : 64);
    for (uint8_t i = 0; i < data_len; i++) {
        tx_data[14 + i] = data[i];
    }

    /* Write to TX Queue via SPI */
    ret = spi_write(dev->spi, tx_data, 14 + data_len);
    if (ret) return ret;

    /* Trigger TX Queue processing */
    ret = mcp2518fd_reg_read(dev, MCP2518FD_FIFOCON(0), &fifocon);
    if (ret) return ret;

    fifocon |= (1U << 0);  /* TXREQ = 1 */
    ret = mcp2518fd_reg_write(dev, MCP2518FD_FIFOCON(0), fifocon);

    return ret;
}