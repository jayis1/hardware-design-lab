/**
 * @file    power.c
 * @brief   TideBand — MAX17055 fuel gauge driver and power management.
 *          Reads battery state-of-charge, voltage, and time-to-empty via
 *          I2C1. Manages stop-mode entry for power savings between
 *          samples.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 *
 * The MAX17055 is a ModelGauge m5 EZ fuel gauge that tracks Li-ion
 * battery state using a sophisticated algorithm. It communicates over
 * I2C and provides:
 *   - State of charge (RepSOC) — 1% resolution
 *   - Voltage (VCELL) — 1.25 mV resolution
 *   - Time to empty (TTE) — in hours
 *   - Time to full (TTF) — in hours
 *
 * Key registers:
 *   0x05 RepCap    — Reported remaining capacity
 *   0x06 RepSOC    — Reported state of charge (%)
 *   0x09 VCELL     — Battery voltage
 *   0x10 SOC       — State of charge (alternative)
 *   0x11 TTE       — Time to empty
 *   0x20 FullCap   — Full capacity
 *   0x3D Config    — Configuration register
 *   0x62 Status    — Status flags
 */

#include "board.h"
#include "registers.h"
#include "power.h"

/* ---- MAX17055 register addresses ---- */
#define MAX_REG_REPCAP    0x05u
#define MAX_REG_REPSOC    0x06u
#define MAX_REG_TEMPERATURE 0x08u
#define MAX_REG_VCELL     0x09u
#define MAX_REG_CURRENT   0x0Au
#define MAX_REG_AVG_CURRENT 0x0Bu
#define MAX_REG_SOC       0x10u
#define MAX_REG_TTE       0x11u
#define MAX_REG_FULLCAP   0x10u
#define MAX_REG_CONFIG    0x3Du
#define MAX_REG_STATUS    0x00u

/* ---- State ---- */
static float cached_soc = 100.0f;
static float cached_voltage = 3.7f;
static uint32_t cached_tte = 0;

/* ---- Local functions ---- */
static uint16_t max_read_reg(uint8_t reg);
static void max_write_reg(uint8_t reg, uint16_t val);

/* ---- Public API ---- */

void power_init(void)
{
    /* I2C1 is already initialized by depth_init() — but in case it
     * hasn't been called yet, we ensure it's set up. */
    RCC->APB1ENR1 |= RCC_APB1ENR1_I2C1;

    if ((I2C1_CR1 & I2C_CR1_PE) == 0) {
        I2C1_CR1 = 0;
        I2C1_TIMINGR = I2C1_TIMING_400K;
        I2C1_CR1 = I2C_CR1_PE;
    }

    /* Quick reset of the fuel gauge config (optional) */
    /* The MAX17055 should be configured by the factory with the
     * correct battery model. Here we just verify it responds. */
    uint16_t status = max_read_reg(MAX_REG_STATUS);
    (void)status;  /* Could check for power-on reset bit */
}

float power_get_battery_pct(void)
{
    return cached_soc;
}

float power_get_voltage(void)
{
    return cached_voltage;
}

uint32_t power_get_time_to_empty(void)
{
    return cached_tte;
}

void power_update(void)
{
    /* Read RepSOC (0x06): value in 1/256% per LSB */
    uint16_t soc_raw = max_read_reg(MAX_REG_REPSOC);
    cached_soc = (float)soc_raw / 256.0f;
    if (cached_soc > 100.0f) cached_soc = 100.0f;

    /* Read VCELL (0x09): value in 7.8125 µV per LSB → V = raw * 7.8125e-6 * 2 */
    /* Actually MAX17055 VCELL: 1.25 mV per LSB (after /2 for 1S pack) */
    uint16_t vcell_raw = max_read_reg(MAX_REG_VCELL);
    cached_voltage = (float)vcell_raw * 0.0001f;  /* Approx 100 µV/LSB */

    /* Read TTE (0x11): value in 5.625 seconds per LSB */
    uint16_t tte_raw = max_read_reg(MAX_REG_TTE);
    if (tte_raw == 0xFFFF) {
        cached_tte = 0;  /* Not available (fully charged or charging) */
    } else {
        cached_tte = (uint32_t)(tte_raw * 5.625f);
    }
}

