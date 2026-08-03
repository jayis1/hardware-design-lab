/*
 * power.c — MAX17048 fuel gauge + MCP73831 charger interface
 *
 * Reads the state-of-charge and cell voltage from the MAX17048 model-gauge
 * over I²C (TWI0), and reports charge state via the MCP73831 STAT pin.
 * Also handles System OFF entry for week-long shelf life.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "power.h"
#include "../board.h"
#include "../registers.h"

/* ---- Minimal TWI0 shim ---- */
static void twi0_start(void)    { /* real: NRF_TWIM0->TASKS_STARTTX */ }
static void twi0_stop(void)     { /* real: NRF_TWIM0->TASKS_STOP */ }
static bool twi0_write(uint8_t addr, const uint8_t *data, uint32_t len)
{ (void)addr; (void)data; (void)len; return true; }
static bool twi0_read(uint8_t addr, uint8_t *data, uint32_t len)
{ (void)addr; (void)data; (void)len; return true; }

static uint16_t max17048_read_reg(uint8_t reg)
{
    uint8_t buf[2];
    if (!twi0_write(MAX17048_ADDR, &reg, 1)) return 0;
    if (!twi0_read(MAX17048_ADDR, buf, 2)) return 0;
    return (uint16_t)((buf[0] << 8) | buf[1]);
}

void power_init(void)
{
    /* Issue quick-start to refresh the model gauge after long shelf. */
    uint8_t cmd[3] = { MAX17048_REG_MODE, 0x40, 0x00 };
    twi0_write(MAX17048_ADDR, cmd, 3);
}

uint8_t power_get_battery_pct(void)
{
    uint16_t soc = max17048_read_reg(MAX17048_REG_SOC);
    /* SOC register: high byte = percent, low byte = fractional 1/256%. */
    return (uint8_t)(soc >> 8);
}

uint16_t power_get_battery_mv(void)
{
    uint16_t vcell = max17048_read_reg(MAX17048_REG_VCELL);
    /* VCELL = register * 78.125 µV (per datasheet) → mV. */
    return (uint16_t)((uint32_t)vcell * 78U / 1000U);
}

bool power_is_charging(void)
{
    return IS_CHARGING();
}

void power_enter_off(void)
{
    /* Real build: set nRF POWER SYSTEMOFF register; wake on GPIO motion int. */
}