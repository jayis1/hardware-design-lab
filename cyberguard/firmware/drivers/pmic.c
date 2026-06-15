/*
 * pmic.c - STPMIC1APQR Power Management IC Driver Implementation
 * CyberGuard Hardware Security Token
 *
 * I2C protocol: I2C2 (PB10=SCL, PB11=SDA) @ 400kHz
 * Device address: 0x08 (7-bit)
 */

#include "pmic.h"
#include "../registers.h"
#include "../board.h"

/* ============================================================
 * I2C2 Transfer Primitives (PMIC)
 * ============================================================ */

static uint32_t i2c2_get_tick(void)
{
    extern volatile uint32_t systick_ms;
    return systick_ms;
}

static int i2c2_wait_flag(uint32_t flag, uint8_t expected)
{
    uint32_t start = i2c2_get_tick();
    while (((I2C2->ISR & flag) ? 1 : 0) != expected) {
        if ((i2c2_get_tick() - start) > 100) return -1;
    }
    return 0;
}

static int i2c2_write_byte(uint8_t reg, uint8_t data)
{
    uint32_t start;

    /* Wait until bus is free */
    start = i2c2_get_tick();
    while (I2C2->ISR & I2C_ISR_BUSY) {
        if ((i2c2_get_tick() - start) > 100) return -1;
    }

    /* Configure transfer: write, 2 bytes (reg + data), auto-end */
    I2C2->CR2 = (STPMIC1_I2C_ADDR_8BIT)
               | (2U << 16)       /* 2 bytes */
               | (1U << 13);      /* Auto-end */

    /* Send register address */
    if (i2c2_wait_flag(I2C_ISR_TXE, 1) != 0) return -1;
    I2C2->TXDR = reg;

    /* Send data */
    if (i2c2_wait_flag(I2C_ISR_TXE, 1) != 0) return -1;
    I2C2->TXDR = data;

    /* Wait for STOP */
    start = i2c2_get_tick();
    while (!(I2C2->ISR & I2C_ISR_STOPF)) {
        if ((i2c2_get_tick() - start) > 100) return -1;
    }
    I2C2->ICR = I2C_ISR_STOPF;

    if (I2C2->ISR & I2C_ISR_NACKF) {
        I2C2->ICR = I2C_ISR_NACKF;
        return -2;
    }

    return 0;
}

static int i2c2_read_byte(uint8_t reg, uint8_t *data)
{
    uint32_t start;

    /* Wait until bus is free */
    start = i2c2_get_tick();
    while (I2C2->ISR & I2C_ISR_BUSY) {
        if ((i2c2_get_tick() - start) > 100) return -1;
    }

    /* Phase 1: Write register address (1 byte, no stop) */
    I2C2->CR2 = (STPMIC1_I2C_ADDR_8BIT)
               | (1U << 16)       /* 1 byte */
               | (0U << 25);      /* Auto-end off (manual stop) */

    if (i2c2_wait_flag(I2C_ISR_TXE, 1) != 0) return -1;
    I2C2->TXDR = reg;

    /* Wait for TC (transfer complete) */
    start = i2c2_get_tick();
    while (!(I2C2->ISR & I2C_ISR_TC)) {
        if ((i2c2_get_tick() - start) > 100) return -1;
    }

    /* Phase 2: Read 1 byte with restart */
    I2C2->CR2 = (STPMIC1_I2C_ADDR_8BIT | 1U)  /* Read bit */
               | (1U << 16)       /* 1 byte */
               | (1U << 13);      /* Auto-end */

    /* Wait for data */
    start = i2c2_get_tick();
    while (!(I2C2->ISR & I2C_ISR_RXNE)) {
        if ((i2c2_get_tick() - start) > 100) return -1;
    }
    *data = (uint8_t)(I2C2->RXDR & 0xFF);

    /* Wait for STOP */
    start = i2c2_get_tick();
    while (!(I2C2->ISR & I2C_ISR_STOPF)) {
        if ((i2c2_get_tick() - start) > 100) return -1;
    }
    I2C2->ICR = I2C_ISR_STOPF;

    return 0;
}

/* ============================================================
 * Public API Implementation
 * ============================================================ */

int pmic_read_reg(uint8_t reg, uint8_t *val)
{
    return i2c2_read_byte(reg, val);
}

int pmic_write_reg(uint8_t reg, uint8_t val)
{
    return i2c2_write_byte(reg, val);
}

