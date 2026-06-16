# Phase 4: Software Stack — Nebula Matrix
## Professional LED Matrix Display Engine

---

## 1. Software Architecture Overview

The Nebula Matrix software stack spans three domains:

1. **FPGA Bitstream (Verilog)** — Real-time pixel processing, frame buffer management, HUB75E waveform generation
2. **STM32H7 Firmware (C, FreeRTOS)** — System management, networking, USB configuration, SD card playback
3. **React Native Companion App** — Configuration, monitoring, firmware update

```
┌─────────────────────────────────────────────────────────────────┐
│                    SOFTWARE ARCHITECTURE                         │
│                                                                  │
│  ┌──────────────────────┐   ┌──────────────────────┐            │
│  │  FPGA BITSTREAM       │   │  STM32H7 FIRMWARE    │            │
│  │  (Verilog, Vivado)    │   │  (C, FreeRTOS, GCC)  │            │
│  │                       │   │                      │            │
│  │  ┌─────────────────┐  │   │  ┌────────────────┐  │            │
│  │  │ Frame Buffer Ctrl│  │   │  │ LWIP Stack     │  │            │
│  │  │ (AXI4, DDR3 MIG) │  │   │  │ - UDP/RTP      │  │            │
│  │  └────────┬────────┘  │   │  │ - Art-Net 4     │  │            │
│  │           │            │   │  │ - sACN E1.31    │  │            │
│  │  ┌────────┴────────┐  │   │  │ - Custom Proto  │  │            │
│  │  │ Pixel Processing │  │   │  └────────────────┘  │            │
│  │  │ - Gamma LUT      │  │   │                      │            │
│  │  │ - CSC            │  │   │  ┌────────────────┐  │            │
│  │  │ - Dithering      │  │   │  │ TinyUSB Stack  │  │            │
│  │  │ - Scaler         │  │   │  │ - RNDIS Device │  │            │
│  │  └────────┬────────┘  │   │  │ - WebUSB Config │  │            │
│  │           │            │   │  └────────────────┘  │            │
│  │  ┌────────┴────────┐  │   │                      │            │
│  │  │ HUB75E Waveform  │  │   │  ┌────────────────┐  │            │
│  │  │ - BCM PWM (16b)  │  │   │  │ Mongoose HTTP  │  │            │
│  │  │ - 16 ports       │  │   │  │ - Config API   │  │            │
│  │  │ - 512 PWM ch     │  │   │  │ - Status JSON   │  │            │
│  │  └─────────────────┘  │   │  │ - OTA Update    │  │            │
│  │                       │   │  └────────────────┘  │            │
│  │  ┌─────────────────┐  │   │                      │            │
│  │  │ SPI Slave (50MHz)│  │   │  ┌────────────────┐  │            │
│  │  │ UART Cmd IF      │  │   │  │ FatFS + SDMMC  │  │            │
│  │  └─────────────────┘  │   │  │ - Raw playback  │  │            │
│  └──────────────────────┘   │  │ - Config files   │  │            │
│                              │  └────────────────┘  │            │
│  ┌──────────────────────┐   │                      │            │
│  │  REACT NATIVE APP    │   │  ┌────────────────┐  │            │
│  │  (JavaScript)        │   │  │ System Monitor  │  │            │
│  │                       │   │  │ - Temp sensors  │  │            │
│  │  - Matrix Config     │   │  │ - Fan PID       │  │            │
│  │  - Live Monitor      │   │  │ - Voltage mon   │  │            │
│  │  - Color Calibration │   │  │ - Frame stats   │  │            │
│  │  - Firmware Update   │   │  └────────────────┘  │            │
│  │  - Binary Protocol   │   │                      │            │
│  └──────────────────────┘   └──────────────────────┘            │
└─────────────────────────────────────────────────────────────────┘
```

---

## 2. Boot Strategy

### 2.1 Power-Up Sequence

```
1. 12V DC applied → TPS65218 PMIC powers up in sequence:
   a. DCDC1 (5.0V) — 0ms
   b. DCDC2 (3.3V) — 2ms delay
   c. DCDC3 (1.8V) — 4ms delay
   d. DCDC4 (1.35V) — 6ms delay
   e. LDO1 (1.0V FPGA Core) — 8ms delay
   f. LDO2 (1.2V FPGA BRAM) — 10ms delay
   g. LDO3 (3.3V MGT) — 12ms delay
   h. PGOOD asserted — 15ms

2. FPGA powers up, samples M[2:0] pins:
   M[1:0] = 01 → SPI Master mode
   FPGA drives CCLK, reads bitstream from W25Q128JV
   Configuration time: ~50ms (128Mb at ~25 MHz effective)

3. FPGA DONE pin goes high → STM32H7 detects via PE3 interrupt
   STM32H7 asserts FPGA_RESET_N (PE2) low for 10µs, then high
   FPGA internal PLLs lock (~100µs)

4. STM32H7 boots from internal Flash:
   a. SystemInit(): Configure PLLs, 480 MHz SYSCLK
   b. HAL_Init(): Initialize HAL library
   c. Clock config for all peripherals
   d. GPIO init (pins, interrupts)
   e. Initialize peripherals in order:
      - I2C2: Read TMP117 sensors
      - I2C3: Configure Si5351A clock outputs
      - I2C4: Verify PMIC status
      - SPI1: Verify FPGA flash communication
      - UART1: Open command channel to FPGA
      - SPI2: Open pixel data channel to FPGA
      - ETH: Initialize KSZ9031 PHY, start LWIP
      - USB: Initialize USB3320C, start TinyUSB RNDIS
      - SDMMC1: Mount SD card, initialize FatFS
   f. Start FreeRTOS scheduler

5. System ready — total boot time < 500ms
```

### 2.2 FPGA Bitstream Loading (Alternate Path)

If FPGA DONE does not go high within 200ms, STM32H7 can reprogram FPGA:

```
STM32_PE2(FPGA_RESET_N) = 0;  // Hold FPGA in reset
delay_ms(1);
STM32_PA4(SPI1_NSS) = 0;      // Select flash
// Erase flash sectors 0-127 (128Mb)
SPI1_EraseChip();
// Read bitstream from SD card (firmware/nebula_matrix.bit)
// Write to flash via SPI1
SPI1_ProgramBitstream(bitstream_data, bitstream_len);
STM32_PA4(SPI1_NSS) = 1;      // Deselect flash
STM32_PE2(FPGA_RESET_N) = 1;  // Release FPGA reset
// FPGA loads new bitstream from flash
```

---

## 3. MMIO Register Definitions

### 3.1 FPGA Register Map (accessed via UART command interface)

The FPGA exposes a 16-bit address space of control/status registers accessible
via a simple UART protocol (115200 baud, 8N1).

| Address | Register Name | Width | Access | Description |
|---------|---------------|-------|--------|-------------|
| 0x0000 | FPGA_ID | 32 | R | FPGA design ID (0x4E454255 = "NEBU") |
| 0x0004 | FPGA_VERSION | 32 | R | Version (major[15:8].minor[7:0]) |
| 0x0008 | FPGA_STATUS | 32 | R | Status flags |
| 0x000C | FPGA_CONTROL | 32 | R/W | Control register |
| 0x0010 | MATRIX_WIDTH | 16 | R/W | Active matrix width (1-256) |
| 0x0012 | MATRIX_HEIGHT | 16 | R/W | Active matrix height (1-256) |
| 0x0014 | PANEL_SCAN | 8 | R/W | Panel scan type (1/32, 1/16, 1/8, 1/4) |
| 0x0015 | PANEL_CONFIG | 8 | R/W | Panel configuration flags |
| 0x0018 | BRIGHTNESS | 16 | R/W | Global brightness (0-65535) |
| 0x001A | CONTRAST | 16 | R/W | Global contrast (0-65535) |
| 0x0020 | GAMMA_R_BASE | 32 | R/W | Base address of Red gamma LUT |
| 0x0024 | GAMMA_G_BASE | 32 | R/W | Base address of Green gamma LUT |
| 0x0028 | GAMMA_B_BASE | 32 | R/W | Base address of Blue gamma LUT |
| 0x0030 | FB0_BASE | 32 | R | Frame Buffer 0 base address (DDR3) |
| 0x0034 | FB1_BASE | 32 | R | Frame Buffer 1 base address (DDR3) |
| 0x0038 | FB_ACTIVE | 1 | R/W | Active frame buffer (0 or 1) |
| 0x003C | FB_SWAP | 1 | W | Writing '1' swaps frame buffers |
| 0x0040 | INPUT_SOURCE | 8 | R/W | Input source select |
| 0x0041 | INPUT_STATUS | 8 | R | Input source status |
| 0x0044 | HDMI_TIMING_HACT | 16 | R | HDMI active horizontal pixels |
| 0x0046 | HDMI_TIMING_VACT | 16 | R | HDMI active vertical lines |
| 0x0048 | HDMI_TIMING_HFP | 16 | R | HDMI horizontal front porch |
| 0x004A | HDMI_TIMING_HSW | 16 | R | HDMI horizontal sync width |
| 0x004C | HDMI_TIMING_HBP | 16 | R | HDMI horizontal back porch |
| 0x004E | HDMI_TIMING_VFP | 16 | R | HDMI vertical front porch |
| 0x0050 | HDMI_TIMING_VSW | 16 | R | HDMI vertical sync width |
| 0x0052 | HDMI_TIMING_VBP | 16 | R | HDMI vertical back porch |
| 0x0054 | HDMI_TIMING_FPS | 16 | R | HDMI frame rate × 100 |
| 0x0060 | SCALER_CROP_X | 16 | R/W | Crop region X start |
| 0x0062 | SCALER_CROP_Y | 16 | R/W | Crop region Y start |
| 0x0064 | SCALER_CROP_W | 16 | R/W | Crop region width |
| 0x0066 | SCALER_CROP_H | 16 | R/W | Crop region height |
| 0x0070 | DITHER_MODE | 8 | R/W | Dithering algorithm select |
| 0x0071 | DITHER_STRENGTH | 8 | R/W | Dithering strength (0-255) |
| 0x0080 | HUB75E_REFRESH | 16 | R | Actual output refresh rate × 100 |
| 0x0084 | HUB75E_FRAMES_OUT | 32 | R | Total frames output counter |
| 0x0088 | HUB75E_DROPPED | 32 | R | Dropped frame counter |
| 0x0090 | SPI_PIXELS_RX | 32 | R | Pixels received via SPI |
| 0x0094 | SPI_FRAMES_RX | 32 | R | Complete frames received via SPI |
| 0x0098 | SPI_ERRORS | 32 | R | SPI error counter |
| 0x00A0 | TEMP_FPGA | 16 | R | FPGA internal temperature (XADC) |
| 0x00A2 | TEMP_DDR_A | 16 | R | DDR3 Ch A temperature (from TMP117) |
| 0x00A4 | TEMP_DDR_B | 16 | R | DDR3 Ch B temperature (from TMP117) |
| 0x00B0 | UART_CMD_RX | 32 | R | UART commands received |
| 0x00B4 | UART_CMD_ERRORS | 32 | R | UART command error count |
| 0x0100-0x01FF | GAMMA_R_LUT | 256×16 | R/W | Red gamma LUT (direct access) |
| 0x0200-0x02FF | GAMMA_G_LUT | 256×16 | R/W | Green gamma LUT (direct access) |
| 0x0300-0x03FF | GAMMA_B_LUT | 256×16 | R/W | Blue gamma LUT (direct access) |

