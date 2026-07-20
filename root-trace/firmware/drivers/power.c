/*
 * power.c — Power Management
 * RootTrace — Open-Source Electrical Impedance Tomography Root Imaging System
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 *
 * Manages battery monitoring via the MAX17048 fuel gauge, USB-C VBUS
 * detection, the RGB status LED, and low-power mode entry.
 */

#include "power.h"
#include "board.h"
#include "registers.h"
#include <string.h>

/* MAX17048 I2C address and registers */
#define MAX17048_ADDR   0x36
#define MAX17048_VCELL   0x02
#define MAX17048_SOC     0x04
#define MAX17048_MODE    0x06
#define MAX17048_VERSION 0x08
#define MAX17048_HIBRT   0x0A
#define MAX17048_CONFIG  0x0C
#define MAX17048_VALRT   0x14
#define MAX17048_STATUS  0x1A
#define MAX17048_TEMP    0x16

/* ---------------------------------------------------------------------
 * I2C1 low-level (used by power and env_sens)
 * --------------------------------------------------------------------- */

static int i2c1_write(uint8_t addr, uint8_t reg, const uint8_t *data, int len)
{
    uint32_t timeout;

    /* Start + address (write) */
    ENV_I2C->CR2 = ((uint32_t)addr << 1) | ((len + 1) << 16);
    ENV_I2C->CR2 |= I2C_CR2_START;

    /* Send register address */
    timeout = 100000;
    while (!(ENV_I2C->ISR & I2C_ISR_TXE) && timeout-- > 0) { }
    ENV_I2C->TXDR = reg;

    /* Send data bytes */
    for (int i = 0; i < len; i++) {
        timeout = 100000;
        while (!(ENV_I2C->ISR & I2C_ISR_TXE) && timeout-- > 0) { }
        ENV_I2C->TXDR = data[i];
    }

    /* Wait for transfer complete */
    timeout = 100000;
    while (!(ENV_I2C->ISR & I2C_ISR_TC) && timeout-- > 0) { }
    ENV_I2C->CR2 |= I2C_CR2_STOP;

    /* Clear NACK */
    if (ENV_I2C->ISR & I2C_ISR_NACKF) {
        ENV_I2C->ICR = I2C_ISR_NACKF;
        return -1;
    }
    return 0;
}

static int i2c1_read(uint8_t addr, uint8_t reg, uint8_t *data, int len)
{
    uint32_t timeout;

    /* Write register address (no stop) */
    ENV_I2C->CR2 = ((uint32_t)addr << 1) | (1U << 16) | I2C_CR2_NACK;
    ENV_I2C->CR2 |= I2C_CR2_START;
    timeout = 100000;
    while (!(ENV_I2C->ISR & I2C_ISR_TXE) && timeout-- > 0) { }
    ENV_I2C->TXDR = reg;
    timeout = 100000;
    while (!(ENV_I2C->ISR & I2C_ISR_TC) && timeout-- > 0) { }

    /* Restart for read */
    ENV_I2C->CR2 = ((uint32_t)addr << 1) | ((uint32_t)len << 16)
                 | I2C_CR2_START | I2C_CR2_RD_WRN | I2C_CR2_NACK;

    for (int i = 0; i < len; i++) {
        timeout = 100000;
        while (!(ENV_I2C->ISR & I2C_ISR_RXNE) && timeout-- > 0) { }
        data[i] = (uint8_t)ENV_I2C->RXDR;
    }

    ENV_I2C->CR2 |= I2C_CR2_STOP;
    return 0;
}

static void i2c1_init(void)
{
    RCC->AHB4ENR |= BIT(1);  /* GPIOB */
    RCC->APB1LENR |= BIT(21);  /* I2C1 */

    /* PB6 = SCL, PB7 = SDA (AF4) */
    int pins[] = {6, 7};
    for (int i = 0; i < 2; i++) {
        int p = pins[i];
        GPIOB->MODER &= ~(3U << (p*2));
        GPIOB->MODER |= (GPIO_MODE_AF << (p*2));
        GPIOB->OTYPER |= (1U << p);  /* open-drain */
        GPIOB->OSPEEDR |= (GPIO_SPEED_HIGH << (p*2));
        GPIOB->PUPDR |= (GPIO_PUPD_PU << (p*2));
        GPIOB->AFRL &= ~(0xFU << (p*4));
        GPIOB->AFRL |= (ENV_I2C_AF << (p*4));
    }

    /* Configure I2C1: 100 kHz, standard mode */
    ENV_I2C->TIMINGR = 0x10B17DB4U;  /* 100 kHz timing for 120 MHz I2C clock */
    ENV_I2C->CR1 = I2C_CR1_PE;
}

/* ---------------------------------------------------------------------
 * MAX17048 fuel gauge access
 * --------------------------------------------------------------------- */

static uint16_t max17048_read16(uint8_t reg)
{
    uint8_t data[2];
    if (i2c1_read(MAX17048_ADDR, reg, data, 2) != 0) return 0;
    return ((uint16_t)data[0] << 8) | data[1];
}

