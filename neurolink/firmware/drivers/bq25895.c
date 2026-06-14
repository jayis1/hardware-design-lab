/*
 * NeuroLink — BQ25895 Battery Charger Driver Implementation
 * I2C2 interface at 400 kHz
 */

#include "bq25895.h"
#include "board.h"
#include "registers.h"

/* ============================================================
 * I2C2 Helper Functions
 * ============================================================ */

static void i2c2_wait_flag(uint32_t flag)
{
    uint32_t timeout = 100000;
    while (!(I2C2_ISR & flag) && timeout--) ;
}

static int i2c2_write_byte(uint8_t addr, uint8_t reg, uint8_t data)
{
    /* Send start + address */
    I2C2_CR2 = ((addr & 0x7F) << 1) |   /* Slave address */
               (2 << 16) |               /* Number of bytes = 2 */
               (1 << 25) |               /* Auto-end */
               (1 << 13) |               /* Generate START */
               (0 << 10);                /* Write = 0 */

    /* Wait for TXIS (TX interrupt status) */
    i2c2_wait_flag(1 << 1);  /* TXIS */

    /* Send register address */
    I2C2_TXDR = reg;

    /* Wait for TXIS */
    i2c2_wait_flag(1 << 1);

    /* Send data */
    I2C2_TXDR = data;

    /* Wait for STOP */
    i2c2_wait_flag(1 << 5);  /* STOPF */

    /* Check NACK */
    if (I2C2_ISR & (1 << 2)) {  /* NACK */
        I2C2_ICR |= (1 << 4);   /* Clear NACK */
        return -1;
    }

    return 0;
}

static int i2c2_read_byte(uint8_t addr, uint8_t reg, uint8_t *data)
{
    /* Phase 1: Write register address */
    I2C2_CR2 = ((addr & 0x7F) << 1) |
               (1 << 16) |       /* 1 byte */
               (0 << 25) |       /* Software end */
               (1 << 13) |       /* START */
               (0 << 10);        /* Write */

    i2c2_wait_flag(1 << 1);  /* TXIS */
    I2C2_TXDR = reg;

    i2c2_wait_flag(1 << 6);  /* TC (transfer complete) */

    /* Phase 2: Read data */
    I2C2_CR2 = ((addr & 0x7F) << 1) |
               (1 << 16) |       /* 1 byte */
               (1 << 25) |       /* Auto-end */
               (1 << 13) |       /* Repeated START */
               (1 << 10);        /* Read */

    i2c2_wait_flag(1 << 2);  /* RXNE */
    *data = (uint8_t)(I2C2_RXDR & 0xFF);

    i2c2_wait_flag(1 << 5);  /* STOPF */

    return 0;
}

/* ============================================================
 * BQ25895 Initialization
 * ============================================================ */
void bq25895_init(void)
{
    uint8_t reg_val;

    /* Read device ID to verify communication */
    reg_val = bq25895_read_reg(BQ25895_REG_14);
    if ((reg_val >> 3) != 0x34) {
        /* Device ID mismatch — handle error */
        /* In production: log error, retry */
        return;
    }

    /* Configure input voltage limit: VINDPM = 4.2V (0x0B) */
    /* VINDPM = 3.88V + offset × 0.04V → offset = (4.2-3.88)/0.04 = 8 */
    bq25895_write_reg(BQ25895_REG_0B, 0x88 | 8);  /* Force DPDM, VINDPM offset = 8 */

    /* Configure input current limit: IINLIM = 1.6A (0x00) */
    /* IINLIM = 100mA + 50mA × reg[6:0] → 100 + 50×30 = 1600mA */
    bq25895_write_reg(BQ25895_REG_00, 0x80 | 30);  /* EN_ILIM=0, IINLIM=1.6A */

    /* Enable ICO (Input Current Optimizer) */
    reg_val = bq25895_read_reg(BQ25895_REG_02);
    bq25895_write_reg(BQ25895_REG_02, reg_val | (1 << 6));  /* ICO_EN */

    /* Set fast charge current: ICHG = 1.0A (0x04) */
    /* ICHG = 64mA + 64mA × reg[6:0] → 64 + 64×14 = 960mA ≈ 1A */
    bq25895_write_reg(BQ25895_REG_04, 14);

    /* Set pre-charge current: IPRECHG = 128mA (0x05) */
    /* IPRECHG = 64mA + 64mA × reg[7:4] → 64 + 64×1 = 128mA */
    /* Set termination current: ITERM = 128mA */
    reg_val = (1 << 4) | 1;  /* IPRECHG=1 (128mA), ITERM=1 (128mA) */
    bq25895_write_reg(BQ25895_REG_05, reg_val);

    /* Set regulation voltage: VREG = 4.208V (0x06) */
    /* VREG = 3.840V + 0.032V × reg[7:3] → 3840 + 32×11.5 = 4208mV */
    bq25895_write_reg(BQ25895_REG_06, (11 << 3));

    /* Enable charging, disable watchdog */
    reg_val = bq25895_read_reg(BQ25895_REG_03);
    reg_val |= (1 << 4);    /* CHG_CONFIG = enable charging */
    reg_val &= ~(3 << 4);   /* Disable watchdog (WD=00) */
    bq25895_write_reg(BQ25895_REG_03, reg_val);

    /* Enable ADC for battery voltage monitoring */
    reg_val = bq25895_read_reg(BQ25895_REG_02);
    bq25895_write_reg(BQ25895_REG_02, reg_val | (1 << 7));  /* ADC_EN = 1 */
}

