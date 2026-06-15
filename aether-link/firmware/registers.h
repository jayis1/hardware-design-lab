// registers.h — Complete MMIO Register Map for Aether-Link FPGA
// All hardware block base addresses, register offsets, and bit definitions.
// This file is the single source of truth for all memory-mapped I/O.

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

// ============================================================================
// AXI4-Lite Peripheral Address Map (MicroBlaze View)
// ============================================================================

// Clocking
#define MMCM_BASE_ADDR          0x80000000
#define MMCM_SIZE               0x00010000

// GPIO Controllers
#define GPIO_LEDS_BASE          0x80010000
#define GPIO_QSFP0_BASE         0x80020000
#define GPIO_QSFP1_BASE         0x80030000
#define GPIO_SYSTEM_BASE        0x80040000

// Communication Peripherals
#define I2C_MASTER_BASE         0x80050000
#define UART_BASE               0x80060000
#define SPI_MASTER_BASE         0x80070000

// Fan PWM Controller
#define FAN_PWM_BASE            0x80080000

// TCP Offload Engine (TOE)
#define TOE_REG_BASE            0x80100000
#define TOE_REG_SIZE            0x00010000

// NVMe-oF PDU Engine
#define NVMEOF_REG_BASE         0x80200000
#define NVMEOF_REG_SIZE         0x00010000

// NVMe Command Processor
#define NVME_CMD_PROC_BASE      0x80300000
#define NVME_CMD_PROC_SIZE      0x00010000

// PCIe Configuration and Status
#define PCIE_CFG_BASE           0x80400000
#define PCIE_CFG_SIZE           0x00010000

// MicroBlaze Local BRAM
#define BRAM_BASE               0x90000000
#define BRAM_SIZE               0x00010000  // 64 KB

// ============================================================================
// MMCM Register Map (Xilinx MMCME2_ADV Dynamic Reconfiguration)
// ============================================================================

typedef struct {
    volatile uint32_t CR;           // 0x00: Control Register
    volatile uint32_t SR;           // 0x04: Status Register
    volatile uint32_t CLKOUT0_1;    // 0x08: CLKOUT0/1 Divide
    volatile uint32_t CLKOUT2_3;    // 0x0C: CLKOUT2/3 Divide
    volatile uint32_t CLKOUT4_5;    // 0x10: CLKOUT4/5 Divide
    volatile uint32_t CLKOUT6;      // 0x14: CLKOUT6 Divide
    volatile uint32_t CLKFBOUT;     // 0x18: Feedback Divide
    volatile uint32_t DIVCLK;       // 0x1C: DIVCLK Divide
    volatile uint32_t LOCK;         // 0x20: Lock Register
    volatile uint32_t FILTER;       // 0x24: Digital Filter
    volatile uint32_t POWER;        // 0x28: Power Register
    volatile uint32_t RESERVED[53]; // 0x2C-0xFC: Reserved
} mmcm_regs_t;

// MMCM Control Register bits
#define MMCM_CR_START      0x0001
#define MMCM_CR_RESET      0x0002
#define MMCM_CR_PWRDWN     0x0004
#define MMCM_CR_CLKINSEL   0x0008

// MMCM Status Register bits
#define MMCM_SR_LOCKED     0x0001
#define MMCM_SR_CLKINSTOP  0x0002
#define MMCM_SR_CLKFBSTOP  0x0004

// ============================================================================
// GPIO Register Map (Xilinx AXI GPIO v2.0)
// ============================================================================

typedef struct {
    volatile uint32_t GPIO_DATA;     // 0x00: Channel 1 Data Register
    volatile uint32_t GPIO_TRI;      // 0x04: Channel 1 Tristate Register
    volatile uint32_t GPIO2_DATA;    // 0x08: Channel 2 Data Register
    volatile uint32_t GPIO2_TRI;     // 0x0C: Channel 2 Tristate Register
    volatile uint32_t RESERVED[67];  // 0x10-0x118: Reserved
    volatile uint32_t GIER;          // 0x11C: Global Interrupt Enable
    volatile uint32_t IPISR;         // 0x120: IP Interrupt Status
    volatile uint32_t IPIER;         // 0x128: IP Interrupt Enable
} gpio_regs_t;

