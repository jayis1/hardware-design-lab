/*
 * pmic_da9063.c — DA9063 PMIC Driver for HexaScope
 * Controls 6 power rails via I2C interface
 */

#include "board.h"
#include "registers.h"
#include <string.h>

/* ============================================================
 * I2C1 Transfer Helpers
 * ============================================================ */

static int i2c1_write(uint8_t dev_addr, uint8_t reg, uint8_t data)
{
    /* Wait until I2C bus is free */
    uint32_t timeout = 10000;
    while (I2C1->ISR & (1U << 6) && timeout--)  /* BUSY */
        ;

    /* Clear any pending flags */
    I2C1->ICR = 0x3F38;

    /* Configure transfer: write 1 byte to reg */
    I2C1->CR2 = ((uint32_t)dev_addr << 1) |   /* Address */
                (1U << 16) |                     /* NBYTES = 2 (reg + data) */
                (0U << 10) |                     /* Write direction */
                I2C_CR2_START;                   /* Start condition */

    /* Wait for TXIS flag or NACK */
    timeout = 10000;
    while (!(I2C1->ISR & I2C_ISR_TXIS) && !(I2C1->ISR & I2C_ISR_NACKF) && timeout--)
        ;

    if (I2C1->ISR & I2C_ISR_NACKF)
        return -1;

    /* Send register address */
    I2C1->TXDR = reg;

    /* Wait for TXIS */
    timeout = 10000;
    while (!(I2C1->ISR & I2C_ISR_TXIS) && timeout--)
        ;

    /* Send data */
    I2C1->TXDR = data;

    /* Wait for TC (transfer complete) */
    timeout = 10000;
    while (!(I2C1->ISR & I2C_ISR_TC) && timeout--)
        ;

    /* Send STOP */
    I2C1->CR2 |= I2C_CR2_STOP;

    return 0;
}

static int i2c1_read(uint8_t dev_addr, uint8_t reg, uint8_t *data)
{
    uint32_t timeout = 10000;

    /* Phase 1: Write register address */
    I2C1->CR2 = ((uint32_t)dev_addr << 1) |
                (1U << 16) |           /* NBYTES = 1 */
                (0U << 10) |           /* Write */
                I2C_CR2_START;

    while (!(I2C1->ISR & I2C_ISR_TXIS) && timeout--)
        ;

    I2C1->TXDR = reg;

    timeout = 10000;
    while (!(I2C1->ISR & I2C_ISR_TC) && timeout--)
        ;

    /* Phase 2: Repeated start, read 1 byte */
    I2C1->CR2 = ((uint32_t)dev_addr << 1) |
                (1U << 16) |           /* NBYTES = 1 */
                I2C_CR2_RD_WRN |      /* Read */
                I2C_CR2_START;

    /* Wait for RXNE */
    timeout = 10000;
    while (!(I2C1->ISR & I2C_ISR_RXNE) && timeout--)
        ;

    *data = (uint8_t)I2C1->RXDR;

    /* Send STOP */
    I2C1->CR2 |= I2C_CR2_STOP;

    return 0;
}

/* ============================================================
 * DA9063 PMIC API
 * ============================================================ */

