/*
 * cs_power.h — Charger, fuel gauge, and mains-detection interface.
 *
 * Author: jayis1
 * License: MIT
 */
#ifndef CRYO_SENTINEL_CS_POWER_H
#define CRYO_SENTINEL_CS_POWER_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    CS_CHG_NOT_CHARGING = 0,
    CS_CHG_PRECHARGE    = 1,
    CS_CHG_FAST         = 2,
    CS_CHG_DONE         = 3
} cs_chg_state_t;

typedef struct {
    float          batt_pct;       /* 0..100 */
    float          batt_v;         /* volts */
    float          batt_i_ma;      /* signed: + = charging, - = discharging */
    float          temp_c;         /* from fuel gauge thermistor */
    cs_chg_state_t chg_state;
    bool           mains_present;
    bool           charger_fault;
    uint32_t       cycle_count;
} cs_power_t;

int  cs_power_init(void);

/* Read a fresh snapshot of the power subsystem. */
int  cs_power_read(cs_power_t *out);

/* Enable / disable the BG770A cellular modem load switch. */
void cs_power_cell_enable(bool on);

#endif /* CRYO_SENTINEL_CS_POWER_H */