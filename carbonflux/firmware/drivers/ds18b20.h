/**
 * @file    ds18b20.h
 * @brief   DS18B20 1-Wire temperature sensor interface.
 * @author  jayis1
 * @copyright Copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef DS18B20_H
#define DS18B20_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

int   ds18b20_init(void);
int   ds18b20_start_conversion(uint8_t index);
void  ds18b20_start_all(void);
float ds18b20_read_temp(uint8_t index);
void  ds18b20_read_all(float *temps_out);
uint8_t ds18b20_get_found(void);

#ifdef __cplusplus
}
#endif

#endif /* DS18B20_H */