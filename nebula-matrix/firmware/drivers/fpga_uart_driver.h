/**
 * fpga_uart_driver.h — UART Command Interface Driver for FPGA Communication
 *
 * Provides a reliable UART-based command/control channel between the
 * STM32H7 and the FPGA. Used for register read/write, gamma LUT upload,
 * status polling, and configuration.
 *
 * Protocol: 115200 baud, 8N1, framed with sync byte and CRC-8
 */

#ifndef FPGA_UART_DRIVER_H
#define FPGA_UART_DRIVER_H

#include "stm32h7xx_hal.h"
#include "board.h"
#include <stdint.h>
#include <stdbool.h>

/* =========================================================================
 * UART Protocol Constants
 * ========================================================================= */

#define FPGA_UART_BAUDRATE      115200
#define FPGA_UART_SYNC_BYTE     0xA5

/* Command codes */
#define UART_CMD_NOP            0x00
#define UART_CMD_READ           0x01
#define UART_CMD_WRITE          0x02
#define UART_CMD_BURST_READ     0x03
#define UART_CMD_BURST_WRITE    0x04
#define UART_CMD_GAMMA_WR       0x10
#define UART_CMD_GAMMA_RD       0x11
#define UART_CMD_FB_SWAP        0x20
#define UART_CMD_FB_SELECT      0x21
#define UART_CMD_RESET          0x30
#define UART_CMD_TEST_PATTERN   0x31
#define UART_CMD_PING           0xFF

/* Response status codes */
#define UART_STATUS_OK              0x00
#define UART_STATUS_INVALID_CMD     0x01
#define UART_STATUS_INVALID_ADDR    0x02
#define UART_STATUS_CRC_ERROR       0x03
#define UART_STATUS_WRITE_PROTECTED 0x04
#define UART_STATUS_BUSY            0x05

/* Timing */
#define UART_RESPONSE_TIMEOUT_MS    50
#define UART_RETRY_COUNT            3

/* =========================================================================
 * Function Prototypes
 * ========================================================================= */

/**
 * Initialize UART1 for FPGA command interface.
 * Configures 115200 baud, 8N1, with interrupt-driven RX.
 *
 * @param huart  Pointer to UART handle
 */
void fpga_uart_init(UART_HandleTypeDef *huart);

/**
 * Read a 32-bit register from the FPGA.
 *
 * @param addr   Register address (16-bit)
 * @param value  Output: register value
 * @return 0 on success, negative on error
 */
int fpga_read_register(uint16_t addr, uint32_t *value);

/**
 * Write a 32-bit value to an FPGA register.
 *
 * @param addr   Register address (16-bit)
 * @param value  Value to write
 * @return 0 on success, negative on error
 */
int fpga_write_register(uint16_t addr, uint32_t value);

/**
 * Burst read multiple registers from FPGA.
 *
 * @param start_addr  Starting register address
 * @param count       Number of registers to read
 * @param buffer      Output buffer (must be count * 4 bytes)
 * @return 0 on success, negative on error
 */
int fpga_burst_read(uint16_t start_addr, uint8_t count, uint32_t *buffer);

/**
 * Burst write multiple registers to FPGA.
 *
 * @param start_addr  Starting register address
 * @param count       Number of registers to write
 * @param buffer      Data buffer (count * 4 bytes)
 * @return 0 on success, negative on error
 */
int fpga_burst_write(uint16_t start_addr, uint8_t count, const uint32_t *buffer);

/**
 * Write a full gamma LUT (256 entries × 16-bit) to FPGA.
 *
 * @param channel  0=Red, 1=Green, 2=Blue
 * @param lut      Pointer to 256 uint16_t values
 * @return 0 on success, negative on error
 */
int fpga_write_gamma_lut(uint8_t channel, const uint16_t *lut);

/**
 * Read a full gamma LUT from FPGA.
 *
 * @param channel  0=Red, 1=Green, 2=Blue
 * @param lut      Output buffer (256 uint16_t values)
 * @return 0 on success, negative on error
 */
int fpga_read_gamma_lut(uint8_t channel, uint16_t *lut);

/**
 * Load default linear gamma LUTs (1:1 mapping) to FPGA.
 * Called during initialization.
 *
 * @return 0 on success, negative on error
 */
int fpga_load_default_gamma_luts(void);

/**
 * Swap frame buffers (atomic flip).
 *
 * @return 0 on success, negative on error
 */
int fpga_swap_framebuffer(void);

/**
 * Select active write buffer.
 *
 * @param fb  0 = FB0, 1 = FB1
 * @return 0 on success, negative on error
 */
int fpga_select_framebuffer(uint8_t fb);

/**
 * Soft reset the FPGA pixel processing pipeline.
 *
 * @return 0 on success, negative on error
 */
int fpga_soft_reset(void);

/**
 * Enable FPGA test pattern.
 *
 * @param pattern  0=color bars, 1=gradient, 2=grid, 3=white
 * @return 0 on success, negative on error
 */
int fpga_enable_test_pattern(uint8_t pattern);

/**
 * Ping the FPGA and verify communication.
 *
 * @param fpga_id     Output: FPGA design ID (should be 0x4E454255)
 * @param fpga_version Output: FPGA firmware version
 * @return 0 on success, negative on error
 */
int fpga_ping(uint32_t *fpga_id, uint32_t *fpga_version);

/**
 * Poll FPGA status register.
 * Convenience wrapper for reading FPGA_REG_STATUS.
 *
 * @param status  Output: status register value
 * @return 0 on success, negative on error
 */
int fpga_poll_status(uint32_t *status);

/**
 * Reprogram FPGA from SD card bitstream.
 * Called when FPGA fails to configure from onboard flash.
 *
 * @return 0 on success, negative on error
 */
int fpga_reprogram_from_sd(void);

/**
 * Calculate CRC-8 for UART protocol.
 * Polynomial: x^8 + x^5 + x^4 + 1 (CRC-8/DVB-S2)
 *
 * @param data  Data buffer
 * @param len   Buffer length
 * @return CRC-8 value
 */
uint8_t fpga_uart_crc8(const uint8_t *data, size_t len);

#endif /* FPGA_UART_DRIVER_H */