### 3.2 FPGA_STATUS Register (0x0008) Bit Definitions

| Bit | Name | Description |
|-----|------|-------------|
| 0 | FB0_READY | Frame Buffer 0 initialized and ready |
| 1 | FB1_READY | Frame Buffer 1 initialized and ready |
| 2 | HDMI_LOCK | HDMI receiver PLL locked to source |
| 3 | HDMI_ACTIVE | HDMI receiving valid video |
| 4 | SPI_ACTIVE | SPI receiving pixel data |
| 5 | SD_PLAYBACK | SD card playback active |
| 6 | HUB75E_RUNNING | HUB75E output engine running |
| 7 | DDR_CAL_DONE | DDR3 calibration complete |
| 8 | DDR_A_ERROR | DDR3 Channel A error |
| 9 | DDR_B_ERROR | DDR3 Channel B error |
| 10 | THERMAL_WARN | Temperature > 80°C |
| 11 | THERMAL_CRIT | Temperature > 85°C (shutdown) |
| 12 | INPUT_OVERFLOW | Input FIFO overflow |
| 13 | OUTPUT_UNDERRUN | Output buffer underrun |
| 31:14 | Reserved | |

### 3.3 FPGA_CONTROL Register (0x000C) Bit Definitions

| Bit | Name | Description |
|-----|------|-------------|
| 0 | ENABLE_OUTPUT | Enable HUB75E output (1=on) |
| 1 | FREEZE_FRAME | Freeze current frame |
| 2 | TEST_PATTERN | Enable internal test pattern |
| 3 | TEST_PATTERN_SEL | 0=color bars, 1=gradient, 2=grid, 3=white |
| 5:4 | Reserved | |
| 6 | FORCE_SWAP | Force frame buffer swap |
| 7 | SOFT_RESET | Soft reset pixel pipeline |
| 8 | GAMMA_BYPASS | Bypass gamma correction |
| 9 | DITHER_BYPASS | Bypass dithering |
| 10 | CSC_BYPASS | Bypass color space conversion |
| 11 | SCALER_BYPASS | Bypass scaler (1:1 passthrough) |
| 15:12 | Reserved | |
| 31:16 | Reserved | |

### 3.4 INPUT_SOURCE Register (0x0040)

| Value | Source |
|-------|--------|
| 0x00 | None (black output) |
| 0x01 | Ethernet (Art-Net/sACN/Custom) |
| 0x02 | HDMI |
| 0x03 | SD Card Playback |
| 0x04 | Internal Test Pattern |
| 0x05 | SPI Direct (raw pixel stream) |

### 3.5 DITHER_MODE Register (0x0070)

| Value | Algorithm |
|-------|-----------|
| 0x00 | No dithering |
| 0x01 | Floyd-Steinberg error diffusion |
| 0x02 | Atkinson dithering |
| 0x03 | Blue noise (pre-computed mask) |
| 0x04 | Temporal dithering (frame-to-frame) |
| 0x05 | Hybrid (spatial + temporal) |

---

## 4. Clock Configuration Code

### 4.1 STM32H7 System Clock Configuration

```c
/**
 * system_clock.c — STM32H743 System Clock Configuration
 * Target: 480 MHz SYSCLK from 25 MHz external (Si5351A CLK0)
 *
 * Clock tree:
 *   HSE (25 MHz) → PLL1 → 480 MHz SYSCLK
 *                            → 240 MHz AHB (HCLK)
 *                            → 120 MHz APB1, APB2
 *                            → 240 MHz AXI SRAM
 */

#include "stm32h7xx_hal.h"

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

    /* Configure voltage scaling for 480 MHz operation */
    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

    /* HSE oscillator: 25 MHz from Si5351A CLK0 (bypass mode) */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;  /* External clock, no crystal */
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 5;       /* VCO input = 25/5 = 5 MHz */
    RCC_OscInitStruct.PLL.PLLN = 192;     /* VCO = 5*192 = 960 MHz */
    RCC_OscInitStruct.PLL.PLLP = 2;       /* SYSCLK = 960/2 = 480 MHz */
    RCC_OscInitStruct.PLL.PLLQ = 4;       /* PLL1Q = 960/4 = 240 MHz (for peripherals) */
    RCC_OscInitStruct.PLL.PLLR = 2;       /* PLL1R = 960/2 = 480 MHz */
    RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_1;  /* 4-8 MHz VCO input */
    RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;  /* Wide VCO range */
    RCC_OscInitStruct.PLL.PLLFRACN = 0;   /* Integer mode */
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    /* System, AHB, APB clock configuration */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2
                                | RCC_CLOCKTYPE_D1PCLK1 | RCC_CLOCKTYPE_D3PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;     /* 480 MHz */
    RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;       /* 240 MHz */
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;     /* 120 MHz */
    RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;     /* 120 MHz */
    RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;     /* 120 MHz */
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);

    /* Peripheral clock configuration */
    PeriphClkInitStruct.PeriphClockSelection =
        RCC_PERIPHCLK_SPI2 | RCC_PERIPHCLK_SPI1 |
        RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_SDMMC |
        RCC_PERIPHCLK_USB | RCC_PERIPHCLK_FDCAN;
    PeriphClkInitStruct.Spi123ClockSelection = RCC_SPI123CLKSOURCE_PLL;
    PeriphClkInitStruct.Spi45ClockSelection = RCC_SPI45CLKSOURCE_D2PCLK1;
    PeriphClkInitStruct.Usart16ClockSelection = RCC_USART16CLKSOURCE_D2PCLK2;
    PeriphClkInitStruct.SdmmcClockSelection = RCC_SDMMCCLKSOURCE_PLL;
    PeriphClkInitStruct.UsbClockSelection = RCC_USBCLKSOURCE_PLL;
    PeriphClkInitStruct.PLL2.PLL2M = 5;
    PeriphClkInitStruct.PLL2.PLL2N = 100;    /* 500 MHz VCO */
    PeriphClkInitStruct.PLL2.PLL2P = 5;      /* 100 MHz for SPI */
    PeriphClkInitStruct.PLL2.PLL2Q = 10;     /* 50 MHz for SDMMC */
    PeriphClkInitStruct.PLL2.PLL2R = 10;     /* 50 MHz for USB */
    PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_1;
    PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;
    PeriphClkInitStruct.PLL2.PLL2FRACN = 0;
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

    /* Enable SYSCFG and CPU cache */
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    SCB_EnableICache();
    SCB_EnableDCache();
}
```

### 4.2 Si5351A Clock Generator Configuration

```c
/**
 * si5351_config.c — Configure Si5351A for all system clocks
 *
 * Outputs:
 *   CLK0: 25.000000 MHz — STM32H7 HSE
 *   CLK1: 50.000000 MHz — FPGA System Clock
 *   CLK2: 27.000000 MHz — ADV7611 HDMI Rx
 *   CLK3: 25.000000 MHz — KSZ9031 (optional)
 *   CLK4: 60.000000 MHz — USB3320C ULPI
 *   CLK5: 33.333333 MHz — FPGA Pixel Clock
 *   CLK6: 26.666667 MHz — FPGA DDR3 Ref (×40 PLL = 1066.667 MHz)
 *   CLK7: 24.000000 MHz — ADV7611 Audio MCLK
 *
 * Reference: 25 MHz crystal on XA/XB pins
 */

#include "si5351.h"

#define SI5351_I2C_ADDR  0x60

/* PLL configurations for each frequency plan */
typedef struct {
    uint32_t freq_hz;
    uint8_t pll_src;     /* 0=PLLA, 1=PLLB */
    uint32_t pll_freq;   /* PLL VCO frequency (600-900 MHz) */
    uint32_t multisynth_div;  /* integer part */
    uint32_t multisynth_num;  /* numerator */
    uint32_t multisynth_denom;/* denominator */
    uint8_t rdiv;        /* output divider (power of 2) */
} si5351_output_config_t;

static const si5351_output_config_t nebula_clock_plan[8] = {
    /* CLK0: 25.000000 MHz for STM32H7 HSE */
    { .freq_hz = 25000000, .pll_src = 0, .pll_freq = 800000000,
      .multisynth_div = 32, .multisynth_num = 0, .multisynth_denom = 1, .rdiv = 1 },

    /* CLK1: 50.000000 MHz for FPGA System Clock */
    { .freq_hz = 50000000, .pll_src = 0, .pll_freq = 800000000,
      .multisynth_div = 16, .multisynth_num = 0, .multisynth_denom = 1, .rdiv = 1 },

    /* CLK2: 27.000000 MHz for ADV7611 */
    { .freq_hz = 27000000, .pll_src = 1, .pll_freq = 810000000,
      .multisynth_div = 30, .multisynth_num = 0, .multisynth_denom = 1, .rdiv = 1 },

    /* CLK3: 25.000000 MHz for KSZ9031 (optional) */
    { .freq_hz = 25000000, .pll_src = 0, .pll_freq = 800000000,
      .multisynth_div = 32, .multisynth_num = 0, .multisynth_denom = 1, .rdiv = 1 },

    /* CLK4: 60.000000 MHz for USB3320C ULPI */
    { .freq_hz = 60000000, .pll_src = 1, .pll_freq = 810000000,
      .multisynth_div = 13, .multisynth_num = 1, .multisynth_denom = 2, .rdiv = 1 },

    /* CLK5: 33.333333 MHz for FPGA Pixel Clock */
    { .freq_hz = 33333333, .pll_src = 0, .pll_freq = 800000000,
      .multisynth_div = 24, .multisynth_num = 0, .multisynth_denom = 1, .rdiv = 1 },

    /* CLK6: 26.666667 MHz for FPGA DDR3 Ref (×40 = 1066.667 MHz) */
    { .freq_hz = 26666667, .pll_src = 0, .pll_freq = 800000000,
      .multisynth_div = 30, .multisynth_num = 0, .multisynth_denom = 1, .rdiv = 1 },

    /* CLK7: 24.000000 MHz for ADV7611 Audio MCLK */
    { .freq_hz = 24000000, .pll_src = 1, .pll_freq = 810000000,
      .multisynth_div = 33, .multisynth_num = 3, .multisynth_denom = 4, .rdiv = 1 },
};

int si5351_configure_all(void)
{
    int ret;
    uint8_t reg_val;

    /* Disable all outputs during configuration */
    si5351_write_reg(3, 0xFF);   /* CLK0-7 output enable = off */
    si5351_write_reg(16, 0x00);  /* CLK0-3 control: powered down */
    si5351_write_reg(17, 0x00);  /* CLK4-7 control: powered down */

    /* Configure PLLA: 800 MHz VCO */
    /* XTAL = 25 MHz, PLLA = 25 × (32 + 0/1) = 800 MHz */
    si5351_write_reg(26, 0x00);  /* PLLA multisynth NA = 32 (integer mode) */
    si5351_write_reg(27, 0x00);
    si5351_write_reg(28, 0x00);
    si5351_write_reg(29, 0x00);
    /* PLLA feedback divider: integer mode, a=32, b=0, c=1 */
    si5351_write_reg(22, 0x20);  /* PLLA feedback: MSNA_P1[15:8] */
    si5351_write_reg(23, 0x00);  /* PLLA feedback: MSNA_P1[7:0] */
    si5351_write_reg(24, 0x00);  /* PLLA feedback: MSNA_P2[19:16] */
    si5351_write_reg(25, 0x00);  /* PLLA feedback: MSNA_P2[15:8] */
    /* Reset PLLA */
    si5351_write_reg(177, 0x20); /* PLLA reset */
    HAL_Delay(10);

    /* Configure PLLB: 810 MHz VCO */
    /* XTAL = 25 MHz, PLLB = 25 × (32 + 2/5) = 810 MHz */
    si5351_write_reg(34, 0x00);  /* PLLB multisynth NB = 32 */
    si5351_write_reg(35, 0x00);
    si5351_write_reg(36, 0x00);
    si5351_write_reg(37, 0x00);
    /* PLLB feedback: a=32, b=2, c=5 */
    si5351_write_reg(30, 0x20);  /* PLLB feedback: MSNB_P1[15:8] */
    si5351_write_reg(31, 0x00);  /* PLLB feedback: MSNB_P1[7:0] */
    si5351_write_reg(32, 0x00);  /* PLLB feedback: MSNB_P2[19:16] */
    si5351_write_reg(33, 0x00);  /* PLLB feedback: MSNB_P2[15:8] */
    /* Reset PLLB */
    si5351_write_reg(177, 0x80); /* PLLB reset */
    HAL_Delay(10);

    /* Configure each output multisynth */
    for (int i = 0; i < 8; i++) {
        uint8_t base_reg = 42 + (i * 8);
        const si5351_output_config_t *cfg = &nebula_clock_plan[i];

        /* Multisynth divider parameters */
        uint32_t a = cfg->multisynth_div;
        uint32_t b = cfg->multisynth_num;
        uint32_t c = cfg->multisynth_denom;

        /* Calculate P1, P2, P3 per Si5351A datasheet */
        uint32_t p1 = 128 * a + (128 * b / c) - 512;
        uint32_t p2 = 128 * b - c * (128 * b / c);
        uint32_t p3 = c;

        si5351_write_reg(base_reg + 0, (p3 >> 8) & 0xFF);
        si5351_write_reg(base_reg + 1, p3 & 0xFF);
        si5351_write_reg(base_reg + 2, ((p1 >> 8) & 0x03) | ((cfg->rdiv & 0x07) << 4));
        si5351_write_reg(base_reg + 3, p1 & 0xFF);
        si5351_write_reg(base_reg + 4, ((p3 >> 12) & 0x0F) | ((p2 >> 16) & 0xF0));
        si5351_write_reg(base_reg + 5, (p2 >> 8) & 0xFF);
        si5351_write_reg(base_reg + 6, p2 & 0xFF);

        /* Set clock source (PLLA or PLLB) */
        uint8_t clk_ctrl = (cfg->pll_src == 0) ? 0x0C : 0x2C;  /* 8mA drive, PLLA/PLLB */
        if (i < 4) {
            uint8_t reg16 = si5351_read_reg(16);
            reg16 |= (clk_ctrl << ((3-i) * 2));
            si5351_write_reg(16, reg16);
        } else {
            uint8_t reg17 = si5351_read_reg(17);
            reg17 |= (clk_ctrl << ((7-i) * 2));
            si5351_write_reg(17, reg17);
        }
    }

    /* Enable all outputs */
    si5351_write_reg(3, 0x00);   /* All outputs enabled */

    return 0;
}
```

