// board.h — Chronos Pulser Board Pin Definitions
// Complete pin map with alternate functions for Lattice ECP5-45F FPGA
// All pins mapped to their physical FPGA ball locations and functions

#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

//=============================================================================
// FPGA: Lattice ECP5-45F (LFE5U-45F-6BG381C), caBGA-381 package
//=============================================================================

// --- Bank 0 (VCCIO=3.3V) — FT601Q USB 3.0 Bridge Interface ---
// 32-bit parallel FIFO at 100 MHz

#define FT_DATA_BASE_PIN        0
#define FT_DATA_PIN_COUNT       32
#define FT_DATA_0               0    // Ball A3
#define FT_DATA_1               1    // Ball B3
#define FT_DATA_2               2    // Ball C4
#define FT_DATA_3               3    // Ball D4
#define FT_DATA_4               4    // Ball A4
#define FT_DATA_5               5    // Ball B4
#define FT_DATA_6               6    // Ball C5
#define FT_DATA_7               7    // Ball D5
#define FT_DATA_8               8    // Ball A5
#define FT_DATA_9               9    // Ball B5
#define FT_DATA_10              10   // Ball C6
#define FT_DATA_11              11   // Ball D6
#define FT_DATA_12              12   // Ball A6
#define FT_DATA_13              13   // Ball B6
#define FT_DATA_14              14   // Ball C7
#define FT_DATA_15              15   // Ball D7
#define FT_DATA_16              16   // Ball A7
#define FT_DATA_17              17   // Ball B7
#define FT_DATA_18              18   // Ball C8
#define FT_DATA_19              19   // Ball D8
#define FT_DATA_20              20   // Ball A8
#define FT_DATA_21              21   // Ball B8
#define FT_DATA_22              22   // Ball C9
#define FT_DATA_23              23   // Ball D9
#define FT_DATA_24              24   // Ball A9
#define FT_DATA_25              25   // Ball B9
#define FT_DATA_26              26   // Ball C10
#define FT_DATA_27              27   // Ball D10
#define FT_DATA_28              28   // Ball A10
#define FT_DATA_29              29   // Ball B10
#define FT_DATA_30              30   // Ball C11
#define FT_DATA_31              31   // Ball D11

// FT601 control signals (Bank 0, continued)
#define FT_CLK                  32   // Ball A11 — 100 MHz FIFO clock (input from FPGA PLL)
#define FT_TXE_N                33   // Ball B11 — Transmit FIFO empty (input, active low)
#define FT_RXF_N                34   // Ball C12 — Receive FIFO full (input, active low)
#define FT_RD_N                 35   // Ball D12 — Read strobe (output, active low)
#define FT_WR_N                 36   // Ball A12 — Write strobe (output, active low)
#define FT_OE_N                 37   // Ball B12 — Output enable (output, active low)
#define FT_SIWU_N               38   // Ball C13 — Send immediate / wake up (output, active low)
#define FT_BE_0                 39   // Ball D13 — Byte enable 0
#define FT_BE_1                 40   // Ball A13 — Byte enable 1
#define FT_BE_2                 41   // Ball B13 — Byte enable 2
#define FT_BE_3                 42   // Ball C14 — Byte enable 3
#define FT_RESET_N              43   // Ball D14 — FT601 reset (output, active low)

// --- Bank 1 (VCCIO=1.8V) — AD9230-250 ADC LVDS Interface ---
// 12-bit DDR LVDS data at 250 MHz, plus clock and SPI config

