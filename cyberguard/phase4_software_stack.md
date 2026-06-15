# CyberGuard — Phase 4: Software Stack

## 1. Boot Strategy

### 1.1 Boot Sequence

```
Power-On / Reset
    │
    ├── BOOT0 = LOW → Boot from Flash (STM32L562)
    │
    ├── Stage 1: Boot ROM (STM32L562)
    │   ├── Verify option bytes (RDP Level 0/1/2)
    │   ├── Load initial stack from 0x08000000
    │   └── Jump to Stage 2
    │
    ├── Stage 2: SPL / System Init (our code, 0x08000000)
    │   ├── Configure clock tree (HSE bypass → PLL → 110 MHz)
    │   ├── Enable FPU
    │   ├── Enable I-Cache / D-Cache
    │   ├── Initialize .data, zero .bss
    │   ├── Configure MPU (secure/non-secure regions)
    │   ├── Initialize power domains (STPMIC1 via I2C2)
    │   │   ├── Enable buck converter → VDD_3V3
    │   │   ├── Enable LDO1 → VDD_1V8
    │   │   └── Enable LDO2 → VDD_1V1
    │   ├── Initialize GPIO
    │   ├── Initialize QSPI flash (memory-mapped mode)
    │   ├── Verify A71CH (I2C1 probe at 0x48)
    │   ├── Initialize UART1 (FPC1025) and UART2 (nRF52833)
    │   └── Jump to Stage 3
    │
    ├── Stage 3: Main Application (0x08008000)
    │   ├── Initialize USB CDC/HID device
    │   ├── Initialize SPI1 (PN7150)
    │   ├── Initialize NFC subsystem
    │   ├── Initialize BLE command interface (UART2)
    │   ├── Initialize CTAP2 authenticator
    │   ├── Load resident keys from A71CH
    │   ├── Start FreeRTOS scheduler
    │   │   ├── Task: USB HID handler
    │   │   ├── Task: NFC APDU handler
    │   │   ├── Task: BLE command handler
    │   │   ├── Task: Fingerprint manager
    │   │   └── Task: Power management (idle → STOP mode)
    │   └── Main loop: event dispatch
    │
    └── Firmware Update Mode (GPIO trigger or OTA)
        ├── Verify Ed25519 signature on update image
        ├── Write new image to QSPI flash (B partition)
        ├── Swap boot partition flag
        └── Reset into new image
```

### 1.2 A/B Partition Layout (QSPI Flash)

| Offset | Size | Content |
|--------|------|---------|
| 0x000000 | 4 KB | Boot header + partition table |
| 0x001000 | 32 KB | Stage 2 SPL (always valid, not updatable OTA) |
| 0x009000 | 256 KB | Partition A (active firmware) |
| 0x049000 | 256 KB | Partition B (update firmware) |
| 0x089000 | 64 KB | Fingerprint template storage |
| 0x099000 | 64 KB | BLE bonding data |
| 0x0A9000 | 1 MB | CTAP2 credential metadata |
| 0x1A9000 | 864 KB | Reserved / OTA download staging |

### 1.3 Secure Boot

- STM32L562 supports hardware secure boot (OB boot lock)
- RSA-3072 public key hash programmed in option bytes
- Firmware images signed with Ed25519 (A71CH stored private key)
- On each boot, SHA-256 hash of active partition verified against stored signature
- If verification fails, fall back to other partition

## 2. MMIO Register Definitions

### 2.1 STM32L562 Register Map (Base Addresses)

