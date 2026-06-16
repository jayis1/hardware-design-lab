// registers.h — Chronos Pulser Complete MMIO Register Map
// All peripheral registers for the VexRiscv soft CPU on ECP5-45F
// Wishbone bus, 32-bit word-aligned access only

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

//=============================================================================
// Top-Level Address Map (256 MB Wishbone address space, 28-bit)
//=============================================================================

#define DDR3_BASE               0x00000000UL  // 256 MB DDR3 SDRAM
#define BOOT_ROM_BASE           0x10000000UL  // 8 KB internal BRAM boot ROM
#define SYS_BASE                0x20000000UL  // System Controller
#define UART_BASE               0x20001000UL  // Debug UART
#define TIMER_BASE              0x20002000UL  // Timer / Timestamp Counter
#define GPIO_BASE               0x20003000UL  // GPIO Controller
#define I2C_BASE                0x20004000UL  // I2C Master
#define SPI_FLASH_BASE          0x20005000UL  // SPI Master (flash)
#define SPI_CFG_BASE            0x20006000UL  // SPI Master (ADC/VGA config)
#define TDC_BASE                0x30000000UL  // TDC Core
#define ADC_CAP_BASE            0x30004000UL  // ADC Capture Engine
#define PULSE_BASE              0x30008000UL  // Pulse Generator Controller
#define USB_BASE                0x3000C000UL  // USB Bridge (FT601 FIFO)
#define ACQ_BASE                0x30010000UL  // Acquisition Sequencer

//=============================================================================
// System Controller Registers (SYS — 0x20000000)
//=============================================================================

#define SYS_ID_REG              (SYS_BASE + 0x00)
#define SYS_STATUS_REG          (SYS_BASE + 0x04)
#define SYS_RESET_REG           (SYS_BASE + 0x08)
#define SYS_CLK_CTRL_REG        (SYS_BASE + 0x0C)
#define SYS_LED_CTRL_REG        (SYS_BASE + 0x10)
#define SYS_TEMP_REG            (SYS_BASE + 0x14)
#define SYS_UPTIME_LO_REG       (SYS_BASE + 0x18)
#define SYS_UPTIME_HI_REG       (SYS_BASE + 0x1C)
#define SYS_SCRATCH_REG         (SYS_BASE + 0x20)
#define SYS_BOOT_SRC_REG        (SYS_BASE + 0x24)
#define SYS_INT_EN_REG          (SYS_BASE + 0x28)
#define SYS_INT_STATUS_REG      (SYS_BASE + 0x2C)
#define SYS_WATCHDOG_REG        (SYS_BASE + 0x30)
#define SYS_WDT_KICK_REG        (SYS_BASE + 0x34)

// SYS_ID bit definitions
#define SYS_ID_DEVICE_MASK      0xFFFF0000UL
#define SYS_ID_DEVICE_SHIFT     16
#define SYS_ID_REV_MASK         0x0000FF00UL
#define SYS_ID_REV_SHIFT        8
#define SYS_ID_VARIANT_MASK     0x000000FFUL
#define SYS_ID_DEVICE_CHRONOS   0x4350  // "CP"

// SYS_STATUS bit definitions
#define SYS_STATUS_CFG_DONE     (1 << 0)
#define SYS_STATUS_DDR3_CAL     (1 << 1)
#define SYS_STATUS_USB_ENUM     (1 << 2)
#define SYS_STATUS_TDC_CAL      (1 << 3)
#define SYS_STATUS_OVERTEMP_WARN (1 << 4)
#define SYS_STATUS_OVERTEMP_SHDN (1 << 5)
#define SYS_STATUS_POWER_GOOD   (1 << 8)
#define SYS_STATUS_BOOT_SRC_MASK (0xF << 16)
#define SYS_STATUS_BOOT_GOLDEN  (0 << 16)
#define SYS_STATUS_BOOT_UPDATE  (1 << 16)

// SYS_RESET bit definitions
#define SYS_RESET_TDC           (1 << 0)
#define SYS_RESET_ADC_CAP       (1 << 1)
#define SYS_RESET_PULSE         (1 << 2)
#define SYS_RESET_USB           (1 << 3)
#define SYS_RESET_FULL          (1 << 31)

