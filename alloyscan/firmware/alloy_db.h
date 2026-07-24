/*
 * alloy_db.h — Alloy reference database definitions
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Defines the alloy_entry_t structure and declares the reference
 * database of 64 alloys with their characteristic electromagnetic
 * signatures (8-dimensional I/Q feature vectors at 4 frequencies).
 */

#ifndef ALLOYSCAN_ALLOY_DB_H
#define ALLOYSCAN_ALLOY_DB_H

#include <stdint.h>

/* Maximum alloy name length (including null terminator) */
#define ALLOY_NAME_MAX      16

/* Number of reference alloys in the built-in database */
#define ALLOY_DB_COUNT      64

/* Alloy family codes */
typedef enum {
    FAMILY_CARBON_STEEL   = 0,
    FAMILY_ALLOY_STEEL    = 1,
    FAMILY_SS_AUSTENITIC  = 2,
    FAMILY_SS_FERRITIC   = 3,
    FAMILY_SS_DUPLEX     = 4,
    FAMILY_ALUMINUM      = 5,
    FAMILY_COPPER        = 6,
    FAMILY_TITANIUM      = 7,
    FAMILY_NICKEL        = 8,
    FAMILY_ZINC_MAG      = 9,
    FAMILY_OTHER         = 10
} alloy_family_t;

/* Reference entry for one alloy.
 * feature[8] = {I1, Q1, I2, Q2, I3, Q3, I4, Q4}
 * where I/Q pairs are at frequencies 1k, 10k, 100k, 500k.
 * Values are normalized to the open-circuit baseline and
 * lift-off compensated.
 */
typedef struct {
    char            name[ALLOY_NAME_MAX];  /* Short name e.g. "304 SS"   */
    alloy_family_t  family;                 /* Family code                */
    float           conductivity_iacs;      /* % IACS (for reference)     */
    float           permeability_rel;       /* Relative permeability      */
    float           density_gcm3;           /* g/cm³                      */
    float           feature[8];             /* I/Q at 4 freqs             */
} alloy_entry_t;

/* Family name strings for UI display */
extern const char *alloy_family_names[];

/* The built-in reference database (stored in flash, mirrored in RAM) */
extern const alloy_entry_t alloy_database[ALLOY_DB_COUNT];

/* Lookup an alloy by index, returns NULL if out of range */
const alloy_entry_t *alloy_db_lookup(int index);

/* Find alloys by family, returns count found */
int alloy_db_find_by_family(alloy_family_t family, int *out_indices, int max_out);

/* Get a human-readable family name */
const char *alloy_family_name(alloy_family_t f);

#endif /* ALLOYSCAN_ALLOY_DB_H */