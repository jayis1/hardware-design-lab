# Vortex-SDR Phase 4: Software Stack

## 1. Memory Map & Register Definitions

### 1.1 STM32H743 Memory Map

| Region | Address Range | Size | Access | Purpose |
|---|---|---|---|---|
| ITCM | 0x00000000–0x0000FFFF | 64 KB | Execute | Interrupt vectors, time-critical code |
| DTCM | 0x20000000–0x2000FFFF | 64 KB | R/W | Stack, critical data, DMA buffers |
| AXI SRAM | 0x24000000–0x2407FFFF | 512 KB | R/W | FFT buffers, display framebuffer |
| SRAM1 | 0x30000000–0x30017FFF | 96 KB | R/W | Driver buffers, USB endpoints |
| SRAM2 | 0x30020000–0x30027FFF | 32 KB | R/W | BLE TX/RX buffers |
| SRAM4 | 0x38000000–0x38000FFF | 4 KB | R/W | Backup registers |
| Flash | 0x08000000–0x081FFFFF | 1 MB | Execute | Firmware code + constants |
| Peripherals | 0x40000000–0x5FFFFFFF | — | R/W | MMIO registers |
| EXTI | 0x40000000–0x400003FF | — | R/W | External interrupts |
| DMA1 | 0x40020000–0x40020FFF | — | R/W | DMA controller 1 |
| DMA2 | 0x40020400–0x400207FF | — | R/W | DMA controller 2 |
| GPIOA | 0x58020000 | — | R/W | GPIO port A |
| GPIOB | 0x58020400 | — | R/W | GPIO port B |
| GPIOC | 0x58020800 | — | R/W | GPIO port C |
| GPIOD | 0x58020C00 | — | R/W | GPIO port D |
| GPIOE | 0x58021000 | — | R/W | GPIO port E |
| SPI1 | 0x40013000 | — | R/W | SPI controller 1 |
| SPI2 | 0x40003800 | — | R/W | SPI controller 2 |
| SPI3 | 0x40003C00 | — | R/W | SPI controller 3 |
| USART1 | 0x40011000 | — | R/W | UART controller 1 |
| USART2 | 0x40004400 | — | R/W | UART controller 2 |
| I2C1 | 0x40005400 | — | R/W | I2C controller 1 |
| USB_OTG_FS | 0x40080000 | — | R/W | USB OTG FS |
| ADC1 | 0x40022000 | — | R/W | ADC controller 1 |
| DAC1 | 0x40017000 | — | R/W | DAC controller 1 |
| TIM1 | 0x40012C00 | — | R/W | Advanced timer 1 |
| TIM2 | 0x40000000 | — | R/W | General timer 2 |
| TIM3 | 0x40000400 | — | R/W | General timer 3 |
| TIM6 | 0x40001000 | — | R/W | Basic timer 6 |
| TIM7 | 0x40001400 | — | R/W | Basic timer 7 |
| DMA2D | 0x5002B000 | — | R/W | 2D DMA (display) |
| RNG | 0x48060800 | — | R/W | Random number generator |

### 1.2 Clock Tree

```
                    ┌───────────┐
                    │  HSE 8MHz │
                    └─────┬─────┘
                          │
                    ┌─────▼─────┐
                    │   PLL1    │
                    │  /4 *240  │─── 480 MHz (SYSCLK)
                    │  /2       │─── 240 MHz (AHB)
                    │  /5       │─── 96 MHz (APB2)
                    │  /8       │─── 60 MHz (APB1)
                    └─────┬─────┘
                          │
                    ┌─────▼─────┐
                    │   PLL2    │
                    │  /2 *50   │─── 200 MHz (FMC)
                    │  /5       │─── 40 MHz (SPI1, SPI3)
                    └─────┬─────┘
                          │
                    ┌─────▼─────┐
                    │   PLL3    │
                    │  /2 *12   │─── 48 MHz (USB)
                    └─────┬─────┘
                          │
                    ┌─────▼─────┐
                    │  HSI48    │─── 48 MHz (USB backup)
                    └─────┬─────┘
                          │
                    ┌─────▼─────┐
                    │  LSE 32k  │─── 32.768 kHz (RTC)
                    └───────────┘
```

