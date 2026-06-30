/*
 * drivers/power.c — Power management driver for SpectraPest
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * Manages the MAX77654 PMIC and MAX17055 fuel gauge via I2C1 (shared
 * with environmental sensors). Controls the power state machine that
 * transitions between SLEEP, WAKE_ENV, ACTIVE_CAPTURE, and MESH_RX
 * states to minimize energy consumption.
 *
 * The MAX77654 provides:
 *   - Solar MPPT charge controller (up to 2A charge current)
 *   - Two LDOs (3.3V for MCU, 5V for peripherals)
 *   - Programmable watchdog
 *
 * The MAX17055 provides:
 *   - ModelGauge m5 EZ fuel gauge (1% accuracy for LiFePO4)
 *   - State of charge (SOC) percentage
 *   - Battery voltage, current, temperature
 *   - Low-battery alerts
 *
 * Power state transitions:
 *
 *   SLEEP ──(timer 60s)──→ WAKE_ENV ──(temp>10°C && light>100lux)──→ ACTIVE_CAPTURE
 *    ↑                       │                                          │
 *    └──(always)──────────────┴──────(after capture + mesh TX)─────────┘
 *
 *   SLEEP ──(LoRa RX window)──→ MESH_RX ──(200ms)──→ SLEEP
 */

#include "power.h"
#include "environment.h"  /* For I2C functions */
#include "../registers.h"
#include "../board.h"

/* MAX17055 register addresses */
#define MAX17055_REG_STATUS     0x00
#define MAX17055_REG_REPCAP      0x05
#define MAX17055_REG_REPSOC      0x06
#define MAX17055_REG_VCELL       0x09
#define MAX17055_REG_CURRENT     0x0A
#define MAX17055_REG_AVG_CURRENT 0x0B
#define MAX17055_REG_TEMP        0x08
#define MAX17055_REG_CONFIG      0x1D
#define MAX17055_REG_DESIGN_CAP  0x18
#define MAX17055_REG_DQACC       0x45
#define MAX17055_REG_DPACC       0x46
#define MAX17055_REG_MODE        0x42
#define MAX17055_REG_HIBRT       0x0A

/* MAX77654 register addresses (subset) */
#define MAX77654_REG_CHG_CNFG_0  0x70
#define MAX77654_REG_CHG_CNFG_1  0x71
#define MAX77654_REG_CHG_CNFG_2  0x72
#define MAX77654_REG_CHG_CNFG_4  0x74
#define MAX77654_REG_CHG_CNFG_7  0x77
#define MAX77654_REG_CHG_STAT     0x78
#define MAX77654_REG_CHG_INT      0x79
#define MAX77654_REG_CNFG_GLBL    0x10
#define MAX77654_REG_CNFG_LDO0   0x20
#define MAX77654_REG_CNFG_LDO1   0x21

static power_state_t g_current_state = PWR_SLEEP;

/* ----------------------------------------------------------------- *
 *  Read 16-bit register from I2C device
 * ----------------------------------------------------------------- */
static int i2c_read16(uint8_t dev_addr, uint8_t reg, uint16_t *out) {
    uint8_t buf[2];
    if (i2c_read_reg(dev_addr, reg, buf, 2) < 0) return -1;
    *out = ((uint16_t)buf[0] << 8) | buf[1];  /* Big-endian for Maxim devices */
    return 0;
}

static int i2c_write16(uint8_t dev_addr, uint8_t reg, uint16_t val) {
    uint8_t buf[3] = { reg, (val >> 8) & 0xFF, val & 0xFF };
    return i2c_write(dev_addr, buf, 3);
}

/* ----------------------------------------------------------------- *
 *  Initialize fuel gauge (MAX17055)
 * ----------------------------------------------------------------- */
int fuel_gauge_init(void) {
    /* Check if device is present */
    uint16_t version;
    if (i2c_read16(I2C_ADDR_MAX17055, 0x08, &version) < 0) return -1;

    /* For LiFePO4, we need to set the custom battery model.
     * In a real implementation, this would write the characterization
     * parameters. For now, use EZ mode (automatic). */

    /* Set design capacity: 1500 mAh → 0x1770 (in 5µV/cell units)
     * For 1500 mAh with 50mΩ sense resistor: 1500 * 0.05 / 5 = 15 → 0x000F */
    i2c_write16(I2C_ADDR_MAX17055, MAX17055_REG_DESIGN_CAP, 0x1770);

    /* Enable EZ mode: set DQACC = DPACC = design cap / 2 */
    i2c_write16(I2C_ADDR_MAX17055, MAX17055_REG_DQACC, 0x0BB8);
    i2c_write16(I2C_ADDR_MAX17055, MAX17055_REG_DPACC, 0x0BB8);

    /* Write MODE register to enable EZ configuration */
    i2c_write16(I2C_ADDR_MAX17055, MAX17055_REG_MODE, 0x0001);

    /* Wait for fuel gauge to settle */
    board_delay_ms(500);

    return 0;
}

