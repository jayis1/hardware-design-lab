/*
 * harvest.h — Energy harvester & supercapacitor management
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 */
#ifndef RAINFORGE_HARVEST_H
#define RAINFORGE_HARVEST_H

#include <stdint.h>

/* ---- Supercapacitor state ---- */
typedef struct {
    float voltage;          /* current terminal voltage, V */
    float charge_c;         /* accumulated charge, Coulombs */
    float energy_j;         /* estimated stored energy, Joules */
    float harvest_rate_mw;  /* rolling-average harvest power, mW */
    uint8_t vok;            /* V_STORE_OK pin state */
    uint8_t vlow;           /* V_STORE_LOW pin state */
} harvest_state_t;

/* ---- API ---- */
void  harvest_init(void);
void  harvest_update(harvest_state_t *st);   /* read GPIO + ADC, update state */
float harvest_estimate_soc(void);             /* state-of-charge 0..1 */
uint8_t harvest_can_tx(void);                  /* 1 if enough energy for a LoRa TX */
void  harvest_log_event(float energy_uj);     /* accumulate per-droplet energy */

/* Coulomb-counting integration */
void  harvest_tick_ms(uint32_t ms);

#endif /* RAINFORGE_HARVEST_H */