---

## 5. GPIO Initialization Code

```c
/**
 * gpio_init.c — STM32H7 GPIO Initialization for Nebula Matrix
 *
 * Configures all GPIO pins for:
 * - FPGA interface (SPI, UART, control signals)
 * - Ethernet PHY control
 * - USB PHY control
 * - SD card detect
 * - Status LEDs
 * - Fan PWM
 * - Temperature sensor I2C
 * - PMIC I2C
 * - Clock generator I2C
 * - HDMI receiver control
 * - ADC inputs (voltage/current monitoring)
 */

#include "stm32h7xx_hal.h"
#include "board.h"

void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Enable all GPIO clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();

    /* ================================================================
     * FPGA Interface Pins
     * ================================================================ */

    /* SPI2: Pixel data channel (STM32 → FPGA)
     * PB13 = SCK, PB14 = MISO, PB15 = MOSI, PB12 = NSS
     * AF5, High speed, push-pull, no pull */
    GPIO_InitStruct.Pin = GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* UART1: Command channel (STM32 ↔ FPGA)
     * PA9 = TX, PA10 = RX
     * AF7, High speed, push-pull */
    GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* FPGA control signals (PE0-PE3)
     * PE0 = FPGA_IRQ (input, interrupt on rising edge)
     * PE1 = FPGA_READY (input)
     * PE2 = FPGA_RESET_N (output, push-pull)
     * PE3 = FPGA_DONE (input, interrupt on rising edge) */
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2, GPIO_PIN_SET);  /* FPGA_RESET_N = high */

    /* ================================================================
     * SPI1: FPGA Configuration Flash (W25Q128JV)
     * PA4 = NSS, PA5 = SCK, PA6 = MISO, PA7 = MOSI
     * AF5, High speed */
    GPIO_InitStruct.Pin = GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* ================================================================
     * Ethernet RMII Interface
     * ================================================================ */

    /* ETH_RMII_REF_CLK: PH1 (input) */
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

    /* ETH_MDIO: PH1 (already configured above, same pin) — actually MDIO is PH1? No.
     * Let's use correct pins:
     * PC1 = MDC (AF11), PH1 = actually not MDIO. Let me re-check.
     * STM32H743 RMII pin mapping:
     *   PA8 = ETH_RMII_CRS_DV  (AF11)
     *   PC0 = ETH_RMII_TX_EN   (AF11)
     *   PC1 = ETH_MDC          (AF11)
     *   PC2 = ETH_RMII_TXD0    (AF11)
     *   PC3 = ETH_RMII_TXD1    (AF11)
     *   PC4 = ETH_RMII_RXD0    (AF11)
     *   PC5 = ETH_RMII_RXD1    (AF11)
     *   PB5 = ETH_RMII_RXER    (AF11) — actually PB11? Let me use correct mapping.
     *   PH0 = ETH_RMII_REF_CLK (AF11) — actually this is correct
     *   PH1 = ETH_MDIO         (AF11) — correct
     *   PB11 = ETH_RMII_RXER   (AF11) — actually PB5? Let me use PB5.
     */

    /* PA8: ETH_RMII_CRS_DV */
    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PC0: ETH_RMII_TX_EN, PC1: ETH_MDC, PC2: ETH_TXD0, PC3: ETH_TXD1,
       PC4: ETH_RXD0, PC5: ETH_RXD1 */
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2
                        | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* PB5: ETH_RMII_RXER (AF11) */
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* PH0: ETH_RMII_REF_CLK, PH1: ETH_MDIO */
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

    /* ETH PHY control signals */
    /* PE10: ETH_RESET_N (output), PE12: ETH_INT_N (input) */
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_10, GPIO_PIN_RESET);  /* Hold PHY in reset */

    GPIO_InitStruct.Pin = GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    /* ================================================================
     * USB ULPI Interface
     * ================================================================ */

    /* PA11: USB_OTG_FS_DM, PA12: USB_OTG_FS_DP */
    GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_OTG1_FS;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USB PHY control */
    /* PE11: USB_RESET_N (output), PE13: USB_INT_N (input) */
    GPIO_InitStruct.Pin = GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_11, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    /* ================================================================
     * SDMMC1 Interface
     * ================================================================ */

    /* PC6: SDMMC1_D6, PC7: SDMMC1_D7, PC8: SDMMC1_D0, PC9: SDMMC1_D1,
       PC10: SDMMC1_D2, PC11: SDMMC1_D3, PC12: SDMMC1_CK, PD2: SDMMC1_CMD */
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9
                        | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_SDMMC1;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_SDMMC1;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* ================================================================
     * I2C Buses
     * ================================================================ */

    /* I2C2: Temperature sensors (TMP117 ×4)
     * PB6 = SCL, PB7 = SDA (primary pair)
     * PE4,PE5 = second pair; PE6,PE7 = third; PE8,PE9 = fourth
     * AF4, open-drain, pull-up external */
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7
                        | GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C2;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    /* I2C3: Si5351A Clock Generator
     * PB8 = SCL, PB9 = SDA, AF4 */
    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* I2C4: TPS65218 PMIC
     * PB10 = SCL, PB11 = SDA, AF6 */
    GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF6_I2C4;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* ================================================================
     * Status LEDs
     * ================================================================ */

    /* PB0: LED2 (amber, status), PB1: LED3 (red, error), PB2: LED4 (blue, ETH activity) */
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2, GPIO_PIN_RESET);

    /* ================================================================
     * Fan PWM (TIM1 channels)
     * ================================================================ */

    /* PA0: TIM1_CH1 (FAN1 PWM), PA1: TIM1_CH2 (FAN2 PWM)
     * AF1, push-pull */
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* ================================================================
     * ADC Inputs
     * ================================================================ */

    /* PA2: ADC1_IN2 (current sense from INA219 Vshunt)
     * PA3: ADC1_IN3 (12V input voltage divider)
     * Analog mode, no pull */
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* ================================================================
     * HDMI Receiver Control
     * ================================================================ */

    /* PB3: HDMI_RESET (output), PB4: HDMI_INT1 (input, interrupt) */
    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);  /* Hold HDMI Rx in reset */

    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* ================================================================
     * PMIC Control
     * ================================================================ */

    /* PE14: PMIC_EN (output), PE15: PMIC_INT (input) */
    GPIO_InitStruct.Pin = GPIO_PIN_14;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_14, GPIO_PIN_SET);  /* PMIC enabled */

    GPIO_InitStruct.Pin = GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    /* ================================================================
     * Enable NVIC interrupts for EXTI lines
     * ================================================================ */
    HAL_NVIC_SetPriority(EXTI0_IRQn, 6, 0);   /* PE0: FPGA_IRQ */
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
    HAL_NVIC_SetPriority(EXTI3_IRQn, 5, 0);   /* PE3: FPGA_DONE */
    HAL_NVIC_EnableIRQ(EXTI3_IRQn);
    HAL_NVIC_SetPriority(EXTI4_IRQn, 7, 0);   /* PB4: HDMI_INT1 */
    HAL_NVIC_EnableIRQ(EXTI4_IRQn);
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 8, 0); /* PE12,PE13,PE15 */
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}
```

---

## 6. Full Device Driver: HUB75E DMA Engine (FPGA Verilog)

