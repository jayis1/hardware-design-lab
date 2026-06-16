/**
 * ddr4_mig_driver.h — DDR4 MIG Memory Controller Driver Header
 *
 * Driver for the Xilinx 7-Series MIG DDR4 memory controller IP.
 * Manages 4 independent 16-bit DDR4-2400 interfaces with ECC.
 * Provides AXI4 read/write access for the CGH pipeline and DMA engine.
 */

#ifndef DDR4_MIG_DRIVER_H
#define DDR4_MIG_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

//=============================================================================
// DDR4 Physical Configuration
//=============================================================================
#define DDR4_NUM_INTERFACES         4
#define DDR4_DATA_WIDTH             16          // Per interface
#define DDR4_TOTAL_DATA_WIDTH       64          // Combined across 4 interfaces
#define DDR4_CLK_FREQ_MHZ           300         // Reference clock
#define DDR4_DATA_RATE_MBPS         2400        // DDR4-2400
#define DDR4_CAPACITY_MB            1024        // Per chip (8 Gb)
#define DDR4_TOTAL_CAPACITY_MB      4096        // 4 GB total
#define DDR4_BURST_LENGTH           8           // BL8 (fixed)

//=============================================================================
// DDR4 Timing Parameters (MT40A512M16JY-075E, DDR4-2400, tCK=0.833ns)
//=============================================================================
#define DDR4_tCK_PS                 833
#define DDR4_tRCD_NS                14          // RAS-to-CAS delay
#define DDR4_tRP_NS                 14          // Row precharge
#define DDR4_tRAS_NS                32          // Row active time
#define DDR4_tRC_NS                 46          // Row cycle time
#define DDR4_tRRD_NS                5           // Row-to-row delay
#define DDR4_tFAW_NS                21          // Four-activate window
#define DDR4_tWTR_NS                4           // Write-to-read
#define DDR4_tRTP_NS                7           // Read-to-precharge
#define DDR4_tWR_NS                 15          // Write recovery
#define DDR4_tCL_CK                 15          // CAS latency
#define DDR4_tCWL_CK                11          // CAS write latency
#define DDR4_tAL_CK                 0           // Additive latency
#define DDR4_tCCD_CK                4           // CAS-to-CAS delay

//=============================================================================
// MIG Register Map (AXI4-Lite slave in FPGA fabric)
//=============================================================================
#define MIG0_BASE_ADDR              0x44000000
#define MIG1_BASE_ADDR              0x44010000
#define MIG2_BASE_ADDR              0x44020000
#define MIG3_BASE_ADDR              0x44030000

// MIG Control/Status Register Offsets
#define MIG_REG_CTRL                0x00
#define MIG_REG_STATUS              0x04
#define MIG_REG_CAL_STATUS          0x08
#define MIG_REG_ECC_STATUS          0x0C
#define MIG_REG_ECC_ERR_ADDR        0x10
#define MIG_REG_ECC_ERR_COUNT       0x14
#define MIG_REG_TEMP                0x18
#define MIG_REG_REFRESH_COUNT       0x1C
#define MIG_REG_PERF_RD_COUNT       0x20
#define MIG_REG_PERF_WR_COUNT       0x24
#define MIG_REG_PERF_RD_BYTES       0x28
#define MIG_REG_PERF_WR_BYTES       0x2C
#define MIG_REG_PERF_LATENCY        0x30
#define MIG_REG_PERF_BANDWIDTH      0x34

// MIG Status Register Bits
#define MIG_STATUS_CAL_DONE         0x01
#define MIG_STATUS_PLL_LOCKED       0x02
#define MIG_STATUS_IDLE             0x04
#define MIG_STATUS_ECC_ENABLED      0x08
#define MIG_STATUS_ECC_ERROR        0x10
#define MIG_STATUS_ECC_UNCORR       0x20
#define MIG_STATUS_TEMP_ALERT       0x40
#define MIG_STATUS_REFRESH_OVF      0x80

// MIG Control Register Bits
#define MIG_CTRL_SOFT_RESET         0x01
#define MIG_CTRL_START_CAL          0x02
#define MIG_CTRL_ECC_ENABLE         0x08
#define MIG_CTRL_PERF_COUNT_EN      0x10

//=============================================================================
// DDR4 Memory Partition Map
//=============================================================================
#define DDR4_PART_FRAME_A           0x00000000ULL
#define DDR4_PART_FRAME_B           0x10000000ULL
#define DDR4_PART_WORKING           0x20000000ULL
#define DDR4_PART_FFT_SCRATCH       0x40000000ULL
#define DDR4_PART_HOLOGRAM_0        0x80000000ULL
#define DDR4_PART_HOLOGRAM_1        0x90000000ULL
#define DDR4_PART_OUTPUT            0xA0000000ULL
#define DDR4_PART_DESC              0xA4000000ULL
#define DDR4_PART_RESERVED          0xA5000000ULL

