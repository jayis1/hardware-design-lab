/*
 * sht45.h — Sensirion SHT45 temperature/humidity sensor driver.
 * Author : jayis1
 * License: MIT
 */
#ifndef STRIDOPHONE_SHT45_H
#define STRIDOPHONE_SHT45_H

void sht45_init(void);

/* Read T (°C) and RH (%). Returns 0 on success. */
int sht45_read(float *temp_c, float *rh_pct);

#endif