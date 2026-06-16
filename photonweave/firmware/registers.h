/**
 * registers.h — PhotonWeave FPGA MMIO Register Map
 *
 * Complete register definitions for the PhotonWeave CGH Engine FPGA.
 * All registers are 32-bit, little-endian, accessed via PCIe BAR0.
 * This header is shared between STM32 firmware and Linux driver.
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

//=============================================================================
// BAR0 Base Address (assigned by PCIe enumeration at runtime)
//=============================================================================
#ifndef BAR0_BASE
#define BAR0_BASE  0x00000000  // Placeholder — set at runtime
#endif

//=============================================================================
// Register Map Offsets
//=============================================================================

// Identification & Version
#define PW_REG_MAGIC            0x0000  // RO: Magic 0x50484F54 ("PHOT")
#define PW_REG_VERSION          0x0004  // RO: [31:16]=major, [15:0]=minor
#define PW_REG_STATUS           0x0008  // RO: System status
#define PW_REG_CONTROL          0x000C  // RW: System control
#define PW_REG_INTERRUPT_MASK   0x0010  // RW: Interrupt mask
#define PW_REG_INTERRUPT_STATUS 0x0014  // RW1C: Interrupt status

// DMA Engine
#define PW_REG_DMA_CTRL         0x0018  // RW: DMA control
#define PW_REG_DMA_STATUS       0x001C  // RO: DMA status
#define PW_REG_DMA_SRC_ADDR_LO  0x0020  // RW: Source address low
#define PW_REG_DMA_SRC_ADDR_HI  0x0024  // RW: Source address high
#define PW_REG_DMA_DST_ADDR_LO  0x0028  // RW: Destination address low
#define PW_REG_DMA_DST_ADDR_HI  0x002C  // RW: Destination address high
#define PW_REG_DMA_LENGTH       0x0030  // RW: Transfer length (bytes)
#define PW_REG_DMA_DESC_ADDR_LO 0x0034  // RW: Descriptor ring base low
#define PW_REG_DMA_DESC_ADDR_HI 0x0038  // RW: Descriptor ring base high
#define PW_REG_DMA_DESC_COUNT   0x003C  // RW: Descriptor count
#define PW_REG_DMA_BYTES_DONE   0x0040  // RO: Bytes transferred
#define PW_REG_DMA_DESC_DONE    0x0044  // RO: Descriptors processed
#define PW_REG_DMA_CRC_ERRORS   0x0048  // RO: CRC error count

// CGH Pipeline
#define PW_REG_CGH_CTRL         0x004C  // RW: Pipeline control
#define PW_REG_CGH_STATUS       0x0050  // RO: Pipeline status
#define PW_REG_CGH_PARAMS       0x0054  // RW: Active parameter set
#define PW_REG_CGH_WAVELENGTH   0x0058  // RW: Wavelength (nm × 1000)
#define PW_REG_CGH_DEPTH_MIN    0x005C  // RW: Min depth (µm)
#define PW_REG_CGH_DEPTH_MAX    0x0060  // RW: Max depth (µm)
#define PW_REG_CGH_DEPTH_PLANES 0x0064  // RW: Number of planes
#define PW_REG_CGH_INPUT_W      0x0068  // RW: Input width
#define PW_REG_CGH_INPUT_H      0x006C  // RW: Input height
#define PW_REG_CGH_OUTPUT_W     0x0070  // RW: Output width
#define PW_REG_CGH_OUTPUT_H     0x0074  // RW: Output height
#define PW_REG_CGH_FRAME_COUNT  0x0078  // RO: Frames processed
#define PW_REG_CGH_FRAME_TIME   0x007C  // RO: Last frame time (µs)
#define PW_REG_CGH_FPS          0x0080  // RO: Current FPS × 1000
#define PW_REG_CGH_ERROR_FLAGS  0x0084  // RO: Pipeline errors
#define PW_REG_CGH_FFT_SCRATCH  0x0088  // RW: FFT scratch buffer offset

// HDMI Output
#define PW_REG_HDMI_CTRL        0x008C  // RW: HDMI control
#define PW_REG_HDMI_STATUS      0x0090  // RO: HDMI status
#define PW_REG_HDMI_FRAME_ADDR_LO 0x0094  // RW: Frame buffer address low
#define PW_REG_HDMI_FRAME_ADDR_HI 0x0098  // RW: Frame buffer address high
#define PW_REG_HDMI_TIMING_H    0x009C  // RW: Horizontal timing
#define PW_REG_HDMI_TIMING_V    0x00A0  // RW: Vertical timing
#define PW_REG_HDMI_FRAME_COUNT 0x00A4  // RO: HDMI frames output
#define PW_REG_HDMI_UNDERRUN    0x00A8  // RO: Buffer underrun count
#define PW_REG_HDMI_CURRENT_LINE 0x00AC // RO: Current scan line

// QSFP+ Interface
#define PW_REG_QSFP_CTRL        0x00B0  // RW: QSFP+ control
#define PW_REG_QSFP_STATUS      0x00B4  // RO: QSFP+ status
#define PW_REG_QSFP_RX_FRAME_LO 0x00B8  // RW: RX buffer address low
#define PW_REG_QSFP_RX_FRAME_HI 0x00BC  // RW: RX buffer address high
#define PW_REG_QSFP_RX_COUNT    0x00C0  // RO: RX frame count
#define PW_REG_QSFP_RX_BYTES_LO 0x00C4  // RO: RX bytes low
#define PW_REG_QSFP_RX_BYTES_HI 0x00C8  // RO: RX bytes high
#define PW_REG_QSFP_RX_ERRORS   0x00CC  // RO: RX error count
#define PW_REG_QSFP_LINK_SPEED  0x00D0  // RO: Link speed (Mbps)

// Temperature Monitoring
#define PW_REG_TEMP_FPGA_1      0x00D4  // RO: FPGA temp 1 (millideg C)
#define PW_REG_TEMP_FPGA_2      0x00D8  // RO: FPGA temp 2 (millideg C)
#define PW_REG_TEMP_DDR4        0x00DC  // RO: DDR4 temp (millideg C)
#define PW_REG_TEMP_PMIC        0x00E0  // RO: PMIC temp (millideg C)

// Power Monitoring
#define PW_REG_POWER_VCCINT     0x00E4  // RO: VCCINT (mV)
#define PW_REG_POWER_VCCAUX     0x00E8  // RO: VCCAUX (mV)
#define PW_REG_POWER_VDD_DDR    0x00EC  // RO: VDD_DDR (mV)
#define PW_REG_POWER_MGTAVCC    0x00F0  // RO: MGTAVCC (mV)
#define PW_REG_POWER_MGTAVTT    0x00F4  // RO: MGTAVTT (mV)
#define PW_REG_POWER_CURRENT    0x00F8  // RO: Total current (mA)
#define PW_REG_POWER_CONSUMED   0x00FC  // RO: Energy consumed (mWh)

// Clock Monitor
#define PW_REG_CLOCK_STATUS     0x0100  // RO: Clock status flags

// Debug & Scratch
#define PW_REG_DEBUG_UART       0x0104  // RW: Debug UART data
#define PW_REG_SCRATCH_0        0x0108  // RW: Scratch register 0
#define PW_REG_SCRATCH_1        0x010C  // RW: Scratch register 1
#define PW_REG_SCRATCH_2        0x0110  // RW: Scratch register 2
#define PW_REG_SCRATCH_3        0x0114  // RW: Scratch register 3

// Device Identity
#define PW_REG_FPGA_DIE_ID_LO   0x0118  // RO: Xilinx DNA low
#define PW_REG_FPGA_DIE_ID_HI   0x011C  // RO: Xilinx DNA high
#define PW_REG_EFUSE_USER       0x0120  // RO: User eFuse

// System Control
#define PW_REG_RESET_CONTROL    0x0124  // RW: Soft reset control
#define PW_REG_CLOCK_THROTTLE   0x0128  // RW: Clock scaling
#define PW_REG_WATCHDOG_KICK    0x012C  // WO: Watchdog kick
#define PW_REG_WATCHDOG_TIMEOUT 0x0130  // RW: Watchdog timeout (ms)
#define PW_REG_WATCHDOG_REMAIN  0x0134  // RO: Remaining time (ms)

// CGH Parameter Sets (16 sets × 16 registers = 256 registers)
#define PW_REG_CGH_PARAM_BASE   0x0200  // Base of parameter set array
#define PW_REG_CGH_PARAM_STRIDE 0x0040  // 64 bytes per parameter set

//=============================================================================
// Status Register (PW_REG_STATUS) Bit Definitions
//=============================================================================
#define PW_STATUS_FPGA_READY        (1 << 0)   // FPGA configured, PLLs locked
#define PW_STATUS_PCIE_LINK_UP      (1 << 1)   // PCIe in L0 state
#define PW_STATUS_DDR4_CAL_DONE     (1 << 2)   // All DDR4 calibrated
#define PW_STATUS_CGH_IDLE          (1 << 3)   // CGH pipeline idle
#define PW_STATUS_CGH_BUSY          (1 << 4)   // CGH pipeline processing
#define PW_STATUS_CGH_FRAME_DONE    (1 << 5)   // Frame complete
#define PW_STATUS_HDMI_ACTIVE      (1 << 6)   // HDMI streaming
#define PW_STATUS_QSFP_LINK_UP      (1 << 7)   // QSFP+ link up
#define PW_STATUS_THERMAL_WARNING   (1 << 8)   // Temp > 80°C
#define PW_STATUS_THERMAL_CRITICAL  (1 << 9)   // Temp > 95°C
#define PW_STATUS_CLOCK_LOL         (1 << 10)  // Si5345 loss of lock
#define PW_STATUS_DDR4_ECC_ERROR   (1 << 11)  // ECC error detected
#define PW_STATUS_DMA_ERROR         (1 << 12)  // DMA error
#define PW_STATUS_PCIE_ERROR        (1 << 13)  // PCIe link error
#define PW_STATUS_WATCHDOG_EXPIRED  (1 << 14)  // Watchdog expired
#define PW_STATUS_POWER_FAULT      (1 << 15)  // Power rail fault
#define PW_STATUS_FFT_FAULT_MASK   (0xFF << 16) // Per-engine fault flags

//=============================================================================
// Control Register (PW_REG_CONTROL) Bit Definitions
//=============================================================================
#define PW_CTRL_CGH_START           (1 << 0)   // Start CGH processing
#define PW_CTRL_CGH_ABORT           (1 << 1)   // Abort current frame
#define PW_CTRL_HDMI_ENABLE         (1 << 2)   // Enable HDMI output
#define PW_CTRL_QSFP_ENABLE         (1 << 3)   // Enable QSFP+ receiver
#define PW_CTRL_DMA_ENABLE          (1 << 4)   // Enable DMA engine
#define PW_CTRL_INTERRUPT_ENABLE    (1 << 5)   // Global interrupt enable
#define PW_CTRL_THERMAL_THROTTLE    (1 << 6)   // Auto thermal throttling
#define PW_CTRL_ECC_ENABLE          (1 << 7)   // DDR4 ECC enable
#define PW_CTRL_WATCHDOG_ENABLE     (1 << 8)   // Watchdog enable
#define PW_CTRL_SOFT_RESET_CGH      (1 << 9)   // Reset CGH pipeline
#define PW_CTRL_SOFT_RESET_DMA      (1 << 10)  // Reset DMA engine
#define PW_CTRL_SOFT_RESET_HDMI     (1 << 11)  // Reset HDMI output
#define PW_CTRL_CLOCK_SCALE_DOWN    (1 << 12)  // Low-power clock scaling
#define PW_CTRL_CGH_PARAM_SET_SHIFT 16
#define PW_CTRL_CGH_PARAM_SET_MASK  (0x0F << 16) // Parameter set select
#define PW_CTRL_FFT_ENGINE_SHIFT   20
#define PW_CTRL_FFT_ENGINE_MASK    (0xFF << 20) // Active FFT engines

//=============================================================================
// Interrupt Register Bit Definitions
//=============================================================================
#define PW_INT_FRAME_DONE           (1 << 0)   // CGH frame complete
#define PW_INT_ERROR                (1 << 1)   // Error condition
#define PW_INT_THERMAL              (1 << 2)   // Thermal alert
#define PW_INT_DMA_COMPLETE         (1 << 3)   // DMA transfer done
#define PW_INT_HDMI_UNDERRUN        (1 << 4)   // HDMI buffer underrun
#define PW_INT_QSFP_FRAME           (1 << 5)   // QSFP+ frame received
#define PW_INT_PCIE_LINK_CHANGE     (1 << 6)   // PCIe link state change
#define PW_INT_WATCHDOG             (1 << 7)   // Watchdog pre-timeout

//=============================================================================
// CGH Error Flags
//=============================================================================
#define CGH_ERR_DDR4_READ           0x0001
#define CGH_ERR_DDR4_WRITE          0x0002
#define CGH_ERR_FFT_OVERFLOW        0x0004
#define CGH_ERR_FFT_TIMEOUT         0x0008
#define CGH_ERR_PARAM_INVALID       0x0010
#define CGH_ERR_DMA_FAULT           0x0020
#define CGH_ERR_BUFFER_OVERRUN      0x0040
#define CGH_ERR_WATCHDOG            0x0080

//=============================================================================
// Clock Status Flags
//=============================================================================
#define CLK_STATUS_FFT_LOCKED       (1 << 0)
#define CLK_STATUS_PCIE_LOCKED      (1 << 1)
#define CLK_STATUS_DDR4_LOCKED      (1 << 2)
#define CLK_STATUS_HDMI_LOCKED      (1 << 3)
#define CLK_STATUS_AXI_LOCKED       (1 << 4)
#define CLK_STATUS_CONFIG_LOCKED    (1 << 5)
#define CLK_STATUS_FT601_LOCKED     (1 << 6)
#define CLK_STATUS_QSFP_LOCKED      (1 << 7)
#define CLK_STATUS_TMDS_LOCKED      (1 << 8)
#define CLK_STATUS_SI5345_LOL       (1 << 9)

//=============================================================================
// DDR4 Memory Partition Map (offsets within 4GB address space)
//=============================================================================
#define DDR4_PART_FRAME_A           0x00000000ULL  // 256 MB
#define DDR4_PART_FRAME_B           0x10000000ULL  // 256 MB
#define DDR4_PART_WORKING           0x20000000ULL  // 512 MB
#define DDR4_PART_FFT_SCRATCH       0x40000000ULL  // 1024 MB
#define DDR4_PART_HOLOGRAM_0        0x80000000ULL  // 256 MB
#define DDR4_PART_HOLOGRAM_1        0x90000000ULL  // 256 MB
#define DDR4_PART_OUTPUT            0xA0000000ULL  // 64 MB
#define DDR4_PART_DESC              0xA4000000ULL  // 16 MB
#define DDR4_PART_RESERVED          0xA5000000ULL  // 1.5 GB

#define DDR4_PART_FRAME_A_SIZE      0x10000000ULL  // 256 MB
#define DDR4_PART_FRAME_B_SIZE      0x10000000ULL
#define DDR4_PART_WORKING_SIZE      0x20000000ULL
#define DDR4_PART_FFT_SCRATCH_SIZE  0x40000000ULL
#define DDR4_PART_HOLOGRAM_SIZE     0x10000000ULL
#define DDR4_PART_OUTPUT_SIZE       0x04000000ULL
#define DDR4_PART_DESC_SIZE         0x01000000ULL

//=============================================================================
// MIG DDR4 Controller Base Addresses (in FPGA fabric)
//=============================================================================
#define MIG0_BASE_ADDR              0x44000000
#define MIG1_BASE_ADDR              0x44010000
#define MIG2_BASE_ADDR              0x44020000
#define MIG3_BASE_ADDR              0x44030000

// MIG Register Offsets
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

// MIG Status Bits
#define MIG_STATUS_CAL_DONE         0x01
#define MIG_STATUS_PLL_LOCKED       0x02
#define MIG_STATUS_IDLE             0x04
#define MIG_STATUS_ECC_ENABLED      0x08
#define MIG_STATUS_ECC_ERROR        0x10
#define MIG_STATUS_ECC_UNCORR       0x20
#define MIG_STATUS_TEMP_ALERT       0x40
#define MIG_STATUS_REFRESH_OVF      0x80

//=============================================================================
// HDMI 4K60 Timing Parameters (CTA-861-G VIC 97)
//=============================================================================
#define HDMI_4K60_H_ACTIVE          3840
#define HDMI_4K60_H_FRONT_PORCH     176
#define HDMI_4K60_H_SYNC            88
#define HDMI_4K60_H_BACK_PORCH      296
#define HDMI_4K60_H_TOTAL           4400

#define HDMI_4K60_V_ACTIVE          2160
#define HDMI_4K60_V_FRONT_PORCH     8
#define HDMI_4K60_V_SYNC            10
#define HDMI_4K60_V_BACK_PORCH      72
#define HDMI_4K60_V_TOTAL           2250

#define HDMI_4K60_PIXEL_CLK_HZ      594000000ULL

//=============================================================================
// CGH Parameter Set Structure (64 bytes per set)
//=============================================================================
typedef struct {
    uint32_t wavelength_nm_x1000;    // offset 0x00
    uint32_t depth_min_um;           // offset 0x04
    uint32_t depth_max_um;           // offset 0x08
    uint32_t depth_planes;           // offset 0x0C
    uint32_t input_width;            // offset 0x10
    uint32_t input_height;           // offset 0x14
    uint32_t output_width;           // offset 0x18
    uint32_t output_height;          // offset 0x1C
    uint32_t fft_size;               // offset 0x20
    uint32_t propagation_model;      // offset 0x24
    uint32_t phase_quantize_bits;    // offset 0x28
    uint32_t color_channel;          // offset 0x2C
    uint32_t pixel_pitch_um_x100;    // offset 0x30
    uint32_t fill_factor_percent;    // offset 0x34
    uint32_t reserved_0;             // offset 0x38
    uint32_t reserved_1;             // offset 0x3C
} cgh_param_set_t;

// Propagation models
#define CGH_PROP_FRESNEL            0
#define CGH_PROP_ANGULAR_SPECTRUM   1
#define CGH_PROP_FRAUNHOFER         2

// Color channels
#define CGH_COLOR_RED               0  // 638 nm
#define CGH_COLOR_GREEN             1  // 532 nm
#define CGH_COLOR_BLUE              2  // 450 nm

//=============================================================================
// DMA Descriptor Structure (32 bytes)
//=============================================================================
typedef struct {
    uint32_t src_addr_lo;
    uint32_t src_addr_hi;
    uint32_t dst_addr_lo;
    uint32_t dst_addr_hi;
    uint32_t length;
    uint32_t control;
    uint32_t next_desc_lo;
    uint32_t next_desc_hi;
} dma_desc_t;

#define DMA_CTRL_IRQ_ON_COMPLETE    0x01
#define DMA_CTRL_CHAIN              0x02
#define DMA_CTRL_CRC_ENABLE         0x04
#define DMA_CTRL_DIR_HOST_TO_FPGA   0x08
#define DMA_CTRL_LAST_DESC          0x10

//=============================================================================
// Helper Macros for Register Access
//=============================================================================
#define REG_READ(offset)            (*(volatile uint32_t *)(BAR0_BASE + (offset)))
#define REG_WRITE(offset, value)    (*(volatile uint32_t *)(BAR0_BASE + (offset)) = (value))
#define REG_SET_BITS(offset, mask)  (REG_WRITE(offset, REG_READ(offset) | (mask)))
#define REG_CLR_BITS(offset, mask)  (REG_WRITE(offset, REG_READ(offset) & ~(mask)))

#endif // REGISTERS_H