int fuel_gauge_read_soc(uint8_t *percent) {
    uint16_t rep_soc;
    if (i2c_read16(I2C_ADDR_MAX17055, MAX17055_REG_REPSOC, &rep_soc) < 0) return -1;
    /* rep_soc is in 1/256% units (high byte = integer %, low byte = fraction) */
    *percent = (uint8_t)(rep_soc >> 8);
    return 0;
}

int fuel_gauge_read_voltage(uint16_t *mv) {
    uint16_t vcell;
    if (i2c_read16(I2C_ADDR_MAX17055, MAX17055_REG_VCELL, &vcell) < 0) return -1;
    /* VCELL is in 78.125 µV per bit → convert to mV */
    *mv = (uint16_t)((float)vcell * 0.078125f);
    return 0;
}

int fuel_gauge_read_temp(int8_t *temp_c) {
    uint16_t temp;
    if (i2c_read16(I2C_ADDR_MAX17055, MAX17055_REG_TEMP, &temp) < 0) return -1;
    /* Temperature in 1/256°C per bit, signed */
    int16_t signed_temp = (int16_t)temp;
    *temp_c = (int8_t)(signed_temp >> 8);
    return 0;
}

/* ----------------------------------------------------------------- *
 *  Initialize PMIC (MAX77654)
 * ----------------------------------------------------------------- */
int pmic_init(void) {
    /* Configure LDO0: 3.3V for MCU (0x3C = 3.3V) */
    i2c_write_reg(I2C_ADDR_MAX77654, MAX77654_REG_CNFG_LDO0, 0x3C | 0x80);

    /* Configure LDO1: 5.0V for peripherals (0x4C = 5.0V) */
    i2c_write_reg(I2C_ADDR_MAX77654, MAX77654_REG_CNFG_LDO1, 0x4C | 0x80);

    /* Configure charger: 500mA charge current, LiFePO4 termination 3.6V */
    /* CHG_CNFG_0: enable charger */
    i2c_write_reg(I2C_ADDR_MAX77654, MAX77654_REG_CHG_CNFG_0, 0x05);

    /* CHG_CNFG_2: charge current = 500mA (0x08 → ~500mA) */
    i2c_write_reg(I2C_ADDR_MAX77654, MAX77654_REG_CHG_CNFG_2, 0x08);

    /* CHG_CNFG_4: termination voltage for LiFePO4 (3.6V) */
    i2c_write_reg(I2C_ADDR_MAX77654, MAX77654_REG_CHG_CNFG_4, 0x18);

    return 0;
}

int pmic_set_charge_current(uint32_t ma) {
    /* CHG_CNFG_2 bits 6-0 = charge current in 50mA steps */
    uint8_t val = (uint8_t)(ma / 50);
    if (val > 0x1F) val = 0x1F;
    return i2c_write_reg(I2C_ADDR_MAX77654, MAX77654_REG_CHG_CNFG_2, val);
}

int pmic_get_charge_status(uint8_t *charging) {
    uint8_t status;
    if (i2c_read_reg(I2C_ADDR_MAX77654, MAX77654_REG_CHG_STAT, &status, 1) < 0) return -1;
    /* Bit 0 = charger actively charging */
    *charging = (status & 0x01) ? 1 : 0;
    return 0;
}

/* ----------------------------------------------------------------- *
 *  Initialize power management
 * ----------------------------------------------------------------- */
int power_init(void) {
    /* Configure PMIC interrupt pin */
    gpio_set_mode(PMIC_INT_PORT, PMIC_INT_PIN, GPIO_MODE_INPUT);

    /* Initialize PMIC and fuel gauge */
    pmic_init();
    fuel_gauge_init();

    g_current_state = PWR_SLEEP;
    return 0;
}

/* ----------------------------------------------------------------- *
 *  Read power status
 * ----------------------------------------------------------------- */
