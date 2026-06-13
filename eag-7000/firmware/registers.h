/*
 * EAG-7000 — MMIO Register Definitions
 *
 * Hardware register base addresses and bit masks for the NXP i.MX8M Plus SoC.
 * These addresses are for the Cortex-M4F peripheral view (same as A72 for
 * shared peripherals, but M4 has its own bus matrix access path).
 *
 * Based on i.MX8M Plus Reference Manual (IMX8MPRM) and Phase 4 definitions.
 *
 * Copyright (c) 2026 EAG-7000 Project
 * SPDX-License-Identifier: MIT
 */

#ifndef EAG7000_REGISTERS_H
#define EAG7000_REGISTERS_H

#include <stdint.h>

/* ============================================================================
 * MMIO Access Helpers
 * ============================================================================ */

static inline void mmio_write32(uintptr_t addr, uint32_t val)
{
    *(volatile uint32_t *)addr = val;
    __asm__ volatile("dmb sy" ::: "memory");
}

static inline uint32_t mmio_read32(uintptr_t addr)
{
    uint32_t val = *(volatile uint32_t *)addr;
    __asm__ volatile("dmb sy" ::: "memory");
    return val;
}

static inline void mmio_set32(uintptr_t addr, uint32_t mask)
{
    mmio_write32(addr, mmio_read32(addr) | mask);
}

static inline void mmio_clr32(uintptr_t addr, uint32_t mask)
{
    mmio_write32(addr, mmio_read32(addr) & ~mask);
}

static inline void mmio_xor32(uintptr_t addr, uint32_t mask)
{
    mmio_write32(addr, mmio_read32(addr) ^ mask);
}

/* ============================================================================
 * Base Addresses — i.MX8M Plus (M4F peripheral view)
 * ============================================================================ */

/* GPIO Registers (5 ports) */
#define GPIO1_BASE               0x30200000UL
#define GPIO2_BASE               0x30210000UL
#define GPIO3_BASE               0x30220000UL
#define GPIO4_BASE               0x30230000UL
#define GPIO5_BASE               0x30240000UL
#define GPIO_SIZE                0x00001000UL

/* GPIO Register Offsets */
#define GPIO_DR                  0x0000   /* Data Register */
#define GPIO_GDIR                0x0004   /* Direction Register */
#define GPIO_PSR                 0x0008   /* Pad Status Register */
#define GPIO_ICR1                0x000C   /* Interrupt Config Register 1 */
#define GPIO_ICR2                0x0010   /* Interrupt Config Register 2 */
#define GPIO_IMR                 0x0014   /* Interrupt Mask Register */
#define GPIO_ISR                 0x0018   /* Interrupt Status Register */
#define GPIO_EDGE_SEL            0x001C   /* Edge Select Register */

/* IOMUXC (I/O Multiplexer) */
#define IOMUXC_BASE             0x30330000UL
#define IOMUXC_SIZE             0x00010000UL
#define IOMUXC_GPR_BASE        0x30340000UL

/* IOMUXC MUX modes */
#define IOMUX_ALT0              0x0
#define IOMUX_ALT1              0x1
#define IOMUX_ALT2              0x2
#define IOMUX_ALT3              0x3   /* GPIO mode */
#define IOMUX_ALT4              0x4
#define IOMUX_ALT5              0x5