**Clock Configuration:**
- SYSCLK = 480 MHz (PLL1)
- AHB = 240 MHz (SYSCLK / 2)
- APB1 = 60 MHz (AHB / 4)
- APB2 = 120 MHz (AHB / 2)
- SPI1 = 50 MHz (PLL2Q / 4)
- SPI2 = 30 MHz (PLL2Q / 6)
- SPI3 = 50 MHz (PLL2Q / 4)
- USART1 = 115200 baud (APB1)
- USART2 = 921600 baud (APB1)
- USB = 48 MHz (PLL3)
- ADC = 61.44 MHz (external TCXO)

## 2. Clock Initialization

```c
/* Clock configuration — STM32H743 @ 480 MHz */
static void clock_init(void)
{
    /* Step 1: Enable HSE (8 MHz crystal) */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY))
        ;

    /* Step 2: Configure voltage scaling for 480 MHz */
    PWR->CR3 &= ~PWR_CR3_SCUEN;
    PWR->D3CR = (PWR_D3CR_VOS_Msk & (3UL << PWR_D3CR_VOS_Pos)); /* VOS = scale 0 */
    while (!(PWR->D3CR & PWR_D3CR_VOSRDY))
        ;

    /* Step 3: Configure PLL1 (480 MHz SYSCLK) */
    /* PLL1: HSE/4 * 240 / 2 = 480 MHz */
    RCC->PLLCKSELR = (4UL << RCC_PLLCKSELR_DIVM1_Pos)    /* PLL1 M = 4 */
                   | RCC_PLLCKSELR_PLLSRC_HSE;            /* Source = HSE */

    RCC->PLL1DIVR = (240UL << RCC_PLL1DIVR_N1_Pos)       /* PLL1 N = 240 */
                  | (1UL << RCC_PLL1DIVR_P1_Pos)          /* PLL1 P = 2 (value = P+1) */
                  | (7UL << RCC_PLL1DIVR_Q1_Pos)           /* PLL1 Q = 8 */
                  | (7UL << RCC_PLL1DIVR_R1_Pos);         /* PLL1 R = 8 */

    RCC->PLLCFGR = RCC_PLLCFGR_PLL1RGE_3TO8MHZ          /* Input 4-8 MHz */
                 | RCC_PLLCFGR_PLL1_VCOI2MHZWIDE         /* Wide VCO range */
                 | RCC_PLLCFGR_PLL1_FRACEN;               /* Fractional enable */

    /* Enable PLL1 */
    RCC->CR |= RCC_CR_PLL1ON;
    while (!(RCC->CR & RCC_CR_PLL1RDY))
        ;

    /* Step 4: Configure PLL2 (200 MHz for FMC/SPI) */
    /* PLL2: HSE/4 * 100 / 5 = 200 MHz */
    RCC->PLLCKSELR |= (4UL << RCC_PLLCKSELR_DIVM2_Pos);  /* PLL2 M = 4 */

    RCC->PLL2DIVR = (100UL << RCC_PLL2DIVR_N2_Pos)       /* PLL2 N = 100 */
                  | (4UL << RCC_PLL2DIVR_P2_Pos)           /* PLL2 P = 5 */
                  | (4UL << RCC_PLL2DIVR_Q2_Pos)           /* PLL2 Q = 5 = 200/5 = 40 MHz */
                  | (9UL << RCC_PLL2DIVR_R2_Pos);          /* PLL2 R = 10 */

    RCC->CR |= RCC_CR_PLL2ON;
    while (!(RCC->CR & RCC_CR_PLL2RDY))
        ;

    /* Step 5: Configure PLL3 (48 MHz for USB) */
    /* PLL3: HSE/4 * 24 / 2 = 48 MHz */
    RCC->PLLCKSELR |= (4UL << RCC_PLLCKSELR_DIVM3_Pos);  /* PLL3 M = 4 */

    RCC->PLL3DIVR = (24UL << RCC_PLL3DIVR_N3_Pos)         /* PLL3 N = 24 */
                  | (1UL << RCC_PLL3DIVR_P3_Pos)           /* PLL3 P = 2 */
                  | (1UL << RCC_PLL3DIVR_Q3_Pos)           /* PLL3 Q = 2 = 48 MHz */
                  | (1UL << RCC_PLL3DIVR_R3_Pos);          /* PLL3 R = 2 */

    RCC->CR |= RCC_CR_PLL3ON;
    while (!(RCC->CR & RCC_CR_PLL3RDY))
        ;

    /* Step 6: Configure flash latency for 480 MHz */
    FLASH->ACR = FLASH_ACR_LATENCY_4WS      /* 4 wait states @ VOS0 */
               | FLASH_ACR_PRFTEN            /* Prefetch enable */
               | FLASH_ACR_DCEN              /* D-cache enable */
               | FLASH_ACR_ICEN;             /* I-cache enable */

    /* Step 7: Switch SYSCLK to PLL1 */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL1;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL1)
        ;

    /* Step 8: Configure AHB/APB prescalers */
    RCC->D1CFGR = (0UL << RCC_D1CFGR_HPRE_Pos)     /* AHB = SYSCLK/1 = 240 MHz — wait, we need 480/2 */
                | (2UL << RCC_D1CFGR_D1PPRE_Pos);     /* APB2 = AHB/2 = 120 MHz */
    /* Actually for H7: D1 domain gets AHB, D2/D3 get APB */
    /* SYSCLK = 480 MHz, AHB = 240 MHz (HPRE=2), APB1 = 60 MHz, APB2 = 120 MHz */
    MODIFY_REG(RCC->D1CFGR, RCC_D1CFGR_HPRE_Msk, 2UL << RCC_D1CFGR_HPRE_Pos);
    MODIFY_REG(RCC->D1CFGR, RCC_D1CFGR_D1PPRE_Msk, 4UL << RCC_D1CFGR_D1PPRE_Pos);
    MODIFY_REG(RCC->D2CFGR, RCC_D2CFGR_D2PPRE1_Msk, 4UL << RCC_D2CFGR_D2PPRE1_Pos);
    MODIFY_REG(RCC->D2CFGR, RCC_D2CFGR_D2PPRE2_Msk, 4UL << RCC_D2CFGR_D2PPRE2_Pos);
    MODIFY_REG(RCC->D3CFGR, RCC_D3CFGR_D3PPRE_Msk, 4UL << RCC_D3CFGR_D3PPRE_Pos);

    /* Step 9: Enable peripheral clocks */
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN | RCC_AHB4ENR_GPIOBEN
                  | RCC_AHB4ENR_GPIOCEN | RCC_AHB4ENR_GPIODEN
                  | RCC_AHB4ENR_GPIOEEN;
    RCC->APB1LENR |= RCC_APB1LENR_USART2EN | RCC_APB1LENR_SPI3EN;
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN | RCC_APB2ENR_SPI1EN | RCC_APB2ENR_SPI4EN;
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMA2EN;
    RCC->APB1HENR |= RCC_APB1HENR_ADC12EN;
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

    /* Step 10: Enable HSI48 for USB (backup) */
    RCC->CRRC |= RCC_CRRC_HSI48ON;
    while (!(RCC->CRRC & RCC_CRRC_HSI48RDY))
        ;

    /* Step 11: Enable LSE for RTC */
    RCC->BDCR |= RCC_BDCR_LSEON;
    while (!(RCC->BDCR & RCC_BDCR_LSERDY))
        ;
}
```