// SYS_CLK_CTRL bit definitions
#define SYS_CLK_ENABLE_SYS      (1 << 0)
#define SYS_CLK_ENABLE_ADC      (1 << 1)
#define SYS_CLK_ENABLE_TDC      (1 << 2)
#define SYS_CLK_ENABLE_USB      (1 << 3)
#define SYS_CLK_ENABLE_DDR3     (1 << 4)
#define SYS_CLK_ADC_DIV_MASK    (0xFF << 8)
#define SYS_CLK_ADC_DIV_SHIFT   8

// SYS_BOOT_SRC
#define SYS_BOOT_COMMIT         0xACCE55

// SYS_WATCHDOG
#define SYS_WDT_ENABLE          (1 << 31)
#define SYS_WDT_TIMEOUT_MASK    0x00FFFFFFUL
#define SYS_WDT_KICK_MAGIC      0x5AFE0001UL

//=============================================================================
// UART Registers (UART — 0x20001000)
//=============================================================================

#define UART_TX_REG             (UART_BASE + 0x00)
#define UART_RX_REG             (UART_BASE + 0x04)
#define UART_STATUS_REG         (UART_BASE + 0x08)
#define UART_CTRL_REG           (UART_BASE + 0x0C)
#define UART_DIV_REG            (UART_BASE + 0x10)

#define UART_STATUS_TX_FULL     (1 << 0)
#define UART_STATUS_TX_EMPTY    (1 << 1)
#define UART_STATUS_RX_FULL     (1 << 2)
#define UART_STATUS_RX_EMPTY    (1 << 3)
#define UART_STATUS_TX_BUSY     (1 << 4)
#define UART_CTRL_ENABLE        (1 << 0)
#define UART_CTRL_TX_INT_EN     (1 << 1)
#define UART_CTRL_RX_INT_EN     (1 << 2)

//=============================================================================
// Timer Registers (TIMER — 0x20002000)
//=============================================================================

#define TIMER_CTRL_REG          (TIMER_BASE + 0x00)
#define TIMER_PRESCALER_REG     (TIMER_BASE + 0x04)
#define TIMER_COMPARE0_REG      (TIMER_BASE + 0x08)
#define TIMER_COMPARE1_REG      (TIMER_BASE + 0x0C)
#define TIMER_COUNT_REG         (TIMER_BASE + 0x10)
#define TIMER_CAPTURE_REG       (TIMER_BASE + 0x14)
#define TIMER_INT_STATUS_REG    (TIMER_BASE + 0x18)

#define TIMER_CTRL_ENABLE       (1 << 0)
#define TIMER_CTRL_ONE_SHOT     (1 << 1)
#define TIMER_CTRL_AUTO_RELOAD  (1 << 2)
#define TIMER_INT_CMP0          (1 << 0)
#define TIMER_INT_CMP1          (1 << 1)
#define TIMER_INT_CAPTURE       (1 << 2)

//=============================================================================
// GPIO Controller Registers (GPIO — 0x20003000)
//=============================================================================

#define GPIO_IN_REG             (GPIO_BASE + 0x00)
#define GPIO_OUT_REG            (GPIO_BASE + 0x04)
#define GPIO_OE_REG             (GPIO_BASE + 0x08)
#define GPIO_INT_EN_REG         (GPIO_BASE + 0x0C)
#define GPIO_INT_STATUS_REG     (GPIO_BASE + 0x10)
#define GPIO_INT_TYPE_REG       (GPIO_BASE + 0x14)
#define GPIO_INT_POL_REG        (GPIO_BASE + 0x18)
#define GPIO_PULL_EN_REG        (GPIO_BASE + 0x1C)
#define GPIO_PULL_SEL_REG       (GPIO_BASE + 0x20)

//=============================================================================
// I2C Master Registers (I2C — 0x20004000)
//=============================================================================

#define I2C_CTRL_REG            (I2C_BASE + 0x00)
#define I2C_STATUS_REG          (I2C_BASE + 0x04)
#define I2C_TX_REG              (I2C_BASE + 0x08)
#define I2C_RX_REG              (I2C_BASE + 0x0C)
#define I2C_ADDR_REG            (I2C_BASE + 0x10)

