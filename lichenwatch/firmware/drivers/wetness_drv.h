/*
 * wetness_drv.h — Surface wetness (thallus conductance grid) driver.
 *
 * Author: jayis1
 * License: MIT
 */

#ifndef WETNESS_DRV_H
#define WETNESS_DRV_H

float wetness_read(void);   /* returns 0.0 (dry) .. 1.0 (saturated) */

int wetness_init(void);

#endif /* WETNESS_DRV_H */