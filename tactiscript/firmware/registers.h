/*
 * registers.h — TactiScript peripheral register definitions
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * Covers: nRF5340 APP-core peripherals, DRV2700 piezo driver,
 * ICM-42688-P IMU, and SSD1316 OLED.
 */

#ifndef TACTISCRIPT_REGISTERS_H
#define TACTISCRIPT_REGISTERS_H

#include <stdint.h>

/* ================================================================
 * nRF5340 APP-core base addresses (selected peripherals)
 * ================================================================ */

#define NRF_PERIPH_BASE       0x40000000UL

/* Clock management */
#define NRF_CLOCK_BASE        (NRF_PERIPH_BASE + 0x0000)
#define NRF_CLOCK_TASKS_HFCLKSTART  (NRF_CLOCK_BASE + 0x000)
#define NRF_CLOCK_TASKS_HFCLKSTOP   (NRF_CLOCK_BASE + 0x004)
#define NRF_CLOCK_EVENTS_HFCLKSTARTED (NRF_CLOCK_BASE + 0x100)
#define NRF_CLOCK_TASKS_LFCLKSTART (NRF_CLOCK_BASE + 0x008)
#define NRF_CLOCK_EVENTS_LFCLKSTARTED (NRF_CLOCK_BASE + 0x104)
#define NRF_CLOCK_LFCLKSRC    (NRF_CLOCK_BASE + 0x518)

/* GPIO (port 0 and 1) */
#define NRF_GPIO_P0_BASE      (NRF_PERIPH_BASE + 0x8400)
#define NRF_GPIO_P1_BASE      (NRF_PERIPH_BASE + 0x8400 + 0x300)
#define GPIO_OUT(base)        (*(volatile uint32_t *)(base + 0x004))
#define GPIO_OUTSET(base)     (*(volatile uint32_t *)(base + 0x008))
#define GPIO_OUTCLR(base)     (*(volatile uint32_t *)(base + 0x00C))
#define GPIO_IN(base)         (*(volatile uint32_t *)(base + 0x010))
#define GPIO_DIRSET(base)     (*(volatile uint32_t *)(base + 0x018))
#define GPIO_DIRCLR(base)     (*(volatile uint32_t *)(base + 0x01C))
#define GPIO_PIN_CNF(base, n) (*(volatile uint32_t *)(base + 0x700 + 0x4*(n)))

/* GPIOTE (GPIO tasks/events) */
#define NRF_GPIOTE_BASE       (NRF_PERIPH_BASE + 0x4300)
#define GPIOTE_CONFIG(n)      (*(volatile uint32_t *)(NRF_GPIOTE_BASE + 0x200 + 4*(n)))
#define GPIOTE_IN(n)          (*(volatile uint32_t *)(NRF_GPIOTE_BASE + 0x100 + 4*(n)))
#define GPIOTE_INTENSET       (*(volatile uint32_t *)(NRF_GPIOTE_BASE + 0x308))

/* SPI (SPIM0 and SPIM1) */
#define NRF_SPIM0_BASE        (NRF_PERIPH_BASE + 0x4000)
#define NRF_SPIM1_BASE        (NRF_PERIPH_BASE + 0x5000)
#define SPIM_TXD_PTR(b)       (*(volatile uint32_t *)(b + 0x510))
#define SPIM_TXD_MAXCNT(b)    (*(volatile uint32_t *)(b + 0x518))
#define SPIM_RXD_PTR(b)       (*(volatile uint32_t *)(b + 0x524))
#define SPIM_RXD_MAXCNT(b)    (*(volatile uint32_t *)(b + 0x52C))
#define SPIM_FREQUENCY(b)    (*(volatile uint32_t *)(b + 0x524 + 0x10))
#define SPIM_CONFIG(b)       (*(volatile uint32_t *)(b + 0x554))
#define SPIM_PSEL_SCK(b)     (*(volatile uint32_t *)(b + 0x508))
#define SPIM_PSEL_MOSI(b)    (*(volatile uint32_t *)(b + 0x50C))
#define SPIM_PSEL_MISO(b)    (*(volatile uint32_t *)(b + 0x510))
#define SPIM_ENABLE(b)       (*(volatile uint32_t *)(b + 0x500))
#define SPIM_TASKS_START(b)  (*(volatile uint32_t *)(b + 0x008))
#define SPIM_TASKS_STOP(b)   (*(volatile uint32_t *)(b + 0x00C))
#define SPIM_EVENTS_END(b)   (*(volatile uint32_t *)(b + 0x108))
#define SPIM_SHORTS(b)       (*(volatile uint32_t *)(b + 0x200))

