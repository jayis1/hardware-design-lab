/*
 * board.h — TensilGuard board-level definitions
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Pin map and timing constants for the TensilGuard reference board
 * built around the STM32G431RBT6. All board-specific constants live here
 * so the driver files remain portable across future revisions.
 */
#ifndef TENSILGUARD_BOARD_H
#define TENSILGUARD_BOARD_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ----------------------------- MCU & clocks ----------------------------- */
#define BOARD_MCU_NAME          "STM32G431RBT6"
#define BOARD_HSI_HZ            16000000UL
#define BOARD_HSE_HZ            8000000UL
#define BOARD_SYSCLK_HZ         170000000UL
#define BOARD_HCLK_HZ           170000000UL
#define BOARD_PCLK1_HZ          170000000UL
#define BOARD_PCLK2_HZ          170000000UL
#define BOARD_LSE_HZ            32768UL
#define BOARD_FLASH_LATENCY     5     /* WS for 170 MHz */

/* ----------------------------- Pin map ----------------------------------- */
/* GPIO port letters map to GPIOA..GPIOC; pin assignments expressed as
 * pin_t compound literals via the PIN() macro defined in the Helpers
 * section below. Kept abstract so the firmware can be ported to other
 * STM32 variants with minimal churn. The actual pin definitions appear
 * after the pin_t typedef so they can use the PIN() helper. */

/* ----------------------------- Analog channel config --------------------- */
#define ADC_VCOIL_CH       1
#define ADC_VSHUNT_CH      2
#define ADC_AE_CH          4
#define ADC_VBATT_CH       8            /* internal VBAT/2 divider */
#define ADC_PANEL_CH       9

/* ----------------------------- Timing ------------------------------------ */
#define TIM_MAG_DRV        1            /* TIM1 for coil PWM            */
#define TIM_AE_SAMPLE      3            /* TIM3 triggers ADC for AE     */
#define TIM_ACC_SAMPLE     6            /* TIM6 triggers accel readout  */
#define TIM_RTC_ALARM      16           /* RTC alarm-A wake             */

/* ----------------------------- Misc constants ---------------------------- */
#define LED_ON(level)     level
#define COIL_BOOST_V      24.0f
#define COIL_DRIVE_MA_MAX 200
#define PANEL_NCELLS       6
#define PANEL_VOC_MV       21000
#define BAT_CHEMISTRY      "LiFePO4"

/* ----------------------------- Helpers ----------------------------------- */
typedef struct { char port; uint8_t pin; } pin_t;

/* helper: turn a (port, pin) pair into a pin_t without compound-literal
 * issues in macro argument position. */
#define PIN(port, num) ((pin_t){ (port), (num) })

/* override the simple macros with pin_t-producing ones so gpio_set(PIN_X, 1)
 * works in all positions. */
#undef PIN_MAG_DRV_P
#undef PIN_MAG_DRV_N
#undef PIN_MAG_BOOST_EN
#undef PIN_MAG_VCOIL_ADC
#undef PIN_MAG_VSHUNT_ADC
#undef PIN_MAG_OPAMP_OUT
#undef PIN_ACC_CS
#undef PIN_ACC_SCK
#undef PIN_ACC_MISO
#undef PIN_ACC_MOSI
#undef PIN_ACC_INT1
#undef PIN_AE_ADC
#undef PIN_AE_COMP_OUT
#undef PIN_AE_AMP_EN
#undef PIN_TEMP_SCL
#undef PIN_TEMP_SDA
#undef PIN_LORA_NSS
#undef PIN_LORA_SCK
#undef PIN_LORA_MISO
#undef PIN_LORA_MOSI
#undef PIN_LORA_RST
#undef PIN_LORA_BUSY
#undef PIN_LORA_DIO1
#undef PIN_LORA_ANT_SW
#undef PIN_FLASH_NSS
#undef PIN_FLASH_SCK
#undef PIN_FLASH_MISO
#undef PIN_FLASH_MOSI
#undef PIN_FUEL_INT
#undef PIN_LED_RED
#undef PIN_LED_GREEN
#undef PIN_LED_BLUE
#undef PIN_UART_TX
#undef PIN_UART_RX

#define PIN_MAG_DRV_P      PIN('A', 8)
#define PIN_MAG_DRV_N      PIN('A', 9)
#define PIN_MAG_BOOST_EN   PIN('A', 10)
#define PIN_MAG_VCOIL_ADC  PIN('A', 0)
#define PIN_MAG_VSHUNT_ADC PIN('A', 1)
#define PIN_MAG_OPAMP_OUT  PIN('A', 2)
#define PIN_ACC_CS         PIN('C', 13)
#define PIN_ACC_SCK        PIN('C', 10)
#define PIN_ACC_MISO       PIN('C', 11)
#define PIN_ACC_MOSI       PIN('C', 12)
#define PIN_ACC_INT1       PIN('B', 0)
#define PIN_AE_ADC         PIN('A', 3)
#define PIN_AE_COMP_OUT    PIN('B', 1)
#define PIN_AE_AMP_EN      PIN('B', 2)
#define PIN_TEMP_SCL       PIN('B', 6)
#define PIN_TEMP_SDA       PIN('B', 7)
#define PIN_LORA_NSS       PIN('A', 4)
#define PIN_LORA_SCK       PIN('A', 5)
#define PIN_LORA_MISO      PIN('A', 6)
#define PIN_LORA_MOSI      PIN('A', 7)
#define PIN_LORA_RST       PIN('B', 10)
#define PIN_LORA_BUSY      PIN('B', 11)
#define PIN_LORA_DIO1      PIN('B', 12)
#define PIN_LORA_ANT_SW    PIN('B', 14)
#define PIN_FLASH_NSS     PIN('B', 12)
#define PIN_FLASH_SCK     PIN('B', 13)
#define PIN_FLASH_MISO    PIN('B', 14)
#define PIN_FLASH_MOSI    PIN('B', 15)
#define PIN_FUEL_INT      PIN('B', 8)
#define PIN_LED_RED       PIN('C', 6)
#define PIN_LED_GREEN     PIN('C', 7)
#define PIN_LED_BLUE      PIN('C', 8)
#define PIN_UART_TX       PIN('A', 2)
#define PIN_UART_RX       PIN('A', 3)

static inline uint32_t gpio_base(char port) {
    switch (port) {
        case 'A': return 0x48000000UL;          /* GPIOA */
        case 'B': return 0x48000400UL;          /* GPIOB */
        case 'C': return 0x48000800UL;          /* GPIOC */
        default: return 0;
    }
}

#endif /* TENSILGUARD_BOARD_H */