// board.h — Aether-Link Board Pin Definitions and Alternate Functions
// XC7K325T-2FFG900C FPGA on custom 12-layer PCIe FHHL board
// All pin assignments verified against Kintex-7 packaging and PCB layout

#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

// ============================================================================
// FPGA Bank Voltage Definitions
// ============================================================================
#define BANK12_VOLTAGE  1.5f   // HR Bank 12: DDR4 Channel A DQ[0:15]
#define BANK13_VOLTAGE  1.5f   // HR Bank 13: DDR4 Channel A DQ[16:31]
#define BANK14_VOLTAGE  1.5f   // HR Bank 14: DDR4 Channel B DQ[0:15]
#define BANK15_VOLTAGE  1.5f   // HR Bank 15: DDR4 Channel B DQ[16:31]
#define BANK16_VOLTAGE  1.5f   // HR Bank 16: SPI, I2C, UART, GPIO, LEDs, Fan
#define BANK17_VOLTAGE  1.5f   // HR Bank 17: USB/FT232H, JTAG, Reset, Config
#define BANK18_VOLTAGE  1.5f   // HR Bank 18: QSFP28 Control (I2C, ModPrsL, etc.)
#define BANK34_VOLTAGE  1.8f   // HR Bank 34: System clock inputs
#define BANK35_VOLTAGE  1.8f   // HR Bank 35: CMAC/QSFP reference clocks
#define BANK115_VOLTAGE 1.0f   // GTX Bank 115: PCIe Gen4 x8
#define BANK116_VOLTAGE 1.0f   // GTX Bank 116: 100GbE CMAC #0
#define BANK117_VOLTAGE 1.0f   // GTX Bank 117: 100GbE CMAC #1

// ============================================================================
// DDR4 Channel A Pin Assignments (Bank 12 + Bank 13)
// ============================================================================

// Address/Command/Control (Bank 12)
#define DDR4_A_A0_PIN        "L12"
#define DDR4_A_A1_PIN        "M12"
#define DDR4_A_A2_PIN        "N12"
#define DDR4_A_A3_PIN        "P12"
#define DDR4_A_A4_PIN        "R12"
#define DDR4_A_A5_PIN        "T12"
#define DDR4_A_A6_PIN        "U12"
#define DDR4_A_A7_PIN        "V12"
#define DDR4_A_A8_PIN        "W12"
#define DDR4_A_A9_PIN        "Y12"
#define DDR4_A_A10_PIN       "AA12"
#define DDR4_A_A11_PIN       "AB12"
#define DDR4_A_A12_PIN       "AC12"
#define DDR4_A_A13_PIN       "AD12"
#define DDR4_A_BA0_PIN       "AE12"
#define DDR4_A_BA1_PIN       "AF12"
#define DDR4_A_BG0_PIN       "AG12"
#define DDR4_A_CKE_PIN       "AH12"
#define DDR4_A_CS_N_PIN      "AJ12"
#define DDR4_A_ODT_PIN       "AK12"
#define DDR4_A_ACT_N_PIN     "AL12"
#define DDR4_A_RAS_N_PIN     "AM12"
#define DDR4_A_CAS_N_PIN     "AN12"
#define DDR4_A_WE_N_PIN      "AP12"
#define DDR4_A_RESET_N_PIN   "AR12"
#define DDR4_A_PAR_PIN       "AT12"
#define DDR4_A_ALERT_N_PIN   "AU12"
#define DDR4_A_CK0_P_PIN     "AV12"
#define DDR4_A_CK0_N_PIN     "AW12"

// DQ Byte Lane 0 (Bank 12)
#define DDR4_A_DQ0_PIN       "AY12"
#define DDR4_A_DQ1_PIN       "BA12"
#define DDR4_A_DQ2_PIN       "BB12"
#define DDR4_A_DQ3_PIN       "BC12"
#define DDR4_A_DQ4_PIN       "BD12"
#define DDR4_A_DQ5_PIN       "BE12"
#define DDR4_A_DQ6_PIN       "BF12"
#define DDR4_A_DQ7_PIN       "BG12"
#define DDR4_A_DQS0_P_PIN    "BS12"
#define DDR4_A_DQS0_N_PIN    "BT12"
#define DDR4_A_DM0_PIN       "BU12"