/* TWI (I2C) */
#define NRF_TWIM0_BASE        (NRF_PERIPH_BASE + 0x6000)
#define TWIM_PSEL_SCL(b)      (*(volatile uint32_t *)(b + 0x508))
#define TWIM_PSEL_SDA(b)      (*(volatile uint32_t *)(b + 0x50C))
#define TWIM_ENABLE(b)        (*(volatile uint32_t *)(b + 0x500))
#define TWIM_TXD_PTR(b)       (*(volatile uint32_t *)(b + 0x510))
#define TWIM_TXD_MAXCNT(b)    (*(volatile uint32_t *)(b + 0x518))
#define TWIM_RXD_PTR(b)       (*(volatile uint32_t *)(b + 0x524))
#define TWIM_RXD_MAXCNT(b)    (*(volatile uint32_t *)(b + 0x52C))
#define TWIM_TASKS_STARTTX(b) (*(volatile uint32_t *)(b + 0x008))
#define TWIM_TASKS_STARTRX(b) (*(volatile uint32_t *)(b + 0x00C))
#define TWIM_EVENTS_END(b)    (*(volatile uint32_t *)(b + 0x108))
#define TWIM_EVENTS_STOPPED(b) (*(volatile uint32_t *)(b + 0x104))
#define TWIM_TASKS_STOP(b)    (*(volatile uint32_t *)(b + 0x014))

/* SAADC (analog-digital converter) */
#define NRF_SAADC_BASE        (NRF_PERIPH_BASE + 0x4000 + 0x400)
#define SAADC_TASKS_START     (*(volatile uint32_t *)(NRF_SAADC_BASE + 0x000))
#define SAADC_TASKS_SAMPLE    (*(volatile uint32_t *)(NRF_SAADC_BASE + 0x004))
#define SAADC_EVENTS_END      (*(volatile uint32_t *)(NRF_SAADC_BASE + 0x100))
#define SAADC_ENABLE          (*(volatile uint32_t *)(NRF_SAADC_BASE + 0x500))
#define SAADC_RESULT_PTR      (*(volatile uint32_t *)(NRF_SAADC_BASE + 0x62C))
#define SAADC_RESULT_MAXCNT   (*(volatile uint32_t *)(NRF_SAADC_BASE + 0x630))

/* Timer (TIMER0 for actuator refresh) */
#define NRF_TIMER0_BASE       (NRF_PERIPH_BASE + 0x5000 + 0x4000)
#define TIMER_TASKS_START(b)  (*(volatile uint32_t *)(b + 0x000))
#define TIMER_TASKS_STOP(b)   (*(volatile uint32_t *)(b + 0x004))
#define TIMER_TASKS_CLEAR(b)   (*(volatile uint32_t *)(b + 0x008))
#define TIMER_EVENTS_COMPARE(b, n) (*(volatile uint32_t *)(b + 0x100 + 4*(n)))
#define TIMER_INTENSET(b)     (*(volatile uint32_t *)(b + 0x304))
#define TIMER_MODE(b)         (*(volatile uint32_t *)(b + 0x504))
#define TIMER_BITMODE(b)      (*(volatile uint32_t *)(b + 0x508))
#define TIMER_PRESCALER(b)    (*(volatile uint32_t *)(b + 0x50C))
#define TIMER_CC(b, n)        (*(volatile uint32_t *)(b + 0x510 + 4*(n)))

/* RTC (low-power timer for system tick) */
#define NRF_RTC0_BASE         (NRF_PERIPH_BASE + 0x4000 + 0xB000)
#define RTC_TASKS_START(b)    (*(volatile uint32_t *)(b + 0x000))
#define RTC_EVENTS_TICK(b)    (*(volatile uint32_t *)(b + 0x100))
#define RTC_INTENSET(b)       (*(volatile uint32_t *)(b + 0x304))
#define RTC_PRESCALER(b)      (*(volatile uint32_t *)(b + 0x508))

/* PWM (for LRA and HV gate) */
#define NRF_PWM0_BASE         (NRF_PERIPH_BASE + 0x4000 + 0x21000)
#define PWM_TASKS_START(b)    (*(volatile uint32_t *)(b + 0x000))
#define PWM_TASKS_STOP(b)     (*(volatile uint32_t *)(b + 0x004))
#define PWM_EVENTS_SEQEND0(b) (*(volatile uint32_t *)(b + 0x110))
#define PWM_ENABLE(b)         (*(volatile uint32_t *)(b + 0x500))
#define PWM_COUNTERTOP(b)     (*(volatile uint32_t *)(b + 0x508))
#define PWM_SEQ_PTR(b)        (*(volatile uint32_t *)(b + 0x520))
#define PWM_SEQ_CNT(b)        (*(volatile uint32_t *)(b + 0x524))

