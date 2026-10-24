/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef PICK_ENGINE_H
#define PICK_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define QUALIFY_MS 20u
#define MAX_BEAT_MS 10000u
#define LEFT_DOCK 1u
#define RIGHT_DOCK 2u
#define BEATER 4u
#define SHED 8u
/* Reasons are sticky until another event; never a fabric-quality verdict. */
typedef enum {
    NONE, ACCEPT, EMPTY_BEAT, REPEAT_SHED, BOTH_DOCKS,
    DOUBLE_PASS, SHED_MOVED, LATE_BEAT, SAMPLE_GAP, AMBIGUOUS,
    RETURNED, COUNTER_LIMIT, FLIGHT_EXPIRED
} Reason;
typedef struct {
    uint32_t now;
    uint32_t accepted;
    uint32_t rejected;
    uint32_t anomalies;
    uint32_t gaps;
    uint32_t events;
    uint32_t pending_age;
    uint32_t flight_age;
    uint32_t last_interval;
    uint32_t min_interval;
    uint32_t max_interval;
    uint32_t transport_losses;
    uint8_t raw;
    uint8_t stable;
    uint8_t candidate;
    uint8_t qualified_ms;
    uint8_t origin;
    uint8_t pending_shed;
    uint8_t flight_shed;
    uint8_t last_shed;
    bool ready;
    bool qualified;
    bool departed;
    bool pending;
    bool have_pick;
    bool saturated;
    Reason reason;
} Engine;
void engine_init(Engine *e, uint32_t now);
void engine_sample(Engine *e, uint8_t raw, uint32_t now);
void engine_transport_loss(Engine *e);
const char *engine_reason(Reason reason);
#endif