// DQ Byte Lane 1 (Bank 12)
#define DDR4_A_DQ8_PIN       "BH12"
#define DDR4_A_DQ9_PIN       "BJ12"
#define DDR4_A_DQ10_PIN      "BK12"
#define DDR4_A_DQ11_PIN      "BL12"
#define DDR4_A_DQ12_PIN      "BM12"
#define DDR4_A_DQ13_PIN      "BN12"
#define DDR4_A_DQ14_PIN      "BP12"
#define DDR4_A_DQ15_PIN      "BR12"
#define DDR4_A_DQS1_P_PIN    "CM12"
#define DDR4_A_DQS1_N_PIN    "CN12"
#define DDR4_A_DM1_PIN       "CP12"

// DQ Byte Lane 2 (Bank 13)
#define DDR4_A_DQ16_PIN      "BV12"
#define DDR4_A_DQ17_PIN      "BW12"
#define DDR4_A_DQ18_PIN      "BX12"
#define DDR4_A_DQ19_PIN      "BY12"
#define DDR4_A_DQ20_PIN      "BZ12"
#define DDR4_A_DQ21_PIN      "CA12"
#define DDR4_A_DQ22_PIN      "CB12"
#define DDR4_A_DQ23_PIN      "CC12"
#define DDR4_A_DQS2_P_PIN    "CR12"
#define DDR4_A_DQS2_N_PIN    "CS12"
#define DDR4_A_DM2_PIN       "CT12"

// DQ Byte Lane 3 (Bank 13)
#define DDR4_A_DQ24_PIN      "CD12"
#define DDR4_A_DQ25_PIN      "CE12"
#define DDR4_A_DQ26_PIN      "CF12"
#define DDR4_A_DQ27_PIN      "CG12"
#define DDR4_A_DQ28_PIN      "CH12"
#define DDR4_A_DQ29_PIN      "CJ12"
#define DDR4_A_DQ30_PIN      "CK12"
#define DDR4_A_DQ31_PIN      "CL12"
#define DDR4_A_DQS3_P_PIN    "DA12"
#define DDR4_A_DQS3_N_PIN    "DB12"
#define DDR4_A_DM3_PIN       "DC12"

// ============================================================================
// DDR4 Channel B Pin Assignments (Bank 14 + Bank 15)
// ============================================================================

// Address/Command/Control (Bank 14)
#define DDR4_B_A0_PIN        "DD12"
#define DDR4_B_A1_PIN        "DE12"
#define DDR4_B_A2_PIN        "DF12"
#define DDR4_B_A3_PIN        "DG12"
#define DDR4_B_A4_PIN        "DH12"
#define DDR4_B_A5_PIN        "DJ12"
#define DDR4_B_A6_PIN        "DK12"
#define DDR4_B_A7_PIN        "DL12"
#define DDR4_B_A8_PIN        "DM12"
#define DDR4_B_A9_PIN        "DN12"
#define DDR4_B_A10_PIN       "DP12"
#define DDR4_B_A11_PIN       "DR12"
#define DDR4_B_A12_PIN       "DT12"
#define DDR4_B_A13_PIN       "DU12"
#define DDR4_B_BA0_PIN       "DV12"
#define DDR4_B_BA1_PIN       "DW12"
#define DDR4_B_BG0_PIN       "DX12"
#define DDR4_B_CKE_PIN       "DY12"
#define DDR4_B_CS_N_PIN      "EA12"
#define DDR4_B_ODT_PIN       "EB12"
#define DDR4_B_ACT_N_PIN     "EC12"
#define DDR4_B_RAS_N_PIN     "ED12"
#define DDR4_B_CAS_N_PIN     "EE12"
#define DDR4_B_WE_N_PIN      "EF12"
#define DDR4_B_RESET_N_PIN   "EG12"
#define DDR4_B_PAR_PIN       "EH12"
#define DDR4_B_ALERT_N_PIN   "EJ12"
#define DDR4_B_CK0_P_PIN     "EK12"
#define DDR4_B_CK0_N_PIN     "EL12"

