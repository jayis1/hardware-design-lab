# PicoRadar — Phase 4: Software Stack

## 1. Boot Strategy

### 1.1 Power-On Sequence

```
t=0 ms:   VBUS 5V applied → TPS65263 EN asserted
t=2 ms:   BUCK1 (3.3V) ramps up → PGOOD asserts
t=4 ms:   BUCK2 (1.8V) ramps up
t=6 ms:   BUCK3 (1.2V) ramps up → STM32H7 power stable
t=8 ms:   LDO1 (1.0V) + LDO2 (3.0V) → IWR6843 RF powered
t=10 ms:  STM32H7 internal POR releases → CPU starts from QSPI flash
t=12 ms:  STM32H7 GPIO holds IWR6843 NRST low (additional 100 ms settle)
t=112 ms: STM32H7 releases IWR6843 NRST → IWR6843 boots from W25Q128
t=500 ms: STM32H7 FreeRTOS scheduler starts
t=700 ms: ESP32-C3 released from reset → AT firmware boots
t=1500 ms: IWR6843 mmWave SDK ready → HOST_INTR asserts
t=2000 ms: STM32H7 sends "sensorStart" → radar active
```

### 1.2 STM32H743 Boot Configuration

| Boot Mode | BOOT1 | BOOT0 | BOOT_SRC | Action |
|---|---|---|---|---|
| QSPI flash | 1 | 0 | — | Map MX25L25645G at 0x90000000, execute from XIP |
| System memory | 0 | 1 | — | Built-in bootloader (UART/USB for FW update) |
| SRAM | 0 | 0 | — | Debug only |

**Hardware config:** BOOT0 pin pulled low (10 kΩ to GND), BOOT1 pin pulled high (10 kΩ to 3.3 V) → Boot from QSPI.

### 1.3 IWR6843 Boot

- SOP0 = HIGH, SOP1 = LOW, SOP2 = LOW → **Functional mode** (loads firmware from W25Q128)
- For debug/flash: SOP0 = LOW, SOP1 = HIGH → **Flashing mode** (UART XMODEM)

## 2. MMIO Register Definitions

### 2.1 STM32H743 Key Register Map (Base Addresses)

```c
// Reset and Clock Control
#define RCC_BASE            0x58024400UL
#define RCC_CR              (*(volatile uint32_t *)(RCC_BASE + 0x00))  // Clock control
#define RCC_PLLCKSELR       (*(volatile uint32_t *)(RCC_BASE + 0x28))  // PLL clock select
#define RCC_PLL1DIVR        (*(volatile uint32_t *)(RCC_BASE + 0x30))  // PLL1 dividers
#define RCC_CFGR            (*(volatile uint32_t *)(RCC_BASE + 0x10))  // Config register
#define RCC_AHB4ENR         (*(volatile uint32_t *)(RCC_BASE + 0x0E0)) // AHB4 periph enable
#define RCC_APB1LENR        (*(volatile uint32_t *)(RCC_BASE + 0x0E8)) // APB1L periph enable
#define RCC_APB4ENR         (*(volatile uint32_t *)(RCC_BASE + 0x0F4)) // APB4 periph enable

// GPIO registers
#define GPIOA_BASE          0x58020000UL
#define GPIOB_BASE          0x58020400UL
#define GPIOC_BASE          0x58020800UL
#define GPIOD_BASE          0x58020C00UL
#define GPIOE_BASE          0x58021000UL

#define GPIO_MODER(gpio)    (*(volatile uint32_t *)(gpio + 0x00))
#define GPIO_OTYPER(gpio)   (*(volatile uint32_t *)(gpio + 0x04))
#define GPIO_OSPEEDR(gpio)  (*(volatile uint32_t *)(gpio + 0x08))
#define GPIO_PUPDR(gpio)    (*(volatile uint32_t *)(gpio + 0x0C))
#define GPIO_IDR(gpio)      (*(volatile uint32_t *)(gpio + 0x10))
#define GPIO_ODR(gpio)      (*(volatile uint32_t *)(gpio + 0x14))
#define GPIO_AFRL(gpio)     (*(volatile uint32_t *)(gpio + 0x20))
#define GPIO_AFRH(gpio)     (*(volatile uint32_t *)(gpio + 0x24))
#define GPIO_BSRR(gpio)     (*(volatile uint32_t *)(gpio + 0x18))

// USART
#define USART1_BASE         0x40011000UL
#define USART2_BASE         0x40004400UL
#define USART_CR1(usart)    (*(volatile uint32_t *)(usart + 0x00))
#define USART_CR2(usart)    (*(volatile uint32_t *)(usart + 0x04))
#define USART_CR3(usart)    (*(volatile uint32_t *)(usart + 0x08))
#define USART_BRR(usart)    (*(volatile uint32_t *)(usart + 0x0C))
#define USART_ISR(usart)    (*(volatile uint32_t *)(usart + 0x1C))
#define USART_TDR(usart)    (*(volatile uint32_t *)(usart + 0x28))
#define USART_RDR(usart)    (*(volatile uint32_t *)(usart + 0x24))

// SPI
#define SPI1_BASE           0x40013000UL
#define SPI_CR1(spi)        (*(volatile uint32_t *)(spi + 0x00))
#define SPI_CR2(spi)        (*(volatile uint32_t *)(spi + 0x04))
#define SPI_SR(spi)         (*(volatile uint32_t *)(spi + 0x08))
#define SPI_DR(spi)         (*(volatile uint32_t *)(spi + 0x0C))

// I2C
#define I2C1_BASE           0x40005400UL
#define I2C_CR1(i2c)        (*(volatile uint32_t *)(i2c + 0x00))
#define I2C_CR2(i2c)        (*(volatile uint32_t *)(i2c + 0x04))
#define I2C_ISR(i2c)        (*(volatile uint32_t *)(i2c + 0x18))
#define I2C_TXDR(i2c)       (*(volatile uint32_t *)(i2c + 0x28))
#define I2C_RXDR(i2c)       (*(volatile uint32_t *)(i2c + 0x24))
#define I2C_TIMINGR(i2c)    (*(volatile uint32_t *)(i2c + 0x10))

// QUADSPI (OctoSPI in single-quad mode)
#define OCTOSPI_BASE        0x52005000UL
#define OCTOSPI_CR(ospi)    (*(volatile uint32_t *)(ospi + 0x00))
#define OCTOSPI_DCR1(ospi)  (*(volatile uint32_t *)(ospi + 0x04))
#define OCTOSPI_DCR2(ospi)  (*(volatile uint32_t *)(ospi + 0x08))
#define OCTOSPI_CCR(ospi)   (*(volatile uint32_t *)(ospi + 0x0C))
#define OCTOSPI_TCR(ospi)   (*(volatile uint32_t *)(ospi + 0x10))
#define OCTOSPI_SR(ospi)    (*(volatile uint32_t *)(ospi + 0x14))
#define OCTOSPI_IR(ospi)    (*(volatile uint32_t *)(ospi + 0x18))
#define OCTOSPI_AR(ospi)    (*(volatile uint32_t *)(ospi + 0x1C))
#define OCTOSPI_DR(ospi)    (*(volatile uint32_t *)(ospi + 0x20))

// Ethernet MAC
#define ETH_BASE            0x40028000UL
#define ETH_MACCR           (*(volatile uint32_t *)(ETH_BASE + 0x00))
#define ETH_MACFFR          (*(volatile uint32_t *)(ETH_BASE + 0x04))
#define ETH_MACMIIAR        (*(volatile uint32_t *)(ETH_BASE + 0x10))
#define ETH_MACMIIDR        (*(volatile uint32_t *)(ETH_BASE + 0x14))
#define ETH_MACFCR         (*(volatile uint32_t *)(ETH_BASE + 0x18))
#define ETH_DMABMR         (*(volatile uint32_t *)(ETH_BASE + 0x1000))
#define ETH_DMATPDR        (*(volatile uint32_t *)(ETH_BASE + 0x1004))
#define ETH_DMARPDR        (*(volatile uint32_t *)(ETH_BASE + 0x1008))
#define ETH_DMASR          (*(volatile uint32_t *)(ETH_BASE + 0x1014))

// SYSCFG
#define SYSCFG_BASE         0x58000400UL
#define SYSCFG_PMCR         (*(volatile uint32_t *)(SYSCFG_BASE + 0x00))
#define SYSCFG_EXTICR1      (*(volatile uint32_t *)(SYSCFG_BASE + 0x08))

// DMA
#define DMA1_BASE           0x40020000UL
#define DMA2_BASE           0x40020400UL
#define DMA_SPAR(dma,ch)    (*(volatile uint32_t *)(dma + (ch*0x18) + 0x10))
#define DMA_SM0AR(dma,ch)   (*(volatile uint32_t *)(dma + (ch*0x18) + 0x14))
#define DMA_SNDTR(dma,ch)   (*(volatile uint32_t *)(dma + (ch*0x18) + 0x0C))
#define DMA_SCR(dma,ch)     (*(volatile uint32_t *)(dma + (ch*0x18) + 0x00))
```