/* PAD_CTRL register bit fields */
#define PAD_CTRL_SRE            (1U << 0)      /* Slew Rate: 0=slow, 1=fast */
#define PAD_CTRL_DSE_SHIFT      3
#define PAD_CTRL_DSE_MASK       (0x7U << 3)
#define PAD_CTRL_DSE_260OHM     (0x1U << 3)   /* 2.6 mA @ 3.3V */
#define PAD_CTRL_DSE_520OHM     (0x2U << 3)   /* 5.2 mA @ 3.3V */
#define PAD_CTRL_DSE_1KOHM      (0x3U << 3)   /* 10.4 mA @ 3.3V */
#define PAD_CTRL_DSE_400OHM     (0x4U << 3)   /* 20.8 mA @ 3.3V */
#define PAD_CTRL_DSE_200OHM     (0x6U << 3)   /* 41.6 mA @ 3.3V */
#define PAD_CTRL_SPEED_SHIFT    6
#define PAD_CTRL_SPEED_MASK     (0x3U << 6)
#define PAD_CTRL_ODE            (1U << 11)     /* Open Drain Enable */
#define PAD_CTRL_PUE           (1U << 12)     /* Pull Up Enable */
#define PAD_CTRL_PUS_SHIFT      14
#define PAD_CTRL_PUS_MASK       (0x3U << 14)
#define PAD_CTRL_PUS_100K_PU    (0x3U << 14)
#define PAD_CTRL_PUS_47K_PU     (0x2U << 14)
#define PAD_CTRL_PUS_100K_PD    (0x0U << 14)
#define PAD_CTRL_HYS           (1U << 16)     /* Hysteresis Enable */

/* CCM (Clock Control Module) */
#define CCM_BASE                0x30380000UL
#define CCM_ANAL_BASE           0x30360000UL

/* CCM_ANAL PLL registers */
#define CCM_ANAL_ARM_PLL_CFG0       0x0000
#define CCM_ANAL_ARM_PLL_CFG1       0x0004
#define CCM_ANAL_ARM_PLL_CFG2       0x0008
#define CCM_ANAL_ARM_PLL_STATUS      0x000C
#define CCM_ANAL_DRAM_PLL_CFG0      0x0050
#define CCM_ANAL_DRAM_PLL_STATUS    0x005C

/* PLL configuration bit masks */
#define PLL_CFG0_ENABLE_MASK        (1U << 0)
#define PLL_CFG0_BYPASS_MASK        (1U << 4)
#define PLL_CFG0_POWERDOWN_MASK     (1U << 12)
#define PLL_CFG0_DIV_SEL_MASK       (0x7FU << 0)
#define PLL_CFG0_DIV_SEL_SHIFT      0
#define PLL_STATUS_LOCK_MASK        (1U << 0)

/* CCM clock root registers */
#define CCM_ROOT_ENET1_CLK          0x4A80
#define CCM_ROOT_ENET2_CLK          0x4AA0
#define CCM_ROOT_USB_CLK             0x4980
#define CCM_ROOT_PCIE_CLK            0x4A00
#define CCM_ROOT_SPI1_CLK            0x4780
#define CCM_ROOT_SPI2_CLK            0x47A0
#define CCM_ROOT_SPI3_CLK            0x47C0
#define CCM_ROOT_UART1_CLK           0x4A80
#define CCM_ROOT_CAN_CLK             0x4C00

#define CCM_ROOT_TARGET_ENABLE_MASK  (1U << 0)
#define CCM_ROOT_TARGET_MUX_SHIFT     8
#define CCM_ROOT_TARGET_MUX_MASK      (0x7U << 8)
#define CCM_ROOT_TARGET_DIV_SHIFT     0
#define CCM_ROOT_TARGET_DIV_MASK      (0xFFU << 0)

/* Clock parent select values */
#define CCM_PARENT_OSC_24M           0
#define CCM_PARENT_SYS_PLL1_800M     1
#define CCM_PARENT_SYS_PLL2_500M     2
#define CCM_PARENT_SYS_PLL3_1200M     3

/* UART Registers (i.MX8MP UART) */
#define UART1_BASE                0x30860000UL
#define UART2_BASE                0x30890000UL
#define UART3_BASE                0x30880000UL
#define UART_SIZE                 0x00001000UL