## 3. GPIO Initialization

```c
static void gpio_init(void)
{
    /* Port A */
    GPIOA->MODER = (0UL << 0)    /* PA0: Input (ADC_IRQ) */
                 | (1UL << 2)    /* PA1: Output (LNA_EN) */
                 | (2UL << 4)    /* PA2: AF7 (USART2_TX) */
                 | (2UL << 6)    /* PA3: AF7 (USART2_RX) */
                 | (3UL << 8)    /* PA4: Analog (DAC1_OUT) */
                 | (2UL << 10)   /* PA5: AF5 (SPI1_SCK) */
                 | (2UL << 12)   /* PA6: AF5 (SPI1_MISO) */
                 | (2UL << 14)   /* PA7: AF5 (SPI1_MOSI) */
                 | (2UL << 16)   /* PA8: AF0 (MCO1) */
                 | (1UL << 18)   /* PA9: Input (USB_VBUS) */
                 | (1UL << 20)   /* PA10: Input (USB_ID) */
                 | (2UL << 22)   /* PA11: AF10 (USB_DM) */
                 | (2UL << 24)   /* PA12: AF10 (USB_DP) */
                 | (2UL << 26)   /* PA13: AF0 (SWDIO) */
                 | (2UL << 28)   /* PA14: AF0 (SWCLK) */
                 | (0UL << 30);  /* PA15: Input (FPGA_CDONE) */

    GPIOA->OSPEEDR = 0xAAAAA800UL;  /* High speed for SPI, USB, MCO */
    GPIOA->PUPDR = (1UL << 0)       /* PA0: Pull-up (ADC_IRQ) */
                 | (0UL << 2)       /* PA1: No pull (LNA_EN) */
                 | (0UL << 30);     /* PA15: Pull-up (FPGA_CDONE) */

    /* Port B */
    GPIOB->MODER = (1UL << 0)    /* PB0: Output (MIXER_EN) */
                 | (1UL << 2)    /* PB1: Output (ATTEN_SEL) */
                 | (2UL << 4)    /* PB2: AF5 (SPI2_SCK) */
                 | (2UL << 6)    /* PB3: AF5 (SPI2_MOSI) */
                 | (1UL << 8)    /* PB4: Output (LCD_DC) */
                 | (1UL << 10)   /* PB5: Output (LCD_CS) */
                 | (1UL << 12)   /* PB6: Output (LCD_RST) */
                 | (0UL << 14)   /* PB7: Input (TOUCH_IRQ) */
                 | (2UL << 16)   /* PB8: AF4 (I2C1_SCL) */
                 | (2UL << 18)   /* PB9: AF4 (I2C1_SDA) */
                 | (1UL << 20)   /* PB10: Output (PLL_LE) */
                 | (1UL << 22)   /* PB11: Output (PLL_CE) */
                 | (2UL << 24)   /* PB12: AF6 (SPI3_SCK) */
                 | (2UL << 26)   /* PB13: AF6 (SPI3_MISO) */
                 | (2UL << 28)   /* PB14: AF6 (SPI3_MOSI) */
                 | (1UL << 30);  /* PB15: Output (FLASH_CS) */

    GPIOB->OSPEEDR = 0xAAA8AA00UL;
    GPIOB->PUPDR = (1UL << 14);   /* PB7: Pull-up (TOUCH_IRQ) */

    /* Port C */
    GPIOC->MODER = (2UL << 0)    /* PC0: AF (ADC_CLK_P) */
                 | (2UL << 2)    /* PC1: AF (ADC_CLK_N) */
                 | (2UL << 4)    /* PC2: AF (ADC_D0_P) */
                 | (2UL << 6)    /* PC3: AF (ADC_D0_N) */
                 | (2UL << 8)    /* PC4: AF (ADC_D1_P) */
                 | (2UL << 10)  /* PC5: AF (ADC_D1_N) */
                 | (1UL << 12)  /* PC6: Output (FPGA_RST) */
                 | (1UL << 14)  /* PC7: Output (FPGA_CS) */
                 | (1UL << 16)  /* PC8: Output (TOUCH_CS) */
                 | (1UL << 18)  /* PC9: Output (LED_STATUS) */
                 | (1UL << 20)  /* PC10: Output (LED_ERROR) */
                 | (0UL << 22)  /* PC11: Input (CHG_STAT1) */
                 | (0UL << 24)  /* PC12: Input (CHG_STAT2) */
                 | (2UL << 26)  /* PC13: AF (unused) */
                 | (0UL << 28)  /* PC14: Input (unused) */
                 | (0UL << 30); /* PC15: Input (unused) */

    /* Port D */
    GPIOD->MODER = (1UL << 0)    /* PD0: Output (FPGA_SCK) */
                 | (1UL << 2)    /* PD1: Output (FPGA_SI) */
                 | (1UL << 4)    /* PD2: Output (FPGA_SS) */
                 | (3UL << 6)    /* PD3: Analog (BAT_SENSE) */
                 | (3UL << 8)    /* PD4: Analog (TEMP_SENSE) */
                 | (1UL << 10)   /* PD5: Output (MUX_SEL0) */
                 | (1UL << 12)   /* PD6: Output (MUX_SEL1) */
                 | (1UL << 14)  /* PD7: Output (MUX_EN) */
                 | (2UL << 16)  /* PD8: AF7 (USART1_TX) */
                 | (2UL << 18)  /* PD9: AF7 (USART1_RX) */
                 | (0UL << 20)  /* PD10: Input (unused) */
                 | (0UL << 22)  /* PD11: Input (unused) */
                 | (0UL << 24)  /* PD12: Input (unused) */
                 | (0UL << 26)  /* PD13: Input (unused) */
                 | (0UL << 28)  /* PD14: Input (unused) */
                 | (0UL << 30); /* PD15: Input (unused) */

    /* Port E */
    GPIOE->MODER = 0x00000000UL;  /* All inputs (buttons) */
    GPIOE->PUPDR = (1UL << 0)     /* PE0: Pull-up (BTN_CENTER) */
                 | (1UL << 2)     /* PE1: Pull-up (BTN_UP) */
                 | (1UL << 4)     /* PE2: Pull-up (BTN_DOWN) */
                 | (1UL << 6)     /* PE3: Pull-up (BTN_LEFT) */
                 | (1UL << 8)     /* PE4: Pull-up (BTN_RIGHT) */
                 | (1UL << 10);  /* PE5: Pull-up (BTN_MENU) */
}
```