#define ADC_D0_P                44   // Ball E1
#define ADC_D0_N                45   // Ball F1
#define ADC_D1_P                46   // Ball E2
#define ADC_D1_N                47   // Ball F2
#define ADC_D2_P                48   // Ball E3
#define ADC_D2_N                49   // Ball F3
#define ADC_D3_P                50   // Ball E4
#define ADC_D3_N                51   // Ball F4
#define ADC_D4_P                52   // Ball E5
#define ADC_D4_N                53   // Ball F5
#define ADC_D5_P                54   // Ball E6
#define ADC_D5_N                55   // Ball F6
#define ADC_D6_P                56   // Ball G1
#define ADC_D6_N                57   // Ball H1
#define ADC_D7_P                58   // Ball G2
#define ADC_D7_N                59   // Ball H2
#define ADC_D8_P                60   // Ball G3
#define ADC_D8_N                61   // Ball H3
#define ADC_D9_P                62   // Ball G4
#define ADC_D9_N                63   // Ball H4
#define ADC_D10_P               64   // Ball G5
#define ADC_D10_N               65   // Ball H5
#define ADC_D11_P               66   // Ball G6
#define ADC_D11_N               67   // Ball H6

#define ADC_OR_P                68   // Ball J1 — Over-range
#define ADC_OR_N                69   // Ball J2
#define ADC_DCO_P               70   // Ball J3 — Data clock output
#define ADC_DCO_N               71   // Ball J4
#define ADC_CLK_P               72   // Ball K1 — Sample clock to ADC (output)
#define ADC_CLK_N               73   // Ball K2
#define ADC_SPI_CS              74   // Ball K3 — ADC SPI chip select
#define ADC_SPI_CLK             75   // Ball K4 — ADC SPI clock
#define ADC_SPI_MOSI            76   // Ball K5 — ADC SPI data (bidirectional SDIO)

// --- Bank 2 (VCCIO=1.35V) — DDR3L SDRAM Interface ---
// MT41K128M16JT-125, 256 MB, 16-bit wide, 800 MT/s

#define DDR_A0                  77   // Ball L1
#define DDR_A1                  78   // Ball L2
#define DDR_A2                  79   // Ball L3
#define DDR_A3                  80   // Ball L4
#define DDR_A4                  81   // Ball L5
#define DDR_A5                  82   // Ball L6
#define DDR_A6                  83   // Ball L7
#define DDR_A7                  84   // Ball L8
#define DDR_A8                  85   // Ball L9
#define DDR_A9                  86   // Ball L10
#define DDR_A10                 87   // Ball M1 — A10/AP
#define DDR_A11                 88   // Ball M2
#define DDR_A12                 89   // Ball M3 — A12/BC#
#define DDR_A13                 90   // Ball M4
#define DDR_BA0                 91   // Ball M5
#define DDR_BA1                 92   // Ball M6
#define DDR_BA2                 93   // Ball M7
#define DDR_CKE                 94   // Ball M8
#define DDR_CLK_P               95   // Ball M9
#define DDR_CLK_N               96   // Ball M10
#define DDR_CS_N                97   // Ball N1
#define DDR_RAS_N               98   // Ball N2
#define DDR_CAS_N               99   // Ball N3
#define DDR_WE_N                100  // Ball N4
#define DDR_ODT                 101  // Ball N5
#define DDR_RESET_N             102  // Ball N6

// DDR3 DQ byte lane 0 (upper byte: DQ[7:0])
#define DDR_DQ0                 103  // Ball N7
#define DDR_DQ1                 104  // Ball N8
#define DDR_DQ2                 105  // Ball N9
#define DDR_DQ3                 106  // Ball N10
#define DDR_DQ4                 107  // Ball P1
#define DDR_DQ5                 108  // Ball P2
#define DDR_DQ6                 109  // Ball P3
#define DDR_DQ7                 110  // Ball P4
#define DDR_DQS0_P              111  // Ball P5 — UDQS
#define DDR_DQS0_N              112  // Ball P6 — UDQS#
#define DDR_DM0                 113  // Ball P7 — UDM

