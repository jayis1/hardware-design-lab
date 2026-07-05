/*
 * sipm.h — SiPM bias voltage control
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 */
#ifndef MUONSCAPE_DRV_SIPM_H
#define MUONSCAPE_DRV_SIPM_H

#include <stdint.h>
#include "board.h"

#ifdef __cplusplus
extern "C" {
#endif

muon_status_t sipm_init(void);
muon_status_t sipm_set_bias_mv(uint32_t mv);     /* 14000..16500 */
uint32_t      sipm_get_bias_mv(void);
muon_status_t sipm_temp_compensate(float temp_c); /* adjust bias by temperature */
void          sipm_disable(void);                 /* bias off */
float         sipm_get_current_ma(void);          /* total SiPM current */

#ifdef __cplusplus
}
#endif
#endif /* MUONSCAPE_DRV_SIPM_H */