### 2.2 ICM-42688-P Register Map (via SPI)

```c
#define ICM42688_WHO_AM_I       0x75  // R: 0x47 (device ID)
#define ICM42688_DEVICE_CONFIG  0x11  // R/W: soft reset, sleep mode
#define ICM42688_DRIVE_CONFIG   0x13  // R/W: I2C/SPI drive strength
#define ICM42688_INT_CONFIG     0x14  // R/W: interrupt config
#define ICM42688_PWR_MGMT0      0x4E  // R/W: accel/gyro on/off
#define ICM42688_GYRO_CONFIG0   0x4F  // R/W: gyro ODR, FS
#define ICM42688_ACCEL_CONFIG0  0x50  // R/W: accel ODR, FS
#define ICM42688_GYRO_CONFIG1   0x51  // R/W: gyro BW
#define ICM42688_ACCEL_CONFIG1  0x53  // R/W: accel BW
#define ICM42688_FIFO_CONFIG    0x16  // R/W: FIFO mode, watermark
#define ICM42688_FIFO_COUNTH    0x3A  // R: FIFO count
#define ICM42688_FIFO_DATA      0x3C  // R: FIFO data
#define ICM42688_TEMP_DATA      0x3D  // R: Temperature (2 bytes)
#define ICM42688_ACCEL_XOUT     0x1F  // R: Accel X (2 bytes, big-endian)
#define ICM42688_ACCEL_YOUT     0x21  // R: Accel Y
#define ICM42688_ACCEL_ZOUT     0x23  // R: Accel Z
#define ICM42688_GYRO_XOUT      0x25  // R: Gyro X (2 bytes, big-endian)
#define ICM42688_GYRO_YOUT      0x27  // R: Gyro Y
#define ICM42688_GYRO_ZOUT      0x29  // R: Gyro Z
#define ICM42688_INT_STATUS     0x3A  // R: interrupt status
#define ICM42688_BANK_SEL       0x76  // R/W: register bank select (0-4)
```

### 2.3 SH1106 OLED Register Map (via I2C)

```c
#define SH1106_I2C_ADDR        0x3C
#define SH1106_CTRL_CMD        0x00  // Control byte: command
#define SH1106_CTRL_DATA       0x40  // Control byte: data
#define SH1106_DISPLAY_OFF     0xAE
#define SH1106_DISPLAY_ON      0xAF
#define SH1106_SET_DISP_START  0x40  // + line (0-63)
#define SH1106_SET_PAGE_ADDR   0xB0  // + page (0-7)
#define SH1106_SET_COL_LO      0x00  // + col_lo nibble
#define SH1106_SET_COL_HI      0x10  // + col_hi nibble
#define SH1106_SEG_REMAP       0xA1  // Column addr 127→0
#define SH1106_COM_SCAN_INV    0xC8  // COM scan direction remap
#define SH1106_SET_CONTRAST    0x81  // + contrast byte (0-255)
#define SH1106_SET_MUX_RATIO   0xA8  // + ratio (63)
#define SH1106_SET_DISP_OFFSET 0xD3  // + offset (0)
#define SH1106_SET_PRECHARGE   0xD9  // + precharge period
#define SH1106_SET_VCOMH       0xDB  // + Vcomh level
#define SH1106_SET_CHARGE_PUMP 0x8D  // + 0x14 enable
```

### 2.4 LAN8720A PHY Register Map (via MDIO/SMI)

```c
#define LAN8720_PHY_ADDR       0x00  // PHY address (strap-configured)
#define LAN8720_BMCR           0x00  // Basic mode control
#define LAN8720_BMSR           0x01  // Basic mode status
#define LAN8720_PHYID1         0x02  // PHY ID 1
#define LAN8720_PHYID2         0x03  // PHY ID 2
#define LAN8720_ANAR           0x04  // Auto-negotiation advertisement
#define LAN8720_ANLPAR         0x05  // Auto-negotiation link partner
#define LAN8720_ANER           0x06  // Auto-negotiation expansion
#define LAN8720_MCS            0x11  // Mode control/status (LAN8720-specific)
#define LAN8720_SCS            0x1F  // Special control/status
#define LAN8720_SECR           0x1A  // Symbol error counter
#define LAN8720_MISR           0x1D  // MII interrupt status
#define LAN8720_MIMR           0x1E  // MII interrupt mask
```

## 3. Clock Configuration Code

```c
/*
 * clock_config.c — STM32H743 clock tree setup
 * Target: SYSCLK = 480 MHz, AHB = 240 MHz, APB1 = 120 MHz, APB2 = 120 MHz
 * Source: HSE = 8 MHz → PLL1 → 480 MHz
 */

#include "registers.h"
#include "board.h"

void SystemClock_Config(void)
{
    /* Step 1: Enable HSE and wait for ready */
    RCC_AHB4ENR |= (1 << 12);  // Enable GPIO port H (for HSE oscillator)
    RCC_CR |= (1 << 2);        // HSEON
    while (!(RCC_CR & (1 << 3)));  // Wait for HSERDY

    /* Step 2: Configure PLL1
     * PLL1 source = HSE (8 MHz)
     * PLL1 M = 4 → VCO input = 2 MHz
     * PLL1 N = 240 → VCO output = 480 MHz
     * PLL1 P = 2 → PLL1 output = 240 MHz
     * PLL1 Q = 4 → PLL1Q = 120 MHz (for USB)
     * PLL1 R = 2 → PLL1R = 240 MHz
     *
     * SYSCLK from PLL1 P = 240 MHz → SYSCLK divider /1 = 240 MHz
     * Actually for 480 MHz: N=480, P=2 → 240 MHz PLL1P
     * Use SYSCLK prescaler /1 and CPU divider = 1 → 240 MHz
     * For 480 MHz: set CPU_D1CPRE to /1 and CPUCLK = PLL1P×2 via CKPER
     * Correct approach: PLL1 N=480, P=1 gives 480 MHz, but H7 PLL P min=1
     * Revised: M=1, N=120, P=1 → VCO = 8/1 × 120 = 960 MHz → /1 = 960? No.
     * Let me use the standard H7 approach:
     * HSE=8MHz → M=4 → 2 MHz VCO_in → N=240 → VCO=480 MHz → P=1 → 480 MHz
     * Actually H743 PLL1: VCO = (HSE/M) × N, output = VCO/P
     * 2 × 240 = 480 MHz VCO, /1 = 480 MHz PLL1P
     * VCO range: 128-560 MHz for H743, so 480 MHz is in range.
     */

    // Power config: enable over-drive for 480 MHz
    (*(volatile uint32_t *)0x58024800UL) |= 0x00003000;  // PWR CR1: VOS=3 (overdrive)
    while (!((*(volatile uint32_t *)0x58024814UL) & 0x00001000)); // VOSRDY

    // PLL1 config
    RCC_PLLCKSELR = (4 << 4)     // PLL1 M = 4
                   | (2 << 0);   // PLL1 source = HSE (0b10)
    RCC_PLL1DIVR  = (240 << 0)  // PLL1 N = 240
                   | (1 << 8)    // PLL1 P = 1 (encoded as P-1=0)
                   | (3 << 12)   // PLL1 Q = 4 (encoded as Q-1=3)
                   | (1 << 16);  // PLL1 R = 2 (encoded as R-1=1)

    // Enable PLL1
    RCC_CR |= (1 << 24);        // PLL1ON
    while (!(RCC_CR & (1 << 25))); // PLL1RDY

    /* Step 3: Set flash latency for 480 MHz (VOS3) */
    (*(volatile uint32_t *)0x52002000UL) = (*(volatile uint32_t *)0x52002000UL & ~0xF) | 0x4; // FLASH ACR: 4 WS

    /* Step 4: Switch system clock to PLL1 */
    RCC_CFGR = (RCC_CFGR & ~(0x3 << 0)) | (0x3 << 0); // SW = PLL1
    while ((RCC_CFGR & (0x3 << 3)) != (0x3 << 3));    // SWS = PLL1

    /* Step 5: Configure bus prescalers */
    // D1CPRE = /1 (CPUCLK = 480 MHz)
    // HPRE = /2 (AHB = 240 MHz)
    // D1PPRE = /2 (APB3 = 120 MHz)
    // D2PPRE1 = /2 (APB1L = 120 MHz)
    // D2PPRE2 = /2 (APB2 = 120 MHz)
    // D3PPRE = /2 (APB4 = 120 MHz)
    (*(volatile uint32_t *)(RCC_BASE + 0x018)) =  // RCC_D1CFGR
        (0 << 16)   // D1CPRE = /1
      | (4 << 8)    // HPRE = /2
      | (4 << 4);   // D1PPRE = /2

    (*(volatile uint32_t *)(RCC_BASE + 0x01C)) =  // RCC_D2CFGR
        (4 << 8)    // D2PPRE2 = /2 (APB2)
      | (4 << 0);   // D2PPRE1 = /2 (APB1L)

    (*(volatile uint32_t *)(RCC_BASE + 0x020)) =  // RCC_D3CFGR
        (4 << 0);   // D3PPRE = /2 (APB4)
}
```

