/* CondenScope peripheral register map
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef CONDENSCOPE_REGISTERS_H
#define CONDENSCOPE_REGISTERS_H

#include <stdint.h>

#define MMIO32(address) (*(volatile uint32_t *)(uintptr_t)(address))
#define BIT(n) (1UL << (n))
#define FIELD(value, shift) ((uint32_t)(value) << (shift))

#define RCC_BASE             0x46020C00UL
#define RCC_AHB2ENR1         MMIO32(RCC_BASE + 0x8CUL)
#define RCC_APB1ENR1         MMIO32(RCC_BASE + 0x9CUL)
#define RCC_APB1ENR2         MMIO32(RCC_BASE + 0xA0UL)
#define RCC_APB2ENR          MMIO32(RCC_BASE + 0xA4UL)
#define RCC_CCIPR1           MMIO32(RCC_BASE + 0xE0UL)
#define RCC_AHB2_GPIOAEN     BIT(0)
#define RCC_AHB2_GPIOBEN     BIT(1)
#define RCC_AHB2_GPIOCEN     BIT(2)
#define RCC_APB1_I2C1EN      BIT(21)
#define RCC_APB1_I2C2EN      BIT(22)
#define RCC_APB1_USBEN       BIT(23)
#define RCC_APB2_ADCEN       BIT(13)

#define GPIOA_BASE           0x42020000UL
#define GPIOB_BASE           0x42020400UL
#define GPIOC_BASE           0x42020800UL
#define GPIO_MODER(base)     MMIO32((base) + 0x00UL)
#define GPIO_OTYPER(base)    MMIO32((base) + 0x04UL)
#define GPIO_OSPEEDR(base)   MMIO32((base) + 0x08UL)
#define GPIO_PUPDR(base)     MMIO32((base) + 0x0CUL)
#define GPIO_IDR(base)       MMIO32((base) + 0x10UL)
#define GPIO_ODR(base)       MMIO32((base) + 0x14UL)
#define GPIO_BSRR(base)      MMIO32((base) + 0x18UL)
#define GPIO_AFRL(base)      MMIO32((base) + 0x20UL)
#define GPIO_AFRH(base)      MMIO32((base) + 0x24UL)
#define GPIO_MODE_INPUT      0U
#define GPIO_MODE_OUTPUT     1U
#define GPIO_MODE_ALT        2U
#define GPIO_MODE_ANALOG     3U
#define GPIO_AF4             4U
#define GPIO_AF10           10U

#define I2C1_BASE            0x40005400UL
#define I2C2_BASE            0x40005800UL
#define I2C_CR1(base)        MMIO32((base) + 0x00UL)
#define I2C_CR2(base)        MMIO32((base) + 0x04UL)
#define I2C_TIMINGR(base)    MMIO32((base) + 0x10UL)
#define I2C_ISR(base)        MMIO32((base) + 0x18UL)
#define I2C_ICR(base)        MMIO32((base) + 0x1CUL)
#define I2C_RXDR(base)       MMIO32((base) + 0x24UL)
#define I2C_TXDR(base)       MMIO32((base) + 0x28UL)
#define I2C_CR1_PE           BIT(0)
#define I2C_CR2_RD_WRN       BIT(10)
#define I2C_CR2_START        BIT(13)
#define I2C_CR2_STOP         BIT(14)
#define I2C_CR2_AUTOEND      BIT(25)
#define I2C_ISR_TXIS         BIT(1)
#define I2C_ISR_RXNE         BIT(2)
#define I2C_ISR_NACKF        BIT(4)
#define I2C_ISR_STOPF        BIT(5)
#define I2C_ISR_BUSY         BIT(15)

#define ADC1_BASE            0x42028000UL
#define ADC_ISR              MMIO32(ADC1_BASE + 0x00UL)
#define ADC_CR               MMIO32(ADC1_BASE + 0x08UL)
#define ADC_CFGR1            MMIO32(ADC1_BASE + 0x0CUL)
#define ADC_SMPR1            MMIO32(ADC1_BASE + 0x14UL)
#define ADC_SQR1             MMIO32(ADC1_BASE + 0x30UL)
#define ADC_DR               MMIO32(ADC1_BASE + 0x40UL)
#define ADC_CR_ADEN          BIT(0)
#define ADC_CR_ADSTART       BIT(2)
#define ADC_ISR_ADRDY        BIT(0)
#define ADC_ISR_EOC          BIT(2)

#define USB_BASE             0x4000D400UL
#define USB_CNTR             MMIO32(USB_BASE + 0x40UL)
#define USB_ISTR             MMIO32(USB_BASE + 0x44UL)
#define USB_DADDR            MMIO32(USB_BASE + 0x4CUL)
#define USB_BCDR             MMIO32(USB_BASE + 0x58UL)
#define USB_CNTR_RESETM      BIT(10)
#define USB_CNTR_CTRM        BIT(15)
#define USB_DADDR_EF         BIT(7)
#define USB_BCDR_DPPU        BIT(15)

/* MLX90640 register definitions. */
#define MLX90640_RAM_START       0x0400U
#define MLX90640_EEPROM_START    0x2400U
#define MLX90640_STATUS          0x8000U
#define MLX90640_CONTROL         0x800DU
#define MLX_STATUS_NEW_DATA      0x0008U
#define MLX_STATUS_SUBPAGE       0x0001U
#define MLX_CTRL_REFRESH_MASK    0x0380U
#define MLX_CTRL_REFRESH_8HZ     0x0400U
#define MLX_CTRL_CHESS_MODE      0x1000U
#define MLX_CTRL_SUBPAGE_REPEAT  0x0008U