// ============================================================================
// I2C Register Map (Xilinx AXI IIC v2.1)
// ============================================================================

typedef struct {
    volatile uint32_t RESERVED0[7];  // 0x00-0x18: Reserved
    volatile uint32_t GIE;           // 0x1C: Global Interrupt Enable
    volatile uint32_t ISR;           // 0x20: Interrupt Status Register
    volatile uint32_t IER;           // 0x28: Interrupt Enable Register
    volatile uint32_t RESERVED1[5];  // 0x2C-0x3C: Reserved
    volatile uint32_t SOFTR;         // 0x40: Soft Reset Register
    volatile uint32_t RESERVED2[47]; // 0x44-0xFC: Reserved
    volatile uint32_t CR;            // 0x100: Control Register
    volatile uint32_t SR;            // 0x104: Status Register
    volatile uint32_t TX_FIFO;       // 0x108: Transmit FIFO
    volatile uint32_t RX_FIFO;       // 0x10C: Receive FIFO
    volatile uint32_t ADR;           // 0x110: Slave Address Register
    volatile uint32_t TX_FIFO_OCY;   // 0x114: Transmit FIFO Occupancy
    volatile uint32_t RX_FIFO_OCY;   // 0x118: Receive FIFO Occupancy
    volatile uint32_t TEN_ADR;       // 0x11C: 10-bit Address Register
    volatile uint32_t RX_FIFO_PIRQ;  // 0x120: RX FIFO Programmable Depth
    volatile uint32_t GPO;           // 0x124: General Purpose Output
    volatile uint32_t TSUSTA;        // 0x128: Timing Setup (Start)
    volatile uint32_t TSUSTO;        // 0x12C: Timing Setup (Stop)
    volatile uint32_t THDSTA;        // 0x130: Timing Hold (Start)
    volatile uint32_t THD_STO;       // 0x134: Timing Hold (Stop)
    volatile uint32_t TBUF;          // 0x138: Bus Free Time
    volatile uint32_t THIGH;         // 0x13C: High Period
    volatile uint32_t TLOW;          // 0x140: Low Period
    volatile uint32_t THD_DAT;       // 0x144: Data Hold Time
} i2c_regs_t;

// I2C Control Register bits
#define I2C_CR_EN       0x00000001
#define I2C_CR_TXAK     0x00000002
#define I2C_CR_MSMS     0x00000004
#define I2C_CR_TX       0x00000008
#define I2C_CR_RSTA     0x00000010
#define I2C_CR_CLR_FIFO 0x00000020
#define I2C_CR_GC_EN    0x00000040

// I2C Status Register bits
#define I2C_SR_RXAK     0x00000001
#define I2C_SR_BB       0x00000002
#define I2C_SR_AAS      0x00000004
#define I2C_SR_SRW      0x00000008
#define I2C_SR_TX_FIFO_EMPTY 0x00000010
#define I2C_SR_RX_FIFO_EMPTY 0x00000020
#define I2C_SR_TX_FIFO_FULL  0x00000040
#define I2C_SR_RX_FIFO_FULL  0x00000080

// ============================================================================
// UART Register Map (Xilinx AXI UART Lite v2.0)
// ============================================================================

typedef struct {
    volatile uint32_t RX_FIFO;       // 0x00: Receive Data FIFO
    volatile uint32_t TX_FIFO;       // 0x04: Transmit Data FIFO
    volatile uint32_t STAT;          // 0x08: Status Register
    volatile uint32_t CTRL;          // 0x0C: Control Register
} uart_regs_t;

// UART Status Register bits
#define UART_SR_RX_FIFO_VALID_DATA  0x01
#define UART_SR_RX_FIFO_FULL        0x02
#define UART_SR_TX_FIFO_EMPTY       0x04
#define UART_SR_TX_FIFO_FULL        0x08
#define UART_SR_INTR_ENABLED        0x10
#define UART_SR_OVERRUN_ERROR       0x20
#define UART_SR_FRAME_ERROR         0x40
#define UART_SR_PARITY_ERROR        0x80

// UART Control Register bits
#define UART_CTRL_RST_TX_FIFO       0x01
#define UART_CTRL_RST_RX_FIFO       0x02
#define UART_CTRL_EN_INTR           0x10