## 4. GPIO Init Code

```c
/*
 * gpio_init.c — STM32H743 GPIO configuration for PicoRadar
 */

#include "registers.h"
#include "board.h"

static void gpio_set_af(uint32_t gpio_base, uint8_t pin, uint8_t af)
{
    if (pin < 8) {
        GPIO_AFRL(gpio_base) = (GPIO_AFRL(gpio_base) & ~(0xF << (pin * 4)))
                               | ((af & 0xF) << (pin * 4));
    } else {
        GPIO_AFRH(gpio_base) = (GPIO_AFRH(gpio_base) & ~(0xF << ((pin - 8) * 4)))
                               | ((af & 0xF) << ((pin - 8) * 4));
    }
}

void GPIO_Init(void)
{
    /* Enable all GPIO port clocks (A, B, C, D, E) */
    RCC_AHB4ENR |= (1 << 0)   // GPIOA
                  | (1 << 1)   // GPIOB
                  | (1 << 2)   // GPIOC
                  | (1 << 3)   // GPIOD
                  | (1 << 4);  // GPIOE

    /* --- UART1 (Radar Host Interface) ---
     * PA9  = USART1_TX (AF7)
     * PA10 = USART1_RX (AF7)
     * PA11 = USART1_CTS (AF7)
     * PA12 = USART1_RTS (AF7)  — also used as USB_DP alt, but we use as UART here
     */

    // PA9: AF7 push-pull, high speed
    GPIO_MODER(GPIOA_BASE) = (GPIO_MODER(GPIOA_BASE) & ~(3 << 18)) | (2 << 18); // AF mode
    GPIO_OTYPER(GPIOA_BASE) &= ~(1 << 9);   // Push-pull
    GPIO_OSPEEDR(GPIOA_BASE) |= (3 << 18);  // Very high speed
    GPIO_PUPDR(GPIOA_BASE) = (GPIO_PUPDR(GPIOA_BASE) & ~(3 << 18)); // No pull
    gpio_set_af(GPIOA_BASE, 9, 7);

    // PA10: AF7
    GPIO_MODER(GPIOA_BASE) = (GPIO_MODER(GPIOA_BASE) & ~(3 << 20)) | (2 << 20);
    GPIO_PUPDR(GPIOA_BASE) = (GPIO_PUPDR(GPIOA_BASE) & ~(3 << 20)) | (1 << 20); // Pull-up
    GPIO_OSPEEDR(GPIOA_BASE) |= (3 << 20);
    gpio_set_af(GPIOA_BASE, 10, 7);

    // PA11: AF7 (CTS)
    GPIO_MODER(GPIOA_BASE) = (GPIO_MODER(GPIOA_BASE) & ~(3 << 22)) | (2 << 22);
    gpio_set_af(GPIOA_BASE, 11, 7);

    /* --- UART2 (ESP32-C3) ---
     * PB10 = USART2_TX (AF4)
     * PB11 = USART2_RX (AF4)
     */
    GPIO_MODER(GPIOB_BASE) = (GPIO_MODER(GPIOB_BASE) & ~(3 << 20)) | (2 << 20);
    GPIO_OSPEEDR(GPIOB_BASE) |= (3 << 20);
    gpio_set_af(GPIOB_BASE, 10, 4);

    GPIO_MODER(GPIOB_BASE) = (GPIO_MODER(GPIOB_BASE) & ~(3 << 22)) | (2 << 22);
    GPIO_PUPDR(GPIOB_BASE) = (GPIO_PUPDR(GPIOB_BASE) & ~(3 << 22)) | (1 << 22);
    gpio_set_af(GPIOB_BASE, 11, 4);

    /* --- SPI1 (IMU) ---
     * PA5 = SPI1_SCK (AF5)
     * PA6 = SPI1_MISO (AF5)
     * PA7 = SPI1_MOSI (AF5) — shared with ETH_MDIO; use ETH_MDIO on separate pin
     * Actually PA7 is ETH_RMII_MDIO (AF11). SPI1_MOSI conflicts.
     * Remap: Use PB5 for SPI1_MOSI (AF5). But PB5 is used for PGOOD.
     * Solution: Use SPI1 on PA5/PA6/PB5/PA4
     */
    // PA5: SPI1_SCK AF5
    GPIO_MODER(GPIOA_BASE) = (GPIO_MODER(GPIOA_BASE) & ~(3 << 10)) | (2 << 10);
    GPIO_OSPEEDR(GPIOA_BASE) |= (3 << 10);
    gpio_set_af(GPIOA_BASE, 5, 5);

    // PA6: SPI1_MISO AF5
    GPIO_MODER(GPIOA_BASE) = (GPIO_MODER(GPIOA_BASE) & ~(3 << 12)) | (2 << 12);
    gpio_set_af(GPIOA_BASE, 6, 5);

    // PA4: SPI1_NSS (GPIO output, manual CS)
    GPIO_MODER(GPIOA_BASE) = (GPIO_MODER(GPIOA_BASE) & ~(3 << 8)) | (1 << 8); // Output
    GPIO_OTYPER(GPIOA_BASE) |= (0 << 4);   // Push-pull
    GPIO_OSPEEDR(GPIOA_BASE) |= (3 << 8);
    GPIO_ODR(GPIOA_BASE) |= (1 << 4);      // CSz high (deselected)

    /* --- I2C1 (OLED + PMIC) ---
     * PB6 = I2C1_SCL (AF4)
     * PB7 = I2C1_SDA (AF4)
     */
    GPIO_MODER(GPIOB_BASE) = (GPIO_MODER(GPIOB_BASE) & ~(3 << 12)) | (2 << 12);
    GPIO_OTYPER(GPIOB_BASE) |= (1 << 6);  // Open-drain
    GPIO_OSPEEDR(GPIOB_BASE) |= (3 << 12);
    GPIO_PUPDR(GPIOB_BASE) = (GPIO_PUPDR(GPIOB_BASE) & ~(3 << 12)); // External pull-ups
    gpio_set_af(GPIOB_BASE, 6, 4);

    GPIO_MODER(GPIOB_BASE) = (GPIO_MODER(GPIOB_BASE) & ~(3 << 14)) | (2 << 14);
    GPIO_OTYPER(GPIOB_BASE) |= (1 << 7);
    GPIO_OSPEEDR(GPIOB_BASE) |= (3 << 14);
    gpio_set_af(GPIOB_BASE, 7, 4);

    /* --- Ethernet RMII ---
     * PA1  = ETH_RMII_MDC (AF11) — wait, PA1 = MDC? Actually PA1=RMII_REF_CLK for some,
     *   but we have PA2=REFCLK. PA1 = ETH_RMII_MDC (AF11) per STM32H7 RMII pinout.
     * PA2  = ETH_RMII_REFCLK (AF11)
     * PA7  = ETH_RMII_MDIO (AF11)
     * PC1  = ETH_RMII_MDC (AF11) — alternate
     * PC4  = ETH_RMII_RXD0 (AF11)
     * PC5  = ETH_RMII_RXD1 (AF11)
     * PB0  = ETH_RMII_RXDV / CRS_DV (AF11)
     * PB13 = ETH_RMII_TXD0 (AF11)
     * PB14 = ETH_RMII_TXD1 (AF11)
     * PB11 = ETH_RMII_TXEN (AF11) — conflicts with UART2_RX!
     *
     * Resolve: Move UART2 to PD5/PD6 (USART2 alternate pins AF7)
     *   or use USART3 on PD8/PD9. Actually PB11 is used by both USART2_RX and ETH_TXEN.
     *   Remap USART2: PD5=TX, PD6=RX (AF7)
     *   Then PB11 can be ETH_RMII_TXEN (AF11)
     */

    // Update: USART2 remapped to PD5/PD6
    GPIO_MODER(GPIOD_BASE) = (GPIO_MODER(GPIOD_BASE) & ~(3 << 10)) | (2 << 10);
    GPIO_OSPEEDR(GPIOD_BASE) |= (3 << 10);
    gpio_set_af(GPIOD_BASE, 5, 7); // PD5 = USART2_TX AF7

    GPIO_MODER(GPIOD_BASE) = (GPIO_MODER(GPIOD_BASE) & ~(3 << 12)) | (2 << 12);
    GPIO_PUPDR(GPIOD_BASE) = (GPIO_PUPDR(GPIOD_BASE) & ~(3 << 12)) | (1 << 12);
    gpio_set_af(GPIOD_BASE, 6, 7); // PD6 = USART2_RX AF7

    // PA1: ETH_RMII_MDC (AF11) — but this conflicts with no other periph. Actually
    // on H7, ETH_RMII_MDC is on PA1 or PC1. Let's use PC1 for MDC.
    // PA1 → GPIO for radar NRST (simpler)
    // Use standard H7 RMII pins:
    //   PA2  = RMII_REFCLK (AF11)
    //   PA7  = RMII_MDIO (AF11)
    //   PC1  = RMII_MDC (AF11)
    //   PC4  = RMII_RXD0 (AF11)
    //   PC5  = RMII_RXD1 (AF11)
    //   PB0  = RMII_CRS_DV (AF11) — conflicts? No, PB0 is fine.
    //   PB13 = RMII_TXD0 (AF11)
    //   PB14 = RMII_TXD1 (AF11)
    //   PB11 = RMII_TXEN (AF11)

    // PA2: ETH_RMII_REFCLK
    GPIO_MODER(GPIOA_BASE) = (GPIO_MODER(GPIOA_BASE) & ~(3 << 4)) | (2 << 4);
    gpio_set_af(GPIOA_BASE, 2, 11);

    // PA7: ETH_RMII_MDIO
    GPIO_MODER(GPIOA_BASE) = (GPIO_MODER(GPIOA_BASE) & ~(3 << 14)) | (2 << 14);
    gpio_set_af(GPIOA_BASE, 7, 11);

    // PC1: ETH_RMII_MDC
    GPIO_MODER(GPIOC_BASE) = (GPIO_MODER(GPIOC_BASE) & ~(3 << 2)) | (2 << 2);
    gpio_set_af(GPIOC_BASE, 1, 11);

    // PC4, PC5: RMII_RXD0/1
    GPIO_MODER(GPIOC_BASE) = (GPIO_MODER(GPIOC_BASE) & ~(3 << 8)) | (2 << 8);
    gpio_set_af(GPIOC_BASE, 4, 11);
    GPIO_MODER(GPIOC_BASE) = (GPIO_MODER(GPIOC_BASE) & ~(3 << 10)) | (2 << 10);
    gpio_set_af(GPIOC_BASE, 5, 11);

    // PB0: RMII_CRS_DV
    GPIO_MODER(GPIOB_BASE) = (GPIO_MODER(GPIOB_BASE) & ~(3 << 0)) | (2 << 0);
    gpio_set_af(GPIOB_BASE, 0, 11);

    // PB13, PB14: RMII_TXD0/1
    GPIO_MODER(GPIOB_BASE) = (GPIO_MODER(GPIOB_BASE) & ~(3 << 26)) | (2 << 26);
    GPIO_OSPEEDR(GPIOB_BASE) |= (3 << 26);
    gpio_set_af(GPIOB_BASE, 13, 11);
    GPIO_MODER(GPIOB_BASE) = (GPIO_MODER(GPIOB_BASE) & ~(3 << 28)) | (2 << 28);
    gpio_set_af(GPIOB_BASE, 14, 11);

    // PB11: RMII_TXEN
    GPIO_MODER(GPIOB_BASE) = (GPIO_MODER(GPIOB_BASE) & ~(3 << 22)) | (2 << 22);
    gpio_set_af(GPIOB_BASE, 11, 11);

    /* --- QSPI Flash ---
     * PE2 = QSPI_CLK (AF9)
     * PE3 = QSPI_NCS (AF9)
     * PE4 = QSPI_DQ0 (AF9)
     * PE5 = QSPI_DQ1 (AF9)
     * PE6 = QSPI_DQ2 (AF9)
     * PE7 = QSPI_DQ3 (AF9)
     */
    for (uint8_t pin = 2; pin <= 7; pin++) {
        GPIO_MODER(GPIOE_BASE) = (GPIO_MODER(GPIOE_BASE) & ~(3 << (pin*2))) | (2 << (pin*2));
        GPIO_OSPEEDR(GPIOE_BASE) |= (3 << (pin*2));
        gpio_set_af(GPIOE_BASE, pin, 9);
    }

    /* --- GPIO outputs ---
     * PD0 = RADAR_NRST (output, active-low, default high)
     * PD1 = RADAR_SYNC_IN (output)
     * PD2 = ETH_nRST (output, active-low, default high)
     * PC7 = ESP32_EN (output, active-low, default high)
     * PC8 = ESP32_BOOT (output, default high = normal boot)
     */
    // PD0: Radar reset
    GPIO_MODER(GPIOD_BASE) = (GPIO_MODER(GPIOD_BASE) & ~(3 << 0)) | (1 << 0);
    GPIO_OSPEEDR(GPIOD_BASE) |= (3 << 0);
    GPIO_ODR(GPIOD_BASE) |= (1 << 0);  // High = not in reset

    // PD1: Radar sync in
    GPIO_MODER(GPIOD_BASE) = (GPIO_MODER(GPIOD_BASE) & ~(3 << 2)) | (1 << 2);
    GPIO_ODR(GPIOD_BASE) &= ~(1 << 1); // Low = no sync

    // PD2: ETH PHY reset
    GPIO_MODER(GPIOD_BASE) = (GPIO_MODER(GPIOD_BASE) & ~(3 << 4)) | (1 << 4);
    GPIO_ODR(GPIOD_BASE) |= (1 << 2);  // High = not in reset

    // PC7: ESP32 enable
    GPIO_MODER(GPIOC_BASE) = (GPIO_MODER(GPIOC_BASE) & ~(3 << 14)) | (1 << 14);
    GPIO_ODR(GPIOC_BASE) |= (1 << 7);  // High = enabled

    // PC8: ESP32 boot mode
    GPIO_MODER(GPIOC_BASE) = (GPIO_MODER(GPIOC_BASE) & ~(3 << 16)) | (1 << 16);
    GPIO_ODR(GPIOC_BASE) |= (1 << 8);  // High = normal boot

    /* --- GPIO inputs ---
     * PC6 = RADAR_INTR (input, pull-up)
     * PD2 = RADAR_SYNC_OUT (input) — conflicts with ETH_nRST above!
     * Fix: Move ETH_nRST to PD3, use PD2 for SYNC_OUT input
     */
    // PD3: ETH PHY reset (remapped)
    GPIO_MODER(GPIOD_BASE) = (GPIO_MODER(GPIOD_BASE) & ~(3 << 6)) | (1 << 6);
    GPIO_ODR(GPIOD_BASE) |= (1 << 3);  // High = not in reset

    // PD2: RADAR_SYNC_OUT (input)
    GPIO_MODER(GPIOD_BASE) = (GPIO_MODER(GPIOD_BASE) & ~(3 << 4)); // Input mode
    GPIO_PUPDR(GPIOD_BASE) = (GPIO_PUPDR(GPIOD_BASE) & ~(3 << 4)); // No pull

    // PC6: RADAR_INTR (input, pull-up)
    GPIO_MODER(GPIOC_BASE) = (GPIO_MODER(GPIOC_BASE) & ~(3 << 12)); // Input
    GPIO_PUPDR(GPIOC_BASE) = (GPIO_PUPDR(GPIOC_BASE) & ~(3 << 12)) | (1 << 12); // Pull-up

    // PC9: IMU_INT1 (input, pull-down)
    GPIO_MODER(GPIOC_BASE) = (GPIO_MODER(GPIOC_BASE) & ~(3 << 18)); // Input
    GPIO_PUPDR(GPIOC_BASE) = (GPIO_PUPDR(GPIOC_BASE) & ~(3 << 18)) | (2 << 18); // Pull-down

    // PD3: ETH_nINT (input, pull-up)
    GPIO_MODER(GPIOD_BASE) = (GPIO_MODER(GPIOD_BASE) & ~(3 << 6)); // Input
    GPIO_PUPDR(GPIOD_BASE) = (GPIO_PUPDR(GPIOD_BASE) & ~(3 << 6)) | (1 << 6); // Pull-up

    /* --- SWD Debug ---
     * PA13 = SWDIO (AF0, default after reset)
     * PA14 = SWCLK (AF0, default after reset)
     * No configuration needed — defaults are correct
     */

    /* --- USB FS ---
     * PD8 = USB_DM (AF10)
     * PD9 = USB_DP (AF10)
     */
    GPIO_MODER(GPIOD_BASE) = (GPIO_MODER(GPIOD_BASE) & ~(3 << 16)) | (2 << 16);
    gpio_set_af(GPIOD_BASE, 8, 10);
    GPIO_MODER(GPIOD_BASE) = (GPIO_MODER(GPIOD_BASE) & ~(3 << 18)) | (2 << 18);
    gpio_set_af(GPIOD_BASE, 9, 10);

    /* --- PB5: SPI1_MOSI (AF5) — remapped from PA7 conflict ---
     */
    GPIO_MODER(GPIOB_BASE) = (GPIO_MODER(GPIOB_BASE) & ~(3 << 10)) | (2 << 10);
    GPIO_OSPEEDR(GPIOB_BASE) |= (3 << 10);
    gpio_set_af(GPIOB_BASE, 5, 5);
}
```

