/*
 * depth.h — HydroFluor MS5837-02BA depth/pressure sensor (I²C)
 * Author: jayis1
 * License: MIT
 */
#ifndef DEPTH_H
#define DEPTH_H
#include <stdint.h>
#include "board.h"

void depth_init(void);
uint32_t depth_read_cm(void);       /* depth below surface in cm */
int32_t depth_read_mbar(void);       /* absolute pressure in mbar */
float  depth_temperature_c(void);    /* compensated temperature °C */

#endif /* DEPTH_H */