/* ============================================================
 * Register Read / Write
 * ============================================================ */
uint8_t bq25895_read_reg(uint8_t reg)
{
    uint8_t val;
    i2c2_read_byte(BQ25895_I2C_ADDR, reg, &val);
    return val;
}

void bq25895_write_reg(uint8_t reg, uint8_t val)
{
    i2c2_write_byte(BQ25895_I2C_ADDR, reg, val);
}

/* ============================================================
 * Charging Status
 * ============================================================ */
uint8_t bq25895_get_charging_status(void)
{
    uint8_t reg = bq25895_read_reg(BQ25895_REG_0E);
    return (reg >> 3) & 0x07;  /* CHG_STATUS[5:3] */
}

uint8_t bq25895_get_fault_status(void)
{
    return bq25895_read_reg(BQ25895_REG_0F);
}

/* ============================================================
 * ADC Readings
 * ============================================================ */
uint16_t bq25895_read_battery_voltage(void)
{
    uint8_t reg = bq25895_read_reg(BQ25895_REG_14);
    /* VBAT = 2304mV + reg[7:2] × 20mV */
    return 2304 + ((reg >> 2) & 0x3F) * 20;
}

uint16_t bq25895_read_vbus_voltage(void)
{
    uint8_t reg = bq25895_read_reg(BQ25895_REG_17);
    /* VBUS = 2600mV + reg[7:2] × 100mV */
    return 2600 + ((reg >> 2) & 0x3F) * 100;
}

uint16_t bq25895_read_system_voltage(void)
{
    uint8_t reg = bq25895_read_reg(BQ25895_REG_15);
    /* VSYS = 2304mV + reg[7:2] × 20mV */
    return 2304 + ((reg >> 2) & 0x3F) * 20;
}

int16_t bq25895_read_ibus_current(void)
{
    uint8_t reg = bq25895_read_reg(BQ25895_REG_18);
    /* IBUS = 0mA + reg[6:0] × 50mA */
    return (reg & 0x7F) * 50;
}

int16_t bq25895_read_die_temp(void)
{
    uint8_t reg = bq25895_read_reg(BQ25895_REG_19);
    /* TDIE = -40°C + reg[7:2] × 2.5°C */
    return -40 + ((reg >> 2) & 0x3F) * 25 / 10;
}

/* ============================================================
 * Configuration Setters
 * ============================================================ */
void bq25895_set_charge_current(uint16_t current_ma)
{
    /* ICHG = 64mA + 64mA × reg[6:0] */
    uint8_t val = (current_ma - 64) / 64;
    if (val > 63) val = 63;
    bq25895_write_reg(BQ25895_REG_04, val);
}

void bq25895_set_charge_voltage(uint16_t voltage_mv)
{
    /* VREG = 3840mV + 32mV × reg[7:3] */
    uint8_t val = (voltage_mv - 3840) / 32;
    if (val > 31) val = 31;
    bq25895_write_reg(BQ25895_REG_06, val << 3);
}

void bq25895_set_input_current_limit(uint16_t current_ma)
{
    /* IINLIM = 100mA + 50mA × reg[6:0] */
    uint8_t val = (current_ma - 100) / 50;
    if (val > 63) val = 63;
    uint8_t reg = bq25895_read_reg(BQ25895_REG_00);
    reg = (reg & 0x80) | (val & 0x7F);
    bq25895_write_reg(BQ25895_REG_00, reg);
}

void bq25895_enable_charging(uint8_t enable)
{
    uint8_t reg = bq25895_read_reg(BQ25895_REG_03);
    if (enable) {
        reg |= (1 << 4);   /* CHG_CONFIG = 01 (enable) */
    } else {
        reg &= ~(3 << 4);  /* CHG_CONFIG = 00 (disable) */
    }
    bq25895_write_reg(BQ25895_REG_03, reg);
}

void bq25895_enable_otg(uint8_t enable)
{
    uint8_t reg = bq25895_read_reg(BQ25895_REG_03);
    if (enable) {
        reg |= (1 << 5);   /* OTG enable */
    } else {
        reg &= ~(1 << 5);
    }
    bq25895_write_reg(BQ25895_REG_03, reg);
}

uint8_t bq25895_is_vbus_present(void)
{
    uint8_t reg = bq25895_read_reg(BQ25895_REG_0E);
    return (reg >> 7) & 1;  /* VBUS_STAT bit */
}

uint8_t bq25895_is_ico_complete(void)
{
    uint8_t reg = bq25895_read_reg(BQ25895_REG_13);
    return (reg >> 6) & 1;  /* ICO_DONE bit */
}