void pmic_init(void)
{
    uint8_t val;

    /* Read device ID to verify communication */
    i2c1_read(DA9063_I2C_ADDR, DA9063_REG_PAGE_CON, &val);

    /* Set page 0 (register bank 0 for control registers) */
    i2c1_write(DA9063_I2C_ADDR, DA9063_REG_PAGE_CON, 0x00);

    /* Disable watchdog timer */
    i2c1_write(DA9063_I2C_ADDR, DA9063_REG_CONTROL_A, 0x00);

    /* Configure BUCK1 (VDD_CORE = 1.10V)
     * DA9063 BUCK voltage: V = 0.300 + VSEL × 0.010
     * For 1.10V: VSEL = (1.10 - 0.300) / 0.010 = 80 = 0x50
     * But DA9063 BUCK1 has a different formula: V = 0.300 + VSEL × 0.010
     * Actually for DA9063-NA: BUCK1 range 0.300-1.475V, step 12.5mV
     * For 1.10V: VSEL = (1.10 - 0.300) / 0.0125 ≈ 64 = 0x40
     */
    i2c1_write(DA9063_I2C_ADDR, DA9063_REG_BUCK1_CONT, 0x40);

    /* Configure BUCK2 (VDD_DDR = 1.35V)
     * For 1.35V: VSEL = (1.35 - 0.300) / 0.0125 ≈ 84 = 0x54
     */
    i2c1_write(DA9063_I2C_ADDR, DA9063_REG_BUCK2_CONT, 0x54);

    /* Configure BUCK3 (VDD_IO18 = 1.80V)
     * For 1.80V: VSEL = (1.80 - 0.300) / 0.0125 ≈ 120 = 0x78
     */
    i2c1_write(DA9063_I2C_ADDR, DA9063_REG_BUCK3_CONT, 0x78);

    /* Configure LDO1 (VDD_IO33 = 3.30V)
     * LDO range: 0.600-3.300V, step 25mV
     * For 3.30V: VSEL = (3.30 - 0.600) / 0.025 = 108 = 0x6C
     */
    i2c1_write(DA9063_I2C_ADDR, DA9063_REG_LDO1_CONT, 0x6C);

    /* Configure LDO2 (VDD_ANA = 5.00V)
     * For 5.00V: VSEL = (5.00 - 0.600) / 0.025 = 176 = 0xB0
     */
    i2c1_write(DA9063_I2C_ADDR, DA9063_REG_LDO2_CONT, 0xB0);

    /* Configure LDO3 (VDD_RTC = 3.30V, always-on) */
    i2c1_write(DA9063_I2C_ADDR, DA9063_REG_LDO3_CONT, 0x6C | 0x80);  /* Enable bit */

    /* Mask all interrupts initially */
    i2c1_write(DA9063_I2C_ADDR, DA9063_REG_IRQ_MASK_A, 0xFF);
    i2c1_write(DA9063_I2C_ADDR, DA9063_REG_IRQ_MASK_B, 0xFF);
    i2c1_write(DA9063_I2C_ADDR, DA9063_REG_IRQ_MASK_C, 0xFF);
    i2c1_write(DA9063_I2C_ADDR, DA9063_REG_IRQ_MASK_D, 0xFF);
}

void pmic_enable_rails(void)
{
    uint8_t val;

    /* Enable BUCK1 (VDD_CORE) */
    i2c1_read(DA9063_I2C_ADDR, DA9063_REG_BUCK1_CONT, &val);
    i2c1_write(DA9063_I2C_ADDR, DA9063_REG_BUCK1_CONT, val | 0x80);

    /* Small delay for rail stabilization */
    for (volatile uint32_t i = 0; i < 10000; i++)
        ;

    /* Enable BUCK2 (VDD_DDR) */
    i2c1_read(DA9063_I2C_ADDR, DA9063_REG_BUCK2_CONT, &val);
    i2c1_write(DA9063_I2C_ADDR, DA9063_REG_BUCK2_CONT, val | 0x80);

    for (volatile uint32_t i = 0; i < 10000; i++)
        ;

    /* Enable BUCK3 (VDD_IO18) */
    i2c1_read(DA9063_I2C_ADDR, DA9063_REG_BUCK3_CONT, &val);
    i2c1_write(DA9063_I2C_ADDR, DA9063_REG_BUCK3_CONT, val | 0x80);

    for (volatile uint32_t i = 0; i < 10000; i++)
        ;

    /* Enable LDO1 (VDD_IO33) */
    i2c1_read(DA9063_I2C_ADDR, DA9063_REG_LDO1_CONT, &val);
    i2c1_write(DA9063_I2C_ADDR, DA9063_REG_LDO1_CONT, val | 0x80);

    /* Enable LDO2 (VDD_ANA) */
    i2c1_read(DA9063_I2C_ADDR, DA9063_REG_LDO2_CONT, &val);
    i2c1_write(DA9063_I2C_ADDR, DA9063_REG_LDO2_CONT, val | 0x80);

    /* LDO3 already enabled with always-on bit in init */
}

uint8_t pmic_read_reg(uint8_t reg)
{
    uint8_t val;
    i2c1_read(DA9063_I2C_ADDR, reg, &val);
    return val;
}

void pmic_write_reg(uint8_t reg, uint8_t val)
{
    i2c1_write(DA9063_I2C_ADDR, reg, val);
}

/* Read PMIC fault register and return status */
uint8_t pmic_get_fault_status(void)
{
    return pmic_read_reg(DA9063_REG_FAULT_LOG);
}

/* Disable a specific rail (for power management) */
void pmic_disable_rail(uint8_t rail_reg)
{
    uint8_t val;
    i2c1_read(DA9063_I2C_ADDR, rail_reg, &val);
    i2c1_write(DA9063_I2C_ADDR, rail_reg, val & ~0x80);
}

/* Enable a specific rail */
void pmic_enable_rail(uint8_t rail_reg)
{
    uint8_t val;
    i2c1_read(DA9063_I2C_ADDR, rail_reg, &val);
    i2c1_write(DA9063_I2C_ADDR, rail_reg, val | 0x80);
}