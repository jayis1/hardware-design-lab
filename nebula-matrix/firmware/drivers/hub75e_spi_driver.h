/**
 * hub75e_spi_driver.h — SPI Pixel Stream Driver for FPGA HUB75E Interface
 *
 * Provides high-speed DMA-driven SPI communication from STM32H7 to the
 * FPGA's pixel input FIFO. Transmits 48-bit pixels (16-bit RGB) with
 * X/Y addressing at 50 MHz over SPI2.
 */

#ifndef HUB75E_SPI_DRIVER_H
#define HUB75E_SPI_DRIVER_H

#include "stm32h7xx_hal.h"
#include "board.h"
#include <stdint.h>
#include <stdbool.h>

/* =========================================================================
 * Data Structures
 * ========================================================================= */

/* Pixel data structure for queue-based transmission */
typedef struct {
    uint16_t x;       /* X coordinate (0-255) */
    uint16_t y;       /* Y coordinate (0-255) */
    uint16_t r;       /* Red channel (0-65535) */
    uint16_t g;       /* Green channel (0-65535) */
    uint16_t b;       /* Blue channel (0-65535) */
} pixel_data_t;

/* FPGA command structure */
typedef struct {
    uint16_t addr;    /* Register address */
    uint32_t data;    /* Register data */
    bool     is_write;/* true = write, false = read */
} fpga_cmd_t;

/* System event types */
typedef enum {
    EVENT_FPGA_DONE,
    EVENT_FPGA_IRQ,
    EVENT_HDMI_INT,
    EVENT_ETH_LINK_UP,
    EVENT_ETH_LINK_DOWN,
    EVENT_USB_CONFIGURED,
    EVENT_SD_INSERTED,
    EVENT_SD_REMOVED,
    EVENT_THERMAL_WARN,
    EVENT_THERMAL_CRIT,
    EVENT_PMIC_FAULT,
} system_event_t;

/* =========================================================================
 * SPI Pixel Stream Protocol Constants
 * ========================================================================= */

/* Command field in upper 4 bits of each 32-bit SPI word */
#define SPI_CMD_PIXEL           0x1   /* Pixel data follows (X, Y address) */
#define SPI_CMD_FRAME_END       0x2   /* End of frame marker */
#define SPI_CMD_FRAME_START     0x3   /* Start of frame marker */
#define SPI_CMD_CONFIG          0x4   /* Configuration data follows */
#define SPI_CMD_NOP             0xF   /* No operation / idle */

/* Buffer size for SPI DMA transfers */
#define SPI_PIXEL_BUF_SIZE      2048  /* 32-bit words (8 KB) */
#define SPI_DMA_TIMEOUT_MS      100

/* =========================================================================
 * Function Prototypes
 * ========================================================================= */

/**
 * Initialize SPI2 for FPGA pixel streaming.
 * Configures 50 MHz, 32-bit word, MSB first, DMA-driven.
 * Must be called after GPIO_Init() and SystemClock_Config().
 *
 * @param hspi  Pointer to SPI handle (will be initialized)
 */
void fpga_spi_init(SPI_HandleTypeDef *hspi);

/**
 * Send a single pixel to the FPGA via SPI.
 * Pixels are buffered and sent via DMA when buffer is full
 * or on explicit flush.
 *
 * @param x  X coordinate (0 to matrix_width-1)
 * @param y  Y coordinate (0 to matrix_height-1)
 * @param r  Red channel value (0-65535, 16-bit)
 * @param g  Green channel value (0-65535, 16-bit)
 * @param b  Blue channel value (0-65535, 16-bit)
 * @return 0 on success, -1 if buffer full and flush failed
 */
int fpga_spi_send_pixel(uint16_t x, uint16_t y,
                         uint16_t r, uint16_t g, uint16_t b);

/**
 * Send frame end marker to FPGA.
 * Flushes any buffered pixels first, then sends FRAME_END command.
 */
void fpga_spi_send_frame_end(void);

/**
 * Send frame start marker to FPGA.
 * Indicates beginning of a new frame.
 */
void fpga_spi_send_frame_start(void);

/**
 * Flush the SPI pixel buffer to FPGA via DMA.
 * Called automatically when buffer is full, or can be
 * called explicitly to force transmission.
 */
void fpga_spi_flush_buffer(void);

/**
 * Check if SPI transfer is currently active.
 * @return true if DMA transfer in progress
 */
bool fpga_spi_is_busy(void);

/**
 * Get count of pixels sent since last reset.
 * @return pixel count
 */
uint32_t fpga_spi_get_pixel_count(void);

/**
 * Get count of SPI errors since last reset.
 * @return error count
 */
uint32_t fpga_spi_get_error_count(void);

/**
 * Reset pixel and error counters.
 */
void fpga_spi_reset_counters(void);

#endif /* HUB75E_SPI_DRIVER_H */