/* UART Register Offsets */
#define UART_URXD                 0x0000   /* UART Receive Register */
#define UART_UTXD                 0x0040   /* UART Transmit Register */
#define UART_UCR1                 0x0080   /* UART Control Register 1 */
#define UART_UCR2                 0x0084   /* UART Control Register 2 */
#define UART_UCR3                 0x0088   /* UART Control Register 3 */
#define UART_UCR4                 0x008C   /* UART Control Register 4 */
#define UART_UFCR                 0x0090   /* UART FIFO Control Register */
#define UART_USR1                 0x0094   /* UART Status Register 1 */
#define UART_USR2                 0x0098   /* UART Status Register 2 */
#define UART_UESC                 0x009C   /* UART Escape Character */
#define UART_UTIM                 0x00A0   /* UART Escape Timer */
#define UART_UBIR                 0x00A4   /* UART BRM Incremental */
#define UART_UBMR                 0x00A8   /* UART BRM Modulator */
#define UART_UBRC                 0x00AC   /* UART BRM Clock */
#define UART_ONEMS               0x00B0   /* UART One Millisecond */
#define UART_UTS                  0x00B4   /* UART Test Register */

/* UART Control Register bits */
#define UART_UCR1_UARTEN          (1U << 0)   /* UART Enable */
#define UART_UCR1_RRDYEN         (1U << 9)   /* RX Ready Interrupt Enable */
#define UART_UCR1_TXMPTYEN       (1U << 6)   /* TX Empty Interrupt Enable */
#define UART_UCR2_SRST           (1U << 0)   /* Soft Reset */
#define UART_UCR2_RXEN           (1U << 1)   /* RX Enable */
#define UART_UCR2_TXEN           (1U << 2)   /* TX Enable */
#define UART_UCR2_IRTS           (1U << 14)  /* Ignore RTS */
#define UART_UCR2_WS             (1U << 5)   /* Word Size (0=7bit, 1=8bit) */
#define UART_UCR2_STPB           (1U << 6)   /* Stop Bits (0=1, 1=2) */
#define UART_UCR2_PREN           (1U << 8)   /* Parity Enable */

/* UART Status Register bits */
#define UART_USR2_RDR            (1U << 0)   /* RX Data Ready */
#define UART_USR2_TXDC          (1U << 3)   /* TX Complete */
#define UART_USR2_TDRE          (1U << 1)   /* TX Data Register Empty */

/* SPI Registers (ECSPI) */
#define ECSPI1_BASE              0x30820000UL
#define ECSPI2_BASE              0x30830000UL
#define ECSPI3_BASE              0x30840000UL
#define ECSPI_SIZE               0x00001000UL

/* ECSPI Register Offsets */
#define ECSPI_RXDATA             0x0000   /* RX Data Register */
#define ECSPI_TXDATA             0x0004   /* TX Data Register */
#define ECSPI_CONREG             0x0008   /* Control Register */
#define ECSPI_CONFIGREG          0x000C   /* Configuration Register */
#define ECSPI_INTREG             0x0010   /* Interrupt Register */
#define ECSPI_DMAREG             0x0014   /* DMA Register */
#define ECSPI_STATREG            0x0018   /* Status Register */
#define ECSPI_PERIODREG          0x001C   /* Period Register */
#define ECSPI_TESTREG            0x0020   /* Test Register */
#define ECSPI_MSGDATA            0x0040   /* Message Data */

/* ECSPI Control Register bits */
#define ECSPI_CONREG_EN          (1U << 0)    /* SPI Enable */
#define ECSPI_CONREG_HT_LEN_SHIFT 8          /* Hardware Trigger Length */
#define ECSPI_CONREG_BURST_LEN_SHIFT 20       /* Burst Length */
#define ECSPI_CONREG_SMC         (1U << 3)    /* Start Mode Control */
#define ECSPI_CONREG_XCH         (1U << 2)    /* Exchange */

/* ECSPI Configuration Register bits */
#define ECSPI_CONFIGREG_SSPOL_SHIFT 12        /* SS Polarity per channel */
#define ECSPI_CONFIGREG_SCLKPHA_SHIFT 0       /* Clock Phase per channel */
#define ECSPI_CONFIGREG_SCLKPOL_SHIFT 4        /* Clock Polarity per channel */

/* ECSPI Status Register bits */
#define ECSPI_STATREG_TE         (1U << 0)    /* TX Empty */
#define ECSPI_STATREG_RR         (1U << 3)    /* RX Ready */
#define ECSPI_STATREG_TC         (1U << 7)    /* Transfer Complete */

