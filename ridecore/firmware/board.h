// ============================================================================
// board.h — RideCore Pin Definitions
// ============================================================================

#ifndef BOARD_H
#define BOARD_H

#include "registers.h"

// ---- GPIO Pin Definitions ----

// Status LEDs (active low)
#define STATUS_LED_PORT   GPIOC
#define STATUS_LED_PIN    13
#define FAULT_LED_PORT    GPIOC
#define FAULT_LED_PIN     14

// Gate enable (active high, default LOW = gates disabled)
#define GATE_EN_PORT      GPIOB
#define GATE_EN_PIN       0

// DESAT fault input (active low, pull-up)
#define DESAT_FAULT_PORT  GPIOB
#define DESAT_FAULT_PIN   1

// Hall sensor inputs
#define HALL_A_PORT       GPIOC
#define HALL_A_PIN        15
#define HALL_B_PORT       GPIOB
#define HALL_B_PIN        5
#define HALL_C_PORT       GPIOB
#define HALL_C_PIN        6

// SPI1 chip selects (active low, GPIO)
#define CAN_CS_PORT       GPIOA
#define CAN_CS_PIN        4
#define FLASH_CS_PORT     GPIOB
#define FLASH_CS_PIN      2

// SPI flash control
#define FLASH_WP_PORT     GPIOA
#define FLASH_WP_PIN      15
#define FLASH_HOLD_PORT   GPIOB
#define FLASH_HOLD_PIN    3

// CAN FD control
#define CAN_INT_PORT      GPIOB
#define CAN_INT_PIN       12
#define CAN_RST_PORT      GPIOB
#define CAN_RST_PIN       13

// BLE control
#define BLE_RST_PORT      GPIOB
#define BLE_RST_PIN       4

// PMIC interrupt
#define PMIC_IRQ_PORT     GPIOB
#define PMIC_IRQ_PIN      7

// BOOT0 (pulled low)
#define BOOT0_PORT        GPIOB
#define BOOT0_PIN         8

// ---- LED Macros (active low: set = OFF, reset = ON) ----
#define STATUS_LED_ON()    (STATUS_LED_PORT->BRR = (1 << STATUS_LED_PIN))
#define STATUS_LED_OFF()   (STATUS_LED_PORT->BSRR = (1 << STATUS_LED_PIN))
#define STATUS_LED_TOGGLE() (STATUS_LED_PORT->ODR ^= (1 << STATUS_LED_PIN))

#define FAULT_LED_ON()     (FAULT_LED_PORT->BRR = (1 << FAULT_LED_PIN))
#define FAULT_LED_OFF()    (FAULT_LED_PORT->BSRR = (1 << FAULT_LED_PIN))

// ---- Gate Enable ----
#define GATE_ENABLE()      (GATE_EN_PORT->BSRR = (1 << GATE_EN_PIN))
#define GATE_DISABLE()     (GATE_EN_PORT->BRR = (1 << GATE_EN_PIN))
#define GATE_IS_ENABLED()  (GATE_EN_PORT->ODR & (1 << GATE_EN_PIN))

// ---- DESAT Fault ----
#define DESAT_FAULT_ACTIVE() (!(DESAT_FAULT_PORT->IDR & (1 << DESAT_FAULT_PIN)))

// ---- SPI CS Macros ----
#define CAN_CS_LOW()       (CAN_CS_PORT->BRR = (1 << CAN_CS_PIN))
#define CAN_CS_HIGH()      (CAN_CS_PORT->BSRR = (1 << CAN_CS_PIN))
#define FLASH_CS_LOW()     (FLASH_CS_PORT->BRR = (1 << FLASH_CS_PIN))
#define FLASH_CS_HIGH()    (FLASH_CS_PORT->BSRR = (1 << FLASH_CS_PIN))

// ---- CAN Reset ----
#define CAN_RESET_LOW()    (CAN_RST_PORT->BRR = (1 << CAN_RST_PIN))
#define CAN_RESET_HIGH()   (CAN_RST_PORT->BSRR = (1 << CAN_RST_PIN))

