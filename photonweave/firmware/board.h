/**
 * board.h — PhotonWeave Board Pin Definitions
 *
 * Complete pin mapping for STM32H743VIT6 on the PhotonWeave CGH Engine.
 * All pins are defined with their primary function and alternate functions
 * as used in this design.
 *
 * STM32H743VIT6: LQFP-100 package, 480 MHz Cortex-M7
 */

#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>
#include <stdbool.h>

//=============================================================================
// Clock Configuration
//=============================================================================
#define HSE_FREQUENCY_HZ        25000000UL   // 25 MHz from Si5345 OUT5
#define SYSCLK_FREQUENCY_HZ     480000000UL  // 480 MHz (VOS1)
#define HCLK_FREQUENCY_HZ       240000000UL  // AHB bus
#define APB1_FREQUENCY_HZ       120000000UL  // APB1 peripheral clock
#define APB2_FREQUENCY_HZ       120000000UL  // APB2 peripheral clock
#define APB4_FREQUENCY_HZ       120000000UL  // APB4 peripheral clock

//=============================================================================
// I2C1 Bus — System Management Bus (100 kHz Standard-mode)
//=============================================================================
#define I2C1_SCL_PORT           GPIOB
#define I2C1_SCL_PIN            6           // PB6, AF4
#define I2C1_SDA_PORT           GPIOB
#define I2C1_SDA_PIN            7           // PB7, AF4
#define I2C1_AF                 4
#define I2C1_TIMING             0x00303D5B  // 100 kHz @ 120 MHz APB1

// I2C Device Addresses (7-bit)
#define I2C_ADDR_TPS65218       0x24        // PMIC
#define I2C_ADDR_FPGA           0x30        // FPGA I2C slave
#define I2C_ADDR_TMP117_1       0x48        // FPGA temp zone 1
#define I2C_ADDR_TMP117_2       0x49        // FPGA temp zone 2
#define I2C_ADDR_TMP117_3       0x4A        // DDR4 ambient
#define I2C_ADDR_TMP117_4       0x4B        // PMIC temp
#define I2C_ADDR_EEPROM         0x50        // M24C02 board EEPROM
#define I2C_ADDR_SI5345         0x68        // Clock generator

//=============================================================================
// SPI1 — FPGA Communication Interface (50 MHz, Master, Hardware NSS)
//=============================================================================
#define SPI1_SCK_PORT           GPIOA
#define SPI1_SCK_PIN            5           // PA5, AF5
#define SPI1_MISO_PORT          GPIOA
#define SPI1_MISO_PIN           6           // PA6, AF5
#define SPI1_MOSI_PORT          GPIOA
#define SPI1_MOSI_PIN           7           // PA7, AF5
#define SPI1_NSS_PORT           GPIOA
#define SPI1_NSS_PIN            4           // PA4, AF5
#define SPI1_AF                 5
#define SPI1_BAUDRATE           50000000UL  // 50 MHz

// SPI Command Protocol
#define SPI_CMD_STATUS_REQ      0x01        // Request FPGA status
#define SPI_CMD_REG_READ        0x02        // Read FPGA register
#define SPI_CMD_REG_WRITE       0x03        // Write FPGA register
#define SPI_CMD_FPGA_PROG       0x10        // Initiate FPGA programming
#define SPI_CMD_FPGA_RESET      0x11        // Soft reset FPGA
#define SPI_CMD_CLOCK_THROTTLE  0x20        // Dynamic clock scaling
#define SPI_CMD_THERMAL_ALERT   0x30        // Thermal alert acknowledge
#define SPI_CMD_FW_UPDATE       0xF0        // Firmware update mode

// SPI Response Codes
#define SPI_RESP_OK             0x00
#define SPI_RESP_ERROR          0xFF
#define SPI_RESP_BUSY           0xFE
#define SPI_RESP_TIMEOUT        0xFD

