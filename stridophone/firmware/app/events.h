/*
 * events.h — event queue, JSON packing, SD logging.
 * Author : jayis1
 * License: MIT
 */
#ifndef STRIDOPHONE_EVENTS_H
#define STRIDOPHONE_EVENTS_H

#include "../classify/forest.h"
#include <stdint.h>

typedef struct {
    uint32_t id;
    uint32_t timestamp_ms;
    int      species_id;
    int      confidence;
    int      doa_bin;
    float    temperature;
    float    humidity;
    float    pulse_rate;
    float    vibe_crest;
    float    vibe_kurtosis;
} event_t;

void events_init(void);
void event_build(event_t *e, const clf_result_t *clf,
                 const uint32_t *doa_hist, int nbins,
                 float temp_c, float rh_pct);
int  events_push(const event_t *e);
int  events_flush_sd(void);
int  events_pop_to_json(char *buf, int maxlen, int max_events);

#endif