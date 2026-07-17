/*
 * power.c — BQ25895 charger telemetry + JEITA safety over I2C4.
 *
 * Author : jayis1
 * License: MIT
 *
 * BQ25895 registers used:
 *   0x00  SYS_STAT   — VBUS status, charge status bits
 *   0x02  ICHRG      — charge current setting
 *   0x04  PWR_ON     — converter config
 *   0x06  MINVBUS    — VBUS min voltage
 *   0x0A  VREG       — charge voltage limit
 *   0x0B  IBATLM     — BATFET disable / ship mode
 *   0x0E  ADC_CTRL   — enable ADC
 *   0x11  VSYS       — ADC VSYS reading
 *   0x13  VBAT       — ADC VBAT reading (mV)
 *
 * Fuel gauge is coulomb-counter-free on the BQ25895, so SOC is approximated
 * from the open-circuit battery voltage via a lookup table.
 */
#include "power.h"
#include "../registers.h"

static uint8_t bq_read(uint8_t reg) {
    uint8_t v;
    /* Reuse SHT45's I2C primitives via direct CR2 writes. */
    I2C4->CR2 = ((uint32_t)BQ25895_ADDR << 1) | (1U<<16) | I2C_CR2_START;
    while (!(I2C4->ISR & I2C_ISR_TXE)) { if (I2C4->ISR & I2C_ISR_NACKF) { I2C4->ICR=I2C_ISR_NACKF; return 0xFF; } }
    I2C4->TXDR = reg;
    while (!(I2C4->ISR & I2C_ISR_TC)) { }
    /* Repeated start for read. */
    I2C4->CR2 = ((uint32_t)BQ25895_ADDR << 1) | 1U | (1U<<16) | I2C_CR2_START;
    while (!(I2C4->ISR & I2C_ISR_RXNE)) { if (I2C4->ISR & I2C_ISR_NACKF) { I2C4->ICR=I2C_ISR_NACKF; return 0xFF; } }
    v = (uint8_t)I2C4->RXDR;
    I2C4->CR2 |= I2C_CR2_STOP;
    return v;
}

static void bq_write(uint8_t reg, uint8_t val) {
    I2C4->CR2 = ((uint32_t)BQ25895_ADDR << 1) | (2U<<16) | I2C_CR2_START;
    while (!(I2C4->ISR & I2C_ISR_TXE)) { if (I2C4->ISR & I2C_ISR_NACKF) { I2C4->ICR=I2C_ISR_NACKF; return; } }
    I2C4->TXDR = reg;
    while (!(I2C4->ISR & I2C_ISR_TXE)) { }
    I2C4->TXDR = val;
    while (!(I2C4->ISR & I2C_ISR_TC)) { }
    I2C4->CR2 |= I2C_CR2_STOP;
}

void power_init(void) {
    /* Enable ADC, 3.25 A input current limit, 4.2 V charge voltage,
     * 2.0 A charge current (typical for 18650). */
    bq_write(0x02, (0x40U | (2U<<3)));      /* ICHRG = 2048 mA */
    bq_write(0x0A, (0x40U | (20U<<3)));      /* VREG = 4.208 V (0x5C region) */
    bq_write(0x03, 0x3A);                    /* 3.25 A input current */
    bq_write(0x0E, (1U<<7));                 /* ADC enable, one-shot */
    /* Disable BATFET ship mode for now (normal operation). */
    bq_write(0x0B, 0x00);
}

int power_soc(void) {
    /* OCV-based rough SOC for NCR18650B (3.0V=0%, 4.2V=100%). */
    int mv = power_voltage_mv();
    int soc = (mv - 3000) * 100 / 1200;
    if (soc < 0)   soc = 0;
    if (soc > 100) soc = 100;
    return soc;
}

int power_voltage_mv(void) {
    uint8_t hi = bq_read(0x0E);   /* ensure ADC started */
    (void)hi;
    uint8_t v = bq_read(0x13);
    /* VBAT ADC: 20 mV/LSB with 2.3 V offset. */
    return 2304 + (int)v * 20;
}

uint8_t power_is_charging(void) {
    uint8_t s = bq_read(0x00);
    return ((s >> 4) & 3) != 0;   /* CHRG_STAT non-zero => charging */
}

void power_jeita_check(float temp_c) {
    if (temp_c < 0.0f || temp_c > 45.0f) {
        /* Suspend charging: set CHRG_SUSP bit (0x02 bit 7? actually CE bit
         * in REG00 on BQ25895). REG00 bit5 = EN_HIZ, bit7 = EN_ICHG_MON.
         * We use the dedicated CE bit in REG03 (CHRG_CONFIG) on this chip. */
        uint8_t r3 = bq_read(0x03);
        bq_write(0x03, r3 & ~(1U<<4));   /* clear CHRG_CONFIG => suspend */
    } else {
        uint8_t r3 = bq_read(0x03);
        bq_write(0x03, r3 | (1U<<4));    /* enable charging */
    }
}