/* NVIC (interrupt controller) */
#define NRF_NVIC_ISER0        (*(volatile uint32_t *)0xE000E100)
#define NRF_NVIC_ICER0        (*(volatile uint32_t *)0xE000E180)
#define NRF_NVIC_ISPR0        (*(volatile uint32_t *)0xE000E200)
#define NRF_NVIC_ICPR0        (*(volatile uint32_t *)0xE000E280)

/* SysTick */
#define SYST_CSR             (*(volatile uint32_t *)0xE000E010)
#define SYST_RVR             (*(volatile uint32_t *)0xE000E014)
#define SYST_CVR             (*(volatile uint32_t *)0xE000E018)

/* ================================================================
 * DRV2700 Piezo Driver — register map (SPI)
 * ================================================================ */

#define DRV2700_REG_CTRL1     0x01  /* Control register 1 */
#define DRV2700_REG_CTRL2     0x02  /* Control register 2 */
#define DRV2700_REG_VGAIN     0x03  /* Voltage gain setting */
#define DRV2700_REG_WAVEFORM1 0x04  /* Waveform peak 1 */
#define DRV2700_REG_WAVEFORM2 0x05  /* Waveform peak 2 */
#define DRV2700_REG_STATUS    0x06  /* Status / fault */
#define DRV2700_REG_GO        0x0B  /* Trigger output */

/* CTRL1 bits */
#define DRV2700_EN_HVBOOST    0x01
#define DRV2700_EN_OUT1       0x02
#define DRV2700_EN_OUT2       0x04
#define DRV2700_RSTN          0x08

/* CTRL2 bits */
#define DRV2700_MODE_RTP      0x00  /* real-time play mode */
#define DRV2700_MODE_SEQ      0x01  /* sequence mode */
#define DRV2700_MODE_CONT     0x02  /* continuous trigger */
#define DRV2700_INT_TRIG      0x04  /* internal trigger */
#define DRV2700_N polarity    0x08

/* VGAIN: maps to voltage output 0-100 V with gain 0-127 */
#define DRV2700_VGAIN_120V    0x7F  /* max gain ≈ 120 V */

/* Status bits */
#define DRV2700_STAT_HVOK     0x01  /* HV rail in regulation */
#define DRV2700_STAT_OC       0x02  /* over-current fault */
#define DRV2700_STAT_OV       0x04  /* over-voltage fault */
#define DRV2700_STAT_BUSY     0x08  /* output busy */

/* ================================================================
 * ICM-42688-P IMU — register map (SPI, 4-wire)
 * ================================================================ */

#define IMU_REG_WHO_AM_I      0x75  /* should read 0x47 */
#define IMU_REG_DEVICE_CONFIG 0x11
#define IMU_REG_PWR_MGMT0     0x4E
#define IMU_REG_GYRO_CONFIG0  0x4F
#define IMU_REG_ACCEL_CONFIG0 0x50
#define IMU_REG_GYRO_CONFIG1  0x51
#define IMU_REG_ACCEL_CONFIG1 0x51
#define IMU_REG_INT_CONFIG    0x14
#define IMU_REG_INT_CONFIG0   0x63
#define IMU_REG_INT_SOURCE0   0x65
#define IMU_REG_FIFO_CONFIG   0x66
#define IMU_REG_FIFO_COUNT    0x2E
#define IMU_REG_FIFO_DATA     0x30
#define IMU_REG_TEMP_DATA1    0x1D
#define IMU_REG_TEMP_DATA0    0x1E
#define IMU_REG_ACCEL_DATA_X1 0x1F
#define IMU_REG_ACCEL_DATA_X0 0x20
#define IMU_REG_ACCEL_DATA_Y1 0x21
#define IMU_REG_ACCEL_DATA_Y0 0x22
#define IMU_REG_ACCEL_DATA_Z1 0x23
#define IMU_REG_ACCEL_DATA_Z0 0x24
#define IMU_REG_GYRO_DATA_X1  0x25
#define IMU_REG_GYRO_DATA_X0  0x26
#define IMU_REG_GYRO_DATA_Y1  0x27
#define IMU_REG_GYRO_DATA_Y0 0x28
#define IMU_REG_GYRO_DATA_Z1  0x29
#define IMU_REG_GYRO_DATA_Z0  0x2A
#define IMU_REG_INT_STATUS    0x2D
#define IMU_REG_INT_STATUS3   0x2F

