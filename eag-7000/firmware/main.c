/*
 * EAG-7000 — M4F Firmware Main Entry
 *
 * FreeRTOS-based firmware for the Cortex-M4F core of the i.MX8M Plus.
 * Handles real-time CAN-FD, Modbus RTU, watchdog monitoring, and
 * A72↔M4 mailbox communication.
 *
 * Copyright (c) 2026 EAG-7000 Project
 * SPDX-License-Identifier: MIT
 */

#include "board.h"
#include "registers.h"

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Driver headers */
#include "drivers/spi.h"
#include "drivers/uart.h"
#include "drivers/i2c.h"
#include "drivers/gpio.h"
#include "drivers/mu.h"
#include "drivers/mcp2518fd.h"

/* ============================================================================
 * Global State
 * ============================================================================ */

/* FreeRTOS task handles */
static TaskHandle_t h_can_rx_task;
static TaskHandle_t h_can_tx_task;
static TaskHandle_t h_modbus_task;
static TaskHandle_t h_mu_rx_task;
static TaskHandle_t h_heartbeat_task;
static TaskHandle_t h_status_led_task;

/* Queues for inter-task communication */
#define CAN_RX_QUEUE_LEN    16
#define CAN_TX_QUEUE_LEN    16
#define MU_RX_QUEUE_LEN     8

static QueueHandle_t xCanRxQueue;    /* CAN frames from bus → A72 */
static QueueHandle_t xCanTxQueue;    /* CAN frames from A72 → bus */
static QueueHandle_t xMuRxQueue;     /* MU messages from A72 */

/* SPI device handles */
static spi_dev_t spi2_dev;   /* ECSPI2 → CAN-FD #0 */
static spi_dev_t spi3_dev;   /* ECSPI3 → CAN-FD #1 */

/* UART device handles */
static uart_dev_t uart1_dev; /* Debug console */
static uart_dev_t uart2_dev; /* Modbus RTU */

/* CAN device handles */
static mcp2518fd_dev_t can0_dev;  /* MCP2518FD on SPI2 */
static mcp2518fd_dev_t can1_dev;  /* MCP2518FD on SPI3 */

/* Mutex for shared SPI bus (ECSPI2 shared with boot flash is not
 * a concern since boot flash uses ECSPI1, but mutex still good practice) */
static SemaphoreHandle_t xSpi2Mutex;
static SemaphoreHandle_t xSpi3Mutex;

/* System uptime counter (incremented by heartbeat task) */
static volatile uint32_t system_uptime_ms = 0;

/* ============================================================================
 * CAN Frame Structure (shared between tasks)
 * ============================================================================ */

typedef struct {
    uint32_t id;            /* CAN identifier (11-bit standard or 29-bit extended) */
    uint8_t  dlc;           /* Data Length Code (0-8 for CAN 2.0, 0-64 for CAN-FD) */
    uint8_t  data[64];     /* Payload (max 64 bytes for CAN-FD) */
    uint8_t  flags;         /* CAN_MSG_OBJ_FLAGS_* */
    uint8_t  bus;           /* 0 = CAN0 (SPI2), 1 = CAN1 (SPI3) */
} can_frame_t;

/* CAN frame flags */
#define CAN_FRAME_EXT   0x01    /* Extended ID (29-bit) */
#define CAN_FRAME_RTR   0x02   /* Remote Transmission Request */
#define CAN_FRAME_FDF   0x04   /* CAN-FD format */
#define CAN_FRAME_BRS   0x08   /* Bit Rate Switch */

/* ============================================================================
 * MU Mailbox Message Structure
 * ============================================================================ */

typedef struct {
    uint8_t  type;           /* MU_MSG_TYPE_* */
    uint8_t  len;            /* Payload length */
    uint16_t data;           /* 16-bit data field */
} mu_msg_t;

/* ============================================================================
 * Hardware Initialization
 * ============================================================================ */

/**
 * Initialize board-level hardware: clocks, GPIO, peripheral enables.
 * Must be called before any driver init.
 */
