/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef TAKEUP_ENGINE_H
#define TAKEUP_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
/* Policy is fixed for TKA1. Any policy change requires a new marker. */
#define CONFIRM_EDGES 3u
#define TIMEOUT_MS 10000u
typedef enum { PRIMING=0, TRACKING=1, TAKING_UP=2, FAULT=3 } State;
typedef struct {
    int32_t position;
    uint32_t invalid;
    uint8_t previous;
} Axis;
typedef struct {
    Axis axis[2];
    uint32_t time;
    uint32_t gaps;
    uint32_t events;
    uint32_t rejected;
    uint32_t reversals;
    uint32_t first;
    uint32_t confirm;
    uint32_t duration;
    uint32_t travel;
    uint32_t elapsed;
    uint32_t candidate_first;
    uint32_t following;
    int8_t direction;
    int8_t last_direction;
    State state;
    bool saturated;
} Engine;
void engine_init(Engine *e, uint32_t time, uint8_t raw);
void engine_sample(Engine *e, uint32_t time, uint8_t raw);
uint32_t engine_flags(const Engine *e);
#endif