// ============================================================================
// SPI Register Map (Xilinx AXI Quad SPI v3.2)
// ============================================================================

typedef struct {
    volatile uint32_t SRR;           // 0x00: Software Reset
    volatile uint32_t SPICR;         // 0x04: SPI Control
    volatile uint32_t SPISR;         // 0x08: SPI Status
    volatile uint32_t SPI_DTR;       // 0x0C: SPI Data Transmit
    volatile uint32_t SPI_DRR;       // 0x10: SPI Data Receive
    volatile uint32_t SPISSR;        // 0x14: SPI Slave Select
    volatile uint32_t SPI_TX_FIFO_OCY; // 0x18: TX FIFO Occupancy
    volatile uint32_t SPI_RX_FIFO_OCY; // 0x1C: RX FIFO Occupancy
    volatile uint32_t DGIER;         // 0x20: Global Interrupt Enable
    volatile uint32_t IPISR;         // 0x24: IP Interrupt Status
    volatile uint32_t IPIER;         // 0x28: IP Interrupt Enable
} spi_regs_t;

// SPI Control Register bits
#define SPICR_SPE       0x00000001  // SPI Enable
#define SPICR_LOOP      0x00000002  // Loopback Mode
#define SPICR_CPOL      0x00000004  // Clock Polarity
#define SPICR_CPHA      0x00000008  // Clock Phase
#define SPICR_TXFIFO_RST 0x00000010 // TX FIFO Reset
#define SPICR_RXFIFO_RST 0x00000020 // RX FIFO Reset
#define SPICR_MAN_SS    0x00000040  // Manual Slave Select
#define SPICR_MASTER    0x00000080  // Master Mode
#define SPICR_QUAD      0x00000100  // Quad SPI Mode

// ============================================================================
// TOE (TCP Offload Engine) Register Map
// ============================================================================

// TOE Register Offsets
#define TOE_REG_CTRL             0x00
#define TOE_REG_STATUS           0x04
#define TOE_REG_CONN_TABLE_BASE  0x08
#define TOE_REG_CONN_COUNT       0x0C
#define TOE_REG_PORT0_MAC_LO     0x10
#define TOE_REG_PORT0_MAC_HI     0x14
#define TOE_REG_PORT1_MAC_LO     0x18
#define TOE_REG_PORT1_MAC_HI     0x1C
#define TOE_REG_PORT0_IP         0x20
#define TOE_REG_PORT1_IP         0x24
#define TOE_REG_PORT0_NETMASK    0x28
#define TOE_REG_PORT1_NETMASK    0x2C
#define TOE_REG_PORT0_GATEWAY    0x30
#define TOE_REG_PORT1_GATEWAY    0x34
#define TOE_REG_ARP_TABLE_BASE   0x38
#define TOE_REG_TX_DESC_BASE     0x3C
#define TOE_REG_RX_DESC_BASE     0x40
#define TOE_REG_INTERRUPT_MASK   0x44
#define TOE_REG_INTERRUPT_STATUS 0x48
#define TOE_REG_STATS_BASE       0x4C
#define TOE_REG_CMD_QUEUE        0x100 // Command queue (offset from CTRL)
#define TOE_REG_RST_QUEUE        0x104 // RST command queue

// TOE Control Register bits
#define TOE_CTRL_ENABLE          0x00000001
#define TOE_CTRL_RESET           0x00000002
#define TOE_CTRL_PORT0_EN        0x00000004
#define TOE_CTRL_PORT1_EN        0x00000008
#define TOE_CTRL_TSO_EN          0x00000010
#define TOE_CTRL_LRO_EN          0x00000020
#define TOE_CTRL_DCTCP_EN        0x00000040
#define TOE_CTRL_CRC32C_EN       0x00000080
#define TOE_CTRL_ARP_EN          0x00000100
#define TOE_CTRL_ICMP_EN         0x00000200
#define TOE_CTRL_DHCP_EN         0x00000400

// TOE Status Register bits
#define TOE_STATUS_READY         0x00000001
#define TOE_STATUS_PORT0_LINK    0x00000002
#define TOE_STATUS_PORT1_LINK    0x00000004
#define TOE_STATUS_ERROR         0x00000008
#define TOE_STATUS_BUSY          0x00000010
#define TOE_STATUS_PORT0_ACTIVE  0x00000020
#define TOE_STATUS_PORT1_ACTIVE  0x00000040