int power_read_status(power_status_t *out) {
    if (!out) return -1;

    fuel_gauge_read_voltage(&out->battery_mv);
    fuel_gauge_read_soc(&out->charge_pct);
    fuel_gauge_read_temp(&out->temp_c);

    /* Read solar voltage from ADC */
    out->solar_mv = power_read_solar_mv();

    pmic_get_charge_status(&out->charging);

    out->state = g_current_state;
    return 0;
}

/* ----------------------------------------------------------------- *
 *  Set power state
 * ----------------------------------------------------------------- */
int power_set_state(power_state_t state) {
    switch (state) {
        case PWR_SLEEP:
            /* Disable peripheral clocks, enter STOP mode */
            RCC->APB1ENR1 &= ~(RCC_APB1ENR1_SPI2EN | RCC_APB1ENR1_USART2EN);
            /* Keep I2C1 for fuel gauge wake */
            /* Disable unused GPIO clocks */
            g_current_state = PWR_SLEEP;
            break;

        case PWR_WAKE_ENV:
            /* Re-enable environmental sensor clocks */
            RCC->APB1ENR1 |= RCC_APB1ENR1_I2C1EN;
            g_current_state = PWR_WAKE_ENV;
            break;

        case PWR_ACTIVE_CAPTURE:
            /* Enable all peripheral clocks for full capture */
            RCC->APB1ENR1 |= RCC_APB1ENR1_SPI2EN | RCC_APB1ENR1_USART2EN;
            RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
            RCC->AHB2ENR |= (1 << 0);  /* DCMI */
            g_current_state = PWR_ACTIVE_CAPTURE;
            break;

        case PWR_MESH_RX:
            /* Enable LoRa radio for receive window */
            RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
            g_current_state = PWR_MESH_RX;
            break;

        case PWR_CHARGING:
            g_current_state = PWR_CHARGING;
            break;
    }
    return 0;
}

power_state_t power_get_state(void) {
    return g_current_state;
}

uint16_t power_read_battery_mv(void) {
    uint16_t mv;
    if (fuel_gauge_read_voltage(&mv) < 0) return 0;
    return mv;
}

uint16_t power_read_solar_mv(void) {
    /* Enable ADC clock */
    RCC->AHB2ENR |= RCC_AHB2ENR_ADC12EN;
    /* Configure ADC channel 5 (PA5) for solar panel voltage via divider
     * Solar: 6V → divider 1:3 → 2V → 12-bit ADC at 3.3V ref = 2482 */
    ADC1->CR = ADC_CR_ADEN;
    ADC1->SQR1 = (5 << 6);  /* Channel 5, 1 conversion */
    ADC1->CR |= ADC_CR_ADSTART;

    while (!(ADC1->ISR & ADC_ISR_EOC));
    uint16_t raw = (uint16_t)ADC1->DR1;

    /* Convert: raw/4095 * 3.3V * 3 (divider) * 1000 = mV */
    return (uint16_t)((float)raw / 4095.0f * 3.3f * 3.0f * 1000.0f);
}

uint8_t power_get_charge_pct(void) {
    uint8_t pct;
    if (fuel_gauge_read_soc(&pct) < 0) return 0;
    return pct;
}

/* ----------------------------------------------------------------- *
 *  Enter STOP mode (lowest power, woken by RTC alarm or EXTI)
 * ----------------------------------------------------------------- */
int power_enter_stop_mode(void) {
    /* Configure all GPIO as analog to minimize leakage */
    /* (In production, this would save ~2mA of GPIO leakage current) */

    /* Set SLEEPDEEP bit in System Control Register */
    *(volatile uint32_t *)0xE000ED10 |= (1 << 2);  /* SCB->SCR SLEEPDEEP */

    /* Enter WFI — will wake on RTC alarm interrupt */
    __asm__("wfi");

    /* Clear SLEEPDEEP after wake */
    *(volatile uint32_t *)0xE000ED10 &= ~(1 << 2);

    return 0;
}

int power_wakeup(void) {
    /* Reconfigure clocks after STOP mode exit */
    /* The PLL needs to be re-enabled after STOP mode */
    RCC->CR |= RCC_CR_PLL1ON;
    while (!(RCC->CR & RCC_CR_PLL1RDY));

    RCC->CFGR = (RCC->CFGR & ~0x3) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS_MASK) != RCC_CFGR_SWS_PLL);

    return 0;
}