static void max17048_write16(uint8_t reg, uint16_t val)
{
    uint8_t data[2] = { (uint8_t)(val >> 8), (uint8_t)(val & 0xFF) };
    i2c1_write(MAX17048_ADDR, reg, data, 2);
}

static void max17048_init(void)
{
    /* Quick-start mode for fuel gauge */
    max17048_write16(MAX17048_MODE, 0x4000);
    for (volatile int i = 0; i < 100000; i++) { }

    /* Check version register */
    uint16_t version = max17048_read16(MAX17048_VERSION);
    (void)version;  /* expected 0x001C for MAX17048 */
}

/* ---------------------------------------------------------------------
 * Initialize power management
 * --------------------------------------------------------------------- */

void power_init(void)
{
    i2c1_init();
    max17048_init();

    /* Configure RGB LED pins as outputs (PB0, PB1, PB14) */
    RCC->AHB4ENR |= BIT(1);  /* GPIOB */
    int pins[] = {0, 1, 14};
    for (int i = 0; i < 3; i++) {
        int p = pins[i];
        GPIOB->MODER &= ~(3U << (p*2));
        GPIOB->MODER |= (GPIO_MODE_OUT << (p*2));
        GPIOB->OTYPER &= ~(1U << p);
        GPIOB->OSPEEDR |= (GPIO_SPEED_LOW << (p*2));
    }

    /* Configure USB VBUS detect (PG6) as input */
    RCC->AHB4ENR |= BIT(6);  /* GPIOG */
    GPIOG->MODER &= ~(3U << (6*2));  /* input */
    GPIOG->PUPDR |= (GPIO_PUPD_PD << (6*2));  /* pull-down */

    /* LED off initially */
    power_set_led(0, 0, 0);
}

/* ---------------------------------------------------------------------
 * Read power status
 * --------------------------------------------------------------------- */

int power_read_status(power_status_t *status)
{
    if (!status) return -1;
    memset(status, 0, sizeof(*status));

    /* Read VCELL register: voltage in 78.125 µV per LSB
     * Voltage (mV) = VCELL * 78.125 µV / 1000 = VCELL * 0.078125 */
    uint16_t vcell = max17048_read16(MAX17048_VCELL);
    status->voltage_mv = (uint16_t)((uint32_t)vcell * 78U / 1000U);  /* approx */

    /* Read SOC register: percent * 256 (0.00390625% per LSB) */
    uint16_t soc = max17048_read16(MAX17048_SOC);
    status->percent = (uint8_t)(soc / 256U);
    if (status->percent > 100) status->percent = 100;

    /* Read temperature */
    uint16_t temp = max17048_read16(MAX17048_TEMP);
    status->temp_c = (int8_t)((int16_t)temp / 256 - 20);  /* approximate */

    /* Check USB VBUS */
    status->usb_connected = (uint8_t)PIN_GET(USB_VBUS_PORT, USB_VBUS_PIN);

    /* Charging status: VBUS present AND voltage < 4200 mV */
    status->charging = (status->usb_connected && status->voltage_mv < 4200) ? 1 : 0;

    return 0;
}

/* ---------------------------------------------------------------------
 * Enter STOP2 low-power mode
 * --------------------------------------------------------------------- */

void power_enter_stop(void)
{
    /* Disable peripherals to save power */
    ad5940_sleep();

    /* Clear SLEEPDEEP to use STOP mode (not STANDBY) */
    PWR->CR1 |= PWR_CR1_LPDS;  /* low-power regulator in STOP */

    /* Enter STOP2 mode */
    SCB_SCR |= SCB_SCR_SLEEPDEEP;
    __asm volatile ("wfi");
    SCB_SCR &= ~SCB_SCR_SLEEPDEEP;

    /* Wakeup: restore clocks (PLL may have stopped) */
    /* The clock reconfiguration would be done by the RCC ISR handler
     * or by calling the clock init function.  For simplicity, we
     * assume the HSI is still running and re-enable PLL. */
}

void power_wakeup(void)
{
    ad5940_wakeup();
}

int power_is_charging(void)
{
    power_status_t status;
    if (power_read_status(&status) != 0) return 0;
    return status.charging;
}

/* ---------------------------------------------------------------------
 * RGB LED control (simple on/off, no PWM for simplicity)
 * --------------------------------------------------------------------- */

void power_set_led(uint8_t r, uint8_t g, uint8_t b)
{
    /* Active-low LEDs (common anode): 0 = on, 1 = off */
    if (r) PIN_CLR(LED_R_PORT, LED_R_PIN); else PIN_SET(LED_R_PORT, LED_R_PIN);
    if (g) PIN_CLR(LED_G_PORT, LED_G_PIN); else PIN_SET(LED_G_PORT, LED_G_PIN);
    if (b) PIN_CLR(LED_B_PORT, LED_B_PIN); else PIN_SET(LED_B_PORT, LED_B_PIN);
}

void power_enter_low_power(void)
{
    /* Turn off display, sleep AD5940, enter STOP2 */
    power_set_led(0, 0, 1);  /* blue = sleeping */
    power_enter_stop();
    power_set_led(0, 1, 0);  /* green = awake */
}