// TOE Interrupt bits
#define TOE_INT_CONN_ESTABLISHED 0x00000001
#define TOE_INT_CONN_CLOSED      0x00000002
#define TOE_INT_DATA_RECEIVED    0x00000004
#define TOE_INT_ERROR            0x00000008
#define TOE_INT_RETX_THRESHOLD   0x00000010
#define TOE_INT_KEEPALIVE        0x00000020
#define TOE_INT_LINK_CHANGE      0x00000040

// TOE Command Types
#define TOE_CMD_CONNECT          0x01
#define TOE_CMD_DISCONNECT       0x02
#define TOE_CMD_SEND             0x03
#define TOE_CMD_RST              0x04
#define TOE_CMD_UPDATE_WINDOW    0x05
#define TOE_CMD_SET_MSS          0x06

// ============================================================================
// NVMe-oF PDU Engine Register Map
// ============================================================================

// NVMe-oF Register Offsets
#define NVMEOF_REG_CTRL          0x00
#define NVMEOF_REG_STATUS        0x04
#define NVMEOF_REG_CMD_QUEUE     0x08
#define NVMEOF_REG_RESP_QUEUE    0x0C
#define NVMEOF_REG_DATA_BUF_BASE 0x10
#define NVMEOF_REG_CONN_MAP      0x14
#define NVMEOF_REG_MAX_CONNS     0x18
#define NVMEOF_REG_IOPS_CNT_LO   0x1C
#define NVMEOF_REG_IOPS_CNT_HI   0x20
#define NVMEOF_REG_ERROR_CNT     0x24
#define NVMEOF_REG_CAPSULE_SIZE  0x28
#define NVMEOF_REG_DIGEST_MODE   0x2C
#define NVMEOF_REG_TIMEOUT_US    0x30
#define NVMEOF_REG_RETRY_COUNT   0x34

// NVMe-oF Control Register bits
#define NVMEOF_CTRL_ENABLE       0x00000001
#define NVMEOF_CTRL_RESET        0x00000002
#define NVMEOF_CTRL_HDGST_EN     0x00000004  // Header digest enable
#define NVMEOF_CTRL_DDGST_EN     0x00000008  // Data digest enable
#define NVMEOF_CTRL_ZERO_COPY    0x00000010  // Zero-copy DMA to host
#define NVMEOF_CTRL_IN_ORDER     0x00000020  // In-order completion

// NVMe-oF Status Register bits
#define NVMEOF_STATUS_READY      0x00000001
#define NVMEOF_STATUS_PDU_PROC   0x00000002  // PDU being processed
#define NVMEOF_STATUS_ERROR      0x00000004
#define NVMEOF_STATUS_DIGEST_ERR 0x00000008
#define NVMEOF_STATUS_CAPSULE_ERR 0x00000010

// ============================================================================
// NVMe Command Processor Register Map
// ============================================================================

// NVMe Command Processor Register Offsets
#define NVME_REG_CAP             0x00   // Controller Capabilities (RO, 8B)
#define NVME_REG_VS              0x08   // Version (RO, 4B)
#define NVME_REG_INTMS           0x0C   // Interrupt Mask Set (RW, 4B)
#define NVME_REG_INTMC           0x10   // Interrupt Mask Clear (WO, 4B)
#define NVME_REG_CC              0x14   // Controller Configuration (RW, 4B)
#define NVME_REG_CSTS            0x1C   // Controller Status (RO, 4B)
#define NVME_REG_NSSR            0x20   // NVM Subsystem Reset (WO, 4B)
#define NVME_REG_AQA             0x24   // Admin Queue Attributes (RW, 4B)
#define NVME_REG_ASQ             0x28   // Admin SQ Base Address (RW, 8B)
#define NVME_REG_ACQ             0x30   // Admin CQ Base Address (RW, 8B)
#define NVME_REG_CMBLOC          0x38   // CMB Location (RO, 4B)
#define NVME_REG_CMBSZ           0x3C   // CMB Size (RO, 4B)