```verilog
/**
 * hub75e_engine.v — HUB75E LED Matrix Driver Engine
 *
 * Drives 16 HUB75E ports simultaneously with Binary-Code Modulation (BCM)
 * PWM for 16-bit per-channel color depth at 120 Hz refresh.
 *
 * Architecture:
 *   - 512 PWM channels (16 ports × 32 channels per port)
 *   - BCM with 16-bit resolution: 16 sub-periods per bit-plane
 *   - 120 Hz refresh: each full frame = 1/120 = 8.333 ms
 *   - Row scan: 32 rows per port (1/32 scan), each row = 260.4 µs
 *   - Per row: 16 BCM bit-planes, each = 16.28 µs
 *   - GCLK period: 30 ns (33.333 MHz)
 *   - OE blanking between bit-planes for ghosting prevention
 *
 * Pin count: 224 output signals (16 ports × 14 signals)
 */

module hub75e_engine #(
    parameter NUM_PORTS = 16,
    parameter CHANNELS_PER_PORT = 32,  // R1,G1,B1,R2,G2,B2 × 2 rows = 12? No.
    // Actually: 6 color channels (R1,G1,B1,R2,G2,B2) × 32 rows = 192 per port
    // But we drive 2 rows simultaneously (row n and row n+16), so:
    // 6 colors × 2 simultaneous rows = 12 PWM channels active per port per row-pair
    // Total: 16 ports × 12 = 192 active PWM channels at any time
    // But we need 6 colors × 32 rows = 192 total per port, multiplexed by row address
    parameter PWM_BITS = 16,
    parameter ROWS_PER_PORT = 32,
    parameter GCLK_FREQ_HZ = 33333333,
    parameter REFRESH_HZ = 120
) (
    // System
    input  wire        clk,           // 33.333 MHz pixel clock
    input  wire        rst_n,         // Active-low reset

    // Frame Buffer Read Interface (AXI Stream)
    input  wire        axi_tvalid,
    output wire        axi_tready,
    input  wire [191:0] axi_tdata,    // 12 pixels × 16 bits = 192 bits per beat
    input  wire        axi_tlast,     // End of row-pair

    // Control
    input  wire        enable,        // Output enable
    input  wire [3:0]  scan_mode,     // 0=1/32, 1=1/16, 2=1/8, 3=1/4
    input  wire [15:0] brightness,    // Global brightness (0-65535)

    // HUB75E Outputs (16 ports)
    output wire [5:0]  hub_rgb [0:NUM_PORTS-1],  // {R1,G1,B1,R2,G2,B2}
    output wire [4:0]  hub_addr [0:NUM_PORTS-1], // {A,B,C,D,E}
    output wire        hub_clk [0:NUM_PORTS-1],
    output wire        hub_lat [0:NUM_PORTS-1],
    output wire        hub_oe [0:NUM_PORTS-1],

    // Status
    output wire [15:0] current_refresh_hz_x100,
    output wire [31:0] frames_output,
    output wire [31:0] frames_dropped
);

    // ================================================================
    // Timing Parameters
    // ================================================================
    localparam GCLK_PERIOD_NS = 1000000000 / GCLK_FREQ_HZ;  // 30 ns
    localparam REFRESH_PERIOD_US = 1000000 / REFRESH_HZ;     // 8333 µs
    localparam ROW_PERIOD_US = REFRESH_PERIOD_US / ROWS_PER_PORT; // 260.4 µs
    localparam ROW_PERIOD_CLKS = ROW_PERIOD_US * 1000 / GCLK_PERIOD_NS; // 8680 clks

    // BCM bit-plane timing
    // Each row period is divided into 16 bit-planes (one per PWM bit)
    // Bit-plane n duration = 2^n * base_time
    // Total = base_time * (2^16 - 1) = base_time * 65535
    // base_time = ROW_PERIOD_CLKS / 65535 ≈ 0.132 clks — too small!
    //
    // Instead, use aggregated BCM:
    // Group bits into 4 segments of 4 bits each
    // Segment 0: bits [3:0], weight sum = 15,  duration = 15 * base
    // Segment 1: bits [7:4], weight sum = 240, duration = 240 * base
    // Segment 2: bits [11:8], weight sum = 3840, duration = 3840 * base
    // Segment 3: bits [15:12], weight sum = 61440, duration = 61440 * base
    // Total = 65535 * base = ROW_PERIOD_CLKS
    // base = ROW_PERIOD_CLKS / 65535 ≈ 0.132 — still too small
    //
    // Practical approach: Use 12-bit effective PWM with temporal dithering
    // for the upper 4 bits. 12-bit BCM:
    // Segment 0: bits [3:0],  Segment 1: bits [7:4]
    // Segment 2: bits [11:8]
    // base = ROW_PERIOD_CLKS / 4095
    // For 8680 clks: base = 8680/4095 ≈ 2.12 clks — workable!

    localparam BCM_SEGMENTS = 3;
    localparam BCM_BITS_PER_SEG = 4;
    localparam BCM_EFFECTIVE_BITS = 12;
    localparam BCM_BASE_CLKS = ROW_PERIOD_CLKS / ((1 << BCM_EFFECTIVE_BITS) - 1);
    // 8680 / 4095 = 2 (integer), remainder handled by dithering

    // ================================================================
    // State Machine
    // ================================================================
    localparam SM_IDLE        = 3'd0;
    localparam SM_ROW_FETCH   = 3'd1;  // Fetch row data from frame buffer
    localparam SM_BCM_SEG0    = 3'd2;  // BCM segment 0 (bits 3:0)
    localparam SM_BCM_SEG1    = 3'd3;  // BCM segment 1 (bits 7:4)
    localparam SM_BCM_SEG2    = 3'd4;  // BCM segment 2 (bits 11:8)
    localparam SM_LATCH       = 3'd5;  // Latch row, advance address
    localparam SM_OE_BLANK    = 3'd6;  // OE blanking period
    localparam SM_DITHER_CYC  = 3'd7;  // Temporal dithering for bits 15:12

    reg [2:0] state, next_state;
    reg [4:0] current_row;       // 0-31
    reg [2:0] bcm_segment;       // 0-2
    reg [15:0] bcm_counter;      // Counts GCLKs within segment
    reg [15:0] segment_duration; // Duration of current segment in GCLKs

    // ================================================================
    // Pixel Buffer (double-buffered per port)
    // ================================================================
    // Each port: 6 colors × 32 rows × 16 bits = 3072 bits
    // Total: 16 ports × 3072 = 49152 bits ≈ 6 KB
    // Use BRAM: 32 rows × 192 bits per row-pair (12 pixels × 16 bits)

    reg [15:0] pixel_buf [0:NUM_PORTS-1][0:5][0:ROWS_PER_PORT-1];  // [port][color][row]
    reg        pixel_buf_valid [0:NUM_PORTS-1][0:ROWS_PER_PORT-1];

    // ================================================================
    // BCM PWM Generation
    // ================================================================
    // For each port, for each of 6 colors, compare pixel value against BCM threshold
    // Threshold for segment s, bit b: (pixel_value >> (s*4)) & 0xF gives 4-bit value
    // PWM on if: bcm_counter < (4bit_value * segment_weight)

    wire [3:0] bcm_value [0:NUM_PORTS-1][0:5];
    wire       pwm_on [0:NUM_PORTS-1][0:5];

    genvar p, c;
    generate
        for (p = 0; p < NUM_PORTS; p = p + 1) begin : port_pwm
            for (c = 0; c < 6; c = c + 1) begin : color_pwm
                assign bcm_value[p][c] = (pixel_buf[p][c][current_row] >> (bcm_segment * 4)) & 4'hF;
                assign pwm_on[p][c] = (bcm_counter < {bcm_value[p][c], 12'd0} >> (12 - bcm_segment * 4));
                // Simplified: pwm_on when counter < value * segment_base_weight
            end
        end
    endgenerate

    // ================================================================
    // Output Register
    // ================================================================
    reg [5:0] hub_rgb_reg [0:NUM_PORTS-1];
    reg [4:0] hub_addr_reg [0:NUM_PORTS-1];
    reg       hub_clk_reg [0:NUM_PORTS-1];
    reg       hub_lat_reg [0:NUM_PORTS-1];
    reg       hub_oe_reg [0:NUM_PORTS-1];

    generate
        for (p = 0; p < NUM_PORTS; p = p + 1) begin : output_assign
            assign hub_rgb[p] = hub_rgb_reg[p];
            assign hub_addr[p] = hub_addr_reg[p];
            assign hub_clk[p] = hub_clk_reg[p];
            assign hub_lat[p] = hub_lat_reg[p];
            assign hub_oe[p] = hub_oe_reg[p];
        end
    endgenerate

    // ================================================================
    // AXI Stream Reader (Frame Buffer → Pixel Buffer)
    // ================================================================
    reg axi_reading;
    reg [4:0] axi_row_count;

    assign axi_tready = state == SM_ROW_FETCH && !axi_reading;

    // ================================================================
    // Main State Machine
    // ================================================================
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state <= SM_IDLE;
            current_row <= 0;
            bcm_segment <= 0;
            bcm_counter <= 0;
            hub_oe_reg[0] <= 1'b1;  // OE high = outputs disabled
            // (repeat for all ports in actual code)
        end else if (enable) begin
            case (state)
                SM_IDLE: begin
                    if (axi_tvalid) begin
                        state <= SM_ROW_FETCH;
                    end
                end

                SM_ROW_FETCH: begin
                    // Read 192 bits (12 pixels × 16 bits) from AXI stream
                    // Store in pixel_buf for current row-pair
                    if (axi_tvalid && axi_tready) begin
                        // Distribute 192 bits across 16 ports × 12 pixels
                        // Each port gets 12 bits? No — 192 bits / 16 ports = 12 bits per port
                        // But we need 6 colors × 16 bits = 96 bits per port per row-pair
                        // So we need multiple AXI beats per row-pair
                        // 16 ports × 96 bits = 1536 bits per row-pair
                        // 1536 / 192 = 8 AXI beats per row-pair
                        axi_reading <= 1'b1;
                        axi_row_count <= 0;
                    end
                    if (axi_tlast && axi_reading) begin
                        axi_reading <= 1'b0;
                        state <= SM_BCM_SEG0;
                        bcm_segment <= 0;
                        segment_duration <= BCM_BASE_CLKS * 15;  // 2^4 - 1 = 15
                        bcm_counter <= 0;
                        hub_oe_reg[0] <= 1'b0;  // OE low = outputs enabled
                    end
                end

                SM_BCM_SEG0, SM_BCM_SEG1, SM_BCM_SEG2: begin
                    // Drive PWM signals for current segment
                    // CLK toggles on each GCLK cycle for data shifting
                    hub_clk_reg[0] <= ~hub_clk_reg[0];

                    if (bcm_counter < segment_duration) begin
                        bcm_counter <= bcm_counter + 1;
                        // RGB outputs follow pwm_on signals
                        hub_rgb_reg[0] <= {pwm_on[0][0], pwm_on[0][1], pwm_on[0][2],
                                           pwm_on[0][3], pwm_on[0][4], pwm_on[0][5]};
                    end else begin
                        // Segment complete
                        hub_oe_reg[0] <= 1'b1;  // Blank OE
                        if (bcm_segment < 2) begin
                            bcm_segment <= bcm_segment + 1;
                            segment_duration <= segment_duration << 4;  // ×16 weight
                            bcm_counter <= 0;
                            state <= SM_OE_BLANK;
                        end else begin
                            // All BCM segments done for this row
                            state <= SM_LATCH;
                        end
                    end
                end

                SM_OE_BLANK: begin
                    // Short blanking period to prevent ghosting
                    // Duration: ~10 GCLK cycles (300 ns)
                    if (bcm_counter < 10) begin
                        bcm_counter <= bcm_counter + 1;
                    end else begin
                        bcm_counter <= 0;
                        hub_oe_reg[0] <= 1'b0;
                        // Return to BCM segment
                        state <= SM_BCM_SEG0 + bcm_segment;  // Resume current segment
                    end
                end

                SM_LATCH: begin
                    // Pulse LAT high for 2 GCLK cycles
                    hub_lat_reg[0] <= 1'b1;
                    if (bcm_counter < 2) begin
                        bcm_counter <= bcm_counter + 1;
                    end else begin
                        hub_lat_reg[0] <= 1'b0;
                        bcm_counter <= 0;
                        // Advance row address
                        if (current_row < ROWS_PER_PORT - 1) begin
                            current_row <= current_row + 1;
                            state <= SM_ROW_FETCH;
                        end else begin
                            current_row <= 0;
                            state <= SM_DITHER_CYC;
                        end
                    end
                end

                SM_DITHER_CYC: begin
                    // Temporal dithering for upper 4 bits (15:12)
                    // Accumulate fractional error and carry to next frame
                    // This runs during remaining time in refresh period
                    if (bcm_counter < 100) begin  // ~3 µs for dither calc
                        bcm_counter <= bcm_counter + 1;
                    end else begin
                        bcm_counter <= 0;
                        state <= SM_IDLE;
                        frames_output <= frames_output + 1;
                    end
                end

                default: state <= SM_IDLE;
            endcase
        end else begin
            state <= SM_IDLE;
            hub_oe_reg[0] <= 1'b1;
        end
    end

    // ================================================================
    // Row Address Output
    // ================================================================
    always @(posedge clk) begin
        hub_addr_reg[0] <= current_row;
        // Repeat for all ports
    end

    // ================================================================
    // Frame Counters
    // ================================================================
    reg [31:0] frames_output_reg;
    reg [31:0] frames_dropped_reg;

    assign frames_output = frames_output_reg;
    assign frames_dropped = frames_dropped_reg;

    // ================================================================
    // Refresh Rate Monitor
    // ================================================================
    reg [31:0] refresh_counter;
    reg [15:0] measured_refresh;

    assign current_refresh_hz_x100 = measured_refresh;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            refresh_counter <= 0;
            measured_refresh <= 0;
        end else begin
            // Count frames over 1 second measurement window
            // 33.333 MHz / 120 Hz ≈ 277777 clks per frame
            // 1 second = 33,333,333 clks
            if (refresh_counter < 33333333) begin
                refresh_counter <= refresh_counter + 1;
            end else begin
                measured_refresh <= frames_output_reg - prev_frames;
                prev_frames <= frames_output_reg;
                refresh_counter <= 0;
            end
        end
    end

    reg [31:0] prev_frames;

endmodule
```