//=============================================================================
// UART4 — Debug Console (115200 baud, 8N1)
//=============================================================================
#define UART4_TX_PORT           GPIOD
#define UART4_TX_PIN            1           // PD1, AF8
#define UART4_RX_PORT           GPIOD
#define UART4_RX_PIN            0           // PD0, AF8
#define UART4_AF                8
#define UART4_BAUDRATE          115200UL

//=============================================================================
// FPGA Control Signals
//=============================================================================
#define FPGA_INT_PORT           GPIOE
#define FPGA_INT_PIN            0           // PE0 — FPGA interrupt (input, pull-up)
#define FPGA_INT_EXTI_LINE      EXTI0
#define FPGA_INT_IRQn           EXTI0_IRQn

#define FPGA_PROG_B_PORT        GPIOE
#define FPGA_PROG_B_PIN         1           // PE1 — FPGA PROGRAM_B (output)
#define FPGA_PROG_B_ACTIVE      0           // Active low

#define FPGA_INIT_B_PORT        GPIOE
#define FPGA_INIT_B_PIN         2           // PE2 — FPGA INIT_B (input, pull-up)

#define FPGA_DONE_PORT          GPIOE
#define FPGA_DONE_PIN           3           // PE3 — FPGA DONE (input, pull-up)

// FPGA configuration timing
#define FPGA_PROG_PULSE_US      1           // PROGRAM_B pulse width (min 300ns)
#define FPGA_INIT_TIMEOUT_MS    500         // Max wait for INIT_B high
#define FPGA_DONE_TIMEOUT_MS    5000        // Max wait for DONE high
#define FPGA_CONFIG_CLK_MHZ     50          // EMCCLK from Si5345 OUT6

//=============================================================================
// Temperature Sensor Alert
//=============================================================================
#define TMP117_ALERT_PORT       GPIOE
#define TMP117_ALERT_PIN        4           // PE4 — TMP117 wired-OR alert
#define TMP117_ALERT_EXTI_LINE  EXTI4
#define TMP117_ALERT_IRQn       EXTI4_IRQn

//=============================================================================
// Si5345 Clock Generator Control
//=============================================================================
#define SI5345_INTR_PORT        GPIOE
#define SI5345_INTR_PIN         5           // PE5 — Si5345 interrupt
#define SI5345_INTR_EXTI_LINE   EXTI5
#define SI5345_INTR_IRQn        EXTI9_5_IRQn

#define SI5345_RST_PORT         GPIOE
#define SI5345_RST_PIN          6           // PE6 — Si5345 reset (output)
#define SI5345_OE_PORT          GPIOE
#define SI5345_OE_PIN           7           // PE7 — Si5345 output enable (output)

//=============================================================================
// LED Indicators
//=============================================================================
#define LED_HEARTBEAT_PORT      GPIOB
#define LED_HEARTBEAT_PIN       0           // PB0 — D5: STM32 heartbeat (green)
#define LED_USB_READY_PORT      GPIOB
#define LED_USB_READY_PIN       1           // PB1 — D6: USB enumerated (blue)

// LED timing
#define LED_HEARTBEAT_PERIOD_MS 1000        // 1 Hz blink
#define LED_ERROR_BLINK_MS      200         // Fast blink for errors

//=============================================================================
// FPGA Bank I/O Pin Mapping (for reference)
// These are the FPGA-side pins that connect to STM32
//=============================================================================

