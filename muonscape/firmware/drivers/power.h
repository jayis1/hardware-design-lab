/*
 * power.h — battery + charger + solar MPPT management
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 */
#ifndef MUONSCAPE_DRV_POWER_H
#define MUONSCAPE_DRV_POWER_H

#include <stdint.h>
#include "board.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t cell_mv[6];     /* per-cell voltage, mV          */
    uint16_t pack_mv;        /* total pack voltage, mV        */
    int16_t  current_ma;     /* signed pack current, mA       */
    int16_t  temp_c;          /* thermistor temperature, °C   */
    uint8_t  soc_percent;    /* state of charge, %            */
    uint8_t  faults;         /* BQ76952 fault flags           */
} power_state_t;

muon_status_t power_init(void);
muon_status_t power_read(power_state_t *st);
muon_status_t power_set_charge_current_ma(uint16_t ma);
muon_status_t power_set_solar_mppt(int enable);  /* 1=track MPPT, 0=fixed */
void          power_mppt_tick(void);             /* one perturb-observe step */
uint16_t      power_get_solar_mv(void);
uint16_t      power_get_usb_mv(void);
void          power_shutdown(void);  /* graceful power-off */
int           power_thermal_ok(void);

#ifdef __cplusplus
}
#endif
#endif /* MUONSCAPE_DRV_POWER_H */