## 5. Full Device Driver: ICM-42688-P IMU (SPI1 with DMA)

### 5.1 Driver Header

```c
/*
 * drivers/imu_icm42688.h — ICM-42688-P 6-axis IMU driver for PicoRadar
 * SPI1 interface with DMA on STM32H743
 */

#ifndef IMU_ICM42688_H
#define IMU_ICM42688_H

#include <stdint.h>
#include <stddef.h>

/* IMU configuration */
#define IMU_SPI_TIMEOUT_MS       100
#define IMU_GYRO_RANGE_DPS       2000   // ±2000 °/s
#define IMU_ACCEL_RANGE_G        16     // ±16 g
#define IMU_GYRO_ODR             6      // 0=12.5Hz..6=800Hz (encoded)
#define IMU_ACCEL_ODR            6      // Same
#define IMU_FIFO_WATERMARK       16     // 16 samples

/* Scaling factors */
#define IMU_GYRO_SCALE           (2000.0f / 32768.0f)  // DPS/LSB
#define IMU_ACCEL_SCALE          (16.0f / 32768.0f)    // g/LSB

/* Register bank 0 (default) */
#define ICM42688_WHO_AM_I        0x75
#define ICM42688_DEVICE_CONFIG   0x11
#define ICM42688_PWR_MGMT0       0x4E
#define ICM42688_GYRO_CONFIG0    0x4F
#define ICM42688_ACCEL_CONFIG0   0x50
#define ICM42688_GYRO_CONFIG1    0x51
#define ICM42688_ACCEL_CONFIG1   0x53
#define ICM42688_FIFO_CONFIG     0x16
#define ICM42688_FIFO_COUNTH     0x3A
#define ICM42688_FIFO_DATA       0x3C
#define ICM42688_INT_STATUS      0x3A
#define ICM42688_INT_CONFIG      0x14

/* SPI read/write bit */
#define ICM42688_SPI_READ        0x80
#define ICM42688_SPI_WRITE       0x00

/* Data structures */
typedef struct {
    int16_t accel_x, accel_y, accel_z;  // Raw 16-bit values
    int16_t gyro_x, gyro_y, gyro_z;     // Raw 16-bit values
    int16_t temperature;                 // Raw temperature
    uint32_t timestamp;                  // Sample counter
} imu_sample_t;

typedef struct {
    float accel_x, accel_y, accel_z;    // g
    float gyro_x, gyro_y, gyro_z;       // deg/s
    float temperature;                   // °C
} imu_scaled_t;

/* Public API */
int  imu_init(void);
int  imu_read_raw(imu_sample_t *sample);
void imu_scale(const imu_sample_t *raw, imu_scaled_t *scaled);
int  imu_fifo_read(imu_sample_t *samples, uint16_t max_count, uint16_t *out_count);
int  imu_self_test(void);
void imu_power_down(void);
int  imu_configure_fifo(uint16_t watermark);
int  imu_read_register(uint8_t reg, uint8_t *val);
int  imu_write_register(uint8_t reg, uint8_t val);

/* DMA callback type */
typedef void (*imu_dma_cb_t)(void);
void imu_set_dma_callback(imu_dma_cb_t cb);

#endif /* IMU_ICM42688_H */
```