//=============================================================================
// Function Prototypes
//=============================================================================

/**
 * Initialize all 4 DDR4 interfaces.
 * Performs soft reset, PLL lock wait, and calibration for each interface.
 * Enables ECC on all interfaces after calibration.
 * @return 0 on success, negative error code on failure
 */
int ddr4_mig_init_all(void);

/**
 * Initialize a single DDR4 interface.
 * @param iface Interface index (0-3)
 * @return 0 on success, negative error code on failure
 */
int ddr4_mig_init_interface(uint8_t iface);

/**
 * Check if a DDR4 interface has completed calibration.
 * @param iface Interface index (0-3)
 * @return true if calibrated
 */
bool ddr4_mig_is_calibrated(uint8_t iface);

/**
 * Wait for calibration to complete with timeout.
 * @param iface Interface index (0-3)
 * @param timeout_ms Maximum wait time in milliseconds
 * @return 0 on success, negative on timeout or error
 */
int ddr4_mig_wait_calibration(uint8_t iface, uint32_t timeout_ms);

/**
 * Get the status register value for an interface.
 * @param iface Interface index (0-3)
 * @param status Pointer to receive status word
 */
void ddr4_mig_get_status(uint8_t iface, uint32_t *status);

/**
 * Check ECC status and retrieve error information.
 * @param iface Interface index (0-3)
 * @param error_count Pointer to receive cumulative error count
 * @param error_addr Pointer to receive last error address
 * @param uncorrectable Pointer to receive uncorrectable flag
 * @return 1 if error detected, 0 if clean, negative on parameter error
 */
int ddr4_mig_check_ecc(uint8_t iface, uint32_t *error_count,
                        uint64_t *error_addr, bool *uncorrectable);

/**
 * Get performance metrics for an interface.
 * @param iface Interface index (0-3)
 * @param rd_bw Pointer to receive read bandwidth (MB/s × 100)
 * @param wr_bw Pointer to receive write bandwidth (MB/s × 100)
 * @param avg_latency Pointer to receive average latency (ns)
 */
void ddr4_mig_get_performance(uint8_t iface, uint32_t *rd_bw,
                               uint32_t *wr_bw, uint32_t *avg_latency);

/**
 * Get DRAM temperature from mode register MR4.
 * @param iface Interface index (0-3)
 * @param temp_millic Pointer to receive temperature in millidegrees C
 */
void ddr4_mig_get_temperature(uint8_t iface, int32_t *temp_millic);

/**
 * AXI4 single-beat write (up to 64 bytes).
 * @param iface Interface index (0-3)
 * @param addr 64-bit physical address in DDR4 space
 * @param data Pointer to data buffer
 * @param len_bytes Transfer length (1-64 bytes, must be 4-byte aligned)
 * @return 0 on success, negative on error
 */
int ddr4_mig_axi_write(uint8_t iface, uint64_t addr, const void *data,
                        uint32_t len_bytes);

/**
 * AXI4 single-beat read (up to 64 bytes).
 * @param iface Interface index (0-3)
 * @param addr 64-bit physical address in DDR4 space
 * @param data Pointer to data buffer
 * @param len_bytes Transfer length (1-64 bytes, must be 4-byte aligned)
 * @return 0 on success, negative on error
 */
int ddr4_mig_axi_read(uint8_t iface, uint64_t addr, void *data,
                       uint32_t len_bytes);

/**
 * AXI4 burst write (up to 256 beats × 64 bytes = 16 KB).
 * @param iface Interface index (0-3)
 * @param addr 64-bit physical address (must be 64-byte aligned)
 * @param data Pointer to data buffer
 * @param burst_len Number of beats (1-256)
 * @return 0 on success, negative on error
 */
int ddr4_mig_axi_burst_write(uint8_t iface, uint64_t addr,
                              const void *data, uint32_t burst_len);

/**
 * AXI4 burst read (up to 256 beats × 64 bytes = 16 KB).
 * @param iface Interface index (0-3)
 * @param addr 64-bit physical address (must be 64-byte aligned)
 * @param data Pointer to data buffer
 * @param burst_len Number of beats (1-256)
 * @return 0 on success, negative on error
 */
int ddr4_mig_axi_burst_read(uint8_t iface, uint64_t addr,
                             void *data, uint32_t burst_len);

/**
 * Soft reset a MIG interface (re-run calibration required after).
 * @param iface Interface index (0-3)
 */
void ddr4_mig_soft_reset(uint8_t iface);

#endif // DDR4_MIG_DRIVER_H