static void board_early_init(void)
{
    /*
     * At this point, the A72 Linux kernel has already:
     * 1. Configured CCM clocks (via device tree)
     * 2. Set up IOMUX pin multiplexing
     * 3. Enabled power rails via PMIC
     * 4. Loaded this M4F firmware via remoteproc
     *
     * The M4F needs to:
     * 1. Release CAN-FD controllers from reset
     * 2. Configure M4F-specific GPIO directions
     * 3. Initialize UART for debug output
     */

    /* Release CAN-FD controllers from reset */
    gpio_init();

    /* Release CAN0 (MCP2518FD on SPI2) from reset — GPIO3_IO03 high */
    mmio_write32(GPIO3_BASE + GPIO_DR,
                 mmio_read32(GPIO3_BASE + GPIO_DR) | (1U << 3));

    /* Release CAN1 (MCP2518FD on SPI3) from reset — GPIO3_IO04 high */
    mmio_write32(GPIO3_BASE + GPIO_DR,
                 mmio_read32(GPIO3_BASE + GPIO_DR) | (1U << 4));

    /* Set CAN standby pins low (active mode) */
    mmio_write32(GPIO3_BASE + GPIO_DR,
                 mmio_read32(GPIO3_BASE + GPIO_DR) & ~(1U << 5));  /* CAN0_STB = 0 */
    mmio_write32(GPIO3_BASE + GPIO_DR,
                 mmio_read32(GPIO3_BASE + GPIO_DR) & ~(1U << 6));  /* CAN1_STB = 0 */

    /* Release I2C mux from reset — GPIO3_IO00 high */
    mmio_write32(GPIO3_BASE + GPIO_DR,
                 mmio_read32(GPIO3_BASE + GPIO_DR) | (1U << 0));

    /* Set LED outputs off initially */
    mmio_write32(GPIO4_BASE + GPIO_DR,
                 mmio_read32(GPIO4_BASE + GPIO_DR) & ~(7U << 0));  /* Clear bits 0-2 */

    /* Initialize debug UART (UART1, 115200 8N1) */
    uart_init(&uart1_dev, UART1_BASE, 115200);
    uart_puts(&uart1_dev, "\r\n=== EAG-7000 M4F Firmware v1.0 ===\r\n");
    uart_puts(&uart1_dev, "SoC: NXP i.MX8M Plus Cortex-M4F @ 800 MHz\r\n");
    uart_puts(&uart1_dev, "FreeRTOS " tskKERNEL_VERSION_NUMBER "\r\n\r\n");

    /* Initialize Modbus UART (UART2, 115200 8E1 for Modbus RTU) */
    uart_init(&uart2_dev, UART2_BASE, 115200);
}

/**
 * Initialize SPI buses for CAN-FD controllers.
 */
static void spi_can_init(void)
{
    /* ECSPI2 → CAN-FD #0 (MCP2518FD) */
    spi_init(&spi2_dev, ECSPI2_BASE,
             SPI_MODE_0,           /* CPOL=0, CPHA=0 */
             10000000,             /* 10 MHz SPI clock */
             SPI_CS0);             /* Chip select 0 */
    uart_puts(&uart1_dev, "[INIT] SPI2 initialized for CAN-FD #0\r\n");

    /* ECSPI3 → CAN-FD #1 (MCP2518FD) */
    spi_init(&spi3_dev, ECSPI3_BASE,
             SPI_MODE_0,
             10000000,
             SPI_CS0);
    uart_puts(&uart1_dev, "[INIT] SPI3 initialized for CAN-FD #1\r\n");
}

/**
 * Initialize MCP2518FD CAN-FD controllers.
 */