// ---- BLE Reset ----
#define BLE_RESET_LOW()    (BLE_RST_PORT->BRR = (1 << BLE_RST_PIN))
#define BLE_RESET_HIGH()   (BLE_RST_PORT->BSRR = (1 << BLE_RST_PIN))

// ---- Hall Sensor Read ----
#define HALL_READ()  (((HALL_A_PORT->IDR >> HALL_A_PIN) & 1) | \
                      (((HALL_B_PORT->IDR >> HALL_B_PIN) & 1) << 1) | \
                      (((HALL_C_PORT->IDR >> HALL_C_PIN) & 1) << 2))

// ---- Motor Parameters (defaults) ----
#define MOTOR_POLE_PAIRS         7
#define MOTOR_PHASE_R_MOHM       50      // 0.050 Ω
#define MOTOR_PHASE_L_UH         100     // 100 µH
#define MOTOR_MAX_CURRENT_MA     200000  // 200 A peak
#define MOTOR_CONT_CURRENT_MA    120000  // 120 A continuous
#define MOTOR_PWM_FREQ_HZ        20000   // 20 kHz
#define MOTOR_DEADTIME_NS         500     // 500 ns

// ---- ADC Channel Mapping ----
#define ADC_CH_CUR_A     1   // ADC1 IN1 / PC0
#define ADC_CH_CUR_B     2   // ADC1 IN2 / PC1
#define ADC_CH_CUR_C     3   // ADC1 IN3 / PC2
#define ADC_CH_VBAT      4   // ADC2 IN4 / PC3

// ---- Current Sense Constants ----
#define SHUNT_RESISTANCE_MOHM     0.5f    // 0.5 mΩ
#define INA241_GAIN               50.0f   // 50 V/V
#define ADC_VREF_MV               3300.0f // 3.3V reference
#define ADC_RESOLUTION            4096.0f // 12-bit
#define CURRENT_SCALE_MA_PER_LSB  ((ADC_VREF_MV * 1000.0f) / \
                                   (ADC_RESOLUTION * INA241_GAIN * SHUNT_RESISTANCE_MOHM))
// = 3300000 / (4096 * 50 * 0.5) = 3300000 / 102400 ≈ 32.23 mA/LSB

// ---- VBAT Divider ----
// R_upper = 100 kΩ, R_lower = 3.3 kΩ
// VBAT = ADC_VBAT * (100 + 3.3) / 3.3 = ADC_VBAT * 31.30
#define VBAT_DIVIDER_RATIO    31.30f

// ---- System Clock ----
#define SYSCLK_HZ     170000000UL
#define HSE_HZ        8000000UL
#define AHB_HZ        170000000UL
#define APB1_HZ       170000000UL
#define APB2_HZ       170000000UL

// ---- TIM1 PWM Configuration ----
// Center-aligned mode 1, 20 kHz
// ARR = SYSCLK / (2 * PWM_FREQ) = 170000000 / 40000 = 4250
#define TIM1_ARR_VALUE  4250
// Dead time: 500 ns @ 170 MHz → DTG = 500ns * 170MHz = 85 ticks
// DTG[7:0] = 85 → BDTR DTG field (0x55 = 85 decimal, but DTG encoding: 0-127 = DTG*1/fDTG)
// For 170 MHz Tclk: 1/fDTG = 5.88 ns, so DTG = 500/5.88 ≈ 85
#define TIM1_DTG_VALUE  85

// ---- Flash (W25Q128) JEDEC ID ----
#define W25Q128_JEDEC_ID   0xEF4018

// ---- PMIC I2C Address ----
#define PMIC_I2C_ADDR      0x24

// ---- Fault Codes ----
#define FAULT_NONE          0x00
#define FAULT_OVERVOLTAGE   0x01
#define FAULT_UNDERVOLTAGE  0x02
#define FAULT_OVERCURRENT   0x04
#define FAULT_OVERTEMP      0x08
#define FAULT_DESAT         0x10
#define FAULT_STALL         0x20
#define FAULT_HW_FAULT      0x40

#endif // BOARD_H