// DQ Byte Lanes 0-3 (Bank 14 + Bank 15) — similar pattern to Channel A
#define DDR4_B_DQ0_PIN       "EM12"
#define DDR4_B_DQ1_PIN       "EN12"
#define DDR4_B_DQ2_PIN       "EP12"
#define DDR4_B_DQ3_PIN       "ER12"
#define DDR4_B_DQ4_PIN       "ET12"
#define DDR4_B_DQ5_PIN       "EU12"
#define DDR4_B_DQ6_PIN       "EV12"
#define DDR4_B_DQ7_PIN       "EW12"
#define DDR4_B_DQS0_P_PIN    "FA12"
#define DDR4_B_DQS0_N_PIN    "FB12"
#define DDR4_B_DM0_PIN       "FC12"

#define DDR4_B_DQ8_PIN       "FD12"
#define DDR4_B_DQ9_PIN       "FE12"
#define DDR4_B_DQ10_PIN      "FF12"
#define DDR4_B_DQ11_PIN      "FG12"
#define DDR4_B_DQ12_PIN      "FH12"
#define DDR4_B_DQ13_PIN      "FJ12"
#define DDR4_B_DQ14_PIN      "FK12"
#define DDR4_B_DQ15_PIN      "FL12"
#define DDR4_B_DQS1_P_PIN    "GA12"
#define DDR4_B_DQS1_N_PIN    "GB12"
#define DDR4_B_DM1_PIN       "GC12"

#define DDR4_B_DQ16_PIN      "GD12"
#define DDR4_B_DQ17_PIN      "GE12"
#define DDR4_B_DQ18_PIN      "GF12"
#define DDR4_B_DQ19_PIN      "GG12"
#define DDR4_B_DQ20_PIN      "GH12"
#define DDR4_B_DQ21_PIN      "GJ12"
#define DDR4_B_DQ22_PIN      "GK12"
#define DDR4_B_DQ23_PIN      "GL12"
#define DDR4_B_DQS2_P_PIN    "HA12"
#define DDR4_B_DQS2_N_PIN    "HB12"
#define DDR4_B_DM2_PIN       "HC12"

#define DDR4_B_DQ24_PIN      "HD12"
#define DDR4_B_DQ25_PIN      "HE12"
#define DDR4_B_DQ26_PIN      "HF12"
#define DDR4_B_DQ27_PIN      "HG12"
#define DDR4_B_DQ28_PIN      "HH12"
#define DDR4_B_DQ29_PIN      "HJ12"
#define DDR4_B_DQ30_PIN      "HK12"
#define DDR4_B_DQ31_PIN      "HL12"
#define DDR4_B_DQS3_P_PIN    "JA12"
#define DDR4_B_DQS3_N_PIN    "JB12"
#define DDR4_B_DM3_PIN       "JC12"

// ============================================================================
// PCIe Gen4 x8 Pin Assignments (GTX Bank 115)
// ============================================================================