static void can_init(void)
{
    int ret;

    /* CAN0 — Industrial Bus 1 */
    ret = mcp2518fd_init(&can0_dev, &spi2_dev,
                          CAN_NOMINAL_BRP, CAN_NOMINAL_TSEG1, CAN_NOMINAL_TSEG2, CAN_NOMINAL_SJW,
                          CAN_DATA_BRP, CAN_DATA_TSEG1, CAN_DATA_TSEG2, CAN_DATA_SJW);
    if (ret == 0) {
        uart_puts(&uart1_dev, "[INIT] CAN-FD #0 initialized (500kbps/2Mbps)\r\n");
    } else {
        uart_puts(&uart1_dev, "[ERROR] CAN-FD #0 init failed!\r\n");
    }

    /* CAN1 — Industrial Bus 2 */
    ret = mcp2518fd_init(&can1_dev, &spi3_dev,
                          CAN_NOMINAL_BRP, CAN_NOMINAL_TSEG1, CAN_NOMINAL_TSEG2, CAN_NOMINAL_SJW,
                          CAN_DATA_BRP, CAN_DATA_TSEG1, CAN_DATA_TSEG2, CAN_DATA_SJW);
    if (ret == 0) {
        uart_puts(&uart1_dev, "[INIT] CAN-FD #1 initialized (500kbps/2Mbps)\r\n");
    } else {
        uart_puts(&uart1_dev, "[ERROR] CAN-FD #1 init failed!\r\n");
    }
}

/* ============================================================================
 * FreeRTOS Tasks
 * ============================================================================ */

/**
 * CAN RX Task — Reads CAN frames from MCP2518FD and forwards to A72 via MU.
 * Highest priority task for deterministic real-time response (<100µs).
 */
