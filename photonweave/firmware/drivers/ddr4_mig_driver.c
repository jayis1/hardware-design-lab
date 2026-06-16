/**
 * ddr4_mig_driver.c — DDR4 MIG Memory Controller Driver Implementation
 *
 * Full driver for Xilinx 7-Series MIG DDR4 controller IP.
 * Handles initialization, calibration, ECC monitoring, performance
 * counters, and AXI4 read/write transactions for all 4 interfaces.
 *
 * This driver runs on the STM32H743 and communicates with the FPGA
 * MIG controllers via SPI-mapped register access.
 */

#include "ddr4_mig_driver.h"
#include "registers.h"
#include "board.h"
#include <string.h>

//=============================================================================
// Internal State
//=============================================================================

// MIG base addresses for each interface
static const uint32_t mig_base_addrs[DDR4_NUM_INTERFACES] = {
    MIG0_BASE_ADDR, MIG1_BASE_ADDR, MIG2_BASE_ADDR, MIG3_BASE_ADDR
};

// Calibration stage names for diagnostics
static const char *cal_stage_names[] = {
    "Write Leveling",
    "Read DQS Gate Training",
    "Read Data Eye Training",
    "Write Data Eye Training",
    "Complete"
};

// Per-interface initialization state tracking
static bool g_mig_initialized[DDR4_NUM_INTERFACES] = {false};
static bool g_mig_calibrated[DDR4_NUM_INTERFACES] = {false};
static bool g_mig_ecc_enabled[DDR4_NUM_INTERFACES] = {false};

//=============================================================================
// SPI Register Access Helpers
// These use the SPI bus to read/write FPGA MIG registers indirectly
//=============================================================================

/**
 * Read a MIG register via SPI command.
 * SPI protocol: [CMD=REG_READ] [ADDR_HI] [ADDR_LO] [PAD] → [STATUS] [DATA2] [DATA1] [DATA0]
 */
static uint32_t mig_reg_read_spi(uint8_t iface, uint32_t offset)
{
    uint32_t full_addr = mig_base_addrs[iface] + offset;
    uint8_t tx[4] = {
        SPI_CMD_REG_READ,
        (uint8_t)((full_addr >> 16) & 0xFF),
        (uint8_t)((full_addr >> 8) & 0xFF),
        (uint8_t)(full_addr & 0xFF)
    };
    uint8_t rx[4] = {0};

    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(&hspi1, tx, rx, 4, 50);
    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_SET);

    return ((uint32_t)rx[1] << 16) | ((uint32_t)rx[2] << 8) | rx[3];
}

/**
 * Write a MIG register via SPI command.
 * SPI protocol: [CMD=REG_WRITE] [ADDR_HI] [ADDR_LO] [DATA_BYTE] → [STATUS] [ECHO]
 */
static void mig_reg_write_spi(uint8_t iface, uint32_t offset, uint32_t value)
{
    uint32_t full_addr = mig_base_addrs[iface] + offset;
    uint8_t tx[4] = {
        SPI_CMD_REG_WRITE,
        (uint8_t)((full_addr >> 16) & 0xFF),
        (uint8_t)((full_addr >> 8) & 0xFF),
        (uint8_t)(value & 0xFF)
    };
    uint8_t rx[4] = {0};

    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(&hspi1, tx, rx, 4, 50);
    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_SET);

    // For multi-byte writes, send additional data bytes
    if (value > 0xFF) {
        tx[0] = SPI_CMD_REG_WRITE;
        tx[1] = (uint8_t)((full_addr >> 16) & 0xFF);
        tx[2] = (uint8_t)(((full_addr + 1) >> 8) & 0xFF);
        tx[3] = (uint8_t)((value >> 8) & 0xFF);
        HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_RESET);
        HAL_SPI_TransmitReceive(&hspi1, tx, rx, 4, 50);
        HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_SET);
    }
    if (value > 0xFFFF) {
        tx[0] = SPI_CMD_REG_WRITE;
        tx[1] = (uint8_t)((full_addr >> 16) & 0xFF);
        tx[2] = (uint8_t)(((full_addr + 2) >> 8) & 0xFF);
        tx[3] = (uint8_t)((value >> 16) & 0xFF);
        HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_RESET);
        HAL_SPI_TransmitReceive(&hspi1, tx, rx, 4, 50);
        HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_SET);
    }
    if (value > 0xFFFFFF) {
        tx[0] = SPI_CMD_REG_WRITE;
        tx[1] = (uint8_t)((full_addr >> 16) & 0xFF);
        tx[2] = (uint8_t)(((full_addr + 3) >> 8) & 0xFF);
        tx[3] = (uint8_t)((value >> 24) & 0xFF);
        HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_RESET);
        HAL_SPI_TransmitReceive(&hspi1, tx, rx, 4, 50);
        HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_SET);
    }
}

