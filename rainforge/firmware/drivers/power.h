/*
 * power.h — Deep-sleep management & wake-source routing
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 */
#ifndef RAINFORGE_POWER_H
#define RAINFORGE_POWER_H

#include <stdint.h>
#include "board.h"

/* ---- API ---- */
void     power_init(void);
void     power_deep_sleep(uint32_t rtc_wake_ms, wake_source_t expected);
void     power_configure_wake_sources(void);
wake_source_t power_get_wake_cause(void);
float    power_get_scap_voltage(void);
uint8_t  power_scap_ok(void);
uint8_t  power_scap_low(void);

/* Cold-start: wait until supercap charged enough to boot */
void     power_cold_start_wait(void);

#endif /* RAINFORGE_POWER_H */