/**
 * @file    adc.h
 * @brief   ADC driver interface for PAR, battery, and spare.
 * @author  jayis1
 * @copyright Copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef ADC_DRIVER_H
#define ADC_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void   adc_init(void);
float  adc_read_par(void);     /* µmol·m⁻²·s⁻¹ */
float  adc_read_vbat(void);    /* Volts */
uint16_t adc_read_spare(void); /* Raw ADC value */

#ifdef __cplusplus
}
#endif

#endif /* ADC_DRIVER_H */