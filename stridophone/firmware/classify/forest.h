/*
 * forest.h — quantized decision-forest inference engine.
 * Author : jayis1
 * License: MIT
 */
#ifndef STRIDOPHONE_FOREST_H
#define STRIDOPHONE_FOREST_H

#include "../board.h"

typedef struct {
    int   species_id;     /* 0..CLF_N_CLASSES-1 */
    int   confidence;     /* percent 0..100 */
    float score;          /* raw vote fraction */
} clf_result_t;

void clf_init(void);

/* Run the forest on a 44-dim feature vector. Returns 1 if a valid
 * (non-ambient) class was selected, 0 otherwise. */
int clf_classify(const float *features, clf_result_t *out);

/* Map a species_id to a human-readable Latin name. */
const char *clf_species_name(int id);

#endif