// Bank 18 — STM32 Interface (3.3V VCCO)
#define FPGA_PIN_STM32_SPI_SCK   "IO_L1P_T0_18"   // PA5
#define FPGA_PIN_STM32_SPI_MISO  "IO_L1N_T0_18"   // PA6
#define FPGA_PIN_STM32_SPI_MOSI  "IO_L2P_T0_18"   // PA7
#define FPGA_PIN_STM32_SPI_NSS   "IO_L2N_T0_18"   // PA4
#define FPGA_PIN_STM32_UART_TX   "IO_L3P_T0_18"   // PD1
#define FPGA_PIN_STM32_UART_RX   "IO_L3N_T0_18"   // PD0
#define FPGA_PIN_STM32_I2C_SCL   "IO_L4P_T0_18"   // PB6
#define FPGA_PIN_STM32_I2C_SDA   "IO_L4N_T0_18"   // PB7
#define FPGA_PIN_FPGA_INT_N      "IO_L5P_T0_18"   // PE0
#define FPGA_PIN_STM32_BOOT0     "IO_L5N_T0_18"   // BOOT0
#define FPGA_PIN_FPGA_PROG_B     "IO_L6P_T0_18"   // PE1
#define FPGA_PIN_FPGA_INIT_B     "IO_L6N_T0_18"   // PE2
#define FPGA_PIN_FPGA_DONE       "IO_L7P_T0_18"   // PE3
#define FPGA_PIN_STM32_RST_N     "IO_L7N_T0_18"   // NRST

// Bank 17 — QSPI Flash + GPIO (3.3V VCCO)
#define FPGA_PIN_QSPI_D0         "IO_L1P_T0_17"
#define FPGA_PIN_QSPI_D1         "IO_L1N_T0_17"
#define FPGA_PIN_QSPI_D2         "IO_L2P_T0_17"
#define FPGA_PIN_QSPI_D3         "IO_L2N_T0_17"
#define FPGA_PIN_QSPI_CLK        "IO_L3P_T0_17"
#define FPGA_PIN_QSPI_CS_N       "IO_L3N_T0_17"
#define FPGA_PIN_QSPI_RESET_N    "IO_L4P_T0_17"
#define FPGA_PIN_QSPI_WP_N       "IO_L4N_T0_17"
#define FPGA_PIN_LED_DONE        "IO_L5P_T0_17"   // D1: Green
#define FPGA_PIN_LED_PCIE        "IO_L5N_T0_17"   // D2: Blue
#define FPGA_PIN_LED_ACTIVITY    "IO_L6P_T0_17"   // D3: Yellow
#define FPGA_PIN_LED_ERROR       "IO_L6N_T0_17"   // D4: Red

// Bank 34 — Clocks + System (1.8V VCCO)
#define FPGA_PIN_CLK_FFT         "IO_L12P_MRCC_34"  // 200 MHz
#define FPGA_PIN_CLK_PCIE        "IO_L13P_MRCC_34"  // 125 MHz
#define FPGA_PIN_CLK_DDR4        "IO_L14P_MRCC_34"  // 300 MHz
#define FPGA_PIN_CLK_HDMI        "IO_L15P_MRCC_34"  // 148.5 MHz
#define FPGA_PIN_CLK_AXI         "IO_L16P_MRCC_34"  // 100 MHz
#define FPGA_PIN_CLK_CONFIG      "IO_L17P_MRCC_34"  // 50 MHz (EMCCLK)
#define FPGA_PIN_CLK_FT601       "IO_L18P_MRCC_34"  // 60 MHz
#define FPGA_PIN_CLK_QSFP        "IO_L19P_MRCC_34"  // 156.25 MHz
#define FPGA_PIN_CLK_TMDS        "IO_L20P_MRCC_34"  // 26 MHz
#define FPGA_PIN_SI5345_SCL      "IO_L21P_T3_34"
#define FPGA_PIN_SI5345_SDA      "IO_L21N_T3_34"
#define FPGA_PIN_SI5345_INTR     "IO_L22P_T3_34"
#define FPGA_PIN_SI5345_LOL      "IO_L22N_T3_34"
#define FPGA_PIN_SI5345_RST      "IO_L23P_T3_34"
#define FPGA_PIN_SI5345_OE       "IO_L23N_T3_34"

