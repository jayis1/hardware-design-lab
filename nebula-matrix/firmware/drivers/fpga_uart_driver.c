/**
 * fpga_uart_driver.c — UART Command Interface Driver Implementation
 *
 * Reliable UART-based register read/write protocol between STM32H7 and FPGA.
 * Uses framed packets with sync byte, CRC-8 error detection, and retry logic.
 *
 * Frame format (host → FPGA):
 *   [SYNC(0xA5)] [CMD(1B)] [ADDR_HI(1B)] [ADDR_LO(1B)] [DATA(0-4B)] [CRC8(1B)]
 *
 * Response format (FPGA → host):
 *   [SYNC(0xA5)] [STATUS(1B)] [ADDR_HI(1B)] [ADDR_LO(1B)] [DATA(0-4B)] [CRC8(1B)]
 *
 * CRC-8 polynomial: 0x31 (x^8 + x^5 + x^4 + 1), initial value 0xFF
 */

#include "fpga_uart_driver.h"
#include "registers.h"
#include <string.h>

/* =========================================================================
 * Static Variables
 * ========================================================================= */

static UART_HandleTypeDef *g_huart1 = NULL;

/* Receive buffer for FPGA responses (max 9 bytes: SYNC+STATUS+ADDR_HI+ADDR_LO+DATA4+CRC8) */
#define UART_RX_BUF_SIZE    9
static uint8_t g_uart_rx_buf[UART_RX_BUF_SIZE];
static volatile uint8_t g_uart_rx_idx = 0;
static volatile bool g_uart_response_ready = false;
static volatile uint8_t g_uart_response_status = 0;
static volatile uint32_t g_uart_response_data = 0;

/* Statistics */
static volatile uint32_t g_uart_cmd_count = 0;
static volatile uint32_t g_uart_error_count = 0;
static volatile uint32_t g_uart_timeout_count = 0;
static volatile uint32_t g_uart_crc_error_count = 0;

/* =========================================================================
 * CRC-8 Implementation
 * ========================================================================= */

uint8_t fpga_uart_crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFF;  /* Initial value */
    const uint8_t polynomial = 0x31;  /* x^8 + x^5 + x^4 + 1 */

    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ polynomial;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

/* =========================================================================
 * Initialization
 * ========================================================================= */

void fpga_uart_init(UART_HandleTypeDef *huart)
{
    g_huart1 = huart;

    /* Enable USART1 clock */
    __HAL_RCC_USART1_CLK_ENABLE();

    /* Configure USART1: 115200 baud, 8N1 */
    huart->Instance = USART1;
    huart->Init.BaudRate = FPGA_UART_BAUDRATE;
    huart->Init.WordLength = UART_WORDLENGTH_8B;
    huart->Init.StopBits = UART_STOPBITS_1;
    huart->Init.Parity = UART_PARITY_NONE;
    huart->Init.Mode = UART_MODE_TX_RX;
    huart->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart->Init.OverSampling = UART_OVERSAMPLING_16;
    huart->Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart->Init.ClockPrescaler = UART_PRESCALER_DIV1;
    huart->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

    if (HAL_UART_Init(huart) != HAL_OK) {
        g_uart_error_count++;
        return;
    }

    /* Enable UART interrupt for RX */
    HAL_NVIC_SetPriority(USART1_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);

    /* Start listening for FPGA responses */
    g_uart_rx_idx = 0;
    g_uart_response_ready = false;
    HAL_UART_Receive_IT(huart, g_uart_rx_buf, 1);  /* Start with sync byte hunt */
}

/* =========================================================================
 * Low-Level Send/Receive
 * ========================================================================= */

/**
 * Send a command frame to the FPGA.
 *
 * @param cmd      Command byte
 * @param addr     Register address (16-bit)
 * @param data     Data to send (NULL for read commands)
 * @param data_len Length of data in bytes (0, 1, 2, or 4)
 * @return 0 on success, -1 on error
 */
static int uart_send_command(uint8_t cmd, uint16_t addr,
                              const uint8_t *data, uint8_t data_len)
{
    uint8_t frame[9];  /* SYNC + CMD + ADDR_HI + ADDR_LO + DATA(0-4) + CRC8 */
    uint8_t frame_len = 4 + data_len + 1;  /* 4 header + data + 1 CRC */

    frame[0] = FPGA_UART_SYNC_BYTE;
    frame[1] = cmd;
    frame[2] = (addr >> 8) & 0xFF;
    frame[3] = addr & 0xFF;

    if (data != NULL && data_len > 0) {
        memcpy(&frame[4], data, data_len);
    }

    /* Calculate CRC over everything except the CRC byte itself */
    frame[frame_len - 1] = fpga_uart_crc8(frame, frame_len - 1);

    /* Reset response state */
    g_uart_response_ready = false;
    g_uart_rx_idx = 0;

    /* Send frame */
    HAL_StatusTypeDef status = HAL_UART_Transmit(g_huart1, frame, frame_len, 100);
    if (status != HAL_OK) {
        g_uart_error_count++;
        return -1;
    }

    g_uart_cmd_count++;
    return 0;
}

