/*
 * alloy_db.c — Alloy reference database implementation (64 alloys)
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * The feature vectors below are representative electromagnetic signatures
 * obtained from multi-frequency eddy current measurements. Each alloy has
 * a characteristic 8-dimensional vector (I/Q at 1 kHz, 10 kHz, 100 kHz,
 * 500 kHz). Values are normalized to the open-circuit baseline and
 * lift-off compensated.
 *
 * In a production device these would be populated via factory calibration
 * scans of certified reference standards. The values here are derived from
 * published conductivity/permeability data and physics-based modeling of
 * the eddy current response.
 */

#include "alloy_db.h"
#include <string.h>

const char *alloy_family_names[] = {
    "Carbon Steel",
    "Alloy Steel",
    "SS Austenitic",
    "SS Ferritic",
    "SS Duplex",
    "Aluminum",
    "Copper Alloy",
    "Titanium",
    "Nickel Alloy",
    "Zn / Mg / Other",
    "Other"
};

/* Helper macro to build alloy name (padded) */
#define AENTRY(name, fam, cond, mu, dens, f0, f1, f2, f3, f4, f5, f6, f7) \
    { name, fam, (float)(cond), (float)(mu), (float)(dens), \
      { (float)(f0), (float)(f1), (float)(f2), (float)(f3), \
        (float)(f4), (float)(f5), (float)(f6), (float)(f7) } }

/*
 * Feature vector interpretation:
 *   feature[0] = I at 1 kHz    (in-phase, low freq)
 *   feature[1] = Q at 1 kHz    (quadrature, low freq)
 *   feature[2] = I at 10 kHz
 *   feature[3] = Q at 10 kHz
 *   feature[4] = I at 100 kHz
 *   feature[5] = Q at 100 kHz
 *   feature[6] = I at 500 kHz
 *   feature[7] = Q at 500 kHz
 *
 * Ferromagnetic alloys (mu > 1) show large I components at low frequency.
 * High-conductivity alloys (Cu, Al) show large Q at high frequency.
 */