int pmic_init(void)
{
    uint8_t reg_val;
    int ret;

    /* Step 1: Check PMIC presence by reading MAIN_CTRL */
    ret = pmic_read_reg(STPMIC1_REG_MAIN_CTRL, &reg_val);
    if (ret != 0) return ret;

    /* Step 2: Configure and enable buck converter → 3.3V */
    ret = pmic_write_reg(STPMIC1_REG_BUCK_OUT_VR, STPMIC1_BUCK_3V3);
    if (ret != 0) return ret;
    ret = pmic_write_reg(STPMIC1_REG_BUCK_CTRL, STPMIC1_BUCK_EN);
    if (ret != 0) return ret;

    /* Wait for buck converter to stabilize */
    extern volatile uint32_t systick_ms;
    uint32_t start = systick_ms;
    while ((systick_ms - start) < 5);

    /* Step 3: Enable LDO1 → 1.8V */
    ret = pmic_write_reg(STPMIC1_REG_LDO1_OUT_VR, STPMIC1_LDO1_1V8);
    if (ret != 0) return ret;
    ret = pmic_write_reg(STPMIC1_REG_LDO1_CTRL, STPMIC1_LDO1_EN);
    if (ret != 0) return ret;

    /* Step 4: Enable LDO2 → 1.1V */
    ret = pmic_write_reg(STPMIC1_REG_LDO2_OUT_VR, STPMIC1_LDO2_1V1);
    if (ret != 0) return ret;
    ret = pmic_write_reg(STPMIC1_REG_LDO2_CTRL, STPMIC1_LDO2_EN);
    if (ret != 0) return ret;

    /* Wait for LDOs to stabilize */
    start = systick_ms;
    while ((systick_ms - start) < 3);

    /* Step 5: Enable VBUS interrupt */
    ret = pmic_write_reg(STPMIC1_REG_INT_MASK, 0x00); /* Unmask all */
    if (ret != 0) return ret;

    /* Step 6: Clear any pending interrupts */
    ret = pmic_read_reg(STPMIC1_REG_INT_CLEAR, &reg_val);
    if (ret == 0) {
        pmic_write_reg(STPMIC1_REG_INT_CLEAR, 0xFF); /* Clear all */
    }

    return 0;
}

int pmic_enable_buck(void)
{
    return pmic_write_reg(STPMIC1_REG_BUCK_CTRL, STPMIC1_BUCK_EN);
}

int pmic_enable_ldo1(void)
{
    return pmic_write_reg(STPMIC1_REG_LDO1_CTRL, STPMIC1_LDO1_EN);
}

int pmic_enable_ldo2(void)
{
    return pmic_write_reg(STPMIC1_REG_LDO2_CTRL, STPMIC1_LDO2_EN);
}

int pmic_disable_buck(void)
{
    uint8_t val;
    int ret = pmic_read_reg(STPMIC1_REG_BUCK_CTRL, &val);
    if (ret != 0) return ret;
    return pmic_write_reg(STPMIC1_REG_BUCK_CTRL, val & ~STPMIC1_BUCK_EN);
}

int pmic_disable_ldo1(void)
{
    uint8_t val;
    int ret = pmic_read_reg(STPMIC1_REG_LDO1_CTRL, &val);
    if (ret != 0) return ret;
    return pmic_write_reg(STPMIC1_REG_LDO1_CTRL, val & ~STPMIC1_LDO1_EN);
}

int pmic_disable_ldo2(void)
{
    uint8_t val;
    int ret = pmic_read_reg(STPMIC1_REG_LDO2_CTRL, &val);
    if (ret != 0) return ret;
    return pmic_write_reg(STPMIC1_REG_LDO2_CTRL, val & ~STPMIC1_LDO2_EN);
}

int pmic_enter_lowpower(void)
{
    /* Disable LDOs, put buck in low-power mode */
    uint8_t val;

    /* LDO2 off (1.1V - nRF core) */
    pmic_disable_ldo2();

    /* LDO1 off (1.8V - FP IO) */
    pmic_disable_ldo1();

    /* Buck in low-power mode, still enabled for 3.3V */
    pmic_read_reg(STPMIC1_REG_BUCK_CTRL, &val);
    pmic_write_reg(STPMIC1_REG_BUCK_CTRL, val | STPMIC1_BUCK_LP);

    return 0;
}

int pmic_exit_lowpower(void)
{
    uint8_t val;

    /* Buck back to normal mode */
    pmic_read_reg(STPMIC1_REG_BUCK_CTRL, &val);
    pmic_write_reg(STPMIC1_REG_BUCK_CTRL, val & ~STPMIC1_BUCK_LP);

    /* Re-enable LDOs */
    pmic_enable_ldo1();
    pmic_enable_ldo2();

    return 0;
}

int pmic_vbus_detected(void)
{
    uint8_t status;
    int ret = pmic_read_reg(STPMIC1_REG_INT_STATUS, &status);
    if (ret != 0) return ret;
    return (status & STPMIC1_INT_VBUS_CONNECT) ? 1 : 0;
}