/**
 * Wait for FPGA response with timeout.
 *
 * @param timeout_ms  Maximum wait time in milliseconds
 * @return 0 on success (response received and CRC valid),
 *         -1 on timeout, -2 on CRC error
 */
static int uart_wait_response(uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();

    /* Wait for response or timeout */
    while (!g_uart_response_ready) {
        if (HAL_GetTick() - start > timeout_ms) {
            g_uart_timeout_count++;
            return -1;  /* Timeout */
        }
        /* Yield to allow interrupt processing */
        __WFI();
    }

    /* Verify CRC on received frame */
    uint8_t rx_len = g_uart_rx_idx;
    uint8_t computed_crc = fpga_uart_crc8(g_uart_rx_buf, rx_len - 1);
    uint8_t received_crc = g_uart_rx_buf[rx_len - 1];

    if (computed_crc != received_crc) {
        g_uart_crc_error_count++;
        g_uart_response_ready = false;
        return -2;  /* CRC error */
    }

    return 0;
}

/* =========================================================================
 * Register Read/Write
 * ========================================================================= */

int fpga_read_register(uint16_t addr, uint32_t *value)
{
    int ret;

    for (int attempt = 0; attempt < UART_RETRY_COUNT; attempt++) {
        /* Send READ command (no data) */
        ret = uart_send_command(UART_CMD_READ, addr, NULL, 0);
        if (ret != 0) continue;

        /* Wait for response */
        ret = uart_wait_response(UART_RESPONSE_TIMEOUT_MS);
        if (ret != 0) continue;

        /* Check status */
        if (g_uart_response_status != UART_STATUS_OK) {
            if (g_uart_response_status == UART_STATUS_BUSY) {
                HAL_Delay(1);  /* Retry after short delay */
                continue;
            }
            g_uart_error_count++;
            return -(g_uart_response_status);
        }

        /* Extract 32-bit data from response (bytes 4-7) */
        *value = ((uint32_t)g_uart_rx_buf[4] << 24)
               | ((uint32_t)g_uart_rx_buf[5] << 16)
               | ((uint32_t)g_uart_rx_buf[6] << 8)
               | ((uint32_t)g_uart_rx_buf[7]);

        return 0;
    }

    g_uart_error_count++;
    return -100;  /* All retries exhausted */
}

int fpga_write_register(uint16_t addr, uint32_t value)
{
    int ret;
    uint8_t data[4];

    /* Pack 32-bit value as big-endian */
    data[0] = (value >> 24) & 0xFF;
    data[1] = (value >> 16) & 0xFF;
    data[2] = (value >> 8) & 0xFF;
    data[3] = value & 0xFF;

    for (int attempt = 0; attempt < UART_RETRY_COUNT; attempt++) {
        ret = uart_send_command(UART_CMD_WRITE, addr, data, 4);
        if (ret != 0) continue;

        ret = uart_wait_response(UART_RESPONSE_TIMEOUT_MS);
        if (ret != 0) continue;

        if (g_uart_response_status != UART_STATUS_OK) {
            if (g_uart_response_status == UART_STATUS_BUSY) {
                HAL_Delay(1);
                continue;
            }
            g_uart_error_count++;
            return -(g_uart_response_status);
        }

        return 0;
    }

    g_uart_error_count++;
    return -100;
}

/* =========================================================================
 * Burst Read/Write
 * ========================================================================= */

int fpga_burst_read(uint16_t start_addr, uint8_t count, uint32_t *buffer)
{
    int ret;
    uint8_t data[1] = { count };

    for (int attempt = 0; attempt < UART_RETRY_COUNT; attempt++) {
        ret = uart_send_command(UART_CMD_BURST_READ, start_addr, data, 1);
        if (ret != 0) continue;

        /* For burst reads, we expect multiple response frames.
         * Each frame contains one 32-bit register value.
         * Simplified: read one at a time via loop. */
        for (uint8_t i = 0; i < count; i++) {
            ret = fpga_read_register(start_addr + i, &buffer[i]);
            if (ret != 0) return ret;
        }
        return 0;
    }

    return -100;
}

int fpga_burst_write(uint16_t start_addr, uint8_t count, const uint32_t *buffer)
{
    int ret;

    for (uint8_t i = 0; i < count; i++) {
        for (int attempt = 0; attempt < UART_RETRY_COUNT; attempt++) {
            ret = fpga_write_register(start_addr + i, buffer[i]);
            if (ret == 0) break;
        }
        if (ret != 0) return ret;
    }

    return 0;
}