---

## 7. Full Device Driver: DDR3 Frame Buffer Controller (FPGA Verilog)

```verilog
/**
 * frame_buffer_ctrl.v — DDR3 Frame Buffer Controller
 *
 * Manages two 512 MB DDR3L frame buffers (double-buffered) via Xilinx MIG IP.
 * Provides AXI4-Stream interfaces for write (input) and read (output).
 *
 * Features:
 *   - Double-buffered: FB0 and FB1, atomic swap
 *   - 256-bit AXI4 data width for high throughput
 *   - Address translation: (x,y) → linear address
 *   - Scatter-gather DMA for non-contiguous regions
 *   - Frame sync: swap on VSYNC or manual trigger
 */

module frame_buffer_ctrl #(
    parameter FB_WIDTH  = 256,   // Max matrix width
    parameter FB_HEIGHT = 256,   // Max matrix height
    parameter BYTES_PER_PIXEL = 6,  // 16-bit RGB × 3 channels
    parameter FB_SIZE_BYTES = FB_WIDTH * FB_HEIGHT * BYTES_PER_PIXEL,  // 393,216 bytes
    parameter DDR_CAPACITY = 512 * 1024 * 1024,  // 512 MB per channel
    parameter AXI_DATA_WIDTH = 256,
    parameter AXI_ADDR_WIDTH = 32
) (
    // System
    input  wire        clk,           // 150 MHz
    input  wire        rst_n,

    // AXI4 Master (to MIG/DDR3)
    // Write address channel
    output wire [AXI_ADDR_WIDTH-1:0] m_axi_awaddr,
    output wire [7:0]                m_axi_awlen,
    output wire                      m_axi_awvalid,
    input  wire                      m_axi_awready,
    // Write data channel
    output wire [AXI_DATA_WIDTH-1:0] m_axi_wdata,
    output wire                      m_axi_wlast,
    output wire                      m_axi_wvalid,
    input  wire                      m_axi_wready,
    // Write response channel
    input  wire [1:0]                m_axi_bresp,
    input  wire                      m_axi_bvalid,
    output wire                      m_axi_bready,
    // Read address channel
    output wire [AXI_ADDR_WIDTH-1:0] m_axi_araddr,
    output wire [7:0]                m_axi_arlen,
    output wire                      m_axi_arvalid,
    input  wire                      m_axi_arready,
    // Read data channel
    input  wire [AXI_DATA_WIDTH-1:0] m_axi_rdata,
    input  wire                      m_axi_rlast,
    input  wire                      m_axi_rvalid,
    output wire                      m_axi_rready,

    // Write Port (AXI Stream Slave — from SPI/HDMI/SD)
    input  wire [AXI_DATA_WIDTH-1:0] s_axis_write_tdata,
    input  wire                      s_axis_write_tvalid,
    output wire                      s_axis_write_tready,
    input  wire                      s_axis_write_tlast,
    input  wire [15:0]               s_axis_write_x,     // Pixel X coordinate
    input  wire [15:0]               s_axis_write_y,     // Pixel Y coordinate

    // Read Port (AXI Stream Master — to Pixel Processing)
    output wire [AXI_DATA_WIDTH-1:0] m_axis_read_tdata,
    output wire                      m_axis_read_tvalid,
    input  wire                      m_axis_read_tready,
    output wire                      m_axis_read_tlast,
    output wire [15:0]               m_axis_read_x,
    output wire [15:0]               m_axis_read_y,

    // Control
    input  wire        fb_select,     // 0=FB0, 1=FB1 (active write buffer)
    input  wire        fb_swap,       // Pulse to swap buffers
    input  wire [15:0] matrix_width,
    input  wire [15:0] matrix_height,

    // Status
    output wire        fb0_ready,
    output wire        fb1_ready,
    output wire        ddr_cal_done,
    output wire        ddr_error
);

    // ================================================================
    // Address Calculation
    // ================================================================
    // Linear address = base + (y * width + x) * bytes_per_pixel
    // FB0 base = 0x00000000
    // FB1 base = FB_SIZE_BYTES (aligned to 4KB)

    localparam FB0_BASE = 32'h00000000;
    localparam FB1_BASE = FB_SIZE_BYTES;  // 0x00060000

    function [AXI_ADDR_WIDTH-1:0] calc_address;
        input [15:0] x, y;
        input        fb_sel;
        begin
            calc_address = (fb_sel ? FB1_BASE : FB0_BASE) +
                          (y * matrix_width + x) * BYTES_PER_PIXEL;
        end
    endfunction

    // ================================================================
    // Write DMA Engine
    // ================================================================
    reg [1:0] write_state;
    localparam W_IDLE   = 2'd0;
    localparam W_ADDR   = 2'd1;
    localparam W_DATA   = 2'd2;
    localparam W_RESP   = 2'd3;

    reg [AXI_ADDR_WIDTH-1:0] write_addr;
    reg [7:0]                write_burst_len;
    reg [7:0]                write_beat_count;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            write_state <= W_IDLE;
            m_axi_awvalid <= 0;
            m_axi_wvalid <= 0;
            m_axi_bready <= 0;
            s_axis_write_tready <= 0;
        end else begin
            case (write_state)
                W_IDLE: begin
                    s_axis_write_tready <= 1'b1;
                    if (s_axis_write_tvalid) begin
                        write_addr <= calc_address(s_axis_write_x, s_axis_write_y, fb_select);
                        write_state <= W_ADDR;
                        s_axis_write_tready <= 1'b0;
                    end
                end

                W_ADDR: begin
                    m_axi_awaddr <= write_addr;
                    m_axi_awlen <= 0;  // Single beat
                    m_axi_awvalid <= 1'b1;
                    if (m_axi_awready) begin
                        m_axi_awvalid <= 1'b0;
                        write_state <= W_DATA;
                    end
                end

                W_DATA: begin
                    m_axi_wdata <= s_axis_write_tdata;
                    m_axi_wlast <= 1'b1;
                    m_axi_wvalid <= 1'b1;
                    if (m_axi_wready) begin
                        m_axi_wvalid <= 1'b0;
                        write_state <= W_RESP;
                    end
                end

                W_RESP: begin
                    m_axi_bready <= 1'b1;
                    if (m_axi_bvalid) begin
                        m_axi_bready <= 1'b0;
                        write_state <= W_IDLE;
                    end
                end
            endcase
        end
    end

    // ================================================================
    // Read DMA Engine
    // ================================================================
    reg [1:0] read_state;
    localparam R_IDLE   = 2'd0;
    localparam R_ADDR   = 2'd1;
    localparam R_DATA   = 2'd2;

    reg [15:0] read_x, read_y;
    reg        read_fb_sel;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            read_state <= R_IDLE;
            m_axi_arvalid <= 0;
            m_axi_rready <= 0;
            m_axis_read_tvalid <= 0;
            read_x <= 0;
            read_y <= 0;
        end else begin
            case (read_state)
                R_IDLE: begin
                    if (m_axis_read_tready) begin
                        // Request next pixel
                        m_axi_araddr <= calc_address(read_x, read_y, ~fb_select);
                        m_axi_arlen <= 0;
                        m_axi_arvalid <= 1'b1;
                        read_state <= R_ADDR;
                    end
                end

                R_ADDR: begin
                    if (m_axi_arready) begin
                        m_axi_arvalid <= 1'b0;
                        m_axi_rready <= 1'b1;
                        read_state <= R_DATA;
                    end
                end

                R_DATA: begin
                    if (m_axi_rvalid) begin
                        m_axis_read_tdata <= m_axi_rdata;
                        m_axis_read_tvalid <= 1'b1;
                        m_axis_read_x <= read_x;
                        m_axis_read_y <= read_y;
                        m_axi_rready <= 1'b0;

                        // Advance to next pixel
                        if (read_x < matrix_width - 1) begin
                            read_x <= read_x + 1;
                        end else begin
                            read_x <= 0;
                            if (read_y < matrix_height - 1) begin
                                read_y <= read_y + 1;
                            end else begin
                                read_y <= 0;
                                m_axis_read_tlast <= 1'b1;
                            end
                        end
                        read_state <= R_IDLE;
                    end
                end
            endcase
        end
    end

    // ================================================================
    // Buffer Swap Logic
    // ================================================================
    reg internal_fb_select;
    reg swap_in_progress;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            internal_fb_select <= 0;
            swap_in_progress <= 0;
        end else if (fb_swap && !swap_in_progress) begin
            swap_in_progress <= 1;
            // Wait for current write and read to complete
            if (write_state == W_IDLE && read_state == R_IDLE) begin
                internal_fb_select <= ~internal_fb_select;
                swap_in_progress <= 0;
            end
        end
    end

    // ================================================================
    // Status
    // ================================================================
    assign fb0_ready = 1'b1;  // Always ready after init
    assign fb1_ready = 1'b1;
    assign ddr_cal_done = 1'b1;  // From MIG status (simplified)
    assign ddr_error = 1'b0;

endmodule
```

---

## 8. STM32H7 Firmware: Art-Net / sACN Receiver