## 4. Critical Device Drivers

### 4.1 ADF4351 PLL Driver (SPI bit-bang)

See `firmware/drivers/adf4351.c` for full implementation.

Key operations:
- `adf4351_init()`: Initialize GPIO pins, power up PLL
- `adf4351_set_frequency(uint64_t freq_hz)`: Program PLL to target frequency
- `adf4351_lock_detect()`: Check PLL lock status
- `adf4351_power_down()`: Put PLL in low-power mode

Register programming: 6 registers (R0-R5), 32 bits each, MSB first, clocked on falling edge, latched on LE rising edge.

### 4.2 AD9645 ADC Driver (SPI config + LVDS data)

See `firmware/drivers/ad9645.c` for full implementation.

Key operations:
- `ad9645_init()`: Initialize SPI, configure ADC registers
- `ad9645_set_sample_rate(uint32_t rate)`: Set decimation ratio
- `ad9645_set_data_format(ad9645_data_fmt_t fmt)`: Select output format
- `ad9645_power_down()`: Put ADC in standby

AD9645 register map (SPI):
- 0x00: Chip ID (read-only)
- 0x01: Chip grade (read-only)
- 0x02: Power modes
- 0x03: Clock divide ratio
- 0x04: Decimation ratio
- 0x05: Decimation filter select
- 0x08: Data format (offset binary / twos complement)
- 0x09: Test mode (ramp, checkerboard, etc.)
- 0x0A: ADC input buffer config
- 0x0B: ADC reference config
- 0x0C: Output mode (LVDS / CMOS)
- 0x0D: Output drive config