/* I2C Registers */
#define I2C1_BASE                0x30A20000UL
#define I2C2_BASE                0x30A30000UL
#define I2C3_BASE                0x30A40000UL
#define I2C_SIZE                 0x00001000UL

/* I2C Register Offsets */
#define I2C_IADR                 0x0000   /* I2C Address Register */
#define I2C_IFDR                 0x0004   /* I2C Frequency Divider */
#define I2C_I2CR                 0x0008   /* I2C Control Register */
#define I2C_I2SR                 0x000C   /* I2C Status Register */
#define I2C_I2DR                 0x0010   /* I2C Data Register */

/* I2C Control Register bits */
#define I2C_I2CR_EN              (1U << 7)   /* I2C Enable */
#define I2C_I2CR_IIEN            (1U << 6)   /* Interrupt Enable */
#define I2C_I2CR_MSTA            (1U << 5)   /* Master/Slave Mode */
#define I2C_I2CR_MTX             (1U << 4)   /* Transmit/Receive Mode */
#define I2C_I2CR_TXAK            (1U << 3)   /* Transmit ACK */
#define I2C_I2CR_RSTA            (1U << 2)   /* Repeated Start */

/* I2C Status Register bits */
#define I2C_I2SR_ICF             (1U << 7)   /* Transfer Complete */
#define I2C_I2SR_IBB             (1U << 5)   /* Bus Busy */
#define I2C_I2SR_IAL             (1U << 4)   /* Arbitration Lost */
#define I2C_I2SR_SRW             (1U << 2)   /* Slave Read/Write */
#define I2C_I2SR_IIF             (1U << 1)   /* Interrupt Flag */
#define I2C_I2SR_RXAK            (1U << 0)   /* RX ACK */

/* MU (Message Unit) — A72 ↔ M4F mailbox */
#define MU1_BASE                 0x30AA0000UL
#define MU_SIZE                  0x00001000UL

/* MU Register Offsets */
#define MU_TR0                   0x0000   /* TX Register 0 */
#define MU_TR1                   0x0004   /* TX Register 1 */
#define MU_TR2                   0x0008   /* TX Register 2 */
#define MU_TR3                   0x000C   /* TX Register 3 */
#define MU_RR0                   0x0100   /* RX Register 0 */
#define MU_RR1                   0x0104   /* RX Register 1 */
#define MU_RR2                   0x0108   /* RX Register 2 */
#define MU_RR3                   0x010C   /* RX Register 3 */
#define MU_SR                    0x0200   /* Status Register */
#define MU_CR                    0x0204   /* Control Register */

/* MU Status Register bits */
#define MU_SR_TEn_SHIFT          20       /* TX Empty flags (TR0-TR3) */
#define MU_SR_RFn_SHIFT          24       /* RX Full flags (RR0-RR3) */
#define MU_SR_GIPn_SHIFT         28       /* GI Pending flags */

/* MU Control Register bits */
#define MU_CR_GIEn_SHIFT         28       /* GI Interrupt Enable */
#define MU_CR_RIEn_SHIFT         24       /* RX Interrupt Enable */
#define MU_CR_TIEn_SHIFT         20       /* TX Interrupt Enable */
#define MU_CR_Fn_SHIFT           16       /* TX Doorbell Enable */