#define I2C_CTRL_ENABLE         (1 << 0)
#define I2C_CTRL_INT_EN         (1 << 1)
#define I2C_STATUS_BUSY         (1 << 0)
#define I2C_STATUS_IN_PROGRESS  (1 << 1)
#define I2C_STATUS_COMPLETE     (1 << 2)
#define I2C_STATUS_NACK         (1 << 3)
#define I2C_STATUS_ARB_LOST     (1 << 4)
#define I2C_TX_START            (1 << 8)
#define I2C_TX_STOP             (1 << 9)
#define I2C_TX_READ             (1 << 10)
#define I2C_TX_GO               (1 << 31)
#define I2C_RX_EMPTY            (1 << 8)
#define I2C_RX_FULL             (1 << 9)

//=============================================================================
// SPI Master Registers (SPI_FLASH — 0x20005000, SPI_CFG — 0x20006000)
//=============================================================================

#define SPI_CTRL_REG(base)      ((base) + 0x00)
#define SPI_STATUS_REG(base)    ((base) + 0x04)
#define SPI_TX_REG(base)        ((base) + 0x08)
#define SPI_RX_REG(base)        ((base) + 0x0C)
#define SPI_DIV_REG(base)       ((base) + 0x10)
#define SPI_CS_REG(base)        ((base) + 0x14)

#define SPI_CTRL_ENABLE         (1 << 0)
#define SPI_CTRL_CPOL           (1 << 1)  // Clock polarity (0=low idle, 1=high idle)
#define SPI_CTRL_CPHA           (1 << 2)  // Clock phase (0=sample leading, 1=trailing)
#define SPI_CTRL_LSB_FIRST      (1 << 3)
#define SPI_CTRL_INT_EN         (1 << 4)
#define SPI_STATUS_BUSY         (1 << 0)
#define SPI_STATUS_TX_EMPTY     (1 << 1)
#define SPI_STATUS_RX_FULL      (1 << 2)
#define SPI_TX_START            (1 << 31)

//=============================================================================
// TDC Core Registers (TDC — 0x30000000)
//=============================================================================

#define TDC_CTRL_REG            (TDC_BASE + 0x00)
#define TDC_STATUS_REG          (TDC_BASE + 0x04)
#define TDC_FIFO_RD_REG         (TDC_BASE + 0x08)
#define TDC_CAL_ADDR_REG        (TDC_BASE + 0x0C)
#define TDC_CAL_DATA_REG        (TDC_BASE + 0x10)
#define TDC_COARSE_OVF_REG      (TDC_BASE + 0x14)
#define TDC_TRIG_COUNT_REG      (TDC_BASE + 0x18)
#define TDC_CFG_REG             (TDC_BASE + 0x1C)
#define TDC_VERSION_REG         (TDC_BASE + 0x20)

// TDC_CTRL bit definitions
#define TDC_CTRL_ENABLE         (1 << 0)
#define TDC_CTRL_CONTINUOUS     (1 << 1)
#define TDC_CTRL_SINGLE_SHOT    (1 << 2)
#define TDC_CTRL_CAL_MODE       (1 << 3)
#define TDC_CTRL_CLEAR_FIFO     (1 << 4)
#define TDC_CTRL_TRIG_SRC_MASK  (0xF << 8)
#define TDC_CTRL_TRIG_INTERNAL  (0 << 8)
#define TDC_CTRL_TRIG_EXTERNAL  (1 << 8)
#define TDC_CTRL_TRIG_ADC_OR    (2 << 8)
#define TDC_CTRL_AVG_DEPTH_MASK (0xFF << 16)
#define TDC_CTRL_AVG_DEPTH_SHIFT 16

// TDC_STATUS bit definitions
#define TDC_STATUS_BUSY         (1 << 0)
#define TDC_STATUS_FIFO_EMPTY   (1 << 1)
#define TDC_STATUS_FIFO_FULL    (1 << 2)
#define TDC_STATUS_FIFO_OVERFLOW (1 << 3)
#define TDC_STATUS_FIFO_COUNT_MASK (0xFF << 8)
#define TDC_STATUS_CAL_DONE     (1 << 16)

// TDC_CFG bit definitions
#define TDC_CFG_RISING          (1 << 0)
#define TDC_CFG_FALLING         (1 << 1)
#define TDC_CFG_BOTH            (1 << 2)
#define TDC_CFG_THRESHOLD_MASK  (0xFF << 8)
#define TDC_CFG_DEADTIME_MASK   (0xFF << 16)