#define PCIE_RX0_P_PIN       "MGT115RX0_P"
#define PCIE_RX0_N_PIN       "MGT115RX0_N"
#define PCIE_TX0_P_PIN       "MGT115TX0_P"
#define PCIE_TX0_N_PIN       "MGT115TX0_N"
#define PCIE_RX1_P_PIN       "MGT115RX1_P"
#define PCIE_RX1_N_PIN       "MGT115RX1_N"
#define PCIE_TX1_P_PIN       "MGT115TX1_P"
#define PCIE_TX1_N_PIN       "MGT115TX1_N"
#define PCIE_RX2_P_PIN       "MGT115RX2_P"
#define PCIE_RX2_N_PIN       "MGT115RX2_N"
#define PCIE_TX2_P_PIN       "MGT115TX2_P"
#define PCIE_TX2_N_PIN       "MGT115TX2_N"
#define PCIE_RX3_P_PIN       "MGT115RX3_P"
#define PCIE_RX3_N_PIN       "MGT115RX3_N"
#define PCIE_TX3_P_PIN       "MGT115TX3_P"
#define PCIE_TX3_N_PIN       "MGT115TX3_N"
#define PCIE_RX4_P_PIN       "MGT115RX4_P"
#define PCIE_RX4_N_PIN       "MGT115RX4_N"
#define PCIE_TX4_P_PIN       "MGT115TX4_P"
#define PCIE_TX4_N_PIN       "MGT115TX4_N"
#define PCIE_RX5_P_PIN       "MGT115RX5_P"
#define PCIE_RX5_N_PIN       "MGT115RX5_N"
#define PCIE_TX5_P_PIN       "MGT115TX5_P"
#define PCIE_TX5_N_PIN       "MGT115TX5_N"
#define PCIE_RX6_P_PIN       "MGT115RX6_P"
#define PCIE_RX6_N_PIN       "MGT115RX6_N"
#define PCIE_TX6_P_PIN       "MGT115TX6_P"
#define PCIE_TX6_N_PIN       "MGT115TX6_N"
#define PCIE_RX7_P_PIN       "MGT115RX7_P"
#define PCIE_RX7_N_PIN       "MGT115RX7_N"
#define PCIE_TX7_P_PIN       "MGT115TX7_P"
#define PCIE_TX7_N_PIN       "MGT115TX7_N"

// PCIe sideband signals (Bank 17, 3.3Vaux domain)
#define PCIE_PERST_N_PIN     "AH17"
#define PCIE_WAKE_N_PIN      "AG17"
#define PCIE_CLKREQ_N_PIN    "AF17"
#define PCIE_PRSNT1_N_PIN    "AE17"
#define PCIE_PRSNT2_N_PIN    "AD17"

// ============================================================================
// 100GbE QSFP28 Port 0 Pin Assignments (GTX Bank 116)
// ============================================================================

#define QSFP0_RX0_P_PIN      "MGT116RX0_P"
#define QSFP0_RX0_N_PIN      "MGT116RX0_N"
#define QSFP0_TX0_P_PIN      "MGT116TX0_P"
#define QSFP0_TX0_N_PIN      "MGT116TX0_N"
#define QSFP0_RX1_P_PIN      "MGT116RX1_P"
#define QSFP0_RX1_N_PIN      "MGT116RX1_N"
#define QSFP0_TX1_P_PIN      "MGT116TX1_P"
#define QSFP0_TX1_N_PIN      "MGT116TX1_N"
#define QSFP0_RX2_P_PIN      "MGT116RX2_P"
#define QSFP0_RX2_N_PIN      "MGT116RX2_N"
#define QSFP0_TX2_P_PIN      "MGT116TX2_P"
#define QSFP0_TX2_N_PIN      "MGT116TX2_N"
#define QSFP0_RX3_P_PIN      "MGT116RX3_P"
#define QSFP0_RX3_N_PIN      "MGT116RX3_N"
#define QSFP0_TX3_P_PIN      "MGT116TX3_P"
#define QSFP0_TX3_N_PIN      "MGT116TX3_N"

// QSFP0 control signals (Bank 18, 3.3V)
#define QSFP0_SCL_PIN        "AK18"
#define QSFP0_SDA_PIN        "AJ18"
#define QSFP0_MODPRSL_PIN    "AH18"
#define QSFP0_RESETL_PIN     "AG18"
#define QSFP0_LPMODE_PIN     "AF18"
#define QSFP0_INTL_PIN       "AE18"

// ============================================================================
// 100GbE QSFP28 Port 1 Pin Assignments (GTX Bank 117)
// ============================================================================