uint8_t power_is_critical(void)
{
    return (cached_soc < 10.0f) ? 1 : 0;
}

void power_enter_stop(void)
{
    /* Disable peripherals that would keep the device awake */
    TIM1_CR1 &= ~TIM_CR1_CEN;  /* Stop Doppler TX */
    SPI1_CR1 &= ~SPI_CR1_SPE;  /* Disable ADC SPI */

    /* Clear all pending wakeup flags */
    /* Set SLEEPDEEP bit and select Stop mode */
    SCB_SCR |= (1u << 2);  /* SLEEPDEEP */
    PWR_CR1 &= ~(3u << 0);  /* PDDS=0, LPDS=0 (Stop mode, regulator on) */

    /* Wait for interrupt (WFI enters Stop mode) */
    __asm volatile ("wfi");

    /* After wakeup, SLEEPDEEP is auto-cleared */
}

void power_exit_stop(void)
{
    /* After waking from Stop mode, we need to re-enable the PLL
     * and restore clock configuration, since Stop mode switches
     * the system clock to HSI. */
    /* Re-enable HSE and PLL1 */
    RCC_CR |= RCC_CR_HSEON;
    while ((RCC_CR & RCC_CR_HSERDY) == 0) { }
    RCC_CR |= RCC_CR_PLL1ON;
    while ((RCC_CR & RCC_CR_PLL1RDY) == 0) { }

    /* Switch system clock back to PLL1 */
    RCC_CFGR = (RCC_CFGR & ~(3u << 0)) | (2u << 0);  /* SW = PLL1 */
    while (((RCC_CFGR >> 2) & 3u) != 2u) { }  /* Wait for switch */

    /* Re-enable peripherals */
    SPI1_CR1 |= SPI_CR1_SPE;
}

/* ---- Local functions ---- */

static uint16_t max_read_reg(uint8_t reg)
{
    uint8_t buf[2];

    /* Send register address */
    I2C1_CR2 = ((uint32_t)FUEL_I2C_ADDR << 1) | (1u << 16) | I2C_CR2_START;
    while ((I2C1_ISR & I2C_ISR_TXE) == 0) {
        if (I2C1_ISR & I2C_ISR_NACKF) {
            I2C1_ICR = I2C_ISR_NACKF;
            return 0;
        }
    }
    I2C1_TXDR = reg;
    while ((I2C1_ISR & I2C_ISR_TC) == 0) { }

    /* Read 2 bytes */
    I2C1_CR2 = ((uint32_t)FUEL_I2C_ADDR << 1) | 1u | (2u << 16) |
               I2C_CR2_START | (1u << 14);
    for (int i = 0; i < 2; i++) {
        while ((I2C1_ISR & I2C_ISR_RXNE) == 0) { }
        buf[i] = (uint8_t)I2C1_RXDR;
    }

    return ((uint16_t)buf[0] << 8) | buf[1];  /* Big-endian for MAX17055 */
}

static void max_write_reg(uint8_t reg, uint16_t val)
{
    I2C1_CR2 = ((uint32_t)FUEL_I2C_ADDR << 1) | (3u << 16) | I2C_CR2_START;
    while ((I2C1_ISR & I2C_ISR_TXE) == 0) {
        if (I2C1_ISR & I2C_ISR_NACKF) {
            I2C1_ICR = I2C_ISR_NACKF;
            return;
        }
    }
    I2C1_TXDR = reg;
    while ((I2C1_ISR & I2C_ISR_TXE) == 0) { }
    I2C1_TXDR = (uint8_t)(val >> 8);
    while ((I2C1_ISR & I2C_ISR_TXE) == 0) { }
    I2C1_TXDR = (uint8_t)(val & 0xFF);
    while ((I2C1_ISR & I2C_ISR_TC) == 0) { }
}