// DDR3 DQ byte lane 1 (lower byte: DQ[15:8])
#define DDR_DQ8                 114  // Ball P8
#define DDR_DQ9                 115  // Ball P9
#define DDR_DQ10                116  // Ball P10
#define DDR_DQ11                117  // Ball R1
#define DDR_DQ12                118  // Ball R2
#define DDR_DQ13                119  // Ball R3
#define DDR_DQ14                120  // Ball R4
#define DDR_DQ15                121  // Ball R5
#define DDR_DQS1_P              122  // Ball R6 — LDQS
#define DDR_DQS1_N              123  // Ball R7 — LDQS#
#define DDR_DM1                 124  // Ball R8 — LDM

// --- Bank 3 (VCCIO=3.3V) — SPI, I2C, GPIO, Pulse Generator ---

// SPI Flash (W25Q128JVSIQ)
#define SPI_FLASH_CS            125  // Ball T1
#define SPI_FLASH_CLK           126  // Ball T2
#define SPI_FLASH_MOSI          127  // Ball T3
#define SPI_FLASH_MISO          128  // Ball T4

// VGA (LMH6517) SPI
#define VGA_SPI_CS              129  // Ball T5
#define VGA_SPI_CLK             130  // Ball T6
#define VGA_SPI_MOSI            131  // Ball T7
#define VGA_SPI_MISO            132  // Ball T8

// I2C (TMP117 temperature sensor)
#define I2C_SCL                 133  // Ball U1 — 4.7k pull-up to 3.3V
#define I2C_SDA                 134  // Ball U2 — 4.7k pull-up to 3.3V

// Pulse Generator Control
#define PULSE_TRIG              135  // Ball U3 — Avalanche transistor trigger
#define SRD_BIAS_DAC            136  // Ball U4 — SRD bias voltage (PWM output)

// WS2812 RGB LED chain
#define LED_DATA                137  // Ball U5

// Trigger Sync
#define SYNC_OUT                138  // Ball U6 — Trigger sync output to J3
#define SYNC_IN                 139  // Ball U7 — External trigger input from J3

// Status LEDs (direct GPIO control, alternative to WS2812)
#define STATUS_LED_R            140  // Ball U8
#define STATUS_LED_G            141  // Ball U9
#define STATUS_LED_B            142  // Ball U10

// Power-down controls
#define ADC_PDWN                143  // Ball U11 — ADC power-down (active high)
#define VGA_PDWN                144  // Ball U12 — VGA power-down (active high)

// --- Bank 6 (VCCIO=3.3V) — Configuration & System ---

// FPGA Configuration (shared with SPI flash pins)
#define CFG_CSN                 145  // Ball V1
#define CFG_CSSPINEN            146  // Ball V2
#define CFG_INITN               147  // Ball V3 — 4.7k pull-up
#define CFG_PROGRAMN            148  // Ball V4 — SW1 to GND, 4.7k pull-up
#define CFG_DONE                149  // Ball V5 — 4.7k pull-up
#define CFG_MCLK                150  // Ball V6 — Shared with SPI_FLASH_CLK
#define CFG_MOSI                151  // Ball V7 — Shared with SPI_FLASH_MOSI
#define CFG_MISO                152  // Ball V8 — Shared with SPI_FLASH_MISO

// JTAG (10-pin header J4)
#define JTAG_TCK                153  // Ball V9
#define JTAG_TMS                154  // Ball V10
#define JTAG_TDI                155  // Ball V11
#define JTAG_TDO                156  // Ball V12

// Reference Clock (SiT9365 250 MHz LVDS)
#define CLK_REF_P               157  // Ball V13
#define CLK_REF_N               158  // Ball V14

// System Reset
#define RESET_N                 159  // Ball V15 — From MAX16054

//=============================================================================
// GPIO Pin Number Aliases (for gpio_init.c and gpio_set/get functions)
//=============================================================================

// These are the GPIO controller pin indices (0–31 within the GPIO peripheral)
// The GPIO controller aggregates pins from Bank 3 and Bank 6