// Doorbell registers start at offset 0x1000
#define NVME_REG_SQ0TDBL         0x1000 // SQ0 Tail Doorbell
#define NVME_REG_CQ0HDBL         0x1004 // CQ0 Head Doorbell
#define NVME_DOORBELL_STRIDE     0x0008 // 8 bytes per doorbell pair

// NVMe CAP register fields
#define NVME_CAP_MQES_MASK       0x0000FFFF  // Max Queue Entries Supported
#define NVME_CAP_MQES_SHIFT      0
#define NVME_CAP_CQR_MASK        0x00010000  // Contiguous Queues Required
#define NVME_CAP_AMS_MASK        0x00060000  // Arbitration Mechanism Supported
#define NVME_CAP_TO_MASK         0xFF000000  // Timeout (500ms units)
#define NVME_CAP_TO_SHIFT        24
#define NVME_CAP_DSTRD_MASK      0x00000F00  // Doorbell Stride
#define NVME_CAP_DSTRD_SHIFT     8
#define NVME_CAP_NSSRS_MASK      0x00100000  // NVM Subsystem Reset Supported
#define NVME_CAP_CSS_MASK        0x00C00000  // Command Sets Supported
#define NVME_CAP_BPS_MASK        0x00008000  // Boot Partition Support
#define NVME_CAP_MPSMIN_MASK     0x000F0000  // Memory Page Size Minimum
#define NVME_CAP_MPSMIN_SHIFT    16
#define NVME_CAP_MPSMAX_MASK     0x00F00000  // Memory Page Size Maximum
#define NVME_CAP_MPSMAX_SHIFT    20
#define NVME_CAP_PMRS_MASK       0x01000000  // Persistent Memory Region Supported
#define NVME_CAP_CMBS_MASK       0x02000000  // Controller Memory Buffer Supported

// NVMe CC register fields
#define NVME_CC_EN               0x00000001  // Enable
#define NVME_CC_IOCQES_MASK      0x00000F00  // I/O CQ Entry Size
#define NVME_CC_IOCQES_SHIFT     8
#define NVME_CC_IOSQES_MASK      0x0000F000  // I/O SQ Entry Size
#define NVME_CC_IOSQES_SHIFT     12
#define NVME_CC_SHN_MASK         0x0000C000  // Shutdown Notification
#define NVME_CC_SHN_SHIFT        14
#define NVME_CC_AMS_MASK         0x00070000  // Arbitration Mechanism Selected
#define NVME_CC_AMS_SHIFT        16
#define NVME_CC_MPS_MASK         0x00F00000  // Memory Page Size
#define NVME_CC_MPS_SHIFT        20
#define NVME_CC_CSS_MASK         0x07000000  // I/O Command Set Selected
#define NVME_CC_CSS_SHIFT        24

// NVMe CSTS register fields
#define NVME_CSTS_RDY            0x00000001  // Ready
#define NVME_CSTS_CFS            0x00000002  // Controller Fatal Status
#define NVME_CSTS_SHST_MASK      0x0000000C  // Shutdown Status
#define NVME_CSTS_SHST_SHIFT     2
#define NVME_CSTS_NSSRO          0x00000010  // NVM Subsystem Reset Occurred
#define NVME_CSTS_PP_MASK        0x0000FF00  // Processing Paused

// ============================================================================
// PCIe Configuration and Status Register Map
// ============================================================================

// PCIe Config Register Offsets
#define PCIE_REG_LINK_STATUS     0x00
#define PCIE_REG_LINK_CONTROL    0x04
#define PCIE_REG_LINK_CAP        0x08
#define PCIE_REG_DEVICE_STATUS   0x0C
#define PCIE_REG_DEVICE_CONTROL  0x10
#define PCIE_REG_AER_STATUS      0x14
#define PCIE_REG_AER_MASK        0x18
#define PCIE_REG_MSIX_STATUS     0x1C
#define PCIE_REG_VENDOR_ID       0x20
#define PCIE_REG_DEVICE_ID       0x24
#define PCIE_REG_SUBSYS_VENDOR   0x28
#define PCIE_REG_SUBSYS_ID       0x2C
#define PCIE_REG_CLASS_CODE      0x30
#define PCIE_REG_BAR0_ENABLE     0x34