### 5.2 Driver Implementation

```c
/*
 * drivers/imu_icm42688.c — ICM-42688-P 6-axis IMU driver for PicoRadar
 *
 * Uses SPI1 on STM32H743 with DMA for FIFO burst reads.
 * Manual NSS (PA4) — CS is managed in software.
 */

#include "drivers/imu_icm42688.h"
#include "registers.h"
#include "board.h"
#include <string.h>

/* DMA1 Stream 0 for SPI1_RX, Channel 3 (per STM32H7 DMAMUX) */
#define DMA_SPI1_RX_STREAM   0
#define DMA_SPI1_RX_CHANNEL  3
#define DMA_SPI1_RX_BASE     (DMA1_BASE + 0x10 + DMA_SPI1_RX_STREAM * 0x18)

/* DMA1 Stream 1 for SPI1_TX, Channel 3 */
#define DMA_SPI1_TX_STREAM   1
#define DMA_SPI1_TX_CHANNEL 3
#define DMA_SPI1_TX_BASE     (DMA1_BASE + 0x10 + DMA_SPI1_TX_STREAM * 0x18)

/* RX/TX buffers (aligned for DMA) */
static __attribute__((aligned(32))) uint8_t spi1_tx_buf[32];
static __attribute__((aligned(32))) uint8_t spi1_rx_buf[32];

static volatile int dma_rx_complete = 0;
static imu_dma_cb_t user_dma_cb = NULL;

/* ---- Low-level SPI helpers ---- */

static void spi1_cs_low(void)
{
    GPIO_ODR(GPIOA_BASE) &= ~(1 << 4);  // PA4 low
}

static void spi1_cs_high(void)
{
    GPIO_ODR(GPIOA_BASE) |= (1 << 4);   // PA4 high
}

static void spi1_wait_busy(void)
{
    uint32_t timeout = 100000;
    while ((SPI_SR(SPI1_BASE) & (1 << 7)) && --timeout); // BSY bit
}

static void spi1_write_byte(uint8_t data)
{
    while (!(SPI_SR(SPI1_BASE) & (1 << 1)));  // TXP (TX ready)
    *((volatile uint8_t *)&SPI_DR(SPI1_BASE)) = data;
    spi1_wait_busy();
}

static uint8_t spi1_read_byte(void)
{
    while (!(SPI_SR(SPI1_BASE) & (1 << 0)));  // RXP (RX available)
    return *((volatile uint8_t *)&SPI_DR(SPI1_BASE));
}

/* ---- Register access ---- */

int imu_read_register(uint8_t reg, uint8_t *val)
{
    spi1_cs_low();
    spi1_write_byte(reg | ICM42688_SPI_READ);
    spi1_write_byte(0x00);  // Dummy for clock
    *val = spi1_read_byte();
    spi1_cs_high();
    return 0;
}

int imu_write_register(uint8_t reg, uint8_t val)
{
    spi1_cs_low();
    spi1_write_byte(reg & ~ICM42688_SPI_READ);
    spi1_write_byte(val);
    spi1_cs_high();
    return 0;
}

/* ---- DMA configuration for FIFO burst read ---- */

static void spi1_dma_config_rx(uint8_t *dst, uint16_t len)
{
    /* DMA1 Stream 0, Channel 3 (SPI1_RX) */
    volatile uint32_t *scr  = (volatile uint32_t *)(DMA_SPI1_RX_BASE + 0x00);
    volatile uint32_t *sndtr = (volatile uint32_t *)(DMA_SPI1_RX_BASE + 0x0C);
    volatile uint32_t *spar = (volatile uint32_t *)(DMA_SPI1_RX_BASE + 0x10);
    volatile uint32_t *sm0ar = (volatile uint32_t *)(DMA_SPI1_RX_BASE + 0x14);

    *scr = 0;  // Disable stream
    while (*scr & 1);  // Wait for disabled

    *spar = (uint32_t)&SPI_DR(SPI1_BASE);  // Source = SPI1 data register
    *sm0ar = (uint32_t)dst;                 // Destination = buffer
    *sndtr = len;                           // Number of data items

    // EN=1, TCIE=1 (transfer complete interrupt), DIR=0 (periph→mem)
    // MINC=1 (memory increment), PSIZE=8-bit, MSIZE=8-bit, PL=high
    *scr = (1 << 0)    // EN
         | (1 << 1)    // TCIE (transfer complete interrupt enable)
         | (0 << 6)    // DIR: periph-to-memory
         | (0 << 8)    // CIRC: not circular
         | (1 << 10)   // MINC: memory increment
         | (0 << 11)   // PSIZE: 8-bit
         | (0 << 13)   // MSIZE: 8-bit
         | (2 << 16)   // PL: high priority
         | (DMA_SPI1_RX_CHANNEL << 25); // Channel selection

    dma_rx_complete = 0;
}

/* ---- DMA ISR (called from DMA1_Stream0_IRQHandler) ---- */

void imu_dma_irq_handler(void)
{
    volatile uint32_t *scr = (volatile uint32_t *)(DMA_SPI1_RX_BASE + 0x00);
    if (*scr & (1 << 1)) { // TCIF (transfer complete flag)
        *scr |= (1 << 2);  // Clear TCIF by writing CTCIF bit
        dma_rx_complete = 1;
        spi1_cs_high();
        if (user_dma_cb) user_dma_cb();
    }
}

/* ---- Public API ---- */

int imu_init(void)
{
    uint8_t who_am_i;
    int retries = 3;

    /* SPI1 already configured in GPIO_Init(); enable SPI1 clock */
    RCC_APB1LENR |= (1 << 12);  // SPI1EN

    /* SPI1 config: Master, CPOL=0, CPHA=0, 8-bit, MSB first, no CRC */
    SPI_CR1(SPI1_BASE) = (1 << 2)   // MSTR: master mode
                       | (0 << 0)   // CPOL=0
                       | (0 << 1)   // CPHA=0
                       | (7 << 3)   // BR[2:0] = 7 (SYSCLK/256 = ~1.9 MHz, safe for init)
                       | (0 << 7)   // LSBFIRST=0 (MSB first)
                       | (0 << 9)   // SSM=0 (hardware NSS management)
                       | (1 << 8)   // SSI=1 (internal NSS high for master)
                       | (0 << 10); // RXONLY=0 (full duplex)
    SPI_CR2(SPI1_BASE) = (1 << 0)   // DS=8-bit (0b0111 in DS field)
                       | (0xF << 8) // DS[3:0] = 0b0111 → 8-bit
                       | (1 << 12)  // FRXTH=1 (RXNE on 8-bit threshold)
                       | (1 << 1);  // SSOE=1 (NSS output enable if SSM=0)
    SPI_CR1(SPI1_BASE) |= (1 << 6); // SPE: enable SPI1

    /* Verify device ID */
    while (retries--) {
        imu_read_register(ICM42688_WHO_AM_I, &who_am_i);
        if (who_am_i == 0x47) break;
    }
    if (who_am_i != 0x47) return -1;

    /* Soft reset */
    imu_write_register(ICM42688_DEVICE_CONFIG, 0x01); // Soft reset
    for (volatile int i = 0; i < 100000; i++);        // ~1 ms

    /* Configure gyro: ±2000 DPS, ODR = 800 Hz (value 6) */
    imu_write_register(ICM42688_GYRO_CONFIG0, (6 << 5) | (0 << 0));
    // ODR=6 (800 Hz), FS=0 (±2000 DPS)

    /* Configure accel: ±16 g, ODR = 800 Hz */
    imu_write_register(ICM42688_ACCEL_CONFIG0, (6 << 5) | (0 << 0));
    // ODR=6 (800 Hz), FS=0 (±16 g)

    /* Power management: enable accel + gyro in low-noise mode */
    imu_write_register(ICM42688_PWR_MGMT0, 0x0F);
    // Gyro mode = LN (0b11), Accel mode = LN (0b11)
    for (volatile int i = 0; i < 500000; i++);  // ~5 ms settle

    /* Increase SPI speed now that device is configured */
    SPI_CR1(SPI1_BASE) &= ~(1 << 6); // Disable SPI
    SPI_CR1(SPI1_BASE) = (SPI_CR1(SPI1_BASE) & ~(7 << 3)) | (1 << 3);
    // BR = 1 (SYSCLK/4 = 120 MHz / 4 = 30 MHz → cap at 24 MHz spec. Use BR=2: /8 = 15 MHz)
    SPI_CR1(SPI1_BASE) = (SPI_CR1(SPI1_BASE) & ~(7 << 3)) | (2 << 3);
    SPI_CR1(SPI1_BASE) |= (1 << 6);  // Re-enable SPI

    /* Enable DMA for SPI1 RX */
    SPI_CR2(SPI1_BASE) |= (1 << 0);   // FDMAEN: DMA enable on RX

    /* Enable DMA1 clock and configure NVIC */
    RCC_AHB1ENR |= (1 << 0);   // DMA1EN — actually on AHB1 for H7
    // Enable DMA1 Stream 0 interrupt
    (*(volatile uint32_t *)0xE000E100UL) |= (1 << 11);  // NVIC ISER0, IRQ11 = DMA1_Stream0

    return 0;
}

int imu_read_raw(imu_sample_t *sample)
{
    uint8_t buf[14];  // 6 axes × 2 bytes + 2 bytes temp

    /* Burst read from ACCEL_XOUT (0x1F) through GYRO_ZOUT (0x29) */
    spi1_cs_low();
    spi1_write_byte(0x1F | ICM42688_SPI_READ);
    for (int i = 0; i < 14; i++) {
        spi1_write_byte(0x00);
        buf[i] = spi1_read_byte();
    }
    spi1_cs_high();

    /* Parse big-endian 16-bit values */
    sample->accel_x = (int16_t)((buf[0] << 8) | buf[1]);
    sample->accel_y = (int16_t)((buf[2] << 8) | buf[3]);
    sample->accel_z = (int16_t)((buf[4] << 8) | buf[5]);
    sample->gyro_x  = (int16_t)((buf[8] << 8) | buf[9]);
    sample->gyro_y  = (int16_t)((buf[10] << 8) | buf[11]);
    sample->gyro_z  = (int16_t)((buf[12] << 8) | buf[13]);
    sample->temperature = (int16_t)((buf[6] << 8) | buf[7]);

    return 0;
}

void imu_scale(const imu_sample_t *raw, imu_scaled_t *scaled)
{
    scaled->accel_x = (float)raw->accel_x * IMU_ACCEL_SCALE;
    scaled->accel_y = (float)raw->accel_y * IMU_ACCEL_SCALE;
    scaled->accel_z = (float)raw->accel_z * IMU_ACCEL_SCALE;
    scaled->gyro_x  = (float)raw->gyro_x * IMU_GYRO_SCALE;
    scaled->gyro_y  = (float)raw->gyro_y * IMU_GYRO_SCALE;
    scaled->gyro_z  = (float)raw->gyro_z * IMU_GYRO_SCALE;
    /* Temperature: 25°C at 0, sensitivity 132.48 LSB/°C */
    scaled->temperature = ((float)raw->temperature / 132.48f) + 25.0f;
}

int imu_fifo_read(imu_sample_t *samples, uint16_t max_count, uint16_t *out_count)
{
    /* Read FIFO count */
    uint8_t fifo_count_hi, fifo_count_lo;
    imu_read_register(ICM42688_FIFO_COUNTH, &fifo_count_hi);
    imu_read_register(ICM42688_FIFO_COUNTH + 1, &fifo_count_lo);
    uint16_t fifo_count = ((fifo_count_hi & 0x07) << 8) | fifo_count_lo;

    /* Each FIFO frame is 16 bytes (header + accel + gyro + temp + padding) */
    uint16_t frame_count = fifo_count / 16;
    if (frame_count > max_count) frame_count = max_count;
    *out_count = frame_count;

    if (frame_count == 0) return 0;

    /* Burst read FIFO using DMA */
    uint16_t total_bytes = frame_count * 16;

    spi1_cs_low();
    spi1_tx_buf[0] = ICM42688_FIFO_DATA | ICM42688_SPI_READ;

    /* Start SPI DMA transfer */
    spi1_dma_config_rx(spi1_rx_buf, total_bytes);

    /* Send FIFO_DATA read address + clock bytes via SPI TX */
    /* For simplicity, use polling for TX and DMA for RX */
    for (uint16_t i = 0; i < total_bytes + 1; i++) {
        spi1_write_byte(i == 0 ? spi1_tx_buf[0] : 0x00);
    }

    /* Wait for DMA completion */
    uint32_t timeout = 10000;
    while (!dma_rx_complete && --timeout);

    spi1_cs_high();

    if (!dma_rx_complete) return -2;  // DMA timeout

    /* Parse FIFO frames */
    for (uint16_t f = 0; f < frame_count; f++) {
        uint8_t *frame = &spi1_rx_buf[f * 16];
        // Frame format: [header(1)][accel_x(2)][accel_y(2)][accel_z(2)]
        //               [gyro_x(2)][gyro_y(2)][gyro_z(2)][temp(2)][ts(1)]
        samples[f].accel_x = (int16_t)((frame[1] << 8) | frame[2]);
        samples[f].accel_y = (int16_t)((frame[3] << 8) | frame[4]);
        samples[f].accel_z = (int16_t)((frame[5] << 8) | frame[6]);
        samples[f].gyro_x  = (int16_t)((frame[7] << 8) | frame[8]);
        samples[f].gyro_y  = (int16_t)((frame[9] << 8) | frame[10]);
        samples[f].gyro_z  = (int16_t)((frame[11] << 8) | frame[12]);
        samples[f].temperature = (int16_t)((frame[13] << 8) | frame[14]);
        samples[f].timestamp = frame[15];
    }

    return 0;
}

int imu_self_test(void)
{
    uint8_t who_am_i;
    imu_read_register(ICM42688_WHO_AM_I, &who_am_i);
    return (who_am_i == 0x47) ? 0 : -1;
}

void imu_power_down(void)
{
    imu_write_register(ICM42688_PWR_MGMT0, 0x00);  // Both accel + gyro off
}

int imu_configure_fifo(uint16_t watermark)
{
    /* Enable FIFO in streaming mode, watermark at specified level */
    imu_write_register(ICM42688_FIFO_CONFIG, 0x02); // Stream mode
    // Set watermark register (2 bytes)
    imu_write_register(0x18, (watermark >> 8) & 0x1F); // FIFO_WM[12:8]
    imu_write_register(0x19, watermark & 0xFF);         // FIFO_WM[7:0]
    return 0;
}

void imu_set_dma_callback(imu_dma_cb_t cb)
{
    user_dma_cb = cb;
}
```