#define IMU_WHO_AM_I_VAL      0x47

/* PWR_MGMT0 bits */
#define IMU_PWR_GYRO_LN       0x0C  /* Low-noise gyro mode */
#define IMU_PWR_ACCEL_LN      0x03  /* Low-noise accel mode */
#define IMU_PWR_TEMP_DIS      0x00  /* temp enabled */

/* GYRO_CONFIG0 ODR bits (upper nibble) */
#define IMU_ODR_1KHZ          (0x06 << 4)
#define IMU_ODR_500HZ         (0x07 << 4)
#define IMU_GYRO_RANGE_2000DPS (0x00 << 0)
#define IMU_GYRO_RANGE_1000DPS (0x01 << 0)
#define IMU_GYRO_RANGE_500DPS  (0x02 << 0)

/* ACCEL_CONFIG0 ODR + range */
#define IMU_ACCEL_RANGE_8G    (0x02 << 0)
#define IMU_ACCEL_RANGE_16G   (0x00 << 0)

/* INT_SOURCE0 — route tap/swipe to INT1 */
#define IMU_INT1_TAP          0x04
#define IMU_INT1_DATA_RDY     0x08

/* SPI read/write: MSB of address byte: 1=read, 0=write */
#define IMU_SPI_READ(addr)    (0x80 | (addr))
#define IMU_SPI_WRITE(addr)   (addr & 0x7F)

/* FIFO data format: 1 byte header + 6 bytes accel + 6 bytes gyro = 16 bytes
 * (when both are enabled, plus 2 bytes temp in some configs). We use the
 * 14-byte packet format: 1 header + 6 accel + 6 gyro + 1 timestamp.
 */
#define IMU_FIFO_PACKET_LEN   16

/* ================================================================
 * SSD1316 OLED controller — commands
 * ================================================================ */

#define OLED_CMD_SET_CONTRAST     0x81
#define OLED_CMD_DISPLAY_ON       0xAF
#define OLED_CMD_DISPLAY_OFF      0xAE
#define OLED_CMD_NORMAL_DISPLAY   0xA4
#define OLED_CMD_INVERT_DISPLAY   0xA7
#define OLED_CMD_SET_MUX          0xA8
#define OLED_CMD_SET_DISPLAY_OFFSET 0xD3
#define OLED_CMD_SET_SEG_REMAP    0xA0
#define OLED_CMD_SET_COM_SCAN_NORMAL 0xC0
#define OLED_CMD_SET_COM_SCAN_REMAP  0xC8
#define OLED_CMD_SET_OSC          0xD5
#define OLED_CMD_SET_PRECHARGE    0xD9
#define OLED_CMD_SET_VCOMH        0xDB
#define OLED_CMD_SET_CHARGE_PUMP  0x8D
#define OLED_CMD_SET_COL_ADDR     0x21
#define OLED_CMD_SET_PAGE_ADDR    0x22
#define OLED_CMD_SET_START_LINE   0x40
#define OLED_CMD_RAM_WRITE        0x40  /* follow with data bytes */

/* ================================================================
 * nPM1300 PMIC — I2C register map (selected)
 * ================================================================ */

#define NPM1300_I2C_ADDR     0x6B
#define NPM1300_REG_MAINSTATUS 0x00
#define NPM1300_REG_BATV      0x03
#define NPM1300_REG_BATICHG   0x04
#define NPM1300_REG_CHGSTATUS 0x0A
#define NPM1300_REG_CHGCONFIG 0x1D
#define NPM1300_REG_SHIPRESET 0x0E

/* Main status bits */
#define NPM1300_STAT_AC       0x01  /* external power present */
#define NPM1300_STAT_VBUS     0x02  /* USB VBUS present */
#define NPM1300_STAT_QI       0x04  /* Qi charging active */

/* Charge status bits */
#define NPM1300_CHG_COMPLETE 0x01
#define NPM1300_CHG_TRICKLE  0x02
#define NPM1300_CHG_CONST_C  0x04
#define NPM1300_CHG_TERM     0x08

/* ================================================================
 * Helper: GPIO port base from port number
 * ================================================================ */
#define GPIO_PORT_BASE(p) ((p) == 0 ? NRF_GPIO_P0_BASE : NRF_GPIO_P1_BASE)

/* Pin number from (port, pin) pair */
#define GPIO_PIN(port, pin) (((port) << 5) | (pin))

#endif /* TACTISCRIPT_REGISTERS_H */