### 4.3 ILI9341 Display Driver (SPI + DMA)

See `firmware/drivers/ili9341.c` for full implementation.

Key operations:
- `ili9341_init()`: Reset display, configure registers
- `ili9341_fill_rect(x, y, w, h, color)`: Fill rectangle (used for waterfall)
- `ili9341_draw_line(x0, y0, x1, y1, color)`: Draw line (used for spectrum trace)
- `ili9341_draw_text(x, y, str, fg, bg)`: Draw text (used for labels)
- `ili9341_set_pixel(x, y, color)`: Set individual pixel

Display pipeline:
1. MCU prepares FFT bin data (8K bins → 320 pixels)
2. MCU maps bins to display columns with peak detection
3. MCU updates display via DMA SPI transfer
4. Waterfall: shift display down by 1 row, draw new top row

## 5. DMA Configuration

### 5.1 DMA Channel Map

| Stream | Channel | Source | Direction | Mode | Priority |
|---|---|---|---|---|---|
| DMA1 Stream0 | Ch0 | SPI1_RX | Peripheral→Memory | Circular | High |
| DMA1 Stream3 | Ch3 | SPI2_TX | Memory→Peripheral | Normal | High |
| DMA1 Stream5 | Ch5 | SPI3_RX | Peripheral→Memory | Normal | Medium |
| DMA1 Stream6 | Ch6 | SPI3_TX | Memory→Peripheral | Normal | Medium |
| DMA2 Stream1 | Ch4 | USART2_RX | Peripheral→Memory | Circular | Medium |
| DMA2 Stream7 | Ch4 | USART2_TX | Memory→Peripheral | Normal | Low |
| DMA2 Stream2 | Ch9 | ADC1 | Peripheral→Memory | Normal | Low |

### 5.2 SPI1 DMA (FPGA data readback)

```c
/* Configure DMA1 Stream0 for SPI1 RX (FPGA data) */
DMA1_Stream0->CR = (0UL << DMA_SxCR_CHSEL_Pos)   /* Channel 0 */
                 | (0UL << DMA_SxCR_DIR_Pos)        /* Peripheral-to-memory */
                 | (1UL << DMA_SxCR_CIRC_Pos)       /* Circular mode */
                 | (1UL << DMA_SxCR_MINC_Pos)       /* Memory increment */
                 | (0UL << DMA_SxCR_PINC_Pos)       /* No peripheral increment */
                 | (1UL << DMA_SxCR_PL_Pos)          /* High priority */
                 | (2UL << DMA_SxCR_MSIZE_Pos)       /* 16-bit memory */
                 | (1UL << DMA_SxCR_PSIZE_Pos)      /* 16-bit peripheral */
                 | (0UL << DMA_SxCR_PINCOS_Pos)      /* Pin offset by PSIZE */
                 | (1UL << DMA_SxCR_HTIE_Pos)        /* Half-transfer interrupt */
                 | (1UL << DMA_SxCR_TCIE_Pos);       /* Transfer-complete interrupt */
```