//=============================================================================
// Public API Implementation
//=============================================================================

int ddr4_mig_init_all(void)
{
    int result;
    uint32_t status;

    debug_uart_printf("DDR4 MIG: Initializing all 4 interfaces...\r\n");

    for (uint8_t iface = 0; iface < DDR4_NUM_INTERFACES; iface++) {
        result = ddr4_mig_init_interface(iface);
        if (result != 0) {
            debug_uart_printf("DDR4 MIG: Interface %d init failed (code %d)\r\n",
                              iface, result);
            return result;
        }
        debug_uart_printf("DDR4 MIG: Interface %d calibrated successfully\r\n", iface);
    }

    // Enable ECC on all interfaces
    for (uint8_t iface = 0; iface < DDR4_NUM_INTERFACES; iface++) {
        uint32_t ctrl = mig_reg_read_spi(iface, MIG_REG_CTRL);
        ctrl |= MIG_CTRL_ECC_ENABLE;
        mig_reg_write_spi(iface, MIG_REG_CTRL, ctrl);
        g_mig_ecc_enabled[iface] = true;
    }

    debug_uart_printf("DDR4 MIG: All interfaces ready, ECC enabled\r\n");
    return 0;
}

int ddr4_mig_init_interface(uint8_t iface)
{
    if (iface >= DDR4_NUM_INTERFACES) return -1;

    uint32_t ctrl, status;

    // Check if already initialized
    if (g_mig_initialized[iface] && g_mig_calibrated[iface]) {
        return 0;
    }

    // 1. Assert soft reset
    ctrl = mig_reg_read_spi(iface, MIG_REG_CTRL);
    ctrl |= MIG_CTRL_SOFT_RESET;
    mig_reg_write_spi(iface, MIG_REG_CTRL, ctrl);

    // Wait 50 µs for reset propagation
    board_delay_us(50);

    // 2. Release reset
    ctrl &= ~MIG_CTRL_SOFT_RESET;
    mig_reg_write_spi(iface, MIG_REG_CTRL, ctrl);

    // 3. Wait for PLL lock (up to 100 ms)
    bool pll_locked = false;
    for (int retry = 0; retry < 1000; retry++) {
        status = mig_reg_read_spi(iface, MIG_REG_STATUS);
        if (status & MIG_STATUS_PLL_LOCKED) {
            pll_locked = true;
            break;
        }
        board_delay_us(100);
    }

    if (!pll_locked) {
        debug_uart_printf("DDR4 MIG%d: PLL failed to lock\r\n", iface);
        return -2;
    }

    // 4. Enable performance counters
    ctrl = mig_reg_read_spi(iface, MIG_REG_CTRL);
    ctrl |= MIG_CTRL_PERF_COUNT_EN;
    mig_reg_write_spi(iface, MIG_REG_CTRL, ctrl);

    // 5. Start calibration
    ctrl |= MIG_CTRL_START_CAL;
    mig_reg_write_spi(iface, MIG_REG_CTRL, ctrl);

    // 6. Wait for calibration (up to 200 ms)
    int cal_result = ddr4_mig_wait_calibration(iface, 200);
    if (cal_result != 0) {
        debug_uart_printf("DDR4 MIG%d: Calibration failed (code %d)\r\n",
                          iface, cal_result);
        return cal_result;
    }

    g_mig_initialized[iface] = true;
    g_mig_calibrated[iface] = true;

    return 0;
}

bool ddr4_mig_is_calibrated(uint8_t iface)
{
    if (iface >= DDR4_NUM_INTERFACES) return false;
    if (g_mig_calibrated[iface]) return true;

    uint32_t status = mig_reg_read_spi(iface, MIG_REG_STATUS);
    return (status & MIG_STATUS_CAL_DONE) != 0;
}