/* MCP2518FD Register Map (SPI-addressed, 12-bit address space) */
#define MCP2518FD_CON            0x0000   /* CAN Control */
#define MCP2518FD_NBTCFG         0x0004   /* Nominal Bit-Time Config */
#define MCP2518FD_DBTCFG         0x0008   /* Data Bit-Time Config */
#define MCP2518FD_TDC            0x000C   /* Transmitter Delay Comp */
#define MCP2518FD_TBC            0x0014   /* Timer-Based Counter */
#define MCP2518FD_TSR            0x001C   /* TX Status */
#define MCP2518FD_RXIF           0x0020   /* RX Interrupt Flags */
#define MCP2518FD_TXIF           0x0024   /* TX Interrupt Flags */
#define MCP2518FD_RXOVIF         0x0028   /* RX Overflow Flags */
#define MCP2518FD_CERRIF         0x002C   /* CAN Error Flags */
#define MCP2518FD_FIFOCON(n)     (0x0050 + ((n) * 0x10))
#define MCP2518FD_FIFOSTA(n)     (0x0054 + ((n) * 0x10))
#define MCP2518FD_FIFOUA(n)      (0x0058 + ((n) * 0x10))
#define MCP2518FD_TXQ            0x0400   /* TX Queue RAM */
#define MCP2518FD_RXFIFO0        0x0600   /* RX FIFO 0 RAM */
#define MCP2518FD_RXFIFO1        0x0800   /* RX FIFO 1 RAM */
#define MCP2518FD_CRC            0x0E00   /* CRC Register */
#define MCP2518FD_ECCCON         0x0E04   /* ECC Control */

/* MCP2518FD SPI Command Bytes */
#define MCP2518FD_SPI_CMD_RESET   0x00
#define MCP2518FD_SPI_CMD_WRITE   0x20
#define MCP2518FD_SPI_CMD_READ    0x30

/* MCP2518FD CAN Control Register Bits */
#define MCP2518FD_CON_REQOP_CONFIG  (0x4U << 24)
#define MCP2518FD_CON_REQOP_NORMAL  (0x0U << 24)
#define MCP2518FD_CON_TXQEN         (1U << 24)
#define MCP2518FD_CON_ESIGM         (1U << 22)

/* GPT (General Purpose Timer) Registers */
#define GPT1_BASE                0x302D0000UL
#define GPT2_BASE                0x302E0000UL
#define GPT_SIZE                 0x00001000UL

#define GPT_CR                   0x0000   /* Control Register */
#define GPT_PR                   0x0004   /* Prescaler Register */
#define GPT_SR                   0x0008   /* Status Register */
#define GPT_OCR1                 0x000C   /* Output Compare Register 1 */
#define GPT_OCR2                 0x0010   /* Output Compare Register 2 */
#define GPT_OCR3                 0x0014   /* Output Compare Register 3 */
#define GPT_ICR1                 0x0018   /* Input Capture Register 1 */
#define GPT_ICR2                 0x001C   /* Input Capture Register 2 */
#define GPT_CNT                  0x0024   /* Counter Register */

/* GPT Control Register bits */
#define GPT_CR_EN                (1U << 0)   /* Timer Enable */
#define GPT_CR_ENMOD             (1U << 1)   /* Enable Mode (reset on enable) */
#define GPT_CR_DBGEN             (1U << 2)   /* Debug Enable */
#define GPT_CR_SWR               (1U << 15)  /* Software Reset */

/* Watchdog Timer Registers */
#define WDOG1_BASE               0x30280000UL
#define WDOG2_BASE               0x30290000UL
#define WDOG_SIZE                0x00001000UL

#define WDOG_WCR                 0x0000   /* Watchdog Control Register */
#define WDOG_WSR                 0x0002   /* Watchdog Service Register */
#define WDOG_WRSR                0x0004   /* Watchdog Reset Status */

#define WDOG_WCR_WDE             (1U << 2)   /* Watchdog Enable */
#define WDOG_WCR_WDT             (1U << 3)   /* Watchdog Timeout */
#define WDOG_WCR_SRE             (1U << 4)   /* Software Reset Enable */

/* SRC (System Reset Controller) */
#define SRC_BASE                 0x30390000UL
#define SRC_SIZE                 0x00001000UL

#define SRC_SRCR1                0x0000   /* System Reset Register 1 */
#define SRC_SRCR2                0x0004   /* System Reset Register 2 */
#define SRC_SRCR3                0x0008   /* System Reset Register 3 */

/* CAAM (Security Block) */
#define CAAM_BASE                0x30900000UL
#define CAAM_SIZE                0x00100000UL

#endif /* EAG7000_REGISTERS_H */