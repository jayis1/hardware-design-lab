/*
 * classify.h — AeroCast particle classifier API
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 */
#ifndef AEROCAST_CLASSIFY_H
#define AEROCAST_CLASSIFY_H

#include <stdint.h>
#include "afe.h"

void       classify_init(void);
int        classify_event(const particle_event_t *ev);
const char*classify_name(int cls);
uint32_t   classify_ref_checksum(void);
int        classify_add_calib_row(const uint8_t f[4], uint8_t cls);

#endif /* AEROCAST_CLASSIFY_H */