#define GPIO_PULSE_TRIG         0
#define GPIO_SRD_BIAS_DAC       1
#define GPIO_LED_DATA           2
#define GPIO_SYNC_OUT           3
#define GPIO_SYNC_IN            4
#define GPIO_ADC_PDWN           5
#define GPIO_VGA_PDWN           6
#define GPIO_FT_RESET_N         7
#define GPIO_SPI_FLASH_CS       8
#define GPIO_VGA_SPI_CS         9
#define GPIO_ADC_SPI_CS         10
#define GPIO_STATUS_LED_R       11
#define GPIO_STATUS_LED_G       12
#define GPIO_STATUS_LED_B       13
#define GPIO_CFG_PROGRAMN       14
#define GPIO_CFG_INITN          15
#define GPIO_CFG_DONE           16

//=============================================================================
// I2C Device Addresses
//=============================================================================

#define I2C_ADDR_TMP117         0x48  // TMP117AIDRVR, 7-bit address (ADDR pin = GND)
#define I2C_ADDR_TMP117_ALT1    0x49  // Alternate if ADDR = VCC
#define I2C_ADDR_TMP117_ALT2    0x4A  // Alternate if ADDR = SDA
#define I2C_ADDR_TMP117_ALT3    0x4B  // Alternate if ADDR = SCL

//=============================================================================
// SPI Flash Parameters (W25Q128JVSIQ)
//=============================================================================

#define SPI_FLASH_SIZE_BYTES    (16 * 1024 * 1024)  // 128 Mbit = 16 MB
#define SPI_FLASH_SECTOR_SIZE   4096                // 4 KB sectors
#define SPI_FLASH_BLOCK_SIZE    65536               // 64 KB blocks
#define SPI_FLASH_PAGE_SIZE     256                 // 256 byte pages
#define SPI_FLASH_MAX_FREQ      50000000            // 50 MHz max clock

// Flash memory map
#define FLASH_OFFSET_GOLDEN     0x000000            // Golden FPGA bitstream
#define FLASH_OFFSET_UPDATE     0x080000            // Update FPGA bitstream
#define FLASH_OFFSET_BOOTLOADER 0x100000            // Soft CPU bootloader
#define FLASH_OFFSET_FIRMWARE   0x140000            // Main firmware
#define FLASH_OFFSET_TDC_CAL    0x1C0000            // TDC calibration LUT
#define FLASH_OFFSET_FACTORY    0x1D0000            // Factory calibration data
#define FLASH_OFFSET_LITTLEFS   0x1E0000            // littlefs filesystem

//=============================================================================
// Clock Frequencies
//=============================================================================

#define CLOCK_REF_FREQ          250000000UL  // SiT9365 reference: 250 MHz
#define CLOCK_SYS_FREQ          100000000UL  // System/bus clock: 100 MHz
#define CLOCK_ADC_FREQ          250000000UL  // ADC sample clock: 250 MHz
#define CLOCK_TDC_FREQ          500000000UL  // TDC tap clock: 500 MHz
#define CLOCK_USB_FREQ          100000000UL  // FT601 FIFO clock: 100 MHz
#define CLOCK_DDR3_FREQ         400000000UL  // DDR3 controller: 400 MHz (800 MT/s)

//=============================================================================
// USB Vendor/Product IDs
//=============================================================================

#define USB_VID                 0x16D0   // Nous Research vendor ID (fictitious for prototype)
#define USB_PID                 0x0C50   // Chronos Pulser product ID
#define USB_MANUFACTURER_STR    "Nous Research"
#define USB_PRODUCT_STR         "Chronos Pulser"
#define USB_SERIAL_STR          "CP000001"

//=============================================================================
// Power Rail Voltage Thresholds (for health monitoring)
//=============================================================================

#define VCC_3V3_NOMINAL         3.30f
#define VCC_3V3_MIN             3.10f
#define VCC_3V3_MAX             3.50f

#define VCC_1V2_NOMINAL         1.20f
#define VCC_1V2_MIN             1.14f
#define VCC_1V2_MAX             1.26f

#define VCC_1V8_NOMINAL         1.80f
#define VCC_1V8_MIN             1.71f
#define VCC_1V8_MAX             1.89f

