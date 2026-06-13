/*
 * EAG-7000 — Board-Level Definitions
 *
 * Pin assignments, clock frequencies, LED macros, and hardware constants
 * for the i.MX8M Plus Cortex-M4F subsystem on the EAG-7000 board.
 *
 * Based on Phase 2 pinouts and Phase 4 register definitions.
 *
 * Copyright (c) 2026 EAG-7000 Project
 * SPDX-License-Identifier: MIT
 */

#ifndef EAG7000_BOARD_H
#define EAG7000_BOARD_H

#include <stdint.h>
#include "registers.h"

/* ============================================================================
 * Clock Configuration — Phase 4 Section 4.3.1
 * ============================================================================ */

#define BOARD_XTAL_FREQ_HZ         24000000UL   /* 24 MHz main oscillator (Y1) */
#define BOARD_M4_CORE_FREQ_HZ      800000000UL  /* 800 MHz Cortex-M4F */
#define BOARD_IPG_FREQ_HZ          133000000UL  /* 133 MHz IPG bus (AHB) */
#define BOARD_PERCLK_FREQ_HZ       66000000UL   /* 66 MHz peripheral clock */
#define BOARD_SPI_FREQ_HZ          50000000UL   /* 50 MHz SPI bus clock root */
#define BOARD_UART_FREQ_HZ         80000000UL   /* 80 MHz UART clock root */
#define BOARD_CAN_CLK_FREQ_HZ      80000000UL   /* 80 MHz CAN-FD clock root */
#define BOARD_I2C_FREQ_HZ          400000       /* 400 kHz I2C fast mode */

/* ============================================================================
 * GPIO Pin Assignments — Phase 2 Section 2.2.5, Phase 4 Section 4.2.3
 * ============================================================================ */

/* GPIO1 — System Control (PCIe, USB) */
#define PIN_PCIE_RST       GPIO1, 10   /* GPIO1_IO10 — PCIe reset (active-low) */
#define PIN_PCIE_WAKE      GPIO1, 11   /* GPIO1_IO11 — PCIe WAKE# (active-low) */
#define PIN_PCIE_CLKREQ    GPIO1, 12   /* GPIO1_IO12 — PCIe CLKREQ# (active-low) */
#define PIN_USB_HOST       GPIO1, 13   /* GPIO1_IO13 — USB host/device select */

/* GPIO3 — Sensor / Expansion (CAN, Camera, I2C mux) */
#define PIN_I2C_MUX_RST    GPIO3, 0    /* GPIO3_IO00 — I2C mux reset (active-low) */
#define PIN_CAM_RST        GPIO3, 1    /* GPIO3_IO01 — Camera sensor reset */
#define PIN_CAM_PWDN       GPIO3, 2    /* GPIO3_IO02 — Camera sensor power-down */
#define PIN_CAN1_EN        GPIO3, 3    /* GPIO3_IO03 — CAN-FD #1 enable (MCP2518FD RST) */
#define PIN_CAN2_EN        GPIO3, 4    /* GPIO3_IO04 — CAN-FD #2 enable (MCP2518FD RST) */
#define PIN_CAN1_STB       GPIO3, 5    /* GPIO3_IO05 — CAN-FD #1 standby */
#define PIN_CAN2_STB       GPIO3, 6    /* GPIO3_IO06 — CAN-FD #2 standby */

/* GPIO4 — Power / Status LEDs */
#define PIN_LED_STATUS     GPIO4, 0    /* GPIO4_IO00 — Status LED (green) */
#define PIN_LED_ERROR      GPIO4, 1    /* GPIO4_IO01 — Error LED (red) */
#define PIN_LED_ACT        GPIO4, 2    /* GPIO4_IO02 — Activity LED (blue) */
#define PIN_POE_STATUS     GPIO4, 3    /* GPIO4_IO03 — PoE+ status input */
#define PIN_RTC_IRQ        GPIO4, 4    /* GPIO4_IO04 — RTC interrupt (active-low) */
#define PIN_NPU_IRQ        GPIO4, 5    /* GPIO4_IO05 — Hailo-8 NPU interrupt */
#define PIN_WATCHDOG       GPIO4, 6    /* GPIO4_IO06 — External watchdog feed */

/* ============================================================================
 * SPI Chip Selects (for MCP2518FD CAN-FD controllers)
 * ============================================================================ */

#define SPI2_CS_CAN0       0           /* ECSPI2 CS0 — CAN-FD controller #0 */
#define SPI3_CS_CAN1       0           /* ECSPI3 CS0 — CAN-FD controller #1 */

/* ============================================================================
 * I2C Addresses
 * ============================================================================ */

#define I2C_ADDR_PCA9450   0x08        /* PMIC (U7) — 7-bit address */
#define I2C_ADDR_PCF2129   0x51        /* RTC (U8) — 7-bit address */
#define I2C_ADDR_TCA9548   0x70        /* I2C mux (U10) — 7-bit address */

/* ============================================================================
 * CAN-FD Configuration — Phase 4 Section 4.4
 * ============================================================================ */

/* Nominal (arbitration) phase: 500 kbps @ 80 MHz CAN clock
 *   BRP = 8, TSEG1 = 15, TSEG2 = 4, SJW = 4
 *   Bit time = (1 + TSEG1 + TSEG2) * BRP / fclk = 20 * 8 / 80MHz = 2 µs
 */
#define CAN_NOMINAL_BRP       8
#define CAN_NOMINAL_TSEG1    15
#define CAN_NOMINAL_TSEG2     4
#define CAN_NOMINAL_SJW       4