## 6. FFT Processing Pipeline

### 6.1 FPGA FFT Engine

The iCE40UP5K FPGA implements a 1024-point FFT engine with the following pipeline:

```
ADC (14-bit, 61.44 MSPS)
    │
    ▼ LVDS (14 bits parallel)
┌────────────┐
│  LVDS      │ ← 14-bit samples at up to 61.44 MSPS
│  Receiver  │
└─────┬──────┘
      │ 16-bit samples
      ▼
┌────────────┐
│  Window    │ ← Hanning / Blackman-Harris / Kaiser (selectable)
│  Function  │
└─────┬──────┘
      │ Windowed samples
      ▼
┌────────────┐
│  Decimator │ ← CIC filter, programmable decimation (1x to 64x)
│  CIC + FIR │
└─────┬──────┘
      │ Decimated samples (down to 960 kSPS)
      ▼
┌────────────┐
│  1024-pt   │ ← Radix-2 DIT FFT, 16-bit fixed-point
│  FFT Core  │ ← 5-stage butterfly pipeline
└─────┬──────┘
      │ Complex FFT output (2048 bytes)
      ▼
┌────────────┐
│  Magnitude │ ← sqrt(I² + Q²), log10() for dB display
│  + dB Conv │
└─────┬──────┘
      │ 1024 power bins (2 bytes each)
      ▼
┌────────────┐
│  Peak      │ ← Find top 10 peaks, store frequency + amplitude
│  Detector  │
└─────┬──────┘
      │ SPI readback
      ▼
┌────────────┐
│  MCU       │ ← STM32H743 reads bins via SPI1
│  (Display) │ ← Renders spectrum + waterfall on ILI9341
└────────────┘
```

### 6.2 FFT Configuration Parameters

| Parameter | Range | Default | Description |
|---|---|---|---|
| FFT_SIZE | 256, 512, 1024, 2048, 4096, 8192 | 1024 | Number of FFT points |
| WINDOW | Rect, Hann, BH, FlatTop, Kaiser | Hann | Window function |
| DECIMATION | 1, 2, 4, 8, 16, 32, 64 | 4 | Sample rate decimation |
| OVERLAP | 0%, 25%, 50%, 75% | 50% | FFT overlap |
| AVERAGING | 1, 2, 4, 8, 16, 32 | 4 | Number of averages |
| SCALE | 1, 2, 5, 10 dB/div | 10 | Display scale |
| REF_LEVEL | -120 to +20 dBm | -20 | Reference level |

## 7. RTOS Task Model (Bare-metal Super-loop)

The Vortex-SDR uses a bare-metal super-loop architecture with interrupt-driven DMA for time-critical operations, rather than a full RTOS. This maximizes deterministic timing for the FFT pipeline.

### 7.1 Interrupt Priority Map

| Priority | IRQ | Handler | Purpose |
|---|---|---|---|
| 0 (highest) | DMA1_Stream0 | spi1_dma_irq | FPGA FFT data readback |
| 1 | DMA1_Stream3 | spi2_dma_irq | Display DMA transfer complete |
| 2 | SPI1 | spi1_irq | FPGA SPI transfer complete |
| 3 | TIM6_DAC | timer6_irq | 10 ms FFT poll timer |
| 4 | TIM7 | timer7_irq | 50 ms display update timer |
| 5 | USART2 | usart2_irq | BLE UART RX |
| 6 | DMA2_Stream1 | ble_dma_rx_irq | BLE DMA RX |
| 7 | EXTI9_5 | exti9_5_irq | Button/touch interrupts |
| 8 | OTG_FS | usb_irq | USB device |
| 9 (lowest) | TIM1_UP | timer1_irq | 1 ms system tick |

### 7.2 Main Loop