//=============================================================================
// Power Rail Monitoring Thresholds
//=============================================================================
#define VCCINT_NOMINAL_MV        1000        // 1.0V
#define VCCINT_TOLERANCE_MV      30          // ±3%
#define VCCAUX_NOMINAL_MV        1800        // 1.8V
#define VCCAUX_TOLERANCE_MV      54          // ±3%
#define VDD_DDR_NOMINAL_MV       1200        // 1.2V
#define VDD_DDR_TOLERANCE_MV     36          // ±3%
#define MGTAVCC_NOMINAL_MV       1000        // 1.0V
#define MGTAVCC_TOLERANCE_MV     30          // ±3%
#define MGTAVTT_NOMINAL_MV       1200        // 1.2V
#define MGTAVTT_TOLERANCE_MV     30          // ±2.5% (tighter for GTX)
#define VCCO_3V3_NOMINAL_MV      3300        // 3.3V
#define VCCO_3V3_TOLERANCE_MV    165         // ±5%
#define VTT_DDR_NOMINAL_MV       600         // 0.6V
#define VTT_DDR_TOLERANCE_MV     30          // ±5%

//=============================================================================
// Thermal Thresholds
//=============================================================================
#define TEMP_WARNING_MILLIC      80000       // 80°C — throttle warning
#define TEMP_CRITICAL_MILLIC     95000       // 95°C — emergency shutdown
#define TEMP_HYSTERESIS_MILLIC   5000        // 5°C hysteresis

//=============================================================================
// Watchdog Configuration
//=============================================================================
#define WATCHDOG_TIMEOUT_MS      2000        // 2 second timeout
#define WATCHDOG_KICK_INTERVAL_MS 500        // Kick every 500ms

//=============================================================================
// USB / FT601 Configuration
//=============================================================================
#define USB_VID                  0x4E4F      // "NO" — jayis1
#define USB_PID                  0x5057      // "PW" — PhotonWeave
#define USB_MAX_PACKET_SIZE      1024        // SuperSpeed bulk
#define USB_CONFIG_VALUE         1

//=============================================================================
// Board EEPROM Layout (M24C02, 256 bytes)
//=============================================================================
#define EEPROM_MAGIC_OFFSET      0x00        // 4 bytes: "PWNV"
#define EEPROM_MAGIC_VALUE       0x50574E56
#define EEPROM_REVISION_OFFSET   0x04        // 2 bytes BCD
#define EEPROM_MAC_OFFSET        0x06        // 6 bytes
#define EEPROM_SERIAL_OFFSET     0x0C        // 4 bytes
#define EEPROM_MFG_DATE_OFFSET   0x10        // 4 bytes Unix timestamp
#define EEPROM_NAME_OFFSET       0x20        // 32 bytes product name
#define EEPROM_MFR_OFFSET        0x40        // 32 bytes manufacturer

//=============================================================================
// System States
//=============================================================================
typedef enum {
    SYS_STATE_BOOT           = 0,  // Initial boot, power sequencing
    SYS_STATE_CLOCK_INIT     = 1,  // Si5345 configuration
    SYS_STATE_FPGA_PROG      = 2,  // FPGA bitstream loading
    SYS_STATE_FPGA_CAL       = 3,  // DDR4 calibration, PCIe link training
    SYS_STATE_READY          = 4,  // Fully operational
    SYS_STATE_ERROR          = 5,  // Error state
    SYS_STATE_THERMAL_THROT  = 6,  // Thermal throttling active
    SYS_STATE_SHUTDOWN       = 7,  // Emergency shutdown
} system_state_t;

//=============================================================================
// Function Prototypes for Board-Level Operations
//=============================================================================
void board_init(void);
void board_get_state(system_state_t *state);
void board_set_state(system_state_t state);
const char *board_state_string(system_state_t state);
void board_delay_ms(uint32_t ms);
void board_delay_us(uint32_t us);
uint32_t board_get_uptime_ms(void);
void board_heartbeat_update(void);
void board_error_indicate(void);

#endif // BOARD_H
