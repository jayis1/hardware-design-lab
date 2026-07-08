/*
 * inference.h — Edge ML state classifier interface (CMSIS-NN).
 *
 * Author: jayis1
 * License: MIT
 */

#ifndef INFERENCE_H
#define INFERENCE_H

#include <stdint.h>

typedef struct {
    float fv_fm;            /* PSII max quantum yield    */
    float lndvi;            /* lichen NDVI                */
    float chl_index;        /* chlorophyll proxy          */
    float wetness;          /* 0..1 thallus conductance  */
    float acoustic_events;  /* desiccation click count    */
    float uv_index;         /* UV index (0..12)           */
} inference_features_t;

#define INFERENCE_STATE_HEALTHY    0
#define INFERENCE_STATE_STRESSED    1
#define INFERENCE_STATE_BLEACHING  2
#define INFERENCE_STATE_RECOVERING 3
#define INFERENCE_STATE_DORMANT     4

uint8_t inference_run(const inference_features_t *f, uint8_t *confidence);

#endif /* INFERENCE_H */