int ddr4_mig_wait_calibration(uint8_t iface, uint32_t timeout_ms)
{
    if (iface >= DDR4_NUM_INTERFACES) return -1;

    uint32_t cal_status;
    uint32_t elapsed_us = 0;
    uint32_t last_stage = 0xFF;

    while (elapsed_us < (timeout_ms * 1000)) {
        cal_status = mig_reg_read_spi(iface, MIG_REG_CAL_STATUS);

        // Extract current calibration stage
        uint32_t current_stage = (cal_status >> 8) & 0x07;

        // Log stage transitions
        if (current_stage != last_stage && current_stage < 5) {
            debug_uart_printf("DDR4 MIG%d: Cal stage %d — %s\r\n",
                              iface, current_stage,
                              cal_stage_names[current_stage]);
            last_stage = current_stage;
        }

        // Check for completion (bit 7 = all stages done)
        if (cal_status & 0x80) {
            // Verify CAL_DONE in main status
            uint32_t status = mig_reg_read_spi(iface, MIG_REG_STATUS);
            if (status & MIG_STATUS_CAL_DONE) {
                return 0;
            }
        }

        // Check for calibration failure
        if (cal_status & 0x40) {
            uint32_t error_stage = (cal_status >> 8) & 0x07;
            uint32_t error_byte = (cal_status >> 16) & 0x0F;
            uint32_t error_bit = (cal_status >> 20) & 0x3F;
            debug_uart_printf("DDR4 MIG%d: CAL ERROR stage=%d byte=%d bit=%d\r\n",
                              iface, error_stage, error_byte, error_bit);
            return -100 - (int)(error_stage * 16) - (int)error_byte;
        }

        board_delay_us(100);
        elapsed_us += 100;
    }

    debug_uart_printf("DDR4 MIG%d: Calibration timeout after %lu ms\r\n",
                      iface, timeout_ms);
    return -3;
}

void ddr4_mig_get_status(uint8_t iface, uint32_t *status)
{
    if (iface >= DDR4_NUM_INTERFACES || status == NULL) return;
    *status = mig_reg_read_spi(iface, MIG_REG_STATUS);
}

int ddr4_mig_check_ecc(uint8_t iface, uint32_t *error_count,
                        uint64_t *error_addr, bool *uncorrectable)
{
    if (iface >= DDR4_NUM_INTERFACES) return -1;

    uint32_t ecc_status = mig_reg_read_spi(iface, MIG_REG_ECC_STATUS);

    if (error_count) {
        *error_count = mig_reg_read_spi(iface, MIG_REG_ECC_ERR_COUNT);
    }

    if (error_addr) {
        uint32_t addr_lo = mig_reg_read_spi(iface, MIG_REG_ECC_ERR_ADDR);
        uint32_t addr_hi = mig_reg_read_spi(iface, MIG_REG_ECC_ERR_ADDR + 4);
        *error_addr = ((uint64_t)addr_hi << 32) | addr_lo;
    }

    if (uncorrectable) {
        *uncorrectable = (ecc_status & MIG_STATUS_ECC_UNCORR) != 0;
    }

    // Clear ECC status flags by writing back
    mig_reg_write_spi(iface, MIG_REG_ECC_STATUS, ecc_status);

    return (ecc_status & MIG_STATUS_ECC_ERROR) ? 1 : 0;
}

void ddr4_mig_get_performance(uint8_t iface, uint32_t *rd_bw,
                               uint32_t *wr_bw, uint32_t *avg_latency)
{
    if (iface >= DDR4_NUM_INTERFACES) return;

    if (rd_bw) {
        *rd_bw = mig_reg_read_spi(iface, MIG_REG_PERF_BANDWIDTH);
    }

    if (wr_bw) {
        // Approximate write bandwidth from byte counter
        // Requires time reference for accurate calculation
        uint32_t wr_bytes = mig_reg_read_spi(iface, MIG_REG_PERF_WR_BYTES);
        uint32_t wr_count = mig_reg_read_spi(iface, MIG_REG_PERF_WR_COUNT);
        // Rough estimate: bytes per transaction × transaction rate
        *wr_bw = (wr_count > 0) ? (wr_bytes / wr_count) * 100 : 0;
    }

    if (avg_latency) {
        *avg_latency = mig_reg_read_spi(iface, MIG_REG_PERF_LATENCY);
    }
}