/* =========================================================================
 * Gamma LUT Operations
 * ========================================================================= */

int fpga_write_gamma_lut(uint8_t channel, const uint16_t *lut)
{
    uint16_t base_addr;

    switch (channel) {
        case 0: base_addr = FPGA_REG_GAMMA_R_LUT; break;
        case 1: base_addr = FPGA_REG_GAMMA_G_LUT; break;
        case 2: base_addr = FPGA_REG_GAMMA_B_LUT; break;
        default: return -1;
    }

    /* Write 256 16-bit entries. Each register holds 32 bits = 2 entries. */
    for (int i = 0; i < 256; i += 2) {
        uint32_t combined = ((uint32_t)lut[i] << 16) | lut[i + 1];
        int ret = fpga_write_register(base_addr + (i / 2), combined);
        if (ret != 0) return ret;
    }

    return 0;
}

int fpga_read_gamma_lut(uint8_t channel, uint16_t *lut)
{
    uint16_t base_addr;

    switch (channel) {
        case 0: base_addr = FPGA_REG_GAMMA_R_LUT; break;
        case 1: base_addr = FPGA_REG_GAMMA_G_LUT; break;
        case 2: base_addr = FPGA_REG_GAMMA_B_LUT; break;
        default: return -1;
    }

    for (int i = 0; i < 256; i += 2) {
        uint32_t combined;
        int ret = fpga_read_register(base_addr + (i / 2), &combined);
        if (ret != 0) return ret;
        lut[i] = (combined >> 16) & 0xFFFF;
        lut[i + 1] = combined & 0xFFFF;
    }

    return 0;
}

int fpga_load_default_gamma_luts(void)
{
    uint16_t linear_lut[256];

    /* Generate linear gamma LUT: output = input (1:1 mapping) */
    for (int i = 0; i < 256; i++) {
        /* 8-bit input → 16-bit output: replicate */
        linear_lut[i] = (uint16_t)((i << 8) | i);
    }

    /* Write to all three channels */
    int ret;
    ret = fpga_write_gamma_lut(0, linear_lut);
    if (ret != 0) return ret;
    ret = fpga_write_gamma_lut(1, linear_lut);
    if (ret != 0) return ret;
    ret = fpga_write_gamma_lut(2, linear_lut);
    if (ret != 0) return ret;

    return 0;
}

/* =========================================================================
 * Convenience Commands
 * ========================================================================= */

int fpga_swap_framebuffer(void)
{
    /* Write any value to FB_SWAP register triggers swap */
    return fpga_write_register(FPGA_REG_FB_SWAP, 1);
}

int fpga_select_framebuffer(uint8_t fb)
{
    return fpga_write_register(FPGA_REG_FB_ACTIVE, fb & 0x01);
}

int fpga_soft_reset(void)
{
    /* Write SOFT_RESET bit to control register */
    uint32_t ctrl;
    int ret = fpga_read_register(FPGA_REG_CONTROL, &ctrl);
    if (ret != 0) return ret;

    ctrl |= FPGA_CTRL_SOFT_RESET;
    ret = fpga_write_register(FPGA_REG_CONTROL, ctrl);
    if (ret != 0) return ret;

    /* Clear soft reset bit after 1ms */
    HAL_Delay(1);
    ctrl &= ~FPGA_CTRL_SOFT_RESET;
    return fpga_write_register(FPGA_REG_CONTROL, ctrl);
}

int fpga_enable_test_pattern(uint8_t pattern)
{
    uint32_t ctrl;
    int ret = fpga_read_register(FPGA_REG_CONTROL, &ctrl);
    if (ret != 0) return ret;

    /* Set input source to test pattern */
    ret = fpga_write_register(FPGA_REG_INPUT_SOURCE, INPUT_SRC_TEST_PATTERN);
    if (ret != 0) return ret;

    /* Enable test pattern and select type */
    ctrl |= FPGA_CTRL_TEST_PATTERN;
    ctrl &= ~(0x18);  /* Clear TEST_PATTERN_SEL bits */
    ctrl |= ((pattern & 0x03) << 3);
    ctrl |= FPGA_CTRL_ENABLE_OUTPUT;

    return fpga_write_register(FPGA_REG_CONTROL, ctrl);
}