## 6. Full Device Driver: SH1106 OLED Display (I2C)

### 6.1 Driver Header

```c
/*
 * drivers/oled_sh1106.h — SH1106 128×64 OLED driver for PicoRadar
 * I2C1 interface on STM32H743
 */

#ifndef OLED_SH1106_H
#define OLED_SH1106_H

#include <stdint.h>

#define OLED_WIDTH    128
#define OLED_HEIGHT   64
#define OLED_PAGES    8    // 64 / 8 rows per page
#define OLED_I2C_ADDR 0x3C

/* Simple 8×8 font (ASCII 0x20–0x7E) */
extern const uint8_t oled_font8x8[96][8];

typedef enum {
    OLED_COLOR_BLACK = 0,
    OLED_COLOR_WHITE = 1,
    OLED_COLOR_INVERT = 2
} oled_color_t;

int  oled_init(void);
void oled_power_on(void);
void oled_power_off(void);
void oled_set_contrast(uint8_t level);
void oled_clear(void);
void oled_update(void);
void oled_draw_pixel(uint8_t x, uint8_t y, oled_color_t color);
void oled_draw_char(uint8_t x, uint8_t y, char c, oled_color_t color);
void oled_draw_string(uint8_t x, uint8_t y, const char *str, oled_color_t color);
void oled_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, oled_color_t color);
void oled_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, oled_color_t color);
void oled_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, oled_color_t color);
void oled_invert_display(uint8_t on);
void oled_scroll_right(void);
void oled_scroll_left(void);
void oled_stop_scroll(void);

#endif /* OLED_SH1106_H */
```

