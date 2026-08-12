/*
 * acoustic.h — Acoustic emission insect detection (header)
 * Author: jayis1  Copyright (C) 2026 jayis1  License: GPL-2.0
 */
#ifndef GRAINGUARD_ACOUSTIC_H
#define GRAINGUARD_ACOUSTIC_H

#include <stdint.h>
#include <stdbool.h>

/* Known insect species signatures (dominant burst frequency) */
typedef enum {
    INSECT_NONE = 0,
    INSECT_SITOPHILUS_GRANARIUS = 1,   /* granary weevil, 26-30 kHz */
    INSECT_TRIBOLIUM_CASTANEUM   = 2,  /* red flour beetle, 42-48 kHz */
    INSECT_RHYZOPERTHA_DOMINICA  = 3,  /* lesser grain borer, 55-65 kHz */
    INSECT_UNKNOWN               = 0xFE
} insect_id_t;

typedef struct {
    uint16_t   events_per_min;     /* count of AE events during scan */
    uint16_t   peak_amplitude_mv;  /* max envelope amplitude */
    uint16_t   avg_event_duration_ms;
    insect_id_t species;           /* classified species, if confident */
    uint8_t    confidence_pct;     /* 0-100 */
} acoustic_result_t;

/* Power on the acoustic AFE and run a listening scan for AE_WINDOW_S seconds. */
int  acoustic_scan(acoustic_result_t *out);

/* On-demand spectral confirmation (raw 192 kS/s capture + FFT + classify). */
int  acoustic_spectral_confirm(acoustic_result_t *out);

void acoustic_power_off(void);
void acoustic_power_on(void);

#endif /* GRAINGUARD_ACOUSTIC_H */