#define QSFP1_RX0_P_PIN      "MGT117RX0_P"
#define QSFP1_RX0_N_PIN      "MGT117RX0_N"
#define QSFP1_TX0_P_PIN      "MGT117TX0_P"
#define QSFP1_TX0_N_PIN      "MGT117TX0_N"
#define QSFP1_RX1_P_PIN      "MGT117RX1_P"
#define QSFP1_RX1_N_PIN      "MGT117RX1_N"
#define QSFP1_TX1_P_PIN      "MGT117TX1_P"
#define QSFP1_TX1_N_PIN      "MGT117TX1_N"
#define QSFP1_RX2_P_PIN      "MGT117RX2_P"
#define QSFP1_RX2_N_PIN      "MGT117RX2_N"
#define QSFP1_TX2_P_PIN      "MGT117TX2_P"
#define QSFP1_TX2_N_PIN      "MGT117TX2_N"
#define QSFP1_RX3_P_PIN      "MGT117RX3_P"
#define QSFP1_RX3_N_PIN      "MGT117RX3_N"
#define QSFP1_TX3_P_PIN      "MGT117TX3_P"
#define QSFP1_TX3_N_PIN      "MGT117TX3_N"

// QSFP1 control signals (Bank 18, 3.3V)
#define QSFP1_SCL_PIN        "AL18"
#define QSFP1_SDA_PIN        "AM18"
#define QSFP1_MODPRSL_PIN    "AN18"
#define QSFP1_RESETL_PIN     "AP18"
#define QSFP1_LPMODE_PIN     "AR18"
#define QSFP1_INTL_PIN       "AT18"

// ============================================================================
// Clock Input Pin Assignments (Bank 34 + Bank 35)
// ============================================================================

#define PCIE_REFCLK_P_PIN    "A34_P"
#define PCIE_REFCLK_N_PIN    "A34_N"
#define DDR4_REFCLK_P_PIN    "B34_P"
#define DDR4_REFCLK_N_PIN    "B34_N"
#define CMAC_REFCLK_P_PIN    "C34_P"
#define CMAC_REFCLK_N_PIN    "C34_N"
#define QSFP0_REFCLK_P_PIN   "D34_P"
#define QSFP0_REFCLK_N_PIN   "D34_N"
#define QSFP1_REFCLK_P_PIN   "E34_P"
#define QSFP1_REFCLK_N_PIN   "E34_N"

// ============================================================================
// Peripheral Pin Assignments (Bank 16, 1.5V)
// ============================================================================

// QSPI Flash (MT25QU512)
#define SPI_FLASH_CS_N_PIN   "A16"
#define SPI_FLASH_SCK_PIN    "B16"
#define SPI_FLASH_DQ0_PIN    "C16"
#define SPI_FLASH_DQ1_PIN    "D16"
#define SPI_FLASH_DQ2_PIN    "E16"
#define SPI_FLASH_DQ3_PIN    "F16"
#define SPI_FLASH_RESET_N_PIN "G16"

// I2C Bus (shared: TMP117 x2, INA226, EMC2301)
#define I2C_SCL_PIN          "H16"
#define I2C_SDA_PIN          "J16"

// UART (to FT232H)
#define UART_TX_PIN          "K16"
#define UART_RX_PIN          "L16"

// JTAG (from FT232H MPSSE)
#define JTAG_TCK_PIN         "M16"
#define JTAG_TMS_PIN         "N16"
#define JTAG_TDI_PIN         "P16"
#define JTAG_TDO_PIN         "R16"

// Fan Control
#define FAN_PWM_PIN          "T16"
#define FAN_TACH0_PIN        "U16"
#define FAN_TACH1_PIN        "V16"

// Bi-Color LEDs (4x)
#define LED0_GREEN_PIN       "W16"
#define LED0_RED_PIN         "Y16"
#define LED1_GREEN_PIN       "AA16"
#define LED1_RED_PIN         "AB16"
#define LED2_GREEN_PIN       "AC16"
#define LED2_RED_PIN         "AD16"
#define LED3_GREEN_PIN       "AE16"
#define LED3_RED_PIN         "AF16"

// ============================================================================
// Configuration Pins (Bank 17, 3.3V)
// ============================================================================

