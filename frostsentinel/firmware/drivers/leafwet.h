/*
 * drivers/leafwet.h — Leaf wetness sensor driver header
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef FROSTSENTINEL_LEAFWET_H
#define FROSTSENTINEL_LEAFWET_H

#include <stdint.h>

/*
 * Read the leaf wetness sensor.
 * Returns 0 on success, -1 on error.
 * On success, *wetness_out holds a normalized wetness index 0–1000
 * (0 = bone dry, 1000 = saturated water film).
 */
int leafwet_read(uint16_t *wetness_out);

/* Returns 1 if dew is present (wetness above threshold), 0 otherwise. */
int leafwet_is_dew_present(uint16_t wetness);

#endif /* FROSTSENTINEL_LEAFWET_H */