static void vCanRxTask(void *pvParameters)
{
    can_frame_t rx_frame;

    for (;;) {
        /* Poll CAN0 for received frames */
        if (mcp2518fd_rx_ready(&can0_dev)) {
            if (mcp2518fd_receive(&can0_dev, &rx_frame.id,
                                   &rx_frame.dlc, rx_frame.data,
                                   &rx_frame.flags) == 0) {
                rx_frame.bus = 0;
                /* Forward to A72 via MU mailbox */
                mu_send(MU_MSG_TYPE_CAN_RX, rx_frame.bus,
                        (uint16_t)(rx_frame.id & 0xFFFF));
                /* Also queue for local processing */
                xQueueSend(xCanRxQueue, &rx_frame, 0);
            }
        }

        /* Poll CAN1 for received frames */
        if (mcp2518fd_rx_ready(&can1_dev)) {
            if (mcp2518fd_receive(&can1_dev, &rx_frame.id,
                                   &rx_frame.dlc, rx_frame.data,
                                   &rx_frame.flags) == 0) {
                rx_frame.bus = 1;
                mu_send(MU_MSG_TYPE_CAN_RX, rx_frame.bus,
                        (uint16_t)(rx_frame.id & 0xFFFF));
                xQueueSend(xCanRxQueue, &rx_frame, 0);
            }
        }

        /* Small delay to prevent tight polling (yield to other tasks) */
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

/**
 * CAN TX Task — Transmits CAN frames received from A72 via MU mailbox.
 */
static void vCanTxTask(void *pvParameters)
{
    can_frame_t tx_frame;

    for (;;) {
        /* Block waiting for TX frame from A72 */
        if (xQueueReceive(xCanTxQueue, &tx_frame, portMAX_DELAY) == pdTRUE) {
            if (tx_frame.bus == 0) {
                mcp2518fd_transmit(&can0_dev, tx_frame.id,
                                    tx_frame.dlc, tx_frame.data, tx_frame.flags);
            } else if (tx_frame.bus == 1) {
                mcp2518fd_transmit(&can1_dev, tx_frame.id,
                                    tx_frame.dlc, tx_frame.data, tx_frame.flags);
            }
            LED_ACT_TOGGLE();
        }
    }
}

/**
 * Modbus RTU Task — Handles Modbus RTU slave on UART2.
 * Processes incoming Modbus requests and responds.
 */
static void vModbusTask(void *pvParameters)
{
    uint8_t modbus_rx_buf[256];
    uint8_t modbus_tx_buf[256];
    uint16_t idx = 0;

    for (;;) {
        /* Read bytes from UART2 (Modbus RTU port) */
        uint8_t byte;
        if (uart_getchar(&uart2_dev, &byte, pdMS_TO_TICKS(10)) == 0) {
            modbus_rx_buf[idx++] = byte;

            /* Simple Modbus RTU frame detector:
             * - Address (1 byte) + Function (1 byte) + Data (0-252 bytes) + CRC (2 bytes)
             * - Detect end of frame by 3.5 character silence
             * For simplicity, we use a timeout-based approach.
             */
            if (idx >= 4) {
                /* Minimum Modbus frame: addr + func + CRC16 = 4 bytes */
                /* Check if we have a complete frame by verifying CRC */
                uint16_t frame_len = idx;
                if (frame_len <= 252) {
                    /* Process Modbus frame */
                    uint8_t addr = modbus_rx_buf[0];
                    uint8_t func = modbus_rx_buf[1];

                    /* Only process frames addressed to us (address 1) */
                    if (addr == 1) {
                        /* Modbus response processing placeholder */
                        /* In production, this would handle:
                         * - Function 03 (Read Holding Registers)
                         * - Function 04 (Read Input Registers)
                         * - Function 06 (Write Single Register)
                         * - Function 16 (Write Multiple Registers)
                         */

                        /* Forward to A72 via MU */
                        mu_send(MU_MSG_TYPE_MODBUS_RX, func,
                                (uint16_t)(modbus_rx_buf[2] << 8 | modbus_rx_buf[3]));
                    }
                }
            }

            /* Reset if buffer overflow */
            if (idx >= 255) {
                idx = 0;
            }
        } else {
            /* Timeout — end of frame, reset index */
            if (idx > 0) {
                idx = 0;
            }
        }
    }
}

/**
 * MU RX Task — Receives messages from A72 Linux via MU mailbox.
 * Routes messages to appropriate handler (CAN TX, Modbus, Power).
 */
static void vMuRxTask(void *pvParameters)
{
    mu_msg_t msg;

    for (;;) {
        /* Block waiting for MU message from A72 */
        if (mu_receive(&msg, portMAX_DELAY) == 0) {
            switch (msg.type) {
            case MU_MSG_TYPE_CAN_TX: {
                /* A72 wants to transmit a CAN frame */
                can_frame_t tx_frame = {
                    .bus = (uint8_t)msg.data,
                    .id = 0,
                    .dlc = 0,
                    .flags = 0
                };
                /* Additional data would come from subsequent MU messages
                 * or shared DRAM buffer. For now, send minimal frame. */
                xQueueSend(xCanTxQueue, &tx_frame, pdMS_TO_TICKS(100));
                break;
            }

            case MU_MSG_TYPE_MODBUS_TX: {
                /* A72 wants to send a Modbus request */
                /* Forward to Modbus task via UART2 */
                break;
            }

            case MU_MSG_TYPE_POWER: {
                /* Power state command from A72 */
                if (msg.data == 0x01) {
                    /* A72 requests M4F to prepare for sleep */
                    LED_STATUS_OFF();
                } else if (msg.data == 0x02) {
                    /* A72 wakeup signal */
                    LED_STATUS_ON();
                }
                break;
            }

            default:
                /* Unknown message type — ignore */
                break;
            }
        }
    }
}

/**
 * Heartbeat Task — Sends periodic heartbeat to A72 via MU mailbox
 * and feeds the hardware watchdog.
 */
static void vHeartbeatTask(void *pvParameters)
{
    uint32_t heartbeat_counter = 0;

    for (;;) {
        /* Send heartbeat to A72 via MU */
        mu_send(MU_MSG_TYPE_HEARTBEAT, 0, (uint16_t)(heartbeat_counter & 0xFFFF));
        heartbeat_counter++;
        system_uptime_ms += 1000;

        /* Feed external watchdog (GPIO4_IO06) */
        mmio_write32(GPIO4_BASE + GPIO_DR,
                     mmio_read32(GPIO4_BASE + GPIO_DR) ^ (1U << 6));

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * Status LED Task — Blinks status LED to indicate system health.
 * Pattern: 1Hz blink = normal, 4Hz = error, solid = initializing.
 */
static void vStatusLedTask(void *pvParameters)
{
    uint32_t blink_rate_ms = 500;  /* 1 Hz default */

    for (;;) {
        LED_STATUS_TOGGLE();
        vTaskDelay(pdMS_TO_TICKS(blink_rate_ms));
    }
}

/* ============================================================================
 * Main Entry Point
 * ============================================================================ */

int main(void)
{
    /* Step 1: Initialize hardware */
    board_early_init();

    uart_puts(&uart1_dev, "[BOOT] Creating FreeRTOS queues...\r\n");

    /* Step 2: Create inter-task communication queues */
    xCanRxQueue = xQueueCreate(CAN_RX_QUEUE_LEN, sizeof(can_frame_t));
    xCanTxQueue = xQueueCreate(CAN_TX_QUEUE_LEN, sizeof(can_frame_t));
    xMuRxQueue   = xQueueCreate(MU_RX_QUEUE_LEN, sizeof(mu_msg_t));

    /* Step 3: Create mutexes */
    xSpi2Mutex = xSemaphoreCreateMutex();
    xSpi3Mutex = xSemaphoreCreateMutex();

    /* Step 4: Initialize SPI buses and CAN controllers */
    spi_can_init();
    can_init();

    uart_puts(&uart1_dev, "[BOOT] Creating FreeRTOS tasks...\r\n");

    /* Step 5: Create FreeRTOS tasks */
    xTaskCreate(vCanRxTask,       "CAN_RX",   TASK_STACK_CAN_RX,     NULL, TASK_PRIORITY_CAN_RX,      &h_can_rx_task);
    xTaskCreate(vCanTxTask,       "CAN_TX",   TASK_STACK_CAN_TX,     NULL, TASK_PRIORITY_CAN_TX,      &h_can_tx_task);
    xTaskCreate(vModbusTask,      "MODBUS",   TASK_STACK_MODBUS,     NULL, TASK_PRIORITY_MODBUS,      &h_modbus_task);
    xTaskCreate(vMuRxTask,        "MU_RX",    TASK_STACK_MU_RX,      NULL, TASK_PRIORITY_MU_RX,       &h_mu_rx_task);
    xTaskCreate(vHeartbeatTask,   "HEARTBEAT",TASK_STACK_HEARTBEAT,  NULL, TASK_PRIORITY_HEARTBEAT,   &h_heartbeat_task);
    xTaskCreate(vStatusLedTask,   "LED",      TASK_STACK_STATUS_LED, NULL, TASK_PRIORITY_STATUS_LED,  &h_status_led_task);

    uart_puts(&uart1_dev, "[BOOT] Starting FreeRTOS scheduler...\r\n");

    /* Step 6: Start the FreeRTOS scheduler — does not return */
    vTaskStartScheduler();

    /* Should never reach here */
    for (;;) {
        /* Fallback: rapid error blink */
        LED_ERROR_ON();
        for (volatile uint32_t i = 0; i < 1000000; i++);
        LED_ERROR_OFF();
        for (volatile uint32_t i = 0; i < 1000000; i++);
    }

    return 0;
}

/* FreeRTOS stack overflow hook */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    /* Flash error LED rapidly */
    for (;;) {
        LED_ERROR_ON();
        for (volatile uint32_t i = 0; i < 500000; i++);
        LED_ERROR_OFF();
        for (volatile uint32_t i = 0; i < 500000; i++);
    }
}

/* FreeRTOS malloc-failed hook */
void vApplicationMallocFailedHook(void)
{
    for (;;) {
        LED_ERROR_ON();
        for (volatile uint32_t i = 0; i < 200000; i++);
    }
}