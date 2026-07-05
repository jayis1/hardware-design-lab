/*
 * asic.h — NUS32toA front-end ASIC driver interface
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 */
#ifndef MUONSCAPE_DRV_ASIC_H
#define MUONSCAPE_DRV_ASIC_H

#include <stdint.h>
#include "board.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Per-channel threshold DAC value (0..255). Lower = more sensitive. */
muon_status_t asic_init(void);
muon_status_t asic_set_threshold(uint8_t chip, uint8_t channel, uint8_t value);
muon_status_t asic_set_threshold_all(uint8_t value);
muon_status_t asic_set_tdc_window(uint8_t chip, uint16_t bins);
uint8_t       asic_get_threshold(uint8_t chip, uint8_t channel);
muon_status_t asic_autocalibrate(void);   /* sweep thresholds, find noise floor */
muon_status_t asic_read_rates(uint8_t chip, uint32_t rates[ASIC_CHANNELS_PER]);
void          asic_dump_status(char *buf, uint16_t len);

#ifdef __cplusplus
}
#endif
#endif /* MUONSCAPE_DRV_ASIC_H */