void ddr4_mig_get_temperature(uint8_t iface, int32_t *temp_millic)
{
    if (iface >= DDR4_NUM_INTERFACES || temp_millic == NULL) return;

    uint32_t temp_raw = mig_reg_read_spi(iface, MIG_REG_TEMP);

    // MR4 temperature format: 7-bit signed value, 0.5°C per LSB
    // Sign-extend the 7-bit value
    int32_t temp_c_x2 = (int32_t)((temp_raw & 0x7F) << 25) >> 25;
    *temp_millic = temp_c_x2 * 500;
}

int ddr4_mig_axi_write(uint8_t iface, uint64_t addr, const void *data,
                        uint32_t len_bytes)
{
    if (iface >= DDR4_NUM_INTERFACES || data == NULL) return -1;
    if (len_bytes == 0 || len_bytes > 64) return -2;
    if (addr & 0x3) return -3;

    // AXI4 write via SPI: send address + data in command sequence
    // Command format: [CMD=AXI_WRITE] [LEN] [ADDR_HI] [ADDR_LO]
    // Followed by data bytes in subsequent SPI transactions

    uint8_t cmd[4] = {
        0x10, // AXI_WRITE command
        (uint8_t)(len_bytes & 0xFF),
        (uint8_t)((addr >> 24) & 0xFF),
        (uint8_t)((addr >> 16) & 0xFF)
    };
    uint8_t rx[4];

    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(&hspi1, cmd, rx, 4, 50);
    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_SET);

    // Send address low bytes
    cmd[0] = (uint8_t)((addr >> 8) & 0xFF);
    cmd[1] = (uint8_t)(addr & 0xFF);
    cmd[2] = 0;
    cmd[3] = 0;

    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(&hspi1, cmd, rx, 4, 50);
    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_SET);

    // Send data in 4-byte chunks
    const uint8_t *src = (const uint8_t *)data;
    for (uint32_t i = 0; i < len_bytes; i += 4) {
        uint8_t remaining = (len_bytes - i) < 4 ? (len_bytes - i) : 4;
        uint8_t data_cmd[4] = {0};
        for (uint8_t j = 0; j < remaining; j++) {
            data_cmd[j] = src[i + j];
        }

        HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_RESET);
        HAL_SPI_TransmitReceive(&hspi1, data_cmd, rx, 4, 50);
        HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_SET);
    }

    // Read response
    uint8_t resp_cmd[4] = {0xFF, 0, 0, 0};
    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(&hspi1, resp_cmd, rx, 4, 50);
    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_SET);

    return (rx[0] == SPI_RESP_OK) ? 0 : -4;
}

int ddr4_mig_axi_read(uint8_t iface, uint64_t addr, void *data,
                       uint32_t len_bytes)
{
    if (iface >= DDR4_NUM_INTERFACES || data == NULL) return -1;
    if (len_bytes == 0 || len_bytes > 64) return -2;
    if (addr & 0x3) return -3;

    // AXI4 read via SPI
    uint8_t cmd[4] = {
        0x11, // AXI_READ command
        (uint8_t)(len_bytes & 0xFF),
        (uint8_t)((addr >> 24) & 0xFF),
        (uint8_t)((addr >> 16) & 0xFF)
    };
    uint8_t rx[4];

    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(&hspi1, cmd, rx, 4, 50);
    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_SET);

    // Send address low
    cmd[0] = (uint8_t)((addr >> 8) & 0xFF);
    cmd[1] = (uint8_t)(addr & 0xFF);
    cmd[2] = 0;
    cmd[3] = 0;

    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(&hspi1, cmd, rx, 4, 50);
    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_SET);

    // Read data in 4-byte chunks
    uint8_t *dst = (uint8_t *)data;
    for (uint32_t i = 0; i < len_bytes; i += 4) {
        uint8_t read_cmd[4] = {0xFF, 0xFF, 0xFF, 0xFF};

        HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_RESET);
        HAL_SPI_TransmitReceive(&hspi1, read_cmd, rx, 4, 50);
        HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_SET);

        uint8_t remaining = (len_bytes - i) < 4 ? (len_bytes - i) : 4;
        for (uint8_t j = 0; j < remaining; j++) {
            dst[i + j] = rx[j];
        }
    }

    return 0;
}