// TDC_VERSION expected values
#define TDC_CORE_ID             0x5444  // "TD"
#define TDC_VERSION_MAJOR       1
#define TDC_VERSION_MINOR       0

//=============================================================================
// ADC Capture Engine Registers (ADC_CAP — 0x30004000)
//=============================================================================

#define ADC_CTRL_REG            (ADC_CAP_BASE + 0x00)
#define ADC_STATUS_REG          (ADC_CAP_BASE + 0x04)
#define ADC_FIFO_RD_REG         (ADC_CAP_BASE + 0x08)
#define ADC_SAMPLES_REG         (ADC_CAP_BASE + 0x0C)
#define ADC_TRIG_DELAY_REG      (ADC_CAP_BASE + 0x10)
#define ADC_DMA_BASE_REG        (ADC_CAP_BASE + 0x14)
#define ADC_DMA_LEN_REG         (ADC_CAP_BASE + 0x18)
#define ADC_DMA_STATUS_REG      (ADC_CAP_BASE + 0x1C)
#define ADC_SPI_TX_REG          (ADC_CAP_BASE + 0x20)
#define ADC_SPI_RX_REG          (ADC_CAP_BASE + 0x24)
#define ADC_CAL_OFFSET_REG      (ADC_CAP_BASE + 0x28)
#define ADC_CAL_GAIN_REG        (ADC_CAP_BASE + 0x2C)

// ADC_CTRL bit definitions
#define ADC_CTRL_ENABLE         (1 << 0)
#define ADC_CTRL_CONTINUOUS     (1 << 1)
#define ADC_CTRL_SINGLE_SHOT    (1 << 2)
#define ADC_CTRL_DECIMATION_EN  (1 << 3)
#define ADC_CTRL_AVERAGING_EN   (1 << 4)
#define ADC_CTRL_DECIMATION_MASK (0xF << 8)
#define ADC_CTRL_AVERAGING_MASK (0xFF << 16)
#define ADC_CTRL_TRIGGERED      (1 << 24)
#define ADC_CTRL_FREE_RUNNING   (1 << 25)
#define ADC_CTRL_CLEAR_FIFO     (1 << 26)

// ADC_STATUS bit definitions
#define ADC_STATUS_BUSY         (1 << 0)
#define ADC_STATUS_FIFO_EMPTY   (1 << 1)
#define ADC_STATUS_FIFO_FULL    (1 << 2)
#define ADC_STATUS_FIFO_OVERFLOW (1 << 3)
#define ADC_STATUS_OVERRANGE    (1 << 4)
#define ADC_STATUS_FIFO_COUNT_MASK (0xFFFF << 8)

// ADC_DMA_STATUS bit definitions
#define ADC_DMA_BUSY            (1 << 0)
#define ADC_DMA_DONE            (1 << 1)
#define ADC_DMA_ERROR           (1 << 2)
#define ADC_DMA_BYTES_MASK      (0xFFFF << 16)

// ADC SPI transaction
#define ADC_SPI_WRITE           (1 << 16)
#define ADC_SPI_START           (1 << 31)
#define ADC_SPI_DONE            (1 << 8)

//=============================================================================
// Pulse Generator Registers (PULSE — 0x30008000)
//=============================================================================

#define PULSE_CTRL_REG          (PULSE_BASE + 0x00)
#define PULSE_STATUS_REG        (PULSE_BASE + 0x04)
#define PULSE_PERIOD_REG        (PULSE_BASE + 0x08)
#define PULSE_AMPLITUDE_REG     (PULSE_BASE + 0x0C)
#define PULSE_DELAY_REG         (PULSE_BASE + 0x10)
#define PULSE_WIDTH_REG         (PULSE_BASE + 0x14)
#define PULSE_CAL_REG           (PULSE_BASE + 0x18)

// PULSE_CTRL bit definitions
#define PULSE_CTRL_ENABLE       (1 << 0)
#define PULSE_CTRL_CONTINUOUS   (1 << 1)
#define PULSE_CTRL_SINGLE       (1 << 2)
#define PULSE_CTRL_EXT_TRIG     (1 << 3)
#define PULSE_CTRL_BURST        (1 << 4)
#define PULSE_CTRL_BURST_COUNT_MASK (0xFF << 8)

// PULSE_STATUS bit definitions
#define PULSE_STATUS_BUSY       (1 << 0)
#define PULSE_STATUS_BURST_ACTIVE (1 << 1)
#define PULSE_STATUS_COUNT_MASK (0xFFFF << 16)