/* Data phase: 2 Mbps (with Bit Rate Switch)
 *   BRP = 2, TSEG1 = 15, TSEG2 = 4, SJW = 4
 *   Bit time = (1 + TSEG1 + TSEG2) * BRP / fclk = 20 * 2 / 80MHz = 0.5 µs
 */
#define CAN_DATA_BRP          2
#define CAN_DATA_TSEG1       15
#define CAN_DATA_TSEG2        4
#define CAN_DATA_SJW          4

/* CAN bus IDs (standard 11-bit) */
#define CAN_ID_HEARTBEAT      0x100     /* M4→A72 heartbeat message */
#define CAN_ID_SENSOR_DATA    0x200     /* CAN sensor data forwarding */
#define CAN_ID_MODBUS_CMD     0x300     /* Modbus-over-CAN command */
#define CAN_ID_ACK            0x400     /* Acknowledgment frame */

/* ============================================================================
 * MU Mailbox Protocol — A72 ↔ M4 Communication
 * ============================================================================ */

/* MU message types (upper 8 bits of 32-bit message) */
#define MU_MSG_TYPE_HEARTBEAT   0x01   /* M4 heartbeat to A72 */
#define MU_MSG_TYPE_CAN_RX      0x02   /* CAN frame received → A72 */
#define MU_MSG_TYPE_CAN_TX      0x03   /* A72 → M4: transmit CAN frame */
#define MU_MSG_TYPE_MODBUS_RX   0x04   /* Modbus response → A72 */
#define MU_MSG_TYPE_MODBUS_TX   0x05   /* A72 → M4: Modbus request */
#define MU_MSG_TYPE_POWER       0x06   /* A72 → M4: power state command */
#define MU_MSG_TYPE_STATUS      0x07   /* M4 → A72: status report */

/* Message format: [TYPE(8)][LEN(8)][DATA(16)] */
#define MU_MSG_TYPE_SHIFT       24
#define MU_MSG_LEN_SHIFT        16
#define MU_MSG_DATA_MASK        0xFFFF

/* ============================================================================
 * LED Control Macros
 * ============================================================================ */

#define LED_STATUS_ON()   mmio_write32(GPIO4_BASE + GPIO_DR, \
                              mmio_read32(GPIO4_BASE + GPIO_DR) | (1U << 0))
#define LED_STATUS_OFF()  mmio_write32(GPIO4_BASE + GPIO_DR, \
                              mmio_read32(GPIO4_BASE + GPIO_DR) & ~(1U << 0))
#define LED_STATUS_TOGGLE() mmio_write32(GPIO4_BASE + GPIO_DR, \
                              mmio_read32(GPIO4_BASE + GPIO_DR) ^ (1U << 0))

#define LED_ERROR_ON()    mmio_write32(GPIO4_BASE + GPIO_DR, \
                              mmio_read32(GPIO4_BASE + GPIO_DR) | (1U << 1))
#define LED_ERROR_OFF()   mmio_write32(GPIO4_BASE + GPIO_DR, \
                              mmio_read32(GPIO4_BASE + GPIO_DR) & ~(1U << 1))

#define LED_ACT_ON()      mmio_write32(GPIO4_BASE + GPIO_DR, \
                              mmio_read32(GPIO4_BASE + GPIO_DR) | (1U << 2))
#define LED_ACT_OFF()     mmio_write32(GPIO4_BASE + GPIO_DR, \
                              mmio_read32(GPIO4_BASE + GPIO_DR) & ~(1U << 2))
#define LED_ACT_TOGGLE()  mmio_write32(GPIO4_BASE + GPIO_DR, \
                              mmio_read32(GPIO4_BASE + GPIO_DR) ^ (1U << 2))

/* ============================================================================
 * FreeRTOS Task Priorities
 * ============================================================================ */

#define TASK_PRIORITY_IDLE          0
#define TASK_PRIORITY_CAN_RX        4   /* Highest — real-time CAN reception */
#define TASK_PRIORITY_CAN_TX        3   /* CAN transmission */
#define TASK_PRIORITY_MODBUS        3   /* Modbus RTU processing */
#define TASK_PRIORITY_MU_RX         2   /* MU mailbox receive from A72 */
#define TASK_PRIORITY_HEARTBEAT     1   /* Heartbeat / watchdog */
#define TASK_PRIORITY_STATUS_LED    1   /* LED blink task */

/* Stack sizes (in 32-bit words) */
#define TASK_STACK_CAN_RX        512
#define TASK_STACK_CAN_TX        256
#define TASK_STACK_MODBUS        512
#define TASK_STACK_MU_RX         256
#define TASK_STACK_HEARTBEAT     128
#define TASK_STACK_STATUS_LED     128

/* ============================================================================
 * M4 TCM Memory Layout (128KB available)
 * ============================================================================ */

#define M4_TCM_BASE          0x007E0000UL
#define M4_TCM_SIZE          0x00020000UL   /* 128KB */

/* Memory partitions within TCM */
#define M4_CODE_START        0x007E0000UL
#define M4_CODE_SIZE         0x00018000UL   /* 96KB code */
#define M4_DATA_START        0x007F8000UL
#define M4_DATA_SIZE         0x00008000UL   /* 32KB data */
#define M4_STACK_START       0x007FF000UL
#define M4_STACK_SIZE        0x00001000UL   /* 4KB stack (grows down) */

#endif /* EAG7000_BOARD_H */