/* SHT45 commands, sent MSB-first without a register address. */
#define SHT45_CMD_MEASURE_HIGH   0xFD00U
#define SHT45_CMD_HEATER_200MW   0x3900U
#define SHT45_CMD_SOFT_RESET     0x9400U
#define SHT45_CONVERSION_MS      10U

/* VL53L4CD compact register set. */
#define VL53_IDENTIFICATION_MODEL_ID       0x010FU
#define VL53_SYSTEM_START                  0x0087U
#define VL53_GPIO_TIO_HV_STATUS            0x0031U
#define VL53_RESULT_RANGE_STATUS           0x0089U
#define VL53_RESULT_DISTANCE_MM            0x0096U
#define VL53_RANGE_CONFIG_A                0x005EU
#define VL53_RANGE_CONFIG_B                0x0061U
#define VL53_INTERMEASUREMENT_MS            0x006CU
#define VL53_MODEL_ID_EXPECTED                 0xEBAAU

/* MAX17048 fuel gauge. */
#define MAX17048_REG_VCELL       0x02U
#define MAX17048_REG_SOC         0x04U
#define MAX17048_REG_MODE        0x06U
#define MAX17048_REG_VERSION     0x08U
#define MAX17048_REG_CONFIG      0x0CU
#define MAX17048_REG_COMMAND     0xFEU
#define MAX17048_QUICKSTART      0x4000U

/* CondenScope binary protocol. */
#define CS_FRAME_MAGIC           0x4353U
#define CS_PROTOCOL_VERSION      0x01U
#define CS_MSG_HELLO             0x01U
#define CS_MSG_STATUS            0x02U
#define CS_MSG_THERMAL_FRAME     0x03U
#define CS_MSG_MAP_POINT         0x04U
#define CS_MSG_SESSION_SUMMARY   0x05U
#define CS_CMD_START_SESSION     0x40U
#define CS_CMD_STOP_SESSION      0x41U
#define CS_CMD_SET_EMISSIVITY    0x42U
#define CS_CMD_SET_THRESHOLDS    0x43U
#define CS_CMD_CALIBRATE         0x44U
#define CS_CMD_ERASE             0x45U
#define CS_MSG_ACK               0x7EU
#define CS_MSG_ERROR             0x7FU

#endif
