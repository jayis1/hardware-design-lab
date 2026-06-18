/*
 * temp.h — HydroFluor DS18B20 1-Wire bench temperature driver
 * Author: jayis1
 * License: MIT
 */
#ifndef TEMP_H
#define TEMP_H
#include <stdint.h>
#include "board.h"

void    temp_init(void);
int16_t temp_read_c01(void);   /* temperature in 0.01 °C */

#endif /* TEMP_H */