```c
// registers.h - CyberGuard MMIO Register Definitions

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

// Base addresses (STM32L562 Reference Manual RM0438)
#define PERIPH_BASE         0x40000000UL
#define APB1PERIPH_BASE     (PERIPH_BASE + 0x00000000UL)
#define APB2PERIPH_BASE     (PERIPH_BASE + 0x00010000UL)
#define AHB1PERIPH_BASE     (PERIPH_BASE + 0x00020000UL)
#define AHB2PERIPH_BASE     (PERIPH_BASE + 0x08000000UL)

// ---- USART Registers ----
#define USART1_BASE          (APB2PERIPH_BASE + 0x00004000UL)  // Fingerprint
#define USART2_BASE          (APB1PERIPH_BASE + 0x00004400UL)  // BLE

typedef struct {
    volatile uint32_t CR1;      // Control register 1
    volatile uint32_t CR2;      // Control register 2
    volatile uint32_t CR3;      // Control register 3
    volatile uint32_t BRR;      // Baud rate register
    volatile uint32_t GTPR;     // Guard time / prescaler
    volatile uint32_t RTOR;     // Receiver timeout register
    volatile uint32_t RQR;     // Request register
    volatile uint32_t ISR;     // Interrupt & status register
    volatile uint32_t ICR;     // Interrupt flag clear register
    volatile uint32_t RDR;     // Receive data register
    volatile uint32_t TDR;     // Transmit data register
} USART_TypeDef;

#define USART1  ((USART_TypeDef *)USART1_BASE)
#define USART2  ((USART_TypeDef *)USART2_BASE)

// USART CR1 bits
#define USART_CR1_UE          (1U << 0)    // USART enable
#define USART_CR1_TE          (1U << 3)    // Transmitter enable
#define USART_CR1_RE          (1U << 2)    // Receiver enable
#define USART_CR1_RXNEIE      (1U << 5)    // RXNE interrupt enable
#define USART_CR1_TXEIE       (1U << 7)    // TXE interrupt enable
#define USART_CR1_M0          (1U << 12)   // Word length bit 0
#define USART_CR1_OVER8       (1U << 15)   // Oversampling by 8

// ---- SPI Registers ----
#define SPI1_BASE             (APB2PERIPH_BASE + 0x00003C00UL)  // NFC
#define SPI2_BASE             (APB1PERIPH_BASE + 0x00003800UL)  // QSPI Flash

typedef struct {
    volatile uint32_t CR1;      // Control register 1
    volatile uint32_t CR2;      // Control register 2
    volatile uint32_t SR;       // Status register
    volatile uint32_t DR;       // Data register
    volatile uint32_t CRCPR;    // CRC polynomial register
    volatile uint32_t RXCRCR;   // RX CRC register
    volatile uint32_t TXCRCR;   // TX CRC register
    volatile uint32_t I2SCFGR;  // I2S configuration
    volatile uint32_t I2SPR;    // I2S prescaler
} SPI_TypeDef;

#define SPI1  ((SPI_TypeDef *)SPI1_BASE)
#define SPI2  ((SPI_TypeDef *)SPI2_BASE)

// SPI CR1 bits
#define SPI_CR1_CPHA          (1U << 0)
#define SPI_CR1_CPOL          (1U << 1)
#define SPI_CR1_MSTR          (1U << 2)
#define SPI_CR1_BR_DIV64      (5U << 3)   // 110 MHz / 64 ≈ 1.7 MHz
#define SPI_CR1_SPE           (1U << 6)
#define SPI_CR1_LSBFIRST      (1U << 7)
#define SPI_CR1_SSI          (1U << 8)
#define SPI_CR1_SSM          (1U << 9)
#define SPI_CR1_RXONLY       (1U << 10)
#define SPI_CR1_CRCL_8BIT    (0U << 11)

// SPI CR2 bits
#define SPI_CR2_DS_8BIT       (7U << 8)
#define SPI_CR2_SSOE          (1U << 2)
#define SPI_CR2_FRF           (1U << 4)    // TI frame format
#define SPI_CR2_ERRIE         (1U << 5)
#define SPI_CR2_RXNEIE        (1U << 6)
#define SPI_CR2_TXEIE         (1U << 7)

// ---- I2C Registers ----
#define I2C1_BASE             (APB1PERIPH_BASE + 0x00005400UL)  // Secure Element
#define I2C2_BASE             (APB1PERIPH_BASE + 0x00005800UL)  // PMIC

typedef struct {
    volatile uint32_t CR1;      // Control register 1
    volatile uint32_t CR2;      // Control register 2
    volatile uint32_t OAR1;     // Own address register 1
    volatile uint32_t OAR2;     // Own address register 2
    volatile uint32_t TIMINGR;  // Timing register
    volatile uint32_t TIMEOUTR; // Timeout register
    volatile uint32_t ISR;     // Interrupt & status register
    volatile uint32_t ICR;     // Interrupt flag clear
    volatile uint32_t PECR;    // PEC register
    volatile uint32_t RXDR;   // Receive data register
    volatile uint32_t TXDR;   // Transmit data register
} I2C_TypeDef;

#define I2C1  ((I2C_TypeDef *)I2C1_BASE)
#define I2C2  ((I2C_TypeDef *)I2C2_BASE)

// I2C CR1 bits
#define I2C_CR1_PE            (1U << 0)    // Peripheral enable
#define I2C_CR1_TXIE          (1U << 1)    // TX interrupt enable
#define I2C_CR1_RXIE          (1U << 2)    // RX interrupt enable
#define I2C_CR1_NACKIE        (1U << 4)    // NACK interrupt enable
#define I2C_CR1_STOPIE        (1U << 5)    // STOP interrupt enable

// I2C ISR bits
#define I2C_ISR_TXE           (1U << 0)
#define I2C_ISR_RXNE          (1U << 2)
#define I2C_ISR_NACKF         (1U << 4)
#define I2C_ISR_STOPF         (1U << 5)
#define I2C_ISR_TC            (1U << 6)
#define I2C_ISR_BUSY          (1U << 15)

// ---- GPIO Registers ----
#define GPIOA_BASE            (AHB2PERIPH_BASE + 0x00000000UL)
#define GPIOB_BASE            (AHB2PERIPH_BASE + 0x00000400UL)
#define GPIOC_BASE            (AHB2PERIPH_BASE + 0x00000800UL)
#define GPIOH_BASE            (AHB2PERIPH_BASE + 0x00001C00UL)

typedef struct {
    volatile uint32_t MODER;     // Mode register
    volatile uint32_t OTYPER;    // Output type register
    volatile uint32_t OSPEEDR;   // Output speed register
    volatile uint32_t PUPDR;     // Pull-up/pull-down register
    volatile uint32_t IDR;       // Input data register
    volatile uint32_t ODR;       // Output data register
    volatile uint32_t BSRR;      // Bit set/reset register
    volatile uint32_t LCKR;      // Lock register
    volatile uint32_t AFR[2];    // Alternate function registers
    volatile uint32_t BRR;       // Bit reset register
    volatile uint32_t SECCFGR;   // Security configuration
} GPIO_TypeDef;

#define GPIOA  ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB  ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC  ((GPIO_TypeDef *)GPIOC_BASE)
#define GPIOH  ((GPIO_TypeDef *)GPIOH_BASE)

// GPIO mode values
#define GPIO_MODE_INPUT        0U
#define GPIO_MODE_OUTPUT       1U
#define GPIO_MODE_AF           2U
#define GPIO_MODE_ANALOG       3U

// GPIO AF mappings (STM32L562)
#define GPIO_AF4_I2C1          4U    // PB0=I2C1_SCL, PC5=I2C1_SDA
#define GPIO_AF4_I2C2          4U    // PB10=I2C2_SCL, PB11=I2C2_SDA
#define GPIO_AF5_SPI1          5U    // PA4-7=SPI1
#define GPIO_AF7_USART1        7U    // PB1=USART1_TX, PB2=USART1_RX
#define GPIO_AF7_USART2        7U    // PA2=USART2_TX, PA3=USART2_RX
#define GPIO_AF10_USB          10U   // PA11=USB_DM, PA12=USB_DP
#define GPIO_AF0_MCO           0U

// ---- RCC Registers ----
#define RCC_BASE              (AHB1PERIPH_BASE + 0x00001000UL)

typedef struct {
    volatile uint32_t CR;        // Clock control register
    volatile uint32_t ICSCR;     // Internal clock sources calibration
    volatile uint32_t CFGR;     // Clock configuration register
    volatile uint32_t PLLCFGR;  // PLL configuration
    volatile uint32_t RESERVED0; 
    volatile uint32_t CIER;     // Clock interrupt enable
    volatile uint32_t CIFR;     // Clock interrupt flags
    volatile uint32_t CICR;     // Clock interrupt clear
    volatile uint32_t RESERVED1;
    volatile uint32_t AHB1ENR;  // AHB1 peripheral clock enable
    volatile uint32_t AHB2ENR;  // AHB2 peripheral clock enable
    volatile uint32_t AHB3ENR;  // AHB3 peripheral clock enable
    volatile uint32_t RESERVED2;
    volatile uint32_t APB1ENR1; // APB1 peripheral clock enable 1
    volatile uint32_t APB1ENR2; // APB1 peripheral clock enable 2
    volatile uint32_t APB2ENR;  // APB2 peripheral clock enable
    volatile uint32_t RESERVED3[8];
    volatile uint32_t AHB1SMENR;
    volatile uint32_t AHB2SMENR;
    volatile uint32_t AHB3SMENR;
    volatile uint32_t RESERVED4;
    volatile uint32_t APB1SMENR1;
    volatile uint32_t APB1SMENR2;
    volatile uint32_t APB2SMENR;
    volatile uint32_t RESERVED5[8];
    volatile uint32_t CCIPR1;   // Clock configuration per IP 1
    volatile uint32_t CCIPR2;   // Clock configuration per IP 2
    volatile uint32_t SECCFGR;  // Security configuration
} RCC_TypeDef;

#define RCC  ((RCC_TypeDef *)RCC_BASE)

// RCC CR bits
#define RCC_CR_HSION          (1U << 8)    // HSI16 enable
#define RCC_CR_MSION          (1U << 0)    // MSI enable
#define RCC_CR_HSEON          (1U << 16)   // HSE enable
#define RCC_CR_PLLON          (1U << 24)   // PLL enable
#define RCC_CR_PLLRDY         (1U << 25)   // PLL ready

// RCC AHB2ENR bits
#define RCC_AHB2ENR_GPIOAEN   (1U << 0)
#define RCC_AHB2ENR_GPIOBEN   (1U << 1)
#define RCC_AHB2ENR_GPIOCEN   (1U << 2)
#define RCC_AHB2ENR_GPIOHEN   (1U << 7)
#define RCC_AHB2ENR_OTGFSEN  (1U << 12)   // USB OTG FS

// RCC APB1ENR1 bits
#define RCC_APB1ENR1_USART2EN (1U << 17)
#define RCC_APB1ENR1_SPI2EN   (1U << 14)
#define RCC_APB1ENR1_I2C1EN   (1U << 21)
#define RCC_APB1ENR1_I2C2EN   (1U << 22)

// RCC APB2ENR bits
#define RCC_APB2ENR_USART1EN  (1U << 14)
#define RCC_APB2ENR_SPI1EN    (1U << 12)
#define RCC_APB2ENR_SYSCFGEN  (1U << 0)

// ---- EXTI Registers ----
#define EXTI_BASE             (PERIPH_BASE + 0x0000EC00UL)

typedef struct {
    volatile uint32_t RTSR1;      // Rising trigger selection
    volatile uint32_t FTSR1;      // Falling trigger selection
    volatile uint32_t SWIER1;     // Software interrupt event
    volatile uint32_t RPR1;      // Rising pending register
    volatile uint32_t FPR1;      // Falling pending register
    volatile uint32_t RESERVED[3];
    volatile uint32_t IMR1;      // Interrupt mask register
    volatile uint32_t EMR1;      // Event mask register
    volatile uint32_t RESERVED2];
    volatile uint32_t EXTICR[4]; // External interrupt configuration
    volatile uint32_t SECCFGR;   // Security configuration
    volatile uint32_t PRIVCFGR;  // Privilege configuration
} EXTI_TypeDef;

#define EXTI  ((EXTI_TypeDef *)EXTI_BASE)

// ---- DMA Registers ----
#define DMA1_BASE              (AHB1PERIPH_BASE + 0x00000000UL)

typedef struct {
    volatile uint32_t ISR;       // Interrupt status
    volatile uint32_t IFCR;     // Interrupt flag clear
} DMA_Common_TypeDef;

typedef struct {
    volatile uint32_t CR;       // Channel configuration
    volatile uint32_t NDTR;     // Number of data to transfer
    volatile uint32_t PAR;       // Peripheral address
    volatile uint32_t MAR;       // Memory address
    volatile uint32_t RESERVED;
} DMA_Channel_TypeDef;

// ---- USB OTG FS Registers ----
#define USB_OTG_FS_BASE        (PERIPH_BASE + 0x06800000UL)
#define USB_OTG_FS_PERIPH_BASE (PERIPH_BASE + 0x06800000UL)

// ---- Flash Registers ----
#define FLASH_BASE             (PERIPH_BASE + 0x00002000UL)

typedef struct {
    volatile uint32_t ACR;       // Access control
    volatile uint32_t PDKEYR;   // Power-down key
    volatile uint32_t KEYR;     // Key
    volatile uint32_t OPTKEYR;  // Option key
    volatile uint32_t SR;       // Status
    volatile uint32_t CR;       // Control
    volatile uint32_t ECCR;     // ECC
    volatile uint32_t RESERVED1;
    volatile uint32_t OPTR;    // Option
    volatile uint32_t PCROP1SR; // PCROP1 start
    volatile uint32_t PCROP1ER; // PCROP1 end
    volatile uint32_t WRP1AR;   // Write protection 1A
    volatile uint32_t WRP1BR;   // Write protection 1B
    volatile uint32_t RESERVED2[4];
    volatile uint32_t PCROP2SR; // PCROP2 start
    volatile uint32_t PCROP2ER; // PCROP2 end
    volatile uint32_t WRP2AR;   // Write protection 2A
    volatile uint32_t WRP2BR;   // Write protection 2B
    volatile uint32_t SECR;    // Security
} FLASH_TypeDef;

#define FLASH  ((FLASH_TypeDef *)FLASH_BASE)

// ---- Power Registers ----
#define PWR_BASE               (APB1PERIPH_BASE + 0x00000000UL)

typedef struct {
    volatile uint32_t CR1;      // Power control register 1
    volatile uint32_t CR2;      // Power control register 2
    volatile uint32_t CR3;      // Power control register 3
    volatile uint32_t CR4;      // Power control register 4
    volatile uint32_t SR1;      // Power status register 1
    volatile uint32_t SR2;      // Power status register 2
    volatile uint32_t SCR;      // Power status clear register
    volatile uint32_t RESERVED;
    volatile uint32_t PUCRA;   // Pull-up control register A
    volatile uint32_t PDCRA;   // Pull-down control register A
    volatile uint32_t PUCRB;   // Pull-up control register B
    volatile uint32_t PDCRB;   // Pull-down control register B
    volatile uint32_t PUCRC;   // Pull-up control register C
    volatile uint32_t PDCRC;   // Pull-down control register C
    volatile uint32_t PUCRH;   // Pull-up control register H
    volatile uint32_t PDCRH;   // Pull-down control register H
} PWR_TypeDef;

#define PWR  ((PWR_TypeDef *)PWR_BASE)

// ---- SysTick ----
#define SysTick_BASE          0xE000E010UL

typedef struct {
    volatile uint32_t CTRL;
    volatile uint32_t LOAD;
    volatile uint32_t VAL;
    volatile uint32_t CALIB;
} SysTick_TypeDef;

#define SysTick  ((SysTick_TypeDef *)SysTick_BASE)

// ---- NVIC ----
#define NVIC_BASE             0xE000E100UL

typedef struct {
    volatile uint32_t ISER[8];
    volatile uint32_t RESERVED0[24];
    volatile uint32_t ICER[8];
    volatile uint32_t RESERVED1[24];
    volatile uint32_t ISPR[8];
    volatile uint32_t RESERVED2[24];
    volatile uint32_t ICPR[8];
    volatile uint32_t RESERVED3[24];
    volatile uint32_t IABR[8];
    volatile uint32_t RESERVED4[56];
    volatile uint32_t IPR[60];
} NVIC_TypeDef;

#define NVIC  ((NVIC_TypeDef *)NVIC_BASE)

// Interrupt numbers (STM32L562)
#define USART1_IRQn            37
#define USART2_IRQn            38
#define SPI1_IRQn               35
#define I2C1_IRQn               31
#define I2C2_IRQn               32
#define EXTI0_IRQn              6
#define EXTI1_IRQn              7
#define EXTI2_IRQn              8
#define EXTI3_IRQn              9
#define EXTI4_IRQn              10
#define EXTI5_9_IRQn            23
#define EXTI10_15_IRQn          40
#define OTG_FS_IRQn             67
#define DMA1_Channel1_IRQn      11
#define DMA1_Channel2_IRQn      12
#define DMA1_Channel3_IRQn      13
#define DMA1_Channel4_IRQn      14

// ---- A71CH I2C Address ----
#define A71CH_I2C_ADDR          0x48    // 7-bit I2C address

// ---- STPMIC1 I2C Address ----
#define STPMIC1_I2C_ADDR        0x08    // 7-bit I2C address

// ---- PN7150 SPI Command Constants ----
#define PN7150_CMD_SYSTEM_RESET         0x01
#define PN532_CMD_DIAGNOSE              0x00
#define PN7150_CMD_WRITE_REGISTER       0x08
#define PN7150_CMD_READ_REGISTER        0x0A
#define PN7150_CMD_RF_CONFIGURATION     0x02
#define PN7150_CMD_RF_FIELD             0x02
#define PN7150_CMD_MIFARE_AUTH           0x60

// ---- FPC1025 UART Protocol Constants ----
#define FPC1025_BAUD_RATE      115200
#define FPC1025_CMD_INIT       0x01
#define FPC1025_CMD_CAPTURE    0x03
#define FPC1025_CMD_ENROLL     0x06
#define FPC1025_CMD_VERIFY     0x07
#define FPC1025_CMD_DELETE     0x08
#define FPC1025_RSP_ACK        0xA0
#define FPC1025_RSP_NACK       0xA1

// ---- nRF52833 BLE Protocol ----
#define BLE_CMD_RESET           0x01
#define BLE_CMD_ADV_START       0x10
#define BLE_CMD_ADV_STOP        0x11
#define BLE_CMD_CONN_PARAM      0x20
#define BLE_CMD_SEND_DATA       0x30
#define BLE_CMD_DISCONNECT      0x40

#endif // REGISTERS_H