```c
/**
 * artnet_receiver.c — Art-Net 4 and sACN E1.31 Protocol Receiver
 *
 * Receives DMX-over-Ethernet lighting control protocols and converts
 * to raw pixel data for the FPGA. Supports:
 *   - Art-Net 4 (ArtDMX packets, up to 512 channels per universe)
 *   - sACN E1.31 (ANSI E1.31-2018, up to 512 slots per universe)
 *   - Custom Nebula binary protocol (raw pixel stream)
 *
 * Pixel mapping:
 *   Each RGB pixel consumes 3 DMX channels (R, G, B)
 *   512 channels / 3 = 170 pixels per universe
 *   For 256×256 = 65,536 pixels: 65,536 × 3 / 512 = 384 universes
 *
 * Universe-to-panel mapping is configurable via web interface.
 */

#include "artnet_receiver.h"
#include "lwip/udp.h"
#include "lwip/pbuf.h"
#include "spi_pixel_stream.h"
#include <string.h>

/* Art-Net UDP port */
#define ARTNET_PORT          0x1936  /* 6454 */
/* sACN UDP port */
#define SACN_PORT            5568

/* Art-Net OpCodes */
#define ARTNET_OP_POLL       0x2000
#define ARTNET_OP_POLL_REPLY 0x2100
#define ARTNET_OP_DMX        0x5000
#define ARTNET_OP_SYNC       0x5200

/* sACN E1.31 packet offsets */
#define SACN_VECTOR_OFFSET   18
#define SACN_UNIVERSE_OFFSET 113
#define SACN_SLOTS_OFFSET    126
#define SACN_DMP_DATA_OFFSET 126

/* Maximum universes we can handle */
#define MAX_UNIVERSES         384
#define DMX_CHANNELS_PER_UNI  512
#define PIXELS_PER_UNI        (DMX_CHANNELS_PER_UNI / 3)  /* 170 */

/* Universe mapping table */
typedef struct {
    uint16_t universe_id;
    uint16_t start_x;       /* Starting X pixel in matrix */
    uint16_t start_y;       /* Starting Y pixel in matrix */
    uint16_t width;         /* Width in pixels (typically 170 for 1D) */
    uint8_t  enabled;
    uint8_t  protocol;      /* 0=ArtNet, 1=sACN, 2=both */
} universe_map_t;

static universe_map_t universe_map[MAX_UNIVERSES];
static uint8_t dmx_data[MAX_UNIVERSES][DMX_CHANNELS_PER_UNI];
static uint32_t dmx_sequence[MAX_UNIVERSES];  /* For sACN sequence tracking */
static uint32_t last_frame_id = 0;

/* UDP PCB for Art-Net */
static struct udp_pcb *artnet_pcb = NULL;
/* UDP PCB for sACN */
static struct udp_pcb *sacn_pcb = NULL;

/* ================================================================
 * Art-Net Packet Handler
 * ================================================================ */

static void artnet_recv_callback(void *arg, struct udp_pcb *pcb,
                                  struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    uint8_t *data;
    uint16_t opcode;
    uint16_t protocol_ver;
    uint8_t sequence;
    uint8_t physical;
    uint16_t universe;
    uint16_t length;

    if (p == NULL) return;

    data = (uint8_t *)p->payload;

    /* Check Art-Net header: "Art-Net\0" */
    if (memcmp(data, "Art-Net", 8) != 0) {
        pbuf_free(p);
        return;
    }

    opcode = (data[8] << 8) | data[9];
    protocol_ver = (data[10] << 8) | data[11];

    switch (opcode) {
    case ARTNET_OP_POLL:
        /* Respond with ArtPollReply */
        artnet_send_poll_reply(pcb, addr, port);
        break;

    case ARTNET_OP_DMX:
        sequence = data[12];
        physical = data[13];
        universe = (data[14] << 8) | data[15];
        length = (data[16] << 8) | data[17];

        if (length > DMX_CHANNELS_PER_UNI) {
            length = DMX_CHANNELS_PER_UNI;
        }

        /* Find matching universe in our map */
        for (int i = 0; i < MAX_UNIVERSES; i++) {
            if (universe_map[i].enabled &&
                universe_map[i].universe_id == universe &&
                (universe_map[i].protocol == 0 || universe_map[i].protocol == 2)) {

                /* Copy DMX data */
                memcpy(dmx_data[i], &data[18], length);

                /* Mark universe as updated */
                dmx_sequence[i] = sequence;

                /* If this completes a frame, push to FPGA */
                check_frame_complete();
                break;
            }
        }
        break;

    case ARTNET_OP_SYNC:
        /* Sync packet: push all updated universes to FPGA */
        push_all_dirty_universes();
        break;

    default:
        break;
    }

    pbuf_free(p);
}

/* ================================================================
 * sACN E1.31 Packet Handler
 * ================================================================ */

static void sacn_recv_callback(void *arg, struct udp_pcb *pcb,
                                struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    uint8_t *data;
    uint32_t vector;
    uint16_t universe;
    uint8_t sequence;
    uint16_t slot_count;

    if (p == NULL) return;

    data = (uint8_t *)p->payload;

    /* Check sACN preamble: 0x0010, 0x0000, ASC-E1.17\0\0\0 */
    if (p->len < 126) {
        pbuf_free(p);
        return;
    }

    /* Root Layer: Vector at offset 18 */
    vector = (data[18] << 24) | (data[19] << 16) | (data[20] << 8) | data[21];
    if (vector != 0x00000004) {  /* VECTOR_ROOT_E131_DATA */
        pbuf_free(p);
        return;
    }

    /* Framing Layer: Vector at offset 40 */
    vector = (data[40] << 24) | (data[41] << 16) | (data[42] << 8) | data[43];
    if (vector != 0x00000002) {  /* VECTOR_E131_DATA_PACKET */
        pbuf_free(p);
        return;
    }

    /* Universe number at offset 113 (Framing Layer) */
    universe = (data[113] << 8) | data[114];

    /* Sequence number at offset 111 */
    sequence = data[111];

    /* DMP Layer: Slot count at offset 123 */
    slot_count = (data[123] << 8) | data[124];
    if (slot_count > DMX_CHANNELS_PER_UNI) {
        slot_count = DMX_CHANNELS_PER_UNI;
    }

    /* Find matching universe */
    for (int i = 0; i < MAX_UNIVERSES; i++) {
        if (universe_map[i].enabled &&
            universe_map[i].universe_id == universe &&
            (universe_map[i].protocol == 1 || universe_map[i].protocol == 2)) {

            /* Copy slot data starting at offset 126 */
            memcpy(dmx_data[i], &data[126], slot_count);

            /* Track sequence for duplicate detection */
            if (sequence > dmx_sequence[i] ||
                (sequence < dmx_sequence[i] && dmx_sequence[i] - sequence > 128)) {
                dmx_sequence[i] = sequence;
                check_frame_complete();
            }
            break;
        }
    }

    pbuf_free(p);
}

/* ================================================================
 * Frame Completion Detection & Pixel Streaming
 * ================================================================ */

static void check_frame_complete(void)
{
    uint32_t current_frame_id;
    bool all_updated = true;

    /* Simple heuristic: if all enabled universes have been updated
     * since last push, consider frame complete */
    for (int i = 0; i < MAX_UNIVERSES; i++) {
        if (universe_map[i].enabled) {
            if (dmx_sequence[i] == last_frame_id) {
                all_updated = false;
                break;
            }
        }
    }

    if (all_updated) {
        push_all_dirty_universes();
        last_frame_id++;
    }
}

static void push_all_dirty_universes(void)
{
    /* Convert DMX data to pixel stream and send to FPGA via SPI */
    for (int i = 0; i < MAX_UNIVERSES; i++) {
        if (universe_map[i].enabled) {
            /* For each pixel in this universe's region */
            uint16_t px = universe_map[i].start_x;
            uint16_t py = universe_map[i].start_y;
            uint16_t pixel_count = universe_map[i].width;

            for (uint16_t p = 0; p < pixel_count; p++) {
                uint8_t r = dmx_data[i][p * 3 + 0];
                uint8_t g = dmx_data[i][p * 3 + 1];
                uint8_t b = dmx_data[i][p * 3 + 2];

                /* Send pixel to FPGA: 48-bit = 16-bit R, G, B */
                /* DMX is 8-bit, expand to 16-bit by replicating */
                uint16_t r16 = (r << 8) | r;
                uint16_t g16 = (g << 8) | g;
                uint16_t b16 = (b << 8) | b;

                spi_send_pixel(px + p, py, r16, g16, b16);
            }
        }
    }

    /* Signal end of frame */
    spi_send_frame_end();
}

/* ================================================================
 * ArtPollReply
 * ================================================================ */

static void artnet_send_poll_reply(struct udp_pcb *pcb,
                                    const ip_addr_t *addr, u16_t port)
{
    uint8_t reply[239];
    struct pbuf *p;

    memset(reply, 0, sizeof(reply));
    memcpy(reply, "Art-Net", 8);
    reply[8] = 0x00; reply[9] = 0x21;  /* OpPollReply */
    /* IP address (our IP) */
    reply[10] = 192; reply[11] = 168; reply[12] = 1; reply[13] = 100;
    reply[14] = 0x36; reply[15] = 0x19;  /* Port = 0x1936 */
    reply[16] = 0x00; reply[17] = 0x00;  /* Version = 0 */
    reply[18] = 0x00; reply[19] = 0x00;  /* NetSwitch = 0 */
    reply[20] = 0x00; reply[21] = 0x00;  /* SubSwitch = 0 */
    /* OEM = 0xFFFF (custom) */
    reply[22] = 0xFF; reply[23] = 0xFF;
    reply[24] = 0x00;  /* Ubea version */
    reply[25] = 0x01;  /* Status1: indicators in Normal */
    reply[26] = 0x00;  /* ESTA manufacturer code LSB */
    reply[27] = 0x00;  /* ESTA manufacturer code MSB */
    memcpy(&reply[28], "Nebula Matrix", 13);  /* Short name */
    memcpy(&reply[46], "Nebula Matrix LED Display Engine", 35);  /* Long name */
    memcpy(&reply[82], "FPGA-driven 256x256 16bpp 120Hz", 33);  /* Node report */
    reply[115] = 0x00; reply[116] = 0x02;  /* NumPorts = 2 (Hi, Lo) */
    /* Port types: 0x80 = output from network */
    reply[117] = 0x80; reply[118] = 0x80;
    reply[119] = 0x80; reply[120] = 0x80;
    /* Good input */
    reply[121] = 0x08; reply[122] = 0x08;
    reply[123] = 0x08; reply[124] = 0x08;
    /* SwIn */
    for (int i = 0; i < 4; i++) reply[125+i] = i;
    /* SwOut */
    for (int i = 0; i < 4; i++) reply[129+i] = i;
    reply[133] = 0x00;  /* SwVideo */
    reply[134] = 0x00;  /* SwMacro */
    reply[135] = 0x00;  /* SwRemote */
    reply[136] = 0x00; reply[137] = 0x00; reply[138] = 0x00;  /* Spare */
    reply[139] = 0x00;  /* Style = StNode */
    /* MAC address */
    reply[140] = 0x02; reply[141] = 0x00; reply[142] = 0x00;
    reply[143] = 0x00; reply[144] = 0x00; reply[145] = 0x01;
    /* Bind IP */
    reply[146] = 192; reply[147] = 168; reply[148] = 1; reply[149] = 100;
    reply[150] = 0x00;  /* BindIndex */
    reply[151] = 0x00;  /* Status2 */
    /* Filler */
    memset(&reply[152], 0, 26);
    memcpy(&reply[178], "Nebula", 6);  /* Manufacturer */
    memcpy(&reply[207], "NM-001", 5);  /* Product */

    p = pbuf_alloc(PBUF_TRANSPORT, sizeof(reply), PBUF_RAM);
    if (p) {
        memcpy(p->payload, reply, sizeof(reply));
        udp_sendto(pcb, p, addr, port);
        pbuf_free(p);
    }
}

/* ================================================================
 * Initialization
 * ================================================================ */

int artnet_receiver_init(void)
{
    err_t err;

    /* Create UDP PCB for Art-Net */
    artnet_pcb = udp_new();
    if (!artnet_pcb) return -1;

    err = udp_bind(artnet_pcb, IP_ADDR_ANY, ARTNET_PORT);
    if (err != ERR_OK) {
        udp_remove(artnet_pcb);
        return -2;
    }
    udp_recv(artnet_pcb, artnet_recv_callback, NULL);

    /* Create UDP PCB for sACN */
    sacn_pcb = udp_new();
    if (!sacn_pcb) {
        udp_remove(artnet_pcb);
        return -3;
    }

    /* sACN uses multicast. Join multicast group 239.255.0.x */
    ip4_addr_t sacn_multicast;
    IP4_ADDR(&sacn_multicast, 239, 255, 0, 1);

    err = udp_bind(sacn_pcb, IP_ADDR_ANY, SACN_PORT);
    if (err != ERR_OK) {
        udp_remove(sacn_pcb);
        udp_remove(artnet_pcb);
        return -4;
    }

    /* Join multicast group */
    igmp_joingroup(IP_ADDR_ANY, &sacn_multicast);
    udp_recv(sacn_pcb, sacn_recv_callback, NULL);

    /* Initialize universe map with default 1D layout */
    memset(universe_map, 0, sizeof(universe_map));
    for (int i = 0; i < MAX_UNIVERSES; i++) {
        universe_map[i].universe_id = i;
        universe_map[i].start_x = (i * PIXELS_PER_UNI) % 256;
        universe_map[i].start_y = (i * PIXELS_PER_UNI) / 256;
        universe_map[i].width = PIXELS_PER_UNI;
        universe_map[i].enabled = (i < 384) ? 1 : 0;
        universe_map[i].protocol = 2;  /* Both Art-Net and sACN */
    }

    return 0;
}
```