#define VCC_1V35_NOMINAL        1.35f
#define VCC_1V35_MIN            1.28f
#define VCC_1V35_MAX            1.42f

#define VCC_5V0_NOMINAL         5.00f
#define VCC_5V0_MIN             4.75f
#define VCC_5V0_MAX             5.25f

//=============================================================================
// Temperature Limits
//=============================================================================

#define TEMP_WARNING_C          70.0f   // Throttle pulse rate above this
#define TEMP_SHUTDOWN_C         85.0f   // Emergency shutdown
#define TEMP_NORMAL_MAX_C       60.0f   // Normal operating range

//=============================================================================
// Pulse Generator Default Parameters
//=============================================================================

#define PULSE_DEFAULT_FREQ_HZ   1000    // 1 kHz default repetition rate
#define PULSE_DEFAULT_AMPLITUDE 128     // DAC mid-scale (~250 mV)
#define PULSE_MIN_FREQ_HZ       1       // 1 Hz minimum
#define PULSE_MAX_FREQ_HZ       1000000 // 1 MHz maximum
#define PULSE_PERIOD_TICKS(f)   (CLOCK_ADC_FREQ / (f))  // Period in 4 ns ticks

//=============================================================================
// ADC Default Parameters
//=============================================================================

#define ADC_DEFAULT_SAMPLES     4096    // Default acquisition length
#define ADC_MAX_SAMPLES         16777215 // 24-bit max
#define ADC_RESOLUTION_BITS     12
#define ADC_FS_VOLTAGE_MV       2000    // 2V peak-to-peak full scale

//=============================================================================
// TDC Parameters
//=============================================================================

#define TDC_COARSE_LSB_PS       4000    // 4 ns = 4000 ps
#define TDC_FINE_BINS           256     // Number of carry-chain taps
#define TDC_NOMINAL_BIN_PS      15      // ~15 ps per bin (uncalibrated)
#define TDC_EFFECTIVE_RES_PS    10      // 10 ps after calibration

//=============================================================================
// DMA Buffer Addresses in DDR3
//=============================================================================

#define DMA_ADC_BUFFER          0x01000000  // 16 MB offset for ADC data
#define DMA_TDC_BUFFER          0x02000000  // 16 MB offset for TDC data
#define DMA_USB_TX_BUFFER       0x03000000  // 16 MB offset for USB TX
#define DMA_USB_RX_BUFFER       0x04000000  // 16 MB offset for USB RX
#define DMA_CAL_BUFFER          0x05000000  // 16 MB offset for calibration
#define DMA_SCRATCH_BUFFER      0x06000000  // 16 MB scratch

//=============================================================================
// LED Color Presets
//=============================================================================

#define LED_COLOR_OFF           0x000000
#define LED_COLOR_RED           0xFF0000
#define LED_COLOR_GREEN         0x00FF00
#define LED_COLOR_BLUE          0x0000FF
#define LED_COLOR_YELLOW        0xFFFF00
#define LED_COLOR_CYAN          0x00FFFF
#define LED_COLOR_MAGENTA       0xFF00FF
#define LED_COLOR_WHITE         0xFFFFFF
#define LED_COLOR_ORANGE        0xFF8000
#define LED_COLOR_DIM_RED       0x200000
#define LED_COLOR_DIM_GREEN     0x002000
#define LED_COLOR_DIM_BLUE      0x000020

// Status LED meanings
#define LED_STATUS_BOOTING      LED_COLOR_YELLOW
#define LED_STATUS_READY         LED_COLOR_GREEN
#define LED_STATUS_ACQUIRING     LED_COLOR_CYAN
#define LED_STATUS_ERROR         LED_COLOR_RED
#define LED_STATUS_CALIBRATING  LED_COLOR_MAGENTA
#define LED_STATUS_OVERTEMP      LED_COLOR_ORANGE

#endif // BOARD_H
