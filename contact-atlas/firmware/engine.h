/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef ATLAS_ENGINE_H
#define ATLAS_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define ATLAS_MASKS 16u
#define ATLAS_QUEUE 16u
#define ATLAS_SETTLE_MS 200u
/* Every missing 1 ms sample destroys transition continuity. */
#define ATLAS_MAX_STEP_MS 1u
typedef struct {
    uint32_t visits;
    uint32_t dwell_ms;
} AtlasNode;
typedef struct {
    uint32_t visits;
    uint32_t minimum_ms;
    uint32_t maximum_ms;
    uint32_t zero_ms;
    uint32_t multi_ms;
} AtlasEdge;
typedef struct {
    uint32_t number;
    uint32_t time;
    uint32_t duration;
    uint32_t zero_ms;
    uint32_t multi_ms;
    uint32_t changes;
    uint8_t from;
    uint8_t to;
} AtlasEvent;
typedef struct {
    AtlasNode nodes[ATLAS_MASKS];
    AtlasEdge edges[ATLAS_MASKS][ATLAS_MASKS];
    AtlasEvent queue[ATLAS_QUEUE];
    uint32_t previous;
    uint32_t candidate_age;
    uint32_t journey_ms;
    uint32_t zero_ms;
    uint32_t multi_ms;
    uint32_t changes;
    uint32_t transitions;
    uint32_t gaps;
    uint32_t invalid;
    uint32_t dropped;
    uint8_t raw;
    uint8_t stable;
    uint8_t candidate;
    uint8_t head;
    uint8_t count;
    bool known;
    bool travelling;
    bool seeded;
} Atlas;
uint32_t atlas_add(uint32_t value, uint32_t increment);
unsigned atlas_population(uint8_t mask);
void atlas_init(Atlas *atlas, uint32_t now);
void atlas_sample(Atlas *atlas, uint32_t now, uint8_t mask);
bool atlas_pop(Atlas *atlas, AtlasEvent *event);
#endif