```c
int main(void)
{
    /* Hardware initialization */
    clock_init();
    gpio_init();
    dma_init();
    spi_init();
    uart_init();
    i2c_init();
    adc_init();
    dac_init();
    timer_init();
    usb_init();

    /* Peripheral initialization */
    fpga_init();          /* Load bitstream, wait for CDONE */
    ad9645_init();        /* Configure ADC via SPI */
    adf4351_init();       /* Initialize PLL to default frequency */
    ili9341_init();       /* Initialize display */
    w25q256_init();       /* Initialize flash storage */
    ble_init();           /* Initialize nRF52832 via UART */

    /* Display boot screen */
    ili9341_fill_rect(0, 0, 320, 240, ILI9341_BLACK);
    ili9341_draw_text(80, 100, "VORTEX-SDR", ILI9341_WHITE, ILI9341_BLACK);
    ili9341_draw_text(80, 120, "Initializing...", ILI9341_CYAN, ILI9341_BLACK);

    /* Start sweep engine */
    sweep_state_t sweep = {
        .start_freq = 88000000,    /* 88 MHz */
        .stop_freq = 108000000,    /* 108 MHz */
        .rbw = 10000,             /* 10 kHz */
        .fft_size = 1024,
        .window = WINDOW_HANN,
        .averaging = 4,
        .mode = SWEEP_CONTINUOUS
    };
    sweep_start(&sweep);

    /* Main super-loop */
    while (1) {
        /* Poll for new FFT data from FPGA */
        if (g_fft_data_ready) {
            fft_process_bins(&sweep);
            g_fft_data_ready = 0;
        }

        /* Update display (50 ms timer) */
        if (g_display_update) {
            display_update(&sweep);
            g_display_update = 0;
        }

        /* Handle BLE commands */
        if (g_ble_rx_ready) {
            ble_process_command(&sweep);
            g_ble_rx_ready = 0;
        }

        /* Handle USB commands */
        if (g_usb_rx_ready) {
            usb_process_command(&sweep);
            g_usb_rx_ready = 0;
        }

        /* Handle button/touch input */
        if (g_ui_event) {
            ui_handle_event(&sweep);
            g_ui_event = 0;
        }

        /* Log to flash (background) */
        if (g_log_pending && !g_flash_busy) {
            flash_log_spectrum(&sweep);
            g_log_pending = 0;
        }

        /* Low-power wait for interrupt */
        __WFI();
    }
}
```

## 8. USB Descriptors

See `firmware/usb_descriptors.h` for full descriptor definitions.

USB Device:
- VID: 0x1209 (pid.codes)
- PID: 0xVR01 (Vortex-SDR Rev 1)
- Configuration 1: CDC-ACM (command interface) + Bulk (data interface)
- Configuration 2: Vendor-specific (raw binary protocol)

## 9. BLE Protocol

### 9.1 GATT Service

| Service UUID | Characteristic UUID | Properties | Description |
|---|---|---|---|
| 0xFEF1 (Vortex Main) | 0xFEF2 | Write | Command input (20 bytes) |
| 0xFEF1 (Vortex Main) | 0xFEF3 | Notify | Spectrum data output (244 bytes max) |
| 0xFEF1 (Vortex Main) | 0xFEF4 | Read | Device status (8 bytes) |
| 0xFEF1 (Vortex Main) | 0xFEF5 | Write | Sweep configuration (16 bytes) |
| 0x180A (Device Info) | 0x2A29 | Read | Manufacturer name |
| 0x180A (Device Info) | 0x2A24 | Read | Model number |
| 0x180A (Device Info) | 0x2A26 | Read | Firmware version |
| 0x180A (Device Info) | 0x2A25 | Read | Serial number |

### 9.2 Command Format (0xFEF2)

| Offset | Size | Field | Description |
|---|---|---|---|
| 0 | 1 | CMD | Command byte |
| 1-2 | 2 | LEN | Payload length |
| 3-4 | 2 | SEQ | Sequence number |
| 5-18 | 14 | DATA | Command payload |
| 19 | 1 | CHK | XOR checksum |

### 9.3 Spectrum Data Format (0xFEF3)

| Offset | Size | Field | Description |
|---|---|---|---|
| 0 | 1 | HDR | Header byte (0x55) |
| 1-2 | 2 | SEQ | Sequence number |
| 3-4 | 2 | FMT | Format (0x01 = uint8 dBm, 0x02 = int16 dBm) |
| 5-6 | 2 | NBINS | Number of bins in this packet |
| 7-8 | 2 | BIN_IDX | Starting bin index |
| 9-240 | 232 | DATA | Spectrum bin data |
| 241-243 | 3 | CRC | CRC-24 |

## 10. Boot Sequence

### 10.1 Boot Flow