### 6.2 Driver Implementation

```c
/*
 * drivers/oled_sh1106.c — SH1106 128×64 OLED driver for PicoRadar
 *
 * I2C1 at 400 kHz on STM32H743 (PB6=SCL, PB7=SDA).
 * Framebuffer stored in RAM, flushed to display on oled_update().
 */

#include "drivers/oled_sh1106.h"
#include "registers.h"
#include "board.h"
#include <string.h>

/* Framebuffer: 128 × 8 pages = 1024 bytes */
static uint8_t framebuffer[OLED_WIDTH * OLED_PAGES];

/* ---- I2C1 helpers ---- */

static void i2c1_wait_isr(uint32_t flag)
{
    uint32_t timeout = 50000;
    while (!(I2C_ISR(I2C1_BASE) & flag) && --timeout);
}

static int i2c1_write(uint8_t dev_addr, const uint8_t *data, uint16_t len)
{
    /* Send start + addr */
    I2C_CR2(I2C1_BASE) = (dev_addr << 1)      // 7-bit addr, left-shifted
                        | (len & 0xFF)           // NBYTES
                        | (1 << 16)              // RELOAD=0, auto-end
                        | (1 << 13);             // START condition
    // Wait TXIS or NACK
    i2c1_wait_isr(1 << 0); // TXIS bit
    if (I2C_ISR(I2C1_BASE) & (1 << 4)) return -1; // NACK

    for (uint16_t i = 0; i < len; i++) {
        i2c1_wait_isr(1 << 0); // TXIS
        I2C_TXDR(I2C1_BASE) = data[i];
    }

    /* Wait STOPF */
    i2c1_wait_isr(1 << 5); // STOPF
    I2C_ISR(I2C1_BASE) = 0; // Clear flags
    return 0;
}

static void oled_send_command(uint8_t cmd)
{
    uint8_t buf[2] = { SH1106_CTRL_CMD, cmd };
    i2c1_write(OLED_I2C_ADDR, buf, 2);
}

static void oled_send_commands(const uint8_t *cmds, uint16_t count)
{
    for (uint16_t i = 0; i < count; i++) {
        oled_send_command(cmds[i]);
    }
}

static void oled_send_data(const uint8_t *data, uint16_t len)
{
    /* Send control byte (0x40 = data) followed by data bytes.
     * For efficiency, send in chunks of 16 (I2C NBYTES max with auto-end).
     */
    uint16_t offset = 0;
    while (offset < len) {
        uint16_t chunk = len - offset;
        if (chunk > 16) chunk = 16;

        /* Start with control byte */
        I2C_CR2(I2C1_BASE) = (OLED_I2C_ADDR << 1)
                            | ((chunk + 1) & 0xFF)  // NBYTES = chunk + 1 control
                            | (1 << 16)              // Auto-end
                            | (1 << 13);             // START
        i2c1_wait_isr(1 << 0);
        I2C_TXDR(I2C1_BASE) = SH1106_CTRL_DATA;

        for (uint16_t i = 0; i < chunk; i++) {
            i2c1_wait_isr(1 << 0);
            I2C_TXDR(I2C1_BASE) = data[offset + i];
        }
        i2c1_wait_isr(1 << 5);
        I2C_ISR(I2C1_BASE) = 0;
        offset += chunk;
    }
}

/* ---- Public API ---- */

int oled_init(void)
{
    /* Enable I2C1 clock */
    RCC_APB1LENR |= (1 << 21); // I2C1EN

    /* I2C1 timing for 400 kHz @ 120 MHz (APB1 clock)
     * PRESC=3, SCLDEL=4, SDADEL=2, SCLH=26, SCLL=40
     */
    I2C_CR1(I2C1_BASE) &= ~(1 << 0); // Disable I2C for timing config
    I2C_TIMINGR(I2C1_BASE) = (3 << 28)   // PRESC
                           | (4 << 20)    // SCLDEL
                           | (2 << 16)    // SDADEL
                           | (26 << 8)    // SCLH
                           | (40 << 0);   // SCLL
    I2C_CR1(I2C1_BASE) |= (1 << 0);  // Enable I2C1

    /* Wait 10 ms after power-on (OLED module needs it) */
    for (volatile int i = 0; i < 1000000; i++);

    /* Init sequence */
    const uint8_t init_cmds[] = {
        SH1106_DISPLAY_OFF,           // 0xAE
        0xD5, 0x80,                   // Set display clock divide: 0x80
        0xA8, 0x3F,                   // Set multiplex ratio: 64
        0xD3, 0x00,                   // Set display offset: 0
        0x40,                         // Set start line: 0
        0xAD, 0x8B,                   // Set charge pump: enable (0x14 next)
        0x8D, 0x14,                   // Charge pump setting: enable
        0x20, 0x00,                   // Memory mode: horizontal addressing
        SH1106_SEG_REMAP,             // 0xA1: segment remap
        SH1106_COM_SCAN_INV,          // 0xC8: COM scan direction
        0xDA, 0x12,                   // Set COM pins: 0x12 (sequential)
        0x81, 0xCF,                   // Set contrast: 207
        0xD9, 0xF1,                   // Set pre-charge period: 0xF1
        0xDB, 0x40,                   // Set VCOMH deselect: 0x40
        0xA4,                         // Entire display on: resume
        0xA6,                         // Normal display (not inverted)
        SH1106_DISPLAY_ON,            // 0xAF
    };
    oled_send_commands(init_cmds, sizeof(init_cmds));

    oled_clear();
    oled_update();

    return 0;
}

void oled_power_on(void)
{
    oled_send_command(SH1106_DISPLAY_ON);
}

void oled_power_off(void)
{
    oled_send_command(SH1106_DISPLAY_OFF);
}

void oled_set_contrast(uint8_t level)
{
    oled_send_command(SH1106_SET_CONTRAST);
    oled_send_command(level);
}

void oled_clear(void)
{
    memset(framebuffer, 0, sizeof(framebuffer));
}

void oled_update(void)
{
    for (uint8_t page = 0; page < OLED_PAGES; page++) {
        oled_send_command(SH1106_SET_PAGE_ADDR | page);
        oled_send_command(SH1106_SET_COL_LO | 2);  // Column offset for SH1106
        oled_send_command(SH1106_SET_COL_HI | 0);
        oled_send_data(&framebuffer[page * OLED_WIDTH], OLED_WIDTH);
    }
}

void oled_draw_pixel(uint8_t x, uint8_t y, oled_color_t color)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT) return;
    uint16_t idx = (y / 8) * OLED_WIDTH + x;
    uint8_t bit = 1 << (y % 8);
    if (color == OLED_COLOR_WHITE)
        framebuffer[idx] |= bit;
    else if (color == OLED_COLOR_BLACK)
        framebuffer[idx] &= ~bit;
    else // INVERT
        framebuffer[idx] ^= bit;
}

void oled_draw_char(uint8_t x, uint8_t y, char c, oled_color_t color)
{
    if (c < ' ' || c > '~') c = ' ';
    uint8_t glyph_idx = c - ' ';
    for (uint8_t col = 0; col < 8; col++) {
        uint8_t line = oled_font8x8[glyph_idx][col];
        for (uint8_t row = 0; row < 8; row++) {
            if (line & (1 << row)) {
                oled_draw_pixel(x + col, y + row, color);
            } else {
                oled_draw_pixel(x + col, y + row, OLED_COLOR_BLACK);
            }
        }
    }
}

void oled_draw_string(uint8_t x, uint8_t y, const char *str, oled_color_t color)
{
    while (*str) {
        oled_draw_char(x, y, *str, color);
        x += 8;
        if (x + 8 > OLED_WIDTH) { x = 0; y += 8; }
        str++;
    }
}

void oled_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, oled_color_t color)
{
    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        oled_draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx)  { err += dx; y0 += sy; }
    }
}

void oled_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, oled_color_t color)
{
    oled_draw_line(x, y, x + w - 1, y, color);
    oled_draw_line(x, y + h - 1, x + w - 1, y + h - 1, color);
    oled_draw_line(x, y, x, y + h - 1, color);
    oled_draw_line(x + w - 1, y, x + w - 1, y + h - 1, color);
}

void oled_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, oled_color_t color)
{
    for (uint8_t i = x; i < x + w; i++)
        for (uint8_t j = y; j < y + h; j++)
            oled_draw_pixel(i, j, color);
}

void oled_invert_display(uint8_t on)
{
    oled_send_command(on ? 0xA7 : 0xA6);
}

void oled_scroll_right(void)
{
    oled_send_command(0x26);
    oled_send_command(0x00);
    oled_send_command(0x00);
    oled_send_command(0x07);
    oled_send_command(0x00);
    oled_send_command(0xFF);
    oled_send_command(0x2F);
}

void oled_scroll_left(void)
{
    oled_send_command(0x27);
    oled_send_command(0x00);
    oled_send_command(0x00);
    oled_send_command(0x07);
    oled_send_command(0x00);
    oled_send_command(0xFF);
    oled_send_command(0x2F);
}

void oled_stop_scroll(void)
{
    oled_send_command(0x2E);
}

/* ---- 8×8 font (ASCII 0x20–0x7E) ---- */
const uint8_t oled_font8x8[96][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // ' '
    {0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00}, // '!'
    {0x36,0x36,0x00,0x00,0x00,0x00,0x00,0x00}, // '"'
    {0x36,0x36,0x7F,0x36,0x7F,0x36,0x36,0x00}, // '#'
    {0x0C,0x3E,0x03,0x1E,0x30,0x1F,0x0C,0x00}, // '$'
    {0x00,0x63,0x33,0x18,0x0C,0x66,0x63,0x00}, // '%'
    {0x1C,0x36,0x1C,0x6E,0x3B,0x33,0x6E,0x00}, // '&'
    {0x06,0x06,0x03,0x00,0x00,0x00,0x00,0x00}, // '''
    {0x18,0x0C,0x06,0x06,0x06,0x0C,0x18,0x00}, // '('
    {0x06,0x0C,0x18,0x18,0x18,0x0C,0x06,0x00}, // ')'
    {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00}, // '*'
    {0x00,0x0C,0x0C,0x3F,0x0C,0x0C,0x00,0x00}, // '+'
    {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x06}, // ','
    {0x00,0x00,0x00,0x3F,0x00,0x00,0x00,0x00}, // '-'
    {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x00}, // '.'
    {0x60,0x30,0x18,0x0C,0x06,0x03,0x01,0x00}, // '/'
    {0x3E,0x63,0x73,0x7B,0x6F,0x67,0x3E,0x00}, // '0'
    {0x0C,0x0E,0x0C,0x0C,0x0C,0x0C,0x3F,0x00}, // '1'
    {0x1E,0x33,0x30,0x1C,0x06,0x33,0x3F,0x00}, // '2'
    {0x1E,0x33,0x30,0x1C,0x30,0x33,0x1E,0x00}, // '3'
    {0x38,0x3C,0x36,0x33,0x7F,0x30,0x78,0x00}, // '4'
    {0x3F,0x03,0x1F,0x30,0x30,0x33,0x1E,0x00}, // '5'
    {0x1C,0x06,0x03,0x1F,0x33,0x33,0x1E,0x00}, // '6'
    {0x3F,0x33,0x30,0x18,0x0C,0x0C,0x0C,0x00}, // '7'
    {0x1E,0x33,0x33,0x1E,0x33,0x33,0x1E,0x00}, // '8'
    {0x1E,0x33,0x33,0x3E,0x30,0x18,0x0E,0x00}, // '9'
    // ... (truncated for brevity, full table would have 96 entries)
    // Remaining entries follow standard 8x8 font encoding
};
```