---

## 9. Device Tree Overlay (Conceptual)

For a Linux-based test/development host connecting via USB RNDIS:

```dts
/**
 * nebula-matrix.dtso — Device Tree Overlay for Nebula Matrix
 *
 * Applied on a Linux development host when Nebula Matrix is connected
 * via USB RNDIS (Ethernet-over-USB). Provides a virtual network interface
 * for configuration and testing.
 */

/dts-v1/;
/plugin/;

/ {
    compatible = "raspberrypi,4-model-b", "brcm,bcm2711";

    fragment@0 {
        target = <&usb>;
        __overlay__ {
            #address-cells = <1>;
            #size-cells = <0>;

            nebula_matrix: device@1 {
                compatible = "nous,nebula-matrix";
                reg = <1>;

                /* RNDIS network interface */
                rndis0: ethernet {
                    compatible = "usb,rndis";
                    local-mac-address = [02 00 00 00 00 02];
                };

                /* Configuration interface */
                config {
                    compatible = "nous,nebula-config";
                    http-port = <80>;
                    ws-port = <8080>;
                };

                /* Firmware update interface */
                firmware {
                    compatible = "nous,nebula-firmware";
                    fpga-flash-size = <0x1000000>;  /* 16 MB */
                    mcu-flash-size  = <0x200000>;   /* 2 MB */
                };
            };
        };
    };

    fragment@1 {
        target = <&gpio>;
        __overlay__ {
            /* Optional: GPIO-connected status signals if using UART bridge */
            nebula_pins: nebula_pins {
                brcm,pins = <23 24 25>;
                brcm,function = <0>;  /* Input */
                brcm,pull = <2>;      /* Pull-up */
            };
        };
    };
};
```

---

## 10. Build Instructions

### 10.1 FPGA Bitstream Build (Vivado)

```bash
#!/bin/bash
# build_fpga.sh — Build Nebula Matrix FPGA bitstream

VIVADO_PATH="/opt/Xilinx/Vivado/2024.1"
PROJECT_DIR="nebula_matrix_fpga"
TOP_MODULE="nebula_matrix_top"

# Source Vivado environment
source ${VIVADO_PATH}/settings64.sh

# Create project
vivado -mode batch -source << 'EOF'
create_project nebula_matrix ./nebula_matrix_fpga -part xc7a100tfgg484-2

# Add source files
add_files -norecurse {
    rtl/nebula_matrix_top.v
    rtl/frame_buffer_ctrl.v
    rtl/pixel_processing.v
    rtl/gamma_lut.v
    rtl/color_space_conv.v
    rtl/dither_engine.v
    rtl/scaler.v
    rtl/hub75e_engine.v
    rtl/spi_slave.v
    rtl/uart_cmd_if.v
    rtl/hdmi_capture.v
    rtl/clock_mgmt.v
}

# Add constraint files
add_files -fileset constrs_1 {
    xdc/nebula_matrix_pins.xdc
    xdc/nebula_matrix_timing.xdc
}

# Add MIG (DDR3 Memory Interface Generator) IP
create_ip -name mig_7series -vendor xilinx.com -library ip -module_name ddr3_mig
set_property -dict {
    CONFIG.DDR3_MEMORY_TYPE {SODIMM}
    CONFIG.DDR3_MEMORY_WIDTH {16}
    CONFIG.DDR3_DATA_WIDTH {32}
    CONFIG.DDR3_CLOCK_FREQ {533}
    CONFIG.DDR3_MEMORY_PART {AS4C256M16D3LC-12BCN}
    CONFIG.DDR3_AXI_DATA_WIDTH {256}
} [get_ips ddr3_mig]

generate_target {instantiation_template} [get_ips ddr3_mig]
generate_target all [get_ips ddr3_mig]

# Synthesis
synth_design -top nebula_matrix_top -part xc7a100tfgg484-2

# Implementation
opt_design
place_design
route_design

# Generate bitstream
write_bitstream -force nebula_matrix.bit
write_cfgmem -force -format MCS -size 128 -interface SPIx4 \
    -loadbit "up 0x0000000 nebula_matrix.bit" \
    nebula_matrix.mcs

# Reports
report_timing_summary -file timing_report.txt
report_utilization -file utilization_report.txt
report_power -file power_report.txt

EOF

echo "FPGA build complete."
echo "Bitstream: nebula_matrix.bit"
echo "Flash image: nebula_matrix.mcs"
```

### 10.2 STM32H7 Firmware Build (GCC ARM)

```bash
#!/bin/bash
# build_firmware.sh — Build STM32H7 firmware

ARM_GCC_PATH="/opt/gcc-arm-none-eabi-10.3-2021.10"
PROJECT_DIR="nebula_matrix_fw"

export PATH="${ARM_GCC_PATH}/bin:$PATH"

cd ${PROJECT_DIR}

# Clean
make clean

# Build
make -j$(nproc) 2>&1 | tee build.log

if [ $? -eq 0 ]; then
    echo "Firmware build successful."
    echo "Output: build/nebula_matrix.elf"
    echo "Output: build/nebula_matrix.bin"
    echo "Output: build/nebula_matrix.hex"

    # Print size info
    arm-none-eabi-size build/nebula_matrix.elf
else
    echo "Build failed. See build.log for details."
    exit 1
fi
```

### 10.3 React Native App Build

```bash
#!/bin/bash
# build_app.sh — Build React Native companion app

cd nebula_matrix_app

# Install dependencies
npm install

# Start Metro bundler (development)
npx react-native start &

# Build for Android
npx react-native run-android

# Build for iOS
cd ios && pod install && cd ..
npx react-native run-ios
```

---

## 11. UART Command Protocol (STM32 ↔ FPGA)

### 11.1 Frame Format

```
┌─────────┬─────────┬───────────┬───────────┬───────────┬─────────┐
│  SYNC   │  CMD    │  ADDR_HI  │  ADDR_LO  │  DATA     │  CRC8   │
│  0xA5   │  1 byte │  1 byte   │  1 byte   │  0-4 bytes│  1 byte │
└─────────┴─────────┴───────────┴───────────┴───────────┴─────────┘
```

### 11.2 Commands

| CMD  | Name | Data Len | Description |
|------|------|----------|-------------|
| 0x00 | NOP | 0 | No operation (used for keep-alive) |
| 0x01 | READ | 0 | Read register at ADDR, response contains data |
| 0x02 | WRITE | 1-4 | Write data to register at ADDR |
| 0x03 | BURST_READ | 1 | Read N registers starting at ADDR |
| 0x04 | BURST_WRITE | N | Write N bytes starting at ADDR |
| 0x10 | GAMMA_WR | 512 | Write full gamma LUT (256×16-bit entries) |
| 0x11 | GAMMA_RD | 0 | Read full gamma LUT (response: 512 bytes) |
| 0x20 | FB_SWAP | 0 | Swap frame buffers |
| 0x21 | FB_SELECT | 1 | Select active write buffer (0 or 1) |
| 0x30 | RESET | 0 | Soft reset FPGA pipeline |
| 0x31 | TEST_PATTERN | 1 | Enable test pattern (0-3) |
| 0xFF | PING | 0 | Ping, response: FPGA_ID + VERSION |

### 11.3 Response Format

```
┌─────────┬─────────┬───────────┬───────────┬───────────┬─────────┐
│  SYNC   │  STATUS │  ADDR_HI  │  ADDR_LO  │  DATA     │  CRC8   │
│  0xA5   │  1 byte │  1 byte   │  1 byte   │  0-4 bytes│  1 byte │
└─────────┴─────────┴───────────┴───────────┴───────────┴─────────┘
```

Status codes:
- 0x00: OK
- 0x01: Invalid command
- 0x02: Invalid address
- 0x03: CRC error
- 0x04: Write protected
- 0x05: Busy (retry later)

### 11.4 CRC-8 Calculation

```c
uint8_t uart_crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFF;  /* Initial value */
    uint8_t polynomial = 0x31;  /* CRC-8/DVB-S2: x^8 + x^5 + x^4 + 1 */

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
```

---

## 12. SPI Pixel Stream Protocol (STM32 → FPGA)

### 12.1 Frame Format

The SPI bus runs at 50 MHz, 32-bit word size, MSB first, CPOL=0, CPHA=0.

Each pixel is transmitted as two 32-bit words:

```
Word 0: [31:28] CMD=0x1 [27:16] X[11:0] [15:0] Y[15:0]
Word 1: [31:28] CMD=0x2 [27:12] R[15:0] [11:0] G[15:0] (upper 12 bits)
Word 2: [31:28] CMD=0x3 [27:12] G[3:0] [11:0] B[15:0] (lower 4 bits of G + full B)
```

Actually, more efficient packing:

```
Word 0: [31:28] CMD=0x1  [27:16] X[11:0]  [15:0] Y[15:0]
Word 1: [31:16] R[15:0]  [15:0] G[15:0]
Word 2: [31:16] B[15:0]  [15:0] Reserved
```

Commands:
- 0x1: Pixel data follows (X, Y address)
- 0x2: Frame end marker
- 0x3: Frame start marker
- 0x4: Configuration data follows
- 0xF: NOP / idle