//=============================================================================
// USB Bridge Registers (USB — 0x3000C000)
//=============================================================================

#define USB_CTRL_REG            (USB_BASE + 0x00)
#define USB_STATUS_REG          (USB_BASE + 0x04)
#define USB_TX_LEN_REG          (USB_BASE + 0x08)
#define USB_TX_BASE_REG         (USB_BASE + 0x0C)
#define USB_TX_START_REG        (USB_BASE + 0x10)
#define USB_TX_STATUS_REG       (USB_BASE + 0x14)
#define USB_RX_LEN_REG          (USB_BASE + 0x18)
#define USB_RX_BASE_REG         (USB_BASE + 0x1C)
#define USB_RX_START_REG        (USB_BASE + 0x20)
#define USB_RX_STATUS_REG       (USB_BASE + 0x24)
#define USB_EP0_DATA_REG        (USB_BASE + 0x28)
#define USB_EP0_CTRL_REG        (USB_BASE + 0x2C)

// USB_CTRL bit definitions
#define USB_CTRL_ENABLE         (1 << 0)
#define USB_CTRL_FT_RESET       (1 << 1)
#define USB_CTRL_WAKEUP         (1 << 2)
#define USB_CTRL_DIR_HOST_TO_DEV (0 << 8)
#define USB_CTRL_DIR_DEV_TO_HOST (1 << 8)

// USB_STATUS bit definitions
#define USB_STATUS_CONFIGURED   (1 << 0)
#define USB_STATUS_SUSPENDED    (1 << 1)
#define USB_STATUS_TXE          (1 << 2)  // TX FIFO empty
#define USB_STATUS_RXF          (1 << 3)  // RX FIFO full
#define USB_STATUS_USB3_ACTIVE  (1 << 4)
#define USB_STATUS_USB2_FALLBACK (1 << 5)

// USB_TX_STATUS / USB_RX_STATUS
#define USB_DMA_BUSY            (1 << 0)
#define USB_DMA_DONE            (1 << 1)
#define USB_DMA_ERROR           (1 << 2)

// USB_EP0_CTRL
#define USB_EP0_SETUP_RCVD      (1 << 0)
#define USB_EP0_IN_COMPLETE     (1 << 1)
#define USB_EP0_OUT_COMPLETE    (1 << 2)

//=============================================================================
// Acquisition Sequencer Registers (ACQ — 0x30010000)
//=============================================================================

#define ACQ_CTRL_REG            (ACQ_BASE + 0x00)
#define ACQ_STATUS_REG          (ACQ_BASE + 0x04)
#define ACQ_CONFIG_REG          (ACQ_BASE + 0x08)
#define ACQ_TDC_BASE_REG        (ACQ_BASE + 0x0C)
#define ACQ_ADC_BASE_REG        (ACQ_BASE + 0x10)
#define ACQ_RESULT_REG          (ACQ_BASE + 0x14)

// ACQ_CTRL bit definitions
#define ACQ_CTRL_START          (1 << 0)
#define ACQ_CTRL_ABORT          (1 << 1)
#define ACQ_CTRL_CONTINUOUS     (1 << 2)
#define ACQ_CTRL_REPETITIONS_MASK (0xFF << 8)

// ACQ_STATUS bit definitions
#define ACQ_STATUS_RUNNING      (1 << 0)
#define ACQ_STATUS_COMPLETE     (1 << 1)
#define ACQ_STATUS_ERROR        (1 << 2)
#define ACQ_STATUS_REP_COUNT_MASK (0xFF << 8)

// ACQ_CONFIG bit definitions
#define ACQ_CONFIG_ENABLE_TDC   (1 << 0)
#define ACQ_CONFIG_ENABLE_ADC   (1 << 1)
#define ACQ_CONFIG_ENABLE_BOTH  (1 << 2)
#define ACQ_CONFIG_AUTO_UPLOAD  (1 << 3)
#define ACQ_CONFIG_PRE_TRIG_MASK (0xFF << 8)
#define ACQ_CONFIG_POST_TRIG_MASK (0xFF << 16)