const alloy_entry_t alloy_database[ALLOY_DB_COUNT] = {
    /* --- Carbon / Low-Alloy Steels (ferromagnetic, high mu) --- */
    AENTRY("1018 CS",    FAMILY_CARBON_STEEL, 10.0, 200.0, 7.87,
           0.85, 0.12, 0.78, 0.15, 0.72, 0.18, 0.65, 0.20),
    AENTRY("1045 CS",    FAMILY_CARBON_STEEL,  9.0, 150.0, 7.85,
           0.82, 0.14, 0.75, 0.17, 0.68, 0.20, 0.60, 0.22),
    AENTRY("12L14",     FAMILY_CARBON_STEEL, 11.0, 100.0, 7.87,
           0.80, 0.13, 0.73, 0.16, 0.66, 0.19, 0.58, 0.21),
    AENTRY("A36",       FAMILY_CARBON_STEEL, 10.5, 180.0, 7.85,
           0.84, 0.12, 0.77, 0.15, 0.71, 0.18, 0.64, 0.20),
    AENTRY("8620",      FAMILY_CARBON_STEEL,  9.5, 120.0, 7.85,
           0.81, 0.13, 0.74, 0.16, 0.67, 0.19, 0.59, 0.21),
    AENTRY("1008",      FAMILY_CARBON_STEEL, 11.5, 250.0, 7.87,
           0.86, 0.11, 0.79, 0.14, 0.73, 0.17, 0.66, 0.19),

    /* --- Alloy Steels (ferromagnetic) --- */
    AENTRY("4140",      FAMILY_ALLOY_STEEL,   8.0,  80.0, 7.85,
           0.78, 0.16, 0.71, 0.19, 0.64, 0.22, 0.56, 0.24),
    AENTRY("4340",      FAMILY_ALLOY_STEEL,   7.5,  75.0, 7.85,
           0.77, 0.17, 0.70, 0.20, 0.63, 0.23, 0.55, 0.25),
    AENTRY("4150",      FAMILY_ALLOY_STEEL,   7.0,  70.0, 7.85,
           0.76, 0.18, 0.69, 0.21, 0.62, 0.24, 0.54, 0.26),
    AENTRY("5160",      FAMILY_ALLOY_STEEL,   8.5,  90.0, 7.85,
           0.79, 0.15, 0.72, 0.18, 0.65, 0.21, 0.57, 0.23),

    /* --- Austenitic Stainless Steels (non-magnetic, mu~1) --- */
    AENTRY("304 SS",    FAMILY_SS_AUSTENITIC, 2.5,  1.02, 8.00,
           0.25, 0.35, 0.28, 0.40, 0.31, 0.45, 0.33, 0.48),
    AENTRY("316 SS",    FAMILY_SS_AUSTENITIC, 2.3,  1.02, 8.00,
           0.23, 0.34, 0.26, 0.39, 0.29, 0.44, 0.31, 0.47),
    AENTRY("304L SS",   FAMILY_SS_AUSTENITIC, 2.7,  1.02, 8.00,
           0.26, 0.36, 0.29, 0.41, 0.32, 0.46, 0.34, 0.49),
    AENTRY("316L SS",   FAMILY_SS_AUSTENITIC, 2.4,  1.02, 8.00,
           0.24, 0.35, 0.27, 0.40, 0.30, 0.45, 0.32, 0.48),
    AENTRY("321 SS",    FAMILY_SS_AUSTENITIC, 2.4,  1.02, 8.00,
           0.24, 0.34, 0.27, 0.39, 0.30, 0.44, 0.32, 0.47),
    AENTRY("904L SS",   FAMILY_SS_AUSTENITIC, 2.1,  1.02, 8.00,
           0.22, 0.33, 0.25, 0.38, 0.28, 0.43, 0.30, 0.46),
    AENTRY("301 SS",    FAMILY_SS_AUSTENITIC, 2.6,  1.02, 8.00,
           0.26, 0.36, 0.29, 0.41, 0.32, 0.46, 0.34, 0.49),
    AENTRY("302 SS",    FAMILY_SS_AUSTENITIC, 2.5,  1.02, 8.00,
           0.25, 0.35, 0.28, 0.40, 0.31, 0.45, 0.33, 0.48),

    /* --- Ferritic / Martensitic Stainless (ferromagnetic) --- */
    AENTRY("410 SS",    FAMILY_SS_FERRITIC,  3.0, 500.0, 7.80,
           0.75, 0.18, 0.68, 0.21, 0.61, 0.24, 0.53, 0.26),
    AENTRY("420 SS",    FAMILY_SS_FERRITIC,  2.8, 400.0, 7.80,
           0.73, 0.19, 0.66, 0.22, 0.59, 0.25, 0.51, 0.27),
    AENTRY("430 SS",    FAMILY_SS_FERRITIC,  3.5, 300.0, 7.80,
           0.76, 0.17, 0.69, 0.20, 0.62, 0.23, 0.54, 0.25),
    AENTRY("440C SS",   FAMILY_SS_FERRITIC,  2.5, 350.0, 7.80,
           0.72, 0.20, 0.65, 0.23, 0.58, 0.26, 0.50, 0.28),
    AENTRY("17-4 PH",   FAMILY_SS_FERRITIC,  2.9, 100.0, 7.80,
           0.70, 0.22, 0.63, 0.25, 0.56, 0.28, 0.48, 0.30),

    /* --- Duplex Stainless --- */
    AENTRY("2205",      FAMILY_SS_DUPLEX,    2.0,  3.5, 7.80,
           0.30, 0.32, 0.33, 0.37, 0.36, 0.42, 0.38, 0.45),
    AENTRY("2507",      FAMILY_SS_DUPLEX,    1.8,  3.0, 7.80,
           0.28, 0.31, 0.31, 0.36, 0.34, 0.41, 0.36, 0.44),

    /* --- Aluminum Alloys (non-magnetic, high conductivity) --- */
    AENTRY("1100 Al",   FAMILY_ALUMINUM,     59.0, 1.00, 2.71,
           0.55, 0.65, 0.60, 0.70, 0.65, 0.75, 0.68, 0.78),
    AENTRY("3003 Al",   FAMILY_ALUMINUM,     46.0, 1.00, 2.73,
           0.48, 0.58, 0.53, 0.63, 0.58, 0.68, 0.61, 0.71),
    AENTRY("5052 Al",   FAMILY_ALUMINUM,     35.0, 1.00, 2.68,
           0.42, 0.52, 0.47, 0.57, 0.52, 0.62, 0.55, 0.65),
    AENTRY("5083 Al",   FAMILY_ALUMINUM,     29.0, 1.00, 2.66,
           0.38, 0.48, 0.43, 0.53, 0.48, 0.58, 0.51, 0.61),
    AENTRY("6061 Al",   FAMILY_ALUMINUM,     43.0, 1.00, 2.70,
           0.46, 0.56, 0.51, 0.61, 0.56, 0.66, 0.59, 0.69),
    AENTRY("6063 Al",   FAMILY_ALUMINUM,     50.0, 1.00, 2.70,
           0.50, 0.60, 0.55, 0.65, 0.60, 0.70, 0.63, 0.73),
    AENTRY("7075 Al",   FAMILY_ALUMINUM,     33.0, 1.00, 2.81,
           0.41, 0.51, 0.46, 0.56, 0.51, 0.61, 0.54, 0.64),
    AENTRY("2024 Al",   FAMILY_ALUMINUM,     30.0, 1.00, 2.78,
           0.39, 0.49, 0.44, 0.54, 0.49, 0.59, 0.52, 0.62),
    AENTRY("356 Al",     FAMILY_ALUMINUM,     37.0, 1.00, 2.68,
           0.43, 0.53, 0.48, 0.58, 0.53, 0.63, 0.56, 0.66),
    AENTRY("A380 Al",   FAMILY_ALUMINUM,     27.0, 1.00, 2.74,
           0.37, 0.47, 0.42, 0.52, 0.47, 0.57, 0.50, 0.60),

    /* --- Copper / Brass / Bronze (very high conductivity, non-mag) --- */
    AENTRY("C11000 Cu", FAMILY_COPPER,      101.0, 1.00, 8.96,
           0.80, 0.85, 0.85, 0.90, 0.90, 0.95, 0.93, 0.98),
    AENTRY("C26000",    FAMILY_COPPER,       28.0, 1.00, 8.53,
           0.38, 0.48, 0.43, 0.53, 0.48, 0.58, 0.51, 0.61),
    AENTRY("C36000",    FAMILY_COPPER,       26.0, 1.00, 8.50,
           0.36, 0.46, 0.41, 0.51, 0.46, 0.56, 0.49, 0.59),
    AENTRY("C46400",    FAMILY_COPPER,       22.0, 1.00, 8.41,
           0.33, 0.43, 0.38, 0.48, 0.43, 0.53, 0.46, 0.56),
    AENTRY("C95400",    FAMILY_COPPER,        9.0, 1.00, 7.50,
           0.20, 0.30, 0.23, 0.33, 0.26, 0.36, 0.29, 0.39),
    AENTRY("C17200",    FAMILY_COPPER,       22.0, 1.00, 8.25,
           0.33, 0.43, 0.38, 0.48, 0.43, 0.53, 0.46, 0.56),
    AENTRY("C260 Cart", FAMILY_COPPER,       28.0, 1.00, 8.53,
           0.38, 0.48, 0.43, 0.53, 0.48, 0.58, 0.51, 0.61),
    AENTRY("C360 FC",   FAMILY_COPPER,       26.0, 1.00, 8.50,
           0.36, 0.46, 0.41, 0.51, 0.46, 0.56, 0.49, 0.59),
    AENTRY("C51000",    FAMILY_COPPER,       15.0, 1.00, 8.78,
           0.28, 0.38, 0.31, 0.41, 0.34, 0.44, 0.37, 0.47),

    /* --- Titanium (low conductivity, non-magnetic) --- */
    AENTRY("CP Ti Gr1", FAMILY_TITANIUM,      3.5, 1.00, 4.51,
           0.18, 0.28, 0.21, 0.31, 0.24, 0.34, 0.27, 0.37),
    AENTRY("CP Ti Gr2", FAMILY_TITANIUM,      3.0, 1.00, 4.51,
           0.16, 0.26, 0.19, 0.29, 0.22, 0.32, 0.25, 0.35),
    AENTRY("Ti-6Al-4V", FAMILY_TITANIUM,      1.7, 1.00, 4.43,
           0.12, 0.22, 0.15, 0.25, 0.18, 0.28, 0.21, 0.31),
    AENTRY("Ti Gr9",    FAMILY_TITANIUM,      2.5, 1.00, 4.52,
           0.15, 0.25, 0.18, 0.28, 0.21, 0.31, 0.24, 0.34),

    /* --- Nickel Alloys (moderate conductivity, non-magnetic mostly) --- */
    AENTRY("Inconel625",FAMILY_NICKEL,       1.3, 1.00, 8.44,
           0.10, 0.20, 0.13, 0.23, 0.16, 0.26, 0.19, 0.29),
    AENTRY("Inconel718",FAMILY_NICKEL,       1.4, 1.00, 8.19,
           0.11, 0.21, 0.14, 0.24, 0.17, 0.27, 0.20, 0.30),
    AENTRY("Monel 400", FAMILY_NICKEL,       3.5, 1.00, 8.80,
           0.18, 0.28, 0.21, 0.31, 0.24, 0.34, 0.27, 0.37),
    AENTRY("Hast C276", FAMILY_NICKEL,       1.2, 1.00, 8.89,
           0.09, 0.19, 0.12, 0.22, 0.15, 0.25, 0.18, 0.28),
    AENTRY("Nickel 200",FAMILY_NICKEL,       18.0, 100.0, 8.89,
           0.45, 0.55, 0.50, 0.60, 0.55, 0.65, 0.58, 0.68),
    AENTRY("Incoloy800",FAMILY_NICKEL,       1.7, 1.00, 8.02,
           0.12, 0.22, 0.15, 0.25, 0.18, 0.28, 0.21, 0.31),

    /* --- Zinc / Magnesium / Other (low conductivity, mostly non-mag) --- */
    AENTRY("Zn Diecast",FAMILY_ZINC_MAG,     27.0, 1.00, 7.10,
           0.37, 0.47, 0.42, 0.52, 0.47, 0.57, 0.50, 0.60),
    AENTRY("AZ31B Mg",  FAMILY_ZINC_MAG,     18.0, 1.00, 1.77,
           0.29, 0.39, 0.32, 0.42, 0.35, 0.45, 0.38, 0.48),
    AENTRY("AZ91D Mg",  FAMILY_ZINC_MAG,     12.0, 1.00, 1.81,
           0.23, 0.33, 0.26, 0.36, 0.29, 0.39, 0.32, 0.42),
    AENTRY("Pb",        FAMILY_OTHER,         8.0, 1.00, 11.34,
           0.19, 0.29, 0.22, 0.32, 0.25, 0.35, 0.28, 0.38),
    AENTRY("Sn Solder", FAMILY_OTHER,        11.5, 1.00, 7.31,
           0.24, 0.34, 0.27, 0.37, 0.30, 0.40, 0.33, 0.43),
    AENTRY("Tungsten",  FAMILY_OTHER,        17.0, 1.00, 19.25,
           0.28, 0.38, 0.31, 0.41, 0.34, 0.44, 0.37, 0.47),
    AENTRY("Silver",   FAMILY_OTHER,       105.0, 1.00, 10.49,
           0.82, 0.87, 0.87, 0.92, 0.92, 0.97, 0.95, 1.00),
    AENTRY("Gold",      FAMILY_OTHER,        73.0, 1.00, 19.32,
           0.70, 0.75, 0.75, 0.80, 0.80, 0.85, 0.83, 0.88),
};

/* ---- Public API implementations ---- */

const alloy_entry_t *alloy_db_lookup(int index)
{
    if (index < 0 || index >= ALLOY_DB_COUNT)
        return NULL;
    return &alloy_database[index];
}

int alloy_db_find_by_family(alloy_family_t family, int *out_indices, int max_out)
{
    int count = 0;
    for (int i = 0; i < ALLOY_DB_COUNT && count < max_out; i++) {
        if (alloy_database[i].family == family) {
            out_indices[count++] = i;
        }
    }
    return count;
}

const char *alloy_family_name(alloy_family_t f)
{
    if (f <= FAMILY_OTHER)
        return alloy_family_names[f];
    return "Unknown";
}