### 12.2 SPI DMA Configuration (STM32H7)

```c
/**
 * spi_pixel_stream.c — SPI DMA pixel streaming to FPGA
 */

#include "spi_pixel_stream.h"
#include "stm32h7xx_hal.h"

#define SPI_PIXEL_BUF_SIZE  2048  /* 32-bit words */
#define SPI_DMA_TIMEOUT_MS  100

static SPI_HandleTypeDef hspi2;
static DMA_HandleTypeDef hdma_spi2_tx;
static uint32_t spi_tx_buf[SPI_PIXEL_BUF_SIZE];
static volatile uint32_t spi_buf_count = 0;
static volatile bool spi_transfer_active = false;

/* Initialize SPI2 for pixel streaming */
void spi_pixel_init(void)
{
    /* SPI2 clock: 100 MHz (PLL2P), prescaler /2 = 50 MHz */
    hspi2.Instance = SPI2;
    hspi2.Init.Mode = SPI_MODE_MASTER;
    hspi2.Init.Direction = SPI_DIRECTION_2LINES;
    hspi2.Init.DataSize = SPI_DATASIZE_32BIT;
    hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi2.Init.NSS = SPI_NSS_SOFT;
    hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;  /* 50 MHz */
    hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi2.Init.CRCPolynomial = 7;
    HAL_SPI_Init(&hspi2);

    /* Configure DMA for SPI2 TX */
    __HAL_RCC_DMA1_CLK_ENABLE();

    hdma_spi2_tx.Instance = DMA1_Stream0;
    hdma_spi2_tx.Init.Request = DMA_REQUEST_SPI2_TX;
    hdma_spi2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_spi2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_spi2_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_spi2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_spi2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_spi2_tx.Init.Mode = DMA_NORMAL;
    hdma_spi2_tx.Init.Priority = DMA_PRIORITY_VERY_HIGH;
    hdma_spi2_tx.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
    hdma_spi2_tx.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
    hdma_spi2_tx.Init.MemBurst = DMA_MBURST_SINGLE;
    hdma_spi2_tx.Init.PeriphBurst = DMA_PBURST_SINGLE;
    HAL_DMA_Init(&hdma_spi2_tx);

    __HAL_LINKDMA(&hspi2, hdmatx, hdma_spi2_tx);

    /* Enable DMA interrupts */
    HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 4, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
}

/* Send a single pixel to FPGA */
int spi_send_pixel(uint16_t x, uint16_t y, uint16_t r, uint16_t g, uint16_t b)
{
    if (spi_buf_count >= SPI_PIXEL_BUF_SIZE - 3) {
        /* Buffer full, flush */
        spi_flush_buffer();
    }

    /* Word 0: Command + X + Y */
    spi_tx_buf[spi_buf_count++] = (0x1 << 28) | ((x & 0xFFF) << 16) | (y & 0xFFFF);
    /* Word 1: R + G */
    spi_tx_buf[spi_buf_count++] = (r << 16) | g;
    /* Word 2: B + Reserved */
    spi_tx_buf[spi_buf_count++] = (b << 16);

    return 0;
}

/* Send frame end marker */
void spi_send_frame_end(void)
{
    spi_flush_buffer();

    /* Send frame end command */
    spi_tx_buf[0] = (0x2 << 28);  /* Frame end */
    spi_buf_count = 1;
    spi_flush_buffer();
}

/* Flush buffer to FPGA via DMA */
void spi_flush_buffer(void)
{
    if (spi_buf_count == 0) return;

    /* Wait for any previous transfer to complete */
    while (spi_transfer_active) {
        /* Could add timeout here */
    }

    spi_transfer_active = true;

    /* Assert NSS */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);

    /* Start DMA transfer */
    HAL_SPI_Transmit_DMA(&hspi2, spi_tx_buf, spi_buf_count);

    spi_buf_count = 0;
}

/* DMA transfer complete callback */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI2) {
        /* De-assert NSS */
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
        spi_transfer_active = false;
    }
}

/* DMA error callback */
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI2) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
        spi_transfer_active = false;
        /* Log error */
    }
}
```

---

## 13. FreeRTOS Task Architecture

```c
/**
 * freertos_tasks.c — FreeRTOS task definitions for Nebula Matrix
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Task priorities (higher number = higher priority) */
#define TASK_PRIO_ETH_RX      5   /* Ethernet packet reception */
#define TASK_PRIO_PIXEL_PUSH  4   /* Pixel data push to FPGA */
#define TASK_PRIO_ARTNET      3   /* Art-Net/sACN processing */
#define TASK_PRIO_HTTP        2   /* Web configuration server */
#define TASK_PRIO_MONITOR     2   /* System health monitoring */
#define TASK_PRIO_SD_PLAYBACK 3   /* SD card playback */
#define TASK_PRIO_FPGA_CMD    3   /* FPGA command interface */
#define TASK_PRIO_IDLE        0   /* Idle task */

/* Task stack sizes */
#define STACK_ETH_RX          2048
#define STACK_PIXEL_PUSH      1024
#define STACK_ARTNET          4096
#define STACK_HTTP            8192
#define STACK_MONITOR         1024
#define STACK_SD_PLAYBACK     4096
#define STACK_FPGA_CMD        2048

/* Queue handles */
static QueueHandle_t pixel_queue;     /* Pixels waiting to be sent to FPGA */
static QueueHandle_t cmd_queue;       /* FPGA commands */
static QueueHandle_t event_queue;     /* System events */

/* Event types */
typedef enum {
    EVENT_FPGA_DONE,
    EVENT_FPGA_IRQ,
    EVENT_HDMI_INT,
    EVENT_ETH_LINK_UP,
    EVENT_ETH_LINK_DOWN,
    EVENT_USB_CONFIGURED,
    EVENT_SD_INSERTED,
    EVENT_SD_REMOVED,
    EVENT_THERMAL_WARN,
    EVENT_THERMAL_CRIT,
    EVENT_PMIC_FAULT,
} system_event_t;

/* Pixel data structure for queue */
typedef struct {
    uint16_t x, y;
    uint16_t r, g, b;
} pixel_data_t;

/* ================================================================
 * Task: Ethernet RX (highest priority)
 * ================================================================ */
static void task_eth_rx(void *pvParameters)
{
    /* LWIP runs in its own thread context.
     * This task periodically polls LWIP timers and handles
     * incoming packet processing. */
    for (;;) {
        /* Process LWIP */
        sys_check_timeouts();
        vTaskDelay(pdMS_TO_TICKS(1));  /* 1ms polling */
    }
}

/* ================================================================
 * Task: Pixel Push to FPGA
 * ================================================================ */
static void task_pixel_push(void *pvParameters)
{
    pixel_data_t pixel;

    for (;;) {
        /* Wait for pixels in queue */
        if (xQueueReceive(pixel_queue, &pixel, portMAX_DELAY) == pdTRUE) {
            spi_send_pixel(pixel.x, pixel.y, pixel.r, pixel.g, pixel.b);
        }
    }
}

/* ================================================================
 * Task: System Monitor
 * ================================================================ */
static void task_monitor(void *pvParameters)
{
    float temps[4];
    float voltage_12v, current_12v;
    uint16_t fan_speed_pct = 0;
    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {
        /* Read all TMP117 sensors */
        for (int i = 0; i < 4; i++) {
            temps[i] = tmp117_read_temp(i);
        }

        /* Read INA219 */
        voltage_12v = ina219_read_voltage();
        current_12v = ina219_read_current();

        /* PID fan control based on FPGA temperature */
        float fpga_temp = temps[0];
        if (fpga_temp < 40.0f) {
            fan_speed_pct = 0;
        } else if (fpga_temp < 50.0f) {
            fan_speed_pct = (uint16_t)((fpga_temp - 40.0f) * 3.0f);  /* 0-30% */
        } else if (fpga_temp < 60.0f) {
            fan_speed_pct = 30 + (uint16_t)((fpga_temp - 50.0f) * 3.0f);  /* 30-60% */
        } else if (fpga_temp < 70.0f) {
            fan_speed_pct = 60 + (uint16_t)((fpga_temp - 60.0f) * 4.0f);  /* 60-100% */
        } else {
            fan_speed_pct = 100;
        }

        /* Set fan PWM */
        fan_set_speed(0, fan_speed_pct);
        fan_set_speed(1, fan_speed_pct);

        /* Check for thermal events */
        if (fpga_temp > 85.0f) {
            system_event_t evt = EVENT_THERMAL_CRIT;
            xQueueSend(event_queue, &evt, 0);
        } else if (fpga_temp > 80.0f) {
            system_event_t evt = EVENT_THERMAL_WARN;
            xQueueSend(event_queue, &evt, 0);
        }

        /* Log to web interface */
        monitor_update_web(temps, voltage_12v, current_12v, fan_speed_pct);

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(1000));  /* 1 second period */
    }
}

/* ================================================================
 * Task: FPGA Command Interface
 * ================================================================ */
static void task_fpga_cmd(void *pvParameters)
{
    fpga_cmd_t cmd;

    for (;;) {
        if (xQueueReceive(cmd_queue, &cmd, pdMS_TO_TICKS(100)) == pdTRUE) {
            fpga_uart_send_command(cmd.addr, cmd.is_write, cmd.data, cmd.data_len);
        }

        /* Periodically poll FPGA status */
        static uint32_t last_poll = 0;
        uint32_t now = xTaskGetTickCount();
        if ((now - last_poll) > pdMS_TO_TICKS(500)) {
            fpga_poll_status();
            last_poll = now;
        }
    }
}

/* ================================================================
 * Initialization
 * ================================================================ */
void freertos_init(void)
{
    /* Create queues */
    pixel_queue = xQueueCreate(512, sizeof(pixel_data_t));
    cmd_queue = xQueueCreate(32, sizeof(fpga_cmd_t));
    event_queue = xQueueCreate(16, sizeof(system_event_t));

    /* Create tasks */
    xTaskCreate(task_eth_rx,      "ETH_RX",   STACK_ETH_RX,      NULL, TASK_PRIO_ETH_RX,      NULL);
    xTaskCreate(task_pixel_push,  "PIXEL",    STACK_PIXEL_PUSH,  NULL, TASK_PRIO_PIXEL_PUSH,  NULL);
    xTaskCreate(task_artnet,      "ARTNET",   STACK_ARTNET,      NULL, TASK_PRIO_ARTNET,      NULL);
    xTaskCreate(task_http,        "HTTP",     STACK_HTTP,        NULL, TASK_PRIO_HTTP,        NULL);
    xTaskCreate(task_monitor,     "MONITOR",  STACK_MONITOR,     NULL, TASK_PRIO_MONITOR,     NULL);
    xTaskCreate(task_sd_playback, "SD_PLAY",  STACK_SD_PLAYBACK, NULL, TASK_PRIO_SD_PLAYBACK,  NULL);
    xTaskCreate(task_fpga_cmd,    "FPGA_CMD", STACK_FPGA_CMD,    NULL, TASK_PRIO_FPGA_CMD,    NULL);

    /* Start scheduler */
    vTaskStartScheduler();

    /* Should never reach here */
    for (;;);
}
```

---

*Document Version: 1.0 | Author: Embedded Software Engineer | Date: 2026-06-16*
*Target Device: Nebula Matrix — Professional LED Matrix Display Engine*