// PCIe Link Status bits
#define PCIE_LINK_SPEED_MASK     0x0000000F
#define PCIE_LINK_SPEED_GEN1     0x01
#define PCIE_LINK_SPEED_GEN2     0x02
#define PCIE_LINK_SPEED_GEN3     0x03
#define PCIE_LINK_SPEED_GEN4     0x04
#define PCIE_LINK_WIDTH_MASK     0x000003F0
#define PCIE_LINK_WIDTH_SHIFT    4
#define PCIE_LINK_TRAINING       0x00000400
#define PCIE_LINK_UP             0x00000800
#define PCIE_SLOT_CLOCK_CONFIG   0x00001000

// ============================================================================
// Aether-Link Vendor-Specific Registers (BAR0 offset 0x2000)
// ============================================================================

#define AETHER_VENDOR_BASE       0x2000

#define AETHER_REG_MAGIC         0x2000  // Magic: 0xA37A4C10
#define AETHER_REG_VERSION       0x2004  // Firmware version
#define AETHER_REG_FEATURES      0x2008  // Feature bitmap
#define AETHER_REG_STATUS        0x200C  // Global status
#define AETHER_REG_CONTROL       0x2010  // Global control
#define AETHER_REG_TEMP_FPGA     0x2014  // FPGA temp (0.01°C LSB)
#define AETHER_REG_TEMP_QSFP0    0x2016  // QSFP0 temp
#define AETHER_REG_TEMP_QSFP1    0x2018  // QSFP1 temp
#define AETHER_REG_POWER_MW      0x201A  // Board power (mW)
#define AETHER_REG_FAN0_RPM      0x201E  // Fan 0 RPM
#define AETHER_REG_FAN1_RPM      0x2020  // Fan 1 RPM
#define AETHER_REG_FAN_PWM       0x2022  // Fan PWM duty
#define AETHER_REG_CONN_COUNT    0x2024  // Active connections
#define AETHER_REG_CONN_TABLE    0x2028  // Connection table DDR4 base
#define AETHER_REG_PORT0_STATUS  0x2030  // Port 0 link status
#define AETHER_REG_PORT0_SPEED   0x2034  // Port 0 speed (Mbps)
#define AETHER_REG_PORT0_MAC_LO  0x2038  // Port 0 MAC [31:0]
#define AETHER_REG_PORT0_MAC_HI  0x203C  // Port 0 MAC [47:32]
#define AETHER_REG_PORT1_STATUS  0x2040  // Port 1 link status
#define AETHER_REG_PORT1_SPEED   0x2044  // Port 1 speed (Mbps)
#define AETHER_REG_PORT1_MAC_LO  0x2048  // Port 1 MAC [31:0]
#define AETHER_REG_PORT1_MAC_HI  0x204C  // Port 1 MAC [47:32]
#define AETHER_REG_STATS_TX_PKTS 0x2050  // TX packets (64-bit)
#define AETHER_REG_STATS_RX_PKTS 0x2058  // RX packets (64-bit)
#define AETHER_REG_STATS_TX_BYTES 0x2060 // TX bytes (64-bit)
#define AETHER_REG_STATS_RX_BYTES 0x2068 // RX bytes (64-bit)
#define AETHER_REG_STATS_TX_DROP 0x2070  // TX drops (64-bit)
#define AETHER_REG_STATS_RX_DROP 0x2078  // RX drops (64-bit)
#define AETHER_REG_STATS_TCP_RETX 0x2080 // TCP retransmissions (64-bit)
#define AETHER_REG_STATS_NVME_IOPS 0x2088 // NVMe IOPS (64-bit)
#define AETHER_REG_FW_UPDATE_ADDR 0x2090 // FW update SPI address
#define AETHER_REG_FW_UPDATE_LEN  0x2098 // FW update length
#define AETHER_REG_FW_UPDATE_GO   0x209C // FW update trigger
#define AETHER_REG_CONN_ADD_TARGET 0x20A0 // Add connection (32B)
#define AETHER_REG_CONN_DEL       0x20C0 // Delete connection
#define AETHER_REG_ERROR_LOG_BASE 0x20D0 // Error log DDR4 base
#define AETHER_REG_ERROR_LOG_IDX  0x20D8 // Error log write index
#define AETHER_REG_DEBUG_UART_TX  0x20E0 // Debug UART TX byte
#define AETHER_REG_DEBUG_UART_RX  0x20E1 // Debug UART RX byte
#define AETHER_REG_SCRATCH0      0x20F0  // Scratch 0
#define AETHER_REG_SCRATCH1      0x20F4  // Scratch 1
#define AETHER_REG_SCRATCH2      0x20F8  // Scratch 2
#define AETHER_REG_SCRATCH3      0x20FC  // Scratch 3

