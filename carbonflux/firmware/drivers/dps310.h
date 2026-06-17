/**
 * @file    dps310.h
 * @brief   DPS310 barometric pressure sensor interface.
 * @author  jayis1
 * @copyright Copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef DPS310_H
#define DPS310_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

int   dps310_init(void);
float dps310_read_pressure_hpa(void);
float dps310_read_temp_c(void);
int   dps310_wake(void);

#ifdef __cplusplus
}
#endif

#endif /* DPS310_H */