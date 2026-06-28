/*
 * cs_power.c — Charger / fuel gauge / mains-detect for Cryo-Sentinel.
 *
 * Author: jayis1
 * License: MIT
 */
#include "cs_power.h"
#include "board.h"
#include "registers.h"
#include <string.h>

/* Same platform shims as cs_sensor.c; in the real build these are Zephyr
 * I2C / GPIO drivers. */
extern int  mock_i2c_read(uint8_t a, uint8_t r, uint8_t *b, int n);
extern int  mock_i2c_write(uint8_t a, uint8_t r, const uint8_t *b, int n);
extern uint32_t mock_gpio_get(uint8_t p);
extern void mock_gpio_set(uint8_t p, uint32_t v);

static uint16_t rd16be(uint8_t addr, uint8_t reg)
{
    uint8_t b[2] = {0,0};
    mock_i2c_read(addr, reg, b, 2);
    return ((uint16_t)b[0] << 8) | b[1];
}

int cs_power_init(void)
{
    /* BQ25895: leave it in standalone mode; just clear faults. */
    uint8_t v = 0x00;
    mock_i2c_write(BQ25895_I2C_ADDR, BQ25895_REG_FAULT, &v, 1);

    /* MAX17055: if POR bit set, do a quick-start. The full model-gauge
     * initialization is deferred to the first read so the chip has time
     * to settle. */
    uint16_t status = rd16be(MAX17055_I2C_ADDR, MAX17055_REG_STATUS);
    if (status & MAX17055_STATUS_POR) {
        uint16_t cfg = 0x7F0E;  /* default CFG, Aen=1, Een=1, Been=1 */
        uint8_t b[2] = { (uint8_t)(cfg >> 8), (uint8_t)(cfg & 0xFF) };
        mock_i2c_write(MAX17055_I2C_ADDR, MAX17055_REG_CONFIG, b, 2);
        /* Clear POR. */
        b[0] = (uint8_t)(status >> 8); b[1] = (uint8_t)(status & 0xFF);
        b[1] &= (uint8_t)~(MAX17055_STATUS_POR & 0xFF);
        mock_i2c_write(MAX17055_I2C_ADDR, MAX17055_REG_STATUS, b, 2);
    }
    return 0;
}

int cs_power_read(cs_power_t *out)
{
    memset(out, 0, sizeof(*out));

    /* Fuel gauge. */
    uint16_t soc  = rd16be(MAX17055_I2C_ADDR, MAX17055_REG_REPSOC);
    uint16_t vcell= rd16be(MAX17055_I2C_ADDR, MAX17055_REG_VCELL);
    uint16_t curr = rd16be(MAX17055_I2C_ADDR, MAX17055_REG_AVG_CURRENT);
    uint16_t temp = rd16be(MAX17055_I2C_ADDR, MAX17055_REG_TEMP);
    uint16_t cyc  = rd16be(MAX17055_I2C_ADDR, MAX17055_REG_CYCLES);

    out->batt_pct    = (float)(soc >> 8);                /* 1% / LSB */
    out->batt_v      = (float)vcell * MAX17055_VSCALE * 1e-6f;  /* uV -> V */
    int16_t cur_i    = (int16_t)curr;
    out->batt_i_ma   = (float)cur_i * MAX17055_ISCALE * 1e-3f;  /* uV -> mA, Rs=5mOhm */
    int16_t t_raw    = (int16_t)temp;
    out->temp_c      = (float)t_raw / 256.0f;
    out->cycle_count = cyc;

    /* Charger status. */
    uint8_t st = 0;
    mock_i2c_read(BQ25895_I2C_ADDR, BQ25895_REG_CHG_STAT, &st, 1);
    uint8_t chg = (st >> 4) & 0x03;
    out->chg_state = (cs_chg_state_t)chg;
    uint8_t flt = 0;
    mock_i2c_read(BQ25895_I2C_ADDR, BQ25895_REG_FAULT, &flt, 1);
    out->charger_fault = (flt != 0);

    /* Mains presence (opto output: low == present). */
    out->mains_present = (mock_gpio_get(CS_GPIO_MAINS_OPTO) == 0);
    return 0;
}

void cs_power_cell_enable(bool on)
{
    mock_gpio_set(CS_GPIO_CELL_PWR_EN, on ? 1 : 0);
}