int ddr4_mig_axi_burst_write(uint8_t iface, uint64_t addr,
                              const void *data, uint32_t burst_len)
{
    if (iface >= DDR4_NUM_INTERFACES || data == NULL) return -1;
    if (burst_len == 0 || burst_len > 256) return -2;
    if (addr & 0x3F) return -3;

    // Burst write: send burst command followed by all data beats
    uint8_t cmd[4] = {
        0x12, // AXI_BURST_WRITE command
        (uint8_t)(burst_len & 0xFF),
        (uint8_t)((addr >> 24) & 0xFF),
        (uint8_t)((addr >> 16) & 0xFF)
    };
    uint8_t rx[4];

    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(&hspi1, cmd, rx, 4, 50);
    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_SET);

    // Send address low
    cmd[0] = (uint8_t)((addr >> 8) & 0xFF);
    cmd[1] = (uint8_t)(addr & 0xFF);
    cmd[2] = 0;
    cmd[3] = 0;

    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(&hspi1, cmd, rx, 4, 50);
    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_SET);

    // Send all data beats (each beat = 64 bytes = 16 SPI transactions)
    const uint8_t *src = (const uint8_t *)data;
    for (uint32_t beat = 0; beat < burst_len; beat++) {
        for (uint32_t w = 0; w < 16; w++) { // 16 × 4-byte words per beat
            uint8_t data_cmd[4];
            for (int b = 0; b < 4; b++) {
                data_cmd[b] = src[beat * 64 + w * 4 + b];
            }

            HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_RESET);
            HAL_SPI_TransmitReceive(&hspi1, data_cmd, rx, 4, 50);
            HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_SET);
        }
    }

    // Read final response
    uint8_t resp_cmd[4] = {0xFF, 0, 0, 0};
    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(&hspi1, resp_cmd, rx, 4, 50);
    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_SET);

    return (rx[0] == SPI_RESP_OK) ? 0 : -4;
}

int ddr4_mig_axi_burst_read(uint8_t iface, uint64_t addr,
                             void *data, uint32_t burst_len)
{
    if (iface >= DDR4_NUM_INTERFACES || data == NULL) return -1;
    if (burst_len == 0 || burst_len > 256) return -2;
    if (addr & 0x3F) return -3;

    // Burst read command
    uint8_t cmd[4] = {
        0x13, // AXI_BURST_READ command
        (uint8_t)(burst_len & 0xFF),
        (uint8_t)((addr >> 24) & 0xFF),
        (uint8_t)((addr >> 16) & 0xFF)
    };
    uint8_t rx[4];

    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(&hspi1, cmd, rx, 4, 50);
    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_SET);

    // Send address low
    cmd[0] = (uint8_t)((addr >> 8) & 0xFF);
    cmd[1] = (uint8_t)(addr & 0xFF);
    cmd[2] = 0;
    cmd[3] = 0;

    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(&hspi1, cmd, rx, 4, 50);
    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_SET);

    // Read all data beats
    uint8_t *dst = (uint8_t *)data;
    for (uint32_t beat = 0; beat < burst_len; beat++) {
        for (uint32_t w = 0; w < 16; w++) {
            uint8_t read_cmd[4] = {0xFF, 0xFF, 0xFF, 0xFF};

            HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_RESET);
            HAL_SPI_TransmitReceive(&hspi1, read_cmd, rx, 4, 50);
            HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_SET);

            for (int b = 0; b < 4; b++) {
                dst[beat * 64 + w * 4 + b] = rx[b];
            }
        }
    }

    return 0;
}

void ddr4_mig_soft_reset(uint8_t iface)
{
    if (iface >= DDR4_NUM_INTERFACES) return;

    uint32_t ctrl = mig_reg_read_spi(iface, MIG_REG_CTRL);
    ctrl |= MIG_CTRL_SOFT_RESET;
    mig_reg_write_spi(iface, MIG_REG_CTRL, ctrl);

    board_delay_us(50);

    ctrl &= ~MIG_CTRL_SOFT_RESET;
    mig_reg_write_spi(iface, MIG_REG_CTRL, ctrl);

    g_mig_initialized[iface] = false;
    g_mig_calibrated[iface] = false;
    g_mig_ecc_enabled[iface] = false;
}