int fpga_ping(uint32_t *fpga_id, uint32_t *fpga_version)
{
    int ret;

    /* Send PING command */
    ret = uart_send_command(UART_CMD_PING, 0, NULL, 0);
    if (ret != 0) return ret;

    ret = uart_wait_response(UART_RESPONSE_TIMEOUT_MS);
    if (ret != 0) return ret;

    if (g_uart_response_status != UART_STATUS_OK) {
        return -1;
    }

    /* Response data contains FPGA_ID (bytes 4-7) */
    *fpga_id = ((uint32_t)g_uart_rx_buf[4] << 24)
             | ((uint32_t)g_uart_rx_buf[5] << 16)
             | ((uint32_t)g_uart_rx_buf[6] << 8)
             | ((uint32_t)g_uart_rx_buf[7]);

    /* Read version register separately */
    ret = fpga_read_register(FPGA_REG_VERSION, fpga_version);
    if (ret != 0) return ret;

    return 0;
}

int fpga_poll_status(uint32_t *status)
{
    return fpga_read_register(FPGA_REG_STATUS, status);
}

/* =========================================================================
 * FPGA Reprogramming from SD Card
 * ========================================================================= */

int fpga_reprogram_from_sd(void)
{
    /* This function is called when the FPGA fails to configure from
     * its onboard SPI flash. It reads a new bitstream from the SD card
     * and writes it to the W25Q128JV flash via SPI1.
     *
     * Steps:
     *   1. Hold FPGA in reset (PROGRAM_B low)
     *   2. Erase SPI flash
     *   3. Read bitstream from SD card
     *   4. Write bitstream to SPI flash
     *   5. Release FPGA reset
     *   6. Wait for FPGA DONE
     */

    /* Hold FPGA in reset */
    RESET_ASSERT(FPGA_RESET_N_PORT, FPGA_RESET_N_PIN);
    HAL_Delay(10);

    /* TODO: Implement SPI flash programming via SPI1
     * This requires the W25Q128JV driver which handles:
     *   - Write enable
     *   - Sector erase (4KB sectors) or chip erase
     *   - Page program (256 bytes per page)
     *   - Read status register for busy polling
     *
     * For now, this is a placeholder that would be filled in
     * with the actual flash driver implementation.
     */

    /* Release FPGA reset to trigger reconfiguration */
    RESET_DEASSERT(FPGA_RESET_N_PORT, FPGA_RESET_N_PIN);

    return 0;
}

/* =========================================================================
 * UART RX Interrupt Handler
 * ========================================================================= */

/**
 * UART RX interrupt callback — called for each received byte.
 * Implements a simple state machine to parse FPGA response frames.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != USART1) return;

    uint8_t byte = g_uart_rx_buf[g_uart_rx_idx];

    if (g_uart_rx_idx == 0) {
        /* Hunting for sync byte */
        if (byte == FPGA_UART_SYNC_BYTE) {
            g_uart_rx_idx = 1;
        }
        /* If not sync byte, stay at idx 0 and keep hunting */
    } else {
        /* Receiving frame body */
        g_uart_rx_idx++;

        /* Check if we have a complete frame */
        /* Frame length depends on command type:
         *   NOP/PING response: 6 bytes (SYNC+STATUS+ADDR_HI+ADDR_LO+DATA0+CRC)
         *   READ response: 9 bytes (SYNC+STATUS+ADDR_HI+ADDR_LO+DATA4+CRC)
         *   WRITE response: 6 bytes (SYNC+STATUS+ADDR_HI+ADDR_LO+DATA0+CRC)
         *
         * We determine length from the status byte at index 1:
         *   If status is OK and it's a read response, expect 9 bytes.
         *   Otherwise, expect 6 bytes.
         */

        uint8_t expected_len = 6;  /* Default: short response */

        if (g_uart_rx_idx >= 2) {
            uint8_t status = g_uart_rx_buf[1];
            /* For OK status on read commands, FPGA sends 4 data bytes */
            if (status == UART_STATUS_OK && g_uart_rx_idx >= 6) {
                /* Check if more data expected — we use a simple heuristic:
                 * if we're at byte 6 and the next byte isn't a sync byte,
                 * assume it's a long response. */
                expected_len = 9;
            }
        }

        if (g_uart_rx_idx >= expected_len) {
            /* Complete frame received */
            g_uart_response_status = g_uart_rx_buf[1];
            g_uart_response_ready = true;

            /* Restart sync byte hunt for next frame */
            g_uart_rx_idx = 0;
        }
    }

    /* Continue receiving */
    HAL_UART_Receive_IT(huart, &g_uart_rx_buf[g_uart_rx_idx], 1);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        g_uart_error_count++;
        g_uart_rx_idx = 0;
        g_uart_response_ready = false;

        /* Recover: restart reception */
        HAL_UART_AbortReceive_IT(huart);
        HAL_UART_Receive_IT(huart, g_uart_rx_buf, 1);
    }
}

/* =========================================================================
 * USART1 Interrupt Handler
 * ========================================================================= */

void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(g_huart1);
}
