/*
 * temp.h — DS18B20 1-Wire temperature driver (9 sensors)
 * Author: jayis1  Copyright (C) 2026 jayis1  License: GPL-2.0
 */
#ifndef GRAINGUARD_TEMP_H
#define GRAINGUARD_TEMP_H

#include <stdint.h>

#define TEMP_NUM_ZONES  9

typedef struct {
    int16_t  celsius_x10[TEMP_NUM_ZONES];  /* temperature ×10 (e.g. 251 = 25.1 C) */
    uint8_t  valid_mask;                   /* bit i set if zone i read OK */
    int8_t   max_zone;                     /* index of hottest valid zone */
    int8_t   min_zone;                     /* index of coldest valid zone */
    int16_t  delta_x10;                    /* max - min, ×10 */
} temp_profile_t;

/* Initialize the 1-Wire bus and discover the 9 DS18B20 ROM IDs. */
int  temp_init(void);

/* Trigger simultaneous conversion on all sensors (parasitic power). */
int  temp_trigger_conversion(void);

/* Read all 9 zones into a profile.  Should be called after conversion. */
int  temp_read_profile(temp_profile_t *out);

/* Compute max, min, delta across valid zones. */
void temp_compute_stats(temp_profile_t *out);

#endif /* GRAINGUARD_TEMP_H */