// Aether Magic Number
#define AETHER_MAGIC_VALUE       0xA37A4C10

// Aether Feature bits
#define AETHER_FEAT_NVMEOF_TCP   0x00000001
#define AETHER_FEAT_DUAL_PORT    0x00000002
#define AETHER_FEAT_RDMA_ZCOPY   0x00000004
#define AETHER_FEAT_TSO          0x00000008
#define AETHER_FEAT_LRO          0x00000010
#define AETHER_FEAT_DCTCP        0x00000020
#define AETHER_FEAT_ECN          0x00000040
#define AETHER_FEAT_DH_CHAP      0x00000080
#define AETHER_FEAT_CRC32C       0x00000100
#define AETHER_FEAT_FW_UPDATE    0x00000200
#define AETHER_FEAT_SECURE_BOOT  0x00000400
#define AETHER_FEAT_DDR4_ECC     0x00000800
#define AETHER_FEAT_THERMAL      0x00001000
#define AETHER_FEAT_TELEMETRY    0x00002000
#define AETHER_FEAT_DEBUG_UART   0x00004000

// Aether Status bits
#define AETHER_STATUS_FPGA_READY 0x00000001
#define AETHER_STATUS_DDR4_CAL   0x00000002
#define AETHER_STATUS_PCIE_UP    0x00000004
#define AETHER_STATUS_PORT0_UP   0x00000008
#define AETHER_STATUS_PORT1_UP   0x00000010
#define AETHER_STATUS_NVME_EN    0x00000020
#define AETHER_STATUS_THROTTLING 0x00000040
#define AETHER_STATUS_FW_UPDATING 0x00000080
#define AETHER_STATUS_ERROR      0x00000100
#define AETHER_STATUS_SECURE_OK  0x00000200
#define AETHER_STATUS_QSFP0_PRES 0x00000400
#define AETHER_STATUS_QSFP1_PRES 0x00000800

// ============================================================================
// DDR4 Memory Map (8 GB total)
// ============================================================================

// Channel A (4 GB)
#define DDR4_CHA_BASE               0x00000000
#define DDR4_CHA_CONN_TABLE         0x00000000  // 256 × 4KB = 1MB
#define DDR4_CHA_ARP_TABLE          0x00100000  // 256 × 16B = 4KB
#define DDR4_CHA_CONN_BUFFERS       0x00104000  // 256 × 512KB = 128MB
#define DDR4_CHA_TX_DESC_RINGS      0x08104000  // TX descriptor rings
#define DDR4_CHA_RX_DESC_RINGS      0x09104000  // RX descriptor rings
#define DDR4_CHA_STATS_COUNTERS     0x0A104000  // Statistics counters
#define DDR4_CHA_PDU_BUFFER         0x0A108000  // PDU assembly buffer (1MB)
#define DDR4_CHA_ERROR_LOG          0x0A208000  // Error log ring (16KB)
#define DDR4_CHA_RESERVED           0x0A20C000  // Reserved

// Channel B (4 GB)
#define DDR4_CHB_BASE               0x00000000  // (relative to channel B)
#define DDR4_CHB_NVME_DATA_POOL     0x00000000  // NVMe data buffer pool (2GB)
#define DDR4_CHB_NVME_PRP_POOL      0x80000000  // NVMe PRP/SGL page pool (2GB)

// ============================================================================
// Watchdog Timer
// ============================================================================

#define WDT_BASE                    0x80090000
#define WDT_REG_CONTROL             0x00
#define WDT_REG_COUNT               0x04
#define WDT_REG_RESET_VALUE         0x08

#define WDT_CTRL_ENABLE             0x01
#define WDT_CTRL_PET                0x02  // Write to reset counter
#define WDT_TIMEOUT_MS              1000  // 1 second timeout
#define WDT_CLOCK_HZ                200000000  // 200 MHz MicroBlaze clock

#endif // REGISTERS_H