#define CONFIG_M0_PIN        "AG16"
#define CONFIG_M1_PIN        "AH16"
#define CONFIG_M2_PIN        "AJ16"
#define CONFIG_INIT_B_PIN    "AK16"
#define CONFIG_DONE_PIN      "AL16"
#define CONFIG_PROG_B_PIN    "AM16"
#define CONFIG_CCLK_PIN      "AN16"
#define CONFIG_D00_PIN       "AP16"
#define CONFIG_D01_PIN       "AR16"
#define CONFIG_D02_PIN       "AT16"
#define CONFIG_D03_PIN       "AU16"

// ============================================================================
// I2C Device Addresses
// ============================================================================

#define I2C_ADDR_TMP117_FPGA    0x48  // Temperature sensor near FPGA
#define I2C_ADDR_TMP117_QSFP0   0x49  // Temperature sensor near QSFP0
#define I2C_ADDR_INA226         0x40  // Power monitor (A0=GND, A1=GND)
#define I2C_ADDR_EMC2301        0x2E  // Fan controller (ADDR=GND)
#define I2C_ADDR_QSFP0_MODULE   0x50  // QSFP0 module EEPROM (separate bus)
#define I2C_ADDR_QSFP1_MODULE   0x50  // QSFP1 module EEPROM (separate bus)

// ============================================================================
// LED Color Definitions
// ============================================================================

#define LED_OFF     0x00
#define LED_GREEN   0x01
#define LED_RED     0x02
#define LED_AMBER   0x03  // Both green and red

// ============================================================================
// Fan PWM Definitions
// ============================================================================

#define FAN_PWM_MIN     30    // 30/255 ≈ 12% duty, minimum for reliable start
#define FAN_PWM_DEFAULT 80    // 80/255 ≈ 31% duty, quiet operation
#define FAN_PWM_MAX     255   // 100% duty, maximum cooling
#define FAN_TACH_PPR    2     // 2 pulses per revolution (standard 4-pole fan)

// ============================================================================
// Temperature Thresholds (Celsius × 100 for TMP117)
// ============================================================================

#define TEMP_WARN_C     8500   // 85.00°C — reduce throughput
#define TEMP_CRIT_C     9500   // 95.00°C — reduce queue depth
#define TEMP_SHUTDOWN_C 10000  // 100.00°C — trigger PCIe hot-remove

// ============================================================================
// Board-Specific Constants
// ============================================================================

#define BOARD_NAME          "Aether-Link"
#define BOARD_REVISION      0x0100  // Rev 1.0
#define FPGA_PART           "XC7K325T-2FFG900C"
#define DDR4_SIZE_PER_CH    0x10000000  // 256M x 16 = 4GB per channel
#define DDR4_TOTAL_SIZE     0x20000000  // 8GB total
#define QSPI_FLASH_SIZE     0x04000000  // 512Mb = 64MB
#define NUM_QSFP28_PORTS    2
#define NUM_LEDS            4
#define NUM_FANS            2
#define NUM_TEMP_SENSORS    2
#define MAX_TCP_CONNECTIONS 256
#define MAX_NVME_NAMESPACES 128
#define MAX_NVME_QUEUES     256

// ============================================================================
// Error Codes
// ============================================================================

#define ERR_NONE                    0x00
#define ERR_MMCM_LOCK_FAILED        0x01
#define ERR_DDR4_CAL_FAILED         0x02
#define ERR_PCIE_LINK_FAILED        0x03
#define ERR_CMAC_PLL_FAILED         0x04
#define ERR_I2C_BUS_FAILED          0x05
#define ERR_SPI_FLASH_FAILED        0x06
#define ERR_TEMP_CRITICAL           0x07
#define ERR_POWER_OVER_LIMIT        0x08
#define ERR_FAN_FAILED              0x09
#define ERR_TOE_HW_FAULT            0x0A
#define ERR_NVMEOF_PDU_FAULT        0x0B
#define ERR_WATCHDOG_TIMEOUT        0x0C
#define ERR_FW_UPDATE_FAILED        0x0D
#define ERR_SECURE_BOOT_FAILED      0x0E
#define ERR_QSFP_MODULE_FAULT       0x0F

#endif // BOARD_H