## 7. Device Tree Overlay (for Linux-based host, reference)

```dts
/*
 * pico-radar.dts — Device tree overlay for PicoRadar
 * (Reference only — STM32H7 runs bare-metal FreeRTOS, not Linux.
 *  This overlay documents the hardware for integration with Linux SBCs
 *  that might connect via USB or Ethernet.)
 */

/dts-v1/;
/plugin/;

/ {
    compatible = "nousresearch,pico-radar";

    fragment@0 {
        target = <&i2c1>;

        __overlay__ {
            #address-cells = <1>;
            #size-cells = <0>;

            oled@3c {
                compatible = "solomon,sh1106";
                reg = <0x3c>;
                width = <128>;
                height = <64>;
                page-offset = <0>;
            };

            pmic@5a {
                compatible = "ti,tps65263";
                reg = <0x5a>;
            };
        };
    };

    fragment@1 {
        target = <&spi1>;

        __overlay__ {
            #address-cells = <1>;
            #size-cells = <0>;
            cs-gpios = <&gpioa 4 0>;

            imu@0 {
                compatible = "inv,icm42688";
                reg = <0>;
                spi-max-frequency = <24000000>;
                interrupt-parent = <&gpioc>;
                interrupts = <9 IRQ_TYPE_EDGE_RISING>;
            };
        };
    };

    fragment@2 {
        target = <&uart1>;

        __overlay__ {
            pico-radar-mmwave {
                compatible = "ti,iwr6843aop";
                current-speed = <921600>;
            };
        };
    };
};
```

## 8. Build Instructions

### 8.1 Toolchain

```bash
# Install ARM GCC cross-toolchain
sudo apt-get install gcc-arm-none-eabi binutils-arm-none-eabi

# Verify
arm-none-eabi-gcc --version
# Expected: 12.x or later

# Install OpenOCD for flashing
sudo apt-get install openocd
```

### 8.2 Building Firmware

```bash
cd firmware/
make clean
make all

# Output: build/pico-radar.bin (raw binary for QSPI flash)
#         build/pico-radar.elf (ELF with debug symbols)
#         build/pico-radar.hex (Intel HEX for flasher)
```

### 8.3 Flashing via OpenOCD + SWD

```bash
# Connect ST-Link or J-Link to 10-pin SWD header (J3)
openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
    -c "program build/pico-radar.elf verify reset exit"

# Or flash QSPI via STM32 built-in bootloader:
# 1. Hold BOOT0 button, press reset
# 2. Use STM32CubeProgrammer via UART or USB
```

### 8.4 Flashing IWR6843AOP Radar Firmware

```bash
# Using TI Uniflash or mmWave Studio
# 1. Set SOP0=LOW, SOP1=HIGH (flash mode)
# 2. Connect UART1 at 115200 baud
# 3. Send firmware via XMODEM
# 4. Set SOP0=HIGH, SOP1=LOW (functional mode)
# 5. Reset radar
```

### 8.5 Building Companion App

```bash
cd app/
npm install
npx react-native run-android   # Android
npx react-native run-ios       # iOS
```

## 9. FreeRTOS Task Architecture

| Task | Priority | Stack | Period | Function |
|---|---|---|---|---|
| RadarTask | 5 (highest) | 1024 | Event-driven | Parse TLV from IWR6843, publish point cloud |
| ImuTask | 4 | 512 | 10 ms | Read IMU FIFO, fuse with radar data |
| DisplayTask | 3 | 1024 | 100 ms | Update OLED with radar range/velocity |
| WifiTask | 2 | 2048 | Event-driven | Handle ESP32 AT commands, stream data |
| EthTask | 2 | 2048 | Event-driven | LwIP TCP/MQTT, process incoming |
| UsbTask | 1 | 512 | Event-driven | USB CDC serial console |
| Idle | 0 (tskIDLE) | 256 | Continuous | Power management, watchdog feed |

```c
/* main.c FreeRTOS task creation */
xTaskCreate(RadarTask,  "Radar",  1024, NULL, 5, &radar_task_handle);
xTaskCreate(ImuTask,    "IMU",    512,  NULL, 4, &imu_task_handle);
xTaskCreate(DisplayTask,"Display",1024, NULL, 3, &display_task_handle);
xTaskCreate(WifiTask,   "WiFi",   2048, NULL, 2, &wifi_task_handle);
xTaskCreate(EthTask,    "Eth",    2048, NULL, 2, &eth_task_handle);
xTaskCreate(UsbTask,    "USB",    512,  NULL, 1, &usb_task_handle);
vTaskStartScheduler();
```