// ACQ_RESULT bit definitions
#define ACQ_RESULT_VALID_REFL   (1 << 0)
#define ACQ_RESULT_OPEN         (1 << 1)
#define ACQ_RESULT_SHORT        (1 << 2)
#define ACQ_RESULT_MULTI_REFL   (1 << 3)
#define ACQ_RESULT_POLARITY_MASK (0xFF << 8)
#define ACQ_RESULT_AMPLITUDE_MASK (0xFFFF << 16)

//=============================================================================
// Interrupt Vector Table
//=============================================================================

#define IRQ_SYS_TIMER           0
#define IRQ_UART_RX             1
#define IRQ_UART_TX             2
#define IRQ_I2C                 3
#define IRQ_SPI_FLASH           4
#define IRQ_SPI_CFG             5
#define IRQ_GPIO                6
#define IRQ_TDC_FIFO            7
#define IRQ_TDC_CAL_DONE        8
#define IRQ_ADC_DMA_DONE        9
#define IRQ_ADC_FIFO            10
#define IRQ_USB_TX_DONE         11
#define IRQ_USB_RX_DONE         12
#define IRQ_USB_EP0             13
#define IRQ_ACQ_COMPLETE        14
#define IRQ_PULSE_DONE          15
#define IRQ_TEMP_ALERT          16
#define IRQ_WATCHDOG            17
#define IRQ_COUNT               18

//=============================================================================
// Helper Macros for Register Access
//=============================================================================

// Volatile register read/write
#define REG_READ(addr)          (*(volatile uint32_t *)(addr))
#define REG_WRITE(addr, val)    (*(volatile uint32_t *)(addr) = (val))

// Bit manipulation
#define REG_SET_BITS(addr, mask)    REG_WRITE(addr, REG_READ(addr) | (mask))
#define REG_CLR_BITS(addr, mask)    REG_WRITE(addr, REG_READ(addr) & ~(mask))
#define REG_TGL_BITS(addr, mask)    REG_WRITE(addr, REG_READ(addr) ^ (mask))

// Bit test
#define REG_TEST_BIT(addr, bit)     (REG_READ(addr) & (bit))

// Wait for bit with timeout
#define REG_WAIT_SET(addr, bit, timeout) \
    do { \
        uint32_t _t = (timeout); \
        while (_t > 0 && !(REG_READ(addr) & (bit))) { _t--; } \
    } while(0)

#define REG_WAIT_CLR(addr, bit, timeout) \
    do { \
        uint32_t _t = (timeout); \
        while (_t > 0 && (REG_READ(addr) & (bit))) { _t--; } \
    } while(0)

//=============================================================================
// Memory Barrier Macros (RISC-V specific)
//=============================================================================

#define FENCE()                 __asm__ volatile ("fence" ::: "memory")
#define FENCE_I()               __asm__ volatile ("fence.i" ::: "memory")
#define NOP()                   __asm__ volatile ("nop")
#define WFI()                   __asm__ volatile ("wfi")

//=============================================================================
// CSR Access Macros (VexRiscv custom CSRs)
//=============================================================================

#define CSR_MCYCLE              0xB00
#define CSR_MCYCLEH             0xB80
#define CSR_MTIME               0x701
#define CSR_MTIMEH              0x741
#define CSR_MTIMECMP            0x780
#define CSR_MTIMECMPH           0x781
#define CSR_MSTATUS             0x300
#define CSR_MIE                 0x304
#define CSR_MTVEC               0x305
#define CSR_MEPC                0x341
#define CSR_MCAUSE              0x342

#define CSR_READ(csr)           ({ uint32_t _v; __asm__ volatile ("csrr %0, " #csr : "=r"(_v)); _v; })
#define CSR_WRITE(csr, val)     __asm__ volatile ("csrw " #csr ", %0" :: "r"(val))
#define CSR_SET(csr, val)       __asm__ volatile ("csrs " #csr ", %0" :: "r"(val))
#define CSR_CLR(csr, val)       __asm__ volatile ("csrc " #csr ", %0" :: "r"(val))

// MSTATUS bit definitions
#define MSTATUS_MIE             (1 << 3)   // Machine interrupt enable

// MIE bit definitions
#define MIE_MTIE                (1 << 7)   // Machine timer interrupt enable
#define MIE_MEIE                (1 << 11)  // Machine external interrupt enable

// MCAUSE values
#define MCAUSE_MTIMER           7          // Machine timer interrupt

#endif // REGISTERS_H