```
Power On / Reset
    │
    ▼
Reset_Handler (startup_stm32h743xx.s)
    │
    ├── Copy .data from Flash to DTCM
    ├── Zero .bss in DTCM + AXI SRAM
    ├── Call SystemInit() [clocks minimal]
    ├── Call main()
    │
    ▼
clock_init()
    ├── Enable HSE (8 MHz)
    ├── Configure PLL1 (480 MHz SYSCLK)
    ├── Configure PLL2 (200 MHz)
    ├── Configure PLL3 (48 MHz USB)
    ├── Set flash latency (4 WS)
    ├── Switch SYSCLK to PLL1
    ├── Enable peripheral clocks
    └── Enable HSI48 + LSE
    │
    ▼
gpio_init()
    ├── Configure all GPIO pins per pin table
    └── Set pull-ups for buttons and inputs
    │
    ▼
power_init()
    ├── Enable LNA (NET_LNA_EN = HIGH)
    ├── Enable Mixer (NET_MIXER_EN = HIGH)
    ├── Enable PLL (NET_PLL_CE = HIGH)
    └── Wait 10ms for rails to stabilize
    │
    ▼
fpga_init()
    ├── Assert FPGA reset (NET_FPGA_RST = LOW)
    ├── Wait 1ms
    ├── Release FPGA reset (NET_FPGA_RST = HIGH)
    ├── Wait for CDONE (NET_FPGA_CDONE = HIGH)
    ├── Load bitstream from W25Q256 via SPI3
    ├── Verify bitstream CRC
    └── Configure FPGA via SPI config port
    │
    ▼
ad9645_init()
    ├── Reset ADC (NET_ADC_PD = LOW)
    ├── Configure ADC registers via SPI1
    ├── Set LVDS output mode
    ├── Set 14-bit resolution
    ├── Set internal reference
    └── Enable ADC data output
    │
    ▼
adf4351_init()
    ├── Configure PLL registers (R5 → R0)
    ├── Set default frequency (1 GHz)
    ├── Wait for lock detect
    └── Enable RF output
    │
    ▼
display_init()
    ├── Reset display (NET_LCD_RST pulse)
    ├── Initialize ILI9341 registers
    ├── Set orientation (landscape)
    ├── Clear screen (black)
    └── Draw boot logo
    │
    ▼
ble_init()
    ├── Reset nRF52832 (UART break)
    ├── Configure UART at 921600 baud
    ├── Send AT+INIT command
    ├── Configure GATT services
    └── Start advertising
    │
    ▼
usb_init()
    ├── Configure USB OTG FS device
    ├── Set device descriptors
    ├── Configure endpoints (EP0, EP1, EP2, EP3)
    ├── Attach to bus
    └── Wait for enumeration
    │
    ▼
sweep_start()
    ├── Configure sweep parameters
    ├── Enable FPGA FFT engine
    ├── Start DMA for data readback
    └── Start timer for display update
    │
    ▼
Main Loop
    ├── Poll FFT data (DMA interrupt)
    ├── Update display (50 ms timer)
    ├── Handle BLE commands (UART DMA)
    ├── Handle USB commands (USB ISR)
    ├── Handle UI events (EXTI)
    └── WFI (low power wait)
```

## 11. Error Handling

### 11.1 Fault Handlers

```c
/* Hard fault handler — log and blink error LED */
void HardFault_Handler(void)
{
    /* Log fault information to flash */
    uint32_t *stack_ptr;
    __asm volatile ("TST LR, #4 \n"
                    "ITE EQ \n"
                    "MRSEQ %0, MSP \n"
                    "MRSNE %0, PSP \n" : "=r" (stack_ptr));

    /* Store fault info */
    g_fault_info.r0 = stack_ptr[0];
    g_fault_info.r1 = stack_ptr[1];
    g_fault_info.r2 = stack_ptr[2];
    g_fault_info.r3 = stack_ptr[3];
    g_fault_info.pc = stack_ptr[6];
    g_fault_info.lr = stack_ptr[5];
    g_fault_info.psr = stack_ptr[7];

    /* Blink red LED rapidly */
    while (1) {
        GPIOC->ODR ^= (1UL << 10);  /* Toggle red LED */
        for (volatile uint32_t i = 0; i < 500000; i++)
            ;
    }
}
```

### 11.2 Watchdog

- **IWDG**: Independent watchdog, 2-second timeout
- Refreshed in main loop (normal operation)
- If main loop hangs > 2s, watchdog resets MCU
- Fault counters stored in backup SRAM (SRAM4)